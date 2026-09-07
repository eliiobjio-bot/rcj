/******************************************************************************
 * File:        nas_context.h
 * Description: Top-level NAS SM class. Creates and links all
 *              interfaces and helpers.
 *****************************************************************************/

#ifndef SRSEPC_NAS_CONTEXT_H
#define SRSEPC_NAS_CONTEXT_H

#include <cstddef>
#include <netinet/in.h>
#include <stdint.h>
#include <unistd.h>

#include "nas_context.h"
#include "srsran/asn1/nas_5g_ies.h"
#include "srsran/asn1/nas_5g_msg.h"
#include "srsran/common/bcd_helpers.h"
#include "srsran/common/security.h"
#include "srsran/srslog/srslog.h"

using namespace srsran;
using namespace nas_5g;

namespace srsepc {

/*enum start */
enum hss_auth_algo { HSS_ALGO_XOR, HSS_ALGO_MILENAGE };
enum pro_type { PROC_NULL, PROC_REGISTERED, PROC_SERVICE };

typedef enum {
  NRCM_STATE_IDLE = 0,
  NRCM_STATE_CONNECTED,
  NRCM_STATE_MAX,
} nrcm_state_t;
static const char nrcm_state_text[NRCM_STATE_MAX][100] = {"IDLE", "CONNECTED"};

typedef enum {
  NRMM_STATE_DEREGISTERED = 0,
  NRMM_STATE_COMMON_PROCEDURE_INITIATED,
  NRMM_STATE_REGISTERED,
  NRMM_STATE_DEREGISTERED_INITIATED,
  NRMM_STATE_MAX,
} nrmm_state_t;
static const char nrmm_state_text[NRMM_STATE_MAX][100] = {"DEREGISTERED",
                                                          "COMMON PROCEDURE INITIATED",
                                                          "REGISTERED",
                                                          "DEREGISTERED INITIATED"};
/*enum end*/

/*mm context struct start*/
typedef struct {
#define NR_NAS_CONTEXT_DEREG_RE_REGISTRATION_NOT_REQUIRED 0
#define NR_NAS_CONTEXT_DEREG_RE_REGISTRATION__REQUIRED 1
  uint8_t ReregistrationRequired;
} DeregCtx;

typedef struct {
  bool ea0_5g_supported;
  bool ea1_128_5g_supported;
  bool ea2_128_5g_supported;
  bool ea3_128_5g_supported;
  bool ea4_5g_supported;
  bool ea5_5g_supported;
  bool ea6_5g_supported;
  bool ea7_5g_supported;
  bool ia0_5g_supported;
  bool ia1_128_5g_supported;
  bool ia2_128_5g_supported;
  bool ia3_128_5g_supported;
  bool ia4_5g_supported;
  bool ia5_5g_supported;
  bool ia6_5g_supported;
  bool ia7_5g_supported;
  bool eps_caps_present;
  bool eea0_supported;
  bool eea1_128_supported;
  bool eea2_128_supported;
  bool eea3_128_supported;
  bool eea4_supported;
  bool nea13_supported;
  bool nea14_supported;
  bool eea7_supported;
  bool eia0_supported;
  bool eia1_128_supported;
  bool eia2_128_supported;
  bool eia3_128_supported;
  bool eia4_supported;
  bool nia13_supported;
  bool nia14_supported;
  bool eia7_supported;
} sec_ue_security_capability_t;

class nas_guti
{
public:
  bool operator<(const nas_guti& args) const
  {
    for (uint8_t i = 0; i < 10; i++) {
      if (args.guti[i] != guti[i]) {
        return false;
      }
    }
    return true;
  }
  bool operator==(const nas_guti args) const
  {
    for (uint8_t i = 0; i < 10; i++) {
      if (args.guti[i] != guti[i]) {
        return false;
      }
    }
    return true;
  }

  nas_guti& operator=(const nas_guti& args)
  {
    for (uint8_t i = 0; i < 10; i++) {
      guti[i] = args.guti[i];
    }
    return *this;
  }

  uint8_t* getGutiAddress(uint8_t index) { return &guti[index]; }

