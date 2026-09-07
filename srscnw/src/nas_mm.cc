#include "srscnw/hdr/nas_mm.h"
#include "srscnw/hdr/cnw.h"
#include "srscnw/hdr/nas_context.h"
#include "srsran/asn1/nas_5g_msg.h"
#include "srsran/asn1/s1ap.h"
#include <inttypes.h>
#include <stdint.h>
#include <vector>

using namespace srsran;
using namespace srsran::nas_5g;
namespace srsepc
{

  nas_mm *nas_mm::m_instance = NULL;
  pthread_mutex_t nas_mm_instance_mutex = PTHREAD_MUTEX_INITIALIZER;
  srsepc::nas_context nas_context_info;

  nas_mm::nas_mm(){};

  nas_mm::~nas_mm()
  {
    return;
  }

  nas_mm *nas_mm::get_instance(void)
  {
    pthread_mutex_lock(&nas_mm_instance_mutex);
    if (NULL == m_instance)
    {
      m_instance = new nas_mm();
    }
    pthread_mutex_unlock(&nas_mm_instance_mutex);
    return (m_instance);
  }

  void nas_mm::cleanup(void)
  {
    pthread_mutex_lock(&nas_mm_instance_mutex);
    if (NULL != m_instance)
    {
      delete m_instance;
      m_instance = NULL;
    }
    pthread_mutex_unlock(&nas_mm_instance_mutex);
  }

  void nas_mm::init(srsenb::rrc_interface_cnw *rrc_, adp *mm_adp_, const cnw_args_t mm_args_)
  {
    rrc_mm = rrc_;
    mm_adp = mm_adp_;
    nasmm_ttcn_test_enble = mm_args_.ttcn_test_enble;
    ttcn_nasmm_enble = mm_args_.ttcn_nasmm_enble;
    std::cout << "nasmm_ttcn_test_enble:" << nasmm_ttcn_test_enble << std::endl;
    std::cout << "ttcn_nasmm_enble:" << ttcn_nasmm_enble << std::endl;
    m_cnw = cnw::get_instance();
    m_nas_sm = nas_sm::get_instance();
  }

  bool nas_mm::handle_registration_request(registration_request_t &msg, uint16_t enb_id, uint16_t cnw_ue_id)
  {
    srsran::console("Received NAS message -- Registration Request\n");
    m_nas_mm_logger.info("Received NAS message -- Registration Request");
    registration_type_5gs_t::registration_type_type reg_type = msg.registration_type_5gs.registration_type;
    srsran::console("Registration request -- registration type is: %s\n", reg_type.to_string());
    m_nas_mm_logger.info("Registration request -- registration type is: %s\n", reg_type.to_string());
    registration_type_5gs_t::follow_on_request_bit_type follow_on_flag = msg.registration_type_5gs.follow_on_request_bit;
    srsran::console("Registration request -- follow on flag is: %s\n", follow_on_flag.to_string());
    m_nas_mm_logger.info("Registration request -- follow on flag is: %s\n", follow_on_flag.to_string());
    srsran::console("Registration request -- ngksi is %d\n", msg.ng_ksi.nas_key_set_identifier);
    m_nas_mm_logger.info("Registration request -- ngksi is %d\n", msg.ng_ksi.nas_key_set_identifier);

    if (msg.capability_5gmm_present == true)
    {
      srsran::console("Registration request -- capability_5gmm is \n");
    }

    if (msg.ue_security_capability_present == true)
    {
      srsran::console("Registration request -- ue_security_capability is \n");
    }

    mobile_identity_5gs_t::identity_types mobile_identity = msg.mobile_identity_5gs.type();

    srsran::console("Registration request -- mobile identity type is %s\n", mobile_identity.to_string());
    m_nas_mm_logger.info("Registration request -- mobile identity type is %s\n", mobile_identity.to_string());

    nas_context *nas_ctx = NULL;
    bool errcode = false;

    std::cout << nasmm_ttcn_test_enble << std::endl;
    std::cout << ttcn_nasmm_enble << std::endl;
    std::cout << mm_adp->udp_.is_net_org_dereg_req_need_regreq_repeat << std::endl;
    std::cout << is_send_dereq_req_info_first << std::endl;
    /* 1. Processing by mobile identity type. */

    char supi[16] = "460000123456780";
    char *c_suci = NULL;
    uint64_t supi_n = 0;
    switch (mobile_identity)
    {
    case mobile_identity_5gs_t::identity_types::suci:
      print_ie_suci(msg.mobile_identity_5gs.suci());

      // compute_supi_from_suci(msg.mobile_identity_5gs.suci(), c_suci, supi);

      // printf("Registration request -- Receive SUCI is %s", c_suci);

      srsran::console("Current supi used is %s\n", supi);
      m_nas_mm_logger.info("Current supi used is %s\n", supi);

      uint64_t suci;
      struct_mobile_identity_to_uint64(&suci, &msg.mobile_identity_5gs);

      supi_n = strtoull(supi, nullptr, 10);

      srsran::console("Current supi used is %d\n", supi_n);
      m_cnw->add_supi_to_suci_map(suci, supi_n);

      nas_ctx = m_cnw->find_nas_ctx_from_suci(suci);

      std::cout << " m_adp->udp_.is_reg_req_congestion = " << mm_adp->udp_.is_authentication_reject << std::endl;
      std::cout << " nasmm_ttcn_test_enble = " << nasmm_ttcn_test_enble << std::endl;
      std::cout << " ttcn_nasmm_enble = " << ttcn_nasmm_enble << std::endl;
      if (mm_adp->udp_.is_reg_req_congestion||mm_adp->udp_.is_reg_rej_plmn_not_allowed||mm_adp->udp_.is_authencation_faileure_mac_code || mm_adp->udp_.is_reg_rej_illegal_ue ||
          mm_adp->udp_.is_security_mode_command2 || mm_adp->udp_.is_reg_succecc_no_5gguti||mm_adp->udp_.is_registration_reject_Ue_attempt_five_count || mm_adp->udp_.is_reg_rej_illegal_ue ||
          mm_adp->udp_.is_authencation_reject_error_res || mm_adp->udp_.is_reg_rej_tracking_area_not_allowed || mm_adp->udp_.is_nas_identity_request || mm_adp->udp_.is_authentication_reject)
      {        
        this->handle_nas_reg_req_ttcn(supi_n, enb_id, cnw_ue_id, reg_type, msg);
      }
      else
      {
        if (NULL == nas_ctx)
        {
          errcode = handle_suci_registration_request_unknown_ue(supi_n, enb_id, cnw_ue_id, reg_type, msg);
        }
        else
        {
          errcode = handle_suci_registration_request_known_ue(nas_ctx, enb_id, cnw_ue_id, reg_type, msg);
        }
      }
      break;
    case mobile_identity_5gs_t::identity_types::guti_5g:
      std::cout << "1111111111111111111111111111115556564666661ssssssssssssssssssssssssssssgggggggggggggggggggggggggg" << std::endl;

      srsepc::nas_guti guti;
      struct_mobile_identity_to_guti(&guti, &msg.mobile_identity_5gs);

      if ((mm_adp->udp_.is_net_org_dereg_req_need_regreq_repeat && (is_send_dereq_req_info_first == 0)) || mm_adp->udp_.is_periodic_registration_request || mm_adp->udp_.is_authencation_faileure_repeated_ngksi)
      {
        this->handle_nas_reg_req_ttcn(supi_n, enb_id, cnw_ue_id, reg_type, msg);
      }
      else
      {
        nas_ctx = m_cnw->find_nas_ctx_from_guti(guti);
        if (NULL == nas_ctx)
        {
          errcode = handle_guti_registration_request_unknown_ue(guti, enb_id, cnw_ue_id, reg_type, msg);
        }
        else
        {
          errcode = handle_guti_registration_request_known_ue(nas_ctx, enb_id, cnw_ue_id, reg_type, msg);
        }
      }

      break;

    default:
      m_nas_mm_logger.warning("Registration request -- Unsupported mobile identity type (%s).",
                              mobile_identity.to_string());
      break;
    }
    return errcode;
  }
  /*2023-11-21 wcb add*/
  bool nas_mm::handle_registration_complete(registration_complete_t &msg, uint16_t enb_id, uint16_t cnw_ue_id)
  {
    srsran::console("Received NAS message -- registration complete\n");
    m_nas_mm_logger.info("Received NAS message -- registration complete");

    nas_context *nas_ctx = m_cnw->find_nas_ctx_from_rnti(cnw_ue_id);
    if (nas_ctx == nullptr)
    {
      srsran::console("Received uplink NAS, but could not find UE NAS context.\n");
      m_nas_mm_logger.warning("Received uplink NAS, but could not find UE NAS context. RNTI id: %d", cnw_ue_id);
      return false;
    }

    /* update ue state*/
    nas_ctx->nrmm_ctx.state = NRMM_STATE_REGISTERED;
    nas_ctx->nrcm_ctx.state = NRCM_STATE_CONNECTED;

    // test pack_close_ue_test_loop
    //  if(tc_control==ENABLE_TC_MODE)
    //  {
    //    srsran::unique_byte_buffer_t test_loop = srsran::make_byte_buffer();
    //    pack_close_ue_test_loop(test_loop,nas_ctx,cnw_ue_id);
    //  }

    printf("nas_ctx->m_sec_ctx.ul_nas_count %d\n", nas_ctx->m_sec_ctx.ul_nas_count);

    std::cout << nasmm_ttcn_test_enble << std::endl;
    std::cout << ttcn_nasmm_enble << std::endl;
    std::cout << mm_adp->udp_.is_net_org_dereg_req_need_regreq_repeat << std::endl;
    std::cout << is_send_dereq_req_info_first << std::endl;
    if (mm_adp->udp_.is_net_org_dereg_req_need_regreq_repeat && is_send_dereq_req_info_first)
    {
      srsran::unique_byte_buffer_t registration_complete_message=srsran::make_byte_buffer();
      registration_complete_message->msg[0]=0x01;
      registration_complete_message->msg[1]=0x02;
      registration_complete_message->msg[2]=0x00;
      registration_complete_message->msg[3]=0x00;
      registration_complete_message->msg[4]=0x00;
      registration_complete_message->msg[5]=0x00;
      registration_complete_message->msg[6]=0x02;
      registration_complete_message->msg[7]=0x42;
      registration_complete_message->msg[8]=0x00;
      registration_complete_message->N_bytes=9;
      mm_adp->udp_.send_ttcn_msg_enb(std::move(registration_complete_message));
      srsran::unique_byte_buffer_t udp=srsran::make_byte_buffer();
      udp->init();
      while(true)
      {
        if(mm_adp->udp_.nas_mm_receive_info.size()!=0)
        {
          mm_adp->udp_.nas_mm_receive_info.try_pop(udp);
          break;
        }
      }

      srsran::unique_byte_buffer_t nas_tx = srsran::make_byte_buffer();
      deregistration_request_ue_terminated_t msg;

      std::cout << "sssssssssssssssssssssssssssssssss" << std::endl;
      std::cout << cnw_ue_id << std::endl;
      m_nas_mm_logger.info("Packing Deregistration Request");

      nas_5gs_msg nas_msg;
      deregistration_request_ue_terminated_t &Dereg_req_ue_ter = nas_msg.set_deregistration_request_ue_terminated();

      Dereg_req_ue_ter.de_registration_type.re_registration_required = de_registration_type_t::re_registration_required_type_::options::re_registration_required;

      sec_ctx_t *sec_ctx = &nas_ctx->m_sec_ctx;

      nas_msg.hdr.sequence_number = sec_ctx->dl_nas_count;
      nas_msg.hdr.spare_1_2_version = nas_ctx->m_sec_ctx.spare_1_2_version;
      nas_msg.hdr.security_header_type = nas_5gs_hdr::integrity_protected_and_ciphered;
      SRSASN_CODE err = nas_msg.pack(nas_tx);
      if (err != SRSASN_SUCCESS)
      {
        m_nas_mm_logger.error("Error packing Deregistration Request");
        srsran::console("Error packing Deregistration Request\n");
        return false;
      }
      printf("nas_tx->msg: ");
      for (uint32_t i = 0; i < nas_tx->N_bytes; ++i)
      {
        printf("  0x%x", nas_tx->msg[i]);
      }
      printf("\n");

      // Encrypt NAS message
      cipher_encrypt(nas_tx, nas_ctx);

      // Integrity protect NAS message
      if (nas_ctx->m_sec_ctx.spare_1_2_version == 0)
      {
        integrity_generate(&(sec_ctx->k_nas_int)[16], // key_128  count  direction  msg  msg_len  mac nas_ctx
                           sec_ctx->dl_nas_count,
                           SECURITY_DIRECTION_DOWNLINK,
                           &nas_tx->msg[SEQ_5G_OFFSET],
                           nas_tx->N_bytes - SEQ_5G_OFFSET,
                           &nas_tx->msg[MAC_5G_OFFSET],
                           nas_ctx);
      }
      else
      {
        integrity_generate(&(sec_ctx->k_nas_int)[16], // key_128  count  direction  msg  msg_len  mac nas_ctx
                           sec_ctx->dl_nas_count,
                           SECURITY_DIRECTION_DOWNLINK,
                           &nas_tx->msg[SEQ_5G_OFFSET + 4],
                           nas_tx->N_bytes - SEQ_5G_OFFSET - 4,
                           &nas_tx->msg[MAC_5G_OFFSET + 4],
                           nas_ctx);
      }
      printf("nas_tx->msg: ");
      for (uint32_t i = 0; i < nas_tx->N_bytes; ++i)
      {
        printf("  0x%x", nas_tx->msg[i]);
      }
      printf("\n");
      send_mm_dl_msg(cnw_ue_id, std::move(nas_tx));

      // 不需要走第二次
      is_send_dereq_req_info_first = false;
    }

    if (mm_adp->udp_.is_first_registration_complete ||mm_adp->udp_.is_authencation_faileure_mac_code || mm_adp->udp_.is_registration_reject_Ue_attempt_five_count)
    {
      std::cout << " ttcn_testId = " << mm_adp->udp_.Gen_.test_id << std::endl;
      srsran::unique_byte_buffer_t is_first_registration_complete_pdu = srsran::make_byte_buffer();
      ;
      is_first_registration_complete_pdu->N_bytes = 5;
      is_first_registration_complete_pdu->msg[0] = 0x00;
      is_first_registration_complete_pdu->msg[1] = mm_adp->udp_.Gen_.test_id >> 8;
      is_first_registration_complete_pdu->msg[2] = mm_adp->udp_.Gen_.test_id;
      uint16_t test_count = (is_first_registration_complete_pdu->msg[1] << 8) | (is_first_registration_complete_pdu->msg[2]);
      std::cout << " test_count id = " << test_count << std::endl;
      is_first_registration_complete_pdu->msg[3] = 0x42;
      is_first_registration_complete_pdu->msg[4] = 0x00;

      // mm_adp->udp_.send_ttcn_info.try_push(std::move(is_first_registration_complete_pdu));
      mm_adp->udp_.send_ttcn_msg_enb(std::move(is_first_registration_complete_pdu));
      mm_adp->udp_.is_first_registration_complete = false;
    }
    
    mm_adp->udp_.registration_complete_number += 1;
    printf("mm_adp->udp_.registration_complete_number = %x\n", mm_adp->udp_.registration_complete_number);

    if (mm_adp->udp_.registration_complete_number == 2 && mm_adp->udp_.is_second_registration_complete == true)
    {
      std::cout << " ttcn_testId = " << mm_adp->udp_.Gen_.test_id << std::endl;
      srsran::unique_byte_buffer_t is_second_registration_complete_pdu = srsran::make_byte_buffer();
      is_second_registration_complete_pdu->N_bytes = 5;
      is_second_registration_complete_pdu->msg[0] = 0x00;
      is_second_registration_complete_pdu->msg[1] = mm_adp->udp_.Gen_.test_id >> 8;
      is_second_registration_complete_pdu->msg[2] = mm_adp->udp_.Gen_.test_id;
      uint16_t test_count = (is_second_registration_complete_pdu->msg[1] << 8) | (is_second_registration_complete_pdu->msg[2]);
      std::cout << " test_count id = " << test_count << std::endl;
      is_second_registration_complete_pdu->msg[3] = 0x42;
      is_second_registration_complete_pdu->msg[4] = 0x00;

      // mm_adp->udp_.send_ttcn_info.try_push(std::move(is_second_registration_complete_pdu));
      mm_adp->udp_.send_ttcn_msg_enb(std::move(is_second_registration_complete_pdu));
      mm_adp->udp_.is_first_registration_complete = false;
    }

    // /* ttcn 8_24 */
    if((mm_adp->udp_.is_periodic_registration_request && mm_adp->udp_.registration_complete_number == 2) || mm_adp->udp_.is_nas_identity_request) 
    {
      /* send registration_complete to ttcn*/
      std::cout << " ttcn_testId = " << mm_adp->udp_.Gen_.test_id << std::endl;
      srsran::unique_byte_buffer_t is_second_registration_complete_pdu = srsran::make_byte_buffer();
      is_second_registration_complete_pdu->N_bytes = 5;
      is_second_registration_complete_pdu->msg[0] = 0x00;
      is_second_registration_complete_pdu->msg[1] = mm_adp->udp_.Gen_.test_id >> 8;
      is_second_registration_complete_pdu->msg[2] = mm_adp->udp_.Gen_.test_id;
      uint16_t test_count = (is_second_registration_complete_pdu->msg[1] << 8) | (is_second_registration_complete_pdu->msg[2]);
      std::cout << " test_count id = " << test_count << std::endl;
      is_second_registration_complete_pdu->msg[3] = 0x42;
      is_second_registration_complete_pdu->msg[4] = 0x00;

      mm_adp->udp_.send_ttcn_msg_enb(std::move(is_second_registration_complete_pdu));

    }

    return true;
  }

  bool nas_mm::handle_authentication_response(authentication_response_t &msg, uint16_t enb_id, uint16_t cnw_ue_id)
  {
    srsran::console("Received NAS message -- Authentication response\n");
    m_nas_mm_logger.info("Received NAS message -- Authentication response");

    bool ue_valid = true;

    nas_context *nas_ctx = m_cnw->find_nas_ctx_from_rnti(cnw_ue_id);
    if (nas_ctx == nullptr)
    {
      srsran::console("Received uplink NAS, but could not find UE NAS context.\n");
      m_nas_mm_logger.warning("Received uplink NAS, but could not find UE NAS context. RNTI id: %d", cnw_ue_id);
      return false;
    }

    nrmm_ctx_t *nrmm_ctx = &nas_ctx->nrmm_ctx;
    nrcm_ctx_t *nrcm_ctx = &nas_ctx->nrcm_ctx;
    sec_ctx_t *sec_ctx = &nas_ctx->m_sec_ctx;

    uint8_t res[16];
    /*将解析收到到的鉴权向量保存到res*/
    for (int ia = 0; ia < 16; ia++)
    {
      res[ia] = msg.authentication_response_parameter.res[ia];
    }
    // Log received authentication response
    srsran::console("Authentication Response -- SUCI %015" PRIu64 "\n", nrmm_ctx->suci);
    m_nas_mm_logger.info("Authentication Response -- SUCI %015" PRIu64 "", nrmm_ctx->suci);
    m_nas_mm_logger.info(res, 8, "Authentication response -- RES");
    m_nas_mm_logger.info(sec_ctx->xres, 8, "Authentication response -- XRES");

    // Check UE authentication
    printf("Check UE authentication\n");

    for (int i = 0; i < 16; i++)
    {
      printf("ue——res:  0x%x    amf——xres:  0x%x \n", res[i], sec_ctx->xres[i]);
      if (res[i] != sec_ctx->xres[i])
      {
        ue_valid = false;
      }
    }

    //-------------------------------------------srsran sent nas_auhencation_response to ttcn------

    if (mm_adp->udp_.is_security_mode_command2 || mm_adp->udp_.is_security_mode_command_ue_cap_errror || mm_adp->udp_.is_reg_succecc_no_5gguti || 
        mm_adp->udp_.is_authencation_reject_error_res || mm_adp->udp_.is_authentication_reject)
    {
      this->nas_authencation_reponse_message_to_ttcn(msg);
    }


    srsran::unique_byte_buffer_t nas_tx = srsran::make_byte_buffer();
    if (nas_tx == nullptr)
    {
      m_nas_mm_logger.error("Couldn't allocate PDU in %s().", __FUNCTION__);
      return false;
    }

    /* ttcn 8_18 */
    if(mm_adp->udp_.is_authencation_reject_error_res || mm_adp->udp_.is_authentication_reject) {
      pack_authentication_reject(nas_tx, nas_ctx);
      /*TODO: send message to RRC message queue. */
      send_mm_dl_msg(cnw_ue_id, std::move(nas_tx));
      sec_ctx->dl_nas_count++;
      auto start_time = std::chrono::high_resolution_clock::now();
      while (true)
      {
        auto now_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now_time - start_time);
        if (duration.count() > 1.0)
        {
          break;
        }
      }
      send_start_rem_user(cnw_ue_id);
      return true;
    }
    else if (!ue_valid)
    {
      // Authentication rejected
      srsran::console("UE Authentication Rejected.\n");
      m_nas_mm_logger.warning("UE Authentication Rejected.");

      // Send back Athentication Reject
      pack_authentication_reject(nas_tx, nas_ctx);
      m_nas_mm_logger.info("Downlink NAS: Sending Authentication Reject.");
    }
    else
    {
      // Authentication accepted
      srsran::console("UE Authentication Accepted.\n");
      m_nas_mm_logger.info("UE Authentication Accepted.");

      // Send Security Mode Command
      sec_ctx->ul_nas_count = 0; // Reset the NAS uplink counter for the right key k_enb derivation
      pack_security_mode_command(nas_tx, nas_ctx);
      srsran::console("Downlink NAS: Sending NAS Security Mode Command.\n");
    }

    /*TODO: send message to RRC message queue. */
    send_mm_dl_msg(cnw_ue_id, std::move(nas_tx));
    sec_ctx->dl_nas_count++;

