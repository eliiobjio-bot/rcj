#include "srsenb/hdr/common/common_enb.h"
#include <atomic>
#include <map>
#include <mutex>
#include <vector>
#include "srsran/interfaces/enb_mac_interfaces.h"
#include <pthread.h>
#include <string.h>
#include "srsenb/hdr/stack/mac/mac.h"
#include "srsran/common/rwlock_guard.h"
#include "srsran/common/time_prof.h"
#include "srsran/interfaces/enb_rlc_interfaces.h"
#include "srsran/interfaces/enb_rrc_interfaces.h"
#include "srsran/srslog/event_trace.h"
#include "srsenb/hdr/stack/mac/sched_interface.h"
#ifndef SRSENB_WXSCHEDULER_H
#define SRSENB_WXSCHEDULER_H
//***************V1.0.0********************
#define MaxBandID 56
#define MaxCarrierID 4
#define MaxSlotID 5
#define MAX_SF 52
#define ALLOC_START 5
#define ALLOC_END 52
#define ALLOC_INTERVAL 1
//*******************************************

namespace srsenb
{
  class wx_sched
  {

  public:
    // 11.06
    uint8_t steup_already = 0;
    uint8_t ue_category = 0;
    uint16_t ue_cap14_band_id;
    uint16_t ue_cap14_freq_id;
    uint16_t ue_cap14_slot;
    int prev_Handover_frame_off = 0;    // Double_frame_off
    int current_Handover_frame_off = 0; // Double_frame_off
    int Reselect_frame_off = 0;

    std::chrono::_V2::system_clock::time_point second_time;
    std::chrono::_V2::system_clock::time_point second_time_end;

    struct dl_sched_IoTsi_t
    {
      slot_sched_cfg_t IoTsiCfg;
      int IoTsiLen = 0;
      bool is_sched = false;
    };
    struct dl_sched_mib_t
    {
      slot_sched_cfg_t mibCfg;
      slot_sched_cfg_t pcchCfg;
      int mibLen = 0;
      int pcchLen = 0;
      int si = 0;
      int index;
      bool is_sched = false;
      bool is_last = false;
    };
    struct dl_sched_sib_t
    {
      slot_sched_cfg_t sibCfg;
      int index = 0;
      int si = 0;
      int sibLen = 0;
      bool is_sched = false;
    };
    struct dl_sched_rar_t
    {
      slot_sched_cfg_t rarCfg;
      std::vector<int> raid;
      int rnti;
      bool is_sched = false;
    };
    struct dl_sched_iot_rar_t
    {
      slot_sched_cfg_t rarCfg;
      std::vector<int> raid;
      int rnti;
      bool is_sched = false;
    };
    struct dl_sched_data_t
    {
      slot_sched_cfg_t dataCfg;
      std::vector<int> raid;
      int rnti;
      bool is_sched = false;
    };
    struct dl_sched_sch2_t
    {
      slot_sched_cfg_t sch2Cfg;
      int rnti;
      int len;
      uint8_t msg[400]; 
      bool is_sched = false;
    };
    struct dl_sched_sch1_t
    {
      slot_sched_cfg_t sch1Cfg;
      int rnti;
      int len;
      int lcid;
      uint8_t msg[400]; 
      bool is_sched = false;
    };
    struct wx_dl_sched_res
    {
      dl_sched_data_t data[MaxSlotID];
      dl_sched_sib_t sib[MaxSlotID];
      dl_sched_mib_t mib[MaxSlotID];
      dl_sched_mib_t pcch[MaxSlotID];
      dl_sched_IoTsi_t IoTsi[MaxSlotID];
      dl_sched_rar_t rar[MaxSlotID];
      dl_sched_iot_rar_t iot_rar[MaxSlotID];
      dl_sched_sch2_t sch2[MaxSlotID];
      dl_sched_sch1_t sch1[MaxSlotID];
      bool fcch[5];
      // 11.06
      bool sch[5];
      bool sych[5];
    }; // 下行调度结果
    // void getDlAlloc(dl_allocate dlallocSource);
    void init(mac_interface_sched *mac_, uint32_t area_mode_, uint32_t network_mode_, uint32_t multi_beam_num_);
    void sched_set_parameter(uint32_t tti, slot_sched_cfg_t &txCfg, phyChanType_t chanT);
    void set_tti(int tti, int fq);
    bool dl_sched(int i, int freq_index, int tti, wx_dl_sched_res &sched_result);
    bool dl_IoT_sched(int freq_index, int tti, wx_dl_sched_res &sched_result);
    void initResourceMap();
    void fill_tbcch_resourceMap();
    void fill_sib_resourceMap();
    void reset_sib_resourceMap();
    void fill_bbch_resourceMap();
    void reset_bbch_resourceMap();
    void reset_mib_resourceMap();
    void alloc_source();

