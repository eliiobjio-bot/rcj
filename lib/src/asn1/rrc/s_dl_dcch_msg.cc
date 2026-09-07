#include "srsran/asn1/rrc/s_dl_dcch_msg.h"
#include "srsran/asn1/rrc/mib_sib_asn1.h"
#include <sstream>

using namespace asn1;
using namespace asn1::rrc;

/*******************************************************************************
 *                                Struct Methods
 ******************************************************************************/
// BandIdentity ::= SEQUENCE
SRSASN_CODE ba_id_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(ba_id.pack(bref));

  return SRSASN_SUCCESS;
}
SRSASN_CODE ba_id_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(ba_id.unpack(bref));

  return SRSASN_SUCCESS;
}
void ba_id_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("band-identity", ba_id.to_string());
  j.end_obj();
}
// // BeamIdentity ::=SEQUENCE
// SRSASN_CODE beam_id_s::pack(bit_ref& bref) const
// {
//   HANDLE_CODE(beam_id.pack(bref));

//   return SRSASN_SUCCESS;
// }
// SRSASN_CODE beam_id_s::unpack(cbit_ref& bref)
// {
//   HANDLE_CODE(beam_id.unpack(bref));

//   return SRSASN_SUCCESS;
// }
// void beam_id_s::to_json(json_writer& j) const
// {
//   j.start_obj();
//   j.write_str("beamIdentity", beam_id.to_string());
//   j.end_obj();
// }

// FrqquencyIdentity ::= SEQUENCE
SRSASN_CODE freq_id_n_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(freq_id.pack(bref));

  return SRSASN_SUCCESS;
}
SRSASN_CODE freq_id_n_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(freq_id.unpack(bref));

  return SRSASN_SUCCESS;
}
void freq_id_n_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("frequency-identity", freq_id.to_string());
  j.end_obj();
}

const char* meas_gap_cfg_normal_s::meas_norm_ratio_opts::to_string() const
{
  static const char* options[] = {
      "dB5", "dB6 ", " dB7 ", " dB8 ", " dB9 ", " dB10 ", " dB11 ", " dB12 ", " dB13 ", " dB14 ", " dB15 ", " dB16 "};
  return convert_enum_idx(options, 12, value, "meas_gap_cfg_normal_s::meas_norm_ratio_opts");
}
uint8_t meas_gap_cfg_normal_s::meas_norm_ratio_opts::to_number() const
{
  static const uint8_t options[] = {5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  return map_enum_number(options, 12, value, "meas_gap_cfg_normal_s::meas_norm_ratio_opts");
}

const char* thr_hold_s::time_to_trigger_opts::to_string() const
{
  static const char* options[] = {"ms0", "ms900", "ms1650", "ms3200"};
  return convert_enum_idx(options, 4, value, "thr_hold_s::time_to_trigger_opts");
}
uint16_t thr_hold_s::time_to_trigger_opts::to_number() const
{
  static const uint16_t options[] = {0, 900, 1650, 3200};
  return map_enum_number(options, 4, value, "thr_hold_s::time_to_trigger_opts");
}

const char* thr_hold_s::filter_coe_ent_opts::to_string() const
{
  static const char* options[] = {"fc0", "fc2 ", " fc4 ", " fc8 ", " fc12 ", " gc25 ", " spare2 ", " spare1 "};
  return convert_enum_idx(options, 8, value, "thr_hold_s::filter_coe_ent_opt");
}
uint8_t thr_hold_s::filter_coe_ent_opts::to_number() const
{
  static const uint8_t options[] = {0, 2, 4, 8, 12, 25, 2, 1};
  return map_enum_number(options, 8, value, "thr_hold_s::filter_coe_ent_opts");
}

const char* report_cfg_s::report_geo_grap_info_opts::to_string() const
{
  static const char* options[] = {"true"};
  return convert_enum_idx(options, 1, value, "report_cfg_s::report_geo_grap_info_opts");
}
uint8_t report_cfg_s::report_geo_grap_info_opts::to_number() const
{
  static const uint8_t options[] = {1};
  return map_enum_number(options, 1, value, "report_cfg_s::report_geo_grap_info_opts");
}

const char* mobility_contro_s::t304_opts::to_string() const
{
  static const char* options[] = {"ms500", "ms800", "ms1000", "ms8000"};
  return convert_enum_idx(options, 4, value, "mobility_contro_s::t304_opts");
}
uint16_t mobility_contro_s::t304_opts::to_number() const
{
  static const uint16_t options[] = {500, 800, 1000, 8000};
  return map_enum_number(options, 4, value, "mobility_contro_s::t304_opts");
}

const char* mobility_contro_s::rach_indi_tor_opts::to_string() const
{
  static const char* options[] = {"True"};
  return convert_enum_idx(options, 1, value, "mobility_contro_s::rach_indi_tor_opts");
}
uint8_t mobility_contro_s::rach_indi_tor_opts::to_number() const
{
  static const uint8_t options[] = {1};
  return map_enum_number(options, 1, value, "mobility_contro_s::rach_indi_tor_opts");
}

const char* rach_cfg_com_s::rach_slot_ass_opts::to_string() const
{
  static const char* options[] = {"halfFrame0", "halfFrame1", "both"};
  return convert_enum_idx(options, 3, value, "rach_cfg_com_s::rach_slot_ass_opts");
}
uint8_t rach_cfg_com_s::rach_slot_ass_opts::to_number() const
{
  static const uint8_t options[] = {0, 1, 2};
  return map_enum_number(options, 3, value, "rach_cfg_com_s::rach_slot_ass_opts");
}

const char* rach_cfg_com_s::ra_res_win_size_opts::to_string() const
{
  static const char* options[] = {"rf5", "rf10", "rf15", "spare1"};
  return convert_enum_idx(options, 4, value, "rach_cfg_com_s::ra_res_win_size_opts");
}
uint8_t rach_cfg_com_s::ra_res_win_size_opts::to_number() const
{
  static const uint8_t options[] = {5, 10, 15, 1};
  return map_enum_number(options, 4, value, "rach_cfg_com_s::ra_res_win_size_opts");
}

const char* agch_cfg_com_n_s ::agch_alot_start_opts::to_string() const
{
  static const char* options[] = {"slot1", "slot2", "slot3", "slot4"};
  return convert_enum_idx(options, 4, value, "agch_cfg_com_n_s::agch_alot_start_opts");
}
uint8_t agch_cfg_com_n_s::agch_alot_start_opts::to_number() const
{
  static const uint8_t options[] = {1, 2, 3, 4};
  return map_enum_number(options, 4, value, "agch_cfg_com_n_s::agch_alot_start_opts");
}

const char* redio_resour_cfg_dedi_s ::peri_bsr_timer_opts::to_string() const
{
  static const char* options[] = {"rf2", "rf5", "rf10", "rf16", "rf20", "rf32", "infinity"};
  return convert_enum_idx(options, 7, value, "redio_resour_cfg_dedi_s ::peri_bsr_timer_opts");
}
uint8_t redio_resour_cfg_dedi_s ::peri_bsr_timer_opts::to_number() const
{
  static const uint8_t options[] = {2, 5, 10, 16, 20, 32, 1};
  return map_enum_number(options, 7, value, "redio_resour_cfg_dedi_s ::peri_bsr_timer_opts");
}

const char* sdap_cfg_s ::sdap_header_dl_opts::to_string() const
{
  static const char* options[] = {"present", "absent"};
  return convert_enum_idx(options, 2, value, "sdap_cfg_s ::sdap_header_dl_opts");
}

const char* sdap_cfg_s ::sdap_header_ul_opts::to_string() const
{
  static const char* options[] = {"present", "absent"};
  return convert_enum_idx(options, 2, value, "sdap_cfg_s ::sdap_header_ul_opts");
}

const char* pdcp_cofg_s ::dis_timer_opts::to_string() const
{
  static const char* options[] = {"ms900", "ms1200", "ms1500", "ms3000", "ms5100", "spare2", "spare1", "infinity"};
  return convert_enum_idx(options, 8, value, "pdcp_cofg_s ::dis_timer_opts");
}
uint16_t pdcp_cofg_s ::dis_timer_opts::to_number() const
{
  static const uint16_t options[] = {900, 1200, 1500, 3000, 5100, 2, 1, 0};
  return map_enum_number(options, 8, value, "pdcp_cofg_s ::dis_timer_opts");
}

const char* pdcp_cofg_s ::integ_pro_opts::to_string() const
{
  static const char* options[] = {"enabled"};
  return convert_enum_idx(options, 1, value, "pdcp_cofg_s ::integ_pro_opts");
}

const char* pdcp_cofg_s ::ciph_dis_opts::to_string() const
{
  static const char* options[] = {"True"};
  return convert_enum_idx(options, 1, value, "pdcp_cofg_s ::ciph_dis_opts");
}

//------------------2023.10.24----------------------------------
const char* rlc_cofg_c::types_opts::to_string() const
{
  static const char* options[] = {"am", "um_bi_dir", "tm"};
  return convert_enum_idx(options, 3, value, "rlc_cofg_c::types_opts");
}
//--------------------------------------------------------------------------------

const char* ul_am_rlcc_s ::max_retx_thres_hold_opts::to_string() const
{
  static const char* options[] = {"t1", "t2", "t4", "t8"};
  return convert_enum_idx(options, 4, value, "ul_am_rlcc_s ::max_retx_thres_hold_opts");
}
uint8_t ul_am_rlcc_s ::max_retx_thres_hold_opts::to_number() const
{
  static const uint8_t options[] = {1, 2, 4, 8};
  return map_enum_number(options, 4, value, "ul_am_rlcc_s ::max_retx_thres_hold_opts");
}

const char* ul_am_rlcc_s ::t_poll_retran_opts::to_string() const
{
  static const char* options[] = {"ms480", "ms1200", "ms2100", "spare1"};
  return convert_enum_idx(options, 4, value, "ul_am_rlcc_s ::t_poll_retran_opts");
}
uint16_t ul_am_rlcc_s ::t_poll_retran_opts::to_number() const
{
  static const uint16_t options[] = {480, 1200, 2100, 1};
  return map_enum_number(options, 4, value, "ul_am_rlcc_s ::t_poll_retran_opts");
}

const char* ul_am_rlcc_s ::poll_pdu_opts::to_string() const
{
  static const char* options[] = {"p8", "p16", "p32", "pInfinity"};
  return convert_enum_idx(options, 4, value, "ul_am_rlcc_s ::poll_pdu_opts");
}
uint8_t ul_am_rlcc_s ::poll_pdu_opts::to_number() const
{
  static const uint8_t options[] = {8, 16, 32, 1};
  return map_enum_number(options, 4, value, "ul_am_rlcc_s ::poll_pdu_opts");
}

const char* ul_am_rlcc_s ::poll_byte_opts::to_string() const
{
  static const char* options[] = {"kb16", "kb128", "kn256", "kBInfinity"};
  return convert_enum_idx(options, 4, value, "ul_am_rlcc_s ::poll_byte_opts");
}
uint16_t ul_am_rlcc_s ::poll_byte_opts::to_number() const
{
  static const uint16_t options[] = {16, 128, 256, 1};
  return map_enum_number(options, 4, value, "ul_am_rlcc_s ::poll_byte_opts");
}

const char* dl_am_rlcc_s ::t_reord_opts::to_string() const
{
  static const char* options[] = {"ms480", "ms1200", "ms2100", "spare1"};
  return convert_enum_idx(options, 4, value, "dl_am_rlcc_s ::t_reord_opts");
}
uint16_t dl_am_rlcc_s ::t_reord_opts::to_number() const
{
  static const uint16_t options[] = {480, 1200, 2100, 1};
  return map_enum_number(options, 4, value, "dl_am_rlcc_s ::t_reord_opts");
}

const char* dl_am_rlcc_s ::t_status_proh_opts::to_string() const
{
  static const char* options[] = {"ms0", "ms420", "ms600", "spare1"};
  return convert_enum_idx(options, 4, value, "dl_am_rlcc_s ::t_status_proh_opts");
}
uint16_t dl_am_rlcc_s ::t_status_proh_opts::to_number() const
{
  static const uint16_t options[] = {0, 420, 600, 1};
  return map_enum_number(options, 4, value, "dl_am_rlcc_s ::t_status_proh_opts");
}

const char* dl_um_rlcc_s ::t_reord_opts::to_string() const
{
  static const char* options[] = {"ms480", "ms1200", "ms2100", "spare1"};
  return convert_enum_idx(options, 4, value, "dl_um_rlcc_s ::t_reord_opts");
}
uint16_t dl_um_rlcc_s ::t_reord_opts::to_number() const
{
  static const uint16_t options[] = {480, 1200, 2100, 1};
  return map_enum_number(options, 4, value, "dl_um_rlcc_s ::t_reord_opts");
}

const char* ul_spec_para_s ::prio_bit_rate_opts::to_string() const
{
  static const char* options[] = {"kBps0", "kBps2dot4", " kBps16", "kBps128", "infinity", "spare3", "spare", "spare1"};
  return convert_enum_idx(options, 8, value, "ul_spec_para_s ::prio_bit_rate_opts");
}
uint16_t ul_spec_para_s ::prio_bit_rate_opts::to_number() const
{
  static const uint16_t options[] = {0, 24, 16, 128, 8, 3, 2, 1};
  return map_enum_number(options, 8, value, "ul_spec_para_s ::prio_bit_rate_opts");
}

const char* ul_spec_para_s ::buck_size_dura_opts::to_string() const
{
  static const char* options[] = {"ms60", " ms120", "ms180", "ms300", "ms600", "ms1200", "spare2", "spare1"};
  return convert_enum_idx(options, 8, value, "ul_spec_para_s ::buck_size_dura_opts");
}
uint16_t ul_spec_para_s ::buck_size_dura_opts::to_number() const
{
  static const uint16_t options[] = {60, 120, 180, 300, 600, 1200, 2, 1};
  return map_enum_number(options, 8, value, "ul_spec_para_s ::buck_size_dura_opts");
}

const char* phy_chan_cfg_s ::direct_opts::to_string() const
{
  static const char* options[] = {"biDirection", " ulDirection", "dlDirection"};
  return convert_enum_idx(options, 3, value, "phy_chan_cfg_s ::direct_opts");
}

const char* phy_chan_cfg_s ::sche_type_opts::to_string() const
{
  static const char* options[] = {"Static", "dynamic"};
  return convert_enum_idx(options, 2, value, "phy_chan_cfg_s ::sche_type_opts");
}

const char* phy_chan_cfg_s ::voice_type_opts::to_string() const
{
  static const char* options[] = {"kbps2point4", " kbps4point8", " bps800"};
  return convert_enum_idx(options, 3, value, "phy_chan_cfg_s ::voice_type_opts");
}

const char* phy_chan_cfg_s::chan_type_opts::to_string() const
{
  static const char* options[] = {"pSYCH",
                                  "pDCH1-1",
                                  "pDCH1-2",
                                  "pSCH1-1",
                                  "pSCH1-2",
                                  "pSCH5-1",
                                  "pSCH5-2",
                                  "dS-PDTCH-1",
                                  "dS-PDTCH-2",
                                  "dS-PDTCH-3",
                                  "dS-PDTCH-T"};
  return convert_enum_idx(options, 11, value, "phy_chan_cfg_s::chan_type_opts");
}

const char* sec_alg_cfg_s ::ciph_alg_opts::to_string() const
{
  static const char* options[] = {"nea0", "nea1", "nea2", "nea3", "nea4", "nea5", "nea6", "nea7", "nea8", "nea9", "nea10", "nea11", "nea12", "nea13", "nea14", "nea15"};
  return convert_enum_idx(options, 16, value, "sec_alg_cfg_s ::ciph_alg_opts");
}
uint8_t sec_alg_cfg_s ::ciph_alg_opts::to_number() const
{
  static const uint8_t options[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  return map_enum_number(options, 16, value, "sec_alg_cfg_s ::ciph_alg_opts");
}


const char* sec_alg_cfg_s ::intef_prot_alg_opts::to_string() const
{
  static const char* options[] = {"nia0", "nia1", "nia2", "nia3", "nia4", "nia5", "nia6", "nia7", "nia8", "nia9", "nia10", "nia11", "nia12", "nia13", "nia14", "nia15"};
  return convert_enum_idx(options, 16, value, "sec_alg_cfg_s ::intef_prot_alg_opts");
}
uint8_t sec_alg_cfg_s ::intef_prot_alg_opts::to_number() const
{
  static const uint8_t options[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  return map_enum_number(options, 16, value, "sec_alg_cfg_s ::intef_prot_alg_opts");
}


const char* set_up_s ::peri_phr_timer_opts::to_string() const
{
  static const char* options[] = {"sf10", "sf20", "sf50", "sf100", "sf200", "sf500", "sf1000", "infinity"};
  return convert_enum_idx(options, 8, value, "set_up_s ::peri_phr_timer_opts");
}
uint16_t set_up_s::peri_phr_timer_opts::to_number() const
{
  static const uint16_t options[] = {10, 20, 50, 100, 200, 500, 100, 0};
  return map_enum_number(options, 8, value, "set_up_s ::peri_phr_timer_opts");
}

const char* set_up_s ::prohi_phr_time_opts::to_string() const
{
  static const char* options[] = {"0", " sf10 ", " sf20 ", " sf50 ", " sf100 ", " sf200 ", " sf500 ", " sf1000 "};
  return convert_enum_idx(options, 8, value, "set_up_s ::prohi_phr_time_opts");
}
uint16_t set_up_s::prohi_phr_time_opts::to_number() const
{
  static const uint16_t options[] = {0, 10, 20, 50, 100, 200, 500, 100};
  return map_enum_number(options, 8, value, "set_up_s ::prohi_phr_time_opts");
}

const char* set_up_s ::dl_path_loss_change_opts::to_string() const
{
  static const char* options[] = {"dB1", "dB3 ", "dB6 "};
  return convert_enum_idx(options, 3, value, "set_up_s ::dl_path_loss_change_opts");
}
uint16_t set_up_s::dl_path_loss_change_opts::to_number() const
{
  static const uint16_t options[] = {1, 3, 6};
  return map_enum_number(options, 8, value, "set_up_s ::dl_path_loss_change_opts");
}

const char* rrc_con_rel_r1_ies_s ::relea_cause_opts::to_string() const
{
  static const char* options[] = {"loadBalancing", "other ", "spare2 ", "spare1"};
  return convert_enum_idx(options, 4, value, "rrc_con_rel_r1_ies_s ::relea_cause_opts");
}

const char* redio_resour_cfg_co_s::naviInfo_slot_ass_opts::to_string() const
{
  static const char* options[] = {"slot1", "slot2", "slot3", "slot4"};
  return convert_enum_idx(options, 4, value, "redio_resour_cfg_co_s::naviInfo_slot_ass_e_");
}
uint8_t redio_resour_cfg_co_s::naviInfo_slot_ass_opts::to_number() const
{
  static const uint8_t options[] = {1, 2, 3, 4};
  return map_enum_number(options, 4, value, "redio_resour_cfg_co_s::naviInfo_slot_ass_e_");
}

// //DedicatedInfoNAS
SRSASN_CODE dedi_info_nas_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(delicated_info_nas.pack(bref));
}
SRSASN_CODE dedi_info_nas_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(delicated_info_nas.unpack(bref));
}
void dedi_info_nas_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("delicated_info_nas", delicated_info_nas.to_string());
  j.end_obj();
}
////DedicatedInfoSCM
SRSASN_CODE dedi_info_scm_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(delicated_info_scm.pack(bref));
}
SRSASN_CODE dedi_info_scm_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(delicated_info_scm.unpack(bref));
}
void dedi_info_scm_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("delicated_info_scm", delicated_info_scm.to_string());
  j.end_obj();
}
// //dedicatedInfoType      CHOICE
void dedi_info_type_c_::destroy_()
{
  switch (type_) {
    case types::dedi_info_nas:
      c.destroy<dedi_info_nas_s>();
      break;
    case types::dedi_info_scm:
      c.destroy<dedi_info_scm_s>();
      break;
    default:
      break;
  }
}
void dedi_info_type_c_::set(types::options e)
{
  destroy_();
  type_ = e;
  switch (type_) {
    case types::dedi_info_nas:
      c.init<dedi_info_nas_s>();
      break;
    case types::dedi_info_scm:
      c.init<dedi_info_scm_s>();
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "dedi_info_type_c_");
  }
}
dedi_info_type_c_::dedi_info_type_c_(const dedi_info_type_c_& other)
{
  type_ = other.type();
  switch (type_) {
    case types::dedi_info_nas:
      c.init(other.c.get<dedi_info_nas_s>());
      break;
    case types::dedi_info_scm:
      c.init(other.c.get<dedi_info_scm_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "dedi_info_type_c_");
  }
}
dedi_info_type_c_& dedi_info_type_c_::operator=(const dedi_info_type_c_& other)
{
  if (this == &other) {
    return *this;
  }
  set(other.type());
  switch (type_) {
    case types::dedi_info_nas:
      c.set(other.c.get<dedi_info_nas_s>());
      break;
    case types::dedi_info_scm:
      c.set(other.c.get<dedi_info_scm_s>());
      break;

    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "dedi_info_type_c_");
  }
  return *this;
}
dedi_info_nas_s& dedi_info_type_c_::set_dedi_info_nas()
{
  set(types::dedi_info_nas);
  return c.get<dedi_info_nas_s>();
}
dedi_info_scm_s& dedi_info_type_c_::set_dedi_info_scm()
{
  set(types::dedi_info_scm);
  return c.get<dedi_info_scm_s>();
}

