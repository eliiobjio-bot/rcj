
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

#include "srsenb/hdr/stack/rrc/rrc.h"
#include "srsenb/hdr/stack/mac/sched_interface.h"
#include "srsenb/hdr/stack/rrc/rrc_cell_cfg.h"
#include "srsenb/hdr/stack/rrc/rrc_endc.h"
#include "srsenb/hdr/stack/rrc/rrc_mobility.h"
#include "srsenb/hdr/stack/rrc/rrc_paging.h"
#include "srsenb/hdr/stack/s1ap/s1ap.h"
#include "srsran/asn1/asn1_utils.h"
#include "srsran/asn1/rrc_utils.h"
#include "srsran/common/bcd_helpers.h"
#include "srsran/common/enb_events.h"
#include "srsran/common/standard_streams.h"
#include "srsran/common/string_helpers.h"
#include "srsran/interfaces/enb_mac_interfaces.h"
#include "srsran/interfaces/enb_pdcp_interfaces.h"
#include "srsran/interfaces/enb_rlc_interfaces.h"

#include "srsenb/hdr/enb.h"
#include <bitset>
#include <iostream>
using srsran::byte_buffer_t;

using namespace asn1::rrc;

namespace srsenb
{

  uint32_t rrc::wx_network_mode = 0;

  // kuopin
  uint32_t rrc::wx_area_mode = 0;
  rrc::rrc(srsran::task_sched_handle task_sched_, enb_bearer_manager &manager_) : logger(srslog::fetch_basic_logger("RRC")), bearer_manager(manager_), task_sched(task_sched_), rx_pdu_queue(128), nas_rx_pdu_queue(128)
  {
  }

  rrc::~rrc() {}

  int32_t rrc::init(const rrc_cfg_t &cfg_,
                    phy_interface_rrc_lte *phy_,
                    mac_interface_rrc *mac_,
                    rlc_interface_rrc *rlc_,
                    pdcp_interface_rrc *pdcp_,
                    s1ap_interface_rrc *s1ap_,
                    gtpu_interface_rrc *gtpu_,
                    adp *rrc_adp_)
  {
    return init(cfg_, phy_, mac_, rlc_, pdcp_, s1ap_, gtpu_, rrc_adp_, nullptr);
  }

  int32_t rrc::init(const rrc_cfg_t &cfg_,
                    phy_interface_rrc_lte *phy_,
                    mac_interface_rrc *mac_,
                    rlc_interface_rrc *rlc_,
                    pdcp_interface_rrc *pdcp_,
                    s1ap_interface_rrc *s1ap_,
                    gtpu_interface_rrc *gtpu_,
                    adp *rrc_adp_,
                    rrc_nr_interface_rrc *rrc_nr_)
  {
    phy = phy_;
    mac = mac_;
    rlc = rlc_;
    pdcp = pdcp_;
    gtpu = gtpu_;
    s1ap = s1ap_;
    rrc_nr = rrc_nr_;

    cfg = cfg_;
    rrc_adp = rrc_adp_;
    // cfg.ttcn_rrc_enble=true;
    std::cout << "cfg.ttcn_rrc_enble:" << cfg.ttcn_rrc_enble << std::endl;

    wx_network_mode = cfg.network_mode;
    // kuopin
    wx_area_mode = cfg.area_mode;
    if (cfg.sibs[12].type() == asn1::rrc::sys_info_r8_ies_s::sib_type_and_info_item_c_::types::sib13_v920 &&
        cfg.enable_mbsfn)
    {
      configure_mbsfn_sibs();
    }

    cell_res_list.reset(new freq_res_common_list{cfg});

    // Loads the PRACH root sequence
    cfg.sibs[1].sib2().rr_cfg_common.prach_cfg.root_seq_idx = cfg.cell_list[0].root_seq_idx;

    if (cfg.num_nr_cells > 0)
    {
      cfg.sibs[1].sib2().ext = true;
      cfg.sibs[1].sib2().plmn_info_list_r15.set_present();
      cfg.sibs[1].sib2().plmn_info_list_r15.get()->resize(1);
      auto &plmn = cfg.sibs[1].sib2().plmn_info_list_r15.get()->back();
      plmn.upper_layer_ind_r15_present = true;
    }

    if (generate_sibs() != SRSRAN_SUCCESS)
    {
      logger.error("Couldn't generate SIBs.");
      return false;
    }

    // configure_mib_wx();
    // generate_sib_wx();

    configure_mib_wx();
    generate_sib_wx();
    // generate_recfg_wx();

    for (int i = 0; i < si_info.mibLen; i++)
    {
      printf("0x:%x\n", *(si_info.mib_msg + i));
    }

    // Generate IoT SystemInformation
    if (cfg.network_mode != 0)
    {
      std::cout << "This is Iot,generate-iot-si" << std::endl;
      generate_iot_si();
    }
    //----------------------------------------------------------------------
    // generate_pag_si();

    config_mac();

    // Check valid inactivity timeout config
    uint32_t t310 = cfg.sibs[1].sib2().ue_timers_and_consts.t310.to_number();
    uint32_t t311 = cfg.sibs[1].sib2().ue_timers_and_consts.t311.to_number();
    uint32_t n310 = cfg.sibs[1].sib2().ue_timers_and_consts.n310.to_number();
    logger.info("T310 %d, T311 %d, N310 %d", t310, t311, n310);
    if (cfg.inactivity_timeout_ms < t310 + t311 + n310)
    {
      srsran::console("\nWarning: Inactivity timeout is smaller than the sum of t310, t311 and n310.\n"
                      "This may break the UE's re-establishment procedure.\n");
      logger.warning("Inactivity timeout is smaller than the sum of t310, t311 and n310. This may break the UE's "
                     "re-establishment procedure.");
    }
    logger.info("Inactivity timeout: %d ms", cfg.inactivity_timeout_ms);
    logger.info("Max consecutive MAC KOs: %d", cfg.max_mac_dl_kos);

    pending_paging.reset(new paging_manager(cfg.sibs[1].sib2().rr_cfg_common.pcch_cfg.default_paging_cycle.to_number(),
                                            cfg.sibs[1].sib2().rr_cfg_common.pcch_cfg.nb.to_number()));

    running = true;

    if (logger.debug.enabled())
    {
      asn1::json_writer js{};
      cfg.srb1_cfg.rlc_cfg.to_json(js);
      logger.debug("SRB1 configuration: %s", js.to_string().c_str());
      js = {};
      cfg.srb2_cfg.rlc_cfg.to_json(js);
      logger.debug("SRB2 configuration: %s", js.to_string().c_str());
    }
    return SRSRAN_SUCCESS;
  }

  void rrc::stop()
  {
    if (running)
    {
      running = false;
      rrc_pdu p = {0, LCID_EXIT, false, nullptr};
      rx_pdu_queue.push_blocking(std::move(p));
    }
    users.clear();
  }

  /*******************************************************************************
    Public functions
  *******************************************************************************/

  void rrc::get_metrics(rrc_metrics_t &m)
  {
    if (running)
    {
      m.ues.resize(users.size());
      size_t count = 0;
      for (auto &ue : users)
      {
        ue.second->get_metrics(m.ues[count++]);
      }
    }
  }

  /*******************************************************************************
    MAC interface

    Those functions that shall be called from a phch_worker should push the command
    to the queue and process later
  *******************************************************************************/

  uint8_t *rrc::read_pdu_bcch_dlsch(const uint8_t cc_idx, const uint32_t sib_index)
  {
    if (sib_index < ASN1_RRC_MAX_SIB && cc_idx < cell_common_list->nof_cells())
    {
      return cell_common_list->get_cc_idx(cc_idx)->sib_buffer.at(sib_index)->msg;
    }
    return nullptr;
  }
  bool rrc::read_pdu_IoTsi(uint8_t *payload, int &len, uint8_t &tbcchSlotStart_, uint8_t &BandID, uint8_t &FrameID)
  {
    len = si_info.IoTsiLen;
    memcpy(payload, si_info.IoTsi_msg, len);
    return true;
  }

  bool rrc::modify_sib_sdu_ttcn(uint8_t *sibData)
  {
    uint8_t temp = 0;
    uint8_t temp1 = 0;
    uint8_t temp2 = 0;

    std::cout << "x2 cfg.ttcn_rrc_enble:" << cfg.ttcn_rrc_enble << std::endl;
    std::cout << "x2 cfg.ttcn_test_enble:" << cfg.ttcn_test_enble << std::endl;
    std::cout << "x2 rrc_adp->udp_.TC_513_mib_sib:" << rrc_adp->udp_.TC_513_mib_sib << std::endl;
    std::cout << "xk---TC_513_barred---" << rrc_adp->udp_.TC_513_barred << std::endl;
    std::cout << "xk--TC_513_mac_not_barred-" << rrc_adp->udp_.TC_513_mac_not_barred << std::endl;

    if (rrc_adp->udp_.TC_513_mac_not_barred)
    {
      rrc_adp->udp_.TC_513_mac_not_barred = false;


      // rrc_adp->udp_.TC_513_barred_con_req=true;
    }
    else if (cfg.ttcn_rrc_enble && cfg.ttcn_test_enble && rrc_adp->udp_.TC_513_barred)
    {
      temp = (*(sibData + 3) & 0x02);
      std::cout << "sibData temp barred:" << temp << std::endl;
      *((sibData + 3)) = temp;
      std::cout << "xk---TC_513_barred" << rrc_adp->udp_.TC_513_barred << std::endl;
      rrc_adp->udp_.TC_513_barred = false;
    }
    else if (rrc_adp->udp_.TC_512_qrexlevmin)
    {
      temp1 = (*(sibData + 3) | 0x05);
      std::cout << "sibData temp qrxlevmin1:" << (int)temp1 << std::endl;
      *((sibData + 3)) = temp1;
      temp2 = (*(sibData + 4) & 0x7f);
      std::cout << "sibData temp qrxlevmin2:" << (int)temp2 << std::endl;
      *((sibData + 4)) = temp2;
      rrc_adp->udp_.TC_512_con_req = true;
      rrc_adp->udp_.TC_512_qrexlevmin = false;
    }
    // else if (rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId==514)
    // {
    //   temp1 = (*(sibData + 3) | 0x05);
    //   //17
    //   std::cout << "sibData temp1 qrxlevmin:" << (int)temp1 << std::endl;
    //   *((sibData + 3)) = temp1;
    // }
    else
    {
    }

    return true;
  }

  bool rrc::modify_mib_sdu(uint8_t *mibData, int sfn)
  {
    srsran::console("修改MIB消息中参数:sys_sh_fra_num=%d\n", sfn);
    *(mibData + 4) = sfn >> 5;
    *(mibData + 5) = (*(mibData + 5) & 0x07) | ((sfn & 0x1f) << 3);
    srsran::console("----mibData-4:%x------", mibData[4]);
    srsran::console("----mibData-5:%x------", mibData[5]);
    srsran::console("当前的启动tti=%d,当前的sfn=%d\n", sfn * 52, sfn);
    uint8_t temp = 0;

    std::cout << "x1 cfg.ttcn_rrc_enble:" << cfg.ttcn_rrc_enble << std::endl;
    std::cout << "x1 cfg.ttcn_test_enble:" << cfg.ttcn_test_enble << std::endl;
    std::cout << "x1 rrc_adp->udp_.TC_513_mib_sib:" << rrc_adp->udp_.TC_513_mib_sib << std::endl;
    std::cout << "x1 TC_513_mac_mib " << rrc_adp->udp_.TC_513_mac_mib << std::endl;

    if (rrc_adp->udp_.TC_513_mac_mib)
    {
      rrc_adp->udp_.TC_513_mac_mib = false;
      temp = (*(mibData + 3) | 0x01);
      std::cout << "mibData temp:" << temp << std::endl;
      *((mibData + 3)) = temp;
    }

    return true;
  }

  bool rrc::read_pdu_mib(uint8_t *payload, int &len, uint8_t &bcchSlotStart_, uint8_t &BandID, uint8_t &FrameID, int sfn)
  {
    std::cout << "  si_info.mib_msg address2:" << &si_info.mib_msg << std::endl;
    std::cout << "--------@@@@TTCN@@@@----MIB-111--" << std::endl;
    for (int i = 0; i < si_info.mibLen; i++)
    {
      printf("0x:%x\n", *(si_info.mib_msg + i));
    }
    len = si_info.mibLen;
    modify_mib_sdu(si_info.mib_msg, sfn);
    //    srsran::console("mibLen:%d",len);
    memcpy(payload, si_info.mib_msg, len);
    std::cout << "--------@@@@TTCN@@@@----MIB---" << std::endl;
    for (int i = 0; i < len; i++)
    {
      printf("0x:%x\n", *(payload + i));
    }
    bcchSlotStart_ = 1;
    // memcpy(payload, mib_buffer[0]->msg, 7);
    // bcchSlotStart_ = BcchInfo.bcchSoltStart;
    // BandID         = BcchInfo.BandID;
    // FrameID        = BcchInfo.FrameID;
    BandID = 3;
    FrameID = 1;
    // srsran::console("读取MIB消息\n");
    if (p_pcap_net)
    {
      p_pcap_net->write_dl_rrc_pdu(payload, len, CY_LOGICCHANNEL_TYPE_MBCH, CY_NET_MODE_RAN);
    }
    return true;
  }

  bool rrc::read_pdu_sib(uint8_t *payload, int &len, sibInfo_t *sibInfo)
  {
    // 测试
    len = si_info.sibLen;
    // std::cout << "--------@@@@TTCN@@@@-------1" << std::endl;
    // for (int i = 0; i < len; i++)
    // {
    //   printf("0x:%x\n", *(si_info.sib_msg + i));
    // }
    // srsran::console("sibLen:%d",len);
    modify_sib_sdu_ttcn(si_info.sib_msg);

#if 0
    std::cout<<"------------------rrrrrrrrrrrrrrrrririririririr--"<<std::endl;
      for (int i = 0; i < len; i++)
    {
      printf("0x:%x\n", *(si_info.sib_msg + i));
    }
#endif
    memcpy(payload, si_info.sib_msg, len);
    /// len              = sib_wx_buffer[0]->N_bytes;
    // bbchFrameAssign_ = BbchInfo.bbchFrameAssinment;
    // agchFrameAssign_ = AgchInfo.agchFrameAssinment;
    // BandID           = BbchInfo.BandID;
    // FrameID          = BbchInfo.FrameID;
    // 这部分内容后面可以从SIB消息中动态获�?
    // std::cout << "--------@@@@TTCN@@@@-------" << std::endl;

#if 0
    for (int i = 0; i < len; i++)
    {
      printf("0x:%x\n", *(payload + i));
    }
#endif

    sibInfo->rarWindows = 10;
    sibInfo->bbchFrameAssinment = 0b0011;
    sibInfo->agchFrameAssinment = 0b1100;
    sibInfo->bbchSlotcfg = 1;
    sibInfo->BandID = 3;
    sibInfo->FrameID = 1;
    // srsran::console("读取SIB消息 \n");

    if (p_pcap_net)
    {
      p_pcap_net->write_dl_rrc_pdu(payload, len, CY_LOGICCHANNEL_TYPE_SBCH, CY_NET_MODE_RAN);
    }

    return true;
  }
  void rrc::set_radiolink_dl_state(uint16_t rnti, bool crc_res)
  {
    // embed parameters in arg value
    rrc_pdu p = {rnti, LCID_RADLINK_DL, crc_res, nullptr};

    if (not rx_pdu_queue.try_push(std::move(p)))
    {
      logger.error("Failed to push radio link DL state");
    }
  }

  void rrc::set_radiolink_ul_state(uint16_t rnti, bool crc_res)
  {
    // embed parameters in arg value
    rrc_pdu p = {rnti, LCID_RADLINK_UL, crc_res, nullptr};

    if (not rx_pdu_queue.try_push(std::move(p)))
    {
      logger.error("Failed to push radio link UL state");
    }
  }

  void rrc::set_activity_user(uint16_t rnti)
  {
    rrc_pdu p = {rnti, LCID_ACT_USER, false, nullptr};

    if (not rx_pdu_queue.try_push(std::move(p)))
    {
      logger.error("Failed to push UE activity command to RRC queue");
    }
  }

  void rrc::rem_user_thread(uint16_t rnti)
  {
    rrc_pdu p = {rnti, LCID_REM_USER, false, nullptr};
    if (not rx_pdu_queue.try_push(std::move(p)))
    {
      logger.error("Failed to push UE remove command to RRC queue");
    }
  }

  uint32_t rrc::get_nof_users()
  {
    return users.size();
  }

  void rrc::max_retx_attempted(uint16_t rnti)
  {
    rrc_pdu p = {rnti, LCID_RLC_RTX, false, nullptr};
    if (not rx_pdu_queue.try_push(std::move(p)))
    {
      logger.error("Failed to push max Retx event to RRC queue");
    }
  }

  void rrc::protocol_failure(uint16_t rnti)
  {
    rrc_pdu p = {rnti, LCID_PROT_FAIL, false, nullptr};
    if (not rx_pdu_queue.try_push(std::move(p)))
    {
      logger.error("Failed to push protocol failure to RRC queue");
    }
  }

  // This function is called from PRACH worker (can wait)
  int rrc::add_user(uint16_t rnti, const sched_interface::ue_cfg_t &sched_ue_cfg)
  {
    auto user_it = users.find(rnti);
    if (user_it == users.end())
    {
      if (rnti != SRSRAN_MRNTI)
      {
        // only non-eMBMS RNTIs are present in user map
        unique_rnti_ptr<ue> u = make_rnti_obj<ue>(rnti, this, rnti, sched_ue_cfg);
        if (u->init() != SRSRAN_SUCCESS)
        {
          logger.error("Adding user rnti=0x%x - Failed to allocate user resources", rnti);
          return SRSRAN_ERROR;
        }
        users.insert(std::make_pair(rnti, std::move(u)));
      }
      rlc->add_user(rnti);
      pdcp->add_user(rnti);
      logger.info("Added new user rnti=0x%x", rnti);
    }
    else
    {
      logger.error("Adding user rnti=0x%x (already exists)", rnti);
    }

    if (rnti == SRSRAN_MRNTI)
    {
      for (auto &mbms_item : mcch.msg.c1().mbsfn_area_cfg_r9().pmch_info_list_r9[0].mbms_session_info_list_r9)
      {
        uint32_t lcid = mbms_item.lc_ch_id_r9;
        uint32_t addr_in;
        // adding UE object to MAC for MRNTI without scheduling configuration (broadcast not part of regular scheduling)
        rlc->add_bearer_mrb(SRSRAN_MRNTI, lcid);
        bearer_manager.add_eps_bearer(SRSRAN_MRNTI, 1, srsran::srsran_rat_t::lte, lcid);
        pdcp->add_bearer(SRSRAN_MRNTI, lcid, srsran::make_drb_pdcp_config_t(1, false));
        gtpu->add_bearer(SRSRAN_MRNTI, lcid, 1, 1, addr_in);
      }
    }
    return SRSRAN_SUCCESS;
  }

  /* Function called by MAC after the reception of a C-RNTI CE indicating that the UE still has a
   * valid RNTI.
   */
  void rrc::upd_user(uint16_t new_rnti, uint16_t old_rnti)
  {
    // Remove new_rnti
    auto new_ue_it = users.find(new_rnti);
    if (new_ue_it != users.end())
    {
      new_ue_it->second->deactivate_bearers();
      rem_user_thread(new_rnti);
    }

    // Send Reconfiguration to old_rnti if is RRC_CONNECT or RRC Release if already released here
    auto old_it = users.find(old_rnti);
    if (old_it == users.end())
    {
      logger.info("rnti=0x%x received MAC CRNTI CE: 0x%x, but old context is unavailable", new_rnti, old_rnti);
      return;
    }
    ue *ue_ptr = old_it->second.get();

    if (ue_ptr->mobility_handler->is_ho_running())
    {
      ue_ptr->mobility_handler->trigger(ue::rrc_mobility::user_crnti_upd_ev{old_rnti, new_rnti});
    }
    else
    {
      logger.info("Resuming rnti=0x%x RRC connection due to received C-RNTI CE from rnti=0x%x.", old_rnti, new_rnti);
      if (ue_ptr->is_connected())
      {
        // Send a new RRC Reconfiguration to overlay previous
        old_it->second->send_connection_reconf();
      }
    }

    // Log event.
    event_logger::get().log_connection_resume(
        ue_ptr->get_cell_list().get_ue_cc_idx(UE_PCELL_CC_IDX)->cell_common->enb_cc_idx, old_rnti, new_rnti);
  }

