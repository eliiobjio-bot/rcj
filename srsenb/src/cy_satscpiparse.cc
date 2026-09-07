#include <cassert>
#include <iostream>
#include <stdlib.h>
#include <string>
// #include "cy_log.h"
#include "srsenb/hdr/cy_satscpiparse.h"
#include "srsran/common/cy_log.h"
// #include "cy_udp_socket.h"
#include "lib/include/srsran/srslog/srslog.h"
#include "sinks/cy_udp_sink.h"
using namespace srslog;
using namespace std;
#if 1
static SATCOMM_NODE st_CONFigure_REMOte_LOG[2] = {
    {"UDP", "UDP", 0, nullptr, &CYSATScpiParse::Set_UDP_Remote, &CYSATScpiParse::Get_UDP_Remote},
    {"", "", 0, nullptr, nullptr, nullptr}};

static SATCOMM_NODE st_CONFigure_LOG[] = {{"REMO", "REMOte", 1, st_CONFigure_REMOte_LOG, 0, 0},
                                          {"ENAB",
                                           "ENABle",
                                           0,
                                           nullptr,
                                           &CYSATScpiParse::Set_CONFigure_LOG_ENABLE,
                                           &CYSATScpiParse::Get_CONFigure_LOG_ENABLE},
                                          {"", "", 0, nullptr, 0, 0}};

static SATCOMM_NODE st_CONFigure_REMOte_PCAP[]{
    {"UDP", "UDP", 0, nullptr, &CYSATScpiParse::Set_PCAP_Remote, &CYSATScpiParse::Get_PCAP_Remote},
    {"", "", 0, nullptr, nullptr, nullptr}};

static SATCOMM_NODE st_CONFigure_PCAP[] = {{"REMO", "REMOte", 1, st_CONFigure_REMOte_PCAP, 0, 0}, {"", "", 0, 0, 0, 0}};

static SATCOMM_NODE st_CONFigure_BEAM_COMM[] = {
    {"BEAM", "BEAM", 0, nullptr, &CYSATScpiParse::Set_BEAM_Common_BEAM, &CYSATScpiParse::Get_BEAM_Common_BEAM},
    {"DUPM", "DUPMode", 0, nullptr, &CYSATScpiParse::Set_BEAM_Common_DUPMode, &CYSATScpiParse::Get_BEAM_Common_DUPMode},
    {"BAND", "BAND", 0, nullptr, &CYSATScpiParse::Set_BEAM_Common_Band, &CYSATScpiParse::Get_BEAM_Common_Band},
    {"NETWORK",
     "NETWORK",
     0,
     nullptr,
     &CYSATScpiParse::Set_BEAM_Common_Network,
     &CYSATScpiParse::Get_BEAM_Common_Network},
    {"MODE", "MODE", 0, nullptr, &CYSATScpiParse::Set_BEAM_Common_Mode, &CYSATScpiParse::Get_BEAM_Common_Mode},
    {"", "", 0, 0, 0, 0}};

static SATCOMM_NODE st_CONFigure_BEAM[] = {
    {"COMM", "COMMon", sizeof(st_CONFigure_BEAM_COMM) / sizeof(SATCOMM_NODE), st_CONFigure_BEAM_COMM, nullptr, nullptr},
    {"", "", 0, 0, 0, 0}};

static SATCOMM_NODE st_INIT_BEAM[] = {
    {"IMM", "IMMediate", 0, nullptr, &CYSATScpiParse::Set_BEAM_IMMediate, &CYSATScpiParse::Get_BEAM_IMMediate},
    {"", "", 0, 0, 0, 0}};

static SATCOMM_NODE st_INIT[] = {
    {"BEAM", "BEAM", sizeof(st_INIT_BEAM) / sizeof(SATCOMM_NODE), st_INIT_BEAM, nullptr, nullptr},
    {"", "", 0, 0, 0, 0}};

