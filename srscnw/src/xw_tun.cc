#include <stdio.h>
#include <stdlib.h>
#include <array>
#include <chrono>
#include <string.h>
#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/if_tun.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "srscnw/hdr/xw_tun.h"
#include "srscnw/hdr/cnw.h"

#include "srsran/upper/gtpu.h"
#include "srsran/common/string_helpers.h"

using namespace std;

namespace srsepc {


#define xw_SRSRAN_ERROR_ALREADY_STARTED -1
#define xw_SRSRAN_ERROR_CANT_START -2
#define xw_SRSRAN_SUCCESS 0

xw_tun*             xw_tun::m_instance    = NULL;
pthread_mutex_t     xw_tun_instance_mutex = PTHREAD_MUTEX_INITIALIZER;

xw_tun::xw_tun() : m_running(false), thread("XW_ICMP")
{
  return;
}

xw_tun::~xw_tun()
{
  return;
}

xw_tun* xw_tun::get_instance(void)
{
  pthread_mutex_lock(&xw_tun_instance_mutex);
  if (NULL == m_instance) {
    m_instance = new xw_tun();
  }
  pthread_mutex_unlock(&xw_tun_instance_mutex);
  return (m_instance);
}

void xw_tun::cleanup(void)                                                                                                                                                                           
{
  pthread_mutex_lock(&xw_tun_instance_mutex);
  if (NULL != m_instance) {
    delete m_instance;
    m_instance = NULL;
  }
  pthread_mutex_unlock(&xw_tun_instance_mutex);
}

void xw_tun::stop()
{
  if (m_running) {
    m_running = false;
    thread_cancel();
    wait_thread_finish();
  }
  return;
}

int xw_tun::init(adp* adp_)
{	
	tun_adp = adp_;
	tun_cnw = cnw::get_instance();
	start();

	return 0;
}

void xw_tun::run_thread()
{
	m_running = true;
	srsran::unique_byte_buffer_t rev_pdu = srsran::make_byte_buffer("xw_tun::run_thread");
    std::cout<< endl << endl;
    std::cout << " xw_tun::run_thread ing" << std::endl;

	Args args;
	char buffer[2048];
    args.sgi_if_name = TUN_NAME; // Adjust the interface name as needed
    args.sgi_if_addr = TUN_IP; // Adjust the interface address as needed

  int result = init_sgi_interface(&args);
  if (result == xw_SRSRAN_SUCCESS) {
    std::cout << "SGi interface initialized successfully." << std::endl;
  } else {
    std::cerr << "Failed to initialize SGi interface." << std::endl;
  }

	while (m_running==true) {
    if(m_sgi_up == true) {
      int nread = read(m_sgi, buffer, sizeof(buffer));
      if (nread < 0) {
        // std::cerr << "Error reading from TUN device: " << strerror(errno) << std::endl;
      }

      std::cout << "Read " << (int)nread << " bytes from TUN device" << std::endl;
      for (int i = 0; i < 30; ++i) {
        printf("%x ", (unsigned int)(unsigned char)buffer[i]);
      }
      printf("\n");
      
      //judge ip info , ue status

      //direct send to sdap (temporary)
      if(buffer[0]==0x45) {
        if(tun_cnw->m_xw_icmp->icmp_state == xw_icmp::ICMP_STATE::ICMP_STATE_NULL || 
          (tun_cnw->m_xw_icmp->icmp_state == xw_icmp::ICMP_STATE::ICMP_STATE_IPERF && tun_cnw->m_xw_icmp->enabled_data_send)) 
        {
          rev_pdu  = srsran::make_byte_buffer();
          rev_pdu->N_bytes = nread;
          memcpy(rev_pdu->msg, buffer, nread);
          //send to sdap 
          tun_cnw->m_xw_icmp->dl_data_size += nread;
          // send_tun_msg(std::move(rev_pdu));
          handle_sgi_pdu(std::move(rev_pdu));
        }
      }
    }
    // usleep(100);//delayTime
	}

	return;
}

void xw_tun::handle_sgi_pdu(srsran::unique_byte_buffer_t msg)
{

  bool usr_found = false;
  bool ctr_found = false;

  std::map<uint32_t, srsran::gtpc_f_teid_ie>::iterator gtpu_fteid_it;
  std::map<in_addr_t, uint32_t>::iterator              gtpc_teid_it;
  srsran::gtpc_f_teid_ie                               enb_fteid;
  uint32_t                                             spgw_teid;
  struct iphdr*                                        iph = (struct iphdr*)msg->msg;
  printf("Received SGi PDU. Bytes %d\n", msg->N_bytes);

  if (iph->version != 4) {
    printf("IPv6 not supported yet.\n");
    return;
  }
  if (ntohs(iph->tot_len) < 20) {
    printf("Invalid IP header length. IP length %d.\n", ntohs(iph->tot_len));
    return;
  }

  // Logging PDU info
  printf("SGi PDU -- IP version %d, Total length %d\n", int(iph->version), ntohs(iph->tot_len));
  fmt::memory_buffer buffer;
  srsran::gtpu_ntoa(buffer, iph->saddr);
  printf("SGi PDU -- IP src addr %s\n", srsran::to_c_str(buffer));

  buffer.clear();
  srsran::gtpu_ntoa(buffer, iph->daddr);
  printf("SGi PDU -- IP dst addr %s\n", srsran::to_c_str(buffer));

  // Find user and control tunnel
  // gtpu_fteid_it = m_ip_to_usr_teid.find(iph->daddr);
  // if (gtpu_fteid_it != m_ip_to_usr_teid.end()) {
  //   usr_found = true;
  //   enb_fteid = gtpu_fteid_it->second;
  // }
  // gtpc_teid_it = m_ip_to_ctr_teid.find(iph->daddr);
  // if (gtpc_teid_it != m_ip_to_ctr_teid.end()) {
  //   ctr_found = true;
  //   spgw_teid = gtpc_teid_it->second;
  // }

  send_s1u_pdu(enb_fteid, msg.get());

  // Handle SGi packet
  // if (usr_found == false && ctr_found == false) {
  //   printf("Packet for unknown UE.\n");
  // } else if (usr_found == false && ctr_found == true) {
  //   // m_logger.debug("Packet for attached UE that is not ECM connected.");
  //   // printf("Packet for unknown UE.\n");
  //   // m_logger.debug("Triggering Donwlink Notification Requset.");
  //   // m_gtpc->send_downlink_data_notification(spgw_teid);
  //   // m_gtpc->queue_downlink_packet(spgw_teid, std::move(msg));
  //   return;
  // } else if (usr_found == true && ctr_found == false) {
  //   // m_logger.error("User plane tunnel found without a control plane tunnel present.");
  // } else {
  //   send_s1u_pdu(enb_fteid, msg.get());
  // }
}

void xw_tun::send_s1u_pdu(srsran::gtp_fteid_t enb_fteid, srsran::byte_buffer_t* msg)
{

  tun_adp->udp_.send_msg_to_enb_gtpu(msg);
  return;

  // Set eNB destination address
  struct sockaddr_in enb_addr;
  enb_addr.sin_family      = AF_INET;
  enb_addr.sin_port        = htons(GTPU_RX_PORT);
  enb_addr.sin_addr.s_addr = enb_fteid.ipv4;

  // Setup GTP-U header
  srsran::gtpu_header_t header;
  header.flags        = GTPU_FLAGS_VERSION_V1 | GTPU_FLAGS_GTP_PROTOCOL;
  header.message_type = GTPU_MSG_DATA_PDU;
  header.length       = msg->N_bytes;
  header.teid         = enb_fteid.teid;

  // m_logger.debug("User plane tunnel found SGi PDU. Forwarding packet to S1-U.");
  // m_logger.debug("eNB F-TEID -- eNB IP %s, eNB TEID 0x%x.", inet_ntoa(enb_addr.sin_addr), enb_fteid.teid);

  // Write header into packet
  int n;
  if (!srsran::gtpu_write_header(&header, msg, m_logger)) {
    m_logger.error("Error writing GTP-U header on PDU");
    goto out;
  }

  // Send packet to destination
  tun_adp->udp_.send_msg_to_enb_gtpu(msg);

out:
  m_logger.debug("Deallocating packet after sending S1-U message");
  return;
}


bool xw_tun::send_tun_msg(srsran::unique_byte_buffer_t tun_pdu)
{
  srsran::unique_byte_buffer_t enb_pdu = srsran::make_byte_buffer();
  enb_msg_header_t s1_header;
  s1_header.enb_id = tun_adp->udp_.enb_id;
  s1_header.rnti = 70;
  s1_header.msg_type = ip_data;

  int len = sizeof(s1_header);

  memcpy(enb_pdu->msg, &s1_header, len);
  memcpy(enb_pdu->msg+len, tun_pdu->msg , tun_pdu->N_bytes);

  enb_pdu->N_bytes = tun_pdu->N_bytes + len;

  tun_adp->udp_.send_enb_msg(std::move(enb_pdu));
  return true;
}


int xw_tun::init_sgi_interface(const Args* args) {
  struct ifreq ifr;
  int sgi_sock;

  if (m_sgi_up) {
    return xw_SRSRAN_ERROR_ALREADY_STARTED;
  }

  if (access("/dev/net/tun", F_OK) != 0) {
    std::cout <<"TUN device is not supported on this system."<< std::endl;   
    return xw_SRSRAN_ERROR_CANT_START;
  }

  // Construct the TUN device
  m_sgi = open("/dev/net/tun", O_RDWR);
  std::cout <<"TUN file descriptor = "<< m_sgi << std::endl;
  if (m_sgi < 0) {
    std::cout <<"Failed to open TUN device: "<< strerror(errno) << std::endl;
    return xw_SRSRAN_ERROR_CANT_START;
  }

  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
  strncpy(ifr.ifr_name, args->sgi_if_name.c_str(), IFNAMSIZ - 1);
  ifr.ifr_name[IFNAMSIZ - 1] = '\0';

  if (ioctl(m_sgi, TUNSETIFF, &ifr) < 0) {
    std::cout <<"Failed to set TUN device name: "<< strerror(errno) << std::endl; 
    close(m_sgi);
    return xw_SRSRAN_ERROR_CANT_START;
  }

  // Bring up the interface
  sgi_sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sgi_sock < 0) {
    std::cout <<"Failed to create socket: "<< strerror(errno) << std::endl;
    close(m_sgi);
    return xw_SRSRAN_ERROR_CANT_START;
  }

