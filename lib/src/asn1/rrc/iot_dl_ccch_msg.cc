#include "srsran/asn1/rrc/iot_dl_ccch_msg.h"
#include <sstream>

using namespace asn1;
using namespace asn1::rrc;

//IoTT-Reordering::= ENUMERATED
const char* iot_t_reordering_opts::to_string() const
{
	static const char* options[] = { "ms100", "ms500", "ms1000", "ms2000" };
	return convert_enum_idx(options, 4, value, "iot_t_reordering_e");
}
uint8_t iot_t_reordering_opts::to_number() const
{
	static const uint8_t options[] = { 1, 2, 3, 4};
	return map_enum_number(options, 4, value, "iot_t_reordering_e");
}



//IoTRRCSetup-r1-IEs::= SEQUENCE
SRSASN_CODE iot_rrc_setup_r1_IEs_s::pack(bit_ref& bref) const
{
	bref.pack(ext, 1);
	HANDLE_CODE(iot_t_reorder.pack(bref));
        return SRSASN_SUCCESS;
}
SRSASN_CODE iot_rrc_setup_r1_IEs_s::unpack(cbit_ref& bref)
{
	bref.unpack(ext, 1);
	HANDLE_CODE(iot_t_reorder.unpack(bref));
        return SRSASN_SUCCESS;
}
void  iot_rrc_setup_r1_IEs_s::to_json(json_writer& j) const
{
	j.start_obj();
	j.write_str("ioTT-Reordering", iot_t_reorder.to_string());
	j.end_obj();
}



const char* iot_rrc_reject_r1_IEs_s::geo_accept_opts::to_string() const
{
	static const char* options[] = { "True" };
	return convert_enum_idx(options, 1, value, "iot_rrc_reject_r1_IEs_s::geo_accept_e");
}
//IoTRRCReject-r1-IEs::= SEQUENCE
SRSASN_CODE iot_rrc_reject_r1_IEs_s::pack(bit_ref& bref) const
{
	bref.pack(ext, 1);
	HANDLE_CODE(bref.pack(redierection_info_present, 1));
	HANDLE_CODE(bref.pack(geo_accept_present, 1));
	if (redierection_info_present)
	{
		HANDLE_CODE(redierection_info.pack(bref));
	}
	if(geo_accept_present)
	{
		HANDLE_CODE(geo_accept.pack(bref));
	}
        return SRSASN_SUCCESS;
}
SRSASN_CODE iot_rrc_reject_r1_IEs_s::unpack(cbit_ref& bref) 
{
	bref.unpack(ext, 1);
	HANDLE_CODE(bref.unpack(redierection_info_present, 1));
	HANDLE_CODE(bref.unpack(geo_accept_present, 1));
	if (redierection_info_present)
	{
		HANDLE_CODE(redierection_info.unpack(bref));
	}
	if (geo_accept_present)
	{
		HANDLE_CODE(geo_accept.unpack(bref));
	}
        return SRSASN_SUCCESS;
}
void       iot_rrc_reject_r1_IEs_s::to_json(json_writer& j) const
{
	j.start_obj();
	if (redierection_info_present)
	{
		j.write_fieldname("redirectionInfo");
		redierection_info.to_json(j);
	}
	if (geo_accept_present)
	{
		j.write_str("geoAccept", geo_accept.to_string());
	}
	j.end_obj();
}
const char* iot_rrc_resst_r1_s::rcv_um_rlc_entity_opts::to_string() const
{
	static const char* options[] = { "recover","reestablish" };
	return convert_enum_idx(options, 2, value, "iot_rrc_resst_r1_s::rcv_um_rlc_entity_e");
}
const char* iot_rrc_resst_r1_s::trans_um_rlc_entity_opts::to_string() const
{
	static const char* options[] = { "recover","reestablish" };
	return convert_enum_idx(options, 2, value, "iot_rrc_resst_r1_s::trans_um_rlc_entity_e");
}

