/*******************************************************************************
 *
 *                     DL-CCCH Ghannel Information
 *
 ******************************************************************************/

#include "srsran/asn1/rrc/s_dl_ccch_msg.h"
#include "srsran/asn1/rrc/mib_sib_asn1.h"
#include <sstream>

using namespace asn1;
using namespace asn1::rrc;

/*******************************************************************************
 *                                Struct Methods
 ******************************************************************************/
// T-PollRetransmit ::=ENUMERATED
const char* t_poll_retran_opts::to_string() const
{
  static const char* options[] = {"ms480", "ms1200", "ms2100", "spare1"};
  return convert_enum_idx(options, 4, value, "t_poll_retran_e");
}
uint16_t t_poll_retran_opts::to_number() const
{
  static const uint16_t options[] = {480, 1200, 2100};
  return map_enum_number(options, 3, value, "t_poll_retran_e");
}

// PollPDU ::=ENUMERATED
const char* poll_pdu_s_opts::to_string() const
{
  static const char* options[] = {"p8", "p16", "p32", "pinfimity"};
  return convert_enum_idx(options, 4, value, "poll_pdu_e");
}
int8_t poll_pdu_s_opts::to_number() const
{
  static const int8_t options[] = {8, 16, 32, -1};
  return map_enum_number(options, 4, value, "poll_pdu_e");
}

// PollPDU ::=ENUMERATED
const char* poll_byte_s_opts::to_string() const
{
  static const char* options[] = {"kB16", "kB128", "kB256", "kBinfimity"};
  return convert_enum_idx(options, 4, value, "poll_byte_e");
}
int16_t poll_byte_s_opts::to_number() const
{
  static const int16_t options[] = {16, 128, 256, -1};
  return map_enum_number(options, 4, value, "poll_byte_e");
}

// T-Reordering ::=ENUMERATED
const char* t_reorder_opts::to_string() const
{
  static const char* options[] = {"ms480", "ms1200", "ms2100", "spare1"};
  return convert_enum_idx(options, 4, value, "t_reorder_e");
}
uint16_t t_reorder_opts::to_number() const
{
  static const uint16_t options[] = {480, 1200, 2100};
  return map_enum_number(options, 3, value, "t_reorder_e");
}

// T-StatusProhibit ::=ENUMERATED
const char* t_status_pro_opts::to_string() const
{
  static const char* options[] = {"ms0", "ms420", "ms600", "spare1"};
  return convert_enum_idx(options, 4, value, "t_status_prohibit_e");
}
uint16_t t_status_pro_opts::to_number() const
{
  static const uint16_t options[] = {0, 420, 600};
  return map_enum_number(options, 3, value, "t_status_prohibit_e");
}

// UL-AM-RLC ::=SEQUENCE
SRSASN_CODE ul_am_rlc_s_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(t_poll_retran.pack(bref));
  HANDLE_CODE(poll_pdu.pack(bref));
  HANDLE_CODE(poll_byte.pack(bref));
  HANDLE_CODE(max_retx_th.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE ul_am_rlc_s_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(t_poll_retran.unpack(bref));
  HANDLE_CODE(poll_pdu.unpack(bref));
  HANDLE_CODE(poll_byte.unpack(bref));
  HANDLE_CODE(max_retx_th.unpack(bref));
  return SRSASN_SUCCESS;
}
void ul_am_rlc_s_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("T-PollRetransmit", t_poll_retran.to_string());
  j.write_str("PollPDU", poll_pdu.to_string());
  j.write_str("PollBYTE", poll_byte.to_string());
  j.write_str("maxRetxThreshold", max_retx_th.to_string());
  j.end_obj();
}
const char* ul_am_rlc_s_s::max_retx_th_opts::to_string() const
{
  static const char* options[] = {"t1", "t2", "t4", "t8"};
  return convert_enum_idx(options, 4, value, "ul_am_rlc_s::max_retx_th_e_");
}
uint8_t ul_am_rlc_s_s::max_retx_th_opts::to_number() const
{
  static const uint8_t options[] = {1, 2, 4, 8};
  return map_enum_number(options, 4, value, "ul_am_rlc_s::max_retx_th_e_");
}

// DL-AM-RLC ::=SEQUENCE
SRSASN_CODE dl_am_rlc_s_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(t_reorder.pack(bref));
  HANDLE_CODE(t_statue_prohibit.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE dl_am_rlc_s_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(t_reorder.unpack(bref));
  HANDLE_CODE(t_statue_prohibit.unpack(bref));
  return SRSASN_SUCCESS;
}
void dl_am_rlc_s_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("T-Reordering", t_reorder.to_string());
  j.write_str("T-StatusProhibit", t_statue_prohibit.to_string());
  j.end_obj();
}

// UL-UM-RLC ::=SEQUENCE
SRSASN_CODE ul_um_rlc_s_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  return SRSASN_SUCCESS;
}
SRSASN_CODE ul_um_rlc_s_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  return SRSASN_SUCCESS;
}
void ul_um_rlc_s_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.end_obj();
}

// DL-UM-RLC ::=SEQUENCE
SRSASN_CODE dl_um_rlc_s_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(t_reorder.pack(bref));
}
SRSASN_CODE dl_um_rlc_s_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(t_reorder.unpack(bref));
}
void dl_um_rlc_s_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("T-Reordering", t_reorder.to_string());
  j.end_obj();
}

// SDAP-Config ::=SEQUENCE
SRSASN_CODE sdap_cfg_s_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(mapped_qos_flows_to_add_present, 1));
  HANDLE_CODE(bref.pack(mapped_qos_flows_to_release_present, 1));

  HANDLE_CODE(pack_integer(bref, pdu_session, (uint16_t)0u, (uint16_t)255u));
  HANDLE_CODE(sdap_hdr_dl.pack(bref));
  HANDLE_CODE(sdap_hdr_ul.pack(bref));
  HANDLE_CODE(bref.pack(default_drb, 1));
  if (mapped_qos_flows_to_add_present) {
    HANDLE_CODE(pack_dyn_seq_of(bref, mapped_qos_flows_to_add, 1, 4, integer_packer<uint8_t>(0, 63)));
  }
  if (mapped_qos_flows_to_release_present) {
    HANDLE_CODE(pack_dyn_seq_of(bref, mapped_qos_flows_to_release, 1, 4, integer_packer<uint8_t>(0, 63)));
  }

  return SRSASN_SUCCESS;
}
SRSASN_CODE sdap_cfg_s_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(mapped_qos_flows_to_add_present, 1));
  HANDLE_CODE(bref.unpack(mapped_qos_flows_to_release_present, 1));

  HANDLE_CODE(unpack_integer(pdu_session, bref, (uint16_t)0u, (uint16_t)255u));
  HANDLE_CODE(sdap_hdr_dl.unpack(bref));
  HANDLE_CODE(sdap_hdr_ul.unpack(bref));
  HANDLE_CODE(bref.unpack(default_drb, 1));
  if (mapped_qos_flows_to_add_present) {
    HANDLE_CODE(unpack_dyn_seq_of(mapped_qos_flows_to_add, bref, 1, 4, integer_packer<uint8_t>(0, 63)));
  }
  if (mapped_qos_flows_to_release_present) {
    HANDLE_CODE(unpack_dyn_seq_of(mapped_qos_flows_to_release, bref, 1, 4, integer_packer<uint8_t>(0, 63)));
  }

  return SRSASN_SUCCESS;
}
void sdap_cfg_s_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_int("pdu-Session", pdu_session);
  j.write_str("sdap-HeaderDL", sdap_hdr_dl.to_string());
  j.write_str("sdap-HeaderUL", sdap_hdr_ul.to_string());
  j.write_bool("defaultDRB", default_drb);
  if (mapped_qos_flows_to_add_present) {
    j.start_array("mappedQoS-FlowsToAdd");
    for (const auto& e1 : mapped_qos_flows_to_add) {
      j.write_int(e1);
    }
    j.end_array();
  }
  if (mapped_qos_flows_to_release_present) {
    j.start_array("mappedQoS-FlowsToRelease");
    for (const auto& e1 : mapped_qos_flows_to_release) {
      j.write_int(e1);
    }
    j.end_array();
  }
  j.end_obj();
}

