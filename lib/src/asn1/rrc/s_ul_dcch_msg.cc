/*******************************************************************************
 *
 *                     UL-DCCH Ghannel Information
 *
 ******************************************************************************/

#include "srsran/asn1/rrc/s_ul_dcch_msg.h"
#include "srsran/asn1/rrc/mib_sib_asn1.h"
#include <sstream>

using namespace asn1;
using namespace asn1::rrc;

/*******************************************************************************
 *                                Struct Methods
 ******************************************************************************/
// MeasResult ::=SEQUENCE
SRSASN_CODE meas_result_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(meas_res_present, 1));

  HANDLE_CODE(beam_id.pack(bref));

  if (meas_res_present) {
    HANDLE_CODE(meas_res.rssi.pack(bref));
  }

  return SRSASN_SUCCESS;
}
SRSASN_CODE meas_result_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(meas_res_present, 1));

  HANDLE_CODE(beam_id.unpack(bref));

  if (meas_res_present) {
    HANDLE_CODE(meas_res.rssi.unpack(bref));
  }
  return SRSASN_SUCCESS;
}
void meas_result_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("BeamIdentity");
  beam_id.to_json(j);

  if (meas_res_present) {
    j.write_fieldname("measResult");
    meas_res.rssi.to_json(j);
  }

  j.end_obj();
}

// AMF-Identifier ::=SEQUENCE
SRSASN_CODE amf_id_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(amf_id.pack(bref));

  return SRSASN_SUCCESS;
}
SRSASN_CODE amf_id_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(amf_id.unpack(bref));

  return SRSASN_SUCCESS;
}
void amf_id_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("AMF-Identifier", amf_id.to_string());
  j.end_obj();
}

// RegisteredAMF ::=SEQUENCE
SRSASN_CODE registered_amf_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(bref.pack(plmn_id_present, 1));

  if (plmn_id_present) {
    HANDLE_CODE(plmn_id.pack(bref));
  }
  HANDLE_CODE(amf_id.pack(bref));

  return SRSASN_SUCCESS;
}
SRSASN_CODE registered_amf_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(bref.unpack(plmn_id_present, 1));

  if (plmn_id_present) {
    HANDLE_CODE(plmn_id.unpack(bref));
  }
  HANDLE_CODE(amf_id.unpack(bref));

  return SRSASN_SUCCESS;
}
void registered_amf_s::to_json(json_writer& j) const
{
  j.start_obj();

  if (plmn_id_present) {
    j.write_fieldname("PLMN-Identity");
    plmn_id.to_json(j);
  }

  j.write_fieldname("AMF-Identifier");
  amf_id.to_json(j);

  j.end_obj();
}

