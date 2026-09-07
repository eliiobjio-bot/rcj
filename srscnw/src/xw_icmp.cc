#include <stdio.h>
#include <stdlib.h>
#include <array>
#include <chrono>
#include <netinet/ip_icmp.h>
#include <string.h>
#include <unistd.h> // 包含 getpid 函数的头文件
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "srscnw/hdr/xw_icmp.h"

#include "srscnw/hdr/nas_context.h"
#include "srsran/asn1/nas_5g_msg.h"

using namespace std;

namespace srsepc {


xw_icmp*             xw_icmp::m_instance    = NULL;
pthread_mutex_t     xw_icmp_instance_mutex = PTHREAD_MUTEX_INITIALIZER;

xw_icmp::xw_icmp() : m_running(false), thread("XW_ICMP")
{
  return;
}

xw_icmp::~xw_icmp()
{
  return;
}

xw_icmp* xw_icmp::get_instance(void)
{
  pthread_mutex_lock(&xw_icmp_instance_mutex);
  if (NULL == m_instance) {
    m_instance = new xw_icmp();
  }
  pthread_mutex_unlock(&xw_icmp_instance_mutex);
  return (m_instance);
}

void xw_icmp::cleanup(void)                                                                                                                                                                           
{
  pthread_mutex_lock(&xw_icmp_instance_mutex);
  if (NULL != m_instance) {
    delete m_instance;
    m_instance = NULL;
  }
  pthread_mutex_unlock(&xw_icmp_instance_mutex);
}

void xw_icmp::stop()
{
  if (m_running) {
    m_running = false;
    thread_cancel();
    wait_thread_finish();
  }
  xw_tun_->stop();
  return;
}

int xw_icmp::init(adp* adp_)
{	
	icmp_adp = adp_;
	icmp_cnw = cnw::get_instance();
	start();

	//init tun
	xw_tun_ = xw_tun::get_instance();
	xw_tun_->init(adp_);

	return 0;
}

void xw_icmp::run_thread()
{
	m_running = true;
	srsran::unique_byte_buffer_t pdu = srsran::make_byte_buffer("xw_icmp::run_thread");
	srsran::unique_byte_buffer_t icmp_ate_msg = srsran::make_byte_buffer();
	srsran::unique_byte_buffer_t icmp_req_msg = srsran::make_byte_buffer();
	srsran::unique_byte_buffer_t iperf_to_ue = srsran::make_byte_buffer();

	srsran::unique_byte_buffer_t s1_msg = srsran::make_byte_buffer();	

	std::cout<< endl << endl;
	std::cout<<"xw_icmp::run_thread ing"<<std::endl;

	while (m_running==true) {
		
		wait_start_imp();
		if(dl_ping_flag==true){
			icmp_req_msg=srsran::make_byte_buffer();
			pack_icmp_pdu(icmp_req_msg);
			for(int i=0; i<4; i++) {
				printf("icmp_sdap_mag[%d]: %x\n", i, icmp_req_msg->msg[i]);
			}
			printf("icmp_req_msg->N_bytes%d\n", icmp_req_msg->N_bytes);
			// icmp_adp->udp_.send_ip_data_to_sdap(std::move(icmp_req_msg));
			icmp_adp->udp_.send_msg_to_enb_gtpu(icmp_req_msg.get());
			dl_ping_flag = false;
		}

		if(reprot_rate_flag==true){
			icmp_ate_msg=srsran::make_byte_buffer();
			pack_report_info(icmp_ate_msg);
			for(int i=0; i<(int)icmp_ate_msg->N_bytes; i++) {
				printf("icmp_ate_data_rate_msg[%d]: %x\n", i, icmp_ate_msg->msg[i]);
			}
			icmp_adp->udp_.send_ate_msg(std::move(icmp_ate_msg));
			reprot_rate_flag = false;
		}

		if(reprot_ping_last_flag==true){
			icmp_ate_msg=srsran::make_byte_buffer();
			pack_last_ping_report_info(icmp_ate_msg);
			for(int i=0; i<(int)icmp_ate_msg->N_bytes; i++) {
				printf("icmp_ate_last_ping_msg[%d]: %x\n", i, icmp_ate_msg->msg[i]);
			}
			icmp_adp->udp_.send_ate_msg(std::move(icmp_ate_msg));
			reprot_ping_last_flag = false;
		}
		// usleep(100);//delayTime
	}
	
	return;
}

bool xw_icmp::pack_report_info(srsran::unique_byte_buffer_t& icmp_ate_msg)
{

	//print now time
	auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    // 转换为毫秒
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    std::cout << "Current timestamp in milliseconds: " << millis << std::endl;
	uint16_t report_time;
	if(last_time==0) 
	{
		last_time = millis;
		report_time = 3000;
	}
	else 
	{
		report_time = (millis-last_time);
		last_time = millis;
	}
	std::cout << "report_time: " << report_time << std::endl;

	icmp_ate_msg->msg[0]=0xff;
	icmp_ate_msg->msg[1]=0x03;
	icmp_ate_msg->msg[2]=0x03;
	uint32_t dl_size = dl_data_size;
	icmp_ate_msg->msg[3] = (dl_size >> 24) & 0xFF;	
	icmp_ate_msg->msg[4] = (dl_size >> 16) & 0xFF;
	icmp_ate_msg->msg[5] = (dl_size >> 8) & 0xFF;
	icmp_ate_msg->msg[6] = dl_size & 0xFF;
	icmp_ate_msg->msg[7] = (report_time >> 8) & 0xFF;
	icmp_ate_msg->msg[8] = report_time & 0xFF;
	uint32_t ul_size = ul_data_size;
	icmp_ate_msg->msg[9] = (ul_size >> 24) & 0xFF;	
	icmp_ate_msg->msg[10] = (ul_size >> 16) & 0xFF;
	icmp_ate_msg->msg[11] = (ul_size >> 8) & 0xFF;
	icmp_ate_msg->msg[12] = ul_size & 0xFF;	
	icmp_ate_msg->msg[13] = (report_time >> 8) & 0xFF;
	icmp_ate_msg->msg[14] = report_time & 0xFF;
	icmp_ate_msg->N_bytes=15;

	//clear 
	ul_data_size = 0;
	dl_data_size = 0;
	
	return true;

}

bool xw_icmp::pack_last_ping_report_info(srsran::unique_byte_buffer_t& icmp_ate_msg)
{

	icmp_ate_msg->msg[0]=0xff;
	icmp_ate_msg->msg[1]=0x03;
	icmp_ate_msg->msg[2]=TC_MSG_ICMP_ATE_PING_INFO_LAST;
	icmp_ate_msg->msg[3] = (ping_send_num >> 8) & 0xFF;
	icmp_ate_msg->msg[4] = ping_send_num & 0xFF;
	icmp_ate_msg->msg[5] = (ping_recv_num >> 8) & 0xFF;
	icmp_ate_msg->msg[6] = ping_recv_num & 0xFF;	
	icmp_ate_msg->N_bytes=7;

	//clear 
	ping_send_num=0;
	ping_recv_num=0;

	return true;

}


bool xw_icmp::pack_icmp_pdu(srsran::unique_byte_buffer_t& pdu)
{
    //pdu = ip_header(20o) + icmp_pdu(8o + ...);
	//set ip header
	struct iphdr ip;
    memset(&ip, 0, sizeof(ip));
    ip.ihl = 5;
    ip.version = 4;
    ip.tos = 0;
    ip.id = htons(1);
    ip.frag_off = 0;
    ip.ttl = 64;
    ip.protocol = IPPROTO_ICMP;
    ip.saddr = inet_addr("1.127.11.1");	//网络ip
    ip.daddr = inet_addr("1.127.11.10");	//终端ip

	//set icmp data
    size_t len = sizeof(struct icmp);
    socklen_t dstlen = sizeof(struct sockaddr_in);
    struct icmp *echo;
    echo = reinterpret_cast<struct icmp*>(pdu->msg + sizeof(ip));
    echo->icmp_type = ICMP_ECHOREQ;
    echo->icmp_code = 0;
    echo->icmp_cksum = 0;
    echo->icmp_id = getpid();
    echo->icmp_seq = ++ping_send_num;
    struct timeval* tval = reinterpret_cast<struct timeval*>(echo->icmp_data);
    auto now = std::chrono::system_clock::now();
    auto sec = std::chrono::time_point_cast<std::chrono::seconds>(now);
    auto usec = now - sec;
    tval->tv_sec = sec.time_since_epoch().count();
    tval->tv_usec = std::chrono::duration_cast<std::chrono::microseconds>(usec).count();
	cout<<" struct timeval size: "<< sizeof(struct timeval)<<endl;
	cout<<" tval->tv_sec: "<< tval->tv_sec<<endl;
	cout<<" tval->tv_usec: "<< tval->tv_usec<<endl;
	uint16_t pack_data_size = packsize + add_data_size;
    echo->icmp_cksum = cal_cksum(reinterpret_cast<uint16_t*>(echo), pack_data_size);

	//compute ip header check
	pdu->N_bytes=sizeof(ip)+ pack_data_size;
	ip.tot_len = htons((uint16_t)pdu->N_bytes);
	uint16_t check_ip_sum = calc_ip_checksum((uint8_t*)&ip, 20);
	ip.check = htons(check_ip_sum);
    memcpy(pdu->msg, &ip, sizeof(ip));

    return true;
}


bool xw_icmp::pack_icmpechoreply_pdu(srsran::unique_byte_buffer_t& pdu, srsran::unique_byte_buffer_t& reply_pdu)
{
	if(pdu->N_bytes<23) return false;

	// 复制 ICMP 请求报文的数据部分（数据部分通常是回显请求的数据）
	memcpy(reply_pdu->msg, pdu->msg, pdu->N_bytes);
	reply_pdu->N_bytes=pdu->N_bytes;
	
	// 设置 ICMP 类型为回显应答
	reply_pdu->msg[20] = ICMP_ECHOREPLY;
	
	//设置src_ip & dec_ip
	reply_pdu->msg[12] = pdu->msg[16];
	reply_pdu->msg[13] = pdu->msg[17];
	reply_pdu->msg[14] = pdu->msg[18];
	reply_pdu->msg[15] = pdu->msg[19];

	reply_pdu->msg[16] = pdu->msg[12];
	reply_pdu->msg[17] = pdu->msg[13];
	reply_pdu->msg[18] = pdu->msg[14];
	reply_pdu->msg[19] = pdu->msg[15];

	//计算 IP头部校验和
	reply_pdu->msg[10] = 0;
	reply_pdu->msg[11] = 0;		
	uint16_t check_ip_sum = calc_ip_checksum((uint8_t*)&reply_pdu->msg[0], 20);
	reply_pdu->msg[10] = check_ip_sum >> 8;
	reply_pdu->msg[11] = check_ip_sum & 0xFF;

	// 计算 ICMP 校验和
	reply_pdu->msg[22] = 0;
	reply_pdu->msg[23] = 0;	
	uint16_t checksum = cal_cksum((uint16_t*)&reply_pdu->msg[20], reply_pdu->N_bytes - 20);
	reply_pdu->msg[23] = checksum >> 8;
	reply_pdu->msg[22] = checksum & 0xFF;

	return true;

}

void xw_icmp::send_ate_ping_info(uint16_t rrt_timers, uint8_t data_size) 
{
	srsran::unique_byte_buffer_t ate_ping_info = srsran::make_byte_buffer();
	if(ate_ping_info==NULL) {
		return;
	}
	ate_ping_info->msg[0] = 0Xff;
	ate_ping_info->msg[1] = 0X03;
	ate_ping_info->msg[2] = TC_MSG_ICMP_ATE_PING_INFO;
	ate_ping_info->msg[3] = (rrt_timers >> 8) & 0XFF;
	ate_ping_info->msg[4] = rrt_timers & 0XFF;
	ate_ping_info->msg[5] = data_size;

	ate_ping_info->N_bytes = 6;

	for(int i=0; i<(int)ate_ping_info->N_bytes; i++) {
		printf("icmp_ate_ping_msg[%d]: %x\n", i, ate_ping_info->msg[i]);
	}

	icmp_adp->udp_.send_ate_msg(std::move(ate_ping_info));

	return;
}

bool xw_icmp::handle_ip_pdu(srsran::unique_byte_buffer_t& pdu)
{
	cout<<"This ip handle function!"<<endl;
    if(pdu->N_bytes<20) {
        cout<<"ip pdu size err!"<<endl;
        return false;
    }
	uint8_t ip_type = pdu->msg[9];

    if(ip_type == IPPROTO_ICMP)
	{
		// 获取 ICMP 报文的类型和代码
		uint8_t type = pdu->msg[20];
		uint8_t code = pdu->msg[21];

		// 根据 ICMP 报文的类型和代码进行判断和处理
		if (type == ICMP_ECHOREPLY && code == 0) {    // 处理 ICMP 回显应答报文
			cout<<"This icmp msg is ICMP_ECHOREPLY!"<<endl;
			recv_ping(pdu);
		}
    	else if (type == ICMP_ECHOREQ && code == 0) {  // 处理 ICMP 回显请求报文 上行ping包
			cout<<"This icmp msg is ICMP_ECHOREQ!"<<endl;
			// sent to tun
			int n = write(xw_tun_->get_sgi(), pdu->msg, pdu->N_bytes);
			std::cout<< "write to tun interface" << n<<std::endl;
			if(n<0) {
				std::cout<<"could not write to tun interface."<<std::endl;
			}
		}
		else {// 其他类型的 ICMP 报文
			std::cout<<"error : icmp type is unknown"<<std::endl;
		}
	}
	else {	//目前是终端发上来的ip data
		// estimate pdu content

		//record ul data
		ul_data_size+=(pdu->N_bytes);
		cout<< "ul_data_size: "<<ul_data_size<<endl;

		// sent to tun
		int n = write(xw_tun_->get_sgi(), pdu->msg, pdu->N_bytes);
		std::cout<< "write to tun interface" << n<<std::endl;
		if(n<0) {
			std::cout<<"could not write to tun interface."<<std::endl;
		}
	}

    return true;
}

bool xw_icmp::handle_ate_pdu(srsran::unique_byte_buffer_t& pdu)
{	
	if(pdu->N_bytes<4) return false;
	if(pdu->msg[2] == TC_MSG_ATE_ICMP_CONTROL) {
		switch(pdu->msg[3]) {
			case control_type_t::CLOSED_PING:
				xw_timer_delete(dl_timerid);	//删除ul定时器（停止速率测试）
				//上报ping包最终结果
				xw_timer_creater_ping_last();
				xw_timer_settime_last_ping();
				printf("INFO: icmp received CLOSED_PING!\n");
				break;

			case control_type_t::OPEN_PING:
				ping_recv_num = 0;
				ping_send_num = 0;

				{
					auto start_time = std::chrono::high_resolution_clock::now();
					while (true)
					{
						auto now_time = std::chrono::high_resolution_clock::now();
						auto duration = std::chrono::duration_cast<std::chrono::seconds>(now_time - start_time);
						if (duration.count() > 2)
						{
							std::cout << " 2 !!!" << std::endl;
							break;
						}
					}
				}

				xw_timer_creater_dl();
				xw_timer_settime(dl_timerid, 500);	//开启ping定时器	间隔200ms发送一次
				printf("INFO: icmp received OPEN_PING!\n");
				break;

			case control_type_t::OPEN_TEST_RATE_RRC:	//会触发rrc重配
				icmp_state = ICMP_STATE_IPERF;
				{
					auto start_time = std::chrono::high_resolution_clock::now();
					while (true)
					{
						auto now_time = std::chrono::high_resolution_clock::now();
						auto duration = std::chrono::duration_cast<std::chrono::seconds>(now_time - start_time);
						if (duration.count() > 2)
						{
							std::cout << " 2 !!!" << std::endl;
							break;
						}
					}
				}		
				ul_data_size = 0;
				dl_data_size = 0;
				enabled_data_send = true;
				xw_timer_creater_ul();
				xw_timer_settime(ul_timerid, report_time_msec);	//开启上报定时器
				printf("INFO: icmp received OPEN_TEST_RATE_RRC!\n");
				break;

			case control_type_t::CLOSED_TEST_RATE:
				enabled_data_send = false;
				icmp_state = ICMP_STATE_NULL;
				xw_timer_delete(ul_timerid);	//删除ul定时器（停止速率测试）
				printf("INFO: icmp received CLOSED_TEST_RATE!\n");
				break;
			
			default:
				printf("ERROR: icmp received unknown control type!\n");
				break;
		}
		//CONTROL RES
		srsran::unique_byte_buffer_t icmp_ate_msg=srsran::make_byte_buffer();
		icmp_ate_msg->msg[0] = 0xff;
		icmp_ate_msg->msg[1] = 0x03;
		icmp_ate_msg->msg[2] = 0x02;
		icmp_ate_msg->N_bytes=3;
		icmp_adp->udp_.send_ate_msg(std::move(icmp_ate_msg));
	}

    return true;
}

uint16_t xw_icmp::cal_cksum(uint16_t *addr, int len)
{
	int nleft = len;
	uint32_t sum = 0;
	uint16_t *w = addr;
	uint16_t answer = 0;

	while (nleft > 1) {
		sum += *w++;
		nleft -= 2;
	}

	if (nleft == 1) {
		*(unsigned char *)(&answer) = *(unsigned char *)w ;
		sum += answer;
	}

	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	answer = ~sum;
	return(answer);
}

bool xw_icmp::xw_timer_creater_ul()
{
	struct sigevent sev;

    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = ul_timer_handler;
    sev.sigev_notify_attributes = NULL;
    sev.sigev_value.sival_ptr = NULL;

    timer_create(CLOCK_REALTIME, &sev, &ul_timerid);

	return true;
}

bool xw_icmp::xw_timer_creater_ping_last()
{
	struct sigevent sev;

    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = last_ping_timer_handler;
    sev.sigev_notify_attributes = NULL;
    sev.sigev_value.sival_ptr = NULL;

    timer_create(CLOCK_REALTIME, &sev, &last_ping_timerid);

	return true;
}

bool xw_icmp::xw_timer_creater_dl()
{
	struct sigevent sev;

    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = dl_timer_handler;
    sev.sigev_notify_attributes = NULL;
    sev.sigev_value.sival_ptr = NULL;

    timer_create(CLOCK_REALTIME, &sev, &dl_timerid);

	return true;
}


bool xw_icmp::xw_timer_delete(timer_t timerid)
{
	timer_delete(timerid);

	return true;
}

bool xw_icmp::xw_timer_settime(timer_t timerid, uint16_t num_msec)
{

	struct itimerspec its;
	if(num_msec<1000) {
		its.it_value.tv_sec = 0; 
		its.it_value.tv_nsec = num_msec*1000000; // 设置定时器到期时间为num_msec秒
		its.it_interval.tv_sec = 0; 
		its.it_interval.tv_nsec = num_msec*1000000; // 以后每次间隔num_msec触发定时器超时函数
	}
	else {
		its.it_value.tv_sec = num_msec/1000; 
		its.it_value.tv_nsec = (num_msec-(num_msec/1000)*1000)*1000000; // 设置定时器到期时间为num_msec秒
		its.it_interval.tv_sec = its.it_value.tv_sec; 
		its.it_interval.tv_nsec = its.it_value.tv_nsec; // 以后每次间隔num_msec触发定时器超时函数
	}

	timer_settime(timerid, 0, &its, NULL);

	return true;
}

bool xw_icmp::xw_timer_settime_last_ping()
{
	struct itimerspec its;

	its.it_value.tv_sec = 2000/1000; 
	its.it_value.tv_nsec = 0; // 设置定时器到期时间为num_msec秒
	its.it_interval.tv_sec = 0; 
	its.it_interval.tv_nsec = 0; // 以后每次间隔num_msec触发定时器超时函数

	timer_settime(last_ping_timerid, 0, &its, NULL);

	return true;
}


void xw_icmp::ul_timer_handler(sigval_t sigval)
{
	if(xw_icmp::m_instance == NULL) return;
	m_instance->reprot_rate_flag = true;
	m_instance->start_imp();

}

void xw_icmp::dl_timer_handler(sigval_t sigval)
{
	if(xw_icmp::m_instance == NULL) return;
	m_instance->dl_ping_flag = true;
	m_instance->start_imp();

}

void xw_icmp::last_ping_timer_handler(sigval_t sigval)
{
	if(xw_icmp::m_instance == NULL) return;
	m_instance->reprot_ping_last_flag = true;
	m_instance->start_imp();

}

uint16_t xw_icmp::calc_ip_checksum(const uint8_t* header, size_t header_len) 
{
    uint32_t sum = 0;
    
    // 将头部按 16 位字为单位进行累加
    for (size_t i = 0; i < header_len; i += 2) {
        sum += (header[i] << 8) + header[i + 1];
    }
    
    // 将高位溢出的部分加到低位上
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    // 取反得到校验和
    return ~sum;
}

void xw_icmp::recv_ping(srsran::unique_byte_buffer_t& pdu)
{

	ssize_t n = pdu->N_bytes;
	struct ip *ip;
	struct icmp *icmp;
	int ttl;
	struct timeval* tvsend;
	struct timeval tvrecv;
	int time;

    auto now = std::chrono::system_clock::now();
    auto sec = std::chrono::time_point_cast<std::chrono::seconds>(now);
    auto usec = now - sec;
    tvrecv.tv_sec = sec.time_since_epoch().count();
    tvrecv.tv_usec = std::chrono::duration_cast<std::chrono::microseconds>(usec).count();

	// check if icmp
	ip = (struct ip*)pdu->msg;
	ttl = ip->ip_ttl;

	// check if icmp packet
	if(ip->ip_p != IPPROTO_ICMP)
	{
		printf("ICMP error: Not ICMP Protocol!\n");
		return;
	}

	// check if icmp reply
	icmp = (struct icmp*)(pdu->msg + sizeof(struct ip));
	if(icmp->icmp_type == ICMP_ECHOREPLY)
	{

		tvsend = (struct timeval *)icmp->icmp_data;
		tv_sub(&tvrecv, tvsend);
		time = tvrecv.tv_sec * 1000 + tvrecv.tv_usec / 1000;
		ping_recv_num++;		
		printf("\tReply from %s: bytes = %ld time = %dms TTL = %d\n", "0.0.0.0", n, time, ttl);
		//上报结果
		send_ate_ping_info(time, n);
	}
	else
	{
		printf("ICMP error: Not ICMP Reply Message!\n");
		return;
	}

}

void xw_icmp::tv_sub(struct timeval *out,struct timeval *in)
{       
	if((out->tv_usec -= in->tv_usec) < 0)
	{       
		--out->tv_sec;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}


}
