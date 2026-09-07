/******************************************************************************
 * File:        nas_mm.h
 * Description: Top-level NAS MM class. Creates and links all
 *              interfaces and helpers.
 *****************************************************************************/

#ifndef SRSEPC_NAS_MM_H
#define SRSEPC_NAS_MM_H

#include "srsran/common/buffer_pool.h"
#include "srsran/common/standard_streams.h"
#include "srsran/common/threads.h"
#include <cstddef>

#include "nas_context.h"
#include "srsran/asn1/nas_5g_ies.h"
#include "srsran/asn1/nas_5g_msg.h"
#include "srscnw/hdr/adp.h"

#include "srsran/interfaces/cnw_interface_enb.h"
#include "lib/include/srsran/interfaces/enb_rrc_interfaces.h" //-------2023/11/14
using namespace srsran;
using namespace nas_5g;

namespace srsepc
{

#define ABBA_LEN 2
#define XRES_LEN 8
#define SEQ_5G_OFFSET 6
#define MAC_5G_OFFSET 2
#define NAS_5G_BEARER 1

  typedef struct
  {
#define NR_NAS_MM_DEREG_NORMAL_DEREGISTRATION 0
#define NR_NAS_MM_DEREG_SWITCHOFF 1
    uint8_t SwitchOff;
#define NR_NAS_MM_DEREG_REGISTRATION_NOT_REQUIRED 0
#define NR_NAS_MM_DEREG_REGISTRATION__REQUIRED 1
    uint8_t RegistrationRequired;
#define NR_NAS_MM_DEREG_ACCESS_3GPP 1
#define NR_NAS_MM_DEREG_ACCESS_NON_3GPP 2
#define NR_NAS_MM_DEREG_ACCESS_3GPP_AND_NON_3GPP 3
    uint8_t AccessType;

    uint8_t MMCauseFlg; /*Exist or not Exist*/
    uint8_t MMCause;

  } DEREG_MSG_INFO;

  typedef struct
  {
    uint8_t MMCause;

  } SERVICE_MSG_INFO;

  typedef struct
  {
#define NR_NAS_MM_IDENTITY_TYPE_SUCI 1
#define NR_NAS_MM_IDENTITY_TYPE_GUTI_5G 2
#define NR_NAS_MM_IDENTITY_TYPE_IMEI 3
#define NR_NAS_MM_IDENTITY_TYPE_S_TMSI_5G 4
#define NR_NAS_MM_IDENTITY_TYPE_IMEISV 5
#define NR_NAS_MM_IDENTITY_TYPE_MAC_ADDRESS 6
#define NR_NAS_MM_IDENTITY_TYPE_EUI_64 7
    uint8_t IdentityType;

  } IDENTITY_MSG_INFO;

  typedef struct
  {
    uint8_t gutiFlag;
    nas_guti guti;

  } CONFIGURATION_UPDATE_COMMAND_MSG_INFO;

  class cnw;
  class nas_sm; // 12.9
  class adp;
  class nas_mm
  {
  public:
    static nas_mm *get_instance();
    static void cleanup();
    void init(srsenb::rrc_interface_cnw *rrc_, adp *mm_adp_, const cnw_args_t mm_args_);

    srslog::basic_logger &m_nas_mm_logger = srslog::fetch_basic_logger("NAS_MM");

    /*handle previous NAS context*/
    bool delete_ue_nas_ctx(uint64_t suci);