  // Note: this method is not part of UE methods, because the UE context may not exist anymore when reject is sent
  void rrc::send_rrc_connection_reject(uint16_t rnti)
  {
    dl_ccch_msg_s dl_ccch_msg;
    dl_ccch_msg.msg.set_c1().set_rrc_conn_reject().crit_exts.set_c1().set_rrc_conn_reject_r8().wait_time = 10;

    // Allocate a new PDU buffer, pack the message and send to PDCP
    srsran::unique_byte_buffer_t pdu = srsran::make_byte_buffer();
    if (pdu == nullptr)
    {
      logger.error("Allocating pdu");
      return;
    }
    asn1::bit_ref bref(pdu->msg, pdu->get_tailroom());
    if (dl_ccch_msg.pack(bref) != asn1::SRSASN_SUCCESS)
    {
      logger.error(pdu->msg, bref.distance_bytes(), "Failed to pack DL-CCCH-Msg:");
      return;
    }
    pdu->N_bytes = bref.distance_bytes();
    log_rrc_message(Tx, rnti, srb_to_lcid(lte_srb::srb0), *pdu, dl_ccch_msg, dl_ccch_msg.msg.c1().type().to_string());

    rlc->write_sdu(rnti, srb_to_lcid(lte_srb::srb0), std::move(pdu));
  }

  /*******************************************************************************
    PDCP interface
  *******************************************************************************/
  void rrc::write_pdu(uint16_t rnti, uint32_t lcid, srsran::unique_byte_buffer_t pdu)
  {
    srsran::console("----------RRC Receive UL PDU-------\n");
    for (int i = 0; i < (int)pdu->N_bytes; i++)
    {
      srsran::console("0x%x\n", *(pdu->msg + i));
    }

    rrc_pdu p = {rnti, lcid, false, std::move(pdu)};
    /*
      rrc_pdu_s  q = {rnti, lcid, false, std::move(pdu)};
      if (not rx_pdu_queue_s.try_push(std::move(q))) {
        logger.error("Failed to push Release command to RRC queue");
      }
      */
    if (not rx_pdu_queue.try_push(std::move(p)))
    {

      srsran::console("Failed to push Release command to RRC queue");
      logger.error("Failed to push Release command to RRC queue");
    }
  }

  void rrc::notify_pdcp_integrity_error(uint16_t rnti, uint32_t lcid)
  {
    logger.warning("Received integrity protection failure indication, rnti=0x%x, lcid=%u", rnti, lcid);
    s1ap->user_release(rnti, asn1::s1ap::cause_radio_network_opts::unspecified);
  }

  /*******************************************************************************
    S1AP interface
  *******************************************************************************/
  void rrc::write_dl_info(uint16_t rnti, srsran::unique_byte_buffer_t sdu)
  {
    dl_dcch_msg_s dl_dcch_msg;
    dl_dcch_msg.msg.set_c1();
    dl_dcch_msg_type_c::c1_c_ *msg_c1 = &dl_dcch_msg.msg.c1();

    auto user_it = users.find(rnti);
    if (user_it != users.end())
    {
      dl_info_transfer_r8_ies_s *dl_info_r8 =
          &msg_c1->set_dl_info_transfer().crit_exts.set_c1().set_dl_info_transfer_r8();
      //    msg_c1->dl_info_transfer().rrc_transaction_id = ;
      dl_info_r8->non_crit_ext_present = false;
      dl_info_r8->ded_info_type.set_ded_info_nas();
      dl_info_r8->ded_info_type.ded_info_nas().resize(sdu->N_bytes);
      memcpy(msg_c1->dl_info_transfer().crit_exts.c1().dl_info_transfer_r8().ded_info_type.ded_info_nas().data(),
             sdu->msg,
             sdu->N_bytes);

      sdu->clear();

      user_it->second->send_dl_dcch(&dl_dcch_msg, std::move(sdu));
    }
    else
    {
      logger.error("Rx SDU for unknown rnti=0x%x", rnti);
    }
  }

  //----------------------------------IOT---------------------------

  void rrc::iot_write_dl_info(uint16_t rnti, srsran::unique_byte_buffer_t sdu)
  {
    iot_dl_dcch_msg_s iot_dl_dcch_msg; // IoT
    iot_dl_dcch_msg.msg.set_iot_dl_information_trans();
    iot_dl_information_trans_s *info_trans = &iot_dl_dcch_msg.msg.set_iot_dl_information_trans();

    auto user_it = users.find(rnti);
    if (user_it != users.end())
    {
      info_trans->iot_dl_information_trans_r1.ded_info_nas.ded_info_nas.resize(sdu->N_bytes);
      memcpy(info_trans->iot_dl_information_trans_r1.ded_info_nas.ded_info_nas.data(), sdu->msg, sdu->N_bytes);

      sdu->clear();
      user_it->second->iot_send_dl_dcch(&iot_dl_dcch_msg, std::move(sdu));
    }
    else
    {
      logger.error("Rx SDU for unknown rnti=0x%x", rnti);
    }
  }
  //--------------------------------------------------------

  void rrc::release_ue(uint16_t rnti)
  {
    rrc_pdu p = {rnti, LCID_REL_USER, false, nullptr};
    if (not rx_pdu_queue.try_push(std::move(p)))
    {
      logger.error("Failed to push Release command to RRC queue");
    }
  }

  bool rrc::setup_ue_ctxt(uint16_t rnti, const asn1::s1ap::init_context_setup_request_s &msg)
  {
    logger.info("Adding initial context for 0x%x", rnti);
    auto user_it = users.find(rnti);
    if (user_it == users.end())
    {
      logger.warning("Unrecognised rnti: 0x%x", rnti);
      return false;
    }
    user_it->second->handle_ue_init_ctxt_setup_req(msg);
    return true;
  }

  bool rrc::modify_ue_ctxt(uint16_t rnti, const asn1::s1ap::ue_context_mod_request_s &msg)
  {
    logger.info("Modifying context for 0x%x", rnti);
    auto user_it = users.find(rnti);

    if (user_it == users.end())
    {
      logger.warning("Unrecognised rnti: 0x%x", rnti);
      return false;
    }

    return user_it->second->handle_ue_ctxt_mod_req(msg);
  }

  bool rrc::release_erabs(uint32_t rnti)
  {
    logger.info("Releasing E-RABs for 0x%x", rnti);
    auto user_it = users.find(rnti);

    if (user_it == users.end())
    {
      logger.warning("Unrecognised rnti: 0x%x", rnti);
      return false;
    }

    bool ret = user_it->second->release_erabs();
    return ret;
  }

  int rrc::release_erab(uint16_t rnti, uint16_t erab_id)
  {
    logger.info("Releasing E-RAB id=%d for 0x%x", erab_id, rnti);
    auto user_it = users.find(rnti);

    if (user_it == users.end())
    {
      logger.warning("Unrecognised rnti: 0x%x", rnti);
      return SRSRAN_ERROR;
    }

    return user_it->second->release_erab(erab_id);
  }

  int rrc::notify_ue_erab_updates(uint16_t rnti, srsran::const_byte_span nas_pdu)
  {
    auto user_it = users.find(rnti);
    if (user_it == users.end())
    {
      logger.warning("Unrecognised rnti: 0x%x", rnti);
      return SRSRAN_ERROR;
    }
    user_it->second->send_connection_reconf(nullptr, false, nas_pdu);
    return SRSRAN_SUCCESS;
  }

  bool rrc::has_erab(uint16_t rnti, uint32_t erab_id) const
  {
    auto user_it = users.find(rnti);
    if (user_it == users.end())
    {
      logger.warning("Unrecognised rnti: 0x%x", rnti);
      return false;
    }
    return user_it->second->has_erab(erab_id);
  }

  int rrc::get_erab_addr_in(uint16_t rnti, uint16_t erab_id, transp_addr_t &addr_in, uint32_t &teid_in) const
  {
    auto user_it = users.find(rnti);
    if (user_it == users.end())
    {
      logger.warning("Unrecognised rnti: 0x%x", rnti);
      return SRSRAN_ERROR;
    }
    return user_it->second->get_erab_addr_in(erab_id, addr_in, teid_in);
  }

  void rrc::set_aggregate_max_bitrate(uint16_t rnti, const asn1::s1ap::ue_aggregate_maximum_bitrate_s &bitrate)
  {
    auto user_it = users.find(rnti);
    if (user_it == users.end())
    {
      logger.warning("Unrecognised rnti: 0x%x", rnti);
      return;
    }
    user_it->second->set_bitrates(bitrate);
  }

  int rrc::setup_erab(uint16_t rnti,
                      uint16_t erab_id,
                      const asn1::s1ap::erab_level_qos_params_s &qos_params,
                      srsran::const_span<uint8_t> nas_pdu,
                      const asn1::bounded_bitstring<1, 160, true, true> &addr,
                      uint32_t gtpu_teid_out,
                      asn1::s1ap::cause_c &cause)
  {
    logger.info("Setting up erab id=%d for 0x%x", erab_id, rnti);
    auto user_it = users.find(rnti);
    if (user_it == users.end())
    {
      logger.warning("Unrecognised rnti: 0x%x", rnti);
      cause.set_radio_network().value = asn1::s1ap::cause_radio_network_opts::unknown_erab_id;
      return SRSRAN_ERROR;
    }
    return user_it->second->setup_erab(erab_id, qos_params, nas_pdu, addr, gtpu_teid_out, cause);
  }

  int rrc::modify_erab(uint16_t rnti,
                       uint16_t erab_id,
                       const asn1::s1ap::erab_level_qos_params_s &qos_params,
                       srsran::const_span<uint8_t> nas_pdu,
                       asn1::s1ap::cause_c &cause)
  {
    logger.info("Modifying E-RAB for 0x%x. E-RAB Id %d", rnti, erab_id);
    auto user_it = users.find(rnti);
    if (user_it == users.end())
    {
      logger.warning("Unrecognised rnti: 0x%x", rnti);
      cause.set_radio_network().value = asn1::s1ap::cause_radio_network_opts::unknown_erab_id;
      return SRSRAN_ERROR;
    }

    return user_it->second->modify_erab(erab_id, qos_params, nas_pdu, cause);
  }

  /*******************************************************************************
    Paging functions
    These functions use a different mutex because access different shared variables
    than user map
  *******************************************************************************/

  //------------------------2024.03.16-----------------------------------------------
  void rrc::add_paging_id_wx_s(srsran::unique_byte_buffer_t pdu)
  {
    std::cout << "-----------545454545-------xiesi-------" << std::endl;

    for (uint32_t i = 0; i < pdu->N_bytes; i++)
    {
      printf("0x%x\n", *(pdu->msg + i));
    }

    std::cout << "------------------xiesi--zhihoujisuanquanbu-----" << std::endl;
    //  pcchPdu[0]=0x40|((pdu->msg[0]>>6)&0x03);
    //  pcchPdu[1]=((pdu->msg[0]<<2)&0xfc)|((pdu->msg[1]>>6)&0x03);
    //  pcchPdu[2]=((pdu->msg[1]<<2)&0xfc)|((pdu->msg[2]>>6)&0x03);
    //  pcchPdu[3]=((pdu->msg[2]<<2)&0xfc)|((pdu->msg[3]>>6)&0x03);
    //  pcchPdu[4]=((pdu->msg[3]<<2)&0xfc)|((pdu->msg[4]>>6)&0x03);
    //  pcchPdu[5]=((pdu->msg[4]<<2)&0xfc)|((pdu->msg[5]>>6)&0x03);
    //  pcchPdu[6]=(pdu->msg[5]<<2)&0xfc;
    pcchPdu[0] = 0x00;
    pcchPdu[1] = 0x41;
    pcchPdu[2] = 0xc2;
    pcchPdu[3] = 0x34;
    pcchPdu[4] = 0x56;
    pcchPdu[5] = 0x78;
    pcchPduLen = 6;
    if (rrc_adp->udp_.TC_516_is_paging_invalid)
    {
      pcchPdu[0] = 0x00;
      pcchPdu[1] = 0x40;
      pcchPdu[2] = 0xc2;
      pcchPdu[3] = 0x34;
      pcchPdu[4] = 0x56;
      pcchPdu[5] = 0x78;
    }

    for (uint32_t i = 0; i < pcchPduLen; i++)
    {
      printf("0x%x\n", pcchPdu[i]);
    }
  }

  bool rrc::read_pdu_pcch_wx_s(int tti_tx_dl, uint8_t *payload, int &len)
  {
    std::cout << "=========4444444=========ueyueywfuiewyu" << std::endl;
    if (0 == pcchPduLen)
    {
      len = 0;
      return false;
    }
    else
    {
      len = pcchPduLen;
      memcpy(payload, pcchPdu, pcchPduLen);
      // std::copy(pcchPdu,&pcchPdu[6],payload);
      // if(pcch_num++==1){
      pcchPduLen = 0;
      pcch_num = 0;
      //}
      return true;
    }
    return true;
  }
  //--------------------------------------------------------------------------------

  void rrc::add_paging_id(uint32_t ueid, const asn1::s1ap::ue_paging_id_c &ue_paging_id)
  {
    if (ue_paging_id.type().value == asn1::s1ap::ue_paging_id_c::types_opts::imsi)
    {
      pending_paging->add_imsi_paging(ueid, ue_paging_id.imsi());
    }
    else
    {
      pending_paging->add_tmsi_paging(ueid, ue_paging_id.s_tmsi().mmec[0], ue_paging_id.s_tmsi().m_tmsi);
    }
  }

  bool rrc::is_paging_opportunity(uint32_t tti, uint32_t *payload_len)
  {
    *payload_len = pending_paging->pending_pcch_bytes(tti_point(tti));
    return *payload_len > 0;
  }

  void rrc::read_pdu_pcch(uint32_t tti_tx_dl, uint8_t *payload, uint32_t buffer_size)
  {
    auto read_func = [this, payload, buffer_size](srsran::const_byte_span pdu, const pcch_msg_s &msg, bool first_tx)
    {
      // copy PCCH pdu to buffer
      if (pdu.size() > buffer_size)
      {
        logger.warning("byte buffer with size=%zd is too small to fit pcch msg with size=%zd", buffer_size, pdu.size());
        return false;
      }
      std::copy(pdu.begin(), pdu.end(), payload);
      if (first_tx)
      {
        logger.info("Assembling PCCH payload with %d UE identities, payload_len=%d bytes",
                    msg.msg.c1().paging().paging_record_list.size(),
                    pdu.size());
        log_broadcast_rrc_message(SRSRAN_PRNTI, pdu, msg, msg.msg.c1().type().to_string());
      }
      return true;
    };

    pending_paging->read_pdu_pcch(tti_point(tti_tx_dl), read_func);
  }

  /*******************************************************************************
    Handover functions
  *******************************************************************************/

  void rrc::ho_preparation_complete(uint16_t rnti,
                                    ho_prep_result result,
                                    const asn1::s1ap::ho_cmd_s &msg,
                                    srsran::unique_byte_buffer_t rrc_container)
  {
    users.at(rnti)->mobility_handler->handle_ho_preparation_complete(result, msg, std::move(rrc_container));
  }

  void rrc::set_erab_status(uint16_t rnti, const asn1::s1ap::bearers_subject_to_status_transfer_list_l &erabs)
  {
    auto ue_it = users.find(rnti);
    if (ue_it == users.end())
    {
      logger.warning("rnti=0x%x does not exist", rnti);
      return;
    }
    ue_it->second->mobility_handler->trigger(erabs);
  }

  /*******************************************************************************
    IoT functions (2023-08-09)
  ********************************************************************************/
  void rrc::generate_iot_si()
  {
    // generate and pack into IoT_SI buffers
    enb_cell_common *cell_ctxt = cell_common_list->get_cc_idx(0);
    tbcch_msg_s tbcch_msg;

    iot_sib_s &iot_si = tbcch_msg.msg.iot_sib();
    iot_si.iot_uac_barring_list_present = true;
    iot_si.iot_uac_barring_list.resize(1);
    iot_si.iot_uac_barring_list[0].access_category = iot_uac_barring_per_cat_s::access_category_opts::acCatNum3;
    iot_si.iot_uac_barring_list[0].uac_barring_for_access_id.from_string("10001");

    // iot_uac_barring_per_cat_s iot_uac_barring_per;
    // iot_uac_barring_per.access_category = iot_uac_barring_per_cat_s::access_category_opts::acCatNum3;
    // iot_uac_barring_per.uac_barring_for_access_id.from_string("10001");
    // iot_si.iot_uac_barring_list[0] = iot_uac_barring_per;

    iot_si.iot_dl_trans_coies.from_string("101");

    iot_rach_config_s &iot_rach_conf = iot_si.iot_rach_config;
    iot_rach_conf.dl_band_id.ba_id.from_string("101001");
    iot_rach_conf.iot_rach_freq_list.resize(1);
    iot_rach_conf.iot_rach_freq_list[0].ul_freq_id.freq_id.from_string("00");
    iot_rach_conf.iot_rach_freq_list[0].ul_sub_freq_bit_map.from_string("0101");
    iot_rach_conf.ul_fn_assignment.from_string("11001111");
    iot_rach_conf.ra_response_windowsize = iot_rach_config_s::ra_response_windowsize_opts::rf30;

    iot_si.iot_agch_config.set_iot_agch_conf_normal();
    iot_agch_conf_normal_s &iot_agch_conf = iot_si.iot_agch_config.set_iot_agch_conf_normal();
    iot_agch_conf.dl_band_id.ba_id.from_string("010110");
    iot_agch_conf.dl_freq_id.freq_id.from_string("11");
    iot_agch_conf.ul_fn_assignment.from_string("00110101");

    // iot_agch_conf_dl_freq_spread_s& iot_agch_conf = iot_si.iot_agch_config.set_iot_agch_conf_dl_freq_spread();
    // iot_agch_conf.spread_factor = iot_agch_conf_dl_freq_spread_s::spread_factor_opts::sf128;
    // iot_agch_conf.code_index.from_string("111111111");
    // iot_agch_conf.dl_fn_assignment.from_string("10101010");

    iot_si.iot_ue_timer.iot_t300 = iot_ue_timer_s::iot_t300_opts::ms2000;
    iot_si.iot_ue_timer.iot_t301 = iot_ue_timer_s::iot_t301_opts::ms2000;
    iot_si.iot_ue_timer.iot_t302 = iot_ue_timer_s::iot_t302_opts::ms200;

    // Allocate a new PDU buffer, pack the IoT_SI message
    srsran::unique_byte_buffer_t iot_si_buffer = srsran::make_byte_buffer();
    if (iot_si_buffer)
    {
      asn1::bit_ref bref(iot_si_buffer->msg, iot_si_buffer->get_tailroom());
      if (tbcch_msg.pack(bref) != asn1::SRSASN_SUCCESS)
      {
        srsran::console("-------------pack error-----------\n");
      }
      iot_si_buffer->N_bytes = bref.distance_bytes();
    }
    else
    {
    }
    cell_ctxt->iot_si_buffer.push_back(std::move(iot_si_buffer));
    std::cout << "-------------------------------当前消息为IOT消息----------------------------" << std::endl;
    srsran::console("当前字节�?=%u\n", cell_ctxt->iot_si_buffer[0]->N_bytes);
    for (uint8_t i = 0; i < cell_ctxt->iot_si_buffer[0]->N_bytes; i++)
    {
      srsran::console("%x\n", *(cell_ctxt->iot_si_buffer[0]->msg + i));
    }
    /*si_info.mibLen  = cell_ctxt->iot_si_buffer[0]->N_bytes;
    si_info.mib_msg = cell_ctxt->iot_si_buffer[0]->msg;*/
    si_info.IoTsi_msg = cell_ctxt->iot_si_buffer[0]->msg;
    si_info.IoTsiLen = cell_ctxt->iot_si_buffer[0]->N_bytes;
  }

  /*******************************************************************************
  paging functions(2023 - 09 - 04)
  ********************************************************************************/
  // void rrc::generate_pag_si()
  // {
  //   // generate and pack into paging_SI buffers
  //    enb_cell_common* cell_ctxt = cell_common_list->get_cc_idx(0);
  //    pag_wx_s       pag_si;
  //    pag_si.pag_reco_list_present = true;
  //    pag_si.pag_reco_list.resize(1);
  //    pag_si.pag_reco_list[0].pag_ue_id.nr_5g_s_tmsi.nr_5g_s_tmsi.from_string("110011001010100011001100");
  //    pag_si.pag_sport_info_indic_present = true;
  //    pag_si.pag_sport_info_indic         = pag_wx_s::pag_sport_info_indic_opts::True;

