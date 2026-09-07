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

#ifndef SRSENB_RRC_H
#define SRSENB_RRC_H
#include "srsran/asn1/rrc/mib_sib_asn1.h"
#include "rrc_bearer_cfg.h"
#include "rrc_cell_cfg.h"
#include "rrc_metrics.h"
#include "srsenb/hdr/common/common_enb.h"
#include "srsenb/hdr/common/rnti_pool.h"
#include "srsran/adt/circular_buffer.h"
#include "srsran/common/bearer_manager.h"
#include "srsran/common/buffer_pool.h"
#include "srsran/common/common.h"
#include "srsran/common/stack_procedure.h"
#include "srsran/common/task_scheduler.h"
#include "srsran/common/timeout.h"
#include "srsran/interfaces/enb_rrc_interfaces.h"
#include "srsran/interfaces/enb_x2_interfaces.h"
#include "srsran/srslog/srslog.h"
#include "srsenb/hdr/adp.h"
#include "srsran/common/rrc_pcap_net.h"
#define   RACH_HANDOVER    2

#include <map>

//-----------------2023/11/14
#include "srsran/interfaces/cnw_interface_enb.h"
//--------------------------

namespace srsenb
{

  class s1ap_interface_rrc;
  class pdcp_interface_rrc;
  class rlc_interface_rrc;
  class mac_interface_rrc;
  class phy_interface_rrc_lte;

  class paging_manager;

  static const char rrc_state_text[RRC_STATE_N_ITEMS][100] = {"IDLE",
                                                              "WAIT FOR CON SETUP COMPLETE",
                                                              "WAIT FOR SECURITY MODE COMPLETE",
                                                              "WAIT FOR UE CAPABILITIY INFORMATION",
                                                              "WAIT FOR CON RECONF COMPLETE",
                                                              "RRC CONNECTED",
                                                              "RELEASE REQUEST"};