    /* message handle functions. */
    bool handle_deregistration_request_from_ue(deregistration_request_ue_originating_t &msg);
    bool handle_deregistration_request_to_ue_ttcn(DEREG_MSG_INFO *DeregMsgInfo);
    bool handle_deregistration_accept_from_ue(uint16_t enb_ue_id);
    bool handle_deregistration_accept_to_ue_ttcn(void);
    bool handle_service_request(service_request_t &msg, uint16_t enb_ue_id);
    bool handle_service_accept_ttcn(SERVICE_MSG_INFO ServiceMsgInfo);
    bool handle_service_reject_ttcn(SERVICE_MSG_INFO ServiceMsgInfo);
    bool handle_registration_request(registration_request_t &msg, uint16_t enb_id, uint16_t cnw_ue_id);
    /*2023-11-21*/
    bool handle_registration_complete(registration_complete_t &msg, uint16_t enb_id, uint16_t cnw_ue_id);
    /*2023-11-21*/
    bool handle_authentication_response(authentication_response_t &msg, uint16_t enb_id, uint16_t cnw_ue_id);
    bool handle_ul_nas_transport(ul_nas_transport_t &msg, uint16_t enb_ue_id);
    bool handle_dl_nas_transport_ttcn(uint16_t enb_ue_id);
    bool handle_security_mode_complete(security_mode_complete_t &msg, uint16_t enb_id, uint16_t cnw_ue_id);
    bool handle_security_mode_reject(security_mode_reject_t &msg, uint16_t enb_id, uint16_t cnw_ue_id);
    bool handle_authentication_failure(authentication_failure_t &msg, uint16_t enb_id, uint16_t cnw_ue_id);
    bool handle_identity_request_ttcn(IDENTITY_MSG_INFO msg);
    bool handle_identity_response(identity_response_t &msg, uint16_t enb_ue_id);
    bool handle_configuration_update_command_ttcn(CONFIGURATION_UPDATE_COMMAND_MSG_INFO msg);
    bool handle_configuration_update_complete(void);

    /* message sender functions. */
    bool pack_identity_request(srsran::unique_byte_buffer_t &nas_buffer, identity_request_t *msg);
    bool pack_authentication_request(srsran::unique_byte_buffer_t &nas_buffer, nas_context *nas_ctx);
    bool pack_authentication_reject(srsran::unique_byte_buffer_t &nas_buffer, nas_context *nas_ctx);
    bool pack_security_mode_command(srsran::unique_byte_buffer_t &nas_buffer, nas_context *nas_ctx);
    bool pack_registration_accept(srsran::unique_byte_buffer_t &nas_buffer, nas_context *nas_ctx);
    bool pack_deregistration_request_to_ue(srsran::unique_byte_buffer_t &nas_buffer,
                                           srsran::nas_5g::deregistration_request_ue_terminated_t *msg);
    bool pack_deregistration_accept_to_ue(srsran::unique_byte_buffer_t &nas_buffer);
    // bool pack_service_accept(srsran::unique_byte_buffer_t& nas_buffer, srsran::nas_5g::service_accept_t* msg);
    bool pack_service_accept(srsran::unique_byte_buffer_t &nas_buffer, nas_context *nas_ctx);
    bool pack_service_reject(srsran::unique_byte_buffer_t &nas_buffer, srsran::nas_5g::service_reject_t *msg);
    bool pack_configuration_update_command(srsran::unique_byte_buffer_t &nas_buffer,
                                           srsran::nas_5g::configuration_update_command_t msg);
    // 12.9
    bool pack_dl_nas_transport(srsran::unique_byte_buffer_t &nas_buffer, srsran::unique_byte_buffer_t &sm_msg, nas_context *nas_ctx);