const char* sdap_cfg_s_s::sdap_hdr_dl_opts::to_string() const
{
  static const char* options[] = {"present", "absent"};
  return convert_enum_idx(options, 2, value, "sdap_cfg_s_s::sdap_hdr_dl_e_");
}

const char* sdap_cfg_s_s::sdap_hdr_ul_opts::to_string() const
{
  static const char* options[] = {"present", "absent"};
  return convert_enum_idx(options, 2, value, "sdap_cfg_s_s::sdap_hdr_ul_e_");
}

// PDCP ::=SEQUENCE
SRSASN_CODE pdcp_cfg_s_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(discard_timer_present, 1));
  HANDLE_CODE(bref.pack(rlc_am_present, 1));
  HANDLE_CODE(bref.pack(rlc_um_present, 1));
  HANDLE_CODE(bref.pack(rlc_tm_present, 1));
  HANDLE_CODE(bref.pack(intgrity_protection_present, 1));
  HANDLE_CODE(bref.pack(ciphering_disabled_present, 1));

  if (discard_timer_present) {
    HANDLE_CODE(discard_timer.pack(bref));
  }
  if (rlc_am_present) {
    HANDLE_CODE(bref.pack(rlc_am.statue_rep_req, 1));
  }
  if (rlc_um_present) {
    bref.pack(ext, 1);
  }
  if (rlc_tm_present) {
    bref.pack(ext, 1);
  }
  HANDLE_CODE(header_comper.pack(bref));

  return SRSASN_SUCCESS;
}
SRSASN_CODE pdcp_cfg_s_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(discard_timer_present, 1));
  HANDLE_CODE(bref.unpack(rlc_am_present, 1));
  HANDLE_CODE(bref.unpack(rlc_um_present, 1));
  HANDLE_CODE(bref.unpack(rlc_tm_present, 1));
  HANDLE_CODE(bref.unpack(intgrity_protection_present, 1));
  HANDLE_CODE(bref.unpack(ciphering_disabled_present, 1));

  if (discard_timer_present) {
    HANDLE_CODE(discard_timer.unpack(bref));
  }
  if (rlc_am_present) {
    HANDLE_CODE(bref.unpack(rlc_am.statue_rep_req, 1));
  }
  if (rlc_um_present) {
    bref.unpack(ext, 1);
  }
  if (rlc_tm_present) {
    bref.unpack(ext, 1);
  }
  HANDLE_CODE(header_comper.unpack(bref));

  return SRSASN_SUCCESS;
}
void pdcp_cfg_s_s::to_json(json_writer& j) const
{
  j.start_obj();
  if (discard_timer_present) {
    j.write_str("discardTimer", discard_timer.to_string());
  }
  if (rlc_am_present) {
    j.write_bool("statusReportRequired", rlc_am.statue_rep_req);
  }

  j.write_fieldname("headerCompression");
  header_comper.to_json(j);

  if (intgrity_protection_present) {
    j.write_str("integrityProtection", "enabled");
  }
  if (ciphering_disabled_present) {
    j.write_str("cipheringDisabled", "true");
  }
  j.end_obj();
}

void pdcp_cfg_s_s::header_compre_c_::set(types::options e)
{
  type_ = e;
}

