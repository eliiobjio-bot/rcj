#include "srscnw/hdr/nas_sm.h"
#include "srscnw/hdr/nas_mm.h"
#include "srscnw/hdr/cnw.h"
#include "srscnw/hdr/nas_context.h"
#include "srsran/asn1/nas_5g_msg.h"
#include <cstring>
#include <inttypes.h>
#include <stdint.h>
#include <chrono>
using namespace srsran;
using namespace srsran::nas_5g;
namespace srsepc
{

  nas_sm *nas_sm::m_instance = NULL;
  pthread_mutex_t nas_sm_instance_mutex = PTHREAD_MUTEX_INITIALIZER;
  srsepc::nas_context nas_sm_context_info;

  nas_sm::nas_sm(){};

  nas_sm::~nas_sm()
  {
    return;
  }

  nas_sm *nas_sm::get_instance(void)
  {
    pthread_mutex_lock(&nas_sm_instance_mutex);
    if (NULL == m_instance)
    {
      m_instance = new nas_sm();
    }
    pthread_mutex_unlock(&nas_sm_instance_mutex);
    return (m_instance);
  }

  void nas_sm::cleanup(void)
  {
    pthread_mutex_lock(&nas_sm_instance_mutex);
    if (NULL != m_instance)
    {
      delete m_instance;
      m_instance = NULL;
    }
    pthread_mutex_unlock(&nas_sm_instance_mutex);
  }

  void nas_sm::init(adp *sm_adp_, const cnw_args_t sm_args_)
  {
    sm_adp = sm_adp_;
    m_mm = nas_mm::get_instance();
    sm_cnw = cnw::get_instance();
  }

  bool nas_sm::handle_pdu_session(srsran::unique_byte_buffer_t pdu, nas_context *nas_ctx, uint16_t enb_ue_id)
  {
    srsran::console("!------------handle-----SM--------massage-----------\n");
    nas_5gs_msg nas_msg;
    // bool is_ims=false;

    // srsran::unique_byte_buffer_t buf(pdu.get());
    SRSASN_CODE err = nas_msg.unpack_outer_hdr(pdu);

    if (err != SRSASN_SUCCESS)
    {
      m_nas_sm_logger.error("Failed to unpack outer hdr!");
      srsran::console("Failed to unpack outer hdr!\n");
      return false;
    }

    if (nas_ctx == nullptr)
    {
      srsran::console("----nullptr------\n");
    }

    assert(nas_msg.hdr.extended_protocol_discriminator == nas_5gs_hdr::extended_protocol_discriminator_5gsm);

    uint8_t a = 0;

    if (nas_msg.hdr.pdu_session_identity == 1)
    {
      a = 0;
      nas_ctx->nrsm_ctx.pdu_session_id[0] = nas_msg.hdr.pdu_session_identity;
    }
    else if (nas_msg.hdr.pdu_session_identity == 2)
    {
      a = 1;
      nas_ctx->nrsm_ctx.is_ims = true;
      printf("------nas_ctx->nrsm_ctx.is_ims=%d-----\n", nas_ctx->nrsm_ctx.is_ims);
      nas_ctx->nrsm_ctx.pdu_session_id[1] = nas_msg.hdr.pdu_session_identity;
    }
    else
    {
      printf("id is error");
    }
    // Message body decoder
    if (SRSRAN_SUCCESS != nas_msg.unpack(pdu))
    {
      m_nas_sm_logger.error("Failed to unpack!");
      srsran::console("Failed to unpack outer Message body!\n");
      return false;
    }
    msg_types message_type = nas_msg.hdr.message_type;
    if(sm_adp->udp_.TC_92_sm_release==true)
    {
      if(message_type == msg_types::options::pdu_session_modification_complete||message_type == msg_types::options::pdu_session_modification_reject)
      {
          srsran::unique_byte_buffer_t nas_buffer = srsran::make_byte_buffer();
          nas_buffer->init();
          TTCN_request_header(&nas_buffer);
          nas_buffer->msg[7] = 0x62;//pdu_session_modification_complete||reject
          nas_buffer->N_bytes = 8;
          // sm_adp->udp_.send_ttcn_info.try_push(std::move(send_TTCN_pdu));
          sm_adp->udp_.send_ttcn_msg_enb(std::move(nas_buffer));
      }
    }
    if (message_type == msg_types::options::pdu_session_establishment_request)
    {
      if (sm_adp->udp_.sm_pdu_accept==true)
      {
        nas_ctx->nrsm_ctx.pti[a] = nas_msg.hdr.procedure_transaction_identity;
        printf("1 nas_ctx->nrsm_ctx.pti[a]=%d\n", nas_ctx->nrsm_ctx.pti[a]);
        TTCN_handle_pdu_session_establishment_request(nas_msg.pdu_session_establishment_request(), nas_ctx, nas_ctx->nrsm_ctx.is_ims);
        nas_ctx->nrsm_ctx.is_accept = true;
      }
      else if (sm_adp->udp_.TC_91_sm_5_request == true)
      {
        nas_ctx->nrsm_ctx.pti[a] = nas_msg.hdr.procedure_transaction_identity;
        printf("2 nas_ctx->nrsm_ctx.pti[a]=%d\n", nas_ctx->nrsm_ctx.pti[a]);
        TTCN2_handle_pdu_session_establishment_request(nas_msg.pdu_session_establishment_request(), nas_ctx, nas_ctx->nrsm_ctx.is_ims);
        nas_ctx->nrsm_ctx.is_accept = true;
      }
      else if (sm_adp->udp_.sdap_nhdr_tran == true)
      {
        nas_ctx->nrsm_ctx.pti[a] = nas_msg.hdr.procedure_transaction_identity;
        printf("go to ttcn sdap request ");
        handle_pdu_session_establishment_request(nas_msg.pdu_session_establishment_request(), nas_ctx, nas_ctx->nrsm_ctx.is_ims);
        nas_ctx->nrsm_ctx.is_accept = true;
      }
      else
      {
        nas_ctx->nrsm_ctx.pti[a] = nas_msg.hdr.procedure_transaction_identity;
        printf("3 nas_ctx->nrsm_ctx.pti[a]=%d\n", nas_ctx->nrsm_ctx.pti[a]);
        handle_pdu_session_establishment_request(nas_msg.pdu_session_establishment_request(), nas_ctx, nas_ctx->nrsm_ctx.is_ims);
        nas_ctx->nrsm_ctx.is_accept = true;

        /*send message to ate infrom  ip address  */
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
        nas_ate_tx->msg[3] = 0x01;
        nas_ate_tx->msg[4] = 0x01;
        uint8_t data_ipv4[] = {1, 127, 11, 10};
        memcpy(nas_ate_tx->msg + nas_ate_tx->N_bytes, &data_ipv4, sizeof(data_ipv4));
        nas_ate_tx->N_bytes += sizeof(data_ipv4);
        // send to ate msg queue
        srsran::console("send nas_msg to ate queue \n");
        // sm_adp->udp_.send_ate_info.try_push(std::move(nas_ate_tx));
        sm_adp->udp_.send_ate_msg(std::move(nas_ate_tx));
      }
    }
    else if (message_type == msg_types::options::pdu_session_authentication_complete)
    {
      if (sm_adp->udp_.sm_pdu_accept==true)
      {
        bool is_accept = nas_ctx->nrsm_ctx.is_accept;
        TTCN_handle_pdu_authentication_complete(nas_msg.pdu_session_authentication_complete(), nas_ctx, is_accept, nas_ctx->nrsm_ctx.is_ims);
      }
      else if (sm_adp->udp_.sdap_nhdr_tran == true)
      {
        bool is_accept =  nas_ctx->nrsm_ctx.is_accept;
        handle_for_sdap_ttcn_complete(nas_msg.pdu_session_authentication_complete(),nas_ctx,is_accept,nas_ctx->nrsm_ctx.is_ims,enb_ue_id);
      }
      else
      {
        bool is_accept = nas_ctx->nrsm_ctx.is_accept;
        handle_pdu_authentication_complete(nas_msg.pdu_session_authentication_complete(), nas_ctx, is_accept, nas_ctx->nrsm_ctx.is_ims, enb_ue_id);
      }
    }
    else if (message_type == msg_types::options::pdu_session_modification_complete)
    {
      handle_pdu_modification_complete(nas_msg.pdu_session_modification_complete(), nas_ctx);
    }
    else if(message_type == msg_types::options::pdu_session_release_request)
    {
      printf("into pdu_session_release_request\n");
      if(sm_adp->udp_.TC_92_sm_release==true)
      {
        srsran::unique_byte_buffer_t nas_buffer = srsran::make_byte_buffer();
        TTCN_handle_pdu_release_request(nas_buffer,nas_ctx);
      }
    }
    else if(message_type == msg_types::options::pdu_session_release_complete)
    {
      if(sm_adp->udp_.TC_92_sm_release==true)
      {
        srsran::unique_byte_buffer_t nas_buffer = srsran::make_byte_buffer();
        TTCN_handle_pdu_release_complete(nas_buffer,nas_ctx);
      }
    }
    else
    {
      srsran::console("out");
    }
    return true;
  }

