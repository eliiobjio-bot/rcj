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

#ifndef SRSRAN_RRC_UE_H
#define SRSRAN_RRC_UE_H

#include "mac_controller.h"
#include "rrc.h"
#include "srsran/adt/pool/batch_mem_pool.h"
#include "srsran/asn1/rrc/uecap.h"
#include "srsran/interfaces/enb_phy_interfaces.h"
#include "srsran/interfaces/pdcp_interface_types.h"
#include "srscnw/hdr/udp.h"

namespace srsenb
{

  class rrc::ue
  {
  public:
    // //----------------2023/09/28----------------
    srsran::rrc_pcap_net *getPcapNet(rrc &outer)
    {
      // InnerClass可以直接访问OuterClass的成员变量
      return outer.p_pcap_net;
    }

    class rrc_mobility;
    class rrc_endc;
    enum activity_timeout_type_t
    {
      MSG3_RX_TIMEOUT = 0,   ///< Msg3 has its own timeout to quickly remove fake UEs from random PRACHs
      UE_INACTIVITY_TIMEOUT, ///< UE inactivity timeout (usually bigger than reestablishment timeout)
      MSG5_RX_TIMEOUT,       ///< UE timeout for receiving RRCConnectionSetupComplete / RRCReestablishmentComplete
      nulltype
    };
    std::mutex metrics_mutex;
    ue(rrc *outer_rrc, uint16_t rnti, const sched_interface::ue_cfg_t &ue_cfg);
    ~ue();
    int init();
    bool is_connected();
    bool is_idle();

    int ultrans_number = 0; // 12.1  zhj

    uint16_t ue_cap14_band_id;
    uint16_t ue_cap14_freq_id;
    uint16_t ue_cap14_slot;

    configMap DRB_configMap;
    configMap SRB_configMap;

    /*
     *@ read file Info
     */
    bool wx_read_enb_recfg_data_reconfig(const std::string &filename);

    bool wx_read_data_reconfig(const std::string &filename);
    bool wx_read_data_DS_reconfig(const std::string &filename);
    bool wx_read_voice_reconfig(const std::string &filename);
    bool wx_read_voice_DS_reconfig(const std::string &filename);
    bool wx_SourceBeam_read_reconfig(const std::string &filename);
    void wx_target_Beam_notify_Source_Beam_To_Release();
    bool wx_SourceBeam_to_read_TargetBeam_reconfig_normal(const std::string &filename,asn1::rrc::rrc_con_recfg_r1_ies_s& rrc_con_); 
    bool wx_SourceBeam_to_read_TargetBeam_reconfig_sp(const std::string &filename,asn1::rrc::rrc_con_recfg_r1_ies_s& rrc_con_); 
    bool wx_SourceBeam_to_read_TargetBeam_reconfig_Voice(const std::string &filename,asn1::rrc::rrc_con_recfg_r1_ies_s& rrc_con_); 
    bool wx_SourceBeam_to_read_TargetBeam_reconfig_Voice_Ds(const std::string &filename,asn1::rrc::rrc_con_recfg_r1_ies_s& rrc_con_); 
    mcs_Info RRC_mcs_Info;
    int multi_cc_num_cc;
    uint8_t rlc_mode =1;
    bool is_rlc_mode_change = false;
    /*
     *@ muti carrir
     */
    struct ChannelConfig
    {
      int band_id;
      int freq_id;
      std::string chan_type;
      std::string direc_t;
      int slot_ass;
      std::string sche_type;
    };
    

    std::string to_string(const activity_timeout_type_t &type);
    void set_activity_timeout(activity_timeout_type_t type);
    void set_activity(bool enabled = true);
    void set_radiolink_dl_state(bool crc_res);
    void set_radiolink_ul_state(bool crc_res);
    void activity_timer_expired(const activity_timeout_type_t type);
    void rlf_timer_expired(uint32_t timeout_id);
    void max_rlc_retx_reached();
    void protocol_failure();
    void deactivate_bearers() { mac_ctrl.set_radio_bearer_state(mac_lc_ch_cfg_t::IDLE); }

    // Init PUCCH resources for PCell
    bool init_pucch();

    rrc_state_t get_state();
    void get_metrics(rrc_ue_metrics_t &ue_metrics) const;

