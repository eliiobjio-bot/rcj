#include "srsran/asn1/rrc/mib_sib_asn1.h"
// #include "srsran/asn1/asn1_utils.h"
#include <sstream>

using namespace asn1;
using namespace asn1::rrc;


// RSSI ::= SEQUENCE
void rssi_c::destroy_() {}
void rssi_c::set(types::options e)
{
  destroy_();
  type_ = e;
}
rssi_c::rssi_c(const rssi_c& other)
{
  type_ = other.type();
  switch (type_) {
    case types::rssi_normal:
      c.init(other.c.get<uint8_t>());
      break;
    case types::rssi_dl_freq_sp:
      c.init(other.c.get<uint8_t>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "rssi_c");
  }
}
rssi_c& rssi_c::operator=(const rssi_c& other)
{
  if (this == &other) {
    return *this;
  }
  set(other.type());
  switch (type_) {
    case types::rssi_normal:
      c.set(other.c.get<uint8_t>());
      break;
    case types::rssi_dl_freq_sp:
      c.set(other.c.get<uint8_t>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "rssi_c");
  }

  return *this;
}
uint8_t& rssi_c::set_rssi_normal()
{
  set(types::rssi_normal);
  return c.get<uint8_t>();
}
uint8_t& rssi_c::set_rssi_dl_freq_sp()
{
  set(types::rssi_dl_freq_sp);
  return c.get<uint8_t>();
}
void rssi_c::to_json(json_writer& j) const
{
  j.start_obj();
  switch (type_) {
    case types::rssi_normal:
      j.write_int("rssi_normal", c.get<uint8_t>());
      break;
    case types::rssi_dl_freq_sp:
      j.write_int("rssi_dl_freq_sp", c.get<uint8_t>());
      break;
    default:
      log_invalid_choice_id(type_, "rssi_c");
  }
  j.end_obj();
}
SRSASN_CODE rssi_c::pack(bit_ref& bref) const
{
  type_.pack(bref);
  switch (type_) {
    case types::rssi_normal:
      HANDLE_CODE(pack_integer(bref, c.get<uint8_t>(), (uint8_t)0u, (uint8_t)63u));
      break;
    case types::rssi_dl_freq_sp:
      HANDLE_CODE(pack_integer(bref, c.get<uint8_t>(), (uint8_t)0u, (uint8_t)63u));
      break;
    default:
      log_invalid_choice_id(type_, "rssi_c");
      return SRSASN_ERROR_ENCODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE rssi_c::unpack(cbit_ref& bref)
{
  types e;
  e.unpack(bref);
  set(e);
  switch (type_) {
    case types::rssi_normal:
      HANDLE_CODE(unpack_integer(c.get<uint8_t>(), bref, (uint8_t)0u, (uint8_t)63u));
      break;
    case types::rssi_dl_freq_sp:
      HANDLE_CODE(unpack_integer(c.get<uint8_t>(), bref, (uint8_t)0u, (uint8_t)63u));
      break;
    default:
      log_invalid_choice_id(type_, "rssi_c");
      return SRSASN_ERROR_DECODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
bool rssi_c::operator==(const rssi_c& other) const
{
  if (type_ != other.type_) {
    return false;
  }
  switch (type_) {
    case types::rssi_normal:
      return c.get<uint8_t>() == other.c.get<uint8_t>();
    case types::rssi_dl_freq_sp:
      return c.get<uint8_t>() == other.c.get<uint8_t>();
    default:
      return true;
  }
  return true;
}
const char* rssi_c::types_opts::to_string() const
{
  static const char* options[] = {"rssi_normal", "rssi_dl_freq_sp"};
  return convert_enum_idx(options, 2, value, "rssi_c::types");
}
uint8_t rssi_c::types_opts::to_number() const
{
  static const uint8_t options[] = {0,1};
  return map_enum_number(options, 2, value, "rssi_c::types");
}

// Q-PxLevMin ::= SEQUENCE
SRSASN_CODE q_rx_lev_min_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(pack_integer(bref, q_min, (int8_t)-70, (int8_t)-40));

  return SRSASN_SUCCESS;
}
SRSASN_CODE q_rx_lev_min_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(unpack_integer(q_min, bref, (int8_t)-70, (int8_t)-40));

  return SRSASN_SUCCESS;
}
void q_rx_lev_min_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_int("Q_rx_lev_min", q_min);
  j.end_obj();
}

SRSASN_CODE band_id_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(ba_id.pack(bref));

  return SRSASN_SUCCESS;
}
SRSASN_CODE band_id_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(ba_id.unpack(bref));

  return SRSASN_SUCCESS;
}
void band_id_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("band-identity", ba_id.to_string());
  j.end_obj();
}
// BeamIdentity ::=SEQUENCE
SRSASN_CODE beam_id_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(beam_id.pack(bref));

  return SRSASN_SUCCESS;
}
SRSASN_CODE beam_id_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(beam_id.unpack(bref));

  return SRSASN_SUCCESS;
}
void beam_id_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("beamIdentity", beam_id.to_string());
  j.end_obj();
}

// FrqquencyIdentity ::= SEQUENCE
SRSASN_CODE freq_id_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(freq_id.pack(bref));

  return SRSASN_SUCCESS;
}
SRSASN_CODE freq_id_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(freq_id.unpack(bref));

  return SRSASN_SUCCESS;
}
void freq_id_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("frequency-identity", freq_id.to_string());
  j.end_obj();
}
  




const char* sib_wx_s::beam_acc_rel_info_s::beam_barred_opts::to_string() const
{
  static const char* options[] = {"barred", "notbarred"};
  return convert_enum_idx(options, 2, value, "sib_wx_s::beam_acc_rel_info_s::beam_barred_e_");

}

const char* sib_wx_s::beam_resel_rel_info_s::meas_gap_cfg_wx_c_::types_opts::to_string() const 
{
  static const char* options[] = {"cfg_li_nom", "cfg_li_dl_fr_sp"};
  return convert_enum_idx(options, 2, value, "sib_wx_s::beam_resel_rel_info_s::meas_gap_cfg_wx_c_");

}

const char* cfg_nom_s::meas_nom_ratio_opts::to_string() const
{
  static const char* options[] = {
      "dB5", "dB6", "dB7", "dB8", "dB9", "dB10", "dB11", "dB12", "dB13", "dB14", "dB15", "dB16"};
  return convert_enum_idx(options, 12, value, "cfg_nom_s::meas_nom_ratio_e_");
}
uint8_t cfg_nom_s::meas_nom_ratio_opts::to_number()const
{
  static const uint8_t options[] = { 5,6,7,8,9,10,11,12,13,14,15,16};
  return map_enum_number(options, 12, value, "cfg_nom_s::meas_nom_ratio_e_");
}

const char* uac_barr_info_set_s::uac_barr_factor_opts::to_string() const
{
  static const char* options[] = {"p00", "p20", "p50", "p70", "p90", "p95", "p100"};
  return convert_enum_idx(options, 7, value, "uac_barr_info_set_s::uac_barr_factor_e_");
}
float uac_barr_info_set_s::uac_barr_factor_opts::to_number() const
{
  static const float options[] = {0.0, 2.0, 5.0, 7.0, 9.0, 9.5, 10.0};
  return map_enum_number(options, 7, value, "uac_barr_info_set_s::uac_barr_factor_e_");
}

const char* uac_barr_info_set_s::uac_barr_time_opts::to_string() const
{
  static const char* options[] = {"s8", "s32", "s64", "s128", "s256"};
  return convert_enum_idx(options, 5, value, "uac_barr_info_set_s::uac_barr_time_e_");
}
uint16_t uac_barr_info_set_s::uac_barr_time_opts::to_number() const
{
  static const uint16_t options[] = {8, 32, 64, 128, 256};
  return map_enum_number(options, 5, value, "uac_barr_info_set_s::uac_barr_time_e_");
}

const char* rach_cfg_com_n_s::rach_slot_ass_opts::to_string() const
{
  static const char* options[] = {
      "half_frame0",
      "half_frame1",
      "both"
  };
  return convert_enum_idx(options, 3, value, " rach_cfg_com_n_s::rach_slot_ass_e_");
}
int8_t rach_cfg_com_n_s::rach_slot_ass_opts::to_number() const
{
  static const uint8_t options[] = {0, 1, 2};
  return map_enum_number(options, 3, value, "rach_cfg_com_n_s::rach_slot_ass_e_");
}

