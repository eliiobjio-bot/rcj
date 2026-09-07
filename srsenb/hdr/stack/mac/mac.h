/**
 * Copyright 2013-2021 Software Radio Systems Limited
 *
 * This file is part of srsRAN.
 *
 * srsRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#ifndef SRSENB_MAC_H
#define SRSENB_MAC_H

#include "sched.h"
#include "sched_interface.h"
#include "srsenb/hdr/common/rnti_pool.h"
#include "srsenb/hdr/stack/mac/schedulers/sched_time_rr.h"
#include "srsran/adt/circular_map.h"
#include "srsran/adt/pool/batch_mem_pool.h"
#include "srsran/common/mac_pcap.h"
#include "srsran/common/pcap_net.h"
#include "srsran/common/task_scheduler.h"
#include "srsran/common/threads.h"
#include "srsran/common/tti_sync_cv.h"
#include "srsran/interfaces/enb_mac_interfaces.h"
#include "srsran/interfaces/enb_metrics_interface.h"
#include "srsran/interfaces/enb_rrc_interface_types.h"
#include "srsran/srslog/srslog.h"
#include "ta.h"
#include "ue.h"
#include "wx_sched.h"
#include <vector>

namespace srsenb
{

//***************V1.0.0********************
#define MIB_LEN 7
#define IoTsi_LEN 37
#define RAR_LEN 20
#define MAX_MIB_PDU_LEN 14
#define MAX_SS_MIB_PDU_LEN 19
#define MAX_IoTsi_PDU_LEN 37
#define MAX_SIB_LEN 144
#define MAX_SIB_PDU_LEN 20
#define MAX_SS_SIB_PDU_LEN 19
#define MaxBandID 56
#define MaxCarrierID 4
#define MaxSlotID 5
#define MAX_PDU_LEN 400
#define MAX_PENDING_RARS 20
#define BCCH_LCID 0x0a
#define PCCH_LCID 0X09
#define BBCH_LCID 0x0b
#define TBCCH_LCID 0x01
#define Pading_LCID 0x1f
#define CCCH_LCID 0x00
#define MAX_SF 52
#define SYCH_LCID 0x08
#define data_time_toleranceMax 3.875 // sacch
#define data_time_toleranceMin -3.875
#define time_toleranceMax 15.875 // sych
#define time_toleranceMin -15.875
#define freq_toleranceMax 2400
#define freq_toleranceMin -2400
#define prach_time_toleranceMax 63.875 // rach
#define prach_time_toleranceMin -63.875
#define prach_freq_toleranceMax 4960
#define prach_freq_toleranceMin -4960
  // typedef enum { MIB, MIB_PCCH, PCCH } pmbchType_t;
  //*******************************************
  class adp;

  class mac final : public mac_interface_phy_lte,  // final的作用：禁止其他类从 mac 类进一步继承
                    public mac_interface_rlc,
                    public mac_interface_rrc,
                    public mac_interface_sched
  {
  public:
    mac(srsran::ext_task_sched_handle task_sched_, srslog::basic_logger &logger);
    ~mac();
    bool init(const mac_args_t &args_,
              const cell_list_t &cells_,
              phy_interface_stack_lte *phy,
              rlc_interface_mac *rlc,
              rrc_interface_mac *rrc, adp *mac_adp_);
    void stop();

    uint32_t network_mode;
    uint32_t area_mode;
    uint32_t multi_beam_num;

    bool judge_rach_detect_counter = false;

    int crc_check_failure_counter = 0;

    int test_bi_backoff = 0;

    int voiceLcid = -1;  // UP Layer indicatte Voice mode - LCID
    int voiceSpeed = -1; // giveup

    // int MCS = 0;
    int temp_MCS = 0;
    mcs_Info MAC_mcs_Info;
    int Pid = -1;


    uint16_t ue_cap14_band_id;
    uint16_t ue_cap14_freq_id;
    uint16_t ue_cap14_slot;
    uint16_t ue_cap14_slot_phy;

    // int phy_voice_type=-1;//0:2.4k 1:4.8k
    std::chrono::_V2::system_clock::time_point start_time;
    std::chrono::_V2::system_clock::time_point end_time;

    std::chrono::_V2::system_clock::time_point first_time;
    std::chrono::_V2::system_clock::time_point first_time_end;

    std::chrono::_V2::system_clock::time_point T3;
    std::chrono::_V2::system_clock::time_point T3_A;
    std::chrono::_V2::system_clock::time_point T3_B;
    std::chrono::_V2::system_clock::time_point T3_C;
    std::chrono::_V2::system_clock::time_point T3_D;
    std::chrono::_V2::system_clock::time_point T3_E;

    void clear_ue();

    void start_pcap(srsran::mac_pcap *pcap_);
    void start_pcap_net(srsran::pcap_net *pcap_net_);
    void Trigger_MCS_param_config();
    bool Reset_Allresource_when_frameOff_NotZero();
    void readFrameOffCfg(int &sched_frameOff);

    // zyg
    /*************Config parameters for RRC**************/
    std::map<uint32_t, configMap> mac_lcid_to_resource; //
    configMap Current_Map;                              // Read the data when the current mapping
    configMap Pading_Map;                               // LCID =1 Map for pading
    bool is_read_data_in_map = false;                   // if read data from RLC(assemble pdu),else padding
    /****************************************************/

    /******** Interface from PHY (PHY -> MAC) ****************/
    int sr_detected(uint32_t tti, uint16_t rnti) final;
    void rach_detected(uint32_t tti, uint32_t enb_cc_idx, uint32_t preamble_idx, uint32_t time_adv) final;
    int ri_info(uint32_t tti, uint16_t rnti, uint32_t enb_cc_idx, uint32_t ri_value) override;
    int pmi_info(uint32_t tti, uint16_t rnti, uint32_t enb_cc_idx, uint32_t pmi_value) override;
    int cqi_info(uint32_t tti, uint16_t rnti, uint32_t enb_cc_idx, uint32_t cqi_value) override;
    int sb_cqi_info(uint32_t tti, uint16_t rnti, uint32_t enb_cc_idx, uint32_t sb_idx, uint32_t cqi_value) override;
    int snr_info(uint32_t tti, uint16_t rnti, uint32_t enb_cc_idx, float snr, ul_channel_t ch) override;
    int ta_info(uint32_t tti, uint16_t rnti, float ta_us) override;
    int ack_info(uint32_t tti, uint16_t rnti, uint32_t enb_cc_idx, uint32_t tb_idx, bool ack) override;
    int crc_info(uint32_t tti, uint16_t rnti, uint32_t enb_cc_idx, uint32_t nof_bytes, bool crc_res) override;
    int push_pdu(uint32_t tti, uint16_t rnti, uint32_t enb_cc_idx, uint32_t nof_bytes, bool crc_res, uint32_t ul_nof_prbs, uint8_t voice);
    bool reestablish_test();
    int get_dl_sched(uint32_t tti_tx_dl, dl_sched_list_t &dl_sched_res) override;
    int get_ul_sched(uint32_t tti_tx_ul, ul_sched_list_t &ul_sched_res) override;
    int get_mch_sched(uint32_t tti, bool is_mcch, dl_sched_list_t &dl_sched_res) override;
    void set_sched_dl_tti_mask(uint8_t *tti_mask, uint32_t nof_sfs) override
    {
      scheduler.set_dl_tti_mask(tti_mask, nof_sfs);
    }
    void build_mch_sched(uint32_t tbs);

    /******** Interface from RRC (RRC -> MAC) ****************/
    /* Provides cell configuration including SIB periodicity, etc. */
    int cell_cfg(const std::vector<sched_interface::cell_cfg_t> &cell_cfg) override;

    /* Manages UE scheduling context */
    int ue_cfg(uint16_t rnti, const sched_interface::ue_cfg_t *cfg) override;
    int ue_rem(uint16_t rnti) override;
    int ue_set_crnti(uint16_t temp_crnti, uint16_t crnti, const sched_interface::ue_cfg_t &cfg) override;

    // Indicates that the PHY config dedicated has been enabled or not
    void phy_config_enabled(uint16_t rnti, bool enabled) override;

    /* Manages UE bearers and associated configuration */
    int bearer_ue_cfg(uint16_t rnti, uint32_t lc_id, mac_lc_ch_cfg_t *cfg) override;
    int bearer_ue_rem(uint16_t rnti, uint32_t lc_id) override;
    int rlc_buffer_state(uint16_t rnti, uint32_t lc_id, uint32_t tx_queue, uint32_t retx_queue) override;

    /* Handover-related */
    uint16_t reserve_new_crnti(const sched_interface::ue_cfg_t &ue_cfg) override;

    void get_metrics(mac_metrics_t &metrics);

    void toggle_padding();

    void add_padding();

    void write_mcch(const srsran::sib2_mbms_t *sib2_,
                    const srsran::sib13_t *sib13_,
                    const srsran::mcch_msg_t *mcch_,
                    const uint8_t *mcch_payload,
                    const uint8_t mcch_payload_length) override;
    //************************V0.0.0*******************************
    logicChanType_t resourceMap[MaxBandID][MaxCarrierID][MAX_SF][MaxSlotID];
    uint8_t pmbchFrameLoc[3] = {13, 26, 39};
    uint8_t sibFrameLoc[8] = {2, 8, 15, 21, 28, 34, 41, 47};
    uint8_t pfcchLoc[8][2] = {{0, 1}, {6, 2}, {13, 3}, {19, 4}, {26, 1}, {32, 2}, {39, 3}, {45, 4}};
    // uint8_t         pfcchLoc[7][2]              = { {6, 2}, {13, 3}, {19, 4}, {26, 1}, {32, 2}, {39, 3}, {45, 4}};
    std::vector<std::vector<uint8_t>> fnConfig = {{},
                                                  {0},
                                                  {1},
                                                  {0, 1},
                                                  {2},
                                                  {0, 2},
                                                  {1, 2},
                                                  {0, 1, 2},
                                                  {3},
                                                  {0, 3},
                                                  {1, 3},
                                                  {0, 1, 3},
                                                  {2, 3},
                                                  {0, 2, 3},
                                                  {1, 2, 3},
                                                  {0, 1, 2, 3}};
    // uint8_t                             setup_already=0;
    uint8_t rach_time = 0;
    uint8_t bcchSlotStart = 1;
    uint8_t tbcchSlotStart = 1;
    uint8_t bbchSlotcfg = 1;
    uint8_t *IoTsiData = new uint8_t[IoTsi_LEN + 1];
    uint8_t *mibData = new uint8_t[MIB_LEN + 1];
    uint8_t *pcchData = new uint8_t[MIB_LEN + 1];
    uint8_t *sibData = new uint8_t[MAX_SIB_LEN + 1];
    uint8_t sibPdu[MAX_SIB_PDU_LEN];
    uint8_t sibSeg[8];
    int last_rnti = 0;
    int totSibLen[4], remSibLen[4];
    slot_sched_cfg_t txCfg[5];
    uint8_t pduData[5][MAX_PDU_LEN];
    int rar_window = 10;
    wx_sched::wx_dl_sched_res sched_result;
        //2024/8/16------------wwh
    typedef struct
    {
      int ta;
      int pa;
      int fa;
      int roid=0b00101011;
      uint8_t L = 4;
      uint16_t temp_crnti=-1;
      uint8_t chanType;
      int chanAssignment;
    }handover_info_t;
    handover_info_t handover_info;
    bool is_handover_rach=false;
    int HO_RACH_COUNT=0;
    //----------------------------------------------

    //2024/8/16------------wwh
    typedef struct
    {
      int BI;
      bool is_albe = false;
      uint8_t normalL = 11;     // normal
      uint8_t L = 11;           // normal
      uint8_t normalSychL = 14; // normal sychagch
      uint8_t ssL = 13;         // PDU长度
      uint8_t ssSychL = 17;     // SS sychagch
      int roid = 0b00101011;
      uint8_t LCID;
    } sub_header;

    typedef struct
    {
      bool isEnable = false;
      uint8_t *conSetupMsg;
      int conSetupMsgLen;
      uint32_t tti = -1;
      uint64_t consID;
      int considLen;
      int rnti;
    } ConsID;

    typedef struct
    {
      int sduLen;
      uint8_t *msg;
      sub_header subhead;
    } dl_sdu_t;

    struct band_freq
    {
      int BandID = 0;
      int freq = 0;
      int FrameAssign = 0;
      int SlotStart = 0;
    };
    struct macCfg
    {
      band_freq agch;
      band_freq bbch;
      band_freq sib;
      band_freq mib;
      band_freq IoTsi;
      band_freq rach;
      band_freq pcch;
      int rar_windows = 0;
      int Frame_Off = 0;
    };
    macCfg mac_cfg;
    // #################################################
    //********************V1.0.0****************************************
    void fillResourceMap();
    void fillResourceMap(uint8_t bbchFrameAssign, uint8_t agchFrameAssign);
    bool readIoTsi(int &IoTsiLen_);
    bool readMib(int &mibLen_, int sfn);
    bool readSib(int &sibLen_);
    bool readPcch(int &pcchLen_, int sfn);
    void generatePading(uint8_t *payload, uint8_t *pdu);
    void generatePtdchPdu(uint8_t *payload, uint8_t *pdu, int IoTsiLen);
    void generatePmbchPdu(uint8_t *payload, uint8_t **pcch_payload, uint8_t *pdu, int mibL, int pcchL, int index, bool is_last);
    void generateSibPdu(int si, uint8_t *payload, uint8_t *pdu, int sibLen);
    void SS_generateSibPdu(int si, uint8_t *payload, uint8_t *pdu, int sibLen);
    void initResourceMap();
    void iot_NM_generateAgchPdu(uint8_t *pdu, int &pduLen, std::vector<int> raid);
    void iot_SS_generateAgchPdu(uint8_t *pdu, int &pduLen, std::vector<int> raid);
    void SS_generateAgchPdu(uint8_t *pdu, int &pduLen, std::vector<int> raid);
    void SS_generatePsychAgchPdu(uint8_t *pdu, int &pduLen, std::vector<int> raid);
    void generateAgchPdu(uint8_t *pdu, int &pduLen, std::vector<int> raid);
    void generatePschPdu(uint8_t *pdu, int &pduLen);
    //2024/8/16------------wwh--
    void generateHOagchpdu(uint8_t* pdu, int& pduLen);
    //--------------------------------------
    void generatePsychAgchPdu(uint8_t *pdu, int &pduLen, std::vector<int> raid);
    void generatePdch2AgchPdu(uint8_t *pdu, int &pduLen, std::vector<int> raid);
    int get_IoT_sched(uint32_t tti, std::vector<sched_t> &dl_sched_res);
    int get_sched(uint32_t tti, std::vector<sched_t> &dl_sched_res, uint32_t prach_tti);
    void setHead(int bi, int ra_id, uint8_t *ptr, bool is_last);
    void iot_SS_setHead(int bi, int ra_id, uint8_t *ptr, bool is_last);
    void iot_NM_setHead(int bi, int ra_id, uint8_t *ptr, bool is_last);
    bool setConsID(set_up_msg setUp, dl_allocate allocSource, ul_allocate ulAlloc);
    bool TC300Timeout(set_up_msg setUp);
    bool setupInfo_uecategory14(set_up_msg setUp);
    void addlcidMap(uint32_t lcid, configMap configMap_);
    // void updateMap(uint32_t lcid,ChanType_t reconfig_new_ul_Type,ChanType_t reconfig_new_dl_Type);
    void updateMap(uint32_t lcid, configMap updateMap_);
    bool getVoiceLcid(uint32_t lcid, int voice_speed);
    /*20240612 xxk add*/
    bool setUecategory(uint8_t uecategory, uint16_t ue_cap14_band_id_, uint16_t ue_cap14_freq_id_, uint16_t ue_cap14_slot_, uint16_t ue_cap14_slot_no_handle_);
    bool getSrnti(int srnti);
    //---------------------------------2023.12.15--------------------
    bool reconf_phy(dl_allocate allocSource, ul_allocate ulAlloc, mcs_Info mcs_Info_, int Handover_frame_off=0); // recf->phy  chantype
    void mac_deallocate();
    bool wx_Switch_SetUser_in_TargetBeam();
    //-------------------------------------------
    //--------------------2024.09.04-------------
