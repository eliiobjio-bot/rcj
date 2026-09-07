/*******************************************************************************
 *
 *                     RRC connection establishment process
 *
 ******************************************************************************/

#ifndef SRSASN1_RRC_S_DLDCCH_MSG_H
#define SRSASN1_RRC_S_DLDCCH_MSG_H

#include "rr_common.h"
#include "srsran/asn1/rrc/mib_sib_asn1.h"
#include <cstdio>
#include <stdarg.h>
namespace asn1 {
namespace rrc {

/*******************************************************************************
 *                              Struct Definitions
 ******************************************************************************/
// BandIdentity
struct ba_id_s {
  fixed_bitstring<6> ba_id;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
// FerquencyIdentity
struct freq_id_n_s {
  fixed_bitstring<2> freq_id;
  SRSASN_CODE        pack(bit_ref& bref) const;
  SRSASN_CODE        unpack(cbit_ref& bref);
  void               to_json(json_writer& j) const;
};

// struct beam_id_s {
//   fixed_bitstring<14> beam_id;
//   SRSASN_CODE pack(bit_ref& bref) const;
//   SRSASN_CODE unpack(cbit_ref& bref);
//   void to_json(json_writer& j) const;
// };

// DedicatedInfoSCM
struct dedi_info_scm_s {
  dyn_octstring delicated_info_scm;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
// DedicatedInfoNAS
struct dedi_info_nas_s {
  dyn_octstring delicated_info_nas;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// dedicatedInfoType      CHOICE
struct dedi_info_type_c_ {
  struct types_opts {
    enum options { dedi_info_nas, dedi_info_scm, nulltype } value;

    const char* to_string() const;
  };
  typedef enumerated<types_opts, true> types;
  // choice methods
  dedi_info_type_c_() = default;
  dedi_info_type_c_(const dedi_info_type_c_& other);
  dedi_info_type_c_& operator=(const dedi_info_type_c_& other);
  ~dedi_info_type_c_() { destroy_(); }

  void  set(types::options e = types::nulltype);
  types type() const { return type_; }

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
  // getters
  dedi_info_nas_s& dedi_info_nas()
  {
    assert_choice_type(types::dedi_info_nas, type_, "dedicatedInfoNAS");
    return c.get<dedi_info_nas_s>();
  }
  dedi_info_scm_s& dedi_info_scm()
  {
    assert_choice_type(types::dedi_info_scm, type_, "dedicatedInfoSCM ");
    return c.get<dedi_info_scm_s>();
  }
  const dedi_info_nas_s& dedi_info_nas() const
  {
    assert_choice_type(types::dedi_info_nas, type_, "dedicatedInfoNAS");
    return c.get<dedi_info_nas_s>();
  }
  const dedi_info_scm_s& dedi_info_scm() const
  {
    assert_choice_type(types::dedi_info_scm, type_, "dedicatedInfoSCM ");
    return c.get<dedi_info_scm_s>();
  }
  dedi_info_nas_s& set_dedi_info_nas();
  dedi_info_scm_s& set_dedi_info_scm();

private:
  types                                             type_;
  choice_buffer_t<dedi_info_scm_s, dedi_info_nas_s> c;

  void destroy_();
};
// DLInformationTransfer-r1-IEs ::=       SEQUENCE
struct dl_info_tran_r1_ies_s {
  dedi_info_type_c_ dedi_info_type;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// DLInformation transfer ::=            SEQUENCE
struct dl_info_tran_s {
  dl_info_tran_r1_ies_s dl_info_tran_r1;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// MeasGapConfig-Normal :: =			SEQUENCE
struct meas_gap_cfg_normal_s {
  struct meas_norm_ratio_opts {
    enum options { dB5, dB6, dB7, dB8, dB9, dB10, dB11, dB12, dB13, dB14, dB15, dB16, nulltype } value;
    typedef uint8_t number_type;
    const char*     to_string() const;
    uint8_t         to_number() const;
  };
  typedef enumerated<meas_norm_ratio_opts> meas_norm_ratio_e_;

  uint8_t            beam_index   = 1;
  uint8_t            frame_offset = 0;
  ba_id_s            fcch_ba_id;
  freq_id_n_s        fcch_freq_id;
  meas_norm_ratio_e_ meas_norm_ratio;
  uint8_t            mib_Re_Fra_Num = 0;
  SRSASN_CODE        pack(bit_ref& bref) const;
  SRSASN_CODE        unpack(cbit_ref& bref);
  void               to_json(json_writer& j) const;
};
// MeasGapConfigLsit-Normal :: =		SEQUENCE(SIZE(1..maxGap))OF MeasGapConfig-Normal
using meas_gap_cfg_list_normal_s = dyn_array<meas_gap_cfg_normal_s>;

// MeasGapConfig-DIFreqSpread :: =	SEQUENCE
struct meas_gap_cfg_dl_freq_spread_s {
  uint8_t beam_id          = 1;
  uint8_t sec_syn_group_id = 1;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
// MeasGapConfigList-DIFreqSpread :: =	SEQUENCE(SIZE(1..maxGap))OF MeasGapConfig-DlFreqSpread
using meas_gap_cfg_list_dl_freq_spread_s = dyn_array<meas_gap_cfg_dl_freq_spread_s>;

// Threshold		SEQUENCE
struct thr_hold_s {
  struct time_to_trigger_opts {
    enum options { ms0, ms900, ms1650, ms3200, nulltype } value;

    typedef uint16_t number_type;
    const char*      to_string() const;
    uint16_t         to_number() const;
  };
  typedef enumerated<time_to_trigger_opts> time_to_trigger_e_;

  struct filter_coe_ent_opts {
    enum options { fc0, fc2, fc4, fc8, fc12, gc25, spare2, spare1, nulltype } value;
    typedef uint8_t number_type;
    const char*     to_string() const;
    uint8_t         to_number() const;
  };
  typedef enumerated<filter_coe_ent_opts> filter_coe_ent_e_;
  bool                                    ext        = false;
  int8_t                                  off_set    = -9;
  uint8_t                                 hysteresis = 0;
  time_to_trigger_e_                      time_to_trigger;
  filter_coe_ent_e_                       filter_coe_ent;
  SRSASN_CODE                             pack(bit_ref& bref) const;
  SRSASN_CODE                             unpack(cbit_ref& bref);
  void                                    to_json(json_writer& j) const;
};
// ReportConfig::= SEQUENCE
struct report_cfg_s {
  struct report_geo_grap_info_opts {
    enum options { True, nulltype } value;
    typedef uint8_t number_type;

