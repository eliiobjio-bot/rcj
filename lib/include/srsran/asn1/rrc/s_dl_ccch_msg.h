/*******************************************************************************
 *
 *                     RRC connection establishment process
 *
 ******************************************************************************/

#ifndef SRSASN1_RRC_S_DLCCCH_MSG_H
#define SRSASN1_RRC_S_DLCCCH_MSG_H

#include "mib_sib_asn1.h"
#include <cstdio>
#include <stdarg.h>

namespace asn1 {
namespace rrc {

/*******************************************************************************
 *                              Struct Definitions
 ******************************************************************************/

// T-PollRetransmit ::=ENUMERATED
struct t_poll_retran_opts {
  enum options { ms480, ms1200, ms2100, spare1, nulltype } value;

  const char* to_string() const;
  uint16_t    to_number() const;
};
typedef enumerated<t_poll_retran_opts> t_poll_retran_e;

// PollPDU ::=ENUMERATED
struct poll_pdu_s_opts {
  enum options { p8, p16, p32, pinfimity, nulltype } value;

  const char* to_string() const;
  int8_t      to_number() const;
};
typedef enumerated<poll_pdu_s_opts> poll_pdu_s_e;

// PollPDU ::=ENUMERATED
struct poll_byte_s_opts {
  enum options { kB16, kB128, kB256, kBinfimity, nulltype } value;

  const char* to_string() const;
  int16_t     to_number() const;
};
typedef enumerated<poll_byte_s_opts> poll_byte_s_e;

// T-Reordering ::=ENUMERATED
struct t_reorder_opts {
  enum options { ms480, ms1200, ms2100, spare1, nulltype } value;

  const char* to_string() const;
  uint16_t    to_number() const;
};
typedef enumerated<t_reorder_opts> t_reorder_e;

// T-StatusProhibit ::=ENUMERATED
struct t_status_pro_opts {
  enum options { ms0, ms420, ms600, spare1, nulltype } value;

  const char* to_string() const;
  uint16_t    to_number() const;
};
typedef enumerated<t_status_pro_opts> t_status_pro_e;

// UL-AM-RLC ::=SEQUENCE
struct ul_am_rlc_s_s {
  struct max_retx_th_opts {
    enum options { t1, t2, t4, t8, nulltype } value;

    const char* to_string() const;
    uint8_t     to_number() const;
  };
  typedef enumerated<max_retx_th_opts> max_retx_th_e_;

  // member variables
  t_poll_retran_e t_poll_retran;
  poll_pdu_s_e    poll_pdu;
  poll_byte_s_e   poll_byte;
  max_retx_th_e_  max_retx_th;

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// DL-AM-RLC ::=SEQUENCE
struct dl_am_rlc_s_s {
  // member variables
  t_reorder_e    t_reorder;
  t_status_pro_e t_statue_prohibit;

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// UL-UM-RLC ::=SEQUENCE
struct ul_um_rlc_s_s {
  // member variables
  bool ext = false;
  //...

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// DL-UM-RLC ::=SEQUENCE
struct dl_um_rlc_s_s {
  // member variables
  t_reorder_e t_reorder;

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// SDAP-Config ::=SEQUENCE
struct sdap_cfg_s_s {
  struct sdap_hdr_dl_opts {
    enum options { present, absent, nulltype } value;

    const char* to_string() const;
  };
  typedef enumerated<sdap_hdr_dl_opts> sdap_hdr_dl_e_;
  struct sdap_hdr_ul_opts {
    enum options { present, absent, nulltype } value;

    const char* to_string() const;
  };
  typedef enumerated<sdap_hdr_ul_opts> sdap_hdr_ul_e_;
  using mapped_qos_flows_to_add_l     = dyn_array<uint8_t>;
  using mapped_qos_flows_to_release_l = dyn_array<uint8_t>;

  // member variables
  bool                          ext                                 = false;
  bool                          mapped_qos_flows_to_add_present     = false;
  bool                          mapped_qos_flows_to_release_present = false;
  uint16_t                      pdu_session                         = 0;
  sdap_hdr_dl_e_                sdap_hdr_dl;
  sdap_hdr_ul_e_                sdap_hdr_ul;
  bool                          default_drb = false;
  mapped_qos_flows_to_add_l     mapped_qos_flows_to_add;
  mapped_qos_flows_to_release_l mapped_qos_flows_to_release;
  // ...

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// PDCP ::=SEQUENCE
struct pdcp_cfg_s_s {
  struct discard_timer_opts {
    enum options { ms900, ms1200, ms1500, ms3000, ms5100, spare2, spare1, infinity, nulltype } value;

