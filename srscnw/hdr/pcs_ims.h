/******************************************************************************
 * File:        nas_mm.h
 * Description: Top-level NAS MM class. Creates and links all
 *              interfaces and helpers.
 *****************************************************************************/

#ifndef SRSEPC_PCS_IMS_H
#define SRSEPC_PCS_IMS_H

#include "srscnw/hdr/pcs_ims_ies.h"
#include "srscnw/hdr/pcs_ims_msg.h"
#include "srsran/common/buffer_pool.h"
#include "srsran/common/standard_streams.h"
#include "srsran/common/threads.h"
#include "srsran/asn1/s1ap_utils.h"
#include <cstddef>
#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include "srscnw/hdr/adp.h"

#include "lib/include/srsran/interfaces/enb_rrc_interfaces.h" //-------2023/11/14

using namespace srsran;
using namespace srsran::ims;
using namespace std;

#define FIRST_REG_OR_DEREG	1

namespace srsepc {

class cnw;

typedef enum {
  IMS_STATE_DEREGISTERED = 0,
  IMS_STATE_REGISTERED,
  IMS_STATE_DEREGISTERED_INITIATED,
  IMS_STATE_REGISTERED_INITIATED,
  IMS_STATE_MAX,
} ims_state_t;

typedef enum {
	NULL_TYPE =0,
	MO_CALL_TYPE,
	MT_CALL_TYPE,
} call_status_type_t;

typedef enum {
    null_    = 0x00,
    set_up_mt_call  ,
    release_mt_call ,
    call_code_rate,
} ate_indicate_type_t;

typedef enum {
  TC_IMS_MSG_NULL,   
  TC_MSG_IMS_ATE_REGISTER = 1, // IMS reports registration status to the test manager
  TC_MSG_IMS_ATE_MO_CALL = 2, // IMS reports MO call situation to the test manager
  TC_MSG_IMS_ATE_MT_CALL = 3, // IMS reports MT call situation to the test manager
  TC_MSG_ATE_IMS_MT_CALL = 4, // Test manager instructs MT call operation
  TC_MSG_ATE_IMS_MT_CALL_RES = 5, // Protocol stack responds to MT call operation instruction
  TC_MSG_IMS_ATE_MO_SMS = 6, // Protocol stack feedback SMS content to the main control software
  TC_MSG_ATE_IMS_MT_SMS = 7, // Main control instructs protocol stack to send SMS to the terminal
  TC_MSG_ATE_IMS_MT_SMS_RES,
  TC_IMS_MSG_MAX,
} TC_IMS_MSG;

enum XW_SMS{SHORT_SMS, LONG_SMS};

typedef struct {
  ims_state_t  state = IMS_STATE_DEREGISTERED;
  uint8_t 	   call_status_type=NULL_TYPE;
  uint8_t	     call_code_rate=1;// 1:2.4k 2:4.8k 3:800bps
  uint16_t     imsi;
  uint16_t     rnti;
  cause_t::cause_type  cause;
  s_tmsi_5g_t  s_tmsi_5g;
  smc_party_bcd_num_t smc_party_bcd_num;
  uint8_t      sms_code_form;
  vector<uint8_t>  ate_mt_sms_content;
  message_content_t message_content;
  uint8_t      reference;
  uint32_t     call_id;
  uint32_t	   call_id_sms=2; 
  bool         is_first_reg_msg = 0;

  uint8_t ate_indication=null_;

  unordered_map<int, vector<int>> tmp_long_message_content;
  vector<uint8_t> ate_mt_long_message_content;
  int long_sms_reference_num = 0x99;
  int long_sms_max_num = 0x00;
  int long_sms_sn = 0x00;
  uint8_t sms_type = SHORT_SMS;

} ims_ctx_t;

typedef struct {
  uint8_t                             k_amf[32];
  uint8_t                             k_ausf[32];
  uint8_t                             k_seaf[32];
  uint8_t                             autn[16];
  uint8_t                             rand[16];
  uint8_t                             xres[16];
  uint8_t                             k_nas_enc[32];
  uint8_t                             k_nas_int[32];
  uint8_t                             k_enb[32];
} ims_sec_ctx_t;

class ims_context
{
public:
  ims_ctx_t ims_ctx  = {};
  ims_sec_ctx_t  m_sec_ctx = {};

  ims_context(){}
  ~ims_context(){}

};

class pcs_ims
{
public:
  static pcs_ims* get_instance();
  static void    cleanup();
  void           init(srsenb::rrc_interface_cnw* rrc_, adp *adp);


  uint32_t generateUniqueID(){
    time_t current_time;
    time(&current_time);
    
    srand((unsigned int)current_time);
    uint32_t uniqueID = ((uint32_t)current_time) ^ rand();

    return uniqueID;
  }
  

