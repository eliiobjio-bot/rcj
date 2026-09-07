/*******************************************************************************
 *
 *                  IoT UL-CCCH Ghannel Information (2023.09.06)
 *
 ******************************************************************************/

#include "srsran/asn1/asn1_utils.h"
#include "srsran/asn1/rrc/iot_ul_ccch_msg.h"
#include <sstream>

using namespace asn1;
using namespace asn1::rrc;

/*******************************************************************************
 *                                Struct Methods
 ******************************************************************************/
// UE-Identity::= CHOICE
void ue_id_c::destroy_()
{
  switch (type_) 
  {
    case types::iot_5g_s_tmsi:
      c.destroy<iot_nr_s_tmsi_s>();
      break;
    case types::random_value:
      c.destroy<fixed_bitstring<48> >();
      break;
    default:
      break;
  }
}
void ue_id_c::set(types::options e)
{
  destroy_();
  type_ = e;
  switch (type_)
  {
    case types::iot_5g_s_tmsi:
      c.init<iot_nr_s_tmsi_s>();
      break;
    case types::random_value:
      c.init<fixed_bitstring<48> >();
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "ue_id_c");
  }
}
ue_id_c::ue_id_c(const ue_id_c& other)
{
  type_ = other.type();
  switch (type_) 
  {
    case types::iot_5g_s_tmsi:
      c.init(other.c.get<iot_nr_s_tmsi_s>());
      break;
    case types::random_value:
      c.init(other.c.get<fixed_bitstring<48> >());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "ue_id_c");
  }
}
ue_id_c& ue_id_c::operator=(const ue_id_c& other)
{
  if (this == &other) {
    return *this;
  }
  set(other.type());
  switch (type_) 
  {
    case types::iot_5g_s_tmsi:
      c.set(other.c.get<iot_nr_s_tmsi_s>());
      break;
    case types::random_value:
      c.set(other.c.get<fixed_bitstring<48> >());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "ue_id_c");
  }
  return *this;
}

iot_nr_s_tmsi_s& ue_id_c::set_iot_5g_s_tmsi()
{
  set(types::iot_5g_s_tmsi);
  return c.get<iot_nr_s_tmsi_s>();
}
fixed_bitstring<48>& ue_id_c::set_random_value()
{
  set(types::random_value);
  return c.get<fixed_bitstring<48> >();
}