bool phy_channel_list_conf(phy_channel_list_t phy_channel_list, phy_channel_ul_list_t phy_ul_channel_list,mcs_Info mcs_Info_);
void RRC_notify_MAC_release(bool is_mac_chanType_release_);
//-------------------------------------------

  // //2024.11.12 to notify PHY resolution in advance
  // void updateUpInfo_advanceSingle();
  // void updateUpInfo_advanceMulti();

    void sychInit();
    int get_CI_state(uint32_t rnti);
    void sych_update();
    void getsych();

    void rarReset(int i);
    int rach_detected(prachInfo_t *prachInfo, uint8_t *pdu, int pduLen);
    int sych_detected(psychInfo_t *psychInfo);
    //*********************2023/09/15****************************
    int iot_rach_detected(iot_prachInfo_t *prachInfo, uint8_t *pdu, int pduLen);
    //****************************************************
    void sched_init(uint32_t tti, sched_t *sched_res, slot_sched_cfg_t &txCfg, phyChanType_t chanT);
    int push_pdu(uint32_t tti_rx, uint16_t rnti, uint8_t *pdu, uint32_t pduLen, bool crc, uint8_t voice);
    int parse_psych_pdu(uint32_t tti_rx, uint16_t rnti, uint8_t *pdu, uint32_t pduLen, bool crc);
    bool valid_tti(uint32_t tti, int ra_id);
    //*********************2023/09/15****************************
    bool valid_iot_tti(uint32_t tti, int ra_id);
    //****************************************************
    void generate_sdu(std::vector<dl_sdu_t> &macSdu, std::vector<int> raid);
    void generate_pdu(uint8_t *pdu, int &pduLen, std::vector<dl_sdu_t> sdu);
    // 11.06
    void generate_padding(uint8_t *pdu);
    void generate_Zeropadding(uint8_t *pdu);
    void generate_psch_padding(uint8_t *pdu);
    void generatePsychPdu(uint8_t *pdu);
    void generatePsychZeroPdu(uint8_t *pdu);
    void get_rar(int tti, std::vector<int> &raid);
    bool handover_rach();
    bool getTC300Timeout();
    void get_iot_rar(int tti, std::vector<int> &raid);
    void get_consinfo(int tti, std::vector<int> &raid);

    void setCfg();
    void readRachCfg(loc_info &bbch);
    void readTbcchCfg(loc_info &IoTsi);
    void readBbchCfg(loc_info &bbch);
    void readAgchCfg(loc_info &agch);
    void readSibCfg(loc_info &sib);
    void readPcchCfg(loc_info &pcch);

    void rlcStateReport(int lcid, int rnti);

    void update_ChanType(ChanType_t dl_Type_, ChanType_t ul_Type_);

    void getSch2(int &len, int &rnti);
    void getSch1(int &len, int &rnti,int slot);
    void generateSch2(uint8_t *pdu, int &pduLen);
    void generateZeroSch2(uint8_t *pdu, int &pduLen);
    void generateSch1(uint8_t *pdu, uint16_t &pui,int s);
    void transPUI(uint8_t *pdu, uint16_t &pui);
    void setAllocate(dl_allocate &allocSource);
    void setulAlloc(ul_allocate &allocSource);
    void set_uecategory14();
    void setupAllocate(dl_allocate &allocSource);
    void set_psch(int pduLen);

    //-----------------2024.03.04----------------------------
