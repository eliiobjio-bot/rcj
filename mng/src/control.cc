#include <future>
#include <memory>
#include <iostream>
#include <iomanip>
#include <chrono>
#include "srsran/common/thread_pool.h"
#include "../hdr/control.h"

extern srsran::task_thread_pool *background_workers;

bool local_is_master()
{
    return true;
}
control::control(srslog::sink &logsink, uint8_t f):control_log(srslog::fetch_basic_logger("control", logsink, false))
{

    inspecting = std::make_shared<inspect>(f, logsink);
    inspecting->start();
}

int control::process_data(const uint8_t *pdata, uint32_t len)
{
     
    int ret = 0;
    //std::cout << "user data process:" << std::hex << std::setfill('0') << std::setw(2)<< pdata[0] << pdata[1]<< pdata[2] << pdata[3] <<std::endl;
    return ret;
}