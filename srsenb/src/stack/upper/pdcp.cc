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

#include "srsenb/hdr/stack/upper/pdcp.h"
#include "srsenb/hdr/common/common_enb.h"
#include "srsran/interfaces/enb_gtpu_interfaces.h"
#include "srsran/interfaces/enb_rlc_interfaces.h"
#include "srsran/interfaces/enb_rrc_interfaces.h"
#include "srsenb/hdr/adp.h"

namespace srsenb
{

  pdcp::pdcp(srsran::task_sched_handle task_sched_, srslog::basic_logger &logger_) : task_sched(task_sched_), logger(logger_)
  {
  }

  void pdcp::init(rlc_interface_pdcp *rlc_, rrc_interface_pdcp *rrc_,
                  const bool ttcn_pdcp_enble_, gtpu_interface_pdcp *gtpu_, adp *pdcp_adp_)
  {
    rlc = rlc_;
    rrc = rrc_;
    gtpu = gtpu_;
    ttcn_pdcp_enble = ttcn_pdcp_enble_;
    // this->pdcp_adp = pdcp_adp_;
    pdcp_adp = pdcp_adp_;
    // if(pdcp_adp->udp_.sdap_nhdr_transport == true)
    // {
    //   sdap_ttcn =1;
    // }
  }

  void pdcp::stop()
  {
    for (std::map<uint32_t, user_interface>::iterator iter = users.begin(); iter != users.end(); ++iter)
    {
      clear_user(&iter->second);
    }
    users.clear();
  }

  void pdcp::add_user(uint16_t rnti)
  {

    if (users.count(rnti) == 0)
    {
      unique_rnti_ptr<srsran::pdcp> obj = make_rnti_obj<srsran::pdcp>(rnti, task_sched, logger.id().c_str());
      obj->init(&users[rnti].rlc_itf, &users[rnti].rrc_itf, &users[rnti].gtpu_itf);
      users[rnti].rlc_itf.rnti = rnti;
      users[rnti].gtpu_itf.rnti = rnti;
      users[rnti].rrc_itf.rnti = rnti;

      users[rnti].rrc_itf.rrc = rrc;
      users[rnti].rlc_itf.rlc = rlc;
      users[rnti].gtpu_itf.gtpu = gtpu;
      users[rnti].pdcp = std::move(obj);
    }
  }

  // Private unlocked deallocation of user
  void pdcp::clear_user(user_interface *ue)
  {
    ue->pdcp->stop();
    ue->pdcp.reset();
  }

  void pdcp::rem_user(uint16_t rnti)
  {
    if (users.count(rnti))
    {
      clear_user(&users[rnti]);
      users.erase(rnti);
    }
  }
  //--------------------------------2023.10.20-----------------------------------
  void pdcp::add_bearerer(uint16_t rnti, uint32_t lcid, const srsran::pdcp_confg_t &cfg)
  {
    if (users.count(rnti))
    {
      if (rnti != SRSRAN_MRNTI)
      {
        users[rnti].pdcp->add_bearer(lcid, cfg);
      }
      else
      {
        users[rnti].pdcp->add_bearer_mrb(lcid, cfg);
      }
    }
  }
  //---------------------------------------------------
  void pdcp::add_bearer(uint16_t rnti, uint32_t lcid, const srsran::pdcp_config_t &cfg)
  {
    if (users.count(rnti))
    {
      if (rnti != SRSRAN_MRNTI)
      {
        //-----------------------------IOT-----------------------  //传递标志到srsRAN域中
        users[rnti].pdcp->Net_mode = pdcp_network_mode;
        //-------------------------------------------------------
        users[rnti].pdcp->add_bearer(lcid, cfg);
      }
      else
      {
        users[rnti].pdcp->add_bearer_mrb(lcid, cfg);
      }
    }
  }

  void pdcp::del_bearer(uint16_t rnti, uint32_t lcid)
  {
    if (users.count(rnti))
    {
      users[rnti].pdcp->del_bearer(lcid);
    }
  }

  void pdcp::set_enabled(uint16_t rnti, uint32_t lcid, bool enabled)
  {
    if (users.count(rnti))
    {
      users[rnti].pdcp->set_enabled(lcid, enabled);
    }
  }

  void pdcp::reset(uint16_t rnti)
  {
    if (users.count(rnti))
    {
      users[rnti].pdcp->reset();
    }
  }

  void pdcp::config_security(uint16_t rnti, uint32_t lcid, const srsran::as_security_config_t &sec_cfg)
  {
    if (users.count(rnti))
    {
      users[rnti].pdcp->config_security(lcid, sec_cfg);
    }
  }

  void pdcp::enable_integrity(uint16_t rnti, uint32_t lcid)
  {
    users[rnti].pdcp->enable_integrity(lcid, srsran::DIRECTION_TXRX);
  }

  void pdcp::enable_encryption(uint16_t rnti, uint32_t lcid)
  {
    users[rnti].pdcp->enable_encryption(lcid, srsran::DIRECTION_TXRX);
  }

  bool pdcp::get_bearer_state(uint16_t rnti, uint32_t lcid, srsran::pdcp_lte_state_t *state)
  {
    if (users.count(rnti) == 0)
    {
      return false;
    }
    return users[rnti].pdcp->get_bearer_state(lcid, state);
  }

  bool pdcp::set_bearer_state(uint16_t rnti, uint32_t lcid, const srsran::pdcp_lte_state_t &state)
  {
    if (users.count(rnti) == 0)
    {
      return false;
    }
    return users[rnti].pdcp->set_bearer_state(lcid, state);
  }

