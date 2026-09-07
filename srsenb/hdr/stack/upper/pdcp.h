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

#include "srsenb/hdr/common/rnti_pool.h"
#include "srsran/common/timers.h"
#include "srsran/interfaces/enb_metrics_interface.h"
#include "srsran/interfaces/enb_pdcp_interfaces.h"
#include "srsran/interfaces/ue_gw_interfaces.h"
#include "srsran/interfaces/ue_rlc_interfaces.h"
#include "srsran/srslog/srslog.h"
#include "srsran/upper/pdcp.h"
#include <map>

#ifndef SRSENB_PDCP_H
#define SRSENB_PDCP_H

namespace srsenb {

class adp;
class rrc_interface_pdcp;
class rlc_interface_pdcp;
class gtpu_interface_pdcp;

class pdcp : public pdcp_interface_rlc,
             public pdcp_interface_gtpu,
             public pdcp_interface_rrc {
 public:

    // LI
  adp* pdcp_adp;
  int  pdcp_testID = -1;
  int  sdap_testID = -1;
  int  rlc_testID=-1;

  #pragma pack(1)
  struct PDCP_TTCN_pdcp_head{
    uint8_t   msgType;
    uint8_t   lcid[4];
    uint8_t   sduLen[4];
    //uint16_t  pdcpSn;
    //uint16_t  rlcSn;
    //uint32_t  lcid;
    //int       sduLen;
  };

   struct PDCP_TTCN_ttcn_head{
    uint8_t   direction;
    uint8_t   rec_lay_id;
    uint8_t   des_layer_id;
    uint8_t   test_id[2];
    //uint16_t  test_id;
    uint8_t   rnti;
    uint8_t   data_length[2];
    //uint16_t  data_length;
  };

    struct SDAP_TTCN_sdap_head{
    uint8_t   msgType;
    uint8_t   sduLen[2];
    //uint8_t   sdu[3];
    uint8_t   dctype;
    uint8_t   qfi;
    uint8_t   drb_id;
  };

   struct SDAP_TTCN_ttcn_head{
    uint8_t   direction;
    uint8_t   rec_lay_id;
    uint8_t   des_layer_id;
    uint8_t   test_id[2];
    uint8_t   rnti;
    uint8_t   data_length[2];
  };
  #pragma pack()


  uint8_t temp_qfi;
  uint8_t temp_drbid;

    // LI end

  pdcp(srsran::task_sched_handle task_sched_, srslog::basic_logger& logger);
  virtual ~pdcp() {}
  void init(rlc_interface_pdcp* rlc_, rrc_interface_pdcp* rrc_,
            const bool ttcn_pdcp_enble_, gtpu_interface_pdcp* gtpu_, adp* pdcp_adp_);
  void stop();

  // pdcp_interface_rlc
  void write_pdu(uint16_t rnti, uint32_t lcid, srsran::unique_byte_buffer_t sdu) override;
  void notify_delivery(uint16_t rnti, uint32_t lcid, const srsran::pdcp_sn_vector_t& pdcp_sn) override;
  void notify_failure(uint16_t rnti, uint32_t lcid, const srsran::pdcp_sn_vector_t& pdcp_sn) override;
  void write_pdu_mch(uint32_t lcid, srsran::unique_byte_buffer_t sdu) {}

  // pdcp_interface_rrc
  void set_enabled(uint16_t rnti, uint32_t lcid, bool enabled) override;
  void reset(uint16_t rnti) override;
  void add_user(uint16_t rnti) override;
  void rem_user(uint16_t rnti) override;
  void write_sdu(uint16_t rnti, uint32_t lcid, srsran::unique_byte_buffer_t sdu, int pdcp_sn = -1) override;

    //---------------2023.10.20---------------------------------------------------------------------
  void add_bearerer(uint16_t rnti, uint32_t lcid, const srsran::pdcp_confg_t& cnfg) override;
  //------------------------------------------------------------------------
  void add_bearer(uint16_t rnti, uint32_t lcid, const srsran::pdcp_config_t& cnfg) override;
  void del_bearer(uint16_t rnti, uint32_t lcid) override;
  void config_security(uint16_t rnti, uint32_t lcid, const srsran::as_security_config_t& cfg_sec) override;
  void enable_integrity(uint16_t rnti, uint32_t lcid) override;
  void enable_encryption(uint16_t rnti, uint32_t lcid) override;
  bool get_bearer_state(uint16_t rnti, uint32_t lcid, srsran::pdcp_lte_state_t* state) override;
  bool set_bearer_state(uint16_t rnti, uint32_t lcid, const srsran::pdcp_lte_state_t& state) override;
  void send_status_report(uint16_t rnti) override;
  void send_status_report(uint16_t rnti, uint32_t lcid) override;
  void reestablish(uint16_t rnti) override;

	//////////////////		li

  void direct_ttcn_msg(srsran::unique_byte_buffer_t ttcn_pdcp_msg);

  void send_msg_ttcn(uint16_t rnti, uint32_t lcid, srsran::unique_byte_buffer_t pdu_to_ttcn);

  void pdcp_ttcn_test();
  void TC_6115(uint16_t rnti, uint32_t lcid, srsran::unique_byte_buffer_t TC_6115_Info);
  void TC_6116(uint16_t rnti, uint32_t lcid, srsran::unique_byte_buffer_t TC_6116_Info);
  void TC_6117(uint16_t rnti, uint32_t lcid, srsran::unique_byte_buffer_t TC_6117_Info);
  void TC_617(uint16_t rnti, uint32_t lcid, srsran::unique_byte_buffer_t TC_617_Info);
  void TC_6113(uint16_t rnti, uint32_t lcid, srsran::unique_byte_buffer_t TC_6113_Info);

  void write_head_value(uint8_t **msg, uint32_t *N_bytes, size_t size, uint32_t val);
	
  int do_ttcn_00(uint16_t rnti, uint32_t lcid, srsran::unique_byte_buffer_t ttcn_pdcp_msg);

  void rlc_ttcn_msg(uint16_t rlc_rnti, srsran::unique_byte_buffer_t new_rx_sdu, uint16_t ttcn_testId);
  	//////////////////////// li end
  
  void SDAP_TTCN_TEST();

  bool sdap_cfgerer(uint16_t rnti,uint32_t lcid,const srsran::sdap_allocate& sdap_cfg);

  void send_sdap_msg_ttcn(uint16_t rnti, uint32_t lcid, srsran::unique_byte_buffer_t ttcn_sdap_msg);

  int do_sdap_ttcn_00(uint16_t rnti, uint32_t lcid, srsran::unique_byte_buffer_t ttcn_pdcp_msg);
  //void SDAP_TTCN_header(srsran::unique_byte_buffer_t* sdap_ttcn_header);

  //-----------------------------IOT--------------------
  static uint32_t pdcp_network_mode;
  //----------------------------------------------------
  // pdcp_interface_gtpu
  std::map<uint32_t, srsran::unique_byte_buffer_t> get_buffered_pdus(uint16_t rnti, uint32_t lcid) override;

  // Metrics
  void get_metrics(pdcp_metrics_t& m, const uint32_t nof_tti);

    //5.21 wcb
  uint16_t icmp_rnti;

private:
  bool ttcn_pdcp_enble = false;
  class user_interface_rlc : public srsue::rlc_interface_pdcp
  {
  public:
    uint16_t                    rnti;
    srsenb::rlc_interface_pdcp* rlc;
    // rlc_interface_pdcp
    void write_sdu(uint32_t lcid, srsran::unique_byte_buffer_t sdu);
    void discard_sdu(uint32_t lcid, uint32_t discard_sn);
    bool rb_is_um(uint32_t lcid);
    bool sdu_queue_is_full(uint32_t lcid);
    bool is_suspended(uint32_t lcid);
    void pdcp_ttcnmsg_rlc_s(uint16_t ttcn_rnti, uint32_t lcid, srsran::unique_byte_buffer_t ttcn_rlc_msg){std::cout<<"class user_interface_rlc : public srsue::rlc_interface_pdcp"<<std::endl;}
    void enable_rlc_testcase_related_configuration(uint16_t ttcn_rnti, uint32_t lcid,uint16_t rlc_testID,int pid){std::cout<<" Nothing !"<<std::endl;}
    void TC_6115_RLC_Handle(uint16_t ttcn_rnti,uint32_t lcid){std::cout<<"TC6115 dummy!"<<std::endl;}
    void TC_6116_RLC_Handle(uint16_t ttcn_rnti,uint32_t lcid){std::cout<<"TC6116 dummy!"<<std::endl;}
    void TC_6117_RLC_Handle(uint16_t ttcn_rnti,uint32_t lcid){std::cout<<"TC6117 dummy!"<<std::endl;}
    void psch_test(uint16_t rnti, uint32_t lcid, srsran::unique_byte_buffer_t psch_pdu){std::cout<<" NULL NULL "<<std::endl;}
  };

  class user_interface_gtpu : public srsue::gw_interface_pdcp
  {
  public:
    uint16_t                     rnti;
    srsenb::gtpu_interface_pdcp* gtpu;
    // gw_interface_pdcp
    void write_pdu(uint32_t lcid, srsran::unique_byte_buffer_t pdu);
    void write_pdu_mch(uint32_t lcid, srsran::unique_byte_buffer_t sdu) {}
  };

  class user_interface_rrc : public srsue::rrc_interface_pdcp
  {
  public:

    uint16_t                    rnti;
    srsenb::rrc_interface_pdcp* rrc;
    // rrc_interface_pdcp
    void        write_pdu(uint32_t lcid, srsran::unique_byte_buffer_t pdu);
    void        write_pdu_bcch_bch(srsran::unique_byte_buffer_t pdu);
    void        write_pdu_bcch_dlsch(srsran::unique_byte_buffer_t pdu);
    void        write_pdu_pcch(srsran::unique_byte_buffer_t pdu);
    void        write_pdu_mch(uint32_t lcid, srsran::unique_byte_buffer_t pdu) {}
    void        notify_pdcp_integrity_error(uint32_t lcid);
    const char* get_rb_name(uint32_t lcid);
  };

  class user_interface
  {
  public:
    user_interface_rlc            rlc_itf;
    user_interface_gtpu           gtpu_itf;
    user_interface_rrc            rrc_itf;
    unique_rnti_ptr<srsran::pdcp> pdcp;
  };

  void clear_user(user_interface* ue);

  std::map<uint32_t, user_interface> users;

  rlc_interface_pdcp*       rlc  = nullptr;
  rrc_interface_pdcp*       rrc  = nullptr;
  gtpu_interface_pdcp*      gtpu = nullptr;
  srsran::task_sched_handle task_sched;
  srslog::basic_logger&     logger;
};

} // namespace srsenb
#endif // SRSENB_PDCP_H