  srslog::basic_logger& pcs_ims_logger = srslog::fetch_basic_logger("IMS");

  /* message handle functions. */
  bool pcs_handle_ulinformation(uint16_t rnti,  srsran::unique_byte_buffer_t pdu );
  bool pcs_handle_ate_info(srsran::unique_byte_buffer_t pdu);

  //handle UE->PCS msg
  bool handle_Voice_Register_Req(uint16_t rnti, voice_register_req_t& msg);  
  /*********************2024-3-11******************/
  bool handle_Voice_DeRegister_Req(uint16_t rnti, voice_deregister_req_t& msg); 
  bool handle_ue_pcs_call_setup(uint16_t rnti, ue_pcs_call_setup_t& msg); 
  bool handle_call_connect_ack(uint16_t rnti, call_connect_ack_t& msg); 
  bool handle_call_disconnect(uint16_t rnti, call_disconnect_t& msg);
  bool handle_call_release_resp(uint16_t rnti, call_release_resp_t& msg);  

  /*********************???call_id??????******************/ 
  bool handle_call_setup_ack(uint16_t rnti, call_setup_ack_t& msg); 
  bool handle_call_comfirmed(uint16_t rnti, call_confirmed_t& msg);
  bool handle_ue_pcs_call_alerting(uint16_t rnti, ue_pcs_call_alerting_t& msg);
  bool handle_ue_pcs_call_connect(uint16_t rnti, ue_pcs_call_connect_t& msg);
  //???
  bool handle_mo_sms_req(uint16_t rnti, mo_sms_req_t& msg); 
  bool handle_mt_sms_resp(uint16_t rnti, mt_sms_resp_t& msg); 
  /************************end********************/

  //pack PCS->UE msg
  bool pack_Authentication_Command(srsran::unique_byte_buffer_t& ims_buffer, ims_context* ims_ctx);
  bool pack_Voice_Register_Rsp(srsran::unique_byte_buffer_t& ims_buffer, ims_context* ims_ctx);
  /*********************2024-3-11******************/  
  bool pack_Voice_DeRegister_Rsp(srsran::unique_byte_buffer_t& ims_buffer, ims_context* ims_ctx);
  bool pack_call_setup_ack(srsran::unique_byte_buffer_t& ims_buffer, ims_context* ims_ctx);  
  bool pack_pcs_ue_call_alerting(srsran::unique_byte_buffer_t& ims_buffer, ims_context* ims_ctx);   
  bool pack_pcs_ue_call_connect(srsran::unique_byte_buffer_t& ims_buffer, ims_context* ims_ctx);   
  bool pack_call_release_req(srsran::unique_byte_buffer_t& ims_buffer, ims_context* ims_ctx);

  /*********************???call_id??????******************/ 
  bool pack_pcs_ue_call_setup(srsran::unique_byte_buffer_t& ims_buffer, ims_context* ims_ctx);
  bool pack_call_connect_ack(srsran::unique_byte_buffer_t& ims_buffer, ims_context* ims_ctx);
  bool pack_call_comfirmed(srsran::unique_byte_buffer_t& ims_buffer, ims_context* ims_ctx);
  bool pack_mo_sms_resp(srsran::unique_byte_buffer_t& ims_buffer, ims_context* ims_ctx); 
  bool pack_mt_sms_req(srsran::unique_byte_buffer_t& ims_buffer, ims_context* ims_ctx, bool flag); 
  /************************end********************/

  //ims database
  bool add_ims_ctx_to_rnti_map(ims_context* ims_ctx);
  ims_context* find_ims_ctx_from_rnti(uint16_t rnti);
  bool delete_ue_ims_ctx(uint16_t rnti);
  bool send_ims_dl_msg(uint16_t rnti, srsran::unique_byte_buffer_t pdu);
  bool handle_message_content(vector<uint8_t>& message_content, ims_context* ims_ctx);
  bool pack_ate_mt_sms(vector<uint8_t>& message_content, ims_context* ims_ctx);
  bool pack_long_sms_header(vector<uint8_t>& long_sms_header, ims_context* ims_ctx);
  bool send_mt_sms_result_to_ate(bool result);

    uint8_t gen_rand()
	{
		return rand() % 256;
	}

  
//12.9
  //-----------------2023/11/14
  srsenb::rrc_interface_cnw* rrc_pcs_ims;
private:
  pcs_ims();
  virtual ~pcs_ims();

  /* rnti - IMS_cxt */
  std::map<uint16_t, ims_context*> imsi_to_ims_ctx; 

  std::map<uint16_t, ims_context*> rnti_to_ims_ctx; 

  uint16_t       pcs_rnti;
  cnw*           m_cnw;
  static pcs_ims* m_instance;
  adp*          ims_adp;

};

} // namespace srsepc
#endif // SRSEPC_PCS_IMS_H