#include "srsenb/hdr/stack/mac/wx_sched.h"
#include "srsenb/hdr/stack/mac/mac.h"
#include "srsran/interfaces/enb_mac_interfaces.h"
#include "srsran/srslog/srslog.h"

namespace srsenb
{

  // extern dl_allocate dlAlloc;
  extern psych_pdu psych;
  void wx_sched::init(mac_interface_sched *mac_, uint32_t area_mode_, uint32_t network_mode_, uint32_t multi_beam_num_)
  {
    mac_h = mac_;
    area_mode = area_mode_;
    network_mode = network_mode_;
    multi_beam_num = multi_beam_num_;
    initResourceMap();
    fill_sib_resourceMap();
    fill_bbch_resourceMap();
    fill_tbcch_resourceMap();
    prev_Handover_frame_off = Reselect_frame_off;
  }
  void wx_sched::initResourceMap()
  {
    mac_h->readRachCfg(rach);
    mac_h->readAgchCfg(agch);
    mac_h->readBbchCfg(bbch);
    mac_h->readSibCfg(sib);
    mac_h->readPcchCfg(pcch);
    mac_h->readTbcchCfg(IoTsi);
    mac_h->readFrameOffCfg(Reselect_frame_off);
    std::cout << "Reselect_frame_off=" << Reselect_frame_off << std::endl;
    std::cout
        << sib.band_ID << "-" << sib.freq << "-" << sib.slot << std::endl;
    std::cout << agch.band_ID << "-" << agch.freq << "-" << agch.fram << "-" << agch.slot << std::endl;
    std::cout << bbch.band_ID << "-" << bbch.freq << "-" << bbch.fram << "-" << bbch.slot << std::endl;
    std::cout << IoTsi.band_ID << "-" << IoTsi.freq << "-" << IoTsi.fram << "-" << IoTsi.slot << std::endl;
    std::cout << rach.band_ID << "-" << rach.freq << "-" << rach.fram << "-" << rach.slot << std::endl;
    for (int a = 0; a < 4 * 56; a++)
    {
      for (int i = 0; i < MaxSlotID; i++)
      {
        for (int j = 0; j < MAX_SF; j++)
        {
          resourceMap[a / 4][a % 4][j][i] = logicChanType_t::NULLTYPE;
        }
      }
    }
    // resourceMap[1][1][0][0] = logicChanType_t::MBCH;
    // for (uint8_t &n : pmbchFrameLoc)
    //{
    resourceMap[1][1][0 + Reselect_frame_off][0] = logicChanType_t::MBCH;
    // resourceMap[pcch.band_ID][pcch.freq][n][0] = logicChanType_t::MBCH;
    //}
    std::cout << "---0612---pcch_band_id:" << pcch.band_ID << std::endl;
    std::cout << "---0612---pcch_freq:" << pcch.freq << std::endl;
    std::cout << "---1128---[0 + Reselect_frame_off]:" << Reselect_frame_off << std::endl;
    for (uint8_t &n : pmbchFrameLoc)
    {
      // resourceMap[1][1][n][0] = logicChanType_t::MBCH;
      resourceMap[pcch.band_ID][pcch.freq][(n + Reselect_frame_off)%MAX_SF][0] = logicChanType_t::PCCH;
    }
  }

  void wx_sched::fill_tbcch_resourceMap()
  {
    // std::cout<<"---fill_tbbch_resourceMap----"<<std::endl;
    resourceMap[IoTsi.band_ID][IoTsi.freq][IoTsi.fram][1] = logicChanType_t::TBCCH;
    // resourceMap[IoTsi.band_ID][IoTsi.freq][IoTsi.fram][2] = logicChanType_t::TBCCH;
    // resourceMap[IoTsi.band_ID][IoTsi.freq][IoTsi.fram][3] = logicChanType_t::TBCCH;
    // resourceMap[IoTsi.band_ID][IoTsi.freq][IoTsi.fram][4] = logicChanType_t::TBCCH;
  }

  void wx_sched::fill_sib_resourceMap()
  {
    for (uint8_t &n : sibFrameLoc)
    {
      std::cout<<"[SIB]n="<<n<<std::endl;
      std::cout<<"[SIB]n+Reselect_frame_off = "<<n+Reselect_frame_off<<std::endl;
      resourceMap[sib.band_ID][sib.freq][(n + Reselect_frame_off)%MAX_SF][sib.slot] = logicChanType_t ::SIB;
      // sfmap[n]=true;
    }
  }
  void wx_sched::fill_bbch_resourceMap()
  {
    std::cout << "sib.band_ID=" << sib.band_ID << std::endl;
    std::cout << "sib.freq=" << sib.freq << std::endl;
    // // 濉玃FCCH璧勬簮
    for (int i = 0; i < 8; i++)
    {
      resourceMap[sib.band_ID][sib.freq][(pfcchLoc[i][0] + Reselect_frame_off)%MAX_SF][pfcchLoc[i][1]] = logicChanType_t ::FCCH;
      // sfmap[n]=true;
      //  std::cout<<"pfcchLoc[i][0]"<<static_cast<int>(pfcchLoc[i][0])<<std::endl;
      //  std::cout<<"pfcchLoc[i][1]"<<static_cast<int>(pfcchLoc[i][1])<<std::endl;
    }
    // 濉獴BCH璧勬簮
    for (int i = 1; i <= MAX_SF; i++)
    {
      int len = fnConfig[bbch.fram].size();
      for (int j = 0; j < len; j++)
      {
        if ((i / 2) % 4 == fnConfig[bbch.fram][j])
        {
          if (resourceMap[bbch.band_ID][bbch.freq][(i - 1 + Reselect_frame_off)%MAX_SF][bbch.slot] == srsenb::logicChanType_t ::NULLTYPE)
          {
            resourceMap[bbch.band_ID][bbch.freq][(i - 1 + Reselect_frame_off)%MAX_SF][bbch.slot] = srsenb::logicChanType_t ::BBCH;
            // sfmap[n]=true;
          }
        }
      }
    }
    // 濉獳GCH璧勬簮
    for (int i = 1; i <= MAX_SF; i++)
    {
      int len = fnConfig[agch.fram].size();
      for (int j = 0; j < len; j++)
      {
        if ((i / 2) % 4 == fnConfig[agch.fram][j])
        {
          if (resourceMap[agch.band_ID][agch.freq][(i - 1 + Reselect_frame_off)%MAX_SF][agch.slot] == srsenb::logicChanType_t ::NULLTYPE && resourceMap[agch.band_ID][agch.freq][(i - 1 + Reselect_frame_off)%MAX_SF][sib.slot] == NULLTYPE)
          {
            resourceMap[agch.band_ID][agch.freq][(i - 1 + Reselect_frame_off)%MAX_SF][agch.slot] = srsenb::logicChanType_t ::AGCH;
            std::cout << "agch.band_ID = " <<agch.band_ID << std::endl;
            std::cout << "agch.freq = " <<agch.freq << std::endl;
            std::cout << "Config sf = " <<i - 1 + Reselect_frame_off << std::endl;
            std::cout << "agch.slot = " <<agch.slot << std::endl;
            // std::cout<<"agch.slot"<<agch.slot<<std::endl;
            // sfmap[n]=true;
          }
        }
      }
    }
  }

