/**
 * Copyright 2013-2021 Software Radio Systems Limited
 *
 * This file is part of srsRAN.
 *
 * srsRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#ifndef SRSRAN_RLC_INTERFACE_TYPES_H
#define SRSRAN_RLC_INTERFACE_TYPES_H

#include "srsran/interfaces/rrc_interface_types.h"
#include "srsran/asn1/rrc.h"
/***************************
 *      RLC Config
 **************************/

namespace srsran
{

  enum class rlc_mode_t
  {
    tm,
    um,
    am,
    nulltype
  };
  inline const char *to_string(const rlc_mode_t &mode, bool long_txt = true)
  {
    constexpr static const char *long_options[] = {"Transparent Mode", "Unacknowledged Mode", "Acknowledged Mode"};
    constexpr static const char *short_options[] = {"TM", "UM", "AM"};
    if (long_txt)
    {
      return enum_to_text(long_options, (uint32_t)rlc_mode_t::nulltype, (uint32_t)mode);
    }
    return enum_to_text(short_options, (uint32_t)rlc_mode_t::nulltype, (uint32_t)mode);
  }

  enum class rlc_umd_sn_size_t
  {
    size5bits,
    size10bits,
    nulltype
  };
  inline std::string to_string(const rlc_umd_sn_size_t &sn_size)
  {
    constexpr static const char *options[] = {"5 bits", "10 bits"};
    return enum_to_text(options, (uint32_t)rlc_mode_t::nulltype, (uint32_t)sn_size);
  }
  inline uint16_t to_number(const rlc_umd_sn_size_t &sn_size)
  {
    constexpr static uint16_t options[] = {5, 10};
    return enum_to_number(options, (uint32_t)rlc_mode_t::nulltype, (uint32_t)sn_size);
  }

  ///< RLC UM NR sequence number field
  enum class rlc_um_nr_sn_size_t
  {
    size6bits,
    size12bits,
    nulltype
  };
  inline std::string to_string(const rlc_um_nr_sn_size_t &sn_size)
  {
    constexpr static const char *options[] = {"6 bits", "12 bits"};
    return enum_to_text(options, (uint32_t)rlc_mode_t::nulltype, (uint32_t)sn_size);
  }
  inline uint16_t to_number(const rlc_um_nr_sn_size_t &sn_size)
  {
    constexpr static uint16_t options[] = {6, 12};
    return enum_to_number(options, (uint32_t)rlc_mode_t::nulltype, (uint32_t)sn_size);
  }

  ///< RLC AM NR sequence number field
  enum class rlc_am_nr_sn_size_t
  {
    size12bits,
    size18bits,
    nulltype
  };
  inline std::string to_string(const rlc_am_nr_sn_size_t &sn_size)
  {
    constexpr static const char *options[] = {"12 bits", "18 bits"};
    return enum_to_text(options, (uint32_t)rlc_mode_t::nulltype, (uint32_t)sn_size);
  }
  inline uint16_t to_number(const rlc_am_nr_sn_size_t &sn_size)
  {
    constexpr static uint16_t options[] = {12, 18};
    return enum_to_number(options, (uint32_t)rlc_mode_t::nulltype, (uint32_t)sn_size);
  }

  struct rlc_am_config_t
  {
    /****************************************************************************
     * Configurable parameters
     * Ref: 3GPP TS 36.322 v10.0.0 Section 7
     ***************************************************************************/

    // TX configs
    int32_t t_poll_retx;      // Poll retx timeout (ms)
    int32_t poll_pdu;         // Insert poll bit after this many PDUs
    int32_t poll_byte;        // Insert poll bit after this much data (KB)
    uint32_t max_retx_thresh; // Max number of retx

    // RX configs
    int32_t t_reordering;      // Timer used by rx to detect PDU loss  (ms)
    int32_t t_status_prohibit; // Timer used by rx to prohibit tx of status PDU (ms)
  };

  struct rlc_um_config_t
  {
    /****************************************************************************
     * Configurable parameters
     * Ref: 3GPP TS 36.322 v10.0.0 Section 7
     ***************************************************************************/

    int32_t t_reordering;                 // Timer used by rx to detect PDU loss  (ms)
    rlc_umd_sn_size_t tx_sn_field_length; // Number of bits used for tx (UL) sequence number
    rlc_umd_sn_size_t rx_sn_field_length; // Number of bits used for rx (DL) sequence number

    uint32_t rx_window_size;
    uint32_t rx_mod; // Rx counter modulus
    uint32_t tx_mod; // Tx counter modulus
    bool is_mrb;     // Whether this is a multicast bearer
  };

