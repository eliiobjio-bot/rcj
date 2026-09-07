


#include "srsran/asn1/rrc/bcch_bch_msg.h"
#include <sstream>

using namespace asn1;
using namespace asn1::rrc;

/*******************************************************************************
 *                                Struct Methods
 ******************************************************************************/

// MBCH-Message ::=SEQUENCE
SRSASN_CODE bcch_mbch_msg_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(msg_wx.pack(bref));

  bref.align_bytes_zero();

  return SRSASN_SUCCESS;
}
SRSASN_CODE bcch_mbch_msg_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(msg_wx.unpack(bref));

  bref.align_bytes();

  return SRSASN_SUCCESS;
}
void bcch_mbch_msg_s::to_json(json_writer& j) const
{
  j.start_array();
  j.start_obj();
  j.start_obj("MBCH-Message");
  j.write_fieldname("message-wx");
  msg_wx.to_json(j);
  j.end_obj();
  j.end_obj();
  j.end_array();
}

// BCCH-SBCH-Message ::= SEQUENCE
SRSASN_CODE bcch_sbch_msg_s::pack(bit_ref& bref) const
{
  HANDLE_CODE(msg.pack(bref));

  bref.align_bytes_zero();

  return SRSASN_SUCCESS;
}
SRSASN_CODE bcch_sbch_msg_s::unpack(cbit_ref& bref)
{
  HANDLE_CODE(msg.unpack(bref));

  bref.align_bytes();

  return SRSASN_SUCCESS;
}
void bcch_sbch_msg_s::to_json(json_writer& j) const
{
  j.start_array();
  j.start_obj();
  j.start_obj("BCCH-SBCH-Message");
  j.write_fieldname("message");
  msg.to_json(j);
  j.end_obj();
  j.end_obj();
  j.end_array();
}