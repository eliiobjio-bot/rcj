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
#include "srsenb/hdr/adp.h"
#include <pthread.h>
#include <string.h>

#include "srsenb/hdr/stack/mac/mac.h"
#include "srsran/adt/pool/obj_pool.h"
#include "srsran/common/rwlock_guard.h"
#include "srsran/common/standard_streams.h"
#include "srsran/common/time_prof.h"
#include "srsran/interfaces/enb_mac_interfaces.h"
#include "srsran/interfaces/enb_phy_interfaces.h"
#include "srsran/interfaces/enb_rlc_interfaces.h"
#include "srsran/interfaces/enb_rrc_interfaces.h"
#include "srsran/srslog/event_trace.h"
#include <map>
#include <unordered_map>
#include <bitset>
#include <iostream>
#include <chrono>
#include <iomanip>
// #define WRITE_SIB_PCAP
using namespace asn1::rrc;
namespace srsenb
{
  // V1.0.0#####################################################################################
  // ioT############################################
  psych_pdu psych;

  float overtimeT = 70.0;
  float notAdjustT = 280.0;
  int sendSychTime = 0;
  int adjust_flag = 0;
  int adjust_start = 0;
  int adjust_interval = 0;
  uint8_t psychSS;
  std::unordered_map<float, int> TAhashMap;
  std::unordered_map<float, int> prachTAhashMap;
  std::unordered_map<float, int> dataTAhashMap;
  std::unordered_map<int, int> FAhashMap;
  std::unordered_map<int, int> prachFAhashMap;
  std::map<float, int> PAmap = {{0, 0}, {-0.5, 1}, {-1, 2}, {-2, 3}, {0.5, 4}, {1, 5}, {2, 6}, {4, 7}};
  psych_ce sych_ce;
  // mac_interface_phy_lte::psychInfo_t psychInfo;

  int mac::get_CI_state(uint32_t rnti)
  {
    std::cout << "mac get ci: " << ue_db[mac_rnti]->get_ci_check_state() << std::endl;
    return ue_db[mac_rnti]->get_ci_check_state();
  }

  void mac::sychInit()
  {
    int n = 0;
    for (float i = -15.875; i <= 15.875; i += 0.125)
    {
      float a = (float)(int)(i * 100) / 100;
      TAhashMap[a] = n;
      n++;
    }
    int m = 0, k = 17;
    for (int j = 0; j <= 2400; j += 160)
    {
      FAhashMap[j] = m;
      m++;
    }
    for (int l = -160; l >= -2400; l -= 160)
    {
      FAhashMap[l] = k;
      k++;
    }
    // std::cout <<"PA:"<<psychInfo.pa<<std::endl;
    // std::cout <<"TA:"<<psychInfo.ta<<std::endl;
    // std::cout <<"FA:"<<psychInfo.fa<<std::endl;
    // std::cout <<"FAhashMap:"<<FAhashMap[psychInfo.fa]<<std::endl;
    // std::cout <<"TAhashMap:"<<TAhashMap[psychInfo.ta]<<std::endl;
    // std::cout <<"PAmap:"<<PAmap[psychInfo.pa]<<std::endl;
  }
  void mac::readFrameOffCfg(int &sched_frameOff)
  {
    sched_frameOff = mac_cfg.Frame_Off;
    std::cout << "sched_frameOff=" << sched_frameOff << std::endl;
  }

  void mac::readRachCfg(loc_info &rach)
  {
    rach.band_ID = mac_cfg.rach.BandID;
    rach.fram = mac_cfg.rach.FrameAssign;
    rach.freq = mac_cfg.rach.freq;
    rach.slot = mac_cfg.rach.SlotStart;
  }

  void mac::readTbcchCfg(loc_info &IoTsi)
  {
    IoTsi.band_ID = mac_cfg.IoTsi.BandID;
    IoTsi.freq = mac_cfg.IoTsi.freq;
    IoTsi.fram = mac_cfg.IoTsi.FrameAssign;
    // IoTsi.slot    = mac_cfg.IoTsi.SlotStart;
  }

  void mac::readBbchCfg(loc_info &bbch)
  {
    bbch.band_ID = mac_cfg.bbch.BandID;
    bbch.freq = mac_cfg.bbch.freq;
    bbch.fram = mac_cfg.bbch.FrameAssign;
    bbch.slot = mac_cfg.bbch.SlotStart;
  }
  void mac::readAgchCfg(loc_info &agch)
  {
    agch.band_ID = mac_cfg.agch.BandID;
    agch.freq = mac_cfg.agch.freq;
    agch.fram = mac_cfg.agch.FrameAssign;
    agch.slot = mac_cfg.agch.SlotStart;
  }
  void mac::readSibCfg(loc_info &sib)
  {

    sib.band_ID = mac_cfg.sib.BandID;
    sib.freq = mac_cfg.sib.freq;
    sib.slot = mac_cfg.sib.SlotStart;
  }
  void mac::readPcchCfg(loc_info &pcch)
  {
    pcch.band_ID = mac_cfg.pcch.BandID;
    pcch.freq = mac_cfg.pcch.freq;
  }
  // 读IoTsi消息
  bool mac::readIoTsi(int &IoTsiLen_)
  {
    uint8_t BandID, FrameID;
    bzero(IoTsiData, MAX_IoTsi_PDU_LEN);
    if (!rrc_h->read_pdu_IoTsi(IoTsiData, IoTsiLen_, tbcchSlotStart, BandID, FrameID))
      return false;
    // fillResourceMap();
    return true;
  }

  bool mac::readMib(int &mibLen_, int sfn)
  {
    uint8_t BandID, FrameID;

    if (!rrc_h->read_pdu_mib(mibData, mibLen_, bcchSlotStart, BandID, FrameID, sfn))
      return false;
    // fillResourceMap();
    return true;
  }
  bool mac::readSib(int &sibLen_)
  {
    sibInfo_t *sibInfo = new sibInfo_t;
    if (!rrc_h->read_pdu_sib(sibData, sibLen_, sibInfo))
      return false;
    bbchSlotcfg = sibInfo->bbchSlotcfg;
    rar_window = sibInfo->rarWindows;
    // fillResourceMap(sibInfo->bbchFrameAssinment, sibInfo->agchFrameAssinment);
    return true;
  }
  bool mac::readPcch(int &pcchLen_, int sfn)
  {
    uint8_t BandID, FrameID;
    std::cout << "----------------------read-pcch--------------------" << std::endl;
    if (!rrc_h->read_pdu_pcch_wx_s(sfn, pcchData, pcchLen_))
    {
      return false;
    }

    for (int i = 0; i < pcchLen_; i++)
    {
      printf("PCCHDATA:%x\n", *(pcchData + i));
    }
    return true;
  }
  // 生成ptdch的pdu
  void mac::generatePtdchPdu(uint8_t *payload, uint8_t *pdu, int IoTsiLen)
  {
    // srsran::console("生成IoTsi PDU\n");
    uint8_t subHead = 0;
    subHead = (TBCCH_LCID << 6) | IoTsiLen;
    memcpy(pdu, &subHead, 1);
    pdu++;
    memcpy(pdu, payload, MAX_IoTsi_PDU_LEN - 1);
  }

  /*void mac::generatePmbchPdu(uint8_t *payload, uint8_t **pcch_payload, uint8_t *pdu, int mibL, int pcchL, int si, int index)
  {
    uint8_t pcchHead = 0;
    pcchHead = si << 6 | (1 << 5) | PCCH_LCID;
    uint8_t Len = pcchL;
    if (index == 1)
    {
      uint8_t mibHead = 0;
      mibHead = BCCH_LCID;
      *pdu = mibHead;
      if (pcchL > 0)
      {
        *pdu = 1 << 5 | mibHead;
        pdu++;
        *pdu = pcchHead;
        pdu++;
        *pdu = Len;
      }
      pdu++;
      memcpy(pdu, payload, mibL);
      pdu += mibL;
      if (pcchL > 0)
      {
        memcpy(pdu, *pcch_payload, pcchL);
        *pcch_payload += pcchL;
      }
    }
    else
    {
      if (pcchL > 0)
      {
        *pdu = pcchHead;
        pdu++;
        *pdu = Len;
        pdu++;
        memcpy(pdu, *pcch_payload, pcchL);
        *pcch_payload += pcchL;
      }
      else
      {
        *pdu = Pading_LCID;
        pdu++;
        memset(pdu, 0xff, MAX_MIB_PDU_LEN - 1);
      }
    }
  }*/

  //**********************version 1.2  sxy  ********************************************************
  void mac::generatePmbchPdu(uint8_t *payload, uint8_t **pcch_payload, uint8_t *pdu, int mibL, int pcchL, int index, bool is_last)
  {
    uint8_t pcchHead1 = 0;
    uint8_t pcchHead2 = 0;
    pcchHead1 = 1 << 5 | PCCH_LCID;
    pcchHead2 = PCCH_LCID;

    uint8_t Len = pcchL;
    if (index == 1)
    {
      uint8_t mibHead = 0;
      mibHead = BCCH_LCID;
      *pdu = mibHead; // 写MIB子头
      // if(pcchL>0){
      // *pdu=1<<5|mibHead;
      // pdu++;
      // *pdu=pcchHead;
      // pdu++;
      // *pdu=Len;
      // }
      pdu++;
      memcpy(pdu, payload, mibL); // 写MIB数据
      pdu += mibL;                // 移动MIB SDU的字节数
      // if(pcchL>0)
      // {
      // memcpy(pdu, *pcch_payload, pcchL);
      // *pcch_payload+=pcchL;
      // }
      memset(pdu, 0x00, MAX_MIB_PDU_LEN - mibL - 1); // 填充padding, 14-10-1
    }
    else
    {
      if (pcchL == 12 && !is_last)
      {
        *pdu = pcchHead1;
        pdu++;
        *pdu = pcchHead1;
        pdu++;
        memcpy(pdu, *pcch_payload, pcchL);
        *pcch_payload += pcchL;
        // pdu+=pcchL;
      }
      else if (pcchL == 12 && is_last)
      {
        *pdu = pcchHead1;
        pdu++;
        *pdu = pcchHead2;
        pdu++;
        memcpy(pdu, *pcch_payload, pcchL);
        // pdu+=pcchL;
      }
      else if (pcchL == 6)
      {
        *pdu = pcchHead2;
        pdu++;
        memcpy(pdu, *pcch_payload, pcchL);
        pdu += pcchL;
        memset(pdu, 0x00, MAX_MIB_PDU_LEN - pcchL - 1);
        // pdu+=MAX_MIB_PDU_LEN-pcchL-1;
      }
      else if (pcchL == 0)
      {
        *pdu = Pading_LCID;
        pdu++;
        memset(pdu, 0x00, MAX_MIB_PDU_LEN - 1);
        // pdu+=MAX_MIB_PDU_LEN;
      }
    }
  }
  //**********************version 1.2  sxy  ********************************************************

  void mac::generateSibPdu(int si, uint8_t *payload, uint8_t *pdu, int sibLen)
  {
    uint8_t subHead = 0;
    if (si == 0b00)
    {
      subHead |= BCCH_LCID;
      memcpy(pdu, &subHead, 1);
      pdu++;
      memcpy(pdu, payload, sibLen);
      memset(pdu + sibLen, 0, MAX_SIB_PDU_LEN - 1 - sibLen);
    }
    else if (si == 0b01)
    {
      subHead = ((si << 6) | (1 << 5)) | BCCH_LCID;
      memcpy(pdu, &subHead, 1);
      uint8_t L = MAX_SIB_PDU_LEN - 2;
      pdu++;
      memcpy(pdu, &L, 1);
      pdu++;
      memcpy(pdu, payload, L);
    }
    else if (si == 0b10)
    {
      subHead = ((si << 6) | (1 << 5)) | BCCH_LCID;
      memcpy(pdu, &subHead, 1);
      uint8_t L = MAX_SIB_PDU_LEN - 2;
      pdu++;
      memcpy(pdu, &L, 1);
      pdu++;
      memcpy(pdu, payload, L);
    }
    else
    {
      subHead = ((si << 6) | (1 << 5)) | BCCH_LCID;
      memcpy(pdu, &subHead, 1);
      uint8_t L = MAX_SIB_PDU_LEN - 2;
      pdu++;
      memcpy(pdu, &L, 1);
      pdu++;
      memcpy(pdu, payload, L);
      // memcpy(pdu ,&sibLen , 1);
      // pdu++;
      memcpy(pdu, payload, sibLen);
      memset(pdu + sibLen, 0, MAX_SIB_PDU_LEN - sibLen - 1);
    }
  }
  // kuopin
  void mac::SS_generateSibPdu(int si, uint8_t *payload, uint8_t *pdu, int sibLen)
  {
    uint8_t subHead = 0;
    // std::cout<<"siblen:"<<sibLen<<std::endl;
    uint8_t *init = pdu;
    if (si == 0b00)
    {
      subHead |= BCCH_LCID;
      memcpy(pdu, &subHead, 1);
      pdu++;
      memcpy(pdu, payload, sibLen);
      memset(pdu + sibLen, 0, MAX_SS_SIB_PDU_LEN - 1 - sibLen);
    }
    else if (si == 0b01)
    {
      subHead = ((si << 6) | (1 << 5)) | BCCH_LCID;
      memcpy(pdu, &subHead, 1);
      uint8_t L = MAX_SS_SIB_PDU_LEN - 2;
      pdu++;
      memcpy(pdu, &L, 1);
      pdu++;
      memcpy(pdu, payload, L);
    }
    else if (si == 0b10)
    {
      subHead = ((si << 6) | (1 << 5)) | BCCH_LCID;
      memcpy(pdu, &subHead, 1);
      uint8_t L = MAX_SS_SIB_PDU_LEN - 2;
      pdu++;
      memcpy(pdu, &L, 1);
      pdu++;
      memcpy(pdu, payload, L);
    }
    else
    {
      subHead = ((si << 6) | (1 << 5)) | BCCH_LCID;
      memcpy(pdu, &subHead, 1);
      uint8_t L = MAX_SS_SIB_PDU_LEN - 2;
      pdu++;
      memcpy(pdu, &L, 1);
      pdu++;
      memcpy(pdu, payload, L);
      //  memcpy(pdu ,&sibLen , 1);
      // pdu++;
      memcpy(pdu, payload, sibLen);
      memset(pdu + sibLen, 0, MAX_SS_SIB_PDU_LEN - sibLen - 1);
    }
  }

  void mac::set_psch(int pduLen)
  {
    switch (pduLen)
    {
    case 14: // psch11 1/2 QPSK
      psch_pdu.pui.MCS = 0;
      psch_pdu.pui.L = 0;
      break;
    case 20: // psch11 2/3 QPSK
      psch_pdu.pui.MCS = 1;
      psch_pdu.pui.L = 0;
      break;
    case 23: // psch11 4/5 QPSK
      psch_pdu.pui.MCS = 2;
      psch_pdu.pui.L = 0;
      break;
    case 36: // psch12 1/2QPSK
      // psch_pdu.pui.MCS = 0;
      // psch_pdu.pui.L = 1;
      // psch_pdu.pui.EI = 1;

      psch_pdu.pui.MCS = 0;
      psch_pdu.pui.L = 1;
      psch_pdu.pui.EI = 0;
      break;
    case 31: // psch12 1/2QPSK epui
      psch_pdu.pui.MCS = 0;
      psch_pdu.pui.L = 1;
      psch_pdu.pui.EI = 1;
      break;
    case 48: // psch12 2/3QPSK
      psch_pdu.pui.MCS = 1;
      psch_pdu.pui.L = 1;
      psch_pdu.pui.EI = 0;
      break;
    case 58: // psch12 4/5QPSK
      psch_pdu.pui.MCS = 2;
      psch_pdu.pui.L = 1;
      psch_pdu.pui.EI = 0;
      break;
    case 102: // DL psch51 1/2QPSK
      psch_pdu.pui.MCS = 0;
      psch_pdu.pui.L = 0;
      break;
    case 144: // DL psch51 7/10QPSK
      psch_pdu.pui.MCS = 3;
      psch_pdu.pui.L = 0;
      break;
    case 185: // DL psch51 3/5 8PSK
      psch_pdu.pui.MCS = 4;
      psch_pdu.pui.L = 0;
      break;
    case 219: // DL psch52 1/2 QPSK
      psch_pdu.pui.MCS = 0;
      psch_pdu.pui.L = 1;
      break;
    case 307: // DL psch52 7/10 QPSK
      psch_pdu.pui.MCS = 3;
      psch_pdu.pui.L = 1;
      break;
    case 395: // DL psch52 3/5 8PSK
      psch_pdu.pui.MCS = 4;
      psch_pdu.pui.L = 1;
      break;

    // UL
    case 99: // UL psch51 1/2QPSK
      psch_pdu.pui.MCS = 0;
      psch_pdu.pui.L = 1;
      break;
    case 140: // UL psch51  7/10 QPSK
      psch_pdu.pui.MCS = 3;
      psch_pdu.pui.L = 1;
      break;
    case 181: // UL psch51 3/5 8PSK
      psch_pdu.pui.MCS = 4;
      psch_pdu.pui.L = 1;
      break;
    case 216: // UL psch52 1/2 QPSK
      psch_pdu.pui.MCS = 0;
      psch_pdu.pui.L = 1;
      break;
    case 303: // UL psch52 7/10 QPSK
      psch_pdu.pui.MCS = 3;
      psch_pdu.pui.L = 1;
      break;
    case 390: // UL psch52 3/5 8PSK
      psch_pdu.pui.MCS = 4;
      psch_pdu.pui.L = 1;
      break;

    default:
      psch_pdu.pui.MCS = 0;
      psch_pdu.pui.L = 1;
      psch_pdu.pui.EI = 1;
      break;
    }
  }

  void mac::setupAllocate(dl_allocate &allocSource)
  {
    std::cout << "[setupAllocate][MCS]=" << MAC_mcs_Info.MCS << std::endl;
    std::cout << "RRC Chanel alloc   " << allocSource.Type << std::endl;
    switch (allocSource.Type)
    {
    case 0:
      allocSource.LogicType = SYCH;
      allocSource.pduLen = 14;
      phy_h->set_ul_type(4);
      break;
    case 1: // pdch1-1
      std::cout << " case 1 :  " << std::endl;
      if (Current_Map.dataType == Voice)
      {
        if (Current_Map.voicetype == voice_24k)
        {
          std::cout << " mac voice_24k " << std::endl;
          allocSource.LogicType = SCH2;
          allocSource.pduLen = 19;
          phy_h->set_ul_type(2);
        }
        else if (Current_Map.voicetype == voice_48k)
        {
          std::cout << " mac voice_48k " << std::endl;
          allocSource.LogicType = SCH2;
          allocSource.pduLen = 37;
          phy_h->set_ul_type(2);
        }
        else if (Current_Map.voicetype == voice_800)
        {
          std::cout << " mac voice_800 " << std::endl;
          allocSource.LogicType = SCH2;
          allocSource.pduLen = 7;
          phy_h->set_ul_type(2);
        }
      }
      else if (Current_Map.dataType == Data)
      {
        allocSource.LogicType = SCH2;
        allocSource.pduLen = 27;
        phy_h->set_ul_type(2);
      }
      else
      {
        allocSource.LogicType = SCH2;
        if (ue_category == 14)
        {
          std::cout << "xxxxxxxxxxxx1111" << std::endl;
          allocSource.pduLen = 7;
        }
        else
        {
          allocSource.pduLen = 19;
        }
        phy_h->set_ul_type(2);
      }
      break;
    case 2: // pdch 1-2
      std::cout << " case 2 :  " << std::endl;
      if (Current_Map.dataType == Voice || Current_Map.dataType == Control)
      {
        allocSource.LogicType = SCH2;
        allocSource.pduLen = 37;
        phy_h->set_ul_type(3);
      }
      else
      {
        allocSource.LogicType = SCH2;
        allocSource.pduLen = 55;
        phy_h->set_ul_type(3);
      }
      break;
    case 3: // PSCH 11
      allocSource.LogicType = SCH1;

      allocSource.pduLen = 14;

      if (MAC_mcs_Info.MCS == 1)
      {
        allocSource.pduLen = 20;
      }
      else if (MAC_mcs_Info.MCS == 2)
      {
        allocSource.pduLen = 23;
      }
      std::cout << "[MCS]allocSource.pduLen:" << allocSource.pduLen << std::endl;

      phy_h->set_ul_type(5);
      break;
    case 4: // PSCH 12
      allocSource.LogicType = SCH1;

      allocSource.pduLen = 36;

      // allocSource.pduLen = 31; // EPUI+1/2QPSK
      if (MAC_mcs_Info.MCS == 1)
      {
        allocSource.pduLen = 48;
      }
      else if (MAC_mcs_Info.MCS == 2)
      {
        allocSource.pduLen = 58;
      }

      phy_h->set_ul_type(6);
      break;
    case 5: // PSCH 51
      allocSource.LogicType = SCH1;

      allocSource.pduLen = 102; // psch5-1

      if (MAC_mcs_Info.MCS == 3)
      {
        allocSource.pduLen = 144;
      }
      else if (MAC_mcs_Info.MCS == 4)
      {
        allocSource.pduLen = 185;
      }

      phy_h->set_ul_type(7);
      break;
    case 6:
      allocSource.LogicType = SCH1;

      //  allocSource.pduLen=395;//psch5-2
      //  allocSource.pduLen=214;//EPUI+1/2QPSK'

      allocSource.pduLen = 219;

      if (MAC_mcs_Info.MCS == 3)
      {
        allocSource.pduLen = 307;
      }
      else if (MAC_mcs_Info.MCS == 4)
      {
        allocSource.pduLen = 395;
      }

      phy_h->set_ul_type(8);
      break;
    //  case 11:
    //   allocSource.LogicType=SCH2;
    //   allocSource.pduLen=27;
    //   phy_h->set_ul_type(2);
    //   break;
    //  case 12:
    //   allocSource.LogicType=SCH2;
    //   if(voiceSpeed==0){
    //     allocSource.pduLen=19;
    //     phy_h->set_ul_type(2);
    //   }else if(voiceSpeed==1){
    //     allocSource.pduLen=37;
    //     phy_h->set_ul_type(3);
    //   }
    //   break;
    //  case 13:
    //   allocSource.LogicType=SCH2;
    //   allocSource.pduLen=19;
    //   phy_h->set_ul_type(2);
    //   break;
    //  case 14:
    //   allocSource.LogicType=SCH2;
    //   allocSource.pduLen=27;
    //   phy_h->set_ul_type(2);
    //   break;
    default:
      std::cout << "RRC Chanel alloc error" << std::endl;
      break;
    }
  }

  void mac::set_uecategory14()
  {
    std::cout << " set_uecategory14 " << std::endl;
    phy_h->set_ul_type(2);
  }