    const char* to_string() const;
  };
  typedef enumerated<discard_timer_opts> discard_timer_e_;

  struct rlc_am_s {
    bool statue_rep_req = false;
  };
  struct rlc_um_s {
    bool ext = false;
  };
  struct rlc_tm_s {
    bool ext = false;
  };
  struct header_compre_c_ {
    struct rohc_s_ {
      struct profiles_s_ {
        bool profile_0x0002 = false;
        bool profile_0x0004 = false;
        bool profile_0x0006 = false;
        bool profile_0x0102 = false;
        bool profile_0x0104 = false;
      };
      bool        ext     = false;
      uint16_t    max_cid = 1;
      profiles_s_ profiles;
    };
    struct types_opts {
      enum options { not_used, rohc, nulltype } value;

      const char* to_string() const;
    };
    typedef enumerated<types_opts> types;
    // choice methods
    header_compre_c_() = default;
    void        set(types::options e = types::nulltype);
    types       type() const { return type_; }
    SRSASN_CODE pack(bit_ref& bref) const;
    SRSASN_CODE unpack(cbit_ref& bref);
    void        to_json(json_writer& j) const;
    // getters
    rohc_s_& rohc()
    {
      assert_choice_type(types::rohc, type_, "PDCP-Confg");
      return c;
    }
    const rohc_s_& rohc() const
    {
      assert_choice_type(types::rohc, type_, "PDCP-Confg");
      return c;
    }
    void     set_not_used();
    rohc_s_& set_rohc();

  private:
    types   type_;
    rohc_s_ c;
  };

  // member variables
  bool             ext                         = false;
  bool             discard_timer_present       = false;
  bool             rlc_am_present              = false;
  bool             rlc_um_present              = false;
  bool             rlc_tm_present              = false;
  bool             intgrity_protection_present = false;
  bool             ciphering_disabled_present  = false;
  discard_timer_e_ discard_timer;
  rlc_am_s         rlc_am;
  rlc_um_s         rlc_um;
  rlc_tm_s         rlc_tm;
  header_compre_c_ header_comper;
  //...

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// RLC-Config ::=CHOICE
struct rlc_cfg_s_c {
  struct types_opts {
    enum options { am, um_bi_dir, tm, nulltype } value;

    const char* to_string() const;
  };
  typedef enumerated<types_opts> types;
  struct am_s_ {
    ul_am_rlc_s_s ul_am_rlc;
    dl_am_rlc_s_s dl_am_rlc;
  };
  struct um_bi_dirrctional_s_ {
    ul_um_rlc_s_s ul_um_rlc;
    dl_um_rlc_s_s dl_um_rlc;
  };

  // choice methods
  rlc_cfg_s_c() = default;
  rlc_cfg_s_c(const rlc_cfg_s_c& other);
  rlc_cfg_s_c& operator=(const rlc_cfg_s_c& other);
  ~rlc_cfg_s_c() { destroy_(); }
  void        set(types::options e = types::nulltype);
  types       type() const { return type_; }
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
  // getters
  am_s_& am()
  {
    assert_choice_type(types::am, type_, "RLC-Config");
    return c.get<am_s_>();
  }
  um_bi_dirrctional_s_& um_bi_dirrctional()
  {
    assert_choice_type(types::um_bi_dir, type_, "RLC-Config");
    return c.get<um_bi_dirrctional_s_>();
  }
  const am_s_& am() const
  {
    assert_choice_type(types::am, type_, "RLC-Config");
    return c.get<am_s_>();
  }
  const um_bi_dirrctional_s_& um_bi_dirrctional() const
  {
    assert_choice_type(types::um_bi_dir, type_, "RLC-Config");
    return c.get<um_bi_dirrctional_s_>();
  }
  am_s_&                set_am();
  um_bi_dirrctional_s_& set_um_bi_dirrctional();
  void                  set_tm();

private:
  types                                        type_;
  choice_buffer_t<am_s_, um_bi_dirrctional_s_> c;

