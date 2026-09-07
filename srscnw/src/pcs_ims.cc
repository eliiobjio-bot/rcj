#include "srscnw/hdr/pcs_ims.h"
#include "srscnw/hdr/cnw.h"
#include "srscnw/hdr/nas_context.h"
#include "srsran/asn1/nas_5g_msg.h"
#include "srsran/asn1/s1ap.h"

#include <inttypes.h>
#include <stdint.h>

using namespace srsran;
using namespace srsran::ims;
using namespace std;

namespace srsepc
{

  pcs_ims *pcs_ims::m_instance = NULL;
  pthread_mutex_t pcs_ims_instance_mutex = PTHREAD_MUTEX_INITIALIZER;

  pcs_ims::pcs_ims(){};

  pcs_ims::~pcs_ims()
  {
    return;
  }

  pcs_ims *pcs_ims::get_instance(void)
  {
    pthread_mutex_lock(&pcs_ims_instance_mutex);
    if (NULL == m_instance)
    {
      m_instance = new pcs_ims();
    }
    pthread_mutex_unlock(&pcs_ims_instance_mutex);
    return (m_instance);
  }

  void pcs_ims::cleanup(void)
  {
    pthread_mutex_lock(&pcs_ims_instance_mutex);
    if (NULL != m_instance)
    {
      delete m_instance;
      m_instance = NULL;
    }
    pthread_mutex_unlock(&pcs_ims_instance_mutex);
  }

  void pcs_ims::init(srsenb::rrc_interface_cnw *rrc_, adp *adp)
  {
    ims_adp = adp;
    rrc_pcs_ims = rrc_;
    m_cnw = cnw::get_instance();
    //   m_nas_sm =nas_sm::get_instance();   //12.9
  }

  bool pcs_ims::pcs_handle_ate_info(srsran::unique_byte_buffer_t pdu)
  {
    printf("----------this is pcs_handle_ate_info function------------\n");
    // print the data/msg
    int pdu_length = pdu->N_bytes;
    for (int j = 0; j < pdu_length; ++j)
    {
      srsran::console("ims received ate_msg[%d] =%x\n", j, pdu->msg[j]);
    }

    ims_context *ims_ctx = find_ims_ctx_from_rnti(pcs_rnti);
    if (ims_ctx == NULL)
    {
      pcs_ims_logger.error("Don't find_ims_ctx_from_rnti. rnti: %d", pcs_rnti);
      return false;
    }

    srsran::unique_byte_buffer_t ims_tx = srsran::make_byte_buffer();
    if (ims_tx == nullptr)
    {
      srsran::console("Couldn't allocate PDU in pcs_handle_ate_info \n");
      return false;
    }

    srsran::unique_byte_buffer_t ims_ate_tx = srsran::make_byte_buffer();
    if (ims_ate_tx == nullptr)
    {
      srsran::console("Couldn't allocate PDU in TC_MSG_IMS_ATE_REGISTER \n");
      return false;
    }

    // uint8_t test_long_sms[]={0xff, 0x01, TC_MSG_ATE_IMS_MT_SMS, 0x08, 0x01, 0x18,
    //                          0x4e, 0x00, 0x4e, 0x8c, 0x4e , 0x09, 0x56, 0xdb, 0x4e, 0x94,
    //                          0x4e, 0x00, 0x4e, 0x8c, 0x4e , 0x09, 0x56, 0xdb, 0x4e, 0x94,
    //                          0x4e, 0x00, 0x4e, 0x8c, 0x4e , 0x09, 0x56, 0xdb, 0x4e, 0x94,
    //                          0x4e, 0x00, 0x4e, 0x8c, 0x4e , 0x09, 0x56, 0xdb, 0x4e, 0x94,
    //                          0x4e, 0x00, 0x4e, 0x8c, 0x4e , 0x09, 0x56, 0xdb, 0x4e, 0x94,
    //                          0x4e, 0x00, 0x4e, 0x8c, 0x4e , 0x09, 0x56, 0xdb, 0x4e, 0x94,
    //                          0x4e, 0x00, 0x4e, 0x8c, 0x4e , 0x09, 0x56, 0xdb, 0x4e, 0x94,
    //                          0x4e, 0x00, 0x4e, 0x8c, 0x4e , 0x09, 0x56, 0xdb, 0x4e, 0x94,
    //                          0x4e, 0x00, 0x4e, 0x8c, 0x4e , 0x09, 0x56, 0xdb, 0x4e, 0x94,
    //                          0x4e, 0x00, 0x4e, 0x8c, 0x4e , 0x09, 0x56, 0xdb, 0x4e, 0x94,
    //                          0x4e, 0x00, 0x4e, 0x8c, 0x4e , 0x09, 0x56, 0xdb, 0x4e, 0x94,
    //                          0x4e, 0x00, 0x4e, 0x8c, 0x4e , 0x09, 0x56, 0xdb, 0x4e, 0x94,
    //                          0x4e, 0x00, 0x4e, 0x8c, 0x4e , 0x09, 0x56, 0xdb, 0x4e, 0x94,
    //                          0x4e, 0x00, 0x4e, 0x8c, 0x4e , 0x09, 0x56, 0xdb, 0x4e, 0x94,
    //                          0x4e, 0x00, 0x4e, 0x8c, 0x4e , 0x09, 0x56, 0xdb, 0x4e, 0x94,
    //                          0x4e, 0x00, 0x4e, 0x8c, 0x4e , 0x09, 0x56, 0xdb, 0x4e, 0x94,
    //                          0x4e, 0x00, 0x4e, 0x8c, 0x4e , 0x09, 0x56, 0xdb, 0x4e, 0x94,
    //                          0x4e, 0x00, 0x4e, 0x8c, 0x4e , 0x09, 0x56, 0xdb, 0x4e, 0x94,
    //                          0x4e, 0x00, 0x4e, 0x8c, 0x4e , 0x09, 0x56, 0xdb, 0x4e, 0x94,
    //                          0x4e, 0x00, 0x4e, 0x8c, 0x4e , 0x09, 0x56, 0xdb, 0x4e, 0x94,
    //                          0x4e, 0x00, 0x4e, 0x8c, 0x4e , 0x09, 0x56, 0xdb, 0x4e, 0x94,
    //                          0x4e, 0x00, 0x4e, 0x8c, 0x4e , 0x09, 0x56, 0xdb, 0x4e, 0x94,
    //                          0x4e, 0x00, 0x4e, 0x8c, 0x4e , 0x09, 0x56, 0xdb, 0x4e, 0x94,
    //                          0x4e, 0x00, 0x4e, 0x8c, 0x4e , 0x09, 0x56, 0xdb, 0x4e, 0x94,
    //                          0x4e, 0x00, 0x4e, 0x8c, 0x4e , 0x09, 0x56, 0xdb, 0x4e, 0x94,
    //                          0x4e, 0x00, 0x4e, 0x8c, 0x4e , 0x09, 0x56, 0xdb, 0x4e, 0x94,
    //                          0x4e, 0x00, 0x4e, 0x8c, 0x4e , 0x09, 0x56, 0xdb, 0x4e, 0x94,
    //                          0x4e, 0x00, 0x4e, 0x8c, 0x4e , 0x09, 0x56, 0xdb, 0x4e, 0x94
    //                         };

    // c2E231B96C3EA3C3E231B96C3EA3    c46372D97C4687C56372D97C4687
    //  uint8_t test_long_sms[]={0xff, 0x01, TC_MSG_ATE_IMS_MT_SMS, 0x00, 0x01, 0x04,
    //                        //1
    //                        0xc2, 0xe2, 0x31, 0xb9, 0x6c , 0x3e, 0xa3, 0xc3, 0xe2, 0x31, 0xb9, 0x6c , 0x3e, 0xa3,
    //                        0xc3, 0xe2, 0x31, 0xb9, 0x6c , 0x3e, 0xa3, 0xc3, 0xe2, 0x31, 0xb9, 0x6c , 0x3e, 0xa3,
    //                        0xc3, 0xe2, 0x31, 0xb9, 0x6c , 0x3e, 0xa3, 0xc3, 0xe2, 0x31, 0xb9, 0x6c , 0x3e, 0xa3,
    //                        0xc3, 0xe2, 0x31, 0xb9, 0x6c , 0x3e, 0xa3, 0xc3, 0xe2, 0x31, 0xb9, 0x6c , 0x3e, 0xa3,
    //                        0xc3, 0xe2, 0x31, 0xb9, 0x6c , 0x3e, 0xa3, 0xc3, 0xe2, 0x31, 0xb9, 0x6c , 0x3e, 0xa3,
    //                        0xc3, 0xe2, 0x31, 0xb9, 0x6c , 0x3e, 0xa3, 0xc3, 0xe2, 0x31, 0xb9, 0x6c , 0x3e, 0xa3,
    //                        0xc3, 0xe2, 0x31, 0xb9, 0x6c , 0x3e, 0xa3, 0xc3, 0xe2, 0x31, 0xb9, 0x6c , 0x3e, 0xa3,
    //                        0xc3, 0xe2, 0x31, 0xb9, 0x6c , 0x3e, 0xa3, 0xc3, 0xe2, 0x31, 0xb9, 0x6c , 0x3e, 0xa3,
    //                        0xc3, 0xe2, 0x31, 0xb9, 0x6c , 0x3e, 0xa3, 0xc3, 0xe2, 0x31, 0xb9, 0x6c , 0x3e, 0xa3,
    //                        0xc3, 0xe2, 0x31, 0xb9, 0x6c , 0x3e, 0xa3, 0xc3,
    //                        //2
    //                        0xc4, 0x63, 0x72, 0xd9, 0x7c , 0x46, 0x87, 0xc5, 0x63, 0x72, 0xd9, 0x7c , 0x46, 0x87,
    //                        0xc5, 0x63, 0x72, 0xd9, 0x7c , 0x46, 0x87, 0xc5, 0x63, 0x72, 0xd9, 0x7c , 0x46, 0x87,
    //                        0xc5, 0x63, 0x72, 0xd9, 0x7c , 0x46, 0x87, 0xc5, 0x63, 0x72, 0xd9, 0x7c , 0x46, 0x87,
    //                        0xc5, 0x63, 0x72, 0xd9, 0x7c , 0x46, 0x87, 0xc5, 0x63, 0x72, 0xd9, 0x7c , 0x46, 0x87,
    //                        0xc5, 0x63, 0x72, 0xd9, 0x7c , 0x46, 0x87, 0xc5, 0x63, 0x72, 0xd9, 0x7c , 0x46, 0x87,
    //                        0xc5, 0x63, 0x72, 0xd9, 0x7c , 0x46, 0x87, 0xc5, 0x63, 0x72, 0xd9, 0x7c , 0x46, 0x87,
    //                        0xc5, 0x63, 0x72, 0xd9, 0x7c , 0x46, 0x87, 0xc5, 0x63, 0x72, 0xd9, 0x7c , 0x46, 0x87,
    //                        0xc5, 0x63, 0x72, 0xd9, 0x7c , 0x46, 0x87, 0xc5, 0x63, 0x72, 0xd9, 0x7c , 0x46, 0x87,
    //                        0xc5, 0x63, 0x72, 0xd9, 0x7c , 0x46, 0x87, 0xc5, 0x63, 0x72, 0xd9, 0x7c , 0x46, 0x03

    //                     };

    // pdu->N_bytes = sizeof(test_long_sms);
    // memcpy(pdu->msg, test_long_sms, pdu->N_bytes);

    switch (pdu->msg[2])
    {
    case TC_MSG_ATE_IMS_MT_CALL:
      srsran::console("ims_ate_msg_type: TC_MSG_ATE_IMS_MT_CALL \n");
      if (pdu->msg[3] == set_up_mt_call && pdu->N_bytes == 5)
      {
        srsran::console("command: set_up_mt_call \n");
        ims_ctx->ims_ctx.call_code_rate = pdu->msg[4];
        ims_ctx->ims_ctx.ate_indication = set_up_mt_call;
        pack_pcs_ue_call_setup(ims_tx, ims_ctx);
        /* send message to RRC message queue. */
        send_ims_dl_msg(pcs_rnti, std::move(ims_tx));
      }
      else if (pdu->msg[3] == release_mt_call && pdu->N_bytes == 5)
      {
        srsran::console("command: release_mt_call \n");
        pack_call_release_req(ims_tx, ims_ctx);
        send_ims_dl_msg(pcs_rnti, std::move(ims_tx));

        ims_ate_tx->N_bytes = 3;
        ims_ate_tx->msg[0] = 0xff;
        ims_ate_tx->msg[1] = 0x00;
        ims_ate_tx->msg[2] = TC_MSG_ATE_IMS_MT_CALL_RES;
        // send to ate msg queue
        srsran::console("send ims_msg to ate queue \n");
        m_cnw->cnw_adp.udp_.send_ate_msg(std::move(ims_ate_tx));
      }
      else if (pdu->msg[3] == call_code_rate && pdu->N_bytes == 5)
      {
        srsran::console("command: call_code_rate \n");
        ims_ctx->ims_ctx.call_code_rate = pdu->msg[4];
        ims_ctx->ims_ctx.ate_indication = call_code_rate;

        ims_ate_tx->N_bytes = 3;
        ims_ate_tx->msg[0] = 0xff;
        ims_ate_tx->msg[1] = 0x00;
        ims_ate_tx->msg[2] = TC_MSG_ATE_IMS_MT_CALL_RES;
        // send to ate msg queue
        srsran::console("send ims_msg to ate queue \n");
        m_cnw->cnw_adp.udp_.send_ate_msg(std::move(ims_ate_tx));
      }
      else
      {
        srsran::console("TC_MSG_ATE_IMS_MT_CALL Unknown indicate type (%d)", pdu->msg[1]);
        return false;
      }
      break;

    case TC_MSG_ATE_IMS_MT_SMS:
      srsran::console("ims_ate_msg_type: TC_MSG_ATE_IMS_MT_SMS \n");
      if (pdu->N_bytes < 6)
      {
        srsran::console("error: ims received ate mt sms size < 5\n");
        return false;
      }
      ims_ctx->ims_ctx.sms_code_form = pdu->msg[3];
      {
        uint16_t ate_sms_length = (int)pdu->msg[4] << 8 & 0xff00;
        ate_sms_length = ate_sms_length | pdu->msg[5];
        if (pdu->N_bytes - 6 != ate_sms_length)
        {
          pcs_ims_logger.error("TC_MSG_ATE_IMS_MT_SMS message length indication error!");
          srsran::console("TC_MSG_ATE_IMS_MT_SMS message length indication error!\\n");
          return false;
        }
        if (ate_sms_length > 140)
        {
          ims_ctx->ims_ctx.sms_type = LONG_SMS;
          for (int i = 0; i < (int)ate_sms_length; i++)
            ims_ctx->ims_ctx.ate_mt_long_message_content.push_back(pdu->msg[6 + i]); // long sms
        }
        else
        {
          ims_ctx->ims_ctx.sms_type = SHORT_SMS;
          for (int i = 0; i < (int)ate_sms_length; i++)
            ims_ctx->ims_ctx.ate_mt_sms_content.push_back(pdu->msg[6 + i]); // short sms
        }
      }
      pack_mt_sms_req(ims_tx, ims_ctx, 1);
      send_ims_dl_msg(pcs_rnti, std::move(ims_tx));
      break;

    default:
      pcs_ims_logger.error("Unknown message type (%d) or not implemented", pdu->msg[2]);
      srsran::console("Unknown message type (%d)", pdu->msg[2]);
      return false;
      break;
    }

    return true;
  }

