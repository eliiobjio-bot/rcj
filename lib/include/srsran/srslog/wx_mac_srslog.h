#include <iostream>
#include <sstream>

/**
  *@brief  Copy from IOT,modify Access
  *@author zhaohongjun
  *@date   2024/05/18
  */


//RA
#define IMAC_RA_RACH_DETECT_LOG(v1,v2)              logger.info("[IoT][RA]detected IoT RACH. prach frameID:%d. allocate rnti:%d.", v1,v2); // presented when mac detect rach
#define NMAC_RA_RACH_DETECT_LOG(v1,v2)              logger.info("[NB][RA]detected NB RACH. prach frameID:%d. allocate rnti:%d.", v1,v2); // presented when mac detect rach
#define MAC_RA_RAR_SCHED_LOG(v1,v2,v3)              logger.info("[WX][RA]AGCH is Sched: cur_tti;%d, prach_tti:%d, rar_win:%d.", v1,v2,v3); // presented when AGCH is ready to sched,i.e.,  tx tti within rar window
#define NMAC_RA_RCV_CONSID_LOG(v1,v2,v3);           logger.debug("[NB][RA]MAC rcv ConsID push to phy.rnti:%d, ConsID:%d, len:%d.",v1,v2,v3); 

#define MAC_PMBCH_SCHED_OCCA_LOG(v1,v2,v3,v4)       logger.info("[WX][PMBCH]mode:%d,tti:%d(sf:%d,slot:%d)mib/paging schedule occasion.", v1,v2,v3,v4); // current tti,(sf, slot) is for mib/paging occasion
#define MAC_PMBCH_PAGING_RSLT_LOG(v1)               logger.info("[WX][PAGING]get paging len:%d.", v1);
#define IMAC_PSBCH_SCHED_OCCA_LOG(v1,v2,v3)         logger.info("[IoT][PSBCH]tti:%d(sf:%d,slot:%d)sib schedule occasion.",v1,v2,v3); // current tti,(sf, slot) is for sib occasion

//IOT PTDCH
#define IMAC_PTDCH_AGCH_LOG                         logger.info("[IoT][RA][PTDCH]RAR is sched."); // AGCH is multiplexing @ptdch process stage
#define IMAC_PTDCH_SCHED_AGCH_LOG(v1,v2,v3,v4,v5,v6,v7,v8,v9)                   logger.info("[IoT][SCHED][PTDCH]UE:%d ul schedinfo updated by AGCH:\n\
                                                                                ulFreqId:%d,\n\
                                                                                ulBandId:%d,\n\
                                                                                ulSchedInterval:%d,\n\
                                                                                ulSchedAmount:%d,\n\
                                                                                ulFnAssignment:%d,\n\
                                                                                ulSubFreqId:%d,\n\
                                                                                ulTransCpy:%d,\n,\
                                                                                ulSchedType:%d,\n",\
                                                                                v1,v2,v3,v4,v5,v6,v7,v8,v9); 
#define IMAC_PTDCH_IOTSI_LOG                        logger.info("[IoT][PTDCH]IOTsi is sched."); // IOTSI is multiplexing @ptdch process stage
#define IMAC_PTDCH_OCCASION_LOG(v1,v2)              logger.info("[IoT][PTDCH]PTDCH occasion, tti:%d(sf:%d).",v1,v2);
#define IMAC_PTDCH_DATA_LOG(v1)                     logger.info("[IoT][PTDCH]data is sched. rem for rlc=%d", v1);
#define IMAC_PTDCH_QUERY_RLC_DATA_LOG(v1,v2)        logger.info("[IoT][PTDCH]query RLC results, total %d logic channel(s), cur lcid=%d has data.", v1,v2); // current ue has dl grant but rlc buffer has no data to tx.
#define IMAC_PTDCH_DATA_RLC_1_LOG(v1)               logger.info("[IoT][PTDCH]total 1 RLC pdu, LCID=%d", v1); // total 1 lcid has data to tx. i.e., 1 mac sdu
#define IMAC_PTDCH_DATA_RLC_N_LOG(v1)               logger.info("[IoT][PTDCH]total %d RLC pdus", v1); // more than 1 lcid have data to tx
#define IMAC_PTDCH_STATUS_LOG(v1,v2,v3,v4,v5)       logger.info("[IoT][PTDCH]network_mode:%d, data size:%d; AGCH:%d, iotsi:%d, data:%d.", v1,v2,v3,v4,v5); // set to 1 means the data type is muxed
#define IMAC_PTDCH_PROC_DONE_LOG(v1,v2)             logger.info("[IoT][PTDCH](%d,%d) PTDCH process done.",v1,v2);