void pdcp_cfg_s_s::header_compre_c_::set_not_used()
{
  set(types::not_used);
}
pdcp_cfg_s_s::header_compre_c_::rohc_s_& pdcp_cfg_s_s::header_compre_c_::set_rohc()
{
  set(types::rohc);
  return c;
}
void pdcp_cfg_s_s::header_compre_c_::to_json(json_writer& j) const
{
  j.start_obj();
  switch (type_) {
    case types::not_used:
      break;
    case types::rohc:
      j.write_fieldname("rohc");
      j.start_obj();

      j.write_int("maxCID", c.max_cid);

      j.write_fieldname("profiles");
      j.start_obj();
      j.write_bool("profile_0X0002", c.profiles.profile_0x0002);
      j.write_bool("profile_0X0004", c.profiles.profile_0x0004);
      j.write_bool("profile_0X0006", c.profiles.profile_0x0006);
      j.write_bool("profile_0X0102", c.profiles.profile_0x0102);
      j.write_bool("profile_0X0104", c.profiles.profile_0x0104);
      j.end_obj();
      j.end_obj();
      break;
    default:
      log_invalid_choice_id(type_, "pdcp_cfg_s::drb_s_::hdr_compress_c_");
  }
  j.end_obj();
}
SRSASN_CODE pdcp_cfg_s_s::header_compre_c_::pack(bit_ref& bref) const
{
  type_.pack(bref);
  switch (type_) {
    case types::not_used:
      break;
    case types::rohc:
      bref.pack(c.ext, 1);
      HANDLE_CODE(pack_integer(bref, c.max_cid, (uint16_t)1u, (uint16_t)16383u));
      HANDLE_CODE(bref.pack(c.profiles.profile_0x0002, 1));
      HANDLE_CODE(bref.pack(c.profiles.profile_0x0004, 1));
      HANDLE_CODE(bref.pack(c.profiles.profile_0x0006, 1));
      HANDLE_CODE(bref.pack(c.profiles.profile_0x0102, 1));
      HANDLE_CODE(bref.pack(c.profiles.profile_0x0104, 1));
      break;
    default:
      log_invalid_choice_id(type_, "pdcp_cfg_s_s::header_compre_c_");
      return SRSASN_ERROR_ENCODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE pdcp_cfg_s_s::header_compre_c_::unpack(cbit_ref& bref)
{
  types e;
  e.unpack(bref);
  set(e);
  switch (type_) {
    case types::not_used:
      break;
    case types::rohc:
      bref.unpack(c.ext, 1);
      HANDLE_CODE(unpack_integer(c.max_cid, bref, (uint16_t)1u, (uint16_t)16383u));
      HANDLE_CODE(bref.unpack(c.profiles.profile_0x0002, 1));
      HANDLE_CODE(bref.unpack(c.profiles.profile_0x0004, 1));
      HANDLE_CODE(bref.unpack(c.profiles.profile_0x0006, 1));
      HANDLE_CODE(bref.unpack(c.profiles.profile_0x0102, 1));
      HANDLE_CODE(bref.unpack(c.profiles.profile_0x0104, 1));
    default:
      log_invalid_choice_id(type_, "pdcp_cfg_s_s::header_compre_c_");
      return SRSASN_ERROR_DECODE_FAIL;
  }
  return SRSASN_SUCCESS;
}

const char* pdcp_cfg_s_s::header_compre_c_::types_opts::to_string() const
{
  static const char* options[] = {"notUsed", "rohc"};
  return convert_enum_idx(options, 2, value, "pdcp_cfg_s_s::header_compre_c_::types");
}

const char* pdcp_cfg_s_s::discard_timer_opts::to_string() const
{
  static const char* options[] = {"ms900", "ms1200", "ms1500", "ms3000", "ms5100", "spare2", "spare1", "infinity"};
  return convert_enum_idx(options, 8, value, "pdcp_cfg_s_s::discard_timer_e_");
}

// RLC-Config ::=CHOICE
void rlc_cfg_s_c::destroy_()
{
  switch (type_) {
    case types::am:
      c.destroy<am_s_>();
      break;
    case types::um_bi_dir:
      c.destroy<um_bi_dirrctional_s_>();
      break;
    default:
      break;
  }
}
void rlc_cfg_s_c::set(types::options e)
{
  destroy_();
  type_ = e;
  switch (type_) {
    case types::am:
      c.init<am_s_>();
      break;
    case types::um_bi_dir:
      c.init<um_bi_dirrctional_s_>();
      break;
    case types::tm:
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "rlc_cfg_s_c");
  }
}
rlc_cfg_s_c::rlc_cfg_s_c(const rlc_cfg_s_c& other)
{
  type_ = other.type();
  switch (type_) {
    case types::am:
      c.init(other.c.get<am_s_>());
      break;
    case types::um_bi_dir:
      c.init(other.c.get<um_bi_dirrctional_s_>());
      break;
    case types::tm:
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "rlc_cfg_s_c");
  }
}
rlc_cfg_s_c& rlc_cfg_s_c::operator=(const rlc_cfg_s_c& other)
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
      c.set(other.c.get<um_bi_dirrctional_s_>());
      break;
    case types::tm:
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "rlc_cfg_s_c");
  }

  return *this;
}
rlc_cfg_s_c::am_s_& rlc_cfg_s_c::set_am()
{
  set(types::am);
  return c.get<am_s_>();
}
rlc_cfg_s_c::um_bi_dirrctional_s_& rlc_cfg_s_c::set_um_bi_dirrctional()
{
  set(types::um_bi_dir);
  return c.get<um_bi_dirrctional_s_>();
}
void rlc_cfg_s_c::set_tm()
{
  set(types::tm);
}
void rlc_cfg_s_c::to_json(json_writer& j) const
{
  j.start_obj();
  switch (type_) {
    case types::am:
      j.write_fieldname("am");
      j.start_obj();
      j.write_fieldname("ul-AM-RLC");
      c.get<am_s_>().ul_am_rlc.to_json(j);
      j.write_fieldname("dl-AM-RLC");
      c.get<am_s_>().dl_am_rlc.to_json(j);
      j.end_obj();
      break;
    case types::um_bi_dir:
      j.write_fieldname("um-Bi-Directional");
      j.start_obj();
      j.write_fieldname("ul-UM-RLC");
      c.get<um_bi_dirrctional_s_>().ul_um_rlc.to_json(j);
      j.write_fieldname("dl-UM-RLC");
      c.get<um_bi_dirrctional_s_>().dl_um_rlc.to_json(j);
      j.end_obj();
      break;
    case types::tm:
      break;
    default:
      log_invalid_choice_id(type_, "rlc_cfg_s_c");
  }
  j.end_obj();
}
SRSASN_CODE rlc_cfg_s_c::pack(bit_ref& bref) const
{
  type_.pack(bref);
  switch (type_) {
    case types::am:
      HANDLE_CODE(c.get<am_s_>().ul_am_rlc.pack(bref));
      HANDLE_CODE(c.get<am_s_>().dl_am_rlc.pack(bref));
      break;
    case types::um_bi_dir:
      HANDLE_CODE(c.get<um_bi_dirrctional_s_>().ul_um_rlc.pack(bref));
      HANDLE_CODE(c.get<um_bi_dirrctional_s_>().dl_um_rlc.pack(bref));
      break;
    case types::tm:
      break;
    default:
      log_invalid_choice_id(type_, "rlc_cfg_s_c");
      return SRSASN_ERROR_ENCODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE rlc_cfg_s_c::unpack(cbit_ref& bref)
{
  types e;
  e.unpack(bref);
  set(e);
  switch (type_) {
    case types::am:
      HANDLE_CODE(c.get<am_s_>().ul_am_rlc.unpack(bref));
      HANDLE_CODE(c.get<am_s_>().dl_am_rlc.unpack(bref));
      break;
    case types::um_bi_dir:
      HANDLE_CODE(c.get<um_bi_dirrctional_s_>().ul_um_rlc.unpack(bref));
      HANDLE_CODE(c.get<um_bi_dirrctional_s_>().dl_um_rlc.unpack(bref));
      break;
    case types::tm:
      break;
    default:
      log_invalid_choice_id(type_, "rlc_cfg_s_c");
      return SRSASN_ERROR_DECODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
const char* rlc_cfg_s_c::types_opts::to_string() const
{
  static const char* options[] = {"am", "um-Bi-Directional", "tm"};
  return convert_enum_idx(options, 3, value, "rlc_cfg_s_c::types");
}

// LogicalChannelConfig ::=SEQUENCE
SRSASN_CODE log_ch_cfg_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(ul_specific_parameter_present, 1));

  if (ul_specific_parameter_present) {
    HANDLE_CODE(bref.pack(ul_specific_parameter.log_ch_group_present, 1));
    HANDLE_CODE(pack_integer(bref, ul_specific_parameter.priority, (uint8_t)1u, (uint8_t)16u));
    HANDLE_CODE(ul_specific_parameter.priort_bit_rate.pack(bref));
    HANDLE_CODE(ul_specific_parameter.bucket_size_duration.pack(bref));
    if (ul_specific_parameter.log_ch_group_present) {
      HANDLE_CODE(pack_integer(bref, ul_specific_parameter.log_ch_group, (uint8_t)0u, (uint8_t)3u));
    }
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE log_ch_cfg_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(ul_specific_parameter_present, 1));

  if (ul_specific_parameter_present) {
    HANDLE_CODE(bref.unpack(ul_specific_parameter.log_ch_group_present, 1));
    HANDLE_CODE(unpack_integer(ul_specific_parameter.priority, bref, (uint8_t)1u, (uint8_t)16u));
    HANDLE_CODE(ul_specific_parameter.priort_bit_rate.unpack(bref));
    HANDLE_CODE(ul_specific_parameter.bucket_size_duration.unpack(bref));
    if (ul_specific_parameter.log_ch_group_present) {
      HANDLE_CODE(unpack_integer(ul_specific_parameter.log_ch_group, bref, (uint8_t)0u, (uint8_t)3u));
    }
  }
  return SRSASN_SUCCESS;
}
void log_ch_cfg_s::to_json(json_writer& j) const
{
  j.start_obj();
  if (ul_specific_parameter_present) {
    j.write_int("prioritized", ul_specific_parameter.priority);
    j.write_str("prioritizedBitRate", ul_specific_parameter.priort_bit_rate.to_string());
    j.write_str("bucketSizeDuration", ul_specific_parameter.bucket_size_duration.to_string());
    if (ul_specific_parameter.log_ch_group_present) {
      j.write_int("logicalChannelGroup", ul_specific_parameter.log_ch_group);
    }
  }
  j.end_obj();
}
const char* log_ch_cfg_s::ul_specific_parameter_s::priort_bit_rate_opts::to_string() const
{
  static const char* options[] = {"kBps0", "kBps16", "kBps2dot4", "kBps128", "infinity", "spare3", "spare2", "spare1"};
  return convert_enum_idx(options, 8, value, "log_ch_cfg_s::ul_specific_parameter_s::priort_bit_rate_e_");
}
int16_t log_ch_cfg_s::ul_specific_parameter_s::priort_bit_rate_opts::to_number() const
{
  static const int16_t options[] = {0, 16, 4, 128, -1};
  return map_enum_number(options, 5, value, "log_ch_cfg_s::ul_specific_parameter_s::priort_bit_rate_e_");
}
const char* log_ch_cfg_s::ul_specific_parameter_s::bucket_size_duration_opts::to_string() const
{
  static const char* options[] = {"ms60", "ms120", "ms180", "ms300", "ms600", "ms1200", "spare2", "spare1"};
  return convert_enum_idx(options, 8, value, "log_ch_cfg_s::ul_specific_parameter_s::bucket_size_duration_e_");
}
uint16_t log_ch_cfg_s::ul_specific_parameter_s::bucket_size_duration_opts::to_number() const
{
  static const uint16_t options[] = {60, 120, 180, 300, 600, 1200};
  return map_enum_number(options, 6, value, "log_ch_cfg_s::ul_specific_parameter_s::bucket_size_duration_e_");
}
// PDTCHCode ::=SEQUENCE
SRSASN_CODE pdtch_code_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(pack_integer(bref, pdtch_phy_code, (uint16_t)0u, (uint16_t)511u));
  return SRSASN_SUCCESS;
}
SRSASN_CODE pdtch_code_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(unpack_integer(pdtch_phy_code, bref, (uint16_t)0u, (uint16_t)511u));
  return SRSASN_SUCCESS;
}
void pdtch_code_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_int("PDTCHCode", pdtch_phy_code);
  j.end_obj();
}