  bool nas_sm::handle_pdu_session_establishment_request(pdu_session_establishment_request_t &msg, nas_context *nas_ctx, bool is_ims)
  {
    srsran::console("--------handle_pdu_session_establishment_request!---------\n");
    // m_nas_sm_logger.info("handle_pdu_session_establishment_request");
    srsran::console("--------handle_pdu_session_establishment_request!-1------\n");
    int a = 0;
    if (is_ims)
    {
      a = 1;
    }
    nas_ctx->nrsm_ctx.integrity_protection.mbr_dl[a] = msg.integrity_protection_maximum_data_rate.max_data_rate_upip_downlink;
    nas_ctx->nrsm_ctx.integrity_protection.mbr_ul[a] = msg.integrity_protection_maximum_data_rate.max_data_rate_upip_uplink;
    // srsran::console("nas_ctx->nrsm_ctx.integrity_protection.mbr_dl[a]=%d\n",nas_ctx->nrsm_ctx.integrity_protection.mbr_dl[a]);
    // srsran::console("nas_ctx->nrsm_ctx.integrity_protection.mbr_ul[a]=%d\n",nas_ctx->nrsm_ctx.integrity_protection.mbr_ul[a]);

    if (msg.pdu_session_type.pdu_session_type_value)
    {
      nas_ctx->nrsm_ctx.ue_session_type[a] = msg.pdu_session_type.pdu_session_type_value;
      // srsran::console("nas_ctx->nrsm_ctx.ue_session_type[a]=%d\n",nas_ctx->nrsm_ctx.ue_session_type[a]);
    }
    if (msg.ssc_mode.ssc_mode_value)
    {
      nas_ctx->nrsm_ctx.ue_ssc_mode[a] = msg.ssc_mode.ssc_mode_value;
    }
    // srsran::console("nas_ctx->nrsm_ctx.ue_ssc_mode[a]=%d\n",msg.ssc_mode.ssc_mode_value);
    if (msg.always_on_pdu_session_requested_present)
    {
      nas_ctx->nrsm_ctx.apsi[a] = msg.always_on_pdu_session_requested.apsi;
    }
    // srsran::console("nas_ctx->nrsm_ctx.apsi[a]=%d\n",nas_ctx->nrsm_ctx.apsi[a]);

    srsran::unique_byte_buffer_t nas_tx = srsran::make_byte_buffer();
    /* 5.28 test  */
    // this->pack_pdu_session_authentication_command(nas_tx,nas_ctx,is_ims);

    uint8_t qfi = (int)QFI_cont_t::QFI_type_::options::QFI_2;
    uint16_t pdu_id = nas_ctx->nrsm_ctx.pdu_session_id[a];
    if (true)
    {
      if(sm_adp->udp_.sdap_nhdr_tran == true)
      {
        printf("pack_for_sdap_ttcn_accept");
        pack_for_sdap_ttcn_accept(nas_tx,nas_ctx,is_ims); 
        printf("sdap sm_write_dl_info");
        m_mm->sm_write_dl_info(std::move(nas_tx), nas_ctx->nrcm_ctx.rnti);
        if (pdu_id == 2)
              {
                auto start = std::chrono::high_resolution_clock::now();
                while (true)
                {
                  auto end = std::chrono::high_resolution_clock::now();
                  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                  if (duration.count() > 120)
                  {
                    break;
                  }
                }
              }
        // if (pdu_id == 1)
        // {
        //   if (tc_control == ENABLE_TC_MODE || sm_adp->udp_.ttcn_close_TC == true)
        //   {
        //     std::cout << "tc_control==ENABLE_TC_MODE||sm_adp->udp_.ttcn_close_TC==true" << std::endl;
        //     srsran::unique_byte_buffer_t test_loop = srsran::make_byte_buffer();
        //     m_mm->pack_close_ue_test_loop(test_loop, nas_ctx, 70);
        //     sleep(3);
        //   }
        // }
              //   auto start=std::chrono::high_resolution_clock::now();
              //   while(true){
              //   auto end=std::chrono::high_resolution_clock::now();
              //   auto duration=std::chrono::duration_cast<std::chrono::milliseconds>(end-start);
              //   if(duration.count()>2400){
              //     break;
              //   }
              // }
              printf("sdap sm_notify_ue_erab_updates");
              m_mm->sm_notify_ue_erab_updates(nas_ctx->nrcm_ctx.rnti, qfi, pdu_id, {}, 0);
        if (pdu_id == 1)
        {
          if (tc_control == ENABLE_TC_MODE || sm_adp->udp_.ttcn_close_TC == true)
          {
            std::cout << "tc_control==ENABLE_TC_MODE||sm_adp->udp_.ttcn_close_TC==true" << std::endl;
            srsran::unique_byte_buffer_t test_loop = srsran::make_byte_buffer();
            m_mm->pack_close_ue_test_loop(test_loop, nas_ctx, 70);
            sleep(3);
          }
        }
      }
      else
      {
        //normal process
              pack_pdu_session_establishment_accept(nas_tx, nas_ctx, is_ims);
              m_mm->sm_write_dl_info(std::move(nas_tx), nas_ctx->nrcm_ctx.rnti);

              if (pdu_id == 2)
              {
                auto start = std::chrono::high_resolution_clock::now();
                while (true)
                {
                  auto end = std::chrono::high_resolution_clock::now();
                  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                  if (duration.count() > 120)
                  {
                    break;
                  }
                }
              }
              // if (pdu_id == 1)
              // {
              //   if (tc_control == ENABLE_TC_MODE || sm_adp->udp_.ttcn_close_TC == true)
              //   {
              //     std::cout << "tc_control==ENABLE_TC_MODE||sm_adp->udp_.ttcn_close_TC==true" << std::endl;
              //     srsran::unique_byte_buffer_t test_loop = srsran::make_byte_buffer();
              //     m_mm->pack_close_ue_test_loop(test_loop, nas_ctx, 70);
              //     sleep(3);
              //   }
              // }
              //   auto start=std::chrono::high_resolution_clock::now();
              //   while(true){
              //   auto end=std::chrono::high_resolution_clock::now();
              //   auto duration=std::chrono::duration_cast<std::chrono::milliseconds>(end-start);
              //   if(duration.count()>2400){
              //     break;
              //   }
              // }
              std::cout << " sm_notify_ue_erab_updates " << std::endl;
              m_mm->sm_notify_ue_erab_updates(nas_ctx->nrcm_ctx.rnti, qfi, pdu_id, {}, 0);

              printf("__________qfi=%d,----------pdu_id=%d--\n", qfi, pdu_id);
              if (pdu_id == 1)
              {
                if (tc_control == ENABLE_TC_MODE || sm_adp->udp_.ttcn_close_TC == true)
                {
                  std::cout << "tc_control==ENABLE_TC_MODE||sm_adp->udp_.ttcn_close_TC==true" << std::endl;
                  srsran::unique_byte_buffer_t test_loop = srsran::make_byte_buffer();
                  m_mm->pack_close_ue_test_loop(test_loop, nas_ctx, 70);
                  sleep(3);
                }
              }
      }
      // pack_pdu_session_establishment_accept(nas_tx, nas_ctx, is_ims);
      // m_mm->sm_write_dl_info(std::move(nas_tx), nas_ctx->nrcm_ctx.rnti);

      // if (pdu_id == 2)
      // {
      //   auto start = std::chrono::high_resolution_clock::now();
      //   while (true)
      //   {
      //     auto end = std::chrono::high_resolution_clock::now();
      //     auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
      //     if (duration.count() > 120)
      //     {
      //       break;
      //     }
      //   }
      // }
      // if (pdu_id == 1)
      // {
      //   if (tc_control == ENABLE_TC_MODE || sm_adp->udp_.ttcn_close_TC == true)
      //   {
      //     std::cout << "tc_control==ENABLE_TC_MODE||sm_adp->udp_.ttcn_close_TC==true" << std::endl;
      //     srsran::unique_byte_buffer_t test_loop = srsran::make_byte_buffer();
      //    // m_mm->pack_close_ue_test_loop(test_loop, nas_ctx, 70);
      //     sleep(3);
      //   }
      // }
      // //   auto start=std::chrono::high_resolution_clock::now();
      // //   while(true){
      // //   auto end=std::chrono::high_resolution_clock::now();
      // //   auto duration=std::chrono::duration_cast<std::chrono::milliseconds>(end-start);
      // //   if(duration.count()>2400){
      // //     break;
      // //   }
      // // }
      // std::cout << " sm_notify_ue_erab_updates " << std::endl;
      // m_mm->sm_notify_ue_erab_updates(nas_ctx->nrcm_ctx.rnti, qfi, pdu_id, {}, 0);

      // printf("__________qfi=%d,----------pdu_id=%d--\n", qfi, pdu_id);
    }
    else
    {
      std::cout << "--------------------panduanpudsessionacc------------------" << std::endl;
      pack_pdu_session_authentication_result(nas_tx, nas_ctx);
      m_mm->sm_write_dl_info(std::move(nas_tx), nas_ctx->nrcm_ctx.rnti);
    }
    return true;
    /***************/

    // sm send message to mm

    // m_mm->test_printf();
    // m_mm->sm_mm_write_pdu(std::move(nas_tx), nas_ctx);
    // mm_interface->sm_write_dl_info(std::move(nas_tx), 79);

    //  sm_cnw->test();
    std::cout << "---------5-----------------" << std::endl;
    // sm_cnw->find_nas_ctx_from_rnti(nas_ctx->nrcm_ctx.rnti);
    std::cout << "---------8-----------------" << std::endl;
    // printf("-----------dl------------------: %d" , sm_cnw->find_nas_ctx_from_rnti(nas_ctx->nrcm_ctx.rnti)->m_sec_ctx.dl_nas_count);
    m_mm->sm_write_dl_info(std::move(nas_tx), nas_ctx->nrcm_ctx.rnti);
    std::cout << "---------9-----------------" << std::endl;
    return true;
  }

  bool nas_sm::pack_pdu_session_establishment_accept(srsran::unique_byte_buffer_t &nas_buffer, nas_context *nas_ctx, bool is_ims)
  {
    std::cout << "---------pack_pdu_session_establishment_accept------------" << std::endl;
    m_nas_sm_logger.info("packing pdu_session_establishment_accept");

    nas_5gs_msg nas_msg;
    int a = 0;
    if (is_ims)
    {
      a = 1;
    }

    // hand include id pti epd
    nas_msg.hdr.pdu_session_identity = nas_ctx->nrsm_ctx.pdu_session_id[a];
    nas_msg.hdr.extended_protocol_discriminator = nas_5gs_hdr::extended_protocol_discriminator_opts::extended_protocol_discriminator_5gsm;
    nas_msg.hdr.procedure_transaction_identity = nas_ctx->nrsm_ctx.pti[a];
    // message
    pdu_session_establishment_accept_t msg = nas_msg.set_pdu_session_establishment_accept();
    nas_msg.hdr.message_type.value = msg_types::pdu_session_establishment_accept;
    pdu_session_establishment_accept_t pdu_sess_esta_accept = nas_msg.pdu_session_establishment_accept();
    // printf("-----------------------\n");
    nas_msg.pdu_session_establishment_accept().selected_pdu_session_type.pdu_session_type_value = nas_ctx->convert_pdu_type(nas_ctx->nrsm_ctx.ue_session_type[a]);
    // printf("-----------------------\n");
    nas_msg.pdu_session_establishment_accept().selected_ssc_mode.ssc_mode_value = nas_ctx->convert_ssc_mode(nas_ctx->nrsm_ctx.ue_ssc_mode[a]);
    // ambr?qos?
    qos_rule_t qos1;
    qos1.DQR = qos_rule_t::DQR_type_::options::default_QoS_rule;
    qos1.QRI = qos_rule_t::QRI_type_::options::QRI_9;
    qos1.Rule_operation_code = qos_rule_t::Rule_operation_code_type_::options::Create_new_QoS_rule;

    qos1.packet_filter_num = 1;
    qos1.packet_filter.Qos_rules_precedence = 0;
    qos1.packet_filter.pacret_filter_type2.packet_filter_direction =
        pacret_filter_type2_t::packet_filter_direction_type_::options::bidirectional;
    qos1.packet_filter.pacret_filter_type2.packet_filter_id = 0;

    qos1.packet_filter.pacret_filter_type2.pf_cont.pf_content = {0x01};
    if(a==0)
    {
      qos1.packet_filter.QFI_cont.QFI=QFI_cont_t::QFI_type_::QFI_2;
    }
    if(a==1)
    {
      qos1.packet_filter.QFI_cont.QFI=QFI_cont_t::QFI_type_::QFI_1;
    }
    qos1.packet_filter.QFI_cont.Segregation=QFI_cont_t::Segregation_type_::Segregation_not_requested; 

    nas_msg.pdu_session_establishment_accept().authorized__qo_s_rules.qos_rules.push_back(qos1);

    nas_msg.pdu_session_establishment_accept().session_ambr.unit_session_ambr_for_downlink =
        session_ambr_t::unit_session_AMBR_type_::options::inc_by_256_kbps;
    nas_msg.pdu_session_establishment_accept().session_ambr.unit_session_ambr_for_uplink =
        session_ambr_t::unit_session_AMBR_type_::options::inc_by_256_kbps;
    nas_msg.pdu_session_establishment_accept().session_ambr.session_ambr_for_downlink = 4;
    nas_msg.pdu_session_establishment_accept().session_ambr.session_ambr_for_uplink = 4;

    // eap
    //  eap
    nas_msg.pdu_session_establishment_accept().eap_message_present = true;
    nas_msg.pdu_session_establishment_accept().eap_message.eap_message = {0x03, 0x00, 0x00, 0x08, 0x32, 0x00, 0x00, 0x00};
    nas_msg.pdu_session_establishment_accept().pdu_address_present = true;
    if (a == 0)
    {
      nas_msg.pdu_session_establishment_accept().pdu_address.ipv4 = {1, 127, 11, 10};
    }
    if (a == 1)
    {
      nas_msg.pdu_session_establishment_accept().pdu_address.ipv4 = {3, 168, 192, 10};
    }

    // allways on
    nas_msg.pdu_session_establishment_accept().always_on_pdu_session_indication_present = true;
    nas_msg.pdu_session_establishment_accept().always_on_pdu_session_indication.apsr = true;


    // printf("msg=%d\n",nas_msg);
    nas_msg.pdu_session_establishment_accept().authorized__qo_s_flow_descriptions_present = true;
    qo_s_flow_description_t qo_s_flow_description1;
    if (!is_ims)
    {
      printf("first pdu accept\n");
      qo_s_flow_description1.QFI.value = qo_s_flow_description_t::QFI_type_::options::QFI_2;
      qo_s_flow_description1.operation_code.value = qo_s_flow_description_t::operation_code_type_::options::Create_new_QoS_flow_description;
      qo_s_flow_description1.parameters_num = 1;
      qo_s_flow_description1.E.value = qo_s_flow_description_t::E_type_::parameters_list_is_included;
      QI5_parameter_contents_t parameter_content1;
      parameter_content1.parameter_id1.value = QI5_parameter_contents_t::parameter_id_type_::options::QI5;
      parameter_content1.QI5.value = QI5_parameter_contents_t::QI5_type_::QI5_9;
      qo_s_flow_description1.parameters_list.parameters1.push_back(parameter_content1);
      nas_msg.pdu_session_establishment_accept().authorized__qo_s_flow_descriptions.qo_s_flow_descriptions.push_back(qo_s_flow_description1);
    }
    if (is_ims)
    {
      printf("second pdu accept\n");
      qo_s_flow_description1.QFI.value = qo_s_flow_description_t::QFI_type_::options::QFI_1;
      qo_s_flow_description1.operation_code.value = qo_s_flow_description_t::operation_code_type_::options::Create_new_QoS_flow_description;
      qo_s_flow_description1.parameters_num = 1;
      qo_s_flow_description1.E.value = qo_s_flow_description_t::E_type_::parameters_list_is_included;
      QI5_parameter_contents_t parameter_content1;
      parameter_content1.parameter_id1.value = QI5_parameter_contents_t::parameter_id_type_::options::QI5;
      parameter_content1.QI5.value = QI5_parameter_contents_t::QI5_type_::QI5_5;
      qo_s_flow_description1.parameters_list.parameters1.push_back(parameter_content1);
      nas_msg.pdu_session_establishment_accept().authorized__qo_s_flow_descriptions.qo_s_flow_descriptions.push_back(qo_s_flow_description1);
    }
    printf("nas_buffer->msg.accept: ");
    for (uint32_t i = 0; i < nas_buffer->N_bytes; ++i)
    {
      printf("  0x%x", nas_buffer->msg[i]);
    }
    printf("\n");
    SRSASN_CODE err = nas_msg.pack(nas_buffer);
    if (err != SRSASN_SUCCESS)
    {
      m_nas_sm_logger.error("Error packing pdu_session_establishment_accept");
      srsran::console("Error packing pdu_session_establishment_accept\n");
      return false;
    }
    return true;
  }