void dedi_info_type_c_::to_json(json_writer& j) const
{
  j.start_obj();
  switch (type_) {
    case types::dedi_info_nas:
      j.write_fieldname("dedi_info_nas");
      c.get<dedi_info_nas_s>().to_json(j);
      break;
    case types::dedi_info_scm:
      j.write_fieldname("dedi_info_scm");
      c.get<dedi_info_scm_s>().to_json(j);
      break;
    default:
      log_invalid_choice_id(type_, "dedi_info_type_c_");
  }
  j.end_obj();
}
SRSASN_CODE dedi_info_type_c_::pack(bit_ref& bref) const
{
  type_.pack(bref);
  switch (type_) {
    case types::dedi_info_nas:
      HANDLE_CODE(c.get<dedi_info_nas_s>().pack(bref));
      break;
    case types::dedi_info_scm:
      HANDLE_CODE(c.get<dedi_info_scm_s>().pack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "s_dl_dcch_msg_type_c");
      return SRSASN_ERROR_ENCODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE dedi_info_type_c_::unpack(cbit_ref& bref)
{
  type_.unpack(bref);
  switch (type_) {
    case types::dedi_info_nas:
      HANDLE_CODE(c.get<dedi_info_nas_s>().unpack(bref));
      break;
    case types::dedi_info_scm:
      HANDLE_CODE(c.get<dedi_info_scm_s>().unpack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "dedi_info_type_c_");
      return SRSASN_ERROR_ENCODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
const char* dedi_info_type_c_::types_opts::to_string() const
{
  static const char* options[] = {"dedi_info_nas", "dedi_info_scm"};
  return convert_enum_idx(options, 2, value, "dedi_info_type_c_::types");
}

/// Threshold		SEQUENCE
SRSASN_CODE thr_hold_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(pack_integer(bref, off_set, (int8_t)-9, (int8_t)9));
  HANDLE_CODE(pack_integer(bref, hysteresis, (uint8_t)0u, (uint8_t)7u));
  HANDLE_CODE(time_to_trigger.pack(bref));
  HANDLE_CODE(filter_coe_ent.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE thr_hold_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(unpack_integer(off_set, bref, (int8_t)-9, (int8_t)9));
  HANDLE_CODE(unpack_integer(hysteresis, bref, (uint8_t)0u, (uint8_t)7u));
  HANDLE_CODE(time_to_trigger.unpack(bref));
  HANDLE_CODE(filter_coe_ent.unpack(bref));
  return SRSASN_SUCCESS;
}
void thr_hold_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_int("off_set", off_set);
  j.write_int("hysteresis", hysteresis);
  j.write_str("time_to_trigger", time_to_trigger.to_string());
  j.write_str("filter_coe_ent", filter_coe_ent.to_string());
  j.end_obj();
}
////ReportConfig::= SEQUENCE
SRSASN_CODE report_cfg_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(thr_hold_present, 1));
  HANDLE_CODE(bref.pack(report_geo_grap_info_present, 1));
  if (thr_hold_present) {
    HANDLE_CODE(thr_hold.pack(bref));
  }
  if (report_geo_grap_info_present) {
    HANDLE_CODE(report_geo_grap_info.pack(bref));
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE report_cfg_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(thr_hold_present, 1));
  HANDLE_CODE(bref.unpack(report_geo_grap_info_present, 1));
  if (thr_hold_present) {
    HANDLE_CODE(thr_hold.unpack(bref));
  }
  if (report_geo_grap_info_present) {
    HANDLE_CODE(report_geo_grap_info.unpack(bref));
  }
  return SRSASN_SUCCESS;
}
void report_cfg_s::to_json(json_writer& j) const
{
  j.start_obj();
  if (thr_hold_present) {
    j.write_fieldname("thr_hold");
    thr_hold.to_json(j);
  }
  if (report_geo_grap_info) {
    j.write_str(" report_geo_grap_info", report_geo_grap_info.to_string());
  }
  j.end_obj();
}

//	RSSI-Normal				INTEGER(0..64)
SRSASN_CODE rssi_normal_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(pack_integer(bref, rssi_normal, (uint16_t)0u, (uint16_t)63u));
  return SRSASN_SUCCESS;
}
SRSASN_CODE rssi_normal_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(unpack_integer(rssi_normal, bref, (uint16_t)0u, (uint16_t)63u));
  return SRSASN_SUCCESS;
}
void rssi_normal_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_int("rssi_normal", rssi_normal);
  j.end_obj();
}
// //RSSI-DlFreqSpread
SRSASN_CODE rssi_dl_freq_spread_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(pack_integer(bref, rssi_dl_freq_spread, (uint16_t)0u, (uint16_t)63u));
  return SRSASN_SUCCESS;
}
SRSASN_CODE rssi_dl_freq_spread_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(unpack_integer(rssi_dl_freq_spread, bref, (uint16_t)0u, (uint16_t)63u));
  return SRSASN_SUCCESS;
}
void rssi_dl_freq_spread_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_int("rssi_dl_freq_spread", rssi_dl_freq_spread);
  j.end_obj();
}
// //rssi_n_c_
void rssi_n_c_::destroy_()
{
  switch (type_) {
    case types::rssi_normal:
      c.destroy<rssi_normal_s>();
      break;
    case types::rssi_dl_freq_spread:
      c.destroy<rssi_dl_freq_spread_s>();
      break;
    default:
      break;
  }
}
void rssi_n_c_::set(types::options e)
{
  destroy_();
  type_ = e;
  switch (type_) {
    case types::rssi_normal:
      c.init<rssi_normal_s>();
      break;
    case types::rssi_dl_freq_spread:
      c.init<rssi_dl_freq_spread_s>();
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "rssi_n_c_");
  }
}
rssi_n_c_::rssi_n_c_(const rssi_n_c_& other)
{
  type_ = other.type();
  switch (type_) {
    case types::rssi_normal:
      c.init(other.c.get<rssi_normal_s>());
      break;
    case types::rssi_dl_freq_spread:
      c.init(other.c.get<rssi_dl_freq_spread_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "rssi_n_c_");
  }
}
rssi_n_c_& rssi_n_c_::operator=(const rssi_n_c_& other)
{
  if (this == &other) {
    return *this;
  }
  set(other.type());
  switch (type_) {
    case types::rssi_normal:
      c.set(other.c.get<rssi_normal_s>());
      break;
    case types::rssi_dl_freq_spread:
      c.set(other.c.get<rssi_dl_freq_spread_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "rssi_n_c_");
  }
  return *this;
}
rssi_normal_s& rssi_n_c_::set_rssi_normal()
{
  set(types::rssi_normal);
  return c.get<rssi_normal_s>();
}
rssi_dl_freq_spread_s& rssi_n_c_::set_rssi_dl_freq_spread()
{
  set(types::rssi_dl_freq_spread);
  return c.get<rssi_dl_freq_spread_s>();
}

void rssi_n_c_::to_json(json_writer& j) const
{
  j.start_obj();
  switch (type_) {
    case types::rssi_normal:
      j.write_fieldname("rssi_normal");
      c.get<rssi_normal_s>().to_json(j);
      break;
    case types::rssi_dl_freq_spread:
      j.write_fieldname("rssi_dl_freq_spread");
      c.get<rssi_dl_freq_spread_s>().to_json(j);
      break;

    default:
      log_invalid_choice_id(type_, "rssi_n_c_");
  }
  j.end_obj();
}
SRSASN_CODE rssi_n_c_::pack(bit_ref& bref) const
{
  type_.pack(bref);
  switch (type_) {
    case types::rssi_normal:
      HANDLE_CODE(c.get<rssi_normal_s>().pack(bref));
      break;
    case types::rssi_dl_freq_spread:
      HANDLE_CODE(c.get<rssi_dl_freq_spread_s>().pack(bref));
      break;

    default:
      log_invalid_choice_id(type_, "rssi_n_c_");
      return SRSASN_ERROR_ENCODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE rssi_n_c_::unpack(cbit_ref& bref)
{
  types e;
  e.unpack(bref);
  set(e);
  switch (type_) {
    case types::rssi_normal:
      HANDLE_CODE(c.get<rssi_normal_s>().unpack(bref));
      break;
    case types::rssi_dl_freq_spread:
      HANDLE_CODE(c.get<rssi_dl_freq_spread_s>().unpack(bref));
      break;

    default:
      log_invalid_choice_id(type_, "rssi_n_c_");
      return SRSASN_ERROR_DECODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
const char* rssi_n_c_::types_opts::to_string() const
{
  static const char* options[] = {"rssi_normal", "rssi_dl_freq_spread"};
  return convert_enum_idx(options, 2, value, "rssi_n_c_::types_opts");
}

/// MeasGapConfig-Normal :: =			SEQUENCE'
SRSASN_CODE meas_gap_cfg_normal_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(pack_integer(bref, beam_index, (uint8_t)1u, (uint8_t)15u));
  HANDLE_CODE(pack_integer(bref, frame_offset, (uint8_t)0u, (uint8_t)51u));
  HANDLE_CODE(fcch_ba_id.pack(bref));
  HANDLE_CODE(fcch_freq_id.pack(bref));
  HANDLE_CODE(meas_norm_ratio.pack(bref));
  HANDLE_CODE(pack_integer(bref, mib_Re_Fra_Num, (uint8_t)0u, (uint8_t)51u));
  return SRSASN_SUCCESS;
}
SRSASN_CODE meas_gap_cfg_normal_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(unpack_integer(beam_index, bref, (uint8_t)1u, (uint8_t)15u));
  HANDLE_CODE(unpack_integer(frame_offset, bref, (uint8_t)0u, (uint8_t)51u));
  HANDLE_CODE(fcch_ba_id.unpack(bref));
  HANDLE_CODE(fcch_freq_id.unpack(bref));
  HANDLE_CODE(meas_norm_ratio.unpack(bref));
  HANDLE_CODE(unpack_integer(mib_Re_Fra_Num, bref, (uint8_t)0u, (uint8_t)51u));
  return SRSASN_SUCCESS;
}
void meas_gap_cfg_normal_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_int("beam_index", beam_index);
  j.write_int("frame_offset", frame_offset);
  j.write_fieldname("fcch_ba_id");
  fcch_ba_id.to_json(j);
  j.write_fieldname("fcch_freq_id");
  fcch_freq_id.to_json(j);
  j.write_str("meas_norm_ratio", meas_norm_ratio.to_string());
  j.write_int("mibRelativeFrameNum", mib_Re_Fra_Num);
  j.end_obj();
}

// //MeasGapConfig-DIFreqSpread :: =	SEQUENCE
SRSASN_CODE meas_gap_cfg_dl_freq_spread_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(pack_integer(bref, beam_id, (uint8_t)1u, (uint8_t)15u));
  HANDLE_CODE(pack_integer(bref, sec_syn_group_id, (uint8_t)1u, (uint8_t)48u));
  return SRSASN_SUCCESS;
}
SRSASN_CODE meas_gap_cfg_dl_freq_spread_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(unpack_integer(beam_id, bref, (uint8_t)1u, (uint8_t)15u));
  HANDLE_CODE(unpack_integer(sec_syn_group_id, bref, (uint8_t)1u, (uint8_t)48u));
  return SRSASN_SUCCESS;
}
void meas_gap_cfg_dl_freq_spread_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_int("beam_id", beam_id);
  j.write_int("sec_syn_group_id", sec_syn_group_id);
  j.end_obj();
}

