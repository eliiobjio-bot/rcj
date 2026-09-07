#pragma once
#include "srsran/common/threads.h"
#include "srsran/srslog/srslog.h"
#include "interface.h"

class inspect: public srsran::periodic_thread
{
public:
    inspect(uint8_t f, srslog::sink &logsink);
    void start();
    ~ inspect();
protected:
    void run_period() override;
    srslog::basic_logger &inspect_log;
    uint8_t field = 0;
    
};