  //      srsran::unique_byte_buffer_t pag_si_buffer = srsran::make_byte_buffer();
  //    if (pag_si_buffer) {
  //     asn1::bit_ref bref(pag_si_buffer->msg, pag_si_buffer->get_tailroom());
  //     if (pag_si.pack(bref) != asn1::SRSASN_SUCCESS) {
  //       srsran::console("-------------pack error-----------\n");
  //     }
  //     pag_si_buffer->N_bytes = bref.distance_bytes();
  //   } else {
  //   }
  //    cell_ctxt->pag_si_buffer.push_back(std::move(pag_si_buffer));
  //    srsran::console("当前字节�?=%u\n", cell_ctxt->pag_si_buffer[0]->N_bytes);
  //    for (uint8_t i = 0; i < cell_ctxt->pag_si_buffer[0]->N_bytes; i++) {
  //     srsran::console("%x\n", *(cell_ctxt->pag_si_buffer[0]->msg + i));
  //    }
  //    /*si_info.mibLen  = cell_ctxt->iot_si_buffer[0]->N_bytes;
  //    si_info.mib_msg = cell_ctxt->iot_si_buffer[0]->msg;*/
  //    si_info.pag_msg = cell_ctxt->pag_si_buffer[0]->msg;
  //    si_info.pagLen  = cell_ctxt->pag_si_buffer[0]->N_bytes;
  // }

  /*******************************************************************************
    EN-DC/NSA helper functions
  *******************************************************************************/

  void rrc::sgnb_addition_ack(uint16_t eutra_rnti, sgnb_addition_ack_params_t params)
  {
    logger.info("Received SgNB addition acknowledgement for rnti=0x%x", eutra_rnti);
    auto ue_it = users.find(eutra_rnti);
    if (ue_it == users.end())
    {
      logger.warning("rnti=0x%x does not exist", eutra_rnti);
      return;
    }
    ue_it->second->endc_handler->trigger(ue::rrc_endc::sgnb_add_req_ack_ev{params});

    // trigger RRC Reconfiguration to send NR config to UE
    ue_it->second->send_connection_reconf();
  }

  void rrc::sgnb_addition_reject(uint16_t eutra_rnti)
  {
    logger.error("Received SgNB addition reject for rnti=%d", eutra_rnti);
    auto ue_it = users.find(eutra_rnti);
    if (ue_it == users.end())
    {
      logger.warning("rnti=0x%x does not exist", eutra_rnti);
      return;
    }
    ue_it->second->endc_handler->trigger(ue::rrc_endc::sgnb_add_req_reject_ev{});
  }

  void rrc::sgnb_addition_complete(uint16_t eutra_rnti, uint16_t nr_rnti)
  {
    logger.info("User rnti=0x%x successfully enabled EN-DC", eutra_rnti);
    auto ue_it = users.find(eutra_rnti);
    if (ue_it == users.end())
    {
      logger.warning("rnti=0x%x does not exist", eutra_rnti);
      return;
    }
    ue_it->second->endc_handler->trigger(ue::rrc_endc::sgnb_add_complete_ev{nr_rnti});
  }

  void rrc::sgnb_inactivity_timeout(uint16_t eutra_rnti)
  {
    logger.info("Received NR inactivity timeout for rnti=0x%x - releasing UE", eutra_rnti);
    auto ue_it = users.find(eutra_rnti);
    if (ue_it == users.end())
    {
      logger.warning("rnti=0x%x does not exist", eutra_rnti);
      return;
    }
    s1ap->user_release(eutra_rnti, asn1::s1ap::cause_radio_network_opts::user_inactivity);
  }

  void rrc::sgnb_release_ack(uint16_t eutra_rnti)
  {
    auto ue_it = users.find(eutra_rnti);
    if (ue_it != users.end())
    {
      logger.info("Received SgNB release acknowledgement for rnti=0x%x", eutra_rnti);
      ue_it->second->endc_handler->trigger(ue::rrc_endc::sgnb_rel_req_ack_ev{});
    }
    else
    {
      // The EUTRA does not need to wait for Release Ack in case it wants to destroy the EUTRA UE
      logger.info("Received SgNB release acknowledgement for already released rnti=0x%x", eutra_rnti);
    }
  }

  /*******************************************************************************
    Private functions
    All private functions are not mutexed and must be called from a mutexed environment
    from either a public function or the internal thread
  *******************************************************************************/
  //------------------------------------------------------------------------------------------------------------------------------------------------------------
  void rrc::parse_ul_ccch_s(ue &ue, srsran::unique_byte_buffer_t pdu)
  {
    srsran_assert(pdu != nullptr, "parse_ul_ccch called for empty message");

    s_ul_ccch_msg_s s_ul_ccch_msg;
    asn1::cbit_ref bref(pdu->msg, pdu->N_bytes);
    if (s_ul_ccch_msg.unpack(bref) != asn1::SRSASN_SUCCESS)
    {
      log_rx_pdu_fail(ue.rnti, srb_to_lcid(lte_srb::srb0), *pdu, "Failed to unpack UL-CCCH message");
      return;
    }
    // tianxu********************************************
    rrc_con_req_r1_ie_s *rrc_con_req = &s_ul_ccch_msg.msg.rrc_con_req().rrc_con_req_r1;
    std::cout << "ue-Identity" << std::endl;

    // Log Rx message
    log_rrc_message(
        Rx, ue.rnti, srsran::srb_to_lcid(lte_srb::srb0), *pdu, s_ul_ccch_msg, s_ul_ccch_msg.msg.type().to_string());

    if (p_pcap_net)
    {
      p_pcap_net->write_ul_rrc_pdu(pdu->msg, pdu->N_bytes, CY_LOGICCHANNEL_TYPE_UL_CCCH, CY_NET_MODE_RAN);
    }

    switch (s_ul_ccch_msg.msg.type().value)
    {
    case s_ul_ccch_msg_type_c::types::rrc_con_req:
      std::cout << " case s_ul_ccch_msg_type_c::types::rrc_con_req:" << std::endl;
      ue.save_ul_message_s(std::move(pdu));
      // ue.handle_rrc_con_req_s(&s_ul_ccch_msg.msg.rrc_con_req());
      ue.handle_rrc_con_req_s(&s_ul_ccch_msg.msg.rrc_con_req());
      break;
    case s_ul_ccch_msg_type_c::types::rrc_con_reest_req:
    std::cout << " case s_ul_ccch_msg_type_c::types::rrc_con_reest_req:" << std::endl;
      ue.save_ul_message_s(std::move(pdu));
      ue.s_handle_rrc_con_reest_req(&s_ul_ccch_msg.msg.rrc_con_reest_req());
      break;
    default:
      logger.error("Processing UL-CCCH for rnti=0x%x - Unsupported message type %s",
                   s_ul_ccch_msg.msg.type().to_string());
      break;
    }
  }

  void rrc::parse_ul_dcch_s(ue &ue, uint32_t lcid, srsran::unique_byte_buffer_t pdu)
  {
    srsran_assert(pdu != nullptr, "parse_ul_dcch called for empty message");
    ue.parse_ul_dcch_s(lcid, std::move(pdu));
  }
  //------------------------------------------------------------------------------------------------------------------------------------------------------------
  //--------------------------------------------------IOT-2023/9/13---------------------------------------------------------------------------------------------
  void rrc::iot_parse_ul_ccch_s(ue &ue, srsran::unique_byte_buffer_t pdu)
  {
    srsran_assert(pdu != nullptr, "parse_ul_ccch called for empty message");

    iot_ul_ccch_msg_s iot_ul_ccch_msg;
    asn1::cbit_ref bref(pdu->msg, pdu->N_bytes);
    if (iot_ul_ccch_msg.unpack(bref) != asn1::SRSASN_SUCCESS)
    {
      log_rx_pdu_fail(ue.rnti, srb_to_lcid(lte_srb::srb0), *pdu, "Failed to unpack IOT-UL-CCCH message");
      return;
    }
    // Log Rx message
    log_rrc_message(
        Rx, ue.rnti, srsran::srb_to_lcid(lte_srb::srb0), *pdu, iot_ul_ccch_msg, iot_ul_ccch_msg.msg.type().to_string());
    switch (iot_ul_ccch_msg.msg.type().value)
    {
    case iot_ul_ccch_msg_type_c::types::iot_rrc_setup_req:
      srsran::console("当前字节�?=%u\n", pdu->N_bytes); // 打印解析的数�?-----标志上行连接建立请求消息解析完成
      for (uint8_t i = 0; i < pdu->N_bytes; i++)
      {
        srsran::console("%x\n", *(pdu->msg + i));
      }

      ue.iot_save_ul_message_s(std::move(pdu));
      ue.iot_handle_rrc_con_req_s(&iot_ul_ccch_msg.msg.iot_rrc_setup_req());
      break;
    default:
      logger.error("Processing IOT-UL-CCCH for rnti=0x%x - Unsupported message type %s",
                   iot_ul_ccch_msg.msg.type().to_string());
      break;
    }
  }
  void rrc::iot_parse_ul_dcch_s(ue &ue, uint32_t lcid, srsran::unique_byte_buffer_t pdu)
  {
    srsran_assert(pdu != nullptr, "parse_ul_dcch called for empty message");
    ue.iot_parse_ul_dcch_s(lcid, std::move(pdu));
  }
  //------------------------------------------------------------------------------------------------------------------------------------------------------------

  void rrc::parse_ul_ccch(ue &ue, srsran::unique_byte_buffer_t pdu)
  {
    srsran_assert(pdu != nullptr, "parse_ul_ccch called for empty message");

    ul_ccch_msg_s ul_ccch_msg;
    asn1::cbit_ref bref(pdu->msg, pdu->N_bytes);
    if (ul_ccch_msg.unpack(bref) != asn1::SRSASN_SUCCESS or
        ul_ccch_msg.msg.type().value != ul_ccch_msg_type_c::types_opts::c1)
    {
      log_rx_pdu_fail(ue.rnti, srb_to_lcid(lte_srb::srb0), *pdu, "Failed to unpack UL-CCCH message");
      return;
    }

    // Log Rx message
    log_rrc_message(
        Rx, ue.rnti, srsran::srb_to_lcid(lte_srb::srb0), *pdu, ul_ccch_msg, ul_ccch_msg.msg.c1().type().to_string());

    switch (ul_ccch_msg.msg.c1().type().value)
    {
    case ul_ccch_msg_type_c::c1_c_::types::rrc_conn_request:
      ue.save_ul_message(std::move(pdu));
      ue.handle_rrc_con_req(&ul_ccch_msg.msg.c1().rrc_conn_request());
      break;
    case ul_ccch_msg_type_c::c1_c_::types::rrc_conn_reest_request:
      ue.save_ul_message(std::move(pdu));
      ue.handle_rrc_con_reest_req(&ul_ccch_msg.msg.c1().rrc_conn_reest_request());
      break;
    default:
      logger.error("Processing UL-CCCH for rnti=0x%x - Unsupported message type %s",
                   ul_ccch_msg.msg.c1().type().to_string());
      break;
    }
  }

  ///< User mutex must be hold by caller
  void rrc::parse_ul_dcch(ue &ue, uint32_t lcid, srsran::unique_byte_buffer_t pdu)
  {
    srsran_assert(pdu != nullptr, "parse_ul_dcch called for empty message");

    ue.parse_ul_dcch(lcid, std::move(pdu));
  }

  ///< User mutex must be hold by caller
  void rrc::process_release_complete(uint16_t rnti)
  {
    logger.info("Received Release Complete rnti=0x%x", rnti);
    auto user_it = users.find(rnti);
    if (user_it == users.end())
    {
      logger.error("Received ReleaseComplete for unknown rnti=0x%x", rnti);
      return;
    }
    ue *u = user_it->second.get();

    if (u->is_idle() or u->mobility_handler->is_ho_running())
    {
      rem_user_thread(rnti);
    }
    else if (not u->is_idle())
    {
      rlc->clear_buffer(rnti);
      user_it->second->send_connection_release();
      // delay user deletion for ~50 TTI (until RRC release is sent)
      task_sched.defer_callback(50, [this, rnti]()
                                { rem_user_thread(rnti); });
    }
  }

  void rrc::send_second_release()
  {
    // process_release_complete_s(70);
    ttcn_control_release_paging(70);
  }

  void rrc::ttcn_handle_in_release_paging()
  {
    printf(" TTCN Test ");
    if (rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 71)
    {
      rrc_adp->udp_.TC_71_is_paging_connection = true;
    }
    else if (rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 75)
    {
      rrc_adp->udp_.TC_75_is_paging_refuse = true;
    }
    else if (rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 79)
    {
      rrc_adp->udp_.TC_79_is_paging_smc = true;
    }
    else if (rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 72)
    {
      rrc_adp->udp_.TC_72_con_capability = true;
    }
    else if (rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 73)
    {
      rrc_adp->udp_.TC_73_t300_timeout = true;
    }
    else if (rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 74)
    {
      rrc_adp->udp_.TC_74_t302_timeout = true;
    }
    else if (rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 711)
    {
      rrc_adp->udp_.TC_711_reconfig = true;
    }
    else if (rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 712)
    {
      rrc_adp->udp_.TC_712_reconfig_update = true;
    }
    else if (rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 517)
    {
      rrc_adp->udp_.TC_517_paging_success = true;
    }
    else if (rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 514)
    {
      rrc_adp->udp_.TC_514_Release = true;
    }
    else if (rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 515)
    {
      rrc_adp->udp_.TC_515_Release = true;
    }

    else if (rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 611)
    {
      rrc_adp->udp_.mac_con_req_flag = true;
    }
    else if (rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 613)
    {
      rrc_adp->udp_.mac_con_req_flag2 = true;
    }
    else if (rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 614)
    {
      rrc_adp->udp_.TC_614_backoff = true;
    }
    else if (rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 615)
    {
      rrc_adp->udp_.TC_615_mac_roid_not_match = true;
    }
    else if (rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 612)
    {
      rrc_adp->udp_.TC_612_mac_crid_not_match = true;
    }
    else if (rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 616)
    {
      rrc_adp->udp_.TC_616_mac_ccch_logic_channel = true;
    }
    else if (rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 710)
    {
      rrc_adp->udp_.TC_710_security_mode_failure = true;
    }
    else if (rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 516)
    {
      rrc_adp->udp_.TC_516_is_paging_invalid = true;
    }
    else if (rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 713)
    {
      rrc_adp->udp_.TC_713_reconfig_DRB = true;
    }
    else if (rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 714)
    {
      rrc_adp->udp_.TC_714_DRB_Release = true;
    }
    else if (rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 715)
    {
      rrc_adp->udp_.TC_715_reest_reconf= true;
    }
  }

  void rrc::ttcn_control_release_paging(uint16_t rnti)
  {
    // //-----------722
    // if (rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 722)
    // {
    //   rrc_adp->udp_.TC_722_ho_success=true;
    //   std::cout<<"stack_adp->udp_.TC_722_ho_success:"<<rrc_adp->udp_.TC_722_ho_success<<std::endl;
    //   //rrctorrc_ue(70);
    //   wx_Mcontrol_Notify_Switch(70, 0);
    //   return;
    // }
    //------------------
    ttcn_handle_in_release_paging();
    logger.info("Received Release Complete rnti=0x%x", rnti);
    auto user_it = users.find(rnti);
    if (user_it == users.end())
    {
      logger.error("Received ReleaseComplete for unknown rnti=0x%x", rnti);
      return;
    }
    ue *u = user_it->second.get();
    user_it->second->TTCN_connection_release();

    rrc_adp->udp_.TC_725_release_num++;
    if (rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 725 && rrc_adp->udp_.TC_725_release_num == 1)
    {
      rrc_adp->udp_.TC_725_release_nodirection = true;
    }

    std::cout << "rrc_adp->udp_.is_paging_refuse:" << rrc_adp->udp_.TC_75_is_paging_refuse << std::endl;
    std::cout << "cfg.ttcn_test_enble:" << cfg.ttcn_test_enble << std::endl;
    std::cout << "cfg.ttcn_rrc_enble:" << cfg.ttcn_rrc_enble << std::endl;
    srsran::unique_byte_buffer_t pag_pdu = srsran::make_byte_buffer();

    std::cout << "rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId:" << rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId << std::endl;

    if (rrc_adp->udp_.TC_514_Release)
    {
      // rrc_adp->udp_.TC_514_Release=false;
      std::cout << "send rrc TC_514_Release kongbao to TTCN" << std::endl;
      srsran::unique_byte_buffer_t rerurn_info_send = srsran::make_byte_buffer();
      rerurn_info_send->init();
      rerurn_info_send->msg[0] = 0x01;
      rerurn_info_send->msg[1] = 0x01;
      rerurn_info_send->msg[2] = 0x01;
      rerurn_info_send->msg[3] = 0x01;
      rerurn_info_send->msg[4] = 0x01;
      rerurn_info_send->msg[5] = 0x01;
      rerurn_info_send->N_bytes = 6;
      rrc_adp->udp_.send_ttcn_info.try_push(std::move(rerurn_info_send));
    }
    if (rrc_adp->udp_.TC_515_Release)
    {
      // rrc_adp->udp_.TC_515_Release=false;
      std::cout << "send rrc TC_515_Release kongbao to TTCN" << std::endl;
      srsran::unique_byte_buffer_t rerurn_info_send = srsran::make_byte_buffer();
      rerurn_info_send->init();
      rerurn_info_send->msg[0] = 0x01;
      rerurn_info_send->msg[1] = 0x01;
      rerurn_info_send->msg[2] = 0x01;
      rerurn_info_send->msg[3] = 0x01;
      rerurn_info_send->msg[4] = 0x01;
      rerurn_info_send->msg[5] = 0x01;
      rerurn_info_send->N_bytes = 6;
      rrc_adp->udp_.send_ttcn_info.try_push(std::move(rerurn_info_send));
    }

    if (cfg.ttcn_test_enble && cfg.ttcn_rrc_enble && (rrc_adp->udp_.TC_75_is_paging_refuse || rrc_adp->udp_.TC_71_is_paging_connection || rrc_adp->udp_.TC_79_is_paging_smc || rrc_adp->udp_.TC_73_t300_timeout || rrc_adp->udp_.TC_74_t302_timeout || rrc_adp->udp_.TC_711_reconfig || rrc_adp->udp_.TC_517_paging_success || rrc_adp->udp_.TC_710_security_mode_failure || rrc_adp->udp_.TC_516_is_paging_invalid || rrc_adp->udp_.TC_713_reconfig_DRB || rrc_adp->udp_.TC_714_DRB_Release|| rrc_adp->udp_.TC_715_reest_reconf))
    {
      while (true)
      {
        if (rrc_adp->udp_.rrc_receive_info.size() != 0)
        {
          srsran::unique_byte_buffer_t ttcn_paging =
              srsran::make_byte_buffer();
          std::cout << "receive rrc paging info from TTCN" << std::endl;
          ttcn_paging->init();
          rrc_adp->udp_.rrc_receive_info.try_pop(ttcn_paging);
          get_general_interface(&ttcn_paging);
          pag_pdu->msg[0] = ttcn_paging->msg[8];
          pag_pdu->msg[1] = ttcn_paging->msg[9];
          pag_pdu->msg[2] = ttcn_paging->msg[10];
          pag_pdu->msg[3] = ttcn_paging->msg[11];
          pag_pdu->msg[4] = ttcn_paging->msg[12];
          pag_pdu->msg[5] = ttcn_paging->msg[13];
          pag_pdu->N_bytes = 6;
          break;
        }
      }
    }
    else if (rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 611 || rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 613 || rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 615 ||
             rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 612 || rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 616)
    {
      // MAC Paging
      std::cout << "MAC Paging" << std::endl;
      while (true)
      {
        if (rrc_adp->udp_.mac_receive_info.size() != 0)
        {
          srsran::unique_byte_buffer_t ttcn_paging =
              srsran::make_byte_buffer();
          std::cout << "receive mac paging info from TTCN" << std::endl;
          ttcn_paging->init();
          rrc_adp->udp_.mac_receive_info.try_pop(ttcn_paging);
          get_general_interface(&ttcn_paging);
          pag_pdu->msg[0] = ttcn_paging->msg[8];
          pag_pdu->msg[1] = ttcn_paging->msg[9];
          pag_pdu->msg[2] = ttcn_paging->msg[10];
          pag_pdu->msg[3] = ttcn_paging->msg[11];
          pag_pdu->msg[4] = ttcn_paging->msg[12];
          pag_pdu->msg[5] = ttcn_paging->msg[13];
          pag_pdu->N_bytes = 6;
          break;
        }
      }
    }
    else if (rrc_adp->udp_.TC_725_release)
    {
      std::cout << "xxk ---" << std::endl;
      while (true)
      {
        if (rrc_adp->udp_.rrc_receive_info.size() != 0)
        {
          srsran::unique_byte_buffer_t ttcn_paging =
              srsran::make_byte_buffer();
          std::cout << "receive rrc paging info from TTCN" << std::endl;
          ttcn_paging->init();
          rrc_adp->udp_.rrc_receive_info.try_pop(ttcn_paging);
          get_general_interface(&ttcn_paging);
          pag_pdu->msg[0] = ttcn_paging->msg[8];
          pag_pdu->msg[1] = ttcn_paging->msg[9];
          pag_pdu->msg[2] = ttcn_paging->msg[10];
          pag_pdu->msg[3] = ttcn_paging->msg[11];
          pag_pdu->msg[4] = ttcn_paging->msg[12];
          pag_pdu->msg[5] = ttcn_paging->msg[13];
          pag_pdu->N_bytes = 6;
          rrc_adp->udp_.TC_725_con_req = true;

          break;
        }
      }
    }

    for (uint32_t i = 0; i < pag_pdu->N_bytes; i++)
    {
      printf("0x%x\n", *(pag_pdu->msg + i));
    }
    auto start_time = std::chrono::high_resolution_clock::now();
    while (true)
    {
      auto now_time = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::seconds>(now_time - start_time);
      if (duration.count() > 12)
      {
        std::cout << "12s over paging!!!" << std::endl;
        break;
      }
    }

    if (rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 72)
    {
      while (true)
      {
        if (rrc_adp->udp_.rrc_receive_info.size() != 0)
        {
          srsran::unique_byte_buffer_t ttcn_null =
              srsran::make_byte_buffer();
          std::cout << "receive ue-capabiliy  info from TTCN" << std::endl;
          ttcn_null->init();
          rrc_adp->udp_.rrc_receive_info.try_pop(ttcn_null);
          get_general_interface(&ttcn_null);

          break;
        }
      }
      std::cout << "---------no paging info------" << std::endl;
    }
    else if (rrc_adp->udp_.TC_515_Release)
    {
      rrc_adp->udp_.TC_515_Release = false;
    }
    else if (rrc_adp->udp_.TC_514_Release)
    {
      rrc_adp->udp_.TC_514_Release = false;
    }
    else
    {
      add_paging_id_wx_s(std::move(pag_pdu));
      std::cout << "------------------xiesi--zhihoujisuanquanbu-----" << std::endl;
    }
  }