  //------------------------IOT----------------
  struct iot_rlc_um_config_t
  {
    int32_t t_reordering;
    uint32_t rx_window_size;
    uint32_t rx_mod; // Rx counter modulus
    uint32_t tx_mod; // Tx counter modulus
    bool is_mrb;     // Whether this is a multicast bearer
  };
  //-------------------------------------------

  //---------------------------2023.10.17-----------------------------------------
  struct s_rlc_am_cfg_t
  {
    // UL-AM-RLC
    uint16_t t_poll_retran;
    uint8_t poll_pdu;
    uint16_t poll_byte;
    uint8_t max_retx_thres_hold;
    // DL - aM - RLC
    uint16_t t_reord;
    uint16_t t_status_proh;
  };
  //---------------------------------------------------------

  //-----------------------2023.11.2--------------------
  struct log_chan_cfg_t
  {
    uint8_t priority;
    uint16_t priori_Bit_Rate;
    uint16_t bucket_Size_Duration;
    uint8_t log_Chann_Group;
  };
  //---------------------------------------

  //-------------------2023.10.23------------------------------------
  struct s_rlc_um_config_t
  {
    uint16_t t_reord;
  };
  struct s_rlc_tm_config_t
  {
  };
  //---------------------------------------------------------

  struct rlc_um_nr_config_t
  {
    /****************************************************************************
     * Configurable parameters
     * Ref: 3GPP TS 38.322 v15.3.0 Section 7
     ***************************************************************************/

    rlc_um_nr_sn_size_t sn_field_length; // Number of bits used for sequence number
    int32_t t_reassembly_ms;             // Timer used by rx to detect PDU loss (ms)
  };

#define RLC_TX_QUEUE_LEN (256)

  class rlc_config_t
  {
  public:
    srsran_rat_t rat;
    rlc_mode_t rlc_mode;
    rlc_am_config_t am;
    rlc_um_config_t um;
    //-----------------2023.10.17-------------------------
    s_rlc_am_cfg_t asam;
    s_rlc_um_config_t aum;
    s_rlc_tm_config_t atm;
    log_chan_cfg_t log_chan_cfg;
    //------------------------------------
    iot_rlc_um_config_t iot_um;
    rlc_um_nr_config_t um_nr;
    uint32_t tx_queue_length;

    rlc_config_t() : rat(srsran_rat_t::lte), rlc_mode(rlc_mode_t::tm), am(), um(), um_nr(), tx_queue_length(RLC_TX_QUEUE_LEN){};

    // Factory for MCH
    static rlc_config_t mch_config()
    {
      rlc_config_t cfg = {};
      cfg.rat = srsran_rat_t::lte;
      cfg.rlc_mode = rlc_mode_t::um;
      cfg.um.t_reordering = 45;
      cfg.um.rx_sn_field_length = rlc_umd_sn_size_t::size5bits;
      cfg.um.rx_window_size = 16;
      cfg.um.rx_mod = 32;
      cfg.um.tx_sn_field_length = rlc_umd_sn_size_t::size5bits;
      cfg.um.tx_mod = 32;
      cfg.um.is_mrb = true;
      cfg.tx_queue_length = 1024;
      return cfg;
    }

    //----------------------------------------------------
    static rlc_config_t s_srb_config(uint32_t idx = 1)
    {
      rlc_config_t rlc_cfg = {};
      rlc_cfg.rat = srsran_rat_t::lte;
      rlc_cfg.rlc_mode = rlc_mode_t::am;
      rlc_cfg.asam.t_poll_retran = 1200;
      // rlc_cfg.asam.poll_pdu            = asn1::rrc::ul_am_rlcc_s::poll_pdu_opts::pInfinity;
      rlc_cfg.asam.poll_pdu = -1;
      rlc_cfg.asam.poll_byte = -1;
      rlc_cfg.asam.max_retx_thres_hold = 4;
      rlc_cfg.asam.t_reord = 1200;
      rlc_cfg.asam.t_status_proh = 420;
      rlc_cfg.log_chan_cfg.priority = 1;
      rlc_cfg.log_chan_cfg.priori_Bit_Rate = asn1::rrc::ul_spec_para_s::prio_bit_rate_opts::infinity;
      rlc_cfg.log_chan_cfg.bucket_Size_Duration = asn1::rrc::ul_spec_para_s::buck_size_dura_opts::spare2;
      rlc_cfg.log_chan_cfg.log_Chann_Group = 0;
      return rlc_cfg;
    }