  void setGuti(const uint8_t* args)
  {
    for (uint8_t i = 0; i < 10; i++) {
      guti[i] = args[i];
    }
  }

private:
  uint8_t guti[10];
};

//#define max_reg_accept_msg_len 60
#define max_reg_or_service_accept_msg_len 60
typedef struct {
  uint64_t     suci;
  nas_guti     guti;
  uint32_t     m_tmsi;
  uint64_t     supi;

  uint8_t      tmsi_s_5g[6] = {0x0,0x41,0xc2,0x34,0x56,0x78};
  uint64_t     tmsi_s_5g_value;

  uint32_t     guami;
  nrmm_state_t state;
  DeregCtx     DeregistrationContextInfo;
  uint8_t      reg_type;
  pro_type     e_pro_type;

  bool isSecCplt;

    /*11-30 wcb add*/ //用于暂存注册接受消息和指示消息的有效性
  bool reg_accept_msg_valid;
  // uint8_t reg_accept_msg[max_reg_accept_msg_len];
  // uint32_t reg_accept_msg_len=0;

  bool service_accept_msg_valid; 
  uint8_t reg_or_service_accept_msg[max_reg_or_service_accept_msg_len];
  uint32_t reg_or_service_accept_msg_len=0;
  std::vector<int> test_loop_drb; 
} nrmm_ctx_t;

/*mm context struct end*/

typedef struct {
  nrcm_state_t state;
  uint16_t     enb_id;
  uint32_t     rnti;
  uint32_t     tmsi_5g;   //32位由网络分配标志
} nrcm_ctx_t;

typedef struct {
  bool is_ims=false;
  bool is_accept=true;
  uint8_t modify_flag=1;
  uint8_t ue_session_type[2]={1,1};
  uint8_t ue_ssc_mode[2]={1,1};

  /* Integrity protection maximum data rate */
  struct {
    uint8_t mbr_dl[2];
    uint8_t mbr_ul[2];
  } integrity_protection;

  // PTI
  uint8_t pti[2];
  uint8_t pdu_session_id[2];
  // always_on_pdu_session_requested
  bool apsi[2];
  uint8_t ttcn_num=0;
} nrsm_ctx_t;//12.9

typedef struct {
  uint8_t                             ngksi;
  uint8_t                             k_amf[32];
  uint8_t                             k_ausf[32];
  uint8_t                             k_seaf[32];
  uint8_t                             autn[16];
  uint8_t                             rand[16];
  uint8_t                             xres[16];
  uint32_t                            dl_nas_count;
  uint32_t                            ul_nas_count;
  srsran::CIPHERING_ALGORITHM_ID_ENUM cipher_algo;
  srsran::INTEGRITY_ALGORITHM_ID_ENUM integ_algo;
  uint8_t                             k_nas_enc[32];
  uint8_t                             k_nas_int[32];
  uint8_t                             k_enb[32];
  sec_ue_security_capability_t        ue_network_cap;

  uint8_t                             spare_1_2_version=0x01;  

} sec_ctx_t;

class nas_context
{
public:
  nrmm_ctx_t nrmm_ctx  = {};
  nrcm_ctx_t nrcm_ctx  = {};
  nrsm_ctx_t nrsm_ctx  = {};
  sec_ctx_t  m_sec_ctx = {};

  nas_context() {}
  ~nas_context() {}
  srslog::basic_logger& m_nas_ctx_logger = srslog::fetch_basic_logger("nas_context");

  security_algorithms_t::ciphering_algorithm_type convert_cipher_algo(srsran::CIPHERING_ALGORITHM_ID_ENUM cipher_algo);
  security_algorithms_t::integrity_protection_algorithm_type
  convert_integ_algo(srsran::INTEGRITY_ALGORITHM_ID_ENUM integ_algo);

  key_set_identifier_t::nas_key_set_identifier_type convert_ng_ksi(uint8_t ngksi);
  void                                              nas_print_byte_buffer(srsran::unique_byte_buffer_t nas_buffer);
  bool                                              compare_bytes_value(uint8_t* args1, uint8_t* args2, uint8_t length);
  //12.9
  
  pdu_session_type_t::PDU_session_type_value_type convert_pdu_type(uint8_t ue_session_type);
  ssc_mode_t::SSC_mode_value_type                 convert_ssc_mode(uint8_t ssc_mode);

private:
};

struct ue_ctx_t {
  std::string        name;
  uint64_t           suci;
  uint64_t           imsi;
  std::string        supi;
  enum hss_auth_algo algo;
  uint8_t            key[16];
  bool               op_configured;
  uint8_t            op[16];
  uint8_t            opc[16];
  uint8_t            amf[2];
  uint8_t            sqn[6];
  uint16_t           qci;
  uint8_t            last_rand[16];
  std::string        static_ip_addr;

  void set_sqn(const uint8_t* sqn_) { memcpy(sqn, sqn_, 6); }
  void set_last_rand(const uint8_t* last_rand_) { memcpy(last_rand, last_rand_, 16); }
  void get_last_rand(uint8_t* last_rand_) { memcpy(last_rand_, last_rand, 16); }
};

struct plmn_id_t {
  uint8_t mcc[3];
  uint8_t mnc[3];
  uint8_t nof_mnc_digits;

