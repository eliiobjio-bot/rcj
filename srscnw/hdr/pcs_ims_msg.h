#ifndef SRSRAN_PCS_IMS_IES_H
#define SRSRAN_PCS_IMS_IES_H
#include "pcs_ims_ies.h"


namespace srsran {
namespace ims {
/*
 * Message: Voice register Request
 *          Based on XW IMS protocol 7.3.1
 */
class voice_register_req_t
{
public:
  // Mandatory fields 
  s_tmsi_5g_t          s_tmsi_5g;
  reg_type_t           reg_type_ims;
  security_indicator_t security_indicator_ims;
  security_algorithm_t security_algorithm_ims;

  // Conditional fields 
  imsi_t              imsi;
  ipaddr_t            ipaddr;
  security_response_t security_response;

  // Optional fields 

  const static uint8_t ipaddr_tag = 0x06;
  const static uint8_t security_response_tag = 0x05;

public:
  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);


}; // voice register request

/*
 * Message: Voice register Resp
 *          Based on XW IMS protocol  7.3.2
 */
class voice_register_resp_t
{
public:
  // Mandatory fields 
  s_tmsi_5g_t s_tmsi_5g;
  cause_t     cause_ims;

  // Conditional fields 
  expire_t expire;

  // Optional fields
  // impu

public:
  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);

}; // voice register resp

/*
 * Message: Voice DeRegister Req
 *          Based on XW IMS protocol 7.3.3
 */
class voice_deregister_req_t
{
public:
  // Mandatory fields 
  s_tmsi_5g_t          s_tmsi_5g;
  dereg_type_t         dereg_type_ims;
  cause_t              cause_ims;
  security_algorithm_t security_algorithm_ims;
  security_indicator_t security_indicator_ims;

  // Conditional fields 
  security_response_t security_response;

  // const static uint8_t security_response_tag = 0x05;
  uint8_t security_response_tag = 0x05;

public:
  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);

}; // voice deregister reqs

/*
 * Message: Voice DeRegister Resp
 *          Based on XW IMS protocol 7.3.4
 */
class voice_deregister_resp_t
{
public:
  // Mandatory fields 
  s_tmsi_5g_t s_tmsi_5g;

public:
  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);

}; // voice deregister resp

/*
 * Message: Authentication Command
 *          Based on XW IMS protocol 7.3.5
 */
class authentication_command_t
{
public:
  // Mandatory fields 
  s_tmsi_5g_t s_tmsi_5g;
  nonce_t     nonce;

  const static uint8_t nonce_tag            = 0x04;
  uint8_t nonce_length;
public:
  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);

}; // authentication command

/*
 * Message: Call Setup
 *          UE->PCS
 *          Based on XW IMS protocol 7.3.6.1
 */
class ue_pcs_call_setup_t
{
public:
  // Mandatory fields 
  call_id_t              call_id;
  s_tmsi_5g_t            s_tmsi_5g;
  call_type_t            call_type_ims;
  called_party_bcd_num_t called_party_bcd_num;
  ttot_call_indicator_t  ttot_call_indicator_ims;
  call_code_rate_t       call_code_rate_ims;

  const static uint8_t called_party_bcd_num_tag = 0x03;

public:
  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);

}; // call setup

/*
 * Message: Call Setup
 *          PCS->UE
 *          Based on XW IMSprotocol 7.3.6.2
 */
class pcs_ue_call_setup_t
{
public:
  // Mandatory fields 
  call_id_t               call_id;
  call_type_t             call_type_ims;
  calling_party_bcd_num_t calling_party_bcd_num;
  ttot_call_indicator_t   ttot_call_indicator_ims;
  call_code_rate_t        call_code_rate_ims;

    const static uint8_t calling_party_bcd_num_tag = 0x02;

public:
  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);

}; // call setup

/*
 * Message: Call Setup Ack
 *          Based on XW IMS protocol 7.3.7
 */
class call_setup_ack_t
{
public:
  // Mandatory fields 
  call_id_t        call_id;
  call_code_rate_t call_code_rate_ims;

public:
  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);

}; // call setup ack


/*
 * Message: Call Alerting
 *          UE?PCS
 *          Based on XW IMS protocol 7.3.8.1
 */