    //-------------------------2023.10.17-------------------------------------
    static rlc_config_t default_s_rlc_am_config()
    {

      rlc_config_t rlc_cofg = {};
      rlc_cofg.asam.t_poll_retran = 1200;
      rlc_cofg.asam.poll_pdu = 8;
      rlc_cofg.asam.poll_byte = 16;
      rlc_cofg.asam.max_retx_thres_hold = 4;
      rlc_cofg.asam.t_reord = 1200;
      rlc_cofg.asam.t_status_proh = 0;

      return rlc_cofg;
    }
    //----------------------------------------------------
    //---------------------2023.10.23--------------
    static rlc_config_t default_s_rlc_um_config()
    {
      rlc_config_t cnfg = {};
      cnfg.aum.t_reord = 480;
      return cnfg;
    }
    static rlc_config_t default_s_rlc_tm_config()
    {
      rlc_config_t cnfg = {};
      return cnfg;
    }
    //----------------------------------------------

    static rlc_config_t srb_config(uint32_t idx)
    {
      if (idx == 0 or idx > 2)
      {
        return {};
      }
      // SRB1 and SRB2 are AM
      rlc_config_t rlc_cfg = {};
      rlc_cfg.rat = srsran_rat_t::lte;
      rlc_cfg.rlc_mode = rlc_mode_t::am;
      rlc_cfg.am.t_poll_retx = 1000;
      rlc_cfg.am.poll_pdu = -1;
      rlc_cfg.am.poll_byte = -1;
      rlc_cfg.am.max_retx_thresh = 4;
      rlc_cfg.am.t_reordering = 1000;
      rlc_cfg.am.t_status_prohibit = 0;
      return rlc_cfg;
    }
    static rlc_config_t default_rlc_um_config(uint32_t sn_size = 10)
    {
      rlc_config_t cnfg = {};
      cnfg.rat = srsran_rat_t::lte;
      cnfg.rlc_mode = rlc_mode_t::um;
      cnfg.um.t_reordering = 5;
      if (sn_size == 10)
      {
        cnfg.um.rx_sn_field_length = rlc_umd_sn_size_t::size10bits;
        cnfg.um.rx_window_size = 512;
        cnfg.um.rx_mod = 1024;
        cnfg.um.tx_sn_field_length = rlc_umd_sn_size_t::size10bits;
        cnfg.um.tx_mod = 1024;
      }
      else if (sn_size == 5)
      {
        cnfg.um.rx_sn_field_length = rlc_umd_sn_size_t::size5bits;
        cnfg.um.rx_window_size = 16;
        cnfg.um.rx_mod = 32;
        cnfg.um.tx_sn_field_length = rlc_umd_sn_size_t::size5bits;
        cnfg.um.tx_mod = 32;
      }
      else
      {
        return {};
      }
      return cnfg;
    }

    //------------------------IOT--------------------------
    static rlc_config_t default_iot_rlc_um_config()
    {
      rlc_config_t cnfg = {};
      cnfg.rat = srsran_rat_t::lte;
      cnfg.rlc_mode = rlc_mode_t::um;
      cnfg.iot_um.t_reordering = 5;
      cnfg.iot_um.rx_window_size = 32;
      cnfg.iot_um.rx_mod = 64;
      cnfg.iot_um.tx_mod = 64;
      cnfg.iot_um.is_mrb = false;
      return cnfg;
    }
    //-----------------------------------------------------

    static rlc_config_t default_rlc_am_config()
    {
      rlc_config_t rlc_cnfg = {};
      rlc_cnfg.rat = srsran_rat_t::lte;
      rlc_cnfg.rlc_mode = rlc_mode_t::am;
      rlc_cnfg.am.t_reordering = 5;
      rlc_cnfg.am.t_status_prohibit = 5;
      rlc_cnfg.am.max_retx_thresh = 4;
      rlc_cnfg.am.poll_byte = 25;
      rlc_cnfg.am.poll_pdu = 4;
      rlc_cnfg.am.t_poll_retx = 5;
      return rlc_cnfg;
    }
    static rlc_config_t default_rlc_um_nr_config(uint32_t sn_size = 6)
    {
      rlc_config_t cnfg = {};
      cnfg.rat = srsran_rat_t::nr;
      cnfg.rlc_mode = rlc_mode_t::um;
      if (sn_size == 6)
      {
        cnfg.um_nr.sn_field_length = rlc_um_nr_sn_size_t::size6bits;
      }
      else if (sn_size == 12)
      {
        cnfg.um_nr.sn_field_length = rlc_um_nr_sn_size_t::size12bits;
      }
      else
      {
        return {};
      }
      cnfg.um_nr.t_reassembly_ms = 5; // lowest non-zero value
      return cnfg;
    }
  };

} // namespace srsran

#endif // SRSRAN_RLC_INTERFACE_TYPES_H