  class rrc final : public rrc_interface_pdcp,
                    public rrc_interface_mac,
                    public rrc_interface_rlc,
                    public rrc_interface_s1ap,
                    public rrc_eutra_interface_rrc_nr,
                    public rrc_interface_cnw
  {
  public:
    explicit rrc(srsran::task_sched_handle task_sched_, enb_bearer_manager &manager_);
    ~rrc();

    int32_t init(const rrc_cfg_t &cfg_,
                 phy_interface_rrc_lte *phy,
                 mac_interface_rrc *mac,
                 rlc_interface_rrc *rlc,
                 pdcp_interface_rrc *pdcp,
                 s1ap_interface_rrc *s1ap,
                 gtpu_interface_rrc *gtpu,
                 adp *rrc_adp_);

    int32_t init(const rrc_cfg_t &cfg_,
                 phy_interface_rrc_lte *phy,
                 mac_interface_rrc *mac,
                 rlc_interface_rrc *rlc,
                 pdcp_interface_rrc *pdcp,
                 s1ap_interface_rrc *s1ap,
                 gtpu_interface_rrc *gtpu,
                 adp *rrc_adp_,
                 rrc_nr_interface_rrc *rrc_nr);

    void stop();
    void get_metrics(rrc_metrics_t &m);
    void tti_clock();
    // ---support pcap net
    void start_pcap_net(srsran::rrc_pcap_net &rrc_net) { p_pcap_net = &rrc_net; };
    //----------------------
    void tti_clock_s();
    //----------------------
    // IOT-2023/9/13---------
    void iot_tti_clock_s();
    //----------------------
    int flag = 0;
    int pcch_num = 0;
    int frame_off = 0;
    
    uint8_t Switch_Voice = 3; // Defalt N_Voice

    // 2024/04/22xxk-------
    void second_configure_mib();
    void second_generate_sib();
    void send_second_release();
    //-----------------------

    int TC_6115_Reconfig(uint16_t rnti);
    int TC_6116_Reconfig(uint16_t rnti);
    int TC_6117_Reconfig(uint16_t rnti);
    int TC_628_Reconfig(uint16_t rnti);

    // ********************切换相关*************************//
    void wx_Trigger_switch_ACK(uint16_t rnti, srsran::unique_byte_buffer_t pdu_);
    void wx_Handle_switch_ACK(uint16_t rnti, srsran::unique_byte_buffer_t pdu_);
    void wx_Source_Beam_release(uint16_t rnti);
    int wx_Mcontrol_Notify_Switch(uint16_t rnti, uint8_t Switch_Type_);
    bool wx_TargetBeam_ReadMib(const std::string &filename);

    bool is_multi=false;




    struct Switch_Info
    {
      uint8_t Beam_Header;
      uint8_t Beam_Type;
      uint8_t Beam_InfoType;
      uint8_t Switch_Type;
      uint8_t Voice_Type;
    };

    Switch_Info rrcSwitchParameters;

    enum Voice_Type
    {
      xxkbps2point4,
      xxkbps4point8,
      xxkbs800b,
      xxN_Voice
    };
    enum Beam_Type
    {
      Source_Beam,
      Target_Beam
    };
    enum Beam_InfoType
    {
      Switch_Request,
      ACK,
      Target_receive_reconfig_complete
    };

    enum wx_Switch_Type
    {
      Data_Service_Switch,
      Voice_Service_Switch
    };
    //----------------------

    // rrc_interface_mac
    int add_user(uint16_t rnti, const sched_interface::ue_cfg_t &init_ue_cfg) override;
    void upd_user(uint16_t new_rnti, uint16_t old_rnti) override;
    void set_activity_user(uint16_t rnti) override;
    void set_radiolink_dl_state(uint16_t rnti, bool crc_res) override;
    void set_radiolink_ul_state(uint16_t rnti, bool crc_res) override;
    bool is_paging_opportunity(uint32_t tti, uint32_t *payload_len) override;
    uint8_t *read_pdu_bcch_dlsch(const uint8_t cc_idx, const uint32_t sib_index) override;

    // V1.0.0###########################################
    bool read_pdu_IoTsi(uint8_t *payload, int &len, uint8_t &tbcchSlotStart_, uint8_t &BandID, uint8_t &FrameID) override;
    bool read_pdu_mib(uint8_t *payload, int &len, uint8_t &bcchSlotStart_, uint8_t &BandID, uint8_t &FrameID, int sfn) override;
    void Notify_rrc_to_notify_nas_to_release(uint16_t rnti) override;
    bool Notify_rrc_release_rach(uint16_t rnti) override;
    bool read_pdu_sib(uint8_t *payload, int &len, sibInfo_t *sibInfo) override;
    bool modify_mib_sdu(uint8_t *mibData, int sfn);

    bool modify_sib_sdu_ttcn(uint8_t *sibData);
    //--------------------------2024.03.16---------------------------------------
    void add_paging_id_wx_s(srsran::unique_byte_buffer_t pdu);
    bool read_pdu_pcch_wx_s(int tti_tx_dl, uint8_t *payload, int &len);

    void start_rem_user(uint16_t rnti);
    void nas_to_notify_rrc_release(uint16_t rnti);
    void start_rem_rel_user(uint16_t rnti);
    void testcase_reg_ss_no5Gguti(uint16_t rnti);

    // #############################################
    void ttcn_control_release_paging(uint16_t rnti);
    void ttcn_handle_in_release_paging();

    // rrc_interface_rlc
    void read_pdu_pcch(uint32_t tti_tx_dl, uint8_t *payload, uint32_t buffer_size) override;
    void max_retx_attempted(uint16_t rnti) override;
    void protocol_failure(uint16_t rnti) override;

    // rrc_interface_cnw
    void write_dl_info(uint16_t rnti, srsran::unique_byte_buffer_t sdu) override;
    //------------------------2023/10/31----------------------------------------
    void s_write_dl_info(uint16_t rnti, srsran::unique_byte_buffer_t sdu) override;
    //-----------------2024.03.05-----------------------
    void s_write_ims_dl_info(uint16_t rnti, srsran::unique_byte_buffer_t sdu);
    //------------------------------------------------------------------------
    void iot_write_dl_info(uint16_t rnti, srsran::unique_byte_buffer_t sdu) override;

    // cnw
    bool send_rrc_notify_nas_to_release(uint16_t rnti);
    bool s_setup_ue_ctxt(uint16_t rnti, const asn1::s1ap::init_ctxt_setup_req_s &msg) override;
    bool s_modify_ue_ctxt(uint16_t rnti, const asn1::s1ap::ctxt_mod_req_s &msg) override;
    void s_release_ue(uint16_t rnti) override;
    void s_add_paging_id(uint32_t ueid, const asn1::s1ap::ue_paging_id_c &ue_paging_id) override;

    int s_setup_erab(uint16_t rnti,
                     uint16_t erab_id,
                     const asn1::s1ap::erab_level_qos_params_s &qos_params,
                     srsran::const_span<uint8_t> nas_pdu,
                     const transp_addr_t &addr,
                     uint32_t gtpu_teid_out,
                     asn1::s1ap::cause_c &cause) override;
    int s_modify_erab(uint16_t rnti,
                      uint16_t erab_id,
                      const asn1::s1ap::erab_level_qos_params_s &qos_params,
                      srsran::const_span<uint8_t> nas_pdu,
                      asn1::s1ap::cause_c &cause) override;
    bool s_release_erabs(uint32_t rnti) override;
    int s_release_erab(uint16_t rnti, uint16_t erab_id) override;

    int smm_notify_ue_erab_updates(uint16_t rnti, uint8_t qos, uint16_t pdu_session_id, srsran::const_byte_span nas_pdu, int am_tm_type); // 12.14
    void rrctorrc_ue(uint16_t rnti);
    int test_smm_notify_ue_erab_updates(uint16_t rnti, uint8_t qos, uint16_t pdu_session_id, srsran::const_byte_span nas_pdu, int am_tm_type);
    // int wx_Mcontrol_Notify_Switch(uint16_t rnti,uint8_t Switch_Type_);

    int s_notify_ue_erab_updates(uint16_t rnti, srsran::const_byte_span nas_pdu) override;
    void s_set_aggregate_max_bitrate(uint16_t rnti, const asn1::s1ap::ue_aggregate_maximum_bitrate_s &bitrate) override;
    void trans_cnw(srsepc::cnw_interface_enb *cnw_) override;
    //--------------------------------------------------------------------------
    void release_ue(uint16_t rnti) override;
    bool setup_ue_ctxt(uint16_t rnti, const asn1::s1ap::init_context_setup_request_s &msg) override;
    bool modify_ue_ctxt(uint16_t rnti, const asn1::s1ap::ue_context_mod_request_s &msg) override;
    bool has_erab(uint16_t rnti, uint32_t erab_id) const override;
    int get_erab_addr_in(uint16_t rnti, uint16_t erab_id, transp_addr_t &addr_in, uint32_t &teid_in) const override;
    void set_aggregate_max_bitrate(uint16_t rnti, const asn1::s1ap::ue_aggregate_maximum_bitrate_s &bitrate) override;
    int setup_erab(uint16_t rnti,
                   uint16_t erab_id,
                   const asn1::s1ap::erab_level_qos_params_s &qos_params,
                   srsran::const_span<uint8_t> nas_pdu,
                   const asn1::bounded_bitstring<1, 160, true, true> &addr,
                   uint32_t gtpu_teid_out,
                   asn1::s1ap::cause_c &cause) override;
    int modify_erab(uint16_t rnti,
                    uint16_t erab_id,
                    const asn1::s1ap::erab_level_qos_params_s &qos_params,
                    srsran::const_span<uint8_t> nas_pdu,
                    asn1::s1ap::cause_c &cause) override;
    bool release_erabs(uint32_t rnti) override;
    int release_erab(uint16_t rnti, uint16_t erab_id) override;
    void add_paging_id(uint32_t ueid, const asn1::s1ap::ue_paging_id_c &ue_paging_id) override;
    void ho_preparation_complete(uint16_t rnti,
                                 rrc::ho_prep_result result,
                                 const asn1::s1ap::ho_cmd_s &msg,
                                 srsran::unique_byte_buffer_t rrc_container) override;
    uint16_t start_ho_ue_resource_alloc(const asn1::s1ap::ho_request_s &msg,
                                        const asn1::s1ap::sourceenb_to_targetenb_transparent_container_s &container,
                                        asn1::s1ap::cause_c &failure_cause) override;
    void set_erab_status(uint16_t rnti, const asn1::s1ap::bearers_subject_to_status_transfer_list_l &erabs) override;

    int notify_ue_erab_updates(uint16_t rnti, srsran::const_byte_span nas_pdu) override;

    // rrc_eutra_interface_rrc_nr
    void sgnb_addition_ack(uint16_t eutra_rnti, const sgnb_addition_ack_params_t params) override;
    void sgnb_addition_reject(uint16_t eutra_rnti) override;
    void sgnb_addition_complete(uint16_t eutra_rnti, uint16_t nr_rnti) override;
    void sgnb_inactivity_timeout(uint16_t eutra_rnti) override;
    void sgnb_release_ack(uint16_t eutra_rnti) override;

    // rrc_interface_pdcp
    void write_pdu(uint16_t rnti, uint32_t lcid, srsran::unique_byte_buffer_t pdu) override;
    void notify_pdcp_integrity_error(uint16_t rnti, uint32_t lcid) override;

    uint32_t get_nof_users();

    // logging
    enum direction_t
    {
      Rx = 0,
      Tx,
      toS1AP,
      fromS1AP
    };
    template <class T>
    void log_rrc_message(const direction_t dir,
                         uint16_t rnti,
                         uint32_t lcid,
                         srsran::const_byte_span pdu,
                         const T &msg,
                         const char *msg_type)
    {
      log_rxtx_pdu_impl(dir, rnti, lcid, pdu, msg_type);
      if (logger.debug.enabled())
      {
        asn1::json_writer json_writer;
        msg.to_json(json_writer);
        logger.debug("Content:\n%s", json_writer.to_string().c_str());
      }
    }
    template <class T>
    void log_broadcast_rrc_message(uint16_t rnti, srsran::const_byte_span pdu, const T &msg, const char *msg_type)
    {
      log_rrc_message(Tx, rnti, -1, pdu, msg, msg_type);
    }
    void get_general_interface(srsran::unique_byte_buffer_t *pdu_);

    bool test_case1 = false;
    adp *rrc_adp;
    class ue;
    static uint32_t wx_network_mode;
    // kuopin
    static uint32_t wx_area_mode;

    typedef enum
    {
      TC_MSG_NULL = 0,
      TC_MSG_RRC_ATE_UE_CATEGORY, // RRC向测管报告UE能力等级
      TC_MSG_RRC_ATE_CON,         // 协议栈向测管报告情况连接状态
      TC_MSG_ATE_RRC_RELEASE,     // 测管对RRC释放进行操作指示
      TC_MSG_ATE_RRC_PAGING,      // 测管对RRC寻呼进行操作指示
      TC_MSG_ATE_RRC_PAGING_RES,  // RRC回复ATE寻呼指令执行完成
      TC_MSG_RRC_ATE_HO_SUCCESS,   // RRC回复ATE切换完成
      TC_MSG_ATE_RRC_MEASURE,	   //主控通知RRC下发带测量报告的重配
      TC_MSG_RRC_ATE_MEASURE_RES,  //RRC回复主控UE生成测量报告
    } TC_RRC_MSG;

    bool rrc_handle_ate_msg(srsran::unique_byte_buffer_t pdu);

  private:
    // args
    srsran::task_sched_handle task_sched;
    enb_bearer_manager &bearer_manager;
    phy_interface_rrc_lte *phy = nullptr;
    mac_interface_rrc *mac = nullptr;
    rlc_interface_rrc *rlc = nullptr;
    pdcp_interface_rrc *pdcp = nullptr;
    gtpu_interface_rrc *gtpu = nullptr;
    s1ap_interface_rrc *s1ap = nullptr;
    rrc_nr_interface_rrc *rrc_nr = nullptr;
    srslog::basic_logger &logger;
    srsran::rrc_pcap_net *p_pcap_net = nullptr;

    // ---------------------2022/7/7--------------------------------
    typedef struct
    {
      uint8_t tbcchSoltStart;
      uint8_t BandID;
      uint8_t FrameID;
    } TbcchInfo_t;
    typedef struct
    {
      uint8_t bcchSoltStart;
      uint8_t BandID;
      uint8_t FrameID;
    } BcchInfo_t;
    typedef struct
    {
      uint8_t bbchSoltCfg;
      uint8_t BandID;
      uint8_t bbchFrameAssinment;
      uint8_t FrameID;
    } BbchInfo_t;
    typedef struct
    {
      uint8_t agchFrameAssinment;
    } AgchInfo_t;
    TbcchInfo_t TbcchInfo;
    BcchInfo_t BcchInfo;
    BbchInfo_t BbchInfo;
    AgchInfo_t AgchInfo;
    typedef struct
    {
      /* data */
      uint8_t *mib_msg;
      uint8_t *sib_msg;
      uint8_t *IoTsi_msg;
      uint8_t *pag_msg;
      //-----22024.03.22-----
      uint8_t *recfg_msg;
      int recfgLen;

      //-----22024.03.22-----
      uint8_t *recfg_voice_msg;
      int recfgvoiceLen;
      //------------------

      //-----22024.03.22-----
      uint8_t *recfg_kuopin_msg;
      int recfgkuopinLen;

      uint8_t *recfg_norm_kuopin_msg;
      int recfgnormkuopinLen;

      int sibLen;
      int mibLen;
      int IoTsiLen;
      int pagLen;
    } si_info_t;
    si_info_t si_info;

    typedef struct
    {
      uint8_t direction;
      uint8_t rec_lay_id;
      uint8_t des_layer_id;
      uint16_t test_id;
      uint16_t data_length;
    } General_interface;
    General_interface RRC_gen_inteface_;

    typedef struct
    {
      uint8_t msgType;
      uint8_t beam_barred;
      uint8_t q_min;
    } RRC_sib_info;
    RRC_sib_info rrc_sib_info_;

    enum RRC_msgType
    {
      RRC_MIB = 1,
      RRC_SIB,
      RRC_PAGING,
      RRC_CONNECT_REQ,
      RRC_CON_SETUP,
      RRC_CON_SETUP_COMP,
      RRC_SEC_MODE_CMD,
      RRC_SEC_MODE_COMP,
      RRC_SEC_MODE_FAIL,
      RRC_CONN_REJ,
      RRC_CON_REEST_REQ,
      RRC_CON_REEST,
      RRC_CON_REEST_COMP,
      RRC_CON_REEST_REJ,
      RRC_CON_RECFG,
      RRC_CON_RECFG_COMP,
      UE_IDENTITY_TRANS,
      UE_INFO_REQ,
      UE_INFO_RESPONSE,
      RRC_UL_INFO_TRANS,
      RRC_DL_INFO_TRANS,
      RRC_MEAS_REPORT,
      UE_GEO_INFO_TRANS,
      RRC_CON_RELEASE
    };
    
    // -------------------------------------------------------------

    uint8_t pcchPdu[7];
    uint8_t pcchPduLen = 0;

    // derived params
    std::unique_ptr<enb_cell_common_list> cell_common_list;

    // state
    std::unique_ptr<freq_res_common_list> cell_res_list;
    std::map<uint16_t, unique_rnti_ptr<ue>> users; // NOTE: has to have fixed addr

    //--------------------------------------------------
    std::map<uint16_t, unique_rnti_ptr<ue>> wx_users;
    //--------------------------------------------------

    std::unique_ptr<paging_manager> pending_paging;

    void process_release_complete(uint16_t rnti);
    void process_release_complete_s(uint16_t rnti);
    /*20240615 xxk add ate control release*/
    void ate_control_release_complete(uint16_t rnti);

    void ate_control_measurement_reconfig(uint16_t rnti);

    void reg_req_congestion_process_release_complete_s(uint16_t rnti);
    void reg_ss_no5Gguti_process_release_complete_s(uint16_t rnti);

    void rem_user(uint16_t rnti);
    uint32_t generate_sibs();
    void configure_mbsfn_sibs();
    int pack_mcch();

    //------------------------
    void configure_mib_wx_ttcn(srsran::unique_byte_buffer_t pdu_);
    void generate_sib_wx_ttcn(srsran::unique_byte_buffer_t pdu_);
    void generate_sib_wx();
    void configure_mib_wx();
    //------------------------

    /*********************************************************************************
     *
     *               IoT Function implementation RRC (2023-08-09)
     *
     *********************************************************************************/
    void generate_iot_si();
    // void generate_pag_si();
    void assemble_general_interface(
        srsran::unique_byte_buffer_t *general_interface_, uint16_t data_len);
    //-------------------------------------------------------------------------------

    //--------2024.03.22--------
    void generate_recfg_wx();
    void generate_recfg_voice();
    void generate_recfg_kuopin();
    void generate_norm_recfg_kuopin();
    //------------------------
    void config_mac();

    //---------------------------------------------------------------------------------
    void parse_ul_dcch_s(ue &ue, uint32_t lcid, srsran::unique_byte_buffer_t pdu);
    void parse_ul_ccch_s(ue &ue, srsran::unique_byte_buffer_t pdu);
    //---------------------------------------------------------------------------------

    //-------------IOT-2023/9/13-------------------------------------------------------
    void iot_parse_ul_dcch_s(ue &ue, uint32_t lcid, srsran::unique_byte_buffer_t pdu);
    void iot_parse_ul_ccch_s(ue &ue, srsran::unique_byte_buffer_t pdu);
    //---------------------------------------------------------------------------------

    void parse_ul_dcch(ue &ue, uint32_t lcid, srsran::unique_byte_buffer_t pdu);
    void parse_ul_ccch(ue &ue, srsran::unique_byte_buffer_t pdu);
    void send_rrc_connection_reject(uint16_t rnti);

    const static int mcch_payload_len = 3000;
    int current_mcch_length = 0;
    uint8_t mcch_payload_buffer[mcch_payload_len] = {};
    struct rrc_pdu
    {
      uint16_t rnti;
      uint32_t lcid;
      uint32_t arg;
      srsran::unique_byte_buffer_t pdu;
    };
    struct nas_rrc_pdu
    {
      uint16_t rnti;
      srsran::unique_byte_buffer_t nas_pdu;
    };
    //----------------------------------------------------
    // struct rrc_pdu_s {
    //   uint16_t                     rnti;
    //   uint32_t                     lcid;
    //   uint32_t                     arg;
    //   srsran::unique_byte_buffer_t pdu;
    // };
    // srsran::dyn_blocking_queue<rrc_pdu_s> rx_pdu_queue_s;

    //--------------------------------------------------

    void log_rx_pdu_fail(uint16_t rnti, uint32_t lcid, srsran::const_byte_span pdu, const char *cause);
    void
    log_rxtx_pdu_impl(direction_t dir, uint16_t rnti, uint32_t lcid, srsran::const_byte_span pdu, const char *msg_type);

    const static uint32_t LCID_EXIT = 0xffff0000;
    const static uint32_t LCID_REM_USER = 0xffff0001;
    const static uint32_t LCID_REL_USER = 0xffff0002;
    const static uint32_t LCID_ACT_USER = 0xffff0004;
    const static uint32_t LCID_RLC_RTX = 0xffff0005;
    const static uint32_t LCID_RADLINK_DL = 0xffff0006;
    const static uint32_t LCID_RADLINK_UL = 0xffff0007;
    const static uint32_t LCID_PROT_FAIL = 0xffff0008;

    bool running = false;
    srsran::dyn_blocking_queue<rrc_pdu> rx_pdu_queue;
    srsran::dyn_blocking_queue<nas_rrc_pdu> nas_rx_pdu_queue;

    asn1::rrc::mcch_msg_s mcch;
    bool enable_mbms = false;
    rrc_cfg_t cfg = {};
    uint32_t nof_si_messages = 0;
    asn1::rrc::sib_type7_s sib7;

    void rem_user_thread(uint16_t rnti);

    //-------------2023/11/14

    srsepc::cnw_interface_enb *cnw;
  };

} // namespace srsenb

#endif // SRSENB_RRC_H
