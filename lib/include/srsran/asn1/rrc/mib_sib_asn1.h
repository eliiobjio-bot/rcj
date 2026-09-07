/*******************************************************************************
 *
 *                           Satellite MIB and SIB
 *
 ******************************************************************************/

#ifndef SRSASN1_RRC_MIB_SIB_ASN1_H
#define SRSASN1_RRC_MIB_SIB_ASN1_H

#include "rr_common.h"

namespace asn1 {
namespace rrc {
/*******************************************************************************
 *                              Struct Definitions
 ******************************************************************************/
// BeamIdentity ::=SEQUENCE
struct beam_id_s {
  fixed_bitstring<14> beam_id;
  SRSASN_CODE         pack(bit_ref& bref) const;
  SRSASN_CODE         unpack(cbit_ref& bref);
  void                to_json(json_writer& j) const;
};
// FrequencyIdentity ::= SEQUENCE
struct freq_id_s {
  fixed_bitstring<2> freq_id;
  SRSASN_CODE        pack(bit_ref& bref) const;
  SRSASN_CODE        unpack(cbit_ref& bref);
  void               to_json(json_writer& bref) const;
};
// BandIdentity ::= SEQUENCE
struct band_id_s {
  fixed_bitstring<6> ba_id;
  SRSASN_CODE        pack(bit_ref& bref) const;
  SRSASN_CODE        unpack(cbit_ref& bref);
  void               to_json(json_writer& bref) const;
};
// Q-RxLevMin ::= SEQUENCE
struct q_rx_lev_min_s {
  int8_t      q_min = -70;
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& bref) const;
};

// RSSI ::= SEQUENCE
struct rssi_c {
  struct types_opts {
    enum options { rssi_normal, rssi_dl_freq_sp, nulltype } value;


    const char* to_string() const;
    uint8_t     to_number() const;
  };
  typedef enumerated<types_opts> types;
  // choice methods
  rssi_c() = default;
  rssi_c(const rssi_c& other);
  rssi_c& operator=(const rssi_c& other);
  ~rssi_c() { this->destroy_(); }
  void  set(types::options e = types::nulltype);
  types type() const { return type_; };

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
  bool        operator==(const rssi_c& other) const;
  bool        operator!=(const rssi_c& other) const { return not(*this == other); }
  // getters
  uint8_t& rssi_normal()
  {
    assert_choice_type(types ::rssi_normal, type_, "rssi_c");
    return c.get<uint8_t>();
  }
  uint8_t& rssi_dl_freq_sp()
  {
    assert_choice_type(types::rssi_dl_freq_sp, type_, "rssi_c");
    return c.get<uint8_t>();
  }
  const uint8_t& rssi_normal() const
  {
    assert_choice_type(types ::rssi_normal, type_, "rssi_c");
    return c.get<uint8_t>();
  }
  const uint8_t& rssi_dl_freq_sp() const
  {
    assert_choice_type(types::rssi_dl_freq_sp, type_, "rssi_c");
    return c.get<uint8_t>();
  }
  uint8_t& set_rssi_normal();
  uint8_t& set_rssi_dl_freq_sp();

private:
  types               type_;
  pod_choice_buffer_t c;

  void destroy_();
};

struct uac_barr_per_cat_s {
  bool    ext                   = false;
  uint8_t access_category       = 1;
  uint8_t uac_barr_info_set_idx = 1;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
using uac_barr_per_cat_list_l = dyn_array<uac_barr_per_cat_s>;

struct uac_barr_info_set_s {
  struct uac_barr_factor_opts {
    enum options { p00, p20, p50, p70, p90, p95, p100, nulltype } value;
    typedef float number_type;

    const char* to_string() const;
    float       to_number() const;
  };
  typedef enumerated<uac_barr_factor_opts> uac_barr_factor_e_;

  struct uac_barr_time_opts {
    enum options { s8, s32, s64, s128, s256, nulltype } value;
    typedef uint16_t number_type;

    const char* to_string() const;
    uint16_t    to_number() const;
  };
  typedef enumerated<uac_barr_time_opts> uac_barr_time_e_;

  bool               ext = false;
  uac_barr_factor_e_ uac_barr_factor;
  uac_barr_time_e_   uac_barr_time;
  fixed_bitstring<7> uac_barr_for_access_id;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
using uac_barr_info_set_list_l = dyn_array<uac_barr_info_set_s>;

struct uac_barr_info_s {
  bool                     uac_barr_for_common_present = false;
  uac_barr_per_cat_list_l  uac_barr_for_common;
  uac_barr_info_set_list_l uac_barr_info_set_list;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// RACH-ConfigCommon ::= SEQUENCE
struct rach_cfg_com_n_s {
  struct rach_slot_ass_opts {
    enum options { halfFrame0, halfFrame1, both, nulltype } value;
    typedef int8_t number_type;

