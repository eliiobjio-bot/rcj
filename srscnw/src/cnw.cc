#include "srscnw/hdr/cnw.h"
#include "srscnw/hdr/nas_mm.h"
#include "srsran/common/bcd_helpers.h"
#include "srsran/common/liblte_security.h"
#include "srsran/common/string_helpers.h"
#include "srsran/common/network_utils.h"
#include <arpa/inet.h>
#include <cmath>
#include <inttypes.h> // for printing uint64_t
#include <netinet/sctp.h>
#include <random>
#include <sys/socket.h>
#include <sys/types.h>
#include <fstream>
#include <unordered_map>

using namespace srsran::nas_5g;

namespace srsepc
{

  cnw *cnw::m_instance = NULL;
  pthread_mutex_t cnw_instance_mutex = PTHREAD_MUTEX_INITIALIZER;

  cnw::cnw() : m_running(false), thread("CNW"), rx_pdu_queue(64)
  {
    return;
  }

  cnw::~cnw()
  {
    return;
  }

  cnw *cnw::get_instance(void)
  {
    pthread_mutex_lock(&cnw_instance_mutex);
    if (NULL == m_instance)
    {
      m_instance = new cnw();
    }
    pthread_mutex_unlock(&cnw_instance_mutex);
    return (m_instance);
  }

  void cnw::cleanup(void)
  {
    pthread_mutex_lock(&cnw_instance_mutex);
    if (NULL != m_instance)
    {
      delete m_instance;
      m_instance = NULL;
    }
    pthread_mutex_unlock(&cnw_instance_mutex);
  }

  // int cnw::init(const cnw_args_t& args , adp *adp_)
  // {
  //   cnw_adp = adp_;
  //   m_nas_mm = nas_mm::get_instance();
  //   m_nas_mm->init(rrc, cnw_adp, args);

  //   m_nas_sm = nas_sm::get_instance();
  //   m_nas_sm->init(cnw_adp, args);

  //   m_pcs_ims = pcs_ims::get_instance();
  //   m_pcs_ims->init(rrc, cnw_adp);

  //   m_xw_icmp = xw_icmp::get_instance();
  //   m_xw_icmp->init(cnw_adp);

  //   // 6.1
  //   cnw_args = args;

  //   /*Read user information from DB*/
  //   if (read_db_file(cnw_args.user_db_file) == false)
  //   {
  //     srsran::console("Error reading user database file %s\n", cnw_args.user_db_file.c_str());
  //     return -1;
  //   }

  //   return 0;
  // }

  int cnw::init(cnw_args_t& args)
  {
    cnw_adp.init(args, m_instance);

    m_nas_mm = nas_mm::get_instance();
    m_nas_mm->init(rrc, &cnw_adp, args);

    m_nas_sm = nas_sm::get_instance();
    m_nas_sm->init(&cnw_adp, args);

    m_pcs_ims = pcs_ims::get_instance();
    m_pcs_ims->init(rrc, &cnw_adp);

    m_xw_icmp = xw_icmp::get_instance();
    m_xw_icmp->init(&cnw_adp);

    // 6.1
    cnw_args = args;

    /*Read user information from DB*/
    if (read_db_file(cnw_args.user_db_file) == false)
    {
      srsran::console("Error reading user database file %s\n", cnw_args.user_db_file.c_str());
      return -1;
    }

    return 0;
  }

  void cnw::stop()
  {
    if (m_running)
    {
      m_running = false;
      rx_pdu_queue.clear();
      thread_cancel();
      wait_thread_finish();
    }
    m_xw_icmp->stop();
    cnw_adp.stop();
    return;
  }

  void cnw::run_thread()
  {
    //  Mark the thread as running
    m_running = true;
    printf("cnw module thread is running!\n");

    srsran::unique_byte_buffer_t s1_msg = srsran::make_byte_buffer();
    srsran::unique_byte_buffer_t ate_msg = srsran::make_byte_buffer();

    while (m_running) {

      m_nas_sm->TC_92_after_3_seconds();
      if(cnw_adp.udp_.received_enb_info.try_pop(s1_msg)) {
        handle_s1u_pdu(std::move(s1_msg));
      }
      if(cnw_adp.udp_.ate_cnw_msg.size()!=0) {
        cnw_adp.udp_.ate_cnw_msg.try_pop(ate_msg);
        uint8_t tar_level = ate_msg->msg[1];
        switch (tar_level)
        {
        case ATE_TARGET_LEVEL_IMS:
          m_pcs_ims->pcs_handle_ate_info(std::move(ate_msg));
          break;
        case ATE_TARGET_LEVEL_NAS:
          handle_ate_msg(std::move(ate_msg));
          break;
        case ATE_TARGET_LEVEL_ICMP:
          // cnw_adp.udp_.icmp_receive_sdap_info.try_push(std::move(ate_msg));
          m_xw_icmp->handle_ate_pdu(ate_msg);
          break;
        default:
          break;
        }
      }

      usleep(1000);

    }
    return;
  }

  void cnw::run_cnw()  
  {
    printf("cnw module thread is running!\n");

    srsran::unique_byte_buffer_t s1_msg = srsran::make_byte_buffer();
    srsran::unique_byte_buffer_t ate_msg = srsran::make_byte_buffer();

    m_nas_sm->TC_92_after_3_seconds();
    if(cnw_adp.udp_.received_enb_info.try_pop(s1_msg)) {
      handle_s1u_pdu(std::move(s1_msg));
    }
    if(cnw_adp.udp_.ate_cnw_msg.size()!=0) {
      cnw_adp.udp_.ate_cnw_msg.try_pop(ate_msg);
      uint8_t tar_level = ate_msg->msg[1];
      switch (tar_level)
      {
      case ATE_TARGET_LEVEL_IMS:
        m_pcs_ims->pcs_handle_ate_info(std::move(ate_msg));
        break;
      case ATE_TARGET_LEVEL_NAS:
        handle_ate_msg(std::move(ate_msg));
        break;
      case ATE_TARGET_LEVEL_ICMP:
        // cnw_adp.udp_.icmp_receive_sdap_info.try_push(std::move(ate_msg));
        m_xw_icmp->handle_ate_pdu(ate_msg);
        break;
      default:
        break;
      }
    }

    return;
  }