  void mac::setulAlloc(ul_allocate &allocSource)
  {

    switch (allocSource.Type)
    {
    case 0:
      allocSource.LogicType = SYCH;
      allocSource.pduLen = 14;
      phy_h->set_ul_type(4);
      break;
    case 1:
      allocSource.LogicType = SCH2;
      if (ue_category == 14)
      {
        allocSource.pduLen = 7;
      }
      else
      {
        allocSource.pduLen = 19;
      }
      phy_h->set_ul_type(2);
      std::cout << "setulAlloc!" << std::endl;
      break;
    case 2:
      allocSource.LogicType = SCH2;
      allocSource.pduLen = 37;
      phy_h->set_ul_type(3);
      break;
    case 3:
      allocSource.LogicType = SCH1;
      // allocSource.pduLen=14;
      // allocSource.pduLen=20;//psch1-1
      // allocSource.pduLen = 23;
      allocSource.pduLen = 14;

      if (MAC_mcs_Info.MCS == 1)
      {
        allocSource.pduLen = 20;
      }
      else if (MAC_mcs_Info.MCS == 2)
      {
        allocSource.pduLen = 23;
      }
      std::cout << "[MCS]allocSource.pduLen:" << allocSource.pduLen << std::endl;

      phy_h->set_ul_type(5);
      break;
    case 4: // PSCH 12
      allocSource.LogicType = SCH1;
      // allocSource.pduLen = 36;

      allocSource.pduLen = 36;

      // allocSource.pduLen = 31; // EPUI+1/2QPSK
      if (MAC_mcs_Info.MCS == 1)
      {
        allocSource.pduLen = 48;
      }
      else if (MAC_mcs_Info.MCS == 2)
      {
        allocSource.pduLen = 58;
      }
      phy_h->set_ul_type(6);
      break;
    case 5: // psch5-1
      allocSource.LogicType = SCH1;
      // allocSource.pduLen = 102; // psch5-1

      allocSource.pduLen = 102; // psch5-1

      if (MAC_mcs_Info.MCS == 3)
      {
        allocSource.pduLen = 144;
      }
      else if (MAC_mcs_Info.MCS == 4)
      {
        allocSource.pduLen = 185;
      }

      phy_h->set_ul_type(7);
      break;
    case 6:
      allocSource.LogicType = SCH1;
      // allocSource.pduLen=219;
      //  allocSource.pduLen=395;//psch5-2
      // allocSource.pduLen = 214; // EPUI+1/2QPSK

      allocSource.pduLen = 219;

      if (MAC_mcs_Info.MCS == 3)
      {
        allocSource.pduLen = 307;
      }
      else if (MAC_mcs_Info.MCS == 4)
      {
        allocSource.pduLen = 395;
      }

      phy_h->set_ul_type(8);
      break;
    case 11:
      std::cout << "ASfasfasgfag" << std::endl;
      allocSource.LogicType = SCH2;
      allocSource.pduLen = 27;
      phy_h->set_ul_type(2);
      break;
    default:
      std::cout << "RRC Chanel alloc error" << std::endl;
      break;
    }
  }
  void mac::setAllocate(dl_allocate &allocSource)
  {
    // PSCH1-1=14/20/23  ||  PSCH1-2=36/48/58 ||   PSCH5-1=102/144/185  || PSCH5-2=219/307/395
    switch (allocSource.Type)
    {
    case 0:
      allocSource.LogicType = SYCH;
      allocSource.pduLen = 20;
      break;
    case 1:
      allocSource.LogicType = SCH2;
      if (ue_category == 14)
      {
        allocSource.pduLen = 7;
      }
      else
      {
        allocSource.pduLen = 19;
      }
      break;
    case 2:
      allocSource.LogicType = SCH2;
      allocSource.pduLen = 37;
      break;
    case 3:
      allocSource.LogicType = SCH1;
      // allocSource.pduLen=14;
      // allocSource.pduLen=20;//psch1-1
      allocSource.pduLen = 23;
      break;
    case 4:
      allocSource.LogicType = SCH1;
      allocSource.pduLen = 36;
      break;
    case 5:
      allocSource.LogicType = SCH1;
      allocSource.pduLen = 102; // psch5-1
      break;
    case 6:
      allocSource.LogicType = SCH1;
      // allocSource.pduLen=219;
      //  allocSource.pduLen=395;//psch5-2
      allocSource.pduLen = 214; // EPUI+1/2QPSK
      break;
    case 7:
      allocSource.LogicType = SCH2;
      allocSource.pduLen = 19;
      break;
    case 8:
      allocSource.LogicType = SCH2;
      allocSource.pduLen = 95;
      break;
    case 9:
      allocSource.LogicType = SCH2;
      allocSource.pduLen = 175;
      break;
    case 10:
      allocSource.LogicType = SCH2;
      allocSource.pduLen = 19;
      break;
    default:
      std::cout << "RRC Chanel alloc error" << std::endl;
      break;
    }
  }

  void mac::MultisetupAllocate(phy_channel_t &allocSource)
  {
    // std::cout << "Multi RRC Chanel alloc   " << allocSource.Type << std::endl;
    switch (allocSource.Type)
    {
    case 0:
      allocSource.LogicType = SYCH;
      allocSource.pduLen = 20;
      phy_h->set_ul_type(4);
      break;
    case 1:
      allocSource.LogicType = SCH2;
      allocSource.pduLen = 19;
      phy_h->set_ul_type(2);
      break;
    case 2:
      allocSource.LogicType = SCH2;
      allocSource.pduLen = 37;
      phy_h->set_ul_type(3);
      break;
    case 3:
      allocSource.LogicType = SCH1;
      // allocSource.pduLen=14;
      // allocSource.pduLen=20;//psch1-1
      allocSource.pduLen = 23;
      phy_h->set_ul_type(5);
      break;
    case 4:
      allocSource.LogicType = SCH1;
      allocSource.pduLen = 36;
      phy_h->set_ul_type(6);
      break;
    case 5:
      allocSource.LogicType = SCH1;
      allocSource.pduLen = 102; // psch5-1
      if (MAC_mcs_Info.MCS == 3)
      {
        allocSource.pduLen = 144;
      }
      else if (MAC_mcs_Info.MCS == 4)
      {
        allocSource.pduLen = 185;
      }
      std::cout << "pduLen:" << allocSource.pduLen << std::endl;
      std::cout << "RRC Chanel set_ul_type(7)" << std::endl;
      phy_h->set_ul_type(7);
      break;
    case 6:
      allocSource.LogicType = SCH1;
      allocSource.pduLen = 219;
      // allocSource.pduLen=395;//psch5-2
      // allocSource.pduLen = 214; // EPUI+1/2QPSK
      phy_h->set_ul_type(8);
      break;
    default:
      std::cout << "RRC Chanel alloc error" << std::endl;
      break;
    }
  }

  void mac::mac_deallocate()
  {
    std::cout << "mac_deallocate" << std::endl;
    // dlAlloc=tempSource;
    // set_psch(dlAlloc.pduLen);
    // wxScheduler.deallocate_source(dlAlloc);
    // wxScheduler.allocate_source(dlAlloc);
  }

  /*
   * @ 用于目标波束创建用户
   */
  bool mac::wx_Switch_SetUser_in_TargetBeam()
  {
    uint16_t rnti = allocate_ue(0);
    mac_rnti = rnti;
    srsran::console("创建UE,RNTI:%u", rnti);

    std::cout << " mac::wx_Switch_SetUser_in_TargetBeam" << std::endl;
    sched_interface::ue_cfg_t uecfg = {};
    uecfg.supported_cc_list.emplace_back();
    uecfg.supported_cc_list.back().active = true;
    uecfg.supported_cc_list.back().enb_cc_idx = 0;
    uecfg.ue_bearers[0].direction = mac_lc_ch_cfg_t::BOTH;
    uecfg.supported_cc_list[0].dl_cfg.tm = SRSRAN_TM1;
    if (ue_cfg(rnti, &uecfg) != SRSRAN_SUCCESS)
    {
      return false;
    }
    // 在RRC层注册新用户
    if (rrc_h->add_user(rnti, uecfg) == SRSRAN_ERROR)
    {
      std::cout << "add_user flase" << std::endl;
      ue_rem(rnti);
      return false;
    }
    return true;
  }

  void mac::Trigger_MCS_param_config()
  {
    ue_db[70]->mcs_adjust.mcs = psch_pdu.pui.MCS;
    std::cout << "ue_db[70]->mcs_adjust.mcs::=" << (int)ue_db[70]->mcs_adjust.mcs << std::endl;
    ue_db[70]->mcs_adjust.fn_mcs = 0;
    ue_db[70]->mcs_adjust.is_ready = true;
  }

  void mac::RRC_notify_MAC_release(bool is_mac_chanType_release_)
  {
    is_reconf_release = is_mac_chanType_release_;
    std::cout << "is_reconf_release:" << is_reconf_release << std::endl;
  }

  // // 2024.11.12 to notify PHY resolution in advance
  // void mac::updateUpInfo_advanceSingle()
  // {
  // }
  // void mac::updateUpInfo_advanceMulti(){
    
  // }

  //---------------2024.09.04-----------------
  bool mac::phy_channel_list_conf(phy_channel_list_t phy_channel_list, phy_channel_ul_list_t phy_ul_channel_list, mcs_Info mcs_Info_)
  {
    std::cout
        << "[MULTI RECONF PHY][MCS_]=" << mcs_Info_.MCS << std::endl;
    MAC_mcs_Info = mcs_Info_;
    std::cout << "[MAC][MCS]=" << mcs_Info_.MCS << std::endl;
    srsran::console("---------------------MAC receive recof chantype msg--------------\n");

    std::cout << "dealloc band" << dlAlloc.freq << std::endl;
    std::cout << "dealloc band" << dlAlloc.solt << std::endl;
    std::cout << "dealloc band" << dlAlloc.bandID << std::endl;
    wxScheduler.deallocate_source(dlAlloc);

    for (int i = 0; i < (int)lastMuiltSource.size(); i++)
    {
      wxScheduler.deallocate_multiSource(lastMuiltSource[i]);
      phy_h->multi_set_parameters(i, 0, 0, 0, 0, 1);
      phy_h->set_ul_type(0);
    }
    sigle_multi = phy_channel_list.size();
    std::cout << "multidlAlloc.Type=" << (int)multidlAlloc.Type << std::endl;
    lastMuiltSource.resize(sigle_multi);

    for (uint32_t i = 0; i < phy_channel_list.size(); i++)
    {
      std::cout << "phy_channel_list[i].phy_channel bandid" << phy_channel_list[i].phy_channel.bandID << std::endl;
      std::cout << "phy_channel_list[i].phy_channel freqid" << phy_channel_list[i].phy_channel.freqID << std::endl;
      std::cout << "phy_channel_list[i].phy_channel slot" << phy_channel_list[i].phy_channel.slot << std::endl;
      std::cout << "phy_channel_list.size()" << phy_channel_list.size() << std::endl;
      multidlAlloc = phy_channel_list[i].phy_channel;
      std::cout << "multidlAlloc.slot=" << multidlAlloc.slot << std::endl;
      MultisetupAllocate(multidlAlloc);
      set_psch(multidlAlloc.pduLen);

      if (multidlAlloc.Type == 4 || multidlAlloc.Type == 6)
      {
        int temp_slot = 0;
        for (int i = 0; i < 5; i++)
        {
          int temp = 0x01 & (multidlAlloc.slot >> i);
          if (temp > 0)
          {
            int temp_off = 0x01;
            temp_slot = temp_slot | (temp_off << i);
            i += 1;
          }
        }
        std::cout << "temp_slot=" << temp_slot << std::endl;
        multidlAlloc.slot = temp_slot;
      }
      wxScheduler.allocate_source_multi(multidlAlloc);
      phy_h->multi_set_parameters(i, multidlAlloc.bandID, multidlAlloc.freqID, multidlAlloc.slot, 0, sigle_multi);
      std::cout << "multidlAlloc bandid" << phy_channel_list[i].phy_channel.bandID << std::endl;
      if (MAC_mcs_Info.MCS != -1)
      {
        std::cout << "multi 触发MCS参数配置" << std::endl;
        Trigger_MCS_param_config();
      }
      lastMuiltSource[i] = multidlAlloc;
    }
    return false;
  }

  bool mac::reconf_phy(dl_allocate dlallocSource, ul_allocate ulAlloc, mcs_Info mcs_Info_, int Handover_frame_off)
  {
    // 每次进入则把frame_off送入we_sched,为重置资源提供依据
    std::cout << "reconf_phy,PID=" << Pid << std::endl;
    std::cout << "[MAC][RECONF]frame_off=" << Handover_frame_off << std::endl;
    wxScheduler.wx_sched_FrameOff_value(Handover_frame_off);

    std::cout
        << "[RECONF PHY][MCS_]=" << mcs_Info_.MCS << std::endl;
    MAC_mcs_Info = mcs_Info_;
    std::cout << "[MAC][MCS]=" << mcs_Info_.MCS << std::endl;
    srsran::console("---------------------MAC receive recof chantype msg--------------\n");

    std::cout << "dealloc freq" << dlAlloc.freq << std::endl;
    std::cout << "dealloc slot" << dlAlloc.solt << std::endl;
    std::cout << "dealloc band" << dlAlloc.bandID << std::endl;
    wxScheduler.deallocate_source(dlAlloc);

    for (int i = 0; i < (int)lastMuiltSource.size(); i++)
    {
      wxScheduler.deallocate_multiSource(lastMuiltSource[i]);
      phy_h->multi_set_parameters(i, 0, 0, 0, 0, 1);
      phy_h->set_ul_type(0);
    }
    lastMuiltSource.resize(0);
    if (area_mode == 1)
    {
      ulAlloc_ = ulAlloc;
      dlAlloc = dlallocSource;
      setAllocate(dlAlloc);
      setulAlloc(ulAlloc_);
      // set_psch(dlAlloc.pduLen);
      set_psch(ulAlloc_.pduLen);
      if (ulAlloc_.Type == 4 || ulAlloc_.Type == 6 || ulAlloc_.Type == 2)
      {
        int temp_slot = 0;
        for (int i = 0; i < 5; i++)
        {
          int temp = 0x01 & (dlAlloc.solt >> i);
          if (temp > 0)
          {
            int temp_off = 0x01;
            temp_slot = temp_slot | (temp_off << i);
            i += 1;
          }
        }
        std::cout << "temp_slot=" << temp_slot << std::endl;
        ulAlloc_.solt = temp_slot;
      }
      phy_h->set_parameters(ulAlloc_.bandID, ulAlloc_.freq, ulAlloc_.solt, 0);
    }
    if (area_mode == 0)
    {
      dlAlloc = dlallocSource;
      setupAllocate(dlAlloc);
      set_psch(dlAlloc.pduLen); // 常规模式 设置
      phy_h->set_parameters(dlAlloc.bandID, dlAlloc.freq, dlAlloc.solt, 0);
    }
    if (dlAlloc.Type == 4 || dlAlloc.Type == 6 || dlAlloc.Type == 2)
    {
      int temp_slot = 0;
      for (int i = 0; i < 5; i++)
      {
        int temp = 0x01 & (dlAlloc.solt >> i);
        if (temp > 0)
        {
          int temp_off = 0x01;
          temp_slot = temp_slot | (temp_off << i);
          i += 1;
        }
      }
      std::cout << "temp_slot=" << temp_slot << std::endl;
      dlAlloc.solt = temp_slot;
    }
    // // 还需重置其他的所有资源，包括mib,sib，还有？
    wxScheduler.Reset_Allresource_when_frameOff_NotZero();
    wxScheduler.allocate_source(dlAlloc);

    if (MAC_mcs_Info.MCS != -1)
    {
      std::cout << "触发MCS参数配置" << std::endl;
      Trigger_MCS_param_config();
    }
    std::cout << "dlAlloc  type" << dlAlloc.Type << std::endl;
    return false;
  }
  //--------------------------------------------------------------------
  // zyg

  /**
   *@brief  the key value of this code is overwritten(mac_lcid_to_resource[lcid]=configMap_) all parameters
   *@author zhaohongjun
   *@date   2024/05/17
   */
  void mac::addlcidMap(uint32_t lcid, configMap configMap_)
  {
    if (configMap_.voicetype != N_Voice)
    {
      std::cout << "-lcid" << lcid << "configMap_.voicetype = " << configMap_.voicetype << std::endl;
      phy_h->voiceType_mode(configMap_.voicetype);
      voiceLcid = lcid;
    }
    printf("lcid:%d,configMap_%d\n", lcid, configMap_.dl_Type);
    mac_lcid_to_resource.insert(std::make_pair(lcid, configMap_));
    // mac_lcid_to_resource[lcid]=configMap_;
  }

  // void mac::updateMap(uint32_t lcid, ChanType_t reconfig_new_ul_Type, ChanType_t reconfig_new_dl_Type)
  // {
  //   for (auto &it : mac_lcid_to_resource)
  //   {
  //     if (lcid == it.first)
  //     {
  //       std::cout << " zhj update recomfig " << std::endl;
  //       it.second.ul_Type = reconfig_new_ul_Type;
  //       it.second.dl_Type = reconfig_new_dl_Type;
  //       break;
  //     }
  //   }
  // }

  void mac::updateMap(uint32_t lcid, configMap updateMap_)
  {
    for (auto &it : mac_lcid_to_resource)
    {
      if (lcid == it.first)
      {
        std::cout << " Update Reconf Map " << std::endl;
        it.second = updateMap_;
      }
    }
  }

  bool mac::setupInfo_uecategory14(set_up_msg setUp)
  {
    srsran::console("--------MAC Receive Connection setup msg------\n");
    // srsran::console("rnti=%u,consID=%u\n", rnti, consID);
    srsran::console("rnti=%u\n", setUp.rnti, setUp.consID);

    ue_db[setUp.rnti]->mcs_adjust.mcs = psch_pdu.pui.MCS;
    ue_db[setUp.rnti]->mcs_adjust.fn_mcs = 0;
    ue_db[setUp.rnti]->mcs_adjust.is_ready = true;
    for (int i = 0; i < MAX_PENDING_RARS; i++)
    {
      if (network_mode == 0)
      {
        if (pending_rars1[i].temp_crnti == setUp.rnti)
        {
          pending_rars1[i].is_enable = true;
          pending_rars1[i].considInfo.consID = setUp.consID;
          pending_rars1[i].considInfo.considLen = setUp.consIDlen;
          pending_rars1[i].considInfo.conSetupMsg = setUp.pdu;
          pending_rars1[i].considInfo.conSetupMsgLen = setUp.pduLen;
          printf("pending_rars1[i].temp_crnti: %d\n", pending_rars1[i].temp_crnti);
          printf("pending_rars1[i].is_enable: %d\n", pending_rars1[i].is_enable);
          printf("pending_rars1[i].considInfo.consID: %ld\n", pending_rars1[i].considInfo.consID);
          printf("pending_rars1[i].considInfo.considLen: %d\n", pending_rars1[i].considInfo.considLen);
          printf("pending_rars1[i].considInfo.conSetupMsg: %x\n", *(pending_rars1[i].considInfo.conSetupMsg));
          printf("pending_rars1[i].considInfo.conSetupMsgLen: %d\n", pending_rars1[i].considInfo.conSetupMsgLen);
          //***************3/22***********
          std::cout << "mac_adp->udp_.mac_con_req_flag4:" << mac_adp->udp_.mac_con_req_flag << std::endl;

          std::cout << "mac_adp->udp_.mac_push_pdu:" << mac_adp->udp_.mac_push_pdu << std::endl;
          return true;
        }
      }
    }
    return false;
  }

  bool mac::TC300Timeout(set_up_msg setUp)
  {
    TC300_Timeout = true;
    for (int i = 0; i < MAX_PENDING_RARS; i++)
    {
      if (pending_rars1[i].temp_crnti == setUp.rnti)
      {
        // pending_rars1[i].temp_crnti = -1;
        pending_rars1[i].is_enable = true;
        pending_rars1[i].considInfo.consID = setUp.consID;
        pending_rars1[i].considInfo.considLen = setUp.consIDlen;
      }
    }
    return false;
  }