  bool nas_sm::pack_pdu_session_authentication_command(srsran::unique_byte_buffer_t &nas_buffer, nas_context *nas_ctx, bool is_ims)
  {

    // m_nas_sm_logger.info("pack pdu_session_authentication_command");
    srsran::console("packing pdu_session_authentication_command\n");
    nas_5gs_msg nas_msg;
    int a = 0;
    if (is_ims)
    {
      a = 1;
    }
    nas_msg.hdr.extended_protocol_discriminator = nas_5gs_hdr::extended_protocol_discriminator_5gsm;

    nas_msg.hdr.pdu_session_identity = nas_ctx->nrsm_ctx.pdu_session_id[a];
    // nas_msg.hdr.pdu_session_identity =1;

    nas_msg.hdr.procedure_transaction_identity = nas_ctx->nrsm_ctx.pti[a];

    nas_msg.hdr.message_type.value = msg_types::pdu_session_authentication_command;
    pdu_session_authentication_command_t msg = nas_msg.set_pdu_session_authentication_command();
    // std::cout<<"---------1-----------------"<<std::endl;
    printf("nas_msg.hdr.extended_protocol_discriminator= %d\n", nas_msg.hdr.extended_protocol_discriminator);
    printf(" nas_msg.hdr.pdu_session_identity =%d\n", nas_msg.hdr.pdu_session_identity);
    printf("nas_msg.hdr.procedure_transaction_identity=%d\n", nas_msg.hdr.procedure_transaction_identity);
    printf("nas_msg.hdr.message_type.value =%d\n", nas_msg.hdr.message_type.value);

    nas_msg.pdu_session_authentication_command().eap_message.eap_message = {0x01, 0x00, 0x00, 0x44, 0x32, 0x01, 0x00, 0x00, 0x01, 0x05, 0x00, 0x00, 0x23, 0xc4, 0xc2, 0x17, 0x5c, 0x36, 0x5a, 0xc4, 0x31, 0x5b, 0x40, 0x5b, 0xd7, 0x64, 0x6a, 0x43, 0x02, 0x05, 0x00, 0x00, 0x54, 0xa3, 0x63, 0x71, 0xa9, 0xce, 0x80, 0x00, 0x2c, 0xfa, 0x4d, 0x3f, 0xe7, 0x09, 0x41, 0x1d, 0x0b, 0x05, 0x00, 0x00, 0x48, 0x9e, 0x8d, 0xcc, 0x00, 0xb6, 0x53, 0xf5, 0xcc, 0x75, 0x6b, 0x96, 0xd6, 0xa1, 0x42};

    // std::cout<<"---------2-----------------"<<std::endl;
    SRSASN_CODE err = nas_msg.pack(nas_buffer);
    if (err != SRSASN_SUCCESS)
    {
      m_nas_sm_logger.error("Error packing pdu_session_authentication_command");
      srsran::console("Error packing pdu_session_authentication_command\n");
      return false;
    }
    // std::cout<<"---------3-----------------"<<std::endl;
    printf("nas_buffer->msg: ");
    for (uint32_t i = 0; i < nas_buffer->N_bytes; ++i)
    {
      printf("  0x%x", nas_buffer->msg[i]);
    }
    printf("\n");
    // std::cout<<"---------4-----------------"<<std::endl;
    return true;
  }

  bool nas_sm::handle_pdu_authentication_complete(pdu_session_authentication_complete_t &msg, nas_context *nas_ctx, bool is_accept, bool is_ims, uint16_t enb_ue_id)
  {
    srsran::console("------handle pdu_session_authentication_complete----------\n");
    m_nas_sm_logger.info("handle pdu_session_authentication_complete");
    int a = 0;
    if (is_ims)
    {
      a = 1;
    }

    srsran::unique_byte_buffer_t nas_tx = srsran::make_byte_buffer();
    // srsran::const_byte_span nas_pdu_ref = srsran::make_byte_buffer();
    uint8_t qfi = (int)QFI_cont_t::QFI_type_::options::QFI_2;
    uint16_t pdu_id = nas_ctx->nrsm_ctx.pdu_session_id[a];
    if (is_accept == true)
    {

      pack_pdu_session_establishment_accept(nas_tx, nas_ctx, is_ims);
      m_mm->sm_write_dl_info(std::move(nas_tx), nas_ctx->nrcm_ctx.rnti);

      if (pdu_id == 2)
      {
        auto start = std::chrono::high_resolution_clock::now();
        while (true)
        {
          auto end = std::chrono::high_resolution_clock::now();
          auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
          if (duration.count() > 120)
          {
            break;
          }
        }
      }
      if (pdu_id == 1)
      {
        if (tc_control == ENABLE_TC_MODE || sm_adp->udp_.ttcn_close_TC == true)
        {
          std::cout << "tc_control==ENABLE_TC_MODE||sm_adp->udp_.ttcn_close_TC==true" << std::endl;
          srsran::unique_byte_buffer_t test_loop = srsran::make_byte_buffer();
          m_mm->pack_close_ue_test_loop(test_loop, nas_ctx, enb_ue_id);
          // sleep(1);
        }
      }
      //   auto start=std::chrono::high_resolution_clock::now();
      //   while(true){
      //   auto end=std::chrono::high_resolution_clock::now();
      //   auto duration=std::chrono::duration_cast<std::chrono::milliseconds>(end-start);
      //   if(duration.count()>2400){
      //     break;
      //   }
      // }
      m_mm->sm_notify_ue_erab_updates(nas_ctx->nrcm_ctx.rnti, qfi, pdu_id, {}, 0);

      printf("__________qfi=%d,----------pdu_id=%d--\n", qfi, pdu_id);
    }
    else
    {
      std::cout << "--------------------panduanpudsessionacc------------------" << std::endl;
      pack_pdu_session_authentication_result(nas_tx, nas_ctx);
      m_mm->sm_write_dl_info(std::move(nas_tx), nas_ctx->nrcm_ctx.rnti);
    }
    return true;
  }

  bool nas_sm::pack_pdu_session_authentication_result(srsran::unique_byte_buffer_t &nas_buffer, nas_context *nas_ctx)
  {
    m_nas_sm_logger.info("pack pdu_session_authentication_result");
    srsran::console("packing pdu_session_authentication_result\n");

    // eap message?
    nas_5gs_msg nas_msg;
    pdu_session_authentication_result_t pdu_sess_auth_res = nas_msg.set_pdu_session_authentication_result();
    nas_msg.pdu_session_authentication_result().eap_message_present = true;
    nas_msg.pdu_session_authentication_result().eap_message.eap_message = {0x01, 0x00, 0x00, 0x44, 0x32, 0x01, 0x00, 0x00, 0x01, 0x05, 0x00, 0x00, 0x23, 0xc4, 0xc2, 0x17, 0x5c, 0x36, 0x5a, 0xc4, 0x31, 0x5b, 0x40, 0x5b, 0xd7, 0x64, 0x6a, 0x43, 0x02, 0x05, 0x00, 0x00, 0x54, 0xa3, 0x63, 0x71, 0xa9, 0xca, 0x80, 0x00, 0x2c, 0xfa, 0x4d, 0x3f, 0xe7, 0x09, 0x41, 0x1d, 0x0b, 0x05, 0x00};

    SRSASN_CODE err = nas_msg.pack(nas_buffer);
    if (err != SRSASN_SUCCESS)
    {
      m_nas_sm_logger.error("Error packing pdu_session_authentication_result");
      srsran::console("Error packing pdu_session_authentication_result\n");
      return false;
    }
    return true;
  }