  void wx_sched::wx_sched_uecategory(uint8_t uecategory_, uint16_t ue_cap14_band_id_, uint16_t ue_cap14_freq_id_, uint16_t ue_cap14_slot_)
  {
    ue_category = uecategory_;
    ue_cap14_band_id = ue_cap14_band_id_;
    ue_cap14_freq_id = ue_cap14_freq_id_;
    ue_cap14_slot = ue_cap14_slot_;
    std::cout << " wx sched ue_category = " << (int)ue_category << std::endl;
  }

  void wx_sched::wx_sched_FrameOff_value(int mac_frame_off_)
  {
    current_Handover_frame_off = mac_frame_off_;
    std::cout
        << " [SCHED][FRAME OFF] = " << current_Handover_frame_off << std::endl;
  }

  void wx_sched::reset_uecap_after_release(uint8_t uecategory_)
  {
    ue_category = uecategory_;
    std::cout << "@ @ ue_category=" << (int)ue_category << std::endl;
  }

//--------------2024.03.04-----------------------------

  void wx_sched::allocate_source_multi(phy_channel_t &alloc)
  {
    std::cout << "alloc.slot=" << alloc.slot << std::endl;
    int n = 0;
    for (int i = (tti_); i < tti_ + ALLOC_END; i += ALLOC_INTERVAL)
    {
      for (int j = 0; j < 5; j++)
      {
        int temp_slot = 0x01 & (alloc.slot >> j);
        std::cout << "temp_slot = " << temp_slot << std::endl;
        if (temp_slot > 0)
        {
          if (resourceMap[alloc.bandID][alloc.freqID][i % 52][j] == logicChanType_t::NULLTYPE)
          {
            //std::cout << "Multi::tx_sf:" << i % 52 << " bandID:" << alloc.bandID << " freqID:" << alloc.freqID << " slot" << j << std::endl;
            std::cout<<"update resourceMap,band:"<<alloc.bandID<<"freqId:"<<alloc.freqID<<std::endl;
            std::cout<<"LogicType:"<<alloc.LogicType<<std::endl;
            resourceMap[alloc.bandID][alloc.freqID][i % 52][j] = alloc.LogicType;
          }
        }
        // if (resourceMap[alloc.bandID][alloc.freqID][i % 52][temp_slot] == logicChanType_t::NULLTYPE&&temp_slot==1)
      }
      std::cout << "------------------------------------------------------------------------------------------------" << std::endl;
      // if(resourceMap[alloc.bandID][alloc.freqID][i%52][alloc.slot]==logicChanType_t::NULLTYPE){
      //          std::cout<<"---sch2::tx_sf-- bandid freqid:"<<i%52<<alloc.bandID<<" "<<alloc.freqID<<" "<<std::endl;
      //   resourceMap[alloc.bandID][alloc.freqID][i%52][alloc.slot]=alloc.LogicType;
      //   // n++;
      //  // break;

      // }
    }
  }
  //----------------------------------------------------




  void wx_sched::allocate_source(dl_allocate &alloc)
  {
    int n = 0;
    // for(int i=tti_+ALLOC_START;i<tti_+ALLOC_END;i+=ALLOC_INTERVAL){
    std::cout << "allocate_source" << std::endl;
    std::cout<<"band:"<<alloc.bandID<<"freqId:"<<alloc.freq<<std::endl;
    std::cout<<"LogicType:"<<alloc.LogicType<<std::endl;
    for (int i = tti_; i < tti_ + ALLOC_END; i += ALLOC_INTERVAL)
    {
      // zhj
      //std::cout << " zhj test 3 " << std::endl;

      if (ue_category == 14)
      {
        std::cout << " ue_cap14_band_id = " << (int)ue_cap14_band_id << std::endl;
        std::cout << " ue_cap14_freq_id = " << (int)ue_cap14_freq_id << std::endl;
        std::cout << " ue_cap14_slot = " << (int)ue_cap14_slot << std::endl;
        std::cout << " wx sched ue_category == 14 && is_reconfig == false " << std::endl;
        if (resourceMap[ue_cap14_band_id][ue_cap14_freq_id][i % 52][ue_cap14_slot] == logicChanType_t::NULLTYPE)
        {
          std::cout << " zhj test 4 " << std::endl;
          std::cout << "tx_sf" << i << " " << tti_ << std::endl;
          std::cout << " i % 52 = " << i % 52 << std::endl;
          resourceMap[ue_cap14_band_id][ue_cap14_freq_id][i % 52][ue_cap14_slot] = SCH2;
        }
      }
      else
      {
        //std::cout << " NO wx sched ue_category == 14 && is_reconfig == false " << std::endl;
        for (int j = 0; j < 5; j++)
      {
        int temp_slot = 0x01 & (alloc.solt >> j);
        //std::cout << "temp_slot = " << temp_slot << std::endl;
        if (temp_slot > 0)
        {
        if (resourceMap[alloc.bandID][alloc.freq][i % 52][j] == logicChanType_t::NULLTYPE)
        {
          // std::cout<<"---sch2::tx_sf--"<<i<<" "<<tti_<<std::endl;
          resourceMap[alloc.bandID][alloc.freq][i % 52][j] = alloc.LogicType;
          n++;
          // break;
        }
        }
      }
      }
    }
  }
void wx_sched::deallocate_multiSource(phy_channel_t &source){
 for (int i = 0; i < MAX_SF; ++i)
    {
      std::cout<<"source.LogicType:"<<source.LogicType<<std::endl;
      for(int j=0;j<5;j++)
      {
         //std::cout<<"resourceMap----"<<(int)resourceMap[source.bandID][source.freqID][i][j]<<std::endl; 
        resourceMap[source.bandID][source.freqID][i][j] = logicChanType_t::NULLTYPE;
      }
    }
}

