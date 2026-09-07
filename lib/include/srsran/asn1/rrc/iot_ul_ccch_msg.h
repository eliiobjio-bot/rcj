/*******************************************************************************
 *
 *
 *                  ASN.1 IoT Message UL-CCCH-Message (2023.08.08)
 *
 *
 ******************************************************************************/

#ifndef SRSASN1_IoT_UL_CCCH_MSG_H
#define SRSASN1_IoT_UL_CCCH_MSG_H


#include "srsran/asn1/asn1_utils.h"
#include "s_ul_ccch_msg.h"
#include <cstdio>
#include <stdarg.h>

namespace asn1 {
namespace rrc {

/*******************************************************************************
 *                              Struct Definitions
 ******************************************************************************/
// 5G-S-TMSI ::=SEQUENCE
    struct iot_nr_s_tmsi_s
    {
         // member variables
        fixed_bitstring<48> iot_5g_s_tmsi;

        // member methods
        SRSASN_CODE pack(bit_ref& bref) const;
        SRSASN_CODE unpack(cbit_ref& bref);
        void        to_json(json_writer& j) const;
    };
//UE-Identity::= CHOICE
struct ue_id_c {
  struct types_opts {
    enum options { iot_5g_s_tmsi, random_value, /*...*/ nulltype } value;

    const char* to_string() const;
  };
  typedef enumerated<types_opts> types;

  // choice methods
  ue_id_c() = default;
  ue_id_c(const ue_id_c& other);
  ue_id_c& operator=(const ue_id_c& other);
  ~ue_id_c() { destroy_(); }
  void        set(types::options e = types::nulltype);
  types       type() const { return type_; }
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
  // getters
  iot_nr_s_tmsi_s& iot_5g_s_tmsi()
  {
    assert_choice_type(types::iot_5g_s_tmsi, type_, "InitialUE-Identity");
    return c.get<iot_nr_s_tmsi_s>();
  }
  fixed_bitstring<48>& random_value()
  {
    assert_choice_type(types::random_value, type_, "InitialUE-Identity");
    return c.get<fixed_bitstring<48> >();
  }
  const iot_nr_s_tmsi_s& iot_5g_s_tmsi() const
  {
    assert_choice_type(types::iot_5g_s_tmsi, type_, "InitialUE-Identity");
    return c.get<iot_nr_s_tmsi_s>();
  }
  const fixed_bitstring<48>& random_value() const
  {
    assert_choice_type(types::random_value, type_, "InitialUE-Identity");
    return c.get<fixed_bitstring<48> >();
  }

  iot_nr_s_tmsi_s&      set_iot_5g_s_tmsi();
  fixed_bitstring<48>& set_random_value();

private:
  types                                             type_;
  choice_buffer_t<fixed_bitstring<48>, iot_nr_s_tmsi_s> c;

  void destroy_();
};

//IoTEstabilshmentCause::= ENUMERATED
struct iot_estableishment_cause_opts {
  enum options {
    highPriorityAccess,
    mt_Access,
    mo_Signalling,
    mo_Data,
    mcs_PriorityAccess,
    spare2,
    spare1,
    nulltype
  } value;

  const char* to_string() const;
};
typedef enumerated<iot_estableishment_cause_opts> iot_estableishment_cause_e;

//IoTQosInfo::= SEQUENCE
struct iot_qos_info_s {
  struct delay_budget_opts {
    enum options { critical, spare2, spare1, tolerant, nulltype } value;

    const char* to_string() const;
   };
  typedef enumerated<delay_budget_opts> delay_budget_e_;

  struct reliablity_budget_opts {
   enum options { critical, spare2, spare1, tolerant, nulltype } value;

   const char* to_string() const;
  };
  typedef enumerated<reliablity_budget_opts> reliablity_budget_e_;

   // member variables
   delay_budget_e_      delay_butget;
   reliablity_budget_e_ reliablity_budget;
   fixed_bitstring<4>   spare;

   // member methods
   SRSASN_CODE pack(bit_ref& bref) const;
   SRSASN_CODE unpack(cbit_ref& bref);
   void        to_json(json_writer& j) const;
};

//IoTUeCapability::= SEQUENCE
struct iot_ue_capability_s {
   struct fdd_duplex_mode_opts {
   enum options { full, half, nulltype } value;

   const char* to_string() const;
   };
   typedef enumerated<fdd_duplex_mode_opts> fdd_duplex_mode_e_;

   struct hgnss_support_opts {
   enum options { True, False, nulltype } value;

   const char* to_string() const;
   };
   typedef enumerated<hgnss_support_opts> hgnss_support_e_;

