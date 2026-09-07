/*******************************************************************************
 *
 * 
 *                  ASN.1 IoT Message TBCCH-Message (2023.08.08)
 *							   	    
 *
 ******************************************************************************/

#ifndef SRSASN1_IoT_TBCCH_MSG_H
#define SRSASN1_IoT_TBCCH_MSG_H

#include "srsran/asn1/asn1_utils.h"
#include "rr_common.h"
#include "mib_sib_asn1.h"
#include <cstdio>
#include <stdarg.h>

namespace asn1 {
namespace rrc {
/*******************************************************************************
 *                              Struct Definitions
 ******************************************************************************/
//IoTRachFreq::= SEQUENCE
struct iot_rach_freq_s{
  // member variables
  freq_id_s          ul_freq_id;
  fixed_bitstring<4> ul_sub_freq_bit_map;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};
 
//IoTRachFreqList::= SEQUENCE(SIZE(1..maxFreqAmount))OF IoTRachFreq
using iot_rach_freq_list_s = dyn_array<iot_rach_freq_s>;

//IoTUAC-BarringPerCat::= SEQUENCE
struct iot_uac_barring_per_cat_s {
  struct access_category_opts {
    enum options { acCatNum3, acCatNum7, acCatNum8, spare, nulltype } value;

    const char* to_string() const;
    uint8_t     to_number() const;
  };
  typedef enumerated<access_category_opts> access_category_e_;

  // member variables
  access_category_e_        access_category;
  fixed_bitstring<5>        uac_barring_for_access_id;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

//IoTUAC-BaringList::= SEQUENCE(SIZE(1..maxIoTAccessCat-1))OF IoTUAC-BarringPerCat
using iot_uac_barring_list_s = dyn_array<iot_uac_barring_per_cat_s>;

//IoTRachConfig::= SEQUENCE
struct iot_rach_config_s {
  struct ra_response_windowsize_opts {
    enum options { rf10, rf20, rf30, spare, nulltype } value;

    const char* to_string() const;
    uint8_t     to_number() const;
  };
  typedef enumerated<ra_response_windowsize_opts> ra_response_windowsize_e_;

  // member variables
  band_id_s                 dl_band_id;
  iot_rach_freq_list_s      iot_rach_freq_list;
  fixed_bitstring<8>        ul_fn_assignment;
  ra_response_windowsize_e_ ra_response_windowsize;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

//IoTAgchConfig-Normal::= SEQUENCE
struct iot_agch_conf_normal_s {
  // member variables
  band_id_s          dl_band_id;
  freq_id_s          dl_freq_id;
  fixed_bitstring<8> ul_fn_assignment;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

//IoTAgchConfig-DlFreqSpread::= SEQUENCE
struct iot_agch_conf_dl_freq_spread_s {
  struct spread_factor_opts {
    enum options { sf128, sf256, sf512, spare, nulltype } value;
    typedef uint16_t number_type;

    const char* to_string() const;
    uint16_t     to_number() const;
  };
  typedef enumerated<spread_factor_opts> spread_factor_e_;

  // member variables
  spread_factor_e_     spread_factor;
  fixed_bitstring<9>   code_index;
  fixed_bitstring<8>   dl_fn_assignment;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

//IoTUeTimer::= SEQUENCE
struct iot_ue_timer_s {
  struct iot_t300_opts {
    enum options { ms2000, ms5000, ms10000, ms20000, nulltype } value;

    const char* to_string() const;
    uint16_t     to_number() const;
  };
  typedef enumerated<iot_t300_opts> iot_t300_e_;

  struct iot_t301_opts {
    enum options { ms2000, ms5000, ms10000, ms20000, nulltype } value;

    const char* to_string() const;
    uint16_t     to_number() const;
  };
  typedef enumerated<iot_t301_opts> iot_t301_e_;

  struct iot_t302_opts {
    enum options { ms200, ms500, ms1000, ms2000, nulltype } value;

    const char* to_string() const;
    uint16_t     to_number() const;
  };
  typedef enumerated<iot_t302_opts> iot_t302_e_;

  // member variables
  bool        ext      = false;
  iot_t300_e_ iot_t300;
  iot_t301_e_ iot_t301;
  iot_t302_e_ iot_t302;
  // ...
  
  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

//IoTSystemInformation::= SEQUENCE
struct iot_sib_s {
  struct iot_agch_config_c_ {
    struct types_opts {
      enum options { iot_agch_conf_normal, iot_agch_conf_dl_freq_spread, nulltype} value;
      const char* to_string() const;
    };
    typedef enumerated<types_opts> types;

    // choice methods
    iot_agch_config_c_() = default;
    iot_agch_config_c_(const iot_agch_config_c_& other);
    iot_agch_config_c_& operator=(const iot_agch_config_c_& other);
    ~iot_agch_config_c_() { this->destroy_(); }
    void        set(types::options e = types::nulltype);
    types       type() const { return type_; }
    SRSASN_CODE pack(bit_ref& bref) const;
    SRSASN_CODE unpack(cbit_ref& bref);
    void        to_json(json_writer& j) const;

    // getters
    iot_agch_conf_normal_s& iot_agch_conf_normal()
    {
      assert_choice_type(types::iot_agch_conf_normal, type_, "iot_agch_config_c_");
      return c.get<iot_agch_conf_normal_s>();
    }
    iot_agch_conf_dl_freq_spread_s& iot_agch_conf_dl_freq_spread()
    {
      assert_choice_type(types::iot_agch_conf_dl_freq_spread, type_, "iot_agch_config_c_");
      return c.get<iot_agch_conf_dl_freq_spread_s>();
    }
    const iot_agch_conf_normal_s& iot_agch_conf_normal() const
    {
      assert_choice_type(types::iot_agch_conf_normal, type_, "iot_agch_config_c_");
      return c.get<iot_agch_conf_normal_s>();
    }
    const iot_agch_conf_dl_freq_spread_s& iot_agch_conf_dl_freq_spread() const
    {
      assert_choice_type(types::iot_agch_conf_dl_freq_spread, type_, "iot_agch_config_c_");
      return c.get<iot_agch_conf_dl_freq_spread_s>();
    }
    iot_agch_conf_normal_s&         set_iot_agch_conf_normal();
    iot_agch_conf_dl_freq_spread_s& set_iot_agch_conf_dl_freq_spread();

  private:
    types                                                               type_;
    choice_buffer_t<iot_agch_conf_normal_s, iot_agch_conf_dl_freq_spread_s> c;

    void destroy_();
  };
    
  // member variables
  bool                      ext                             = false;
  bool                      iot_uac_barring_list_present    = false;
  iot_uac_barring_list_s    iot_uac_barring_list;
  fixed_bitstring<3>        iot_dl_trans_coies;
  iot_rach_config_s         iot_rach_config;
  iot_agch_config_c_        iot_agch_config;
  iot_ue_timer_s            iot_ue_timer;
  // ...

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};


//TBCCH-MessageType::= CHOICE
struct tbcch_msg_type_c {
  struct types_opts {
    enum options { iot_sib, nulltype } value;

    const char* to_string() const;
  };
  typedef enumerated<types_opts> types;

  // choice methods
  types       type() const { return types::iot_sib; }
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;

  // getters
  iot_sib_s& iot_sib(){ return c; }
  const iot_sib_s& ioT_sib() const{ return c; }

private:
  iot_sib_s c;
};

//TBCCH-Message::= SEQUENCE
struct tbcch_msg_s {

  // member variables
  tbcch_msg_type_c   msg;

  // member methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};


}
} // namespace asn1
#endif // SRSASN1_IoT_TBCCH_MSG_H