  void pdcp::reestablish(uint16_t rnti)
  {
    if (users.count(rnti) == 0)
    {
      return;
    }
    users[rnti].pdcp->reestablish();
  }

  void pdcp::send_status_report(uint16_t rnti)
  {
    if (users.count(rnti) == 0)
    {
      return;
    }
    users[rnti].pdcp->send_status_report();
  }

  void pdcp::notify_delivery(uint16_t rnti, uint32_t lcid, const srsran::pdcp_sn_vector_t &pdcp_sns)
  {
    if (users.count(rnti))
    {
      users[rnti].pdcp->notify_delivery(lcid, pdcp_sns);
    }
  }

  void pdcp::notify_failure(uint16_t rnti, uint32_t lcid, const srsran::pdcp_sn_vector_t &pdcp_sns)
  {
    if (users.count(rnti))
    {
      users[rnti].pdcp->notify_failure(lcid, pdcp_sns);
    }
  }

  void pdcp::write_sdu(uint16_t rnti, uint32_t lcid, srsran::unique_byte_buffer_t sdu, int pdcp_sn)
  {
    std::cout << "@@@@@@@@@@@@@------------pdcp-write-sdu-------------@@@@@@@@@@" << std::endl;
    // for(uint32_t i=0;i<sdu->N_bytes;i++)
    // {
    //   printf("0x%x\n",*(sdu->msg+i));
    // }

    if (users.count(rnti))
    {
      if (rnti != SRSRAN_MRNTI)
      {
        // TODO: Handle PDCP SN coming from GTPU
        users[rnti].pdcp->write_sdu(lcid, std::move(sdu), pdcp_sn);
      }
      else
      {
        users[rnti].pdcp->write_sdu_mch(lcid, std::move(sdu));
      }
    }
  }

  void pdcp::send_status_report(uint16_t rnti, uint32_t lcid)
  {
    if (users.count(rnti))
    {
      users[rnti].pdcp->send_status_report(lcid);
    }
  }

  std::map<uint32_t, srsran::unique_byte_buffer_t> pdcp::get_buffered_pdus(uint16_t rnti, uint32_t lcid)
  {
    if (users.count(rnti))
    {
      return users[rnti].pdcp->get_buffered_pdus(lcid);
    }
    return {};
  }

