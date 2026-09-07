/*******************************************************************************
 *
 *                     UL-CCCH Ghannel Information
 *
 ******************************************************************************/

#ifndef SRSASN1_RRC_S_UL_CCCH_MSG_H
#define SRSASN1_RRC_S_UL_CCCH_MSG_H


#include "mib_sib_asn1.h"
#include <cstdio>
#include <stdarg.h>

namespace asn1 {
namespace rrc {


// 5G-S-TMSI ::=SEQUENCE
struct nr_s_tmsi_s {
  // member variables
  fixed_bitstring<48> nr_s_tmsi;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// AccessStratumRelease ::=ENUMERARED
struct access_strat_rel_opts {
  enum options { r1, r2, spare6, spare5, spare4, spare3, spare2, spare1, nulltype } value;

  const char* to_string() const;
  uint8_t     to_number() const;
};
typedef enumerated<access_strat_rel_opts> access_strat_rel_e;

// InitialUE-Identity ::= CHOICE
struct init_ue_wx_id_c {
  struct types_opts {
    enum options { nr_s_tmsi, random_value, nulltype } value;

    const char* to_string() const;
  };
  typedef enumerated<types_opts> types;

  // choice methods
  init_ue_wx_id_c() = default;
  init_ue_wx_id_c(const init_ue_wx_id_c& other);
  init_ue_wx_id_c& operator=(const init_ue_wx_id_c& other);
  ~init_ue_wx_id_c() { destroy_(); }
  void        set(types::options e = types::nulltype);
  types       type() const { return type_; }
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
  // getters
  nr_s_tmsi_s& nr_s_tmsi()
  {
    assert_choice_type(types::nr_s_tmsi, type_, "InitialUE-Identity");
    return c.get<nr_s_tmsi_s>();
  }
  fixed_bitstring<48>& random_value()
  {
    assert_choice_type(types::random_value, type_, "InitialUE-Identity");
    return c.get<fixed_bitstring<48> >();
  }
  const nr_s_tmsi_s& nr_s_tmsi() const
  {
    assert_choice_type(types::nr_s_tmsi, type_, "InitialUE-Identity");
    return c.get<nr_s_tmsi_s>();
  }
  const fixed_bitstring<48>& random_value() const
  {
    assert_choice_type(types::random_value, type_, "InitialUE-Identity");
    return c.get<fixed_bitstring<48> >();
  }

  nr_s_tmsi_s& set_nr_s_tmsi();
  fixed_bitstring<48>& set_random_value();

private:
  types                                             type_;
  choice_buffer_t<fixed_bitstring<48>, nr_s_tmsi_s> c;

  void destroy_();
};

// EstablishmentCause ::= ENUMERATED
struct estab_cause_opts {
  enum options { emergency, high_prio_access, mt_access, mo_sig, mo_data, geo_access, spare1, spare2, nulltype } value;

  const char* to_string() const;
};
typedef enumerated<estab_cause_opts> estab_cause_e;

// UE-Capability ::= SEQUENCE
struct ue_cap_s {
    struct ue_doubleMode_opts {
    enum options { True, False, nulltype } value;

    const char* to_string() const;
  };
    typedef enumerated<ue_doubleMode_opts> ue_doubleMode_e_;

  // member variables
  bool               ext = false;
  access_strat_rel_e access_strat_rel;
  uint8_t            ue_category_r1 = 1;
  ue_doubleMode_e_   ue_doubleMode;
  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// GeographicalInfo ::=SEQUENCE
struct geo_gra_info_s {
  // member variables
  fixed_bitstring<40> geo_gra_info;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// ReestablishmentCause ::=ENUMERATED
struct reest_caus_opts {
  enum options { reconfigurationfailure, handoverfailure, otherfailure, spare1 ,nulltype} value;

  const char* to_string()const;
};
typedef enumerated<reest_caus_opts> reest_caus_e;

// RRCConnectionReestablishmentRequest-r1-IEs ::=SEQUENCE
struct rrc_con_reest_req_r1_s {
  // member variables
  bool               ext = false;
  fixed_bitstring<6>  s_rnti;
  freq_id_s           freq_id;
  band_id_s           band_id;
  beam_id_s           tri_beam_id;
  fixed_bitstring<64> shortMac_I;
  reest_caus_e        reest_cause;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// RRCConnectionRequest-r1-IEs ::= SEQUENCE
struct rrc_con_req_r1_ie_s {
  // member variables
  bool            ext = false;
  init_ue_wx_id_c ue_id;
  estab_cause_e   establishment_cause;
  ue_cap_s        ue_cap;
  geo_gra_info_s  ue_geo_gra_info;
  fixed_bitstring<8> geo_security_para;  
  //...

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// RRCConnectionReestablishmentRequest ::=SEQUENCE
struct rrc_con_reest_req_s {
  // member variables
  rrc_con_reest_req_r1_s rrc_con_reest_req;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// RRCConnectionRequest ::= SEQUENCE
struct rrc_con_req_s {
  // member variables
  rrc_con_req_r1_ie_s rrc_con_req_r1;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

//UL-CCCH-MessageType :: =CHOICE
struct s_ul_ccch_msg_type_c {
  struct types_opts {
    enum options { rrc_con_reest_req, rrc_con_req, /*...*/ nulltype } value;

    const char* to_string() const;
  };
  typedef enumerated<types_opts,true> types;

  // choice methods
  s_ul_ccch_msg_type_c() = default;
  s_ul_ccch_msg_type_c(const s_ul_ccch_msg_type_c& other);
  s_ul_ccch_msg_type_c& operator=(const s_ul_ccch_msg_type_c& other);
  ~s_ul_ccch_msg_type_c() { destroy_(); }
  void        set(types::options e = types::nulltype);
  types       type() const { return type_; }
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
  // getters
  rrc_con_reest_req_s& rrc_con_reest_req()
  {
    assert_choice_type(types::rrc_con_reest_req, type_, "s_ul_ccch_msg_type");
    return c.get<rrc_con_reest_req_s>();
  }
  rrc_con_req_s& rrc_con_req()
  {
    assert_choice_type(types::rrc_con_req, type_, "s_ul_ccch_msg_type");
    return c.get<rrc_con_req_s>();
  }
  const rrc_con_reest_req_s& rrc_con_reest_req() const
  {
    assert_choice_type(types::rrc_con_reest_req, type_, "s_ul_ccch_msg_type");
    return c.get<rrc_con_reest_req_s>();
  }
  const rrc_con_req_s& rrc_con_req() const
  {
    assert_choice_type(types::rrc_con_req, type_, "s_ul_ccch_msg_type");
    return c.get<rrc_con_req_s>();
  }
  rrc_con_reest_req_s& set_rrc_con_reest_req();
  rrc_con_req_s&       set_rrc_con_req();

private:
  types                                               type_;
  choice_buffer_t<rrc_con_reest_req_s, rrc_con_req_s> c;

  void destroy_();
};

// UL-CCCH-Message ::=SEQUENCE
struct s_ul_ccch_msg_s {
  s_ul_ccch_msg_type_c msg;

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
} // namespace rrc
} // namespace asn1

#endif // SRSASN1_RRC_ULCCCH_MSG_H