    const char* to_string() const;
    uint8_t     to_number() const;
  };
  typedef enumerated<report_geo_grap_info_opts> report_geo_grap_info_e_;
  bool                                          ext                          = false;
  bool                                          thr_hold_present             = false;
  bool                                          report_geo_grap_info_present = false;
  thr_hold_s                                    thr_hold;
  report_geo_grap_info_e_                       report_geo_grap_info;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
// measGapConfigList		CHOICE
struct meas_gap_cfg_list_c_ {
  struct types_opts {
    enum options { meas_gap_cfg_list_normal, meas_gap_cfg_list_dl_freq_spread, nulltype } value;
    const char* to_string() const;
  };
  typedef enumerated<types_opts> types;
  // choice methods
  meas_gap_cfg_list_c_() = default;
  meas_gap_cfg_list_c_(const meas_gap_cfg_list_c_& other);
  meas_gap_cfg_list_c_& operator=(const meas_gap_cfg_list_c_& other);
  ~meas_gap_cfg_list_c_() { this->destroy_(); }
  void        set(types::options e = types::nulltype);
  types       type() const { return type_; }
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
  // getters
  meas_gap_cfg_list_normal_s& meas_gap_cfg_list_normal()
  {
    assert_choice_type(types::meas_gap_cfg_list_normal, type_, "MeasGapConfigList-Normal");
    return c.get<meas_gap_cfg_list_normal_s>();
  }
  meas_gap_cfg_list_dl_freq_spread_s& meas_gap_cfg_list_dl_freq_spread()
  {
    assert_choice_type(types::meas_gap_cfg_list_dl_freq_spread, type_, "MeasGapConfigList-DIFreqSpread");
    return c.get<meas_gap_cfg_list_dl_freq_spread_s>();
  }
  const meas_gap_cfg_list_normal_s& meas_gap_cfg_list_normal() const
  {
    assert_choice_type(types::meas_gap_cfg_list_normal, type_, "MeasGapConfigList-Normal");
    return c.get<meas_gap_cfg_list_normal_s>();
  }
  const meas_gap_cfg_list_dl_freq_spread_s& meas_gap_cfg_list_dl_freq_spread() const
  {
    assert_choice_type(types::meas_gap_cfg_list_dl_freq_spread, type_, "MeasGapConfigList-DIFreqSpread");
    return c.get<meas_gap_cfg_list_dl_freq_spread_s>();
  }
  meas_gap_cfg_list_normal_s&         set_meas_gap_cfg_list_normal();
  meas_gap_cfg_list_dl_freq_spread_s& set_meas_gap_cfg_list_dl_freq_spread_s();

private:
  types                                                                           type_;
  choice_buffer_t<meas_gap_cfg_list_dl_freq_spread_s, meas_gap_cfg_list_normal_s> c;

  void destroy_();
};

// RSSI-Normal
struct rssi_normal_s {
  uint16_t rssi_normal = 0;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
// RSSI-DlFreqSpread
struct rssi_dl_freq_spread_s {
  uint16_t    rssi_dl_freq_spread = 0;
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
// rssi_n_c_
struct rssi_n_c_ {
  struct types_opts {
    enum options { rssi_normal, rssi_dl_freq_spread, nulltype } value;
    typedef uint16_t number_type;

    const char* to_string() const;
    uint16_t    to_number() const;
  };
  typedef enumerated<types_opts> types;
  // choice methods
  rssi_n_c_() = default;
  rssi_n_c_(const rssi_n_c_& other);
  rssi_n_c_& operator=(const rssi_n_c_& other);
  ~rssi_n_c_() { this->destroy_(); }

  void  set(types::options e = types::nulltype);
  types type() const { return type_; }

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
  // getters
  rssi_normal_s& rssi_normal()
  {
    assert_choice_type(types::rssi_normal, type_, "RSSI-Normal");
    return c.get<rssi_normal_s>();
  }
  rssi_dl_freq_spread_s& rssi_dl_freq_spread()
  {
    assert_choice_type(types::rssi_dl_freq_spread, type_, "RSSI-Normal");
    return c.get<rssi_dl_freq_spread_s>();
  }
  const rssi_normal_s& rssi_normal() const
  {
    assert_choice_type(types::rssi_normal, type_, "RSSI-Normal");
    return c.get<rssi_normal_s>();
  }
  const rssi_dl_freq_spread_s& rssi_dl_freq_spread() const
  {
    assert_choice_type(types::rssi_dl_freq_spread, type_, "RSSI-Normal");
    return c.get<rssi_dl_freq_spread_s>();
  }
  rssi_normal_s&         set_rssi_normal();
  rssi_dl_freq_spread_s& set_rssi_dl_freq_spread();

private:
  types                                                 type_;
  choice_buffer_t<rssi_dl_freq_spread_s, rssi_normal_s> c;

  void destroy_();
};

// MeasConfig
struct meas_cofg_s {
  bool                 ext          = false;
  bool                 meas_gap_cfg_list_present=false;
  bool                 s_mea_sure_present = false;
  meas_gap_cfg_list_c_ meas_gap_cfg_list;
  report_cfg_s         report_cfg;
  rssi_n_c_            s_mea_sure;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// MobilityControlInfo::= SEQUENCE
struct mobility_contro_s {
  struct t304_opts {
    enum options { ms500, ms800, ms1000, ms8000, nulltype } value;
    typedef uint16_t number_type;
    const char*      to_string() const;
    uint16_t         to_number() const;
  };
  typedef enumerated<t304_opts> t304_e_;

  struct rach_indi_tor_opts {
    enum options { True, nulltype } value;