  void pdcp::write_pdu(uint16_t rnti, uint32_t lcid, srsran::unique_byte_buffer_t sdu)
  {
    /*
    0x:64
    0x:26
    0x:20
    0x:f
    0x:92
    0x:f9
    0x:8a
    0x:37
    0x:f8
    0x:61
    0x:23
    0x:3a
    0x:a0
    0x:26
    0x:3c
    0x:1c
    0x:9e
    0x:9e
    0x:c8
    0x:1f
    0x:69
    0x:30
    0x:8d
    0x:30
    0x:3f
    0x:4
    0x:c2
    0x:f
    0x:2
    0x:9a
    0x:52
    0x:ec
    0x:71
    0x:79
    0x:18
    0x:4e*/

#if 1
    // std::cout << " PDCP LCID = " << lcid << std::endl;

    // for (uint32_t i = 0; i < sdu->N_bytes; i++)
    // {
    //   printf(" pdcp msg[%d] = %0x\n", i, *(sdu->msg + i));
    // }
#endif

#if 0
  srsran::unique_byte_buffer_t sdu_test=srsran::make_byte_buffer();
  sdu_test->N_bytes=36;
  
  sdu_test->msg[0]=0x64;
  sdu_test->msg[1]=0x26;
  sdu_test->msg[2]=0x20;
  sdu_test->msg[3]=0x0f;
  sdu_test->msg[4]=0x92;
  sdu_test->msg[5]=0xf9;
  sdu_test->msg[6]=0x8a;
  sdu_test->msg[7]=0x37;
  sdu_test->msg[8]=0xf8;
  sdu_test->msg[9]=0x61;
  sdu_test->msg[10]=0x23;
  sdu_test->msg[11]=0x3a;
  sdu_test->msg[12]=0xa0;
  sdu_test->msg[13]=0x26;
  sdu_test->msg[14]=0x3c;
  sdu_test->msg[15]=0x1c;
  sdu_test->msg[16]=0x9e;
  sdu_test->msg[17]=0x9e;
  sdu_test->msg[18]=0xc8;
  sdu_test->msg[19]=0x1f;
  sdu_test->msg[20]=0x69;
  sdu_test->msg[21]=0x30;
  sdu_test->msg[22]=0x8d;
  sdu_test->msg[23]=0x30;
  sdu_test->msg[24]=0x3f;
  sdu_test->msg[25]=0x04;
  sdu_test->msg[26]=0xc2;
  sdu_test->msg[27]=0x0f;
  sdu_test->msg[28]=0x02;
  sdu_test->msg[29]=0x9a;
  sdu_test->msg[30]=0x52;
  sdu_test->msg[31]=0xec;
  sdu_test->msg[32]=0x71;
  sdu_test->msg[33]=0x79;
  sdu_test->msg[34]=0x18;
  sdu_test->msg[35]=0x4e;
#endif
    std::cout << " zhj test lcid = " << lcid << std::endl;
#if 0 // zhj psch test
  if(lcid==3){
      sdu->msg[1]+=1;
     std::cout<<" zhj psch test "<<std::endl;
     for(uint32_t i=0;i<sdu->N_bytes;i++){
        printf("0x:%x\n",*(sdu->msg+i));
    }
    if (users.count(rnti)){
      rlc->write_sdu(rnti,lcid,std::move(sdu));
    }
  }

  else
#endif
    if (lcid == 5)
    {
      std::cout << "down-------pdcp----lcid----:" << lcid << std::endl;
      for (uint32_t i = 0; i < sdu->N_bytes; i++)
      {
        printf("0x:%x\n", *(sdu->msg + i));
      }
      if (users.count(rnti))
      {
        rlc->write_sdu(rnti, lcid, std::move(sdu));
        // rlc->write_sdu(rnti,lcid,std::move(sdu_test));
      }
    }
    else
    {
#if 1
      // std::cout << " lcid = " << lcid << std::endl;
      // std::cout << " sdu bytes = " << sdu->N_bytes << std::endl;
      //  for (uint32_t i = 0; i < sdu->N_bytes; i++)
      //  {
      //    printf("0x:%x\n", *(sdu->msg + i));
      //  }
#endif

      //*********TC618 start*******    //*********TC619 start*******
      else if (pdcp_adp->udp_.TC_618_Info_To_TTCN || pdcp_adp->udp_.TC_619_Info_To_TTCN)
      {
          for (uint32_t i = 0; i < sdu->N_bytes; i++)
          {
              printf("TC618/9:%x\n", *(sdu->msg + i));
          }
          srsran::unique_byte_buffer_t TC_6114_Info = srsran::make_byte_buffer();
          TC_6114_Info->N_bytes = 10;
          TC_6114_Info->msg[0] = 0x01;
          TC_6114_Info->msg[1] = 0x06;
          TC_6114_Info->msg[2] = 0x00;
          TC_6114_Info->msg[3] = 0x02; //test id
          TC_6114_Info->msg[4] = 0x69; //test id
          TC_6114_Info->msg[5] = 0x00;
          TC_6114_Info->msg[6] = 0x02;
          TC_6114_Info->msg[7] = 0x00;
          TC_6114_Info->msg[8] = 0x0b;   //此处的lcid直接填写了shoert bsr,因为只有MAC层通过检查，PDCP这边的标志才会为true，才能发到TTCN
          TC_6114_Info->msg[9] = sdu->msg[2];
          std::cout << "sdu->msg[2] = " << sdu->msg[2]<< std::endl;
          pdcp_adp->udp_.send_ttcn_info.try_push(std::move(TC_6114_Info));
      }
      //*********TC618 end*******   //*********TC619 end*******

      //*********TC6114 start*******
      else if (pdcp_adp->udp_.TC_6114_Info_To_TTCN)
      {
          for (uint32_t i = 0; i < sdu->N_bytes; i++)
          {
              printf("TC6114:%x\n", *(sdu->msg + i));
          }
          srsran::unique_byte_buffer_t TC_6114_Info = srsran::make_byte_buffer();
          TC_6114_Info->N_bytes = 10;
          TC_6114_Info->msg[0] = 0x01;
          TC_6114_Info->msg[1] = 0x06;
          TC_6114_Info->msg[2] = 0x00;
          TC_6114_Info->msg[3] = 0x02; //test id
          TC_6114_Info->msg[4] = 0x69; //test id
          TC_6114_Info->msg[5] = 0x00;
          TC_6114_Info->msg[6] = 0x02;
          TC_6114_Info->msg[7] = 0x00;
          TC_6114_Info->msg[8] = 0x00;
          TC_6114_Info->msg[9] = sdu->msg[2];
          std::cout << "sdu->msg[2] = " << sdu->msg[2]<< std::endl;
          pdcp_adp->udp_.send_ttcn_info.try_push(std::move(TC_6114_Info));
      } 
      //*********TC6114 end*******

      if (pdcp_adp->udp_.TC_6115_Info_To_TTCN)
      {
        std::cout << "asdasfasfasfasgagf" << std::endl;
        for (uint32_t i = 0; i < sdu->N_bytes; i++)
        {
          printf("0x:%x\n", *(sdu->msg + i));
        }
        pdcp_adp->udp_.TC_6115_Info_To_TTCN = false;
        srsran::unique_byte_buffer_t TC_6115_Info = srsran::make_byte_buffer();
        TC_6115_Info->N_bytes = 10;
        TC_6115_Info->msg[0] = 0x01;
        TC_6115_Info->msg[1] = 0x06;
        TC_6115_Info->msg[2] = 0x00;
        TC_6115_Info->msg[3] = 0x17;
        TC_6115_Info->msg[4] = 0xe3;
        TC_6115_Info->msg[5] = 0x00;
        TC_6115_Info->msg[6] = 0x02;
        TC_6115_Info->msg[7] = 0x00;
        TC_6115_Info->msg[8] = 0x00;
        TC_6115_Info->msg[9] = 0x00;
        pdcp_adp->udp_.send_ttcn_info.try_push(std::move(TC_6115_Info));
      }
      else if (pdcp_adp->udp_.TC_6116_Info_To_TTCN && abs(pdcp_adp->udp_.pa_adjust_before-pdcp_adp->udp_.pa_adjust_after)>=1)
      {
          std::cout << "asdasfasf---jjcpa---asfasgagf" << std::endl;
          for (uint32_t i = 0; i < sdu->N_bytes; i++)
          {
              printf("0x:%x\n", *(sdu->msg + i));
          }
          pdcp_adp->udp_.TC_6116_Info_To_TTCN = false;
          srsran::unique_byte_buffer_t TC_6116_Info = srsran::make_byte_buffer();
          TC_6116_Info->N_bytes = 10;
          TC_6116_Info->msg[0] = 0x01;
          TC_6116_Info->msg[1] = 0x06;
          TC_6116_Info->msg[2] = 0x00;
          TC_6116_Info->msg[3] = 0x17;
          TC_6116_Info->msg[4] = 0xe4;
          TC_6116_Info->msg[5] = 0x00;
          TC_6116_Info->msg[6] = 0x02;
          TC_6116_Info->msg[7] = 0x00;
          TC_6116_Info->msg[8] = 0x00;
          TC_6116_Info->msg[9] = 0x00;
          pdcp_adp->udp_.send_ttcn_info.try_push(std::move(TC_6116_Info));
      }
      else if (pdcp_adp->udp_.TC_6117_Info_To_TTCN &&(abs(pdcp_adp->udp_.fa_adjust_before-pdcp_adp->udp_.fa_adjust_after)>=160||abs(pdcp_adp->udp_.ta_adjust_before-pdcp_adp->udp_.ta_adjust_after)>=0.125))
      {
          std::cout << "asdasfasf---jjcfata---asfasgagf" << std::endl;
          pdcp_adp->udp_.fa_adjust_before=pdcp_adp->udp_.fa_adjust_after;
          pdcp_adp->udp_.ta_adjust_before=pdcp_adp->udp_.ta_adjust_after;
          for (uint32_t i = 0; i < sdu->N_bytes; i++)
          {
              printf("0x:%x\n", *(sdu->msg + i));
          }
         // pdcp_adp->udp_.TC_6117_Info_To_TTCN = false;
          srsran::unique_byte_buffer_t TC_6117_Info = srsran::make_byte_buffer();
          TC_6117_Info->N_bytes = 10;
          TC_6117_Info->msg[0] = 0x01;
          TC_6117_Info->msg[1] = 0x06;
          TC_6117_Info->msg[2] = 0x00;
          TC_6117_Info->msg[3] = 0x17;
          TC_6117_Info->msg[4] = 0xe4;
          TC_6117_Info->msg[5] = 0x00;
          TC_6117_Info->msg[6] = 0x02;
          TC_6117_Info->msg[7] = 0x00;
          TC_6117_Info->msg[8] = 0x00;
          TC_6117_Info->msg[9] = 0x00;
          pdcp_adp->udp_.send_ttcn_info.try_push(std::move(TC_6117_Info));
      }
      else if (pdcp_adp->udp_.TC_617_Info_To_TTCN)
      {
          for (uint32_t i = 0; i < sdu->N_bytes; i++)
          {
              printf("TC617:%x\n", *(sdu->msg + i));
          }
          srsran::unique_byte_buffer_t TC_617_Info = srsran::make_byte_buffer();
          TC_617_Info->N_bytes = 10;
          TC_617_Info->msg[0] = 0x01;
          TC_617_Info->msg[1] = 0x06;
          TC_617_Info->msg[2] = 0x00;
          TC_617_Info->msg[3] = 0x17; //test id
          TC_617_Info->msg[4] = 0xe1; //test id
          TC_617_Info->msg[5] = 0x00;
          TC_617_Info->msg[6] = 0x02;
          TC_617_Info->msg[7] = 0x00;
          TC_617_Info->msg[8] = 0x00;
          TC_617_Info->msg[9] = sdu->msg[2];
          std::cout << "sdu->msg[2] = " << sdu->msg[2]<< std::endl;
          pdcp_adp->udp_.send_ttcn_info.try_push(std::move(TC_617_Info));
      }
      
      //*********TC61119 start*******
      else if (pdcp_adp->udp_.TC_6113_Info_To_TTCN || pdcp_adp->udp_.TC_6119_Info_To_TTCN)
      {
          for (uint32_t i = 0; i < sdu->N_bytes; i++)
          {
              printf("TC6113:%x\n", *(sdu->msg + i));
          }
          srsran::unique_byte_buffer_t TC_6113_Info = srsran::make_byte_buffer();
          TC_6113_Info->N_bytes = 10;
          TC_6113_Info->msg[0] = 0x01;
          TC_6113_Info->msg[1] = 0x06;
          TC_6113_Info->msg[2] = 0x00;
          TC_6113_Info->msg[3] = 0x02; //test id
          TC_6113_Info->msg[4] = 0x69; //test id  不一样也没事，TTCN收到PDU就行
          TC_6113_Info->msg[5] = 0x00;
          TC_6113_Info->msg[6] = 0x02;
          TC_6113_Info->msg[7] = 0x00;
          TC_6113_Info->msg[8] = 0x00;
          TC_6113_Info->msg[9] = sdu->msg[2];
          std::cout << "sdu->msg[2] = " << sdu->msg[2]<< std::endl;
          pdcp_adp->udp_.send_ttcn_info.try_push(std::move(TC_6113_Info));
      } 
      //*********TC61119 end*******

      else if (sdap_testID == 1)
      {
        std::cout << "ttcn sdap case lcid = " << lcid << std::endl;
        send_sdap_msg_ttcn(rnti, lcid, std::move(sdu));
      }
      else if (rlc_testID == 1 || pdcp_testID == 1)
      {
        std::cout << " RLC send msg to TTCN " << std::endl;
        send_msg_ttcn(rnti, lcid, std::move(sdu));
      }
      else if (users.count(rnti))
      {
        users[rnti].pdcp->write_pdu(lcid, std::move(sdu));

        // 5.21 wcb

        icmp_rnti = rnti;
        if (users[rnti].pdcp->valid_lcid(lcid))
        {
          srsran::unique_byte_buffer_t icmp_data_1 = users[rnti].pdcp->pdcp_array.at(lcid)->get_icmp_data();
          if (icmp_data_1->N_bytes != 0)
          {
            // adp 传输 sdap --> icmp
            std::cout << "pdcp send ip data to upf" << std::endl;
            pdcp_adp->udp_.send_ip_data_to_cnw(std::move(icmp_data_1));
          }
        }
      }
    }
  }

