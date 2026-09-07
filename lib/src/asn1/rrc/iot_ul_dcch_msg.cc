#include "srsran/asn1/asn1_utils.h"
#include "srsran/asn1/rrc/iot_ul_dcch_msg.h"
#include <sstream>
using namespace asn1;
using namespace asn1::rrc;

/*******************************************************************************
 *                                Struct Methods
 ******************************************************************************/
//IoTRRCReconfigurationComplete-r1-IEs::= SEQUENCE
SRSASN_CODE iot_rrc_reconf_complete_r1_IEs_s::pack(bit_ref& bref)const
{
  bref.pack(ext, 1);
  return SRSASN_SUCCESS;
}
SRSASN_CODE iot_rrc_reconf_complete_r1_IEs_s::unpack(cbit_ref& bref) 
{
  bref.unpack(ext, 1);
  return SRSASN_SUCCESS;
}
void iot_rrc_reconf_complete_r1_IEs_s::to_json(json_writer& j)const 
{
  j.start_obj();
  j.end_obj();
}
    // IoTRRCReconfigurationComplete::= SEQUENCE
SRSASN_CODE iot_rrc_reconf_complete_s::pack(bit_ref& bref)const {
  HANDLE_CODE(rrc_transaction_id.pack(bref));
  HANDLE_CODE(iot_rrc_reconf_complete_r1_IEs.pack(bref));
  return SRSASN_SUCCESS;

}
SRSASN_CODE iot_rrc_reconf_complete_s::unpack(cbit_ref& bref) 
{
  HANDLE_CODE(rrc_transaction_id.unpack(bref));
  HANDLE_CODE(iot_rrc_reconf_complete_r1_IEs.unpack(bref));
  return SRSASN_SUCCESS;
}
void iot_rrc_reconf_complete_s::to_json(json_writer& j)const {
  j.start_obj();
  j.write_fieldname("rrc_transaction_id");
  rrc_transaction_id.to_json(j);
  j.write_fieldname("iot_rrc_reconf_complete_r1_IEs");
  iot_rrc_reconf_complete_r1_IEs.to_json(j);
  j.end_obj();
}

///IoTRRCReestablishmentComplete-r1-IEs::= SEQUENCE
SRSASN_CODE iot_rrc_reest_complete_r1_IEs_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  return SRSASN_SUCCESS;
}
SRSASN_CODE iot_rrc_reest_complete_r1_IEs_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  return SRSASN_SUCCESS;
}
void iot_rrc_reest_complete_r1_IEs_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.end_obj();
}
// IoTRRCReestablishmentComplete	::= SEQUENCE
SRSASN_CODE iot_rrc_reest_complete_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(rrc_transaction_id.pack(bref));
  HANDLE_CODE(iot_rrc_reest_complete_r1_IEs.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE iot_rrc_reest_complete_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(rrc_transaction_id.unpack(bref));
  HANDLE_CODE(iot_rrc_reest_complete_r1_IEs.unpack(bref));
  return SRSASN_SUCCESS;
}
void iot_rrc_reest_complete_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("rrc_transaction_id");
  rrc_transaction_id.to_json(j);
  j.write_fieldname("iot_rrc_reest_complete_r1_IEs");
  iot_rrc_reest_complete_r1_IEs.to_json(j);
  j.end_obj();
}