  void destroy_();
};

// LogicalChannelConfig ::=SEQUENCE
struct log_ch_cfg_s {
  struct ul_specific_parameter_s {
    struct priort_bit_rate_opts {
      enum options { kBps0, kBps2dot4, kBps16, kBps128, infinity, spare3, spare2, spare1, nulltype } value;

      const char* to_string() const;
      int16_t     to_number() const;
    };
    typedef enumerated<priort_bit_rate_opts> priort_bit_rate_e_;

    struct bucket_size_duration_opts {
      enum options { ms60, ms120, ms180, ms300, ms600, ms1200, spare2, spare1, nulltype } value;

      const char* to_string() const;
      uint16_t    to_number() const;
    };
    typedef enumerated<bucket_size_duration_opts> bucket_size_duration_e_;

    // member variables
    bool                    log_ch_group_present = false;
    uint8_t                 priority             = 1;
    priort_bit_rate_e_      priort_bit_rate;
    bucket_size_duration_e_ bucket_size_duration;
    uint8_t                 log_ch_group = 0;
  };
  // member variables
  bool                    ext                           = false;
  bool                    ul_specific_parameter_present = false;
  ul_specific_parameter_s ul_specific_parameter;
  //...

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// ChannelType ::=ENUMERATED
struct channel_type_opts {
  enum options {
    psych,
    pdch1_1,
    pdch1_2,
    psch1_1,
    psch1_2,
    psch5_1,
    psch5_2,
    ds_pdtch_1,
    ds_pdtch_2,
    ds_pdtch_3,
    ds_pdtch_t,
    /*...*/
    nulltype
  } value;
  const char* to_string() const;
};
typedef enumerated<channel_type_opts, true> channel_type_e;

// PDTCHCode ::=SEQUENCE
struct pdtch_code_s {
  // member variables
  bool     ext            = false;
  uint16_t pdtch_phy_code = 0;
  //...

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// SRB-ToAdd ::= SEQUENCE
struct srb_to_add_s {
  // member variables
  bool ext = false;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// DRB-Identity ::= SEQUENCE
struct drb_id_s_s {
  // member variables
  uint8_t drb_id = 3;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
// DRB-ToReleaseList  ::=SEQUENCE(SIZE(1??maxDRB))OF DRB-Identity
using drb_to_rel_list_l = dyn_array<drb_id_s_s>;
// DRB-ToAddMod :: = SEQUENCE
struct drb_to_add_mod_s_s {
  // member variables
  bool         sdap_cfg_present   = false;
  bool         pdcp_cfg_present   = false;
  bool         rlc_cfg_present    = false;
  bool         log_ch_cfg_present = false;
  sdap_cfg_s_s sdap_cfg;
  drb_id_s_s   drb_id;
  pdcp_cfg_s_s pdcp_cfg;
  rlc_cfg_s_c  rlc_cfg;
  log_ch_cfg_s log_ch_cfg;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
// DRB-ToAddModList :: =SEQUENCE(SIZE(1..maxDRB))OF DRB-ToAddMod
using drb_to_add_mod_list_s = dyn_array<drb_to_add_mod_s_s>;

// PhysicalChannel-Config ::=SEQUENCE
struct phy_ch_cfg_s {
  struct direction_opts {
    enum options { bidirection, uldirection, dldirection, nulltype } value;

    const char* to_string() const;
  };
  typedef enumerated<direction_opts> direction_e_;
  struct shced_type_opts {
    enum options { static_t, dynamic, nulltype } value;

    const char* to_string() const;
  };
  typedef enumerated<shced_type_opts> shced_type_e_;
  struct voice_type_opts {
    enum options { kbps2point4, kbps4point8,bps800, nulltype } value;

    const char* to_string() const;
  };
  typedef enumerated<voice_type_opts> voice_type_e_;