  void pdcp::user_interface_gtpu::write_pdu(uint32_t lcid, srsran::unique_byte_buffer_t pdu)
  {
    gtpu->write_pdu(rnti, lcid, std::move(pdu));
  }

  void pdcp::user_interface_rlc::write_sdu(uint32_t lcid, srsran::unique_byte_buffer_t sdu)
  {
    std::cout << "@@@@@@@------pdcp-user_interface_rlc-write_sdu-------@@@@@@@@" << std::endl;
    rlc->write_sdu(rnti, lcid, std::move(sdu));
  }

  void pdcp::user_interface_rlc::discard_sdu(uint32_t lcid, uint32_t discard_sn)
  {
    rlc->discard_sdu(rnti, lcid, discard_sn);
  }

  bool pdcp::user_interface_rlc::rb_is_um(uint32_t lcid)
  {
    return rlc->rb_is_um(rnti, lcid);
  }

  bool pdcp::user_interface_rlc::is_suspended(uint32_t lcid)
  {
    return rlc->is_suspended(rnti, lcid);
  }

  bool pdcp::user_interface_rlc::sdu_queue_is_full(uint32_t lcid)
  {
    return rlc->sdu_queue_is_full(rnti, lcid);
  }

  void pdcp::user_interface_rrc::write_pdu(uint32_t lcid, srsran::unique_byte_buffer_t pdu)
  {
    if (pdcp_network_mode == 1)
    {
      std::cout << "-----------------PDCP TM transfer--------------------" << std::endl;
    }
    rrc->write_pdu(rnti, lcid, std::move(pdu));
  }

