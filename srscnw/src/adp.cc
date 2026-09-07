#include "srscnw/hdr/adp.h"
#include "srscnw/hdr/udp.h"
#include <iostream>
#include <fstream>
#include <unordered_map>

namespace srsepc
{
  extern adp adp_;
  // extern adp adp_;

  adp::adp() : thread("ADP"), running(false)
  { /* Do nothing */
  }

  bool adp::init(cnw_args_t& args, cnw_interface_cnwadp* adp_to_cnw)
  {
    std::cout << "------adp init succeed------" << std::endl;
    running = true;

    if (!udp_.init(args, adp_to_cnw))
    {
      std::cout << "-----udp init Faiure-----" << std::endl;
      return false;
    }
    start();
    return true;
  }

  void adp::stop()
  {
    if (running)
    {
      running = false;
      wait_thread_finish();
    }
  }

  void adp::run_thread()
  {
    while (running)
    {
      udp_.run_udp();
    }
  }

} // namespace srsenb