  // 0511 zhj modify
  bool nas_sm::pack_pdu_session_modification_command(srsran::unique_byte_buffer_t &nas_buffer, nas_context *nas_ctx, bool is_ims)
  {
    m_nas_sm_logger.info("pack pdu_session_modification_command");
    srsran::console("packing pdu_session_modification_command\n");

    ims_context *ims_ctx = sm_cnw->m_pcs_ims->find_ims_ctx_from_rnti(nas_ctx->nrcm_ctx.rnti);

    nas_5gs_msg nas_msg;
    int a = 0;
    if (is_ims)
    {
      a = 1;
    }

    nas_msg.hdr.extended_protocol_discriminator = nas_5gs_hdr::extended_protocol_discriminator_opts::extended_protocol_discriminator_5gsm;
    nas_msg.hdr.pdu_session_identity = nas_ctx->nrsm_ctx.pdu_session_id[a];
    nas_msg.hdr.procedure_transaction_identity = 0;

    pdu_session_modification_command_t msg = nas_msg.set_pdu_session_modification_command();
    nas_msg.hdr.message_type.value = msg_types::pdu_session_modification_command;

    // ims_ctx->ims_ctx.call_code_rate=2;//1:2.4 ; 2:4.8
    if (ims_ctx->ims_ctx.call_code_rate == 1)
    {

      nas_msg.pdu_session_modification_command().authorized__qo_s_rules_present = true;
      qos_rule_t qos1;
      qos1.DQR = qos_rule_t::DQR_type_::options::no_default_QoS_rule;
      qos1.QRI = qos_rule_t::QRI_type_::options::QRI_1;
      qos1.Rule_operation_code = qos_rule_t::Rule_operation_code_type_::options::Create_new_QoS_rule;
      qos1.packet_filter_num = 1;

      qos1.packet_filter.Qos_rules_precedence = 0x0b;
      qos1.packet_filter.pacret_filter_type2.packet_filter_direction =
          pacret_filter_type2_t::packet_filter_direction_type_::options::bidirectional;
      qos1.packet_filter.pacret_filter_type2.packet_filter_id = 1;

      qos1.packet_filter.pacret_filter_type2.pf_cont.pf_content = {0x10, 0xca, 0x6c, 0x16, 0x05, 0xff, 0xff, 0xff, 0xff};
      qos1.packet_filter.QFI_cont.QFI = QFI_cont_t::QFI_type_::options::QFI_3;
      qos1.packet_filter.QFI_cont.Segregation = QFI_cont_t::Segregation_type_::options::Segregation_not_requested;

      nas_msg.pdu_session_modification_command().authorized__qo_s_rules.qos_rules.push_back(qos1);

      nas_msg.pdu_session_modification_command().authorized__qo_s_flow_descriptions_present = true;

      qo_s_flow_description_t qo_s_flow_description1;
      qo_s_flow_description1.QFI.value = qo_s_flow_description_t::QFI_type_::options::QFI_3;
      qo_s_flow_description1.operation_code.value = qo_s_flow_description_t::operation_code_type_::options::Create_new_QoS_flow_description;
      qo_s_flow_description1.parameters_num = 1;
      qo_s_flow_description1.E.value = qo_s_flow_description_t::E_type_::parameters_list_is_included;

      QI5_parameter_contents_t parameter_content1;
      parameter_content1.parameter_id1.value = QI5_parameter_contents_t::parameter_id_type_::options::QI5;
      parameter_content1.QI5.value = QI5_parameter_contents_t::QI5_type_::QI5_1;

      // GFBR_uplink_parameter_contents_t parameter_content2;
      // parameter_content2.parameter_id2.value=GFBR_uplink_parameter_contents_t::parameter_id_type_::options::GFBR_uplink;
      // parameter_content2.content.guaranteed_flow.value=content_t::guaranteed_flow_type_::options::value_1_Kbps;
      // parameter_content2.content.guaranteed_flow_bit_rate1=0;
      // parameter_content2.content.guaranteed_flow_bit_rate2=1;

      // GFBR_uplink_parameter_contents_t parameter_content3;
      // parameter_content3.parameter_id2.value=GFBR_uplink_parameter_contents_t::parameter_id_type_::options::GFBR_downlink;
      // parameter_content3.content.guaranteed_flow.value=content_t::guaranteed_flow_type_::options::value_256_Kbps;
      // parameter_content3.content.guaranteed_flow_bit_rate1=0;
      // parameter_content3.content.guaranteed_flow_bit_rate2=2;

      // GFBR_uplink_parameter_contents_t parameter_content4;
      // parameter_content4.parameter_id2.value=GFBR_uplink_parameter_contents_t::parameter_id_type_::options::MFBR_uplink;
      // parameter_content4.content.guaranteed_flow.value=content_t::guaranteed_flow_type_::options::value_4_Mbps;
      // parameter_content4.content.guaranteed_flow_bit_rate1=0;
      // parameter_content4.content.guaranteed_flow_bit_rate2=3;

      // GFBR_uplink_parameter_contents_t parameter_content5;
      // parameter_content5.parameter_id2.value=GFBR_uplink_parameter_contents_t::parameter_id_type_::options::MFBR_downlink;
      // parameter_content5.content.guaranteed_flow.value=content_t::guaranteed_flow_type_::options::value_16_Mbps;
      // parameter_content5.content.guaranteed_flow_bit_rate1=0;
      // parameter_content5.content.guaranteed_flow_bit_rate2=4;

      qo_s_flow_description1.parameters_list.parameters1.push_back(parameter_content1);
      // qo_s_flow_description1.parameters_list.parameters2.push_back(parameter_content2);
      // qo_s_flow_description1.parameters_list.parameters2.push_back(parameter_content3);
      // qo_s_flow_description1.parameters_list.parameters2.push_back(parameter_content4);
      // qo_s_flow_description1.parameters_list.parameters2.push_back(parameter_content5);

      nas_msg.pdu_session_modification_command().authorized__qo_s_flow_descriptions.qo_s_flow_descriptions.push_back(qo_s_flow_description1);

      SRSASN_CODE err = nas_msg.pack(nas_buffer);
      if (err != SRSASN_SUCCESS)
      {
        m_nas_sm_logger.error("Error packing pdu_session_modification_command");
        srsran::console("Error packing pdu_session_modification_command\n");
        return false;
      }

      printf("pdu_session_modification_command nas_buffer->msg: ");
      for (uint32_t i = 0; i < nas_buffer->N_bytes; ++i)
      {
        printf("  %x", nas_buffer->msg[i]);
      }
      printf("\n");
      // srsran::console("sm_msg->N_bytes : %d\n",nas_buffer->N_bytes);
    }
    else if (ims_ctx->ims_ctx.call_code_rate == 2)
    {
      m_nas_sm_logger.info("pack pack_pdu_session_modification_command_4_8k");
      srsran::console("packing pack_pdu_session_modification_command_4_8k\n");

      nas_msg.pdu_session_modification_command().authorized__qo_s_rules_present = true;
      qos_rule_t qos1;
      qos1.DQR = qos_rule_t::DQR_type_::options::no_default_QoS_rule;
      qos1.QRI = qos_rule_t::QRI_type_::options::QRI_3;
      qos1.Rule_operation_code = qos_rule_t::Rule_operation_code_type_::options::Create_new_QoS_rule;
      qos1.packet_filter_num = 1;

      qos1.packet_filter.Qos_rules_precedence = 0x03;
      qos1.packet_filter.pacret_filter_type2.packet_filter_direction =
          pacret_filter_type2_t::packet_filter_direction_type_::options::bidirectional;
      qos1.packet_filter.pacret_filter_type2.packet_filter_id = 2;

      qos1.packet_filter.pacret_filter_type2.pf_cont.pf_content = {0x10, 0xca, 0x6c, 0x16, 0x05, 0xff, 0xff, 0xff, 0xff};
      qos1.packet_filter.QFI_cont.QFI = QFI_cont_t::QFI_type_::options::QFI_3;
      qos1.packet_filter.QFI_cont.Segregation = QFI_cont_t::Segregation_type_::options::Segregation_not_requested;

      nas_msg.pdu_session_modification_command().authorized__qo_s_rules.qos_rules.push_back(qos1);

      nas_msg.pdu_session_modification_command().authorized__qo_s_flow_descriptions_present = true;

      qo_s_flow_description_t qo_s_flow_description1;
      qo_s_flow_description1.QFI.value = qo_s_flow_description_t::QFI_type_::options::QFI_3;
      qo_s_flow_description1.operation_code.value = qo_s_flow_description_t::operation_code_type_::options::Create_new_QoS_flow_description;
      qo_s_flow_description1.parameters_num = 5;
      qo_s_flow_description1.E.value = qo_s_flow_description_t::E_type_::parameters_list_is_included;

      QI5_parameter_contents_t parameter_content1;
      parameter_content1.parameter_id1.value = QI5_parameter_contents_t::parameter_id_type_::options::QI5;
      parameter_content1.QI5.value = QI5_parameter_contents_t::QI5_type_::QI5_1;

      GFBR_uplink_parameter_contents_t parameter_content2;
      parameter_content2.parameter_id2.value = GFBR_uplink_parameter_contents_t::parameter_id_type_::options::GFBR_uplink;
      parameter_content2.content.guaranteed_flow.value = content_t::guaranteed_flow_type_::options::value_1_Kbps;
      parameter_content2.content.guaranteed_flow_bit_rate1 = 0;
      parameter_content2.content.guaranteed_flow_bit_rate2 = 1;

      GFBR_uplink_parameter_contents_t parameter_content3;
      parameter_content3.parameter_id2.value = GFBR_uplink_parameter_contents_t::parameter_id_type_::options::GFBR_downlink;
      parameter_content3.content.guaranteed_flow.value = content_t::guaranteed_flow_type_::options::value_256_Kbps;
      parameter_content3.content.guaranteed_flow_bit_rate1 = 0;
      parameter_content3.content.guaranteed_flow_bit_rate2 = 2;

      GFBR_uplink_parameter_contents_t parameter_content4;
      parameter_content4.parameter_id2.value = GFBR_uplink_parameter_contents_t::parameter_id_type_::options::MFBR_uplink;
      parameter_content4.content.guaranteed_flow.value = content_t::guaranteed_flow_type_::options::value_4_Mbps;
      parameter_content4.content.guaranteed_flow_bit_rate1 = 0;
      parameter_content4.content.guaranteed_flow_bit_rate2 = 3;

      GFBR_uplink_parameter_contents_t parameter_content5;
      parameter_content5.parameter_id2.value = GFBR_uplink_parameter_contents_t::parameter_id_type_::options::MFBR_downlink;
      parameter_content5.content.guaranteed_flow.value = content_t::guaranteed_flow_type_::options::value_16_Mbps;
      parameter_content5.content.guaranteed_flow_bit_rate1 = 0;
      parameter_content5.content.guaranteed_flow_bit_rate2 = 4;

      qo_s_flow_description1.parameters_list.parameters1.push_back(parameter_content1);
      qo_s_flow_description1.parameters_list.parameters2.push_back(parameter_content2);
      qo_s_flow_description1.parameters_list.parameters2.push_back(parameter_content3);
      qo_s_flow_description1.parameters_list.parameters2.push_back(parameter_content4);
      qo_s_flow_description1.parameters_list.parameters2.push_back(parameter_content5);

      nas_msg.pdu_session_modification_command().authorized__qo_s_flow_descriptions.qo_s_flow_descriptions.push_back(qo_s_flow_description1);

      SRSASN_CODE err = nas_msg.pack(nas_buffer);
      if (err != SRSASN_SUCCESS)
      {
        m_nas_sm_logger.error("Error packing pdu_session_modification_command");
        srsran::console("Error packing pdu_session_modification_command\n");
        return false;
      }

      printf("pdu_session_modification_command nas_buffer->msg: ");
      for (uint32_t i = 0; i < nas_buffer->N_bytes; ++i)
      {
        printf("  %x", nas_buffer->msg[i]);
      }
      printf("\n");
      // srsran::console("sm_msg->N_bytes : %d\n",nas_buffer->N_bytes);
    }
    else if (ims_ctx->ims_ctx.call_code_rate == 3)
    {
      m_nas_sm_logger.info("pack pack_pdu_session_modification_command_4_8k");
      srsran::console("packing pack_pdu_session_modification_command_4_8k\n");

      nas_msg.pdu_session_modification_command().authorized__qo_s_rules_present = true;
      qos_rule_t qos1;
      qos1.DQR = qos_rule_t::DQR_type_::options::no_default_QoS_rule;
      qos1.QRI = qos_rule_t::QRI_type_::options::QRI_3;
      qos1.Rule_operation_code = qos_rule_t::Rule_operation_code_type_::options::Create_new_QoS_rule;
      qos1.packet_filter_num = 1;

      qos1.packet_filter.Qos_rules_precedence = 0x03;
      qos1.packet_filter.pacret_filter_type2.packet_filter_direction =
          pacret_filter_type2_t::packet_filter_direction_type_::options::bidirectional;
      qos1.packet_filter.pacret_filter_type2.packet_filter_id = 2;

      qos1.packet_filter.pacret_filter_type2.pf_cont.pf_content = {0x10, 0xca, 0x6c, 0x16, 0x05, 0xff, 0xff, 0xff, 0xff};
      qos1.packet_filter.QFI_cont.QFI = QFI_cont_t::QFI_type_::options::QFI_3;
      qos1.packet_filter.QFI_cont.Segregation = QFI_cont_t::Segregation_type_::options::Segregation_not_requested;

      nas_msg.pdu_session_modification_command().authorized__qo_s_rules.qos_rules.push_back(qos1);

      nas_msg.pdu_session_modification_command().authorized__qo_s_flow_descriptions_present = true;

      qo_s_flow_description_t qo_s_flow_description1;
      qo_s_flow_description1.QFI.value = qo_s_flow_description_t::QFI_type_::options::QFI_3;
      qo_s_flow_description1.operation_code.value = qo_s_flow_description_t::operation_code_type_::options::Create_new_QoS_flow_description;
      qo_s_flow_description1.parameters_num = 5;
      qo_s_flow_description1.E.value = qo_s_flow_description_t::E_type_::parameters_list_is_included;

      QI5_parameter_contents_t parameter_content1;
      parameter_content1.parameter_id1.value = QI5_parameter_contents_t::parameter_id_type_::options::QI5;
      parameter_content1.QI5.value = QI5_parameter_contents_t::QI5_type_::QI5_1;

      GFBR_uplink_parameter_contents_t parameter_content2;
      parameter_content2.parameter_id2.value = GFBR_uplink_parameter_contents_t::parameter_id_type_::options::GFBR_uplink;
      parameter_content2.content.guaranteed_flow.value = content_t::guaranteed_flow_type_::options::value_1_Kbps;
      parameter_content2.content.guaranteed_flow_bit_rate1 = 0;
      parameter_content2.content.guaranteed_flow_bit_rate2 = 1;

      GFBR_uplink_parameter_contents_t parameter_content3;
      parameter_content3.parameter_id2.value = GFBR_uplink_parameter_contents_t::parameter_id_type_::options::GFBR_downlink;
      parameter_content3.content.guaranteed_flow.value = content_t::guaranteed_flow_type_::options::value_256_Kbps;
      parameter_content3.content.guaranteed_flow_bit_rate1 = 0;
      parameter_content3.content.guaranteed_flow_bit_rate2 = 2;

      GFBR_uplink_parameter_contents_t parameter_content4;
      parameter_content4.parameter_id2.value = GFBR_uplink_parameter_contents_t::parameter_id_type_::options::MFBR_uplink;
      parameter_content4.content.guaranteed_flow.value = content_t::guaranteed_flow_type_::options::value_4_Mbps;
      parameter_content4.content.guaranteed_flow_bit_rate1 = 0;
      parameter_content4.content.guaranteed_flow_bit_rate2 = 3;

      GFBR_uplink_parameter_contents_t parameter_content5;
      parameter_content5.parameter_id2.value = GFBR_uplink_parameter_contents_t::parameter_id_type_::options::MFBR_downlink;
      parameter_content5.content.guaranteed_flow.value = content_t::guaranteed_flow_type_::options::value_16_Mbps;
      parameter_content5.content.guaranteed_flow_bit_rate1 = 0;
      parameter_content5.content.guaranteed_flow_bit_rate2 = 4;

      qo_s_flow_description1.parameters_list.parameters1.push_back(parameter_content1);
      qo_s_flow_description1.parameters_list.parameters2.push_back(parameter_content2);
      qo_s_flow_description1.parameters_list.parameters2.push_back(parameter_content3);
      qo_s_flow_description1.parameters_list.parameters2.push_back(parameter_content4);
      qo_s_flow_description1.parameters_list.parameters2.push_back(parameter_content5);

      nas_msg.pdu_session_modification_command().authorized__qo_s_flow_descriptions.qo_s_flow_descriptions.push_back(qo_s_flow_description1);

      SRSASN_CODE err = nas_msg.pack(nas_buffer);
      if (err != SRSASN_SUCCESS)
      {
        m_nas_sm_logger.error("Error packing pdu_session_modification_command");
        srsran::console("Error packing pdu_session_modification_command\n");
        return false;
      }

      printf("pdu_session_modification_command nas_buffer->msg: ");
      for (uint32_t i = 0; i < nas_buffer->N_bytes; ++i)
      {
        printf("  %x", nas_buffer->msg[i]);
      }
      printf("\n");
      // srsran::console("sm_msg->N_bytes : %d\n",nas_buffer->N_bytes);
    }

    return true;
  }

