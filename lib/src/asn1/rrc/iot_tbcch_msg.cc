/*******************************************************************************
 *
 *                  IoT TBCCH Ghannel Information (2023.08.08)
 *
 ******************************************************************************/

#include "srsran/asn1/rrc/iot_tbcch_msg.h"
#include "srsran/asn1/asn1_utils.h"
#include <sstream>

using namespace asn1;
using namespace asn1::rrc;

/*******************************************************************************
 *                                Struct Methods
 ******************************************************************************/

// IoTRachFreq::= SEQUENCE
SRSASN_CODE iot_rach_freq_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(ul_freq_id.pack(bref));
  HANDLE_CODE(ul_sub_freq_bit_map.pack(bref));

  return SRSASN_SUCCESS;
}
SRSASN_CODE iot_rach_freq_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(ul_freq_id.unpack(bref));
  HANDLE_CODE(ul_sub_freq_bit_map.unpack(bref));

  return SRSASN_SUCCESS;
}
void iot_rach_freq_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("ulFrequencyIdentity");
  ul_freq_id.to_json(j);
  j.write_str("ulSubFreqBitMap", ul_sub_freq_bit_map.to_string());
  j.end_obj();
}

// IoTUAC-BarringPerCat::= SEQUENCE
SRSASN_CODE iot_uac_barring_per_cat_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(access_category.pack(bref));
  HANDLE_CODE(uac_barring_for_access_id.pack(bref));

  return SRSASN_SUCCESS;
}
SRSASN_CODE iot_uac_barring_per_cat_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(access_category.unpack(bref));
  HANDLE_CODE(uac_barring_for_access_id.unpack(bref));

  return SRSASN_SUCCESS;
}
void iot_uac_barring_per_cat_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("accessCategory", access_category.to_string());
  j.write_str("uac-BarringForAccessIdentity", uac_barring_for_access_id.to_string());
  j.end_obj();
}
const char* iot_uac_barring_per_cat_s::access_category_opts::to_string() const
{
  static const char* options[] = {"acCatNum3", "acCatNum7", "acCatNum8", "spare"};
  return convert_enum_idx(options, 4, value, "iot_uac_barring_per_cat_s::access_category_e_");
}
uint8_t iot_uac_barring_per_cat_s::access_category_opts::to_number() const
{
  static const uint8_t options[] = {3, 7, 8, 1};
  return map_enum_number(options, 4, value, "iot_uac_barring_per_cat_s::access_category_e_");
}

// IoTRachConfig::= SEQUENCE
SRSASN_CODE iot_rach_config_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(dl_band_id.pack(bref));
  HANDLE_CODE(pack_dyn_seq_of(bref, iot_rach_freq_list, 1, 4));
  HANDLE_CODE(ul_fn_assignment.pack(bref));
  HANDLE_CODE(ra_response_windowsize.pack(bref));

  return SRSASN_SUCCESS;
}
SRSASN_CODE iot_rach_config_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(dl_band_id.unpack(bref));
  HANDLE_CODE(unpack_dyn_seq_of(iot_rach_freq_list, bref, 1, 4));
  HANDLE_CODE(ul_fn_assignment.unpack(bref));
  HANDLE_CODE(ra_response_windowsize.unpack(bref));

  return SRSASN_SUCCESS;
}
void iot_rach_config_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("dlBandIdentity	");
  dl_band_id.to_json(j);

  j.start_array("IoTRachFreqList");
  for (uint32_t i1 = 0; i1 < iot_rach_freq_list.size(); ++i1) {
    iot_rach_freq_list[i1].to_json(j);
  }
  j.end_array();

  j.write_str("ulFnAssignment", ul_fn_assignment.to_string());
  j.write_str("ra-ResponseWindowSize", ra_response_windowsize.to_string());
  j.end_obj();
}
const char* iot_rach_config_s::ra_response_windowsize_opts::to_string() const
{
  static const char* options[] = {"rf10", "rf20", "rf30", "spare"};
  return convert_enum_idx(options, 4, value, "iot_rach_config_s::ra_response_windowsize_e_");
}
uint8_t iot_rach_config_s::ra_response_windowsize_opts::to_number() const
{
  static const uint8_t options[] = {10, 20, 30, 1};
  return map_enum_number(options, 4, value, "iot_rach_config_s::ra_response_windowsize_e_");
}