void MultisetupAllocate(phy_channel_t& allocSource);
//------------------------------

    bool is_reconf_release=false;




    adp *mac_adp;
    bool ttcn_mac_enble = false;
    bool ttcn_test_enble = false;
    bool TC300_Timeout=false;
    bool setup_flag = false;
    int tset_roid_not_match_count = 0;
    int tset_crid_not_match_count = 0;
    // adp* mac_adp;
    void get_general_interface_mac(srsran::unique_byte_buffer_t *pdu_);
    typedef struct
    {
      uint8_t direction;
      uint8_t rec_lay_id;
      uint8_t des_layer_id;
      uint16_t test_id;
      uint16_t data_length;
    } public_interface;
    public_interface mac_public_interface_; // ttcn中前七个不变的数据
    typedef struct
    {
      uint8_t msgType;
      uint8_t beam_barred;
      uint8_t q_min;
    } MAC_sib_info;
    MAC_sib_info mac_sib_info_;
    enum MAC_msgType
    {
      TEST_FLAG = 1,
      RRC_CONNECT_REQ = 4,
      RRC_CONNECT_COM = 6,
      MAC_RAR = 21,
      MAC_sync_power,
      MAC_ul_data_req,
      MAC_dl_data_req,
      MAC_beam_handover,
      MAC_window
    };
    //---------------------------2023.12.15-----------------------
    // void setrecfAllocate(dl_reconf& allocrecfSource );
    //-----------------------------------------------------
    //********************************************************************
  private:
    // V1.0.0########################

    //*********2023/09/15**********************************
    typedef struct
    {
      uint16_t temp_crnti = -1;
      int frameID = -1;
      uint8_t fa;
      uint8_t ta;
      uint8_t pa;
      uint8_t ScheduleInfolnd;    // 调度信息指示,为1时下行，0时上行调度信息
      uint8_t DLBandid;           // 下行频段标识，6bits。为接入网在下行调度信息中，为UE指定的下行频段号
      uint8_t DLFreqid;           // 下行频点标识，2bit，为接入网在下行调度信息中，为UE指定的下行载波号
      uint8_t DLScheduleType;     // 下行调度类型，1bit，指示下行调度为静态调度或动态调度
      uint8_t DLFnAssignment;     // 下行调度的帧资源分配，8bits，指示用于下行数传的相对帧号
      uint8_t DLSceheduleAmount;  // 下行动态调度的次数，3bits
      uint8_t DLScheduleInterval; // 下行动态调度的间隔，2bits
      uint8_t ULBandid;           // 上行频段标识，6bits，为接入网在上行调度信息中，为UE指定的下上行频段号
      uint8_t ULFreqid;           // 下行频点标识，2bit，为接入网在上行调度信息中，为UE指定的上行载波号
      uint8_t ULSubFreqid;        // 上行子载波标识，2bit，指示接入网为UE分配的上行子载波号
      uint8_t ULSchedUleType;     // 上行调度类型，1bit，指示上行调度静态/动态调度
      uint8_t ULFnAssignment;     // 上行调度的帧资源分配，8bits，指示用于上行数传的相对帧号
      uint8_t ULSceheduleAmount;  // 上行动态调度的次数，3bits，当ULScheduleType为动态调度时，用于指示上行动态调度的次数
      uint8_t ULScheduleInterval; // 上行动态调度的间隔，2bits。
      uint8_t ULTransmitCopies;   // 上行重复传输的副本数量，3bit，指示针对同一上行MAC PDU进行重复传输的副本数量
      uint8_t FreqSparedFactor;   // 下行扩频系数，2bit，指示下行扩频系数为128、256或512 扩频******************
      uint8_t CodeIndex;          //                                                                     扩频******************
      bool is_enable = false;
      sub_header subHead;
      ConsID considInfo;
    } iot_pending_rar_t1;
    iot_pending_rar_t1 iot_pending_rars1[MAX_PENDING_RARS];

    //*****************************************************

    typedef struct
    {
      uint8_t msg[400]; // zhj
      int rnti;
      uint32_t lcid;
      int len;
    } rlcState;

    std::deque<rlcState> rlcReport;

    // psch
    typedef struct
    {
      uint8_t s_rnti;
      uint8_t MCS;
      uint8_t L;
      uint8_t EI;
    } pui_t;
    typedef struct
    {
      uint8_t s_rnti1;
      uint8_t s_rnti2;
    } extend_pui_t;
    typedef struct
    {
      uint8_t L;
      uint8_t F;
      uint8_t LCID;
    } head_t;
    typedef struct
    {
      pui_t pui;
      extend_pui_t extend_pui;
      head_t head;
      uint8_t *msg;
    } psch_pdu_t;
    psch_pdu_t psch_pdu;

    typedef struct
    {
      uint16_t temp_crnti = -1;
      int frameID = -1;
      uint16_t ta;
      int fa;
      int pa;
      int direction;
      uint8_t chanType1 = 2;         // pdch1-1
      uint8_t chanType2 = 3;         // pdch1-2
      uint8_t chanType3 = 1;         // psych
      int chanAssignment1 = 0b1000;  // pdch1-1
      int chanAssignment2 = 0b11000; // pdch1-2
      int chanAssignment3 = 0b00100; // psych
      int ds_chanType1 = 1;
      int ds_chanAss1 = 1;
      bool is_enable = false;
      sub_header subHead;
      ConsID considInfo;
      int band_id = 9;

      // kuopin
      uint16_t PDTCHCodeIndex;
      uint8_t PDTCHType;
    } pending_rar_t1;
    pending_rar_t1 pending_rars1[MAX_PENDING_RARS];
    int nof_subHead = 0;
    // #################################################

    bool check_ue_active(uint16_t rnti);
    uint16_t allocate_ue(uint32_t enb_cc_idx);
    bool is_valid_rnti_unprotected(uint16_t rnti);

    srslog::basic_logger &logger;

    // We use a rwlock in MAC to allow multiple workers to access MAC simultaneously. No conflicts will happen since
    // access for different TTIs
    pthread_rwlock_t rwlock = {};

    // Interaction with PHY
    phy_interface_stack_lte *phy_h = nullptr;
    rlc_interface_mac *rlc_h = nullptr;
    rrc_interface_mac *rrc_h = nullptr;
    srsran::ext_task_sched_handle task_sched;

    cell_list_t cells = {};
    mac_args_t args = {};

    // derived from args
    srsran::task_multiqueue::queue_handle stack_task_queue;

    bool started = false;

    /* Scheduler unit */
    sched scheduler;
    // 卫星的调度器
    wx_sched wxScheduler;
    std::vector<sched_interface::cell_cfg_t> cell_config;

    sched_interface::dl_pdu_mch_t mch = {};

    /* Map of active UEs */
    static const uint16_t FIRST_RNTI = 0x46;
    // static const uint16_t            FIRST_RNTI = 0x32;
    rnti_map_t<unique_rnti_ptr<ue>> ue_db;
    std::atomic<uint16_t> ue_counter{0};

    uint8_t *assemble_rar(sched_interface::dl_sched_rar_grant_t *grants,
                          uint32_t enb_cc_idx,
                          uint32_t nof_grants,
                          uint32_t rar_idx,
                          uint32_t pdu_len,
                          uint32_t tti);

    const static int rar_payload_len = 128;
    std::array<srsran::rar_pdu, sched_interface::MAX_RAR_LIST> rar_pdu_msg;
    srsran::byte_buffer_t rar_payload[SRSRAN_MAX_CARRIERS][sched_interface::MAX_RAR_LIST];

    const static int NOF_BCCH_DLSCH_MSG = sched_interface::MAX_SIBS;

    const static int pcch_payload_buffer_len = 1024;
    struct common_buffers_t
    {
      uint8_t pcch_payload_buffer[pcch_payload_buffer_len] = {};
      srsran_softbuffer_tx_t bcch_softbuffer_tx[NOF_BCCH_DLSCH_MSG] = {};
      srsran_softbuffer_tx_t pcch_softbuffer_tx = {};
      srsran_softbuffer_tx_t rar_softbuffer_tx = {};
    };

    std::vector<common_buffers_t> common_buffers;

    const static int mcch_payload_len = 3000; // TODO FIND OUT MAX LENGTH
    int current_mcch_length = 0;
    uint8_t mcch_payload_buffer[mcch_payload_len] = {};
    srsran::mcch_msg_t mcch;
    srsran::sib2_mbms_t sib2;
    srsran::sib13_t sib13;
    const static int mtch_payload_len = 10000;
    uint8_t mtch_payload_buffer[mtch_payload_len] = {};
    // pointer to MAC PCAP object
    srsran::mac_pcap *pcap = nullptr;
    srsran::pcap_net *ppcap_net = nullptr;
    bool do_padding = false;
    dl_allocate dlAlloc; // allocSource_;
    ul_allocate ulAlloc_;
    dl_allocate tempSource;
    uint32_t mac_rnti = 0;
    /*20240605 add uecatecory*/
    uint8_t ue_category;
    int s_rnti_ = 0;
    
  phy_channel_t multidlAlloc;
  std::vector<phy_channel_t>lastMuiltSource;

  phy_ul_channel_t multiulAlloc;
  //------------------------2024.07.13multi-----------------------------
  int sigle_multi=1;
  // phy_channel_t multidlAlloc;
  // phy_ul_channel_t multiulAlloc;
  // mac_phy_channel_t macmuldlAlloc;
  //--------------------------------------------------------------------

    dl_reconf reconf_allo_;
    int chanLen = 19;
    // Number of rach preambles detected for a cc.
    std::vector<uint32_t> detected_rachs;

    // Softbuffer pool
    std::unique_ptr<srsran::obj_pool_itf<ue_cc_softbuffers>> softbuffer_pool;
  };
  // psych
  typedef struct
  {
    uint8_t CI = 5;
    uint8_t LCID;
    uint8_t CQI;
    uint8_t PA;
  } psych_pdu;
  typedef struct
  {
    float ta;
    int fa;
    float pa;
  } psych_ce;

  // extern float time_tolerance; //时间容差
  // extern float time_tolerance; //时间容差
  // extern int freq_toleranceMax; //频率容差
  // extern int freq_toleranceMin; //频率容差
  extern uint8_t psychSS;
  extern float overtimeT;
  extern int sendSychTime;
  extern float notAdjustT;
  extern int adjust_flag;
  extern int adjust_start;
  extern int adjust_interval;
} // namespace srsenb

#endif // SRSENB_MAC_H
