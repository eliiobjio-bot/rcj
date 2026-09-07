#ifndef CNW_ADP_H
#define CNW_ADP_H

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
#include "srsran/srslog/srslog.h"
#include "udp.h"

namespace srsepc {

class adp : public srsran::thread
{
 public:
  adp();

  int beam_id = -1;
  bool init(cnw_args_t& args); 
  bool init(cnw_args_t& args, cnw_interface_cnwadp* adp_to_cnw); 
  void stop();
  udp udp_;

  // cnw* m_cnw = NULL;

 private:
  void run_thread() override;  

  std::atomic<bool> running;
};

};      // namespace srsenb
#endif  // ADP_H