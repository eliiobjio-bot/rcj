
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>

#include "cy_log.h"
//#include "CY52_Target.h"
//#include "CY52_LibTarget.h"
//#include "CY52_VXI11.h"
#include "cy_tcpserverfunc.h"
#include "cy_vxi11.h"
#include "cy_server.h"

#if 1
int AfxInitContext()
{
	//device_init();
	
	// g_cy52Context.phtVXI11Reply = VLE_HASHTABLE_CREATE(MAX_SUPPORTED_VXI11LID_COUNT,NULL,AfxHaseFree);

	// g_cy52Context.multiLink.InitContext();

	// g_cy52Context.subInstruManager.InitContext();	
	// g_cy52Context.cDevice.InitContext();

	return 0;
}
 std::shared_ptr<G_SERVER_CONTEXT> AfxGetContext()
 {
	static std::shared_ptr<G_SERVER_CONTEXT> pcontext;
	if(nullptr == pcontext)
	{
		pcontext = std::make_shared<G_SERVER_CONTEXT>();
		//pcontext->pTcpServer = std::make_shared<CTcpServer>();
		//pcontext->pMultiLink = std::make_shared<CMultiLink>();
	}
	return pcontext;
 }
int AfxRelContext()
{
	// g_cy52Context.multiLink.RelContext();
	// g_cy52Context.netTcpServer.ReleaseLocalServer();
	// VLE_HASHTABLE_DESTROY(&g_cy52Context.phtVXI11Reply);
	// //g_cy52Context.heapTimer.RelContext();

	// g_cy52Context.subInstruManager.RelContext();
	// g_cy52Context.cDevice.RelContext();

	return 0;
}
#endif

void* start_server(SERVER_CALLBACK callback, uint16_t port)
{
	CLog cylog;
	cylog.InitContext("/var/log/cylog", "server");
	std::shared_ptr<G_SERVER_CONTEXT> serverContext = AfxGetContext();
	serverContext->pMultiLink = std::make_shared<CMultiLink>(callback);
	serverContext->pTcpServer = std::make_shared<CTcpServer>(serverContext->pMultiLink);
	//serverContext->pVXI11Server = std::make_shared<CYVXI11Server>(serverContext->pMultiLink);
	if(start_tcp_server(serverContext->pMultiLink, port) < 0)
	{
		VLE_LOGE(CONTROL_LEVEL_ERROR,"Init TcpServer Fail!");
	}
	else
	{
		VLE_LOGE(CONTROL_LEVEL_INFO,"Init TcpServer Success!");
	}

	// start_vxi_server();
	return NULL;	
}
void stop_server()
{
	stop_tcp_server();
	stop_vxi11_server();
	return;
}