  void wx_sched::deallocate_source(dl_allocate &source)
  {
    for (int i = 0; i < MAX_SF; ++i)
    {
      std::cout<<"resourceMap["<<source.bandID<<"]["<<source.freq<<"]["<<i<<"]["<<source.solt <<"] :"<<(int)resourceMap[source.bandID][source.freq][i][source.solt]<<std::endl; 
      std::cout<<"source.LogicType:"<<source.LogicType<<std::endl;
        // sfmap[i]=false;
        // std::cout << "[deallocate_source][BAND_ID]:" << source.bandID << std::endl;
        // std::cout << "[deallocate_source][FREQ_ID]:" << source.freq << std::endl;
        // std::cout << "[deallocate_source][SLOT]:" << source.solt << std::endl;
        for(int j=0;j<5;j++){
          resourceMap[source.bandID][source.freq][i][j] = logicChanType_t::NULLTYPE;
        }
        
    }
  }

 void wx_sched::UE_14_Deallocate_Source(int Uecap14_bandId,int Uecap14_freqId,int Uecap14_slot){
    for (int i = 0; i < MAX_SF; ++i)
    {
      std::cout<<"resourceMap["<<Uecap14_bandId<<"]["<<Uecap14_freqId<<"]["<<i<<"]["<<Uecap14_slot <<"] :"<<(int)resourceMap[Uecap14_bandId][Uecap14_freqId][i][Uecap14_slot]<<std::endl; 
      if (resourceMap[Uecap14_bandId][Uecap14_freqId][i][Uecap14_slot] ==SCH2)
      {
        // sfmap[i]=false;
        std::cout << "[UE_14 deallocate_source][BAND_ID]:" << Uecap14_bandId << std::endl;
        std::cout << "[UE_14 deallocate_source][FREQ_ID]:" << Uecap14_freqId << std::endl;
        std::cout << "[UE_14 deallocate_source][SLOT]:" << Uecap14_slot << std::endl;

        resourceMap[Uecap14_bandId][Uecap14_freqId][i][Uecap14_slot] = logicChanType_t::NULLTYPE;
      }
    }
 }

  void wx_sched::alloc_source()
  {
    for (int i = (tti_ + 3); i < tti_ + ALLOC_END; i++)
    {
      std::cout << "[alloc_source][UE_CAP][14][BAND_ID]:" << (int)ue_cap14_band_id << std::endl;
      std::cout << "[alloc_source][UE_CAP][14][FREQ_ID]:" << (int)ue_cap14_freq_id << std::endl;
      std::cout << "[alloc_source][UE_CAP][14][SLOT]:" << (int)ue_cap14_slot << std::endl;
      if (ue_category == 14)
      {
        std::cout << "[alloc_source][UE_CAP][14]:TRUE" << std::endl;
        if (resourceMap[ue_cap14_band_id][ue_cap14_freq_id][i % 52][ue_cap14_slot] == logicChanType_t::NULLTYPE)
        {
          std::cout << " zhj test 2 " << std::endl;
          std::cout << "tx_sf" << i << " " << tti_ << std::endl;
          std::cout << " i % 52 = " << i % 52 << std::endl;
          resourceMap[ue_cap14_band_id][ue_cap14_freq_id][i % 52][ue_cap14_slot] = DCH;
          break;
        }
      }
      else
      {
        //std::cout << "[alloc_source][UE_CAP][14]:FALSE" << std::endl;
        if (resourceMap[9][1][i % 52][3] == logicChanType_t::NULLTYPE)
        {
          std::cout << " zhj test 2 " << std::endl;
          std::cout << "tx_sf" << i << " " << tti_ << std::endl;
          resourceMap[9][1][i % 52][3] = DCH;
          break;
        }
      }
    }
  }

  void wx_sched::fill_sych_resourceMap()
  {
    if (network_mode == 0)
    {
      if (resourceMap[8][1][(tx_sf + 2)%52][2] == logicChanType_t::NULLTYPE)
      {
        resourceMap[8][1][(tx_sf + 2)%52][2] = logicChanType_t::SYCH;
      }
    }
  }

  // 11.06
  void wx_sched::keep_in_touch()
  {
    for (int j = tx_sf + 1; j < MAX_SF; j++)
    {
      if (network_mode == 0)
      {
        if (resourceMap[9][1][j][3] == logicChanType_t::NULLTYPE)
        {
          resourceMap[9][1][j][3] = logicChanType_t::SCH2;
        }
      }
    }
  }
  void wx_sched::reset_source(int bd, int fq, int sf, int slot)
  {
    resourceMap[bd][fq][sf][slot] = logicChanType_t::NULLTYPE;
  }

  void wx_sched::sched_set_parameter(uint32_t tti, slot_sched_cfg_t &txCfg, phyChanType_t chanT)
  {
    txCfg.burstID = chanT;
    txCfg.modeType = 0;
    txCfg.equiFlag = 0;
    txCfg.udFlag = 1;
    txCfg.payloadType = 0;
    // std::cout<<"txCfg.burstID"<<txCfg.burstID<<std::endl;
  }

  void wx_sched::set_tti(int tti, int fq)
  {
    tx_sfn = tti / 52;
    tx_sf = tti % 52;
    // std::cout<<"[SET TTI]tx_sf="<<tx_sf<<std::endl;
    tx_bd = fq / 4;
    tx_fq = fq % 4;
    tti_ = tti;
  }

