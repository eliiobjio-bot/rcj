#include "srscnw/hdr/udp.h"

#include <iostream>

namespace srsepc
{

  udp::udp()
      : nas_sm_receive_info(128),
        nas_mm_receive_info(128),
        rrc_receive_info(128),
        mac_receive_info(128),
        send_ttcn_info(128),
        received_enb_info(128),

        sdap_receive_info(128),
        pdcp_receive_info(128),
        ate_cnw_msg(128),
        icmp_receive_iperf_info(128),
        icmp_receive_sdap_info(128)
  {
  }

  bool udp::init(cnw_args_t &args, cnw_interface_cnwadp* adp_to_cnw)
  {
    
    this->adp_to_cnw = adp_to_cnw;

    // Create a socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1)
    {
      std::cerr << "Failed to create socket" << std::endl;
      return 1;
    }
    this->args = &args;

    memset(&serverAddr_, 0, sizeof(serverAddr_));
    serverAddr_.sin_family = AF_INET;            // IPv4
    serverAddr_.sin_port = htons(args.cnw_port); // Port number monitored by the server
    std::cout << "args.cnw_port " << args.cnw_port << std::endl;
    serverAddr_.sin_addr.s_addr = htonl(INADDR_ANY);
    char *pid1_str = inet_ntoa(serverAddr_.sin_addr);
    // printf("[PID1][ADDR]:%s\n", pid1_str);

    // ate_clientaddr   info
    memset(&ate1_clientaddr, 0, sizeof(ate1_clientaddr));
    ate1_clientaddr.sin_family = AF_INET;
    // ate1_clientaddr.sin_port = htons(9038);
    // ate1_clientaddr.sin_addr.s_addr = inet_addr("192.168.0.254");
    ate1_clientaddr.sin_port = htons(args.ate1_port);
    ate1_clientaddr.sin_addr.s_addr = inet_addr(args.ate1_ip_adrr.c_str());

    // ate_clientaddr   info
    memset(&ate2_clientaddr, 0, sizeof(ate2_clientaddr));
    ate2_clientaddr.sin_family = AF_INET;
    // ate2_clientaddr.sin_port = htons(9058);
    // ate2_clientaddr.sin_addr.s_addr = inet_addr("192.168.0.254");
    ate2_clientaddr.sin_port = htons(args.ate2_port);
    ate2_clientaddr.sin_addr.s_addr = inet_addr(args.ate2_ip_adrr.c_str());

    // enb1_clientaddr   info
    memset(&enb1_clientaddr, 0, sizeof(enb1_clientaddr));
    enb1_clientaddr.sin_family = AF_INET;
    // enb1_clientaddr.sin_port = htons(5001);
    // enb1_clientaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    enb1_clientaddr.sin_port = htons(args.enb1_port);
    enb1_clientaddr.sin_addr.s_addr = inet_addr(args.enb1_ip_adrr.c_str());

    // enb2_clientaddr   info
    memset(&enb2_clientaddr, 0, sizeof(enb2_clientaddr));
    enb2_clientaddr.sin_family = AF_INET;
    // enb2_clientaddr.sin_port = htons(6001);
    // enb2_clientaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    enb2_clientaddr.sin_port = htons(args.enb2_port);
    enb2_clientaddr.sin_addr.s_addr = inet_addr(args.enb2_ip_adrr.c_str());

    // enb1_gtpu_clientaddr   info
    memset(&enb1_gtpu_clientaddr, 0, sizeof(enb1_gtpu_clientaddr));
    enb1_gtpu_clientaddr.sin_family = AF_INET;
    enb1_gtpu_clientaddr.sin_port = htons(GTPU_RX_PORT);
    enb1_gtpu_clientaddr.sin_addr.s_addr = inet_addr("127.0.1.1");

    // enb2_gtpu_clientaddr   info
    memset(&enb2_gtpu_clientaddr, 0, sizeof(enb2_gtpu_clientaddr));
    enb2_gtpu_clientaddr.sin_family = AF_INET;
    // enb1_gtpu_clientaddr.sin_port = htons(5001);
    // enb1_gtpu_clientaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    enb2_gtpu_clientaddr.sin_port = htons(args.enb1_port);
    enb2_gtpu_clientaddr.sin_addr.s_addr = inet_addr(args.enb1_ip_adrr.c_str());


    if (bind(sockfd, (struct sockaddr *)&serverAddr_, sizeof(serverAddr_)) == -1)
    {
      std::cerr << "Failed to bind socket on port 5004" << cnw_port << std::endl;
      close(sockfd);
      return 1;
    }

    return true;
  }

  void udp::printf_data(uint8_t *arr, int len)
  {
    std::cout << "[UDP][PRINTF][DATA]" << std::endl;
    std::cout << "[UDP][PRINTF][DATA][LEN]=" << (int)len << std::endl;
    for (int i = 0; i < len; i++)
    {
      printf("0x:%x ", arr[i]);
    }
    std::cout << std::endl;
  }