    const char* to_string() const;
    uint8_t     to_number() const;
  };
  typedef enumerated<rach_indi_tor_opts> rach_indi_tor_e_;
  bool                                   ext                   = false;
  bool                                   rach_indi_tor_present = false;
  uint8_t                                target_beam_index     = 1;
  beam_id_s                              target_beam_id;
  ba_id_s                                target_bcch_band_id;
  t304_e_                                t304;
  rach_indi_tor_e_                       rach_indi_tor;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// dedi_info_nas
struct dedi_info_nas_n_l_s {
  dyn_octstring dedi_info_nas_n1;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
using dedi_info_nas_n_s = dyn_array<dedi_info_nas_n_l_s>;

// RACH-ConfigCommon :: =			SEQUENCE
struct rach_cfg_com_s {
  struct rach_slot_ass_opts {
    enum options { halfFrame0, halfFrame1, both, nulltype } value;
    typedef uint8_t number_type;

    const char* to_string() const;
    uint8_t     to_number() const;
  };
  typedef enumerated<rach_slot_ass_opts> rach_slot_ass_e_;

  struct ra_res_win_size_opts {
    enum options { rf5, rf10, rf15, spare1, nulltype } value;
    typedef uint8_t number_type;

    const char* to_string() const;
    uint8_t     to_number() const;
  };
  typedef enumerated<ra_res_win_size_opts> ra_res_win_size_e_;

  bool               ext = false;
  band_id_s          band_is;
  fixed_bitstring<4> freq_bit_map;
  fixed_bitstring<4> rach_frame_ass;
  rach_slot_ass_e_   rach_slot_ass;
  ra_res_win_size_e_ ra_res_win_size;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// AGCH-ConfigCommon :: =			SEQUENCE
struct agch_cfg_com_n_s {
  struct agch_alot_start_opts {
    enum options { slot1, slot2, slot3, slot4, nulltype } value;
    typedef uint8_t number_type;

    const char* to_string() const;
    uint8_t     to_number() const;
  };
  typedef enumerated<agch_alot_start_opts> agch_slot_start_e_;
  bool                                     ext              = false;
  bool                                     band_id_present  = false;
  bool                                     freq_id_present  = false;
  bool                                     agch_slot_styart_present = false;
  band_id_s                                band_id;
  freq_id_n_s                              freq_id;
  fixed_bitstring<4>                       agch_frame_ass;
  agch_slot_start_e_                       agch_slot_start;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// P-Max::= INTEGER(0..15)
struct p_max_s {
  uint8_t p_max = 0;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// PowerControl-ConfigCommon :: =	SEQUENCE
struct power_control_cfg_com_s {
  bool               ext                          = false;
  bool               exp_rx_power_prach_present   = false;
  bool               exp_rx_power_psych_present   = false;
  bool               exp_rx_power_pdch1_1_present = false;
  bool               exp_rx_power_pdch1_2_present = false;
  bool               exp_rx_power_psch1_1_present = false;
  bool               exp_rx_power_psch1_2_present = false;
  bool               exp_rx_power_psch5_1_present = false;
  bool               exp_rx_power_psch5_2_present = false;
  bool               exp_rx_power_ptuch_present   = false;
  fixed_bitstring<7> pmbch_tx_power;
  rssi_n_c_          exp_rx_power_prach;
  rssi_n_c_          exp_rx_power_psych;
  rssi_n_c_          exp_rx_power_pdch1_1;
  rssi_n_c_          exp_rx_power_pdch1_2;
  rssi_n_c_          exp_rx_power_psch1_1;
  rssi_n_c_          exp_rx_power_psch1_2;
  rssi_n_c_          exp_rx_power_psch5_1;
  rssi_n_c_          exp_rx_power_psch5_2;
  rssi_n_c_          exp_rx_power_ptuch;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// EphemerisParameters::= SEQUENCE
//struct ephe_para_s {
//  bool     ext                        = false;
//  uint32_t sate_ephe_semi_major_axis  = 0;
//  uint8_t  sate_ephe_ecce_e           = 0;
//  uint16_t sate_ephe_argu_of_peri     = 0;
//  uint16_t sate_ephe_long_of_asc_node = 0;
//  uint16_t sate_ephe_inc_i            = 0;
//  uint16_t sate_ephe_mean_anoma_m     = 0;
//  uint16_t n_date                     = 1;
//  uint32_t n_time                     = 0;
//
//  SRSASN_CODE pack(bit_ref& bref) const;
//  SRSASN_CODE unpack(cbit_ref& bref);
//  void        to_json(json_writer& j) const;
//};

// RadioResourceConfigCommon  :: =	SEQUENCE
struct redio_resour_cfg_co_s {

  struct naviInfo_slot_ass_opts {
    enum options { slot1, slot2, slot3, slot4, nulltype } value;
    typedef uint8_t number_type;

    const char* to_string() const;
    uint8_t     to_number() const;
  };
  typedef enumerated<naviInfo_slot_ass_opts> naviInfo_slot_ass_e_;


  bool ext                      = false;
  bool rach_cfg_com_present     = false;
  bool agch_cfg_com_present     = false;
  bool p_max_present            = false;
  bool frame_offset_present     = false;
  bool band_id_present          = false;
  bool freq_id_present          = false;
  bool sec_syn_group_id_present = false;
  bool ephem_para_t_sat_present = false;
  bool mib_Re_Fra_num_present   = false;

  fixed_bitstring<2>      dis_to_beam_center;
  //rach_cfg_com_s          rach_cfg_com;
  rach_cfg_com_n_s        rach_cfg_com;
  agch_cfg_com_n_s        agch_cfg_com_n;
  p_max_s                 p_max;
  power_control_cfg_com_s power_control_cfg_com;
  fixed_bitstring<6>      frame_offset;
  band_id_s               bcch_band_id;
  freq_id_n_s             freq_id_n;
  uint8_t                 sec_syn_group_id = 1;
  ephemerise_param_s      ephe_para_t_sat;
  fixed_bitstring<6>      mib_Re_Fra_num;
  band_id_s               naviInfo_band_id;
  freq_id_s               naviInfo_fre_id;
  naviInfo_slot_ass_e_    naviInfo_slot_ass;
 