  // loT调度
  bool wx_sched::dl_IoT_sched(int freq_index, int tti, wx_dl_sched_res &sched_result)
  {
    set_tti(tti, freq_index);
    for (int i = 0; i < MaxSlotID; i++)
    {
      // IoTsi消息
      if (resourceMap[tx_bd][tx_fq][tx_sf][i] == logicChanType_t ::TBCCH)
      {
        sched_set_parameter(tti, sched_result.IoTsi[i].IoTsiCfg, serio ? PTSCH : PTDCH);
        sched_result.IoTsi[i].is_sched = true;
        mac_h->readIoTsi(sched_result.IoTsi[i].IoTsiLen);
      }
      // 调度MIB消息
      else if (resourceMap[tx_bd][tx_fq][tx_sf][i] == logicChanType_t ::MBCH)
      {
        sched_set_parameter(tti, sched_result.mib[i].mibCfg, PMBCH);
        sched_result.mib[i].is_sched = true;
        if (tx_sf == 0)
        {
          mac_h->readMib(sched_result.mib[i].mibLen, tti / 52);
          //     mac_h->readPcch(sched_result.mib[i].pcchLen,tti / 52);

          //     pcchIndex=1;
          //     if(pcchLen<(11-sched_result.mib[i].mibLen)){
          //     sched_result.mib[i].index=pcchIndex;
          //     sched_result.mib[i].si=0;
          //     sched_result.mib[i].pcchLen=pcchLen;
          //     pcchLen=0;
          //     }else{
          //     sched_result.mib[i].index=pcchIndex;
          //     sched_result.mib[i].si=1;
          //     sched_result.mib[i].pcchLen=12-sched_result.mib[i].mibLen;
          //     pcchLen-=sched_result.mib[i].pcchLen;
          // }
          // pcchIndex++;
          //   }else{
          //      if(tx_sf==13){
          //      sched_result.mib[i].index=pcchIndex;
          //      sched_result.mib[i].pcchLen=pcchLen>12?12:pcchLen;
          //      pcchLen-=sched_result.mib[i].pcchLen;
          //   }
          //   else if(tx_sf==26){
          //      sched_result.mib[i].index=pcchIndex;
          //      sched_result.mib[i].pcchLen=pcchLen>12?12:pcchLen;
          //      pcchLen-=sched_result.mib[i].pcchLen;
          //   }else if(tx_sf==39){
          //      sched_result.mib[i].index=pcchIndex;
          //      sched_result.mib[i].pcchLen=pcchLen>12?12:pcchLen;
          //      pcchLen-=sched_result.mib[i].pcchLen;
          //   }
          //   if(pcchLen<=0){
          //   sched_result.mib[i].si=3;
          // }else{
          //    sched_result.mib[i].si=2;
          // }
        }
      }
      else if (resourceMap[tx_bd][tx_fq][tx_sf][i] == logicChanType_t ::FCCH) // 调度fcch
      {
        sched_result.fcch[i] = true;
      }
      else if (resourceMap[tx_bd][tx_fq][tx_sf][i] == logicChanType_t ::SIB) // 调度SIB
      {
        sched_result.sib[i].is_sched = true;
        if (tx_sf == 2)
        {
          sched_set_parameter(tti, sched_result.sib[i].sibCfg, PSBCH);
          mac_h->readSib(sibLen);
          sibIndex = 1;
          if (sibLen < 20)
          {
            sched_result.sib[i].index = sibIndex;
            sched_result.sib[i].si = 0;
            sched_result.sib[i].sibLen = sibLen;
            sibLen = 0;
          }
          else
          {
            sched_result.sib[i].index = sibIndex;
            sched_result.sib[i].si = 0b01;
            sched_result.sib[i].sibLen = 18;
            sibLen -= 18;
          }
        }
        else
        {
          sched_set_parameter(tti, sched_result.sib[i].sibCfg, PSBCH);
          sibIndex += 1;
          if (sibLen > 0)
          {
            if (sibLen < 20)
            {
              sched_result.sib[i].index = sibIndex;
              sched_result.sib[i].si = 0b11;
              sched_result.sib[i].sibLen = sibLen;
              sibLen = 0;
            }
            else
            {
              sched_result.sib[i].index = sibIndex;
              sched_result.sib[i].si = 0b10;
              sched_result.sib[i].sibLen = 18;
              sibLen -= 18;
            }
          }
          else
          {
            sched_result.sib[i].index = sibIndex;
            sched_result.sib[i].si = 0b11;
            sched_result.sib[i].sibLen = 0;
            sibLen = 0;
          }
        }
      }
      else if (resourceMap[tx_bd][tx_fq][tx_sf][i] == logicChanType_t ::AGCH)
      {
        std::vector<int> raid1;
        if (area_mode == 0)
        {
          sched_set_parameter(tti, sched_result.rar[i].rarCfg, PTDCH);
        }
        else if (area_mode == 1)
        {
          sched_set_parameter(tti, sched_result.rar[i].rarCfg, DSPDTCH);
        }
        mac_h->get_rar(tti, raid1);
        if ((int)raid1.size() > 0)
        {
          sched_result.iot_rar[i].is_sched = true;
          sched_result.iot_rar[i].raid = raid1;
          alloc_source();
        }
      }
      else if (resourceMap[tx_bd][tx_fq][tx_sf][i] == logicChanType_t ::DCCH)
      {
        std::vector<int> raid2;
        mac_h->get_consinfo(tti, raid2);
        if ((int)raid2.size() > 0)
        {
          sched_set_parameter(tti, sched_result.data[i].dataCfg, PTDCH);
          sched_result.data[i].is_sched = true;
          sched_result.data[i].raid = raid2;
        }
      }
    }
    return true;
  }