  // handle the data/msg of rrc to pcs
  bool pcs_ims::pcs_handle_ulinformation(uint16_t rnti, srsran::unique_byte_buffer_t pdu)
  {
    pcs_ims_logger.info("----------this is pcs_handle_ulinformation function------------");
    // print the data/msg
    int pdu_length = pdu->N_bytes;
    for (int j = 0; j < pdu_length; ++j)
    {
      srsran::console("ims received ulmsg[%d] =%x\n", j, pdu->msg[j]);
    }

    pcs_ims_msg ims_msg;

    /* 1. message decode */
    if (SRSRAN_SUCCESS != ims_msg.unpack(pdu))
    {
      pcs_ims_logger.error("Failed to unpack!");
      srsran::console("Failed to unpack!\n");
      return false;
    }

    ims_context *ims_ctx = find_ims_ctx_from_rnti(rnti);

    // abnormal handle
    if (ims_ctx == NULL)
    {
      if (ims_msg.hdr.ims_message_type.value == ims_msg_types::options::voice_register_req)
      {
        srsran::console(" receive voice_register_req msg but no find_ims_ctx_from_rnti!\n");
        pcs_ims_logger.info("This is the first receive voice_register_req !");
      }
      else
      {
        pcs_ims_logger.error("Don't find_ims_ctx_from_rnti and msg_type isn't voice_register_req msg");
        return false;
      }
    }
    else
    {
      if (ims_ctx->ims_ctx.state != IMS_STATE_REGISTERED && ims_msg.hdr.ims_message_type.value != ims_msg_types::options::voice_register_req)
      {
        pcs_ims_logger.error(" User state is deregistered, but received the msg type is %s", ims_msg.hdr.ims_message_type.to_string());
        return false;
      }
    }
    srsran::console("Received the msg type is %s", ims_msg.hdr.ims_message_type.to_string());
    pcs_ims_logger.info("Received the msg type is %s", ims_msg.hdr.ims_message_type.to_string());

    /* 2. message handler */
    switch (ims_msg.hdr.ims_message_type)
    {
    case srsran::ims::ims_msg_types::voice_register_req:
      handle_Voice_Register_Req(rnti, ims_msg.voice_register());
      break;
    case srsran::ims::ims_msg_types::call_setup:
      handle_ue_pcs_call_setup(rnti, ims_msg.ue_pcs_call_setup());
      break;
    case srsran::ims::ims_msg_types::call_connect_ack:
      handle_call_connect_ack(rnti, ims_msg.call_connect_ack());
      break;
    case srsran::ims::ims_msg_types::call_disconnect:
      handle_call_disconnect(rnti, ims_msg.call_disconnect());
      break;
    case srsran::ims::ims_msg_types::call_release_resp:
      handle_call_release_resp(rnti, ims_msg.call_release_resp());
      break;
    case srsran::ims::ims_msg_types::call_setup_ack:
      handle_call_setup_ack(rnti, ims_msg.call_setup_ack());
      break;
    case srsran::ims::ims_msg_types::call_confirmed:
      handle_call_comfirmed(rnti, ims_msg.call_confirmed());
      break;
    case srsran::ims::ims_msg_types::call_alerting:
      handle_ue_pcs_call_alerting(rnti, ims_msg.ue_pcs_call_alerting());
      break;
    case srsran::ims::ims_msg_types::call_connect:
      handle_ue_pcs_call_connect(rnti, ims_msg.ue_pcs_call_connect());
      break;
    case srsran::ims::ims_msg_types::mo_sms_req:
      handle_mo_sms_req(rnti, ims_msg.mo_sms_req());
      break;
    case srsran::ims::ims_msg_types::mt_sms_resp:
      handle_mt_sms_resp(rnti, ims_msg.mt_sms_resp());
      break;
    case srsran::ims::ims_msg_types::voice_deregister_req:
      handle_Voice_DeRegister_Req(rnti, ims_msg.voice_deregister_req());
      break;

    default:
      srsran::console("Unknown message type (%s) or not implemented", ims_msg.hdr.ims_message_type.to_string());
      pcs_ims_logger.error("Unknown message type (%s) or not implemented", ims_msg.hdr.ims_message_type.to_string());
      break;
    }

    return true;
  }

  bool pcs_ims::add_ims_ctx_to_rnti_map(ims_context *ims_ctx)
  {
    std::map<uint16_t, ims_context *>::iterator ctx_it = rnti_to_ims_ctx.find(ims_ctx->ims_ctx.rnti);
    if (ctx_it != rnti_to_ims_ctx.end())
    {
      pcs_ims_logger.error("UE Context already exists. RNTI %015" PRIu64 "", ims_ctx->ims_ctx.rnti);
      srsran::console(" The UE's ims Context already exists. RNTI %d\n", ims_ctx->ims_ctx.rnti);
      return false;
    }

    rnti_to_ims_ctx.insert(std::pair<uint16_t, ims_context *>(ims_ctx->ims_ctx.rnti, ims_ctx));
    pcs_ims_logger.debug("Saved UE context corresponding to RNTI %015" PRIu64 "", ims_ctx->ims_ctx.rnti);
    srsran::console(" The UE's ims Context remains successful. RNTI %d\n", ims_ctx->ims_ctx.rnti);
    return true;
  }

  ims_context *pcs_ims::find_ims_ctx_from_rnti(uint16_t rnti)
  {
    std::map<uint16_t, ims_context *>::iterator iter = rnti_to_ims_ctx.find(rnti);
    if (iter == rnti_to_ims_ctx.end())
    {
      return NULL;
    }
    else
    {
      return iter->second;
    }
  }

  bool pcs_ims::delete_ue_ims_ctx(uint16_t rnti)
  {
    ims_context *ims_ctx = find_ims_ctx_from_rnti(rnti);
    if (ims_ctx == nullptr)
    {
      cout << "no find the ims context from rnti: " << rnti << endl;
      return false;
    }
    rnti_to_ims_ctx.erase(rnti);
    delete ims_ctx;
    return true;
  }