// //measGapConfigList		CHOICE
void meas_gap_cfg_list_c_::destroy_()
{
  switch (type_) {
    case types::meas_gap_cfg_list_normal:
      c.destroy<meas_gap_cfg_list_normal_s>();
      break;
    case types::meas_gap_cfg_list_dl_freq_spread:
      c.destroy<meas_gap_cfg_list_dl_freq_spread_s>();
      break;
    default:
      break;
  }
}
void meas_gap_cfg_list_c_::set(types::options e)
{
  destroy_();
  type_ = e;
  switch (type_) {
    case types::meas_gap_cfg_list_normal:
      c.init<meas_gap_cfg_list_normal_s>();
      break;
    case types::meas_gap_cfg_list_dl_freq_spread:
      c.init<meas_gap_cfg_list_dl_freq_spread_s>();
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "meas_gap_cfg_list_c_");
  }
}
meas_gap_cfg_list_c_::meas_gap_cfg_list_c_(const meas_gap_cfg_list_c_& other)
{
  type_ = other.type();
  switch (type_) {
    case types::meas_gap_cfg_list_normal:
      c.init(other.c.get<meas_gap_cfg_list_normal_s>());
      break;
    case types::meas_gap_cfg_list_dl_freq_spread:
      c.init(other.c.get<meas_gap_cfg_list_dl_freq_spread_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "meas_gap_cfg_list_c_");
  }
}
meas_gap_cfg_list_c_& meas_gap_cfg_list_c_::operator=(const meas_gap_cfg_list_c_& other)
{
  if (this == &other) {
    return *this;
  }
  set(other.type());
  switch (type_) {
    case types::meas_gap_cfg_list_normal:
      c.set(other.c.get<meas_gap_cfg_list_normal_s>());
      break;
    case types::meas_gap_cfg_list_dl_freq_spread:
      c.set(other.c.get<meas_gap_cfg_list_dl_freq_spread_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "meas_gap_cfg_list_c_");
  }
  return *this;
}
meas_gap_cfg_list_normal_s& meas_gap_cfg_list_c_::set_meas_gap_cfg_list_normal()
{
  set(types::meas_gap_cfg_list_normal);
  return c.get<meas_gap_cfg_list_normal_s>();
}
meas_gap_cfg_list_dl_freq_spread_s& meas_gap_cfg_list_c_::set_meas_gap_cfg_list_dl_freq_spread_s()
{
  set(types::meas_gap_cfg_list_dl_freq_spread);
  return c.get<meas_gap_cfg_list_dl_freq_spread_s>();
}

void meas_gap_cfg_list_c_::to_json(json_writer& j) const
{
  j.start_obj();
  switch (type_) {
    case types::meas_gap_cfg_list_normal:
      j.start_array("meas_gap_cfg_list_normal");
      for (const auto& e1 : c.get<meas_gap_cfg_list_normal_s>()) {
        e1.to_json(j);
      }
      j.end_array();
      break;
    case types::meas_gap_cfg_list_dl_freq_spread:
      j.start_array("meas_gap_cfg_list_dl_freq_spread");
      for (const auto& e1 : c.get<meas_gap_cfg_list_dl_freq_spread_s>()) {
        e1.to_json(j);
      }
      j.end_array();
      break;
    default:
      log_invalid_choice_id(type_, "meas_gap_cfg_list_c_");
  }
  j.end_obj();
}
SRSASN_CODE meas_gap_cfg_list_c_::pack(bit_ref& bref) const
{
  type_.pack(bref);
  switch (type_) {
    case types::meas_gap_cfg_list_normal:
      HANDLE_CODE(pack_dyn_seq_of(bref, c.get<meas_gap_cfg_list_normal_s>(), 1, 15));
      break;
    case types::meas_gap_cfg_list_dl_freq_spread:
      HANDLE_CODE(pack_dyn_seq_of(bref, c.get<meas_gap_cfg_list_dl_freq_spread_s>(), 1, 15));
      break;
    default:
      log_invalid_choice_id(type_, "meas_gap_cfg_list_c_");
      return SRSASN_ERROR_ENCODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE meas_gap_cfg_list_c_::unpack(cbit_ref& bref)
{
  type_.unpack(bref);
  switch (type_) {
    case types::meas_gap_cfg_list_normal:
      HANDLE_CODE(unpack_dyn_seq_of(c.get<meas_gap_cfg_list_normal_s>(), bref, 1, 15));
      break;
    case types::meas_gap_cfg_list_dl_freq_spread:
      HANDLE_CODE(unpack_dyn_seq_of(c.get<meas_gap_cfg_list_dl_freq_spread_s>(), bref, 1, 15));
      break;
    default:
      log_invalid_choice_id(type_, "meas_gap_cfg_list_c_");
      return SRSASN_ERROR_ENCODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
const char* meas_gap_cfg_list_c_::types_opts::to_string() const
{
  static const char* options[] = {"meas_gap_cfg_list_normal", "meas_gap_cfg_list_dl_freq_spread"};
  return convert_enum_idx(options, 2, value, "meas_gap_cfg_list_c_::types");
}

/// MeasConfig
SRSASN_CODE meas_cofg_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(meas_gap_cfg_list_present, 1));
  HANDLE_CODE(bref.pack(s_mea_sure_present, 1));
  if (meas_gap_cfg_list_present)
  {
    HANDLE_CODE(meas_gap_cfg_list.pack(bref));
  }
    HANDLE_CODE(report_cfg.pack(bref));
  if (s_mea_sure_present)
  {
    HANDLE_CODE(s_mea_sure.pack(bref));
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE meas_cofg_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(meas_gap_cfg_list_present, 1));
  HANDLE_CODE(bref.unpack(s_mea_sure_present, 1));
  if (meas_gap_cfg_list_present)
  {
    HANDLE_CODE(meas_gap_cfg_list.unpack(bref));
  }
    HANDLE_CODE(report_cfg.unpack(bref));
  if (s_mea_sure_present)
  {
    HANDLE_CODE(s_mea_sure.unpack(bref));
  }
  
  return SRSASN_SUCCESS;
}
void meas_cofg_s::to_json(json_writer& j) const
{
  j.start_obj();
  if (meas_gap_cfg_list_present)
  {
    j.write_fieldname("meas_gap_cfg_list");
    meas_gap_cfg_list.to_json(j);
  }
  j.write_fieldname(" report_cfg");
  report_cfg.to_json(j);
  if (s_mea_sure_present) {
    j.write_fieldname("s_mea_sure");
    s_mea_sure.to_json(j);
  }
  j.end_obj();
}

// mobility_contro_s
SRSASN_CODE mobility_contro_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(rach_indi_tor_present, 1));
  HANDLE_CODE(pack_integer(bref, target_beam_index, (uint8_t)1u, (uint8_t)15u));
  HANDLE_CODE(target_beam_id.pack(bref));
  HANDLE_CODE(target_bcch_band_id.pack(bref));
  HANDLE_CODE(t304.pack(bref));
  if (rach_indi_tor_present) {
    HANDLE_CODE(rach_indi_tor.pack(bref));
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE mobility_contro_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(rach_indi_tor_present, 1));

  HANDLE_CODE(unpack_integer(target_beam_index, bref, (uint8_t)1u, (uint8_t)15u));
  HANDLE_CODE(target_beam_id.unpack(bref));
  HANDLE_CODE(target_bcch_band_id.unpack(bref));
  HANDLE_CODE(t304.unpack(bref));
  if (rach_indi_tor_present) {
    HANDLE_CODE(rach_indi_tor.unpack(bref));
  }
  return SRSASN_SUCCESS;
}
void mobility_contro_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_int("target_beam_index", target_beam_index);
  j.write_fieldname(" target_beam_id");
  target_beam_id.to_json(j);
  j.write_fieldname("target_bcch_band_id");
  target_bcch_band_id.to_json(j);
  j.write_str("t304", t304.to_string());
  j.write_str(" rach_indi_tor", rach_indi_tor.to_string());
  j.end_obj();
}
// dedi_info_nas
SRSASN_CODE dedi_info_nas_n_l_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(dedi_info_nas_n1.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE dedi_info_nas_n_l_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(dedi_info_nas_n1.unpack(bref));
  return SRSASN_SUCCESS;
}
void dedi_info_nas_n_l_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("dedi_info_nas_n1", dedi_info_nas_n1.to_string());
  j.end_obj();
}

// RACH-ConfigCommon :: =SEQUENCE

SRSASN_CODE rach_cfg_com_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(band_is.pack(bref));
  HANDLE_CODE(freq_bit_map.pack(bref));
  HANDLE_CODE(rach_frame_ass.pack(bref));
  HANDLE_CODE(rach_slot_ass.pack(bref));
  HANDLE_CODE(ra_res_win_size.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE rach_cfg_com_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(band_is.unpack(bref));
  HANDLE_CODE(freq_bit_map.unpack(bref));
  HANDLE_CODE(rach_frame_ass.unpack(bref));
  HANDLE_CODE(rach_slot_ass.unpack(bref));
  HANDLE_CODE(ra_res_win_size.unpack(bref));
  return SRSASN_SUCCESS;
}
void rach_cfg_com_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname(" band_is");
  band_is.to_json(j);
  j.write_str("freq_bit_map", freq_bit_map.to_string());
  j.write_str("rach_frame_ass", rach_frame_ass.to_string());
  j.write_str("rach_slot_ass", rach_slot_ass.to_string());
  j.write_str("ra_res_win_siz", ra_res_win_size.to_string());
  j.end_obj();
}

// //AGCH-ConfigCommon :: =			SEQUENCE
SRSASN_CODE agch_cfg_com_n_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(band_id_present, 1));
  HANDLE_CODE(bref.pack(freq_id_present, 1));
  HANDLE_CODE(bref.pack(agch_slot_styart_present, 1));
  if(band_id_present)
  {
    HANDLE_CODE(band_id.pack(bref));
  }
  if(freq_id_present)
  {
    HANDLE_CODE(freq_id.pack(bref));
  }
  HANDLE_CODE(agch_frame_ass.pack(bref));
  if (agch_slot_styart_present) {
    HANDLE_CODE(agch_slot_start.pack(bref));
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE agch_cfg_com_n_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(band_id_present, 1));
  HANDLE_CODE(bref.unpack(freq_id_present, 1));
  HANDLE_CODE(bref.unpack(agch_slot_styart_present, 1));
  if(band_id_present)
  {
    HANDLE_CODE(band_id.unpack(bref));
  }
  if (freq_id_present)
  {
    HANDLE_CODE(freq_id.unpack(bref));
  }
  HANDLE_CODE(agch_frame_ass.unpack(bref));
  if (agch_slot_styart_present) {
    HANDLE_CODE(agch_slot_start.unpack(bref));
  }

  return SRSASN_SUCCESS;
}
void agch_cfg_com_n_s::to_json(json_writer& j) const
{
  j.start_obj();
  if (band_id_present) {
    j.write_fieldname("band_id");
    band_id.to_json(j);
  }
  if (freq_id_present) {
    j.write_fieldname("freq_id");
    freq_id.to_json(j);
  }

  j.write_str("agch_frame_ass", agch_frame_ass.to_string());

  if (agch_slot_styart_present) {
    j.write_str("agch_slot_start", agch_slot_start.to_string());
  }
  j.end_obj();
}
// P-Max::= INTEGER(0..15)
SRSASN_CODE p_max_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(pack_integer(bref, p_max, (uint8_t)0u, (uint8_t)15u));
  return SRSASN_SUCCESS;
}
SRSASN_CODE p_max_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(unpack_integer(p_max, bref, (uint8_t)0u, (uint8_t)15u));
  return SRSASN_SUCCESS;
}
void p_max_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_int("p_max", p_max);
  j.end_obj();
}

