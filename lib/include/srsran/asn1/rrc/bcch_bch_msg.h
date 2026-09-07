/*******************************************************************************
 *
 *                           Satellite MIB and SIB
 *
 ******************************************************************************/

#ifndef SRSASN1_RRC_BCCH_BCH_MSG_H
#define SRSASN1_RRC_BCCH_BCH_MSG_H

#include "mib_sib_asn1.h"

namespace asn1 {
namespace rrc {
/*******************************************************************************
 *                              Struct Definitions
 ******************************************************************************/

// MBCH-MessageType ::= MasterInformationBlock-WX
using bcch_mbch_msg_type_s = mib_wx_s;

// MBCH-Message ::= SEQUENCE
struct bcch_mbch_msg_s {
  // member variables
  bcch_mbch_msg_type_s msg_wx;

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

// BCCH-SBCH-MessageType ::= SystemInfomationBlockWX
using bcch_sbch_msg_type_s = sib_wx_s;

// BCCH-SBCH-MessageType ::= SEQUENCE
struct bcch_sbch_msg_s {
  // member variables
  bcch_sbch_msg_type_s msg;

  // sequence methods
  SRSASN_CODE pack(bit_ref& bref) const;
  SRSASN_CODE unpack(cbit_ref& bref);
  void        to_json(json_writer& j) const;
};

} // namespace rrc
} // namespace asn1

#endif // SRSASN1_RRC_SI_H