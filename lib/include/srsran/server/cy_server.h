#ifndef CY_INIT_H
#define CY_INIT_H
#include <memory>
#include <map>
#include "srsran/common/cy_errorcode.h"
#include "srsran/common/cy_log.h"
#include "cy_tcpserver.h"
#include "cy_multilink.h"
#include "cy_vxi11.h"
//#include "cy_vxi11server.h"
#define  MAX_SUPPORTED_VXI11LID_COUNT			200


//#define	 AFX_SPRINTF(...)						AfxGetContext()->multiLink.SprintFormat(__VA_ARGS__)
//#define  AFX_SPRINTFDOUBLEARRAY(dfData,nNum)	AfxGetContext()->multiLink.SprintDoubleArrayFormat(dfData,nNum)
//#define  AFX_GETSPRINTFBUFPTR()					AfxGetContext()->multiLink.GetSprintfBufferPtr()

// typedef enum
// {
// 	CY_TARGET_EC_NULL = CY_GLOBAL_EC_TARGET,
// 	CY_TARGET_EC_FILELOCKOPENERROR,
// 	CY_TARGET_EC_CHECKLOCKFILEERROR,
// 	CY_TARGET_EC_REGSIGNALERROR,
// 	CY_TARGET_EC_CREATETHREADERROR,
// 	CY_TARGET_EC_NOTVALID,
	
// }CY_TARGET_ERROR_CODE;

// typedef enum
// {
// 	CY_OPC_TYPE_NULL,
// 	CY_OPC_TYPE_MEASINITFINISH				= 0x1,
// 	CY_OPC_TYPE_MEASABORTFINSH				= 0x2,

// }CY_OPC_TYPE;

typedef struct
{
	unsigned int			nVersion=0.1;
	int						fd;

	//std::shared_ptr<CHeapTimer>				heapTimer;
	
	//CErrorCode				ecErrorCode;
	//CLog					lLog;

	//hash_table_t*			phtVXI11Reply;
	std::map<long, std::shared_ptr<CY_VXI11_COMMAND_REPLY_CONTEXT>>   phtVXI11Reply;
	
	std::shared_ptr<CTcpServer> 	pTcpServer;	
	//std::shared_ptr<CYVXI11Server>		pVXI11Server;
	std::shared_ptr<CMultiLink>				pMultiLink;

	// CSCPICommon	            scpiCommon;
	// CSubInstruManager		subInstruManager;
	// CDevice					cDevice;

	// int 					(*pAfxDLConfig)(int nType,char* pszData,int nLength);
	// int 					(*pAfxPCIeSend)(char* pszData,int nDataLength);
	// int						(*pAfxCalDLCongif)(int nType,char* pszData,int nLength);
	
}G_SERVER_CONTEXT;

//extern SERVER_CONTEXT 		g_ServerContext;

extern double dfDeltaF_CASValue ;
extern double dfDeltaF_FNAValue ;


extern std::shared_ptr<G_SERVER_CONTEXT> 		AfxGetContext();
void* start_server(SERVER_CALLBACK callback, uint16_t port);
void stop_server();
// extern int 					AfxGetCurSubInstrumentID();
// extern CSCPIGprfGen* 		AfxGetGprfGenContext();
// extern CSCPIGprfMeas* 		AfxGetGprfMeasContext();
// extern CSCPIMmWaveMeas* 	AfxGetMmWaveMeasContext();
// extern CSCPICommon*     	AfxGetCommonContext();
// extern CSubInstruManager* 	AfxGetSubInstruManagerContext();
// extern CSubInstru* 			AfxGetSubInstrumentContext(int nSubInstruID);
// extern CDevice* 			AfxGetDeviceContext();
// extern int 					AfxRFHeadIsExsit();

// extern int 					AfxCreateTimer(long nsec,long nmsec,int 	(*cb_func)(void * data));

// extern int		 			AfxSetOPCType(int nFlag);
// extern int		 			AfxReSetOPCType(int nFlag);
// extern int		 			AfxGetOPCType();

#endif
