#include <cassert>
#include <iostream>
#include <stdlib.h>
#include <string>
// #include "cy_log.h"
#include "../hdr/mng_scpiparse.h"
#include "srsran/common/cy_log.h"
// #include "cy_udp_socket.h"
#include "../hdr/init.h"
#include "lib/include/srsran/srslog/srslog.h"
#include "sinks/cy_udp_sink.h"
#include "srsran/common/standard_streams.h"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <srsran/common/bcd_helpers.h>
#include <unordered_map>
using namespace srslog;
using namespace std;
using namespace libconfig;

static unordered_multimap< string, vector< string > > mmp{
    // mib config
    { "mib.beamid", { "/access", "/mib1.conf", "mib.beam_id" } },
    { "mib.beamid", { "/IOT", "/mib.conf", "mib.beam_id" } },
    { "mib.freqid", { "/access", "/mib1.conf", "mib.bcch_fre_id" } },
    { "mib.freqid", { "/IOT", "/mib.conf", "mib.bcch_fre_id" } },
    { "mib.bandid", { "/access", "/mib1.conf", "mib.bcch_band_id" } },
    { "mib.bandid", { "/IOT", "/mib.conf", "mib.bcch_band_id" } },
    { "mib.sslot", { "/access", "/mib1.conf", "mib.bcch_slot_start" } },
    { "mib.sslot", { "/IOT", "/mib.conf", "mib.bcch_slot_start" } },
    { "mib.frameoffset", { "/access", "/mib1.conf", "mib.frame_off" } },
    { "mib.frameoffset", { "/IOT", "/mib.conf", "mib.frame_off" } },
    { "mib.distance", { "/access", "/mib1.conf", "mib.d_to_beam_center" } },
    { "mib.distance", { "/IOT", "/mib.conf", "mib.d_to_beam_center" } },
    { "multi.transmode", { "/access", "/enb1.conf", "enb.get_rlc_mode" } },

    // sib config
    { "sib.mcc", { "/access", "/enb1.conf", "enb.mcc" } },
    { "sib.mcc", { "/IOT", "/enb.conf", "enb.mcc" } },
    { "sib.mnc", { "/access", "/enb1.conf", "enb.mnc" } },
    { "sib.mnc", { "/IOT", "/enb.conf", "enb.mnc" } },
    { "ttcn.delay", { "/access", "/enb1.conf", "enb.ttcn_time" } },
    { "ttcn.delay", { "/IOT", "/enb.conf", "enb.ttcn_time" } },
    { "beam.mode", { "/access", "/enb1.conf", "enb.area_mode" } },
    { "beam.mode", { "/IOT", "/enb.conf", "enb.area_mode" } },
    { "sib.sminlevel", { "/access", "/sib_wx1.conf", "sib.resel_q_min" } },
    { "sib.sminlevel", { "/IOT", "/sib_wx.conf", "sib.resel_q_min" } },
    { "sib.rminlevel", { "/access", "/sib_wx1.conf", "sib.q_min" } },
    { "sib.rminlevel", { "/IOT", "/sib_wx.conf", "sib.q_min" } },
    { "sib.threshold", { "/access", "/sib_wx1.conf", "sib.threshold-resel" } },
    { "sib.threshold", { "/IOT", "/sib_wx.conf", "sib.threshold-resel" } },
    { "sib.offset", { "/access", "/sib_wx1.conf", "sib.offset" } },
    { "sib.offset", { "/IOT", "/sib_wx.conf", "sib.offset" } },
    { "sib.bearmselect", { "/access", "/sib_wx1.conf", "sib.t_resel" } },
    { "sib.bearmselect", { "/IOT", "/sib_wx.conf", "sib.t_resel" } },
    { "norm.num", { "/access", "/enb1.conf", "enb.multi_beam_num" } },
    { "iot.carriernum", { "/IOT", "/enb.conf", "enb.multi_beam_num" } },
    // enb config
    { "spread.factor", { "/IOT", "/enb.conf", "scheduler.iot_freq_spread" } },

};
static unordered_map< int, string > mp{
    { 1, "/beam1" },
    { 2, "/beam2" },
};
static unordered_map< string, string > networkmap = { { "AN", "/access" }, { "IOT", "/IOT" } };
static unordered_map< string, string > sibmap = { { "IOT", "/sib_wx.conf" }, { "AN", "/sib_wx1.conf" } };
static unordered_map< string, string > recfgmap = { { "IOT", "/enb.conf" }, { "AN", "/recfg_wx.conf" } };
static unordered_map< string, string > normrecfgfilemap = { { "DATA", "/recfg_wx.conf" }, { "VOICE", "/recfg_voice.conf" } };
static unordered_map< string, string > spreadrecfgfilemap = { { "DATA", "/recfg_norm_kuopin.conf" }, { "VOICE", "/recfg_voice_kuopin.conf" } };

static unordered_map< string, string > normtypemp = {
    { "PDCH1_1", "pDCH11" },
    { "PDCH1_2", "pDCH12" },
    { "PSCH1_1", "pSCH11" },
    { "PSCH1_2", "pSCH12" },
    { "PSCH5_1", "pSCH51" },
    { "PSCH5_2", "pSCH52" },
};
/////
static unordered_map< string, string > spreadDLtypemp = {
    { "DS_PDTCH_1", "dSPDTCH1" },
    { "DS_PDTCH_2", "dSPDTCH2" },
    { "DS_PDTCH_3", "dSPDTCH3" },
    { "DS_PDTCH_T", "dSPDTCHT" },
};
static unordered_map< string, string > iotnormtype = {
    { "PDTCH", "pDTCH" },
    { "PUTCH", "pUTCH" },
};
static std::vector< string >        nor_channeltype{};
static std::string                  spread_ul_channeltype = "PDCH1_1";
static std::string                  spread_dl_channeltype = "DS_PDTCH_1";
static unordered_map< string, int > MSCtoValmp = {
    { "1_2_QPSK", 0 }, { "2_3_QPSK", 1 }, { "4_5_QPSK", 2 }, { "7_10_QPSK", 3 }, { "3_5_8PSK", 4 }
};
static unordered_map< string, std::vector< int > > channeltypetomsc_mp = {

    { "PDCH1_1", { MSCtoValmp["1_2_QPSK"] } },
    { "PDCH1_2", { MSCtoValmp["1_2_QPSK"] } },
    { "PSCH1_1", { MSCtoValmp["1_2_QPSK"], MSCtoValmp["2_3_QPSK"], MSCtoValmp["4_5_QPSK"] } },
    { "PSCH1_2", { MSCtoValmp["1_2_QPSK"], MSCtoValmp["2_3_QPSK"], MSCtoValmp["4_5_QPSK"] } },
    { "PSCH5_1", { MSCtoValmp["1_2_QPSK"], MSCtoValmp["7_10_QPSK"], MSCtoValmp["3_5_8PSK"] } },
    { "PSCH5_2", { MSCtoValmp["1_2_QPSK"], MSCtoValmp["7_10_QPSK"], MSCtoValmp["3_5_8PSK"] } }
};

vector< vector< string > > CYSATScpiParse::ParseParameter(string const& s, int idx, string const& val, string const& type)
{
    vector< vector< string > > vec;
    auto                       range = mmp.equal_range(s);
    for (auto it = range.first; it != range.second; ++it)
    {
        vector< string > tmp{ it->second[0] + mp[idx] + it->second[1], it->second[2], val, type };
        vec.push_back(tmp);
    }
    return vec;
}

#define NEW_BEAM_IMM

#if 1
static SATCOMM_NODE st_CONFigure_REMOte_LOG[2] = {
    { "UDP", "UDP", 0, nullptr, &CYSATScpiParse::Set_UDP_Remote, &CYSATScpiParse::Get_UDP_Remote },
    { "", "", 0, nullptr, nullptr, nullptr }
};

static SATCOMM_NODE st_CONFigure_LOG[] = { { "REMO", "REMOte", 1, st_CONFigure_REMOte_LOG, 0, 0 },
                                           { "ENAB",
                                             "ENABle",
                                             0,
                                             nullptr,
                                             &CYSATScpiParse::Set_CONFigure_LOG_ENABLE,
                                             &CYSATScpiParse::Get_CONFigure_LOG_ENABLE },
                                           { "", "", 0, nullptr, 0, 0 } };

static SATCOMM_NODE st_CONFigure_BEAM_COMM[] = {
    { "BEAM", "BEAM", 0, nullptr, &CYSATScpiParse::Set_BEAM_Common_BEAM, &CYSATScpiParse::Get_BEAM_Common_BEAM },
    { "DUPM", "DUPMode", 0, nullptr, &CYSATScpiParse::Set_BEAM_Common_DUPMode, &CYSATScpiParse::Get_BEAM_Common_DUPMode },
    { "BAND", "BAND", 0, nullptr, &CYSATScpiParse::Set_BEAM_Common_Band, &CYSATScpiParse::Get_BEAM_Common_Band },
    { "NETWORK", "NETWORK", 0, nullptr, &CYSATScpiParse::Set_BEAM_Common_Network, &CYSATScpiParse::Get_BEAM_Common_Network },
    { "MODE", "MODE", 0, nullptr, &CYSATScpiParse::Set_BEAM_Common_Mode, &CYSATScpiParse::Get_BEAM_Common_Mode },
    { "", "", 0, 0, 0, 0 }
};
static SATCOMM_NODE st_CONFigure_BEAM_AN_NORM_MULTICARRier[] = {
    { "NUM", "NUM", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_AN_NORM_MULTICARRier_NUM, nullptr },
    { "TYPE", "TYPE", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_AN_NORM_MULTICARRier_TYPE, nullptr },
    { "BAND", "BAND", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_AN_NORM_MULTICARRier_BAND, nullptr },
    { "FREQNO", "FREQNO", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_AN_NORM_MULTICARRier_FREQNO, nullptr },
    { "SLOT", "SLOT", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_AN_NORM_MULTICARRier_SLOT, nullptr },
    { "MCS", "MCS", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_AN_NORM_MULTICARRier_MCS, nullptr },
    { "", "", 0, 0, 0, 0 }

};
static SATCOMM_NODE st_CONFigure_BEAM_AN_NORM[] = {
    { "MULTICARRier", "MULTICARRier<carrier>", sizeof(st_CONFigure_BEAM_AN_NORM_MULTICARRier) / sizeof(SATCOMM_NODE), st_CONFigure_BEAM_AN_NORM_MULTICARRier, nullptr, nullptr },
    { "", "", 0, 0, 0, 0 }

};
static SATCOMM_NODE st_CONFigure_BEAM_AN_SPREAD_DL_MULTICARRier[] = {
    { "TYPE", "TYPE", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_AN_SPREAD_DL_MULTICARRier_TYPE, nullptr },
    { "BAND", "BAND", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_AN_SPREAD_DL_MULTICARRier_BAND, nullptr },
    { "FREQNO", "FREQNO", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_AN_SPREAD_DL_MULTICARRier_FREQNO, nullptr },
    { "SLOT", "SLOT", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_AN_SPREAD_DL_MULTICARRier_SLOT, nullptr },
    { "", "", 0, 0, 0, 0 }
};
static SATCOMM_NODE st_CONFigure_BEAM_AN_SPREAD_UL_MULTICARRier[] = {
    { "TYPE", "TYPE", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_AN_SPREAD_UL_MULTICARRier_TYPE, nullptr },
    { "BAND", "BAND", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_AN_SPREAD_UL_MULTICARRier_BAND, nullptr },
    { "FREQNO", "FREQNO", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_AN_SPREAD_UL_MULTICARRier_FREQNO, nullptr },
    { "SLOT", "SLOT", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_AN_SPREAD_UL_MULTICARRier_SLOT, nullptr },
    { "MCS", "MCS", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_AN_SPREAD_UL_MULTICARRier_MCS, nullptr },
    { "", "", 0, 0, 0, 0 }
};
static SATCOMM_NODE st_CONFigure_BEAM_AN_SPREAD_DL[] = {
    { "MULTICARRier", "MULTICARRier<carrier>", sizeof(st_CONFigure_BEAM_AN_SPREAD_DL_MULTICARRier) / sizeof(SATCOMM_NODE), st_CONFigure_BEAM_AN_SPREAD_DL_MULTICARRier, nullptr, nullptr },
    { "", "", 0, 0, 0, 0 }
};
static SATCOMM_NODE st_CONFigure_BEAM_AN_SPREAD_UL[] = {
    { "MULTICARRier", "MULTICARRier<carrier>", sizeof(st_CONFigure_BEAM_AN_SPREAD_UL_MULTICARRier) / sizeof(SATCOMM_NODE), st_CONFigure_BEAM_AN_SPREAD_UL_MULTICARRier, nullptr, nullptr },
    { "", "", 0, 0, 0, 0 }
};