  void rrc::process_release_complete_s(uint16_t rnti)
  {
    logger.info("Received Release Complete rnti=0x%x", rnti);
    auto user_it = users.find(rnti);
    if (user_it == users.end())
    {
      logger.error("Received ReleaseComplete for unknown rnti=0x%x", rnti);
      return;
    }
    ue *u = user_it->second.get();
    user_it->second->s_send_connection_release();

    srsran::unique_byte_buffer_t pag_pdu = srsran::make_byte_buffer();

    for (uint32_t i = 0; i < pag_pdu->N_bytes; i++)
    {
      printf("0x%x\n", *(pag_pdu->msg + i));
    }
    auto start_time = std::chrono::high_resolution_clock::now();

    while (true)
    {
      auto now_time = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::seconds>(now_time - start_time);
      if (duration.count() > 12)
      {
        std::cout << "12s over paging!!!" << std::endl;
        break;
      }
    }

    add_paging_id_wx_s(std::move(pag_pdu));
    std::cout << "------------------xiesi--zhihoujisuanquanbu-----" << std::endl;
  }

  void rrc::ate_control_release_complete(uint16_t rnti)
  {
    logger.info("Received Release Complete rnti=0x%x", rnti);
    auto user_it = users.find(rnti);
    if (user_it == users.end())
    {
      logger.error("Received ReleaseComplete for unknown rnti=0x%x", rnti);
      return;
    }
    ue *u = user_it->second.get();
    user_it->second->s_send_connection_release();
  }

  void rrc::ate_control_measurement_reconfig(uint16_t rnti)
  {
    logger.info("Received ate_control_measurement_reconfig rnti=0x%x", rnti);
    auto user_it = users.find(rnti);
    if (user_it == users.end())
    {
      logger.error("Received ate_control_measurement_reconfig for unknown rnti=0x%x", rnti);
      return;
    }
    ue *u = user_it->second.get();
    user_it->second->send_rrc_measure_reconf();
  }

  void rrc::rem_user(uint16_t rnti)
  {
    std::cout << "rrc rem user!!" << std::endl;
    auto user_it = users.find(rnti);
    if (user_it != users.end())
    {
      // First remove MAC and GTPU to stop processing DL/UL traffic for this user
      mac->ue_rem(rnti); // MAC handles PHY
      gtpu->rem_user(rnti);

      // Now remove RLC and PDCP
      bearer_manager.rem_user(rnti);
      rlc->rem_user(rnti);
      pdcp->rem_user(rnti);

      users.erase(rnti);

      srsran::console("Disconnecting rnti=0x%x.\n", rnti);
      logger.info("Removed user rnti=0x%x", rnti);
    }
    else
    {
      logger.error("Removing user rnti=0x%x (does not exist)", rnti);
    }
  }

  void rrc::config_mac()
  {
    using sched_cell_t = sched_interface::cell_cfg_t;

    // Fill MAC scheduler configuration for SIBs
    std::vector<sched_cell_t> sched_cfg;
    sched_cfg.resize(cfg.cell_list.size());

    for (uint32_t ccidx = 0; ccidx < cfg.cell_list.size(); ++ccidx)
    {
      sched_interface::cell_cfg_t &item = sched_cfg[ccidx];

      // set sib/prach cfg
      for (uint32_t i = 0; i < nof_si_messages; i++)
      {
        item.sibs[i].len = cell_common_list->get_cc_idx(ccidx)->sib_buffer.at(i)->N_bytes;
        if (i == 0)
        {
          item.sibs[i].period_rf = 8; // SIB1 is always 8 rf
        }
        else
        {
          item.sibs[i].period_rf = cfg.sib1.sched_info_list[i - 1].si_periodicity.to_number();
        }
      }
      item.prach_config = cfg.sibs[1].sib2().rr_cfg_common.prach_cfg.prach_cfg_info.prach_cfg_idx;
      item.prach_nof_preambles = cfg.sibs[1].sib2().rr_cfg_common.rach_cfg_common.preamb_info.nof_ra_preambs.to_number();
      item.si_window_ms = cfg.sib1.si_win_len.to_number();
      item.prach_rar_window =
          cfg.sibs[1].sib2().rr_cfg_common.rach_cfg_common.ra_supervision_info.ra_resp_win_size.to_number();
      item.prach_freq_offset = cfg.sibs[1].sib2().rr_cfg_common.prach_cfg.prach_cfg_info.prach_freq_offset;
      item.maxharq_msg3tx = cfg.sibs[1].sib2().rr_cfg_common.rach_cfg_common.max_harq_msg3_tx;
      item.enable_64qam = cfg.sibs[1].sib2().rr_cfg_common.pusch_cfg_common.pusch_cfg_basic.enable64_qam;
      item.target_pucch_ul_sinr = cfg.cell_list[ccidx].target_pucch_sinr_db;
      item.target_pusch_ul_sinr = cfg.cell_list[ccidx].target_pusch_sinr_db;
      item.enable_phr_handling = cfg.cell_list[ccidx].enable_phr_handling;
      item.min_phr_thres = cfg.cell_list[ccidx].min_phr_thres;
      item.delta_pucch_shift = cfg.sibs[1].sib2().rr_cfg_common.pucch_cfg_common.delta_pucch_shift.to_number();
      item.ncs_an = cfg.sibs[1].sib2().rr_cfg_common.pucch_cfg_common.ncs_an;
      item.n1pucch_an = cfg.sibs[1].sib2().rr_cfg_common.pucch_cfg_common.n1_pucch_an;
      item.nrb_cqi = cfg.sibs[1].sib2().rr_cfg_common.pucch_cfg_common.nrb_cqi;

      item.nrb_pucch = SRSRAN_MAX(cfg.sr_cfg.nof_prb, item.nrb_cqi);
      logger.info("Allocating %d PRBs for PUCCH", item.nrb_pucch);

      // Copy base cell configuration
      item.cell = cfg.cell;
      item.cell.id = cfg.cell_list[ccidx].pci;

      // copy secondary cell list info
      sched_cfg[ccidx].scell_list.reserve(cfg.cell_list[ccidx].scell_list.size());
      for (uint32_t scidx = 0; scidx < cfg.cell_list[ccidx].scell_list.size(); ++scidx)
      {
        const auto &scellitem = cfg.cell_list[ccidx].scell_list[scidx];
        // search enb_cc_idx specific to cell_id
        auto it = std::find_if(cfg.cell_list.begin(), cfg.cell_list.end(), [&scellitem](const cell_cfg_t &e)
                               { return e.cell_id == scellitem.cell_id; });
        if (it == cfg.cell_list.end())
        {
          logger.warning("Secondary cell 0x%x not configured", scellitem.cell_id);
          continue;
        }
        sched_interface::cell_cfg_t::scell_cfg_t scellcfg;
        scellcfg.enb_cc_idx = it - cfg.cell_list.begin();
        scellcfg.ul_allowed = scellitem.ul_allowed;
        scellcfg.cross_carrier_scheduling = scellitem.cross_carrier_sched;
        sched_cfg[ccidx].scell_list.push_back(scellcfg);
      }
    }

    // Configure MAC scheduler
    mac->cell_cfg(sched_cfg);
  }