// SRB-ToAdd ::= SEQUENCE
SRSASN_CODE srb_to_add_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  return SRSASN_SUCCESS;
}
SRSASN_CODE srb_to_add_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  return SRSASN_SUCCESS;
}
void srb_to_add_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.end_obj();
}

// DRB-Identity ::= SEQUENCE
SRSASN_CODE drb_id_s_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(pack_integer(bref, drb_id, (uint8_t)3u, (uint8_t)6u));
  return SRSASN_SUCCESS;
}
SRSASN_CODE drb_id_s_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(unpack_integer(drb_id, bref, (uint8_t)3u, (uint8_t)6u));
  return SRSASN_SUCCESS;
}
void drb_id_s_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_int("DRB-Identity", drb_id);
  j.end_obj();
}

// DRB-ToAddMod :: = SEQUENCE
SRSASN_CODE drb_to_add_mod_s_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(bref.pack(sdap_cfg_present, 1));
  HANDLE_CODE(bref.pack(pdcp_cfg_present, 1));
  HANDLE_CODE(bref.pack(rlc_cfg_present, 1));
  HANDLE_CODE(bref.pack(log_ch_cfg_present, 1));

  if (sdap_cfg_present) {
    HANDLE_CODE(sdap_cfg.pack(bref));
  }

  HANDLE_CODE(drb_id.pack(bref));

  if (pdcp_cfg_present) {
    HANDLE_CODE(pdcp_cfg.pack(bref));
  }
  if (rlc_cfg_present) {
    HANDLE_CODE(rlc_cfg.pack(bref));
  }
  if (log_ch_cfg_present) {
    HANDLE_CODE(log_ch_cfg.pack(bref));
  }

  return SRSASN_SUCCESS;
}
SRSASN_CODE drb_to_add_mod_s_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(bref.unpack(sdap_cfg_present, 1));
  HANDLE_CODE(bref.unpack(pdcp_cfg_present, 1));
  HANDLE_CODE(bref.unpack(rlc_cfg_present, 1));
  HANDLE_CODE(bref.unpack(log_ch_cfg_present, 1));

  if (sdap_cfg_present) {
    HANDLE_CODE(sdap_cfg.unpack(bref));
  }

  HANDLE_CODE(drb_id.unpack(bref));

  if (pdcp_cfg_present) {
    HANDLE_CODE(pdcp_cfg.unpack(bref));
  }
  if (rlc_cfg_present) {
    HANDLE_CODE(rlc_cfg.unpack(bref));
  }
  if (log_ch_cfg_present) {
    HANDLE_CODE(log_ch_cfg.unpack(bref));
  }

  return SRSASN_SUCCESS;
}
void drb_to_add_mod_s_s::to_json(json_writer& j) const
{
  j.start_obj();

  if (sdap_cfg_present) {
    j.write_fieldname("Sdap-Config");
    sdap_cfg.to_json(j);
  }

  j.write_fieldname("Drb-Identity");
  drb_id.to_json(j);

  if (pdcp_cfg_present) {
    j.write_fieldname("Pdcp-Config");
    pdcp_cfg.to_json(j);
  }
  if (rlc_cfg_present) {
    j.write_fieldname("Rlc-Config");
    rlc_cfg.to_json(j);
  }
  if (log_ch_cfg_present) {
    j.write_fieldname("Logicalcannelconfig");
    log_ch_cfg.to_json(j);
  }

  j.end_obj();
}

// PhysicalChannelList-Config ::= SEQUENCE
SRSASN_CODE phy_ch_cfg_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(band_id_present, 1));
  HANDLE_CODE(bref.pack(freq_id_present, 1));
  HANDLE_CODE(bref.pack(slot_ass_present, 1));
  HANDLE_CODE(bref.pack(pdtch_code_present, 1));
  HANDLE_CODE(bref.pack(shced_type_present, 1));
  HANDLE_CODE(bref.pack(voice_type_present, 1));

  HANDLE_CODE(s_rnti.pack(bref));
  HANDLE_CODE(ch_type.pack(bref));
  if (band_id_present) {
    HANDLE_CODE(band_id.pack(bref));
  }
  if (freq_id_present) {
    HANDLE_CODE(freq_id.pack(bref));
  }
  if (slot_ass_present) {
    HANDLE_CODE(slot_ass.pack(bref));
  }
  HANDLE_CODE(direction.pack(bref));
  if (pdtch_code_present) {
    HANDLE_CODE(pdtch_code.pack(bref));
  }
  if (shced_type_present) {
    HANDLE_CODE(shced_type.pack(bref));
  }
  if (voice_type_present) {
    HANDLE_CODE(voice_type.pack(bref));
  }

  return SRSASN_SUCCESS;
}
SRSASN_CODE phy_ch_cfg_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(band_id_present, 1));
  HANDLE_CODE(bref.unpack(freq_id_present, 1));
  HANDLE_CODE(bref.unpack(slot_ass_present, 1));
  HANDLE_CODE(bref.unpack(pdtch_code_present, 1));
  HANDLE_CODE(bref.unpack(shced_type_present, 1));
  HANDLE_CODE(bref.unpack(voice_type_present, 1));

  HANDLE_CODE(s_rnti.unpack(bref));
  HANDLE_CODE(ch_type.unpack(bref));

  if (band_id_present) {
    HANDLE_CODE(band_id.unpack(bref));
  }
  if (freq_id_present) {
    HANDLE_CODE(freq_id.unpack(bref));
  }
  if (slot_ass_present) {
    HANDLE_CODE(slot_ass.unpack(bref));
  }
  HANDLE_CODE(direction.unpack(bref));
  if (pdtch_code_present) {
    HANDLE_CODE(pdtch_code.unpack(bref));
  }
  if (shced_type_present) {
    HANDLE_CODE(shced_type.unpack(bref));
  }
  if (voice_type_present) {
    HANDLE_CODE(voice_type.unpack(bref));
  }

  return SRSASN_SUCCESS;
}
void phy_ch_cfg_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("S_RNTI", s_rnti.to_string());
  j.write_str("ChannelType", ch_type.to_string());

  if (band_id_present) {
    j.write_fieldname("BandIdentity");
    band_id.to_json(j);
  }
  if (freq_id_present) {
    j.write_fieldname("FrequencyIdentity");
    freq_id.to_json(j);
  }
  if (slot_ass_present) {
    j.write_str("SlotAssignment", slot_ass.to_string());
  }
  j.write_str("Direction", direction.to_string());

  if (pdtch_code_present) {
    j.write_fieldname("PDYCHCode");
    pdtch_code.to_json(j);
  }

  if (shced_type_present) {
    j.write_str("SchedulingType", shced_type.to_string());
  }
  if (voice_type_present) {
    j.write_str("VoiceType", voice_type.to_string());
  }

  j.end_obj();
}
const char* channel_type_opts::to_string() const
{
  static const char* options[] = {"psych",
                                  "pdch1_1",
                                  "pdch1_2",
                                  "psch1_1",
                                  "psch1_2",
                                  "psch5_1",
                                  "psch5_2",
                                  "ds_pdtch_1",
                                  "ds_pdtch_2",
                                  "ds_pdtch_3",
                                  "ds_pdtch_t"};
  return convert_enum_idx(options, 11, value, "phy_ch_cfg_s::direction_e_");
}
const char* phy_ch_cfg_s::direction_opts::to_string() const
{
  static const char* options[] = {"bidirection", "uldirection", "dldirection"};
  return convert_enum_idx(options, 3, value, "phy_ch_cfg_s::direction_e_");
}
const char* phy_ch_cfg_s::shced_type_opts::to_string() const
{
  static const char* options[] = {"static_t", "dynamic"};
  return convert_enum_idx(options, 2, value, "phy_ch_cfg_s::shced_type_e_");
}
const char* phy_ch_cfg_s::voice_type_opts::to_string() const
{
  static const char* options[] = {"kbps2point4", "kbps4point8","bps800"};
  return convert_enum_idx(options, 2, value, "phy_ch_cfg_s::voice_type_e_");
}