//IoTRRCReestablishment-r1::= SEQUENCE
SRSASN_CODE iot_rrc_resst_r1_s::pack(bit_ref& bref) const
{
	bref.pack(ext, 1);
	HANDLE_CODE(iot_t_reorder.pack(bref));
	HANDLE_CODE(rcv_um_rlc_entity.pack(bref));
	HANDLE_CODE(trans_um_rlc_entity.pack(bref));
	HANDLE_CODE(dl_nas_mac.pack(bref));
        return SRSASN_SUCCESS;
}
SRSASN_CODE iot_rrc_resst_r1_s::unpack(cbit_ref& bref) 
{
	bref.unpack(ext, 1);
	HANDLE_CODE(iot_t_reorder.unpack(bref));
	HANDLE_CODE(rcv_um_rlc_entity.unpack(bref));
	HANDLE_CODE(trans_um_rlc_entity.unpack(bref));
	HANDLE_CODE(dl_nas_mac.unpack(bref));
        return SRSASN_SUCCESS;
}
void      iot_rrc_resst_r1_s::to_json(json_writer& j) const
{
	j.start_obj();
	j.write_str("ioTT-Reordering", iot_t_reorder.to_string());
	j.write_str("receivingUmRlcEntity", rcv_um_rlc_entity.to_string());
	j.write_str("transmitingUmRlcEntity", trans_um_rlc_entity.to_string());
	j.write_str("dlNasMac", dl_nas_mac.to_string());
	j.end_obj();
}
//IoTRRCResstablishment::= SEQUENCE
SRSASN_CODE iot_rrc_resst_s::pack(bit_ref& bref) const
{
	HANDLE_CODE(rrc_transaction_id.pack(bref));
	HANDLE_CODE(iot_rrc_resst_r1.pack(bref));
        return SRSASN_SUCCESS;
}
SRSASN_CODE iot_rrc_resst_s::unpack(cbit_ref& bref) 
{
	HANDLE_CODE(rrc_transaction_id.unpack(bref));
	HANDLE_CODE(iot_rrc_resst_r1.unpack(bref));
        return SRSASN_SUCCESS;
}
void       iot_rrc_resst_s::to_json(json_writer& j) const
{
	j.start_obj();
	j.write_fieldname("rrc-TransactionIdentifir");
	rrc_transaction_id.to_json(j);
	j.write_fieldname("ioTRRCReestablishment-r1");
	iot_rrc_resst_r1.to_json(j);
	j.end_obj();
}
//oTRRCReestablishmentReject::=SEQUENCE
 SRSASN_CODE iot_rrc_reest_reject_r1_ie_s::pack(bit_ref& bref) const
 {
	bref.pack(ext, 1);
        return SRSASN_SUCCESS;
 }
 SRSASN_CODE iot_rrc_reest_reject_r1_ie_s::unpack(cbit_ref& bref) 
 {
	bref.unpack(ext, 1);
        return SRSASN_SUCCESS;
 }
void   iot_rrc_reest_reject_r1_ie_s::to_json(json_writer& j) const
{
	j.start_obj();
	j.end_obj();
}
// IoTRRCResstablishmentReject::= SEQUENCE
SRSASN_CODE iot_rrc_reest_reject_s::pack(bit_ref& bref) const
 {
	HANDLE_CODE(iot_rrc_reest_reject_r1_ies.pack(bref));
        return SRSASN_SUCCESS;
 }
 SRSASN_CODE iot_rrc_reest_reject_s::unpack(cbit_ref& bref) 
 {
	HANDLE_CODE(iot_rrc_reest_reject_r1_ies.unpack(bref));
        return SRSASN_SUCCESS;
 }
void   iot_rrc_reest_reject_s::to_json(json_writer& j) const
{
	j.start_obj();
	j.write_fieldname("IoTRRCReestablishmentReject-r1-IEs");
	iot_rrc_reest_reject_r1_ies.to_json(j);
	j.end_obj();
}
//IoTRRCReject::= SEQUENCE
SRSASN_CODE iot_rrc_reject_s::pack(bit_ref& bref) const
 {
	HANDLE_CODE(iot_rrc_reject_r1.pack(bref));
        return SRSASN_SUCCESS;
 }
 SRSASN_CODE iot_rrc_reject_s::unpack(cbit_ref& bref) 
 {
	HANDLE_CODE(iot_rrc_reject_r1.unpack(bref));
        return SRSASN_SUCCESS;
 }