////PowerControl-ConfigCommon :: =	SEQUENCE
SRSASN_CODE power_control_cfg_com_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(exp_rx_power_prach_present, 1));
  HANDLE_CODE(bref.pack(exp_rx_power_psych_present, 1));
  HANDLE_CODE(bref.pack(exp_rx_power_pdch1_1_present, 1));
  HANDLE_CODE(bref.pack(exp_rx_power_pdch1_2_present, 1));
  HANDLE_CODE(bref.pack(exp_rx_power_psch1_1_present, 1));
  HANDLE_CODE(bref.pack(exp_rx_power_psch1_2_present, 1));
  HANDLE_CODE(bref.pack(exp_rx_power_psch5_1_present, 1));
  HANDLE_CODE(bref.pack(exp_rx_power_psch5_2_present, 1));
  HANDLE_CODE(bref.pack(exp_rx_power_ptuch_present, 1));

  HANDLE_CODE(pmbch_tx_power.pack(bref));
  if (exp_rx_power_prach_present) {
    HANDLE_CODE(exp_rx_power_prach.pack(bref));
  }
  if (exp_rx_power_psych_present) {
    HANDLE_CODE(exp_rx_power_psych.pack(bref));
  }
  if (exp_rx_power_pdch1_1_present) {
    HANDLE_CODE(exp_rx_power_pdch1_1.pack(bref));
  }

  if (exp_rx_power_pdch1_2_present) {
    HANDLE_CODE(exp_rx_power_pdch1_2.pack(bref));
  }
  if (exp_rx_power_psch1_1_present) {
    HANDLE_CODE(exp_rx_power_psch1_1.pack(bref));
  }
  if (exp_rx_power_psch1_2_present) {
    HANDLE_CODE(exp_rx_power_psch1_2.pack(bref));
  }
  if (exp_rx_power_psch5_1_present) {
    HANDLE_CODE(exp_rx_power_psch5_1.pack(bref));
  }
  if (exp_rx_power_psch5_2_present) {
    HANDLE_CODE(exp_rx_power_psch5_2.pack(bref));
  }
  if (exp_rx_power_ptuch_present) {
    HANDLE_CODE(exp_rx_power_ptuch.pack(bref));
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE power_control_cfg_com_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(exp_rx_power_prach_present, 1));
  HANDLE_CODE(bref.unpack(exp_rx_power_psych_present, 1));
  HANDLE_CODE(bref.unpack(exp_rx_power_pdch1_1_present, 1));
  HANDLE_CODE(bref.unpack(exp_rx_power_pdch1_2_present, 1));
  HANDLE_CODE(bref.unpack(exp_rx_power_psch1_1_present, 1));
  HANDLE_CODE(bref.unpack(exp_rx_power_psch1_2_present, 1));
  HANDLE_CODE(bref.unpack(exp_rx_power_psch5_1_present, 1));
  HANDLE_CODE(bref.unpack(exp_rx_power_psch5_2_present, 1));
  HANDLE_CODE(bref.unpack(exp_rx_power_ptuch_present, 1));

  HANDLE_CODE(pmbch_tx_power.unpack(bref));
  if (exp_rx_power_prach_present) {
    HANDLE_CODE(exp_rx_power_prach.unpack(bref));
  }
  if (exp_rx_power_psych_present) {
    HANDLE_CODE(exp_rx_power_psych.unpack(bref));
  }
  if (exp_rx_power_pdch1_1_present) {
    HANDLE_CODE(exp_rx_power_pdch1_1.unpack(bref));
  }

  if (exp_rx_power_pdch1_2_present) {
    HANDLE_CODE(exp_rx_power_pdch1_2.unpack(bref));
  }
  if (exp_rx_power_psch1_1_present) {
    HANDLE_CODE(exp_rx_power_psch1_1.unpack(bref));
  }
  if (exp_rx_power_psch1_2_present) {
    HANDLE_CODE(exp_rx_power_psch1_2.unpack(bref));
  }
  if (exp_rx_power_psch5_1_present) {
    HANDLE_CODE(exp_rx_power_psch5_1.unpack(bref));
  }
  if (exp_rx_power_psch5_2_present) {
    HANDLE_CODE(exp_rx_power_psch5_2.unpack(bref));
  }
  if (exp_rx_power_ptuch_present) {
    HANDLE_CODE(exp_rx_power_ptuch.unpack(bref));
  }
  return SRSASN_SUCCESS;
}
void power_control_cfg_com_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str(" pmbch_tx_power", pmbch_tx_power.to_string());

  if (exp_rx_power_prach_present) {
    j.write_fieldname(" exp_rx_power_prach");
    exp_rx_power_prach.to_json(j);
  }
  if (exp_rx_power_psych_present) {
    j.write_fieldname("exp_rx_power_psych");
    exp_rx_power_psych.to_json(j);
  }

  if (exp_rx_power_pdch1_1_present) {
    j.write_fieldname("exp_rx_power_pdch1_1");
    exp_rx_power_pdch1_1.to_json(j);
  }
  if (exp_rx_power_pdch1_2_present) {
    j.write_fieldname("exp_rx_power_pdch1_2");
    exp_rx_power_pdch1_2.to_json(j);
  }

  if (exp_rx_power_psch1_1_present) {
    j.write_fieldname(" exp_rx_power_psch1_1");
    exp_rx_power_psch1_1.to_json(j);
  }
  if (exp_rx_power_psch1_2_present) {
    j.write_fieldname(" exp_rx_power_psch1_2");
    exp_rx_power_psch1_2.to_json(j);
  }
  if (exp_rx_power_psch5_1_present) {
    j.write_fieldname("exp_rx_power_psch5_1");
    exp_rx_power_psch5_1.to_json(j);
  }
  if (exp_rx_power_psch5_2_present) {
    j.write_fieldname("exp_rx_power_psch5_2");
    exp_rx_power_psch5_2.to_json(j);
  }
  if (exp_rx_power_ptuch_present) {
    j.write_fieldname(" exp_rx_power_ptuch");
    exp_rx_power_ptuch.to_json(j);
  }
  j.end_obj();
}

////EphemerisParameters::= SEQUENCE
// EphemerisParametes
//SRSASN_CODE ephe_para_s::pack(bit_ref& bref) const
//{
//  bref.pack(ext, 1);
//  HANDLE_CODE(pack_integer(bref, sate_ephe_semi_major_axis, (uint32_t)0u, (uint32_t)36500000u));
//  HANDLE_CODE(pack_integer(bref, sate_ephe_ecce_e, (uint8_t)0u, (uint8_t)15u));
//  HANDLE_CODE(pack_integer(bref, sate_ephe_argu_of_peri, (uint16_t)0u, (uint16_t)1000u));
//  HANDLE_CODE(pack_integer(bref, sate_ephe_long_of_asc_node, (uint16_t)0u, (uint16_t)1000u));
//  HANDLE_CODE(pack_integer(bref, sate_ephe_inc_i, (uint16_t)0u, (uint16_t)1000u));
//  HANDLE_CODE(pack_integer(bref, sate_ephe_mean_anoma_m, (uint16_t)0u, (uint16_t)1000u));
//  HANDLE_CODE(pack_integer(bref, n_date, (uint16_t)1u, (uint16_t)16383u));
//  HANDLE_CODE(pack_integer(bref, n_time, (uint32_t)0u, (uint32_t)86400u));
//  return SRSASN_SUCCESS;
//}
//SRSASN_CODE ephe_para_s::unpack(cbit_ref& bref)
//{
//  bref.unpack(ext, 1);
//  HANDLE_CODE(unpack_integer(sate_ephe_semi_major_axis, bref, (uint32_t)0u, (uint32_t)36500000u));
//  HANDLE_CODE(unpack_integer(sate_ephe_ecce_e, bref, (uint8_t)0u, (uint8_t)15u));
//  HANDLE_CODE(unpack_integer(sate_ephe_argu_of_peri, bref, (uint16_t)0u, (uint16_t)1000u));
//  HANDLE_CODE(unpack_integer(sate_ephe_long_of_asc_node, bref, (uint16_t)0u, (uint16_t)1000u));
//  HANDLE_CODE(unpack_integer(sate_ephe_inc_i, bref, (uint16_t)0u, (uint16_t)1000u));
//  HANDLE_CODE(unpack_integer(sate_ephe_mean_anoma_m, bref, (uint16_t)0u, (uint16_t)1000u));
//  HANDLE_CODE(unpack_integer(n_date, bref, (uint16_t)1u, (uint16_t)16383u));
//  HANDLE_CODE(unpack_integer(n_time, bref, (uint32_t)0u, (uint32_t)86400u));
//  return SRSASN_SUCCESS;
//}
//void ephe_para_s::to_json(json_writer& j) const
//{
//  j.start_obj();
//  j.write_int("satelliteEphemerisSemiMajorAxis", sate_ephe_semi_major_axis);
//  j.write_int("satelliteEphemerisEccentricityE", sate_ephe_ecce_e);
//  j.write_int("satelliteEphemerisArgumentOfPeriapsis", sate_ephe_argu_of_peri);
//  j.write_int("satelliteEphemerisLongitudeOfAscendingNode", sate_ephe_long_of_asc_node);
//  j.write_int("satelliteEphemerisInclinationI", sate_ephe_inc_i);
//  j.write_int("satelliteEphemerisMeanAnomalyM", sate_ephe_mean_anoma_m);
//  j.write_int("nDate", n_date);
//  j.write_int("nTime", n_time);
//  j.end_obj();
//}

////RadioResourceConfigCommon  :: =	SEQUENCE
SRSASN_CODE redio_resour_cfg_co_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(rach_cfg_com_present, 1));
  HANDLE_CODE(bref.pack(agch_cfg_com_present, 1));
  HANDLE_CODE(bref.pack(p_max_present, 1));
  HANDLE_CODE(bref.pack(frame_offset_present, 1));
  HANDLE_CODE(bref.pack(band_id_present, 1));
  HANDLE_CODE(bref.pack(freq_id_present, 1));
  HANDLE_CODE(bref.pack(sec_syn_group_id_present, 1));
  HANDLE_CODE(bref.pack(ephem_para_t_sat_present, 1));
  HANDLE_CODE(bref.pack(mib_Re_Fra_num_present, 1));

  HANDLE_CODE(dis_to_beam_center.pack(bref));
  if (rach_cfg_com_present) {
    HANDLE_CODE(rach_cfg_com.pack(bref));
  }
  if (agch_cfg_com_present) {
    HANDLE_CODE(agch_cfg_com_n.pack(bref));
  }
  if (p_max_present) {
    HANDLE_CODE(p_max.pack(bref));
  }
  HANDLE_CODE(power_control_cfg_com.pack(bref));
  if (frame_offset_present) {
    HANDLE_CODE(frame_offset.pack(bref));
  }
  if (band_id_present) {
    HANDLE_CODE(bcch_band_id.pack(bref));
  }
  if (freq_id_present) {
    HANDLE_CODE(freq_id_n.pack(bref));
  }
  if (sec_syn_group_id_present) {
    HANDLE_CODE(pack_integer(bref, sec_syn_group_id, (uint8_t)1u, (uint8_t)48u));
  }
  if (ephem_para_t_sat_present) {
    HANDLE_CODE(ephe_para_t_sat.pack(bref));
  }
  if (mib_Re_Fra_num_present)
  {
    HANDLE_CODE(mib_Re_Fra_num.pack(bref));
  }
  HANDLE_CODE(naviInfo_band_id.pack(bref));
  HANDLE_CODE(naviInfo_fre_id.pack(bref));
  HANDLE_CODE(naviInfo_slot_ass.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE redio_resour_cfg_co_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(rach_cfg_com_present, 1));
  HANDLE_CODE(bref.unpack(agch_cfg_com_present, 1));
  HANDLE_CODE(bref.unpack(p_max_present, 1));
  HANDLE_CODE(bref.unpack(frame_offset_present, 1));
  HANDLE_CODE(bref.unpack(band_id_present, 1));
  HANDLE_CODE(bref.unpack(freq_id_present, 1));
  HANDLE_CODE(bref.unpack(sec_syn_group_id_present, 1));
  HANDLE_CODE(bref.unpack(ephem_para_t_sat_present, 1));
  HANDLE_CODE(bref.unpack(mib_Re_Fra_num_present, 1));

  HANDLE_CODE(dis_to_beam_center.unpack(bref));
  if (rach_cfg_com_present) {
    HANDLE_CODE(rach_cfg_com.unpack(bref));
  }
  if (agch_cfg_com_present) {
    HANDLE_CODE(agch_cfg_com_n.unpack(bref));
  }
  if (p_max_present) {
    HANDLE_CODE(p_max.unpack(bref));
  }
  HANDLE_CODE(power_control_cfg_com.unpack(bref));
  if (frame_offset_present) {
    HANDLE_CODE(frame_offset.unpack(bref));
  }
  if (band_id_present) {
    HANDLE_CODE(bcch_band_id.unpack(bref));
  }
  if (freq_id_present) {
    HANDLE_CODE(freq_id_n.unpack(bref));
  }
  if (sec_syn_group_id_present) {
    HANDLE_CODE(unpack_integer(sec_syn_group_id, bref, (uint8_t)1u, (uint8_t)48u));
  }
  if (ephem_para_t_sat_present) {
    HANDLE_CODE(ephe_para_t_sat.unpack(bref));
  }
  if (mib_Re_Fra_num_present) {
    HANDLE_CODE(mib_Re_Fra_num.unpack(bref));
  }
  HANDLE_CODE(naviInfo_band_id.unpack(bref));
  HANDLE_CODE(naviInfo_fre_id.unpack(bref));
  HANDLE_CODE(naviInfo_slot_ass.unpack(bref));
  return SRSASN_SUCCESS;
}
void redio_resour_cfg_co_s::to_json(json_writer& j) const
{
  j.start_obj();

  j.write_str(" dis_to_beam_center", dis_to_beam_center.to_string());

  if (rach_cfg_com_present) {
    j.write_fieldname("rach_cfg_com");
    rach_cfg_com.to_json(j);
  }
  if (agch_cfg_com_present) {
    j.write_fieldname("agch_cfg_com_present");
    agch_cfg_com_n.to_json(j);
  }
  if (p_max_present) {
    j.write_fieldname(" p_max");
    p_max.to_json(j);
  }
  j.write_fieldname("power_control_cfg_com");
  power_control_cfg_com.to_json(j);

  if (frame_offset_present) {
    j.write_str("frame_offset", frame_offset.to_string());
  }

  if (band_id_present) {
    j.write_fieldname("bcch_band_id");
    bcch_band_id.to_json(j);
  }
  if (freq_id_present) {
    j.write_fieldname(" freq_id_n");
    freq_id_n.to_json(j);
  }
  j.write_int("sec_syn_group_id", sec_syn_group_id);
  if (sec_syn_group_id_present) {
    j.write_int("sec_syn_group_id", sec_syn_group_id);
  }
  if (ephem_para_t_sat_present) {
    j.write_fieldname("ephe_para_t_sat");
    ephe_para_t_sat.to_json(j);
  }
  if (mib_Re_Fra_num_present) {
    j.write_str("mibRelativeFrameNum", mib_Re_Fra_num.to_string());
  }
  j.write_fieldname("naviInfo-bandIdentity");
  naviInfo_band_id.to_json(j);
  j.write_fieldname("naviInfo-frequencyIdentity");
  naviInfo_fre_id.to_json(j);
  j.write_str("naviInfo-slotAssignment", naviInfo_slot_ass.to_string());
  j.end_obj();
}

//  //RRCConnectionReconfiguration-r1-IEs::= SEQUENCE
SRSASN_CODE rrc_con_recfg_r1_ies_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(meas_cfg_present, 1));
  HANDLE_CODE(bref.pack(mobility_contro_present, 1));
  HANDLE_CODE(bref.pack(dedi_info_nas_n_present, 1));
  HANDLE_CODE(bref.pack(redio_resour_cfg_com_present, 1));
  HANDLE_CODE(bref.pack(redio_resour_cfg_dedi_present, 1));
  HANDLE_CODE(bref.pack(security_cfg_ho_present, 1));

  if (meas_cfg_present) {
    HANDLE_CODE(meas_cfg.pack(bref));
  }
  if (mobility_contro_present) {
    HANDLE_CODE(mobility_contro.pack(bref));
  }
  if (dedi_info_nas_n_present) {
    HANDLE_CODE(pack_dyn_seq_of(bref, dedi_info_nas_n, 1, 4));
  }
  if (redio_resour_cfg_com_present) {
    HANDLE_CODE(redio_resour_cfg_co.pack(bref));
  }
  if (redio_resour_cfg_dedi_present) {
    HANDLE_CODE(redio_resour_cfg_dedi.pack(bref));
  }
  if (security_cfg_ho_present) {
    HANDLE_CODE(security_cfg_ho.pack(bref));
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE rrc_con_recfg_r1_ies_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(meas_cfg_present, 1));
  HANDLE_CODE(bref.unpack(mobility_contro_present, 1));
  HANDLE_CODE(bref.unpack(dedi_info_nas_n_present, 1));
  HANDLE_CODE(bref.unpack(redio_resour_cfg_com_present, 1));
  HANDLE_CODE(bref.unpack(redio_resour_cfg_dedi_present, 1));
  HANDLE_CODE(bref.unpack(security_cfg_ho_present, 1));
  if (meas_cfg_present) {
    HANDLE_CODE(meas_cfg.unpack(bref));
  }
  if (mobility_contro_present) {
    HANDLE_CODE(mobility_contro.unpack(bref));
  }
  if (dedi_info_nas_n_present) {
    HANDLE_CODE(unpack_dyn_seq_of(dedi_info_nas_n, bref, 1, 4));
  }
  if (redio_resour_cfg_com_present) {
    HANDLE_CODE(redio_resour_cfg_co.unpack(bref));
  }
  if (redio_resour_cfg_dedi_present) {
    HANDLE_CODE(redio_resour_cfg_dedi.unpack(bref));
  }
  if (security_cfg_ho_present) {
    HANDLE_CODE(security_cfg_ho.unpack(bref));
  }
  return SRSASN_SUCCESS;
}
void rrc_con_recfg_r1_ies_s::to_json(json_writer& j) const
{
  j.start_obj();
  if (meas_cfg_present) {
    j.write_fieldname("meas_cfg");
    meas_cfg.to_json(j);
  }
  if (mobility_contro_present) {
    j.write_fieldname("mobility_contro");
    mobility_contro.to_json(j);
  }
  j.start_array("dedi_info_nas_n");
  for (uint16_t i1 = 0; i1 < dedi_info_nas_n.size(); i1++) {
    dedi_info_nas_n[i1].to_json(j);
  }
  j.end_array();

  if (redio_resour_cfg_com_present) {
    j.write_fieldname(" redio_resour_cfg_co");
    redio_resour_cfg_co.to_json(j);
  }
  if (redio_resour_cfg_dedi_present) {
    j.write_fieldname(" redio_resour_cfg_dedi");
    redio_resour_cfg_dedi.to_json(j);
  }
  if (security_cfg_ho_present) {
    j.write_fieldname("security_cfg_ho");
    security_cfg_ho.to_json(j);
  }
  j.end_obj();
}

