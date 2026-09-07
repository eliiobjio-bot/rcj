/*******************************************************************************
 *
 * 
 *                  ASN.1 IoT Message DL-DCCH-Message (2023.08.08)
 *							   	    
 *
 ******************************************************************************/

#ifndef SRSASN1_IoT_DL_DCCH_MSG_H
#define SRSASN1_IoT_DL_DCCH_MSG_H

#include "srsran/asn1/asn1_utils.h"
#include "s_ul_dcch_msg.h"
#include <cstdio>
#include <stdarg.h>

namespace asn1 {
namespace rrc {

/*******************************************************************************
 *                              Struct Definitions
 ******************************************************************************/

//IoTDLInformationTransfer-r1-IEs::= SEQUENCE
struct iot_dl_information_trans_r1_IEs_s {
  // member variables
  ded_info_nas_s ded_info_nas; 

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

//IoTRRCReconfiguration-r1-IEs::= SEQUENCE
struct iot_rrc_reconf_r1_IEs_s {
  // member variables
  bool                 ext = false;
  rrc_transaction_id_s rrc_trans_id;
  ded_info_nas_s       ded_info_nas;
  // ...

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

//IoTRRCRelease-r1-IEs::= SEQUENCE
struct iot_rrc_release_r1_IEs_s {
  // member variables
  bool ext = false;
  // ...

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

//IoTDLInformationTransfer::= SEQUENCE
struct iot_dl_information_trans_s {
  // member variables
  iot_dl_information_trans_r1_IEs_s iot_dl_information_trans_r1;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

//IoTRRCReconfiguration::= SEQUENCE
struct iot_rrc_reconf_s {
  // member variables
  iot_rrc_reconf_r1_IEs_s iot_rrc_reconf_r1;

  // ...

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

//IoTRRCRelease::= SEQUENCE
struct iot_rrc_release_s {
  // member variables
  iot_rrc_release_r1_IEs_s iot_rrc_release_r1;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

//DL-DCCH-MessageType::= CHIOCE
struct iot_dl_dcch_msg_type_c {
  struct types_opts {
    enum options { iot_dl_information_trans, iot_rrc_reconf, iot_rrc_release, /*...*/ nulltype } value;

    const char* to_string() const;
  };
  typedef enumerated<types_opts, true> types;

  // choice methods
  iot_dl_dcch_msg_type_c() = default;
  iot_dl_dcch_msg_type_c(const iot_dl_dcch_msg_type_c& other);
  iot_dl_dcch_msg_type_c& operator=(const iot_dl_dcch_msg_type_c& other);
  ~iot_dl_dcch_msg_type_c() { destroy_(); }
  void        set(types::options e = types::nulltype);
  types       type() const { return type_; }
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;

  // getters
  iot_dl_information_trans_s& iot_dl_information_trans()
  {
    assert_choice_type(types::iot_dl_information_trans, type_, "tbcch_msg_type_c");
    return c.get<iot_dl_information_trans_s>();
  }
  const iot_dl_information_trans_s& iot_dl_information_trans() const
  {
    assert_choice_type(types::iot_dl_information_trans, type_, "tbcch_msg_type_c");
    return c.get<iot_dl_information_trans_s>();
  }
  iot_rrc_reconf_s& iot_rrc_reconf()
  {
    assert_choice_type(types::iot_rrc_reconf, type_, "tbcch_msg_type_c");
    return c.get<iot_rrc_reconf_s>();
  }
  const iot_rrc_reconf_s& iot_rrc_reconf() const
  {
    assert_choice_type(types::iot_rrc_reconf, type_, "tbcch_msg_type_c");
    return c.get<iot_rrc_reconf_s>();
  }
  iot_rrc_release_s& iot_rrc_release()
  {
    assert_choice_type(types::iot_rrc_release, type_, "tbcch_msg_type_c");
    return c.get<iot_rrc_release_s>();
  }
  const iot_rrc_release_s& iot_rrc_release() const
  {
    assert_choice_type(types::iot_rrc_release, type_, "tbcch_msg_type_c");
    return c.get<iot_rrc_release_s>();
  }

  iot_dl_information_trans_s& set_iot_dl_information_trans();
  iot_rrc_reconf_s&           set_iot_rrc_reconf();
  iot_rrc_release_s&          set_iot_rrc_release();

private:
  types                                                                            type_;
  choice_buffer_t<iot_dl_information_trans_s, iot_rrc_reconf_s, iot_rrc_release_s> c;

  void destroy_();
};

// DL-DCCH-Message::= SEQUENCE
struct iot_dl_dcch_msg_s {
  // member variables
  iot_dl_dcch_msg_type_c msg;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
}
} // namespace asn1
#endif // SRSASN1_IoT_DL_DCCH_MSG_H