    ///< Helper to access a cell cfg based on ue_cc_idx
    enb_cell_common *get_ue_cc_cfg(uint32_t ue_cc_idx);

    /// List of results a RRC procedure may produce.
    enum class procedure_result_code
    {
      none,
      activity_timeout,
      error_mme_not_connected,
      error_unknown_rnti,
      radio_conn_with_ue_lost,
      msg3_timeout,
      fail_in_radio_interface_proc,
      unspecified
    };

    struct RRC_con_qeq_info
    {
      uint8_t direction;
      uint8_t rec_lay_id;
      uint8_t des_layer_id;
      uint16_t test_id;
      uint16_t data_length;

      uint8_t msgType;
      uint8_t establishmentCause;
    };
    RRC_con_qeq_info rrc_con_qeq_info_;

    struct RRC_con_rej_info
    {
      uint8_t direction;
      uint8_t rec_lay_id;
      uint8_t des_layer_id;
      uint16_t test_id;
      uint16_t data_length;
      uint8_t msgType;
      bool redirectPresent;
    };
    RRC_con_rej_info rrc_con_rej_info_;

    void enble_dl_allocate_ue();

    //----------------2022/08/04----------------
    void send_rrc_con_setup();
    void send_rrc_con_setup_ttcn();
    void send_rrc_con_setup_ttcns();
    //-----------------------------------------

    //----------------IOT-2023/9/13-------------
    void iot_send_rrc_con_setup();
    //------------------------------------------

    void assemble_header_req(srsran::unique_byte_buffer_t *
                                 rrc_con_req_ttcn_); // To the header of the data
                                                     // group to be sent to TTCN
    void assemble_header_com(srsran::unique_byte_buffer_t *rrc_con_com_ttcn_);

    void send_connection_setup();
    void send_connection_reest(uint8_t ncc);
    void send_connection_reject(procedure_result_code cause);
    void send_connection_release();
    void s_send_connection_release();
    void TTCN_connection_release();
    void s_send_connection_release_reg_req_congestion();
    void s_send_connection_release_reg_ss_no5Gguti();

    //------------2024.8.16-------------------------------------------------------
    void s_handle_rrc_con_reest_req(asn1::rrc::rrc_con_reest_req_s* msg);
    void s_send_connection_reest();
    void s_handle_rrc_con_reest_complete(asn1::rrc::rrc_con_reest_comp_s* msg, srsran::unique_byte_buffer_t pdu);
    //------------------2024/8/16--------------------
    uint16_t s_rnti=0;
    uint8_t freq_id=0;
    uint16_t band_id=0;
    uint64_t short_mac_I_e=0;
    uint32 trig_beam_id=0;
    asn1::rrc::reest_caus_e        reest_cause;

    void send_connection_reest_rej(procedure_result_code cause);
    //-------------2024/03/19----xxk--//
    void send_con_reject_ttcn();
    //-------------------------------//
    void send_connection_reconf(srsran::unique_byte_buffer_t sdu = {},
                                bool phy_cfg_updated = true,
                                srsran::const_byte_span nas_pdu = {});

    //-----------------------------2023.12.14--------------------------------------
    // void send_rrc_con_reconf(uint16_t rnti,
    //                          uint8_t qos,
    //                          uint16_t pdu_session_id,
    //                          srsran::unique_byte_buffer_t sdu = {},srsran::const_byte_span nas_pdu = {});

    void send_rrc_con_reconf(
        uint16_t rnti,
        uint8_t qfi_nas,
        uint16_t pdu_ses_id,
        int am_tm_type,
        srsran::unique_byte_buffer_t pdu,
        srsran::const_byte_span nas_pdu);
    //sum TC_713_reconfig_DRB reconf number
    uint8_t TC_713_reconfig_number =0;
    uint8_t TC_714_reconfig_number =0;
    uint8_t TC_722_measure_report_number = 0;
   
    void send_rrc_measure_reconf();
    void TC_722_handle_measure_report();
    void handle_measure_report(asn1::rrc::measure_rep_s *msg);
    void send_rrc_con_voice_reconf(
        uint16_t rnti,
        uint8_t qfi_nas,
        uint16_t pdu_ses_id,
        srsran::unique_byte_buffer_t pdu,
        srsran::const_byte_span nas_pdu);