    const char* to_string() const;
    int8_t      to_number() const;
  };
  typedef enumerated<rach_slot_ass_opts> rach_slot_ass_e_;

  struct ra_res_wi_si_opts {
    enum options { rf5, rf10, rf15, spare1, nulltype } value;
    typedef int8_t number_type;

    const char* to_string() const;
    int8_t      to_number() const;
  };
  typedef enumerated<ra_res_wi_si_opts> ra_res_wi_si_e_;

  bool               ext = false;
  bool               freqBitmap_FrameFull_present = false;
  band_id_s          band_id;
  fixed_bitstring<4> freq_bit_map;
  fixed_bitstring<4> rach_frame_ass;
  rach_slot_ass_e_   rach_slot_ass;
  ra_res_wi_si_e_    ra_res_wi_si;
  fixed_bitstring<4> freqBitmap_FrameFull;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// AGCH-ConfigCommon ::= SEQUENCE
struct agch_cfg_com_s {
  struct agch_slot_start_opts {
    enum options { slot1, slot2, slot3, slot4, nulltype } value;
    typedef int8_t number_type;

    const char* to_string() const;
    int8_t      to_number() const;
  };
  typedef enumerated<agch_slot_start_opts> agch_slot_start_e_;
  bool                                     ext                     = false;
  bool                                     band_id_present         = false;
  bool                                     freq_id_present         = false;
  bool                                     agch_slot_start_present = false;
  band_id_s                                band_id;
  freq_id_s                                freq_id;
  fixed_bitstring<4>                       agch_fram_ass;
  agch_slot_start_e_                       agch_slot_start;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// POWER-ConfigCommon ::= SEQUENCE
struct power_ctrl_cfg_com_s {
  bool ext                      = false;
  bool exp_rx_p_prach_present   = false;
  bool exp_rx_p_psych_present   = false;
  bool exp_rx_p_pdch1_1_present = false;
  bool exp_rx_p_pdch1_2_present = false;
  bool exp_rx_p_psch1_1_present = false;
  bool exp_rx_p_psch1_2_present = false;
  bool exp_rx_p_psch5_1_present = false;
  bool exp_rx_p_psch5_2_present = false;
  bool exp_rx_p_ptuch_present   = false;

  fixed_bitstring<7> pmbch_tx_p;
  rssi_c             exp_rx_p_prach;
  rssi_c             exp_rx_p_psych;
  rssi_c             exp_rx_p_pdch1_1;
  rssi_c             exp_rx_p_pdch1_2;
  rssi_c             exp_rx_p_psch1_1;
  rssi_c             exp_rx_p_psch1_2;
  rssi_c             exp_rx_p_psch5_1;
  rssi_c             exp_rx_p_psch5_2;
  rssi_c             exp_rx_p_ptuch;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// BBCH-ConfigCommon ::= SEQUENCE
struct bbch_cfg_com_s {
  struct bbch_slot_ass_opts {
    enum options { slot1, slot2, slot3, slot4, nulltype } value;
    typedef int8_t unmber_type;

    const char* to_string() const;
    int8_t      to_number() const;
  };
  typedef enumerated<bbch_slot_ass_opts> bbch_slot_ass_e_;

  bool               ext = false;
  band_id_s          band_id;
  freq_id_s          freq_id;
  fixed_bitstring<4> bbch_frame_ass;
  bbch_slot_ass_e_   bbch_slot_ass;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// IoTSI-Config-Normal ::= SEQUENCE
struct iotsi_cfg_normal_s {
  band_id_s          dl_band_id;
  freq_id_s          dl_freq_id;
  fixed_bitstring<6> fn_ass;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& bref) const;
};

// IoSTI-Config-DIFreqSpread  ::= SEQUENCE
struct iotsi_cfg_dl_freq_spread_s {
  struct spread_factor_opts {
    enum options { sf128, sf256, sf512, spare, nulltype } value;
    typedef int16_t number_type;

    const char* to_string() const;
    int16_t     to_number() const;
  };
  typedef enumerated<spread_factor_opts> spread_factor_e_;