class call_confirmed_t
{
public:
  // Mandatory fields 
  call_id_t call_id;

public:
  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);

}; // call alerting


/*
 * Message: Call Alerting
 *          UE?PCS
 *          Based on XW IMS protocol 7.3.8.1
 */
class ue_pcs_call_alerting_t
{
public:
  // Mandatory fields 
  call_id_t call_id;

public:
  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);

}; // call alerting

/*
 * Message: Call Alerting
 *          PCS->UE
 *          Based on XW IMS protocol 7.3.8.2
 */
class pcs_ue_call_alerting_t
{
public:
  // Mandatory fields 
  call_id_t call_id;

public:
  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);

}; // call alerting

/*
 * Message: Call Connect
 *          UE->PCS
 *          Based on XW IMS?? 7.3.9.1
 */
class ue_pcs_call_connect_t
{
public:
  // Mandatory fields 
  call_id_t   call_id;
  call_type_t call_type_ims;

public:
  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);

}; // call connect

/*
 * Message: Call Connect
 *          PCS->UE
 *          Based on XW IMS protocol 7.3.9.2
 */
class pcs_ue_call_connect_t
{
public:
  // Mandatory fields 
  call_id_t   call_id;
  call_type_t call_type_ims;

public:
  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);

}; // call connect

/*
 * Message: Call Connect Ack
 *          Based on XW IMS protocol 7.3.10
 */
class call_connect_ack_t
{
public:
  // Mandatory fields 
  call_id_t call_id;

public:
  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);

}; // call connect

/*
 * Message: Call Disconnect
 *          UE->PCS
 *          Based on XW IMS protocol 7.3.11
 */
class call_disconnect_t
{
public:
  // Mandatory fields 
  call_id_t call_id;
  cause_t   cause_ims;

public:
  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);

}; // call disconnect

/*
 * Message: Call Release Req
 *          PCS->UE
 *          Based on XW IMS protocol 7.3.12
 */
class call_release_req_t
{
public:
  // Mandatory fields 
  call_id_t call_id;
  cause_t   cause_ims;

public:
  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);

}; // call release req

/*
 * Message: Call Release Resp
 *          UE->PCS
 *          Based on XW IMS protocol 7.3.13
 */
class call_release_resp_t
{
public:
  // Mandatory fields 
  call_id_t call_id;
  cause_t   cause_ims;

public:
  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);

}; // call release resp

/*
 * Message: MO SMS Req  UE sends SMS messages to PCS
 *          UE->PCS
 *          Based on XW IMS protocol 7.3.14
 */
class mo_sms_req_t
{
public:
  // Mandatory fields 
  call_id_t           call_id;
  reference_t         reference;
  s_tmsi_5g_t         s_tmsi_5g;
  smc_party_bcd_num_t smc_party_bcd_num;
  message_content_t   message_content;
  const static uint8_t smc_party_bcd_num_tag    = 0x08;
  const static uint8_t message_content_tag       = 0x01;

public:
  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);

}; // mo sms req


struct tp_ud_t
{
  uint8_t header_length;
  uint8_t identifier;
  uint8_t element_length;
  uint8_t reference_num;
  uint8_t max_num;
  uint8_t sequence_num;
  
};

class rp_data_msg_to_network_t   
{
public:

  uint8_t rp_msg_type;
  uint8_t rp_msg_reference;
  uint8_t rp_originator_address;
  uint8_t rp_destionation_address_len;
  std::vector<uint8_t> rp_destionation_address;

  //rp user data
  uint8_t rp_user_data_len;
  uint8_t rp_user_data_header;
  uint8_t tp_msg_type_indicator;
  uint8_t tp_reject_duplicates;
  uint8_t tp_validity_period_format;
  uint8_t tp_reply_path;
  uint8_t tp_user_data_header_indicator;
  uint8_t tp_status_reprot_request;

  uint8_t tp_msg_reference;
  uint8_t tp_destination_address_num;
  std::vector<uint8_t> tp_destination_address;

  uint8_t tp_protocol_indentifier;
  uint8_t tp_data_coding_form;
  uint8_t tp_validity_period;
  uint8_t tp_user_data_len;
  std::vector<uint8_t> tp_user_data;

  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);
  SRSASN_CODE unpack(const std::vector<uint8_t>& buf);


};