  SRSASN_CODE             pack(bit_ref& bref) const;
  SRSASN_CODE             unpack(cbit_ref& bref);
  void                    to_json(json_writer& j) const;
};

struct def_cfg_s {
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
// SRB-ToAdd :=	SEQUENCE
struct srb_to_addd_s {
  bool ext = false;

  def_cfg_s   def_cfg;
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// PDU-SessionID ::=                   INTEGER(0..255),
struct pdu_sess_id_s {
  uint16_t pdu_ses_id = 0;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// QFI::=                   INTEGER (0..maxQFI)mappedQoS-FlowsToAdd
struct map_qos_flows_to_add_l {
  uint8_t qfi = 0;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
using map_qos_flows_to_add_s = dyn_array<map_qos_flows_to_add_l>;

// QFI::=          INTEGER (0..maxQFI)
struct map_qos_flow_to_rel_l {
  uint8_t qfi = 0;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
using map_qos_flow_to_rel_s = dyn_array<map_qos_flow_to_rel_l>;

// SDAP-Config ::=           SEQUENCE
struct sdap_cfg_s {
  struct sdap_header_dl_opts {
    enum options { present, absent, nulltype } value;
    const char* to_string() const;
  };
  typedef enumerated<sdap_header_dl_opts> sdap_header_dl_e_;

  struct sdap_header_ul_opts {
    enum options { present, absent, nulltype } value;
    const char* to_string() const;
  };
  typedef enumerated<sdap_header_ul_opts> sdap_header_ul_e_;
  bool                                    ext                          = false;
  bool                                    map_qos_flows_to_add_present = false;
  bool                                    map_qos_flow_to_rel_present  = false;
  pdu_sess_id_s                           pdu_sess_id;
  sdap_header_dl_e_                       sdap_header_dl;
  sdap_header_ul_e_                       sdap_header_ul;
  bool                                    default_drb = false;
  map_qos_flows_to_add_s                  map_qos_flows_to_add;
  map_qos_flow_to_rel_s                   map_qos_flow_to_rel;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// rlc-AM             SEQUENCE
struct rlc_am_s {
  bool        stat_rep_req = false;
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
// rlc-UM                        SEQUENCE
struct rlc_um_s {

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
// rlc-TM     SEQUENCE
struct rlc_tm_s {

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// NULL
struct not_used_s {
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// profiles                        SEQUENCE
struct pro_file_s {
  bool pro_0x_0002 = false;
  bool pro_0x_0004 = false;
  bool pro_0x_0006 = false;
  bool pro_0x_0102 = false;
  bool pro_0x_0104 = false;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// rohc       SEQUENCE
struct ro_hc_s {
  bool       ext     = false;
  uint16_t   max_cid = 1;
  pro_file_s pro_file;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// headerCompression         CHOICE
struct header_com_c_ {
  struct types_opts {
    enum options { not_used_l, ro_hc, nulltype } value;
    typedef uint8_t unmber_type;

    const char* to_string() const;
    uint8_t     to_number() const;
  };
  typedef enumerated<types_opts> types;
  // choice methods
  header_com_c_() = default;
  header_com_c_(const header_com_c_& other);
  header_com_c_& operator=(const header_com_c_& other);
  ~header_com_c_() { this->destroy_(); }
  void  set(types::options e = types::nulltype);
  types type() const { return type_; };

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
  // getters
  not_used_s& not_used_l()
  {
    assert_choice_type(types ::not_used_l, type_, "not_used_l");
    return c.get<not_used_s>();
  }
  ro_hc_s& ro_hc()
  {
    assert_choice_type(types::ro_hc, type_, "ro_hc");
    return c.get<ro_hc_s>();
  }
  const not_used_s& not_used_l() const
  {
    assert_choice_type(types ::not_used_l, type_, "not_used_l");
    return c.get<not_used_s>();
  }
  const ro_hc_s& ro_hc() const
  {
    assert_choice_type(types::ro_hc, type_, "ro_hc");
    return c.get<ro_hc_s>();
  }
  not_used_s& set_not_used_l();
  ro_hc_s&    set_ro_hc();

private:
  types                                type_;
  choice_buffer_t<ro_hc_s, not_used_s> c;

  void destroy_();
};
// PDCP-Confg ::=        SEQUENCE
struct pdcp_cofg_s {
  struct dis_timer_opts {
    enum options { ms900, ms1200, ms1500, ms3000, ms5100, spare2, spare1, infinity, nulltype } value;
    const char* to_string() const;
    uint16_t    to_number() const;
  };
  typedef enumerated<dis_timer_opts> dis_timer_e_;

  struct integ_pro_opts {
    enum options { enabled, nulltype } value;
    const char* to_string() const;
  };
  typedef enumerated<integ_pro_opts> integ_pro_e_;

  struct ciph_dis_opts {
    enum options { True, nulltype } value;
    const char* to_string() const;
  };
  typedef enumerated<ciph_dis_opts> ciph_dis_e_;

  bool ext               = false;
  bool dis_timer_present = false;
  bool rlc_am_present    = false;
  bool rlc_um_present    = false;
  bool rlc_tm_present    = false;
  bool integ_pro_present = false;
  bool ciph_dis_present  = false;

  dis_timer_e_  dis_timer;
  rlc_am_s      rlc_am;
  rlc_um_s      rlc_um;
  rlc_tm_s      rlc_tm;
  header_com_c_ header_com;
  integ_pro_e_  integ_pro;
  ciph_dis_e_   ciph_dis;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// UL-AM-RLC ::=                      SEQUENCE
struct ul_am_rlcc_s {
  struct max_retx_thres_hold_opts {
    enum options { t1, t2, t4, t8, nulltype } value;

    typedef uint8_t number_type;

    const char* to_string() const;
    uint8_t     to_number() const;
  };
  typedef enumerated<max_retx_thres_hold_opts> max_retx_thres_hold_e_;

  struct t_poll_retran_opts {
    enum options { ms480, ms1200, ms2100, spare1, nulltype } value;
    typedef uint16_t number_type;

    const char* to_string() const;
    uint16_t    to_number() const;
  };
  typedef enumerated<t_poll_retran_opts> t_poll_retran_e_;