  void pdcp::user_interface_rrc::notify_pdcp_integrity_error(uint32_t lcid)
  {
    rrc->notify_pdcp_integrity_error(rnti, lcid);
  }

  void pdcp::user_interface_rrc::write_pdu_bcch_bch(srsran::unique_byte_buffer_t pdu)
  {
    ERROR("Error: Received BCCH from ue=%d", rnti);
  }

  void pdcp::user_interface_rrc::write_pdu_bcch_dlsch(srsran::unique_byte_buffer_t pdu)
  {
    ERROR("Error: Received BCCH from ue=%d", rnti);
  }

  void pdcp::user_interface_rrc::write_pdu_pcch(srsran::unique_byte_buffer_t pdu)
  {
    ERROR("Error: Received PCCH from ue=%d", rnti);
  }

  const char *pdcp::user_interface_rrc::get_rb_name(uint32_t lcid)
  {
    return srsenb::get_rb_name(lcid);
  }

  void pdcp::get_metrics(pdcp_metrics_t &m, const uint32_t nof_tti)
  {
    m.ues.resize(users.size());
    size_t count = 0;
    for (auto &user : users)
    {
      user.second.pdcp->get_metrics(m.ues[count], nof_tti);
      count++;
    }
  }

  ////////////////////// li

  int pdcp::do_ttcn_00(uint16_t rnti, uint32_t lcid, srsran::unique_byte_buffer_t ttcn_pdcp_msg)
  {

    std::cout << std::endl
              << "------  PDCP TEST 00 begin ------ " << std::endl;

    // move the msg pointer to the first byte
    ///---------------------------------------------
    ttcn_pdcp_msg->msg += 17;
    ttcn_pdcp_msg->N_bytes -= 17;

    if (users.count(rnti))
    {

      if (users[rnti].pdcp->pdcp_array.count(lcid))
      {
        users[rnti].pdcp->pdcp_array.at(lcid)->direct_write_rlc_sdu(lcid, std::move(ttcn_pdcp_msg));
      }
      else
      {
        std::cout << "no such lcid found in PDCP TEST 00, lcid =" << lcid << std::endl;
      }
    }
    else
    {
      std::cout << "no such rnti found in PDCP TEST 00, rnti =" << rnti << std::endl;
    }

    return 0;
  }