  // member variables
  bool               ext                = false;
  bool               band_id_present    = false;
  bool               freq_id_present    = false;
  bool               slot_ass_present   = false;
  bool               pdtch_code_present = false;
  bool               shced_type_present = false;
  bool               voice_type_present = false;
  fixed_bitstring<6> s_rnti;
  channel_type_e     ch_type;
  band_id_s          band_id;
  freq_id_s          freq_id;
  fixed_bitstring<5> slot_ass;
  direction_e_       direction;
  pdtch_code_s       pdtch_code;
  shced_type_e_      shced_type;
  voice_type_e_      voice_type;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
// PhysicalChannelList-Config ::=SEQUENCE(SIZE(1..maxNoofPhysicalChannel))OF PhysicalChannel-Config
using phy_ch_list_cfg_s = dyn_array<phy_ch_cfg_s>;

// SecurityAlgorithmConfig ::=SEQUENCE
struct security_alg_cfg_s {
  struct ci_phe_algorithm_opts {
    enum options { nea0, nea1, nea2, nea3, sm4, spare3, spare2, spare1, /*...*/ nulltype } value;

    const char* to_string() const;
    uint8_t     to_number() const;
  };
  typedef enumerated<ci_phe_algorithm_opts, true> ci_phe_algorithm_e_;
  struct int_prot_algorithm_opts {
    enum options { nia0, nia1, nia2, nia3, sm4, spare3, spare2, spare1, /*...*/ nulltype } value;

    const char* to_string() const;
    uint8_t     to_number() const;
  };
  typedef enumerated<int_prot_algorithm_opts, true> int_prot_algorithm_e_;

  // member variables
  bool                  ext                        = false;
  bool                  int_prot_algorithm_present = false;
  ci_phe_algorithm_e_   ci_phe_algorithm;
  int_prot_algorithm_e_ int_prot_algorithm;
  //...

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// securityPayload-Normal		OCTET STRING.
struct security_payload_n_s {
  dyn_octstring security_payload_n_s_;
  SRSASN_CODE   pack(bit_ref& bref) const;
  SRSASN_CODE   unpack(cbit_ref& bref);
  void          to_json(json_writer& j) const;
};
// securityPayload-TtoT			OCTET STRING.
struct security_payload_t_s {
  dyn_octstring security_payload_t_s_;
  SRSASN_CODE   pack(bit_ref& bref) const;
  SRSASN_CODE   unpack(cbit_ref& bref);
  void          to_json(json_writer& j) const;
};

struct security_payload {
  struct types_opts {
    enum options { security_payload_n_, security_payload_t_, nulltype } value;
    typedef uint8_t unmber_type;

    const char* to_string() const;
    uint8_t     to_number() const;
  };
  typedef enumerated<types_opts> types;
  // choice methods
  security_payload() = default;
  security_payload(const security_payload& other);
  security_payload& operator=(const security_payload& other);
  ~security_payload() { this->destroy_(); }
  void  set(types::options e = types::nulltype);
  types type() const { return type_; };

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
  // getters
  security_payload_n_s& security_payload_n()
  {
    assert_choice_type(types::security_payload_n_, type_, " sec_pay_load");
    return c.get<security_payload_n_s>();
  }
  security_payload_t_s& security_payload_t()
  {
    assert_choice_type(types::security_payload_t_, type_, " sec_pay_load");
    return c.get<security_payload_t_s>();
  }
  const security_payload_n_s& security_payload_n() const
  {
    assert_choice_type(types::security_payload_n_, type_, " sec_pay_load");
    return c.get<security_payload_n_s>();
  }
  const security_payload_t_s& security_payload_t() const
  {
    assert_choice_type(types::security_payload_t_, type_, " sec_pay_load");
    return c.get<security_payload_t_s>();
  }
  security_payload_n_s& set_security_payload_n();
  security_payload_t_s& set_security_payload_t();

private:
  types                                                       type_;
  choice_buffer_t<security_payload_n_s, security_payload_t_s> c;

  void destroy_();
};

struct security_Config {
  bool               security_payload_present = false;
  security_alg_cfg_s security_alg_cfg_;
  security_payload   security_payload_;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// PHR-Config ::= SEQUENCE
struct phr_cfg_s_s {
  struct periodic_phr_timer_opts {
    enum options { sf10, sf20, sf50, sf100, sf200, sf500, sf1000, infinity, nulltype } value;
    typedef int16_t number_type;