  /**************************************************************************
   *       UDP Connect, receive and send messages
   **************************************************************************/

  int udp::run_udp()
  {
    char ipbuf[16]; // Stored ip address

    while (true)
    {
      srsran::unique_byte_buffer_t pdu = srsran::make_byte_buffer();
      pdu->N_bytes = 1024;
      pdu->init();

      struct sockaddr_in clientaddr;
      int len = sizeof(clientaddr);

      int receivelength =
          recvfrom(sockfd, pdu->msg, 1500, 0,
                   (struct sockaddr *)&clientaddr, (socklen_t *)&len);
      pdu->N_bytes = receivelength;

      if (receivelength > 0)
      {
        printf("client IP : %s,  Port: % d\n ",
               inet_ntop(AF_INET, &clientaddr.sin_addr.s_addr, ipbuf, sizeof(ipbuf)),
               ntohs(clientaddr.sin_port));
      }

      if (pdu->N_bytes == 2 && pdu->msg[0] == 0x5a)
      {
        uint8_t src = (pdu->msg[1] >> 5);
        printf(" pdu->msg[0]: %x\n", pdu->msg[0]);
        printf(" pdu->msg[1]: %x\n", pdu->msg[1]);
         printf(" src: %d\n", (int)src);
        uint8_t destination = (pdu->msg[1] >> 2);
        printf(" destination= %d\n", (int)destination);
        srsran::console("[NAS]:heart_packet_handle\n");
        if (destination == 0x04)
        {
          srsran::unique_byte_buffer_t res_pdu = srsran::make_byte_buffer();
          res_pdu->msg[0] = 0x5a;
          res_pdu->msg[1] = 0x80;
          res_pdu->N_bytes = 2;
          srsran::console("[NAS]:heart_packet_handle\n");
          if (sendto(sockfd, res_pdu->msg, res_pdu->N_bytes, 0,
                     (struct sockaddr *)&clientaddr,
                     sizeof(clientaddr)) == -1)
          {
            std::cerr << "Failed to send res_pdu to test_master" << std::endl;
          }
        }
        continue;
      }

      if (handle_s1_msg(pdu))
        ;                                                // s1 msg
      else if (receivelength > 0 && pdu->msg[0] == 0xFF) // receive the ATE msg
      {
        pdu->N_bytes = receivelength;
        std::cout << "FF pdu->N_bytes=" << pdu->N_bytes << std::endl;
        pdu->msg[receivelength] = '\0';
        std::cout << "Received message from ATE client: " << std::endl;
        // printf_data(pdu->msg, receivelength);
        read_ate_pdu(std::move(pdu));
      }
      
      // adp_to_cnw->run_cnw();

      // usleep(1000);//delayTime
    }
    close(sockfd);
    return 0;
  }

  void udp::communication_test()
  {
    srsran::unique_byte_buffer_t test_data = srsran::make_byte_buffer();
    test_data->init();
    test_data->msg[0] = 0x0f;
    test_data->msg[1] = 0x00;
    test_data->msg[2] = 0x0f;
    test_data->msg[3] = 0x0f;
    test_data->msg[4] = 0x0e;
    test_data->msg[5] = 0x60;
    test_data->N_bytes = 6;

    // send_ttcn_info.try_push(std::move(test_data));
    // send_ttcn_msg_enb(std::move(test_data));
  }