  void pdcp::TC_6115(uint16_t rnti, uint32_t lcid, srsran::unique_byte_buffer_t TC_6115_Info)
  {
    rlc->TC_6115_RLC_Handle(rnti, lcid);
    users[rnti].pdcp->write_sdu(lcid, std::move(TC_6115_Info));
  }
  void pdcp::TC_6116(uint16_t rnti, uint32_t lcid, srsran::unique_byte_buffer_t TC_6116_Info)
  {
      //rlc->TC_6116_RLC_Handle(rnti, lcid);
      users[rnti].pdcp->write_sdu(lcid, std::move(TC_6116_Info));
  }
  void pdcp::TC_6117(uint16_t rnti, uint32_t lcid, srsran::unique_byte_buffer_t TC_6117_Info)
  {
      // rlc->TC_6117_RLC_Handle(rnti, lcid);
      users[rnti].pdcp->write_sdu(lcid, std::move(TC_6117_Info));
  }
  void pdcp::TC_617(uint16_t rnti, uint32_t lcid, srsran::unique_byte_buffer_t TC_617_Info)
  {
      users[rnti].pdcp->write_sdu(lcid, std::move(TC_617_Info));
      pdcp_adp->udp_.TC_617_Info_To_TTCN = true;
  }
  void pdcp::TC_6113(uint16_t rnti, uint32_t lcid, srsran::unique_byte_buffer_t TC_6113_Info)
  {
      users[rnti].pdcp->write_sdu(lcid, std::move(TC_6113_Info));
  }
  void pdcp::pdcp_ttcn_test()
  {
    // if(pdcp ceshi)
    while (true)
    {
      if (pdcp_adp->udp_.pdcp_receive_info.size() != 0)
      {
        std::cout << "pdcp_adp->udp_.pdcp_receive_info.size() =" << pdcp_adp->udp_.pdcp_receive_info.size() << std::endl;
        srsran::unique_byte_buffer_t ttcn_pdcp_msg = srsran::make_byte_buffer();
        ttcn_pdcp_msg->init();
        pdcp_adp->udp_.pdcp_receive_info.try_pop(ttcn_pdcp_msg);

        uint16_t rnti = (uint16_t)ttcn_pdcp_msg->msg[5];
        std::cout << "------ rnti in pdcp test == " << rnti << "-------" << std::endl;
        uint32_t lcid = (uint32_t)ttcn_pdcp_msg->msg[12];
        std::cout << "------ lcid in pdcp test == " << lcid << "-------" << std::endl;

        if (ttcn_pdcp_msg->msg[2] == 0x01)
        {
          // TO RLC
          std::cout << "ENTER RLC TTCN TEST" << std::endl;
          lcid = (uint32_t)ttcn_pdcp_msg->msg[16];
          uint16_t testID = (ttcn_pdcp_msg->msg[3] << 8) | (ttcn_pdcp_msg->msg[4]);
          // if (testID == 623)
          // {
          //   rlc_testID = 1;
          // }
          rlc_testID = 1;
          if (testID == 6211 || testID == 6212)
          {
            printf("6212    6212");
            rlc->pdcp_ttcnmsg_rlc_s(rnti, lcid, std::move(ttcn_pdcp_msg));
          }
          else
          {
            printf("66666622288888\n");
            rlc->enable_rlc_testcase_related_configuration(rnti, lcid, testID, pdcp_adp->udp_.pid);
            ttcn_pdcp_msg->N_bytes -= 21;
            ttcn_pdcp_msg->msg += 21;
            users[rnti].pdcp->write_sdu(lcid, std::move(ttcn_pdcp_msg));
          }
          // remove Header
          // ttcn_pdcp_msg->N_bytes -= 21;
          // ttcn_pdcp_msg->msg += 21;
          // rlc->pdcp_ttcnmsg_rlc_s(rnti,lcid,std::move(ttcn_pdcp_msg));
          // rlc->enable_rlc_testcase_related_configuration(rnti, lcid, testID);
          // users[rnti].pdcp->write_sdu(lcid, std::move(ttcn_pdcp_msg));

          // // 0718
          // if (pdcp_adp->udp_.pdcp_receive_info.size() == 0)
          // {
          //   return;
          // }
        }
        else if (ttcn_pdcp_msg->msg[2] != 0x00)
        {

          // std::cout << std::endl
          //           << "------ ERROR!!!!: in PDCP TTCN TEST 0 -------" << std::endl;

          // return;
        }
        else
        {
          uint16_t testID = (ttcn_pdcp_msg->msg[3] << 8) | (ttcn_pdcp_msg->msg[4]);
          switch (testID)
          {

          // case 153
          case 631:
          case 632:

            pdcp_testID = 1;

            std::cout << std::endl
                      << "ready to do ttcn 00" << std::endl;
            // rlc->mod_ttcn_lcid(3,24);

            if (do_ttcn_00(rnti, lcid, std::move(ttcn_pdcp_msg)) != 0)
            {
              std::cout << std::endl
                        << "------ ERROR PDCP TEST 00 -------" << std::endl;
              pdcp_testID = -1;
              return;
            };

            break;

          case 0xFE:
            pdcp_testID = -1;
            // rlc->reset_ttcn_lcid();
            std::cout << std::endl
                      << "------ END PDCP TTCN TEST -------" << std::endl;
            return;

          default:
            pdcp_testID = -1;
            std::cout << std::endl
                      << "ERROR!!!!: in PDCP TTCN TEST 1" << std::endl;
            return;
          }
        }
        // std::cout << "gxm test 2" << std::endl;
        std::cout << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxzzzzzz 111=" << pdcp_adp->udp_.pdcp_receive_info.size() << std::endl;
        // 0718
        if (pdcp_adp->udp_.pdcp_receive_info.size() == 0)
        {
          std::cout << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxzzzzzz" << std::endl;
          break;
        }
      }
    }
  }

  void pdcp::write_head_value(uint8_t **msg, uint32_t *N_bytes, size_t size, uint32_t val)
  {
    *msg -= size;
    uint8_t *p = (uint8_t *)&val;
    memcpy(*msg, p + (4 - size), size);
    *N_bytes += size;
  }

  void pdcp::direct_ttcn_msg(srsran::unique_byte_buffer_t ttcn_pdcp_msg)
  {
#if 1
    std::cout << "TO TTCN" << std::endl;
    for (uint32_t i = 0; i < ttcn_pdcp_msg->N_bytes; i++)
    {
      printf("0x:%x ", ttcn_pdcp_msg->msg[i]);
    }
    printf("\n");
#endif
    pdcp_adp->udp_.send_ttcn_info.try_push(std::move(ttcn_pdcp_msg));
  }

