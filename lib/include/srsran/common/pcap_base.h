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

#ifndef SRSRAN_MAC_PCAP_BASE_H
#define SRSRAN_MAC_PCAP_BASE_H

#include "srsran/adt/circular_buffer.h"
#include "srsran/common/buffer_pool.h"
#include "srsran/common/common.h"
#include "srsran/common/pcap.h"
#include "srsran/common/threads.h"
#include "srsran/srslog/srslog.h"
#include <mutex>
#include <stdint.h>
#include <thread>

namespace srsran {

class pcap_base : protected srsran::thread
{
public:
  typedef struct {
    // Different PCAP context for both RATs
    srsran::srsran_rat_t  rat;
    MAC_Context_Info_t    context;
    RRC_Context_Info_t    rrc_context;
    NAS_Context_Info_t    nas_context;
    mac_nr_context_info_t context_nr;
    unique_byte_buffer_t  pdu;
    uint8_t               module;
    uint8_t               reserve[3];
  } pcap_pdu_t;
  pcap_base();

  pcap_base(const pcap_base& other)            = delete;
  pcap_base& operator=(const pcap_base& other) = delete;
  pcap_base(pcap_base&& other)                 = delete;
  pcap_base& operator=(pcap_base&& other)      = delete;

  ~pcap_base();
  void             enable(bool enable);
  virtual uint32_t close() = 0;

  void set_ue_id(uint16_t ue_id);

  void write_ul_rrc_pdu(const uint8_t*       input,
                        int32_t              input_len,
                        CY_LOGICCHANNEL_TYPE channel    = CY_LOGICCHANNEL_TYPE_UL_DCCH,
                        CY_NET_MODE          acess_type = CY_NET_MODE_RAN);
  void write_dl_rrc_pdu(const uint8_t*       input,
                        int32_t              input_len,
                        CY_LOGICCHANNEL_TYPE channel    = CY_LOGICCHANNEL_TYPE_DL_DCCH,
                        CY_NET_MODE          acess_type = CY_NET_MODE_RAN);
  void write_ul_crnti(const uint8_t*       pdu,
                      uint32_t             pdu_len_bytes,
                      uint16_t             crnti,
                      uint32_t             reTX,
                      uint32_t             tti,
                      uint8_t              cc_idx,
                      uint8_t              module     = CY_LAYER_TYPE_MAC,
                      CY_LOGICCHANNEL_TYPE channel    = CY_LOGICCHANNEL_TYPE_DL_DCCH,
                      CY_NET_MODE          acess_type = CY_NET_MODE_RAN);
  void write_dl_crnti(const uint8_t*       pdu,
                      uint32_t             pdu_len_bytes,
                      uint16_t             crnti,
                      bool                 crc_ok,
                      uint32_t             tti,
                      uint8_t              cc_idx,
                      uint8_t              module     = CY_LAYER_TYPE_MAC,
                      CY_LOGICCHANNEL_TYPE channel    = CY_LOGICCHANNEL_TYPE_DL_DCCH,
                      CY_NET_MODE          acess_type = CY_NET_MODE_RAN);
  void write_dl_ranti(const uint8_t*       pdu,
                      uint32_t             pdu_len_bytes,
                      uint16_t             ranti,
                      bool                 crc_ok,
                      uint32_t             tti,
                      uint8_t              cc_idx,
                      uint8_t              module     = CY_LAYER_TYPE_MAC,
                      CY_LOGICCHANNEL_TYPE channel    = CY_LOGICCHANNEL_TYPE_DL_DCCH,
                      CY_NET_MODE          acess_type = CY_NET_MODE_RAN);

  void write_ul_crnti(const uint8_t*       pdu,
                      uint32_t             pdu_len_bytes,
                      uint16_t             crnti,
                      uint16_t             ue_id,
                      uint32_t             reTX,
                      uint32_t             tti,
                      uint8_t              cc_idx,
                      uint8_t              module     = CY_LAYER_TYPE_MAC,
                      CY_LOGICCHANNEL_TYPE channel    = CY_LOGICCHANNEL_TYPE_DL_DCCH,
                      CY_NET_MODE          acess_type = CY_NET_MODE_RAN);

  void write_dl_crnti(const uint8_t*       pdu,
                      uint32_t             pdu_len_bytes,
                      uint16_t             crnti,
                      uint16_t             ue_id,
                      bool                 crc_ok,
                      uint32_t             tti,
                      uint8_t              cc_idx,
                      uint8_t              module     = CY_LAYER_TYPE_MAC,
                      CY_LOGICCHANNEL_TYPE channel    = CY_LOGICCHANNEL_TYPE_DL_DCCH,
                      CY_NET_MODE          acess_type = CY_NET_MODE_RAN);

