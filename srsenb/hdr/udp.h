#ifndef SRSENB_UDP_H
#define SRSENB_UDP_H
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <iostream>
#include <queue>

#include "mutex"
#include "srsenb/hdr/enb.h"
#include "srscnw/hdr/udp.h"

namespace srsenb
{
    class udp
    {
    public:
        udp();
        ~udp()
        {
            if (sockfd != -1)
            {
                close(sockfd);
            }
        }

        /*
         *@ double beam
         */
        int pid = 0;
        int access_port1 = 0;
        int access_port2 = 0;
        bool trans_mib_config(int pid_, int access_port1_, int access_port2_);

        bool is_zhj_psch_test = false;

        bool TTCN_TEST = false;

        struct enble_ttcn_flag
        {
            uint8_t flag_type;
            uint8_t flag_value;
            uint16_t ttcn_testId;
        };
        enble_ttcn_flag enble_ttcn_flag_;

        typedef struct
        {
            uint8_t direction;
            uint8_t rec_lay_id;
            uint8_t des_layer_id;
            uint16_t test_id;
            uint16_t data_length;
        } General_interface;
        General_interface Gen_;

        enum module_flag
        {
            ttcn_nasmm_enble = 1,
            ttcn_nassm_enble,
            ttcn_rrc_enble,
            ttcn_sdap_enble,
            ttcn_pdcp_enble,
            ttcn_rlc_enble,
            ttcn_mac_enble
        };

        enum Beam_Type
        {
            Source_Beam,
            Target_Beam
        };

        bool is_Control = false;
        uint8_t voice_indicate = 0;
        uint8_t ue_category = 0;

        srsran::INTEGRITY_ALGORITHM_ID_ENUM asintegrity_algo;
        srsran::CIPHERING_ALGORITHM_ID_ENUM asencryption_algo;
        // PDCP
        bool is_um = false;
        bool ttcn_close_TC = false;

        // rrc
        bool RACH_Handover = false;
        bool is_connection = false;
        bool TC_71_is_paging_connection = false;
        bool TC_75_is_paging_refuse = false;
        bool TC_75_is_second_con_req = false;
        bool TC_79_is_paging_smc = false;
        bool TC_72_con_capability = false;
        bool TC_73_t300_timeout = false;
        bool TC_73_second_t300_con_req = false;
        bool TC_74_t302_timeout = false;
        bool TC_74_second_t302_con_req = false;
        bool TC_711_reconfig = false;
        bool TC_712_reconfig_update = false;
        bool TC_517_paging_success = false;
        bool TC_513_sib_barred = false;
        bool TC_513_mib_sib = false;
        bool TC_513_barred_con_req = false;
        int sib_barred_num = 0;
        bool TC_513_barred = false;
        bool TC_513_mac_not_barred = false;
        bool TC_513_mac_mib = false;
        bool TC_725_release_nodirection = false;
        bool TC_725_con_req = false;
        bool TC_725_second_release = false;
        int TC_725_release_num = 0;
        bool TC_725_release = false;
        bool TC_73_t300_is = false;
        bool TC_512_con_req = false;
        bool TC_512_qrexlevmin = false;
        bool TC_514_Release = false;
        bool TC_514_Con_Req = false;
        bool TC_515_Release = false;
        bool TC_515_Con_Req = false;
        bool TC_710_security_mode_failure = false;
        bool TC_516_is_paging_invalid = false;
        bool TC_713_reconfig_DRB = false;
        bool TC_714_DRB_Release = false;
        bool TC_722_ho_success=false;
        bool TC_722_ho_success_last=false;
		bool TC_628_RECONFIG=false;
        bool TC_628_RECONFIG_SUCCESS=false;
        bool TC_511_Con_Req=false;
        bool TC_715_reest_reconf=false;
        bool TC_715_BEAM2_REEST=false;
        bool TC_716_reconf_fail_first=false;
        bool TC_716_reconf_fail_last=false;
        bool TC_720_measure_report_first=false;
        bool TC_720_measure_report_last=false;

		
        // mac
        bool mac_con_req_flag = false;
        bool mac_push_pdu = false;
        bool mac_ss_flag = false;
        bool mac_con_req_flag2 = false;
        bool window_response_flag = true;
        int send_rach_count = 0;
        bool is_second_send_rach_testcase2 = false;
        bool TC_612_mac_crid_not_match = false;
        bool TC_615_mac_roid_not_match = false;
        bool TC_616_mac_ccch_logic_channel = false;
        bool TC_616_mac_push_pdu = false;
        // jjc
        bool TC_6115_mac_srnti_match = false;
        bool TC_6115_Info_To_TTCN = false;
        uint8_t TTCN_Srnti=0;

