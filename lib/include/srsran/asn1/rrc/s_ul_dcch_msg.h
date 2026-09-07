/*******************************************************************************
 *
 *                     UL-DCCH Ghannel Information
 *
 ******************************************************************************/

#ifndef SRSASN1_RRC_S_UL_DCCH_MSG_H
#define SRSASN1_RRC_S_UL_DCCH_MSG_H

#include "mib_sib_asn1.h"
#include "s_dl_ccch_msg.h"
#include "s_ul_ccch_msg.h"
#include <cstdio>
#include <stdarg.h>

namespace asn1 {
namespace rrc {
// BeamList ::=SEQUENCE(SIZE(1...maxBeamReport)) OF BeamIndex = INTEGER(0..maxGap = 15)
using beam_list_l = bounded_array<uint8_t, 16>;

// MeasResult ::=SEQUENCE
struct meas_result_s {
  struct meas_res_s {
    rssi_c rssi;
  };

  // member variables
  bool       ext              = false;
  bool       meas_res_present = false;
  beam_id_s  beam_id;
  meas_res_s meas_res;

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// MeasResultList ::=SEQUENCE(SIZE(1...maxGap)) OF MeasResult
using meas_res_list_l = dyn_array<meas_result_s>;

// AMF-Identifier ::=SEQUENCE
struct amf_id_s {
  fixed_bitstring<24> amf_id;

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// RegisteredAMF ::=SEQUENCE
struct registered_amf_s {
  // member variables
  bool      plmn_id_present = false;
  plmn_id_s plmn_id;
  amf_id_s  amf_id;

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// S-NSSAI ::=CHOICE
struct s_nssai_c {
  struct types_opts {
    enum options { sst, sst_sd, nulltype } value;

    const char* to_string() const;
  };
  typedef enumerated<types_opts> types;

  // choice methods
  s_nssai_c() = default;
  s_nssai_c(const s_nssai_c& other);
  s_nssai_c& operator=(const s_nssai_c& other);
  ~s_nssai_c() { destroy_(); }
  void        set(types::options e = types::nulltype);
  types       type() const { return type_; }
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
  // getters
  fixed_bitstring<8>& sst()
  {
    assert_choice_type(types::sst, type_, "InitialUE-Identity");
    return c.get<fixed_bitstring<8> >();
  }
  fixed_bitstring<32>& sst_sd()
  {
    assert_choice_type(types::sst_sd, type_, "InitialUE-Identity");
    return c.get<fixed_bitstring<32> >();
  }
  const fixed_bitstring<8>& nr_s_tmsi() const
  {
    assert_choice_type(types::sst, type_, "InitialUE-Identity");
    return c.get<fixed_bitstring<8> >();
  }
  const fixed_bitstring<32>& random_value() const
  {
    assert_choice_type(types::sst_sd, type_, "InitialUE-Identity");
    return c.get<fixed_bitstring<32> >();
  }

  fixed_bitstring<8>&  set_sst();
  fixed_bitstring<32>& set_sst_sd();

private:
  types                                                     type_;
  choice_buffer_t<fixed_bitstring<8>, fixed_bitstring<32> > c;

  void destroy_();
};
// SEQUENCE(SIZE(1..maxNrofS-NSSAI))OF S-NSSAI
using s_nssai_list_l = dyn_array<s_nssai_c>;

// Rlf-Report-r1 ::=SEQUENCE
struct rlf_report_r1_s {
  struct meas_res_last_serv_beam_s {
    rssi_c rssi;
  };
  struct meas_res_neigh_beam_s {
    bool            meas_res_list_present = false;
    meas_res_list_l meas_res_list;
  };