const char* rach_cfg_com_n_s::ra_res_wi_si_opts::to_string() const
{
  static const char* options[] = {"rf5", "rf10", "rf15", "spare1"};
  return convert_enum_idx(options, 4, value, "rach_cfg_com_n_s::ra_res_wi_si_e_");
}
int8_t rach_cfg_com_n_s::ra_res_wi_si_opts::to_number() const
{
  static const uint8_t options[] = {5, 10, 15, 1};
  return map_enum_number(options, 4, value, "rach_cfg_com_n_s::ra_res_wi_si_e_");
}

const char* agch_cfg_com_s::agch_slot_start_opts::to_string()const
{
  static const char* options[] = {"slot1", "slot2", "slot3", "slot4"};
  return convert_enum_idx(options, 4, value, "agch_cfg_com_s::agch_slot_start_e_");
}
int8_t agch_cfg_com_s::agch_slot_start_opts::to_number() const
{
  static const uint8_t options[] = {1, 2, 3, 4};
  return map_enum_number(options, 4, value, "agch_cfg_com_s::agch_slot_start_e_");
}

const char* bbch_cfg_com_s::bbch_slot_ass_opts::to_string() const 
{
  static const char* options[] = {"slot1", "slot2", "slot3", "slot4"};
  return convert_enum_idx(options, 4, value, "bbch_cfg_com_s::bbch_slot_ass_e_");
}
int8_t bbch_cfg_com_s::bbch_slot_ass_opts::to_number()const 
{
  static const uint8_t options[] = {1, 2, 3, 4};
  return map_enum_number(options, 4, value, "bbch_cfg_com_s::bbch_slot_ass_e_");
}

const char* iotsi_cfg_c::types_opts::to_string()const 
{
  static const char* options[] = {"iotsi_cfg_normal", "iotsi_cfg_dl_freq_spread"};
  return convert_enum_idx(options, 2, value, "iotsi_cfg_c::types");
}
uint8_t iotsi_cfg_c::types_opts::to_number() const
{
  static const uint8_t options[] = {0, 1};
  return map_enum_number(options, 2, value, "iotsi_cfg_c::types");
}

const char* iotsi_cfg_dl_freq_spread_s::spread_factor_opts::to_string()const 
{
  static const char* options[] = {"sf128", "sf256", "sf512", "spare"};
  return convert_enum_idx(options, 4, value, " iotsi_cfg_dl_freq_spread_s::spread_factor_e_");
}
int16_t iotsi_cfg_dl_freq_spread_s::spread_factor_opts::to_number() const
{
  static const uint16_t options[] = {128,256,512,1};
  return map_enum_number(options, 4, value, "iotsi_cfg_dl_freq_spread_s::spread_factor_e_");
}

const char* ue_timer_and_constant_s::t300_opts::to_string() const 
{
  static const char* options[] = {"ms1000", "ms2000", "ms3000", "ms5000", "ms8000", "ms10000", "ms15000", "ms20000"};
  return convert_enum_idx(options, 8, value, "ue_timer_and_constant_s::t300_e_");
}
int16_t ue_timer_and_constant_s::t300_opts::to_number() const
{
  static const uint16_t options[] = {1000, 2000, 3000, 5000, 8000, 10000, 15000, 20000};
  return map_enum_number(options, 8, value, "ue_timer_and_constant_s::t300_e_");
}

const char* ue_timer_and_constant_s::t301_opts::to_string() const
{
  static const char* options[] = {"ms100", "ms200", "ms300", "ms400", "ms600", "ms1000", "ms1500", "ms2000"};
  return convert_enum_idx(options, 8, value, "ue_timer_and_constant_s::t301_e_");
}
int16_t ue_timer_and_constant_s::t301_opts::to_number() const
{
  static const uint16_t options[] = {100, 200, 300, 400, 600, 1000, 1500, 2000};
  return map_enum_number(options, 8, value, "ue_timer_and_constant_s::t301_e_");
}

const char* ue_timer_and_constant_s::t302_opts::to_string() const
{
  static const char* options[] = {"ms100", "ms200", "ms300", "ms400", "ms600", "ms1000", "ms1500", "ms2000"};
  return convert_enum_idx(options, 8, value, "ue_timer_and_constant_s::t302_e_");
}
int16_t ue_timer_and_constant_s::t302_opts::to_number() const
{
  static const uint16_t options[] = {100, 200, 300, 400, 600, 1000, 1500, 2000};
  return map_enum_number(options, 8, value, "ue_timer_and_constant_s::t302_e_");
}

const char* ue_timer_and_constant_s::t310_opts::to_string() const
{
  static const char* options[] = {"ms0", "ms50", "ms100", "ms200", "ms500", "ms1000", "ms2000"};
 return convert_enum_idx(options, 7, value, "ue_timer_and_constant_s::t310_e_");
}
int16_t ue_timer_and_constant_s::t310_opts::to_number() const
{
  static const uint16_t options[] = {0, 50, 100, 200, 500, 1000,2000};
  return map_enum_number(options, 7, value, "ue_timer_and_constant_s::t310_e_");
}

const char* ue_timer_and_constant_s::n310_opts::to_string() const
{
  static const char* options[] = {"n1", "n2", "n3", "n4", "n6", "n8", "n10", "n20"};
  return convert_enum_idx(options, 8, value, "ue_timer_and_constant_s::n310_e_");
}
uint8_t ue_timer_and_constant_s::n310_opts::to_number() const
{
  static const uint8_t options[] = {1, 2, 3, 4, 6, 8, 10, 20};
  return map_enum_number(options, 8, value, "ue_timer_and_constant_s::n310_e_");
}

const char* ue_timer_and_constant_s::t311_opts::to_string() const
{
  static const char* options[] = {"ms1000", "ms3000", "ms5000", "ms10000", "ms15000", "ms20000", "ms30000", "spare1"};
  return convert_enum_idx(options, 8, value, "ue_timer_and_constant_s::t311_e_");
}
uint16_t ue_timer_and_constant_s::t311_opts::to_number() const
{
  static const uint16_t options[] = {1000, 3000, 5000, 10000, 15000, 20000, 30000,1};
  return map_enum_number(options, 8, value, "ue_timer_and_constant_s::t311_e_");
}

const char* ue_timer_and_constant_s::n311_opts::to_string() const
{
  static const char* options[] = {
    "n1",
    "n2",
    "n3",
    "n4",
    "n5",
    "n6",
    "n8 ",
    "n10 "};
  return convert_enum_idx(options, 8, value, "ue_timer_and_constant_s::n311_e_");
}
uint8_t ue_timer_and_constant_s::n311_opts::to_number() const
{
  static const uint8_t options[] = {1, 2, 3, 4, 5, 6, 8, 10};
  return map_enum_number(options, 8, value, "ue_timer_and_constant_s::n311_e_");
}

const char* geo_info_update_para_s::update_timer_opts::to_string()const 
{
  static const char* options[] = {"min10", "min30", "min60", "infinity"};
  return convert_enum_idx(options, 4, value, "geo_info_update_para_s::update_timer_e_");

}
int8_t geo_info_update_para_s::update_timer_opts::to_number() const
{
  static const uint8_t options[] = {10, 30, 60, 1};
  return map_enum_number(options, 4, value, "geo_info_update_para_s::update_timer_e_");
}

const char* geo_info_update_para_s::update_distance_opts::to_string() const
{
  static const char* options[] = {"km3", "km10", "km50", "infinity"};
  return convert_enum_idx(options, 4, value, "geo_info_update_para_s::update_distance_e_");
}
int8_t geo_info_update_para_s::update_distance_opts::to_number() const
{
  static const uint8_t options[] = {3, 10, 5, 1};
  return map_enum_number(options, 4, value, "geo_info_update_para_s::update_distance_e_");
}

const char* mib_wx_s::bcch_slot_start_opts::to_string() const
{
  static const char* options[] = {"slot1", "slot2", "slot3", "slot4"};
  return convert_enum_idx(options, 4, value, "mib_wx_s::bcch_slot_start_e_");
}
uint8_t mib_wx_s::bcch_slot_start_opts::to_number() const
{
  static const uint8_t options[] = {1, 2, 3, 4};
  return map_enum_number(options, 4, value, "mib_wx_s::bcch_slot_start_e_");
}