static SATCOMM_NODE st_CONFigure_BEAM_AN_SPREAD_MULTICARRier[] = {
    { "NUM", "NUM", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_AN_SPREAD_MULTICARRier_NUM, nullptr },
    { "", "", 0, 0, 0, 0 }
};
static SATCOMM_NODE st_CONFigure_BEAM_AN_SPREAD[] = {
    { "DL", "DL", sizeof(st_CONFigure_BEAM_AN_SPREAD_DL) / sizeof(SATCOMM_NODE), st_CONFigure_BEAM_AN_SPREAD_DL, nullptr, nullptr },
    { "UL", "UL", sizeof(st_CONFigure_BEAM_AN_SPREAD_UL) / sizeof(SATCOMM_NODE), st_CONFigure_BEAM_AN_SPREAD_UL, nullptr, nullptr },
    { "MULTICARRier", "MULTICARRier<carrier>", sizeof(st_CONFigure_BEAM_AN_SPREAD_MULTICARRier) / sizeof(SATCOMM_NODE), st_CONFigure_BEAM_AN_SPREAD_MULTICARRier, nullptr, nullptr },
    { "", "", 0, 0, 0, 0 }
};
static SATCOMM_NODE st_CONFigure_BEAM_AN[] = {
    { "NORM", "NORM", sizeof(st_CONFigure_BEAM_AN_NORM) / sizeof(SATCOMM_NODE), st_CONFigure_BEAM_AN_NORM, nullptr, nullptr },
    { "SPREAD", "SPREAD", sizeof(st_CONFigure_BEAM_AN_SPREAD) / sizeof(SATCOMM_NODE), st_CONFigure_BEAM_AN_SPREAD, nullptr, nullptr },
    { "", "", 0, 0, 0, 0 }
};

static SATCOMM_NODE st_CONFigure_BEAM_IOT_NORM_MULTICARRier[] = {
    { "NUM", "NUM", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_IOT_NORM_MULTICARRier_NUM, nullptr },
    { "", "", 0, 0, 0, 0 }
};
static SATCOMM_NODE st_CONFigure_BEAM_IOT_SPREAD_MULTICARRier[] = {
    { "NUM", "NUM", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_IOT_SPREAD_MULTICARRier_NUM, nullptr },
    { "", "", 0, 0, 0, 0 }
};
static SATCOMM_NODE st_CONFigure_BEAM_IOT_NORM_DL_MULTICARRier[] = {
    { "TYPE", "TYPE", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_IOT_NORM_DL_MULTICARRier_TYPE, nullptr },
    { "SCHETYPE", "SCHETYPE", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_IOT_NORM_DL_MULTICARRier_SCHETYPE, nullptr },
    { "BAND", "BAND", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_IOT_NORM_DL_MULTICARRier_BAND, nullptr },
    { "FREQNO", "FREQNO", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_IOT_NORM_UL_MULTICARRier_FREQNO, nullptr },
    { "", "", 0, 0, 0, 0 }

};
static SATCOMM_NODE st_CONFigure_BEAM_IOT_NORM_UL_MULTICARRier[] = {
    { "TYPE", "TYPE", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_IOT_NORM_UL_MULTICARRier_TYPE, nullptr },
    { "SCHETYPE", "SCHETYPE", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_IOT_NORM_UL_MULTICARRier_SCHETYPE, nullptr },
    { "BAND", "BAND", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_IOT_NORM_UL_MULTICARRier_BAND, nullptr },
    { "FREQNO", "FREQNO", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_IOT_NORM_UL_MULTICARRier_FREQNO, nullptr },
    { "", "", 0, 0, 0, 0 }

};
static SATCOMM_NODE st_CONFigure_BEAM_IOT_SPREAD_DL_MULTICARRier[] = {
    { "TYPE", "TYPE", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_IOT_SPREAD_DL_MULTICARRier_TYPE, nullptr },
    { "SCHETYPE", "SCHETYPE", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_IOT_SPREAD_DL_MULTICARRier_SCHETYPE, nullptr },
    { "BAND", "BAND", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_IOT_SPREAD_DL_MULTICARRier_BAND, nullptr },
    { "FREQNO", "FREQNO", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_IOT_SPREAD_DL_MULTICARRier_FREQNO, nullptr },
    { "", "", 0, 0, 0, 0 }

};
static SATCOMM_NODE st_CONFigure_BEAM_IOT_SPREAD_UL_MULTICARRier[] = {
    { "TYPE", "TYPE", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_IOT_SPREAD_UL_MULTICARRier_TYPE, nullptr },
    { "SCHETYPE", "SCHETYPE", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_IOT_SPREAD_UL_MULTICARRier_SCHETYPE, nullptr },
    { "BAND", "BAND", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_IOT_SPREAD_UL_MULTICARRier_BAND, nullptr },
    { "FREQNO", "FREQNO", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_IOT_SPREAD_UL_MULTICARRier_FREQNO, nullptr },
    { "", "", 0, 0, 0, 0 }

};
static SATCOMM_NODE st_CONFigure_BEAM_IOT_NORM_DL[] = {
    { "MULTICARRier", "MULTICARRier<carrier>", sizeof(st_CONFigure_BEAM_IOT_NORM_DL_MULTICARRier) / sizeof(SATCOMM_NODE), st_CONFigure_BEAM_IOT_NORM_DL_MULTICARRier, nullptr, nullptr },
    { "", "", 0, 0, 0, 0 }
};
static SATCOMM_NODE st_CONFigure_BEAM_IOT_NORM_UL[] = {
    { "MULTICARRier", "MULTICARRier<carrier>", sizeof(st_CONFigure_BEAM_IOT_NORM_UL_MULTICARRier) / sizeof(SATCOMM_NODE), st_CONFigure_BEAM_IOT_NORM_UL_MULTICARRier, nullptr, nullptr },
    { "", "", 0, 0, 0, 0 }
};

static SATCOMM_NODE st_CONFigure_BEAM_IOT_SPREAD_DL[] = {
    { "MULTICARRier", "MULTICARRier<carrier>", sizeof(st_CONFigure_BEAM_IOT_SPREAD_DL_MULTICARRier) / sizeof(SATCOMM_NODE), st_CONFigure_BEAM_IOT_SPREAD_DL_MULTICARRier, nullptr, nullptr },
    { "", "", 0, 0, 0, 0 }
};
static SATCOMM_NODE st_CONFigure_BEAM_IOT_SPREAD_UL[] = {
    { "MULTICARRier", "MULTICARRier<carrier>", sizeof(st_CONFigure_BEAM_IOT_SPREAD_UL_MULTICARRier) / sizeof(SATCOMM_NODE), st_CONFigure_BEAM_IOT_SPREAD_UL_MULTICARRier, nullptr, nullptr },
    { "", "", 0, 0, 0, 0 }
};
static SATCOMM_NODE st_CONFigure_BEAM_IOT_NORM[] = {
    { "MULTICARRier", "MULTICARRier<carrier>", sizeof(st_CONFigure_BEAM_IOT_NORM_MULTICARRier) / sizeof(SATCOMM_NODE), st_CONFigure_BEAM_IOT_NORM_MULTICARRier, nullptr, nullptr },
    { "DL", "DL", sizeof(st_CONFigure_BEAM_IOT_NORM_DL) / sizeof(SATCOMM_NODE), st_CONFigure_BEAM_IOT_NORM_DL, nullptr, nullptr },
    { "UL", "UL", sizeof(st_CONFigure_BEAM_IOT_NORM_UL) / sizeof(SATCOMM_NODE), st_CONFigure_BEAM_IOT_NORM_UL, nullptr, nullptr },
    { "", "", 0, 0, 0, 0 }
};
static SATCOMM_NODE st_CONFigure_BEAM_IOT_SPREAD[] = {
    { "MULTICARRier", "MULTICARRier<carrier>", sizeof(st_CONFigure_BEAM_IOT_SPREAD_MULTICARRier) / sizeof(SATCOMM_NODE), st_CONFigure_BEAM_IOT_SPREAD_MULTICARRier, nullptr, nullptr },
    { "DL", "DL", sizeof(st_CONFigure_BEAM_IOT_SPREAD_DL) / sizeof(SATCOMM_NODE), st_CONFigure_BEAM_IOT_SPREAD_DL, nullptr, nullptr },
    { "UL", "UL", sizeof(st_CONFigure_BEAM_IOT_SPREAD_UL) / sizeof(SATCOMM_NODE), st_CONFigure_BEAM_IOT_SPREAD_UL, nullptr, nullptr },
    { "FACTOR", "FACTOR", 0, nullptr, &CYSATScpiParse::st_CONFigure_BEAM_IOT_SPREAD_FACTOR, nullptr },
    { "", "", 0, 0, 0, 0 }
};
static SATCOMM_NODE st_CONFigure_BEAM_IOT[] = {
    { "NORM", "NORM", sizeof(st_CONFigure_BEAM_IOT_NORM) / sizeof(SATCOMM_NODE), st_CONFigure_BEAM_IOT_NORM, nullptr, nullptr },
    { "SPREAD", "SPREAD", sizeof(st_CONFigure_BEAM_IOT_SPREAD) / sizeof(SATCOMM_NODE), st_CONFigure_BEAM_IOT_SPREAD, nullptr, nullptr },
    { "", "", 0, 0, 0, 0 }
};
static SATCOMM_NODE st_CONFigure_BEAM_MULTICARRier[] = 
{
    { "TransMode", "TransMode", 0, nullptr, &CYSATScpiParse::Set_CONFigure_BEAM_MULTICARRier_TransMode, nullptr },
    { "", "", 0, 0, 0, 0 }
};
static SATCOMM_NODE st_CONFigure_BEAM[] = {
    { "COMM", "COMMon", sizeof(st_CONFigure_BEAM_COMM) / sizeof(SATCOMM_NODE), st_CONFigure_BEAM_COMM, nullptr, nullptr },
    { "AN", "AN", sizeof(st_CONFigure_BEAM_AN) / sizeof(SATCOMM_NODE), st_CONFigure_BEAM_AN, nullptr, nullptr },
    { "IOT", "IOT", sizeof(st_CONFigure_BEAM_IOT) / sizeof(SATCOMM_NODE), st_CONFigure_BEAM_IOT, nullptr, nullptr },
    { "LOOP", "LOOP", 0, nullptr, &CYSATScpiParse::Set_CONFigure_BEAM_LOOP, nullptr },
    { "MODE", "MODE", 0, nullptr, &CYSATScpiParse::Set_BEAM_Mode, nullptr },
    { "FRAMEOFFS", "FRAMEOFFSet", 0, nullptr, &CYSATScpiParse::Set_CONFigure_BEAM_FRAMEOFFSet, nullptr },
    { "DIST", "DISTance", 0, nullptr, &CYSATScpiParse::Set_BEAM_DISTance, nullptr },
    { "MULTICARRier", "MULTICARRier<carrier>", sizeof(st_CONFigure_BEAM_MULTICARRier) / sizeof(SATCOMM_NODE), st_CONFigure_BEAM_MULTICARRier, nullptr, nullptr },
    { "", "", 0, 0, 0, 0 }
};

static SATCOMM_NODE st_INIT_BEAM[] = {
    { "IMM", "IMMediate", 0, nullptr, &CYSATScpiParse::Set_BEAM_IMMediate, &CYSATScpiParse::Get_BEAM_IMMediate },
    { "", "", 0, 0, 0, 0 }
};

static SATCOMM_NODE st_INIT_IMMediate[] = {
    { "SRSCNW", "SRSCNW", 0, nullptr, &CYSATScpiParse::Set_INIT_IMMediate_SRSCNW, nullptr },
    { "", "", 0, 0, 0, 0 }
};
static SATCOMM_NODE st_INIT[] = {
    { "BEAM", "BEAM<beamindex>", sizeof(st_INIT_BEAM) / sizeof(SATCOMM_NODE), st_INIT_BEAM, nullptr, nullptr },
    { "IMM", "IMMediate", sizeof(st_INIT_IMMediate) / sizeof(SATCOMM_NODE), st_INIT_IMMediate, nullptr, nullptr },
    { "", "", 0, 0, 0, 0 }
};