  struct poll_pdu_opts {
    enum options { p8, p16, p32, pInfinity, nulltype } value;
    typedef uint8_t number_type;

    const char* to_string() const;
    uint8_t     to_number() const;
  };
  typedef enumerated<poll_pdu_opts> poll_pdu_e_;

  struct poll_byte_opts {
    enum options { kB16, kB128, kB256, kBInfinity, nulltype } value;
    typedef uint16_t number_type;

    const char* to_string() const;
    uint16_t    to_number() const;
  };
  typedef enumerated<poll_byte_opts> poll_byte_e_;
  t_poll_retran_e_                   t_poll_retran;
  poll_pdu_e_                        poll_pdu;
  poll_byte_e_                       poll_byte;
  max_retx_thres_hold_e_             max_retx_thres_hold;
  SRSASN_CODE                        pack(bit_ref& bref) const;
  SRSASN_CODE                        unpack(cbit_ref& bref);
  void                               to_json(json_writer& j) const;
};

// DL-UM-RLC ::=                        SEQUENCE
struct dl_am_rlcc_s {
  struct t_reord_opts {
    enum options { ms480, ms1200, ms2100, spare1, nulltype } value;
    typedef uint16_t number_type;

    const char* to_string() const;
    uint16_t    to_number() const;
  };
  typedef enumerated<t_reord_opts> t_reord_e_;

  struct t_status_proh_opts {
    enum options { ms0, ms420, ms600, spare1, nulltype } value;
    typedef uint16_t number_type;

    const char* to_string() const;
    uint16_t    to_number() const;
  };
  typedef enumerated<t_status_proh_opts> t_status_proh_e_;

  t_reord_e_       t_reord;
  t_status_proh_e_ t_status_proh;
  SRSASN_CODE      pack(bit_ref& bref) const;
  SRSASN_CODE      unpack(cbit_ref& bref);
  void             to_json(json_writer& j) const;
};

// UL-UM-RLC ::=               SEQUENCE{
struct ul_um_rlcc_s {

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
// DL-UM-RLC ::=                        SEQUENCE
struct dl_um_rlcc_s {
  struct t_reord_opts {
    enum options { ms480, ms1200, ms2100, spare1, nulltype } value;
    typedef uint16_t number_type;

    const char* to_string() const;
    uint16_t    to_number() const;
  };
  typedef enumerated<t_reord_opts> t_reord_e_;

  t_reord_e_ t_reord;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
// RLC-Config ::=           CHOICE
struct am_s_ {
  ul_am_rlcc_s ul_am_rlc;
  dl_am_rlcc_s dl_am_rlc;
  SRSASN_CODE  pack(bit_ref& bref) const;
  SRSASN_CODE  unpack(cbit_ref& bref);
  void         to_json(json_writer& j) const;
};
struct um_bi_dir_s_ {
  ul_um_rlcc_s ul_um_rlc;
  dl_um_rlcc_s dl_um_rlc;
  SRSASN_CODE  pack(bit_ref& bref) const;
  SRSASN_CODE  unpack(cbit_ref& bref);
  void         to_json(json_writer& j) const;
};
struct tm_s {
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
struct rlc_cofg_c {
  struct types_opts {
    enum options { am, um_bi_dir, tm, nulltype } value;

    const char* to_string() const;
  };
  typedef enumerated<types_opts, true> types;

  // choice methods
  rlc_cofg_c() = default;
  rlc_cofg_c(const rlc_cofg_c& other);
  rlc_cofg_c& operator=(const rlc_cofg_c& other);
  ~rlc_cofg_c() { destroy_(); }
  void        set(types::options e = types::nulltype);
  types       type() const { return type_; }
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
  // getters
  am_s_& am()
  {
    assert_choice_type(types::am, type_, "RLC-Config");
    return c.get<am_s_>();
  }
  um_bi_dir_s_& um_bi_dir()
  {
    assert_choice_type(types::um_bi_dir, type_, "RLC-Config");
    return c.get<um_bi_dir_s_>();
  }
  tm_s& tm()
  {
    assert_choice_type(types::tm, type_, "RLC-Config");
    return c.get<tm_s>();
  }
  const am_s_& am() const
  {
    assert_choice_type(types::am, type_, "RLC-Config");
    return c.get<am_s_>();
  }
  const um_bi_dir_s_& um_bi_dir() const
  {
    assert_choice_type(types::um_bi_dir, type_, "RLC-Config");
    return c.get<um_bi_dir_s_>();
  }
  const tm_s& tm() const
  {
    assert_choice_type(types::tm, type_, "RLC-Config");
    return c.get<tm_s>();
  }
  am_s_&        set_am();
  um_bi_dir_s_& set_um_bi_dir();
  tm_s&         set_tm();

private:
  types                                      type_;
  choice_buffer_t<am_s_, um_bi_dir_s_, tm_s> c;

  void destroy_();
};

// ul-SpecificParameters                 SEQUENCE
struct ul_spec_para_s {
  struct prio_bit_rate_opts {
    enum options { kBps0, kBps2dot4, kBps16, kBps128, infinity, spare3, spare2, spare1, nulltype } value;
    typedef uint16_t number_type;

    const char* to_string() const;
    uint16_t    to_number() const;
  };
  typedef enumerated<prio_bit_rate_opts> prio_bit_rate_e_;

  struct buck_size_dura_opts {
    enum options { ms60, ms120, ms180, ms300, ms600, ms1200, spare2, spare1, nulltype } value;
    typedef uint16_t number_type;

    const char* to_string() const;
    uint16_t    to_number() const;
  };
  typedef enumerated<buck_size_dura_opts> buck_size_dura_e_;

  bool              log_chan_goup_present = false;
  uint8_t           priority              = 1;
  prio_bit_rate_e_  prio_bit_rate;
  buck_size_dura_e_ buck_size_dura;
  uint8_t           log_chan_goup = 0;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
// LogicalChannelConfig
struct log_chan_cfg_s {
  bool           ext                  = false;
  bool           ul_spec_para_present = false;
  ul_spec_para_s ul_spec_para;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
// DRB-ToAddMod :=	SEQUENCE
struct drb_to_add_modi_s {
  bool ext                  = false;
  bool sdap_cfg_present     = false;
  bool pdcp_cfg_present     = false;
  bool rlc_cfg_present      = false;
  bool log_chan_cfg_present = false;

