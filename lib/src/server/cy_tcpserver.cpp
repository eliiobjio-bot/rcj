
#include <sys/types.h>
#include <netinet/tcp.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include "cy_log.h"
#include "define.h"
// #include "CY52_Target.h"
#include "cy_tcpserver.h"

#include <time.h>
#include <stdarg.h>
#include <sys/time.h>
#include "cy_server.h"
#include <vector>
#include <sstream>
#include <string>

// SERVER_CALLBACK CTcpServer::m_lpHandlePostScpiCommandFunc = NULL;
//static std::string                       tcprecvs{};
static inline std::vector< std::string > split_words(const std::string& str, char delimiter)
{
    std::vector< std::string > tokens;
    std::string                token;
    std::istringstream         tokenStream(str);

    while (std::getline(tokenStream, token, delimiter))
    {
        if (token.size() > 0)
            tokens.push_back(token);
    }
    return tokens;
}
CTcpServer::CTcpServer(std::shared_ptr< CMultiLink > multiLink)
{
    memset(&m_gtcCommandContext, 0, sizeof(GLOBALNET_TCPSERVER_CONTEXT));
    m_gtcCommandContext.nTcpWSAStartup = -1;

    m_gtcCommandContext.tlsTCPLocalServer.nServerport = 5214;
    m_lpMultiLink = multiLink;
}

CTcpServer::~CTcpServer()
{
    ReleaseLocalServer();
}

int CTcpServer::ReleaseLocalServer()
{
    if (m_gtcCommandContext.nTcpWSAStartup == 0)
    {
        /*关闭客户端*/
        for (int i = 0; i < GLOBAL_NET_MAX_TCPCONNECT; i++)
        {
            if (m_gtcCommandContext.tccTCPConnectClient[i].nClientStatus == 1)
            {
                m_gtcCommandContext.tccTCPConnectClient[i].nClientStatus = 0;
                if (m_gtcCommandContext.tccTCPConnectClient[i].stClient)
                {
                    shutdown(m_gtcCommandContext.tccTCPConnectClient[i].stClient, SHUT_RDWR);
                    close(m_gtcCommandContext.tccTCPConnectClient[i].stClient);
                    m_gtcCommandContext.tccTCPConnectClient[i].stClient = 0;
                }

                pthread_kill(m_gtcCommandContext.tccTCPConnectClient[i].pThreadClientID, 1);
                pthread_join(m_gtcCommandContext.tccTCPConnectClient[i].pThreadClientID, NULL);

                pthread_mutex_destroy(&m_gtcCommandContext.tccTCPConnectClient[i].mutexClient);
            }
        }

        /*关闭服务器端*/
        m_gtcCommandContext.tlsTCPLocalServer.nServerStatus = 0;
        if (m_gtcCommandContext.tlsTCPLocalServer.stSOCKET)
        {
            shutdown(m_gtcCommandContext.tlsTCPLocalServer.stSOCKET, SHUT_RDWR);
            close(m_gtcCommandContext.tlsTCPLocalServer.stSOCKET);
            m_gtcCommandContext.tlsTCPLocalServer.stSOCKET = 0;
        }

        pthread_kill(m_gtcCommandContext.tlsTCPLocalServer.pThreadLocalServerID, 1);
        pthread_join(m_gtcCommandContext.tlsTCPLocalServer.pThreadLocalServerID, NULL);
    }
    return 0;
}