// IoTAgchConfig-Normal::= SEQUENCE
SRSASN_CODE iot_agch_conf_normal_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(dl_band_id.pack(bref));
  HANDLE_CODE(dl_freq_id.pack(bref));
  HANDLE_CODE(ul_fn_assignment.pack(bref));

  return SRSASN_SUCCESS;
}
SRSASN_CODE iot_agch_conf_normal_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(dl_band_id.unpack(bref));
  HANDLE_CODE(dl_freq_id.unpack(bref));
  HANDLE_CODE(ul_fn_assignment.unpack(bref));

  return SRSASN_SUCCESS;
}
void iot_agch_conf_normal_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("dlBandIdentity");
  dl_band_id.to_json(j);

  j.write_fieldname("dlFrequencyIdentity");
  dl_freq_id.to_json(j);

  j.write_str("ulFnAssignment", ul_fn_assignment.to_string());
  j.end_obj();
}

// IoTAgchConfig-DlFreqSpread::= SEQUENCE
SRSASN_CODE iot_agch_conf_dl_freq_spread_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(spread_factor.pack(bref));
  HANDLE_CODE(code_index.pack(bref));
  HANDLE_CODE(dl_fn_assignment.pack(bref));

  return SRSASN_SUCCESS;
}
SRSASN_CODE iot_agch_conf_dl_freq_spread_s::unpack(cbit_ref& bref)
{

  HANDLE_CODE(spread_factor.unpack(bref));
  HANDLE_CODE(code_index.unpack(bref));
  HANDLE_CODE(dl_fn_assignment.unpack(bref));

  return SRSASN_SUCCESS;
}
void iot_agch_conf_dl_freq_spread_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("spreadFactor", spread_factor.to_string());
  j.write_str("codeIndex", code_index.to_string());
  j.write_str("dlFnAssignment", dl_fn_assignment.to_string());
  j.end_obj();
}

const char* iot_agch_conf_dl_freq_spread_s::spread_factor_opts::to_string() const
{
  static const char* options[] = {"sf128", "sf256", "sf512", "spare"};
  return convert_enum_idx(options, 4, value, "iot_agch_conf_dl_freq_spread_s::spread_factor_e_");
}
uint16_t iot_agch_conf_dl_freq_spread_s::spread_factor_opts::to_number() const
{
  static const uint16_t options[] = {128, 256, 512, 1};
  return map_enum_number(options, 4, value, "iot_agch_conf_dl_freq_spread_s::spread_factor_e_");
}

// IoTUeTimer::= SEQUENCE
SRSASN_CODE iot_ue_timer_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(iot_t300.pack(bref));
  HANDLE_CODE(iot_t301.pack(bref));
  HANDLE_CODE(iot_t302.pack(bref));

  return SRSASN_SUCCESS;
}
SRSASN_CODE iot_ue_timer_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(iot_t300.unpack(bref));
  HANDLE_CODE(iot_t301.unpack(bref));
  HANDLE_CODE(iot_t302.unpack(bref));

  return SRSASN_SUCCESS;
}
void iot_ue_timer_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("iotT300", iot_t300.to_string());
  j.write_str("iotT301", iot_t301.to_string());
  j.write_str("iotT302", iot_t302.to_string());
  j.end_obj();
}
const char* iot_ue_timer_s::iot_t300_opts::to_string() const
{
  static const char* options[] = {"ms2000", "ms5000", "ms10000", "ms20000"};
  return convert_enum_idx(options, 4, value, "iot_ue_timer_s::iot_t300_e_");
}
uint16_t iot_ue_timer_s::iot_t300_opts::to_number() const
{
  static const uint16_t options[] = {2000, 5000, 10000, 20000};
  return map_enum_number(options, 4, value, "iot_ue_timer_s::iot_t300_e_");
}

const char* iot_ue_timer_s::iot_t301_opts::to_string() const
{
  static const char* options[] = {"ms2000", "ms5000", "ms10000", "ms20000"};
  return convert_enum_idx(options, 4, value, "iot_ue_timer_s::iot_t301_e_");
}
uint16_t iot_ue_timer_s::iot_t301_opts::to_number() const
{
  static const uint16_t options[] = {2000, 5000, 10000, 20000};
  return map_enum_number(options, 4, value, "iot_ue_timer_s::iot_t301_e_");
}

