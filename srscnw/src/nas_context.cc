#include "srscnw/hdr/nas_context.h"
#include "srscnw/hdr/cnw.h"

using namespace srsran::nas_5g;

namespace srsepc {

security_algorithms_t::ciphering_algorithm_type
nas_context::convert_cipher_algo(srsran::CIPHERING_ALGORITHM_ID_ENUM cipher_algo)
{
  security_algorithms_t::ciphering_algorithm_type ret;
  switch (cipher_algo) {
    case CIPHERING_ALGORITHM_ID_EEA0:
      ret = security_algorithms_t::ciphering_algorithm_type::ea0_5g;
      break;

    case CIPHERING_ALGORITHM_ID_128_EEA1:
      ret = security_algorithms_t::ciphering_algorithm_type::ea1_128_5g;
      break;

    case CIPHERING_ALGORITHM_ID_128_EEA2:
      ret = security_algorithms_t::ciphering_algorithm_type::ea2_128_5g;
      break;

    case CIPHERING_ALGORITHM_ID_128_EEA3:
      ret = security_algorithms_t::ciphering_algorithm_type::ea3_128_5g;
      break;

    default:
      srsran::console("Unsupported cipher algo, default algo is ea0.\n");
      ret = security_algorithms_t::ciphering_algorithm_type::ea0_5g;
      break;
  }
  return ret;
}

security_algorithms_t::integrity_protection_algorithm_type
nas_context::convert_integ_algo(srsran::INTEGRITY_ALGORITHM_ID_ENUM integ_algo)
{
  security_algorithms_t::integrity_protection_algorithm_type ret;
  switch (integ_algo) {
    case INTEGRITY_ALGORITHM_ID_EIA0:
      ret = security_algorithms_t::integrity_protection_algorithm_type::ia0_5g;
      break;

    case INTEGRITY_ALGORITHM_ID_128_EIA1:
      ret = security_algorithms_t::integrity_protection_algorithm_type::ia1_128_5g;
      break;

    case INTEGRITY_ALGORITHM_ID_128_EIA2:
      ret = security_algorithms_t::integrity_protection_algorithm_type::ia2_128_5g;
      break;

    case INTEGRITY_ALGORITHM_ID_128_EIA3:
      ret = security_algorithms_t::integrity_protection_algorithm_type::ia3_128_5g;
      break;

    default:
      srsran::console("Unsupported integrity protection algo, default algo is ia0.\n");
      ret = security_algorithms_t::integrity_protection_algorithm_type::ia0_5g;
      break;
  }
  return ret;
}

key_set_identifier_t::nas_key_set_identifier_type nas_context::convert_ng_ksi(uint8_t ngksi)
{
  key_set_identifier_t::nas_key_set_identifier_type ret;
  switch (ngksi) {
    case 0:
      ret = key_set_identifier_t::nas_key_set_identifier_type::ng_ksi_0;
      break;
    case 1:
      ret = key_set_identifier_t::nas_key_set_identifier_type::ng_ksi_1;
      break;
    case 2:
      ret = key_set_identifier_t::nas_key_set_identifier_type::ng_ksi_2;
      break;
    case 3:
      ret = key_set_identifier_t::nas_key_set_identifier_type::ng_ksi_3;
      break;
    case 4:
      ret = key_set_identifier_t::nas_key_set_identifier_type::ng_ksi_4;
      break;
    case 5:
      ret = key_set_identifier_t::nas_key_set_identifier_type::ng_ksi_5;
      break;
    case 6:
      ret = key_set_identifier_t::nas_key_set_identifier_type::ng_ksi_6;
      break;
    case 7:
      ret = key_set_identifier_t::nas_key_set_identifier_type::no_key_is_available_or_reserved;
      break;
    default:
      srsran::console("Error ngksi.\n");
      ret = key_set_identifier_t::nas_key_set_identifier_type::no_key_is_available_or_reserved;
      break;
  }
  return ret;
}
bool nas_context::compare_bytes_value(uint8_t* args1, uint8_t* args2, uint8_t length)
{
  for (uint8_t i = 0; i < length; i++) {
    if (args1[i] != args2[i]) {
      return false;
    }
  }

  return true;
}

void nas_context::nas_print_byte_buffer(srsran::unique_byte_buffer_t nas_buffer)
{
  std::cout << "----@@@@@@@ nas print byte buffer @@@@@---" << std::endl;
  // for (uint8_t i = 0; i < nas_buffer->N_bytes; i++) {
  //   printf("0x%x\n", *(nas_buffer->msg + i));
  // }
  // return;
}

//sm 12-5
pdu_session_type_t::PDU_session_type_value_type nas_context::convert_pdu_type(uint8_t ue_session_type)
{
  pdu_session_type_t::PDU_session_type_value_type ret=pdu_session_type_t::PDU_session_type_value_type::ipv4;
  //std::cout<<"-----ue_session_type-----:"<<std::endl;
  switch (ue_session_type) {
    case 1:
      ret = pdu_session_type_t::PDU_session_type_value_type::ipv4;
      break;
    case 2:
      ret = pdu_session_type_t::PDU_session_type_value_type::ipv6;
      break;
    case 3:
      ret = pdu_session_type_t::PDU_session_type_value_type::ipv4v6;
      break;
    case 4:
      ret = pdu_session_type_t::PDU_session_type_value_type::unstructured;
      break;
    case 5:
      ret = pdu_session_type_t::PDU_session_type_value_type::ethernet;
      break;
    case 6:
      ret = pdu_session_type_t::PDU_session_type_value_type::reserved;
      break;
    default:
      srsran::console("Error pdu session type.\n");
      break;
  }
  return ret;
}

ssc_mode_t::SSC_mode_value_type nas_context::convert_ssc_mode(uint8_t ssc_mode)
{
  ssc_mode_t::SSC_mode_value_type ret=ssc_mode_t::SSC_mode_value_type::ssc_mode_1;
switch (ssc_mode) {
  case 1:
    ret = ssc_mode_t::SSC_mode_value_type::ssc_mode_1;
    break;
  case 2:
    ret = ssc_mode_t::SSC_mode_value_type::ssc_mode_2;
    break;
  case 3:
    ret = ssc_mode_t::SSC_mode_value_type::ssc_mode_3;
    break;
  case 4:
    ret = ssc_mode_t::SSC_mode_value_type::unused_or_ssc_mode_1;
    break;
  case 5:
    ret = ssc_mode_t::SSC_mode_value_type::unused_or_ssc_mode_2;
    break;
  case 6:
    ret = ssc_mode_t::SSC_mode_value_type::unused_or_ssc_mode_3;
    break;
  case 7:
    ret = ssc_mode_t::SSC_mode_value_type::reserved;
    break;
  default:
    srsran::console("Error SSC mode.\n");
    break;
}
return ret;
}//12.9

} // namespace srsepc