  // member variables
  bool                      ext                         = false;
  bool                      meas_res_neigh_beam_present = false;
  meas_res_last_serv_beam_s meas_res_last_serv_beam;
  meas_res_neigh_beam_s     meas_res_neigh_beam;
  //...

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// DedicatedInfoSCM ::=SEQUENCE
struct ded_info_scm_s {
  dyn_octstring ded_info_scm;

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
// DedicatedInfoNAS ::=SEQUENCE
struct ded_info_nas_s {
  dyn_octstring ded_info_nas;
  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// MeasResults ::=SEQUENCE
struct meas_ress_s {
  // member variables
  bool        ext = false;
  beam_list_l beam_list;

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// GeographicalInfo ::=SEQUENCE
struct geo_info_s {
  fixed_bitstring<40> geo_info;

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// MeasurementReport-r1-IEs ::=SEQUENCE
struct measure_rep_r1_s {
  // member variables
  bool        ext                 = false;
  bool        meas_result_present = false;
  bool        ue_geo_info_present = false;
  meas_ress_s meas_result;
  geo_info_s  ue_geo_info;
  //...

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// MeasurementReport ::=SEQUENCE
struct measure_rep_s {
  // member variables
  measure_rep_r1_s measure_rep_r1;

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// RRCCoonnectionReconfigurationComplete-r1-IEs ::=SEQUENCE
struct rrc_con_recon_com_r1_s {
  bool ext = false;

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// RRCCoonnectionReconfigurationComplete ::=SEQUENCE
struct rrc_con_recon_comp_s {
  // member variables
  rrc_transaction_id_s   rrc_transaction_id;
  rrc_con_recon_com_r1_s rrc_con_recon_com_r1;

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// RRCConnectionReestablishmentComplete-r1-IEs ::=SEQUENCE
struct rrc_con_reest_com_r1_s {
  bool ext               = false;
  bool rlf_info_ava_r1_e = false;

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// RRCConnectionReestablishmentComplete ::=SEQUENCE
struct rrc_con_reest_comp_s {
  // member variables
  rrc_transaction_id_s   rrc_transaction_id;
  rrc_con_reest_com_r1_s rrc_con_reest_com_r1;

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// RRCConnectionSetupComplete-r1-IEs ::=SEQUENCE
struct rrc_con_setup_com_r1_s {
  // member variables
  bool             ext                    = false;
  bool             registered_amf_present = false;
  bool             s_nssai_list_present   = false;
  bool             ded_info_nas_present   = false;
  uint8_t          selecte_plmn_id        = 1;
  registered_amf_s registered_amf;
  s_nssai_list_l   s_nssai_list;
  ded_info_nas_s   ded_info_nas;
  //...

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// RRCConnectionSetupComplete ::=SEQUENCE
struct rrc_con_setup_comp_s {
  // member variables
  rrc_transaction_id_s   rrc_transaction_id;
  rrc_con_setup_com_r1_s rrc_con_setup_com_r1;

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// SecurityModeComplete-r1-IEs ::=SEQUENCE
struct security_mode_comp_r1_s {
  // member variables
  bool ext = false;
  //...

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// SecurityModeComplete ::=SEQUENCE
struct security_mode_comp_s {
  // member variables
  rrc_transaction_id_s    rrc_transaction_id;
  security_mode_comp_r1_s security_mode_comp_r1;

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// SecurityModeFailure-r1-IEs ::=SEQUENCE
struct security_mode_fail_r1_s {
  // member variables
  bool ext = false;
  //...

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// SecurityModeFailure ::=SEQUENCE
struct secur_mode_fail_s {
  // member variables
  rrc_transaction_id_s    rrc_transaction_id;
  security_mode_fail_r1_s security_mode_fail_r1;

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// UEGeoInfoTransfer-r1-IEs ::=SEQUENCE
struct ue_geo_info_trans_r1_s {
  // member variables
  bool       ext = false;
  geo_info_s ue_geo_info;
  //...

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// UEGeoInfoTransfer ::=SEQUENCE
struct ue_geo_info_trans_s {
  // member variables
  ue_geo_info_trans_r1_s ue_geo_info_trans_r1;

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// UEIdentity-Transfer-r1-IEs ::=SEQUENCE
struct ue_id_trans_r1_s {
  // member variables
  nr_s_tmsi_s ng_nr_s_tmsi;

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// UEIdentity-Transfer ::=SEQUENCE
struct ue_id_trans_s {
  // member variables
  ue_id_trans_r1_s ue_id_trans_r1;

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// UEInformationResponse-r1-IEs ::=SEQUENCE
struct ue_info_response_r1_s {
  struct rach_report_r1_s {
    uint8_t num_of_rach_sent_r1 = 1;
  };

  // member variables
  bool             ext                = false;
  bool             rlf_report_present = false;
  rach_report_r1_s rach_report_r1;
  rlf_report_r1_s  rlf_report;
  //...

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// UEInformationResponse-r1 ::=SEQUENCE
struct ue_info_response_s {
  // member variables
  rrc_transaction_id_s  rrc_transaction_id;
  ue_info_response_r1_s ue_info_response_r1;

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// ULInformationTransfer-r1-IEs ::=SEQUENCE
 struct ded_info_type_c {
    struct types_opts {
      enum options { ded_info_nas, ded_info_scm, /*...*/ nulltype } value;

