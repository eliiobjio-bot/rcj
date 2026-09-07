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

#include "srsenb/hdr/stack/mac/sched_interface.h"
#include "srsran/interfaces/rrc_interface_types.h"
#include "srsran/adt/circular_map.h"

#ifndef SRSRAN_ENB_MAC_INTERFACES_H
#define SRSRAN_ENB_MAC_INTERFACES_H

namespace srsenb
{

  // ###################V1.0.0########################
  typedef enum
  {
    NULLTYPE = 0,
    MBCH,
    SIB,
    BBCH,
    BCCH,
    AGCH,
    FCCH,
    SCH,
    SCH1,
    TBCCH, // IOT,物联网广播控制系统
    DTCH,
    DCCH, // iot的共享信道
    CCCH,
    TDDCH, // IOT,物联网下行数据信道
    TUDCH, // IOT,物联网上行数据信道
    SCH2,
    SYCH,
    DCH,
    PCCH,
  } logicChanType_t;

  typedef enum
  {
    null = 0,
    PMBCH,
    PRACH,
    PDCH,  // 接入网
    PSCH,  // 接入网
    PSYCH, // 接入网
    PFCCH,
    PTDCH, // 物联网下行物理信道
    PTSCH, // 是否存在？
    PSBCH,
    PTUCH, // 物联网上行物理信道
    PDCH1_1,
    PDCH1_2,

    // kuopin
    DS_SCH,
    DS_CPICH,
    DS_PBCH,
    DS_PPCH,
    DS_PDTCH_1,
    DS_PDTCH_2,
    DS_PTDCH_3,
    DS_PTDCH_T,

    DSPDTCH, // 扩频
  } phyChanType_t;

  typedef enum
  {
    psych0,
    pdch1_1,
    pdch1_2,
    psch1_1,
    psch1_2,
    psch5_1,
    psch5_2,
    ds_pdtch_1,
    ds_pdtch_2,
    ds_pdtch_3,
    ds_pdtch_t,
    // dtch1, //business
    // dtch2, //ims signal
    // dtch3, //voicec
    // dtch4,
    nulltype
  } ChanType_t;

  // zyg //for The resource of the corresponding label
  typedef enum
  {
    Control = 0,
    Data,
    Voice
  } DataType;

  typedef enum
  {
    rlc_am = 0,
    rlc_um,
    rlc_tm
  } RlcType;

  typedef enum
  {
    voice_24k = 0,
    voice_48k,
    voice_800,
    N_Voice
  } VoiceType;

  typedef struct
  {
    uint8_t msg[400] = {0}; // Used to receive data from the upper layer
    VoiceType voicetype;
    ChanType_t ul_Type;
    ChanType_t dl_Type;
    DataType dataType;
    RlcType rlcType;
  } configMap;

  typedef struct
  {
    bool is_data_service_switching = false;
    int MCS = 0;
  } mcs_Info;
  // zyg

  typedef struct
  {
    phyChanType_t burstID; // 逻辑信道标识
    bool equiFlag;         // 扩展标志
    bool udFlag;           // 上下行标志
    int payloadType = 0;   // 负载类型（控制数据/语音）
    int DSType = 1;
    uint8_t pduBits[400];
    int pduBitlen = -1;
    int modeType;
    uint16_t pui;

    // kuopin
    int SF_i = 20;       // 扩频码索引
    int subframe_offset; // 针对DS-PTDCH_T
    float Power = 0.0;   // 各信道功率
  } slot_sched_cfg_t;
  // ########################################

  struct mac_args_t
  {
    uint32_t nof_prb; ///< Needed to dimension MAC softbuffers for all cells
    sched_interface::sched_args_t sched;
    int lcid_padding;
    uint32_t nof_prealloc_ues; ///< Number of UE resources to pre-allocate at eNB startup
    uint32_t max_nof_kos;
    int rlf_min_ul_snr_estim;

    uint32_t network_mode;
    uint32_t area_mode;
    uint32_t multi_beam_num;

    int mac_init_frame_off = 0;

    bool ttcn_mac_enble = false;
    int pid;
  };

  typedef struct
  {
    int rnti;
    uint64_t consID;
    int consIDlen;
    uint8_t *pdu;
    int pduLen;
  } set_up_msg; // setup