  bool cnw::handle_s1u_pdu(srsran::unique_byte_buffer_t s1_pdu)
  { 
    printf("[cnw]: handle_s1u_pdu function!\n");
    if(s1_pdu->N_bytes < 4) {
      printf("[cnw]: received error enb msg! cause:s1_size < 4\n");
      return false;
    }
    enb_msg_header_t *enb_head = (enb_msg_header_t *)s1_pdu->msg;
    printf("[cnw]: enb_id: %d rnti: %d\n", (int)enb_head->enb_id, (int)enb_head->rnti);
    cnw_adp.udp_.enb_id = enb_head->enb_id;

    srsran::unique_byte_buffer_t pdu = srsran::make_byte_buffer();
    pdu->N_bytes = s1_pdu->N_bytes-4;
    memcpy(pdu->msg, s1_pdu->msg+4, s1_pdu->N_bytes-4);

    switch (enb_head->msg_type)
    {
    case enb_msg_type::init_ue:
      printf("[cnw] received s1_msg_type: init_ue\n");
      initial_ue(enb_head->rnti, std::move(pdu));
      break;
    case enb_msg_type::nas_ul:
      printf("[cnw] received s1_msg_type: nas_ul\n");
      write_pdu(enb_head->rnti, std::move(pdu));
      break;
    case enb_msg_type::ims_ul:
      printf("[cnw] received s1_msg_type: ims_ul\n");
      rrc_to_pcs_ims(enb_head->rnti, std::move(pdu));
      break;   
    case enb_msg_type::send_reg_or_service_accept :
      printf("[cnw] received s1_msg_type: send_reg_or_service_accept\n"); 
      send_reg_or_service_accept(enb_head->rnti);
      break;  
    case enb_msg_type::ip_data:
      printf("[cnw] received s1_msg_type: ip_data\n"); 
      m_xw_icmp->handle_ip_pdu(pdu);
      break;
    case enb_msg_type::notify_change_enb:
      printf("[cnw] received s1_msg_type: notify_change_enb\n"); 
      cnw_adp.udp_.enb_id = enb_head->enb_id;
      break;
    case enb_msg_type::notify_send_user_release:
      printf("[cnw] received s1_msg_type: notify_send_user_release\n"); 
      rrc_notify_nas_to_release(enb_head->rnti);
      break;

    default:
      break;
    }

    return true;
  }


  bool cnw::handle_ate_msg(srsran::unique_byte_buffer_t pdu)
  {
    printf("[NAS]: handle_ate_msg function!!\n");
    if (pdu->N_bytes < 4)
    {
      printf("[NAS]: handle_ate_msg error size < 4 !\n");
      return false;
    }
    switch (pdu->msg[2])
    {
    case TC_MSG_ATE_NAS_CONTROL:
      if (pdu->msg[3] == TC_NAS_CONTROL_EXCHANGE_CHANNEL)
      {
        printf("[NAS]: received ate msg type: \n");
        m_nas_mm->send_test_smm_notify_ue_erab_updates(70, 2, 1, {}, 0);
      }
      if (pdu->msg[3] == TC_NAS_CONTROL_EXCHANGE_BEAM)
      { // 切换波束
        if (pdu->N_bytes < 5)
        {
          std::cout << "TC_NAS_CONTROL_EXCHANGE_BEAM error size <5" << std::endl;
          return false;
        }
        if (pdu->msg[4] == EXCHANGE_BEAM_DATA)
        {
          std::cout<<"EXCHANGE_BEAM_DATA"<<std::endl;
          //m_nas_mm->send_wx_Mcontrol_Notify_Switch(70,0);
          m_nas_mm->send_wx_Mcontrol_Notify_Switch(70, 0, &pdu->msg[5], 1);
        }
        else if (pdu->msg[4] == EXCHANGE_BEAM_CALL)
        {
          std::cout<<"EXCHANGE_BEAM_CALL"<<std::endl;
          //m_nas_mm->send_wx_Mcontrol_Notify_Switch(70,1);
          m_nas_mm->send_wx_Mcontrol_Notify_Switch(70, 1, &pdu->msg[5], 1);
        }
        else if (pdu->msg[4] == EXCHANGE_BEAM_NOT_TASK)
        {
          std::cout<<"EXCHANGE_BEAM_NOT_TASK"<<std::endl;
          //m_nas_mm->send_wx_Mcontrol_Notify_Switch(70,0);
          m_nas_mm->send_wx_Mcontrol_Notify_Switch(70, 0, &pdu->msg[5], 1);
        }
      }

      break;

    default:

      break;
    }

    /*send message to ate infrom  res  */
    srsran::unique_byte_buffer_t nas_ate_tx = srsran::make_byte_buffer();
    if (nas_ate_tx == nullptr)
    {
      srsran::console("Couldn't allocate PDU in TC_MSG_ATE_NAS_CONTROL_RES \n");
      return false;
    }
    nas_ate_tx->N_bytes = 3;
    nas_ate_tx->msg[0] = 0xff;
    nas_ate_tx->msg[1] = 0x01;
    nas_ate_tx->msg[2] = TC_MSG_ATE_NAS_CONTROL_RES;

    // send to ate msg queue
 	srsran::console("send nas_msg to ate queue \n");
    cnw_adp.udp_.send_ate_msg(std::move(nas_ate_tx));

    return true;
  }