const char* mib_wx_s::meas_normal_ra_opts::to_string() const
{
  static const char* options[] = {
      "dB5", "dB6", "dB7", "dB8", "dB9", "dB10", "dB11", "dB12", "dB13", "dB14", "dB15", "dB16"};
  return convert_enum_idx(options, 12, value, "mib_wx_s::meas_normal_ra_e_");
}
uint8_t mib_wx_s::meas_normal_ra_opts::to_number() const
{
  static const uint8_t options[] = {5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  return map_enum_number(options, 12, value, "mib_wx_s::meas_normal_ra_e_");
}

const char* mib_wx_s::naviInfo_slot_ass_opts::to_string() const
{
  static const char* options[] = {"slot1", "slot2", "slot3", "slot4"};
  return convert_enum_idx(options, 4, value, "mib_wx_s::naviInfo_slot_ass_e_");
}
uint8_t mib_wx_s::naviInfo_slot_ass_opts::to_number() const
{
  static const uint8_t options[] = {1, 2, 3, 4};
  return map_enum_number(options, 4, value, "mib_wx_s::naviInfo_slot_ass_e_");
}


// MeasGapConfig-Normal ::= SEQUENCE
SRSASN_CODE cfg_nom_s::pack(bit_ref& bref)const 
{
  HANDLE_CODE(pack_integer(bref, beam_index, (uint8_t)1u, (uint8_t)15u));
  HANDLE_CODE(pack_integer(bref, frame_offset, (uint8_t)0u, (uint8_t)51u));
  HANDLE_CODE(fcch_band_id.pack(bref));
  HANDLE_CODE(fcch_freq_id.pack(bref));
  HANDLE_CODE(meas_nom_ratio.pack(bref));
  HANDLE_CODE(pack_integer(bref, mib_re_fra_num, (uint8_t)0u, (uint8_t)51u));
  return SRSASN_SUCCESS;
}
SRSASN_CODE cfg_nom_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(unpack_integer(beam_index, bref, (uint8_t)1u, (uint8_t)15u));
  HANDLE_CODE(unpack_integer(frame_offset, bref, (uint8_t)0u, (uint8_t)51u));
  HANDLE_CODE(fcch_band_id.unpack(bref));
  HANDLE_CODE(fcch_freq_id.unpack(bref));
  HANDLE_CODE(meas_nom_ratio.unpack(bref));
  HANDLE_CODE(unpack_integer(mib_re_fra_num, bref, (uint8_t)0u, (uint8_t)51u));
  return SRSASN_SUCCESS;
}
void cfg_nom_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_int("beam-Index", beam_index);
  j.write_int("frame-Offset", frame_offset);
  j.write_fieldname("fcch-BandIdentity");
  fcch_band_id.to_json(j);
  j.write_fieldname("fcch-FerquencyIdentity");
  fcch_freq_id.to_json(j);
  j.write_str("meas-NormalizationRatio", meas_nom_ratio.to_string());
  j.write_int("mibRelativeFrameNum", mib_re_fra_num);
  j.end_obj();
}
// MeasGapConfig-DlFreqSpread ::= SEQUENCE
SRSASN_CODE cfg_dl_fr_sp_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(pack_integer(bref, beam_index, (uint8_t)1u, (uint8_t)15u));
  HANDLE_CODE(pack_integer(bref, sec_syn_group_id, (uint8_t)1u, (uint8_t)48u));
  return SRSASN_SUCCESS;
}
SRSASN_CODE cfg_dl_fr_sp_s::unpack(cbit_ref& bref) 
{
  HANDLE_CODE(unpack_integer(beam_index, bref, (uint8_t)1u, (uint8_t)15u));
  HANDLE_CODE(unpack_integer(sec_syn_group_id, bref, (uint8_t)1u, (uint8_t)48u));
  return SRSASN_SUCCESS;
}
void cfg_dl_fr_sp_s::to_json(json_writer& j)const 
{
  j.start_obj();
  j.write_int("beam-Index", beam_index);
  j.write_int("second-SynGroupID", sec_syn_group_id);
  j.end_obj();
}
    // MeasGapConfig ::= CHOICE
void sib_wx_s::beam_resel_rel_info_s::meas_gap_cfg_wx_c_::destroy_()
{
  switch (type_) {
    case types::cfg_li_nom:
      c.destroy<cfg_li_nom_s>();
      break;
    case types::cfg_li_dl_fr_sp:
      c.destroy<cfg_li_dl_fr_sp_s>();
      break;
    default:
      break;
  }
}
void sib_wx_s ::beam_resel_rel_info_s::meas_gap_cfg_wx_c_::set(types::options e)
{
  destroy_();
  type_ = e;
  switch (type_) {
    case types::cfg_li_nom:
        c.init<cfg_li_nom_s>();
        break; 
    case types::cfg_li_dl_fr_sp:
        c.init<cfg_li_dl_fr_sp_s>();
        break;
    default:
        log_invalid_choice_id(type_, "meas_gap_cfg_wx_c_");

  }
}
sib_wx_s::beam_resel_rel_info_s::meas_gap_cfg_wx_c_::meas_gap_cfg_wx_c_(const meas_gap_cfg_wx_c_& other)
{
  type_ = other.type();
  switch (type_) {
    case types::cfg_li_nom:
        c.init(other.c.get<cfg_li_nom_s>());
        break;
    case types::cfg_li_dl_fr_sp:
        c.init(other.c.get<cfg_li_dl_fr_sp_s>());
        break;
    case types::nulltype:
        break;
    default:
        log_invalid_choice_id(type_, "meas_gap_cfg_wx_c_");
  }
}
sib_wx_s::beam_resel_rel_info_s::meas_gap_cfg_wx_c_& 
sib_wx_s::beam_resel_rel_info_s::meas_gap_cfg_wx_c_::operator=(const meas_gap_cfg_wx_c_& other)
{
  if (this==&other) {
    return *this;
  }
  set(other.type());
  switch (type_) {
    case types::cfg_li_nom:
        c.set(other.c.get<cfg_li_nom_s>());
        break;
    case types::cfg_li_dl_fr_sp:
        c.set(other.c.get<cfg_li_dl_fr_sp_s>());
        break;
    case types::nulltype:
        break;
    default:
        log_invalid_choice_id(type_ ,"meas_gap_cfg_wx_c_");
  }
  return *this;
}