static SATCOMM_NODE st_CONFigure[] = {
    {"LOG", "LOG", sizeof(st_CONFigure_LOG) / sizeof(SATCOMM_NODE), st_CONFigure_LOG, 0, 0},
    {"PCAP", "PCAP", sizeof(st_CONFigure_PCAP) / sizeof(SATCOMM_NODE), st_CONFigure_PCAP, 0, 0},
    {"BEAM", "BEAM", sizeof(st_CONFigure_BEAM) / sizeof(SATCOMM_NODE), st_CONFigure_BEAM, 0, 0}, //
    {"", "", 0, 0, 0, 0}};
#endif
// CONFigure:LOG:REMOte:UDP  126.1.2.1:2000
// CONFigure:PCAP:REMOte:UDP  126.1.2.1:2000
static SATCOMM_NODE _RootNode[] = {
    {"", "*CLS", 0, 0, &CYSATScpiParse::Set_CLS, 0, NEED_RESET_MEAS_RESULT},
    {"", "*IDN", 0, 0, 0, &CYSATScpiParse::Get_IDN},
    {"", "*OPC", 0, 0, &CYSATScpiParse::Set_OPC, 0, NEED_RESET_MEAS_RESULT},
    {"", "*RST", 0, 0, &CYSATScpiParse::Set_RST, 0, NEED_RESET_MEAS_RESULT},
    {"INIT", "INITiate", sizeof(st_INIT) / sizeof(SATCOMM_NODE), st_INIT, nullptr, nullptr},
    {"CONF", "CONFig", sizeof(st_CONFigure) / sizeof(SATCOMM_NODE), st_CONFigure, 0, 0},
    {"", "", 0, 0, 0, 0}};

CYSATScpiParse::CYSATScpiParse(srsenb::enb* pENB, srsenb::all_args_t* args_) :
  ptENB(pENB), p_g_args_(args_), CYICScpiParse<CYSATScpiParse>(_RootNode, sizeof(_RootNode) / sizeof(SATCOMM_NODE))
{
}

CYSATScpiParse::~CYSATScpiParse() {}
void CYSATScpiParse::Set_CLS(std::string& ouput)
{
  VLE_LOGE(CONTROL_LEVEL_INFO, "CYSATScpiParse: Set_CLS()");
}

void CYSATScpiParse::Get_IDN(std::string& ouput)
{
  VLE_LOGE(CONTROL_LEVEL_INFO, "CYSATScpiParse: Get_IDN()");
}
void CYSATScpiParse::Set_OPC(std::string& ouput)
{
  VLE_LOGE(CONTROL_LEVEL_INFO, "CYSATScpiParse: Set_OPC()");
}
void CYSATScpiParse::Set_RST(std::string &ouput)
{
	VLE_LOGE(CONTROL_LEVEL_INFO,"CYSATScpiParse: Set_RST()");
}
void CYSATScpiParse::Set_DEBUG(std::string &ouput)
{
	VLE_LOGE(CONTROL_LEVEL_INFO,"CYSATScpiParse: Set_DEBUG()");
}
void CYSATScpiParse::Set_SYSTem_ERRor_CLEar_ALL(std::string &ouput)
{
	VLE_LOGE(CONTROL_LEVEL_INFO,"CYSATScpiParse: Set_SYSTem_ERRor_CLEar_ALL()");
}
void CYSATScpiParse::Get_SYSTem_ERRor_NEXT(std::string &ouput)
{
	VLE_LOGE(CONTROL_LEVEL_INFO,"CYSATScpiParse: Get_SYSTem_ERRor_NEXT()");
}
/// @brief need finished
/// @param ouput
void CYSATScpiParse::Set_CONFigure_LOG_ENABLE(std::string& ouput)
{
  // VLE_LOGE(CONTROL_LEVEL_INFO, "CYSATScpiParse: Get_SYSTem_ERRor_NEXT()");
  char* penable = GetEnumPara();

  const char* able_list[] = {"OFF", "ON"};
  int         index       = CheckEnumPara(penable, able_list, 2);
  if (index < 0) {
    // VLE_LOGE(CONTROL_LEVEL_ERROR,"Get Para 'State_index' out of range,%d",State_index);
    index = 0;
  }
  srsran::console("Set_CONFigure_LOG_ENABLE enable= %d\n", index);
  if (index > 0) {
    ptENB->set_logger_enable(true);
  } else {
    ptENB->set_logger_enable(false);
  }
}
void CYSATScpiParse::Get_CONFigure_LOG_ENABLE(std::string& ouput)
{
  srsran::console("CYSATScpiParse: Get_CONFigure_LOG_ENABLE\n");
}
void CYSATScpiParse::Set_BEAM_Common_BEAM(std::string& ouput)
{
  int nbeam = GetIntPara();

  if (CheckIntPara(nbeam, 1, 2) < 0) {
    nbeam = 1;
  }
  srsran::console("CYSATScpiParse: Set_BEAM_Common_BEAM:%d\n", nbeam);
}
void CYSATScpiParse::Get_BEAM_Common_BEAM(std::string& ouput) {}

