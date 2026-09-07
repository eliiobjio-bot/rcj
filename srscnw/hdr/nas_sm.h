/******************************************************************************
 * File:        nas_sm.h
 * Description: Top-level NAS SM class. Creates and links all
 *              interfaces and helpers.
 *****************************************************************************/

#ifndef SRSEPC_NAS_SM_H
#define SRSEPC_NAS_SM_H

#include "srsran/common/buffer_pool.h"
#include "srsran/common/standard_streams.h"
#include "srsran/common/threads.h"
#include <cstddef>

#include "nas_context.h"
#include "srsran/asn1/nas_5g_ies.h"
#include "srsran/asn1/nas_5g_msg.h"

#include "srsran/interfaces/cnw_interface_enb.h"
#include"srscnw/hdr/adp.h"


using namespace srsran;
using namespace nas_5g;

namespace srsepc {

class cnw;
class nas_mm;
class adp;
class nas_sm 
{
public:
  static nas_sm* get_instance();
  static void    cleanup();
  void           init(adp* sm_adp_,const cnw_args_t sm_args_);

  srslog::basic_logger& m_nas_sm_logger = srslog::fetch_basic_logger("nas_sm");

  bool handle_pdu_session(srsran::unique_byte_buffer_t nas_buffer, nas_context* nas_ctx,uint16_t enb_ue_id);

  /* message handle functions. */
  bool handle_pdu_session_establishment_request(pdu_session_establishment_request_t& msg, nas_context* nas_ctx,bool is_ims);

  bool handle_pdu_authentication_complete(pdu_session_authentication_complete_t& msg,nas_context* nas_ctx,bool is_accept,bool is_ims,uint16_t enb_ue_id);

  /* message sender functions. */
  bool pack_pdu_session_establishment_accept(srsran::unique_byte_buffer_t& nas_buffer,nas_context* nas_ctx,bool is_ims);
  bool pack_pdu_session_authentication_command(srsran::unique_byte_buffer_t& nas_buffer,nas_context* nas_ctx,bool is_ims);
  bool pack_pdu_session_authentication_result(srsran::unique_byte_buffer_t& nas_buffer, nas_context* nas_ctx);

  /*sm interface*/
  void TTCN_request_header(srsran::unique_byte_buffer_t* TTCN_header);
  void TTCN_complete_header(srsran::unique_byte_buffer_t* TTCN_header);
  bool TTCN_handle_pdu_session_establishment_request(pdu_session_establishment_request_t& msg, nas_context* nas_ctx,bool is_ims);
  bool TTCN2_handle_pdu_session_establishment_request(pdu_session_establishment_request_t& msg, nas_context* nas_ctx,bool is_ims);
  bool TTCN_handle_pdu_authentication_complete(pdu_session_authentication_complete_t& msg,nas_context* nas_ctx,bool is_accept,bool is_ims);

  bool pack_for_sdap_ttcn_accept(srsran::unique_byte_buffer_t& nas_buffer,nas_context* nas_ctx,bool is_ims);
  bool handle_for_sdap_ttcn_complete(pdu_session_authentication_complete_t& msg,nas_context* nas_ctx,bool is_accept,bool is_ims,uint16_t enb_ue_id);

  int test_num=0;
  std::chrono::_V2::system_clock::time_point start_time;
  std::chrono::_V2::system_clock::time_point s2_time;
  std::chrono::_V2::system_clock::time_point s3_time;
  std::chrono::_V2::system_clock::time_point s4_time;
  std::chrono::_V2::system_clock::time_point s5_time;
  int sm_ttcn=0;

  bool pack_pdu_session_modification_command(srsran::unique_byte_buffer_t& nas_buffer, nas_context* nas_ctx,bool is_ims);
  bool pack_pdu_session_modification_command_close_phone(srsran::unique_byte_buffer_t& nas_buffer, nas_context* nas_ctx,bool is_ims);
  bool handle_pdu_modification_complete(pdu_session_modification_complete_t& msg,nas_context* nas_ctx);

  bool    TTCN_handle_pdu_release_request(srsran::unique_byte_buffer_t& nas_buffer, nas_context* nas_ctx);
  bool    TTCN_handle_pdu_release_complete(srsran::unique_byte_buffer_t& nas_buffer, nas_context* nas_ctx);
  bool    TTCN_pack_pdu_session_modification_command(srsran::unique_byte_buffer_t &nas_buffer, nas_context *nas_ctx, bool is_ims);
  bool     handle_pdu_session_release_command_from_ttcn();
  bool TC_92_3_seconds=false;
  void TC_92_after_3_seconds();

  nas_sm();
  virtual ~nas_sm();
  adp *sm_adp;
  nas_mm*           m_mm;
  cnw*              sm_cnw;
  static nas_sm* m_instance;


};

} // namespace srsepc
#endif // SRSEPC_NAS_SM_H