void  iot_rrc_reject_s::to_json(json_writer& j) const
{
	j.start_obj();
	j.write_fieldname("ioTRRCReject-r1");
	iot_rrc_reject_r1.to_json(j);
	j.end_obj();
}
//IoTRRCSetup::= SEQUENCE
SRSASN_CODE iot_rrc_setup_s::pack(bit_ref& bref) const
{
	HANDLE_CODE(rrc_transaction_id.pack(bref));
	HANDLE_CODE(iot_rrc_setup_r1.pack(bref));
        return SRSASN_SUCCESS;
}
SRSASN_CODE iot_rrc_setup_s::unpack(cbit_ref& bref) 
{
	HANDLE_CODE(rrc_transaction_id.unpack(bref));
	HANDLE_CODE(iot_rrc_setup_r1.unpack(bref));
        return SRSASN_SUCCESS;
}
void       iot_rrc_setup_s::to_json(json_writer& j) const
{
	j.start_obj();
	j.write_fieldname("rrc-TransactionIdentifier");
	rrc_transaction_id.to_json(j);
	j.write_fieldname("ioTRRCSetup-r1");
	iot_rrc_setup_r1.to_json(j);
	j.end_obj();
}

//DL-CCCH-Message::= CHIOCE
const char* iot_dl_ccch_msg_type_c::types_opts::to_string() const
{
	static const char* options[] = { "iot_rrc_resst","iot_rrc_reest_reject" ,
                                            "iot_rrc_reject","iot_rrc_setup"};
	return convert_enum_idx(options, 4, value, "iot_dl_ccch_msg_type_c::types_opts");
}
void iot_dl_ccch_msg_type_c::destroy_()
{
  switch (type_) {
    case types::iot_rrc_resst:
      c.destroy<iot_rrc_resst_s>();
      break;
    case types::iot_rrc_reest_reject:
      c.destroy<iot_rrc_reest_reject_s>();
      break;
    case types::iot_rrc_reject:
      c.destroy<iot_rrc_reject_s>();
      break;
    case types::iot_rrc_setup:
      c.destroy<iot_rrc_setup_s>();
      break;
    default:
      break;
  }
}
void iot_dl_ccch_msg_type_c::set(types::options e)
{
  destroy_();
  type_ = e;
  switch (type_) {
    case types::iot_rrc_resst:
      c.init<iot_rrc_resst_s>();
      break;
    case types::iot_rrc_reest_reject:
      c.init<iot_rrc_reest_reject_s>();
      break;
    case types::iot_rrc_reject:
      c.init<iot_rrc_reject_s>();
      break;
    case types::iot_rrc_setup:
      c.init<iot_rrc_setup_s>();
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "iot_dl_ccch_msg_type_c");
  }
}
iot_dl_ccch_msg_type_c::iot_dl_ccch_msg_type_c(const iot_dl_ccch_msg_type_c& other)
{
  type_ = other.type();
  switch (type_) {
    case types::iot_rrc_resst:
      c.init(other.c.get<iot_rrc_resst_s>());
      break;
    case types::iot_rrc_reest_reject:
      c.init(other.c.get<iot_rrc_reest_reject_s>());
      break;
    case types::iot_rrc_reject:
      c.init(other.c.get<iot_rrc_reject_s>());
      break;
    case types::iot_rrc_setup:
      c.init(other.c.get<iot_rrc_setup_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "iot_dl_ccch_msg_type_c");
  }
}
iot_dl_ccch_msg_type_c&iot_dl_ccch_msg_type_c::operator=(const iot_dl_ccch_msg_type_c& other)
{
  if (this == &other) {
    return *this;
  }
  set(other.type());
  switch (type_) {
    case types::iot_rrc_resst:
      c.set(other.c.get<iot_rrc_resst_s>());
      break;
    case types::iot_rrc_reest_reject:
      c.set(other.c.get<iot_rrc_reest_reject_s>());
      break;
    case types::iot_rrc_reject:
      c.set(other.c.get<iot_rrc_reject_s>());
      break;
    case types::iot_rrc_setup:
      c.set(other.c.get<iot_rrc_setup_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "iot_dl_ccch_msg_type_c");
  }

  return *this;
}
iot_rrc_resst_s& iot_dl_ccch_msg_type_c::set_iot_rrc_resst()
{
  set(types::iot_rrc_resst);
  return c.get<iot_rrc_resst_s>();
}
iot_rrc_reest_reject_s& iot_dl_ccch_msg_type_c::set_iot_rrc_reest_reject()
{
  set(types::iot_rrc_reest_reject);
  return c.get<iot_rrc_reest_reject_s>();
}
iot_rrc_reject_s& iot_dl_ccch_msg_type_c::set_iot_rrc_reject()
{
  set(types::iot_rrc_reject);
  return c.get<iot_rrc_reject_s>();
}
iot_rrc_setup_s& iot_dl_ccch_msg_type_c::set_iot_rrc_setup()
{
  set(types::iot_rrc_setup);
  return c.get<iot_rrc_setup_s>();
}
void iot_dl_ccch_msg_type_c::to_json(json_writer& j) const
{
  j.start_obj();
  switch (type_) {
    case types::iot_rrc_resst:
      j.write_fieldname("ioTRRCResstablishment");
      c.get<iot_rrc_resst_s>().to_json(j);
      break;
    case types::iot_rrc_reest_reject:
      j.write_fieldname("ioTRRCResstablishmentReject");
      c.get<iot_rrc_reest_reject_s>().to_json(j);
      break;
    case types::iot_rrc_reject:
      j.write_fieldname("ioTRRCReject");
      c.get<iot_rrc_reject_s>().to_json(j);
      break;
    case types::iot_rrc_setup:
      j.write_fieldname("ioTRRCSetup");
      c.get<iot_rrc_setup_s>().to_json(j);
      break;
      j.end_array();
      break;
    default:
      log_invalid_choice_id(type_, "iot_dl_ccch_msg_type_c");
  }
  j.end_obj();
}
SRSASN_CODE iot_dl_ccch_msg_type_c::pack(bit_ref& bref) const
{
    type_.pack(bref);
    switch (type_) {
    case types::iot_rrc_resst:
        HANDLE_CODE(c.get<iot_rrc_resst_s>().pack(bref));
        break;
    case types::iot_rrc_reest_reject:
        HANDLE_CODE(c.get<iot_rrc_reest_reject_s>().pack(bref));
        break;
    case types::iot_rrc_reject:
        HANDLE_CODE(c.get<iot_rrc_reject_s>().pack(bref));
        break;
    case types::iot_rrc_setup:
        HANDLE_CODE(c.get<iot_rrc_setup_s>().pack(bref));
        break;
    default:
        log_invalid_choice_id(type_, "iot_dl_ccch_msg_type_c");
        return SRSASN_ERROR_ENCODE_FAIL;
    }
    return SRSASN_SUCCESS;
}
SRSASN_CODE iot_dl_ccch_msg_type_c::unpack(cbit_ref& bref)
{
    types e;
    e.unpack(bref);
    set(e);
    switch (type_) {
    case types::iot_rrc_resst:
        HANDLE_CODE(c.get<iot_rrc_resst_s>().unpack(bref));
        break;
    case types::iot_rrc_reest_reject:
        HANDLE_CODE(c.get<iot_rrc_reest_reject_s>().unpack(bref));
        break;
    case types::iot_rrc_reject:
        HANDLE_CODE(c.get<iot_rrc_reject_s>().unpack(bref));
        break;
    case types::iot_rrc_setup:
        HANDLE_CODE(c.get<iot_rrc_setup_s>().unpack(bref));
        break;
    default:
        log_invalid_choice_id(type_, "iot_dl_ccch_msg_type_c");
        return SRSASN_ERROR_ENCODE_FAIL;
    }
    return SRSASN_SUCCESS;
}
//DL-CCCH-Message::= SEQUENCE
SRSASN_CODE iot_dl_ccch_msg_s::pack(bit_ref& bref) const
{
	HANDLE_CODE(msg.pack(bref));
    return SRSASN_SUCCESS;
}
SRSASN_CODE iot_dl_ccch_msg_s::unpack(cbit_ref& bref)
{
	HANDLE_CODE(msg.unpack(bref));
    return SRSASN_SUCCESS;
}
void iot_dl_ccch_msg_s::to_json(json_writer& j) const
{
	j.start_obj();
	j.write_fieldname("message");
	msg.to_json(j);
	j.end_obj();
}