  sdap_cfg_s     sdap_cfg;
  uint8_t        drb_id = 3;
  pdcp_cofg_s    pdcp_cfg;
  rlc_cofg_c     rlc_cfg;
  log_chan_cfg_s log_chan_cfg;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
using drb_to_add_modi_list_s = dyn_array<drb_to_add_modi_s>;

// DRB-ToReleseList
struct drb_id_s {
  uint8_t     drb_id = 3;
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
using drb_to_rel_list_s = dyn_array<drb_id_s>;

// s_rnti(bit6)
struct s_rnti_s {
  fixed_bitstring<6> srnti;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
// ChannelType:: = SEQUENCE
/*struct chan_type_s {

    bool ext = false;
    uint16_t pdtch_phy_code = 0;
    SRSASN_CODE pack(bit_ref& bref) const;
    SRSASN_CODE unpack(cbit_ref& bref);
    void to_json(json_writer& j) const;
};*/

// pdtch_code_s                pdtch_code;
struct pdtch_code_s_s {
  bool                  ext=0;
  uint16_t    pdt_phy_code = 0;
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
// PhysicalChannel-Config::= SEQUENCE
struct phy_chan_cfg_s {
  struct direct_opts {
    enum options { biDirection, ulDirection, dlDirection, nulltype } value;
    const char* to_string() const;
  };
  typedef enumerated<direct_opts> direct_e_;

  struct sche_type_opts {
    enum options { Static, dynamic, nulltype } value;
    const char* to_string() const;
  };
  typedef enumerated<sche_type_opts> sche_type_e_;

  struct voice_type_opts {
    enum options { kbps2point4, kbps4point8, bps800, nulltype } value;
    const char* to_string() const;
  };
  typedef enumerated<voice_type_opts> voice_type_e_;

  struct chan_type_opts {
    enum options {
      pSYCH,
      pDCH11,
      pDCH12,
      pSCH11,
      pSCH12,
      pSCH51,
      pSCH52,
      dSPDTCH1,
      dSPDTCH2,
      dSPDTCH3,
      dSPDTCHT,
      /*...*/
      nulltype
    } value;
    const char* to_string() const;
  };
  typedef enumerated<chan_type_opts,true> chan_type_e_;

  bool               ext                = false;
  bool               band_id_present    = false;
  bool               freq_id_present    = false;
  bool               slot_ass_present   = false;
  bool               pdtch_code_present = false;
  bool               sche_type_present  = false;
  bool               voice_type_present = false;
  s_rnti_s           s_rnti;
  chan_type_e_       chan_type;
  band_id_s          band_id;
  freq_id_n_s        freq_id;
  fixed_bitstring<5> slot_ass;
  direct_e_          direc_t;
  pdtch_code_s_s     pdtch_code;
  sche_type_e_       sche_type;
  voice_type_e_      voice_type;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
using phy_chan_list_cfg_s = dyn_array<phy_chan_cfg_s>;

// SecurityAlgorithmConfig ::=               SEQUENCE
struct sec_alg_cfg_s {
  struct ciph_alg_opts {
    enum options { nea0, nea1, nea2, nea3, nea4, nea5, nea6, nea7, nea8, nea9, nea10, nea11, nea12, nea13, nea14, nea15,/*...*/ nulltype } value;
    typedef uint8_t number_type;
    const char*     to_string() const;
    uint8_t         to_number() const;
  };
  typedef enumerated<ciph_alg_opts,true> ciph_alg_e_;

  struct intef_prot_alg_opts {
    enum options { nia0, nia1, nia2, nia3, nia4, nia5, nia6, nia7, nia8, nia9, nia10, nia11, nia12, nia13, nia14, nia15,/*...*/ nulltype } value;
    typedef uint8_t number_type;
    const char*     to_string() const;
    uint8_t         to_number() const;
  };
  typedef enumerated<intef_prot_alg_opts,true> intef_prot_alg_e_;

  bool              ext                    = false;
  bool              integ_prot_alg_present = false;
  ciph_alg_e_       coph_alg;
  intef_prot_alg_e_ intef_prot_alg;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
// securityPayload-Normal		OCTET STRING.
struct sec_pay_load_normal_s {
  dyn_octstring sec_pay_load_nor;
  SRSASN_CODE   pack(bit_ref& bref) const;
  SRSASN_CODE   unpack(cbit_ref& bref);
  void          to_json(json_writer& j) const;
};
// securityPayload-TtoT			OCTET STRING.
struct sec_pay_load_t_iot_s {
  dyn_octstring sec_pay_load_yiot;
  SRSASN_CODE   pack(bit_ref& bref) const;
  SRSASN_CODE   unpack(cbit_ref& bref);
  void          to_json(json_writer& j) const;
};
// securityPayload				CHOICE
struct sec_pay_load_c_ {
  struct types_opts {
    enum options { sec_pay_load_normal, sec_pay_load_t_iot, nulltype } value;
    typedef uint8_t unmber_type;

    const char* to_string() const;
    uint8_t     to_number() const;
  };
  typedef enumerated<types_opts> types;
  // choice methods
  sec_pay_load_c_() = default;
  sec_pay_load_c_(const sec_pay_load_c_& other);
  sec_pay_load_c_& operator=(const sec_pay_load_c_& other);
  ~sec_pay_load_c_() { this->destroy_(); }
  void  set(types::options e = types::nulltype);
  types type() const { return type_; };

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
  // getters
  sec_pay_load_normal_s& sec_pay_load_normal()
  {
    assert_choice_type(types::sec_pay_load_normal, type_, " sec_pay_load");
    return c.get<sec_pay_load_normal_s>();
  }
  sec_pay_load_t_iot_s& sec_pay_load_t_iot()
  {
    assert_choice_type(types::sec_pay_load_t_iot, type_, " sec_pay_load");
    return c.get<sec_pay_load_t_iot_s>();
  }
  const sec_pay_load_normal_s& sec_pay_load_normal() const
  {
    assert_choice_type(types::sec_pay_load_normal, type_, " sec_pay_load");
    return c.get<sec_pay_load_normal_s>();
  }
  const sec_pay_load_t_iot_s& sec_pay_load_t_iot() const
  {
    assert_choice_type(types::sec_pay_load_t_iot, type_, " sec_pay_load");
    return c.get<sec_pay_load_t_iot_s>();
  }
  sec_pay_load_normal_s& set_sec_pay_load_normal();
  sec_pay_load_t_iot_s&  set_sec_pay_load_t_iot();

private:
  types                                                        type_;
  choice_buffer_t<sec_pay_load_t_iot_s, sec_pay_load_normal_s> c;

