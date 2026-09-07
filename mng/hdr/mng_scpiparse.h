#ifndef CEYEAR_SAT_SCPI_PARSE
#define CEYEAR_SAT_SCPI_PARSE
#include <memory>
#include <string>
#include <unordered_map>
// #include "enb.h"
// #include "srsenb/hdr/enb.h"
#include "inicpp.h"
#include "init.h"
#include "srsran/common/cy_errorcode.h"
#include "srsran/common/string_helpers.h"
#include "srsran/scpi/cy_scpiparse.h"
#include <libconfig.h++>
using namespace libconfig;
typedef enum { AN = 0,
               IOT = 1 } net_mode_e;
typedef enum {
    EBAND = 0,
    EFREQNO = 1,
    ECHANNELTYPE = 2,
    ESLOT = 3,
    EMCS = 4,
    ESCHETYPE = 5,
} carrier_mode_e;
typedef enum { UL = 0,
               DL = 1 } spread_direction;
typedef enum {
    NORM,
    SPREAD
} run_mode;
class CYSATScpiParse;
using SATCOMM_NODE = SCPI_NODE< CYSATScpiParse >;

class CYSATScpiParse : public CYICScpiParse< CYSATScpiParse > {
public:
    CYSATScpiParse(const all_args_t& args);
    ~CYSATScpiParse();
    USING_PARSE_FUNC();
    void Set_CLS(std::string& ouput);
    void Get_IDN(std::string& ouput);
    void Set_OPC(std::string& ouput);
    void Set_RST(std::string& ouput);
    void Set_DEBUG(std::string& ouput);
    void Set_IPERF(std::string& output);
    int  ResetMeasResult() override
    {
        return 0;
    };
    void Set_UDP_Remote(std::string& ouput);
    void Get_UDP_Remote(std::string&);

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
    void Set_BEAM_Mode(std::string& ouput);