int CTcpServer::InitLocalServerTCP(const char* pszIP, int nPort)
{
    int                      option = 1;
    int                      nReuseaddr = 1;
    TCP_LOCAL_SERVER_STRUCT& server_config = m_gtcCommandContext.tlsTCPLocalServer;

    server_config.nServerport = nPort;

    ReleaseLocalServer();

    server_config.stSOCKET = socket(AF_INET, SOCK_STREAM, 0);
    if (server_config.stSOCKET < 0)
    {
        VLE_LOGE(CONTROL_LEVEL_ERROR, "CY_TCPSERVER_EC_SOCKETERROR[%d]:socket Error,%d!", CY_TCPSERVER_EC_SOCKETERROR, m_gtcCommandContext.tlsTCPLocalServer.stSOCKET);
        return CY_TCPSERVER_EC_SOCKETERROR;
    }

    if (setsockopt(server_config.stSOCKET, IPPROTO_TCP, TCP_NODELAY, (char*)&option, sizeof(option)) != 0)
    {
        close(server_config.stSOCKET);
        VLE_LOGE(CONTROL_LEVEL_ERROR, "CY_TCPSERVER_EC_SETSOCKETOPTERROR[%d]:setsockopt Error!", CY_TCPSERVER_EC_SETSOCKETOPTERROR);
        return CY_TCPSERVER_EC_SETSOCKETOPTERROR;
    }

    server_config.siADDRserver.sin_addr.s_addr =
        (pszIP != NULL) ? inet_addr(m_gtcCommandContext.tlsTCPLocalServer.szIP) : htonl(INADDR_ANY);

    server_config.siADDRserver.sin_family = AF_INET;
    server_config.siADDRserver.sin_port = htons(server_config.nServerport);

    if (setsockopt(server_config.stSOCKET, SOL_SOCKET, SO_REUSEADDR, (const char*)&nReuseaddr, sizeof(int)) < 0)
    {
        close(m_gtcCommandContext.tlsTCPLocalServer.stSOCKET);
        VLE_LOGE(CONTROL_LEVEL_ERROR, "CY_TCPSERVER_EC_SETSOCKETREUSEERROR[%d]:setsockopt reuse Error!", CY_TCPSERVER_EC_SETSOCKETREUSEERROR);
        return CY_TCPSERVER_EC_SETSOCKETREUSEERROR;
    }

    if (bind(server_config.stSOCKET, (struct sockaddr*)&server_config.siADDRserver, sizeof(server_config.siADDRserver)) < 0)
    {
        close(m_gtcCommandContext.tlsTCPLocalServer.stSOCKET);
        VLE_LOGE(CONTROL_LEVEL_ERROR, "CY_TCPSERVER_EC_BINDERROR[%d]:bind Error!", CY_TCPSERVER_EC_BINDERROR);
        return CY_TCPSERVER_EC_BINDERROR;
    }

    server_config.nServerStatus = 1;
    server_config.hThreadLocalServer =
        pthread_create(&server_config.pThreadLocalServerID, NULL, CTcpServer::GlobalNet_LOCALSERVERPROCESS, &m_gtcCommandContext);

    return 0;
}

void* CTcpServer::GlobalNet_LOCALSERVERPROCESS(void* lpParam)
{
    GLOBALNET_TCPSERVER_CONTEXT* pTCPContext = (GLOBALNET_TCPSERVER_CONTEXT*)lpParam;
    int                          stSOCKET;
    socklen_t                    iAddrSize = sizeof(sockaddr_in);
    struct sockaddr_in           siADDRDevice;
    TCP_CONNECT_CLIENT_STRUCT*   pstClientContext;

    if (listen(pTCPContext->tlsTCPLocalServer.stSOCKET, GLOBAL_NET_MAX_TCPCONNECT) != 0)
    {
        close(pTCPContext->tlsTCPLocalServer.stSOCKET);
        VLE_LOGE(CONTROL_LEVEL_ERROR, "CY_TCPSERVER_EC_LISTENERROR[%d]:listen Error!", CY_TCPSERVER_EC_LISTENERROR);
        return NULL;
    }
    VLE_LOGE(CONTROL_LEVEL_INFO, "server listen success,%d", pTCPContext->tlsTCPLocalServer.stSOCKET);

    while (pTCPContext->tlsTCPLocalServer.nServerStatus == 1)
    {
        stSOCKET = accept(pTCPContext->tlsTCPLocalServer.stSOCKET,
                          (struct sockaddr*)&siADDRDevice,
                          &iAddrSize);

        if (stSOCKET == -1)
        {
            VLE_LOGE(CONTROL_LEVEL_INFO, "CY_TCPSERVER_EC_ACCEPTERROR[%d]:accept Error!", CY_TCPSERVER_EC_ACCEPTERROR);
            continue;
        }
        VLE_LOGE(CONTROL_LEVEL_INFO, "server accept success:%d", stSOCKET);

        pstClientContext = GetIdleClientContext(pTCPContext);
        if (!pstClientContext)
        {
            VLE_LOGE(CONTROL_LEVEL_ERROR, "GetIdleClientContext Error,%p!", pstClientContext);
            continue;
        }
        pstClientContext->stClient = stSOCKET;
        pstClientContext->nClientStatus = 1;
        pthread_mutex_init(&pstClientContext->mutexClient, NULL);
        pstClientContext->hThreadClient =
            pthread_create(&pstClientContext->pThreadClientID, NULL, CTcpServer::GlobalNet_CLIENTRECVPROCESS, pstClientContext);
    }
    return NULL;
}