  typedef struct
  {
    ChanType_t Type;
    logicChanType_t LogicType;
    int pduLen=0;
    int bandID=0;
    int solt=0;
    int freq=0;
    int SF_i=0;
  } dl_allocate; // setup
  typedef struct
  {
    ChanType_t Type;
    logicChanType_t LogicType;
    int pduLen;
    int bandID;
    int solt;
    int freq;
  } ul_allocate; // setup
  //-----------------------2023.12.15--------------------
  typedef struct
  {
    ChanType_t Type_recf;
    logicChanType_t LogicType_recof;
    int pduLen;
    int srnti;
    int bandID;
    int freq;
    int solt;
  } dl_reconf;
  //---------------------------------------------------------





typedef enum {
    biDirection, 
    ulDirection, 
    dlDirection, 
  
}direct_opt_t;

typedef enum   {
    Static, 
    dynamic, 
  
}sche_type_opts_t;

typedef struct {
    int srnti;
    int pduLen;
    int crc;
    logicChanType_t  LogicType;
    ChanType_t  Type;
    int bandID;
    int freqID;
    int slot;
    int SF_i;
    direct_opt_t direct_opt;
    sche_type_opts_t sche_type_opt;

}phy_ul_channel_t;

typedef struct  {
    int srnti;
    int pduLen;
    int crc;
    logicChanType_t  LogicType;
    ChanType_t  Type;
    int bandID;
    int freqID;
    int slot;
    int SF_i;
    direct_opt_t direct_opt;
    sche_type_opts_t sche_type_opt;

}phy_channel_t;





  /* Interface PHY -> MAC */
  class mac_interface_phy_lte
  {
  public:
    const static int MAX_GRANTS = 64;
    // V1.0.0######################################
    typedef struct
    {
      int framID;   // 帧号
      int burstNum; // 帧号中的触发数
      int BandID = 0;
      int txFreq = 0;                   // 传输频点
      slot_sched_cfg_t burstPrasCfg[5]; // 5个时隙的详细配置

      // kuopin
      int Freq_size;      // 传输频点个数
      int Freq_nof;       // 传输频点个数
      int Scrambling_num; // 基本扰码（0,16,32,48）
      int k_s;            // 码组编号（0-11）
      int ds_mode;
    } sched_t;

    typedef struct
    {
      int framID;
      float ta;
      float fa;
      float pa;
      bool crc;
      int roid;
    } prachInfo_t; // RACH

    typedef struct
    {
      float ta;
      float fa;
      float pa;
      bool crc;
    } psychInfo_t; // PSYCH
                   //*******2023/09/15*************************
    typedef struct
    {
      int framID;
      int ta;
      int fa;
      int pa;
      bool crc;
      int roid;
    } iot_prachInfo_t; // iot RACH
                       //**************************************

    // ######################################
    /**
     * DL grant structure per UE
     */
    struct dl_sched_grant_t
    {
      srsran_dci_dl_t dci = {};
      uint8_t *data[SRSRAN_MAX_TB] = {};
      srsran_softbuffer_tx_t *softbuffer_tx[SRSRAN_MAX_TB] = {};
    };
    /**
     * DL Scheduling result per cell/carrier
     */
    typedef struct
    {
      dl_sched_grant_t pdsch[MAX_GRANTS]; //< DL Grants
      uint32_t nof_grants;                //< Number of DL grants
      uint32_t cfi;                       //< Current CFI of the cell, it can vary across cells
    } dl_sched_t;
    /**
     * List of DL scheduling results, one entry per cell/carrier
     */
    using dl_sched_list_t = srsran::bounded_vector<dl_sched_t, SRSRAN_MAX_CARRIERS>;
    typedef struct
    {
      uint16_t rnti;
      bool ack;
    } ul_sched_ack_t;
    /**
     * UL grant information per UE
     */
    typedef struct
    {
      srsran_dci_ul_t dci;
      uint32_t pid;
      uint32_t current_tx_nb;
      uint8_t *data;
      bool needs_pdcch;
      srsran_softbuffer_rx_t *softbuffer_rx;
    } ul_sched_grant_t;

    /**
     * UL Scheduling result per cell/carrier
     */
    typedef struct
    {
      ul_sched_grant_t pusch[MAX_GRANTS];
      ul_sched_ack_t phich[MAX_GRANTS];
      uint32_t nof_grants;
      uint32_t nof_phich;
    } ul_sched_t;