const char* iot_ue_timer_s::iot_t302_opts::to_string() const
{
  static const char* options[] = {"ms200", "ms500", "ms1000", "ms2000"};
  return convert_enum_idx(options, 4, value, "iot_ue_timer_s::iot_t302_e_");
}
uint16_t iot_ue_timer_s::iot_t302_opts::to_number() const
{
  static const uint16_t options[] = {200, 500, 1000, 2000};
  return map_enum_number(options, 4, value, "iot_ue_timer_s::iot_t302_e_");
}

//ioTAgchConfig			CHIOCE
void iot_sib_s::iot_agch_config_c_::destroy_()
{
  switch (type_) {
    case types::iot_agch_conf_normal:
      c.destroy<iot_agch_conf_normal_s>();
      break;
    case types::iot_agch_conf_dl_freq_spread:
      c.destroy<iot_agch_conf_dl_freq_spread_s>();
      break;
    default:
      break;
  }
}
void iot_sib_s::iot_agch_config_c_::set(types::options e)
{
  destroy_();
  type_ = e;
  switch (type_) {
    case types::iot_agch_conf_normal:
      c.init<iot_agch_conf_normal_s>();
      break;
    case types::iot_agch_conf_dl_freq_spread:
      c.init<iot_agch_conf_dl_freq_spread_s>();
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "iot_sib_s::iot_agch_config_c_");
  }
}
iot_sib_s::iot_agch_config_c_::iot_agch_config_c_(const iot_sib_s::iot_agch_config_c_& other)
{
  type_ = other.type();
  switch (type_) {
    case types::iot_agch_conf_normal:
      c.init(other.c.get<iot_agch_conf_normal_s>());
      break;
    case types::iot_agch_conf_dl_freq_spread:
      c.init(other.c.get<iot_agch_conf_dl_freq_spread_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "iot_sib_s::iot_agch_config_c_");
  }
}
iot_sib_s::iot_agch_config_c_& iot_sib_s::iot_agch_config_c_::operator=(const iot_sib_s::iot_agch_config_c_& other)
{
  if (this == &other) {
    return *this;
  }
  set(other.type());
  switch (type_) {
    case types::iot_agch_conf_normal:
      c.set(other.c.get<iot_agch_conf_normal_s>());
      break;
    case types::iot_agch_conf_dl_freq_spread:
      c.set(other.c.get<iot_agch_conf_dl_freq_spread_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "iot_sib_s::iot_agch_config_c_");
  }

  return *this;
}

iot_agch_conf_normal_s& iot_sib_s::iot_agch_config_c_::set_iot_agch_conf_normal()
{
  set(types::iot_agch_conf_normal);
  return c.get<iot_agch_conf_normal_s>();
}
iot_agch_conf_dl_freq_spread_s& iot_sib_s::iot_agch_config_c_::set_iot_agch_conf_dl_freq_spread()
{
  set(types::iot_agch_conf_dl_freq_spread);
  return c.get<iot_agch_conf_dl_freq_spread_s>();
}