  bool nas_sm::pack_pdu_session_modification_command_close_phone(srsran::unique_byte_buffer_t &nas_buffer, nas_context *nas_ctx, bool is_ims)
  {
    m_nas_sm_logger.info("pack pack_pdu_session_modification_command_close_phone");
    srsran::console("packing pack_pdu_session_modification_command_close_phone\n");

    nas_5gs_msg nas_msg;
    int a = 0;
    if (is_ims)
    {
      a = 1;
    }

    nas_msg.hdr.extended_protocol_discriminator = nas_5gs_hdr::extended_protocol_discriminator_opts::extended_protocol_discriminator_5gsm;
    nas_msg.hdr.pdu_session_identity = nas_ctx->nrsm_ctx.pdu_session_id[a];
    nas_msg.hdr.procedure_transaction_identity = 0;

    pdu_session_modification_command_t msg = nas_msg.set_pdu_session_modification_command();
    nas_msg.hdr.message_type.value = msg_types::pdu_session_modification_command;
    pdu_session_modification_command_t pdu_session_modification_command = nas_msg.pdu_session_modification_command();

    nas_msg.pdu_session_modification_command().always_on_pdu_session_indication_present = true;
    nas_msg.pdu_session_modification_command().always_on_pdu_session_indication.apsr = true;

    nas_msg.pdu_session_modification_command().authorized__qo_s_rules_present = true;
    qos_rule_t qos1;
    qos1.DQR = qos_rule_t::DQR_type_::options::no_default_QoS_rule;
    qos1.QRI = qos_rule_t::QRI_type_::options::QRI_3;
    qos1.Rule_operation_code = qos_rule_t::Rule_operation_code_type_::options::Delete_existing_QoS_rule;
    qos1.packet_filter_num = 0;
    qos1.packet_filter.Qos_rules_precedence = 0x79;
    qos1.packet_filter.QFI_cont.QFI = QFI_cont_t::QFI_type_::options::no_QFI;
    qos1.packet_filter.QFI_cont.Segregation = QFI_cont_t::Segregation_type_::options::Segregation_not_requested;
    nas_msg.pdu_session_modification_command().authorized__qo_s_rules.qos_rules.push_back(qos1);

    nas_msg.pdu_session_modification_command().authorized__qo_s_flow_descriptions_present = true;

    qo_s_flow_description_t qo_s_flow_description1;
    qo_s_flow_description1.QFI.value = qo_s_flow_description_t::QFI_type_::options::QFI_3;
    qo_s_flow_description1.operation_code.value = qo_s_flow_description_t::operation_code_type_::options::Delete_existing_QoS_flow_description;
    qo_s_flow_description1.parameters_num = 0;
    qo_s_flow_description1.E.value = qo_s_flow_description_t::E_type_::Reserved;
    nas_msg.pdu_session_modification_command().authorized__qo_s_flow_descriptions.qo_s_flow_descriptions.push_back(qo_s_flow_description1);

    SRSASN_CODE err = nas_msg.pack(nas_buffer);
    if (err != SRSASN_SUCCESS)
    {
      m_nas_sm_logger.error("Error packing pack_pdu_session_modification_command_close_phone");
      srsran::console("Error packing pack_pdu_session_modification_command_close_phone\n");
      return false;
    }

    printf("pack_pdu_session_modification_command_close_phone nas_buffer->msg: ");
    for (uint32_t i = 0; i < nas_buffer->N_bytes; ++i)
    {
      printf("  %x", nas_buffer->msg[i]);
    }
    printf("\n");

    return true;
  }

  bool nas_sm::handle_pdu_modification_complete(pdu_session_modification_complete_t &msg, nas_context *nas_ctx)
  {
    srsran::console("------handle pdu_modification_complete----------\n");
    m_nas_sm_logger.info("handle pdu_modification_complete");
    ims_context *ims_ctx = sm_cnw->m_pcs_ims->find_ims_ctx_from_rnti(nas_ctx->nrcm_ctx.rnti);

    if ((sms_call_control == CALL_NETWORE_TO_UE || ims_ctx->ims_ctx.ate_indication == set_up_mt_call) && nas_ctx->nrsm_ctx.modify_flag == 1)
    { // 被叫流程触呼叫发连接响应

      ims_ctx->ims_ctx.ate_indication = null_;
      // /*construct aka command message. */
      srsran::unique_byte_buffer_t ims_tx = srsran::make_byte_buffer();
      if (ims_tx == nullptr)
      {
        srsran::console("Couldn't allocate PDU in pack_call_connect_ack \n");
        return false;
      }

      sm_cnw->m_pcs_ims->pack_call_connect_ack(ims_tx, ims_ctx);
      /* update ims msg_length. 更新消息的第二个字节 (索引为1) 为当前消息的总字节数，用于标识实际消息长度. */
      ims_tx->msg[1] = ims_tx->N_bytes;
      /* save the ims context. */
      // add_ims_ctx_to_rnti_map(ims_ctx);
      // 等待IMS的回应
      srsran::console("Downlink IMS: Sending call connect ack\n");

      /*print msg ie*/
      int msg_len = ims_tx->N_bytes;
      for (int i = 0; i < msg_len; i++)
      {
        printf("ims_msg[%d]=%x", i, ims_tx->msg[i]);
        printf("\n");
      }
      /* send message to RRC message queue. */
      // sm_cnw->m_pcs_ims->rrc_pcs_ims->s_write_ims_dl_info(nas_ctx->nrcm_ctx.rnti, std::move(ims_tx));
      sm_cnw->m_pcs_ims->send_ims_dl_msg(nas_ctx->nrcm_ctx.rnti, std::move(ims_tx));
    }

    if (ims_ctx->ims_ctx.ate_indication == set_up_mt_call && nas_ctx->nrsm_ctx.modify_flag == 3)
    { // 被叫流程触呼叫发连接响应

      // /*construct aka command message. */
      srsran::unique_byte_buffer_t ims_tx = srsran::make_byte_buffer();
      if (ims_tx == nullptr)
      {
        srsran::console("Couldn't allocate PDU in pack_call_connect_ack \n");
        return false;
      }

      sm_cnw->m_pcs_ims->pack_call_comfirmed(ims_tx, ims_ctx);
      /* update ims msg_length. 更新消息的第二个字节 (索引为1) 为当前消息的总字节数，用于标识实际消息长度. */
      ims_tx->msg[1] = ims_tx->N_bytes;

      // 等待IMS的回应
      srsran::console("Downlink IMS: Sending call comfirmed \n");

      /*print msg ie*/
      int msg_len = ims_tx->N_bytes;
      for (int i = 0; i < msg_len; i++)
      {
        printf("ims_msg[%d]=%x", i, ims_tx->msg[i]);
        printf("\n");
      }
      /* send message to RRC message queue. */
      // sm_cnw->m_pcs_ims->rrc_pcs_ims->s_write_ims_dl_info(nas_ctx->nrcm_ctx.rnti, std::move(ims_tx));
      sm_cnw->m_pcs_ims->send_ims_dl_msg(nas_ctx->nrcm_ctx.rnti, std::move(ims_tx));
    }

    return true;
  }