    // test change channel
    void send_rrc_con_reconf_data(
        uint16_t rnti,
        uint8_t qfi_nas,
        uint16_t pdu_ses_id,
        int am_tm_type,
        srsran::unique_byte_buffer_t pdu,
        srsran::const_byte_span nas_pdu);

    // test change channel
    void send_rrc_con_reconf_kuopin_data(
        uint16_t rnti,
        uint8_t qfi_nas,
        uint16_t pdu_ses_id,
        int am_tm_type,
        srsran::unique_byte_buffer_t pdu,
        srsran::const_byte_span nas_pdu);

    //------------------------------------------------------------------------------
    // 6_27 ho
    bool is_ho_complete = false;
    void wx_Switch_Dataing_or_NBusness();
    void wx_Switch_Voicing();

   // asn1::rrc::phy_chan_cfg_s::voice_type_opts::options Voice_Type_record;
  asn1::enumerated<asn1::rrc::phy_chan_cfg_s::voice_type_opts>  Voice_Type_record;

    void wx_Sourcebeam_send_RRC_Switch_Data_Reconfig(srsran::unique_byte_buffer_t pdu);
    void wx_Sourcebeam_send_RRC_Switch_Voice_Reconfig(srsran::unique_byte_buffer_t pdu);
    void wx_Update_TargetBeam_Data_Config();
    void wx_Update_TargetBeam_Voice_Config();
    void wx_Set_SRB();
    //------------------------------------------------------------------

    void send_security_mode_command();
    //------------------------2023/11/1-------------------
    void s_send_security_mode_command();
    //----------------------------------------------------
    void send_ue_cap_enquiry(const std::vector<asn1::rrc::rat_type_opts::options> &rats);
    void send_ue_info_req();

    void parse_ul_dcch(uint32_t lcid, srsran::unique_byte_buffer_t pdu);

    /// List of generated RRC events.
    enum class rrc_event_type
    {
      con_request,
      con_setup,
      con_setup_complete,
      con_reconf,
      con_reconf_complete,
      con_reest_req,
      con_reest,
      con_reest_complete,
      con_reest_reject,
      con_reject,
      con_release
    };

    //-----------------------------------------------------------------------
    void handle_rrc_con_req_s(asn1::rrc::rrc_con_req_s *msg);
    void handle_rrc_con_req_s_ttcn(asn1::rrc::rrc_con_req_s *msg);
    void parse_ul_dcch_s(uint32_t lcid, srsran::unique_byte_buffer_t pdu);
    void parse_ul_dcch_s_ttcn(uint32_t lcid, srsran::unique_byte_buffer_t pdu);
    //-----------------------------------------------------------------------
    //-------------------------IOT-2023/9/13---------------------------------
    void iot_handle_rrc_con_req_s(asn1::rrc::iot_rrc_setup_req_s *msg);
    void iot_parse_ul_dcch_s(uint32_t lcid, srsran::unique_byte_buffer_t pdu);
    //-----------------------------------------------------------------------
    void handle_rrc_con_req(asn1::rrc::rrc_conn_request_s *msg);
    void handle_rrc_con_setup_complete(asn1::rrc::rrc_conn_setup_complete_s *msg, srsran::unique_byte_buffer_t pdu);
    void handle_rrc_con_reest_req(asn1::rrc::rrc_conn_reest_request_s *msg);
    void handle_rrc_con_reest_complete(asn1::rrc::rrc_conn_reest_complete_s *msg, srsran::unique_byte_buffer_t pdu);
    // ##############################
    void handle_rrc_con_setup_comp(asn1::rrc::rrc_con_setup_comp_s *msg, srsran::unique_byte_buffer_t pdu);
    // ##################################
    //------------------IOT--------------
    void iot_handle_rrc_con_setup_comp(asn1::rrc::iot_rrc_setup_complete_s *msg, srsran::unique_byte_buffer_t pdu);
    //-----------------------------------

    //--------------------------2023.10.17--------------------------------------------
    void s_handle_rrc_reconf_complete(asn1::rrc::rrc_con_recon_comp_s *msg, srsran::unique_byte_buffer_t pdu);
    //-------------------------------------------------------------------------------------------