static SATCOMM_NODE st_CONFigure_ENB[] = {
    { "PARA", "PARAmeter", 0, nullptr, &CYSATScpiParse::Set_CONFigure_ENB_PARA, nullptr },
    { "", "", 0, 0, 0, 0 }
};
// CONFig:FUNCtion:BEAM<beamindex>:MIB:BEAMID <id>
// CONFig:FUNCtion:BEAM<beamindex>:MIB:FREQID <id>
// CONFig:FUNCtion:BEAM<beamindex>:MIB:BANDID <id>
// CONFig:FUNCtion:BEAM<beamindex>:MIB:SSLOT <slot>
static SATCOMM_NODE st_CONFigure_FUNCtion_BEAM_MIB[] = {
    { "BEAMID", "BEAMID", 0, nullptr, &CYSATScpiParse::st_CONFigure_FUNCtion_BEAM_MIB_BEAMID, nullptr },
    { "FREQID", "FREQID", 0, nullptr, &CYSATScpiParse::st_CONFigure_FUNCtion_BEAM_MIB_FREQID, nullptr },
    { "BANDID", "BANDID", 0, nullptr, &CYSATScpiParse::st_CONFigure_FUNCtion_BEAM_MIB_BANDID, nullptr },
    { "SSLOT", "SSLOT", 0, nullptr, &CYSATScpiParse::st_CONFigure_FUNCtion_BEAM_MIB_SSLOT, nullptr },
    { "", "", 0, 0, 0, 0 }
};
// CONFig:FUNCtion:BEAM<beamindex>:SIB<sibindex>:MCC <mcc>
// CONFig:FUNCtion:BEAM<beamindex>:SIB<sibindex>:RMINLEVel <level>
// CONFig:FUNCtion:BEAM<beamindex>:SIB<sibindex>:THREshold <value>
// CONFig:FUNCtion:BEAM<beamindex>:SIB<sibindex>:OFFSet <value>
// CONFig:FUNCtion:BEAM<beamindex>:SIB<sibindex>:BEAMRSELect <value>
// CONFig:FUNCtion:BEAM<beamindex>:SIB<sibindex>:POWer <channeltype>,<value>
static SATCOMM_NODE st_CONFigure_FUNCtion_BEAM_SIB[] = {
    { "MCC", "MCC", 0, nullptr, &CYSATScpiParse::st_CONFigure_FUNCtion_BEAM_SIB_MCC, nullptr },
    { "MNC", "MNC", 0, nullptr, &CYSATScpiParse::st_CONFigure_FUNCtion_BEAM_SIB_MNC, nullptr },
    { "SMINLEV", "SMINLEVel", 0, nullptr, &CYSATScpiParse::st_CONFigure_FUNCtion_BEAM_SIB_SMINLEVel, nullptr },
    { "RMINLEV", "RMINLEVel", 0, nullptr, &CYSATScpiParse::st_CONFigure_FUNCtion_BEAM_SIB_RMINLEVel, nullptr },
    { "THRE", "THREshold", 0, nullptr, &CYSATScpiParse::st_CONFigure_FUNCtion_BEAM_SIB_THREshold, nullptr },
    { "OFFS", "OFFSet", 0, nullptr, &CYSATScpiParse::st_CONFigure_FUNCtion_BEAM_SIB_OFFSet, nullptr },
    { "BEAMRSEL", "BEAMRSELect", 0, nullptr, &CYSATScpiParse::st_CONFigure_FUNCtion_BEAM_SIB_BEAMRSELect, nullptr },
    { "POW", "POWer", 0, nullptr, &CYSATScpiParse::st_CONFigure_FUNCtion_BEAM_SIB_POWer, nullptr },
    { "", "", 0, 0, 0, 0 }
};
static SATCOMM_NODE st_CONFigure_FUNCtion_BEAM[] = {
    { "MIB", "MIB", sizeof(st_CONFigure_FUNCtion_BEAM_MIB) / sizeof(SATCOMM_NODE), st_CONFigure_FUNCtion_BEAM_MIB, 0, 0 },
    { "SIB", "SIB<sibindex>", sizeof(st_CONFigure_FUNCtion_BEAM_SIB) / sizeof(SATCOMM_NODE), st_CONFigure_FUNCtion_BEAM_SIB, 0, 0 }, //
    { "", "", 0, 0, 0, 0 }
};

static SATCOMM_NODE st_CONFigure_FUNCtion[] = {
    { "BEAM", "BEAM<beamindex>", sizeof(st_CONFigure_FUNCtion_BEAM) / sizeof(SATCOMM_NODE), st_CONFigure_FUNCtion_BEAM, nullptr, nullptr },
    { "", "", 0, 0, 0, 0 },
};
// CONFig:PROTocol:BEAM<beamindex>:BEAMRSELect:POWer <value>
// CONFig:PROTocol:BEAM<beamindex>:BEAMSWITch:POWer <value>
static SATCOMM_NODE st_CONFigure_PROTocol_BEAM_BEAMRSELect[] = {
    { "POW", "POWer", 0, nullptr, &CYSATScpiParse::st_CONFigure_PROTocol_BEAM_BEAMRSELect_POWer, nullptr },
    { "", "", 0, 0, 0, 0 },
};
static SATCOMM_NODE st_CONFigure_PROTocol_BEAM_BEAMSWITch[] = {
    { "POW", "POWer", 0, nullptr, &CYSATScpiParse::st_CONFigure_PROTocol_BEAM_BEAMSWITch_POWer, nullptr },
    { "", "", 0, 0, 0, 0 },
};
static SATCOMM_NODE st_CONFigure_PROTocol_BEAM[] = {
    { "BEAMRSEL", "BEAMRSELect", sizeof(st_CONFigure_PROTocol_BEAM_BEAMRSELect) / sizeof(SATCOMM_NODE), st_CONFigure_PROTocol_BEAM_BEAMRSELect, 0, 0 },
    { "BEAMSWIT", "BEAMSWITch", sizeof(st_CONFigure_PROTocol_BEAM_BEAMSWITch) / sizeof(SATCOMM_NODE), st_CONFigure_PROTocol_BEAM_BEAMSWITch, 0, 0 },
    { "", "", 0, 0, 0, 0 },
};
static SATCOMM_NODE st_CONFigure_PROTocol[] = {
    { "BEAM", "BEAM<beamindex>", sizeof(st_CONFigure_PROTocol_BEAM) / sizeof(SATCOMM_NODE), st_CONFigure_PROTocol_BEAM, 0, 0 }, //
    { "", "", 0, 0, 0, 0 },
};

static SATCOMM_NODE st_CONFigure_RESOurce_BEAM_RACH[] = {
    { "BAND", "BAND", 0, 0, &CYSATScpiParse::Set_Config_Resource_Beam_Rach_Band, 0, NEED_RESET_MEAS_RESULT },
    { "FREQNO", "FREQNO", 0, 0, &CYSATScpiParse::Set_Config_Resource_Beam_Rach_FreqNO, 0, NEED_RESET_MEAS_RESULT },
    { "FRAM", "FRAMe", 0, 0, &CYSATScpiParse::Set_Config_Resource_Beam_Rach_Frame, 0, NEED_RESET_MEAS_RESULT },
    { "SUBFREQNO", "SUBFREQNO", 0, 0, &CYSATScpiParse::Set_Config_Resource_Beam_Rach_SubfreqNO, 0, NEED_RESET_MEAS_RESULT },
    { "", "", 0, 0, 0, 0 }
};
static SATCOMM_NODE st_CONFigure_RESOurce_BEAM[] = {
    { "RACH", "RACH", sizeof(st_CONFigure_RESOurce_BEAM_RACH) / sizeof(SATCOMM_NODE), st_CONFigure_RESOurce_BEAM_RACH, 0, 0 }, //
    { "SCH", "SCH", 0, 0, &CYSATScpiParse::Set_Config_Resource_Beam_Sch, 0 },
    { "", "", 0, 0, 0, 0 },
};
static SATCOMM_NODE st_CONFigure_RESOurce[] = {
    { "BEAM", "BEAM<beamindex>", sizeof(st_CONFigure_RESOurce_BEAM) / sizeof(SATCOMM_NODE), st_CONFigure_RESOurce_BEAM, 0, 0 },
    { "", "", 0, 0, 0, 0 },
};
static SATCOMM_NODE st_CONFigure_TTCN[] = {
    { "DEL", "DELay", 0, nullptr, &CYSATScpiParse::st_CONFigure_TTCN_DELay, nullptr },
    { "", "", 0, 0, 0, 0 }
};
static SATCOMM_NODE st_CONFigure[] = {
    { "LOG", "LOG", sizeof(st_CONFigure_LOG) / sizeof(SATCOMM_NODE), st_CONFigure_LOG, 0, 0 },
    { "BEAM", "BEAM<beamindex>", sizeof(st_CONFigure_BEAM) / sizeof(SATCOMM_NODE), st_CONFigure_BEAM, 0, 0 },  //
    { "ENB", "ENB", sizeof(st_CONFigure_ENB) / sizeof(SATCOMM_NODE), st_CONFigure_ENB, 0, 0 },                 //
    { "FUNC", "FUNCtion", sizeof(st_CONFigure_FUNCtion) / sizeof(SATCOMM_NODE), st_CONFigure_FUNCtion, 0, 0 }, //
    { "PROT", "PROTocol", sizeof(st_CONFigure_PROTocol) / sizeof(SATCOMM_NODE), st_CONFigure_PROTocol, 0, 0 },
    { "RESO", "RESOurce", sizeof(st_CONFigure_RESOurce) / sizeof(SATCOMM_NODE), st_CONFigure_RESOurce, 0, 0 },
    { "TTCN", "TTCN", sizeof(st_CONFigure_TTCN) / sizeof(SATCOMM_NODE), st_CONFigure_TTCN, 0, 0 },
    { "", "", 0, 0, 0, 0 }
};
#endif
// CONFigure:LOG:REMOte:UDP  126.1.2.1:2000
// CONFigure:PCAP:REMOte:UDP  126.1.2.1:2000
static SATCOMM_NODE st_FETCh_BEAM_IOT[] = {
    { "RANPID", "RANPID", 0, 0, 0, &CYSATScpiParse::st_FETCh_BEAM_IOT_RANPID, NEED_RESET_MEAS_RESULT },
    { "", "", 0, 0, 0, 0 }
};
static SATCOMM_NODE st_FETCh_BEAM_AN[] = {
    { "RANPID", "RANPID", 0, 0, 0, &CYSATScpiParse::st_FETCh_BEAM_AN_RANPID, NEED_RESET_MEAS_RESULT },
    { "", "", 0, 0, 0, 0 }
};
static SATCOMM_NODE st_FETCh_BEAM[] = {
    { "IOT", "IOT", sizeof(st_FETCh_BEAM_IOT) / sizeof(SATCOMM_NODE), st_FETCh_BEAM_IOT, nullptr, nullptr },
    { "AN", "AN", sizeof(st_FETCh_BEAM_AN) / sizeof(SATCOMM_NODE), st_FETCh_BEAM_AN, 0, 0 },
    { "", "", 0, 0, 0, 0 }
};
static SATCOMM_NODE st_FETCh[] = {
    { "BEAM", "BEAM<beamindex>", sizeof(st_FETCh_BEAM) / sizeof(SATCOMM_NODE), st_FETCh_BEAM, 0, 0 },
    { "", "", 0, 0, 0, 0 }
};
static SATCOMM_NODE _RootNode[] = {
    { "", "*CLS", 0, 0, &CYSATScpiParse::Set_CLS, 0, NEED_RESET_MEAS_RESULT },
    { "", "*IDN", 0, 0, 0, &CYSATScpiParse::Get_IDN },
    { "", "*OPC", 0, 0, &CYSATScpiParse::Set_OPC, 0, NEED_RESET_MEAS_RESULT },
    { "", "*RST", 0, 0, &CYSATScpiParse::Set_RST, 0, NEED_RESET_MEAS_RESULT },
    { "INIT", "INITiate", sizeof(st_INIT) / sizeof(SATCOMM_NODE), st_INIT, nullptr, nullptr },
    { "CONF", "CONFig", sizeof(st_CONFigure) / sizeof(SATCOMM_NODE), st_CONFigure, 0, 0 },
    { "IPERF", "IPERF", 0, 0, &CYSATScpiParse::Set_IPERF, 0, NEED_RESET_MEAS_RESULT },
    { "FETC", "FETCh", sizeof(st_FETCh) / sizeof(SATCOMM_NODE), st_FETCh, nullptr, nullptr },
    { "", "", 0, 0, 0, 0 }
};