cfg_li_nom_s& sib_wx_s::beam_resel_rel_info_s::meas_gap_cfg_wx_c_::set_cfg_li_nom() 
{
  set(types::cfg_li_nom);
  return c.get<cfg_li_nom_s>();
}
cfg_li_dl_fr_sp_s& sib_wx_s::beam_resel_rel_info_s::meas_gap_cfg_wx_c_::set_cfg_li_dl_fr_sp() 
{
  set(types::cfg_li_dl_fr_sp);
  return c.get<cfg_li_dl_fr_sp_s>();
}
SRSASN_CODE sib_wx_s::beam_resel_rel_info_s::meas_gap_cfg_wx_c_::pack(bit_ref& bref) const
{
  type_.pack(bref);
  switch (type_) {
    case types::cfg_li_nom:
        HANDLE_CODE(pack_dyn_seq_of(bref, c.get<cfg_li_nom_s>(), 1, 15));
        break;
    case types::cfg_li_dl_fr_sp:
        HANDLE_CODE(pack_dyn_seq_of(bref, c.get<cfg_li_dl_fr_sp_s>(), 1, 15));
        break;
    default:
        log_invalid_choice_id(type_, "beam_resel_rel_info_s::meas_gap_cfg_wx_c_");
        return SRSASN_ERROR_ENCODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE sib_wx_s::beam_resel_rel_info_s::meas_gap_cfg_wx_c_::unpack(cbit_ref& bref)
{
  types e;
  e.unpack(bref);
  set(e);
  switch (type_) {
    case types::cfg_li_nom:
        HANDLE_CODE(unpack_dyn_seq_of(c.get<cfg_li_nom_s>(), bref, 1, 15));
        break;
    case types::cfg_li_dl_fr_sp:
        HANDLE_CODE(unpack_dyn_seq_of(c.get<cfg_li_dl_fr_sp_s>(), bref, 1, 15));
        break;
    default:
        log_invalid_choice_id(type_, "meas_gap_cfg_wx_c_");
        return SRSASN_ERROR_DECODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
  void sib_wx_s::beam_resel_rel_info_s::meas_gap_cfg_wx_c_::to_json(json_writer & j) const
  {
  j.start_obj();
  switch (type_) {
    case types::cfg_li_nom:
        j.start_array("measGapConfigList-Normal");
        for (const auto&e1:c.get<cfg_li_nom_s>() )
        {
          e1.to_json(j);
        }
        j.end_array();
        break;
    case types::cfg_li_dl_fr_sp:
        j.start_array("measGapConfigList-DIFreqSpread");
        for (const auto& e1 : c.get<cfg_li_dl_fr_sp_s>()) {
          e1.to_json(j
          );
        }
        break;
    default:
        log_invalid_choice_id(type_, "meas_gap_cfg_wx_c_");
  }
  j.end_obj();
  }




  // UAC-BarringPerCat ::= SEQUENCE
  SRSASN_CODE uac_barr_per_cat_s::pack(bit_ref& bref)const
  {
  bref.pack(ext, 1);
  HANDLE_CODE(pack_integer(bref, access_category, (uint8_t)1u, (uint8_t)8u));
  HANDLE_CODE(pack_integer(bref, uac_barr_info_set_idx, (uint8_t)1u, (uint8_t)8u));
  return SRSASN_SUCCESS;
  }
  SRSASN_CODE uac_barr_per_cat_s::unpack(cbit_ref& bref)
  {
  bref.unpack(ext, 1);
  HANDLE_CODE(unpack_integer(access_category, bref, (uint8_t)1u, (uint8_t)8u));
  HANDLE_CODE(unpack_integer(uac_barr_info_set_idx,bref, (uint8_t)1u, (uint8_t)8u));
  return SRSASN_SUCCESS;
  }
  void uac_barr_per_cat_s::to_json(json_writer& j)const 
  {
  j.start_obj();
  j.write_int("accessCategory", access_category);
  j.write_int("uac-barringInfoSetIndex", uac_barr_info_set_idx);
  j.end_obj();
  }

  // UAC-BarringInfoSet ::= SEQUENCE
  SRSASN_CODE uac_barr_info_set_s::pack(bit_ref& bref) const 
  {
  bref.pack(ext, 1);
  HANDLE_CODE(uac_barr_factor.pack(bref));
  HANDLE_CODE(uac_barr_time.pack(bref));
  HANDLE_CODE(uac_barr_for_access_id.pack(bref));
  return SRSASN_SUCCESS;
  }
  SRSASN_CODE uac_barr_info_set_s::unpack(cbit_ref& bref)
  {
  bref.unpack(ext, 1);
  HANDLE_CODE(uac_barr_factor.unpack(bref));
  HANDLE_CODE(uac_barr_time.unpack(bref));
  HANDLE_CODE(uac_barr_for_access_id.unpack(bref));
  return SRSASN_SUCCESS;
  }
  void uac_barr_info_set_s::to_json(json_writer& j)const 
  {
  j.start_obj();
  j.write_str("uac-BarringFactor", uac_barr_factor.to_string());
  j.write_str("uac-BarringTime", uac_barr_time.to_string());
  j.write_str("uac-BarringForAccessIdentity", uac_barr_for_access_id.to_string());
  j.end_obj();
  }

  // UAC-BarringInfo ::= SEQUENCE
  SRSASN_CODE uac_barr_info_s::pack(bit_ref& bref) const 
  {
  HANDLE_CODE(bref.pack(uac_barr_for_common_present, 1));
  if (uac_barr_for_common_present)
  {
    HANDLE_CODE(pack_dyn_seq_of(bref, uac_barr_for_common, 1, 8));
  }
  HANDLE_CODE(pack_dyn_seq_of(bref, uac_barr_info_set_list, 1, 8));
  return SRSASN_SUCCESS;
  }
  SRSASN_CODE uac_barr_info_s::unpack(cbit_ref& bref) 
  {
  HANDLE_CODE(bref.unpack(uac_barr_for_common_present, 1));
  if (uac_barr_for_common_present) 
  {
    HANDLE_CODE(unpack_dyn_seq_of(uac_barr_for_common, bref,1, 8));
  }
  HANDLE_CODE(unpack_dyn_seq_of(uac_barr_info_set_list,bref, 1, 8));
  return SRSASN_SUCCESS;
  }
  void uac_barr_info_s::to_json(json_writer& j)const 
  {
  j.start_obj();
  if (uac_barr_for_common_present) 
  {
    j.start_array("uac-BarringForCommon");
    for (uint32_t i1 = 0; i1 < uac_barr_for_common.size(); i1++) 
    {
        uac_barr_for_common[i1].to_json(j);
    }
    j.end_array();
  }
  j.start_array("uac-BarringInfoSetList");
  for (uint32_t i1 = 0; i1 < uac_barr_info_set_list.size(); i1++) {
    uac_barr_info_set_list[i1].to_json(j);
  }
  j.end_array();
  j.end_obj();
  }



  // RACH-ConfigCommon ::= SEQUENCE
  SRSASN_CODE rach_cfg_com_n_s::pack(bit_ref& bref)const 
  {
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(freqBitmap_FrameFull_present, 1));
  HANDLE_CODE(band_id.pack(bref));
  HANDLE_CODE(freq_bit_map.pack(bref));
  HANDLE_CODE(rach_frame_ass.pack(bref));
  HANDLE_CODE(rach_slot_ass.pack(bref));
  HANDLE_CODE(ra_res_wi_si.pack(bref));
  if (freqBitmap_FrameFull_present)
  {
    HANDLE_CODE(freqBitmap_FrameFull.pack(bref));
  }
  return SRSASN_SUCCESS;
  }
  SRSASN_CODE rach_cfg_com_n_s::unpack(cbit_ref& bref)
  {
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(freqBitmap_FrameFull_present, 1));
  HANDLE_CODE(band_id.unpack(bref));
  HANDLE_CODE(freq_bit_map.unpack(bref));
  HANDLE_CODE(rach_frame_ass.unpack(bref));
  HANDLE_CODE(rach_slot_ass.unpack(bref));
  HANDLE_CODE(ra_res_wi_si.unpack(bref));
  if (freqBitmap_FrameFull_present) {
    HANDLE_CODE(freqBitmap_FrameFull.unpack(bref));
  }
  return SRSASN_SUCCESS;
  }
  void rach_cfg_com_n_s::to_json(json_writer& j)const
  {
  j.start_obj();
  j.write_fieldname("band-Identity");
  band_id.to_json(j);
  j.write_str("freq-Bitmap", freq_bit_map.to_string());
  j.write_str("rach-FrameAssignment", rach_frame_ass.to_string());
  j.write_str("rach-slotAssignment", rach_slot_ass.to_string());
  j.write_str("ra-ResponseWindowSize", ra_res_wi_si.to_string());
  if (freqBitmap_FrameFull_present) {
    j.write_str("freqBitmap-FrameFull", freqBitmap_FrameFull.to_string());
  }
  j.end_obj();
  }

  // AGCH-ConfigCommon ::= SEQUENCE
  SRSASN_CODE agch_cfg_com_s::pack(bit_ref& bref)const
  {
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(band_id_present, 1));
  HANDLE_CODE(bref.pack(freq_id_present, 1));
  HANDLE_CODE(bref.pack(agch_slot_start_present, 1));
  if (band_id_present) 
  {
    HANDLE_CODE(band_id.pack(bref));
  }
  if (freq_id_present) 
  {
    HANDLE_CODE(freq_id.pack(bref));
  }

  HANDLE_CODE(agch_fram_ass.pack(bref));

  if (agch_slot_start_present)
  {
    HANDLE_CODE(agch_slot_start.pack(bref));
  }
  return SRSASN_SUCCESS;
  }
  SRSASN_CODE agch_cfg_com_s::unpack(cbit_ref& bref) 
  {
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(band_id_present, 1));
  HANDLE_CODE(bref.unpack(freq_id_present, 1));
  HANDLE_CODE(bref.unpack(agch_slot_start_present, 1));
  if (band_id_present) {
    HANDLE_CODE(band_id.unpack(bref));
  }
  if (freq_id_present) {
    HANDLE_CODE(freq_id.unpack(bref));
  }

  HANDLE_CODE(agch_fram_ass.unpack(bref));

  if (agch_slot_start_present) {
    HANDLE_CODE(agch_slot_start.unpack(bref));
  }
  return SRSASN_SUCCESS;
  }
  void agch_cfg_com_s::to_json(json_writer& j)const
  {
  j.start_obj();
  j.start_obj();
  if (band_id_present)
  {
    j.write_fieldname("band-Identity");
    band_id.to_json(j);
  }
  j.end_obj();
  j.start_obj();
  if (freq_id_present) 
  {
    j.write_fieldname("frequency-Identity");
    freq_id.to_json(j);
  }
  j.end_obj();
  j.write_str("agch-FrameAssignment", agch_fram_ass.to_string());
  j.start_obj();
  if (agch_slot_start_present) 
  {
    j.write_str("agch-slotStart", agch_slot_start.to_string());
  }
  j.end_obj();
  j.end_obj();
  }

  // PowerControl-ConfigCommon ::= SEQUENCE
  SRSASN_CODE power_ctrl_cfg_com_s::pack(bit_ref& bref) const 
  {
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(exp_rx_p_prach_present, 1));
  HANDLE_CODE(bref.pack(exp_rx_p_psych_present, 1));
  HANDLE_CODE(bref.pack(exp_rx_p_pdch1_1_present, 1));
  HANDLE_CODE(bref.pack(exp_rx_p_pdch1_2_present, 1));
  HANDLE_CODE(bref.pack(exp_rx_p_psch1_1_present, 1));
  HANDLE_CODE(bref.pack(exp_rx_p_psch1_2_present, 1));
  HANDLE_CODE(bref.pack(exp_rx_p_psch5_1_present, 1));
  HANDLE_CODE(bref.pack(exp_rx_p_psch5_2_present, 1));
  HANDLE_CODE(bref.pack(exp_rx_p_ptuch_present, 1));

  HANDLE_CODE(pmbch_tx_p.pack(bref));
  if (exp_rx_p_prach_present)
  {
    HANDLE_CODE(exp_rx_p_prach.pack(bref));
  }
  if (exp_rx_p_psych_present) 
  {
    HANDLE_CODE(exp_rx_p_psych.pack(bref));
  }
  if (exp_rx_p_pdch1_1_present) 
  {
    HANDLE_CODE(exp_rx_p_pdch1_1.pack(bref));
  }
  if (exp_rx_p_pdch1_2_present) 
  {
    HANDLE_CODE(exp_rx_p_pdch1_2.pack(bref));
  }
  if (exp_rx_p_psch1_1_present) 
  {
    HANDLE_CODE(exp_rx_p_psch1_1.pack(bref));
  }
  if (exp_rx_p_psch1_2_present)
  {
    HANDLE_CODE(exp_rx_p_psch1_2.pack(bref));
  }
  if (exp_rx_p_psch5_1_present)
  {
    HANDLE_CODE(exp_rx_p_psch5_1.pack(bref));
  }
  if (exp_rx_p_psch5_2_present)
  {
    HANDLE_CODE(exp_rx_p_psch5_2.pack(bref));
  }
  if (exp_rx_p_ptuch_present)
  {
    HANDLE_CODE(exp_rx_p_ptuch.pack(bref));
  }
  return SRSASN_SUCCESS;
  }
  SRSASN_CODE power_ctrl_cfg_com_s::unpack(cbit_ref& bref) 
  {
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(exp_rx_p_prach_present, 1));
  HANDLE_CODE(bref.unpack(exp_rx_p_psych_present, 1));
  HANDLE_CODE(bref.unpack(exp_rx_p_pdch1_1_present, 1));
  HANDLE_CODE(bref.unpack(exp_rx_p_pdch1_2_present, 1));
  HANDLE_CODE(bref.unpack(exp_rx_p_psch1_1_present, 1));
  HANDLE_CODE(bref.unpack(exp_rx_p_psch1_2_present, 1));
  HANDLE_CODE(bref.unpack(exp_rx_p_psch5_1_present, 1));
  HANDLE_CODE(bref.unpack(exp_rx_p_psch5_2_present, 1));
  HANDLE_CODE(bref.unpack(exp_rx_p_ptuch_present, 1));

  HANDLE_CODE(pmbch_tx_p.unpack(bref));
  if (exp_rx_p_prach_present) {
    HANDLE_CODE(exp_rx_p_prach.unpack(bref));
  }
  if (exp_rx_p_psych_present) {
    HANDLE_CODE(exp_rx_p_psych.unpack(bref));
  }
  if (exp_rx_p_pdch1_1_present) {
    HANDLE_CODE(exp_rx_p_pdch1_1.unpack(bref));
  }
  if (exp_rx_p_pdch1_2_present) {
    HANDLE_CODE(exp_rx_p_pdch1_2.unpack(bref));
  }
  if (exp_rx_p_psch1_1_present) {
    HANDLE_CODE(exp_rx_p_psch1_1.unpack(bref));
  }
  if (exp_rx_p_psch1_2_present) {
    HANDLE_CODE(exp_rx_p_psch1_2.unpack(bref));
  }
  if (exp_rx_p_psch5_1_present) {
    HANDLE_CODE(exp_rx_p_psch5_1.unpack(bref));
  }
  if (exp_rx_p_psch5_2_present) {
    HANDLE_CODE(exp_rx_p_psch5_2.unpack(bref));
  }
  if (exp_rx_p_ptuch_present) {
    HANDLE_CODE(exp_rx_p_ptuch.unpack(bref));
  }
  return SRSASN_SUCCESS;
  }
  void power_ctrl_cfg_com_s::to_json(json_writer& j)const 
  {
  j.start_obj();
  j.write_str("pmbchRxPower", pmbch_tx_p.to_string());
  j.start_obj();
  if (exp_rx_p_prach_present)
  {
    j.write_fieldname("expectedRxPower-PRACH");
    exp_rx_p_prach.to_json(j);
  }
  j.end_obj();
  j.start_obj();
  if (exp_rx_p_psych_present)
  {
    j.write_fieldname("expectedRxPower-PSYCH");
    exp_rx_p_psych.to_json(j);
  }
  j.end_obj();
  j.start_obj();
  if (exp_rx_p_pdch1_1_present) {
    j.write_fieldname("expectedRxPower-PDCH1-1");
    exp_rx_p_pdch1_1.to_json(j);
  }
  j.end_obj();
  j.start_obj();
  if (exp_rx_p_pdch1_2_present) {
    j.write_fieldname("expectedRxPower-PDCH1-2");
    exp_rx_p_pdch1_2.to_json(j);
  }
  j.end_obj();
  j.start_obj();
  if (exp_rx_p_psch1_1_present) {
    j.write_fieldname("expectedRxPower-PSCH1-1");
    exp_rx_p_psch1_1.to_json(j);
  }
  j.end_obj();
  j.start_obj();
  if (exp_rx_p_psch1_2_present) {
    j.write_fieldname("expectedRxPower-PSCH1-2");
    exp_rx_p_psch1_2.to_json(j);
  }
  j.end_obj();
  j.start_obj();
  if (exp_rx_p_psch5_1_present) {
    j.write_fieldname("expectedRxPower-PSCH5-1");
    exp_rx_p_psch5_1.to_json(j);
  }
  j.end_obj();
  j.start_obj();
  if (exp_rx_p_psch5_2_present) {
    j.write_fieldname("expectedRxPower-PSCH5-2");
    exp_rx_p_psch5_2.to_json(j);
  }
  j.end_obj();
  j.start_obj();
  if (exp_rx_p_ptuch_present) {
    j.write_fieldname("expectedRxPower-PTUCH");
    exp_rx_p_ptuch.to_json(j);
  }
  j.end_obj();
  j.end_obj();
  }

  // BBCH-ConfigCommon ::= SEQUENCE
  SRSASN_CODE bbch_cfg_com_s::pack(bit_ref& bref)const 
  {
  bref.pack(ext, 1);
  HANDLE_CODE(band_id.pack(bref));
  HANDLE_CODE(freq_id.pack(bref));
  HANDLE_CODE(bbch_frame_ass.pack(bref));
  HANDLE_CODE(bbch_slot_ass.pack(bref));
  return SRSASN_SUCCESS;
  }
  SRSASN_CODE bbch_cfg_com_s::unpack(cbit_ref& bref) 
  {
  bref.unpack(ext, 1);
  HANDLE_CODE(band_id.unpack(bref));
  HANDLE_CODE(freq_id.unpack(bref));
  HANDLE_CODE(bbch_frame_ass.unpack(bref));
  HANDLE_CODE(bbch_slot_ass.unpack(bref));
  return SRSASN_SUCCESS;
  }
  void bbch_cfg_com_s::to_json(json_writer& j)const
  {
  j.start_obj();
  j.write_fieldname("band-Identity");
  band_id.to_json(j);
  j.write_fieldname("Frequency-Identity");
  freq_id.to_json(j);
  j.write_str("bbch-FrameAssignment", bbch_frame_ass.to_string());
  j.write_str("bbch-slotAssignment", bbch_slot_ass.to_string());
  j.end_obj();
  }

  // IoTSI-Config-Normal ::= SEQUENCE
  SRSASN_CODE iotsi_cfg_normal_s::pack(bit_ref& bref) const
  {
  HANDLE_CODE(dl_band_id.pack(bref));
  HANDLE_CODE(dl_freq_id.pack(bref));
  HANDLE_CODE(fn_ass.pack(bref));
  return SRSASN_SUCCESS;
  }
  SRSASN_CODE iotsi_cfg_normal_s::unpack(cbit_ref& bref) 
  {
  HANDLE_CODE(dl_band_id.unpack(bref));
  HANDLE_CODE(dl_freq_id.unpack(bref));
  HANDLE_CODE(fn_ass.unpack(bref));
  return SRSASN_SUCCESS;
  }
  void iotsi_cfg_normal_s::to_json(json_writer& j)const
  {
  j.start_obj();
  j.write_fieldname("dl-BandIdentity");
  dl_band_id.to_json(j);
  j.write_fieldname("dl-FrequencyIdentity");
  dl_freq_id.to_json(j);
  j.write_str("fn-Assignment", fn_ass.to_string());
  j.end_obj();
  }

  // IoTSI-Config-DlFreqSpread ::= SEQUENCE
  SRSASN_CODE iotsi_cfg_dl_freq_spread_s::pack(bit_ref& bref) const
  {
  HANDLE_CODE(spread_factor.pack(bref));
  HANDLE_CODE(code_index.pack(bref));
  HANDLE_CODE(fn_ass.pack(bref));
  return SRSASN_SUCCESS;
  }
  SRSASN_CODE iotsi_cfg_dl_freq_spread_s::unpack(cbit_ref& bref) 
  {
  HANDLE_CODE(spread_factor.unpack(bref));
  HANDLE_CODE(code_index.unpack(bref));
  HANDLE_CODE(fn_ass.unpack(bref));
  return SRSASN_SUCCESS;
  }
  void iotsi_cfg_dl_freq_spread_s::to_json(json_writer& j)const 
  {
  j.start_obj();
  j.write_str("spread_factor", spread_factor.to_string());
  j.write_str("code_Index", code_index.to_string());
  j.write_str("fn_Assignment", fn_ass.to_string());
  j.end_obj();
  }

  // IoTSI-Config ::= CHOICE
  void iotsi_cfg_c::destroy_() 
  {
  switch (type_) 
  {
    case types::iotsi_cfg_normal:
        c.destroy<iotsi_cfg_normal_s>();
        break;
    case types::iotsi_cfg_dl_freq_spread:
        c.destroy<iotsi_cfg_dl_freq_spread_s>();
        break;
    default:
        break;
  }
  }

  void iotsi_cfg_c::set(types::options e)
  {
  destroy_();
  type_ = e;
  switch (type_) {
    case types::iotsi_cfg_normal:
        c.init<iotsi_cfg_normal_s>();
        break;
    case types::iotsi_cfg_dl_freq_spread:
        c.init<iotsi_cfg_dl_freq_spread_s>();
        break;
    case types::nulltype: 
        break;
    default:
        log_invalid_choice_id(type_, "iotsi_cfg_c");
  }
  }

  iotsi_cfg_c::iotsi_cfg_c(const iotsi_cfg_c& other) 
  {
  type_ = other.type();
  switch (type_) {
    case types::iotsi_cfg_normal:
        c.init(other.c.get<iotsi_cfg_normal_s>());
        break;
    case types::iotsi_cfg_dl_freq_spread:
        c.init(other.c.get<iotsi_cfg_dl_freq_spread_s>());
        break;
    case types::nulltype: 
        break;
    default:
        log_invalid_choice_id(type_, "iotsi_cfg_c");
  }
  }

  iotsi_cfg_c& iotsi_cfg_c::operator=(const iotsi_cfg_c& other)
  {
  if (this==&other) {
    return *this;
  }
  set(other.type());
  switch (type_) {
    case types::iotsi_cfg_normal:
        c.set(other.c.get<iotsi_cfg_normal_s>());
        break;
    case types::iotsi_cfg_dl_freq_spread:
        c.set(other.c.get<iotsi_cfg_dl_freq_spread_s>());
        break;
    case types::nulltype:
        break;
    default:
        log_invalid_choice_id(type_, "iotsi_cfg_c");
  }
  return *this;
  }

  iotsi_cfg_normal_s& iotsi_cfg_c::set_iotsi_cfg_normal()
  {
  set(types::iotsi_cfg_normal);
  return c.get<iotsi_cfg_normal_s>();
  }
  iotsi_cfg_dl_freq_spread_s& iotsi_cfg_c::set_iotsi_cfg_dl_freq_spread()
  {
  set(types::iotsi_cfg_dl_freq_spread);
  return c.get<iotsi_cfg_dl_freq_spread_s>();
  }

  void iotsi_cfg_c::to_json(json_writer& j) const
  {
  j.start_obj();
  switch (type_) {
    case types::iotsi_cfg_normal:
        j.write_fieldname("ioTSI-Config-Normal");
        c.get<iotsi_cfg_normal_s>().to_json(j);
        break;
    case types::iotsi_cfg_dl_freq_spread:
        j.write_fieldname("ioTSI-Config-DIFreqSpread");
        c.get<iotsi_cfg_dl_freq_spread_s>().to_json(j);
        break;
    default:
        log_invalid_choice_id(type_, "iotsi_cfg_c");
  }
  j.end_obj();
  }

  SRSASN_CODE iotsi_cfg_c::pack(bit_ref& bref) const
  {
  type_.pack(bref);
  switch (type_) {
    case types::iotsi_cfg_normal:
        HANDLE_CODE(c.get<iotsi_cfg_normal_s>().pack(bref));
        break;
    case types::iotsi_cfg_dl_freq_spread:
        HANDLE_CODE(c.get<iotsi_cfg_dl_freq_spread_s>().pack(bref));
        break;
    default:
        log_invalid_choice_id(type_, "iotsi_cfg_c");
        return SRSASN_ERROR_ENCODE_FAIL;
  }
  return SRSASN_SUCCESS;
  }

  SRSASN_CODE iotsi_cfg_c::unpack(cbit_ref& bref)
  {
  types e;
  e.unpack(bref);
  set(e);
  switch (type_) {
    case types::iotsi_cfg_normal:
        HANDLE_CODE(c.get<iotsi_cfg_normal_s>().unpack(bref));
        break;
    case types::iotsi_cfg_dl_freq_spread:
        HANDLE_CODE(c.get<iotsi_cfg_dl_freq_spread_s>().unpack(bref));
        break;
    default:
        log_invalid_choice_id(type_, "iotsi_cfg_c");
        return SRSASN_ERROR_DECODE_FAIL;
  }
  return SRSASN_SUCCESS;
  }


  // RadioResourceConfigCommonSIB ::= SEQUENCE
  SRSASN_CODE rr_cfg_com_sib_s::pack(bit_ref& bref) const
  {
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(bbch_cfg_com_present, 1));
  HANDLE_CODE(bref.pack(iotsi_cfg_present, 1));

  HANDLE_CODE(rach_cfg_com.pack(bref));
  HANDLE_CODE(agch_cfg_com.pack(bref));
  HANDLE_CODE(power_ctrl_cfg_com.pack(bref));
  if (bbch_cfg_com_present) 
  {
    HANDLE_CODE(bbch_cfg_com.pack(bref));
  }
  if (iotsi_cfg_present) 
  {
    HANDLE_CODE(iotsi_cfg.pack(bref));
  }
  return SRSASN_SUCCESS;
  }
  SRSASN_CODE rr_cfg_com_sib_s::unpack(cbit_ref& bref)
  {
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(bbch_cfg_com_present, 1));
  HANDLE_CODE(bref.unpack(iotsi_cfg_present, 1));

  HANDLE_CODE(rach_cfg_com.unpack(bref));
  HANDLE_CODE(agch_cfg_com.unpack(bref));
  HANDLE_CODE(power_ctrl_cfg_com.unpack(bref));
  if (bbch_cfg_com_present)
  {
    HANDLE_CODE(bbch_cfg_com.unpack(bref));
  }
  if (iotsi_cfg_present) 
  {
    HANDLE_CODE(iotsi_cfg.unpack(bref));
  }
  return SRSASN_SUCCESS;
  }
  void rr_cfg_com_sib_s::to_json(json_writer& j)const 
  {
  j.start_obj();
  j.write_fieldname("rach-ConfigCommon");
  rach_cfg_com.to_json(j);
  j.write_fieldname("agch-ConfigCommon");
  agch_cfg_com.to_json(j);
  j.write_fieldname("powerControl-ConfigCommon");
  power_ctrl_cfg_com.to_json(j);
  if (bbch_cfg_com_present)
  {
    j.write_fieldname("bbch-ConfigCommon");
    bbch_cfg_com.to_json(j);
  }
  if (iotsi_cfg_present)
  {
    j.write_fieldname("iotsi-Config");
    iotsi_cfg.to_json(j);
  }
  j.end_obj();
  }

  // UE-TimersAndConstants ::= SEQUENCE
  SRSASN_CODE ue_timer_and_constant_s::pack(bit_ref& bref)const 
  {
  bref.pack(ext, 1);
  HANDLE_CODE(t300.pack(bref));
  HANDLE_CODE(t301.pack(bref));
  HANDLE_CODE(t302.pack(bref));
  HANDLE_CODE(t310.pack(bref));
  HANDLE_CODE(n310.pack(bref));
  HANDLE_CODE(t311.pack(bref));
  HANDLE_CODE(n311.pack(bref));
  return SRSASN_SUCCESS;
  }
  SRSASN_CODE ue_timer_and_constant_s::unpack(cbit_ref& bref) 
  {
  bref.unpack(ext, 1);
  HANDLE_CODE(t300.unpack(bref));
  HANDLE_CODE(t301.unpack(bref));
  HANDLE_CODE(t302.unpack(bref));
  HANDLE_CODE(t310.unpack(bref));
  HANDLE_CODE(n310.unpack(bref));
  HANDLE_CODE(t311.unpack(bref));
  HANDLE_CODE(n311.unpack(bref));
  return SRSASN_SUCCESS;
  }
  void ue_timer_and_constant_s::to_json(json_writer& j)const
  {
  j.start_obj();
  j.write_str("t300", t300.to_string());
  j.write_str("t301", t301.to_string());
  j.write_str("t302", t302.to_string());
  j.write_str("t310", t310.to_string());
  j.write_str("n310", n310.to_string());
  j.write_str("t311", t311.to_string());
  j.write_str("n311", n311.to_string());
  j.end_obj();
  }

  // GeoInfoUpdateParameters ::=	SEQUENCE
  SRSASN_CODE geo_info_update_para_s::pack(bit_ref& bref)const 
  {
  bref.pack(ext, 1);
  HANDLE_CODE(update_timer.pack(bref));
  HANDLE_CODE(update_distance.pack(bref));
  return SRSASN_SUCCESS;
  }
  SRSASN_CODE geo_info_update_para_s::unpack(cbit_ref& bref) 
  {
  bref.unpack(ext, 1);
  HANDLE_CODE(update_timer.unpack(bref));
  HANDLE_CODE(update_distance.unpack(bref));
  return SRSASN_SUCCESS;
  }
  void geo_info_update_para_s::to_json(json_writer& j) const 
  {
  j.start_obj();
  j.write_str("Update-Timer", update_timer.to_string());
  j.write_str("Update-Distance", update_distance.to_string());
  j.end_obj();
  }

  //EphemerisParametes
  SRSASN_CODE ephemerise_param_s::pack(bit_ref& bref)const 
  {
      bref.pack(ext, 1);
      HANDLE_CODE(pack_integer(bref, toe, (uint16_t)0u, (uint16_t)10079u));
      HANDLE_CODE(pack_integer(bref, fitArc_Len, (uint8_t)0u, (uint8_t)1u));
      HANDLE_CODE(pack_integer(bref, sate_ephem_semi_major_axis_off, (int32_t)-4194304, (int32_t)4194304));
      HANDLE_CODE(pack_integer(bref, sate_ephem_eccen_e, (uint32_t)0u, (uint32_t)268435456u));
      HANDLE_CODE(pack_integer(bref, sate_ephem_inc_off_i, (int32_t)-67108864, (int32_t)67108864));
      HANDLE_CODE(pack_integer(bref, sate_ephem_rate_right_node, (int32_t)-4194304, (int32_t)4194304));
      HANDLE_CODE(pack_integer(bref, sate_ephem_argu_of_peria, (int64_t)-2147483648, (int64_t)2147483648));
      HANDLE_CODE(pack_integer(bref, sate_ephem_long_of_ascen_node, (int64_t)-2147483648, (int64_t)2147483648));
      HANDLE_CODE(pack_integer(bref, sate_ephem_mean_ano_m, (int64_t)-2147483648, (int64_t)2147483648));
      //HANDLE_CODE(pack_integer(bref, n_date, (uint16_t)1u, (uint16_t)16383u));
      //HANDLE_CODE(pack_integer(bref, n_time, (uint32_t)0u, (uint32_t)86400u));
      return SRSASN_SUCCESS;
  }
  SRSASN_CODE ephemerise_param_s::unpack(cbit_ref& bref)
  {
      bref.unpack(ext, 1);
      HANDLE_CODE(unpack_integer(toe, bref, (uint16_t)0u, (uint16_t)10079u));
      HANDLE_CODE(unpack_integer(fitArc_Len,bref, (uint8_t)0u, (uint8_t)1u));
      HANDLE_CODE(unpack_integer(sate_ephem_semi_major_axis_off, bref, (int32_t)-4194304, (int32_t)4194304));
      HANDLE_CODE(unpack_integer(sate_ephem_eccen_e, bref, (uint32_t)0u, (uint32_t)268435456u));
      HANDLE_CODE(unpack_integer(sate_ephem_inc_off_i, bref, (int32_t)-67108864, (int32_t)67108864));
      HANDLE_CODE(unpack_integer(sate_ephem_rate_right_node,bref, (int32_t)-4194304, (int32_t)4194304));
      HANDLE_CODE(unpack_integer(sate_ephem_argu_of_peria, bref, (int64_t)-2147483648, (int64_t)2147483648));
      HANDLE_CODE(unpack_integer(sate_ephem_long_of_ascen_node, bref, (int64_t)-2147483648, (int64_t)2147483648));
      HANDLE_CODE(unpack_integer(sate_ephem_mean_ano_m, bref, (int64_t)-2147483648, (int64_t)2147483648));
      //HANDLE_CODE(unpack_integer(n_date, bref, (uint16_t)1u, (uint16_t)16383u));
      //HANDLE_CODE(unpack_integer(n_time, bref, (uint32_t)0u, (uint32_t)86400u));
      return SRSASN_SUCCESS;
  }
  void ephemerise_param_s::to_json(json_writer& j)const 
  {
  j.start_obj();
  j.write_int("toe", toe);
  j.write_int("fitArcLength", fitArc_Len);
  j.write_int("satelliteEphemerisSemiMajorAxisOffest", sate_ephem_semi_major_axis_off);
  j.write_int("satelliteEphemerisEccentricityE", sate_ephem_eccen_e);
  j.write_int("satelliteEphemerisInclinationOffsetI", sate_ephem_inc_off_i);
  j.write_int("satelliteEphemerisRateOfRightAscendingNode", sate_ephem_rate_right_node);
  j.write_int("satelliteEphemerisArgumentOfPeriapsis", sate_ephem_argu_of_peria);
  j.write_int("satelliteEphemerisLongitudeOfAscendingNode",sate_ephem_long_of_ascen_node);
  j.write_int("satelliteEphemerisMeanAnomalyM", sate_ephem_mean_ano_m);
  //j.write_int("nDate", n_date);
  //j.write_int("nTime", n_time);
  j.end_obj();
  }







  //mib_wx_s\CF\FBϢ\B5\C4pack
  SRSASN_CODE mib_wx_s ::pack(bit_ref& bref) const
  {
      HANDLE_CODE(beam_id.pack(bref));
      HANDLE_CODE(bcch_band_id.pack(bref));
      HANDLE_CODE(bcch_fre_id.pack(bref));
      HANDLE_CODE(bcch_slot_start.pack(bref));
      HANDLE_CODE(frame_off.pack(bref));
      HANDLE_CODE(sys_info_ver_tag.pack(bref));
      HANDLE_CODE(sys_sh_fra_num.pack(bref));
      HANDLE_CODE(d_to_beam_center.pack(bref));
      HANDLE_CODE(meas_normal_ra.pack(bref));
      HANDLE_CODE(bref.pack(hot_info_indication,1));
      HANDLE_CODE(pcch_band_id.pack(bref));
      HANDLE_CODE(pcch_fre_id.pack(bref));
      HANDLE_CODE(naviInfo_band_id.pack(bref));
      HANDLE_CODE(naviInfo_fre_id.pack(bref));
      HANDLE_CODE(naviInfo_slot_ass.pack(bref));
      HANDLE_CODE(mib_Re_Fra_num.pack(bref));
      HANDLE_CODE(spare.pack(bref));
      return SRSASN_SUCCESS;
  }
  SRSASN_CODE mib_wx_s::unpack(cbit_ref& bref)
  {
      HANDLE_CODE(beam_id.unpack(bref));
      HANDLE_CODE(bcch_band_id.unpack(bref));
      HANDLE_CODE(bcch_fre_id.unpack(bref));
      HANDLE_CODE(bcch_slot_start.unpack(bref));
      HANDLE_CODE(frame_off.unpack(bref));
      HANDLE_CODE(sys_info_ver_tag.unpack(bref));
      HANDLE_CODE(sys_sh_fra_num.unpack(bref));
      HANDLE_CODE(d_to_beam_center.unpack(bref));
      HANDLE_CODE(meas_normal_ra.unpack(bref));
      HANDLE_CODE(bref.unpack(hot_info_indication, 1));
      HANDLE_CODE(pcch_band_id.unpack(bref));
      HANDLE_CODE(pcch_fre_id.unpack(bref));
      HANDLE_CODE(naviInfo_band_id.unpack(bref));
      HANDLE_CODE(naviInfo_fre_id.unpack(bref));
      HANDLE_CODE(naviInfo_slot_ass.unpack(bref));
      HANDLE_CODE(mib_Re_Fra_num.unpack(bref));
      HANDLE_CODE(spare.unpack(bref));
      return SRSASN_SUCCESS;
  }
  void mib_wx_s::to_json(json_writer& j) const
  {
      j.start_obj();
      j.write_fieldname("beamIdentity");
      beam_id.to_json(j);
      j.write_fieldname("bcch-bandIdentity");
      bcch_band_id.to_json(j);
      j.write_fieldname("bcch-frequencyIdentity");
      bcch_fre_id.to_json(j);
      j.write_str("bcch_slotStart", bcch_slot_start.to_string());
      j.write_str("frameOffset", frame_off.to_string());
      j.write_str("systemInfoVersionTag", sys_info_ver_tag.to_string());
      j.write_str("systemSHFrameNumber", sys_sh_fra_num.to_string());
      j.write_str("distanceToBeamCenter", d_to_beam_center.to_string());
      j.write_str("measNormalizationRation", meas_normal_ra.to_string());
      j.write_bool("hotSpotInfo-Indication", hot_info_indication);
      j.write_fieldname("pcch-bandIdentity");
      pcch_band_id.to_json(j);
      j.write_fieldname("pcch-frequencyIdentity");
      pcch_fre_id.to_json(j);
      j.write_fieldname("naviInfo-bandIdentity");
      naviInfo_band_id.to_json(j);
      j.write_fieldname("naviInfo-frequencyIdentity");
      naviInfo_fre_id.to_json(j);
      j.write_str("naviInfo-slotAssignment", naviInfo_slot_ass.to_string());
      j.write_str("mibRelativeFrameNum", mib_Re_Fra_num.to_string());
      j.write_str("spare", spare.to_string());
      j.end_obj();
  }