void ue_id_c::to_json(json_writer& j) const
{
  j.start_obj();
  switch (type_) 
  {
    case types::iot_5g_s_tmsi:
      j.write_fieldname("ioT-5G-S-TMSI");
      c.get<iot_nr_s_tmsi_s>().to_json(j);
      break;
    case types::random_value:
      j.write_str("randomValue", c.get<fixed_bitstring<48> >().to_string());
      break;
    default:
      log_invalid_choice_id(type_, "ue_id_c");
  }
  j.end_obj();
}
SRSASN_CODE ue_id_c::pack(bit_ref& bref) const
{
  type_.pack(bref);
  switch (type_) 
  {
    case types::iot_5g_s_tmsi:
      HANDLE_CODE(c.get<iot_nr_s_tmsi_s>().pack(bref));
      break;
    case types::random_value:
      HANDLE_CODE(c.get<fixed_bitstring<48> >().pack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "ue_id_c");
      return SRSASN_ERROR_ENCODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE ue_id_c::unpack(cbit_ref& bref)
{
  types e;
  e.unpack(bref);
  set(e);
  switch (type_) 
  {
    case types::iot_5g_s_tmsi:
      HANDLE_CODE(c.get<iot_nr_s_tmsi_s>().unpack(bref));
      break;
    case types::random_value:
      HANDLE_CODE(c.get<fixed_bitstring<48> >().unpack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "ue_id_c");
      return SRSASN_ERROR_ENCODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
const char* ue_id_c::types_opts::to_string() const
{
  static const char* options[] = {"iot_5g_s_tmsi", "random_value"};
  return convert_enum_idx(options, 2, value, "ue_id_c::types");
}

// IoTEstabilshmentCause::= ENUMERATED
const char* iot_estableishment_cause_opts::to_string() const
{
  static const char* options[] = { "highPriorityAccess", "mt-Access", "mo-Signalling", "mo-Data", "mcs-PriorityAccess", "spare2", "spare1"};
  return convert_enum_idx(options, 7, value, "iot_estableishment_cause_e");
}

// 5G-S-TMSI ::=SEQUENCE
SRSASN_CODE iot_nr_s_tmsi_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(iot_5g_s_tmsi.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE iot_nr_s_tmsi_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(iot_5g_s_tmsi.unpack(bref));
  return SRSASN_SUCCESS;
}
void iot_nr_s_tmsi_s::to_json(json_writer& j) const
{
  j.start_array();
  j.write_str("IOT-5G-S-TMSI", iot_5g_s_tmsi.to_string());
  j.end_array();
}

// IoTQosInfo::= SEQUENCE
SRSASN_CODE iot_qos_info_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(delay_butget.pack(bref));
  HANDLE_CODE(reliablity_budget.pack(bref));
  HANDLE_CODE(spare.pack(bref));

  return SRSASN_SUCCESS;
}
SRSASN_CODE iot_qos_info_s::unpack(cbit_ref& bref)
{
   HANDLE_CODE(delay_butget.unpack(bref));
   HANDLE_CODE(reliablity_budget.unpack(bref));
   HANDLE_CODE(spare.unpack(bref));

   return SRSASN_SUCCESS;
}
void iot_qos_info_s::to_json(json_writer& j) const
{
   j.start_obj();
   j.write_str("delayBudget", delay_butget.to_string());
   j.write_str("reliablityBudget", reliablity_budget.to_string());
   j.write_str("spare", spare.to_string());
   j.end_obj();
}
const char* iot_qos_info_s::delay_budget_opts::to_string() const
{
   static const char* options[] = {"critical", "spare2", "spare1", "tolerant"};
   return convert_enum_idx(options, 4, value, "iot_qos_info_s::delay_budget_e_");
}
const char* iot_qos_info_s::reliablity_budget_opts::to_string() const
{
   static const char* options[] = {"critical", "spare2", "spare1", "tolerant"};
   return convert_enum_idx(options, 4, value, "iot_qos_info_s::reliablity_budget_e_");
}

// IoTUeCapability::= SEQUENCE
SRSASN_CODE iot_ue_capability_s::pack(bit_ref& bref) const
{
   HANDLE_CODE(fdd_duplex_mod.pack(bref));
   HANDLE_CODE(hgnss_support.pack(bref));
   HANDLE_CODE(valid_ephemeris.pack(bref));
   HANDLE_CODE(process_duration_for_ul.pack(bref));
   HANDLE_CODE(process_duration_for_dl.pack(bref));
   HANDLE_CODE(spare.pack(bref));

   return SRSASN_SUCCESS;
}
SRSASN_CODE iot_ue_capability_s::unpack(cbit_ref& bref)
{
   HANDLE_CODE(fdd_duplex_mod.unpack(bref));
   HANDLE_CODE(hgnss_support.unpack(bref));
   HANDLE_CODE(valid_ephemeris.unpack(bref));
   HANDLE_CODE(process_duration_for_ul.unpack(bref));
   HANDLE_CODE(process_duration_for_dl.unpack(bref));
   HANDLE_CODE(spare.unpack(bref));

   return SRSASN_SUCCESS;
}
void iot_ue_capability_s::to_json(json_writer& j) const
{
   j.start_obj();
   j.write_str("fddDuplexMode", fdd_duplex_mod.to_string());
   j.write_str("hgnssSupport", hgnss_support.to_string());
   j.write_str("validEphemeris", valid_ephemeris.to_string());
   j.write_str("processDurationForUl", process_duration_for_ul.to_string());
   j.write_str("processDurationForDl", process_duration_for_dl.to_string());
   j.write_str("spare", spare.to_string());
   j.end_obj();
}
const char* iot_ue_capability_s::fdd_duplex_mode_opts::to_string() const
{
   static const char* options[] = {"full", "half"};
   return convert_enum_idx(options, 2, value, "iot_ue_capability_s::fdd_duplex_mode_e_");
}
const char* iot_ue_capability_s::hgnss_support_opts::to_string() const
{
   static const char* options[] = {"True", "False"};
   return convert_enum_idx(options, 2, value, "iot_ue_capability_s::hgnss_support_e_");
}
const char* iot_ue_capability_s::valid_ephemeris_opts::to_string() const
{
   static const char* options[] = {"True", "False"};
   return convert_enum_idx(options, 2, value, "iot_ue_capability_s::valid_ephemeris_e_");
}

// IoTRRCReestablishmentRequest-r1-IEs::= SEQUENCE
SRSASN_CODE iot_rrc_resst_req_r1_IEs_s::pack(bit_ref& bref) const
{
   bref.pack(ext, 1);
   HANDLE_CODE(reestab_ue_id.pack(bref));
   HANDLE_CODE(reestab_ue_id_l.pack(bref));
   HANDLE_CODE(ul_mas_mac.pack(bref));
   HANDLE_CODE(ul_nas_count.pack(bref));

   return SRSASN_SUCCESS;
}
SRSASN_CODE iot_rrc_resst_req_r1_IEs_s::unpack(cbit_ref& bref)
{
   bref.unpack(ext, 1);
   HANDLE_CODE(reestab_ue_id.unpack(bref));
   HANDLE_CODE(reestab_ue_id_l.unpack(bref));
   HANDLE_CODE(ul_mas_mac.unpack(bref));
   HANDLE_CODE(ul_nas_count.unpack(bref));

   return SRSASN_SUCCESS;
}
void iot_rrc_resst_req_r1_IEs_s::to_json(json_writer& j) const
{
   j.start_obj();
   j.write_fieldname("reestabUE-Identity");
   reestab_ue_id.to_json(j);

   j.write_fieldname("reestabUE-Identity");
   reestab_ue_id_l.to_json(j);

   j.write_str("ulNasMac", ul_mas_mac.to_string());
   j.write_str("ulNasCount", ul_nas_count.to_string());

   j.end_obj();
}

// IoTRRCSetupRequset-r1-IEs::= SEQUENCE
SRSASN_CODE iot_rrc_setup_req_r1_IEs_s::pack(bit_ref& bref) const
{
   bref.pack(ext, 1);
   HANDLE_CODE(bref.pack(iot_qos_info_present, 1));
   HANDLE_CODE(bref.pack(iot_ue_cap_present, 1));
   HANDLE_CODE(bref.pack(ue_geo_info_present, 1));
   HANDLE_CODE(bref.pack(geo_security_para_present, 1));

   HANDLE_CODE(ue_id.pack(bref));
   HANDLE_CODE(iot_estableishment_cause.pack(bref));

   if (iot_qos_info_present) 
   {
    HANDLE_CODE(iot_qos_info.pack(bref));
   }

   if (iot_ue_cap_present)
   {
    HANDLE_CODE(iot_ue_cap.pack(bref));
   }

   if (ue_geo_info_present) 
   {
    HANDLE_CODE(ue_geo_info.pack(bref));
   }

   if (geo_security_para_present) 
   {
    HANDLE_CODE(geo_security_para.pack(bref));
   }

   return SRSASN_SUCCESS;
}
SRSASN_CODE iot_rrc_setup_req_r1_IEs_s::unpack(cbit_ref& bref)
{
   bref.unpack(ext, 1);
   HANDLE_CODE(bref.unpack(iot_qos_info_present, 1));
   HANDLE_CODE(bref.unpack(iot_ue_cap_present, 1));
   HANDLE_CODE(bref.unpack(ue_geo_info_present, 1));
   HANDLE_CODE(bref.unpack(geo_security_para_present, 1));

   HANDLE_CODE(ue_id.unpack(bref));
   HANDLE_CODE(iot_estableishment_cause.unpack(bref));

   if (iot_qos_info_present) {
    HANDLE_CODE(iot_qos_info.unpack(bref));
   }

   if (iot_ue_cap_present) {
    HANDLE_CODE(iot_ue_cap.unpack(bref));
   }

   if (ue_geo_info_present) {
    HANDLE_CODE(ue_geo_info.unpack(bref));
   }

   if (geo_security_para_present) {
    HANDLE_CODE(geo_security_para.unpack(bref));
   }

   return SRSASN_SUCCESS;
}
void iot_rrc_setup_req_r1_IEs_s::to_json(json_writer& j) const
{
   j.start_obj();
   j.write_fieldname("ue-Identity");
   ue_id.to_json(j);

   j.write_str("ioTEstablishmentCause", iot_estableishment_cause.to_string());
   if (iot_qos_info_present) {
    j.write_fieldname("ioTQosInfo");
    iot_qos_info.to_json(j);
   }
 
   if (iot_ue_cap_present) {
    j.write_fieldname("ioTUeCapability");
    iot_ue_cap.to_json(j);
   }
   if (ue_geo_info_present) {
    j.write_fieldname("ueGeographicalInfo");
    ue_geo_info.to_json(j);
   }

   if (geo_security_para_present) {
    j.write_str("geOSecurityPara", geo_security_para.to_string());
    
   }

   j.end_obj();
}

// IoTRRCReestablishmentRequest::= SEQUENCE
SRSASN_CODE iot_rrc_resst_req_s::pack(bit_ref& bref) const
{
   HANDLE_CODE(iot_rrc_resst_req_r1.pack(bref));

   return SRSASN_SUCCESS;
}
SRSASN_CODE iot_rrc_resst_req_s::unpack(cbit_ref& bref)
{
   HANDLE_CODE(iot_rrc_resst_req_r1.unpack(bref));

   return SRSASN_SUCCESS;
}
void iot_rrc_resst_req_s::to_json(json_writer& j) const
{
   j.start_obj();
   j.write_fieldname("ioTRRCReestablishmentRequest-r1");
   iot_rrc_resst_req_r1.to_json(j);
   j.end_obj();
}

// IoTRRCSetupRequest::= SEQUENCE
SRSASN_CODE iot_rrc_setup_req_s::pack(bit_ref& bref) const
{
   HANDLE_CODE(iot_rrc_setup_req_r1.pack(bref));
   
   return SRSASN_SUCCESS;
}
SRSASN_CODE iot_rrc_setup_req_s::unpack(cbit_ref& bref)
{
   HANDLE_CODE(iot_rrc_setup_req_r1.unpack(bref));

   return SRSASN_SUCCESS;
}
void iot_rrc_setup_req_s::to_json(json_writer& j) const
{
   j.start_obj();
   j.write_fieldname("ioTRRCSetupRequset-r1");
   iot_rrc_setup_req_r1.to_json(j);
   j.end_obj();
}

// UL-CCCH-MessageType::= CHIOCE
void iot_ul_ccch_msg_type_c::destroy_()
{
   switch (type_) {
    case types::iot_rrc_resst_req:
      c.destroy<iot_rrc_resst_req_s>();
      break;
    case types::iot_rrc_setup_req:
      c.destroy<iot_rrc_setup_req_s>();
      break;
    default:
      break;
   }
}
void iot_ul_ccch_msg_type_c::set(types::options e)
{
   destroy_();
   type_ = e;
   switch (type_) {
    case types::iot_rrc_resst_req:
      c.init<iot_rrc_resst_req_s>();
      break;
    case types::iot_rrc_setup_req:
      c.init<iot_rrc_setup_req_s>();
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "iot_ul_ccch_msg_type_c");
   }
}
iot_ul_ccch_msg_type_c::iot_ul_ccch_msg_type_c(const iot_ul_ccch_msg_type_c& other)
{
   type_ = other.type();
   switch (type_) {
    case types::iot_rrc_resst_req:
      c.init(other.c.get<iot_rrc_resst_req_s>());
      break;
    case types::iot_rrc_setup_req:
      c.init(other.c.get<iot_rrc_setup_req_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "iot_ul_ccch_msg_type_c");
   }
}
iot_ul_ccch_msg_type_c& iot_ul_ccch_msg_type_c::operator=(const iot_ul_ccch_msg_type_c& other)
{
  if (this == &other) {
    return *this;
  }
  set(other.type());
  switch (type_) {
    case types::iot_rrc_resst_req:
      c.set(other.c.get<iot_rrc_resst_req_s>());
      break;
    case types::iot_rrc_setup_req:
      c.set(other.c.get<iot_rrc_setup_req_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "iot_ul_ccch_msg_type_c");
  }
  return *this;
}

iot_rrc_resst_req_s& iot_ul_ccch_msg_type_c::set_iot_rrc_resst_req()
{
  set(types::iot_rrc_resst_req);
  return c.get<iot_rrc_resst_req_s>();
}
iot_rrc_setup_req_s& iot_ul_ccch_msg_type_c::set_iot_rrc_setup_req()
{
  set(types::iot_rrc_setup_req);
  return c.get<iot_rrc_setup_req_s>();
}

void iot_ul_ccch_msg_type_c::to_json(json_writer& j) const
{
  j.start_obj();
  switch (type_) {
    case types::iot_rrc_resst_req:
      j.write_fieldname("ioTRRCReestablishmentRequest");
      c.get<iot_rrc_resst_req_s>().to_json(j);
      break;
    case types::iot_rrc_setup_req:
      j.write_fieldname("ioTRRCSetupRequest");
      c.get<iot_rrc_resst_req_s>().to_json(j);
      break;
    default:
      log_invalid_choice_id(type_, "iot_ul_ccch_msg_type_c");
  }
  j.end_obj();
}
SRSASN_CODE iot_ul_ccch_msg_type_c::pack(bit_ref& bref) const
{
  type_.pack(bref);
  switch (type_) {
    case types::iot_rrc_resst_req:
      HANDLE_CODE(c.get<iot_rrc_resst_req_s>().pack(bref));
      break;
    case types::iot_rrc_setup_req:
      HANDLE_CODE(c.get<iot_rrc_setup_req_s>().pack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "iot_ul_ccch_msg_type_c");
      return SRSASN_ERROR_ENCODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE iot_ul_ccch_msg_type_c::unpack(cbit_ref& bref)
{
  types e;
  e.unpack(bref);
  set(e);
  switch (type_) {
    case types::iot_rrc_resst_req:
      HANDLE_CODE(c.get<iot_rrc_resst_req_s>().unpack(bref));
      break;
    case types::iot_rrc_setup_req:
      HANDLE_CODE(c.get<iot_rrc_setup_req_s>().unpack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "iot_ul_ccch_msg_type_c");
      return SRSASN_ERROR_ENCODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
const char* iot_ul_ccch_msg_type_c::types_opts::to_string() const
{
  static const char* options[] = {"iot_rrc_resst_req", "iot_rrc_setup_req"};
  return convert_enum_idx(options, 2, value, "iot_ul_ccch_msg_type_c::types");
}

// UL-CCCH-Message::= SEQUENCE
SRSASN_CODE iot_ul_ccch_msg_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(msg.pack(bref));

  bref.align_bytes_zero();

  return SRSASN_SUCCESS;
}
SRSASN_CODE iot_ul_ccch_msg_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(msg.unpack(bref));

  bref.align_bytes();

  return SRSASN_SUCCESS;
}
void iot_ul_ccch_msg_s::to_json(json_writer& j) const
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