//// IoTRRCSetupComplete-r1-IEs::= SEQUENCE
SRSASN_CODE iot_rrc_setup_complete_r1_IEs_s::pack(bit_ref& bref) const
{
    HANDLE_CODE(bref.pack(registered_amf_present,1));
    HANDLE_CODE(pack_integer(bref, select_plmn_id, (uint8_t)1u, (uint8_t)4u));
    if (registered_amf_present) {
      HANDLE_CODE(registered_amf.pack(bref));
    }
    HANDLE_CODE(ded_info_nas_msg.pack(bref));

      return SRSASN_SUCCESS;
}
SRSASN_CODE iot_rrc_setup_complete_r1_IEs_s::unpack(cbit_ref& bref) 
{
      HANDLE_CODE(bref.unpack(registered_amf_present, 1));
      HANDLE_CODE(unpack_integer(select_plmn_id, bref, (uint8_t)1u, (uint8_t)4u));
      if (registered_amf_present) {
      HANDLE_CODE(registered_amf.unpack(bref));
      }
      HANDLE_CODE(ded_info_nas_msg.unpack(bref));

      return SRSASN_SUCCESS;
}
void iot_rrc_setup_complete_r1_IEs_s::to_json(json_writer& j)const
{
      j.start_obj();
      j.write_int("select_plmn_id", select_plmn_id);
      if (registered_amf_present) {
      j.write_fieldname("registered_amf");
      registered_amf.to_json(j);
      }
      j.write_fieldname("ded_info_nas_msg");
      ded_info_nas_msg.to_json(j);
      j.end_obj();
}

    // IoTRRCSetupComplete::= SEQUENCE
SRSASN_CODE iot_rrc_setup_complete_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(rrc_transaction_id.pack(bref));
  HANDLE_CODE(iot_rrc_setup_complete_r1_IEs.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE iot_rrc_setup_complete_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(rrc_transaction_id.unpack(bref));
  HANDLE_CODE(iot_rrc_setup_complete_r1_IEs.unpack(bref));
  return SRSASN_SUCCESS;
}
void iot_rrc_setup_complete_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("rrc_transaction_id");
  rrc_transaction_id.to_json(j);
  j.write_fieldname("iot_rrc_setup_complete_r1_IEs");
  iot_rrc_setup_complete_r1_IEs.to_json(j);
  j.end_obj();
}


//DedicatedInfoNAS
SRSASN_CODE iot_ul_trans_r1_IEs_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(ded_info_nas.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE iot_ul_trans_r1_IEs_s::unpack(cbit_ref& bref) 
{
  HANDLE_CODE(ded_info_nas.unpack(bref));
  return SRSASN_SUCCESS;
}
void iot_ul_trans_r1_IEs_s::to_json(json_writer& j)const 
{
  j.start_obj();
  j.write_fieldname("ded_info_nas");
  ded_info_nas.to_json(j);
  j.end_obj();
}
    // IoTULInformationTransfer::=	SEQUENCE
SRSASN_CODE iot_ul_info_trans_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(iot_ul_trans_r1_IEs.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE iot_ul_info_trans_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(iot_ul_trans_r1_IEs.unpack(bref));
  return SRSASN_SUCCESS;
}
void iot_ul_info_trans_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("iot_ul_trans_r1_IEs");
  iot_ul_trans_r1_IEs.to_json(j);
  j.end_obj();
}

// // UEIdentity-Transfer ::=SEQUENCE
// SRSASN_CODE ue_id_trans_s::pack(bit_ref& bref) const
// {
//   HANDLE_CODE(ue_id_trans_r1.pack(bref));
//   return SRSASN_SUCCESS;
// }
// SRSASN_CODE ue_id_trans_s::unpack(cbit_ref& bref)
// {
//   HANDLE_CODE(ue_id_trans_r1.unpack(bref));
//   return SRSASN_SUCCESS;
// }
// void ue_id_trans_s::to_json(json_writer& j) const
// {
//   j.start_obj();
//   j.write_fieldname("ue_id_trans_r1");
//   ue_id_trans_r1.to_json(j);
//   j.end_obj();
// }
    // //UL-DCCH-MessageType::= CHIOCE