   struct valid_ephemeris_opts {
   enum options { True, False, nulltype } value;

   const char* to_string() const;
   };
   typedef enumerated<valid_ephemeris_opts> valid_ephemeris_e_;

   // member variables
   fdd_duplex_mode_e_   fdd_duplex_mod;
   hgnss_support_e_     hgnss_support;
   valid_ephemeris_e_   valid_ephemeris;
   fixed_bitstring<3>   process_duration_for_ul;
   fixed_bitstring<3>   process_duration_for_dl;
   fixed_bitstring<5>   spare;

   // member methods
   SRSASN_CODE pack(bit_ref& bref) const;
   SRSASN_CODE unpack(cbit_ref& bref);
   void        to_json(json_writer& j) const;
};

//IoTRRCReestablishmentRequest-r1-IEs::= SEQUENCE
struct iot_rrc_resst_req_r1_IEs_s {
  // member variables
  bool                ext = false;
  iot_nr_s_tmsi_s     reestab_ue_id;
   beam_id_s    reestab_ue_id_l;
  fixed_bitstring<16> ul_mas_mac;
  fixed_bitstring<5>  ul_nas_count;
  // ...
  
  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

//IoTRRCSetupRequset-r1-IEs::= SEQUENCE
struct iot_rrc_setup_req_r1_IEs_s {
  // member variables
  bool                       ext = false;
  bool                       iot_qos_info_present = false;
  bool                       iot_ue_cap_present   = false;
  bool                       ue_geo_info_present  = false;
  bool                       geo_security_para_present = false;
  ue_id_c                    ue_id;
  iot_estableishment_cause_e iot_estableishment_cause;
  iot_qos_info_s             iot_qos_info;
  iot_ue_capability_s        iot_ue_cap;
  geo_gra_info_s             ue_geo_info;
  fixed_bitstring<8>         geo_security_para;
  // ...

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

//IoTRRCReestablishmentRequest::= SEQUENCE
struct iot_rrc_resst_req_s {
  // member variables
  iot_rrc_resst_req_r1_IEs_s iot_rrc_resst_req_r1;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

//IoTRRCSetupRequest::= SEQUENCE
struct iot_rrc_setup_req_s {
  // member variables
  iot_rrc_setup_req_r1_IEs_s iot_rrc_setup_req_r1;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

//UL-CCCH-MessageType::= CHIOCE
struct iot_ul_ccch_msg_type_c {
  struct types_opts {
    enum options { iot_rrc_resst_req, iot_rrc_setup_req, /*...*/ nulltype } value;

    const char* to_string() const;
  };
  typedef enumerated<types_opts, true> types;

  // choice methods
  iot_ul_ccch_msg_type_c() = default;
  iot_ul_ccch_msg_type_c(const iot_ul_ccch_msg_type_c& other);
  iot_ul_ccch_msg_type_c& operator=(const iot_ul_ccch_msg_type_c& other);
  ~iot_ul_ccch_msg_type_c() { destroy_(); }
  void        set(types::options e = types::nulltype);
  types       type() const { return type_; }
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;

  // getters
  iot_rrc_resst_req_s& iot_rrc_resst_req()
  {
    assert_choice_type(types::iot_rrc_resst_req, type_, "tbcch_msg_type_c");
    return c.get<iot_rrc_resst_req_s>();
  }
  const iot_rrc_resst_req_s& iot_rrc_resst_req() const
  {
    assert_choice_type(types::iot_rrc_resst_req, type_, "tbcch_msg_type_c");
    return c.get<iot_rrc_resst_req_s>();
  }
  iot_rrc_setup_req_s& iot_rrc_setup_req()
  {
    assert_choice_type(types::iot_rrc_setup_req, type_, "tbcch_msg_type_c");
    return c.get<iot_rrc_setup_req_s>();
  }
  const iot_rrc_setup_req_s& iot_rrc_setup_req() const
  {
    assert_choice_type(types::iot_rrc_setup_req, type_, "tbcch_msg_type_c");
    return c.get<iot_rrc_setup_req_s>();
  }

  iot_rrc_resst_req_s& set_iot_rrc_resst_req();
  iot_rrc_setup_req_s& set_iot_rrc_setup_req();


private:
  types                                                     type_;
  choice_buffer_t<iot_rrc_resst_req_s, iot_rrc_setup_req_s> c;

  void destroy_();
};

// UL-CCCH-Message::= SEQUENCE
struct iot_ul_ccch_msg_s {
  // member variables
  iot_ul_ccch_msg_type_c msg;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
}
} // namespace asn1
#endif // SRSASN1_IoT_UL_CCCH_MSG_H