/*
 * Message: MO SMC Resp   The network side replies to the UE after receiving the SMS message
 *          PCS->UE
 *          Based on XW IMS protocol 7.3.15
 */
class mo_sms_resp_t
{
public:
  // Mandatory fields 
  call_id_t   call_id;
  reference_t reference;
  cause_t     cause_ims;

public:
  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);

}; // mo smc resp

/*
 * Message: MT SMC Req    The network side sends a short message to the UE
 *          UE?PCS
 *          Based on XW IMS protocol 7.3.16
 */
class mt_sms_req_t
{
public:
  // Mandatory fields 
  call_id_t           call_id;
  smc_party_bcd_num_t smc_party_bcd_num;
  message_content_t   message_content;

  const static uint8_t smc_party_bcd_num_tag = 0x08;
  const static uint8_t message_content_tag    = 0x01;

public:
  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);

}; // mt sms req

/*
 * Message: MT SMC Resp The network side replies to the UE after receiving the SMS message
 *          UE->PCS
 *          Based on XW IMS protocol 7.3.17
 */
class mt_sms_resp_t
{
public:
  // Mandatory fields 
  call_id_t   call_id;
  s_tmsi_5g_t s_tmsi_5g;
  cause_t     cause_ims;

public:
  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);

}; // mt smc resp

// Reference: 8.1 Message Type
struct ims_msg_types {
  enum options {
    voice_register_req        = 0x01,
    authentication_command    = 0x02,
    voice_register_resp       = 0x03,
    voice_deregister_req      = 0x04,
    voice_deregister_resp     = 0x05,
    call_setup                = 0x06,
    call_setup_ack            = 0x07,
    call_alerting             = 0x08,
    call_connect              = 0x09,
    call_connect_ack          = 0x0a,
    call_disconnect           = 0x0b,
    call_release_req          = 0x0c,
    call_release_resp         = 0x0d,
    call_hold                 = 0x0e,
    call_hold_ack             = 0x0f,
    call_wait_indication      = 0x10,
    call_wait_indication_resp = 0x11,
    mo_sms_req                = 0x12,
    mo_sms_resp               = 0x13,
    mt_sms_req                = 0x14,
    mt_sms_resp               = 0x15,
    call_confirmed            = 0x16,
    nulltype                  = 0xff,

  } value;
  const char* to_string() const
  {
    switch (value) {
      case voice_register_req:
        return "Voice register regquest";
      case authentication_command:
        return "Authentication Command";
      case voice_register_resp:
        return "Voice Register Resp";
      case voice_deregister_req:
        return "Voice DeRegister Req";
      case voice_deregister_resp:
        return "Voice DeRegister Resp";
      case call_setup:
        return "Call Setup";
      case call_setup_ack:
        return "Call Setup ack";
      case call_alerting:
        return "Call Alerting";
      case call_connect:
        return "Call Connect";
      case call_connect_ack:
        return "Call Connect Ack";
      case call_disconnect:
        return "Call Disconnect";
      case call_release_req:
        return "Call Release Req";
      case call_release_resp:
        return "Call Release Resp";
      case call_hold:
        return "Call Hold";
      case call_hold_ack:
        return "Call Hold Ack";
      case call_wait_indication:
        return "Call Wait Indication";
      case call_wait_indication_resp:
        return "Call Wait Indication Rsp";
      case mo_sms_req:
        return "MO SMC Req";
      case mo_sms_resp:
        return "MO SMC Rsp";
      case mt_sms_req:
        return "MT SMC Req";
      case mt_sms_resp:
        return "MT SMC Rsp";
      default:
        return "Error";
    }
  }
};

typedef asn1::enumerated<ims_msg_types> msg_type;
// IMS Message Header
struct pcs_ims_hdr {
  // Protocol version
  enum protocol_version_options {
    test_version    = 0b0000,
    initial_version = 0b0001,
  };
  // Rev
  enum extended_protocol_discriminator_options {
    extended_protocol_discriminator_pcs = 0b0000,
  };

  protocol_version_options                protocol_version                = test_version;
  extended_protocol_discriminator_options extended_protocol_discriminator = extended_protocol_discriminator_pcs;
  // Message length
  uint8_t message_length;
  // Message type
  msg_type ims_message_type = ims_msg_types::options::voice_register_req;