  bool cnw::handle_enb_rx_pdu(srsran::unique_byte_buffer_t pdu, uint16_t enb_id, uint16_t enb_ue_id, bool is_initial_msg)
  {
    std::cout << "--------------@@Enter the handle-enb-rx-pdu process@@------------" << std::endl;
    uint8_t pd, msg_type, sec_hdr_type;
    nas_5gs_msg nas_msg;
    msg_types message_type;
    // bool                         is_initial_msg;
    bool msg_encrypted = false;
    bool mac_valid = false;
    bool increase_ul_nas_cnt = true;
    nrmm_ctx_t *nrmm_ctx = NULL;
    nrcm_ctx_t *nrcm_ctx = NULL;
    sec_ctx_t *sec_ctx = NULL;

    if (SRSRAN_SUCCESS != nas_msg.unpack_outer_hdr(pdu))
    {
      srsran::console("------Failed to unpack outer hdr!.----- \n");
      m_cnw_logger.error("Failed to unpack outer hdr!");
      return false;
    }
    assert(nas_msg.hdr.extended_protocol_discriminator == nas_5gs_hdr::extended_protocol_discriminator_5gmm);

    /* Now use whether have saved enb_ue_id before. */
    nas_context *nas_ctx = find_nas_ctx_from_rnti(enb_ue_id);

    if ((nas_ctx == nullptr) && (is_initial_msg != 1))
    {
      m_cnw_logger.debug("--Not find the nas context and isn't initial_msg , so ignore this msg. rnti: %d \n", enb_ue_id);
      srsran::console("--Not find the nas context and isn't initial_msg , so ignore this msg. rnti: %d \n", enb_ue_id);
      return false;
    }

    /*cy add*/
    if (is_initial_msg == 1)
    {
      m_cnw_logger.debug("Received Initial UE Message.");
      srsran::console("-----------Received Initial UE Message.  RNTI id: %d\n", enb_ue_id);
    }
    else
    {
      m_cnw_logger.debug("Received Uplink NAS Transport Message. RNTI id: %d", enb_ue_id);
      srsran::console("Received Uplink NAS Transport Message. RNTI id: %d\n", enb_ue_id);
      nrmm_ctx_t *nrmm_ctx = &nas_ctx->nrmm_ctx;
      nrcm_ctx_t *nrcm_ctx = &nas_ctx->nrcm_ctx;
      sec_ctx_t *sec_ctx = &nas_ctx->m_sec_ctx;
    }

    /* 2. Security header handle */
    if (!is_initial_msg)
    {
      switch (nas_msg.hdr.security_header_type)
      {
      case nas_5gs_hdr::security_header_type_opts::plain_5gs_nas_message:
        srsran::console("Received Uplink plain_5gs_nas_message \n");
        break;
      case nas_5gs_hdr::security_header_type_opts::integrity_protected:
        mac_valid = m_nas_mm->integrity_check(pdu.get(), nas_ctx);
        if (!mac_valid)
        {
          std::cout << "-------------Not handling NAS message with integrity check error----------" << std::endl;
          m_cnw_logger.error("Not handling NAS message with integrity check error");
          return false;
        }
        break;
      case nas_5gs_hdr::security_header_type_opts::integrity_protected_and_ciphered:
        mac_valid = m_nas_mm->integrity_check(pdu.get(), nas_ctx);
        if (!mac_valid)
        {
          std::cout << "-------------Not handling NAS message with integrity check error----------" << std::endl;
          m_cnw_logger.error("Not handling NAS message with integrity check error");
          return false;
        }
        else
        {
          m_nas_mm->cipher_decrypt(pdu.get(), nas_ctx);
          msg_encrypted = true;
        }
        break;
      case nas_5gs_hdr::security_header_type_opts::integrity_protected_with_new_5G_nas_context:
        return false;
      case nas_5gs_hdr::security_header_type_opts::integrity_protected_and_ciphered_with_new_5G_nas_context:
        mac_valid = m_nas_mm->integrity_check(pdu.get(), nas_ctx);
        if (!mac_valid)
        {
          m_cnw_logger.error("Not handling NAS message with integrity check error");
          return false;
        }
        else
        {
          m_nas_mm->cipher_decrypt(pdu.get(), nas_ctx);
          msg_encrypted = true;
        }
        break;
      default:
        m_cnw_logger.error("Not handling NAS message with unkown security header");
        break;
      }
    }

    /* 3. Message body decoder */
    if (SRSRAN_SUCCESS != nas_msg.unpack(pdu))
    {
      m_cnw_logger.error("Failed to unpack!");
      srsran::console("Failed to unpack!\n");
      return false;
    }
    /* 3. message handler */
    message_type = nas_msg.hdr.message_type;
    
    //PCAP
    if(cnw_args.pcap_net.enable) {  
      if(nas_msg.hdr.security_header_type == nas_5gs_hdr::security_header_type_opts::plain_5gs_nas_message) {
        send_pcap_nas_pdu_to_enb(nas_pcap_ul, pdu->msg, pdu->N_bytes);
        if(msg_types::options::ul_nas_transport == message_type) {
          send_pcap_nas_pdu_to_enb(nas_pcap_ul, pdu->msg + 6, pdu->N_bytes - 6);
        }
      }
      else {
        if(nas_msg.hdr.spare_1_2_version == 0x00) {
          send_pcap_nas_pdu_to_enb(nas_pcap_ul, pdu->msg + 7, pdu->N_bytes - 7);
          if(msg_types::options::ul_nas_transport == message_type) {
            send_pcap_nas_pdu_to_enb(nas_pcap_ul, pdu->msg + 7 + 6, pdu->N_bytes -7 - 6);
          }
        }
        else {
          send_pcap_nas_pdu_to_enb(nas_pcap_ul, pdu->msg + 11, pdu->N_bytes - 11);
          if(msg_types::options::ul_nas_transport == message_type) {
            send_pcap_nas_pdu_to_enb(nas_pcap_ul, pdu->msg +11 + 6, pdu->N_bytes -11 - 6);
          }
        }
      } 
    } 

    switch (message_type)
    {
    case msg_types::options::registration_request:
      m_nas_mm->handle_registration_request(nas_msg.registration_request(), enb_id, enb_ue_id);
      break;
    case msg_types::options::registration_complete:
      m_nas_mm->handle_registration_complete(nas_msg.registration_complete(), enb_id, enb_ue_id);
      // if (nas_msg.hdr.security_header_type ==
      //         nas_5gs_hdr::security_header_type_opts::integrity_protected_and_ciphered &&
      //     mac_valid) {
      //   // m_nas_mm->handle_security_mode_complete(nas_msg.security_mode_complete(), enb_id, enb_ue_id);
      // } else {
      //   srsran::console("Registration Complete %s. Discard message.\n",
      //                   (mac_valid ? "not integrity protected" : "invalid integrity"));
      //   m_cnw_logger.warning("Registration Complete %s. Discard message.",
      //                        (mac_valid ? "not integrity protected" : "invalid integrity"));
      //   increase_ul_nas_cnt = false;
      // }
      break;

    case msg_types::options::deregistration_request_ue_originating:
      m_nas_mm->handle_deregistration_request_from_ue(nas_msg.deregistration_request_ue_originating());
      break;

    case msg_types::options::deregistration_accept_ue_terminated:
      m_nas_mm->handle_deregistration_accept_from_ue(enb_ue_id);
      break;

    case msg_types::options::service_request:
      m_nas_mm->handle_service_request(nas_msg.service_request(), enb_ue_id);

      break;

    case msg_types::options::configuration_update_complete:
      m_nas_mm->handle_configuration_update_complete();
      break;

    case msg_types::options::authentication_response:
      nas_ctx->m_sec_ctx.spare_1_2_version = nas_msg.hdr.spare_1_2_version; 
      m_nas_mm->handle_authentication_response(nas_msg.authentication_response(), enb_id, enb_ue_id);
      if (NULL != sec_ctx)
      {
        sec_ctx->ul_nas_count = 0;
        sec_ctx->dl_nas_count = 0;
        increase_ul_nas_cnt = false;
      }

      break;
    case msg_types::options::authentication_failure:
      m_nas_mm->handle_authentication_failure(nas_msg.authentication_failure(), enb_id, enb_ue_id);
      break;

    case msg_types::options::identity_response:
      m_nas_mm->handle_identity_response(nas_msg.identity_response(), enb_ue_id);
      break;

    case msg_types::options::security_mode_complete:
      /*2023-11-21*/
      m_nas_mm->handle_security_mode_complete(nas_msg.security_mode_complete(), enb_id, enb_ue_id);

      // 测试：完整性保护通过后，进出消息内容解析、处理
      //  mac_valid=1;
      //  if (nas_msg.hdr.security_header_type ==
      //          nas_5gs_hdr::security_header_type_opts::integrity_protected &&mac_valid)
      //  {
      //    m_nas_mm->handle_security_mode_complete(nas_msg.security_mode_complete(), enb_id, enb_ue_id);
      //  }
      //  else {
      //    // Security Mode Complete was not integrity protected
      //    srsran::console("Security Mode Complete %s. Discard message.\n",
      //                    (mac_valid ? "not integrity protected" : "invalid integrity"));
      //    m_cnw_logger.warning("Security Mode Complete %s. Discard message.",
      //                         (mac_valid ? "not integrity protected" : "invalid integrity"));
      //    increase_ul_nas_cnt = false;
      //  }
      break;

    case msg_types::options::security_mode_reject:
      m_nas_mm->handle_security_mode_reject(nas_msg.security_mode_reject(), enb_id, enb_ue_id);
      break;

    case msg_types::options::status_5gmm:

      break;

    case msg_types::options::ul_nas_transport:
      m_nas_mm->handle_ul_nas_transport(nas_msg.ul_nas_transport(), enb_ue_id);
      /*sm消息包含在此条消息中，处理后转发SMF*/
      break;

    case msg_types::options::active_test_mode_complete:
      cout << "---------active_test_mode_complete------" << endl;
      m_nas_mm->handle_test_loop_message(nas_msg.hdr, nas_ctx);
      break;

    case msg_types::options::close_ue_test_loop_complete:
      cout << "---------close_ue_test_loop_complete------" << endl;
      m_nas_mm->handle_test_loop_message(nas_msg.hdr, nas_ctx);
      break;

    default:
      m_cnw_logger.error("Unknown message type (%s) or not implemented", message_type.to_string());
      break;
    }

    if (increase_ul_nas_cnt == true && sec_ctx != NULL)
    {
      sec_ctx->ul_nas_count++;
    }

    return true;
  }