SRSASN_CODE def_cfg_s::pack(bit_ref& bref) const
{
  return SRSASN_SUCCESS;
}
SRSASN_CODE def_cfg_s::unpack(cbit_ref& bref)
{
  return SRSASN_SUCCESS;
}
void def_cfg_s::to_json(json_writer& j) const
{
  j.start_obj();

  j.end_obj();
}

//// SRB-ToAdd ::= SEQUENCE// SRB-ToAdd ::= SEQUENCE
SRSASN_CODE srb_to_addd_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(def_cfg.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE srb_to_addd_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(def_cfg.unpack(bref));
  return SRSASN_SUCCESS;
}
void srb_to_addd_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("def_cfg");
  def_cfg.to_json(j);
  j.end_obj();
}

////PDU-SessionID ::=                   INTEGER(0..255),
SRSASN_CODE pdu_sess_id_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(pack_integer(bref, pdu_ses_id, (uint16_t)0u, (uint16_t)255u));
  return SRSASN_SUCCESS;
}
SRSASN_CODE pdu_sess_id_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(unpack_integer(pdu_ses_id, bref, (uint16_t)0u, (uint16_t)255u));
  return SRSASN_SUCCESS;
}
void pdu_sess_id_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_int("pdu_ses_id", pdu_ses_id);
  j.end_obj();
}

// QFI::=                   INTEGER (0..maxQFI)mappedQoS-FlowsToAdd
SRSASN_CODE map_qos_flows_to_add_l::pack(bit_ref& bref) const
{
  HANDLE_CODE(pack_integer(bref, qfi, (uint8_t)0u, (uint8_t)3u));
  return SRSASN_SUCCESS;
}
SRSASN_CODE map_qos_flows_to_add_l::unpack(cbit_ref& bref)
{
  HANDLE_CODE(unpack_integer(qfi, bref, (uint8_t)0u, (uint8_t)3u));
  return SRSASN_SUCCESS;
}
void map_qos_flows_to_add_l::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_int("qfi", qfi);
  j.end_obj();
}

// QFI::=          INTEGER (0..maxQFI)
SRSASN_CODE map_qos_flow_to_rel_l::pack(bit_ref& bref) const
{
  HANDLE_CODE(pack_integer(bref, qfi, (uint8_t)0u, (uint8_t)3u));
  return SRSASN_SUCCESS;
}
SRSASN_CODE map_qos_flow_to_rel_l::unpack(cbit_ref& bref)
{
  HANDLE_CODE(unpack_integer(qfi, bref, (uint8_t)0u, (uint8_t)3u));
  return SRSASN_SUCCESS;
}
void map_qos_flow_to_rel_l::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_int("qfi", qfi);
  j.end_obj();
}

// //SDAP-Config ::=           SEQUENCE
SRSASN_CODE sdap_cfg_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(map_qos_flows_to_add_present, 1));
  HANDLE_CODE(bref.pack(map_qos_flow_to_rel_present, 1));

  HANDLE_CODE(pdu_sess_id.pack(bref));
  HANDLE_CODE(sdap_header_dl.pack(bref));
  HANDLE_CODE(sdap_header_ul.pack(bref));
  HANDLE_CODE(bref.pack(default_drb, 1));
  if (map_qos_flows_to_add_present) {
    HANDLE_CODE(pack_dyn_seq_of(bref, map_qos_flows_to_add, 1, 4));
  }
  if (map_qos_flow_to_rel_present) {
    HANDLE_CODE(pack_dyn_seq_of(bref, map_qos_flow_to_rel, 1, 4));
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE sdap_cfg_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(map_qos_flows_to_add_present, 1));
  HANDLE_CODE(bref.unpack(map_qos_flow_to_rel_present, 1));
  HANDLE_CODE(pdu_sess_id.unpack(bref));
  HANDLE_CODE(sdap_header_dl.unpack(bref));
  HANDLE_CODE(sdap_header_ul.unpack(bref));
  HANDLE_CODE(bref.unpack(default_drb, 1));
  if (map_qos_flows_to_add_present) {
    HANDLE_CODE(unpack_dyn_seq_of(map_qos_flows_to_add, bref, 1, 4));
  }
  if (map_qos_flow_to_rel_present) {
    HANDLE_CODE(unpack_dyn_seq_of(map_qos_flow_to_rel, bref, 1, 4));
  }
  return SRSASN_SUCCESS;
}
void sdap_cfg_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("pdu_sess_id");
  pdu_sess_id.to_json(j);
  j.write_str("sdap_header_dl", sdap_header_dl.to_string());
  j.write_str("sdap_header_ul", sdap_header_ul.to_string());
  j.write_bool("default_drb", default_drb);

  if (map_qos_flows_to_add_present) {
    j.start_array("map_qos_flows_to_add");
    for (const auto& e1 : map_qos_flows_to_add) {
      e1.to_json(j);
    }
    j.end_array();
  }

  j.start_array("map_qos_flow_to_rel");
  if (map_qos_flow_to_rel_present) {
    for (const auto& e1 : map_qos_flow_to_rel) {
      e1.to_json(j);
    }
    j.end_array();
  }
  j.end_obj();
}

// rlc-AM
SRSASN_CODE rlc_am_s::pack(bit_ref& bref) const
{
  bref.pack(stat_rep_req, 1);
  return SRSASN_SUCCESS;
}
SRSASN_CODE rlc_am_s::unpack(cbit_ref& bref)
{
  bref.unpack(stat_rep_req, 1);
  return SRSASN_SUCCESS;
}
void rlc_am_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_bool("stat_rep_req", stat_rep_req);
  j.end_obj();
}
// rlc-UM
SRSASN_CODE rlc_um_s::pack(bit_ref& bref) const
{

  return SRSASN_SUCCESS;
}
SRSASN_CODE rlc_um_s::unpack(cbit_ref& bref)
{

  return SRSASN_SUCCESS;
}
void rlc_um_s::to_json(json_writer& j) const
{
  j.start_obj();

  j.end_obj();
}
// rlc-TM
SRSASN_CODE rlc_tm_s::pack(bit_ref& bref) const
{

  return SRSASN_SUCCESS;
}
SRSASN_CODE rlc_tm_s::unpack(cbit_ref& bref)
{

  return SRSASN_SUCCESS;
}
void rlc_tm_s::to_json(json_writer& j) const
{
  j.start_obj();

  j.end_obj();
}

// headerCompression         CHOICE::not_used_s
SRSASN_CODE not_used_s::pack(bit_ref& bref) const
{
  return SRSASN_SUCCESS;
}
SRSASN_CODE not_used_s::unpack(cbit_ref& bref)
{
  return SRSASN_SUCCESS;
}
void not_used_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.end_obj();
}

// headerCompression         CHOIC::ro_hc_s::profiles
SRSASN_CODE pro_file_s::pack(bit_ref& bref) const
{
  bref.pack(pro_0x_0002, 1);
  bref.pack(pro_0x_0004, 1);
  bref.pack(pro_0x_0006, 1);
  bref.pack(pro_0x_0102, 1);
  bref.pack(pro_0x_0104, 1);
  return SRSASN_SUCCESS;
}
SRSASN_CODE pro_file_s::unpack(cbit_ref& bref)
{
  bref.unpack(pro_0x_0002, 1);
  bref.unpack(pro_0x_0004, 1);
  bref.unpack(pro_0x_0006, 1);
  bref.unpack(pro_0x_0102, 1);
  bref.unpack(pro_0x_0104, 1);
  return SRSASN_SUCCESS;
}
void pro_file_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_bool("pro_0x_0002", pro_0x_0002);
  j.write_bool("pro_0x_0004", pro_0x_0004);
  j.write_bool("pro_0x_0006", pro_0x_0006);
  j.write_bool("pro_0x_0102", pro_0x_0102);
  j.write_bool("pro_0x_0104", pro_0x_0104);
  j.end_obj();
}
// headerCompression         CHOIC::ro_hc_s
SRSASN_CODE ro_hc_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(pack_integer(bref, max_cid, (uint16_t)1u, (uint16_t)16383u));
  HANDLE_CODE(pro_file.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE ro_hc_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(unpack_integer(max_cid, bref, (uint16_t)1u, (uint16_t)16383u));
  HANDLE_CODE(pro_file.unpack(bref));
  return SRSASN_SUCCESS;
}
void ro_hc_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_int("max_cid", max_cid);
  j.write_fieldname("pro_file");
  pro_file.to_json(j);
  j.end_obj();
}
// headerCompression         CHOICE
void header_com_c_::destroy_()
{
  switch (type_) {
    case types::not_used_l:
      c.destroy<not_used_s>();
      break;
    case types::ro_hc:
      c.destroy<ro_hc_s>();
      break;
    default:
      break;
  }
}
void header_com_c_::set(types::options e)
{
  destroy_();
  type_ = e;
  switch (type_) {
    case types::not_used_l:
      c.init<not_used_s>();
      break;
    case types::ro_hc:
      c.init<ro_hc_s>();
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "header_com_c_");
  }
}
header_com_c_::header_com_c_(const header_com_c_& other)
{
  type_ = other.type();
  switch (type_) {
    case types::not_used_l:
      c.init(other.c.get<not_used_s>());
      break;
    case types::ro_hc:
      c.init(other.c.get<ro_hc_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "header_com_c_");
  }
}
header_com_c_& header_com_c_::operator=(const header_com_c_& other)
{
  if (this == &other) {
    return *this;
  }
  set(other.type());
  switch (type_) {
    case types::not_used_l:
      c.set(other.c.get<not_used_s>());
      break;
    case types::ro_hc:
      c.set(other.c.get<ro_hc_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "header_com_c_");
  }

  return *this;
}

not_used_s& header_com_c_::set_not_used_l()
{
  set(types::not_used_l);
  return c.get<not_used_s>();
}
ro_hc_s& header_com_c_::set_ro_hc()
{
  set(types::ro_hc);
  return c.get<ro_hc_s>();
}

void header_com_c_::to_json(json_writer& j) const
{
  j.start_obj();
  switch (type_) {
    case types::not_used_l:
      j.write_fieldname("not_used_l");
      c.get<not_used_s>().to_json(j);
      break;
    case types::ro_hc:
      j.write_fieldname("ro_hc");
      c.get<ro_hc_s>().to_json(j);
      break;
    default:
      log_invalid_choice_id(type_, "header_com_c_");
  }
  j.end_obj();
}
SRSASN_CODE header_com_c_::pack(bit_ref& bref) const
{
  type_.pack(bref);
  switch (type_) {
    case types::not_used_l:
      HANDLE_CODE(c.get<not_used_s>().pack(bref));
      break;
    case types::ro_hc:
      HANDLE_CODE(c.get<ro_hc_s>().pack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "header_com_c_");
      return SRSASN_ERROR_ENCODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE header_com_c_::unpack(cbit_ref& bref)
{
  types e;
  e.unpack(bref);
  set(e);
  switch (type_) {
    case types::not_used_l:
      HANDLE_CODE(c.get<not_used_s>().unpack(bref));
      break;
    case types::ro_hc:
      HANDLE_CODE(c.get<ro_hc_s>().unpack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "header_com_c_");
      return SRSASN_ERROR_DECODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
/// PDCP-Confg ::=        SEQUENCE
SRSASN_CODE pdcp_cofg_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(dis_timer_present, 1));
  HANDLE_CODE(bref.pack(rlc_am_present, 1));
  HANDLE_CODE(bref.pack(rlc_um_present, 1));
  HANDLE_CODE(bref.pack(rlc_tm_present, 1));
  HANDLE_CODE(bref.pack(integ_pro_present, 1));
  HANDLE_CODE(bref.pack(ciph_dis_present, 1));
  if (dis_timer_present) {
    HANDLE_CODE(dis_timer.pack(bref));
  }

  if (rlc_am_present) {
    HANDLE_CODE(rlc_am.pack(bref));
  }
  if (rlc_um_present) {
    HANDLE_CODE(rlc_um.pack(bref));
  }
  if (rlc_tm_present) {
    HANDLE_CODE(rlc_tm.pack(bref));
  }

  HANDLE_CODE(header_com.pack(bref));

  if (integ_pro_present) {
    HANDLE_CODE(integ_pro.pack(bref));
  }
  if (ciph_dis_present) {
    HANDLE_CODE(ciph_dis.pack(bref));
  }

  return SRSASN_SUCCESS;
}
SRSASN_CODE pdcp_cofg_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(dis_timer_present, 1));
  HANDLE_CODE(bref.unpack(rlc_am_present, 1));
  HANDLE_CODE(bref.unpack(rlc_um_present, 1));
  HANDLE_CODE(bref.unpack(rlc_tm_present, 1));
  HANDLE_CODE(bref.unpack(integ_pro_present, 1));
  HANDLE_CODE(bref.unpack(ciph_dis_present, 1));
  if (dis_timer_present) {
    HANDLE_CODE(dis_timer.unpack(bref));
  }

  if (rlc_am_present) {
    HANDLE_CODE(rlc_am.unpack(bref));
  }
  if (rlc_um_present) {
    HANDLE_CODE(rlc_um.unpack(bref));
  }
  if (rlc_tm_present) {
    HANDLE_CODE(rlc_tm.unpack(bref));
  }

  HANDLE_CODE(header_com.unpack(bref));

  if (integ_pro_present) {
    HANDLE_CODE(integ_pro.unpack(bref));
  }
  if (ciph_dis_present) {
    HANDLE_CODE(ciph_dis.unpack(bref));
  }

  return SRSASN_SUCCESS;
}
void pdcp_cofg_s::to_json(json_writer& j) const
{
  j.start_obj();
  if (dis_timer_present) {
    j.write_str("dis_timer", dis_timer.to_string());
  }

  if (rlc_am_present) {
    j.write_fieldname("rlc_am");
    rlc_am.to_json(j);
  }
  if (rlc_um_present) {
    j.write_fieldname("rlc_um");
    rlc_um.to_json(j);
  }
  if (rlc_tm_present) {
    j.write_fieldname("rlc_tm");
    rlc_tm.to_json(j);
  }
  j.write_fieldname("header_com");
  header_com.to_json(j);
  if (integ_pro_present) {
    j.write_str("integ_pro", integ_pro.to_string());
  }
  if (ciph_dis_present) {
    j.write_str("ciph_dis", ciph_dis.to_string());
  }
  j.end_obj();
}