  /******************************************************
             Here is MIB and SIB generation
  ******************************************************/
  /******************************************************
             Here is MIB and SIB generation
  ******************************************************/
  /*void rrc::configure_mib_wx_ttcn(srsran::unique_byte_buffer_t pdu_)
  {
    // Store configs,MIB in common cell ctxt list
    enb_cell_common *cell_ctxt = cell_common_list->get_cc_idx(0);
    // generate and pack into MIB buffers
    std::cout << "--------@@@@@@@------image------2" << std::endl;
    for (uint32_t i = 0; i < pdu_->N_bytes; i++)
    {
      printf("0x:%x ", *(pdu_->msg + i));
    }
    // Replace it with something passed in the interface

    cell_ctxt->mib_buffer.push_back(std::move(pdu_));
    srsran::console("当前字节�?=%u\n", cell_ctxt->mib_buffer[0]->N_bytes);
    for (uint8_t i = 0; i < cell_ctxt->mib_buffer[0]->N_bytes; i++)
    {
      srsran::console("%x\n", *(cell_ctxt->mib_buffer[0]->msg + i));
    }
    si_info.mibLen = cell_ctxt->mib_buffer[0]->N_bytes;
    si_info.mib_msg = cell_ctxt->mib_buffer[0]->msg;
    std::cout << "  si_info.mib_msg address1:" << &si_info.mib_msg << std::endl;
    std::cout << "--------@@@@@@@----si_info" << std::endl;
    for (int i = 0; i < si_info.mibLen; i++)
    {
      printf("0x:%x ", *(si_info.mib_msg + i));
    }
  }

  void rrc::generate_sib_wx_ttcn(srsran::unique_byte_buffer_t pdu_)
  {
    // generate and pack into SIB buffers
    enb_cell_common *cell_ctxt = cell_common_list->get_cc_idx(0);
    bcch_sbch_msg_s msg;

    std::string mcc_str = "460";
    std::string mnc_str = "00";

    msg.msg.beam_acc_rel_info.plmn_id_li.resize(1);
    srsran::plmn_id_t plmn;
    if (plmn.from_string(mcc_str + mnc_str) == SRSRAN_ERROR)
    {
      ERROR("Could not convert %s to a plmn_id", (mcc_str + mnc_str).c_str());
    }
    srsran::to_asn1(&msg.msg.beam_acc_rel_info.plmn_id_li[0], plmn);

std::cout << "------plmn_id_li----"
            << msg.msg.beam_acc_rel_info.plmn_id_li[0].mcc_present << std::endl;

    msg.msg.beam_acc_rel_info.beam_barred = static_cast<
        asn1::rrc::sib_wx_s::beam_acc_rel_info_s::beam_barred_opts::options>(
        ((pdu_->msg[0] & 0x80) >> 7)); //[0]1
    std::cout << " msg.msg.beam_acc_rel_info.beam_barred:     "
              << msg.msg.beam_acc_rel_info.beam_barred.to_string() << std::endl;

    msg.msg.beam_sel_rel_info.q_rx_lev_min.q_min =
        ((pdu_->msg[0] & 0x7c) >> 2) - 70; // 璐熸暟-70  [0]5
    printf(" msg.msg.beam_sel_rel_info.q_rx_lev_min.q_min:    %d\n",
           msg.msg.beam_sel_rel_info.q_rx_lev_min.q_min);

    msg.msg.beam_resel_rel_info.threshold_resel.set_rssi_normal() =
        ((pdu_->msg[0] & 0x03) << 4) | ((pdu_->msg[1] & 0xf0) >> 4); //[0]2 [1]4
    printf("msg.msg.beam_resel_rel_info.t_resel:   %d\n",
           msg.msg.beam_resel_rel_info.threshold_resel.set_rssi_normal());

    msg.msg.beam_resel_rel_info.q_rx_lev_min.q_min =
        (((pdu_->msg[1] & 0x0f) << 1) | ((pdu_->msg[2] & 0x80) >> 7)) -
        70; // 璐熸暟 -70 [1]4 [2]1
    printf(" msg.msg.beam_resel_rel_info.q_rx_lev_min.q_min:   %d\n",
           msg.msg.beam_resel_rel_info.q_rx_lev_min.q_min);

    msg.msg.beam_resel_rel_info.offset = (pdu_->msg[2] & 0x7c)-15; //[2]5
    printf("(pdu_->msg[2] & 0x7c):   %d\n",
           (pdu_->msg[2] & 0x7c));
    printf("msg.msg.beam_resel_rel_info.offset:   %d\n",
           msg.msg.beam_resel_rel_info.offset);

    msg.msg.beam_resel_rel_info.hysteresis =
        ((pdu_->msg[2] & 0x03) << 1) | ((pdu_->msg[3] & 0x80) >> 7); //[2]2 [3]1
    printf("msg.msg.beam_resel_rel_info.hysteresis:   %d\n",
           msg.msg.beam_resel_rel_info.hysteresis);

    msg.msg.beam_resel_rel_info.t_resel = (pdu_->msg[3] & 0x70) >> 4; //[3]3

    printf("msg.msg.beam_resel_rel_info.t_resel:   %d\n",
           msg.msg.beam_resel_rel_info.t_resel);

    cfg_li_nom_s &cfg_li_nom =
        msg.msg.beam_resel_rel_info.meas_gap_cfg_wx.set_cfg_li_nom();
    cfg_li_nom.resize(1);

    cfg_li_nom[0].beam_index = pdu_->msg[3] & 0x0f; //[3]4

    printf("cfg_li_nom[0].beam_index:   %d\n", cfg_li_nom[0].beam_index);

    cfg_li_nom[0].frame_offset = (pdu_->msg[4] & 0xfc) >> 2; //[4]6

    printf("cfg_li_nom[0].frame_offset:   %d\n", cfg_li_nom[0].frame_offset);

    cfg_li_nom[0].fcch_band_id.ba_id.from_number(
        ((pdu_->msg[4] & 0x03) << 4) | ((pdu_->msg[5] & 0xf0) >> 4)); //[4]2 [5]4
    std::cout << " cfg_li_nom[0].fcch_band_id.ba_id:"
              << cfg_li_nom[0].fcch_band_id.ba_id.to_string() << std::endl;

    cfg_li_nom[0].fcch_freq_id.freq_id.from_number((pdu_->msg[5] & 0x0c) >>
                                                   2); //[5]2
    std::cout << " cfg_li_nom[0].fcch_freq_id.freq_id:"
              << cfg_li_nom[0].fcch_freq_id.freq_id.to_string() << std::endl;

    cfg_li_nom[0].meas_nom_ratio =
        static_cast<asn1::rrc::cfg_nom_s::meas_nom_ratio_opts::options>(
            ((pdu_->msg[5] & 0x03) << 2) |
            ((pdu_->msg[6] & 0xc0) >> 6)); // [5]2 [6]2
    printf("cfg_li_nom[0].meas_nom_ratio:   %d\n",
           cfg_li_nom[0].meas_nom_ratio.to_number());
    std::cout << " cfg_li_nom[0].meas_nom_ratio:   "
              << cfg_li_nom[0].meas_nom_ratio.to_string() << std::endl;

    printf(" msg.msg.uac_barring_info_present:  %d\n",
           (pdu_->msg[6] & 0x20) >> 5);
    if ((pdu_->msg[6] & 0x20) >> 5)
    { //[6]1   msg.msg.uac_barring_info_present
      std::cout << "enter 1?" << std::endl;
      msg.msg.uac_barring_info_present = true;

      if ((pdu_->msg[6] & 0x10) >> 4)
      { //[6]1 uac_barr_for_common_present
        std::cout << "enter 2?" << std::endl;
        msg.msg.uac_barring_info.uac_barr_for_common_present = true;
        msg.msg.uac_barring_info.uac_barr_for_common.resize(1);

        msg.msg.uac_barring_info.uac_barr_for_common[0].access_category =
            (pdu_->msg[6] & 0x0e) >> 1; //[6]3
        printf(
            "msg.msg.uac_barring_info.uac_barr_for_common[0].access_category:   "
            "%d\n",
            msg.msg.uac_barring_info.uac_barr_for_common[0].access_category);

        msg.msg.uac_barring_info.uac_barr_for_common[0].uac_barr_info_set_idx =
            ((pdu_->msg[6] & 0x01) << 2) |
            ((pdu_->msg[7] & 0xc0) >> 6); //[6]1 [7]2
        printf(
            "msg.msg.uac_barring_info.uac_barr_for_common[0].uac_barr_info_set_"
            "idx:   "
            "%d\n",
            msg.msg.uac_barring_info.uac_barr_for_common[0]
                .uac_barr_info_set_idx);
      }

      msg.msg.uac_barring_info.uac_barr_info_set_list.resize(1);
      msg.msg.uac_barring_info.uac_barr_info_set_list[0].uac_barr_factor =
          static_cast<
              asn1::rrc::uac_barr_info_set_s::uac_barr_factor_opts::options>(
              (pdu_->msg[7] & 0x38) >> 3); // [7]3
      std::cout << "msg.msg.uac_barring_info.uac_barr_info_set_list[0].uac_"
                   "barr_factor:    "
                << msg.msg.uac_barring_info.uac_barr_info_set_list[0]
                       .uac_barr_factor.to_string()
                << std::endl;

      msg.msg.uac_barring_info.uac_barr_info_set_list[0].uac_barr_time =
          static_cast<
              asn1::rrc::uac_barr_info_set_s::uac_barr_time_opts::options>(
              pdu_->msg[7] & 0x07); //[7]3
      std::cout << "msg.msg.uac_barring_info.uac_barr_info_set_list[0].uac_"
                   "barr_time:  "
                << msg.msg.uac_barring_info.uac_barr_info_set_list[0]
                       .uac_barr_time.to_string()
                << std::endl;

      msg.msg.uac_barring_info.uac_barr_info_set_list[0]
          .uac_barr_for_access_id.from_number((pdu_->msg[8] & 0xfe) >>
                                              1); // [8]7
      std::cout << " msg.msg.uac_barring_info.uac_barr_info_set_list[0]"
                   ".uac_barr_for_access_id : "
                << msg.msg.uac_barring_info.uac_barr_info_set_list[0]
                       .uac_barr_for_access_id.to_string()
                << std::endl;
    }

    uint8_t rach_cfg_ba_id_number =
        ((pdu_->msg[8] & 0x01) << 5) | ((pdu_->msg[9] & 0xf8) >> 3); //[8]1 [9]5
    msg.msg.rr_cfg_com.rach_cfg_com.band_id.ba_id.from_number(
        rach_cfg_ba_id_number);
    std::cout << "msg.msg.rr_cfg_com.rach_cfg_com.band_id.ba_id:   "
              << msg.msg.rr_cfg_com.rach_cfg_com.band_id.ba_id.to_string()
              << std::endl;

    msg.msg.rr_cfg_com.rach_cfg_com.freq_bit_map.from_number(
        ((pdu_->msg[9] & 0x07) << 1) |
        ((pdu_->msg[10] & 0x80) >> 7)); // [9]3 [10]1
    std::cout << " msg.msg.rr_cfg_com.rach_cfg_com.freq_bit_map:   "
              << msg.msg.rr_cfg_com.rach_cfg_com.freq_bit_map.to_string()
              << std::endl;

    msg.msg.rr_cfg_com.rach_cfg_com.rach_frame_ass.from_number(
        (pdu_->msg[10] & 0x78) >> 3); //[10]4
    std::cout << "msg.msg.rr_cfg_com.rach_cfg_com.rach_frame_ass:   "
              << msg.msg.rr_cfg_com.rach_cfg_com.rach_frame_ass.to_string()
              << std::endl;

    msg.msg.rr_cfg_com.rach_cfg_com.rach_slot_ass =
        static_cast<asn1::rrc::rach_cfg_com_n_s::rach_slot_ass_opts::options>(
            (pdu_->msg[10] & 0x06) >> 1); // [10]2
    std::cout << " msg.msg.rr_cfg_com.rach_cfg_com.rach_slot_ass:   "
              << msg.msg.rr_cfg_com.rach_cfg_com.rach_slot_ass.to_string()
              << std::endl;

    msg.msg.rr_cfg_com.rach_cfg_com.ra_res_wi_si =
        static_cast<asn1::rrc::rach_cfg_com_n_s::ra_res_wi_si_opts::options>(
            ((pdu_->msg[10] & 0x01) << 1) |
            ((pdu_->msg[11] & 0x80) >> 1)); // [10]1 [11]1
    std::cout << " msg.msg.rr_cfg_com.rach_cfg_com.ra_res_wi_si:   "
              << msg.msg.rr_cfg_com.rach_cfg_com.ra_res_wi_si.to_string()
              << std::endl;

    printf("rr_cfg_com.agch_cfg_com.band_id_present:  %d\n",
           (pdu_->msg[11] & 0x40) >> 6);
    if ((pdu_->msg[11] & 0x40) >> 6)
    { //[11]1
      msg.msg.rr_cfg_com.agch_cfg_com.band_id_present = true;
      msg.msg.rr_cfg_com.agch_cfg_com.band_id.ba_id.from_number(pdu_->msg[11] &
                                                                0x3f); //[11]6
      std::cout << "msg.msg.rr_cfg_com.agch_cfg_com.band_id.ba_id:   "
                << msg.msg.rr_cfg_com.agch_cfg_com.band_id.ba_id.to_string()
                << std::endl;
    }

    printf("rr_cfg_com.agch_cfg_com.freq_id_present:  %d\n",
           (pdu_->msg[12] & 0x80) >> 7);
    if ((pdu_->msg[12] & 0x80) >> 7) //[12]1
    {
      msg.msg.rr_cfg_com.agch_cfg_com.freq_id_present = true;
      msg.msg.rr_cfg_com.agch_cfg_com.freq_id.freq_id.from_number(
          (pdu_->msg[12] & 0x60) >> 5); //[12]2
      std::cout << "msg.msg.rr_cfg_com.agch_cfg_com.freq_id.freq_id:   "
                << msg.msg.rr_cfg_com.agch_cfg_com.freq_id.freq_id.to_string()
                << std::endl;
    }

    msg.msg.rr_cfg_com.agch_cfg_com.agch_fram_ass.from_number(
        (pdu_->msg[12] & 0x1e) >> 1); //[12]4
    std::cout << "msg.msg.rr_cfg_com.agch_cfg_com.agch_fram_ass:   "
              << msg.msg.rr_cfg_com.agch_cfg_com.agch_fram_ass.to_string()
              << std::endl;

    printf("rr_cfg_com.agch_cfg_com.agch_slot_start_present:  %d\n",
           pdu_->msg[12] & 0x01);
    if (pdu_->msg[12] & 0x01)
    { //[12]1
      msg.msg.rr_cfg_com.agch_cfg_com.agch_slot_start_present = true;
      msg.msg.rr_cfg_com.agch_cfg_com.agch_slot_start =
          static_cast<asn1::rrc::agch_cfg_com_s::agch_slot_start_opts::options>(
              (pdu_->msg[13] & 0xc0) >> 6); //[13]2
      std::cout << "msg.msg.rr_cfg_com.agch_cfg_com.agch_slot_start:   "
                << msg.msg.rr_cfg_com.agch_cfg_com.agch_slot_start.to_string()
                << std::endl;
    }

    msg.msg.rr_cfg_com.power_ctrl_cfg_com.pmbch_tx_p.from_number(
        ((pdu_->msg[13] & 0x3f) << 1) |
        ((pdu_->msg[14] & 0x80) >> 7)); //[13]6 [14]1
    std::cout << "msg.msg.rr_cfg_com.power_ctrl_cfg_com.pmbch_tx_p : "
              << msg.msg.rr_cfg_com.power_ctrl_cfg_com.pmbch_tx_p.to_string()
              << std::endl;

    // PowerControl-ConfigCommon
    if ((pdu_->msg[14] & 0x40) >> 6)
    { //[14]1
      msg.msg.rr_cfg_com.power_ctrl_cfg_com.exp_rx_p_prach_present = true;
      msg.msg.rr_cfg_com.power_ctrl_cfg_com.exp_rx_p_prach.set_rssi_normal() = 63;
    }
    if ((pdu_->msg[14] & 0x20) >> 5)
    { //[14]1
      msg.msg.rr_cfg_com.power_ctrl_cfg_com.exp_rx_p_psych_present = true;
      msg.msg.rr_cfg_com.power_ctrl_cfg_com.exp_rx_p_psych.set_rssi_normal() = 62;
    }
    if ((pdu_->msg[14] & 0x10) >> 4)
    { //[14]1
      msg.msg.rr_cfg_com.power_ctrl_cfg_com.exp_rx_p_pdch1_1_present = true;
      msg.msg.rr_cfg_com.power_ctrl_cfg_com.exp_rx_p_pdch1_1.set_rssi_normal() =
          60;
    }
    if ((pdu_->msg[14] & 0x08) >> 3)
    { //[14]1
      msg.msg.rr_cfg_com.power_ctrl_cfg_com.exp_rx_p_pdch1_2_present = true;
      msg.msg.rr_cfg_com.power_ctrl_cfg_com.exp_rx_p_pdch1_2.set_rssi_normal() =
          59;
    }
    if ((pdu_->msg[14] & 0x04) >> 2)
    { //[14]1
      msg.msg.rr_cfg_com.power_ctrl_cfg_com.exp_rx_p_psch1_1_present = true;
      msg.msg.rr_cfg_com.power_ctrl_cfg_com.exp_rx_p_psch1_1.set_rssi_normal() =
          58;
    }
    if ((pdu_->msg[14] & 0x02) >> 1)
    { //[14]1
      msg.msg.rr_cfg_com.power_ctrl_cfg_com.exp_rx_p_psch1_2_present = true;
      msg.msg.rr_cfg_com.power_ctrl_cfg_com.exp_rx_p_psch1_2.set_rssi_normal() =
          57;
    }
    if (pdu_->msg[14] & 0x01)
    { //[14]1
      msg.msg.rr_cfg_com.power_ctrl_cfg_com.exp_rx_p_psch5_1_present = true;
      msg.msg.rr_cfg_com.power_ctrl_cfg_com.exp_rx_p_psch5_1.set_rssi_normal() =
          56;
    }
    if ((pdu_->msg[15] & 0x80) >> 7)
    { //[15]1
      msg.msg.rr_cfg_com.power_ctrl_cfg_com.exp_rx_p_psch5_2_present = true;
      msg.msg.rr_cfg_com.power_ctrl_cfg_com.exp_rx_p_psch5_2.set_rssi_normal() =
          55;
    }
    if ((pdu_->msg[15] & 0x40) >> 6)
    { //[15]1
      msg.msg.rr_cfg_com.power_ctrl_cfg_com.exp_rx_p_ptuch_present = true;
      msg.msg.rr_cfg_com.power_ctrl_cfg_com.exp_rx_p_ptuch.set_rssi_normal() = 54;
    }

    printf("rr_cfg_com.bbch_cfg_com_present:  %d\n", (pdu_->msg[15] & 0x20) >> 5);
    if ((pdu_->msg[15] & 0x20) >> 5) // false   //[15]1
    {
      msg.msg.rr_cfg_com.bbch_cfg_com_present = true;
      msg.msg.rr_cfg_com.bbch_cfg_com.band_id.ba_id.from_number(
          ((pdu_->msg[15] & 0x1f) << 1) |
          ((pdu_->msg[16] & 0x80) >> 7)); //[15]5 [16]1
      std::cout << " msg.msg.rr_cfg_com.bbch_cfg_com.band_id.ba_id:   "
                << msg.msg.rr_cfg_com.bbch_cfg_com.band_id.ba_id.to_string()
                << std::endl;

      msg.msg.rr_cfg_com.bbch_cfg_com.freq_id.freq_id.from_number(
          (pdu_->msg[16] & 0x60) >> 5); //[16]2
      std::cout << " msg.msg.rr_cfg_com.bbch_cfg_com.freq_id.freq_id:   "
                << msg.msg.rr_cfg_com.bbch_cfg_com.freq_id.freq_id.to_string()
                << std::endl;

      msg.msg.rr_cfg_com.bbch_cfg_com.bbch_frame_ass.from_number(
          (pdu_->msg[16] & 0x1e) >> 1); //[16]4
      std::cout << " msg.msg.rr_cfg_com.bbch_cfg_com.bbch_frame_ass:   "
                << msg.msg.rr_cfg_com.bbch_cfg_com.bbch_frame_ass.to_string()
                << std::endl;

      msg.msg.rr_cfg_com.bbch_cfg_com.bbch_slot_ass =
          static_cast<asn1::rrc::bbch_cfg_com_s::bbch_slot_ass_opts::options>(
              ((pdu_->msg[16] & 0x01) << 1) |
              ((pdu_->msg[17] & 0x80) >> 7)); //[16]1 [17]1
      std::cout << "msg.msg.rr_cfg_com.bbch_cfg_com.bbch_slot_ass:   "
                << msg.msg.rr_cfg_com.bbch_cfg_com.bbch_slot_ass.to_string()
                << std::endl;
    }

    printf("rr_cfg_com.iotsi_cfg_present:  %d\n", (pdu_->msg[17] & 0x40) >> 6);
    if ((pdu_->msg[17] & 0x40) >> 6)
    { //[17]1
      msg.msg.rr_cfg_com.iotsi_cfg_present = true;

      iotsi_cfg_normal_s &iotsi_cfg_normal =
          msg.msg.rr_cfg_com.iotsi_cfg.set_iotsi_cfg_normal();

      iotsi_cfg_normal.dl_band_id.ba_id.from_number(pdu_->msg[17] &
                                                    0x3f); //[17]6
      std::cout << " iotsi_cfg_normal.dl_band_id.ba_id:   "
                << iotsi_cfg_normal.dl_band_id.ba_id.to_string() << std::endl;

      iotsi_cfg_normal.dl_freq_id.freq_id.from_number((pdu_->msg[18] & 0xc0) >>
                                                      6); //[18]2
      std::cout << "iotsi_cfg_normal.dl_freq_id.freq_id:   "
                << iotsi_cfg_normal.dl_freq_id.freq_id.to_string() << std::endl;

      iotsi_cfg_normal.fn_ass.from_number(pdu_->msg[18] & 0x3f); //[18]6
      std::cout << " iotsi_cfg_normal.fn_ass:   "
                << iotsi_cfg_normal.fn_ass.to_string() << std::endl;
    }

    msg.msg.ue_timer_and_constant.t300 =
        static_cast<asn1::rrc::ue_timer_and_constant_s::t300_opts::options>(
            (pdu_->msg[19] & 0xe0) >> 5); //[19]3
    std::cout << "  msg.msg.ue_timer_and_constant.t300:   "
              << msg.msg.ue_timer_and_constant.t300.to_string() << std::endl;

    msg.msg.ue_timer_and_constant.t301 =
        static_cast<asn1::rrc::ue_timer_and_constant_s::t301_opts::options>(
            (pdu_->msg[19] & 0x1c) >> 2); //[19]3
    std::cout << "  msg.msg.ue_timer_and_constant.t301:   "
              << msg.msg.ue_timer_and_constant.t301.to_string() << std::endl;

    msg.msg.ue_timer_and_constant.t302 =
        static_cast<asn1::rrc::ue_timer_and_constant_s::t302_opts::options>(
            ((pdu_->msg[19] & 0x03) << 1) |
            ((pdu_->msg[20] & 0x80) >> 7)); //[19]2 [20]1
    std::cout << "  msg.msg.ue_timer_and_constant.t302:   "
              << msg.msg.ue_timer_and_constant.t302.to_string() << std::endl;

    msg.msg.ue_timer_and_constant.t310 =
        static_cast<asn1::rrc::ue_timer_and_constant_s::t310_opts::options>(
            (pdu_->msg[20] & 0x70) >> 4); //[20]3
    std::cout << "  msg.msg.ue_timer_and_constant.t310:   "
              << msg.msg.ue_timer_and_constant.t310.to_string() << std::endl;

    msg.msg.ue_timer_and_constant.n310 =
        static_cast<asn1::rrc::ue_timer_and_constant_s::n310_opts::options>(
            (pdu_->msg[20] & 0x0e) >> 1); //[20]3
    std::cout << "  msg.msg.ue_timer_and_constant.n310:   "
              << msg.msg.ue_timer_and_constant.n310.to_string() << std::endl;

    msg.msg.ue_timer_and_constant.t311 =
        static_cast<asn1::rrc::ue_timer_and_constant_s::t311_opts::options>(
            ((pdu_->msg[20] & 0x01) << 2) |
            ((pdu_->msg[21] & 0xc0) >> 6)); //[20]1 [21]2
    std::cout << "  msg.msg.ue_timer_and_constant.t311:   "
              << msg.msg.ue_timer_and_constant.t311.to_string() << std::endl;

    msg.msg.ue_timer_and_constant.n311 =
        static_cast<asn1::rrc::ue_timer_and_constant_s::n311_opts::options>(
            (pdu_->msg[21] & 0x38) >> 3); //[21]3
    std::cout << "  msg.msg.ue_timer_and_constant.n311:   "
              << msg.msg.ue_timer_and_constant.n311.to_string() << std::endl;

    printf("p_max_n_present:  %d\n", (pdu_->msg[21] & 0x04) >> 2);
    if ((pdu_->msg[21] & 0x04) >> 2)
    { //[21]1
      msg.msg.p_max_n_present = true;
      printf("((pdu_->msg[21] & 0x03) << 2) :   %d\n",
             ((pdu_->msg[21] & 0x03) << 2));
      printf("((pdu_->msg[22] & 0xc0) >> 6):   %d\n",
             ((pdu_->msg[22] & 0xc0) >> 6));
      msg.msg.p_max_n = ((pdu_->msg[21] & 0x03) << 2) |
                        ((pdu_->msg[22] & 0xc0) >> 6); //[21]2 [22]2
      printf("msg.msg.p_max_n:   %d\n", msg.msg.p_max_n);
    }

    msg.msg.geo_info_update_para.update_timer = static_cast<
        asn1::rrc::geo_info_update_para_s::update_timer_opts::options>(
        (pdu_->msg[22] & 0x30) >> 4); //[22]2
    std::cout << "msg.msg.geo_info_update_para.update_timer:   "
              << msg.msg.geo_info_update_para.update_timer.to_string()
              << std::endl;

    msg.msg.geo_info_update_para.update_distance = static_cast<
        asn1::rrc::geo_info_update_para_s::update_distance_opts::options>(
        (pdu_->msg[22] & 0x0c) >> 2); //[22]2
    std::cout << "msg.msg.geo_info_update_para.update_distance:   "
              << msg.msg.geo_info_update_para.update_distance.to_string()
              << std::endl;

    //[23]
    msg.msg.eph_para_ser_sat.sate_ephem_semi_major_axis =
        (static_cast<uint32_t>(pdu_->msg[23]) << 24) |
        (static_cast<uint32_t>(pdu_->msg[24]) << 16) |
        (static_cast<uint32_t>(pdu_->msg[25]) << 8) |
        static_cast<uint32_t>(pdu_->msg[26]);
    ;
    printf("sate_ephem_semi_major_axis:  %d\n",
           msg.msg.eph_para_ser_sat.sate_ephem_semi_major_axis);

    msg.msg.eph_para_ser_sat.sate_ephem_eccen_e = pdu_->msg[27];
    printf("sate_ephem_eccen_e:  %d\n",
           msg.msg.eph_para_ser_sat.sate_ephem_eccen_e);

    msg.msg.eph_para_ser_sat.sate_ephem_argu_of_peria =
        (static_cast<uint16_t>(pdu_->msg[28]) << 8) |
        static_cast<uint16_t>(pdu_->msg[29]);
    printf("msg.msg.eph_para_ser_sat.sate_ephem_argu_of_peria:  %d\n",
           msg.msg.eph_para_ser_sat.sate_ephem_argu_of_peria);

    msg.msg.eph_para_ser_sat.sate_ephem_long_of_ascen_node =
        (static_cast<uint16_t>(pdu_->msg[30]) << 8) |
        static_cast<uint16_t>(pdu_->msg[31]);
    printf(" msg.msg.eph_para_ser_sat.sate_ephem_long_of_ascen_node:  %d\n",
           msg.msg.eph_para_ser_sat.sate_ephem_long_of_ascen_node);

    msg.msg.eph_para_ser_sat.sate_ephem_inc =
        (static_cast<uint16_t>(pdu_->msg[32]) << 8) |
        static_cast<uint16_t>(pdu_->msg[33]);
    printf("  msg.msg.eph_para_ser_sat.sate_ephem_inc :  %d\n",
           msg.msg.eph_para_ser_sat.sate_ephem_inc);

    msg.msg.eph_para_ser_sat.sate_ephem_mean_ano_m =
        (static_cast<uint16_t>(pdu_->msg[34]) << 8) |
        static_cast<uint16_t>(pdu_->msg[35]);
    printf("   msg.msg.eph_para_ser_sat.sate_ephem_mean_ano_m :  %d\n",
           msg.msg.eph_para_ser_sat.sate_ephem_mean_ano_m);

    msg.msg.eph_para_ser_sat.n_date =
        (static_cast<uint16_t>(pdu_->msg[36]) << 8) |
        static_cast<uint16_t>(pdu_->msg[37]);
    printf("  msg.msg.eph_para_ser_sat.n_date:  %d\n",
           msg.msg.eph_para_ser_sat.n_date);

    msg.msg.eph_para_ser_sat.n_time =
        (static_cast<uint32_t>(pdu_->msg[38]) << 24) |
        (static_cast<uint32_t>(pdu_->msg[39]) << 16) |
        (static_cast<uint32_t>(pdu_->msg[40]) << 8) |
        static_cast<uint32_t>(pdu_->msg[41]);
    printf("msg.msg.eph_para_ser_sat.n_time:  %d\n",
           msg.msg.eph_para_ser_sat.n_time);

    // Pack payload for SIB
    srsran::unique_byte_buffer_t sib_buffer = srsran::make_byte_buffer();
    sib_buffer->init();
    if (sib_buffer)
    {
      asn1::bit_ref bref(sib_buffer->msg, sib_buffer->get_tailroom());
      if (msg.msg.pack(bref) != asn1::SRSASN_SUCCESS)
      {
        srsran::console("---pack error---\n");
      }
      sib_buffer->N_bytes = bref.distance_bytes();
    }
    else
    {
      std::cout << "sib_buffer error" << std::endl;
    }

    cell_ctxt->sib_wx_buffer.push_back(std::move(sib_buffer));
    srsran::console("bytes num=%u\n", cell_ctxt->sib_wx_buffer[0]->N_bytes);
    for (uint8_t i = 0; i < cell_ctxt->sib_wx_buffer[0]->N_bytes; i++)
    {
      srsran::console("%x\n", *(cell_ctxt->sib_wx_buffer[0]->msg + i));
    }
    si_info.sib_msg = cell_ctxt->sib_wx_buffer[0]->msg;
    si_info.sibLen = cell_ctxt->sib_wx_buffer[0]->N_bytes;
  }*/

  //---------------------------------2024.03.22--------
  void rrc::generate_recfg_wx()
  {
    enb_cell_common *cell_ctxt = cell_common_list->get_cc_idx(0);

    for (uint32_t i = 0; i < cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg.size(); i++)
    {
      printf("phy_chan_list_cfg[%d] band_id =%ld\n", i, cfg.recfg_wx.msg.rrc_con_recfg().rrc_con_recfg_r1.redio_resour_cfg_dedi.phy_chan_list_cfg[i].band_id.ba_id.to_number());
    }

    s_dl_dcch_msg_s msg = cfg.recfg_wx;
    srsran::unique_byte_buffer_t recfg_buffer = srsran::make_byte_buffer();
    if (recfg_buffer)
    {
      asn1::bit_ref bref(recfg_buffer->msg, recfg_buffer->get_tailroom());
      if (msg.pack(bref) != asn1::SRSASN_SUCCESS)
      {
        srsran::console("-------------pack error-----------\n");
      }
      recfg_buffer->N_bytes = bref.distance_bytes();
    }
    else
    {
    }
    cell_ctxt->recfg_wx_buffer.push_back(std::move(recfg_buffer));
    srsran::console("当前字节dddddddddddddddddddddddddddddddd�?=%u\n", cell_ctxt->recfg_wx_buffer[0]->N_bytes);
    for (uint8_t i = 0; i < cell_ctxt->recfg_wx_buffer[0]->N_bytes; i++)
    {
      srsran::console("%x\n", *(cell_ctxt->recfg_wx_buffer[0]->msg + i));
    }
    si_info.recfgLen = cell_ctxt->recfg_wx_buffer[0]->N_bytes;
    si_info.recfg_msg = cell_ctxt->recfg_wx_buffer[0]->msg;
  }

