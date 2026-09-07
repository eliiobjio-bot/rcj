
#ifndef _GLOBAL_NET_TCP_SERVER_HEADERFILE_
#define _GLOBAL_NET_TCP_SERVER_HEADERFILE_

#include <netinet/in.h>
#include <sys/socket.h>
#include <memory>
#include "cy_multilink.h"

#include "srsran/common/cy_errorcode.h"
#include "define.h"

#pragma pack(push, 1)

#ifndef TRACE
#define TRACE printf
#endif

#ifndef GLOBAL_NET_TCPSERVER_ENVIRONMENT
#define GLOBAL_NET_TCPSERVER_ENVIRONMENT

typedef int (*MLINK_LPHANDLE_POSTSCPICOMMAND_FUNC)(void* pLink, MLINK_SCPISOURCE_TYPE scpiSourceType, void* pvClientContext, char* pszCommand, int nszCommand);

#define GLOBAL_NET_MAX_TCPCONNECT      5
#define GLOBAL_NET_MAX_IPBUFSIZE       32
#define GLOBAL_NET_MAX_UDPBROADCASTNUM 10
#define GLOBAL_NET_MAX_SENDBUFFERSIZE  10000
#define GLOBAL_NET_MAX_RECVBUFFERSIZE  10000

#define MAX_SUPPORTED_RECV_TIMEOUT   5
#define GLOBAL_NET_MAX_GETDATADELAY  5000
#define MAX_TCPTRANS_RECV_LENGTH     10000 // 400000
#define MAX_TCPTRANS_RECV_PER_PACKET 1000  // 20000
#define MAX_TCPTRANS_SEND_LENGTH     1500  // 50000

// #ifndef min
// #define min(x,y) ((x)>(y)?(y):(x))
// #endif

// #ifndef max
// #define max(x,y) ((x)>(y)?(x):(y))
// #endif

typedef enum {
    CY_TCPSERVER_EC_NULL = CY_GLOBAL_EC_TCPSERVER,
    CY_TCPSERVER_EC_SOCKETERROR,
    CY_TCPSERVER_EC_SETSOCKETOPTERROR,
    CY_TCPSERVER_EC_SETSOCKETREUSEERROR,
    CY_TCPSERVER_EC_BINDERROR,
    CY_TCPSERVER_EC_LISTENERROR,
    CY_TCPSERVER_EC_ACCEPTERROR,
    CY_TCPSERVER_EC_GETCLIENTERROR,
    CY_TCPSERVER_EC_INITTCPERROR,
    CY_TCPSERVER_EC_RECVERROR,
    CY_TCPSERVER_EC_ADDLISTNODEMEMORYERROR,
    CY_TCPSERVER_EC_WAITRECVTIMEOUTERROR,
    CY_TCPSERVER_EC_SENDCONTEXTERROR,
    CY_TCPSERVER_EC_SENDCLIENTERROR,
} CY_TCPSERVER_ERROR_CODE;

typedef struct
{
    int                nServerStatus;
    char               szIP[GLOBAL_NET_MAX_IPBUFSIZE];
    int                nServerport;
    int                stSOCKET;
    struct sockaddr_in siADDRserver;
    pthread_t          pThreadLocalServerID;
    int                hThreadLocalServer;

} TCP_LOCAL_SERVER_STRUCT;

typedef struct
{
    int             nClientStatus;
    int             stClient;
    pthread_mutex_t mutexClient;
    pthread_t       pThreadClientID;
    int             hThreadClient;

    int  netRecvHead;
    int  netRecvTail;
    char sztcprecvCache[MAX_TCPTRANS_RECV_LENGTH + MAX_TCPTRANS_RECV_PER_PACKET];
    char sztcpsendCache[MAX_TCPTRANS_SEND_LENGTH];
} TCP_CONNECT_CLIENT_STRUCT;

typedef struct
{
    int nRunStatus;
    int nTcpWSAStartup;

    TCP_LOCAL_SERVER_STRUCT   tlsTCPLocalServer;
    TCP_CONNECT_CLIENT_STRUCT tccTCPConnectClient[GLOBAL_NET_MAX_TCPCONNECT];

} GLOBALNET_TCPSERVER_CONTEXT;

#endif

#pragma pack(pop)

class CTcpServer {

public:
    CTcpServer(std::shared_ptr< CMultiLink >);
    CTcpServer() = delete;
    ~CTcpServer();

    int InitLocalServerTCP(const char* pszIP, int nPort);
    int ReleaseLocalServer();

    // static int SetRecvCommandCallBack(SERVER_CALLBACK lpHandlePostScpiCommandFunc);
    int SendCommand(void* pContext, const char* pszData, int nDataLength);

    static void* GlobalNet_LOCALSERVERPROCESS(void* lpParam);
    static void* GlobalNet_CLIENTRECVPROCESS(void* lpParam);

private:
    static TCP_CONNECT_CLIENT_STRUCT* GetIdleClientContext(GLOBALNET_TCPSERVER_CONTEXT* pTCPServerContext);

    static int WaitCacheIdle(TCP_CONNECT_CLIENT_STRUCT* pstClientContext);
    static int JudgeBufferIsError(int nHead, int nTail, int nLength);
    static int MoveCircleCache(TCP_CONNECT_CLIENT_STRUCT* pstClientContext);
    static int SearchCompeleteCommand(TCP_CONNECT_CLIENT_STRUCT* pstClientContext);

private:
    GLOBALNET_TCPSERVER_CONTEXT m_gtcCommandContext;
    // static SERVER_CALLBACK	m_lpHandlePostScpiCommandFunc;
    std::shared_ptr< CMultiLink > m_lpMultiLink = nullptr;
};

#endif