  bool pcs_ims::handle_Voice_Register_Req(uint16_t rnti, voice_register_req_t &msg)
  {
    m_cnw->cnw_adp.udp_.enter_Voice_Register_Req_num++;
    srsran::console("This is handle_Voice_Register_Req function!\n");

    ims_context *ims_ctx = find_ims_ctx_from_rnti(rnti);
    srsran::unique_byte_buffer_t ims_tx;
    if (ims_ctx == NULL)
    {
      ims_ctx = new ims_context();
      srsran::console(" no find_ims_ctx_from_rnti!\n"); // the first register
    }

    ims_ctx->ims_ctx.s_tmsi_5g = msg.s_tmsi_5g;

    if (msg.security_indicator_ims.security_indicator.value == FIRST_REG_OR_DEREG)
    {

      ims_ctx->ims_ctx.rnti = rnti;

      std::cout << " This is the first reg msg handle " << std::endl;

      printf("msg.s_tmsi_5g_value1: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[0]);
      printf("msg.s_tmsi_5g_value2: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[1]);
      printf("msg.s_tmsi_5g_value3: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[2]);
      printf("msg.s_tmsi_5g_value4: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[3]);
      printf("msg.s_tmsi_5g_value5: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[4]);
      printf("msg.s_tmsi_5g_value6: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[5]);

      printf("msg.reg_type_ims: %s\n", msg.reg_type_ims.reg_type.to_string());
      printf("msg.security_indicator_ims.security_indicator.value: %d\n", msg.security_indicator_ims.security_indicator.value);
      printf("msg.msg.imsi.imsi_5: %d\n", msg.imsi.imsi[4]);
      printf("msg.msg.imsi.imsi_4: %d\n", msg.imsi.imsi[5]);
      printf("msg.msg.imsi.imsi_7: %d\n", msg.imsi.imsi[8]);
      printf("msg.msg.imsi.imsi_8: %d\n", msg.imsi.imsi[9]);

      /*handle the first reg msg*/

      // ue identity info check  未完成

      /*construct aka command message. */
      ims_tx = srsran::make_byte_buffer();
      if (ims_tx == nullptr)
      {
        srsran::console("Couldn't allocate PDU in handle_Voice_Register_Req \n");
        return false;
      }
      pack_Authentication_Command(ims_tx, ims_ctx);
      ims_tx->msg[1] = ims_tx->N_bytes; // update ims msg_length
      /* save the ims context. */
      add_ims_ctx_to_rnti_map(ims_ctx);

      srsran::console("Downlink IMS: Sending Authentication command\n");
    }
    // the second reg message handle (include security response)
    else
    {
      std::cout << " This is the second reg msg handle " << std::endl;

      printf("msg.s_tmsi_5g_value1: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[0]);
      printf("msg.s_tmsi_5g_value2: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[1]);
      printf("msg.s_tmsi_5g_value3: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[2]);
      printf("msg.s_tmsi_5g_value4: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[3]);
      printf("msg.s_tmsi_5g_value5: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[4]);
      printf("msg.s_tmsi_5g_value6: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[5]);

      printf("msg.reg_type_ims: %s\n", msg.reg_type_ims.reg_type.to_string());
      printf("msg.security_indicator_ims.security_indicator.value: %d\n", msg.security_indicator_ims.security_indicator.value);
      printf("msg.msg.imsi.imsi_5: %d\n", msg.imsi.imsi[4]);
      printf("msg.msg.imsi.imsi_4: %d\n", msg.imsi.imsi[5]);
      printf("msg.msg.imsi.imsi_7: %d\n", msg.imsi.imsi[8]);
      printf("msg.msg.imsi.imsi_8: %d\n", msg.imsi.imsi[9]);

      /*handle the second reg msg*/

      // ue identity info check  未完成

      uint8_t default_sercurity_response[] = {
          0x38, 0x32, 0x35, 0x32, 0x35, 0x37, 0x66, 0x35, 0x36, 0x62, 0x63, 0x33, 0x34, 0x35, 0x39, 0x62,
          0x61, 0x30, 0x66, 0x34, 0x66, 0x34, 0x30, 0x30, 0x65, 0x30, 0x36, 0x37, 0x62, 0x64, 0x62, 0x36};
      ims_ctx->ims_ctx.cause = cause_t::cause_type_::options::success;
      for (int i = 0; i < 32; ++i)
      { // check sercurity response
        if (msg.security_response.security_response[i] != default_sercurity_response[i])
        {
          printf("msg.security_response.security_response[%d] : %x\n", i, msg.security_response.security_response[i]);
          printf("default_sercurity_response[%d] : %x\n", i, default_sercurity_response[i]);
          ims_ctx->ims_ctx.cause = cause_t::cause_type_::options::auth_failure;
          break;
        }
      }

      /*construct aka command message. */
      ims_tx = srsran::make_byte_buffer();
      if (ims_tx == nullptr)
      {
        srsran::console("Couldn't allocate PDU in handle_Voice_Register_Req \n");
        return false;
      }
      pack_Voice_Register_Rsp(ims_tx, ims_ctx);
      ims_tx->msg[1] = ims_tx->N_bytes; // update ims msg_length

      /*autn successful*/
      if (ims_ctx->ims_ctx.cause == cause_t::cause_type_::options::success)
      {
        // register rsp ,ims into IMS_STATE_REGISTERED
        ims_ctx->ims_ctx.state = IMS_STATE_REGISTERED;
        this->pcs_rnti = rnti;
        srsran::console("Downlink IMS: Sending Voice Register Rsp\n");

        /*send message to ate infrom ims register status   4.22 */
        srsran::unique_byte_buffer_t ims_ate_tx = srsran::make_byte_buffer();
        if (ims_ate_tx == nullptr)
        {
          srsran::console("Couldn't allocate PDU in TC_MSG_IMS_ATE_REGISTER \n");
          return false;
        }
        ims_ate_tx->N_bytes = 4;
        ims_ate_tx->msg[0] = 0xff;
        ims_ate_tx->msg[1] = 0x00;
        ims_ate_tx->msg[2] = TC_MSG_IMS_ATE_REGISTER;
        ims_ate_tx->msg[3] = IMS_STATE_REGISTERED;

        // send to ate msg queue
        srsran::console("send ims_msg to ate queue \n");
        m_cnw->cnw_adp.udp_.send_ate_msg(std::move(ims_ate_tx));
      }
      else
      { // autn failed
        delete_ue_ims_ctx(rnti);
      }
    }

    /*print msg ie*/
    int msg_len = ims_tx->N_bytes;
    for (int i = 0; i < msg_len; i++)
    {
      printf("ims_msg[%d]=%x", i, ims_tx->msg[i]);
      printf("\n");
    }

    /* send message to RRC message queue. */
    send_ims_dl_msg(rnti, std::move(ims_tx));
    std::cout << "m_cnw->cnw_adp.udp_.enter_Voice_Register_Req_num" << m_cnw->cnw_adp.udp_.enter_Voice_Register_Req_num << std::endl;
    if (m_cnw->cnw_adp.udp_.enter_Voice_Register_Req_num == 2)
    {
      // //--------2024/06/27------ho add----------
      //   auto start_time = std::chrono::high_resolution_clock::now();
      //   while (true)
      //   {
      //     auto now_time = std::chrono::high_resolution_clock::now();
      //     auto duration = std::chrono::duration_cast<std::chrono::seconds>(now_time - start_time);
      //     if (duration.count() > 3)
      //     {
      //       break;
      //     }
      //   }
      // m_cnw->m_nas_mm->rrc_mm->wx_Mcontrol_Notify_Switch(rnti);
      // //-----------------------------------------
      std::cout << "m_cnw->cnw_adp.udp_.is_voice_complete" << m_cnw->cnw_adp.udp_.is_voice_complete << std::endl;
#if 0 // reestablish hm
      auto start_time = std::chrono::high_resolution_clock::now();
      while (true)
      {
        auto now_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now_time - start_time);
        if (duration.count() > 2)
        {
          std::cout << " 2 !!!" << std::endl;
          break;
        }
      }
      srsran::unique_byte_buffer_t enb_pdu = srsran::make_byte_buffer();
      enb_msg_header_t s1_header;
      s1_header.enb_id = ims_adp->udp_.enb_id;
      s1_header.rnti = 70;
      s1_header.msg_type = srsepc::enb_msg_type::rrc_rebuild_fail;
      int len = sizeof(s1_header);
      memcpy(enb_pdu->msg, &s1_header, len);
      enb_pdu->N_bytes = len;
      ims_adp->udp_.send_enb_msg(std::move(enb_pdu));
      printf("s1_header.msg_type:%d  len%d",s1_header.msg_type,len);
#endif
      if (m_cnw->cnw_adp.udp_.is_voice_complete || m_cnw->cnw_adp.udp_.is_ue_config_update)
      {
        printf(" tend voice complete info to TTCN ");
        auto start_time = std::chrono::high_resolution_clock::now();
        while (true)
        {
          auto now_time = std::chrono::high_resolution_clock::now();
          auto duration = std::chrono::duration_cast<std::chrono::seconds>(now_time - start_time);
          if (duration.count() > 2)
          {
            std::cout << " 2 !!!" << std::endl;
            break;
          }
        }

        srsran::unique_byte_buffer_t voice_to_ttcn_pdu = srsran::make_byte_buffer();
        voice_to_ttcn_pdu->N_bytes = 4;
        for (uint32_t i = 0; i < voice_to_ttcn_pdu->N_bytes; i++)
        {
          voice_to_ttcn_pdu->msg[i] = 0x00;
        }
        m_cnw->cnw_adp.udp_.send_ttcn_msg_enb(std::move(voice_to_ttcn_pdu));
        printf(" Send voice complete info to TTCN ");

        if(m_cnw->cnw_adp.udp_.is_ue_config_update) {
          while(true)
          {
            srsran::unique_byte_buffer_t udp = srsran::make_byte_buffer();
            if(udp == NULL) {
              pcs_ims_logger.error("");
              return false;
            }
            if(ims_adp->udp_.nas_mm_receive_info.size()!=0)
            {
              ims_adp->udp_.nas_mm_receive_info.try_pop(udp);
              break;
            }
          }
          m_cnw->m_nas_mm->send_configuration_update_command_5g_guti_ttcn(rnti);
        }
      }

      /* ttcn 8_17  */
      if(m_cnw->cnw_adp.udp_.is_authencation_faileure_repeated_ngksi || m_cnw->cnw_adp.udp_.is_ue_send_service_request) 
      {
 
        /*wait ttcn send rrc release or wait 1 second*/
        auto start_time = std::chrono::high_resolution_clock::now();
        while (true)
        {
          auto now_time = std::chrono::high_resolution_clock::now();
          auto duration = std::chrono::duration_cast<std::chrono::seconds>(now_time - start_time);
          if (duration.count() > 1)
          {
            break;
          }
        }
          m_cnw->m_nas_mm->send_start_rem_rel_user(rnti);
          if(m_cnw->cnw_adp.udp_.is_ue_send_service_request)
          {
       
          srsran::unique_byte_buffer_t control_send_AT=srsran::make_byte_buffer();
          control_send_AT->msg[0]=0x01;
          control_send_AT->msg[1]=0x02;
          control_send_AT->msg[2]=0x01;
          control_send_AT->msg[3]=0x02;
          control_send_AT->N_bytes=4;
          m_cnw->cnw_adp.udp_.send_ttcn_msg_enb(std::move(control_send_AT));  
          }
      }

      /* ttcn 8_17  */
      if(m_cnw->cnw_adp.udp_.is_periodic_registration_request) 
      {
      
        /*wait ttcn send rrc release or wait 1 second*/
        auto start_time = std::chrono::high_resolution_clock::now();
        while (true)
        {
          auto now_time = std::chrono::high_resolution_clock::now();
          auto duration = std::chrono::duration_cast<std::chrono::seconds>(now_time - start_time);
          if (duration.count() > 1)
          {
            break;
          }
        }
        m_cnw->m_nas_mm->send_start_rem_rel_user(rnti);

        srsran::unique_byte_buffer_t voice_to_ttcn_pdu = srsran::make_byte_buffer();
        voice_to_ttcn_pdu->N_bytes = 4;
        for (uint32_t i = 0; i < voice_to_ttcn_pdu->N_bytes; i++)
        {
          voice_to_ttcn_pdu->msg[i] = 0x00;
        }
        m_cnw->cnw_adp.udp_.send_ttcn_msg_enb(std::move(voice_to_ttcn_pdu));
        printf(" Send voice complete info to TTCN ");

      }
      
    }

    /************************测试mt_sms**************************/
    if (msg.security_indicator_ims.security_indicator.value != FIRST_REG_OR_DEREG)
    {
      ims_tx = srsran::make_byte_buffer();
      if (ims_tx == nullptr)
      {
        srsran::console("Couldn't allocate PDU in handle_mt_sms_req \n");
        return false;
      }
      switch (sms_call_control)
      {
      case SMS_CALL_UE_TO_NETWOEK:
        // 主叫业务
        break;

      case CALL_NETWORE_TO_UE: // 被叫语音业务
        sleep(1);
        pack_pcs_ue_call_setup(ims_tx, ims_ctx);
        ims_tx->msg[1] = ims_tx->N_bytes; // update ims msg_length
        // rrc_pcs_ims->s_write_ims_dl_info(rnti, std::move(ims_tx));
        send_ims_dl_msg(rnti, std::move(ims_tx));
        srsran::console("Downlink IMS: Sending pcs_ue_call_setup\n");
        break;

      case SMS_NETWORE_TO_UE: // 被叫短信业务
        sleep(1);
        pack_mt_sms_req(ims_tx, ims_ctx, 1);
        ims_tx->msg[1] = ims_tx->N_bytes; // update ims msg_length
        // rrc_pcs_ims->s_write_ims_dl_info(rnti, std::move(ims_tx));
        send_ims_dl_msg(rnti, std::move(ims_tx));
        srsran::console("Downlink IMS: Sending pack_mt_sms_req\n");
        break;

      default:
        break;
      }
    }
    /***************************end******************************/

    return true;
  } // handle_Voice_Register_Req

  /*********************2024-3-11******************/
  bool pcs_ims::handle_Voice_DeRegister_Req(uint16_t rnti, voice_deregister_req_t &msg)
  {
    std::cout << "This is handle_Voice_DeRegister_Req function!" << std::endl;

    ims_context *ims_ctx;
    srsran::unique_byte_buffer_t ims_tx; // 创建一个新的IMS上下文对象ims_ctx和一个独立的字节缓冲区ims_tx。
    if (find_ims_ctx_from_rnti(rnti) == NULL)
    {
      srsran::console("error: handle_Voice_DeRegister_Req but no find_ims_ctx_from_rnti!\n");
      return false;
    }
    else
    {
      ims_ctx = find_ims_ctx_from_rnti(rnti);
    }

    /*judge s_tmsi_5g*/
    for (int i = 0; i < 6; ++i)
    {
      if (ims_ctx->ims_ctx.s_tmsi_5g.s_tmsi_5g_value[i] != msg.s_tmsi_5g.s_tmsi_5g_value[i])
      {
        srsran::console("error: handle_Voice_DeRegister_Req but s_tmsi_5g error!\n");
        return false;
      }
    }
    /*judge s_tmsi_5g*/
    if (msg.dereg_type_ims.dereg_type != dereg_type_t::dereg_type_type_::user_dereg)
    {
      srsran::console("error: handle_Voice_DeRegister_Req but dereg_type isn't user_dereg!\n");
      return false;
    }

    if (msg.security_indicator_ims.security_indicator.value == FIRST_REG_OR_DEREG)
    {

      ims_ctx->ims_ctx.rnti = rnti; // 将变量rnti的值赋给IMS上下文对象中的rnti字段，实现了将特定RNTI与当前处理的IMS上下文对象关联起来。

      std::cout << " This is dereg msg handle " << std::endl;

      printf("msg.s_tmsi_5g_value1: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[0]);
      printf("msg.s_tmsi_5g_value2: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[1]);
      printf("msg.s_tmsi_5g_value3: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[2]);
      printf("msg.s_tmsi_5g_value4: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[3]);
      printf("msg.s_tmsi_5g_value5: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[4]);
      printf("msg.s_tmsi_5g_value6: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[5]);

      printf("msg.dereg_type_ims: %s\n", msg.dereg_type_ims.dereg_type.to_string());
      printf("msg.dereg_type_ims: %s\n", msg.cause_ims.cause.to_string());
      printf("msg.security_indicator_ims.security_indicator.value: %d\n", msg.security_indicator_ims.security_indicator.value);

      /*construct Voice DeRegister Rsp message. */
      ims_tx = srsran::make_byte_buffer();
      if (ims_tx == nullptr)
      {
        srsran::console("Couldn't allocate PDU in handle_Voice_DeRegister_Req \n");
        return false;
      }
      pack_Voice_DeRegister_Rsp(ims_tx, ims_ctx);
      /* update ims msg_length. 更新消息的第二个字节 (索引为1) 为当前消息的总字节数，用于标识实际消息长度. */
      ims_tx->msg[1] = ims_tx->N_bytes;
      /* save the ims context. */
      // 等待IMS的回应
      srsran::console("Downlink IMS: Sending Voice_DeRegister_Rsp\n");
    }

    /*print msg ie*/
    int msg_len = ims_tx->N_bytes;
    for (int i = 0; i < msg_len; i++)
    {
      printf("ims_msg[%d]=%x", i, ims_tx->msg[i]);
      printf("\n");
    }

    /* send message to RRC message queue. */
    // rrc_pcs_ims->s_write_ims_dl_info(rnti, std::move(ims_tx));
    send_ims_dl_msg(rnti, std::move(ims_tx));

    /*delete ims context*/
    delete_ue_ims_ctx(rnti);

    return true;
  } // handle_Voice_DeRegister_Req

  bool pcs_ims::handle_ue_pcs_call_setup(uint16_t rnti, ue_pcs_call_setup_t &msg)
  {
    std::cout << "This is handle_ue_pcs_call_setup function!" << std::endl;

    ims_context *ims_ctx = new ims_context();
    srsran::unique_byte_buffer_t ims_tx; // 创建一个新的IMS上下文对象ims_ctx和一个独立的字节缓冲区ims_tx
    if (find_ims_ctx_from_rnti(rnti) == NULL)
    {
      srsran::console(" no find_ims_ctx_from_rnti!\n");
      return false;
    }
    else
    {
      ims_ctx = find_ims_ctx_from_rnti(rnti);
    }

    ims_ctx->ims_ctx.s_tmsi_5g = msg.s_tmsi_5g;

    ims_ctx->ims_ctx.rnti = rnti; // 将变量rnti的值赋给IMS上下文对象中的rnti字段，实现了将特定RNTI与当前处理的IMS上下文对象关联起来。

    std::cout << " This is Call Setup msg handle " << std::endl;

    printf("msg.s_tmsi_5g_value1: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[0]);
    printf("msg.s_tmsi_5g_value2: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[1]);
    printf("msg.s_tmsi_5g_value3: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[2]);
    printf("msg.s_tmsi_5g_value4: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[3]);
    printf("msg.s_tmsi_5g_value5: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[4]);
    printf("msg.s_tmsi_5g_value6: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[5]);

    printf("msg.call_type_ims: %s\n", msg.call_type_ims.call_type.to_string());

    printf("msg.called_party_bcd_num: ");
    for (uint8_t num : msg.called_party_bcd_num.called_party_bcd_num)
    {
      printf("%x ", num);
    }
    printf("\n");

    printf("msg.ttot_call_indicator_ims.ttot_call_indicator.value: %d\n", msg.ttot_call_indicator_ims.ttot_call_indicator.value);
    printf("msg.call_code_rate_ims.call_code_rate.value: %d\n", msg.call_code_rate_ims.call_code_rate.value);

    /*After converting the 100Trying message sent by IMS,construct call_setup_ack message. */
    ims_tx = srsran::make_byte_buffer();
    if (ims_tx == nullptr)
    {
      srsran::console("Couldn't allocate PDU in handle_Call_setup_ack \n");
      return false;
    }
    pack_call_setup_ack(ims_tx, ims_ctx);
    /* update ims msg_length. 更新消息的第二个字节 (索引为1) 为当前消息的总字节数，用于标识实际消息长度. */
    ims_tx->msg[1] = ims_tx->N_bytes;
    srsran::console("Downlink IMS: Sending call setup ack\n");

    /* send message to RRC message queue. */
    // rrc_pcs_ims->s_write_ims_dl_info(rnti, std::move(ims_tx));
    send_ims_dl_msg(rnti, std::move(ims_tx));

    // 1.2v modify
    m_cnw->m_nas_mm->pcs_sm_modification(rnti, 3);

    /*After converting the 180Ringing message sent by IMS,construct call_alerting message. */
    srsran::console("wcb__-111111s\n");
    ims_tx = srsran::make_byte_buffer();
    if (ims_tx == nullptr)
    {
      srsran::console("Couldn't allocate PDU in handle_call_alerting \n");
      return false;
    }
    pack_pcs_ue_call_alerting(ims_tx, ims_ctx);
    /* update ims msg_length. 更新消息的第二个字节 (索引为1) 为当前消息的总字节数，用于标识实际消息长度. */
    ims_tx->msg[1] = ims_tx->N_bytes;
    srsran::console("Downlink IMS: Sending call alerting\n");

    /* send message to RRC message queue. */
    // rrc_pcs_ims->s_write_ims_dl_info(rnti, std::move(ims_tx));
    send_ims_dl_msg(rnti, std::move(ims_tx));

    /*After converting the 200ok message sent by IMS,construct call_connect message. */
    ims_tx = srsran::make_byte_buffer();
    if (ims_tx == nullptr)
    {
      srsran::console("Couldn't allocate PDU in handle_call_connect \n");
      return false;
    }
    pack_pcs_ue_call_connect(ims_tx, ims_ctx);
    /* update ims msg_length. 更新消息的第二个字节 (索引为1) 为当前消息的总字节数，用于标识实际消息长度. */
    ims_tx->msg[1] = ims_tx->N_bytes;
    srsran::console("Downlink IMS: Sending call connect\n");

    /*print msg ie*/
    // int msg_len = ims_tx->N_bytes;
    // for(int i=0; i<msg_len; i++){
    //   printf("ims_msg[%d]=%x", i, ims_tx->msg[i]);
    //   printf("\n");
    // }

    /* send message to RRC message queue. */
    // rrc_pcs_ims->s_write_ims_dl_info(rnti, std::move(ims_tx));
    send_ims_dl_msg(rnti, std::move(ims_tx));

    return true;
  } // handle_ue_pcs_call_setup

  bool pcs_ims::handle_call_connect_ack(uint16_t rnti, call_connect_ack_t &msg)
  {
    std::cout << "This is handle_call_connect_ack function!" << std::endl;

    ims_context *ims_ctx = new ims_context();
    srsran::unique_byte_buffer_t ims_tx; // 创建一个新的IMS上下文对象ims_ctx和一个独立的字节缓冲区ims_tx。
    if (find_ims_ctx_from_rnti(rnti) == NULL)
    {
      srsran::console(" no find_ims_ctx_from_rnti!\n");
      return false;
    }
    else
    {
      ims_ctx = find_ims_ctx_from_rnti(rnti);
    }

    ims_ctx->ims_ctx.rnti = rnti; // 将变量rnti的值赋给IMS上下文对象中的rnti字段，实现了将特定RNTI与当前处理的IMS上下文对象关联起来。

    std::cout << " This is Call Connect Ack msg handle " << std::endl;
    
    printf("Generated unique Call ID (call Connect ack): %x\n", msg.call_id.call_id_value); // 这个由上行的信息流确定了，有待更改

    srsran::console("Uplink IMS: Call Connect Ack 通话建立完成\n");

    // mm 触发重配过程 暂时打桩写死  此时应该对注册过程对建立的会话资源进行重配
    // ims->sm
    m_cnw->m_nas_mm->pcs_sm_modification(rnti, 1);
    ims_ctx->ims_ctx.call_status_type = MO_CALL_TYPE;
    /*send message to ate infrom ims mo status   4.22 */

    ims_tx = srsran::make_byte_buffer();
    if (ims_tx == nullptr)
    {
      srsran::console("Couldn't allocate PDU in TC_MSG_IMS_ATE_MO_CALL \n");
      return false;
    }
    ims_tx->N_bytes = 4;
    ims_tx->msg[0] = 0xff;
    ims_tx->msg[1] = 0x00;
    ims_tx->msg[2] = TC_MSG_IMS_ATE_MO_CALL;
    ims_tx->msg[3] = 0x01;

    // send to ate msg queue
    srsran::console("send ims_msg to ate queue \n");
    // m_cnw->cnw_adp.udp_.send_ate_info.try_push(std::move(ims_tx));
    m_cnw->cnw_adp.udp_.send_ate_msg(std::move(ims_tx));

    return true;
  } // handle_call_connect_ack

  bool pcs_ims::handle_call_disconnect(uint16_t rnti, call_disconnect_t &msg)
  {
    std::cout << "This is handle_call_disconnect function!" << std::endl;

    ims_context *ims_ctx = new ims_context();
    srsran::unique_byte_buffer_t ims_tx; // 创建一个新的IMS上下文对象ims_ctx和一个独立的字节缓冲区ims_tx。
    if (find_ims_ctx_from_rnti(rnti) == NULL)
    {
      srsran::console(" no find_ims_ctx_from_rnti!\n");
    }
    else
    {
      ims_ctx = find_ims_ctx_from_rnti(rnti);
    }

    ims_ctx->ims_ctx.rnti = rnti; // 将变量rnti的值赋给IMS上下文对象中的rnti字段，实现了将特定RNTI与当前处理的IMS上下文对象关联起来。

    std::cout << " This is Call disConnect  msg handle " << std::endl;

    printf("Generated unique Call ID (call disconnect): %x\n", msg.call_id.call_id_value);
    ims_ctx->ims_ctx.call_id = msg.call_id.call_id_value;
    printf("call disconnect cause: %s\n", msg.cause_ims.cause.to_string());
    srsran::console("Uplink IMS: Call Disconnect \n");

    ims_tx = srsran::make_byte_buffer();
    if (ims_tx == nullptr)
    {
      srsran::console("Couldn't allocate PDU in handle_Call_release_req \n");
      return false;
    }
    pack_call_release_req(ims_tx, ims_ctx);
    /* update ims msg_length. 更新消息的第二个字节 (索引为1) 为当前消息的总字节数，用于标识实际消息长度. */
    ims_tx->msg[1] = ims_tx->N_bytes;
    srsran::console("Downlink IMS: Sending call release req\n");
    /*print msg ie*/
    int msg_len = ims_tx->N_bytes;
    for (int i = 0; i < msg_len; i++)
    {
      printf("ims_msg[%d]=%x", i, ims_tx->msg[i]);
      printf("\n");
    }

    /* send message to RRC message queue. */
    send_ims_dl_msg(rnti, std::move(ims_tx));

    return true;
  } // handle_call_disconnect

  bool pcs_ims::handle_call_release_resp(uint16_t rnti, call_release_resp_t &msg)
  {
    std::cout << "This is handle_call_release_resp function!" << std::endl;

    ims_context *ims_ctx;
    srsran::unique_byte_buffer_t ims_tx; // 创建一个新的IMS上下文对象ims_ctx和一个独立的字节缓冲区ims_tx。
    if (find_ims_ctx_from_rnti(rnti) == NULL)
    {
      srsran::console(" no find_ims_ctx_from_rnti!\n");
      return false;
    }
    else
    {
      ims_ctx = find_ims_ctx_from_rnti(rnti);
    }

    ims_ctx->ims_ctx.rnti = rnti; // 将变量rnti的值赋给IMS上下文对象中的rnti字段，实现了将特定RNTI与当前处理的IMS上下文对象关联起来。

    std::cout << " This is Call Release Resp  msg handle " << std::endl;

    printf("Generated unique Call ID (call release resp): %x\n", msg.call_id.call_id_value);
    printf("call disconnect cause: %s\n", msg.cause_ims.cause.to_string());
    srsran::console("Uplink IMS: Call  release resp \n");

    // ims->sm
    m_cnw->m_nas_mm->pcs_sm_modification(rnti, 2);
    /*send message to ate infrom ims status   4.22 */
    ims_tx = srsran::make_byte_buffer();
    if (ims_tx == nullptr)
    {
      srsran::console("Couldn't allocate PDU in TC_MSG_IMS_ATE_MT_or_MO_CALL \n");
      return false;
    }
    switch (ims_ctx->ims_ctx.call_status_type)
    {
    case MO_CALL_TYPE:
      ims_tx->N_bytes = 4;
      ims_tx->msg[0] = 0xff;
      ims_tx->msg[1] = 0x00;
      ims_tx->msg[2] = TC_MSG_IMS_ATE_MO_CALL;
      ims_tx->msg[3] = 0x02;
      break;
    case MT_CALL_TYPE:
      ims_tx->N_bytes = 4;
      ims_tx->msg[0] = 0xff;
      ims_tx->msg[1] = 0x00;
      ims_tx->msg[2] = TC_MSG_IMS_ATE_MT_CALL;
      ims_tx->msg[3] = 0x02;
      break;
    default:
      break;
    }
    // send to ate msg queue
    srsran::console("send ims_msg to ate queue \n");
    // m_cnw->cnw_adp.udp_.send_ate_info.try_push(std::move(ims_tx));
    m_cnw->cnw_adp.udp_.send_ate_msg(std::move(ims_tx));
    ims_ctx->ims_ctx.call_status_type = NULL_TYPE;

    return true;
  } // handle_call_release_resp

  bool pcs_ims::handle_call_setup_ack(uint16_t rnti, call_setup_ack_t &msg)
  {
    std::cout << "This is handle_call_setup_ack function!" << std::endl;

    ims_context *ims_ctx;
    srsran::unique_byte_buffer_t ims_tx; // 创建一个新的IMS上下文对象ims_ctx和一个独立的字节缓冲区ims_tx。
    if (find_ims_ctx_from_rnti(rnti) == NULL)
    {
      srsran::console(" no find_ims_ctx_from_rnti!\n");
      return false;
    }
    else
    {
      ims_ctx = find_ims_ctx_from_rnti(rnti);
    }

    std::cout << " This is Call Setup Ack  msg handle " << std::endl;

    if (msg.call_id.call_id_value != ims_ctx->ims_ctx.call_id)
    {
      printf("call id error cnw_ims_id:%d , ue_ims_id:%d \n", ims_ctx->ims_ctx.call_id, msg.call_id.call_id_value);
      return false;
    }

    printf("call code rate: %s\n", msg.call_code_rate_ims.call_code_rate.to_string());
    srsran::console("Uplink IMS: Call Setup Ack \n");

    ims_tx = srsran::make_byte_buffer();

    m_cnw->m_nas_mm->pcs_sm_modification(rnti, 3);

    // pack_call_comfirmed(ims_tx, ims_ctx);

    /* send message to RRC message queue. */
    // rrc_pcs_ims->s_write_ims_dl_info(rnti, std::move(ims_tx));
    // send_ims_dl_msg(rnti, std::move(ims_tx));

    return true;
  } // handle_call_setup_ack

  bool pcs_ims::handle_call_comfirmed(uint16_t rnti, call_confirmed_t &msg)
  {
    std::cout << "This is handle_call_comfirmed function!" << std::endl;

    ims_context *ims_ctx;
    srsran::unique_byte_buffer_t ims_tx; // 创建一个新的IMS上下文对象ims_ctx和一个独立的字节缓冲区ims_tx。
    if (find_ims_ctx_from_rnti(rnti) == NULL)
    {
      srsran::console(" no find_ims_ctx_from_rnti!\n");
      return false;
    }
    else
    {
      ims_ctx = find_ims_ctx_from_rnti(rnti);
    }

    ims_ctx->ims_ctx.rnti = rnti; // 将变量rnti的值赋给IMS上下文对象中的rnti字段，实现了将特定RNTI与当前处理的IMS上下文对象关联起来。

    printf("Generated unique Call ID (handle_ue_pcs_call_alerting): %x\n", msg.call_id.call_id_value);
    srsran::console("Uplink IMS: Call confirmed \n");

    /* send message to RRC message queue. */
    // rrc_pcs_ims->s_write_ims_dl_info(rnti, std::move(ims_tx));
    // send_ims_dl_msg(rnti, std::move(ims_tx));

    return true;
  } // handle_ue_pcs_call_alerting

  bool pcs_ims::handle_ue_pcs_call_alerting(uint16_t rnti, ue_pcs_call_alerting_t &msg)
  {
    std::cout << "This is handle_ue_pcs_call_alerting function!" << std::endl;

    ims_context *ims_ctx;
    srsran::unique_byte_buffer_t ims_tx; // 创建一个新的IMS上下文对象ims_ctx和一个独立的字节缓冲区ims_tx。
    if (find_ims_ctx_from_rnti(rnti) == NULL)
    {
      srsran::console(" no find_ims_ctx_from_rnti!\n");
      return false;
    }
    else
    {
      ims_ctx = find_ims_ctx_from_rnti(rnti);
    }

    ims_ctx->ims_ctx.rnti = rnti; // 将变量rnti的值赋给IMS上下文对象中的rnti字段，实现了将特定RNTI与当前处理的IMS上下文对象关联起来。

    std::cout << " This is ue_pcs_call_alerting  msg handle " << std::endl;

    printf("Generated unique Call ID (handle_ue_pcs_call_alerting): %x\n", msg.call_id.call_id_value);
    srsran::console("Uplink IMS: Call Alerting \n");

    /*send message to ate infrom ims register status   4.22 */
    srsran::unique_byte_buffer_t ims_ate_tx = srsran::make_byte_buffer();
    if (ims_ate_tx == nullptr)
    {
      srsran::console("Couldn't allocate PDU in TC_MSG_IMS_ATE_REGISTER \n");
      return false;
    }

    ims_ate_tx->N_bytes = 3;
    ims_ate_tx->msg[0] = 0xff;
    ims_ate_tx->msg[1] = 0x00;
    ims_ate_tx->msg[2] = TC_MSG_ATE_IMS_MT_CALL_RES;

    // send to ate msg queue
    srsran::console("send ims_msg to ate queue \n");
    // m_cnw->cnw_adp.udp_.send_ate_info.try_push(std::move(ims_ate_tx));
    m_cnw->cnw_adp.udp_.send_ate_msg(std::move(ims_ate_tx));

    /* send message to RRC message queue. */
    // rrc_pcs_ims->s_write_ims_dl_info(rnti, std::move(ims_tx));
    // send_ims_dl_msg(rnti, std::move(ims_tx));

    return true;
  } // handle_ue_pcs_call_alerting

  bool pcs_ims::handle_ue_pcs_call_connect(uint16_t rnti, ue_pcs_call_connect_t &msg)
  {
    std::cout << "This is handle_ue_pcs_call_connect function!" << std::endl;

    ims_context *ims_ctx = new ims_context();
    srsran::unique_byte_buffer_t ims_tx; // 创建一个新的IMS上下文对象ims_ctx和一个独立的字节缓冲区ims_tx。
    if (find_ims_ctx_from_rnti(rnti) == NULL)
    {
      srsran::console(" no find_ims_ctx_from_rnti!\n");
    }
    else
    {
      ims_ctx = find_ims_ctx_from_rnti(rnti);
    }

    ims_ctx->ims_ctx.rnti = rnti; // 将变量rnti的值赋给IMS上下文对象中的rnti字段，实现了将特定RNTI与当前处理的IMS上下文对象关联起来。

    std::cout << " This is ue_pcs_call_connect  msg handle " << std::endl;

    printf("Generated unique Call ID (handle_ue_pcs_call_alerting): %x\n", msg.call_id.call_id_value);
    printf("msg.call_type_ims: %s\n", msg.call_type_ims.call_type.to_string());
    srsran::console("Uplink IMS: Call Connect \n");

    // /*construct aka command message. */
    ims_tx = srsran::make_byte_buffer();
    if (ims_tx == nullptr)
    {
      srsran::console("Couldn't allocate PDU in pack_call_connect_ack \n");
      return false;
    }
    pack_call_connect_ack(ims_tx, ims_ctx);
    /* update ims msg_length. 更新消息的第二个字节 (索引为1) 为当前消息的总字节数，用于标识实际消息长度. */
    ims_tx->msg[1] = ims_tx->N_bytes;

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
    // rrc_pcs_ims->s_write_ims_dl_info(rnti, std::move(ims_tx));
    send_ims_dl_msg(rnti, std::move(ims_tx));

    // m_cnw->m_nas_mm->pcs_sm_modification(rnti, 1);

    ims_ctx->ims_ctx.call_status_type = MT_CALL_TYPE;
    /*send message to ate infrom ims mo status   4.22 */
    ims_tx = srsran::make_byte_buffer();
    if (ims_tx == nullptr)
    {
      srsran::console("Couldn't allocate PDU in TC_MSG_IMS_ATE_MT_CALL \n");
      return false;
    }
    ims_tx->N_bytes = 4;
    ims_tx->msg[0] = 0xff;
    ims_tx->msg[1] = 0x00;
    ims_tx->msg[2] = TC_MSG_IMS_ATE_MT_CALL;
    ims_tx->msg[3] = 0x01;

    // send to ate msg queue
    srsran::console("send ims_msg to ate queue \n");
    // m_cnw->cnw_adp.udp_.send_ate_info.try_push(std::move(ims_tx));
    m_cnw->cnw_adp.udp_.send_ate_msg(std::move(ims_tx));

    return true;
  } // handle_ue_pcs_call_connect

  // SMS
  bool pcs_ims::handle_mo_sms_req(uint16_t rnti, mo_sms_req_t &msg)
  {
    std::cout << "This is handle_mo_sms_req function!" << std::endl;

    ims_context *ims_ctx = find_ims_ctx_from_rnti(rnti);
    srsran::unique_byte_buffer_t ims_tx;
    if (ims_ctx == NULL)
    {
      pcs_ims_logger.error("Don't find_ims_ctx_from_rnti. rnti: %d", rnti);
      return false;
    }

    if (msg.call_id.call_id_value == 0xffffffff) // pcs receive ue sms msg
    {
      ims_ctx->ims_ctx.call_id_sms++;
      // ims_ctx->ims_ctx.call_id_sms=9;
      ims_ctx->ims_ctx.reference = msg.reference.reference;
      ims_ctx->ims_ctx.smc_party_bcd_num = msg.smc_party_bcd_num;
      ims_ctx->ims_ctx.message_content = msg.message_content; //

      printf("Generated unique Call ID (handle_mo_sms_req): %x\n", msg.call_id.call_id_value);
      printf("msg.reference: %x\n", msg.reference.reference);
      // printf("msg.s_tmsi_5g_value1: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[0]);
      // printf("msg.s_tmsi_5g_value2: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[1]);
      // printf("msg.s_tmsi_5g_value3: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[2]);
      // printf("msg.s_tmsi_5g_value4: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[3]);
      // printf("msg.s_tmsi_5g_value5: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[4]);
      // printf("msg.s_tmsi_5g_value6: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[5]);

      printf("msg.smc_party_bcd_num: ");
      for (uint8_t num : msg.smc_party_bcd_num.smc_party_bcd_num)
      {
        printf("%x ", num);
      }
      printf("\n");

      printf("msg.message_content: ");
      for (uint8_t value : msg.message_content.message_content)
      {
        printf("%x ", value);
      }
      printf("\n");

      if (!handle_message_content(msg.message_content.message_content, ims_ctx))
      {
        pcs_ims_logger.error("handle_message_content failed!");
        return false;
      }

      /*construct mo sms resp. */
      ims_tx = srsran::make_byte_buffer();
      if (ims_tx == nullptr)
      { // 检查内存分配是否成功
        srsran::console("Couldn't allocate PDU in mo sms req \n");
        return false;
      }
      pack_mo_sms_resp(ims_tx, ims_ctx);
      ims_tx->msg[1] = ims_tx->N_bytes;

      srsran::console("Downlink IMS: Sending mo_sms_resp\n");
      /*print msg ie*/
      int msg_len = ims_tx->N_bytes;
      for (int i = 0; i < msg_len; i++)
      {
        printf("ims_msg[%d]=%x", i, ims_tx->msg[i]);
        printf("\n");
      }
      /* send message to RRC message queue. */
      send_ims_dl_msg(rnti, std::move(ims_tx));

      srsran::console("Downlink IMS: Sending mt_sms_req\n");
      ims_tx = srsran::make_byte_buffer();
      pack_mt_sms_req(ims_tx, ims_ctx, 0);
      ims_tx->msg[1] = ims_tx->N_bytes;
      msg_len = ims_tx->N_bytes;
      for (int i = 0; i < msg_len; i++)
      {
        printf("ims_msg[%d]=%x", i, ims_tx->msg[i]);
        printf("\n");
      }

      send_ims_dl_msg(rnti, std::move(ims_tx));
    }
    else // sms called process
    {

      printf("msg.reference: %x\n", msg.reference.reference);
      ims_ctx->ims_ctx.reference = msg.reference.reference;
      ims_ctx->ims_ctx.call_id_sms = msg.call_id.call_id_value;
      srsran::console("Downlink IMS: Sending mo_sms_resp\n");
      ims_tx = srsran::make_byte_buffer();
      pack_mo_sms_resp(ims_tx, ims_ctx);
      ims_tx->msg[1] = ims_tx->N_bytes;
      send_ims_dl_msg(rnti, std::move(ims_tx));

      // check long sms vector
      if (!ims_ctx->ims_ctx.ate_mt_long_message_content.empty() && ims_ctx->ims_ctx.sms_type == LONG_SMS)
      {
        ims_tx = srsran::make_byte_buffer();
        if (ims_tx == NULL)
        {
          pcs_ims_logger.error("handle_mt_sms_resp make_byte_buffer failed!");
          return false;
        }
        pack_mt_sms_req(ims_tx, ims_ctx, 1);
        send_ims_dl_msg(rnti, std::move(ims_tx));
      }

      if (ims_ctx->ims_ctx.sms_type == LONG_SMS) // long sms
      {
        if (ims_ctx->ims_ctx.long_sms_sn == ims_ctx->ims_ctx.long_sms_max_num)
        { // 长短信发送完毕
          send_mt_sms_result_to_ate(true);
        }
      }
      else // short sms
      {
        send_mt_sms_result_to_ate(true);
      }
    }

    return true;
  } // handle_mo_sms_req

  bool pcs_ims::handle_mt_sms_resp(uint16_t rnti, mt_sms_resp_t &msg)
  {
    std::cout << "This is handle_mt_sms_resp function!" << std::endl;

    ims_context *ims_ctx = find_ims_ctx_from_rnti(rnti);
    srsran::unique_byte_buffer_t ims_tx; // 创建一个新的IMS上下文对象ims_ctx和一个独立的字节缓冲区ims_tx。
    if (ims_ctx == NULL)
    {
      pcs_ims_logger.error("Don't find_ims_ctx_from_rnti. rnti: %d", rnti);
      return false;
    }

    printf("Generated unique Call ID (handle_mt_sms_resp): %x\n", msg.call_id.call_id_value);
    printf("msg.s_tmsi_5g_value1: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[0]);
    printf("msg.s_tmsi_5g_value2: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[1]);
    printf("msg.s_tmsi_5g_value3: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[2]);
    printf("msg.s_tmsi_5g_value4: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[3]);
    printf("msg.s_tmsi_5g_value5: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[4]);
    printf("msg.s_tmsi_5g_value6: %x\n", msg.s_tmsi_5g.s_tmsi_5g_value[5]);
    printf("msg.cause_ims.cause: %s\n", msg.cause_ims.cause.to_string());

    srsran::console("Uplink IMS: mt sms resp \n");

    return true;
  } // handle_mt_sms_resp
  /************************end********************/

  // 用于打包鉴权命令消息并将其存储在给定的字节缓冲区中
  bool pcs_ims::pack_Authentication_Command(srsran::unique_byte_buffer_t &ims_buffer, ims_context *ims_ctx)
  {
    std::cout << "This is pack_Authentication_Command function!" << std::endl;
    pcs_ims_msg ims_msg;
    authentication_command_t &auth_comd = ims_msg.set_authentication_command();

    ims_msg.hdr.protocol_version = srsran::ims::pcs_ims_hdr::protocol_version_options::initial_version;
    ims_msg.hdr.ims_message_type = srsran::ims::ims_msg_types::options::authentication_command;
    ims_msg.hdr.message_length = 0;

    auth_comd.s_tmsi_5g = ims_ctx->ims_ctx.s_tmsi_5g;
    printf("auth_comd.s_tmsi_5g:: %d  ims_ctx->ims_ctx.s_tmsi_5g:: %d\n", auth_comd.s_tmsi_5g.s_tmsi_5g_value[2], ims_ctx->ims_ctx.s_tmsi_5g.s_tmsi_5g_value[2]);

    uint8_t test_nonce_contents[] = {0x01, 0x49, 0x38, 0x54, 0x43, 0x46, 0x31, 0x77, 0x32, 0x57, 0x73, 0x51, 0x78, 0x57, 0x30, 0x42, 0x62, 0x31,
                                     0x32, 0x52, 0x71, 0x51, 0x31, 0x53, 0x6a, 0x59, 0x33, 0x47, 0x70, 0x7a, 0x6f, 0x41, 0x41, 0x4c, 0x50, 0x70, 0x4e, 0x50, 0x2b, 0x63, 0x4a, 0x51, 0x52, 0x30, 0x3d};
    int len_test = sizeof(test_nonce_contents);
    auth_comd.nonce.nonce_content.security_algorithm = test_nonce_contents[0];
    for (int i = 1; i < len_test; ++i)
    {
      auth_comd.nonce.nonce_content.base64.push_back(test_nonce_contents[i]);
    }

    SRSASN_CODE err = ims_msg.pack(ims_buffer);
    if (err != SRSASN_SUCCESS)
    {
      // m_nas_mm_logger.error("Error packing Authentication Request");
      srsran::console("Error packing Authentication Command\n");
      return false;
    }

    return true;
  } // pack_Authentication_Command

  bool pcs_ims::pack_Voice_Register_Rsp(srsran::unique_byte_buffer_t &ims_buffer, ims_context *ims_ctx)
  {
    std::cout << "This is pack_Voice_Register_Rsp function!" << std::endl;
    pcs_ims_msg ims_msg;
    voice_register_resp_t &reg_resp = ims_msg.set_voice_register_resp();

    ims_msg.hdr.protocol_version = srsran::ims::pcs_ims_hdr::protocol_version_options::initial_version;
    ims_msg.hdr.ims_message_type = srsran::ims::ims_msg_types::options::voice_register_resp;
    ims_msg.hdr.message_length = 0;

    reg_resp.s_tmsi_5g = ims_ctx->ims_ctx.s_tmsi_5g;
    switch (ims_ctx->ims_ctx.cause)
    {
    case srsran::ims::cause_t::cause_type_::options::success:
      reg_resp.cause_ims.cause = srsran::ims::cause_t::cause_type_::options::success;
      reg_resp.expire.expire_value = 0x08;
      break;
    case srsran::ims::cause_t::cause_type_::options::auth_failure:
      reg_resp.cause_ims.cause = srsran::ims::cause_t::cause_type_::options::auth_failure;
      break;
    default:

      break;
    }

    SRSASN_CODE err = ims_msg.pack(ims_buffer);
    if (err != SRSASN_SUCCESS)
    {
      srsran::console("Error packing Voice_Register_Rsp\n");
      return false;
    }

    return true;
  } // pack_Voice_Register_Rsp

  /*********************2024-3-11******************/
  bool pcs_ims::pack_Voice_DeRegister_Rsp(srsran::unique_byte_buffer_t &ims_buffer, ims_context *ims_ctx)
  {
    std::cout << "This is pack_Voice_DeRegister_Rsp function!" << std::endl;
    pcs_ims_msg ims_msg;
    voice_deregister_resp_t &dereg_resp = ims_msg.set_voice_deregister_resp();

    ims_msg.hdr.protocol_version = srsran::ims::pcs_ims_hdr::protocol_version_options::initial_version;
    ims_msg.hdr.ims_message_type = srsran::ims::ims_msg_types::options::voice_deregister_resp;
    ims_msg.hdr.message_length = 0;

    dereg_resp.s_tmsi_5g = ims_ctx->ims_ctx.s_tmsi_5g;

    SRSASN_CODE err = ims_msg.pack(ims_buffer);
    if (err != SRSASN_SUCCESS)
    {
      // m_nas_mm_logger.error("Error packing Authentication Request");
      srsran::console("Error packing Voice_DeRegister_Rsp\n");
      return false;
    }

    return true;
  } // pack_Voice_DeRegister_Rsp

  bool pcs_ims::pack_call_setup_ack(srsran::unique_byte_buffer_t &ims_buffer, ims_context *ims_ctx)
  {
    std::cout << "This is pack_Call_Setup_Ack function!" << std::endl;
    pcs_ims_msg ims_msg;
    call_setup_ack_t &call_setup_ack = ims_msg.set_call_setup_ack();

    ims_msg.hdr.protocol_version = srsran::ims::pcs_ims_hdr::protocol_version_options::initial_version;
    ims_msg.hdr.ims_message_type = srsran::ims::ims_msg_types::options::call_setup_ack;
    ims_msg.hdr.message_length = 0;
    /* call_id 由pcs进行分配*/
    // ims_ctx->ims_ctx.call_id = generateUniqueID();
    ims_ctx->ims_ctx.call_id = gen_rand();
    printf("Generated unique Call ID (call setup ack): %x\n", ims_ctx->ims_ctx.call_id);
    call_setup_ack.call_id.call_id_value = ims_ctx->ims_ctx.call_id;
    std::cout << "ims_ctx->ims_ctx.call_code_rate = " << (int)ims_ctx->ims_ctx.call_code_rate << std::endl;

    if (m_cnw->cnw_adp.udp_.ue_category == 14)
    {
      ims_ctx->ims_ctx.call_code_rate = 0x03;
    }

    if (ims_ctx->ims_ctx.call_code_rate == 0x01)
    {
      call_setup_ack.call_code_rate_ims.call_code_rate = srsran::ims::call_code_rate_t::call_code_rate_type_::options::_2400bps;
      m_cnw->cnw_adp.udp_.is_Control = true;
      m_cnw->cnw_adp.udp_.voice_indicate = 0x01;
    }
    else if (ims_ctx->ims_ctx.call_code_rate == 0x02)
    {
      call_setup_ack.call_code_rate_ims.call_code_rate = srsran::ims::call_code_rate_t::call_code_rate_type_::options::_4800bps;
      m_cnw->cnw_adp.udp_.is_Control = true;
      m_cnw->cnw_adp.udp_.voice_indicate = 0x02;
    }
    else if (ims_ctx->ims_ctx.call_code_rate == 0x03)
    {
      call_setup_ack.call_code_rate_ims.call_code_rate = srsran::ims::call_code_rate_t::call_code_rate_type_::options::_800bps;
      m_cnw->cnw_adp.udp_.is_Control = true;
      m_cnw->cnw_adp.udp_.voice_indicate = 0x03;
    }

    SRSASN_CODE err = ims_msg.pack(ims_buffer);
    if (err != SRSASN_SUCCESS)
    {
      // m_nas_mm_logger.error("Error packing Authentication Request");
      srsran::console("Error packing Call_Setup_Ack\n");
      return false;
    }

    return true;
  } // pack_call_setup_ack

  bool pcs_ims::pack_call_comfirmed(srsran::unique_byte_buffer_t &ims_buffer, ims_context *ims_ctx)
  {
    std::cout << "This is pack_call_comfirmed function!" << std::endl;
    pcs_ims_msg ims_msg;
    call_confirmed_t &call_confirmed = ims_msg.set_call_confirmed();

    ims_msg.hdr.protocol_version = srsran::ims::pcs_ims_hdr::protocol_version_options::initial_version;
    ims_msg.hdr.ims_message_type = srsran::ims::ims_msg_types::options::call_confirmed;
    ims_msg.hdr.message_length = 0;

    /* call_id 由pcs进行分配*/

    call_confirmed.call_id.call_id_value = ims_ctx->ims_ctx.call_id;
    SRSASN_CODE err = ims_msg.pack(ims_buffer);
    if (err != SRSASN_SUCCESS)
    {
      // m_nas_mm_logger.error("Error packing Authentication Request");
      srsran::console("Error packing Call_Setup_Ack\n");
      return false;
    }

    return true;
  }

  bool pcs_ims::pack_pcs_ue_call_alerting(srsran::unique_byte_buffer_t &ims_buffer, ims_context *ims_ctx)
  {
    std::cout << "This is pack_pcs_ue_call_alerting function!" << std::endl;
    pcs_ims_msg ims_msg;
    pcs_ue_call_alerting_t &pcs_ue_call_alerting = ims_msg.set_pcs_ue_call_alerting();

    ims_msg.hdr.protocol_version = srsran::ims::pcs_ims_hdr::protocol_version_options::initial_version;
    ims_msg.hdr.ims_message_type = srsran::ims::ims_msg_types::options::call_alerting;
    ims_msg.hdr.message_length = 0;
    /* call_id 由pcs进行分配*/
    // uint32_t call_id = generateUniqueID();
    pcs_ue_call_alerting.call_id.call_id_value = ims_ctx->ims_ctx.call_id;
    printf("Generated unique Call ID(Call Alerting): %x\n", pcs_ue_call_alerting.call_id.call_id_value);
    // pcs_ue_call_alerting.call_id.call_id_value = call_id;

    SRSASN_CODE err = ims_msg.pack(ims_buffer);
    if (err != SRSASN_SUCCESS)
    {
      // m_nas_mm_logger.error("Error packing Authentication Request");
      srsran::console("Error packing Call_Alerting\n");
      return false;
    }

    return true;
  } // pack_pcs_ue_call_alerting

  bool pcs_ims::pack_pcs_ue_call_connect(srsran::unique_byte_buffer_t &ims_buffer, ims_context *ims_ctx)
  {
    std::cout << "This is pack_pcs_ue_call_alerting function!" << std::endl;
    pcs_ims_msg ims_msg;
    pcs_ue_call_connect_t &pcs_ue_call_connect = ims_msg.set_pcs_ue_call_connect();

    ims_msg.hdr.protocol_version = srsran::ims::pcs_ims_hdr::protocol_version_options::initial_version;
    ims_msg.hdr.ims_message_type = srsran::ims::ims_msg_types::options::call_connect;
    ims_msg.hdr.message_length = 0;
    /* call_id 由pcs进行分配*/
    pcs_ue_call_connect.call_id.call_id_value = ims_ctx->ims_ctx.call_id;
    printf("Generated unique Call ID(Call Connect): %x\n", pcs_ue_call_connect.call_id.call_id_value);
    // pcs_ue_call_connect.call_id.call_id_value = call_id;
    pcs_ue_call_connect.call_type_ims.call_type = srsran::ims::call_type_t::call_type_type_::options::voice_call;

    SRSASN_CODE err = ims_msg.pack(ims_buffer);
    if (err != SRSASN_SUCCESS)
    {
      // m_nas_mm_logger.error("Error packing Authentication Request");
      srsran::console("Error packing Call_Connect\n");
      return false;
    }

    return true;
  } // pack_pcs_ue_call_connect

  bool pcs_ims::pack_call_release_req(srsran::unique_byte_buffer_t &ims_buffer, ims_context *ims_ctx)
  {
    std::cout << "This is pack_call_release_req function!" << std::endl;
    pcs_ims_msg ims_msg;
    call_release_req_t &call_release_req = ims_msg.set_call_release_req();

    ims_msg.hdr.protocol_version = srsran::ims::pcs_ims_hdr::protocol_version_options::initial_version;
    ims_msg.hdr.ims_message_type = srsran::ims::ims_msg_types::options::call_release_req;
    ims_msg.hdr.message_length = 0;
    /* call_id 由pcs进行分配*/
    call_release_req.call_id.call_id_value = ims_ctx->ims_ctx.call_id;
    printf("Generated unique Call ID(Call Connect): %x\n", call_release_req.call_id.call_id_value);
    // pcs_ue_call_connect.call_id.call_id_value = call_id;
    call_release_req.cause_ims.cause = srsran::ims::cause_t::cause_type_::ue_call_released_normally;

    SRSASN_CODE err = ims_msg.pack(ims_buffer);
    if (err != SRSASN_SUCCESS)
    {
      // m_nas_mm_logger.error("Error packing Authentication Request");
      srsran::console("Error packing pack_call_release_req\n");
      return false;
    }

    return true;
  } // pack_call_release_req

  bool pcs_ims::pack_pcs_ue_call_setup(srsran::unique_byte_buffer_t &ims_buffer, ims_context *ims_ctx)
  {
    std::cout << "This is pack_pcs_ue_call_setup function!" << std::endl;
    pcs_ims_msg ims_msg;
    pcs_ue_call_setup_t &pcs_ue_call_setup = ims_msg.set_pcs_ue_call_setup();

    ims_msg.hdr.protocol_version = srsran::ims::pcs_ims_hdr::protocol_version_options::initial_version;
    ims_msg.hdr.ims_message_type = srsran::ims::ims_msg_types::options::call_setup;
    ims_msg.hdr.message_length = 0;
    /* call_id 由pcs进行分配*/
    uint32_t call_id = gen_rand();
    ims_ctx->ims_ctx.call_id = call_id;
    printf("Generated unique Call ID(pcs_ue_call_setup): %x\n", call_id);
    pcs_ue_call_setup.call_id.call_id_value = call_id;
    pcs_ue_call_setup.call_type_ims.call_type = srsran::ims::call_type_t::call_type_type_::options::voice_call;
    std::cout << "ims_ctx->ims_ctx.call_code_rate = " << (int)ims_ctx->ims_ctx.call_code_rate << std::endl;

    // pcs_ue_call_setup.calling_party_bcd_num.calling_party_bcd_num = {0X68, 0X61, 0X91, 0X44, 0X14, 0X30, 0Xf9};
    pcs_ue_call_setup.calling_party_bcd_num.calling_party_bcd_num = {0X68, 0X51, 0X92, 0X44, 0X14, 0X00, 0Xf8};
    pcs_ue_call_setup.ttot_call_indicator_ims.ttot_call_indicator = srsran::ims::ttot_call_indicator_t::ttot_call_indicator_type_::options::ttot_call_indicator;
    if (ims_ctx->ims_ctx.call_code_rate == 0x01)
    {
      pcs_ue_call_setup.call_code_rate_ims.call_code_rate = srsran::ims::call_code_rate_t::call_code_rate_type_::options::_2400bps;
      m_cnw->cnw_adp.udp_.is_Control = true;
      m_cnw->cnw_adp.udp_.voice_indicate = 0x01;
    }
    else if (ims_ctx->ims_ctx.call_code_rate == 0x02)
    {
      pcs_ue_call_setup.call_code_rate_ims.call_code_rate = srsran::ims::call_code_rate_t::call_code_rate_type_::options::_4800bps;
      m_cnw->cnw_adp.udp_.is_Control = true;
      m_cnw->cnw_adp.udp_.voice_indicate = 0x02;
    }
    else if (ims_ctx->ims_ctx.call_code_rate == 0x03)
    {
      pcs_ue_call_setup.call_code_rate_ims.call_code_rate = srsran::ims::call_code_rate_t::call_code_rate_type_::options::_800bps;
      m_cnw->cnw_adp.udp_.is_Control = true;
      m_cnw->cnw_adp.udp_.voice_indicate = 0x03;
    }

    SRSASN_CODE err = ims_msg.pack(ims_buffer);
    if (err != SRSASN_SUCCESS)
    {
      // m_nas_mm_logger.error("Error packing Authentication Request");
      srsran::console("Error packing PCS_UE_Call_Setup\n");
      return false;
    }

    return true;
  } // pack_pcs_ue_call_setup

  bool pcs_ims::pack_call_connect_ack(srsran::unique_byte_buffer_t &ims_buffer, ims_context *ims_ctx)
  {
    std::cout << "This is pack_call_connect_ack function!" << std::endl;
    pcs_ims_msg ims_msg;
    call_connect_ack_t &call_connect_ack = ims_msg.set_call_connect_ack();

    ims_msg.hdr.protocol_version = srsran::ims::pcs_ims_hdr::protocol_version_options::initial_version;
    ims_msg.hdr.ims_message_type = srsran::ims::ims_msg_types::options::call_connect_ack;
    ims_msg.hdr.message_length = 0;
    /* call_id 由pcs进行分配*/
    call_connect_ack.call_id.call_id_value = ims_ctx->ims_ctx.call_id;
    printf("Generated unique Call ID(pcs_ue_call_setup): %x\n", ims_ctx->ims_ctx.call_id);

    SRSASN_CODE err = ims_msg.pack(ims_buffer);
    if (err != SRSASN_SUCCESS)
    {
      // m_nas_mm_logger.error("Error packing Authentication Request");
      srsran::console("Error packing PCS_UE_Call_Setup\n");
      return false;
    }

    return true;
  } // pack_call_connect_ack

  // 短消息
  bool pcs_ims::pack_mo_sms_resp(srsran::unique_byte_buffer_t &ims_buffer, ims_context *ims_ctx)
  {
    std::cout << "This is pack_mo_sms_resp function!" << std::endl;
    pcs_ims_msg ims_msg;
    mo_sms_resp_t &mo_sms_resp = ims_msg.set_mo_sms_resp();

    ims_msg.hdr.protocol_version = srsran::ims::pcs_ims_hdr::protocol_version_options::initial_version;
    ims_msg.hdr.ims_message_type = srsran::ims::ims_msg_types::options::mo_sms_resp;
    ims_msg.hdr.message_length = 0;

    mo_sms_resp.call_id.call_id_value = ims_ctx->ims_ctx.call_id_sms;
    printf("Generated unique Call ID(mo_sms_resp): %x\n", mo_sms_resp.call_id.call_id_value);
    mo_sms_resp.reference.reference = ims_ctx->ims_ctx.reference;
    mo_sms_resp.cause_ims.cause = srsran::ims::cause_t::cause_type_::success;

    SRSASN_CODE err = ims_msg.pack(ims_buffer);
    if (err != SRSASN_SUCCESS)
    {
      // m_nas_mm_logger.error("Error packing Authentication Request");
      srsran::console("Error packing mo_sms_resp\n");
      return false;
    }

    return true;
  } // pack_mo_sms_resp

  bool pcs_ims::pack_mt_sms_req(srsran::unique_byte_buffer_t &ims_buffer, ims_context *ims_ctx, bool flag)
  {
    std::cout << "This is pack_mt_sms_req!" << std::endl;
    pcs_ims_msg ims_msg;
    mt_sms_req_t &mt_sms_req = ims_msg.set_mt_sms_req();

    ims_msg.hdr.protocol_version = srsran::ims::pcs_ims_hdr::protocol_version_options::initial_version;
    ims_msg.hdr.ims_message_type = srsran::ims::ims_msg_types::options::mt_sms_req;
    ims_msg.hdr.message_length = 0;
    
    mt_sms_req.smc_party_bcd_num = ims_ctx->ims_ctx.smc_party_bcd_num;
    printf("mt_sms_req.smc_party_bcd_num.smc_party_bcd_num: ");
    for (uint8_t num : mt_sms_req.smc_party_bcd_num.smc_party_bcd_num)
    {
      printf("%u ", num);
    }
    printf("\n");

    int length;

    if (flag == 0)
    {
      // mt ack
      mt_sms_req.call_id.call_id_value = ims_ctx->ims_ctx.call_id_sms;
      mt_sms_req.message_content.message_content.push_back(0x03);
      mt_sms_req.message_content.message_content.push_back(ims_ctx->ims_ctx.message_content.message_content[1]); // sn,xuliehao
      mt_sms_req.message_content.message_content.push_back(0x41);
      mt_sms_req.message_content.message_content.push_back(0x00);
    }
    else
    {
      // mt sms
      mt_sms_req.call_id.call_id_value = 0x08;
      /*TC MT SMS message*/
      if ((!ims_ctx->ims_ctx.ate_mt_sms_content.empty() || !ims_ctx->ims_ctx.ate_mt_long_message_content.empty()) && tc_sms_flag == ENABLE_SMS_TC_MODE)
      {
        pack_ate_mt_sms(mt_sms_req.message_content.message_content, ims_ctx);
      }
      else
      {
        // pack default mt sms
        uint8_t default_message_content1[] = {0x01, 0x02, 0x08, 0x91, 0x68, 0x31, 0x08, 0x10, 0x83, 0x00, 0xf0, 0x00,
                                              0x25, 0x24, 0x0b, 0x81, 0x51, 0x92, 0x78, 0x00, 0x78, 0xf6, 0x00, 0x08,
                                              0x32, 0x80, 0x90, 0x01, 0x90, 0x52, 0x23, 0x12, 0x8b, 0xf7, 0x4e, 0x0e,
                                              0x62, 0x11, 0x80, 0x54, 0x7c, 0xfb, 0xff, 0x0c, 0x8c, 0x22, 0x8c, 0x22,
                                              0x30, 0x02};
        length = sizeof(default_message_content1);
        for (int i = 0; i < length; i++)
        {
          mt_sms_req.message_content.message_content.push_back(default_message_content1[i]);
        }
      }
    }

    printf("mt_sms_req.message_content.message_content: ");
    for (uint8_t value : mt_sms_req.message_content.message_content)
    {
      printf("%u ", value);
    }
    printf("\n");

    srsran::console("Downlink IMS: mt sms req \n");

    SRSASN_CODE err = ims_msg.pack(ims_buffer);
    if (err != SRSASN_SUCCESS)
    {
      srsran::console("Error packing mt_sms_req\n");
      return false;
    }

    return true;

  } // pack_mt_sms_req

  bool pcs_ims::send_ims_dl_msg(uint16_t rnti, srsran::unique_byte_buffer_t pdu)
  {
    srsran::unique_byte_buffer_t enb_pdu = srsran::make_byte_buffer();
    enb_msg_header_t s1_header;
    s1_header.enb_id = ims_adp->udp_.enb_id;
    s1_header.rnti = 70;
    s1_header.msg_type = ims_dl;

    int len = sizeof(s1_header);

    memcpy(enb_pdu->msg, &s1_header, len);
    memcpy(enb_pdu->msg + len, pdu->msg, pdu->N_bytes);

    enb_pdu->N_bytes = pdu->N_bytes + len;

    ims_adp->udp_.send_enb_msg(std::move(enb_pdu));

    return true;
  }

  bool pcs_ims::handle_message_content(vector<uint8_t> &message_content, ims_context *ims_ctx)
  {
    srsran::console("this is handle_message_content handler!\n");
    rp_data_msg_to_network_t rp_data;
    if (rp_data.unpack(message_content) != SRSRAN_SUCCESS)
    {
      pcs_ims_logger.error("sms message content deconde failed!");
      srsran::console("sms message content deconde failed!\n");
      return false;
    }

    srsran::unique_byte_buffer_t ims_ate_tx = srsran::make_byte_buffer();
    if (ims_ate_tx == nullptr)
    {
      srsran::console("Couldn't allocate PDU in TC_MSG_IMS_ATE_REGISTER \n");
      return false;
    }

    if (rp_data.tp_user_data_header_indicator == 0x01) // long sms
    {
      srsran::console("this is long sms handler!\n");
      tp_ud_t tp_ud;
      if (rp_data.tp_user_data_len < 6)
      {
        pcs_ims_logger.error("Sms is long sms, but message content size<6!");
        srsran::console("Sms is long sms, but message content size<6!\n");
        return false;
      }
      tp_ud.header_length = rp_data.tp_user_data[0];
      tp_ud.identifier = rp_data.tp_user_data[1];
      tp_ud.element_length = rp_data.tp_user_data[2];
      tp_ud.reference_num = rp_data.tp_user_data[3];
      tp_ud.max_num = rp_data.tp_user_data[4];
      tp_ud.sequence_num = rp_data.tp_user_data[5];

      for (int i = 6; i < rp_data.tp_user_data_len; i++)
      {
        ims_ctx->ims_ctx.tmp_long_message_content[tp_ud.reference_num].push_back(rp_data.tp_user_data[i]);
      }
      srsran::console("tp_ud.max_num:%d, tp_ud.sequence_num%d \n", tp_ud.max_num, tp_ud.sequence_num);
      // check: this message is the last paket?
      if (tp_ud.max_num == tp_ud.sequence_num)
      {
        uint16_t long_message_length = ims_ctx->ims_ctx.tmp_long_message_content[tp_ud.reference_num].size();
        ims_ate_tx->msg[0] = 0xff;
        ims_ate_tx->msg[1] = 0x00;
        ims_ate_tx->msg[2] = 0x06;
        ims_ate_tx->msg[3] = rp_data.tp_data_coding_form;
        ims_ate_tx->msg[4] = (long_message_length >> 8) & 0xff;
        ims_ate_tx->msg[5] = long_message_length & 0xff;

        for (int i = 0; i < (int)long_message_length; i++)
        {
          ims_ate_tx->msg[6 + i] = ims_ctx->ims_ctx.tmp_long_message_content[tp_ud.reference_num][i];
        }

        ims_ate_tx->N_bytes = long_message_length + 6;
        printf("long ims_ate_tx: ");
        for (int i = 0; i < (int)ims_ate_tx->N_bytes; i++)
        {
          printf(" %x", ims_ate_tx->msg[i]);
        }
        printf("\n");

        srsran::console("send long_sms_msg to ate queue \n");
        m_cnw->cnw_adp.udp_.send_ate_msg(std::move(ims_ate_tx));

        // delete the long sms content
        ims_ctx->ims_ctx.tmp_long_message_content.erase(tp_ud.reference_num);
      }
    }
    else // short sms
    {
      srsran::console("this is short sms handler!\n");
      ims_ate_tx->msg[0] = 0xff;
      ims_ate_tx->msg[1] = 0x00;
      ims_ate_tx->msg[2] = 0x06;
      ims_ate_tx->msg[3] = rp_data.tp_data_coding_form;
      ims_ate_tx->msg[4] = 0x00;
      ims_ate_tx->msg[5] = rp_data.tp_user_data_len & 0xff;

      printf("rp_data.tp_user_data.size(): %d\n", (int)rp_data.tp_user_data.size());
      printf("tp_user_data: ");
      for (int i = 0; i < rp_data.tp_user_data_len; i++)
      {
        ims_ate_tx->msg[6 + i] = rp_data.tp_user_data[i];
        printf(" %x", rp_data.tp_user_data[i]);
      }
      printf("\n");

      ims_ate_tx->N_bytes = rp_data.tp_user_data_len + 6;
      printf("short ims_ate_tx: ");
      for (int i = 0; i < (int)ims_ate_tx->N_bytes; i++)
      {
        printf(" %x", ims_ate_tx->msg[i]);
      }
      printf("\n");

      // send to ate msg queue
      srsran::console("send short_sms_msg to ate queue \n");
      m_cnw->cnw_adp.udp_.send_ate_msg(std::move(ims_ate_tx));
    }

    return true;
  }

  bool pcs_ims::pack_ate_mt_sms(vector<uint8_t> &message_content, ims_context *ims_ctx)
  {
    // pack ate mt sms
    uint8_t rp_msg_type = 0x01;
    message_content.push_back(rp_msg_type);

    uint8_t rp_msg_reference = 0x02;
    message_content.push_back(rp_msg_reference);

    uint8_t rp_originator_address[] = {0x08, 0x91, 0x68, 0x31, 0x08, 0x10, 0x83, 0x00, 0xf0};
    int length = sizeof(rp_originator_address);
    for (int i = 0; i < length; i++)
    {
      message_content.push_back(rp_originator_address[i]);
    }

    uint8_t rp_destination_address = 0x00;
    message_content.push_back(rp_destination_address);

    // rp data
    vector<uint8_t> rp_user_data;
    rp_user_data.push_back(0x00); // length

    if (ims_ctx->ims_ctx.sms_type == SHORT_SMS)
    {
      rp_user_data.push_back(0x24);
    }
    else
    {
      if (ims_ctx->ims_ctx.long_sms_sn + 1 == ims_ctx->ims_ctx.long_sms_max_num)
      {
        rp_user_data.push_back(0x64);
      }
      else
      {
        rp_user_data.push_back(0x60);
      }
    }

    uint8_t tp_originator_address[] = {0x0b, 0x81, 0x51, 0x92, 0x78, 0x00, 0x78, 0xf6};
    length = sizeof(tp_originator_address);
    for (int i = 0; i < length; i++)
    {
      rp_user_data.push_back(tp_originator_address[i]);
    }

    uint8_t tp_protocol_identifier = 0x00;
    rp_user_data.push_back(tp_protocol_identifier);

    uint8_t tp_data_code_form = ims_ctx->ims_ctx.sms_code_form;
    rp_user_data.push_back(tp_data_code_form);

    // pack default_time  (service centre time)
    uint8_t default_time[] = {0x32, 0x80, 0x90, 0x01, 0x90, 0x52, 0x23};
    length = sizeof(default_time);
    for (int i = 0; i < length; i++)
    {
      rp_user_data.push_back(default_time[i]);
    }

    // ate msg content
    vector<uint8_t> ate_mt_sms_content;
    uint8_t sms_content_len = 0;
    if (!ims_ctx->ims_ctx.ate_mt_sms_content.empty() && ims_ctx->ims_ctx.sms_type == SHORT_SMS) // short sms
    {
      srsran::console("pack the short sms content.\n");
      length = ims_ctx->ims_ctx.ate_mt_sms_content.size();
      if (ims_ctx->ims_ctx.sms_code_form == 0x00)
      {
        // sms_content_len = length*8%7==0? length*8/7 : length*8/7+1;
        sms_content_len = length * 8 / 7;
      }
      else
      {
        sms_content_len = length;
      }
      ate_mt_sms_content.push_back(sms_content_len);
      for (int i = 0; i < length; i++)
      {
        ate_mt_sms_content.push_back(ims_ctx->ims_ctx.ate_mt_sms_content[i]);
      }
      ims_ctx->ims_ctx.ate_mt_sms_content.clear();
    }
    else if (!ims_ctx->ims_ctx.ate_mt_long_message_content.empty() && ims_ctx->ims_ctx.sms_type == LONG_SMS) // long sms
    {
      srsran::console("pack the long sms content.\n");
      if (ims_ctx->ims_ctx.long_sms_max_num == 0x00) // 发送第一包
      {
        length = ims_ctx->ims_ctx.ate_mt_long_message_content.size();
        printf("ate_mt_long_message_content size:%d", length);
        if (length < 134)
        {
          pcs_ims_logger.error("pack_ate_mt_sms: ate_mt_long_message_content size < 134");
          return false;
        }
        if (ims_ctx->ims_ctx.sms_code_form == 0x00)
        { // 英文编码，字符数
          sms_content_len = 0xa0;
        }
        else
        { // 字节数
          sms_content_len = 140;
        }
        ate_mt_sms_content.push_back(sms_content_len);
        ims_ctx->ims_ctx.long_sms_max_num = length / 134 + 1;
        ims_ctx->ims_ctx.long_sms_sn = 0x01;
        pack_long_sms_header(ate_mt_sms_content, ims_ctx);
        for (int i = 0; i < 134; i++)
        {
          ate_mt_sms_content.push_back(ims_ctx->ims_ctx.ate_mt_long_message_content[i]);
        }
        auto it = ims_ctx->ims_ctx.ate_mt_long_message_content.begin();
        ims_ctx->ims_ctx.ate_mt_long_message_content.erase(it, it + 134);
      }
      else
      { // 发送后面的包数
        length = ims_ctx->ims_ctx.ate_mt_long_message_content.size();
        ims_ctx->ims_ctx.long_sms_sn++;
        if (length >= 134 && ims_ctx->ims_ctx.long_sms_sn < ims_ctx->ims_ctx.long_sms_max_num)
        {
          if (ims_ctx->ims_ctx.sms_code_form == 0x00)
          { // 英文编码，字符数
            sms_content_len = 0xa0;
          }
          else
          { // 字节数
            sms_content_len = 140;
          }
          ate_mt_sms_content.push_back(sms_content_len);
          pack_long_sms_header(ate_mt_sms_content, ims_ctx);
          for (int i = 0; i < 134; i++)
          {
            ate_mt_sms_content.push_back(ims_ctx->ims_ctx.ate_mt_long_message_content[i]);
          }
          auto it = ims_ctx->ims_ctx.ate_mt_long_message_content.begin();
          ims_ctx->ims_ctx.ate_mt_long_message_content.erase(it, it + 134);
        }
        else if (length < 134 && ims_ctx->ims_ctx.long_sms_sn == ims_ctx->ims_ctx.long_sms_max_num)
        {
          if (ims_ctx->ims_ctx.sms_code_form == 0x00)
          {
            sms_content_len = 8 + (length - 1) * 8 / 7;
          }
          else
          {
            sms_content_len = length + 6;
          }
          ate_mt_sms_content.push_back(sms_content_len);
          pack_long_sms_header(ate_mt_sms_content, ims_ctx);
          for (int i = 0; i < length; i++)
          {
            ate_mt_sms_content.push_back(ims_ctx->ims_ctx.ate_mt_long_message_content[i]);
          }
          // last paket clear
          ims_ctx->ims_ctx.ate_mt_long_message_content.clear();
          ims_ctx->ims_ctx.long_sms_max_num = 0x00;
          ims_ctx->ims_ctx.long_sms_sn = 0x00;
        }
        else
        {
          pcs_ims_logger.error("pack_ate_mt_sms error!");
          return false;
        }
      }
    }

    // pack rp_user_data
    length = ate_mt_sms_content.size();
    for (int i = 0; i < length; i++)
    {
      rp_user_data.push_back(ate_mt_sms_content[i]);
    }

    rp_user_data[0] = rp_user_data.size() - 1;

    length = rp_user_data.size();
    for (int i = 0; i < length; i++)
    {
      message_content.push_back(rp_user_data[i]);
    }

    return true;
  }

  bool pcs_ims::pack_long_sms_header(vector<uint8_t> &long_sms_header, ims_context *ims_ctx)
  {
    long_sms_header.push_back(0x05);
    long_sms_header.push_back(0x00);
    long_sms_header.push_back(0x03);
    long_sms_header.push_back(ims_ctx->ims_ctx.long_sms_reference_num);
    long_sms_header.push_back(ims_ctx->ims_ctx.long_sms_max_num);
    long_sms_header.push_back(ims_ctx->ims_ctx.long_sms_sn);
    return true;
  }

  bool pcs_ims::send_mt_sms_result_to_ate(bool result)
  {
    // send ste mt sms res
    srsran::unique_byte_buffer_t ims_ate_tx = srsran::make_byte_buffer();
    if (ims_ate_tx == NULL)
    {
      pcs_ims_logger.error("handle_mt_sms_resp make_byte_buffer failed!");
      return false;
    }
    ims_ate_tx->N_bytes = 4;
    ims_ate_tx->msg[0] = 0xff;
    ims_ate_tx->msg[1] = 0x00;
    ims_ate_tx->msg[2] = TC_MSG_ATE_IMS_MT_SMS_RES;
    ims_ate_tx->msg[3] = (result == true ? 0x00 : 0x01); // sucessful

    // send to ate msg queue
    srsran::console("send TC_MSG_ATE_IMS_MT_SMS_RES to ate queue \n");
    if (!m_cnw->cnw_adp.udp_.send_ate_msg(std::move(ims_ate_tx)))
    {
      return false;
    }

    return true;
  }
  /************************end********************/

} // namespace srsepc