  SRSASN_CODE pack(asn1::bit_ref& bref);
  SRSASN_CODE unpack(asn1::cbit_ref& bref);
};

class pcs_ims_msg
{
public:
  pcs_ims_hdr hdr;

  SRSASN_CODE pack(unique_byte_buffer_t& buf);
  SRSASN_CODE pack(std::vector<uint8_t>& buf);
  SRSASN_CODE unpack(const unique_byte_buffer_t& buf);
  SRSASN_CODE unpack(const std::vector<uint8_t>& buf);

/*
* The purpose of set () is to set the message_type field of the nas_5gs_hdr struct in the nas_5gs_msg class.
* Set () provides flexibility by allowing the value of msg_types::options to be specified, 
* if no value is provided, it defaults to msg_types::voice_register_req.
*/
  void set(ims_msg_types::options e = ims_msg_types::voice_register_req)
  { 
    hdr.ims_message_type = e; 
    // Assigns the value of parameter e to the message_type field of the hdr member variable
  };

  // Getters
/*
 * register_request() is used to get a reference of type register_request_t,
 * And asserts that the value of hdr.message_type is msg_type::options::voice_register_req before getting it.
 * Ensure that the extracted objects are used safely.
 */
  voice_register_req_t& voice_register()
  {
    asn1::assert_choice_type(msg_type::options::voice_register_req, hdr.ims_message_type, "voice_registation_req");
/* 
 * assert_choice_type() belongs to the asn1 namespace.
 * Action: Asserts whether the value of hdr.message_type is equal to msg_types::options::voice_register_req,
 * If not, an exception will be raised. This ensures that the value of message_type is the same as the function expects.
 */
    return *srslog::detail::any_cast<voice_register_req_t>(&msg_container); // Get the address of msg_container
/* Returns a reference to an object of type voice_register_req_t.
 * srslog::detail::any_cast Utility function used to extract an object of a specific type from the generic pointer any*.
 * Here, it is used to extract an object of type voice_register_req_t from msg_container.
 * srslog::detail::any_cast<voice_register_req_t> : Attempts to convert an object in msg_container to type voice_register_req_t. 
 * This assumes that the previous assertion has ensured the correctness of the message_type.
 */
  }

  voice_register_resp_t& voice_register_resp()
  {
    asn1::assert_choice_type(msg_type::options::voice_register_resp, hdr.ims_message_type, "voice_register_resp");
    return *srslog::detail::any_cast<voice_register_resp_t>(&msg_container);
  }

  voice_deregister_req_t& voice_deregister_req()
  {
    asn1::assert_choice_type(msg_type::options::voice_deregister_req, hdr.ims_message_type, "voice_deregister_req");
    return *srslog::detail::any_cast<voice_deregister_req_t>(&msg_container);
  }

  voice_deregister_resp_t& voice_deregister_resp()
  {
    asn1::assert_choice_type(msg_type::options::voice_deregister_resp, hdr.ims_message_type, "voice_deregister_resp");
    return *srslog::detail::any_cast<voice_deregister_resp_t>(&msg_container);
  }

  authentication_command_t& authentication_command()
  {
    asn1::assert_choice_type(msg_type::options::authentication_command, hdr.ims_message_type, "authentication_command");
    return *srslog::detail::any_cast<authentication_command_t>(&msg_container);
  }

  ue_pcs_call_setup_t& ue_pcs_call_setup()
  {
    asn1::assert_choice_type(msg_type::options::call_setup, hdr.ims_message_type, "call_setup");
    return *srslog::detail::any_cast<ue_pcs_call_setup_t>(&msg_container);
  }

  pcs_ue_call_setup_t& pcs_ue_call_setup()
  {
    asn1::assert_choice_type(msg_type::options::call_setup, hdr.ims_message_type, "call_setup");
    return *srslog::detail::any_cast<pcs_ue_call_setup_t>(&msg_container);
  }

  call_setup_ack_t& call_setup_ack()
  {
    asn1::assert_choice_type(msg_type::options::call_setup_ack, hdr.ims_message_type, "call_setup_ack");
    return *srslog::detail::any_cast<call_setup_ack_t>(&msg_container);
  }

