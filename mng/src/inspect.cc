#include <iostream>
#include "../hdr/inspect.h"


inspect::inspect(uint8_t f, srslog::sink &sink) : inspect_log(srslog::fetch_basic_logger("inspect", sink, false)), periodic_thread("inspect")
{
    field = f;
}
void inspect::start()
{
    start_periodic(2e6); // 2s
}

inspect::~inspect()
{
}
void inspect::run_period()
{
    
}