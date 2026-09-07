
#ifndef  SRSENB_MAC_TEST_H
#define SRSENB_MAC_TEST_H

#include "phy_common.h"
#include "prach_worker.h"
#include "srsran/config.h"
#include "srsran/interfaces/enb_time_interface.h"
#include "srsran/phy/channel/channel.h"
#include "srsran/radio/radio.h"
#include "srsenb/hdr/stack/mac/mac.h"
#include <atomic>
namespace srsenb{
class mac_test:public  srsran::thread
{
public:
mac_test():thread("mac_test")
{
    running=false;
}
  bool init(  enb_time_interface*  enb_  ,stack_interface_phy_lte* stack_,uint32_t  prio,uint32_t network_mode);
//void rach_test(stack_interface_phy_lte*stack,uint32_t test_tti);
void iot_rach_test(stack_interface_phy_lte*stack,uint32_t test_tti);
void rach_test(stack_interface_phy_lte*stack,uint32_t test_tti);
void sched_test(stack_interface_phy_lte*stack,uint32_t test_tti);
void completeTest(stack_interface_phy_lte*stack,uint32_t test_tti);
void iot_completeTest(stack_interface_phy_lte*stack,uint32_t test_tti);
void stop();
private:
void run_thread()override;
uint32_t tti=0;
uint32_t test_network_mode;
stack_interface_phy_lte* mstack=nullptr;
enb_time_interface*  enb=nullptr;
std::atomic<bool> running;
};
}
#endif