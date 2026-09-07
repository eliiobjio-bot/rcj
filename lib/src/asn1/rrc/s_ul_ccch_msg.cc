/*******************************************************************************
 *
 *                     UL-CCCH Ghannel Information
 *
 ******************************************************************************/


#include "srsran/asn1/rrc/s_ul_ccch_msg.h"
#include "srsran/asn1/rrc/mib_sib_asn1.h"
#include <sstream>

using namespace asn1;
using namespace asn1::rrc;

// 5G-S-TMSI ::=SEQUENCE
SRSASN_CODE nr_s_tmsi_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(nr_s_tmsi.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE nr_s_tmsi_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(nr_s_tmsi.unpack(bref));
  return SRSASN_SUCCESS;
}
void nr_s_tmsi_s::to_json(json_writer& j) const
{
  j.start_array();
  j.write_str("5G-S-TMSI", nr_s_tmsi.to_string());
  j.end_array();
}

// AccessStratumRelease ::=ENUMERARED
const char* access_strat_rel_opts::to_string() const
{
  static const char* options[] = {"r1", "r2", "spare6", "spare5", "spare4", "spare3", "spare2","spare1"};
  return convert_enum_idx(options, 8, value, "t_poll_retran_e");
}
uint8_t access_strat_rel_opts::to_number() const
{
  static const uint8_t options[] = {1, 2};
  return map_enum_number(options, 2, value, "t_poll_retran_e");
}

// InitialUE-Identity ::= CHOICE
void init_ue_wx_id_c::destroy_()
{
  switch (type_) {
    case types::nr_s_tmsi:
      c.destroy<nr_s_tmsi_s>();
      break;
    case types::random_value:
      c.destroy<fixed_bitstring<48> >();
      break;
    default:
      break;
  }
}
void init_ue_wx_id_c::set(types::options e)
{
  destroy_();
  type_ = e;
  switch (type_) {
    case types::nr_s_tmsi:
      c.init<nr_s_tmsi_s>();
      break;
    case types::random_value:
      c.init<fixed_bitstring<48> >();
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "init_ue_wx_id_c");
  }
}
init_ue_wx_id_c::init_ue_wx_id_c(const init_ue_wx_id_c& other)
{
  type_ = other.type();
  switch (type_) {
    case types::nr_s_tmsi:
      c.init(other.c.get<nr_s_tmsi_s>());
      break;
    case types::random_value:
      c.init(other.c.get<fixed_bitstring<48> >());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "init_ue_wx_id_c");
  }
}
init_ue_wx_id_c& init_ue_wx_id_c::operator=(const init_ue_wx_id_c& other)
{
  if (this == &other) {
    return *this;
  }
  set(other.type());
  switch (type_) {
    case types::nr_s_tmsi:
      c.set(other.c.get<nr_s_tmsi_s>());
      break;
    case types::random_value:
      c.set(other.c.get<fixed_bitstring<48> >());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "init_ue_wx_id_c");
  }

  return *this;
}
nr_s_tmsi_s& init_ue_wx_id_c::set_nr_s_tmsi() 
{
  set(types::nr_s_tmsi);
  return c.get<nr_s_tmsi_s>();
}
fixed_bitstring<48>& init_ue_wx_id_c::set_random_value()
{
  set(types::random_value);
  return c.get<fixed_bitstring<48> >();
}