  // SI and BCH only for DL
  void write_dl_sirnti(const uint8_t*       pdu,
                       uint32_t             pdu_len_bytes,
                       bool                 crc_ok,
                       uint32_t             tti,
                       uint8_t              cc_idx,
                       uint8_t              module     = CY_LAYER_TYPE_MAC,
                       CY_LOGICCHANNEL_TYPE channel    = CY_LOGICCHANNEL_TYPE_DL_DCCH,
                       CY_NET_MODE          acess_type = CY_NET_MODE_RAN);
  void write_dl_bch(const uint8_t*       pdu,
                    uint32_t             pdu_len_bytes,
                    bool                 crc_ok,
                    uint32_t             tti,
                    uint8_t              cc_idx,
                    uint8_t              module     = CY_LAYER_TYPE_MAC,
                    CY_LOGICCHANNEL_TYPE channel    = CY_LOGICCHANNEL_TYPE_DL_DCCH,
                    CY_NET_MODE          acess_type = CY_NET_MODE_RAN);
  void write_dl_pch(const uint8_t*       pdu,
                    uint32_t             pdu_len_bytes,
                    bool                 crc_ok,
                    uint32_t             tti,
                    uint8_t              cc_idx,
                    uint8_t              module     = CY_LAYER_TYPE_MAC,
                    CY_LOGICCHANNEL_TYPE channel    = CY_LOGICCHANNEL_TYPE_DL_DCCH,
                    CY_NET_MODE          acess_type = CY_NET_MODE_RAN);
  void write_dl_mch(const uint8_t*       pdu,
                    uint32_t             pdu_len_bytes,
                    bool                 crc_ok,
                    uint32_t             tti,
                    uint8_t              cc_idx,
                    uint8_t              module     = CY_LAYER_TYPE_MAC,
                    CY_LOGICCHANNEL_TYPE channel    = CY_LOGICCHANNEL_TYPE_DL_DCCH,
                    CY_NET_MODE          acess_type = CY_NET_MODE_RAN);

  ////////////////////////needn't focus//////////////////////
  // Sidelink
  void write_sl_crnti(const uint8_t*       pdu,
                      uint32_t             pdu_len_bytes,
                      uint16_t             rnti,
                      uint32_t             reTX,
                      uint32_t             tti,
                      uint8_t              cc_idx,
                      uint8_t              module     = CY_LAYER_TYPE_MAC,
                      CY_LOGICCHANNEL_TYPE channel    = CY_LOGICCHANNEL_TYPE_DL_DCCH,
                      CY_NET_MODE          acess_type = CY_NET_MODE_RAN);

  // NR
  void write_dl_crnti_nr(uint8_t* pdu, uint32_t pdu_len_bytes, uint16_t crnti, uint8_t harqid, uint32_t tti);
  void write_ul_crnti_nr(uint8_t* pdu, uint32_t pdu_len_bytes, uint16_t rnti, uint8_t harqid, uint32_t tti);
  void write_dl_ra_rnti_nr(uint8_t* pdu, uint32_t pdu_len_bytes, uint16_t rnti, uint8_t harqid, uint32_t tti);
  void write_dl_bch_nr(uint8_t* pdu, uint32_t pdu_len_bytes, uint16_t rnti, uint8_t harqid, uint32_t tti);
  void write_dl_pch_nr(uint8_t* pdu, uint32_t pdu_len_bytes, uint16_t rnti, uint8_t harqid, uint32_t tti);
  void write_dl_si_rnti_nr(uint8_t* pdu, uint32_t pdu_len_bytes, uint16_t rnti, uint8_t harqid, uint32_t tti);

  // NR for enb with different ue_id
  // clang-format off
  void write_dl_crnti_nr(uint8_t* pdu, uint32_t pdu_len_bytes, uint16_t crnti, uint16_t ue_id, uint8_t harqid, uint32_t tti);
  void write_ul_crnti_nr(uint8_t* pdu, uint32_t pdu_len_bytes, uint16_t rnti, uint16_t ue_id, uint8_t harqid, uint32_t tti);
  // clang-format on

  void write_ul_nas_pdu(const uint8_t* input, int32_t input_len, CY_NET_MODE acess_type);
  void write_dl_nas_pdu(const uint8_t* input,
                        const int32_t  input_len,
                        // CY_LOGICCHANNEL_TYPE channel,
                        CY_NET_MODE acess_type);

protected:
  virtual void write_pdu(pcap_pdu_t& pdu) = 0;
  void         run_thread() final;

  std::mutex                                     mutex;
  srslog::basic_logger&                          logger;
  std::atomic<bool>                              running = {false};
  static static_blocking_queue<pcap_pdu_t, 1024> queue;
  uint16_t                                       ue_id = 0;

private:
  void pack_and_queue(uint8_t* payload,
                      uint32_t payload_len,
                      uint16_t ue_id,
                      uint32_t reTX,
                      bool     crc_ok,
                      uint8_t  cc_idx,
                      uint32_t tti,
                      uint16_t crnti_,
                      uint8_t  direction,
                      uint8_t  rnti_type);
  void pack_and_queue_nbiot(const uint8_t*       payload,
                            uint32_t             payload_len,
                            uint16_t             ue_id,
                            uint32_t             reTX,
                            bool                 crc_ok,
                            uint8_t              cc_idx,
                            uint32_t             tti,
                            uint16_t             crnti_,
                            uint8_t              direction,
                            uint8_t              rnti_type,
                            uint8_t              m          = CY_LAYER_TYPE_MAC,
                            CY_LOGICCHANNEL_TYPE channel    = CY_LOGICCHANNEL_TYPE_DL_DCCH,
                            CY_NET_MODE          acess_type = CY_NET_MODE_RAN);
  void pack_and_queue_nr(uint8_t* payload,
                         uint32_t payload_len,
                         uint32_t tti,
                         uint16_t crnti,
                         uint16_t ue_id,
                         uint8_t  harqid,
                         uint8_t  direction,
                         uint8_t  rnti_type);
};

} // namespace srsran

#endif // SRSRAN_MAC_PCAP_BASE_H
