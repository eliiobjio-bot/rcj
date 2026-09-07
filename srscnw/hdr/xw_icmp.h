#ifndef XW_PING_H
#define XW_PING_H

#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include "srsran/common/buffer_pool.h"
#include "srsran/common/standard_streams.h"
#include "srsran/common/threads.h"
#include "srsran/asn1/s1ap_utils.h"
#include <iostream>
#include <vector>
#include <map>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include <condition_variable>
#include <mutex>

#include "srscnw/hdr/cnw.h"
#include "srscnw/hdr/adp.h"
#include "srscnw/hdr/xw_tun.h"

#include "lib/include/srsran/interfaces/enb_rrc_interfaces.h" //-------2023/11/14


namespace srsepc {

class cnw;
class xw_tun;

#define ICMP_ECHOREPLY 0
#define ICMP_ECHOREQ 8

#define MAX_ICMP_COUNT 20


#define UE_IP_V4 "1.127.11.10"


class xw_icmp: public srsran::thread
{
private:
    /* data */
    bool   m_running;
    static xw_icmp* m_instance;
    cnw *icmp_cnw;
    adp* icmp_adp;
    // xw_tun *xw_tun_;


    bool dl_ping_flag = false;
    bool reprot_rate_flag = false;
    bool reprot_ping_last_flag = false;
    bool start_report_flag = false;

    timer_t ul_timerid; //上行速率定时器
    timer_t dl_timerid; //下行ping包定时器
    timer_t last_ping_timerid; //收到ping包停止消息，触发

    std::condition_variable              icmp_imp  = {};
    std::mutex                           icmp_mutex = {};
    bool ready = false;
  
    uint64_t last_time=0;
    uint16_t report_time_msec = 3000;

    srsran::unique_byte_buffer_t icmp_rate_pdu=srsran::make_byte_buffer();
    int packsize = ICMP_MINLEN + sizeof(struct timeval);  //icmp head and timev
    int add_data_size=20;

    bool m_sgi_up = false;
    int m_sgi;

    // statistics
    int ping_send_num = 0, ping_recv_num = 0;   //nsend send_num  nrecv receive_num
    int rrt[10];
  
    typedef enum 
    { 
        TC_ICMP_MSG_NULL=0, 
        TC_MSG_ATE_ICMP_CONTROL, // ATE control ICMP start ping packet or end ping packet
        TC_MSG_ATE_ICMP_CONTROL_RES, // ICMP reply ATE layer control command executed
        TC_MSG_ICMP_ATE_INFO, // ICMP report ping packet rate
        TC_MSG_ICMP_ATE_PING_INFO,
        TC_MSG_ICMP_ATE_PING_INFO_LAST,
        TC_ICMP_MSG_MAX,
    } TC_ICMP_MSG;


public:
    xw_icmp();
    virtual ~xw_icmp();

    static xw_icmp* get_instance(void);
    static void cleanup(void);

    int  init(adp* adp_ );
    void stop();
    void run_thread();

    void start_imp() {
        std::lock_guard<std::mutex> lock(icmp_mutex);  
        ready = true; 
        icmp_imp.notify_one(); // 唤醒等待的线程  
    }
    
    void wait_start_imp() {
        std::unique_lock<std::mutex> lock(icmp_mutex);  
        std::cout << "wait indication...\n"; 
        while(!ready) {
            icmp_imp.wait(lock); // 等待条件变量  
            std::cout << "icmp receive indication, start implement..\n"; 
        }
        ready = false;
    }

    xw_tun *xw_tun_;
    uint32_t dl_data_size=0;
    uint32_t ul_data_size=0;

    bool enabled_data_send = false;
    enum ICMP_STATE{ICMP_STATE_NULL = 0, ICMP_STATE_IPERF};
    int icmp_state = ICMP_STATE_NULL;
    
    bool handle_ip_pdu(srsran::unique_byte_buffer_t& pdu);
    bool handle_ate_pdu(srsran::unique_byte_buffer_t& pdu);
    void recv_ping(srsran::unique_byte_buffer_t& pdu);

    bool pack_icmp_pdu(srsran::unique_byte_buffer_t& pdu);
    bool pack_report_info(srsran::unique_byte_buffer_t& pdu);
    bool pack_last_ping_report_info(srsran::unique_byte_buffer_t& icmp_ate_msg);
    bool pack_icmpechoreply_pdu(srsran::unique_byte_buffer_t& pdu, srsran::unique_byte_buffer_t& reply_pdu);
    void send_ate_ping_info(uint16_t rrt_timers, uint8_t data_size);

    bool xw_timer_creater_ul();
    bool xw_timer_creater_dl();
    bool xw_timer_creater_ping_last();
    bool xw_timer_delete(timer_t timerid);
    bool xw_timer_settime(timer_t timerid, uint16_t num_sec);
    bool xw_timer_settime_last_ping();
    static void ul_timer_handler(sigval_t sigval);
    static void dl_timer_handler(sigval_t sigval);
    static void last_ping_timer_handler(sigval_t sigval);

    uint16_t cal_cksum(uint16_t *addr, int len);
    uint16_t calc_ip_checksum(const uint8_t* header, size_t header_len);

    void tv_sub(struct timeval *out,struct timeval *in);

};

class tc_msg_ate_icmp_control_t   //ate --> icmp
{
public:
    uint8_t message_source_type;
    uint8_t protocol_type;
    uint8_t message_type;
    uint8_t control_command;
};

class control_type_t   //ate --> icmp
{
public:
    static const uint8_t CLOSED_PING=0X00;
    static const uint8_t OPEN_PING=0X01;
    static const uint8_t OPEN_TEST_RATE_RRC=0X02;
    static const uint8_t CLOSED_TEST_RATE=0X03;
};

class tc_msg_ate_icmp_control_res_t   //icmp --> ate  
{
public:
    uint8_t message_source_type;
    uint8_t protocol_type;
    uint8_t message_type;
};


struct udp_pseudo_header_t
{
    uint32_t sourceAddress;
    uint32_t destAddress;
    uint8_t  placeholder;
    uint8_t  protocol;
    uint16_t udpLength;

};


}

#endif