  /**************************************************************************
   *         Remove the header and put it into the corresponding queue
   **************************************************************************/
  void udp::read_pdu(srsran::unique_byte_buffer_t pdu_)
  {
    // Temporarily simulates read head operation

    Gen_.direction = pdu_->msg[0];
    Gen_.rec_lay_id = pdu_->msg[1];
    Gen_.des_layer_id = pdu_->msg[2];
    Gen_.test_id = (pdu_->msg[3] << 8) | (pdu_->msg[4]);
    Gen_.data_length = (pdu_->msg[5] << 8) | (pdu_->msg[6]);
    std::cout << "xx r layer_id:" << (int)Gen_.rec_lay_id << std::endl;
    std::cout << "xx r test_id:" << (int)Gen_.test_id << std::endl;

    if (receive_ttcn_pdu_number == 0 && enb_id == 1)
    {
      ttcn_control_release(Gen_.test_id);
    }
    if (receive_ttcn_pdu_number == 2 && (Gen_.test_id == 74 || Gen_.test_id == 75) && enb_id == 1)
    {
      second_ttcn_paging = true;
    }
    if (receive_ttcn_pdu_number == 1 && Gen_.test_id == 73 && enb_id == 1)
    {
      second_ttcn_paging = true;
    }
    if (receive_ttcn_pdu_number == 0 && enb_id == 2)
    {
      if (Gen_.test_id == 514)
      {
        second_ttcn_paging = true;
        TC_514_Con_Req = true;
      }
      if (Gen_.test_id == 515)
      {
        second_ttcn_paging = true;
        TC_515_Con_Req = true;
      }
    }
    if (receive_ttcn_pdu_number == 2 && Gen_.test_id == 628)
    {
      TC_628_TTCN_RECONFIG = true;
    }

    receive_ttcn_pdu_number++;

    // if(Gen_.test_id==75|| Gen_.test_id == 73|| Gen_.test_id == 74){//for second paging
    //   paging_count_num++;
    //   if(paging_count_num==2)
    //   {
    //     second_ttcn_paging=true;
    //   }
    // }

    // if(Gen_.test_id==2){
    if (Gen_.test_id == 613)
    {
      std::cout << "send_rach_count" << send_rach_count << std::endl;
      if (send_rach_count == 1)
      {
        is_second_send_rach_testcase2 = true;
      }
      send_rach_count++;
    }

    // 4.10 XK
    if (Gen_.rec_lay_id == 0x05)
    {
      pdcp_ttcn_test = true;
    }
    else if (Gen_.rec_lay_id == 0x04)
    {
      sdap_ttcn_test = true;
    }

    /* srsran::unique_byte_buffer_t handle_pdu=
      srsran::make_byte_buffer();
      memcpy(handle_pdu->msg,pdu_->msg+7,pdu_->N_bytes-7);
      handle_pdu->N_bytes=pdu_->N_bytes-7; */

    if (Gen_.rec_lay_id == 0x01)
    {
      nas_sm_receive_info.try_push(std::move(pdu_));
    }
    else if (Gen_.rec_lay_id == 0x02)
    {
      nas_mm_receive_info.try_push(std::move(pdu_));
    }
    else if (Gen_.rec_lay_id == 0x03)
    {
      rrc_receive_info.try_push(std::move(pdu_));
    }
    else if (Gen_.rec_lay_id == 0x04)
    {

      std::cout << std::endl
                << "------- sdap_receive_info receive PDU from TTCN -------" << std::endl;
      std::cout << "PDU SIZE = " << pdu_->N_bytes << std::endl;
      sdap_receive_info.try_push(std::move(pdu_));
    }
    else if (Gen_.rec_lay_id == 0x05)
    {

      std::cout << std::endl
                << "------- pdcp_receive_info receive PDU from TTCN ------- " << std::endl;
      std::cout << "PDU SIZE = " << pdu_->N_bytes << std::endl;
      pdcp_receive_info.try_push(std::move(pdu_));
    }
    else if (Gen_.rec_lay_id == 0x06)
    {
      mac_receive_info.try_push(std::move(pdu_));
      std::cout << "mac_receive_info:" << mac_receive_info.size() << std::endl;
    }
    else
    {
      std::cout << "--------This message doesn't belong on any level---------" << std::endl;
    }
  }

  // ATE
  void udp::read_ate_pdu(srsran::unique_byte_buffer_t pdu_)
  {
    if (pdu_->N_bytes < 2)
    {
      std::cout << "-error-pdu_->N_bytes < 2---" << std::endl;
      return;
    }
    ate_cnw_msg.try_push(std::move(pdu_));

    return;
  }