// UL-AM-RLC ::=                      SEQUENCE
SRSASN_CODE ul_am_rlcc_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(t_poll_retran.pack(bref));
  HANDLE_CODE(poll_pdu.pack(bref));
  HANDLE_CODE(poll_byte.pack(bref));
  HANDLE_CODE(max_retx_thres_hold.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE ul_am_rlcc_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(t_poll_retran.unpack(bref));
  HANDLE_CODE(poll_pdu.unpack(bref));
  HANDLE_CODE(poll_byte.unpack(bref));
  HANDLE_CODE(max_retx_thres_hold.unpack(bref));
  return SRSASN_SUCCESS;
}
void ul_am_rlcc_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("t_poll_retran", t_poll_retran.to_string());
  j.write_str("poll_pdu", poll_pdu.to_string());
  j.write_str("poll_byte", poll_byte.to_string());
  j.write_str("max_retx_thres_hold", max_retx_thres_hold.to_string());
  j.end_obj();
}
// DL-UM-RLC ::=                        SEQUENCE
SRSASN_CODE dl_am_rlcc_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(t_reord.pack(bref));
  HANDLE_CODE(t_status_proh.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE dl_am_rlcc_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(t_reord.unpack(bref));
  HANDLE_CODE(t_status_proh.unpack(bref));

  return SRSASN_SUCCESS;
}
void dl_am_rlcc_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("t_reord", t_reord.to_string());
  j.write_str("t_status_proh", t_status_proh.to_string());

  j.end_obj();
}

// UL-UM-RLC ::=               SEQUENCE{
SRSASN_CODE ul_um_rlcc_s::pack(bit_ref& bref) const
{

  return SRSASN_SUCCESS;
}
SRSASN_CODE ul_um_rlcc_s::unpack(cbit_ref& bref)
{

  return SRSASN_SUCCESS;
}
void ul_um_rlcc_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.end_obj();
}

// DL-UM-RLC ::=                        SEQUENCE

SRSASN_CODE dl_um_rlcc_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(t_reord.pack(bref));

  return SRSASN_SUCCESS;
}
SRSASN_CODE dl_um_rlcc_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(t_reord.unpack(bref));

  return SRSASN_SUCCESS;
}
void dl_um_rlcc_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("t_reord", t_reord.to_string());

  j.end_obj();
}

// RLC-Config ::=         CHOICE

SRSASN_CODE am_s_::pack(bit_ref& bref) const
{
  HANDLE_CODE(ul_am_rlc.pack(bref));
  HANDLE_CODE(dl_am_rlc.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE am_s_::unpack(cbit_ref& bref)
{
  HANDLE_CODE(ul_am_rlc.unpack(bref));
  HANDLE_CODE(dl_am_rlc.unpack(bref));
  return SRSASN_SUCCESS;
}
void am_s_::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("ul_am_rlc");
  ul_am_rlc.to_json(j);
  j.write_fieldname("dl_am_rlc");
  dl_am_rlc.to_json(j);
  j.end_obj();
}

SRSASN_CODE um_bi_dir_s_::pack(bit_ref& bref) const
{
  HANDLE_CODE(ul_um_rlc.pack(bref));
  HANDLE_CODE(dl_um_rlc.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE um_bi_dir_s_::unpack(cbit_ref& bref)
{
  HANDLE_CODE(ul_um_rlc.unpack(bref));
  HANDLE_CODE(dl_um_rlc.unpack(bref));
  return SRSASN_SUCCESS;
}
void um_bi_dir_s_::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("ul_um_rlc");
  ul_um_rlc.to_json(j);
  j.write_fieldname("dl_um_rlc");
  dl_um_rlc.to_json(j);
  j.end_obj();
}

SRSASN_CODE tm_s::pack(bit_ref& bref) const
{
  return SRSASN_SUCCESS;
}
SRSASN_CODE tm_s::unpack(cbit_ref& bref)
{
  return SRSASN_SUCCESS;
}
void tm_s::to_json(json_writer& j) const
{
  j.start_obj();

  j.end_obj();
}

void rlc_cofg_c::destroy_()
{
  switch (type_) {
    case types::am:
      c.destroy<am_s_>();
      break;
    case types::um_bi_dir:
      c.destroy<um_bi_dir_s_>();
      break;
    case types::tm:
      c.destroy<tm_s>();
      break;
    default:
      break;
  }
}
void rlc_cofg_c::set(types::options e)
{
  destroy_();
  type_ = e;
  switch (type_) {
    case types::am:
      c.init<am_s_>();
      break;
    case types::um_bi_dir:
      c.init<um_bi_dir_s_>();
      break;
    case types::tm:
      c.init<tm_s>();
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "rlc_cofg_c");
  }
}
rlc_cofg_c::rlc_cofg_c(const rlc_cofg_c& other)
{
  type_ = other.type();
  switch (type_) {
    case types::am:
      c.init(other.c.get<am_s_>());
      break;
    case types::um_bi_dir:
      c.init(other.c.get<um_bi_dir_s_>());
      break;
    case types::tm:
      c.init(other.c.get<tm_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "rlc_cofg_c");
  }
}
rlc_cofg_c& rlc_cofg_c::operator=(const rlc_cofg_c& other)
{
  if (this == &other) {
    return *this;
  }
  set(other.type());
  switch (type_) {
    case types::am:
      c.set(other.c.get<am_s_>());
      break;
    case types::um_bi_dir:
      c.set(other.c.get<um_bi_dir_s_>());
      break;
    case types::tm:
      c.set(other.c.get<tm_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "rlc_cofg_c");
  }

  return *this;
}

am_s_& rlc_cofg_c::set_am()
{
  set(types::am);
  return c.get<am_s_>();
}
um_bi_dir_s_& rlc_cofg_c::set_um_bi_dir()
{
  set(types::um_bi_dir);
  return c.get<um_bi_dir_s_>();
}
tm_s& rlc_cofg_c::set_tm()
{
  set(types::tm);
  return c.get<tm_s>();
}
void rlc_cofg_c::to_json(json_writer& j) const
{
  j.start_obj();
  switch (type_) {
    case types::am:
      j.write_fieldname("am");
      c.get<am_s_>().to_json(j);
      break;
    case types::um_bi_dir:
      j.write_fieldname("um_bi_dir");
      c.get<um_bi_dir_s_>().to_json(j);
      break;
    case types::tm:
      j.write_fieldname("tm");
      c.get<tm_s>().to_json(j);
    default:
      log_invalid_choice_id(type_, "rlc_cofg_c");
  }
  j.end_obj();
}
SRSASN_CODE rlc_cofg_c::pack(bit_ref& bref) const
{
  type_.pack(bref);
  switch (type_) {
    case types::am:
      HANDLE_CODE(c.get<am_s_>().pack(bref));
      break;
    case types::um_bi_dir:
      HANDLE_CODE(c.get<um_bi_dir_s_>().pack(bref));
      break;
    case types::tm:
      HANDLE_CODE(c.get<tm_s>().pack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "rlc_cofg_c");
      return SRSASN_ERROR_ENCODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE rlc_cofg_c::unpack(cbit_ref& bref)
{
  types e;
  e.unpack(bref);
  set(e);
  switch (type_) {
    case types::am:
      HANDLE_CODE(c.get<am_s_>().unpack(bref));
      break;
    case types::um_bi_dir:
      HANDLE_CODE(c.get<um_bi_dir_s_>().unpack(bref));
      break;
    case types::tm:
      HANDLE_CODE(c.get<tm_s>().unpack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "rlc_cofg_c");
      return SRSASN_ERROR_DECODE_FAIL;
  }
  return SRSASN_SUCCESS;
}

////ul-SpecificParameters                 SEQUENCE
SRSASN_CODE ul_spec_para_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(bref.pack(log_chan_goup_present, 1));
  HANDLE_CODE(pack_integer(bref, priority, (uint8_t)1u, (uint8_t)16u));
  HANDLE_CODE(prio_bit_rate.pack(bref));
  HANDLE_CODE(buck_size_dura.pack(bref));
  if (log_chan_goup_present) {
    HANDLE_CODE(pack_integer(bref, log_chan_goup, (uint8_t)0u, (uint8_t)3u));
  }

  return SRSASN_SUCCESS;
}
SRSASN_CODE ul_spec_para_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(bref.unpack(log_chan_goup_present, 1));
  HANDLE_CODE(unpack_integer(priority, bref, (uint8_t)1u, (uint8_t)16u));
  HANDLE_CODE(prio_bit_rate.unpack(bref));
  HANDLE_CODE(buck_size_dura.unpack(bref));
  if (log_chan_goup_present) {
    HANDLE_CODE(unpack_integer(log_chan_goup, bref, (uint8_t)0u, (uint8_t)3u));
  }

  return SRSASN_SUCCESS;
}
void ul_spec_para_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_int("priority", priority);
  j.write_str("prio_bit_rate", prio_bit_rate.to_string());
  j.write_str("buck_size_dura", buck_size_dura.to_string());
  if (log_chan_goup_present) {
    j.write_int("log_chan_goup", log_chan_goup);
  }
  j.end_obj();
}

// LogicalChannelConfig
SRSASN_CODE log_chan_cfg_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(ul_spec_para_present, 1));
  if (ul_spec_para_present) {
    HANDLE_CODE(ul_spec_para.pack(bref));
  }

  return SRSASN_SUCCESS;
}
SRSASN_CODE log_chan_cfg_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(ul_spec_para_present, 1));
  if (ul_spec_para_present) {
    HANDLE_CODE(ul_spec_para.unpack(bref));
  }
  return SRSASN_SUCCESS;
}
void log_chan_cfg_s::to_json(json_writer& j) const
{
  j.start_obj();
  if (ul_spec_para_present) {
    j.write_fieldname("ul_spec_para");
    ul_spec_para.to_json(j);
  }

  j.end_obj();
}

///// DRB-ToAddMod :: = SEQUENCE
SRSASN_CODE drb_to_add_modi_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(sdap_cfg_present, 1));
  HANDLE_CODE(bref.pack(pdcp_cfg_present, 1));
  HANDLE_CODE(bref.pack(rlc_cfg_present, 1));
  HANDLE_CODE(bref.pack(log_chan_cfg_present, 1));

  if (sdap_cfg_present) {
    HANDLE_CODE(sdap_cfg.pack(bref));
  }

  HANDLE_CODE(pack_integer(bref, drb_id, (uint8_t)3u, (uint8_t)6u));

  if (pdcp_cfg_present) {
    HANDLE_CODE(pdcp_cfg.pack(bref));
  }
  if (rlc_cfg_present) {
    HANDLE_CODE(rlc_cfg.pack(bref));
  }
  if (log_chan_cfg_present) {
    HANDLE_CODE(log_chan_cfg.pack(bref));
  }

  return SRSASN_SUCCESS;
}
SRSASN_CODE drb_to_add_modi_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(sdap_cfg_present, 1));
  HANDLE_CODE(bref.unpack(pdcp_cfg_present, 1));
  HANDLE_CODE(bref.unpack(rlc_cfg_present, 1));
  HANDLE_CODE(bref.unpack(log_chan_cfg_present, 1));

  if (sdap_cfg_present) {
    HANDLE_CODE(sdap_cfg.unpack(bref));
  }

  HANDLE_CODE(unpack_integer(drb_id, bref, (uint8_t)3u, (uint8_t)6u));

  if (pdcp_cfg_present) {
    HANDLE_CODE(pdcp_cfg.unpack(bref));
  }
  if (rlc_cfg_present) {
    HANDLE_CODE(rlc_cfg.unpack(bref));
  }
  if (log_chan_cfg_present) {
    HANDLE_CODE(log_chan_cfg.unpack(bref));
  }

  return SRSASN_SUCCESS;
}
void drb_to_add_modi_s::to_json(json_writer& j) const
{
  j.start_obj();
  if (sdap_cfg_present) {
    j.write_fieldname("sdap_cfg");
    sdap_cfg.to_json(j);
  }

  j.write_int("drb_id", drb_id);

  if (pdcp_cfg_present) {
    j.write_fieldname("pdcp_cfg");
    pdcp_cfg.to_json(j);
  }

  if (rlc_cfg_present) {
    j.write_fieldname("rlc_cfg");
    rlc_cfg.to_json(j);
  }

  if (log_chan_cfg_present) {
    j.write_fieldname("log_chan_cfg");
    log_chan_cfg.to_json(j);
  }
  j.end_obj();
}

// DRB-ToReleseList
SRSASN_CODE drb_id_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(pack_integer(bref, drb_id, (uint8_t)3u, (uint8_t)6u));
  return SRSASN_SUCCESS;
}
SRSASN_CODE drb_id_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(unpack_integer(drb_id, bref, (uint8_t)3u, (uint8_t)6u));

  return SRSASN_SUCCESS;
}
void drb_id_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_int("drb_id", drb_id);
  j.end_obj();
}

////s_rnti(bit6)
SRSASN_CODE s_rnti_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(srnti.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE s_rnti_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(srnti.unpack(bref));
  return SRSASN_SUCCESS;
}
void s_rnti_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("srnti", srnti.to_string());
  j.end_obj();
}

// ChannelType:: = SEQUENCE
/*SRSASN_CODE chan_type_s::pack(bit_ref& bref) const
{
    bref.pack(ext, 1);
    HANDLE_CODE(pack_integer(bref, pdtch_phy_code, (uint16_t)0u, (uint16_t)511u));
    return SRSASN_SUCCESS;
}
SRSASN_CODE chan_type_s::unpack(cbit_ref& bref)
{
    bref.unpack(ext, 1);
    HANDLE_CODE(unpack_integer(pdtch_phy_code, bref, (uint16_t)0u, (uint16_t)511u));
    return SRSASN_SUCCESS;
}
void chan_type_s::to_json(json_writer& j) const
{
    j.start_obj();
    j.write_int("pdtch_phy_code", pdtch_phy_code);
    j.end_obj();
}*/

// BandIdentity ::= SEQUENCE
/*SRSASN_CODE band_id_s::pack(bit_ref& bref) const
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
}*/
// pdtch_code_s                pdtch_code;
SRSASN_CODE pdtch_code_s_s::pack(bit_ref& bref) const
{
  bref.pack(ext,1);
  HANDLE_CODE(pack_integer(bref, pdt_phy_code, (uint16_t)0u, (uint16_t)511u));
  return SRSASN_SUCCESS;
}
SRSASN_CODE pdtch_code_s_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext,1);
  HANDLE_CODE(unpack_integer(pdt_phy_code, bref, (uint16_t)0u, (uint16_t)511u));
  return SRSASN_SUCCESS;
}
void pdtch_code_s_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_int("pdt_phy_code", pdt_phy_code);
  j.end_obj();
}

