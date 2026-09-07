#ifndef ADP_H
#define ADP_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <queue>

#include "lib/include/srsran/common/threads.h"
#include "srsenb/hdr/enb.h"
#include "srsran/srslog/srslog.h"
#include "udp.h"

namespace srsenb {

class adp : public srsran::thread {
 public:
  adp();

    int beam_id = -1;
    all_args_t *args;
    bool init(all_args_t *args_); // ��ʼ��
    void stop();
    bool read_mib_config(const std::string &filename);
    udp udp_;

 private:
  void run_thread() override;  // �̣߳�ִ��  ��
  // srslog::basic_logger& logger;
  std::atomic<bool> running;
};

};      // namespace srsenb
#endif  // ADP_H