  void udp::enble_flag(srsran::unique_byte_buffer_t pdu_)
  {
    enble_ttcn_flag_.flag_type = pdu_->msg[0];
    enble_ttcn_flag_.flag_value = pdu_->msg[1];
    enble_ttcn_flag_.ttcn_testId = (pdu_->msg[2] << 8) | (pdu_->msg[3]);

    std::cout << "enble_ttcn_flag_.flag_value:" << enble_ttcn_flag_.flag_value << std::endl;

    std::cout << "enble_ttcn_flag_.ttcn_testId" << enble_ttcn_flag_.ttcn_testId << std::endl;
    Indicate_voice_complete_flag(enble_ttcn_flag_.ttcn_testId);
    enble_testcase_end_info_flag(enble_ttcn_flag_.ttcn_testId);

    // testcase end flag

    /*   Discarded empty packet reply
    if(enble_ttcn_flag_.ttcn_testId==75||enble_ttcn_flag_.ttcn_testId==71||enble_ttcn_flag_.ttcn_testId==79||
        enble_ttcn_flag_.ttcn_testId==1||enble_ttcn_flag_.ttcn_testId==2||enble_ttcn_flag_.ttcn_testId ==73 ||
        enble_ttcn_flag_.ttcn_testId == 74 || enble_ttcn_flag_.ttcn_testId == 711 || enble_ttcn_flag_.ttcn_testId == 712||
        enble_ttcn_flag_.ttcn_testId ==517 || enble_ttcn_flag_.ttcn_testId==615 || enble_ttcn_flag_.ttcn_testId==612 ||
        enble_ttcn_flag_.ttcn_testId==616||
        enble_ttcn_flag_.ttcn_testId==725){

        srsran::unique_byte_buffer_t rerurn_info_send=srsran::make_byte_buffer();
        rerurn_info_send->init();
        rerurn_info_send->msg[0]=0x01;
        rerurn_info_send->msg[1]=0x01;
        rerurn_info_send->msg[2]=0x01;
        rerurn_info_send->msg[3]=0x01;
        rerurn_info_send->N_bytes=4;
        send_ttcn_info.try_push(std::move(rerurn_info_send));
      }*/

    if (enble_ttcn_flag_.ttcn_testId == 513)
    {
      sib_barred_num++;
      TC_513_barred_con_req = true;

      srsran::unique_byte_buffer_t rerurn_info_send = srsran::make_byte_buffer();
      rerurn_info_send->init();
      rerurn_info_send->msg[0] = 0x01;
      rerurn_info_send->msg[1] = 0x01;
      rerurn_info_send->msg[2] = 0x01;
      rerurn_info_send->msg[3] = 0x01;
      rerurn_info_send->N_bytes = 4;
      // send_ttcn_info.try_push(std::move(rerurn_info_send));
      // send_ttcn_msg_enb(std::move(rerurn_info_send));

      if (sib_barred_num == 1)
      {
        TC_513_barred = true;
      }

      if (sib_barred_num == 2)
      {
        TC_513_barred = false;
        TC_513_sib_barred = true;
      }
    }
    else if (enble_ttcn_flag_.ttcn_testId == 512)
    {
      srsran::unique_byte_buffer_t rerurn_info_send = srsran::make_byte_buffer();
      rerurn_info_send->init();
      rerurn_info_send->msg[0] = 0x01;
      rerurn_info_send->msg[1] = 0x01;
      rerurn_info_send->msg[2] = 0x01;
      rerurn_info_send->msg[3] = 0x01;
      rerurn_info_send->N_bytes = 4;
      // send_ttcn_info.try_push(std::move(rerurn_info_send));

      TC_512_qrexlevmin = true;
    }
    else if (enble_ttcn_flag_.ttcn_testId == 739)
    {
      std::cout << "enble_ttcn_flag_.ttcn_testId" << std::endl;
      is_connection = true;
    }
    else if (enble_ttcn_flag_.ttcn_testId == 511)
    {
      TC_511_Con_Req = true;
    }
    else if (enble_ttcn_flag_.ttcn_testId == 802)
    {
      is_reg_rej_illegal_ue = true;
    }
    else if (enble_ttcn_flag_.ttcn_testId == 804)
    {
      is_reg_req_congestion = true;
    }
    else if (enble_ttcn_flag_.ttcn_testId == 820)
    {
      is_security_mode_command2 = true;
    }
    else if (enble_ttcn_flag_.ttcn_testId == 816)
    {
      is_authencation_faileure_mac_code = true;
    }
    else if (enble_ttcn_flag_.ttcn_testId == 801)
    {
      is_reg_succecc_no_5gguti = true;
    }
    else if (enble_ttcn_flag_.ttcn_testId == 805)
    {
      is_periodic_registration_request = true;
    }
    else if (enble_ttcn_flag_.ttcn_testId == 815)
    {
      is_authentication_reject = true;
    }
    else if (enble_ttcn_flag_.ttcn_testId == 817)
    {
      is_authencation_faileure_repeated_ngksi = true;
    }
    else if (enble_ttcn_flag_.ttcn_testId == 818)
    {
      is_authencation_reject_error_res = true;
    }
    else if (enble_ttcn_flag_.ttcn_testId == 819)
    {
      is_security_mode_command_ue_cap_errror = true;
    }
    else if (enble_ttcn_flag_.ttcn_testId == 821)
    {
      is_nas_identity_request = true;
    }
    else if (enble_ttcn_flag_.ttcn_testId == 822)
    {
      is_ue_send_service_request = true;
    }
    else if (enble_ttcn_flag_.ttcn_testId == 824)
    {
      is_ue_config_update = true;
    }
    else if (enble_ttcn_flag_.ttcn_testId == 807)
    {
      is_reg_rej_tracking_area_not_allowed = true;
    }
    else if (enble_ttcn_flag_.ttcn_testId == 93)
    {
      std::cout << "ttcn_testId:" << enble_ttcn_flag_.ttcn_testId << std::endl;
      sm_pdu_accept = true;
    }
    else if (enble_ttcn_flag_.ttcn_testId == 91)
    {
      TC_91_sm_5_request = true;
    }

    else if (enble_ttcn_flag_.ttcn_testId == 92)
    {
      printf("enble_ttcn_flag_.ttcn_testId\n");
      TC_92_sm_release = true;
    }
    else if (enble_ttcn_flag_.ttcn_testId == 154)
    {
      std::cout << "ttcn_sdap_testId:" << enble_ttcn_flag_.ttcn_testId << std::endl;
      sdap_nhdr_tran = true;
    }
    else if (enble_ttcn_flag_.ttcn_testId == 811)
    {
      std::cout << "sssssssssssssssssssssssssssssssssssssssssssss------sgj-----5" << std::endl;
      is_dereg_reg_req_ue_switchon = true;
    }
    else if (enble_ttcn_flag_.ttcn_testId == 813)
    {
      is_dereg_normal = true;
    }
    else if (enble_ttcn_flag_.ttcn_testId == 806)
    {
      is_reg_rej_plmn_not_allowed = true;
    }
    else if (enble_ttcn_flag_.ttcn_testId == 803)
    {
      is_registration_reject_Ue_attempt_five_count = true;
    }
    else if (enble_ttcn_flag_.ttcn_testId == 814)
    {
      is_net_org_dereg_req_need_regreq_repeat = true;
    } // ttcn close tc flag
    else if (enble_ttcn_flag_.ttcn_testId == 617 ||
            enble_ttcn_flag_.ttcn_testId == 6113 || 
            enble_ttcn_flag_.ttcn_testId == 624 || 
            enble_ttcn_flag_.ttcn_testId == 628 || 
            enble_ttcn_flag_.ttcn_testId == 622 || 
            enble_ttcn_flag_.ttcn_testId == 623 || 
            enble_ttcn_flag_.ttcn_testId == 632 || 
            enble_ttcn_flag_.ttcn_testId == 625 || 
            enble_ttcn_flag_.ttcn_testId == 621)
    {
      is_um = true;
    }

    //*********TC6114 start*******
    else if (enble_ttcn_flag_.ttcn_testId == 6114)
    { 
      TC_6114_process_subheader_dl = true;
    }
    //*********TC6114 end*******

    else if (enble_ttcn_flag_.ttcn_testId == 6115)
    { // jjc
      // srsran::unique_byte_buffer_t rerurn_info_send=srsran::make_byte_buffer();
      // rerurn_info_send->init();
      // rerurn_info_send->msg[0]=0x01;
      // rerurn_info_send->msg[1]=0x01;
      // rerurn_info_send->msg[2]=0x01;
      // rerurn_info_send->msg[3]=0x01;
      // rerurn_info_send->N_bytes=4;
      // send_ttcn_info.try_push(std::move(rerurn_info_send));
      TC_6115_mac_srnti_match = true;
    }
    else if (enble_ttcn_flag_.ttcn_testId == 6116)
    { // jjc
      TC_6116_closed_loop_power_control = true;
    }
    else if (enble_ttcn_flag_.ttcn_testId == 6117)
    { // jjc
      TC_6117_ul_tf_sync = true;
    }
    else if (enble_ttcn_flag_.ttcn_testId == 6118)
    { // sxy
      TC_6118_mac_ce_handover = true;
    }

    //*********TC6119 start*******
    else if (enble_ttcn_flag_.ttcn_testId == 6119)
    { 
      TC_6119_ul_mcs = true;
    }
    //*********TC6119 end*******

    else if (enble_ttcn_flag_.ttcn_testId == 617)
    { // sxy
      TC_617_map_dtch = true;
    }
    //*********TC618 start*******
    else if (enble_ttcn_flag_.ttcn_testId == 618)
    { 
      TC_618_mac_regular_pdu = true;
    }
    //*********TC618 end*******

    //*********TC619 start*******
    else if (enble_ttcn_flag_.ttcn_testId == 619)
    { 
      TC_619_mac_bsr_timer = true;
    }
    //*********TC619 end*******

    //*********TC6111 start*******
    else if (enble_ttcn_flag_.ttcn_testId == 6111)
    { 
      TC_6111_mac_periodic_phr_timer = true;
    }
    //*********TC6111 end*******

    //*********TC6112 start*******
    else if (enble_ttcn_flag_.ttcn_testId == 6112)
    { 
      TC_6112_mac_pass_loss = true;
    }
    //*********TC6112 end*******      

    else if (enble_ttcn_flag_.ttcn_testId == 6113)
    { // sxy
      TC_6113_padding_dl = true;
    }
    if (enble_ttcn_flag_.ttcn_testId == 6212 || 
    enble_ttcn_flag_.ttcn_testId == 6113 || 
    enble_ttcn_flag_.ttcn_testId == 624 || 
    enble_ttcn_flag_.ttcn_testId == 628 || 
    enble_ttcn_flag_.ttcn_testId == 617 ||
    enble_ttcn_flag_.ttcn_testId == 618 ||   //*********TC618 start*******
    enble_ttcn_flag_.ttcn_testId == 619 ||   //*********TC619 start******* 
    enble_ttcn_flag_.ttcn_testId == 629 || 
    enble_ttcn_flag_.ttcn_testId == 6210 || 
    enble_ttcn_flag_.ttcn_testId == 6211 || 
    enble_ttcn_flag_.ttcn_testId == 623 || 
    enble_ttcn_flag_.ttcn_testId == 622 || 
    enble_ttcn_flag_.ttcn_testId == 6213 || 
    enble_ttcn_flag_.ttcn_testId == 626 || 
    enble_ttcn_flag_.ttcn_testId == 625 || 
    enble_ttcn_flag_.ttcn_testId == 621 || 
    enble_ttcn_flag_.ttcn_testId == 627 || 
    enble_ttcn_flag_.ttcn_testId == 631 || 
    enble_ttcn_flag_.ttcn_testId == 632 || 
    enble_ttcn_flag_.ttcn_testId == 154 ||
    enble_ttcn_flag_.ttcn_testId == 6114 || //*********TC6114 start*******
    enble_ttcn_flag_.ttcn_testId == 6115 || 
    enble_ttcn_flag_.ttcn_testId == 6116 || 
    enble_ttcn_flag_.ttcn_testId == 6117
    enble_ttcn_flag_.ttcn_testId == 6119)  //*********TC6119 start*******
    {
      ttcn_close_TC = true;
    }

    std::cout << "is_connection:" << is_connection << std::endl;

    // if (enble_ttcn_flag_.flag_type == ttcn_nasmm_enble)
    // {
    //   printf("enble_ttcn_flag_.flag_value:%x\n", enble_ttcn_flag_.flag_value);
    //   args->stack.ttcn_nasmm_enble = enble_ttcn_flag_.flag_value;
    // }
    // else if (enble_ttcn_flag_.flag_type == ttcn_nassm_enble)
    // {
    //   args->stack.ttcn_nassm_enble = enble_ttcn_flag_.flag_value;
    // }
    // else if (enble_ttcn_flag_.flag_type == ttcn_rrc_enble)
    // {
    //   args->stack.ttcn_rrc_enble = true;
    //   std::cout << "args->stack.ttcn_rrc_enble:" << args->stack.ttcn_rrc_enble << std::endl;
    // }
    // else if (enble_ttcn_flag_.flag_type == ttcn_sdap_enble)
    // {
    //   args->stack.ttcn_sdap_enble = enble_ttcn_flag_.flag_value;
    // }
    // else if (enble_ttcn_flag_.flag_type == ttcn_pdcp_enble)
    // {
    //   args->stack.ttcn_pdcp_enble = enble_ttcn_flag_.flag_value;
    // }
    // else if (enble_ttcn_flag_.flag_type == ttcn_rlc_enble)
    // {
    //   args->stack.ttcn_rlc_enble = enble_ttcn_flag_.flag_value;
    // }
    // else if (enble_ttcn_flag_.flag_type == ttcn_mac_enble)
    // {
    //   std::cout << "        enble_ttcn_flag_.flag_type==ttcn_mac_enble        " << std::endl;
    //   args->stack.mac.ttcn_mac_enble = enble_ttcn_flag_.flag_value;
    //   mac_ss_flag = true;
    //   std::cout << "          args->stack.mac.ttcn_mac_enble       " << args->stack.mac.ttcn_mac_enble << std::endl;
    // }
    // else
    // {
    //   std::cout << "            TTCN-3 set modulde-flag is Nothing             " << std::endl;
    // }
  }