  /*cy 初始ue消息上行消息接口函数*/
  // void initial_ue(uint16_t rnti,  asn1::s1ap::rrc_establishment_cause_e     cause, srsran::unique_byte_buffer_t pdu, ue_capab_s  ue_capab)
  void cnw::initial_ue(uint16_t rnti, srsran::unique_byte_buffer_t pdu)
  {
    bool is_initial_msg = true;
    srsran::console("----------CNW Receive Initial UE-------\n");
    // cnw_pdu_t rx_pdu;
    // int enb_id = 0; //该参数是否是必要
    // 是否需要对其他没用的参数进行处理  asn1::s1ap::rrc_establishment_cause_e  cause  ue_capab_s  ue_capab

    for (int i = 0; i < (int)pdu->N_bytes; i++)
      srsran::console("0x%x\n", *(pdu->msg + i)); // 打印收到的数据

    handle_enb_rx_pdu(std::move(pdu), 0, rnti, is_initial_msg);
  }

  /*cy 非初始ue消息上行消息接口函数*/
  void cnw::write_pdu(uint16_t rnti, srsran::unique_byte_buffer_t pdu)
  {
    bool is_initial_msg = false;
    srsran::console("----------CNW Receive write_pdu-------\n");

    for (int i = 0; i < (int)pdu->N_bytes; i++)
      srsran::console("0x%x\n", *(pdu->msg + i)); // 打印收到的数据

    handle_enb_rx_pdu(std::move(pdu), 0, rnti, is_initial_msg);
  }

  /*cy*/
  void cnw::s_user_release(uint16_t rnti)
  {
    nas_context *n_ctx = find_nas_ctx_from_rnti(rnti);
    if (n_ctx == NULL)
    {
      srsran::console("cnw error, ue not find\n");
      return;
    }
    n_ctx->nrcm_ctx.state = nrcm_state_t::NRCM_STATE_IDLE;  
    
  }

  bool cnw::release_nas_ctx(uint16_t rnti)
  {
    nas_context *n_ctx = find_nas_ctx_from_rnti(rnti);
    if (n_ctx == NULL)
    {
      srsran::console("can't delete Ue context,ue not find\n");
      return false;
    }
    m_tmsi_to_nas_ctx.clear(); // the project only a user
    m_rnti_to_nas_ctx.clear();

    delete n_ctx;
    srsran::console("deleted UE context.\n");
    return true;
  }

  /*cy */
  void cnw::rrc_notify_nas_to_release(uint16_t rnti)
  {
    std::cout << " rrc->nas_to_notify_rrc_release(rnti) " << std::endl;
    m_nas_mm->send_nas_to_notify_rrc_release(rnti);
    m_nas_mm->send_switch_info_to_ate();

    if (!release_nas_ctx(rnti))
    {
      srsran::console("release nas ctx info failure\n");
    }
  }