  plmn_id_t() : mcc(), mnc(), nof_mnc_digits(0) {}
  void reset()
  {
    nof_mnc_digits = 0;
    std::fill(&mnc[0], &mnc[3], 0);
    std::fill(&mcc[0], &mcc[3], 0);
  }
  int from_number(uint16_t mcc_num, uint16_t mnc_num)
  {
    srsran::mcc_to_bytes(mcc_num, &mcc[0]);
    if (not srsran::mnc_to_bytes(mnc_num, &mnc[0], &nof_mnc_digits)) {
      reset();
      return SRSRAN_ERROR;
    }
    return SRSRAN_SUCCESS;
  }
  std::pair<uint16_t, uint16_t> to_number()
  {
    uint16_t mcc_num, mnc_num;
    srsran::bytes_to_mcc(&mcc[0], &mcc_num);
    srsran::bytes_to_mnc(&mnc[0], &mnc_num, nof_mnc_digits);
    return std::make_pair(mcc_num, mnc_num);
  }
  uint32_t to_s1ap_plmn()
  {
    auto     mcc_mnc_pair = to_number();
    uint32_t s1ap_plmn;
    srsran::s1ap_mccmnc_to_plmn(mcc_mnc_pair.first, mcc_mnc_pair.second, &s1ap_plmn);
    return s1ap_plmn;
  }
  void to_s1ap_plmn_bytes(uint8_t* plmn_bytes)
  {
    uint32_t s1ap_plmn = to_s1ap_plmn();
    s1ap_plmn          = htonl(s1ap_plmn);
    uint8_t* plmn_ptr  = (uint8_t*)&s1ap_plmn;
    memcpy(&plmn_bytes[0], plmn_ptr + 1, 3);
  }
  int from_string(const std::string& plmn_str)
  {
    if (plmn_str.size() < 5 or plmn_str.size() > 6) {
      reset();
      return SRSRAN_ERROR;
    }
    uint16_t mnc_num, mcc_num;
    if (not string_to_mcc(std::string(plmn_str.begin(), plmn_str.begin() + 3), &mcc_num)) {
      reset();
      return SRSRAN_ERROR;
    }
    if (not string_to_mnc(std::string(plmn_str.begin() + 3, plmn_str.end()), &mnc_num)) {
      reset();
      return SRSRAN_ERROR;
    }
    return from_number(mcc_num, mnc_num);
  }
  int to_number(uint16_t* mcc_num, uint16_t* mnc_num) const
  {
    srsran::bytes_to_mcc(&mcc[0], mcc_num);
    srsran::bytes_to_mnc(&mnc[0], mnc_num, nof_mnc_digits);
    return SRSRAN_SUCCESS;
  }
  std::string to_string() const
  {
    std::string mcc_str, mnc_str;
    uint16_t    mnc_num, mcc_num;
    bytes_to_mnc(&mnc[0], &mnc_num, nof_mnc_digits);
    bytes_to_mcc(&mcc[0], &mcc_num);
    mnc_to_string(mnc_num, &mnc_str);
    mcc_to_string(mcc_num, &mcc_str);
    return mcc_str + mnc_str;
  }

  std::string to_serving_network_name_string() const
  {
    char        buff[50];
    std::string mcc_str, mnc_str;
    uint16_t    mnc_num, mcc_num;
    bytes_to_mnc(&mnc[0], &mnc_num, nof_mnc_digits);
    bytes_to_mcc(&mcc[0], &mcc_num);
    mnc_to_string(mnc_num, &mnc_str);
    mcc_to_string(mcc_num, &mcc_str);
    if (mnc_str.size() == 2) {
      mnc_str = "0" + mnc_str;
    }
    snprintf(buff, sizeof(buff), "5G:mnc%s.mcc%s.3gppnetwork.org", mnc_str.c_str(), mcc_str.c_str());
    std::string ssn_s = buff;
    return ssn_s;
  }

  bool operator==(const plmn_id_t& other) const
  {
    return std::equal(&mcc[0], &mcc[3], &other.mcc[0]) and nof_mnc_digits == other.nof_mnc_digits and
           std::equal(&mnc[0], &mnc[nof_mnc_digits], &other.mnc[0]);
  }
  bool is_valid() const { return nof_mnc_digits == 2 or nof_mnc_digits == 3; }
};

} // namespace srsepc
#endif // SRSEPC_NAS_CONTEXT_H