// securityPayload	    CHOICE
void security_payload::destroy_()
{
  switch (type_) {
    case types::security_payload_n_:
      c.destroy<security_payload_n_s>();
      break;
    case types::security_payload_t_:
      c.destroy<security_payload_t_s>();
      break;
    default:
      break;
  }
}
void security_payload::set(types::options e)
{
  destroy_();
  type_ = e;
  switch (type_) {
    case types::security_payload_n_:
      c.init<security_payload_n_s>();
      break;
    case types::security_payload_t_:
      c.init<security_payload_t_s>();
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "security_payload");
  }
}
security_payload::security_payload(const security_payload& other)
{
  type_ = other.type();
  switch (type_) {
    case types::security_payload_n_:
      c.init(other.c.get<security_payload_n_s>());
      break;
    case types::security_payload_t_:
      c.init(other.c.get<security_payload_t_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "security_payload");
  }
}
security_payload& security_payload::operator=(const security_payload& other)
{
  if (this == &other) {
    return *this;
  }
  set(other.type());
  switch (type_) {
    case types::security_payload_n_:
      c.set(other.c.get<security_payload_n_s>());
      break;
    case types::security_payload_t_:
      c.set(other.c.get<security_payload_t_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "security_payload");
  }

  return *this;
}

security_payload_n_s& security_payload::set_security_payload_n()
{
  set(types::security_payload_n_);
  return c.get<security_payload_n_s>();
}
security_payload_t_s& security_payload::set_security_payload_t()
{
  set(types::security_payload_t_);
  return c.get<security_payload_t_s>();
}

////securityPayload-Normal		OCTET STRING.
SRSASN_CODE security_payload_n_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(security_payload_n_s_.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE security_payload_n_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(security_payload_n_s_.unpack(bref));
  return SRSASN_SUCCESS;
}
void security_payload_n_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("sec_pay_load_nor", security_payload_n_s_.to_string());
  j.end_obj();
}
// securityPayload-TtoT			OCTET STRING.
SRSASN_CODE security_payload_t_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(security_payload_t_s_.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE security_payload_t_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(security_payload_t_s_.unpack(bref));
  return SRSASN_SUCCESS;
}
void security_payload_t_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("sec_pay_load_yiot", security_payload_t_s_.to_string());
  j.end_obj();
}

void security_payload::to_json(json_writer& j) const
{
  j.start_obj();
  switch (type_) {
    case types::security_payload_n_:
      j.write_fieldname("security_payload_n_");
      c.get<security_payload_n_s>().to_json(j);
      break;
    case types::security_payload_t_:
      j.write_fieldname("security_payload_t_");
      c.get<security_payload_t_s>().to_json(j);
      break;
    default:
      log_invalid_choice_id(type_, "security_payload");
  }
  j.end_obj();
}
SRSASN_CODE security_payload::pack(bit_ref& bref) const
{
  type_.pack(bref);
  switch (type_) {
    case types::security_payload_n_:
      HANDLE_CODE(c.get<security_payload_n_s>().pack(bref));
      break;
    case types::security_payload_t_:
      HANDLE_CODE(c.get<security_payload_t_s>().pack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "security_payload");
      return SRSASN_ERROR_ENCODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE security_payload::unpack(cbit_ref& bref)
{
  types e;
  e.unpack(bref);
  set(e);
  switch (type_) {
    case types::security_payload_n_:
      HANDLE_CODE(c.get<security_payload_n_s>().unpack(bref));
      break;
    case types::security_payload_t_:
      HANDLE_CODE(c.get<security_payload_t_s>().unpack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "security_payload");
      return SRSASN_ERROR_DECODE_FAIL;
  }
  return SRSASN_SUCCESS;
}

// SecurityAlgorithmConfig ::= SEQUENCE
SRSASN_CODE security_alg_cfg_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(int_prot_algorithm_present, 1));
  HANDLE_CODE(ci_phe_algorithm.pack(bref));
  if (int_prot_algorithm_present) {
    HANDLE_CODE(int_prot_algorithm.pack(bref));
  }

  return SRSASN_SUCCESS;
}
SRSASN_CODE security_alg_cfg_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(int_prot_algorithm_present, 1));
  HANDLE_CODE(ci_phe_algorithm.unpack(bref));
  if (int_prot_algorithm_present) {
    HANDLE_CODE(int_prot_algorithm.unpack(bref));
  }

  return SRSASN_SUCCESS;
}
void security_alg_cfg_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str(" CipheringAlgorithm", ci_phe_algorithm.to_string());

  if (int_prot_algorithm_present) {
    j.write_str("IntegrityProtAlgorithm", int_prot_algorithm.to_string());
  }

  j.end_obj();
}

SRSASN_CODE security_Config::pack(bit_ref& bref) const
{
  HANDLE_CODE(bref.pack(security_payload_present, 1));
  HANDLE_CODE(security_alg_cfg_.pack(bref));
  if (security_payload_present) {
    HANDLE_CODE(security_payload_.pack(bref));
  }
  return SRSASN_SUCCESS;
}

SRSASN_CODE security_Config::unpack(cbit_ref& bref)
{
  HANDLE_CODE(bref.unpack(security_payload_present, 1));
  HANDLE_CODE(security_alg_cfg_.unpack(bref));
  if (security_payload_present) {
    HANDLE_CODE(security_payload_.unpack(bref));
  }
  return SRSASN_SUCCESS;
}

void security_Config::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("security_alg_cfg");
  security_alg_cfg_.to_json(j);
  if (security_payload_present) {
    j.write_fieldname("security_payload");
    security_payload_.to_json(j);
  }
  j.end_obj();
}

const char* security_alg_cfg_s::ci_phe_algorithm_opts::to_string() const
{
  static const char* options[] = {"nea0", "nea1", "nea2", "nea3", "sm4", "spare4", "spare3", "spare2", "spare1"};
  return convert_enum_idx(options, 9, value, "ciphering_algorithm_e");
}
uint8_t security_alg_cfg_s::ci_phe_algorithm_opts::to_number() const
{
  static const uint8_t options[] = {0, 1, 2, 3, 4};
  return map_enum_number(options, 5, value, "integrity_prot_algorithm_e");
}
const char* security_alg_cfg_s::int_prot_algorithm_opts::to_string() const
{
  static const char* options[] = {"nia0", "nia1", "nia2", "nia3", "sm4", "spare4", "spare3", "spare2", "spare1"};
  return convert_enum_idx(options, 9, value, "integrity_prot_algorithm_e");
}
uint8_t security_alg_cfg_s::int_prot_algorithm_opts::to_number() const
{
  static const uint8_t options[] = {0, 1, 2, 3, 4};
  return map_enum_number(options, 5, value, "ciphering_algorithm_e");
}