// S-NSSAI ::=CHOICE
void s_nssai_c::destroy_()
{
  switch (type_) {
    case types::sst:
      c.destroy<fixed_bitstring<8> >();
      break;
    case types::sst_sd:
      c.destroy<fixed_bitstring<32> >();
      break;
    default:
      break;
  }
}
void s_nssai_c::set(types::options e)
{
  destroy_();
  type_ = e;
  switch (type_) {
    case types::sst:
      c.init<fixed_bitstring<8> >();
      break;
    case types::sst_sd:
      c.init<fixed_bitstring<32> >();
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "s_nssai_c");
  }
}
s_nssai_c::s_nssai_c(const s_nssai_c& other)
{
  type_ = other.type();
  switch (type_) {
    case types::sst:
      c.init(other.c.get<fixed_bitstring<8> >());
      break;
    case types::sst_sd:
      c.init(other.c.get<fixed_bitstring<32> >());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "s_dl_ccch_msg_type_c");
  }
}
s_nssai_c& s_nssai_c::operator=(const s_nssai_c& other)
{
  if (this == &other) {
    return *this;
  }
  set(other.type());
  switch (type_) {
    case types::sst:
      c.set(other.c.get<fixed_bitstring<8> >());
      break;
    case types::sst_sd:
      c.set(other.c.get<fixed_bitstring<32> >());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "s_nssai_c");
  }

  return *this;
}
fixed_bitstring<8>& s_nssai_c::set_sst()
{
  set(types::sst);
  return c.get<fixed_bitstring<8> >();
}
fixed_bitstring<32>& s_nssai_c::set_sst_sd()
{
  set(types::sst_sd);
  return c.get<fixed_bitstring<32> >();
}
void s_nssai_c::to_json(json_writer& j) const
{
  j.start_obj();
  switch (type_) {
    case types::sst:
      j.write_str("SST", c.get<fixed_bitstring<8> >().to_string());
      break;
    case types::sst_sd:
      j.write_str("SST-SD", c.get<fixed_bitstring<32> >().to_string());
      break;
    default:
      log_invalid_choice_id(type_, "s_nssai_c");
  }
  j.end_obj();
}
SRSASN_CODE s_nssai_c::pack(bit_ref& bref) const
{
  type_.pack(bref);
  switch (type_) {
    case types::sst:
      HANDLE_CODE(c.get<fixed_bitstring<8> >().pack(bref));
      break;
    case types::sst_sd:
      HANDLE_CODE(c.get<fixed_bitstring<32> >().pack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "s_nssai_c");
      return SRSASN_ERROR_ENCODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE s_nssai_c::unpack(cbit_ref& bref)
{
  types e;
  e.unpack(bref);
  set(e);
  switch (type_) {
    case types::sst:
      HANDLE_CODE(c.get<fixed_bitstring<8> >().unpack(bref));
      break;
    case types::sst_sd:
      HANDLE_CODE(c.get<fixed_bitstring<32> >().unpack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "s_nssai_c");
      return SRSASN_ERROR_DECODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
const char* s_nssai_c::types_opts::to_string() const
{
  static const char* options[] = {"sst", "sst_sd"};
  return convert_enum_idx(options, 2, value, "s_nssai_c::types");
}

// Rlf-Report-r1 ::=SEQUENCE
SRSASN_CODE rlf_report_r1_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(meas_res_neigh_beam_present, 1));

  HANDLE_CODE(meas_res_last_serv_beam.rssi.pack(bref));

  if (meas_res_neigh_beam_present) {
    HANDLE_CODE(bref.pack(meas_res_neigh_beam.meas_res_list_present, 1));
    if (meas_res_neigh_beam.meas_res_list_present) {
      HANDLE_CODE(pack_dyn_seq_of(bref, meas_res_neigh_beam.meas_res_list, 1, 15));
    }
  }

  return SRSASN_SUCCESS;
}
SRSASN_CODE rlf_report_r1_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(meas_res_neigh_beam_present, 1));

  HANDLE_CODE(meas_res_last_serv_beam.rssi.unpack(bref));

  if (meas_res_neigh_beam_present) {
    HANDLE_CODE(bref.unpack(meas_res_neigh_beam.meas_res_list_present, 1));
    if (meas_res_neigh_beam.meas_res_list_present) {
      HANDLE_CODE(unpack_dyn_seq_of(meas_res_neigh_beam.meas_res_list, bref, 1, 15));
    }
  }

  return SRSASN_SUCCESS;
}
void rlf_report_r1_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("MeasResultLastServBeam");
  meas_res_last_serv_beam.rssi.to_json(j);

  if (meas_res_neigh_beam_present) {
    if (meas_res_neigh_beam.meas_res_list_present) {
      j.start_array("MeasResultList");
      for (uint32_t i1 = 0; i1 < meas_res_neigh_beam.meas_res_list.size(); ++i1) {
        meas_res_neigh_beam.meas_res_list[i1].to_json(j);
      }
      j.end_array();
    }
  }
  j.end_obj();
}

// DedicatedInfoSCM ::=SEQUENCE
SRSASN_CODE ded_info_scm_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(ded_info_scm.pack(bref));

  return SRSASN_SUCCESS;
}
SRSASN_CODE ded_info_scm_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(ded_info_scm.unpack(bref));

  return SRSASN_SUCCESS;
}
void ded_info_scm_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("DedicatedInfoSCM", ded_info_scm.to_string());
  j.end_obj();
}

// DedicatedInfoNAS ::=SEQUENCE
SRSASN_CODE ded_info_nas_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(ded_info_nas.pack(bref));

  return SRSASN_SUCCESS;
}
SRSASN_CODE ded_info_nas_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(ded_info_nas.unpack(bref));

  return SRSASN_SUCCESS;
}
void ded_info_nas_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("DedicatedInfoNAS", ded_info_nas.to_string());
  j.end_obj();
}

// MeasResults ::=SEQUENCE
SRSASN_CODE meas_ress_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(pack_dyn_seq_of(bref, beam_list, 1, 16, integer_packer<uint8_t>(0, 15)));

  return SRSASN_SUCCESS;
}
SRSASN_CODE meas_ress_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(unpack_dyn_seq_of(beam_list, bref, 1, 16, integer_packer<uint8_t>(0, 15)));

  return SRSASN_SUCCESS;
}
void meas_ress_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.start_array("BeamList");
  for (const auto& e1 : beam_list) {
    j.write_int(e1);
  }
  j.end_array();
  j.end_obj();
}

// GeographicalInfo ::=SEQUENCE
SRSASN_CODE geo_info_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(geo_info.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE geo_info_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(geo_info.unpack(bref));
  return SRSASN_SUCCESS;
}
void geo_info_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("GeographicalInfo", geo_info.to_string());
  j.end_obj();
}

// MeasurementReport-r1-IEs ::=SEQUENCE
SRSASN_CODE measure_rep_r1_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(meas_result_present, 1));
  HANDLE_CODE(bref.pack(ue_geo_info_present, 1));

  if (meas_result_present) {
    HANDLE_CODE(meas_result.pack(bref));
  }

  if (ue_geo_info_present) {
    HANDLE_CODE(ue_geo_info.pack(bref));
  }

  return SRSASN_SUCCESS;
}
SRSASN_CODE measure_rep_r1_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(meas_result_present, 1));
  HANDLE_CODE(bref.unpack(ue_geo_info_present, 1));

  if (meas_result_present) {
    HANDLE_CODE(meas_result.unpack(bref));
  }

  if (ue_geo_info_present) {
    HANDLE_CODE(ue_geo_info.unpack(bref));
  }
  return SRSASN_SUCCESS;
}
void measure_rep_r1_s::to_json(json_writer& j) const
{
  j.start_obj();
  if (meas_result_present) {
    j.write_fieldname("MeasResults");
    meas_result.to_json(j);
  }

  if (ue_geo_info_present) {
    j.write_fieldname("GeographicalInfo");
    ue_geo_info.to_json(j);
  }
  j.end_obj();
}