//sib_wx_s\CF\FBϢ\B5\C4pack
SRSASN_CODE sib_wx_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(uac_barring_info_present, 1));
  HANDLE_CODE(bref.pack(p_max_n_present, 1));

  HANDLE_CODE(pack_dyn_seq_of(bref,beam_acc_rel_info.plmn_id_li,1,4));
  HANDLE_CODE(beam_acc_rel_info.beam_barred.pack(bref));

  bref.pack(beam_sel_rel_info.ext, 1);
  HANDLE_CODE(beam_sel_rel_info.q_rx_lev_min.pack(bref));

  HANDLE_CODE(beam_resel_rel_info.threshold_resel.pack(bref));
  HANDLE_CODE(beam_resel_rel_info.q_rx_lev_min.pack(bref));
  HANDLE_CODE(pack_integer(bref, beam_resel_rel_info.offset, (int8_t)-9, (int8_t)9));
  HANDLE_CODE(pack_integer(bref, beam_resel_rel_info.hysteresis, (uint8_t)0u, (uint8_t)7u));
  HANDLE_CODE(pack_integer(bref, beam_resel_rel_info.t_resel, (uint8_t)0u, (uint8_t)7u));
  HANDLE_CODE(beam_resel_rel_info.meas_gap_cfg_wx.pack(bref));

  if (uac_barring_info_present)
  {
    HANDLE_CODE(uac_barring_info.pack(bref));
  }

  HANDLE_CODE(rr_cfg_com.pack(bref));

  HANDLE_CODE(ue_timer_and_constant.pack(bref));

  if (p_max_n_present)
  {
    HANDLE_CODE(pack_integer(bref, p_max_n, (uint8_t)0u, (uint8_t)15u));
  }

  HANDLE_CODE(geo_info_update_para.pack(bref));

  HANDLE_CODE(eph_para_ser_sat.pack(bref));
}