  void rrc::generate_recfg_voice()
  {
    enb_cell_common *cell_ctxt = cell_common_list->get_cc_idx(0);

    s_dl_dcch_msg_s msg = cfg.recfg_wx_voice;
    srsran::unique_byte_buffer_t recfg_buffer = srsran::make_byte_buffer();
    if (recfg_buffer)
    {
      asn1::bit_ref bref(recfg_buffer->msg, recfg_buffer->get_tailroom());
      if (msg.pack(bref) != asn1::SRSASN_SUCCESS)
      {
        srsran::console("-------------pack error-----------\n");
      }
      recfg_buffer->N_bytes = bref.distance_bytes();
    }
    else
    {
    }
    cell_ctxt->recfg_voice_buffer.push_back(std::move(recfg_buffer));
    srsran::console("当前字节ddddddddddvoice=%u\n", cell_ctxt->recfg_voice_buffer[0]->N_bytes);
    for (uint8_t i = 0; i < cell_ctxt->recfg_voice_buffer[0]->N_bytes; i++)
    {
      srsran::console("%x\n", *(cell_ctxt->recfg_voice_buffer[0]->msg + i));
    }
    si_info.recfgvoiceLen = cell_ctxt->recfg_voice_buffer[0]->N_bytes;
    si_info.recfg_voice_msg = cell_ctxt->recfg_voice_buffer[0]->msg;
  }

  void rrc::generate_recfg_kuopin()
  {
    enb_cell_common *cell_ctxt = cell_common_list->get_cc_idx(0);

    s_dl_dcch_msg_s msg = cfg.recfg_wx_kuopin;
    srsran::unique_byte_buffer_t recfg_buffer = srsran::make_byte_buffer();
    if (recfg_buffer)
    {
      asn1::bit_ref bref(recfg_buffer->msg, recfg_buffer->get_tailroom());
      if (msg.pack(bref) != asn1::SRSASN_SUCCESS)
      {
        srsran::console("-------------pack error-----------\n");
      }
      recfg_buffer->N_bytes = bref.distance_bytes();
    }
    else
    {
    }
    cell_ctxt->recfg_kuopin_buffer.push_back(std::move(recfg_buffer));
    srsran::console("当前字节ddddddddddvoicekuopinhsfghsd=%u\n", cell_ctxt->recfg_kuopin_buffer[0]->N_bytes);
    for (uint8_t i = 0; i < cell_ctxt->recfg_kuopin_buffer[0]->N_bytes; i++)
    {
      srsran::console("%x\n", *(cell_ctxt->recfg_kuopin_buffer[0]->msg + i));
    }
    si_info.recfgkuopinLen = cell_ctxt->recfg_kuopin_buffer[0]->N_bytes;
    si_info.recfg_kuopin_msg = cell_ctxt->recfg_kuopin_buffer[0]->msg;
  }

  void rrc::generate_norm_recfg_kuopin()
  {
    enb_cell_common *cell_ctxt = cell_common_list->get_cc_idx(0);

    s_dl_dcch_msg_s msg = cfg.recfg_norm_kuopin;
    srsran::unique_byte_buffer_t recfg_buffer = srsran::make_byte_buffer();
    if (recfg_buffer)
    {
      asn1::bit_ref bref(recfg_buffer->msg, recfg_buffer->get_tailroom());
      if (msg.pack(bref) != asn1::SRSASN_SUCCESS)
      {
        srsran::console("-------------pack error-----------\n");
      }
      recfg_buffer->N_bytes = bref.distance_bytes();
    }
    else
    {
    }
    cell_ctxt->recfg_norm_kuopin_buffer.push_back(std::move(recfg_buffer));
    srsran::console("当前字节ddddddddddnormalkuopin=%u\n", cell_ctxt->recfg_norm_kuopin_buffer[0]->N_bytes);
    for (uint8_t i = 0; i < cell_ctxt->recfg_norm_kuopin_buffer[0]->N_bytes; i++)
    {
      srsran::console("%x\n", *(cell_ctxt->recfg_norm_kuopin_buffer[0]->msg + i));
    }
    si_info.recfgnormkuopinLen = cell_ctxt->recfg_norm_kuopin_buffer[0]->N_bytes;
    si_info.recfg_norm_kuopin_msg = cell_ctxt->recfg_norm_kuopin_buffer[0]->msg;
  }

  void rrc::second_configure_mib()
  {
    configure_mib_wx();
  }
  void rrc::configure_mib_wx()
  {
    // Store configs,MIB in common cell ctxt list

    // generate and pack into MIB buffers
    enb_cell_common *cell_ctxt = cell_common_list->get_cc_idx(0);
    bcch_mbch_msg_s msg;
    msg.msg_wx = cfg.mib;

    if (cfg.ttcn_rrc_enble && cfg.ttcn_test_enble && rrc_adp->udp_.TC_513_mib_sib)
    {
      while (true)
      {
        if (rrc_adp->udp_.rrc_receive_info.size() != 0)
        {
          srsran::unique_byte_buffer_t ttcn_mib =
              srsran::make_byte_buffer();
          std::cout << "receive mib info from TTCN" << std::endl;
          ttcn_mib->init();
          rrc_adp->udp_.rrc_receive_info.try_pop(ttcn_mib);
          get_general_interface(&ttcn_mib);

          // TTCN-3 config parameters
          msg.msg_wx.sys_info_ver_tag.from_number(ttcn_mib->msg[8]);
          std::cout << "msg.msg_wx.sys_info_ver_tag" << msg.msg_wx.sys_info_ver_tag.to_string() << std::endl;

          break;
        }
      }
    }

    // Allocate a new PDU buffer, pack the MIB-WX message
    srsran::unique_byte_buffer_t mib_buffer = srsran::make_byte_buffer();
    if (mib_buffer)
    {
      asn1::bit_ref bref(mib_buffer->msg, mib_buffer->get_tailroom());
      if (msg.pack(bref) != asn1::SRSASN_SUCCESS)
      {
        srsran::console("-------------pack error-----------\n");
      }
      mib_buffer->N_bytes = bref.distance_bytes();
    }
    else
    {
    }
    cell_ctxt->mib_buffer.push_back(std::move(mib_buffer));
    srsran::console("当前字节�?=%u\n", cell_ctxt->mib_buffer[0]->N_bytes);
    for (uint8_t i = 0; i < cell_ctxt->mib_buffer[0]->N_bytes; i++)
    {
      srsran::console("%x\n", *(cell_ctxt->mib_buffer[0]->msg + i));
    }
    si_info.mibLen = cell_ctxt->mib_buffer[0]->N_bytes;
    si_info.mib_msg = cell_ctxt->mib_buffer[0]->msg;

    if (cfg.ttcn_rrc_enble && cfg.ttcn_test_enble && rrc_adp->udp_.TC_513_mib_sib)
    {
      std::cout << "send mib  kong bao" << std::endl;
      srsran::unique_byte_buffer_t rerurn_info_send = srsran::make_byte_buffer();
      rerurn_info_send->init();
      rerurn_info_send->msg[0] = 0x01;
      rerurn_info_send->msg[1] = 0x01;
      rerurn_info_send->msg[2] = 0x01;
      rerurn_info_send->msg[3] = 0x01;
      rerurn_info_send->msg[4] = 0x01;
      rerurn_info_send->msg[5] = 0x01;
      rerurn_info_send->N_bytes = 6;
      rrc_adp->udp_.send_ttcn_info.try_push(std::move(rerurn_info_send));
    }
  }

  void rrc::get_general_interface(srsran::unique_byte_buffer_t *pdu_)
  {
    RRC_gen_inteface_.direction = pdu_->get()->msg[0];
    RRC_gen_inteface_.rec_lay_id = pdu_->get()->msg[1];
    RRC_gen_inteface_.des_layer_id = pdu_->get()->msg[2];
    RRC_gen_inteface_.test_id = (pdu_->get()->msg[3] << 8) | (pdu_->get()->msg[4]);
    RRC_gen_inteface_.data_length = (pdu_->get()->msg[5] << 8) | (pdu_->get()->msg[6]);
  }
  void rrc::second_generate_sib()
  {
    generate_sib_wx();
  }

  void rrc::generate_sib_wx()
  {
    // generate and pack into SIB buffers
    enb_cell_common *cell_ctxt = cell_common_list->get_cc_idx(0);
    bcch_sbch_msg_s msg;
      uint16_t mcc;


    // Copy SIB to message
    msg.msg = cfg.sib;

    if (cfg.ttcn_rrc_enble && cfg.ttcn_test_enble && rrc_adp->udp_.TC_512_qrexlevmin)
    {
      while (true)
      {
        if (rrc_adp->udp_.rrc_receive_info.size() != 0)
        {
          srsran::unique_byte_buffer_t ttcn_sib =
              srsran::make_byte_buffer();
          std::cout << "receive sib qrxlevmin info from TTCN" << std::endl;
          ttcn_sib->init();
          rrc_adp->udp_.rrc_receive_info.try_pop(ttcn_sib);
          get_general_interface(&ttcn_sib);

          // TTCN-3 config parameters
          msg.msg.beam_sel_rel_info.q_rx_lev_min.q_min = (ttcn_sib->msg[9]) - 70;
          std::cout << "msg.msg.beam_sel_rel_info.q_rx_lev_min.q_min: " << msg.msg.beam_sel_rel_info.q_rx_lev_min.q_min << std::endl;

          break;
        }
      }
    }

    if (cfg.ttcn_rrc_enble && cfg.ttcn_test_enble && rrc_adp->udp_.TC_513_mib_sib)
    {
      while (true)
      {
        if (rrc_adp->udp_.rrc_receive_info.size() != 0)
        {
          srsran::unique_byte_buffer_t ttcn_sib =
              srsran::make_byte_buffer();
          std::cout << "receive sib notbarred info from TTCN" << std::endl;
          ttcn_sib->init();
          rrc_adp->udp_.rrc_receive_info.try_pop(ttcn_sib);
          get_general_interface(&ttcn_sib);

          rrc_sib_info_.msgType = ttcn_sib->msg[7];
          rrc_sib_info_.beam_barred = ttcn_sib->msg[8];

          // TTCN-3 config parameters
          msg.msg.beam_acc_rel_info.beam_barred = static_cast<
              asn1::rrc::sib_wx_s::beam_acc_rel_info_s::beam_barred_opts::options>(
              ttcn_sib->msg[8]);
          std::cout << "msg.msg.beam_acc_rel_info.beam_barred: " << msg.msg.beam_acc_rel_info.beam_barred.value << std::endl;
          rrc_adp->udp_.TC_513_mac_mib = true;
          rrc_adp->udp_.TC_513_mac_not_barred = true;

          // rrc_adp->udp_.TC_513_barred_con_req=true;

          break;
        }
      }
    }

    if (cfg.ttcn_rrc_enble && cfg.ttcn_test_enble && rrc_adp->udp_.TC_513_barred)
    {
      while (true)
      {
        if (rrc_adp->udp_.rrc_receive_info.size() != 0)
        {
          srsran::unique_byte_buffer_t ttcn_sib =
              srsran::make_byte_buffer();
          std::cout << "receive sib barred info from TTCN" << std::endl;
          ttcn_sib->init();
          rrc_adp->udp_.rrc_receive_info.try_pop(ttcn_sib);
          get_general_interface(&ttcn_sib);

          rrc_sib_info_.msgType = ttcn_sib->msg[7];
          rrc_sib_info_.beam_barred = ttcn_sib->msg[8];

          // TTCN-3 config parameters
          msg.msg.beam_acc_rel_info.beam_barred = static_cast<
              asn1::rrc::sib_wx_s::beam_acc_rel_info_s::beam_barred_opts::options>(
              ttcn_sib->msg[8]);
          std::cout << "msg.msg.beam_acc_rel_info.beam_barred: " << msg.msg.beam_acc_rel_info.beam_barred.value << std::endl;

          break;
        }
      }
    }

    if (rrc_adp->udp_.enble_ttcn_flag_.ttcn_testId == 514)
    {
      msg.msg.beam_sel_rel_info.q_rx_lev_min.q_min = -50;
    }

    // Pack payload for SIB
    srsran::unique_byte_buffer_t sib_buffer = srsran::make_byte_buffer();
    if (sib_buffer)
    {
      asn1::bit_ref bref(sib_buffer->msg, sib_buffer->get_tailroom());
      if (msg.pack(bref) != asn1::SRSASN_SUCCESS)
      {
        // parent->rrc_log->error_hex(mib->msg,mib->N_bytes,"Failed to pack DL-CCCH-Msg:\n");
        srsran::console("---pack error---\n");
      }
      sib_buffer->N_bytes = bref.distance_bytes();
    }
    else
    {
    }

      if (srsran::bytes_to_mcc(&msg.msg.beam_acc_rel_info.plmn_id_li[0].mnc[0], &mcc)) {
        printf("mnc %u", mcc);
      }


    cell_ctxt->sib_wx_buffer.clear();
    cell_ctxt->sib_wx_buffer.push_back(std::move(sib_buffer));
    srsran::console("当前barred=%u\n", cell_ctxt->sib_wx_buffer.back()->msg[3]);
    srsran::console("当前字节�?=%u\n", cell_ctxt->sib_wx_buffer.back()->N_bytes);
      si_info.sibLen  = cell_ctxt->sib_wx_buffer.back()->N_bytes;

      if (si_info.sib_msg != NULL)
      {
        free(si_info.sib_msg);
        si_info.sib_msg = NULL;
      }
      
      si_info.sib_msg = (uint8_t*)malloc(si_info.sibLen*sizeof(uint8));
      memcpy(si_info.sib_msg, cell_ctxt->sib_wx_buffer.back()->data(), si_info.sibLen);

    for (uint8_t i = 0; i < cell_ctxt->sib_wx_buffer.back()->N_bytes; i++)
    {
      srsran::console("%x\n", *(si_info.sib_msg + i));
    }




    // si_info.sib_msg = cell_ctxt->sib_wx_buffer.back()->msg;
    // si_info.sibLen = cell_ctxt->sib_wx_buffer.back()->N_bytes;

    if (cfg.ttcn_rrc_enble && cfg.ttcn_test_enble && rrc_adp->udp_.TC_513_barred)
    {
      std::cout << "send sib barred  kong bao" << std::endl;
      srsran::unique_byte_buffer_t rerurn_info_send = srsran::make_byte_buffer();
      rerurn_info_send->init();
      rerurn_info_send->msg[0] = 0x01;
      rerurn_info_send->msg[1] = 0x01;
      rerurn_info_send->msg[2] = 0x01;
      rerurn_info_send->msg[3] = 0x01;
      rerurn_info_send->msg[4] = 0x01;
      rerurn_info_send->msg[5] = 0x01;
      rerurn_info_send->N_bytes = 6;
      rrc_adp->udp_.send_ttcn_info.try_push(std::move(rerurn_info_send));
    }
  }

  //-------------------2024.06.06-------------------
  bool rrc::rrc_handle_ate_msg(srsran::unique_byte_buffer_t pdu)
  {
    std::cout << "handle_ate_rrc_msg function!" << std::endl;

    if (pdu->N_bytes < 4)
    {
      std::cout << "handle_ate_msg error size <4" << std::endl;
      return false;
    }
    switch (pdu->msg[2])
    {
    case TC_MSG_ATE_RRC_RELEASE:
      if (pdu->msg[3] == 0x01)
      {
        ate_control_release_complete(70);
        srsran::unique_byte_buffer_t curr_status = srsran::make_byte_buffer();
        curr_status->init();
        curr_status->N_bytes = 4;
        curr_status->msg[0] = 0xff;
        curr_status->msg[1] = 0x02;
        curr_status->msg[2] = TC_MSG_RRC_ATE_CON;
        curr_status->msg[3] = 0x00;

        // send to ate msg queue
        srsran::console("send curr_status to ate queue \n");
        // rrc_adp->udp_.send_ate_info.try_push(std::move(curr_status));
        rrc_adp->udp_.send_ate_msg(std::move(curr_status));
      }
      else
      {
        srsran::console("TC_MSG_ATE_RRC_RELEASE Unknown indicate type (%d)", pdu->msg[3]);
        return false;
      }
      break;
    case TC_MSG_ATE_RRC_PAGING:
      if (pdu->msg[3] == 0x01)
      {
        srsran::unique_byte_buffer_t pag_pdu = srsran::make_byte_buffer();
        add_paging_id_wx_s(std::move(pag_pdu));
        srsran::unique_byte_buffer_t paging_res = srsran::make_byte_buffer();
        paging_res->init();
        paging_res->N_bytes = 3;
        paging_res->msg[0] = 0xff;
        paging_res->msg[1] = 0x02;
        paging_res->msg[2] = TC_MSG_ATE_RRC_PAGING_RES;

        // send to ate msg queue
        srsran::console("send ATE_RRC_PAGING_RES to ate queue \n");
        // rrc_adp->udp_.send_ate_info.try_push(std::move(paging_res));
        rrc_adp->udp_.send_ate_msg(std::move(paging_res));
      }
      else
      {
        srsran::console("TC_MSG_ATE_RRC_PAGING Unknown indicate type (%d)", pdu->msg[3]);
        return false;
      }
      break;
    case TC_MSG_ATE_RRC_MEASURE:
      if (pdu->msg[3] == 0x01)
      {
        ate_control_measurement_reconfig(70);
      }
      else
      {
        srsran::console("TC_MSG_ATE_RRC_MEASURE Unknown indicate type (%d)", pdu->msg[3]);
        return false;
      }
      break;
    default:
      srsran::console("Unknown message type (%d)", pdu->msg[2]);
      return false;
      break;
    }

    return true;
  }
  //-----------------------

  /* This methods packs the SIBs for each component carrier and stores them
   * inside the sib_buffer, a vector of SIBs for each CC.
   *
   * Before packing the message, it patches the cell specific params of
   * the SIB, including the cellId and the PRACH config index.
   *
   * The number of generates SIB messages is stored in the class member nof_si_messages
   *
   * @return SRSRAN_SUCCESS on success, SRSRAN_ERROR on failure
   */
  uint32_t rrc::generate_sibs()
  {
    // nof_messages includes SIB2 by default, plus all configured SIBs
    uint32_t nof_messages = 1 + cfg.sib1.sched_info_list.size();
    sched_info_list_l &sched_info = cfg.sib1.sched_info_list;

    // Store configs,SIBs in common cell ctxt list
    cell_common_list.reset(new enb_cell_common_list{cfg});

    // generate and pack into SIB buffers
    for (uint32_t cc_idx = 0; cc_idx < cfg.cell_list.size(); cc_idx++)
    {
      enb_cell_common *cell_ctxt = cell_common_list->get_cc_idx(cc_idx);
      // msg is array of SI messages, each SI message msg[i] may contain multiple SIBs
      // all SIBs in a SI message msg[i] share the same periodicity
      asn1::dyn_array<bcch_dl_sch_msg_s> msg(nof_messages + 1);

      // Copy SIB1 to first SI message
      msg[0].msg.set_c1().set_sib_type1() = cell_ctxt->sib1;

      // Copy rest of SIBs
      for (uint32_t sched_info_elem = 0; sched_info_elem < nof_messages - 1; sched_info_elem++)
      {
        uint32_t msg_index = sched_info_elem + 1; // first msg is SIB1, therefore start with second

        msg[msg_index].msg.set_c1().set_sys_info().crit_exts.set_sys_info_r8();
        sys_info_r8_ies_s::sib_type_and_info_l_ &sib_list =
            msg[msg_index].msg.c1().sys_info().crit_exts.sys_info_r8().sib_type_and_info;

        // SIB2 always in second SI message
        if (msg_index == 1)
        {
          sib_info_item_c sibitem;
          sibitem.set_sib2() = cell_ctxt->sib2;
          sib_list.push_back(sibitem);
        }

        // Add other SIBs to this message, if any
        for (auto &mapping_enum : sched_info[sched_info_elem].sib_map_info)
        {
          sib_list.push_back(cfg.sibs[(int)mapping_enum + 2]);
        }
      }

      // Pack payload for all messages
      for (uint32_t msg_index = 0; msg_index < nof_messages; msg_index++)
      {
        srsran::unique_byte_buffer_t sib_buffer = srsran::make_byte_buffer();
        if (sib_buffer == nullptr)
        {
          logger.error("Couldn't allocate PDU in %s().", __FUNCTION__);
          return SRSRAN_ERROR;
        }
        asn1::bit_ref bref(sib_buffer->msg, sib_buffer->get_tailroom());
        if (msg[msg_index].pack(bref) != asn1::SRSASN_SUCCESS)
        {
          logger.error("Failed to pack SIB message %d", msg_index);
          return SRSRAN_ERROR;
        }
        sib_buffer->N_bytes = bref.distance_bytes();
        cell_ctxt->sib_buffer.push_back(std::move(sib_buffer));

        // Log SIBs in JSON format
        fmt::memory_buffer membuf;
        const char *msg_str = msg[msg_index].msg.c1().type().to_string();
        if (msg[msg_index].msg.c1().type().value != asn1::rrc::bcch_dl_sch_msg_type_c::c1_c_::types_opts::sib_type1)
        {
          msg_str = msg[msg_index].msg.c1().sys_info().crit_exts.type().to_string();
        }
        fmt::format_to(membuf, "{}, cc={}, idx={}", msg_str, cc_idx, msg_index);
        log_broadcast_rrc_message(SRSRAN_SIRNTI, *cell_ctxt->sib_buffer.back(), msg[msg_index], srsran::to_c_str(membuf));
      }

      if (cfg.sibs[6].type() == asn1::rrc::sys_info_r8_ies_s::sib_type_and_info_item_c_::types::sib7)
      {
        sib7 = cfg.sibs[6].sib7();
      }
    }

    nof_si_messages = nof_messages;

    return SRSRAN_SUCCESS;
  }