// MeasurementReport ::=SEQUENCE
SRSASN_CODE measure_rep_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(measure_rep_r1.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE measure_rep_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(measure_rep_r1.unpack(bref));
  return SRSASN_SUCCESS;
}
void measure_rep_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("MeasurementReport-r1-IEs");
  measure_rep_r1.to_json(j);
  j.end_obj();
}

// RRCCoonnectionReconfigurationComplete-r1-IEs ::=SEQUENCE
SRSASN_CODE rrc_con_recon_com_r1_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  return SRSASN_SUCCESS;
}
SRSASN_CODE rrc_con_recon_com_r1_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  return SRSASN_SUCCESS;
}
void rrc_con_recon_com_r1_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.end_obj();
}

// RRCCoonnectionReconfigurationComplete ::=SEQUENCE
SRSASN_CODE rrc_con_recon_comp_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(rrc_transaction_id.pack(bref));
  HANDLE_CODE(rrc_con_recon_com_r1.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE rrc_con_recon_comp_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(rrc_transaction_id.unpack(bref));
  HANDLE_CODE(rrc_con_recon_com_r1.unpack(bref));
  return SRSASN_SUCCESS;
}
void rrc_con_recon_comp_s::to_json(json_writer& j) const
{
  j.start_obj();

  j.write_fieldname("RRC-TransasctionIdentifier");
  rrc_transaction_id.to_json(j);

  j.write_fieldname("RRCCoonnectionReconfigurationComplete-r1-IEs");
  rrc_con_recon_com_r1.to_json(j);

  j.end_obj();
}

// RRCConnectionReestablishmentComplete-r1-IEs ::=SEQUENCE
SRSASN_CODE rrc_con_reest_com_r1_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(rlf_info_ava_r1_e, 1));

  return SRSASN_SUCCESS;
}
SRSASN_CODE rrc_con_reest_com_r1_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(rlf_info_ava_r1_e, 1));

  return SRSASN_SUCCESS;
}
void rrc_con_reest_com_r1_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("Rlf-InfoAvailable-r1", "true");
  j.end_obj();
}

// RRCConnectionReestablishmentComplete ::=SEQUENCE
SRSASN_CODE rrc_con_reest_comp_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(rrc_transaction_id.pack(bref));
  HANDLE_CODE(rrc_con_reest_com_r1.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE rrc_con_reest_comp_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(rrc_transaction_id.unpack(bref));
  HANDLE_CODE(rrc_con_reest_com_r1.unpack(bref));
  return SRSASN_SUCCESS;
}
void rrc_con_reest_comp_s::to_json(json_writer& j) const
{
  j.start_obj();

  j.write_fieldname("RRC-TransasctionIdentifier");
  rrc_transaction_id.to_json(j);

  j.write_fieldname("RRCCoonnectionReestablishmentComplete-r1-IEs");
  rrc_con_reest_com_r1.to_json(j);

  j.end_obj();
}

// RRCConnectionSetupComplete-r1-IEs ::=SEQUENCE
SRSASN_CODE rrc_con_setup_com_r1_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(registered_amf_present, 1));
  HANDLE_CODE(bref.pack(s_nssai_list_present, 1));
  HANDLE_CODE(bref.pack(ded_info_nas_present, 1));
  HANDLE_CODE(pack_integer(bref, selecte_plmn_id, (uint8_t)1u, (uint8_t)4u));
  if (registered_amf_present) {
    HANDLE_CODE(registered_amf.pack(bref));
  }
  if (s_nssai_list_present) {
    HANDLE_CODE(pack_dyn_seq_of(bref, s_nssai_list, 1, 8));
  }
  if (ded_info_nas_present) {
    HANDLE_CODE(ded_info_nas.pack(bref));
  }

  return SRSASN_SUCCESS;
}
SRSASN_CODE rrc_con_setup_com_r1_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(registered_amf_present, 1));
  HANDLE_CODE(bref.unpack(s_nssai_list_present, 1));
  HANDLE_CODE(bref.unpack(ded_info_nas_present, 1));
  HANDLE_CODE(unpack_integer(selecte_plmn_id, bref, (uint8_t)1u, (uint8_t)4u));
  if (registered_amf_present) {
    HANDLE_CODE(registered_amf.unpack(bref));
  }
  if (s_nssai_list_present) {
    HANDLE_CODE(unpack_dyn_seq_of(s_nssai_list, bref, 1, 8));
  }
  if (ded_info_nas_present) {
    HANDLE_CODE(ded_info_nas.unpack(bref));
  }

  return SRSASN_SUCCESS;
}
void rrc_con_setup_com_r1_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_int("SelectedPLMN-Identity", selecte_plmn_id);
  if (registered_amf_present) {
    j.write_fieldname("RegisteredAMF");
    registered_amf.to_json(j);
  }
  if (s_nssai_list_present) {
    j.start_obj();
    j.start_array("S-NSSAI-List");
    for (const auto& e1 : s_nssai_list) {
      e1.to_json(j);
    }
    j.end_array();
  }

  if (ded_info_nas_present) {
    j.write_fieldname("DedicatedInfoNAS");
    ded_info_nas.to_json(j);
  }

  j.end_obj();
}