// sib_wx_s\CF\FBϢ\B5\C4unpack
SRSASN_CODE sib_wx_s::unpack(cbit_ref& bref) 
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(uac_barring_info_present, 1));
  HANDLE_CODE(bref.unpack(p_max_n_present, 1));

  HANDLE_CODE(unpack_dyn_seq_of(beam_acc_rel_info.plmn_id_li, bref, 1, 4));
  HANDLE_CODE(beam_acc_rel_info.beam_barred.unpack(bref));

  bref.unpack(beam_sel_rel_info.ext, 1);
  HANDLE_CODE(beam_sel_rel_info.q_rx_lev_min.unpack(bref));

  HANDLE_CODE(beam_resel_rel_info.threshold_resel.unpack(bref));
  HANDLE_CODE(beam_resel_rel_info.q_rx_lev_min.unpack(bref));
  HANDLE_CODE(unpack_integer(beam_resel_rel_info.offset, bref, (int8_t)-9, (int8_t)9));
  HANDLE_CODE(unpack_integer(beam_resel_rel_info.hysteresis, bref, (uint8_t)0u, (uint8_t)7u));
  HANDLE_CODE(unpack_integer(beam_resel_rel_info.t_resel, bref, (uint8_t)0u, (uint8_t)7u));
  HANDLE_CODE(beam_resel_rel_info.meas_gap_cfg_wx.unpack(bref));

  if (uac_barring_info_present)
  {
    HANDLE_CODE(uac_barring_info.unpack(bref));
  }

  HANDLE_CODE(rr_cfg_com.unpack(bref));

  HANDLE_CODE(ue_timer_and_constant.unpack(bref));

  if (p_max_n_present)
  {
    HANDLE_CODE(unpack_integer(p_max_n, bref, (uint8_t)0u, (uint8_t)15u));
  }

  HANDLE_CODE(geo_info_update_para.unpack(bref));

  HANDLE_CODE(eph_para_ser_sat.unpack(bref));
}