void init_ue_wx_id_c::to_json(json_writer& j) const
{
  j.start_obj();
  switch (type_) {
    case types::nr_s_tmsi:
      j.write_fieldname("5G-S-TMSI");
      c.get<nr_s_tmsi_s>().to_json(j);
      break;
    case types::random_value:
      j.write_str("randomValue", c.get<fixed_bitstring<48> >().to_string());
      break;
    default:
      log_invalid_choice_id(type_, "init_ue_id_c");
  }
  j.end_obj();
}
SRSASN_CODE init_ue_wx_id_c::pack(bit_ref& bref) const
{
  type_.pack(bref);
  switch (type_) {
    case types::nr_s_tmsi:
      HANDLE_CODE(c.get<nr_s_tmsi_s>().pack(bref));
      break;
    case types::random_value:
      HANDLE_CODE(c.get<fixed_bitstring<48> >().pack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "init_ue_id_c");
      return SRSASN_ERROR_ENCODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE init_ue_wx_id_c::unpack(cbit_ref& bref)
{
  types e;
  e.unpack(bref);
  set(e);
  switch (type_) {
    case types::nr_s_tmsi:
      HANDLE_CODE(c.get<nr_s_tmsi_s>().unpack(bref));
      break;
    case types::random_value:
      HANDLE_CODE(c.get<fixed_bitstring<48> >().unpack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "init_ue_id_c");
      return SRSASN_ERROR_DECODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
const char* init_ue_wx_id_c::types_opts::to_string() const
{
  static const char* options[] = {"nr_s_tmsi", "random_value"};
  return convert_enum_idx(options, 2, value, "init_ue_wx_id_c::types");
}
// EstablishmentCause ::= ENUMERATED
const char* estab_cause_opts::to_string() const
{
  static const char* options[] = {"emergency", "high_prio_access", "mt_access", "mo_sig", "mo_data", "geo_access", "spare1","spare2"};
  return convert_enum_idx(options, 8, value, "estab_cause_e");
}

const char* ue_cap_s::ue_doubleMode_opts::to_string() const
{
  static const char* options[] = {"True", "False"};
  return convert_enum_idx(options, 2, value, "ue_doubleMode");
}

// UE-Capability ::= SEQUENCE
SRSASN_CODE ue_cap_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(access_strat_rel.pack(bref));
  HANDLE_CODE(pack_integer(bref, ue_category_r1, (uint8_t)1u, (uint8_t)16u));
  HANDLE_CODE(ue_doubleMode.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE ue_cap_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(access_strat_rel.unpack(bref));
  HANDLE_CODE(unpack_integer(ue_category_r1, bref, (uint8_t)1u, (uint8_t)16u));
  HANDLE_CODE(ue_doubleMode.unpack(bref));
  return SRSASN_SUCCESS;
}
void ue_cap_s::to_json(json_writer& j) const
{
  j.start_array();
  j.write_str("AccessStratumRelease", access_strat_rel.to_string());
  j.write_int("ue-Category-r1", ue_category_r1);
  j.write_str("ue-DoubleMode", ue_doubleMode.to_string());
  j.end_array();
}

// GeographicalInfo ::=SEQUENCE
SRSASN_CODE geo_gra_info_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(geo_gra_info.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE geo_gra_info_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(geo_gra_info.unpack(bref));
  return SRSASN_SUCCESS;
}
void geo_gra_info_s::to_json(json_writer& j) const
{
  j.start_array();
  j.write_str("GeographicalInfo", geo_gra_info.to_string());
  j.end_array();
}

// ReestablishmentCause ::=ENUMERATED
const char* reest_caus_opts::to_string() const
{
  static const char* options[] = {"reconfigurationFailure", "handoverFailure", "otherFailure", "spare1"};
  return convert_enum_idx(options, 4, value, "reest_cause_e");
}

// RRCConnectionReestablishmentRequest-r1-IEs ::=SEQUENCE
SRSASN_CODE rrc_con_reest_req_r1_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(s_rnti.pack(bref));
  HANDLE_CODE(freq_id.pack(bref));
  HANDLE_CODE(band_id.pack(bref));
  HANDLE_CODE(tri_beam_id.pack(bref));
  HANDLE_CODE(shortMac_I.pack(bref));
  HANDLE_CODE(reest_cause.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE rrc_con_reest_req_r1_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(s_rnti.unpack(bref));
  HANDLE_CODE(freq_id.unpack(bref));
  HANDLE_CODE(band_id.unpack(bref));
  HANDLE_CODE(tri_beam_id.unpack(bref));
  HANDLE_CODE(shortMac_I.unpack(bref));
  HANDLE_CODE(reest_cause.unpack(bref));

  return SRSASN_SUCCESS;
}
void rrc_con_reest_req_r1_s::to_json(json_writer& j) const
{
  j.start_array();
  j.write_str("s-RNTI", s_rnti.to_string());
  j.write_fieldname("frequencyIdentity");
  freq_id.to_json(j);
  j.write_fieldname("bandIdentity");
  band_id.to_json(j);
  j.write_fieldname("beamIdentity");
  tri_beam_id.to_json(j);
  j.write_str("shortMAC-I", shortMac_I.to_string());
  j.write_str("ReestablishmentCause", reest_cause.to_string());
  j.end_array();
}

// RRCConnectionRequest-r1-IEs ::= SEQUENCE
SRSASN_CODE rrc_con_req_r1_ie_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(ue_id.pack(bref));
  HANDLE_CODE(establishment_cause.pack(bref));
  HANDLE_CODE(ue_cap.pack(bref));
  HANDLE_CODE(ue_geo_gra_info.pack(bref));
  HANDLE_CODE(geo_security_para.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE rrc_con_req_r1_ie_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(ue_id.unpack(bref));
  HANDLE_CODE(establishment_cause.unpack(bref));
  HANDLE_CODE(ue_cap.unpack(bref));
  HANDLE_CODE(ue_geo_gra_info.unpack(bref));
  HANDLE_CODE(geo_security_para.unpack(bref));
  return SRSASN_SUCCESS;
}
void rrc_con_req_r1_ie_s::to_json(json_writer& j) const
{
  j.start_array();
  j.write_fieldname("InitialUE-Identity");
  ue_id.to_json(j);
  j.write_str("EstablishmentCause", establishment_cause.to_string());
  j.write_fieldname("UE-Capability");
  ue_cap.to_json(j);
  j.write_fieldname("UE-GeographicalInfo");
  ue_geo_gra_info.to_json(j);
  j.write_str("Geo_security_para", geo_security_para.to_string());
  j.end_array();
}

// RRCConnectionReestablishmentRequest ::=SEQUENCE
SRSASN_CODE rrc_con_reest_req_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(rrc_con_reest_req.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE rrc_con_reest_req_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(rrc_con_reest_req.unpack(bref));
  return SRSASN_SUCCESS;
}
void rrc_con_reest_req_s::to_json(json_writer& j) const
{
  j.start_array();
  j.write_fieldname("RRCConnectionReestablishmentRequest");
  rrc_con_reest_req.to_json(j);
  j.end_array();
}

// RRCConnectionRequest ::= SEQUENCE
SRSASN_CODE rrc_con_req_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(rrc_con_req_r1.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE rrc_con_req_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(rrc_con_req_r1.unpack(bref));
  return SRSASN_SUCCESS;
}
void rrc_con_req_s::to_json(json_writer& j) const
{
  j.start_array();
  j.write_fieldname("RRCConnectionRequest");
  rrc_con_req_r1.to_json(j);
  j.end_array();
}

// UL-CCCH-MessageType :: =CHOICE
void s_ul_ccch_msg_type_c::destroy_()
{
  switch (type_) {
    case types::rrc_con_reest_req:
      c.destroy<rrc_con_reest_req_s>();
      break;
    case types::rrc_con_req:
      c.destroy<rrc_con_req_s>();
      break;
    default:
      break;
  }
}
void s_ul_ccch_msg_type_c::set(types::options e)
{
  destroy_();
  type_ = e;
  switch (type_) {
    case types::rrc_con_reest_req:
      c.init<rrc_con_reest_req_s>();
      break;
    case types::rrc_con_req:
      c.init<rrc_con_req_s>();
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "s_dl_ccch_msg_type_c");
  }
}
s_ul_ccch_msg_type_c::s_ul_ccch_msg_type_c(const s_ul_ccch_msg_type_c& other)
{
  type_ = other.type();
  switch (type_) {
    case types::rrc_con_reest_req:
      c.init(other.c.get<rrc_con_reest_req_s>());
      break;
    case types::rrc_con_req:
      c.init(other.c.get<rrc_con_req_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "s_dl_ccch_msg_type_c");
  }
}
s_ul_ccch_msg_type_c& s_ul_ccch_msg_type_c::operator=(const s_ul_ccch_msg_type_c& other)
{
  if (this == &other) {
    return *this;
  }
  set(other.type());
  switch (type_) {
    case types::rrc_con_reest_req:
      c.set(other.c.get<rrc_con_reest_req_s>());
      break;
    case types::rrc_con_req:
      c.set(other.c.get<rrc_con_req_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "s_dl_ccch_msg_type_c");
  }

  return *this;
}
rrc_con_reest_req_s& s_ul_ccch_msg_type_c::set_rrc_con_reest_req()
{
  set(types::rrc_con_reest_req);
  return c.get<rrc_con_reest_req_s>();
}
rrc_con_req_s& s_ul_ccch_msg_type_c::set_rrc_con_req()
{
  set(types::rrc_con_req);
  return c.get<rrc_con_req_s>();
}
void s_ul_ccch_msg_type_c::to_json(json_writer& j) const
{
  j.start_obj();
  switch (type_) {
    case types::rrc_con_reest_req:
      j.write_fieldname("RRCConnectionReestablishmentRequest");
      c.get<rrc_con_reest_req_s>().to_json(j);
      break;
    case types::rrc_con_req:
      j.write_fieldname("RRCConnectionRequest");
      c.get<rrc_con_req_s>().to_json(j);
      break;
    default:
      log_invalid_choice_id(type_, "s_ul_ccch_msg_type_c");
  }
  j.end_obj();
}
SRSASN_CODE s_ul_ccch_msg_type_c::pack(bit_ref& bref) const
{
  type_.pack(bref);
  switch (type_) {
    case types::rrc_con_reest_req:
      HANDLE_CODE(c.get<rrc_con_reest_req_s>().pack(bref));
      break;
    case types::rrc_con_req:
      HANDLE_CODE(c.get<rrc_con_req_s>().pack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "s_dl_ccch_msg_type_c");
      return SRSASN_ERROR_ENCODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE s_ul_ccch_msg_type_c::unpack(cbit_ref& bref)
{
  types e;
  e.unpack(bref);
  set(e);
  switch (type_) {
    case types::rrc_con_reest_req:
      HANDLE_CODE(c.get<rrc_con_reest_req_s>().unpack(bref));
      break;
    case types::rrc_con_req:
      HANDLE_CODE(c.get<rrc_con_req_s>().unpack(bref));
      break;
    default:
     printf("\n-- default:  ---\n");
      log_invalid_choice_id(type_, "s_dl_ccch_msg_type_c");
      return SRSASN_ERROR_DECODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
const char* s_ul_ccch_msg_type_c::types_opts::to_string() const
{
  static const char* options[] = {"rrc_con_reest_req", "rrc_con_req"};
  return convert_enum_idx(options, 2, value, "s_ul_ccch_msg_type_c::types");
}

// UL-CCCH-Message ::=SEQUENCE
SRSASN_CODE s_ul_ccch_msg_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(msg.pack(bref));

  bref.align_bytes_zero();

  return SRSASN_SUCCESS;
}
SRSASN_CODE s_ul_ccch_msg_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(msg.unpack(bref));

  bref.align_bytes();

  return SRSASN_SUCCESS;
}
void s_ul_ccch_msg_s::to_json(json_writer& j) const
{
  j.start_array();
  j.start_obj();
  j.start_obj("UL-CCCH-Message");
  j.write_fieldname("message");
  msg.to_json(j);
  j.end_obj();
  j.end_obj();
  j.end_array();
}