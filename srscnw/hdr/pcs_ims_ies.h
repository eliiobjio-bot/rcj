#ifndef PCS_IMS_IES_H
#define PCS_IMS_IES_H
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
// IE: 5G-S-TMSI
// Reference: 8.18
class s_tmsi_5g_t 
{
public:
  std::array<uint8_t, 6> s_tmsi_5g_value;
  //uint64_t s_tmsi_5g_value; 

  SRSASN_CODE pack(asn1::bit_ref& bref); 
  SRSASN_CODE unpack(asn1::cbit_ref& bref);//?
}; // s_tmsi_5g

// IE: REG_TYPE
// Reference: 8.15
class reg_type_t
{
public:
  struct reg_type_type_ {
    enum options {
      poweron_register           = 0x00,
      periodic_register_updating = 0x01,

    } value;
    const char* to_string();
  };
  typedef srsran::nas_5g::nas_enumerated<reg_type_type_, 8> reg_type_type;

  reg_type_type                             reg_type = reg_type_type_::options::poweron_register;

  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);
}; // reg_type_t

// IE: IMSI
// Reference: 8.10
class imsi_t
{
public:
  std::array<uint8_t, 16> imsi;

  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);
}; // imsi See 5GMM imei and imeisv codecs

// IE: IP Addr
// Reference: 8.11
class ipaddr_t
{
public:
  uint8_t  ipaddr_tag;
  std::array<uint8_t, 4> ipaddr_value;
  //uint32_t ipaddr_value;

  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);
}; // ipaddr

// IE: Security indicator
// Reference: 8.22
class security_indicator_t
{
public:
  struct security_indicator_type_ {
    enum options {
      first_reg_or_dereg_req_no_security_resp                            = 0x01,
      ue_auth_net_mac_incompatible_req_no_security_resp                  = 0x02,
      ue_auth_net_mac_compatible_sqn_incompatible_req_security_resp_auts = 0x03,
      ue_auth_net_mac_compatible_sqn_compatible_req_security_resp_res    = 0x04,
    } value;
    const char* to_string();
  };
  typedef srsran::nas_5g::nas_enumerated<security_indicator_type_, 8> security_indicator_type;

  security_indicator_type security_indicator =
      security_indicator_type_::options::first_reg_or_dereg_req_no_security_resp;

  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);
}; // security_indicator

// IE: Security algorithm
// Reference: 8.17
class security_algorithm_t
{
public:
  struct security_algorithm_type_ {
    enum options {
      aka       = 0x00,
      akav1_md5 = 0x01,
      md5       = 0x02,

    } value;
    const char* to_string();
  };
  typedef srsran::nas_5g::nas_enumerated<security_algorithm_type_, 8> security_algorithm_type;

  security_algorithm_type security_algorithm = security_algorithm_type_::options::aka;
  SRSASN_CODE             pack(asn1::bit_ref& bref);
  SRSASN_CODE             unpack(asn1::cbit_ref& bref);
}; // security_algorithm

// IE: Security Response
// Reference: 8.16
class security_response_t
{
public:
  uint8_t security_response_tag;
  std::vector<uint8_t> security_response;

  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);
}; // security_response

// IE: Cause
// Reference: 8.3
class cause_t
{
public:
  struct cause_type_ {
    enum options {
      success                                  = 0x00,
      net_failure                              = 0x01,
      insufficient_resources                   = 0x02,
      auth_failure                             = 0x03,
      bearer_establishment_failure             = 0x04,
      undefined_cause                          = 0x05,
      timer_timeout                            = 0x06,
      administrator_disables_service           = 0x07,
      illegal_user                             = 0x08,
      net_not_support_service                  = 0x09,
      net_side_current_not_support_service     = 0x0A,
      user_not_enable_service                  = 0x0B,
      calling_user_no_permission               = 0x0C,
      user_stopped_due_to_overpayment          = 0x0D,
      parameter_error                          = 0x0E,
      message_error                            = 0x0F,
      shutdown_logout                          = 0x10,
      register_needs_reinitiated               = 0x11,
      user_contract_data_delete_causing_logout = 0x12,
      called_party_not_exist                   = 0x13,
      called_party_prohibited                  = 0x14,
      called_party_turns_off                   = 0x15,
      called_user_not_respond                  = 0x16,
      called_party_no_call_auth                = 0x17,
      called_user_busy                         = 0x18,
      called_party_rings_user_not_answer       = 0x19,
      called_user_declines_answer              = 0x1A,
      ue_call_released_normally                = 0x1B,
      net_side_abnormal_call_release           = 0x1C,
      call_not_found                           = 0x1D,
      sms_memory_full                          = 0x1E,
    } value;
    const char* to_string();
  };
  typedef srsran::nas_5g::nas_enumerated<cause_type_, 8> cause_type;

  cause_type  cause = cause_type_::options::success;
  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);
}; // cause

// IE: Expire
// Reference: 8.9
class expire_t
{
public:
  uint8_t expire_value;

  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);
}; // expire