//DL INFO
#define MAC_DL_PUSH_TO_PHY_DATA_LOG(v1,v2,v3);     logger.debug("[RAN][WXMAC]data push to phy.tti:%d, slot:%d data:%s",v1,v2,v3); 
#define MAC_DL_PUSH_TO_PHY_LOG(v1)                 logger.debug("[RAN][WXMAC]MAC push to PHY: tti:%d", v1);
#define MAC_DL_FREQ_CHANNEL_INFO_0_LOG(v1,v2,v3)   logger.debug("[RAN][WXMAC]slot:0, bd:%d, fq:%d, channel:%d",v1,v2,v3);
#define MAC_DL_FREQ_CHANNEL_INFO_1_LOG(v1,v2,v3)   logger.debug("[RAN][WXMAC]slot:1, bd:%d, fq:%d, channel:%d",v1,v2,v3);
#define MAC_DL_FREQ_CHANNEL_INFO_2_LOG(v1,v2,v3)   logger.debug("[RAN][WXMAC]slot:2, bd:%d, fq:%d, channel:%d",v1,v2,v3); 
#define MAC_DL_FREQ_CHANNEL_INFO_3_LOG(v1,v2,v3)   logger.debug("[RAN][WXMAC]slot:3, bd:%d, fq:%d, channel:%d",v1,v2,v3); 
#define MAC_DL_FREQ_CHANNEL_INFO_4_LOG(v1,v2,v3)   logger.debug("[RAN][WXMAC]slot:4, bd:%d, fq:%d, channel:%d",v1,v2,v3);

//PADING
#define PADING_INFO(v1)                            logger.info("[WX][DL][PADING]tti:%d ,",v1);

// NB GEN PDU
#define NMAC_DL_PDU_LOG(v1,v2,v3)                   logger.debug("[NB][PDU]SDU:   set_sdu(), lcid=%d, sdu_len=%d, sdu_space=%d", lcid, sdu_len, sdu_space);


 //UL
#define MAC_UL_PUSH_PDU_INFO_LOG(v1,v2,v3,v4,v5);   logger.debug("[WX][PDU][UL]push pdu tti:%d,rnti:%d, crc:%d, size:%d, data:%s",v1,v2,v3,v4,v5); // ul pdu received from phy
#define MAC_UL_PDU_INFO_LOG(v1,v2);                 logger.debug("[WX][PDU][UL]PDU TB size:%d, data:%s",v1,v2); // ul pdu received from phy
#define MAC_UL_SUBHDR_INFO_LOG(v1,v2,v3);           logger.debug("[WX][PDU][UL]current subhdr info, lcid:%d,l_bit:%d,e_bit:%d",v1,v2,v3);
#define MAC_ROUTE_PDU_TO_RLC_LOG(v1,v2,v3);         logger.debug("[WX][PDU][UL]route PDU to RLC.rnti:%d, lcid:%d, size:%d",v1,v2,v3);
#define MAC_UL_SDU_INFO_LOG(v1,v2);                 logger.debug("[WX][PDU][UL]SDU size:%d, data:%s",v1,v2); // info of sdu forward to rlc
#define MAC_UL_VOICE_INFO_LOG(v1,v2);               logger.info("[WX][VOICE PDU][UL]lcid:%d ,data:%s",v1,v2);

//sched
#define IMAC_SCHED_DATA_LOG(v1,v2,v3,v4,v5)         logger.info("[IoT][SCHED]tti:%d,fq:%d,bd:%d.rnti:%d is sched.(dla:%x)",v1,v2,v3,v4,v5);
#define MAC_SCHED_REM_UE_LOG(v1);                   logger.debug("[WX][SCHED]UE rem. rnti:%d is erased.",v1); 

// MAC CE
#define IMAC_UL_BSR_RCVD(v1,v2);                    logger.info("[IoT][PDU][BSR]BSR index:%d, size:less than %d",v1,v2); // info of IOT BSR

#define CONVERT_PDU_TO_HEX(pdu_data, pdu_size, result) \
    do { \
        uint8_t* pdu_ptr = pdu_data; \
        std::ostringstream ss; \
        ss << std::hex << std::setfill('0'); \
        for (int j = 0; j < static_cast<int>(pdu_size); j++) { \
            ss << "0x" << std::setw(2) << static_cast<int>(*pdu_ptr) << ","; \
            pdu_ptr++; \
        } \
        result = std::move(ss).str(); \
    } while (0)

#define IMAC_TMP_AVOID_SIB_COLLISION_LOG(v1)             logger.info("[IoT][TMP]currrnt tti:%d, do not sched ptdch",v1);
#define IMAC_TMP_PCCH_OFFSET_LOG(v1)             logger.info("[IoT][TMP]PCCH OFFSET:%d",v1);
#define IMAC_TMP_PAGING_DATA_LOG(v1);     logger.debug("[IoT][TMP]paging data:%s",v1); 