CYSATScpiParse::CYSATScpiParse(const all_args_t& args)
    : CYICScpiParse< CYSATScpiParse >(_RootNode, sizeof(_RootNode) / sizeof(SATCOMM_NODE)), conf_path(args.conf.conf_path)
{
    cout << "conf path:" << conf_path << endl;
    conf_path.erase(remove(conf_path.begin(), conf_path.end(), '\"'), conf_path.end());
    cout << "conf path:" << conf_path << endl;
    SetConfigFilePath(conf_path);
#if 0
    vector< string > vs{};
    vs.push_back("/IOT/beam1/enb.conf");
    vs.push_back("enb_files.mib_config");
    vs.push_back(conf_path + "/IOT/beam1/mib.conf");
    vs.push_back("string");
    ParseINIConfig(vs);
    vs[1] = "enb_files.sib_wx_config";
    vs[2] = conf_path + "/IOT/beam1/sib_wx.conf";
    ParseINIConfig(vs);
    vs.clear();
    vs.push_back("/access/beam1/enb1.conf");
    vs.push_back("enb_files.mib_config");
    vs.push_back(conf_path + "/access/beam1/mib1.conf");
    vs.push_back("string");
    ParseINIConfig(vs);
    vs[1] = "enb_files.sib_wx_config";
    vs[2] = conf_path + "/access/beam1/sib_wx1.conf";
    ParseINIConfig(vs);
    vs.clear();
    vs.push_back("/IOT/beam2/enb.conf");
    vs.push_back("enb_files.mib_config");
    vs.push_back(conf_path + "/IOT/beam2/mib.conf");
    vs.push_back("string");
    ParseINIConfig(vs);
    vs[1] = "enb_files.sib_wx_config";
    vs[2] = conf_path + "/IOT/beam2/sib_wx.conf";
    ParseINIConfig(vs);
    vs.clear();
    vs.push_back("/access/beam2/enb1.conf");
    vs.push_back("enb_files.mib_config");
    vs.push_back(conf_path + "/access/beam2/mib1.conf");
    vs.push_back("string");
    ParseINIConfig(vs);
    vs[1] = "enb_files.sib_wx_config";
    vs[2] = conf_path + "/access/beam2/sib_wx1.conf";
    ParseINIConfig(vs);
#endif
    for (int i = 0; i < 16; i++)
    {
        nor_channeltype.push_back("PDCH1_1");
    }
}

CYSATScpiParse::~CYSATScpiParse() {}
void CYSATScpiParse::Set_CLS(std::string& ouput) {}

void CYSATScpiParse::Get_IDN(std::string& ouput) {}
void CYSATScpiParse::Set_OPC(std::string& ouput) {}
void CYSATScpiParse::Set_RST(std::string& ouput)
{
    vector< vector< string > > vvs{};
    ReadDefaultParas(vvs);
    for (const auto& c : vvs)
    {
        if (c[0].find("enb") == string::npos)
            ParseConfigFile(c);
        else
            ParseINIConfig(c);
    }
}
void CYSATScpiParse::Set_IPERF(std::string& output)
{
    string status = GetEnumPara();
    string execmd = GetEnumPara();
    std::cout << "status:" << status << " execmd:" << execmd << std::endl;
    string cmd = "/usr/bin/" + execmd;
    if (status == "ON")
    {
        // pid_t pid = spawn_process("/usr/bin/iperf3 -c 192.168.0.30 -p 9527 -t 99 -f M", "iperf3");
        pid_t pid = spawn_process(cmd.data(), "iperf");
        if (pid > 0)
        {
            if (std::find(vpid.begin(), vpid.end(), pid) == vpid.end()) // no find push pid.
            {
                vpid.push_back(pid);
                std::cout << pid << std::endl;
            }
        }
        std::cout << "run iperf." << std::endl;
    }
    else if (status == "OFF") // run once no need kill.
    {
        for (const auto& c : vpid)
        {
            if (c > 0)
            {
                stop_process(c);
                waitpid(c, 5000);
            }
        }
        vpid.clear();
        std::cout << "end iperf." << std::endl;
    }
}

void CYSATScpiParse::Set_DEBUG(std::string& ouput) {}

/// @brief need finished
/// @param ouput
void CYSATScpiParse::Set_CONFigure_LOG_ENABLE(std::string& ouput)
{
    // VLE_LOGE(CONTROL_LEVEL_INFO, "CYSATScpiParse: Get_SYSTem_ERRor_NEXT()");
    char* penable = GetEnumPara();

    const char* able_list[] = { "OFF", "ON" };
    int         index = CheckEnumPara(penable, able_list, 2);
    if (index < 0)
    {
        // VLE_LOGE(CONTROL_LEVEL_ERROR,"Get Para 'State_index' out of range,%d",State_index);
        index = 0;
    }
    // srsran::console("Set_CONFigure_LOG_ENABLE enable= %d\n", index);
}
void CYSATScpiParse::Get_CONFigure_LOG_ENABLE(std::string& ouput)
{
    // srsran::console("CYSATScpiParse: Get_CONFigure_LOG_ENABLE\n");
}
void CYSATScpiParse::Set_BEAM_Common_BEAM(std::string& ouput)
{
    int nbeam = GetIntPara();

    if (CheckIntPara(nbeam, 1, 2) < 0)
    {
        nbeam = 1;
    }
    // srsran::console("CYSATScpiParse: Set_BEAM_Common_BEAM:%d\n", nbeam);
}
void CYSATScpiParse::Get_BEAM_Common_BEAM(std::string& ouput) {}

void CYSATScpiParse::Set_BEAM_Common_DUPMode(std::string& ouput)
{
    char* mode = GetEnumPara();

    const char* mode_list[] = { "FDD" };
    int         index = CheckEnumPara(mode, mode_list, sizeof(mode_list) / sizeof(const char*));
    if (index < 0)
    {
        // VLE_LOGE(CONTROL_LEVEL_ERROR,"Get Para 'State_index' out of range,%d",State_index);
        index = 0;
    }
    // srsran::console("Set_BEAM_Common_DUPMode mode= %d\n", index);
}
void CYSATScpiParse::Get_BEAM_Common_DUPMode(std::string& ouput) {}

void CYSATScpiParse::Set_BEAM_Common_Band(std::string& ouput)
{
    const int   sz = 55;
    const int   col = 8;
    char        str_list[sz][col] = { 0 };
    const char* check_list[sz];
    for (int i = 1; i <= sz; i++)
    {
        int n = snprintf(str_list[i - 1], col, "n%d", i);
        str_list[i - 1][n] = '\0';
        check_list[i - 1] = str_list[i - 1];
    }
    char* band = GetEnumPara();
    int   index = CheckEnumPara(band, check_list, sz);
    if (index < 0)
    {
        index = 0;
        // srsran::console("Set_BEAM_Common_Band target mode= %s\n", band);
        //  for (int i = 1; i <= sz; i++) {
        //    srsran::console("Set_BEAM_Common_Band mode[i]= %s\n", check_list[i - 1]);
        //  }
    }
    // srsran::console("Set_BEAM_Common_Band mode= %d\n", index + 1);
}
void CYSATScpiParse::Get_BEAM_Common_Band(std::string& ouput) {}

void CYSATScpiParse::Set_BEAM_Common_Network(std::string& ouput)
{
    // CONFig:BEAM:COMmon:NETWORK <nNetwork> AN/IOT
    char*       mode = GetEnumPara();
    int         beam_idx = GetSuffixPara("beamindex");
    const char* access_list[] = { "AN", "IOT" };
    int         index = CheckEnumPara(mode, access_list, sizeof(access_list) / sizeof(const char*));
    if (index < 0)
    {
        index = 0;
    }
    if (beam_idx < 1 || beam_idx > 2)
    {
        srsran::console("beam idx beyond!,beam idx :%d\n", beam_idx);
        return;
    }
    beam_mode_map[beam_idx] = std::string(mode);
    return;
}
void CYSATScpiParse::Get_BEAM_Common_Network(std::string& ouput)
{
    int beam_idx = GetSuffixPara("beamindex");
    if (beam_idx < 1 || beam_idx > 2)
    {
        srsran::console("beam idx beyond!,beam idx :%d\n", beam_idx);
        return;
    }
    srsran::console("beam: %d -- network mode: %s", beam_idx, beam_mode_map[beam_idx].data());
    ouput.assign(beam_mode_map[beam_idx].data());
}

void CYSATScpiParse::Set_BEAM_Common_Mode(std::string& ouput)
{
    // CONFig:BEAM:COMmon:MODE <nMode> NORMal/SPREAD
    char* mode = GetEnumPara();

    const char* mode_list[] = { "NORMal", "SPREAD" };
    int         index = CheckEnumPara(mode, mode_list, sizeof(mode_list) / sizeof(const char*));
    if (index < 0)
    {
        index = 0;
    }

    // srsran::console("Set_BEAM_Common_Mode net= %d\n", index);
}
void CYSATScpiParse::Get_BEAM_Common_Mode(std::string& ouput) {}

void CYSATScpiParse::Set_BEAM_Mode(std::string& output)
{
    int                                  beamindex = GetSuffixPara("beamindex");
    string                               valstr = GetEnumPara();
    std::unordered_map< string, string > modemp = { { "NORMal", "0" }, { "NORM", "0" }, { "SPREAD", "1" } };
    if (modemp.find(valstr) != modemp.end())
    {
        auto res = ParseParameter("beam.mode", beamindex, modemp[valstr], string{ "string" });
        for (auto const& c : res)
        {
            ParseINIConfig(c);
        }
    }
}
void CYSATScpiParse::Set_INIT_IMMediate_SRSCNW(std::string& output)
{
    string state = GetEnumPara();
    if (state == "ON")
    {
        start_an_app(4); //
    }
    else if (state == "OFF")
    {
        stop_an_app(4);
    }
}
void CYSATScpiParse::Set_BEAM_IMMediate(std::string& ouput)
{
#ifdef NEW_BEAM_IMM
    // INITiate:BEAM:IMMediate OFF/ON
    char*       state = GetEnumPara();   // Get On Off
    char*       NetMode = GetEnumPara(); // Get net mode
    int         beam_idx = GetSuffixPara("beamindex");
    const char* NetMode_list[] = { "AN", "IOT" };
    int         net_mode_idx = CheckEnumPara(NetMode, NetMode_list, sizeof(NetMode_list) / sizeof(const char*));
    if (net_mode_idx < 0)
    {
        net_mode_idx = 0;
        std::cerr << "Invalid beam NET MODE" << std::endl;
        return;
    }
    const char* state_list[] = { "OFF", "ON" };
    int         idx = CheckEnumPara(state, state_list, sizeof(state_list) / sizeof(const char*));
    if (idx < 0)
    {
        idx = 0;
        std::cerr << "Invalid beam state" << std::endl;
        return;
    }
    if (beam_idx < 1 || beam_idx > 2) // range 1-2
    {
        std::cerr << "Get invalid beam index:" << beam_idx << std::endl;
        return;
    }
    if (net_mode_idx == 0)
    {
        beam_idx += 2;
    }

    if (beam_status[beam_idx - 1] == idx)
    {
        srsran::console("before already configed! --( beam %d,net work:%s ,state :%s)\n", idx, NetMode, state);
        return;
    }
    if (idx == 0)
    {
        stop_an_app(beam_idx - 1);
    }
    else
    {
        start_an_app(beam_idx - 1);
    }
    beam_status[beam_idx - 1] = idx;
#else
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    char*       state = GetEnumPara(); // Get On Off
    const char* state_list[] = { "OFF", "ON" };
    int         state_idx = CheckEnumPara(state, state_list, sizeof(state_list) / sizeof(const char*));
    if (state_idx < 0)
    {
        state_idx = 0;
        std::cerr << "Invalid beam state" << std::endl;
        return;
    }
    int beam_idx = GetSuffixPara("beamindex");
    if (beam_idx < 1 || beam_idx > 2) // range 1-2
    {
        std::cerr << "Get invalid beam index:" << beam_idx << std::endl;
        return;
    }
    if (beam_mode_map[beam_idx] == "AN")
    {
        beam_idx += 2;
    }
    int status = get_an_app_status(beam_idx - 1);
    if (status == state_idx)
    {
        srsran::console("enb state no change!\n");
        return;
    }
    if (state_idx == 0)
    {
        stop_an_app(beam_idx - 1);
    }
    else
    {
        start_an_app(beam_idx - 1);
    }
#endif
    return;
}
void CYSATScpiParse::Get_BEAM_IMMediate(std::string& output)
{
    int         state = 0;
    int         beam_idx = GetSuffixPara("beamindex");
    const char* state_list[] = { "OFF", "ON" };
    if (beam_idx < 1 || beam_idx > 2)
    {
        std::cerr << "Get invalid beam index:" << beam_idx << std::endl;
        output.assign("ERR");
        return;
    }
    if (beam_mode_map[beam_idx] == "AN")
    {
        beam_idx += 2;
    }
    int status = get_an_app_status(beam_idx - 1);
    if (status <= 0)
    {
        output.assign("OFF");
    }
    else
    {
        output.assign("ON");
    }
    std::cout << "send replay:" << output << std::endl;
    return;
}
/*
    function :modify enb para using config enb start para.
    para list:
    filepath : configure file exsit path
    Word Segsemt: eg:[enb]
    Segment Sub Para Name: eg:enb_id
    Para Value : eg:1
  */
