/*******************************************************************************
 *
 *
 *                  ASN.1 IoT Message DL-CCCH-Message (2023.08.08)
 *
 *
 ******************************************************************************/

#ifndef SRSASN1_IoT_DL_CCCH_MSG_H
#define SRSASN1_IoT_DL_CCCH_MSG_H


#include "srsran/asn1/asn1_utils.h"
#include "s_dl_ccch_msg.h"
#include <cstdio>
#include <stdarg.h>

namespace asn1 {
namespace rrc {

/*******************************************************************************
 *                              Struct Definitions
 ******************************************************************************/

//IoTT-Reordering::= ENUMERATED
struct iot_t_reordering_opts {
  enum options { ms100, ms500, ms1000, ms2000, nulltype } value;

  const char* to_string() const;
  uint8_t     to_number() const;
};
typedef enumerated<iot_t_reordering_opts> iot_t_reordering_e;
    

//IoTRRCSetup-r1-IEs::= SEQUENCE
struct iot_rrc_setup_r1_IEs_s {
  // member variables
  bool               ext = false;
  iot_t_reordering_e iot_t_reorder;
  // ...

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

//IoTRRCReject-r1-IEs::= SEQUENCE
struct iot_rrc_reject_r1_IEs_s {
  struct geo_accept_opts {
    enum options { True, nulltype } value;

    const char* to_string() const;
  };
  typedef enumerated<geo_accept_opts> geo_accept_e_;

  // member variables
  bool               ext = false;
  bool               redierection_info_present = false;
  bool               geo_accept_present        = false;
  redirection_info_s redierection_info;
  geo_accept_e_      geo_accept;
  // ...
  
  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

//IoTRRCReestablishment-r1::= SEQUENCE
struct iot_rrc_resst_r1_s {
  struct rcv_um_rlc_entity_opts {
    enum options { recover, reestablish, nulltype } value;

    const char* to_string() const;
  };
  typedef enumerated<rcv_um_rlc_entity_opts> rcv_um_rlc_entity_e_;

  struct trans_um_rlc_entity_opts {
    enum options { recover, reestablish, nulltype } value;

    const char* to_string() const;
  };
  typedef enumerated<trans_um_rlc_entity_opts> trans_um_rlc_entity_e_;
  // member variables
  bool                   ext = false;
  iot_t_reordering_e     iot_t_reorder;
  rcv_um_rlc_entity_e_   rcv_um_rlc_entity;
  trans_um_rlc_entity_e_ trans_um_rlc_entity;
  fixed_bitstring<16>    dl_nas_mac;
  // ...

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};


//IoTRRCResstablishment::= SEQUENCE
struct iot_rrc_resst_s {
  // member variables
  rrc_transaction_id_s rrc_transaction_id;
  iot_rrc_resst_r1_s   iot_rrc_resst_r1;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};


//oTRRCReestablishmentReject::=SEQUENCE
struct iot_rrc_reest_reject_r1_ie_s {

    bool ext = false;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
    // IoTRRCResstablishmentReject::= SEQUENCE
struct iot_rrc_reest_reject_s {
  // member variables
  iot_rrc_reest_reject_r1_ie_s  iot_rrc_reest_reject_r1_ies;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};


//IoTRRCReject::= SEQUENCE
struct iot_rrc_reject_s {
  // member variables
  iot_rrc_reject_r1_IEs_s iot_rrc_reject_r1;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

//IoTRRCSetup::= SEQUENCE
struct iot_rrc_setup_s {
  // member variables
  rrc_transaction_id_s   rrc_transaction_id;
  iot_rrc_setup_r1_IEs_s iot_rrc_setup_r1;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};


//DL-CCCH-Message::= CHIOCE
struct iot_dl_ccch_msg_type_c {
  struct types_opts {
    enum options { iot_rrc_resst, iot_rrc_reest_reject, iot_rrc_reject, iot_rrc_setup, /*...*/ nulltype } value;

    const char* to_string() const;
  };
  typedef enumerated<types_opts,true> types;

  // choice methods
  iot_dl_ccch_msg_type_c() = default;
  iot_dl_ccch_msg_type_c(const iot_dl_ccch_msg_type_c& other);
  iot_dl_ccch_msg_type_c& operator=(const iot_dl_ccch_msg_type_c& other);
  ~iot_dl_ccch_msg_type_c() { destroy_(); }
  void        set(types::options e = types::nulltype);
  types       type() const { return type_; }
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;

  // getters
  iot_rrc_resst_s& iot_rrc_resst()
  {
    assert_choice_type(types::iot_rrc_resst, type_, "iot_dl_ccch_msg_type_c");
    return c.get<iot_rrc_resst_s>();
  }
  const iot_rrc_resst_s& iot_rrc_resst() const
  {
    assert_choice_type(types::iot_rrc_resst, type_, "iot_dl_ccch_msg_type_c");
    return c.get<iot_rrc_resst_s>();
  }
  iot_rrc_reest_reject_s& iot_rrc_reest_reject()
  {
    assert_choice_type(types::iot_rrc_reest_reject, type_, "iot_dl_ccch_msg_type_c");
    return c.get<iot_rrc_reest_reject_s>();
  }
  const iot_rrc_reest_reject_s& iot_rrc_reest_reject() const
  {
    assert_choice_type(types::iot_rrc_reest_reject, type_, "iot_dl_ccch_msg_type_c");
    return c.get<iot_rrc_reest_reject_s>();
  }
  iot_rrc_reject_s& iot_rrc_reject()
  {
    assert_choice_type(types::iot_rrc_reject, type_, "iot_dl_ccch_msg_type_c");
    return c.get<iot_rrc_reject_s>();
  }
  const iot_rrc_reject_s& iot_rrc_reject() const
  {
    assert_choice_type(types::iot_rrc_reject, type_, "iot_dl_ccch_msg_type_c");
    return c.get<iot_rrc_reject_s>();
  }
  iot_rrc_setup_s& iot_rrc_setup()
  {
    assert_choice_type(types::iot_rrc_setup, type_, "iot_dl_ccch_msg_type_c");
    return c.get<iot_rrc_setup_s>();
  }
  const iot_rrc_setup_s& iot_rrc_setup() const
  {
    assert_choice_type(types::iot_rrc_setup, type_, "iot_dl_ccch_msg_type_c");
    return c.get<iot_rrc_setup_s>();
  }
  iot_rrc_resst_s&        set_iot_rrc_resst();
  iot_rrc_reest_reject_s& set_iot_rrc_reest_reject();
  iot_rrc_reject_s&       set_iot_rrc_reject();
  iot_rrc_setup_s&        set_iot_rrc_setup();

private:
  types                      type_;
  choice_buffer_t<iot_rrc_resst_s, iot_rrc_reest_reject_s, iot_rrc_reject_s, iot_rrc_setup_s> c;

  void destroy_();
};

//DL-CCCH-Message::= SEQUENCE
struct iot_dl_ccch_msg_s {
  // member variables
  iot_dl_ccch_msg_type_c msg;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
}
} // namespace asn1
#endif // SRSASN1_IoT_DL_CCCH_MSG_H
