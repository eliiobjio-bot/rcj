#include "pcs_ims_msg.h"
#include "pcs_ims_ies.h"
#include "srsran/asn1/nas_5g_utils.h"

#include "srsran/asn1/asn1_utils.h"
#include "srsran/common/buffer_pool.h"
#include "srsran/common/common.h"
#include "srsran/config.h"

#include <array>
#include <stdint.h>
#include <vector>

namespace srsran {
namespace ims {

/*
 * Message: Voice register Request
 *          Based on XW IMS protocol 7.3.1
 */
SRSASN_CODE voice_register_req_t::pack(asn1::bit_ref& bref)
{
  // Mandatory fields
  HANDLE_CODE(s_tmsi_5g.pack(bref));
  HANDLE_CODE(reg_type_ims.pack(bref));
  HANDLE_CODE(security_indicator_ims.pack(bref));
  HANDLE_CODE(security_algorithm_ims.pack(bref));

  // Conditional fields
  if (reg_type_ims.reg_type == reg_type_t::reg_type_type_::options::poweron_register) {
    HANDLE_CODE(imsi.pack(bref));
  }
  
  if (reg_type_ims.reg_type == reg_type_t::reg_type_type_::options::poweron_register) {
    HANDLE_CODE(bref.pack(ipaddr_tag, 8)); // Package the identity into a bit reference bref, encoded with 8 bits.
    HANDLE_CODE(ipaddr.pack(bref)); // Called pack () in class ipaddr_t to pack the ipaddr object into the reference bref.
  }

  if(security_indicator_ims.security_indicator != security_indicator_t::security_indicator_type_::options::first_reg_or_dereg_req_no_security_resp && security_indicator_ims.security_indicator != security_indicator_t::security_indicator_type_::options::ue_auth_net_mac_incompatible_req_no_security_resp){
    //if (reg_type_ims.reg_type != reg_type_t::reg_type_type_::options::poweron_register) { 
      HANDLE_CODE(bref.pack(security_response_tag, 8)); 
      HANDLE_CODE(security_response.pack(bref)); 
    //}

  }

  return SRSASN_SUCCESS;
}
SRSASN_CODE voice_register_req_t::unpack(asn1::cbit_ref& bref)
{
  // Mandatory fields
  HANDLE_CODE(s_tmsi_5g.unpack(bref));
  HANDLE_CODE(reg_type_ims.unpack(bref));

  // Conditional fields               
  if (reg_type_ims.reg_type == reg_type_t::reg_type_type_::options::poweron_register) {
    HANDLE_CODE(imsi.unpack(bref));
  }

  uint8_t tag;
  HANDLE_CODE(bref.unpack(tag, 8));
  if(tag==0x06){
    ipaddr.ipaddr_tag = tag;
    HANDLE_CODE(ipaddr.unpack(bref)); 
  }
  else{
    if(tag==0x01){
      security_indicator_ims.security_indicator.value = security_indicator_t::security_indicator_type_::first_reg_or_dereg_req_no_security_resp;
    }
    if(tag==0x02){
      security_indicator_ims.security_indicator.value = security_indicator_t::security_indicator_type_::ue_auth_net_mac_incompatible_req_no_security_resp;
    }
    if(tag==0x03){
      security_indicator_ims.security_indicator.value = security_indicator_t::security_indicator_type_::ue_auth_net_mac_compatible_sqn_incompatible_req_security_resp_auts;
    }
    if(tag==0x04){
      security_indicator_ims.security_indicator.value = security_indicator_t::security_indicator_type_::ue_auth_net_mac_compatible_sqn_compatible_req_security_resp_res;
    }

  }

  HANDLE_CODE(security_algorithm_ims.unpack(bref));

  if(security_indicator_ims.security_indicator != security_indicator_t::security_indicator_type_::options::first_reg_or_dereg_req_no_security_resp && 
     security_indicator_ims.security_indicator != security_indicator_t::security_indicator_type_::options::ue_auth_net_mac_incompatible_req_no_security_resp){
      HANDLE_CODE(bref.unpack(security_response.security_response_tag, 8)); 
      HANDLE_CODE(security_response.unpack(bref)); 

  }

  return SRSASN_SUCCESS;
}

/*
 * Message : Voice register Resp
 * Based on XW IMS protocol 7.3.2
 */
SRSASN_CODE voice_register_resp_t::pack(asn1::bit_ref& bref)
{
  // Mandatory fields
  HANDLE_CODE(s_tmsi_5g.pack(bref));
  HANDLE_CODE(cause_ims.pack(bref));

  // Conditional fields
  if (cause_ims.cause == cause_t::cause_type_::options::success) {
    HANDLE_CODE(expire.pack(bref));
  }

  return SRSASN_SUCCESS;
}
SRSASN_CODE voice_register_resp_t::unpack(asn1::cbit_ref& bref)
{
  // Mandatory fields
  HANDLE_CODE(s_tmsi_5g.unpack(bref));
  HANDLE_CODE(cause_ims.unpack(bref));

  // Conditional fields
  if (cause_ims.cause == cause_t::cause_type_::options::success) {
    HANDLE_CODE(expire.unpack(bref));
  }

  return SRSASN_SUCCESS;
}

/*
 * Message: Voice DeRegister Req
 *          Based on XW IMS protocol 7.3.3
 */
SRSASN_CODE voice_deregister_req_t::pack(asn1::bit_ref& bref)
{
  // Mandatory fields
  HANDLE_CODE(s_tmsi_5g.pack(bref));
  HANDLE_CODE(dereg_type_ims.pack(bref));
  HANDLE_CODE(cause_ims.pack(bref));
  HANDLE_CODE(security_indicator_ims.pack(bref));
  HANDLE_CODE(security_algorithm_ims.pack(bref));
  // Conditional fields
  if(security_indicator_ims.security_indicator != security_indicator_t::security_indicator_type_::options::first_reg_or_dereg_req_no_security_resp && security_indicator_ims.security_indicator != security_indicator_t::security_indicator_type_::options::ue_auth_net_mac_incompatible_req_no_security_resp){
  //if (dereg_type_ims.dereg_type != dereg_type_t::dereg_type_type_::options::user_dereg) {
    HANDLE_CODE(bref.pack(security_response_tag, 8));
    HANDLE_CODE(security_response.pack(bref)); 
  //}
  }

  return SRSASN_SUCCESS;
}
SRSASN_CODE voice_deregister_req_t::unpack(asn1::cbit_ref& bref)
{
  // Mandatory fields
  HANDLE_CODE(s_tmsi_5g.unpack(bref));
  HANDLE_CODE(dereg_type_ims.unpack(bref));
  HANDLE_CODE(cause_ims.unpack(bref));
  HANDLE_CODE(security_indicator_ims.unpack(bref));
  HANDLE_CODE(security_algorithm_ims.unpack(bref));
  // Conditional fields
  if(security_indicator_ims.security_indicator != security_indicator_t::security_indicator_type_::options::first_reg_or_dereg_req_no_security_resp && security_indicator_ims.security_indicator != security_indicator_t::security_indicator_type_::options::ue_auth_net_mac_incompatible_req_no_security_resp){
  //if (dereg_type_ims.dereg_type != dereg_type_t::dereg_type_type_::options::user_dereg) {
    // HANDLE_CODE(bref.unpack(security_response_tag, 8));
    HANDLE_CODE(security_response.unpack(bref)); 
  //}
  }  

  // while (bref.distance_bytes_end() > 0) {
  //   uint8_t tag;
  //   HANDLE_CODE(bref.unpack(tag, 8));
  //   switch (tag) {
  //     case security_response_tag:
  //     HANDLE_CODE(security_response.unpack(bref));
  //       break;
  //     default:
  //       asn1::log_error("Invalid IE %x", tag);
  //       break;
  //   }
  // }
  
  return SRSASN_SUCCESS;
}

/*
 * Message: Voice DeRegister Resp
 *          Based on XW IMS protocol 7.3.4
 */
SRSASN_CODE voice_deregister_resp_t::pack(asn1::bit_ref& bref)
{
  // Mandatory fields
  HANDLE_CODE(s_tmsi_5g.pack(bref));

  return SRSASN_SUCCESS;
}
SRSASN_CODE voice_deregister_resp_t::unpack(asn1::cbit_ref& bref)
{
  // Mandatory fields
  HANDLE_CODE(s_tmsi_5g.unpack(bref));

  return SRSASN_SUCCESS;
}

/*
 * Message: Authentication Command
 *          Based on XW IMS protocol 7.3.5
 */
SRSASN_CODE authentication_command_t::pack(asn1::bit_ref& bref)
{
  printf("authentication_command_t::pack\n");

  // Mandatory fields
  HANDLE_CODE(s_tmsi_5g.pack(bref));
    // printf("1111111111111111111111111111111\n");
  HANDLE_CODE(bref.pack(nonce_tag, 8));
    // printf("2222222222222222222222222222222\n");
  HANDLE_CODE(nonce.pack(bref));
    // printf("333333333333333333333333333333\n");

  return SRSASN_SUCCESS;
}
SRSASN_CODE authentication_command_t::unpack(asn1::cbit_ref& bref)
{
  // Mandatory fields
  HANDLE_CODE(s_tmsi_5g.unpack(bref));
  // HANDLE_CODE(bref.unpack(nonce_tag, 8));
  HANDLE_CODE(nonce.unpack(bref));

  return SRSASN_SUCCESS;
}

/*
 * Message : Call Setup
 * Based on XW IMS protocol 7.3.6.1
 * UE->PCS
 */
SRSASN_CODE ue_pcs_call_setup_t::pack(asn1::bit_ref& bref)
{
    // Mandatory fields
    HANDLE_CODE(call_id.pack(bref));
    HANDLE_CODE(s_tmsi_5g.pack(bref));
    HANDLE_CODE(call_type_ims.pack(bref));

    //TODO: Mandatory fields with tag?
    HANDLE_CODE(bref.pack(called_party_bcd_num_tag, 8));
    HANDLE_CODE(called_party_bcd_num.pack(bref));
    HANDLE_CODE(ttot_call_indicator_ims.pack(bref));
    HANDLE_CODE(call_code_rate_ims.pack(bref));

    return SRSASN_SUCCESS;
}

SRSASN_CODE ue_pcs_call_setup_t::unpack(asn1::cbit_ref& bref)
{
    HANDLE_CODE(call_id.unpack(bref));
    HANDLE_CODE(s_tmsi_5g.unpack(bref));
    HANDLE_CODE(call_type_ims.unpack(bref));
    //TODO: Mandatory fields with tag?
    uint8_t tag;
    HANDLE_CODE(bref.unpack(tag, 8));
    HANDLE_CODE(called_party_bcd_num.unpack(bref));
    HANDLE_CODE(ttot_call_indicator_ims.unpack(bref));
    HANDLE_CODE(call_code_rate_ims.unpack(bref));

    return SRSASN_SUCCESS;
}

/*
 * Message : Call Setup
 * Based on XW IMS protocol 7.3.6.2
 * PCS->UE
 */
SRSASN_CODE pcs_ue_call_setup_t::pack(asn1::bit_ref& bref)
{
    // Mandatory fields
    HANDLE_CODE(call_id.pack(bref));
    HANDLE_CODE(call_type_ims.pack(bref));

    //TODO: Mandatory fields with tag?
    HANDLE_CODE(bref.pack(calling_party_bcd_num_tag, 8));
    HANDLE_CODE(calling_party_bcd_num.pack(bref));
    HANDLE_CODE(ttot_call_indicator_ims.pack(bref));
    HANDLE_CODE(call_code_rate_ims.pack(bref));

    return SRSASN_SUCCESS;
}

SRSASN_CODE pcs_ue_call_setup_t::unpack(asn1::cbit_ref& bref)
{
    HANDLE_CODE(call_id.unpack(bref));
    HANDLE_CODE(call_type_ims.unpack(bref));
    //TODO: Mandatory fields with tag?
    uint8_t tag;
    HANDLE_CODE(bref.unpack(tag, 8));
    HANDLE_CODE(calling_party_bcd_num.unpack(bref));
    HANDLE_CODE(ttot_call_indicator_ims.unpack(bref));
    HANDLE_CODE(call_code_rate_ims.unpack(bref));

    return SRSASN_SUCCESS;
}

/*
 * Message : Call Setup Ack
 * Based on XW IMS protocol 7.3.7
 */
SRSASN_CODE call_setup_ack_t::pack(asn1::bit_ref& bref)
{
    HANDLE_CODE(call_id.pack(bref));
    HANDLE_CODE(call_code_rate_ims.pack(bref));

    return SRSASN_SUCCESS;
}

SRSASN_CODE call_setup_ack_t::unpack(asn1::cbit_ref& bref)
{
    HANDLE_CODE(call_id.unpack(bref));
    HANDLE_CODE(call_code_rate_ims.unpack(bref));

    return SRSASN_SUCCESS;
}

/*
 * Message : Call Alerting
 * Based on XW IMS protocol 7.3.8.1
 * UE->PCS
 */
SRSASN_CODE call_confirmed_t::pack(asn1::bit_ref& bref) {

  HANDLE_CODE(call_id.pack(bref));

  return SRSASN_SUCCESS;
}

SRSASN_CODE call_confirmed_t::unpack(asn1::cbit_ref& bref)
{
    HANDLE_CODE(call_id.unpack(bref));

    return SRSASN_SUCCESS;
}


/*
 * Message : Call Alerting
 * Based on XW IMS protocol 7.3.8.1
 * UE->PCS
 */
SRSASN_CODE ue_pcs_call_alerting_t::pack(asn1::bit_ref& bref)
{
    HANDLE_CODE(call_id.pack(bref));

    return SRSASN_SUCCESS;
}

SRSASN_CODE ue_pcs_call_alerting_t::unpack(asn1::cbit_ref& bref)
{
    HANDLE_CODE(call_id.unpack(bref));

    return SRSASN_SUCCESS;
}

/*
 * Message : Call Alerting
 * Based on XW IMS protocol 7.3.8.2
 * PCS->UE
 */
SRSASN_CODE pcs_ue_call_alerting_t::pack(asn1::bit_ref& bref)
{
    HANDLE_CODE(call_id.pack(bref));

    return SRSASN_SUCCESS;
}

SRSASN_CODE pcs_ue_call_alerting_t::unpack(asn1::cbit_ref& bref)
{
    HANDLE_CODE(call_id.unpack(bref));

    return SRSASN_SUCCESS;
}

/*
 * Message : Call Connect
 * Based on XW IMS protocol 7.3.9.1
 * UE->PCS
 */
SRSASN_CODE ue_pcs_call_connect_t::pack(asn1::bit_ref& bref)
{
    HANDLE_CODE(call_id.pack(bref));
    HANDLE_CODE(call_type_ims.pack(bref));

    return SRSASN_SUCCESS;
}

SRSASN_CODE ue_pcs_call_connect_t::unpack(asn1::cbit_ref& bref)
{
    HANDLE_CODE(call_id.unpack(bref));
    HANDLE_CODE(call_type_ims.unpack(bref));

    return SRSASN_SUCCESS;
}

/*
 * Message : Call Connect
 * Based on XW IMS protocol 7.3.9.2
 * PCS->UE
 */
SRSASN_CODE pcs_ue_call_connect_t::pack(asn1::bit_ref& bref)
{ 
    HANDLE_CODE(call_id.pack(bref));
    HANDLE_CODE(call_type_ims.pack(bref));

    return SRSASN_SUCCESS;
}

SRSASN_CODE pcs_ue_call_connect_t::unpack(asn1::cbit_ref& bref)
{
    HANDLE_CODE(call_id.unpack(bref));
    HANDLE_CODE(call_type_ims.unpack(bref));

    return SRSASN_SUCCESS;
}

/*
 * Message : Call Connect Ack
 * Based on XW IMS protocol 7.3.10
 */
SRSASN_CODE call_connect_ack_t::pack(asn1::bit_ref& bref)
{
    HANDLE_CODE(call_id.pack(bref));

    return SRSASN_SUCCESS;
}

SRSASN_CODE call_connect_ack_t::unpack(asn1::cbit_ref& bref)
{
    HANDLE_CODE(call_id.unpack(bref));

    return SRSASN_SUCCESS;
}

/*
 * Message : Call disConnect 
 * Based on XW IMS protocol 7.3.11
 */
SRSASN_CODE call_disconnect_t::pack(asn1::bit_ref& bref)
{
    HANDLE_CODE(call_id.pack(bref));
    HANDLE_CODE(cause_ims.pack(bref));

    return SRSASN_SUCCESS;
}

SRSASN_CODE call_disconnect_t::unpack(asn1::cbit_ref& bref)
{
    HANDLE_CODE(call_id.unpack(bref));
    HANDLE_CODE(cause_ims.unpack(bref));

    return SRSASN_SUCCESS;
}

/*
 * Message : Call Release Req 
 * Based on XW IMS protocol 7.3.12
 */
SRSASN_CODE call_release_req_t::pack(asn1::bit_ref& bref)
{
    HANDLE_CODE(call_id.pack(bref));
    HANDLE_CODE(cause_ims.pack(bref));

    return SRSASN_SUCCESS;
}

SRSASN_CODE call_release_req_t::unpack(asn1::cbit_ref& bref)
{
    HANDLE_CODE(call_id.unpack(bref));
    HANDLE_CODE(cause_ims.unpack(bref));

    return SRSASN_SUCCESS;
}

/*
 * Message : Call Release Resp 
 * Based on XW IMS protocol 7.3.13
 */
SRSASN_CODE call_release_resp_t::pack(asn1::bit_ref& bref)
{
    HANDLE_CODE(call_id.pack(bref));
    HANDLE_CODE(cause_ims.pack(bref));

    return SRSASN_SUCCESS;
}

SRSASN_CODE call_release_resp_t::unpack(asn1::cbit_ref& bref)
{
    HANDLE_CODE(call_id.unpack(bref));
    HANDLE_CODE(cause_ims.unpack(bref));

    return SRSASN_SUCCESS;
}

/*
 * Message : MO SMS Req 
 * Based on XW IMS protocol 7.3.14
 */
SRSASN_CODE mo_sms_req_t::pack(asn1::bit_ref& bref)
{
    HANDLE_CODE(call_id.pack(bref));
    HANDLE_CODE(reference.pack(bref));
    HANDLE_CODE(s_tmsi_5g.pack(bref));
    //TODO: Mandatory fields with tag?
    HANDLE_CODE(bref.pack(smc_party_bcd_num_tag, 8));
    HANDLE_CODE(smc_party_bcd_num.pack(bref));
    //TODO: Mandatory fields with tag?
    HANDLE_CODE(bref.pack(message_content_tag, 8));
    HANDLE_CODE(message_content.pack(bref));

    return SRSASN_SUCCESS;
}

SRSASN_CODE mo_sms_req_t::unpack(asn1::cbit_ref& bref)
{
    HANDLE_CODE(call_id.unpack(bref));
    HANDLE_CODE(reference.unpack(bref));
    HANDLE_CODE(s_tmsi_5g.unpack(bref));
//TODO: Mandatory fields with tag?
    uint8_t tag;
    HANDLE_CODE(bref.unpack(tag, 8));
    HANDLE_CODE(smc_party_bcd_num.unpack(bref));
//TODO: Mandatory fields with tag?
    HANDLE_CODE(bref.unpack(tag, 8));
    HANDLE_CODE(message_content.unpack(bref));
    return SRSASN_SUCCESS;
}


SRSASN_CODE rp_data_msg_to_network_t::unpack(asn1::cbit_ref& bref)
{
    
  HANDLE_CODE(bref.unpack(rp_msg_type, 8)); 
  HANDLE_CODE(bref.unpack(rp_msg_reference, 8));
  HANDLE_CODE(bref.unpack(rp_originator_address, 8));
  HANDLE_CODE(bref.unpack(rp_destionation_address_len, 8));
  rp_destionation_address.resize(rp_destionation_address_len);
  HANDLE_CODE(bref.unpack_bytes(rp_destionation_address.data(), rp_destionation_address_len));

  HANDLE_CODE(bref.unpack(rp_user_data_len, 8));
  printf("rp_user_data_len%x\n", rp_user_data_len); 
  HANDLE_CODE(bref.unpack(rp_user_data_header, 8));
  printf("rp_user_data_header%x\n", rp_user_data_header); 
  tp_msg_type_indicator = rp_user_data_header & 0x03;
  tp_reject_duplicates = (rp_user_data_header >> 2) & 0x01;
  tp_validity_period_format = (rp_user_data_header >> 3) & 0x03;
  tp_reply_path = (rp_user_data_header >> 5) & 0x01;
  tp_user_data_header_indicator = (rp_user_data_header >> 6) & 0x01;
  tp_status_reprot_request = (rp_user_data_header >> 7) & 0x01;
  printf("tp_user_data_header_indicator%d\n", tp_user_data_header_indicator);

  HANDLE_CODE(bref.unpack(tp_msg_reference, 8));
  HANDLE_CODE(bref.unpack(tp_destination_address_num, 8));

  if(tp_destination_address_num == 0x0b) {  //exclusive 86
    tp_destination_address.resize(7);
    HANDLE_CODE(bref.unpack_bytes(tp_destination_address.data(), 7));
  }
  else if(tp_destination_address_num == 0x0d) { //contain 86
    tp_destination_address.resize(8);
    HANDLE_CODE(bref.unpack_bytes(tp_destination_address.data(), 8));
  }

  HANDLE_CODE(bref.unpack(tp_protocol_indentifier, 8));
  HANDLE_CODE(bref.unpack(tp_data_coding_form, 8));

  if(tp_validity_period_format) {
    HANDLE_CODE(bref.unpack(tp_validity_period, 8));
  }
  HANDLE_CODE(bref.unpack(tp_user_data_len, 8));
  printf("tp_user_data_len0x%0x , size:%d\n", tp_user_data_len, tp_user_data_len); 

  if (tp_user_data_len < 1) {
  asn1::log_error("Decoding Failed (Payload container): Length (%d) is not in range of min: 1 bytes", tp_user_data_len);
  return asn1::SRSASN_ERROR_DECODE_FAIL;
  }  

  if(tp_data_coding_form == 0x00) { //7bit code form English
    if(tp_user_data_header_indicator == 0x00) { 
      tp_user_data_len = (tp_user_data_len*7)%8 == 0 ? tp_user_data_len*7/8 : tp_user_data_len*7/8+1;
    }
    tp_user_data.resize(tp_user_data_len);
    HANDLE_CODE(bref.unpack_bytes(tp_user_data.data(), tp_user_data_len));
  }
  else {
    tp_user_data.resize(tp_user_data_len);
    HANDLE_CODE(bref.unpack_bytes(tp_user_data.data(), tp_user_data_len));
  }
  
  return SRSASN_SUCCESS;
}

/*
 * Message : MO SMS Resp 
 * Based on XW IMS protocol 7.3.15
 */
SRSASN_CODE mo_sms_resp_t::pack(asn1::bit_ref& bref)
{
    HANDLE_CODE(call_id.pack(bref));
    HANDLE_CODE(reference.pack(bref));
    HANDLE_CODE(cause_ims.pack(bref));

    return SRSASN_SUCCESS;
}

SRSASN_CODE mo_sms_resp_t::unpack(asn1::cbit_ref& bref)
{
    HANDLE_CODE(call_id.unpack(bref));
    HANDLE_CODE(reference.unpack(bref));
    HANDLE_CODE(cause_ims.unpack(bref));

    return SRSASN_SUCCESS;
}

/*
 * Message : MT SMS Req 
 * Based on XW IMS protocol 7.3.16
 */
SRSASN_CODE mt_sms_req_t::pack(asn1::bit_ref& bref)
{
    HANDLE_CODE(call_id.pack(bref));
    //TODO: Mandatory fields with tag?
    HANDLE_CODE(bref.pack(smc_party_bcd_num_tag, 8));
    HANDLE_CODE(smc_party_bcd_num.pack(bref));
    //TODO: Mandatory fields with tag?
    HANDLE_CODE(bref.pack(message_content_tag, 8));
    HANDLE_CODE(message_content.pack(bref));

    return SRSASN_SUCCESS;
}
SRSASN_CODE mt_sms_req_t::unpack(asn1::cbit_ref& bref)
{
    HANDLE_CODE(call_id.unpack(bref));
    //TODO: Mandatory fields with tag?
    uint8_t tag;
    HANDLE_CODE(bref.unpack(tag, 8));
    HANDLE_CODE(smc_party_bcd_num.unpack(bref));
    //TODO: Mandatory fields with tag?
    HANDLE_CODE(bref.unpack(tag, 8));
    HANDLE_CODE(message_content.unpack(bref));

    return SRSASN_SUCCESS;
}

/*
 * Message : MT SMS Resp 
 * Based on XW IMS protocol 7.3.17
 */
SRSASN_CODE mt_sms_resp_t::pack(asn1::bit_ref& bref)
{
    HANDLE_CODE(call_id.pack(bref));
    HANDLE_CODE(s_tmsi_5g.pack(bref));
    HANDLE_CODE(cause_ims.pack(bref));

    return SRSASN_SUCCESS;
}

SRSASN_CODE mt_sms_resp_t::unpack(asn1::cbit_ref& bref)
{
    // printf("**********2024-03-20-1************\n");
    HANDLE_CODE(call_id.unpack(bref));
    // printf("**********2024-03-20-2************\n");
    HANDLE_CODE(s_tmsi_5g.unpack(bref));
    // printf("**********2024-03-20-3************\n");
    HANDLE_CODE(cause_ims.unpack(bref));
    // printf("**********2024-03-20-4************\n");

    return SRSASN_SUCCESS;
}

/*
 * Message : Message structure 
 * Based on XW IMS protocol 7.1
 */
SRSASN_CODE pcs_ims_hdr::unpack(asn1::cbit_ref& bref)
{
  srsran::nas_5g::unpack_enum<extended_protocol_discriminator_options, 4>(bref, &extended_protocol_discriminator);
  printf("--------extended_protocol_discriminator--------:%d\n", extended_protocol_discriminator);
  /* Calls(diao yong) the template(mo ban) function unpack_enum () to parse the 8-bit 
     extended_protocol_discriminator field and stores the result in the extended_protocol_discriminator variable.*/  
  switch (extended_protocol_discriminator) {
    case extended_protocol_discriminator_pcs:
      srsran::nas_5g::unpack_enum<protocol_version_options, 4>(bref, &protocol_version);
      printf("--------protocol_version_options--------:%d\n", protocol_version);
      HANDLE_CODE(bref.unpack(message_length, 8));
      printf("--------message_length--------:%d\n", message_length);
      if (protocol_version == test_version) {
        HANDLE_CODE(ims_message_type.unpack(bref));
      } 
      else {
        HANDLE_CODE(ims_message_type.unpack(bref));
      }
      break;
    default:
      asn1::log_error("Unsupported extended protocol discriminator %x\n", extended_protocol_discriminator);
      return SRSASN_ERROR_DECODE_FAIL;
  }
  return SRSASN_SUCCESS;
}

SRSASN_CODE pcs_ims_hdr::pack(asn1::bit_ref& bref)
{
  srsran::nas_5g::pack_enum<extended_protocol_discriminator_options, 4>(bref, extended_protocol_discriminator);

  switch (extended_protocol_discriminator) {
    case extended_protocol_discriminator_pcs:
      srsran::nas_5g::pack_enum<protocol_version_options, 4>(bref, protocol_version);
      HANDLE_CODE(bref.pack(message_length, 8));
      if (protocol_version == test_version) {
        HANDLE_CODE(ims_message_type.pack(bref));
      } else {
        HANDLE_CODE(ims_message_type.pack(bref));
      }
      break;
    default:
      asn1::log_error("Unsupported extended protocol discriminator %x\n", extended_protocol_discriminator);
      return SRSASN_ERROR_DECODE_FAIL;
  }
  return SRSASN_SUCCESS;
}

SRSASN_CODE pcs_ims_msg::pack(unique_byte_buffer_t& buf)
{
  //printf("12111111111111111111\n");
  asn1::bit_ref msg_bref(buf->msg, buf->get_tailroom());
  HANDLE_CODE(pack(msg_bref));
  buf->N_bytes = msg_bref.distance_bytes();
  //printf("12111111111111111111\n");
  return SRSASN_SUCCESS;
}

SRSASN_CODE pcs_ims_msg::pack(std::vector<uint8_t>& buf)
{
  buf.resize(SRSRAN_MAX_BUFFER_SIZE_BYTES);
  asn1::bit_ref msg_bref(buf.data(), buf.size());
  HANDLE_CODE(pack(msg_bref));
  buf.resize(msg_bref.distance_bytes());
  return SRSASN_SUCCESS;
}

SRSASN_CODE pcs_ims_msg::pack(asn1::bit_ref& msg_bref)
{
  HANDLE_CODE(hdr.pack(msg_bref));
  switch (hdr.ims_message_type) {
    case ims_msg_types::options::voice_register_req: {
      voice_register_req_t* msg = srslog::detail::any_cast<voice_register_req_t>(&msg_container);
      HANDLE_CODE(msg->pack(msg_bref));
      break;
    }
    case ims_msg_types::options::voice_register_resp: {
      voice_register_resp_t* msg = srslog::detail::any_cast<voice_register_resp_t>(&msg_container);
      HANDLE_CODE(msg->pack(msg_bref));
      break;
    }
    case ims_msg_types::options::voice_deregister_req: {
      voice_deregister_req_t* msg = srslog::detail::any_cast<voice_deregister_req_t>(&msg_container);
      HANDLE_CODE(msg->pack(msg_bref));
      break;
    }
    case ims_msg_types::options::voice_deregister_resp: {
      voice_deregister_resp_t* msg = srslog::detail::any_cast<voice_deregister_resp_t>(&msg_container);
      HANDLE_CODE(msg->pack(msg_bref));
      break;
    }
    case ims_msg_types::options::authentication_command: {
      authentication_command_t* msg = srslog::detail::any_cast<authentication_command_t>(&msg_container);
      HANDLE_CODE(msg->pack(msg_bref));
      break;
    }
    case ims_msg_types::options::call_setup: {
      //ue_pcs_call_setup_t* msg = srslog::detail::any_cast<ue_pcs_call_setup_t>(&msg_container);
      pcs_ue_call_setup_t* msg = srslog::detail::any_cast<pcs_ue_call_setup_t>(&msg_container);      
      HANDLE_CODE(msg->pack(msg_bref));
      break;
    }   
    case ims_msg_types::options::call_setup_ack: {
      call_setup_ack_t* msg = srslog::detail::any_cast<call_setup_ack_t>(&msg_container);
      HANDLE_CODE(msg->pack(msg_bref));
      break;
    }  
    case ims_msg_types::options::call_confirmed :{
      //ue_pcs_call_alerting_t* msg = srslog::detail::any_cast<ue_pcs_call_alerting_t>(&msg_container);
      call_confirmed_t* msg = srslog::detail::any_cast<call_confirmed_t>(&msg_container);
      HANDLE_CODE(msg->pack(msg_bref));
      break;
    }   
    case ims_msg_types::options::call_alerting: {
      //ue_pcs_call_alerting_t* msg = srslog::detail::any_cast<ue_pcs_call_alerting_t>(&msg_container);
      pcs_ue_call_alerting_t* msg = srslog::detail::any_cast<pcs_ue_call_alerting_t>(&msg_container);
      HANDLE_CODE(msg->pack(msg_bref));
      break;
    }
    case ims_msg_types::options::call_connect: {
      //ue_pcs_call_connect_t* msg = srslog::detail::any_cast<ue_pcs_call_connect_t>(&msg_container);
      pcs_ue_call_connect_t* msg = srslog::detail::any_cast<pcs_ue_call_connect_t>(&msg_container);
      HANDLE_CODE(msg->pack(msg_bref));
      break;
    }
    case ims_msg_types::options::call_connect_ack: {
      call_connect_ack_t* msg = srslog::detail::any_cast<call_connect_ack_t>(&msg_container);
      HANDLE_CODE(msg->pack(msg_bref));
      break;
    }
    case ims_msg_types::options::call_disconnect: {
      call_disconnect_t* msg = srslog::detail::any_cast<call_disconnect_t>(&msg_container);
      HANDLE_CODE(msg->pack(msg_bref));
      break;
    }    
    case ims_msg_types::options::call_release_req: {
      call_release_req_t* msg = srslog::detail::any_cast<call_release_req_t>(&msg_container);
      HANDLE_CODE(msg->pack(msg_bref));
      break;
    } 
    case ims_msg_types::options::call_release_resp: {
      call_release_resp_t* msg = srslog::detail::any_cast<call_release_resp_t>(&msg_container);
      HANDLE_CODE(msg->pack(msg_bref));
      break;
    } 
    case ims_msg_types::options::mo_sms_req: {
      mo_sms_req_t* msg = srslog::detail::any_cast<mo_sms_req_t>(&msg_container);
      HANDLE_CODE(msg->pack(msg_bref));
      break;
    } 
    case ims_msg_types::options::mo_sms_resp: {
      mo_sms_resp_t* msg = srslog::detail::any_cast<mo_sms_resp_t>(&msg_container);
      HANDLE_CODE(msg->pack(msg_bref));
      break;
    }
    case ims_msg_types::options::mt_sms_req: {
      mt_sms_req_t* msg = srslog::detail::any_cast<mt_sms_req_t>(&msg_container);
      HANDLE_CODE(msg->pack(msg_bref));
      break;
    } 
    case ims_msg_types::options::mt_sms_resp: {
      mt_sms_resp_t* msg = srslog::detail::any_cast<mt_sms_resp_t>(&msg_container);
      HANDLE_CODE(msg->pack(msg_bref));
      break;
    }
    case ims_msg_types::options::call_hold: {
        
      break;
    }
    case ims_msg_types::options::call_hold_ack: {
        
      break;
    }
    case ims_msg_types::options::call_wait_indication: {
        
      break;
    }
    case ims_msg_types::options::call_wait_indication_resp: {
    
      break;
    }
    default:
      break;
  }
  return SRSASN_SUCCESS;
}

SRSASN_CODE pcs_ims_msg::unpack(const unique_byte_buffer_t& buf)
{
    asn1::cbit_ref msg_bref(buf->msg, buf->N_bytes);
    HANDLE_CODE(unpack(msg_bref));
    return SRSASN_SUCCESS;
}

SRSASN_CODE pcs_ims_msg::unpack(const std::vector<uint8_t>& buf)
{
    asn1::cbit_ref msg_bref(buf.data(), buf.size());
    HANDLE_CODE(unpack(msg_bref));
    return SRSASN_SUCCESS;
}

SRSASN_CODE rp_data_msg_to_network_t::unpack(const std::vector<uint8_t>& buf)
{
    asn1::cbit_ref msg_bref(buf.data(), buf.size());
    HANDLE_CODE(unpack(msg_bref));
    return SRSASN_SUCCESS;
}

SRSASN_CODE pcs_ims_msg::unpack(asn1::cbit_ref& msg_bref)
{
  std::cout << "-----unpack pcs_ims_msg----" << std::endl;
  HANDLE_CODE(hdr.unpack(msg_bref));
  switch (hdr.ims_message_type) {
    case ims_msg_types::options::voice_register_req: {
    msg_container             = srslog::detail::any{voice_register_req_t()};
    voice_register_req_t* msg = srslog::detail::any_cast<voice_register_req_t>(&msg_container);
    HANDLE_CODE(msg->unpack(msg_bref));
    break;
    } 
    case ims_msg_types::options::voice_register_resp: {
    msg_container             = srslog::detail::any{voice_register_resp_t()};
    voice_register_resp_t* msg = srslog::detail::any_cast<voice_register_resp_t>(&msg_container);
    HANDLE_CODE(msg->unpack(msg_bref));
    break;
    }
    case ims_msg_types::options::voice_deregister_req: {
    msg_container             = srslog::detail::any{voice_deregister_req_t()};
    voice_deregister_req_t* msg = srslog::detail::any_cast<voice_deregister_req_t>(&msg_container);
    HANDLE_CODE(msg->unpack(msg_bref));
    break;
    }      
    case ims_msg_types::options::voice_deregister_resp: {
    msg_container             = srslog::detail::any{voice_deregister_resp_t()};
    voice_deregister_resp_t* msg = srslog::detail::any_cast<voice_deregister_resp_t>(&msg_container);
    HANDLE_CODE(msg->unpack(msg_bref));
    break;
    } 
    case ims_msg_types::options::authentication_command: {
    msg_container             = srslog::detail::any{authentication_command_t()};
    authentication_command_t* msg = srslog::detail::any_cast<authentication_command_t>(&msg_container);
    HANDLE_CODE(msg->unpack(msg_bref));
    break;
    }
    case ims_msg_types::options::call_setup: {
    //msg_container             = srslog::detail::any{pcs_ue_call_setup_t()};
    //pcs_ue_call_setup_t* msg = srslog::detail::any_cast<pcs_ue_call_setup_t>(&msg_container);      
    msg_container             = srslog::detail::any{ue_pcs_call_setup_t()};
    ue_pcs_call_setup_t* msg = srslog::detail::any_cast<ue_pcs_call_setup_t>(&msg_container);
    HANDLE_CODE(msg->unpack(msg_bref));
    break;
    }    
    case ims_msg_types::options::call_setup_ack: {     
    msg_container             = srslog::detail::any{call_setup_ack_t()};
    call_setup_ack_t* msg = srslog::detail::any_cast<call_setup_ack_t>(&msg_container);
    HANDLE_CODE(msg->unpack(msg_bref));
    break;
    }
    case ims_msg_types::options::call_confirmed: {     
    msg_container             = srslog::detail::any{call_confirmed_t()};
    call_confirmed_t* msg = srslog::detail::any_cast<call_confirmed_t>(&msg_container);
    HANDLE_CODE(msg->unpack(msg_bref));
    break;
    }
    case ims_msg_types::options::call_alerting: {
    //msg_container             = srslog::detail::any{pcs_ue_call_alerting_t()};
    //pcs_ue_call_alerting_t* msg = srslog::detail::any_cast<pcs_ue_call_alerting_t>(&msg_container);           
    msg_container             = srslog::detail::any{ue_pcs_call_alerting_t()};
    ue_pcs_call_alerting_t* msg = srslog::detail::any_cast<ue_pcs_call_alerting_t>(&msg_container);
    HANDLE_CODE(msg->unpack(msg_bref));
    break;
    } 
    case ims_msg_types::options::call_connect: {
    //msg_container             = srslog::detail::any{pcs_ue_call_connect_t()};
    //pcs_ue_call_connect_t* msg = srslog::detail::any_cast<pcs_ue_call_connect_t>(&msg_container);           
    msg_container             = srslog::detail::any{ue_pcs_call_connect_t()};
    ue_pcs_call_connect_t* msg = srslog::detail::any_cast<ue_pcs_call_connect_t>(&msg_container);
    HANDLE_CODE(msg->unpack(msg_bref));
    break;
    } 
    case ims_msg_types::options::call_connect_ack: {     
    msg_container             = srslog::detail::any{call_connect_ack_t()};
    call_connect_ack_t* msg = srslog::detail::any_cast<call_connect_ack_t>(&msg_container);
    HANDLE_CODE(msg->unpack(msg_bref));
    break;
    } 
    case ims_msg_types::options::call_disconnect: {     
    msg_container             = srslog::detail::any{call_disconnect_t()};
    call_disconnect_t* msg = srslog::detail::any_cast<call_disconnect_t>(&msg_container);
    HANDLE_CODE(msg->unpack(msg_bref));
    break;
    } 
    case ims_msg_types::options::call_release_req: {
    msg_container             = srslog::detail::any{call_release_req_t()};
    call_release_req_t* msg = srslog::detail::any_cast<call_release_req_t>(&msg_container);
    HANDLE_CODE(msg->unpack(msg_bref));
    break;
    } 
    case ims_msg_types::options::call_release_resp: {
    msg_container             = srslog::detail::any{call_release_resp_t()};
    call_release_resp_t* msg = srslog::detail::any_cast<call_release_resp_t>(&msg_container);
    HANDLE_CODE(msg->unpack(msg_bref));
    break;
    } 
    case ims_msg_types::options::mo_sms_req: {
    msg_container             = srslog::detail::any{mo_sms_req_t()};
    mo_sms_req_t* msg = srslog::detail::any_cast<mo_sms_req_t>(&msg_container);
    HANDLE_CODE(msg->unpack(msg_bref));
    break;
    } 
    case ims_msg_types::options::mo_sms_resp: {
    msg_container             = srslog::detail::any{mo_sms_resp_t()};
    mo_sms_resp_t* msg = srslog::detail::any_cast<mo_sms_resp_t>(&msg_container);
    HANDLE_CODE(msg->unpack(msg_bref));
    break;
    }  
    case ims_msg_types::options::mt_sms_req: {
    msg_container             = srslog::detail::any{mt_sms_req_t()};
    mt_sms_req_t* msg = srslog::detail::any_cast<mt_sms_req_t>(&msg_container);
    HANDLE_CODE(msg->unpack(msg_bref));
    break;
    } 
    case ims_msg_types::options::mt_sms_resp: {
    msg_container             = srslog::detail::any{mt_sms_resp_t()};
    mt_sms_resp_t* msg = srslog::detail::any_cast<mt_sms_resp_t>(&msg_container);
    HANDLE_CODE(msg->unpack(msg_bref));
    break;
    }    
    case ims_msg_types::options::call_hold: {
        
      break;
    }
    case ims_msg_types::options::call_hold_ack: {
        
      break;
    }
    case ims_msg_types::options::call_wait_indication: {
        
      break;
    }
    case ims_msg_types::options::call_wait_indication_resp: {
    
      break;
    }         
    default:
      break;
  }
  return SRSASN_SUCCESS;
}


} // namespace pcs_ims
} // namespace srsran