    /**
     * List of UL scheduling results, one entry per cell/carrier
     */
    using ul_sched_list_t = srsran::bounded_vector<ul_sched_t, SRSRAN_MAX_CARRIERS>;
    // V1.0.0#################################
    virtual int get_sched(uint32_t tti, std::vector<sched_t> &dl_sched_res, uint32_t prach_tti) = 0;
    virtual int get_IoT_sched(uint32_t tti, std::vector<sched_t> &dl_sched_res) = 0;
    virtual int rach_detected(prachInfo_t *prachInfo, uint8_t *pdu, int pduLen) = 0;
    virtual int sych_detected(psychInfo_t *psychInfo) = 0;
    // virtual int sych_update(psychInfo_t* psychInfo)                              =0;
    virtual int iot_rach_detected(iot_prachInfo_t *iot_prachInfo, uint8_t *pdu, int pduLen) = 0;
    virtual int push_pdu(uint32_t tti_rx, uint16_t rnti, uint8_t *msg, uint32_t pduLen, bool crc, uint8_t voice) = 0;
    // #################################################
    virtual int sr_detected(uint32_t tti, uint16_t rnti) = 0;
    virtual void rach_detected(uint32_t tti, uint32_t primary_cc_idx, uint32_t preamble_idx, uint32_t time_adv) = 0;
    /**
     * PHY callback for giving MAC the Rank Indicator information of a given RNTI for an eNb cell/carrier.
     *
     * @param tti the given TTI
     * @param rnti the UE identifier in the eNb
     * @param cc_idx The eNb Cell/Carrier where the measurement corresponds
     * @param ri_value the actual Rank Indicator value, 0 for 1 layer, 1 for two layers and so on.
     * @return SRSRAN_SUCCESS if no error occurs, SRSRAN_ERROR* if an error occurs
     */
    virtual int ri_info(uint32_t tti, uint16_t rnti, uint32_t cc_idx, uint32_t ri_value) = 0;
    /**
     * PHY callback for giving MAC the Pre-coding Matrix Indicator information of a given RNTI for an eNb cell/carrier.
     *
     * @param tti the given TTI
     * @param rnti the UE identifier in the eNb
     * @param cc_idx The eNb Cell/Carrier where the measurement corresponds
     * @param pmi_value the actual PMI value
     * @return SRSRAN_SUCCESS if no error occurs, SRSRAN_ERROR* if an error occurs
     */
    virtual int pmi_info(uint32_t tti, uint16_t rnti, uint32_t cc_idx, uint32_t pmi_value) = 0;

    /**
     * PHY callback for for giving MAC the Channel Quality information of a given RNTI, TTI and eNb cell/carrier
     * @param tti the given TTI
     * @param rnti the UE identifier in the eNb
     * @param cc_idx The eNb Cell/Carrier where the measurement corresponds
     * @param cqi_value the corresponding Channel Quality Information
     * @return SRSRAN_SUCCESS if no error occurs, SRSRAN_ERROR* if an error occurs
     */
    virtual int cqi_info(uint32_t tti, uint16_t rnti, uint32_t cc_idx, uint32_t cqi_value) = 0;

    /**
     * PHY callback for giving MAC the Channel Quality information of a given RNTI, TTI, eNb cell/carrier for a specific
     * subband
     * @param tti the given TTI
     * @param rnti the UE identifier in the eNb
     * @param cc_idx The eNb Cell/Carrier where the measurement corresponds
     * @param sb_idx Index of the Sub-band
     * @param cqi_value the corresponding Channel Quality Information
     * @return SRSRAN_SUCCESS if no error occurs, SRSRAN_ERROR* if an error occurs
     */
    virtual int sb_cqi_info(uint32_t tti, uint16_t rnti, uint32_t enb_cc_idx, uint32_t sb_idx, uint32_t cqi_value) = 0;

    typedef enum
    {
      PUSCH = 0,
      PUCCH,
      SRS
    } ul_channel_t;

    /**
     * PHY callback for giving MAC the SNR in dB of an UL transmission for a given RNTI at a given carrier
     *
     * @param tti The measurement was made
     * @param rnti The UE identifier in the eNb
     * @param cc_idx The eNb Cell/Carrier where the UL transmission was received
     * @param snr_db The actual SNR of the received signal
     * @param ch Indicates uplink channel (PUSCH, PUCCH or SRS)
     * @return SRSRAN_SUCCESS if no error occurs, SRSRAN_ERROR* if an error occurs
     */
    virtual int snr_info(uint32_t tti, uint16_t rnti, uint32_t cc_idx, float snr_db, ul_channel_t ch) = 0;

    /**
     * PHY callback for giving MAC the Time Aligment information in microseconds of a given RNTI during a TTI processing
     *
     * @param tti The measurement was made
     * @param rnti The UE identifier in the eNb
     * @param ta_us The actual time alignment in microseconds
     * @return SRSRAN_SUCCESS if no error occurs, SRSRAN_ERROR* if an error occurs
     */
    virtual int ta_info(uint32_t tti, uint16_t rnti, float ta_us) = 0;