// RRCConnectionSetupComplete ::=SEQUENCE
SRSASN_CODE rrc_con_setup_comp_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(rrc_transaction_id.pack(bref));
  HANDLE_CODE(rrc_con_setup_com_r1.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE rrc_con_setup_comp_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(rrc_transaction_id.unpack(bref));
  HANDLE_CODE(rrc_con_setup_com_r1.unpack(bref));
  return SRSASN_SUCCESS;
}
void rrc_con_setup_comp_s::to_json(json_writer& j) const
{
  j.start_obj();

  j.write_fieldname("RRC-TransasctionIdentifier");
  rrc_transaction_id.to_json(j);

  j.write_fieldname("RRCConnectionSetupComplete-r1-IEs");
  rrc_con_setup_com_r1.to_json(j);

  j.end_obj();
}

// SecurityModeComplete-r1-IEs ::=SEQUENCE
SRSASN_CODE security_mode_comp_r1_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  return SRSASN_SUCCESS;
}
SRSASN_CODE security_mode_comp_r1_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  return SRSASN_SUCCESS;
}
void security_mode_comp_r1_s::to_json(json_writer& j) const
{
  j.start_obj();

  j.end_obj();
}

// SecurityModeComplete ::=SEQUENCE
SRSASN_CODE security_mode_comp_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(rrc_transaction_id.pack(bref));
  HANDLE_CODE(security_mode_comp_r1.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE security_mode_comp_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(rrc_transaction_id.unpack(bref));
  HANDLE_CODE(security_mode_comp_r1.unpack(bref));
  return SRSASN_SUCCESS;
}
void security_mode_comp_s::to_json(json_writer& j) const
{
  j.start_obj();

  j.write_fieldname("RRC-TransasctionIdentifier");
  rrc_transaction_id.to_json(j);

  j.write_fieldname("SecurityModeComplete-r1-IEs");
  security_mode_comp_r1.to_json(j);

  j.end_obj();
}

// SecurityModeFailure-r1-IEs ::=SEQUENCE
SRSASN_CODE security_mode_fail_r1_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  return SRSASN_SUCCESS;
}
SRSASN_CODE security_mode_fail_r1_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  return SRSASN_SUCCESS;
}
void security_mode_fail_r1_s::to_json(json_writer& j) const
{
  j.start_obj();

  j.end_obj();
}

// SecurityModeFailure ::=SEQUENCE
SRSASN_CODE secur_mode_fail_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(rrc_transaction_id.pack(bref));
  HANDLE_CODE(security_mode_fail_r1.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE secur_mode_fail_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(rrc_transaction_id.unpack(bref));
  HANDLE_CODE(security_mode_fail_r1.unpack(bref));
  return SRSASN_SUCCESS;
}
void secur_mode_fail_s::to_json(json_writer& j) const
{
  j.start_obj();

  j.write_fieldname("RRC-TransasctionIdentifier");
  rrc_transaction_id.to_json(j);

  j.write_fieldname("SecurityModeFailure-r1-IEs");
  security_mode_fail_r1.to_json(j);

  j.end_obj();
}

// UEGeoInfoTransfer-r1-IEs ::=SEQUENCE
SRSASN_CODE ue_geo_info_trans_r1_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(ue_geo_info.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE ue_geo_info_trans_r1_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(ue_geo_info.unpack(bref));
  return SRSASN_SUCCESS;
}
void ue_geo_info_trans_r1_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("GeographicalInfo");
  ue_geo_info.to_json(j);
  j.end_obj();
}

// UEGeoInfoTransfer ::=SEQUENCE
SRSASN_CODE ue_geo_info_trans_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(ue_geo_info_trans_r1.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE ue_geo_info_trans_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(ue_geo_info_trans_r1.unpack(bref));
  return SRSASN_SUCCESS;
}
void ue_geo_info_trans_s::to_json(json_writer& j) const
{
  j.start_obj();

  j.write_fieldname("UEGeoInfoTransfer-r1-IEs");
  ue_geo_info_trans_r1.to_json(j);

  j.end_obj();
}

// UEIdentity-Transfer-r1-IEs ::=SEQUENCE
SRSASN_CODE ue_id_trans_r1_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(ng_nr_s_tmsi.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE ue_id_trans_r1_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(ng_nr_s_tmsi.unpack(bref));
  return SRSASN_SUCCESS;
}
void ue_id_trans_r1_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_fieldname("5G-S-TMSI");
  ng_nr_s_tmsi.to_json(j);
  j.end_obj();
}