      const char* to_string() const;
    };
    typedef enumerated<types_opts,true> types;

    // choice methods
    ded_info_type_c() = default;
    ded_info_type_c(const ded_info_type_c& other);
    ded_info_type_c& operator=(const ded_info_type_c& other);
    ~ded_info_type_c() { destroy_(); }
    void        set(types::options e = types::nulltype);
    types       type() const { return type_; }
    SRSASN_CODE pack(bit_ref& bref) const;
    SRSASN_CODE unpack(cbit_ref& bref);
    void        to_json(json_writer& j) const;

    // getters
    ded_info_nas_s& ded_info_nas()
    {
      assert_choice_type(types::ded_info_nas, type_, "ded_info_type_c");
      return c.get<ded_info_nas_s>();
    }
    ded_info_scm_s& ded_info_scm()
    {
      assert_choice_type(types::ded_info_scm, type_, "ded_info_type_c");
      return c.get<ded_info_scm_s>();
    }
    const ded_info_nas_s& ded_info_nas() const
    {
      assert_choice_type(types::ded_info_nas, type_, "ded_info_type_c");
      return c.get<ded_info_nas_s>();
    }
    const ded_info_scm_s& ded_info_scm() const
    {
      assert_choice_type(types::ded_info_scm, type_, "ded_info_type_c");
      return c.get<ded_info_scm_s>();
    }

    ded_info_nas_s& set_ded_info_nas();
    ded_info_scm_s& set_ded_info_scm();

  private:
    types                                           type_;
    choice_buffer_t<ded_info_nas_s, ded_info_scm_s> c;

    void destroy_();
  };
struct ul_info_trans_r1_s {

  // member variables
  ded_info_type_c ded_info_type;

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// ULInformationTransfer ::=SEQUENCE
struct ul_info_trans_s {
  // member variables
  ul_info_trans_r1_s ul_info_trans_r1;

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// UL-DCCH-MessageType ::=CHOICE
struct s_ul_dcch_msg_type_c {
  struct types_opts {
    enum options {
      measure_report,
      rrc_con_recon_comp,
      rrc_con_reest_comp,
      rrc_con_setup_comp,
      security_mode_comp,
      security_mode_fail,
      ue_geo_info_trans,
      ue_id_trans,
      ue_info_response,
      ul_info_trans,
      /*...*/
      nulltype
    } value;

    const char* to_string() const;
  };
  typedef enumerated<types_opts, true> types;