  void destroy_();
};
// securityConfig				SEQUENCE
struct secu_cfg_s {
  bool            sec_pay_load_Present = false;
  sec_alg_cfg_s   sec_alg_cfg;
  sec_pay_load_c_ sec_pay_load;
  SRSASN_CODE     pack(bit_ref& bref) const;
  SRSASN_CODE     unpack(cbit_ref& bref);
  void            to_json(json_writer& j) const;
};

// phr-Config						CHOICE{
struct relea_s {
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
struct set_up_s {
  struct peri_phr_timer_opts {
    enum options { sf10, sf20, sf50, sf100, sf200, sf500, sf1000, infinity, nulltype } value;
    typedef uint16_t number_type;
    const char*      to_string() const;
    uint16_t         to_number() const;
  };
  typedef enumerated<peri_phr_timer_opts> peri_phr_timer_e_;

  struct prohi_phr_time_opts {
    enum options { sf0, sf10, sf20, sf50, sf100, Sf200, sf500, sf1000, nulltype } value;
    typedef uint16_t number_type;
    const char*      to_string() const;
    uint16_t         to_number() const;
  };
  typedef enumerated<prohi_phr_time_opts> prohi_phr_time_e_;

  struct dl_path_loss_change_opts {
    enum options { dB1, dB3, dB6, infinity, nulltype } value;
    typedef uint16_t number_type;
    const char*      to_string() const;
    uint16_t         to_number() const;
  };

  typedef enumerated<dl_path_loss_change_opts> dl_path_loss_change_e_;
  peri_phr_timer_e_                            peri_phr_timer;
  prohi_phr_time_e_                            prohi_phr_time;
  dl_path_loss_change_e_                       dl_path_loss_change;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
struct phr_cfg_c_ {
  struct types_opts {
    enum options { relea, set_up, nulltype } value;
    const char* to_string() const;
  };
  typedef enumerated<types_opts> types;

  // choice methods
  phr_cfg_c_() = default;
  phr_cfg_c_(const phr_cfg_c_& other);
  phr_cfg_c_& operator=(const phr_cfg_c_& other);
  ~phr_cfg_c_() { this->destroy_(); }
  void        set(types::options e = types::nulltype);
  types       type() const { return type_; }
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
  // getters
  relea_s& relea()
  {
    assert_choice_type(types::relea, type_, " phr_cfg");
    return c.get<relea_s>();
  }
  set_up_s& set_up()
  {
    assert_choice_type(types::set_up, type_, " phr_cfg");
    return c.get<set_up_s>();
  }
  const relea_s& relea() const
  {
    assert_choice_type(types::relea, type_, " phr_cfg");
    return c.get<relea_s>();
  }
  const set_up_s& set_up() const
  {
    assert_choice_type(types::set_up, type_, " phr_cfg");
    return c.get<set_up_s>();
  }
  relea_s&  set_relea();
  set_up_s& set_set_up();

private:
  types                              type_;
  choice_buffer_t<set_up_s, relea_s> c;

  void destroy_();
};
// RadioResourceConfigDedicated::=		SEQUENCE
struct redio_resour_cfg_dedi_s {
  struct peri_bsr_timer_opts {
    enum options { rf2, rf5, rf10, rf16, rf20, rf32, infinity, nulltype } value;
    typedef uint8_t numnber_type;
    const char*     to_string() const;
    uint8_t         to_number() const;
  };
  typedef enumerated<peri_bsr_timer_opts> peri_bsr_timer_e_;

  bool ext                         = false;
  bool srb_to_add_present          = false;
  bool drb_to_add_mod_list_present = false;
  bool drb_to_rel_list_present     = false;
  bool peri_bsr_timer_present      = false;
  bool phy_chan_list_cfg_present   = false;
  bool secu_cfg_present            = false;
  bool phr_cfg_present             = false;

  srb_to_addd_s          srb_to_add;
  drb_to_add_modi_list_s drb_to_add_mod_list;
  drb_to_rel_list_s      drb_to_rel_list;
  peri_bsr_timer_e_      peri_bsr_timer;
  phy_chan_list_cfg_s    phy_chan_list_cfg;
  secu_cfg_s             secu_cfg;
  phr_cfg_c_             phr_cfg;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// SecurityConfigHO::= SEQUENCE{
struct security_cofg_ho_s {
  bool          ext                 = false;
  bool          sec_alg_cfg_present = false;
  sec_alg_cfg_s sec_alg_cfg;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
// RRCConnectionReconfiguration-r1-IEs::= SEQUENCE
struct rrc_con_recfg_r1_ies_s {
  bool ext                           = false;
  bool meas_cfg_present              = false;
  bool mobility_contro_present       = false;
  bool dedi_info_nas_n_present       = false;
  bool redio_resour_cfg_com_present  = false;
  bool redio_resour_cfg_dedi_present = false;
  bool security_cfg_ho_present       = false;