  void rrc::configure_mbsfn_sibs()
  {
    // populate struct with sib2 values needed in PHY/MAC
    srsran::sib2_mbms_t sibs2;
    sibs2.mbsfn_sf_cfg_list_present = cfg.sibs[1].sib2().mbsfn_sf_cfg_list_present;
    sibs2.nof_mbsfn_sf_cfg = cfg.sibs[1].sib2().mbsfn_sf_cfg_list.size();
    for (int i = 0; i < sibs2.nof_mbsfn_sf_cfg; i++)
    {
      sibs2.mbsfn_sf_cfg_list[i].nof_alloc_subfrs = srsran::mbsfn_sf_cfg_t::sf_alloc_type_t::one_frame;
      sibs2.mbsfn_sf_cfg_list[i].radioframe_alloc_offset =
          cfg.sibs[1].sib2().mbsfn_sf_cfg_list[i].radioframe_alloc_offset;
      sibs2.mbsfn_sf_cfg_list[i].radioframe_alloc_period =
          (srsran::mbsfn_sf_cfg_t::alloc_period_t)cfg.sibs[1].sib2().mbsfn_sf_cfg_list[i].radioframe_alloc_period.value;
      sibs2.mbsfn_sf_cfg_list[i].sf_alloc =
          (uint32_t)cfg.sibs[1].sib2().mbsfn_sf_cfg_list[i].sf_alloc.one_frame().to_number();
    }
    // populate struct with sib13 values needed for PHY/MAC
    srsran::sib13_t sibs13;
    sibs13.notif_cfg.notif_offset = cfg.sibs[12].sib13_v920().notif_cfg_r9.notif_offset_r9;
    sibs13.notif_cfg.notif_repeat_coeff =
        (srsran::mbms_notif_cfg_t::coeff_t)cfg.sibs[12].sib13_v920().notif_cfg_r9.notif_repeat_coeff_r9.value;
    sibs13.notif_cfg.notif_sf_idx = cfg.sibs[12].sib13_v920().notif_cfg_r9.notif_sf_idx_r9;
    sibs13.nof_mbsfn_area_info = cfg.sibs[12].sib13_v920().mbsfn_area_info_list_r9.size();
    for (uint32_t i = 0; i < sibs13.nof_mbsfn_area_info; i++)
    {
      sibs13.mbsfn_area_info_list[i].mbsfn_area_id =
          cfg.sibs[12].sib13_v920().mbsfn_area_info_list_r9[i].mbsfn_area_id_r9;
      sibs13.mbsfn_area_info_list[i].notif_ind = cfg.sibs[12].sib13_v920().mbsfn_area_info_list_r9[i].notif_ind_r9;
      sibs13.mbsfn_area_info_list[i].mcch_cfg.sig_mcs = (srsran::mbsfn_area_info_t::mcch_cfg_t::sig_mcs_t)cfg.sibs[12]
                                                            .sib13_v920()
                                                            .mbsfn_area_info_list_r9[i]
                                                            .mcch_cfg_r9.sig_mcs_r9.value;
      sibs13.mbsfn_area_info_list[i].mcch_cfg.sf_alloc_info =
          cfg.sibs[12].sib13_v920().mbsfn_area_info_list_r9[i].mcch_cfg_r9.sf_alloc_info_r9.to_number();
      sibs13.mbsfn_area_info_list[i].mcch_cfg.mcch_repeat_period =
          (srsran::mbsfn_area_info_t::mcch_cfg_t::repeat_period_t)cfg.sibs[12]
              .sib13_v920()
              .mbsfn_area_info_list_r9[i]
              .mcch_cfg_r9.mcch_repeat_period_r9.value;
      sibs13.mbsfn_area_info_list[i].mcch_cfg.mcch_offset =
          cfg.sibs[12].sib13_v920().mbsfn_area_info_list_r9[i].mcch_cfg_r9.mcch_offset_r9;
      sibs13.mbsfn_area_info_list[i].mcch_cfg.mcch_mod_period =
          (srsran::mbsfn_area_info_t::mcch_cfg_t::mod_period_t)cfg.sibs[12]
              .sib13_v920()
              .mbsfn_area_info_list_r9[i]
              .mcch_cfg_r9.mcch_mod_period_r9.value;
      sibs13.mbsfn_area_info_list[i].non_mbsfn_region_len = (srsran::mbsfn_area_info_t::region_len_t)cfg.sibs[12]
                                                                .sib13_v920()
                                                                .mbsfn_area_info_list_r9[i]
                                                                .non_mbsfn_region_len.value;
      sibs13.mbsfn_area_info_list[i].notif_ind = cfg.sibs[12].sib13_v920().mbsfn_area_info_list_r9[i].notif_ind_r9;
    }

    // pack MCCH for transmission and pass relevant MCCH values to PHY/MAC
    pack_mcch();
    srsran::mcch_msg_t mcch_t;
    mcch_t.common_sf_alloc_period = srsran::mcch_msg_t::common_sf_alloc_period_t::rf64;
    mcch_t.nof_common_sf_alloc = 1;
    srsran::mbsfn_sf_cfg_t sf_alloc_item = mcch_t.common_sf_alloc[0];
    sf_alloc_item.radioframe_alloc_offset = 0;
    sf_alloc_item.radioframe_alloc_period = srsran::mbsfn_sf_cfg_t::alloc_period_t::n1;
    sf_alloc_item.sf_alloc = 63;
    mcch_t.nof_pmch_info = 1;
    srsran::pmch_info_t *pmch_item = &mcch_t.pmch_info_list[0];

    pmch_item->nof_mbms_session_info = 1;
    pmch_item->mbms_session_info_list[0].lc_ch_id = 1;
    if (pmch_item->nof_mbms_session_info > 1)
    {
      pmch_item->mbms_session_info_list[1].lc_ch_id = 2;
    }
    uint16_t mbms_mcs = cfg.mbms_mcs;
    if (mbms_mcs > 28)
    {
      mbms_mcs = 28; // TS 36.213, Table 8.6.1-1
      logger.warning("PMCH data MCS too high, setting it to 28");
    }
    logger.debug("PMCH data MCS=%d", mbms_mcs);
    pmch_item->data_mcs = mbms_mcs;
    pmch_item->mch_sched_period = srsran::pmch_info_t::mch_sched_period_t::rf64;
    pmch_item->sf_alloc_end = 64 * 6;

    // Configure PHY when PHY is done being initialized
    task_sched.defer_task([this, sibs2, sibs13, mcch_t]() mutable
                          {
    phy->configure_mbsfn(&sibs2, &sibs13, mcch_t);
    mac->write_mcch(&sibs2, &sibs13, &mcch_t, mcch_payload_buffer, current_mcch_length); });
  }

  int rrc::pack_mcch()
  {
    mcch.msg.set_c1();
    mbsfn_area_cfg_r9_s &area_cfg_r9 = mcch.msg.c1().mbsfn_area_cfg_r9();
    area_cfg_r9.common_sf_alloc_period_r9 = mbsfn_area_cfg_r9_s::common_sf_alloc_period_r9_e_::rf64;
    area_cfg_r9.common_sf_alloc_r9.resize(1);
    mbsfn_sf_cfg_s *sf_alloc_item = &area_cfg_r9.common_sf_alloc_r9[0];
    sf_alloc_item->radioframe_alloc_offset = 0;
    sf_alloc_item->radioframe_alloc_period = mbsfn_sf_cfg_s::radioframe_alloc_period_e_::n1;
    sf_alloc_item->sf_alloc.set_one_frame().from_number(32 + 31);

    area_cfg_r9.pmch_info_list_r9.resize(1);
    pmch_info_r9_s *pmch_item = &area_cfg_r9.pmch_info_list_r9[0];
    pmch_item->mbms_session_info_list_r9.resize(1);

    pmch_item->mbms_session_info_list_r9[0].lc_ch_id_r9 = 1;
    pmch_item->mbms_session_info_list_r9[0].session_id_r9_present = true;
    pmch_item->mbms_session_info_list_r9[0].session_id_r9[0] = 0;
    pmch_item->mbms_session_info_list_r9[0].tmgi_r9.plmn_id_r9.set_explicit_value_r9();
    srsran::plmn_id_t plmn_obj;
    plmn_obj.from_string("00003");
    srsran::to_asn1(&pmch_item->mbms_session_info_list_r9[0].tmgi_r9.plmn_id_r9.explicit_value_r9(), plmn_obj);
    uint8_t byte[] = {0x0, 0x0, 0x0};
    memcpy(&pmch_item->mbms_session_info_list_r9[0].tmgi_r9.service_id_r9[0], &byte[0], 3);

    if (pmch_item->mbms_session_info_list_r9.size() > 1)
    {
      pmch_item->mbms_session_info_list_r9[1].lc_ch_id_r9 = 2;
      pmch_item->mbms_session_info_list_r9[1].session_id_r9_present = true;
      pmch_item->mbms_session_info_list_r9[1].session_id_r9[0] = 1;
      pmch_item->mbms_session_info_list_r9[1].tmgi_r9.plmn_id_r9.set_explicit_value_r9() =
          pmch_item->mbms_session_info_list_r9[0].tmgi_r9.plmn_id_r9.explicit_value_r9();
      byte[2] = 1;
      memcpy(&pmch_item->mbms_session_info_list_r9[1].tmgi_r9.service_id_r9[0],
             &byte[0],
             3); // TODO: Check if service is set to 1
    }

    uint16_t mbms_mcs = cfg.mbms_mcs;
    if (mbms_mcs > 28)
    {
      mbms_mcs = 28; // TS 36.213, Table 8.6.1-1
      logger.warning("PMCH data MCS too high, setting it to 28");
    }

    logger.debug("PMCH data MCS=%d", mbms_mcs);
    pmch_item->pmch_cfg_r9.data_mcs_r9 = mbms_mcs;
    pmch_item->pmch_cfg_r9.mch_sched_period_r9 = pmch_cfg_r9_s::mch_sched_period_r9_e_::rf64;
    pmch_item->pmch_cfg_r9.sf_alloc_end_r9 = 64 * 6;

    const int rlc_header_len = 1;
    asn1::bit_ref bref(&mcch_payload_buffer[rlc_header_len], sizeof(mcch_payload_buffer) - rlc_header_len);
    if (mcch.pack(bref) != asn1::SRSASN_SUCCESS)
    {
      logger.error("Failed to pack MCCH message");
    }

    current_mcch_length = bref.distance_bytes(&mcch_payload_buffer[1]);
    current_mcch_length = current_mcch_length + rlc_header_len;
    return current_mcch_length;
  }

  /*******************************************************************************
    RRC run tti method
  *******************************************************************************/
  //--------------------------------------------------------------------------------------------------------------------------------------------------------------
  void rrc::tti_clock_s()
  {
    // pop cmds from quene
    rrc_pdu p;

    while (rx_pdu_queue.try_pop(p))
    {
      auto user_it = users.find(p.rnti);
      if (user_it == users.end())
      {
        if (p.pdu != nullptr)
        {
          log_rx_pdu_fail(p.rnti, p.lcid, *p.pdu, "unknown rnti");
        }
        else
        {
          logger.warning("Ignoring rnti=0x%x command. Cause: unknown rnti", p.rnti);
        }
        continue;
      }
      ue &ue = *user_it->second;

      // handle queue cmd
      switch (p.lcid)
      {
      case srb_to_lcid(lte_srb::srb0):
        parse_ul_ccch_s(ue, std::move(p.pdu));
        break;
      case srb_to_lcid(lte_srb::srb1):
      case srb_to_lcid(lte_srb::srb2):
        parse_ul_dcch_s(ue, p.lcid, std::move(p.pdu));
        break;
      // case LCID_REM_USER:
      //   rem_user(p.rnti);
      //   break;
      // case LCID_REL_USER:
      //   process_release_complete_s(p.rnti);
      //   break;
      default:
        logger.error("Rx PDU with invalid bearer id: %d", p.lcid);
        break;
      }
    }
  }
  //-------------------------------------------------------------------------------------------------------
  //-----------------------------------------IOT-2023/9/19-------------------------------------------------
  void rrc::iot_tti_clock_s()
  {
    // pop cmds from quene
    rrc_pdu p;

    while (rx_pdu_queue.try_pop(p))
    {
      // std::cout<<"-------------------------------------------- x1:"<<p.rnti<<std::endl;
      // for(int i=0;i<(int)p.pdu->N_bytes;i++)
      //   srsran::console("0x%x\n",*(p.pdu->msg+i));
      auto user_it = users.find(p.rnti);
      if (user_it == users.end())
      {
        if (p.pdu != nullptr)
        {
          log_rx_pdu_fail(p.rnti, p.lcid, *p.pdu, "unknown rnti");
        }
        else
        {
          logger.warning("Ignoring rnti=0x%x command. Cause: unknown rnti", p.rnti);
        }
        continue;
      }
      ue &ue = *user_it->second;

      // handle queue cmd
      switch (p.lcid)
      {
      case srb_to_lcid(lte_srb::srb0):
        iot_parse_ul_ccch_s(ue, std::move(p.pdu));
        break;
      case srb_to_lcid(lte_srb::srb1):
      case srb_to_lcid(lte_srb::srb2):
        iot_parse_ul_dcch_s(ue, p.lcid, std::move(p.pdu));
        break;
      default:
        logger.error("Rx PDU with invalid bearer id: %d", p.lcid);
        break;
      }
    }
  }
  //---------------------------------------------------------------------------------------------------------
  /*
  void rrc::tti_clock()
  {
    // pop cmds from queue
    rrc_pdu p;
    while (rx_pdu_queue.try_pop(p)) {
      // check if user exists
      auto user_it = users.find(p.rnti);
      if (user_it == users.end()) {
        if (p.pdu != nullptr) {
          log_rx_pdu_fail(p.rnti, p.lcid, *p.pdu, "unknown rnti");
        } else {
          logger.warning("Ignoring rnti=0x%x command. Cause: unknown rnti", p.rnti);
        }
        continue;
      }
      ue& ue = *user_it->second;

      // handle queue cmd
      switch (p.lcid) {
        case srb_to_lcid(lte_srb::srb0):
          parse_ul_ccch(ue, std::move(p.pdu));
          break;
        case srb_to_lcid(lte_srb::srb1):
        case srb_to_lcid(lte_srb::srb2):
          parse_ul_dcch(ue, p.lcid, std::move(p.pdu));
          break;
        case LCID_REM_USER:
          rem_user(p.rnti);
          break;
        case LCID_REL_USER:
          process_release_complete(p.rnti);
          break;
        case LCID_ACT_USER:
          user_it->second->set_activity();
          break;
        case LCID_RADLINK_DL:
          user_it->second->set_radiolink_dl_state(p.arg);
          break;
        case LCID_RADLINK_UL:
          user_it->second->set_radiolink_ul_state(p.arg);
          break;
        case LCID_RLC_RTX:
          user_it->second->max_rlc_retx_reached();
          break;
        case LCID_PROT_FAIL:
          user_it->second->protocol_failure();
          break;
        case LCID_EXIT:
          logger.info("Exiting thread");
          break;
        default:
          logger.error("Rx PDU with invalid bearer id: %d", p.lcid);
          break;
      }
    }
  }
  */

  void rrc::log_rx_pdu_fail(uint16_t rnti, uint32_t lcid, srsran::const_byte_span pdu, const char *cause_str)
  {
    logger.error(
        pdu.data(), pdu.size(), "Rx %s PDU, rnti=0x%x - Discarding. Cause: %s", get_rb_name(lcid), rnti, cause_str);
  }

  void rrc::log_rxtx_pdu_impl(direction_t dir,
                              uint16_t rnti,
                              uint32_t lcid,
                              srsran::const_byte_span pdu,
                              const char *msg_type)
  {
    static const char *dir_str[] = {"Rx", "Tx", "Tx S1AP", "Rx S1AP"};
    fmt::memory_buffer membuf;
    fmt::format_to(membuf, "{} ", dir_str[dir]);
    if (rnti != SRSRAN_PRNTI and rnti != SRSRAN_SIRNTI)
    {
      if (dir == Tx or dir == Rx)
      {
        fmt::format_to(membuf, "{} ", srsran::get_srb_name(srsran::lte_lcid_to_srb(lcid)));
      }
      fmt::format_to(membuf, "PDU, rnti=0x{:x} ", rnti);
    }
    else
    {
      fmt::format_to(membuf, "Broadcast PDU ");
    }
    fmt::format_to(membuf, "- {} ({} B)", msg_type, pdu.size());

    logger.info(pdu.data(), pdu.size(), "%s", srsran::to_c_str(membuf));
  }

  /*******************************************************************************
    CNW interface
  *******************************************************************************/
  //-----------------------------------2023/10/31-----------------------------------
  void rrc::trans_cnw(srsepc::cnw_interface_enb *cnw_)
  {
    std::cout << "trans cnw entity access!" << std::endl;
    cnw = cnw_;
  }

  //--------------------------2024.03.05-----------------------------
  void rrc::s_write_ims_dl_info(uint16_t rnti, srsran::unique_byte_buffer_t sdu)
  {
    std::cout << "@@@@@@@@@@@@@-------This is an iiimmmsss message--------@@@@@@@@@@@@@" << std::endl;
    // for (uint32_t i = 0; i < sdu->N_bytes; i++) {
    //   printf("0x%x\n", *(sdu->msg + i));
    // }
    s_dl_dcch_msg_s s_dl_dcch_msg;
    s_dl_dcch_msg.msg.set_dl_info_tran();
    dl_info_tran_s *dl_info_tran = &s_dl_dcch_msg.msg.set_dl_info_tran();
    auto user_it = users.find(rnti);
    if (user_it != users.end())
    {
      dl_info_tran->dl_info_tran_r1.dedi_info_type.set_dedi_info_scm();
      dedi_info_scm_s *dedi_info_scm = &dl_info_tran->dl_info_tran_r1.dedi_info_type.set_dedi_info_scm();
      dedi_info_scm->delicated_info_scm.resize(sdu->N_bytes);
      memcpy(dedi_info_scm->delicated_info_scm.data(), sdu->msg, sdu->N_bytes);

      sdu->clear();
      user_it->second->s_send_dl_dcch(&s_dl_dcch_msg, std::move(sdu));
    }
    else
    {
      logger.error("Rx SDU for unknown rnti=0x%x", rnti);
    }
    return;
  }

  //------------------------------------------------------
  // for testcase reg_req_congestion
  void rrc::reg_req_congestion_process_release_complete_s(uint16_t rnti)
  {
    std::cout<<"is_registration_reject_Ue_attempt_five_count:"<<rrc_adp->udp_.is_registration_reject_Ue_attempt_five_count<<std::endl;
    logger.info("Received Release Complete rnti=0x%x", rnti);
    auto user_it = users.find(rnti);
    if (user_it == users.end())
    {
      logger.error("Received ReleaseComplete for unknown rnti=0x%x", rnti);
      return;
    }
    ue *u = user_it->second.get();
    if (rrc_adp->udp_.is_reg_rej_illegal_ue)
    {
      user_it->second->s_send_connection_release_reg_req_congestion();
    }
    else if (rrc_adp->udp_.is_reg_rej_plmn_not_allowed||rrc_adp->udp_.is_registration_reject_Ue_attempt_five_count)
    {
      user_it->second->s_send_connection_release();
    }
    else if(rrc_adp->udp_.is_authentication_reject)
    {
      user_it->second->s_send_connection_release();
    }
  }