    const char* to_string() const;
    int16_t     to_number() const;
  };
  typedef enumerated<periodic_phr_timer_opts> periodic_phr_timer_e_;
  struct prohibit_phr_timer_opts {
    enum options { sf0, sf10, sf20, sf50, sf100, sf200, sf500, sf1000, nulltype } value;
    typedef uint16_t number_type;

    const char* to_string() const;
    uint16_t    to_number() const;
  };
  typedef enumerated<prohibit_phr_timer_opts> prohibit_phr_timer_e_;
  struct dl_pathloss_change_opts {
    enum options { db1, db3, db6, infinity, nulltype } value;
    typedef int8_t number_type;

    const char* to_string() const;
    int8_t      to_number() const;
  };
  typedef enumerated<dl_pathloss_change_opts> dl_pathloss_change_e_;

  // member variables
  periodic_phr_timer_e_ periodic_phr_timer;
  prohibit_phr_timer_e_ prohibit_phr_timer;
  dl_pathloss_change_e_ dl_pathloss_change;

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// RadioResurceConfigDedicated ::= SEQUENCE
struct radio_resurce_cfg_ded_s {
  struct periodic_bsr_timer_opts {
    enum options { rf2, rf5, rf10, rf16, rf20, rf32, infinity, spare1, nulltype } value;

    const char* to_string() const;
    uint8_t     to_number() const;
  };
  typedef enumerated<periodic_bsr_timer_opts> periodic_bsr_timer_e_;

  struct phr_cfg_s_c_ {
    struct types_opts {
      enum options { relese, setup, nulltype } value;

      const char* to_string() const;
    };
    typedef enumerated<types_opts> types;

    // choice methods
    phr_cfg_s_c_() = default;
    void        set(types::options e = types::nulltype);
    types       type() const { return type_; }
    SRSASN_CODE pack(bit_ref& bref) const;
    SRSASN_CODE unpack(cbit_ref& bref);
    void        to_json(json_writer& j) const;
    // getters
    phr_cfg_s_s& setup()
    {
      assert_choice_type(types::setup, type_, "phy-Config");
      return c;
    }
    const phr_cfg_s_s& setup() const
    {
      assert_choice_type(types::setup, type_, "phy-Config");
      return c;
    }
    void         set_relese();
    phr_cfg_s_s& set_setup();

  private:
    types       type_;
    phr_cfg_s_s c;
  };

  // member variables
  bool                  ext                         = false;
  bool                  srb_to_add_present          = false;
  bool                  drb_to_add_mod_list_present = false;
  bool                  drb_to_release_list_present = false;
  bool                  periodic_bsr_timer_present  = false;
  bool                  phy_ch_list_cfg_present     = false;
  bool                  security_cfg_present        = false;
  bool                  phr_cfg_present             = false;
  srb_to_add_s          srb_to_add;
  drb_to_add_mod_list_s drb_to_add_mod_list;
  drb_to_rel_list_l     drb_to_release_list;
  periodic_bsr_timer_e_ periodic_bsr_timer;
  phy_ch_list_cfg_s     phy_ch_list_cfg;
  security_Config       security_cfg;
  phr_cfg_s_c_          phr_cfg;
  //...

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// RRC-TransasctionIdentifier ::= SEQUENCE
struct rrc_transaction_id_s {
  // member variables
  uint8_t rrc_t_id = 0;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// RedirectionInfo ::=SEQUENCE
struct redirection_info_s {
  // member variables
  bool    ext     = false;
  uint8_t beam_id = 1;
  //...

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// RRCConnectionReestablishment-r1-IEs ::=SEQUENCE
struct rrc_con_reest_r1_s {
  // member variables
  bool                    ext = false;
  radio_resurce_cfg_ded_s rr_cfg_ded;
  //...

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// RRCConnectionReestablishmentReject-r1-IEs ::=SEQUENCE
struct rrc_con_reest_reject_r1_s {
  // member variables
  bool ext = false;
  //...

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// RRCConnectionReject-r1-IEs ::=SEQUENCE
struct rrc_con_reject_r1_s {
  // member variables
  bool               ext                = false;
  bool               redir_info_present = false;
  bool               geo_accept_present = false;
  redirection_info_s redir_info;
  //...

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// RRCConnectionSetup-r1-IEs ::= SEQUENCE
struct rrc_con_setup_r1_s {
  // member variables
  bool                    ext = false;
  radio_resurce_cfg_ded_s rr_cfg_ded;
  //...

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// RRCConnectionReestablishment ::=SEQUENCE
struct rrc_con_reest_s {
  // member variables
  rrc_transaction_id_s rrc_transaction_id;
  rrc_con_reest_r1_s   rrc_con_reest_r1;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// RRCConnectionReestablishmentReject ::=SEQUENCE
struct rrc_con_reest_reject_s {
  // member variables
  rrc_con_reest_reject_r1_s rrc_con_reest_reject_r1;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// RRCConnectionReject ::=SEQUENCE
struct rrc_con_reject_s {
  // member variables
  rrc_con_reject_r1_s rrc_con_reject;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// RRConnectionSetup :: = SEQUENCE
struct rrc_con_setup_s {
  // member variables
  rrc_transaction_id_s rrc_transaction_id;
  rrc_con_setup_r1_s   rrc_con_setup_r1;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// DL-CCCH-MessageType ::= CHOICE
struct s_dl_ccch_msg_type_c {
  struct types_opts {
    enum options { rrc_con_reest, rrc_con_reest_reject, rrc_con_reject, rrc_con_setup, /*...*/ nulltype } value;