// PHR-Config ::= SEQUENCE
SRSASN_CODE phr_cfg_s_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(periodic_phr_timer.pack(bref));
  HANDLE_CODE(prohibit_phr_timer.pack(bref));
  HANDLE_CODE(dl_pathloss_change.pack(bref));

  return SRSASN_SUCCESS;
}
SRSASN_CODE phr_cfg_s_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(periodic_phr_timer.unpack(bref));
  HANDLE_CODE(prohibit_phr_timer.unpack(bref));
  HANDLE_CODE(dl_pathloss_change.unpack(bref));

  return SRSASN_SUCCESS;
}
void phr_cfg_s_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("periodicPHR-Timer", periodic_phr_timer.to_string());
  j.write_str("prohibitPHR-Timer", prohibit_phr_timer.to_string());
  j.write_str("dl-PathlossChange", dl_pathloss_change.to_string());
  j.end_obj();
}
const char* phr_cfg_s_s::periodic_phr_timer_opts::to_string() const
{
  static const char* options[] = {"sf10", "sf20", "sf50", "sf100", "sf200", "sf500", "sf1000", "infinity"};
  return convert_enum_idx(options, 8, value, "phr_cfg_s_s::periodic_phr_timer_e_");
}
int16_t phr_cfg_s_s::periodic_phr_timer_opts::to_number() const
{
  static const int16_t options[] = {10, 20, 50, 100, 200, 500, 1000, -1};
  return map_enum_number(options, 8, value, "phr_cfg_s_s::periodic_phr_timer_e_");
}

const char* phr_cfg_s_s::prohibit_phr_timer_opts::to_string() const
{
  static const char* options[] = {"sf0", "sf10", "sf20", "sf50", "sf100", "sf200", "sf500", "sf1000"};
  return convert_enum_idx(options, 8, value, "phr_cfg_s_s::prohibit_phr_timer_e_");
}
uint16_t phr_cfg_s_s::prohibit_phr_timer_opts::to_number() const
{
  static const uint16_t options[] = {0, 10, 20, 50, 100, 200, 500, 1000};
  return map_enum_number(options, 8, value, "phr_cfg_s_s::prohibit_phr_timer_e_");
}

const char* phr_cfg_s_s::dl_pathloss_change_opts::to_string() const
{
  static const char* options[] = {"dB1", "dB3", "dB6", "infinity"};
  return convert_enum_idx(options, 4, value, "phr_cfg_s_s::dl_pathloss_change_e_");
}
int8_t phr_cfg_s_s::dl_pathloss_change_opts::to_number() const
{
  static const int8_t options[] = {1, 3, 6, -1};
  return map_enum_number(options, 4, value, "phr_cfg_s_s::dl_pathloss_change_e_");
}

// RadioResurceConfigDedicated ::= SEQUENCE
SRSASN_CODE radio_resurce_cfg_ded_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(srb_to_add_present, 1));
  HANDLE_CODE(bref.pack(drb_to_add_mod_list_present, 1));
  HANDLE_CODE(bref.pack(drb_to_release_list_present, 1));
  HANDLE_CODE(bref.pack(periodic_bsr_timer_present, 1));
  HANDLE_CODE(bref.pack(phy_ch_list_cfg_present, 1));
  HANDLE_CODE(bref.pack(security_cfg_present, 1));
  HANDLE_CODE(bref.pack(phr_cfg_present, 1));

  if (srb_to_add_present) {
    HANDLE_CODE(srb_to_add.pack(bref));
  }
  if (drb_to_add_mod_list_present) {
    HANDLE_CODE(pack_dyn_seq_of(bref, drb_to_add_mod_list, 1, 4));
  }
  if (drb_to_release_list_present) {
    HANDLE_CODE(pack_dyn_seq_of(bref, drb_to_release_list, 1, 4));
  }
  if (periodic_bsr_timer_present) {
    HANDLE_CODE(periodic_bsr_timer.pack(bref));
  }
  if (phy_ch_list_cfg_present) {
    HANDLE_CODE(pack_dyn_seq_of(bref, phy_ch_list_cfg, 1, 16));
  }
  if (security_cfg_present) {
    HANDLE_CODE(security_cfg.pack(bref));
  }
  if (phr_cfg_present) {
    HANDLE_CODE(phr_cfg.pack(bref));
  }

  return SRSASN_SUCCESS;
}
SRSASN_CODE radio_resurce_cfg_ded_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(srb_to_add_present, 1));
  HANDLE_CODE(bref.unpack(drb_to_add_mod_list_present, 1));
  HANDLE_CODE(bref.unpack(drb_to_release_list_present, 1));
  HANDLE_CODE(bref.unpack(periodic_bsr_timer_present, 1));
  HANDLE_CODE(bref.unpack(phy_ch_list_cfg_present, 1));
  HANDLE_CODE(bref.unpack(security_cfg_present, 1));
  HANDLE_CODE(bref.unpack(phr_cfg_present, 1));

  if (srb_to_add_present) {
    HANDLE_CODE(srb_to_add.unpack(bref));
  }
  if (drb_to_add_mod_list_present) {
    HANDLE_CODE(unpack_dyn_seq_of(drb_to_add_mod_list, bref, 1, 4));
  }
  if (drb_to_release_list_present) {
    HANDLE_CODE(unpack_dyn_seq_of(drb_to_release_list, bref, 1, 4));
  }
  if (periodic_bsr_timer_present) {
    HANDLE_CODE(periodic_bsr_timer.unpack(bref));
  }
  if (phy_ch_list_cfg_present) {
    HANDLE_CODE(unpack_dyn_seq_of(phy_ch_list_cfg, bref, 1, 16));
  }
  if (security_cfg_present) {
    HANDLE_CODE(security_cfg.unpack(bref));
  }
  if (phr_cfg_present) {
    HANDLE_CODE(phr_cfg.unpack(bref));
  }
  return SRSASN_SUCCESS;
}
void radio_resurce_cfg_ded_s::to_json(json_writer& j) const
{
  j.start_obj();
  if (srb_to_add_present) {
    j.write_fieldname("SRB-ToAdd");
    phr_cfg.to_json(j);
  }
  if (drb_to_add_mod_list_present) {
    j.start_array("DRB-ToAddModList");
    for (const auto& e1 : drb_to_add_mod_list) {
      e1.to_json(j);
    }
    j.end_array();
  }
  if (drb_to_release_list_present) {
    j.start_array("DRB-ToReleaseList");
    for (const auto& e1 : drb_to_release_list) {
      e1.to_json(j);
    }
  }
  if (periodic_bsr_timer_present) {
    j.write_str("PeriodicBSR-Timer", periodic_bsr_timer.to_string());
  }
  if (phy_ch_list_cfg_present) {
    j.start_array("PhysicalChannelList-Config");
    for (const auto& e1 : phy_ch_list_cfg) {
      e1.to_json(j);
    }
    j.end_array();
  }
  if (security_cfg_present) {
    j.write_fieldname("SecurityConfig	");
    security_cfg.to_json(j);
  }
  if (phr_cfg_present) {
    j.write_fieldname("PHR-Config");
    phr_cfg.to_json(j);
  }
  j.end_obj();
}
void radio_resurce_cfg_ded_s::phr_cfg_s_c_::set(types::options e)
{
  type_ = e;
}

