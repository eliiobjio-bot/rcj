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

#include "srsran/common/pcap_base.h"
#include "srsran/config.h"
#include "srsran/phy/common/phy_common.h"
#include "srsran/support/emergency_handlers.h"
#include <stdint.h>

namespace srsran {
static_blocking_queue<pcap_base::pcap_pdu_t, 1024> pcap_base::queue;
/// Try to flush the contents of the pcap class before the application is killed.
static void emergency_cleanup_handler(void* data)
{
  reinterpret_cast<pcap_base*>(data)->close();
}

pcap_base::pcap_base() : logger(srslog::fetch_basic_logger("PCAP_NET")), thread("PCAP_NET")
{
  add_emergency_cleanup_handler(emergency_cleanup_handler, this);
}

pcap_base::~pcap_base() {}

void pcap_base::enable(bool enable_)
{
  std::lock_guard<std::mutex> lock(mutex);
  running = enable_;
}

void pcap_base::set_ue_id(uint16_t ue_id_)
{
  std::lock_guard<std::mutex> lock(mutex);
  ue_id = ue_id_;
}

void pcap_base::run_thread()
{
  // blocking write until stopped
  while (running) {
    pcap_pdu_t pdu = queue.pop_blocking();
    {
      std::lock_guard<std::mutex> lock(mutex);
      write_pdu(pdu);
    }
  }

  // write remainder of queue
  pcap_pdu_t pdu = {};
  while (queue.try_pop(pdu)) {
    std::lock_guard<std::mutex> lock(mutex);
    write_pdu(pdu);
  }
}

// Function called from PHY worker context, locking not needed as PDU queue is thread-safe
void pcap_base::pack_and_queue(uint8_t* payload,
                               uint32_t payload_len,
                               uint16_t ue_id,
                               uint32_t reTX,
                               bool     crc_ok,
                               uint8_t  cc_idx,
                               uint32_t tti,
                               uint16_t crnti,
                               uint8_t  direction,
                               uint8_t  rnti_type)
{
  if (running && payload != nullptr) {
    pcap_pdu_t pdu             = {};
    pdu.rat                    = srsran::srsran_rat_t::lte;
    pdu.context.radioType      = FDD_RADIO;
    pdu.context.direction      = direction;
    pdu.context.rntiType       = rnti_type;
    pdu.context.rnti           = crnti;
    pdu.context.ueid           = ue_id;
    pdu.context.isRetx         = (uint8_t)reTX;
    pdu.context.crcStatusOK    = crc_ok;
    pdu.context.cc_idx         = cc_idx;
    pdu.context.sysFrameNumber = (uint16_t)(tti / 10);
    pdu.context.subFrameNumber = (uint16_t)(tti % 10);

    // try to allocate PDU buffer
    pdu.pdu = srsran::make_byte_buffer();
    if (pdu.pdu != nullptr && pdu.pdu->get_tailroom() >= payload_len) {
      // copy payload into PDU buffer
      memcpy(pdu.pdu->msg, payload, payload_len);
      pdu.pdu->N_bytes = payload_len;
      if (not queue.try_push(std::move(pdu))) {
        logger.warning("Dropping PDU (%d B) in PCAP. Write queue full.", payload_len);
      }
    } else {
      logger.warning("Dropping PDU in PCAP. No buffer available or not enough space (pdu_len=%d).", payload_len);
    }
  }
}
void pcap_base::pack_and_queue_nbiot(const uint8_t*       payload,
                                     uint32_t             payload_len,
                                     uint16_t             ue_id,
                                     uint32_t             reTX,
                                     bool                 crc_ok,
                                     uint8_t              cc_idx,
                                     uint32_t             tti,
                                     uint16_t             crnti,
                                     uint8_t              direction,
                                     uint8_t              rnti_type,
                                     uint8_t              module,
                                     CY_LOGICCHANNEL_TYPE channel,
                                     CY_NET_MODE          acess_type)
{
  if (running && payload != nullptr) {
    pcap_pdu_t pdu = {};
    if (module == CY_LAYER_TYPE_MAC) {
      pdu.rat                    = srsran::srsran_rat_t::weixing;
      pdu.context.radioType      = FDD_RADIO;
      pdu.context.direction      = direction;
      pdu.context.rntiType       = rnti_type;
      pdu.context.rnti           = crnti;
      pdu.context.ueid           = ue_id;
      pdu.context.isRetx         = (uint8_t)reTX;
      pdu.context.crcStatusOK    = crc_ok;
      pdu.context.cc_idx         = cc_idx;
      pdu.context.sysFrameNumber = (uint16_t)(tti / 52);
      pdu.context.subFrameNumber = (uint16_t)(tti % 52);
      pdu.context.nbiotMode      = acess_type;
      return;
    } else if (module == CY_LAYER_TYPE_RRC) {
      pdu.rrc_context.channel   = channel;
      pdu.rrc_context.direction = direction;
      pdu.rrc_context.nbiotMode = acess_type;
    }  else if (module == CY_LAYER_TYPE_NAS) {
      // pdu.nas_context.channel   = channel;
      pdu.nas_context.direction = direction;
      pdu.nas_context.nbiotMode = acess_type;
    } else {
      return; // not support temp
    }
    pdu.module = module;
    pdu.rat    = srsran_rat_t::weixing;
    // try to allocate PDU buffer
    pdu.pdu = srsran::make_byte_buffer();
    if (pdu.pdu != nullptr && pdu.pdu->get_tailroom() >= payload_len) {
      // copy payload into PDU buffer
      memcpy(pdu.pdu->msg, payload, payload_len);
      pdu.pdu->N_bytes = payload_len;
      if (not queue.try_push(std::move(pdu))) {
        logger.warning("Dropping PDU (%d B) in PCAP. Write queue full.", payload_len);
      }
    } else {
      logger.warning("Dropping PDU in PCAP. No buffer available or not enough space (pdu_len=%d).", payload_len);
    }
  }
}

void pcap_base::write_dl_crnti(const uint8_t*       pdu,
                               uint32_t             pdu_len_bytes,
                               uint16_t             rnti,
                               bool                 crc_ok,
                               uint32_t             tti,
                               uint8_t              cc_idx,
                               uint8_t              module,
                               CY_LOGICCHANNEL_TYPE channel,
                               CY_NET_MODE          acess_type)
{
  pack_and_queue_nbiot(
      pdu, pdu_len_bytes, ue_id, 0, crc_ok, cc_idx, tti, rnti, DIRECTION_DOWNLINK, C_RNTI, module, channel, acess_type);
}
void pcap_base::write_dl_ranti(const uint8_t*       pdu,
                               uint32_t             pdu_len_bytes,
                               uint16_t             rnti,
                               bool                 crc_ok,
                               uint32_t             tti,
                               uint8_t              cc_idx,
                               uint8_t              module,
                               CY_LOGICCHANNEL_TYPE channel,
                               CY_NET_MODE          acess_type)
{
  pack_and_queue_nbiot(pdu,
                       pdu_len_bytes,
                       ue_id,
                       0,
                       crc_ok,
                       cc_idx,
                       tti,
                       rnti,
                       DIRECTION_DOWNLINK,
                       RA_RNTI,
                       module,
                       channel,
                       acess_type);
}
void pcap_base::write_ul_crnti(const uint8_t*       pdu,
                               uint32_t             pdu_len_bytes,
                               uint16_t             rnti,
                               uint32_t             reTX,
                               uint32_t             tti,
                               uint8_t              cc_idx,
                               uint8_t              module,
                               CY_LOGICCHANNEL_TYPE channel,
                               CY_NET_MODE          acess_type)
{
  pack_and_queue_nbiot(
      pdu, pdu_len_bytes, ue_id, reTX, true, cc_idx, tti, rnti, DIRECTION_UPLINK, C_RNTI, module, channel, acess_type);
}

void pcap_base::write_ul_crnti(const uint8_t*       pdu,
                               uint32_t             pdu_len_bytes,
                               uint16_t             rnti,
                               uint16_t             ue_id,
                               uint32_t             reTX,
                               uint32_t             tti,
                               uint8_t              cc_idx,
                               uint8_t              module,
                               CY_LOGICCHANNEL_TYPE channel,
                               CY_NET_MODE          acess_type)
{
  pack_and_queue_nbiot(
      pdu, pdu_len_bytes, ue_id, reTX, true, cc_idx, tti, rnti, DIRECTION_UPLINK, C_RNTI, module, channel, acess_type);
}

void pcap_base::write_dl_crnti(const uint8_t*       pdu,
                               uint32_t             pdu_len_bytes,
                               uint16_t             rnti,
                               uint16_t             ue_id,
                               bool                 crc_ok,
                               uint32_t             tti,
                               uint8_t              cc_idx,
                               uint8_t              module,
                               CY_LOGICCHANNEL_TYPE channel,
                               CY_NET_MODE          acess_type)
{
  pack_and_queue_nbiot(
      pdu, pdu_len_bytes, ue_id, 0, crc_ok, cc_idx, tti, rnti, DIRECTION_DOWNLINK, C_RNTI, module, channel, acess_type);
}

void pcap_base::write_sl_crnti(const uint8_t*       pdu,
                               uint32_t             pdu_len_bytes,
                               uint16_t             rnti,
                               uint32_t             reTX,
                               uint32_t             tti,
                               uint8_t              cc_idx,
                               uint8_t              module,
                               CY_LOGICCHANNEL_TYPE channel,
                               CY_NET_MODE          acess_type)
{
  pack_and_queue_nbiot(
      pdu, pdu_len_bytes, ue_id, reTX, true, cc_idx, tti, rnti, DIRECTION_UPLINK, SL_RNTI, module, channel, acess_type);
}

void pcap_base::write_dl_bch(const uint8_t*       pdu,
                             uint32_t             pdu_len_bytes,
                             bool                 crc_ok,
                             uint32_t             tti,
                             uint8_t              cc_idx,
                             uint8_t              module,
                             CY_LOGICCHANNEL_TYPE channel,
                             CY_NET_MODE          acess_type)
{
  pack_and_queue_nbiot(
      pdu, pdu_len_bytes, ue_id, 0, crc_ok, cc_idx, tti, 0, DIRECTION_DOWNLINK, NO_RNTI, module, channel, acess_type);
}
void pcap_base::write_dl_pch(const uint8_t*       pdu,
                             uint32_t             pdu_len_bytes,
                             bool                 crc_ok,
                             uint32_t             tti,
                             uint8_t              cc_idx,
                             uint8_t              module,
                             CY_LOGICCHANNEL_TYPE channel,
                             CY_NET_MODE          acess_type)
{
  pack_and_queue_nbiot(pdu,
                       pdu_len_bytes,
                       ue_id,
                       0,
                       crc_ok,
                       cc_idx,
                       tti,
                       SRSRAN_PRNTI,
                       DIRECTION_DOWNLINK,
                       P_RNTI,
                       module,
                       channel,
                       acess_type);
}
void pcap_base::write_dl_mch(const uint8_t*       pdu,
                             uint32_t             pdu_len_bytes,
                             bool                 crc_ok,
                             uint32_t             tti,
                             uint8_t              cc_idx,
                             uint8_t              module,
                             CY_LOGICCHANNEL_TYPE channel,
                             CY_NET_MODE          acess_type)
{
  pack_and_queue_nbiot(pdu,
                       pdu_len_bytes,
                       ue_id,
                       0,
                       crc_ok,
                       cc_idx,
                       tti,
                       SRSRAN_MRNTI,
                       DIRECTION_DOWNLINK,
                       M_RNTI,
                       module,
                       channel,
                       acess_type);
}
void pcap_base::write_dl_sirnti(const uint8_t*       pdu,
                                uint32_t             pdu_len_bytes,
                                bool                 crc_ok,
                                uint32_t             tti,
                                uint8_t              cc_idx,
                                uint8_t              module,
                                CY_LOGICCHANNEL_TYPE channel,
                                CY_NET_MODE          acess_type)
{
  pack_and_queue_nbiot(pdu,
                       pdu_len_bytes,
                       ue_id,
                       0,
                       crc_ok,
                       cc_idx,
                       tti,
                       SRSRAN_SIRNTI,
                       DIRECTION_DOWNLINK,
                       SI_RNTI,
                       module,
                       channel,
                       acess_type);
}

void pcap_base::write_dl_crnti_nr(uint8_t* pdu, uint32_t pdu_len_bytes, uint16_t rnti, uint8_t harqid, uint32_t tti)
{
  // pack_and_queue_nr(pdu, pdu_len_bytes, tti, rnti, ue_id, harqid, DIRECTION_DOWNLINK, C_RNTI);
}

void pcap_base::write_dl_crnti_nr(uint8_t* pdu,
                                  uint32_t pdu_len_bytes,
                                  uint16_t crnti,
                                  uint16_t ue_id,
                                  uint8_t  harqid,
                                  uint32_t tti)
{
  // pack_and_queue_nr(pdu, pdu_len_bytes, tti, crnti, ue_id, harqid, DIRECTION_DOWNLINK, C_RNTI);
}

void pcap_base::write_ul_crnti_nr(uint8_t* pdu, uint32_t pdu_len_bytes, uint16_t rnti, uint8_t harqid, uint32_t tti)
{
  // pack_and_queue_nr(pdu, pdu_len_bytes, tti, rnti, ue_id, harqid, DIRECTION_UPLINK, C_RNTI);
}

void pcap_base::write_ul_crnti_nr(uint8_t* pdu,
                                  uint32_t pdu_len_bytes,
                                  uint16_t rnti,
                                  uint16_t ue_id,
                                  uint8_t  harqid,
                                  uint32_t tti)
{
  // pack_and_queue_nr(pdu, pdu_len_bytes, tti, rnti, ue_id, harqid, DIRECTION_UPLINK, C_RNTI);
}

void pcap_base::write_dl_ra_rnti_nr(uint8_t* pdu, uint32_t pdu_len_bytes, uint16_t rnti, uint8_t harqid, uint32_t tti)
{
  // pack_and_queue_nr(pdu, pdu_len_bytes, tti, rnti, ue_id, harqid, DIRECTION_DOWNLINK, RA_RNTI);
}

void pcap_base::write_dl_bch_nr(uint8_t* pdu, uint32_t pdu_len_bytes, uint16_t rnti, uint8_t harqid, uint32_t tti)
{
  // pack_and_queue_nr(pdu, pdu_len_bytes, tti, rnti, ue_id, harqid, DIRECTION_DOWNLINK, NO_RNTI);
}

void pcap_base::write_dl_pch_nr(uint8_t* pdu, uint32_t pdu_len_bytes, uint16_t rnti, uint8_t harqid, uint32_t tti)
{
  // pack_and_queue_nr(pdu, pdu_len_bytes, tti, rnti, ue_id, harqid, DIRECTION_DOWNLINK, P_RNTI);
}

void pcap_base::write_dl_si_rnti_nr(uint8_t* pdu, uint32_t pdu_len_bytes, uint16_t rnti, uint8_t harqid, uint32_t tti)
{
  // pack_and_queue_nr(pdu, pdu_len_bytes, tti, rnti, ue_id, harqid, DIRECTION_DOWNLINK, SI_RNTI);
}

// modified for xw
void pcap_base::write_ul_rrc_pdu(const uint8_t*       input,
                                 int32_t              input_len,
                                 CY_LOGICCHANNEL_TYPE channel,
                                 CY_NET_MODE          acess_type)
{
  write_ul_crnti(input, input_len, 0, true, 0, 0, CY_LAYER_TYPE_RRC, channel, acess_type);
}
void pcap_base::write_dl_rrc_pdu(const uint8_t*       input,
                                 const int32_t        input_len,
                                 CY_LOGICCHANNEL_TYPE channel,
                                 CY_NET_MODE          acess_type)
{
  write_dl_crnti(input, input_len, 0, true, 0, 0, CY_LAYER_TYPE_RRC, channel, acess_type);
}
void pcap_base::write_ul_nas_pdu(const uint8_t*       input,
                                 int32_t              input_len,
                                 //CY_LOGICCHANNEL_TYPE channel,
                                 CY_NET_MODE          acess_type)
{
  write_ul_crnti(input, input_len, 0, true, 0, 0, CY_LAYER_TYPE_NAS, CY_LOGICCHANNEL_TYPE_NULL, acess_type);
}
void pcap_base::write_dl_nas_pdu(const uint8_t*       input,
                                 const int32_t        input_len,
                                 //CY_LOGICCHANNEL_TYPE channel,
                                 CY_NET_MODE          acess_type)
{
  write_dl_crnti(input, input_len, 0, true, 0, 0, CY_LAYER_TYPE_NAS, CY_LOGICCHANNEL_TYPE_NULL, acess_type);
}
} // namespace srsran
