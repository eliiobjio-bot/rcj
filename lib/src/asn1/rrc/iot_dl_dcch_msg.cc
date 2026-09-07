#include "srsran/asn1/rrc/iot_dl_dcch_msg.h"
#include <sstream>

using namespace asn1;
using namespace asn1::rrc;
// IoTDLInformationTransfer-r1-IEs::= SEQUENCE
SRSASN_CODE iot_dl_information_trans_r1_IEs_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(ded_info_nas.pack(bref));
}
SRSASN_CODE iot_dl_information_trans_r1_IEs_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(ded_info_nas.unpack(bref));
}
void iot_dl_information_trans_r1_IEs_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("IoTDLInformationTransfer-r1-IEs");
  ded_info_nas.to_json(j);
  j.end_obj();
}
// IoTRRCReconfiguration-r1-IEs::= SEQUENCE
SRSASN_CODE iot_rrc_reconf_r1_IEs_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(rrc_trans_id.pack(bref));
  HANDLE_CODE(ded_info_nas.pack(bref));
}
SRSASN_CODE iot_rrc_reconf_r1_IEs_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(rrc_trans_id.unpack(bref));
  HANDLE_CODE(ded_info_nas.unpack(bref));
}
void iot_rrc_reconf_r1_IEs_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("RRC-TransasctionIdentifier");
  rrc_trans_id.to_json(j);
  j.write_fieldname("DedicatedInfoNAS");
  ded_info_nas.to_json(j);
  j.end_obj();
}
// IoTRRCRelease-r1-IEs::= SEQUENCE 有点小疑问
SRSASN_CODE iot_rrc_release_r1_IEs_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
}
SRSASN_CODE iot_rrc_release_r1_IEs_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
}
void iot_rrc_release_r1_IEs_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.end_obj();
}
// IoTDLInformationTransfer::= SEQUENCE
SRSASN_CODE iot_dl_information_trans_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(iot_dl_information_trans_r1.pack(bref));
}
SRSASN_CODE iot_dl_information_trans_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(iot_dl_information_trans_r1.unpack(bref));
}
void iot_dl_information_trans_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("IoTDLInformationTransfer-r1-IEs");
  iot_dl_information_trans_r1.to_json(j);
  j.end_obj();
}
// IoTRRCReconfiguration::= SEQUENCE
SRSASN_CODE iot_rrc_reconf_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(iot_rrc_reconf_r1.pack(bref));
}
SRSASN_CODE iot_rrc_reconf_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(iot_rrc_reconf_r1.unpack(bref));
}
void iot_rrc_reconf_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("IoTRRCReconfiguration-r1-IEs");
  iot_rrc_reconf_r1.to_json(j);
  j.end_obj();
}
// IoTRRCRelease::= SEQUENCE
SRSASN_CODE iot_rrc_release_s::pack(bit_ref& bref) const
{ 
  HANDLE_CODE(iot_rrc_release_r1.pack(bref)); 
}
SRSASN_CODE iot_rrc_release_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(iot_rrc_release_r1.unpack(bref)); 
}
void iot_rrc_release_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("IoTRRCRelease-r1-IEs");
  iot_rrc_release_r1.to_json(j); 
  j.end_obj();
}
// DL-DCCH-MessageType::= CHIOCE
void iot_dl_dcch_msg_type_c::destroy_()
{
  switch (type_) {
    case types::iot_dl_information_trans:
      c.destroy<iot_dl_information_trans_s>();
      break;
    case types::iot_rrc_reconf:
      c.destroy<iot_rrc_reconf_s>();
      break;
    case types::iot_rrc_release:
      c.destroy<iot_rrc_release_s>();
      break;
    default:
      break;
  }
}
void iot_dl_dcch_msg_type_c::set(types::options e)
{
  destroy_();
  type_ = e;
  switch (type_) {
    case types::iot_dl_information_trans:
      c.init<iot_dl_information_trans_s>(); 
      break;
    case types::iot_rrc_reconf: 
      c.init<iot_rrc_reconf_s>(); 
      break;
    case types::iot_rrc_release:
      c.init<iot_rrc_release_s>();  
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "iot_dl_dcch_msg_type_c");
  }
}
iot_dl_dcch_msg_type_c::iot_dl_dcch_msg_type_c(const iot_dl_dcch_msg_type_c& other)
{
  type_ = other.type();
  switch (type_) {
    case types::iot_dl_information_trans:
      c.init(other.c.get<iot_dl_information_trans_s>()); 
      break;
    case types::iot_rrc_reconf: 
      c.init(other.c.get<iot_rrc_reconf_s>()); 
      break;
    case types::iot_rrc_release: 
      c.init(other.c.get<iot_rrc_release_s>()); 
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "iot_dl_dcch_msg_type_c");
  }
}
iot_dl_dcch_msg_type_c& iot_dl_dcch_msg_type_c::operator=(const iot_dl_dcch_msg_type_c& other)
{
  if (this == &other) {
    return *this;
  }
  set(other.type());
  switch (type_) {
    case types::iot_dl_information_trans:
      c.set(other.c.get<iot_dl_information_trans_s>());
      break;
    case types::iot_rrc_reconf: 
      c.set(other.c.get<iot_rrc_reconf_s>());
      break;
    case types::iot_rrc_release:
      c.set(other.c.get<iot_rrc_release_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "iot_dl_dcch_msg_type_c");
  }

  return *this;
}
iot_dl_information_trans_s& iot_dl_dcch_msg_type_c::set_iot_dl_information_trans()
{
  set(types::iot_dl_information_trans);
  return c.get<iot_dl_information_trans_s>();
}
iot_rrc_reconf_s& iot_dl_dcch_msg_type_c::set_iot_rrc_reconf()
{
  set(types::iot_rrc_reconf);
  return c.get<iot_rrc_reconf_s>();
}
iot_rrc_release_s& iot_dl_dcch_msg_type_c::set_iot_rrc_release()
{
  set(types::iot_rrc_release);
  return c.get<iot_rrc_release_s>();
}
void iot_dl_dcch_msg_type_c::to_json(json_writer& j) const
{
  j.start_obj();
  switch (type_) {
    case types::iot_dl_information_trans:
      j.write_fieldname("IoTDLInformationTransfer");
      c.get<iot_dl_information_trans_s>().to_json(j);
      break;
    case types::iot_rrc_reconf:
      j.write_fieldname("IoTRRCReconfiguration");
      c.get<iot_rrc_reconf_s>().to_json(j);
      break;
    case types::iot_rrc_release:
      j.write_fieldname("IoTRRCRelease");
      c.get<iot_rrc_release_s>().to_json(j);
      break;
    default:
      log_invalid_choice_id(type_, "sec_payload_c_");
  }
  j.end_obj();
}
SRSASN_CODE iot_dl_dcch_msg_type_c::pack(bit_ref& bref) const
{
  type_.pack(bref); 
  switch (type_) {
    case types::iot_dl_information_trans:
      HANDLE_CODE(c.get<iot_dl_information_trans_s>().pack(bref)); 
      break;
    case types::iot_rrc_reconf:
      HANDLE_CODE(c.get<iot_rrc_reconf_s>().pack(bref)); 
      break;
    case types::iot_rrc_release:
      HANDLE_CODE(c.get<iot_rrc_release_s>().pack(bref)); 
      break;
    default:
      log_invalid_choice_id(type_, "iot_dl_dcch_msg_type_c");
      return SRSASN_ERROR_ENCODE_FAIL; 
  } 
  return SRSASN_SUCCESS; 
}
SRSASN_CODE iot_dl_dcch_msg_type_c::unpack(cbit_ref& bref)
{
  types e; 
  e.unpack(bref); 
  set(e); 
  switch (type_) { 
    case types::iot_dl_information_trans: 
      HANDLE_CODE(c.get<iot_dl_information_trans_s>().unpack(bref)); 
      break;
    case types::iot_rrc_reconf: 
      HANDLE_CODE(c.get<iot_rrc_reconf_s>().unpack(bref)); 
      break;
    case types::iot_rrc_release: 
      HANDLE_CODE(c.get<iot_rrc_release_s>().unpack(bref)); 
      break;
    default:
      log_invalid_choice_id(type_, "iot_dl_dcch_msg_type_c"); 
      return SRSASN_ERROR_DECODE_FAIL; 
  }
  return SRSASN_SUCCESS; 
}
const char* iot_dl_dcch_msg_type_c::types_opts::to_string() const
{
  static const char* options[] = {"ioTDLInformationTransfer", "ioTRRCReconfiguration", "ioTRRCRelease"};
  return convert_enum_idx(options, 3, value, "iot_dl_dcch_msg_type_c::types");
}
// DL-DCCH-Message::= SEQUENCE
SRSASN_CODE iot_dl_dcch_msg_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(msg.pack(bref)); 
}
SRSASN_CODE iot_dl_dcch_msg_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(msg.unpack(bref)); 
}
void iot_dl_dcch_msg_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("DL-DCCH-MessageType");
  msg.to_json(j); 
  j.end_obj();
}