    void handle_rrc_reconf_complete(asn1::rrc::rrc_conn_recfg_complete_s *msg, srsran::unique_byte_buffer_t pdu);
    void handle_security_mode_complete(asn1::rrc::security_mode_complete_s *msg);
    void handle_security_mode_failure(asn1::rrc::security_mode_fail_s *msg);
    //------------------------------------------------2023/10/12-------------------------------------------------------//
    void s_handle_security_mode_complete(asn1::rrc::security_mode_comp_s *msg);
    void s_handle_security_mode_failure(asn1::rrc::secur_mode_fail_s *msg);
    //-------------------------------------------------------------------------------------
    int handle_ue_cap_info(asn1::rrc::ue_cap_info_s *msg);
    void handle_ue_init_ctxt_setup_req(const asn1::s1ap::init_context_setup_request_s &msg);
    //-----------------------2023/11/1---------------------------
    void s_handle_ue_init_ctxt_setup_req(const asn1::s1ap::init_ctxt_setup_req_s &msg);
    bool s_handle_ue_ctxt_mod_req(const asn1::s1ap::ctxt_mod_req_s &msg);
    //----------------------------------------------------------
    bool handle_ue_ctxt_mod_req(const asn1::s1ap::ue_context_mod_request_s &msg);
    void handle_ue_info_resp(const asn1::rrc::ue_info_resp_r9_s &msg, srsran::unique_byte_buffer_t pdu);

    void set_bitrates(const asn1::s1ap::ue_aggregate_maximum_bitrate_s &rates);

    /// Helper to check UE ERABs
    bool has_erab(uint32_t erab_id) const { return bearer_list.get_erabs().count(erab_id) > 0; }
    int get_erab_addr_in(uint16_t erab_id, transp_addr_t &addr_in, uint32_t &teid_in) const;

    bool release_erabs();
    int release_erab(uint32_t erab_id);
    int setup_erab(uint16_t erab_id,
                   const asn1::s1ap::erab_level_qos_params_s &qos_params,
                   srsran::const_span<uint8_t> nas_pdu,
                   const asn1::bounded_bitstring<1, 160, true, true> &addr,
                   uint32_t gtpu_teid_out,
                   asn1::s1ap::cause_c &cause);
    int modify_erab(uint16_t erab_id,
                    const asn1::s1ap::erab_level_qos_params_s &qos_params,
                    srsran::const_span<uint8_t> nas_pdu,
                    asn1::s1ap::cause_c &cause);

    // Getters for PUCCH resources
    int get_cqi(uint16_t *pmi_idx, uint16_t *n_pucch, uint32_t ue_cc_idx);
    int get_ri(uint32_t m_ri, uint16_t *ri_idx);
    bool is_allocated() const;
    bool is_crnti_set() const { return mac_ctrl.is_crnti_set(); }

    /**
     * Sends the CCCH message to the underlying layer and optionally encodes it as an octet string if a valid string
     * pointer is passed.
     */
    void send_s_dl_ccch(asn1::rrc::s_dl_ccch_msg_s *dl_ccch_msg, std::string *octet_str = nullptr);
    void send_dl_ccch(asn1::rrc::dl_ccch_msg_s *dl_ccch_msg, std::string *octet_str = nullptr);
    //------------------------------IOT-2023/9/14--------------------------------------------------
    void iot_send_s_dl_ccch(asn1::rrc::iot_dl_ccch_msg_s *iot_dl_ccch_msg, std::string *octet_str = nullptr);
    //---------------------------------------------------------------------------------------------

    /**
     * Sends the DCCH message to the underlying layer and optionally encodes it as an octet string if a valid string
     * pointer is passed.
     */
    bool send_dl_dcch(const asn1::rrc::dl_dcch_msg_s *dl_dcch_msg,
                      srsran::unique_byte_buffer_t pdu = srsran::unique_byte_buffer_t(),
                      std::string *octet_str = nullptr);

    //--------------------------2023/10/31------------------------------

    bool s_sends_dl_dcch(const asn1::rrc::s_dl_dcch_msg_s *s_dl_dcch_msg,
                         srsran::unique_byte_buffer_t pdu = srsran::unique_byte_buffer_t(),
                         std::string *octet_str = nullptr);

