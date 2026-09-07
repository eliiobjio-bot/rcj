#include <netinet/tcp.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include "cy_log.h"
#include "cy_multilink.h"
#include "cy_vxi11.h"
#include "cy_server.h"
//#include "CommandParser.h"

CMultiLink::CMultiLink (SERVER_CALLBACK callback)
{
	pthread_mutex_init(&m_mcContext.mutexCmdRoot,NULL);
	SetCommandCallBack(callback);
	InitContext();
}

CMultiLink::~CMultiLink()
{
	
}

int CMultiLink::InitContext()
{
	m_mcContext.pcmdRoot 	= NULL;
	m_mcContext.ncmdCount 	= 0;
	//m_mcContext.lpHandleCommandFunc = RemoteCommandProcess;

	return 0;
}

int CMultiLink::ResetContext()
{
	return 0;
}

int CMultiLink::RelContext()
{

	return 0;
}

MLINK_CONTEXT* CMultiLink::GetContext()
{
	return &m_mcContext;
}


int CMultiLink::SetCommandCallBack(SERVER_CALLBACK lpHandleCommandFunc)
{
	m_mcContext.lpHandleCommandFunc = lpHandleCommandFunc;
	return 0;
}

/*
注：所有的Tcp连接、VXI 连接都会阻塞在这个模块中，即串行执行所有对外指令；
    TCP的所有接收侧会被阻塞；
    VXI的所有接收侧指令也会被阻塞住；
*/
int CMultiLink::PostScpiCommand(MLINK_SCPISOURCE_TYPE 	scpiSourceType,void*	pvClientContext,char* pszCommand,int nszCommand)
{
	
	MLINK_CONTEXT* pMLinkContext = GetContext();
	
	pthread_mutex_lock(&pMLinkContext->mutexCmdRoot);
	//printf("[%s][%s][%d]--AddCommandContext:%s,AddCommandcount:%d!\n",__FILE__,__FUNCTION__,__LINE__,pszCommand,pMLinkContext->ncmdCount);
	MLINK_COMMAND_LIST_NODE* pT = pMLinkContext->pcmdRoot;
	if(pMLinkContext->pcmdRoot)
	{
		while(pT->pNext)
		{
			pT = pT->pNext;
		}
	}

	MLINK_COMMAND_LIST_NODE* pN = new MLINK_COMMAND_LIST_NODE();
	pN->pNext 				= NULL;
	pN->scpiSourceType		= scpiSourceType;
	pN->pvClientContext 	= pvClientContext;
	pN->nszCommandLength 	= nszCommand;
	pN->pszCommand = new char[nszCommand];
	if(pN->pszCommand == NULL)
	{
		pthread_mutex_unlock(&pMLinkContext->mutexCmdRoot);
		VLE_LOGE(CONTROL_LEVEL_ERROR,"CY_MLINK_EC_POSTMEMORYERROR[%d]:AddCommandListTail new memory error,length:%d",CY_MLINK_EC_POSTMEMORYERROR,nszCommand);
		return CY_MLINK_EC_POSTMEMORYERROR;
	}
	memcpy(pN->pszCommand,pszCommand,nszCommand);
	pN->pszCommand[nszCommand - 1] = 0;
	if(pMLinkContext->pcmdRoot)
	{
		pT->pNext = pN;
	}
	else
	{
		pMLinkContext->pcmdRoot = pN;
	}
	pMLinkContext->ncmdCount++;

	/*执行命令处理*/
	if(DoCommandMessageProcess(pMLinkContext) < 0)
	{
		VLE_LOGE(CONTROL_LEVEL_ERROR,"CY_MLINK_EC_DOCMDPROCESSERROR[%d]",CY_MLINK_EC_DOCMDPROCESSERROR);
		pthread_mutex_unlock(&pMLinkContext->mutexCmdRoot);
		return CY_MLINK_EC_DOCMDPROCESSERROR;
	}
	pthread_mutex_unlock(&pMLinkContext->mutexCmdRoot);

	return 0;
}