void CYSATScpiParse::Set_CONFigure_ENB_PARA(std::string& ouput)
{
    char* filepath = GetEnumPara();
    char* WordSegment = GetEnumPara();
    char* SegSubParaName = GetEnumPara();
    char* ParaValue = GetEnumPara();
    assert(filepath);
    srsran::console("ENB Config Para Modify:\n filepath:%s Word Segment:%s ,Subname :%s, Value:%s",
                    filepath,
                    WordSegment,
                    SegSubParaName,
                    ParaValue);
    Handle_Enb_Config_Modify(filepath, WordSegment, SegSubParaName, ParaValue);
}
void CYSATScpiParse::Set_Config_Resource_Beam_Rach_Band(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    string network = GetEnumPara();
    string channel_type = GetEnumPara();
    int    val = GetIntPara();
    string valstr = to_string(val);
    int    ret = CheckIntPara(val, 1, 56);

    if (ret < 0)
    {
        srsran::console("line: %d para beyond!\n", __LINE__);
        valstr = "28";
    }
    unordered_map< string, vector< string > > nwmode{
        { "AN", { "/access", "/sib_wx1.conf" } },
        { "IOT", { "/IOT", "/sib_wx.conf" } },
    };
    unordered_map< string, string > mpchntype{
        { "AN RACH", "sib.rr_config_com_sib.rach_cfg.ba_id" },
        { "AN AGCH", "sib.rr_config_com_sib.agch_cfg.band_id.band_id" },
        { "IOT RACH", "iot_sib.iot_rach_config.dl_band_id" },
        { "IOT AGCH", "iot_sib.iot_agch_conf.dl_band_id" },
    };
    vector< string > vs;
    vs.push_back(nwmode[network][0] + mp[beamindex] + nwmode[network][1]);
    vs.push_back(mpchntype[network + " " + channel_type]);
    vs.push_back(valstr);
    vs.push_back("int");
    cout << vs[0] << " " << vs[1] << " " << __LINE__;
    ParseConfigFile(vs);
}
void CYSATScpiParse::Set_Config_Resource_Beam_Rach_FreqNO(std::string& output)
{
    int                             beamindex = GetSuffixPara("beamindex");
    string                          network = GetEnumPara();
    string                          channel_type = GetEnumPara();
    int                             val = GetIntPara();
    string                          valstr = to_string(val);
    unordered_map< string, string > default_valmap = { { "IOTRACH", "3" }, { "IOTAGCH", "1" }, { "ANRACH", "8" }, { "ANAGCH", "3" } };
    int                             rang = 15;
    if (network == "AN" && channel_type == "RACH")
        rang = 15;
    else
        rang = 3;
    int ret = CheckIntPara(val, 0, rang);
    if (ret < 0)
    {
        srsran::console("line: %d para beyond!\n", __LINE__);
        valstr = default_valmap[network + channel_type];
    }
    unordered_map< string, string > channeltypemap = { { "IOTRACH", "iot_sib.iot_rach_config.iot_rach_freq_list.ul_freq_id" },
                                                       { "IOTAGCH", "iot_sib.iot_agch_conf.dl_freq_id" },
                                                       { "ANRACH", "sib.rr_config_com_sib.rach_cfg.freq_bit_map" },
                                                       { "ANAGCH", "sib.rr_config_com_sib.agch_cfg.freq_id.freq_id" } };
    vector< string >                vs{};
    string                          path = networkmap[network] + mp[beamindex] + sibmap[network];
    string                          key = channeltypemap[network + channel_type];
    vs.push_back(path);
    vs.push_back(key);
    vs.push_back(valstr);
    vs.push_back("int");
    cout << vs[0] << "____" << vs[1] << endl;
    ParseConfigFile(vs);
}
void CYSATScpiParse::Set_Config_Resource_Beam_Rach_Frame(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    string network = GetEnumPara();
    string channel_type = GetEnumPara();
    int    val = GetIntPara();
    string valstr = to_string(val);
    int    ret = CheckIntPara(val, 0, 15);
    if (ret < 0)
    {
        srsran::console("line: %d para beyond!\n", __LINE__);
        valstr = "15";
    }
    unordered_map< string, string > channeltypemap = {
        { "IOTRACH", "sib.rr_config_com_sib.rach_cfg.rach_frame_ass" },
        { "IOTAGCH", "sib.rr_config_com_sib.agch_cfg.agch_fram_ass" },
        { "ANRACH", "sib.rr_config_com_sib.rach_cfg.rach_frame_ass" },
        { "ANAGCH", "sib.rr_config_com_sib.agch_cfg.agch_fram_ass.agch_fram_ass" }
    };
    vector< string > vs{};
    string           path = networkmap[network] + mp[beamindex] + sibmap[network];
    string           key = channeltypemap[network + channel_type];
    vs.push_back(path);
    vs.push_back(key);
    vs.push_back(valstr);
    vs.push_back("int");
    cout << vs[0] << "____" << vs[1] << endl;
    ParseConfigFile(vs);
}
void CYSATScpiParse::Set_Config_Resource_Beam_Rach_SubfreqNO(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    string network = GetEnumPara();
    string channel_type = GetEnumPara();
    int    val = GetIntPara();
    string valstr = to_string(val);
    int    ret = CheckIntPara(val, 0, 15);
    if (ret < 0)
    {
        srsran::console("line: %d para beyond!\n", __LINE__);
        valstr = "1";
    }
    unordered_map< string, vector< string > > nwmode{
        { "AN", { "/access", "/sib_wx1.conf" } },
        { "IOT", { "/IOT", "/sib_wx.conf" } },
    };
    unordered_map< string, string > mpchntype{
        { "AN RACH", "sib.rr_config_com_sib.rach_cfg.ba_id" },
        { "AN AGCH", "sib.rr_config_com_sib.agch_cfg.band_id.band_id" },
        { "IOT RACH", "iot_sib.iot_rach_config.iot_rach_freq_list.ul_sub_freq_bit_map" },
        { "IOT AGCH", "iot_sib.iot_agch_conf.dl_band_id" },
    };
    vector< string > vs;
    vs.push_back(nwmode[network][0] + mp[beamindex] + nwmode[network][1]);
    vs.push_back(mpchntype[network + " " + channel_type]);
    vs.push_back(valstr);
    vs.push_back("int");
    cout << vs[0] << " " << vs[1] << " " << __LINE__;
    ParseConfigFile(vs);
}
void CYSATScpiParse::Set_Config_Resource_Beam_Sch(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    string network = GetEnumPara();
    string mode = GetEnumPara();
    string channel_type = GetEnumPara();
    int    val = GetIntPara();
    float  freq = GetFreqHzPara();
    string valstr = to_string(val);
    int    ret = CheckIntPara(val, 1, 56);
    if (ret < 0)
    {
        srsran::console("line: %d para beyond!\n", __LINE__);
        valstr = "1";
    }
}

void CYSATScpiParse::Set_CONFigure_BEAM_LOOP(std::string& output)
{
    // dddd
}
void CYSATScpiParse::Set_CONFigure_BEAM_FRAMEOFFSet(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    val = GetIntPara();
    string valstr = to_string(val);
    int    ret = CheckIntPara(val, 0, 51);
    if (ret < 0)
    {
        srsran::console("line: %d para beyond!\n", __LINE__);
        valstr = "10";
    }
    auto res = ParseParameter("mib.frameoffset", beamindex, valstr, string{ "int" });
    for (auto const& c : res)
    {
        cout << c[0] << " " << c[1] << " " << c[2] << " " << c[3] << " " << __LINE__;
        ParseConfigFile(c);
    }
}
void CYSATScpiParse::Set_CONFigure_BEAM_MULTICARRier_TransMode(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    string val = GetEnumPara();
    unordered_map<string,int> mp = {{"AM",0},{"UM",1}};
    string valstr = to_string(mp[val]);
    auto res = ParseParameter("multi.transmode", beamindex, valstr, string{ "int" });
    for (auto const& c : res)
    {
        cout << c[0] << " " << c[1] << " " << c[2] << " " << c[3] << " " << __LINE__;
        ParseINIConfig(c);
    }
}
void CYSATScpiParse::Set_BEAM_DISTance(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    val = GetIntPara();
    string valstr = to_string(val);
    int    ret = CheckIntPara(val, 0, 3);
    if (ret < 0)
    {
        srsran::console("line: %d para beyond!\n", __LINE__);
        valstr = "1";
    }
    auto res = ParseParameter("mib.distance", beamindex, valstr, string{ "int" });
    for (auto const& c : res)
    {
        cout << c[0] << " " << c[1] << " " << c[2] << " " << c[3] << " " << __LINE__;
        ParseConfigFile(c);
    }
}
void CYSATScpiParse::st_CONFigure_FUNCtion_BEAM_MIB_BEAMID(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    val = GetIntPara();
    string valstr = to_string(val);
    int    ret = CheckIntPara(val, 0, 16383);
    if (ret < 0)
    {
        srsran::console("line: %d para beyond!\n", __LINE__);

        valstr = "2";
    }
    cout << "hre" << __LINE__;
    auto res = ParseParameter("mib.beamid", beamindex, valstr, string{ "int" });
    for (auto const& c : res)
    {
        cout << c[0] << " " << c[1] << " " << c[2] << " " << c[3] << " " << __LINE__;
        ParseConfigFile(c);
    }
}

void CYSATScpiParse::st_CONFigure_FUNCtion_BEAM_MIB_FREQID(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    val = GetIntPara();
    string valstr = to_string(val);
    int    ret = CheckIntPara(val, 0, 3);
    if (ret < 0)
    {
        srsran::console("line: %d para beyond!\n", __LINE__);
        valstr = "1";
    }
    auto res = ParseParameter("mib.freqid", beamindex, valstr, string{ "int" });
    for (auto const& c : res)
        ParseConfigFile(c);
}
void CYSATScpiParse::st_CONFigure_FUNCtion_BEAM_MIB_BANDID(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    val = GetIntPara();
    string valstr = to_string(val);
    int    ret = CheckIntPara(val, 1, 56);
    if (ret < 0)
    {
        srsran::console("line: %d para beyond!\n", __LINE__);
        valstr = "3";
    }
    auto res = ParseParameter("mib.bandid", beamindex, valstr, string{ "int" });
    for (auto const& c : res)
        ParseConfigFile(c);
}
void CYSATScpiParse::st_CONFigure_FUNCtion_BEAM_MIB_SSLOT(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    val = GetIntPara();
    string valstr = to_string(val);
    int    ret = CheckIntPara(val, 1, 4);
    if (ret < 0)
    {
        srsran::console("line: %d para beyond!\n", __LINE__);
        valstr = "4";
    }
    auto res = ParseParameter("mib.sslot", beamindex, valstr, string{ "int" });
    for (auto const& c : res)
        ParseConfigFile(c);
}
void CYSATScpiParse::st_CONFigure_FUNCtion_BEAM_SIB_MCC(std::string& output)
{
    int      beamindex = GetSuffixPara("beamindex");
    uint16_t mcc = 0;
    string   valstr = GetEnumPara();
    if (!srsran::string_to_mcc(valstr, &mcc))
    {
        srsran::console("line: %d send parameter error,set to default value!\n", __LINE__);
        valstr = "460";
    }
    srsran::console("line: %d mcc val: %s\n", __LINE__, valstr.data());
    auto res = ParseParameter("sib.mcc", beamindex, valstr, string{ "int" });
    for (auto const& c : res)
    {
        ParseINIConfig(c);
    }
}