  void udp::Indicate_voice_complete_flag(uint16_t testId)
  {   //*********TC618 start*******   //*********TC619 start*******  //*********TC6111 start*******  //*********TC6112 start*******  //*********TC6114 start*******  //*********TC6119 start*******    
    if (enb_id == 1 && (testId == 618 || testId == 619 || testId == 6111 || testId == 6112 || testId == 6114 || testId == 6119 || testId == 75 || testId == 71 || testId == 79 || testId == 611 || testId == 613 || testId == 72 || testId == 73 || testId == 74 ||
                        testId == 711 || testId == 712 || testId == 517 || testId == 614 || testId == 615 || testId == 612 || testId == 616 || testId == 617 || testId == 6113 || testId == 725 || testId == 514 || testId == 515 || testId == 710 || testId == 516 || testId == 713 || testId == 714 || testId == 6115 || testId == 6116 || testId == 6117 || testId == 6118 || testId == 722 || testId == 628 || testId == 92 || testId == 720 || testId == 716 || testId == 715))
    {
      printf(" Testcase %d voice complete flag is enble", testId);
      is_voice_complete = true;
    }
    else
    {
      printf("  Testcase %d voice complete flag isn't enble ", testId);
    }
  }

  void udp::ttcn_control_release(uint16_t testId)
  { // read_pdu
    if (testId == 75 || testId == 71 || testId == 79 || testId == 611 || testId == 613 || testId == 72 || testId == 73 || testId == 74 ||
        testId == 711 || testId == 712 || testId == 517 || testId == 614 || testId == 615 || testId == 612 || testId == 616 || testId == 725 || testId == 514 || testId == 515 || testId == 710 || testId == 516 || testId == 713 || testId == 714 || testId == 715)
    {
      printf(" Testcase %d  release flag is enble ", testId);
      is_ttcn_release_paging = true;
    }
    else
    {
      printf("  Testcase %d  didn't need to release ", testId);
    }
  }