void iot_sib_s::iot_agch_config_c_::to_json(json_writer& j) const
{
  j.start_obj();
  switch (type_) {
    case types::iot_agch_conf_normal:
      j.write_fieldname("IoTAgchConfig-Normal");
      c.get<iot_agch_conf_normal_s>().to_json(j);
      break;
    case types::iot_agch_conf_dl_freq_spread:
      j.write_fieldname("IoTAgchConfig-DlFreqSpread");
      c.get<iot_agch_conf_dl_freq_spread_s>().to_json(j);
      break;
    default:
      log_invalid_choice_id(type_, "iot_sib_s::iot_agch_config_c_");
  }
  j.end_obj();
}
SRSASN_CODE iot_sib_s::iot_agch_config_c_::pack(bit_ref& bref) const
{
  type_.pack(bref);
  switch (type_) {
    case types::iot_agch_conf_normal:
      HANDLE_CODE(c.get<iot_agch_conf_normal_s>().pack(bref));
      break;
    case types::iot_agch_conf_dl_freq_spread:
      HANDLE_CODE(c.get<iot_agch_conf_dl_freq_spread_s>().pack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "iot_sib_s::iot_agch_config_c_");
      return SRSASN_ERROR_ENCODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE iot_sib_s::iot_agch_config_c_::unpack(cbit_ref& bref)
{
  types e;
  e.unpack(bref);
  set(e);
  switch (type_) {
    case types::iot_agch_conf_normal:
      HANDLE_CODE(c.get<iot_agch_conf_normal_s>().unpack(bref));
      break;
    case types::iot_agch_conf_dl_freq_spread:
      HANDLE_CODE(c.get<iot_agch_conf_dl_freq_spread_s>().unpack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "iot_sib_s::iot_agch_config_c_");
      return SRSASN_ERROR_DECODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
const char* iot_sib_s::iot_agch_config_c_::types_opts::to_string() const
{
  static const char* options[] = {"iot_agch_conf_normal", "iot_agch_conf_dl_freq_spread"};
  return convert_enum_idx(options, 2, value, "iot_sib_s::iot_agch_config_c_::types");
}

// IoTSystemInformation::= SEQUENCE
SRSASN_CODE iot_sib_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(iot_uac_barring_list_present, 1));

  if (iot_uac_barring_list_present) {
    HANDLE_CODE(pack_dyn_seq_of(bref, iot_uac_barring_list,1,3));
  }
  HANDLE_CODE(iot_dl_trans_coies.pack(bref));
  HANDLE_CODE(iot_rach_config.pack(bref));
  HANDLE_CODE(iot_agch_config.pack(bref));
  HANDLE_CODE(iot_ue_timer.pack(bref));

  return SRSASN_SUCCESS;
}
SRSASN_CODE iot_sib_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(iot_uac_barring_list_present, 1));

  if (iot_uac_barring_list_present) {
    HANDLE_CODE(unpack_dyn_seq_of(iot_uac_barring_list, bref, 1, 3));
  }
  HANDLE_CODE(iot_dl_trans_coies.unpack(bref));
  HANDLE_CODE(iot_rach_config.unpack(bref));
  HANDLE_CODE(iot_agch_config.unpack(bref));
  HANDLE_CODE(iot_ue_timer.unpack(bref));

  return SRSASN_SUCCESS;
}
void iot_sib_s::to_json(json_writer& j) const
{
  j.start_obj();

  if (iot_uac_barring_list_present) {
    j.start_array("ioTUAC-BarringList");
    for (uint32_t i1 = 0; i1 < iot_uac_barring_list.size(); ++i1) {
      iot_uac_barring_list[i1].to_json(j);
    }
    j.end_array();
  }

  j.write_str("ioTDlTransmitCoies", iot_dl_trans_coies.to_string());
  j.write_fieldname("ioTRachConfig");
  iot_rach_config.to_json(j);
  j.write_fieldname("ioTAgchConfig");
  iot_agch_config.to_json(j);
  j.write_fieldname("ioTUeTimer");
  iot_ue_timer.to_json(j);
  j.end_obj();
}

// TBCCH-MessageType::= CHOICE
void tbcch_msg_type_c::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("ioTSystemInformation");
  c.to_json(j);
  j.end_obj();
}
SRSASN_CODE tbcch_msg_type_c::pack(bit_ref& bref) const
{
  HANDLE_CODE(c.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE tbcch_msg_type_c::unpack(cbit_ref& bref)
{
  HANDLE_CODE(c.unpack(bref));
  return SRSASN_SUCCESS;
}

// TBCCH-Message::= SEQUENCE
SRSASN_CODE tbcch_msg_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(msg.pack(bref));

  bref.align_bytes_zero();

  return SRSASN_SUCCESS;
}
SRSASN_CODE tbcch_msg_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(msg.unpack(bref));

  bref.align_bytes();

  return SRSASN_SUCCESS;
}
void tbcch_msg_s::to_json(json_writer& j) const
{
  j.start_array();
  j.start_obj();
  j.start_obj("TBCCH-Message");
  j.write_fieldname("TBCCH-MessageType");
  msg.to_json(j);
  j.end_obj();
  j.end_obj();
  j.end_array();
}