  /*cy RRC收到smc完成后调用接口函数下发注册接受消息*/
  bool cnw::send_reg_or_service_accept(uint16_t rnti)
  {
    nas_context *nas_ctx = find_nas_ctx_from_rnti(rnti);
    if (nas_ctx == nullptr)
    {
      srsran::console("---------not found nas context--by-rnti----\n");
      return false;
    }

    if (nas_ctx->nrmm_ctx.reg_accept_msg_valid == true)
    { // 此时应该发送注册接受消息
      srsran::unique_byte_buffer_t nas_msg = srsran::make_byte_buffer();
      if (nas_msg == nullptr)
      {
        srsran::console("---------Couldn't allocate PDU in -send_reg_accept------\n");
        return false;
      }
      // nas_msg->N_bytes = nas_ctx->nrmm_ctx.reg_or_service_accept_msg_len;
      // memcpy(nas_msg->msg, nas_ctx->nrmm_ctx.reg_or_service_accept_msg, nas_msg->N_bytes);

      m_nas_mm->pack_registration_accept(nas_msg, nas_ctx);
      nas_ctx->m_sec_ctx.dl_nas_count++;
      m_nas_mm->send_mm_dl_msg(rnti, std::move(nas_msg));
      nas_ctx->nrmm_ctx.reg_accept_msg_valid = false;
      return true;
    }

    if (nas_ctx->nrmm_ctx.service_accept_msg_valid == true)
    { // 此时应该发送服务接受消息
      srsran::console("---------send rrc recon and service accept-----\n");
      srsran::unique_byte_buffer_t temp_nas_msg = srsran::make_byte_buffer();  
      if (temp_nas_msg == nullptr)
      {
        srsran::console("---------Couldn't allocate PDU in -send_reg_accept------\n");
        return false;
      }
      m_nas_mm->pack_service_accept(temp_nas_msg, nas_ctx);
      const srsran::unique_byte_buffer_t nas_msg = srsran::make_byte_buffer();
      if (nas_msg == nullptr)
      {
        srsran::console("---------Couldn't allocate PDU in -send_reg_accept------\n");
        return false;
      }
      // nas_msg->N_bytes = nas_ctx->nrmm_ctx.reg_or_service_accept_msg_len;
      // memcpy(nas_msg->msg, nas_ctx->nrmm_ctx.reg_or_service_accept_msg, nas_msg->N_bytes);
      nas_msg->N_bytes = temp_nas_msg->N_bytes;
      memcpy(nas_msg->msg, temp_nas_msg->msg, nas_msg->N_bytes);
      nas_ctx->nrmm_ctx.service_accept_msg_valid = false;

      srsran::const_byte_span service_accept_msg = srsran::make_span(nas_msg);
      printf("---service_accept_msg-- ");
      for (int i = 0; i < (int)service_accept_msg.size(); ++i)
      {
        printf(" %x ", *(service_accept_msg.data() + i));
      }
      printf("\n");

      // mm 触发重配过程 暂时打桩写死  此时应该对注册过程对建立的会话资源进行重配
      uint8_t qos = 2;
      uint16_t pdu_session_id = 1;
      nas_ctx->m_sec_ctx.dl_nas_count++;
      m_nas_mm->send_smm_notify_ue_erab_updates(rnti, qos, pdu_session_id, service_accept_msg, 0);

      return true;
    }

    return false;
  }

  bool cnw::read_db_file(std::string db_filename)
  {
    std::ifstream m_db_file;

    m_db_file.open(db_filename.c_str(), std::ifstream::in);
    if (!m_db_file.is_open())
    {
      return false;
    }
    m_cnw_logger.info("Opened DB file: %s", db_filename.c_str());
    srsran::console("Opened DB file: %s\n", db_filename.c_str());

    std::string line;
    bool set_default_supi = false;
    while (std::getline(m_db_file, line))
    {
      if (line[0] != '#' && line.length() > 0)
      {
        uint column_size = 10;
        std::vector<std::string> split = srsran::split_string(line, ',');
        if (split.size() != column_size)
        {
          m_cnw_logger.error("Error parsing UE database. Wrong number of columns in .csv");
          m_cnw_logger.error("Columns: %zd, Expected %d.", split.size(), column_size);

          srsran::console("\nError parsing UE database. Wrong number of columns in user database CSV.\n");
          srsran::console("Perhaps you are using an old user_db.csv?\n");
          srsran::console("See 'srsepc/user_db.csv.example' for an example.\n\n");
          return false;
        }
        std::unique_ptr<ue_ctx_t> ue_ctx = std::unique_ptr<ue_ctx_t>(new ue_ctx_t);
        ue_ctx->name = split[0];
        if (split[1] == std::string("xor"))
        {
          ue_ctx->algo = HSS_ALGO_XOR;
        }
        else if (split[1] == std::string("mil"))
        {
          ue_ctx->algo = HSS_ALGO_MILENAGE;
        }
        else
        {
          m_cnw_logger.error("Neither XOR nor MILENAGE configured.");
          return false;
        }
        ue_ctx->imsi = strtoull(split[2].c_str(), nullptr, 10);
        ue_ctx->supi = split[2];
        srsran::get_uint_vec_from_hex_str(split[3], ue_ctx->key, 16);
        if (split[4] == std::string("op"))
        {
          ue_ctx->op_configured = true;
          srsran::get_uint_vec_from_hex_str(split[5], ue_ctx->op, 16);
          srsran::compute_opc(ue_ctx->key, ue_ctx->op, ue_ctx->opc);
        }
        else if (split[4] == std::string("opc"))
        {
          ue_ctx->op_configured = false;
          srsran::get_uint_vec_from_hex_str(split[5], ue_ctx->opc, 16);
        }
        else
        {
          m_cnw_logger.error("Neither OP nor OPc configured.");
          return false;
        }
        srsran::get_uint_vec_from_hex_str(split[6], ue_ctx->amf, 2);
        srsran::get_uint_vec_from_hex_str(split[7], ue_ctx->sqn, 6);

        m_cnw_logger.debug("Added user from DB, IMSI: %015" PRIu64 "", ue_ctx->imsi);
        m_cnw_logger.debug(ue_ctx->key, 16, "User Key : ");
        if (ue_ctx->op_configured)
        {
          m_cnw_logger.debug(ue_ctx->op, 16, "User OP : ");
        }
        m_cnw_logger.debug(ue_ctx->opc, 16, "User OPc : ");
        m_cnw_logger.debug(ue_ctx->amf, 2, "AMF : ");
        m_cnw_logger.debug(ue_ctx->sqn, 6, "SQN : ");
        ue_ctx->qci = (uint16_t)strtol(split[8].c_str(), nullptr, 10);
        m_cnw_logger.debug("Default Bearer QCI: %d", ue_ctx->qci);

        if (!set_default_supi)
        {
          strcpy(m_default_supi, ue_ctx->supi.c_str());
          set_default_supi = true;
        }

        srsran::console("User Name: %s\n", ue_ctx->name.c_str());
        srsran::console("Algorithm: %s\n", split[1].c_str());
        srsran::console("IMSI(supi): %015" PRIu64 "\n", ue_ctx->imsi);

        // 打印二进制密钥
        srsran::console("User Key: ");
        for (int i = 0; i < 16; ++i)
        {
          srsran::console("0x%02x ", ue_ctx->key[i]);
        }
        srsran::console("\n");

        // 根据是否配置了OP，打印相关信息
        if (ue_ctx->op_configured)
        {
          srsran::console("User OP: ");
          for (int i = 0; i < 16; ++i)
          {
            srsran::console("0x%02x ", ue_ctx->op[i]);
          }
          srsran::console("\n");
        }
        else
        {
          srsran::console("User OPc: ");
          for (int i = 0; i < 16; ++i)
          {
            srsran::console("0x%02x ", ue_ctx->opc[i]);
          }
          srsran::console("\n");
        }

        // 打印AMF和SQN
        srsran::console("AMF: ");
        for (int i = 0; i < 2; ++i)
        {
          srsran::console("0x%02x ", ue_ctx->amf[i]);
        }
        srsran::console("\n");

        srsran::console("SQN: ");
        for (int i = 0; i < 6; ++i)
        {
          srsran::console("0x%02x ", ue_ctx->sqn[i]);
        }
        srsran::console("\n");

        srsran::console("Default Bearer QCI: %d\n", ue_ctx->qci);

        m_supi_to_ue_ctx.insert(std::make_pair(ue_ctx->imsi, std::move(ue_ctx)));
      }
    }

    if (m_db_file.is_open())
    {
      m_db_file.close();
    }

    return true;
  }