  void udp::enble_testcase_end_info_flag(uint16_t testId)
  {
    if (testId == 511 || testId == 512 || testId == 513 || testId == 739 || testId == 801 || testId == 804 || testId == 811 || testId == 813 || testId == 820)
    { // 1 registration
      std::cout << "1 registration" << std::endl;
      is_first_registration_complete = true;
    }
    else if (testId == 93)
    { // 2 reconfig
      std::cout << "2 reconfig" << std::endl;
      is_2_reconfig_complete = true;
    }
    else if (testId == 618 || testId == 619 || testId == 6111 || testId == 6112 || testId == 6119 || testId == 517 || testId == 611 || testId == 612 || testId == 613 || testId == 614 || testId == 615 || testId == 616 || testId == 71 || testId == 72 || testId == 74 || testId == 75 || testId == 79)
    { // 3 reconfig   //*********TC618 start*******   //*********TC619 start*******  //*********TC6111 start*******  //*********TC611 start*******  //*********TC6119 start*******
      is_3_reconfig_complete = true;
      std::cout << "3 reconfig" << std::endl;
    }
    else if (testId == 725)
    { // 4 reconfig
      std::cout << "4 reconfig" << std::endl;
      is_4_reconfig_complete = true;
    }
    else if (testId == 814)
    { // 2 registration
      std::cout << "2 registration" << std::endl;
      is_second_registration_complete = true;
    }
  }

