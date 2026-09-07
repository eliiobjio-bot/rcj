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

#include "srsenb/hdr/stack/rrc/rrc_ue.h"
#include "srsenb/hdr/common/common_enb.h"
#include "srsenb/hdr/stack/rrc/mac_controller.h"
#include "srsenb/hdr/stack/rrc/rrc_endc.h"
#include "srsenb/hdr/stack/rrc/rrc_mobility.h"
#include "srsenb/hdr/stack/rrc/ue_rr_cfg.h"
#include "srsran/asn1/rrc_utils.h"
#include "srsran/common/enb_events.h"
#include "srsran/common/standard_streams.h"
#include "srsran/interfaces/enb_pdcp_interfaces.h"
#include "srsran/interfaces/enb_rlc_interfaces.h"
#include "srsran/interfaces/enb_s1ap_interfaces.h"
#include "srsran/support/srsran_assert.h"
#include "srsran/asn1/rrc/dl_ccch_msg.h"
// ##########################################
#include "srsran/interfaces/enb_mac_interfaces.h"
#include <chrono>
#include <iostream>
#include <fstream>
#include <regex>

using namespace asn1::rrc;

namespace srsenb
{

  /*******************************************************************************
    UE class

    Every function in UE class is called from a mutex environment thus does not
    need extra protection.
  *******************************************************************************/

  rrc::ue::ue(rrc *outer_rrc, uint16_t rnti_, const sched_interface::ue_cfg_t &sched_ue_cfg) : parent(outer_rrc),
                                                                                               rnti(rnti_),
                                                                                               phy_rrc_dedicated_list(sched_ue_cfg.supported_cc_list.size()),
                                                                                               ue_cell_list(parent->cfg, *outer_rrc->cell_res_list, *outer_rrc->cell_common_list),
                                                                                               bearer_list(rnti_, parent->cfg, outer_rrc->gtpu),
                                                                                               ue_security_cfg(parent->cfg),
                                                                                               mac_ctrl(rnti, ue_cell_list, bearer_list, parent->cfg, parent->mac, *parent->cell_common_list, sched_ue_cfg)
  {
  }

  rrc::ue::~ue() {}

  bool rrc::ue::init_pucch()
  {
    // Allocate PUCCH resources for PCell
    return ue_cell_list.init_pucch_pcell();
  }

  int rrc::ue::init()
  {
    // Allocate cell (PUCCH resources are not allocated here)
    if (ue_cell_list.add_cell(mac_ctrl.get_ue_sched_cfg().supported_cc_list[0].enb_cc_idx, false) == nullptr)
    {
      return SRSRAN_ERROR;
    }

    // Configure
    apply_setup_phy_common(parent->cfg.sibs[1].sib2().rr_cfg_common, true);

    phy_dl_rlf_timer = parent->task_sched.get_unique_timer();
    phy_ul_rlf_timer = parent->task_sched.get_unique_timer();
    rlc_rlf_timer = parent->task_sched.get_unique_timer();
    activity_timer = parent->task_sched.get_unique_timer();
    set_activity_timeout(MSG3_RX_TIMEOUT); // next UE response is Msg3

    // Set timeout to release UE context after RLF detection
    uint32_t deadline_ms = parent->cfg.rlf_release_timer_ms;
    if (rnti != SRSRAN_MRNTI)
    {
      auto timer_expire_func = [this](uint32_t tid)
      { rlf_timer_expired(tid); };
      phy_dl_rlf_timer.set(deadline_ms, timer_expire_func);
      phy_ul_rlf_timer.set(deadline_ms, timer_expire_func);
      rlc_rlf_timer.set(deadline_ms, timer_expire_func);
      parent->logger.info("Setting RLF timer for rnti=0x%x to %dms", rnti, deadline_ms);
    }
    else
    {
      // in case of M-RNTI do not handle rlf timer expiration
      auto timer_expire_func = [](uint32_t tid) {};
      phy_dl_rlf_timer.set(deadline_ms, timer_expire_func);
      phy_ul_rlf_timer.set(deadline_ms, timer_expire_func);
      rlc_rlf_timer.set(deadline_ms, timer_expire_func);
    }

    mobility_handler = make_rnti_obj<rrc_mobility>(rnti, this);
    if (parent->rrc_nr != nullptr)
    {
      endc_handler = make_rnti_obj<rrc_endc>(rnti, this, parent->cfg.endc_cfg);
    }

    return SRSRAN_SUCCESS;
  }

  rrc_state_t rrc::ue::get_state()
  {
    return state;
  }

  void rrc::ue::get_metrics(rrc_ue_metrics_t &ue_metrics) const
  {
    ue_metrics.state = state;
    const auto &drb_list = bearer_list.get_established_drbs();
    const auto &erab_list = bearer_list.get_erabs();
    ue_metrics.drb_qci_map.reserve(drb_list.size());
    for (size_t i = 0; i < drb_list.size(); ++i)
    {
      auto erab_it = erab_list.find(drb_list[i].eps_bearer_id);
      if (erab_it != erab_list.end())
      {
        ue_metrics.drb_qci_map.push_back(std::make_pair(drb_list[i].lc_ch_id, erab_it->second.qos_params.qci));
      }
    }
  }

  void rrc::ue::set_activity(bool enabled)
  {
    if (rnti == SRSRAN_MRNTI)
    {
      return;
    }
    if (not enabled)
    {
      if (activity_timer.is_running())
      {
        parent->logger.debug("Inactivity timer interrupted for rnti=0x%x", rnti);
      }
      activity_timer.stop();
      return;
    }

    // re-start activity timer with current timeout value
    activity_timer.run();
    parent->logger.debug("Activity registered for rnti=0x%x (timeout_value=%dms)", rnti, activity_timer.duration());
  }

  void rrc::ue::set_radiolink_dl_state(bool crc_res)
  {
    parent->logger.debug(
        "Radio-Link downlink state for rnti=0x%x: crc_res=%d, consecutive_ko=%d", rnti, crc_res, consecutive_kos_dl);

    // If received OK, restart DL counter and stop RLF timer
    if (crc_res)
    {
      consecutive_kos_dl = 0;
      if (phy_dl_rlf_timer.is_running())
      {
        parent->logger.info(
            "DL RLF timer stopped for rnti=0x%x (time elapsed=%dms)", rnti, phy_dl_rlf_timer.time_elapsed());
        phy_dl_rlf_timer.stop();
        mac_ctrl.set_radio_bearer_state(mac_lc_ch_cfg_t::BOTH);
      }
      return;
    }

    // Count KOs in MAC and trigger release if it goes above a certain value.
    // This is done to detect out-of-coverage UEs
    if (phy_dl_rlf_timer.is_running())
    {
      // RLF timer already running, no need to count KOs
      return;
    }

    consecutive_kos_dl++;
    if (consecutive_kos_dl > parent->cfg.max_mac_dl_kos)
    {
      parent->logger.info("Max KOs in DL reached, starting RLF timer rnti=0x%x", rnti);
      mac_ctrl.set_radio_bearer_state(mac_lc_ch_cfg_t::IDLE);
      phy_dl_rlf_timer.run();
    }
  }

  void rrc::ue::set_radiolink_ul_state(bool crc_res)
  {
    parent->logger.debug(
        "Radio-Link uplink state for rnti=0x%x: crc_res=%d, consecutive_ko=%d", rnti, crc_res, consecutive_kos_ul);

    // If received OK, restart UL counter and stop RLF timer
    if (crc_res)
    {
      consecutive_kos_ul = 0;
      if (phy_ul_rlf_timer.is_running())
      {
        parent->logger.info(
            "UL RLF timer stopped for rnti=0x%x (time elapsed=%dms)", rnti, phy_ul_rlf_timer.time_elapsed());
        phy_ul_rlf_timer.stop();
        mac_ctrl.set_radio_bearer_state(mac_lc_ch_cfg_t::BOTH);
      }
      return;
    }

    if (mobility_handler->is_ho_running())
    {
      // Do not count UL KOs if handover is on-going.
      // Source eNB should only rely in relocation timer
      // Target eNB should either wait for UE to handover or explicit release by the MME
      return;
    }

    // Count KOs in MAC and trigger release if it goes above a certain value.
    // This is done to detect out-of-coverage UEs
    if (phy_ul_rlf_timer.is_running())
    {
      // RLF timer already running, no need to count KOs
      return;
    }

    consecutive_kos_ul++;
    if (consecutive_kos_ul > parent->cfg.max_mac_ul_kos)
    {
      parent->logger.info("Max KOs in UL reached, starting RLF timer rnti=0x%x", rnti);
      mac_ctrl.set_radio_bearer_state(mac_lc_ch_cfg_t::IDLE);
      phy_ul_rlf_timer.run();
    }
  }

  void rrc::ue::activity_timer_expired(const activity_timeout_type_t type)
  {
    parent->logger.info("Activity timer for rnti=0x%x expired after %d ms", rnti, activity_timer.time_elapsed());

    if (true)
    { //(parent->s1ap->user_exists(rnti)) {
      switch (type)
      {
      case UE_INACTIVITY_TIMEOUT:
        // parent->s1ap->user_release(rnti, asn1::s1ap::cause_radio_network_opts::user_inactivity);
        // parent->cnw->s_user_release(rnti);
        parent->process_release_complete_s(rnti);
        con_release_result = procedure_result_code::activity_timeout;
        break;
      case MSG3_RX_TIMEOUT:
      case MSG5_RX_TIMEOUT:
        // MSG3 timeout, no need to notify S1AP, just remove UE
        parent->rem_user_thread(rnti);
        con_release_result = procedure_result_code::msg3_timeout;
        break;
      default:
        // Unhandled activity timeout, just remove UE and log an error
        parent->rem_user_thread(rnti);
        con_release_result = procedure_result_code::activity_timeout;
        parent->logger.error(
            "Unhandled reason for activity timer expiration. rnti=0x%x, cause %d", rnti, static_cast<unsigned>(type));
      }
    }
    else
    {
      parent->rem_user_thread(rnti);
    }

    state = RRC_STATE_RELEASE_REQUEST;
  }

  void rrc::ue::rlf_timer_expired(uint32_t timeout_id)
  {
    activity_timer.stop();

    std::string event_type = "Unknown";
    if (timeout_id == phy_dl_rlf_timer.id())
    {
      event_type = "dl_rlf";
      parent->logger.info("DL RLF timer for rnti=0x%x expired after %d ms", rnti, phy_dl_rlf_timer.time_elapsed());
    }
    else if (timeout_id == phy_ul_rlf_timer.id())
    {
      event_type = "ul_rlf";
      parent->logger.info("UL RLF timer for rnti=0x%x expired after %d ms", rnti, phy_ul_rlf_timer.time_elapsed());
    }
    else if (timeout_id == rlc_rlf_timer.id())
    {
      event_type = "rlc_rlf";
      parent->logger.info("RLC RLF timer for rnti=0x%x expired after %d ms", rnti, rlc_rlf_timer.time_elapsed());
    }

    phy_ul_rlf_timer.stop();
    phy_dl_rlf_timer.stop();
    rlc_rlf_timer.stop();
    state = RRC_STATE_RELEASE_REQUEST;

    parent->s1ap->user_release(rnti, asn1::s1ap::cause_radio_network_opts::radio_conn_with_ue_lost);
    con_release_result = procedure_result_code::radio_conn_with_ue_lost;

    // Log event.
    event_logger::get().log_rlf_detected(
        ue_cell_list.get_ue_cc_idx(UE_PCELL_CC_IDX)->cell_common->enb_cc_idx, event_type, rnti);
  }

  void rrc::ue::max_rlc_retx_reached()
  {
    parent->logger.info("Max RLC retx reached for rnti=0x%x", rnti);

    // Turn off scheduling but give UE chance to start re-establishment
    mac_ctrl.set_radio_bearer_state(mac_lc_ch_cfg_t::IDLE);
    rlc_rlf_timer.run();
  }

  void rrc::ue::protocol_failure()
  {
    parent->logger.info("RLC protocol failure for rnti=0x%x", rnti);

    // Release UE immediately with appropiate cause
    state = RRC_STATE_RELEASE_REQUEST;

    parent->s1ap->user_release(rnti, asn1::s1ap::cause_radio_network_opts::fail_in_radio_interface_proc);
    con_release_result = procedure_result_code::fail_in_radio_interface_proc;
  }

  void rrc::ue::set_activity_timeout(activity_timeout_type_t type)
  {
    uint32_t deadline_ms = 0;

    switch (type)
    {
    case MSG3_RX_TIMEOUT:
      deadline_ms = static_cast<uint32_t>(
          (get_ue_cc_cfg(UE_PCELL_CC_IDX)->sib2.rr_cfg_common.rach_cfg_common.max_harq_msg3_tx + 1) * 16);
      break;
    case UE_INACTIVITY_TIMEOUT:
      deadline_ms = parent->cfg.inactivity_timeout_ms;
      break;
    case MSG5_RX_TIMEOUT:
      deadline_ms = get_ue_cc_cfg(UE_PCELL_CC_IDX)->sib2.ue_timers_and_consts.t301.to_number();
      break;
    default:
      parent->logger.error("Unknown timeout type %d", type);
    }

    activity_timer.set(deadline_ms, [this, type](uint32_t tid)
                       { activity_timer_expired(type); });
    parent->logger.debug("Setting timer for %s for rnti=0x%x to %dms", to_string(type).c_str(), rnti, deadline_ms);

    set_activity();
  }

  bool rrc::ue::is_connected()
  {
    return state == RRC_STATE_REGISTERED;
  }

  bool rrc::ue::is_idle()
  {
    return state == RRC_STATE_IDLE;
  }
  // #####################################################################
  void rrc::ue::assemble_header_com(
      srsran::unique_byte_buffer_t *rrc_con_com_ttcn_)
  {
    rrc_con_com_ttcn_->get()->msg[0] = 0x00;
    rrc_con_com_ttcn_->get()->msg[1] = 0x00;
    rrc_con_com_ttcn_->get()->msg[2] = 0x00;
    rrc_con_com_ttcn_->get()->msg[3] = 0x04; // message type     rrc_con_com
    rrc_con_com_ttcn_->get()->msg[4] = 0x00; // msg[4] [5]??????
    rrc_con_com_ttcn_->get()->msg[5] = 0x02;
  }

  void rrc::ue::parse_ul_dcch_s(uint32_t lcid, srsran::unique_byte_buffer_t pdu)
  {
    s_ul_dcch_msg_s s_ul_dcch_msg;

    // s_dl_dcch_msg_s  s_dl_dcch_msg;
    asn1::cbit_ref bref(pdu->msg, pdu->N_bytes);
    if (s_ul_dcch_msg.unpack(bref) != asn1::SRSASN_SUCCESS)
    {
      parent->log_rx_pdu_fail(rnti, lcid, *pdu, "Failed to unpack UL-DCCH message");
      return;
    }

    if (parent->cfg.pid == 1)
    {
      std::cout << "[Compelete Info][PID=1]" << std::endl;
    }
    else
    {
      std::cout << "[Compelete Info][PID=2]" << std::endl;
    }

    std::cout << "-------------------Compelete Info--------------------------------" << std::endl;
    // srsran::console("当前字节数=%u\n", pdu->N_bytes);   //打印解析的数据
    //  for (uint8_t i=0; i < pdu->N_bytes; i++)
    //  {
    //  srsran::console("%x\n", *(pdu->msg + i));
    //  }

    // nas info to nas_pdu
    srsran::unique_byte_buffer_t nas_pdu = srsran::make_byte_buffer();
    nas_pdu->init();

    //-------------2024.03.05-------------
    srsran::unique_byte_buffer_t scm_pdu = srsran::make_byte_buffer();
    scm_pdu->init();
    //----------------------------------------

    // std::cout<<"------------@@@@@@@@@@---------sss--"<<std::endl;
    //  for(uint32_t i=0;i<30;i++)
    //  {
    //    printf("0x%x\n",*(nas_pdu->msg+i));
    //  }

    parent->log_rrc_message(Rx, rnti, lcid, *pdu, s_ul_dcch_msg, s_ul_dcch_msg.msg.type().to_string());
    srsran::unique_byte_buffer_t original_pdu = std::move(pdu);
    pdu = srsran::make_byte_buffer();
    if (pdu == nullptr)
    {
      parent->logger.error("Couldn't allocate PDU in %s().", __FUNCTION__);
      return;
    }
    // std::cout<<"---------@@@@@@@@@@@@@s_ul_dcch_msg.msg.type()@@@@@@@@----:"<<s_ul_dcch_msg.msg.type()<<std::endl;

    // std::cout<<"---------------------s_ul_dcch_msg.msg.type()--------------------------"<<s_ul_dcch_msg.msg.type()<<std::endl;

    srsran::rrc_pcap_net *p_pcap_net = getPcapNet(*parent);
    if (p_pcap_net)
    {
      p_pcap_net->write_ul_rrc_pdu(original_pdu->msg, original_pdu->N_bytes, CY_LOGICCHANNEL_TYPE_UL_DCCH, CY_NET_MODE_RAN);
    }

    switch (s_ul_dcch_msg.msg.type())
    {
    case s_ul_dcch_msg_type_c::types::rrc_con_setup_comp:
      if (parent->cfg.ttcn_rrc_enble && parent->cfg.ttcn_test_enble && (parent->rrc_adp->udp_.is_connection || parent->rrc_adp->udp_.TC_71_is_paging_connection || parent->rrc_adp->udp_.TC_79_is_paging_smc || parent->rrc_adp->udp_.TC_72_con_capability || parent->rrc_adp->udp_.TC_711_reconfig || parent->rrc_adp->udp_.TC_517_paging_success || parent->rrc_adp->udp_.TC_710_security_mode_failure || parent->rrc_adp->udp_.TC_713_reconfig_DRB || parent->rrc_adp->udp_.TC_714_DRB_Release || parent->rrc_adp->udp_.TC_715_reest_reconf))
      {
        srsran::unique_byte_buffer_t rrc_con_com_ttcn = srsran::make_byte_buffer();
        rrc_con_com_ttcn->init();

        rrc_con_com_ttcn->msg[7] = RRC_CON_SETUP_COMP;

        rrc_con_com_ttcn->N_bytes = 8;

        parent->assemble_general_interface(&rrc_con_com_ttcn, 8);

        parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rrc_con_com_ttcn));
        std::cout << "send_ttcn_info size:"
                  << parent->rrc_adp->udp_.send_ttcn_info.size() << std::endl;
      }

      nas_pdu->N_bytes = s_ul_dcch_msg.msg.rrc_con_setup_comp().rrc_con_setup_com_r1.ded_info_nas.ded_info_nas.size();
      std::cout << "@@@@----nas_pdu->N_bytes:" << nas_pdu->N_bytes << std::endl;
      nas_pdu->msg = s_ul_dcch_msg.msg.rrc_con_setup_comp().rrc_con_setup_com_r1.ded_info_nas.ded_info_nas.data();
      // for (uint32_t i = 0; i < nas_pdu->N_bytes; i++) {
      //   printf("0x%x\n", *(nas_pdu->msg + i));
      // }

      // parse complete info
      printf("-------------rrc_t_id:%d\n", s_ul_dcch_msg.msg.rrc_con_setup_comp().rrc_transaction_id.rrc_t_id);
      printf("-------------selecte_plmn_id:%d\n",
             s_ul_dcch_msg.msg.rrc_con_setup_comp().rrc_con_setup_com_r1.selecte_plmn_id);
      printf("-------------ded_info_nas_present:%d\n",
             s_ul_dcch_msg.msg.rrc_con_setup_comp().rrc_con_setup_com_r1.ded_info_nas_present);
      std::cout << "-------ded_info_nas:"
                << s_ul_dcch_msg.msg.rrc_con_setup_comp().rrc_con_setup_com_r1.ded_info_nas.ded_info_nas.to_string()
                << std::endl;

      save_ul_message_s(std::move(original_pdu));
      handle_rrc_con_setup_comp(&s_ul_dcch_msg.msg.rrc_con_setup_comp(), std::move(pdu));
      std::cout << "----连接建立完成----" << std::endl;
      /*nas initial_ue message*/
      std::cout << "----Send the nas initialization message------" << std::endl;
      // parent->cnw->initial_ue(rnti, std::move(nas_pdu));
      initial_ue_to_nas(rnti, std::move(nas_pdu));
      set_activity();
      {
        srsran::unique_byte_buffer_t current_status = srsran::make_byte_buffer();
        current_status->init();
        current_status->N_bytes = 4;
        current_status->msg[0] = 0xff;
        current_status->msg[1] = 0x02;
        current_status->msg[2] = TC_MSG_RRC_ATE_CON;
        current_status->msg[3] = 0x01;

        // send to ate msg queue
        // parent->rrc_adp->udp_.send_ate_info.try_push(std::move(current_status));
        parent->rrc_adp->udp_.send_ate_msg(std::move(current_status));
      }
      break;

      // zhj 11/27
    case s_ul_dcch_msg_type_c::types::ul_info_trans:
      ultrans_number++;

      //-------------------------2024.03.05-------------------------
      scm_pdu->N_bytes = s_ul_dcch_msg.msg.ul_info_trans().ul_info_trans_r1.ded_info_type.ded_info_scm().ded_info_scm.size();
      scm_pdu->msg = s_ul_dcch_msg.msg.ul_info_trans()
                         .ul_info_trans_r1.ded_info_type
                         .ded_info_scm()
                         .ded_info_scm.data();

      // scm_pdu->msg=s_ul_dcch_msg.msg.ul_info_trans().ul_info_trans_r1.ded_info_type.ded_info_scm().ded_info_scm.data();

      std::cout << "--------@@@@@@@@@----ultrans_number----@@@@@@@@----------:" << ultrans_number << std::endl;
      nas_pdu->N_bytes = s_ul_dcch_msg.msg.ul_info_trans().ul_info_trans_r1.ded_info_type.ded_info_nas().ded_info_nas.size();
      // std::cout << "ul_info_trans---nas_pdu->N_bytes:" << nas_pdu->N_bytes << std::endl;
      nas_pdu->msg = s_ul_dcch_msg.msg.ul_info_trans().ul_info_trans_r1.ded_info_type.ded_info_nas().ded_info_nas.data();
      // for (uint32_t i = 0; i < nas_pdu->N_bytes; i++) {
      //   printf("0x%x\n", *(nas_pdu->msg + i));
      // }
      std::cout << "qiqiqiqiqiqi++::" << (int)s_ul_dcch_msg.msg.ul_info_trans().ul_info_trans_r1.ded_info_type.type() << std::endl;
      if (s_ul_dcch_msg.msg.ul_info_trans().ul_info_trans_r1.ded_info_type.type() == ded_info_type_c::types_opts::ded_info_nas)
      {
        std::cout << "zoucuole----222222------" << std::endl;
        pdu->N_bytes =
            s_ul_dcch_msg.msg.ul_info_trans()
                .ul_info_trans_r1.ded_info_type
                .ded_info_nas()
                .ded_info_nas.size();

        memcpy(pdu->msg,
               s_ul_dcch_msg.msg.ul_info_trans()
                   .ul_info_trans_r1.ded_info_type
                   .ded_info_nas()
                   .ded_info_nas.data(),
               pdu->N_bytes);
        // parent->cnw->write_pdu(rnti, std::move(pdu));
        write_pdu_to_nas(rnti, std::move(pdu));
      }
      if (s_ul_dcch_msg.msg.ul_info_trans().ul_info_trans_r1.ded_info_type.type() == ded_info_type_c::types_opts::ded_info_scm)
      {
        std::cout << "zoucuole--11111--222222------" << std::endl;
        pdu->N_bytes =
            s_ul_dcch_msg.msg.ul_info_trans()
                .ul_info_trans_r1.ded_info_type
                .ded_info_scm()
                .ded_info_scm.size();

        memcpy(pdu->msg,
               s_ul_dcch_msg.msg.ul_info_trans()
                   .ul_info_trans_r1.ded_info_type
                   .ded_info_scm()
                   .ded_info_scm.data(),
               pdu->N_bytes);
        // parent->cnw->rrc_to_pcs_ims(rnti, std::move(pdu));
        write_pdu_to_ims(rnti, std::move(pdu));
      }
      //  switch(ul_info_trans.ded_info_type.type())
      //  {
      //   case ul_info_trans.ded_info_type.type().ded_info_nas:
      //    std::cout<<"zoucuole----222222------"<<std::endl;
      //      pdu->N_bytes =
      //         s_ul_dcch_msg.msg.ul_info_trans()
      //         .ul_info_trans_r1.ded_info_type
      //         .ded_info_nas().ded_info_nas.size();

      //     memcpy(pdu->msg,
      //            s_ul_dcch_msg.msg.ul_info_trans()
      //         .ul_info_trans_r1.ded_info_type
      //         .ded_info_nas().ded_info_nas.data(),
      //            pdu->N_bytes);
      //     parent->cnw->write_pdu(rnti, std::move(pdu));
      //     break;

      //   case ul_info_trans.ded_info_type.type().ded_info_scm:
      //    std::cout<<"zoucuole-111111111---------"<<std::endl;
      //         pdu->N_bytes =
      //         s_ul_dcch_msg.msg.ul_info_trans()
      //         .ul_info_trans_r1.ded_info_type
      //         .ded_info_scm().ded_info_scm.size();

      //     memcpy(pdu->msg,
      //            s_ul_dcch_msg.msg.ul_info_trans()
      //         .ul_info_trans_r1.ded_info_type
      //         .ded_info_scm().ded_info_scm.data(),
      //            pdu->N_bytes);
      //     parent->cnw->rrc_to_pcs_ims(rnti, std::move(pdu));
      //     break;
      //     default:
      //     std::cout<<"zoucuole-huhuhuhuhhuhuhuhu---------"<<std::endl;
      //      break;
      //  }
      break;

      // 11/30 zhj
    case s_ul_dcch_msg_type_c::types::security_mode_comp:

      if (parent->cfg.ttcn_rrc_enble && parent->cfg.ttcn_test_enble && (parent->rrc_adp->udp_.TC_79_is_paging_smc || parent->rrc_adp->udp_.TC_711_reconfig || parent->rrc_adp->udp_.TC_713_reconfig_DRB || parent->rrc_adp->udp_.TC_714_DRB_Release || parent->rrc_adp->udp_.TC_715_reest_reconf))
      {
        srsran::unique_byte_buffer_t rrc_smc_com_ttcn = srsran::make_byte_buffer();
        rrc_smc_com_ttcn->init();

        rrc_smc_com_ttcn->msg[7] = RRC_SEC_MODE_COMP;

        rrc_smc_com_ttcn->msg[8] = 0;
        rrc_smc_com_ttcn->msg[9] = 0;

        rrc_smc_com_ttcn->N_bytes = 10;

        parent->assemble_general_interface(&rrc_smc_com_ttcn, 10);

        parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rrc_smc_com_ttcn));
        std::cout << "send_ttcn_info size:"
                  << parent->rrc_adp->udp_.send_ttcn_info.size() << std::endl;
      }

      s_handle_security_mode_complete(&s_ul_dcch_msg.msg.security_mode_comp());
      // parent->cnw->send_reg_accept(rnti);

      // parent->cnw->send_reg_or_service_accept(rnti);
      notify_send_reg_or_service_accept(rnti);
      std::cout << "----------security mode complete--------------" << std::endl;

      break;
    case s_ul_dcch_msg_type_c::types::security_mode_fail:
      if (parent->cfg.ttcn_rrc_enble && parent->cfg.ttcn_test_enble && parent->rrc_adp->udp_.TC_710_security_mode_failure)
      {
        srsran::unique_byte_buffer_t rrc_smf_ttcn = srsran::make_byte_buffer();
        rrc_smf_ttcn->init();
        rrc_smf_ttcn->msg[7] = RRC_SEC_MODE_FAIL; // security mode failure
        rrc_smf_ttcn->msg[8] = 0;
        rrc_smf_ttcn->msg[9] = 0;
        rrc_smf_ttcn->N_bytes = 10;
        parent->assemble_general_interface(&rrc_smf_ttcn, 10);
        parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rrc_smf_ttcn));
        std::cout << "rrc_smf_ttcn size:"
                  << parent->rrc_adp->udp_.send_ttcn_info.size() << std::endl;
      }
      s_handle_security_mode_failure(&s_ul_dcch_msg.msg.secur_mode_fail());
      break;

    // 12.14 zyq
    case s_ul_dcch_msg_type_c::types::rrc_con_recon_comp:
      std::cout << "-----------rrc_con_recon_comp---------" << std::endl;
      // 当在目标波束上接收到重配时，通知源波束发送RRC释放命令
      if (parent->is_multi == true)
      {
        parent->mac->RRC_notify_MAC_release(parent->is_multi);
        parent->is_multi = false;
      }
      if (parent->cfg.pid == 2)
      {
        wx_target_Beam_notify_Source_Beam_To_Release();
        std::cout << "发送通知源波束资源释放命令" << std::endl;
      }
      // 6_27
      if (is_ho_complete)
      {
        std::cout << "is_ho_complete" << std::endl;
        srsran::unique_byte_buffer_t ho_success = srsran::make_byte_buffer();
        ho_success->init();
        ho_success->N_bytes = 3;
        ho_success->msg[0] = 0xff;
        ho_success->msg[1] = 0x02;
        ho_success->msg[2] = TC_MSG_RRC_ATE_HO_SUCCESS;
        // send to ate msg queue
        std::cout << "send ho success to ate queue" << std::endl;
        // parent->rrc_adp->udp_.send_ate_info.try_push(std::move(ho_success));
        parent->rrc_adp->udp_.send_ate_msg(std::move(ho_success));
        parent->rrc_adp->udp_.notify_change_enb(70);
  
        is_ho_complete = false;
      }
      parent->rrc_adp->udp_.reconfig_complete_number += 1;
      printf("reconfig_complete_number = %x\n", parent->rrc_adp->udp_.reconfig_complete_number);
      std::cout << "is_2_reconfig_complete = " << parent->rrc_adp->udp_.is_2_reconfig_complete << std::endl;
      std::cout << "is_3_reconfig_complete = " << parent->rrc_adp->udp_.is_3_reconfig_complete << std::endl;
      std::cout << "is_3_reconfig_complete = " << parent->rrc_adp->udp_.is_4_reconfig_complete << std::endl;
      if (parent->rrc_adp->udp_.TC_722_ho_success_last)
      {
        printf("TC_722 send rrc_conf_comp to ttcn");
        srsran::unique_byte_buffer_t rrc_reconf_comp_ttcn = srsran::make_byte_buffer();
        rrc_reconf_comp_ttcn->init();

        rrc_reconf_comp_ttcn->msg[7] = RRC_CON_RECFG_COMP;
        rrc_reconf_comp_ttcn->N_bytes = 11;

        parent->assemble_general_interface(&rrc_reconf_comp_ttcn, 11);

        parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rrc_reconf_comp_ttcn));
        std::cout << "TC_722 send_ttcn_info size:"
                  << parent->rrc_adp->udp_.send_ttcn_info.size() << std::endl;
        parent->rrc_adp->udp_.TC_722_ho_success_last = false;
      }
      if (parent->rrc_adp->udp_.reconfig_complete_number == 2 && parent->rrc_adp->udp_.is_2_reconfig_complete == true)
      {
        std::cout << " ttcn_testId = " << parent->rrc_adp->udp_.Gen_.test_id << std::endl;
        srsran::unique_byte_buffer_t is_2_reconfig_complete_pdu = srsran::make_byte_buffer();
        is_2_reconfig_complete_pdu->N_bytes = 5;
        is_2_reconfig_complete_pdu->msg[0] = 0x00;
        is_2_reconfig_complete_pdu->msg[1] = parent->rrc_adp->udp_.Gen_.test_id >> 8;
        is_2_reconfig_complete_pdu->msg[2] = parent->rrc_adp->udp_.Gen_.test_id;
        uint16_t test_count = (is_2_reconfig_complete_pdu->msg[1] << 8) | (is_2_reconfig_complete_pdu->msg[2]);
        std::cout << " test_count id = " << test_count << std::endl;
        is_2_reconfig_complete_pdu->msg[3] = RRC_CON_RECFG_COMP;
        is_2_reconfig_complete_pdu->msg[4] = 0x00;

        parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(is_2_reconfig_complete_pdu));
        parent->rrc_adp->udp_.is_2_reconfig_complete = false;
      }

      if (parent->rrc_adp->udp_.reconfig_complete_number == 3 && parent->rrc_adp->udp_.is_3_reconfig_complete == true)
      {
        std::cout << " ttcn_testId = " << parent->rrc_adp->udp_.Gen_.test_id << std::endl;
        srsran::unique_byte_buffer_t is_3_reconfig_complete_pdu = srsran::make_byte_buffer();
        is_3_reconfig_complete_pdu->N_bytes = 5;
        is_3_reconfig_complete_pdu->msg[0] = 0x00;
        is_3_reconfig_complete_pdu->msg[1] = parent->rrc_adp->udp_.Gen_.test_id >> 8;
        is_3_reconfig_complete_pdu->msg[2] = parent->rrc_adp->udp_.Gen_.test_id;
        uint16_t test_count = (is_3_reconfig_complete_pdu->msg[1] << 8) | (is_3_reconfig_complete_pdu->msg[2]);
        std::cout << " test_count id = " << test_count << std::endl;
        is_3_reconfig_complete_pdu->msg[3] = RRC_CON_RECFG_COMP;
        is_3_reconfig_complete_pdu->msg[4] = 0x00;

        parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(is_3_reconfig_complete_pdu));
        parent->rrc_adp->udp_.is_3_reconfig_complete = false;
      }

      if (parent->rrc_adp->udp_.reconfig_complete_number == 4 && parent->rrc_adp->udp_.is_4_reconfig_complete == true)
      {
        std::cout << " ttcn_testId = " << parent->rrc_adp->udp_.Gen_.test_id << std::endl;
        srsran::unique_byte_buffer_t is_4_reconfig_complete_pdu = srsran::make_byte_buffer();
        is_4_reconfig_complete_pdu->N_bytes = 5;
        is_4_reconfig_complete_pdu->msg[0] = 0x00;
        is_4_reconfig_complete_pdu->msg[1] = parent->rrc_adp->udp_.Gen_.test_id >> 8;
        is_4_reconfig_complete_pdu->msg[2] = parent->rrc_adp->udp_.Gen_.test_id;
        printf("  is_4_reconfig_complete_pdu->msg[1] = %x\n", is_4_reconfig_complete_pdu->msg[1]);
        printf("  is_4_reconfig_complete_pdu->msg[2] = %x\n", is_4_reconfig_complete_pdu->msg[2]);
        uint16_t test_count = (is_4_reconfig_complete_pdu->msg[1] << 8) | (is_4_reconfig_complete_pdu->msg[2]);
        std::cout << " test_count id = " << test_count << std::endl;
        is_4_reconfig_complete_pdu->msg[3] = RRC_CON_RECFG_COMP;
        is_4_reconfig_complete_pdu->msg[4] = 0x00;

        parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(is_4_reconfig_complete_pdu));
        parent->rrc_adp->udp_.is_4_reconfig_complete = false;
      }
      if (parent->rrc_adp->udp_.TC_628_RECONFIG_SUCCESS)
      {
        srsran::unique_byte_buffer_t rrc_reconf_comp_ttcn = srsran::make_byte_buffer();
        rrc_reconf_comp_ttcn->init();

        rrc_reconf_comp_ttcn->msg[7] = RRC_CON_RECFG_COMP;

        rrc_reconf_comp_ttcn->msg[8] = 0;
        rrc_reconf_comp_ttcn->msg[9] = 0;
        rrc_reconf_comp_ttcn->msg[10] = 0;

        rrc_reconf_comp_ttcn->N_bytes = 11;

        parent->assemble_general_interface(&rrc_reconf_comp_ttcn, 11);

        parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rrc_reconf_comp_ttcn));
        std::cout << "send_ttcn_info size:"
                  << parent->rrc_adp->udp_.send_ttcn_info.size() << std::endl;
        parent->rrc_adp->udp_.TC_628_RECONFIG_SUCCESS = false;
      }
      // parent->mac->mac_deallocate();
      if (parent->cfg.ttcn_rrc_enble && parent->cfg.ttcn_test_enble && (parent->rrc_adp->udp_.TC_711_reconfig || parent->rrc_adp->udp_.TC_713_reconfig_DRB || parent->rrc_adp->udp_.TC_714_DRB_Release || parent->rrc_adp->udp_.TC_715_reest_reconf || parent->rrc_adp->udp_.TC_720_measure_report_last || parent->rrc_adp->udp_.TC_715_BEAM2_REEST))
      {
        srsran::unique_byte_buffer_t rrc_reconf_comp_ttcn = srsran::make_byte_buffer();
        rrc_reconf_comp_ttcn->init();

        rrc_reconf_comp_ttcn->msg[7] = RRC_CON_RECFG_COMP;

        rrc_reconf_comp_ttcn->msg[8] = 0;
        rrc_reconf_comp_ttcn->msg[9] = 0;
        rrc_reconf_comp_ttcn->msg[10] = 0;

        rrc_reconf_comp_ttcn->N_bytes = 11;

        parent->assemble_general_interface(&rrc_reconf_comp_ttcn, 11);

        parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rrc_reconf_comp_ttcn));
        if (TC_713_reconfig_number == 1)
        {
          send_rrc_con_reconf(rnti, 2, 1, 0, nullptr, {});
          // parent->cnw->send_reg_or_service_accept(rnti);
        }
        if (TC_713_reconfig_number == 2)
        {
          TC_713_reconfig_number = 0;
        }

        if (TC_714_reconfig_number == 1)
        {
          send_rrc_con_reconf(rnti, 2, 1, 0, nullptr, {});
          // parent->cnw->send_reg_or_service_accept(rnti);
        }
        if (TC_714_reconfig_number == 2)
        {
          TC_714_reconfig_number = 0;
        }
        if (parent->rrc_adp->udp_.TC_715_reest_reconf)
        {
          parent->rrc_adp->udp_.TC_715_reest_reconf = false;
        }
        if (parent->rrc_adp->udp_.TC_715_BEAM2_REEST)
        {
          parent->rrc_adp->udp_.TC_715_BEAM2_REEST = false;
        }
        std::cout << "send_ttcn_info size:"
                  << parent->rrc_adp->udp_.send_ttcn_info.size() << std::endl;
      }

      if (parent->cfg.ttcn_rrc_enble && parent->cfg.ttcn_test_enble && parent->rrc_adp->udp_.TC_712_reconfig_update)
      {
        srsran::unique_byte_buffer_t rrc_reconf_comp_ttcn = srsran::make_byte_buffer();
        rrc_reconf_comp_ttcn->init();

        rrc_reconf_comp_ttcn->msg[7] = RRC_CON_RECFG_COMP;

        rrc_reconf_comp_ttcn->msg[8] = 0;
        rrc_reconf_comp_ttcn->msg[9] = 0;
        rrc_reconf_comp_ttcn->msg[10] = 0;

        rrc_reconf_comp_ttcn->N_bytes = 11;

        parent->assemble_general_interface(&rrc_reconf_comp_ttcn, 11);

        parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rrc_reconf_comp_ttcn));
        std::cout << "send_ttcn_info size:"
                  << parent->rrc_adp->udp_.send_ttcn_info.size() << std::endl;
        parent->rrc_adp->udp_.TC_712_reconfig_update = false;
      }
      if (parent->rrc_adp->udp_.TC_514_Con_Req)
      {
        parent->rrc_adp->udp_.TC_514_Con_Req = false;
        srsran::unique_byte_buffer_t rrc_reconf_comp_ttcn = srsran::make_byte_buffer();
        rrc_reconf_comp_ttcn->init();

        rrc_reconf_comp_ttcn->msg[7] = RRC_CON_RECFG_COMP;

        rrc_reconf_comp_ttcn->msg[8] = 0;
        rrc_reconf_comp_ttcn->msg[9] = 0;
        rrc_reconf_comp_ttcn->msg[10] = 0;

        rrc_reconf_comp_ttcn->N_bytes = 11;

        parent->assemble_general_interface(&rrc_reconf_comp_ttcn, 11);

        parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rrc_reconf_comp_ttcn));
        std::cout << "send_ttcn_info size:"
                  << parent->rrc_adp->udp_.send_ttcn_info.size() << std::endl;
      }
      if (parent->rrc_adp->udp_.TC_515_Con_Req)
      {
        parent->rrc_adp->udp_.TC_515_Con_Req = false;
        srsran::unique_byte_buffer_t rrc_reconf_comp_ttcn = srsran::make_byte_buffer();
        rrc_reconf_comp_ttcn->init();

        rrc_reconf_comp_ttcn->msg[7] = RRC_CON_RECFG_COMP;

        rrc_reconf_comp_ttcn->msg[8] = 0;
        rrc_reconf_comp_ttcn->msg[9] = 0;
        rrc_reconf_comp_ttcn->msg[10] = 0;

        rrc_reconf_comp_ttcn->N_bytes = 11;

        parent->assemble_general_interface(&rrc_reconf_comp_ttcn, 11);

        parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rrc_reconf_comp_ttcn));
        std::cout << "send_ttcn_info size:"
                  << parent->rrc_adp->udp_.send_ttcn_info.size() << std::endl;
      }

      save_ul_message_s(std::move(original_pdu));
      if (parent->cfg.pid == 2)
      {
        parent->rrc_adp->udp_.RACH_Handover=false;
      }
      s_handle_rrc_reconf_complete(&s_ul_dcch_msg.msg.rrc_con_recon_comp(), std::move(pdu));
      srsran::console("User 0x%x connected\n", rnti);
      state = RRC_STATE_REGISTERED;

      if (parent->rrc_adp->udp_.TC_725_release_nodirection)
      {
        std::cout << "udp_.TC_725_release_nodirection" << parent->rrc_adp->udp_.TC_725_release_nodirection << std::endl;
        parent->rrc_adp->udp_.TC_725_second_release = true;
        parent->rrc_adp->udp_.TC_725_release_nodirection = false;
      }

      set_activity_timeout(UE_INACTIVITY_TIMEOUT);
      // set_activity();
      break;

    case s_ul_dcch_msg_type_c::types::ue_id_trans:
      std::cout << "-----------This is ueIdentityTransfor---------" << std::endl;
      break;
    case s_ul_dcch_msg_type_c::types::measure_report:
      std::cout << "-----------This is measurereport---------" << std::endl;
     
      {
        srsran::unique_byte_buffer_t MEASURE_RES = srsran::make_byte_buffer();
        MEASURE_RES->init();
        MEASURE_RES->N_bytes = 3;
        MEASURE_RES->msg[0] = 0xff;
        MEASURE_RES->msg[1] = 0x02;
        MEASURE_RES->msg[2] = TC_MSG_RRC_ATE_MEASURE_RES;
        // send to ate msg queue
        parent->rrc_adp->udp_.send_ate_msg(std::move(MEASURE_RES));
      }
      handle_measure_report(&s_ul_dcch_msg.msg.measure_rep());
      break;
    case s_ul_dcch_msg_type_c::types::rrc_con_reest_comp:
      save_ul_message_s(std::move(original_pdu));
      s_handle_rrc_con_reest_complete(&s_ul_dcch_msg.msg.rrc_con_reest_comp(), std::move(pdu));
      std::cout << "----Connection reerererestablishment complete----" << std::endl;

      send_rrc_con_reconf(70, 2, 1, 0, nullptr, {});
      set_activity();
      break;
    default:
      std::cout << "----连接建立失败----" << std::endl;
      parent->logger.error("Msg: %s not supported", s_ul_dcch_msg.msg.type().to_string());
      break;
    }
  }
  //----------------------------------------TC_722_measure_report-------
  void rrc::ue::TC_722_handle_measure_report()
  {
    srsran::unique_byte_buffer_t rrc_measure_report = srsran::make_byte_buffer();
    rrc_measure_report->init();
    rrc_measure_report->msg[7] = RRC_MEAS_REPORT;
    rrc_measure_report->N_bytes = 8;
    parent->assemble_general_interface(&rrc_measure_report, 8);
    parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rrc_measure_report));
    std::cout << "send_ttcn_info size:"
              << parent->rrc_adp->udp_.send_ttcn_info.size() << std::endl;

    srsran::unique_byte_buffer_t ttcn_ho_reconf = srsran::make_byte_buffer();
    while (true)
    {
      if (parent->rrc_adp->udp_.rrc_receive_info.size() != 0)
      {

        std::cout << "receive rrc ho_reconf info from TTCN" << std::endl;
        ttcn_ho_reconf->init();
        parent->rrc_adp->udp_.rrc_receive_info.try_pop(ttcn_ho_reconf);
        parent->get_general_interface(&ttcn_ho_reconf);
        break;
      }
    }
    std::cout << "send ho rrc conf" << std::endl;
    parent->wx_Mcontrol_Notify_Switch(70, 0);
  }
  void rrc::ue::handle_measure_report(measure_rep_s *msg)
  {
    printf("---------handle measurereport------------\n");
    asn1::json_writer json_writer;
    msg->to_json(json_writer);

    measure_rep_r1_s *msg_r1 = &msg->measure_rep_r1;
    if (msg_r1->meas_result_present)
    {
      uint8_t beam_size = msg_r1->meas_result.beam_list.size();
      for (auto &val : msg_r1->meas_result.beam_list)
      {
        std::cout << "msg_r1->meas_result.beam_list" << static_cast<int>(val) << " ";
        std::cout << std::endl;
      }
    }
    if (msg_r1->ue_geo_info_present)
    {
      std::cout << "msg_r1->ue_geo_info.geo_info:" << std::endl;
    }
    if (parent->rrc_adp->udp_.TC_722_ho_success && TC_722_measure_report_number == 1)
    {
      TC_722_handle_measure_report();
    }
    if (parent->rrc_adp->udp_.TC_720_measure_report_last)
    {
      srsran::unique_byte_buffer_t rrc_measure_report = srsran::make_byte_buffer();
      rrc_measure_report->init();
      rrc_measure_report->msg[7] = RRC_MEAS_REPORT;
      rrc_measure_report->N_bytes = 8;
      parent->assemble_general_interface(&rrc_measure_report, 8);
      parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rrc_measure_report));
      std::cout << "send_ttcn_info size:"
                << parent->rrc_adp->udp_.send_ttcn_info.size() << std::endl;
    }
  }
  //--------------------------------IOT-2023/9/13----------------------------------
  void rrc::ue::iot_parse_ul_dcch_s(uint32_t lcid, srsran::unique_byte_buffer_t pdu)
  {
    iot_ul_dcch_msg_s iot_ul_dcch_msg;
    asn1::cbit_ref bref(pdu->msg, pdu->N_bytes);

    if (iot_ul_dcch_msg.unpack(bref) != asn1::SRSASN_SUCCESS)
    {
      parent->log_rx_pdu_fail(rnti, lcid, *pdu, "Failed to unpack UL-DCCH message");
      return;
    }

    //  std::cout<<"-------------------Compelete Info--------------------------------"<<std::endl;
    //    srsran::console("当前字节数=%u\n", pdu->N_bytes);   //打印解析的数据
    //         for (uint8_t i=0; i < pdu->N_bytes; i++)
    //         {
    //         srsran::console("%x\n", *(pdu->msg + i));
    //         }

    parent->log_rrc_message(Rx, rnti, lcid, *pdu, iot_ul_dcch_msg, iot_ul_dcch_msg.msg.type().to_string());
    srsran::unique_byte_buffer_t original_pdu = std::move(pdu);
    pdu = srsran::make_byte_buffer();
    if (pdu == nullptr)
    {
      parent->logger.error("Couldn't allocate PDU in %s().", __FUNCTION__);
      return;
    }
    rrc_t_id = 0;
    // std::cout<<"--------iot_ul_dcch_msg.msg.type()-------------"<<iot_ul_dcch_msg.msg.type()<<std::endl;
    switch (iot_ul_dcch_msg.msg.type())
    {
    case iot_ul_dcch_msg_type_c::types::iot_rrc_setup_complete:
      iot_save_ul_message_s(std::move(original_pdu)); /// 到这   save_ul_message_s又储存了一个新的，会覆盖之前的吗？
      iot_handle_rrc_con_setup_comp(&iot_ul_dcch_msg.msg.iot_rrc_setup_complete(), std::move(pdu));
      std::cout << "----连接建立完成----" << std::endl;
      set_activity();
      break;
    default:
      std::cout << "----连接建立失败----" << std::endl;
      parent->logger.error("Msg: %s not supported", iot_ul_dcch_msg.msg.type().to_string());
      break;
    }
  }
  //-------------------------------------------------------------------------------
  void rrc::ue::parse_ul_dcch(uint32_t lcid, srsran::unique_byte_buffer_t pdu)
  {
    ul_dcch_msg_s ul_dcch_msg;
    asn1::cbit_ref bref(pdu->msg, pdu->N_bytes);
    if (ul_dcch_msg.unpack(bref) != asn1::SRSASN_SUCCESS or
        ul_dcch_msg.msg.type().value != ul_dcch_msg_type_c::types_opts::c1)
    {
      parent->log_rx_pdu_fail(rnti, lcid, *pdu, "Failed to unpack UL-DCCH message");
      return;
    }

    // Log Rx message
    parent->log_rrc_message(Rx, rnti, lcid, *pdu, ul_dcch_msg, ul_dcch_msg.msg.c1().type().to_string());

    srsran::unique_byte_buffer_t original_pdu = std::move(pdu);
    pdu = srsran::make_byte_buffer();
    if (pdu == nullptr)
    {
      parent->logger.error("Couldn't allocate PDU in %s().", __FUNCTION__);
      return;
    }

    transaction_id = 0;

    switch (ul_dcch_msg.msg.c1().type())
    {
    case ul_dcch_msg_type_c::c1_c_::types::rrc_conn_setup_complete:
      save_ul_message(std::move(original_pdu));
      handle_rrc_con_setup_complete(&ul_dcch_msg.msg.c1().rrc_conn_setup_complete(), std::move(pdu));
      set_activity();
      break;
    case ul_dcch_msg_type_c::c1_c_::types::rrc_conn_reest_complete:
      save_ul_message(std::move(original_pdu));
      handle_rrc_con_reest_complete(&ul_dcch_msg.msg.c1().rrc_conn_reest_complete(), std::move(pdu));
      set_activity_timeout(UE_INACTIVITY_TIMEOUT);
      set_activity();
      break;
    case ul_dcch_msg_type_c::c1_c_::types::ul_info_transfer:
      pdu->N_bytes = ul_dcch_msg.msg.c1()
                         .ul_info_transfer()
                         .crit_exts.c1()
                         .ul_info_transfer_r8()
                         .ded_info_type.ded_info_nas()
                         .size();
      memcpy(pdu->msg,
             ul_dcch_msg.msg.c1()
                 .ul_info_transfer()
                 .crit_exts.c1()
                 .ul_info_transfer_r8()
                 .ded_info_type.ded_info_nas()
                 .data(),
             pdu->N_bytes);
      parent->s1ap->write_pdu(rnti, std::move(pdu));
      break;
    case ul_dcch_msg_type_c::c1_c_::types::rrc_conn_recfg_complete:
      save_ul_message(std::move(original_pdu));
      handle_rrc_reconf_complete(&ul_dcch_msg.msg.c1().rrc_conn_recfg_complete(), std::move(pdu));
      srsran::console("User 0x%x connected\n", rnti);
      state = RRC_STATE_REGISTERED;
      set_activity_timeout(UE_INACTIVITY_TIMEOUT);
      break;
    case ul_dcch_msg_type_c::c1_c_::types::security_mode_complete:
      handle_security_mode_complete(&ul_dcch_msg.msg.c1().security_mode_complete());
      send_ue_cap_enquiry({asn1::rrc::rat_type_opts::options::eutra});
      state = RRC_STATE_WAIT_FOR_UE_CAP_INFO;
      break;
    case ul_dcch_msg_type_c::c1_c_::types::security_mode_fail:
      handle_security_mode_failure(&ul_dcch_msg.msg.c1().security_mode_fail());
      break;
    case ul_dcch_msg_type_c::c1_c_::types::ue_cap_info:
      if (handle_ue_cap_info(&ul_dcch_msg.msg.c1().ue_cap_info()) == SRSRAN_SUCCESS)
      {
        if (endc_handler != nullptr && endc_handler->is_endc_supported() && state == RRC_STATE_WAIT_FOR_UE_CAP_INFO)
        {
          // request EUTRA-NR and NR capabilities as well
          send_ue_cap_enquiry({asn1::rrc::rat_type_opts::options::eutra_nr, asn1::rrc::rat_type_opts::options::nr});
          state = RRC_STATE_WAIT_FOR_UE_CAP_INFO_ENDC; // avoid endless loop
        }
        else
        {
          // send RRC reconfiguration to complete procedure
          send_connection_reconf(std::move(pdu));
        }
      }
      else
      {
        send_connection_reject(procedure_result_code::none);
        state = RRC_STATE_IDLE;
      }
      break;
    case ul_dcch_msg_type_c::c1_c_::types::meas_report:
      if (state == RRC_STATE_REGISTERED)
      {
        if (mobility_handler != nullptr)
        {
          mobility_handler->handle_ue_meas_report(ul_dcch_msg.msg.c1().meas_report(), std::move(original_pdu));
        }
        if (endc_handler != nullptr)
        {
          endc_handler->handle_ue_meas_report(ul_dcch_msg.msg.c1().meas_report());
        }
      }
      else
      {
        parent->logger.warning(
            "measurementReport for rnti=0x%x ignored. Cause: RRC Reconfiguration is not yet complete", rnti);
      }
      break;
    case ul_dcch_msg_type_c::c1_c_::types::ue_info_resp_r9:
      handle_ue_info_resp(ul_dcch_msg.msg.c1().ue_info_resp_r9(), std::move(original_pdu));
      break;
    default:
      parent->logger.error("Msg: %s not supported", ul_dcch_msg.msg.c1().type().to_string());
      break;
    }
  }

  std::string rrc::ue::to_string(const activity_timeout_type_t &type)
  {
    constexpr static const char *options[] = {"Msg3 reception", "UE inactivity", "UE reestablishment"};
    return srsran::enum_to_text(options, (uint32_t)activity_timeout_type_t::nulltype, (uint32_t)type);
  }

  /*
   *  Connection Setup
   */

  //---------------------------------------------------------------------------------------------------------------------------------------------------------------
  void rrc::ue::assemble_header_req(
      srsran::unique_byte_buffer_t *rrc_con_req_ttcn_)
  {
    rrc_con_req_ttcn_->get()->msg[0] = 0x00;
    rrc_con_req_ttcn_->get()->msg[1] = 0x00;
    rrc_con_req_ttcn_->get()->msg[2] = 0x00;
    rrc_con_req_ttcn_->get()->msg[3] = 0x02; // message type    rrc_con_req
    rrc_con_req_ttcn_->get()->msg[4] = 0x00; // msg[4] [5]??????
    rrc_con_req_ttcn_->get()->msg[5] = 0x01;
  }

  void rrc::ue::handle_rrc_con_req_s_ttcn(rrc_con_req_s *msg)
  {
    // RRC Message about connection establishment request sent to TTCN
    srsran::unique_byte_buffer_t rrc_con_req_ttcn = srsran::make_byte_buffer();
    assemble_header_req(&rrc_con_req_ttcn);
    rrc_con_req_ttcn->msg[6] = msg->rrc_con_req_r1.establishment_cause.value;
    rrc_con_req_ttcn->N_bytes = 7;
    parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rrc_con_req_ttcn));
    std::cout << "------send_ttcn_info size---:"
              << parent->rrc_adp->udp_.send_ttcn_info.size() << std::endl;

    asn1::json_writer json_writer;
    msg->to_json(json_writer);
    /*
    int a = 0;
    if (a) {
      send_connection_reject(procedure_result_code::fail_in_radio_interface_proc);
      return;
    }
    */
    rrc_con_req_r1_ie_s *msg_r1 = &msg->rrc_con_req_r1;
    if (msg_r1->ue_id.type() == init_ue_wx_id_c::types::nr_s_tmsi)
    {
      nr_s_tmsi = (uint64_t)msg_r1->ue_id.nr_s_tmsi().nr_s_tmsi.to_number();
      has_nr_s_tmsi = true;
      for (auto &user : parent->users)
      {
        if (user.first != rnti && user.second->has_nr_s_tmsi &&
            user.second->nr_s_tmsi == nr_s_tmsi)
        {
          parent->logger.info(
              "RRC connection request: UE context already exists. nr_s_tmsi=%d",
              nr_s_tmsi);
          user.second->state =
              RRC_STATE_IDLE; // Set old rnti to IDLE so that enb doesn't send
                              // RRC Connection Release
          parent->s1ap->user_release(
              user.first,
              asn1::s1ap::cause_radio_network_opts::interaction_with_other_proc);
          break;
        }
      }
    }
    else if (msg_r1->ue_id.type() == init_ue_wx_id_c::types::random_value)
    {
      // std::cout << "---------------------------random_value------number
      // value:"
      //           << msg_r1->ue_id.random_value().to_number() << std::endl;
      std::string a = msg_r1->ue_id.random_value().to_string();
      int power = 0;

      for (int i = a.length() - 1; i >= 0; i--)
      {
        if (a[i] == '1')
        {
          random_value += static_cast<uint64_t>(std::pow(2, power));
        }
        power++;
      }
      has_random_value = true;
      // random_value = msg_r1->ue_id.random_value().to_number();
      std::cout << "------------------------random_value:" << random_value
                << "--------------" << std::endl;
    }
    else
    {
      parent->logger.warning(
          "error msg_r1 int rrc::ue::handle_rrc_con_req_s() function ... ");
    }
    estab_cause = msg_r1->establishment_cause;
    send_rrc_con_setup_ttcn();
    // send_connection_setup();
    state = RRC_STATE_WAIT_FOR_CON_SETUP_COMPLETE;
  }

  void rrc::ue::send_rrc_con_setup_ttcn()
  {
    s_dl_ccch_msg_s dl_ccch_msg;

    rrc_con_setup_s &rrc_setup = dl_ccch_msg.msg.set_rrc_con_setup();
    srsran::unique_byte_buffer_t udp = srsran::make_byte_buffer();
    udp->init();
    while (true)
    {
      if (parent->rrc_adp->udp_.rrc_receive_info.size() != 0)
      {
        parent->rrc_adp->udp_.rrc_receive_info.try_pop(udp);
        std::cout << "receive setup info from TTCN" << std::endl;
        break;
      }
    }
    srsran::unique_byte_buffer_t setup_info = srsran::make_byte_buffer();
    setup_info->init();
    memcpy(setup_info->msg, udp->msg + 6, udp->N_bytes - 6);
    setup_info->N_bytes = udp->N_bytes - 6;
    std::cout << "------receive setup info from TTCN after dispose-----"
              << std::endl;
    for (uint32_t i = 0; i < setup_info->N_bytes; i++)
    {
      printf("0x:%x\n", *(setup_info->msg + i));
    }
    // Start judging data
    rrc_setup.rrc_transaction_id.rrc_t_id =
        (setup_info->msg[0] & 0xc0) >> 6; // [0]2

    printf("rrc_setup.rrc_transaction_id.rrc_t_id:%d", rrc_setup.rrc_transaction_id.rrc_t_id);

    rrc_con_setup_r1_s &setup = rrc_setup.rrc_con_setup_r1;
    radio_resurce_cfg_ded_s &rr_cfg_ded = setup.rr_cfg_ded;

    if ((setup_info->msg[0] & 0x20) >> 5)
    { //[0]1
      rr_cfg_ded.srb_to_add_present = true;
      std::cout << "--------SRB-ToAdd------" << std::endl;
    }
    if ((setup_info->msg[0] & 0x10) >> 4)
    { // Leave out for now   DRB-ToAddModList [0]1
      std::cout << "--------DRB-ToAddModList------" << std::endl;
    }
    if ((setup_info->msg[0] & 0x08) >> 3)
    { // DRB-ToReleaseList [0]1
      std::cout << "--------DRB-ToReleaseList------" << std::endl;
    }
    if ((setup_info->msg[0] & 0x04) >> 2)
    { //[0] 1
      rr_cfg_ded.periodic_bsr_timer_present = true;

      // Take the corresponding value  [0]2 [1]2
      rr_cfg_ded.periodic_bsr_timer = static_cast<
          asn1::rrc::radio_resurce_cfg_ded_s::periodic_bsr_timer_opts::options>(
          ((setup_info->msg[0] & 0x03) << 2) |
          ((setup_info->msg[1] & 0xc0) >> 6));
    }

    std::cout << "rr_cfg_ded.periodic_bsr_timer:"
              << rr_cfg_ded.periodic_bsr_timer.to_string() << std::endl;

    if ((setup_info->msg[1] & 0x20) >> 5)
    { //[1]1
      rr_cfg_ded.phy_ch_list_cfg_present = true;
      rr_cfg_ded.phy_ch_list_cfg.resize(1);

      rr_cfg_ded.phy_ch_list_cfg[0].s_rnti.from_number(
          ((setup_info->msg[1] & 0x1f) << 1) |
          ((setup_info->msg[2] & 0x80) >> 7)); //[1]5 [2]1
      std::cout << "--- rr_cfg_ded.phy_ch_list_cfg[0].s_rnti---:"
                << rr_cfg_ded.phy_ch_list_cfg[0].s_rnti.to_string() << std::endl;

      rr_cfg_ded.phy_ch_list_cfg[0].ch_type =
          static_cast<asn1::rrc::channel_type_opts::options>(
              ((setup_info->msg[2] & 0x78) >> 3)); //[2]4
      std::cout << "---rr_cfg_ded.phy_ch_list_cfg[0].ch_type---:"
                << rr_cfg_ded.phy_ch_list_cfg[0].ch_type.to_string() << std::endl;

      if ((setup_info->msg[2] & 0x04) >> 2)
      { //[2]1
        rr_cfg_ded.phy_ch_list_cfg[0].band_id_present = true;
        rr_cfg_ded.phy_ch_list_cfg[0].band_id.ba_id.from_number( //[2]2 [3]4
            ((setup_info->msg[2] & 0x03) << 4) |
            ((setup_info->msg[3] & 0xf0) >> 4));
        std::cout << "---  rr_cfg_ded.phy_ch_list_cfg[0].band_id.ba_id---:"
                  << rr_cfg_ded.phy_ch_list_cfg[0].band_id.ba_id.to_string()
                  << std::endl;
      }

      if ((setup_info->msg[3] & 0x08) >> 3)
      { //[3]1
        rr_cfg_ded.phy_ch_list_cfg[0].freq_id_present = true;

        rr_cfg_ded.phy_ch_list_cfg[0].freq_id.freq_id.from_number( //[3]2
            (setup_info->msg[3] & 0x06) >> 1);
        std::cout << "--- rr_cfg_ded.phy_ch_list_cfg[0].freq_id---:"
                  << rr_cfg_ded.phy_ch_list_cfg[0].freq_id.freq_id.to_string()
                  << std::endl;
      }

      if (setup_info->msg[3] & 0x01)
      { //[3]1
        rr_cfg_ded.phy_ch_list_cfg[0].slot_ass_present = true;

        rr_cfg_ded.phy_ch_list_cfg[0].slot_ass.from_number(
            (setup_info->msg[4] & 0xf8) >> 3); //[4]5
        std::cout << "---- rr_cfg_ded.phy_ch_list_cfg[0].slot_ass---:"
                  << rr_cfg_ded.phy_ch_list_cfg[0].slot_ass.to_string()
                  << std::endl;
      }

      rr_cfg_ded.phy_ch_list_cfg[0].direction =
          static_cast<asn1::rrc::phy_ch_cfg_s::direction_opts::options>(
              (setup_info->msg[4] & 0x06) >> 1); //[4]2
      std::cout << "----rr_cfg_ded.phy_ch_list_cfg[0].direction---"
                << rr_cfg_ded.phy_ch_list_cfg[0].direction.to_string()
                << std::endl;
    }

    // Fill RR config dedicated
    // generate_rr_cfg_ded_setup(rr_cfg_ded);
    apply_rlc_srb_updates();
    apply_pdcp_srb_updates();
    std::string octet_str;
    send_s_dl_ccch(&dl_ccch_msg, &octet_str);
  }

  void rrc::ue::wx_Set_SRB()
  {
    apply_rlc_srb_updates();
    apply_pdcp_srb_updates();
    std::cout << "[wx_Set_SRB]" << std::endl;
  }

  void rrc::ue::send_rrc_con_setup_ttcns()
  {
    s_dl_ccch_msg_s dl_ccch_msg;

    rrc_con_setup_s &rrc_setup = dl_ccch_msg.msg.set_rrc_con_setup();

    srsran::unique_byte_buffer_t udp = srsran::make_byte_buffer();
    udp->init();
    while (true)
    {
      if (parent->rrc_adp->udp_.rrc_receive_info.size() != 0)
      {
        parent->rrc_adp->udp_.rrc_receive_info.try_pop(udp);
        if (udp.get()->msg[7] != 0x05)
        {
          return;
        }
        else if (udp.get()->msg[7] == 0x05)
        {
          std::cout << "receive setup info from ttcn" << std::endl;
          break;
        }
      }
    }
    rrc_setup.rrc_transaction_id.rrc_t_id = 0;
    rrc_con_setup_r1_s &setup = rrc_setup.rrc_con_setup_r1;
    radio_resurce_cfg_ded_s &rr_cfg_ded = setup.rr_cfg_ded;

    rr_cfg_ded.periodic_bsr_timer_present = true;
    rr_cfg_ded.periodic_bsr_timer = radio_resurce_cfg_ded_s::periodic_bsr_timer_opts::rf10;

    rr_cfg_ded.phy_ch_list_cfg_present = true;
    rr_cfg_ded.phy_ch_list_cfg.resize(1);

    rr_cfg_ded.phy_ch_list_cfg[0].s_rnti.from_string("000101");
    rr_cfg_ded.phy_ch_list_cfg[0].ch_type = channel_type_e::pdch1_1;
    rr_cfg_ded.phy_ch_list_cfg[0].band_id_present = true;
    rr_cfg_ded.phy_ch_list_cfg[0].band_id.ba_id.from_string("000110");
    rr_cfg_ded.phy_ch_list_cfg[0].freq_id_present = true;
    rr_cfg_ded.phy_ch_list_cfg[0].freq_id.freq_id.from_string("01");
    rr_cfg_ded.phy_ch_list_cfg[0].slot_ass_present = true;
    rr_cfg_ded.phy_ch_list_cfg[0].slot_ass.from_string("00100");
    rr_cfg_ded.phy_ch_list_cfg[0].direction = phy_ch_cfg_s::direction_opts::uldirection;
    // Fill RR config dedicated
    // generate_rr_cfg_ded_setup(rr_cfg_ded);
    apply_rlc_srb_updates();
    apply_pdcp_srb_updates();
    std::string octet_str;
    send_s_dl_ccch(&dl_ccch_msg, &octet_str);
  }
  void rrc::ue::handle_rrc_con_req_s(rrc_con_req_s *msg)
  {
    if (parent->cfg.ttcn_test_enble && parent->cfg.ttcn_rrc_enble && parent->rrc_adp->udp_.TC_73_t300_timeout)
    {
      std::cout << "xxk_udp_.TC_73_t300_timeout" << parent->rrc_adp->udp_.TC_73_t300_timeout << std::endl;
      srsran::unique_byte_buffer_t rrc_con_req_ttcn = srsran::make_byte_buffer();
      rrc_con_req_ttcn->init();

      rrc_con_qeq_info_.msgType = RRC_CONNECT_REQ;
      rrc_con_qeq_info_.establishmentCause = msg->rrc_con_req_r1.establishment_cause.value;

      rrc_con_req_ttcn->msg[7] = rrc_con_qeq_info_.msgType;
      rrc_con_req_ttcn->msg[8] = msg->rrc_con_req_r1.establishment_cause.value;
      rrc_con_req_ttcn->N_bytes = 9;

      parent->assemble_general_interface(&rrc_con_req_ttcn, 9);

      parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rrc_con_req_ttcn));
      std::cout << "send_ttcn_info size:"
                << parent->rrc_adp->udp_.send_ttcn_info.size() << std::endl;
    }

    if (parent->cfg.ttcn_test_enble && parent->cfg.ttcn_rrc_enble && (parent->rrc_adp->udp_.is_connection || parent->rrc_adp->udp_.TC_71_is_paging_connection || parent->rrc_adp->udp_.TC_79_is_paging_smc || parent->rrc_adp->udp_.TC_72_con_capability || parent->rrc_adp->udp_.TC_711_reconfig || parent->rrc_adp->udp_.TC_517_paging_success || parent->rrc_adp->udp_.TC_710_security_mode_failure || parent->rrc_adp->udp_.TC_516_is_paging_invalid || parent->rrc_adp->udp_.TC_713_reconfig_DRB || parent->rrc_adp->udp_.TC_714_DRB_Release || parent->rrc_adp->udp_.TC_715_reest_reconf))
    {
      printf("parent->rrc_adp->udp_.TC_713_reconfig_DRB");
      // RRC Message about connection establishment request sent to TTCN
      srsran::unique_byte_buffer_t rrc_con_req_ttcn = srsran::make_byte_buffer();
      rrc_con_req_ttcn->init();

      rrc_con_qeq_info_.msgType = RRC_CONNECT_REQ;
      rrc_con_qeq_info_.establishmentCause = msg->rrc_con_req_r1.establishment_cause.value;

      rrc_con_req_ttcn->msg[7] = rrc_con_qeq_info_.msgType;
      rrc_con_req_ttcn->msg[8] = msg->rrc_con_req_r1.establishment_cause.value;
      rrc_con_req_ttcn->N_bytes = 9;

      parent->assemble_general_interface(&rrc_con_req_ttcn, 9);

      parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rrc_con_req_ttcn));
      std::cout << "send_ttcn_info size:"
                << parent->rrc_adp->udp_.send_ttcn_info.size() << std::endl;
      // parent->rrc_adp->udp_.TC_713_reconfig_DRB=false;
    }

    std::cout << "parent->cfg.ttcn_test_enble:" << parent->cfg.ttcn_test_enble << std::endl;
    std::cout << "parent->cfg.ttcn_rrc_enble:" << parent->cfg.ttcn_rrc_enble << std::endl;
    std::cout << "parent->rrc_adp->udp_.is_paging_refuse:" << parent->rrc_adp->udp_.TC_75_is_paging_refuse << std::endl;

    if (parent->cfg.ttcn_test_enble && parent->cfg.ttcn_rrc_enble && parent->rrc_adp->udp_.TC_75_is_paging_refuse)
    {
      std::cout << "888888888888888" << std::endl;
      // RRC Message about connection establishment request sent to TTCN
      srsran::unique_byte_buffer_t rrc_con_req_ttcn = srsran::make_byte_buffer();
      rrc_con_req_ttcn->init();

      rrc_con_qeq_info_.msgType = RRC_CONNECT_REQ;
      rrc_con_qeq_info_.establishmentCause = msg->rrc_con_req_r1.establishment_cause.value;

      rrc_con_req_ttcn->msg[7] = rrc_con_qeq_info_.msgType;
      rrc_con_req_ttcn->msg[8] = msg->rrc_con_req_r1.establishment_cause.value;
      rrc_con_req_ttcn->N_bytes = 9;

      parent->assemble_general_interface(&rrc_con_req_ttcn, 9);

      parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rrc_con_req_ttcn));
      std::cout << "send_ttcn_info size:"
                << parent->rrc_adp->udp_.send_ttcn_info.size() << std::endl;
    }

    if (parent->cfg.ttcn_test_enble && parent->cfg.ttcn_rrc_enble && parent->rrc_adp->udp_.TC_75_is_second_con_req)
    {
      std::cout << "TC_75_is_second_con_req" << parent->rrc_adp->udp_.TC_75_is_second_con_req << std::endl;
      parent->rrc_adp->udp_.TC_75_is_second_con_req = false;
      // RRC Message about connection establishment request sent to TTCN
      srsran::unique_byte_buffer_t rrc_con_req_ttcn = srsran::make_byte_buffer();
      rrc_con_req_ttcn->init();

      rrc_con_qeq_info_.msgType = RRC_CONNECT_REQ;
      rrc_con_qeq_info_.establishmentCause = msg->rrc_con_req_r1.establishment_cause.value;

      rrc_con_req_ttcn->msg[7] = rrc_con_qeq_info_.msgType;
      rrc_con_req_ttcn->msg[8] = msg->rrc_con_req_r1.establishment_cause.value;
      rrc_con_req_ttcn->N_bytes = 9;

      parent->assemble_general_interface(&rrc_con_req_ttcn, 9);

      parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rrc_con_req_ttcn));
      std::cout << "send_ttcn_info size:"
                << parent->rrc_adp->udp_.send_ttcn_info.size() << std::endl;
    }

    if (parent->cfg.ttcn_test_enble && parent->cfg.ttcn_rrc_enble && parent->rrc_adp->udp_.TC_74_t302_timeout)
    {
      // RRC Message about connection establishment request sent to TTCN
      std::cout << "C_74_t302_timeout" << parent->rrc_adp->udp_.TC_74_t302_timeout << std::endl;
      srsran::unique_byte_buffer_t rrc_con_req_ttcn = srsran::make_byte_buffer();
      rrc_con_req_ttcn->init();

      rrc_con_qeq_info_.msgType = RRC_CONNECT_REQ;
      rrc_con_qeq_info_.establishmentCause = msg->rrc_con_req_r1.establishment_cause.value;

      rrc_con_req_ttcn->msg[7] = rrc_con_qeq_info_.msgType;
      rrc_con_req_ttcn->msg[8] = msg->rrc_con_req_r1.establishment_cause.value;
      rrc_con_req_ttcn->N_bytes = 9;

      parent->assemble_general_interface(&rrc_con_req_ttcn, 9);

      parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rrc_con_req_ttcn));
      std::cout << "send_ttcn_info size:"
                << parent->rrc_adp->udp_.send_ttcn_info.size() << std::endl;
    }

    std::cout << "zhjzhjzhjzhj!!!" << std::endl;
    if (parent->cfg.ttcn_test_enble && parent->cfg.ttcn_rrc_enble && parent->rrc_adp->udp_.TC_73_second_t300_con_req)
    {
      std::cout << " ond_t300_con_req " << std::endl;
      parent->rrc_adp->udp_.TC_73_second_t300_con_req = false;
      // RRC Message about connection establishment request sent to TTCN
      srsran::unique_byte_buffer_t rrc_con_req_ttcn = srsran::make_byte_buffer();
      rrc_con_req_ttcn->init();

      rrc_con_qeq_info_.msgType = RRC_CONNECT_REQ;
      rrc_con_qeq_info_.establishmentCause = msg->rrc_con_req_r1.establishment_cause.value;

      rrc_con_req_ttcn->msg[7] = rrc_con_qeq_info_.msgType;
      rrc_con_req_ttcn->msg[8] = msg->rrc_con_req_r1.establishment_cause.value;
      rrc_con_req_ttcn->N_bytes = 9;

      parent->assemble_general_interface(&rrc_con_req_ttcn, 9);

      parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rrc_con_req_ttcn));
      std::cout << "send_ttcn_info size:"
                << parent->rrc_adp->udp_.send_ttcn_info.size() << std::endl;
    }

    if (parent->cfg.ttcn_test_enble && parent->cfg.ttcn_rrc_enble && parent->rrc_adp->udp_.TC_74_second_t302_con_req)
    {

      parent->rrc_adp->udp_.TC_74_second_t302_con_req = false;
      // RRC Message about connection establishment request sent to TTCN
      srsran::unique_byte_buffer_t rrc_con_req_ttcn = srsran::make_byte_buffer();
      rrc_con_req_ttcn->init();

      rrc_con_qeq_info_.msgType = RRC_CONNECT_REQ;
      rrc_con_qeq_info_.establishmentCause = msg->rrc_con_req_r1.establishment_cause.value;

      rrc_con_req_ttcn->msg[7] = rrc_con_qeq_info_.msgType;
      rrc_con_req_ttcn->msg[8] = msg->rrc_con_req_r1.establishment_cause.value;
      rrc_con_req_ttcn->N_bytes = 9;

      parent->assemble_general_interface(&rrc_con_req_ttcn, 9);

      parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rrc_con_req_ttcn));
      std::cout << "send_ttcn_info size:"
                << parent->rrc_adp->udp_.send_ttcn_info.size() << std::endl;
    }

    if (parent->cfg.ttcn_test_enble && parent->cfg.ttcn_rrc_enble && parent->rrc_adp->udp_.TC_725_con_req)
    {
      // RRC Message about connection establishment request sent to TTCN
      srsran::unique_byte_buffer_t rrc_con_req_ttcn = srsran::make_byte_buffer();
      rrc_con_req_ttcn->init();

      rrc_con_qeq_info_.msgType = RRC_CONNECT_REQ;
      rrc_con_qeq_info_.establishmentCause = msg->rrc_con_req_r1.establishment_cause.value;

      rrc_con_req_ttcn->msg[7] = rrc_con_qeq_info_.msgType;
      rrc_con_req_ttcn->msg[8] = msg->rrc_con_req_r1.establishment_cause.value;
      rrc_con_req_ttcn->N_bytes = 9;

      parent->assemble_general_interface(&rrc_con_req_ttcn, 9);

      parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rrc_con_req_ttcn));
      std::cout << "send_ttcn_info size:"
                << parent->rrc_adp->udp_.send_ttcn_info.size() << std::endl;

      parent->rrc_adp->udp_.TC_725_con_req = false;
    }

    if (parent->cfg.ttcn_test_enble && parent->cfg.ttcn_rrc_enble && parent->rrc_adp->udp_.TC_511_Con_Req)
    {
      std::cout << "parent->rrc_adp->udp_.TC_511_Con_Req:" << parent->rrc_adp->udp_.TC_511_Con_Req << std::endl;
      // RRC Message about connection establishment request sent to TTCN
      srsran::unique_byte_buffer_t rrc_con_req_ttcn = srsran::make_byte_buffer();
      rrc_con_req_ttcn->init();

      rrc_con_qeq_info_.msgType = RRC_CONNECT_REQ;
      rrc_con_qeq_info_.establishmentCause = msg->rrc_con_req_r1.establishment_cause.value;

      rrc_con_req_ttcn->msg[7] = rrc_con_qeq_info_.msgType;
      rrc_con_req_ttcn->msg[8] = msg->rrc_con_req_r1.establishment_cause.value;
      rrc_con_req_ttcn->N_bytes = 9;

      parent->assemble_general_interface(&rrc_con_req_ttcn, 9);

      parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rrc_con_req_ttcn));
      std::cout << "send_ttcn_info size:"
                << parent->rrc_adp->udp_.send_ttcn_info.size() << std::endl;

      parent->rrc_adp->udp_.TC_511_Con_Req = false;
    }
    if (parent->cfg.ttcn_test_enble && parent->cfg.ttcn_rrc_enble && parent->rrc_adp->udp_.TC_513_barred_con_req)
    {
      std::cout << "TC_513_barred_con_req" << parent->rrc_adp->udp_.TC_513_barred_con_req << std::endl;
      // RRC Message about connection establishment request sent to TTCN
      srsran::unique_byte_buffer_t rrc_con_req_ttcn = srsran::make_byte_buffer();
      rrc_con_req_ttcn->init();

      rrc_con_qeq_info_.msgType = RRC_CONNECT_REQ;
      rrc_con_qeq_info_.establishmentCause = msg->rrc_con_req_r1.establishment_cause.value;

      rrc_con_req_ttcn->msg[7] = rrc_con_qeq_info_.msgType;
      rrc_con_req_ttcn->msg[8] = msg->rrc_con_req_r1.establishment_cause.value;
      rrc_con_req_ttcn->N_bytes = 9;

      parent->assemble_general_interface(&rrc_con_req_ttcn, 9);

      parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rrc_con_req_ttcn));
      std::cout << "send_ttcn_info size:"
                << parent->rrc_adp->udp_.send_ttcn_info.size() << std::endl;

      parent->rrc_adp->udp_.TC_513_barred_con_req = false;
    }
    if (parent->rrc_adp->udp_.TC_512_con_req)
    {
      std::cout << "TC_512_con_req" << parent->rrc_adp->udp_.TC_512_con_req << std::endl;
      // RRC Message about connection establishment request sent to TTCN
      srsran::unique_byte_buffer_t rrc_con_req_ttcn = srsran::make_byte_buffer();
      rrc_con_req_ttcn->init();

      rrc_con_qeq_info_.msgType = RRC_CONNECT_REQ;
      rrc_con_qeq_info_.establishmentCause = msg->rrc_con_req_r1.establishment_cause.value;

      rrc_con_req_ttcn->msg[7] = rrc_con_qeq_info_.msgType;
      rrc_con_req_ttcn->msg[8] = msg->rrc_con_req_r1.establishment_cause.value;
      rrc_con_req_ttcn->N_bytes = 9;

      parent->assemble_general_interface(&rrc_con_req_ttcn, 9);

      parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rrc_con_req_ttcn));
      std::cout << "send_ttcn_info size:"
                << parent->rrc_adp->udp_.send_ttcn_info.size() << std::endl;

      parent->rrc_adp->udp_.TC_512_con_req = false;
    }
    if (parent->rrc_adp->udp_.TC_514_Con_Req)
    {
      std::cout << "parent->rrc_adp->udp_.TC_514_Con_Req=" << parent->rrc_adp->udp_.TC_514_Con_Req;
      // parent->rrc_adp->udp_.TC_514_Con_Req=false;
      //  RRC Message about connection establishment request sent to TTCN
      srsran::unique_byte_buffer_t rrc_con_req_ttcn = srsran::make_byte_buffer();
      rrc_con_req_ttcn->init();

      rrc_con_qeq_info_.msgType = RRC_CONNECT_REQ;
      rrc_con_qeq_info_.establishmentCause = msg->rrc_con_req_r1.establishment_cause.value;

      rrc_con_req_ttcn->msg[7] = rrc_con_qeq_info_.msgType;
      rrc_con_req_ttcn->msg[8] = msg->rrc_con_req_r1.establishment_cause.value;
      rrc_con_req_ttcn->N_bytes = 9;

      parent->assemble_general_interface(&rrc_con_req_ttcn, 9);

      parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rrc_con_req_ttcn));
      std::cout << "send_ttcn_info size:"
                << parent->rrc_adp->udp_.send_ttcn_info.size() << std::endl;
    }
    if (parent->rrc_adp->udp_.TC_515_Con_Req)
    {
      std::cout << "parent->rrc_adp->udp_.TC_515_Con_Req=" << parent->rrc_adp->udp_.TC_515_Con_Req;
      // parent->rrc_adp->udp_.TC_515_Con_Req=false;
      //  RRC Message about connection establishment request sent to TTCN
      srsran::unique_byte_buffer_t rrc_con_req_ttcn = srsran::make_byte_buffer();
      rrc_con_req_ttcn->init();

      rrc_con_qeq_info_.msgType = RRC_CONNECT_REQ;
      rrc_con_qeq_info_.establishmentCause = msg->rrc_con_req_r1.establishment_cause.value;

      rrc_con_req_ttcn->msg[7] = rrc_con_qeq_info_.msgType;
      rrc_con_req_ttcn->msg[8] = msg->rrc_con_req_r1.establishment_cause.value;
      rrc_con_req_ttcn->N_bytes = 9;

      parent->assemble_general_interface(&rrc_con_req_ttcn, 9);

      parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rrc_con_req_ttcn));
      std::cout << "send_ttcn_info size:"
                << parent->rrc_adp->udp_.send_ttcn_info.size() << std::endl;
    }

    asn1::json_writer json_writer;
    msg->to_json(json_writer);

    rrc_con_req_r1_ie_s *msg_r1 = &msg->rrc_con_req_r1;
    if (msg_r1->ue_id.type() == init_ue_wx_id_c::types::nr_s_tmsi)
    {
      nr_s_tmsi = msg_r1->ue_id.nr_s_tmsi().nr_s_tmsi.to_number();
      std::cout << "nr_s_tmsi:" << nr_s_tmsi << std::endl;

      has_nr_s_tmsi = true;
      for (auto &user : parent->users)
      {
        if (user.first != rnti && user.second->has_nr_s_tmsi && user.second->nr_s_tmsi == nr_s_tmsi)
        {
          parent->logger.info("RRC connection request: UE context already exists. nr_s_tmsi=%d", nr_s_tmsi);
          user.second->state = RRC_STATE_IDLE; // Set old rnti to IDLE so that enb doesn't send RRC Connection Release
          parent->s1ap->user_release(user.first, asn1::s1ap::cause_radio_network_opts::interaction_with_other_proc);
          break;
        }
      }
    }
    else if (msg_r1->ue_id.type() == init_ue_wx_id_c::types::random_value)
    {
      random_value = msg_r1->ue_id.random_value().to_number();
      has_random_value = true;
      std::cout << "***************to number*****random_value:" << random_value << std::endl;
      std::cout << "***************to string*****random_value:" << msg_r1->ue_id.random_value().to_string() << std::endl;
    }

    estab_cause = msg_r1->establishment_cause;

    /*20240605 xxk add ue category to mac******/
    ue_category = msg_r1->ue_cap.ue_category_r1;
    parent->rrc_adp->udp_.ue_category = ue_category;
    std::cout << "udp_.ue_category= " << (int)parent->rrc_adp->udp_.ue_category << std::endl;

    /*send message to ate infrom ue category   6.14 */
    srsran::unique_byte_buffer_t ue_category_ate = srsran::make_byte_buffer();
    ue_category_ate->init();
    ue_category_ate->N_bytes = 4;
    ue_category_ate->msg[0] = 0xff;
    ue_category_ate->msg[1] = 0x02;
    ue_category_ate->msg[2] = TC_MSG_RRC_ATE_UE_CATEGORY;
    ue_category_ate->msg[3] = ue_category;

    // send to ate msg queue
    std::cout << "send uecategory to ate queue" << std::endl;
    // parent->rrc_adp->udp_.send_ate_info.try_push(std::move(ue_category_ate));
    parent->rrc_adp->udp_.send_ate_msg(std::move(ue_category_ate));

    std::cout << "[RRC][SETUP][UECAP][14]" << std::endl;
    std::cout << "[RRC][SETUP][UECAP][14]phy_chan_list_cfg[0].band_id.ba_id=" << parent->cfg.recfg_wx_voice.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].band_id.ba_id.to_number() << std::endl;
    std::cout << "[RRC][SETUP][UECAP][14]phy_chan_list_cfg[0].freq_id.freq_id=" << parent->cfg.recfg_wx_voice.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].freq_id.freq_id.to_number() << std::endl;
    std::cout << "[RRC][SETUP][UECAP][14]phy_chan_list_cfg[0].slot_ass=" << parent->cfg.recfg_wx_voice.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.to_number() << std::endl;
    ue_cap14_band_id = parent->cfg.recfg_wx_voice.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].band_id.ba_id.to_number();
    ue_cap14_freq_id = parent->cfg.recfg_wx_voice.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].freq_id.freq_id.to_number();
    ue_cap14_slot = parent->cfg.recfg_wx_voice.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.to_number();
    std::cout << "[RRC][SETUP][UECAP][14]ue_cap14_band_id=" << ue_cap14_band_id << std::endl;
    std::cout << "[RRC][SETUP][UECAP][14]ue_cap14_freq_id=" << ue_cap14_freq_id << std::endl;
    std::cout << "[RRC][SETUP][UECAP][14]ue_cap14_slot=" << ue_cap14_slot << std::endl;
    // 800b
    // ue_cap14_band_id = 0x10;
    // ue_cap14_freq_id = 0x01;
    // ue_cap14_slot = 0x08;
    uint16_t temp_ue_cap14_slot = ue_cap14_slot;
    int n = 0;
    while ((temp_ue_cap14_slot & 0x01) == 0)
    {
      temp_ue_cap14_slot = temp_ue_cap14_slot >> 1;
      n++;
    }

    std::cout << "@@ n = " << n << std::endl;

    parent->mac->setUecategory(ue_category, ue_cap14_band_id, ue_cap14_freq_id, n, ue_cap14_slot);

    /******************************************/

    if (parent->cfg.ttcn_test_enble && parent->cfg.ttcn_rrc_enble && (parent->rrc_adp->udp_.TC_75_is_paging_refuse || parent->rrc_adp->udp_.TC_74_t302_timeout))
    {
      std::cout << "send_con_reject_ttcn();" << std::endl;
      std::cout << "C_74_t302_timeout1" << parent->rrc_adp->udp_.TC_74_t302_timeout << std::endl;
      send_con_reject_ttcn();
    }
    else
    {
      send_rrc_con_setup();
    }
    state = RRC_STATE_WAIT_FOR_CON_SETUP_COMPLETE;
  }

  void rrc::ue::send_rrc_con_setup()
  {
    if (parent->cfg.ttcn_test_enble && parent->cfg.ttcn_rrc_enble && (parent->rrc_adp->udp_.is_connection || parent->rrc_adp->udp_.TC_71_is_paging_connection || parent->rrc_adp->udp_.TC_79_is_paging_smc || parent->rrc_adp->udp_.TC_72_con_capability || parent->rrc_adp->udp_.TC_711_reconfig || parent->rrc_adp->udp_.TC_517_paging_success || parent->rrc_adp->udp_.TC_710_security_mode_failure || parent->rrc_adp->udp_.TC_713_reconfig_DRB || parent->rrc_adp->udp_.TC_714_DRB_Release || parent->rrc_adp->udp_.TC_715_reest_reconf))
    {
      while (true)
      {
        if (parent->rrc_adp->udp_.rrc_receive_info.size() != 0)
        {
          std::cout << " TTCN Setup Info " << std::endl;
          srsran::unique_byte_buffer_t ttcn_setup =
              srsran::make_byte_buffer();
          ttcn_setup->init();
          parent->rrc_adp->udp_.rrc_receive_info.try_pop(ttcn_setup);
          parent->get_general_interface(&ttcn_setup);

          // TTCN-3 config parameters
          break;
        }
      }
    }

    // }
    s_dl_ccch_msg_s dl_ccch_msg;

    rrc_con_setup_s &rrc_setup = dl_ccch_msg.msg.set_rrc_con_setup();
    rrc_setup.rrc_transaction_id.rrc_t_id = 0;
    rrc_con_setup_r1_s &setup = rrc_setup.rrc_con_setup_r1;
    radio_resurce_cfg_ded_s &rr_cfg_ded = setup.rr_cfg_ded;

    rr_cfg_ded.srb_to_add_present = true;

    if (ue_category == 14)
    {
      std::cout << " ue_category = " << (int)ue_category << std::endl;

      SRB_configMap.ul_Type = pdch1_1;
      SRB_configMap.dl_Type = pdch1_1;
      std::cout << " SRB_configMap.ul_Type = " << SRB_configMap.ul_Type << std::endl;
      std::cout << " SRB_configMap.dl_Type = " << SRB_configMap.dl_Type << std::endl;
      /*
      SRB_configMap.dataType=static_cast<DataType>(0);//control
      SRB_configMap.rlcType=static_cast<RlcType>(0);//AM
      SRB_configMap.voicetype=static_cast< VoiceType>(2); //NO Voice
      */
      SRB_configMap.dataType = Control;  // control
      SRB_configMap.rlcType = rlc_am;    // AM
      SRB_configMap.voicetype = N_Voice; // NO Voice

      parent->mac->addlcidMap(1, SRB_configMap);
    }
    else
    {
      std::cout << "@@@@@@@@@@@@@@@@@@@@@@1" << std::endl;
      rr_cfg_ded.periodic_bsr_timer_present = false;
      rr_cfg_ded.periodic_bsr_timer = radio_resurce_cfg_ded_s::periodic_bsr_timer_opts::rf10;

      rr_cfg_ded.phy_ch_list_cfg_present = true;
      // nom
      if (wx_area_mode == 0)
      {
        std::cout << "111wx_area_mode==0" << std::endl;
        rr_cfg_ded.phy_ch_list_cfg.resize(1);

        rr_cfg_ded.phy_ch_list_cfg[0].s_rnti.from_string("000110");
        rr_cfg_ded.phy_ch_list_cfg[0].ch_type = channel_type_e::pdch1_1;
        rr_cfg_ded.phy_ch_list_cfg[0].band_id_present = true;
        // rr_cfg_ded.phy_ch_list_cfg[0].band_id.ba_id.from_string("001110");
        rr_cfg_ded.phy_ch_list_cfg[0].band_id.ba_id.from_string("010101");
        rr_cfg_ded.phy_ch_list_cfg[0].freq_id_present = true;
        rr_cfg_ded.phy_ch_list_cfg[0].freq_id.freq_id.from_string("01");
        rr_cfg_ded.phy_ch_list_cfg[0].slot_ass_present = true;

        if (rr_cfg_ded.phy_ch_list_cfg[0].ch_type == 1 || rr_cfg_ded.phy_ch_list_cfg[0].ch_type == 3 || rr_cfg_ded.phy_ch_list_cfg[0].ch_type == 5)
        {
          // rr_cfg_ded.phy_ch_list_cfg[0].slot_ass.from_string("00100"); // psch5-1/pdch1-1/psch1-1=00100
          rr_cfg_ded.phy_ch_list_cfg[0].slot_ass.from_string("01000");
        }
        if (rr_cfg_ded.phy_ch_list_cfg[0].ch_type == 2 || rr_cfg_ded.phy_ch_list_cfg[0].ch_type == 4 || rr_cfg_ded.phy_ch_list_cfg[0].ch_type == 6)
        {
          rr_cfg_ded.phy_ch_list_cfg[0].slot_ass.from_string("11000"); // psch1-2/psch_5-2/pdch1-2=11000,from right to left is slot: 0 ,1,2,3,4
        }
        rr_cfg_ded.phy_ch_list_cfg[0].direction = phy_ch_cfg_s::direction_opts::bidirection;

        if (rr_cfg_ded.phy_ch_list_cfg[0].ch_type != 1 && rr_cfg_ded.phy_ch_list_cfg[0].ch_type != 2)
        {
          rr_cfg_ded.phy_ch_list_cfg[0].shced_type_present = true;
          rr_cfg_ded.phy_ch_list_cfg[0].shced_type = phy_ch_cfg_s::shced_type_opts::static_t;
        }
        // configMap SRB_configMap;
        SRB_configMap.ul_Type = static_cast<ChanType_t>(rr_cfg_ded.phy_ch_list_cfg[0].ch_type.value);
        SRB_configMap.dl_Type = static_cast<ChanType_t>(rr_cfg_ded.phy_ch_list_cfg[0].ch_type.value);
        std::cout << " SRB_configMap.ul_Type = " << SRB_configMap.ul_Type << std::endl;
        std::cout << " SRB_configMap.dl_Type = " << SRB_configMap.dl_Type << std::endl;
        /*
        SRB_configMap.dataType=static_cast<DataType>(0);//control
        SRB_configMap.rlcType=static_cast<RlcType>(0);//AM
        SRB_configMap.voicetype=static_cast< VoiceType>(2); //NO Voice
        */
        SRB_configMap.dataType = Control;  // control
        SRB_configMap.rlcType = rlc_am;    // AM
        SRB_configMap.voicetype = N_Voice; // NO Voice

        parent->mac->addlcidMap(1, SRB_configMap);
      }
      // kuopin
      if (wx_area_mode == 1)
      {
        std::cout << "222wx_area_mode==1" << std::endl;
        rr_cfg_ded.phy_ch_list_cfg.resize(2);

        rr_cfg_ded.phy_ch_list_cfg[0].s_rnti.from_string("000110");
        rr_cfg_ded.phy_ch_list_cfg[0].ch_type = channel_type_e::pdch1_1;
        rr_cfg_ded.phy_ch_list_cfg[0].band_id_present = true;
        rr_cfg_ded.phy_ch_list_cfg[0].band_id.ba_id.from_string("001110"); // psch1_1=000110
        rr_cfg_ded.phy_ch_list_cfg[0].freq_id_present = true;
        rr_cfg_ded.phy_ch_list_cfg[0].freq_id.freq_id.from_string("01");
        rr_cfg_ded.phy_ch_list_cfg[0].slot_ass_present = true;

        if (rr_cfg_ded.phy_ch_list_cfg[0].ch_type == 1 || rr_cfg_ded.phy_ch_list_cfg[0].ch_type == 3 || rr_cfg_ded.phy_ch_list_cfg[0].ch_type == 5)
        {
          rr_cfg_ded.phy_ch_list_cfg[0].slot_ass.from_string("00100"); // psch5-1/pdch1-1/psch1-1=00100
        }
        if (rr_cfg_ded.phy_ch_list_cfg[0].ch_type == 2 || rr_cfg_ded.phy_ch_list_cfg[0].ch_type == 4 || rr_cfg_ded.phy_ch_list_cfg[0].ch_type == 6)
        {
          rr_cfg_ded.phy_ch_list_cfg[0].slot_ass.from_string("11000"); // psch1-2/psch_5-2/pdch1-2=11000
        }
        rr_cfg_ded.phy_ch_list_cfg[0].direction = phy_ch_cfg_s::direction_opts::uldirection;

        if (rr_cfg_ded.phy_ch_list_cfg[0].ch_type == 3 || rr_cfg_ded.phy_ch_list_cfg[0].ch_type == 4 || rr_cfg_ded.phy_ch_list_cfg[0].ch_type == 5 || rr_cfg_ded.phy_ch_list_cfg[0].ch_type == 6)
        {
          rr_cfg_ded.phy_ch_list_cfg[0].shced_type_present = true;
          rr_cfg_ded.phy_ch_list_cfg[0].shced_type = phy_ch_cfg_s::shced_type_opts::static_t;
        }

        // sp
        rr_cfg_ded.phy_ch_list_cfg[1].s_rnti.from_string("000110");
        rr_cfg_ded.phy_ch_list_cfg[1].ch_type = channel_type_e::ds_pdtch_1;
        rr_cfg_ded.phy_ch_list_cfg[1].band_id_present = false;
        rr_cfg_ded.phy_ch_list_cfg[1].band_id.ba_id.from_string("001001");
        rr_cfg_ded.phy_ch_list_cfg[1].freq_id_present = false;
        rr_cfg_ded.phy_ch_list_cfg[1].freq_id.freq_id.from_string("01");
        rr_cfg_ded.phy_ch_list_cfg[1].slot_ass_present = true;
        rr_cfg_ded.phy_ch_list_cfg[1].slot_ass.from_string("01000");
        rr_cfg_ded.phy_ch_list_cfg[1].pdtch_code_present = true;
        rr_cfg_ded.phy_ch_list_cfg[1].pdtch_code.pdtch_phy_code = 21;

        rr_cfg_ded.phy_ch_list_cfg[1].direction = phy_ch_cfg_s::direction_opts::dldirection;

        if (rr_cfg_ded.phy_ch_list_cfg[1].ch_type == 3 || rr_cfg_ded.phy_ch_list_cfg[1].ch_type == 4 || rr_cfg_ded.phy_ch_list_cfg[1].ch_type == 5 || rr_cfg_ded.phy_ch_list_cfg[1].ch_type == 6)
        {
          rr_cfg_ded.phy_ch_list_cfg[1].shced_type_present = true;
          rr_cfg_ded.phy_ch_list_cfg[1].shced_type = phy_ch_cfg_s::shced_type_opts::static_t;
        }

        configMap SRB_configMap;
        SRB_configMap.ul_Type = static_cast<ChanType_t>(rr_cfg_ded.phy_ch_list_cfg[0].ch_type.value);
        SRB_configMap.dl_Type = static_cast<ChanType_t>(rr_cfg_ded.phy_ch_list_cfg[1].ch_type.value);
        std::cout << " SRB_configMap.ul_Type = " << SRB_configMap.ul_Type << std::endl;
        std::cout << " SRB_configMap.dl_Type = " << SRB_configMap.dl_Type << std::endl;
        /*
        SRB_configMap.dataType=static_cast<DataType>(0);//control
        SRB_configMap.rlcType=static_cast<RlcType>(0);//AM
        SRB_configMap.voicetype=static_cast< VoiceType>(2); //NO Voice
        */
        SRB_configMap.dataType = Control;  // control
        SRB_configMap.rlcType = rlc_am;    // AM
        SRB_configMap.voicetype = N_Voice; // NO Voice

        parent->mac->addlcidMap(1, SRB_configMap);
      }
    }

    // Fill RR config dedicated
    // generate_rr_cfg_ded_setup(rr_cfg_ded);

    apply_rlc_srb_updates();
    apply_pdcp_srb_updates();

    /*
      configMap SRB_configMap;
      SRB_configMap.Type=static_cast<ChanType_t>(rr_cfg_ded.phy_ch_list_cfg[0].ch_type.value);
      std::cout<<" SRB_configMap.Type = "<<SRB_configMap.Type<<std::endl;

      SRB_configMap.dataType=static_cast<DataType>(0);//control
      SRB_configMap.rlcType=static_cast<RlcType>(0);//AM
      SRB_configMap.voicetype=static_cast< VoiceType>(2); //NO Voice

      SRB_configMap.dataType=Control;//control
      SRB_configMap.rlcType=rlc_am;//AM
      SRB_configMap.voicetype=N_Voice;//NO Voice

      parent->mac->addlcidMap(1,SRB_configMap);
    */

    std::string octet_str;
    send_s_dl_ccch(&dl_ccch_msg, &octet_str);
  }
  //------------------------------------------------------------------------------------------------------------------------------

  //-----------------------------------------IOT-2023/9/13------------------------------------------------------------------------
  void rrc::ue::iot_handle_rrc_con_req_s(iot_rrc_setup_req_s *msg)
  {
    asn1::json_writer json_writer;
    msg->to_json(json_writer);
    /*
    int a = 0;
    if (a) {
      send_connection_reject(procedure_result_code::fail_in_radio_interface_proc);
      return;
    }
    */
    iot_rrc_setup_req_r1_IEs_s *msg_r1 = &msg->iot_rrc_setup_req_r1;
    if (msg_r1->ue_id.type() == ue_id_c::types::iot_5g_s_tmsi)
    {
      iot_5g_s_tmsi = (uint64_t)msg_r1->ue_id.iot_5g_s_tmsi().iot_5g_s_tmsi.to_number();
      has_iot_5g_s_tmsi = true;
      for (auto &user : parent->users)
      {
        if (user.first != rnti && user.second->has_iot_5g_s_tmsi && user.second->iot_5g_s_tmsi == iot_5g_s_tmsi)
        {
          parent->logger.info("RRC connection request: UE context already exists. iot_5g_s_tmsi=%d", iot_5g_s_tmsi);
          user.second->state = RRC_STATE_IDLE;                                                                       // Set old rnti to IDLE so that enb doesn't send RRC Connection Release
          parent->s1ap->user_release(user.first, asn1::s1ap::cause_radio_network_opts::interaction_with_other_proc); //--------------------物联网下无S1AP。这应该为MM-RRC接口
          break;
        }
      }
    }

    iot_estab_cause = msg_r1->iot_estableishment_cause;
    iot_send_rrc_con_setup();
    state = RRC_STATE_WAIT_FOR_CON_SETUP_COMPLETE;
  }

  void rrc::ue::iot_send_rrc_con_setup()
  {
    iot_dl_ccch_msg_s iot_dl_ccch_msg;
    iot_dl_ccch_msg.msg.set_iot_rrc_setup();

    iot_rrc_setup_s &rrc_setup = iot_dl_ccch_msg.msg.set_iot_rrc_setup();
    rrc_setup.rrc_transaction_id.rrc_t_id = 0;
    iot_rrc_setup_r1_IEs_s &setup = rrc_setup.iot_rrc_setup_r1;

    // radio_resurce_cfg_ded_s& rr_cfg_ded   = setup.rr_cfg_ded;   //-------沿用SIB消息中的？暂时保留
    //  Fill RR config dedicated
    // generate_rr_cfg_ded_setup(rr_cfg_ded);

    setup.iot_t_reorder = iot_t_reordering_opts::ms100;

    iot_apply_rlc_srb_updates();
    apply_pdcp_srb_updates();
    std::string octet_str;

    iot_send_s_dl_ccch(&iot_dl_ccch_msg, &octet_str); //-------------------------------此处改动也需要仔细思考          //未赋初始值？
  }
  //------------------------------------------------------------------------------------------------------------------

  void rrc::ue::handle_rrc_con_req(rrc_conn_request_s *msg)
  {
    // Log event.
    asn1::json_writer json_writer;
    msg->to_json(json_writer);
    event_logger::get().log_rrc_event(ue_cell_list.get_ue_cc_idx(UE_PCELL_CC_IDX)->cell_common->enb_cc_idx,
                                      asn1::octstring_to_string(last_ul_msg->msg, last_ul_msg->N_bytes),
                                      json_writer.to_string(),
                                      static_cast<unsigned>(rrc_event_type::con_request),
                                      static_cast<unsigned>(procedure_result_code::none),
                                      rnti);

    if (not parent->s1ap->is_mme_connected())
    {
      parent->logger.error("MME isn't connected. Sending Connection Reject");
      send_connection_reject(procedure_result_code::error_mme_not_connected);
      return;
    }

    // Allocate PUCCH resources and reject if not available
    if (not init_pucch())
    {
      parent->logger.warning("Could not allocate PUCCH resources for rnti=0x%x. Sending Connection Reject", rnti);
      send_connection_reject(procedure_result_code::fail_in_radio_interface_proc);
      return;
    }

    rrc_conn_request_r8_ies_s *msg_r8 = &msg->crit_exts.rrc_conn_request_r8();

    if (msg_r8->ue_id.type() == init_ue_id_c::types::s_tmsi)
    {
      mmec = (uint8_t)msg_r8->ue_id.s_tmsi().mmec.to_number();
      m_tmsi = (uint32_t)msg_r8->ue_id.s_tmsi().m_tmsi.to_number();
      has_tmsi = true;

      // Make sure that the context does not already exist
      for (auto &user : parent->users)
      {
        if (user.first != rnti && user.second->has_tmsi && user.second->mmec == mmec && user.second->m_tmsi == m_tmsi)
        {
          parent->logger.info("RRC connection request: UE context already exists. M-TMSI=%d", m_tmsi);
          user.second->state = RRC_STATE_IDLE; // Set old rnti to IDLE so that enb doesn't send RRC Connection Release
          parent->s1ap->user_release(user.first, asn1::s1ap::cause_radio_network_opts::interaction_with_other_proc);
          break;
        }
      }
    }

    establishment_cause = msg_r8->establishment_cause;
    send_connection_setup();
    state = RRC_STATE_WAIT_FOR_CON_SETUP_COMPLETE;

    set_activity_timeout(UE_INACTIVITY_TIMEOUT);
  }

  void rrc::ue::send_connection_setup()
  {
    dl_ccch_msg_s dl_ccch_msg;
    dl_ccch_msg.msg.set_c1();

    rrc_conn_setup_s &rrc_setup = dl_ccch_msg.msg.c1().set_rrc_conn_setup();
    rrc_setup.rrc_transaction_id = (uint8_t)((transaction_id++) % 4);
    rrc_conn_setup_r8_ies_s &setup_r8 = rrc_setup.crit_exts.set_c1().set_rrc_conn_setup_r8();
    rr_cfg_ded_s &rr_cfg = setup_r8.rr_cfg_ded;

    // Fill RR config dedicated
    if (fill_rr_cfg_ded_setup(rr_cfg, parent->cfg, ue_cell_list))
    {
      parent->logger.error("Generating ConnectionSetup. Aborting");
      return;
    }
    // Apply ConnectionSetup Configuration to MAC scheduler
    mac_ctrl.handle_con_setup(setup_r8);
    // Add SRBs/DRBs, and configure RLC+PDCP
    apply_pdcp_srb_updates(setup_r8.rr_cfg_ded);
    apply_pdcp_drb_updates(setup_r8.rr_cfg_ded);
    apply_rlc_rb_updates(setup_r8.rr_cfg_ded);

    // Configure PHY layer
    apply_setup_phy_config_dedicated(rr_cfg.phys_cfg_ded); // It assumes SCell has not been set before

    std::string octet_str;
    send_dl_ccch(&dl_ccch_msg, &octet_str);

    // Log event.
    asn1::json_writer json_writer;
    dl_ccch_msg.to_json(json_writer);
    event_logger::get().log_rrc_event(ue_cell_list.get_ue_cc_idx(UE_PCELL_CC_IDX)->cell_common->enb_cc_idx,
                                      octet_str,
                                      json_writer.to_string(),
                                      static_cast<unsigned>(rrc_event_type::con_setup),
                                      static_cast<unsigned>(procedure_result_code::none),
                                      rnti);

    apply_rr_cfg_ded_diff(current_ue_cfg.rr_cfg, rr_cfg);
  }
  // #########################################################
  void rrc::ue::handle_rrc_con_setup_comp(asn1::rrc::rrc_con_setup_comp_s *msg, srsran::unique_byte_buffer_t pdu)
  {
    // if (parent->cfg.ttcn_test_enble && parent->cfg.ttcn_rrc_enble) {

    //   // RRC Message about connection establishment complete sent to TTCN
    //   srsran::unique_byte_buffer_t rrc_con_comp_ttcn = srsran::make_byte_buffer();
    //   rrc_con_comp_ttcn->init();
    //   rrc_con_comp_ttcn->msg[7] = RRC_CON_SETUP_COMP;
    //   rrc_con_comp_ttcn->N_bytes = 8;
    //   parent->assemble_general_interface(&rrc_con_comp_ttcn, 8);

    //   parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rrc_con_comp_ttcn));
    //   std::cout << "send_ttcn_info size:" << parent->rrc_adp->udp_.send_ttcn_info.size() << std::endl;

    //   asn1::json_writer json_writer;
    //   msg->to_json(json_writer);

    //   parent->phy->complete_config(rnti);
    //   parent->logger.info("RRCConnectionSetupComplete transaction ID: %d", msg->rrc_transaction_id.rrc_t_id);
    //   rrc_con_setup_com_r1_s* msg_r1 = &msg->rrc_con_setup_com_r1;
    //   pdu->N_bytes                   = msg_r1->ded_info_nas.ded_info_nas.size();
    //   memcpy(pdu->msg, msg_r1->ded_info_nas.ded_info_nas.data(), pdu->N_bytes);
    //   mac_ctrl.handle_con_setup_complete();
    // }
    asn1::json_writer json_writer;
    msg->to_json(json_writer);
    parent->phy->complete_config(rnti);
    parent->logger.info("RRCConnectionSetupComplete transaction ID: %d", msg->rrc_transaction_id.rrc_t_id);
    rrc_con_setup_com_r1_s *msg_r1 = &msg->rrc_con_setup_com_r1;
    pdu->N_bytes = msg_r1->ded_info_nas.ded_info_nas.size();
    memcpy(pdu->msg, msg_r1->ded_info_nas.ded_info_nas.data(), pdu->N_bytes);
    mac_ctrl.handle_con_setup_complete();
  }
  // #####################################################
  //---------------------------IOT-----------------------------
  void rrc::ue::iot_handle_rrc_con_setup_comp(iot_rrc_setup_complete_s *msg, srsran::unique_byte_buffer_t pdu)
  {
    asn1::json_writer json_writer;
    msg->to_json(json_writer);
    parent->phy->complete_config(rnti);
    parent->logger.info("RRCConnectionSetupComplete transaction ID: %d", msg->rrc_transaction_id.rrc_t_id);
    iot_rrc_setup_complete_r1_IEs_s *msg_r1 = &msg->iot_rrc_setup_complete_r1_IEs;
    pdu->N_bytes = msg_r1->ded_info_nas_msg.ded_info_nas.size();
    memcpy(pdu->msg, msg_r1->ded_info_nas_msg.ded_info_nas.data(), pdu->N_bytes);
    mac_ctrl.handle_con_setup_complete();
  }
  //-----------------------------------------------------------
  void rrc::ue::handle_rrc_con_setup_complete(rrc_conn_setup_complete_s *msg, srsran::unique_byte_buffer_t pdu)
  {
    // Log event.
    asn1::json_writer json_writer;
    msg->to_json(json_writer);
    event_logger::get().log_rrc_event(ue_cell_list.get_ue_cc_idx(UE_PCELL_CC_IDX)->cell_common->enb_cc_idx,
                                      asn1::octstring_to_string(last_ul_msg->msg, last_ul_msg->N_bytes),
                                      json_writer.to_string(),
                                      static_cast<unsigned>(rrc_event_type::con_setup_complete),
                                      static_cast<unsigned>(procedure_result_code::none),
                                      rnti);

    // Inform PHY about the configuration completion
    parent->phy->complete_config(rnti);

    parent->logger.info("RRCConnectionSetupComplete transaction ID: %d", msg->rrc_transaction_id);
    rrc_conn_setup_complete_r8_ies_s *msg_r8 = &msg->crit_exts.c1().rrc_conn_setup_complete_r8();

    // TODO: msg->selected_plmn_id - used to select PLMN from SIB1 list
    // TODO: if(msg->registered_mme_present) - the indicated MME should be used from a pool

    pdu->N_bytes = msg_r8->ded_info_nas.size();
    memcpy(pdu->msg, msg_r8->ded_info_nas.data(), pdu->N_bytes);

    // Signal MAC scheduler that configuration was successful
    mac_ctrl.handle_con_setup_complete();

    asn1::s1ap::rrc_establishment_cause_e s1ap_cause;
    s1ap_cause.value = (asn1::s1ap::rrc_establishment_cause_opts::options)establishment_cause.value;

    uint32_t enb_cc_idx = ue_cell_list.get_ue_cc_idx(UE_PCELL_CC_IDX)->cell_common->enb_cc_idx;
    if (has_tmsi)
    {
      parent->s1ap->initial_ue(rnti, enb_cc_idx, s1ap_cause, std::move(pdu), m_tmsi, mmec);
    }
    else
    {
      parent->s1ap->initial_ue(rnti, enb_cc_idx, s1ap_cause, std::move(pdu));
    }

    // 2> if the UE has radio link failure or handover failure information available
    if (msg->crit_exts.type().value == c1_or_crit_ext_opts::c1 and
        msg->crit_exts.c1().type().value ==
            rrc_conn_setup_complete_s::crit_exts_c_::c1_c_::types_opts::rrc_conn_setup_complete_r8)
    {
      const auto &complete_r8 = msg->crit_exts.c1().rrc_conn_setup_complete_r8();
      if (complete_r8.non_crit_ext.non_crit_ext.rlf_info_available_r10_present)
      {
        rlf_info_pending = true;
      }
    }
  }

  void rrc::ue::send_connection_reject(procedure_result_code cause)
  {
    mac_ctrl.handle_con_reject();

    dl_ccch_msg_s dl_ccch_msg;
    dl_ccch_msg.msg.set_c1().set_rrc_conn_reject().crit_exts.set_c1().set_rrc_conn_reject_r8().wait_time = 10;

    std::string octet_str;
    send_dl_ccch(&dl_ccch_msg, &octet_str);

    // Log event.
    asn1::json_writer json_writer;
    dl_ccch_msg.to_json(json_writer);
    event_logger::get().log_rrc_event(ue_cell_list.get_ue_cc_idx(UE_PCELL_CC_IDX)->cell_common->enb_cc_idx,
                                      octet_str,
                                      json_writer.to_string(),
                                      static_cast<unsigned>(rrc_event_type::con_reject),
                                      static_cast<unsigned>(cause),
                                      rnti);
  }

  //------------------------------2024/3/16-------------------//
  void rrc::ue::send_con_reject_ttcn()
  {
    // static const uint32_t release_delay = 0;
    // mac_ctrl.handle_con_reject();

    s_dl_ccch_msg_s s_dl_ccch_msg;
    rrc_con_reject_s &rrc_reject = s_dl_ccch_msg.msg.set_rrc_con_reject();
    rrc_con_reject_r1_s &rrc_reject_r1 = rrc_reject.rrc_con_reject;

    while (true)
    {
      if (parent->rrc_adp->udp_.rrc_receive_info.size() != 0)
      {
        srsran::unique_byte_buffer_t ttcn_reject = srsran::make_byte_buffer();
        ttcn_reject->init();
        parent->rrc_adp->udp_.rrc_receive_info.try_pop(ttcn_reject);
        std::cout << "receive rrc reject info from TTCN" << std::endl;
        for (uint32_t i = 0; i < ttcn_reject->N_bytes; i++)
        {
          printf("0x:%x\n", *(ttcn_reject->msg + i));
        }
        rrc_con_rej_info_.redirectPresent = ttcn_reject->msg[8];
        rrc_reject_r1.redir_info_present = rrc_con_rej_info_.redirectPresent;
        break;
      }
    }
    rrc_reject_r1.geo_accept_present = true;

    std::cout << "rem_user rrc_adp->udp_.is_paging_refuse:" << parent->rrc_adp->udp_.TC_75_is_paging_refuse << std::endl;
    std::cout << "rem_user rrc_adp->udp_.TC_74_t302_timeout:" << parent->rrc_adp->udp_.TC_74_t302_timeout << std::endl;
    if (parent->cfg.ttcn_test_enble && parent->cfg.ttcn_rrc_enble && (parent->rrc_adp->udp_.TC_75_is_paging_refuse || parent->rrc_adp->udp_.TC_74_t302_timeout))
    {
      std::cout << "enter di 2 ci kong bao" << std::endl;
      srsran::unique_byte_buffer_t rerurn_info_send = srsran::make_byte_buffer();
      rerurn_info_send->init();
      rerurn_info_send->msg[0] = 0x01;
      rerurn_info_send->msg[1] = 0x01;
      rerurn_info_send->msg[2] = 0x01;
      rerurn_info_send->msg[3] = 0x01;
      rerurn_info_send->N_bytes = 4;
      parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rerurn_info_send));
    }

    std::string octet_str;
    send_s_dl_ccch(&s_dl_ccch_msg, &octet_str);

    // Log event.
    asn1::json_writer json_writer;
    s_dl_ccch_msg.to_json(json_writer);

    auto start_time = std::chrono::high_resolution_clock::now();

    while (true)
    {
      auto now_time = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::seconds>(now_time - start_time);
      if (duration.count() > 0.5)
      {
        break;
      }
    }
    parent->rem_user(rnti);

    // parent->task_sched.defer_callback(release_delay, [this]() { parent->rem_user(rnti); });
  }
  //-----------------------------------------------------------//

  /*
   * Connection Reestablishment
   */

  // 2024/8/16
  void rrc::ue::s_handle_rrc_con_reest_req(rrc_con_reest_req_s *msg)
  {
    printf("------------handle Connection Reestablishment request1--------------\n");
    asn1::json_writer json_writer;
    msg->to_json(json_writer);

    rrc_con_reest_req_r1_s *rrc_con_reest_req_r1 = &msg->rrc_con_reest_req;

    // s_rnti=(uint16_t)rrc_con_reest_req_r1->s_rnti.to_number();
    // freq_id=(uint8_t)rrc_con_reest_req_r1->freq_id.freq_id.to_number();
    // band_id=(uint16_t)rrc_con_reest_req_r1->band_id.ba_id.to_number();
    // trig_beam_id=(uint32_t)rrc_con_reest_req_r1->tri_beam_id.beam_id.to_number();

    // reest_cause                                  = rrc_con_reest_req_r1->reest_cause;
    if (parent->rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 716)
    {
      printf("send rrc_reset_ttcn to ttcn\n ");
      srsran::unique_byte_buffer_t rrc_reset_ttcn = srsran::make_byte_buffer();
      rrc_reset_ttcn->init();
      rrc_reset_ttcn->msg[0] = 0x01;
      rrc_reset_ttcn->msg[1] = 0x03;
      rrc_reset_ttcn->msg[2] = 0x00;
      rrc_reset_ttcn->msg[3] = 0x02;
      rrc_reset_ttcn->msg[4] = 0xcc;
      rrc_reset_ttcn->msg[5] = 0x00;
      rrc_reset_ttcn->msg[6] = 0x01;
      rrc_reset_ttcn->msg[7] = RRC_CON_REEST_REQ;

      rrc_reset_ttcn->N_bytes = 8;

      // parent->assemble_general_interface(&rrc_reset_ttcn, 8);

      parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rrc_reset_ttcn));
      std::cout << "send_ttcn_info size:"
                << parent->rrc_adp->udp_.send_ttcn_info.size() << std::endl;

      srsran::unique_byte_buffer_t ttcn_reconf = srsran::make_byte_buffer();
      while (true)
      {
        // TTCN-3 config parameters
        if (parent->rrc_adp->udp_.rrc_receive_info.size() != 0)
        {
          std::cout << " TTCN reconf Info " << std::endl;
          ttcn_reconf->init();
          parent->rrc_adp->udp_.rrc_receive_info.try_pop(ttcn_reconf);
          parent->get_general_interface(&ttcn_reconf);
          break;
        }
      }
    }
    if (parent->rrc_adp->udp_.TC_715_BEAM2_REEST)
    {
      printf("send rrc_reset_ttcn to ttcn\n ");
      srsran::unique_byte_buffer_t rrc_reset_ttcn = srsran::make_byte_buffer();
      rrc_reset_ttcn->init();
      rrc_reset_ttcn->msg[7] = RRC_CON_REEST_REQ;
      rrc_reset_ttcn->N_bytes = 8;
      parent->assemble_general_interface(&rrc_reset_ttcn, 8);
      parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rrc_reset_ttcn));
      std::cout << "send_ttcn_info size:"
                << parent->rrc_adp->udp_.send_ttcn_info.size() << std::endl;

      srsran::unique_byte_buffer_t ttcn_reconf = srsran::make_byte_buffer();
      while (true)
      {
        // TTCN-3 config parameters
        if (parent->rrc_adp->udp_.rrc_receive_info.size() != 0)
        {
          std::cout << " TTCN reest Info " << std::endl;
          ttcn_reconf->init();
          parent->rrc_adp->udp_.rrc_receive_info.try_pop(ttcn_reconf);
          parent->get_general_interface(&ttcn_reconf);
          break;
        }
      }
    }

    s_send_connection_reest();
    state = RRC_STATE_WAIT_FOR_CON_REEST_COMPLETE;
  }

  void rrc::ue::s_send_connection_reest()
  {
    printf("--------------send Connection Reestablishment message1------------------\n");
    s_dl_ccch_msg_s s_dl_ccch_msg;
    s_dl_ccch_msg.msg.set_rrc_con_reest();
    rrc_con_reest_s &rrc_con_reest = s_dl_ccch_msg.msg.rrc_con_reest();

    rrc_con_reest.rrc_transaction_id.rrc_t_id = 0;
    radio_resurce_cfg_ded_s &radio_resurce_cfg = rrc_con_reest.rrc_con_reest_r1.rr_cfg_ded;

    rrc_con_reest.rrc_con_reest_r1.rr_cfg_ded.srb_to_add_present = true;
    // rrc_con_reest.rrc_con_reest_r1.rr_cfg_ded.drb_to_add_mod_list_present = true;
    rrc_con_reest.rrc_con_reest_r1.rr_cfg_ded.phy_ch_list_cfg_present = true;
    // rrc_con_reest.rrc_con_reest_r1.rr_cfg_ded.phy_chan_list_cfg_present   = true;
    // rrc_con_reest.rrc_con_reest_r1.rr_cfg_ded.drb_to_add_mod_list.resize(1);
    // drb_to_add_modi_s& drb_to_add_mod = rrc_con_reest.rrc_con_reest_r1.rr_cfg_ded.drb_to_add_mod_list[0];

    if (wx_area_mode == 0)
    {
      std::cout << "111chongjian wx_area_mode==0" << std::endl;
      radio_resurce_cfg.phy_ch_list_cfg.resize(1);
      radio_resurce_cfg.phy_ch_list_cfg[0].s_rnti.from_string("000110");
      radio_resurce_cfg.phy_ch_list_cfg[0].ch_type = channel_type_e::pdch1_1;
      radio_resurce_cfg.phy_ch_list_cfg[0].band_id_present = true;
      radio_resurce_cfg.phy_ch_list_cfg[0].band_id.ba_id.from_string("001110");
      radio_resurce_cfg.phy_ch_list_cfg[0].freq_id_present = true;
      radio_resurce_cfg.phy_ch_list_cfg[0].freq_id.freq_id.from_string("01");
      radio_resurce_cfg.phy_ch_list_cfg[0].slot_ass_present = true;

      if (radio_resurce_cfg.phy_ch_list_cfg[0].ch_type == 1 || radio_resurce_cfg.phy_ch_list_cfg[0].ch_type == 3 ||
          radio_resurce_cfg.phy_ch_list_cfg[0].ch_type == 5)
      {
        radio_resurce_cfg.phy_ch_list_cfg[0].slot_ass.from_string("00100"); // psch5-1/pdch1-1/psch1-1=00100
      }
      if (radio_resurce_cfg.phy_ch_list_cfg[0].ch_type == 2 || radio_resurce_cfg.phy_ch_list_cfg[0].ch_type == 4 ||
          radio_resurce_cfg.phy_ch_list_cfg[0].ch_type == 6)
      {
        radio_resurce_cfg.phy_ch_list_cfg[0].slot_ass.from_string("11000"); // psch1-2/psch_5-2/pdch1-2=11000,from right to left is slot: 0 ,1,2,3,4
      }
      radio_resurce_cfg.phy_ch_list_cfg[0].direction = phy_ch_cfg_s::direction_opts::bidirection;

      if (radio_resurce_cfg.phy_ch_list_cfg[0].ch_type != 1 && radio_resurce_cfg.phy_ch_list_cfg[0].ch_type != 2)
      {
        radio_resurce_cfg.phy_ch_list_cfg[0].shced_type_present = true;
        radio_resurce_cfg.phy_ch_list_cfg[0].shced_type = phy_ch_cfg_s::shced_type_opts::static_t;
      }

      SRB_configMap.ul_Type = static_cast<ChanType_t>(radio_resurce_cfg.phy_ch_list_cfg[0].ch_type.value);
      SRB_configMap.dl_Type = static_cast<ChanType_t>(radio_resurce_cfg.phy_ch_list_cfg[0].ch_type.value);
      std::cout << " SRB_configMap.ul_Typechongjian1e = " << SRB_configMap.ul_Type << std::endl;
      std::cout << " SRB_configMap.dl_Typechongjian1e = " << SRB_configMap.dl_Type << std::endl;
      /*
      SRB_configMap.dataType=static_cast<DataType>(0);//control
      SRB_configMap.rlcType=static_cast<RlcType>(0);//AM
      SRB_configMap.voicetype=static_cast< VoiceType>(2); //NO Voice
      */
      SRB_configMap.dataType = Control;  // control
      SRB_configMap.rlcType = rlc_am;    // AM
      SRB_configMap.voicetype = N_Voice; // NO Voice
      parent->mac->addlcidMap(1, SRB_configMap);
    }
    if (wx_area_mode == 1)
    {
      std::cout << "222chongjian wx_area_mode==1" << std::endl;
      radio_resurce_cfg.phy_ch_list_cfg.resize(2);

      radio_resurce_cfg.phy_ch_list_cfg[0].s_rnti.from_string("000110");
      radio_resurce_cfg.phy_ch_list_cfg[0].ch_type = channel_type_e::pdch1_1;
      radio_resurce_cfg.phy_ch_list_cfg[0].band_id_present = true;
      radio_resurce_cfg.phy_ch_list_cfg[0].band_id.ba_id.from_string("001110"); // psch1_1=000110
      radio_resurce_cfg.phy_ch_list_cfg[0].freq_id_present = true;
      radio_resurce_cfg.phy_ch_list_cfg[0].freq_id.freq_id.from_string("01");
      radio_resurce_cfg.phy_ch_list_cfg[0].slot_ass_present = true;

      if (radio_resurce_cfg.phy_ch_list_cfg[0].ch_type == 1 || radio_resurce_cfg.phy_ch_list_cfg[0].ch_type == 3 ||
          radio_resurce_cfg.phy_ch_list_cfg[0].ch_type == 5)
      {
        radio_resurce_cfg.phy_ch_list_cfg[0].slot_ass.from_string("00100"); // psch5-1/pdch1-1/psch1-1=00100
      }
      if (radio_resurce_cfg.phy_ch_list_cfg[0].ch_type == 2 || radio_resurce_cfg.phy_ch_list_cfg[0].ch_type == 4 ||
          radio_resurce_cfg.phy_ch_list_cfg[0].ch_type == 6)
      {
        radio_resurce_cfg.phy_ch_list_cfg[0].slot_ass.from_string("11000"); // psch1-2/psch_5-2/pdch1-2=11000
      }
      radio_resurce_cfg.phy_ch_list_cfg[0].direction = phy_ch_cfg_s::direction_opts::uldirection;

      if (radio_resurce_cfg.phy_ch_list_cfg[0].ch_type == 3 || radio_resurce_cfg.phy_ch_list_cfg[0].ch_type == 4 ||
          radio_resurce_cfg.phy_ch_list_cfg[0].ch_type == 5 || radio_resurce_cfg.phy_ch_list_cfg[0].ch_type == 6)
      {
        radio_resurce_cfg.phy_ch_list_cfg[0].shced_type_present = true;
        radio_resurce_cfg.phy_ch_list_cfg[0].shced_type = phy_ch_cfg_s::shced_type_opts::static_t;
      }

      // sp
      radio_resurce_cfg.phy_ch_list_cfg[1].s_rnti.from_string("000110");
      radio_resurce_cfg.phy_ch_list_cfg[1].ch_type = channel_type_e::ds_pdtch_1;
      radio_resurce_cfg.phy_ch_list_cfg[1].band_id_present = false;
      radio_resurce_cfg.phy_ch_list_cfg[1].band_id.ba_id.from_string("001001");
      radio_resurce_cfg.phy_ch_list_cfg[1].freq_id_present = false;
      radio_resurce_cfg.phy_ch_list_cfg[1].freq_id.freq_id.from_string("01");
      radio_resurce_cfg.phy_ch_list_cfg[1].slot_ass_present = true;
      radio_resurce_cfg.phy_ch_list_cfg[1].slot_ass.from_string("01000");
      radio_resurce_cfg.phy_ch_list_cfg[1].pdtch_code_present = true;
      radio_resurce_cfg.phy_ch_list_cfg[1].pdtch_code.pdtch_phy_code = 21;

      radio_resurce_cfg.phy_ch_list_cfg[1].direction = phy_ch_cfg_s::direction_opts::dldirection;

      if (radio_resurce_cfg.phy_ch_list_cfg[1].ch_type == 3 || radio_resurce_cfg.phy_ch_list_cfg[1].ch_type == 4 ||
          radio_resurce_cfg.phy_ch_list_cfg[1].ch_type == 5 || radio_resurce_cfg.phy_ch_list_cfg[1].ch_type == 6)
      {
        radio_resurce_cfg.phy_ch_list_cfg[1].shced_type_present = true;
        radio_resurce_cfg.phy_ch_list_cfg[1].shced_type = phy_ch_cfg_s::shced_type_opts::static_t;
      }

      configMap SRB_configMap;
      SRB_configMap.ul_Type = static_cast<ChanType_t>(radio_resurce_cfg.phy_ch_list_cfg[0].ch_type.value);
      SRB_configMap.dl_Type = static_cast<ChanType_t>(radio_resurce_cfg.phy_ch_list_cfg[1].ch_type.value);
      std::cout << " SRB_configMap.ul_Typechongjian2  = " << SRB_configMap.ul_Type << std::endl;
      std::cout << " SRB_configMap.dl_Typechongjian2  = " << SRB_configMap.dl_Type << std::endl;
      /*
      SRB_configMap.dataType=static_cast<DataType>(0);//control
      SRB_configMap.rlcType=static_cast<RlcType>(0);//AM
      SRB_configMap.voicetype=static_cast< VoiceType>(2); //NO Voice
      */
      SRB_configMap.dataType = Control;  // control
      SRB_configMap.rlcType = rlc_am;    // AM
      SRB_configMap.voicetype = N_Voice; // NO Voice

      parent->mac->addlcidMap(1, SRB_configMap);
    }
    // s_reset_apply_rlc_rb_updates(drb_to_add_mod);
    // s_apply_pdcp_srb_updates();
    apply_rlc_srb_updates();
    apply_pdcp_srb_updates();
    // s_reset_apply_pdcp_drb_updates(drb_to_add_mod);
    std::string octet_str;
    send_s_dl_ccch(&s_dl_ccch_msg, &octet_str);
  }

  void rrc::ue::s_handle_rrc_con_reest_complete(rrc_con_reest_comp_s *msg, srsran::unique_byte_buffer_t pdu)
  {
    printf("--------------handle Connection Reestablishment complete message1--------------\n");
    asn1::json_writer json_writer;
    msg->to_json(json_writer);
    msg->rrc_transaction_id.rrc_t_id = 0;
    if (parent->cfg.ttcn_test_enble && parent->cfg.ttcn_rrc_enble && (parent->rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 716 || parent->rrc_adp->udp_.TC_715_BEAM2_REEST))
    {
      printf("send rrc_reset_ttcn to ttcn\n ");
      srsran::unique_byte_buffer_t rrc_reset_ttcn = srsran::make_byte_buffer();
      rrc_reset_ttcn->init();
      // rrc_reset_ttcn->msg[0]=0x01;
      // rrc_reset_ttcn->msg[1]=0x03;
      // rrc_reset_ttcn->msg[2]=0x00;
      // rrc_reset_ttcn->msg[3]=0x02;
      // rrc_reset_ttcn->msg[4]=0xcc;
      // rrc_reset_ttcn->msg[5]=0x00;
      // rrc_reset_ttcn->msg[6]=0x01;
      rrc_reset_ttcn->msg[7] = RRC_CON_REEST_COMP;

      rrc_reset_ttcn->N_bytes = 8;

      parent->assemble_general_interface(&rrc_reset_ttcn, 8);

      parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rrc_reset_ttcn));
      std::cout << "send_ttcn_info size:"
                << parent->rrc_adp->udp_.send_ttcn_info.size() << std::endl;
    }

    parent->phy->complete_config(rnti);

    mac_ctrl.handle_con_reest_complete();
  }
  void rrc::ue::handle_rrc_con_reest_req(rrc_conn_reest_request_s *msg)
  {
    // Log event.
    asn1::json_writer json_writer;
    msg->to_json(json_writer);
    event_logger::get().log_rrc_event(ue_cell_list.get_ue_cc_idx(UE_PCELL_CC_IDX)->cell_common->enb_cc_idx,
                                      asn1::octstring_to_string(last_ul_msg->msg, last_ul_msg->N_bytes),
                                      json_writer.to_string(),
                                      static_cast<unsigned>(rrc_event_type::con_reest_req),
                                      static_cast<unsigned>(procedure_result_code::none),
                                      rnti);
    const rrc_conn_reest_request_r8_ies_s &req_r8 = msg->crit_exts.rrc_conn_reest_request_r8();
    uint16_t old_rnti = req_r8.ue_id.c_rnti.to_number();

    if (not parent->s1ap->is_mme_connected())
    {
      parent->logger.error("RRCReestablishmentReject for rnti=0x%x. Cause: MME not connected", rnti);
      send_connection_reest_rej(procedure_result_code::error_mme_not_connected);
      srsran::console("RRCReestablishmentReject for rnti=0x%x. Cause: MME not connected.\n", rnti);
      return;
    }

    // Allocate PUCCH resources and reject if not available
    if (not init_pucch())
    {
      parent->logger.warning("Could not allocate PUCCH resources for rnti=0x%x. Sending RRCReestablishmentReject", rnti);
      send_connection_reest_rej(procedure_result_code::fail_in_radio_interface_proc);
      return;
    }

    parent->logger.debug("rnti=0x%x, phyid=0x%x, smac=0x%x, cause=%s",
                         (uint32_t)msg->crit_exts.rrc_conn_reest_request_r8().ue_id.c_rnti.to_number(),
                         msg->crit_exts.rrc_conn_reest_request_r8().ue_id.pci,
                         (uint32_t)msg->crit_exts.rrc_conn_reest_request_r8().ue_id.short_mac_i.to_number(),
                         msg->crit_exts.rrc_conn_reest_request_r8().reest_cause.to_string());

    if (not is_idle())
    {
      // The created RNTI has to receive ReestablishmentRequest as first message
      parent->logger.error(
          "RRCReestablishmentReject for rnti=0x%x. Cause: old rnti=0x%x is not in RRC_IDLE", rnti, old_rnti);
      send_connection_reest_rej(procedure_result_code::error_unknown_rnti);
      srsran::console("ERROR: RRCReestablishmentReject for rnti=0x%x not in RRC_IDLE\n", rnti);
      return;
    }

    uint16_t old_pci = msg->crit_exts.rrc_conn_reest_request_r8().ue_id.pci;
    const enb_cell_common *old_cell = parent->cell_common_list->get_pci(old_pci);
    auto old_ue_it = parent->users.find(old_rnti);

    // Reject unrecognized rntis, and PCIs that do not belong to eNB
    if (old_ue_it == parent->users.end() or old_cell == nullptr or
        old_ue_it->second->ue_cell_list.get_enb_cc_idx(old_cell->enb_cc_idx) == nullptr)
    {
      send_connection_reest_rej(procedure_result_code::error_unknown_rnti);
      parent->logger.info(
          "RRCReestablishmentReject for rnti=0x%x. Cause: no rnti=0x%x context available", rnti, old_rnti);
      srsran::console("RRCReestablishmentReject for rnti=0x%x. Cause: no context available\n", rnti);
      return;
    }
    ue *old_ue = old_ue_it->second.get();
    bool old_ue_supported_endc = old_ue->endc_handler and old_ue->endc_handler->is_endc_supported();
    if (not old_ue_supported_endc and req_r8.reest_cause.value == reest_cause_opts::recfg_fail)
    {
      // Reestablishment Reject for ReconfigFailures of LTE-only mode
      parent->logger.info(
          "RRCReestablishmentReject for rnti=0x%x. Cause: Unhandled Reestablishment due to ReconfigFailure", rnti);
      srsran::console("RRCReestablishmentReject for rnti=0x%x. Cause: Unhandled Reestablishment due to ReconfigFailure\n",
                      rnti);
      return;
    }

    // Reestablishment procedure going forward
    parent->logger.info("ConnectionReestablishmentRequest for rnti=0x%x. Sending Connection Reestablishment", old_rnti);
    srsran::console(
        "User 0x%x requesting RRC Reestablishment as 0x%x. Cause: %s\n", rnti, old_rnti, req_r8.reest_cause.to_string());

    if (endc_handler != nullptr)
    {
      old_ue->endc_handler->trigger(rrc_endc::rrc_reest_rx_ev{});
    }

    // Cancel Handover in Target eNB if on-going
    asn1::s1ap::cause_c cause;
    cause.set_radio_network().value = asn1::s1ap::cause_radio_network_opts::interaction_with_other_proc;
    old_ue->mobility_handler->trigger(rrc_mobility::ho_cancel_ev{cause});

    // Recover security setup
    const enb_cell_common *pcell_cfg = get_ue_cc_cfg(UE_PCELL_CC_IDX);
    ue_security_cfg = old_ue->ue_security_cfg;
    ue_security_cfg.regenerate_keys_handover(pcell_cfg->cell_cfg.pci, pcell_cfg->cell_cfg.dl_earfcn);

    // send RRC Reestablishment message and restore bearer configuration
    send_connection_reest(old_ue->ue_security_cfg.get_ncc());

    // Get PDCP entity state (required when using RLC AM)
    for (const auto &erab_pair : old_ue->bearer_list.get_erabs())
    {
      uint16_t lcid = erab_pair.second.lcid;
      old_reest_pdcp_state[lcid] = {};
      parent->pdcp->get_bearer_state(old_rnti, lcid, &old_reest_pdcp_state[lcid]);

      parent->logger.debug("Getting PDCP state for E-RAB with LCID %d", lcid);
      parent->logger.debug("Got PDCP state: TX HFN %d, NEXT_PDCP_TX_SN %d, RX_HFN %d, NEXT_PDCP_RX_SN %d, "
                           "LAST_SUBMITTED_PDCP_RX_SN %d",
                           old_reest_pdcp_state[lcid].tx_hfn,
                           old_reest_pdcp_state[lcid].next_pdcp_tx_sn,
                           old_reest_pdcp_state[lcid].rx_hfn,
                           old_reest_pdcp_state[lcid].next_pdcp_rx_sn,
                           old_reest_pdcp_state[lcid].last_submitted_pdcp_rx_sn);
    }

    // Make sure UE capabilities are copied over to new RNTI
    eutra_capabilities = old_ue->eutra_capabilities;
    eutra_capabilities_unpacked = old_ue->eutra_capabilities_unpacked;
    ue_capabilities = old_ue->ue_capabilities;
    if (parent->logger.debug.enabled())
    {
      asn1::json_writer js{};
      eutra_capabilities.to_json(js);
      parent->logger.debug("rnti=0x%x EUTRA capabilities: %s", rnti, js.to_string().c_str());
    }
    if (endc_handler)
    {
      if (req_r8.reest_cause.value == reest_cause_opts::recfg_fail)
      {
        // In case of Reestablishment due to ReconfFailure, avoid re-enabling NR EN-DC, otherwise
        // the eNB and UE may enter in a reconfiguration + reestablishment loop.
        endc_handler->trigger(rrc_endc::disable_endc_ev{});
      }
      else
      {
        // In case of Reestablishment with cause other than ReconfFailure, recompute whether
        // the new RNTI supports NR EN-DC.
        endc_handler->handle_eutra_capabilities(eutra_capabilities);
      }
    }

    // Recover GTP-U tunnels and S1AP context
    parent->gtpu->mod_bearer_rnti(old_rnti, rnti);
    parent->s1ap->user_mod(old_rnti, rnti);

    // Reestablish E-RABs of old rnti later, during ConnectionReconfiguration
    bearer_list.reestablish_bearers(std::move(old_ue->bearer_list));

    // remove old RNTI
    old_ue->mac_ctrl.set_drb_activation(false);
    parent->rem_user_thread(old_rnti);

    state = RRC_STATE_WAIT_FOR_CON_REEST_COMPLETE;
    set_activity_timeout(MSG5_RX_TIMEOUT);
  }

  void rrc::ue::send_connection_reest(uint8_t ncc)
  {
    dl_ccch_msg_s dl_ccch_msg;
    auto &reest = dl_ccch_msg.msg.set_c1().set_rrc_conn_reest();
    reest.rrc_transaction_id = (uint8_t)((transaction_id++) % 4);
    rrc_conn_reest_r8_ies_s &reest_r8 = reest.crit_exts.set_c1().set_rrc_conn_reest_r8();
    rr_cfg_ded_s &rr_cfg = reest_r8.rr_cfg_ded;

    // Fill RR config dedicated
    if (fill_rr_cfg_ded_setup(rr_cfg, parent->cfg, ue_cell_list))
    {
      parent->logger.error("Generating ConnectionReestablishment. Aborting...");
      return;
    }

    // Set NCC
    reest_r8.next_hop_chaining_count = ncc;

    // Apply ConnectionReest Configuration to MAC scheduler
    mac_ctrl.handle_con_reest(reest_r8);

    // Add SRBs/DRBs, and configure RLC+PDCP
    apply_pdcp_srb_updates(rr_cfg);
    apply_pdcp_drb_updates(rr_cfg);
    apply_rlc_rb_updates(rr_cfg);

    // Configure PHY layer
    apply_setup_phy_config_dedicated(rr_cfg.phys_cfg_ded); // It assumes SCell has not been set before

    std::string octet_str;
    send_dl_ccch(&dl_ccch_msg, &octet_str);

    apply_rr_cfg_ded_diff(current_ue_cfg.rr_cfg, rr_cfg);

    // Log event.
    asn1::json_writer json_writer;
    dl_ccch_msg.to_json(json_writer);
    event_logger::get().log_rrc_event(ue_cell_list.get_ue_cc_idx(UE_PCELL_CC_IDX)->cell_common->enb_cc_idx,
                                      octet_str,
                                      json_writer.to_string(),
                                      static_cast<unsigned>(rrc_event_type::con_reest),
                                      static_cast<unsigned>(procedure_result_code::none),
                                      rnti);
  }

  void rrc::ue::handle_rrc_con_reest_complete(rrc_conn_reest_complete_s *msg, srsran::unique_byte_buffer_t pdu)
  {
    // Log event.
    asn1::json_writer json_writer;
    msg->to_json(json_writer);
    event_logger::get().log_rrc_event(ue_cell_list.get_ue_cc_idx(UE_PCELL_CC_IDX)->cell_common->enb_cc_idx,
                                      asn1::octstring_to_string(last_ul_msg->msg, last_ul_msg->N_bytes),
                                      json_writer.to_string(),
                                      static_cast<unsigned>(rrc_event_type::con_reest_complete),
                                      static_cast<unsigned>(procedure_result_code::none),
                                      rnti);

    // Inform PHY about the configuration completion
    parent->phy->complete_config(rnti);

    parent->logger.info("RRCConnectionReestablishComplete transaction ID: %d", msg->rrc_transaction_id);

    // TODO: msg->selected_plmn_id - used to select PLMN from SIB1 list
    // TODO: if(msg->registered_mme_present) - the indicated MME should be used from a pool

    // signal mac scheduler that configuration was successful
    mac_ctrl.handle_con_reest_complete();

    state = RRC_STATE_REESTABLISHMENT_COMPLETE;

    // 2> if the UE has radio link failure or handover failure information available
    if (msg->crit_exts.type().value == rrc_conn_reest_complete_s::crit_exts_c_::types_opts::rrc_conn_reest_complete_r8)
    {
      const auto &complete_r8 = msg->crit_exts.rrc_conn_reest_complete_r8();
      if (complete_r8.non_crit_ext.rlf_info_available_r9_present)
      {
        rlf_info_pending = true;
      }
    }

    send_connection_reconf(std::move(pdu));
  }

  void rrc::ue::send_connection_reest_rej(procedure_result_code cause)
  {
    mac_ctrl.handle_con_reject();

    dl_ccch_msg_s dl_ccch_msg;
    dl_ccch_msg.msg.set_c1().set_rrc_conn_reest_reject().crit_exts.set_rrc_conn_reest_reject_r8();

    std::string octet_str;
    send_dl_ccch(&dl_ccch_msg, &octet_str);

    // Log event.
    asn1::json_writer json_writer;
    dl_ccch_msg.to_json(json_writer);
    event_logger::get().log_rrc_event(ue_cell_list.get_ue_cc_idx(UE_PCELL_CC_IDX)->cell_common->enb_cc_idx,
                                      octet_str,
                                      json_writer.to_string(),
                                      static_cast<unsigned>(rrc_event_type::con_reest_reject),
                                      static_cast<unsigned>(cause),
                                      rnti);
  }

  /*
   * Connection Reconfiguration
   */

  //---------------------------------2023.10.12-------------------------------
  /*卫星接入网的连接重配Connection Reconfiguration*/
  // void rrc::ue::send_rrc_con_reconf(
  //                            uint16_t rnti,
  //                            uint8_t  qfi_nas,
  //                            uint16_t pdu_ses_id,
  //                            srsran::unique_byte_buffer_t pdu,

  //                            srsran::const_byte_span nas_pdu)
  // {

  //   if (parent->cfg.ttcn_test_enble && parent->cfg.ttcn_rrc_enble && parent->rrc_adp->udp_.TC_711_reconfig) {
  //     while (true) {
  //         if (parent->rrc_adp->udp_.rrc_receive_info.size() != 0) {
  //             std::cout << " TTCN reconf Info " << std::endl;
  //             srsran::unique_byte_buffer_t ttcn_reconf =
  //                 srsran::make_byte_buffer();
  //             ttcn_reconf->init();
  //             parent->rrc_adp->udp_.rrc_receive_info.try_pop(ttcn_reconf);
  //             parent->get_general_interface(&ttcn_reconf);

  //             //TTCN-3 config parameters
  //             break;
  //         }
  //     }
  // }
  //   update_scells();
  //   s_dl_dcch_msg_s  ss_dl_dcch_msg;
  //   rrc_con_recfg_s& recofg_si = ss_dl_dcch_msg.msg.set_rrc_con_recfg();
  //   recofg_si.rrc_tran_iden    = 0;
  //   rrc_con_recfg_r1_ies_s& recfg_r1 = recofg_si.rrc_con_recfg_r1;

  // // MeasConfig的消息
  //   recofg_si.rrc_con_recfg_r1.meas_cfg_present = true;
  //   // recofg_si.rrc_con_recfg_r1.meas_cfg.meas_gap_cfg_list.set_meas_gap_cfg_list_normal();
  //   meas_cofg_s& mea_cfg = recofg_si.rrc_con_recfg_r1.meas_cfg;
  //   mea_cfg.meas_gap_cfg_list_present = true;
  //   mea_cfg.s_mea_sure_present = true;
  //    if (wx_area_mode==1)
  //    {
  //     meas_gap_cfg_list_dl_freq_spread_s& meas_gap_cfg_list_dl_freq= mea_cfg.meas_gap_cfg_list.set_meas_gap_cfg_list_dl_freq_spread_s();
  //     meas_gap_cfg_list_dl_freq.resize(5);
  //     meas_gap_cfg_list_dl_freq[0].beam_id=2;
  //     meas_gap_cfg_list_dl_freq[0].sec_syn_group_id=3;

  //     meas_gap_cfg_list_dl_freq[1].beam_id=3;
  //     meas_gap_cfg_list_dl_freq[1].sec_syn_group_id=3;

  //     meas_gap_cfg_list_dl_freq[2].beam_id=3;
  //     meas_gap_cfg_list_dl_freq[2].sec_syn_group_id=4;

  //     meas_gap_cfg_list_dl_freq[3].beam_id=4;
  //     meas_gap_cfg_list_dl_freq[3].sec_syn_group_id=5;

  //     meas_gap_cfg_list_dl_freq[4].beam_id=5;
  //     meas_gap_cfg_list_dl_freq[4].sec_syn_group_id=6;

  //  rssi_dl_freq_spread_s& rssi_nor_sp = recofg_si.rrc_con_recfg_r1.meas_cfg.s_mea_sure.set_rssi_dl_freq_spread();
  //   rssi_nor_sp.rssi_dl_freq_spread   = 32;
  //    }

  // if (wx_area_mode==0)
  //    {
  //   meas_gap_cfg_list_normal_s& mea_gap_normal = mea_cfg.meas_gap_cfg_list.set_meas_gap_cfg_list_normal();
  //   mea_gap_normal.resize(4);

  //   mea_gap_normal[0].beam_index   = 1;
  //   mea_gap_normal[0].frame_offset = 1;
  //   mea_gap_normal[0].fcch_ba_id.ba_id.from_string("000110");
  //   mea_gap_normal[0].fcch_freq_id.freq_id.from_string("01");
  //   mea_gap_normal[0].meas_norm_ratio = meas_gap_cfg_normal_s::meas_norm_ratio_opts::dB9;

  //   mea_gap_normal[1].beam_index   = 2;
  //   mea_gap_normal[1].frame_offset = 2;
  //   mea_gap_normal[1].fcch_ba_id.ba_id.from_string("000111");
  //   mea_gap_normal[1].fcch_freq_id.freq_id.from_string("01");
  //   mea_gap_normal[1].meas_norm_ratio = meas_gap_cfg_normal_s::meas_norm_ratio_opts::dB9;

  //   mea_gap_normal[2].beam_index   = 3;
  //   mea_gap_normal[2].frame_offset = 3;
  //   mea_gap_normal[2].fcch_ba_id.ba_id.from_string("001000");
  //   mea_gap_normal[2].fcch_freq_id.freq_id.from_string("01");
  //   mea_gap_normal[2].meas_norm_ratio = meas_gap_cfg_normal_s::meas_norm_ratio_opts::dB9;

  //   mea_gap_normal[3].beam_index   = 4;
  //   mea_gap_normal[3].frame_offset = 4;
  //   mea_gap_normal[3].fcch_ba_id.ba_id.from_string("001001");
  //   mea_gap_normal[3].fcch_freq_id.freq_id.from_string("01");
  //   mea_gap_normal[3].meas_norm_ratio = meas_gap_cfg_normal_s::meas_norm_ratio_opts::dB9;

  //    rssi_normal_s& rssi_nor = recofg_si.rrc_con_recfg_r1.meas_cfg.s_mea_sure.set_rssi_normal();
  //   rssi_nor.rssi_normal    = 32;
  //    }
  //   report_cfg_s& rep_cfg                = recofg_si.rrc_con_recfg_r1.meas_cfg.report_cfg;
  //   rep_cfg.thr_hold_present             = true;
  //   rep_cfg.report_geo_grap_info_present = true;
  //   rep_cfg.thr_hold.off_set             = 1;
  //   rep_cfg.thr_hold.hysteresis          = 1;
  //   rep_cfg.thr_hold.time_to_trigger     = thr_hold_s::time_to_trigger_opts::ms0;
  //   rep_cfg.thr_hold.filter_coe_ent      = thr_hold_s::filter_coe_ent_opts::fc0;
  //   rep_cfg.report_geo_grap_info         = report_cfg_s::report_geo_grap_info_opts::True;

  // //  if (wx_area_mode==0)
  // //    {
  // //   rssi_normal_s& rssi_nor = recofg_si.rrc_con_recfg_r1.meas_cfg.s_mea_sure.set_rssi_normal();
  // //   rssi_nor.rssi_normal    = 32;
  // //    }

  // //   if (wx_area_mode==1)
  // //    {
  // //   rssi_dl_freq_spread_s& rssi_nor_sp = recofg_si.rrc_con_recfg_r1.meas_cfg.s_mea_sure.set_rssi_dl_freq_spread();
  // //   rssi_nor_sp.rssi_dl_freq_spread   = 32;
  // //    }
  //   // RadioResourceConfigDedicated的消息
  //   recofg_si.rrc_con_recfg_r1.redio_resour_cfg_dedi_present = true;
  //   redio_resour_cfg_dedi_s& rr_cfg_ded                      = recofg_si.rrc_con_recfg_r1.redio_resour_cfg_dedi;
  //   rr_cfg_ded.srb_to_add_present                            = true;
  //   rr_cfg_ded.drb_to_add_mod_list_present                   = true;
  //   rr_cfg_ded.peri_bsr_timer_present                        = true;
  //   rr_cfg_ded.phy_chan_list_cfg_present                     = true;
  //   rr_cfg_ded.secu_cfg_present                              = true;

  //   def_cfg_s& defau_cfg = rr_cfg_ded.srb_to_add.def_cfg;

  //   rr_cfg_ded.drb_to_add_mod_list.resize(1);
  //   drb_to_add_modi_s& drb_add   = rr_cfg_ded.drb_to_add_mod_list[0];
  //   drb_add.sdap_cfg_present     = true;
  //   drb_add.pdcp_cfg_present     = true;
  //   drb_add.rlc_cfg_present      = true;
  //   drb_add.log_chan_cfg_present = true;
  //   // sdap
  //   drb_add.sdap_cfg.map_qos_flows_to_add_present = true;
  //   drb_add.sdap_cfg.pdu_sess_id.pdu_ses_id       = pdu_ses_id;
  //   drb_add.sdap_cfg.sdap_header_dl               = sdap_cfg_s::sdap_header_dl_opts::absent;
  //   drb_add.sdap_cfg.sdap_header_ul               = sdap_cfg_s::sdap_header_ul_opts::absent;
  //   drb_add.sdap_cfg.default_drb                  = false;
  //   drb_add.sdap_cfg.map_qos_flows_to_add.resize(1);
  //   drb_add.sdap_cfg.map_qos_flows_to_add[0].qfi = qfi_nas;
  // if(drb_add.sdap_cfg.pdu_sess_id.pdu_ses_id==1)
  // {
  //   drb_add.drb_id = 3;
  // }
  // if(drb_add.sdap_cfg.pdu_sess_id.pdu_ses_id==2)
  // {
  //   drb_add.drb_id = 4;
  //   if (parent->cfg.ttcn_test_enble && parent->cfg.ttcn_rrc_enble && parent->rrc_adp->udp_.TC_712_reconfig_update) {
  //      while (true) {
  //          if (parent->rrc_adp->udp_.rrc_receive_info.size() != 0) {
  //              std::cout << " TTCN Reconf Update Info " << std::endl;
  //              srsran::unique_byte_buffer_t reconf_update =
  //                 srsran::make_byte_buffer();
  //              reconf_update->init();
  //              parent->rrc_adp->udp_.rrc_receive_info.try_pop(reconf_update);
  //              parent->get_general_interface(&reconf_update);
  //              ams.ul_am_rlc.t_poll_retran= static_cast<asn1::rrc::ul_am_rlcc_s::t_poll_retran_opts::options>((reconf_update->msg[8]));
  //              ams.ul_am_rlc.poll_pdu = static_cast<asn1::rrc::ul_am_rlcc_s::poll_pdu_opts::options>((reconf_update->msg[9]));
  //              ams.ul_am_rlc.poll_byte = static_cast<asn1::rrc::ul_am_rlcc_s::poll_byte_opts::options>((reconf_update->msg[10]));
  //              ams.ul_am_rlc.max_retx_thres_hold = static_cast<asn1::rrc::ul_am_rlcc_s::max_retx_thres_hold_opts::options>((reconf_update->msg[11]));
  //              ams.dl_am_rlc.t_reord = static_cast<asn1::rrc::dl_am_rlcc_s::t_reord_opts::options>((reconf_update->msg[12]));
  //              ams.dl_am_rlc.t_status_proh= static_cast<asn1::rrc::dl_am_rlcc_s::t_status_proh_opts::options>((reconf_update->msg[13]));

  //              std::cout << "t_poll_retran:" << ams.ul_am_rlc.t_poll_retran << std::endl;
  //              std::cout << "poll_pdu:" << ams.ul_am_rlc.poll_pdu << std::endl;
  //              std::cout << "poll_byte:" << ams.ul_am_rlc.poll_byte << std::endl;
  //              std::cout << "max_retx_thres_hold:" << ams.ul_am_rlc.max_retx_thres_hold << std::endl;
  //              std::cout << "t_reord:" << ams.dl_am_rlc.t_reord << std::endl;
  //              std::cout << "t_status_proh:" << ams.dl_am_rlc.t_status_proh << std::endl;

  //              //TTCN-3 config parameters
  //              break;
  //          }
  //      }
  //  }

  //   // LogicalChannelConfig
  //   drb_add.log_chan_cfg.ul_spec_para_present = true;
  //   ul_spec_para_s& ul_sp_para       = drb_add.log_chan_cfg.ul_spec_para;
  //   ul_sp_para.log_chan_goup_present = true;
  //   ul_sp_para.priority              = 1;
  //   ul_sp_para.prio_bit_rate         = ul_spec_para_s::prio_bit_rate_opts::kBps16;
  //   ul_sp_para.buck_size_dura        = ul_spec_para_s::buck_size_dura_opts::ms60;
  //   ul_sp_para.log_chan_goup         = 1;

  //   // peri_bsr_timer_e_
  //   rr_cfg_ded.peri_bsr_timer = redio_resour_cfg_dedi_s::peri_bsr_timer_opts::rf5;

  //   // PhysicalChannel-Config------------------------------------------配置物理参数
  //   if (wx_area_mode==0)
  //   {
  //   rr_cfg_ded.phy_chan_list_cfg.resize(1);
  //   phy_chan_cfg_s& phy_chan  = rr_cfg_ded.phy_chan_list_cfg[0];
  //   phy_chan.band_id_present  = true;
  //   phy_chan.freq_id_present  = true;
  //   phy_chan.slot_ass_present = true;
  //   phy_chan.s_rnti.srnti.from_string("000101");
  //   phy_chan.chan_type = phy_chan_cfg_s::chan_type_opts::pDCH11;
  //   phy_chan.band_id.ba_id.from_string("001110");
  //   phy_chan.freq_id.freq_id.from_string("01");

  //   if(phy_chan.chan_type==1||phy_chan.chan_type==3||phy_chan.chan_type==5)
  //   {
  //     phy_chan.slot_ass.from_string("00100");//psch5-1/pdch1-1/psch1-1=00100
  //   }
  //   if(phy_chan.chan_type==2||phy_chan.chan_type==4||phy_chan.chan_type==6)
  //   {
  //     phy_chan.slot_ass.from_string("11000");//psch1-2/psch_5-2/pdch1-2=11000
  //   }

  //   phy_chan.direc_t = phy_chan_cfg_s::direct_opts::biDirection;

  //   if(phy_chan.chan_type!=1&&phy_chan.chan_type!=2)
  //   {
  //    phy_chan.sche_type_present=true;
  //     phy_chan.sche_type= phy_chan_cfg_s::sche_type_opts::Static;
  //   }
  //   }

  //   if (wx_area_mode==1)
  //   {
  //     rr_cfg_ded.phy_chan_list_cfg.resize(2);

  //     rr_cfg_ded.phy_chan_list_cfg[0].band_id_present  = true;
  //     rr_cfg_ded.phy_chan_list_cfg[0].freq_id_present  = true;
  //     rr_cfg_ded.phy_chan_list_cfg[0].slot_ass_present = true;
  //     rr_cfg_ded.phy_chan_list_cfg[0].s_rnti.srnti.from_string("000110");
  //     rr_cfg_ded.phy_chan_list_cfg[0].chan_type = phy_chan_cfg_s::chan_type_opts::pSCH11;
  //     rr_cfg_ded.phy_chan_list_cfg[0].band_id.ba_id.from_string("000111");
  //     rr_cfg_ded.phy_chan_list_cfg[0].freq_id.freq_id.from_string("01");
  //      if(rr_cfg_ded.phy_chan_list_cfg[0].chan_type==1||rr_cfg_ded.phy_chan_list_cfg[0].chan_type==3||rr_cfg_ded.phy_chan_list_cfg[0].chan_type==5)
  //   {
  //     rr_cfg_ded.phy_chan_list_cfg[0].slot_ass.from_string("00100");//psch5-1/pdch1-1/psch1-1=00100
  //   }
  //   if(rr_cfg_ded.phy_chan_list_cfg[0].chan_type==2||rr_cfg_ded.phy_chan_list_cfg[0].chan_type==4||rr_cfg_ded.phy_chan_list_cfg[0].chan_type==6)
  //   {
  //     rr_cfg_ded.phy_chan_list_cfg[0].slot_ass.from_string("11000");//psch1-2/psch_5-2/pdch1-2=11000
  //   }

  //   rr_cfg_ded.phy_chan_list_cfg[0].direc_t = phy_chan_cfg_s::direct_opts::ulDirection;

  //   if(rr_cfg_ded.phy_chan_list_cfg[0].chan_type!=1&&rr_cfg_ded.phy_chan_list_cfg[0].chan_type!=2)
  //   {
  //    rr_cfg_ded.phy_chan_list_cfg[0].sche_type_present=true;
  //     rr_cfg_ded.phy_chan_list_cfg[0].sche_type= phy_chan_cfg_s::sche_type_opts::Static;
  //   }

  //     rr_cfg_ded.phy_chan_list_cfg[1].band_id_present  = true;
  //     rr_cfg_ded.phy_chan_list_cfg[1].freq_id_present  = true;
  //     rr_cfg_ded.phy_chan_list_cfg[1].slot_ass_present = true;
  //     rr_cfg_ded.phy_chan_list_cfg[1].s_rnti.srnti.from_string("000110");
  //     rr_cfg_ded.phy_chan_list_cfg[1].chan_type = phy_chan_cfg_s::chan_type_opts::dSPDTCH1;
  //     rr_cfg_ded.phy_chan_list_cfg[1].band_id.ba_id.from_string("000110");
  //     rr_cfg_ded.phy_chan_list_cfg[1].freq_id.freq_id.from_string("01");
  //     rr_cfg_ded.phy_chan_list_cfg[1].slot_ass.from_string("01000");  //dSPDTCHT need to modify to 01000
  //     rr_cfg_ded.phy_chan_list_cfg[1].pdtch_code_present=true;
  //     rr_cfg_ded.phy_chan_list_cfg[1].pdtch_code.pdt_phy_code=21;
  //     rr_cfg_ded.phy_chan_list_cfg[1].direc_t = phy_chan_cfg_s::direct_opts::dlDirection;

  //       if (rr_cfg_ded.phy_chan_list_cfg[1].chan_type== 3 || rr_cfg_ded.phy_chan_list_cfg[1].chan_type ==4|| rr_cfg_ded.phy_chan_list_cfg[1].chan_type ==5|| rr_cfg_ded.phy_chan_list_cfg[1].chan_type ==6)
  //       {
  //           rr_cfg_ded.phy_chan_list_cfg[1].sche_type_present= true;
  //           rr_cfg_ded.phy_chan_list_cfg[1].sche_type = phy_chan_cfg_s::sche_type_opts::Static;
  //       }
  //   }

  //   // securityConfig
  //   secu_cfg_s& sec_cfg = rr_cfg_ded.secu_cfg;
  //   sec_cfg.sec_alg_cfg.integ_prot_alg_present = true;
  //   sec_cfg.sec_alg_cfg.coph_alg               = sec_alg_cfg_s::ciph_alg_opts::nea0;
  //   sec_cfg.sec_alg_cfg.intef_prot_alg         = sec_alg_cfg_s::intef_prot_alg_opts::nia0;

  //   dl_dcch_msg_s            dl_dcch_msg;
  //   rrc_conn_recfg_s&        rrc_conn_recfg = dl_dcch_msg.msg.set_c1().set_rrc_conn_recfg();
  //   rrc_conn_recfg_r8_ies_s& recfg_r8       = rrc_conn_recfg.crit_exts.set_c1().set_rrc_conn_recfg_r8();
  //   // Fill RR Config Ded
  //   if (s_apply_reconf_updates(recfg_r8, recfg_r1, cur_ue_cfg, parent->cfg, ue_cell_list, bearer_list, ue_capabilities)) {
  //     parent->logger.error("Generating ConnectionReconfiguration. Aborting...");
  //     return;
  //   }
  //   /*if (s_apply_reconf_updates(recfg_r1, cur_ue_cfg, parent->cfg, bearer_list)) {
  //     parent->logger.error("Generating ConnectionReconfiguration. Aborting...");
  //     return;
  //   }*/
  //   if (not(recfg_r1.redio_resour_cfg_com_present || recfg_r1.meas_cfg_present || recfg_r1.mobility_contro_present ||
  //           recfg_r1.dedi_info_nas_n_present || recfg_r1.redio_resour_cfg_com_present ||
  //           recfg_r1.security_cfg_ho_present)) {
  //     return;
  //   }

  //   // Fill in NAS PDU - Only for RRC Connection Reconfiguration during E-RAB Release Command
  //    if (nas_pdu.size() > 0 && !recfg_r1.dedi_info_nas_n_present) {
  //     recfg_r1.dedi_info_nas_n_present = true;
  //     // Add NAS PDU
  //     printf("wcb----test11111\n");

  //     recfg_r1.dedi_info_nas_n.resize(1);
  //     recfg_r1.dedi_info_nas_n[0].dedi_info_nas_n1.resize(nas_pdu.size());
  //     memcpy(recfg_r1.dedi_info_nas_n[0].dedi_info_nas_n1.data(), nas_pdu.data(), nas_pdu.size());

  //     // for (uint32_t idx = 0; idx < recfg_r1.redio_resour_cfg_dedi.drb_to_rel_list.size(); idx++) {
  //     //   recfg_r1.dedi_info_nas_n[idx].dedi_info_nas_n1.resize(nas_pdu.size());
  //     //   memcpy(recfg_r1.dedi_info_nas_n[idx].dedi_info_nas_n1.data(), nas_pdu.data(), nas_pdu.size());
  //     // }
  //   }

  //   s_apply_rlc_rb_updates(drb_add);
  //   s_apply_pdcp_srb_updates();
  //   s_apply_pdcp_drb_updates(drb_add);
  //   // Reuse same PDU
  //   if (pdu != nullptr) {
  //     pdu->clear();
  //   }
  //   std::string octet_str;
  //   s_sends_dl_dcch(&ss_dl_dcch_msg, std::move(pdu), &octet_str);

  //   state = RRC_STATE_WAIT_FOR_CON_RECONF_COMPLETE;
  // }
  //-------------------------------------------------------------------------------------------
  // 6_27 ho_re
  void rrc::ue::wx_Switch_Dataing_or_NBusness()
  {
    std::cout << "wx_Switch_Dataing_or_NBusness()" << std::endl;
    Switch_Info Temp_Switch_Info;

    Temp_Switch_Info.Beam_Header = 0xEE;
    Temp_Switch_Info.Beam_Type = Target_Beam;
    Temp_Switch_Info.Beam_InfoType = Switch_Request;
    Temp_Switch_Info.Switch_Type = Data_Service_Switch;
    Temp_Switch_Info.Voice_Type = N_Voice;

    srsran::unique_byte_buffer_t pdu = srsran::make_byte_buffer();
    pdu->N_bytes = 5;
    pdu->msg[0] = Temp_Switch_Info.Beam_Header;
    pdu->msg[1] = Temp_Switch_Info.Beam_Type;
    pdu->msg[2] = Temp_Switch_Info.Beam_InfoType;
    pdu->msg[3] = Temp_Switch_Info.Switch_Type;
    pdu->msg[4] = Temp_Switch_Info.Voice_Type;
    // if(parent->rrc_adp->udp_.TC_722_ho_success){
    //   printf("parent->rrc_adp->udp_.TC_722_ho_success\n");
    //   pdu->N_bytes=6;
    //   pdu->msg[5]=0x22;//TC_722;
    // }
    parent->rrc_adp->udp_.source_beam_trans_queue.try_push(std::move(pdu));
  }

  void rrc::ue::wx_Switch_Voicing()
  {
    std::cout << "wx_Switch_Voicing()" << std::endl;
    Switch_Info Temp_Switch_Info;

    Temp_Switch_Info.Beam_Header = 0xEE;
    Temp_Switch_Info.Beam_Type = Target_Beam;
    Temp_Switch_Info.Beam_InfoType = Switch_Request;
    Temp_Switch_Info.Switch_Type = Voice_Service_Switch;
    Temp_Switch_Info.Voice_Type = static_cast<int>(Voice_Type_record);
    std::cout << "Temp_Switch_Info.Voice_Type=" << Temp_Switch_Info.Voice_Type << std::endl;

    srsran::unique_byte_buffer_t pdu = srsran::make_byte_buffer();
    pdu->N_bytes = 5;
    pdu->msg[0] = Temp_Switch_Info.Beam_Header;
    pdu->msg[1] = Temp_Switch_Info.Beam_Type;
    pdu->msg[2] = Temp_Switch_Info.Beam_InfoType;
    pdu->msg[3] = Temp_Switch_Info.Switch_Type;
    pdu->msg[4] = Temp_Switch_Info.Voice_Type;

    parent->rrc_adp->udp_.source_beam_trans_queue.try_push(std::move(pdu));
  }

  /**
   * Data更新目标波束参数配置,建立DRB承载及DRB对应映射表
   */
  void rrc::ue::wx_Update_TargetBeam_Data_Config()
  {
    //********************************************************************************//
    if (wx_area_mode == 0)
    {
      if (!wx_SourceBeam_read_reconfig(parent->cfg.reconf_data_config))
      {
        std::cout << " read data reconf normal failure When switching " << std::endl;
      }
    }
    else
    {
      if (!wx_SourceBeam_read_reconfig(parent->cfg.reconf_data_ds_config))
      {
        std::cout << " read data reconf sp failure When switching " << std::endl;
      }
    }

    RRC_mcs_Info.is_data_service_switching = true;
    std::cout << "[DOUBLE BEAM][TARGET BEAM][DATA]" << std::endl;
    std::cout << "[DOUBLE BEAM][TARGET BEAM][DATA][MCS]:" << RRC_mcs_Info.MCS << std::endl;
    std::cout << "[DOUBLE BEAM][TARGET BEAM][DATA][IS_DATA_SERVICE_SWITCHING]:" << RRC_mcs_Info.is_data_service_switching << std::endl;
    //*******************************************************************************//
    if (parent->rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 628)
    {
      parent->rrc_adp->udp_.TC_628_RECONFIG_SUCCESS = true;
    }
    if (parent->rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 722)
    {
      parent->rrc_adp->udp_.TC_722_ho_success_last = true;
    }
    is_ho_complete = true;
    srsran::const_byte_span nas_pdu = {};
    update_scells();
    s_dl_dcch_msg_s s_dl_dcch_msg;
    if (wx_area_mode == 0)
    {
      s_dl_dcch_msg = parent->cfg.recfg_wx;
      std::cout << "parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].band_id.ba_id.to_number();=" << parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].band_id.ba_id.to_number() << std::endl;
      std::cout << "parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].freq_id.freq_id.to_number();=" << parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].freq_id.freq_id.to_number() << std::endl;
      std::cout << "parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.to_string();=" << parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.to_string() << std::endl;
      std::cout << "parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.to_number();=" << parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.to_number() << std::endl;
    }

    else
    {
      s_dl_dcch_msg = parent->cfg.recfg_norm_kuopin;

      std::cout << "parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].band_id.ba_id.to_number();=" << parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].band_id.ba_id.to_number() << std::endl;
      std::cout << "parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].freq_id.freq_id.to_number();=" << parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].freq_id.freq_id.to_number() << std::endl;
      std::cout << "parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.to_string();=" << parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.to_string() << std::endl;
      std::cout << "parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.to_number();=" << parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.to_number() << std::endl;

      std::cout << "parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].band_id.ba_id.to_number();=" << parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].band_id.ba_id.to_number() << std::endl;
      std::cout << "parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].freq_id.freq_id.to_number();=" << parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].freq_id.freq_id.to_number() << std::endl;
      std::cout << "parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type.to_string();=" << parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type.to_string() << std::endl;
      std::cout << "parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].slot_ass.to_number();=" << parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].slot_ass.to_number() << std::endl;
    }

    // //zhj819
    if (parent->rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 628)
    {
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].rlc_cfg.set_um_bi_dir();
      um_bi_dir_s_ &ums = s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].rlc_cfg.set_um_bi_dir();
      ums.dl_um_rlc.t_reord = dl_um_rlcc_s::t_reord_opts::ms480;
    }

    dl_dcch_msg_s dl_dcch_msg;
    rrc_conn_recfg_s &rrc_conn_recfg = dl_dcch_msg.msg.set_c1().set_rrc_conn_recfg();
    rrc_conn_recfg_r8_ies_s &recfg_r8 = rrc_conn_recfg.crit_exts.set_c1().set_rrc_conn_recfg_r8();

    std::cout << "[Target BEAM][Update][Slot]:" << s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.to_number() << std::endl;

    if (s_apply_reconf_updates(recfg_r8, s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1, cur_ue_cfg, parent->cfg, ue_cell_list, bearer_list, ue_capabilities))
    {
      parent->logger.error("Generating ConnectionReconfiguration. Aborting...");
      return;
    }

    if (not(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_com_present || s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.meas_cfg_present || s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.mobility_contro_present || s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi_present ||
            s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.dedi_info_nas_n_present || s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_com_present ||
            s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.security_cfg_ho_present))
    {

      return;
    }

    // Fill in NAS PDU - Only for RRC Connection Reconfiguration during E-RAB Release Command
    if (nas_pdu.size() > 0 && !s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.dedi_info_nas_n_present)
    {
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.dedi_info_nas_n_present = true;
      // Add NAS PDU
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.dedi_info_nas_n.resize(1);
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.dedi_info_nas_n[0].dedi_info_nas_n1.resize(nas_pdu.size());
      memcpy(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.dedi_info_nas_n[0].dedi_info_nas_n1.data(), nas_pdu.data(), nas_pdu.size());
    }

    std::cout << "RRC PID = " << parent->rrc_adp->udp_.pid << std::endl;
    s_apply_rlc_rb_updates(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0]);
    s_apply_pdcp_srb_updates();
    s_apply_pdcp_drb_updates(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0]);
    //**********************************************************************************************//
    // configMap DRB_configMap;

    if (s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.rlc_am_present == true)
    {
      DRB_configMap.ul_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
      if (wx_area_mode == 0)
      {
        std::cout << " wx_area_mode 0 " << std::endl;
        DRB_configMap.dl_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
      }
      else
      {
        std::cout << " wx_area_mode 1 " << std::endl;
        DRB_configMap.dl_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type.value);
      }
      DRB_configMap.voicetype = N_Voice;
      DRB_configMap.rlcType = rlc_am;
      DRB_configMap.dataType = Data;
      parent->mac->addlcidMap(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].drb_id, DRB_configMap);
    }
    else if (s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.rlc_um_present == true)
    {
      DRB_configMap.ul_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
      if (wx_area_mode == 0)
      {
        std::cout << " wx_area_mode 0 " << std::endl;
        DRB_configMap.dl_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
      }
      else
      {
        std::cout << " wx_area_mode 1 " << std::endl;
        DRB_configMap.dl_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type.value);
      }
      DRB_configMap.voicetype = N_Voice;
      DRB_configMap.rlcType = rlc_um;
      DRB_configMap.dataType = Data;
      parent->mac->addlcidMap(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].drb_id, DRB_configMap);
    }

    // update Control Map Info
    SRB_configMap.ul_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
    if (wx_area_mode == 0)
    {
      SRB_configMap.dl_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
    }
    else
    {
      SRB_configMap.dl_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type.value);
    }
    std::cout << " SRB_configMap.ul_Type = " << SRB_configMap.ul_Type << std::endl;
    std::cout << " SRB_configMap.dl_Type = " << SRB_configMap.dl_Type << std::endl;
    SRB_configMap.dataType = Control;  // control
    SRB_configMap.rlcType = rlc_am;    // AM
    SRB_configMap.voicetype = N_Voice; // NO Voice
                                       //**********************************************************************************************//

    ul_allocate reconf_allo;
    dl_allocate reconf_sp_allo;
    reconf_sp_allo.bandID = s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].band_id.ba_id.to_number();
    reconf_sp_allo.freq = s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].freq_id.freq_id.to_number();
    reconf_sp_allo.Type = (ChanType_t)(int)s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type;
    /*uint8_t solt*/
    reconf_sp_allo.solt = s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.to_number();
    // int n = 0;
    // while ((solt & 0x01) == 0)
    // {
    //   solt = solt >> 1;
    //   n++;
    // }

    parent->mac->addlcidMap(1, SRB_configMap);
    std::cout << "AAAAAAAAAAAA" << std::endl;
    parent->mac->reconf_phy(reconf_sp_allo, reconf_allo, RRC_mcs_Info, parent->frame_off);
  }

  /**
   * Voice:更新目标波束参数配置,建立DRB承载及DRB对应映射表
   */
  void rrc::ue::wx_Update_TargetBeam_Voice_Config()
  {
    RRC_mcs_Info.is_data_service_switching = false;
    is_ho_complete = true;
    RRC_mcs_Info.MCS = 0; // Voice only 1/2QPSK
    srsran::const_byte_span nas_pdu = {};
    update_scells();

    s_dl_dcch_msg_s s_dl_dcch_msg;
    if(wx_area_mode==0){
      s_dl_dcch_msg = parent->cfg.recfg_wx_voice;
      std::cout << "wx_Update_TargetBeam_Voice_Config BAND_ID=" << s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].band_id.ba_id.to_number() << std::endl;
    }else{
      s_dl_dcch_msg = parent->cfg.recfg_wx_kuopin;
      std::cout << "wx_Update_TargetBeam_Voice_Config BAND_ID=" << s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].band_id.ba_id.to_number() << std::endl;
    }
    std::cout << "wx_Update_TargetBeam_Voice_Config DRB.ID=" << (int)s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].drb_id << std::endl;
  

    dl_dcch_msg_s dl_dcch_msg;
    rrc_conn_recfg_s &rrc_conn_recfg = dl_dcch_msg.msg.set_c1().set_rrc_conn_recfg();
    rrc_conn_recfg_r8_ies_s &recfg_r8 = rrc_conn_recfg.crit_exts.set_c1().set_rrc_conn_recfg_r8();

    if (parent->Switch_Voice == 0)
    {
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].voice_type = asn1::rrc::phy_chan_cfg_s::voice_type_opts::kbps2point4;
    }
    else if (parent->Switch_Voice == 1)
    {
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].voice_type = asn1::rrc::phy_chan_cfg_s::voice_type_opts::kbps4point8;
    }
    else if (parent->Switch_Voice == 2)
    {
      ue_category = 14;
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].voice_type = asn1::rrc::phy_chan_cfg_s::voice_type_opts::bps800;
    }
    if (ue_category == 14)
    {
      uint16_t temp_ue_cap14_band_id = s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].band_id.ba_id.to_number();
      uint16_t temp_ue_cap14_freq_id = s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].freq_id.freq_id.to_number();
      uint16_t temp_ue_cap14_slot = s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.to_number();
      std::cout << "temp_ue_cap14_band_id=" << temp_ue_cap14_band_id << std::endl;
      std::cout << "temp_ue_cap14_freq_id=" << temp_ue_cap14_freq_id << std::endl;
      std::cout << "temp_ue_cap14_slot=" << temp_ue_cap14_slot << std::endl;

      uint16_t temp_ue_cap14_slot_ = temp_ue_cap14_slot;
      int n = 0;
      while ((temp_ue_cap14_slot_ & 0x01) == 0)
      {
        temp_ue_cap14_slot_ = temp_ue_cap14_slot_ >> 1;
        n++;
      }
      std::cout << "n=" << n << std::endl;

      parent->mac->setUecategory(ue_category, temp_ue_cap14_band_id, temp_ue_cap14_freq_id, n, temp_ue_cap14_slot);
    }

    //************************   READ UE14 RELATIVE FILE  ****************************************//
    // {
    //   std::cout << "xx ue_cap14_band_id = " << ue_cap14_band_id << std::endl;
    //   std::cout << "xx ue_cap14_freq_id = " << ue_cap14_freq_id << std::endl;
    //   std::cout << "xx ue_cap14_slot = " << ue_cap14_slot << std::endl;
    //   s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = phy_chan_cfg_s::chan_type_opts::pDCH11; // change channel
    //   s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].band_id.ba_id.from_number(ue_cap14_band_id);
    //   s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].freq_id.freq_id.from_number(ue_cap14_freq_id);
    //   s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.from_number(ue_cap14_slot);
    // }

    //********************************************************************************************//

    std::cout << "[Target BEAM][Update][Slot]:" << s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.to_number() << std::endl;

    if (s_apply_reconf_updates(recfg_r8, s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1, cur_ue_cfg, parent->cfg, ue_cell_list, bearer_list, ue_capabilities))
    {
      parent->logger.error("Generating ConnectionReconfiguration. Aborting...");
      return;
    }

    if (not(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_com_present || s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.meas_cfg_present || s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.mobility_contro_present || s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi_present ||
            s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.dedi_info_nas_n_present || s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_com_present ||
            s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.security_cfg_ho_present))
    {
      return;
    }

    // Fill in NAS PDU - Only for RRC Connection Reconfiguration during E-RAB Release Command
    if (nas_pdu.size() > 0 && !s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.dedi_info_nas_n_present)
    {
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.dedi_info_nas_n_present = true;
      // Add NAS PDU
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.dedi_info_nas_n.resize(1);
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.dedi_info_nas_n[0].dedi_info_nas_n1.resize(nas_pdu.size());
      memcpy(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.dedi_info_nas_n[0].dedi_info_nas_n1.data(), nas_pdu.data(), nas_pdu.size());
    }

    s_apply_rlc_rb_updates(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0]);
    s_apply_pdcp_srb_updates();
    s_apply_pdcp_drb_updates(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0]);
    //**********************************************************************************************//
    // configMap DRB_configMap;

    DRB_configMap.ul_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
    if (wx_area_mode == 0)
    {
      DRB_configMap.dl_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
    }
    else if (wx_area_mode == 1)
    {
      DRB_configMap.dl_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type.value);
    }
    std::cout << "[RRC][VOICE][DRB_CONF][UL_TYPE]:" << DRB_configMap.ul_Type << std::endl;
    std::cout << "[RRC][VOICE][DRB_CONF][DL_TYPE]:" << DRB_configMap.dl_Type << std::endl;
    DRB_configMap.voicetype = static_cast<VoiceType>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].voice_type.value);
    std::cout << "[RRC][VOICE][DRB_CONF][VOICE_TYPE]:" << DRB_configMap.voicetype << std::endl;
    DRB_configMap.rlcType = rlc_tm;
    DRB_configMap.dataType = Voice;
    parent->mac->addlcidMap(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].drb_id, DRB_configMap);

    // update Control Map Info
    SRB_configMap.ul_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
    if (wx_area_mode == 0)
    {
      SRB_configMap.dl_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
    }
    else
    {
      SRB_configMap.dl_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type.value);
    }
    std::cout << "[RRC][VOICE][SRB_CONF][UL_TYPE]:" << SRB_configMap.ul_Type << std::endl;
    std::cout << "[RRC][VOICE][SRB_CONF][DL_TYPE]:" << SRB_configMap.dl_Type << std::endl;
    SRB_configMap.dataType = Control;  // control
    SRB_configMap.rlcType = rlc_am;    // AM
    SRB_configMap.voicetype = N_Voice; // NO Voice
                                       //**********************************************************************************************//

    ul_allocate reconf_allo;
    dl_allocate reconf_sp_allo;
    reconf_sp_allo.bandID = s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].band_id.ba_id.to_number();
    reconf_sp_allo.freq = s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].freq_id.freq_id.to_number();
    reconf_sp_allo.Type = (ChanType_t)(int)s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type;
    // uint8_t solt
    reconf_sp_allo.solt = s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.to_number();
    // int n = 0;
    // while ((solt & 0x01) == 0)
    // {
    //   solt = solt >> 1;
    //   n++;
    // }
    parent->mac->addlcidMap(1, SRB_configMap);
    parent->mac->reconf_phy(reconf_sp_allo, reconf_allo, RRC_mcs_Info, parent->frame_off);
  }

  /**
   * 源波束发送重配
   */
  void rrc::ue::wx_Sourcebeam_send_RRC_Switch_Data_Reconfig(srsran::unique_byte_buffer_t pdu)
  {
    std::cout << "wx_Sourcebeam_send_RRC_Switch_Data_Reconfig" << std::endl;
    update_scells();
    s_dl_dcch_msg_s ss_dl_dcch_msg;
    rrc_con_recfg_s &recofg_si = ss_dl_dcch_msg.msg.set_rrc_con_recfg();
    recofg_si.rrc_tran_iden = 1;
    rrc_con_recfg_r1_ies_s &recfg_r1 = recofg_si.rrc_con_recfg_r1;
    // std::string target_datareconfig_filename = "build/access/beam2/recfg_wx.conf";

    // MeasConfig的消息
    recofg_si.rrc_con_recfg_r1.meas_cfg_present = true;
    meas_cofg_s &mea_cfg = recofg_si.rrc_con_recfg_r1.meas_cfg;
    mea_cfg.meas_gap_cfg_list_present = true;
    mea_cfg.s_mea_sure_present = true;

    if (wx_area_mode == 0)
    {
      meas_gap_cfg_list_normal_s &mea_gap_normal = mea_cfg.meas_gap_cfg_list.set_meas_gap_cfg_list_normal();
      mea_gap_normal.resize(7);

      mea_gap_normal[0].beam_index = 1;
      mea_gap_normal[0].frame_offset = 1;
      mea_gap_normal[0].fcch_ba_id.ba_id.from_string("010000");
      mea_gap_normal[0].fcch_freq_id.freq_id.from_string("10");
      mea_gap_normal[0].mib_Re_Fra_Num = 20;
      mea_gap_normal[0].meas_norm_ratio = meas_gap_cfg_normal_s::meas_norm_ratio_opts::dB5;

      mea_gap_normal[1].beam_index = 2;
      mea_gap_normal[1].frame_offset = 2;
      mea_gap_normal[1].fcch_ba_id.ba_id.from_string("001100");
      mea_gap_normal[1].fcch_freq_id.freq_id.from_string("10");
      mea_gap_normal[1].mib_Re_Fra_Num = 20;
      mea_gap_normal[1].meas_norm_ratio = meas_gap_cfg_normal_s::meas_norm_ratio_opts::dB5;

      mea_gap_normal[2].beam_index = 3;
      mea_gap_normal[2].frame_offset = 3;
      mea_gap_normal[2].fcch_ba_id.ba_id.from_string("000100");
      mea_gap_normal[2].fcch_freq_id.freq_id.from_string("10");
      mea_gap_normal[2].mib_Re_Fra_Num = 20;
      mea_gap_normal[2].meas_norm_ratio = meas_gap_cfg_normal_s::meas_norm_ratio_opts::dB5;

      mea_gap_normal[3].beam_index = 4;
      mea_gap_normal[3].frame_offset = 4;
      mea_gap_normal[3].fcch_ba_id.ba_id.from_string("101000");
      mea_gap_normal[3].fcch_freq_id.freq_id.from_string("10");
      mea_gap_normal[3].mib_Re_Fra_Num = 20;
      mea_gap_normal[3].meas_norm_ratio = meas_gap_cfg_normal_s::meas_norm_ratio_opts::dB5;

      mea_gap_normal[4].beam_index = 5;
      mea_gap_normal[4].frame_offset = 5;
      mea_gap_normal[4].fcch_ba_id.ba_id.from_string("101100");
      mea_gap_normal[4].fcch_freq_id.freq_id.from_string("10");
      mea_gap_normal[4].mib_Re_Fra_Num = 20;
      mea_gap_normal[4].meas_norm_ratio = meas_gap_cfg_normal_s::meas_norm_ratio_opts::dB5;

      mea_gap_normal[5].beam_index = 6;
      mea_gap_normal[5].frame_offset = 6;
      mea_gap_normal[5].fcch_ba_id.ba_id.from_string("110001");
      mea_gap_normal[5].fcch_freq_id.freq_id.from_string("10");
      mea_gap_normal[5].mib_Re_Fra_Num = 20;
      mea_gap_normal[5].meas_norm_ratio = meas_gap_cfg_normal_s::meas_norm_ratio_opts::dB5;

      mea_gap_normal[6].beam_index = 7;
      mea_gap_normal[6].frame_offset = 7;
      mea_gap_normal[6].fcch_ba_id.ba_id.from_string("110110");
      mea_gap_normal[6].fcch_freq_id.freq_id.from_string("10");
      mea_gap_normal[6].mib_Re_Fra_Num = 20;
      mea_gap_normal[6].meas_norm_ratio = meas_gap_cfg_normal_s::meas_norm_ratio_opts::dB5;

      rssi_normal_s &rssi_nor = recofg_si.rrc_con_recfg_r1.meas_cfg.s_mea_sure.set_rssi_normal();
      rssi_nor.rssi_normal = 2;
    }
    else
    {
      meas_gap_cfg_list_dl_freq_spread_s &meas_gap_cfg_list_dl_freq = mea_cfg.meas_gap_cfg_list.set_meas_gap_cfg_list_dl_freq_spread_s();
      meas_gap_cfg_list_dl_freq.resize(5);
      meas_gap_cfg_list_dl_freq[0].beam_id = 1;
      meas_gap_cfg_list_dl_freq[0].sec_syn_group_id = 2;

      meas_gap_cfg_list_dl_freq[1].beam_id = 2;
      meas_gap_cfg_list_dl_freq[1].sec_syn_group_id = 3;

      meas_gap_cfg_list_dl_freq[2].beam_id = 3;
      meas_gap_cfg_list_dl_freq[2].sec_syn_group_id = 4;

      meas_gap_cfg_list_dl_freq[3].beam_id = 4;
      meas_gap_cfg_list_dl_freq[3].sec_syn_group_id = 5;

      meas_gap_cfg_list_dl_freq[4].beam_id = 5;
      meas_gap_cfg_list_dl_freq[4].sec_syn_group_id = 6;

      rssi_dl_freq_spread_s &rssi_nor_sp = recofg_si.rrc_con_recfg_r1.meas_cfg.s_mea_sure.set_rssi_dl_freq_spread();
      rssi_nor_sp.rssi_dl_freq_spread = 32;
    }

    report_cfg_s &rep_cfg = recofg_si.rrc_con_recfg_r1.meas_cfg.report_cfg;

    // MobilityControlInfo
    recfg_r1.mobility_contro_present = true;
    mobility_contro_s &mob_cont = recfg_r1.mobility_contro;
    mob_cont.target_beam_index = 1;
    mob_cont.target_beam_id.beam_id.from_string("00000000000010");
    mob_cont.target_bcch_band_id.ba_id.from_string("001000");
    mob_cont.t304 = mobility_contro_s::t304_opts::ms8000;
    if (parent->rrc_adp->udp_.RACH_Handover)
    {
        std::cout<<"is rach handover 2"<<std::endl;
      
      mob_cont.rach_indi_tor_present=true;
      mob_cont.rach_indi_tor = mobility_contro_s::rach_indi_tor_opts::True;
    }

    // RadioResourceConfigCommon
    recfg_r1.redio_resour_cfg_com_present = true;
    redio_resour_cfg_co_s &rr_cfg_com = recfg_r1.redio_resour_cfg_co;

    rr_cfg_com.rach_cfg_com_present = true;
    rr_cfg_com.agch_cfg_com_present = true;
    rr_cfg_com.dis_to_beam_center.from_string("11");
    rr_cfg_com.rach_cfg_com.freqBitmap_FrameFull_present = true;
    if (parent->rrc_adp->udp_.RACH_Handover)
    {
      rr_cfg_com.rach_cfg_com.band_id.ba_id.from_string("001010");//old_1128 (001000)
      //parent->rrc_adp->udp_.RACH_Handover=false;
    }else{
      rr_cfg_com.rach_cfg_com.band_id.ba_id.from_string("011100");//old_1128 (001000)
    }
    
    rr_cfg_com.rach_cfg_com.freq_bit_map.from_string("0100");
    rr_cfg_com.rach_cfg_com.rach_frame_ass.from_string("1111");
    rr_cfg_com.rach_cfg_com.rach_slot_ass = rach_cfg_com_n_s::rach_slot_ass_opts::halfFrame0;
    rr_cfg_com.rach_cfg_com.ra_res_wi_si = rach_cfg_com_n_s::ra_res_wi_si_opts::rf15;
    rr_cfg_com.rach_cfg_com.freqBitmap_FrameFull.from_string("0010");

    rr_cfg_com.agch_cfg_com_n.band_id_present = true;
    rr_cfg_com.agch_cfg_com_n.band_id.ba_id.from_string("001000");
    rr_cfg_com.agch_cfg_com_n.freq_id_present = true;
    rr_cfg_com.agch_cfg_com_n.freq_id.freq_id.from_string("01");
    rr_cfg_com.agch_cfg_com_n.agch_frame_ass.from_string("1111");
    rr_cfg_com.agch_cfg_com_n.agch_slot_styart_present = true;
    rr_cfg_com.agch_cfg_com_n.agch_slot_start = agch_cfg_com_n_s::agch_alot_start_opts::slot3;


    rr_cfg_com.power_control_cfg_com.pmbch_tx_power.from_string("1101111");

    if (wx_area_mode == 1)
    {
      rr_cfg_com.frame_offset_present = true;
      rr_cfg_com.mib_Re_Fra_num_present = true;
      rr_cfg_com.frame_offset.from_string("000011");
      rr_cfg_com.mib_Re_Fra_num.from_string("000000");
    }

    rr_cfg_com.naviInfo_band_id.ba_id.from_string("000100");
    rr_cfg_com.naviInfo_fre_id.freq_id.from_string("01");
    rr_cfg_com.naviInfo_slot_ass = redio_resour_cfg_co_s::naviInfo_slot_ass_opts::slot3;

    // RadioResourceConfigDedicated
    recfg_r1.redio_resour_cfg_dedi_present = true;
    redio_resour_cfg_dedi_s &rr_cfg_ded = recfg_r1.redio_resour_cfg_dedi;

    rr_cfg_ded.srb_to_add_present = true;
    def_cfg_s &defau_cfg = rr_cfg_ded.srb_to_add.def_cfg;

    rr_cfg_ded.peri_bsr_timer_present = true;
    rr_cfg_ded.peri_bsr_timer = redio_resour_cfg_dedi_s::peri_bsr_timer_opts::infinity;

    rr_cfg_ded.phy_chan_list_cfg_present = true;

    // phy_chan.voice_type_present = true;

    if (wx_area_mode == 0)
    {
      rr_cfg_ded.phy_chan_list_cfg.resize(1);
      phy_chan_cfg_s &phy_chan = rr_cfg_ded.phy_chan_list_cfg[0];
      phy_chan.band_id_present = true;
      phy_chan.freq_id_present = true;
      phy_chan.slot_ass_present = true;
      phy_chan.sche_type_present = true;
      std::cout << "parent->cfg.pid=" << parent->cfg.pid << std::endl;

      std::cout << " parent->rrcSwitchParameters.Beam_Type=" << (int)parent->rrcSwitchParameters.Beam_Type << std::endl;
      if (parent->cfg.pid == 1 && parent->rrcSwitchParameters.Beam_Type == Source_Beam)
      {
        // std::string target_datareconfig_filename = "/opt/AccessIot/access/beam2/recfg_wx.conf";
        std::cout << " A parent->cfg.reconf_data_config = " << parent->cfg.reconf_data_config << std::endl;
        std::string target_datareconfig_filename = parent->cfg.reconf_data_config;
        // Find "beam1" in the string and replace it with "beam2"
        size_t pos = target_datareconfig_filename.find("beam1");
        if (pos != std::string::npos)
        {
          target_datareconfig_filename.replace(pos, 5, "beam2");
        }
        std::cout << "target_datareconfig_filename:" << target_datareconfig_filename << std::endl;
        if (!wx_SourceBeam_to_read_TargetBeam_reconfig_normal(
                target_datareconfig_filename, recofg_si.rrc_con_recfg_r1))
        {
          std::cout << "wx_SourceBeam_to_read_TargetBeam_reconfig_normal"
                    << std::endl;
        }
      }
      else if (parent->cfg.pid == 2 && parent->rrcSwitchParameters.Beam_Type == Source_Beam)
      {
        // std::string target_datareconfig_filename = "/opt/AccessIot/access/beam1/recfg_wx.conf";
        std::cout << " B parent->cfg.reconf_data_config = " << parent->cfg.reconf_data_config << std::endl;
        std::string target_datareconfig_filename = parent->cfg.reconf_data_config;
        // Find "beam2" in the string and replace it with "beam1"
        size_t pos = target_datareconfig_filename.find("beam2");
        if (pos != std::string::npos)
        {
          target_datareconfig_filename.replace(pos, 5, "beam1");
        }
        std::cout << "target_datareconfig_filename:" << target_datareconfig_filename << std::endl;
        if (!wx_SourceBeam_to_read_TargetBeam_reconfig_normal(
                target_datareconfig_filename, recofg_si.rrc_con_recfg_r1))
        {
          std::cout << "wx_SourceBeam_to_read_TargetBeam_reconfig_normal"
                    << std::endl;
        }
      }
      else
      {
        std::cout << "zzzzzzzzzzzzzzzzz" << std::endl;
        std::cerr << "常规时数据 不具备读取文件条件！" << std::endl;
      }

      phy_chan.s_rnti.srnti.from_string("000101");
      std::cout << " phy_chan.sche_type:" << phy_chan.sche_type.to_string()
                << std::endl;
      std::cout << " phy_chan.band_id.ba_id:"
                << phy_chan.band_id.ba_id.to_string() << std::endl;
      phy_chan.direc_t = phy_chan_cfg_s::direct_opts::biDirection;
    }
    else
    {
      rr_cfg_ded.phy_chan_list_cfg.resize(2);
      phy_chan_cfg_s &phy_chan = rr_cfg_ded.phy_chan_list_cfg[0];
      phy_chan.band_id_present = true;
      phy_chan.freq_id_present = true;
      phy_chan.slot_ass_present = true;
      phy_chan.sche_type_present = true;

      phy_chan_cfg_s &ds_phy_chan = rr_cfg_ded.phy_chan_list_cfg[1];
      ds_phy_chan.band_id_present = true;
      ds_phy_chan.freq_id_present = true;
      ds_phy_chan.slot_ass_present = true;
      ds_phy_chan.pdtch_code_present = true;

      if (parent->cfg.pid == 1 && parent->rrcSwitchParameters.Beam_Type == Source_Beam)
      {
        // std::string target_datareconfig_filename =
        //     "/opt/AccessIot/access/beam2/recfg_norm_kuopin.conf";
        std::cout << " A parent->cfg.reconf_data_ds_config = " << parent->cfg.reconf_data_ds_config << std::endl;
        std::string target_datareconfig_filename = parent->cfg.reconf_data_ds_config;
        // Find "beam1" in the string and replace it with "beam2"
        size_t pos = target_datareconfig_filename.find("beam1");
        if (pos != std::string::npos)
        {
          target_datareconfig_filename.replace(pos, 5, "beam2");
        }
        std::cout << "target_datareconfig_filename:" << target_datareconfig_filename << std::endl;
        if (!wx_SourceBeam_to_read_TargetBeam_reconfig_sp(
                target_datareconfig_filename, recofg_si.rrc_con_recfg_r1))
        {
          std::cout << "wx_SourceBeam_to_read_TargetBeam_reconfig_sp"
                    << std::endl;
        }
      }
      else if (parent->cfg.pid == 2 && parent->rrcSwitchParameters.Beam_Type == Source_Beam)
      {
        // std::string target_datareconfig_filename =
        //     "/opt/AccessIot/access/beam1/recfg_norm_kuopin.conf";
        std::cout << " B parent->cfg.reconf_data_ds_config = " << parent->cfg.reconf_data_ds_config << std::endl;
        std::string target_datareconfig_filename = parent->cfg.reconf_data_ds_config;
        // Find "beam2" in the string and replace it with "beam1"
        size_t pos = target_datareconfig_filename.find("beam2");
        if (pos != std::string::npos)
        {
          target_datareconfig_filename.replace(pos, 5, "beam1");
        }
        if (!wx_SourceBeam_to_read_TargetBeam_reconfig_sp(
                target_datareconfig_filename, recofg_si.rrc_con_recfg_r1))
        {
          std::cout << "wx_SourceBeam_to_read_TargetBeam_reconfig_sp"
                    << std::endl;
        }
      }
      else
      {
        std::cerr << "扩频时数据 不具备读取文件条件！" << std::endl;
      }

      phy_chan.s_rnti.srnti.from_string("000110");
      ds_phy_chan.s_rnti.srnti.from_string("000110");
      std::cout << " phy_chan.sche_type:" << phy_chan.sche_type.to_string()
                << std::endl;
      std::cout << " phy_chan.band_id.ba_id:"
                << phy_chan.band_id.ba_id.to_string() << std::endl;
      std::cout << " ds_phy_chan.band_id.ba_id:"
                << ds_phy_chan.band_id.ba_id.to_string() << std::endl;
    }

    // SecurityConfigHO
    recfg_r1.security_cfg_ho_present = true;
    security_cofg_ho_s &security_ho = recfg_r1.security_cfg_ho;
    security_ho.sec_alg_cfg_present = true;
    security_ho.sec_alg_cfg.coph_alg = sec_alg_cfg_s::ciph_alg_opts::nea0;

    dl_dcch_msg_s dl_dcch_msg;
    rrc_conn_recfg_s &rrc_conn_recfg = dl_dcch_msg.msg.set_c1().set_rrc_conn_recfg();
    rrc_conn_recfg_r8_ies_s &recfg_r8 = rrc_conn_recfg.crit_exts.set_c1().set_rrc_conn_recfg_r8();
    // Fill RR Config Ded
    if (s_apply_reconf_updates(recfg_r8, recfg_r1, cur_ue_cfg, parent->cfg, ue_cell_list, bearer_list, ue_capabilities))
    {
      parent->logger.error("Generating ConnectionReconfiguration. Aborting...");
      std::cout << "test wcb 146746738" << std::endl;
      return;
    }

    if (not(recfg_r1.redio_resour_cfg_com_present || recfg_r1.meas_cfg_present || recfg_r1.mobility_contro_present || recfg_r1.redio_resour_cfg_dedi_present ||
            recfg_r1.dedi_info_nas_n_present || recfg_r1.security_cfg_ho_present))
    {
      return;
    }

    s_apply_pdcp_srb_updates();

    pdu = srsran::make_byte_buffer();
    pdu->init();
    asn1::bit_ref bref(pdu->msg, pdu->get_tailroom());
    if (ss_dl_dcch_msg.pack(bref) == asn1::SRSASN_ERROR_ENCODE_FAIL)
    {
      parent->logger.error("Failed to encode DL-DCCH-Msg for rnti=0x%x", rnti);
      std::cout << "Failed to encode DL-DCCH-Msg" << std::endl;
      return;
    }
    pdu->N_bytes = (uint32_t)bref.distance_bytes();

    for (uint32_t i = 0; i < pdu->N_bytes; i++)
    {
      printf("0x%x\n", *(pdu->msg + i));
    }

    // 下传

    parent->pdcp->write_sdu(rnti, srb_to_lcid(lte_srb::srb1), std::move(pdu));
    // std::cout << "write_sdu(rnti, srb_to_lcid(lte_srb" << std::endl;
    // s_apply_pdcp_drb_updates(recofg_si.rrc_con_recfg_r1.redio_resour_cfg_dedi);
    auto start = std::chrono::high_resolution_clock::now();
    while (true)
    {
      auto end = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
      if (duration.count() > 3)
      {
        break;
      }
    }
  }

  /**
   * 源波束发送重配
   */
  void rrc::ue::wx_Sourcebeam_send_RRC_Switch_Voice_Reconfig(srsran::unique_byte_buffer_t pdu)
  {
    std::cout << "wx_Sourcebeam_send_RRC_Switch_Voice_Reconfig" << std::endl;
    update_scells();
    s_dl_dcch_msg_s ss_dl_dcch_msg;
    rrc_con_recfg_s &recofg_si = ss_dl_dcch_msg.msg.set_rrc_con_recfg();
    recofg_si.rrc_tran_iden = 1;
    rrc_con_recfg_r1_ies_s &recfg_r1 = recofg_si.rrc_con_recfg_r1;

    // MeasConfig的消息
    recofg_si.rrc_con_recfg_r1.meas_cfg_present = true;
    meas_cofg_s &mea_cfg = recofg_si.rrc_con_recfg_r1.meas_cfg;
    mea_cfg.meas_gap_cfg_list_present = true;
    mea_cfg.s_mea_sure_present = true;

    meas_gap_cfg_list_normal_s &mea_gap_normal = mea_cfg.meas_gap_cfg_list.set_meas_gap_cfg_list_normal();
    mea_gap_normal.resize(7);

    mea_gap_normal[0].beam_index = 1;
    mea_gap_normal[0].frame_offset = 1;
    mea_gap_normal[0].fcch_ba_id.ba_id.from_string("010000");
    mea_gap_normal[0].fcch_freq_id.freq_id.from_string("10");
    mea_gap_normal[0].mib_Re_Fra_Num = 20;
    mea_gap_normal[0].meas_norm_ratio = meas_gap_cfg_normal_s::meas_norm_ratio_opts::dB5;

    mea_gap_normal[1].beam_index = 2;
    mea_gap_normal[1].frame_offset = 2;
    mea_gap_normal[1].fcch_ba_id.ba_id.from_string("001100");
    mea_gap_normal[1].fcch_freq_id.freq_id.from_string("10");
    mea_gap_normal[1].mib_Re_Fra_Num = 20;
    mea_gap_normal[1].meas_norm_ratio = meas_gap_cfg_normal_s::meas_norm_ratio_opts::dB5;

    mea_gap_normal[2].beam_index = 3;
    mea_gap_normal[2].frame_offset = 3;
    mea_gap_normal[2].fcch_ba_id.ba_id.from_string("000100");
    mea_gap_normal[2].fcch_freq_id.freq_id.from_string("10");
    mea_gap_normal[2].mib_Re_Fra_Num = 20;
    mea_gap_normal[2].meas_norm_ratio = meas_gap_cfg_normal_s::meas_norm_ratio_opts::dB5;

    mea_gap_normal[3].beam_index = 4;
    mea_gap_normal[3].frame_offset = 4;
    mea_gap_normal[3].fcch_ba_id.ba_id.from_string("101000");
    mea_gap_normal[3].fcch_freq_id.freq_id.from_string("10");
    mea_gap_normal[3].mib_Re_Fra_Num = 20;
    mea_gap_normal[3].meas_norm_ratio = meas_gap_cfg_normal_s::meas_norm_ratio_opts::dB5;

    mea_gap_normal[4].beam_index = 5;
    mea_gap_normal[4].frame_offset = 5;
    mea_gap_normal[4].fcch_ba_id.ba_id.from_string("101100");
    mea_gap_normal[4].fcch_freq_id.freq_id.from_string("10");
    mea_gap_normal[4].mib_Re_Fra_Num = 20;
    mea_gap_normal[4].meas_norm_ratio = meas_gap_cfg_normal_s::meas_norm_ratio_opts::dB5;

    mea_gap_normal[5].beam_index = 6;
    mea_gap_normal[5].frame_offset = 6;
    mea_gap_normal[5].fcch_ba_id.ba_id.from_string("110001");
    mea_gap_normal[5].fcch_freq_id.freq_id.from_string("10");
    mea_gap_normal[5].mib_Re_Fra_Num = 20;
    mea_gap_normal[5].meas_norm_ratio = meas_gap_cfg_normal_s::meas_norm_ratio_opts::dB5;

    mea_gap_normal[6].beam_index = 7;
    mea_gap_normal[6].frame_offset = 7;
    mea_gap_normal[6].fcch_ba_id.ba_id.from_string("110110");
    mea_gap_normal[6].fcch_freq_id.freq_id.from_string("10");
    mea_gap_normal[6].mib_Re_Fra_Num = 20;
    mea_gap_normal[6].meas_norm_ratio = meas_gap_cfg_normal_s::meas_norm_ratio_opts::dB5;

    rssi_normal_s &rssi_nor = recofg_si.rrc_con_recfg_r1.meas_cfg.s_mea_sure.set_rssi_normal();
    rssi_nor.rssi_normal = 2;
    report_cfg_s &rep_cfg = recofg_si.rrc_con_recfg_r1.meas_cfg.report_cfg;

    // MobilityControlInfo
    recfg_r1.mobility_contro_present = true;
    mobility_contro_s &mob_cont = recfg_r1.mobility_contro;
    mob_cont.target_beam_index = 1;
    mob_cont.target_beam_id.beam_id.from_string("00000000000010");
    mob_cont.target_bcch_band_id.ba_id.from_string("001000");
    mob_cont.t304 = mobility_contro_s::t304_opts::ms8000;
    if (parent->rrc_adp->udp_.RACH_Handover)
    {
      //parent->rrc_adp->udp_.RACH_Handover=false;
      mob_cont.rach_indi_tor_present=true;
      mob_cont.rach_indi_tor = mobility_contro_s::rach_indi_tor_opts::True;
    }

    // RadioResourceConfigCommon
    recfg_r1.redio_resour_cfg_com_present = true;
    redio_resour_cfg_co_s &rr_cfg_com = recfg_r1.redio_resour_cfg_co;

    rr_cfg_com.rach_cfg_com_present = true;
    rr_cfg_com.agch_cfg_com_present = true;

    rr_cfg_com.dis_to_beam_center.from_string("00");

    rr_cfg_com.rach_cfg_com.freqBitmap_FrameFull_present = true;
    rr_cfg_com.rach_cfg_com.band_id.ba_id.from_string("001000");
    rr_cfg_com.rach_cfg_com.freq_bit_map.from_string("1000");
    rr_cfg_com.rach_cfg_com.rach_frame_ass.from_string("1111");
    rr_cfg_com.rach_cfg_com.rach_slot_ass = rach_cfg_com_n_s::rach_slot_ass_opts::halfFrame0;
    rr_cfg_com.rach_cfg_com.ra_res_wi_si = rach_cfg_com_n_s::ra_res_wi_si_opts::rf15;
    rr_cfg_com.rach_cfg_com.freqBitmap_FrameFull.from_string("0010");

    rr_cfg_com.agch_cfg_com_n.band_id_present = true;
    rr_cfg_com.agch_cfg_com_n.band_id.ba_id.from_string("001000");
    rr_cfg_com.agch_cfg_com_n.freq_id_present = true;
    rr_cfg_com.agch_cfg_com_n.freq_id.freq_id.from_string("01");
    rr_cfg_com.agch_cfg_com_n.agch_frame_ass.from_string("1111");
    rr_cfg_com.agch_cfg_com_n.agch_slot_styart_present = true;
    rr_cfg_com.agch_cfg_com_n.agch_slot_start = agch_cfg_com_n_s::agch_alot_start_opts::slot3;

    rr_cfg_com.power_control_cfg_com.pmbch_tx_power.from_string("1101111");

    rr_cfg_com.naviInfo_band_id.ba_id.from_string("000100");
    rr_cfg_com.naviInfo_fre_id.freq_id.from_string("01");
    rr_cfg_com.naviInfo_slot_ass = redio_resour_cfg_co_s::naviInfo_slot_ass_opts::slot3;

    // RadioResourceConfigDedicated
    recfg_r1.redio_resour_cfg_dedi_present = true;
    redio_resour_cfg_dedi_s &rr_cfg_ded = recfg_r1.redio_resour_cfg_dedi;

    rr_cfg_ded.srb_to_add_present = true;
    def_cfg_s &defau_cfg = rr_cfg_ded.srb_to_add.def_cfg;

    rr_cfg_ded.peri_bsr_timer_present = true;
    rr_cfg_ded.peri_bsr_timer = redio_resour_cfg_dedi_s::peri_bsr_timer_opts::infinity;

    rr_cfg_ded.phy_chan_list_cfg_present = true;

    // Voice Rate ???

    if (wx_area_mode == 0)
    {
      rr_cfg_ded.phy_chan_list_cfg.resize(1);
      rr_cfg_ded.phy_chan_list_cfg[0].band_id_present = true;
      rr_cfg_ded.phy_chan_list_cfg[0].freq_id_present = true;
      rr_cfg_ded.phy_chan_list_cfg[0].slot_ass_present = true;
      rr_cfg_ded.phy_chan_list_cfg[0].sche_type_present = false;
      rr_cfg_ded.phy_chan_list_cfg[0].voice_type_present = true;

      rr_cfg_ded.phy_chan_list_cfg[0].s_rnti.srnti.from_string("000101");

      rr_cfg_ded.phy_chan_list_cfg[0].voice_type = Voice_Type_record;
      std::cout << "phy_chan.voice_type="
                << rr_cfg_ded.phy_chan_list_cfg[0].voice_type.to_string()
                << std::endl;
      if (parent->cfg.pid == 1 && parent->rrcSwitchParameters.Beam_Type == Source_Beam)
      {
        // std::string target_datareconfig_filename =
        //     "/opt/AccessIot/access/beam2/recfg_voice.conf";
        std::cout << " A parent->cfg.reconf_voice_config = " << parent->cfg.reconf_voice_config << std::endl;
        std::string target_datareconfig_filename = parent->cfg.reconf_voice_config;
        // Find "beam1" in the string and replace it with "beam2"
        size_t pos = target_datareconfig_filename.find("beam1");
        if (pos != std::string::npos)
        {
          target_datareconfig_filename.replace(pos, 5, "beam2");
        }
        if (!wx_SourceBeam_to_read_TargetBeam_reconfig_Voice(
                target_datareconfig_filename, recofg_si.rrc_con_recfg_r1))
        {
          std::cout
              << "READ wx_SourceBeam_to_read_TargetBeam_reconfig_Voice Failure!!!"
              << std::endl;
        }
      }
      else if (parent->cfg.pid == 2 && parent->rrcSwitchParameters.Beam_Type == Source_Beam)
      {
        // std::string target_datareconfig_filename =
        //     "/opt/AccessIot/access/beam1/recfg_voice.conf";
        std::cout << " B parent->cfg.reconf_voice_config = " << parent->cfg.reconf_voice_config << std::endl;
        std::string target_datareconfig_filename = parent->cfg.reconf_voice_config;
        // Find "beam2" in the string and replace it with "beam1"
        size_t pos = target_datareconfig_filename.find("beam2");
        if (pos != std::string::npos)
        {
          target_datareconfig_filename.replace(pos, 5, "beam1");
        }
        if (!wx_SourceBeam_to_read_TargetBeam_reconfig_Voice(
                target_datareconfig_filename, recofg_si.rrc_con_recfg_r1))
        {
          std::cout
              << "READ wx_SourceBeam_to_read_TargetBeam_reconfig_Voice Failure!!!"
              << std::endl;
        }
      }
      else
      {
        std::cerr << "常规时语音 不具备读取文件的条件！" << std::endl;
      }
      if (ue_category == 14)
      {
        rr_cfg_ded.phy_chan_list_cfg[0].voice_type =
            phy_chan_cfg_s::voice_type_opts::bps800;
      }

      rr_cfg_ded.phy_chan_list_cfg[0].direc_t =
          phy_chan_cfg_s::direct_opts::biDirection;
    }
    else if (wx_area_mode == 1)
    {
      rr_cfg_ded.phy_chan_list_cfg.resize(2);
      rr_cfg_ded.phy_chan_list_cfg[0].band_id_present = true;
      rr_cfg_ded.phy_chan_list_cfg[0].freq_id_present = true;
      rr_cfg_ded.phy_chan_list_cfg[0].slot_ass_present = true;
      rr_cfg_ded.phy_chan_list_cfg[0].sche_type_present = false;
      rr_cfg_ded.phy_chan_list_cfg[0].voice_type_present = true;

      rr_cfg_ded.phy_chan_list_cfg[0].s_rnti.srnti.from_string("000101");

      rr_cfg_ded.phy_chan_list_cfg[0].voice_type = Voice_Type_record;

      rr_cfg_ded.phy_chan_list_cfg[1].band_id_present = true;
      rr_cfg_ded.phy_chan_list_cfg[1].freq_id_present = true;
      rr_cfg_ded.phy_chan_list_cfg[1].slot_ass_present = true;
      rr_cfg_ded.phy_chan_list_cfg[1].sche_type_present = false;
      rr_cfg_ded.phy_chan_list_cfg[1].voice_type_present = true;
      rr_cfg_ded.phy_chan_list_cfg[1].pdtch_code_present = true;

      rr_cfg_ded.phy_chan_list_cfg[1].s_rnti.srnti.from_string("000101");

      if (parent->cfg.pid == 1 && parent->rrcSwitchParameters.Beam_Type == Source_Beam)
      {
        // std::string target_datareconfig_filename =
        //     "/opt/AccessIot/access/beam2/recfg_voice_kuopin.conf";
        std::cout << " A  parent->cfg.reconf_voice_ds_config = " << parent->cfg.reconf_voice_ds_config << std::endl;
        std::string target_datareconfig_filename =
            parent->cfg.reconf_voice_ds_config;
        // Find "beam1" in the string and replace it with "beam2"
        size_t pos = target_datareconfig_filename.find("beam1");
        if (pos != std::string::npos)
        {
          target_datareconfig_filename.replace(pos, 5, "beam2");
        }
        if (!wx_SourceBeam_to_read_TargetBeam_reconfig_Voice_Ds(
                target_datareconfig_filename, recofg_si.rrc_con_recfg_r1))
        {
          std::cout << "READ wx_SourceBeam_to_read_TargetBeam_reconfig_Voice_Ds "
                       "Failure!!!"
                    << std::endl;
        }
      }
      else if (parent->cfg.pid == 2 && parent->rrcSwitchParameters.Beam_Type == Source_Beam)
      {
        // std::string target_datareconfig_filename =
        //     "/opt/AccessIot/access/beam1/recfg_voice_kuopin.conf";
        std::cout << " B  parent->cfg.reconf_voice_ds_config = " << parent->cfg.reconf_voice_ds_config << std::endl;
        std::string target_datareconfig_filename =
            parent->cfg.reconf_voice_ds_config;
        // Find "beam2" in the string and replace it with "beam1"
        size_t pos = target_datareconfig_filename.find("beam2");
        if (pos != std::string::npos)
        {
          target_datareconfig_filename.replace(pos, 5, "beam1");
        }
        if (!wx_SourceBeam_to_read_TargetBeam_reconfig_Voice_Ds(
                target_datareconfig_filename, recofg_si.rrc_con_recfg_r1))
        {
          std::cout << "READ wx_SourceBeam_to_read_TargetBeam_reconfig_Voice_Ds "
                       "Failure!!!"
                    << std::endl;
        }
      }
      else
      {
        std::cerr << "扩频时语音 不具备读取文件的条件！" << std::endl;
      }
    }
    // if(!wx_SourceBeam_to_read_TargetBeam_reconfig_Voice()){

    // }

    // if (ue_category == 14)
    // {
    //   std::cout << "xx ue_cap14_band_id = " << ue_cap14_band_id << std::endl;
    //   std::cout << "xx ue_cap14_freq_id = " << ue_cap14_freq_id << std::endl;
    //   std::cout << "xx ue_cap14_slot = " << ue_cap14_slot << std::endl;
    //   phy_chan.chan_type = phy_chan_cfg_s::chan_type_opts::pDCH11; // change channel
    //   phy_chan.band_id.ba_id.from_number(ue_cap14_band_id);
    //   phy_chan.freq_id.freq_id.from_number(ue_cap14_freq_id);
    //   phy_chan.slot_ass.from_number(ue_cap14_slot);
    //   phy_chan.voice_type = phy_chan_cfg_s::voice_type_opts::bps800;
    // }
    // else
    // {
    //   phy_chan.chan_type = parent->cfg.recfg_wx_voice.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type;
    //   // phy_chan.band_id.ba_id = parent->cfg.recfg_wx_voice.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].band_id.ba_id;
    //   phy_chan.band_id.ba_id.from_number(50);
    //   phy_chan.freq_id.freq_id = parent->cfg.recfg_wx_voice.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].freq_id.freq_id;
    //   phy_chan.slot_ass = parent->cfg.recfg_wx_voice.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass;
    // }

    // phy_chan.sche_type = parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].sche_type;

    // SecurityConfigHO
    recfg_r1.security_cfg_ho_present = true;
    security_cofg_ho_s &security_ho = recfg_r1.security_cfg_ho;
    security_ho.sec_alg_cfg_present = true;
    security_ho.sec_alg_cfg.coph_alg = sec_alg_cfg_s::ciph_alg_opts::nea0;

    dl_dcch_msg_s dl_dcch_msg;
    rrc_conn_recfg_s &rrc_conn_recfg = dl_dcch_msg.msg.set_c1().set_rrc_conn_recfg();
    rrc_conn_recfg_r8_ies_s &recfg_r8 = rrc_conn_recfg.crit_exts.set_c1().set_rrc_conn_recfg_r8();
    // Fill RR Config Ded
    if (s_apply_reconf_updates(recfg_r8, recfg_r1, cur_ue_cfg, parent->cfg, ue_cell_list, bearer_list, ue_capabilities))
    {
      parent->logger.error("Generating ConnectionReconfiguration. Aborting...");
      std::cout << "test wcb 146746738" << std::endl;
      return;
    }

    if (not(recfg_r1.redio_resour_cfg_com_present || recfg_r1.meas_cfg_present || recfg_r1.mobility_contro_present || recfg_r1.redio_resour_cfg_dedi_present ||
            recfg_r1.dedi_info_nas_n_present || recfg_r1.security_cfg_ho_present))
    {
      return;
    }

    s_apply_pdcp_srb_updates();

    pdu = srsran::make_byte_buffer();
    pdu->init();
    asn1::bit_ref bref(pdu->msg, pdu->get_tailroom());
    if (ss_dl_dcch_msg.pack(bref) == asn1::SRSASN_ERROR_ENCODE_FAIL)
    {
      parent->logger.error("Failed to encode DL-DCCH-Msg for rnti=0x%x", rnti);
      std::cout << "Failed to encode DL-DCCH-Msg" << std::endl;
      return;
    }
    pdu->N_bytes = (uint32_t)bref.distance_bytes();

    for (uint32_t i = 0; i < pdu->N_bytes; i++)
    {
      printf("0x%x\n", *(pdu->msg + i));
    }

    // 下传

    parent->pdcp->write_sdu(rnti, srb_to_lcid(lte_srb::srb1), std::move(pdu));
    // std::cout << "write_sdu(rnti, srb_to_lcid(lte_srb" << std::endl;
    // s_apply_pdcp_drb_updates(recofg_si.rrc_con_recfg_r1.redio_resour_cfg_dedi);
    auto start = std::chrono::high_resolution_clock::now();
    while (true)
    {
      auto end = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
      if (duration.count() > 3)
      {
        break;
      }
    }
  }

  /*卫星接入网的连接重配Connection Reconfiguration*/
  void rrc::ue::send_rrc_con_reconf(
      uint16_t rnti,
      uint8_t qfi_nas,
      uint16_t pdu_ses_id,
      int am_tm_type,
      srsran::unique_byte_buffer_t pdu,
      srsran::const_byte_span nas_pdu)
  {
    std::cout << "@@@@@@@@@@@@@@@@@@@@@@2" << std::endl;
    std::cout << " RECONFIG TYPE = " << am_tm_type << std::endl; // 0:Normal reconf  1:Voice reconf 2:Release reconf
    std::cout << "[RRC][RECONF][1/2][MCS]:" << RRC_mcs_Info.MCS << std::endl;
    std::cout << "[RRC][RECONF][1/2][is_data_service_switching]:" << RRC_mcs_Info.is_data_service_switching << std::endl;
    if (parent->cfg.ttcn_test_enble && parent->cfg.ttcn_rrc_enble && parent->rrc_adp->udp_.TC_711_reconfig)
    {
      while (true)
      {
        // TTCN-3 config parameters
        if (parent->rrc_adp->udp_.rrc_receive_info.size() != 0)
        {
          std::cout << " TTCN reconf Info " << std::endl;
          srsran::unique_byte_buffer_t ttcn_reconf =
              srsran::make_byte_buffer();
          ttcn_reconf->init();
          parent->rrc_adp->udp_.rrc_receive_info.try_pop(ttcn_reconf);
          parent->get_general_interface(&ttcn_reconf);

          break;
        }
      }
    }
    if (parent->cfg.ttcn_test_enble && parent->cfg.ttcn_rrc_enble && parent->rrc_adp->udp_.TC_713_reconfig_DRB)
    {
      TC_713_reconfig_number++;
      while (true)
      {
        // TTCN-3 config parameters
        if (parent->rrc_adp->udp_.rrc_receive_info.size() != 0)
        {
          std::cout << " TTCN reconf Info " << std::endl;
          srsran::unique_byte_buffer_t ttcn_reconf =
              srsran::make_byte_buffer();
          ttcn_reconf->init();
          parent->rrc_adp->udp_.rrc_receive_info.try_pop(ttcn_reconf);
          parent->get_general_interface(&ttcn_reconf);

          break;
        }
      }
    }
    if (parent->cfg.ttcn_test_enble && parent->cfg.ttcn_rrc_enble && parent->rrc_adp->udp_.TC_714_DRB_Release)
    {
      TC_714_reconfig_number++;
      while (true)
      {
        // TTCN-3 config parameters
        if (parent->rrc_adp->udp_.rrc_receive_info.size() != 0)
        {
          std::cout << " TTCN reconf Info " << std::endl;
          srsran::unique_byte_buffer_t ttcn_reconf =
              srsran::make_byte_buffer();
          ttcn_reconf->init();
          parent->rrc_adp->udp_.rrc_receive_info.try_pop(ttcn_reconf);
          parent->get_general_interface(&ttcn_reconf);

          break;
        }
      }
    }
    if (parent->cfg.ttcn_test_enble && parent->cfg.ttcn_rrc_enble && (parent->rrc_adp->udp_.TC_715_reest_reconf || parent->rrc_adp->udp_.TC_715_BEAM2_REEST))
    {
      while (true)
      {
        // TTCN-3 config parameters
        if (parent->rrc_adp->udp_.rrc_receive_info.size() != 0)
        {
          std::cout << " TTCN reconf Info " << std::endl;
          srsran::unique_byte_buffer_t ttcn_reconf =
              srsran::make_byte_buffer();
          ttcn_reconf->init();
          parent->rrc_adp->udp_.rrc_receive_info.try_pop(ttcn_reconf);
          parent->get_general_interface(&ttcn_reconf);

          break;
        }
      }
    }
    update_scells();
    s_dl_dcch_msg_s ss_dl_dcch_msg;
    rrc_con_recfg_s &recofg_si = ss_dl_dcch_msg.msg.set_rrc_con_recfg();
    recofg_si.rrc_tran_iden = 0;
    rrc_con_recfg_r1_ies_s &recfg_r1 = recofg_si.rrc_con_recfg_r1;

    if (am_tm_type == 0)
    {
      // MeasConfig的消息
      recofg_si.rrc_con_recfg_r1.meas_cfg_present = true;
      // recofg_si.rrc_con_recfg_r1.meas_cfg.meas_gap_cfg_list.set_meas_gap_cfg_list_normal();
      meas_cofg_s &mea_cfg = recofg_si.rrc_con_recfg_r1.meas_cfg;
      mea_cfg.meas_gap_cfg_list_present = true;
      mea_cfg.s_mea_sure_present = true;
      if (wx_area_mode == 1)
      {
        meas_gap_cfg_list_dl_freq_spread_s &meas_gap_cfg_list_dl_freq = mea_cfg.meas_gap_cfg_list.set_meas_gap_cfg_list_dl_freq_spread_s();
        meas_gap_cfg_list_dl_freq.resize(5);
        meas_gap_cfg_list_dl_freq[0].beam_id = 2;
        meas_gap_cfg_list_dl_freq[0].sec_syn_group_id = 3;

        meas_gap_cfg_list_dl_freq[1].beam_id = 3;
        meas_gap_cfg_list_dl_freq[1].sec_syn_group_id = 3;

        meas_gap_cfg_list_dl_freq[2].beam_id = 3;
        meas_gap_cfg_list_dl_freq[2].sec_syn_group_id = 4;

        meas_gap_cfg_list_dl_freq[3].beam_id = 4;
        meas_gap_cfg_list_dl_freq[3].sec_syn_group_id = 5;

        meas_gap_cfg_list_dl_freq[4].beam_id = 5;
        meas_gap_cfg_list_dl_freq[4].sec_syn_group_id = 6;

        rssi_dl_freq_spread_s &rssi_nor_sp = recofg_si.rrc_con_recfg_r1.meas_cfg.s_mea_sure.set_rssi_dl_freq_spread();
        rssi_nor_sp.rssi_dl_freq_spread = 32;
      }

      if (wx_area_mode == 0)
      {
        meas_gap_cfg_list_normal_s &mea_gap_normal = mea_cfg.meas_gap_cfg_list.set_meas_gap_cfg_list_normal();
        mea_gap_normal.resize(4);

        mea_gap_normal[0].beam_index = 1;
        mea_gap_normal[0].frame_offset = 1;
        mea_gap_normal[0].fcch_ba_id.ba_id.from_string("000110");
        mea_gap_normal[0].fcch_freq_id.freq_id.from_string("01");
        // add1.2
        mea_gap_normal[0].mib_Re_Fra_Num = 20;
        // 0-3----------------------------------------
        mea_gap_normal[0].meas_norm_ratio = meas_gap_cfg_normal_s::meas_norm_ratio_opts::dB9;

        mea_gap_normal[1].beam_index = 2;
        mea_gap_normal[1].frame_offset = 2;
        mea_gap_normal[1].fcch_ba_id.ba_id.from_string("000111");
        mea_gap_normal[1].fcch_freq_id.freq_id.from_string("01");
        mea_gap_normal[1].mib_Re_Fra_Num = 20;
        mea_gap_normal[1].meas_norm_ratio = meas_gap_cfg_normal_s::meas_norm_ratio_opts::dB9;

        mea_gap_normal[2].beam_index = 3;
        mea_gap_normal[2].frame_offset = 3;
        mea_gap_normal[2].fcch_ba_id.ba_id.from_string("001000");
        mea_gap_normal[2].fcch_freq_id.freq_id.from_string("01");
        mea_gap_normal[2].mib_Re_Fra_Num = 20;
        mea_gap_normal[2].meas_norm_ratio = meas_gap_cfg_normal_s::meas_norm_ratio_opts::dB9;

        mea_gap_normal[3].beam_index = 4;
        mea_gap_normal[3].frame_offset = 4;
        mea_gap_normal[3].fcch_ba_id.ba_id.from_string("001001");
        mea_gap_normal[3].fcch_freq_id.freq_id.from_string("01");
        mea_gap_normal[3].mib_Re_Fra_Num = 20;
        mea_gap_normal[3].meas_norm_ratio = meas_gap_cfg_normal_s::meas_norm_ratio_opts::dB9;

        rssi_normal_s &rssi_nor = recofg_si.rrc_con_recfg_r1.meas_cfg.s_mea_sure.set_rssi_normal();
        rssi_nor.rssi_normal = 32;
      }
      report_cfg_s &rep_cfg = recofg_si.rrc_con_recfg_r1.meas_cfg.report_cfg;
      rep_cfg.thr_hold_present = true;
      rep_cfg.report_geo_grap_info_present = true;
      rep_cfg.thr_hold.off_set = 1;
      rep_cfg.thr_hold.hysteresis = 1;
      rep_cfg.thr_hold.time_to_trigger = thr_hold_s::time_to_trigger_opts::ms0;
      rep_cfg.thr_hold.filter_coe_ent = thr_hold_s::filter_coe_ent_opts::fc0;
      rep_cfg.report_geo_grap_info = report_cfg_s::report_geo_grap_info_opts::True;
      // RadioResourceConfigDedicated的消息
      recofg_si.rrc_con_recfg_r1.redio_resour_cfg_dedi_present = true;
      redio_resour_cfg_dedi_s &rr_cfg_ded = recofg_si.rrc_con_recfg_r1.redio_resour_cfg_dedi;
      rr_cfg_ded.srb_to_add_present = true;
      if (parent->rrc_adp->udp_.TC_714_DRB_Release && TC_714_reconfig_number == 2)
      {
        rr_cfg_ded.drb_to_rel_list_present = true;
      }
      else
      {
        rr_cfg_ded.drb_to_add_mod_list_present = true;
      }
      rr_cfg_ded.peri_bsr_timer_present = true;
      rr_cfg_ded.phy_chan_list_cfg_present = true;
      rr_cfg_ded.secu_cfg_present = true;

      def_cfg_s &defau_cfg = rr_cfg_ded.srb_to_add.def_cfg;
      if (pdu_ses_id == 1 && parent->rrc_adp->udp_.sdap_nhdr_tran)
      {
        rr_cfg_ded.drb_to_add_mod_list.resize(2);
      }
      else if (rr_cfg_ded.drb_to_rel_list_present)
      {
        rr_cfg_ded.drb_to_rel_list.resize(1);
      }
      else
      {
        rr_cfg_ded.drb_to_add_mod_list.resize(1);
      }

      if (rr_cfg_ded.drb_to_add_mod_list.size() > 1)
      {
        std::cout << "rr_cfg_ded.drb_to_add_mod_list.size() > 1" << std::endl;
        drb_to_add_modi_s &drb_add = rr_cfg_ded.drb_to_add_mod_list[0];
        drb_add.sdap_cfg_present = true;
        drb_add.pdcp_cfg_present = true;
        drb_add.rlc_cfg_present = true;
        drb_add.log_chan_cfg_present = true;
        // sdap
        drb_add.sdap_cfg.map_qos_flows_to_add_present = true;
        drb_add.sdap_cfg.pdu_sess_id.pdu_ses_id = pdu_ses_id;
        drb_add.sdap_cfg.sdap_header_dl = sdap_cfg_s::sdap_header_dl_opts::present;
        drb_add.sdap_cfg.sdap_header_ul = sdap_cfg_s::sdap_header_ul_opts::present;
        drb_add.sdap_cfg.default_drb = true;
        drb_add.sdap_cfg.map_qos_flows_to_add.resize(1);
        drb_add.sdap_cfg.map_qos_flows_to_add[0].qfi = 1;

        drb_add.drb_id = 3;

        // pdcp
        drb_add.pdcp_cfg.dis_timer_present = true;
        drb_add.pdcp_cfg.integ_pro_present = true;
        drb_add.pdcp_cfg.dis_timer = pdcp_cofg_s::dis_timer_opts::infinity;
        drb_add.pdcp_cfg.header_com.set_not_used_l();
        drb_add.pdcp_cfg.integ_pro = pdcp_cofg_s::integ_pro_opts::enabled;

        drb_add.pdcp_cfg.rlc_am_present = true;
        drb_add.pdcp_cfg.rlc_am.stat_rep_req = true;

        // drb_add.pdcp_cfg.rlc_um_present = true;

        //  rlc

        drb_add.rlc_cfg.set_am();
        am_s_ &ams = drb_add.rlc_cfg.set_am();
        ams.ul_am_rlc.t_poll_retran = ul_am_rlcc_s::t_poll_retran_opts::ms480;
        ams.ul_am_rlc.poll_pdu = ul_am_rlcc_s::poll_pdu_opts::p32;
        // ams.ul_am_rlc.poll_pdu = ul_am_rlcc_s::poll_pdu_opts::pInfinity;
        ams.ul_am_rlc.poll_byte = ul_am_rlcc_s::poll_byte_opts::kB128;
        // ams.ul_am_rlc.poll_byte = ul_am_rlcc_s::poll_byte_opts::kBInfinity;
        ams.ul_am_rlc.max_retx_thres_hold = ul_am_rlcc_s::max_retx_thres_hold_opts::t4;
        ams.dl_am_rlc.t_reord = dl_am_rlcc_s::t_reord_opts::ms480;
        ams.dl_am_rlc.t_status_proh = dl_am_rlcc_s::t_status_proh_opts::ms420;

        // LogicalChannelConfig
        drb_add.log_chan_cfg.ul_spec_para_present = true;
        ul_spec_para_s &ul_sp_para = drb_add.log_chan_cfg.ul_spec_para;
        ul_sp_para.log_chan_goup_present = true;
        ul_sp_para.priority = 1;
        ul_sp_para.prio_bit_rate = ul_spec_para_s::prio_bit_rate_opts::kBps16;
        ul_sp_para.buck_size_dura = ul_spec_para_s::buck_size_dura_opts::ms60;
        ul_sp_para.log_chan_goup = 1;

        drb_to_add_modi_s &drb_add_2 = rr_cfg_ded.drb_to_add_mod_list[1];
        drb_add_2.sdap_cfg_present = true;
        drb_add_2.pdcp_cfg_present = true;
        drb_add_2.rlc_cfg_present = true;
        drb_add_2.log_chan_cfg_present = true;
        // sdap
        drb_add_2.sdap_cfg.map_qos_flows_to_add_present = true;
        drb_add_2.sdap_cfg.pdu_sess_id.pdu_ses_id = pdu_ses_id;
        drb_add_2.sdap_cfg.sdap_header_dl = sdap_cfg_s::sdap_header_dl_opts::present;
        drb_add_2.sdap_cfg.sdap_header_ul = sdap_cfg_s::sdap_header_ul_opts::present;
        drb_add_2.sdap_cfg.default_drb = false;
        drb_add_2.sdap_cfg.map_qos_flows_to_add.resize(1);
        drb_add_2.sdap_cfg.map_qos_flows_to_add[0].qfi = 2;

        drb_add_2.drb_id = 6;

        // pdcp
        drb_add_2.pdcp_cfg.dis_timer_present = true;
        drb_add_2.pdcp_cfg.integ_pro_present = true;
        drb_add_2.pdcp_cfg.dis_timer = pdcp_cofg_s::dis_timer_opts::infinity;
        drb_add_2.pdcp_cfg.header_com.set_not_used_l();
        drb_add_2.pdcp_cfg.integ_pro = pdcp_cofg_s::integ_pro_opts::enabled;

        drb_add_2.pdcp_cfg.rlc_am_present = true;
        drb_add_2.pdcp_cfg.rlc_am.stat_rep_req = true;

        // drb_add.pdcp_cfg.rlc_um_present = true;

        //  rlc
        drb_add_2.rlc_cfg.set_am();
        am_s_ &ams_2 = drb_add_2.rlc_cfg.set_am();
        ams_2.ul_am_rlc.t_poll_retran = ul_am_rlcc_s::t_poll_retran_opts::ms480;
        ams_2.ul_am_rlc.poll_pdu = ul_am_rlcc_s::poll_pdu_opts::p32;
        ams_2.ul_am_rlc.poll_byte = ul_am_rlcc_s::poll_byte_opts::kB128;
        ams_2.ul_am_rlc.max_retx_thres_hold = ul_am_rlcc_s::max_retx_thres_hold_opts::t4;
        ams_2.dl_am_rlc.t_reord = dl_am_rlcc_s::t_reord_opts::ms480;   // fixed
        ams_2.dl_am_rlc.t_status_proh = dl_am_rlcc_s::t_status_proh_opts::ms420;

        // LogicalChannelConfig
        drb_add_2.log_chan_cfg.ul_spec_para_present = true;
        ul_spec_para_s &ul_sp_para_2 = drb_add_2.log_chan_cfg.ul_spec_para;
        ul_sp_para_2.log_chan_goup_present = true;
        ul_sp_para_2.priority = 1;
        ul_sp_para_2.prio_bit_rate = ul_spec_para_s::prio_bit_rate_opts::kBps16;
        ul_sp_para_2.buck_size_dura = ul_spec_para_s::buck_size_dura_opts::ms60;
        ul_sp_para_2.log_chan_goup = 1;

        // peri_bsr_timer_e_
        rr_cfg_ded.peri_bsr_timer = redio_resour_cfg_dedi_s::peri_bsr_timer_opts::rf5;

        // PhysicalChannel-Config------------------------------------------配置物理参数
        if (wx_area_mode == 0)
        {
          rr_cfg_ded.phy_chan_list_cfg.resize(1);
          if (rr_cfg_ded.phy_chan_list_cfg.size() == 1)
          {
            phy_chan_cfg_s &phy_chan = rr_cfg_ded.phy_chan_list_cfg[0];
            phy_chan.band_id_present = true;
            phy_chan.freq_id_present = true;
            phy_chan.slot_ass_present = true;

            phy_chan.s_rnti.srnti.from_string("000101");
            srnti = phy_chan.s_rnti.srnti.to_number();
            parent->mac->getSrnti(srnti);

            if (ue_category == 14)
            {

              std::cout << "xx ue_cap14_band_id = " << ue_cap14_band_id << std::endl;
              std::cout << "xx ue_cap14_freq_id = " << ue_cap14_freq_id << std::endl;
              std::cout << "xx ue_cap14_slot = " << ue_cap14_slot << std::endl;
              phy_chan.chan_type = phy_chan_cfg_s::chan_type_opts::pDCH11; // change channel
              phy_chan.band_id.ba_id.from_number(ue_cap14_band_id);
              phy_chan.freq_id.freq_id.from_number(ue_cap14_freq_id);
              phy_chan.slot_ass.from_number(ue_cap14_slot);
            }
            else
            {
              phy_chan.chan_type = phy_chan_cfg_s::chan_type_opts::pDCH11; // change channel
              // phy_chan.chan_type = phy_chan_cfg_s::chan_type_opts::pSCH11; // change channel
              phy_chan.band_id.ba_id.from_string("001011");
              phy_chan.freq_id.freq_id.from_string("01");

              if (phy_chan.chan_type == 1 || phy_chan.chan_type == 3 || phy_chan.chan_type == 5)
              {
                phy_chan.slot_ass.from_string("01000"); // psch5-1/pdch1-1/psch1-1=00100
              }
              if (phy_chan.chan_type == 2 || phy_chan.chan_type == 4 || phy_chan.chan_type == 6)
              {
                phy_chan.slot_ass.from_string("11000"); // psch1-2/psch_5-2/pdch1-2=01100
              }
            }

            std::cout << " phy_chan.band_id.ba_id = " << phy_chan.band_id.ba_id.to_string() << std::endl;
            std::cout << " phy_chan.freq_id.freq_id = " << phy_chan.freq_id.freq_id.to_string() << std::endl;
            std::cout << " phy_chan.slot_ass = " << phy_chan.slot_ass.to_string() << std::endl;

            phy_chan.direc_t = phy_chan_cfg_s::direct_opts::biDirection;

            if (phy_chan.chan_type != 1 && phy_chan.chan_type != 2)
            {
              phy_chan.sche_type_present = true;
              phy_chan.sche_type = phy_chan_cfg_s::sche_type_opts::Static;
            }
          }

          if (rr_cfg_ded.phy_chan_list_cfg.size() > 1)
          {
            rr_cfg_ded.phy_chan_list_cfg[0].band_id_present = true;
            rr_cfg_ded.phy_chan_list_cfg[0].freq_id_present = true;
            rr_cfg_ded.phy_chan_list_cfg[0].slot_ass_present = true;
            rr_cfg_ded.phy_chan_list_cfg[0].sche_type_present = true;
            rr_cfg_ded.phy_chan_list_cfg[0].sche_type = phy_chan_cfg_s::sche_type_opts::Static;
            rr_cfg_ded.phy_chan_list_cfg[0].s_rnti.srnti.from_string("000101");

            srnti = rr_cfg_ded.phy_chan_list_cfg[0].s_rnti.srnti.to_number();
            parent->mac->getSrnti(srnti);

            rr_cfg_ded.phy_chan_list_cfg[0].chan_type = phy_chan_cfg_s::chan_type_opts::pSCH51;
            rr_cfg_ded.phy_chan_list_cfg[0].band_id.ba_id.from_string("000110");
            rr_cfg_ded.phy_chan_list_cfg[0].freq_id.freq_id.from_string("00");
            rr_cfg_ded.phy_chan_list_cfg[0].slot_ass.from_string("00001");
            rr_cfg_ded.phy_chan_list_cfg[0].direc_t = phy_chan_cfg_s::direct_opts::biDirection;

            rr_cfg_ded.phy_chan_list_cfg[1].band_id_present = true;
            rr_cfg_ded.phy_chan_list_cfg[1].freq_id_present = true;
            rr_cfg_ded.phy_chan_list_cfg[1].slot_ass_present = true;
            rr_cfg_ded.phy_chan_list_cfg[1].sche_type_present = true;
            rr_cfg_ded.phy_chan_list_cfg[1].sche_type = phy_chan_cfg_s::sche_type_opts::Static;
            rr_cfg_ded.phy_chan_list_cfg[1].s_rnti.srnti.from_string("000101");
            rr_cfg_ded.phy_chan_list_cfg[1].chan_type = phy_chan_cfg_s::chan_type_opts::pSCH51;
            rr_cfg_ded.phy_chan_list_cfg[1].band_id.ba_id.from_string("010000");
            rr_cfg_ded.phy_chan_list_cfg[1].freq_id.freq_id.from_string("00");
            rr_cfg_ded.phy_chan_list_cfg[1].slot_ass.from_string("00001");
            rr_cfg_ded.phy_chan_list_cfg[1].direc_t = phy_chan_cfg_s::direct_opts::biDirection;
          }
        }

        if (wx_area_mode == 1)
        {
          rr_cfg_ded.phy_chan_list_cfg.resize(2);

          rr_cfg_ded.phy_chan_list_cfg[0].band_id_present = true;
          rr_cfg_ded.phy_chan_list_cfg[0].freq_id_present = true;
          rr_cfg_ded.phy_chan_list_cfg[0].slot_ass_present = true;
          rr_cfg_ded.phy_chan_list_cfg[0].s_rnti.srnti.from_string("000110");
          rr_cfg_ded.phy_chan_list_cfg[0].chan_type = phy_chan_cfg_s::chan_type_opts::pDCH11;
          rr_cfg_ded.phy_chan_list_cfg[0].band_id.ba_id.from_string("001110");
          rr_cfg_ded.phy_chan_list_cfg[0].freq_id.freq_id.from_string("01");
          if (rr_cfg_ded.phy_chan_list_cfg[0].chan_type == 1 || rr_cfg_ded.phy_chan_list_cfg[0].chan_type == 3 || rr_cfg_ded.phy_chan_list_cfg[0].chan_type == 5)
          {
            rr_cfg_ded.phy_chan_list_cfg[0].slot_ass.from_string("00100"); // psch5-1/pdch1-1/psch1-1=00100
          }
          if (rr_cfg_ded.phy_chan_list_cfg[0].chan_type == 2 || rr_cfg_ded.phy_chan_list_cfg[0].chan_type == 4 || rr_cfg_ded.phy_chan_list_cfg[0].chan_type == 6)
          {
            rr_cfg_ded.phy_chan_list_cfg[0].slot_ass.from_string("11000"); // psch1-2/psch_5-2/pdch1-2=11000
          }

          rr_cfg_ded.phy_chan_list_cfg[0].direc_t = phy_chan_cfg_s::direct_opts::ulDirection;

          if (rr_cfg_ded.phy_chan_list_cfg[0].chan_type != 1 && rr_cfg_ded.phy_chan_list_cfg[0].chan_type != 2)
          {
            rr_cfg_ded.phy_chan_list_cfg[0].sche_type_present = true;
            rr_cfg_ded.phy_chan_list_cfg[0].sche_type = phy_chan_cfg_s::sche_type_opts::Static;
          }

          rr_cfg_ded.phy_chan_list_cfg[1].band_id_present = true;
          rr_cfg_ded.phy_chan_list_cfg[1].freq_id_present = true;
          rr_cfg_ded.phy_chan_list_cfg[1].slot_ass_present = true;
          rr_cfg_ded.phy_chan_list_cfg[1].s_rnti.srnti.from_string("000110");
          rr_cfg_ded.phy_chan_list_cfg[1].chan_type = phy_chan_cfg_s::chan_type_opts::dSPDTCH1;
          rr_cfg_ded.phy_chan_list_cfg[1].band_id.ba_id.from_string("000110");
          rr_cfg_ded.phy_chan_list_cfg[1].freq_id.freq_id.from_string("01");
          rr_cfg_ded.phy_chan_list_cfg[1].slot_ass.from_string("01000"); // dSPDTCHT need to modify to 01000
          rr_cfg_ded.phy_chan_list_cfg[1].pdtch_code_present = true;
          rr_cfg_ded.phy_chan_list_cfg[1].pdtch_code.pdt_phy_code = 21;
          rr_cfg_ded.phy_chan_list_cfg[1].direc_t = phy_chan_cfg_s::direct_opts::dlDirection;

          if (rr_cfg_ded.phy_chan_list_cfg[1].chan_type == 3 || rr_cfg_ded.phy_chan_list_cfg[1].chan_type == 4 || rr_cfg_ded.phy_chan_list_cfg[1].chan_type == 5 || rr_cfg_ded.phy_chan_list_cfg[1].chan_type == 6)
          {
            rr_cfg_ded.phy_chan_list_cfg[1].sche_type_present = true;
            rr_cfg_ded.phy_chan_list_cfg[1].sche_type = phy_chan_cfg_s::sche_type_opts::Static;
          }
        }

        // securityConfig
        secu_cfg_s &sec_cfg = rr_cfg_ded.secu_cfg;
        sec_cfg.sec_alg_cfg.integ_prot_alg_present = true;
        sec_cfg.sec_alg_cfg.coph_alg = sec_alg_cfg_s::ciph_alg_opts::nea0;
        sec_cfg.sec_alg_cfg.intef_prot_alg = sec_alg_cfg_s::intef_prot_alg_opts::nia0;

        // zyg
        // for MAC lcid map
        // parent->mac->updateMap(1,static_cast<ChanType_t>(rr_cfg_ded.phy_chan_list_cfg[0].chan_type.value));

        // configMap DRB_configMap;
        if (drb_add.pdcp_cfg.rlc_am_present == true)
        {

          DRB_configMap.ul_Type = static_cast<ChanType_t>(rr_cfg_ded.phy_chan_list_cfg[0].chan_type.value);
          if (wx_area_mode == 0)
          {
            std::cout << " wx_area_mode 0 " << std::endl;
            DRB_configMap.dl_Type = static_cast<ChanType_t>(rr_cfg_ded.phy_chan_list_cfg[0].chan_type.value);
          }
          else
          {
            std::cout << " wx_area_mode 1 " << std::endl;
            DRB_configMap.dl_Type = static_cast<ChanType_t>(rr_cfg_ded.phy_chan_list_cfg[1].chan_type.value);
          }
          DRB_configMap.voicetype = N_Voice;
          DRB_configMap.rlcType = rlc_am;
          DRB_configMap.dataType = Data;
          std::cout << "drb_add_2.drb_id = " << drb_add_2.drb_id << std::endl;
          parent->mac->addlcidMap(drb_add_2.drb_id, DRB_configMap);
          parent->mac->addlcidMap(drb_add.drb_id, DRB_configMap);
        }
        else if (drb_add.pdcp_cfg.rlc_um_present == true)
        {
          DRB_configMap.ul_Type = static_cast<ChanType_t>(rr_cfg_ded.phy_chan_list_cfg[0].chan_type.value);
          if (wx_area_mode == 0)
          {
            std::cout << " wx_area_mode 0 " << std::endl;
            DRB_configMap.dl_Type = static_cast<ChanType_t>(rr_cfg_ded.phy_chan_list_cfg[0].chan_type.value);
          }
          else
          {
            std::cout << " wx_area_mode 1 " << std::endl;
            DRB_configMap.dl_Type = static_cast<ChanType_t>(rr_cfg_ded.phy_chan_list_cfg[1].chan_type.value);
          }
          DRB_configMap.voicetype = N_Voice;
          DRB_configMap.rlcType = rlc_um;
          DRB_configMap.dataType = Data;
          std::cout << "drb_add_2.drb_id = " << drb_add_2.drb_id << std::endl;
          parent->mac->addlcidMap(drb_add_2.drb_id, DRB_configMap);
          parent->mac->addlcidMap(drb_add.drb_id, DRB_configMap);
        }
      }
      else if (parent->rrc_adp->udp_.TC_714_DRB_Release && TC_714_reconfig_number == 2)
      {

        drb_id_s &drb_rel = rr_cfg_ded.drb_to_rel_list[0];
        drb_rel.drb_id = 3;

        // peri_bsr_timer_e_
        rr_cfg_ded.peri_bsr_timer = redio_resour_cfg_dedi_s::peri_bsr_timer_opts::rf5;

        // PhysicalChannel-Config------------------------------------------配置物理参数
        if (wx_area_mode == 0)
        {
          rr_cfg_ded.phy_chan_list_cfg.resize(1);
          if (rr_cfg_ded.phy_chan_list_cfg.size() == 1)
          {
            phy_chan_cfg_s &phy_chan = rr_cfg_ded.phy_chan_list_cfg[0];
            phy_chan.band_id_present = true;
            phy_chan.freq_id_present = true;
            phy_chan.slot_ass_present = true;

            phy_chan.s_rnti.srnti.from_string("000101");
            if (ue_category == 14)
            {

              std::cout << "xx ue_cap14_band_id = " << ue_cap14_band_id << std::endl;
              std::cout << "xx ue_cap14_freq_id = " << ue_cap14_freq_id << std::endl;
              std::cout << "xx ue_cap14_slot = " << ue_cap14_slot << std::endl;
              phy_chan.chan_type = phy_chan_cfg_s::chan_type_opts::pDCH11; // change channel
              phy_chan.band_id.ba_id.from_number(ue_cap14_band_id);
              phy_chan.freq_id.freq_id.from_number(ue_cap14_freq_id);
              phy_chan.slot_ass.from_number(ue_cap14_slot);
            }
            else
            {
              phy_chan.chan_type = phy_chan_cfg_s::chan_type_opts::pDCH11; // change channel
              if (parent->rrc_adp->udp_.Gen_.test_id == 623)
              {
                std::cout << "parent->rrc_adp->udp_.Gen_.test_id==623" << std::endl;
                phy_chan.chan_type = phy_chan_cfg_s::chan_type_opts::pDCH12; // change channel
              }
              // phy_chan.chan_type = phy_chan_cfg_s::chan_type_opts::pSCH11; // change channel
              phy_chan.band_id.ba_id.from_string("001011");
              phy_chan.freq_id.freq_id.from_string("01");

              if (phy_chan.chan_type == 1 || phy_chan.chan_type == 3 || phy_chan.chan_type == 5)
              {
                phy_chan.slot_ass.from_string("00100"); // psch5-1/pdch1-1/psch1-1=00100
              }
              if (phy_chan.chan_type == 2 || phy_chan.chan_type == 4 || phy_chan.chan_type == 6)
              {
                phy_chan.slot_ass.from_string("11000"); // psch1-2/psch_5-2/pdch1-2=01100
              }
            }

            std::cout << " phy_chan.band_id.ba_id = " << phy_chan.band_id.ba_id.to_string() << std::endl;
            std::cout << " phy_chan.freq_id.freq_id = " << phy_chan.freq_id.freq_id.to_string() << std::endl;
            std::cout << " phy_chan.slot_ass = " << phy_chan.slot_ass.to_string() << std::endl;

            phy_chan.direc_t = phy_chan_cfg_s::direct_opts::biDirection;

            if (phy_chan.chan_type != 1 && phy_chan.chan_type != 2)
            {
              phy_chan.sche_type_present = true;
              phy_chan.sche_type = phy_chan_cfg_s::sche_type_opts::Static;
            }
          }
        }

        if (wx_area_mode == 1)
        {
          rr_cfg_ded.phy_chan_list_cfg.resize(2);

          rr_cfg_ded.phy_chan_list_cfg[0].band_id_present = true;
          rr_cfg_ded.phy_chan_list_cfg[0].freq_id_present = true;
          rr_cfg_ded.phy_chan_list_cfg[0].slot_ass_present = true;
          rr_cfg_ded.phy_chan_list_cfg[0].s_rnti.srnti.from_string("000110");
          rr_cfg_ded.phy_chan_list_cfg[0].chan_type = phy_chan_cfg_s::chan_type_opts::pDCH11;
          rr_cfg_ded.phy_chan_list_cfg[0].band_id.ba_id.from_string("001110");
          rr_cfg_ded.phy_chan_list_cfg[0].freq_id.freq_id.from_string("01");
          if (rr_cfg_ded.phy_chan_list_cfg[0].chan_type == 1 || rr_cfg_ded.phy_chan_list_cfg[0].chan_type == 3 || rr_cfg_ded.phy_chan_list_cfg[0].chan_type == 5)
          {
            rr_cfg_ded.phy_chan_list_cfg[0].slot_ass.from_string("00100"); // psch5-1/pdch1-1/psch1-1=00100
          }
          if (rr_cfg_ded.phy_chan_list_cfg[0].chan_type == 2 || rr_cfg_ded.phy_chan_list_cfg[0].chan_type == 4 || rr_cfg_ded.phy_chan_list_cfg[0].chan_type == 6)
          {
            rr_cfg_ded.phy_chan_list_cfg[0].slot_ass.from_string("11000"); // psch1-2/psch_5-2/pdch1-2=11000
          }

          rr_cfg_ded.phy_chan_list_cfg[0].direc_t = phy_chan_cfg_s::direct_opts::ulDirection;

          if (rr_cfg_ded.phy_chan_list_cfg[0].chan_type != 1 && rr_cfg_ded.phy_chan_list_cfg[0].chan_type != 2)
          {
            rr_cfg_ded.phy_chan_list_cfg[0].sche_type_present = true;
            rr_cfg_ded.phy_chan_list_cfg[0].sche_type = phy_chan_cfg_s::sche_type_opts::Static;
          }

          rr_cfg_ded.phy_chan_list_cfg[1].band_id_present = true;
          rr_cfg_ded.phy_chan_list_cfg[1].freq_id_present = true;
          rr_cfg_ded.phy_chan_list_cfg[1].slot_ass_present = true;
          rr_cfg_ded.phy_chan_list_cfg[1].s_rnti.srnti.from_string("000110");
          rr_cfg_ded.phy_chan_list_cfg[1].chan_type = phy_chan_cfg_s::chan_type_opts::dSPDTCH1;
          rr_cfg_ded.phy_chan_list_cfg[1].band_id.ba_id.from_string("000110");
          rr_cfg_ded.phy_chan_list_cfg[1].freq_id.freq_id.from_string("01");
          rr_cfg_ded.phy_chan_list_cfg[1].slot_ass.from_string("01000"); // dSPDTCHT need to modify to 01000
          rr_cfg_ded.phy_chan_list_cfg[1].pdtch_code_present = true;
          rr_cfg_ded.phy_chan_list_cfg[1].pdtch_code.pdt_phy_code = 21;
          rr_cfg_ded.phy_chan_list_cfg[1].direc_t = phy_chan_cfg_s::direct_opts::dlDirection;

          if (rr_cfg_ded.phy_chan_list_cfg[1].chan_type == 3 || rr_cfg_ded.phy_chan_list_cfg[1].chan_type == 4 || rr_cfg_ded.phy_chan_list_cfg[1].chan_type == 5 || rr_cfg_ded.phy_chan_list_cfg[1].chan_type == 6)
          {
            rr_cfg_ded.phy_chan_list_cfg[1].sche_type_present = true;
            rr_cfg_ded.phy_chan_list_cfg[1].sche_type = phy_chan_cfg_s::sche_type_opts::Static;
          }
        }

        // securityConfig
        secu_cfg_s &sec_cfg = rr_cfg_ded.secu_cfg;
        sec_cfg.sec_alg_cfg.integ_prot_alg_present = true;
        sec_cfg.sec_alg_cfg.coph_alg = sec_alg_cfg_s::ciph_alg_opts::nea0;
        sec_cfg.sec_alg_cfg.intef_prot_alg = sec_alg_cfg_s::intef_prot_alg_opts::nia0;
      }
      else
      {
        drb_to_add_modi_s &drb_add = rr_cfg_ded.drb_to_add_mod_list[0];
        drb_add.sdap_cfg_present = true;
        drb_add.pdcp_cfg_present = true;
        drb_add.rlc_cfg_present = true;
        drb_add.log_chan_cfg_present = true;
        // sdap
        drb_add.sdap_cfg.map_qos_flows_to_add_present = true;
        drb_add.sdap_cfg.pdu_sess_id.pdu_ses_id = pdu_ses_id;
        drb_add.sdap_cfg.sdap_header_dl = sdap_cfg_s::sdap_header_dl_opts::absent;
        drb_add.sdap_cfg.sdap_header_ul = sdap_cfg_s::sdap_header_ul_opts::absent;
        drb_add.sdap_cfg.default_drb = false;
        drb_add.sdap_cfg.map_qos_flows_to_add.resize(1);
        drb_add.sdap_cfg.map_qos_flows_to_add[0].qfi = qfi_nas;
        if (drb_add.sdap_cfg.pdu_sess_id.pdu_ses_id == 1)
        {
          if (parent->rrc_adp->udp_.TC_715_BEAM2_REEST)
          {
            drb_add.drb_id = 4;
          }
          else
          {
            drb_add.drb_id = 3;
          }
        }
        if (drb_add.sdap_cfg.pdu_sess_id.pdu_ses_id == 2)
        {
          drb_add.drb_id = 4;
        }

        // pdcp
        drb_add.pdcp_cfg.dis_timer_present = true;
        drb_add.pdcp_cfg.integ_pro_present = true;
        drb_add.pdcp_cfg.dis_timer = pdcp_cofg_s::dis_timer_opts::infinity;
        drb_add.pdcp_cfg.header_com.set_not_used_l();
        drb_add.pdcp_cfg.integ_pro = pdcp_cofg_s::integ_pro_opts::enabled;

        // if testcase don't need um , its am
        // 11.15
        std::cout << "parent->cfg.ttcn_test_enble = " << parent->cfg.ttcn_test_enble << std::endl;
        if (parent->cfg.ttcn_test_enble)
        {
          if (parent->rrc_adp->udp_.is_um == true || parent->rrc_adp->udp_.TC_713_reconfig_DRB)
          {
            drb_add.pdcp_cfg.rlc_um_present = true;
          }
          else
          {
            drb_add.pdcp_cfg.rlc_am_present = true;
            drb_add.pdcp_cfg.rlc_am.stat_rep_req = true;
          }
        }
        else
        {
          drb_add.pdcp_cfg.rlc_um_present = true;
          if (rlc_mode == 0)
          {
            drb_add.pdcp_cfg.rlc_am_present = true;
          }
          else if (rlc_mode ==2)
          {
            drb_add.pdcp_cfg.rlc_tm_present = true;
          }
          else
          {
            drb_add.pdcp_cfg.rlc_um_present = true;
          }
          
        }

        //  rlc
        if (drb_add.pdcp_cfg.rlc_am_present == true)
        {
          std::cout << "rlc_am_present" << std::endl;
          drb_add.rlc_cfg.set_am();
          am_s_ &ams = drb_add.rlc_cfg.set_am();
          ams.ul_am_rlc.t_poll_retran = ul_am_rlcc_s::t_poll_retran_opts::ms480;
          ams.ul_am_rlc.poll_pdu = ul_am_rlcc_s::poll_pdu_opts::p32;
          ams.ul_am_rlc.poll_byte = ul_am_rlcc_s::poll_byte_opts::kB128;
          ams.ul_am_rlc.max_retx_thres_hold = ul_am_rlcc_s::max_retx_thres_hold_opts::t4;
          ams.dl_am_rlc.t_reord = dl_am_rlcc_s::t_reord_opts::ms480;   // fixed
          ams.dl_am_rlc.t_status_proh = dl_am_rlcc_s::t_status_proh_opts::ms420;

          // TC_712
          if (parent->cfg.ttcn_test_enble && parent->cfg.ttcn_rrc_enble && parent->rrc_adp->udp_.TC_712_reconfig_update)
          {
            while (true)
            {
              if (parent->rrc_adp->udp_.rrc_receive_info.size() != 0)
              {
                std::cout << " TTCN Reconf Update Info " << std::endl;
                srsran::unique_byte_buffer_t reconf_update =
                    srsran::make_byte_buffer();
                reconf_update->init();
                parent->rrc_adp->udp_.rrc_receive_info.try_pop(reconf_update);
                parent->get_general_interface(&reconf_update);
                ams.ul_am_rlc.t_poll_retran = static_cast<asn1::rrc::ul_am_rlcc_s::t_poll_retran_opts::options>((reconf_update->msg[8]));
                ams.ul_am_rlc.poll_pdu = static_cast<asn1::rrc::ul_am_rlcc_s::poll_pdu_opts::options>((reconf_update->msg[9]));
                ams.ul_am_rlc.poll_byte = static_cast<asn1::rrc::ul_am_rlcc_s::poll_byte_opts::options>((reconf_update->msg[10]));
                ams.ul_am_rlc.max_retx_thres_hold = static_cast<asn1::rrc::ul_am_rlcc_s::max_retx_thres_hold_opts::options>((reconf_update->msg[11]));
                ams.dl_am_rlc.t_reord = static_cast<asn1::rrc::dl_am_rlcc_s::t_reord_opts::options>((reconf_update->msg[12]));
                ams.dl_am_rlc.t_status_proh = static_cast<asn1::rrc::dl_am_rlcc_s::t_status_proh_opts::options>((reconf_update->msg[13]));

                std::cout << "t_poll_retran:" << ams.ul_am_rlc.t_poll_retran << std::endl;
                std::cout << "poll_pdu:" << ams.ul_am_rlc.poll_pdu << std::endl;
                std::cout << "poll_byte:" << ams.ul_am_rlc.poll_byte << std::endl;
                std::cout << "max_retx_thres_hold:" << ams.ul_am_rlc.max_retx_thres_hold << std::endl;
                std::cout << "t_reord:" << ams.dl_am_rlc.t_reord << std::endl;
                std::cout << "t_status_proh:" << ams.dl_am_rlc.t_status_proh << std::endl;

                // TTCN-3 config parameters
                break;
              }
            }
          }
        }
        else if (drb_add.pdcp_cfg.rlc_um_present == true)
        {
          std::cout << "rlc_um_present" << std::endl;
          drb_add.rlc_cfg.set_um_bi_dir();
          um_bi_dir_s_ &ums = drb_add.rlc_cfg.set_um_bi_dir();
          ums.dl_um_rlc.t_reord = dl_um_rlcc_s::t_reord_opts::ms480;   // fixed:480ms
          // 713
          if (TC_713_reconfig_number == 2)
          {
            ums.dl_um_rlc.t_reord = dl_um_rlcc_s::t_reord_opts::ms1200;
          }
        }

        // LogicalChannelConfig
        drb_add.log_chan_cfg.ul_spec_para_present = true;
        ul_spec_para_s &ul_sp_para = drb_add.log_chan_cfg.ul_spec_para;
        ul_sp_para.log_chan_goup_present = true;
        ul_sp_para.priority = 1;
        ul_sp_para.prio_bit_rate = ul_spec_para_s::prio_bit_rate_opts::kBps16;
        ul_sp_para.buck_size_dura = ul_spec_para_s::buck_size_dura_opts::ms60;
        ul_sp_para.log_chan_goup = 1;

        // peri_bsr_timer_e_
        rr_cfg_ded.peri_bsr_timer = redio_resour_cfg_dedi_s::peri_bsr_timer_opts::rf5;

        // PhysicalChannel-Config------------------------------------------配置物理参数
        if (wx_area_mode == 0)
        {
          rr_cfg_ded.phy_chan_list_cfg.resize(1);
          if (rr_cfg_ded.phy_chan_list_cfg.size() == 1)
          {
            phy_chan_cfg_s &phy_chan = rr_cfg_ded.phy_chan_list_cfg[0];
            phy_chan.band_id_present = true;
            phy_chan.freq_id_present = true;
            phy_chan.slot_ass_present = true;

            phy_chan.s_rnti.srnti.from_string("000101");
            srnti = phy_chan.s_rnti.srnti.to_number();
            parent->mac->getSrnti(srnti);

            if (ue_category == 14)
            {

              std::cout << "xx ue_cap14_band_id = " << ue_cap14_band_id << std::endl;
              std::cout << "xx ue_cap14_freq_id = " << ue_cap14_freq_id << std::endl;
              std::cout << "xx ue_cap14_slot = " << ue_cap14_slot << std::endl;
              phy_chan.chan_type = phy_chan_cfg_s::chan_type_opts::pDCH11; // change channel
              phy_chan.band_id.ba_id.from_number(ue_cap14_band_id);
              phy_chan.freq_id.freq_id.from_number(ue_cap14_freq_id);
              phy_chan.slot_ass.from_number(ue_cap14_slot);
            }
            else
            {
              phy_chan.chan_type = phy_chan_cfg_s::chan_type_opts::pDCH11; // change channel
              // phy_chan.chan_type = phy_chan_cfg_s::chan_type_opts::pSCH51;
              if (parent->rrc_adp->udp_.Gen_.test_id == 623)
              {
                std::cout << "parent->rrc_adp->udp_.Gen_.test_id==623" << std::endl;
                phy_chan.chan_type = phy_chan_cfg_s::chan_type_opts::pDCH12; // change channel
              }
              // phy_chan.chan_type = phy_chan_cfg_s::chan_type_opts::pSCH11; // change channel
              if (parent->rrc_adp->udp_.TC_716_reconf_fail_last)
              {
                parent->rrc_adp->udp_.TC_716_reconf_fail_last = false;
              }
              else
              {
                phy_chan.band_id.ba_id.from_string("001011");
              }
              phy_chan.freq_id.freq_id.from_string("01");

              if (phy_chan.chan_type == 1 || phy_chan.chan_type == 3 || phy_chan.chan_type == 5)
              {
                phy_chan.slot_ass.from_string("00100"); // psch5-1/pdch1-1/psch1-1=00100
              }
              if (phy_chan.chan_type == 2 || phy_chan.chan_type == 4 || phy_chan.chan_type == 6)
              {
                phy_chan.slot_ass.from_string("11000"); // psch1-2/psch_5-2/pdch1-2=01100
              }
            }

            std::cout << " phy_chan.band_id.ba_id = " << phy_chan.band_id.ba_id.to_string() << std::endl;
            std::cout << " phy_chan.freq_id.freq_id = " << phy_chan.freq_id.freq_id.to_string() << std::endl;
            std::cout << " phy_chan.slot_ass = " << phy_chan.slot_ass.to_string() << std::endl;

            phy_chan.direc_t = phy_chan_cfg_s::direct_opts::biDirection;

            if (phy_chan.chan_type != 1 && phy_chan.chan_type != 2)
            {
              phy_chan.sche_type_present = true;
              phy_chan.sche_type = phy_chan_cfg_s::sche_type_opts::Static;
            }
          }

          if (rr_cfg_ded.phy_chan_list_cfg.size() > 1)
          {
            rr_cfg_ded.phy_chan_list_cfg[0].band_id_present = true;
            rr_cfg_ded.phy_chan_list_cfg[0].freq_id_present = true;
            rr_cfg_ded.phy_chan_list_cfg[0].slot_ass_present = true;
            rr_cfg_ded.phy_chan_list_cfg[0].sche_type_present = true;
            rr_cfg_ded.phy_chan_list_cfg[0].sche_type = phy_chan_cfg_s::sche_type_opts::Static;
            rr_cfg_ded.phy_chan_list_cfg[0].s_rnti.srnti.from_string("000101");
            srnti = rr_cfg_ded.phy_chan_list_cfg[0].s_rnti.srnti.to_number();
            parent->mac->getSrnti(srnti);

            rr_cfg_ded.phy_chan_list_cfg[0].chan_type = phy_chan_cfg_s::chan_type_opts::pSCH51;
            rr_cfg_ded.phy_chan_list_cfg[0].band_id.ba_id.from_string("000110");
            rr_cfg_ded.phy_chan_list_cfg[0].freq_id.freq_id.from_string("00");
            rr_cfg_ded.phy_chan_list_cfg[0].slot_ass.from_string("00001");
            rr_cfg_ded.phy_chan_list_cfg[0].direc_t = phy_chan_cfg_s::direct_opts::biDirection;

            rr_cfg_ded.phy_chan_list_cfg[1].band_id_present = true;
            rr_cfg_ded.phy_chan_list_cfg[1].freq_id_present = true;
            rr_cfg_ded.phy_chan_list_cfg[1].slot_ass_present = true;
            rr_cfg_ded.phy_chan_list_cfg[1].sche_type_present = true;
            rr_cfg_ded.phy_chan_list_cfg[1].sche_type = phy_chan_cfg_s::sche_type_opts::Static;
            rr_cfg_ded.phy_chan_list_cfg[1].s_rnti.srnti.from_string("000101");
            rr_cfg_ded.phy_chan_list_cfg[1].chan_type = phy_chan_cfg_s::chan_type_opts::pSCH51;
            rr_cfg_ded.phy_chan_list_cfg[1].band_id.ba_id.from_string("010000");
            rr_cfg_ded.phy_chan_list_cfg[1].freq_id.freq_id.from_string("00");
            rr_cfg_ded.phy_chan_list_cfg[1].slot_ass.from_string("00001");
            rr_cfg_ded.phy_chan_list_cfg[1].direc_t = phy_chan_cfg_s::direct_opts::biDirection;
          }
        }

        if (wx_area_mode == 1)
        {
          rr_cfg_ded.phy_chan_list_cfg.resize(2);

          rr_cfg_ded.phy_chan_list_cfg[0].band_id_present = true;
          rr_cfg_ded.phy_chan_list_cfg[0].freq_id_present = true;
          rr_cfg_ded.phy_chan_list_cfg[0].slot_ass_present = true;
          rr_cfg_ded.phy_chan_list_cfg[0].s_rnti.srnti.from_string("000110");
          rr_cfg_ded.phy_chan_list_cfg[0].chan_type = phy_chan_cfg_s::chan_type_opts::pDCH11;
          rr_cfg_ded.phy_chan_list_cfg[0].band_id.ba_id.from_string("001110");
          rr_cfg_ded.phy_chan_list_cfg[0].freq_id.freq_id.from_string("01");
          if (rr_cfg_ded.phy_chan_list_cfg[0].chan_type == 1 || rr_cfg_ded.phy_chan_list_cfg[0].chan_type == 3 || rr_cfg_ded.phy_chan_list_cfg[0].chan_type == 5)
          {
            rr_cfg_ded.phy_chan_list_cfg[0].slot_ass.from_string("00100"); // psch5-1/pdch1-1/psch1-1=00100
          }
          if (rr_cfg_ded.phy_chan_list_cfg[0].chan_type == 2 || rr_cfg_ded.phy_chan_list_cfg[0].chan_type == 4 || rr_cfg_ded.phy_chan_list_cfg[0].chan_type == 6)
          {
            rr_cfg_ded.phy_chan_list_cfg[0].slot_ass.from_string("11000"); // psch1-2/psch_5-2/pdch1-2=11000
          }

          rr_cfg_ded.phy_chan_list_cfg[0].direc_t = phy_chan_cfg_s::direct_opts::ulDirection;

          if (rr_cfg_ded.phy_chan_list_cfg[0].chan_type != 1 && rr_cfg_ded.phy_chan_list_cfg[0].chan_type != 2)
          {
            rr_cfg_ded.phy_chan_list_cfg[0].sche_type_present = true;
            rr_cfg_ded.phy_chan_list_cfg[0].sche_type = phy_chan_cfg_s::sche_type_opts::Static;
          }

          rr_cfg_ded.phy_chan_list_cfg[1].band_id_present = true;
          rr_cfg_ded.phy_chan_list_cfg[1].freq_id_present = true;
          rr_cfg_ded.phy_chan_list_cfg[1].slot_ass_present = true;
          rr_cfg_ded.phy_chan_list_cfg[1].s_rnti.srnti.from_string("000110");
          rr_cfg_ded.phy_chan_list_cfg[1].chan_type = phy_chan_cfg_s::chan_type_opts::dSPDTCH1;
          rr_cfg_ded.phy_chan_list_cfg[1].band_id.ba_id.from_string("000110");
          rr_cfg_ded.phy_chan_list_cfg[1].freq_id.freq_id.from_string("01");
          rr_cfg_ded.phy_chan_list_cfg[1].slot_ass.from_string("01000"); // dSPDTCHT need to modify to 01000
          rr_cfg_ded.phy_chan_list_cfg[1].pdtch_code_present = true;
          rr_cfg_ded.phy_chan_list_cfg[1].pdtch_code.pdt_phy_code = 21;
          rr_cfg_ded.phy_chan_list_cfg[1].direc_t = phy_chan_cfg_s::direct_opts::dlDirection;

          if (rr_cfg_ded.phy_chan_list_cfg[1].chan_type == 3 || rr_cfg_ded.phy_chan_list_cfg[1].chan_type == 4 || rr_cfg_ded.phy_chan_list_cfg[1].chan_type == 5 || rr_cfg_ded.phy_chan_list_cfg[1].chan_type == 6)
          {
            rr_cfg_ded.phy_chan_list_cfg[1].sche_type_present = true;
            rr_cfg_ded.phy_chan_list_cfg[1].sche_type = phy_chan_cfg_s::sche_type_opts::Static;
          }
        }

        // securityConfig
        secu_cfg_s &sec_cfg = rr_cfg_ded.secu_cfg;
        sec_cfg.sec_alg_cfg.integ_prot_alg_present = true;
        sec_cfg.sec_alg_cfg.coph_alg = sec_alg_cfg_s::ciph_alg_opts::nea0;
        sec_cfg.sec_alg_cfg.intef_prot_alg = sec_alg_cfg_s::intef_prot_alg_opts::nia0;

        // zyg
        // for MAC lcid map
        // parent->mac->updateMap(1,static_cast<ChanType_t>(rr_cfg_ded.phy_chan_list_cfg[0].chan_type.value));

        // configMap DRB_configMap;
        if (drb_add.pdcp_cfg.rlc_am_present == true)
        {

          DRB_configMap.ul_Type = static_cast<ChanType_t>(rr_cfg_ded.phy_chan_list_cfg[0].chan_type.value);
          if (wx_area_mode == 0)
          {
            std::cout << " wx_area_mode 0 " << std::endl;
            DRB_configMap.dl_Type = static_cast<ChanType_t>(rr_cfg_ded.phy_chan_list_cfg[0].chan_type.value);
          }
          else
          {
            std::cout << " wx_area_mode 1 " << std::endl;
            DRB_configMap.dl_Type = static_cast<ChanType_t>(rr_cfg_ded.phy_chan_list_cfg[1].chan_type.value);
          }
          DRB_configMap.voicetype = N_Voice;
          DRB_configMap.rlcType = rlc_am;
          DRB_configMap.dataType = Data;

          parent->mac->addlcidMap(drb_add.drb_id, DRB_configMap);
        }
        else if (drb_add.pdcp_cfg.rlc_um_present == true)
        {
          DRB_configMap.ul_Type = static_cast<ChanType_t>(rr_cfg_ded.phy_chan_list_cfg[0].chan_type.value);
          if (wx_area_mode == 0)
          {
            std::cout << " wx_area_mode 0 " << std::endl;
            DRB_configMap.dl_Type = static_cast<ChanType_t>(rr_cfg_ded.phy_chan_list_cfg[0].chan_type.value);
          }
          else
          {
            std::cout << " wx_area_mode 1 " << std::endl;
            DRB_configMap.dl_Type = static_cast<ChanType_t>(rr_cfg_ded.phy_chan_list_cfg[1].chan_type.value);
          }
          DRB_configMap.voicetype = N_Voice;
          DRB_configMap.rlcType = rlc_um;
          DRB_configMap.dataType = Data;
          parent->mac->addlcidMap(drb_add.drb_id, DRB_configMap);
        }
      }

      // // peri_bsr_timer_e_
      // rr_cfg_ded.peri_bsr_timer = redio_resour_cfg_dedi_s::peri_bsr_timer_opts::rf5;

      // // PhysicalChannel-Config------------------------------------------配置物理参数
      // if (wx_area_mode == 0)
      // {
      //   rr_cfg_ded.phy_chan_list_cfg.resize(1);
      //   if (rr_cfg_ded.phy_chan_list_cfg.size() == 1)
      //   {
      //     phy_chan_cfg_s &phy_chan = rr_cfg_ded.phy_chan_list_cfg[0];
      //     phy_chan.band_id_present = true;
      //     phy_chan.freq_id_present = true;
      //     phy_chan.slot_ass_present = true;

      //     phy_chan.s_rnti.srnti.from_string("000101");
      //     if (ue_category == 14)
      //     {

      //       std::cout << "xx ue_cap14_band_id = " << ue_cap14_band_id << std::endl;
      //       std::cout << "xx ue_cap14_freq_id = " << ue_cap14_freq_id << std::endl;
      //       std::cout << "xx ue_cap14_slot = " << ue_cap14_slot << std::endl;
      //       phy_chan.chan_type = phy_chan_cfg_s::chan_type_opts::pDCH11; // change channel
      //       phy_chan.band_id.ba_id.from_number(ue_cap14_band_id);
      //       phy_chan.freq_id.freq_id.from_number(ue_cap14_freq_id);
      //       phy_chan.slot_ass.from_number(ue_cap14_slot);
      //     }
      //     else
      //     {
      //       phy_chan.chan_type = phy_chan_cfg_s::chan_type_opts::pDCH11; // change channel
      //       phy_chan.band_id.ba_id.from_string("001011");
      //       phy_chan.freq_id.freq_id.from_string("01");

      //       if (phy_chan.chan_type == 1 || phy_chan.chan_type == 3 || phy_chan.chan_type == 5)
      //       {
      //         phy_chan.slot_ass.from_string("00100"); // psch5-1/pdch1-1/psch1-1=00100
      //       }
      //       if (phy_chan.chan_type == 2 || phy_chan.chan_type == 4 || phy_chan.chan_type == 6)
      //       {
      //         phy_chan.slot_ass.from_string("11000"); // psch1-2/psch_5-2/pdch1-2=01100
      //       }
      //     }

      //     std::cout << " phy_chan.band_id.ba_id = " << phy_chan.band_id.ba_id.to_string() << std::endl;
      //     std::cout << " phy_chan.freq_id.freq_id = " << phy_chan.freq_id.freq_id.to_string() << std::endl;
      //     std::cout << " phy_chan.slot_ass = " << phy_chan.slot_ass.to_string() << std::endl;

      //     phy_chan.direc_t = phy_chan_cfg_s::direct_opts::biDirection;

      //     if (phy_chan.chan_type != 1 && phy_chan.chan_type != 2)
      //     {
      //       phy_chan.sche_type_present = true;
      //       phy_chan.sche_type = phy_chan_cfg_s::sche_type_opts::Static;
      //     }
      //   }

      //   if (rr_cfg_ded.phy_chan_list_cfg.size() > 1)
      //   {
      //     rr_cfg_ded.phy_chan_list_cfg[0].band_id_present = true;
      //     rr_cfg_ded.phy_chan_list_cfg[0].freq_id_present = true;
      //     rr_cfg_ded.phy_chan_list_cfg[0].slot_ass_present = true;
      //     rr_cfg_ded.phy_chan_list_cfg[0].sche_type_present = true;
      //     rr_cfg_ded.phy_chan_list_cfg[0].sche_type = phy_chan_cfg_s::sche_type_opts::Static;
      //     rr_cfg_ded.phy_chan_list_cfg[0].s_rnti.srnti.from_string("000101");
      //     rr_cfg_ded.phy_chan_list_cfg[0].chan_type = phy_chan_cfg_s::chan_type_opts::pSCH51;
      //     rr_cfg_ded.phy_chan_list_cfg[0].band_id.ba_id.from_string("000110");
      //     rr_cfg_ded.phy_chan_list_cfg[0].freq_id.freq_id.from_string("00");
      //     rr_cfg_ded.phy_chan_list_cfg[0].slot_ass.from_string("00001");
      //     rr_cfg_ded.phy_chan_list_cfg[0].direc_t = phy_chan_cfg_s::direct_opts::biDirection;

      //     rr_cfg_ded.phy_chan_list_cfg[1].band_id_present = true;
      //     rr_cfg_ded.phy_chan_list_cfg[1].freq_id_present = true;
      //     rr_cfg_ded.phy_chan_list_cfg[1].slot_ass_present = true;
      //     rr_cfg_ded.phy_chan_list_cfg[1].sche_type_present = true;
      //     rr_cfg_ded.phy_chan_list_cfg[1].sche_type = phy_chan_cfg_s::sche_type_opts::Static;
      //     rr_cfg_ded.phy_chan_list_cfg[1].s_rnti.srnti.from_string("000101");
      //     rr_cfg_ded.phy_chan_list_cfg[1].chan_type = phy_chan_cfg_s::chan_type_opts::pSCH51;
      //     rr_cfg_ded.phy_chan_list_cfg[1].band_id.ba_id.from_string("010000");
      //     rr_cfg_ded.phy_chan_list_cfg[1].freq_id.freq_id.from_string("00");
      //     rr_cfg_ded.phy_chan_list_cfg[1].slot_ass.from_string("00001");
      //     rr_cfg_ded.phy_chan_list_cfg[1].direc_t = phy_chan_cfg_s::direct_opts::biDirection;

      //     // phy_chan_cfg_s& phy_chan3 = rr_cfg_ded.phy_chan_list_cfg[2];
      //     // rr_cfg_ded.phy_chan_list_cfg[2].band_id_present = true;
      //     // rr_cfg_ded.phy_chan_list_cfg[2].freq_id_present = true;
      //     // rr_cfg_ded.phy_chan_list_cfg[2].slot_ass_present = true;
      //     // rr_cfg_ded.phy_chan_list_cfg[2].sche_type_present = true;
      //     // rr_cfg_ded.phy_chan_list_cfg[2].sche_type = phy_chan_cfg_s::sche_type_opts::Static;
      //     // rr_cfg_ded.phy_chan_list_cfg[2].s_rnti.srnti.from_string("000101");
      //     // rr_cfg_ded.phy_chan_list_cfg[2].chan_type = phy_chan_cfg_s::chan_type_opts::pSCH51;
      //     // rr_cfg_ded.phy_chan_list_cfg[2].band_id.ba_id.from_string("001000");
      //     // rr_cfg_ded.phy_chan_list_cfg[2].freq_id.freq_id.from_string("00");
      //     // rr_cfg_ded.phy_chan_list_cfg[2].slot_ass.from_string("11110");
      //     // rr_cfg_ded.phy_chan_list_cfg[2].direc_t = phy_chan_cfg_s::direct_opts::biDirection;
      //   }
      // }

      // if (wx_area_mode == 1)
      // {
      //   rr_cfg_ded.phy_chan_list_cfg.resize(2);

      //   rr_cfg_ded.phy_chan_list_cfg[0].band_id_present = true;
      //   rr_cfg_ded.phy_chan_list_cfg[0].freq_id_present = true;
      //   rr_cfg_ded.phy_chan_list_cfg[0].slot_ass_present = true;
      //   rr_cfg_ded.phy_chan_list_cfg[0].s_rnti.srnti.from_string("000110");
      //   rr_cfg_ded.phy_chan_list_cfg[0].chan_type = phy_chan_cfg_s::chan_type_opts::pDCH11;
      //   rr_cfg_ded.phy_chan_list_cfg[0].band_id.ba_id.from_string("001110");
      //   rr_cfg_ded.phy_chan_list_cfg[0].freq_id.freq_id.from_string("01");
      //   if (rr_cfg_ded.phy_chan_list_cfg[0].chan_type == 1 || rr_cfg_ded.phy_chan_list_cfg[0].chan_type == 3 || rr_cfg_ded.phy_chan_list_cfg[0].chan_type == 5)
      //   {
      //     rr_cfg_ded.phy_chan_list_cfg[0].slot_ass.from_string("00100"); // psch5-1/pdch1-1/psch1-1=00100
      //   }
      //   if (rr_cfg_ded.phy_chan_list_cfg[0].chan_type == 2 || rr_cfg_ded.phy_chan_list_cfg[0].chan_type == 4 || rr_cfg_ded.phy_chan_list_cfg[0].chan_type == 6)
      //   {
      //     rr_cfg_ded.phy_chan_list_cfg[0].slot_ass.from_string("11000"); // psch1-2/psch_5-2/pdch1-2=11000
      //   }

      //   rr_cfg_ded.phy_chan_list_cfg[0].direc_t = phy_chan_cfg_s::direct_opts::ulDirection;

      //   if (rr_cfg_ded.phy_chan_list_cfg[0].chan_type != 1 && rr_cfg_ded.phy_chan_list_cfg[0].chan_type != 2)
      //   {
      //     rr_cfg_ded.phy_chan_list_cfg[0].sche_type_present = true;
      //     rr_cfg_ded.phy_chan_list_cfg[0].sche_type = phy_chan_cfg_s::sche_type_opts::Static;
      //   }

      //   rr_cfg_ded.phy_chan_list_cfg[1].band_id_present = true;
      //   rr_cfg_ded.phy_chan_list_cfg[1].freq_id_present = true;
      //   rr_cfg_ded.phy_chan_list_cfg[1].slot_ass_present = true;
      //   rr_cfg_ded.phy_chan_list_cfg[1].s_rnti.srnti.from_string("000110");
      //   rr_cfg_ded.phy_chan_list_cfg[1].chan_type = phy_chan_cfg_s::chan_type_opts::dSPDTCH1;
      //   rr_cfg_ded.phy_chan_list_cfg[1].band_id.ba_id.from_string("000110");
      //   rr_cfg_ded.phy_chan_list_cfg[1].freq_id.freq_id.from_string("01");
      //   rr_cfg_ded.phy_chan_list_cfg[1].slot_ass.from_string("01000"); // dSPDTCHT need to modify to 01000
      //   rr_cfg_ded.phy_chan_list_cfg[1].pdtch_code_present = true;
      //   rr_cfg_ded.phy_chan_list_cfg[1].pdtch_code.pdt_phy_code = 21;
      //   rr_cfg_ded.phy_chan_list_cfg[1].direc_t = phy_chan_cfg_s::direct_opts::dlDirection;

      //   if (rr_cfg_ded.phy_chan_list_cfg[1].chan_type == 3 || rr_cfg_ded.phy_chan_list_cfg[1].chan_type == 4 || rr_cfg_ded.phy_chan_list_cfg[1].chan_type == 5 || rr_cfg_ded.phy_chan_list_cfg[1].chan_type == 6)
      //   {
      //     rr_cfg_ded.phy_chan_list_cfg[1].sche_type_present = true;
      //     rr_cfg_ded.phy_chan_list_cfg[1].sche_type = phy_chan_cfg_s::sche_type_opts::Static;
      //   }
      // }

      // // securityConfig
      // secu_cfg_s &sec_cfg = rr_cfg_ded.secu_cfg;
      // sec_cfg.sec_alg_cfg.integ_prot_alg_present = true;
      // sec_cfg.sec_alg_cfg.coph_alg = sec_alg_cfg_s::ciph_alg_opts::nea0;
      // sec_cfg.sec_alg_cfg.intef_prot_alg = sec_alg_cfg_s::intef_prot_alg_opts::nia0;

      // // zyg
      // // for MAC lcid map
      // // parent->mac->updateMap(1,static_cast<ChanType_t>(rr_cfg_ded.phy_chan_list_cfg[0].chan_type.value));

      // // configMap DRB_configMap;
      // if (drb_add.pdcp_cfg.rlc_am_present == true)
      // {

      //   DRB_configMap.ul_Type = static_cast<ChanType_t>(rr_cfg_ded.phy_chan_list_cfg[0].chan_type.value);
      //   if (wx_area_mode == 0)
      //   {
      //     std::cout << " wx_area_mode 0 " << std::endl;
      //     DRB_configMap.dl_Type = static_cast<ChanType_t>(rr_cfg_ded.phy_chan_list_cfg[0].chan_type.value);
      //   }
      //   else
      //   {
      //     std::cout << " wx_area_mode 1 " << std::endl;
      //     DRB_configMap.dl_Type = static_cast<ChanType_t>(rr_cfg_ded.phy_chan_list_cfg[1].chan_type.value);
      //   }
      //   DRB_configMap.voicetype = N_Voice;
      //   DRB_configMap.rlcType = rlc_am;
      //   DRB_configMap.dataType = Data;
      //   parent->mac->addlcidMap(drb_add.drb_id, DRB_configMap);
      // }
      // else if (drb_add.pdcp_cfg.rlc_um_present == true)
      // {
      //   DRB_configMap.ul_Type = static_cast<ChanType_t>(rr_cfg_ded.phy_chan_list_cfg[0].chan_type.value);
      //   if (wx_area_mode == 0)
      //   {
      //     std::cout << " wx_area_mode 0 " << std::endl;
      //     DRB_configMap.dl_Type = static_cast<ChanType_t>(rr_cfg_ded.phy_chan_list_cfg[0].chan_type.value);
      //   }
      //   else
      //   {
      //     std::cout << " wx_area_mode 1 " << std::endl;
      //     DRB_configMap.dl_Type = static_cast<ChanType_t>(rr_cfg_ded.phy_chan_list_cfg[1].chan_type.value);
      //   }
      //   DRB_configMap.voicetype = N_Voice;
      //   DRB_configMap.rlcType = rlc_um;
      //   DRB_configMap.dataType = Data;
      //   parent->mac->addlcidMap(drb_add.drb_id, DRB_configMap);
      // }

      // srsran::sdap_allocate sdap_dl_cfg;
      // sdap_dl_cfg.pdu_session_id = drb_add.sdap_cfg.pdu_sess_id.pdu_ses_id;
      // sdap_dl_cfg.qfi_add = drb_add.sdap_cfg.map_qos_flows_to_add[0].qfi;
      // sdap_dl_cfg.is_default_drb = drb_add.sdap_cfg.default_drb;
      // sdap_dl_cfg.sdap_head_dl = (srsran::sdap_header_t)(int)drb_add.sdap_cfg.sdap_header_dl;
      // sdap_dl_cfg.sdap_head_ul = (srsran::sdap_header_t)(int)drb_add.sdap_cfg.sdap_header_ul;
      // sdap_dl_cfg.drb_id = drb_add.drb_id;

      // parent->pdcp->sdap_cfgerer(rnti, drb_add.drb_id, sdap_dl_cfg);
    }
    if (am_tm_type == 1)
    {
      std::cout << "test wcb 124871" << std::endl;
      recofg_si.rrc_tran_iden = 1;

      recofg_si.rrc_con_recfg_r1.redio_resour_cfg_dedi_present = true;
      redio_resour_cfg_dedi_s &rr_cfg_ded = recofg_si.rrc_con_recfg_r1.redio_resour_cfg_dedi;

      rr_cfg_ded.drb_to_add_mod_list_present = true;

      rr_cfg_ded.phy_chan_list_cfg_present = true;
      rr_cfg_ded.secu_cfg_present = true;

      rr_cfg_ded.drb_to_add_mod_list.resize(1);
      drb_to_add_modi_s &drb_add = rr_cfg_ded.drb_to_add_mod_list[0];
      drb_add.sdap_cfg_present = true;
      drb_add.pdcp_cfg_present = true;
      drb_add.rlc_cfg_present = true;
      drb_add.log_chan_cfg_present = true;
      // sdap
      drb_add.sdap_cfg.map_qos_flows_to_add_present = true;
      drb_add.sdap_cfg.pdu_sess_id.pdu_ses_id = pdu_ses_id;
      drb_add.sdap_cfg.sdap_header_dl = sdap_cfg_s::sdap_header_dl_opts::absent;
      drb_add.sdap_cfg.sdap_header_ul = sdap_cfg_s::sdap_header_ul_opts::absent;
      drb_add.sdap_cfg.default_drb = false;
      drb_add.sdap_cfg.map_qos_flows_to_add.resize(1);
      drb_add.sdap_cfg.map_qos_flows_to_add[0].qfi = qfi_nas;

      drb_add.drb_id = 5;

      // pdcp
      drb_add.pdcp_cfg.dis_timer_present = true;
      drb_add.pdcp_cfg.rlc_tm_present = true;

      drb_add.pdcp_cfg.dis_timer = pdcp_cofg_s::dis_timer_opts::infinity;
      drb_add.pdcp_cfg.header_com.set_not_used_l();

      // rlc
      drb_add.rlc_cfg.set_tm();
      tm_s &tms = drb_add.rlc_cfg.set_tm();

      // LogicalChannelConfig
      drb_add.log_chan_cfg.ul_spec_para_present = true;
      ul_spec_para_s &ul_sp_para = drb_add.log_chan_cfg.ul_spec_para;
      ul_sp_para.log_chan_goup_present = true;
      // ul_sp_para.priority = 7;
      // ul_sp_para.prio_bit_rate = ul_spec_para_s::prio_bit_rate_opts::kBps2dot4;
      // ul_sp_para.buck_size_dura = ul_spec_para_s::buck_size_dura_opts::ms300;
      // ul_sp_para.log_chan_goup = 3;

      std::cout << "  voice type 2.4k " << std::endl;
      ul_sp_para.priority = 1;
      ul_sp_para.prio_bit_rate = ul_spec_para_s::prio_bit_rate_opts::kBps16;
      ul_sp_para.buck_size_dura = ul_spec_para_s::buck_size_dura_opts::ms60;
      ul_sp_para.log_chan_goup = 1;

      if (wx_area_mode == 0)
      {
        rr_cfg_ded.phy_chan_list_cfg.resize(1);

        phy_chan_cfg_s &phy_chan = rr_cfg_ded.phy_chan_list_cfg[0];
        phy_chan.band_id_present = true;
        phy_chan.freq_id_present = true;
        phy_chan.slot_ass_present = true;
        phy_chan.voice_type_present = true;

        phy_chan.s_rnti.srnti.from_string("000010");
        phy_chan.chan_type = phy_chan_cfg_s::chan_type_opts::pDCH11;
        phy_chan.band_id.ba_id.from_string("001011");
        phy_chan.freq_id.freq_id.from_string("01");
        if (phy_chan.chan_type == 1)
        {
          phy_chan.slot_ass.from_string("00100");
        }
        else if (phy_chan.chan_type == 2)
        {
          phy_chan.slot_ass.from_string("11000");
        }
        phy_chan.direc_t = phy_chan_cfg_s::direct_opts::biDirection;
        phy_chan.voice_type = phy_chan_cfg_s::voice_type_opts::kbps2point4; // 2.4k voice
      }
      else if (wx_area_mode == 1)
      {
        rr_cfg_ded.phy_chan_list_cfg.resize(2);

        rr_cfg_ded.phy_chan_list_cfg[0].band_id_present = true;
        rr_cfg_ded.phy_chan_list_cfg[0].freq_id_present = true;
        rr_cfg_ded.phy_chan_list_cfg[0].slot_ass_present = true;

        rr_cfg_ded.phy_chan_list_cfg[0].s_rnti.srnti.from_string("000110");
        rr_cfg_ded.phy_chan_list_cfg[0].chan_type = phy_chan_cfg_s::chan_type_opts::pDCH11;
        rr_cfg_ded.phy_chan_list_cfg[0].band_id.ba_id.from_string("000111");
        rr_cfg_ded.phy_chan_list_cfg[0].freq_id.freq_id.from_string("01");
        if (rr_cfg_ded.phy_chan_list_cfg[0].chan_type == 1 || rr_cfg_ded.phy_chan_list_cfg[0].chan_type == 3 || rr_cfg_ded.phy_chan_list_cfg[0].chan_type == 5)
        {
          rr_cfg_ded.phy_chan_list_cfg[0].slot_ass.from_string("00100"); // psch5-1/pdch1-1/psch1-1=00100
        }
        if (rr_cfg_ded.phy_chan_list_cfg[0].chan_type == 2 || rr_cfg_ded.phy_chan_list_cfg[0].chan_type == 4 || rr_cfg_ded.phy_chan_list_cfg[0].chan_type == 6)
        {
          rr_cfg_ded.phy_chan_list_cfg[0].slot_ass.from_string("11000"); // psch1-2/psch_5-2/pdch1-2=11000
        }

        rr_cfg_ded.phy_chan_list_cfg[0].direc_t = phy_chan_cfg_s::direct_opts::ulDirection;

        if (rr_cfg_ded.phy_chan_list_cfg[0].chan_type != 1 && rr_cfg_ded.phy_chan_list_cfg[0].chan_type != 2)
        {
          rr_cfg_ded.phy_chan_list_cfg[0].sche_type_present = true;
          rr_cfg_ded.phy_chan_list_cfg[0].sche_type = phy_chan_cfg_s::sche_type_opts::Static;
        }

        rr_cfg_ded.phy_chan_list_cfg[1].band_id_present = true;
        rr_cfg_ded.phy_chan_list_cfg[1].freq_id_present = true;
        rr_cfg_ded.phy_chan_list_cfg[1].slot_ass_present = true;
        rr_cfg_ded.phy_chan_list_cfg[1].s_rnti.srnti.from_string("000110");
        rr_cfg_ded.phy_chan_list_cfg[1].chan_type = phy_chan_cfg_s::chan_type_opts::dSPDTCHT;
        // rr_cfg_ded.phy_chan_list_cfg[1].band_id.ba_id.from_string("000110");
        rr_cfg_ded.phy_chan_list_cfg[1].band_id.ba_id.from_string("001100");
        rr_cfg_ded.phy_chan_list_cfg[1].freq_id.freq_id.from_string("01");
        rr_cfg_ded.phy_chan_list_cfg[1].slot_ass.from_string("01000"); // dSPDTCHT need to modify to 01000
        rr_cfg_ded.phy_chan_list_cfg[1].pdtch_code_present = true;
        rr_cfg_ded.phy_chan_list_cfg[1].pdtch_code.pdt_phy_code = 21;
        rr_cfg_ded.phy_chan_list_cfg[1].direc_t = phy_chan_cfg_s::direct_opts::dlDirection;

        if (rr_cfg_ded.phy_chan_list_cfg[1].chan_type == 3 || rr_cfg_ded.phy_chan_list_cfg[1].chan_type == 4 || rr_cfg_ded.phy_chan_list_cfg[1].chan_type == 5 || rr_cfg_ded.phy_chan_list_cfg[1].chan_type == 6)
        {
          rr_cfg_ded.phy_chan_list_cfg[1].sche_type_present = true;
          rr_cfg_ded.phy_chan_list_cfg[1].sche_type = phy_chan_cfg_s::sche_type_opts::Static;
        }

        std::cout << "rr_cfg_ded.phy_chan_list_cfg[1].chan_type.value=" << std::endl;
        // DS voice
        if (rr_cfg_ded.phy_chan_list_cfg[1].chan_type.value == 7 || rr_cfg_ded.phy_chan_list_cfg[1].chan_type == 10)
        {
          rr_cfg_ded.phy_chan_list_cfg[0].voice_type_present = true;
          rr_cfg_ded.phy_chan_list_cfg[0].voice_type = phy_chan_cfg_s::voice_type_opts::kbps2point4;
        }
      }

      // 2.4k voice
      // phy_chan.voice_type = phy_chan_cfg_s::voice_type_opts::kbps4point8; //4.8k voice

      if (parent->rrc_adp->udp_.is_Control == true)
      {
        switch (parent->rrc_adp->udp_.voice_indicate)
        {
        case 0x01:
          rr_cfg_ded.phy_chan_list_cfg[0].voice_type = phy_chan_cfg_s::voice_type_opts::kbps2point4; // 2.4k voice
          break;
        case 0x02:
          rr_cfg_ded.phy_chan_list_cfg[0].voice_type = phy_chan_cfg_s::voice_type_opts::kbps4point8;
          break;
        case 0x03:
          rr_cfg_ded.phy_chan_list_cfg[0].voice_type = phy_chan_cfg_s::voice_type_opts::bps800;
          break;
        }
      }

      if (ue_category == 14)
      {
        std::cout << " reconf ue_category==14 " << std::endl;
        rr_cfg_ded.phy_chan_list_cfg[0].voice_type = phy_chan_cfg_s::voice_type_opts::bps800;

        rr_cfg_ded.phy_chan_list_cfg[0].band_id.ba_id.from_number(ue_cap14_band_id);
        rr_cfg_ded.phy_chan_list_cfg[0].freq_id.freq_id.from_number(ue_cap14_freq_id);
        rr_cfg_ded.phy_chan_list_cfg[0].slot_ass.from_number(ue_cap14_slot);
      }

      std::cout << " phy_chan.voice_type = " << rr_cfg_ded.phy_chan_list_cfg[0].voice_type.to_string();

      // phy_chan.voice_type = phy_chan_cfg_s::voice_type_opts::kbps4point8; //4.8k voice
      // zhj

      srsran::sdap_allocate sdap_dl_cfg;
      sdap_dl_cfg.pdu_session_id = drb_add.sdap_cfg.pdu_sess_id.pdu_ses_id;
      sdap_dl_cfg.qfi_add = drb_add.sdap_cfg.map_qos_flows_to_add[0].qfi;
      sdap_dl_cfg.is_default_drb = drb_add.sdap_cfg.default_drb;
      sdap_dl_cfg.sdap_head_dl = (srsran::sdap_header_t)(int)drb_add.sdap_cfg.sdap_header_dl;
      sdap_dl_cfg.sdap_head_ul = (srsran::sdap_header_t)(int)drb_add.sdap_cfg.sdap_header_ul;
      sdap_dl_cfg.drb_id = drb_add.drb_id;

      parent->pdcp->sdap_cfgerer(rnti, drb_add.drb_id, sdap_dl_cfg);

      // securityConfig
      secu_cfg_s &sec_cfg = rr_cfg_ded.secu_cfg;
      sec_cfg.sec_pay_load_Present = true;
      sec_cfg.sec_alg_cfg.coph_alg = sec_alg_cfg_s::ciph_alg_opts::nea0;
      sec_pay_load_normal_s &sec_pay_load = sec_cfg.sec_pay_load.set_sec_pay_load_normal();
      // sec_pay_load.sec_pay_load_nor.from_string("111111111111111111111111111111111111111");
      sec_pay_load.sec_pay_load_nor.from_string("0000000000000000000000000000000");

      // parent->mac->getVoiceLcid(drb_add.drb_id,voice_speed);
      // configMap DRB_configMap;
      DRB_configMap.ul_Type = static_cast<ChanType_t>(rr_cfg_ded.phy_chan_list_cfg[0].chan_type.value);
      if (wx_area_mode == 0)
      {
        DRB_configMap.dl_Type = static_cast<ChanType_t>(rr_cfg_ded.phy_chan_list_cfg[0].chan_type.value);
      }
      else if (wx_area_mode == 1)
      {
        DRB_configMap.dl_Type = static_cast<ChanType_t>(rr_cfg_ded.phy_chan_list_cfg[1].chan_type.value);
      }
      std::cout << "  DRB_configMap.ul_Type = " << DRB_configMap.ul_Type << std::endl;

      DRB_configMap.voicetype = static_cast<VoiceType>(rr_cfg_ded.phy_chan_list_cfg[0].voice_type.value);
      std::cout << " DRB_configMap.voicetype = " << DRB_configMap.voicetype << std::endl;

      DRB_configMap.rlcType = rlc_tm;
      DRB_configMap.dataType = Voice;
      parent->mac->addlcidMap(drb_add.drb_id, DRB_configMap);
    }
    if (am_tm_type == 2)
    {

      printf("111dhsfg\n");
      recofg_si.rrc_tran_iden = 0;

      recofg_si.rrc_con_recfg_r1.meas_cfg_present = true;
      meas_cofg_s &mea_cfg = recofg_si.rrc_con_recfg_r1.meas_cfg;
      mea_cfg.meas_gap_cfg_list_present = true;
      mea_cfg.s_mea_sure_present = true;

      recofg_si.rrc_con_recfg_r1.redio_resour_cfg_dedi_present = true;
      redio_resour_cfg_dedi_s &rr_cfg_ded = recofg_si.rrc_con_recfg_r1.redio_resour_cfg_dedi;

      printf("111dhsfg\n");
      rr_cfg_ded.drb_to_rel_list_present = true;

      rr_cfg_ded.drb_to_rel_list.resize(1);

      drb_id_s &drb_rel = rr_cfg_ded.drb_to_rel_list[0];
      drb_rel.drb_id = 5;

      meas_gap_cfg_list_normal_s &mea_gap_normal = mea_cfg.meas_gap_cfg_list.set_meas_gap_cfg_list_normal();
      mea_gap_normal.resize(4);
      printf("111dhsfg\n");
      mea_gap_normal[0].beam_index = 1;
      mea_gap_normal[0].frame_offset = 1;
      mea_gap_normal[0].fcch_ba_id.ba_id.from_string("000110");
      mea_gap_normal[0].fcch_freq_id.freq_id.from_string("01");
      mea_gap_normal[0].meas_norm_ratio = meas_gap_cfg_normal_s::meas_norm_ratio_opts::dB9;

      mea_gap_normal[1].beam_index = 2;
      mea_gap_normal[1].frame_offset = 2;
      mea_gap_normal[1].fcch_ba_id.ba_id.from_string("000111");
      mea_gap_normal[1].fcch_freq_id.freq_id.from_string("01");
      mea_gap_normal[1].meas_norm_ratio = meas_gap_cfg_normal_s::meas_norm_ratio_opts::dB9;

      mea_gap_normal[2].beam_index = 3;
      mea_gap_normal[2].frame_offset = 3;
      mea_gap_normal[2].fcch_ba_id.ba_id.from_string("001000");
      mea_gap_normal[2].fcch_freq_id.freq_id.from_string("01");
      mea_gap_normal[2].meas_norm_ratio = meas_gap_cfg_normal_s::meas_norm_ratio_opts::dB9;

      mea_gap_normal[3].beam_index = 4;
      mea_gap_normal[3].frame_offset = 4;
      mea_gap_normal[3].fcch_ba_id.ba_id.from_string("001001");
      mea_gap_normal[3].fcch_freq_id.freq_id.from_string("01");
      mea_gap_normal[3].meas_norm_ratio = meas_gap_cfg_normal_s::meas_norm_ratio_opts::dB9;

      rssi_normal_s &rssi_nor = recofg_si.rrc_con_recfg_r1.meas_cfg.s_mea_sure.set_rssi_normal();
      rssi_nor.rssi_normal = 32;

      report_cfg_s &rep_cfg = recofg_si.rrc_con_recfg_r1.meas_cfg.report_cfg;
      rep_cfg.thr_hold_present = true;
      rep_cfg.report_geo_grap_info_present = true;
      rep_cfg.thr_hold.off_set = 1;
      rep_cfg.thr_hold.hysteresis = 1;
      rep_cfg.thr_hold.time_to_trigger = thr_hold_s::time_to_trigger_opts::ms0;
      rep_cfg.thr_hold.filter_coe_ent = thr_hold_s::filter_coe_ent_opts::fc0;
      rep_cfg.report_geo_grap_info = report_cfg_s::report_geo_grap_info_opts::True;
      printf("111dhsfg\n");
    }

    dl_dcch_msg_s dl_dcch_msg;
    rrc_conn_recfg_s &rrc_conn_recfg = dl_dcch_msg.msg.set_c1().set_rrc_conn_recfg();
    rrc_conn_recfg_r8_ies_s &recfg_r8 = rrc_conn_recfg.crit_exts.set_c1().set_rrc_conn_recfg_r8();
    // Fill RR Config Ded
    if (s_apply_reconf_updates(recfg_r8, recfg_r1, cur_ue_cfg, parent->cfg, ue_cell_list, bearer_list, ue_capabilities))
    {
      parent->logger.error("Generating ConnectionReconfiguration. Aborting...");
      return;
    }

    /*if (s_apply_reconf_updates(recfg_r1, cur_ue_cfg, parent->cfg, bearer_list)) {
      parent->logger.error("Generating ConnectionReconfiguration. Aborting...");
      return;
    }*/

    if (not(recfg_r1.redio_resour_cfg_com_present || recfg_r1.meas_cfg_present || recfg_r1.mobility_contro_present || recfg_r1.redio_resour_cfg_dedi_present ||
            recfg_r1.dedi_info_nas_n_present || recfg_r1.redio_resour_cfg_com_present ||
            recfg_r1.security_cfg_ho_present))
    {

      std::cout << "test wcb 346374637" << std::endl;
      return;
    }

    // Fill in NAS PDU - Only for RRC Connection Reconfiguration during E-RAB Release Command
    if (nas_pdu.size() > 0 && !recfg_r1.dedi_info_nas_n_present)
    {
      recfg_r1.dedi_info_nas_n_present = true;
      // Add NAS PDU
      printf("wcb----test11111\n");

      recfg_r1.dedi_info_nas_n.resize(1);
      recfg_r1.dedi_info_nas_n[0].dedi_info_nas_n1.resize(nas_pdu.size());
      memcpy(recfg_r1.dedi_info_nas_n[0].dedi_info_nas_n1.data(), nas_pdu.data(), nas_pdu.size());

      // for (uint32_t idx = 0; idx < recfg_r1.redio_resour_cfg_dedi.drb_to_rel_list.size(); idx++) {
      //   recfg_r1.dedi_info_nas_n[idx].dedi_info_nas_n1.resize(nas_pdu.size());
      //   memcpy(recfg_r1.dedi_info_nas_n[idx].dedi_info_nas_n1.data(), nas_pdu.data(), nas_pdu.size());
      // }
    }

    std::cout << " zhj test 11 " << std::endl;
    // s_apply_rlc_rb_updates(recofg_si.rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0]);
    xw_apply_rlc_rb_updates(recofg_si.rrc_con_recfg_r1.redio_resour_cfg_dedi);
    s_apply_pdcp_srb_updates();
    // s_apply_pdcp_drb_updates(recofg_si.rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0]);

    // if (ss_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list.size() > 1)
    // {
    //   s_apply_rlc_rb_updates(recofg_si.rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[1]);
    //   // xw_apply_rlc_rb_updates(recofg_si.rrc_con_recfg_r1.redio_resour_cfg_dedi);
    //   s_apply_pdcp_srb_updates();
    //   s_apply_pdcp_drb_updates(recofg_si.rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[1]);
    // }
    xw_apply_pdcp_drb_updates(recofg_si.rrc_con_recfg_r1.redio_resour_cfg_dedi);

    //**********************************************************************************************//
    // update Control Map Info
    SRB_configMap.ul_Type = static_cast<ChanType_t>(recofg_si.rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
    if (wx_area_mode == 0)
    {
      SRB_configMap.dl_Type = static_cast<ChanType_t>(recofg_si.rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
    }
    else
    {
      SRB_configMap.dl_Type = static_cast<ChanType_t>(recofg_si.rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type.value);
    }
    std::cout << " SRB_configMap.ul_Type = " << SRB_configMap.ul_Type << std::endl;
    std::cout << " SRB_configMap.dl_Type = " << SRB_configMap.dl_Type << std::endl;
    SRB_configMap.dataType = Control;  // control
    SRB_configMap.rlcType = rlc_am;    // AM
    SRB_configMap.voicetype = N_Voice; // NO Voice
                                       //**********************************************************************************************//
    std::cout << "@@@@ x Chan_Type =" << ss_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.to_string() << std::endl;
    // Reuse same PDU
    if (pdu != nullptr)
    {
      pdu->clear();
    }
    std::cout << "recofg_si.rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list_present: " << recofg_si.rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list_present << std::endl;
    std::string octet_str;
    s_sends_dl_dcch(&ss_dl_dcch_msg, std::move(pdu), &octet_str);
    state = RRC_STATE_WAIT_FOR_CON_RECONF_COMPLETE;
  }

  void rrc::ue::send_rrc_con_reconf_data(
      uint16_t rnti,
      uint8_t qfi_nas,
      uint16_t pdu_ses_id,
      int am_tm_type,
      srsran::unique_byte_buffer_t pdu,
      srsran::const_byte_span nas_pdu)
  {
    std::cout << "[Reconfig][Data]qfi_nas=" << (int)qfi_nas << std::endl;
    std::cout << "[Reconfig][Data]pdu_ses_id=" << pdu_ses_id << std::endl;
    update_scells();
    std::cout << " [RRC][RECONF][DATA]filename:" << parent->cfg.reconf_data_config << std::endl;
    // std::string multi_cc_readenb_recfg_filename = "/opt/AccessIot/access/beam1/enb1.conf";
    std::string multi_cc_readenb_recfg_filename = parent->cfg.config_file;
    std::cout << " [RRC][RECONF]wx_read_enb_recfg_data[DATA]filename: " << multi_cc_readenb_recfg_filename << std::endl;
    if (!wx_read_enb_recfg_data_reconfig(multi_cc_readenb_recfg_filename))
    {
      std::cerr << " [RRC][RECONF][DATA]read " << multi_cc_readenb_recfg_filename << " file failure!" << std::endl;
    }
    std::cout << " parent->cfg.multi_beam_num[RRcfilename:" << parent->cfg.multi_beam_num << std::endl;

    if (!wx_read_data_reconfig(parent->cfg.reconf_data_config))
    {
      std::cerr << " [RRC][RECONF][DATA]read " << parent->cfg.reconf_data_config << " file failure!" << std::endl;
    }

    std::cout << "[RRC][RECONF][DATA]read file end!" << std::endl;
    std::cout << "[RRC][RECONF][DATA][MCS]:" << RRC_mcs_Info.MCS << std::endl;
    std::cout << "[RRC][RECONF][DATA][is_data_service_switching]:" << RRC_mcs_Info.is_data_service_switching << std::endl;
    RRC_mcs_Info.is_data_service_switching = false;
    s_dl_dcch_msg_s s_dl_dcch_msg = parent->cfg.recfg_wx;

    std::cout<<"xx parent->cfg.ttcn_test_enble = "<<parent->cfg.ttcn_test_enble<<std::endl;
    if (parent->cfg.ttcn_test_enble)
    {
    }
    else
    {
      if (is_rlc_mode_change)
      {
        printf("is_rlc_mode_change: delete drb id 3\n");
        s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_rel_list.resize(1);
        s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_rel_list_present =true;
        s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_rel_list[0].drb_id = 3;
        s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list_present =false;
      }
      else{
        if (rlc_mode == 1 || (s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.rlc_am_present == false &&
            s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.rlc_um_present == true))
        {
          printf("current rlc mode:%d,um_present:%d\n",rlc_mode,s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.rlc_um_present);
          s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].rlc_cfg.set_um_bi_dir();
          um_bi_dir_s_ &ums = s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].rlc_cfg.set_um_bi_dir();
          ums.dl_um_rlc.t_reord = dl_um_rlcc_s::t_reord_opts::ms480; // fixed
        }
        else if(rlc_mode == 0 || (s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.rlc_am_present == true &&
        s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.rlc_um_present == false))
        {
          printf("current rlc mode:%d and to am mode!\n",rlc_mode);
          s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.dis_timer_present = true;
          s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.integ_pro_present = true;
          s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.dis_timer = pdcp_cofg_s::dis_timer_opts::infinity;
          s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.header_com.set_not_used_l();
          s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.integ_pro = pdcp_cofg_s::integ_pro_opts::enabled;

          s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.rlc_am.stat_rep_req = true;

          s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].rlc_cfg.set_am();
          am_s_ &ams = s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].rlc_cfg.set_am();
          ams.ul_am_rlc.t_poll_retran = ul_am_rlcc_s::t_poll_retran_opts::ms480;
          ams.ul_am_rlc.poll_pdu = ul_am_rlcc_s::poll_pdu_opts::p32;
          // ams.ul_am_rlc.poll_pdu = ul_am_rlcc_s::poll_pdu_opts::pInfinity;
          ams.ul_am_rlc.poll_byte = ul_am_rlcc_s::poll_byte_opts::kB128;
          // ams.ul_am_rlc.poll_byte = ul_am_rlcc_s::poll_byte_opts::kBInfinity;
          ams.ul_am_rlc.max_retx_thres_hold = ul_am_rlcc_s::max_retx_thres_hold_opts::t4;
          ams.dl_am_rlc.t_reord = dl_am_rlcc_s::t_reord_opts::ms480;
          ams.dl_am_rlc.t_status_proh = dl_am_rlcc_s::t_status_proh_opts::ms420;

        }
        else
        {
          s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].rlc_cfg.set_tm();
        }
      }

      

    }

    if (parent->rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 6115)
    {
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = phy_chan_cfg_s::chan_type_opts::pSCH51;
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].sche_type = phy_chan_cfg_s::sche_type_opts::dynamic;
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.rlc_am_present = false;
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.rlc_um_present = true;
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.from_string("00100");
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].band_id.ba_id.from_number(20);
      std::cout << "rlc_um_present333" << std::endl;
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].rlc_cfg.set_um_bi_dir();
      um_bi_dir_s_ &ums = s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].rlc_cfg.set_um_bi_dir();
      ums.dl_um_rlc.t_reord = dl_um_rlcc_s::t_reord_opts::ms480;
    }
    else if (parent->rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 6116)
    {
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = phy_chan_cfg_s::chan_type_opts::pDCH11;
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].sche_type = phy_chan_cfg_s::sche_type_opts::Static;
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.rlc_am_present = false;
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.rlc_um_present = true;
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.from_string("00100");
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].band_id.ba_id.from_number(16);
      std::cout << "rlc_um_present" << std::endl;
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].rlc_cfg.set_um_bi_dir();
      um_bi_dir_s_ &ums = s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].rlc_cfg.set_um_bi_dir();
      ums.dl_um_rlc.t_reord = dl_um_rlcc_s::t_reord_opts::ms480;
    }
    else if (parent->rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 6117)
    {
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = phy_chan_cfg_s::chan_type_opts::pDCH11;
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].sche_type = phy_chan_cfg_s::sche_type_opts::Static;
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.rlc_am_present = false;
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.rlc_um_present = true;
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.from_string("00100");
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].band_id.ba_id.from_number(16);
      std::cout << "rlc_um_present" << std::endl;
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].rlc_cfg.set_um_bi_dir();
      um_bi_dir_s_ &ums = s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].rlc_cfg.set_um_bi_dir();
      ums.dl_um_rlc.t_reord = dl_um_rlcc_s::t_reord_opts::ms480;
    }
    //*********TC6111 start*******
    else if(parent->rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 6111)
    {
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phr_cfg_present=true;
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phr_cfg.set_set_up().peri_phr_timer = set_up_s::peri_phr_timer_opts::sf50;
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phr_cfg.set_set_up().prohi_phr_time = set_up_s::prohi_phr_time_opts::sf0;
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phr_cfg.set_set_up().dl_path_loss_change = set_up_s::dl_path_loss_change_opts::infinity;
    }
    //*********TC6111 end*******

    //*********TC6112 start*******
    else if(parent->rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 6111)
    {
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phr_cfg_present=true;
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phr_cfg.set_set_up().peri_phr_timer = set_up_s::peri_phr_timer_opts::infinity;
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phr_cfg.set_set_up().prohi_phr_time = set_up_s::prohi_phr_time_opts::sf100;
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phr_cfg.set_set_up().dl_path_loss_change = set_up_s::dl_path_loss_change_opts::dB3;
    }
    //*********TC6112 end*******

    std::cout << "[RRC][RECONF][DATA]channel:" << s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.to_string();

    srnti = s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].s_rnti.srnti.to_number();
    parent->mac->getSrnti(srnti);

    dl_dcch_msg_s dl_dcch_msg;
    rrc_conn_recfg_s &rrc_conn_recfg = dl_dcch_msg.msg.set_c1().set_rrc_conn_recfg();
    rrc_conn_recfg_r8_ies_s &recfg_r8 = rrc_conn_recfg.crit_exts.set_c1().set_rrc_conn_recfg_r8();

    s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].sdap_cfg.map_qos_flows_to_add[0].qfi = qfi_nas;

    std::cout << "[RRC][RECONF][DATA][SLOT]=" << parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.to_number();

    if (s_apply_reconf_updates(recfg_r8, s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1, cur_ue_cfg, parent->cfg, ue_cell_list, bearer_list, ue_capabilities))
    {
      parent->logger.error("Generating ConnectionReconfiguration. Aborting...");
      return;
    }

    if (not(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_com_present || s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.meas_cfg_present || s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.mobility_contro_present || s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi_present ||
            s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.dedi_info_nas_n_present || s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_com_present ||
            s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.security_cfg_ho_present))
    {

      return;
    }

    // Fill in NAS PDU - Only for RRC Connection Reconfiguration during E-RAB Release Command
    if (nas_pdu.size() > 0 && !s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.dedi_info_nas_n_present)
    {
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.dedi_info_nas_n_present = true;
      // Add NAS PDU

      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.dedi_info_nas_n.resize(1);
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.dedi_info_nas_n[0].dedi_info_nas_n1.resize(nas_pdu.size());
      memcpy(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.dedi_info_nas_n[0].dedi_info_nas_n1.data(), nas_pdu.data(), nas_pdu.size());
    }
    uint8_t del_number = 0;
    if(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_rel_list_present)
    {
      printf("enter drb_to_rel_list_present count:%d!\n",del_number++);
      for (auto& drb_id : s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_rel_list) {
        parent->pdcp->del_bearer(rnti, drb_id.drb_id);
        printf("rnti=%d,drbID=%d!\n",rnti,drb_id.drb_id);
      }

      if (s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_rel_list.size() > 0) {
        for (auto& drb_id : s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_rel_list) {
          parent->rlc->del_bearer(rnti, drb_id.drb_id);
        }
      }
    }

    if (s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list_present)
    {
      s_apply_rlc_rb_updates(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0]);
    }
    s_apply_pdcp_srb_updates();
    s_apply_pdcp_drb_updates(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0]);


    //**********************************************************************************************//
    // configMap DRB_configMap;

    if (s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.rlc_am_present == true)
    {
      DRB_configMap.ul_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
      if (wx_area_mode == 0)
      {
        printf("[RRC_UE][send_rrc_con_reconf_data] Normal Mode - AM\n");
        DRB_configMap.dl_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
      }
      else
      {
        printf("[RRC_UE][send_rrc_con_reconf_data] DS Mode - AM\n");
        DRB_configMap.dl_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type.value);
      }
      DRB_configMap.voicetype = N_Voice;
      DRB_configMap.rlcType = rlc_am;
      DRB_configMap.dataType = Data;
      parent->mac->addlcidMap(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].drb_id, DRB_configMap);
    }
    else if (s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.rlc_um_present == true)
    {
      DRB_configMap.ul_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
      if (wx_area_mode == 0)
      {
        std::cout << "Normal Mode - UM" << std::endl;
        DRB_configMap.dl_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
      }
      else
      {
        std::cout << "DS Mode - UM" << std::endl;
        DRB_configMap.dl_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type.value);
      }
      DRB_configMap.voicetype = N_Voice;
      DRB_configMap.rlcType = rlc_um;
      DRB_configMap.dataType = Data;
      parent->mac->addlcidMap(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].drb_id, DRB_configMap);
    }

    // update Control Map Info
    SRB_configMap.ul_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
    if (wx_area_mode == 0)
    {
      SRB_configMap.dl_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
    }
    else
    {
      SRB_configMap.dl_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type.value);
    }
    std::cout << " SRB_configMap.ul_Type = " << SRB_configMap.ul_Type << std::endl;
    std::cout << " SRB_configMap.dl_Type = " << SRB_configMap.dl_Type << std::endl;
    SRB_configMap.dataType = Control;  // control
    SRB_configMap.rlcType = rlc_am;    // AM
    SRB_configMap.voicetype = N_Voice; // NO Voice
                                       //**********************************************************************************************//

    // Reuse same PDU
    if (pdu != nullptr)
    {
      pdu->clear();
    }
    std::cout << "recofg_si.rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list_present: " << s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list_present << std::endl;
    std::string octet_str;
    s_sends_dl_dcch(&s_dl_dcch_msg, std::move(pdu), &octet_str);

    state = RRC_STATE_WAIT_FOR_CON_RECONF_COMPLETE;
  }

  void rrc::ue::send_rrc_con_reconf_kuopin_data(
      uint16_t rnti,
      uint8_t qfi_nas,
      uint16_t pdu_ses_id,
      int am_tm_type,
      srsran::unique_byte_buffer_t pdu,
      srsran::const_byte_span nas_pdu)
  {

    update_scells();
    // std::string multi_cc_readenb_recfg_filename = parent->cfg.config_file;
    std::cout << " [RRC][RECONF]wx_read_enb_recfg_data[DATA]filename: " << parent->cfg.config_file << std::endl;
    if (!wx_read_enb_recfg_data_reconfig(parent->cfg.config_file))
    {
      std::cerr << " [RRC][RECONF][DATA][DS]read file failure!" << std::endl;
    }
    if (!wx_read_data_DS_reconfig(parent->cfg.reconf_data_ds_config))
    {
      std::cerr << " [RRC][RECONF][DATA][DS]read file failure!" << std::endl;
    }
    std::cerr << " [RRC][RECONF][DATA][DS]read file end!" << std::endl;

    std::cout << "[RRC][RECONF][DATA][DS][MCS]:" << RRC_mcs_Info.MCS << std::endl;
    std::cout << "[RRC][RECONF][DATA][DS][is_data_service_switching]:" << RRC_mcs_Info.is_data_service_switching << std::endl;
    RRC_mcs_Info.is_data_service_switching = false;

    // std::cout << "[TEST]slot:" << s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.to_number();
    s_dl_dcch_msg_s s_dl_dcch_msg = parent->cfg.recfg_norm_kuopin;
    
    if (parent->cfg.ttcn_test_enble)
    {
    }
    else
    {
      if (is_rlc_mode_change)
      {
        printf("is_rlc_mode_change: delete drb id 3\n");
        s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_rel_list.resize(1);
        s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_rel_list_present =true;
        s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_rel_list[0].drb_id = 3;
        s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list_present =false;
      }
      else{
        if (rlc_mode == 1 || (s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.rlc_am_present == false &&
            s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.rlc_um_present == true))
        {
          printf("current rlc mode:%d,um_present:%d\n",rlc_mode,s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.rlc_um_present);
          s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].rlc_cfg.set_um_bi_dir();
          um_bi_dir_s_ &ums = s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].rlc_cfg.set_um_bi_dir();
          ums.dl_um_rlc.t_reord = dl_um_rlcc_s::t_reord_opts::ms480; // fixed

          s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.rlc_um_present = true;
        }
        else if(rlc_mode == 0 || (s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.rlc_am_present == true &&
        s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.rlc_um_present == false))
        {
          printf("current rlc mode:%d and to am mode!\n",rlc_mode);
          s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.dis_timer_present = true;
          s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.integ_pro_present = true;
          s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.dis_timer = pdcp_cofg_s::dis_timer_opts::infinity;
          s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.header_com.set_not_used_l();
          s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.integ_pro = pdcp_cofg_s::integ_pro_opts::enabled;

          s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.rlc_am.stat_rep_req = true;

          s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].rlc_cfg.set_am();
          am_s_ &ams = s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].rlc_cfg.set_am();
          ams.ul_am_rlc.t_poll_retran = ul_am_rlcc_s::t_poll_retran_opts::ms480;
          ams.ul_am_rlc.poll_pdu = ul_am_rlcc_s::poll_pdu_opts::p32;
          // ams.ul_am_rlc.poll_pdu = ul_am_rlcc_s::poll_pdu_opts::pInfinity;
          ams.ul_am_rlc.poll_byte = ul_am_rlcc_s::poll_byte_opts::kB128;
          // ams.ul_am_rlc.poll_byte = ul_am_rlcc_s::poll_byte_opts::kBInfinity;
          ams.ul_am_rlc.max_retx_thres_hold = ul_am_rlcc_s::max_retx_thres_hold_opts::t4;
          ams.dl_am_rlc.t_reord = dl_am_rlcc_s::t_reord_opts::ms480;
          ams.dl_am_rlc.t_status_proh = dl_am_rlcc_s::t_status_proh_opts::ms420;

        }
        else
        {
          s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].rlc_cfg.set_tm();
        }
      }
    }
    
    std::cout << "xx [TEST]slot:" << s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.to_number();
    dl_dcch_msg_s dl_dcch_msg;
    rrc_conn_recfg_s &rrc_conn_recfg = dl_dcch_msg.msg.set_c1().set_rrc_conn_recfg();
    rrc_conn_recfg_r8_ies_s &recfg_r8 = rrc_conn_recfg.crit_exts.set_c1().set_rrc_conn_recfg_r8();

    s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].sdap_cfg.map_qos_flows_to_add[0].qfi = qfi_nas;


    std::cout << "[TEST]slot:" << s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.to_number();

    if (s_apply_reconf_updates(recfg_r8, s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1, cur_ue_cfg, parent->cfg, ue_cell_list, bearer_list, ue_capabilities))
    {
      parent->logger.error("Generating ConnectionReconfiguration. Aborting...");

      return;
    }
    if (not(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_com_present || s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.meas_cfg_present || s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.mobility_contro_present || s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi_present ||
            s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.dedi_info_nas_n_present || s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_com_present ||
            s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.security_cfg_ho_present))
    {
      return;
    }

    // Fill in NAS PDU - Only for RRC Connection Reconfiguration during E-RAB Release Command
    if (nas_pdu.size() > 0 && !s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.dedi_info_nas_n_present)
    {
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.dedi_info_nas_n_present = true;
      // Add NAS PDU
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.dedi_info_nas_n.resize(1);
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.dedi_info_nas_n[0].dedi_info_nas_n1.resize(nas_pdu.size());
      memcpy(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.dedi_info_nas_n[0].dedi_info_nas_n1.data(), nas_pdu.data(), nas_pdu.size());
    }

    uint8_t del_number = 0;
    if(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_rel_list_present)
    {
      printf("enter drb_to_rel_list_present count:%d!\n",del_number++);
      for (auto& drb_id : s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_rel_list) {
        parent->pdcp->del_bearer(rnti, drb_id.drb_id);
        printf("rnti=%d,drbID=%d!\n",rnti,drb_id.drb_id);
      }

      if (s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_rel_list.size() > 0) {
        for (auto& drb_id : s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_rel_list) {
          parent->rlc->del_bearer(rnti, drb_id.drb_id);
        }
      }
    }

    if (s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list_present)
    {
      s_apply_rlc_rb_updates(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0]);
    }

    // xw_apply_rlc_rb_updates(recofg_si.rrc_con_recfg_r1.redio_resour_cfg_dedi);
    s_apply_pdcp_srb_updates();
    s_apply_pdcp_drb_updates(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0]);
    // xw_apply_pdcp_drb_updates(recofg_si.rrc_con_recfg_r1.redio_resour_cfg_dedi);

    //**********************************************************************************************//
    // configMap DRB_configMap;

    if (s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.rlc_am_present == true)
    {
      DRB_configMap.ul_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
      if (wx_area_mode == 0)
      {
        std::cout << " wx_area_mode 0 " << std::endl;
        DRB_configMap.dl_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
      }
      else
      {
        std::cout << " wx_area_mode 1 " << std::endl;
        DRB_configMap.dl_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type.value);
      }
      DRB_configMap.voicetype = N_Voice;
      DRB_configMap.rlcType = rlc_am;
      DRB_configMap.dataType = Data;
      parent->mac->addlcidMap(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].drb_id, DRB_configMap);
    }
    else if (s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.rlc_um_present == true)
    {
      DRB_configMap.ul_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
      if (wx_area_mode == 0)
      {
        std::cout << " wx_area_mode 0 " << std::endl;
        DRB_configMap.dl_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
      }
      else
      {
        std::cout << " wx_area_mode 1 " << std::endl;
        DRB_configMap.dl_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type.value);
      }
      DRB_configMap.voicetype = N_Voice;
      DRB_configMap.rlcType = rlc_um;
      DRB_configMap.dataType = Data;
      parent->mac->addlcidMap(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].drb_id, DRB_configMap);
    }

    // update Control Map Info
    SRB_configMap.ul_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
    if (wx_area_mode == 0)
    {
      SRB_configMap.dl_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
    }
    else
    {
      SRB_configMap.dl_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type.value);
    }
    std::cout << " SRB_configMap.ul_Type = " << SRB_configMap.ul_Type << std::endl;
    std::cout << " SRB_configMap.dl_Type = " << SRB_configMap.dl_Type << std::endl;
    SRB_configMap.dataType = Control;  // control
    SRB_configMap.rlcType = rlc_am;    // AM
    SRB_configMap.voicetype = N_Voice; // NO Voice
                                       //**********************************************************************************************//

    // Reuse same PDU
    if (pdu != nullptr)
    {
      pdu->clear();
    }
    std::cout << "recofg_si.rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list_present: " << s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list_present << std::endl;
    std::string octet_str;
    s_sends_dl_dcch(&s_dl_dcch_msg, std::move(pdu), &octet_str);

    state = RRC_STATE_WAIT_FOR_CON_RECONF_COMPLETE;
  }

  void rrc::ue::send_rrc_con_voice_reconf(
      uint16_t rnti,
      uint8_t qfi_nas,
      uint16_t pdu_ses_id,
      srsran::unique_byte_buffer_t pdu,
      srsran::const_byte_span nas_pdu)
  {
    std::cout << "[RRC][VOICE]" << std::endl;
    RRC_mcs_Info.MCS = 0; //
    RRC_mcs_Info.is_data_service_switching = false;

    update_scells();
    s_dl_dcch_msg_s s_dl_dcch_msg;
    if (wx_area_mode == 0)
    {
      std::cout << "[RRC][VOICE][NORMAL]" << std::endl;
      std::cout << "[RRC][RECONF][VOICE]filename:" << parent->cfg.reconf_voice_config << std::endl;
      if (!wx_read_voice_reconfig(parent->cfg.reconf_voice_config))
      {
        std::cerr << "[RRC][RECONF][VOICE]read file failure!" << std::endl;
      }
      std::cerr << "[RRC][RECONF][VOICE]read file end!" << std::endl;
      s_dl_dcch_msg = parent->cfg.recfg_wx_voice;
    }
    else
    {
      std::cout << "[RRC][VOICE][DS]" << std::endl;
      std::cout << "[RRC][RECONF][VOICE][DS]filename:" << parent->cfg.reconf_voice_ds_config << std::endl;
      if (!wx_read_voice_DS_reconfig(parent->cfg.reconf_voice_ds_config))
      {
        std::cerr << "[RRC][RECONF][VOICE][DS]read file failure!" << std::endl;
      }
      std::cerr << "[RRC][RECONF][VOICE][DS]read file end!" << std::endl;
      s_dl_dcch_msg = parent->cfg.recfg_wx_kuopin;
    }
    s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.secu_cfg.sec_pay_load.set_sec_pay_load_normal().sec_pay_load_nor.from_string("0000000000000000000000000000000");

    dl_dcch_msg_s dl_dcch_msg;
    rrc_conn_recfg_s &rrc_conn_recfg = dl_dcch_msg.msg.set_c1().set_rrc_conn_recfg();
    rrc_conn_recfg_r8_ies_s &recfg_r8 = rrc_conn_recfg.crit_exts.set_c1().set_rrc_conn_recfg_r8();

    s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].sdap_cfg.map_qos_flows_to_add[0].qfi = qfi_nas;
    s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].sdap_cfg.pdu_sess_id.pdu_ses_id = pdu_ses_id;

    // if (s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type == 2 ||
    //     s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type == 4 ||
    //     s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type == 6)
    // {
    //   s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.from_string("01100"); // psch1-2/psch_5-2/pdch1-2=11000
    // }

    // Fill RR Config Ded
    if (parent->rrc_adp->udp_.is_Control == true)
    {
      switch (parent->rrc_adp->udp_.voice_indicate)
      {
      case 0x01:
        s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].voice_type = phy_chan_cfg_s::voice_type_opts::kbps2point4; // 2.4k voice
        break;
      case 0x02:
        s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].voice_type = phy_chan_cfg_s::voice_type_opts::kbps4point8;
        break;
      case 0x03:
        s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].voice_type = phy_chan_cfg_s::voice_type_opts::bps800;
        break;
      }
    }

    Voice_Type_record = s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].voice_type;
    std::cout << "Voice_Type_record:" << Voice_Type_record.to_string() << std::endl;

    if (wx_area_mode == 0)
    {
      std::cout << "[RRC][VOICE][NORMAL][VOICE_TYPE]:" << s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].voice_type.to_string() << std::endl;
    }
    else
    {
      std::cout << "[RRC][VOICE][DS][VOICE_TYPE:]" << s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].voice_type.to_string() << std::endl;
    }

    std::cout << "[RRC][VOICE][UECAP]:" << ue_category << std::endl;
    if (ue_category == 14)
    {
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].voice_type = phy_chan_cfg_s::voice_type_opts::bps800;

      // is_reconfig_uecap14_after_access = false; // use config parameters;

      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].band_id.ba_id.from_number(ue_cap14_band_id);
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].freq_id.freq_id.from_number(ue_cap14_freq_id);
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.from_number(ue_cap14_slot);
    }

    if (s_apply_reconf_updates(recfg_r8, s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1, cur_ue_cfg, parent->cfg, ue_cell_list, bearer_list, ue_capabilities))
    {
      parent->logger.error("Generating ConnectionReconfiguration. Aborting...");
      return;
    }

    if (not(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_com_present || s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.meas_cfg_present || s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.mobility_contro_present || s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi_present ||
            s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.dedi_info_nas_n_present || s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_com_present ||
            s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.security_cfg_ho_present))
    {
      return;
    }

    // Fill in NAS PDU - Only for RRC Connection Reconfiguration during E-RAB Release Command
    if (nas_pdu.size() > 0 && !s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.dedi_info_nas_n_present)
    {
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.dedi_info_nas_n_present = true;
      // Add NAS PDU
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.dedi_info_nas_n.resize(1);
      s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.dedi_info_nas_n[0].dedi_info_nas_n1.resize(nas_pdu.size());
      memcpy(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.dedi_info_nas_n[0].dedi_info_nas_n1.data(), nas_pdu.data(), nas_pdu.size());
    }
    s_apply_rlc_rb_updates(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0]);

    // xw_apply_rlc_rb_updates(recofg_si.rrc_con_recfg_r1.redio_resour_cfg_dedi);
    s_apply_pdcp_srb_updates();
    s_apply_pdcp_drb_updates(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0]);
    // xw_apply_pdcp_drb_updates(recofg_si.rrc_con_recfg_r1.redio_resour_cfg_dedi);

    //**********************************************************************************************//
    // configMap DRB_configMap;

    DRB_configMap.ul_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
    if (wx_area_mode == 0)
    {
      DRB_configMap.dl_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
    }
    else if (wx_area_mode == 1)
    {
      DRB_configMap.dl_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type.value);
    }
    std::cout << "[RRC][VOICE][DRB_CONF][UL_TYPE]:" << DRB_configMap.ul_Type << std::endl;
    std::cout << "[RRC][VOICE][DRB_CONF][DL_TYPE]:" << DRB_configMap.dl_Type << std::endl;
    DRB_configMap.voicetype = static_cast<VoiceType>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].voice_type.value);
    std::cout << "[RRC][VOICE][DRB_CONF][VOICE_TYPE]:" << DRB_configMap.voicetype << std::endl;
    DRB_configMap.rlcType = rlc_tm;
    DRB_configMap.dataType = Voice;
    parent->mac->addlcidMap(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].drb_id, DRB_configMap);

    // update Control Map Info
    SRB_configMap.ul_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
    if (wx_area_mode == 0)
    {
      SRB_configMap.dl_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
    }
    else
    {
      SRB_configMap.dl_Type = static_cast<ChanType_t>(s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type.value);
    }
    std::cout << "[RRC][VOICE][SRB_CONF][UL_TYPE]:" << SRB_configMap.ul_Type << std::endl;
    std::cout << "[RRC][VOICE][SRB_CONF][DL_TYPE]:" << SRB_configMap.dl_Type << std::endl;
    SRB_configMap.dataType = Control;  // control
    SRB_configMap.rlcType = rlc_am;    // AM
    SRB_configMap.voicetype = N_Voice; // NO Voice
                                       //**********************************************************************************************//
    // Reuse same PDU
    if (pdu != nullptr)
    {
      pdu->clear();
    }
    std::cout << "recofg_si.rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list_present: " << s_dl_dcch_msg.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list_present << std::endl;
    std::string octet_str;
    s_sends_dl_dcch(&s_dl_dcch_msg, std::move(pdu), &octet_str);
    state = RRC_STATE_WAIT_FOR_CON_RECONF_COMPLETE;
  }

  void rrc::ue::send_connection_reconf(srsran::unique_byte_buffer_t pdu,
                                       bool phy_cfg_updated,
                                       srsran::const_byte_span nas_pdu)
  {
    parent->logger.debug("RRC state %d", state);

    update_scells();

    /* Create RRCConnectionReconfiguration ASN1 message */
    dl_dcch_msg_s dl_dcch_msg;
    rrc_conn_recfg_s &rrc_conn_recfg = dl_dcch_msg.msg.set_c1().set_rrc_conn_recfg();
    rrc_conn_recfg.rrc_transaction_id = (uint8_t)((transaction_id++) % 4);
    rrc_conn_recfg_r8_ies_s &recfg_r8 = rrc_conn_recfg.crit_exts.set_c1().set_rrc_conn_recfg_r8();

    // Fill RR Config Ded and SCells
    if (apply_reconf_updates(
            recfg_r8, current_ue_cfg, parent->cfg, ue_cell_list, bearer_list, ue_capabilities, phy_cfg_updated))
    {
      parent->logger.error("Generating ConnectionReconfiguration. Aborting...");
      return;
    }

    // Add measConfig
    if (mobility_handler != nullptr)
    {
      mobility_handler->fill_conn_recfg_no_ho_cmd(&recfg_r8);
    }

    // if no updates were detected, skip rrc reconfiguration
    if (not(recfg_r8.rr_cfg_ded_present or recfg_r8.meas_cfg_present or recfg_r8.mob_ctrl_info_present or
            recfg_r8.ded_info_nas_list_present or recfg_r8.security_cfg_ho_present or recfg_r8.non_crit_ext_present))
    {
      return;
    }

    // Fill in NAS PDU - Only for RRC Connection Reconfiguration during E-RAB Release Command
    if (nas_pdu.size() > 0 and !recfg_r8.ded_info_nas_list_present)
    {
      recfg_r8.ded_info_nas_list_present = true;
      recfg_r8.ded_info_nas_list.resize(recfg_r8.rr_cfg_ded.drb_to_release_list.size());
      // Add NAS PDU
      for (uint32_t idx = 0; idx < recfg_r8.rr_cfg_ded.drb_to_release_list.size(); idx++)
      {
        recfg_r8.ded_info_nas_list[idx].resize(nas_pdu.size());
        memcpy(recfg_r8.ded_info_nas_list[idx].data(), nas_pdu.data(), nas_pdu.size());
      }
    }

    if (endc_handler != nullptr)
    {
      endc_handler->fill_conn_recfg(&recfg_r8);
    }

    /* Apply updates present in RRCConnectionReconfiguration to lower layers */
    // apply PHY config
    apply_reconf_phy_config(recfg_r8, true);

    // setup SRB2/DRBs in PDCP and RLC
    apply_rlc_rb_updates(recfg_r8.rr_cfg_ded);
    apply_pdcp_srb_updates(recfg_r8.rr_cfg_ded);
    apply_pdcp_drb_updates(recfg_r8.rr_cfg_ded);

    // UE MAC scheduler updates
    mac_ctrl.handle_con_reconf(recfg_r8, ue_capabilities);

    // Reuse same PDU
    if (pdu != nullptr)
    {
      pdu->clear();
    }

    // send DL-DCCH message to lower layers
    std::string octet_str;
    send_dl_dcch(&dl_dcch_msg, std::move(pdu), &octet_str);

    // Log event.
    asn1::json_writer json_writer;
    dl_dcch_msg.to_json(json_writer);
    event_logger::get().log_rrc_event(ue_cell_list.get_ue_cc_idx(UE_PCELL_CC_IDX)->cell_common->enb_cc_idx,
                                      octet_str,
                                      json_writer.to_string(),
                                      static_cast<unsigned>(rrc_event_type::con_reconf),
                                      static_cast<unsigned>(procedure_result_code::none),
                                      rnti);

    state = RRC_STATE_WAIT_FOR_CON_RECONF_COMPLETE;
  }

  //----------------2023.10.17------------------------------------------------------------------------------
  void rrc::ue::s_handle_rrc_reconf_complete(rrc_con_recon_comp_s *msg, srsran::unique_byte_buffer_t pdu)
  {
    asn1::json_writer json_writer;
    msg->to_json(json_writer);

    parent->phy->complete_config(rnti);
    rrc_transaction_id_s *msg_r1 = &msg->rrc_transaction_id;
    msg_r1->rrc_t_id = 0;
    parent->logger.info("RRCReconfigurationComplete with transaction ID: %d", msg_r1->rrc_t_id);
    mac_ctrl.s_handle_con_reconf_complete();

    if (is_rlc_mode_change)
    {
      is_rlc_mode_change = false;
      printf("second reconfiguration for rlc mode modify!\n");
      if (wx_area_mode==0)
      {
        send_rrc_con_reconf_data(70, 2, 1,0, nullptr, {});
      }
      else
      {
        send_rrc_con_reconf_kuopin_data(70, 2, 1,0, nullptr, {});
      }
      
    }
    // parent->s1ap->notify_rrc_reconf_complete(rnti);//-----------------s1ap需要改为mm的接口----------
  }

  void rrc::ue::handle_rrc_reconf_complete(rrc_conn_recfg_complete_s *msg, srsran::unique_byte_buffer_t pdu)
  {
    // Inform PHY about the configuration completion
    parent->phy->complete_config(rnti);

    if (transaction_id != msg->rrc_transaction_id)
    {
      parent->logger.error(
          "Expected RRCReconfigurationComplete with transaction ID: %d, got %d", transaction_id, msg->rrc_transaction_id);
      return;
    }

    // Log event.
    asn1::json_writer json_writer;
    msg->to_json(json_writer);
    event_logger::get().log_rrc_event(ue_cell_list.get_ue_cc_idx(UE_PCELL_CC_IDX)->cell_common->enb_cc_idx,
                                      asn1::octstring_to_string(last_ul_msg->msg, last_ul_msg->N_bytes),
                                      json_writer.to_string(),
                                      static_cast<unsigned>(rrc_event_type::con_reconf_complete),
                                      static_cast<unsigned>(procedure_result_code::none),
                                      rnti);

    // Activate SCells and bearers in the MAC scheduler that were advertised in the RRC Reconf message
    mac_ctrl.handle_con_reconf_complete();

    // If performing handover, signal its completion
    mobility_handler->trigger(*msg);

    // 2> if the UE has radio link failure or handover failure information available
    const auto &complete_r8 = msg->crit_exts.rrc_conn_recfg_complete_r8();
    if (complete_r8.non_crit_ext.non_crit_ext.rlf_info_available_r10_present or rlf_info_pending)
    {
      rlf_info_pending = false;
      send_ue_info_req();
    }

    // Many S1AP procedures end with RRC Reconfiguration. Notify S1AP accordingly.
    parent->s1ap->notify_rrc_reconf_complete(rnti);
  }

  void rrc::ue::send_ue_info_req()
  {
    dl_dcch_msg_s msg;
    auto &req_r9 = msg.msg.set_c1().set_ue_info_request_r9();
    req_r9.rrc_transaction_id = (uint8_t)((transaction_id++) % 4);

    auto &req = req_r9.crit_exts.set_c1().set_ue_info_request_r9();
    req.rlf_report_req_r9 = true;
    req.rach_report_req_r9 = true;

    send_dl_dcch(&msg);
  }

  void rrc::ue::handle_ue_info_resp(const asn1::rrc::ue_info_resp_r9_s &msg, srsran::unique_byte_buffer_t pdu)
  {
    auto &resp_r9 = msg.crit_exts.c1().ue_info_resp_r9();
    if (resp_r9.rlf_report_r9_present)
    {
      asn1::json_writer json_writer;
      msg.to_json(json_writer);
      event_logger::get().log_rlf_report(ue_cell_list.get_ue_cc_idx(UE_PCELL_CC_IDX)->cell_common->enb_cc_idx,
                                         asn1::octstring_to_string(pdu->msg, pdu->N_bytes),
                                         json_writer.to_string(),
                                         rnti);
    }
    if (resp_r9.rach_report_r9_present)
    {
      // TODO: Handle RACH-Report
    }
  }

  /*
   * Security Mode command
   */
  void rrc::ue::send_security_mode_command()
  {
    // Setup SRB1 security/integrity. Encryption is set on completion
    parent->pdcp->config_security(rnti, srb_to_lcid(lte_srb::srb1), ue_security_cfg.get_as_sec_cfg());
    parent->pdcp->enable_integrity(rnti, srb_to_lcid(lte_srb::srb1));

    dl_dcch_msg_s dl_dcch_msg;
    security_mode_cmd_s *comm = &dl_dcch_msg.msg.set_c1().set_security_mode_cmd();
    comm->rrc_transaction_id = (uint8_t)((transaction_id++) % 4);

    comm->crit_exts.set_c1().set_security_mode_cmd_r8().security_cfg_smc.security_algorithm_cfg =
        ue_security_cfg.get_security_algorithm_cfg();

    send_dl_dcch(&dl_dcch_msg);
  }

  void rrc::ue::handle_security_mode_complete(security_mode_complete_s *msg)
  {
    parent->logger.info("SecurityModeComplete transaction ID: %d", msg->rrc_transaction_id);

    parent->pdcp->enable_encryption(rnti, srb_to_lcid(lte_srb::srb1));
  }

  void rrc::ue::handle_security_mode_failure(security_mode_fail_s *msg)
  {
    parent->logger.info("SecurityModeFailure transaction ID: %d", msg->rrc_transaction_id);
  }

  /*
   * UE capabilities info
   */
  void rrc::ue::send_ue_cap_enquiry(const std::vector<asn1::rrc::rat_type_opts::options> &rats)
  {
    dl_dcch_msg_s dl_dcch_msg;
    dl_dcch_msg.msg.set_c1().set_ue_cap_enquiry().crit_exts.set_c1().set_ue_cap_enquiry_r8();

    ue_cap_enquiry_s *enq = &dl_dcch_msg.msg.c1().ue_cap_enquiry();
    enq->rrc_transaction_id = (uint8_t)((transaction_id++) % 4);

    enq->crit_exts.c1().ue_cap_enquiry_r8().ue_cap_request.resize(rats.size());
    for (uint32_t i = 0; i < rats.size(); ++i)
    {
      enq->crit_exts.c1().ue_cap_enquiry_r8().ue_cap_request[i].value = rats.at(i);
    }

    send_dl_dcch(&dl_dcch_msg);
  }

  /**
   * @brief Handle the reception of UE capability information message
   *
   * @return int SRSRAN_SUCCESS if unpacking was ok. SRSRAN_ERROR otherwise
   */
  int rrc::ue::handle_ue_cap_info(ue_cap_info_s *msg)
  {
    parent->logger.info("UECapabilityInformation transaction ID: %d", msg->rrc_transaction_id);
    ue_cap_info_r8_ies_s *msg_r8 = &msg->crit_exts.c1().ue_cap_info_r8();

    for (uint32_t i = 0; i < msg_r8->ue_cap_rat_container_list.size(); i++)
    {
      if (msg_r8->ue_cap_rat_container_list[i].rat_type != rat_type_e::eutra)
      {
        // Not handling UE capability information for RATs other than EUTRA
        continue;
      }
      asn1::cbit_ref bref(msg_r8->ue_cap_rat_container_list[i].ue_cap_rat_container.data(),
                          msg_r8->ue_cap_rat_container_list[i].ue_cap_rat_container.size());
      if (eutra_capabilities.unpack(bref) != asn1::SRSASN_SUCCESS)
      {
        parent->logger.error("Failed to unpack EUTRA capabilities message");
        return SRSRAN_ERROR;
      }
      if (parent->logger.debug.enabled())
      {
        asn1::json_writer js{};
        eutra_capabilities.to_json(js);
        parent->logger.debug("rnti=0x%x EUTRA capabilities: %s", rnti, js.to_string().c_str());
      }
      eutra_capabilities_unpacked = true;
      ue_capabilities = srsran::make_rrc_ue_capabilities(eutra_capabilities);

      parent->logger.info("UE rnti: 0x%x category: %d", rnti, eutra_capabilities.ue_category);

      if (endc_handler != nullptr)
      {
        endc_handler->handle_eutra_capabilities(eutra_capabilities);
      }
    }

    if (eutra_capabilities_unpacked)
    {
      srsran::unique_byte_buffer_t pdu = srsran::make_byte_buffer();
      if (pdu == nullptr)
      {
        parent->logger.error("Couldn't allocate PDU in %s().", __FUNCTION__);
        return SRSRAN_ERROR;
      }
      asn1::bit_ref bref2{pdu->msg, pdu->get_tailroom()};
      msg->pack(bref2);
      asn1::rrc::ue_radio_access_cap_info_s ue_rat_caps;
      auto &dest = ue_rat_caps.crit_exts.set_c1().set_ue_radio_access_cap_info_r8().ue_radio_access_cap_info;
      dest.resize(bref2.distance_bytes());
      memcpy(dest.data(), pdu->msg, bref2.distance_bytes());
      bref2 = asn1::bit_ref{pdu->msg, pdu->get_tailroom()};
      if (ue_rat_caps.pack(bref2) != asn1::SRSASN_SUCCESS)
      {
        parent->logger.error("Couldn't pack ue rat caps");
        return SRSRAN_ERROR;
      }
      pdu->N_bytes = bref2.distance_bytes();
      parent->s1ap->send_ue_cap_info_indication(rnti, std::move(pdu));
    }

    return SRSRAN_SUCCESS;
  }

  /*
   * Connection Release
   */
  void rrc::ue::s_send_connection_release_reg_req_congestion()
  {
    static const uint32_t release_delay = 200;
    s_dl_dcch_msg_s s_dl_dcch_msg;
    rrc_con_release_s &rrc_release = s_dl_dcch_msg.msg.set_rrc_con_release();
    rrc_con_rel_r1_ies_s &rel_ies = rrc_release.rrc_con_rel_r1_ies;
    rel_ies.relea_cause = rrc_con_rel_r1_ies_s::relea_cause_opts::other;

    auto start_time = std::chrono::high_resolution_clock::now();

    while (true)
    {
      auto now_time = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::seconds>(now_time - start_time);
      if (duration.count() > 3)
      {
        break;
      }
    }

    std::string octet_str;
    s_send_dl_dcch(&s_dl_dcch_msg, nullptr, &octet_str);

    // Log rrc release event.
    asn1::json_writer json_writer;
    s_dl_dcch_msg.to_json(json_writer);
    event_logger::get().log_rrc_event(ue_cell_list.get_ue_cc_idx(UE_PCELL_CC_IDX)->cell_common->enb_cc_idx,
                                      octet_str,
                                      json_writer.to_string(),
                                      static_cast<unsigned>(rrc_event_type::con_release),
                                      static_cast<unsigned>(con_release_result),
                                      rnti);
    // Restore release result.
    con_release_result = procedure_result_code::none;
    parent->task_sched.defer_callback(release_delay, [this]()
                                      { parent->rem_user(rnti); });
  }

  void rrc::ue::s_send_connection_release_reg_ss_no5Gguti()
  {
    static const uint32_t release_delay = 100;

    // Restore release result.
    con_release_result = procedure_result_code::none;
    parent->task_sched.defer_callback(release_delay, [this]()
                                      { parent->rem_user(rnti); });
  }

  void rrc::ue::TTCN_connection_release()
  {
    // static const uint32_t release_delay=0;

    std::cout << "rrc::ue::s_send_connection_release()" << std::endl;

    s_dl_dcch_msg_s s_dl_dcch_msg;
    rrc_con_release_s &rrc_release = s_dl_dcch_msg.msg.set_rrc_con_release();
    rrc_con_rel_r1_ies_s &rel_ies = rrc_release.rrc_con_rel_r1_ies;
    rel_ies.relea_cause = rrc_con_rel_r1_ies_s::relea_cause_opts::other;
    if (parent->cfg.ttcn_rrc_enble && parent->cfg.ttcn_test_enble && (parent->rrc_adp->udp_.TC_725_release || parent->rrc_adp->udp_.TC_514_Release || parent->rrc_adp->udp_.TC_515_Release))
    {
      while (true)
      {
        if (parent->rrc_adp->udp_.rrc_receive_info.size() != 0)
        {
          srsran::unique_byte_buffer_t ttcn_release =
              srsran::make_byte_buffer();
          std::cout << "receive rrc release info from TTCN" << std::endl;
          ttcn_release->init();
          parent->rrc_adp->udp_.rrc_receive_info.try_pop(ttcn_release);
          parent->get_general_interface(&ttcn_release);
          rel_ies.relea_cause = static_cast<asn1::rrc::rrc_con_rel_r1_ies_s::relea_cause_opts::options>(ttcn_release->msg[8]);

          std::cout << "rel_ies.relea_cause TTCN:" << rel_ies.relea_cause.to_string() << std::endl;

          break;
        }
      }
    }

    std::string octet_str;
    s_send_dl_dcch(&s_dl_dcch_msg, nullptr, &octet_str);

    // Log rrc release event.
    asn1::json_writer json_writer;
    s_dl_dcch_msg.to_json(json_writer);
    event_logger::get().log_rrc_event(ue_cell_list.get_ue_cc_idx(UE_PCELL_CC_IDX)->cell_common->enb_cc_idx,
                                      octet_str,
                                      json_writer.to_string(),
                                      static_cast<unsigned>(rrc_event_type::con_release),
                                      static_cast<unsigned>(con_release_result),
                                      rnti);
    // Restore release result.
    con_release_result = procedure_result_code::none;
    auto start_time = std::chrono::high_resolution_clock::now();

    while (true)
    {
      auto now_time = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::seconds>(now_time - start_time);
      if (duration.count() > 0.5)
      {
        break;
      }
    }
    parent->rem_user(rnti);

    if (parent->cfg.ttcn_rrc_enble && parent->cfg.ttcn_test_enble && parent->rrc_adp->udp_.TC_725_release)
    {
      std::cout << "send rrc release kongbao to TTCN" << std::endl;
      srsran::unique_byte_buffer_t rerurn_info_send = srsran::make_byte_buffer();
      rerurn_info_send->init();
      rerurn_info_send->msg[0] = 0x01;
      rerurn_info_send->msg[1] = 0x01;
      rerurn_info_send->msg[2] = 0x01;
      rerurn_info_send->msg[3] = 0x01;
      rerurn_info_send->msg[3] = 0x01;
      rerurn_info_send->msg[3] = 0x01;
      rerurn_info_send->N_bytes = 6;
      parent->rrc_adp->udp_.send_ttcn_info.try_push(std::move(rerurn_info_send));
    }
    // parent->task_sched.defer_callback(release_delay,[this](){parent->rem_user(rnti);});
  }
  //----------------------------2024/3/14---------------------------------//
  void rrc::ue::s_send_connection_release()
  {
    // static const uint32_t release_delay=0;

    std::cout << " RRC RELEASE " << std::endl;

    s_dl_dcch_msg_s s_dl_dcch_msg;
    rrc_con_release_s &rrc_release = s_dl_dcch_msg.msg.set_rrc_con_release();
    rrc_con_rel_r1_ies_s &rel_ies = rrc_release.rrc_con_rel_r1_ies;
    rel_ies.relea_cause = rrc_con_rel_r1_ies_s::relea_cause_opts::other;

    std::string octet_str;
    s_send_dl_dcch(&s_dl_dcch_msg, nullptr, &octet_str);

    // Log rrc release event.
    asn1::json_writer json_writer;
    s_dl_dcch_msg.to_json(json_writer);
    event_logger::get().log_rrc_event(ue_cell_list.get_ue_cc_idx(UE_PCELL_CC_IDX)->cell_common->enb_cc_idx,
                                      octet_str,
                                      json_writer.to_string(),
                                      static_cast<unsigned>(rrc_event_type::con_release),
                                      static_cast<unsigned>(con_release_result),
                                      rnti);
    // Restore release result.
    con_release_result = procedure_result_code::none;
    auto start_time = std::chrono::high_resolution_clock::now();

    while (true)
    {
      auto now_time = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::seconds>(now_time - start_time);
      if (duration.count() > 0.5)
      {
        break;
      }
    }
    parent->rem_user(rnti);

    // parent->task_sched.defer_callback(release_delay,[this](){parent->rem_user(rnti);});
  }
  //-----------------------------------------------------------------//
  void rrc::ue::send_connection_release()
  {
    dl_dcch_msg_s dl_dcch_msg;
    auto &rrc_release = dl_dcch_msg.msg.set_c1().set_rrc_conn_release();
    rrc_release.rrc_transaction_id = (uint8_t)((transaction_id++) % 4);
    rrc_conn_release_r8_ies_s &rel_ies = rrc_release.crit_exts.set_c1().set_rrc_conn_release_r8();
    rel_ies.release_cause = release_cause_e::other;
    if (is_csfb)
    {
      if (parent->sib7.carrier_freqs_info_list.size() > 0)
      {
        rel_ies.redirected_carrier_info_present = true;
        rel_ies.redirected_carrier_info.set_geran();
        rel_ies.redirected_carrier_info.geran() = parent->sib7.carrier_freqs_info_list[0].carrier_freqs;
      }
      else
      {
        rel_ies.redirected_carrier_info_present = false;
      }
    }

    std::string octet_str;
    send_dl_dcch(&dl_dcch_msg, nullptr, &octet_str);

    // Log rrc release event.
    asn1::json_writer json_writer;
    dl_dcch_msg.to_json(json_writer);
    event_logger::get().log_rrc_event(ue_cell_list.get_ue_cc_idx(UE_PCELL_CC_IDX)->cell_common->enb_cc_idx,
                                      octet_str,
                                      json_writer.to_string(),
                                      static_cast<unsigned>(rrc_event_type::con_release),
                                      static_cast<unsigned>(con_release_result),
                                      rnti);
    // Restore release result.
    con_release_result = procedure_result_code::none;
  }
  /*
   * UE Init Context Setup Request
   */
  void rrc::ue::handle_ue_init_ctxt_setup_req(const asn1::s1ap::init_context_setup_request_s &msg)
  {
    set_bitrates(msg.protocol_ies.ueaggregate_maximum_bitrate.value);
    ue_security_cfg.set_security_capabilities(msg.protocol_ies.ue_security_cap.value, 0, 0);
    ue_security_cfg.set_security_key(msg.protocol_ies.security_key.value);

    // CSFB
    if (msg.protocol_ies.cs_fallback_ind_present)
    {
      if (msg.protocol_ies.cs_fallback_ind.value.value == asn1::s1ap::cs_fallback_ind_opts::cs_fallback_required or
          msg.protocol_ies.cs_fallback_ind.value.value == asn1::s1ap::cs_fallback_ind_opts::cs_fallback_high_prio)
      {
        is_csfb = true;
      }
    }

    // Send RRC security mode command
    send_security_mode_command();
  }

  //-----------------------------------2023/11/1------------------------------
  void rrc::ue::s_handle_ue_init_ctxt_setup_req(const asn1::s1ap::init_ctxt_setup_req_s &msg)
  {
    set_bitrates(msg.ueaggregate_maximum_bitrate);
    set_as_security_cfg();
    ue_security_cfg.set_security_capabilities(msg.ue_security_cap, parent->cfg.ttcn_test_enble, parent->rrc_adp->udp_.TC_79_is_paging_smc);
    ue_security_cfg.set_security_key(msg.security_key);

    // Send RRC security mode command
    // send_security_mode_command(); // 闇€瑕佹敼鎴愯枦鏅撳嚡锟�???????
    s_send_security_mode_command();
  }

  //-------------------------2023/11/1-------------------------
  void rrc::ue::s_send_security_mode_command()
  {
    // Setup SRB1 security/integrity. Encryption is set on completion
    parent->pdcp->config_security(rnti, srb_to_lcid(lte_srb::srb1), ue_security_cfg.get_as_sec_cfg());
    parent->pdcp->enable_integrity(rnti, srb_to_lcid(lte_srb::srb1));

    s_dl_dcch_msg_s s_dl_dcch_msg;
    sec_mode_com_s *comma = &s_dl_dcch_msg.msg.set_sec_mode_com();
    comma->rrc_tran_id.rrc_tran_id_t = 0;
    comma->sec_mode_com_r1_ies.sec_alg_cfg = ue_security_cfg.get_sec_alg_cfg();

    if (parent->cfg.ttcn_rrc_enble && parent->cfg.ttcn_test_enble && (parent->rrc_adp->udp_.TC_79_is_paging_smc || parent->rrc_adp->udp_.TC_713_reconfig_DRB || parent->rrc_adp->udp_.TC_714_DRB_Release || parent->rrc_adp->udp_.TC_715_reest_reconf))
    {
      while (true)
      {
        if (parent->rrc_adp->udp_.rrc_receive_info.size() != 0)
        {
          srsran::unique_byte_buffer_t ttcn_smc =
              srsran::make_byte_buffer();
          std::cout << "receive rrc smc info from TTCN" << std::endl;
          ttcn_smc->init();
          parent->rrc_adp->udp_.rrc_receive_info.try_pop(ttcn_smc);
          parent->get_general_interface(&ttcn_smc);
          comma->sec_mode_com_r1_ies.sec_alg_cfg.integ_prot_alg_present = true;
          comma->sec_mode_com_r1_ies.sec_alg_cfg.coph_alg = static_cast<asn1::rrc::sec_alg_cfg_s::ciph_alg_opts::options>(
              (ttcn_smc->msg[8]));
          std::cout << "comma->sec_mode_com_r1_ies.sec_alg_cfg.coph_alg:" << comma->sec_mode_com_r1_ies.sec_alg_cfg.coph_alg.to_string() << std::endl;
          comma->sec_mode_com_r1_ies.sec_alg_cfg.intef_prot_alg = static_cast<asn1::rrc::sec_alg_cfg_s::intef_prot_alg_opts::options>(
              (ttcn_smc->msg[9]));
          std::cout << "comma->sec_mode_com_r1_ies.sec_alg_cfg.intef_prot_alg:" << comma->sec_mode_com_r1_ies.sec_alg_cfg.intef_prot_alg.to_string() << std::endl;
          break;
        }
      }
    }

    if (parent->cfg.ttcn_rrc_enble && parent->cfg.ttcn_test_enble && parent->rrc_adp->udp_.TC_711_reconfig)
    {
      while (true)
      {
        if (parent->rrc_adp->udp_.rrc_receive_info.size() != 0)
        {
          srsran::unique_byte_buffer_t ttcn_smc =
              srsran::make_byte_buffer();
          std::cout << "receive rrc smc info from TTCN" << std::endl;
          ttcn_smc->init();
          parent->rrc_adp->udp_.rrc_receive_info.try_pop(ttcn_smc);
          parent->get_general_interface(&ttcn_smc);
          comma->sec_mode_com_r1_ies.sec_alg_cfg.integ_prot_alg_present = true;
          comma->sec_mode_com_r1_ies.sec_alg_cfg.coph_alg = static_cast<asn1::rrc::sec_alg_cfg_s::ciph_alg_opts::options>(
              (ttcn_smc->msg[8]));
          std::cout << "comma->sec_mode_com_r1_ies.sec_alg_cfg.coph_alg:" << comma->sec_mode_com_r1_ies.sec_alg_cfg.coph_alg.to_string() << std::endl;
          comma->sec_mode_com_r1_ies.sec_alg_cfg.intef_prot_alg = static_cast<asn1::rrc::sec_alg_cfg_s::intef_prot_alg_opts::options>(
              (ttcn_smc->msg[9]));
          std::cout << "comma->sec_mode_com_r1_ies.sec_alg_cfg.intef_prot_alg:" << comma->sec_mode_com_r1_ies.sec_alg_cfg.intef_prot_alg.to_string() << std::endl;
          break;
        }
      }
    }
    if (parent->rrc_adp->udp_.TC_710_security_mode_failure)
    {
      while (true)
      {
        if (parent->rrc_adp->udp_.rrc_receive_info.size() != 0)
        {
          srsran::unique_byte_buffer_t ttcn_smc =
              srsran::make_byte_buffer();
          std::cout << "receive rrc smc failure info from TTCN" << std::endl;
          ttcn_smc->init();
          parent->rrc_adp->udp_.rrc_receive_info.try_pop(ttcn_smc);
          parent->get_general_interface(&ttcn_smc);
          comma->sec_mode_com_r1_ies.sec_alg_cfg.integ_prot_alg_present = true;
          comma->sec_mode_com_r1_ies.sec_alg_cfg.coph_alg = static_cast<asn1::rrc::sec_alg_cfg_s::ciph_alg_opts::options>(
              (ttcn_smc->msg[8]));
          std::cout << "ttcn_smc->msg[8]" << ttcn_smc->msg[8] << std::endl;
          std::cout << "ttcn_smc->msg[9]" << ttcn_smc->msg[9] << std::endl;
          std::cout << "comma->sec_mode_com_r1_ies.sec_alg_cfg.coph_alg:" << comma->sec_mode_com_r1_ies.sec_alg_cfg.coph_alg.to_string() << std::endl;
          comma->sec_mode_com_r1_ies.sec_alg_cfg.intef_prot_alg = static_cast<asn1::rrc::sec_alg_cfg_s::intef_prot_alg_opts::options>(
              (ttcn_smc->msg[9]));
          std::cout << "comma->sec_mode_com_r1_ies.sec_alg_cfg.intef_prot_alg:" << comma->sec_mode_com_r1_ies.sec_alg_cfg.intef_prot_alg.to_string() << std::endl;
          break;
        }
      }
    }
    std::cout << "SMC INFO" << std::endl;
    s_send_dl_dcch(&s_dl_dcch_msg);
  }
  void rrc::ue::s_handle_security_mode_complete(security_mode_comp_s *msg)
  {
    parent->logger.info("SecurityModeComplete transaction ID: %d", msg->rrc_transaction_id.rrc_t_id);

    parent->pdcp->enable_encryption(rnti, srb_to_lcid(lte_srb::srb1));
  }
  void rrc::ue::s_handle_security_mode_failure(secur_mode_fail_s *msg)
  {
    parent->logger.info("SecurityModeFailure transaction ID: %d", msg->rrc_transaction_id.rrc_t_id);
    printf("s_handle_security_mode_failure");
  }
  //-----------------------------------------------------------

  bool rrc::ue::s_handle_ue_ctxt_mod_req(const asn1::s1ap::ctxt_mod_req_s &msg)
  {
    if (msg.ueaggreagate_maximum_bitrate_present)
    {
      set_bitrates(msg.ueaggregate_maximum_bitrate);
    }

    if (msg.ue_security_cap_present)
    {
      ue_security_cfg.set_security_capabilities(msg.ue_security_cap, 0, 0);
    }

    if (msg.security_key_present)
    {
      ue_security_cfg.set_security_key(msg.security_key);
      send_security_mode_command(); // 闇€瑕佹敼鎴愯枦灏忓嚡锟�???????
    }

    return true;
  }
  //--------------------------------------------------------------------------

  bool rrc::ue::handle_ue_ctxt_mod_req(const asn1::s1ap::ue_context_mod_request_s &msg)
  {
    if (msg.protocol_ies.cs_fallback_ind_present)
    {
      if (msg.protocol_ies.cs_fallback_ind.value.value == asn1::s1ap::cs_fallback_ind_opts::cs_fallback_required ||
          msg.protocol_ies.cs_fallback_ind.value.value == asn1::s1ap::cs_fallback_ind_opts::cs_fallback_high_prio)
      {
        /* Remember that we are in a CSFB right now */
        is_csfb = true;
      }
    }

    // UEAggregateMaximumBitrate
    if (msg.protocol_ies.ueaggregate_maximum_bitrate_present)
    {
      set_bitrates(msg.protocol_ies.ueaggregate_maximum_bitrate.value);
    }

    if (msg.protocol_ies.ue_security_cap_present)
    {
      ue_security_cfg.set_security_capabilities(msg.protocol_ies.ue_security_cap.value, 0, 0);
    }

    if (msg.protocol_ies.security_key_present)
    {
      ue_security_cfg.set_security_key(msg.protocol_ies.security_key.value);

      send_security_mode_command();
    }

    return true;
  }

  void rrc::ue::set_bitrates(const asn1::s1ap::ue_aggregate_maximum_bitrate_s &rates)
  {
    bitrates = rates;
  }

  bool rrc::ue::release_erabs()
  {
    bearer_list.release_erabs();
    return true;
  }

  int rrc::ue::release_erab(uint32_t erab_id)
  {
    return bearer_list.release_erab(erab_id);
  }

  int rrc::ue::get_erab_addr_in(uint16_t erab_id, transp_addr_t &addr_in, uint32_t &teid_in) const
  {
    auto it = bearer_list.get_erabs().find(erab_id);
    if (it == bearer_list.get_erabs().end())
    {
      parent->logger.error("E-RAB id=%d for rnti=0x%x not found", erab_id, rnti);
      return SRSRAN_ERROR;
    }
    addr_in = it->second.address;
    teid_in = it->second.teid_in;
    return SRSRAN_SUCCESS;
  }

  int rrc::ue::setup_erab(uint16_t erab_id,
                          const asn1::s1ap::erab_level_qos_params_s &qos_params,
                          srsran::const_span<uint8_t> nas_pdu,
                          const asn1::bounded_bitstring<1, 160, true, true> &addr,
                          uint32_t gtpu_teid_out,
                          asn1::s1ap::cause_c &cause)
  {
    if (bearer_list.get_erabs().count(erab_id) > 0)
    {
      cause.set_radio_network().value = asn1::s1ap::cause_radio_network_opts::multiple_erab_id_instances;
      return SRSRAN_ERROR;
    }
    if (bearer_list.add_erab(erab_id, qos_params, addr, gtpu_teid_out, nas_pdu, cause) != SRSRAN_SUCCESS)
    {
      parent->logger.error("Couldn't add E-RAB id=%d for rnti=0x%x", erab_id, rnti);
      return SRSRAN_ERROR;
    }
    if (bearer_list.add_gtpu_bearer(erab_id) != SRSRAN_SUCCESS)
    {
      cause.set_radio_network().value = asn1::s1ap::cause_radio_network_opts::radio_res_not_available;
      bearer_list.release_erab(erab_id);
      parent->logger.error("Couldn't add E-RAB id=%d for rnti=0x%x", erab_id, rnti);
      return SRSRAN_ERROR;
    }
    return SRSRAN_SUCCESS;
  }

  int rrc::ue::modify_erab(uint16_t erab_id,
                           const asn1::s1ap::erab_level_qos_params_s &qos_params,
                           srsran::const_span<uint8_t> nas_pdu,
                           asn1::s1ap::cause_c &cause)
  {
    return bearer_list.modify_erab(erab_id, qos_params, nas_pdu, cause);
  }

  //! Helper method to access Cell configuration based on UE Carrier Index
  enb_cell_common *rrc::ue::get_ue_cc_cfg(uint32_t ue_cc_idx)
  {
    if (ue_cc_idx >= ue_cell_list.nof_cells())
    {
      return nullptr;
    }
    uint32_t enb_cc_idx = ue_cell_list.get_ue_cc_idx(ue_cc_idx)->cell_common->enb_cc_idx;
    return parent->cell_common_list->get_cc_idx(enb_cc_idx);
  }

  void rrc::ue::update_scells()
  {
    const ue_cell_ded *pcell = ue_cell_list.get_ue_cc_idx(UE_PCELL_CC_IDX);
    const enb_cell_common *pcell_cfg = pcell->cell_common;

    // Check whether UE supports CA
    if (eutra_capabilities.access_stratum_release.to_number() < 10)
    {
      parent->logger.info("UE doesn't support CA. Skipping SCell activation");
      return;
    }
    if (not eutra_capabilities.non_crit_ext_present or not eutra_capabilities.non_crit_ext.non_crit_ext_present or
        not eutra_capabilities.non_crit_ext.non_crit_ext.non_crit_ext_present or
        not eutra_capabilities.non_crit_ext.non_crit_ext.non_crit_ext.rf_params_v1020_present or
        eutra_capabilities.non_crit_ext.non_crit_ext.non_crit_ext.rf_params_v1020.supported_band_combination_r10.size() ==
            0)
    {
      parent->logger.info("UE doesn't support CA. Skipping SCell activation");
      return;
    }

    if (ue_cell_list.nof_cells() == pcell_cfg->scells.size() + 1)
    {
      // SCells already added
      return;
    }

    for (const enb_cell_common *scell : pcell_cfg->scells)
    {
      ue_cell_list.add_cell(scell->enb_cc_idx);
    }

    parent->logger.info("SCells activated for rnti=0x%x", rnti);
  }

  /********************** HELPERS ***************************/

  //---------------------2022/08/04--------------------------
  void rrc::ue::send_s_dl_ccch(s_dl_ccch_msg_s *dl_ccch_msg, std::string *octet_str)
  {
    // Allocate a new PDU buffer, pack the message and send to PDCP
    srsran::unique_byte_buffer_t pdu = srsran::make_byte_buffer();
    // pdu->init();
    std::cout << "----------------Init  setup message pdu ------------" << std::endl;
    //   for(uint32_t i=0;i<10;i++)
    // {
    //   printf("0x%x\n",*(pdu->msg+i));
    // }
    if (pdu)
    {
      asn1::bit_ref bref(pdu->msg, pdu->get_tailroom());
      if (dl_ccch_msg->pack(bref) != asn1::SRSASN_SUCCESS)
      {
        parent->logger.error(pdu->msg, pdu->N_bytes, "Failed to pack DL-CCCH-Msg:");
        return;
      }
      pdu->N_bytes = (uint32_t)bref.distance_bytes();
      std::cout << "****************SET UP INFO****************" << std::endl;
      // for(uint32_t i=0;i<pdu->N_bytes;i++)
      // {
      //   printf("0x%x\n",*(pdu->msg+i));
      // }

      // Log Tx message
      parent->log_rrc_message(
          Tx, rnti, srb_to_lcid(lte_srb::srb0), *pdu, *dl_ccch_msg, dl_ccch_msg->msg.type().to_string());

      srsran::rrc_pcap_net *p_pcap_net = getPcapNet(*parent);
      if (p_pcap_net)
      {
        p_pcap_net->write_dl_rrc_pdu(pdu->msg, pdu->N_bytes, CY_LOGICCHANNEL_TYPE_DL_CCCH, CY_NET_MODE_RAN);
      }

      // Encode the pdu as an octet string if the user passed a valid pointer.
      if (octet_str)
      {
        *octet_str = asn1::octstring_to_string(pdu->msg, pdu->N_bytes);
      }

      std::cout << "***************random value*****:" << random_value << std::endl;
      // set_up_msg setUp,dl_allocate allocSource,   ul_allocate ulAlloc;

      set_up_msg setup;
      setup.rnti = rnti;
      setup.consIDlen = 48;

      if (has_nr_s_tmsi)
      {
        setup.consID = nr_s_tmsi;
        has_nr_s_tmsi = false;
      }
      else if (has_random_value)
      {
        setup.consID = random_value;
        has_random_value = false;
      }
      else
      {
        setup.consID = 282431084152;
      }

      setup.pdu = pdu->msg;
      setup.pduLen = pdu->N_bytes;

      if (ue_category == 14)
      {
        std::cout << " ue_category 2 = " << (int)ue_category << std::endl;

        if (!parent->mac->setupInfo_uecategory14(setup))
        {
          std::cout << "  setupInfo_uecategory14 error " << std::endl;
        }
      }
      else
      {
        dl_allocate allocSource;
        ul_allocate ulAlloc;
        if (wx_area_mode == 0)
        {

          if (parent->rrc_adp->udp_.TC_75_is_paging_refuse == true || parent->rrc_adp->udp_.TC_74_t302_timeout == true)
          {
            std::cout << "udp_.TC_74_t302_timeout_refuse--" << parent->rrc_adp->udp_.TC_74_t302_timeout << std::endl;
            allocSource.bandID = 9;     // dl_ccch_msg->msg.rrc_con_setup().rrc_con_setup_r1.rr_cfg_ded.phy_ch_list_cfg[0].band_id.ba_id.to_number();
            allocSource.freq = 3;       // dl_ccch_msg->msg.rrc_con_setup().rrc_con_setup_r1.rr_cfg_ded.phy_ch_list_cfg[0].freq_id.freq_id.to_number();
            allocSource.Type = pdch1_1; //(ChanType_t)(int)dl_ccch_msg->msg.rrc_con_setup().rrc_con_setup_r1.rr_cfg_ded.phy_ch_list_cfg[0].ch_type;
                                        // uint8_t ulSolt = 6;
            allocSource.solt = 6;       // dl_ccch_msg->msg.rrc_con_setup().rrc_con_setup_r1.rr_cfg_ded.phy_ch_list_cfg[0].slot_ass.to_number();
            // allocSource.solt=dl_ccch_msg->msg.rrc_con_setup().rrc_con_setup_r1.rr_cfg_ded.phy_ch_list_cfg[0].slot_ass.to_number();
            // add tx
            ulAlloc.bandID = 9;     // dl_ccch_msg->msg.rrc_con_setup().rrc_con_setup_r1.rr_cfg_ded.phy_ch_list_cfg[0].band_id.ba_id.to_number();
            ulAlloc.freq = 1;       // dl_ccch_msg->msg.rrc_con_setup().rrc_con_setup_r1.rr_cfg_ded.phy_ch_list_cfg[0].freq_id.freq_id.to_number();
            ulAlloc.Type = pdch1_1; //(ChanType_t)(int)dl_ccch_msg->msg.rrc_con_setup().rrc_con_setup_r1.rr_cfg_ded.phy_ch_list_cfg[0].ch_type;
                                    // std::cout<<"to number ulAlloc.ulAlloc.freq:"<<ulAlloc.freq<<std::endl;
                                    // std::cout<<"to number ulAlloc.ulAlloc.bandID:"<<ulAlloc.bandID<<std::endl;
                                    // std::cout<<"to number ulAlloc.allocSource.Type:"<<ulAlloc.Type<<std::endl;
                                    // int n = 0;
            // while ((ulSolt & 0x01) == 0)
            // {
            //   // 0x10
            //   // solt=solt<<1;
            //   ulSolt = ulSolt >> 1;
            //   n++;

            // }
            // allocSource.solt = n;
            if (parent->rrc_adp->udp_.TC_75_is_paging_refuse)
            {
              parent->rrc_adp->udp_.TC_75_is_paging_refuse = false; // zhj get nomol module
              parent->rrc_adp->udp_.TC_75_is_second_con_req = true;
            }
            std::cout << "to number ulAlloc.solt2:" << (int)allocSource.solt << std::endl;
            // std::cout<<"to number , allocSource.sol444t"<<allocSource.solt<<std::endl;
            parent->mac->setConsID(setup, allocSource, ulAlloc);
          }
          else
          {
            allocSource.bandID = dl_ccch_msg->msg.rrc_con_setup().rrc_con_setup_r1.rr_cfg_ded.phy_ch_list_cfg[0].band_id.ba_id.to_number();
            allocSource.freq = dl_ccch_msg->msg.rrc_con_setup().rrc_con_setup_r1.rr_cfg_ded.phy_ch_list_cfg[0].freq_id.freq_id.to_number();
            allocSource.Type = (ChanType_t)(int)dl_ccch_msg->msg.rrc_con_setup().rrc_con_setup_r1.rr_cfg_ded.phy_ch_list_cfg[0].ch_type;
            // uint8_t ulSolt;
            allocSource.solt = dl_ccch_msg->msg.rrc_con_setup().rrc_con_setup_r1.rr_cfg_ded.phy_ch_list_cfg[0].slot_ass.to_number();
            // allocSource.solt=dl_ccch_msg->msg.rrc_con_setup().rrc_con_setup_r1.rr_cfg_ded.phy_ch_list_cfg[0].slot_ass.to_number();
            // add tx
            ulAlloc.bandID = dl_ccch_msg->msg.rrc_con_setup().rrc_con_setup_r1.rr_cfg_ded.phy_ch_list_cfg[0].band_id.ba_id.to_number();
            ulAlloc.freq = dl_ccch_msg->msg.rrc_con_setup().rrc_con_setup_r1.rr_cfg_ded.phy_ch_list_cfg[0].freq_id.freq_id.to_number();
            ulAlloc.Type = (ChanType_t)(int)dl_ccch_msg->msg.rrc_con_setup().rrc_con_setup_r1.rr_cfg_ded.phy_ch_list_cfg[0].ch_type;
            // std::cout<<"to number ulAlloc.ulAlloc.freq:"<<ulAlloc.freq<<std::endl;
            // std::cout<<"to number ulAlloc.ulAlloc.bandID:"<<ulAlloc.bandID<<std::endl;
            // std::cout<<"to number ulAlloc.allocSource.Type:"<<ulAlloc.Type<<std::endl;
            // int n = 0;
            // while ((ulSolt & 0x01) == 0)
            // {
            //   // 0x10
            //   // solt=solt<<1;
            //   ulSolt = ulSolt >> 1;
            //   n++;
            //   // std::cout<<"to number , allocSource.sol4445555t"<<n<<std::endl;
            // }
            // allocSource.solt = n;
            std::cout << "to number ulAlloc.solt:2" << (int)allocSource.solt << std::endl;
            // std::cout<<"to number , allocSource.sol444t"<<allocSource.solt<<std::endl;
            std::cout<<"TC_73_t300_timeout"<<parent->rrc_adp->udp_.TC_73_t300_timeout<<std::endl;
            if (parent->rrc_adp->udp_.TC_73_t300_timeout)
            {
              parent->rrc_adp->udp_.TC_73_t300_timeout = false;
              parent->mac->TC300Timeout(setup);
            }
            else
            {
              parent->mac->setConsID(setup, allocSource, ulAlloc);
            }
          }
        }

        if (wx_area_mode == 1)
        {

          ulAlloc.bandID = dl_ccch_msg->msg.rrc_con_setup().rrc_con_setup_r1.rr_cfg_ded.phy_ch_list_cfg[0].band_id.ba_id.to_number();
          ulAlloc.freq = dl_ccch_msg->msg.rrc_con_setup().rrc_con_setup_r1.rr_cfg_ded.phy_ch_list_cfg[0].freq_id.freq_id.to_number();
          ulAlloc.Type = (ChanType_t)(int)dl_ccch_msg->msg.rrc_con_setup().rrc_con_setup_r1.rr_cfg_ded.phy_ch_list_cfg[0].ch_type;
          // uint8_t ulSolt ;
          ulAlloc.solt = dl_ccch_msg->msg.rrc_con_setup().rrc_con_setup_r1.rr_cfg_ded.phy_ch_list_cfg[0].slot_ass.to_number();
          // allocSource.SF_i=dl_ccch_msg->msg.rrc_con_setup().rrc_con_setup_r1.rr_cfg_ded.phy_ch_list_cfg[0].pdtch_code.pdtch_phy_code;
          std::cout << "to number ulAlloc.solt" << ulAlloc.solt << std::endl;
          // int a = 0;
          // while ((ulSolt & 0x01) == 0)
          // {
          //   // 0x10
          //   // solt=solt<<1;
          //   ulSolt = ulSolt >> 1;
          //   a++;
          // }
          // ulAlloc.solt = a;

          allocSource.bandID = dl_ccch_msg->msg.rrc_con_setup().rrc_con_setup_r1.rr_cfg_ded.phy_ch_list_cfg[1].band_id.ba_id.to_number();
          allocSource.freq = dl_ccch_msg->msg.rrc_con_setup().rrc_con_setup_r1.rr_cfg_ded.phy_ch_list_cfg[1].freq_id.freq_id.to_number();
          allocSource.Type = (ChanType_t)(int)dl_ccch_msg->msg.rrc_con_setup().rrc_con_setup_r1.rr_cfg_ded.phy_ch_list_cfg[1].ch_type;
          allocSource.SF_i = dl_ccch_msg->msg.rrc_con_setup().rrc_con_setup_r1.rr_cfg_ded.phy_ch_list_cfg[1].pdtch_code.pdtch_phy_code;

          // uint8_t dlSolt ;
          allocSource.solt = dl_ccch_msg->msg.rrc_con_setup().rrc_con_setup_r1.rr_cfg_ded.phy_ch_list_cfg[1].slot_ass.to_number();
          // ulAlloc.solt=dl_ccch_msg->msg.rrc_con_setup().rrc_con_setup_r1.rr_cfg_ded.phy_ch_list_cfg[1].slot_ass.to_number();
          std::cout << "to number dlAlloc.solt" << allocSource.solt << std::endl;
          // int n = 0;
          // while ((dlSolt & 0x01) == 0)
          // {
          //   // 0x10
          //   // solt=solt<<1;
          //   dlSolt = dlSolt >> 1;
          //   n++;
          // }
          // allocSource.solt = n;
          parent->mac->setConsID(setup, allocSource, ulAlloc);
        }
      }
    }
    else
    {
      parent->logger.error("Allocating pdu");
    }
  }
  //-----------------------------------------------------------------------------------
  //-------------------------------IOT-2023/9/14---------------------------------------
  void rrc::ue::iot_send_s_dl_ccch(iot_dl_ccch_msg_s *iot_dl_ccch_msg, std::string *octet_str)
  {
    // Allocate a new PDU buffer, pack the message and send
    srsran::unique_byte_buffer_t pdu = srsran::make_byte_buffer();
    if (pdu)
    {
      asn1::bit_ref bref(pdu->msg, pdu->get_tailroom());
      if (iot_dl_ccch_msg->pack(bref) != asn1::SRSASN_SUCCESS)
      {
        parent->logger.error(pdu->msg, pdu->N_bytes, "Failed to pack IOT-DL-CCCH-Msg:");
        return;
      }
      pdu->N_bytes = (uint32_t)bref.distance_bytes();
      // std::cout<<"This  is iot_dl_ccch_msg_s:"<<std::endl;
      // srsran::console("当前字节数=%u\n", pdu->N_bytes);   //打印解析的数据
      // for (uint8_t i=0; i < pdu->N_bytes; i++)
      // {
      // srsran::console("%x\n", *(pdu->msg + i));
      // }

      // Log Tx message
      parent->log_rrc_message(
          Tx, rnti, srb_to_lcid(lte_srb::srb0), *pdu, *iot_dl_ccch_msg, iot_dl_ccch_msg->msg.type().to_string());

      // Encode the pdu as an octet string if the user passed a valid pointer.
      if (octet_str)
      {
        *octet_str = asn1::octstring_to_string(pdu->msg, pdu->N_bytes);
      }

      // 这里需要考虑RLC是TM模式还是UM模式
      //  parent->rlc->write_sdu(rnti, srb_to_lcid(lte_srb::srb0), std::move(pdu));
      if (has_iot_5g_s_tmsi)
      {
        // set_up_msg setUp,dl_allocate allocSource
        set_up_msg setup;
        setup.rnti = rnti;
        setup.consIDlen = 48;
        setup.consID = random_value;
        setup.pdu = pdu->msg;
        setup.pduLen = pdu->N_bytes;
        dl_allocate allocSource;
        ul_allocate ulAlloc;
        ulAlloc.bandID = 6;
        ulAlloc.freq = 1;
        ulAlloc.solt = 3;
        ulAlloc.LogicType = SCH2;
        allocSource.bandID = 6;
        allocSource.freq = 1;
        allocSource.solt = 3;
        allocSource.LogicType = SCH1;                        // pdch1-1
        parent->mac->setConsID(setup, allocSource, ulAlloc); // 物联网下5G-S-TMSI Randomvalue都是48bit
      }
    }
    else
    {
      parent->logger.error("Allocating pdu");
    }
  }
  //-----------------------------------------------------------------------------------
  void rrc::ue::send_dl_ccch(dl_ccch_msg_s *dl_ccch_msg, std::string *octet_str)
  {
    // Allocate a new PDU buffer, pack the message and send to PDCP
    srsran::unique_byte_buffer_t pdu = srsran::make_byte_buffer();
    if (pdu)
    {
      asn1::bit_ref bref(pdu->msg, pdu->get_tailroom());
      if (dl_ccch_msg->pack(bref) != asn1::SRSASN_SUCCESS)
      {
        parent->logger.error(pdu->msg, pdu->N_bytes, "Failed to pack DL-CCCH-Msg:");
        return;
      }
      pdu->N_bytes = (uint32_t)bref.distance_bytes();

      // Log Tx message
      parent->log_rrc_message(
          Tx, rnti, srb_to_lcid(lte_srb::srb0), *pdu, *dl_ccch_msg, dl_ccch_msg->msg.c1().type().to_string());

      // Encode the pdu as an octet string if the user passed a valid pointer.
      if (octet_str)
      {
        *octet_str = asn1::octstring_to_string(pdu->msg, pdu->N_bytes);
      }

      parent->rlc->write_sdu(rnti, srb_to_lcid(lte_srb::srb0), std::move(pdu));
    }
    else
    {
      parent->logger.error("Allocating pdu");
    }
  }

  bool rrc::ue::send_dl_dcch(const dl_dcch_msg_s *dl_dcch_msg, srsran::unique_byte_buffer_t pdu, std::string *octet_str)
  {
    if (pdu == nullptr)
    {
      pdu = srsran::make_byte_buffer();
      if (pdu == nullptr)
      {
        parent->logger.error("Allocating pdu");
        return false;
      }
    }

    asn1::bit_ref bref(pdu->msg, pdu->get_tailroom());
    if (dl_dcch_msg->pack(bref) == asn1::SRSASN_ERROR_ENCODE_FAIL)
    {
      parent->logger.error("Failed to encode DL-DCCH-Msg for rnti=0x%x", rnti);
      return false;
    }
    pdu->N_bytes = (uint32_t)bref.distance_bytes();

    lte_srb rb = lte_srb::srb1;
    if (dl_dcch_msg->msg.c1().type() == dl_dcch_msg_type_c::c1_c_::types_opts::dl_info_transfer)
    {
      // send messages with NAS on SRB2 if user is fully registered (after RRC reconfig complete)
      rb = (parent->rlc->has_bearer(rnti, srb_to_lcid(lte_srb::srb2)) && state == RRC_STATE_REGISTERED) ? lte_srb::srb2
                                                                                                        : lte_srb::srb1;
    }

    // Log Tx message
    parent->log_rrc_message(Tx, rnti, srb_to_lcid(rb), *pdu, *dl_dcch_msg, dl_dcch_msg->msg.c1().type().to_string());

    // Encode the pdu as an octet string if the user passed a valid pointer.
    if (octet_str != nullptr)
    {
      *octet_str = asn1::octstring_to_string(pdu->msg, pdu->N_bytes);
    }

    parent->pdcp->write_sdu(rnti, srb_to_lcid(rb), std::move(pdu));
    return true;
  }

  //-----------------------------------2023.10.17-----------------------------------------

  bool rrc::ue::s_sends_dl_dcch(const s_dl_dcch_msg_s *s_dl_dcch_msg,
                                srsran::unique_byte_buffer_t pdu,
                                std::string *octet_str)
  {
    std::cout << "-----This is the delivery dl-dcch-info process--11111111---" << std::endl;
    pdu = srsran::make_byte_buffer();
    asn1::bit_ref bref(pdu->msg, pdu->get_tailroom());
    if (s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg.size() == 4)
    {
      for (uint32_t i = 0; i < 4; i++)
      {
        std::cout << "s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[" << i << "].band_id.ba_id.to_number():" << s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].band_id.ba_id.to_number() << std::endl;
      }
    }
    else if (s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg.size() == 8)
    {
      for (uint32_t i = 0; i < 8; i++)
      {
        std::cout << "s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[" << i << "].band_id.ba_id.to_number():" << s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].band_id.ba_id.to_number() << std::endl;
      }
    }
    else
    {
      std::cout << " CC 1 " << std::endl;
    }
    if (s_dl_dcch_msg->pack(bref) == asn1::SRSASN_ERROR_ENCODE_FAIL)
    {
      parent->logger.error("Failed to encode DL-DCCH-Msg for rnti=0x%x", rnti);
      std::cout << "Failed to encode DL-DCCH-Msg" << std::endl;
      return false;
    }
    pdu->N_bytes = (uint32_t)bref.distance_bytes();

    std::cout << "pdu->N_bytes" << pdu->N_bytes << std::endl;
    for (uint32_t i = 0; i < pdu->N_bytes; i++)
    {
      printf("0x%x\n", *(pdu->msg + i));
    }
    // if(parent->rrc_adp->udp_.TC_716_reconf_fail_last)
    // {
    //   printf("parent->rrc_adp->udp_.TC_716_reconf_fail_last\n");
    //   pdu->msg[pdu->N_bytes-7]=0xab;
    //   pdu->msg[pdu->N_bytes-6]=0xcd;
    //   pdu->msg[pdu->N_bytes-5]=0xab;
    //   pdu->msg[pdu->N_bytes-4]=0xcd;
    //   pdu->msg[pdu->N_bytes-3]=0xaf;
    //   pdu->msg[pdu->N_bytes-2]=0xff;
    //   pdu->msg[pdu->N_bytes-1]=0xfa;
    //   pdu->msg[pdu->N_bytes]=0x18;
    //   for (uint32_t i = 0; i < pdu->N_bytes; i++)
    //   {
    //     printf("0x%x\n", *(pdu->msg + i));
    //   }
    // }
    // Log Tx message
    parent->log_rrc_message(
        Tx, rnti, srb_to_lcid(lte_srb::srb1), *pdu, *s_dl_dcch_msg, s_dl_dcch_msg->msg.type().to_string());
    // Encode the pdu as an octet string if the user passed a valid pointer.
    if (octet_str != nullptr)
    {
      *octet_str = asn1::octstring_to_string(pdu->msg, pdu->N_bytes);
    }

    srsran::rrc_pcap_net *p_pcap_net = getPcapNet(*parent);
    if (p_pcap_net)
    {
      p_pcap_net->write_dl_rrc_pdu(pdu->msg, pdu->N_bytes, CY_LOGICCHANNEL_TYPE_DL_DCCH, CY_NET_MODE_RAN);
    }

    parent->pdcp->write_sdu(rnti, srb_to_lcid(lte_srb::srb1), std::move(pdu));

    // //2024.11.12 to notify PHY resolution in advance
    // int temp_carrNum = s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg.size();
    // parent->mac->updateUpInfo_advanceSingle();
    // parent->mac->updateUpInfo_advanceMulti();

    // if (s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].rlc_cfg.type() == rlc_cofg_c::types_opts::am)
    // {
    //   parent->pdcp->write_sdu(rnti, srb_to_lcid(lte_srb::srb1), std::move(pdu));
    // }
    // if (s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].rlc_cfg.type() == rlc_cofg_c::types_opts::tm ||
    //     s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].rlc_cfg.type() == rlc_cofg_c::types_opts::um_bi_dir)
    // {

    //   parent->rlc->write_sdu(rnti, srb_to_lcid(lte_srb::srb0), std::move(pdu));
    // }

    auto start = std::chrono::high_resolution_clock::now();
    if ((ChanType_t)(int)s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type == 6)
    {
      std::cout << "This psch 52" << std::endl;
      while (true)
      {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        if (duration.count() > 250)
        {
          break;
        }
      }
    }
    else
    {
      std::cout << "This else" << std::endl;
      while (true)
      {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        if (duration.count() > 610)
        {
          break;
        }
      }
    }

    // 定义物理层配置的消息内容reconf_phy_config
    ul_allocate reconf_allo;
    dl_allocate reconf_sp_allo;

    if (wx_area_mode == 0)
    {

      uint32_t m = s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg.size();
      phy_channel_ul_list_t phy_ul_channel_list;
      phy_channel_list_t phy_multi_carr;
      phy_multi_carr.resize(m);
      printf("---m----::%d\n", m);
      if (m > 1)
      {
        std::cout << "enter muleti" << std::endl;

        for (uint32_t i = 0; i < m; i++)
        {
          phy_multi_carr[i].phy_channel.bandID = s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].band_id.ba_id.to_number();
          phy_multi_carr[i].phy_channel.direct_opt = (direct_opt_t)(int)s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].direc_t;
          phy_multi_carr[i].phy_channel.freqID = s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].freq_id.freq_id.to_number();
          phy_multi_carr[i].phy_channel.sche_type_opt = (sche_type_opts_t)(int)s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].sche_type;
          phy_multi_carr[i].phy_channel.slot = s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].slot_ass.to_number();
          phy_multi_carr[i].phy_channel.srnti = s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].s_rnti.srnti.to_number();
          phy_multi_carr[i].phy_channel.Type = (ChanType_t)(int)s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].chan_type;
          uint8_t solot = s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].slot_ass.to_number();
          std::cout << "phy_multi_carr[i].phy_channel.slot = " << phy_multi_carr[i].phy_channel.slot << std::endl;
          // int n = 0;
          // while ((solot & 0x01) == 0)
          // {
          //   solot = solot >> 1;
          //   n++;
          // }
          // phy_multi_carr[i].phy_channel.slot = n;
        }
        std::cout << "mmm-xxxxSRB_configMap.dl_Type" << SRB_configMap.dl_Type << std::endl;
        std::cout << "mmm-xxxxSRB_configMap.ul_Type:" << SRB_configMap.ul_Type << std::endl;
        // parent->mac->updateMap(1, update_ul_Type, update_dl_Type);
        parent->mac->updateMap(1, SRB_configMap);
        std::cout << "mmm-DRB_configMap.dl_Type:" << DRB_configMap.dl_Type << std::endl;
        std::cout << "mmm-DRB_configMap.ul_Type:" << DRB_configMap.ul_Type << std::endl;

        parent->mac->updateMap(s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].drb_id, DRB_configMap);
        std::cout << "mmm-MCS=" << RRC_mcs_Info.MCS << std::endl;
        std::cout << "mmm-@@@ zzz DL Type=" << reconf_sp_allo.Type << std::endl;
        parent->mac->phy_channel_list_conf(phy_multi_carr, phy_ul_channel_list, RRC_mcs_Info);
        parent->is_multi = true;
        std::cout << "set  parent->is_multi " << parent->is_multi << std::endl;
      }
      if (m == 1)
      {
        reconf_sp_allo.bandID = s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].band_id.ba_id.to_number();
        reconf_sp_allo.freq = s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].freq_id.freq_id.to_number();
        reconf_sp_allo.Type = (ChanType_t)(int)s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type;
        // uint8_t solt
        reconf_sp_allo.solt = s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.to_number();
        // int n = 0;
        // while ((solt & 0x01) == 0)
        // {
        //   solt = solt >> 1;
        //   n++;
        // }
        // reconf_sp_allo.solt = n;

        /**
         *@brief   update map parameters(updateMap need to change all map and entity map) area_mode = 0 ul and dl is same
         *@author zhaohongjun
         *@date   2024/05/17
         */
        /*
        re_configMap.dl_Type = static_cast<ChanType_t>(s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
        re_configMap.ul_Type = static_cast<ChanType_t>(s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value); // rr_cfg_ded.drb_to_add_mod_list[0]
        if (s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.rlc_am_present)
        {
          re_configMap.rlcType = rlc_am;
        }
        else if (s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].pdcp_cfg.rlc_um_present)
        {
          re_configMap.rlcType = rlc_um;
        }
        else
        {
        }*/

        /*
        ChanType_t update_ul_Type;
        ChanType_t update_dl_Type;
        update_ul_Type = static_cast<ChanType_t>(s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
        update_dl_Type = static_cast<ChanType_t>(s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
        */

        // parent->mac->updateMap(1, update_ul_Type, update_dl_Type);
        parent->mac->updateMap(1, SRB_configMap);
        std::cout << "DRB_configMap.dl_Type:" << DRB_configMap.dl_Type << std::endl;
        std::cout << "DRB_configMap.ul_Type:" << DRB_configMap.ul_Type << std::endl;

        if (s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_rel_list_present)
        {
          std::cout << "xxk 111" << std::endl;
        }
        else
        {
          parent->mac->updateMap(s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].drb_id, DRB_configMap);
        }

        std::cout << "MCS=" << RRC_mcs_Info.MCS << std::endl;
        std::cout << "@@@ zzz DL Type=" << reconf_sp_allo.Type << std::endl;
        parent->mac->reconf_phy(reconf_sp_allo, reconf_allo, RRC_mcs_Info);
      }
    }

    if (wx_area_mode == 1)
    {
      reconf_allo.Type = (ChanType_t)(int)s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type;
      reconf_allo.bandID = s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].band_id.ba_id.to_number();
      reconf_allo.freq = s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].freq_id.freq_id.to_number();
      reconf_allo.solt = s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.to_number();
      // int m = 0;
      // while ((solt & 0x01) == 0)
      // {
      //   solt = solt >> 1;
      //   m++;
      // }

      // reconf_allo.solt = m;
      // std::cout << "---------normal----------test recf------" << m << std::endl;

      reconf_sp_allo.bandID = s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].band_id.ba_id.to_number();
      reconf_sp_allo.freq = s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].freq_id.freq_id.to_number();
      reconf_sp_allo.Type = (ChanType_t)(int)s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type;
      reconf_sp_allo.SF_i = s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].pdtch_code.pdt_phy_code;
      // uint8_t slot
      reconf_sp_allo.solt = s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].slot_ass.to_number();
      // int n = 0;
      // while ((slot & 0x01) == 0)
      // {
      //   slot = slot >> 1;
      //   n++;
      // }
      // reconf_sp_allo.solt = n;

      /**
       *@brief   update map parameters(updateMap need to change all map and entity map) area_mode = 1 ul not change, dl is ds
       *@author zhaohongjun
       *@date   2024/05/17
       */

      /*
      ChanType_t update_ul_Type;
      ChanType_t update_dl_Type;
      update_ul_Type = static_cast<ChanType_t>(s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.value);
      update_dl_Type = static_cast<ChanType_t>(s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type.value);
      parent->mac->updateMap(1, update_ul_Type, update_dl_Type);*/
      parent->mac->updateMap(1, SRB_configMap);
      parent->mac->updateMap(s_dl_dcch_msg->msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[0].drb_id, DRB_configMap);
      std::cout << "DS MCS=" << RRC_mcs_Info.MCS << std::endl;

      parent->mac->reconf_phy(reconf_sp_allo, reconf_allo, RRC_mcs_Info);
    }

    std::cout << "------jianchadiyige   first  chantype---------------" << reconf_sp_allo.Type << std::endl;
    return true;
  }

  bool rrc::ue::s_send_dl_dcch(const s_dl_dcch_msg_s *s_dl_dcch_msg,
                               srsran::unique_byte_buffer_t pdu,
                               std::string *octet_str)
  {
    std::cout << "-----This is the delivery dl-dcch-info process-----" << std::endl;
    // for(uint8_t i=0;i<pdu->N_bytes;i++){
    //   printf("%x\n",*(pdu->msg+i));
    // }

    pdu = srsran::make_byte_buffer();
    asn1::bit_ref bref(pdu->msg, pdu->get_tailroom());
    if (s_dl_dcch_msg->pack(bref) == asn1::SRSASN_ERROR_ENCODE_FAIL)
    {
      parent->logger.error("Failed to encode DL-DCCH-Msg for rnti=0x%x", rnti);
      return false;
    }
    pdu->N_bytes = (uint32_t)bref.distance_bytes();

    // std::cout<<"@@@@@@@@@@@@@------------rrc::ue::s_send_dl_dcch-------------@@@@@@@@@@"<<std::endl;
    std::cout << "pdu->N_bytes" << pdu->N_bytes << std::endl;
    for (uint32_t i = 0; i < pdu->N_bytes; i++)
    {
      printf("0x%x\n", *(pdu->msg + i));
    }

    // Log Tx message
    parent->log_rrc_message(
        Tx, rnti, srb_to_lcid(lte_srb::srb1), *pdu, *s_dl_dcch_msg, s_dl_dcch_msg->msg.type().to_string());
    // Encode the pdu as an octet string if the user passed a valid pointer.
    if (octet_str != nullptr)
    {
      *octet_str = asn1::octstring_to_string(pdu->msg, pdu->N_bytes);
    }

    srsran::rrc_pcap_net *p_pcap_net = getPcapNet(*parent);
    if (p_pcap_net)
    {
      p_pcap_net->write_dl_rrc_pdu(pdu->msg, pdu->N_bytes, CY_LOGICCHANNEL_TYPE_DL_DCCH, CY_NET_MODE_RAN);
    }

    parent->pdcp->write_sdu(rnti, srb_to_lcid(lte_srb::srb1), std::move(pdu));

    return true;
  }
  //-----------------------------------------------------------------------------------------
  //-------------------------------------2023.10.31------------------------------------------
  bool rrc::ue::iot_send_dl_dcch(const iot_dl_dcch_msg_s *iot_dl_dcch_msg,
                                 srsran::unique_byte_buffer_t pdu,
                                 std::string *octet_str)
  {
    if (pdu == nullptr)
    {
      pdu = srsran::make_byte_buffer();
      if (pdu == nullptr)
      {
        parent->logger.error("Allocating pdu");
        return false;
      }
    }

    asn1::bit_ref bref(pdu->msg, pdu->get_tailroom());
    if (iot_dl_dcch_msg->pack(bref) == asn1::SRSASN_ERROR_ENCODE_FAIL)
    {
      parent->logger.error("Failed to encode DL-DCCH-Msg for rnti=0x%x", rnti);
      return false;
    }
    pdu->N_bytes = (uint32_t)bref.distance_bytes();

    // Log Tx message
    parent->log_rrc_message(
        Tx, rnti, srb_to_lcid(lte_srb::srb1), *pdu, *iot_dl_dcch_msg, iot_dl_dcch_msg->msg.type().to_string());

    // Encode the pdu as an octet string if the user passed a valid pointer.
    if (octet_str != nullptr)
    {
      *octet_str = asn1::octstring_to_string(pdu->msg, pdu->N_bytes);
    }

    parent->pdcp->write_sdu(rnti, srb_to_lcid(lte_srb::srb1), std::move(pdu));
    return true;
  }
  //-----------------------------------------------------------------------------------------

  void rrc::ue::apply_setup_phy_common(const asn1::rrc::rr_cfg_common_sib_s &config, bool update_phy)
  {
    // Return if no cell is supported
    if (phy_rrc_dedicated_list.empty())
    {
      return;
    }

    // Flatten common configuration
    auto &current_phy_cfg = phy_rrc_dedicated_list[0].phy_cfg;
    set_phy_cfg_t_common_prach(&current_phy_cfg, &config.prach_cfg.prach_cfg_info, config.prach_cfg.root_seq_idx);
    set_phy_cfg_t_common_pdsch(&current_phy_cfg, config.pdsch_cfg_common);
    set_phy_cfg_t_common_pusch(&current_phy_cfg, config.pusch_cfg_common);
    set_phy_cfg_t_common_pucch(&current_phy_cfg, config.pucch_cfg_common);
    set_phy_cfg_t_common_srs(&current_phy_cfg, config.srs_ul_cfg_common);
    set_phy_cfg_t_common_pwr_ctrl(&current_phy_cfg, config.ul_pwr_ctrl_common);

    // Set PCell index
    phy_rrc_dedicated_list[0].configured = true;
    phy_rrc_dedicated_list[0].enb_cc_idx = get_ue_cc_cfg(UE_PCELL_CC_IDX)->enb_cc_idx;

    // Send configuration to physical layer
    if (parent->phy != nullptr and update_phy)
    {
      parent->phy->set_config(rnti, phy_rrc_dedicated_list);
    }
  }

  void rrc::ue::apply_setup_phy_config_dedicated(const asn1::rrc::phys_cfg_ded_s &phys_cfg_ded)
  {
    // Return if no cell is supported
    if (phy_rrc_dedicated_list.empty())
    {
      return;
    }

    // Load PCell dedicated configuration
    srsran::set_phy_cfg_t_dedicated_cfg(&phy_rrc_dedicated_list[0].phy_cfg, phys_cfg_ded);

    // Deactivates eNb/Cells for this UE
    for (uint32_t cc = 1; cc < phy_rrc_dedicated_list.size(); cc++)
    {
      phy_rrc_dedicated_list[cc].configured = false;
    }

    // Send configuration to physical layer
    if (parent->phy != nullptr)
    {
      parent->phy->set_config(rnti, phy_rrc_dedicated_list);
    }
  }

  //-------------------------2023.10.24-------------------
  void rrc::ue::s_apply_reconf_phy_config(rrc_con_recfg_r1_ies_s &reconfig_r1)
  {
    parent->phy->s_set_config(rnti, phy_rrc_dedicated_list);
  }
  //-------------------------------------------------------------------

  void rrc::ue::apply_reconf_phy_config(const rrc_conn_recfg_r8_ies_s &reconfig_r8, bool update_phy)
  {
    // Return if no cell is supported
    if (phy_rrc_dedicated_list.empty())
    {
      return;
    }

    // Configure PCell if available configuration
    if (reconfig_r8.rr_cfg_ded_present)
    {
      auto &rr_cfg_ded = reconfig_r8.rr_cfg_ded;
      if (rr_cfg_ded.phys_cfg_ded_present)
      {
        auto &phys_cfg_ded = rr_cfg_ded.phys_cfg_ded;
        srsran::set_phy_cfg_t_dedicated_cfg(&phy_rrc_dedicated_list[0].phy_cfg, phys_cfg_ded);
        srsran::set_phy_cfg_t_enable_64qam(
            &phy_rrc_dedicated_list[0].phy_cfg,
            ue_capabilities.support_ul_64qam and
                parent->cfg.sibs[1].sib2().rr_cfg_common.pusch_cfg_common.pusch_cfg_basic.enable64_qam);
      }
    }

    // Parse extensions
    if (reconfig_r8.non_crit_ext_present)
    {
      auto &reconfig_r890 = reconfig_r8.non_crit_ext;
      if (reconfig_r890.non_crit_ext_present)
      {
        auto &reconfig_r920 = reconfig_r890.non_crit_ext;
        if (reconfig_r920.non_crit_ext_present)
        {
          auto &reconfig_r1020 = reconfig_r920.non_crit_ext;

          // Handle Add/Modify SCell list
          if (reconfig_r1020.scell_to_add_mod_list_r10_present)
          {
            auto &list = reconfig_r1020.scell_to_add_mod_list_r10;
            phy_rrc_dedicated_list.resize(ue_cell_list.nof_cells());
            for (const scell_to_add_mod_r10_s &scell : list)
            {
              ue_cell_ded *ue_cc = ue_cell_list.get_ue_cc_idx(scell.scell_idx_r10);
              // Create new PHY configuration structure for this SCell
              phy_interface_rrc_lte::phy_rrc_cfg_t scell_phy_rrc_ded = {};
              srsran::set_phy_cfg_t_scell_config(&scell_phy_rrc_ded.phy_cfg, scell);
              scell_phy_rrc_ded.configured = true;

              // Set PUSCH dedicated configuration following 3GPP TS 36.331 R 10 Section 6.3.2 Radio resource control
              // information elements - PUSCH-Config
              //   One value applies for all serving cells with an uplink (the associated functionality is common i.e. not
              //   performed independently for each cell).
              scell_phy_rrc_ded.phy_cfg.ul_cfg.pusch.uci_offset =
                  phy_rrc_dedicated_list[0].phy_cfg.ul_cfg.pusch.uci_offset;

              // Get corresponding eNB CC index
              scell_phy_rrc_ded.enb_cc_idx = ue_cc->cell_common->enb_cc_idx;

              // Append to PHY RRC config dedicated which will be applied further down
              phy_rrc_dedicated_list[scell.scell_idx_r10] = scell_phy_rrc_ded;
              srsran::set_phy_cfg_t_enable_64qam(
                  &phy_rrc_dedicated_list[scell.scell_idx_r10].phy_cfg,
                  ue_capabilities.support_ul_64qam and
                      parent->cfg.sibs[1].sib2().rr_cfg_common.pusch_cfg_common.pusch_cfg_basic.enable64_qam);
            }
          }
        }
      }
    }

    // Send configuration to physical layer
    if (parent->phy != nullptr and update_phy)
    {
      parent->phy->set_config(rnti, phy_rrc_dedicated_list);
    }
  }

  //------------------------------------2023.10.13--------------------------------------------
  void rrc::ue::s_apply_pdcp_srb_updates()
  {
    parent->pdcp->add_bearer(rnti, (uint32_t)1, srsran::make_srb_pdcp_config_t((uint32_t)1, false));
    if (ue_security_cfg.is_as_sec_cfg_valid())
    {
      parent->pdcp->config_security(rnti, (uint32_t)1, ue_security_cfg.get_as_sec_cfg());
      parent->pdcp->enable_integrity(rnti, (uint32_t)1);
      parent->pdcp->enable_encryption(rnti, (uint32_t)1);
    }
  }
  //---------------------------------------------------------------------------------------------

  //-------------------------------------------2023.10.13------------------------------------
  void rrc::ue::s_apply_pdcp_drb_updates(drb_to_add_modi_s &drb_to_add)
  {
    // drb

    drb_to_add.pdcp_cfg_present = true;
    srsran::pdcp_confg_t pdcp_cnfg_drb = srsran::make_drb_pdcp_confg_t(drb_to_add.drb_id, drb_to_add.pdcp_cfg);
    if (drb_to_add.pdcp_cfg.rlc_tm_present)
    {
      pdcp_cnfg_drb.is_tm = true;
    }
    parent->pdcp->add_bearerer(rnti, drb_to_add.drb_id, pdcp_cnfg_drb);

    if (ue_security_cfg.is_as_sec_cfg_valid())
    {
      parent->pdcp->config_security(rnti, drb_to_add.drb_id, ue_security_cfg.get_as_sec_cfg());
      parent->pdcp->enable_integrity(rnti, drb_to_add.drb_id);
      parent->pdcp->enable_encryption(rnti, drb_to_add.drb_id);
    }
  }
  //-----------------------------------------------------------------------------------------
  // ##############################################
  void rrc::ue::apply_pdcp_srb_updates()
  {
    // enable security config
    if (ue_security_cfg.is_as_sec_cfg_valid())
    {
      parent->pdcp->config_security(rnti, (uint32_t)1, ue_security_cfg.get_as_sec_cfg());
      parent->pdcp->enable_integrity(rnti, (uint32_t)1);
      parent->pdcp->enable_encryption(rnti, (uint32_t)1);
    }
    parent->pdcp->add_bearer(rnti, (uint32_t)1, srsran::make_srb_pdcp_config_t((uint32_t)1, false));
  }
  // ######################################
  void rrc::ue::apply_pdcp_srb_updates(const rr_cfg_ded_s &pending_rr_cfg)
  {
    for (const srb_to_add_mod_s &srb : pending_rr_cfg.srb_to_add_mod_list)
    {
      parent->pdcp->add_bearer(rnti, srb.srb_id, srsran::make_srb_pdcp_config_t(srb.srb_id, false));
      // enable security config
      if (ue_security_cfg.is_as_sec_cfg_valid())
      {
        parent->pdcp->config_security(rnti, srb.srb_id, ue_security_cfg.get_as_sec_cfg());
        parent->pdcp->enable_integrity(rnti, srb.srb_id);
        parent->pdcp->enable_encryption(rnti, srb.srb_id);
      }
    }
  }

  void rrc::ue::apply_pdcp_drb_updates(const rr_cfg_ded_s &pending_rr_cfg)
  {
    for (uint8_t drb_id : pending_rr_cfg.drb_to_release_list)
    {
      parent->pdcp->del_bearer(rnti, drb_to_lcid((lte_drb)drb_id));
    }
    for (const drb_to_add_mod_s &drb : pending_rr_cfg.drb_to_add_mod_list)
    {
      // Configure DRB1 in PDCP
      if (drb.pdcp_cfg_present)
      {
        srsran::pdcp_config_t pdcp_cnfg_drb = srsran::make_drb_pdcp_config_t(drb.drb_id, false, drb.pdcp_cfg);
        parent->pdcp->add_bearer(rnti, drb.lc_ch_id, pdcp_cnfg_drb);
      }
      else
      {
        srsran::pdcp_config_t pdcp_cnfg_drb = srsran::make_drb_pdcp_config_t(drb.drb_id, false);
        parent->pdcp->add_bearer(rnti, drb.lc_ch_id, pdcp_cnfg_drb);
      }

      if (ue_security_cfg.is_as_sec_cfg_valid())
      {
        parent->pdcp->config_security(rnti, drb.lc_ch_id, ue_security_cfg.get_as_sec_cfg());
        parent->pdcp->enable_integrity(rnti, drb.lc_ch_id);
        parent->pdcp->enable_encryption(rnti, drb.lc_ch_id);
      }
    }

    // If reconf due to reestablishment, recover PDCP state
    if (state == RRC_STATE_REESTABLISHMENT_COMPLETE)
    {
      for (const auto &erab_pair : bearer_list.get_erabs())
      {
        uint16_t lcid = erab_pair.second.lcid;
        bool is_am = parent->cfg.qci_cfg[erab_pair.second.qos_params.qci].rlc_cfg.type().value ==
                     asn1::rrc::rlc_cfg_c::types_opts::am;
        if (is_am)
        {
          parent->logger.debug("Set PDCP state: TX HFN %d, NEXT_PDCP_TX_SN %d, RX_HFN %d, NEXT_PDCP_RX_SN %d, "
                               "LAST_SUBMITTED_PDCP_RX_SN %d",
                               old_reest_pdcp_state[lcid].tx_hfn,
                               old_reest_pdcp_state[lcid].next_pdcp_tx_sn,
                               old_reest_pdcp_state[lcid].rx_hfn,
                               old_reest_pdcp_state[lcid].next_pdcp_rx_sn,
                               old_reest_pdcp_state[lcid].last_submitted_pdcp_rx_sn);
          parent->pdcp->set_bearer_state(rnti, lcid, old_reest_pdcp_state[lcid]);
          parent->pdcp->set_bearer_state(rnti, lcid, old_reest_pdcp_state[lcid]);
          if (parent->cfg.qci_cfg[erab_pair.second.qos_params.qci].pdcp_cfg.rlc_am.status_report_required)
          {
            parent->pdcp->send_status_report(rnti, lcid);
          }
        }
      }
    }
  }

  void rrc::ue::xw_apply_rlc_rb_updates(asn1::rrc::redio_resour_cfg_dedi_s &pending_rr_cfg)
  {
    if (pending_rr_cfg.drb_to_rel_list.size() > 0)
    {
      for (const asn1::rrc::drb_id_s &release_drb_id : pending_rr_cfg.drb_to_rel_list)
      {
        printf("[RRC_UE][xw_apply_rlc_rb_updates] Release rlc entity");
        parent->rlc->del_bearer(rnti, release_drb_id.drb_id);

        // deregister EPS bearer
        uint8_t eps_bearer_id = parent->bearer_manager.get_lcid_bearer(rnti, release_drb_id.drb_id).eps_bearer_id;
        parent->bearer_manager.remove_eps_bearer(rnti, eps_bearer_id);
      }
    }

    for (const drb_to_add_modi_s &drb_add : pending_rr_cfg.drb_to_add_mod_list)
    {
      printf("[RRC_UE][xw_apply_rlc_rb_updates]- first reconf-\n");
      if (not drb_add.rlc_cfg_present)
        printf("Default RLC DRB config not supported");

      srsran::rlc_config_t rlc_cfg = srsran::make_rlc_confg_t(drb_add.rlc_cfg);

      printf("[RRC_UE][xw_apply_rlc_rb_updates]rlc_cfg um t_record = %d\n", (int)rlc_cfg.aum.t_reord );
      printf("[RRC_UE][xw_apply_rlc_rb_updates]rlc_cfg am t_record = %d\n", (int)rlc_cfg.asam.t_reord );

      parent->rlc->add_bearer(rnti, drb_add.drb_id, rlc_cfg);

      if (drb_add.drb_id == 3)
      {
        if (drb_add.rlc_cfg.type()==rlc_cofg_c::types_opts::am)
        {
          parent->cfg.rlc_mode = 0;
        }
        else if(drb_add.rlc_cfg.type()==rlc_cofg_c::types_opts::um_bi_dir)
        {
          parent->cfg.rlc_mode = 1;
        }
        else
        {
          parent->cfg.rlc_mode = 2;
        }
      }
      printf("[RRC_UE][xw_apply_rlc_rb_updates]apply rlc_mode:%d\n",parent->cfg.rlc_mode);
      parent->bearer_manager.add_eps_bearer(rnti, (uint32_t)0, srsran::srsran_rat_t::lte, drb_add.drb_id);
    }
  }

  void rrc::ue::xw_apply_pdcp_drb_updates(asn1::rrc::redio_resour_cfg_dedi_s &pending_rr_cfg)
  {
    for (asn1::rrc::drb_id_s drb_id_release : pending_rr_cfg.drb_to_rel_list)
    {
      std::cout << " PDCP RELEASE " << std::endl;
      parent->pdcp->del_bearer(rnti, drb_id_release.drb_id);
    }

    for (const drb_to_add_modi_s &xw_pdcp_drb_add : pending_rr_cfg.drb_to_add_mod_list)
    {
      srsran::pdcp_confg_t pdcp_cnfg_drb = srsran::make_drb_pdcp_confg_t(xw_pdcp_drb_add.drb_id, xw_pdcp_drb_add.pdcp_cfg);
      if (xw_pdcp_drb_add.pdcp_cfg.rlc_tm_present)
      {
        pdcp_cnfg_drb.is_tm = true;
      }
      parent->pdcp->add_bearerer(rnti, xw_pdcp_drb_add.drb_id, pdcp_cnfg_drb);

      if (ue_security_cfg.is_as_sec_cfg_valid())
      {
        parent->pdcp->config_security(rnti, xw_pdcp_drb_add.drb_id, ue_security_cfg.get_as_sec_cfg());
        parent->pdcp->enable_integrity(rnti, xw_pdcp_drb_add.drb_id);
        parent->pdcp->enable_encryption(rnti, xw_pdcp_drb_add.drb_id);
      }
    }
  }

  ///-------------------2023.10.13---------------------------------------------------------------
  void rrc::ue::s_apply_rlc_rb_updates(drb_to_add_modi_s &drb_to_add)
  {
    // drb
    std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@2" << std::endl;
    drb_to_add.rlc_cfg_present = true;
    srsran::rlc_config_t rlc_cofg = srsran::make_rlc_confg_t(drb_to_add.rlc_cfg);
    std::cout << "rlc_cofg um t_record = " << (int)rlc_cofg.aum.t_reord << std::endl;
    std::cout << "rlc_cofg am t_record = " << (int)rlc_cofg.asam.t_reord << std::endl;
    std::cout << "drb_to_add.drb_id=" << drb_to_add.drb_id << std::endl;
    parent->rlc->add_bearer(rnti, drb_to_add.drb_id, rlc_cofg);

    if (drb_to_add.rlc_cfg.type() == rlc_cofg_c::types_opts::am)
    {
      parent->cfg.rlc_mode = 0;
    }
    else if (drb_to_add.rlc_cfg.type() == rlc_cofg_c::types_opts::um_bi_dir)
    {
      parent->cfg.rlc_mode = 1;
    }
    else
    {
      parent->cfg.rlc_mode = 2;
    }
    printf("apply rlc_mode:%d" ,parent->cfg.rlc_mode);
    std::cout << " zhj test 55 " << std::endl;
    parent->bearer_manager.add_eps_bearer(rnti, (uint32_t)0, srsran::srsran_rat_t::lte, drb_to_add.drb_id);
  }
  //-------------------------------------------------------------------------------------------------

  // #################################################
  void rrc::ue::apply_rlc_srb_updates()
  {
    srsran::rlc_config_t rlc_cfg = srsran::rlc_config_t::s_srb_config((uint32_t)1);
    srb_cfg_t *srb_cfg = &parent->cfg.srb1_cfg;
    if (rlc_cfg.rlc_mode == srsran::rlc_mode_t::am and srb_cfg->enb_dl_max_retx_thres > 0)
    {
      rlc_cfg.am.max_retx_thresh = srb_cfg->enb_dl_max_retx_thres;
    }
    std::cout << "rlc_cfg.asam.t_reord=" << (int)rlc_cfg.asam.t_reord << std::endl;
    parent->rlc->add_bearer(rnti, (uint32_t)1, rlc_cfg);
  }

  void rrc::ue::iot_apply_rlc_srb_updates()
  {
    srsran::rlc_config_t rlc_cfg = srsran::rlc_config_t::default_iot_rlc_um_config();
    srb_cfg_t *srb_cfg = &parent->cfg.srb1_cfg;
    parent->rlc->add_bearer(rnti, (uint32_t)1, rlc_cfg);
  }

  // #####################################################

  void rrc::ue::apply_rlc_rb_updates(const rr_cfg_ded_s &pending_rr_cfg)
  {
    // std::cout<<"---------------------------------------------------3---------------------------"<<std::endl;
    for (const srb_to_add_mod_s &srb : pending_rr_cfg.srb_to_add_mod_list)
    {
      srb_cfg_t *srb_cfg;
      if (srb.srb_id == 1)
      {
        srb_cfg = &parent->cfg.srb1_cfg;
      }
      else if (srb.srb_id == 2)
      {
        srb_cfg = &parent->cfg.srb2_cfg;
      }
      else
      {
        srsran_assertion_failure("Invalid LTE SRB id=%d", srb.srb_id);
      }

      if (srb_cfg->rlc_cfg.type() == srb_to_add_mod_s::rlc_cfg_c_::types_opts::explicit_value)
      {
        srsran::rlc_config_t rlc_cfg = srsran::make_rlc_config_t(srb_cfg->rlc_cfg.explicit_value());
        if (rlc_cfg.rlc_mode == srsran::rlc_mode_t::am and srb_cfg->enb_dl_max_retx_thres > 0)
        {
          rlc_cfg.am.max_retx_thresh = srb_cfg->enb_dl_max_retx_thres;
        }
        parent->rlc->add_bearer(rnti, srb.srb_id, rlc_cfg);
      }
      else
      {
        srsran::rlc_config_t rlc_cfg = srsran::rlc_config_t::srb_config(srb.srb_id);
        if (rlc_cfg.rlc_mode == srsran::rlc_mode_t::am and srb_cfg->enb_dl_max_retx_thres > 0)
        {
          rlc_cfg.am.max_retx_thresh = srb_cfg->enb_dl_max_retx_thres;
        }
        parent->rlc->add_bearer(rnti, srb.srb_id, rlc_cfg);
      }
    }

    if (pending_rr_cfg.drb_to_release_list.size() > 0)
    {
      for (uint8_t drb_id : pending_rr_cfg.drb_to_release_list)
      {
        parent->rlc->del_bearer(rnti, drb_to_lcid((lte_drb)drb_id));

        // deregister EPS bearer
        uint8_t eps_bearer_id = parent->bearer_manager.get_lcid_bearer(rnti, drb_to_lcid((lte_drb)drb_id)).eps_bearer_id;
        parent->bearer_manager.remove_eps_bearer(rnti, eps_bearer_id);
      }
    }
    for (const drb_to_add_mod_s &drb : pending_rr_cfg.drb_to_add_mod_list)
    {
      if (not drb.rlc_cfg_present)
      {
        parent->logger.warning("Default RLC DRB config not supported");
      }
      srsran::rlc_config_t rlc_cfg = srsran::make_rlc_config_t(drb.rlc_cfg);
      const bearer_cfg_handler::erab_t &erab = bearer_list.get_erabs().at(drb.eps_bearer_id);
      if (rlc_cfg.rlc_mode == srsran::rlc_mode_t::am and
          parent->cfg.qci_cfg.at(erab.qos_params.qci).enb_dl_max_retx_thres > 0)
      {
        rlc_cfg.am.max_retx_thresh = parent->cfg.qci_cfg.at(erab.qos_params.qci).enb_dl_max_retx_thres;
      }
      parent->rlc->add_bearer(rnti, drb.lc_ch_id, rlc_cfg);

      // register EPS bearer over LTE PDCP
      parent->bearer_manager.add_eps_bearer(rnti, drb.eps_bearer_id, srsran::srsran_rat_t::lte, drb.lc_ch_id);
    }
  }

  int rrc::ue::get_cqi(uint16_t *pmi_idx, uint16_t *n_pucch, uint32_t ue_cc_idx)
  {
    ue_cell_ded *c = ue_cell_list.get_ue_cc_idx(ue_cc_idx);
    if (c != nullptr and c->cqi_res_present)
    {
      *pmi_idx = c->cqi_res.pmi_idx;
      *n_pucch = c->cqi_res.pucch_res;
      return SRSRAN_SUCCESS;
    }
    else
    {
      parent->logger.error("CQI resources for ue_cc_idx=%d have not been allocated", ue_cc_idx);
      return SRSRAN_ERROR;
    }
  }

  bool rrc::ue::is_allocated() const
  {
    return ue_cell_list.is_allocated();
  }

  int rrc::ue::get_ri(uint32_t m_ri, uint16_t *ri_idx)
  {
    int32_t ret = SRSRAN_SUCCESS;

    uint32_t I_ri = 0;
    int32_t N_offset_ri = 0; // Naivest approach: overlap RI with PMI
    switch (m_ri)
    {
    case 0:
      // Disabled
      break;
    case 1:
      I_ri = -N_offset_ri;
      break;
    case 2:
      I_ri = 161 - N_offset_ri;
      break;
    case 4:
      I_ri = 322 - N_offset_ri;
      break;
    case 8:
      I_ri = 483 - N_offset_ri;
      break;
    case 16:
      I_ri = 644 - N_offset_ri;
      break;
    case 32:
      I_ri = 805 - N_offset_ri;
      break;
    default:
      parent->logger.error("Allocating RI: invalid m_ri=%d", m_ri);
    }

    // If ri_dix is available, copy
    if (ri_idx)
    {
      *ri_idx = I_ri;
    }

    return ret;
  }

  void rrc::assemble_general_interface(srsran::unique_byte_buffer_t *general_interface_, uint16_t data_len)
  {
    general_interface_->get()->msg[0] = 0x01;
    general_interface_->get()->msg[4] = RRC_gen_inteface_.test_id & 0xff;
    general_interface_->get()->msg[1] = RRC_gen_inteface_.rec_lay_id;
    general_interface_->get()->msg[2] = RRC_gen_inteface_.des_layer_id;
    general_interface_->get()->msg[3] = (RRC_gen_inteface_.test_id >> 8) & 0xff;
    general_interface_->get()->msg[4] = RRC_gen_inteface_.test_id & 0xff;
    general_interface_->get()->msg[5] = (data_len >> 8) & 0xff;
    general_interface_->get()->msg[6] = data_len & 0xff;
  }

  bool rrc::ue::wx_read_enb_recfg_data_reconfig(const std::string &filename)
  {
    std::ifstream file(filename);
    std::string line;

    std::unordered_map<std::string, std::function<void(const std::string &)>> assigners = {
        {"multi_beam_num", [&](const std::string &value)
         { multi_cc_num_cc = std::stoi(value); }},
         {"get_rlc_mode", [&](const std::string &value)
         { rlc_mode = std::stoi(value); }}};
    if (file.is_open())
    {
      while (std::getline(file, line))
      {
        // Remove whitespace from the line
        line.erase(remove_if(line.begin(), line.end(), isspace), line.end());

        // Find the position of '=' and ';'
        size_t pos_equal = line.find("=");
        size_t pos_semicolon = line.find(";");

        if (pos_equal != std::string::npos)
        {
          // Extract the key and value
          std::string key = line.substr(0, pos_equal);
          std::string value = line.substr(pos_equal + 1, pos_semicolon - pos_equal - 1);

          // Assign the value if the key exists in the map
          if (assigners.find(key) != assigners.end())
          {
            assigners[key](value);
          }
        }
      }
      parent->cfg.multi_beam_num = multi_cc_num_cc;
      parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg.resize(parent->cfg.multi_beam_num);
      
      if (parent->cfg.ttcn_test_enble == false)
      {
        // 0 AM, 1 UM, 2 TM 
        if ((parent->cfg.rlc_mode != rlc_mode) && (rlc_mode != 0xff))
        {
          parent->cfg.rlc_mode = rlc_mode;
          is_rlc_mode_change = true;
        }
        for(uint32_t i = 0; i < parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list.size();i++)
        {
          if (rlc_mode == 1)
          {
              parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[i].pdcp_cfg.rlc_um_present = true;
              parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[i].pdcp_cfg.rlc_am_present = false;
              parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[i].pdcp_cfg.rlc_tm_present = false;
          }
          else if(rlc_mode == 2)
          {
              parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[i].pdcp_cfg.rlc_um_present = false;
              parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[i].pdcp_cfg.rlc_am_present = false;
              parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[i].pdcp_cfg.rlc_tm_present = true;
          }
          else if(rlc_mode ==0)
          {
              parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[i].pdcp_cfg.rlc_um_present = false;
              parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[i].pdcp_cfg.rlc_am_present = true;
              parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.drb_to_add_mod_list[i].pdcp_cfg.rlc_tm_present = false;
          }
          else{
            parent->logger.error("error:rlc_mode:%d cfg failed",rlc_mode);
          }      
        }
      }
      
      file.close();
      return true;
    }
    else
    {
      std::cerr << "Unable to open wx_read_enb_recfg_data_reconfig906 file" << std::endl;
      return false;
    }
  }

  bool rrc::ue::wx_read_data_reconfig(const std::string &filename)
  {
    std::cout << "[Reconfig][Data][filename]:" << filename << std::endl;
    std::ifstream file(filename);
    std::string line;

    std::vector<ChannelConfig> channel_configs(parent->cfg.multi_beam_num);

    bool rach_indi_tor_present = false;
    std::string rach_indi_tor;

    // 定义其他的配置信息
    bool secu_cfg_present = false;
    bool integ_prot_alg_present = false;

    std::string coph_alg;
    std::string intef_prot_alg;

    std::unordered_map<std::string, std::function<void(const std::string &)>> assigners = {
        {"MCS", [&](const std::string &value)
         { RRC_mcs_Info.MCS = std::stoi(value); }},
        {"rach_indi_tor_present", [&](const std::string &value)
         { rach_indi_tor_present = (value == "true"); }},
        {"rach_indi_tor", [&](const std::string &value)
         { rach_indi_tor = value; }},
        {"secu_cfg_present", [&](const std::string &value)
         { secu_cfg_present = (value == "true"); }},
        {"integ_prot_alg_present", [&](const std::string &value)
         { integ_prot_alg_present = (value == "true"); }},
        {"coph_alg", [&](const std::string &value)
         { coph_alg = value; }},
        {"intef_prot_alg", [&](const std::string &value)
         { intef_prot_alg = value; }}};

    for (uint32_t i = 0; i < parent->cfg.multi_beam_num; ++i)
    {
      if (i == 0)
      {
        std::string prefix = "band_id";
        assigners[prefix] = [&, i](const std::string &value)
        { channel_configs[i].band_id = std::stoi(value); };

        prefix = "freq_id";
        assigners[prefix] = [&, i](const std::string &value)
        { channel_configs[i].freq_id = std::stoi(value); };

        prefix = "chan_type";
        assigners[prefix] = [&, i](const std::string &value)
        { channel_configs[i].chan_type = value; };

        prefix = "direc_t";
        assigners[prefix] = [&, i](const std::string &value)
        { channel_configs[i].direc_t = value; };

        prefix = "slot_ass";
        assigners[prefix] = [&, i](const std::string &value)
        { channel_configs[i].slot_ass = std::stoi(value); };

        prefix = "sche_type";
        assigners[prefix] = [&, i](const std::string &value)
        { channel_configs[i].sche_type = value; };
      }
      else
      {
        std::string prefix = "band_id" + std::to_string(i);
        assigners[prefix] = [&, i](const std::string &value)
        { channel_configs[i].band_id = std::stoi(value); };

        prefix = "freq_id" + std::to_string(i);
        assigners[prefix] = [&, i](const std::string &value)
        { channel_configs[i].freq_id = std::stoi(value); };

        prefix = "chan_type" + std::to_string(i);
        assigners[prefix] = [&, i](const std::string &value)
        { channel_configs[i].chan_type = value; };

        prefix = "direc_t" + std::to_string(i);
        assigners[prefix] = [&, i](const std::string &value)
        { channel_configs[i].direc_t = value; };

        prefix = "slot_ass" + std::to_string(i);
        assigners[prefix] = [&, i](const std::string &value)
        { channel_configs[i].slot_ass = std::stoi(value); };

        prefix = "sche_type" + std::to_string(i);
        assigners[prefix] = [&, i](const std::string &value)
        { channel_configs[i].sche_type = value; };
      }
    }

    if (file.is_open())
    {
      while (std::getline(file, line))
      {
        // Remove whitespace from the line
        line.erase(remove_if(line.begin(), line.end(), isspace), line.end());

        // Find the position of '=' and ';'
        size_t pos_equal = line.find("=");
        size_t pos_semicolon = line.find(";");

        if (pos_equal != std::string::npos)
        {
          // Extract the key and value
          std::string key = line.substr(0, pos_equal);
          std::string value = line.substr(pos_equal + 1, pos_semicolon - pos_equal - 1);

          // Assign the value if the key exists in the map
          if (assigners.find(key) != assigners.end())
          {
            assigners[key](value);
          }
        }
      }
      file.close();

      std::cout << "MCS:" << RRC_mcs_Info.MCS << std::endl;
      // 打印所有的信道配置
      for (uint32_t i = 0; i < parent->cfg.multi_beam_num; ++i)
      {
        std::cout << "信道 " << i << " 配置:" << std::endl;
        std::cout << "band_id = " << channel_configs[i].band_id << std::endl;
        std::cout << "freq_id = " << channel_configs[i].freq_id << std::endl;
        std::cout << "chan_type = " << channel_configs[i].chan_type << std::endl;
        std::cout << "direc_t = " << channel_configs[i].direc_t << std::endl;
        std::cout << "slot_ass = " << channel_configs[i].slot_ass << std::endl;
        std::cout << "sche_type = " << channel_configs[i].sche_type << std::endl;
      }

      std::cout << "rach_indi_tor_present = " << std::boolalpha << rach_indi_tor_present << std::endl;
      std::cout << "rach_indi_tor = " << std::boolalpha << rach_indi_tor << std::endl;

      // 打印其他的配置信息
      std::cout << "secu_cfg_present = " << std::boolalpha << secu_cfg_present << std::endl;
      std::cout << "integ_prot_alg_present = " << std::boolalpha << integ_prot_alg_present << std::endl;
      std::cout << "coph_alg = " << coph_alg << std::endl;
      std::cout << "intef_prot_alg = " << intef_prot_alg << std::endl;

      parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.mobility_contro.rach_indi_tor_present = rach_indi_tor_present;
      if (rach_indi_tor == "True")
      {
        parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.mobility_contro.rach_indi_tor = asn1::rrc::mobility_contro_s::rach_indi_tor_opts::True;
      }
      std::regex quo_tes_regex("\"");
      for (uint32_t i = 0; i < parent->cfg.multi_beam_num; ++i)
      {
        channel_configs[i].chan_type = std::regex_replace(channel_configs[i].chan_type, quo_tes_regex, "");
        channel_configs[i].direc_t = std::regex_replace(channel_configs[i].direc_t, quo_tes_regex, "");
        channel_configs[i].sche_type = std::regex_replace(channel_configs[i].sche_type, quo_tes_regex, "");

        std::cout << "channel_configs[" << i << "].chan_type:" << channel_configs[i].chan_type << std::endl;
        std::cout << "channel_configs[" << i << "].direc_t:" << channel_configs[i].direc_t << std::endl;
        std::cout << "channel_configs[" << i << "].sche_type:" << channel_configs[i].sche_type << std::endl;

        parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].band_id.ba_id.from_number(channel_configs[i].band_id);
        parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].freq_id.freq_id.from_number(channel_configs[i].freq_id);

        std::cout << "parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[" << i << "].band_id.ba_id=" << parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].band_id.ba_id.to_number() << std::endl;
        std::cout << "parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[" << i << "].freq_id.freq_id=" << parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].freq_id.freq_id.to_number() << std::endl;

        if (channel_configs[i].chan_type == static_cast<std::string>("pDCH11"))
        {
          parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pDCH11;
        }
        else if (channel_configs[i].chan_type == static_cast<std::string>("pDCH12"))
        {
          parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pDCH12;
        }
        else if (channel_configs[i].chan_type == static_cast<std::string>("pSCH11"))
        {
          parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH11;
        }
        else if (channel_configs[i].chan_type == static_cast<std::string>("pSCH12"))
        {
          parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH12;
        }
        else if (channel_configs[i].chan_type == static_cast<std::string>("pSCH51"))
        {
          parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH51;
        }
        else if (channel_configs[i].chan_type == static_cast<std::string>("pSCH52"))
        {
          parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH52;
        }
        else
        {
          std::cout << " wx_read_data_reconfig  chan_type failure" << std::endl;
        }

        if (channel_configs[i].sche_type == static_cast<std::string>("Static"))
        {
          parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].sche_type = asn1::rrc::phy_chan_cfg_s::sche_type_opts::Static;
        }
        else if (channel_configs[i].sche_type == static_cast<std::string>("dynamic"))
        {
          parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].sche_type = asn1::rrc::phy_chan_cfg_s::sche_type_opts::dynamic;
        }
        else
        {
          std::cout << " wx_read_data_reconfig  sche_type failure" << std::endl;
        }

        if (channel_configs[i].direc_t == static_cast<std::string>("biDirection"))
        {
          parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].direc_t = asn1::rrc::phy_chan_cfg_s::direct_opts::biDirection;
        }
        else if (channel_configs[i].direc_t == static_cast<std::string>("ulDirection"))
        {
          parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].direc_t = asn1::rrc::phy_chan_cfg_s::direct_opts::ulDirection;
        }
        else if (channel_configs[i].direc_t == static_cast<std::string>("dldirection"))
        {
          parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].direc_t = asn1::rrc::phy_chan_cfg_s::direct_opts::dlDirection;
        }
        else
        {
          std::cout << " wx_read_data_reconfig  direc_t failure" << std::endl;
        }

        std::cout << "parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[" << i << "].chan_type=" << parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].chan_type.to_string() << std::endl;
        std::cout << "parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[" << i << "].sche_type=" << parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].sche_type << std::endl;
        std::cout << "parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[" << i << "].direc_t" << parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].direc_t.to_string() << std::endl;

        parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].s_rnti.srnti.from_number(parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].s_rnti.srnti.to_number());
        std::cout << "parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[" << i << "].s_rnti.srnti=" << parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].s_rnti.srnti.to_number() << std::endl;
        std::cout << "channel_configs[" << i << "].slot_ass" << channel_configs[i].slot_ass << std::endl;
        parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].slot_ass.from_number(channel_configs[i].slot_ass);
        // std::cout << "parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[" << i << "].slot_ass" << parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].slot_ass.to_number();
        // std::cout << "[RECONFIG][DATA][" << i << "][chan_type]:" << parent->cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.to_string();
      }
      // auto start = std::chrono::high_resolution_clock::now();
      // while (true)
      // {
      //   auto end = std::chrono::high_resolution_clock::now();
      //   auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
      //   if (duration.count() > 10)
      //   {
      //     break;
      //   }
      // }
      // // 打印其他的配置信息
      // std::cout << "secu_cfg_present = " << std::boolalpha << secu_cfg_present << std::endl;
      // std::cout << "integ_prot_alg_present = " << std::boolalpha << integ_prot_alg_present << std::endl;
      // std::cout << "coph_alg = " << coph_alg << std::endl;
      // std::cout << "intef_prot_alg = " << intef_prot_alg << std::endl;

      return true;
    }
    else
    {
      std::cerr << "Unable to open file" << std::endl;
      return false;
    }
  }

  bool rrc::ue::wx_read_data_DS_reconfig(const std::string &filename)
  {
    std::ifstream file(filename);
    std::string line;
    // area_mode == 1 扩频模式

    int nor_band_id = 0;
    int nor_freq_id = 0;
    std::string nor_chan_type;
    std::string nor_direc_t;
    int nor_slot_ass = 0;
    std::string nor_sche_type;

    int sp_band_id = 0;
    int sp_freq_id = 0;
    std::string sp_chan_type;
    std::string sp_direc_t;
    int sp_slot_ass = 0;
    int sp_pdtch_phy_code = 0;

    bool secu_cfg_present = false;
    bool integ_prot_alg_present = false;
    std::string coph_alg;
    std::string intef_prot_alg;

    std::unordered_map<std::string, std::function<void(const std::string &)>> assigners = {
        {"MCS", [&](const std::string &value)
         { RRC_mcs_Info.MCS = std::stoi(value); }},
        {"nor_band_id", [&](const std::string &value)
         { nor_band_id = std::stoi(value); }},
        {"nor_freq_id", [&](const std::string &value)
         { nor_freq_id = std::stoi(value); }},
        {"nor_chan_type", [&](const std::string &value)
         { nor_chan_type = value; }},
        {"nor_direc_t", [&](const std::string &value)
         { nor_direc_t = value; }},
        {"nor_slot_ass", [&](const std::string &value)
         { nor_slot_ass = std::stoi(value); }},
        {"nor_sche_type", [&](const std::string &value)
         { nor_sche_type = value; }},
        {"sp_band_id", [&](const std::string &value)
         { sp_band_id = std::stoi(value); }},
        {"sp_freq_id", [&](const std::string &value)
         { sp_freq_id = std::stoi(value); }},
        {"sp_chan_type", [&](const std::string &value)
         { sp_chan_type = value; }},
        {"sp_direc_t", [&](const std::string &value)
         { sp_direc_t = value; }},
        {"sp_slot_ass", [&](const std::string &value)
         { sp_slot_ass = std::stoi(value); }},
        {"sp_pdtch_phy_code", [&](const std::string &value)
         { sp_pdtch_phy_code = std::stoi(value); }},
        {"secu_cfg_present", [&](const std::string &value)
         { secu_cfg_present = (value == "true"); }},
        {"integ_prot_alg_present", [&](const std::string &value)
         { integ_prot_alg_present = (value == "true"); }},
        {"coph_alg", [&](const std::string &value)
         { coph_alg = value; }},
        {"intef_prot_alg", [&](const std::string &value)
         { intef_prot_alg = value; }}};

    if (file.is_open())
    {
      while (std::getline(file, line))
      {
        // Remove whitespace from the line
        line.erase(remove_if(line.begin(), line.end(), isspace), line.end());

        // Find the position of '=' and ';'
        size_t pos_equal = line.find("=");
        size_t pos_semicolon = line.find(";");

        if (pos_equal != std::string::npos)
        {
          // Extract the key and value
          std::string key = line.substr(0, pos_equal);
          std::string value = line.substr(pos_equal + 1, pos_semicolon - pos_equal - 1);

          // Assign the value if the key exists in the map
          if (assigners.find(key) != assigners.end())
          {
            assigners[key](value);
          }
        }
      }
      file.close();

      std::cout << "DS MCS:" << RRC_mcs_Info.MCS << std::endl;

      std::cout << "nor_band_id = " << nor_band_id << std::endl;
      std::cout << "nor_freq_id = " << nor_freq_id << std::endl;
      std::cout << "nor_chan_type = " << nor_chan_type << std::endl;
      std::cout << "nor_direc_t = " << nor_direc_t << std::endl;
      std::cout << "nor_slot_ass = " << nor_slot_ass << std::endl;
      std::cout << "nor_sche_type = " << nor_sche_type << std::endl;

      std::cout << "sp_band_id = " << sp_band_id << std::endl;
      std::cout << "sp_freq_id = " << sp_freq_id << std::endl;
      std::cout << "sp_chan_type = " << sp_chan_type << std::endl;
      std::cout << "sp_direc_t = " << sp_direc_t << std::endl;
      std::cout << "sp_slot_ass = " << sp_slot_ass << std::endl;
      std::cout << "sp_pdtch_phy_code = " << sp_pdtch_phy_code << std::endl;

      std::cout << "secu_cfg_present = " << std::boolalpha << secu_cfg_present << std::endl;
      std::cout << "integ_prot_alg_present = " << std::boolalpha << integ_prot_alg_present << std::endl;
      std::cout << "coph_alg = " << coph_alg << std::endl;
      std::cout << "intef_prot_alg = " << intef_prot_alg << std::endl;

      // UL
      /*************************************** */
      parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].band_id.ba_id.from_number(nor_band_id);
      parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].freq_id.freq_id.from_number(nor_freq_id);
      std::regex quo_tes_regex("\"");
      // UL
      nor_chan_type = std::regex_replace(nor_chan_type, quo_tes_regex, "");
      nor_direc_t = std::regex_replace(nor_direc_t, quo_tes_regex, "");
      nor_sche_type = std::regex_replace(nor_sche_type, quo_tes_regex, "");

      // DL
      sp_chan_type = std::regex_replace(sp_chan_type, quo_tes_regex, "");
      sp_direc_t = std::regex_replace(sp_direc_t, quo_tes_regex, "");

      if (nor_chan_type == static_cast<std::string>("pDCH11"))
      {
        parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pDCH11;
      }
      else if (nor_chan_type == static_cast<std::string>("pDCH12"))
      {
        parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pDCH12;
      }
      else if (nor_chan_type == static_cast<std::string>("pSCH11"))
      {
        parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH11;
      }
      else if (nor_chan_type == static_cast<std::string>("pSCH12"))
      {
        parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH12;
      }
      else if (nor_chan_type == static_cast<std::string>("pSCH51"))
      {
        parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH51;
      }
      else if (nor_chan_type == static_cast<std::string>("pSCH52"))
      {
        parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH52;
      }

      if (nor_sche_type == static_cast<std::string>("Static"))
      {
        parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].sche_type = asn1::rrc::phy_chan_cfg_s::sche_type_opts::Static;
      }
      else if (nor_sche_type == static_cast<std::string>("dynamic"))
      {
        parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].sche_type = asn1::rrc::phy_chan_cfg_s::sche_type_opts::dynamic;
      }

      if (nor_direc_t == static_cast<std::string>("biDirection"))
      {
        parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].direc_t = asn1::rrc::phy_chan_cfg_s::direct_opts::biDirection;
      }
      else if (nor_direc_t == static_cast<std::string>("ulDirection"))
      {
        parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].direc_t = asn1::rrc::phy_chan_cfg_s::direct_opts::ulDirection;
      }
      else if (nor_direc_t == static_cast<std::string>("dldirection"))
      {
        parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].direc_t = asn1::rrc::phy_chan_cfg_s::direct_opts::dlDirection;
      }

      std::cout << "xx nor_slot_ass = " << nor_slot_ass << std::endl;
      parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.from_number(nor_slot_ass);
      /*************************************** */
      std::cout << "xx parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass = " << parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.to_number() << std::endl;
      // DL
      /*************************************** */

      // std::cout << "sp_band_id = " << sp_band_id << std::endl;
      // std::cout << "sp_freq_id = " << sp_freq_id << std::endl;
      // std::cout << "sp_chan_type = " << sp_chan_type << std::endl;
      // std::cout << "sp_direc_t = " << sp_direc_t << std::endl;
      // std::cout << "sp_slot_ass = " << sp_slot_ass << std::endl;
      // std::cout << "sp_pdtch_phy_code = " << sp_pdtch_phy_code << std::endl;

      // std::cout << "secu_cfg_present = " << boolalpha << secu_cfg_present << std::endl;
      // std::cout << "integ_prot_alg_present = " << boolalpha << integ_prot_alg_present << std::endl;
      // std::cout << "coph_alg = " << coph_alg << std::endl;
      // std::cout << "intef_prot_alg = " << intef_prot_alg << std::endl;
      parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].band_id.ba_id.from_number(sp_band_id);
      parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].freq_id.freq_id.from_number(sp_freq_id);

      if (sp_chan_type == static_cast<std::string>("dSPDTCH1"))
      {
        parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::dSPDTCH1;
      }
      else if (sp_chan_type == static_cast<std::string>("dSPDTCH2"))
      {
        parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::dSPDTCH2;
      }
      else if (sp_chan_type == static_cast<std::string>("dSPDTCH3"))
      {
        parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::dSPDTCH3;
      }
      else if (sp_chan_type == static_cast<std::string>("dSPDTCHT"))
      {
        parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::dSPDTCHT;
      }

      if (sp_direc_t == static_cast<std::string>("biDirection"))
      {
        parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].direc_t = asn1::rrc::phy_chan_cfg_s::direct_opts::biDirection;
      }
      else if (sp_direc_t == static_cast<std::string>("ulDirection"))
      {
        parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].direc_t = asn1::rrc::phy_chan_cfg_s::direct_opts::ulDirection;
      }
      else if (sp_direc_t == static_cast<std::string>("dldirection"))
      {
        parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].direc_t = asn1::rrc::phy_chan_cfg_s::direct_opts::dlDirection;
      }
      parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].slot_ass.from_number(sp_slot_ass);
      parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].freq_id.freq_id.from_number(sp_freq_id);

      /****************************************/
      return true;
    }
    else
    {
      std::cerr << "Unable to open file" << std::endl;
      return false;
    }
  }

  bool rrc::ue::wx_read_voice_reconfig(const std::string &filename)
  {
    std::cout << "filename = " << filename << std::endl;
    std::ifstream file(filename);
    std::string line;

    int band_id = 0;
    int freq_id = 0;
    std::string chan_type;
    std::string direc_t;
    int slot_ass = 0;

    std::unordered_map<std::string, std::function<void(const std::string &)>> assigners = {
        {"band_id", [&](const std::string &value)
         { band_id = std::stoi(value); }},
        {"freq_id", [&](const std::string &value)
         { freq_id = std::stoi(value); }},
        {"chan_type", [&](const std::string &value)
         { chan_type = value; }},
        {"direc_t", [&](const std::string &value)
         { direc_t = value; }},
        {"slot_ass", [&](const std::string &value)
         { slot_ass = std::stoi(value); }}};

    if (file.is_open())
    {
      while (std::getline(file, line))
      {
        // Remove whitespace from the line
        line.erase(remove_if(line.begin(), line.end(), isspace), line.end());

        // Find the position of '=' and ';'
        size_t pos_equal = line.find("=");
        size_t pos_semicolon = line.find(";");

        if (pos_equal != std::string::npos)
        {
          // Extract the key and value
          std::string key = line.substr(0, pos_equal);
          std::string value = line.substr(pos_equal + 1, pos_semicolon - pos_equal - 1);

          // Assign the value if the key exists in the map
          if (assigners.find(key) != assigners.end())
          {
            assigners[key](value);
          }
        }
      }
      file.close();

      std::cout << "band_id:" << band_id << std::endl;
      std::cout << "freq_id:" << freq_id << std::endl;
      std::cout << "chan_type:" << chan_type << std::endl;
      std::cout << "direc_t:" << direc_t << std::endl;
      std::cout << "slot_ass:" << slot_ass << std::endl;

      parent->cfg.recfg_wx_voice.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].band_id.ba_id.from_number(band_id);
      parent->cfg.recfg_wx_voice.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].freq_id.freq_id.from_number(freq_id);

      std::regex quo_tes_regex("\"");
      chan_type = std::regex_replace(chan_type, quo_tes_regex, "");
      direc_t = std::regex_replace(direc_t, quo_tes_regex, "");

      if (chan_type == static_cast<std::string>("pDCH11"))
      {
        parent->cfg.recfg_wx_voice.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pDCH11;
      }
      else if (chan_type == static_cast<std::string>("pDCH12"))
      {
        parent->cfg.recfg_wx_voice.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pDCH12;
      }
      else if (chan_type == static_cast<std::string>("pSCH11"))
      {
        parent->cfg.recfg_wx_voice.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH11;
      }
      else if (chan_type == static_cast<std::string>("pSCH12"))
      {
        parent->cfg.recfg_wx_voice.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH12;
      }
      else if (chan_type == static_cast<std::string>("pSCH51"))
      {
        parent->cfg.recfg_wx_voice.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH51;
      }
      else if (chan_type == static_cast<std::string>("pSCH52"))
      {
        parent->cfg.recfg_wx_voice.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH52;
      }

      if (direc_t == static_cast<std::string>("biDirection"))
      {
        parent->cfg.recfg_wx_voice.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].direc_t = asn1::rrc::phy_chan_cfg_s::direct_opts::biDirection;
      }
      else if (direc_t == static_cast<std::string>("ulDirection"))
      {
        parent->cfg.recfg_wx_voice.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].direc_t = asn1::rrc::phy_chan_cfg_s::direct_opts::ulDirection;
      }
      else if (direc_t == static_cast<std::string>("dldirection"))
      {
        parent->cfg.recfg_wx_voice.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].direc_t = asn1::rrc::phy_chan_cfg_s::direct_opts::dlDirection;
      }

      parent->cfg.recfg_wx_voice.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.from_number(slot_ass);
      return true;
    }
    else
    {
      std::cerr << "Unable to open file" << std::endl;
      return false;
    }
  }

  bool rrc::ue::wx_read_voice_DS_reconfig(const std::string &filename)
  {
    std::cout << "filename = " << filename << std::endl;
    std::ifstream file(filename);
    std::string line;

    int nor_band_id = 0;
    int nor_freq_id = 0;
    std::string nor_chan_type;
    std::string nor_direc_t;
    int nor_slot_ass = 0;
    std::string nor_sche_type;

    std::string sp_chan_type;
    std::string sp_direc_t;
    int sp_slot_ass = 0;
    int sp_pdtch_phy_code = 0;

    // // area_mode == 1 扩频模式
    // nor_band_id = 7;
    // nor_freq_id = 1;
    // nor_chan_type = "pDCH11";
    // nor_direc_t = "ulDirection";
    // nor_slot_ass = 4;
    // nor_sche_type = "Static";

    // sp_chan_type = "dSPDTCHT";
    // sp_direc_t = "dldirection";
    // sp_slot_ass = 8;
    // // sp_sche_type = "Static";
    // sp_pdtch_phy_code = 21;
    std::unordered_map<std::string, std::function<void(const std::string &)>> assigners = {
        {"nor_band_id", [&](const std::string &value)
         { nor_band_id = std::stoi(value); }},
        {"nor_freq_id", [&](const std::string &value)
         { nor_freq_id = std::stoi(value); }},
        {"nor_chan_type", [&](const std::string &value)
         { nor_chan_type = value; }},
        {"nor_direc_t", [&](const std::string &value)
         { nor_direc_t = value; }},
        {"nor_slot_ass", [&](const std::string &value)
         { nor_slot_ass = std::stoi(value); }},
        {"nor_sche_type", [&](const std::string &value)
         { nor_sche_type = value; }},

        {"sp_chan_type", [&](const std::string &value)
         { sp_chan_type = value; }},
        {"sp_direc_t", [&](const std::string &value)
         { sp_direc_t = value; }},
        {"sp_slot_ass", [&](const std::string &value)
         { sp_slot_ass = std::stoi(value); }},
        {"sp_pdtch_phy_code", [&](const std::string &value)
         { sp_pdtch_phy_code = std::stoi(value); }}

    };
    if (file.is_open())
    {
      while (std::getline(file, line))
      {
        // Remove whitespace from the line
        line.erase(remove_if(line.begin(), line.end(), isspace), line.end());

        // Find the position of '=' and ';'
        size_t pos_equal = line.find("=");
        size_t pos_semicolon = line.find(";");

        if (pos_equal != std::string::npos)
        {
          // Extract the key and value
          std::string key = line.substr(0, pos_equal);
          std::string value = line.substr(pos_equal + 1, pos_semicolon - pos_equal - 1);

          // Assign the value if the key exists in the map
          if (assigners.find(key) != assigners.end())
          {
            assigners[key](value);
          }
        }
      }
      file.close();

      // // area_mode == 1 扩频模式
      // nor_band_id = 7;
      // nor_freq_id = 1;
      // nor_chan_type = "pDCH11";
      // nor_direc_t = "ulDirection";
      // nor_slot_ass = 4;
      // nor_sche_type = "Static";

      // sp_chan_type = "dSPDTCHT";
      // sp_direc_t = "dldirection";
      // sp_slot_ass = 8;
      // // sp_sche_type = "Static";
      // sp_pdtch_phy_code = 21;

      std::cout << "nor_band_id:" << nor_band_id << std::endl;
      std::cout << "nor_freq_id:" << nor_freq_id << std::endl;
      std::cout << "nor_chan_type:" << nor_chan_type << std::endl;
      std::cout << "nor_direc_t:" << nor_direc_t << std::endl;

      std::cout << "nor_slot_ass:" << nor_slot_ass << std::endl;
      std::cout << "nor_sche_type:" << nor_sche_type << std::endl;

      std::cout << "sp_chan_type:" << sp_chan_type << std::endl;
      std::cout << "sp_direc_t:" << sp_direc_t << std::endl;
      std::cout << "sp_slot_ass:" << sp_slot_ass << std::endl;
      std::cout << "sp_pdtch_phy_code:" << sp_pdtch_phy_code << std::endl;

      std::regex quo_tes_regex("\"");

      // UL
      //******************************************************************************** */

      nor_chan_type = std::regex_replace(nor_chan_type, quo_tes_regex, "");
      nor_direc_t = std::regex_replace(nor_direc_t, quo_tes_regex, "");
      nor_sche_type = std::regex_replace(nor_sche_type, quo_tes_regex, "");
      parent->cfg.recfg_wx_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].band_id.ba_id.from_number(nor_band_id);
      parent->cfg.recfg_wx_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].freq_id.freq_id.from_number(nor_freq_id);

      if (nor_chan_type == static_cast<std::string>("pDCH11"))
      {
        parent->cfg.recfg_wx_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pDCH11;
      }
      else if (nor_chan_type == static_cast<std::string>("pDCH12"))
      {
        parent->cfg.recfg_wx_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pDCH12;
      }
      else if (nor_chan_type == static_cast<std::string>("pSCH11"))
      {
        parent->cfg.recfg_wx_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH11;
      }
      else if (nor_chan_type == static_cast<std::string>("pSCH12"))
      {
        parent->cfg.recfg_wx_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH12;
      }
      else if (nor_chan_type == static_cast<std::string>("pSCH51"))
      {
        parent->cfg.recfg_wx_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH51;
      }
      else if (nor_chan_type == static_cast<std::string>("pSCH52"))
      {
        parent->cfg.recfg_wx_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH52;
      }

      if (nor_sche_type == static_cast<std::string>("Static"))
      {
        parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].sche_type = asn1::rrc::phy_chan_cfg_s::sche_type_opts::Static;
      }
      else if (nor_sche_type == static_cast<std::string>("dynamic"))
      {
        parent->cfg.recfg_norm_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].sche_type = asn1::rrc::phy_chan_cfg_s::sche_type_opts::dynamic;
      }

      if (nor_direc_t == static_cast<std::string>("biDirection"))
      {
        parent->cfg.recfg_wx_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].direc_t = asn1::rrc::phy_chan_cfg_s::direct_opts::biDirection;
      }
      else if (nor_direc_t == static_cast<std::string>("ulDirection"))
      {
        parent->cfg.recfg_wx_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].direc_t = asn1::rrc::phy_chan_cfg_s::direct_opts::ulDirection;
      }
      else if (nor_direc_t == static_cast<std::string>("dldirection"))
      {
        parent->cfg.recfg_wx_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].direc_t = asn1::rrc::phy_chan_cfg_s::direct_opts::dlDirection;
      }

      parent->cfg.recfg_wx_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.from_number(nor_slot_ass);
      //******************************************************************************** */

      // DL
      //****************************************************************************************** */
      sp_chan_type = std::regex_replace(sp_chan_type, quo_tes_regex, "");
      sp_direc_t = std::regex_replace(sp_direc_t, quo_tes_regex, "");

      if (sp_chan_type == static_cast<std::string>("dSPDTCH1"))
      {
        parent->cfg.recfg_wx_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::dSPDTCH1;
      }
      else if (sp_chan_type == static_cast<std::string>("dSPDTCH2"))
      {
        parent->cfg.recfg_wx_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::dSPDTCH2;
      }
      else if (sp_chan_type == static_cast<std::string>("dSPDTCH3"))
      {
        parent->cfg.recfg_wx_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::dSPDTCH3;
      }
      else if (sp_chan_type == static_cast<std::string>("dSPDTCHT"))
      {
        parent->cfg.recfg_wx_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::dSPDTCHT;
      }

      if (sp_direc_t == static_cast<std::string>("biDirection"))
      {
        parent->cfg.recfg_wx_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].direc_t = asn1::rrc::phy_chan_cfg_s::direct_opts::biDirection;
      }
      else if (sp_direc_t == static_cast<std::string>("ulDirection"))
      {
        parent->cfg.recfg_wx_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].direc_t = asn1::rrc::phy_chan_cfg_s::direct_opts::ulDirection;
      }
      else if (sp_direc_t == static_cast<std::string>("dldirection"))
      {
        parent->cfg.recfg_wx_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].direc_t = asn1::rrc::phy_chan_cfg_s::direct_opts::dlDirection;
      }
      parent->cfg.recfg_wx_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].slot_ass.from_number(sp_slot_ass);
      parent->cfg.recfg_wx_kuopin.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[1].freq_id.freq_id.from_number(sp_pdtch_phy_code);
      //********************************************************************************************************* */
      return true;
    }
    else
    {
      std::cerr << "Unable to open file" << std::endl;
      return false;
    }
  }

  void rrc::ue::wx_target_Beam_notify_Source_Beam_To_Release()
  {
    std::cout << "wx_Switch_Dataing_or_NBusness()" << std::endl;
    Switch_Info Temp_Switch_Info;

    Temp_Switch_Info.Beam_Header = 0xEE;
    Temp_Switch_Info.Beam_Type = Target_Beam;
    Temp_Switch_Info.Beam_InfoType = Target_receive_reconfig_complete;
    Temp_Switch_Info.Switch_Type = Data_Service_Switch;
    Temp_Switch_Info.Voice_Type = parent->Switch_Voice;

    srsran::unique_byte_buffer_t pdu = srsran::make_byte_buffer();
    pdu->N_bytes = 4;
    pdu->msg[0] = Temp_Switch_Info.Beam_Header;
    pdu->msg[1] = Temp_Switch_Info.Beam_Type;
    pdu->msg[2] = Temp_Switch_Info.Beam_InfoType;
    pdu->msg[3] = Temp_Switch_Info.Switch_Type;
    pdu->msg[4] = Temp_Switch_Info.Voice_Type;

    parent->rrc_adp->udp_.target_beam_trans_queue.try_push(std::move(pdu));
    std::cout << "wx_target_Beam_notify_Source_Beam_To_Release" << std::endl;
  }

  bool rrc::ue::wx_SourceBeam_read_reconfig(const std::string &filename)
  {
    std::cout << "filename = " << filename << std::endl;
    std::ifstream file(filename);
    std::string line;

    std::unordered_map<std::string, std::function<void(const std::string &)>> assigners = {
        {"MCS", [&](const std::string &value)
         { RRC_mcs_Info.MCS = std::stoi(value); }}};

    if (file.is_open())
    {
      while (std::getline(file, line))
      {
        // Remove whitespace from the line
        line.erase(remove_if(line.begin(), line.end(), isspace), line.end());

        // Find the position of '=' and ';'
        size_t pos_equal = line.find("=");
        size_t pos_semicolon = line.find(";");

        if (pos_equal != std::string::npos)
        {
          // Extract the key and value
          std::string key = line.substr(0, pos_equal);
          std::string value = line.substr(pos_equal + 1, pos_semicolon - pos_equal - 1);

          // Assign the value if the key exists in the map
          if (assigners.find(key) != assigners.end())
          {
            assigners[key](value);
          }
        }
      }
      file.close();
      std::cout << "wx_SourceBeam_read_reconfig MCS=" << RRC_mcs_Info.MCS << std::endl;

      return true;
    }
    else
    {
      std::cerr << "Unable to open file" << std::endl;
      return false;
    }
  }

  bool rrc::ue::wx_SourceBeam_to_read_TargetBeam_reconfig_normal(const std::string &filename, rrc_con_recfg_r1_ies_s &rrc_con_)
  {
    std::cout << "filename = " << filename << std::endl;
    std::ifstream file(filename);
    std::string line;

    std::string chan_type;
    int band_id = 0;
    int freq_id = 0;
    int slot_ass = 0;
    std::string sche_type;

    //  band_id = 50;
    // freq_id = 1;
    // chan_type = "pSCH11";

    // slot_ass = 8;
    // sche_type = "Static";

    std::unordered_map<std::string, std::function<void(const std::string &)>> assigners = {
        {"chan_type", [&](const std::string &value)
         { chan_type = value; }},
        {"band_id", [&](const std::string &value)
         { band_id = std::stoi(value); }},
        {"freq_id", [&](const std::string &value)
         { freq_id = std::stoi(value); }},
        {"slot_ass", [&](const std::string &value)
         { slot_ass = std::stoi(value); }},
        {"sche_type", [&](const std::string &value)
         { sche_type = value; }}};

    if (file.is_open())
    {
      while (std::getline(file, line))
      {
        // Remove whitespace from the line
        line.erase(remove_if(line.begin(), line.end(), isspace), line.end());

        // Find the position of '=' and ';'
        size_t pos_equal = line.find("=");
        size_t pos_semicolon = line.find(";");

        if (pos_equal != std::string::npos)
        {
          // Extract the key and value
          std::string key = line.substr(0, pos_equal);
          std::string value = line.substr(pos_equal + 1, pos_semicolon - pos_equal - 1);

          // Assign the value if the key exists in the map
          if (assigners.find(key) != assigners.end())
          {
            assigners[key](value);
          }
        }
      }
      file.close();

      std::cout << "chan_type:" << chan_type << std::endl;
      std::cout << "sche_type:" << sche_type << std::endl;
      std::cout << "band_id:" << band_id << std::endl;
      std::cout << "freq_id:" << freq_id << std::endl;
      std::cout << "slot_ass:" << slot_ass << std::endl;

      std::regex quo_tes_regex("\"");
      chan_type = std::regex_replace(chan_type, quo_tes_regex, "");
      sche_type = std::regex_replace(sche_type, quo_tes_regex, "");

      if (chan_type == static_cast<std::string>("pDCH11"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pDCH11;
      }
      else if (chan_type == static_cast<std::string>("pDCH12"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pDCH12;
      }
      else if (chan_type == static_cast<std::string>("pSCH11"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH11;
      }
      else if (chan_type == static_cast<std::string>("pSCH12"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH12;
      }
      else if (chan_type == static_cast<std::string>("pSCH51"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH51;
      }
      else if (chan_type == static_cast<std::string>("pSCH52"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH52;
      }

      if (sche_type == static_cast<std::string>("Static"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].sche_type = asn1::rrc::phy_chan_cfg_s::sche_type_opts::Static;
      }
      else if (sche_type == static_cast<std::string>("dynamic"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].sche_type = asn1::rrc::phy_chan_cfg_s::sche_type_opts::dynamic;
      }
      rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].band_id.ba_id.from_number(band_id);
      rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].freq_id.freq_id.from_number(freq_id);
      rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.from_number(slot_ass);

      return true;
    }
    else
    {
      std::cout << "read failure" << std::endl;
      return false;
    }
  }

  bool rrc::ue::wx_SourceBeam_to_read_TargetBeam_reconfig_sp(const std::string &filename, rrc_con_recfg_r1_ies_s &rrc_con_)
  {
    std::cout << "filename = " << filename << std::endl;
    std::ifstream file(filename);
    std::string line;

    std::string nor_chan_type;
    int nor_band_id = 0;
    int nor_freq_id = 0;
    int nor_slot_ass = 0;
    std::string nor_sche_type;
    std::string nor_direc_t;

    std::string sp_direc_t;
    std::string sp_chan_type;
    int sp_band_id = 0;
    int sp_freq_id = 0;
    int sp_slot_ass = 0;

    int sp_pdtch_phy_code;

    //  band_id = 50;
    // freq_id = 1;
    // chan_type = "pSCH11";

    // slot_ass = 8;
    // sche_type = "Static";

    std::unordered_map<std::string, std::function<void(const std::string &)>> assigners = {
        {"nor_chan_type", [&](const std::string &value)
         { nor_chan_type = value; }},
        {"nor_band_id", [&](const std::string &value)
         { nor_band_id = std::stoi(value); }},
        {"nor_freq_id", [&](const std::string &value)
         { nor_freq_id = std::stoi(value); }},
        {"nor_slot_ass", [&](const std::string &value)
         { nor_slot_ass = std::stoi(value); }},
        {"nor_sche_type", [&](const std::string &value)
         { nor_sche_type = value; }},
        {"nor_direc_t", [&](const std::string &value)
         { nor_direc_t = value; }},

        {"sp_direc_t", [&](const std::string &value)
         { sp_direc_t = value; }},
        {"sp_chan_type", [&](const std::string &value)
         { sp_chan_type = value; }},
        {"sp_band_id", [&](const std::string &value)
         { sp_band_id = std::stoi(value); }},
        {"sp_freq_id", [&](const std::string &value)
         { sp_freq_id = std::stoi(value); }},
        {"sp_slot_ass", [&](const std::string &value)
         { sp_slot_ass = std::stoi(value); }},

        {"sp_pdtch_phy_code", [&](const std::string &value)
         { sp_pdtch_phy_code = std::stoi(value); }}};

    if (file.is_open())
    {
      while (std::getline(file, line))
      {
        // Remove whitespace from the line
        line.erase(remove_if(line.begin(), line.end(), isspace), line.end());

        // Find the position of '=' and ';'
        size_t pos_equal = line.find("=");
        size_t pos_semicolon = line.find(";");

        if (pos_equal != std::string::npos)
        {
          // Extract the key and value
          std::string key = line.substr(0, pos_equal);
          std::string value = line.substr(pos_equal + 1, pos_semicolon - pos_equal - 1);

          // Assign the value if the key exists in the map
          if (assigners.find(key) != assigners.end())
          {
            assigners[key](value);
          }
        }
      }
      file.close();

      std::cout << "nor_chan_type:" << nor_chan_type << std::endl;
      std::cout << "nor_sche_type:" << nor_sche_type << std::endl;
      std::cout << "nor_band_id:" << nor_band_id << std::endl;
      std::cout << "nor_freq_id:" << nor_freq_id << std::endl;
      std::cout << "nor_slot_ass:" << nor_slot_ass << std::endl;
      std::cout << "nor_direc_t:" << nor_direc_t << std::endl;

      std::cout << "sp_direc_t:" << sp_direc_t << std::endl;
      std::cout << "sp_chan_type:" << sp_chan_type << std::endl;

      std::cout << "sp_band_id:" << sp_band_id << std::endl;
      std::cout << "sp_freq_id:" << sp_freq_id << std::endl;
      std::cout << "sp_slot_ass:" << sp_slot_ass << std::endl;
      std::cout << "sp_pdtch_phy_code:" << sp_pdtch_phy_code << std::endl;

      std::regex quo_tes_regex("\"");
      nor_chan_type = std::regex_replace(nor_chan_type, quo_tes_regex, "");
      nor_sche_type = std::regex_replace(nor_sche_type, quo_tes_regex, "");
      sp_chan_type = std::regex_replace(sp_chan_type, quo_tes_regex, "");
      nor_direc_t = std::regex_replace(nor_direc_t, quo_tes_regex, "");
      sp_direc_t = std::regex_replace(sp_direc_t, quo_tes_regex, "");

      if (nor_chan_type == static_cast<std::string>("pDCH11"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pDCH11;
      }
      else if (nor_chan_type == static_cast<std::string>("pDCH12"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pDCH12;
      }
      else if (nor_chan_type == static_cast<std::string>("pSCH11"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH11;
      }
      else if (nor_chan_type == static_cast<std::string>("pSCH12"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH12;
      }
      else if (nor_chan_type == static_cast<std::string>("pSCH51"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH51;
      }
      else if (nor_chan_type == static_cast<std::string>("pSCH52"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH52;
      }

      if (nor_sche_type == static_cast<std::string>("Static"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].sche_type = asn1::rrc::phy_chan_cfg_s::sche_type_opts::Static;
      }
      else if (nor_sche_type == static_cast<std::string>("dynamic"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].sche_type = asn1::rrc::phy_chan_cfg_s::sche_type_opts::dynamic;
      }

      if (nor_direc_t == static_cast<std::string>("biDirection"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].direc_t = asn1::rrc::phy_chan_cfg_s::direct_opts::biDirection;
      }
      else if (nor_direc_t == static_cast<std::string>("ulDirection"))
      {
        std::cout << "aaaaaaaaaaaaaaaaaa" << std::endl;
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].direc_t = asn1::rrc::phy_chan_cfg_s::direct_opts::ulDirection;
      }
      else if (nor_direc_t == static_cast<std::string>("dldirection"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].direc_t = asn1::rrc::phy_chan_cfg_s::direct_opts::dlDirection;
      }
      std::cout << "rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type=" << rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type.to_string() << std::endl;
      std::cout << "rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].sche_type=" << rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].sche_type.to_string() << std::endl;
      std::cout << "rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].direc_t=" << rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].direc_t.to_string() << std::endl;

      rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].band_id.ba_id.from_number(nor_band_id);
      rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].freq_id.freq_id.from_number(nor_freq_id);
      rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.from_number(nor_slot_ass);

      if (sp_chan_type == static_cast<std::string>("dSPDTCH1"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::dSPDTCH1;
      }
      else if (sp_chan_type == static_cast<std::string>("dSPDTCH2"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::dSPDTCH2;
      }
      else if (sp_chan_type == static_cast<std::string>("dSPDTCH3"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::dSPDTCH3;
      }
      else if (sp_chan_type == static_cast<std::string>("dSPDTCHT"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::dSPDTCHT;
      }

      if (sp_direc_t == static_cast<std::string>("biDirection"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[1].direc_t = asn1::rrc::phy_chan_cfg_s::direct_opts::biDirection;
      }
      else if (sp_direc_t == static_cast<std::string>("ulDirection"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[1].direc_t = asn1::rrc::phy_chan_cfg_s::direct_opts::ulDirection;
      }
      else if (sp_direc_t == static_cast<std::string>("dldirection"))
      {
        std::cout << "bbbbbbbbbbbbbbbbbbb" << std::endl;
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[1].direc_t = asn1::rrc::phy_chan_cfg_s::direct_opts::dlDirection;
      }
      std::cout << "rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type=" << rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type.to_string() << std::endl;
      std::cout << "rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[1].direc_t=" << rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[1].direc_t.to_string() << std::endl;

      rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[1].band_id.ba_id.from_number(sp_band_id);
      rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[1].freq_id.freq_id.from_number(sp_freq_id);
      rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[1].slot_ass.from_number(sp_slot_ass);
      rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[1].pdtch_code.pdt_phy_code = 21;
      return true;
    }
    else
    {
      std::cout << "read failure" << std::endl;
      return false;
    }
  }

  bool rrc::ue::wx_SourceBeam_to_read_TargetBeam_reconfig_Voice(const std::string &filename, rrc_con_recfg_r1_ies_s &rrc_con_)
  {
    std::cout << "filename = " << filename << std::endl;
    std::ifstream file(filename);
    std::string line;

    std::string chan_type;
    int band_id = 0;
    int freq_id = 0;
    int slot_ass = 0;

    //  band_id = 50;
    // freq_id = 1;
    // chan_type = "pSCH11";

    // slot_ass = 8;
    // sche_type = "Static";

    std::unordered_map<std::string, std::function<void(const std::string &)>> assigners = {
        {"chan_type", [&](const std::string &value)
         { chan_type = value; }},
        {"band_id", [&](const std::string &value)
         { band_id = std::stoi(value); }},
        {"freq_id", [&](const std::string &value)
         { freq_id = std::stoi(value); }},
        {"slot_ass", [&](const std::string &value)
         { slot_ass = std::stoi(value); }}};

    if (file.is_open())
    {
      while (std::getline(file, line))
      {
        // Remove whitespace from the line
        line.erase(remove_if(line.begin(), line.end(), isspace), line.end());

        // Find the position of '=' and ';'
        size_t pos_equal = line.find("=");
        size_t pos_semicolon = line.find(";");

        if (pos_equal != std::string::npos)
        {
          // Extract the key and value
          std::string key = line.substr(0, pos_equal);
          std::string value = line.substr(pos_equal + 1, pos_semicolon - pos_equal - 1);

          // Assign the value if the key exists in the map
          if (assigners.find(key) != assigners.end())
          {
            assigners[key](value);
          }
        }
      }
      file.close();

      std::cout << "chan_type:" << chan_type << std::endl;

      std::cout << "band_id:" << band_id << std::endl;
      std::cout << "freq_id:" << freq_id << std::endl;
      std::cout << "slot_ass:" << slot_ass << std::endl;

      std::regex quo_tes_regex("\"");
      chan_type = std::regex_replace(chan_type, quo_tes_regex, "");

      if (chan_type == static_cast<std::string>("pDCH11"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pDCH11;
      }
      else if (chan_type == static_cast<std::string>("pDCH12"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pDCH12;
      }
      else if (chan_type == static_cast<std::string>("pSCH11"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH11;
      }
      else if (chan_type == static_cast<std::string>("pSCH12"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH12;
      }
      else if (chan_type == static_cast<std::string>("pSCH51"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH51;
      }
      else if (chan_type == static_cast<std::string>("pSCH52"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH52;
      }

      rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].band_id.ba_id.from_number(band_id);
      rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].freq_id.freq_id.from_number(freq_id);
      rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.from_number(slot_ass);

      return true;
    }
    else
    {
      std::cout << "read failure" << std::endl;
      return false;
    }
  }

  bool rrc::ue::wx_SourceBeam_to_read_TargetBeam_reconfig_Voice_Ds(const std::string &filename, rrc_con_recfg_r1_ies_s &rrc_con_)
  {
    std::cout << "filename = " << filename << std::endl;
    std::ifstream file(filename);
    std::string line;

    std::string nor_chan_type;
    int nor_band_id = 0;
    int nor_freq_id = 0;
    int nor_slot_ass = 0;

    std::string nor_direc_t;

    std::string sp_direc_t;
    std::string sp_chan_type;
    int sp_band_id = 0;
    int sp_freq_id = 0;
    int sp_slot_ass = 0;
    int sp_pdtch_phy_code = 0;

    //  band_id = 50;
    // freq_id = 1;
    // chan_type = "pSCH11";

    // slot_ass = 8;
    // sche_type = "Static";

    std::unordered_map<std::string, std::function<void(const std::string &)>> assigners = {
        {"nor_chan_type", [&](const std::string &value)
         { nor_chan_type = value; }},
        {"nor_band_id", [&](const std::string &value)
         { nor_band_id = std::stoi(value); }},
        {"nor_freq_id", [&](const std::string &value)
         { nor_freq_id = std::stoi(value); }},
        {"nor_slot_ass", [&](const std::string &value)
         { nor_slot_ass = std::stoi(value); }},
        {"nor_direc_t", [&](const std::string &value)
         { nor_direc_t = value; }},

        {"sp_direc_t", [&](const std::string &value)
         { sp_direc_t = value; }},
        {"sp_chan_type", [&](const std::string &value)
         { sp_chan_type = value; }},
        {"sp_band_id", [&](const std::string &value)
         { sp_band_id = std::stoi(value); }},
        {"sp_freq_id", [&](const std::string &value)
         { sp_freq_id = std::stoi(value); }},
        {"sp_slot_ass", [&](const std::string &value)
         { sp_slot_ass = std::stoi(value); }},
        {"sp_pdtch_phy_code", [&](const std::string &value)
         { sp_pdtch_phy_code = std::stoi(value); }}};

    if (file.is_open())
    {
      while (std::getline(file, line))
      {
        // Remove whitespace from the line
        line.erase(remove_if(line.begin(), line.end(), isspace), line.end());

        // Find the position of '=' and ';'
        size_t pos_equal = line.find("=");
        size_t pos_semicolon = line.find(";");

        if (pos_equal != std::string::npos)
        {
          // Extract the key and value
          std::string key = line.substr(0, pos_equal);
          std::string value = line.substr(pos_equal + 1, pos_semicolon - pos_equal - 1);

          // Assign the value if the key exists in the map
          if (assigners.find(key) != assigners.end())
          {
            assigners[key](value);
          }
        }
      }
      file.close();

      std::cout << "nor_chan_type:" << nor_chan_type << std::endl;
      std::cout << "nor_band_id:" << nor_band_id << std::endl;
      std::cout << "nor_freq_id:" << nor_freq_id << std::endl;
      std::cout << "nor_slot_ass:" << nor_slot_ass << std::endl;

      std::cout << "sp_chan_type:" << sp_chan_type << std::endl;
      std::cout << "sp_band_id:" << sp_band_id << std::endl;
      std::cout << "sp_freq_id:" << sp_freq_id << std::endl;
      std::cout << "sp_slot_ass:" << sp_slot_ass << std::endl;
      std::cout << "sp_direc_t:" << sp_direc_t << std::endl;
      std::cout << "sp_pdtch_phy_code:" << sp_pdtch_phy_code << std::endl;

      std::regex quo_tes_regex("\"");
      nor_chan_type = std::regex_replace(nor_chan_type, quo_tes_regex, "");
      sp_chan_type = std::regex_replace(sp_chan_type, quo_tes_regex, "");

      nor_direc_t = std::regex_replace(nor_direc_t, quo_tes_regex, "");
      sp_direc_t = std::regex_replace(sp_direc_t, quo_tes_regex, "");

      if (nor_chan_type == static_cast<std::string>("pDCH11"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pDCH11;
      }
      else if (nor_chan_type == static_cast<std::string>("pDCH12"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pDCH12;
      }
      else if (nor_chan_type == static_cast<std::string>("pSCH11"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH11;
      }
      else if (nor_chan_type == static_cast<std::string>("pSCH12"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH12;
      }
      else if (nor_chan_type == static_cast<std::string>("pSCH51"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH51;
      }
      else if (nor_chan_type == static_cast<std::string>("pSCH52"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::pSCH52;
      }

      if (nor_direc_t == static_cast<std::string>("biDirection"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].direc_t = asn1::rrc::phy_chan_cfg_s::direct_opts::biDirection;
      }
      else if (nor_direc_t == static_cast<std::string>("ulDirection"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].direc_t = asn1::rrc::phy_chan_cfg_s::direct_opts::ulDirection;
      }
      else if (nor_direc_t == static_cast<std::string>("dlDirection"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].direc_t = asn1::rrc::phy_chan_cfg_s::direct_opts::dlDirection;
      }

      rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].band_id.ba_id.from_number(nor_band_id);
      rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].freq_id.freq_id.from_number(nor_freq_id);
      rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[0].slot_ass.from_number(nor_slot_ass);

      if (sp_chan_type == static_cast<std::string>("dSPDTCH1"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::dSPDTCH1;
      }
      else if (sp_chan_type == static_cast<std::string>("dSPDTCH2"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::dSPDTCH2;
      }
      else if (sp_chan_type == static_cast<std::string>("dSPDTCH3"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::dSPDTCH3;
      }
      else if (sp_chan_type == static_cast<std::string>("dSPDTCHT"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[1].chan_type = asn1::rrc::phy_chan_cfg_s::chan_type_opts::dSPDTCHT;
      }
      if (sp_direc_t == static_cast<std::string>("biDirection"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[1].direc_t = asn1::rrc::phy_chan_cfg_s::direct_opts::biDirection;
      }
      else if (sp_direc_t == static_cast<std::string>("ulDirection"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[1].direc_t = asn1::rrc::phy_chan_cfg_s::direct_opts::ulDirection;
      }
      else if (sp_direc_t == static_cast<std::string>("dlDirection"))
      {
        rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[1].direc_t = asn1::rrc::phy_chan_cfg_s::direct_opts::dlDirection;
      }

      rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[1].band_id.ba_id.from_number(sp_band_id);
      rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[1].freq_id.freq_id.from_number(sp_freq_id);
      rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[1].slot_ass.from_number(sp_slot_ass);
      rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[1].pdtch_code.pdt_phy_code = sp_pdtch_phy_code;
      std::cout << "rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[1].pdtch_code.pdt_phy_code:" << rrc_con_.redio_resour_cfg_dedi.phy_chan_list_cfg[1].pdtch_code.pdt_phy_code << std::endl;
      return true;
    }
    else
    {
      std::cout << "read failure" << std::endl;
      return false;
    }
  }

  bool rrc::ue::initial_ue_to_nas(uint16_t rnti, srsran::unique_byte_buffer_t pdu)
  {
    srsran::unique_byte_buffer_t enb_pdu = srsran::make_byte_buffer();
    srsepc::enb_msg_header_t s1_header;
    s1_header.enb_id = parent->rrc_adp->udp_.pid;
    s1_header.rnti = 70;
    s1_header.msg_type = srsepc::enb_msg_type::init_ue;

    int len = sizeof(s1_header);

    memcpy(enb_pdu->msg, &s1_header, len);
    memcpy(enb_pdu->msg + len, pdu->msg, pdu->N_bytes);

    enb_pdu->N_bytes = pdu->N_bytes + len;

    parent->rrc_adp->udp_.send_msg_to_cnw(std::move(enb_pdu));

    return true;
  }

  bool rrc::ue::write_pdu_to_nas(uint16_t rnti, srsran::unique_byte_buffer_t pdu)
  {
    srsran::unique_byte_buffer_t enb_pdu = srsran::make_byte_buffer();
    srsepc::enb_msg_header_t s1_header;
    s1_header.enb_id = parent->rrc_adp->udp_.pid;
    s1_header.rnti = 70;
    s1_header.msg_type = srsepc::enb_msg_type::nas_ul;

    int len = sizeof(s1_header);

    memcpy(enb_pdu->msg, &s1_header, len);
    memcpy(enb_pdu->msg + len, pdu->msg, pdu->N_bytes);

    enb_pdu->N_bytes = pdu->N_bytes + len;

    parent->rrc_adp->udp_.send_msg_to_cnw(std::move(enb_pdu));

    return true;
  }

  bool rrc::ue::write_pdu_to_ims(uint16_t rnti, srsran::unique_byte_buffer_t pdu)
  {
    srsran::unique_byte_buffer_t enb_pdu = srsran::make_byte_buffer();
    srsepc::enb_msg_header_t s1_header;
    s1_header.enb_id = parent->rrc_adp->udp_.pid;
    s1_header.rnti = 70;
    s1_header.msg_type = srsepc::enb_msg_type::ims_ul;

    int len = sizeof(s1_header);

    memcpy(enb_pdu->msg, &s1_header, len);
    memcpy(enb_pdu->msg + len, pdu->msg, pdu->N_bytes);

    enb_pdu->N_bytes = pdu->N_bytes + len;

    parent->rrc_adp->udp_.send_msg_to_cnw(std::move(enb_pdu));

    return true;
  }

  bool rrc::ue::notify_send_reg_or_service_accept(uint16_t rnti)
  {
    srsran::unique_byte_buffer_t enb_pdu = srsran::make_byte_buffer();
    srsepc::enb_msg_header_t s1_header;
    s1_header.enb_id = parent->rrc_adp->udp_.pid;
    s1_header.rnti = 70;
    s1_header.msg_type = srsepc::enb_msg_type::send_reg_or_service_accept;

    int len = sizeof(s1_header);

    memcpy(enb_pdu->msg, &s1_header, len);
    enb_pdu->N_bytes = len;

    parent->rrc_adp->udp_.send_msg_to_cnw(std::move(enb_pdu));

    return true;
  }

  bool rrc::ue::notify_send_user_release(uint16_t rnti)
  {
    srsran::unique_byte_buffer_t enb_pdu = srsran::make_byte_buffer();
    srsepc::enb_msg_header_t s1_header;
    s1_header.enb_id = parent->rrc_adp->udp_.pid;
    s1_header.rnti = 70;
    s1_header.msg_type = srsepc::enb_msg_type::notify_send_user_release;

    int len = sizeof(s1_header);

    memcpy(enb_pdu->msg, &s1_header, len);
    enb_pdu->N_bytes = len;

    parent->rrc_adp->udp_.send_msg_to_cnw(std::move(enb_pdu));

    return true;
  }
  void rrc::ue::send_rrc_measure_reconf()
  {
    std::cout << "for_measurereport_send_RRC_Reconfig" << std::endl;
    update_scells();
    s_dl_dcch_msg_s ss_dl_dcch_msg;
    rrc_con_recfg_s &recofg_si = ss_dl_dcch_msg.msg.set_rrc_con_recfg();
    recofg_si.rrc_tran_iden = 0;

    rrc_con_recfg_r1_ies_s &recfg_r1 = recofg_si.rrc_con_recfg_r1;
    recofg_si.rrc_con_recfg_r1.meas_cfg_present = true;
    meas_cofg_s &mea_cfg = recofg_si.rrc_con_recfg_r1.meas_cfg;
    mea_cfg.meas_gap_cfg_list_present = true;
    mea_cfg.s_mea_sure_present = true;
    if(wx_area_mode==0){
      meas_gap_cfg_list_normal_s &mea_gap_normal = mea_cfg.meas_gap_cfg_list.set_meas_gap_cfg_list_normal();
      mea_gap_normal.resize(1);
      mea_gap_normal[0].beam_index = 1;
      mea_gap_normal[0].frame_offset = 3;
      mea_gap_normal[0].fcch_ba_id.ba_id.from_string("001000");
      mea_gap_normal[0].fcch_freq_id.freq_id.from_string("01");
      mea_gap_normal[0].mib_Re_Fra_Num = 0;
      mea_gap_normal[0].meas_norm_ratio = meas_gap_cfg_normal_s::meas_norm_ratio_opts::dB5;
      mea_cfg.s_mea_sure.set_rssi_normal().rssi_normal = 63;
    }else{
        meas_gap_cfg_list_dl_freq_spread_s &meas_gap_cfg_list_dl_freq = mea_cfg.meas_gap_cfg_list.set_meas_gap_cfg_list_dl_freq_spread_s();
        meas_gap_cfg_list_dl_freq.resize(1);
        meas_gap_cfg_list_dl_freq[0].beam_id = 1;
        meas_gap_cfg_list_dl_freq[0].sec_syn_group_id = 3;
        mea_cfg.s_mea_sure.set_rssi_dl_freq_spread().rssi_dl_freq_spread=63;
    }

    report_cfg_s &rep_cfg = recofg_si.rrc_con_recfg_r1.meas_cfg.report_cfg;
    // rep_cfg.report_geo_grap_info_present = true;
    //  rep_cfg.report_geo_grap_info=report_cfg_s::report_geo_grap_info_opts::True;
    rep_cfg.thr_hold_present = true;
    rep_cfg.thr_hold.off_set = -9;
    rep_cfg.thr_hold.hysteresis = 0;
    rep_cfg.thr_hold.time_to_trigger = thr_hold_s::time_to_trigger_opts::ms900;
    rep_cfg.thr_hold.filter_coe_ent = thr_hold_s::filter_coe_ent_opts::fc0;


    srsran::unique_byte_buffer_t pdu = srsran::make_byte_buffer();
    pdu->init();
    asn1::bit_ref bref(pdu->msg, pdu->get_tailroom());
    if (ss_dl_dcch_msg.pack(bref) == asn1::SRSASN_ERROR_ENCODE_FAIL)
    {
      parent->logger.error("Failed to encode DL-DCCH-Msg for rnti=0x%x", rnti);
      std::cout << "Failed to encode DL-DCCH-Msg" << std::endl;
      return;
    }
    pdu->N_bytes = (uint32_t)bref.distance_bytes();
    printf("measure report reconf:\n");
    for (uint32_t i = 0; i < pdu->N_bytes; i++)
    {
      printf("0x%x\n", *(pdu->msg + i));
    }

    // 下传

    parent->pdcp->write_sdu(rnti, srb_to_lcid(lte_srb::srb1), std::move(pdu));

    auto start = std::chrono::high_resolution_clock::now();
    while (true)
    {
      auto end = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
      if (duration.count() > 1)
      {
        break;
      }
    }
  }

} // namespace srsenb