// UEIdentity-Transfer ::=SEQUENCE
SRSASN_CODE ue_id_trans_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(ue_id_trans_r1.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE ue_id_trans_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(ue_id_trans_r1.unpack(bref));
  return SRSASN_SUCCESS;
}
void ue_id_trans_s::to_json(json_writer& j) const
{
  j.start_obj();

  j.write_fieldname("UEIdentity-Transfer-r1-IEs");
  ue_id_trans_r1.to_json(j);

  j.end_obj();
}

// UEInformationResponse-r1-IEs ::=SEQUENCE
SRSASN_CODE ue_info_response_r1_s::pack(bit_ref& bref) const
{
  bref.pack(ext, 1);
  HANDLE_CODE(bref.pack(rlf_report_present, 1));
  HANDLE_CODE(pack_integer(bref, rach_report_r1.num_of_rach_sent_r1, (uint8_t)1u, (uint8_t)15u));

  if (rlf_report_present) {
    HANDLE_CODE(rlf_report.pack(bref));
  }

  return SRSASN_SUCCESS;
}
SRSASN_CODE ue_info_response_r1_s::unpack(cbit_ref& bref)
{
  bref.unpack(ext, 1);
  HANDLE_CODE(bref.unpack(rlf_report_present, 1));
  HANDLE_CODE(unpack_integer(rach_report_r1.num_of_rach_sent_r1, bref, (uint8_t)1u, (uint8_t)15u));

  if (rlf_report_present) {
    HANDLE_CODE(rlf_report.unpack(bref));
  }
  return SRSASN_SUCCESS;
}
void ue_info_response_r1_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_int("Number-Of-RACH-Sent-r1", rach_report_r1.num_of_rach_sent_r1);

  if (rlf_report_present) {
    j.write_fieldname("Rlf-Report-r1");
    rlf_report.to_json(j);
  }

  j.end_obj();
}

// UEInformationResponse-r1 ::=SEQUENCE
SRSASN_CODE ue_info_response_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(rrc_transaction_id.pack(bref));
  HANDLE_CODE(ue_info_response_r1.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE ue_info_response_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(rrc_transaction_id.unpack(bref));
  HANDLE_CODE(ue_info_response_r1.unpack(bref));
  return SRSASN_SUCCESS;
}
void ue_info_response_s::to_json(json_writer& j) const
{
  j.start_obj();

  j.write_fieldname("RRC-TransasctionIdentifier");
  rrc_transaction_id.to_json(j);

  j.write_fieldname("UEInformationResponse-r1-IEs");
  ue_info_response_r1.to_json(j);

  j.end_obj();
}

// ULInformationTransfer-r1-IEs ::=SEQUENCE
SRSASN_CODE ul_info_trans_r1_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(ded_info_type.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE ul_info_trans_r1_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(ded_info_type.unpack(bref));
  return SRSASN_SUCCESS;
}
void ul_info_trans_r1_s::to_json(json_writer& j) const
{
  j.start_obj();

  j.write_fieldname("DedicatedInfoType");
  ded_info_type.to_json(j);

  j.end_obj();
}