    /**
     * PHY callback for giving MAC the HARQ DL ACK/NACK feedback information for a given RNTI, TTI, eNb cell/carrier and
     * Transport block.
     *
     * @param tti the given TTI
     * @param rnti the UE identifier in the eNb
     * @param cc_idx the eNb Cell/Carrier identifier
     * @param tb_idx the transport block index
     * @param ack true for ACK, false for NACK, do not call for DTX
     * @return SRSRAN_SUCCESS if no error occurs, SRSRAN_ERROR* if an error occurs
     */
    virtual int ack_info(uint32_t tti, uint16_t rnti, uint32_t cc_idx, uint32_t tb_idx, bool ack) = 0;

    /**
     * Informs MAC about a received PUSCH transmission for given RNTI, TTI and eNb Cell/carrier.
     *
     * This function does not deallocate the uplink buffer. The function push_pdu() must be called after this
     * to inform the MAC that the uplink buffer can be discarded or pushed to the stack
     *
     * @param tti the given TTI
     * @param rnti the UE identifier in the eNb
     * @param cc_idx the eNb Cell/Carrier identifier
     * @param nof_bytes the number of grants carrierd by the PUSCH message
     * @param crc_res the CRC check, set to true if the message was decoded succesfully
     * @return SRSRAN_SUCCESS if no error occurs, SRSRAN_ERROR* if an error occurs
     */
    virtual int crc_info(uint32_t tti, uint16_t rnti, uint32_t cc_idx, uint32_t nof_bytes, bool crc_res) = 0;

    /**
     * Pushes an uplink PDU through the stack if crc_res==true or discards it if crc_res==false
     *
     * @param tti the given TTI
     * @param rnti the UE identifier in the eNb
     * @param enb_cc_idx the eNb Cell/Carrier identifier
     * @param nof_bytes the number of grants carrierd by the PUSCH message
     * @param crc_res the CRC check, set to true if the message was decoded succesfully
     * @param ul_nof_prbs Number of PRBs allocated to grant
     * @return SRSRAN_SUCCESS if no error occurs, SRSRAN_ERROR* if an error occurs
     */
    virtual int push_pdu(uint32_t tti_rx,
                         uint16_t rnti,
                         uint32_t enb_cc_idx,
                         uint32_t nof_bytes,
                         bool crc_res,
                         uint32_t ul_nof_prbs,
                         uint8_t voice) = 0;

    virtual int get_dl_sched(uint32_t tti, dl_sched_list_t &dl_sched_res) = 0;
    virtual int get_mch_sched(uint32_t tti, bool is_mcch, dl_sched_list_t &dl_sched_res) = 0;
    virtual int get_ul_sched(uint32_t tti, ul_sched_list_t &ul_sched_res) = 0;
    virtual void set_sched_dl_tti_mask(uint8_t *tti_mask, uint32_t nof_sfs) = 0;
  };

  class mac_interface_rlc
  {
  public:
    virtual int rlc_buffer_state(uint16_t rnti, uint32_t lc_id, uint32_t tx_queue, uint32_t retx_queue) = 0;
    virtual void rlcStateReport(int lcid, int rnti) = 0;
  };


//----------------------------2024.03.04---------------------
struct phy_channel_list {
    phy_channel_t  phy_channel = {};
};
typedef std::vector<phy_channel_list> phy_channel_list_t;



struct phy_ul_channel_list {
    phy_ul_channel_t  phy_channel = {};
};
typedef std::vector<phy_ul_channel_list> phy_channel_ul_list_t;

//-------------------------------------------------------------------------



  class mac_interface_rrc
  {
  public:
    /* Provides cell configuration including SIB periodicity, etc. */
    virtual int cell_cfg(const std::vector<sched_interface::cell_cfg_t> &cell_cfg) = 0;
    /* Manages UE configuration context */
    virtual int ue_cfg(uint16_t rnti, const sched_interface::ue_cfg_t *cfg) = 0;
    virtual int ue_rem(uint16_t rnti) = 0;

    /**
     * Called after Msg3 reception to set the UE C-RNTI, resolve contention, and alter the UE's configuration in the
     * scheduler and phy.
     *
     * @param temp_crnti temporary C-RNTI of the UE
     * @param crnti chosen C-RNTI for the UE
     * @param cfg new UE scheduler configuration
     */
    virtual int ue_set_crnti(uint16_t temp_crnti, uint16_t crnti, const sched_interface::ue_cfg_t &cfg) = 0;