int CMultiLink::DelScpiCommandListHead(MLINK_CONTEXT* pMLinkContext)
{
	//pthread_mutex_lock(&pMLinkContext->mutexCmdRoot);
	if(pMLinkContext->pcmdRoot == NULL)
	{
		//pthread_mutex_unlock(&pMLinkContext->mutexCmdRoot);
		return 0;
	}
	MLINK_COMMAND_LIST_NODE* pT = pMLinkContext->pcmdRoot->pNext;
	delete pMLinkContext->pcmdRoot->pszCommand;
	delete pMLinkContext->pcmdRoot;
	pMLinkContext->pcmdRoot = pT;
	pMLinkContext->ncmdCount--;
	if(pMLinkContext->ncmdCount < 0)
	{
		//pthread_mutex_unlock(&pMLinkContext->mutexCmdRoot);
		VLE_LOGE(CONTROL_LEVEL_ERROR,"CY_MLINK_EC_DOCMDPROCESSERROR[%d]:DelScpiCommandListHead ncmdCount error!",CY_MLINK_EC_DOCMDPROCESSERROR);
		return CY_MLINK_EC_POSTCMDCOUNTERROR;
	}
	//pthread_mutex_unlock(&pMLinkContext->mutexCmdRoot);
	return 0;
}

int CMultiLink::DoCommandMessageProcess(MLINK_CONTEXT* pMLinkContext)
{
	//pthread_mutex_lock(&pMLinkContext->mutexCmdRoot);
	while(pMLinkContext->pcmdRoot)
	{
		/*解析并发送数据*/
		if(pMLinkContext->lpHandleCommandFunc)
		{
			std::string output{};
			VLE_LOGE(CONTROL_LEVEL_DEBUG,"DoCommandMessageProcess 4");

		    /*解析指令*/
			memset(&pMLinkContext->mcCommandContext,0,sizeof(pMLinkContext->mcCommandContext));
			pMLinkContext->mcCommandContext.nszResLength = 0;

			/* 修改指定子系统上下文时，需要加锁访问 */
		
			VLE_LOGE(CONTROL_LEVEL_DEBUG,"MultiLink,Ready Lock Instrument,%s",pMLinkContext->pcmdRoot->pszCommand);	
			/* OPC等公共指令不进行加锁保护 */
			if(strncmp(pMLinkContext->pcmdRoot->pszCommand,"*OPC?",strlen("*OPC?")))
			{
				if(0 == strncmp(pMLinkContext->pcmdRoot->pszCommand,"SYST:PRES",strlen("SYST:PRES")))
				{
					VLE_LOGE(CONTROL_LEVEL_INFO,"MultiLink,Ready Lock Instrument,%s",pMLinkContext->pcmdRoot->pszCommand);
				}
				//AfxGetSubInstrumentContext(pMLinkContext->nCurSubInstruId)->LockSubInstrument();
				pMLinkContext->lpHandleCommandFunc(pMLinkContext->pcmdRoot->pszCommand, output);
				//AfxGetSubInstrumentContext(pMLinkContext->nCurSubInstruId)->UnLockSubInstrument();
			}
			else
			{
	
			    VLE_LOGE(CONTROL_LEVEL_DEBUG,"OPC PATH");
				pMLinkContext->lpHandleCommandFunc(pMLinkContext->pcmdRoot->pszCommand, output);
			}
		
			VLE_LOGE(CONTROL_LEVEL_DEBUG,"MultiLink,Already UnLock Instrument");
			
		    /* 发送数据到底层缓冲区 */
			if(output.length() > 0)
			{
				VLE_LOGE(CONTROL_LEVEL_DEBUG,"DoCommandMessageProcess 5");
				switch(pMLinkContext->pcmdRoot->scpiSourceType)
				{
					case MLINK_SCPISOURCE_TYPE_TCP:
					{
			
						VLE_LOGE(CONTROL_LEVEL_DEBUG,"DoCommandMessageProcess 6");
					
						AfxGetContext()->pTcpServer->SendCommand(pMLinkContext->pcmdRoot->pvClientContext,
							output.data(), output.length());
					}break;
					case MLINK_SCPISOURCE_TYPE_VXI:
					{
						VLE_LOGE(CONTROL_LEVEL_DEBUG,"DoCommandMessageProcess 7 [%s]",pMLinkContext->mcCommandContext.szRes);

						/* 发送数据到VXI的发送缓冲区 */
						device_send_reply(pMLinkContext->pcmdRoot->pvClientContext,
							output.data(), output.length());
					}break;
					default:
					{
						VLE_LOGE(CONTROL_LEVEL_ERROR,"CY_MLINK_EC_DOCMDPROCESSSOURCETYPEERROR[%d]:scpiSourceType error:%d",CY_MLINK_EC_DOCMDPROCESSSOURCETYPEERROR,
							pMLinkContext->pcmdRoot->scpiSourceType);
					}break;
				}
			}
		}

		/* 释放命令节点内存 */
		DelScpiCommandListHead(pMLinkContext);
		continue;
	}
	//pthread_mutex_unlock(&pMLinkContext->mutexCmdRoot);		
	return 0;
}