// PhysicalChannel-Config::= SEQUENCE
SRSASN_CODE phy_chan_cfg_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(band_id_present, 1));
  HANDLE_CODE(bref.pack(freq_id_present, 1));
  HANDLE_CODE(bref.pack(slot_ass_present, 1));
  HANDLE_CODE(bref.pack(pdtch_code_present, 1));
  HANDLE_CODE(bref.pack(sche_type_present, 1));
  HANDLE_CODE(bref.pack(voice_type_present, 1));

  HANDLE_CODE(s_rnti.pack(bref));
  HANDLE_CODE(chan_type.pack(bref));
  if (band_id_present) {
    HANDLE_CODE(band_id.pack(bref));
  }
  if (freq_id_present) {
    HANDLE_CODE(freq_id.pack(bref));
  }
  if (slot_ass_present) {
    HANDLE_CODE(slot_ass.pack(bref));
  }
  HANDLE_CODE(direc_t.pack(bref));
  if (pdtch_code_present) {
    HANDLE_CODE(pdtch_code.pack(bref));
  }
  if (sche_type_present) {
    HANDLE_CODE(sche_type.pack(bref));
  }
  if (voice_type_present) {
    HANDLE_CODE(voice_type.pack(bref));
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE phy_chan_cfg_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(band_id_present, 1));
  HANDLE_CODE(bref.unpack(freq_id_present, 1));
  HANDLE_CODE(bref.unpack(slot_ass_present, 1));
  HANDLE_CODE(bref.unpack(pdtch_code_present, 1));
  HANDLE_CODE(bref.unpack(sche_type_present, 1));
  HANDLE_CODE(bref.unpack(voice_type_present, 1));

  HANDLE_CODE(s_rnti.unpack(bref));
  HANDLE_CODE(chan_type.unpack(bref));
  if (band_id_present) {
    HANDLE_CODE(band_id.unpack(bref));
  }
  if (freq_id_present) {
    HANDLE_CODE(freq_id.unpack(bref));
  }
  if (slot_ass_present) {
    HANDLE_CODE(slot_ass.unpack(bref));
  }
  HANDLE_CODE(direc_t.unpack(bref));
  if (pdtch_code_present) {
    HANDLE_CODE(pdtch_code.unpack(bref));
  }
  if (sche_type_present) {
    HANDLE_CODE(sche_type.unpack(bref));
  }
  if (voice_type_present) {
    HANDLE_CODE(voice_type.unpack(bref));
  }
  return SRSASN_SUCCESS;
}
void phy_chan_cfg_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("s_rnti");
  s_rnti.to_json(j);
  j.write_str("chan_type", chan_type.to_string());
  if (band_id_present) {
    j.write_fieldname("band_id");
    band_id.to_json(j);
  }

  if (freq_id_present) {
    j.write_fieldname("freq_id");
    freq_id.to_json(j);
  }

  if (slot_ass_present) {
    j.write_str(" slot_ass", slot_ass.to_string());
  }
  j.write_str("direc_t", direc_t.to_string());
  if (pdtch_code_present) {
    j.write_fieldname(" pdtch_code");
    pdtch_code.to_json(j);
  }
  if (sche_type_present) {
    j.write_str("sche_type", sche_type.to_string());
  }
  if (voice_type_present) {
    j.write_str("voice_type", voice_type.to_string());
  }
  j.end_obj();
}

/// SecurityAlgorithmConfig ::=               SEQUENCE
SRSASN_CODE sec_alg_cfg_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(integ_prot_alg_present, 1));
  HANDLE_CODE(coph_alg.pack(bref));
  if (integ_prot_alg_present) {
    HANDLE_CODE(intef_prot_alg.pack(bref));
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE sec_alg_cfg_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(integ_prot_alg_present, 1));
  HANDLE_CODE(coph_alg.unpack(bref));
  if (integ_prot_alg_present) {
    HANDLE_CODE(intef_prot_alg.unpack(bref));
  }
  return SRSASN_SUCCESS;
}
void sec_alg_cfg_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("coph_alg", coph_alg.to_string());
  if (integ_prot_alg_present) {
    j.write_str("intef_prot_alg", intef_prot_alg.to_string());
  }
  j.end_obj();
}

////securityPayload-Normal		OCTET STRING.
SRSASN_CODE sec_pay_load_normal_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(sec_pay_load_nor.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE sec_pay_load_normal_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(sec_pay_load_nor.unpack(bref));
  return SRSASN_SUCCESS;
}
void sec_pay_load_normal_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("sec_pay_load_nor", sec_pay_load_nor.to_string());
  j.end_obj();
}
// securityPayload-TtoT			OCTET STRING.
SRSASN_CODE sec_pay_load_t_iot_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(sec_pay_load_yiot.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE sec_pay_load_t_iot_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(sec_pay_load_yiot.unpack(bref));
  return SRSASN_SUCCESS;
}
void sec_pay_load_t_iot_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("sec_pay_load_yiot", sec_pay_load_yiot.to_string());
  j.end_obj();
}

// securityPayload	    CHOICE
void sec_pay_load_c_::destroy_()
{
  switch (type_) {
    case types::sec_pay_load_normal:
      c.destroy<sec_pay_load_normal_s>();
      break;
    case types::sec_pay_load_t_iot:
      c.destroy<sec_pay_load_t_iot_s>();
      break;
    default:
      break;
  }
}
void sec_pay_load_c_::set(types::options e)
{
  destroy_();
  type_ = e;
  switch (type_) {
    case types::sec_pay_load_normal:
      c.init<sec_pay_load_normal_s>();
      break;
    case types::sec_pay_load_t_iot:
      c.init<sec_pay_load_t_iot_s>();
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "sec_pay_load_c_");
  }
}
sec_pay_load_c_::sec_pay_load_c_(const sec_pay_load_c_& other)
{
  type_ = other.type();
  switch (type_) {
    case types::sec_pay_load_normal:
      c.init(other.c.get<sec_pay_load_normal_s>());
      break;
    case types::sec_pay_load_t_iot:
      c.init(other.c.get<sec_pay_load_t_iot_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "sec_pay_load_c_");
  }
}
sec_pay_load_c_& sec_pay_load_c_::operator=(const sec_pay_load_c_& other)
{
  if (this == &other) {
    return *this;
  }
  set(other.type());
  switch (type_) {
    case types::sec_pay_load_normal:
      c.set(other.c.get<sec_pay_load_normal_s>());
      break;
    case types::sec_pay_load_t_iot:
      c.set(other.c.get<sec_pay_load_t_iot_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "sec_pay_load_c_");
  }

  return *this;
}

sec_pay_load_normal_s& sec_pay_load_c_::set_sec_pay_load_normal()
{
  set(types::sec_pay_load_normal);
  return c.get<sec_pay_load_normal_s>();
}
sec_pay_load_t_iot_s& sec_pay_load_c_::set_sec_pay_load_t_iot()
{
  set(types::sec_pay_load_t_iot);
  return c.get<sec_pay_load_t_iot_s>();
}

void sec_pay_load_c_::to_json(json_writer& j) const
{
  j.start_obj();
  switch (type_) {
    case types::sec_pay_load_normal:
      j.write_fieldname("sec_pay_load_normal");
      c.get<sec_pay_load_normal_s>().to_json(j);
      break;
    case types::sec_pay_load_t_iot:
      j.write_fieldname("sec_pay_load_t_iot");
      c.get<sec_pay_load_t_iot_s>().to_json(j);
      break;
    default:
      log_invalid_choice_id(type_, "sec_pay_load_c_");
  }
  j.end_obj();
}
SRSASN_CODE sec_pay_load_c_::pack(bit_ref& bref) const
{
  type_.pack(bref);
  switch (type_) {
    case types::sec_pay_load_normal:
      HANDLE_CODE(c.get<sec_pay_load_normal_s>().pack(bref));
      break;
    case types::sec_pay_load_t_iot:
      HANDLE_CODE(c.get<sec_pay_load_t_iot_s>().pack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "sec_pay_load_c_");
      return SRSASN_ERROR_ENCODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE sec_pay_load_c_::unpack(cbit_ref& bref)
{
  types e;
  e.unpack(bref);
  set(e);
  switch (type_) {
    case types::sec_pay_load_normal:
      HANDLE_CODE(c.get<sec_pay_load_normal_s>().unpack(bref));
      break;
    case types::sec_pay_load_t_iot:
      HANDLE_CODE(c.get<sec_pay_load_t_iot_s>().unpack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "sec_pay_load_c_");
      return SRSASN_ERROR_DECODE_FAIL;
  }
  return SRSASN_SUCCESS;
}

SRSASN_CODE secu_cfg_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(bref.pack(sec_pay_load_Present, 1));
  HANDLE_CODE(sec_alg_cfg.pack(bref));
  if (sec_pay_load_Present) {
    HANDLE_CODE(sec_pay_load.pack(bref));
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE secu_cfg_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(bref.unpack(sec_pay_load_Present, 1));
  HANDLE_CODE(sec_alg_cfg.unpack(bref));
  if (sec_pay_load_Present) {
    HANDLE_CODE(sec_pay_load.unpack(bref));
  }
  return SRSASN_SUCCESS;
}
void secu_cfg_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("sec_alg_cfg");
  sec_alg_cfg.to_json(j);
  if (sec_pay_load_Present) {
    j.write_fieldname(" sec_pay_load");
    sec_pay_load.to_json(j);
  }
  j.end_obj();
}

// release							NULL
SRSASN_CODE relea_s::pack(bit_ref& bref) const
{
  return SRSASN_SUCCESS;
}
SRSASN_CODE relea_s::unpack(cbit_ref& bref)
{
  return SRSASN_SUCCESS;
}
void relea_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.end_obj();
}

// setup							PHR-Config,
SRSASN_CODE set_up_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(peri_phr_timer.pack(bref));
  HANDLE_CODE(prohi_phr_time.pack(bref));
  HANDLE_CODE(dl_path_loss_change.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE set_up_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(peri_phr_timer.unpack(bref));
  HANDLE_CODE(prohi_phr_time.unpack(bref));
  HANDLE_CODE(dl_path_loss_change.unpack(bref));
  return SRSASN_SUCCESS;
}
void set_up_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("peri_phr_timer", peri_phr_timer.to_string());
  j.write_str("prohi_phr_time", prohi_phr_time.to_string());
  j.write_str("dl_path_loss_change", dl_path_loss_change.to_string());
  j.end_obj();
}
// phr-Config						CHOICE{
void phr_cfg_c_::destroy_()
{
  switch (type_) {
    case types::relea:
      c.destroy<relea_s>();
      break;
    case types::set_up:
      c.destroy<set_up_s>();
      break;
    default:
      break;
  }
}
void phr_cfg_c_::set(types::options e)
{
  destroy_();
  type_ = e;
  switch (type_) {
    case types::relea:
      c.init<relea_s>();
      break;
    case types::set_up:
      c.init<set_up_s>();
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "phr_cfg_c_");
  }
}
phr_cfg_c_::phr_cfg_c_(const phr_cfg_c_& other)
{
  type_ = other.type();
  switch (type_) {
    case types::relea:
      c.init(other.c.get<relea_s>());
      break;
    case types::set_up:
      c.init(other.c.get<set_up_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "phr_cfg_c_");
  }
}
phr_cfg_c_& phr_cfg_c_::operator=(const phr_cfg_c_& other)
{
  if (this == &other) {
    return *this;
  }
  set(other.type());
  switch (type_) {
    case types::relea:
      c.set(other.c.get<relea_s>());
      break;
    case types::set_up:
      c.set(other.c.get<set_up_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "phr_cfg_c_");
  }

  return *this;
}

relea_s& phr_cfg_c_::set_relea()
{
  set(types::relea);
  return c.get<relea_s>();
}
set_up_s& phr_cfg_c_::set_set_up()
{
  set(types::set_up);
  return c.get<set_up_s>();
}

void phr_cfg_c_::to_json(json_writer& j) const
{
  j.start_obj();
  switch (type_) {
    case types::relea:
      j.write_fieldname("relea");
      c.get<relea_s>().to_json(j);
      break;
    case types::set_up:
      j.write_fieldname("set_up");
      c.get<set_up_s>().to_json(j);
      break;
    default:
      log_invalid_choice_id(type_, "phr_cfg_c_");
  }
  j.end_obj();
}
SRSASN_CODE phr_cfg_c_::pack(bit_ref& bref) const
{
  type_.pack(bref);
  switch (type_) {
    case types::relea:
      HANDLE_CODE(c.get<relea_s>().pack(bref));
      break;
    case types::set_up:
      HANDLE_CODE(c.get<set_up_s>().pack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "phr_cfg_c_");
      return SRSASN_ERROR_ENCODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE phr_cfg_c_::unpack(cbit_ref& bref)
{
  types e;
  e.unpack(bref);
  set(e);
  switch (type_) {
    case types::relea:
      HANDLE_CODE(c.get<relea_s>().unpack(bref));
      break;
    case types::set_up:
      HANDLE_CODE(c.get<set_up_s>().unpack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "phr_cfg_c_");
      return SRSASN_ERROR_DECODE_FAIL;
  }
  return SRSASN_SUCCESS;
}

////RadioResourceConfigDedicated::=		SEQUENCE
SRSASN_CODE redio_resour_cfg_dedi_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(srb_to_add_present, 1));
  HANDLE_CODE(bref.pack(drb_to_add_mod_list_present, 1));
  HANDLE_CODE(bref.pack(drb_to_rel_list_present, 1));
  HANDLE_CODE(bref.pack(peri_bsr_timer_present, 1));
  HANDLE_CODE(bref.pack(phy_chan_list_cfg_present, 1));
  HANDLE_CODE(bref.pack(secu_cfg_present, 1));
  HANDLE_CODE(bref.pack(phr_cfg_present, 1));
  if (srb_to_add_present) {
    HANDLE_CODE(srb_to_add.pack(bref));
  }
  if (drb_to_add_mod_list_present) {
    HANDLE_CODE(pack_dyn_seq_of(bref, drb_to_add_mod_list, 1, 4));
  }
  if (drb_to_rel_list_present) {
    HANDLE_CODE(pack_dyn_seq_of(bref, drb_to_rel_list, 1, 4));
  }
  if (peri_bsr_timer_present) {
    HANDLE_CODE(peri_bsr_timer.pack(bref));
  }
  if (phy_chan_list_cfg_present) {
    HANDLE_CODE(pack_dyn_seq_of(bref, phy_chan_list_cfg, 1, 16));
  }
  if (secu_cfg_present) {
    HANDLE_CODE(secu_cfg.pack(bref));
  }
  if (phr_cfg_present) {
    HANDLE_CODE(phr_cfg.pack(bref));
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE redio_resour_cfg_dedi_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(srb_to_add_present, 1));
  HANDLE_CODE(bref.unpack(drb_to_add_mod_list_present, 1));
  HANDLE_CODE(bref.unpack(drb_to_rel_list_present, 1));
  HANDLE_CODE(bref.unpack(peri_bsr_timer_present, 1));
  HANDLE_CODE(bref.unpack(phy_chan_list_cfg_present, 1));
  HANDLE_CODE(bref.unpack(secu_cfg_present, 1));
  HANDLE_CODE(bref.unpack(phr_cfg_present, 1));
  if (srb_to_add_present) {
    HANDLE_CODE(srb_to_add.unpack(bref));
  }
  if (drb_to_add_mod_list_present) {
    HANDLE_CODE(unpack_dyn_seq_of(drb_to_add_mod_list, bref, 1, 4));
  }
  if (drb_to_rel_list_present) {
    HANDLE_CODE(unpack_dyn_seq_of(drb_to_rel_list, bref, 1, 4));
  }
  if (peri_bsr_timer_present) {
    HANDLE_CODE(peri_bsr_timer.unpack(bref));
  }
  if (phy_chan_list_cfg_present) {
    HANDLE_CODE(unpack_dyn_seq_of(phy_chan_list_cfg, bref, 1, 16));
  }
  if (secu_cfg_present) {
    HANDLE_CODE(secu_cfg.unpack(bref));
  }
  if (phr_cfg_present) {
    HANDLE_CODE(phr_cfg.unpack(bref));
  }
  return SRSASN_SUCCESS;
}
void redio_resour_cfg_dedi_s::to_json(json_writer& j) const
{
  j.start_obj();
  if (srb_to_add_present) {
    j.write_fieldname("srb_to_add");
    srb_to_add.to_json(j);
  }
  j.start_array("drb_to_add_mod_list");
  if (drb_to_add_mod_list_present) {
    for (uint16_t i1 = 0; i1 < drb_to_add_mod_list.size(); i1++) {
      drb_to_add_mod_list[i1].to_json(j);
    }
  }
  j.end_array();
  j.start_array(" drb_to_rel_list");
  if(drb_to_rel_list_present){
   for (uint16_t i1 = 0; i1 < drb_to_rel_list.size(); i1++) {
    drb_to_rel_list[i1].to_json(j);
  }
  }
  j.end_array();

  if (peri_bsr_timer_present) {
    j.write_str("peri_bsr_timer", peri_bsr_timer.to_string());
  }
  j.start_array("phy_chan_list_cfg");
  if (phy_chan_list_cfg_present) {
    for (uint16_t i1 = 0; i1 < phy_chan_list_cfg.size(); i1++) {
      phy_chan_list_cfg[i1].to_json(j);
    }
  }
  j.end_array();
  if (secu_cfg_present) {
    j.write_fieldname("secu_cfg");
    secu_cfg.to_json(j);
  }
  if (phr_cfg_present) {
    j.write_fieldname("phr_cfg");
    phr_cfg.to_json(j);
  }
  j.end_obj();
}