TCP_CONNECT_CLIENT_STRUCT* CTcpServer::GetIdleClientContext(GLOBALNET_TCPSERVER_CONTEXT* pTCPServerContext)
{
    for (int i = 0; i < GLOBAL_NET_MAX_TCPCONNECT; i++)
    {
        if (pTCPServerContext->tccTCPConnectClient[i].nClientStatus == 0)
        {
            return &pTCPServerContext->tccTCPConnectClient[i];
        }
    }
    return NULL;
}

void* CTcpServer::GlobalNet_CLIENTRECVPROCESS(void* lpParam)
{
    int                        ret;
    TCP_CONNECT_CLIENT_STRUCT* pstClientContext = (TCP_CONNECT_CLIENT_STRUCT*)lpParam;
    std::string                       tcprecvs{};
    while (pstClientContext->nClientStatus == 1)
    {
        // if(WaitCacheIdle(pstClientContext))
        // {
        // 	break;
        // }
        // if(-1 == JudgeBufferIsError(pstClientContext->netRecvHead,pstClientContext->netRecvTail,MAX_TCPTRANS_RECV_LENGTH/2))
        // {
        // 	break;  // 出现致命错误， 以后优化
        // }
        ret = recv(pstClientContext->stClient,
                   pstClientContext->sztcprecvCache + pstClientContext->netRecvTail,
                   MAX_TCPTRANS_RECV_PER_PACKET,
                   0);
        if (ret > 0)
        {
            tcprecvs += std::string(pstClientContext->sztcprecvCache, ret);
            auto pos = tcprecvs.rfind('\n');
            if (pos != tcprecvs.npos)
            {
                std::string tmps{ tcprecvs.substr(0, pos + 1) };
                tcprecvs.erase(0, pos + 1);
                std::vector< std::string > vec = split_words(tmps, '\n');
                for (auto& c : vec)
                {
					c += '\n';
                    printf("scpi cmd:%ld-->%s \n", c.size(),c.data());
                    AfxGetContext()->pMultiLink->PostScpiCommand(MLINK_SCPISOURCE_TYPE_TCP, (void*)pstClientContext, (char*)c.data(), c.size());
                }
            }
        }
        else
        {
            VLE_LOGE(CONTROL_LEVEL_INFO, "GlobalNet_CLIENTRECVPROCESS:client recv ret:%d", ret);
            printf("client close, recv ret:%d\n", ret);
            break;
        }

#ifdef OLD_TCP_RECV_PROCESS
        if (ret > 0)
        {
            pstClientContext->netRecvTail = (pstClientContext->netRecvTail + ret);
            if (pstClientContext->netRecvTail > MAX_TCPTRANS_RECV_LENGTH)
            {
                MoveCircleCache(pstClientContext);
            }
            SearchCompeleteCommand(pstClientContext);
            usleep(1);
        }
        else
        {
            VLE_LOGE(CONTROL_LEVEL_INFO, "GlobalNet_CLIENTRECVPROCESS:client recv ret:%d", ret);
            break;
        }
#endif
    }
    VLE_LOGE(CONTROL_LEVEL_INFO, "client recv thread quit:%d", pstClientContext->stClient);

    printf("GlobalNet_CLIENTRECVPROCESS recv thread exit \n");

    pstClientContext->nClientStatus = 0;
    close(pstClientContext->stClient);
    pstClientContext->stClient = 0;
    pthread_mutex_destroy(&pstClientContext->mutexClient);
    return NULL;
}