  spread_factor_e_   spread_factor;
  fixed_bitstring<9> code_index;
  fixed_bitstring<6> fn_ass;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
// IoTSI-Config ::= CHOICE
struct iotsi_cfg_c {
  struct types_opts {
    enum options { iotsi_cfg_normal, iotsi_cfg_dl_freq_spread, nulltype } value;
    typedef uint8_t unmber_type;

    const char* to_string() const;
    uint8_t     to_number() const;
  };
  typedef enumerated<types_opts> types;
  // choice methods
  iotsi_cfg_c() = default;
  iotsi_cfg_c(const iotsi_cfg_c& other);
  iotsi_cfg_c& operator=(const iotsi_cfg_c& other);
  ~iotsi_cfg_c() { this->destroy_(); }
  void  set(types::options e = types::nulltype);
  types type() const { return type_; };

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
  // getters
  iotsi_cfg_normal_s& iotsi_cfg_normal()
  {
    assert_choice_type(types ::iotsi_cfg_normal, type_, "iotsi_cfg");
    return c.get<iotsi_cfg_normal_s>();
  }
  iotsi_cfg_dl_freq_spread_s& iotsi_cfg_dl_freq_spread()
  {
    assert_choice_type(types::iotsi_cfg_dl_freq_spread, type_, "iotsi_cfg");
    return c.get<iotsi_cfg_dl_freq_spread_s>();
  }
  const iotsi_cfg_normal_s& iotsi_cfg_normal() const
  {
    assert_choice_type(types ::iotsi_cfg_normal, type_, "iotsi_cfg");
    return c.get<iotsi_cfg_normal_s>();
  }
  const iotsi_cfg_dl_freq_spread_s& iotsi_cfg_dl_freq_spread() const
  {
    assert_choice_type(types::iotsi_cfg_dl_freq_spread, type_, "iotsi_cfg");
    return c.get<iotsi_cfg_dl_freq_spread_s>();
  }
  iotsi_cfg_normal_s&         set_iotsi_cfg_normal();
  iotsi_cfg_dl_freq_spread_s& set_iotsi_cfg_dl_freq_spread();

private:
  types                                                           type_;
  choice_buffer_t<iotsi_cfg_dl_freq_spread_s, iotsi_cfg_normal_s> c;

  void destroy_();
};

struct rr_cfg_com_sib_s {
  bool                 ext                  = false;
  bool                 bbch_cfg_com_present = false;
  bool                 iotsi_cfg_present    = false;
  rach_cfg_com_n_s     rach_cfg_com;
  agch_cfg_com_s       agch_cfg_com;
  power_ctrl_cfg_com_s power_ctrl_cfg_com;
  bbch_cfg_com_s       bbch_cfg_com;
  iotsi_cfg_c          iotsi_cfg;
  
  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// UE-TimersAndConstants
struct ue_timer_and_constant_s {
  struct t300_opts {
    enum options { ms1000, ms2000, ms3000, ms5000, ms8000, ms10000, ms15000, ms20000, nulltype } value;
    typedef int16_t number_type;

    const char* to_string() const;
    int16_t     to_number() const;
  };
  typedef enumerated<t300_opts> t300_e_;

  struct t301_opts {
    enum options { ms100, ms200, ms300, ms400, ms600, ms1000, ms1500, ms2000, nulltype } value;
    typedef int16_t number_type;

    const char* to_string() const;
    int16_t     to_number() const;
  };
  typedef enumerated<t301_opts> t301_e_;

  struct t302_opts {
    enum options { ms100, ms200, ms300, ms400, ms600, ms1000, ms1500, ms2000, nulltype } value;
    typedef int16_t number_type;

    const char* to_string() const;
    int16_t     to_number() const;
  };
  typedef enumerated<t302_opts> t302_e_;

  struct t310_opts {
    enum options { ms0, ms50, ms100, ms200, ms500, ms1000, ms2000, nulltype } value;
    typedef int16_t number_type;

    const char* to_string() const;
    int16_t     to_number() const;
  };
  typedef enumerated<t310_opts> t310_e_;

  struct n310_opts {
    enum options { n1, n2, n3, n4, n6, n8, n10, n20, nulltype } value;
    typedef uint8_t number_type;

    const char* to_string() const;
    uint8_t     to_number() const;
  };
  typedef enumerated<n310_opts> n310_e_;

  struct t311_opts {
    enum options { ms1000, ms3000, ms5000, ms10000, ms15000, ms20000, ms30000, spare1, nulltype } value;
    typedef uint16_t number_type;

    const char* to_string() const;
    uint16_t    to_number() const;
  };
  typedef enumerated<t311_opts> t311_e_;

