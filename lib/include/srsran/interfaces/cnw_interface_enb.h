#ifndef SRSRAN_CNW_INTERFACES_H
#define SRSRAN_CNW_INTERFACES_H

#include "srsran/asn1/gtpc_ies.h"
#include "srsran/common/common.h"
#include"srsran/common/security.h"
#include <netinet/sctp.h>
#include <queue>

namespace srsepc
{

  enum nas_timer_type
  {
    T_3413,
  };

  typedef struct {
  bool        enable;
  std::string client_ip;
  std::string bind_ip;
  uint16_t    client_port;
  uint16_t    bind_port;
} pcap_net_args_t;

  typedef struct
  {
    bool enb_name_present;
    uint32_t enb_id;
    std::string enb_name;
    uint16_t mcc, mnc;
    uint32_t plmn;
    uint8_t nof_supported_ta;
    //   std::array<uint16_t, MAX_TA>                        tacs;
    //   std::array<uint16_t, MAX_BPLMN>                     nof_supported_bplmns;
    //   std::array<std::array<uint32_t, MAX_BPLMN>, MAX_TA> bplmns;
    //   asn1::s1ap::paging_drx_opts                         drx;
    // struct sctp_sndrcvinfo                              sri;
  } enb_ctx_t;

  typedef struct
  {
    bool ttcn_nasmm_enble;
    bool ttcn_nassm_enble;
    bool ttcn_test_enble;

    /* AMF */
    uint16_t tac;          // 16-bit tac
    uint16_t mcc = 0xf460; // BCD-coded with 0xF filler
    
    uint16_t mnc = 0xff00; // BCD-coded with 0xF filler
    uint8_t amf_region_id = 0xfe;
    uint16_t amf_set_id = 0x0001;
    uint8_t amf_pointer = 0x01;
    uint8_t t3512_value;
    // uint16_t                            paging_timer; // Paging timer in sec (T3413)
    std::string dns_addr;
    std::string full_net_name;
    std::string short_net_name;
    srsran::CIPHERING_ALGORITHM_ID_ENUM encryption_algo = srsran::CIPHERING_ALGORITHM_ID_EEA0;
    srsran::INTEGRITY_ALGORITHM_ID_ENUM integrity_algo = srsran::INTEGRITY_ALGORITHM_ID_128_EIA1;
    bool request_imeisv;

    /* UPF */
    std::string set_ue_ip_adrr;
    std::string sgi_if_addr;
    std::string sgi_if_name;
    uint32_t max_paging_queue;

    /* user DB */
    std::string user_db_file;
    std::string config_file;

    /*  ip */
    std::string cnw_ip_adrr;
    uint16_t         cnw_port;

    std::string enb1_ip_adrr;
    uint16_t         enb1_port;
    std::string enb2_ip_adrr;
    uint16_t         enb2_port;

    std::string ate1_ip_adrr;
    uint16_t         ate1_port;
    std::string ate2_ip_adrr;
    uint16_t         ate2_port;

    pcap_net_args_t pcap_net;

    /* Log */
    std::string log_all_level;
    int log_all_hex_limit;
    std::string log_filename;
  } cnw_args_t;

  /******************
   * CNW Interfaces *
   ******************/
  class cnw_interface_enb // CNW -> ENB
  {
  public:
    // virtual void write_ul_info(srsran::unique_byte_buffer_t pdu, uint16_t enb_id, uint16_t enb_ue_s1ap_id) = 0;
    virtual void release_enb(uint16_t enb_id) = 0;
    virtual void setup_enb(uint16_t enb_id, enb_ctx_t enb_ctx) = 0;
    virtual bool handle_enb_rx_pdu(srsran::unique_byte_buffer_t pdu, uint16_t enb_id, uint16_t enb_ue_id, bool is_initial_msg) = 0;
    // virtual void print() = 0;

    /*cy add*/
    virtual void write_pdu(uint16_t rnti, srsran::unique_byte_buffer_t pdu) = 0;
    virtual void initial_ue(uint16_t rnti, srsran::unique_byte_buffer_t pdu) = 0;
    // virtual bool send_reg_accept(uint16_t rnti) =0;
    virtual bool send_reg_or_service_accept(uint16_t rnti) = 0;

    virtual void rrc_to_pcs_ims(uint16_t rnti, srsran::unique_byte_buffer_t pdu) = 0;
    virtual void s_user_release(uint16_t rnti) = 0;
    virtual void rrc_notify_nas_to_release(uint16_t rnti) = 0;

    virtual bool rrc_get_tmsi_5g(uint16_t rnti, uint8_t tmsi_s_5g[]) = 0;
  };

} // namespace srsepc
#endif // SRSRAN_CNW_INTERFACES_H
