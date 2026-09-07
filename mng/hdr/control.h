
#include <iostream>
#include <map>
#include <memory>
#include "srsran/srslog/srslog.h"
#include "interface.h"
#include "inspect.h"

#pragma once
class control
{
protected:

    srslog::basic_logger &control_log;
    std::shared_ptr<inspect> inspecting;

public:
    control(srslog::sink &logsink, uint8_t field = 1);
    int process_data(const uint8_t *pdata, uint32_t len);
};