  void rrc::reg_ss_no5Gguti_process_release_complete_s(uint16_t rnti)
  {
    logger.info("Received Release Complete rnti=0x%x", rnti);
    auto user_it = users.find(rnti);
    if (user_it == users.end())
    {
      logger.error("Received ReleaseComplete for unknown rnti=0x%x", rnti);
      return;
    }
    ue *u = user_it->second.get();
    user_it->second->s_send_connection_release_reg_ss_no5Gguti();
  }

  void rrc::start_rem_user(uint16_t rnti)
  {
    this->reg_req_congestion_process_release_complete_s(rnti);
  }
  

  /*cy 6.20*/
  void rrc::nas_to_notify_rrc_release(uint16_t rnti)
  {
    this->rem_user(rnti);
  }

  void rrc::testcase_reg_ss_no5Gguti(uint16_t rnti)
  {
    this->reg_ss_no5Gguti_process_release_complete_s(rnti);
  }

  void rrc::start_rem_rel_user(uint16_t rnti)
  {
    printf("---start_rem_rel_user----\n");
    auto user_it = users.find(rnti);
    if (user_it == users.end())
    {
      logger.error("Received ReleaseComplete for unknown rnti=0x%x", rnti);
      return;
    }
    user_it->second->s_send_connection_release();
  }

  void rrc::s_write_dl_info(uint16_t rnti, srsran::unique_byte_buffer_t sdu)
  {
    for (uint32_t i = 0; i < sdu->N_bytes; i++)
    {
      printf("0x%x\n", *(sdu->msg + i));
    }
    s_dl_dcch_msg_s s_dl_dcch_msg;
    s_dl_dcch_msg.msg.set_dl_info_tran();
    dl_info_tran_s *dl_info_tran = &s_dl_dcch_msg.msg.set_dl_info_tran();
    auto user_it = users.find(rnti);
    if (user_it != users.end())
    {
      std::cout << "@@@@@@@@@@@@@-------This is an dl nas message--------@@@@@@@@@@@@@" << std::endl;

      dl_info_tran->dl_info_tran_r1.dedi_info_type.set_dedi_info_nas();
      dedi_info_nas_s *dedi_info_nas = &dl_info_tran->dl_info_tran_r1.dedi_info_type.set_dedi_info_nas();
      dedi_info_nas->delicated_info_nas.resize(sdu->N_bytes);
      memcpy(dedi_info_nas->delicated_info_nas.data(), sdu->msg, sdu->N_bytes);

      sdu->clear();
      user_it->second->s_send_dl_dcch(&s_dl_dcch_msg, std::move(sdu));
    }
    else
    {
      logger.error("Rx SDU for unknown rnti=0x%x", rnti);
    }
    return;
  }

  void rrc::s_release_ue(uint16_t rnti)
  {
    rrc_pdu p = {rnti, LCID_REL_USER, false, nullptr};
    if (not rx_pdu_queue.try_push(std::move(p)))
    {
      logger.error("Failed to push Release command to RRC queue");
    }
  }

  bool rrc::s_setup_ue_ctxt(uint16_t rnti, const asn1::s1ap::init_ctxt_setup_req_s &msg)
  {
    logger.info("Adding initial context for 0x%x", rnti);
    auto user_it = users.find(rnti);
    if (user_it == users.end())
    {
      logger.warning("Unrecognised rnti: 0x%x", rnti);
      return false;
    }
    user_it->second->s_handle_ue_init_ctxt_setup_req(msg);
    return true;
  }

  bool rrc::s_modify_ue_ctxt(uint16_t rnti, const asn1::s1ap::ctxt_mod_req_s &msg)
  {
    logger.info("Modifying context for 0x%x", rnti);
    auto user_it = users.find(rnti);

    if (user_it == users.end())
    {
      logger.warning("Unrecognised rnti: 0x%x", rnti);
      return false;
    }
    return true; // 暂定
  }

  void rrc::s_add_paging_id(uint32_t ueid, const asn1::s1ap::ue_paging_id_c &ue_paging_id)
  {
    if (ue_paging_id.type().value == asn1::s1ap::ue_paging_id_c::types_opts::imsi)
    {
      pending_paging->add_imsi_paging(ueid, ue_paging_id.imsi());
    }
    else
    {
      pending_paging->add_tmsi_paging(ueid, ue_paging_id.s_tmsi().mmec[0], ue_paging_id.s_tmsi().m_tmsi);
    }
  }

  int rrc::s_setup_erab(uint16_t rnti,
                        uint16_t erab_id,
                        const asn1::s1ap::erab_level_qos_params_s &qos_params,
                        srsran::const_span<uint8_t> nas_pdu,
                        const asn1::bounded_bitstring<1, 160, true, true> &addr,
                        uint32_t gtpu_teid_out,
                        asn1::s1ap::cause_c &cause)
  {
    logger.info("Setting up erab id=%d for 0x%x", erab_id, rnti);
    auto user_it = users.find(rnti);
    if (user_it == users.end())
    {
      logger.warning("Unrecognised rnti: 0x%x", rnti);
      cause.set_radio_network().value = asn1::s1ap::cause_radio_network_opts::unknown_erab_id;
      return SRSRAN_ERROR;
    }
    return user_it->second->setup_erab(erab_id, qos_params, nas_pdu, addr, gtpu_teid_out, cause);
  }

  int rrc::s_modify_erab(uint16_t rnti,
                         uint16_t erab_id,
                         const asn1::s1ap::erab_level_qos_params_s &qos_params,
                         srsran::const_span<uint8_t> nas_pdu,
                         asn1::s1ap::cause_c &cause)
  {
    logger.info("Modifying E-RAB for 0x%x. E-RAB Id %d", rnti, erab_id);
    auto user_it = users.find(rnti);
    if (user_it == users.end())
    {
      logger.warning("Unrecognised rnti: 0x%x", rnti);
      cause.set_radio_network().value = asn1::s1ap::cause_radio_network_opts::unknown_erab_id;
      return SRSRAN_ERROR;
    }

    return user_it->second->modify_erab(erab_id, qos_params, nas_pdu, cause);
  }

  bool rrc::s_release_erabs(uint32_t rnti)
  {
    logger.info("Releasing E-RABs for 0x%x", rnti);
    auto user_it = users.find(rnti);

    if (user_it == users.end())
    {
      logger.warning("Unrecognised rnti: 0x%x", rnti);
      return false;
    }

    bool ret = user_it->second->release_erabs();
    return ret;
  }

  int rrc::s_release_erab(uint16_t rnti, uint16_t erab_id)
  {
    logger.info("Releasing E-RAB id=%d for 0x%x", erab_id, rnti);
    auto user_it = users.find(rnti);

    if (user_it == users.end())
    {
      logger.warning("Unrecognised rnti: 0x%x", rnti);
      return SRSRAN_ERROR;
    }

    return user_it->second->release_erab(erab_id);
  }
  //---------------------------------------2023.12.14----------------------------------------------------------------------------
  int rrc::smm_notify_ue_erab_updates(uint16_t rnti, uint8_t qos, uint16_t pdu_session_id, srsran::const_byte_span nas_pdu, int am_tm_type)
  {
    std::cout << "---------rrc::smm_notify_ue_erab_updates--------" << std::endl;
    auto user_it = users.find(rnti);
 
      if (user_it == users.end())
      {
        logger.warning("Unrecognised rnti: 0x%x", rnti);
        return SRSRAN_ERROR;
      }
      // zhjtest0624
      // user_it->second->send_rrc_con_reconf(rnti, qos, pdu_session_id, am_tm_type, nullptr, nas_pdu);
      std::cout << "test am_tm_type:" << am_tm_type << std::endl;
      if (am_tm_type == 1)
      {
        user_it->second->send_rrc_con_voice_reconf(rnti, qos, pdu_session_id, nullptr, nas_pdu);
        // user_it->second->send_rrc_con_reconf_data(rnti, qos, pdu_session_id, am_tm_type,nullptr, nas_pdu);
      }
      else
      {
        user_it->second->send_rrc_con_reconf(rnti, qos, pdu_session_id, am_tm_type, nullptr, nas_pdu);
      }
    
    return SRSRAN_SUCCESS;
  }
  void rrc::rrctorrc_ue(uint16_t rnti)
  {
    std::cout << "---------rrrctorrc_ue--------" << std::endl;
    auto user_it = users.find(rnti);
    if(rrc_adp->udp_.TC_720_measure_report_last)
    {
      user_it->second->send_rrc_measure_reconf();
    }
    if(rrc_adp->udp_.TC_716_reconf_fail_last)
    {
      printf("send TC_716_reconf_fail\n");
      user_it->second->send_rrc_con_reconf(70,2,1,0,nullptr,{});
    }
  }
  
  int rrc::test_smm_notify_ue_erab_updates(uint16_t rnti, uint8_t qos, uint16_t pdu_session_id, srsran::const_byte_span nas_pdu, int am_tm_type)
  {
    std::cout << "---------rrc::test_smm_notify_ue_erab_updates--------" << std::endl;
    auto user_it = users.find(rnti);
    if (user_it == users.end())
    {
      logger.warning("Unrecognised rnti: 0x%x", rnti);
      return SRSRAN_ERROR;
    }
    if (wx_area_mode == 0)
    {
      user_it->second->send_rrc_con_reconf_data(rnti, qos, pdu_session_id, am_tm_type, nullptr, nas_pdu);
    }
    else
    {
      user_it->second->send_rrc_con_reconf_kuopin_data(rnti, qos, pdu_session_id, am_tm_type, nullptr, nas_pdu);
    }

    // user_it->second->send_rrc_con_reconf_kuopin_data(rnti, qos, pdu_session_id, am_tm_type, nullptr, nas_pdu);
    return SRSRAN_SUCCESS;
  }

  //----------------------------------------------------------------------------------------------

  int rrc::wx_Mcontrol_Notify_Switch(uint16_t rnti, uint8_t Switch_Type_)
  {
    auto user_it = users.find(rnti);
    if (user_it == users.end())
    {
      logger.warning("Unrecognised rnti: 0x%x", rnti);
      return SRSRAN_ERROR;
    }
    switch (Switch_Type_)
    {
    case Data_Service_Switch: // 01 03
      std::cout << "Data_Service_Switch" << std::endl;
      user_it->second->wx_Switch_Dataing_or_NBusness();
      break;
    case Voice_Service_Switch:
      std::cout << "Voice_Service_Switch" << std::endl;
      user_it->second->wx_Switch_Voicing();
      break;
    }

    return SRSRAN_SUCCESS;
  }

  bool rrc::wx_TargetBeam_ReadMib(const std::string &filename)
  {
    std::cout << "filename = " << filename << std::endl;
    std::ifstream file(filename);
    std::string line;

    std::unordered_map<std::string, std::function<void(const std::string &)>> assigners = {
        {"frame_off", [&](const std::string &value)
         { frame_off = std::stoi(value); }}};

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
      std::cout << "frame_off=" << frame_off << std::endl;

      return true;
    }
    else
    {
      std::cerr << "Unable to open file" << std::endl;
      return false;
    }
  }

  /**
   * sourceBeam trigger Switch
   * @date 2024/7/4
   * @param rnti=70
   */
  void rrc::wx_Trigger_switch_ACK(uint16_t rnti, srsran::unique_byte_buffer_t pdu_)
  {
    // TODO
    // 切换时读取mib信息，永远读目标波束的mib消息
    if (cfg.pid == 1)
    {
      // std::string targetBeam_readMib_filename = "/opt/AccessIot/access/beam1/mib1.conf";
      std::string targetBeam_readMib_filename = cfg.mib_config;
      std::cout<<" A targetBeam_readMib_filename = "<<targetBeam_readMib_filename<<std::endl;
      if (!wx_TargetBeam_ReadMib(targetBeam_readMib_filename))
      {
        std::cout << " READ MIB.CONF FAILURE IN TargetBeam !!!" << std::endl;
        return;
      }
    }
    else if (cfg.pid == 2)
    {
      // std::string targetBeam_readMib_filename = "/opt/AccessIot/access/beam2/mib1.conf";
      std::string targetBeam_readMib_filename = cfg.mib_config;
      std::cout<<" B targetBeam_readMib_filename = "<<targetBeam_readMib_filename<<std::endl;
      if (!wx_TargetBeam_ReadMib(targetBeam_readMib_filename))
      {
        std::cout << " READ MIB.CONF FAILURE IN TargetBeam !!!" << std::endl;
        return;
      }
    }

    std::cout << "NEW Frame Off =" << frame_off << std::endl;
    Switch_Info Temp_Switch_Info;
    Temp_Switch_Info.Beam_Header = 0xEE;
    Temp_Switch_Info.Beam_Type = Source_Beam;
    Temp_Switch_Info.Beam_InfoType = ACK;
    Temp_Switch_Info.Switch_Type = pdu_->msg[3];
    Temp_Switch_Info.Voice_Type = pdu_->msg[4];
    Switch_Voice = Temp_Switch_Info.Voice_Type;
    std::cout << "[wx_Trigger_switch_ACK][Switch_Voice]=" << (int)Switch_Voice << std::endl;

    srsran::unique_byte_buffer_t pdu = srsran::make_byte_buffer();
    pdu->N_bytes = 5;
    pdu->msg[0] = Temp_Switch_Info.Beam_Header;
    pdu->msg[1] = Temp_Switch_Info.Beam_Type;
    pdu->msg[2] = Temp_Switch_Info.Beam_InfoType;
    pdu->msg[3] = Temp_Switch_Info.Switch_Type;
    pdu->msg[4] = Temp_Switch_Info.Voice_Type;

    rrc_adp->udp_.target_beam_trans_queue.try_push(std::move(pdu));
    std::cout << "target_beam_trans_queue size=" << rrc_adp->udp_.target_beam_trans_queue.size() << std::endl;

    if (!mac->wx_Switch_SetUser_in_TargetBeam())
    {
      std::cout << "[TARGET][BEAM][ADD_USER][FAILURE]" << std::endl;
    }

    auto user_it = users.find(rnti);
    if (user_it == users.end())
    {
      logger.warning("Unrecognised rnti: 0x%x", rnti);
    }

    /**
     * 建立SRB承载
     */
    user_it->second->wx_Set_SRB();
    /**
     * 更新下层参数配置
     */
    if (Temp_Switch_Info.Switch_Type == Data_Service_Switch)
    {
      user_it->second->wx_Update_TargetBeam_Data_Config(); //? IF Voice ,read file?
    }
    else if (Temp_Switch_Info.Switch_Type == Voice_Service_Switch)
      user_it->second->wx_Update_TargetBeam_Voice_Config(); //? IF Voice ,read file?
  }

  void rrc::wx_Handle_switch_ACK(uint16_t rnti,
                                 srsran::unique_byte_buffer_t pdu_)
  {
    std::cout << "[RRC][s_handle_ho_ack]" << std::endl;
    auto user_it = users.find(rnti);
    if (user_it == users.end())
    {
      logger.warning("Unrecognised rnti: 0x%x", rnti);
      return;
    }

    //**************************
    // 双波束切换参数赋值
    rrcSwitchParameters.Beam_Header = pdu_->msg[0];
    rrcSwitchParameters.Beam_Type = pdu_->msg[1];
    rrcSwitchParameters.Beam_InfoType = pdu_->msg[2];
    rrcSwitchParameters.Switch_Type = pdu_->msg[3];
    rrcSwitchParameters.Voice_Type = pdu_->msg[4];

    std::cout << "[RRC][RRCSWITCHPARAMETERS][BEAM_INFOTYPE]" << std::endl;
    std::cout << "rrcSwitchParameters.Beam_Header" << (int)rrcSwitchParameters.Beam_Header << std::endl;
    std::cout << "rrcSwitchParameters.Beam_Type" << (int)rrcSwitchParameters.Beam_Type << std::endl;
    std::cout << "rrcSwitchParameters.Beam_InfoType" << (int)rrcSwitchParameters.Beam_InfoType << std::endl;
    std::cout << "rrcSwitchParameters.Switch_Type" << (int)rrcSwitchParameters.Switch_Type << std::endl;
    std::cout << "rrcSwitchParameters.Voice_Type" << (int)rrcSwitchParameters.Voice_Type << std::endl;
    //*************************

    if (rrcSwitchParameters.Switch_Type == Data_Service_Switch)
    {
      std::cout << "Switch_Type_ == Data_Service_Switch" << std::endl;
      user_it->second->wx_Sourcebeam_send_RRC_Switch_Data_Reconfig({});
    }
    else if (rrcSwitchParameters.Switch_Type == Voice_Service_Switch)
    {
      std::cout << "Switch_Type_ == Voice_Service_Switch" << std::endl;
      user_it->second->wx_Sourcebeam_send_RRC_Switch_Voice_Reconfig({});
    }
  }

  int rrc::s_notify_ue_erab_updates(uint16_t rnti, srsran::const_byte_span nas_pdu)
  {
    auto user_it = users.find(rnti);
    if (user_it == users.end())
    {
      logger.warning("Unrecognised rnti: 0x%x", rnti);
      return SRSRAN_ERROR;
    }
    user_it->second->send_connection_reconf(nullptr, false, nas_pdu);
    return SRSRAN_SUCCESS;
  }

  void rrc::s_set_aggregate_max_bitrate(uint16_t rnti, const asn1::s1ap::ue_aggregate_maximum_bitrate_s &bitrate)
  {
    auto user_it = users.find(rnti);
    if (user_it == users.end())
    {
      logger.warning("Unrecognised rnti: 0x%x", rnti);
      return;
    }
    user_it->second->set_bitrates(bitrate);
  }
  //--------------------------------------------------------------------------------

  void rrc::Notify_rrc_to_notify_nas_to_release(uint16_t rnti)
  {
    std::cout << " cnw->rrc_notify_nas_to_release(rnti) " << std::endl;
    // cnw->rrc_notify_nas_to_release(rnti);
    send_rrc_notify_nas_to_release(rnti);
  }

  bool rrc::Notify_rrc_release_rach(uint16_t rnti){
    rem_user(rnti);
    return true;
  }


  void rrc::wx_Source_Beam_release(uint16_t rnti)
  {
    std::cout << "wx_Source_Beam_release 1111" << std::endl;
    rem_user(rnti);
    std::cout << "wx_Source_Beam_release" << std::endl;
  }

  bool rrc::send_rrc_notify_nas_to_release(uint16_t rnti)
  {
    srsran::unique_byte_buffer_t enb_pdu = srsran::make_byte_buffer();
    srsepc::enb_msg_header_t s1_header;
    s1_header.enb_id = rrc_adp->udp_.pid;
    s1_header.rnti = 70;
    s1_header.msg_type = srsepc::enb_msg_type::notify_send_user_release;

    int len = sizeof(s1_header);

    memcpy(enb_pdu->msg, &s1_header, len);
    enb_pdu->N_bytes = len;

    rrc_adp->udp_.send_msg_to_cnw(std::move(enb_pdu));

    return true;
  }

  int rrc::TC_6115_Reconfig(uint16_t rnti)
  {
    auto user_it = users.find(rnti);
    if (user_it == users.end())
    {
      logger.warning("Unrecognised rnti: 0x%x", rnti);
      return SRSRAN_ERROR;
    }

    user_it->second->send_rrc_con_reconf_data(rnti,2,1,0,{},{});
    return SRSRAN_SUCCESS;
  }
  int rrc::TC_6116_Reconfig(uint16_t rnti)
  {
    auto user_it = users.find(rnti);
    if (user_it == users.end())
    {
      logger.warning("Unrecognised rnti: 0x%x", rnti);
      return SRSRAN_ERROR;
    }

    user_it->second->send_rrc_con_reconf_data(rnti,2,1,0,{},{});
    return SRSRAN_SUCCESS;
  }
  int rrc::TC_6117_Reconfig(uint16_t rnti)
  {
    auto user_it = users.find(rnti);
    if (user_it == users.end())
    {
      logger.warning("Unrecognised rnti: 0x%x", rnti);
      return SRSRAN_ERROR;
    }

    user_it->second->send_rrc_con_reconf_data(rnti,2,1,0,{},{});
    return SRSRAN_SUCCESS;
  }

  int rrc::TC_628_Reconfig(uint16_t rnti)
  {
    auto user_it = users.find(rnti);
    if (user_it == users.end())
    {
      logger.warning("Unrecognised rnti: 0x%x", rnti);
      return SRSRAN_ERROR;
    }

    user_it->second->wx_Sourcebeam_send_RRC_Switch_Data_Reconfig({});
    return SRSRAN_SUCCESS;
  }

} // namespace srsenb