int CTcpServer::WaitCacheIdle(TCP_CONNECT_CLIENT_STRUCT* pstClientContext)
{
    /* 等待缓冲区处理完毕 */
    int nTimeout = 0;
    while (nTimeout++ < MAX_SUPPORTED_RECV_TIMEOUT)
    {
        if (!JudgeBufferIsError(pstClientContext->netRecvHead, pstClientContext->netRecvTail, MAX_TCPTRANS_RECV_LENGTH / 2))
        {
            break;
        }
        usleep(1000000);
    }
    if (nTimeout == MAX_SUPPORTED_RECV_TIMEOUT)
    {
        VLE_LOGE(CONTROL_LEVEL_ERROR, "CY_TCPSERVER_EC_WAITRECVTIMEOUTERROR[%d]:wait cache fail,ready reconnect server!", CY_TCPSERVER_EC_WAITRECVTIMEOUTERROR);
        return CY_TCPSERVER_EC_WAITRECVTIMEOUTERROR;
    }
    return 0;
}

int CTcpServer::JudgeBufferIsError(int nHead, int nTail, int nLength)
{
    if (nTail >= nHead)
    {
        if (nTail > (nHead + MAX_TCPTRANS_RECV_PER_PACKET))
        {
            return -1;
        }
    }
    else
    {
        if ((nTail + nLength) > (nHead + MAX_TCPTRANS_RECV_PER_PACKET))
        {
            return -1;
        }
    }
    return 0;
}

int CTcpServer::MoveCircleCache(TCP_CONNECT_CLIENT_STRUCT* pstClientContext)
{
    memcpy(pstClientContext->sztcprecvCache,
           pstClientContext->sztcprecvCache + MAX_TCPTRANS_RECV_LENGTH,
           pstClientContext->netRecvTail - MAX_TCPTRANS_RECV_LENGTH);
    pstClientContext->netRecvTail = pstClientContext->netRecvTail - MAX_TCPTRANS_RECV_LENGTH;
    return 1;
}

int CTcpServer::SearchCompeleteCommand(TCP_CONNECT_CLIENT_STRUCT* pstClientContext)
{
    char* pszM = pstClientContext->sztcprecvCache;
    int   nHead = pstClientContext->netRecvHead;
    int   nTail = pstClientContext->netRecvTail;
    int   nSeg = MAX_TCPTRANS_RECV_LENGTH - 1;
    char  szT[MAX_TCPTRANS_RECV_PER_PACKET];
    int   nStart = nHead;
    if (nTail < nHead)
    {
        nTail += MAX_TCPTRANS_RECV_LENGTH;
    }

    for (int i = nHead; i < nTail; i++)
    {
        if ((pszM[i % MAX_TCPTRANS_RECV_LENGTH] == '\n'))
        {
            /*segment*/
            if ((i > nSeg) && (nStart < nSeg))
            {
                memcpy(szT, pszM + nStart, nSeg - nStart);
                memcpy(szT + nSeg - nStart, pszM, i - nSeg);
                AfxGetContext()->pMultiLink->PostScpiCommand(MLINK_SCPISOURCE_TYPE_TCP, (void*)pstClientContext, szT, i - nStart + 1);
            }
            else
            {
                AfxGetContext()->pMultiLink->PostScpiCommand(MLINK_SCPISOURCE_TYPE_TCP, (void*)pstClientContext, pszM + nStart, i - nStart + 1);
            }
            nStart = i + 1;
        }
    }
    pstClientContext->netRecvHead = nStart % MAX_TCPTRANS_RECV_LENGTH;
    return 0;
}

int CTcpServer::SendCommand(void* pContext, const char* pszData, int nDataLength)
{
    int ret;

    TCP_CONNECT_CLIENT_STRUCT* pstClientContext = (TCP_CONNECT_CLIENT_STRUCT*)pContext;

    if (!pContext)
    {
        VLE_LOGE(CONTROL_LEVEL_ERROR, "CY_TCPSERVER_EC_SENDCONTEXTERROR[%d]", CY_TCPSERVER_EC_SENDCONTEXTERROR);
        return CY_TCPSERVER_EC_SENDCONTEXTERROR;
    }

    ret = send(pstClientContext->stClient, pszData, nDataLength, 0);

    if (ret == SO_ERROR)
    {
        VLE_LOGE(CONTROL_LEVEL_ERROR, "CY_TCPSERVER_EC_SENDCLIENTERROR[%d]:send to fail,restart!", CY_TCPSERVER_EC_SENDCLIENTERROR);
        return CY_TCPSERVER_EC_SENDCLIENTERROR;
    }
    // printf("*************ret is %d\n",ret);
    return 0;
}