  bool wx_sched::dl_sched(int i, int freq_index, int tti, wx_dl_sched_res &sched_result)
  {
    set_tti(tti, freq_index);
      // 调度MIB消息psychsf][i] == logicChanType_t ::MBCH) {
      if (resourceMap[tx_bd][tx_fq][tx_sf][i] == logicChanType_t::MBCH)
      {
        std::cout<<"i="<<i<<std::endl;
        std::cout<<"tx_sf="<<tx_sf<<std::endl;
        if (area_mode == 0)
        {
          sched_set_parameter(tti, sched_result.mib[i].mibCfg, PMBCH);
          // sched_set_parameter(tti, sched_result.mib[i].pcchCfg, PMBCH);
          //  std::cout<<"*******************MBCHbd:"<<tx_bd<<std::endl;
          //  std::cout<<"*******************MBCHfq:"<<tx_fq<<std::endl;
          //  std::cout<<"*******************MBCHsf:"<<tx_sf<<std::endl;
          //  std::cout<<"*******************MBCHslot:"<<i<<std::endl;
          //  sched_result.mib[i].is_sched = true;
        }
        // kuopin
        if (area_mode == 1)
        {
          sched_set_parameter(tti, sched_result.mib[i].mibCfg, DS_PPCH);
          // sched_set_parameter(tti, sched_result.mib[i].pcchCfg, DS_PPCH);
          //  sched_result.mib[i].is_sched = true;
          //  if (tx_sf == 0)
          //  {
          //      mac_h->readMib(sched_result.mib[i].mibLen, tti / 52);
          //  }
        }
        sched_result.mib[i].is_sched = true;
        if (tx_sf == 0+(uint32_t)Reselect_frame_off)//Frame Offset
        {
          mac_h->readMib(sched_result.mib[i].mibLen, tti / 52);
          // mac_h->readPcch(pcchLen,tti / 52);
          pcchIndex = 1;
          // if(pcchLen<(11-sched_result.mib[i].mibLen)){
          sched_result.mib[i].index = pcchIndex;
          //     sched_result.mib[i].si=0;
          //     sched_result.mib[i].pcchLen=pcchLen;
          //     pcchLen=0;
          //     }else{
          //         sched_result.mib[i].index=pcchIndex;
          //         sched_result.mib[i].si=1;
          //         sched_result.mib[i].pcchLen=11-sched_result.mib[i].mibLen;
          //         pcchLen-=sched_result.mib[i].pcchLen;
          //     }
          pcchIndex++;
        }
        else
        {
          if (tx_sf == 13+(uint32_t)Reselect_frame_off)//Frame Offset
          {
            // mac_h->readPcch(pcchLen,tti / 52);

            sched_result.mib[i].index = pcchIndex;
            sched_result.mib[i].pcchLen = 0;
            // pcchLen-=sched_result.mib[i].pcchLen;
            sched_result.mib[i].is_last = pcchLen > 0 ? false : true;
            // sched_result.mib[i].pcchSubhdr_num=pcchSubhdr_num>2?2:pcchSubhdr_num;
            // pcchSubhdr_num-=sched_result.mib[i].pcchSubhdr_num;
          }
          else if (tx_sf == 26+(uint32_t)Reselect_frame_off)//Frame Offset
          {
            sched_result.mib[i].index = pcchIndex;
            sched_result.mib[i].pcchLen = 0;
            // pcchLen-=sched_result.mib[i].pcchLen;
            sched_result.mib[i].is_last = pcchLen > 0 ? false : true;
          }
          else if (tx_sf == 39+(uint32_t)Reselect_frame_off)//Frame Offset
          {
            sched_result.mib[i].index = pcchIndex;
            sched_result.mib[i].pcchLen = 0;
            // pcchLen-=sched_result.mib[i].pcchLen;
            sched_result.mib[i].is_last = pcchLen > 0 ? false : true;
          }
        }
        // std::cout<<"*******************MBCHbd:"<<tx_bd<<std::endl;
        // std::cout<<"*******************MBCHfq:"<<tx_fq<<std::endl;
        // std::cout<<"*******************MBCHsf:"<<tx_sf<<std::endl;
        // std::cout<<"*******************MBCHslot:"<<i<<std::endl;
      }
      else if (resourceMap[tx_bd][tx_fq][tx_sf][i] == logicChanType_t::PCCH) // 调度pcch
      {
        if (area_mode == 0)
        {
          // sched_set_parameter(tti, sched_result.mib[i].mibCfg, PMBCH);
          sched_set_parameter(tti, sched_result.mib[i].pcchCfg, PMBCH);
          // std::cout<<"*******************MBCHbd:"<<tx_bd<<std::endl;
          // std::cout<<"*******************MBCHfq:"<<tx_fq<<std::endl;
          // std::cout<<"*******************MBCHsf:"<<tx_sf<<std::endl;
          // std::cout<<"*******************MBCHslot:"<<i<<std::endl;
          // sched_result.mib[i].is_sched = true;
        }
        // kuopin
        if (area_mode == 1)
        {
          // sched_set_parameter(tti, sched_result.mib[i].mibCfg, DS_PPCH);
          sched_set_parameter(tti, sched_result.mib[i].pcchCfg, DS_PPCH);
          // sched_result.mib[i].is_sched = true;
          // if (tx_sf == 0)
          // {
          //     mac_h->readMib(sched_result.mib[i].mibLen, tti / 52);
          // }
        }
        sched_result.pcch[i].is_sched = true;
        if (tx_sf == 0+(uint32_t)Reselect_frame_off)//Frame Offset
        {
          // mac_h->readMib(sched_result.mib[i].mibLen, tti / 52);
          // mac_h->readPcch(pcchLen,tti / 52);
          pcchIndex = 1;
          // if(pcchLen<(11-sched_result.mib[i].mibLen)){
          sched_result.mib[i].index = pcchIndex;
          //     sched_result.mib[i].si=0;
          //     sched_result.mib[i].pcchLen=pcchLen;
          //     pcchLen=0;
          //     }else{
          //         sched_result.mib[i].index=pcchIndex;
          //         sched_result.mib[i].si=1;
          //         sched_result.mib[i].pcchLen=11-sched_result.mib[i].mibLen;
          //         pcchLen-=sched_result.mib[i].pcchLen;
          //     }
          pcchIndex++;
        }
        else
        {
          if (tx_sf == 13+(uint32_t)Reselect_frame_off)//Frame Offset
          {
            if(mac_h->readPcch(pcchLen, tti / 52)){
              sched_result.pcch[i].is_sched = true;
              std::cout<<"1206-sched_result.pcch[i].is_sched1:"<<pcchLen<<std::endl;
            };
            pcchIndex = 2;
            sched_result.mib[i].index = pcchIndex;
            sched_result.mib[i].pcchLen = pcchLen > 12 ? 12 : pcchLen;
            pcchLen -= sched_result.mib[i].pcchLen;
            sched_result.mib[i].is_last = pcchLen > 0 ? false : true;
            // sched_result.mib[i].pcchSubhdr_num=pcchSubhdr_num>2?2:pcchSubhdr_num;
            // pcchSubhdr_num-=sched_result.mib[i].pcchSubhdr_num;
          }
          else if (tx_sf == 26+(uint32_t)Reselect_frame_off)//Frame Offset
          {
            /*check if need mux paging first*/

            sched_result.pcch[i].is_sched = pcchLen > 0 ? true : false;
            std::cout<<"1206-sched_result.pcch[i].is_sched2:"<<pcchLen<<std::endl;
            sched_result.mib[i].index = pcchIndex;
            sched_result.mib[i].pcchLen = pcchLen > 12 ? 12 : pcchLen;
            pcchLen -= sched_result.mib[i].pcchLen;
            sched_result.mib[i].is_last = pcchLen > 0 ? false : true;
          }
          else if (tx_sf == 39+(uint32_t)Reselect_frame_off)//Frame Offset
          {
            /*check if need mux paging first*/
            sched_result.pcch[i].is_sched = pcchLen > 0 ? true : false;
            std::cout<<"1206-sched_result.pcch[i].is_sched3:"<<pcchLen<<std::endl;
            sched_result.mib[i].index = pcchIndex;
            sched_result.mib[i].pcchLen = pcchLen > 12 ? 12 : pcchLen;
            pcchLen -= sched_result.mib[i].pcchLen;
            sched_result.mib[i].is_last = pcchLen > 0 ? false : true;
          }
          else{
            /*[20241129]re-check if need mux paging*/
            sched_result.pcch[i].is_sched = false;
            std::cout<<"1206-other paging not sched; paginglen:"<<pcchLen<<std::endl;
            /*end*/                   
          }
        } 
      }
      else if (resourceMap[tx_bd][tx_fq][tx_sf][i] == logicChanType_t::FCCH) // 调度fcch
      {
        // std::cout<<"*******************FCCHbd:"<<tx_bd<<std::endl;
        // std::cout<<"*******************FCCHfq:"<<tx_fq<<std::endl;
        // std::cout<<"*******************FCCHsf:"<<tx_sf<<std::endl;
        // std::cout<<"*******************FCCHslot:"<<i<<std::endl;
        if (area_mode == 0){
          sched_result.fcch[i] = true;
        }else{
          sched_result.fcch[i] = false;
        }
      }
      else if (resourceMap[tx_bd][tx_fq][tx_sf][i] == logicChanType_t::SIB) // 调度SIB
      {
        // std::cout<<"*******************SIBbd:"<<tx_bd<<std::endl;
        // std::cout<<"*******************SIBfq:"<<tx_fq<<std::endl;
        // std::cout<<"*******************SIBsf:"<<tx_sf<<std::endl;
        // std::cout<<"*******************SIBslot:"<<i<<std::endl;
        if (area_mode == 0)
        {
          sched_result.sib[i].is_sched = true;
          if (tx_sf == 2+(uint32_t)Reselect_frame_off)//Frame Offset
          {
            sched_set_parameter(tti, sched_result.sib[i].sibCfg, PSBCH);
            // std::cout<<"sib read star"<<std::endl;
            mac_h->readSib(sibLen);
            std::cout << "sib read end"
                      << "sib_len:" << sibLen << std::endl;
            sibIndex = 1;
            if (sibLen < 20)
            {
              sched_result.sib[i].index = (sibIndex - 1) * 18;
              sched_result.sib[i].si = 0;
              sched_result.sib[i].sibLen = sibLen;
              sibLen = 0;
            }
            else
            {
              sched_result.sib[i].index = (sibIndex - 1) * 18;
              sched_result.sib[i].si = 0b01;
              sched_result.sib[i].sibLen = 18;
              sibLen -= 18;
            }
          }
          else
          {
            sched_set_parameter(tti, sched_result.sib[i].sibCfg, PSBCH);
            sibIndex += 1;
            if (sibLen > 0)
            {
              if (sibLen < 20)
              {
                sched_result.sib[i].index = (sibIndex - 1) * 18;
                sched_result.sib[i].si = 0b11;
                sched_result.sib[i].sibLen = sibLen;
                sibLen = 0;
              }
              else
              {
                sched_result.sib[i].index = (sibIndex - 1) * 18;
                sched_result.sib[i].si = 0b10;
                sched_result.sib[i].sibLen = 18;
                sibLen -= 18;
              }
            }
            else
            {
              sched_result.sib[i].index = (sibIndex - 1) * 18;
              sched_result.sib[i].si = 0b11;
              sched_result.sib[i].sibLen = 0;
              sibLen = 0;
            }
          }
        }

        // kuopin
        if (area_mode == 1)
        {
          // std::cout<<"sib area_mode"<<std::endl;
          sched_result.sib[i].is_sched = true;
          if (tx_sf == 2)
          {
            // std::cout<<"setconf"<<std::endl;
            sched_set_parameter(tti, sched_result.sib[i].sibCfg, DS_PBCH);
            // std::cout<<"sib read star"<<std::endl;
            mac_h->readSib(sibLen);
            // std::cout<<"sib read end"<<std::endl;
            sibIndex = 1;
            if (sibLen < 19)
            {
              sched_result.sib[i].index = (sibIndex - 1) * 17;
              sched_result.sib[i].si = 0;
              sched_result.sib[i].sibLen = sibLen;
              sibLen = 0;
            }
            else
            {
              sched_result.sib[i].index = (sibIndex - 1) * 17;
              sched_result.sib[i].si = 0b01;
              sched_result.sib[i].sibLen = 17;
              sibLen -= 17;
            }
          }
          else
          {
            // std::cout<<"sib "<<std::endl;
            sched_set_parameter(tti, sched_result.sib[i].sibCfg, DS_PBCH);
            sibIndex += 1;
            if (sibLen > 0)
            {
              if (sibLen < 19)
              {
                sched_result.sib[i].index = (sibIndex - 1) * 17;
                sched_result.sib[i].si = 0b11;
                sched_result.sib[i].sibLen = sibLen;
                sibLen = 0;
              }
              else
              {
                sched_result.sib[i].index = (sibIndex - 1) * 17;
                sched_result.sib[i].si = 0b10;
                sched_result.sib[i].sibLen = 17;
                sibLen -= 17;
              }
            }
            else
            {
              sched_result.sib[i].index = (sibIndex - 1) * 17;
              sched_result.sib[i].si = 0b11;
              sched_result.sib[i].sibLen = 0;
              sibLen = 0;
            }
          }
        }
      }
      else if (resourceMap[tx_bd][tx_fq][tx_sf][i] == logicChanType_t::AGCH)
      {
        std::vector<int> raid1;
        // std::cout<<"                dl_sched:"<<tti<<std::endl;
        // std::cout<<"**********wx_sched  agch*****************"<<std::endl;
        if (area_mode == 0)
        {
          sched_set_parameter(tti, sched_result.rar[i].rarCfg, PSBCH);
        }
        else
        {
          sched_set_parameter(tti, sched_result.rar[i].rarCfg, DS_PBCH);
          //     std::cout<<"*******************AGCHslot:"<<i<<std::endl;
        }
        if(mac_h->handover_rach()){
            sched_result.rar[i].is_sched = true;
            std::cout<<"ho_agch tti="<<tti<<std::endl;
        }
        mac_h->get_rar(tti, raid1);
        std::cout<<"[AGCH]tti="<<tti<<std::endl;
        // std::cout<<"[AGCH]tx_sf="<<tx_sf<<std::endl;
        // std::cout<<"*******************AGCHbd:"<<tx_bd<<std::endl;
        // std::cout<<"*******************AGCHfq:"<<tx_fq<<std::endl;
        // std::cout<<"*******************AGCHsf:"<<tx_sf<<std::endl;
        // std::cout<<"*******************AGCHslot:"<<i<<std::endl;
        if ((int)raid1.size() > 0)
        {
          sched_result.rar[i].is_sched = true;
          sched_result.rar[i].raid = raid1;
          // printf("sched_result.rar[i].raid.size() :%d",sched_result.rar[i].raid.size());
          // printf("psych.CI:%d\n",psych.CI);
          // std::cout << "psych.CI" << psych.CI << std::endl;
          // if (psych.CI < 2)
          // {
          //   fill_sych_resourceMap();
          // }
          // if(area_mode==0){
          if (mac_h->get_CI_state(70) < 2)
          // if(mac_h->get_CI_state(70)<2&&mac_h->get_CI_state(70)>-1)
          {
            // start = std::chrono::high_resolution_clock::now();
            sych_source.LogicType = SYCH;
            sych_source.Type = psych0;
            sych_source.pduLen = area_mode == 0 ? 14 : 19;
            sych_source.bandID = 8;
            sych_source.solt = 2;
            sych_source.freq = 1;
            std::cout << "allocate sych" << std::endl;
            // allocate_source(sych_source);
           if(!mac_h->getTC300Timeout()){
            alloc_source();
           }
          }
          // }else{
          //   alloc_source();
          // }
          //   std::cout << "fill_sych_resourceMap" << std::endl;
          // 11.06
          // keep_in_touch();
        }
      }
      else if (resourceMap[tx_bd][tx_fq][tx_sf][i] == logicChanType_t::DCH)
      {
        std::vector<int> raid2;
        //   std::cout<<"*******************SCHbd:"<<tx_bd<<std::endl;
        // std::cout<<"*******************SCHfq:"<<tx_fq<<std::endl;
        // std::cout<<"*******************SCHsf:"<<tx_sf<<std::endl;
        // std::cout<<"*******************SCHslot:"<<i<<std::endl;
        mac_h->get_consinfo(tti, raid2);
        if ((int)raid2.size() > 0)
        {
          if (area_mode == 0)
          {
            sched_set_parameter(tti, sched_result.data[i].dataCfg, PDCH);
          }
          else
          {
            sched_set_parameter(tti, sched_result.data[i].dataCfg, DS_PDTCH_1);
          }
          sched_result.data[i].is_sched = true;
          sched_result.data[i].raid = raid2;
        }
        // 11.06
      }
      else if (resourceMap[tx_bd][tx_fq][tx_sf][i] == logicChanType_t::SCH2) // 调度SCH2
      {
        std::cout << " 调度SCH2 " << i << std::endl;
        second_time = std::chrono::high_resolution_clock::now();
        mac_h->getSch2(sched_result.sch2[i].len, sched_result.sch2[i].rnti);
        sched_result.sch2[i].is_sched = true;
        if (area_mode == 0)
        {
          sched_set_parameter(tti, sched_result.sch2[i].sch2Cfg, PDCH);
        }
        else
        {
          sched_set_parameter(tti, sched_result.sch2[i].sch2Cfg, DS_PDTCH_1);
        }
        second_time_end = std::chrono::high_resolution_clock::now();
        auto duration_D2 = std::chrono::duration_cast<std::chrono::microseconds>(second_time_end - second_time);
        // printf("[GET_SCHED][%d][DL_SCHED][TIME]:%lu\n", duration.count());
        if (duration_D2.count() > 1000)
        {
          printf("[DL_SCHED][OVER][1000]duration_D1");
          printf("[DL_SCHED][I=%d][OVER][1000][TIME]duration_D1:%lu\n", i, duration_D2.count());
        }
      }
      else if (resourceMap[tx_bd][tx_fq][tx_sf][i] == logicChanType_t::SCH1)
      {
        mac_h->getSch1(sched_result.sch1[i].len, sched_result.sch1[i].rnti,i);

        if (area_mode == 0)
        {
          sched_set_parameter(tti, sched_result.sch1[i].sch1Cfg, PSCH);
        }
        else
        {
          sched_set_parameter(tti, sched_result.sch1[i].sch1Cfg, DS_PDTCH_1);
        }
        sched_result.sch1[i].is_sched = true;
      }
      else if (resourceMap[tx_bd][tx_fq][tx_sf][i] == logicChanType_t ::SYCH) // 调度SYCH
      {
        sched_result.sych[i] = true;
        int time_start = tx_sf * 60;
        if (start_flag == 0)
        {
          start_time = time_start;
          start_flag = 1;
        }
        interval = time_start - start_time;
        totalTime += interval;
        if (interval > overtimeT)
        {
          interval = 0;
          start_flag = 0;
          // sched_result.sych[i] = false;
        }
        // start = std::chrono::high_resolution_clock::now();
        std::cout << "SYCH---------------:" << i << std::endl;
        mac_h->sych_update();
        mac_h->getsych();
        // end = std::chrono::high_resolution_clock::now();
        // duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "sych   tti----------------------" << tti << std::endl;
        // duration = end - start;
        // float ms=duration.count()*1000.0f;
        // if(ms>overtimeT||mac_h->get_CI_state(70)==1)   //返回CI为confirm时
        if (interval > overtimeT || mac_h->get_CI_state(70) == 1) // 返回CI为confirm时
        {
          std::cout << "avoid repeat" << std::endl;
          mac_h->sych_update();
        }
        // time+=ms;
        std::cout << "Total Time:" << totalTime << "ms" << std::endl;
        std::cout << "Time took:" << interval << "ms" << std::endl;
        if (mac_h->get_CI_state(70) == 1)
        {
          deallocate_source(sych_source);
          std::cout << "start set up" << std::endl;
          alloc_source();
        }
        // if(time>Tsync||psychSS==1)
        if (totalTime > Tsync)
        {
          deallocate_source(sych_source);
          std::cout << "deallocate sych source" << std::endl;
        }
      }
    return true;
  }

