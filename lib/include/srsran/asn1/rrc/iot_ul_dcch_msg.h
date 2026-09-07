  /*******************************************************************************
 *
 *
 *                  ASN.1 IoT Message DL-DCCH-Message (2023.08.08)
 *
 *
 ******************************************************************************/

#ifndef SRSASN1_IoT_UL_DCCH_MSG_H
#define SRSASN1_IoT_UL_DCCH_MSG_H

#include "srsran/asn1/asn1_utils.h"
#include "s_ul_dcch_msg.h"
#include <cstdio>
#include <stdarg.h>

namespace asn1 {
namespace rrc {

/*******************************************************************************
 *                              Struct Definitions
 ******************************************************************************/
//IoTRRCReconfigurationComplete-r1-IEs::= SEQUENCE
struct iot_rrc_reconf_complete_r1_IEs_s {
  // member variables
  bool ext = false;
  // ...

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

//IoTRRCReestablishmentComplete-r1-IEs::= SEQUENCE
struct iot_rrc_reest_complete_r1_IEs_s {
  // member variables
  bool ext = false;
  // ...

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// IoTRRCSetupComplete-r1-IEs::= SEQUENCE
struct iot_rrc_setup_complete_r1_IEs_s {
  // member variables
  bool               registered_amf_present = false;
  uint8_t            select_plmn_id         = 1;
  registered_amf_s   registered_amf;
  ded_info_nas_s     ded_info_nas_msg;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

//DedicatedInfoNAS
struct iot_ul_trans_r1_IEs_s {
  // member variables
  ded_info_nas_s ded_info_nas;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

//IoTRRCReconfigurationComplete::= SEQUENCE
struct iot_rrc_reconf_complete_s {
  // member variables
  rrc_transaction_id_s             rrc_transaction_id;
  iot_rrc_reconf_complete_r1_IEs_s iot_rrc_reconf_complete_r1_IEs;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

//IoTRRCReestablishmentComplete	::= SEQUENCE
struct iot_rrc_reest_complete_s {
  // member variables
  rrc_transaction_id_s            rrc_transaction_id;
  iot_rrc_reest_complete_r1_IEs_s iot_rrc_reest_complete_r1_IEs;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

//IoTRRCSetupComplete::= SEQUENCE
struct iot_rrc_setup_complete_s {
  // member variables
  rrc_transaction_id_s            rrc_transaction_id;
  iot_rrc_setup_complete_r1_IEs_s iot_rrc_setup_complete_r1_IEs;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// IoTRRCReestablishment::=	SEQUENCE
struct iot_ul_info_trans_s {
  // member variables
  iot_ul_trans_r1_IEs_s iot_ul_trans_r1_IEs;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

//UL-DCCH-MessageType::= CHIOCE
struct iot_ul_dcch_msg_type_c {
  struct types_opts {
    enum options { iot_rrc_reconf_complete,
                   iot_rrc_reest_complete,
                   iot_rrc_setup_complete,
                   iot_ul_info_trans,
                   ue_id_trans,
                   /*...*/ nulltype } value;

    const char* to_string() const;
  };
  typedef enumerated<types_opts, true> types;

  // choice methods
  iot_ul_dcch_msg_type_c() = default;
  iot_ul_dcch_msg_type_c(const iot_ul_dcch_msg_type_c& other);
  iot_ul_dcch_msg_type_c& operator=(const iot_ul_dcch_msg_type_c& other);
  ~iot_ul_dcch_msg_type_c() { destroy_(); }
  void        set(types::options e = types::nulltype);
  types       type() const { return type_; }
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;

  // getters
  iot_rrc_reconf_complete_s& iot_rrc_reconf_complete()
  {
    assert_choice_type(types::iot_rrc_reconf_complete, type_, "iot_ul_dcch_msg_type_c");
    return c.get<iot_rrc_reconf_complete_s>();
  }
  const iot_rrc_reconf_complete_s& iot_rrc_reconf_complete() const
  {
    assert_choice_type(types::iot_rrc_reconf_complete, type_, "iot_ul_dcch_msg_type_c");
    return c.get<iot_rrc_reconf_complete_s>();
  }
  iot_rrc_reest_complete_s& iot_rrc_reest_complete()
  {
    assert_choice_type(types::iot_rrc_reest_complete, type_, "iot_ul_dcch_msg_type_c");
    return c.get<iot_rrc_reest_complete_s>();
  }
  const iot_rrc_reest_complete_s& iot_rrc_reest_complete() const
  {
    assert_choice_type(types::iot_rrc_reest_complete, type_, "iot_ul_dcch_msg_type_c");
    return c.get<iot_rrc_reest_complete_s>();
  }
  iot_rrc_setup_complete_s& iot_rrc_setup_complete()
  {
    assert_choice_type(types::iot_rrc_setup_complete, type_, "iot_ul_dcch_msg_type_c");
    return c.get<iot_rrc_setup_complete_s>();
  }
  const iot_rrc_setup_complete_s& iot_rrc_setup_complete() const
  {
    assert_choice_type(types::iot_rrc_setup_complete, type_, "iot_ul_dcch_msg_type_c");
    return c.get<iot_rrc_setup_complete_s>();
  }
  iot_ul_info_trans_s& iot_ul_info_trans()
  {
    assert_choice_type(types::iot_ul_info_trans, type_, "iot_ul_dcch_msg_type_c");
    return c.get<iot_ul_info_trans_s>();
  }
  const iot_ul_info_trans_s& iot_ul_info_trans() const
  {
    assert_choice_type(types::iot_ul_info_trans, type_, "iot_ul_dcch_msg_type_c");
    return c.get<iot_ul_info_trans_s>();
  }
  ue_id_trans_s& iot_rrc_setup_comp()
  {
    assert_choice_type(types::ue_id_trans, type_, "iot_ul_dcch_msg_type_c");
    return c.get<ue_id_trans_s>();
  }
  const ue_id_trans_s& ue_id_trans() const
  {
    assert_choice_type(types::ue_id_trans, type_, "iot_ul_dcch_msg_type_c");
    return c.get<ue_id_trans_s>();
  }

  iot_rrc_reconf_complete_s& set_iot_rrc_reconf_complete();
  iot_rrc_reest_complete_s&  set_iot_rrc_reest_complete();
  iot_rrc_setup_complete_s&  set_iot_rrc_setup_complete();
  iot_ul_info_trans_s&       set_iot_ul_info_trans();
  ue_id_trans_s&             set_ue_id_trans();

private:
  types type_;
  choice_buffer_t<iot_rrc_reconf_complete_s,
                  iot_rrc_reest_complete_s,
                  iot_rrc_setup_complete_s,
                  iot_ul_info_trans_s,
                  ue_id_trans_s> c;

  void destroy_();
};

// UL-DCCH-Message::= SEQUENCE
struct iot_ul_dcch_msg_s {
  // member variables
  iot_ul_dcch_msg_type_c msg;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
}
} // namespace asn1
#endif // SRSASN1_IoT_UL_DCCH_MSG_H