/**
 * Copyright 2013-2021 Software Radio Systems Limited
 *
 * This file is part of srsRAN.
 *
 * srsRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include "srsenb/hdr/stack/enb_stack_lte.h"
#include "srsenb/hdr/common/rnti_pool.h"
#include "srsenb/hdr/enb.h"
#include "srsran/interfaces/enb_metrics_interface.h"
#include "srsran/interfaces/enb_x2_interfaces.h"
#include "srsran/rlc/bearer_mem_pool.h"
#include "srsran/srslog/event_trace.h"

using namespace srsran;

static int wcb_tmp_conut = 0;

namespace srsenb
{

  uint32_t pdcp::pdcp_network_mode = 0;

  class gtpu_pdcp_adapter final : public gtpu_interface_pdcp, public pdcp_interface_gtpu
  {
  public:
    gtpu_pdcp_adapter(srslog::basic_logger &logger_,
                      pdcp *pdcp_lte,
                      pdcp_interface_gtpu *pdcp_x2,
                      gtpu *gtpu_,
                      enb_bearer_manager &bearers_) : logger(logger_), pdcp_obj(pdcp_lte), pdcp_x2_obj(pdcp_x2), gtpu_obj(gtpu_), bearers(&bearers_)
    {
    }

    /// Converts LCID to EPS-BearerID and sends corresponding PDU to GTPU
    void write_pdu(uint16_t rnti, uint32_t lcid, srsran::unique_byte_buffer_t pdu) override
    {
      auto bearer = bearers->get_lcid_bearer(rnti, lcid);
      if (not bearer.is_valid())
      {
        logger.error("Bearer rnti=0x%x, lcid=%d not found", rnti, lcid);
        return;
      }
      gtpu_obj->write_pdu(rnti, bearer.eps_bearer_id, std::move(pdu));
    }
    void write_sdu(uint16_t rnti, uint32_t eps_bearer_id, srsran::unique_byte_buffer_t sdu, int pdcp_sn = -1) override
    {
      auto bearer = bearers->get_radio_bearer(rnti, eps_bearer_id);
      // route SDU to PDCP entity
      if (bearer.rat == srsran_rat_t::lte)
      {
        pdcp_obj->write_sdu(rnti, bearer.lcid, std::move(sdu), pdcp_sn);
      }
      else if (bearer.rat == srsran_rat_t::nr)
      {
        pdcp_x2_obj->write_sdu(rnti, bearer.lcid, std::move(sdu), pdcp_sn);
      }
      else
      {
        logger.warning("Can't deliver SDU for EPS bearer %d. Dropping it.", eps_bearer_id);
      }
    }
    std::map<uint32_t, srsran::unique_byte_buffer_t> get_buffered_pdus(uint16_t rnti, uint32_t eps_bearer_id) override
    {
      auto bearer = bearers->get_radio_bearer(rnti, eps_bearer_id);
      // route SDU to PDCP entity
      if (bearer.rat == srsran_rat_t::lte)
      {
        return pdcp_obj->get_buffered_pdus(rnti, bearer.lcid);
      }
      else if (bearer.rat == srsran_rat_t::nr)
      {
        return pdcp_x2_obj->get_buffered_pdus(rnti, bearer.lcid);
      }
      logger.error("Bearer rnti=0x%x, eps-BearerID=%d not found", rnti, eps_bearer_id);
      return {};
    }

  private:
    srslog::basic_logger &logger;
    gtpu *gtpu_obj = nullptr;
    pdcp *pdcp_obj = nullptr;
    pdcp_interface_gtpu *pdcp_x2_obj = nullptr;
    enb_bearer_manager *bearers = nullptr;
  };

  enb_stack_lte::enb_stack_lte(srslog::sink &log_sink) : thread("STACK"),
                                                         mac_logger(srslog::fetch_basic_logger("MAC", log_sink)),
                                                         rlc_logger(srslog::fetch_basic_logger("RLC", log_sink, false)),
                                                         pdcp_logger(srslog::fetch_basic_logger("PDCP", log_sink, false)),
                                                         rrc_logger(srslog::fetch_basic_logger("RRC", log_sink, false)),
                                                         s1ap_logger(srslog::fetch_basic_logger("S1AP", log_sink, false)),
                                                         gtpu_logger(srslog::fetch_basic_logger("GTPU", log_sink, false)),
                                                         stack_logger(srslog::fetch_basic_logger("STCK", log_sink, false)),
                                                         task_sched(512, 128),
                                                         pdcp(&task_sched, pdcp_logger),
                                                         mac(&task_sched, mac_logger),
                                                         rlc(rlc_logger),
                                                         gtpu(&task_sched, gtpu_logger, &rx_sockets),
                                                         s1ap(&task_sched, s1ap_logger, &rx_sockets),
                                                         rrc(&task_sched, bearers),
                                                         mac_pcap(),
                                                         pending_stack_metrics(64)
  {
    get_background_workers().set_nof_workers(2);
    enb_task_queue = task_sched.make_task_queue();
    metrics_task_queue = task_sched.make_task_queue();
    // sync_queue is added in init()
  }

  enb_stack_lte::~enb_stack_lte()
  {
    stop();
  }

  std::string enb_stack_lte::get_type()
  {
    return "lte";
  }

  int enb_stack_lte::init(const stack_args_t &args_,
                          const rrc_cfg_t &rrc_cfg_,
                          phy_interface_stack_lte *phy_,
                          x2_interface *x2_, adp *adp_)
  {
    args = args_;
    rrc_cfg = rrc_cfg_;
    phy = phy_;
    stack_adp = adp_;
    setCfg();

    pdcp::pdcp_network_mode = rrc_cfg.network_mode;

    args_cnw.ttcn_nasmm_enble = args.ttcn_nasmm_enble;
    args_cnw.ttcn_nassm_enble = args.ttcn_nassm_enble;
    args_cnw.ttcn_test_enble = rrc_cfg.ttcn_test_enble;

    args.mac.pid = rrc_cfg.pid;
    std::cout << "args.mac.pid:" << args.mac.pid << std::endl;

    rrc_cfg.ttcn_rrc_enble = true;

    // Init RNTI and bearer memory pools
    reserve_rnti_memblocks(args.mac.nof_prealloc_ues);
    uint32_t min_nof_bearers_per_ue = 4;
    reserve_rlc_memblocks(args.mac.nof_prealloc_ues * min_nof_bearers_per_ue);

    // setup logging for each layer
    mac_logger.set_level(srslog::str_to_basic_level(args.log.mac_level));
    rlc_logger.set_level(srslog::str_to_basic_level(args.log.rlc_level));
    pdcp_logger.set_level(srslog::str_to_basic_level(args.log.pdcp_level));
    rrc_logger.set_level(srslog::str_to_basic_level(args.log.rrc_level));
    gtpu_logger.set_level(srslog::str_to_basic_level(args.log.gtpu_level));
    s1ap_logger.set_level(srslog::str_to_basic_level(args.log.s1ap_level));
    stack_logger.set_level(srslog::str_to_basic_level(args.log.stack_level));

    mac_logger.set_hex_dump_max_size(args.log.mac_hex_limit);
    rlc_logger.set_hex_dump_max_size(args.log.rlc_hex_limit);
    pdcp_logger.set_hex_dump_max_size(args.log.pdcp_hex_limit);
    rrc_logger.set_hex_dump_max_size(args.log.rrc_hex_limit);
    gtpu_logger.set_hex_dump_max_size(args.log.gtpu_hex_limit);
    s1ap_logger.set_hex_dump_max_size(args.log.s1ap_hex_limit);
    stack_logger.set_hex_dump_max_size(args.log.stack_hex_limit);

    // Set up pcap and trace
    if (args.mac_pcap.enable)
    {
      mac_pcap.open(args.mac_pcap.filename);
      mac.start_pcap(&mac_pcap);
    }

    if (args.mac_pcap_net.enable)
    {
      mac_pcap_net.open(args.mac_pcap_net.client_ip,
                        args.mac_pcap_net.bind_ip,
                        args.mac_pcap_net.client_port,
                        args.mac_pcap_net.bind_port);
      mac.start_pcap_net(&mac_pcap_net);
    }

    if (args.s1ap_pcap.enable)
    {
      s1ap_pcap.open(args.s1ap_pcap.filename.c_str());
      s1ap.start_pcap(&s1ap_pcap);
    }

    // add sync queue
    sync_task_queue = task_sched.make_task_queue(args.sync_queue_size);

    // add x2 queue
    if (x2_ != nullptr)
    {
      x2_task_queue = task_sched.make_task_queue();
    }

    // setup bearer managers
    gtpu_adapter.reset(new gtpu_pdcp_adapter(stack_logger, &pdcp, x2_, &gtpu, bearers));

    // Init all LTE layers
    if (!mac.init(args.mac, rrc_cfg.cell_list, phy, &rlc, &rrc, stack_adp))
    {
      stack_logger.error("Couldn't initialize MAC");
      return SRSRAN_ERROR;
    }
    rlc.init(&pdcp, &rrc, &mac, task_sched.get_timer_handler());
    pdcp.init(&rlc, &rrc, args.ttcn_pdcp_enble, gtpu_adapter.get(), stack_adp);
    // Init cnw
    // if (s_nas_cnw->init(args_cnw, &rrc, stack_adp))
    // {
    //   std::cout << "Error initializing CNW" << std::endl;
    //   return SRSRAN_ERROR;
    // }
    // 11/13
    if (rrc.init(rrc_cfg, phy, &mac, &rlc, &pdcp, &s1ap, &gtpu, stack_adp, x2_) != SRSRAN_SUCCESS)
    {
      stack_logger.error("Couldn't initialize RRC");
      return SRSRAN_ERROR;
    }
    if (args.mac_pcap_net.enable)
    {
      rrc.start_pcap_net(mac_pcap_net);
    }
    if (s1ap.init(args.s1ap, &rrc) != SRSRAN_SUCCESS)
    {
      stack_logger.error("Couldn't initialize S1AP");
      return SRSRAN_ERROR;
    }
    gtpu_args_t gtpu_args;
    gtpu_args.embms_enable = args.embms.enable;
    gtpu_args.embms_m1u_multiaddr = args.embms.m1u_multiaddr;
    gtpu_args.embms_m1u_if_addr = args.embms.m1u_if_addr;
    gtpu_args.mme_addr = args.s1ap.mme_addr;
    gtpu_args.gtp_bind_addr = args.s1ap.gtp_bind_addr;
    gtpu_args.indirect_tunnel_timeout_msec = args.gtpu_indirect_tunnel_timeout_msec;
    if (gtpu.init(gtpu_args, &pdcp) != SRSRAN_SUCCESS)
    {
      stack_logger.error("Couldn't initialize GTPU");
      return SRSRAN_ERROR;
    }
    started = true;
    start(STACK_MAIN_THREAD_PRIO);

    return SRSRAN_SUCCESS;
  }

  void enb_stack_lte::tti_clock()
  {
    if (started.load(std::memory_order_relaxed))
    {
      sync_task_queue.push([this]()
                           { 
                            tti_clock_impl(); });
    }
  }

  void enb_stack_lte::tti_clock_impl()
  {
    //*********TC618 start*******
    if (!stack_adp->udp_.mac_receive_info.empty() && (stack_adp->udp_.TC_618_mac_regular_pdu == true) || (stack_adp->udp_.TC_618_mac_regular_pdu_2 == true))
    {
      std::cout << "stack_adp->udp_.mac_receive_info.size() 1=" << stack_adp->udp_.mac_receive_info.size() << std::endl;
      srsran::unique_byte_buffer_t TC_618_DL_Info = srsran::make_byte_buffer();
      stack_adp->udp_.mac_receive_info.try_pop(TC_618_DL_Info);
      stack_adp->udp_.TTCN_Srnti = TC_618_DL_Info->msg[8];
      std::cout << "stack_adp->udp_.TTCN_Srnti=" << (int)stack_adp->udp_.TTCN_Srnti << std::endl;
      // trans data to pdcp
      srsran::unique_byte_buffer_t TC_618_DL_Info_Data = srsran::make_byte_buffer();
      memcpy(TC_618_DL_Info_Data->msg, TC_618_DL_Info->msg + 9, 1);
      std::cout << "TC_618_DL_Info_Data->msg[0]=" << TC_618_DL_Info_Data->msg[0] << std::endl;
      TC_618_DL_Info_Data->N_bytes = 1;
      pdcp.TC_6117(70, 3, std::move(TC_618_DL_Info_Data));   //这个函数就行，目的就是往下层递交
      std::cout << "stack_adp->udp_.mac_receive_info.size() 2=" << stack_adp->udp_.mac_receive_info.size() << std::endl;
    }
    //*********TC618 end*******

    //*********TC6114 start*******     //*********TC619 start*******     //*********TC6119 start*******
    if (!stack_adp->udp_.mac_receive_info.empty() && (stack_adp->udp_.TC_6114_process_subheader_dl == true || stack_adp->udp_.TC_619_mac_bsr_timer == true || stack_adp->udp_.TC_6119_ul_mcs == true))
    {
      std::cout << "stack_adp->udp_.mac_receive_info.size() 1=" << stack_adp->udp_.mac_receive_info.size() << std::endl;
      srsran::unique_byte_buffer_t TC_6114_DL_Info = srsran::make_byte_buffer();
      stack_adp->udp_.mac_receive_info.try_pop(TC_6114_DL_Info);

      stack_adp->udp_.TTCN_Srnti = TC_6114_DL_Info->msg[8];
      std::cout << "stack_adp->udp_.TTCN_Srnti=" << (int)stack_adp->udp_.TTCN_Srnti << std::endl;
      // trans data to pdcp
      srsran::unique_byte_buffer_t TC_6114_DL_Info_Data = srsran::make_byte_buffer();
      memcpy(TC_6114_DL_Info_Data->msg, TC_6114_DL_Info->msg + 9, 1);
      std::cout << "TC_6114_DL_Info_Data->msg[0]=" << TC_6114_DL_Info_Data->msg[0] << std::endl;
      TC_6114_DL_Info_Data->N_bytes = 1;
      pdcp.TC_6117(70, 3, std::move(TC_6114_DL_Info_Data));
      std::cout << "stack_adp->udp_.mac_receive_info.size() 2=" << stack_adp->udp_.mac_receive_info.size() << std::endl;
    }
    //*********TC6114 end*******    //*********TC619 end*******  //*********TC6119 end*******

    if (!stack_adp->udp_.mac_receive_info.empty() && stack_adp->udp_.TC_6115_mac_srnti_match == true)
    {
      std::cout << "stack_adp->udp_.mac_receive_info.size() 1=" << stack_adp->udp_.mac_receive_info.size() << std::endl;
      std::cout << "zvvvvvvvvvvvvvvvzzzzzz" << std::endl;
      srsran::unique_byte_buffer_t TC_6115_DL_Info = srsran::make_byte_buffer();
      stack_adp->udp_.mac_receive_info.try_pop(TC_6115_DL_Info);
      for (uint32_t i = 0; i < TC_6115_DL_Info->N_bytes; i++)
      {
        printf("0x:%x\n", TC_6115_DL_Info->msg[i]);
      }
      if (TC_6115_DL_Info->msg[7] == 0x0f)
      {
        std::cout << "aaaaaaaaaaaaaaaaaazzzzzzz" << std::endl;
        rrc.TC_6115_Reconfig(70);
      }
      else
      {
        stack_adp->udp_.TTCN_Srnti = TC_6115_DL_Info->msg[8];
        std::cout << "stack_adp->udp_.TTCN_Srnti=" << (int)stack_adp->udp_.TTCN_Srnti << std::endl;
        printf("stack_adp->udp_.TTCN_Srnti= 0x:%x\n", stack_adp->udp_.TTCN_Srnti);
        // trans data to pdcp
        srsran::unique_byte_buffer_t TC_6115_DL_Info_Data = srsran::make_byte_buffer();
        memcpy(TC_6115_DL_Info_Data->msg, TC_6115_DL_Info->msg + 9, 1);
        printf("TC_6115_DL_Info_Data->msg[0]= 0x:%x\n", TC_6115_DL_Info_Data->msg[0]);
        std::cout << "TC_6115_DL_Info_Data->msg[0]=" << TC_6115_DL_Info_Data->msg[0] << std::endl;
        TC_6115_DL_Info_Data->N_bytes = 1;

        // rlc.psch_test(70, 3, std::move(TC_6115_DL_Info_Data));
        pdcp.TC_6115(70, 3, std::move(TC_6115_DL_Info_Data));
      }
      std::cout << "stack_adp->udp_.mac_receive_info.size() 2=" << stack_adp->udp_.mac_receive_info.size() << std::endl;
    }
    if (!stack_adp->udp_.mac_receive_info.empty() && stack_adp->udp_.TC_6116_closed_loop_power_control == true)
    {
        std::cout << "stack_adp->udp_.mac_receive_info.size() 1=" << stack_adp->udp_.mac_receive_info.size() << std::endl;
        std::cout << "zvvvvvvvvvPAvvvvvvzzzzzz" << std::endl;
        srsran::unique_byte_buffer_t TC_6116_DL_Info = srsran::make_byte_buffer();
        stack_adp->udp_.mac_receive_info.try_pop(TC_6116_DL_Info);
        for (uint32_t i = 0; i < TC_6116_DL_Info->N_bytes; i++)
        {
            printf("0x:%x\n", TC_6116_DL_Info->msg[i]);
        }
        if (TC_6116_DL_Info->msg[7] == 0x0f)
        {
            std::cout << "aaaaaaaaaaaaaaaaaazzzzzzz" << std::endl;
            rrc.TC_6116_Reconfig(70);
        }
        else
        {
            stack_adp->udp_.TTCN_sacch = TC_6116_DL_Info->msg[9];
            std::cout << "stack_adp->udp_.TTCN_sacch=" << (int)stack_adp->udp_.TTCN_sacch << std::endl;
            printf("stack_adp->udp_.TTCN_sacch= 0x:%x\n", stack_adp->udp_.TTCN_sacch);
            // trans data to pdcp
            srsran::unique_byte_buffer_t TC_6116_DL_Info_Data = srsran::make_byte_buffer();
            memcpy(TC_6116_DL_Info_Data->msg, TC_6116_DL_Info->msg + 9, 1);
            printf("TC_6116_DL_Info_Data->msg[0]= 0x:%x\n", TC_6116_DL_Info_Data->msg[0]);
            std::cout << "TC_6116_DL_Info_Data->msg[0]=" << TC_6116_DL_Info_Data->msg[0] << std::endl;
            TC_6116_DL_Info_Data->N_bytes = 1;
            pdcp.TC_6116(70, 3, std::move(TC_6116_DL_Info_Data));
        }
        std::cout << "stack_adp->udp_.mac_receive_info.size() 2=" << stack_adp->udp_.mac_receive_info.size() << std::endl;
    }
    if (!stack_adp->udp_.mac_receive_info.empty() && stack_adp->udp_.TC_6117_ul_tf_sync == true)
    {
        std::cout << "stack_adp->udp_.mac_receive_info.size() 1=" << stack_adp->udp_.mac_receive_info.size() << std::endl;
        std::cout << "zvvvvvvvvvTAFAvvvvvvzzzzzz" << std::endl;
        srsran::unique_byte_buffer_t TC_6117_DL_Info = srsran::make_byte_buffer();
        stack_adp->udp_.mac_receive_info.try_pop(TC_6117_DL_Info);
        for (uint32_t i = 0; i < TC_6117_DL_Info->N_bytes; i++)
        {
            printf("0x:%x\n", TC_6117_DL_Info->msg[i]);
        }
        if (TC_6117_DL_Info->msg[7] == 0x0f)
        {
            std::cout << "aaaaaaaaaaaaaaaaaazzzzzzz" << std::endl;
            rrc.TC_6117_Reconfig(70);
        }
        else
        {
            stack_adp->udp_.TTCN_sacch = TC_6117_DL_Info->msg[9];
            std::cout << "stack_adp->udp_.TTCN_sacch=" << (int)stack_adp->udp_.TTCN_sacch << std::endl;
            printf("stack_adp->udp_.TTCN_sacch= 0x:%x\n", stack_adp->udp_.TTCN_sacch);
            // trans data to pdcp
            srsran::unique_byte_buffer_t TC_6117_DL_Info_Data = srsran::make_byte_buffer();
            memcpy(TC_6117_DL_Info_Data->msg, TC_6117_DL_Info->msg + 9, 1);
            printf("TC_6117_DL_Info_Data->msg[0]= 0x:%x\n", TC_6117_DL_Info_Data->msg[0]);
            std::cout << "TC_6117_DL_Info_Data->msg[0]=" << TC_6117_DL_Info_Data->msg[0] << std::endl;
            TC_6117_DL_Info_Data->N_bytes = 1;
            pdcp.TC_6117(70, 3, std::move(TC_6117_DL_Info_Data));
        }
        std::cout << "stack_adp->udp_.mac_receive_info.size() 2=" << stack_adp->udp_.mac_receive_info.size() << std::endl;
    }
    if (!stack_adp->udp_.mac_receive_info.empty() && stack_adp->udp_.TC_617_map_dtch == true)
    {
      std::cout << "stack_adp->udp_.mac_receive_info.size() 1=" << stack_adp->udp_.mac_receive_info.size() << std::endl;
      srsran::unique_byte_buffer_t TC_617_DL_Info = srsran::make_byte_buffer();
      stack_adp->udp_.mac_receive_info.try_pop(TC_617_DL_Info);
      for (uint32_t i = 0; i < TC_617_DL_Info->N_bytes; i++)
      {
        printf("0x:%x\n", TC_617_DL_Info->msg[i]);
      }

      stack_adp->udp_.TTCN_Srnti = TC_617_DL_Info->msg[8];
      std::cout << "stack_adp->udp_.TTCN_Srnti=" << (int)stack_adp->udp_.TTCN_Srnti << std::endl;
      // trans data to pdcp
      srsran::unique_byte_buffer_t TC_617_DL_Info_Data = srsran::make_byte_buffer();
      memcpy(TC_617_DL_Info_Data->msg, TC_617_DL_Info->msg + 9, 1);
      std::cout << "TC_617_DL_Info_Data->msg[0]=" << TC_617_DL_Info_Data->msg[0] << std::endl;
      TC_617_DL_Info_Data->N_bytes = 1;
      pdcp.TC_617(70, 3, std::move(TC_617_DL_Info_Data));
      std::cout << "stack_adp->udp_.mac_receive_info.size() 2=" << stack_adp->udp_.mac_receive_info.size() << std::endl;
    }
    if (!stack_adp->udp_.mac_receive_info.empty() && stack_adp->udp_.TC_6113_padding_dl == true)
    {
      std::cout << "stack_adp->udp_.mac_receive_info.size() 1=" << stack_adp->udp_.mac_receive_info.size() << std::endl;
      srsran::unique_byte_buffer_t TC_6113_DL_Info = srsran::make_byte_buffer();
      stack_adp->udp_.mac_receive_info.try_pop(TC_6113_DL_Info);
      for (uint32_t i = 0; i < TC_6113_DL_Info->N_bytes; i++)
      {
        printf("0xdl6113:%x\n", TC_6113_DL_Info->msg[i]);
      }

      stack_adp->udp_.TTCN_Srnti = TC_6113_DL_Info->msg[8];
      std::cout << "stack_adp->udp_.TTCN_Srnti=" << (int)stack_adp->udp_.TTCN_Srnti << std::endl;
      // trans data to pdcp
      srsran::unique_byte_buffer_t TC_6113_DL_Info_Data = srsran::make_byte_buffer();
      memcpy(TC_6113_DL_Info_Data->msg, TC_6113_DL_Info->msg + 9, 1);
      std::cout << "TC_6113_DL_Info_Data->msg[0]=" << TC_6113_DL_Info_Data->msg[0] << std::endl;
      TC_6113_DL_Info_Data->N_bytes = 1;
      pdcp.TC_6113(70, 3, std::move(TC_6113_DL_Info_Data));
      std::cout << "stack_adp->udp_.mac_receive_info.size() 2=" << stack_adp->udp_.mac_receive_info.size() << std::endl;
    }
    if (stack_adp->udp_.TC_628_RECONFIG)
    {
      std::cout << "[TEST][RLC][TTCN][RECONF]" << std::endl;
      stack_adp->udp_.TC_628_RECONFIG = false;
      rrc.wx_Mcontrol_Notify_Switch(70, 0);
    }

    if (stack_adp->udp_.TC_722_ho_success)
    {
      std::cout << "[TEST][RRC][TTCN][RECONF]" << std::endl;
      stack_adp->udp_.TC_722_ho_success = false;
      rrc.wx_Mcontrol_Notify_Switch(70, 0);
    }
    
    if (stack_adp->udp_.TC_720_measure_report_first)
    {
      std::cout << "[TEST][RRC][TTCN][TC_720_measure_report]" << std::endl;
      stack_adp->udp_.TC_720_measure_report_first = false;
      stack_adp->udp_.TC_720_measure_report_last = true;
      rrc.rrctorrc_ue(70);
    }
    if (stack_adp->udp_.TC_716_reconf_fail_first)
    {
      std::cout << "[TEST][RRC][TTCN][TC_716_reconf_fail_first]" << std::endl;
      stack_adp->udp_.TC_716_reconf_fail_first = false;
      stack_adp->udp_.TC_716_reconf_fail_last = true;
      rrc.rrctorrc_ue(70);
    }
  
    if (!stack_adp->udp_.target_beam_receive_queue.empty())
    {
      std::cout << "[enb_stack_lte][stack_adp][target_beam_receive_queue]" << std::endl;
      // TO DO RRC
      srsran::unique_byte_buffer_t source_beam_msg = srsran::make_byte_buffer();
      stack_adp->udp_.target_beam_receive_queue.try_pop(source_beam_msg);
      int len = source_beam_msg->N_bytes;
      if (len > 2)
      {
        switch (source_beam_msg->msg[2])
        {
        case 0x00: // source beam ho request message
          std::cout << " rrc.wx_Trigger_switch_ACK(70,source_beam_msg->msg[2]); " << std::endl;
          rrc.wx_Trigger_switch_ACK(70, std::move(source_beam_msg));
          break;
        default:
          printf("error: This message is unrealized!\n");
          break;
        }
        printf("source_beam_msg->N_bytes;:%d\n", len);
        // if(len==5){
        //   printf("tcc 722 beam2 flag,pid:%d\n",rrc_cfg.pid);
        //    rrc.rrc_adp->udp_.TC_722_ho_success_last=true;
        //  }
      }
      else
      {
        printf("error: received source beam msg size < 2 !\n");
      }
    }
 
    if (!stack_adp->udp_.source_beam_receive_queue.empty())
    {
      std::cout << "[enb_stack_lte][stack_adp][source_beam_receive_queue]" << std::endl;
      // TO DO RRC
      srsran::unique_byte_buffer_t target_beam_msg = srsran::make_byte_buffer();
      stack_adp->udp_.source_beam_receive_queue.try_pop(target_beam_msg);

      int len = target_beam_msg->N_bytes;
      std::cout << "target_beam_msg->msg[1]:" << target_beam_msg->msg[2] << std::endl;
      if (len > 2)
      {
        switch (target_beam_msg->msg[2])
        {
        case 0x01: // target beam ho ack message
          std::cout << "  rrc.wx_Handle_switch_ACK(70,target_beam_msg->msg[2]); " << std::endl;
          rrc.wx_Handle_switch_ACK(70, std::move(target_beam_msg));
        case 0x02:
          std::cout << " case 0x02: wx_Source_Beam_release " << std::endl;
          rrc.wx_Source_Beam_release(70);
          break;
        default:
          printf("error: This message is unrealized!\n");
          break;
        }
      }
      else
      {
        printf("error: received target beam msg size < 2 !\n");
      }
    }

    // ATE
    if (stack_adp->udp_.stack_receive_ate_info.size() != 0)
    {
      printf("stack_adp->udp_.stack_receive_ate_info\n");
      srsran::unique_byte_buffer_t ate_msg = srsran::make_byte_buffer();
      stack_adp->udp_.stack_receive_ate_info.try_pop(ate_msg);
      /*print ate msg*/
      int len = ate_msg->N_bytes;
      for (int i = 0; i < len; i++)
      {
        printf("ate_ims_msg[%d]: ox%x\n", i, ate_msg->msg[i]);
      }
      if (len > 2)
      {
        switch (ate_msg->msg[1])
        {
        case 0x00: // ims message
          break;

        case 0x01: // nas message
          break;

        case 0x02: // rrc message
          rrc.rrc_handle_ate_msg(std::move(ate_msg));
          break;

        default:
          printf("error: This message doesn't belong on any level or unrealized!\n");
          break;
        }
      }
      else
      {
        printf("error: received ate msg size < 2 !\n");
      }
    }

    if (stack_adp->udp_.received_cnw_info.size() != 0)
    {
      // receive cnw msg
      srsran::unique_byte_buffer_t cnw_msg = srsran::make_byte_buffer();
      stack_adp->udp_.received_cnw_info.try_pop(cnw_msg);
      handle_cnw_pdu(cnw_msg);
    }

    if (stack_adp->udp_.is_zhj_psch_test == true)
    {
      std::cout << " PSCH TEST !!!! " << std::endl;
      stack_adp->udp_.is_zhj_psch_test = false;
      srsran::unique_byte_buffer_t psch_data_sdu = srsran::make_byte_buffer();
      psch_data_sdu->N_bytes = 7;
      psch_data_sdu->msg[0] = 0x80;
      psch_data_sdu->msg[1] = 0x00;
      psch_data_sdu->msg[2] = 0xaa;
      psch_data_sdu->msg[3] = 00;
      psch_data_sdu->msg[4] = 00;
      psch_data_sdu->msg[5] = 00;
      psch_data_sdu->msg[6] = 00;
      rlc.psch_test(70, 3, std::move(psch_data_sdu));
    }
   
    if (stack_adp->udp_.is_ttcn_release_paging == true)
    {
      stack_adp->udp_.is_ttcn_release_paging = false;
      rrc.ttcn_control_release_paging(70);
    }

    if (stack_adp->udp_.second_ttcn_paging == true)
    {
      stack_adp->udp_.second_ttcn_paging = false;
      while (true)
      {
        if (stack_adp->udp_.rrc_receive_info.size() != 0)
        {
          srsran::unique_byte_buffer_t ttcn_second_paging = srsran::make_byte_buffer();
          std::cout << "second_ttcn_paging receive rrc paging info from TTCN" << std::endl;
          ttcn_second_paging->init();
          stack_adp->udp_.rrc_receive_info.try_pop(ttcn_second_paging);
          rrc.get_general_interface(&ttcn_second_paging);

          srsran::unique_byte_buffer_t ttcn_second_paging_pdu = srsran::make_byte_buffer();
          ttcn_second_paging_pdu->init();

          ttcn_second_paging_pdu->msg[0] = ttcn_second_paging->msg[8];
          ttcn_second_paging_pdu->msg[1] = ttcn_second_paging->msg[9];
          ttcn_second_paging_pdu->msg[2] = ttcn_second_paging->msg[10];
          ttcn_second_paging_pdu->msg[3] = ttcn_second_paging->msg[11];
          ttcn_second_paging_pdu->msg[4] = ttcn_second_paging->msg[12];
          ttcn_second_paging_pdu->msg[5] = ttcn_second_paging->msg[13];
          ttcn_second_paging_pdu->N_bytes = 6;

          rrc.add_paging_id_wx_s(std::move(ttcn_second_paging_pdu));

          if (stack_adp->udp_.enble_ttcn_flag_.ttcn_testId==73)
          {
            stack_adp->udp_.TC_73_second_t300_con_req=true;
          }
               
          if (stack_adp->udp_.TC_74_t302_timeout == true)
          {
            stack_adp->udp_.TC_74_t302_timeout = false;
            stack_adp->udp_.TC_74_second_t302_con_req = true;
          }

          std::cout << "udp_.TC_74_second_t302_con_req" << stack_adp->udp_.TC_74_second_t302_con_req << std::endl;
          break;
        }
      }
    }
    // 4.10 XK pdcp
    
    if (stack_adp->udp_.pid == 2)
    {
      std::cout << "stack_adp->udp_.pdcp_ttcn_test=" << stack_adp->udp_.pdcp_ttcn_test << std::endl;
      if (stack_adp->udp_.pdcp_ttcn_test == true)
      {
        std::cout << "ZHJ AA" << std::endl;
      }
    }
   
    if (stack_adp->udp_.pdcp_ttcn_test == true)
    {
      std::cout << "stack_adp->udp_.pid=" << stack_adp->udp_.pid << std::endl;
      std::cout << std::endl
                << "------- PDCP TTCN TEST flag -------" << std::endl;
      stack_adp->udp_.pdcp_ttcn_test = false;
      pdcp.pdcp_ttcn_test();
    }
    else if (stack_adp->udp_.sdap_ttcn_test == true)
    {
      std::cout << std::endl
                << "------- SDAP TTCN TEST flag -------" << std::endl;
      stack_adp->udp_.sdap_ttcn_test = false;
      pdcp.SDAP_TTCN_TEST();
    }

    if (stack_adp->udp_.TC_513_sib_barred == true)
    {
      stack_adp->udp_.TC_513_sib_barred = false;
      stack_adp->udp_.TC_513_mib_sib = true;
      rrc.second_configure_mib();
      rrc.second_generate_sib();
      stack_adp->udp_.TC_513_mib_sib = false;
    }

    if (stack_adp->udp_.TC_725_second_release == true)
    {
      std::cout << "   xxk 1111 " << std::endl;
      stack_adp->udp_.TC_725_second_release = false;
      stack_adp->udp_.TC_725_release = true;
      rrc.send_second_release();
      stack_adp->udp_.TC_725_release = false;
    }
  
    // std::cout<<"-----------------------tti--network_mode----------------------"<<args.network_mode<<std::endl;
    task_sched.tic();
    if (args.network_mode != 0) // �����Ϊ������
    {
      rrc.iot_tti_clock_s();
    }
    else
    { // �������н�����
      rrc.tti_clock_s();
    }
    // rrc.tti_clock_s();
    // rrc.tti_clock();
  }

  void enb_stack_lte::stop()
  {
    if (started)
    {
      enb_task_queue.push([this]()
                          { stop_impl(); });
      wait_thread_finish();
    }
  }

  void enb_stack_lte::stop_impl()
  {
    std::cout << "adafafaf" << std::endl;
    rx_sockets.stop();

    s1ap.stop();
    gtpu.stop();
    mac.stop();
    rlc.stop();
    pdcp.stop();
    rrc.stop();

    if (args.mac_pcap.enable)
    {
      mac_pcap.close();
    }

    if (args.mac_pcap_net.enable)
    {
      mac_pcap_net.close();
    }

    if (args.s1ap_pcap.enable)
    {
      s1ap_pcap.close();
    }

    task_sched.stop();
    get_background_workers().stop();

    started = false;
  }

  bool enb_stack_lte::get_metrics(stack_metrics_t *metrics)
  {
    // use stack thread to query metrics
    auto ret = metrics_task_queue.try_push([this]()
                                           {
    stack_metrics_t metrics{};
    mac.get_metrics(metrics.mac);
    if (not metrics.mac.ues.empty()) {
      rlc.get_metrics(metrics.rlc, metrics.mac.ues[0].nof_tti);
      pdcp.get_metrics(metrics.pdcp, metrics.mac.ues[0].nof_tti);
    }
    rrc.get_metrics(metrics.rrc);
    s1ap.get_metrics(metrics.s1ap);
    if (not pending_stack_metrics.try_push(metrics)) {
      stack_logger.error("Unable to push metrics to queue");
    } });

    if (ret.has_value())
    {
      // wait for result
      *metrics = pending_stack_metrics.pop_blocking();
      return true;
    }
    return false;
  }

  void enb_stack_lte::run_thread()
  {
    while (started.load(std::memory_order_relaxed))
    {
      task_sched.run_next_task();
    }
  }

  void enb_stack_lte::write_pdu(uint16_t rnti, uint32_t lcid, srsran::unique_byte_buffer_t pdu)
  {
    // call GTPU adapter to map to EPS bearer
    auto task = [this, rnti, lcid](srsran::unique_byte_buffer_t &pdu)
    {
      gtpu_adapter->write_pdu(rnti, lcid, std::move(pdu));
    };
    x2_task_queue.push(std::bind(task, std::move(pdu)));
  }
  void enb_stack_lte::setCfg()
  {
    mac.mac_cfg.agch.FrameAssign = (int)rrc_cfg.sib.rr_cfg_com.agch_cfg_com.agch_fram_ass.to_number();
    mac.mac_cfg.agch.SlotStart = (int)rrc_cfg.sib.rr_cfg_com.agch_cfg_com.agch_slot_start.to_number();
    mac.mac_cfg.agch.BandID = (int)rrc_cfg.sib.rr_cfg_com.agch_cfg_com.band_id.ba_id.to_number();
    mac.mac_cfg.agch.freq = (int)rrc_cfg.sib.rr_cfg_com.agch_cfg_com.freq_id.freq_id.to_number();
    mac.mac_cfg.bbch.BandID = (int)rrc_cfg.sib.rr_cfg_com.bbch_cfg_com.band_id.ba_id.to_number();
    mac.mac_cfg.bbch.freq = (int)rrc_cfg.sib.rr_cfg_com.bbch_cfg_com.freq_id.freq_id.to_number();
    mac.mac_cfg.bbch.FrameAssign = (int)rrc_cfg.sib.rr_cfg_com.bbch_cfg_com.bbch_frame_ass.to_number();
    mac.mac_cfg.bbch.SlotStart = (int)rrc_cfg.sib.rr_cfg_com.bbch_cfg_com.bbch_slot_ass.to_number();
    mac.mac_cfg.sib.BandID = (int)rrc_cfg.mib.bcch_band_id.ba_id.to_number();
    mac.mac_cfg.sib.freq = (int)rrc_cfg.mib.bcch_fre_id.freq_id.to_number();
    mac.mac_cfg.sib.SlotStart = (int)rrc_cfg.mib.bcch_slot_start.to_number();
    mac.mac_cfg.rar_windows = (int)rrc_cfg.sib.rr_cfg_com.rach_cfg_com.ra_res_wi_si.to_number();
    mac.mac_cfg.pcch.BandID = (int)rrc_cfg.mib.pcch_band_id.ba_id.to_number();
    mac.mac_cfg.pcch.freq = (int)rrc_cfg.mib.pcch_fre_id.freq_id.to_number();
    mac.mac_cfg.Frame_Off = (int)rrc_cfg.mib.frame_off.to_number();
    std::cout << "mac.mac_cfg.Frame_Off=" << mac.mac_cfg.Frame_Off << std::endl;
    // IoT-SI
    if (rrc_cfg.sib.rr_cfg_com.iotsi_cfg_present && args.network_mode != 0)
    {
      std::cout << "This is Iot-MODE, Config Iot SI" << std::endl;
      mac.mac_cfg.IoTsi.BandID = (int)rrc_cfg.sib.rr_cfg_com.iotsi_cfg.iotsi_cfg_normal().dl_band_id.ba_id.to_number();
      mac.mac_cfg.IoTsi.freq = (int)rrc_cfg.sib.rr_cfg_com.iotsi_cfg.iotsi_cfg_normal().dl_freq_id.freq_id.to_number();
      mac.mac_cfg.IoTsi.FrameAssign = (int)rrc_cfg.sib.rr_cfg_com.iotsi_cfg.iotsi_cfg_normal().fn_ass.to_number();
    }
    // Pag-SI
    // mac.mac_cfg.pagsi.BandID = (int)rrc_cfg.mib.bcch_band_id.ba_id.to_number();
    // mac.mac_cfg.pagsi.freq   = (int)rrc_cfg.mib.bcch_fre_id.freq_id.to_number();
    // mac.mac_cfg.pagsi.FrameAssign = (int)rrc_cfg.mib.frame_off.to_number();
    // mac.mac_cfg.pagsi.SlotStart   = (int)rrc_cfg.mib.bcch_slot_start.to_number();
  }

  void enb_stack_lte::set_pcap_remote_addr(std::string &ip, uint16_t port)
  {
    mac_pcap_net.set_remote(ip, port);
  }
  void enb_stack_lte::test_pcap_pkt()
  {
    char p_data[] = "1223344556";
    for (int i = 0; i < 10; i++)
    {
      // srsran::console("send data: %s", p_data);
      mac_pcap_net.write_dl_rrc_pdu((unsigned char *)p_data, strlen(p_data), CY_LOGICCHANNEL_TYPE_MBCH, CY_NET_MODE_IOT);
    }
  }

  bool enb_stack_lte::handle_cnw_pdu(srsran::unique_byte_buffer_t &s1_pdu)
  {
    std::cout << "[enb]: handle_cnw_pdu function!" << std::endl;
    if (s1_pdu->N_bytes < 4)
    {
      std::cout << "[enb]: received error enb msg! cause:s1_size < 4" << std::endl;
      return false;
    }
    srsepc::enb_msg_header_t *enb_head = (srsepc::enb_msg_header_t *)s1_pdu->msg;
    std::cout << "[enb]: " << "enb_id: " << enb_head->enb_id << "rnti:" << enb_head->rnti << std::endl;

    srsran::unique_byte_buffer_t pdu = srsran::make_byte_buffer();
    pdu->N_bytes = s1_pdu->N_bytes - 4;
    memcpy(pdu->msg, s1_pdu->msg + 4, pdu->N_bytes);

    asn1::s1ap::init_ctxt_setup_req_s ctx_setup_req;

    switch (enb_head->msg_type)
    {
    case srsepc::enb_msg_type::nas_dl:
      std::cout << "[enb]: nas_dl " << std::endl;
      rrc.s_write_dl_info(enb_head->rnti, std::move(pdu));
      break;
    case srsepc::enb_msg_type::ims_dl:
      std::cout << "[enb]: ims_dl " << std::endl;
      rrc.s_write_ims_dl_info(enb_head->rnti, std::move(pdu));
      break;
    case srsepc::enb_msg_type::setup_ue_ctxt:
      std::cout << "[enb]: setup_ue_ctxt " << std::endl;
      memcpy(ctx_setup_req.security_key.data(), s1_pdu->msg + 4, 32);
      rrc.s_setup_ue_ctxt(enb_head->rnti, ctx_setup_req);
      break;
    case srsepc::enb_msg_type::rrc_reconfig:
      std::cout << "[enb]: rrc_reconfig " << std::endl;
      if (pdu->N_bytes < 3)
        return false;
      {
        uint8_t qos = pdu->msg[0];
        uint8_t pdu_id = pdu->msg[1];
        uint8_t am_tm_type = pdu->msg[2];
        const srsran::unique_byte_buffer_t nas_msg = srsran::make_byte_buffer();
        nas_msg->N_bytes = pdu->N_bytes - 3;
        memcpy(nas_msg->msg, pdu->msg + 3, nas_msg->N_bytes);
        srsran::const_byte_span nas_pdu = srsran::make_span(nas_msg);
        rrc.smm_notify_ue_erab_updates(enb_head->rnti, qos, pdu_id, nas_pdu, am_tm_type);
      }
      break;
    case srsepc::enb_msg_type::start_rem_user:
      std::cout << "[enb]: start_rem_user " << std::endl;
      rrc.start_rem_user(enb_head->rnti);
      break;
    case srsepc::enb_msg_type::start_rem_rel_user:
      std::cout << "[enb]: start_rem_rel_user " << std::endl;
      rrc.start_rem_rel_user(enb_head->rnti);
      break;
    case srsepc::enb_msg_type::test_reg_ss_no5Gguti:
      std::cout << "[enb]: test_reg_ss_no5Gguti " << std::endl;
      rrc.testcase_reg_ss_no5Gguti(enb_head->rnti);
      break;
    case srsepc::enb_msg_type::wx_Mcontrol_Notify_Switch:
      std::cout << "[enb]: wx_Mcontrol_Notify_Switch " << std::endl;
      if (pdu->N_bytes < 1)
        return false;
      {
        uint8_t Switch_Type = pdu->msg[0];
        uint8_t Rach_Type = pdu->msg[1];
        if (Rach_Type==RACH_HANDOVER)
        {
          std::cout<<"is rach handover 1"<<std::endl;
          stack_adp->udp_.RACH_Handover=true;
        }
        rrc.wx_Mcontrol_Notify_Switch(enb_head->rnti, Switch_Type);
      }
      break;
    case srsepc::enb_msg_type::test_smm_notify_ue_erab_updates:
      std::cout << "[enb]: test_smm_notify_ue_erab_updates " << std::endl;
      if (pdu->N_bytes < 3)
        return false;
      {
        uint8_t qos = pdu->msg[0];
        uint8_t pdu_id = pdu->msg[1];
        uint8_t am_tm_type = pdu->msg[2];
        const srsran::unique_byte_buffer_t nas_msg = srsran::make_byte_buffer();
        nas_msg->N_bytes = pdu->N_bytes - 3;
        memcpy(nas_msg->msg, pdu->msg + 3, nas_msg->N_bytes);
        srsran::const_byte_span nas_pdu = srsran::make_span(nas_msg);
        rrc.test_smm_notify_ue_erab_updates(enb_head->rnti, qos, pdu_id, nas_pdu, am_tm_type);
      }
      break;
    case srsepc::enb_msg_type::rrc_reconfig_voice:
      std::cout << "[enb]: rrc_reconfig_voice " << std::endl;
      if (pdu->N_bytes < 4)
        return false;
      {
        uint8_t qos = pdu->msg[0];
        uint8_t pdu_id = pdu->msg[1];
        uint8_t am_tm_type = pdu->msg[2];
        stack_adp->udp_.is_Control = true;
        stack_adp->udp_.voice_indicate = pdu->msg[3];
        std::cout << "enb voice rate: " << stack_adp->udp_.voice_indicate << std::endl;
        const srsran::unique_byte_buffer_t nas_msg = srsran::make_byte_buffer();
        nas_msg->N_bytes = pdu->N_bytes - 4;
        memcpy(nas_msg->msg, pdu->msg + 4, nas_msg->N_bytes);
        srsran::const_byte_span nas_pdu = srsran::make_span(nas_msg);
        rrc.smm_notify_ue_erab_updates(enb_head->rnti, qos, pdu_id, nas_pdu, am_tm_type);
      }
      break;
    case srsepc::enb_msg_type::nas_to_notify_rrc_release:
      std::cout << "[enb]: nas_to_notify_rrc_release " << std::endl;
      rrc.nas_to_notify_rrc_release(enb_head->rnti);
      break;
    case srsepc::enb_msg_type::ip_data:
      wcb_tmp_conut++;
      std::cout << "[enb]: ip_data " << " wcb_tmp_conut: " << wcb_tmp_conut << std::endl;
      pdcp.write_sdu(pdcp.icmp_rnti, 3, std::move(pdu), -1);
      break;
    case srsepc::enb_msg_type::rrc_rebuild_fail:
      printf("restablish test succsess!zzzz\n");
      if (mac.reestablish_test())
      {
        printf("restablish test succsess!\n");
      }
      break;
    case srsepc::enb_msg_type::nas_pcap_dl: //cbs
      std::cout << "[enb]: nas_pcap_dl " << std::endl;
      if (args.mac_pcap_net.enable)
      {
        mac_pcap_net.write_dl_nas_pdu(pdu->msg, pdu->N_bytes, CY_NET_MODE_RAN);
      }
      break;
    case srsepc::enb_msg_type::nas_pcap_ul:
      std::cout << "[enb]: nas_pcap_ul " << std::endl;
      if (args.mac_pcap_net.enable)
      {
        mac_pcap_net.write_ul_nas_pdu(pdu->msg, pdu->N_bytes, CY_NET_MODE_RAN);
      }
      break;
    
    default:
      printf("restablish test %d\n", enb_head->msg_type);
      break;
    }
    return true;
  }
} // namespace srsenb
