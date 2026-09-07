/******************************************************************************
 * File:        cnw.h
 * Description: Top-level 5GC class. Creates and links all
 *              interfaces and helpers.
 *****************************************************************************/

#ifndef SRSRAN_CNW_H
#define SRSRAN_CNW_H

#include "lib/include/srsran/interfaces/enb_rrc_interfaces.h" //-------2023/11/14
#include "nas_context.h"
#include "nas_mm.h"
#include "nas_sm.h"
#include "pcs_ims.h"
#include "xw_icmp.h"
#include "srsran/adt/circular_buffer.h"
#include "srsran/asn1/nas_5g_msg.h"
#include "srsran/common/buffer_pool.h"
#include "srsran/common/security.h"
#include "srsran/common/standard_streams.h"
#include "srsran/common/threads.h"
#include "srsran/interfaces/cnw_interface_enb.h"
#include "srsran/srslog/srslog.h"
// #include "srsran/common/pcap_net.h"
#include <cstddef>
#include <stdint.h>
#include <unistd.h>
#include "srscnw/hdr/adp.h"
#include <algorithm>

#include "cnw_adp_interface.h"

#define SMS_CALL_UE_TO_NETWOEK 0
#define CALL_NETWORE_TO_UE 1
#define SMS_NETWORE_TO_UE 2

const int sms_call_control = SMS_CALL_UE_TO_NETWOEK;    //0 主叫业务   1 被叫语音业务  2 被叫短信业务 

#define ENABLE_TC_MODE 1
#define DISENABLE_TC_MODE 0
const int tc_control = DISENABLE_TC_MODE;    //0 TC模式关闭 1 TC模式开�?

#define ENABLE_SMS_TC_MODE 1
#define DISENABLE_SMS_TC_MODE 0

const uint8_t tc_sms_flag=ENABLE_SMS_TC_MODE;


namespace srsepc {

#define LTE_FDD_ENB_IND_HE_N_BITS 5
#define LTE_FDD_ENB_IND_HE_MASK 0x1FUL
#define LTE_FDD_ENB_IND_HE_MAX_VALUE 31
#define LTE_FDD_ENB_SEQ_HE_MAX_VALUE 0x07FFFFFFFFFFUL

//const string conf_filename ="./access/cnw/cnw.conf";


typedef enum {
  TC_NAS_MSG_NULL=0, 
  TC_MSG_ATE_NAS_CONTROL ,//测管对NAS层级进行控制指示（触发信道切换等�?
  TC_MSG_ATE_NAS_CONTROL_RES ,// NAS回复ATE层控制指令执行完�?
  TC_MSG_NAS_ATE_INFO, //NAS上报相应消息内容
  TC_NAS_MSG_MAX,
}TC_NAS_MSG;


typedef enum {
  ATE_TARGET_LEVEL_IMS=0, 
  ATE_TARGET_LEVEL_NAS ,
  ATE_TARGET_LEVEL_RRC ,
  ATE_TARGET_LEVEL_ICMP ,
}ATE_TARGET_LEVEL;

typedef enum {
  TC_NAS_CONTROL_NULL=0, 
  TC_NAS_CONTROL_EXCHANGE_CHANNEL, //exchange channel
  TC_NAS_CONTROL_EXCHANGE_BEAM,
  TC_NAS_CONTROL_MAX,
}TC_NAS_CONTROL;

typedef enum {
  EXCHANGE_BEAM_NULL=0, 
  EXCHANGE_BEAM_DATA, 
  EXCHANGE_BEAM_CALL,
  EXCHANGE_BEAM_NOT_TASK,
  EXCHANGE_BEAM_MAX,
}TC_NAS_CONTROL_EXCHANGE_BEAM_TYPE;

typedef struct {
  int                 fd;
  uint64_t            imsi;
  enum nas_timer_type type;
} mme_timer_t;

typedef struct {
  int32_t                      enb_id;
  uint16_t                     rnti;
  srsran::unique_byte_buffer_t pdu;
} cnw_pdu_t;


struct pri_key_s{
    uint8_t avail;
    uint8_t scheme;
    uint8_t key[32]; /* 32 bytes Private Key */
};

class xw_icmp;

class cnw : public srsran::thread, public cnw_interface_enb, public cnw_interface_cnwadp
{

public:
  cnw();
  virtual ~cnw();
  static cnw* get_instance(void);
  static void cleanup(void);

  // Logs
  srslog::basic_logger& m_cnw_logger = srslog::fetch_basic_logger("CNW");

  int  init(cnw_args_t& args , adp *adp_);
  int  init(cnw_args_t& args);
  void stop();
  void run_thread();

  // adp --> cnw 
  virtual void run_cnw();

  // cnw -> enb api
  /*cy add*/
  virtual void initial_ue(uint16_t rnti, srsran::unique_byte_buffer_t pdu);
  virtual void write_pdu(uint16_t rnti,  srsran::unique_byte_buffer_t pdu);
  virtual bool send_reg_or_service_accept(uint16_t rnti);

