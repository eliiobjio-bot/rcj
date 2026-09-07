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

#include "srsenb/hdr/stack/mac/sched_interface.h"
#include "srsran/asn1/s1ap_utils.h"
#include "srsran/common/buffer_pool.h" //-------2023/11/14----
#include "srsran/interfaces/cnw_interface_enb.h"
#include "srsran/interfaces/enb_rrc_interface_types.h"

#ifndef SRSRAN_ENB_RRC_INTERFACES_H
#define SRSRAN_ENB_RRC_INTERFACES_H

namespace srsenb
{

  typedef struct
  {
    uint8_t BandID;
    uint8_t bbchFrameAssinment;
    uint8_t agchFrameAssinment;
    uint8_t FrameID;
    int rarWindows;
    uint8_t bbchSlotcfg;
  } sibInfo_t;

  // RRC interface for S1AP
  class rrc_interface_s1ap
  {
  public:
    using failed_erab_list = std::map<uint32_t, asn1::s1ap::cause_c>;

    virtual void write_dl_info(uint16_t rnti, srsran::unique_byte_buffer_t sdu) = 0;
    //--------------------------------------2023/10/31-------------------------------------------------
    virtual void iot_write_dl_info(uint16_t rnti, srsran::unique_byte_buffer_t sdu) = 0;
    //-------------------------------------------------------------------------------------------------
    virtual void release_ue(uint16_t rnti) = 0;
    virtual bool setup_ue_ctxt(uint16_t rnti, const asn1::s1ap::init_context_setup_request_s &msg) = 0;
    virtual bool modify_ue_ctxt(uint16_t rnti, const asn1::s1ap::ue_context_mod_request_s &msg) = 0;
    virtual bool has_erab(uint16_t rnti, uint32_t erab_id) const = 0;
    virtual bool release_erabs(uint32_t rnti) = 0;

    virtual int get_erab_addr_in(uint16_t rnti, uint16_t erab_id, transp_addr_t &addr_in, uint32_t &teid_in) const = 0;
    virtual void set_aggregate_max_bitrate(uint16_t rnti, const asn1::s1ap::ue_aggregate_maximum_bitrate_s &bitrate) = 0;

    /**
     * TS 36.413, 8.2.1 and 8.3.1 - Setup E-RAB / Initial Context Setup
     * @return if error, cause argument is updated with cause
     */
    virtual int setup_erab(uint16_t rnti,
                           uint16_t erab_id,
                           const asn1::s1ap::erab_level_qos_params_s &qos_params,
                           srsran::const_span<uint8_t> nas_pdu,
                           const transp_addr_t &addr,
                           uint32_t gtpu_teid_out,
                           asn1::s1ap::cause_c &cause) = 0;
    /**
     * TS 36.413, 8.2.2 - Modify E-RAB
     * @return if error, cause argument is updated with cause
     */
    virtual int modify_erab(uint16_t rnti,
                            uint16_t erab_id,
                            const asn1::s1ap::erab_level_qos_params_s &qos_params,
                            srsran::const_span<uint8_t> nas_pdu,
                            asn1::s1ap::cause_c &cause) = 0;
    /**
     * TS 36.413, 8.2.3 - Release E-RAB id
     * @return error if E-RAB id or rnti were not found
     */
    virtual int release_erab(uint16_t rnti, uint16_t erab_id) = 0;

    virtual void add_paging_id(uint32_t ueid, const asn1::s1ap::ue_paging_id_c &ue_paging_id) = 0;

    /// TS 36.413, 8.2.1, 8.2.2, 8.2.3 - Notify UE of ERAB updates (done via RRC Reconfiguration Message)
    virtual int notify_ue_erab_updates(uint16_t rnti, srsran::const_span<uint8_t> nas_pdu) = 0;

    /**
     * Reports the reception of S1 HandoverCommand / HandoverPreparationFailure or abnormal conditions during
     * S1 Handover preparation back to RRC.
     *
     * @param rnti user
     * @param is_success true if ho cmd was received
     * @param container TargeteNB RRCConnectionReconfiguration message with MobilityControlInfo
     */
    enum class ho_prep_result
    {
      success,
      failure,
      timeout
    };
    virtual void ho_preparation_complete(uint16_t rnti,
                                         ho_prep_result result,
                                         const asn1::s1ap::ho_cmd_s &msg,
                                         srsran::unique_byte_buffer_t container) = 0;
    virtual uint16_t
    start_ho_ue_resource_alloc(const asn1::s1ap::ho_request_s &msg,
                               const asn1::s1ap::sourceenb_to_targetenb_transparent_container_s &container,
                               asn1::s1ap::cause_c &failure_cause) = 0;
    virtual void set_erab_status(uint16_t rnti, const asn1::s1ap::bearers_subject_to_status_transfer_list_l &erabs) = 0;
  };

  /// RRC interface for RLC
  class rrc_interface_rlc
  {
  public:
    virtual void max_retx_attempted(uint16_t rnti) = 0;
    virtual void protocol_failure(uint16_t rnti) = 0;
    virtual void write_pdu(uint16_t rnti, uint32_t lcid, srsran::unique_byte_buffer_t sdu) = 0;
  };

