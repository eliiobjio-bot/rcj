#ifndef	_CEYEAR_52_LOG_HEADERFILE_
#define _CEYEAR_52_LOG_HEADERFILE_
#include <unistd.h>
#if defined (__WINDOWS__) || defined(_MSC_VER)
	#include <direct.h>
    #include "stdafx.h"
#else
#if defined (__linux) || defined(__MINGW64__) || defined(__MINGW32__)
	#include <pthread.h>
	#include <sys/stat.h>
	#include <sys/types.h>
#endif
#endif
#include "cy_errorcode.h"

#define  CY_GETERRORFLAG()	CLog::m_nIsError
#define  CY_RESETERROR()	CLog::ResetError()


#define GLOBALLOG_MAXLOGINFO_LENGTH					 512

#ifndef MAX_PATH
#define MAX_PATH                                        256
#endif

#define _Assert_Exit_                           		\
	exit(EXIT_FAILURE);                        	 		\

#define _Assert_(cOND, aCTION)								\
	do {                                                    \
		if (!(cOND)) {                                      \
		fprintf(stderr, "\nAssertion ("#cOND") failed!\n"   \
		"In %s() %s:%d\n",									\
		__FUNCTION__, __FILE__, __LINE__);					\
		aCTION;                                             \
		}													\
	} while(0)

#define AssertError(cOND, aCTION)  _Assert_(cOND, aCTION)
#define AssertFatal(cOND)          _Assert_(cOND, _Assert_Exit_)

#define VLE_LOGE(...)				CLog::LE_TRACE(__LINE__,__FILE__,__FUNCTION__,__VA_ARGS__)

#define  VLE_ISFILEEXSITS(filename) ((access(filename,0) == -1)?(printf("file not exsit"),0):1)

#ifdef WIN32 //DEFINE_LINUX
#define  VLE_MAKE_DIR(dirname) 								\
{															\
	if(access(dirname,0) != 0)								\
	{														\
		printf("Directory not exsit\nCreate directory...\n");	\
		if(mkdir(dirname) != 0) printf("Failed\n");			\
		else printf("Done\n");								\
	}														\
}
#else
#define  VLE_MAKE_DIR(dirname) 								\
{															\
	if(access(dirname,0) != 0)								\
	{														\
		printf("Directory not exsit\nCreate directory...\n");	\
		if(mkdir(dirname,0777)!=0) printf("Failed\n");		\
		else printf("Done\n");								\
	}														\
}
#endif //linux

#define  CY_PRINTF_PARAMS(param) {char szP[256] = {0};sprintf(szP,"%s,%d;",#param,param);VLE_LOGE(CONTROL_LEVEL_UNDEFINED,szP,param);}
#define  CY_GETERRORFLAG()	CLog::m_nIsError
#define  CY_RESETERROR()	CLog::ResetError()

typedef enum
{
	CY_LOG_EC_NULL = CY_GLOBAL_EC_LOG,
	CY_LOG_EC_CONTROLLEVELERROR,
	CY_LOG_EC_WRITELOGFILEERROR,
	CY_LOG_EC_TRACEMEMORYERROR,

}CY_LOG_ERROR_CODE;

typedef enum{
    CONTROL_LEVEL_UNDEFINED =0x0,
	CONTROL_LEVEL_DEBUG     =0x1,
    CONTROL_LEVEL_INFO      =0x2,
    CONTROL_LEVEL_ERROR     =0x3,
}LOG_CONTROL_LEVEL;

typedef struct
{
	char 	szLOGDir[MAX_PATH];
	char 	szLOGfile[MAX_PATH+256];
	char	szERRLOGfile[MAX_PATH+256];
}LOG_COMMON_CONFIG_STRUCT;

typedef struct
{
	LOG_COMMON_CONFIG_STRUCT mtCommonConfig;
#if defined(__WINDOWS__) || defined(_MSC_VER)
	HANDLE			mutex;
#else
	pthread_mutex_t mutex;
#endif
	unsigned int mtLevelControl;
	unsigned int nIsPrintf;

}LOG_CONTEXT;

class CLog
{
public:
    CLog(void);
    virtual ~CLog(void);

    int InitContext(const char* pszDIR, const char* channel);
	int RelContext();
	LOG_CONTEXT* GetContext();

 	int SetControlLevel(unsigned int ControlLevel);
	int SetIsPrintf(unsigned int nIsPrintf);
	int SetMode(int nMode);

	static int LE_TRACE(unsigned int lineNo, const char* fileName,const char* functionName, unsigned int nLevelType, const char* pszFormat, ...);
	static int ResetError();
private:
	static int WriteLOGFile(char* pszFile,const char* pszMode,char* pszInfo,int nszInfoLength);
	static int WriteLOGFile(char* pszFile, const char* pszMode, char* pszInfo, size_t nszInfoLength);

public:
   static int		m_nIsError;
   static int		m_nMode;
   static int init;
private:
	static LOG_CONTEXT m_cContext;

};

#endif