  call_confirmed_t& call_confirmed()
  {
    asn1::assert_choice_type(msg_type::options::call_confirmed, hdr.ims_message_type, "call_alerting");
    return *srslog::detail::any_cast<call_confirmed_t>(&msg_container);
  }

  ue_pcs_call_alerting_t& ue_pcs_call_alerting()
  {
    asn1::assert_choice_type(msg_type::options::call_alerting, hdr.ims_message_type, "call_alerting");
    return *srslog::detail::any_cast<ue_pcs_call_alerting_t>(&msg_container);
  }

  pcs_ue_call_alerting_t& pcs_ue_call_alerting()
  {
    asn1::assert_choice_type(msg_type::options::call_alerting, hdr.ims_message_type, "call_alerting");
    return *srslog::detail::any_cast<pcs_ue_call_alerting_t>(&msg_container);
  }

  ue_pcs_call_connect_t& ue_pcs_call_connect()
  {
    asn1::assert_choice_type(msg_type::options::call_connect, hdr.ims_message_type, "call_connect");
    return *srslog::detail::any_cast<ue_pcs_call_connect_t>(&msg_container);
  }

  pcs_ue_call_connect_t& pcs_ue_call_connect()
  {
    asn1::assert_choice_type(msg_type::options::call_connect, hdr.ims_message_type, "call_connect");
    return *srslog::detail::any_cast<pcs_ue_call_connect_t>(&msg_container);
  }

  call_connect_ack_t& call_connect_ack()
  {
    asn1::assert_choice_type(msg_type::options::call_connect_ack, hdr.ims_message_type, "call_connect_ack");
    return *srslog::detail::any_cast<call_connect_ack_t>(&msg_container);
  }

  call_disconnect_t& call_disconnect()
  {
    asn1::assert_choice_type(msg_type::options::call_disconnect, hdr.ims_message_type, "call_disconnect");
    return *srslog::detail::any_cast<call_disconnect_t>(&msg_container);
  }

  call_release_req_t& call_release_req()
  {
    asn1::assert_choice_type(msg_type::options::call_release_req, hdr.ims_message_type, "call_release_req");
    return *srslog::detail::any_cast<call_release_req_t>(&msg_container);
  }

  call_release_resp_t& call_release_resp()
  {
    asn1::assert_choice_type(msg_type::options::call_release_resp, hdr.ims_message_type, "call_release_resp");
    return *srslog::detail::any_cast<call_release_resp_t>(&msg_container);
  }

  mo_sms_req_t& mo_sms_req()
  {
    asn1::assert_choice_type(msg_type::options::mo_sms_req, hdr.ims_message_type, "mo_sms_req");
    return *srslog::detail::any_cast<mo_sms_req_t>(&msg_container);
  }

  mo_sms_resp_t& mo_sms_resp()
  {
    asn1::assert_choice_type(msg_type::options::mo_sms_resp, hdr.ims_message_type, "mo_sms_resp");
    return *srslog::detail::any_cast<mo_sms_resp_t>(&msg_container);
  }

  mt_sms_req_t& mt_sms_req()
  {
    asn1::assert_choice_type(msg_type::options::mt_sms_req, hdr.ims_message_type, "mt_sms_req");
    return *srslog::detail::any_cast<mt_sms_req_t>(&msg_container);
  }

  mt_sms_resp_t& mt_sms_resp()
  {
    asn1::assert_choice_type(msg_type::options::mt_sms_resp, hdr.ims_message_type, "mt_sms_resp");
    return *srslog::detail::any_cast<mt_sms_resp_t>(&msg_container);
  }

  // Setters
/*
 * This function is mainly used to set values in a message container, msg_container, and return references to objects stored in it
 * 1. Call set() and set the message type to voice_register_req_t
 * 2. Set the protocol discriminator to extended_protocol_discriminator_pcs
 * 3. Create an object of type voice_register_req_t and place it into the message container msg_container.
 * The srslog::detail::any type is used here to indicate that msg_container can hold objects of different types.
 * 4. Use the srslog::detail::any_cast function to retrieve the stored voice_register_req_t object from the message container and return a reference to the object
 */
  voice_register_req_t& set_voice_register_req()
  {
    set(ims_msg_types::options::voice_register_req);
    hdr.extended_protocol_discriminator = pcs_ims_hdr::extended_protocol_discriminator_pcs;
    msg_container                       = srslog::detail::any{voice_register_req_t()};
    return *srslog::detail::any_cast<voice_register_req_t>(&msg_container);
  }