  void wx_sched::reset_sib_resourceMap()
  {
    std::cout << "[SCHED][prev_Handover_frame_off]=" << prev_Handover_frame_off << std::endl;
    std::cout << "[SCHED][current_Handover_frame_off]=" << current_Handover_frame_off << std::endl;
    for (uint8_t &n : sibFrameLoc)
    {
      if (resourceMap[sib.band_ID][sib.freq][(n + prev_Handover_frame_off)%52][sib.slot] == logicChanType_t::SIB)
      {
        resourceMap[sib.band_ID][sib.freq][(n + prev_Handover_frame_off)%52][sib.slot] = srsenb::logicChanType_t ::NULLTYPE;
      }
      resourceMap[sib.band_ID][sib.freq][(n + current_Handover_frame_off)%52][sib.slot] = logicChanType_t::SIB;
      // sfmap[n]=true;
    }
  }

  void wx_sched::reset_bbch_resourceMap()
  {
    std::cout << "sib.band_ID=" << sib.band_ID << std::endl;
    std::cout << "sib.freq=" << sib.freq << std::endl;
    // // 濉玃FCCH璧勬簮
    for (int i = 0; i < 8; i++)
    {
      // 娓呴櫎涔嬪墠鐨勮祫婧愰厤缃�
      resourceMap[sib.band_ID][sib.freq][(pfcchLoc[i][0] + prev_Handover_frame_off)%52][pfcchLoc[i][1]] = logicChanType_t ::NULLTYPE;
      // 鐩稿甯у亸绉籧urrent_Handover_frame_off
      std::cout << "pfcchLoc[i][0] + current_Handover_frame_off=" << pfcchLoc[i][0] + current_Handover_frame_off << std::endl;
      resourceMap[sib.band_ID][sib.freq][(pfcchLoc[i][0] + current_Handover_frame_off)%52][pfcchLoc[i][1]] = logicChanType_t ::FCCH;
      //  sfmap[n]=true;
      //  std::cout<<"pfcchLoc[i][0]"<<static_cast<int>(pfcchLoc[i][0])<<std::endl;
      //  std::cout<<"pfcchLoc[i][1]"<<static_cast<int>(pfcchLoc[i][1])<<std::endl;
    }

    // 濉獴BCH璧勬簮
    int bbch_sf1,bbch_sf2;
    for (int i = 1; i <= MAX_SF; i++)
    {
      int len = fnConfig[bbch.fram].size();
      for (int j = 0; j < len; j++)
      {
        if ((i / 2) % 4 == fnConfig[bbch.fram][j])
        {
          bbch_sf1 = (i - 1 + prev_Handover_frame_off)%MAX_SF;
          // 鍏堥噴鏀捐祫婧�
          if (resourceMap[bbch.band_ID][bbch.freq][bbch_sf1][bbch.slot] == srsenb::logicChanType_t::BBCH)
          {
            std::cout << "unmap bbch.sf="<<bbch_sf1<< std::endl;
            resourceMap[bbch.band_ID][bbch.freq][bbch_sf1][bbch.slot] = srsenb::logicChanType_t::NULLTYPE;
            // sfmap[n]=true;
          }
          // 鏍规嵁current_Handover_frame_off閲嶆柊璁剧疆璧勬簮
          bbch_sf2 = (i - 1 + current_Handover_frame_off)%MAX_SF;
          if (resourceMap[bbch.band_ID][bbch.freq][bbch_sf2][bbch.slot] == srsenb::logicChanType_t::NULLTYPE)
          {
            std::cout << "map bbch.sf="<<bbch_sf2<< std::endl;
            resourceMap[bbch.band_ID][bbch.freq][bbch_sf2][bbch.slot] = srsenb::logicChanType_t::BBCH;
            // sfmap[n]=true;
          }
        }
      }
    }
    int agch_sf1,agch_sf2;
    // 濉獳GCH璧勬簮
    for (int i = 1; i <= MAX_SF; i++)
    {
      int len = fnConfig[agch.fram].size();
      for (int j = 0; j < len; j++)
      {
        if ((i / 2) % 4 == fnConfig[agch.fram][j])
        {
          agch_sf1 = (i - 1 + prev_Handover_frame_off)%MAX_SF;
          // 鍏堥噴鏀捐祫婧�
          if (resourceMap[agch.band_ID][agch.freq][agch_sf1][agch.slot] == srsenb::logicChanType_t ::AGCH)
          {
            //std::cout << "unmap agch.sf="<<agch_sf1<< std::endl;
            resourceMap[agch.band_ID][agch.freq][agch_sf1][agch.slot] = srsenb::logicChanType_t ::NULLTYPE;
          }
          // 鏍规嵁current_Handover_frame_off閲嶆柊璁剧疆璧勬簮
          agch_sf2 = (i - 1 + current_Handover_frame_off)%MAX_SF;
          if (resourceMap[agch.band_ID][agch.freq][agch_sf2][agch.slot] == srsenb::logicChanType_t ::NULLTYPE && resourceMap[agch.band_ID][agch.freq][agch_sf2][sib.slot] == NULLTYPE)
          {
            resourceMap[agch.band_ID][agch.freq][agch_sf2][agch.slot] = srsenb::logicChanType_t ::AGCH;
            // std::cout<<"agch.slot"<<agch.slot<<std::endl;
            //std::cout << "map agch.sf="<<agch_sf2<< std::endl;
            // sfmap[n]=true;
          }
        }
      }
    }
  }
  void wx_sched::reset_mib_resourceMap()
  {
    resourceMap[1][1][0 + prev_Handover_frame_off][0] = logicChanType_t::NULLTYPE;
    resourceMap[1][1][0 + current_Handover_frame_off][0] = logicChanType_t::MBCH;
    std::cout << "current_Handover_frame_off=" << current_Handover_frame_off << std::endl;
    for (uint8_t &n : pmbchFrameLoc)
    {
      if (resourceMap[pcch.band_ID][pcch.freq][(n + prev_Handover_frame_off)%52][0] == logicChanType_t::PCCH)
      {
        resourceMap[pcch.band_ID][pcch.freq][(n + prev_Handover_frame_off)%52][0] = logicChanType_t::NULLTYPE;
      }
      resourceMap[pcch.band_ID][pcch.freq][(n + current_Handover_frame_off)%52][0] = logicChanType_t::PCCH;
    }
  }

  void wx_sched::Reset_Allresource_when_frameOff_NotZero()
  {
    // 鏍规嵁frame_off閲嶇疆鎵€鏈夎祫婧�
    std::cout << " Start Reset ALL Resource " << std::endl;
    reset_sib_resourceMap();
    reset_bbch_resourceMap();
    reset_mib_resourceMap();
    prev_Handover_frame_off = current_Handover_frame_off;
    std::cout << "prev_Handover_frame_off=" << prev_Handover_frame_off << std::endl;
  }
}
// namespace srsenb