void radio_resurce_cfg_ded_s::phr_cfg_s_c_::set_relese()
{
  set(types::relese);
}

phr_cfg_s_s& radio_resurce_cfg_ded_s::phr_cfg_s_c_::set_setup()
{
  set(types::setup);
  return c;
}

void radio_resurce_cfg_ded_s::phr_cfg_s_c_::to_json(json_writer& j) const
{
  j.start_obj();
  switch (type_) {
    case types::relese:
      break;
    case types::setup:
      j.start_obj();
      j.write_fieldname("PHR-Config");
      c.to_json(j);
      j.end_obj();
      break;
    default:
      log_invalid_choice_id(type_, "radio_resurce_cfg_ded_s::phr_cfg_c_");
  }
  j.end_obj();
}

SRSASN_CODE radio_resurce_cfg_ded_s::phr_cfg_s_c_::pack(bit_ref& bref) const
{
  type_.pack(bref);
  switch (type_) {
    case types::relese:
      break;
    case types::setup:
      HANDLE_CODE(c.pack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "radio_resurce_cfg_ded_s::phr_cfg_c_");
      return SRSASN_ERROR_ENCODE_FAIL;
  }
  return SRSASN_SUCCESS;
}

SRSASN_CODE radio_resurce_cfg_ded_s::phr_cfg_s_c_::unpack(cbit_ref& bref)
{
  types e;
  e.unpack(bref);
  set(e);
  switch (type_) {
    case types::relese:
      break;
    case types::setup:
      HANDLE_CODE(c.unpack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "radio_resurce_cfg_ded_s::phr_cfg_c_");
      return SRSASN_ERROR_DECODE_FAIL;
  }
  return SRSASN_SUCCESS;
}

const char* radio_resurce_cfg_ded_s::phr_cfg_s_c_::types_opts::to_string() const
{
  static const char* options[] = {"relese", "setup"};
  return convert_enum_idx(options, 2, value, "radio_resurce_cfg_ded_s::phr_cfg_c_::types");
}

const char* radio_resurce_cfg_ded_s::periodic_bsr_timer_opts::to_string() const
{
  static const char* options[] = {"rf2", "rf5", "rf10", "rf16", "rf20", "rf32", "infinity", "spare1"};
  return convert_enum_idx(options, 8, value, "radio_resurce_cfg_ded_s::periodic_bsr_timer_e_");
}
uint8_t radio_resurce_cfg_ded_s::periodic_bsr_timer_opts::to_number() const
{
  static const uint8_t options[] = {2, 5, 10, 16, 20, 32, 33, 34};
  return map_enum_number(options, 8, value, "radio_resurce_cfg_ded_s::periodic_bsr_timer_e_");
}

// RRC-TransasctionIdentifier ::= SEQUENCE
SRSASN_CODE rrc_transaction_id_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(pack_integer(bref, rrc_t_id, (uint8_t)0, (uint8_t)3));

  return SRSASN_SUCCESS;
}
SRSASN_CODE rrc_transaction_id_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(unpack_integer(rrc_t_id, bref, (uint8_t)0, (uint8_t)3));

  return SRSASN_SUCCESS;
}
void rrc_transaction_id_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_int("RRC-TransactionIdentifier", rrc_t_id);
  j.end_obj();
}

// RedirectionInfo ::=SEQUENCE
SRSASN_CODE redirection_info_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(pack_integer(bref, beam_id, (uint8_t)1, (uint8_t)15));

  return SRSASN_SUCCESS;
}
SRSASN_CODE redirection_info_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(unpack_integer(beam_id, bref, (uint8_t)1, (uint8_t)15));

  return SRSASN_SUCCESS;
}
void redirection_info_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_int("beamIndex", beam_id);
  j.end_obj();
}

// RRCConnectionReestablishment - r1 - IEs :: = SEQUENCE
SRSASN_CODE rrc_con_reest_r1_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(rr_cfg_ded.pack(bref));

  return SRSASN_SUCCESS;
}
SRSASN_CODE rrc_con_reest_r1_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(rr_cfg_ded.unpack(bref));

  return SRSASN_SUCCESS;
}
void rrc_con_reest_r1_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("RadioResurceConfigDedicated");
  rr_cfg_ded.to_json(j);
  j.end_obj();
}

// RRCConnectionReestablishmentReject-r1-IEs ::=SEQUENCE
SRSASN_CODE rrc_con_reest_reject_r1_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);

  return SRSASN_SUCCESS;
}
SRSASN_CODE rrc_con_reest_reject_r1_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);

  return SRSASN_SUCCESS;
}
void rrc_con_reest_reject_r1_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.end_obj();
}

// RRCConnectionReject-r1-IEs ::=SEQUENCE
SRSASN_CODE rrc_con_reject_r1_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(redir_info_present, 1));
  HANDLE_CODE(bref.pack(geo_accept_present, 1));

  if (geo_accept_present) {
    HANDLE_CODE(redir_info.pack(bref));
  }

  return SRSASN_SUCCESS;
}
SRSASN_CODE rrc_con_reject_r1_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(redir_info_present, 1));
  HANDLE_CODE(bref.unpack(geo_accept_present, 1));

  if (geo_accept_present) {
    HANDLE_CODE(redir_info.unpack(bref));
  }
  return SRSASN_SUCCESS;
}
void rrc_con_reject_r1_s::to_json(json_writer& j) const
{
  j.start_obj();
  if (geo_accept_present) {
    j.write_fieldname("RedirectionInfo");
    redir_info.to_json(j);
  }
  j.write_str("geoAccept", "true");
  j.end_obj();
}

// RRCConnectionSetup-r1-IEs ::= SEQUENCE
SRSASN_CODE rrc_con_setup_r1_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(rr_cfg_ded.pack(bref));

  return SRSASN_SUCCESS;
}
SRSASN_CODE rrc_con_setup_r1_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(rr_cfg_ded.unpack(bref));

  return SRSASN_SUCCESS;
}
void rrc_con_setup_r1_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("RRCConnectionSetup-r1-IEs");
  rr_cfg_ded.to_json(j);
  j.end_obj();
}

// RRCConnectionReestablishment ::=SEQUENCE
SRSASN_CODE rrc_con_reest_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(rrc_transaction_id.pack(bref));
  HANDLE_CODE(rrc_con_reest_r1.pack(bref));

  return SRSASN_SUCCESS;
}
SRSASN_CODE rrc_con_reest_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(rrc_transaction_id.unpack(bref));
  HANDLE_CODE(rrc_con_reest_r1.unpack(bref));

  return SRSASN_SUCCESS;
}
void rrc_con_reest_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("RRC-TransactionIdentifier");
  rrc_transaction_id.to_json(j);
  j.write_fieldname("RRCConnectionReestablishment-r1-IEs");
  rrc_con_reest_r1.to_json(j);
  j.end_obj();
}

// RRCConnectionReestablishmentReject ::=SEQUENCE
SRSASN_CODE rrc_con_reest_reject_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(rrc_con_reest_reject_r1.pack(bref));

  return SRSASN_SUCCESS;
}
SRSASN_CODE rrc_con_reest_reject_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(rrc_con_reest_reject_r1.unpack(bref));

  return SRSASN_SUCCESS;
}
void rrc_con_reest_reject_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("RRCConnectionReestablishmentReject-r1-IEs");
  rrc_con_reest_reject_r1.to_json(j);
  j.end_obj();
}

// RRCConnectionReject ::=SEQUENCE
SRSASN_CODE rrc_con_reject_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(rrc_con_reject.pack(bref));

  return SRSASN_SUCCESS;
}
SRSASN_CODE rrc_con_reject_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(rrc_con_reject.unpack(bref));

  return SRSASN_SUCCESS;
}
void rrc_con_reject_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("RRCConnectionReject-r1-IEs");
  rrc_con_reject.to_json(j);
  j.end_obj();
}