void CYSATScpiParse::Set_BEAM_Common_DUPMode(std::string& ouput)
{
  char* mode = GetEnumPara();

  const char* mode_list[] = {"FDD"};
  int         index       = CheckEnumPara(mode, mode_list, sizeof(mode_list) / sizeof(const char*));
  if (index < 0) {
    // VLE_LOGE(CONTROL_LEVEL_ERROR,"Get Para 'State_index' out of range,%d",State_index);
    index = 0;
  }
  srsran::console("Set_BEAM_Common_DUPMode mode= %d\n", index);
}
void CYSATScpiParse::Get_BEAM_Common_DUPMode(std::string& ouput) {}

void CYSATScpiParse::Set_BEAM_Common_Band(std::string& ouput)
{
  const int   sz                = 55;
  const int   col               = 8;
  char        str_list[sz][col] = {0};
  const char* check_list[sz];
  for (int i = 1; i <= sz; i++) {
    int n              = snprintf(str_list[i - 1], col, "n%d", i);
    str_list[i - 1][n] = '\0';
    check_list[i - 1]  = str_list[i - 1];
  }
  char* band  = GetEnumPara();
  int   index = CheckEnumPara(band, check_list, sz);
  if (index < 0) {
    // VLE_LOGE(CONTROL_LEVEL_ERROR,"Get Para 'State_index' out of range,%d",State_index);
    index = 0;
    srsran::console("Set_BEAM_Common_Band target mode= %s\n", band);
    // for (int i = 1; i <= sz; i++) {
    //   srsran::console("Set_BEAM_Common_Band mode[i]= %s\n", check_list[i - 1]);
    // }
  }
  srsran::console("Set_BEAM_Common_Band mode= %d\n", index + 1);
}
void CYSATScpiParse::Get_BEAM_Common_Band(std::string& ouput) {}

void CYSATScpiParse::Set_BEAM_Common_Network(std::string& ouput)
{
  // CONFig:BEAM:COMmon:NETWORK <nNetwork> AN/IOT
  char* mode = GetEnumPara();

  const char* access_list[] = {"AN", "IOT"};
  int         index         = CheckEnumPara(mode, access_list, sizeof(access_list) / sizeof(const char*));
  if (index < 0) {
    // VLE_LOGE(CONTROL_LEVEL_ERROR,"Get Para 'State_index' out of range,%d",State_index);
    index = 0;
  }
  srsran::console("Set_BEAM_Common_Network net= %d\n", index);
  if (p_g_args_ != nullptr) {
    // ptENB->set_network_mode(index);
    p_g_args_->enb.network_mode = index;
  }
  return;
}
void CYSATScpiParse::Get_BEAM_Common_Network(std::string& ouput) {}

void CYSATScpiParse::Set_BEAM_Common_Mode(std::string& ouput)
{
  // CONFig:BEAM:COMmon:MODE <nMode> NORMal/SPREAD
  char* mode = GetEnumPara();

  const char* mode_list[] = {"NORMal", "SPREAD"};
  int         index       = CheckEnumPara(mode, mode_list, sizeof(mode_list) / sizeof(const char*));
  if (index < 0) {
    // VLE_LOGE(CONTROL_LEVEL_ERROR,"Get Para 'State_index' out of range,%d",State_index);
    index = 0;
  }
  if (p_g_args_ != nullptr) {
    // ptENB->set_network_mode(index);
    p_g_args_->enb.area_mode = index;
  }

  srsran::console("Set_BEAM_Common_Mode net= %d\n", index);
}
void CYSATScpiParse::Get_BEAM_Common_Mode(std::string& ouput) {}