void CYSATScpiParse::st_CONFigure_FUNCtion_BEAM_SIB_MNC(std::string& output)
{
    int      beamindex = GetSuffixPara("beamindex");
    uint16_t mnc = 0;
    string   valstr = GetEnumPara();
    if (!srsran::string_to_mnc(valstr, &mnc))
    {
        srsran::console("line: %d send parameter error,set to default value!\n", __LINE__);
        valstr = "00";
    }
    srsran::console("line: %d mnc val: %s\n", __LINE__, valstr.data());
    auto res = ParseParameter("sib.mnc", beamindex, valstr, string{ "int" });
    for (auto const& c : res)
        ParseINIConfig(c);
}
void CYSATScpiParse::st_CONFigure_TTCN_DELay(std::string& output)
{
    int beamindex = 1;
    int delay = GetIntPara();
    int ret = CheckIntPara(delay, 0, 120);
    if (ret < 0)
    {
        delay = 0;
    }
    string valstr = to_string(delay);
    srsran::console("line: %d ttcn delay val: %s\n", __LINE__, valstr.data());
    auto res = ParseParameter("ttcn.delay", beamindex, valstr, string{ "int" });
    for (auto const& c : res)
    {
        ParseINIConfig(c);
    }
    beamindex = 2;
    res = ParseParameter("ttcn.delay", beamindex, valstr, string{ "int" });
    for (auto const& c : res)
    {
        ParseINIConfig(c);
    }
}
void CYSATScpiParse::st_CONFigure_FUNCtion_BEAM_SIB_SMINLEVel(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    float  val = GetFloatPara();
    string valstr = to_string(val);
    int    ret = CheckFloatPara(val, -70, -40);
    if (ret < 0)
    {
        srsran::console("line: %d para beyond!\n", __LINE__);
        valstr = "-60";
    }
    auto res = ParseParameter("sib.sminlevel", beamindex, valstr, string{ "float" });
    for (auto const& c : res)
        ParseConfigFile(c);
}
void CYSATScpiParse::st_CONFigure_FUNCtion_BEAM_SIB_RMINLEVel(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    float  val = GetFloatPara();
    string valstr = to_string(val);
    int    ret = CheckFloatPara(val, -70, -40);
    if (ret < 0)
    {
        srsran::console("line: %d para beyond!\n", __LINE__);
        valstr = "-60";
    }
    auto res = ParseParameter("sib.rminlevel", beamindex, valstr, string{ "float" });
    for (auto const& c : res)
        ParseConfigFile(c);
}
void CYSATScpiParse::st_CONFigure_FUNCtion_BEAM_SIB_THREshold(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    val = GetIntPara();
    string valstr = to_string(val);
    int    ret = CheckIntPara(val, 0, 63);
    if (ret < 0)
    {
        srsran::console("line: %d para beyond!\n", __LINE__);
        valstr = "63";
    }
    auto res = ParseParameter("sib.threshold", beamindex, valstr, string{ "int" });
    for (auto const& c : res)
        ParseConfigFile(c);
}
void CYSATScpiParse::st_CONFigure_FUNCtion_BEAM_SIB_OFFSet(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    val = GetIntPara();
    string valstr = to_string(val);
    int    ret = CheckIntPara(val, -9, 9);
    if (ret < 0)
    {
        srsran::console("line: %d para beyond!\n", __LINE__);
        valstr = "-1";
    }
    auto res = ParseParameter("sib.offset", beamindex, valstr, string{ "int" });
    for (auto const& c : res)
        ParseConfigFile(c);
}
void CYSATScpiParse::st_CONFigure_FUNCtion_BEAM_SIB_BEAMRSELect(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    val = GetIntPara();
    string valstr = to_string(val);
    int    ret = CheckIntPara(val, 0, 7);
    if (ret < 0)
    {
        srsran::console("line: %d para beyond!\n", __LINE__);
        valstr = "5";
    }
    auto res = ParseParameter("sib.bearmselect", beamindex, valstr, string{ "int" });
    for (auto const& c : res)
        ParseConfigFile(c);
}
void CYSATScpiParse::st_CONFigure_FUNCtion_BEAM_SIB_POWer(std::string& output)
{
    int                                       beamindex = GetSuffixPara("beamindex");
    string                                    channeltype = GetEnumPara();
    float                                     val = GetFloatPara();
    string                                    valstr = to_string(val);
    int                                       ret = CheckFloatPara(val, 0, 63);
    unordered_map< string, vector< string > > default_power_map = {
        { "PRACH", { "33", "sib.rr_config_com_sib.power_cfg.exp_rx_p_prach.exp_rx_p_prach11" } },
        { "PSYCH", { "60", "sib.rr_config_com_sib.power_cfg.exp_rx_p_psych.exp_rx_p_psych" } },
        { "PDCH1_1", { "60", "sib.rr_config_com_sib.power_cfg.exp_rx_p_pdch1_1.exp_rx_p_pdch1_1" } },
        { "PDCH1_2", { "30", "sib.rr_config_com_sib.power_cfg.exp_rx_p_pdch1_2.exp_rx_p_pdch1_2" } },
        { "PSCH1_1", { "30", "sib.rr_config_com_sib.power_cfg.exp_rx_p_psch1_1.exp_rx_p_psch1_1" } },
        { "PSCH1_2", { "30", "sib.rr_config_com_sib.power_cfg.exp_rx_p_psch1_2.exp_rx_p_psch1_2" } },
        { "PSCH5_1", { "37", "sib.rr_config_com_sib.power_cfg.exp_rx_p_psch5_1.exp_rx_p_psch5_1" } },
        { "PSCH5_2", { "37", "sib.rr_config_com_sib.power_cfg.exp_rx_p_psch5_2.exp_rx_p_psch5_2" } },
        { "PTUCH", { "37", "sib.rr_config_com_sib.power_cfg.exp_rx_p_ptuch.exp_rx_p_ptuch" } },
    };
    if (default_power_map.find(channeltype) == default_power_map.end())
    {
        srsran::console("line: %d ,no exist this type : %s.\n", __LINE__, channeltype.data());
        return;
    }
    if (ret < 0)
    {
        srsran::console("line: %d para beyond!\n", __LINE__);
        valstr = default_power_map[channeltype][0];
    }
    // no mapping to .conf data
    string           network = "AN";
    vector< string > vs{};
    string           path = networkmap[network] + mp[beamindex] + sibmap[network];
    string           key = default_power_map[channeltype][1];
    vs.push_back(path);
    vs.push_back(key);
    vs.push_back(valstr);
    vs.push_back("float");
    ParseConfigFile(vs);
}
void CYSATScpiParse::st_CONFigure_PROTocol_BEAM_BEAMRSELect_POWer(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    float  val = GetFloatPara();
    string valstr = to_string(val);
    int    ret = CheckFloatPara(val, -120, 0);
    if (ret < 0)
    {
        srsran::console("line: %d para beyond!\n", __LINE__);
        valstr = "-20";
    }
    // no mapping to .conf data
}
void CYSATScpiParse::st_CONFigure_PROTocol_BEAM_BEAMSWITch_POWer(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    float  val = GetFloatPara();
    string valstr = to_string(val);
    int    ret = CheckFloatPara(val, -120, 0);
    if (ret < 0)
    {
        srsran::console("line: %d para beyond!\n", __LINE__);
        valstr = "-20";
    }
}
std::string CYSATScpiParse::MapCarrierToKeyName(std::string bussinesstype, int carrier, int type)
{
    unordered_map< string, string > keymp = {
        { "DATA", "recfg_wx.rrc_con_recfg_r1_ies.redio_resour_cfg_dedi.phy_chan_list_cfg." },
        { "VOICE", "recfg_wx.rrc_con_recfg_r1_ies.redio_resour_cfg_dedi.phy_chan_list_cfg." }
    };
    string keyname = keymp[bussinesstype];
    if (type == EMCS)
    {
        keyname = "recfg_wx.";
    }
    unordered_map< int, string > mp = { { EBAND, "band_id" }, { EFREQNO, "freq_id" }, { ECHANNELTYPE, "chan_type" }, { ESLOT, "slot_ass" }, { EMCS, "MCS" } };
    string                       tmp{};
    if (carrier == 1)
    {
        tmp = mp[type];
    }
    else
    {
        tmp = mp[type] + to_string(carrier - 1);
    }
    keyname += tmp;

    return keyname;
}
std::string CYSATScpiParse::MapiotCarrierToKeyName(int mode, int carriernum, int type, int direction)
{
    // mode : norm spread
    // dirtection ul dl
    string                       keyname = "scheduler.";
    unordered_map< int, string > mode_mp = { { NORM, "norm" }, { SPREAD, "spread" } };
    unordered_map< int, string > direct_mp = { { UL, "ul" }, { DL, "dl" } };
    unordered_map< int, string > type_mp = { { EBAND, "band" }, { EFREQNO, "freqid" }, { ECHANNELTYPE, "chantype" }, { ESCHETYPE, "schetype" } };
    unordered_map< int, string > norm_type_mp_ul = { { EBAND, "iot_ul_band" }, { EFREQNO, "iot_ul_freq" }, { ECHANNELTYPE, "chan_type" }, { ESCHETYPE, "iot_ul_sched_type" } };
    unordered_map< int, string > norm_type_mp_dl = { { EBAND, "iot_dl_band" }, { EFREQNO, "iot_dl_freq" }, { ECHANNELTYPE, "chan_type" }, { ESCHETYPE, "iot_dl_sched_type" } };
    unordered_map< int, string > spread_type_mp_ul = { { EBAND, "iot_ul_band" }, { EFREQNO, "iot_ul_freq" }, { ECHANNELTYPE, "chan_type" }, { ESCHETYPE, "iot_ul_sched_type" } };
    // unordered_map< int, string >         spread_type_mp_dl = { { EBAND, "band_id" }, { EFREQNO, "freq_id" }, { ECHANNELTYPE, "chan_type" } };
    string                               search = mode_mp[mode] + direct_mp[direction] + type_mp[type];
    std::unordered_map< string, string > key_mp = {
        { "normulschetype", norm_type_mp_ul[type] },
        { "normdlschetype", norm_type_mp_dl[type] },
        { "spreadulschetype", spread_type_mp_ul[type] },

        { "normulband", norm_type_mp_ul[type] },
        { "normulfreqid", norm_type_mp_ul[type] },
        { "normdlband", norm_type_mp_dl[type] },
        { "normdlfreqid", norm_type_mp_dl[type] },
        { "spreadulband", spread_type_mp_ul[type] },
        { "spreadulfreqid", spread_type_mp_ul[type] },

        { "normulchantype", norm_type_mp_ul[type] },
        { "normdlchantype", norm_type_mp_dl[type] },
        { "spreadulchantype", spread_type_mp_ul[type] },
        //{ "spreaddlfreqid", spread_type_mp_dl[type] },
        // { "spreaddlchantype", spread_type_mp_dl[type] },
    };
    keyname += key_mp[search];
    std::cout << keyname.data() << std::endl;
    return keyname;

    return keyname;
}
std::string CYSATScpiParse::MapSpreadToKeyName(std::string bussinesstype, int type, int direction)
{
    unordered_map< string, string > keymp = {
        { "DATA", "recfg_wx.rrc_con_recfg_r1_ies.redio_resour_cfg_dedi.phy_chan_list_cfg." },
        { "VOICE", "recfg_wx.rrc_con_recfg_r1_ies.redio_resour_cfg_dedi.phy_chan_list_cfg." }
    };

    string keyname = keymp[bussinesstype];
    if (type == EMCS)
    {
        keyname = "recfg_wx.";
    }
    std::unordered_map< int, string > ulmp = {
        { EBAND, "nor_band_id" }, { EFREQNO, "nor_freq_id" }, { ECHANNELTYPE, "nor_chan_type" }, { ESLOT, "nor_slot_ass" }, { EMCS, "MCS" }
    };
    std::unordered_map< int, string > dlmp = {
        { EBAND, "sp_band_id" }, { EFREQNO, "sp_freq_id" }, { ECHANNELTYPE, "sp_chan_type" }, { ESLOT, "sp_slot_ass" }
    };
    string tmp{};
    if (direction == DL)
    {
        tmp = dlmp[type];
    }
    else
    {
        tmp = ulmp[type];
    }
    keyname += tmp;
    return keyname;
}
std::vector< string > CYSATScpiParse::GenerateWriteFileParas(std::string bussinesstype, int carriernum, int beamindex, int type, std::string valstr)
{
    string           network = "AN";
    vector< string > vs{};
    string           path = networkmap[network] + mp[beamindex] + normrecfgfilemap[bussinesstype];
    string           key = MapCarrierToKeyName(bussinesstype, carriernum, type);
    vs.push_back(path);
    vs.push_back(key);
    vs.push_back(valstr);
    vs.push_back("string");
    return vs;
}
std::vector< std::string > CYSATScpiParse::GeniotWriteFileParas(int mode, int carriernum, int beamindex, int type, int direction, std::string valstr)
{
    string           network = "IOT";
    vector< string > vs{};
    string           path = networkmap[network] + mp[beamindex] + recfgmap[network];
    string           key = MapiotCarrierToKeyName(mode, carriernum, type, direction);
    std::cout << path.data() << std::endl;
    vs.push_back(path);
    vs.push_back(key);
    vs.push_back(valstr);
    vs.push_back("string");
    return vs;
}
std::vector< string > CYSATScpiParse::GenSpreadWriteFileParas(std::string bussinesstype, int beamindex, int type, int dir, std::string valstr)
{
    string           network = "AN";
    vector< string > vs{};
    string           path = networkmap[network] + mp[beamindex] + spreadrecfgfilemap[bussinesstype];
    string           key = MapSpreadToKeyName(bussinesstype, type, dir);
    vs.push_back(path);
    vs.push_back(key);
    vs.push_back(valstr);
    vs.push_back("string");
    return vs;
}
void CYSATScpiParse::st_CONFigure_BEAM_AN_NORM_MULTICARRier_NUM(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetIntPara();
    int    ret = CheckIntPara(carriernum, 1, 16);
    string valstr = to_string(carriernum);
    if (ret < 0)
    {
        valstr = "1";
    }
    auto res = ParseParameter("norm.num", beamindex, valstr, string{ "int" });
    for (auto const& c : res)
        ParseINIConfig(c);
}
void CYSATScpiParse::st_CONFigure_BEAM_AN_SPREAD_MULTICARRier_NUM(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetIntPara();
    int    ret = CheckIntPara(carriernum, 2, 2);
    string valstr = to_string(carriernum);
    if (ret < 0)
    {
        valstr = "2";
    }
    auto res = ParseParameter("norm.num", beamindex, valstr, string{ "int" });
    for (auto const& c : res)
        ParseINIConfig(c);
}
void CYSATScpiParse::st_CONFigure_BEAM_AN_NORM_MULTICARRier_TYPE(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetSuffixPara("carrier");
    string bussinesstype = GetEnumPara();
    string valstr = GetEnumPara();

    if (normtypemp.find(valstr) != normtypemp.end())
    {
        nor_channeltype[carriernum - 1] = valstr;
        ParseConfigFile(GenerateWriteFileParas(bussinesstype, carriernum, beamindex, ECHANNELTYPE, normtypemp[valstr]));
    }
}
void CYSATScpiParse::st_CONFigure_BEAM_AN_NORM_MULTICARRier_BAND(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetSuffixPara("carrier");
    string bussinesstype = GetEnumPara();
    int    val = GetIntPara();
    string valstr = to_string(val);
    int    ret = CheckIntPara(val, 1, 56);
    if (ret < 0)
    {
        valstr = "1";
    }
    ParseConfigFile(GenerateWriteFileParas(bussinesstype, carriernum, beamindex, EBAND, valstr));
}
void CYSATScpiParse::st_CONFigure_BEAM_AN_NORM_MULTICARRier_FREQNO(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetSuffixPara("carrier");
    string bussinesstype = GetEnumPara();
    int    val = GetIntPara();
    string valstr = to_string(val);
    int    ret = CheckIntPara(val, 0, 3);
    if (ret < 0)
    {
        valstr = "0";
    }

    ParseConfigFile(GenerateWriteFileParas(bussinesstype, carriernum, beamindex, EFREQNO, valstr));
}
std::vector< int > CYSATScpiParse::GetChannelTypeRange(std::string currentchanneltype, int mode)
{
    std::vector< int >                          vi{};
    std::unordered_map< string, vector< int > > normp = {
        { "PDCH1_1", { 2, 4, 8, 16 } },
        { "PSCH1_1", { 2, 4, 8, 16 } },
        { "PSCH5_1", { 2, 4, 8, 16 } },
        { "PDCH1_2", { 6, 12, 24 } },
        { "PSCH1_2", { 6, 12, 24 } },
        { "PSCH5_2", { 6, 12, 24 } },
    };
    std::unordered_map< string, vector< int > > spreadmp = {
        { "PDCH1_1", { 1, 2, 4, 8, 16 } },
        { "PSCH1_1", { 1, 2, 4, 8, 16 } },
        { "PSCH5_1", { 1, 2, 4, 8, 16 } },
        { "PDCH1_2", { 3, 6, 12, 24 } },
        { "PSCH1_2", { 3, 6, 12, 24 } },
        { "PSCH5_2", { 3, 6, 12, 24 } },
    };
    if (mode == NORM)
    {
        if (normp.find(currentchanneltype) != normp.end())
            return normp[currentchanneltype];
    }
    else if (mode == SPREAD)
    {
        if (spreadmp.find(currentchanneltype) != spreadmp.end())
            return spreadmp[currentchanneltype];
    }
    return vi;
}
void CYSATScpiParse::st_CONFigure_BEAM_AN_NORM_MULTICARRier_SLOT(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetSuffixPara("carrier");
    string bussinesstype = GetEnumPara();
    int    val = GetIntPara();
    if (val < 2 or val > 30)
    {
        val = 2;
    }
    string valstr = to_string(val);
#if 0
    std::string        currentchanneltype = nor_channeltype[carriernum - 1];
    std::vector< int > vi = GetChannelTypeRange(currentchanneltype, NORM);
    if (vi.size() == 0)
        return;
    if (std::find(vi.begin(), vi.end(), val) == vi.end())
    {
        cout << "slot paras error" << std::endl;
        return;
    }
#endif
    ParseConfigFile(GenerateWriteFileParas(bussinesstype, carriernum, beamindex, ESLOT, valstr));
}
void CYSATScpiParse::st_CONFigure_BEAM_AN_NORM_MULTICARRier_MCS(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetSuffixPara("carrier");
    string bussinesstype = GetEnumPara();
    string valstr = GetEnumPara();
    if (MSCtoValmp.find(valstr) == MSCtoValmp.end())
    {
        cout << "mcs paras error" << std::endl;
        return;
    }
    std::string        currentchanneltype = nor_channeltype[carriernum - 1];
    std::vector< int > vi = channeltypetomsc_mp[currentchanneltype];
    if (std::find(vi.begin(), vi.end(), MSCtoValmp[valstr]) == vi.end())
    {
        cout << "input paras error" << std::endl;
        return;
    }

    ParseConfigFile(GenerateWriteFileParas(bussinesstype, carriernum, beamindex, EMCS, to_string(MSCtoValmp[valstr])));
}
void CYSATScpiParse::st_CONFigure_BEAM_AN_SPREAD_DL_MULTICARRier_TYPE(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetSuffixPara("carrier");
    string bussinesstype = GetEnumPara();
    string valstr = GetEnumPara();
    if (spreadDLtypemp.find(valstr) != spreadDLtypemp.end())
    {
        spread_dl_channeltype = valstr;
        ParseConfigFile(GenSpreadWriteFileParas(bussinesstype, beamindex, ECHANNELTYPE, DL, spreadDLtypemp[valstr]));
    }
}
void CYSATScpiParse::st_CONFigure_BEAM_AN_SPREAD_DL_MULTICARRier_BAND(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetSuffixPara("carrier");
    string bussinesstype = GetEnumPara();
    int    val = GetIntPara();
    string valstr = to_string(val);
    int    ret = CheckIntPara(val, 1, 56);
    if (ret < 0)
    {
        valstr = "1";
    }
    ParseConfigFile(GenSpreadWriteFileParas(bussinesstype, beamindex, EBAND, DL, valstr));
}
void CYSATScpiParse::st_CONFigure_BEAM_AN_SPREAD_DL_MULTICARRier_FREQNO(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetSuffixPara("carrier");
    string bussinesstype = GetEnumPara();
    int    val = GetIntPara();
    string valstr = to_string(val);
    int    ret = CheckIntPara(val, 0, 3);
    if (ret < 0)
    {
        valstr = "0";
    }
    ParseConfigFile(GenSpreadWriteFileParas(bussinesstype, beamindex, EFREQNO, DL, valstr));
}
void CYSATScpiParse::st_CONFigure_BEAM_AN_SPREAD_DL_MULTICARRier_SLOT(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetSuffixPara("carrier");
    string bussinesstype = GetEnumPara();
    int    val = GetIntPara();
    if (val < 1 or val > 31)
    {
        val = 1;
    }
    string valstr = to_string(val);
#if 0
    std::vector< int > vi = { 1, 2, 4, 8, 16 };
    if (std::find(vi.begin(), vi.end(), val) == vi.end())
    {
        cout << "slot paras error" << std::endl;
        return;
    }
#endif
    ParseConfigFile(GenSpreadWriteFileParas(bussinesstype, beamindex, ESLOT, DL, valstr));
}
void CYSATScpiParse::st_CONFigure_BEAM_AN_SPREAD_UL_MULTICARRier_TYPE(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetSuffixPara("carrier");
    string bussinesstype = GetEnumPara();
    string valstr = GetEnumPara();
    if (normtypemp.find(valstr) != normtypemp.end())
    {
        spread_ul_channeltype = valstr;
        ParseConfigFile(GenSpreadWriteFileParas(bussinesstype, beamindex, ECHANNELTYPE, UL, normtypemp[valstr]));
    }
}
void CYSATScpiParse::st_CONFigure_BEAM_AN_SPREAD_UL_MULTICARRier_BAND(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetSuffixPara("carrier");
    string bussinesstype = GetEnumPara();
    int    val = GetIntPara();
    string valstr = to_string(val);
    int    ret = CheckIntPara(val, 1, 56);
    if (ret < 0)
    {
        valstr = "1";
    }
    ParseConfigFile(GenSpreadWriteFileParas(bussinesstype, beamindex, EBAND, UL, valstr));
}
void CYSATScpiParse::st_CONFigure_BEAM_AN_SPREAD_UL_MULTICARRier_FREQNO(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetSuffixPara("carrier");
    string bussinesstype = GetEnumPara();
    int    val = GetIntPara();
    string valstr = to_string(val);
    int    ret = CheckIntPara(val, 0, 3);
    if (ret < 0)
    {
        valstr = "0";
    }
    ParseConfigFile(GenSpreadWriteFileParas(bussinesstype, beamindex, EFREQNO, UL, valstr));
}
void CYSATScpiParse::st_CONFigure_BEAM_AN_SPREAD_UL_MULTICARRier_SLOT(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetSuffixPara("carrier");
    string bussinesstype = GetEnumPara();
    int    val = GetIntPara();
    if (val < 1 or val > 31)
    {
        val = 1;
    }
    string valstr = to_string(val);
#if 0
    std::string        currentchanneltype = spread_ul_channeltype;
    std::vector< int > vi = GetChannelTypeRange(currentchanneltype, SPREAD);
    if (vi.size() == 0)
        return;
    if (std::find(vi.begin(), vi.end(), val) == vi.end())
    {
        cout << "slot paras error" << std::endl;
        return;
    }
#endif
    ParseConfigFile(GenSpreadWriteFileParas(bussinesstype, beamindex, ESLOT, UL, valstr));
}
void CYSATScpiParse::st_CONFigure_BEAM_AN_SPREAD_UL_MULTICARRier_MCS(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetSuffixPara("carrier");
    string bussinesstype = GetEnumPara();
    string valstr = GetEnumPara();
    if (MSCtoValmp.find(valstr) == MSCtoValmp.end())
    {
        cout << "mcs paras error" << std::endl;
        return;
    }
    std::string        currentchanneltype = spread_ul_channeltype;
    std::vector< int > vi = channeltypetomsc_mp[currentchanneltype];
    if (std::find(vi.begin(), vi.end(), MSCtoValmp[valstr]) == vi.end())
    {
        cout << "input paras error" << std::endl;
        return;
    }

    ParseConfigFile(GenSpreadWriteFileParas(bussinesstype, beamindex, EMCS, UL, to_string(MSCtoValmp[valstr])));
}
// 2024.06.24 ADD FOR IOT CMD
void CYSATScpiParse::st_CONFigure_BEAM_IOT_NORM_MULTICARRier_NUM(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetIntPara();
    int    ret = CheckIntPara(carriernum, 2, 2);
    string valstr = to_string(carriernum);
    if (ret < 0)
    {
        valstr = "2";
    }
    auto res = ParseParameter("iot.carriernum", beamindex, valstr, string{ "int" });
    for (auto const& c : res)
        ParseINIConfig(c);
}
void CYSATScpiParse::st_CONFigure_BEAM_IOT_SPREAD_FACTOR(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetSuffixPara("carrier");
    int    val = GetIntPara();
    string valstr = to_string(val);
    int    ret = CheckIntPara(val, 0, 2);
    if (ret < 0)
    {
        valstr = "0";
    }
    auto res = ParseParameter("spread.factor", beamindex, valstr, string{ "int" });
    for (auto const& c : res)
        ParseINIConfig(c);
}
void CYSATScpiParse::st_CONFigure_BEAM_IOT_SPREAD_MULTICARRier_NUM(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetIntPara();
    int    ret = CheckIntPara(carriernum, 2, 2);
    string valstr = to_string(carriernum);
    if (ret < 0)
    {
        valstr = "2";
    }
    auto res = ParseParameter("iot.carriernum", beamindex, valstr, string{ "int" });
    for (auto const& c : res)
        ParseINIConfig(c);
}
void CYSATScpiParse::st_CONFigure_BEAM_IOT_NORM_DL_MULTICARRier_TYPE(std::string& output)
{
    int beamindex = GetSuffixPara("beamindex");
    int carriernum = GetSuffixPara("carrier");
    // PTDCH
    string valstr = GetEnumPara();
    if (valstr != "PTDCH")
        return;
    if (iotnormtype.find(valstr) != iotnormtype.end())
        ParseConfigFile(GeniotWriteFileParas(NORM, carriernum, beamindex, ECHANNELTYPE, DL, iotnormtype[valstr]));
}
void CYSATScpiParse::st_CONFigure_BEAM_IOT_NORM_DL_MULTICARRier_SCHETYPE(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetSuffixPara("carrier");
    int    val = GetIntPara();
    string valstr = to_string(val);
    if (val != 0 and val != 1)
        return;
    ParseINIConfig(GeniotWriteFileParas(NORM, carriernum, beamindex, ESCHETYPE, DL, valstr));
}
void CYSATScpiParse::st_CONFigure_BEAM_IOT_NORM_DL_MULTICARRier_BAND(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetSuffixPara("carrier");
    int    val = GetIntPara();
    string valstr = to_string(val);
    int    ret = CheckIntPara(val, 1, 56);
    if (ret < 0)
    {
        valstr = "1";
    }
    ParseINIConfig(GeniotWriteFileParas(NORM, carriernum, beamindex, EBAND, DL, valstr));
}
void CYSATScpiParse::st_CONFigure_BEAM_IOT_NORM_DL_MULTICARRier_FREQNO(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetSuffixPara("carrier");
    int    val = GetIntPara();
    string valstr = to_string(val);
    int    ret = CheckIntPara(val, 0, 3);
    if (ret < 0)
    {
        valstr = "0";
    }

    ParseINIConfig(GeniotWriteFileParas(NORM, carriernum, beamindex, EFREQNO, DL, valstr));
}
void CYSATScpiParse::st_CONFigure_BEAM_IOT_NORM_UL_MULTICARRier_TYPE(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetSuffixPara("carrier");
    string valstr = GetEnumPara();

    if (valstr != "PTUCH")
        return;
    if (iotnormtype.find(valstr) != iotnormtype.end())
        ParseConfigFile(GeniotWriteFileParas(NORM, carriernum, beamindex, ECHANNELTYPE, UL, iotnormtype[valstr]));
}
void CYSATScpiParse::st_CONFigure_BEAM_IOT_NORM_UL_MULTICARRier_SCHETYPE(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetSuffixPara("carrier");
    int    val = GetIntPara();
    string valstr = to_string(val);
    if (val != 0 and val != 1)
        return;
    ParseINIConfig(GeniotWriteFileParas(NORM, carriernum, beamindex, ESCHETYPE, UL, valstr));
}
void CYSATScpiParse::st_CONFigure_BEAM_IOT_NORM_UL_MULTICARRier_BAND(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetSuffixPara("carrier");
    int    val = GetIntPara();
    string valstr = to_string(val);
    int    ret = CheckIntPara(val, 1, 56);
    if (ret < 0)
    {
        valstr = "1";
    }
    ParseINIConfig(GeniotWriteFileParas(NORM, carriernum, beamindex, EBAND, UL, valstr));
}
void CYSATScpiParse::st_CONFigure_BEAM_IOT_NORM_UL_MULTICARRier_FREQNO(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetSuffixPara("carrier");
    int    val = GetIntPara();
    string valstr = to_string(val);
    int    ret = CheckIntPara(val, 0, 3);
    if (ret < 0)
    {
        valstr = "0";
    }

    ParseINIConfig(GeniotWriteFileParas(NORM, carriernum, beamindex, EFREQNO, UL, valstr));
}
void CYSATScpiParse::st_CONFigure_BEAM_IOT_SPREAD_DL_MULTICARRier_TYPE(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetSuffixPara("carrier");
    string valstr = GetEnumPara();
    if (spreadDLtypemp.find(valstr) != spreadDLtypemp.end())
        ParseConfigFile(GeniotWriteFileParas(SPREAD, carriernum, beamindex, ECHANNELTYPE, DL, spreadDLtypemp[valstr]));
}
void CYSATScpiParse::st_CONFigure_BEAM_IOT_SPREAD_DL_MULTICARRier_SCHETYPE(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetSuffixPara("carrier");
    string valstr = GetEnumPara();
    if (spreadDLtypemp.find(valstr) != spreadDLtypemp.end())
        ParseConfigFile(GeniotWriteFileParas(SPREAD, carriernum, beamindex, ESCHETYPE, DL, spreadDLtypemp[valstr]));
}
void CYSATScpiParse::st_CONFigure_BEAM_IOT_SPREAD_DL_MULTICARRier_BAND(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetSuffixPara("carrier");
    int    val = GetIntPara();
    string valstr = to_string(val);
    int    ret = CheckIntPara(val, 1, 56);
    if (ret < 0)
    {
        valstr = "1";
    }
    ParseConfigFile(GeniotWriteFileParas(SPREAD, carriernum, beamindex, EBAND, DL, valstr));
}
void CYSATScpiParse::st_CONFigure_BEAM_IOT_SPREAD_DL_MULTICARRier_FREQNO(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetSuffixPara("carrier");
    int    val = GetIntPara();
    string valstr = to_string(val);
    int    ret = CheckIntPara(val, 0, 3);
    if (ret < 0)
    {
        valstr = "0";
    }
    ParseConfigFile(GeniotWriteFileParas(SPREAD, carriernum, beamindex, EFREQNO, DL, valstr));
}
void CYSATScpiParse::st_CONFigure_BEAM_IOT_SPREAD_UL_MULTICARRier_TYPE(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetSuffixPara("carrier");
    string valstr = GetEnumPara();
    if (valstr != "PTUCH")
        return;
    if (iotnormtype.find(valstr) != iotnormtype.end())
        ParseConfigFile(GeniotWriteFileParas(SPREAD, carriernum, beamindex, ECHANNELTYPE, UL, iotnormtype[valstr]));
}
void CYSATScpiParse::st_CONFigure_BEAM_IOT_SPREAD_UL_MULTICARRier_SCHETYPE(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetSuffixPara("carrier");
    int    val = GetIntPara();
    string valstr = to_string(val);
    if (val != 0 and val != 1)
        return;
    ParseINIConfig(GeniotWriteFileParas(SPREAD, carriernum, beamindex, ESCHETYPE, UL, valstr));
}
void CYSATScpiParse::st_CONFigure_BEAM_IOT_SPREAD_UL_MULTICARRier_BAND(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetSuffixPara("carrier");
    int    val = GetIntPara();
    string valstr = to_string(val);
    int    ret = CheckIntPara(val, 1, 56);
    if (ret < 0)
    {
        valstr = "1";
    }
    ParseINIConfig(GeniotWriteFileParas(SPREAD, carriernum, beamindex, EBAND, UL, valstr));
}
void CYSATScpiParse::st_CONFigure_BEAM_IOT_SPREAD_UL_MULTICARRier_FREQNO(std::string& output)
{
    int    beamindex = GetSuffixPara("beamindex");
    int    carriernum = GetSuffixPara("carrier");
    int    val = GetIntPara();
    string valstr = to_string(val);
    int    ret = CheckIntPara(val, 0, 3);
    if (ret < 0)
    {
        valstr = "0";
    }
    ParseINIConfig(GeniotWriteFileParas(SPREAD, carriernum, beamindex, EFREQNO, UL, valstr));
}
// add for fetch
void CYSATScpiParse::st_FETCh_BEAM_IOT_RANPID(std::string& output)
{
    int         beamindex = GetSuffixPara("beamindex");
    int         pid = Get_beam_pid(beamindex - 1);
    std::string spid = to_string(pid);
    output = "beam" + to_string(beamindex) + "," + "iot" + "," + spid;
    std::cout << output << std::endl;
}
void CYSATScpiParse::st_FETCh_BEAM_AN_RANPID(std::string& output)
{
    int         beamindex = GetSuffixPara("beamindex");
    int         pid = Get_beam_pid(beamindex + 1);
    std::string spid = to_string(pid);
    output = "beam" + to_string(beamindex) + "," + "an" + "," + spid;
    std::cout << output << std::endl;
}