    return true;
  }

  bool nas_mm::handle_authentication_failure(authentication_failure_t &msg, uint16_t enb_id, uint16_t cnw_ue_id)
  {
    m_nas_mm_logger.info("Received NAS message -- Authentication Failure.\n");
    srsran::console("Received NAS message -- Authentication Failure.\n");

    m_nas_mm_logger.info("Authentication Failure Cause is %s\n", msg.cause_5gmm.cause_5gmm.to_string());
    srsran::console("Authentication Failure Cause is %s\n", msg.cause_5gmm.cause_5gmm.to_string());
    
    /* Set the security invaild. */
    bool err = false;

    nas_context *nas_ctx = m_cnw->find_nas_ctx_from_rnti(cnw_ue_id);
    if (nas_ctx == nullptr)
    {
      m_nas_mm_logger.warning("Received uplink NAS, but could not find UE NAS context. RNTI id: %d", cnw_ue_id);
      return false;
    } 

    if(mm_adp->udp_.is_authencation_faileure_repeated_ngksi) 
    {
      srsran::unique_byte_buffer_t authen_failure_message = srsran::make_byte_buffer();
      // send Nas_req_re message
      authen_failure_message->msg[0] = 0x01;
      authen_failure_message->msg[1] = 0x02;
      authen_failure_message->msg[2] = 0x00;
      authen_failure_message->msg[3] = 0x00;
      authen_failure_message->msg[4] = 0x00;
      authen_failure_message->msg[5] = 0x00;
      authen_failure_message->msg[6] = 0x02; // data总长度
      authen_failure_message->msg[7] = 0x3f; // autn failure msg
      authen_failure_message->msg[8] = 0x47; 
      authen_failure_message->N_bytes=9;
      mm_adp->udp_.send_ttcn_msg_enb(std::move(authen_failure_message));

      // /* wait ttcn send autn req*/
      // srsran::unique_byte_buffer_t udp = srsran::make_byte_buffer();
      // udp->init();
      // while(true)
      // {
      //   if(mm_adp->udp_.nas_mm_receive_info.size()!=0)
      //   {
      //     mm_adp->udp_.nas_mm_receive_info.try_pop(udp);
      //     break;
      //   }
      // }

      // nas_ctx->m_sec_ctx.ngksi = 2;
      // if (!gen_auth_info_answer(nas_ctx,
      //                         nas_ctx->nrmm_ctx.supi,
      //                         nas_ctx->m_sec_ctx.k_ausf,
      //                         nas_ctx->m_sec_ctx.k_seaf,
      //                         nas_ctx->m_sec_ctx.k_amf,
      //                         nas_ctx->m_sec_ctx.autn,
      //                         nas_ctx->m_sec_ctx.rand,
      //                         nas_ctx->m_sec_ctx.xres))
      // {
      //   srsran::console("User not found. SUPI %015" PRIu64 "\n", nas_ctx->nrmm_ctx.supi);
      //   m_nas_mm_logger.info("User not found. SUPI %015" PRIu64 "", nas_ctx->nrmm_ctx.supi);
      //   return false;
      // }

      // /* 2.3 construct aka request message. */
      // srsran::unique_byte_buffer_t nas_tx = srsran::make_byte_buffer();
      // if (nas_tx == nullptr)
      // {
      //   m_nas_mm_logger.error("Couldn't allocate PDU in %s().", __FUNCTION__);
      //   return false;
      // }

      // this->pack_authentication_request(nas_tx, nas_ctx);
      // /* 2.4 send message to RRC message queue. */
      // send_mm_dl_msg(cnw_ue_id, std::move(nas_tx));

    }

    if(mm_adp->udp_.is_authencation_faileure_mac_code)
    {     
      srsran::unique_byte_buffer_t authen_failure_message = srsran::make_byte_buffer();
      // send Nas_req_re message
      authen_failure_message->msg[0] = 0x01;
      authen_failure_message->msg[1] = 0x02;
      authen_failure_message->msg[2] = 0x00;
      authen_failure_message->msg[3] = 0x00;
      authen_failure_message->msg[4] = 0x00;
      authen_failure_message->msg[5] = 0x00;
      authen_failure_message->msg[6] = 0x01; // data总长度
      authen_failure_message->msg[7] = 0x3f; // 注册请求消息
      authen_failure_message->N_bytes=8;
      if(msg.cause_5gmm.cause_5gmm==cause_5gmm_t::cause_5gmm_type_::mac_failure)
      {
        authen_failure_message->msg[8]=0x14;
        authen_failure_message->N_bytes=9;  
      } 
      // mm_adp->udp_.send_ttcn_info.try_push(std::move(Nas_reg_req));
      mm_adp->udp_.send_ttcn_msg_enb(std::move(authen_failure_message));

      if (!gen_auth_info_answer(nas_ctx,
                              nas_ctx->nrmm_ctx.supi,
                              nas_ctx->m_sec_ctx.k_ausf,
                              nas_ctx->m_sec_ctx.k_seaf,
                              nas_ctx->m_sec_ctx.k_amf,
                              nas_ctx->m_sec_ctx.autn,
                              nas_ctx->m_sec_ctx.rand,
                              nas_ctx->m_sec_ctx.xres))
      {
        srsran::console("User not found. SUPI %015" PRIu64 "\n", nas_ctx->nrmm_ctx.supi);
        m_nas_mm_logger.info("User not found. SUPI %015" PRIu64 "", nas_ctx->nrmm_ctx.supi);
        return false;
      }

      /* 2.3 construct aka request message. */
      srsran::unique_byte_buffer_t nas_tx = srsran::make_byte_buffer();
      if (nas_tx == nullptr)
      {
        m_nas_mm_logger.error("Couldn't allocate PDU in %s().", __FUNCTION__);
        return false;
      }

      this->pack_authentication_request(nas_tx, nas_ctx);
      /* 2.4 send message to RRC message queue. */
      send_mm_dl_msg(cnw_ue_id, std::move(nas_tx));
    }
    else
    {
      nas_ctx->nrmm_ctx.isSecCplt = false;
      nas_ctx->m_sec_ctx = sec_ctx_t();

    }
  
    return true;
  }

  bool nas_mm::handle_security_mode_complete(security_mode_complete_t &msg, uint16_t enb_id, uint16_t enb_ue_id)
  {
    m_nas_mm_logger.info("Received NAS message -- Security Mode Command Complete.\n");
    srsran::console("Received NAS message -- Security Mode Command Complete.\n");

    bool err = true;

    nas_context *nas_ctx = m_cnw->find_nas_ctx_from_rnti(enb_ue_id);
    if (nas_ctx == nullptr)
    {
      m_nas_mm_logger.warning("Received uplink NAS, but could not find UE NAS context. RNTI id: %d", enb_ue_id);
      return false;
    }

    nrmm_ctx_t nrmm_ctx = nas_ctx->nrmm_ctx;
    std::cout << "sssssssssssssss==" << msg.nas_message_container_present << std::endl;
    if (msg.nas_message_container_present == true)
    {
      // TODO: 2023/11/1 17:23
      err = handle_nas_message_container(
          nas_ctx, enb_id, enb_ue_id, msg_types::options::security_mode_complete, &(msg.nas_message_container));
    }

    if (tc_control == ENABLE_TC_MODE || mm_adp->udp_.ttcn_close_TC == true)
    {
      srsran::unique_byte_buffer_t test_loop = srsran::make_byte_buffer();
      pack_activate_test_mode(test_loop, nas_ctx, enb_ue_id);
      sleep(1);
    }

    srsran::unique_byte_buffer_t nas_tx = srsran::make_byte_buffer();
    if (nas_tx == nullptr)
    {
      m_nas_mm_logger.error("Couldn't allocate PDU in %s().", __FUNCTION__);
      return false;
    }

    nrmm_ctx.e_pro_type = PROC_REGISTERED; // 2023-11-19 test

    asn1::s1ap::init_ctxt_setup_req_s ctx_setup_req;

    if (mm_adp->udp_.is_security_mode_command2 || mm_adp->udp_.is_reg_succecc_no_5gguti || mm_adp->udp_.is_periodic_registration_request)
    {
      this->nas_security_mode_complete_message_to_ttcn(msg);
    }
    memcpy(ctx_setup_req.security_key.data(), nas_ctx->m_sec_ctx.k_enb, sizeof(nas_ctx->m_sec_ctx.k_enb));

    if (err)
    {
      /* send reg/serivce accept message. */
      switch (nrmm_ctx.e_pro_type)
      {
      case PROC_REGISTERED:

        // pack_registration_accept(nas_tx, nas_ctx);
        nas_ctx->nrmm_ctx.reg_accept_msg_valid = true;
        // nas_ctx->nrmm_ctx.reg_or_service_accept_msg_len = nas_tx->N_bytes;
        // memcpy(nas_ctx->nrmm_ctx.reg_or_service_accept_msg, nas_tx->msg, nas_tx->N_bytes);

        srsran::console("-----------send s_setup_ue_ctxt-----------------.\n");
        send_setup_ue_ctxt(enb_ue_id, nas_ctx->m_sec_ctx.k_enb);

        break;

      case PROC_SERVICE:

        break;

      default:
        break;
      }
    }
    else
    {
      /* send reg/service reject message. */
      switch (nrmm_ctx.e_pro_type)
      {
      case PROC_REGISTERED:
        break;

      case PROC_SERVICE:
        break;
      default:
        break;
      }
    }
    /*-TODO----*/
    // nas_ctx->m_sec_ctx.dl_nas_count++; // 12.9

    return true;
  }

  bool nas_mm::handle_security_mode_reject(security_mode_reject_t &msg, uint16_t enb_id, uint16_t cnw_ue_id)
  {
    srsran::console("UE Security Mode Rejected, Cause is %s\n", msg.cause_5gmm.cause_5gmm.to_string());
    m_nas_mm_logger.warning("UE Security Mode Rejected, Cause is %s\n", msg.cause_5gmm.cause_5gmm.to_string());

    /* ttcn 8_19 ue_cap_error*/
    if(mm_adp->udp_.is_security_mode_command_ue_cap_errror) {
      /*send security_mode_reject to ttcn*/
      srsran::unique_byte_buffer_t Nas_security_mode_rej = srsran::make_byte_buffer();

      Nas_security_mode_rej->msg[0] = 0x01;
      Nas_security_mode_rej->msg[1] = 0x02;
      Nas_security_mode_rej->msg[2] = 0x00;
      Nas_security_mode_rej->msg[3] = 0x00;
      Nas_security_mode_rej->msg[4] = 0x00;
      Nas_security_mode_rej->msg[5] = 0x00;
      Nas_security_mode_rej->msg[6] = 0x02; // data总长度
      Nas_security_mode_rej->msg[7] = 0x4c; // 
      Nas_security_mode_rej->msg[8] = 0x17; // cause_5gmm

      Nas_security_mode_rej->N_bytes = 9;

      mm_adp->udp_.send_ttcn_msg_enb(std::move(Nas_security_mode_rej));
    }


    return true;
  }

  /*===================================================================================================================================*/
  /* pack downlink message. */

  bool nas_mm::pack_authentication_request(srsran::unique_byte_buffer_t &nas_buffer, nas_context *nas_ctx)
  {
    std::cout << "---------@@pack_authentication_request@@------------" << std::endl;
    m_nas_mm_logger.info("Packing Authentication Request");

    nas_5gs_msg nas_msg;

    nas_context m_nas_context;
    authentication_request_t &auth_req = nas_msg.set_authentication_request();

    /* mandatory IE */
    auth_req.ng_ksi.security_context_flag.value =
        key_set_identifier_t::security_context_flag_type::native_security_context;
    auth_req.ng_ksi.nas_key_set_identifier = m_nas_context.convert_ng_ksi(nas_ctx->m_sec_ctx.ngksi);

    for (uint8_t i = 0; i < ABBA_LEN; i++)
    {
      auth_req.abba.abba_contents.push_back(0);
    }

    /* Optional IE */
    auth_req.authentication_parameter_autn_present = true;
    std::cout<<"is_authencation_faileure_mac_code="<<mm_adp->udp_.is_authencation_faileure_mac_code<<std::endl;
    if(mm_adp->udp_.is_authencation_faileure_mac_code&&is_congestion_first)
    {
        auth_req.authentication_parameter_autn.autn.insert(
        auth_req.authentication_parameter_autn.autn.begin(), nas_ctx->m_sec_ctx.autn, nas_ctx->m_sec_ctx.autn + 8);
        srsran::unique_byte_buffer_t udp = srsran::make_byte_buffer();
        udp->init();
        while(true)
        {
          if(mm_adp->udp_.nas_mm_receive_info.size()!=0)
          {
            mm_adp->udp_.nas_mm_receive_info.try_pop(udp);
            break;
          }
        }

        for(uint32_t i=0;i<8;i++)
        {
           auth_req.authentication_parameter_autn.autn.push_back(0x01);
        }
        is_congestion_first=false;
    }
    else{
        auth_req.authentication_parameter_autn.autn.insert(
        auth_req.authentication_parameter_autn.autn.begin(), nas_ctx->m_sec_ctx.autn, nas_ctx->m_sec_ctx.autn + 16);
    }
  

    auth_req.authentication_parameter_rand_present = true;
    std::copy(std::begin(nas_ctx->m_sec_ctx.rand),
              std::end(nas_ctx->m_sec_ctx.rand),
              std::begin(auth_req.authentication_parameter_rand.rand));

    SRSASN_CODE err = nas_msg.pack(nas_buffer);
    if (err != SRSASN_SUCCESS)
    {
      m_nas_mm_logger.error("Error packing Authentication Request");
      srsran::console("Error packing Authentication Request\n");
      return false;
    }

    //PCAP
    if(m_cnw->get_cnw_args()->pcap_net.enable) {  
      if(nas_msg.hdr.security_header_type == nas_5gs_hdr::security_header_type_opts::plain_5gs_nas_message) {
        m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg, nas_buffer->N_bytes);
      }
      else {
        if(nas_msg.hdr.spare_1_2_version == 0x00) {
          m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg + 7, nas_buffer->N_bytes - 7);
        }
        else {
          m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg + 11, nas_buffer->N_bytes - 11);
        }
      } 
    } 

    return true;
  }

  bool nas_mm::pack_authentication_reject(srsran::unique_byte_buffer_t &nas_buffer, nas_context *nas_ctx)
  {
    m_nas_mm_logger.info("Packing Authentication Reject");

    nas_5gs_msg nas_msg;
    authentication_reject_t &auth_rej = nas_msg.set_authentication_reject();
    nas_msg.hdr.security_header_type = nas_5gs_hdr::security_header_type_opts::plain_5gs_nas_message;
    nas_msg.hdr.spare_1_2_version = nas_ctx->m_sec_ctx.spare_1_2_version;

    SRSASN_CODE err = nas_msg.pack(nas_buffer);
    if (err != SRSASN_SUCCESS)
    {
      m_nas_mm_logger.error("Error packing Authentication Reject");
      srsran::console("Error packing Authentication Reject\n");
      return false;
    }

    //PCAP
    if(m_cnw->get_cnw_args()->pcap_net.enable) {  
      if(nas_msg.hdr.security_header_type == nas_5gs_hdr::security_header_type_opts::plain_5gs_nas_message) {
        m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg, nas_buffer->N_bytes);
      }
      else {
        if(nas_msg.hdr.spare_1_2_version == 0x00) {
          m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg + 7, nas_buffer->N_bytes - 7);
        }
        else {
          m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg + 11, nas_buffer->N_bytes - 11);
        }
      } 
    } 

    return true;
  }

  bool nas_mm::pack_security_mode_command(srsran::unique_byte_buffer_t &nas_buffer, nas_context *nas_ctx)
  {
    m_nas_mm_logger.info("pack_security_mode_command");
    srsran::console("pack_security_mode_command\n");

    nas_5gs_msg nas_msg;
    /*设置security_mode_command hdr*/
    security_mode_command_t &smc_msg = nas_msg.set_security_mode_command();

    // smc_msg.additional_5g_security_information_present=true;
    // smc_msg.additional_5g_security_information.rinmr=true;

    nrmm_ctx_t *nrmm_ctx = &nas_ctx->nrmm_ctx;
    nrcm_ctx_t *nrcm_ctx = &nas_ctx->nrcm_ctx;
    sec_ctx_t *sec_ctx = &nas_ctx->m_sec_ctx;

    // 完整性算法设为1和加密算法为0
    //sec_ctx->integ_algo = INTEGRITY_ALGORITHM_ID_EIA0;

    smc_msg.selected_nas_security_algorithms.ciphering_algorithm = nas_ctx->convert_cipher_algo(sec_ctx->cipher_algo);
    smc_msg.selected_nas_security_algorithms.integrity_protection_algorithm = nas_ctx->convert_integ_algo(sec_ctx->integ_algo);

    smc_msg.ng_ksi.security_context_flag = key_set_identifier_t::security_context_flag_type::native_security_context;
    smc_msg.ng_ksi.nas_key_set_identifier = nas_ctx->convert_ng_ksi(sec_ctx->ngksi);

    // Replay UE security cap

    smc_msg.replayed_ue_security_capabilities.eps_caps_present = true;

    smc_msg.replayed_ue_security_capabilities.ea0_5g_supported = sec_ctx->ue_network_cap.ea0_5g_supported;
    smc_msg.replayed_ue_security_capabilities.ea1_128_5g_supported = sec_ctx->ue_network_cap.ea1_128_5g_supported;
    smc_msg.replayed_ue_security_capabilities.ea2_128_5g_supported = sec_ctx->ue_network_cap.ea2_128_5g_supported;
    smc_msg.replayed_ue_security_capabilities.ea3_128_5g_supported = sec_ctx->ue_network_cap.ea3_128_5g_supported;
    smc_msg.replayed_ue_security_capabilities.ea4_5g_supported = sec_ctx->ue_network_cap.ea4_5g_supported;
    smc_msg.replayed_ue_security_capabilities.ea5_5g_supported = sec_ctx->ue_network_cap.ea5_5g_supported;
    smc_msg.replayed_ue_security_capabilities.ea6_5g_supported = sec_ctx->ue_network_cap.ea6_5g_supported;
    smc_msg.replayed_ue_security_capabilities.ea7_5g_supported = sec_ctx->ue_network_cap.ea7_5g_supported;

    smc_msg.replayed_ue_security_capabilities.ia0_5g_supported = sec_ctx->ue_network_cap.ia0_5g_supported;
    smc_msg.replayed_ue_security_capabilities.ia1_128_5g_supported = sec_ctx->ue_network_cap.ia1_128_5g_supported;
    smc_msg.replayed_ue_security_capabilities.ia2_128_5g_supported = sec_ctx->ue_network_cap.ia2_128_5g_supported;
    smc_msg.replayed_ue_security_capabilities.ia3_128_5g_supported = sec_ctx->ue_network_cap.ia3_128_5g_supported;
    smc_msg.replayed_ue_security_capabilities.ia4_5g_supported = sec_ctx->ue_network_cap.ia4_5g_supported;
    smc_msg.replayed_ue_security_capabilities.ia5_5g_supported = sec_ctx->ue_network_cap.ia5_5g_supported;
    smc_msg.replayed_ue_security_capabilities.ia6_5g_supported = sec_ctx->ue_network_cap.ia6_5g_supported;
    smc_msg.replayed_ue_security_capabilities.ia7_5g_supported = sec_ctx->ue_network_cap.ia7_5g_supported;

    smc_msg.replayed_ue_security_capabilities.eea0_supported = sec_ctx->ue_network_cap.eea0_supported;
    smc_msg.replayed_ue_security_capabilities.eea1_128_supported = sec_ctx->ue_network_cap.eea1_128_supported;
    smc_msg.replayed_ue_security_capabilities.eea2_128_supported = sec_ctx->ue_network_cap.eea2_128_supported;
    smc_msg.replayed_ue_security_capabilities.eea3_128_supported = sec_ctx->ue_network_cap.eea3_128_supported;
    smc_msg.replayed_ue_security_capabilities.eea4_supported = sec_ctx->ue_network_cap.eea4_supported;
    smc_msg.replayed_ue_security_capabilities.nea13_supported = sec_ctx->ue_network_cap.nea13_supported;
    smc_msg.replayed_ue_security_capabilities.nea14_supported = sec_ctx->ue_network_cap.nea14_supported;
    smc_msg.replayed_ue_security_capabilities.eea7_supported = sec_ctx->ue_network_cap.eea7_supported;

    smc_msg.replayed_ue_security_capabilities.eia0_supported = sec_ctx->ue_network_cap.eia0_supported;
    smc_msg.replayed_ue_security_capabilities.eia1_128_supported = sec_ctx->ue_network_cap.eia1_128_supported;
    smc_msg.replayed_ue_security_capabilities.eia2_128_supported = sec_ctx->ue_network_cap.eia2_128_supported;
    smc_msg.replayed_ue_security_capabilities.eia3_128_supported = sec_ctx->ue_network_cap.eia3_128_supported;
    smc_msg.replayed_ue_security_capabilities.eia4_supported = sec_ctx->ue_network_cap.eia4_supported;
    smc_msg.replayed_ue_security_capabilities.nia13_supported = sec_ctx->ue_network_cap.nia13_supported;
    smc_msg.replayed_ue_security_capabilities.nia14_supported = sec_ctx->ue_network_cap.nia14_supported;
    smc_msg.replayed_ue_security_capabilities.eia7_supported = sec_ctx->ue_network_cap.eia7_supported;

    /* ttcn 8_19*/
    if(mm_adp->udp_.is_security_mode_command_ue_cap_errror) {
      smc_msg.replayed_ue_security_capabilities.ea0_5g_supported = !sec_ctx->ue_network_cap.ea0_5g_supported;
    }

    nas_msg.hdr.sequence_number = 0;
    nas_msg.hdr.spare_1_2_version = sec_ctx->spare_1_2_version;
    nas_ctx->m_sec_ctx.spare_1_2_version = nas_msg.hdr.spare_1_2_version;
    nas_msg.hdr.security_header_type = nas_5gs_hdr::integrity_protected_with_new_5G_nas_context;

    srsran::security_generate_k_nas_5g(
        sec_ctx->k_amf, sec_ctx->cipher_algo, sec_ctx->integ_algo, sec_ctx->k_nas_enc, sec_ctx->k_nas_int);

    srsran::console("sec_ctx.k_nas_int :");
    for (int j = 16; j < 32; ++j)
    {
      srsran::console("  %x", sec_ctx->k_nas_int[j]);
    }
    printf("\n");
    srsran::console("sec_ctx.k_nas_enc :");
    for (int j = 16; j < 32; ++j)
    {
      srsran::console("  %x", sec_ctx->k_nas_enc[j]);
    }
    printf("\n"); // 生成秘钥为32位，使用时采用后16位  11.25

    m_nas_mm_logger.info(sec_ctx->k_nas_enc, 32, "Key NAS Encryption (k_nas_enc)");
    m_nas_mm_logger.info(sec_ctx->k_nas_int, 32, "Key NAS Integrity (k_nas_int)");

    uint8_t key_enb[32];
    srsran::security_generate_k_gnb(sec_ctx->k_amf, sec_ctx->ul_nas_count, sec_ctx->k_enb);
    m_nas_mm_logger.info("Generating KeNB with UL NAS COUNT: %d", sec_ctx->ul_nas_count);
    srsran::console("Generating KeNB with UL NAS COUNT: %d\n", sec_ctx->ul_nas_count);
    m_nas_mm_logger.info(sec_ctx->k_enb, 32, "Key eNodeB (k_enb)");

    srsran::console("sec_ctx.k_gnb :");
    for (int j = 0; j < 32; ++j)
    {
      srsran::console("  %x", sec_ctx->k_enb[j]);
    }
    printf("\n");

    /*默认值：7E 00 5D 11 01 02 F0 F0*/
    if (nas_msg.pack(nas_buffer) != SRSASN_SUCCESS)
    {
      m_nas_mm_logger.error("Failed to pack security mode complete");
      return SRSRAN_ERROR;
    } // pack Assemble the message before integrity protection

    //PCAP
    if(m_cnw->get_cnw_args()->pcap_net.enable) {  
      if(nas_msg.hdr.security_header_type == nas_5gs_hdr::security_header_type_opts::plain_5gs_nas_message) {
        m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg, nas_buffer->N_bytes);
      }
      else {
        if(nas_msg.hdr.spare_1_2_version == 0x00) {
          m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg + 7, nas_buffer->N_bytes - 7);
        }
        else {
          m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg + 11, nas_buffer->N_bytes - 11);
        }
      } 
    } 

    srsran::console(" smc mag1:");
    for (int j = 0; j < (int)nas_buffer->N_bytes; ++j)
    {
      srsran::console("  %x", nas_buffer->msg[j]);
    }
    printf("\n");
    // AMF应发送未经加密的安全模式命令消息，但应使用基于KAMF或由消息中包含的ngKSI指示的映射K'AMF的5G NAS完整性密钥对消息进行完整性保护
    //  Generate MAC for integrity protection
    if (nas_ctx->m_sec_ctx.spare_1_2_version == 0)
    {
      integrity_generate(&(sec_ctx->k_nas_int)[16], // key_128  count  direction  msg  msg_len  mac nas_ctx
                         sec_ctx->dl_nas_count,
                         SECURITY_DIRECTION_DOWNLINK,
                         &nas_buffer->msg[SEQ_5G_OFFSET],
                         nas_buffer->N_bytes - SEQ_5G_OFFSET,
                         &nas_buffer->msg[MAC_5G_OFFSET],
                         nas_ctx);
    }
    else
    {
      integrity_generate(&(sec_ctx->k_nas_int)[16], // key_128  count  direction  msg  msg_len  mac nas_ctx
                         sec_ctx->dl_nas_count,
                         SECURITY_DIRECTION_DOWNLINK,
                         &nas_buffer->msg[SEQ_5G_OFFSET + 4],
                         nas_buffer->N_bytes - SEQ_5G_OFFSET - 4,
                         &nas_buffer->msg[MAC_5G_OFFSET + 4],
                         nas_ctx);
    }

    srsran::console(" smc mag2:");
    for (int j = 0; j < (int)nas_buffer->N_bytes; ++j)
    {
      srsran::console("  %x", nas_buffer->msg[j]);
    }
    printf("\n");

    return true;
  }

  bool nas_mm::pack_registration_accept(srsran::unique_byte_buffer_t &nas_buffer, nas_context *nas_ctx)
  {

    m_nas_mm_logger.info("Packing Registration accept");
    srsran::console("---------Packing Registration accept---------\n");
    nas_5gs_msg nas_msg;

    nrmm_ctx_t *nrmm_ctx = &nas_ctx->nrmm_ctx;
    nrcm_ctx_t *nrcm_ctx = &nas_ctx->nrcm_ctx;
    sec_ctx_t *sec_ctx = &nas_ctx->m_sec_ctx;
    uint16_t mcc_num = 0;
    u_int16_t mnc_num = 0;
    // uint8_t     mcc[3];
    // uint8_t     mnc[3];
    uint8_t nof_mnc_digits;
    cnw_args_t *cnw_args = m_cnw->get_cnw_args();

    registration_accept_t &reg_acpt_msg = nas_msg.set_registration_accept();

    // 安全头设置
    nas_msg.hdr.sequence_number = sec_ctx->dl_nas_count;
    nas_msg.hdr.security_header_type = nas_5gs_hdr::integrity_protected_and_ciphered;
    nas_msg.hdr.spare_1_2_version = nas_ctx->m_sec_ctx.spare_1_2_version;

    reg_acpt_msg.registration_result_5gs.registration_result =
        registration_result_5gs_t::registration_result_type::access_3_gpp;
    reg_acpt_msg.registration_result_5gs.sms_allowed =
        registration_result_5gs_t::SMS_allowed_type::sms_over_nas_not_allowed;
    reg_acpt_msg.registration_result_5gs.emergency_registered =
        registration_result_5gs_t::Emergency_registered_type::not_registered_for_emergency_services;
    reg_acpt_msg.registration_result_5gs.nssaa_to_be_performed =
        registration_result_5gs_t::NSSAA_to_be_performed_type::nssaa_is_not_to_be_performed;

    /* Guti Setter */
    reg_acpt_msg.guti_5g_present = true;
    mobile_identity_5gs_t::guti_5g_s &guti_5g = reg_acpt_msg.guti_5g.set_guti_5g();
    mcc_num = 0x460f;
    mnc_num = 0xff00;
    uint8_t mnc_len = 0;

    srsran::mcc_to_bytes(mcc_num, &guti_5g.mcc[0]); /*这个赋值没有执行进去*/
    srsran::mnc_to_bytes(mnc_num, &guti_5g.mnc[0], &mnc_len);

    // 直接采用默认值的赋值
    guti_5g.mcc[0] = 0x04;
    guti_5g.mcc[1] = 0x06;
    guti_5g.mcc[2] = 0x00;

    guti_5g.mnc[0] = 0x00;
    guti_5g.mnc[1] = 0x00;
    guti_5g.mnc[2] = 0x0f;

    guti_5g.amf_pointer = cnw_args->amf_pointer;
    guti_5g.amf_region_id = cnw_args->amf_region_id;
    guti_5g.amf_set_id = cnw_args->amf_set_id;
    // guti_5g.tmsi_5g       = m_cnw->allocate_m_tmsi(nrmm_ctx->suci);

    // guti_5g.amf_pointer = 0x01;
    // guti_5g.amf_region_id = 0xfe;
    // guti_5g.amf_set_id = 0x0001;
    guti_5g.tmsi_5g = 0xc2345678;

    /*add tmsi_nas_ctx*/

    nrcm_ctx->tmsi_5g = guti_5g.tmsi_5g; // 将tmsi_5g保存到网络
    m_cnw->add_nas_ctx_to_tmsi_5g_map(nas_ctx);

    /* Tai list Setter */
    reg_acpt_msg.tai_list_present = true;

    // uint16_t mcc = 0x460f;
    // uint16_t mnc = 0xff00;

    uint16_t mcc = cnw_args->mcc;
    uint16_t mnc = cnw_args->mnc;

    tracking_area_identity_list_5gs_t::non_consecutive_tac_values_s &tai_list_s = reg_acpt_msg.tai_list.set_non_consecutive_tac_values();
    tai_list_s.set(tracking_area_identity_list_5gs_t::number_of_elements_::number_of_elements_1);
    tai_list_s.plmn.mcc[0] = (uint8_t)((mcc & 0xF000) >> 12);
    tai_list_s.plmn.mcc[1] = (uint8_t)((mcc & 0x0F00) >> 8);
    tai_list_s.plmn.mcc[2] = (uint8_t)(mcc & 0x00F0);
    tai_list_s.plmn.mnc[2] = (uint8_t)((mnc & 0xF00) >> 8);
    tai_list_s.plmn.mnc[1] = (uint8_t)((mnc & 0x0F0) >> 4);
    tai_list_s.plmn.mnc[0] = (uint8_t)(mnc & 0x00F0);
    // TODO: wait
    // TODO: wait
    tai_list_s.tac_t[0].tac[0] = 1;
    tai_list_s.tac_t[0].tac[1] = 0;
    tai_list_s.tac_t[0].tac[2] = 0;
    //  reg_acpt_msg.tai_list

    /* T3512 Setter */
    reg_acpt_msg.t3512_value_present = true;
    reg_acpt_msg.t3512_value.unit = gprs_timer_3_t::Unit_type::value_is_incremented_in_multiples_of_1_hour; 
    reg_acpt_msg.t3512_value.timer_value = 0x02;  

    if(mm_adp->udp_.is_periodic_registration_request) {
      reg_acpt_msg.t3512_value.unit = gprs_timer_3_t::Unit_type::value_is_incremented_in_multiples_of_1_minute; 
      reg_acpt_msg.t3512_value.timer_value = 0x03;       
      srsran::unique_byte_buffer_t udp=srsran::make_byte_buffer();
      udp->init();
      while(true) {
          if(mm_adp->udp_.nas_mm_receive_info.size()!=0)
          {
            mm_adp->udp_.nas_mm_receive_info.try_pop(udp);
            /* judge ttcn msg???*/
            break;
          }
      }
    }   

    if(mm_adp->udp_.is_authencation_faileure_repeated_ngksi) {
      reg_acpt_msg.t3512_value.unit = gprs_timer_3_t::Unit_type::value_is_incremented_in_multiples_of_2_seconds; 
      reg_acpt_msg.t3512_value.timer_value = 0x05;       
    }   

    /* Eplmn list Setter */
    SRSASN_CODE err = nas_msg.pack(nas_buffer);
    if (err != SRSASN_SUCCESS)
    {
      m_nas_mm_logger.error("Error packing Registration accept");
      srsran::console("Error packing Registration accept\n");
      return false;
    }

    srsran::console(" registration_accept mag1:");
    for (int j = 0; j < (int)nas_buffer->N_bytes; ++j)
    {
      srsran::console("  %x", nas_buffer->msg[j]);
    }
    printf("\n");
    
    //PCAP
    if(m_cnw->get_cnw_args()->pcap_net.enable) {  
      if(nas_msg.hdr.security_header_type == nas_5gs_hdr::security_header_type_opts::plain_5gs_nas_message) {
        m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg, nas_buffer->N_bytes);
      }
      else {
        if(nas_msg.hdr.spare_1_2_version == 0x00) {
          m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg + 7, nas_buffer->N_bytes - 7);
        }
        else {
          m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg + 11, nas_buffer->N_bytes - 11);
        }
      } 
    } 

    // Encrypt NAS message
    cipher_encrypt(nas_buffer, nas_ctx);

    // Integrity protect NAS message
    if (nas_ctx->m_sec_ctx.spare_1_2_version == 0)
    {
      integrity_generate(&(sec_ctx->k_nas_int)[16], // key_128  count  direction  msg  msg_len  mac nas_ctx
                         sec_ctx->dl_nas_count,
                         SECURITY_DIRECTION_DOWNLINK,
                         &nas_buffer->msg[SEQ_5G_OFFSET],
                         nas_buffer->N_bytes - SEQ_5G_OFFSET,
                         &nas_buffer->msg[MAC_5G_OFFSET],
                         nas_ctx);
    }
    else
    {
      integrity_generate(&(sec_ctx->k_nas_int)[16], // key_128  count  direction  msg  msg_len  mac nas_ctx
                         sec_ctx->dl_nas_count,
                         SECURITY_DIRECTION_DOWNLINK,
                         &nas_buffer->msg[SEQ_5G_OFFSET + 4],
                         nas_buffer->N_bytes - SEQ_5G_OFFSET - 4,
                         &nas_buffer->msg[MAC_5G_OFFSET + 4],
                         nas_ctx);
    }

    srsran::console(" registration_accept mag2:");
    for (int j = 0; j < (int)nas_buffer->N_bytes; ++j)
    {
      srsran::console("  %x", nas_buffer->msg[j]);
    }
    printf("\n");

    return true;
  }

  /**
   *@brief  handle NAS context
   *@date   2024/05/29
   */
  bool nas_mm::delete_ue_nas_ctx(uint64_t suci)
  {
    return true;
  }

  bool nas_mm::pack_deregistration_request_to_ue(srsran::unique_byte_buffer_t &nas_buffer,
                                                 srsran::nas_5g::deregistration_request_ue_terminated_t *msg)
  {
    m_nas_mm_logger.info("Packing Deregistration Request");

    nas_5gs_msg nas_msg;
    deregistration_request_ue_terminated_t &Dereg_req_ue_ter = nas_msg.set_deregistration_request_ue_terminated();

    Dereg_req_ue_ter.de_registration_type.re_registration_required = de_registration_type_t::re_registration_required_type_::options::re_registration_required;

    SRSASN_CODE err = nas_msg.pack(nas_buffer);
    if (err != SRSASN_SUCCESS)
    {
      m_nas_mm_logger.error("Error packing Deregistration Request");
      srsran::console("Error packing Deregistration Request\n");
      return false;
    }

    //PCAP
    if(m_cnw->get_cnw_args()->pcap_net.enable) {  
      if(nas_msg.hdr.security_header_type == nas_5gs_hdr::security_header_type_opts::plain_5gs_nas_message) {
        m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg, nas_buffer->N_bytes);
      }
      else {
        if(nas_msg.hdr.spare_1_2_version == 0x00) {
          m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg + 7, nas_buffer->N_bytes - 7);
        }
        else {
          m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg + 11, nas_buffer->N_bytes - 11);
        }
      } 
    } 

    printf("nas_buffer->msg: ");
    for (uint32_t i = 0; i < nas_buffer->N_bytes; ++i)
    {
      printf("  0x%x", nas_buffer->msg[i]);
    }
    printf("\n");
    return true;
  }

  bool nas_mm::handle_deregistration_request_from_ue(deregistration_request_ue_originating_t &msg)
  {
    nas_context *nas_ctx = nullptr;
    std::cout << "handle deregistration request" << std::endl;
    /*find ue by guti*/
    std::cout << "msg mobile_identity_5gs = " << msg.mobile_identity_5gs.type().to_string() << std::endl;
    switch (msg.mobile_identity_5gs.type())
    {
    case mobile_identity_5gs_t::identity_types::guti_5g:
      // srsepc::nas_guti guti;
      // struct_mobile_identity_to_guti(&guti, &msg.mobile_identity_5gs);
      // nas_ctx = m_cnw->find_nas_ctx_from_guti(guti);
      nas_ctx = m_cnw->find_nas_ctx_from_tmsi_5g(msg.mobile_identity_5gs.guti_5g().tmsi_5g);
      if (nas_ctx)
      {
        m_nas_mm_logger.info("DeRegistration request  known UE by GUTI\n");
        /*save msg info*/
        de_registration_type_t::switch_off_type SwitchOff = msg.de_registration_type.switch_off;
        srsran::console("DeRegistration request -- Switch Off flag is: %s\n", SwitchOff.to_string());
        m_nas_mm_logger.info("DeRegistration request -- Switch Off flag is: %s\n", SwitchOff.to_string());

        /*release PDU*/
        /*TBD*/

        /*switch off*/
        if (de_registration_type_t::switch_off_type::options::switch_off == SwitchOff)
        {
          /*do not send dereg accept*/
          std::cout << " switch_off " << std::endl;
          send_switch_info_to_ate();

          if (mm_adp->udp_.is_dereg_reg_req_ue_switchon)
          {
            this->nas_dereg_req_message_to_ttcn(msg);
            send_start_rem_rel_user(70);
          }
          send_start_rem_rel_user(70);
        }
        else
        {
          /*Normal dereg*/
          /*send dereg accept*/
          std::cout << " Normal dereg " << std::endl;

          srsran::unique_byte_buffer_t nas_tx = srsran::make_byte_buffer();
          if (mm_adp->udp_.is_dereg_normal)
          {
            this->nas_dereg_req_message_to_ttcn(msg);
          }

          sec_ctx_t *sec_ctx = &nas_ctx->m_sec_ctx;

          nas_5gs_msg nas_msg;
          deregistration_accept_ue_terminated_t &Dereg_acc_ue_ter = nas_msg.set_deregistration_accept_ue_terminated();

          nas_msg.hdr.sequence_number = sec_ctx->dl_nas_count;
          nas_msg.hdr.security_header_type = nas_5gs_hdr::integrity_protected_and_ciphered;

          m_nas_mm_logger.info("Packing Deregistration accept");
          SRSASN_CODE err = nas_msg.pack(nas_tx);
          if (err != SRSASN_SUCCESS)
          {
            m_nas_mm_logger.error("Error packing Deregistration accept");
            srsran::console("Error packing Deregistration accept\n");
            return false;
          }

          // Encrypt NAS message
          cipher_encrypt(nas_tx, nas_ctx);

          // Integrity protect NAS message
          if (nas_ctx->m_sec_ctx.spare_1_2_version == 0)
          {
            integrity_generate(&(sec_ctx->k_nas_int)[16], // key_128  count  direction  msg  msg_len  mac nas_ctx
                               sec_ctx->dl_nas_count,
                               SECURITY_DIRECTION_DOWNLINK,
                               &nas_tx->msg[SEQ_5G_OFFSET],
                               nas_tx->N_bytes - SEQ_5G_OFFSET,
                               &nas_tx->msg[MAC_5G_OFFSET],
                               nas_ctx);
          }
          else
          {
            integrity_generate(&(sec_ctx->k_nas_int)[16], // key_128  count  direction  msg  msg_len  mac nas_ctx
                               sec_ctx->dl_nas_count,
                               SECURITY_DIRECTION_DOWNLINK,
                               &nas_tx->msg[SEQ_5G_OFFSET + 4],
                               nas_tx->N_bytes - SEQ_5G_OFFSET - 4,
                               &nas_tx->msg[MAC_5G_OFFSET + 4],
                               nas_ctx);
          }

          send_mm_dl_msg(70, std::move(nas_tx));
          // nas_mm::handle_deregistration_accept_to_ue_ttcn();
          // 调用RRC连接释放函数
          send_start_rem_rel_user(70);
        }
        /*change state to IDLE*/
        nas_ctx->nrcm_ctx.state = NRCM_STATE_IDLE;
        /*clear ctx*/
        // Delete previous NAS context
        m_cnw->delete_ue_nas_ctx(msg.mobile_identity_5gs.guti_5g().tmsi_5g);
        // m_cnw->delete_ue_ctx(nas_ctx->nrmm_ctx.suci);
      }
      else
      {
        m_nas_mm_logger.info("DeRegistration request -- unknown UE by GUTI\n");
      }
      break;

    default:

      break;
    }

    return true;
  }

  bool nas_mm::handle_deregistration_request_to_ue_ttcn(DEREG_MSG_INFO *DeregMsgInfo)
  {
    srsran::nas_5g::deregistration_request_ue_terminated_t *msg = NULL;
    srsran::unique_byte_buffer_t nas_tx;

    if (NR_NAS_MM_DEREG_NORMAL_DEREGISTRATION == DeregMsgInfo->SwitchOff)
    {
      msg->de_registration_type.switch_off = de_registration_type_t::switch_off_type::normal_de_registration;
    }
    else // if(NR_NAS_MM_DEREG_SWITCHOFF == DeregMsgInfo->SwitchOff)
    {
      msg->de_registration_type.switch_off = de_registration_type_t::switch_off_type::switch_off;
    }

    /*set 5GMM cause*/
    /*get value from ttcn*/
    if (DeregMsgInfo->MMCauseFlg)
    {
      msg->cause_5gmm_present = true;

      cause_5gmm_t cause_5gmm;
      nas_mm::common_5g_mm_cause_set_value(DeregMsgInfo->MMCause, cause_5gmm);
      msg->cause_5gmm = cause_5gmm;
    }
    /*start timer T3522*/
    /*tmp not start timer*/

    /*indicate whether re-registration is needed or not in the Deregistration type IE*/
    /*get value from ttcn*/
    if (NR_NAS_MM_DEREG_REGISTRATION_NOT_REQUIRED == DeregMsgInfo->RegistrationRequired)
    {
      msg->de_registration_type.re_registration_required =
          de_registration_type_t::re_registration_required_type_::re_registration_not_required;
    }
    else // if(NR_NAS_MM_DEREG_REGISTRATION__REQUIRED == DeregMsgInfo->RegistrationRequired)
    {
      msg->de_registration_type.re_registration_required =
          de_registration_type_t::re_registration_required_type_::re_registration_required;
    }

    /*indicate access type*/
    if (NR_NAS_MM_DEREG_ACCESS_3GPP == DeregMsgInfo->AccessType)
    {
      msg->de_registration_type.access_type = de_registration_type_t::access_type_type::access_3_gpp;
    }
    else if (NR_NAS_MM_DEREG_ACCESS_NON_3GPP == DeregMsgInfo->AccessType)
    {
      msg->de_registration_type.access_type = de_registration_type_t::access_type_type::non_3_gpp_access;
    }
    else
    {
      msg->de_registration_type.access_type = de_registration_type_t::access_type_type::access_3_gpp_and_non_3_gpp_access;
    }

    /*release PDU*/
    /*switch off*/

    nas_tx = srsran::make_byte_buffer();
    if (nas_tx == nullptr)
    {
      m_nas_mm_logger.error("Couldn't allocate PDU in %s().", __FUNCTION__);
      return false;
    }
    this->pack_deregistration_request_to_ue(nas_tx, msg);

    /*TODO: send message to RRC message queue. */
    return true;
  }

  bool nas_mm::handle_deregistration_accept_from_ue(uint16_t enb_ue_id)
  {

    if (mm_adp->udp_.is_net_org_dereg_req_need_regreq_repeat)
    {
      this->nas_dereg_acc_message_to_ttcn();
      // 调用RRC连接释放函数
      // rrc_mm->start_rem_rel_user(enb_ue_id);
      send_start_rem_rel_user(enb_ue_id);
    }

    nas_context *nas_ctx = m_cnw->find_nas_ctx_from_rnti((uint32_t)enb_ue_id);
    /*change state to IDLE*/
    if (nas_ctx)
    {
      /*change state to IDLE*/
      nas_ctx->nrcm_ctx.state = NRCM_STATE_IDLE;
      /*clear ctx*/
      // Delete previous NAS context
      std::cout << " Delete previous NAS context " << std::endl;
      // m_cnw->delete_ue_nas_ctx(nas_ctx->nrcm_ctx.tmsi_5g);

      // m_cnw->delete_ue_ctx(nas_ctx->nrmm_ctx.suci);
    }
    return true;
  }

  bool nas_mm::handle_deregistration_accept_to_ue_ttcn(void)
  {
    srsran::unique_byte_buffer_t nas_tx;
    nas_tx = srsran::make_byte_buffer();
    if (nas_tx == nullptr)
    {
      m_nas_mm_logger.error("Couldn't allocate PDU in %s().", __FUNCTION__);
      return false;
    }
    this->pack_deregistration_accept_to_ue(nas_tx);
    /*TODO: send message to RRC message queue. */
    return true;
  }

  bool nas_mm::struct_mobile_identity_to_uint64(uint64_t *mobile_identity,
                                                srsran::nas_5g::mobile_identity_5gs_t *t_mobile_identity)
  {
    std::cout << "---------@@@@struct_mobile_identity_to_uint64@@----------" << std::endl;
    srsran::unique_byte_buffer_t buf_cmp = srsran::make_byte_buffer();
    buf_cmp->init();
    // for (uint8_t i = 0; i < 10; i++) {
    //   printf("0x%x:\n", *(buf_cmp->msg + i));
    // }
    asn1::bit_ref msg_bref(buf_cmp->msg, buf_cmp->get_tailroom());
    t_mobile_identity->pack(msg_bref);
    buf_cmp->N_bytes = msg_bref.distance_bytes();

    uint8_t temp_mobile_identity[buf_cmp.get()->N_bytes];
    memcpy(temp_mobile_identity, buf_cmp.get()->msg, buf_cmp.get()->N_bytes);

    srsran::console("Convert mobility identity byte stream:\n");
    for (uint8_t i = 0; i < buf_cmp.get()->N_bytes; i++)
    {
      printf("0x%0x, ", temp_mobile_identity[i]);
      if (0 == i % 9 && 0 != i)
      {
        printf("\n");
      }
    }
    printf("\n");

    // TODO: add u8->u64 func or forced convert?
    *mobile_identity = *(uint64_t *)temp_mobile_identity;
    assert(mobile_identity);
    return true;
  }

  void nas_mm::print_capability_5gmm(capability_5gmm_t capability_5gmm)
  {
    printf("capability_5gmm: sgc flag is %d\n", capability_5gmm.sgc);
    printf("capability_5gmm: iphc_cp_c_io_t_5g flag is %d\n", capability_5gmm.iphc_cp_c_io_t_5g);
    printf("capability_5gmm: n3_data flag is %d\n", capability_5gmm.n3_data);
    printf("capability_5gmm: cp_c_io_t_5g flag is %d\n", capability_5gmm.cp_c_io_t_5g);
    printf("capability_5gmm: restrict_ec flag is %d\n", capability_5gmm.restrict_ec);
    printf("capability_5gmm: lpp flag is %d\n", capability_5gmm.lpp);
    printf("capability_5gmm: ho_attach flag is %d\n", capability_5gmm.ho_attach);
    printf("capability_5gmm: s1_mode flag is %d\n", capability_5gmm.s1_mode);
    printf("capability_5gmm: racs flag is %d\n", capability_5gmm.racs);
    printf("capability_5gmm: nssaa flag is %d\n", capability_5gmm.nssaa);
    printf("capability_5gmm: lcs_5g flag is %d\n", capability_5gmm.lcs_5g);
    printf("capability_5gmm: v2_xcnpc5 flag is %d\n", capability_5gmm.v2_xcnpc5);
    printf("capability_5gmm: v2_xcepc5 flag is %d\n", capability_5gmm.v2_xcepc5);
    printf("capability_5gmm: v2_x flag is %d\n", capability_5gmm.v2_x);
    printf("capability_5gmm: up_c_io_t_5g flag is %d\n", capability_5gmm.up_c_io_t_5g);
    printf("capability_5gmm: srvcc_5g flag is %d\n", capability_5gmm.srvcc_5g);
    printf("capability_5gmm: ehc_cp_c_io_t_5g flag is %d\n", capability_5gmm.ehc_cp_c_io_t_5g);
    printf("capability_5gmm: multiple_up flag is %d\n", capability_5gmm.multiple_up);
    printf("capability_5gmm: wusa flag is %d\n", capability_5gmm.wusa);
    printf("capability_5gmm: cag flag is %d\n", capability_5gmm.cag);
  }

  void nas_mm::print_ue_security_capability(ue_security_capability_t ue_security_capability)
  {
    printf("ue_security_capability: ea0_5g_supported flag is %d\n", ue_security_capability.ea0_5g_supported);
    printf("ue_security_capability: ea1_128_5g_supported flag is %d\n", ue_security_capability.ea1_128_5g_supported);
    printf("ue_security_capability: ea2_128_5g_supported flag is %d\n", ue_security_capability.ea2_128_5g_supported);
    printf("ue_security_capability: ea3_128_5g_supported flag is %d\n", ue_security_capability.ea3_128_5g_supported);
    printf("ue_security_capability: ea4_5g_supported flag is %d\n", ue_security_capability.ea4_5g_supported);
    printf("ue_security_capability: ea5_5g_supported flag is %d\n", ue_security_capability.ea5_5g_supported);
    printf("ue_security_capability: ea6_5g_supported flag is %d\n", ue_security_capability.ea6_5g_supported);
    printf("ue_security_capability: ea7_5g_supported flag is %d\n", ue_security_capability.ea7_5g_supported);
    printf("ue_security_capability: ia0_5g_supported flag is %d\n", ue_security_capability.ia0_5g_supported);
    printf("ue_security_capability: ia1_128_5g_supported flag is %d\n", ue_security_capability.ia1_128_5g_supported);
    printf("ue_security_capability: ia2_128_5g_supported flag is %d\n", ue_security_capability.ia2_128_5g_supported);
    printf("ue_security_capability: ia3_128_5g_supported flag is %d\n", ue_security_capability.ia3_128_5g_supported);
    printf("ue_security_capability: ia4_5g_supported flag is %d\n", ue_security_capability.ia4_5g_supported);
    printf("ue_security_capability: ia5_5g_supported flag is %d\n", ue_security_capability.ia5_5g_supported);
    printf("ue_security_capability: ia6_5g_supported flag is %d\n", ue_security_capability.ia6_5g_supported);
    printf("ue_security_capability: ia7_5g_supported flag is %d\n", ue_security_capability.ia7_5g_supported);
    printf("ue_security_capability: eps_caps_present flag is %d\n", ue_security_capability.eps_caps_present);
    printf("ue_security_capability: eea0_supported flag is %d\n", ue_security_capability.eea0_supported);
    printf("ue_security_capability: eea1_128_supported flag is %d\n", ue_security_capability.eea1_128_supported);
    printf("ue_security_capability: eea2_128_supported flag is %d\n", ue_security_capability.eea2_128_supported);
    printf("ue_security_capability: eea3_128_supported flag is %d\n", ue_security_capability.eea3_128_supported);
    printf("ue_security_capability: eea4_supported flag is %d\n", ue_security_capability.eea4_supported);
    printf("ue_security_capability: nea13_supported flag is %d\n", ue_security_capability.nea13_supported);
    printf("ue_security_capability: nea14_supported flag is %d\n", ue_security_capability.nea14_supported);
    printf("ue_security_capability: eea7_supported flag is %d\n", ue_security_capability.eea7_supported);
    printf("ue_security_capability: eia0_supported flag is %d\n", ue_security_capability.eia0_supported);
    printf("ue_security_capability: eia1_128_supported flag is %d\n", ue_security_capability.eia1_128_supported);
    printf("ue_security_capability: eia2_128_supported flag is %d\n", ue_security_capability.eia2_128_supported);
    printf("ue_security_capability: eia3_128_supported flag is %d\n", ue_security_capability.eia3_128_supported);
    printf("ue_security_capability: eia4_supported flag is %d\n", ue_security_capability.eia4_supported);
    printf("ue_security_capability: nia13_supported flag is %d\n", ue_security_capability.nia13_supported);
    printf("ue_security_capability: nia14_supported flag is %d\n", ue_security_capability.nia14_supported);
    printf("ue_security_capability: eia7_supported flag is %d\n", ue_security_capability.eia7_supported);
  }

  void nas_mm::print_ie_suci(mobile_identity_5gs_t::suci_s &suci)
  {
    printf("suci_s: supi_format is %s\n", suci.supi_format.to_string());
    printf("suci_s: mcc is : \n");
    for (int i = 0; i < 3; i++)
    {
      printf("%d, ", suci.mcc[i]);
    }
    printf("\n");

    printf("suci_s: mnc is : \n");
    for (int i = 0; i < 3; i++)
    {
      printf("%d, ", suci.mnc[i]);
    }
    printf("\n");

    printf("suci_s: routing_indicator is : %s\n", suci.protection_scheme_id.to_string());

    printf("suci_s: home_network_public_key_identifier is : %d\n", suci.home_network_public_key_identifier);

    printf("suci_s: scheme_output is :\n");
    for (auto it = suci.scheme_output.begin(); it != suci.scheme_output.end(); it++)
    {
      printf("%d, ", *it);
    }
    printf("\n");
  }

  void nas_mm::nas_authencation_reponse_message_to_ttcn(authentication_response_t &msg)
  {

    std::cout << "srsran send message to TTCN------authencation" << std::endl;
    srsran::unique_byte_buffer_t authentication_resopn_ttcn = srsran::make_byte_buffer();
    authentication_resopn_ttcn->msg[0] = 0x01;
    authentication_resopn_ttcn->msg[1] = 0x02;
    authentication_resopn_ttcn->msg[2] = 0x00;
    authentication_resopn_ttcn->msg[3] = 0x09; //  test_id
    authentication_resopn_ttcn->msg[4] = 0x00; //  test_id
    authentication_resopn_ttcn->msg[5] = 0x00; // data_length
    authentication_resopn_ttcn->msg[6] = 0x05; // data_length
    authentication_resopn_ttcn->msg[7] = 0x3e; // message_type
    authentication_resopn_ttcn->msg[8] = 0x2d;
    authentication_resopn_ttcn->msg[9] = 0x10;
    authentication_resopn_ttcn->msg[10] = msg.authentication_response_parameter.res[0];
    authentication_resopn_ttcn->msg[11] = msg.authentication_response_parameter.res[1];
    authentication_resopn_ttcn->N_bytes = 12;
    srsran::unique_byte_buffer_t udp = srsran::make_byte_buffer();
    mm_adp->udp_.send_ttcn_msg_enb(std::move(authentication_resopn_ttcn));
    udp->init();
    std::cout << "this is autencation resopnse produce and send to security command message" << std::endl;
    while (true)
    {
      if (mm_adp->udp_.nas_mm_receive_info.size() != 0)
      {
        mm_adp->udp_.nas_mm_receive_info.try_pop(udp);
        std::cout << "receive security_command_message" << std::endl;
        break;
      }
    }
    // 处理接受到的smc_command消息
    // 暂时做打印消息，Smc_command还是由处理函数提供
    for (uint32_t i = 0; i < udp->N_bytes; i++)
    {
      std::cout << "this message from TTcn" << std::endl;
      printf("0x%x\n", udp->msg[i]);
    }

  }

  // bool nas_mm::struct_mobile_identity_to_guti(srsepc::nas_guti* guti, mobile_identity_5gs_t* t_mobile_identity)
  // {
  //   srsran::unique_byte_buffer_t buf_cmp = srsran::make_byte_buffer();
  //   asn1::bit_ref                msg_bref(buf_cmp->msg, buf_cmp->get_tailroom());
  //   t_mobile_identity->pack(msg_bref);
  //   buf_cmp->N_bytes = msg_bref.distance_bytes();

  //   memcpy(guti->getGutiAddress(0), buf_cmp.get()->msg, buf_cmp.get()->N_bytes);

  //   return true;
  // }

  void nas_mm::struct_mobile_identity_to_array(uint8_t **mobile_identity,
                                               srsran::nas_5g::mobile_identity_5gs_t *t_mobile_identity)
  {
    srsran::unique_byte_buffer_t buf_cmp = srsran::make_byte_buffer();
    asn1::bit_ref msg_bref(buf_cmp->msg, buf_cmp->get_tailroom());
    t_mobile_identity->pack(msg_bref);
    buf_cmp->N_bytes = msg_bref.distance_bytes();

    *mobile_identity = new uint8_t[buf_cmp.get()->N_bytes];
    memcpy(mobile_identity, buf_cmp.get()->msg, buf_cmp.get()->N_bytes);
    assert(mobile_identity);

    return;
  }

  void nas_mm::nas_security_mode_complete_message_to_ttcn(security_mode_complete_t &msg)
  {
    srsran::unique_byte_buffer_t sercrity_mode_complete_info = srsran::make_byte_buffer();
    sercrity_mode_complete_info->msg[0] = 0x01;
    sercrity_mode_complete_info->msg[1] = 0x02;
    sercrity_mode_complete_info->msg[2] = 0x00;
    sercrity_mode_complete_info->msg[3] = 0x00; // test_id
    sercrity_mode_complete_info->msg[4] = 0x00; // test_id
    sercrity_mode_complete_info->msg[5] = 0x00; // length
    sercrity_mode_complete_info->msg[6] = 0x06;
    sercrity_mode_complete_info->msg[7] = 0x51; // message_type
    sercrity_mode_complete_info->msg[8] = msg.nas_message_container.nas_message_container[0];
    sercrity_mode_complete_info->msg[9] = msg.nas_message_container.nas_message_container[1];
    sercrity_mode_complete_info->msg[10] = msg.nas_message_container.nas_message_container[2]; // 0x41;
    sercrity_mode_complete_info->msg[11] = msg.nas_message_container.nas_message_container[3];
    sercrity_mode_complete_info->msg[12] = msg.nas_message_container.nas_message_container[4];
    sercrity_mode_complete_info->N_bytes = 13;
    
    mm_adp->udp_.send_ttcn_msg_enb(std::move(sercrity_mode_complete_info));
  }

  void nas_mm::nas_dereg_req_message_to_ttcn(deregistration_request_ue_originating_t &msg)
  {
    std::cout << "sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss3" << std::endl;
    srsran::unique_byte_buffer_t dereg_reg_reg_info = srsran::make_byte_buffer();
    dereg_reg_reg_info->msg[0] = 0x01;
    dereg_reg_reg_info->msg[1] = 0x02;
    dereg_reg_reg_info->msg[2] = 0x00;
    dereg_reg_reg_info->msg[3] = 0x07; // test_id=813
    dereg_reg_reg_info->msg[4] = 0x0b;
    dereg_reg_reg_info->msg[5] = 0x00; // data_length
    dereg_reg_reg_info->msg[6] = 0x02;
    dereg_reg_reg_info->msg[7] = msg.de_registration_type.switch_off.value;
    dereg_reg_reg_info->N_bytes = 8;
    // mm_adp->udp_.send_ttcn_info.try_push(std::move(dereg_reg_reg_info));
    mm_adp->udp_.send_ttcn_msg_enb(std::move(dereg_reg_reg_info));
  }

  void nas_mm::nas_dereg_acc_message_to_ttcn()
  {
    srsran::unique_byte_buffer_t dereg_req_acc_info = srsran::make_byte_buffer();
    dereg_req_acc_info->msg[0] = 0x01;
    dereg_req_acc_info->msg[1] = 0x02;
    dereg_req_acc_info->msg[2] = 0x00;
    dereg_req_acc_info->msg[3] = 0x00; // test_id
    dereg_req_acc_info->msg[4] = 0x00;
    dereg_req_acc_info->msg[5] = 0x00; // data_lengtg]
    dereg_req_acc_info->msg[6] = 0x01;
    dereg_req_acc_info->msg[7] = 0x37;
    dereg_req_acc_info->N_bytes = 8;
    // mm_adp->udp_.send_ttcn_info.try_push(std::move(dereg_req_acc_info));
    mm_adp->udp_.send_ttcn_msg_enb(std::move(dereg_req_acc_info));
  }

  bool nas_mm::struct_mobile_identity_to_guti(srsepc::nas_guti *guti, mobile_identity_5gs_t *t_mobile_identity)
  {
    srsran::unique_byte_buffer_t buf_cmp = srsran::make_byte_buffer();
    asn1::bit_ref msg_bref(buf_cmp->msg, buf_cmp->get_tailroom());
    t_mobile_identity->pack(msg_bref);
    buf_cmp->N_bytes = msg_bref.distance_bytes();

    memcpy(guti->getGutiAddress(0), buf_cmp.get()->msg, buf_cmp.get()->N_bytes);

    return true;
  }

  bool nas_mm::handle_suci_registration_request_unknown_ue(uint64_t supi,
                                                           uint16_t enb_id,
                                                           uint16_t cnw_ue_id,
                                                           registration_type_5gs_t::registration_type_type reg_type,
                                                           registration_request_t &msg)
  {
    if (reg_type == 0b010 || reg_type == 0b011)
    {
      m_nas_mm_logger.error("Registration request -- registration type is: %s, but mobile identity is suci.\n",
                            reg_type.to_string());
    }
    std::cout << "---------@@handle_suci_registration_request_unknown_ue@@--------" << std::endl;
    nas_context *nas_ctx = new nas_context();
    srsran::unique_byte_buffer_t nas_tx;

    /* 1.save a new nas context */
    /* 1.1 add mm context. */
    nas_ctx->nrmm_ctx.supi = supi;
    nas_ctx->nrmm_ctx.state = NRMM_STATE_DEREGISTERED;
    nas_ctx->nrmm_ctx.reg_type = msg.registration_type_5gs.registration_type;
    nas_ctx->nrmm_ctx.e_pro_type = PROC_REGISTERED;

    /* 1.2 add cm context. */
    nas_ctx->nrcm_ctx.enb_id = enb_id;
    nas_ctx->nrcm_ctx.rnti = cnw_ue_id;
    nas_ctx->nrcm_ctx.state = NRCM_STATE_CONNECTED;

    /* 1.3 add sec context. */
    nas_ctx->m_sec_ctx.ngksi = 0;
    nas_ctx->m_sec_ctx.ul_nas_count = 0;
    nas_ctx->m_sec_ctx.dl_nas_count = 0;

    nas_ctx->m_sec_ctx.integ_algo = m_cnw->get_cnw_args()->integrity_algo;
    nas_ctx->m_sec_ctx.cipher_algo = m_cnw->get_cnw_args()->encryption_algo;

    if (true == msg.capability_5gmm_present)
    {
      // handle or not?
    }

    /*save ue_security_capability 2023-11-18add*/
    if (true == msg.ue_security_capability_present)
    {
      nas_ctx->m_sec_ctx.ue_network_cap.ea0_5g_supported = msg.ue_security_capability.ea0_5g_supported;
      nas_ctx->m_sec_ctx.ue_network_cap.ea1_128_5g_supported = msg.ue_security_capability.ea1_128_5g_supported;
      nas_ctx->m_sec_ctx.ue_network_cap.ea2_128_5g_supported = msg.ue_security_capability.ea2_128_5g_supported;
      nas_ctx->m_sec_ctx.ue_network_cap.ea3_128_5g_supported = msg.ue_security_capability.ea3_128_5g_supported;
      nas_ctx->m_sec_ctx.ue_network_cap.ea4_5g_supported = msg.ue_security_capability.ea4_5g_supported;
      nas_ctx->m_sec_ctx.ue_network_cap.ea5_5g_supported = msg.ue_security_capability.ea5_5g_supported;
      nas_ctx->m_sec_ctx.ue_network_cap.ea6_5g_supported = msg.ue_security_capability.ea6_5g_supported;
      nas_ctx->m_sec_ctx.ue_network_cap.ea7_5g_supported = msg.ue_security_capability.ea7_5g_supported;
      nas_ctx->m_sec_ctx.ue_network_cap.ia0_5g_supported = msg.ue_security_capability.ia0_5g_supported;

      nas_ctx->m_sec_ctx.ue_network_cap.ia0_5g_supported = msg.ue_security_capability.ia0_5g_supported;
      nas_ctx->m_sec_ctx.ue_network_cap.ia1_128_5g_supported = msg.ue_security_capability.ia1_128_5g_supported;
      nas_ctx->m_sec_ctx.ue_network_cap.ia2_128_5g_supported = msg.ue_security_capability.ia2_128_5g_supported;
      nas_ctx->m_sec_ctx.ue_network_cap.ia3_128_5g_supported = msg.ue_security_capability.ia3_128_5g_supported;
      nas_ctx->m_sec_ctx.ue_network_cap.ia4_5g_supported = msg.ue_security_capability.ia4_5g_supported;
      nas_ctx->m_sec_ctx.ue_network_cap.ia5_5g_supported = msg.ue_security_capability.ia5_5g_supported;
      nas_ctx->m_sec_ctx.ue_network_cap.ia6_5g_supported = msg.ue_security_capability.ia6_5g_supported;
      nas_ctx->m_sec_ctx.ue_network_cap.ia7_5g_supported = msg.ue_security_capability.ia7_5g_supported;

      nas_ctx->m_sec_ctx.ue_network_cap.eea0_supported = msg.ue_security_capability.eea0_supported;
      nas_ctx->m_sec_ctx.ue_network_cap.eea1_128_supported = msg.ue_security_capability.eea1_128_supported;
      nas_ctx->m_sec_ctx.ue_network_cap.eea2_128_supported = msg.ue_security_capability.eea2_128_supported;
      nas_ctx->m_sec_ctx.ue_network_cap.eea3_128_supported = msg.ue_security_capability.eea3_128_supported;
      nas_ctx->m_sec_ctx.ue_network_cap.eea4_supported = msg.ue_security_capability.eea4_supported;
      nas_ctx->m_sec_ctx.ue_network_cap.nea13_supported = msg.ue_security_capability.nea13_supported;
      nas_ctx->m_sec_ctx.ue_network_cap.nea14_supported = msg.ue_security_capability.nea14_supported;
      nas_ctx->m_sec_ctx.ue_network_cap.eea7_supported = msg.ue_security_capability.eea7_supported;

      nas_ctx->m_sec_ctx.ue_network_cap.eia0_supported = msg.ue_security_capability.eia0_supported;
      nas_ctx->m_sec_ctx.ue_network_cap.eia1_128_supported = msg.ue_security_capability.eia1_128_supported;
      nas_ctx->m_sec_ctx.ue_network_cap.eia2_128_supported = msg.ue_security_capability.eia2_128_supported;
      nas_ctx->m_sec_ctx.ue_network_cap.eia3_128_supported = msg.ue_security_capability.eia3_128_supported;
      nas_ctx->m_sec_ctx.ue_network_cap.eia4_supported = msg.ue_security_capability.eia4_supported;
      nas_ctx->m_sec_ctx.ue_network_cap.nia13_supported = msg.ue_security_capability.nia13_supported;
      nas_ctx->m_sec_ctx.ue_network_cap.nia14_supported = msg.ue_security_capability.nia14_supported;
      nas_ctx->m_sec_ctx.ue_network_cap.eia7_supported = msg.ue_security_capability.eia7_supported;
    }

    /*TODO: 1.4 add sm context.? */

    /* 2.1 generate AUTN && RAND && Kausf && Kseaf && Kamf value. */
    if (!gen_auth_info_answer(nas_ctx,
                              nas_ctx->nrmm_ctx.supi,
                              nas_ctx->m_sec_ctx.k_ausf,
                              nas_ctx->m_sec_ctx.k_seaf,
                              nas_ctx->m_sec_ctx.k_amf,
                              nas_ctx->m_sec_ctx.autn,
                              nas_ctx->m_sec_ctx.rand,
                              nas_ctx->m_sec_ctx.xres))
    {
      srsran::console("User not found. SUPI %015" PRIu64 "\n", nas_ctx->nrmm_ctx.supi);
      m_nas_mm_logger.info("User not found. SUPI %015" PRIu64 "", nas_ctx->nrmm_ctx.supi);
      return false;
    }

    nas_ctx->m_sec_ctx.ngksi = 1;

    /* 2.2 save nas content.*/
    m_cnw->add_nas_ctx_to_rnti_map(nas_ctx);
    /* 2.3 construct aka request message. */
    nas_tx = srsran::make_byte_buffer();
    if (nas_tx == nullptr)
    {
      m_nas_mm_logger.error("Couldn't allocate PDU in %s().", __FUNCTION__);
      return false;
    }

    if(mm_adp->udp_.is_nas_identity_request) {
      cout << "is_nas_identity_request " << endl;

      identity_request_t identity_request;
      identity_request.identity_type.type_of_identity = identity_type_5gs_t::identity_types_::options::suci;
      
      this->pack_identity_request(nas_tx, &identity_request);
      send_mm_dl_msg(cnw_ue_id, std::move(nas_tx));
      return true;
    }

    this->pack_authentication_request(nas_tx, nas_ctx);
    /* 2.4 send message to RRC message queue. */
    send_mm_dl_msg(cnw_ue_id, std::move(nas_tx));
    m_nas_mm_logger.info("Downlink NAS: Sending Authentication Request");
    srsran::console("Downlink NAS: Sending Authentication Request\n");
    return true;
  }

  bool nas_mm::handle_suci_registration_request_known_ue(nas_context *nas_ctx,
                                                         uint16_t enb_id,
                                                         uint16_t cnw_ue_id,
                                                         registration_type_5gs_t::registration_type_type reg_type,
                                                         registration_request_t &msg)
  {
    if (reg_type == 0b010 || reg_type == 0b011)
    {
      m_nas_mm_logger.error("Registration request -- registration type is: %s, but mobile identity is suci.\n",
                            reg_type.to_string());
    }
    std::cout << "---------@@handle_suci_registration_request_known_ue@@--------" << std::endl;
    bool err = false;
    // Delete previous GTP-U session

    // Release previous context in the eNB, if present
    if (nas_ctx->nrcm_ctx.rnti != 0)
    {
      // invoke rrc's functions.
    }
    // Delete previous NAS context
    // m_cnw->delete_ue_ctx(nas_ctx->nrmm_ctx.suci);

    // Handle new attach
    err = nas_mm::handle_suci_registration_request_unknown_ue(nas_ctx->nrmm_ctx.supi, enb_id, cnw_ue_id, reg_type, msg);
    return err;
  }

  bool nas_mm::handle_guti_registration_request_unknown_ue(srsepc::nas_guti guti,
                                                           uint16_t enb_id,
                                                           uint16_t cnw_ue_id,
                                                           registration_type_5gs_t::registration_type_type reg_type,
                                                           registration_request_t &msg)
  {
    std::cout << " as_mm::handle_guti_registration_request_unknown_ue  " << std::endl;
    nas_context *nas_ctx = new nas_context();
    IDENTITY_MSG_INFO id_msg = {NR_NAS_MM_IDENTITY_TYPE_SUCI};

    /**
     * 1. Could not find IMSI from GUTI, send identity request.
     * The SUCI will be set when the identity response is received
     * */

    /* 1.1 add mm context. */
    nas_ctx->nrmm_ctx.suci = 0;
    nas_ctx->nrmm_ctx.state = NRMM_STATE_DEREGISTERED;
    nas_ctx->nrmm_ctx.reg_type = msg.registration_type_5gs.registration_type;

    /* 1.2 add cm context. */
    nas_ctx->nrcm_ctx.enb_id = enb_id;
    nas_ctx->nrcm_ctx.rnti = cnw_ue_id;
    nas_ctx->nrcm_ctx.state = NRCM_STATE_CONNECTED;

    /* 1.3 add sec context. */
    nas_ctx->m_sec_ctx.ngksi = 0;
    nas_ctx->m_sec_ctx.ul_nas_count = 0;
    nas_ctx->m_sec_ctx.dl_nas_count = 0;

    /* 2. Store temporary ue context, wait identity response. */
    m_cnw->add_nas_ctx_to_rnti_map(nas_ctx);

    /* 3. Send Identity Request */
    // this->handle_identity_request_ttcn(id_msg);

    return true;
  }

  bool nas_mm::handle_guti_registration_request_known_ue(nas_context *nas_ctx,
                                                         uint16_t enb_id,
                                                         uint16_t cnw_ue_id,
                                                         registration_type_5gs_t::registration_type_type reg_type,
                                                         registration_request_t &msg)
  {
    std::cout << "handle_guti_registration_request_known_ue xxxxxxx" << std::endl;
    bool msg_valid = false;
    srsran::unique_byte_buffer_t nas_tx;

    nrmm_ctx_t *nrmm_ctx = &nas_ctx->nrmm_ctx;
    nrcm_ctx_t *nrcm_ctx = &nas_ctx->nrcm_ctx;
    sec_ctx_t *sec_ctx = &nas_ctx->m_sec_ctx;

    srsran::console("Found UE context. SUCI: %015" PRIu64 ", old RNTI Id %d\n", nrmm_ctx->suci, cnw_ue_id);

    // TODO: integerity check?
    //  msg_valid = nas_ctx->integrity_check(nas_rx);
    if (msg_valid == true && nrmm_ctx->state == NRMM_STATE_DEREGISTERED)
    {
      srsran::console(
          "GUTI Attach -- NAS Integrity OK. UL count %d, DL count %d\n", sec_ctx->ul_nas_count, sec_ctx->dl_nas_count);
      m_nas_mm_logger.info(
          "GUTI Attach -- NAS Integrity OK. UL count %d, DL count %d", sec_ctx->ul_nas_count, sec_ctx->dl_nas_count);

      // Create new MME UE S1AP Identity
      nrcm_ctx->rnti = cnw_ue_id;
      nrcm_ctx->enb_id = enb_id;

      // Save Attach type
      nrmm_ctx->reg_type = msg.registration_type_5gs.registration_type.value;

      // Set security flag
      nrmm_ctx->isSecCplt = true;

      // Re-generate K_eNB
      srsran::security_generate_k_enb(sec_ctx->k_amf, sec_ctx->ul_nas_count, sec_ctx->k_enb);
      m_nas_mm_logger.info("Generating KeNB with UL NAS COUNT: %d", sec_ctx->ul_nas_count);
      srsran::console("Generating KeNB with UL NAS COUNT: %d\n", sec_ctx->ul_nas_count);
      m_nas_mm_logger.info(sec_ctx->k_enb, 32, "Key eNodeB (k_enb)");

      // Send Register accept.
      sec_ctx->ul_nas_count++;

      // Store context based on MME UE S1AP id
      m_cnw->add_nas_ctx_to_rnti_map(nas_ctx);

      return true;
    }
    else
    {
      /* periodic registration ans mobile registration*/
      if (nrmm_ctx->state == NRMM_STATE_REGISTERED) 
      {
        m_nas_mm_logger.info("Received 5G-GUTI-Reg Request from REGISTERED user.");
        srsran::console("Received 5G-GUTI-Reg Request from REGISTERED user.\n");

        // Delete previous Ctx, restart authentication
        // Detaching previoulsy attached UE.
        // TODO: how to do?

        /* ttcn 8_5 periodic_registration */
        if(mm_adp->udp_.is_periodic_registration_request) 
        {
          srsran::unique_byte_buffer_t is_periodic_registration_request_pdu = srsran::make_byte_buffer();
          is_periodic_registration_request_pdu->init();

          pack_registration_accept(is_periodic_registration_request_pdu, nas_ctx);
          send_mm_dl_msg(cnw_ue_id, std::move(is_periodic_registration_request_pdu));
          nas_ctx->m_sec_ctx.dl_nas_count++;
          return true;
        }

        /* ttcn 8_5 periodic_registration */
        if(mm_adp->udp_.is_authencation_faileure_repeated_ngksi) 
        {
          srsran::unique_byte_buffer_t is_authencation_faileure_repeated_ngksi_pdu = srsran::make_byte_buffer();
          is_authencation_faileure_repeated_ngksi_pdu->init();
        
          srsran::unique_byte_buffer_t ttcn_pdu = srsran::make_byte_buffer();
          ttcn_pdu->init();

          while (true)
          {
            if (mm_adp->udp_.nas_mm_receive_info.size() != 0)
            {
              mm_adp->udp_.nas_mm_receive_info.try_pop(ttcn_pdu);

              std::cout << "receive info from TTCN" << std::endl;
              break;
            }
          }

          pack_authentication_request(is_authencation_faileure_repeated_ngksi_pdu, nas_ctx);
          send_mm_dl_msg(cnw_ue_id, std::move(is_authencation_faileure_repeated_ngksi_pdu));
          nas_ctx->m_sec_ctx.dl_nas_count++;
          return true;
        }

      }

      // Set security flag
      nrmm_ctx->isSecCplt = false;

      sec_ctx->ul_nas_count = 0;
      sec_ctx->dl_nas_count = 0;

      // Make sure context from previous NAS connections is not present
      if (nrcm_ctx->rnti != 0)
      {
        m_cnw->release_ue_nrcm_ctx(nrcm_ctx->rnti);
      }
      nrcm_ctx->rnti = cnw_ue_id;

      // Set EMM as de-registered
      nrmm_ctx->state = NRMM_STATE_DEREGISTERED;

      // Save Attach type
      nrmm_ctx->reg_type = msg.registration_type_5gs.registration_type.value;

      // Set eNB information
      nrcm_ctx->enb_id = enb_id;

      // Store context based on RNTI
      m_cnw->add_nas_ctx_to_rnti_map(nas_ctx);

      // NAS integrity failed. Re-start authentication process.
      srsran::console("GUTI Reg request NAS integrity failed.\n");
      srsran::console("RE-starting authentication procedure.\n");

      // Get Authentication Vectors from HSS
      if (!gen_auth_info_answer(nas_ctx,
                                nrmm_ctx->supi,
                                sec_ctx->k_ausf,
                                sec_ctx->k_seaf,
                                sec_ctx->k_amf,
                                sec_ctx->autn,
                                sec_ctx->rand,
                                sec_ctx->xres))
      {
        srsran::console("User not found. SUPI %015" PRIu64 "\n", nrmm_ctx->supi);
        m_nas_mm_logger.info("User not found. SUPI %015" PRIu64 "", nrmm_ctx->supi);
        return false;
      }

      // Restarting security context. Reseting eKSI to 0.
      sec_ctx->ngksi = 0;
      nas_tx = srsran::make_byte_buffer();
      if (nas_tx == nullptr)
      {
        m_nas_mm_logger.error("Couldn't allocate PDU in %s().", __FUNCTION__);
        return false;
      }
      this->pack_authentication_request(nas_tx, nas_ctx);

      /*TODO: send message to RRC message queue. */
      send_mm_dl_msg(cnw_ue_id, std::move(nas_tx));
      m_nas_mm_logger.info("Downlink NAS: Sent Authentication Request");
      srsran::console("Downlink NAS: Sent Authentication Request\n");
      return true;
    }
    return true;
  }

  bool nas_mm::pack_registration_reject(srsran::unique_byte_buffer_t &nas_buffer, uint8_t gmm_cause)
  {
    m_nas_mm_logger.info("Packing Configuration registration reject");
    nas_5gs_msg nas_msg;
    /*handl header*/
    nas_msg.hdr.message_type.value = msg_types::registration_reject;
    nas_msg.hdr.spare_1_2_version = 0X01;
    registration_reject_t &reg_req_msg = nas_msg.set_registration_reject();
    if (gmm_cause == 0x16) // 判断TTCN传下来的消息是否为0x16;
    {
      reg_req_msg.t3346_value_present = true;
      reg_req_msg.cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::congestion;
      // reg_req_msg.t3346_value.timer_value=0x86; //30 s
      reg_req_msg.t3346_value.timer_value = 0xa3; // 3 fenzhong
      // reg_req_msg.t3346_value.timer_value=0x65;   //10s
    }
    else if (gmm_cause == 0x03)
    {
      reg_req_msg.cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::illegal_ue;
    }
    else if(gmm_cause==0x0b)
    {
      reg_req_msg.cause_5gmm.cause_5gmm=cause_5gmm_t::cause_5gmm_type::plmn_not_allowed;
    }
    else if(gmm_cause==0x5f)
    {
      reg_req_msg.cause_5gmm.cause_5gmm=cause_5gmm_t::cause_5gmm_type::semantically_incorrect_message;
      reg_req_msg.t3502_value_present=true;
      reg_req_msg.t3502_value.timer_value=0x1b;
    }
    else if(gmm_cause == 0x0c) 
    {
      reg_req_msg.cause_5gmm.cause_5gmm=cause_5gmm_t::cause_5gmm_type::tracking_area_not_allowed;
    }
    else if(gmm_cause == 0x0f) 
    {
      reg_req_msg.cause_5gmm.cause_5gmm=cause_5gmm_t::cause_5gmm_type::no_suitable_cells_in_tracking_area;
    }
    std::cout << "@@@@@@@@@@@@@@@@@@@@@@@-----------------------------------------------reg_rej----------------------------------------@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << std::endl;
    // std::cout<<reg_req_msg.cause_5gmm.cause_5gmm<<std::endl;
    std::cout << "@@@@@@@@@@@@@@@@@@@@@@@-----------------------------------------------reg_rej----------------------------------------@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << std::endl;
    SRSASN_CODE err = nas_msg.pack(nas_buffer);
    if (err != SRSASN_SUCCESS)
    {
      m_nas_mm_logger.error("Error packing Registration Reject");
      srsran::console("Error packing Registration Reject\n");
      return false;
    }

    //PCAP
    if(m_cnw->get_cnw_args()->pcap_net.enable) {  
      if(nas_msg.hdr.security_header_type == nas_5gs_hdr::security_header_type_opts::plain_5gs_nas_message) {
        m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg, nas_buffer->N_bytes);
      }
      else {
        if(nas_msg.hdr.spare_1_2_version == 0x00) {
          m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg + 7, nas_buffer->N_bytes - 7);
        }
        else {
          m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg + 11, nas_buffer->N_bytes - 11);
        }
      } 
    } 

    return true;
  }

  bool nas_mm::handle_nas_message_container(nas_context *nas_ctx,
                                            uint16_t enb_id,
                                            uint16_t cnw_ue_id,
                                            msg_types message_type,
                                            message_container_t *message_container)
  {
    /*暂时不做处理，打印验证*/
    srsran::console("---------------handle_nas_message_container.---------\n");
    for (int i = 0; i < 14; ++i)
    {
      srsran::console("---------------handle_nas_message_container.-----0x%x----\n", message_container->nas_message_container[i]);
    }
    srsran::console("---------------handle_nas_message_container.-----0x%x----\n", message_container->nas_message_container[32]);
    return true;
  }

  bool nas_mm::integrity_check(byte_buffer_t *pdu, nas_context *nas_ctx)
  {
    if (nas_ctx->m_sec_ctx.spare_1_2_version == 0)
    {
      uint8_t exp_mac[4] = {};
      const uint8_t *mac = &pdu->msg[MAC_5G_OFFSET];
      sec_ctx_t sec_ctx = nas_ctx->m_sec_ctx;

      uint32_t estimated_count = (sec_ctx.ul_nas_count & 0x00FFFF00u) | pdu->msg[SEQ_5G_OFFSET];

      switch (sec_ctx.integ_algo)
      {
      case srsran::INTEGRITY_ALGORITHM_ID_EIA0:
        break;
      case srsran::INTEGRITY_ALGORITHM_ID_128_EIA1:

        srsran::security_128_eia1(&sec_ctx.k_nas_int[16],
                                  estimated_count,
                                  NAS_5G_BEARER,
                                  srsran::SECURITY_DIRECTION_UPLINK,
                                  &pdu->msg[SEQ_5G_OFFSET],
                                  pdu->N_bytes - SEQ_5G_OFFSET,
                                  &exp_mac[0]);
        break;
      case srsran::INTEGRITY_ALGORITHM_ID_128_EIA2:
        srsran::security_128_eia2(&sec_ctx.k_nas_int[16],
                                  estimated_count,
                                  NAS_5G_BEARER,
                                  srsran::SECURITY_DIRECTION_UPLINK,
                                  &pdu->msg[SEQ_5G_OFFSET],
                                  pdu->N_bytes - SEQ_5G_OFFSET,
                                  &exp_mac[0]);
        break;
      case srsran::INTEGRITY_ALGORITHM_ID_128_EIA3:
        srsran::security_128_eia3(&sec_ctx.k_nas_int[16],
                                  estimated_count,
                                  NAS_5G_BEARER,
                                  srsran::SECURITY_DIRECTION_UPLINK,
                                  &pdu->msg[SEQ_5G_OFFSET],
                                  pdu->N_bytes - SEQ_5G_OFFSET,
                                  &exp_mac[0]);
        break;
      default:
        break;
      }
      // Check if expected mac equals the sent mac
      for (int i = 0; i < 4; i++)
      {
        if (exp_mac[i] != mac[i])
        {
          srsran::console("Integrity check failure. Algorithm=EIA%d", (int)sec_ctx.integ_algo);
          srsran::console("UL Local: est_count=%d, old_count=%d, MAC=[%02x %02x %02x %02x], "
                          "Received: UL count=%d, MAC=[%02x %02x %02x %02x]",
                          estimated_count,
                          sec_ctx.ul_nas_count,
                          exp_mac[0],
                          exp_mac[1],
                          exp_mac[2],
                          exp_mac[3],
                          pdu->msg[6],
                          mac[0],
                          mac[1],
                          mac[2],
                          mac[3]);
          return false;
        }
      }
      m_nas_mm_logger.info("Integrity check ok. Local: count=%d, Received: count=%d", estimated_count, pdu->msg[5]);
      sec_ctx.ul_nas_count = estimated_count;
    }
    else // 1.2版本 8字节长度MAC
    {
      uint8_t exp_mac[8] = {};
      const uint8_t *mac = &pdu->msg[MAC_5G_OFFSET];
      sec_ctx_t sec_ctx = nas_ctx->m_sec_ctx;

      uint32_t estimated_count = (sec_ctx.ul_nas_count & 0x00FFFF00u) | pdu->msg[SEQ_5G_OFFSET + 4];

      switch (sec_ctx.integ_algo)
      {
      case srsran::INTEGRITY_ALGORITHM_ID_EIA0:
        break;
      case srsran::INTEGRITY_ALGORITHM_ID_128_EIA1:

        srsran::security_128_eia1(&sec_ctx.k_nas_int[16],
                                  estimated_count,
                                  NAS_5G_BEARER,
                                  srsran::SECURITY_DIRECTION_UPLINK,
                                  &pdu->msg[SEQ_5G_OFFSET + 4],
                                  pdu->N_bytes - SEQ_5G_OFFSET - 4,
                                  &exp_mac[4]);
        break;
      case srsran::INTEGRITY_ALGORITHM_ID_128_EIA2:
        srsran::security_128_eia2(&sec_ctx.k_nas_int[16],
                                  estimated_count,
                                  NAS_5G_BEARER,
                                  srsran::SECURITY_DIRECTION_UPLINK,
                                  &pdu->msg[SEQ_5G_OFFSET + 4],
                                  pdu->N_bytes - SEQ_5G_OFFSET - 4,
                                  &exp_mac[4]);
        break;
      case srsran::INTEGRITY_ALGORITHM_ID_128_EIA3:
        srsran::security_128_eia3(&sec_ctx.k_nas_int[16],
                                  estimated_count,
                                  NAS_5G_BEARER,
                                  srsran::SECURITY_DIRECTION_UPLINK,
                                  &pdu->msg[SEQ_5G_OFFSET + 4],
                                  pdu->N_bytes - SEQ_5G_OFFSET - 4,
                                  &exp_mac[4]);
        break;
      default:
        break;
      }
      // Check if expected mac equals the sent mac
      for (int i = 0; i < 8; i++)
      {
        if (exp_mac[i] != mac[i])
        {
          srsran::console("Integrity check failure. Algorithm=EIA%d", (int)sec_ctx.integ_algo);
          srsran::console("UL Local: est_count=%d, old_count=%d, MAC=[%02x %02x %02x %02x %02x %02x %02x %02x], "
                          "Received: UL count=%d, MAC=[%02x %02x %02x %02x %02x %02x %02x %02x]",
                          estimated_count,
                          sec_ctx.ul_nas_count,
                          exp_mac[0],
                          exp_mac[1],
                          exp_mac[2],
                          exp_mac[3],
                          exp_mac[4],
                          exp_mac[5],
                          exp_mac[6],
                          exp_mac[7],
                          pdu->msg[10],
                          mac[0],
                          mac[1],
                          mac[2],
                          mac[3],
                          mac[4],
                          mac[5],
                          mac[6],
                          mac[7]);
          return false;
        }
      }
      m_nas_mm_logger.info("Integrity check ok. Local: count=%d, Received: count=%d", estimated_count, pdu->msg[10]);
      sec_ctx.ul_nas_count = estimated_count;
    }

    return true;
  }

  void nas_mm::gen_rand(uint8_t rand_[16])
  {
    for (int i = 0; i < 16; i++)
    {
      rand_[i] = rand() % 256; // Pulls on byte at a time. It's slow, but does not depend on RAND_MAX.
    }
    return;
  }

  bool nas_mm::handle_service_request(service_request_t &msg, uint16_t enb_ue_id)
  {
    SERVICE_MSG_INFO ServiceMsgInfo;

    srsran::console("Received NAS message -- Service Request\n");
    m_nas_mm_logger.info("Received NAS message -- Service Request");

    // 这里不用使用rnti进行判断，要用tmsi查找上下文

    srsran::console("ngKSI: %d\n", msg.ng_ksi.nas_key_set_identifier);
    srsran::console("service_type: %d\n", msg.service_type.service_type_value);
    srsran::console("tmsi_5g: %ld\n", msg.s_tmsi_5g.s_tmsi_5g().tmsi_5g); // uint32_t: 3258209912  十进制：c2345678   码流顺序：c2 34 56 78

    nas_context *nas_ctx = m_cnw->find_nas_ctx_from_tmsi_5g(msg.s_tmsi_5g.s_tmsi_5g().tmsi_5g);
    if (nas_ctx == nullptr)
    {
      srsran::console("Received Service Request, but could not find UE NAS context by tmsi.\n");
      m_nas_mm_logger.warning("Received Service Request, but could not find UE NAS context. tmsi_5g id: %d", msg.s_tmsi_5g.s_tmsi_5g().tmsi_5g);
      return false;
    }
    else
    {
      srsran::console("Received Service Request , find UE NAS context by tmsi.\n");
    }
    nas_ctx->nrcm_ctx.rnti = enb_ue_id; // update rnti;

    m_cnw->add_nas_ctx_to_rnti_map(nas_ctx);

    /*check by m_tmsi*/
    // uint8_t* m_tmsi = nullptr;
    // struct_mobile_identity_to_array(&m_tmsi, &msg.s_tmsi_5g);
    // if (!m_cnw->check_guti_from_m_tmsi(m_tmsi, nas_ctx)) {
    //   /* send service reject */
    //   // ServiceMsgInfo.MMCause = 3;
    //   // handle_service_reject_ttcn(ServiceMsgInfo);

    //   return true;
    // }

    if (msg.uplink_data_status_present)
    {
      srsran::console("Received NAS message -- Service Request -- uplink_data_status IE is present\n");
      m_nas_mm_logger.info("Received NAS message -- Service Request -- uplink_data_status IE is present");
      /*If the Uplink data status IE is included in the SERVICE REQUEST message and the UE is:
  a) not in NB-N1 mode; or
  b) in NB-N1 mode and the UE does not indicate a request to have user-plane resources established for a number ofPDU
  sessions that exceeds the UE's maximum number of supported user-plane resources; the AMF shall: a) indicate the SMF to
  re-establish the user-plane resources for the corresponding PDU sessions; b) include the PDU session reactivation result
  IE in the SERVICE ACCEPT message to indicate the user-planeresources re-establishment result of the PDU sessions for
  which the UE requested to re-establish the user-planeresources; */

      /*TODO:re-establish the user-plane resources for the corresponding PDU sessions*/

      /*TODO:set flag PDU session reactivation result IE in the SERVICE ACCEPT message*/
    }

    if (msg.pdu_session_status_present)
    {
      srsran::console("Received NAS message -- Service Request -- pdu_session_status IE is present\n");
      m_nas_mm_logger.info("Received NAS message -- Service Request -- pdu_session_status IE is present");
      /*If the PDU session status information element is included in the SERVICE REQUEST message, then:
  a) for single access PDU sessions, the AMF shall:
  1) perform a local release of all those PDU sessions which are not in 5GSM state PDU SESSION INACTIVE on the AMF side
  associated with the access type the SERVICE REQUEST message is sent over, but are indicated by the UE as being in 5GSM
  state PDU SESSION INACTIVE;*/

      /*TODO:check pdu session state in pdu_session_status IE, local release pdu that are in 5GSM state PDU SESSION
       * INACTIVE*/

      /*TODO:set flag pdu_session_status_present is true in SERVICE ACCEPT*/
    }

    if (msg.allowed_pdu_session_status_present)
    {
      srsran::console("Received NAS message -- Service Request -- allowed_pdu_session_status IE is present\n");
      m_nas_mm_logger.info("Received NAS message -- Service Request -- allowed_pdu_session_status IE is present");
      /*for non-3gpp access*/
    }

    if (msg.nas_message_container_present)
    {
      /*If the SERVICE REQUEST message includes a NAS message container IE, the AMF shall process the SERVICE
  REQUEST message that is obtained from the NAS message container IE as described in subclause 4.4.6.*/
      /*TODO:protect check*/
    }

    /*pack_service_accept*/
    srsran::unique_byte_buffer_t nas_tx;
    nas_tx = srsran::make_byte_buffer();
    if (nas_tx == nullptr)
    {
      m_nas_mm_logger.error("Couldn't allocate PDU in %s().", __FUNCTION__);
      return false;
    }

    asn1::s1ap::init_ctxt_setup_req_s ctx_setup_req;

    memcpy(ctx_setup_req.security_key.data(), nas_ctx->m_sec_ctx.k_enb, sizeof(nas_ctx->m_sec_ctx.k_enb));
    printf("tmsi_s_5g : %d  ul_count: %d \n", nas_ctx->nrcm_ctx.tmsi_5g, nas_ctx->m_sec_ctx.ul_nas_count);
    printf("nas_ctx->nrmm_ctx.guami : %d  dl_count: %d \n", nas_ctx->nrmm_ctx.guami, nas_ctx->m_sec_ctx.dl_nas_count);

    /* ttcn 8.22*/
    if(mm_adp->udp_.is_ue_send_service_request) {
      /* send msg to ttcn*/
      srsran::unique_byte_buffer_t is_ue_send_service_request_pdu = srsran::make_byte_buffer();
      is_ue_send_service_request_pdu->msg[0] = 0x01;
      is_ue_send_service_request_pdu->msg[1] = 0x02;
      is_ue_send_service_request_pdu->msg[2] = 0x00;
      is_ue_send_service_request_pdu->msg[3] = 0x09; //  test_id
      is_ue_send_service_request_pdu->msg[4] = 0x00; //  test_id
      is_ue_send_service_request_pdu->msg[5] = 0x00; // data_length
      is_ue_send_service_request_pdu->msg[6] = 0x05; // data_length
      is_ue_send_service_request_pdu->msg[7] = 0x46; // message_type
      is_ue_send_service_request_pdu->msg[8] = 0x02;
      is_ue_send_service_request_pdu->N_bytes = 9;
      mm_adp->udp_.send_ttcn_msg_enb(std::move(is_ue_send_service_request_pdu));
      
    }

    nas_ctx->nrmm_ctx.service_accept_msg_valid = true;
    nas_ctx->nrmm_ctx.reg_or_service_accept_msg_len = nas_tx->N_bytes;
    memcpy(nas_ctx->nrmm_ctx.reg_or_service_accept_msg, nas_tx->msg, nas_tx->N_bytes);

    srsran::console("-----------send s_setup_ue_ctxt-----------------.\n");
    send_setup_ue_ctxt(enb_ue_id, nas_ctx->m_sec_ctx.k_enb);

    nas_ctx->m_sec_ctx.dl_nas_count++;
    return true;
  }

  bool nas_mm::handle_service_accept_ttcn(SERVICE_MSG_INFO ServiceMsgInfo)
  {
    service_accept_t msg;
    srsran::console("Send NAS message -- Service Accept\n");
    m_nas_mm_logger.info("Send NAS message -- Service Accept");

    /*PDU session status*/
    /*This IE shall be included when the network needs to indicate the PDU sessions that are associated with the access
  type that the message is sent over that are active within the network*/
    msg.pdu_session_status_present = true;
    /*TODO:set pdu session status from ctx*/

    /*PDU session reactivation result*/
    /*This IE shall be included:
  - if the Uplink data status IE is included in the SERVICE REQUEST message;
  - if the Allowed PDU session status IE is included in the SERVICE REQUEST message and there is at least one
  PDU session indicated in the Allowed PDU session status IE for which user-plane resources can be reestablished over 3GPP
  access.*/

    /*PDU session reactivation result error cause*/
    /*This IE may be included if the PDU session reactivation result IE is included and there exist one or more PDU
  sessions for which the user-plane resources cannot be re-established, to indicate the cause of failure to re-establish
  the user-plane resources.*/

    /*EAP message*/
    /*EAP message IE is included if the SERVICE ACCEPT message is sent to a UE registered for emergency services and
  is used to convey EAP-failure message.*/

    srsran::unique_byte_buffer_t nas_tx;
    nas_tx = srsran::make_byte_buffer();
    if (nas_tx == nullptr)
    {
      m_nas_mm_logger.error("Couldn't allocate PDU in %s().", __FUNCTION__);
      return false;
    }
    // this->nas_mm::pack_service_accept(nas_tx, &msg);

    /*TODO: send message to RRC message queue. */

    return true;
  }

  bool nas_mm::handle_ul_nas_transport(ul_nas_transport_t &msg, uint16_t enb_ue_id)
  {
    nas_context *nas_ctx = m_cnw->find_nas_ctx_from_rnti(enb_ue_id);
    srsran::unique_byte_buffer_t nas_tx = srsran::make_byte_buffer();
    if (nas_tx == nullptr)
    {
      m_nas_mm_logger.error("Couldn't allocate PDU in %s().", __FUNCTION__);
      return false;
    }
    /*save payload_container_contents in nas_tx->msg*/
    nas_tx->N_bytes = msg.payload_container.payload_container_contents.size();
    for (uint32_t i = 0; i < nas_tx->N_bytes; ++i)
    {
      nas_tx->msg[i] = msg.payload_container.payload_container_contents[i];
    }
    /*Payload container type*/
    srsran::console("ul nas transport -- payload container type is: %s\n",
                    msg.payload_container_type.payload_container_type.to_string());
    m_nas_mm_logger.info("ul nas transport -- payload container type is: %s\n",
                         msg.payload_container_type.payload_container_type.to_string());
    switch (msg.payload_container_type.payload_container_type)
    {
    /*Payload container*/
    case payload_container_type_t::Payload_container_type_type::options::n1_sm_information:
      /* code */
      // test_printf();
      srsran::console("-------mm--send_sm message--------\n");
      srsran::console("-------msg.pdu_session_id: %d --------\n", msg.pdu_session_id);
      srsran::console("-------msg.request_type: %d --------\n", msg.request_type);
      srsran::console("--------payload_container_contents:-------\n");
      for (uint32_t i = 0; i < nas_tx->N_bytes; ++i)
      {
        srsran::console("  0x%x\n", nas_tx->msg[i]);
      }
      m_nas_sm->handle_pdu_session(std::move(nas_tx), nas_ctx, enb_ue_id);
      // m_nas_sm->handle_pdu_session(std::move(nas_sm_msg), std::move(nas_tx) , nas_ctx);
      break;

    default:
      break;
    } // 12.9

    return true;
  }

  bool nas_mm::handle_nas_reg_req_ttcn(uint64_t supi, uint16_t enb_id, uint16_t cnw_ue_id, registration_type_5gs_t::registration_type_type reg_type, registration_request_t &msg)
  {
    std::cout << "reg receive111111111" << std::endl;
    static const uint32_t release_delay_mm = 0;
    uint8_t gmm_cause;
    srsran::unique_byte_buffer_t Nas_reg_req = srsran::make_byte_buffer();
    // send Nas_req_re message
    Nas_reg_req->msg[0] = 0x01;
    Nas_reg_req->msg[1] = 0x02;
    Nas_reg_req->msg[2] = 0x00;
    Nas_reg_req->msg[3] = 0x00;
    Nas_reg_req->msg[4] = 0x00;
    Nas_reg_req->msg[5] = 0x00;
    Nas_reg_req->msg[6] = 0x04; // data总长度
    Nas_reg_req->msg[7] = 0x41; // 注册请求消息
    Nas_reg_req->msg[8] = static_cast<int>(reg_type.value) << 4;
    Nas_reg_req->msg[9] = 0x01;
    Nas_reg_req->N_bytes = 10;

    mm_adp->udp_.send_ttcn_msg_enb(std::move(Nas_reg_req));

    srsran::unique_byte_buffer_t udp = srsran::make_byte_buffer();
    udp->init();

    if (mm_adp->udp_.is_reg_req_congestion || mm_adp->udp_.is_reg_rej_illegal_ue || mm_adp->udp_.is_reg_rej_plmn_not_allowed || 
        mm_adp->udp_.is_registration_reject_Ue_attempt_five_count || mm_adp->udp_.is_reg_rej_tracking_area_not_allowed)
    {
      if (is_congestion_first == true)
      {
        while (true)
        {
          if (mm_adp->udp_.nas_mm_receive_info.size() != 0)
          {
            mm_adp->udp_.nas_mm_receive_info.try_pop(udp);

            std::cout << "receive  info from TTCN" << std::endl;
            break;
          }
        }
        is_congestion_first = false;
        std::cout << "zhj test 11111111" << std::endl;
        srsran::unique_byte_buffer_t reg_rej_info = srsran::make_byte_buffer();

        memcpy(reg_rej_info->msg, udp->msg + 7, udp->N_bytes - 7);
        reg_rej_info->N_bytes = udp->N_bytes - 7;
        std::cout << "------------------------------------------5" << std::endl;
        for (uint32_t i = 0; i < reg_rej_info->N_bytes; i++)
        {
          printf("0x%x\n", reg_rej_info->msg[i]);
        }
        gmm_cause = reg_rej_info->msg[reg_rej_info->N_bytes - 1];

        if (reg_rej_info->msg[0] == 0x35)
        {
          if (gmm_cause == 0x16 || gmm_cause == 0x03|| gmm_cause == 0x0b || gmm_cause == 0x5f || gmm_cause == 0x0c || gmm_cause == 0x0f)
          {

            std::cout << "--------------------------pack_registration_reject------------------" << std::endl;
            this->pack_registration_reject(reg_rej_info, gmm_cause);

            srsran::unique_byte_buffer_t Nas_reg_req1 = srsran::make_byte_buffer();

            std::cout << "---------------------------7777777777777777777777777-------------2" << std::endl;
            
            send_mm_dl_msg(cnw_ue_id, std::move(reg_rej_info));
            // 调用RRC连接释放函数

            auto start_time = std::chrono::high_resolution_clock::now();
            while (true)
            {
              auto now_time = std::chrono::high_resolution_clock::now();
              auto duration = std::chrono::duration_cast<std::chrono::seconds>(now_time - start_time);
              if (duration.count() > 1.0)
              {
                break;
              }
            }

            send_start_rem_user(cnw_ue_id);

          }
        }
        else
        {
          // 说明ttcn发送过来的是鉴权消息,处理鉴权消息
          srsran::unique_byte_buffer_t nas_tx = srsran::make_byte_buffer();
          // 接下来把收到的鉴权消息赋值给nas_context;
          // 暂未处理，先通过处理注册请求消息下发鉴权请求

          this->handle_suci_registration_request_unknown_ue(supi, enb_id, cnw_ue_id, reg_type, msg);
          send_mm_dl_msg(cnw_ue_id, std::move(nas_tx));
        }
      }
      else
      {
        std::cout << "zhj test 222222" << std::endl;
        this->handle_suci_registration_request_unknown_ue(supi, enb_id, cnw_ue_id, reg_type, msg);
      }
    }

    //  security--mode-command2----------
    if (mm_adp->udp_.is_security_mode_command2 || mm_adp->udp_.is_authencation_reject_error_res)
    {
      // security_mode_command
      while (true)
      {
        if (mm_adp->udp_.nas_mm_receive_info.size() != 0)
        {
          mm_adp->udp_.nas_mm_receive_info.try_pop(udp);

          std::cout << "receive  info from TTCN" << std::endl;
          break;
          ;
        }
      }
      this->handle_suci_registration_request_unknown_ue(supi, enb_id, cnw_ue_id, reg_type, msg);
    }
    // registration_success_no_5g_guti;
    if (mm_adp->udp_.is_reg_succecc_no_5gguti)
    {
      std::cout << "test_ttcn_801" << std::endl;
      while (true)
      {
        if (mm_adp->udp_.nas_mm_receive_info.size() != 0)
        {
          mm_adp->udp_.nas_mm_receive_info.try_pop(udp);

          std::cout << "receive  info from TTCN" << std::endl;
          break;
          ;
        }
      }
      srsran::unique_byte_buffer_t nas_tx = srsran::make_byte_buffer();
      memcpy(nas_tx->msg, udp->msg + 7, udp->N_bytes - 7);
      nas_tx->N_bytes = udp->N_bytes - 7;
      gmm_cause = nas_tx->msg[nas_tx->N_bytes - 1];
      if (gmm_cause == 0x03)
      {
        this->pack_registration_reject(nas_tx, gmm_cause);
        send_mm_dl_msg(cnw_ue_id, std::move(nas_tx));
        send_test_reg_ss_no5Gguti(cnw_ue_id);
      }
      else
      {
        std::cout << "test_ttcn_801_handle_suci_registration_request_unknown_ue" << std::endl;
        this->handle_suci_registration_request_unknown_ue(supi, enb_id, cnw_ue_id, reg_type, msg);
      }
    }

    if (mm_adp->udp_.is_net_org_dereg_req_need_regreq_repeat || mm_adp->udp_.is_periodic_registration_request || mm_adp->udp_.is_authencation_faileure_repeated_ngksi)
    {
      std::cout << "sssssssssssssssssssssssssssssssssssssss-----------------1111111111111111" << std::endl;
      nas_context *nas_ctx = m_cnw->find_nas_ctx_from_rnti(cnw_ue_id);
      this->handle_guti_registration_request_known_ue(nas_ctx, enb_id, cnw_ue_id, reg_type, msg);
    }
    
    if(mm_adp->udp_.is_authencation_faileure_mac_code)
    {
      this->handle_suci_registration_request_unknown_ue(supi, enb_id, cnw_ue_id, reg_type, msg);
    }

    if(mm_adp->udp_.is_nas_identity_request || mm_adp->udp_.is_authentication_reject) 
    {
      std::cout << "test_ttcn_821" << std::endl;
      while (true)
      {
        if (mm_adp->udp_.nas_mm_receive_info.size() != 0)
        {
          mm_adp->udp_.nas_mm_receive_info.try_pop(udp);
          break;
        }
      }
      this->handle_suci_registration_request_unknown_ue(supi, enb_id, cnw_ue_id, reg_type, msg);

    }
    
    return true;
  }

  bool nas_mm::handle_dl_nas_transport_ttcn(uint16_t enb_ue_id)
  {
    return true;
  }

  bool nas_mm::pack_deregistration_accept_to_ue(srsran::unique_byte_buffer_t &nas_buffer)
  {
    m_nas_mm_logger.info("Packing Deregistration accept");

    nas_5gs_msg nas_msg;
    deregistration_accept_ue_terminated_t &Dereg_acc_ue_ter = nas_msg.set_deregistration_accept_ue_terminated();

    SRSASN_CODE err = nas_msg.pack(nas_buffer);
    if (err != SRSASN_SUCCESS)
    {
      m_nas_mm_logger.error("Error packing Deregistration accept");
      srsran::console("Error packing Deregistration accept\n");
      return false;
    }

    //PCAP
    if(m_cnw->get_cnw_args()->pcap_net.enable) {  
      if(nas_msg.hdr.security_header_type == nas_5gs_hdr::security_header_type_opts::plain_5gs_nas_message) {
        m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg, nas_buffer->N_bytes);
      }
      else {
        if(nas_msg.hdr.spare_1_2_version == 0x00) {
          m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg + 7, nas_buffer->N_bytes - 7);
        }
        else {
          m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg + 11, nas_buffer->N_bytes - 11);
        }
      } 
    } 

    return true;
  }

  bool nas_mm::handle_service_reject_ttcn(SERVICE_MSG_INFO ServiceMsgInfo)
  {
    service_reject_t msg;
    cause_5gmm_t cause_5gmm;

    nas_mm::common_5g_mm_cause_set_value(ServiceMsgInfo.MMCause, cause_5gmm);
    msg.cause_5gmm = cause_5gmm;

    /*PDU session status*/
    /*This IE shall be included when the network needs to indicate the PDU sessions that are associated with the access
    type that the message is sent over, that are active within the network*/
    msg.pdu_session_status_present = true;

    /*EAP message*/
    /*EAP message IE is included if the SERVICE REJECT message is used to convey EAP-failure message.*/

    srsran::unique_byte_buffer_t nas_tx;
    nas_tx = srsran::make_byte_buffer();
    if (nas_tx == nullptr)
    {
      m_nas_mm_logger.error("Couldn't allocate PDU in %s().", __FUNCTION__);
      return false;
    }
    // this->nas_mm::pack_service_reject(nas_tx, &msg);

    /*TODO: send message to RRC message queue. */

    return true;
  }

  // bool nas_mm::pack_service_accept(srsran::unique_byte_buffer_t& nas_buffer, srsran::nas_5g::service_accept_t* msg)
  // {
  //   m_nas_mm_logger.info("Packing Service Accept");

  //   nas_5gs_msg       nas_msg;
  //   service_accept_t& Service_Accept = nas_msg.set_service_accept();

  //   Service_Accept.pdu_session_status_present              = msg->pdu_session_status_present;
  //   Service_Accept.pdu_session_reactivation_result_present = msg->pdu_session_reactivation_result_present;
  //   Service_Accept.pdu_session_reactivation_result_error_cause_present =
  //       msg->pdu_session_reactivation_result_error_cause_present;
  //   Service_Accept.eap_message_present = msg->eap_message_present;

  //   Service_Accept.pdu_session_status                          = msg->pdu_session_status;
  //   Service_Accept.pdu_session_reactivation_result             = msg->pdu_session_reactivation_result;
  //   Service_Accept.pdu_session_reactivation_result_error_cause = msg->pdu_session_reactivation_result_error_cause;

  //   SRSASN_CODE err = nas_msg.pack(nas_buffer);
  //   if (err != SRSASN_SUCCESS) {
  //     m_nas_mm_logger.error("Error packing Service Accept");
  //     srsran::console("Error packing Service Accept\n");
  //     return false;
  //   }
  //   return true;
  // }


  bool nas_mm::pack_service_accept(srsran::unique_byte_buffer_t &nas_buffer, nas_context *nas_ctx)
  {
    srsran::console("----MM---pack_service_accept-------\n");
    nas_5gs_msg nas_msg;

    nrmm_ctx_t *nrmm_ctx = &nas_ctx->nrmm_ctx;
    nrcm_ctx_t *nrcm_ctx = &nas_ctx->nrcm_ctx;
    sec_ctx_t *sec_ctx = &nas_ctx->m_sec_ctx;
    nrsm_ctx_t *nrsm_ctx = &nas_ctx->nrsm_ctx;

    service_accept_t &msg = nas_msg.set_service_accept();

    /*header setter*/
    nas_msg.hdr.sequence_number = sec_ctx->dl_nas_count;
    nas_msg.hdr.security_header_type = nas_5gs_hdr::integrity_protected_and_ciphered;
    nas_msg.hdr.spare_1_2_version = nas_ctx->m_sec_ctx.spare_1_2_version;

    /*message IE pack*/

    /*message pack*/
    SRSASN_CODE err = nas_msg.pack(nas_buffer);
    if (err != SRSASN_SUCCESS)
    {
      srsran::console("Error packing pack_service_accept\n");
      return false;
    }

    /* ttcn 8.22 wait ttcn send service*/
    if(mm_adp->udp_.is_ue_send_service_request) {  
      srsran::unique_byte_buffer_t udp=srsran::make_byte_buffer();
      udp->init();
      while(true) {
          if(mm_adp->udp_.nas_mm_receive_info.size()!=0)
          {
            mm_adp->udp_.nas_mm_receive_info.try_pop(udp);
            /* judge ttcn msg???*/
            break;
          }
      }
    } 

    //PCAP
    if(m_cnw->get_cnw_args()->pcap_net.enable) {  
      if(nas_msg.hdr.security_header_type == nas_5gs_hdr::security_header_type_opts::plain_5gs_nas_message) {
        m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg, nas_buffer->N_bytes);
      }
      else {
        if(nas_msg.hdr.spare_1_2_version == 0x00) {
          m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg + 7, nas_buffer->N_bytes - 7);
        }
        else {
          m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg + 11, nas_buffer->N_bytes - 11);
        }
      } 
    } 

    printf("  nas_buffer= ");
    for (uint32_t i = 0; i < nas_buffer->N_bytes; ++i)
    {
      printf(" %x", nas_buffer->msg[i]);
    }
    printf("\n");
    // Encrypt NAS message
    cipher_encrypt(nas_buffer, nas_ctx);

    // Integrity protect NAS message
    if (nas_ctx->m_sec_ctx.spare_1_2_version == 0)
    {
      integrity_generate(&(sec_ctx->k_nas_int)[16], // key_128  count  direction  msg  msg_len  mac nas_ctx
                         sec_ctx->dl_nas_count,
                         SECURITY_DIRECTION_DOWNLINK,
                         &nas_buffer->msg[SEQ_5G_OFFSET],
                         nas_buffer->N_bytes - SEQ_5G_OFFSET,
                         &nas_buffer->msg[MAC_5G_OFFSET],
                         nas_ctx);
    }
    else
    {
      integrity_generate(&(sec_ctx->k_nas_int)[16], // key_128  count  direction  msg  msg_len  mac nas_ctx
                         sec_ctx->dl_nas_count,
                         SECURITY_DIRECTION_DOWNLINK,
                         &nas_buffer->msg[SEQ_5G_OFFSET + 4],
                         nas_buffer->N_bytes - SEQ_5G_OFFSET - 4,
                         &nas_buffer->msg[MAC_5G_OFFSET + 4],
                         nas_ctx);
    }

    return true;
  }

  // bool nas_mm::pack_service_reject(srsran::unique_byte_buffer_t& nas_buffer, srsran::nas_5g::service_reject_t* msg)
  // {
  //   m_nas_mm_logger.info("Packing Service Reject");

  //   nas_5gs_msg       nas_msg;
  //   service_reject_t& Service_Reject = nas_msg.set_service_reject();

  //   Service_Reject.cause_5gmm                 = msg->cause_5gmm;
  //   Service_Reject.pdu_session_status_present = msg->pdu_session_status_present;
  //   Service_Reject.eap_message_present        = msg->eap_message_present;
  //   Service_Reject.pdu_session_status         = msg->pdu_session_status;
  //   Service_Reject.eap_message                = msg->eap_message;

  //   SRSASN_CODE err = nas_msg.pack(nas_buffer);
  //   if (err != SRSASN_SUCCESS) {
  //     m_nas_mm_logger.error("Error packing Service Reject");
  //     srsran::console("Error packing Service Reject\n");
  //     return false;
  //   }
  //   return true;
  // }
  void nas_mm::common_5g_mm_cause_set_value(uint8_t MMCause, cause_5gmm_t &cause_5gmm)
  {
    switch (MMCause)
    {
    case 0b00000011:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::illegal_ue;
      break;

    case 0b00000101:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::pei_not_accepted;
      break;

    case 0b00000110:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::illegal_me;
      break;

    case 0b00000111:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::services_not_allowed_5gs;
      break;

    case 0b00001001:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::ue_identity_cannot_be_derived_by_the_network;
      break;

    case 0b00001010:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::implicitly_de_registered;
      break;

    case 0b00001011:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::plmn_not_allowed;
      break;

    case 0b00001100:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::tracking_area_not_allowed;
      break;

    case 0b00001101:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::roaming_not_allowed_in_this_tracking_area;
      break;

    case 0b00001111:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::no_suitable_cells_in_tracking_area;
      break;

    case 0b00010100:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::mac_failure;
      break;

    case 0b00010101:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::synch_failure;
      break;

    case 0b00010110:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::congestion;
      break;

    case 0b00010111:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::ue_security_capabilities_mismatch;
      break;

    case 0b00011000:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::security_mode_rejected_unspecified;
      break;

    case 0b00011010:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::non_5g_authentication_unacceptable;
      break;

    case 0b00011100:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::restricted_service_area;
      break;

    case 0b00011111:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::redirection_to_epc_required;
      break;

    case 0b00101011:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::ladn_not_available;
      break;

    case 0b00111110:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::no_network_slices_available;
      break;

    case 0b01000001:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::maximum_number_of_pdu_sessions_reached_;
      break;

    case 0b01000011:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::insufficient_resources_for_specific_slice_and_dnn;
      break;

    case 0b01000101:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::insufficient_resources_for_specific_slice;
      break;

    case 0b01000111:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::ng_ksi_already_in_use;
      break;

    case 0b01001000:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::non_3_gpp_access_to_5gcn_not_allowed;
      break;

    case 0b01001001:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::serving_network_not_authorized;
      break;

    case 0b01001010:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::temporarily_not_authorized_for_this_snpn;
      break;

    case 0b01001011:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::permanently_not_authorized_for_this_snpn;
      break;

    case 0b01001100:
      cause_5gmm.cause_5gmm =
          cause_5gmm_t::cause_5gmm_type::not_authorized_for_this_cag_or_authorized_for_cag_cells_only;
      break;

    case 0b01001101:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::wireline_access_area_not_allowed;
      break;

    case 0b01011010:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::payload_was_not_forwarded;
      break;

    case 0b01011011:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::dnn_not_supported_or_not_subscribed_in_the_slice;
      break;

    case 0b01011100:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::insufficient_user_plane_resources_for_the_pdu_session;
      break;

    case 0b01011111:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::semantically_incorrect_message;
      break;

    case 0b01100000:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::invalid_mandatory_information;
      break;

    case 0b01100001:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::message_type_non_existent_or_not_implemented;
      break;

    case 0b01100010:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::message_type_not_compatible_with_the_protocol_state;
      break;

    case 0b01100011:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::information_element_non_existent_or_not_implemented;
      break;

    case 0b01100100:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::conditional_ie_error;
      break;

    case 0b01100101:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::message_not_compatible_with_the_protocol_state;
      break;

    case 0b01101111:
      cause_5gmm.cause_5gmm = cause_5gmm_t::cause_5gmm_type::protocol_error_unspecified;
      break;

    default:
      /*print log*/
      break;
    }

    return;
  }

  void nas_mm::integrity_generate(uint8_t *key_128,
                                  uint32_t count,
                                  uint8_t direction,
                                  uint8_t *msg,
                                  uint32_t msg_len,
                                  uint8_t *mac,
                                  nas_context *nas_ctx)
  {
    sec_ctx_t m_sec_ctx = nas_ctx->m_sec_ctx;
    // uint32_t  bearer_id = 0;
    uint32_t bearer_id = 0x01;
    switch (m_sec_ctx.integ_algo)
    {
    case INTEGRITY_ALGORITHM_ID_EIA0:
      break;
    case INTEGRITY_ALGORITHM_ID_128_EIA1:
      std::cout << "-----integrity_generate---security_128_eia1-------" << std::endl;
      security_128_eia1(key_128, count, bearer_id, direction, msg, msg_len, mac);
      break;
    case INTEGRITY_ALGORITHM_ID_128_EIA2:
      security_128_eia2(key_128, count, bearer_id, direction, msg, msg_len, mac);
      break;
    case INTEGRITY_ALGORITHM_ID_128_EIA3:
      security_128_eia3(key_128, count, bearer_id, direction, msg, msg_len, mac);
      break;
    default:
      break;
    }
    m_nas_mm_logger.debug("Generating MAC with inputs: Algorithm %s, DL COUNT %d",
                          srsran::integrity_algorithm_id_text[m_sec_ctx.integ_algo],
                          m_sec_ctx.dl_nas_count);
  }

  void nas_mm::cipher_encrypt(srsran::unique_byte_buffer_t &pdu, nas_context *nas_ctx)
  {
    byte_buffer_t pdu_tmp;
    sec_ctx_t m_sec_ctx = nas_ctx->m_sec_ctx;
    uint32_t bearer_id = 1;

    if (m_sec_ctx.cipher_algo != CIPHERING_ALGORITHM_ID_EEA0)
    {
      m_nas_mm_logger.debug("Encrypting PDU. count=%d", m_sec_ctx.dl_nas_count);
    }

    switch (m_sec_ctx.cipher_algo)
    {
    case CIPHERING_ALGORITHM_ID_EEA0:
      break;
    case CIPHERING_ALGORITHM_ID_128_EEA1:
      if (nas_ctx->m_sec_ctx.spare_1_2_version == 0)
      {
        security_128_eea1(&(m_sec_ctx.k_nas_enc)[16],
                          m_sec_ctx.dl_nas_count,
                          NAS_5G_BEARER,
                          SECURITY_DIRECTION_DOWNLINK,
                          &pdu->msg[SEQ_5G_OFFSET + 1],
                          pdu->N_bytes - SEQ_5G_OFFSET - 1,
                          &pdu_tmp.msg[SEQ_5G_OFFSET + 1]);
        memcpy(&pdu->msg[SEQ_5G_OFFSET + 1], &pdu_tmp.msg[SEQ_5G_OFFSET + 1], pdu->N_bytes - SEQ_5G_OFFSET - 1);
      }
      else
      {
        security_128_eea1(&(m_sec_ctx.k_nas_enc)[16],
                          m_sec_ctx.dl_nas_count,
                          NAS_5G_BEARER,
                          SECURITY_DIRECTION_DOWNLINK,
                          &pdu->msg[SEQ_5G_OFFSET + 5],
                          pdu->N_bytes - SEQ_5G_OFFSET - 5,
                          &pdu_tmp.msg[SEQ_5G_OFFSET + 5]);
        memcpy(&pdu->msg[SEQ_5G_OFFSET + 5], &pdu_tmp.msg[SEQ_5G_OFFSET + 5], pdu->N_bytes - SEQ_5G_OFFSET - 5);
      }
      break;
    case CIPHERING_ALGORITHM_ID_128_EEA2:
      if (nas_ctx->m_sec_ctx.spare_1_2_version == 0)
      {
        security_128_eea2(&(m_sec_ctx.k_nas_enc)[16],
                          m_sec_ctx.dl_nas_count,
                          NAS_5G_BEARER,
                          SECURITY_DIRECTION_DOWNLINK,
                          &pdu->msg[SEQ_5G_OFFSET + 1],
                          pdu->N_bytes - SEQ_5G_OFFSET - 1,
                          &pdu_tmp.msg[SEQ_5G_OFFSET + 1]);
        memcpy(&pdu->msg[SEQ_5G_OFFSET + 1], &pdu_tmp.msg[SEQ_5G_OFFSET + 1], pdu->N_bytes - SEQ_5G_OFFSET - 1);
      }
      else
      {
        security_128_eea2(&(m_sec_ctx.k_nas_enc)[16],
                          m_sec_ctx.dl_nas_count,
                          NAS_5G_BEARER,
                          SECURITY_DIRECTION_DOWNLINK,
                          &pdu->msg[SEQ_5G_OFFSET + 5],
                          pdu->N_bytes - SEQ_5G_OFFSET - 5,
                          &pdu_tmp.msg[SEQ_5G_OFFSET + 5]);
        memcpy(&pdu->msg[SEQ_5G_OFFSET + 5], &pdu_tmp.msg[SEQ_5G_OFFSET + 5], pdu->N_bytes - SEQ_5G_OFFSET - 5);
      }
      break;
    case CIPHERING_ALGORITHM_ID_128_EEA3:
      if (nas_ctx->m_sec_ctx.spare_1_2_version == 0)
      {
        security_128_eea3(&(m_sec_ctx.k_nas_enc)[16],
                          m_sec_ctx.dl_nas_count,
                          NAS_5G_BEARER,
                          SECURITY_DIRECTION_DOWNLINK,
                          &pdu->msg[SEQ_5G_OFFSET + 1],
                          pdu->N_bytes - SEQ_5G_OFFSET - 1,
                          &pdu_tmp.msg[SEQ_5G_OFFSET + 1]);
        memcpy(&pdu->msg[SEQ_5G_OFFSET + 1], &pdu_tmp.msg[SEQ_5G_OFFSET + 1], pdu->N_bytes - SEQ_5G_OFFSET - 1);
      }
      else
      {
        security_128_eea3(&(m_sec_ctx.k_nas_enc)[16],
                          m_sec_ctx.dl_nas_count,
                          NAS_5G_BEARER,
                          SECURITY_DIRECTION_DOWNLINK,
                          &pdu->msg[SEQ_5G_OFFSET + 5],
                          pdu->N_bytes - SEQ_5G_OFFSET - 5,
                          &pdu_tmp.msg[SEQ_5G_OFFSET + 5]);
        memcpy(&pdu->msg[SEQ_5G_OFFSET + 5], &pdu_tmp.msg[SEQ_5G_OFFSET + 5], pdu->N_bytes - SEQ_5G_OFFSET - 5);
      }
      break;
    default:
      m_nas_mm_logger.error("Ciphering algorithm not known");
      break;
    }
  }

  void nas_mm::cipher_decrypt(srsran::byte_buffer_t *pdu, nas_context *nas_ctx)
  {
    srsran::byte_buffer_t tmp_pdu;
    sec_ctx_t m_sec_ctx = nas_ctx->m_sec_ctx;
    uint32_t bearer_id = 1;

    switch (m_sec_ctx.cipher_algo)
    {
    case srsran::CIPHERING_ALGORITHM_ID_EEA0:
      std::cout << "-------------cipher_decrypt--CIPHERING_ALGORITHM_ID_EEA0--------" << std::endl;
      break;
    case srsran::CIPHERING_ALGORITHM_ID_128_EEA1:
      if (nas_ctx->m_sec_ctx.spare_1_2_version == 0)
      {
        srsran::security_128_eea1(&m_sec_ctx.k_nas_enc[16],
                                  pdu->msg[SEQ_5G_OFFSET],
                                  NAS_5G_BEARER,
                                  srsran::SECURITY_DIRECTION_UPLINK,
                                  &pdu->msg[SEQ_5G_OFFSET + 1],
                                  pdu->N_bytes - SEQ_5G_OFFSET - 1,
                                  &tmp_pdu.msg[SEQ_5G_OFFSET + 1]);
        memcpy(&pdu->msg[SEQ_5G_OFFSET + 1], &tmp_pdu.msg[SEQ_5G_OFFSET + 1], pdu->N_bytes - SEQ_5G_OFFSET - 1);
      }
      else
      {
        srsran::security_128_eea1(&m_sec_ctx.k_nas_enc[16],
                                  pdu->msg[SEQ_5G_OFFSET + 4],
                                  NAS_5G_BEARER,
                                  srsran::SECURITY_DIRECTION_UPLINK,
                                  &pdu->msg[SEQ_5G_OFFSET + 5],
                                  pdu->N_bytes - SEQ_5G_OFFSET - 5,
                                  &tmp_pdu.msg[SEQ_5G_OFFSET + 5]);
        memcpy(&pdu->msg[SEQ_5G_OFFSET + 5], &tmp_pdu.msg[SEQ_5G_OFFSET + 5], pdu->N_bytes - SEQ_5G_OFFSET - 5);
      }
      m_nas_mm_logger.debug(tmp_pdu.msg, pdu->N_bytes, "Decrypted");
      break;
    case srsran::CIPHERING_ALGORITHM_ID_128_EEA2:
      if (nas_ctx->m_sec_ctx.spare_1_2_version == 0)
      {
        srsran::security_128_eea2(&m_sec_ctx.k_nas_enc[16],
                                  pdu->msg[SEQ_5G_OFFSET],
                                  NAS_5G_BEARER,
                                  srsran::SECURITY_DIRECTION_UPLINK,
                                  &pdu->msg[SEQ_5G_OFFSET + 1],
                                  pdu->N_bytes - SEQ_5G_OFFSET - 1,
                                  &tmp_pdu.msg[SEQ_5G_OFFSET + 1]);
        memcpy(&pdu->msg[SEQ_5G_OFFSET + 1], &tmp_pdu.msg[SEQ_5G_OFFSET + 1], pdu->N_bytes - SEQ_5G_OFFSET - 1);
      }
      else
      {
        srsran::security_128_eea2(&m_sec_ctx.k_nas_enc[16],
                                  pdu->msg[SEQ_5G_OFFSET + 4],
                                  NAS_5G_BEARER,
                                  srsran::SECURITY_DIRECTION_UPLINK,
                                  &pdu->msg[SEQ_5G_OFFSET + 5],
                                  pdu->N_bytes - SEQ_5G_OFFSET - 5,
                                  &tmp_pdu.msg[SEQ_5G_OFFSET + 5]);
        memcpy(&pdu->msg[SEQ_5G_OFFSET + 5], &tmp_pdu.msg[SEQ_5G_OFFSET + 5], pdu->N_bytes - SEQ_5G_OFFSET - 5);
      }
      m_nas_mm_logger.debug(tmp_pdu.msg, pdu->N_bytes, "Decrypted");
      break;
    case srsran::CIPHERING_ALGORITHM_ID_128_EEA3:
      if (nas_ctx->m_sec_ctx.spare_1_2_version == 0)
      {
        srsran::security_128_eea3(&m_sec_ctx.k_nas_enc[16],
                                  pdu->msg[SEQ_5G_OFFSET],
                                  NAS_5G_BEARER,
                                  srsran::SECURITY_DIRECTION_UPLINK,
                                  &pdu->msg[SEQ_5G_OFFSET + 1],
                                  pdu->N_bytes - SEQ_5G_OFFSET - 1,
                                  &tmp_pdu.msg[SEQ_5G_OFFSET + 1]);
        memcpy(&pdu->msg[SEQ_5G_OFFSET + 1], &tmp_pdu.msg[SEQ_5G_OFFSET + 1], pdu->N_bytes - SEQ_5G_OFFSET - 1);
      }
      else
      {
        srsran::security_128_eea3(&m_sec_ctx.k_nas_enc[16],
                                  pdu->msg[SEQ_5G_OFFSET + 4],
                                  NAS_5G_BEARER,
                                  srsran::SECURITY_DIRECTION_UPLINK,
                                  &pdu->msg[SEQ_5G_OFFSET + 5],
                                  pdu->N_bytes - SEQ_5G_OFFSET - 5,
                                  &tmp_pdu.msg[SEQ_5G_OFFSET + 5]);
        memcpy(&pdu->msg[SEQ_5G_OFFSET + 5], &tmp_pdu.msg[SEQ_5G_OFFSET + 5], pdu->N_bytes - SEQ_5G_OFFSET - 5);
      }
      m_nas_mm_logger.debug(tmp_pdu.msg, pdu->N_bytes, "Decrypted");
      break;
    default:
      m_nas_mm_logger.error("Ciphering algorithms not known");
      break;
    }
  }

  bool nas_mm::gen_auth_info_answer(nas_context *nas_ctx,
                                    uint64_t supi,
                                    uint8_t *k_ausf,
                                    uint8_t *k_seaf,
                                    uint8_t *k_amf,
                                    uint8_t *autn,
                                    uint8_t *rand,
                                    uint8_t *xres)
  {
    std::cout << "-------@@gen_auth_info_answer@@------------" << std::endl;
    m_nas_mm_logger.debug("Generating AUTH info answer");
    std::cout << "Generating AUTH info answer: " << supi << endl;
    ue_ctx_t *ue_ctx = m_cnw->get_ue_ctx(supi);
    if (ue_ctx == nullptr)
    {
      srsran::console("User not found at UDM. SUCI: %015" PRIu64 "\n", supi);
      m_nas_mm_logger.error("User not found at UDM. SUCI: %015" PRIu64 "", supi);
      return false;
    }

    /*创建ue上下文*/
    // ue_ctx = new ue_ctx_t;
    // uint8_t default_key[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    //                          0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    // memcpy(ue_ctx->key, default_key, sizeof(default_key));

    // uint8_t default_opc[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    //                          0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
    // memcpy(ue_ctx->opc, default_opc, sizeof(default_opc));

    // uint8_t default_amf[] = {0x80, 0x00};
    // memcpy(ue_ctx->amf, default_amf, sizeof(default_amf));

    // // uint8_t default_sqn[] = {0x00, 0x00, 0x00, 0x00, 0x03, 0xc0}; //11-24 iot
    // // memcpy(ue_ctx->sqn, default_sqn, sizeof(default_sqn));

    // uint8_t default_sqn[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    // memcpy(ue_ctx->sqn, default_sqn, sizeof(default_sqn));

    /*开始秘钥推衍*/
    //     std::cout<< "--------gen_auth_info_answer_milenage---!!!!!!!!!!1-----"<<std::endl;
    //  gen_auth_info_answer_milenage(ue_ctx, k_ausf, k_seaf, k_amf, autn, rand, xres);

    //   std::cout<< "--------gen_auth_info_answer_milenage over---!!!!!!!!!!1-----"<<std::endl;
    // for(int j=0; j<16; ++j){
    //       srsran::console("-autn[%d] =%x\n", j, autn[j]);
    // }

    switch (ue_ctx->algo)
    {
    case HSS_ALGO_XOR:
      // std::cout<< "--------HSS_ALGO_MILENAGE----!!!!!!!!!!1-----"<<std::endl;
      gen_auth_info_answer_milenage(ue_ctx, k_ausf, k_seaf, k_amf, autn, rand, xres);
      break;
    case HSS_ALGO_MILENAGE:
      // std::cout<< "--------HSS_ALGO_MILENAGE----!!!!!!!!!!1-----"<<std::endl;
      gen_auth_info_answer_milenage(ue_ctx, k_ausf, k_seaf, k_amf, autn, rand, xres);
      break;
    default:
      std::cout << "-------Unsupported ue algo----!!!!!!!!!!1-----" << std::endl;
      m_nas_mm_logger.warning("Unsupported ue algo.");
    }
    m_cnw->increment_ue_sqn(ue_ctx);
    return true;
  }

  void nas_mm::gen_auth_info_answer_milenage(ue_ctx_t *ue_ctx,
                                             uint8_t *k_ausf,
                                             uint8_t *k_seaf,
                                             uint8_t *k_amf,
                                             uint8_t *autn,
                                             uint8_t *rand,
                                             uint8_t *xres_star)
  {
    // Get K, AMF, OPC and SQN
    uint8_t *k = ue_ctx->key;
    uint8_t *amf = ue_ctx->amf;
    uint8_t *opc = ue_ctx->opc;
    uint8_t *sqn = ue_ctx->sqn;

    // Temp variables
    uint8_t xres[16]; // test

    uint8_t ck[16];
    uint8_t ik[16];
    uint8_t ak[6];
    uint8_t mac[8];
    uint16_t mcc, mnc;
    u_int8_t abba[2] = {0};
    // u_int8_t    abba[3] = {0x00,0x00,0x00};
    std::string supi = ue_ctx->supi; // supi的计算???
    // supi = "460000123456780";        // 没有配置文件，暂时写定
    cnw_args_t *cnw_args = m_cnw->get_cnw_args();

    cnw_args->mcc = 0xf460; // 没有配置文件，暂时写定
    cnw_args->mnc = 0xff00; // 没有配置文件，暂时写定

    mcc = cnw_args->mcc;
    mnc = cnw_args->mnc;

    plmn_id_t plmn_id;
    plmn_id.from_number(mcc, mnc);

    gen_rand(rand);

    // uint8_t default_rand[] = {0x23,0xc4,0xc2,0x17,0x5c,0x36,0x5a,0xc4,0x31,0x5b,0x40,0x5b,0xd7,0x64,0x6a,0x43};
    // memcpy(rand, default_rand, sizeof(default_rand));
    // for(int i=0; i<16; ++i){
    //       srsran::console("-rand[%d] =%x\n", i, rand[i]);
    // }
    srsran::security_milenage_f2345(k, opc, rand, xres, ck, ik, ak);

    m_nas_mm_logger.debug(k, 16, "User Key : ");
    m_nas_mm_logger.debug(opc, 16, "User OPc : ");
    m_nas_mm_logger.debug(rand, 16, "User Rand : ");
    m_nas_mm_logger.debug(xres, 8, "User XRES: ");
    m_nas_mm_logger.debug(ck, 16, "User CK: ");
    m_nas_mm_logger.debug(ik, 16, "User IK: ");
    m_nas_mm_logger.debug(ak, 6, "User AK: ");

    // std::cout<< "--------security_milenage_f2345 over---!!!!!!!!!!1-----"<<std::endl;
    // for(int j=0; j<16; ++j){
    //       srsran::console("-ck[%d] =%x\n", j, ck[j]);
    // }

    // std::cout<< "--------security_milenage_f2345 over---!!!!!!!!!!1-----"<<std::endl;
    // for(int j=0; j<6; ++j){
    //       srsran::console("-ak[%d] =%x\n", j, ak[j]);
    // }

    // std::cout<< "--------security_milenage_f2345 over---!!!!!!!!!!1-----"<<std::endl;
    // for(int j=0; j<16; ++j){
    //       srsran::console("-xres[%d] =%x\n", j, xres[j]);
    // }

    srsran::security_milenage_f1(k, opc, rand, sqn, amf, mac);

    m_nas_mm_logger.debug(sqn, 6, "User SQN : ");
    m_nas_mm_logger.debug(mac, 8, "User MAC : ");

    uint8_t ak_xor_sqn[6];
    for (int i = 0; i < 6; i++)
    {
      ak_xor_sqn[i] = sqn[i] ^ ak[i];
    }

    // Generate AUTN (autn = sqn ^ ak |+| amf |+| mac)
    for (int i = 0; i < 6; i++)
    {
      autn[i] = sqn[i] ^ ak[i];
    }
    for (int i = 0; i < 2; i++)
    {
      autn[6 + i] = amf[i];
    }
    for (int i = 0; i < 8; i++)
    {
      autn[8 + i] = mac[i];
    }
    m_nas_mm_logger.debug(autn, 16, "User AUTN: ");

    // std::cout<< "-----mac---------------- over---!!!!!!!!!!1-----"<<std::endl;
    // for(int j=0; j<8; ++j){
    //       srsran::console("-mac[%d] =%x\n", j, mac[j]);
    // }

    // std::cout<< "-----AUTN--------------- over---!!!!!!!!!!1-----"<<std::endl;
    // for(int j=0; j<16; ++j){
    //       srsran::console("-AUTN[%d] =%x\n", j, autn[j]);
    // }

    // Generate Xres*?
    memset(xres_star, 0, 16);
    security_generate_res_star(ck, ik, plmn_id.to_serving_network_name_string().c_str(), rand, xres, XRES_LEN, xres_star);
    // security_generate_res_star(ck, ik,"5G:mnc000.mcc460.3gppnetwork.org", rand, xres, XRES_LEN, xres_star);
    // logger.debug(res_star, 16, "RES STAR");

    std::cout << "-----security_generate_res_star over---!!!!!!!!!!1-----" << std::endl;
    // for(int j=0; j<16; ++j){
    //       srsran::console("-xres_star[%d] =%x\n", j, xres_star[j]);
    // }

    // Generate k_ausf
    srsran::security_generate_k_ausf(ck, ik, ak_xor_sqn, plmn_id.to_serving_network_name_string().c_str(), k_ausf);
    m_nas_mm_logger.debug(k_ausf, 32, "K AUSF");

    // std::cout<< "-----security_generate_res_star over---!!!!!!!!!!1-----"<<std::endl;
    // for(int j=0; j<32; ++j){
    //       srsran::console("-k_ausf[%d] =%x\n", j, k_ausf[j]);
    // }

    // Generate K_seaf
    srsran::security_generate_k_seaf(k_ausf, plmn_id.to_serving_network_name_string().c_str(), k_seaf);
    m_nas_mm_logger.debug(k_seaf, 32, "K SEAF");

    // std::cout<< "-----security_generate_res_star over---!!!!!!!!!!1-----"<<std::endl;
    // for(int j=0; j<32; ++j){
    //       srsran::console("-k_seaf[%d] =%x\n", j, k_seaf[j]);
    // }

    // Generate K_amf
    srsran::security_generate_k_amf(k_seaf, supi.c_str(), abba, ABBA_LEN, k_amf);
    m_nas_mm_logger.debug(k_amf, 32, "K AMF");

    // std::cout<< "-----security_generate_res_star over---!!!!!!!!!!1-----"<<std::endl;
    // for(int j=0; j<32; ++j){
    //       srsran::console("-k_amf[%d] =%x\n", j, k_amf[j]);
    // }

    // Set last RAND
    ue_ctx->set_last_rand(rand);
    return;
  }

  bool nas_mm::handle_identity_request_ttcn(IDENTITY_MSG_INFO msg)
  {
    identity_request_t identity_msg;

    switch (msg.IdentityType)
    {
    case NR_NAS_MM_IDENTITY_TYPE_SUCI:
      identity_msg.identity_type.type_of_identity = identity_type_5gs_t::identity_types_::options::suci;
      break;
    case NR_NAS_MM_IDENTITY_TYPE_GUTI_5G:
      identity_msg.identity_type.type_of_identity = identity_type_5gs_t::identity_types_::options::guti_5g;
      break;
    case NR_NAS_MM_IDENTITY_TYPE_IMEI:
      identity_msg.identity_type.type_of_identity = identity_type_5gs_t::identity_types_::options::imei;
      break;
    case NR_NAS_MM_IDENTITY_TYPE_S_TMSI_5G:
      identity_msg.identity_type.type_of_identity = identity_type_5gs_t::identity_types_::options::s_tmsi_5g;
      break;
    case NR_NAS_MM_IDENTITY_TYPE_IMEISV:
      identity_msg.identity_type.type_of_identity = identity_type_5gs_t::identity_types_::options::imeisv;
      break;
    case NR_NAS_MM_IDENTITY_TYPE_MAC_ADDRESS:
      identity_msg.identity_type.type_of_identity = identity_type_5gs_t::identity_types_::options::mac_address;
      break;
    case NR_NAS_MM_IDENTITY_TYPE_EUI_64:
      identity_msg.identity_type.type_of_identity = identity_type_5gs_t::identity_types_::options::eui_64;
      break;

    default:
      break;
    }

    srsran::unique_byte_buffer_t nas_tx;
    nas_tx = srsran::make_byte_buffer();
    if (nas_tx == nullptr)
    {
      m_nas_mm_logger.error("Couldn't allocate PDU in %s().", __FUNCTION__);
      return false;
    }
    this->nas_mm::pack_identity_request(nas_tx, &identity_msg);
    /*TODO: send message to RRC message queue. */

    return true;
  }

  bool nas_mm::pack_identity_request(srsran::unique_byte_buffer_t &nas_buffer, identity_request_t *msg)
  {
    m_nas_mm_logger.info("Packing Identity Request");

    nas_5gs_msg nas_msg;
    identity_request_t &Identity_Request = nas_msg.set_identity_request();

    nas_msg.hdr.security_header_type = nas_5gs_hdr::plain_5gs_nas_message;
    nas_msg.hdr.spare_1_2_version = 0x01;
    Identity_Request.identity_type = msg->identity_type;

    SRSASN_CODE err = nas_msg.pack(nas_buffer);
    if (err != SRSASN_SUCCESS)
    {
      m_nas_mm_logger.error("Error packing Service Reject");
      srsran::console("Error packing Service Reject\n");
      return false;
    }
    //PCAP
    if(m_cnw->get_cnw_args()->pcap_net.enable) {  
      if(nas_msg.hdr.security_header_type == nas_5gs_hdr::security_header_type_opts::plain_5gs_nas_message) {
        m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg, nas_buffer->N_bytes);
      }
      else {
        if(nas_msg.hdr.spare_1_2_version == 0x00) {
          m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg + 7, nas_buffer->N_bytes - 7);
        }
        else {
          m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg + 11, nas_buffer->N_bytes - 11);
        }
      } 
    } 

    return true;
  }

  bool nas_mm::handle_identity_response(identity_response_t &msg, uint16_t enb_ue_id)
  {
    
    nas_context *nas_ctx = NULL;
    mobile_identity_5gs_t::identity_types mobile_identity = msg.mobile_identity.type();
    nas_ctx = m_cnw->find_nas_ctx_from_rnti(enb_ue_id);

    switch (mobile_identity)
    {
    case mobile_identity_5gs_t::identity_types::suci:

      uint64_t suci;
      struct_mobile_identity_to_uint64(&suci, &msg.mobile_identity);

      /*change suci in nas ctx*/
      /*TODO:change map*/
      nas_ctx->nrmm_ctx.suci = suci;

      /*ttcn 8_21_identity_request_suci*/
      if (mm_adp->udp_.is_nas_identity_request) {
        /*send identity reponse to ttcn*/
        srsran::unique_byte_buffer_t Nas_identity_res = srsran::make_byte_buffer();

        Nas_identity_res->msg[0] = 0x01;
        Nas_identity_res->msg[1] = 0x02;
        Nas_identity_res->msg[2] = 0x00;
        Nas_identity_res->msg[3] = 0x00;
        Nas_identity_res->msg[4] = 0x00;
        Nas_identity_res->msg[5] = 0x00;
        Nas_identity_res->msg[6] = 0x04; // data总长度
        Nas_identity_res->msg[7] = 0x4e; // 注册请求消息
        Nas_identity_res->msg[8] = 0x01;
        Nas_identity_res->N_bytes = 9;

        mm_adp->udp_.send_ttcn_msg_enb(std::move(Nas_identity_res));

        /* 2.3 construct aka request message. */
        srsran::unique_byte_buffer_t nas_tx = srsran::make_byte_buffer();
        if (nas_tx == nullptr)
        {
          m_nas_mm_logger.error("Couldn't allocate PDU in %s().", __FUNCTION__);
          return false;
        }
        pack_authentication_request(nas_tx, nas_ctx);
        send_mm_dl_msg(enb_ue_id, std::move(nas_tx));
      }

      break;

    case mobile_identity_5gs_t::identity_types::guti_5g:
      srsepc::nas_guti guti;
      struct_mobile_identity_to_guti(&guti, &msg.mobile_identity);

      /*change guti in nas ctx*/
      /*TODO:change map*/
      nas_ctx->nrmm_ctx.guti = guti;
      break;

    default:
      m_nas_mm_logger.warning("Identity request -- Unsupported mobile identity type (%s).",
                              mobile_identity.to_string());
      break;
    }
    return true;
  }

  bool nas_mm::handle_configuration_update_command_ttcn(CONFIGURATION_UPDATE_COMMAND_MSG_INFO msg)
  {
    srsran::nas_5g::configuration_update_command_t Config_updata_cmd;

    if (true == msg.gutiFlag)
    {
      Config_updata_cmd.guti_5g_present = true;
      guti_to_mobile_identity_5gs_t(Config_updata_cmd, msg.guti);
    }

    return true;
  }

  bool nas_mm::send_configuration_update_command_5g_guti_ttcn(uint16_t rnti)
  {
    nas_context *nas_ctx = m_cnw->find_nas_ctx_from_rnti(rnti);
    if(nas_ctx == NULL) {
      m_nas_mm_logger.error("not find nas context, rnti: %d", rnti);
      return false;
    }

    nrmm_ctx_t *nrmm_ctx = &nas_ctx->nrmm_ctx;
    nrcm_ctx_t *nrcm_ctx = &nas_ctx->nrcm_ctx;
    sec_ctx_t *sec_ctx = &nas_ctx->m_sec_ctx;

    srsran::unique_byte_buffer_t nas_buffer =  srsran::make_byte_buffer();

    nas_5gs_msg nas_msg;
    
    configuration_update_command_t &Config_update_Cmd = nas_msg.set_configuration_update_command();

    nas_msg.hdr.sequence_number = sec_ctx->dl_nas_count;
    nas_msg.hdr.security_header_type = nas_5gs_hdr::integrity_protected_and_ciphered;
    nas_msg.hdr.spare_1_2_version = sec_ctx->spare_1_2_version;

    Config_update_Cmd.configuration_update_indication_present = true;
    Config_update_Cmd.guti_5g_present = true;
    Config_update_Cmd.configuration_update_indication.control_plane_service_type_value = configuration_update_indication_t::control_plane_service_type_value_type_::options::mobile_terminating_request;
    mobile_identity_5gs_t::guti_5g_s &guti_5g = Config_update_Cmd.guti_5g.set_guti_5g();

    // 直接采用默认值的赋值
    guti_5g.mcc[0] = 0x04;
    guti_5g.mcc[1] = 0x06;
    guti_5g.mcc[2] = 0x00;

    guti_5g.mnc[0] = 0x00;
    guti_5g.mnc[1] = 0x00;
    guti_5g.mnc[2] = 0x0f;

    // guti_5g.amf_pointer = 0x01;
    // guti_5g.amf_region_id = 0xfe;
    // guti_5g.amf_set_id = 0x0001;

    guti_5g.amf_pointer = 0x01;
    guti_5g.amf_region_id = 0xfe;
    guti_5g.amf_set_id = 0x0002;
    guti_5g.tmsi_5g = 0xc2345678;

    SRSASN_CODE err = nas_msg.pack(nas_buffer);
    if (err != SRSASN_SUCCESS)
    {
      m_nas_mm_logger.error("Error packing Service Reject");
      srsran::console("Error packing Service Reject\n");
      return false;
    }

    //PCAP
    if(m_cnw->get_cnw_args()->pcap_net.enable) {  
      if(nas_msg.hdr.security_header_type == nas_5gs_hdr::security_header_type_opts::plain_5gs_nas_message) {
        m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg, nas_buffer->N_bytes);
      }
      else {
        if(nas_msg.hdr.spare_1_2_version == 0x00) {
          m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg + 7, nas_buffer->N_bytes - 7);
        }
        else {
          m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg + 11, nas_buffer->N_bytes - 11);
        }
      } 
    } 

    // Encrypt NAS message
    cipher_encrypt(nas_buffer, nas_ctx);

    if (nas_ctx->m_sec_ctx.spare_1_2_version == 0)
    {
      integrity_generate(&(sec_ctx->k_nas_int)[16], // key_128  count  direction  msg  msg_len  mac nas_ctx
                         sec_ctx->dl_nas_count,
                         SECURITY_DIRECTION_DOWNLINK,
                         &nas_buffer->msg[SEQ_5G_OFFSET],
                         nas_buffer->N_bytes - SEQ_5G_OFFSET,
                         &nas_buffer->msg[MAC_5G_OFFSET],
                         nas_ctx);
    }
    else
    {
      integrity_generate(&(sec_ctx->k_nas_int)[16], // key_128  count  direction  msg  msg_len  mac nas_ctx
                         sec_ctx->dl_nas_count,
                         SECURITY_DIRECTION_DOWNLINK,
                         &nas_buffer->msg[SEQ_5G_OFFSET + 4],
                         nas_buffer->N_bytes - SEQ_5G_OFFSET - 4,
                         &nas_buffer->msg[MAC_5G_OFFSET + 4],
                         nas_ctx);
    }
    sec_ctx->dl_nas_count++;
    send_mm_dl_msg(rnti, std::move(nas_buffer));
    return true;
  }

  bool nas_mm::pack_configuration_update_command(srsran::unique_byte_buffer_t &nas_buffer,
                                                 srsran::nas_5g::configuration_update_command_t msg)
  {
    m_nas_mm_logger.info("Packing Configuration update Command");

    nas_5gs_msg nas_msg;
    configuration_update_command_t &Config_update_Cmd = nas_msg.set_configuration_update_command();
    if(msg.guti_5g_present) {
      Config_update_Cmd.guti_5g_present = msg.guti_5g_present;
      mobile_identity_5gs_t::guti_5g_s &guti_5g = Config_update_Cmd.guti_5g.set_guti_5g();

      // 直接采用默认值的赋值
      guti_5g.mcc[0] = 0x04;
      guti_5g.mcc[1] = 0x06;
      guti_5g.mcc[2] = 0x00;

      guti_5g.mnc[0] = 0x00;
      guti_5g.mnc[1] = 0x00;
      guti_5g.mnc[2] = 0x0f;

      // guti_5g.amf_pointer = 0x01;
      // guti_5g.amf_region_id = 0xfe;
      // guti_5g.amf_set_id = 0x0001;

      guti_5g.amf_pointer = 0x01;
      guti_5g.amf_region_id = 0xfe;
      guti_5g.amf_set_id = 0x0002;
      guti_5g.tmsi_5g = 0xc2345678;
    }

    SRSASN_CODE err = nas_msg.pack(nas_buffer);
    if (err != SRSASN_SUCCESS)
    {
      m_nas_mm_logger.error("Error packing Service Reject");
      srsran::console("Error packing Service Reject\n");
      return false;
    }

    //PCAP
    if(m_cnw->get_cnw_args()->pcap_net.enable) {  
      if(nas_msg.hdr.security_header_type == nas_5gs_hdr::security_header_type_opts::plain_5gs_nas_message) {
        m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg, nas_buffer->N_bytes);
      }
      else {
        if(nas_msg.hdr.spare_1_2_version == 0x00) {
          m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg + 7, nas_buffer->N_bytes - 7);
        }
        else {
          m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg + 11, nas_buffer->N_bytes - 11);
        }
      } 
    } 

    return true;
  }

  bool nas_mm::handle_configuration_update_complete(void)
  {
    /* ttcn 8_24 ue_config_update*/
    if(mm_adp->udp_.is_ue_config_update) {
      /*send configuration_update_complete to ttcn*/
      srsran::unique_byte_buffer_t Nas_config_update_complete = srsran::make_byte_buffer();

      Nas_config_update_complete->msg[0] = 0x01;
      Nas_config_update_complete->msg[1] = 0x02;
      Nas_config_update_complete->msg[2] = 0x00;
      Nas_config_update_complete->msg[3] = 0x00;
      Nas_config_update_complete->msg[4] = 0x00;
      Nas_config_update_complete->msg[5] = 0x00;
      Nas_config_update_complete->msg[6] = 0x01; // data总长度
      Nas_config_update_complete->msg[7] = 0x48; // ttcn configuration_update_complete消息
      Nas_config_update_complete->N_bytes = 8;

      mm_adp->udp_.send_ttcn_msg_enb(std::move(Nas_config_update_complete));
    }
    return true;
  }

  void nas_mm::guti_to_mobile_identity_5gs_t(srsran::nas_5g::configuration_update_command_t &nas_msg,
                                             srsepc::nas_guti guti)
  {
    m_nas_mm_logger.info("guti struct to mobile_identity_5gs_t");

    mobile_identity_5gs_t::guti_5g_s guti_5g;

    guti_5g = nas_msg.guti_5g.set_guti_5g();

    nas_msg.guti_5g.set(mobile_identity_5gs_t::identity_types::guti_5g);

    guti_5g.mcc[0] = *guti.getGutiAddress(0) & 0x0f;
    guti_5g.mcc[1] = *guti.getGutiAddress(0) & 0xf0;
    guti_5g.mcc[2] = *guti.getGutiAddress(1) & 0x0f;
    guti_5g.mnc[2] = *guti.getGutiAddress(1) & 0xf0;
    guti_5g.mnc[1] = *guti.getGutiAddress(2) & 0x0f;
    guti_5g.mnc[0] = *guti.getGutiAddress(2) & 0xf0;
    guti_5g.amf_region_id = *guti.getGutiAddress(3);
    guti_5g.amf_set_id = (((uint16_t)*guti.getGutiAddress(4)) << 2) | ((uint16_t)(*guti.getGutiAddress(5) & 0x03));
    guti_5g.amf_pointer = *guti.getGutiAddress(5) & 0x3f;
    guti_5g.tmsi_5g = common_5g_mm_array_to_uint32(guti.getGutiAddress(6));

    return;
  }

  uint32_t nas_mm::common_5g_mm_array_to_uint32(uint8_t *args)
  {
    uint32_t values = 0;

    for (uint8_t i = 0; i < 4; i++)
    {
      values += (((uint32_t)args[i]) << (i * 8));
    }

    return values;
  }

  bool nas_mm::compute_supi_from_suci(mobile_identity_5gs_t::suci_s suci, char *c_suci, char *supi)
  {
#define MAX_SUCI_TOKEN 16
    if (suci.supi_format != mobile_identity_5gs_t::suci_s::supi_format_type_::imsi)
    {
      m_nas_mm_logger.error("Compute_supi_from_suci: Suci format is not imsi.");
      srsran::console("Compute_supi_from_suci: Suci format is not imsi.\n");
      return false;
    }

    // TODO: Is convert suci to c_suci?
    //  struct_suci_to_char(suci, c_suci);

    u_int8_t home_network_pki_value = suci.home_network_public_key_identifier;

    /* 1.find hnet by pki. */
    pri_key_s k = m_cnw->find_hnet(home_network_pki_value);

    if (k.avail == 0 ||
        k.scheme != suci.protection_scheme_id.value)
    {
      m_nas_mm_logger.warning("Can't find valid private key to compute supi, Use default supi from config file.");

      /* 1.1 return default supi. */
      strcpy(supi, m_cnw->get_default_supi());
      return true;
    }

    // TODO: compute SUPI
    if (suci.protection_scheme_id == mobile_identity_5gs_t::suci_s::protection_scheme_id_type::null_scheme)
    {
    }

    return true;
  }

  bool nas_mm::sm_write_dl_info(srsran::unique_byte_buffer_t sm_msg, uint16_t rnti)
  {
    srsran::console("-------mas_mm receive sm message-------\n");
    nas_context *nas_ctx = m_cnw->find_nas_ctx_from_rnti(rnti);
    if (nas_ctx == NULL)
    {
      srsran::console("-------NOT find nas context------\n");
    }
    printf("------------ nas_test_buffer->N_bytes--%d-", sm_msg->N_bytes);
    srsran::unique_byte_buffer_t nas_buffer = srsran::make_byte_buffer();
    if (nas_buffer == nullptr)
    {
      m_nas_mm_logger.error("Couldn't allocate PDU in %s().", __FUNCTION__);
      return false;
    }
    pack_dl_nas_transport(nas_buffer, sm_msg, nas_ctx);

    /*send dl nas transport message*/
    send_mm_dl_msg(rnti, std::move(nas_buffer));
    nas_ctx->m_sec_ctx.dl_nas_count++;

    return true;

  } // 12.9

  bool nas_mm::pack_dl_nas_transport(srsran::unique_byte_buffer_t &nas_buffer, srsran::unique_byte_buffer_t &sm_msg, nas_context *nas_ctx)
  {
    srsran::console("----MM---pack_dl_nas_transport-------\n");
    nas_5gs_msg nas_msg;

    nrmm_ctx_t *nrmm_ctx = &nas_ctx->nrmm_ctx;
    nrcm_ctx_t *nrcm_ctx = &nas_ctx->nrcm_ctx;
    sec_ctx_t *sec_ctx = &nas_ctx->m_sec_ctx;
    nrsm_ctx_t *nrsm_ctx = &nas_ctx->nrsm_ctx;

    dl_nas_transport_t &dl_nas_transport_msg = nas_msg.set_dl_nas_transport();
    /*header setter*/
    nas_msg.hdr.sequence_number = sec_ctx->dl_nas_count;
    nas_msg.hdr.security_header_type = nas_5gs_hdr::integrity_protected_and_ciphered;
    nas_msg.hdr.spare_1_2_version = nas_ctx->m_sec_ctx.spare_1_2_version;

    /*message IE pack*/
    dl_nas_transport_msg.payload_container_type.payload_container_type =
        payload_container_type_t::Payload_container_type_type::options::n1_sm_information;
    int sm_msg_len = sm_msg->N_bytes;
    for (int i = 0; i < sm_msg_len; ++i)
    {
      dl_nas_transport_msg.payload_container.payload_container_contents.push_back(sm_msg->msg[i]);
    }
    dl_nas_transport_msg.pdu_session_id_present = true;
    dl_nas_transport_msg.pdu_session_id.pdu_session_identity_2_value = sm_msg->msg[1];

    //PCAP
    if(m_cnw->get_cnw_args()->pcap_net.enable) {  
      m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, sm_msg->msg, sm_msg->N_bytes);
    } 

    /*message pack*/
    SRSASN_CODE err = nas_msg.pack(nas_buffer);
    if (err != SRSASN_SUCCESS)
    {
      m_nas_mm_logger.error("Error packing pack_dl_nas_transport");
      srsran::console("Error packing pack_dl_nas_transport\n");
      return false;
    }

    //PCAP
    if(m_cnw->get_cnw_args()->pcap_net.enable) {  
      if(nas_msg.hdr.security_header_type == nas_5gs_hdr::security_header_type_opts::plain_5gs_nas_message) {
        m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg, nas_buffer->N_bytes);
      }
      else {
        if(nas_msg.hdr.spare_1_2_version == 0x00) {
          m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg + 7, nas_buffer->N_bytes - 7);
        }
        else {
          m_cnw->send_pcap_nas_pdu_to_enb(nas_pcap_dl, nas_buffer->msg + 11, nas_buffer->N_bytes - 11);
        }
      } 
    } 

    printf("  nas_buffer= ");
    for (uint32_t i = 0; i < nas_buffer->N_bytes; ++i)
    {
      printf(" %x", nas_buffer->msg[i]);
    }
    printf("\n");
    // Encrypt NAS message
    cipher_encrypt(nas_buffer, nas_ctx);

    printf("Encrypt NAS message  nas_buffer= ");
    for (uint32_t i = 0; i < nas_buffer->N_bytes; ++i)
    {
      printf(" %x", nas_buffer->msg[i]);
    }
    printf("\n");

    // Integrity protect NAS message
    if (nas_ctx->m_sec_ctx.spare_1_2_version == 0)
    {
      integrity_generate(&(sec_ctx->k_nas_int)[16], // key_128  count  direction  msg  msg_len  mac nas_ctx
                         sec_ctx->dl_nas_count,
                         SECURITY_DIRECTION_DOWNLINK,
                         &nas_buffer->msg[SEQ_5G_OFFSET],
                         nas_buffer->N_bytes - SEQ_5G_OFFSET,
                         &nas_buffer->msg[MAC_5G_OFFSET],
                         nas_ctx);
    }
    else
    {
      integrity_generate(&(sec_ctx->k_nas_int)[16], // key_128  count  direction  msg  msg_len  mac nas_ctx
                         sec_ctx->dl_nas_count,
                         SECURITY_DIRECTION_DOWNLINK,
                         &nas_buffer->msg[SEQ_5G_OFFSET + 4],
                         nas_buffer->N_bytes - SEQ_5G_OFFSET - 4,
                         &nas_buffer->msg[MAC_5G_OFFSET + 4],
                         nas_ctx);
    };

    printf("protect NAS message  nas_buffer= ");
    for (uint32_t i = 0; i < nas_buffer->N_bytes; ++i)
    {
      printf(" %x", nas_buffer->msg[i]);
    }
    printf("\n");

    return true;
  } // 12.9

  bool nas_mm::sm_notify_ue_erab_updates(uint16_t rnti, uint8_t qos, uint16_t pdu_session_id, srsran::const_byte_span nas_pdu, int am_tm_type)
  {
    printf("-------nas_mm::sm_notify_ue_erab_updates----------\n");
     //rrc_mm->smm_notify_ue_erab_updates(rnti, qos, pdu_session_id, nas_pdu, am_tm_type);
    send_smm_notify_ue_erab_updates(rnti, qos, pdu_session_id, nas_pdu, am_tm_type);
    return true;
  } // 12.14

  // 2024.3.29  test loop
  bool nas_mm::handle_test_loop_message(nas_5gs_hdr &msg, nas_context *nas_ctx)
  {
    srsran::console("!------------handle loop_buffer-----------\n");

    if (msg.message_type == msg_types::options::active_test_mode_complete)
    {
      printf("receive active_test_mode_complete\n");
    }
    if (msg.message_type == msg_types::options::close_ue_test_loop_complete && mm_adp->udp_.ttcn_close_TC == true)
    {
      printf("receive close_ue_test_loop_complete\n");
      srsran::unique_byte_buffer_t send_TTCN_pdu = srsran::make_byte_buffer();
      send_TTCN_pdu->msg[0] = 0x01;
      send_TTCN_pdu->msg[1] = 0x02;
      send_TTCN_pdu->msg[2] = 0x00;
      // test id
      send_TTCN_pdu->msg[3] = 0x00;
      send_TTCN_pdu->msg[4] = 0xff;
      // data_length
      send_TTCN_pdu->msg[5] = 0x00;
      send_TTCN_pdu->msg[6] = 0x01;
      // msg type
      send_TTCN_pdu->msg[7] = 0x81;
      // data drb,first four is first drb,second four is second drb
      send_TTCN_pdu->msg[8] = (nas_ctx->nrmm_ctx.test_loop_drb[0] << 4) | (nas_ctx->nrmm_ctx.test_loop_drb[1]);

      send_TTCN_pdu->N_bytes = 9;

      //   printf("  send_TTCN_pdu= ");
      //   for(uint32_t i=0; i<send_TTCN_pdu->N_bytes; ++i){
      //   printf(" %x",send_TTCN_pdu->msg[i]);
      // }
      // printf("\n");

      // mm_adp->udp_.send_ttcn_info.try_push(std::move(send_TTCN_pdu));
      mm_adp->udp_.send_ttcn_msg_enb(std::move(send_TTCN_pdu));
      std::cout << "mm send close_ue_test_loop_complete to ttcn" << std::endl;
    }
    if (msg.message_type == msg_types::options::deactive_test_mode_complete)
    {
      printf("receive deactive_test_mode_complete\n");
    }
    if (msg.message_type == msg_types::options::open_ue_test_loop_complete)
    {
      printf("receive open_ue_test_loop_complete\n");
    }
    if (msg.extended_protocol_discriminator == nas_5gs_hdr::extended_protocol_discriminator_test_loop)
      return true;
    return false;
  }
  bool nas_mm::pack_activate_test_mode(srsran::unique_byte_buffer_t &nas_buffer, nas_context *nas_ctx, uint16_t enb_ue_id)
  {
    printf("----------pack activate test mode-----------\n");

    nas_5gs_msg nas_msg;

    nrmm_ctx_t *nrmm_ctx = &nas_ctx->nrmm_ctx;
    nrcm_ctx_t *nrcm_ctx = &nas_ctx->nrcm_ctx;
    sec_ctx_t *sec_ctx = &nas_ctx->m_sec_ctx;
    nrsm_ctx_t *nrsm_ctx = &nas_ctx->nrsm_ctx;

    // service_accept_t& msg =  nas_msg.set_service_accept();
    // /*header setter*/

    nas_msg.hdr.sequence_number = sec_ctx->dl_nas_count;
    nas_msg.hdr.security_header_type = nas_5gs_hdr::integrity_protected_and_ciphered;
    nas_msg.hdr.spare_1_2_version = nas_ctx->m_sec_ctx.spare_1_2_version;
    // nas_msg.hdr.security_header_type = nas_5gs_hdr::plain_5gs_nas_message;

    /*message IE pack*/
    activate_test_mode_t &activate_test_mode = nas_msg.set_activate_test_mode_t();
    nas_msg.hdr.message_type.value = msg_types::options::active_test_mode;
    cout << "msg type1:" << nas_msg.hdr.message_type.value << endl;

    nas_msg.hdr.extended_protocol_discriminator = nas_5gs_hdr::extended_protocol_discriminator_test_loop; // 1111
    if (mm_adp->udp_.sdap_nhdr_tran == true)
    {
      activate_test_mode.ue_test_loop_mode.type_value.value = ue_test_loop_mode_t::ue_test_loop_mode::ue_test_loop_mode_b;
    }
    else
    {
      activate_test_mode.ue_test_loop_mode.type_value.value = ue_test_loop_mode_t::ue_test_loop_mode::ue_test_loop_mode_a;
    }

    /*message pack*/
    SRSASN_CODE err = nas_msg.pack(nas_buffer);
    if (err != SRSASN_SUCCESS)
    {
      srsran::console("Error packing pack_activate_test_mode\n");
      return false;
    }

    printf("  nas_buffer11= ");
    for (uint32_t i = 0; i < nas_buffer->N_bytes; ++i)
    {
      printf(" %x", nas_buffer->msg[i]);
    }
    printf("\n");
    // Encrypt NAS message
    cipher_encrypt(nas_buffer, nas_ctx);

    // Integrity protect NAS message
    if (nas_ctx->m_sec_ctx.spare_1_2_version == 0)
    {
      integrity_generate(&(sec_ctx->k_nas_int)[16], // key_128  count  direction  msg  msg_len  mac nas_ctx
                         sec_ctx->dl_nas_count,
                         SECURITY_DIRECTION_DOWNLINK,
                         &nas_buffer->msg[SEQ_5G_OFFSET],
                         nas_buffer->N_bytes - SEQ_5G_OFFSET,
                         &nas_buffer->msg[MAC_5G_OFFSET],
                         nas_ctx);
    }
    else
    {
      integrity_generate(&(sec_ctx->k_nas_int)[16], // key_128  count  direction  msg  msg_len  mac nas_ctx
                         sec_ctx->dl_nas_count,
                         SECURITY_DIRECTION_DOWNLINK,
                         &nas_buffer->msg[SEQ_5G_OFFSET + 4],
                         nas_buffer->N_bytes - SEQ_5G_OFFSET - 4,
                         &nas_buffer->msg[MAC_5G_OFFSET + 4],
                         nas_ctx);
    }

    printf("  nas_buffer22= ");
    for (uint32_t i = 0; i < nas_buffer->N_bytes; ++i)
    {
      printf(" %x", nas_buffer->msg[i]);
    }
    send_mm_dl_msg(enb_ue_id, std::move(nas_buffer));
    sec_ctx->dl_nas_count++;
    cout << "-----test--dl_nas_count: " << sec_ctx->dl_nas_count << endl;
    return true;
  }

  uint16_t nas_mm::loop_bytes_allocate(uint16_t testID)
  {
    // pdcp (8 bits->1 bytes)
    if (mm_adp->udp_.enble_ttcn_flag_.ttcn_testId == 627 || mm_adp->udp_.enble_ttcn_flag_.ttcn_testId == 621)
    {
      return (uint16_t)0x01 << 8 | (uint16_t)0x70; // 46bytes
    }
    else if (mm_adp->udp_.enble_ttcn_flag_.ttcn_testId == 626 || mm_adp->udp_.enble_ttcn_flag_.ttcn_testId == 624)
    {
      return 0x00f0; // 30bytes
    }
    else if (mm_adp->udp_.enble_ttcn_flag_.ttcn_testId == 628)
    {
      return 0x0060; // 12bytes
    }
    else if (mm_adp->udp_.enble_ttcn_flag_.ttcn_testId == 622)
    {
      return 0x10; // 2bytes
    }
    else if (mm_adp->udp_.enble_ttcn_flag_.ttcn_testId == 623)
    {
      return 0x0190; // 50bytes
    }
    else if (mm_adp->udp_.enble_ttcn_flag_.ttcn_testId == 625 || mm_adp->udp_.enble_ttcn_flag_.ttcn_testId == 631 || mm_adp->udp_.enble_ttcn_flag_.ttcn_testId == 632 || mm_adp->udp_.enble_ttcn_flag_.ttcn_testId == 154)
    {
      return 0x08; // 1bytes
    }
    else
    {
      std::cout<<""<<std::endl;
      return 0x08; // 1bytes
    }
  }

  bool nas_mm::pack_close_ue_test_loop(srsran::unique_byte_buffer_t &nas_buffer, nas_context *nas_ctx, uint16_t enb_ue_id)
  {
    printf("------close ue test loop----------\n");

    nas_5gs_msg nas_msg;

    nrmm_ctx_t *nrmm_ctx = &nas_ctx->nrmm_ctx;
    nrcm_ctx_t *nrcm_ctx = &nas_ctx->nrcm_ctx;
    sec_ctx_t *sec_ctx = &nas_ctx->m_sec_ctx;
    nrsm_ctx_t *nrsm_ctx = &nas_ctx->nrsm_ctx;

    // /*header setter*/

    nas_msg.hdr.sequence_number = sec_ctx->dl_nas_count;
    nas_msg.hdr.security_header_type = nas_5gs_hdr::integrity_protected_and_ciphered;
    nas_msg.hdr.spare_1_2_version = nas_ctx->m_sec_ctx.spare_1_2_version;
    /*message IE pack*/
    // test_loop_message test_loop;

    close_ue_test_loop_t &close_ue_test_loop = nas_msg.set_close_ue_test_loop();
    if (mm_adp->udp_.sdap_nhdr_tran == true)
    {
      // mode b
      std::cout << "mm_adp->udp_.sdap_nhdr_tran == " << mm_adp->udp_.sdap_nhdr_tran << endl;
      close_ue_test_loop.ue_test_loop_mode.type_value.value = ue_test_loop_mode_t::ue_test_loop_mode::ue_test_loop_mode_b;
      close_ue_test_loop.ue_test_loop_mode_b_lb_setup_present = true;
      close_ue_test_loop.ue_test_loop_mode_b_lb_setup.ip_pdu_delay = 0x00;
      nas_msg.hdr.message_type.value = msg_types::options::close_ue_test_loop;
      nas_msg.hdr.extended_protocol_discriminator = nas_5gs_hdr::extended_protocol_discriminator_test_loop; // 1111
      nas_ctx->nrmm_ctx.test_loop_drb.push_back(0x02);
    }
    else
    {
      // mode a
      close_ue_test_loop.ue_test_loop_mode.type_value.value = ue_test_loop_mode_t::ue_test_loop_mode::ue_test_loop_mode_a;
      close_ue_test_loop.ue_test_loop_mode_a_lb_setup_present = true;
      ue_test_loop_mode_a_lb_setup_t::lb_setup_drb lb1;
      lb1.q = 0x02;
      lb1.z = loop_bytes_allocate(mm_adp->udp_.enble_ttcn_flag_.ttcn_testId);
      int temp = static_cast<int>(lb1.z);
      std::cout << " lb1.z = " << (int)lb1.z << std::endl;
      std::cout << " lb1.z temp =" << temp << std::endl;
      close_ue_test_loop.ue_test_loop_mode_a_lb_setup.lb_setup_list.push_back(lb1);
      // close_ue_test_loop.ue_test_loop_mode_b_lb_setup_present=true;
      // close_ue_test_loop.ue_test_loop_mode_b_lb_setup.ip_pdu_delay=0x01;
      nas_msg.hdr.message_type.value = msg_types::options::close_ue_test_loop;
      nas_msg.hdr.extended_protocol_discriminator = nas_5gs_hdr::extended_protocol_discriminator_test_loop; // 1111

      // tset
      // ue_test_loop_mode_a_lb_setup_t::lb_setup_drb lb2;
      //  lb2.q=0x04;
      //  lb2.z=0x20;
      //  close_ue_test_loop.ue_test_loop_mode_a_lb_setup.lb_setup_list.push_back(lb2);
      //  //drb
      nas_ctx->nrmm_ctx.test_loop_drb.push_back(lb1.q);
      // nas_ctx->nrmm_ctx.test_loop_drb.push_back(lb2.q);
    }
    /*message pack*/
    SRSASN_CODE err = nas_msg.pack(nas_buffer);
    if (err != SRSASN_SUCCESS)
    {
      srsran::console("Error packing pack_close_ue_test_loop\n");
      return false;
    }

    printf("  nas_buffer= ");
    for (uint32_t i = 0; i < nas_buffer->N_bytes; ++i)
    {
      printf(" %x", nas_buffer->msg[i]);
    }
    printf("\n");
    // Encrypt NAS message
    cipher_encrypt(nas_buffer, nas_ctx);

    // Integrity protect NAS message
    if (nas_ctx->m_sec_ctx.spare_1_2_version == 0)
    {
      integrity_generate(&(sec_ctx->k_nas_int)[16], // key_128  count  direction  msg  msg_len  mac nas_ctx
                         sec_ctx->dl_nas_count,
                         SECURITY_DIRECTION_DOWNLINK,
                         &nas_buffer->msg[SEQ_5G_OFFSET],
                         nas_buffer->N_bytes - SEQ_5G_OFFSET,
                         &nas_buffer->msg[MAC_5G_OFFSET],
                         nas_ctx);
    }
    else
    {
      integrity_generate(&(sec_ctx->k_nas_int)[16], // key_128  count  direction  msg  msg_len  mac nas_ctx
                         sec_ctx->dl_nas_count,
                         SECURITY_DIRECTION_DOWNLINK,
                         &nas_buffer->msg[SEQ_5G_OFFSET + 4],
                         nas_buffer->N_bytes - SEQ_5G_OFFSET - 4,
                         &nas_buffer->msg[MAC_5G_OFFSET + 4],
                         nas_ctx);
    }

    send_mm_dl_msg(enb_ue_id, std::move(nas_buffer));
    sec_ctx->dl_nas_count++;
    return true;
  }
  bool nas_mm::pack_open_ue_test_loop(srsran::unique_byte_buffer_t &nas_buffer, nas_context *nas_ctx, uint16_t enb_ue_id)
  {
    printf("----------pack pack open ue test loope-----------\n");
    nas_5gs_msg nas_msg;

    nrmm_ctx_t *nrmm_ctx = &nas_ctx->nrmm_ctx;
    nrcm_ctx_t *nrcm_ctx = &nas_ctx->nrcm_ctx;
    sec_ctx_t *sec_ctx = &nas_ctx->m_sec_ctx;
    nrsm_ctx_t *nrsm_ctx = &nas_ctx->nrsm_ctx;

    // service_accept_t& msg =  nas_msg.set_service_accept();
    // /*header setter*/

    nas_msg.hdr.sequence_number = sec_ctx->dl_nas_count;
    nas_msg.hdr.security_header_type = nas_5gs_hdr::integrity_protected_and_ciphered;
    nas_msg.hdr.spare_1_2_version = nas_ctx->m_sec_ctx.spare_1_2_version;

    // nas_msg.hdr.security_header_type = nas_5gs_hdr::plain_5gs_nas_message;

    /*message IE pack*/
    open_ue_test_loop_t &open_ue_test_loop = nas_msg.set_open_ue_test_loop_t();
    nas_msg.hdr.message_type.value = msg_types::options::open_ue_test_loop;
    nas_msg.hdr.extended_protocol_discriminator = nas_5gs_hdr::extended_protocol_discriminator_test_loop; // 1111

    /*message pack*/
    SRSASN_CODE err = nas_msg.pack(nas_buffer);
    if (err != SRSASN_SUCCESS)
    {
      srsran::console("Error packing pack_open_ue_test_loop\n");
      return false;
    }

    printf("  open ue test loope11= ");
    for (uint32_t i = 0; i < nas_buffer->N_bytes; ++i)
    {
      printf(" %x", nas_buffer->msg[i]);
    }
    printf("\n");
    // Encrypt NAS message
    cipher_encrypt(nas_buffer, nas_ctx);

    // Integrity protect NAS message
    if (nas_ctx->m_sec_ctx.spare_1_2_version == 0)
    {
      integrity_generate(&(sec_ctx->k_nas_int)[16], // key_128  count  direction  msg  msg_len  mac nas_ctx
                         sec_ctx->dl_nas_count,
                         SECURITY_DIRECTION_DOWNLINK,
                         &nas_buffer->msg[SEQ_5G_OFFSET],
                         nas_buffer->N_bytes - SEQ_5G_OFFSET,
                         &nas_buffer->msg[MAC_5G_OFFSET],
                         nas_ctx);
    }
    else
    {
      integrity_generate(&(sec_ctx->k_nas_int)[16], // key_128  count  direction  msg  msg_len  mac nas_ctx
                         sec_ctx->dl_nas_count,
                         SECURITY_DIRECTION_DOWNLINK,
                         &nas_buffer->msg[SEQ_5G_OFFSET + 4],
                         nas_buffer->N_bytes - SEQ_5G_OFFSET - 4,
                         &nas_buffer->msg[MAC_5G_OFFSET + 4],
                         nas_ctx);
    }

    printf(" open ue test loope22= ");
    for (uint32_t i = 0; i < nas_buffer->N_bytes; ++i)
    {
      printf(" %x", nas_buffer->msg[i]);
    }
    send_mm_dl_msg(enb_ue_id, std::move(nas_buffer));
    sec_ctx->dl_nas_count++;
    cout << "-----test--dl_nas_count: " << sec_ctx->dl_nas_count << endl;
    return true;
  }

  bool nas_mm::pack_deactivate_test_mode(srsran::unique_byte_buffer_t &nas_buffer, nas_context *nas_ctx, uint16_t enb_ue_id)
  {
    printf("----------pack deactivate test mode-----------\n");
    nas_5gs_msg nas_msg;

    nrmm_ctx_t *nrmm_ctx = &nas_ctx->nrmm_ctx;
    nrcm_ctx_t *nrcm_ctx = &nas_ctx->nrcm_ctx;
    sec_ctx_t *sec_ctx = &nas_ctx->m_sec_ctx;
    nrsm_ctx_t *nrsm_ctx = &nas_ctx->nrsm_ctx;

    // service_accept_t& msg =  nas_msg.set_service_accept();
    // /*header setter*/

    nas_msg.hdr.sequence_number = sec_ctx->dl_nas_count;
    nas_msg.hdr.spare_1_2_version = nas_ctx->m_sec_ctx.spare_1_2_version;
    nas_msg.hdr.security_header_type = nas_5gs_hdr::integrity_protected_and_ciphered;

    // nas_msg.hdr.security_header_type = nas_5gs_hdr::plain_5gs_nas_message;

    /*message IE pack*/
    deactivate_test_mode_t &deactivate_test_mode = nas_msg.set_deactivate_test_mode_t();
    nas_msg.hdr.message_type.value = msg_types::options::deactive_test_mode;
    nas_msg.hdr.extended_protocol_discriminator = nas_5gs_hdr::extended_protocol_discriminator_test_loop; // 1111
    /*message pack*/
    SRSASN_CODE err = nas_msg.pack(nas_buffer);
    if (err != SRSASN_SUCCESS)
    {
      srsran::console("Error packing pack_deactivate_test_mode\n");
      return false;
    }

    printf("  deactivate test mode11= ");
    for (uint32_t i = 0; i < nas_buffer->N_bytes; ++i)
    {
      printf(" %x", nas_buffer->msg[i]);
    }
    printf("\n");
    // Encrypt NAS message
    cipher_encrypt(nas_buffer, nas_ctx);

    // Integrity protect NAS message
    if (nas_ctx->m_sec_ctx.spare_1_2_version == 0)
    {
      integrity_generate(&(sec_ctx->k_nas_int)[16], // key_128  count  direction  msg  msg_len  mac nas_ctx
                         sec_ctx->dl_nas_count,
                         SECURITY_DIRECTION_DOWNLINK,
                         &nas_buffer->msg[SEQ_5G_OFFSET],
                         nas_buffer->N_bytes - SEQ_5G_OFFSET,
                         &nas_buffer->msg[MAC_5G_OFFSET],
                         nas_ctx);
    }
    else
    {
      integrity_generate(&(sec_ctx->k_nas_int)[16], // key_128  count  direction  msg  msg_len  mac nas_ctx
                         sec_ctx->dl_nas_count,
                         SECURITY_DIRECTION_DOWNLINK,
                         &nas_buffer->msg[SEQ_5G_OFFSET + 4],
                         nas_buffer->N_bytes - SEQ_5G_OFFSET - 4,
                         &nas_buffer->msg[MAC_5G_OFFSET + 4],
                         nas_ctx);
    }

    printf(" deactivate test mode22= ");
    for (uint32_t i = 0; i < nas_buffer->N_bytes; ++i)
    {
      printf(" %x", nas_buffer->msg[i]);
    }

    send_mm_dl_msg(enb_ue_id, std::move(nas_buffer));
    sec_ctx->dl_nas_count++;
    cout << "-----test--dl_nas_count: " << sec_ctx->dl_nas_count << endl;
    return true;
  }

  bool nas_mm::pcs_sm_modification(uint16_t rnti, uint8_t modify_flag)
  {
    printf("mm to sm modification\n");
    nas_context *nas_ctx = m_cnw->find_nas_ctx_from_rnti(rnti);
    ims_context *ims_ctx = m_cnw->m_pcs_ims->find_ims_ctx_from_rnti(rnti);

    nas_ctx->nrsm_ctx.modify_flag = modify_flag;
    srsran::unique_byte_buffer_t sm_tx = srsran::make_byte_buffer();
    if (modify_flag == 3) // nas 1.0 modify
    {
      m_nas_sm->pack_pdu_session_modification_command(sm_tx, nas_ctx, nas_ctx->nrsm_ctx.is_ims);
    }
    if (modify_flag == 2) // nas 1.0 modify relese
    {
      m_nas_sm->pack_pdu_session_modification_command_close_phone(sm_tx, nas_ctx, nas_ctx->nrsm_ctx.is_ims);
    }

    srsran::unique_byte_buffer_t mm_ctx = srsran::make_byte_buffer();
    this->pack_dl_nas_transport(mm_ctx, sm_tx, nas_ctx);

    // const srsran::unique_byte_buffer_t nas_msg = srsran::make_byte_buffer();       //rrc reconf
    srsran::unique_byte_buffer_t nas_msg = srsran::make_byte_buffer(); // 6.3 dl
    if (nas_msg == nullptr)
    {
      srsran::console("---------Couldn't allocate PDU in -send_reg_accept------\n");
      return false;
    }
    nas_msg->N_bytes = mm_ctx->N_bytes;
    memcpy(nas_msg->msg, mm_ctx->msg, nas_msg->N_bytes);

    srsran::const_byte_span nas_pdu = srsran::make_span(nas_msg);
    uint8_t qos = 3;
    if (modify_flag == 1)
    {
      // dl
      //  if(ims_ctx->ims_ctx.ate_indication != set_up_mt_call)
      //  {

      //     send_mm_dl_msg(rnti, std::move(nas_msg));
      //  }

      // rrc_mm->smm_notify_ue_erab_updates(rnti, qos, nas_ctx->nrsm_ctx.pdu_session_id[1], {}, 1);
      send_voice_smm_notify_ue_erab_updates(rnti, qos, nas_ctx->nrsm_ctx.pdu_session_id[1], {}, 1);
      // rrc reconf
      //  rrc_mm->smm_notify_ue_erab_updates(rnti, qos, nas_ctx->nrsm_ctx.pdu_session_id[1], nas_pdu, 1);
    }
    if (modify_flag == 2)
    {
      // dl
      send_mm_dl_msg(rnti, std::move(nas_msg));
      // rrc_mm->smm_notify_ue_erab_updates(rnti, qos, nas_ctx->nrsm_ctx.pdu_session_id[1], {}, 1);
      send_voice_smm_notify_ue_erab_updates(rnti, qos, nas_ctx->nrsm_ctx.pdu_session_id[1], {}, 1);
    }
    if (modify_flag == 3) // 1.2v mt call
    {
      //send_mm_dl_msg(rnti, std::move(nas_msg));
      send_voice_smm_notify_ue_erab_updates(rnti,qos,nas_ctx->nrsm_ctx.pdu_session_id[1],nas_pdu,1);
    }

    return true;
  }

  bool nas_mm::send_switch_info_to_ate()
  {
    /*send message to ate infrom  res  */
    srsran::unique_byte_buffer_t nas_ate_tx = srsran::make_byte_buffer();
    if (nas_ate_tx == nullptr)
    {
      srsran::console("Couldn't allocate PDU in TC_MSG_NAS_ATE_INFO \n");
      return false;
    }
    nas_ate_tx->N_bytes = 5;
    nas_ate_tx->msg[0] = 0xff;
    nas_ate_tx->msg[1] = 0x01;
    nas_ate_tx->msg[2] = TC_MSG_NAS_ATE_INFO;
    nas_ate_tx->msg[3] = 0x02;
    nas_ate_tx->msg[4] = 0x01;

    // send to ate msg queue
    srsran::console("send nas_msg to ate queue \n");
    mm_adp->udp_.send_ate_msg(std::move(nas_ate_tx));

    return true;
  }

  bool nas_mm::send_mm_dl_msg(uint16_t rnti, srsran::unique_byte_buffer_t mm_pdu)
  {
    srsran::unique_byte_buffer_t enb_pdu = srsran::make_byte_buffer();
    enb_msg_header_t s1_header;
    s1_header.enb_id = mm_adp->udp_.enb_id;
    s1_header.rnti = 70;
    s1_header.msg_type = nas_dl;

    int len = sizeof(s1_header);

    memcpy(enb_pdu->msg, &s1_header, len);
    memcpy(enb_pdu->msg + len, mm_pdu->msg, mm_pdu->N_bytes);

    enb_pdu->N_bytes = mm_pdu->N_bytes + len;

    mm_adp->udp_.send_enb_msg(std::move(enb_pdu));

    return true;
  }

  bool nas_mm::send_start_rem_rel_user(uint16_t rnti)
  {
    srsran::unique_byte_buffer_t enb_pdu = srsran::make_byte_buffer();
    enb_msg_header_t s1_header;
    s1_header.enb_id = mm_adp->udp_.enb_id;
    s1_header.rnti = rnti;
    s1_header.msg_type = start_rem_rel_user;

    int len = sizeof(s1_header);

    memcpy(enb_pdu->msg, &s1_header, len);
    enb_pdu->N_bytes = len;

    mm_adp->udp_.send_enb_msg(std::move(enb_pdu));

    return true;
  }

  bool nas_mm::send_start_rem_user(uint16_t rnti)
  {
    srsran::unique_byte_buffer_t enb_pdu = srsran::make_byte_buffer();
    enb_msg_header_t s1_header;
    s1_header.enb_id = mm_adp->udp_.enb_id;
    s1_header.rnti = rnti;
    s1_header.msg_type = start_rem_user;

    int len = sizeof(s1_header);

    memcpy(enb_pdu->msg, &s1_header, len);
    enb_pdu->N_bytes = len;

    mm_adp->udp_.send_enb_msg(std::move(enb_pdu));

    return true;
  }

  bool nas_mm::send_test_reg_ss_no5Gguti(uint16_t rnti)
  {
    srsran::unique_byte_buffer_t enb_pdu = srsran::make_byte_buffer();
    enb_msg_header_t s1_header;
    s1_header.enb_id = mm_adp->udp_.enb_id;
    s1_header.rnti = rnti;
    s1_header.msg_type = test_reg_ss_no5Gguti;

    int len = sizeof(s1_header);

    memcpy(enb_pdu->msg, &s1_header, len);
    enb_pdu->N_bytes = len;

    mm_adp->udp_.send_enb_msg(std::move(enb_pdu));

    return true;
  }

  bool nas_mm::send_setup_ue_ctxt(uint16_t rnti, uint8 *k_gnb)
  {
    srsran::unique_byte_buffer_t enb_pdu = srsran::make_byte_buffer();
    enb_msg_header_t s1_header;
    s1_header.enb_id = mm_adp->udp_.enb_id;
    s1_header.rnti = rnti;
    s1_header.msg_type = setup_ue_ctxt;

    int len = sizeof(s1_header);

    memcpy(enb_pdu->msg, &s1_header, len);
    memcpy(enb_pdu->msg + len, k_gnb, 32);

    enb_pdu->N_bytes = len + 32;

    mm_adp->udp_.send_enb_msg(std::move(enb_pdu));

    return true;
  }

  bool nas_mm::send_smm_notify_ue_erab_updates(uint16_t rnti, uint8_t qos, uint16_t pdu_session_id, srsran::const_byte_span nas_pdu, int am_tm_type)
  {
    srsran::unique_byte_buffer_t enb_pdu = srsran::make_byte_buffer();
    enb_msg_header_t s1_header;
    s1_header.enb_id = mm_adp->udp_.enb_id;
    s1_header.rnti = rnti;
    s1_header.msg_type = rrc_reconfig;

    int len = sizeof(s1_header);
    enb_pdu->N_bytes = len;

    memcpy(enb_pdu->msg, &s1_header, len);
    memcpy(enb_pdu->msg + len, &qos, 1);
    uint8_t pdu_id = pdu_session_id;
    memcpy(enb_pdu->msg + len + 1, &pdu_id, 1);
    uint8_t am_tm_type_ = am_tm_type;
    memcpy(enb_pdu->msg + len + 2, &am_tm_type_, 1);

    enb_pdu->N_bytes += 3;

    memcpy(enb_pdu->msg + enb_pdu->N_bytes, (uint8_t *)nas_pdu.data(), nas_pdu.size());

    enb_pdu->N_bytes += nas_pdu.size();

    mm_adp->udp_.send_enb_msg(std::move(enb_pdu));

    return true;
  }

  bool nas_mm::send_voice_smm_notify_ue_erab_updates(uint16_t rnti, uint8_t qos, uint16_t pdu_session_id, srsran::const_byte_span nas_pdu, int am_tm_type)
  {
    srsran::unique_byte_buffer_t enb_pdu = srsran::make_byte_buffer();
    enb_msg_header_t s1_header;
    s1_header.enb_id = mm_adp->udp_.enb_id;
    s1_header.rnti = rnti;
    s1_header.msg_type = rrc_reconfig_voice;

    int len = sizeof(s1_header);
    enb_pdu->N_bytes = len;

    memcpy(enb_pdu->msg, &s1_header, len);
    memcpy(enb_pdu->msg + len, &qos, 1);
    uint8_t pdu_id = pdu_session_id;
    memcpy(enb_pdu->msg + len + 1, &pdu_id, 1);
    uint8_t am_tm_type_ = am_tm_type;
    memcpy(enb_pdu->msg + len + 2, &am_tm_type_, 1);
    memcpy(enb_pdu->msg + len + 3, &mm_adp->udp_.voice_indicate, 1);
    std::cout << "cnw voice rate: " << mm_adp->udp_.voice_indicate << std::endl;
    enb_pdu->N_bytes += 4;

    memcpy(enb_pdu->msg + enb_pdu->N_bytes, (uint8_t *)nas_pdu.data(), nas_pdu.size());

    enb_pdu->N_bytes += nas_pdu.size();

    mm_adp->udp_.send_enb_msg(std::move(enb_pdu));

    return true;
  }

  bool nas_mm::send_test_smm_notify_ue_erab_updates(uint16_t rnti, uint8_t qos, uint16_t pdu_session_id, srsran::const_byte_span nas_pdu, int am_tm_type)
  {
    srsran::unique_byte_buffer_t enb_pdu = srsran::make_byte_buffer();
    enb_msg_header_t s1_header;
    s1_header.enb_id = mm_adp->udp_.enb_id;
    s1_header.rnti = rnti;
    s1_header.msg_type = test_smm_notify_ue_erab_updates;

    int len = sizeof(s1_header);
    enb_pdu->N_bytes = len;

    memcpy(enb_pdu->msg, &s1_header, len);
    memcpy(enb_pdu->msg + len, &qos, 1);
    uint8_t pdu_id = pdu_session_id;
    memcpy(enb_pdu->msg + len + 1, &pdu_id, 1);
    uint8_t am_tm_type_ = am_tm_type;
    memcpy(enb_pdu->msg + len + 2, &am_tm_type_, 1);

    enb_pdu->N_bytes += 3;

    memcpy(enb_pdu->msg + enb_pdu->N_bytes, (uint8_t *)nas_pdu.data(), nas_pdu.size());

    enb_pdu->N_bytes += nas_pdu.size();

    mm_adp->udp_.send_enb_msg(std::move(enb_pdu));

    return true;
  }
  void nas_mm::pcstomm(uint16_t rnti)
  {
    rrc_mm->rrctorrc_ue(rnti);
  }
  // bool nas_mm::send_wx_Mcontrol_Notify_Switch(uint16_t rnti, uint8_t Switch_Type_)
  // {
  //   srsran::unique_byte_buffer_t enb_pdu = srsran::make_byte_buffer();
  //   enb_msg_header_t s1_header;
  //   s1_header.enb_id = mm_adp->udp_.enb_id;
  //   s1_header.rnti = rnti;
  //   s1_header.msg_type = wx_Mcontrol_Notify_Switch;

  //   int len = sizeof(s1_header);
  //   enb_pdu->N_bytes = len;

  //   memcpy(enb_pdu->msg, &s1_header, len);
  //   memcpy(enb_pdu->msg + len, &Switch_Type_, 1);

  //   enb_pdu->N_bytes += 1;

  //   mm_adp->udp_.send_enb_msg(std::move(enb_pdu));

  //   return true;
  // }

  bool nas_mm::send_wx_Mcontrol_Notify_Switch(uint16_t rnti, int Switch_Type_, uint8_t* add_info,  int len_info)
  {
    srsran::unique_byte_buffer_t enb_pdu = srsran::make_byte_buffer();
    enb_msg_header_t s1_header;
    s1_header.enb_id = mm_adp->udp_.enb_id;
    s1_header.rnti = rnti;
    s1_header.msg_type = wx_Mcontrol_Notify_Switch;

    int len = sizeof(s1_header);
    enb_pdu->N_bytes = len;

    memcpy(enb_pdu->msg, &s1_header, len);
    memcpy(enb_pdu->msg + len, &Switch_Type_, 1);
    memcpy(enb_pdu->msg + 1 + len, add_info, len_info);

    enb_pdu->N_bytes += 1;
    enb_pdu->N_bytes += len_info;

    mm_adp->udp_.send_enb_msg(std::move(enb_pdu));

    return true;
  }

  bool nas_mm::send_nas_to_notify_rrc_release(uint16_t rnti)
  {
    srsran::unique_byte_buffer_t enb_pdu = srsran::make_byte_buffer();
    enb_msg_header_t s1_header;
    s1_header.enb_id = mm_adp->udp_.enb_id;
    s1_header.rnti = rnti;
    s1_header.msg_type = nas_to_notify_rrc_release;

    int len = sizeof(s1_header);
    enb_pdu->N_bytes = len;

    memcpy(enb_pdu->msg, &s1_header, len);

    mm_adp->udp_.send_enb_msg(std::move(enb_pdu));

    return true;
  }

} // namespace srsepc