void iot_ul_dcch_msg_type_c::destroy_()
{
  switch (type_) {
    case types::iot_rrc_reconf_complete:
      c.destroy<iot_rrc_reconf_complete_s>();
      break;
    case types::iot_rrc_reest_complete:
      c.destroy<iot_rrc_reest_complete_s>();
      break;
    case types::iot_rrc_setup_complete:
      c.destroy<iot_rrc_setup_complete_s>();
          break;
    case types::iot_ul_info_trans:
          c.destroy<iot_ul_info_trans_s>();
          break;
    case types::ue_id_trans:
          c.destroy<ue_id_trans_s>();
          break;
    default:
      break;
  }
}
void iot_ul_dcch_msg_type_c::set(types::options e)
{
  destroy_();
  type_ = e;
  switch (type_) {
    case types::iot_rrc_reconf_complete:
      c.init<iot_rrc_reconf_complete_s>();
      break;
    case types::iot_rrc_reest_complete:
      c.init<iot_rrc_reest_complete_s>();
      break;
    case types::iot_rrc_setup_complete:
      c.init<iot_rrc_setup_complete_s>();
      break;
    case types::iot_ul_info_trans:
      c.init<iot_ul_info_trans_s>();
      break;
    case types::ue_id_trans:
      c.init<ue_id_trans_s>();
      break;
   
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "iot_ul_dcch_msg_type_c");
  }
}
iot_ul_dcch_msg_type_c::iot_ul_dcch_msg_type_c(const iot_ul_dcch_msg_type_c& other)
{
  type_ = other.type();
  switch (type_) {
    case types::iot_rrc_reconf_complete:
      c.init(other.c.get<iot_rrc_reconf_complete_s>());
      break;
    case types::iot_rrc_reest_complete:
      c.init(other.c.get<iot_rrc_reest_complete_s>());
      break;
    case types::iot_rrc_setup_complete:
      c.init(other.c.get<iot_rrc_setup_complete_s>());
      break;
    case types::iot_ul_info_trans:
      c.init(other.c.get<iot_ul_info_trans_s>());
      break;
    case types::ue_id_trans:
      c.init(other.c.get<ue_id_trans_s>());
      break;
    
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "iot_ul_dcch_msg_type_c");
  }
}
iot_ul_dcch_msg_type_c& iot_ul_dcch_msg_type_c::operator=(const iot_ul_dcch_msg_type_c& other)
{
  if (this == &other) {
    return *this;
  }
  set(other.type());
  switch (type_) {
    case types::iot_rrc_reconf_complete:
      c.set(other.c.get<iot_rrc_reconf_complete_s>());
      break;
    case types::iot_rrc_reest_complete:
      c.set(other.c.get<iot_rrc_reest_complete_s>());
      break;
    case types::iot_rrc_setup_complete:
      c.set(other.c.get<iot_rrc_setup_complete_s>());
      break;
    case types::iot_ul_info_trans:
      c.set(other.c.get<iot_ul_info_trans_s>());
      break;
    case types::ue_id_trans:
      c.set(other.c.get<ue_id_trans_s>());
      break;
   
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "iot_ul_dcch_msg_type_c");
  }

  return *this;
}

iot_rrc_reconf_complete_s& iot_ul_dcch_msg_type_c::set_iot_rrc_reconf_complete()
{
  set(types::iot_rrc_reconf_complete);
  return c.get<iot_rrc_reconf_complete_s>();
}
iot_rrc_reest_complete_s& iot_ul_dcch_msg_type_c::set_iot_rrc_reest_complete()
{
  set(types::iot_rrc_reest_complete);
  return c.get<iot_rrc_reest_complete_s>();
}
iot_rrc_setup_complete_s& iot_ul_dcch_msg_type_c::set_iot_rrc_setup_complete()
{
  set(types::iot_rrc_setup_complete);
  return c.get<iot_rrc_setup_complete_s>();
}
iot_ul_info_trans_s& iot_ul_dcch_msg_type_c::set_iot_ul_info_trans()
{
  set(types::iot_ul_info_trans);
  return c.get<iot_ul_info_trans_s>();
}
ue_id_trans_s& iot_ul_dcch_msg_type_c::set_ue_id_trans()
{
  set(types::ue_id_trans);
  return c.get<ue_id_trans_s>();
}

