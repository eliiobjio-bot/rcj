
#ifndef SRSLOG_UDP_SINK_H
#define SRSLOG_UDP_SINK_H
#include <memory>
#include "srsran/srslog/sink.h"
#include "srsran/net/cy_udpsocket.h"
namespace srslog {

const int MAX_UDP_BUFF = 0xffff;  // 64K

/// This sink implementation writes to either stdout or stderr streams.
class cy_udp_sink : public sink
{
public:
  
  cy_udp_sink(std::unique_ptr<log_formatter> f, const std::string &remote, uint16_t port) :
    sink(std::move(f))
  {
  	mp_socket.reset(new UDPSocket<MAX_UDP_BUFF>());
    std::string host = remote;
    reinit(host, port);
  }

  cy_udp_sink(const cy_udp_sink& other) = delete;
  cy_udp_sink& operator=(const cy_udp_sink& other) = delete;
  void reinit(const std::string &host, uint16_t port) {
  	m_host = host;
	  m_port = port;
    inited = true;
  }
  detail::error_string write(detail::memory_buffer buffer) override
  {
    //assert(handle && "Invalid stream handle");
    if(inited)
    {
    // (const std::string& message, const std::string& host, uint16_t port, FDR_ON_ERROR)
      int len = buffer.size();
	  int num = len/MAX_UDP_BUFF + 1;
	  for(int i=  0; i < num ; i++)
	  {
	    if(i == num -1)
	    {
          mp_socket->SendTo(buffer.begin() +i * MAX_UDP_BUFF, len - i* MAX_UDP_BUFF, m_host, m_port);
		}
		else
		{
          mp_socket->SendTo(buffer.begin() +i * MAX_UDP_BUFF, MAX_UDP_BUFF, m_host, m_port);
		}
	  }
    }
    return {};
  }

  detail::error_string flush() override
  {
    //std::fflush(handle);
    return {};
  }

private:
  std::string m_host;
  uint16_t m_port = 0;
  std::unique_ptr<UDPSocket<MAX_UDP_BUFF>> mp_socket;
  bool inited = false;
};

} // namespace srslog

#endif // SRSLOG_STREAM_SINK_H
