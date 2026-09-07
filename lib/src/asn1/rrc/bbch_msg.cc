#include "srsran/asn1/rrc/bbch_msg.h"
#include <sstream>

using namespace asn1;
using namespace asn1::rrc;

/*******************************************************************************
 *                                Struct Methods
 ******************************************************************************/

// HotSpotInformation ::=               SEQUENCE
SRSASN_CODE hot_spot_info_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(hot_spot_info_r1.pack(bref));
  return SRSASN_SUCCESS;
}
SRSASN_CODE hot_spot_info_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(hot_spot_info_r1.unpack(bref));
  return SRSASN_SUCCESS;
}
void hot_spot_info_s::to_json(json_writer& j) const
{
  j.start_obj();
  j.write_str("hotSpotInformation-r1", hot_spot_info_r1.to_string());
  j.end_obj();
}

// BBCH - Message :: = SEQUENCE
SRSASN_CODE bbch_msg_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(msg.pack(bref));

  bref.align_bytes_zero();

  return SRSASN_SUCCESS;
}
SRSASN_CODE bbch_msg_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(msg.unpack(bref));

  bref.align_bytes();

  return SRSASN_SUCCESS;
}
void bbch_msg_s::to_json(json_writer& j) const
{
  j.start_array();
  j.start_obj();
  j.start_obj("BBCH-Message");
  j.write_fieldname("message");
  msg.to_json(j);
  j.end_obj();
  j.end_obj();
  j.end_array();
}