  /// RRC interface for MAC
  class rrc_interface_mac
  {
  public:
    /* Radio Link failure */
    virtual int add_user(uint16_t rnti, const sched_interface::ue_cfg_t &init_ue_cfg) = 0;
    virtual void upd_user(uint16_t new_rnti, uint16_t old_rnti) = 0;
    virtual void set_activity_user(uint16_t rnti) = 0;
    virtual void set_radiolink_dl_state(uint16_t rnti, bool crc_res) = 0;
    virtual void set_radiolink_ul_state(uint16_t rnti, bool crc_res) = 0;
    virtual bool is_paging_opportunity(uint32_t tti_tx_dl, uint32_t *payload_len) = 0;
    virtual void read_pdu_pcch(uint32_t tti_tx_dl, uint8_t *payload, uint32_t payload_size) = 0;
    ///< Provide packed SIB to MAC (buffer is managed by RRC)
    virtual uint8_t *read_pdu_bcch_dlsch(const uint8_t enb_cc_idx, const uint32_t sib_index) = 0;
    virtual bool read_pdu_mib(uint8_t *payload, int &len, uint8_t &bcchSlotStart_, uint8_t &BandID, uint8_t &FrameID, int sfn) = 0;
    virtual bool read_pdu_sib(uint8_t *payload, int &len, sibInfo_t *sibInfo) = 0;

    virtual bool read_pdu_pcch_wx_s(int tti_tx_dl, uint8_t *payload, int &len) = 0;

    virtual bool read_pdu_IoTsi(uint8_t *payload, int &len, uint8_t &tbcchSlotStart_, uint8_t &BandID, uint8_t &FrameID) = 0;

    virtual void Notify_rrc_to_notify_nas_to_release(uint16_t rnti)=0;

    virtual bool Notify_rrc_release_rach(uint16_t rnti)=0;
  };

  /// RRC interface for PDCP
  class rrc_interface_pdcp
  {
  public:
    virtual void write_pdu(uint16_t rnti, uint32_t lcid, srsran::unique_byte_buffer_t pdu) = 0;
    virtual void notify_pdcp_integrity_error(uint16_t rnti, uint32_t lcid) = 0;
  };

  // RRC interface for CNW
  class rrc_interface_cnw
  {
  public:
    virtual void add_paging_id_wx_s(srsran::unique_byte_buffer_t pdu) = 0;

    virtual void start_rem_user(uint16_t rnti) = 0;
    virtual void start_rem_rel_user(uint16_t rnti) = 0;
    virtual void testcase_reg_ss_no5Gguti(uint16_t rnti) = 0;

    virtual void s_write_dl_info(uint16_t rnti, srsran::unique_byte_buffer_t sdu) = 0;
    //--------------2024.03.05--------------
    virtual void s_write_ims_dl_info(uint16_t rnti, srsran::unique_byte_buffer_t sdu) = 0;
    //---------------------------
    virtual bool s_setup_ue_ctxt(uint16_t rnti, const asn1::s1ap::init_ctxt_setup_req_s &msg) = 0;
    virtual bool s_modify_ue_ctxt(uint16_t rnti, const asn1::s1ap::ctxt_mod_req_s &msg) = 0;
    virtual void s_add_paging_id(uint32_t ueid, const asn1::s1ap::ue_paging_id_c &ue_paging_id) = 0;
    virtual void s_release_ue(uint16_t rnti) = 0;

    virtual int smm_notify_ue_erab_updates(uint16_t rnti, uint8_t qos, uint16_t pdu_session_id, srsran::const_byte_span nas_pdu, int am_tm_type) = 0; // 12.14
    virtual void rrctorrc_ue(uint16_t rnti)=0;
    virtual int test_smm_notify_ue_erab_updates(uint16_t rnti, uint8_t qos, uint16_t pdu_session_id, srsran::const_byte_span nas_pdu, int am_tm_type) = 0; // 12.14

    virtual int s_setup_erab(uint16_t rnti,
                             uint16_t erab_id,
                             const asn1::s1ap::erab_level_qos_params_s &qos_params,
                             srsran::const_span<uint8_t> nas_pdu,
                             const transp_addr_t &addr,
                             uint32_t gtpu_teid_out,
                             asn1::s1ap::cause_c &cause) = 0;

    virtual int s_modify_erab(uint16_t rnti,
                              uint16_t erab_id,
                              const asn1::s1ap::erab_level_qos_params_s &qos_params,
                              srsran::const_span<uint8_t> nas_pdu,
                              asn1::s1ap::cause_c &cause) = 0;
    virtual bool s_release_erabs(uint32_t rnti) = 0;
    virtual int s_release_erab(uint16_t rnti, uint16_t erab_id) = 0;
    virtual int s_notify_ue_erab_updates(uint16_t rnti, srsran::const_byte_span nas_pdu) = 0;

    // virtual int  smm_notify_ue_erab_updates(uint16_t rnti,uint8_t qos,uint16_t pdu_session_id,srsran::const_byte_span nas_pdu)=0;

    virtual void s_set_aggregate_max_bitrate(uint16_t rnti,
                                             const asn1::s1ap::ue_aggregate_maximum_bitrate_s &bitrate) = 0;
    virtual void trans_cnw(srsepc::cnw_interface_enb *cnw_) = 0;

    virtual void nas_to_notify_rrc_release(uint16_t rnti) = 0;

    virtual int wx_Mcontrol_Notify_Switch(uint16_t rnti,uint8_t Switch_Type_) = 0;
  };

} // namespace srsenb

#endif // SRSRAN_ENB_RRC_INTERFACES_H