// sib_wx_s\CF\FBϢ\B5\C4to_json
void sib_wx_s::to_json(json_writer& j)const 
{

    j.start_obj();

    j.start_array("plmn-IdentityList");
    for (uint32_t i1 = 0; i1 < beam_acc_rel_info.plmn_id_li.size(); i1++) 
    {
    beam_acc_rel_info.plmn_id_li[i1].to_json(j);
    }
    j.end_array();

    j.write_str("beamBarred", beam_acc_rel_info.beam_barred.to_string());

    j.write_fieldname("q-RxLevMin");
    beam_sel_rel_info.q_rx_lev_min.to_json(j);

    j.write_fieldname("threshold-Resel");
    beam_resel_rel_info.threshold_resel.to_json(j);
    j.write_fieldname("q-RxLevMin");
    beam_resel_rel_info.q_rx_lev_min.to_json(j);
    j.write_int("offset", beam_resel_rel_info.offset);
    j.write_int("hysteresis", beam_resel_rel_info.hysteresis);
    j.write_int("t_Reselection", beam_resel_rel_info.t_resel);
    j.write_fieldname("meas-GapConfig");
    beam_resel_rel_info.meas_gap_cfg_wx.to_json(j);

    if (uac_barring_info_present)
    {
    j.write_fieldname("uac-BarringInfo");
    uac_barring_info.to_json(j);
    }

    j.write_fieldname("radio-ResourceConfigCommon");
    rr_cfg_com.to_json(j);

    j.write_fieldname("ue-TimersAndConstants");
    ue_timer_and_constant.to_json(j);

    if (p_max_n_present) 
    {
    j.write_int("p-Max", p_max_n);
    }

    j.write_fieldname("geo-InfoUpdateParameters");
    geo_info_update_para.to_json(j);

    j.write_fieldname("ephemerisParameters-ServeSat");
    eph_para_ser_sat.to_json(j);

    j.end_obj();

}