        bool TC_6116_closed_loop_power_control = false;
        bool TC_6116_Info_To_TTCN = false;
        uint8_t TTCN_sacch = 0;
        float pa_adjust_before = 0.0f;
        float pa_adjust_after = 0.0f;

        bool TC_6117_ul_tf_sync = false;
        bool TC_6117_Info_To_TTCN = false;
        int fa_adjust_before = 0;
        int fa_adjust_after = 0;
        float ta_adjust_before = 0.0f;
        float ta_adjust_after = 0.0f;
        //sxy
        bool TC_614_backoff = false;
        bool TC_6118_mac_ce_handover = false;
        bool TC_6118_mac_push_pdu = false;
        bool TC_6118_mac_push_pdu_2 = false;
        bool TC_617_map_dtch = false;
        bool TC_617_Info_To_TTCN = false;
        bool TC_6113_padding_dl = false;
        bool TC_6113_Info_To_TTCN = false;

        //*********TC618 start*******
        bool TC_618_mac_regular_pdu = false;
        bool TC_618_mac_regular_pdu_2 = false;
        bool TC_618_Info_To_TTCN = false;
        //*********TC618 end*******

        //*********TC619 start*******
        bool TC_619_mac_bsr_timer = false;
        bool TC_619_mac_bsr_timer_2 = false;
        bool TC_619_Info_To_TTCN = false;
        //*********TC619 end*******

        //*********TC6111 start*******
        bool TC_6111_mac_periodic_phr_timer = false;
        bool TC_6111_mac_periodic_phr_timer_2 = false;
        //*********TC6111 end*******

        //*********TC6112 start*******
        bool TC_6112_mac_pass_loss = false;
        bool TC_6112_mac_pass_loss_2 = false;
        //*********TC6112 end*******  

        //*********TC6114 start*******
        bool TC_6114_process_subheader_dl = false;
        bool TC_6114_Info_To_TTCN = false;
        //*********TC6114 end********

        //*********TC61119 start*******
        bool TC_6119_ul_mcs = false;
        bool TC_6119_ul_mcs_2 = false;
        bool TC_6119_Info_To_TTCN = false;
        //*********TC61119 end*******    
        
        // nas sm
        bool sm_pdu_accept = false;
        bool TC_91_sm_5_request = false;
        bool TC_92_sm_release = false;
        // sdap
        bool sdap_nhdr_tran = false;

        // nas mm
        bool is_reg_req_congestion = false;
        bool is_security_mode_command2 = false;
        bool is_reg_succecc_no_5gguti = false;
        bool is_dereg_reg_req_ue_switchon = false;
        bool is_dereg_normal = false;
        bool is_net_org_dereg_req_need_regreq_repeat = false;
        bool is_reg_rej_illegal_ue = false;      /*6-17---sgj_add*/
        bool is_reg_rej_plmn_not_allowed = false; /*7-26--sgj_add*/
        bool is_authencation_faileure_mac_code = false;
        bool is_registration_reject_Ue_attempt_five_count=false;
        bool is_authentication_reject=false;
        std::mutex rrc_receive_info_mutex;

        // TTCN Release  and paging flag
        bool is_ttcn_release_paging = false;
        int receive_ttcn_pdu_number = 0;
        bool is_voice_complete = false;
        int enter_Voice_Register_Req_num = 0;

        int test_f = 0;

        // TTCN -> Control End Flag
        bool is_first_registration_complete = false;
        bool is_second_registration_complete = false;

        bool is_2_reconfig_complete = false;
        bool is_3_reconfig_complete = false;
        bool is_4_reconfig_complete = false;
        void enble_testcase_end_info_flag(uint16_t testId);
        uint8_t reconfig_complete_number = 0;
        uint8_t registration_complete_number = 0;