// IE: DEREG_TYPE
// Reference: 8.8
class dereg_type_t
{
public:
  struct dereg_type_type_ {
    enum options {
      net_side_dereg = 0x00,
      reauth_dereg   = 0x01,
      user_dereg     = 0x02,
    } value;
    const char* to_string();
  };
  typedef srsran::nas_5g::nas_enumerated<dereg_type_type_, 8> dereg_type_type;

  dereg_type_type dereg_type = dereg_type_type_::options::net_side_dereg;
  SRSASN_CODE     pack(asn1::bit_ref& bref);
  SRSASN_CODE     unpack(asn1::cbit_ref& bref);
}; // dereg_type

// IE: Nonce
// Reference: 8.13
class nonce_t
{
public:
struct nonce_parameter_contents_ {
    uint8_t     security_algorithm;
    std::vector<uint8_t> base64;
  };

  nonce_parameter_contents_ nonce_content;

  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);
}; // nonce

// IE: Call ID
// Reference: 8.4
class call_id_t
{
public:
  uint32_t call_id_value;

  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);
}; // call_id

// IE: Call type
// Reference: 8.7
class call_type_t
{
public:
  struct call_type_type_ {
    enum options {
      voice_call     = 0x01,
      emergency_call = 0x02,
      extension_bit  = 0x03,
    } value;
    const char* to_string();
  };
  typedef srsran::nas_5g::nas_enumerated<call_type_type_, 8> call_type_type;

  call_type_type call_type = call_type_type_::options::voice_call;
  SRSASN_CODE    pack(asn1::bit_ref& bref);
  SRSASN_CODE    unpack(asn1::cbit_ref& bref);
}; // call_type

// IE: Called Party BCD Number
// UE->Pcs
// Reference: 8.6
class called_party_bcd_num_t
{
public:
  //uint8_t called_party_bcd_num_tag;
  //std::array<uint8_t, 24> called_party_bcd_num;
  std::vector<uint8_t> called_party_bcd_num;
  //uint8_t end_mark = 00001111;

  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);
}; // Called Party BCD Number

// IE: Calling Party BCD Number
// Pcs->UE
// Reference: 8.5
class calling_party_bcd_num_t
{
public:
  std::vector<uint8_t> calling_party_bcd_num;

  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);
}; // Calling Party BCD Number

// IE: TtoT Call indicator
// Reference: 8.19
class ttot_call_indicator_t
{
public:
  struct ttot_call_indicator_type_ {
    enum options {
      ttot_call_indicator = 0x01,
    } value;
    const char* to_string();
  };
  typedef srsran::nas_5g::nas_enumerated<ttot_call_indicator_type_, 8> ttot_call_indicator_type;

  ttot_call_indicator_type ttot_call_indicator = ttot_call_indicator_type_::options::ttot_call_indicator;
  SRSASN_CODE              pack(asn1::bit_ref& bref);
  SRSASN_CODE              unpack(asn1::cbit_ref& bref);
}; // TtoT Call indicator

// IE: Call Code rate
// Reference: 8.20
class call_code_rate_t
{
public:
  struct call_code_rate_type_ {
    enum options {
      _2400bps = 0x01,
      _4800bps = 0x02,
      _800bps  = 0x03,
    } value;
    const char* to_string();
  };
  typedef srsran::nas_5g::nas_enumerated<call_code_rate_type_, 8> call_code_rate_type;

  call_code_rate_type call_code_rate = call_code_rate_type_::options::_2400bps;
  //uint8_t call_code_rate_value;

  SRSASN_CODE         pack(asn1::bit_ref& bref);
  SRSASN_CODE         unpack(asn1::cbit_ref& bref);
}; // Call Code rate

// IE: Reference
// Reference: 8.23
class reference_t
{
public:
  uint8_t reference;

  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);
}; // reference

// IE: SMC Party BCD Number
// Reference: 8.21
class smc_party_bcd_num_t
{
public:
  std::vector<uint8_t> smc_party_bcd_num;

  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);
}; // smc_party_bcd_num

// IE: Message Content
// Reference: 8.12
class message_content_t
{
public:
  std::vector<uint8_t> message_content;

  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);
}; // message content

// IE: ie code
// Reference: 8.2
class ie_code_t
{
public:
  struct ie_code_type_ {
    enum options {
      message_content        = 0x01,
      calling_party_bcd_num  = 0x02,
      called_party_bcd_num   = 0x03,
      nonce                  = 0x04,
      security_response      = 0x05,
      ip_addr                = 0x06,
      e2e_security_container = 0x07,
      smc_party_bcd_num      = 0x08,
    } value;
    const char* to_string();
  };
  typedef srsran::nas_5g::nas_enumerated<ie_code_type_, 8> ie_code_type;

  ie_code_type ie_code = ie_code_type_::options::message_content;
  SRSASN_CODE  pack(asn1::bit_ref& bref);
  SRSASN_CODE  unpack(asn1::cbit_ref& bref);
};

} // namespace pcs_ims
} // namespace srsran
#endif 