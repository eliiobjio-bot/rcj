#ifndef CEYEAR_SAT_SCPI_PARSE
#define CEYEAR_SAT_SCPI_PARSE
#include <memory>
#include <string>

// #include "enb.h"
#include "srsenb/hdr/enb.h"
#include "srsran/common/cy_errorcode.h"
#include "srsran/scpi/cy_scpiparse.h"

class CYSATScpiParse;
using SATCOMM_NODE = SCPI_NODE<CYSATScpiParse>;

//using WBTS_SuffixPara = SCPISuffixPara;

class CYSATScpiParse: public CYICScpiParse<CYSATScpiParse>
{

public:
  CYSATScpiParse(srsenb::enb* pENB, srsenb::all_args_t* arg);
  CYSATScpiParse() = delete;
  ~CYSATScpiParse();
  USING_PARSE_FUNC();
  void Set_CLS(std::string& ouput);
  void Get_IDN(std::string& ouput);
  void Set_OPC(std::string& ouput);
  void Set_RST(std::string& ouput);
  void Set_DEBUG(std::string& ouput);
  void Set_SYSTem_ERRor_CLEar_ALL(std::string& ouput);
  void Get_SYSTem_ERRor_NEXT(std::string& ouput);
  int  ResetMeasResult() override { return 0; };
  void Set_UDP_Remote(std::string& ouput);
  void Get_UDP_Remote(std::string&);
  void Set_PCAP_Remote(std::string&);
  void Get_PCAP_Remote(std::string&);

  void Set_CONFigure_LOG_ENABLE(std::string& ouput);
  void Get_CONFigure_LOG_ENABLE(std::string& ouput);

  void Set_BEAM_Common_BEAM(std::string& ouput);
  void Get_BEAM_Common_BEAM(std::string& ouput);

  void Set_BEAM_Common_DUPMode(std::string& ouput);
  void Get_BEAM_Common_DUPMode(std::string& ouput);

  void Set_BEAM_Common_Band(std::string& ouput);
  void Get_BEAM_Common_Band(std::string& ouput);

  void Set_BEAM_Common_Network(std::string& ouput);
  void Get_BEAM_Common_Network(std::string& ouput);

  void Set_BEAM_Common_Mode(std::string& ouput);
  void Get_BEAM_Common_Mode(std::string& ouput);

  void Set_BEAM_IMMediate(std::string& ouput);
  void Get_BEAM_IMMediate(std::string& ouput);

public:

	int RemoteCommandProcess(char *CommandBuf,std::string &output) override;

private:
  srsenb::enb*        ptENB     = nullptr;
  srsenb::all_args_t* p_g_args_ = nullptr;
  srsenb::adp*ptADP=nullptr;
};


#endif