    /* Manages UE bearers and associated configuration */
    virtual int bearer_ue_cfg(uint16_t rnti, uint32_t lc_id, mac_lc_ch_cfg_t *cfg) = 0;
    virtual int bearer_ue_rem(uint16_t rnti, uint32_t lc_id) = 0;
    virtual void phy_config_enabled(uint16_t rnti, bool enabled) = 0;
    virtual void write_mcch(const srsran::sib2_mbms_t *sib2_,
                            const srsran::sib13_t *sib13_,
                            const srsran::mcch_msg_t *mcch_,
                            const uint8_t *mcch_payload,
                            const uint8_t mcch_payload_length) = 0;

    /**
     * Allocate a C-RNTI for a new user, without adding it to the phy layer and scheduler yet
     * @return value of the allocated C-RNTI
     */
    virtual uint16_t reserve_new_crnti(const sched_interface::ue_cfg_t &ue_cfg) = 0;
    // V1.0.1####################################################################
    virtual bool setConsID(set_up_msg setUp, dl_allocate allocSource, ul_allocate ulAlloc) = 0;
    virtual bool TC300Timeout(set_up_msg setUp)=0;
    virtual bool setupInfo_uecategory14(set_up_msg setUp) = 0;

    virtual void addlcidMap(uint32_t lcid, configMap configMap_) = 0;
    // virtual void updateMap(uint32_t lcid,ChanType_t reconfig_new_ul_Type,ChanType_t reconfig_new_dl_Type)=0;
    virtual void updateMap(uint32_t lcid, configMap updateMap_) = 0;
    virtual bool getVoiceLcid(uint32_t lcid, int voice_speed) = 0;
    /*20240612 xxk add */
    virtual bool setUecategory(uint8_t uecategory, uint16_t ue_cap14_band_id_, uint16_t ue_cap14_freq_id_, uint16_t ue_cap14_slot_, uint16_t ue_cap14_slot_no_handle_) = 0; // handle is >>
    virtual bool getSrnti(int srnti) = 0;




     virtual void RRC_notify_MAC_release(bool is_mac_chanType_release_)=0;

    //------------------------2023.12.15------------------------------------
    virtual bool reconf_phy(dl_allocate allocSource, ul_allocate ulAlloc, mcs_Info mcs_Info_, int Handover_frame_off=0) = 0;
    virtual void mac_deallocate() = 0;
    virtual bool wx_Switch_SetUser_in_TargetBeam() = 0;
    //----------------------------------------------------------
//--------------2024.09.04--------------------
  virtual bool phy_channel_list_conf( phy_channel_list_t phy_channel_list, phy_channel_ul_list_t phy_ul_channel_list,mcs_Info mcs_Info_)=0;
 //----------------------------------------------------------

  // //2024.11.12 to notify PHY resolution in advance
  // virtual void updateUpInfo_advanceSingle()=0;
  // virtual void updateUpInfo_advanceMulti()=0;
  // //------------------------------------------

    virtual void rlcStateReport(int lcid, int rnti) = 0;
  };

  class mac_interface_sched
  {
  public:
    virtual bool readIoTsi(int &IoTsiLen_) = 0;
    virtual bool readMib(int &mibLen_, int sfn) = 0;
    virtual bool readPcch(int &pcchLen_, int sfn) = 0;
    virtual bool readSib(int &sibLen_) = 0;
    virtual void get_rar(int tti, std::vector<int> &raid) = 0;
    virtual bool handover_rach()=0;
    virtual bool getTC300Timeout()=0;
    virtual void readRachCfg(loc_info &rach) = 0;
    virtual void readFrameOffCfg(int &sched_frameOff) = 0;
    virtual void readBbchCfg(loc_info &bbch) = 0;
    virtual void readAgchCfg(loc_info &agch) = 0;
    virtual void readTbcchCfg(loc_info &IoTsi) = 0;
    virtual void readSibCfg(loc_info &sib) = 0;
    virtual void readPcchCfg(loc_info &pcch) = 0;
    virtual void get_consinfo(int tti, std::vector<int> &raid) = 0;
    virtual void getSch2(int &len, int &rnti) = 0;
    virtual void getSch1(int &len, int &rnti,int slot) = 0;
    virtual void getsych() = 0;
    virtual void sych_update() = 0;
    virtual int get_CI_state(uint32_t rnti) = 0;
  };

  // Combined interface for PHY to access stack (MAC and RRC)
  class stack_interface_phy_lte : public mac_interface_phy_lte
  {
  };

} // namespace srsenb

#endif // SRSRAN_ENB_MAC_INTERFACES_H