  nas_context *cnw::find_nas_ctx_from_tmsi_5g(uint32_t tmsi_5g)
  {
    std::map<uint32_t, nas_context *>::iterator iter = m_tmsi_to_nas_ctx.find(tmsi_5g);
    if (iter == m_tmsi_to_nas_ctx.end())
    {
      return NULL;
    }
    else
    {
      return iter->second;
    }
  }

  bool cnw::add_nas_ctx_to_tmsi_5g_map(nas_context *nas_ctx)
  {
    std::map<uint32_t, nas_context *>::iterator ctx_it = m_tmsi_to_nas_ctx.find(nas_ctx->nrcm_ctx.tmsi_5g);
    if (ctx_it != m_tmsi_to_nas_ctx.end())
    {
      m_cnw_logger.error("UE Context already exists. 5g_tmsi %015" PRIu64 "", nas_ctx->nrcm_ctx.tmsi_5g);
      return false;
    }

    m_tmsi_to_nas_ctx.insert(std::pair<uint32_t, nas_context *>(nas_ctx->nrcm_ctx.tmsi_5g, nas_ctx));
    m_cnw_logger.debug("Saved UE context corresponding to 5g_tmsi %015" PRIu64 "", nas_ctx->nrcm_ctx.tmsi_5g);
    return true;
  }

  /*rrc use rnti to get the 5g_tmsi*/
  bool cnw::rrc_get_tmsi_5g(uint16_t rnti, uint8_t tmsi_s_5g[])
  {
    nas_context *nas_ctx = find_nas_ctx_from_rnti(rnti);
    if (nas_ctx == nullptr)
    {
      srsran::console("---------not found nas context----rnti: %d\n", rnti);
      return false;
    }

    for (int i = 0; i < 6; ++i)
    {
      tmsi_s_5g[i] = nas_ctx->nrmm_ctx.tmsi_s_5g[i];
    }
    return true;
  }

  void cnw::release_enb(uint16_t enb_id)
  {
    std::map<uint16_t, enb_ctx_t *>::iterator it_ctx = m_active_enbs.find(enb_id);

    if (it_ctx == m_active_enbs.end())
    {
      m_cnw_logger.error("Could not find eNB to delete. Id: %d\n", enb_id);
      return;
    }

    m_cnw_logger.info("Deleting eNB context. eNB Id: 0x%x", enb_id);
    srsran::console("Deleting eNB context. eNB Id: 0x%x\n", enb_id);

    // TODO: Delete connected UEs ctx
    //  release_ues_ecm_ctx_in_enb(assoc_id);

    // Delete eNB
    delete it_ctx->second;
    m_active_enbs.erase(it_ctx);
    return;
  }

  void cnw::setup_enb(uint16_t enb_id, enb_ctx_t enb_ctx)
  {
    m_cnw_logger.info("Adding new eNB context. eNB ID %d", enb_ctx.enb_id);

    enb_ctx_t *enb_ptr = new enb_ctx_t;
    *enb_ptr = enb_ctx;
    m_active_enbs.insert(std::pair<uint16_t, enb_ctx_t *>(enb_ptr->enb_id, enb_ptr));
  }

  nas_context *cnw::find_nas_ctx_from_suci(uint64_t suci)
  {
    std::cout << "---------@@find_nas_ctx_from_suci@@----------" << std::endl;
    std::map<uint64_t, nas_context *>::iterator iter = m_suci_to_nas_ctx.find(suci);
    if (iter == m_suci_to_nas_ctx.end())
    {
      return NULL;
    }
    else
    {
      return iter->second;
    }
  }

