#include "srscnw/hdr/pcs_ims_ies.h"

#include "srsran/asn1/asn1_utils.h"
#include "srsran/common/buffer_pool.h"
#include "srsran/common/common.h"
#include "srsran/config.h"

#include <array>
#include <stdint.h>
#include <vector>

namespace srsran {
namespace ims{

using namespace asn1;
// IE: 5G-S-TMSI
// Reference: 8.18
SRSASN_CODE s_tmsi_5g_t::pack(asn1::bit_ref& bref)
{
  HANDLE_CODE(bref.pack(s_tmsi_5g_value[0], 8));
  HANDLE_CODE(bref.pack(s_tmsi_5g_value[1], 8));
  HANDLE_CODE(bref.pack(s_tmsi_5g_value[2], 8));
  HANDLE_CODE(bref.pack(s_tmsi_5g_value[3], 8));
  HANDLE_CODE(bref.pack(s_tmsi_5g_value[4], 8));
  HANDLE_CODE(bref.pack(s_tmsi_5g_value[5], 8));

  return SRSASN_SUCCESS;
}

SRSASN_CODE s_tmsi_5g_t::unpack(asn1::cbit_ref& bref)
{
  HANDLE_CODE(bref.unpack(s_tmsi_5g_value[0], 8));
  HANDLE_CODE(bref.unpack(s_tmsi_5g_value[1], 8));
  HANDLE_CODE(bref.unpack(s_tmsi_5g_value[2], 8));
  HANDLE_CODE(bref.unpack(s_tmsi_5g_value[3], 8));
  HANDLE_CODE(bref.unpack(s_tmsi_5g_value[4], 8));
  HANDLE_CODE(bref.unpack(s_tmsi_5g_value[5], 8));

  return SRSASN_SUCCESS;
}
// IE: REG_TYPE
// Reference: 8.15
SRSASN_CODE reg_type_t::pack(asn1::bit_ref& bref)
{
  HANDLE_CODE(reg_type.pack(bref));

  return SRSASN_SUCCESS;
}

SRSASN_CODE reg_type_t::unpack(asn1::cbit_ref& bref)
{
  HANDLE_CODE(reg_type.unpack(bref));

  return SRSASN_SUCCESS;
}
const char* reg_type_t::reg_type_type_::to_string()
{
  switch (value) {
    case reg_type_t::reg_type_type_::poweron_register:
      return "poweron_register";
    case reg_type_t::reg_type_type_::periodic_register_updating:
      return "periodic_register_updating";
    default:
      return "Invalid Choice";
  }
}

// IE: IMSI
// Reference: 8.10
SRSASN_CODE imsi_t::pack(asn1::bit_ref& bref)
{
  HANDLE_CODE(bref.pack(imsi[1], 4));
  HANDLE_CODE(bref.pack(imsi[0], 4));//Pack the 4 bits of the first element of the imsi array into ASN.1 format.
  HANDLE_CODE(bref.pack(imsi[3], 4));
  HANDLE_CODE(bref.pack(imsi[2], 4));
  HANDLE_CODE(bref.pack(imsi[5], 4));
  HANDLE_CODE(bref.pack(imsi[4], 4));
  HANDLE_CODE(bref.pack(imsi[7], 4));
  HANDLE_CODE(bref.pack(imsi[6], 4));
  HANDLE_CODE(bref.pack(imsi[9], 4));
  HANDLE_CODE(bref.pack(imsi[8], 4));
  HANDLE_CODE(bref.pack(imsi[11], 4));
  HANDLE_CODE(bref.pack(imsi[10], 4));
  HANDLE_CODE(bref.pack(imsi[13], 4));
  HANDLE_CODE(bref.pack(imsi[12], 4));
  HANDLE_CODE(bref.pack(imsi[13], 4));
  HANDLE_CODE(bref.pack(0xf, 4));//Package a 4-bit fixed value 0xf into ASN.1 format. This value may be a predefined flag.
  HANDLE_CODE(bref.pack(imsi[14], 4));//Pack the 4 bits of the last element of the imsi array into ASN.1 format.

  return SRSASN_SUCCESS;
}

SRSASN_CODE imsi_t::unpack(asn1::cbit_ref& bref)
{
  //imsi[1] = (uint8_t)(tmp >> 1) & 0x0f;//?
  HANDLE_CODE(bref.unpack(imsi[1], 4));
  HANDLE_CODE(bref.unpack(imsi[0], 4)); 
  HANDLE_CODE(bref.unpack(imsi[3], 4));
  HANDLE_CODE(bref.unpack(imsi[2], 4));
  HANDLE_CODE(bref.unpack(imsi[5], 4));
  HANDLE_CODE(bref.unpack(imsi[4], 4));
  HANDLE_CODE(bref.unpack(imsi[7], 4));
  HANDLE_CODE(bref.unpack(imsi[6], 4));
  HANDLE_CODE(bref.unpack(imsi[9], 4));
  HANDLE_CODE(bref.unpack(imsi[8], 4));
  HANDLE_CODE(bref.unpack(imsi[11], 4));
  HANDLE_CODE(bref.unpack(imsi[10], 4));
  HANDLE_CODE(bref.unpack(imsi[13], 4));
  HANDLE_CODE(bref.unpack(imsi[12], 4));
  HANDLE_CODE(bref.advance_bits(4));      
  HANDLE_CODE(bref.unpack(imsi[14], 4)); 

  return SRSASN_SUCCESS;
}

// IE: IP Addr
// Reference: 8.11
SRSASN_CODE ipaddr_t::pack(asn1::bit_ref& bref)
{
  HANDLE_CODE(bref.advance_bits(8));
  // Advances the bit stream by 8 bits to reserve space for the ip_addr_tag.
  //HANDLE_CODE(bref.pack(ipaddr_value, 4 * 8));
  HANDLE_CODE(bref.pack(ipaddr_value[0], 8));
  HANDLE_CODE(bref.pack(ipaddr_value[1], 8));
  HANDLE_CODE(bref.pack(ipaddr_value[2], 8));
  HANDLE_CODE(bref.pack(ipaddr_value[3], 8));

  return SRSASN_SUCCESS;
}

SRSASN_CODE ipaddr_t::unpack(asn1::cbit_ref& bref)
{
  // HANDLE_CODE(bref.unpack(ipaddr_value, 4 * 8));
  HANDLE_CODE(bref.unpack(ipaddr_value[0], 8));
  HANDLE_CODE(bref.unpack(ipaddr_value[1], 8));
  HANDLE_CODE(bref.unpack(ipaddr_value[2], 8));
  HANDLE_CODE(bref.unpack(ipaddr_value[3], 8));

  return SRSASN_SUCCESS;
}

// IE: Security indicator
// Reference: 8.22
SRSASN_CODE security_indicator_t::pack(asn1::bit_ref& bref)
{
  HANDLE_CODE(security_indicator.pack(bref));

  return SRSASN_SUCCESS;
}

SRSASN_CODE security_indicator_t::unpack(asn1::cbit_ref& bref)
{
  HANDLE_CODE(security_indicator.unpack(bref));

  return SRSASN_SUCCESS;
}
const char* security_indicator_t::security_indicator_type_::to_string()
{
  switch (value) {
    case security_indicator_t::security_indicator_type_::first_reg_or_dereg_req_no_security_resp:
      return "first_reg_or_dereg_req_no_security_resp";
    case security_indicator_t::security_indicator_type_::ue_auth_net_mac_incompatible_req_no_security_resp:
      return "ue_auth_net_mac_incompatible_req_no_security_resp";
    case security_indicator_t::security_indicator_type_::
        ue_auth_net_mac_compatible_sqn_incompatible_req_security_resp_auts:
      return "ue_auth_net_mac_compatible_sqn_incompatible_req_security_resp_auts";
    case security_indicator_t::security_indicator_type_::
        ue_auth_net_mac_compatible_sqn_compatible_req_security_resp_res:
      return "ue_auth_net_mac_compatible_sqn_compatible_req_security_resp_res";
    default:
      return "Invalid Choice";
  }
}

// IE: Security algorithm
// Reference: 8.17
SRSASN_CODE security_algorithm_t::pack(asn1::bit_ref& bref)
{
  HANDLE_CODE(security_algorithm.pack(bref));

  return SRSASN_SUCCESS;
}

SRSASN_CODE security_algorithm_t::unpack(asn1::cbit_ref& bref)
{
  HANDLE_CODE(security_algorithm.unpack(bref));

  return SRSASN_SUCCESS;
}
const char* security_algorithm_t::security_algorithm_type_::to_string()
{
  switch (value) {
    case security_algorithm_t::security_algorithm_type_::aka:
      return "aka";    
    case security_algorithm_t::security_algorithm_type_::akav1_md5:
      return "akav1_md5";
    case security_algorithm_t::security_algorithm_type_::md5:
      return "md5";
    default:
      return "Invalid Choice";
  }
}

// IE: Security Response
// Reference: 8.16
SRSASN_CODE security_response_t::pack(asn1::bit_ref& bref)
{    
    // Save Length of Security Response
    asn1::bit_ref bref_length = bref; 
    // Create a new bit_ref object, bref_length, as a copy of the original bref. Marks where the length information will be inserted later.
    HANDLE_CODE(bref.advance_bits(8)); 
    // Advances the bit stream by 8 bits to reserve space for security_response_tag.

    // Using pack_bytes (), package the security_response into the bit stream bref.
    HANDLE_CODE(bref.pack_bytes(security_response.data(), security_response.size()));
    bref.align_bytes_zero(); // Align the bit stream to the nearest byte boundary and fill it with zeros if needed.
    uint8_t length = (uint8_t)(ceilf((float)bref.distance(bref_length) / 8) - 1);
    /* Calculates the length of the encoded data. It calculates the distance between the bit stream that starts and ends at bref_length and converts it to the number of bytes.
    * -1 is to take into account the one bytes taken up by the length field itself.
    */

    if (length < 1) 
    {
    asn1::log_error("Encoding Failed (Payload container): Packed length (%d) is not in range of min: 1 bytes", length);
    return asn1::SRSASN_ERROR_ENCODE_FAIL;
    }
    HANDLE_CODE(bref_length.pack(length, 8));

    return SRSASN_SUCCESS;
}

SRSASN_CODE security_response_t::unpack(asn1::cbit_ref& bref)
{
    uint8_t length = 0; 
    HANDLE_CODE(bref.unpack(length, 8));
    // Decode 8 bits from bref using unpack () and store the result in the length variable. Gets the actual length of the security_response.

    // max. length of 255 not checked: uint overflow
    if (length < 1) {
    asn1::log_error("Decoding Failed (Payload container): Length (%d) is not in range of min: 1 bytes", length);
    return asn1::SRSASN_ERROR_DECODE_FAIL;
    }

    security_response.resize(length); // Adjust the size of the security_response content according to the decoded length.
    HANDLE_CODE(bref.unpack_bytes(security_response.data(), length));
    // Using unpack_bytes (), decode the specified length of bytes from bref and store the result in security_response.
    return SRSASN_SUCCESS;
}

// IE: Cause
// Reference: 8.3
SRSASN_CODE cause_t::pack(asn1::bit_ref& bref)
{
    HANDLE_CODE(cause.pack(bref));

    return SRSASN_SUCCESS;
}

SRSASN_CODE cause_t::unpack(asn1::cbit_ref& bref)
{
    HANDLE_CODE(cause.unpack(bref));

    return SRSASN_SUCCESS;
}
const char* cause_t::cause_type_::to_string()
{
    switch (value) {
    case cause_t::cause_type_::success:
      return "success";
    case cause_t::cause_type_::net_failure:
      return "net_failure";
    case cause_t::cause_type_::insufficient_resources:
      return "insufficient_resources";
    case cause_t::cause_type_::auth_failure:
      return "auth_failure";
    case cause_t::cause_type_::bearer_establishment_failure:
      return "bearer_establishment_failure";
    case cause_t::cause_type_::undefined_cause:
      return "undefined_cause";
    case cause_t::cause_type_::timer_timeout:
      return "timer_timeout";
    case cause_t::cause_type_::administrator_disables_service:
      return "administrator_disables_service";
    case cause_t::cause_type_::illegal_user:
      return "illegal_user";
    case cause_t::cause_type_::net_not_support_service:
      return "net_not_support_service";
    case cause_t::cause_type_::net_side_current_not_support_service:
      return "net_side_current_not_support_service";
    case cause_t::cause_type_::user_not_enable_service:
      return "user_not_enable_service";
    case cause_t::cause_type_::calling_user_no_permission:
      return "calling_user_no_permission";
    case cause_t::cause_type_::user_stopped_due_to_overpayment:
      return "user_stopped_due_to_overpayment";
    case cause_t::cause_type_::parameter_error:
      return "parameter_error";
    case cause_t::cause_type_::message_error:
      return "message_error";
    case cause_t::cause_type_::shutdown_logout:
      return "shutdown_logout";
    case cause_t::cause_type_::register_needs_reinitiated:
      return "register_needs_reinitiated";
    case cause_t::cause_type_::user_contract_data_delete_causing_logout:
      return "user_contract_data_delete_causing_logout";
    case cause_t::cause_type_::called_party_not_exist:
      return "called_party_not_exist";
    case cause_t::cause_type_::called_party_prohibited:
      return "called_party_prohibited";
    case cause_t::cause_type_::called_party_turns_off:
      return "called_party_turns_off";
    case cause_t::cause_type_::called_user_not_respond:
      return "called_user_not_respond";
    case cause_t::cause_type_::called_party_no_call_auth:
      return "called_party_no_call_auth";
    case cause_t::cause_type_::called_user_busy:
      return "called_user_busy";
    case cause_t::cause_type_::called_party_rings_user_not_answer:
      return "called_party_rings_user_not_answer";
    case cause_t::cause_type_::called_user_declines_answer:
      return "called_user_declines_answer";
    case cause_t::cause_type_::ue_call_released_normally:
      return "ue_call_released_normally";
    case cause_t::cause_type_::net_side_abnormal_call_release:
      return "net_side_abnormal_call_release";
    case cause_t::cause_type_::call_not_found:
      return "call_not_found";
    case cause_t::cause_type_::sms_memory_full:
      return "sms_memory_full";
    default:
      return "Invalid Choice";
    }
}

// IE: Expire
// Reference: 8.9
SRSASN_CODE expire_t::pack(asn1::bit_ref& bref)
{
    HANDLE_CODE(bref.pack(expire_value, 8));

    return SRSASN_SUCCESS;
}

SRSASN_CODE expire_t::unpack(asn1::cbit_ref& bref)
{
    HANDLE_CODE(bref.unpack(expire_value, 8));

    return SRSASN_SUCCESS;
}
// IE: DEREG_TYPE
// Reference: 8.8
SRSASN_CODE dereg_type_t::pack(asn1::bit_ref& bref)
{
    HANDLE_CODE(dereg_type.pack(bref));

    return SRSASN_SUCCESS;
}

SRSASN_CODE dereg_type_t::unpack(asn1::cbit_ref& bref)
{
    HANDLE_CODE(dereg_type.unpack(bref));

    return SRSASN_SUCCESS;
}

const char* dereg_type_t::dereg_type_type_::to_string()
{
    switch (value) {
    case dereg_type_t::dereg_type_type_::net_side_dereg:
      return "net_side_dereg";
    case dereg_type_t::dereg_type_type_::reauth_dereg:
      return "reauth_dereg";
    case dereg_type_t::dereg_type_type_::user_dereg:
      return "user_dereg";
    default:
      return "Invalid Choice";
    }
}

// IE: Nonce
// Reference: 8.13
// Comment refer to Security Response
SRSASN_CODE nonce_t::pack(asn1::bit_ref& bref)
{
    asn1::bit_ref bref_length = bref;
    HANDLE_CODE(bref.advance_bits(8)); 

    HANDLE_CODE(bref.pack(nonce_content.security_algorithm, 8)); 
    HANDLE_CODE(bref.pack_bytes(nonce_content.base64.data(), nonce_content.base64.size()));
    bref.align_bytes_zero(); 
    uint8_t length = (uint8_t)(ceilf((float)bref.distance(bref_length) / 8) - 1);

    if (length < 1) {
    asn1::log_error("Encoding Failed (Payload container): Packed length (%d) is not in range of min: 1 bytes", length);
    return asn1::SRSASN_ERROR_ENCODE_FAIL;
    }
    HANDLE_CODE(bref_length.pack(length, 8));

    return SRSASN_SUCCESS;
}

SRSASN_CODE nonce_t::unpack(asn1::cbit_ref& bref)
{
    uint8_t length = 0; 
    HANDLE_CODE(bref.unpack(length, 8)); 

    // max. length of 255 not checked: uint overflow
    if (length < 4 || length > 254) {
      asn1::log_error("Decoding Failed (nonce message): Length (%d) is not in range of min: 4 and max 255 bytes", length);
      return asn1::SRSASN_ERROR_DECODE_FAIL;
    }
    
    HANDLE_CODE(bref.unpack(nonce_content.security_algorithm, 8));
    nonce_content.base64.resize(length - 1);
    HANDLE_CODE(bref.unpack_bytes(nonce_content.base64.data(), length - 1));

    return SRSASN_SUCCESS;
}

// IE: Call ID
// Reference: 8.4
SRSASN_CODE call_id_t::pack(asn1::bit_ref& bref)
{
  HANDLE_CODE(bref.pack(call_id_value, 32));

  return SRSASN_SUCCESS;
}

SRSASN_CODE call_id_t::unpack(asn1::cbit_ref& bref)
{
  HANDLE_CODE(bref.unpack(call_id_value, 32));

  return SRSASN_SUCCESS;
}

// IE: Call type
// Reference: 8.7
SRSASN_CODE call_type_t::pack(asn1::bit_ref& bref)
{
    HANDLE_CODE(call_type.pack(bref));

    return SRSASN_SUCCESS;
}

SRSASN_CODE call_type_t::unpack(asn1::cbit_ref& bref)
{
    HANDLE_CODE(call_type.unpack(bref));

    return SRSASN_SUCCESS;
}

const char* call_type_t::call_type_type_::to_string()
{
    switch (value) {
    case call_type_t::call_type_type_::voice_call:
      return "voice_call";
    case call_type_t::call_type_type_::emergency_call:
      return "emergency_call";
    case call_type_t::call_type_type_::extension_bit:
      return "extension_bit";
    default:
      return "Invalid Choice";
    }
}

/*****************************************************/
// IE: Called Party BCD Number
// UE->Pcs
// Reference: 8.6
SRSASN_CODE called_party_bcd_num_t::pack(asn1::bit_ref& bref)
{
  HANDLE_CODE(bref.pack(called_party_bcd_num.size(), 8));//length
  HANDLE_CODE(bref.pack_bytes(called_party_bcd_num.data(), called_party_bcd_num.size()));
  return SRSASN_SUCCESS;
}

SRSASN_CODE called_party_bcd_num_t::unpack(asn1::cbit_ref& bref)
{
  uint8_t length = 0;
  HANDLE_CODE(bref.unpack(length, 8));//从 bref 中解包一个长度为 8 位的数据到变量 length 中。
  if (length < 4 || length > 254) {
    asn1::log_error("Decoding Failed (called_party_bcd_number message): Length (%d) is not in range of min: 4 and max 255 bytes", length);
    return asn1::SRSASN_ERROR_DECODE_FAIL;
  }
  called_party_bcd_num.resize(length);// 进行长度校验如果长度校验通过，就会调用 resize 函数调整 called_party_bcd_num 容器的大小为 length，以便存储后续解包得到的数据
  HANDLE_CODE(bref.unpack_bytes(called_party_bcd_num.data(), length));
  //使用了 bref 对象的 unpack_bytes 函数，从 bref 中解包 length 个字节的数据到 called_party_bcd_num 的内存空间中。
  return SRSASN_SUCCESS;
}

// IE: Calling Party BCD Number
// Pcs->UE
// Reference: 8.5
SRSASN_CODE calling_party_bcd_num_t::pack(asn1::bit_ref& bref)
{
  HANDLE_CODE(bref.pack(calling_party_bcd_num.size(), 8));
  HANDLE_CODE(bref.pack_bytes(calling_party_bcd_num.data(), calling_party_bcd_num.size()));
  return SRSASN_SUCCESS;
}

SRSASN_CODE calling_party_bcd_num_t::unpack(asn1::cbit_ref& bref)
{
  uint8_t length = 0;
  HANDLE_CODE(bref.unpack(length, 8));
  if (length < 4 || length > 254) {
    asn1::log_error("Decoding Failed (calling_party_bcd_number message): Length (%d) is not in range of min: 4 and max 255 bytes", length);
    return asn1::SRSASN_ERROR_DECODE_FAIL;
  }
  calling_party_bcd_num.resize(length);
  HANDLE_CODE(bref.unpack_bytes(calling_party_bcd_num.data(), length));
  return SRSASN_SUCCESS;
}

// IE: ttot call indicator
// Reference: 8.19
const char* ttot_call_indicator_t::ttot_call_indicator_type_::to_string()
{
  switch (value) {
    case ttot_call_indicator_t::ttot_call_indicator_type_::ttot_call_indicator:
      return "ttot_call_indicator";
    default:
      return "Invalid Choice";
  }
}

SRSASN_CODE ttot_call_indicator_t::pack(asn1::bit_ref& bref)
{
  HANDLE_CODE(ttot_call_indicator.pack(bref));
  return SRSASN_SUCCESS;
}

SRSASN_CODE ttot_call_indicator_t::unpack(asn1::cbit_ref& bref)
{
  HANDLE_CODE(ttot_call_indicator.unpack(bref));
  return SRSASN_SUCCESS;
}

// IE: call code rate
// Reference: 8.20
const char* call_code_rate_t::call_code_rate_type_::to_string()
{
  switch (value) {
    case call_code_rate_t::call_code_rate_type_::_2400bps:
      return "_2400bps";
    case call_code_rate_t::call_code_rate_type_::_4800bps:
      return "_4800bps";
    default:
      return "Invalid Choice";
  }
}

SRSASN_CODE call_code_rate_t::pack(asn1::bit_ref& bref)
{
  HANDLE_CODE(call_code_rate.pack(bref));
  //HANDLE_CODE(bref.pack(call_code_rate_value, 8));
  return SRSASN_SUCCESS;
}

SRSASN_CODE call_code_rate_t::unpack(asn1::cbit_ref& bref)
{
  HANDLE_CODE(call_code_rate.unpack(bref));
  //HANDLE_CODE(bref.unpack(call_code_rate_value, 8));
  return SRSASN_SUCCESS;
}

// IE: reference
// Reference: 8.23
SRSASN_CODE reference_t::pack(asn1::bit_ref& bref)
{
  HANDLE_CODE(bref.pack(reference, 8));
  return SRSASN_SUCCESS;
}

SRSASN_CODE reference_t::unpack(asn1::cbit_ref& bref)
{
  HANDLE_CODE(bref.unpack(reference, 8));
  return SRSASN_SUCCESS;
}

// IE: SMC party bcd number
// Reference: 8.21
SRSASN_CODE smc_party_bcd_num_t::pack(asn1::bit_ref& bref)
{
  HANDLE_CODE(bref.pack(smc_party_bcd_num.size(), 8));
  HANDLE_CODE(bref.pack_bytes(smc_party_bcd_num.data(), smc_party_bcd_num.size()));
  return SRSASN_SUCCESS;
}

SRSASN_CODE smc_party_bcd_num_t::unpack(asn1::cbit_ref& bref)
{
  uint8_t length = 0;
  HANDLE_CODE(bref.unpack(length, 8));
  if (length < 4 || length > 254) {
    asn1::log_error("Decoding Failed (sms_party_bcd_number_t message): Length (%d) is not in range of min: 4 and max 255 bytes", length);
    return asn1::SRSASN_ERROR_DECODE_FAIL;
  }
  smc_party_bcd_num.resize(length);
  HANDLE_CODE(bref.unpack_bytes(smc_party_bcd_num.data(), length));
  return SRSASN_SUCCESS;
}

// IE: Message Content
// Reference: 8.12
// Comment refer to Security Response
SRSASN_CODE message_content_t::pack(asn1::bit_ref& bref)
{
    asn1::bit_ref bref_length = bref;
    HANDLE_CODE(bref.advance_bits(16)); 
         
    HANDLE_CODE(bref.pack_bytes(message_content.data(), message_content.size()));
    bref.align_bytes_zero(); 
    uint8_t length = message_content.size();

    if (length < 1) {
    asn1::log_error("Encoding Failed (Payload container): Packed length (%d) is not in range of min: 1 bytes", length);
    return asn1::SRSASN_ERROR_ENCODE_FAIL;
    }
    HANDLE_CODE(bref_length.pack(length, 16));

    return SRSASN_SUCCESS;
}

SRSASN_CODE message_content_t::unpack(asn1::cbit_ref& bref)
{
    uint16_t length = 0; 
    HANDLE_CODE(bref.unpack(length, 16)); 

    // max. length of 255 not checked: uint overflow
    if (length < 1) {
    asn1::log_error("Decoding Failed (Payload container): Length (%d) is not in range of min: 1 bytes", length);
    return asn1::SRSASN_ERROR_DECODE_FAIL;
    }

    message_content.resize(length); 
    HANDLE_CODE(bref.unpack_bytes(message_content.data(), length));
    
    return SRSASN_SUCCESS;
}


} // namespace pcs_ims
} // namespace srsran