int CMultiLink::SprintFormat(const char* pszFormat, ...)
{
	if(m_mcContext.mcCommandContext.nszResLength > MAX_MLINK_SUPPORTED_RESPONSE_LENGTH)
	{
		VLE_LOGE(CONTROL_LEVEL_ERROR,"CY_MLINK_EC_SPRINTFLENGTHERROR[%d]:pre,%d",CY_MLINK_EC_SPRINTFLENGTHERROR);
		return CY_MLINK_EC_SPRINTFLENGTHERROR;
	}
	va_list args;
	if(pszFormat!=NULL)
	{
		va_start(args,pszFormat);
		vsprintf(m_mcContext.mcCommandContext.szRes+m_mcContext.mcCommandContext.nszResLength,pszFormat,args);
		va_end(args);
		m_mcContext.mcCommandContext.nszResLength = strlen(m_mcContext.mcCommandContext.szRes);
	}
	if(m_mcContext.mcCommandContext.nszResLength > MAX_MLINK_SUPPORTED_RESPONSE_LENGTH)
	{
		VLE_LOGE(CONTROL_LEVEL_ERROR,"CY_MLINK_EC_SPRINTFLENGTHERROR[%d]:aft,%d",CY_MLINK_EC_SPRINTFLENGTHERROR);
		return CY_MLINK_EC_SPRINTFLENGTHERROR;
	}
	return 0;
}

int CMultiLink::SprintDoubleArrayFormat(double* pdfData,int nDataNum)
{
	if(m_mcContext.mcCommandContext.nszResLength > MAX_MLINK_SUPPORTED_RESPONSE_LENGTH)
	{
		VLE_LOGE(CONTROL_LEVEL_ERROR,"CY_MLINK_EC_SPRINTFLENGTHERROR[%d]:pre,%d",CY_MLINK_EC_SPRINTFLENGTHERROR);
		return CY_MLINK_EC_SPRINTFLENGTHERROR;
	}
	if(pdfData!=NULL)
	{
		for(int i = 0;i < nDataNum;i ++)
		{
			if(i != 0)
				sprintf(m_mcContext.mcCommandContext.szRes+m_mcContext.mcCommandContext.nszResLength,",%f",pdfData[i]);
			else 
				sprintf(m_mcContext.mcCommandContext.szRes+m_mcContext.mcCommandContext.nszResLength,"%f",pdfData[i]);
			m_mcContext.mcCommandContext.nszResLength += sizeof(double);
		}
		m_mcContext.mcCommandContext.nszResLength = strlen(m_mcContext.mcCommandContext.szRes);
	}
	if(m_mcContext.mcCommandContext.nszResLength > MAX_MLINK_SUPPORTED_RESPONSE_LENGTH)
	{
		VLE_LOGE(CONTROL_LEVEL_ERROR,"CY_MLINK_EC_SPRINTFLENGTHERROR[%d]:aft,%d",CY_MLINK_EC_SPRINTFLENGTHERROR);
		return CY_MLINK_EC_SPRINTFLENGTHERROR;
	}
	return 0;
}

char* CMultiLink::GetSprintfBufferPtr()
{
	return m_mcContext.mcCommandContext.szRes;
}

