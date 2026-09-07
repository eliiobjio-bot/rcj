#ifndef XW_TUN_H
#define XW_TUN_H

#include "srsran/common/buffer_pool.h"
#include "srsran/common/standard_streams.h"
#include "srsran/common/threads.h"
#include <iostream>
#include <vector>
#include <map>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include "srscnw/hdr/adp.h"
#include "srsran/srslog/srslog.h"


namespace srsepc {

class cnw;

const std::string TUN_IP = "1.127.11.1";
const std::string TUN_NAME = "tun0";
const uint16_t GTPU_RX_PORT = 2152;

class Args {
public:
    std::string sgi_if_name;
    std::string sgi_if_addr;
};

class xw_tun: public srsran::thread
{
private:
    /* data */
    bool   m_running;
    cnw *tun_cnw;
    adp* tun_adp;
    static xw_tun* m_instance;

    bool m_sgi_up = false;
    int m_sgi;


public:
    xw_tun();
    virtual ~xw_tun();

    static xw_tun* get_instance(void);
    static void cleanup(void);

    int  init(adp* adp_ );
    void stop();
    void run_thread();

    int init_sgi_interface(const Args* args);
    bool set_sockaddr(struct sockaddr_in* addr, const char* ip, int port);
    int get_sgi();
    
    void handle_sgi_pdu(srsran::unique_byte_buffer_t msg);
    void send_s1u_pdu(srsran::gtp_fteid_t enb_fteid, srsran::byte_buffer_t* msg);

    bool send_tun_msg(srsran::unique_byte_buffer_t tun_pdu);


    srslog::basic_logger& m_logger = srslog::fetch_basic_logger("TUN");

};



}

#endif