  bool mac::setConsID(set_up_msg setUp, dl_allocate allocSource, ul_allocate ulAlloc)
  {
    srsran::console("--------MAC Receive Connection setup msg------\n");
    srsran::console("rnti=%u\n", setUp.rnti, setUp.consID);

    if (area_mode == 1)
    {
      ulAlloc_ = ulAlloc;
      dlAlloc = allocSource;
      setAllocate(dlAlloc);
      setulAlloc(ulAlloc_);
      set_psch(dlAlloc.pduLen);
      phy_h->set_parameters(ulAlloc_.bandID, ulAlloc_.freq, ulAlloc_.solt, 0);
    }
    if (area_mode == 0)
    {
      ulAlloc_ = ulAlloc;
      setulAlloc(ulAlloc_);
      dlAlloc = allocSource;
      setupAllocate(dlAlloc);
      phy_h->set_parameters(dlAlloc.bandID, dlAlloc.freq, dlAlloc.solt, 0);
    }
    ue_db[setUp.rnti]->mcs_adjust.mcs = psch_pdu.pui.MCS;
    ue_db[setUp.rnti]->mcs_adjust.fn_mcs = 0;
    ue_db[setUp.rnti]->mcs_adjust.is_ready = true;
    for (int i = 0; i < MAX_PENDING_RARS; i++)
    {
      if (network_mode == 0)
      {
        if (pending_rars1[i].temp_crnti == setUp.rnti)
        {
          pending_rars1[i].is_enable = true;
          pending_rars1[i].considInfo.consID = setUp.consID;
          pending_rars1[i].considInfo.considLen = setUp.consIDlen;
          pending_rars1[i].considInfo.conSetupMsg = setUp.pdu;
          pending_rars1[i].considInfo.conSetupMsgLen = setUp.pduLen;
          printf("pending_rars1[i].temp_crnti: %d\n", pending_rars1[i].temp_crnti);
          printf("pending_rars1[i].is_enable: %d\n", pending_rars1[i].is_enable);
          printf("pending_rars1[i].considInfo.consID: %ld\n", pending_rars1[i].considInfo.consID);
          printf("pending_rars1[i].considInfo.considLen: %d\n", pending_rars1[i].considInfo.considLen);
          printf("pending_rars1[i].considInfo.conSetupMsg: %x\n", *(pending_rars1[i].considInfo.conSetupMsg));
          printf("pending_rars1[i].considInfo.conSetupMsgLen: %d\n", pending_rars1[i].considInfo.conSetupMsgLen);
          //***************3/22***********

          if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.mac_con_req_flag)
          {
            while (true)
            {
              if (mac_adp->udp_.mac_receive_info.size() != 0)
              {
                printf("队列里面有连接建立数据\n");
                srsran::unique_byte_buffer_t ttcn_con =
                    srsran::make_byte_buffer();
                ttcn_con->init();
                mac_adp->udp_.mac_receive_info.try_pop(ttcn_con);

                mac_adp->udp_.mac_push_pdu = true;
                std::cout << "  rrc_adp->udp_.mac_push_pdu :" << mac_adp->udp_.mac_push_pdu << std::endl;
                break;
              }
            }
          }
          return true;
        }
      }
      else if (network_mode == 1)
      {
        if (iot_pending_rars1[i].temp_crnti == setUp.rnti)
        {
          iot_pending_rars1[i].is_enable = true;
          iot_pending_rars1[i].considInfo.consID = setUp.consID;
          iot_pending_rars1[i].considInfo.considLen = setUp.consIDlen;
          iot_pending_rars1[i].considInfo.conSetupMsg = setUp.pdu;
          iot_pending_rars1[i].considInfo.conSetupMsgLen = setUp.pduLen;
          return true;
        }
      }
    }
    return false;
  }

  bool mac::getVoiceLcid(uint32_t lcid, int voice_speed)
  {
    voiceLcid = lcid;
    voiceSpeed = voice_speed;
    return true;
  }

  bool mac::setUecategory(uint8_t uecategory, uint16_t ue_cap14_band_id_, uint16_t ue_cap14_freq_id_, uint16_t ue_cap14_slot_, uint16_t ue_cap14_slot_no_handle_)
  {
    ue_category = uecategory;
    ue_cap14_band_id = ue_cap14_band_id_;
    ue_cap14_freq_id = ue_cap14_freq_id_;
    ue_cap14_slot = ue_cap14_slot_no_handle_;
    ue_cap14_slot_phy = ue_cap14_slot;
    std::cout << "[setUecategory][UECAP]:" << ue_category << std::endl;
    std::cout << "[setUecategory][ue_cap14_band_id]:" << ue_cap14_band_id << std::endl;
    std::cout << "[setUecategory][ue_cap14_freq_id]:" << ue_cap14_freq_id << std::endl;
    std::cout << "[setUecategory][ue_cap14_slot]:" << ue_cap14_slot << std::endl;
    std::cout << "[setUecategory][ue_cap14_slot_phy]:" << ue_cap14_slot_phy << std::endl;

    wxScheduler.wx_sched_uecategory(ue_category, ue_cap14_band_id_, ue_cap14_freq_id_, ue_cap14_slot_);
    phy_h->set_uecategory(ue_category);

    return true;
  }

  bool mac::getSrnti(int srnti)
  {
    s_rnti_ = srnti;
    std::cout << "[s_rnti_]: " << s_rnti_ << std::endl;

    return true;
  }

  void mac::setHead(int bi, int ra_id, uint8_t *ptr, bool is_last)
  {
    if (bi)
    {
      *ptr = (uint8_t)bi & 0x0f;
      return;
    }
    if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.TC_614_backoff)
    {
      while (true)
      {
        if (mac_adp->udp_.mac_receive_info.size() != 0)
        {
          srsran::unique_byte_buffer_t ttcn_bi = srsran::make_byte_buffer();
          ttcn_bi->init();
          mac_adp->udp_.mac_receive_info.try_pop(ttcn_bi);
          int roid = ttcn_bi->msg[9];
          // printf("ttcn_bi->msg[9]=%d\n", ttcn_bi->msg[9]);
          *ptr = (uint8_t)(!is_last << 7 | 1 << 6) | (roid >> 1);
          ptr++;
          *ptr = (uint8_t)((roid & 0x01) << 7) | pending_rars1[ra_id].subHead.L;
          return;
        }
      }
    }
    else if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.TC_615_mac_roid_not_match)
    {
      while (true)
      {
        if (mac_adp->udp_.mac_receive_info.size() != 0)
        {
          srsran::unique_byte_buffer_t ttcn_ROID = srsran::make_byte_buffer();
          ttcn_ROID->init();
          mac_adp->udp_.mac_receive_info.try_pop(ttcn_ROID);
          int roid = ttcn_ROID->msg[9];
          printf("ttcn_ROID->msg[9]=%d\n", ttcn_ROID->msg[9]);
          *ptr = (uint8_t)(!is_last << 7 | 1 << 6) | (roid >> 1);
          break;
        }
      }
    }
    else
    {
      *ptr = (uint8_t)(!is_last << 7 | 1 << 6) | (pending_rars1[ra_id].subHead.roid >> 1);
    }
    ptr++;
    if (area_mode == 0)
    {
      //  *ptr = (uint8_t)((pending_rars1[ra_id].subHead.roid & 0x01) << 7) | pending_rars1[ra_id].subHead.normalSychL;
      *ptr = (uint8_t)((pending_rars1[ra_id].subHead.roid & 0x01) << 7) | pending_rars1[ra_id].subHead.L;
    }
    else
    {
      //*ptr = (uint8_t)((pending_rars1[ra_id].subHead.roid & 0x01) << 7) | pending_rars1[ra_id].subHead.ssSychL;
      *ptr = (uint8_t)((pending_rars1[ra_id].subHead.roid & 0x01) << 7) | pending_rars1[ra_id].subHead.ssL;
    }
  }

  void mac::iot_NM_setHead(int bi, int ra_id, uint8_t *ptr, bool is_last)
  {
    if (bi)
    {
      *ptr = (uint8_t)bi & 0x0f;
      return;
    }
    *ptr = (uint8_t)(!is_last << 7 | 1 << 6) | (iot_pending_rars1->subHead.roid >> 3);
    ptr++;
    *ptr = (uint8_t)((iot_pending_rars1->subHead.roid & 0x07) << 5) | iot_pending_rars1->subHead.L;
  }

  void mac::iot_SS_setHead(int bi, int ra_id, uint8_t *ptr, bool is_last)
  {
    if (bi)
    {
      *ptr = (uint8_t)bi & 0x0f;
      return;
    }
    *ptr = (uint8_t)(!is_last << 7 | 1 << 6) | (iot_pending_rars1->subHead.roid >> 3);
    ptr++;
    *ptr = (uint8_t)((iot_pending_rars1->subHead.roid & 0x07) << 5) | iot_pending_rars1->subHead.L;
  }

  void mac::rarReset(int i)
  {
    if (network_mode == 0)
    {
      pending_rars1[i].frameID = -1;
      pending_rars1[i].is_enable = false;
      pending_rars1[i].temp_crnti = -1;
      pending_rars1[i].subHead.is_albe = false;
      pending_rars1[i].considInfo.isEnable = false;
      pending_rars1[i].considInfo.tti = -1;
    }
    else if (network_mode == 1)
    {
      iot_pending_rars1[i].frameID = -1;
      iot_pending_rars1[i].is_enable = false;
      iot_pending_rars1[i].temp_crnti = -1;
      iot_pending_rars1[i].subHead.is_albe = false;
      iot_pending_rars1[i].considInfo.isEnable = false;
      iot_pending_rars1[i].considInfo.tti = -1;
    }
  }

  void mac::iot_NM_generateAgchPdu(uint8_t *pdu, int &pduLen, std::vector<int> raid)
  {
    uint8_t *init_ptr = pdu;
    int n = 0;
    for (int i = 0; i < (int)raid.size(); i++)
    {
      n++;
      iot_NM_setHead(0, raid[i], pdu, n == (int)raid.size());
      pdu = pdu + 2;
    }
    for (int i = 0; i < (int)raid.size(); i++)
    {
      printf("iot_NM_generateAgchPdu\n");
      *pdu = (uint8_t)(iot_pending_rars1[raid[i]].considInfo.consID >> 8) & 0xff;
      pdu++;
      *pdu = (uint8_t)(iot_pending_rars1[raid[i]].considInfo.consID) & 0xff;
      pdu++;
      *pdu = (uint8_t)(iot_pending_rars1[raid[i]].fa << 2) | iot_pending_rars1[raid[i]].ScheduleInfolnd;
      pdu++;
      *pdu = (uint8_t)iot_pending_rars1[raid[i]].temp_crnti;
      pdu++;
      *pdu = (uint8_t)(iot_pending_rars1[raid[i]].DLBandid << 2) | iot_pending_rars1[raid[i]].DLFreqid;
      pdu++;
      *pdu = (uint8_t)iot_pending_rars1[raid[i]].DLScheduleType << 6 | iot_pending_rars1[raid[i]].DLFnAssignment;
      pdu++;
      *pdu = (uint8_t)(iot_pending_rars1[raid[i]].ULFnAssignment << 6) |
             (iot_pending_rars1[raid[i]].DLSceheduleAmount << 3) | iot_pending_rars1[raid[i]].DLScheduleInterval;
      pdu++;
      *pdu = (uint8_t)(iot_pending_rars1[raid[i]].ULBandid << 2) | iot_pending_rars1[raid[i]].ULFreqid;
      pdu++;
      *pdu = (uint8_t)(iot_pending_rars1[raid[i]].ULSubFreqid << 6) | (iot_pending_rars1[raid[i]].ULSchedUleType << 5) |
             (iot_pending_rars1[raid[i]].ULFnAssignment >> 4);
      pdu++;
      *pdu = (uint8_t)(iot_pending_rars1[raid[i]].ULFnAssignment & 0x0f) | iot_pending_rars1[raid[i]].ULSceheduleAmount;
      pdu++;
      *pdu = (uint8_t)(iot_pending_rars1[raid[i]].ULScheduleInterval << 5) | iot_pending_rars1[raid[i]].ULTransmitCopies;
    }
    pduLen = pdu - init_ptr;
  }

  void mac::iot_SS_generateAgchPdu(uint8_t *pdu, int &pduLen, std::vector<int> raid)
  {
    uint8_t *init_ptr = pdu;
    int n = 0;
    for (int i = 0; i < (int)raid.size(); i++)
    {
      n++;
      iot_SS_setHead(0, raid[i], pdu, n == (int)raid.size());
      pdu = pdu + 2;
    }
    for (int i = 0; i < (int)raid.size(); i++)
    {
      printf("iot_SS_generateAgchPdu\n");
      *pdu = (uint8_t)(iot_pending_rars1[raid[i]].considInfo.consID >> 8) & 0xff;
      pdu++;
      *pdu = (uint8_t)(iot_pending_rars1[raid[i]].considInfo.consID) & 0xff;
      pdu++;
      *pdu = (uint8_t)(iot_pending_rars1[raid[i]].fa << 2) | iot_pending_rars1[raid[i]].ScheduleInfolnd;
      pdu++;
      *pdu = (uint8_t)(iot_pending_rars1[raid[i]].temp_crnti % 64);
      pdu++;
      *pdu = (uint8_t)(iot_pending_rars1[raid[i]].FreqSparedFactor << 5) | (iot_pending_rars1[raid[i]].CodeIndex >> 4);
      pdu++;
      *pdu = (uint8_t)(iot_pending_rars1[raid[i]].CodeIndex & 0x0f) | iot_pending_rars1[raid[i]].DLScheduleType |
             (iot_pending_rars1[raid[i]].DLFnAssignment >> 5);
      pdu++;
      *pdu = (uint8_t)(iot_pending_rars1[raid[i]].DLFnAssignment & 0x1f) << 3;
      pdu++;
      *pdu = (uint8_t)(iot_pending_rars1[raid[i]].DLSceheduleAmount << 5) | iot_pending_rars1[raid[i]].DLScheduleInterval;
      pdu++;
      *pdu = (uint8_t)(iot_pending_rars1[raid[i]].ULBandid) | iot_pending_rars1[raid[i]].ULFreqid;
      pdu++;
      *pdu = (uint8_t)(iot_pending_rars1[raid[i]].ULSubFreqid << 6) | iot_pending_rars1[raid[i]].ULSchedUleType << 5 |
             (iot_pending_rars1[raid[i]].ULFnAssignment >> 4);
      pdu++;
      *pdu = (uint8_t)(iot_pending_rars1[raid[i]].ULFnAssignment & 0x0f) | iot_pending_rars1[raid[i]].ULSceheduleAmount;
      pdu++;
      *pdu = (uint8_t)(iot_pending_rars1[raid[i]].ULScheduleInterval << 5) | iot_pending_rars1[raid[i]].ULTransmitCopies;
    }
    pduLen = pdu - init_ptr;
  }

  void mac::SS_generatePsychAgchPdu(uint8_t *pdu, int &pduLen, std::vector<int> raid)
  {
    uint8_t *init_ptr = pdu;
    int n = 0;
    for (int i = 0; i < (int)raid.size(); i++)
    {
      n++;
      setHead(0, raid[i], pdu, n == (int)raid.size());
      pdu = pdu + 2;
    }
    for (int i = 0; i < (int)raid.size(); i++)
    {
      *pdu = (uint8_t)(pending_rars1[raid[i]].considInfo.consID >> 40) & 0xff;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].considInfo.consID >> 32) & 0xff;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].considInfo.consID >> 24) & 0xff;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].considInfo.consID >> 16) & 0xff;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].considInfo.consID >> 8) & 0xff;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].considInfo.consID) & 0xff;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].ta >> 2) & 0xff;
      pdu++;
      *pdu = (uint8_t)((pending_rars1[raid[i]].ta & 0x03) << 6) | pending_rars1[raid[i]].fa;
      pdu++;
      // sych
      *pdu = (uint8_t)(0x00 << 3) | ((pending_rars1[raid[i]].PDTCHType) << 1) |
             ((pending_rars1[raid[i]].PDTCHCodeIndex >> 8) & 0x01);
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].PDTCHCodeIndex) & 0xff;
      pdu++;
      *pdu = (uint8_t)(mac_cfg.agch.freq << 6) | (mac_cfg.agch.BandID);
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].ds_chanType1 & 0x07) << 5 | pending_rars1[raid[i]].ds_chanAss1;
      pdu++;
      // end
      *pdu = (uint8_t)(0x00 << 3) | ((pending_rars1[raid[i]].PDTCHType) << 1) |
             ((pending_rars1[raid[i]].PDTCHCodeIndex >> 8) & 0x01);
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].PDTCHCodeIndex) & 0xff;
      pdu++;
      *pdu = (uint8_t)(mac_cfg.agch.freq << 6) | (mac_cfg.agch.BandID);
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].chanType1 & 0x07) << 5 | pending_rars1[raid[i]].chanAssignment1; // 00000111
      pdu++;

      *pdu = (uint8_t)(pending_rars1[raid[i]].temp_crnti % 64);
      pdu++;
      pending_rars1[raid[i]].is_enable = false;
    }
    pduLen = pdu - init_ptr;
    std::cout << "SS agch pdulen:" << pduLen << std::endl;
  }
  // kuopin
  void mac::SS_generateAgchPdu(uint8_t *pdu, int &pduLen, std::vector<int> raid)
  {
    uint8_t *init_ptr = pdu;
    int n = 0;
    for (int i = 0; i < (int)raid.size(); i++)
    {
      n++;
      setHead(0, raid[i], pdu, n == (int)raid.size());
      pdu = pdu + 2;
    }
    for (int i = 0; i < (int)raid.size(); i++)
    {
      printf("SS_generateAgchPdu\n");
      *pdu = (uint8_t)(pending_rars1[raid[i]].considInfo.consID >> 40) & 0xff;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].considInfo.consID >> 32) & 0xff;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].considInfo.consID >> 24) & 0xff;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].considInfo.consID >> 16) & 0xff;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].considInfo.consID >> 8) & 0xff;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].considInfo.consID) & 0xff;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].ta >> 2) & 0xff;
      pdu++;
      *pdu = (uint8_t)((pending_rars1[raid[i]].ta & 0x03) << 6) | pending_rars1[raid[i]].fa;
      pdu++;
      *pdu = (uint8_t)(0x00 << 3) | ((pending_rars1[raid[i]].PDTCHType) << 1) |
             ((pending_rars1[raid[i]].PDTCHCodeIndex >> 8) & 0x01);
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].PDTCHCodeIndex) & 0xff;
      pdu++;
      *pdu = (uint8_t)(mac_cfg.agch.freq << 6) | (mac_cfg.agch.BandID);
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].chanType1 & 0x07) << 5 | pending_rars1[raid[i]].chanAssignment1; // 00000111
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].temp_crnti % 64);
      pdu++;
      pending_rars1[raid[i]].is_enable = false;
    }
    pduLen = pdu - init_ptr;
    std::cout << "SS agch pdulen:" << pduLen << std::endl;
  }

  // void mac::generatePschPdu(uint8_t* pdu, int& pduLen, std::vector<int> raid)
  // {

  // }
  //2024/11/26--jjc
  void mac::generateHOagchpdu(uint8_t* pdu, int& pduLen)
  {
      uint8_t* init_ptr = pdu;
      printf("generate ho agch pdu, ta:%d\n",handover_info.ta);
      //sethead
      *pdu = (uint8_t)(1<<6)|(handover_info.roid>>1);
      pdu++;
      *pdu=(uint8_t)((handover_info.roid&0x01)<<7)|handover_info.L;
      pdu++;

      //setpdu
      *pdu=(uint8_t)(ue_db[70]->get_handover_para()->lrnti_bandid) & 0xff;
      pdu++;
      *pdu=(uint8_t)((ue_db[70]->get_handover_para()->lrnti_freqid<<6) | (ue_db[70]->get_handover_para()->lrnti_srnti)) & 0xff;
      pdu++;
      *pdu = (uint8_t)(handover_info.ta >> 2) & 0xff;
      pdu++;
      *pdu = (uint8_t)((handover_info.ta & 0x03) << 6) | handover_info.fa;
      pdu++;
    pduLen = pdu - init_ptr;
  }

  // pdch1-1
  void mac::generateAgchPdu(uint8_t *pdu, int &pduLen, std::vector<int> raid)
  {
    uint8_t *init_ptr = pdu;
    if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.TC_614_backoff)
    {
      *pdu = (uint8_t)((1 << 7) | (2 & 0x0f)); // E/T/R/R/BI
      pdu++;
    }
    int n = 0;
    for (int i = 0; i < (int)raid.size(); i++)
    {
      n++;
      setHead(0, raid[i], pdu, n == (int)raid.size());
      pdu = pdu + 2;
    }

    for (int i = 0; i < (int)raid.size(); i++)
    {
      printf("generateAgchPdu\n");
      if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.TC_612_mac_crid_not_match)
      {
        while (true)
        {
          if (mac_adp->udp_.mac_receive_info.size() != 0)
          {
            srsran::unique_byte_buffer_t ttcn_CRID = srsran::make_byte_buffer();
            ttcn_CRID->init();
            pending_rars1[raid[i]].considInfo.consID = (uint64_t)(((uint64_t)ttcn_CRID->msg[8] << 40) |
                                                                  ((uint64_t)ttcn_CRID->msg[9] << 32) | ((uint64_t)ttcn_CRID->msg[10] << 24) |
                                                                  ((uint64_t)ttcn_CRID->msg[11] << 16) | ((uint64_t)ttcn_CRID->msg[12] << 8) | ((uint64_t)ttcn_CRID->msg[13]));
            break;
          }
        }
      }
      pending_rars1[raid[i]].band_id = 9;
      pending_rars1[raid[i]].chanAssignment1 = 8;
      mac_cfg.agch.freq = 1;

      std::cout << "[generateAgchPdu][ue_category]:" << (int)ue_category << std::endl;
      if (ue_category == 14)
      {
        mac_cfg.agch.freq = ue_cap14_freq_id;
        pending_rars1[raid[i]].band_id = ue_cap14_band_id;
        pending_rars1[raid[i]].chanAssignment1 = ue_cap14_slot;
      }

      std::cout << "[generateAgchPdu][BAND_ID]" << pending_rars1[raid[i]].band_id << std::endl;

      std::cout << "[generateAgchPdu][FREQ_ID]" << mac_cfg.agch.freq << std::endl;

      std::cout << "[generateAgchPdu][SLOT]" << pending_rars1[raid[i]].chanAssignment1 << std::endl;

      std::cout << "wwh       pending_rars1[raid[i]].considInfo.consID:" << pending_rars1[raid[i]].considInfo.consID << std::endl;
      *pdu = (uint8_t)(pending_rars1[raid[i]].considInfo.consID >> 40) & 0xff;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].considInfo.consID >> 32) & 0xff;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].considInfo.consID >> 24) & 0xff;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].considInfo.consID >> 16) & 0xff;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].considInfo.consID >> 8) & 0xff;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].considInfo.consID) & 0xff;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].ta >> 2) & 0xff;
      pdu++;
      *pdu = (uint8_t)((pending_rars1[raid[i]].ta & 0x03) << 6) | 0;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].temp_crnti % 64);
      printf("pending_rars1[raid[i]].temp_crnti=0x%x\n", pending_rars1[raid[i]].temp_crnti);
      pdu++;
      *pdu = (uint8_t)(mac_cfg.agch.freq << 6) | (pending_rars1[raid[i]].band_id);
      printf("mac_cfg.agch.freq=0x%x   ending_rars1[raid[i]].band_id=0x%x\n", mac_cfg.agch.freq, pending_rars1[raid[i]].band_id);
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].chanType1 & 0x07) << 5 | pending_rars1[raid[i]].chanAssignment1;
      printf("pending_rars1[raid[i]].chanType1=0x%x  pending_rars1[raid[i]].chanAssignment1=0x%x\n", pending_rars1[raid[i]].chanType1, pending_rars1[raid[i]].chanAssignment1);
      pdu++;
      pending_rars1[raid[i]].is_enable = false;
    }
    printf("*************agchpdu*****3/22****\n");

    pduLen = pdu - init_ptr;
    std::cout << " AGCH Len = " << pduLen << std::endl;
  }

  // psch
  //  void mac::generatePschPdu(uint8_t* pdu, int& pdulen) {
  //    //pui and extend pui
  //    *pdu = (uint8_t)(psch_pdu.pui.s_rnti << 2) | (psch_pdu.pui.MCS >> 1);
  //    pdu++;
  //    if (psch_pdu.pui.EI == 1) {//有extend pui
  //      *pdu = (uint8_t)((psch_pdu.pui.MCS & 0x01) << 7) | (psch_pdu.pui.L << 6) | (psch_pdu.pui.EI << 5) |
  //             (psch_pdu.extend_pui.s_rnti1 >> 2);
  //      pdu++;
  //      *pdu = (uint8_t)((psch_pdu.extend_pui.s_rnti1 & 0x03) << 6) | psch_pdu.extend_pui.s_rnti2;
  //      pdu++;
  //      // head
  //      *pdu = (uint8_t)((psch_pdu.head.L & 0x01) << 7) | (psch_pdu.head.F << 6) | (psch_pdu.head.LCID);
  //      pdu++;
  //      *pdu = (uint8_t)(psch_pdu.head.L);
  //      pdu++;
  //      // payload and padding
  //      memcpy(pdu, psch_pdu.msg, pdulen - 5); // pui、extend pui和头占五个字节
  //    }
  //    if (psch_pdu.pui.EI == 0) {//无extend pui
  //      *pdu = (uint8_t)((psch_pdu.pui.MCS & 0x01) << 7) | (psch_pdu.pui.L << 6) | (psch_pdu.pui.EI << 5);
  //      pdu++;
  //      // head
  //      *pdu = (uint8_t)((psch_pdu.head.L & 0x01) << 7) | (psch_pdu.head.F << 6) | (psch_pdu.head.LCID);
  //      pdu++;
  //      *pdu = (uint8_t)(psch_pdu.head.L);
  //      pdu++;
  //      // payload and padding
  //      memcpy(pdu, psch_pdu.msg, pdulen - 4); // pui和头占4个字节
  //    }
  //  }

  // PSYCH
  void mac::generatePsychAgchPdu(uint8_t *pdu, int &pduLen, std::vector<int> raid)
  {
    uint8_t *init_ptr = pdu;
    int n = 0;
    for (int i = 0; i < (int)raid.size(); i++)
    {
      n++;
      setHead(0, raid[i], pdu, n == (int)raid.size());
      pdu = pdu + 2;
    }

    for (int i = 0; i < (int)raid.size(); i++)
    {
      std::cout << "pending_rars1[raid[i]].ta:" << pending_rars1[raid[i]].ta << std::endl;
      *pdu = (uint8_t)(pending_rars1[raid[i]].considInfo.consID >> 40) & 0xff;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].considInfo.consID >> 32) & 0xff;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].considInfo.consID >> 24) & 0xff;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].considInfo.consID >> 16) & 0xff;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].considInfo.consID >> 8) & 0xff;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].considInfo.consID) & 0xff;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].ta >> 2) & 0xff;
      pdu++;
      *pdu = (uint8_t)((pending_rars1[raid[i]].ta & 0x03) << 6) | pending_rars1[raid[i]].fa;
      pdu++;
      // psych
      *pdu = (uint8_t)(pending_rars1[raid[i]].temp_crnti % 64);
      pdu++;
      *pdu = (uint8_t)(mac_cfg.agch.freq << 6) | 8;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].chanType3 & 0x07) << 5 | pending_rars1[raid[i]].chanAssignment3;
      pdu++;
      // pdch1
      *pdu = (uint8_t)(pending_rars1[raid[i]].temp_crnti % 64);
      pdu++;
      *pdu = (uint8_t)(mac_cfg.agch.freq << 6) | (pending_rars1[raid[i]].band_id);
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].chanType1 & 0x07) << 5 | pending_rars1[raid[i]].chanAssignment1;
      pdu++;
      pending_rars1[raid[i]].is_enable = false;
    }
    pduLen = pdu - init_ptr;
  }

  // void mac::generatePsychPdu(uint8_t* pdu)
  // {
  //    *pdu=(0<<6)|(SYCH_LCID);
  //    pdu++;
  //    *pdu=0;
  //    pdu++;
  //    *pdu=0;
  //    pdu++;
  // }
  void mac::generatePsychPdu(uint8_t *pdu)
  {
    if (sych_ce.ta > time_toleranceMax)
    {
      TAhashMap[sych_ce.ta] = TAhashMap[time_toleranceMax];
    }
    if (sych_ce.ta < time_toleranceMin)
    {
      TAhashMap[sych_ce.ta] = TAhashMap[time_toleranceMin];
    }
    if (sych_ce.fa > freq_toleranceMax)
    {
      FAhashMap[sych_ce.fa] = FAhashMap[freq_toleranceMax];
    }
    if (sych_ce.fa < freq_toleranceMin)
    {
      FAhashMap[sych_ce.fa] = FAhashMap[freq_toleranceMin];
    }
    std::cout << "psychpdu SS: " << (int)psychSS << std::endl;
    // if(area_mode==1){
    //   *pdu=1<<7|31;
    //   pdu++;
    // }
    uint8_t *xxx = pdu;
    *pdu = (psychSS << 6) | (SYCH_LCID);
    pdu++;
    *pdu = TAhashMap[sych_ce.ta];
    //  std::cout <<"sychPdu FAhashMap:"<<FAhashMap[sych_ce.fa]<<std::endl;
    //       std::cout <<"TAhashMap:"<<TAhashMap[sych_ce.ta]<<std::endl;
    //       std::cout <<"PAmap:"<<PAmap[sych_ce.pa]<<std::endl;
    pdu++;
    // printf("@@@@@@psych0x");
    // for (int i = 0; i < 2; i++)
    // {
    //   printf("@@@@@@psych0x:%x\n", *(xxx + i));
    // }

    *pdu = (FAhashMap[sych_ce.fa] << 3) | PAmap[sych_ce.pa];
    pdu++;
  }

  void mac::generatePsychZeroPdu(uint8_t *pdu)
  {
    std::cout << "psychpdu SS: " << (int)psychSS << std::endl;
    //  if(area_mode==0){
    //     *pdu=1<<7|31;
    //     pdu++;
    //   }
    *pdu = (psychSS << 6) | (SYCH_LCID);
    pdu++;
    *pdu = 127;
    //  std::cout <<"Zero FAhashMap:"<<FAhashMap[sych_ce.fa]<<std::endl;
    //       std::cout <<"TAhashMap:"<<TAhashMap[sych_ce.ta]<<std::endl;
    //       std::cout <<"PAmap:"<<PAmap[sych_ce.pa]<<std::endl;
    pdu++;
    *pdu = 0;
    pdu++;
  }

  void mac::sych_update()
  {
    int n = 0;

    for (float i = -15.875; i <= 15.875; i += 0.125)
    {
      float a = (float)(int)(i * 1000) / 1000;
      TAhashMap[i] = n;
      n++;
    }
    int m = 0, k = 17;
    for (int j = 0; j <= 2400; j += 160)
    {
      FAhashMap[j] = m;
      m++;
    }
    for (int l = -160; l >= -2400; l -= 160)
    {
      FAhashMap[l] = k;
      k++;
    }
    if (sych_ce.ta > time_toleranceMax)
    {
      TAhashMap[sych_ce.ta] = TAhashMap[time_toleranceMax];
    }
    if (sych_ce.ta < time_toleranceMin)
    {
      TAhashMap[sych_ce.ta] = TAhashMap[time_toleranceMin];
    }
    if (sych_ce.fa > freq_toleranceMax)
    {
      FAhashMap[sych_ce.fa] = FAhashMap[freq_toleranceMax];
    }
    if (sych_ce.fa < freq_toleranceMin)
    {
      FAhashMap[sych_ce.fa] = FAhashMap[freq_toleranceMin];
    }
    // std::cout <<"PA:"<<sych_ce.pa<<std::endl;
    // std::cout <<"TA:"<<sych_ce.ta<<std::endl;
    // std::cout <<"FA:"<<sych_ce.fa<<std::endl;
    // std::cout <<"FAhashMap:"<<FAhashMap[sych_ce.fa]<<std::endl;
    // std::cout <<"TAhashMap:"<<TAhashMap[sych_ce.ta]<<std::endl;
    // std::cout <<"PAmap:"<<PAmap[sych_ce.pa]<<std::endl;
  }

  void mac::getsych()
  {
    std::cout << "getsych  fa :" << sych_ce.fa << std::endl;
    std::cout << "getsych  pa " << sych_ce.pa << std::endl;
    std::cout << "getsychp ta:" << sych_ce.ta << std::endl;
    if (wxScheduler.interval < wxScheduler.Tsync)
    {
      if (sych_ce.ta < data_time_toleranceMax && sych_ce.ta > data_time_toleranceMin && sych_ce.fa < freq_toleranceMax && sych_ce.fa > freq_toleranceMin)
      {
        std::cout << "在容差范围内" << std::endl;
        psychSS = 1;
      }
      else
      {
        std::cout << "bu在容差范围内" << std::endl;
        psychSS = 0;
      }
    }
    else
    {
      psychSS = 3;
      wxScheduler.deallocate_source(dlAlloc);
    }
  }

  void mac::update_ChanType(ChanType_t dl_Type_, ChanType_t ul_Type_)
  {
    if (area_mode == 0)
    {
      dlAlloc.Type = dl_Type_; // for mode 0,ul and dl common
      std::cout << "dlAlloc.Type:" << dlAlloc.Type << std::endl;
      setupAllocate(dlAlloc);
    }
    else if (area_mode == 1)
    {
      dlAlloc.Type = dl_Type_;
      ulAlloc_.Type = ul_Type_;
      setAllocate(dlAlloc);
      setulAlloc(ulAlloc_);
    }
  }

  void mac::rlcStateReport(int lcid, int rnti)
  {
    if (!rlcReport.empty())
      rlcReport.pop_back();
    rlcState temp;
    temp.lcid = lcid;
    temp.rnti = rnti;
    rlcReport.push_back(temp);
    // rlcReport.front().lcid=lcid;
    // std::cout << " rlcReport.size()" << rlcReport.size() << std::endl;
    wxScheduler.allocate_source(dlAlloc);
  }
  void mac::getSch1(int &len, int &rnti, int slot)
  {
    std::cout << "[GET SCH1]" << std::endl;
    if (ue_db.empty())
    {
      std::cout << " check ue error " << std::endl;
      return;
    }
#if 1
    std::cout << "[GET SCH1][MCS]:" << MAC_mcs_Info.MCS << std::endl;
    std::cout << "[GET SCH1][temp_MCS]:" << temp_MCS << std::endl;
    std::cout << "[GET SCH1][is_data_service_switching]:" << MAC_mcs_Info.is_data_service_switching << std::endl;
    psch_pdu.pui.s_rnti = s_rnti_;
    std::cout << "[psch_pdu][pui][s_rnti]:" << (int)psch_pdu.pui.s_rnti << std::endl;
    if (!mac_lcid_to_resource.empty())
    {
      Pading_Map = mac_lcid_to_resource.begin()->second; // keyouhua
      std::cout << " Pading_Map Type = " << (int)Pading_Map.ul_Type << std::endl;

      for (auto &it : mac_lcid_to_resource)
      {
        if (it.second.ul_Type != pdch1_1 && it.second.ul_Type != pdch1_2)
        {
          if (MAC_mcs_Info.MCS != temp_MCS && MAC_mcs_Info.is_data_service_switching == false)
          {
            temp_MCS = MAC_mcs_Info.MCS;
            std::cout << "temp_MCS:" << temp_MCS << std::endl;
            std::cout << "[SEND][MCS][INFO]" << std::endl;
            len = ue_db[rlcReport.front().rnti]->generate_mcs_ce(rlcReport.front().lcid, rlcReport.front().msg);
            // sched_result.sch1
            rlcReport.front().lcid = 1; // MAC 在LCID = 1的地方发
            sched_result.sch1[slot].lcid = 1;
            memcpy(sched_result.sch1[slot].msg, rlcReport.front().msg, len);
            // MCS = -1;
            std::cout << "[GET SCH][IS MCS SENT][BEFORE]:" << ue_db[70]->mcs_adjust.is_mcs_sent << std::endl;
            ue_db[70]->mcs_adjust.is_mcs_sent = true; // 区分组包头
            std::cout << "[GET SCH][IS MCS SENT][AFTER]:" << ue_db[70]->mcs_adjust.is_mcs_sent << std::endl;
            break;
          }
          Current_Map = it.second;
          update_ChanType(it.second.dl_Type, it.second.ul_Type);
          bzero(it.second.msg, 400);
          int temp_len = 0;
          if (it.second.dataType == Control || it.second.dataType == Data)
          {
            temp_len = dlAlloc.pduLen - 2;
          }
          std::cout << " read rlc Lcid = " << it.first << std::endl;
          std::cout << " read rlc temp_len = " << temp_len << std::endl;
          len = ue_db[70]->read_pdu(it.first, it.second.msg, temp_len);
          std::cout << " read rlc len = " << len << std::endl;
          if (len > 0)
          {
            is_read_data_in_map = true;
            std::cout << "LEN > 0 ; LCID = " << it.first << std::endl;
            rlcReport.front().lcid = it.first;
            sched_result.sch1[slot].lcid = it.first;
            memcpy(rlcReport.front().msg, it.second.msg, len);
            memcpy(sched_result.sch1[slot].msg, it.second.msg, len);
            break;
          }
        }
      }
      if (is_read_data_in_map == false)
      {
        Current_Map = Pading_Map;
      }
      else
      {
        is_read_data_in_map = false;
      }

      update_ChanType(Current_Map.dl_Type, Current_Map.ul_Type);
      // std::cout << "[rlcReport.front().lcid]=" << rlcReport.front().lcid << std::endl;
      rlcReport.front().len = len;
      // std::cout << "rlcReport.front().rnti" << rlcReport.front().rnti << " " << mac_rnti << std::endl;
      // for (int i = 0; i < len; i++)
      // {
      //   printf("rlcReport.front().msg:%x\n", *(rlcReport.front().msg + i));
      // }
      std::cout << "dlAlloc.type>" << dlAlloc.Type << std::endl;
      std::cout << "get sch1 len>" << len << std::endl;
      std::cout << "rlcReport.front().len = " << rlcReport.front().len << std::endl;
      rnti = rlcReport.front().rnti;
    }
#endif

#if 0
     if(!rlcReport.empty()){
         bzero(rlcReport.front().msg,400);
          //rlcReport.front().lcid=1;
         len= ue_db[rlcReport.front().rnti]->read_pdu(rlcReport.front().lcid,rlcReport.front().msg,dlAlloc.pduLen-2);
        //  if(len==0)
        //  {
        //   len=ue_db[rlcReport.front().rnti]->generate_mcs_ce(rlcReport.front().lcid,rlcReport.front().msg);
        //  }
        rlcReport.front().len=len;
        rnti=rlcReport.front().rnti;
        psch_pdu.pui.s_rnti=rnti%64;
        std::cout<<"-----------getsch1---------"<<std::endl;
        // for(int i=0;i<dlAlloc.pduLen-2;i++)
        // {
        //   printf("0x%x--\n",*(rlcReport.front().msg+i));
        // }
        std::cout<<"getSch1"<<rnti<<" "<<len<<std::endl;
     }
#endif
    else
    {
      len = -1;
      printf("do not receive \n");
    }
    std::cout << "sch1Len:" << len << std::endl;
  }

  void mac::getSch2(int &len, int &rnti)
  {
    std::cout << "[GET SCH2]" << std::endl;
    logger.info("[ZHJ][GET SCH 2]");
    T3 = std::chrono::high_resolution_clock::now();
    srsran::rwlock_read_guard lock(rwlock);
    T3_A = std::chrono::high_resolution_clock::now();
    if (ue_db.empty())
    {
      std::cout << " check ue error " << std::endl;
      return;
    }
    std::cout << "MAP Size = " << mac_lcid_to_resource.size() << std::endl;
    T3_B = std::chrono::high_resolution_clock::now();
    if (!mac_lcid_to_resource.empty())
    {
      if (ue_category == 14 && mac_lcid_to_resource.size() < 2)
      {
        std::cout << "ue_category == 14 && mac_lcid_to_resource.size() == 1" << std::endl;
        for (auto &it : mac_lcid_to_resource)
        {
          int templen = 4;
          len = ue_db[70]->read_pdu(it.first, it.second.msg, templen);
          if (len > 0)
          {
            rlcReport.front().lcid = it.first;
            memcpy(rlcReport.front().msg, it.second.msg, len);
            break;
          }
        }
        rlcReport.front().len = len;
        rnti = rlcReport.front().rnti;
      }
      else
      {
        Pading_Map = mac_lcid_to_resource.begin()->second; // keyouhua
        for (auto &it : mac_lcid_to_resource)
        {
          if (MAC_mcs_Info.MCS != temp_MCS && MAC_mcs_Info.is_data_service_switching == false)
          {
            temp_MCS = MAC_mcs_Info.MCS;
            std::cout << "temp_MCS:" << temp_MCS << std::endl;
            std::cout << "[SEND][MCS][INFO]" << std::endl;
            len = ue_db[rlcReport.front().rnti]->generate_mcs_ce(rlcReport.front().lcid, rlcReport.front().msg);
            rlcReport.front().lcid = 1; // MAC 在LCID = 1的地方发
            // MCS = -1;
            std::cout << "[GET SCH][IS MCS SENT][BEFORE]:" << ue_db[70]->mcs_adjust.is_mcs_sent << std::endl;
            ue_db[70]->mcs_adjust.is_mcs_sent = true; // 区分组包头
            std::cout << "[GET SCH][IS MCS SENT][AFTER]:" << ue_db[70]->mcs_adjust.is_mcs_sent << std::endl;
            break;
          }
          if (it.first == 5)
          {
            printf("voice Type = %d", it.second.voicetype);
          }

          Current_Map = it.second;
          update_ChanType(it.second.dl_Type, it.second.ul_Type);
          bzero(it.second.msg, 400);
          int temp_len = 0;
          if (it.second.dataType == Control || it.second.dataType == Data)
          { // Have MAC head and sacch
            // Determining the channel type is handled by RRC
            if (mac_adp->udp_.mac_receive_info.size() != 0 && mac_adp->udp_.TC_6118_mac_ce_handover)
            {
              temp_len = dlAlloc.pduLen - 5;
            }
            else
            {
              temp_len = dlAlloc.pduLen - 3;
            }
          }
          else if (it.second.dataType == Voice)
          { // only have sacch
            temp_len = dlAlloc.pduLen - 1;
          }
          // std::cout << " read rlc Lcid = " << it.first << std::endl;
          // std::cout << " read rlc temp_len = " << temp_len << std::endl;
          len = ue_db[70]->read_pdu(it.first, it.second.msg, temp_len);
          // std::cout << " read rlc len = " << len << std::endl;
          if (len > 0)
          {
            is_read_data_in_map = true;
            // std::cout << "LEN > 0 ; LCID = " << it.first << std::endl;
            rlcReport.front().lcid = it.first;
            memcpy(rlcReport.front().msg, it.second.msg, len);

            break;
          }
        }
        if (is_read_data_in_map == false)
        {
          // std::cout << " zhj kong bao " << std::endl;
          Current_Map = Pading_Map;
        }
        else
        {
          is_read_data_in_map = false;
        }

        update_ChanType(Current_Map.dl_Type, Current_Map.ul_Type);

        rlcReport.front().len = len;
        // std::cout << "rlcReport.front().rnti" << rlcReport.front().rnti << " " << mac_rnti << std::endl;
        // for (int i = 0; i < len; i++)
        // {
        //   printf("rlcReport.front().msg:%x\n", *(rlcReport.front().msg + i));
        // }
        // std::cout << "dlAlloc.type>" << dlAlloc.Type << std::endl;
        // std::cout << "get sch2 len>" << len << std::endl;
        // std::cout << "rlcReport.front().len = " << rlcReport.front().len << std::endl;
        rnti = rlcReport.front().rnti;
        // std::cout << "getSch2" << std::endl;
      }
    }
    else
    {
      len = -1;
      printf("do not receive rlc report\n");
    }
    T3_C = std::chrono::high_resolution_clock::now();
    auto T3_Duration_A = std::chrono::duration_cast<std::chrono::microseconds>(T3_A - T3);
    auto T3_Duration_B = std::chrono::duration_cast<std::chrono::microseconds>(T3_B - T3);
    auto T3_Duration_C = std::chrono::duration_cast<std::chrono::microseconds>(T3_C - T3);
    // printf("[GET_SCHED][%d][DL_SCHED][TIME]:%lu\n", duration.count());
    if (T3_Duration_A.count() > 1000)
    {
      printf("[GETSCH2][OVER][1000]T3_Duration_A");
      printf("[GETSCH2][OVER][1000][TIME]T3_Duration_A:%lu\n", T3_Duration_A.count());
    }
    if (T3_Duration_B.count() > 1000)
    {
      printf("[GETSCH2][OVER][1000]T3_Duration_B");
      printf("[GETSCH2][OVER][1000][TIME]T3_Duration_B:%lu\n", T3_Duration_B.count());
    }
    if (T3_Duration_C.count() > 1000)
    {
      printf("[GETSCH2][OVER][1000]T3_Duration_C");
      printf("[GETSCH2][OVER][1000][TIME]T3_Duration_C:%lu\n", T3_Duration_C.count());
    }

    std::cout << "sch2Len:" << len << std::endl;
  }

  void mac::transPUI(uint8_t *pdu, uint16_t &pui)
  {
    uint8_t *init_p = pdu;
    // std::cout << "------transPUI------address:" << &init_p << std::endl;

    uint16_t srnti = 0;
    // std::cout << "mac_adp->udp_.TC_6115_mac_srnti_match=" << mac_adp->udp_.TC_6115_mac_srnti_match << std::endl;
    // std::cout << "mac_adp->udp_.TTCN_Srnti=" << mac_adp->udp_.TTCN_Srnti << std::endl;
    std::cout << "" << std::endl;

    //*********TC618 start*******
    if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.TC_618_mac_regular_pdu_2)
    { 
      srnti = mac_adp->udp_.TTCN_Srnti << 6;
      printf("ttcn_SRNTI=%d\n", srnti);
      mac_adp->udp_.TTCN_Srnti = 0;
      uint8_t mcs = psch_pdu.pui.MCS << 3;
      uint8_t L = 0 << 2;
      uint8_t EI = 0;  //无扩展PUI
      uint8_t R = 0;
      pui = srnti | mcs | L | EI | R;
      *pdu=pui>>4;     //PUI的左8bit  
      pdu++;
    }
    if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.TC_618_mac_regular_pdu)
    { 
      mac_adp->udp_.TC_618_mac_regular_pdu = false;
      srnti = mac_adp->udp_.TTCN_Srnti << 6;
      printf("ttcn_SRNTI=%d\n", srnti);
      mac_adp->udp_.TTCN_Srnti = 0;
      uint8_t mcs = psch_pdu.pui.MCS << 3;
      uint8_t L = 1 << 2;
      uint8_t EI = 1;  //有扩展PUI
      uint8_t R = 0;
      pui = srnti | mcs | L | EI | R;
      *pdu=pui>>4;
      pdu++;
      *pdu=(pui&0xf<<4) | (srnti&0x3c);    //扩展PUI，第一个SRNTI与终端相符
      pdu++;
      *pdu=(srnti&0x03<<6) | 3;  //3d代表与终端不匹配的第二个srnti
      pdu++;
      mac_adp->udp_.TC_618_mac_regular_pdu_2 = true;
    }
    //*********TC618 end*******

    //*********TC619 start*******
    if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.TC_619_mac_bsr_timer_2)
    { 
      mac_adp->udp_.TC_619_mac_bsr_timer_2 = false;
      srnti = mac_adp->udp_.TTCN_Srnti << 6;
      printf("ttcn_SRNTI=%d\n", srnti);
      mac_adp->udp_.TTCN_Srnti = 0;
      uint8_t mcs = psch_pdu.pui.MCS << 3;
      uint8_t L = 0 << 2;
      uint8_t EI = 1;  //有扩展PUI
      uint8_t R = 0;
      pui = srnti | mcs | L | EI | R;
      *pdu=pui>>4;
      pdu++;
      srnti = 4;  //修改SRNTI，扩展PUI里面两个SRNTI都不符合
      *pdu=(pui&0xf<<4) | (srnti&0x3c);    //扩展PUI
      pdu++;
      *pdu=(srnti&0x03<<6) | 3;  //3d代表与终端不匹配的第二个srnti
      pdu++;
    }
    if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.TC_619_mac_bsr_timer)
    { 
      mac_adp->udp_.TC_619_mac_bsr_timer = false;
      srnti = mac_adp->udp_.TTCN_Srnti << 6;
      printf("ttcn_SRNTI=%d\n", srnti);
      mac_adp->udp_.TTCN_Srnti = 0;
      uint8_t mcs = psch_pdu.pui.MCS << 3;
      uint8_t L = 1 << 2;
      uint8_t EI = 1;  //有扩展PUI
      uint8_t R = 0;
      pui = srnti | mcs | L | EI | R;
      *pdu=pui>>4;
      pdu++;
      srnti = 2;  //修改SRNI值，扩展PUI里面的两个SRNTI都不匹配
      *pdu=(pui&0xf<<4) | (srnti&0x3c);    //扩展PUI
      pdu++;
      *pdu=(srnti&0x03<<6) | 3;  //3代表与终端不匹配的第二个srnti
      pdu++;
      mac_adp->udp_.TC_619_mac_bsr_timer_2 = true;
    }
    //*********TC619 end******* 

    if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.TC_6115_mac_srnti_match && mac_adp->udp_.TTCN_Srnti != 0)
    { // jjc
      std::cout << "@@@@@@@@@ xxxxxxxxx" << std::endl;
      mac_adp->udp_.TC_6115_Info_To_TTCN = true;
      srnti = mac_adp->udp_.TTCN_Srnti << 6;
      printf("ttcn_SRNTI=%d\n", srnti);
      mac_adp->udp_.TTCN_Srnti = 0;
      // while (true)
      // {
      //   if (mac_adp->udp_.mac_receive_info.size() != 0)
      //   {
      //     srsran::unique_byte_buffer_t ttcn_SRNTI = srsran::make_byte_buffer();
      //     ttcn_SRNTI->init();
      //     mac_adp->udp_.mac_receive_info.try_pop(ttcn_SRNTI);
      //     srnti = ttcn_SRNTI->msg[9] << 6;
      //     printf("ttcn_SRNTI->msg[9]=%d\n", ttcn_SRNTI->msg[9]);
      //     // mac_adp->udp_.TC_616_mac_push_pdu = true;
      //     break;
      //   }
      // }
      // mac_adp->udp_.TC_6115_mac_srnti_match = false;
    }
    else
    {
      srnti = psch_pdu.pui.s_rnti << 6;
    }

    // uint16_t srnti = psch_pdu.pui.s_rnti << 6;
    // std::cout << "jcjcjcjpsch_pdu.pui.s_rnti:" << (int)psch_pdu.pui.s_rnti << std::endl;
    // std::cout << "jjcjcjcjcjcjsrnti:" << srnti << std::endl;
    uint8_t mcs = psch_pdu.pui.MCS << 3;
    uint8_t L = psch_pdu.pui.L << 2;
    uint8_t EI = 0;
    uint8_t R = 0;
    pui = srnti | mcs | L | EI | R;
    // std::cout << "xx pui=" << (int)pui << std::endl;
    // printf("--pui:0x%x\n",pui);
    uint8_t *init_p1 = pdu;
    // std::cout<<"------transPUI------address:"<<&init_p1<<std::endl;
  }
  void mac::generateSch1(uint8_t *pdu, uint16_t &pui, int s)
  { // psch
    //   uint16_t srnti=psch_pdu.pui.s_rnti<<6;
    //   uint8_t mcs=psch_pdu.pui.MCS<<3;
    //   uint8_t L=psch_pdu.pui.L<<2;
    //   uint8_t EI=0;
    //   uint8_t R=0;
    //   pui=srnti|mcs|L|EI|R;
    //  // printf("--pui:0x%x\n",pui);
    //  *pdu=pui>>4;
    //  pdu++;
    //  *pdu=pui&0xf<<4;
    //  pdu++;
    std::cout << "[GET SCH1][IS MCS SENT][GENERATE SCH1][IS MCS SENT]:" << ue_db[70]->mcs_adjust.is_mcs_sent << std::endl;
    if (ue_db[70]->mcs_adjust.is_mcs_sent)
    {
      ue_db[70]->mcs_adjust.is_mcs_sent = false;
      uint8_t *init_ptr = pdu;

      *pdu = 0x02; // R|E|LCID  当前E=0,LCID=2
      pdu++;
      // memcpy(pdu, rlcReport.front().msg, rlcReport.front().len);
      memcpy(pdu, sched_result.sch1[s].msg, sched_result.sch1[s].len);
      // std::cout << "xx MAC assemble info:" << std::endl;
      // for (int i = 0; i < 15; i++)
      // {
      //   printf("0x:%x\n", *(init_ptr + i));
      // }
      std::cout << "[GET SCH][IS MCS SENT][GENERATE SCH1][IS MCS SENT][END]:" << ue_db[70]->mcs_adjust.is_mcs_sent << std::endl;
    }

    //*********TC61119 start*******
    else if(mac_adp->udp_.mac_receive_info.size() != 0 && mac_adp->udp_.TC_6119_ul_mcs)
    {
      *pdu = 1 >> 5 | 2;  //MCS MAC CE子头
      pdu++;
      *pdu = 1 << 6 | sched_result.sch1[s].lcid;   //sch子头  F/E/LCID
      pdu++;
      *pdu = sched_result.sch1[s].len;   //L域
      pdu++;      

      *pdu = 3 >> 5 | 1 >> 2;   // MCS MAC CE ,MCS字段填的3，FN MCS填的1，不知道这样填写是否可行
      pdu++;
      memcpy(pdu, sched_result.sch1[s].msg, sched_result.sch1[s].len);

      mac_adp->udp_.TC_6119_ul_mcs = false;
      mac_adp->udp_.TC_6119_ul_mcs_2 = true;
    }
    //*********TC61119 end*******

    else
    {
      uint8_t *init_ptr = pdu;
      // std::cout<<"------generateSch1------address:"<<&init_ptr<<std::endl;
      *pdu = 1 << 6 | sched_result.sch1[s].lcid;
      pdu++;
      *pdu = sched_result.sch1[s].len;
      pdu++;
      // memcpy(pdu, rlcReport.front().msg, rlcReport.front().len);
      memcpy(pdu, sched_result.sch1[s].msg, sched_result.sch1[s].len);
      // std::cout << "xx MAC assemble info" << std::endl;
      // for (int i = 0; i < 15; i++)
      // {
      //   printf("0x:%x\n", *(init_ptr + i));
      // }
    }
  }

  void mac::generateSch2(uint8_t *pdu, int &pduLen)
  { // pdch

    std::cout << "[generateSch2]ue_db[70]->mcs_adjust.is_mcs_sent:-----" << ue_db[70]->mcs_adjust.is_mcs_sent << std::endl;
    if (ue_db[70]->mcs_adjust.is_mcs_sent)
    {
      ue_db[70]->mcs_adjust.is_mcs_sent = false;
      uint8_t *init_ptr = pdu;
      *pdu = 1 << 7 | 31; // sacch
      pdu++;
      *pdu = 0x02; // R|E|LCID  当前E=0,LCID=2
      pdu++;
      memcpy(pdu, rlcReport.front().msg, rlcReport.front().len);
      // std::cout << "xx MAC assemble info" << std::endl;
      // for (int i = 0; i < 15; i++)
      // {
      //   printf("0x:%x\n", *(init_ptr + i));
      // }
      std::cout << "[GET SCH][IS MCS SENT][GENERATE SCH1][IS MCS SENT][END]:" << ue_db[70]->mcs_adjust.is_mcs_sent << std::endl;
    }
    else
    {
      if (mac_adp->udp_.mac_receive_info.size() != 0 && mac_adp->udp_.TC_6118_mac_ce_handover)
      {
        uint8_t *init_ptr = pdu;
        *pdu = 31; // sacch
        pdu++;
        *pdu = (uint8_t)((1 << 5) | 12); // R/R/E/LCID handover mac ce sub header
        pdu++;
        *pdu = 0; // bandnum/R
        pdu++;
        *pdu = (uint8_t)((1 << 6) | rlcReport.front().lcid);
        pdu++;
        *pdu = rlcReport.front().len;
        pdu++;
        printf("receive mac ce ho command msg\n");

        if (mac_adp->udp_.mac_receive_info.size() != 0)
        {
          srsran::unique_byte_buffer_t ttcn_macce_command = srsran::make_byte_buffer();
          ttcn_macce_command->init();
          mac_adp->udp_.mac_receive_info.try_pop(ttcn_macce_command);

          *pdu = ttcn_macce_command->msg[8];
          pdu++;
          *pdu = ttcn_macce_command->msg[9];
          pdu++;
          *pdu = ttcn_macce_command->msg[10];
          pdu++;
          *pdu = ttcn_macce_command->msg[11];
          pdu++;
          *pdu = ttcn_macce_command->msg[12];
          pdu++;
        }
        memcpy(pdu, rlcReport.front().msg, rlcReport.front().len);
        mac_adp->udp_.TC_6118_mac_push_pdu = true;
        mac_adp->udp_.TC_6118_mac_ce_handover = false;
      }
      else
      {
        uint8_t *init_ptr = pdu;
        std::cout << "voiceLcid:" << voiceLcid << std::endl;
        if (Current_Map.dataType == Voice)
        {
          *pdu = 31;
          pdu++;
          memcpy(pdu, rlcReport.front().msg, rlcReport.front().len);
        }
        else
        {
          std::cout << "mac_adp->udp_.TC_6116_closed_loop_power_control=" << mac_adp->udp_.TC_6116_closed_loop_power_control << std::endl;
          std::cout << "mac_adp->udp_.TTCN_sacch=" << (int)mac_adp->udp_.TTCN_sacch << std::endl;
          std::cout << "" << std::endl;
          if (area_mode == 0)
          {
            if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.TC_6116_closed_loop_power_control && mac_adp->udp_.TTCN_sacch != 0)
            {
              std::cout << "@@@@@@@@@ PAPAPAPAxxxxxxxxx" << std::endl;
              mac_adp->udp_.pa_adjust_before = sych_ce.pa; // 记录调整前的pa
              mac_adp->udp_.TC_6116_Info_To_TTCN = true;
              *pdu = mac_adp->udp_.TTCN_sacch; // sacch
              printf("TTCN_sacch=%d\n", mac_adp->udp_.TTCN_sacch);
              mac_adp->udp_.TTCN_sacch = 0;
            }
            else if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.TC_6117_ul_tf_sync && mac_adp->udp_.TTCN_sacch != 0)
            {
              std::cout << "@@@@@@@@@ FATAFATAxxxxxxxxx" << std::endl;
              mac_adp->udp_.fa_adjust_before = sych_ce.fa; // 记录调整前的fa
              mac_adp->udp_.ta_adjust_before = sych_ce.ta; // 记录调整前的ta
              printf("ta fa adjust:%f--%d", sych_ce.ta, sych_ce.fa);
              mac_adp->udp_.TC_6117_Info_To_TTCN = true;
              *pdu = mac_adp->udp_.TTCN_sacch; // sacch
              printf("TTCN_sacch=%d\n", mac_adp->udp_.TTCN_sacch);
              mac_adp->udp_.TTCN_sacch = 0;
            }
            else
            {
              *pdu = 31;
            }
            // *pdu=dataTAhashMap[sych_ce.ta];//sacch ta
            //*pdu = 31;
            // *pdu=0x40|FAhashMap[sych_ce.fa];//sacch fa
            // *pdu=0x60|PAmap[sych_ce.pa];//sacch pa
            // std::cout<<"normal generate pdch dataTAhashMap[sych_ce.ta]:"<<dataTAhashMap[sych_ce.ta]<<std::endl;
          }
          else
          {
            *pdu = 1 << 7 | 31; // sacch
            // std::cout<<"kuopin generate pdch dataTAhashMap[sych_ce.ta]:"<<std::endl;
          }
          pdu++;
          // zhj for test
          *pdu = (1 << 6) | rlcReport.front().lcid;
          //*pdu=(1<<6)|rlcReport.front().lcid;
          pdu++;
          *pdu = rlcReport.front().len;
          pdu++;

          // std::cout<<"-----@-------------rlcReport.front().len-------@-------:"<<rlcReport.front().len<<std::endl;
          memcpy(pdu, rlcReport.front().msg, rlcReport.front().len);
          // rlcReport.pop_front();
          // std::cout<<"----------rlcReport msg-------reset:"<<std::endl;
          // memset(rlcReport.front().msg,0,17);
          pduLen = pdu - init_ptr + 15;
          std::cout << "MAC assemble info" << std::endl;
          for (int i = 0; i < pduLen; i++)
          {
            printf("0x:%x\n", *(init_ptr + i));
          }
        }
        // std:: cout<<"pduLen"<<pduLen<<std::endl;
      }
    }
  }

  void mac::generateZeroSch2(uint8_t *pdu, int &pduLen)
  { // pdch  not adjust ta

    std::cout << "[generateSch2]ue_db[70]->mcs_adjust.is_mcs_sent:" << ue_db[70]->mcs_adjust.is_mcs_sent << std::endl;
    if (ue_db[70]->mcs_adjust.is_mcs_sent)
    {
      ue_db[70]->mcs_adjust.is_mcs_sent = false;
      uint8_t *init_ptr = pdu;
      *pdu = 1 << 7 | 31; // sacch
      pdu++;
      *pdu = 0x02; // R|E|LCID  当前E=0,LCID=2
      pdu++;
      memcpy(pdu, rlcReport.front().msg, rlcReport.front().len);
      // std::cout << "xx MAC assemble info" << std::endl;
      // for (int i = 0; i < 15; i++)
      // {
      //   printf("0x:%x\n", *(init_ptr + i));
      // }
      std::cout << "[GET SCH][IS MCS SENT][GENERATE SCH1][IS MCS SENT][END]:" << ue_db[70]->mcs_adjust.is_mcs_sent << std::endl;
    }
    else
    {
      uint8_t *init_ptr = pdu;
      if (Current_Map.dataType == Voice)
      {
        *pdu = 31;
        pdu++;
        memcpy(pdu, rlcReport.front().msg, rlcReport.front().len);
      }
      else
      {
        if (area_mode == 0)
        {
          // *pdu=dataTAhashMap[sych_ce.ta];//sacch ta
          *pdu = 31;
          // *pdu=0x40|FAhashMap[sych_ce.fa];//sacch fa
          // *pdu=0x60|PAmap[sych_ce.pa];//sacch pa
          // std::cout<<"normal generate pdch dataTAhashMap[sych_ce.ta]:"<<dataTAhashMap[sych_ce.ta]<<std::endl;
        }
        else
        {
          *pdu = 1 << 7 | 31; // sacch
          // std::cout<<"kuopin generate pdch dataTAhashMap[sych_ce.ta]:"<<std::endl;
        }
        std::cout << "xx1 rlcReport.front().lcid:" << rlcReport.front().lcid << std::endl;
        pdu++;
        *pdu = (1 << 6) | rlcReport.front().lcid;
        pdu++;
        *pdu = rlcReport.front().len;
        pdu++;

        // std::cout<<"-----@-------------rlcReport.front().len-------@-------:"<<rlcReport.front().len<<std::endl;
        memcpy(pdu, rlcReport.front().msg, rlcReport.front().len);
        // rlcReport.pop_front();
        // std::cout<<"----------rlcReport msg-------reset:"<<std::endl;
        // memset(rlcReport.front().msg,0,17);
        pduLen = pdu - init_ptr + 15;
        // std::cout << "MAC assemble info--" << std::endl;
        // for (int i = 0; i < pduLen; i++)
        // {
        //   printf("0x:%x\n", *(init_ptr + i));
        // }
      }

      // std:: cout<<"pduLen"<<pduLen<<std::endl;
    }
  }

  // PDCH1-2
  void mac::generatePdch2AgchPdu(uint8_t *pdu, int &pduLen, std::vector<int> raid)
  {
    uint8_t *init_ptr = pdu;
    int n = 0;
    for (int i = 0; i < (int)raid.size(); i++)
    {
      n++;
      setHead(0, raid[i], pdu, n == (int)raid.size());
      pdu = pdu + 2;
    }

    for (int i = 0; i < (int)raid.size(); i++)
    {
      // std::cout<<"pending_rars1[raid[i]].ta:"<<pending_rars1[raid[i]].ta<<std::endl;
      *pdu = (uint8_t)(pending_rars1[raid[i]].considInfo.consID >> 40) & 0xff;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].considInfo.consID >> 32) & 0xff;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].considInfo.consID >> 24) & 0xff;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].considInfo.consID >> 16) & 0xff;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].considInfo.consID >> 8) & 0xff;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].considInfo.consID) & 0xff;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].ta >> 2) & 0xff;
      pdu++;
      *pdu = (uint8_t)((pending_rars1[raid[i]].ta & 0x03) << 6) | 0; // pending_rars1[raid[i]].fa;
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].temp_crnti % 64);
      pdu++;
      *pdu = (uint8_t)(mac_cfg.agch.freq << 6) | (pending_rars1[raid[i]].band_id);
      pdu++;
      *pdu = (uint8_t)(pending_rars1[raid[i]].chanType2 & 0x07) << 5 | pending_rars1[raid[i]].chanAssignment2;
      pdu++;

      pending_rars1[raid[i]].is_enable = false;
    }
    //  printf("*************agchpdu**********\n");

    pduLen = pdu - init_ptr;
  }

  bool mac::valid_tti(uint32_t tti, int ra_id)
  {
    pending_rars1[ra_id].frameID = pending_rars1[ra_id].frameID % 254592;
    tti = tti % 254592;
    std::cout<<"**************1129-tti:"<<tti<<std::endl;
    std::cout<<"mac_cfg.rar_windows"<<mac_cfg.rar_windows<<std::endl;
    std::cout<<"pending_rars1[ra_id].frameID:"<<pending_rars1[ra_id].frameID<<std::endl;
    if ((int)tti > pending_rars1[ra_id].frameID + 3 &&
        (int)tti < mac_cfg.rar_windows + pending_rars1[ra_id].frameID + 3)
    {

      return true;
    }
    return false;
  }

  bool mac::valid_iot_tti(uint32_t tti, int ra_id)
  {
    if ((int)tti > iot_pending_rars1[ra_id].frameID + 3 &&
        (int)tti < mac_cfg.rar_windows + iot_pending_rars1[ra_id].frameID + 3)
    {
      return true;
    }
    return false;
  }

  void mac::sched_init(uint32_t tti, sched_t *sched_res, slot_sched_cfg_t &txCfg, phyChanType_t chanT)
  {
    sched_res->burstNum++;
    sched_res->framID = tti;
    txCfg.burstID = chanT;
    txCfg.modeType = 0;
    txCfg.equiFlag = 0;
    txCfg.udFlag = 1;
    txCfg.payloadType = 0;
  }

  // padding
  void mac::generate_padding(uint8_t *pdu)
  {
    std::cout << "generate padding" << std::endl;
    if (area_mode == 0)
    {
      // *pdu=dataTAhashMap[sych_ce.ta];//sacch
      *pdu = 31;
    }
    else
    {
      //*pdu=1<<7|dataTAhashMap[sych_ce.ta];//sacch
      *pdu = 1 << 7 | 31;
      std::cout << "generate_padding" << std::endl;
    }
    if (mac_adp->udp_.mac_receive_info.size() != 0 && mac_adp->udp_.TC_6118_mac_ce_handover)
    {
      pdu++;
      *pdu = (uint8_t)((0 << 5) | 12); // R/R/E/LCID handover mac ce sub header
      pdu++;
      *pdu = 8; // bandnum/R
      pdu++;

      printf("receive mac ce ho command msg\n");

      if (mac_adp->udp_.mac_receive_info.size() != 0)
      {
        srsran::unique_byte_buffer_t ttcn_macce_command = srsran::make_byte_buffer();
        ttcn_macce_command->init();
        mac_adp->udp_.mac_receive_info.try_pop(ttcn_macce_command);

        *pdu = ttcn_macce_command->msg[8];
        pdu++;
        *pdu = ttcn_macce_command->msg[9];
        pdu++;
        *pdu = ttcn_macce_command->msg[10];
        pdu++;
        *pdu = ttcn_macce_command->msg[11];
        pdu++;
        *pdu = ttcn_macce_command->msg[12];
        pdu++;
      }
      mac_adp->udp_.TC_6118_mac_push_pdu = true;
      mac_adp->udp_.TC_6118_mac_ce_handover = false;
    }
    else
    {
      pdu++;
      *pdu = Pading_LCID;
      pdu++;
    }
  }

  void mac::generate_Zeropadding(uint8_t *pdu) // not adjust
  {
    std::cout << "not adjust 空包" << std::endl;
    if (area_mode == 0)
    {
      //*pdu=dataTAhashMap[sych_ce.ta];//sacch
      *pdu = 31;
    }
    else
    {
      // *pdu=1<<7|dataTAhashMap[sych_ce.ta];//sacch
      *pdu = 1 << 7 | 31;
      std::cout << "generate_padding" << std::endl;
    }
    pdu++;
    *pdu = Pading_LCID;
    pdu++;
  }

  void mac::generate_psch_padding(uint8_t *pdu)
  {
    // int ta=31;
    // *pdu = ta;
    // pdu++;
    std::cout << "1201-generate psch padding" << std::endl;
    *pdu = (uint8_t)(Pading_LCID);
    pdu++;
  }
  void mac::generate_sdu(std::vector<dl_sdu_t> &macSdu, std::vector<int> raid)
  {
    for (int i = 0; i < (int)raid.size(); i++)
    {
      dl_sdu_t sdu;
      if (network_mode == 0)
      {
        sdu.sduLen = pending_rars1[raid[i]].considInfo.conSetupMsgLen;
        sdu.msg = pending_rars1[raid[i]].considInfo.conSetupMsg;
        rarReset(raid[i]);
        printf("pending_rars1[i].considInfo.conSetupMsg: %x\n", *(pending_rars1[i].considInfo.conSetupMsg));
        printf("pending_rars1[i].considInfo.conSetupMsgLen: %d\n", pending_rars1[i].considInfo.conSetupMsgLen);
      }
      else if (network_mode == 1)
      {
        sdu.sduLen = iot_pending_rars1[i].considInfo.conSetupMsgLen;
        sdu.msg = iot_pending_rars1[i].considInfo.conSetupMsg;
      }
      sdu.subhead.is_albe = true;
      sdu.subhead.LCID = 0;
      if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.TC_616_mac_ccch_logic_channel)
      {
        while (true)
        {
          if (mac_adp->udp_.mac_receive_info.size() != 0)
          {
            srsran::unique_byte_buffer_t ttcn_CCCH = srsran::make_byte_buffer();
            ttcn_CCCH->init();
            mac_adp->udp_.mac_receive_info.try_pop(ttcn_CCCH);
            sdu.subhead.LCID = ttcn_CCCH->msg[9];
            // printf("ttcn_CCCH->msg[9]=%d\n", ttcn_CCCH->msg[9]);
            mac_adp->udp_.TC_616_mac_push_pdu = true;
            break;
          }
        }
      }
      macSdu.push_back(sdu);
    }
  }
  void mac::generate_pdu(uint8_t *pdu, int &pduLen, std::vector<dl_sdu_t> sdu)
  {
    // set head
    uint8_t *init_ptr = pdu;
    std::cout << "**********sdu.size():" << (int)sdu.size() << std::endl;

    int ta = 31;

    for (int i = 0; i < (int)sdu.size(); i++)
    {
      int e = ((int)sdu.size() - i - 1);
      if (area_mode == 0)
      {
        //  *pdu=dataTAhashMap[sych_ce.ta];
        *pdu = 31;
      }
      else
      {
        *pdu = 1 << 7 | dataTAhashMap[sych_ce.ta];
      }
      pdu++;
      *pdu = 1 << 6 | (e << 5) | (CCCH_LCID);
      pdu++;
      *pdu = sdu[i].sduLen;
      pdu++;
    }
    // set payload
    for (int i = 0; i < (int)sdu.size(); i++)
    {
      memcpy(pdu, sdu[i].msg, sdu[i].sduLen);
      pdu = pdu + sdu[i].sduLen;
    }
    // int lenpdu=siezeof(*pdu);
    //     for(int i=0;i<lenpdu;i++)
    //   {
    //     std::cout<<"****************pdu:"<<*(pdu+i)<<std::endl;
    //   }
    pduLen = pdu - init_ptr;
    std::cout << "***********-------*************************  value:" << pduLen
              << std::endl;
    // for (uint8_t i = 0; i < pduLen; i++) {
    //   printf("**********set up pdu:%u\n",*(init_ptr + i));
    // }
  }
  bool mac::getTC300Timeout()
  {
    return TC300_Timeout;
  }
  //2024/11/26--jjc
  bool mac::handover_rach(){
      return is_handover_rach;
  }
  void mac::get_rar(int tti, std::vector<int> &raid)
  {

    for (int j = 0; j < MAX_PENDING_RARS; j++)
    {

      if (mac_adp->udp_.is_second_send_rach_testcase2 == false && mac_adp->udp_.mac_con_req_flag2)
      {
        // std::cout<<"mac_adp->udp_.is_second_send_rach_testcase2:"<<mac_adp->udp_.is_second_send_rach_testcase2<<std::endl;
        std::cout << "j:" << j << std::endl;
        rarReset(j);
        // return;
      }

      // std::cout<<"test case 2"<<std::endl;
      if (network_mode == 0)
      {
        if (pending_rars1[j].is_enable && valid_tti(tti, j)) // 当pending-rars1可以被发送，且tti满足发送时机时，使能PDU头
        {
          if (area_mode == 0)
          {
            // pending_rars1[j].subHead.L=11;
            pending_rars1[j].chanType1 = 2;
          }
          else
          {
            pending_rars1[j].chanType1 = 2;
            // pending_rars1[j].subHead.L=13;
            pending_rars1[j].PDTCHCodeIndex = 20;
            pending_rars1[j].PDTCHType = 0; // dspdtch1--0;ds
            printf("pending_rars1[j].PDTCHType %d\n", pending_rars1[j].PDTCHType);
            // pending_rars1[j].fa=0;
            // pending_rars1[j].ta=254;
            pending_rars1[j].ds_chanAss1 = 2;
            pending_rars1[j].ds_chanType1 = 1;
          }
          raid.push_back(j);
          if (!TC300_Timeout)
          {
            pending_rars1[j].considInfo.isEnable = true;
          }
        }
        else if (pending_rars1[j].is_enable && (int)tti > mac_cfg.rar_windows + pending_rars1[j].frameID + 3) // 超出时间窗，丢弃该RAR
        {
          rarReset(j);
        }
      }
      else if (network_mode == 1)
      {
        if (iot_pending_rars1[j].is_enable &&
            valid_iot_tti(tti, j)) // 当pending-rars1可以被发送，且tti满足发送时机时，使能PDU头
        {
          raid.push_back(j);
          iot_pending_rars1[j].considInfo.isEnable = true;
        }
        else if ((int)tti > mac_cfg.rar_windows + iot_pending_rars1[j].frameID + 3) // 超出时间窗，丢弃该RAR
        {
          rarReset(j);
        }
      }
    }
  }
  void mac::get_consinfo(int tti, std::vector<int> &raid)
  {
    for (int j = 0; j < MAX_PENDING_RARS; j++)
    {
      if (network_mode == 0)
      {
        if (pending_rars1[j].considInfo.isEnable) // 当pending-rars1可以被发送，且tti满足发送时机时，使能PDU头
        {
          raid.push_back(j);
          rlcStateReport(1, pending_rars1[j].temp_crnti);
          // rarReset(j);
        }
      }
      else if (network_mode == 1)
      {
        if (iot_pending_rars1[j].considInfo.isEnable) // 当pending-rars1可以被发送，且tti满足发送时机时，使能PDU头
        {
          raid.push_back(j);
          rarReset(j);
        }
      }
    }
  }

  // IoT获取调度
  int mac::get_IoT_sched(uint32_t tti, std::vector<sched_t> &dl_sched_res)
  {
    // 计算系统帧号和子帧号
    srsran::rwlock_read_guard lock(rwlock);
    int sfn = tti / 52;
    int sf = tti % 52;
    for (int fq_index = 0; fq_index < 4 * 56; fq_index++)
    {
      if (!wxScheduler.dl_IoT_sched(fq_index, tti, sched_result))
      {
        return 0;
      }
      sched_t sched_res;
      bzero(static_cast<void *>(txCfg), sizeof(slot_sched_cfg_t) * 5);
      for (int i = 0; i < MaxSlotID; i++)
      {
        bzero(pduData[i], sizeof(uint8_t) * 400);
      }
      int n = 0;
      for (int i = 0; i < MaxSlotID; i++)
      {
        // 调度IoTsi数据
        if (sched_result.IoTsi[i].is_sched)
        {
          sched_result.IoTsi[i].is_sched = false;
          txCfg[i] = sched_result.IoTsi[i].IoTsiCfg;
          generatePtdchPdu(IoTsiData, pduData[i], sched_result.IoTsi[i].IoTsiLen);
          txCfg[i].pduBitlen = MAX_IoTsi_PDU_LEN;
          n++;
        }
        // 调度MIB数据
        else if (sched_result.mib[i].is_sched)
        {
          sched_result.mib[i].is_sched = false;
          txCfg[i] = sched_result.mib[i].mibCfg;
          // generatePmbchPdu(pduData[i], sched_result.mib[i].mibLen,sched_result.mib[i].pcchLen,sched_result.mib[i].si,sched_result.mib[i].index);
          txCfg[i].pduBitlen = MAX_MIB_PDU_LEN;
          n++;
        }
        // 调度SIB数据
        else if (sched_result.sib[i].is_sched)
        {
          sched_result.sib[i].is_sched = false;
          n++;
          txCfg[i] = sched_result.sib[i].sibCfg;
          int offset = (sched_result.sib[i].index - 1) * 18;
          txCfg[i].pduBitlen = MAX_SIB_PDU_LEN;
          if (sched_result.sib[i].sibLen > 0)
          {
            generateSibPdu(sched_result.sib[i].si, sibData + offset, pduData[i], sched_result.sib[i].sibLen);
          }
          else
          {
            memset(pduData[i], 0xdf, 1);
            memset(pduData[i] + 1, 0xff, MAX_SIB_PDU_LEN - 1);
          }
        }
        // 调度RAR
        else if (sched_result.iot_rar[i].is_sched)
        {
          sched_result.iot_rar[i].is_sched = false;
          if ((int)sched_result.iot_rar[i].raid.size() > 0)
          {
            int len;
            if (area_mode == 0)
            {
              iot_NM_generateAgchPdu(pduData[i], len, sched_result.iot_rar[i].raid);
            }
            else if (area_mode == 1)
            {
              iot_SS_generateAgchPdu(pduData[i], len, sched_result.iot_rar[i].raid);
            }
            n++;
            txCfg[i] = sched_result.iot_rar[i].rarCfg;
            txCfg[i].pduBitlen = len;
            srsran::console("物联网发送RAR--\n");
          }
        }
        else if (sched_result.data[i].is_sched)
        {
          sched_result.data[i].is_sched = false;
          std::vector<dl_sdu_t> macSdu;
          if ((int)sched_result.data[i].raid.size() > 0)
          {
            int len;
            generate_sdu(macSdu, sched_result.data[i].raid);
            generate_pdu(pduData[i], len, macSdu);
            n++;
            txCfg[i] = sched_result.data[i].dataCfg;
            wxScheduler.reset_source(fq_index / 4, fq_index % 4, tti % 52, i);
            txCfg[i].pduBitlen = len;
            srsran::console("物联网发送连接建立消息\n");
          }
        }
        if (txCfg[i].pduBitlen >= 0)
        {
          memcpy(txCfg[i].pduBits, pduData[i], txCfg[i].pduBitlen);
          sched_res.burstPrasCfg[i] = txCfg[i];
        }
      }
      if (n > 0)
      {
        sched_res.BandID = fq_index / 4;
        sched_res.burstNum = n;
        sched_res.txFreq = fq_index % 4;
        sched_res.framID = tti;
        dl_sched_res.push_back(sched_res);
      }
    }

    // std::cout << "----------------------sf---------------------:" << tti << std::endl;
    // std::cout << "-----------------iot_dl_grant----------------:" << dl_sched_res.size() << std::endl;
    return 0;
  }

  int mac::get_sched(uint32_t tti, std::vector<sched_t> &dl_sched_res, uint32_t prach_tti)
  {
    std::cout << "[GET][SCHED][PID]=" << Pid << std::endl;
    // 计算系统帧号和子帧号
    printf(" get_sched_start_time enble\n");
    srsran::rwlock_read_guard lock(rwlock);
    // std::cout<<"get sched"<<tti<<std::endl;
    int sfn = tti / 52;
    int sf = tti % 52;
    int current_tti = tti % 254592; // MAC TTI
    printf(" sfn =%d \n", sfn);
    printf(" sf =%d \n", sf);
    printf(" current_tti =%d \n", current_tti);
    printf(" area_mode =%d \n", area_mode);

    printf("1128tti====%d\n",tti);
    // for (int fq_index = 0; fq_index < 4 * 56; fq_index++)z
    for (int i = 0; i < MaxSlotID; i++)   //先载波后时隙，因为小循环是频域，第一个时隙的频域循环遍历完了才是下一个时隙，因此是先载波后时隙
    {
      // printf("1128slot:%d\n",i);
      start_time = std::chrono::high_resolution_clock::now();
      first_time = std::chrono::high_resolution_clock::now();
      end_time = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
      // printf("[GET_SCHED][%d][DL_SCHED][TIME]:%lu\n", duration.count());
      if (duration.count() > 1000)
      {
        printf("[GET_SCHED][DL_SCHED][OVER][1000]");
      }
      // bzero(static_cast<void *>(txCfg), sizeof(slot_sched_cfg_t) * 5);
      // for (int i = 0; i < MaxSlotID; i++)
      // {
        // bzero(pduData[i], sizeof(uint8_t) * 400);
      // }
      // int n = 0;
      sched_t sched_res;
      bzero(&sched_res, sizeof(sched_t));
      sched_res.k_s = 1;            // 0-11
      sched_res.Scrambling_num = 0; //(0,16,32.48)
      sched_res.ds_mode = area_mode;
      // std::cout<<"mac::get_sched ds_mode"<<sched_res.ds_mode<<std::endl;

      // for (int i = 0; i < MaxSlotID; i++)
      for (int fq_index = 0; fq_index < 4 * 56; fq_index++)
      {
        // printf("1128fq_index:%d\n",fq_index);
        int n = 0;
        bzero(static_cast<void *>(txCfg), sizeof(slot_sched_cfg_t) * 5);
        bzero(pduData[i], sizeof(uint8_t) * 400);
        if (!wxScheduler.dl_sched(i, fq_index, tti, sched_result))
        {
          return 0;
        }        
        if (sched_result.mib[i].is_sched)
        { // 调度MIB数据
          printf("wwh____________617\n");
          sched_result.mib[i].is_sched = false;
          txCfg[i] = sched_result.mib[i].mibCfg;
          generatePmbchPdu(mibData, &pcchData, pduData[i], sched_result.mib[i].mibLen, sched_result.mib[i].pcchLen, sched_result.mib[i].index, sched_result.mib[i].is_last);
          if (area_mode == 0)
          {
            printf(" MIB area_mode = 0");
            txCfg[i].pduBitlen = MAX_MIB_PDU_LEN;
          }
          else
          {
            printf(" MIB area_mode = 1");
            txCfg[i].pduBitlen = MAX_SS_MIB_PDU_LEN;
            printf(" MIB txCfg[%d] pduBitlen = %d\n", i, txCfg[i].pduBitlen);
          }
          n++;
          if (sched_result.mib[i].pcchLen > 0)
          {
            for (int c = 0; c < MAX_MIB_PDU_LEN; c++)
            {
              printf("MIBPDU:%x\n", *(pduData[i] + c));
            }
          }
        }
        else if (sched_result.pcch[i].is_sched) // 调度PCCH数据
        {
          printf(" now is sched pcch");
          sched_result.pcch[i].is_sched = false;
          txCfg[i] = sched_result.mib[i].pcchCfg;
          generatePmbchPdu(mibData, &pcchData, pduData[i], sched_result.mib[i].mibLen, sched_result.mib[i].pcchLen, sched_result.mib[i].index, sched_result.mib[i].is_last);
          if (area_mode == 0)
          {
            printf(" MIB area_mode = 0");
            txCfg[i].pduBitlen = MAX_MIB_PDU_LEN;
          }
          else
          {
            printf(" MIB area_mode = 1");
            txCfg[i].pduBitlen = MAX_SS_MIB_PDU_LEN;
            printf(" MIB txCfg[%d] pduBitlen = %d\n", i, txCfg[i].pduBitlen);
          }
          n++;
        }
        else if (sched_result.fcch[i])
        { // 调度PFCCH数据
          sched_result.fcch[i] = false;
          txCfg[i].burstID = phyChanType_t::PFCCH;
          txCfg[i].pduBitlen = 0;
          n++;
        }
        else if (sched_result.sib[i].is_sched)
        { // 调度SIB数据
          sched_result.sib[i].is_sched = false;
          n++;
          txCfg[i] = sched_result.sib[i].sibCfg;
          int offset = sched_result.sib[i].index;
          if (area_mode == 0)
          {
            txCfg[i].pduBitlen = MAX_SIB_PDU_LEN;
          }
          else
          {
            txCfg[i].pduBitlen = MAX_SS_SIB_PDU_LEN;
          }
          if (sched_result.sib[i].sibLen > 0)
          {
            if (area_mode == 0)
            {
              printf(" SIB area_mode = 0");
              generateSibPdu(sched_result.sib[i].si, sibData + offset, pduData[i], sched_result.sib[i].sibLen);
            }
            else
            {
              printf(" SIB area_mode = 1");
              // txCfg[i].pduBitlen = MAX_SS_SIB_PDU_LEN;
              printf(" SIB txCfg[%d] pduBitlen = %d\n", i, txCfg[i].pduBitlen);
              // std::cout<<"sib input"<<std::endl;
              SS_generateSibPdu(sched_result.sib[i].si, sibData + offset, pduData[i], sched_result.sib[i].sibLen);
              // std::cout<<"sib output"<<std::endl;
            }
          }
          else
          {
            if (area_mode == 0)
            {
              memset(pduData[i], 0xdf, 1);
              memset(pduData[i] + 1, 0xff, MAX_SIB_PDU_LEN - 1);
            }
            else
            {
              memset(pduData[i], 0xdf, 1);
              memset(pduData[i] + 1, 0xff, MAX_SS_SIB_PDU_LEN - 1);
            }
          }
        }
        else if (sched_result.rar[i].is_sched)
        { // 调度RAR
          std::cout << "AGCH SCHED  PID = "<<Pid<<"tti = "<<current_tti<<std::endl;
          sched_result.rar[i].is_sched = false;
           //2024/8/16------------wwh----------
          if(is_handover_rach){
            HO_RACH_COUNT++;
            //std::cout << "handover send time TTI = "<<current_tti<<std::endl;
            std::cout<<wxScheduler.agch.band_ID<<"-"<<wxScheduler.agch.freq<<"-"<<wxScheduler.agch.slot<<std::endl;
            int len;
            //printf("ue_db[70]->is_handover_msg=%d\n",ue_db[70]->get_handover_para()->is_handover_falg);
            //printf("开始组装切换rar\n");
            printf("ho rar processing, count:%d\n",HO_RACH_COUNT);
            generateHOagchpdu(pduData[i], len);
            n++;
            txCfg[i].pduBitlen = RAR_LEN;
            wxScheduler.sched_set_parameter(tti,txCfg[i],PSBCH);
            //printf("发送RAR的帧号=%d\n",sf);
            for(int j=0;j<len;j++){
              printf("生成的切换agch pdu数据为:0x%x\n",*(pduData[i]+j));
            }
            //ue_db[70]->get_handover_para()->is_handover_falg=false;
            if (HO_RACH_COUNT==1){
              std::cout<<"alloc source after the first agch."<<std::endl;
              setulAlloc(ulAlloc_);
              setupAllocate(dlAlloc);
              wxScheduler.allocate_source(dlAlloc);
              phy_h->set_parameters(dlAlloc.bandID, dlAlloc.freq, dlAlloc.solt, 0);
            }
            if(HO_RACH_COUNT==10)
            {
              is_handover_rach=false;
              HO_RACH_COUNT=0;
            }
            std::cout<<"XX PID = "<<Pid<<"(int)sched_result.rar[i].raid.size() = "<<(int)sched_result.rar[i].raid.size()<<std::endl;
          }
          //std::cout<<"XX PID = "<<Pid<<"(int)sched_result.rar[i].raid.size() = "<<(int)sched_result.rar[i].raid.size()<<std::endl;
          //------------------------------------------
          else if ((int)sched_result.rar[i].raid.size() > 0&&is_handover_rach==false)
          {
            int len;
            // generatePdch2AgchPdu(pduData[i], len, sched_result.rar[i].raid); //pdch1-2
            if (area_mode == 0)
            {
              // generatePdch2AgchPdu(pduData[i], len, sched_result.rar[i].raid);
              generateAgchPdu(pduData[i], len, sched_result.rar[i].raid); // pdch1-1
              if (TC300_Timeout)
                TC300_Timeout = false;
              for (int j = 0; j < len; j++)
              {
                printf("0x:%x\n", pduData[i][j]);
              }
              std::cout << "normal!!" << std::endl;
              // generatePsychAgchPdu(pduData[i], len, sched_result.rar[i].raid);  //psych
            }
            else
            {
              std::cout << "spread spectrum!!" << std::endl;
              SS_generateAgchPdu(pduData[i], len, sched_result.rar[i].raid);
              // SS_generatePsychAgchPdu(pduData[i], len, sched_result.rar[i].raid);
            }
            n++;
            txCfg[i] = sched_result.rar[i].rarCfg;

            txCfg[i].pduBitlen = RAR_LEN;

            // txCfg[i].pduBitlen =37; //pdch1-2
            //  if(area_mode==0){
            ul_allocate ulAlloc;
            ulAlloc.Type = psych0;
            ulAlloc.bandID = area_mode == 0 ? 8 : 3;
            ulAlloc.freq = 1;
            ulAlloc.solt = area_mode == 0 ? 2 : 1;

            // zhj
            if (ue_category == 14)
            {
              std::cout << " ue_category @@ 2 " << std::endl;
              set_uecategory14();
              phy_h->set_parameters(ue_cap14_band_id, ue_cap14_freq_id, ue_cap14_slot_phy, 0);
            }
            else
            {
              setulAlloc(ulAlloc);
              phy_h->set_parameters(ulAlloc.bandID, ulAlloc.freq, ulAlloc.solt, 0);
              std::cout << "ulAlloc type:" << ulAlloc.Type;
            }

            //}

#if 0
          for(int j=0;j<txCfg[i].pduBitlen;j++)
          {
            printf("agch pduData[i]rar:%x\n",pduData[i][j]);
          }
#endif
            srsran::console("接入网发送RAR--\n");
          }
        }
        else if (sched_result.data[i].is_sched)
        { // SETUP INFO
          std::cout << "enter  data" << std::endl;
          sched_result.data[i].is_sched = false;
          std::vector<dl_sdu_t> macSdu;
          if ((int)sched_result.data[i].raid.size() > 0)
          {
            int len;
            generate_sdu(macSdu, sched_result.data[i].raid);
            generate_pdu(pduData[i], len, macSdu);
            n++;
            txCfg[i] = sched_result.data[i].dataCfg;
            wxScheduler.reset_source(fq_index / 4, fq_index % 4, tti % 52, i);
            if (ue_category == 14)
            {
              txCfg[i].pduBitlen = 7; // pdch1-1
            }
            else
            {
              txCfg[i].pduBitlen = 19; // pdch1-1
            }
            // txCfg[i].pduBitlen = 37;//pdch1-2
            //  if(area_mode==1){
            //      txCfg[i].burstID=phyChanType_t(dlAlloc.Type+10);
            //      txCfg[i].pduBitlen=dlAlloc.pduLen;
            //   }

            // txCfg[i].pduBitlen = 37;//pdch1-2
            //  for(int j=0;j<txCfg[i].pduBitlen;j++)
            //  {
            //    printf("pduData[i]1:%x\n",pduData[i][j]);
            //  }
            std::cout << "!!!!!fq_index:" << fq_index << std::endl;
            std::cout << "!!!!!SFN:" << tti << std::endl;
            srsran::console("接入网发送连接建立消息\n");
            //  ulAlloc2.Type=pdch1_1;
            //  ulAlloc2.bandID=9;
            //  ulAlloc2.freq=1;
            //  ulAlloc2.solt=3;
            ul_allocate ulAlloc2;
            ulAlloc2.Type = ulAlloc_.Type;
            ulAlloc2.bandID = ulAlloc_.bandID;
            ulAlloc2.freq = ulAlloc_.freq;
            ulAlloc2.solt = ulAlloc_.solt;

            // zhj
            if (ue_category == 14)
            {
              std::cout << " ue_category @@ 1 " << std::endl;
              set_uecategory14();
              phy_h->set_parameters(ue_cap14_band_id, ue_cap14_freq_id, ue_cap14_slot_phy, 0);
            }
            else
            {
              setulAlloc(ulAlloc2);
              if (area_mode == 0)
              {
                phy_h->set_parameters(dlAlloc.bandID, dlAlloc.freq, dlAlloc.solt, 0);
              }
              else if (area_mode == 1)
              {
                phy_h->set_parameters(ulAlloc2.bandID, ulAlloc2.freq, ulAlloc2.solt, 0);
              }
              std::cout << "ulAlloc_ type :" << ulAlloc2.Type << "bandID:" << ulAlloc2.bandID << "ulAlloc_.freq" << ulAlloc2.freq << "slot:" << ulAlloc2.solt << std::endl;
            }
          }
        }
        else if (sched_result.sch2[i].is_sched)
        { // 11.06
          //std::cout << "zhj  tti:" << tti << std::endl;
          std::cout << "sch2, Pid:" << Pid << std::endl;
          std::cout << "fq_index:" << fq_index << " "
                    << "slot:" << i << std::endl;
          int adjust_time = tti * 60;
          if (adjust_flag == 0)
          {
            adjust_start = adjust_time;
            adjust_flag = 1;
          }
          adjust_interval = adjust_time - adjust_start;
          if (adjust_interval > notAdjustT)
          {
            adjust_interval = 0;
            adjust_flag = 0;
            // sched_result.sych[i] = false;
          }
          // sched_result.sch=flase;
          sched_result.sch2[i].is_sched = false;
          txCfg[i] = sched_result.sch2[i].sch2Cfg;
          //  generate_padding(pduData[i]);
          if (adjust_flag == 0)
          {
            std::cout << " djust_flag == 0 " << std::endl;
            std::cout << "generateSch2 sched_result.sch2[i].len" << sched_result.sch2[i].len << std::endl;
            if (sched_result.sch2[i].len > 0)
            {
              std::cout << "sched_result.sch2[i].len>0" << std::endl;
              generateSch2(pduData[i], txCfg[i].pduBitlen);
              std::cout << "txCfg[i].pduBitlen:" << txCfg[i].pduBitlen << std::endl;
            }
            else
            {
              generate_padding(pduData[i]);
              std::cout << " 1generate_Zeropadding(pduData[i]); " << std::endl;
            }
          }
          else
          {
            std::cout << " djust_flag != 0 " << std::endl;
            std::cout << "generateSch2 sched_result.sch2[i].len" << sched_result.sch2[i].len << std::endl;
            if (sched_result.sch2[i].len > 0)
            {
              // generateZeroSch2(pduData[i], txCfg[i].pduBitlen);
              generateSch2(pduData[i], txCfg[i].pduBitlen);
              std::cout << "txCfg[i].pduBitlen:" << txCfg[i].pduBitlen << std::endl;
            }
            else
            {
              generate_padding(pduData[i]);
              std::cout << " 2generate_Zeropadding(pduData[i]); " << std::endl;
              for (int j = 0; j < 5; j++)
              {
                printf("0x:%x\n", pduData[i][j]);
              }
            }
          }
          n++;
          if (ue_category == 14)
          {
            txCfg[i].pduBitlen = 7;
          }
          else
          {
            std::cout << "dlAlloc.pduLen=" << dlAlloc.pduLen << std::endl;
            txCfg[i].pduBitlen = dlAlloc.pduLen;
          }
          std::cout << "zz txCfg[i].pduBitlen = " << txCfg[i].pduBitlen << std::endl;
          if (area_mode == 1)
          {
            txCfg[i].burstID = phyChanType_t(dlAlloc.Type + 10); // DS_PDTCH_1,DS_PDTCH_2,DS_PTDCH_3,DS_PTDCH_T
          }
          // txCfg[i].DSType=(int)dlAlloc.Type-6;
          txCfg[i].SF_i = dlAlloc.SF_i;
          // std::cout<<"txCfg[i].burstID"<<txCfg[i].DSType<<std::endl;
          // printf("txCfg[i].burstID:%d\n",txCfg[i].burstID);
          // std::cout<<"padding slot-----------:"<<i<< "len"<< txCfg[i].pduBitlen<<std::endl;
        }
        else if (sched_result.sch1[i].is_sched)
        { // PSCH INFO
          sched_result.sch1[i].is_sched = false;
          n++;
          txCfg[i] = sched_result.sch1[i].sch1Cfg;
          transPUI(pduData[i], txCfg[i].pui);
          // printf("jjcjcjccjcjcjtransPUI pui:%x", txCfg[i].pui);
          if (sched_result.sch1[i].len > 0)
          {
            generateSch1(pduData[i], txCfg[i].pui, i);
          }
          else
          {
            generate_psch_padding(pduData[i]);
            // for (int j = 0; j < 10; j++)
            // {
            //   printf("1201-psch padding 0x:%x\n", pduData[i][j]);
            // }
          }
          n++;
          // generate_padding(pduData[i]);
          txCfg[i].pduBitlen = dlAlloc.pduLen; // PSCH1-1=14/20/23  ||  PSCH1-2=36/48/58 ||   PSCH5-1=102/144/185  || PSCH5-2=219/307/395
          std::cout << "[GET SCHED][SCH1][ txCfg[i].pduBitlen]:" << txCfg[i].pduBitlen << std::endl;
          std::cout << "psch slot and band id-----------:" << i << " " << tti << std::endl;
          txCfg[i].SF_i = 21;

#if 0
        for(int j=0;j<txCfg[i].pduBitlen;j++)
        {
        printf("SCH1----------pduData[i]:%x\n",pduData[i][j]);
        }
#endif
        }
        else if (sched_result.sych[i])
        { // PSYCH
          // sched_result.sch=flase;
          sched_result.sych[i] = false;
          if (get_CI_state(mac_rnti) < 2 && get_CI_state(mac_rnti) > -1)
          {
            printf("sych sendSychTime= %d\n", sendSychTime);
            if (sendSychTime == 0)
            { // 0:adjust  1:don't adjust
              generatePsychPdu(pduData[i]);
              if (psychSS == 0)
              {
                std::cout << "psych have information" << std::endl;
              }
              else
              {
                std::cout << "switch" << std::endl;
              }
              sendSychTime = 1;
            }
            else
            {
              generatePsychZeroPdu(pduData[i]);
              std::cout << "zeropsych" << std::endl;
            }
            //  }else{
            // generatePsychPdu(pduData[i]);
          }
          n++;
          txCfg[i].burstID = area_mode == 0 ? PSYCH : DS_PDTCH_1;
          txCfg[i].pduBitlen = area_mode == 0 ? 14 : 19;
          txCfg[i].SF_i = 20;
          std::cout << "sych slot-----------:" << i << std::endl;
#if 0
          for(int j=0;j<txCfg[i].pduBitlen;j++)
          {
            printf("sych-----------pduData[i]:%x\n",pduData[i][j]);
          }
#endif
        }
        if (txCfg[i].pduBitlen >= 0)
        {
          memcpy(txCfg[i].pduBits, pduData[i], txCfg[i].pduBitlen);
          sched_res.burstPrasCfg[i] = txCfg[i];

          if (txCfg[i].pduBitlen > 0)
          {
            printf(" 1128burstPrasCfg[%d] PDUBITLEN = %d \n", i, txCfg[i].pduBitlen);
            printf(" 1128burstPrasCfg[%d] burstID = %d \n", i, txCfg[i].burstID);
          }
          /**
           *@brief  to PHY distinguish Voice ,Data ,Control
           *@author zhaohongjun
           *@date   2024/05/17
           */
          if (Current_Map.dataType == Voice)
          {
            sched_res.burstPrasCfg[i].payloadType = 0; // Voice
          }
          else if (Current_Map.dataType == Data)
          {
            sched_res.burstPrasCfg[i].payloadType = 1; // Data
          }
          else
          {
            sched_res.burstPrasCfg[i].payloadType = 2; // Control
          }
        }
          if (n > 0)
          {
            sched_res.BandID = fq_index / 4;
            sched_res.burstNum = n;
            sched_res.txFreq = fq_index % 4;
            sched_res.framID = tti;

            bool next_cc = true;
            for (int cc = 0; cc < (int)((dl_sched_res).size()); cc++){
              if (sched_res.BandID == dl_sched_res[cc].BandID && sched_res.txFreq == dl_sched_res[cc].txFreq){
                dl_sched_res[cc].burstPrasCfg[i] = sched_res.burstPrasCfg[i];
                dl_sched_res[cc].burstNum++;
                next_cc = false;
                std::cout<<"-update sched_res.framID :"<<sched_res.framID<<std::endl;
                std::cout<<"update slot :"<<i<<std::endl;
                std::cout<<"update BandID :"<<sched_res.BandID <<std::endl;
                std::cout<<"update fq_index :"<<sched_res.txFreq <<std::endl;
                std::cout<<"update n:"<<sched_res.burstNum <<std::endl;
                std::cout<<"update burstID :"<<sched_res.burstPrasCfg[i].burstID <<std::endl;                
              }
            }
            if (next_cc == true || 0  == dl_sched_res.size()){
              dl_sched_res.emplace_back(sched_res);
              std::cout<<"-addnew sched_res.framID :"<<sched_res.framID<<std::endl;
              std::cout<<"addnew slot :"<<i<<std::endl;
              std::cout<<"addnew BandID :"<<sched_res.BandID <<std::endl;
              std::cout<<"addnew fq_index :"<<sched_res.txFreq <<std::endl;
              std::cout<<"addnew n:"<<sched_res.burstNum <<std::endl;
              std::cout<<"addnew burstID :"<<sched_res.burstPrasCfg[i].burstID <<std::endl;                
            }
          }        
      }

      first_time_end = std::chrono::high_resolution_clock::now();

      auto duration_D1 = std::chrono::duration_cast<std::chrono::microseconds>(first_time_end - first_time);
      // printf("[GET_SCHED][%d][DL_SCHED][TIME]:%lu\n", duration.count());
      if (duration_D1.count() > 1000)
      {
        printf("[GET_SCHED][DL_SCHED][OVER][1000]duration_D1");
        //printf("[GET_SCHED][fq_index=%d][DL_SCHED][OVER][1000][TIME]duration_D1:%lu\n", fq_index, duration_D1.count());
      }
    }
    return 0;
  }

  //******************************2023/09/15***********************************
  int mac::iot_rach_detected(iot_prachInfo_t *iot_prachInfo, uint8_t *pdu, int pduLen)
  {
    if (!iot_prachInfo->crc)
    {
      return -1;
    }
    uint32_t ra_id = 0;
    // 给RACH消息的RAR分配内存
    while (iot_pending_rars1[ra_id].temp_crnti != (uint16_t)-1 && ra_id < MAX_PENDING_RARS)
    {
      ra_id++;
    }
    if (ra_id == MAX_PENDING_RARS)
    {
      // srsran::console("超出最大未处理RAR\n");
      return -1;
    }
    uint16_t rnti = allocate_ue(0);
    srsran::console("物联网下创建UE,RNTI:%u,帧号：%u,RA_ID:%u\n", rnti, iot_prachInfo->framID, ra_id);
    // prachInfo->framID/=5;
    iot_pending_rars1[ra_id].frameID = iot_prachInfo->framID;
    iot_pending_rars1[ra_id].ta = iot_prachInfo->ta;
    iot_pending_rars1[ra_id].fa = iot_prachInfo->fa;
    iot_pending_rars1[ra_id].pa = iot_prachInfo->pa;
    iot_pending_rars1[ra_id].temp_crnti = rnti;
    // pending_rars1[ra_id].is_enable=true;
    iot_pending_rars1[ra_id].considInfo.tti = iot_prachInfo->framID;
    iot_pending_rars1[ra_id].considInfo.rnti = rnti;
    iot_pending_rars1[ra_id].subHead.roid = iot_prachInfo->framID % 16;
    std::cout << "--------------------rach detected------------------" << std::endl;
    std::cout << "--------------------iot_pending_rars1[ra_id].frameID:" << iot_pending_rars1[ra_id].frameID << std::endl;
    std::cout << "--------------------iot_pending_rars1[ra_id].ta:" << iot_pending_rars1[ra_id].ta << std::endl;
    std::cout << "--------------------iot_pending_rars1[ra_id].fa:" << iot_pending_rars1[ra_id].fa << std::endl;
    std::cout << "--------------------iot_pending_rars1[ra_id].pa:" << iot_pending_rars1[ra_id].pa << std::endl;
    std::cout << "--------------------iot_pending_rars1[ra_id].temp_crnti:" << iot_pending_rars1[ra_id].temp_crnti
              << std::endl;
    std::cout << "--------------------iot_pending_rars1[ra_id].considInfo.tti:" << iot_pending_rars1[ra_id].considInfo.tti
              << std::endl;
    std::cout << "-------------------- iot_pending_rars1[ra_id].considInfo.rnti:"
              << iot_pending_rars1[ra_id].considInfo.rnti << std::endl;
    std::cout << "--------------------iot_pending_rars1[ra_id].subHead.roid:" << iot_pending_rars1[ra_id].subHead.roid
              << std::endl;
    // Add new user to the scheduler so that it can RX/TX SRB0
    sched_interface::ue_cfg_t uecfg = {};
    uecfg.supported_cc_list.emplace_back();
    uecfg.supported_cc_list.back().active = true;
    uecfg.supported_cc_list.back().enb_cc_idx = 0;
    uecfg.ue_bearers[0].direction = mac_lc_ch_cfg_t::BOTH;
    uecfg.supported_cc_list[0].dl_cfg.tm = SRSRAN_TM1;
    if (ue_cfg(rnti, &uecfg) != SRSRAN_SUCCESS)
    {
      return -1;
    }
    // 在RRC层注册新用户
    if (rrc_h->add_user(rnti, uecfg) == SRSRAN_ERROR)
    {
      ue_rem(rnti);
      return -1;
    }
    push_pdu(iot_prachInfo->framID, rnti, pdu, pduLen, iot_prachInfo->crc, 1); // 上传PDU
    std::cout << "--------------------This is rach Pdu------------------" << std::endl;
    for (int i = 0; i < pduLen; i++)
    {
      srsran::console("0x%x\n", *(pdu + i));
    }
    return 0;
  }
  //****************************************

  int mac::sych_detected(psychInfo_t *psychInfo)
  {
    if (judge_rach_detect_counter == true)
    {
      if (crc_check_failure_counter == 50)
      {
        crc_check_failure_counter = 0; // reset
        if (mac_adp->udp_.TTCN_TEST == true)
        {
        }
        else
        {
          rrc_h->Notify_rrc_to_notify_nas_to_release(70);
        }
      }

      if (!psychInfo->crc)
      {
        ++crc_check_failure_counter;
        std::cout << " crc_check_failure_counter = " << crc_check_failure_counter << std::endl;
        logger.debug("[WX][PUSH][PDU]CRC = false counter:%d", crc_check_failure_counter);
        return -1;
      }
      else
      {
        crc_check_failure_counter = 0;
      }
    }
    else
    {
      crc_check_failure_counter = 0;

      if (!psychInfo->crc)
      {
        return -1;
      }
    }
    // if(psychInfo->fa!=sych_ce.fa||sych_ce.pa!=psychInfo->pa||abs(sych_ce.ta-psychInfo->ta)>2.5){
    if (psychInfo->fa != sych_ce.fa || abs(sych_ce.ta - psychInfo->ta) > 2.5)
    {
      sendSychTime = 0;
    }
    // printf("sych detected sendSychTime= %d\n", sendSychTime);
    sych_ce.fa = (int)(psychInfo->fa);
    mac_adp->udp_.fa_adjust_after = sych_ce.fa; // 记录调整后的fa值
    // sych_ce.fa=0;
    // sych_ce.pa = 0;
    sych_ce.pa = psychInfo->pa;
    mac_adp->udp_.pa_adjust_after = sych_ce.pa; // 记录调整后的pa值
    sych_ce.ta = psychInfo->ta;
    mac_adp->udp_.ta_adjust_after = sych_ce.ta; // 记录调整后的ta值
    // std::cout<<"psychInfo.crc:"<<psychInfo->crc <<std::endl;
    std::cout << "psychInfo.fa :" << sych_ce.fa << std::endl;
    std::cout << "psychInfo.pa " << sych_ce.pa << std::endl;
    std::cout << "psychInfo.ta:" << sych_ce.ta << std::endl;
    // map

    int n = 0;
    int o = 0;
    // sych ta
    for (float i = -15.875; i <= 15.875; i += 0.125)
    {
      float a = (float)(int)(i * 1000) / 1000;
      TAhashMap[i] = n;
      n++;
    }
    // pdch ta
    for (float i = -3.875; i <= 3.875; i += 0.125)
    {
      float a = (float)(int)(i * 1000) / 1000;
      dataTAhashMap[i] = o;
      o++;
    }
    // fa
    int m = 0, k = 17;
    for (int j = 0; j <= 2400; j += 160)
    {
      FAhashMap[j] = m;
      m++;
    }
    for (int l = -160; l >= -2400; l -= 160)
    {
      FAhashMap[l] = k;
      k++;
    }
    if (sych_ce.ta > time_toleranceMax)
    {
      TAhashMap[sych_ce.ta] = TAhashMap[time_toleranceMax];
    }
    if (sych_ce.ta < time_toleranceMin)
    {
      TAhashMap[sych_ce.ta] = TAhashMap[time_toleranceMin];
    }
    if (sych_ce.fa > freq_toleranceMax)
    {
      FAhashMap[sych_ce.fa] = FAhashMap[freq_toleranceMax];
    }
    if (sych_ce.fa < freq_toleranceMin)
    {
      FAhashMap[sych_ce.fa] = FAhashMap[freq_toleranceMin];
    }
    // pdch ta
    if (sych_ce.ta > data_time_toleranceMax)
    {
      dataTAhashMap[sych_ce.ta] = dataTAhashMap[data_time_toleranceMax];
    }
    if (sych_ce.ta < data_time_toleranceMin)
    {
      dataTAhashMap[sych_ce.ta] = dataTAhashMap[data_time_toleranceMin];
    }
    return 0;
  }

  void mac::clear_ue()
  {
    srsran::rwlock_write_guard rw_lock(rwlock);
    if (not ue_db.empty())
    {
      std::cout << "clear_ue()" << std::endl;
      // rrc_h->Notify_rrc_to_notify_nas_to_release(70);
      // ue_db.clear(); //crc failure over 30

      // rrc mac - release
      if (!rrc_h->Notify_rrc_release_rach(70))
      {
        std::cout << "rach Notify release failure!" << std::endl;
      }
    }
  }

  // v0.0.5****************************************************************
  int mac::rach_detected(prachInfo_t *prachInfo, uint8_t *pdu, int pduLen)
  {
    printf("1129-rach_dec");
#if 1
    std::cout << "rach_detected" << std::endl;
    for (int i = 0; i < pduLen; i++) // 测试上传的PDU是否正确
    {
      printf("mac bitset<8>(*(pdu->msg+i)):%x\n", (*(pdu + i)));
    }
#endif

    if (!prachInfo->crc)
    {
      printf("1206--rach crc FAIL\n");
      return -1;
    }
#if 1
    bool rach_ho_ind = false;
    uint8_t lcid = *(pdu+1)&0x1f;
    if (lcid == 9){
      std::cout << "received ho rach!"<<std::endl;
      rach_ho_ind = true;
      HO_RACH_COUNT = 0;
    }
#endif
    // for clear ue
    if ((ue_category == 14) || (rach_ho_ind == true))
    //if (ue_category == 14)
    {
      std::cout << "skip clear_ue!"<<std::endl;
    }
    else
    {
      clear_ue();
      HO_RACH_COUNT = 0;
    }
    judge_rach_detect_counter = true;
    std::cout << "mac_adp->udp_.mac_con_req_flag3:" << mac_adp->udp_.mac_con_req_flag << std::endl;
    std::cout << "mac_adp->udp_.mac_ss_flagzzzz:" << mac_adp->udp_.mac_ss_flag << std::endl;
    std::cout << "  mac_adp->udp_.mac_con_req_flag2   222:  " << mac_adp->udp_.mac_con_req_flag2 << std::endl;
    if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.mac_con_req_flag)
    {
      srsran::unique_byte_buffer_t mac_con_req_ttcn = srsran::make_byte_buffer();
      mac_con_req_ttcn->init();
      mac_con_req_ttcn->msg[7] = RRC_CONNECT_REQ;
      mac_con_req_ttcn->N_bytes = 9;
      printf("111111111准备发送连接建立请求消息\n");
      // generate_m2t_interface(&mac_con_req_ttcn,9);
      mac_con_req_ttcn->msg[0] = 0x01;
      mac_con_req_ttcn->msg[1] = 0x00;
      mac_con_req_ttcn->msg[2] = 0x00;
      mac_con_req_ttcn->msg[3] = 0x00;
      mac_con_req_ttcn->msg[4] = 0x00;
      mac_con_req_ttcn->msg[5] = 0x00;
      mac_con_req_ttcn->msg[6] = 0x09;
      mac_con_req_ttcn->msg[8] = 0x01;
      mac_adp->udp_.send_ttcn_info.try_push(std::move(mac_con_req_ttcn));
      std::cout << "111111send_ttcn_info size:" << mac_adp->udp_.send_ttcn_info.size() << std::endl;
    }
    // mac_case2
    if (rach_time == 0)
    {
      if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.mac_con_req_flag2)
      {
        srsran::unique_byte_buffer_t mac_con_req_ttcn_x = srsran::make_byte_buffer();
        mac_con_req_ttcn_x->init();
        mac_con_req_ttcn_x->msg[7] = RRC_CONNECT_REQ;
        mac_con_req_ttcn_x->N_bytes = 9;
        printf("22222222准备发送连接建立请求消息\n");
        // generate_m2t_interface(&mac_con_req_ttcn,9);
        mac_con_req_ttcn_x->msg[0] = 0x01;
        mac_con_req_ttcn_x->msg[1] = 0x00;
        mac_con_req_ttcn_x->msg[2] = 0x00;
        mac_con_req_ttcn_x->msg[3] = 0x00;
        mac_con_req_ttcn_x->msg[4] = 0x00;
        mac_con_req_ttcn_x->msg[5] = 0x00;
        mac_con_req_ttcn_x->msg[6] = 0x09;
        mac_con_req_ttcn_x->msg[8] = 0x01;
        mac_adp->udp_.send_ttcn_info.try_push(std::move(mac_con_req_ttcn_x));

        // printf("rach detected mac_adp->udp_.window_response_flag:%d\n",mac_adp->udp_.window_response_flag);
        // }
        rach_time++;
      }
    }
    else if (rach_time == 1 && mac_adp->udp_.is_second_send_rach_testcase2)
    {
      std::cout << "is_second_send_rach_testcase2" << std::endl;
      if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.mac_con_req_flag2)
      {
        srsran::unique_byte_buffer_t mac_con_req_ttcn_x = srsran::make_byte_buffer();
        mac_con_req_ttcn_x->init();
        mac_con_req_ttcn_x->msg[7] = RRC_CONNECT_REQ;
        mac_con_req_ttcn_x->N_bytes = 9;
        printf("22222222准备发送连接建立请求消息 2\n");
        // generate_m2t_interface(&mac_con_req_ttcn,9);
        mac_con_req_ttcn_x->msg[0] = 0x01;
        mac_con_req_ttcn_x->msg[1] = 0x00;
        mac_con_req_ttcn_x->msg[2] = 0x00;
        mac_con_req_ttcn_x->msg[3] = 0x00;
        mac_con_req_ttcn_x->msg[4] = 0x00;
        mac_con_req_ttcn_x->msg[5] = 0x00;
        mac_con_req_ttcn_x->msg[6] = 0x09;
        mac_con_req_ttcn_x->msg[8] = 0x01;
        mac_adp->udp_.send_ttcn_info.try_push(std::move(mac_con_req_ttcn_x));
      }
    }
    //************ test bi backoff***************    
    if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.TC_614_backoff)
    {
      test_bi_backoff++;
      srsran::unique_byte_buffer_t mac_con_req_ttcn_bi = srsran::make_byte_buffer();
      mac_con_req_ttcn_bi->init();
      mac_con_req_ttcn_bi->msg[7] = RRC_CONNECT_REQ;
      mac_con_req_ttcn_bi->N_bytes = 9;
      printf("bi test 准备发送连接建立请求消息\n");
      mac_con_req_ttcn_bi->msg[0] = 0x01;
      mac_con_req_ttcn_bi->msg[1] = 0x00;
      mac_con_req_ttcn_bi->msg[2] = 0x00;
      mac_con_req_ttcn_bi->msg[3] = 0x00;
      mac_con_req_ttcn_bi->msg[4] = 0x00;
      mac_con_req_ttcn_bi->msg[5] = 0x00;
      mac_con_req_ttcn_bi->msg[6] = 0x09;
      mac_con_req_ttcn_bi->msg[8] = 0x01;
      mac_adp->udp_.send_ttcn_info.try_push(std::move(mac_con_req_ttcn_bi));
      std::cout << "ptc614 send_ttcn_info size:" << mac_adp->udp_.send_ttcn_info.size() << std::endl;
      if (test_bi_backoff == 2)
      {
        test_bi_backoff = 0;
        mac_adp->udp_.TC_614_backoff = false;
      }
    }
    //************ test roid not match***************
    if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.TC_615_mac_roid_not_match)
    {
      tset_roid_not_match_count++;
      srsran::unique_byte_buffer_t mac_con_req_ttcn_roid = srsran::make_byte_buffer();
      mac_con_req_ttcn_roid->init();
      mac_con_req_ttcn_roid->msg[7] = RRC_CONNECT_REQ;
      mac_con_req_ttcn_roid->N_bytes = 9;
      printf("roid test 准备发送连接建立请求消息\n");
      // generate_m2t_interface(&mac_con_req_ttcn,9);
      mac_con_req_ttcn_roid->msg[0] = 0x01;
      mac_con_req_ttcn_roid->msg[1] = 0x00;
      mac_con_req_ttcn_roid->msg[2] = 0x00;
      mac_con_req_ttcn_roid->msg[3] = 0x00;
      mac_con_req_ttcn_roid->msg[4] = 0x00;
      mac_con_req_ttcn_roid->msg[5] = 0x00;
      mac_con_req_ttcn_roid->msg[6] = 0x09;
      mac_con_req_ttcn_roid->msg[8] = 0x01;
      mac_adp->udp_.send_ttcn_info.try_push(std::move(mac_con_req_ttcn_roid));
      std::cout << "111111send_ttcn_info size:" << mac_adp->udp_.send_ttcn_info.size() << std::endl;
      if (tset_roid_not_match_count == 2)
      {
        tset_roid_not_match_count = 0;
        mac_adp->udp_.TC_615_mac_roid_not_match = false;
      }
    }
    //************ test crid not match***************
    if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.TC_612_mac_crid_not_match)
    {
      tset_crid_not_match_count++;
      srsran::unique_byte_buffer_t mac_con_req_ttcn_crid = srsran::make_byte_buffer();
      mac_con_req_ttcn_crid->init();
      mac_con_req_ttcn_crid->msg[7] = RRC_CONNECT_REQ;
      mac_con_req_ttcn_crid->N_bytes = 9;
      printf("crid test 准备发送连接建立请求消息\n");
      // generate_m2t_interface(&mac_con_req_ttcn,9);
      mac_con_req_ttcn_crid->msg[0] = 0x01;
      mac_con_req_ttcn_crid->msg[1] = 0x00;
      mac_con_req_ttcn_crid->msg[2] = 0x00;
      mac_con_req_ttcn_crid->msg[3] = 0x00;
      mac_con_req_ttcn_crid->msg[4] = 0x00;
      mac_con_req_ttcn_crid->msg[5] = 0x00;
      mac_con_req_ttcn_crid->msg[6] = 0x09;
      mac_con_req_ttcn_crid->msg[8] = 0x01;
      mac_adp->udp_.send_ttcn_info.try_push(std::move(mac_con_req_ttcn_crid));
      std::cout << "111111send_ttcn_info size:" << mac_adp->udp_.send_ttcn_info.size() << std::endl;
      if (tset_crid_not_match_count == 2)
      {
        tset_crid_not_match_count = 0;
        mac_adp->udp_.TC_612_mac_crid_not_match = false;
      }
    }
    ////************ test ccch logic channel***************
    if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.TC_616_mac_ccch_logic_channel)
    {
      srsran::unique_byte_buffer_t mac_con_req_ttcn_ccch = srsran::make_byte_buffer();
      mac_con_req_ttcn_ccch->init();
      mac_con_req_ttcn_ccch->msg[7] = RRC_CONNECT_REQ;
      mac_con_req_ttcn_ccch->N_bytes = 9;
      printf("ccch logic channel 准备发送连接建立请求消息\n");
      // generate_m2t_interface(&mac_con_req_ttcn,9);
      mac_con_req_ttcn_ccch->msg[0] = 0x01;
      mac_con_req_ttcn_ccch->msg[1] = 0x00;
      mac_con_req_ttcn_ccch->msg[2] = 0x00;
      mac_con_req_ttcn_ccch->msg[3] = 0x00;
      mac_con_req_ttcn_ccch->msg[4] = 0x00;
      mac_con_req_ttcn_ccch->msg[5] = 0x00;
      mac_con_req_ttcn_ccch->msg[6] = 0x09;
      mac_con_req_ttcn_ccch->msg[8] = 0x01;
      mac_adp->udp_.send_ttcn_info.try_push(std::move(mac_con_req_ttcn_ccch));
      std::cout << "111111send_ttcn_info size:" << mac_adp->udp_.send_ttcn_info.size() << std::endl;
    }

   //*********TC619 start*******
    if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.TC_619_mac_bsr_timer_2)
    {
      if (msg[1]&0x1f == 0x0b )     //检查MAC头的UL LCID是否是short BSR  因为PRACH的第一个字节是beamID,第二个字节才是MAC头
        mac_adp->udp_.TC_619_Info_To_TTCN = true;
    }
    //*********TC619 end*******

    std::cout << " xxxxxxxxxxxxxxxxxxxxxxxxeeee 1" << std::endl;

    uint32_t ra_id = 0;

    uint8_t beam_id = *pdu;
    if(beam_id==2)
    {
      pduLen -= 1;
      pdu += 1;
    }
    else 
    {
      pduLen -= 2;
      pdu += 2;
    };
    //------------------wjj
    // 给RACH消息的RAR分配内存
    rarReset(0);
    while (pending_rars1[ra_id].temp_crnti != (uint16_t)-1 && ra_id < MAX_PENDING_RARS)
    {
      printf("ra_id++ ");
      ra_id++;
    }
    std::cout << " xxxxxxxxxxxxxxxxxxxxxxxxeeee 2" << std::endl;
    if (ra_id == MAX_PENDING_RARS)
    {
      // srsran::console("超出最大未处理RAR\n");
      return -1;
    }
    //std::cout << " xxxxxxxxxxxxxxxxxxxxxxxxeeee 3" << std::endl;
    //uint16_t rnti = allocate_ue(0);
    //mac_rnti = rnti;