    /* message handle subfunctions. */
    bool handle_suci_registration_request_unknown_ue(uint64_t suci,
                                                     uint16_t enb_id,
                                                     uint16_t cnw_ue_id,
                                                     registration_type_5gs_t::registration_type_type reg_type,
                                                     registration_request_t &msg);
    bool handle_suci_registration_request_known_ue(nas_context *nas_ctx,
                                                   uint16_t enb_id,
                                                   uint16_t cnw_ue_id,
                                                   registration_type_5gs_t::registration_type_type reg_type,
                                                   registration_request_t &msg);
    bool handle_guti_registration_request_unknown_ue(srsepc::nas_guti guti,
                                                     uint16_t enb_id,
                                                     uint16_t cnw_ue_id,
                                                     registration_type_5gs_t::registration_type_type reg_type,
                                                     registration_request_t &msg);
    bool handle_guti_registration_request_known_ue(nas_context *nas_ctx,
                                                   uint16_t enb_id,
                                                   uint16_t cnw_ue_id,
                                                   registration_type_5gs_t::registration_type_type reg_type,
                                                   registration_request_t &msg);
    bool handle_nas_message_container(nas_context *nas_ctx,
                                      uint16_t enb_id,
                                      uint16_t cnw_ue_id,
                                      msg_types message_type,
                                      message_container_t *message_container);

    /* IE utils func */
    bool struct_mobile_identity_to_uint64(uint64_t *mobile_identity, mobile_identity_5gs_t *t_mobile_identity);
    void print_capability_5gmm(capability_5gmm_t capability_5gmm);
    bool struct_mobile_identity_to_guti(srsepc::nas_guti *guti, mobile_identity_5gs_t *t_mobile_identity);
    void struct_mobile_identity_to_array(uint8_t **mobile_identity,
                                         srsran::nas_5g::mobile_identity_5gs_t *t_mobile_identity);
    void common_5g_mm_cause_set_value(uint8_t MMCause, cause_5gmm_t &cause_5gmm);
    void print_ue_security_capability(ue_security_capability_t ue_security_capability);
    void print_ie_suci(mobile_identity_5gs_t::suci_s &suci);

    // 11-27  41
    void guti_to_mobile_identity_5gs_t(srsran::nas_5g::configuration_update_command_t &nas_msg, srsepc::nas_guti guti);
    // uint8_t tai_list_to_tracking_area_identity_list_5gs_t(srsran::nas_5g::configuration_update_command_t& nas_msg, tai_list tai_List);
    bool compute_supi_from_suci(mobile_identity_5gs_t::suci_s suci, char *c_suci, char *supi);
    // bool struct_suci_to_char(mobile_identity_5gs_t::suci_s suci, char* c_suci);

    /* Security func */
    bool integrity_check(byte_buffer_t *pdu, nas_context *nas_ctx);
    void integrity_generate(uint8_t *key_128,
                            uint32_t count,
                            uint8_t direction,
                            uint8_t *msg,
                            uint32_t msg_len,
                            uint8_t *mac,
                            nas_context *nas_ctx);
    void cipher_encrypt(srsran::unique_byte_buffer_t &pdu, nas_context *nas_ctx);
    void cipher_decrypt(srsran::byte_buffer_t *pdu, nas_context *nas_ctx);
    bool gen_auth_info_answer(nas_context *nas_ctx,
                              uint64_t suci,
                              uint8_t *k_ausf,
                              uint8_t *k_seaf,
                              uint8_t *k_amf,
                              uint8_t *autn,
                              uint8_t *rand,
                              uint8_t *xres);
    void gen_auth_info_answer_milenage(ue_ctx_t *ue_ctx,
                                       uint8_t *k_ausf,
                                       uint8_t *k_seaf,
                                       uint8_t *k_amf,
                                       uint8_t *autn,
                                       uint8_t *rand,
                                       uint8_t *xres);
    void gen_rand(uint8_t rand_[16]);
    uint32_t common_5g_mm_array_to_uint32(uint8_t *args);
    // void guti_to_mobile_identity_5gs_t(srsran::nas_5g::configuration_update_command_t& nas_msg, srsepc::nas_guti guti);