  struct n311_opts {
    enum options { n1, n2, n3, n4, n5, n6, n8, n10, nulltype } value;
    typedef uint8_t number_type;

    const char* to_string() const;
    uint8_t     to_number() const;
  };
  typedef enumerated<n311_opts> n311_e_;

  bool    ext = false;
  t300_e_ t300;
  t301_e_ t301;
  t302_e_ t302;
  t310_e_ t310;
  n310_e_ n310;
  t311_e_ t311;
  n311_e_ n311;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

struct geo_info_update_para_s {
  struct update_timer_opts {
    enum options { min10, min30, min60, infinity, nulltype } value;
    typedef int8_t number_type;

    const char* to_string() const;
    int8_t      to_number() const;
  };
  typedef enumerated<update_timer_opts> update_timer_e_;

  struct update_distance_opts {
    enum options { km3, km10, km50, infinity, nulltype } value;
    typedef int8_t number_type;

    const char* to_string() const;
    int8_t      to_number() const;
  };
  typedef enumerated<update_distance_opts> update_distance_e_;

  bool               ext = false;
  update_timer_e_    update_timer;
  update_distance_e_ update_distance;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// member variables循环
using plmn_id_li_l = dyn_array<plmn_id_s>;

// MeasGapConfig-Normal ::= SEQUENCE
struct cfg_nom_s {
  struct meas_nom_ratio_opts {
    enum options { dB5, dB6, dB7, dB8, dB9, dB10, dB11, dB12, dB13, dB14, dB15, dB16, nulltype } value;
    typedef uint8_t number_type;

    const char* to_string() const;
    uint8_t     to_number() const;
  };
  typedef enumerated<meas_nom_ratio_opts> meas_nom_ratio_e_;

  uint8_t           beam_index   = 1;
  uint8_t           frame_offset = 0;
  band_id_s         fcch_band_id;
  freq_id_s         fcch_freq_id;
  band_id_s         pcch_band_id;
  freq_id_s         pcch_fre_id;
  meas_nom_ratio_e_ meas_nom_ratio;
  uint8_t           mib_re_fra_num = 0;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
using cfg_li_nom_s = dyn_array<cfg_nom_s>;

// MeasGapConfig-DlFreqSpread ::= SEQUENC
struct cfg_dl_fr_sp_s {
  uint8_t beam_index       = 1;
  uint8_t sec_syn_group_id = 1;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
using cfg_li_dl_fr_sp_s = dyn_array<cfg_dl_fr_sp_s>;





struct ephemerise_param_s {
  bool     ext                               = false;
  uint16_t toe                               = 0;
  uint8_t  fitArc_Len                        = 0;
  int32_t  sate_ephem_semi_major_axis_off    = -4194304;
  uint32_t sate_ephem_eccen_e                = 0;
  int32_t  sate_ephem_inc_off_i                = -67108864;
  int32_t  sate_ephem_rate_right_node        = -4194304;
  int64_t  sate_ephem_argu_of_peria          = -2147483648;
  int64_t  sate_ephem_long_of_ascen_node     = -2147483648;
  int64_t  sate_ephem_mean_ano_m             = -2147483648;
  //uint16_t n_date                            = 1;
  //uint32_t n_time                            = 0;

  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};



// MIB主消息
struct mib_wx_s {
  struct bcch_slot_start_opts {
    enum options { slot1, slot2, slot3, slot4, nulltype } value;
    typedef uint8_t number_type;
    const char*     to_string() const;
    uint8_t         to_number() const;
  };
  typedef enumerated<bcch_slot_start_opts> bcch_slot_start_e_;

  struct meas_normal_ra_opts {
    enum options { dB5, dB6, dB7, dB8, dB9, dB10, dB11, dB12, dB13, dB14, dB15, dB16, nulltype } value;
    typedef uint8_t number_type;

    const char* to_string() const;
    uint8_t     to_number() const;
  };
  typedef enumerated<meas_normal_ra_opts> meas_normal_ra_e_;

  struct naviInfo_slot_ass_opts {
    enum options { slot1, slot2, slot3, slot4, nulltype } value;
    typedef uint8_t number_type;

    const char* to_string() const;
    uint8_t     to_number() const;
  };
  typedef enumerated<naviInfo_slot_ass_opts> naviInfo_slot_ass_e_;

