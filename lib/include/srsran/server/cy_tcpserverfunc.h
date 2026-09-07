#ifndef TCP_SERVER_FUNC_H
#define TCP_SERVER_FUNC_H
#include <memory>
#include "cy_multilink.h"

typedef enum
{
	CY_TCPSERVERFUNC_EC_NULL = CY_GLOBAL_EC_TCPSERVERFUNC,
	CY_TCPSERVERFUNC_EC_INITTCPSERVERERROR,
}CY_TCPSERVERFUNC_ERROR_CODE;

extern int start_tcp_server(std::shared_ptr<CMultiLink> multilink, uint16_t port);
extern int stop_tcp_server();
#endif