    bool s_send_dl_dcch(const asn1::rrc::s_dl_dcch_msg_s *s_dl_dcch_msg,
                        srsran::unique_byte_buffer_t pdu = srsran::unique_byte_buffer_t(),
                        std::string *octet_str = nullptr);
    bool iot_send_dl_dcch(const asn1::rrc::iot_dl_dcch_msg_s *dl_dcch_msg,
                          srsran::unique_byte_buffer_t pdu = srsran::unique_byte_buffer_t(),
                          std::string *octet_str = nullptr);
    //------------------------------------------------------------------
    void save_ul_message(srsran::unique_byte_buffer_t pdu) { last_ul_msg = std::move(pdu); }

    //---------------------------------------
    void save_ul_message_s(srsran::unique_byte_buffer_t pdu) { last_ul_msg_s = std::move(pdu); }
    //---------------------------------------
    //-------------IOT-2023/9/13-------------
    void iot_save_ul_message_s(srsran::unique_byte_buffer_t pdu) { iot_last_ul_msg_s = std::move(pdu); }
    //---------------------------------------
    const ue_cell_ded_list &get_cell_list() const { return ue_cell_list; }

    uint16_t rnti = 0;
    rrc *parent = nullptr;

    bool connect_notified = false;
    unique_rnti_ptr<rrc_mobility> mobility_handler;
    unique_rnti_ptr<rrc_endc> endc_handler;

    bool is_csfb = false;

  private:
    srsran::unique_timer activity_timer; // for basic DL/UL activity timeout

    /// Radio link failure handling uses distinct timers for PHY (DL and UL) and RLC signaled RLF
    srsran::unique_timer phy_dl_rlf_timer; // can be stopped through recovered DL activity
    srsran::unique_timer phy_ul_rlf_timer; // can be stopped through recovered UL activity
    srsran::unique_timer rlc_rlf_timer;    // can only be stoped through UE reestablishment

    /// cached ASN1 fields for RRC config update checking, and ease of context transfer during HO
    ue_var_cfg_t current_ue_cfg;
    //---------------2023.10.13----
    s_ue_var_cfg_t cur_ue_cfg;
    //--------------------------------
    asn1::rrc::establishment_cause_e establishment_cause;

    //---------------------------------------------------
    asn1::rrc::estab_cause_e estab_cause;
    bool has_nr_s_tmsi = false;
    bool has_random_value = false;
    uint64_t nr_s_tmsi = 0; // ��������nr_s_tmsiΪ24λ
    uint64_t random_value;
    uint8_t rrc_t_id = 0;
    uint8_t ue_category = 0;
    int    srnti = 0;

    //---------------------------------------------------
    //------------------IOT-2023/9/14--------------------
    asn1::rrc::iot_estableishment_cause_e iot_estab_cause;
    bool has_iot_5g_s_tmsi = false;
    uint64_t iot_5g_s_tmsi = 0;
    //---------------------------------------------------

    // S-TMSI for this UE
    bool has_tmsi = false;
    uint32_t m_tmsi = 0;
    uint8_t mmec = 0;

    // state
    uint32_t rlf_cnt = 0;
    uint8_t transaction_id = 0;
    rrc_state_t state = RRC_STATE_IDLE;
    std::map<uint16_t, srsran::pdcp_lte_state_t> old_reest_pdcp_state = {};
    bool rlf_info_pending = false;

    asn1::s1ap::ue_aggregate_maximum_bitrate_s bitrates;
    bool eutra_capabilities_unpacked = false;
    asn1::rrc::ue_eutra_cap_s eutra_capabilities;
    srsran::rrc_ue_capabilities_t ue_capabilities;

    const static uint32_t UE_PCELL_CC_IDX = 0;

    // consecutive KO counter for DL and UL
    uint32_t consecutive_kos_dl = 0;
    uint32_t consecutive_kos_ul = 0;

    ue_cell_ded_list ue_cell_list;
    bearer_cfg_handler bearer_list;
    security_cfg_handler ue_security_cfg;

    void set_as_security_cfg()
    {
      std::cout << "dksjfhufhfh" << std::endl;
      ue_security_cfg.sec_cfg.integ_algo = parent->rrc_adp->udp_.asintegrity_algo;
      ue_security_cfg.sec_cfg.cipher_algo = parent->rrc_adp->udp_.asencryption_algo;
      std::cout << "dksjfhufhfh" << parent->rrc_adp->udp_.asintegrity_algo << " " << parent->rrc_adp->udp_.asencryption_algo << std::endl;
    }