  bool udp::send_ate_msg(srsran::unique_byte_buffer_t pdu)
  {
    std::cout << "-----cnw-send_ATE_info-------message:" << std::endl;
    // printf_data(pdu->msg, (pdu->N_bytes > 4 ? 4 : pdu->N_bytes));
    if (pdu->msg[0] != 0xff)
    {
      std::cout << " ate message error !" << std::endl;
      return false;
    }
    if (enb_id == 1)
    {
      if (sendto(sockfd, pdu->msg, pdu->N_bytes, 0,
                 (struct sockaddr *)&ate1_clientaddr, sizeof(ate1_clientaddr)) == -1)
      {
        std::cerr << "Failed to send data to ate1" << std::endl;
        return false;
      }
      else
      {
        std::cout << "Data sent to ate1 successfully" << std::endl;
      }
    }
    else if (enb_id == 2)
    {
      if (sendto(sockfd, pdu->msg, pdu->N_bytes, 0,
                 (struct sockaddr *)&ate2_clientaddr, sizeof(ate2_clientaddr)) == -1)
      {
        std::cerr << "Failed to send data to ate2" << std::endl;
        return false;
      }
      else
      {
        std::cout << "Data sent to ate2 successfully" << std::endl;
      }
    }

    return true;
  }

  bool udp::send_enb_msg(srsran::unique_byte_buffer_t pdu)
  {
    printf("----cnw-send-enb-message---\n");
    // printf_data(pdu->msg, (pdu->N_bytes > 4 ? 4 : pdu->N_bytes));
    if (enb_id == 1)
    {
      if (sendto(sockfd, pdu->msg, pdu->N_bytes, 0,
                 (struct sockaddr *)&enb1_clientaddr, sizeof(enb1_clientaddr)) == -1)
      {
        std::cerr << "Failed to send data to enb1" << std::endl;
        return false;
      }
      else
      {
        std::cout << "Data sent to enb1 successfully" << std::endl;
      }
    }
    else if (enb_id == 2)
    {
      if (sendto(sockfd, pdu->msg, pdu->N_bytes, 0,
                 (struct sockaddr *)&enb2_clientaddr, sizeof(enb2_clientaddr)) == -1)
      {
        std::cerr << "Failed to send data to enb2" << std::endl;
        return false;
      }
      else
      {
        std::cout << "Data sent to enb2 successfully" << std::endl;
      }
    }

    return true;
  }

  bool udp::send_msg_to_enb_gtpu(srsran::byte_buffer_t* pdu)
  {
    std::cout << "-----cnw-send-gtpu-message---" << std::endl;
    // printf_data(pdu->msg, (pdu->N_bytes > 4 ? 4 : pdu->N_bytes));
    if (enb_id == 1)
    {
      if (sendto(sockfd, pdu->msg, pdu->N_bytes, 0,
                 (struct sockaddr *)&enb1_gtpu_clientaddr, sizeof(enb1_gtpu_clientaddr)) == -1)
      {
        std::cerr << "Failed to send data to enb1" << std::endl;
        return false;
      }
      else
      {
        std::cout << "Data sent to enb1 successfully" << std::endl;
      }
    }
    else if (enb_id == 2)
    {
      if (sendto(sockfd, pdu->msg, pdu->N_bytes, 0,
                 (struct sockaddr *)&enb2_gtpu_clientaddr, sizeof(enb2_gtpu_clientaddr)) == -1)
      {
        std::cerr << "Failed to send data to enb2" << std::endl;
        return false;
      }
      else
      {
        std::cout << "Data sent to enb2 successfully" << std::endl;
      }
    }

    return true;
  }