  bool cnw::delete_ue_nas_ctx(srsepc::nas_guti guti) // 2024-4-2----
  {
    nas_context *n_ctx = find_nas_ctx_from_guti(guti);
    if (n_ctx == NULL)
    {
      m_cnw_logger.info("can't delete Ue context,ue not find");
      return false;
    }
    // delete UE context
    m_guti_to_nas_ctx.erase(guti);
    delete n_ctx;
    m_cnw_logger.info("delete UE context.");
    return true;
  }

  bool cnw::delete_ue_nas_ctx(uint32_t tmsi_5g) //-----------2024---4-2
  {
    std::cout << "delete UE context." << std::endl;
    nas_context *n_ctx = find_nas_ctx_from_tmsi_5g(tmsi_5g);
    if (n_ctx == NULL)
    {
      m_cnw_logger.info("can't delete Ue context,ue not find");
      return false;
    }
    m_tmsi_to_nas_ctx.erase(tmsi_5g);

    m_rnti_to_nas_ctx.erase(n_ctx->nrcm_ctx.rnti);

    delete n_ctx;
    std::cout << "delete UE context." << std::endl;
    m_cnw_logger.info("delete UE context.");
    return true;
  }

  nas_context *cnw::find_nas_ctx_from_guti(srsepc::nas_guti guti)
  {
    std::map<srsepc::nas_guti, nas_context *>::iterator iter = m_guti_to_nas_ctx.find(guti);
    if (iter == m_guti_to_nas_ctx.end())
    {
      return NULL;
    }
    else
    {
      return iter->second;
    }
  }

  nas_context *cnw::find_nas_ctx_from_rnti(uint32_t rnti)
  {
    std::cout << "---------6-----------------" << std::endl;
    std::map<uint32_t, nas_context *>::iterator iter = m_rnti_to_nas_ctx.find(rnti);
    if (iter == m_rnti_to_nas_ctx.end())
    {
      return NULL;
    }
    else
    {
      return iter->second;
    }
    std::cout << "---------7-----------------" << std::endl;
  }

  bool cnw::add_nas_ctx_to_suci_map(nas_context *nas_ctx)
  {
    std::map<uint64_t, nas_context *>::iterator ctx_it = m_suci_to_nas_ctx.find(nas_ctx->nrmm_ctx.suci);
    if (ctx_it != m_suci_to_nas_ctx.end())
    {
      m_cnw_logger.error("UE Context already exists. SUCI %015" PRIu64 "", nas_ctx->nrmm_ctx.suci);
      return false;
    }

    m_suci_to_nas_ctx.insert(std::pair<uint64_t, nas_context *>(nas_ctx->nrmm_ctx.suci, nas_ctx));
    m_cnw_logger.debug("Saved UE context corresponding to SUCI %015" PRIu64 "", nas_ctx->nrmm_ctx.suci);
    return true;
  }

  bool cnw::add_nas_ctx_to_guti_map(nas_context *nas_ctx)
  {
    std::map<srsepc::nas_guti, nas_context *>::iterator ctx_it = m_guti_to_nas_ctx.find(nas_ctx->nrmm_ctx.guti);
    if (ctx_it != m_guti_to_nas_ctx.end())
    {
      for (uint8_t i = 0; i < 10; i++)
      {
        m_cnw_logger.error("UE Context already exists. GUTI %015" PRIu64 "", nas_ctx->nrmm_ctx.guti.getGutiAddress(i));
      }

      return false;
    }

    m_guti_to_nas_ctx.insert(std::pair<nas_guti, nas_context *>(nas_ctx->nrmm_ctx.guti, nas_ctx));
    for (uint8_t i = 0; i < 10; i++)
    {
      m_cnw_logger.debug("Saved UE context corresponding to GUTI %015" PRIu64 "",
                         nas_ctx->nrmm_ctx.guti.getGutiAddress(i));
    }
    return true;
  }

  bool cnw::add_nas_ctx_to_rnti_map(nas_context *nas_ctx)
  {
    std::map<uint32_t, nas_context *>::iterator ctx_it = m_rnti_to_nas_ctx.find(nas_ctx->nrcm_ctx.rnti);
    if (ctx_it != m_rnti_to_nas_ctx.end())
    {
      m_cnw_logger.error("UE Context already exists. RNTI %015" PRIu64 "", nas_ctx->nrcm_ctx.rnti);
      return false;
    }

    m_rnti_to_nas_ctx.insert(std::pair<uint32_t, nas_context *>(nas_ctx->nrcm_ctx.rnti, nas_ctx));
    m_cnw_logger.debug("Saved UE context corresponding to RNTI %015" PRIu64 "", nas_ctx->nrcm_ctx.rnti);
    return true;
  }

  bool cnw::release_ue_nrcm_ctx(uint32_t enb_ue_id)
  {
    nas_context *nas_ctx = find_nas_ctx_from_rnti(enb_ue_id);
    if (nas_ctx == NULL)
    {
      m_cnw_logger.error("Cannot release UE ECM context, UE not found. MME-UE S1AP Id: %d\n", enb_ue_id);
      return false;
    }
    nrcm_ctx_t *nrcm_ctx = &nas_ctx->nrcm_ctx;

    // Release UE ECM context
    m_rnti_to_nas_ctx.erase(enb_ue_id);

    m_cnw_logger.info("Released UE ECM Context, RNTI ID: %d.\n", enb_ue_id);
    return true;
  }

  // bool cnw::delete_ue_ctx(uint64_t suci)
  // {
  //   nas_context* nas_ctx = find_nas_ctx_from_suci(suci);
  //   if (nas_ctx == NULL) {
  //     m_cnw_logger.info("Cannot delete UE context, UE not found. SUCI: %" PRIu64 "", suci);
  //     return false;
  //   }

  //   // Make sure to release ECM ctx
  //   if (nas_ctx->nrcm_ctx.rnti != 0) {
  //     // TODO: invoke rrc's functions.
  //     m_rnti_to_nas_ctx.erase(nas_ctx->nrcm_ctx.rnti);
  //   }

  //   // Make sure to release ECM ctx
  //   if (*nas_ctx->nrmm_ctx.guti.getGutiAddress(0) != 0) {
  //     m_guti_to_nas_ctx.erase(nas_ctx->nrmm_ctx.guti);
  //   }