    void Set_BEAM_IMMediate(std::string& ouput);
    void Get_BEAM_IMMediate(std::string& ouput);
    void Set_INIT_IMMediate_SRSCNW(std::string& output);
    void Set_CONFigure_BEAM_LOOP(std::string& output);
    void Set_CONFigure_BEAM_FRAMEOFFSet(std::string& output);
    void Set_CONFigure_BEAM_MULTICARRier_TransMode(std::string& output);
    void Set_BEAM_DISTance(std::string& output);
    void Set_CONFigure_ENB_PARA(std::string& ouput);
    void st_CONFigure_FUNCtion_BEAM_MIB_BEAMID(std::string& ouput);
    void st_CONFigure_FUNCtion_BEAM_MIB_FREQID(std::string& ouput);
    void st_CONFigure_FUNCtion_BEAM_MIB_BANDID(std::string& ouput);
    void st_CONFigure_FUNCtion_BEAM_MIB_SSLOT(std::string& ouput);
    void st_CONFigure_FUNCtion_BEAM_SIB_MCC(std::string& ouput);
    void st_CONFigure_FUNCtion_BEAM_SIB_MNC(std::string& ouput);
    void st_CONFigure_FUNCtion_BEAM_SIB_SMINLEVel(std::string& output);
    void st_CONFigure_FUNCtion_BEAM_SIB_RMINLEVel(std::string& output);
    void st_CONFigure_FUNCtion_BEAM_SIB_THREshold(std::string& output);
    void st_CONFigure_FUNCtion_BEAM_SIB_OFFSet(std::string& output);
    void st_CONFigure_FUNCtion_BEAM_SIB_BEAMRSELect(std::string& output);
    void st_CONFigure_FUNCtion_BEAM_SIB_POWer(std::string& output);
    void st_CONFigure_PROTocol_BEAM_BEAMRSELect_POWer(std::string& output);
    void st_CONFigure_PROTocol_BEAM_BEAMSWITch_POWer(std::string& output);
    void Set_Config_Resource_Beam_Rach_Band(std::string& output);
    void Set_Config_Resource_Beam_Rach_FreqNO(std::string& output);
    void Set_Config_Resource_Beam_Rach_Frame(std::string& output);
    void Set_Config_Resource_Beam_Rach_SubfreqNO(std::string& output);
    void Set_Config_Resource_Beam_Sch(std::string& output);
    void st_CONFigure_TTCN_DELay(std::string& output);
    void st_CONFigure_BEAM_AN_NORM_MULTICARRier_NUM(std::string& output);
    void st_CONFigure_BEAM_AN_SPREAD_MULTICARRier_NUM(std::string& output);
    void st_CONFigure_BEAM_AN_NORM_MULTICARRier_TYPE(std::string& output);
    void st_CONFigure_BEAM_AN_NORM_MULTICARRier_BAND(std::string& output);
    void st_CONFigure_BEAM_AN_NORM_MULTICARRier_FREQNO(std::string& output);
    void st_CONFigure_BEAM_AN_NORM_MULTICARRier_SLOT(std::string& output);
    void st_CONFigure_BEAM_AN_NORM_MULTICARRier_MCS(std::string& output);
    void st_CONFigure_BEAM_AN_SPREAD_DL_MULTICARRier_TYPE(std::string& output);
    void st_CONFigure_BEAM_AN_SPREAD_DL_MULTICARRier_BAND(std::string& output);
    void st_CONFigure_BEAM_AN_SPREAD_DL_MULTICARRier_FREQNO(std::string& output);
    void st_CONFigure_BEAM_AN_SPREAD_DL_MULTICARRier_SLOT(std::string& output);
    void st_CONFigure_BEAM_AN_SPREAD_UL_MULTICARRier_TYPE(std::string& output);
    void st_CONFigure_BEAM_AN_SPREAD_UL_MULTICARRier_BAND(std::string& output);
    void st_CONFigure_BEAM_AN_SPREAD_UL_MULTICARRier_FREQNO(std::string& output);
    void st_CONFigure_BEAM_AN_SPREAD_UL_MULTICARRier_SLOT(std::string& output);
    void st_CONFigure_BEAM_AN_SPREAD_UL_MULTICARRier_MCS(std::string& output);
    // ADD FOR IOT
    void st_CONFigure_BEAM_IOT_NORM_MULTICARRier_NUM(std::string& output);
    void st_CONFigure_BEAM_IOT_SPREAD_MULTICARRier_NUM(std::string& output);
    void st_CONFigure_BEAM_IOT_NORM_DL_MULTICARRier_TYPE(std::string& output);
    void st_CONFigure_BEAM_IOT_NORM_DL_MULTICARRier_SCHETYPE(std::string& output);
    void st_CONFigure_BEAM_IOT_NORM_DL_MULTICARRier_BAND(std::string& output);
    void st_CONFigure_BEAM_IOT_NORM_DL_MULTICARRier_FREQNO(std::string& output);
    void st_CONFigure_BEAM_IOT_NORM_UL_MULTICARRier_TYPE(std::string& output);
    void st_CONFigure_BEAM_IOT_NORM_UL_MULTICARRier_SCHETYPE(std::string& output);
    void st_CONFigure_BEAM_IOT_NORM_UL_MULTICARRier_BAND(std::string& output);
    void st_CONFigure_BEAM_IOT_NORM_UL_MULTICARRier_FREQNO(std::string& output);
    void st_CONFigure_BEAM_IOT_SPREAD_DL_MULTICARRier_TYPE(std::string& output);
    void st_CONFigure_BEAM_IOT_SPREAD_DL_MULTICARRier_SCHETYPE(std::string& output);
    void st_CONFigure_BEAM_IOT_SPREAD_DL_MULTICARRier_BAND(std::string& output);
    void st_CONFigure_BEAM_IOT_SPREAD_DL_MULTICARRier_FREQNO(std::string& output);
    void st_CONFigure_BEAM_IOT_SPREAD_UL_MULTICARRier_TYPE(std::string& output);
    void st_CONFigure_BEAM_IOT_SPREAD_UL_MULTICARRier_SCHETYPE(std::string& output);
    void st_CONFigure_BEAM_IOT_SPREAD_UL_MULTICARRier_BAND(std::string& output);
    void st_CONFigure_BEAM_IOT_SPREAD_UL_MULTICARRier_FREQNO(std::string& output);
    void st_CONFigure_BEAM_IOT_SPREAD_FACTOR(std::string& output);
    // add by wwy for fetch
    void st_FETCh_BEAM_IOT_RANPID(std::string& output);
    void st_FETCh_BEAM_AN_RANPID(std::string& output);

    std::string                MapCarrierToKeyName(std::string bussinesstype, int carriernum, int type);
    std::string                MapSpreadToKeyName(std::string bussinesstype, int type, int direction);
    std::string                MapiotCarrierToKeyName(int mode, int carriernum, int type, int direction);
    std::vector< std::string > GenerateWriteFileParas(std::string bussinesstype, int carriernum, int beamindex, int type, std::string valstr);
    std::vector< std::string > GenSpreadWriteFileParas(std::string bussinesstype, int beamindex, int type, int dir, std::string valstr);
    std::vector< std::string > GeniotWriteFileParas(int mode, int carriernum, int beamindex, int type, int direction, std::string valstr);
    void                       ParseConfigFile(const std::vector< std::string >& vs);
    void                       ConvetConfigValue(Setting& Setting, const std::string& val);
    void                       ParseINIConfig(const std::vector< std::string >& vs);
    std::vector< std::vector< std::string > >
                       ParseParameter(std::string const& s, int idx, std::string const& val, std::string const& type);
    void               ReadDefaultParas(std::vector< std::vector< std::string > >& vvs);
    std::vector< int > GetChannelTypeRange(std::string, int);

public:
    int RemoteCommandProcess(char* CommandBuf, std::string& output) override;

private:
    // srsenb::enb*        ptENB     = nullptr;
    //  srsenb::all_args_t* p_g_args_ = nullptr;
    std::unordered_map< int, int >         beam_status = { { 0, 0 }, { 1, 0 }, { 2, 0 }, { 3, 0 } };
    std::unordered_map< int, std::string > beam_mode_map{ { 1, "IOT" }, { 2, "IOT" } };
    std::string                            conf_path{};
    std::vector< pid_t >                   vpid{};
};

#endif