  bool                 hot_info_indication = false;
  beam_id_s            beam_id;
  band_id_s            bcch_band_id;
  freq_id_s            bcch_fre_id;
  bcch_slot_start_e_   bcch_slot_start;
  fixed_bitstring<6>   frame_off;
  fixed_bitstring<2>   sys_info_ver_tag;
  fixed_bitstring<13>  sys_sh_fra_num;
  fixed_bitstring<2>   d_to_beam_center;
  meas_normal_ra_e_    meas_normal_ra;
  band_id_s            pcch_band_id;
  freq_id_s            pcch_fre_id;
  band_id_s            naviInfo_band_id;
  freq_id_s            naviInfo_fre_id;
  naviInfo_slot_ass_e_ naviInfo_slot_ass;
  fixed_bitstring<6>   mib_Re_Fra_num;
  fixed_bitstring<4>   spare;

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// SIB主消息；
struct sib_wx_s {
  // beamAccessRelatedInfo
  struct beam_acc_rel_info_s {
    struct beam_barred_opts {
        enum options { barred, not_barred, nulltype } value;

        const char* to_string() const;
    };
    typedef enumerated<beam_barred_opts> beam_barred_e_;
    plmn_id_li_l                         plmn_id_li;
    beam_barred_e_                       beam_barred;
  };

  // beamSelectionRelatedInfo
  struct beam_sel_rel_info_s {
    bool           ext = false;
    q_rx_lev_min_s q_rx_lev_min;
  };
  // beamReselectionRelatedInfo
  struct beam_resel_rel_info_s {
    struct meas_gap_cfg_wx_c_ {
        struct types_opts {
          enum options { cfg_li_nom, cfg_li_dl_fr_sp, nulltype } value;
          const char* to_string() const;
        };
        typedef enumerated<types_opts> types;

        // choice methods
        meas_gap_cfg_wx_c_() = default;
        meas_gap_cfg_wx_c_(const meas_gap_cfg_wx_c_& other);
        meas_gap_cfg_wx_c_& operator=(const meas_gap_cfg_wx_c_& other);
        ~meas_gap_cfg_wx_c_() { this->destroy_(); }
        void        set(types::options e = types::nulltype);
        types       type() const { return type_; }
        SRSASN_CODE pack(bit_ref& bref) const;
        SRSASN_CODE unpack(cbit_ref& bref);
        void        to_json(json_writer& j) const;
        // getters
        cfg_li_nom_s& cfg_li_nom()
        {
          assert_choice_type(types::cfg_li_nom, type_, "meas_gap_cfg_wx");
          return c.get<cfg_li_nom_s>();
        }
        cfg_li_dl_fr_sp_s& cfg_li_dl_fr_sp()
        {
          assert_choice_type(types::cfg_li_dl_fr_sp, type_, "meas_gap_cfg_wx");
          return c.get<cfg_li_dl_fr_sp_s>();
        }
        const cfg_li_nom_s& cfg_li_nom() const
        {
          assert_choice_type(types::cfg_li_nom, type_, "meas_gap_cfg_wx");
          return c.get<cfg_li_nom_s>();
        }
        const cfg_li_dl_fr_sp_s& cfg_li_dl_fr_sp() const
        {
          assert_choice_type(types::cfg_li_dl_fr_sp, type_, "meas_gap_cfg_wx");
          return c.get<cfg_li_dl_fr_sp_s>();
        }
        cfg_li_nom_s&      set_cfg_li_nom();
        cfg_li_dl_fr_sp_s& set_cfg_li_dl_fr_sp();

      private:
        types                                            type_;
        choice_buffer_t<cfg_li_dl_fr_sp_s, cfg_li_nom_s> c;

        void destroy_();
    };
    rssi_c             threshold_resel;
    q_rx_lev_min_s     q_rx_lev_min;
    int8_t             offset     = -9;
    uint8_t            hysteresis = 0;
    uint8_t            t_resel    = 0;
    meas_gap_cfg_wx_c_ meas_gap_cfg_wx;
  };
  bool                    ext                      = false;
  bool                    uac_barring_info_present = false;
  bool                    p_max_n_present          = false;
  beam_acc_rel_info_s     beam_acc_rel_info;
  beam_sel_rel_info_s     beam_sel_rel_info;
  beam_resel_rel_info_s   beam_resel_rel_info;
  uac_barr_info_s         uac_barring_info;
  rr_cfg_com_sib_s        rr_cfg_com;
  ue_timer_and_constant_s ue_timer_and_constant;
  uint8_t                 p_max_n = 0;
  geo_info_update_para_s  geo_info_update_para;
  ephemerise_param_s      eph_para_ser_sat;
  
  
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
} // namespace rrc
} // namespace asn1

#endif // SRSASN1_RRC_SI_H