    /// Cached message of the last uplink message.
    srsran::unique_byte_buffer_t last_ul_msg;

    ///------------------------------------------------
    srsran::unique_byte_buffer_t last_ul_msg_s;
    //-------------------------------------------------

    ///------------------IOT-2023/9/13-----------------
    srsran::unique_byte_buffer_t iot_last_ul_msg_s;
    //-------------------------------------------------
    /// Connection release result.
    procedure_result_code con_release_result = procedure_result_code::none;

    // controllers
    mac_controller mac_ctrl;

    /// Helper to fill cell_ded_list with SCells provided in the eNB config
    void update_scells();

    ///< UE's Physical layer dedicated configuration
    phy_interface_rrc_lte::phy_rrc_cfg_list_t phy_rrc_dedicated_list = {};

    /**
     * Setups the PCell physical layer common configuration of the UE from the SIB2 message. This methods is designed to
     * be called from the constructor.
     *
     * @param config ASN1 Common SIB struct carrying the common physical layer parameters
     */
    void apply_setup_phy_common(const asn1::rrc::rr_cfg_common_sib_s &config, bool update_phy);

    /**
     * Setups the PCell physical layer dedicated configuration of the UE. This method shall be called from the
     * connection setup only.
     *
     * @param phys_cfg_ded ASN1 Physical layer configuration dedicated
     */
    void apply_setup_phy_config_dedicated(const asn1::rrc::phys_cfg_ded_s &phys_cfg_ded);

    /**
     * Reconfigures the PCell and SCell physical layer dedicated configuration of the UE. This method shall be called
     * from the connection reconfiguration. `apply_setup_phy_config` shall not be called before/after. It automatically
     * parses the PCell and SCell reconfiguration.
     *
     * @param reconfig_r8 ASN1 reconfiguration message
     */
    void apply_reconf_phy_config(const asn1::rrc::rrc_conn_recfg_r8_ies_s &reconfig_r8, bool update_phy);
    //----------------------------2023.10.24------------------------------
    void s_apply_reconf_phy_config(asn1::rrc::rrc_con_recfg_r1_ies_s &reconfig_r1);
    //-------------------------------------------------------------------------
    /**
     * Reconfigures PDCP bearers
     * @param srbs_to_add SRBs to add
     */

    void xw_apply_rlc_rb_updates(asn1::rrc::redio_resour_cfg_dedi_s &pending_rr_cfg);
    void xw_apply_pdcp_srb_updates();
    void xw_apply_pdcp_drb_updates(asn1::rrc::redio_resour_cfg_dedi_s &pending_rr_cfg);

    //-----------------------------------2023.10.13--------------------------------
    void s_apply_rlc_rb_updates(asn1::rrc::drb_to_add_modi_s &drb_to_add);
    void s_apply_pdcp_srb_updates();
    void s_apply_pdcp_drb_updates(asn1::rrc::drb_to_add_modi_s &drb_to_add);
    //---------------------------------------------------------------------------------
    void apply_pdcp_srb_updates(const asn1::rrc::rr_cfg_ded_s &pending_rr_cfg);
    void apply_pdcp_srb_updates();
    void apply_pdcp_drb_updates(const asn1::rrc::rr_cfg_ded_s &pending_rr_cfg);
    void apply_rlc_rb_updates(const asn1::rrc::rr_cfg_ded_s &pending_rr_cfg);
    void apply_rlc_srb_updates();

    //-----------------IOT--------------
    void iot_apply_rlc_srb_updates();
    //--------------------------------------

    
    bool initial_ue_to_nas(uint16_t rnti, srsran::unique_byte_buffer_t pdu);
    bool write_pdu_to_ims(uint16_t rnti, srsran::unique_byte_buffer_t pdu);
    bool notify_send_reg_or_service_accept(uint16_t rnti);
    bool notify_send_user_release(uint16_t rnti);
    bool write_pdu_to_nas(uint16_t rnti, srsran::unique_byte_buffer_t pdu);
  }; // class ue

} // namespace srsenb

#endif // SRSRAN_RRC_UE_H