    /*mm interface*/
    bool sm_write_dl_info(srsran::unique_byte_buffer_t sm_msg, uint16_t rnti);
    bool sm_notify_ue_erab_updates(uint16_t rnti, uint8_t qos, uint16_t pdu_session_id, srsran::const_byte_span nas_pdu, int am_tm_type); // 12.14
    bool handle_nas_reg_req_ttcn(uint64_t suci, uint16_t enb_id, uint16_t cnw_ue_id, registration_type_5gs_t::registration_type_type reg_type, registration_request_t &msg);
    bool pack_registration_reject(srsran::unique_byte_buffer_t &nas_buffer, uint8_t gmm_cause);
    void nas_authencation_reponse_message_to_ttcn(authentication_response_t &msg);
    void nas_security_mode_complete_message_to_ttcn(security_mode_complete_t &msg);
    void nas_dereg_req_message_to_ttcn(deregistration_request_ue_originating_t &msg);
    void nas_dereg_acc_message_to_ttcn();

    // 12.9
    //-----------------2023/11/14
    srsenb::rrc_interface_cnw *rrc_mm = nullptr;
    bool nasmm_ttcn_test_enble = false;
    bool ttcn_nasmm_enble = false;
    bool is_congestion_first = true;
    bool is_send_dereq_req_info_first = true;
    bool handle_test_loop_message(nas_5gs_hdr &msg, nas_context *nas_ctx);
    bool pack_activate_test_mode(srsran::unique_byte_buffer_t &nas_buffer, nas_context *nas_ctx, uint16_t enb_ue_id);
    bool pack_close_ue_test_loop(srsran::unique_byte_buffer_t &loop_buffer, nas_context *nas_ctx, uint16_t enb_ue_id);
    // bool pack_close_ue_test_loop(srsran::unique_byte_buffer_t& loop_buffer, nas_context* nas_ctx);
    bool pack_open_ue_test_loop(srsran::unique_byte_buffer_t &loop_buffer, nas_context *nas_ctx, uint16_t enb_ue_id);
    bool pack_deactivate_test_mode(srsran::unique_byte_buffer_t &loop_buffer, nas_context *nas_ctx, uint16_t enb_ue_id);
    bool pcs_sm_modification(uint16_t rnti, uint8_t flag);

    bool send_switch_info_to_ate();

    uint16_t loop_bytes_allocate(uint16_t testID);

    bool send_mm_dl_msg(uint16_t rnti, srsran::unique_byte_buffer_t mm_pdu);
    bool send_start_rem_rel_user(uint16_t rnti);
    bool send_start_rem_user(uint16_t rnti);
    bool send_test_reg_ss_no5Gguti(uint16_t rnti);
    bool send_setup_ue_ctxt(uint16_t rnti, uint8* k_gnb);
    bool send_smm_notify_ue_erab_updates(uint16_t rnti, uint8_t qos, uint16_t pdu_session_id, srsran::const_byte_span nas_pdu, int am_tm_type);
    bool send_test_smm_notify_ue_erab_updates(uint16_t rnti, uint8_t qos, uint16_t pdu_session_id, srsran::const_byte_span nas_pdu, int am_tm_type);
    void pcstomm(uint16_t rnti);
    bool send_voice_smm_notify_ue_erab_updates(uint16_t rnti, uint8_t qos, uint16_t pdu_session_id, srsran::const_byte_span nas_pdu, int am_tm_type);
    //bool send_wx_Mcontrol_Notify_Switch(uint16_t rnti, uint8_t Switch_Type_);
    bool send_wx_Mcontrol_Notify_Switch(uint16_t rnti, int Switch_Type_, uint8_t* add_info,  int len);
    bool send_nas_to_notify_rrc_release(uint16_t rnti);
    bool send_configuration_update_command_5g_guti_ttcn(uint16_t rnti);

  private:
    nas_mm();
    virtual ~nas_mm();
    adp *mm_adp = nullptr;
    cnw *m_cnw = nullptr;
    nas_sm *m_nas_sm = nullptr; // 12.9
    static nas_mm *m_instance; 

    // //-----------------2023/11/14
    // srsenb::rrc_interface_cnw* rrc_mm;
  };

} // namespace srsepc
#endif // SRSEPC_NAS_MM_H