void CYSATScpiParse::Set_UDP_Remote(std::string& ouput)
{
    srslog::sink& sink = srslog::get_default_sink();
    cy_udp_sink*  udpSink = dynamic_cast< cy_udp_sink* >(&sink);
    char*         strPara = GetStringPara();

    if (udpSink)
    {
        std::string ipcfg(strPara);
        std::cout << "get ipcfg:" << ipcfg << std::endl;
        auto        pos = ipcfg.find(':');
        int         uport = 0;
        std::string ip{};
        if (pos > 0)
        {
            std::string ip = ipcfg.substr(0, pos);
            std::string port = ipcfg.substr(pos + 1, ipcfg.size());
            uport = atoi(port.c_str());
        }
        else
        {
            ip = ipcfg;
            uport = 6666;
        }

        std::cout << "get ip:" << ip << " port:" << uport << std::endl;
        // VLE_LOGE(CONTROL_LEVEL_INFO,"Set_UDP_Remote:%s, port %d", ip, port);
        udpSink->reinit(ip, static_cast< uint16_t >(uport));
    }
    else
    {
        // srsran::console("CYSATScpiParse: Set_UDP_Remote get sink invalid.");
    }
}
void CYSATScpiParse::Get_UDP_Remote(std::string& ouput) {}

int CYSATScpiParse::RemoteCommandProcess(char* CommandBuf, std::string& output)
{
    int ret = CYICScpiParse< CYSATScpiParse >::RemoteCommandProcess(CommandBuf, output);
    return ret;
}