// RRConnectionSetup :: = SEQUENCE
SRSASN_CODE rrc_con_setup_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(rrc_transaction_id.pack(bref));
  HANDLE_CODE(rrc_con_setup_r1.pack(bref));

  return SRSASN_SUCCESS;
}
SRSASN_CODE rrc_con_setup_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(rrc_transaction_id.unpack(bref));
  HANDLE_CODE(rrc_con_setup_r1.unpack(bref));

  return SRSASN_SUCCESS;
}
void rrc_con_setup_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("RRC-TransactionIdentifier");
  rrc_transaction_id.to_json(j);
  j.write_fieldname("RRCConnectionSetup-r1-IEs");
  rrc_con_setup_r1.to_json(j);
  j.end_obj();
}
// DL-CCCH-MessageType ::= CHOICE
void s_dl_ccch_msg_type_c::destroy_()
{
  switch (type_) {
    case types::rrc_con_reest:
      c.destroy<rrc_con_reest_s>();
      break;
    case types::rrc_con_reest_reject:
      c.destroy<rrc_con_reest_reject_s>();
      break;
    case types::rrc_con_reject:
      c.destroy<rrc_con_reject_s>();
      break;
    case types::rrc_con_setup:
      c.destroy<rrc_con_setup_s>();
      break;
    default:
      break;
  }
}
void s_dl_ccch_msg_type_c::set(types::options e)
{
  destroy_();
  type_ = e;
  switch (type_) {
    case types::rrc_con_reest:
      c.init<rrc_con_reest_s>();
      break;
    case types::rrc_con_reest_reject:
      c.init<rrc_con_reest_reject_s>();
      break;
    case types::rrc_con_reject:
      c.init<rrc_con_reject_s>();
      break;
    case types::rrc_con_setup:
      c.init<rrc_con_setup_s>();
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "s_dl_ccch_msg_type_c");
  }
}
s_dl_ccch_msg_type_c::s_dl_ccch_msg_type_c(const s_dl_ccch_msg_type_c& other)
{
  type_ = other.type();
  switch (type_) {
    case types::rrc_con_reest:
      c.init(other.c.get<rrc_con_reest_s>());
      break;
    case types::rrc_con_reest_reject:
      c.init(other.c.get<rrc_con_reest_reject_s>());
      break;
    case types::rrc_con_reject:
      c.init(other.c.get<rrc_con_reject_s>());
      break;
    case types::rrc_con_setup:
      c.init(other.c.get<rrc_con_setup_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "s_dl_ccch_msg_type_c");
  }
}
s_dl_ccch_msg_type_c& s_dl_ccch_msg_type_c::operator=(const s_dl_ccch_msg_type_c& other)
{
  if (this == &other) {
    return *this;
  }
  set(other.type());
  switch (type_) {
    case types::rrc_con_reest:
      c.set(other.c.get<rrc_con_reest_s>());
      break;
    case types::rrc_con_reest_reject:
      c.set(other.c.get<rrc_con_reest_reject_s>());
      break;
    case types::rrc_con_reject:
      c.set(other.c.get<rrc_con_reject_s>());
      break;
    case types::rrc_con_setup:
      c.set(other.c.get<rrc_con_setup_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "s_dl_ccch_msg_type_c");
  }

  return *this;
}
rrc_con_reest_s& s_dl_ccch_msg_type_c::set_rrc_con_reest()
{
  set(types::rrc_con_reest);
  return c.get<rrc_con_reest_s>();
}
rrc_con_reest_reject_s& s_dl_ccch_msg_type_c::set_rrc_con_reest_reject()
{
  set(types::rrc_con_reest_reject);
  return c.get<rrc_con_reest_reject_s>();
}
rrc_con_reject_s& s_dl_ccch_msg_type_c::set_rrc_con_reject()
{
  set(types::rrc_con_reject);
  return c.get<rrc_con_reject_s>();
}
rrc_con_setup_s& s_dl_ccch_msg_type_c::set_rrc_con_setup()
{
  set(types::rrc_con_setup);
  return c.get<rrc_con_setup_s>();
}
void s_dl_ccch_msg_type_c::to_json(json_writer& j) const
{
  j.start_obj();
  switch (type_) {
    case types::rrc_con_reest:
      j.write_fieldname("rrcConnectionReestablishment");
      c.get<rrc_con_reest_s>().to_json(j);
      break;
    case types::rrc_con_reest_reject:
      j.write_fieldname("rrcConnectionReestablishmentReject");
      c.get<rrc_con_reest_reject_s>().to_json(j);
      break;
    case types::rrc_con_reject:
      j.write_fieldname("rrcConnectionReject");
      c.get<rrc_con_reject_s>().to_json(j);
      break;
    case types::rrc_con_setup:
      j.write_fieldname("rrcConnectionSetup");
      c.get<rrc_con_setup_s>().to_json(j);
      break;
    default:
      log_invalid_choice_id(type_, "s_dl_ccch_msg_type_c");
  }
  j.end_obj();
}
SRSASN_CODE s_dl_ccch_msg_type_c::pack(bit_ref& bref) const
{
  type_.pack(bref);
  switch (type_) {
    case types::rrc_con_reest:
      HANDLE_CODE(c.get<rrc_con_reest_s>().pack(bref));
      break;
    case types::rrc_con_reest_reject:
      HANDLE_CODE(c.get<rrc_con_reest_reject_s>().pack(bref));
      break;
    case types::rrc_con_reject:
      HANDLE_CODE(c.get<rrc_con_reject_s>().pack(bref));
      break;
    case types::rrc_con_setup:
      HANDLE_CODE(c.get<rrc_con_setup_s>().pack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "s_dl_ccch_msg_type_c");
      return SRSASN_ERROR_ENCODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE s_dl_ccch_msg_type_c::unpack(cbit_ref& bref)
{
  types e;
  e.unpack(bref);
  set(e);
  switch (type_) {
    case types::rrc_con_reest:
      HANDLE_CODE(c.get<rrc_con_reest_s>().unpack(bref));
      break;
    case types::rrc_con_reest_reject:
      HANDLE_CODE(c.get<rrc_con_reest_reject_s>().unpack(bref));
      break;
    case types::rrc_con_reject:
      HANDLE_CODE(c.get<rrc_con_reject_s>().unpack(bref));
      break;
    case types::rrc_con_setup:
      HANDLE_CODE(c.get<rrc_con_setup_s>().unpack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "s_dl_ccch_msg_type_c");
      return SRSASN_ERROR_DECODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
const char* s_dl_ccch_msg_type_c::types_opts::to_string() const
{
  static const char* options[] = {"rrc_con_reest", "rrc_con_reest_reject", "rrc_con_reject", "rrc_con_setup"};
  return convert_enum_idx(options, 4, value, "s_dl_ccch_msg_type_c::types");
}

// DL-CCCH-Message ::= SEQUENCE
SRSASN_CODE s_dl_ccch_msg_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(msg.pack(bref));

  bref.align_bytes_zero();

  return SRSASN_SUCCESS;
}
SRSASN_CODE s_dl_ccch_msg_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(msg.unpack(bref));

  bref.align_bytes();

  return SRSASN_SUCCESS;
}
void s_dl_ccch_msg_s::to_json(json_writer& j) const
{
  j.start_array();
  j.start_obj();
  j.start_obj("DL-CCCH-Message");
  j.write_fieldname("message");
  msg.to_json(j);
  j.end_obj();
  j.end_obj();
  j.end_array();
}