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

#ifndef SRSRAN_MAC_PCAP_NET_H
#define SRSRAN_MAC_PCAP_NET_H

#include "srsran/common/common.h"
#include "srsran/common/pcap_base.h"
#include "srsran/common/network_utils.h"
#include "srsran/srsran.h"

namespace srsran {
class pcap_net : public pcap_base
{
public:
  pcap_net();
  virtual ~pcap_net();
  uint32_t open(std::string client_ip_addr_,
                std::string bind_addr_str    = "0.0.0.0",
                uint16_t    client_udp_port_ = 5847,
                uint16_t    bind_udp_port_   = 5687,
                uint32_t    ue_id_           = 0);
  uint32_t close();
  void set_remote(std::string client_ip_addr, uint16_t client_udp_port);
protected:
  void write_pdu(srsran::pcap_base::pcap_pdu_t& pdu);
  void write_mac_lte_pdu_to_net(srsran::pcap_base::pcap_pdu_t& pdu);
  void write_mac_nr_pdu_to_net(srsran::pcap_base::pcap_pdu_t& pdu);
  void write_mac_nbiot_pdu_to_net(pcap_pdu_t& pdu);
  static srsran::unique_socket socket;
  struct sockaddr_in    client_addr;
};
} // namespace srsran

#endif // SRSRAN_MAC_PCAP_NET_H