  /*
  bool nas_sm::TTCN_handle_pdu_session_establishment_request(pdu_session_establishment_request_t& msg, nas_context* nas_ctx,bool is_ims)
  {
    srsran::console("--------TTCN_handle_pdu_session_establishment_request!---------\n");
    //m_nas_sm_logger.info("handle_pdu_session_establishment_request");

    int a=0;
    if(is_ims)
    {
       a=1;
    }
    nas_ctx->nrsm_ctx.integrity_protection.mbr_dl[a]=msg.integrity_protection_maximum_data_rate.max_data_rate_upip_downlink;
    nas_ctx->nrsm_ctx.integrity_protection.mbr_ul[a]=msg.integrity_protection_maximum_data_rate.max_data_rate_upip_uplink;
    if(msg.always_on_pdu_session_requested_present)
    {
      nas_ctx->nrsm_ctx.apsi[a]=msg.always_on_pdu_session_requested.apsi;
    }
      if(msg.pdu_session_type.pdu_session_type_value)
    {
      nas_ctx->nrsm_ctx.ue_session_type[a] = msg.pdu_session_type.pdu_session_type_value;
      //srsran::console("nas_ctx->nrsm_ctx.ue_session_type[a]=%d\n",nas_ctx->nrsm_ctx.ue_session_type[a]);
    }
    if(msg.ssc_mode.ssc_mode_value)
    {
      nas_ctx->nrsm_ctx.ue_ssc_mode[a]=msg.ssc_mode.ssc_mode_value;
    }

  //ttcn_request
   srsran::unique_byte_buffer_t send_TTCN_pdu = srsran::make_byte_buffer();
   send_TTCN_pdu->init();
   TTCN_request_header(&send_TTCN_pdu);
   send_TTCN_pdu->msg[8] = msg.pdu_session_type.pdu_session_type_value;
   send_TTCN_pdu->msg[9] = msg.ssc_mode.ssc_mode_value;
   send_TTCN_pdu->N_bytes = 10;


    sm_adp->udp_.send_ttcn_info.try_push(std::move(send_TTCN_pdu));

    std::cout<<"sm send request to ttcn"<<std::endl;


    srsran::unique_byte_buffer_t udp = srsran::make_byte_buffer();
    udp->init();
    while (true)  {
      if (sm_adp->udp_.nas_sm_receive_info.size() != 0) {
        sm_adp->udp_.nas_sm_receive_info.try_pop(udp);
        std::cout << "receive command info from TTCN" << std::endl;
        break;
      }
    }

    if(udp->msg[7]==0X57)
    {
       srsran::unique_byte_buffer_t nas_tx = srsran::make_byte_buffer();
       this->pack_pdu_session_authentication_command(nas_tx,nas_ctx,is_ims);

      m_mm->sm_write_dl_info(std::move(nas_tx), nas_ctx->nrcm_ctx.rnti);
      std::cout<<"---------9-----------------"<<std::endl;

    }

    return true;
  }*/

  bool nas_sm::TTCN_handle_pdu_session_establishment_request(pdu_session_establishment_request_t &msg, nas_context *nas_ctx, bool is_ims)
  {
    srsran::console("--------TTCN_handle_pdu_session_establishment_request!---------\n");
    // m_nas_sm_logger.info("handle_pdu_session_establishment_request");

    int a = 0;
    if (is_ims)
    {
      a = 1;
    }
    nas_ctx->nrsm_ctx.integrity_protection.mbr_dl[a] = msg.integrity_protection_maximum_data_rate.max_data_rate_upip_downlink;
    nas_ctx->nrsm_ctx.integrity_protection.mbr_ul[a] = msg.integrity_protection_maximum_data_rate.max_data_rate_upip_uplink;
    if (msg.always_on_pdu_session_requested_present)
    {
      nas_ctx->nrsm_ctx.apsi[a] = msg.always_on_pdu_session_requested.apsi;
    }
    if (msg.pdu_session_type.pdu_session_type_value)
    {
      nas_ctx->nrsm_ctx.ue_session_type[a] = msg.pdu_session_type.pdu_session_type_value;
      // srsran::console("nas_ctx->nrsm_ctx.ue_session_type[a]=%d\n",nas_ctx->nrsm_ctx.ue_session_type[a]);
    }
    if (msg.ssc_mode.ssc_mode_value)
    {
      nas_ctx->nrsm_ctx.ue_ssc_mode[a] = msg.ssc_mode.ssc_mode_value;
    }

    uint8_t qfi = (int)QFI_cont_t::QFI_type_::options::QFI_2;
    uint16_t pdu_id = nas_ctx->nrsm_ctx.pdu_session_id[a];
    if (nas_ctx->nrsm_ctx.pdu_session_id[a] == 2)
    {
      srsran::unique_byte_buffer_t send_TTCN_pdu = srsran::make_byte_buffer();
      send_TTCN_pdu->init();
      TTCN_request_header(&send_TTCN_pdu);
      send_TTCN_pdu->msg[8] = msg.pdu_session_type.pdu_session_type_value;
      send_TTCN_pdu->msg[9] = msg.ssc_mode.ssc_mode_value;
      send_TTCN_pdu->N_bytes = 10;

      // sm_adp->udp_.send_ttcn_info.try_push(std::move(send_TTCN_pdu));
      sm_adp->udp_.send_ttcn_msg_enb(std::move(send_TTCN_pdu));

      std::cout << "sm send request to ttcn" << std::endl;
      srsran::unique_byte_buffer_t nas_tx = srsran::make_byte_buffer();
      pack_pdu_session_establishment_accept(nas_tx, nas_ctx, is_ims);
      m_mm->sm_write_dl_info(std::move(nas_tx), nas_ctx->nrcm_ctx.rnti);
      auto start = std::chrono::high_resolution_clock::now();
      while (true)
      {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        if (duration.count() > 120)
        {
          break;
        }
      }
      m_mm->sm_notify_ue_erab_updates(nas_ctx->nrcm_ctx.rnti, qfi, pdu_id, {}, 0);
    }
    // ttcn_request
    if (nas_ctx->nrsm_ctx.pdu_session_id[a] == 1)
    {
      srsran::unique_byte_buffer_t send_TTCN_pdu = srsran::make_byte_buffer();
      send_TTCN_pdu->init();
      TTCN_request_header(&send_TTCN_pdu);
      send_TTCN_pdu->msg[8] = msg.pdu_session_type.pdu_session_type_value;
      send_TTCN_pdu->msg[9] = msg.ssc_mode.ssc_mode_value;
      send_TTCN_pdu->N_bytes = 10;

      // sm_adp->udp_.send_ttcn_info.try_push(std::move(send_TTCN_pdu));
      sm_adp->udp_.send_ttcn_msg_enb(std::move(send_TTCN_pdu));

      std::cout << "sm send request to ttcn" << std::endl;

      srsran::unique_byte_buffer_t udp = srsran::make_byte_buffer();
      udp->init();
      while (true)
      {
        if (sm_adp->udp_.nas_sm_receive_info.size() != 0)
        {
          sm_adp->udp_.nas_sm_receive_info.try_pop(udp);
          std::cout << "receive command info from TTCN" << std::endl;
          break;
        }
      }

      if (udp->msg[7] == 0X61)
      {
        srsran::unique_byte_buffer_t nas_tx = srsran::make_byte_buffer();
        pack_pdu_session_establishment_accept(nas_tx, nas_ctx, is_ims);
        m_mm->sm_write_dl_info(std::move(nas_tx), nas_ctx->nrcm_ctx.rnti);

        m_mm->sm_notify_ue_erab_updates(nas_ctx->nrcm_ctx.rnti, qfi, pdu_id, {}, 0);

        printf("__________qfi=%d,----------pdu_id=%d--\n", qfi, pdu_id);
      }
    }

    return true;
  }

  bool nas_sm::TTCN2_handle_pdu_session_establishment_request(pdu_session_establishment_request_t &msg, nas_context *nas_ctx, bool is_ims)
  {
    if (test_num == 0)
    {
      start_time = std::chrono::high_resolution_clock::now();
    }

    if (test_num == 1)
    {

      s2_time = std::chrono::high_resolution_clock::now();
      auto now_time = std::chrono::high_resolution_clock::now();
      auto duration =
          std::chrono::duration_cast<std::chrono::milliseconds>(now_time - start_time);
      std::cout << "1 SM test_num:" << duration.count() << std::endl;
    }
    else if (test_num == 2)
    {
      s3_time = std::chrono::high_resolution_clock::now();
      auto now_time = std::chrono::high_resolution_clock::now();
      auto duration =
          std::chrono::duration_cast<std::chrono::milliseconds>(now_time - s2_time);
      std::cout << "2 SM test_num:" << duration.count() << std::endl;
    }
    else if (test_num == 3)
    {
      s4_time = std::chrono::high_resolution_clock::now();
      auto now_time = std::chrono::high_resolution_clock::now();
      auto duration =
          std::chrono::duration_cast<std::chrono::milliseconds>(now_time - s3_time);
      std::cout << "3 SM test_num:" << duration.count() << std::endl;
    }
    else if (test_num == 4)
    {
      auto now_time = std::chrono::high_resolution_clock::now();
      auto duration =
          std::chrono::duration_cast<std::chrono::milliseconds>(now_time - s4_time);
      std::cout << "4 SM test_num:" << duration.count() << std::endl;
    }

    //  auto start_time = std::chrono::high_resolution_clock::now();

    // std::cout<<"test_num:"<<test_num<<std::endl;

    test_num++;
    srsran::console("--------TTCN2handle_pdu_session_establishment_request!---------\n");
    // m_nas_sm_logger.info("handle_pdu_session_establishment_request");

    int a = 0;
    if (is_ims)
    {
      a = 1;
    }
    nas_ctx->nrsm_ctx.integrity_protection.mbr_dl[a] = msg.integrity_protection_maximum_data_rate.max_data_rate_upip_downlink;
    nas_ctx->nrsm_ctx.integrity_protection.mbr_ul[a] = msg.integrity_protection_maximum_data_rate.max_data_rate_upip_uplink;
    if (msg.always_on_pdu_session_requested_present)
    {
      nas_ctx->nrsm_ctx.apsi[a] = msg.always_on_pdu_session_requested.apsi;
    }
    if (msg.pdu_session_type.pdu_session_type_value)
    {
      nas_ctx->nrsm_ctx.ue_session_type[a] = msg.pdu_session_type.pdu_session_type_value;
      // srsran::console("nas_ctx->nrsm_ctx.ue_session_type[a]=%d\n",nas_ctx->nrsm_ctx.ue_session_type[a]);
    }
    if (msg.ssc_mode.ssc_mode_value)
    {
      nas_ctx->nrsm_ctx.ue_ssc_mode[a] = msg.ssc_mode.ssc_mode_value;
    }

    // ttcn_request
    srsran::unique_byte_buffer_t send_TTCN_pdu = srsran::make_byte_buffer();
    send_TTCN_pdu->init();
    TTCN_request_header(&send_TTCN_pdu);
    send_TTCN_pdu->msg[8] = msg.pdu_session_type.pdu_session_type_value;
    send_TTCN_pdu->msg[9] = msg.ssc_mode.ssc_mode_value;
    send_TTCN_pdu->N_bytes = 10;

    // sm_adp->udp_.send_ttcn_info.try_push(std::move(send_TTCN_pdu));
      sm_adp->udp_.send_ttcn_msg_enb(std::move(send_TTCN_pdu));

    std::cout << "sm send request to ttcn" << std::endl;

    return true;
  }

