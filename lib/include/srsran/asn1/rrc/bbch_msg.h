/*******************************************************************************
 *
 *                           Satellite BBCH-Message
 *
 ******************************************************************************/

#ifndef SRSASN1_RRC_BBCH_MSG_H
#define SRSASN1_RRC_BBCH_MSG_H

#include "rr_common.h"

namespace asn1 {
namespace rrc {
/*******************************************************************************
 *                              Struct Definitions
 ******************************************************************************/

//HotSpotInformation ::=               SEQUENCE
struct hot_spot_info_s {
  // member variables
  dyn_octstring hot_spot_info_r1;

    // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// BBCH - MessageType :: = HotSpotInformation
using bbch_msg_type_s = hot_spot_info_s;

// BBCH - Message :: = SEQUENCE
struct bbch_msg_s {
  // member variables
  bbch_msg_type_s msg;
  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

} // namespace rrc
} // namespace asn1

#endif // SRSASN1_RRC_BBCH_MSG_H