              //-----------------------------2024.03.04-----------
          void allocate_source_multi(phy_channel_t &alloc);
          //---------------------------------------------


    void allocate_source(dl_allocate &alloc);
    void deallocate_source(dl_allocate &source);
    void deallocate_multiSource(phy_channel_t &source);
    void UE_14_Deallocate_Source(int Uecap14_bandId,int Uecap14_freqId,int Uecap14_slot);
    // 11.06
    void fill_sych_resourceMap();
    void keep_in_touch();
    void reset_source(int bd, int fq, int fr, int sf);

    void wx_sched_uecategory(uint8_t uecategory_, uint16_t ue_cap14_band_id_, uint16_t ue_cap14_freq_id_, uint16_t ue_cap14_slot_);
    void wx_sched_FrameOff_value(int mac_frame_off_);
    void reset_uecap_after_release(uint8_t uecategory_);
    void Reset_Allresource_when_frameOff_NotZero();

    logicChanType_t resourceMap[MaxBandID][MaxCarrierID][MAX_SF][MaxSlotID];
    uint8_t ptdchFrameLoc[4] = {1, 14, 27, 40};
    uint8_t pmbchFrameLoc[3] = {13, 26, 39};
    uint8_t sibFrameLoc[8] = {2, 8, 15, 21, 28, 34, 41, 47};
    uint8_t pfcchLoc[8][2] = {{0, 1}, {6, 2}, {13, 3}, {19, 4}, {26, 1}, {32, 2}, {39, 3}, {45, 4}};
    std::vector<std::vector<uint8_t>> fnConfig = {{}, {0}, {1}, {0, 1}, {2}, {0, 2}, {1, 2}, {0, 1, 2}, {3}, {0, 3}, {1, 3}, {0, 1, 3}, {2, 3}, {0, 2, 3}, {1, 2, 3}, {0, 1, 2, 3}};
    // 需要从从cfg_mac中读取的数据

    int sibLen;
    int sibIndex;
    int pcchLen;
    int pcchIndex;
    rrc_interface_mac *rrc = nullptr;
    mac_interface_sched *mac_h = nullptr;
    std::vector<sched_cell_params_t> sched_cell_params;
    loc_info bbch;
    loc_info agch;
    loc_info sib;
    loc_info IoTsi;
    loc_info rach;
    loc_info pcch;
    // rnti_map_t<std::unique_ptr<sched_ue>> ue_db;
    sched_result_ringbuffer sched_results;
    std::mutex ched_mutex;
    uint32_t tx_sfn; // 系统超高帧号
    uint32_t tx_sf;  // 子帧号
    int tx_fq;
    int tx_bd;
    bool serio = false;
    int tti_;
    uint32_t network_mode;
    uint32_t area_mode;
    uint32_t multi_beam_num;
    bool sfmap[52];
    std::chrono::time_point<std::chrono::system_clock> start, end;
    // float time;
    std::chrono::duration<float> duration;
    float Tsync = 3000.0;
    dl_allocate sych_source; // 需初始化
                             //  dl_allocate dlAlloc_;
    bool start_flag = 0;
    int start_time = 0;
    int interval = 0;
    int totalTime = 0;
  };
} // namespace srsenb
#endif