void CYSATScpiParse::Set_BEAM_IMMediate(std::string& ouput)
{
  // INITiate:BEAM:IMMediate OFF/ON

  char* state = GetEnumPara();

  const char* state_list[] = {"OFF", "ON"};
  int         idx          = CheckEnumPara(state, state_list, sizeof(state_list) / sizeof(const char*));
  if (idx < 0) {
    idx = 0;
  }
  srsran::console("Set_BEAM_IMMediate state= %d\n", idx);
  if (idx == 0) {
    if (ptENB != nullptr) {
      ptENB->stop();
      srsran::console("Set_BEAM_IMMediate state1= %d\n", idx);
    }
  } else {
    if (ptENB != nullptr) {
      ptENB->init(*p_g_args_,ptADP);
      srsran::console("Set_BEAM_IMMediate state0= %d\n", idx);
    }
  }
}
void CYSATScpiParse::Get_BEAM_IMMediate(std::string& ouput)
{
  int state = 0;
  if (ptENB != nullptr) {
    state = ptENB->get_beam_state();
  }
  ouput.assign(to_string(state));
}

void CYSATScpiParse::Set_UDP_Remote(std::string& ouput)
{
  srslog::sink& sink    = srslog::get_default_sink();
  cy_udp_sink*  udpSink = dynamic_cast<cy_udp_sink*>(&sink);
  char*         strPara = GetStringPara();

  if (udpSink) {
    std::string ipcfg(strPara);
    std::cout << "get ipcfg:" << ipcfg << std::endl;
    auto        pos   = ipcfg.find(':');
    int         uport = 0;
    std::string ip{};
    if (pos > 0) {
      std::string ip   = ipcfg.substr(0, pos);
      std::string port = ipcfg.substr(pos + 1, ipcfg.size());
      uport            = atoi(port.c_str());

    } else {
      ip    = ipcfg;
      uport = 6666;
    }

    std::cout << "get ip:" << ip << " port:" << uport << std::endl;
    // VLE_LOGE(CONTROL_LEVEL_INFO,"Set_UDP_Remote:%s, port %d", ip, port);
    udpSink->reinit(ip, static_cast<uint16_t>(uport));
  } else {
    srsran::console("CYSATScpiParse: Set_UDP_Remote get sink invalid.");
  }
}
void CYSATScpiParse::Get_UDP_Remote(std::string& ouput) {}
void CYSATScpiParse::Set_PCAP_Remote(std::string& output)
{
  char* strPara = GetStringPara();
  // VLE_LOGE(CONTROL_LEVEL_INFO,"CYSATScpiParse: Set_PCAP_Remote getpara:%s", strPara);

  std::string ipcfg(strPara);
  std::cout << "get ipcfg:" << ipcfg << std::endl;
  auto        pos   = ipcfg.find(':');
  int         uport = 0;
  std::string ip{};
  if (pos > 0) {
    ip               = ipcfg.substr(0, pos);
    std::string port = ipcfg.substr(pos + 1, ipcfg.size());
    uport            = atoi(port.c_str());
  } else {
    ip    = ipcfg;
    uport = 6666;
  }
  if (ptENB != nullptr) {
    ptENB->set_pcap_remote_addr(ip, uport);
  }

  std::cout << "get ip:" << ip << " port:" << uport << std::endl;
}
void CYSATScpiParse::Get_PCAP_Remote(std::string& output) {}
int  CYSATScpiParse::RemoteCommandProcess(char* CommandBuf, std::string& output)
{
  int ret = CYICScpiParse<CYSATScpiParse>::RemoteCommandProcess(CommandBuf, output);
  return ret;
}
