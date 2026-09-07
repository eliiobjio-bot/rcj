#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cy_log.h"
#include "define.h"
#include "cy_server.h"
#include "cy_multilink.h"
#include "cy_tcpserverfunc.h"

int start_tcp_server(std::shared_ptr<CMultiLink> multilink,  uint16_t port)
{	
	auto pServer = AfxGetContext()->pTcpServer;
	//pServer->SetRecvCommandCallBack(callback);
	if(pServer && pServer->InitLocalServerTCP(NULL, port) != 0)  // 33770
	{
		VLE_LOGE(CONTROL_LEVEL_ERROR,"CY_TCPSERVERFUNC_EC_INITTCPSERVERERROR[%d]",CY_TCPSERVERFUNC_EC_INITTCPSERVERERROR);
		return CY_TCPSERVERFUNC_EC_INITTCPSERVERERROR;
	}
	
	return 0;
}
int stop_tcp_server()
{	
	auto pServer = AfxGetContext()->pTcpServer;
	pServer->ReleaseLocalServer();
	return 0;
}