#if 1
    uint16_t rnti = mac_rnti;
    if (rnti == 0){
      rnti = allocate_ue(0);
      mac_rnti = rnti;
    }
#endif
    srsran::console("创建UE,RNTI:%u,帧号：%u,RA_ID:%u\n", rnti, prachInfo->framID, ra_id);
    //std::cout << " xxxxxxxxxxxxxxxxxxxxxxxxeeee 4" << std::endl;
    // prachInfo->framID/=5;
    // map
    int n = 0;

    for (float i = -63.875; i <= 63.875; i += 0.125)
    {
      float a = (float)(int)(i * 1000) / 1000;
      prachTAhashMap[i] = n;
      n++;
    }
    int m = 0, k = 33;
    for (int j = 0; j <= 4960; j += 160)
    {
      prachFAhashMap[j] = m;
      m++;
    }
    for (int l = -160; l >= -4960; l -= 160)
    {
      prachFAhashMap[l] = k;
      k++;
    }
    if (prachInfo->ta > prach_time_toleranceMax)
    {
      prachTAhashMap[prachInfo->ta] = prachTAhashMap[prach_time_toleranceMax];
    }
    if (prachInfo->ta < prach_time_toleranceMin)
    {
      prachTAhashMap[prachInfo->ta] = prachTAhashMap[prach_time_toleranceMin];
    }
    if (prachInfo->fa > prach_freq_toleranceMax)
    {
      prachFAhashMap[prachInfo->fa] = prachFAhashMap[prach_freq_toleranceMax];
    }
    if (prachInfo->fa < prach_freq_toleranceMin)
    {
      prachFAhashMap[prachInfo->fa] = prachFAhashMap[prach_freq_toleranceMin];
    }
    std::cout << "prachPA:" << prachInfo->pa << std::endl;
    std::cout << "TA:" << prachInfo->ta << std::endl;
    std::cout << "FA:" << prachInfo->fa << std::endl;
    std::cout << "prachFAhashMap:" << prachFAhashMap[prachInfo->fa] << std::endl;
    std::cout << "prachTAhashMap:" << prachTAhashMap[prachInfo->ta] << std::endl;
    std::cout << "prach PAmap:" << PAmap[prachInfo->pa] << std::endl;
    // if(area_mode==0)
    // {

    pending_rars1[ra_id].frameID = prachInfo->framID;
    // pending_rars1[ra_id].ta         = prachInfo->ta;
    // std::cout<<"prachInfo->ta:"<<prachInfo->ta<<std::endl;
    // debug20240124
    pending_rars1[ra_id].ta = prachTAhashMap[prachInfo->ta];
    // pending_rars1[ra_id].ta         = 511;
    pending_rars1[ra_id].fa = prachFAhashMap[prachInfo->fa];
    pending_rars1[ra_id].pa = PAmap[prachInfo->pa];
    pending_rars1[ra_id].temp_crnti = rnti;
    // pending_rars1[ra_id].is_enable=true;
    pending_rars1[ra_id].considInfo.tti = prachInfo->framID;
    pending_rars1[ra_id].considInfo.rnti = rnti;
    pending_rars1[ra_id].subHead.roid = prachInfo->roid;

    // printf( "--------------------wx_mac rach detected------------------\n");
    printf("--------------------pending_rars1[ra_id].frameID:%d\n", pending_rars1[ra_id].frameID);
    printf("--------------------pending_rars1[ra_id].ta:%d\n", pending_rars1[ra_id].ta);
    printf("--------------------pending_rars1[ra_id].fa:%d\n", pending_rars1[ra_id].fa);
    printf("--------------------pending_rars1[ra_id].pa:%d\n", pending_rars1[ra_id].pa);
    printf("--------------------pending_rars1[ra_id].temp_crnti:%d\n", pending_rars1[ra_id].temp_crnti);
    printf("--------------------pending_rars1[ra_id].subHead.roid:%d\n", pending_rars1[ra_id].subHead.roid);
    // }else if(area_mode==1){
    //   pending_rars1[ra_id].frameID    = prachInfo->framID;
    // //pending_rars1[ra_id].ta         = prachInfo->ta;
    // std::cout<<"prachInfo->ta:"<<prachInfo->ta<<std::endl;
    // pending_rars1[ra_id].ta         = prachTAhashMap[prachInfo->ta];
    // pending_rars1[ra_id].fa         = prachFAhashMap[prachInfo->fa];
    // pending_rars1[ra_id].pa         = PAmap[prachInfo->pa];
    // pending_rars1[ra_id].temp_crnti = rnti;
    // // pending_rars1[ra_id].is_enable=true;
    // pending_rars1[ra_id].considInfo.tti  = prachInfo->framID;
    // pending_rars1[ra_id].considInfo.rnti = rnti;
    // pending_rars1[ra_id].subHead.roid    = prachInfo->roid;


     //2024/8/16------------wwh-----------
    handover_info.ta=prachTAhashMap[prachInfo->ta];
    handover_info.pa=PAmap[prachInfo->pa];
    handover_info.fa=prachFAhashMap[prachInfo->fa];
    //handover_info.fa = 0;
    handover_info.roid=prachInfo->roid;
    //----------------------------------------------------------

    // }
    #if 0
    // Add new user to the scheduler so that it can RX/TX SRB0
    sched_interface::ue_cfg_t uecfg = {};
    uecfg.supported_cc_list.emplace_back();
    uecfg.supported_cc_list.back().active = true;
    uecfg.supported_cc_list.back().enb_cc_idx = 0;
    uecfg.ue_bearers[0].direction = mac_lc_ch_cfg_t::BOTH;
    uecfg.supported_cc_list[0].dl_cfg.tm = SRSRAN_TM1;
    if (ue_cfg(rnti, &uecfg) != SRSRAN_SUCCESS)
    {
      printf("ue_cfg error!\n");
      return -1;
    }
    // 在RRC层注册新用户
    //printf("add user in rrc, beamid:%d\n",beam_id);
    if (rrc_h->add_user(rnti, uecfg) == SRSRAN_ERROR)
    {
      std::cout << "add_user flase" << std::endl;
      ue_rem(rnti);
      return -1;
    }
    #else
    if (rach_ho_ind == false){
      // Add new user to the scheduler so that it can RX/TX SRB0
      sched_interface::ue_cfg_t uecfg = {};
      uecfg.supported_cc_list.emplace_back();
      uecfg.supported_cc_list.back().active = true;
      uecfg.supported_cc_list.back().enb_cc_idx = 0;
      uecfg.ue_bearers[0].direction = mac_lc_ch_cfg_t::BOTH;
      uecfg.supported_cc_list[0].dl_cfg.tm = SRSRAN_TM1;
      if (ue_cfg(rnti, &uecfg) != SRSRAN_SUCCESS)
      {
        printf("ue_cfg error!\n");
        return -1;
      }
      // 在RRC层注册新用户   
      //std::cout << "add user in rrc, beamid:" << beam_id << std::endl;
      printf("add user in rrc, beamid:%d\n",beam_id);
      if (rrc_h->add_user(rnti, uecfg) == SRSRAN_ERROR)
      {
        std::cout << "add_user flase" << std::endl;
        ue_rem(rnti);
        return -1;
      }
    }
    #endif
    push_pdu(prachInfo->framID, rnti, pdu, pduLen, prachInfo->crc, 20); // 上传PDU
    return 0;
  }
  //****************************************

  // parse_psych_pdu
  int mac::parse_psych_pdu(uint32_t tti_rx, uint16_t rnti, uint8_t *msg, uint32_t pduLen, bool crc)
  {
    // std::cout<<"@@@@@@@@@@@@@@"<<std::endl;
    if (crc)
    {
      // for(uint8_t i=0;i<pduLen;i++)
      // {
      //   std::cout<<"@@@@@push_pdu:"<<std::endl;
      //   	printf(" msg[%d] = %02X\n",  i, msg[i]);
      // }
    }
    // psych_pdu psych;
    psych.CI = msg[0] & 0xc0;
    psych.LCID = msg[0] & 0x1f;
    psych.CQI = msg[1] & 0xf0;
    psych.PA = msg[1] & 0x0e;
    printf(" psych.CI= %02X\n", psych.CI);
    printf(" psych.LCID= %02X\n", psych.LCID);
    printf(" psych.CQI= %02X\n", psych.CQI);
    printf(" psych.PA= %02X\n", psych.PA);
    return 0;
  }

  int mac::push_pdu(uint32_t tti_rx, uint16_t rnti, uint8_t *msg, uint32_t pduLen, bool crc, uint8_t voice)
  {

    std::cout << "[PUSH PDU[PID]=" << Pid << std::endl;

    // std::cout << " @@@@@ push_pdu " << std::endl;
    //  if(crc==false){
    //    ++crc_check_failure_counter;
    //    std::cout<<" crc_check_failure_counter = "<<crc_check_failure_counter<<std::endl;
    //    logger.debug("[WX][PUSH][PDU]CRC = false counter:%d",crc_check_failure_counter);
    //  }
    //  if(crc_check_failure_counter==20){
    //    crc_check_failure_counter=0;//reset
    //    rrc_h->Notify_rrc_to_notify_nas_to_release(rnti);
    //  }

    logger.debug("[WX][PUSH][PDU]VOICE_TYPE:%d", voice);
    if (area_mode == 0 && dlAlloc.Type != pdch1_1 && dlAlloc.Type != pdch1_2)
    {
      voice = 0;
    }
    // if()

    // if (crc)
    // {
    //   for (uint8_t i = 0; i < pduLen; i++)
    //   {
    //     std::cout << "@@@@@push_pdu:" << std::endl;
    //     printf(" msg[%d] = %02X\n", i, msg[i]);
    //   }
    // }

    // std::cout << "mac_adp->udp_.mac_ss_flag:" << mac_adp->udp_.mac_ss_flag << std::endl;
    // std::cout << "mac_adp->udp_.mac_push_pdu22:" << mac_adp->udp_.mac_push_pdu << std::endl;

    //****************************3/22**********
    // if(mac_adp->udp_.TC_6115_Info_To_TTCN){
    //     mac_adp->udp_.TC_6115_Info_To_TTCN=false;
    //     srsran::unique_byte_buffer_t TC_6115_Info = srsran::make_byte_buffer();
    //     TC_6115_Info->N_bytes=10;
    //     TC_6115_Info->msg[1]=;
    //     TC_6115_Info->msg[2]=;
    //     TC_6115_Info->msg[3]=;
    //     TC_6115_Info->msg[4]=;
    //     TC_6115_Info->msg[5]=;
    //     TC_6115_Info->msg[6]=;
    //     TC_6115_Info->msg[7]=;
    //     TC_6115_Info->msg[8]=;
    //     TC_6115_Info->msg[9]=;
    //     TC_6115_Info->msg[10]=;
    // }
    // if (mac_adp->udp_.TC_6115_Info_To_TTCN == true)
    // {
    //   std::cout<<"@@@@@@XXXXXX"<<std::endl;
    //   mac_adp->udp_.TC_6115_Info_To_TTCN = false;
    //   srsran::unique_byte_buffer_t TC_6115_Info = srsran::make_byte_buffer();
    //   TC_6115_Info->N_bytes = 10;
    //   TC_6115_Info->msg[1] = 0x01;
    //   TC_6115_Info->msg[2] = 0x06;
    //   TC_6115_Info->msg[3] = 0x00;
    //   TC_6115_Info->msg[4] = 0x17;
    //   TC_6115_Info->msg[5] = 0xe3;
    //   TC_6115_Info->msg[6] = 0x00;
    //   TC_6115_Info->msg[7] = 0x02;
    //   TC_6115_Info->msg[8] = 0x00;
    //   TC_6115_Info->msg[9] = 0x00;
    //   TC_6115_Info->msg[10] = 0x00;
    //    mac_adp->udp_.send_ttcn_info.try_push(std::move(TC_6115_Info));
    // }
    if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.mac_push_pdu)
    {
      srsran::unique_byte_buffer_t mac_con_com_ttcn = srsran::make_byte_buffer();
      mac_con_com_ttcn->init();
      mac_con_com_ttcn->msg[7] = RRC_CONNECT_COM;
      mac_con_com_ttcn->N_bytes = 8;
      printf("准备发送连接建立完成消息\n");
      // generate_m2t_interface(&mac_con_com_ttcn,8);//这个函数是生成前面七个字节的
      mac_con_com_ttcn->msg[0] = 0x01;
      mac_con_com_ttcn->msg[1] = 0x00;
      mac_con_com_ttcn->msg[2] = 0x00;
      mac_con_com_ttcn->msg[3] = 0x00;
      mac_con_com_ttcn->msg[4] = 0x00;
      mac_con_com_ttcn->msg[5] = 0x00;
      mac_con_com_ttcn->msg[6] = 0x08;
      mac_adp->udp_.send_ttcn_info.try_push(std::move(mac_con_com_ttcn));
      std::cout << "连接建立完成send_ttcn_info size:" << mac_adp->udp_.send_ttcn_info.size() << std::endl;

      mac_adp->udp_.mac_push_pdu = false;
    }

    //*********test ccch logic channel*********
    if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.TC_616_mac_push_pdu)
    {
      srsran::unique_byte_buffer_t mac_con_com_ttcn_ccch = srsran::make_byte_buffer();
      mac_con_com_ttcn_ccch->init();
      mac_con_com_ttcn_ccch->msg[7] = RRC_CONNECT_COM;
      mac_con_com_ttcn_ccch->N_bytes = 8;
      printf("ccch  test   准备发送连接建立完成消息\n");
      // generate_m2t_interface(&mac_con_com_ttcn,8);//这个函数是生成前面七个字节的
      mac_con_com_ttcn_ccch->msg[0] = 0x01;
      mac_con_com_ttcn_ccch->msg[1] = 0x00;
      mac_con_com_ttcn_ccch->msg[2] = 0x00;
      mac_con_com_ttcn_ccch->msg[3] = 0x00;
      mac_con_com_ttcn_ccch->msg[4] = 0x00;
      mac_con_com_ttcn_ccch->msg[5] = 0x00;
      mac_con_com_ttcn_ccch->msg[6] = 0x08;
      mac_adp->udp_.send_ttcn_info.try_push(std::move(mac_con_com_ttcn_ccch));
      std::cout << "连接建立完成send_ttcn_info size:" << mac_adp->udp_.send_ttcn_info.size() << std::endl;

      mac_adp->udp_.TC_616_mac_push_pdu = false;
    }
    //*********test 07-16*********
    /*if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.TC_6115_mac_srnti_match)
    {
      srsran::unique_byte_buffer_t mac_srnti_match_ttcn = srsran::make_byte_buffer();
      mac_srnti_match_ttcn->init();
      mac_srnti_match_ttcn->msg[7] = MAC_ul_data_req;
      mac_srnti_match_ttcn->N_bytes = 8;
      printf("准备发送连接建立完成消息\n");
      // generate_m2t_interface(&mac_con_com_ttcn,8);//这个函数是生成前面七个字节的
      mac_srnti_match_ttcn->msg[0] = 0x01;
      mac_srnti_match_ttcn->msg[1] = 0x00;
      mac_srnti_match_ttcn->msg[2] = 0x00;
      mac_srnti_match_ttcn->msg[3] = 0x00;
      mac_srnti_match_ttcn->msg[4] = 0x00;
      mac_srnti_match_ttcn->msg[5] = 0x00;
      mac_srnti_match_ttcn->msg[6] = 0x08;
      mac_srnti_match_ttcn->msg[8] = 0x01;
      mac_adp->udp_.send_ttcn_info.try_push(std::move(mac_srnti_match_ttcn));
      std::cout << "连接建立完成send_ttcn_info size:" << mac_adp->udp_.send_ttcn_info.size() << std::endl;

      mac_adp->udp_.TC_6115_mac_srnti_match = false;
    }*/

   //*********TC618 start*******
    if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.TC_618_mac_regular_pdu_2)
    {
      for (uint32_t i = 0; i < pduLen; i++)
      {
        printf("0xup6114:%x\n", msg[i]);
      }
      if (msg[0]&0x1f == 0x0b )     //检查MAC头的UL LCID是否是short BSR
        mac_adp->udp_.TC_618_Info_To_TTCN = true;
    }
    //*********TC618 end******* 

   //*********TC6111 start*******
    if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.TC_6111_mac_periodic_phr_timer && (msg[0] & 0x1f) == 15) //测试步骤3  //第一个MAC子头的LCID是否是PHR MAC CE
    {
      srsran::unique_byte_buffer_t TC_6111_Info = srsran::make_byte_buffer();
      TC_6111_Info->N_bytes = 10;
      TC_6111_Info->msg[0] = 0x01;
      TC_6111_Info->msg[1] = 0x06;
      TC_6111_Info->msg[2] = 0x00;
      TC_6111_Info->msg[3] = 0x17; //test id
      TC_6111_Info->msg[4] = 0xdf; //test id
      TC_6111_Info->msg[5] = 0x00;
      TC_6111_Info->msg[6] = 0x02;
      TC_6111_Info->msg[7] = 0x00;
      TC_6111_Info->msg[8] = 0x0f;   
      TC_6111_Info->msg[9] = sdu->msg[2];
      mac_adp->udp_.send_ttcn_info.try_push(std::move(TC_6111_Info));

      mac_adp->udp_.TC_6111_mac_periodic_phr_timer = false;
      mac_adp->udp_.TC_6111_mac_periodic_phr_timer_2 = true;
    }

    if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.TC_6111_mac_periodic_phr_timer_2 && (msg[0] & 0x1f) == 15) //测试步骤5 //第一个MAC子头的LCID是否是PHR MAC CE
    {
      srsran::unique_byte_buffer_t TC_6111_Info = srsran::make_byte_buffer();
      TC_6111_Info->N_bytes = 10;
      TC_6111_Info->msg[0] = 0x01;
      TC_6111_Info->msg[1] = 0x06;
      TC_6111_Info->msg[2] = 0x00;
      TC_6111_Info->msg[3] = 0x17; //test id
      TC_6111_Info->msg[4] = 0xdf; //test id
      TC_6111_Info->msg[5] = 0x00;
      TC_6111_Info->msg[6] = 0x02;
      TC_6111_Info->msg[7] = 0x00;
      TC_6111_Info->msg[8] = 0x0f;   
      TC_6111_Info->msg[9] = sdu->msg[2];
      mac_adp->udp_.send_ttcn_info.try_push(std::move(TC_6111_Info));

      mac_adp->udp_.TC_6111_mac_periodic_phr_timer_2 = false;
    }
    //*********TC6111 end******* 

    //*********TC6112 start*******
    if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.TC_6112_mac_pass_loss && (msg[0] & 0x1f) == 15) //第一个MAC子头的LCID是否是PHR MAC CE
    {
      srsran::unique_byte_buffer_t TC_6112_Info = srsran::make_byte_buffer();
      TC_6112_Info->N_bytes = 10;
      TC_6112_Info->msg[0] = 0x01;
      TC_6112_Info->msg[1] = 0x06;
      TC_6112_Info->msg[2] = 0x00;
      TC_6112_Info->msg[3] = 0x17; //test id
      TC_6112_Info->msg[4] = 0xdf; //test id
      TC_6112_Info->msg[5] = 0x00;
      TC_6112_Info->msg[6] = 0x02;
      TC_6112_Info->msg[7] = 0x00;
      TC_6112_Info->msg[8] = 0x0f; //lcid
      TC_6112_Info->msg[9] = sdu->msg[2];
      mac_adp->udp_.send_ttcn_info.try_push(std::move(TC_6112_Info));

      mac_adp->udp_.TC_6112_mac_pass_loss = false;
    }
    //*********TC6112 end*******

    if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.TC_6113_padding_dl)
    {
      for (uint32_t i = 0; i < pduLen; i++)
      {
        printf("0xup6113:%x\n", msg[i]);
      }
      if ((msg[0] == 0x63) && (msg[2] == 0x1f) && (msg[7] == 0xbb))
        mac_adp->udp_.TC_6113_Info_To_TTCN = true;
    }

    //*********TC6114 start*******
    if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.TC_6114_process_subheader_dl)
    {
      for (uint32_t i = 0; i < pduLen; i++)
      {
        printf("0xup6114:%x\n", msg[i]);
      }
      if ((msg[0] == 0x63) && (msg[2] == 0x1f) )  //有填充子头
        mac_adp->udp_.TC_6114_Info_To_TTCN = true;
    }
    //*********TC6114 end*******

    //**********************************
    if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.TC_6118_mac_push_pdu && (msg[1] & 0x1f) == 13)
    {
      mac_adp->udp_.TC_6118_mac_push_pdu = false;
      srsran::unique_byte_buffer_t mac_ttcn_ho_response = srsran::make_byte_buffer();
      mac_ttcn_ho_response->init();
      mac_ttcn_ho_response->N_bytes = 11;
      mac_ttcn_ho_response->msg[0] = 0x01;
      mac_ttcn_ho_response->msg[1] = 0x00;
      mac_ttcn_ho_response->msg[2] = 0x00;
      mac_ttcn_ho_response->msg[3] = 0x00;
      mac_ttcn_ho_response->msg[4] = 0x00;
      mac_ttcn_ho_response->msg[5] = 0x00;
      mac_ttcn_ho_response->msg[6] = 0x0b;
      mac_ttcn_ho_response->msg[7] = 0x00;
      mac_ttcn_ho_response->msg[8] = msg[1] & 0x1f; // 取子头的LCID
      mac_adp->udp_.send_ttcn_info.try_push(std::move(mac_ttcn_ho_response));
      std::cout << "完成mac_ttcn_ho_response:" << mac_adp->udp_.send_ttcn_info.size() << std::endl;

      wxScheduler.deallocate_source(dlAlloc);

      ul_allocate ulAlloc_;
      ulAlloc_.Type = pdch1_1;
      ulAlloc_.LogicType = SCH2;
      ulAlloc_.bandID = 9;
      ulAlloc_.freq = 1;
      ulAlloc_.solt = area_mode == 0 ? 2 : 1;

      configMap TC6118_updateMap;

      updateMap(3, TC6118_updateMap);
      update_ChanType(pdch1_1, pdch1_1);
      phy_h->set_parameters(ulAlloc_.bandID, ulAlloc_.freq, ulAlloc_.solt, 0);
      mac_adp->udp_.TC_6118_mac_push_pdu_2 = true;
    }
    else if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.TC_6118_mac_push_pdu_2 && (msg[1] & 0x1f) == 14)
    {
      srsran::unique_byte_buffer_t mac_ttcn_ho_complete = srsran::make_byte_buffer();
      mac_ttcn_ho_complete->init();
      mac_ttcn_ho_complete->N_bytes = 11;
      mac_ttcn_ho_complete->msg[0] = 0x01;
      mac_ttcn_ho_complete->msg[1] = 0x00;
      mac_ttcn_ho_complete->msg[2] = 0x00;
      mac_ttcn_ho_complete->msg[3] = 0x00;
      mac_ttcn_ho_complete->msg[4] = 0x00;
      mac_ttcn_ho_complete->msg[5] = 0x00;
      mac_ttcn_ho_complete->msg[6] = 0x0b;
      mac_ttcn_ho_complete->msg[7] = 0x00;
      mac_ttcn_ho_complete->msg[8] = msg[1] & 0x1f; // 取子头的LCID
      mac_adp->udp_.send_ttcn_info.try_push(std::move(mac_ttcn_ho_complete));
      std::cout << "完成mac_ttcn_ho_complete:" << mac_adp->udp_.send_ttcn_info.size() << std::endl;

      mac_adp->udp_.TC_6118_mac_push_pdu_2 = false;
    }

    //*********TC61119 start*******
    else if (mac_adp->udp_.mac_ss_flag && mac_adp->udp_.TC_6119_ul_mcs_2)
    {
      uint8_t part1 = msg[0] & 0x03;
      uint8_t part2 = (msg[1] & 0x80) >> 7;
      uint8_t pui_mcs = (part1 << 1) | part2; //从PUI中提取出MCS字段
      if(pui_mcs == 3)   // 3是之前直接配的，如果要改值，这边也要改，测试例测得就是MCS的值和配的一样
      {
        mac_adp->udp_.TC_6119_ul_mcs_2 = false;
        mac_adp->udp_.TC_6119_Info_To_TTCN = true;
      }
    }    
    //*********TC61119 end*******

    srsran::rwlock_read_guard lock(rwlock);
    // std::cout<<"@@@@@rnti"<<rnti<<std::endl;
    if (not check_ue_active(mac_rnti))
    {
      std::cout << "rnti" << rnti << std::endl;
      return SRSRAN_ERROR;
    }

    std::string result;
    CONVERT_PDU_TO_HEX(msg, pduLen, result);
    logger.debug("[WX][PUSH][PDU]MSG:%s", result);

    srsran::unique_byte_buffer_t pdu = srsran::make_byte_buffer(); // 为上行MAC PDU分配内存

    memcpy(pdu->msg, msg, pduLen);
    pdu->N_bytes = pduLen;

    // // 11.08
    // std::cout << "@@@@@pduLen" << pduLen << std::endl;
    // for (uint8_t i = 0; i <10; i++)
    // {
    //   printf(" -----msg[%d] = %02X\n", i, msg[i]);
    // }

    if (pdu == nullptr)
    {
      logger.warning("Could not find MAC UL PDU for rnti=0x%x, cc=%d, tti=%d", mac_rnti, 0, tti_rx);
      return SRSRAN_ERROR;
    }

    // push the pdu through the queue if received correctly
    if (crc)
    {
      if (voice == 2)
      {
        std::cout << "---VOICE MODE---" << std::endl;
        std::cout << " VOICE LCID = " << voiceLcid << std::endl;
        std::string result;
        CONVERT_PDU_TO_HEX(pdu->msg, pdu->N_bytes, result);
        MAC_UL_VOICE_INFO_LOG(voiceLcid, result);
        rlc_h->write_pdu(rnti,
                         voiceLcid, //  tm mode voice
                         pdu->msg,
                         pdu->N_bytes);
        return SRSRAN_SUCCESS;
      }
      if (voice == 20)
      {
        std::cout << "---rach indication---" << std::endl;
        ue_db[mac_rnti]->process_pdu(std::move(pdu), 0, 0);
        
        is_handover_rach=ue_db[mac_rnti]->get_handover_para()->is_handover_falg;
        printf("jjc_is_handover_rach=%d\n",is_handover_rach);
        return SRSRAN_SUCCESS;
      }
      printf("6666666666666666push_pdu233333333333333\n");
      // std::cout<<"enter crc"<<std::endl;
      logger.info("Pushing PDU mac_rnti=0x%x, tti_rx=%d, nof_bytes=%d", mac_rnti, tti_rx, pduLen);
      srsran_expect(pduLen == pdu->N_bytes,
                    "Inconsistent PDU length for mac_rnti=0x%x, tti_rx=%d (%d!=%d)",
                    mac_rnti,
                    tti_rx,
                    pduLen,
                    (int)pdu->size());
      // 将处理MAC PDU推入队列
      uint32_t rnti = mac_rnti;
      auto process_pdu_task = [this, rnti](srsran::unique_byte_buffer_t &pdu)
      {
        srsran::rwlock_read_guard lock(rwlock);
        if (check_ue_active(mac_rnti))
        {
          std::cout << "enter mac_rnti" << mac_rnti << std::endl;

          // for (uint32_t i = 0; i < pdu->N_bytes; i++)
          // {
          //   printf("7170x:%x\n", *(pdu->msg + i));
          // }

          ue_db[mac_rnti]->process_pdu(std::move(pdu), 0, 0);
        }
        else
        {
          logger.debug("Discarding PDU mac_rnti=0x%x", mac_rnti);
        }
      };
      stack_task_queue.try_push(std::bind(process_pdu_task, std::move(pdu)));
    }
    else
    {
      logger.debug("Discarding PDU mac_rnti=0x%x, tti_rx=%d, nof_bytes=%d", mac_rnti, tti_rx, pduLen);
    }
    return SRSRAN_SUCCESS;
  }
  bool mac::reestablish_test()
  {
    wxScheduler.deallocate_source(dlAlloc);
    return true;
  }
} // namespace srsenb