////SecurityConfigHO::= SEQUENCE{
SRSASN_CODE security_cofg_ho_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(sec_alg_cfg_present, 1));
  if (sec_alg_cfg_present)
  {
    HANDLE_CODE(sec_alg_cfg.pack(bref));
  }
  
  return SRSASN_SUCCESS;
}
SRSASN_CODE security_cofg_ho_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(sec_alg_cfg_present, 1));
  if (sec_alg_cfg_present)
  {
    HANDLE_CODE(sec_alg_cfg.unpack(bref));
  }
  return SRSASN_SUCCESS;
}
void security_cofg_ho_s::to_json(json_writer& j) const
{
  j.start_obj();
  if(sec_alg_cfg_present)
  {
    j.write_fieldname("sec_alg_cfg");
    sec_alg_cfg.to_json(j);
  }
  j.end_obj();
}
// DLInformation transfer ::=            SEQUENCE
SRSASN_CODE dl_info_tran_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(dl_info_tran_r1.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE dl_info_tran_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(dl_info_tran_r1.unpack(bref));
  return SRSASN_SUCCESS;
}
void dl_info_tran_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("DLInformation transfer");
  dl_info_tran_r1.to_json(j);
  j.end_obj();
}

////RRCCoonnectionReconfiguration
SRSASN_CODE rrc_con_recfg_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(pack_integer(bref, rrc_tran_iden, (uint8_t)0u, (uint8_t)3u));
  HANDLE_CODE(rrc_con_recfg_r1.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE rrc_con_recfg_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(unpack_integer(rrc_tran_iden, bref, (uint8_t)0u, (uint8_t)3u));
  HANDLE_CODE(rrc_con_recfg_r1.unpack(bref));
  return SRSASN_SUCCESS;
}
void rrc_con_recfg_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_int("rrc_tran_iden", rrc_tran_iden);
  j.write_fieldname(" rrc_con_recfg_r1");
  rrc_con_recfg_r1.to_json(j);
  j.end_obj();
}

/// RedirectionInfo ::=                    SEQUENCE
SRSASN_CODE redir_info_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(pack_integer(bref, beam_index, (uint8_t)1u, (uint8_t)15u));
  return SRSASN_SUCCESS;
}
SRSASN_CODE redir_info_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(unpack_integer(beam_index, bref, (uint8_t)1u, (uint8_t)15u));
  return SRSASN_SUCCESS;
}
void redir_info_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_int("beam_index", beam_index);
  j.end_obj();
}
// RRCConnectionRelease-r1-IEs ::=          SEQUENCE{
SRSASN_CODE rrc_con_rel_r1_ies_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(redir_info_present, 1));
  HANDLE_CODE(relea_cause.pack(bref));
  if (redir_info_present) {
    HANDLE_CODE(redir_info.pack(bref));
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE rrc_con_rel_r1_ies_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(redir_info_present, 1));
  HANDLE_CODE(relea_cause.unpack(bref));
  if (redir_info_present) {
    HANDLE_CODE(redir_info.unpack(bref));
  }
  return SRSASN_SUCCESS;
}
void rrc_con_rel_r1_ies_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("relea_cause", relea_cause.to_string());
  if (redir_info_present) {
    j.write_fieldname("redir_info");
    redir_info.to_json(j);
  }
  j.end_obj();
}

////RRCConnectionRelease ::=               SEQUENCE{
SRSASN_CODE rrc_con_release_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(rrc_con_rel_r1_ies.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE rrc_con_release_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(rrc_con_rel_r1_ies.unpack(bref));
  return SRSASN_SUCCESS;
}
void rrc_con_release_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("rrc_con_rel_r1_ies");
  rrc_con_rel_r1_ies.to_json(j);
  j.end_obj();
}

// rrc_tran_id
SRSASN_CODE rrc_tran_id_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(pack_integer(bref, rrc_tran_id_t, (uint8_t)0u, (uint8_t)3u));
  return SRSASN_SUCCESS;
}
SRSASN_CODE rrc_tran_id_s::unpack(cbit_ref& bref)
{
  ;
  HANDLE_CODE(unpack_integer(rrc_tran_id_t, bref, (uint8_t)0u, (uint8_t)3u));
  return SRSASN_SUCCESS;
}
void rrc_tran_id_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_int("rrc_tran_id_t", rrc_tran_id_t);
  j.end_obj();
}
// sec_mode_com_r1_ies_s
SRSASN_CODE sec_mode_com_r1_ies_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(sec_alg_cfg.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE sec_mode_com_r1_ies_s::unpack(cbit_ref& bref)
{
  ;
  bref.unpack(ext, 1);
  HANDLE_CODE(sec_alg_cfg.unpack(bref));
  return SRSASN_SUCCESS;
}
void sec_mode_com_r1_ies_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("sec_alg_cfg");
  sec_alg_cfg.to_json(j);
  j.end_obj();
}
// //SecurityModeConmmand::= SEQUENC
SRSASN_CODE sec_mode_com_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(rrc_tran_id.pack(bref));
  HANDLE_CODE(sec_mode_com_r1_ies.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE sec_mode_com_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(rrc_tran_id.unpack(bref));
  HANDLE_CODE(sec_mode_com_r1_ies.unpack(bref));
  return SRSASN_SUCCESS;
}
void sec_mode_com_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("rrc_tran_id");
  rrc_tran_id.to_json(j);
  j.write_fieldname("sec_mode_com_r1_ies");
  sec_mode_com_r1_ies.to_json(j);
  j.end_obj();
}

// DLInformationTransfer-r1-IEs ::=   SEQUENCE
SRSASN_CODE dl_info_tran_r1_ies_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(dedi_info_type.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE dl_info_tran_r1_ies_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(dedi_info_type.unpack(bref));
  return SRSASN_SUCCESS;
}
void dl_info_tran_r1_ies_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("DLInformationTransfer-r1-IEs");
  dedi_info_type.to_json(j);
  j.end_obj();
}

// ue_info_req_r1_ies_s
SRSASN_CODE ue_info_req_r1_ies_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(rach_repo_req, 1));
  HANDLE_CODE(bref.pack(rlf_repo_req, 1));
  return SRSASN_SUCCESS;
}
SRSASN_CODE ue_info_req_r1_ies_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(rach_repo_req, 1));
  HANDLE_CODE(bref.unpack(rlf_repo_req, 1));
  return SRSASN_SUCCESS;
}
void ue_info_req_r1_ies_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_bool("rach_repo_req", rach_repo_req);
  j.write_bool("rlf_repo_req", rlf_repo_req);
  j.end_obj();
}
////UEInfomationRequest::= SEQUENCE{
SRSASN_CODE ue_info_req_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(rrc_tran_id.pack(bref));
  HANDLE_CODE(ue_info_req_r1_ies.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE ue_info_req_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(rrc_tran_id.unpack(bref));
  HANDLE_CODE(ue_info_req_r1_ies.unpack(bref));
  return SRSASN_SUCCESS;
}
void ue_info_req_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("rrc_tran_id");
  rrc_tran_id.to_json(j);
  j.write_fieldname("ue_info_req_r1_ies");
  ue_info_req_r1_ies.to_json(j);

  j.end_obj();
}

// //// DL-DCCH-MessageType ::= CHOICE
void s_dl_dcch_msg_type_c_::destroy_()
{
  switch (type_) {
    case types::dl_info_tran:
      c.destroy<dl_info_tran_s>();
      break;
    case types::rrc_con_recfg:
      c.destroy<rrc_con_recfg_s>();
      break;
    case types::rrc_con_release:
      c.destroy<rrc_con_release_s>();
      break;
    case types::sec_mode_com:
      c.destroy<sec_mode_com_s>();
      break;
    case types::ue_info_req:
      c.destroy<ue_info_req_s>();
      break;
    default:
      break;
  }
}
void s_dl_dcch_msg_type_c_::set(types::options e)
{
  destroy_();
  type_ = e;
  switch (type_) {
    case types::dl_info_tran:
      c.init<dl_info_tran_s>();
      break;
    case types::rrc_con_recfg:
      c.init<rrc_con_recfg_s>();
      break;
    case types::rrc_con_release:
      c.init<rrc_con_release_s>();
      break;
    case types::sec_mode_com:
      c.init<sec_mode_com_s>();
      break;
    case types::ue_info_req:
      c.init<ue_info_req_s>();
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "s_dl_dcch_msg_type_c_");
  }
}
s_dl_dcch_msg_type_c_::s_dl_dcch_msg_type_c_(const s_dl_dcch_msg_type_c_& other)
{
  type_ = other.type();
  switch (type_) {
    case types::dl_info_tran:
      c.init(other.c.get<dl_info_tran_s>());
      break;
    case types::rrc_con_recfg:
      c.init(other.c.get<rrc_con_recfg_s>());
      break;
    case types::rrc_con_release:
      c.init(other.c.get<rrc_con_release_s>());
      break;
    case types::sec_mode_com:
      c.init(other.c.get<sec_mode_com_s>());
      break;
    case types::ue_info_req:
      c.init(other.c.get<ue_info_req_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "s_dl_dcch_msg_type_c_");
  }
}
s_dl_dcch_msg_type_c_& s_dl_dcch_msg_type_c_::operator=(const s_dl_dcch_msg_type_c_& other)
{
  if (this == &other) {
    return *this;
  }
  set(other.type());
  switch (type_) {
    case types::dl_info_tran:
      c.set(other.c.get<dl_info_tran_s>());
      break;
    case types::rrc_con_recfg:
      c.set(other.c.get<rrc_con_recfg_s>());
      break;
    case types::rrc_con_release:
      c.set(other.c.get<rrc_con_release_s>());
      break;
    case types::sec_mode_com:
      c.set(other.c.get<sec_mode_com_s>());
      break;
    case types::ue_info_req:
      c.set(other.c.get<ue_info_req_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "s_dl_dcch_msg_type_c_");
  }
  return *this;
}
dl_info_tran_s& s_dl_dcch_msg_type_c_::set_dl_info_tran()
{
  set(types::dl_info_tran);
  return c.get<dl_info_tran_s>();
}
rrc_con_recfg_s& s_dl_dcch_msg_type_c_::set_rrc_con_recfg()
{
  set(types::rrc_con_recfg);
  return c.get<rrc_con_recfg_s>();
}
rrc_con_release_s& s_dl_dcch_msg_type_c_::set_rrc_con_release()
{
  set(types::rrc_con_release);
  return c.get<rrc_con_release_s>();
}
sec_mode_com_s& s_dl_dcch_msg_type_c_::set_sec_mode_com()
{
  set(types::sec_mode_com);
  return c.get<sec_mode_com_s>();
}
ue_info_req_s& s_dl_dcch_msg_type_c_::set_ue_info_req()
{
  set(types::ue_info_req);
  return c.get<ue_info_req_s>();
}

void s_dl_dcch_msg_type_c_::to_json(json_writer& j) const
{
  j.start_obj();
  switch (type_) {
    case types::dl_info_tran:
      j.write_fieldname("dl_info_tran");
      c.get<dl_info_tran_s>().to_json(j);
      break;
    case types::rrc_con_recfg:
      j.write_fieldname("rrc_con_recfg");
      c.get<rrc_con_recfg_s>().to_json(j);
      break;
    case types::rrc_con_release:
      j.write_fieldname("rrc_con_release");
      c.get<rrc_con_release_s>().to_json(j);
      break;
    case types::sec_mode_com:
      j.write_fieldname("sec_mode_com");
      c.get<sec_mode_com_s>().to_json(j);
      break;
    case types::ue_info_req:
      j.write_fieldname("ue_info_req");
      c.get<ue_info_req_s>().to_json(j);
      break;
    default:
      log_invalid_choice_id(type_, "s_dl_dcch_msg_type_c");
  }
  j.end_obj();
}
SRSASN_CODE s_dl_dcch_msg_type_c_::pack(bit_ref& bref) const
{
  type_.pack(bref);
  switch (type_) {
    case types::dl_info_tran:
      HANDLE_CODE(c.get<dl_info_tran_s>().pack(bref));
      break;
    case types::rrc_con_recfg:
      HANDLE_CODE(c.get<rrc_con_recfg_s>().pack(bref));
      break;
    case types::rrc_con_release:
      HANDLE_CODE(c.get<rrc_con_release_s>().pack(bref));
      break;
    case types::sec_mode_com:
      HANDLE_CODE(c.get<sec_mode_com_s>().pack(bref));
      break;
    case types::ue_info_req:
      HANDLE_CODE(c.get<ue_info_req_s>().pack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "s_dl_dcch_msg_type_c");
      return SRSASN_ERROR_ENCODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE s_dl_dcch_msg_type_c_::unpack(cbit_ref& bref)
{
  type_.unpack(bref);
  switch (type_) {
    case types::dl_info_tran:
      HANDLE_CODE(c.get<dl_info_tran_s>().unpack(bref));
      break;
    case types::rrc_con_recfg:
      HANDLE_CODE(c.get<rrc_con_recfg_s>().unpack(bref));
      break;
    case types::rrc_con_release:
      HANDLE_CODE(c.get<rrc_con_release_s>().unpack(bref));
      break;
    case types::sec_mode_com:
      HANDLE_CODE(c.get<sec_mode_com_s>().unpack(bref));
      break;
    case types::ue_info_req:
      HANDLE_CODE(c.get<ue_info_req_s>().unpack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "s_dl_dcch_msg_type_c");
      return SRSASN_ERROR_ENCODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
const char* s_dl_dcch_msg_type_c_::types_opts::to_string() const
{
  static const char* options[] = {
      "ue_info_req_s", "sec_mode_com_s", "rrc_con_release_s", "rrc_con_recfg_s", "dl_info_tran_s"};
  return convert_enum_idx(options, 5, value, "s_dl_dcch_msg_type_c_::types");
}

// DL-dCCH-Message ::= SEQUENCE
SRSASN_CODE s_dl_dcch_msg_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(msg.pack(bref));

  bref.align_bytes_zero();

  return SRSASN_SUCCESS;
}
SRSASN_CODE s_dl_dcch_msg_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(msg.unpack(bref));

  bref.align_bytes();

  return SRSASN_SUCCESS;
}
void s_dl_dcch_msg_s::to_json(json_writer& j) const
{
  j.start_array();
  j.start_obj();
  j.start_obj("DL-DCCH-Message");
  j.write_fieldname("message");
  msg.to_json(j);
  j.end_obj();
  j.end_obj();
  j.end_array();
}