    const char* to_string() const;
  };
  typedef enumerated<types_opts, true> types;

  // choice methods
  s_dl_ccch_msg_type_c() = default;
  s_dl_ccch_msg_type_c(const s_dl_ccch_msg_type_c& other);
  s_dl_ccch_msg_type_c& operator=(const s_dl_ccch_msg_type_c& other);
  ~s_dl_ccch_msg_type_c() { destroy_(); }
  void        set(types::options e = types::nulltype);
  types       type() const { return type_; }
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;

  // getters
  rrc_con_reest_s& rrc_con_reest()
  {
    assert_choice_type(types::rrc_con_reest, type_, "s_dl_ccch_msg_type");
    return c.get<rrc_con_reest_s>();
  }
  rrc_con_reest_reject_s& rrc_con_reest_reject()
  {
    assert_choice_type(types::rrc_con_reest_reject, type_, "s_dl_ccch_msg_type");
    return c.get<rrc_con_reest_reject_s>();
  }
  rrc_con_reject_s& rrc_con_reject()
  {
    assert_choice_type(types::rrc_con_reject, type_, "s_dl_ccch_msg_type");
    return c.get<rrc_con_reject_s>();
  }
  rrc_con_setup_s& rrc_con_setup()
  {
    assert_choice_type(types::rrc_con_setup, type_, "s_dl_ccch_msg_type");
    return c.get<rrc_con_setup_s>();
  }
  const rrc_con_reest_s& rrc_con_reest() const
  {
    assert_choice_type(types::rrc_con_reest, type_, "s_dl_ccch_msg_type");
    return c.get<rrc_con_reest_s>();
  }
  const rrc_con_reest_reject_s& rrc_con_reest_reject() const
  {
    assert_choice_type(types::rrc_con_reest_reject, type_, "s_dl_ccch_msg_type");
    return c.get<rrc_con_reest_reject_s>();
  }
  const rrc_con_reject_s& rrc_con_reject() const
  {
    assert_choice_type(types::rrc_con_reject, type_, "s_dl_ccch_msg_type");
    return c.get<rrc_con_reject_s>();
  }
  const rrc_con_setup_s& rrc_con_setup() const
  {
    assert_choice_type(types::rrc_con_setup, type_, "s_dl_ccch_msg_type");
    return c.get<rrc_con_setup_s>();
  }
  rrc_con_reest_s&        set_rrc_con_reest();
  rrc_con_reest_reject_s& set_rrc_con_reest_reject();
  rrc_con_reject_s&       set_rrc_con_reject();
  rrc_con_setup_s&        set_rrc_con_setup();

private:
  types                                                                                       type_;
  choice_buffer_t<rrc_con_reest_reject_s, rrc_con_reest_s, rrc_con_reject_s, rrc_con_setup_s> c;

  void destroy_();
};

// DL-CCCH-Message ::= SEQUENCE
struct s_dl_ccch_msg_s {
  s_dl_ccch_msg_type_c msg;

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

} // namespace rrc
} // namespace asn1

#endif // SRSASN1_RRC_DLCCCH_MSG_H