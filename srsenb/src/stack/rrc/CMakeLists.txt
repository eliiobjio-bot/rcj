#
# Copyright 2013-2021 Software Radio Systems Limited
#
# This file is part of srsRAN
#
# srsRAN is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# srsRAN is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# A copy of the GNU Affero General Public License can be found in
# the LICENSE file in the top-level directory of this distribution
# and at http://www.gnu.org/licenses/.
#


set(SOURCES rrc.cc rrc_ue.cc rrc_mobility.cc rrc_cell_cfg.cc rrc_bearer_cfg.cc mac_controller.cc ue_rr_cfg.cc ue_meas_cfg.cc rrc_endc.cc ${PROJECT_SOURCE_DIR}/srscnw/src/nas_mm.cc ${PROJECT_SOURCE_DIR}/srscnw/src/cnw.cc ${PROJECT_SOURCE_DIR}/srscnw/src/nas_context.cc ${PROJECT_SOURCE_DIR}/lib/src/asn1/nas_5g_ies.cc  ${PROJECT_SOURCE_DIR}/lib/src/asn1/nas_5g_msg.cc ${PROJECT_SOURCE_DIR}/lib/src/asn1/nas_5g_utils.cc ../../adp.cc ../../udp.cc)
add_library(srsenb_rrc STATIC ${SOURCES})

# target_link_libraries(srsenb_rrc srsran_asn1 srscnw)
#target_link_libraries(srsenb_rrc srscnw)
set(SOURCES rrc_nr.cc nr/cell_asn1_config.cc)
add_library(srsgnb_rrc STATIC ${SOURCES})

# target_include_directories(srsenb_rrc PUBLIC
#     ${PROJECT_SOURCE_DIR}/../../../../srscnw/hdr
# )
#target_link_libraries(srsenb_rrc  srscnw)