  //   // Delete UE context
  //   m_suci_to_nas_ctx.erase(suci);
  //   delete nas_ctx;
  //   m_cnw_logger.info("Deleted UE Context. SUCI: %" PRIu64 "", suci);
  //   return true;
  // }

  bool cnw::check_guti_from_m_tmsi(uint8_t *m_tmsi, nas_context *nas_ctx)
  {
    return nas_ctx->compare_bytes_value(m_tmsi, nas_ctx->nrmm_ctx.guti.getGutiAddress(4), 6);
  }

  cnw_args_t *cnw::get_cnw_args()
  {
    return &cnw_args;
  }

  ue_ctx_t *cnw::get_ue_ctx(uint64_t supi)
  {
    // uint64_t supi_i = get_supi_from_suci(suci);
    uint64_t supi_i = supi;
    std::map<uint64_t, std::unique_ptr<ue_ctx_t>>::iterator ue_ctx_it = m_supi_to_ue_ctx.find(supi_i);
    if (ue_ctx_it == m_supi_to_ue_ctx.end())
    {
      m_cnw_logger.info("User not found. SUCI: %015" PRIu64 "", supi_i);
      return nullptr;
    }

    return ue_ctx_it->second.get();
  }

  void cnw::add_supi_to_suci_map(uint64_t suci, uint64_t supi)
  {
    m_suci_to_supi.insert(std::make_pair(suci, supi));
  }

  uint64_t cnw::get_supi_from_suci(uint64_t suci)
  {
    std::map<uint64_t, uint64_t>::iterator supi_it = m_suci_to_supi.find(suci);
    if (supi_it == m_suci_to_supi.end())
    {
      m_cnw_logger.info("User not found. SUCI: %015" PRIu64 "", suci);
      return 0;
    }
    return supi_it->second;
  }

  ue_ctx_t *cnw::get_ue_ctx_by_default_supi()
  {
    uint64_t imsi = strtoull(m_default_supi, nullptr, 10);
    std::map<uint64_t, std::unique_ptr<ue_ctx_t>>::iterator ue_ctx_it = m_supi_to_ue_ctx.find(imsi);
    if (ue_ctx_it == m_supi_to_ue_ctx.end())
    {
      m_cnw_logger.info("User not found. IMSI: %015" PRIu64 "", imsi);
      return nullptr;
    }

    return ue_ctx_it->second.get();
  }

  char *cnw::get_default_supi()
  {
    std::cout << "cnw::get_default_supi()" << std::endl;
    return m_default_supi;
  }

  void cnw::increment_ue_sqn(ue_ctx_t *ue_ctx)
  {
    std::cout << "--------increment_ue_sqn!!!!!!!!!!1-----" << std::endl;
    this->increment_sqn(ue_ctx->sqn, ue_ctx->sqn);
    m_cnw_logger.debug("Incremented SQN  -- SUCI: %015" PRIu64 "", ue_ctx->suci);
    m_cnw_logger.debug(ue_ctx->sqn, 6, "SQN: ");
  }

  void cnw::increment_sqn(uint8_t *sqn, uint8_t *next_sqn)
  {
    // The following SQN incrementation function is implemented according to 3GPP TS 33.102 version 11.5.1 Annex C
    uint64_t seq;
    uint64_t ind;
    uint64_t sqn64;

    sqn64 = 0;

    for (int i = 0; i < 6; i++)
    {
      sqn64 |= (uint64_t)sqn[i] << (5 - i) * 8;
    }

    seq = sqn64 >> LTE_FDD_ENB_IND_HE_N_BITS;
    ind = sqn64 & LTE_FDD_ENB_IND_HE_MASK;

    uint64_t nextseq;
    uint64_t nextind;
    uint64_t nextsqn;

    nextseq = (seq + 1) % LTE_FDD_ENB_SEQ_HE_MAX_VALUE;
    nextind = (ind + 1) % LTE_FDD_ENB_IND_HE_MAX_VALUE;
    nextsqn = (nextseq << LTE_FDD_ENB_IND_HE_N_BITS) | nextind;

    for (int i = 0; i < 6; i++)
    {
      next_sqn[i] = (nextsqn >> (5 - i) * 8) & 0xFF;
    }
    return;
  }

  uint32_t cnw::allocate_m_tmsi(uint64_t suci)
  {
    uint32_t m_tmsi = m_next_tmsi;
    m_next_tmsi = (m_next_tmsi + 1) % UINT32_MAX;

    m_tmsi_to_suci.insert(std::pair<uint32_t, uint64_t>(m_tmsi, suci));
    m_cnw_logger.debug("Allocated M-TMSI 0x%x to SUCI %015" PRIu64 ",", m_tmsi, suci);
    return m_tmsi;
  }

  pri_key_s cnw::find_hnet(uint8_t pki)
  {
    return hnet[pki];
  }

  // 此函数用于cnw转发到ims中
  void cnw::rrc_to_pcs_ims(uint16_t rnti, srsran::unique_byte_buffer_t pdu)
  {
    m_pcs_ims->pcs_handle_ulinformation(rnti, std::move(pdu));
  }

  bool cnw::send_pcap_nas_pdu_to_enb(enb_msg_type pcap_dir, const uint8_t* pdu, int pdu_len)
  {
    srsran::unique_byte_buffer_t enb_pdu = srsran::make_byte_buffer();
    enb_msg_header_t s1_header;
    s1_header.enb_id = cnw_adp.udp_.enb_id;
    s1_header.rnti = 70;
    s1_header.msg_type = pcap_dir;

    int len = sizeof(s1_header);
    enb_pdu->N_bytes = len;

    memcpy(enb_pdu->msg, &s1_header, len);
    memcpy(enb_pdu->msg + len, pdu, pdu_len);
    enb_pdu->N_bytes += pdu_len;

    cnw_adp.udp_.send_enb_msg(std::move(enb_pdu));

    return true;
  }

  void cnw::cnw_print_func(uint8_t *pdu, int pdu_len, string pdu_info)
  {
    if(!pdu_info.empty()) {
      cout << pdu_info << ":" << endl;
    }
    for(int i=0; i<pdu_len; i++) {
      printf("%x ", *(pdu+i));
      if(i/9 > 0 && i%9 ==0) cout << endl;
    }
    cout << endl;
  }

} // namespace srsepc