  meas_cofg_s             meas_cfg;
  mobility_contro_s       mobility_contro;
  dedi_info_nas_n_s       dedi_info_nas_n;
  redio_resour_cfg_co_s   redio_resour_cfg_co;
  redio_resour_cfg_dedi_s redio_resour_cfg_dedi;
  security_cofg_ho_s      security_cfg_ho;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// RRCCoonnectionReconfiguration
struct rrc_con_recfg_s {
  uint8_t                rrc_tran_iden = 0;
  rrc_con_recfg_r1_ies_s rrc_con_recfg_r1;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// RedirectionInfo ::=                    SEQUENCE
struct redir_info_s {
  bool        ext        = false;
  uint8_t     beam_index = 1;
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
// RRCConnectionRelease-r1-IEs ::=          SEQUENCE{
struct rrc_con_rel_r1_ies_s {
  struct relea_cause_opts {
    enum options { loadBalancing, other, spare2, spare1, nulltype } value;
    const char* to_string() const;
  };
  typedef enumerated<relea_cause_opts> relea_cause_e_;
  bool                                 ext                = false;
  bool                                 redir_info_present = false;
  relea_cause_e_                       relea_cause;
  redir_info_s                         redir_info;
  SRSASN_CODE                          pack(bit_ref& bref) const;
  SRSASN_CODE                          unpack(cbit_ref& bref);
  void                                 to_json(json_writer& j) const;
};
// RRCConnectionRelease ::=               SEQUENCE{
struct rrc_con_release_s {
  rrc_con_rel_r1_ies_s rrc_con_rel_r1_ies;
  SRSASN_CODE          pack(bit_ref& bref) const;
  SRSASN_CODE          unpack(cbit_ref& bref);
  void                 to_json(json_writer& j) const;
};

struct rrc_tran_id_s {
  uint8_t     rrc_tran_id_t = 0;
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
struct sec_mode_com_r1_ies_s {
  bool          ext = false;
  sec_alg_cfg_s sec_alg_cfg;
  SRSASN_CODE   pack(bit_ref& bref) const;
  SRSASN_CODE   unpack(cbit_ref& bref);
  void          to_json(json_writer& j) const;
};
// SecurityModeConmmand::= SEQUENCE
struct sec_mode_com_s {
  rrc_tran_id_s         rrc_tran_id;
  sec_mode_com_r1_ies_s sec_mode_com_r1_ies;
  SRSASN_CODE           pack(bit_ref& bref) const;
  SRSASN_CODE           unpack(cbit_ref& bref);
  void                  to_json(json_writer& j) const;
};

struct ue_info_req_r1_ies_s {
  bool        ext           = false;
  bool        rach_repo_req = false;
  bool        rlf_repo_req  = false;
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
// UEInfomationRequest::= SEQUENCE{
struct ue_info_req_s {
  rrc_tran_id_s        rrc_tran_id;
  ue_info_req_r1_ies_s ue_info_req_r1_ies;
  SRSASN_CODE          pack(bit_ref& bref) const;
  SRSASN_CODE          unpack(cbit_ref& bref);
  void                 to_json(json_writer& j) const;
};
//// DL-DCCH-MessageType ::= CHOICE
struct s_dl_dcch_msg_type_c_ {
  struct types_opts {
    enum options { dl_info_tran, rrc_con_recfg, rrc_con_release, sec_mode_com, ue_info_req, nulltype } value;

    const char* to_string() const;
  };
  typedef enumerated<types_opts, true> types;
  // choice methods
  s_dl_dcch_msg_type_c_() = default;
  s_dl_dcch_msg_type_c_(const s_dl_dcch_msg_type_c_& other);
  s_dl_dcch_msg_type_c_& operator=(const s_dl_dcch_msg_type_c_& other);
  ~s_dl_dcch_msg_type_c_() { destroy_(); }

  void  set(types::options e = types::nulltype);
  types type() const { return type_; }

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
  // getters
  dl_info_tran_s& dl_info_tran()
  {
    assert_choice_type(types::dl_info_tran, type_, "s_dl_dcch_msg_type");
    return c.get<dl_info_tran_s>();
  }
  rrc_con_recfg_s& rrc_con_recfg()
  {
    assert_choice_type(types::rrc_con_recfg, type_, "s_dl_dcch_msg_type");
    return c.get<rrc_con_recfg_s>();
  }
  rrc_con_release_s& rrc_con_release()
  {
    assert_choice_type(types::rrc_con_release, type_, "s_dl_dcch_msg_type");
    return c.get<rrc_con_release_s>();
  }
  sec_mode_com_s& sec_mode_com()
  {
    assert_choice_type(types::sec_mode_com, type_, "s_dl_dcch_msg_type");
    return c.get<sec_mode_com_s>();
  }
  ue_info_req_s& ue_info_req()
  {
    assert_choice_type(types::ue_info_req, type_, "s_dl_dcch_msg_type");
    return c.get<ue_info_req_s>();
  }
  const dl_info_tran_s& dl_info_tran() const
  {
    assert_choice_type(types::dl_info_tran, type_, "s_dl_dcch_msg_type");
    return c.get<dl_info_tran_s>();
  }
  const rrc_con_recfg_s& rrc_con_recfg() const
  {
    assert_choice_type(types::rrc_con_recfg, type_, "s_dl_dcch_msg_type");
    return c.get<rrc_con_recfg_s>();
  }
  const rrc_con_release_s& rrc_con_release() const
  {
    assert_choice_type(types::rrc_con_release, type_, "s_dl_dcch_msg_type");
    return c.get<rrc_con_release_s>();
  }
  const sec_mode_com_s& sec_mode_com() const
  {
    assert_choice_type(types::sec_mode_com, type_, "s_dl_dcch_msg_type");
    return c.get<sec_mode_com_s>();
  }
  const ue_info_req_s& ue_info_req() const
  {
    assert_choice_type(types::ue_info_req, type_, "s_dl_dcch_msg_type");
    return c.get<ue_info_req_s>();
  }

  dl_info_tran_s&    set_dl_info_tran();
  rrc_con_recfg_s&   set_rrc_con_recfg();
  rrc_con_release_s& set_rrc_con_release();
  sec_mode_com_s&    set_sec_mode_com();
  ue_info_req_s&     set_ue_info_req();

private:
  types                                                                                              type_;
  choice_buffer_t<ue_info_req_s, sec_mode_com_s, rrc_con_release_s, rrc_con_recfg_s, dl_info_tran_s> c;

  void destroy_();
};
//// DL-DCCH-Message ::= SEQUENCE
struct s_dl_dcch_msg_s {
  s_dl_dcch_msg_type_c_ msg;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

} // namespace rrc
} // namespace asn1

#endif // SRSASN1_RRC_DLDCCH_MSG_H