  bool udp::send_ttcn_msg_enb(srsran::unique_byte_buffer_t pdu)
  {
    std::cout << "------send_TTCN_msg_to_enb---" << std::endl;
    srsran::unique_byte_buffer_t enb_cnw_ttcn_pdu = srsran::make_byte_buffer();
    memcpy(enb_cnw_ttcn_pdu->msg, &srsepc::S1_ENB_CNW_TTCN, 1);
    memcpy(enb_cnw_ttcn_pdu->msg + 1, pdu->msg, pdu->N_bytes);
    enb_cnw_ttcn_pdu->N_bytes = pdu->N_bytes + 1;
    // printf_data(enb_cnw_ttcn_pdu->msg, enb_cnw_ttcn_pdu->N_bytes);
    if (enb_id == 1)
    {
      if (sendto(sockfd, enb_cnw_ttcn_pdu->msg, enb_cnw_ttcn_pdu->N_bytes, 0,
                 (struct sockaddr *)&enb1_clientaddr, sizeof(enb1_clientaddr)) == -1)
      {
        std::cerr << "Failed to send ttcn_data to enb1" << std::endl;
        return false;
      }
      else
      {
        std::cout << "ttcn_data sent to enb1 successfully" << std::endl;
      }
    }
    else if (enb_id == 2)
    {
      if (sendto(sockfd, enb_cnw_ttcn_pdu->msg, enb_cnw_ttcn_pdu->N_bytes, 0,
                 (struct sockaddr *)&enb2_clientaddr, sizeof(enb2_clientaddr)) == -1)
      {
        std::cerr << "Failed to send ttcn_data to enb2" << std::endl;
        return false;
      }
      else
      {
        std::cout << "ttcn_data sent to enb2 successfully" << std::endl;
      }
    }
    return true;
  }

  bool udp::handle_s1_msg(srsran::unique_byte_buffer_t &pdu)
  {
    std::cout << "handle_s1_msg" << std::endl;
    // printf_data(pdu->msg, pdu->N_bytes);
    srsran::unique_byte_buffer_t enb_pdu = srsran::make_byte_buffer();
    memcpy(enb_pdu->msg, pdu->msg, pdu->N_bytes);
    enb_pdu->N_bytes = pdu->N_bytes;
    uint8_t s1_type = pdu->msg[0];

    if (s1_type == S1_ENB_CNW_MSG)
    { // enb msg
      std::cout << "Received message from enb " << std::endl;
      received_enb_info.try_push(std::move(enb_pdu));
      received_enb_info_flag = true;
      return true;
    }
    if (s1_type == S1_ENB_CNW_TTCN)
    { // ttcn msg
      std::cout << "recived ttcn msg from enb" << std::endl;
      args->ttcn_test_enble = true;
      TTCN_TEST = true;

      srsran::unique_byte_buffer_t ttcn_cnw = srsran::make_byte_buffer();
      ttcn_cnw->N_bytes = pdu->N_bytes - 1;
      memcpy(ttcn_cnw->msg, pdu->msg + 1, ttcn_cnw->N_bytes);

      if (ttcn_cnw->N_bytes == 4)
      {
        std::cout << "ttcn_dbug_11" << std::endl;
        enble_flag(std::move(ttcn_cnw)); // not return message
      }
      else
      {
        read_pdu(std::move(ttcn_cnw));
        std::cout << "ttcn_dbug_12" << std::endl;
      }
      return true;
    }

    // other msg
    return false;
  }

  bool udp::send_ip_data_to_sdap(srsran::unique_byte_buffer_t ip_data)
  {
    srsran::unique_byte_buffer_t enb_pdu = srsran::make_byte_buffer();
    srsepc::enb_msg_header_t s1_header;
    s1_header.enb_id = enb_id;
    s1_header.rnti = 70;
    s1_header.msg_type = srsepc::enb_msg_type::ip_data;

    int len = sizeof(s1_header);

    memcpy(enb_pdu->msg, &s1_header, len);
    enb_pdu->N_bytes = len;
    memcpy(enb_pdu->msg + len, ip_data->msg, ip_data->N_bytes);
    enb_pdu->N_bytes += ip_data->N_bytes;

    send_enb_msg(std::move(enb_pdu));

    return true;
  }


} // namespace srsenb