  virtual void release_enb(uint16_t enb_id);
  virtual void s_user_release(uint16_t rnti);
  virtual void rrc_notify_nas_to_release(uint16_t rnti);
  
  virtual void setup_enb(uint16_t enb_id, enb_ctx_t enb_ctx);
  virtual bool rrc_get_tmsi_5g(uint16_t rnti,uint8_t tmsi_s_5g[]);

  //cnw->pcs_ims interface
  virtual void rrc_to_pcs_ims(uint16_t rnti, srsran::unique_byte_buffer_t pdu);

  //6.1 
  bool read_db_file(std::string db_filename);
  bool release_nas_ctx(uint16_t rnti);


  // cnw -> nas database
  nas_context* find_nas_ctx_from_suci(uint64_t suci);
  bool delete_ue_nas_ctx(srsepc::nas_guti guti);
  bool delete_ue_nas_ctx(uint32_t tmsi_5g);
  nas_context* find_nas_ctx_from_guti(srsepc::nas_guti guti);
  nas_context* find_nas_ctx_from_rnti(uint32_t rnti);
  nas_context* find_nas_ctx_from_tmsi_5g(uint32_t tmsi_5g);   
  bool add_nas_ctx_to_tmsi_5g_map(nas_context* nas_ctx);

  bool add_nas_ctx_to_suci_map(nas_context* nas_ctx);
  bool add_nas_ctx_to_guti_map(nas_context* nas_ctx);
  bool add_nas_ctx_to_rnti_map(nas_context* nas_ctx);
  bool check_guti_from_m_tmsi(uint8_t* m_tmsi, nas_context* nas_ctx);
  cnw_args_t* get_cnw_args();

  // cnw -> ue database
  ue_ctx_t* get_ue_ctx(uint64_t suci);
  char* get_default_supi();
  void increment_ue_sqn(ue_ctx_t* ue_ctx);
  void increment_sqn(uint8_t* sqn, uint8_t* next_sqn);
  bool release_ue_nrcm_ctx(uint32_t enb_ue_id);
  uint32_t allocate_m_tmsi(uint64_t suci);
  pri_key_s find_hnet(uint8_t pki);
  void add_supi_to_suci_map(uint64_t suci, uint64_t supi);
  uint64_t get_supi_from_suci(uint64_t suci);
  ue_ctx_t* get_ue_ctx_by_default_supi();

  bool handle_enb_rx_pdu(srsran::unique_byte_buffer_t pdu, uint16_t enb_id, uint16_t enb_ue_id, bool is_initial_msg);

  bool handle_ate_msg(srsran::unique_byte_buffer_t pdu);   
  bool handle_s1u_pdu(srsran::unique_byte_buffer_t pdu);

  bool send_pcap_nas_pdu_to_enb(enb_msg_type pcap_dir, const uint8_t* pdu, int len);
  void cnw_print_func(uint8_t *pdu, int pdu_len, string pdu_info="");
  
  adp cnw_adp;

  nas_mm*     m_nas_mm;
  nas_sm*     m_nas_sm;
  pcs_ims*    m_pcs_ims;
  xw_icmp*    m_xw_icmp;

private:
  // Timer Methods
  void handle_timer_expire(int timer_fd);

  static cnw* m_instance;

  bool       m_running;
  fd_set     m_set;
  cnw_args_t cnw_args;

  // Timer map
  std::vector<mme_timer_t> timers;

  // message queue
  srsran::dyn_blocking_queue<cnw_pdu_t> rx_pdu_queue;

  /* Database */
  /* save enb info.*/
  std::map<uint16_t, enb_ctx_t*> m_active_enbs;

  /* suci - nas_cxt */
  std::map<uint64_t, nas_context*> m_suci_to_nas_ctx;

  /* guti - nas_cxt */
  std::map<nas_guti, nas_context*> m_guti_to_nas_ctx;

  /* rnti - nas_cxt - temp? */
  std::map<uint32_t, nas_context*> m_rnti_to_nas_ctx;

    /* tmsi - nas_cxt*/
  std::map<uint32_t, nas_context*> m_tmsi_to_nas_ctx;

  /* suci - ue_cxt */
  std::map<uint64_t, std::unique_ptr<ue_ctx_t> > m_suci_to_ue_ctx;

  /* tmsi - suci */
  std::map<uint32_t, uint64_t> m_tmsi_to_suci;

  uint32_t m_next_tmsi;

    /* suci - supi */
  std::map<uint64_t, uint64_t> m_suci_to_supi;

  /* supi - ue_cxt */
  std::map<uint64_t, std::unique_ptr<ue_ctx_t>> m_supi_to_ue_ctx;

  char m_default_supi[16];

  /* hnet */
  pri_key_s hnet[255];
  // pass-through rrc entity
  srsenb::rrc_interface_cnw* rrc;
};


} // namespace srsepc
#endif // SRSRAN_CNW_H