  bool nas_sm::TTCN_handle_pdu_authentication_complete(pdu_session_authentication_complete_t &msg, nas_context *nas_ctx, bool is_accept, bool is_ims)
  {
    srsran::console("------TTCN_handle pdu_session_authentication_complete----------\n");
    m_nas_sm_logger.info("TTCN_handle pdu_session_authentication_complete");
    int a = 0;
    if (is_ims)
    {
      a = 1;
    }

    srsran::unique_byte_buffer_t SM_TTCN_complete_header = srsran::make_byte_buffer();
    TTCN_complete_header(&SM_TTCN_complete_header);
    SM_TTCN_complete_header->N_bytes = 8;

    sm_adp->udp_.send_ttcn_info.try_push(std::move(SM_TTCN_complete_header));
    sm_adp->udp_.send_ttcn_msg_enb(std::move(SM_TTCN_complete_header));

    std::cout << "sm send complete to ttcn" << endl;
    srsran::unique_byte_buffer_t udp = srsran::make_byte_buffer();
    udp->init();
    while (true)
    {
      if (sm_adp->udp_.nas_sm_receive_info.size() != 0)
      {
        sm_adp->udp_.nas_sm_receive_info.try_pop(udp);
        std::cout << "receive accept info from TTCN" << std::endl;
        break;
      }
    }
    nas_ctx->nrsm_ctx.ue_session_type[a] = udp->msg[8];
    nas_ctx->nrsm_ctx.ue_ssc_mode[a] = udp->msg[9];

    srsran::unique_byte_buffer_t nas_tx = srsran::make_byte_buffer();
    // srsran::const_byte_span nas_pdu_ref = srsran::make_byte_buffer();
    uint8_t qfi = (int)QFI_cont_t::QFI_type_::options::QFI_2;

    uint16_t pdu_id = nas_ctx->nrsm_ctx.pdu_session_id[a];
    if (is_accept == true)
    {

      pack_pdu_session_establishment_accept(nas_tx, nas_ctx, is_ims);
      m_mm->sm_write_dl_info(std::move(nas_tx), nas_ctx->nrcm_ctx.rnti);

      if (pdu_id == 2)
      {
        auto start = std::chrono::high_resolution_clock::now();
        while (true)
        {
          auto end = std::chrono::high_resolution_clock::now();
          auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
          if (duration.count() > 120)
          {
            break;
          }
        }
      }
      //   auto start=std::chrono::high_resolution_clock::now();
      //   while(true){
      //   auto end=std::chrono::high_resolution_clock::now();
      //   auto duration=std::chrono::duration_cast<std::chrono::milliseconds>(end-start);
      //   if(duration.count()>2400){
      //     break;
      //   }
      // }
      m_mm->sm_notify_ue_erab_updates(nas_ctx->nrcm_ctx.rnti, qfi, pdu_id, {}, 0);

      printf("TTCN__________qfi=%d,----------pdu_id=%d--\n", qfi, pdu_id);
    }
    else
    {
      std::cout << "--------------------panduanpudsessionacc------------------" << std::endl;
      pack_pdu_session_authentication_result(nas_tx, nas_ctx);
      m_mm->sm_write_dl_info(std::move(nas_tx), nas_ctx->nrcm_ctx.rnti);
    }
    return true;
  }

  void nas_sm::TTCN_request_header(srsran::unique_byte_buffer_t *SM_TTCN_request_header)
  {
    // hexstring   direction 				length(2),
    // hexstring   rec_lay_id 				length(2),
    // hexstring   des_layer_id 			length(2),
    // hexstring   message_type 			length(2),
    // hexstring   data_length 			length(2),
    SM_TTCN_request_header->get()->msg[0] = 0x01;
    SM_TTCN_request_header->get()->msg[1] = 0x01;
    SM_TTCN_request_header->get()->msg[2] = 0x00;
    SM_TTCN_request_header->get()->msg[3] = 0x00;
    SM_TTCN_request_header->get()->msg[4] = 0x00;
    SM_TTCN_request_header->get()->msg[5] = 0x00;
    SM_TTCN_request_header->get()->msg[6] = 0x03;
    SM_TTCN_request_header->get()->msg[7] = 0x66;
  }