        bool init(all_args_t *args_);
        int run_udp();
        void printf_zhj() { std::cout << "zhj start" << std::endl; };
        void printf_data(uint8_t *arr, int len);
        void read_pdu(srsran::unique_byte_buffer_t pdu_); // ��ͷ   ������
        void read_ate_pdu(srsran::unique_byte_buffer_t pdu_);
        void communication_test();

        void enble_flag(srsran::unique_byte_buffer_t pdu_);

        void ttcn_control_release(uint16_t testId);
        void Indicate_voice_complete_flag(uint16_t testId);

        bool send_msg_to_cnw(srsran::unique_byte_buffer_t pdu);
        bool notify_change_enb(uint16_t rnti);
        bool send_ate_msg(srsran::unique_byte_buffer_t pdu);
        bool send_ip_data_to_cnw(srsran::unique_byte_buffer_t pdu);
        bool send_TTCN_msg_to_cnw(srsran::unique_byte_buffer_t pdu);
        bool handle_s1_msg(srsran::unique_byte_buffer_t &pdu);

        all_args_t *args;

        // struct ttcn_info {
        //   uint8_t Direction;
        //   uint8_t Rec_Layer_id;
        //   uint8_t Des_layer_id;
        //   uint8_t message_typeis_authentication_reject;
        //   uint16_t Length;
        //   srsran::unique_byte_buffer_t pdu;
        // };

        // std::queue<srsran::unique_byte_buffer_t> rrc_receive_info;

        srsran::dyn_blocking_queue<srsran::unique_byte_buffer_t>
            stack_receive_ate_info; // Message sent to the nas layer   00
        srsran::dyn_blocking_queue<srsran::unique_byte_buffer_t>
            nas_sm_receive_info; // Message sent to the nas layer   01
        srsran::dyn_blocking_queue<srsran::unique_byte_buffer_t>
            nas_mm_receive_info; // Message sent to the nas layer   02
        srsran::dyn_blocking_queue<srsran::unique_byte_buffer_t>
            rrc_receive_info; // Message sent to the rrc layer      03

        srsran::dyn_blocking_queue<srsran::unique_byte_buffer_t>
            sdap_receive_info; //                                    04
        srsran::dyn_blocking_queue<srsran::unique_byte_buffer_t>
            pdcp_receive_info; //                                    05
        srsran::dyn_blocking_queue<srsran::unique_byte_buffer_t>
            mac_receive_info; // Message sent to the nas layer      06

        srsran::dyn_blocking_queue<srsran::unique_byte_buffer_t>
            sdap_receive_icmp_info; //
        bool sdap_receive_icmp_flag = true;
        srsran::dyn_blocking_queue<srsran::unique_byte_buffer_t>
            received_cnw_info;
        bool icmp_receive_sdap_flag = true;

        srsran::dyn_blocking_queue<srsran::unique_byte_buffer_t> icmp_receive_iperf_info;

        // The queue used to send messages to the TTCN
        srsran::dyn_blocking_queue<srsran::unique_byte_buffer_t> send_ttcn_info;

        srsran::dyn_blocking_queue<srsran::unique_byte_buffer_t> send_ate_info;

        srsran::dyn_blocking_queue<srsran::unique_byte_buffer_t> source_beam_trans_queue;
        srsran::dyn_blocking_queue<srsran::unique_byte_buffer_t> source_beam_receive_queue;

        srsran::dyn_blocking_queue<srsran::unique_byte_buffer_t> target_beam_trans_queue;
        srsran::dyn_blocking_queue<srsran::unique_byte_buffer_t> target_beam_receive_queue;

        int paging_count_num = 0;
        bool second_ttcn_paging = false;

        // 4.10 XK
        bool pdcp_ttcn_test = false;
        bool sdap_ttcn_test = false;

    private:
        int sockfd;
        int port;
        struct sockaddr_in serverAddr_;
        struct sockaddr_in ate_clientaddr;
        struct sockaddr_in Target_beam_clientaddr;
        struct sockaddr_in cnw_clientaddr;
        struct sockaddr_in ttcn_clientaddr;
        struct sockaddr_in control_clientaddr;
    };
} // namespace srsenb

#endif