  void pdcp::send_msg_ttcn(uint16_t rnti, uint32_t lcid, srsran::unique_byte_buffer_t pdu_to_ttcn)
  {
#if 0
      std::cout << std::endl << " pdu_to_ttcn->N_bytes = " << pdu_to_ttcn->N_bytes<<std::endl;
      for(uint32_t i=0;i<pdu_to_ttcn->N_bytes;i++){
      printf("0x:%x\n",*(pdu_to_ttcn->msg+i));
    }
#endif

    if (rlc_testID == 1)
    {
      std::cout << std::endl
                << "in fun:send RLC ttcn msg" << std::endl;
      direct_ttcn_msg(std::move(pdu_to_ttcn));
    }
    else if (pdcp_testID == 1)
    {
      PDCP_TTCN_pdcp_head subHead;

      subHead.lcid[0] = 0x00;
      subHead.lcid[1] = 0x00;
      subHead.lcid[2] = 0x00;
      subHead.lcid[3] = 0x03;

      subHead.msgType = 0x72;

      subHead.sduLen[0] = 0x00;
      subHead.sduLen[1] = 0x00;
      subHead.sduLen[2] = 0x00;
      subHead.sduLen[3] = pdu_to_ttcn->N_bytes;

      // subHead.sduLen = ttcn_pdcp_msg->N_bytes;
      pdu_to_ttcn->N_bytes += sizeof(subHead);
      pdu_to_ttcn->msg -= sizeof(subHead);
      memcpy(pdu_to_ttcn->msg, &subHead, sizeof(subHead));

      PDCP_TTCN_ttcn_head Head;
      Head.data_length[0] = 0x00;
      Head.data_length[1] = pdu_to_ttcn->N_bytes;

      // Head.data_length = ttcn_pdcp_msg->N_bytes;
      Head.des_layer_id = 0x05;
      Head.direction = 0x01;
      Head.rec_lay_id = 0x00;

      Head.test_id[0] = 0x00;
      Head.test_id[1] = 0x00;

      Head.rnti = rnti;

      // Head.test_id = 0x00;
      // Head.rnti=0;
      pdu_to_ttcn->N_bytes += sizeof(Head);
      pdu_to_ttcn->msg -= sizeof(Head);
      memcpy(pdu_to_ttcn->msg, &Head, sizeof(Head));

      std::cout << "ttcn_pdcp_msg->N 2 = " << pdu_to_ttcn->N_bytes << std::endl;
      for (uint32_t i = 0; i < pdu_to_ttcn->N_bytes; i++)
      {
        printf("0x:%x\n", *(pdu_to_ttcn->msg + i));
      }

      direct_ttcn_msg(std::move(pdu_to_ttcn));
    }
  }

  void pdcp::rlc_ttcn_msg(uint16_t rlc_rnti, srsran::unique_byte_buffer_t new_rx_sdu, uint16_t ttcn_testId)
  {

    direct_ttcn_msg(std::move(new_rx_sdu));
  }

  ////////////////////// li_end
  void pdcp::SDAP_TTCN_TEST()
  {
    while (true)
    {
      if (pdcp_adp->udp_.sdap_receive_info.size() != 0)
      {
        srsran::unique_byte_buffer_t ttcn_sdap_msg = srsran::make_byte_buffer();
        ttcn_sdap_msg->init();
        pdcp_adp->udp_.sdap_receive_info.try_pop(ttcn_sdap_msg);

        std::cout << "ttcn_sdap_msg Bytes=" << ttcn_sdap_msg->N_bytes << std::endl;
        for (uint32_t i = 0; i < ttcn_sdap_msg->N_bytes; i++)
        {
          printf("0x:%x ", ttcn_sdap_msg->msg[i]);
        }
        printf("\n");

        uint16_t rnti = ttcn_sdap_msg->msg[5];

        std::cout << "------ rnti in sdap test == " << rnti << "-------" << std::endl;

        uint32_t lcid = ttcn_sdap_msg->msg[12];
        lcid = 6;
        std::cout << "------ lcid in sdap test == " << lcid << "-------" << std::endl;

        switch (ttcn_sdap_msg->msg[4])
        {

          // case 644
        case 0x00:

          sdap_testID = 1;

          std::cout << std::endl
                    << "ready to do ttcn 00" << std::endl;
          if (do_sdap_ttcn_00(rnti, lcid, std::move(ttcn_sdap_msg)) != 0)
          {
            std::cout << std::endl
                      << "------ ERROR SDAP TEST 00 -------" << std::endl;
            sdap_testID = -1;
            return;
          };

          break;

        case 0xFF:
          sdap_testID = -1;
          rlc->reset_ttcn_lcid();
          std::cout << std::endl
                    << "------ END SDAP TTCN TEST -------" << std::endl;
          return;

        default:
          sdap_testID = -1;
          std::cout << std::endl
                    << "ERROR!!!!: in SDAP TTCN TEST 1" << std::endl;
          return;
        }

        break;
      }
    }
  }

  int pdcp::do_sdap_ttcn_00(uint16_t rnti, uint32_t lcid, srsran::unique_byte_buffer_t ttcn_sdap_msg)
  {
    std::cout << std::endl
              << "------  SDAP TEST 00 begin ------ " << std::endl;

    // move the msg pointer to the first byte
    ttcn_sdap_msg->msg += 12;
    ttcn_sdap_msg->N_bytes -= 12;

    // temp_qfi = ttcn_sdap_msg->msg[11];
    // temp_drbid = ttcn_sdap_msg->msg[12];
    // ttcn_sdap_msg->msg[12] = 0x01;
    // uint8_t temp = 0x01;
    // temp = (*(ttcn_sdap_msg + 12) & 0x01);
    //(uint8_t)*((ttcn_sdap_msg+12)) = temp;
    std::cout << "sdap1 send SDU to ttcn  22222" << std::endl;
    // temp_drbid = users[rnti].pdcp->sdap_mapping_modfiy(temp_qfi, temp_drbid);

    if (users.count(rnti))
    {

      if (users[rnti].pdcp->pdcp_array.count(lcid))
      {
        // users[rnti].pdcp->pdcp_array.at(lcid)->direct_write_rlc_sdu(lcid, std::move(ttcn_sdap_msg));
        users[rnti].pdcp->write_sdu(lcid, std::move(ttcn_sdap_msg));
      }
      else
      {
        std::cout << "no such lcid found in SDAP TEST 00, lcid =" << lcid << std::endl;
      }
    }
    else
    {
      std::cout << "no such rnti found in SDAP TEST 00, rnti =" << rnti << std::endl;
    }

    return 0;
  }