  void nas_sm::TTCN_complete_header(srsran::unique_byte_buffer_t *SM_TTCN_complete_header)
  {
    SM_TTCN_complete_header->get()->msg[0] = 0x01;
    SM_TTCN_complete_header->get()->msg[1] = 0x01;
    SM_TTCN_complete_header->get()->msg[2] = 0x00;

    SM_TTCN_complete_header->get()->msg[3] = 0x00;
    SM_TTCN_complete_header->get()->msg[4] = 0x00;

    SM_TTCN_complete_header->get()->msg[5] = 0x00;
    SM_TTCN_complete_header->get()->msg[6] = 0x01;

    SM_TTCN_complete_header->get()->msg[7] = 0x63;
  }


bool nas_sm::pack_for_sdap_ttcn_accept(srsran::unique_byte_buffer_t& nas_buffer,nas_context* nas_ctx,bool is_ims)
{
  std::cout << "---------pack_pdu_session_establishment_accept for sdap------------" << std::endl;
  nas_5gs_msg nas_msg;

  if(is_ims)
  {
    this->pack_pdu_session_establishment_accept(nas_buffer,nas_ctx,is_ims);
  }
  else if(!is_ims)
  {
    //hand include id pti epd 
    nas_msg.hdr.pdu_session_identity=nas_ctx->nrsm_ctx.pdu_session_id[0]; 
    nas_msg.hdr.extended_protocol_discriminator = nas_5gs_hdr::extended_protocol_discriminator_opts::extended_protocol_discriminator_5gsm;
    nas_msg.hdr.procedure_transaction_identity = nas_ctx->nrsm_ctx.pti[0];
    //message
    pdu_session_establishment_accept_t msg = nas_msg.set_pdu_session_establishment_accept();
    nas_msg.hdr.message_type.value = msg_types::pdu_session_establishment_accept;
    pdu_session_establishment_accept_t pdu_sess_esta_accept = nas_msg.pdu_session_establishment_accept();
    //printf("-----------------------\n");
    nas_msg.pdu_session_establishment_accept().selected_pdu_session_type.pdu_session_type_value = nas_ctx->convert_pdu_type(nas_ctx->nrsm_ctx.ue_session_type[0]);
    //printf("-----------------------\n");
    nas_msg.pdu_session_establishment_accept().selected_ssc_mode.ssc_mode_value = nas_ctx->convert_ssc_mode(nas_ctx->nrsm_ctx.ue_ssc_mode[0]);
    //ambr?qos?
    nas_msg.pdu_session_establishment_accept().authorized__qo_s_flow_descriptions_present=true;

    qos_rule_t qos1;
    qos1.DQR                 = qos_rule_t::DQR_type_::options::default_QoS_rule;
    qos1.QRI                 = qos_rule_t::QRI_type_::options::QRI_3;
    qos1.Rule_operation_code = qos_rule_t::Rule_operation_code_type_::options::Create_new_QoS_rule;
    qos1.packet_filter_num                  = 1;
    qos1.packet_filter.Qos_rules_precedence = 0x03;
    qos1.packet_filter.pacret_filter_type2.packet_filter_direction =
        pacret_filter_type2_t::packet_filter_direction_type_::options::bidirectional;
    qos1.packet_filter.pacret_filter_type2.packet_filter_id = 0;
    qos1.packet_filter.pacret_filter_type2.pf_cont.pf_content = {0x01};
    qos1.packet_filter.QFI_cont.QFI=QFI_cont_t::QFI_type_::QFI_1;
    qos1.packet_filter.QFI_cont.Segregation=QFI_cont_t::Segregation_type_::Segregation_not_requested;
    nas_msg.pdu_session_establishment_accept().authorized__qo_s_rules.qos_rules.push_back(qos1);

    qo_s_flow_description_t qo_s_flow_description1;
    qo_s_flow_description1.QFI.value=qo_s_flow_description_t::QFI_type_::options::QFI_1;
    qo_s_flow_description1.operation_code.value=qo_s_flow_description_t::operation_code_type_::options::Create_new_QoS_flow_description;
    qo_s_flow_description1.parameters_num=0x01;
    qo_s_flow_description1.E.value=qo_s_flow_description_t::E_type_::parameters_list_is_included;
    QI5_parameter_contents_t parameter_content1;
    parameter_content1.parameter_id1.value=QI5_parameter_contents_t::parameter_id_type_::options::QI5;
    parameter_content1.QI5.value=QI5_parameter_contents_t::QI5_type_::QI5_9;
    qo_s_flow_description1.parameters_list.parameters1.push_back(parameter_content1);
    nas_msg.pdu_session_establishment_accept().authorized__qo_s_flow_descriptions.qo_s_flow_descriptions.push_back(qo_s_flow_description1);

    std::cout<<"qos 5qi value:"<<(int)parameter_content1.QI5.value<<std::endl;
    //m_mm->tran_sm_5qi_to_mm(nas_ctx->nrcm_ctx.rnti,(int)parameter_content1.QI5.value);

    qos_rule_t qos2;
    qos2.DQR                 = qos_rule_t::DQR_type_::options::no_default_QoS_rule;
    qos2.QRI                 = qos_rule_t::QRI_type_::options::QRI_5;
    qos2.Rule_operation_code = qos_rule_t::Rule_operation_code_type_::options::Create_new_QoS_rule;
    qos2.packet_filter_num                  = 1;
    qos2.packet_filter.Qos_rules_precedence = 0x05;
    qos2.packet_filter.pacret_filter_type2.packet_filter_direction =
        pacret_filter_type2_t::packet_filter_direction_type_::options::bidirectional;
    qos2.packet_filter.pacret_filter_type2.packet_filter_id = 0;
    qos2.packet_filter.pacret_filter_type2.pf_cont.pf_content = {0x01};
    qos2.packet_filter.QFI_cont.QFI=QFI_cont_t::QFI_type_::QFI_2;
    qos2.packet_filter.QFI_cont.Segregation=QFI_cont_t::Segregation_type_::Segregation_not_requested;  
    nas_msg.pdu_session_establishment_accept().authorized__qo_s_rules.qos_rules.push_back(qos2);
  
    qo_s_flow_description_t qo_s_flow_description2;
    qo_s_flow_description2.QFI.value=qo_s_flow_description_t::QFI_type_::options::QFI_2;
    qo_s_flow_description2.operation_code.value=qo_s_flow_description_t::operation_code_type_::options::Create_new_QoS_flow_description;
    qo_s_flow_description2.parameters_num=0x01;
    qo_s_flow_description2.E.value=qo_s_flow_description_t::E_type_::options::parameters_list_is_included;
    QI5_parameter_contents_t parameter_content2;
    parameter_content2.parameter_id1.value=QI5_parameter_contents_t::parameter_id_type_::options::QI5;
    parameter_content2.QI5.value=QI5_parameter_contents_t::QI5_type_::options::QI5_5;
    qo_s_flow_description2.parameters_list.parameters1.push_back(parameter_content2);  
    nas_msg.pdu_session_establishment_accept().authorized__qo_s_flow_descriptions.qo_s_flow_descriptions.push_back(qo_s_flow_description2);

    //std::cout<<"qos 5qi value:"<<(int)parameter_content2.QI5.value<<std::endl;
    //m_mm->tran_sm_5qi_to_mm(nas_ctx->nrcm_ctx.rnti,(int)parameter_content2.QI5.value);

    nas_msg.pdu_session_establishment_accept().session_ambr.unit_session_ambr_for_downlink =
        session_ambr_t::unit_session_AMBR_type_::options::inc_by_256_kbps;
    nas_msg.pdu_session_establishment_accept().session_ambr.unit_session_ambr_for_uplink =
        session_ambr_t::unit_session_AMBR_type_::options::inc_by_256_kbps;
    nas_msg.pdu_session_establishment_accept().session_ambr.session_ambr_for_downlink = 4;
    nas_msg.pdu_session_establishment_accept().session_ambr.session_ambr_for_uplink   = 4;


    //eap
    // eap
    nas_msg.pdu_session_establishment_accept().eap_message_present     = true;
    nas_msg.pdu_session_establishment_accept().eap_message.eap_message = {0x03, 0x00, 0x00, 0x08, 0x32, 0x00, 0x00, 0x00};
    nas_msg.pdu_session_establishment_accept().pdu_address_present     = true;
    nas_msg.pdu_session_establishment_accept().pdu_address.ipv4        = {1, 127, 11, 10};
    //allways on
    nas_msg.pdu_session_establishment_accept().always_on_pdu_session_indication_present = true;
    nas_msg.pdu_session_establishment_accept().always_on_pdu_session_indication.apsr = true;

    
    SRSASN_CODE err = nas_msg.pack(nas_buffer);
    if (err != SRSASN_SUCCESS) {
      m_nas_sm_logger.error("Error packing pdu_session_establishment_accept");
      srsran::console("Error packing pdu_session_establishment_accept\n");
      return false;
    }
    //printf("msg=%d\n",nas_msg);
  }
  
 return true;
}

bool nas_sm::handle_for_sdap_ttcn_complete(pdu_session_authentication_complete_t& msg,nas_context* nas_ctx,bool is_accept,bool is_ims,uint16_t enb_ue_id)
{
  srsran::console("------handle pdu_session_authentication_complete for sdap ttcn----------\n");
  int a=0;
  if(is_ims)
  {
     a=1;  
  }
  srsran::unique_byte_buffer_t nas_tx = srsran::make_byte_buffer();
  uint8_t qfi=2;
  uint16_t pdu_id = nas_ctx->nrsm_ctx.pdu_session_id[a];
  if(is_accept == true)
    {
       
     pack_for_sdap_ttcn_accept(nas_tx,nas_ctx,is_ims); 
     m_mm->sm_write_dl_info(std::move(nas_tx), nas_ctx->nrcm_ctx.rnti);
     
     if(pdu_id==2)
     {
        auto start=std::chrono::high_resolution_clock::now();
        while(true){
        auto end=std::chrono::high_resolution_clock::now();
        auto duration=std::chrono::duration_cast<std::chrono::milliseconds>(end-start);
        if(duration.count()>120){
          break;
        }
        }
      }
      printf("sm_ttcn——重配");
      m_mm->sm_notify_ue_erab_updates(nas_ctx->nrcm_ctx.rnti,qfi,pdu_id,  {},0);
      if(tc_control==ENABLE_TC_MODE&&pdu_id==1) 
      {
          srsran::unique_byte_buffer_t test_loop = srsran::make_byte_buffer();
          m_mm->pack_close_ue_test_loop(test_loop,nas_ctx,enb_ue_id);
          //sleep(1);
      }
      printf("__________qfi=%d,----------pdu_id=%d--\n",qfi,pdu_id);
    }
    else
    {
      std::cout<<"--------------------panduanpudsessionacc------------------"<<std::endl;
      pack_pdu_session_authentication_result(nas_tx,nas_ctx);
      m_mm->sm_write_dl_info(std::move(nas_tx), nas_ctx->nrcm_ctx.rnti);
    }
  return true;
}
bool nas_sm::TTCN_handle_pdu_release_request(srsran::unique_byte_buffer_t& nas_buffer, nas_context* nas_ctx)
{
  printf("TTCN_handle_pdu_release_request\n");
  nas_buffer->init();
  TTCN_request_header(&nas_buffer);
  nas_buffer->msg[7] = 0x67;//pdu_release_request
  nas_buffer->N_bytes = 8;

  // sm_adp->udp_.send_ttcn_info.try_push(std::move(send_TTCN_pdu));
  sm_adp->udp_.send_ttcn_msg_enb(std::move(nas_buffer));

  std::cout << "sm send request to ttcn" << std::endl;
  srsran::unique_byte_buffer_t udp = srsran::make_byte_buffer();
  udp->init();
  while (true)
  {
    if (sm_adp->udp_.nas_sm_receive_info.size() != 0)
    {
      sm_adp->udp_.nas_sm_receive_info.try_pop(udp);
      std::cout << "receive command info from TTCN" << std::endl;
      break;
    }
  }

  if (udp->msg[7] == 0X59)
  {
    srsran::unique_byte_buffer_t nas_tx = srsran::make_byte_buffer();
    TTCN_pack_pdu_session_modification_command(nas_tx, nas_ctx, nas_ctx->nrsm_ctx.is_ims);
    TC_92_3_seconds=true;
    m_mm->sm_write_dl_info(std::move(nas_tx), nas_ctx->nrcm_ctx.rnti);
  }
  return true;

}
bool nas_sm::TTCN_handle_pdu_release_complete(srsran::unique_byte_buffer_t& nas_buffer, nas_context* nas_ctx)
{
  printf("TTCN_handle_pdu_release_complete\n");
  nas_buffer->init();
  TTCN_request_header(&nas_buffer);
  nas_buffer->msg[7] = 0x65;//pdu_release_complete
  nas_buffer->N_bytes = 8;

  // sm_adp->udp_.send_ttcn_info.try_push(std::move(send_TTCN_pdu));
  sm_adp->udp_.send_ttcn_msg_enb(std::move(nas_buffer));

  std::cout << "sm send pdu_release_complete to ttcn" << std::endl;
  return true;

}


void nas_sm::TC_92_after_3_seconds()
{
  if(this->TC_92_3_seconds && sm_adp->udp_.TC_92_sm_release)
  {
    srsran::unique_byte_buffer_t udp = srsran::make_byte_buffer();
    if (sm_adp->udp_.nas_sm_receive_info.size() != 0)
    {
      sm_adp->udp_.nas_sm_receive_info.try_pop(udp);
      std::cout << "receive command info from TTCN" << std::endl;
      if(udp->msg[7]==0x60)//realse command
      {
        handle_pdu_session_release_command_from_ttcn();
      }
      this->TC_92_3_seconds=false;
    }
  }
}

bool nas_sm::handle_pdu_session_release_command_from_ttcn()
{
  printf("handle_pdu_session_release_command_from_ttcn\n");
    nas_5gs_msg nas_msg;
    pdu_session_release_command_t& pduSessionRelCmd= nas_msg.set_pdu_session_release_command();
    nas_msg.hdr.extended_protocol_discriminator = nas_5gs_hdr::extended_protocol_discriminator_opts::extended_protocol_discriminator_5gsm;

    nas_context* nas_ctx = sm_cnw->find_nas_ctx_from_rnti(70);
    if (nas_ctx == nullptr) {
        m_nas_sm_logger.warning("Prepare to send pdu session modification command msg from ttcn, but could not find UE NAS context. RNTI id: %d", 70);
        return false;
    }

    srsran::unique_byte_buffer_t nas_tx;
    nas_tx        = srsran::make_byte_buffer();
    if (nas_tx == nullptr) {
        m_nas_sm_logger.error("Couldn't allocate PDU in %s().", __FUNCTION__);
        return false;
    }

    uint8_t pduSessionId = 0x01;
    uint8_t pti = 0x03;

    if(0xff != pduSessionId)
    {
        nas_msg.hdr.pdu_session_identity = pduSessionId;
    }
    else
    {
        nas_msg.hdr.pdu_session_identity = 0;
        nas_ctx->nrsm_ctx.pdu_session_id[1] = 0;
    }

    if(0xff != pti)
    {
        nas_msg.hdr.procedure_transaction_identity = pti;
    }
    else
    {
        nas_msg.hdr.procedure_transaction_identity = 0;
        //nas_ctx->nrsm_ctx.pti = 0;
    }

    pduSessionRelCmd.cause_5gsm.cause_value = cause_5gsm_t::cause_value_type::options::regular_deactivation;

    SRSASN_CODE err = nas_msg.pack(nas_tx);
    if (err != SRSASN_SUCCESS) {
        m_nas_sm_logger.error("Error packing pdu session release command");
        srsran::console("Error packing pdu session release command\n");
        return false;
    }

    m_mm->sm_write_dl_info(std::move(nas_tx), nas_ctx->nrcm_ctx.rnti);

    srsran::console("send pdu session release command msg\n");
    m_nas_sm_logger.info("send pdu session release command msg\n");
    return true;
}
bool nas_sm::TTCN_pack_pdu_session_modification_command(srsran::unique_byte_buffer_t &nas_buffer, nas_context *nas_ctx, bool is_ims)
{
    m_nas_sm_logger.info("pack pdu_session_modification_command");
    srsran::console("packing pdu_session_modification_command\n");

    ims_context *ims_ctx = sm_cnw->m_pcs_ims->find_ims_ctx_from_rnti(nas_ctx->nrcm_ctx.rnti);

    nas_5gs_msg nas_msg;
    int a = 0;
    if (is_ims)
    {
      a = 1;
    }
    nas_msg.hdr.extended_protocol_discriminator = nas_5gs_hdr::extended_protocol_discriminator_opts::extended_protocol_discriminator_5gsm;
    nas_msg.hdr.pdu_session_identity = 0x01;
    nas_msg.hdr.procedure_transaction_identity = 0;

    pdu_session_modification_command_t msg = nas_msg.set_pdu_session_modification_command();
    nas_msg.hdr.message_type.value = msg_types::pdu_session_modification_command;
    SRSASN_CODE err = nas_msg.pack(nas_buffer);
    if (err != SRSASN_SUCCESS)
    {
      m_nas_sm_logger.error("Error packing pdu_session_modification_command");
      srsran::console("Error packing pdu_session_modification_command\n");
      return false;
    }

    printf("pdu_session_modification_command nas_buffer->msg: ");
    for (uint32_t i = 0; i < nas_buffer->N_bytes; ++i)
    {
      printf("  %x", nas_buffer->msg[i]);
    }
    printf("\n");
    // srsran::console("sm_msg->N_bytes : %d\n",nas_buffer->N_bytes);

    return true;

}





} // namespace srsepc