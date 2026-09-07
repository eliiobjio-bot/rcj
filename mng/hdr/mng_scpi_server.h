#ifndef CY_SCPI_SERVER_H
#define CY_SCPI_SERVER_H

#include "srsran/common/threads.h"
#include "srsran/srslog/srslog.h"
#include "mng_scpi_channel.h"
#include "srsran/server/cy_server.h"


class SCPI_SERVER final : public srsran::thread
{
public:
  SCPI_SERVER() = delete;
  SCPI_SERVER(srslog::sink& log_sink, uint16_t p=33990):thread("SCPI_SERVER"),server_log(srslog::fetch_basic_logger("SCPI_SERVER", log_sink, false)), tcp_port(p)
  {
    
  };
  bool init()
  {
    start();
    return true;
  };
  void stop()
  {
    stop_server();
  }

private:
  void run_thread() override
  {
    start_server(CYSCPI_CHANNEL::HandleScpiInput, tcp_port);
  };

  //srslog::sink&         log_sink;
  srslog::basic_logger& server_log;
  uint16_t tcp_port;

};

#endif