  voice_register_resp_t& set_voice_register_resp()
  {
    set(ims_msg_types::options::voice_register_resp);
    hdr.extended_protocol_discriminator = pcs_ims_hdr::extended_protocol_discriminator_pcs;
    msg_container                       = srslog::detail::any{voice_register_resp_t()};
    return *srslog::detail::any_cast<voice_register_resp_t>(&msg_container);
  }

  voice_deregister_req_t& set_voice_deregister_req()
  {
    set(ims_msg_types::options::voice_deregister_req);
    hdr.extended_protocol_discriminator = pcs_ims_hdr::extended_protocol_discriminator_pcs;
    msg_container                       = srslog::detail::any{voice_deregister_req_t()};
    return *srslog::detail::any_cast<voice_deregister_req_t>(&msg_container);
  }

  voice_deregister_resp_t& set_voice_deregister_resp()
  {
    set(ims_msg_types::options::voice_deregister_resp);
    hdr.extended_protocol_discriminator = pcs_ims_hdr::extended_protocol_discriminator_pcs;
    msg_container                       = srslog::detail::any{voice_deregister_resp_t()};
    return *srslog::detail::any_cast<voice_deregister_resp_t>(&msg_container);
  }

  authentication_command_t& set_authentication_command()
  {
    set(ims_msg_types::options::authentication_command);
    hdr.extended_protocol_discriminator = pcs_ims_hdr::extended_protocol_discriminator_pcs;
    msg_container                       = srslog::detail::any{authentication_command_t()};
    return *srslog::detail::any_cast<authentication_command_t>(&msg_container);
  }

  ue_pcs_call_setup_t& set_ue_pcs_call_setup()
  {
    set(ims_msg_types::options::call_setup);
    hdr.extended_protocol_discriminator = pcs_ims_hdr::extended_protocol_discriminator_pcs;
    msg_container                       = srslog::detail::any{ue_pcs_call_setup_t()};
    return *srslog::detail::any_cast<ue_pcs_call_setup_t>(&msg_container);
  }

  pcs_ue_call_setup_t& set_pcs_ue_call_setup()
  {
    set(ims_msg_types::options::call_setup);
    hdr.extended_protocol_discriminator = pcs_ims_hdr::extended_protocol_discriminator_pcs;
    msg_container                       = srslog::detail::any{pcs_ue_call_setup_t()};
    return *srslog::detail::any_cast<pcs_ue_call_setup_t>(&msg_container);
  }

  call_setup_ack_t& set_call_setup_ack()
  {
    set(ims_msg_types::options::call_setup_ack);
    hdr.extended_protocol_discriminator = pcs_ims_hdr::extended_protocol_discriminator_pcs;
    msg_container                       = srslog::detail::any{call_setup_ack_t()};
    return *srslog::detail::any_cast<call_setup_ack_t>(&msg_container);
  }

  call_confirmed_t& set_call_confirmed()
  {
    set(ims_msg_types::options::call_confirmed);
    hdr.extended_protocol_discriminator = pcs_ims_hdr::extended_protocol_discriminator_pcs;
    msg_container                       = srslog::detail::any{call_confirmed_t()};
    return *srslog::detail::any_cast<call_confirmed_t>(&msg_container);
  }

  ue_pcs_call_alerting_t& set_ue_pcs_call_alerting()
  {
    set(ims_msg_types::options::call_alerting);
    hdr.extended_protocol_discriminator = pcs_ims_hdr::extended_protocol_discriminator_pcs;
    msg_container                       = srslog::detail::any{ue_pcs_call_alerting_t()};
    return *srslog::detail::any_cast<ue_pcs_call_alerting_t>(&msg_container);
  }

  pcs_ue_call_alerting_t& set_pcs_ue_call_alerting()
  {
    set(ims_msg_types::options::call_alerting);
    hdr.extended_protocol_discriminator = pcs_ims_hdr::extended_protocol_discriminator_pcs;
    msg_container                       = srslog::detail::any{pcs_ue_call_alerting_t()};
    return *srslog::detail::any_cast<pcs_ue_call_alerting_t>(&msg_container);
  }

