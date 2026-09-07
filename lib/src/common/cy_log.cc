#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#ifdef _MSC_VER
#include <io.h>
#else
#include <sys/time.h>
#endif
#include "srsran/common/cy_log.h"

LOG_CONTEXT CLog::m_cContext;
int CLog::m_nIsError = 0;
int CLog::m_nMode = 0;
int CLog::init = 0;
const static char* g_ControlLevel[]=
{
    "","[DEBUG]","[INFO]","[ERROR]",
};
	
CLog::CLog(void)
{
   
}

CLog::~CLog(void)
{
	RelContext();
}

int CLog::SetControlLevel(unsigned int ControlLevel)
{
    if(ControlLevel > CONTROL_LEVEL_ERROR)
    {
        return CY_LOG_EC_CONTROLLEVELERROR;
    }
    m_cContext.mtLevelControl = ControlLevel;
    return 0;
}

int CLog::SetIsPrintf(unsigned int nIsPrintf)
{
	m_cContext.nIsPrintf = nIsPrintf;
	return 0;
}
int CLog::SetMode(int nMode)
{
	m_nMode = nMode;
	return 1;
}
int CLog::InitContext(const char* pszDIR, const char* channel)
{
	memset(m_cContext.mtCommonConfig.szLOGDir,0,sizeof(m_cContext.mtCommonConfig.szLOGDir));
	if(pszDIR)
	{
		snprintf(m_cContext.mtCommonConfig.szLOGDir,MAX_PATH, "%s",pszDIR);
	}
	else
	{
		snprintf(m_cContext.mtCommonConfig.szLOGDir,MAX_PATH,"LOG");
	}
	VLE_MAKE_DIR(m_cContext.mtCommonConfig.szLOGDir);

	memset(m_cContext.mtCommonConfig.szLOGfile,0,sizeof(m_cContext.mtCommonConfig.szLOGfile));
    snprintf(m_cContext.mtCommonConfig.szLOGfile,MAX_PATH+256,"%s/%s_Log.txt",m_cContext.mtCommonConfig.szLOGDir, channel);

	memset(m_cContext.mtCommonConfig.szERRLOGfile,0,sizeof(m_cContext.mtCommonConfig.szERRLOGfile));
    snprintf(m_cContext.mtCommonConfig.szERRLOGfile,MAX_PATH+256,"%s/%s_ErrLog.txt",m_cContext.mtCommonConfig.szLOGDir, channel);
    if(init == 0){
#ifdef _MSC_VER
		m_cContext.mutex	= CreateEvent(NULL,TRUE,TRUE,NULL);
#else
		pthread_mutex_init(&m_cContext.mutex,NULL);
#endif
		init =1;
	}

	m_cContext.mtLevelControl 	= CONTROL_LEVEL_DEBUG;
	m_cContext.nIsPrintf		= 0;

	return 0;
}

int CLog::RelContext()
{
    if(init > 0)
    {
#ifdef _MSC_VER
	    CloseHandle(m_cContext.mutex);
#else
	    pthread_mutex_destroy(&m_cContext.mutex);
#endif
        init --;
	}

	return 0;
}

LOG_CONTEXT* CLog::GetContext()
{
	return &m_cContext;
}
int CLog::WriteLOGFile(char* pszFile, const char* pszMode, char* pszInfo, size_t nszInfoLength)
{
	int len = static_cast<int>(nszInfoLength);
	return WriteLOGFile(pszFile, pszMode, pszInfo, len);
}
int CLog::WriteLOGFile(char* pszFile,const char* pszMode,char* pszInfo,int nszInfoLength)
{
	FILE* pfLogfile = fopen(pszFile,pszMode);
	if(pfLogfile)
	{
		if(nszInfoLength > 0 && pszInfo!=NULL)
		{
			fwrite(pszInfo,nszInfoLength,1,pfLogfile);
		}
		fclose(pfLogfile);
		return 0;
	}
	return CY_LOG_EC_WRITELOGFILEERROR;
}

int CLog::LE_TRACE(unsigned int lineNo,const char* fileName,const char* functionName, unsigned int nLevelType,const char* pszFormat, ...)
{
	//UNUSED(functionName);
	char g_szInfo[GLOBALLOG_MAXLOGINFO_LENGTH] = {0};
	if(nLevelType != CONTROL_LEVEL_UNDEFINED && nLevelType < m_cContext.mtLevelControl)return 0;

#ifdef _MSC_VER
	WaitForSingleObject(m_cContext.mutex,INFINITE);
	ResetEvent(m_cContext.mutex);
#else
	pthread_mutex_lock(&m_cContext.mutex);
#endif

	va_list args;

    time_t CurrentTime=time(NULL);
    struct tm *pCurrentTime = localtime(&CurrentTime);
    char sHeaderInfo[256]={0};
	char sCurrentTime[100]={0};
	
	//memset(g_szInfo,0,GLOBALLOG_MAXLOGINFO_LENGTH);
	if(CONTROL_LEVEL_UNDEFINED!=nLevelType)
	{
		strftime(sCurrentTime, sizeof(sCurrentTime), "%Y-%m-%d %H:%M:%S", pCurrentTime);  


		/*取的当前时间毫秒级*/	
#ifdef _MSC_VER
		SYSTEMTIME st = { 0 };
		GetLocalTime(&st);
		sprintf(sCurrentTime+strlen(sCurrentTime),".%03d",st.wMilliseconds);
#else
		struct timeval nowTus;
		gettimeofday( &nowTus, NULL ); 
		sprintf(sCurrentTime+strlen(sCurrentTime),".%.06ld",nowTus.tv_usec);
#endif
		if(nLevelType == CONTROL_LEVEL_DEBUG || nLevelType == CONTROL_LEVEL_ERROR)
		{
			/*发布版本不进行错误位置打印*/
			if(!m_nMode)
			{
				sprintf(sHeaderInfo,"[%s(%d)]",fileName,lineNo);
			}
			else
			{
				sprintf(sHeaderInfo,"[(%d)]",lineNo);
			}
		}
		sprintf(g_szInfo,"%s %s%s:",
			sCurrentTime,
			g_ControlLevel[nLevelType],
			sHeaderInfo);
	}

	if(pszFormat!=NULL)
	{
		va_start(args,pszFormat);
		vsprintf(g_szInfo+strlen(g_szInfo),pszFormat,args);
		va_end(args);
	}
	sprintf(g_szInfo+strlen(g_szInfo),"\r\n");

	if(strlen(g_szInfo) >= GLOBALLOG_MAXLOGINFO_LENGTH)
	{
		g_szInfo[GLOBALLOG_MAXLOGINFO_LENGTH-1] = '\0';
	}

	/*保存文件*/
	WriteLOGFile(m_cContext.mtCommonConfig.szLOGfile,"ab+",g_szInfo,strlen(g_szInfo));

	if(nLevelType == CONTROL_LEVEL_ERROR)
	{
		WriteLOGFile(m_cContext.mtCommonConfig.szERRLOGfile,"ab+",g_szInfo,strlen(g_szInfo));
		m_nIsError = 1;
		//AfxSendDataToUI(CY_COMMU_HEADER_TYPE_ERROR,NULL,0);
	}

	/*打印屏幕*/
	if(m_cContext.nIsPrintf)
	{
		printf("%s",g_szInfo);
	}

#ifdef _MSC_VER	
	SetEvent(m_cContext.mutex);
#else
	pthread_mutex_unlock(&m_cContext.mutex);
#endif
	return 0;
}
int CLog::ResetError()
{
	m_nIsError = 0;
	return 0;
}