  // choice methods
  s_ul_dcch_msg_type_c() = default;
  s_ul_dcch_msg_type_c(const s_ul_dcch_msg_type_c& other);
  s_ul_dcch_msg_type_c& operator=(const s_ul_dcch_msg_type_c& other);
  ~s_ul_dcch_msg_type_c() { destroy_(); }
  void        set(types::options e = types::nulltype);
  types       type() const { return type_; }
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
  // getters
  measure_rep_s& measure_rep()
  {
    assert_choice_type(types::measure_report, type_, "s_ul_dcch_msg_type_c");
    return c.get<measure_rep_s>();
  }
  rrc_con_recon_comp_s& rrc_con_recon_comp()
  {
    assert_choice_type(types::rrc_con_recon_comp, type_, "s_ul_dcch_msg_type_c");
    return c.get<rrc_con_recon_comp_s>();
  }
  rrc_con_reest_comp_s& rrc_con_reest_comp()
  {
    assert_choice_type(types::rrc_con_reest_comp, type_, "s_ul_dcch_msg_type_c");
    return c.get<rrc_con_reest_comp_s>();
  }
  rrc_con_setup_comp_s& rrc_con_setup_comp()
  {
    assert_choice_type(types::rrc_con_setup_comp, type_, "s_ul_dcch_msg_type_c");
    return c.get<rrc_con_setup_comp_s>();
  }
  security_mode_comp_s& security_mode_comp()
  {
    assert_choice_type(types::security_mode_comp, type_, "s_ul_dcch_msg_type_c");
    return c.get<security_mode_comp_s>();
  }
  secur_mode_fail_s& secur_mode_fail()
  {
    assert_choice_type(types::security_mode_fail, type_, "s_ul_dcch_msg_type_c");
    return c.get<secur_mode_fail_s>();
  }
  ue_geo_info_trans_s& ue_geo_info_trans()
  {
    assert_choice_type(types::ue_geo_info_trans, type_, "s_ul_dcch_msg_type_c");
    return c.get<ue_geo_info_trans_s>();
  }
  ue_id_trans_s& ue_id_trans()
  {
    assert_choice_type(types::ue_id_trans, type_, "s_ul_dcch_msg_type_c");
    return c.get<ue_id_trans_s>();
  }
  ue_info_response_s& ue_info_response()
  {
    assert_choice_type(types::ue_info_response, type_, "s_ul_dcch_msg_type_c");
    return c.get<ue_info_response_s>();
  }
  ul_info_trans_s& ul_info_trans()
  {
    assert_choice_type(types::ul_info_trans, type_, "s_ul_dcch_msg_type_c");
    return c.get<ul_info_trans_s>();
  }
  const measure_rep_s& measure_rep() const
  {
    assert_choice_type(types::measure_report, type_, "s_ul_dcch_msg_type_c");
    return c.get<measure_rep_s>();
  }
  const rrc_con_recon_comp_s& rrc_con_recon_comp() const
  {
    assert_choice_type(types::rrc_con_recon_comp, type_, "s_ul_dcch_msg_type_c");
    return c.get<rrc_con_recon_comp_s>();
  }
  const rrc_con_reest_comp_s& rrc_con_reest_comp() const
  {
    assert_choice_type(types::rrc_con_reest_comp, type_, "s_ul_dcch_msg_type_c");
    return c.get<rrc_con_reest_comp_s>();
  }
  const rrc_con_setup_comp_s& rrc_con_setup_comp() const
  {
    assert_choice_type(types::rrc_con_setup_comp, type_, "s_ul_dcch_msg_type_c");
    return c.get<rrc_con_setup_comp_s>();
  }
  const security_mode_comp_s& security_mode_comp() const
  {
    assert_choice_type(types::security_mode_comp, type_, "s_ul_dcch_msg_type_c");
    return c.get<security_mode_comp_s>();
  }
  const secur_mode_fail_s& secur_mode_fail() const
  {
    assert_choice_type(types::security_mode_fail, type_, "s_ul_dcch_msg_type_c");
    return c.get<secur_mode_fail_s>();
  }
  const ue_geo_info_trans_s& ue_geo_info_trans() const
  {
    assert_choice_type(types::ue_geo_info_trans, type_, "s_ul_dcch_msg_type_c");
    return c.get<ue_geo_info_trans_s>();
  }
  const ue_id_trans_s& ue_id_trans() const
  {
    assert_choice_type(types::ue_id_trans, type_, "s_ul_dcch_msg_type_c");
    return c.get<ue_id_trans_s>();
  }
  const ue_info_response_s& ue_info_response() const
  {
    assert_choice_type(types::ue_info_response, type_, "s_ul_dcch_msg_type_c");
    return c.get<ue_info_response_s>();
  }
  const ul_info_trans_s& ul_info_trans() const
  {
    assert_choice_type(types::ul_info_trans, type_, "s_ul_dcch_msg_type_c");
    return c.get<ul_info_trans_s>();
  }

  measure_rep_s&        set_measure_rep();
  rrc_con_recon_comp_s& set_rrc_con_recon_comp();
  rrc_con_reest_comp_s& set_rrc_con_reest_comp();
  rrc_con_setup_comp_s& set_rrc_con_setup_comp();
  security_mode_comp_s& set_security_mode_comp();
  secur_mode_fail_s&    set_secur_mode_fail();
  ue_geo_info_trans_s&  set_ue_geo_info_trans();
  ue_id_trans_s&        set_ue_id_trans();
  ue_info_response_s&   set_ue_info_response();
  ul_info_trans_s&      set_ul_info_trans();

private:
  types type_;
  choice_buffer_t<measure_rep_s,
                  rrc_con_recon_comp_s,
                  rrc_con_reest_comp_s,
                  rrc_con_setup_comp_s,
                  security_mode_comp_s,
                  secur_mode_fail_s,
                  ue_geo_info_trans_s,
                  ue_id_trans_s,
                  ue_info_response_s,
                  ul_info_trans_s>
      c;

  void destroy_();
};

// UL-DCCH-Message ::=SEQUENCE
struct s_ul_dcch_msg_s {
  s_ul_dcch_msg_type_c msg;

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
} // namespace rrc
} // namespace asn1

#endif // SRSASN1_RRC_ULCCCH_MSG_H