void CYSATScpiParse::ConvetConfigValue(Setting& Setting, const string& val)
{
    switch (Setting.getType())
    {
        case Setting::TypeInt:
            Setting = stoi(val);
            break;
        case Setting::TypeString:
            Setting = val;
            break;
        case Setting::TypeFloat:
            Setting = stod(val);
            break;
        default:
            break;
    }
}
void CYSATScpiParse::ParseINIConfig(const std::vector< std::string >& vs)
{
    if (vs.size() != 4)
        return;
    inicpp::IniManager _ini(conf_path + vs[0]);
    vector< string >   key_list;
    srsran::string_parse_list(vs[1], '.', key_list);
    if (key_list.size() != 2)
        return;
    _ini.modify(key_list[0], key_list[1], vs[2]);
}
void CYSATScpiParse::ParseConfigFile(const vector< string >& vs)
{
    if (vs.size() != 4)
        return;
    Config cfg;
    try
    {
#if LIBCONFIGXX_VER_MINOR < 6
        cfg.setOptions(Setting::OptionSemicolonSeparators | Setting::OptionOpenBraceOnSeparateLine);
#else
        cfg.setOptions(Config::OptionSemicolonSeparators | Config::OptionOpenBraceOnSeparateLine);
#endif
        cout << (conf_path + vs[0]).data() << __LINE__ << endl;
        cfg.readFile((conf_path + vs[0]).data());
    }
    catch (const FileIOException& fioex)
    {
        std::cerr << "I/O error while reading file." << std::endl;
        return;
    }
    vector< string > cmd_list;
    srsran::string_parse_list(vs[1], '.', cmd_list);
    for (const auto& c : cmd_list)
    {
        cout << "cmd_list:" << c.data() << endl;
    }
    if (cmd_list.size() == 0)
    {
        return;
    }
    unordered_map< string, Setting::Type > mp{
        { "int", Setting::TypeInt }, { "float", Setting::TypeFloat }, { "string", Setting::TypeString }
    };
    string tmp{ cmd_list[0] };
    size_t i = 0;
    for (; i != cmd_list.size(); i++)
    {
        if (!cfg.exists(tmp))
        {
            string   skey{ tmp };
            Setting& key = cfg.lookup(skey.erase(skey.rfind(".")));
            if (i != cmd_list.size() - 1)
            {
                key.add(cmd_list[i], Setting::TypeGroup);
            }
            else
            {
                Setting& val = key.add(cmd_list[i], mp[vs[3]]);
            }
        }
        if (i + 1 < cmd_list.size())
            tmp += "." + cmd_list[i + 1];
    }
    cout << "key : " << tmp.data() << " exists: " << (cfg.exists(tmp.data()) ? "true" : "false") << endl;
    Setting& write_val = cfg.lookup(tmp.data());
    ConvetConfigValue(write_val, vs[2]);
    cout << "write file path: " << vs[0].data() << endl;
    cout << "write key :" << tmp.data() << " val :" << vs[2] << endl;
    cfg.writeFile((conf_path + vs[0]).data());
    cout << "write file end" << endl;
}
void CYSATScpiParse::ReadDefaultParas(std::vector< std::vector< string > >& vvs)
{
    ifstream ifs{ "default_paras.conf" };
    string   line{};
    if (ifs.is_open())
    {
        while (getline(ifs, line))
        {
            istringstream    is{ line };
            string           tmp{};
            vector< string > vs{};
            while (getline(is, tmp, ' '))
            {
                vs.push_back(tmp);
            }
            vvs.push_back(vs);
        }
        ifs.close();
    }
}