  if (ioctl(sgi_sock, SIOCGIFFLAGS, &ifr) < 0) {
    std::cout <<"Failed to get interface flags: "<< strerror(errno) << std::endl;
    close(sgi_sock);
    close(m_sgi);
    return xw_SRSRAN_ERROR_CANT_START;
  }

  ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
  if (ioctl(sgi_sock, SIOCSIFFLAGS, &ifr) < 0) {
    std::cout <<"Failed to set interface flags: "<< strerror(errno) << std::endl;
    close(sgi_sock);
    close(m_sgi);
    return xw_SRSRAN_ERROR_CANT_START;
  }

  // Set IP of the interface
  struct sockaddr_in* addr = (struct sockaddr_in*)&ifr.ifr_addr;
  if (!set_sockaddr(addr, args->sgi_if_addr.c_str(), 0)) {
    std::cout <<"Invalid sgi_if_addr: "<< args->sgi_if_addr << std::endl;
    return xw_SRSRAN_ERROR_CANT_START;
  }

  if (ioctl(sgi_sock, SIOCSIFADDR, &ifr) < 0) {
    std::cout <<"Failed to set TUN interface IP. Address: "<< args->sgi_if_addr << ", Error: "<< strerror(errno) << std::endl;
    close(m_sgi);
    close(sgi_sock);
    return xw_SRSRAN_ERROR_CANT_START;
  }

  // Set netmask of the interface
  ifr.ifr_netmask.sa_family = AF_INET;
  if (inet_pton(AF_INET, "255.255.255.0", &((struct sockaddr_in*)&ifr.ifr_netmask)->sin_addr) != 1) {
    perror("inet_pton");
    return xw_SRSRAN_ERROR_CANT_START;
  }
  if (ioctl(sgi_sock, SIOCSIFNETMASK, &ifr) < 0) {
    std::cout <<"Failed to set TUN interface Netmask. Error: "<< strerror(errno) << std::endl;
    close(m_sgi);
    close(sgi_sock);
    return xw_SRSRAN_ERROR_CANT_START;
  }

  close(sgi_sock);
  m_sgi_up = true;
  std::cout <<"Initialized SGi interface"<< std::endl;
  return xw_SRSRAN_SUCCESS;
}

bool xw_tun::set_sockaddr(struct sockaddr_in* addr, const char* ip, int port) 
{
  addr->sin_family = AF_INET;
  addr->sin_port = htons(port);
  return inet_pton(AF_INET, ip, &addr->sin_addr) == 1;
}

int xw_tun::get_sgi()
{
  return m_sgi;
}

}