  void pdcp::send_sdap_msg_ttcn(uint16_t rnti, uint32_t lcid, srsran::unique_byte_buffer_t sdap_ttcn_msg)
  {
    std::cout << "ttcn_sdap_msg->N" << sdap_ttcn_msg->N_bytes << std::endl;
    for (uint32_t i = 0; i < sdap_ttcn_msg->N_bytes; i++)
    {
      printf("0x:%x\n", *(sdap_ttcn_msg->msg + i));
    }

    temp_qfi = sdap_ttcn_msg->msg[2] & 0x02;
    std::cout << "temp_qfi = " << temp_qfi << std::endl;
    sdap_ttcn_msg->msg += 3;
    sdap_ttcn_msg->N_bytes -= 3;

    SDAP_TTCN_sdap_head subHead;

    subHead.msgType = 0x4a;

    subHead.sduLen[0] = 0x00;
    subHead.sduLen[1] = sdap_ttcn_msg->N_bytes;

    subHead.dctype = 0x00;
    subHead.qfi = temp_qfi;
    subHead.drb_id = lcid;

    sdap_ttcn_msg->N_bytes += sizeof(subHead);
    sdap_ttcn_msg->msg -= sizeof(subHead);
    memcpy(sdap_ttcn_msg->msg, &subHead, sizeof(subHead));

    SDAP_TTCN_ttcn_head Head;
    Head.data_length[0] = 0x00;
    Head.data_length[1] = sdap_ttcn_msg->N_bytes;

    Head.des_layer_id = 0x00;
    Head.direction = 0x01;
    Head.rec_lay_id = 0x04;

    Head.test_id[0] = 0x00;
    Head.test_id[1] = 0x00;

    Head.rnti = rnti;

    sdap_ttcn_msg->N_bytes += sizeof(Head);
    sdap_ttcn_msg->msg -= sizeof(Head);
    memcpy(sdap_ttcn_msg->msg, &Head, sizeof(Head));

    std::cout << "ttcn_sdap_msg->N" << sdap_ttcn_msg->N_bytes << std::endl;
    for (uint32_t i = 0; i < sdap_ttcn_msg->N_bytes; i++)
    {
      printf("0xy:%x\n", *(sdap_ttcn_msg->msg + i));
    }

    // srsran::unique_byte_buffer_t sdap_ttcn_msg_1 = srsran::make_byte_buffer();
    // sdap_ttcn_msg_1->init();
    // memcpy(sdap_ttcn_msg_1->msg,&(sdap_ttcn_msg->msg),62);
    // sdap_ttcn_msg_1->N_bytes = 62;

    // //std::cout << "ttcn_sdap_msg->N" << sdap_ttcn_msg->N_bytes << std::endl;
    // std::cout << "sdap_ttcn_msg_1->N" << sdap_ttcn_msg_1->N_bytes << std::endl;
    // for (uint32_t i = 0; i < sdap_ttcn_msg_1->N_bytes; i++)
    // {
    //   printf("0xyz[%d]:%x\n",i, *(sdap_ttcn_msg_1->msg + i));
    // }

    pdcp_adp->udp_.send_ttcn_info.try_push(std::move(sdap_ttcn_msg));
  }

  bool pdcp::sdap_cfgerer(uint16_t rnti, uint32_t lcid, const srsran::sdap_allocate &sdap_cfg)
  {

    if (users.count(rnti))
    {
      users[rnti].pdcp->sdap_cfger(lcid, sdap_cfg);
    }
    return true;
  }
  //-----------------------------------------------------------------------------

  // void pdcp::SDAP_TTCN_header(srsran::unique_byte_buffer_t* sdap_ttcn_header)
  // {
  //   sdap_ttcn_header->get()->msg[0] = 0x01;  //Direction
  //   sdap_ttcn_header->get()->msg[1] = 0x04;  //Rec_Layer_id
  //   sdap_ttcn_header->get()->msg[2] = 0x00;  //Des_layer_id
  //   sdap_ttcn_header->get()->msg[3] = 0x00;  //test_id
  //   sdap_ttcn_header->get()->msg[4] = 0x00;  //test_id
  //   sdap_ttcn_header->get()->msg[5] = 0x03;  //rnti
  //   sdap_ttcn_header->get()->msg[6] = 0x00;  //data_Length
  //   sdap_ttcn_header->get()->msg[7] = 0x0f;  //data_Length
  //   sdap_ttcn_header->get()->msg[8] = 0x4a;  //msgType  74
  //   sdap_ttcn_header->get()->msg[9] = 0x00;  //sduLen
  //   sdap_ttcn_header->get()->msg[10] = 0x14;  //sduLen
  //   sdap_ttcn_header->get()->msg[11] = 0b00000000;
  //   sdap_ttcn_header->get()->msg[12] = 0b00000000;
  //   sdap_ttcn_header->get()->msg[13] = 0b00000000;
  //   sdap_ttcn_header->get()->msg[14] = 0x00;
  // }

} // namespace srsenb
