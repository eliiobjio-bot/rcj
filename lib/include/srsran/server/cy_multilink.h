#ifndef _CEYEAR_52_MULTILINK_HEADERFILE_
#define _CEYEAR_52_MULTILINK_HEADERFILE_

#include <pthread.h>
#include <memory>
#include "srsran/common/cy_errorcode.h"
#include "define.h"
#define 	MAX_MLINK_SUPPORTED_RESPONSE_LENGTH		50000

typedef enum
{
	CY_MLINK_EC_NULL = CY_GLOBAL_EC_MULTILINK,
	CY_MLINK_EC_CREATETHREADERROR,
	CY_MLINK_EC_POSTMEMORYERROR,
	CY_MLINK_EC_POSTPARAMERROR,
	CY_MLINK_EC_POSTCMDCOUNTERROR,
	CY_MLINK_EC_DOCMDPROCESSSOURCETYPEERROR,
	CY_MLINK_EC_DOCMDPROCESSERROR,
	CY_MLINK_EC_SPRINTFLENGTHERROR,
	CY_MLINK_EC_SUBINSTRUIDXERROR,
	
}CY_GLOBAL_MLINK_ERROR_CODE;

typedef enum
{
	MLINK_SCPISOURCE_TYPE_NULL,
	MLINK_SCPISOURCE_TYPE_TCP,
	MLINK_SCPISOURCE_TYPE_VXI,
}MLINK_SCPISOURCE_TYPE;


typedef struct
{
	int		nszResLength;
	char	szRes[MAX_MLINK_SUPPORTED_RESPONSE_LENGTH];	
}MLINK_COMMAND_CONTEXT;

typedef struct _MLINK_COMMAND_NODE_STRUCT_ 
{
	MLINK_SCPISOURCE_TYPE 			scpiSourceType;
	void* 							pvClientContext;
	int								nszCommandLength;
	char* 							pszCommand;
	_MLINK_COMMAND_NODE_STRUCT_*	pNext;
}MLINK_COMMAND_LIST_NODE;


/*命令回调函数声明*/
typedef void (*MLINK_LPHANDLE_COMMAND_FUNC) (char* pszCommand);

typedef struct  
{
	/*运行状态*/
	int 						nMLinkStatus;
//	int							nCurSubInstruId;
	
	/*指令链表节点*/
	MLINK_COMMAND_LIST_NODE*	pcmdRoot;
	int							ncmdCount;
	pthread_mutex_t				mutexCmdRoot;

	/*回调函数*/
	MLINK_COMMAND_CONTEXT		mcCommandContext;
	SERVER_CALLBACK	lpHandleCommandFunc;
	
}MLINK_CONTEXT;


class CMultiLink
{

public:
	CMultiLink (SERVER_CALLBACK ); 
	CMultiLink ()=delete; 
	~CMultiLink();

	int InitContext();
	int ResetContext();
	int RelContext();

	MLINK_CONTEXT* GetContext();
	int GetCurSubInsruId();
	
	
	int SprintFormat(const char* pszFormat, ...);
	int SprintDoubleArrayFormat(double* pdfData,int nDataNum);
	char* GetSprintfBufferPtr();
	
	/*static*/ int PostScpiCommand(MLINK_SCPISOURCE_TYPE 	scpiSourceType,void*	pvClientContext,char* pszCommand,int nszCommand);
protected:
	int SetCommandCallBack(SERVER_CALLBACK lpHandleCommandFunc);
private:
	/*static*/ int DelScpiCommandListHead(MLINK_CONTEXT* pMLinkContext);
	/*static*/ int DoCommandMessageProcess(MLINK_CONTEXT* pMLinkContext);
	
public:
	
private:
	MLINK_CONTEXT m_mcContext;
	//SERVER_CALLBACK m_scpiCallback;
	
};

#endif