  ue_pcs_call_connect_t& set_ue_pcs_call_connect()
  {
    set(ims_msg_types::options::call_connect);
    hdr.extended_protocol_discriminator = pcs_ims_hdr::extended_protocol_discriminator_pcs;
    msg_container                       = srslog::detail::any{ue_pcs_call_connect_t()};
    return *srslog::detail::any_cast<ue_pcs_call_connect_t>(&msg_container);
  }

  pcs_ue_call_connect_t& set_pcs_ue_call_connect()
  {
    set(ims_msg_types::options::call_connect);
    hdr.extended_protocol_discriminator = pcs_ims_hdr::extended_protocol_discriminator_pcs;
    msg_container                       = srslog::detail::any{pcs_ue_call_connect_t()};
    return *srslog::detail::any_cast<pcs_ue_call_connect_t>(&msg_container);
  }

  call_connect_ack_t& set_call_connect_ack()
  {
    set(ims_msg_types::options::call_connect_ack);
    hdr.extended_protocol_discriminator = pcs_ims_hdr::extended_protocol_discriminator_pcs;
    msg_container                       = srslog::detail::any{call_connect_ack_t()};
    return *srslog::detail::any_cast<call_connect_ack_t>(&msg_container);
  }

  call_disconnect_t& set_call_disconnect()
  {
    set(ims_msg_types::options::call_disconnect);
    hdr.extended_protocol_discriminator = pcs_ims_hdr::extended_protocol_discriminator_pcs;
    msg_container                       = srslog::detail::any{call_disconnect_t()};
    return *srslog::detail::any_cast<call_disconnect_t>(&msg_container);
  }

  call_release_req_t& set_call_release_req()
  {
    set(ims_msg_types::options::call_release_req);
    hdr.extended_protocol_discriminator = pcs_ims_hdr::extended_protocol_discriminator_pcs;
    msg_container                       = srslog::detail::any{call_release_req_t()};
    return *srslog::detail::any_cast<call_release_req_t>(&msg_container);
  }

  call_release_resp_t& set_call_release_resp()
  {
    set(ims_msg_types::options::call_release_resp);
    hdr.extended_protocol_discriminator = pcs_ims_hdr::extended_protocol_discriminator_pcs;
    msg_container                       = srslog::detail::any{call_release_resp_t()};
    return *srslog::detail::any_cast<call_release_resp_t>(&msg_container);
  }

  mo_sms_req_t& set_mo_sms_req()
  {
    set(ims_msg_types::options::mo_sms_req);
    hdr.extended_protocol_discriminator = pcs_ims_hdr::extended_protocol_discriminator_pcs;
    msg_container                       = srslog::detail::any{mo_sms_req_t()};
    return *srslog::detail::any_cast<mo_sms_req_t>(&msg_container);
  }

  mo_sms_resp_t& set_mo_sms_resp()
  {
    set(ims_msg_types::options::mo_sms_resp);
    hdr.extended_protocol_discriminator = pcs_ims_hdr::extended_protocol_discriminator_pcs;
    msg_container                       = srslog::detail::any{mo_sms_resp_t()};
    return *srslog::detail::any_cast<mo_sms_resp_t>(&msg_container);
  }

  mt_sms_req_t& set_mt_sms_req()
  {
    set(ims_msg_types::options::mt_sms_req);
    hdr.extended_protocol_discriminator = pcs_ims_hdr::extended_protocol_discriminator_pcs;
    msg_container                       = srslog::detail::any{mt_sms_req_t()};
    return *srslog::detail::any_cast<mt_sms_req_t>(&msg_container);
  }

  mt_sms_resp_t& set_mt_sms_resp()
  {
    set(ims_msg_types::options::mt_sms_resp);
    hdr.extended_protocol_discriminator = pcs_ims_hdr::extended_protocol_discriminator_pcs;
    msg_container                       = srslog::detail::any{mt_sms_resp_t()};
    return *srslog::detail::any_cast<mt_sms_resp_t>(&msg_container);
  }

private:
  SRSASN_CODE         unpack(asn1::cbit_ref& bref);
  SRSASN_CODE         pack(asn1::bit_ref& bref);
  srslog::detail::any msg_container = srslog::detail::any{voice_register_req_t()};
};
} // namespace pcs_ims
} // namespace srsran
#endif 