void ded_info_type_c::destroy_()
{
  switch (type_) {
    case types::ded_info_nas:
      c.destroy<ded_info_nas_s>();
      break;
    case types::ded_info_scm:
      c.destroy<ded_info_scm_s>();
      break;
    default:
      break;
  }
}
void ded_info_type_c::set(types::options e)
{
  destroy_();
  type_ = e;
  switch (type_) {
    case types::ded_info_nas:
      c.init<ded_info_nas_s>();
      break;
    case types::ded_info_scm:
      c.init<ded_info_scm_s>();
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "ul_info_trans_r1_s::ded_info_type_c");
  }
}
ded_info_type_c::ded_info_type_c(const ded_info_type_c& other)
{
  type_ = other.type();
  switch (type_) {
    case types::ded_info_nas:
      c.init(other.c.get<ded_info_nas_s>());
      break;
    case types::ded_info_scm:
      c.init(other.c.get<ded_info_scm_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "ul_info_trans_r1_s::ded_info_type_c");
  }
}
ded_info_type_c&
ded_info_type_c::operator=(const ded_info_type_c& other)
{
  if (this == &other) {
    return *this;
  }
  set(other.type());
  switch (type_) {
    case types::ded_info_nas:
      c.set(other.c.get<ded_info_nas_s>());
      break;
    case types::ded_info_scm:
      c.set(other.c.get<ded_info_scm_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "ul_info_trans_r1_s::ded_info_type_c");
  }

  return *this;
}
ded_info_nas_s& ded_info_type_c::set_ded_info_nas()
{
  set(types::ded_info_nas);
  return c.get<ded_info_nas_s>();
}
ded_info_scm_s& ded_info_type_c::set_ded_info_scm()
{
  set(types::ded_info_scm);
  return c.get<ded_info_scm_s>();
}
void ded_info_type_c::to_json(json_writer& j) const
{
  j.start_obj();
  switch (type_) {
    case types::ded_info_nas:
      j.write_fieldname("DedicatedInfoNAS");
      c.get<ded_info_nas_s>().to_json(j);
      break;
    case types::ded_info_scm:
      j.write_str("DedicatedInfoSCM");
      c.get<ded_info_scm_s>().to_json(j);
      break;
    default:
      log_invalid_choice_id(type_, "ul_info_trans_r1_s::ded_info_type_c");
  }
  j.end_obj();
}
SRSASN_CODE ded_info_type_c::pack(bit_ref& bref) const
{
  type_.pack(bref);
  switch (type_) {
    case types::ded_info_nas:
      HANDLE_CODE(c.get<ded_info_nas_s>().pack(bref));
      break;
    case types::ded_info_scm:
      HANDLE_CODE(c.get<ded_info_scm_s>().pack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "ul_info_trans_r1_s::ded_info_type_c");
      return SRSASN_ERROR_ENCODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE ded_info_type_c::unpack(cbit_ref& bref)
{
  types e;
  e.unpack(bref);
  set(e);
  switch (type_) {
    case types::ded_info_nas:
      HANDLE_CODE(c.get<ded_info_nas_s>().unpack(bref));
      break;
    case types::ded_info_scm:
      HANDLE_CODE(c.get<ded_info_scm_s>().unpack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "ul_info_trans_r1_s::ded_info_type_c");
      return SRSASN_ERROR_DECODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
const char* ded_info_type_c::types_opts::to_string() const
{
  static const char* options[] = {"ded_info_nas", "ded_info_scm"};
  return convert_enum_idx(options, 2, value, "ul_info_trans_r1_s::ded_info_type_c::types");
}

// ULInformationTransfer ::=SEQUENCE
SRSASN_CODE ul_info_trans_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(ul_info_trans_r1.pack(bref));

  return SRSASN_SUCCESS;
}
SRSASN_CODE ul_info_trans_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(ul_info_trans_r1.unpack(bref));

  return SRSASN_SUCCESS;
}
void ul_info_trans_s::to_json(json_writer& j) const
{
  j.start_obj();

  j.write_fieldname("ULInformationTransfer-r1-IEs");
  ul_info_trans_r1.to_json(j);

  j.end_obj();
}

// UL-DCCH-MessageType ::=CHOICE
void s_ul_dcch_msg_type_c::destroy_()
{
  switch (type_) {
    case types::measure_report:
      c.destroy<measure_rep_s>();
      break;
    case types::rrc_con_recon_comp:
      c.destroy<rrc_con_recon_comp_s>();
      break;
    case types::rrc_con_reest_comp:
      c.destroy<rrc_con_reest_comp_s>();
      break;
    case types::rrc_con_setup_comp:
      c.destroy<rrc_con_setup_comp_s>();
      break;
    case types::security_mode_comp:
      c.destroy<security_mode_comp_s>();
      break;
    case types::security_mode_fail:
      c.destroy<secur_mode_fail_s>();
      break;
    case types::ue_geo_info_trans:
      c.destroy<ue_geo_info_trans_s>();
      break;
    case types::ue_id_trans:
      c.destroy<ue_id_trans_s>();
      break;
    case types::ue_info_response:
      c.destroy<ue_info_response_s>();
      break;
    case types::ul_info_trans:
      c.destroy<ul_info_trans_s>();
      break;
    default:
      break;
  }
}
void s_ul_dcch_msg_type_c::set(types::options e)
{
  destroy_();
  type_ = e;
  switch (type_) {
    case types::measure_report:
      c.init<measure_rep_s>();
      break;
    case types::rrc_con_recon_comp:
      c.init<rrc_con_recon_comp_s>();
      break;
    case types::rrc_con_reest_comp:
      c.init<rrc_con_reest_comp_s>();
      break;
    case types::rrc_con_setup_comp:
      c.init<rrc_con_setup_comp_s>();
      break;
    case types::security_mode_comp:
      c.init<security_mode_comp_s>();
      break;
    case types::security_mode_fail:
      c.init<secur_mode_fail_s>();
      break;
    case types::ue_geo_info_trans:
      c.init<ue_geo_info_trans_s>();
      break;
    case types::ue_id_trans:
      c.init<ue_id_trans_s>();
      break;
    case types::ue_info_response:
      c.init<ue_info_response_s>();
      break;
    case types::ul_info_trans:
      c.init<ul_info_trans_s>();
      break;
    default:
      log_invalid_choice_id(type_, "s_ul_dcch_msg_type_c");
  }
}
s_ul_dcch_msg_type_c::s_ul_dcch_msg_type_c(const s_ul_dcch_msg_type_c& other)
{
  type_ = other.type();
  switch (type_) {
    case types::measure_report:
      c.init(other.c.get<measure_rep_s>());
      break;
    case types::rrc_con_recon_comp:
      c.init(other.c.get<rrc_con_recon_comp_s>());
      break;
    case types::rrc_con_reest_comp:
      c.init(other.c.get<rrc_con_reest_comp_s>());
      break;
    case types::rrc_con_setup_comp:
      c.init(other.c.get<rrc_con_setup_comp_s>());
      break;
    case types::security_mode_comp:
      c.init(other.c.get<security_mode_comp_s>());
      break;
    case types::security_mode_fail:
      c.init(other.c.get<secur_mode_fail_s>());
      break;
    case types::ue_geo_info_trans:
      c.init(other.c.get<ue_geo_info_trans_s>());
      break;
    case types::ue_id_trans:
      c.init(other.c.get<ue_id_trans_s>());
      break;
    case types::ue_info_response:
      c.init(other.c.get<ue_info_response_s>());
      break;
    case types::ul_info_trans:
      c.init(other.c.get<ul_info_trans_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "s_ul_dcch_msg_type_c");
  }
}
s_ul_dcch_msg_type_c& s_ul_dcch_msg_type_c::operator=(const s_ul_dcch_msg_type_c& other)
{
  if (this == &other) {
    return *this;
  }
  set(other.type());
  switch (type_) {
    case types::measure_report:
      c.set(other.c.get<measure_rep_s>());
      break;
    case types::rrc_con_recon_comp:
      c.set(other.c.get<rrc_con_recon_comp_s>());
      break;
    case types::rrc_con_reest_comp:
      c.set(other.c.get<rrc_con_reest_comp_s>());
      break;
    case types::rrc_con_setup_comp:
      c.set(other.c.get<rrc_con_setup_comp_s>());
      break;
    case types::security_mode_comp:
      c.set(other.c.get<security_mode_comp_s>());
      break;
    case types::security_mode_fail:
      c.set(other.c.get<secur_mode_fail_s>());
      break;
    case types::ue_geo_info_trans:
      c.set(other.c.get<ue_geo_info_trans_s>());
      break;
    case types::ue_id_trans:
      c.set(other.c.get<ue_id_trans_s>());
      break;
    case types::ue_info_response:
      c.set(other.c.get<ue_info_response_s>());
      break;
    case types::ul_info_trans:
      c.set(other.c.get<ul_info_trans_s>());
      break;
    case types::nulltype:
      break;
    default:
      log_invalid_choice_id(type_, "s_ul_dcch_msg_type_c");
  }

  return *this;
}
measure_rep_s& s_ul_dcch_msg_type_c::set_measure_rep()
{
  set(types::measure_report);
  return c.get<measure_rep_s>();
}
rrc_con_recon_comp_s& s_ul_dcch_msg_type_c::set_rrc_con_recon_comp()
{
  set(types::rrc_con_recon_comp);
  return c.get<rrc_con_recon_comp_s>();
}
rrc_con_reest_comp_s& s_ul_dcch_msg_type_c::set_rrc_con_reest_comp()
{
  set(types::rrc_con_reest_comp);
  return c.get<rrc_con_reest_comp_s>();
}
rrc_con_setup_comp_s& s_ul_dcch_msg_type_c::set_rrc_con_setup_comp()
{
  set(types::rrc_con_setup_comp);
  return c.get<rrc_con_setup_comp_s>();
}
security_mode_comp_s& s_ul_dcch_msg_type_c::set_security_mode_comp()
{
  set(types::security_mode_comp);
  return c.get<security_mode_comp_s>();
}
secur_mode_fail_s& s_ul_dcch_msg_type_c::set_secur_mode_fail()
{
  set(types::security_mode_fail);
  return c.get<secur_mode_fail_s>();
}
ue_geo_info_trans_s& s_ul_dcch_msg_type_c::set_ue_geo_info_trans()
{
  set(types::ue_geo_info_trans);
  return c.get<ue_geo_info_trans_s>();
}
ue_id_trans_s& s_ul_dcch_msg_type_c::set_ue_id_trans()
{
  set(types::ue_id_trans);
  return c.get<ue_id_trans_s>();
}
ue_info_response_s& s_ul_dcch_msg_type_c::set_ue_info_response()
{
  set(types::ue_info_response);
  return c.get<ue_info_response_s>();
}
ul_info_trans_s& s_ul_dcch_msg_type_c::set_ul_info_trans()
{
  set(types::ul_info_trans);
  return c.get<ul_info_trans_s>();
}
void s_ul_dcch_msg_type_c::to_json(json_writer& j) const
{
  j.start_obj();
  switch (type_) {
    case types::measure_report:
      j.write_fieldname("MeasurementReport");
      c.get<measure_rep_s>().to_json(j);
      break;
    case types::rrc_con_recon_comp:
      j.write_fieldname("RRCCoonnectionReconfigurationComplete");
      c.get<rrc_con_recon_comp_s>().to_json(j);
      break;
    case types::rrc_con_reest_comp:
      j.write_fieldname("RRCConnectionReestablishmentComplete");
      c.get<rrc_con_reest_comp_s>().to_json(j);
      break;
    case types::rrc_con_setup_comp:
      j.write_fieldname("RRCConnectionSetupComplete");
      c.get<rrc_con_setup_comp_s>().to_json(j);
      break;
    case types::security_mode_comp:
      j.write_fieldname("SecurityModeComplete");
      c.get<security_mode_comp_s>().to_json(j);
      break;
    case types::security_mode_fail:
      j.write_fieldname("SecurityModeFailure");
      c.get<secur_mode_fail_s>().to_json(j);
      break;
    case types::ue_geo_info_trans:
      j.write_fieldname("UEGeoInfoTransfer");
      c.get<ue_geo_info_trans_s>().to_json(j);
      break;
    case types::ue_id_trans:
      j.write_fieldname("UEIdentity-Transfer");
      c.get<ue_id_trans_s>().to_json(j);
      break;
    case types::ue_info_response:
      j.write_fieldname("UEInformationResponse-r1");
      c.get<ue_info_response_s>().to_json(j);
      break;
    case types::ul_info_trans:
      j.write_fieldname("ULInformationTransfer");
      c.get<ul_info_trans_s>().to_json(j);
      break;
    default:
      log_invalid_choice_id(type_, "s_ul_dcch_msg_type_c");
  }
  j.end_obj();
}
SRSASN_CODE s_ul_dcch_msg_type_c::pack(bit_ref& bref) const
{
  type_.pack(bref);
  switch (type_) {
    case types::measure_report:
      HANDLE_CODE(c.get<measure_rep_s>().pack(bref));
      break;
    case types::rrc_con_recon_comp:
      HANDLE_CODE(c.get<rrc_con_recon_comp_s>().pack(bref));
      break;
    case types::rrc_con_reest_comp:
      HANDLE_CODE(c.get<rrc_con_reest_comp_s>().pack(bref));
      break;
    case types::rrc_con_setup_comp:
      HANDLE_CODE(c.get<rrc_con_setup_comp_s>().pack(bref));
      break;
    case types::security_mode_comp:
      HANDLE_CODE(c.get<security_mode_comp_s>().pack(bref));
      break;
    case types::security_mode_fail:
      HANDLE_CODE(c.get<secur_mode_fail_s>().pack(bref));
      break;
    case types::ue_geo_info_trans:
      HANDLE_CODE(c.get<ue_geo_info_trans_s>().pack(bref));
      break;
    case types::ue_id_trans:
      HANDLE_CODE(c.get<ue_id_trans_s>().pack(bref));
      break;
    case types::ue_info_response:
      HANDLE_CODE(c.get<ue_info_response_s>().pack(bref));
      break;
    case types::ul_info_trans:
      HANDLE_CODE(c.get<ul_info_trans_s>().pack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "s_ul_dcch_msg_type_c");
      return SRSASN_ERROR_ENCODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
SRSASN_CODE s_ul_dcch_msg_type_c::unpack(cbit_ref& bref)
{
  types e;
  e.unpack(bref);
  set(e);
  switch (type_) {
    case types::measure_report:
      HANDLE_CODE(c.get<measure_rep_s>().unpack(bref));
      break;
    case types::rrc_con_recon_comp:
      HANDLE_CODE(c.get<rrc_con_recon_comp_s>().unpack(bref));
      break;
    case types::rrc_con_reest_comp:
      HANDLE_CODE(c.get<rrc_con_reest_comp_s>().unpack(bref));
      break;
    case types::rrc_con_setup_comp:
      HANDLE_CODE(c.get<rrc_con_setup_comp_s>().unpack(bref));
      break;
    case types::security_mode_comp:
      HANDLE_CODE(c.get<security_mode_comp_s>().unpack(bref));
      break;
    case types::security_mode_fail:
      HANDLE_CODE(c.get<secur_mode_fail_s>().unpack(bref));
      break;
    case types::ue_geo_info_trans:
      HANDLE_CODE(c.get<ue_geo_info_trans_s>().unpack(bref));
      break;
    case types::ue_id_trans:
      HANDLE_CODE(c.get<ue_id_trans_s>().unpack(bref));
      break;
    case types::ue_info_response:
      HANDLE_CODE(c.get<ue_info_response_s>().unpack(bref));
      break;
    case types::ul_info_trans:
      HANDLE_CODE(c.get<ul_info_trans_s>().unpack(bref));
      break;
    default:
      log_invalid_choice_id(type_, "s_ul_dcch_msg_type_c");
      return SRSASN_ERROR_DECODE_FAIL;
  }
  return SRSASN_SUCCESS;
}
const char* s_ul_dcch_msg_type_c::types_opts::to_string() const
{
  static const char* options[] = {
      "measure_report",
      "rrc_con_recon_comp",
      "rrc_con_reest_comp",
      "rrc_con_setup_comp",
      "security_mode_comp",
      "security_mode_fail",
      "ue_geo_info_trans",
      "ue_id_trans",
      "ue_info_response",
      "ul_info_trans",
  };
  return convert_enum_idx(options, 10, value, "s_ul_dcch_msg_type_c::types");
}

// UL-DCCH-Message ::=SEQUENCE
SRSASN_CODE s_ul_dcch_msg_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(msg.pack(bref));

  return SRSASN_SUCCESS;
}
SRSASN_CODE s_ul_dcch_msg_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(msg.unpack(bref));

  return SRSASN_SUCCESS;
}
void s_ul_dcch_msg_s::to_json(json_writer& j) const
{
  j.start_obj();

  j.write_fieldname("UL-DCCH-MessageType");
  msg.to_json(j);

  j.end_obj();
}