void iot_ul_dcch_msg_type_c::to_json(json_writer& j) const
{
  j.start_obj();
  switch (type_) {
    case types::iot_rrc_reconf_complete:
      j.write_fieldname("iot_rrc_reconf_complete");
      c.get<iot_rrc_reconf_complete_s>().to_json(j);
      break;
    case types::iot_rrc_reest_complete:
      j.write_fieldname("iot_rrc_reest_complete");
      c.get<iot_rrc_reest_complete_s>().to_json(j);
      break;
    case types::iot_rrc_setup_complete:
      j.write_fieldname("iot_rrc_setup_complete");
      c.get<iot_rrc_setup_complete_s>().to_json(j);
      break;
    case types::iot_ul_info_trans:
      j.write_fieldname("iot_ul_info_trans");
      c.get<iot_ul_info_trans_s>().to_json(j);
      break;
    case types::ue_id_trans:
      j.write_fieldname("ue_id_trans");
      c.get<ue_id_trans_s>().to_json(j);
      break;
   
    default:
      log_invalid_choice_id(type_, "iot_ul_dcch_msg_type_c");
  }
  j.end_obj();
}
SRSASN_CODE iot_ul_dcch_msg_type_c::pack(bit_ref& bref) const
{
  type_.pack(bref);
  switch (type_) {
    case types::iot_rrc_reconf_complete:
      HANDLE_CODE(c.get<iot_rrc_reconf_complete_s>().pack(bref));
      break;
    case types::iot_rrc_reest_complete:
      HANDLE_CODE(c.get<iot_rrc_reest_complete_s>().pack(bref));
      break;
    case types::iot_rrc_setup_complete:
      HANDLE_CODE(c.get<iot_rrc_setup_complete_s>().pack(bref));
      break;
    case types::iot_ul_info_trans:
      HANDLE_CODE(c.get<iot_ul_info_trans_s>().pack(bref));
      break;
    case types::ue_id_trans:
      HANDLE_CODE(c.get<ue_id_trans_s>().pack(bref));
      break;
   
    default:
      log_invalid_choice_id(type_, "iot_ul_dcch_msg_type_c");
      return SRSASN_ERROR_ENCODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE iot_ul_dcch_msg_type_c::unpack(cbit_ref& bref)
{
  types e;
  e.unpack(bref);
  set(e);
  switch (type_) {
    case types::iot_rrc_reconf_complete:
      HANDLE_CODE(c.get<iot_rrc_reconf_complete_s>().unpack(bref));
      break;
    case types::iot_rrc_reest_complete:
      HANDLE_CODE(c.get<iot_rrc_reest_complete_s>().unpack(bref));
      break;
    case types::iot_rrc_setup_complete:
      HANDLE_CODE(c.get<iot_rrc_setup_complete_s>().unpack(bref));
      break;
    case types::iot_ul_info_trans:
      HANDLE_CODE(c.get<iot_ul_info_trans_s>().unpack(bref));
      break;
    case types::ue_id_trans:
      HANDLE_CODE(c.get<ue_id_trans_s>().unpack(bref));
      break;
  
    default:
      log_invalid_choice_id(type_, "iot_ul_dcch_msg_type_c");
      return SRSASN_ERROR_DECODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
const char* iot_ul_dcch_msg_type_c::types_opts::to_string() const
{
  static const char* options[] = {"iot_rrc_reconf_complete",
                                  "iot_rrc_reest_complete", "iot_rrc_setup_complete",
                                  "iot_ul_info_trans", "ue_id_trans"};
  return convert_enum_idx(options, 5, value, "iot_ul_dcch_msg_type_c");
}


// UL-DCCH-Message::= SEQUENCE
SRSASN_CODE iot_ul_dcch_msg_s::pack(bit_ref& bref)const 
{
  HANDLE_CODE(msg.pack(bref));
  return SRSASN_SUCCESS;

}
SRSASN_CODE iot_ul_dcch_msg_s::unpack(cbit_ref& bref) 
{
  HANDLE_CODE(msg.unpack(bref));
  return SRSASN_SUCCESS;
}
void iot_ul_dcch_msg_s::to_json(json_writer& j)const
{
  j.start_obj();
  j.write_fieldname("msg");
  msg.to_json(j);
  j.end_obj();
}
