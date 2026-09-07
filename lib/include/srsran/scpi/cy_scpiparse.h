#ifndef CY_SCPI_PARSE_H
#define CY_SCPI_PARSE_H
#include <string>
#include <cstring>
#include <setjmp.h>
#include "srsran/common/cy_errorcode.h"
#include "srsran/common/cy_log.h"
//using namespace std;
#ifndef PARA_STR_CHAR1
typedef enum
{
	ERR_MISSPARA = CY_GLOBAL_EC_COMMANDEXEC,
	ERR_FORMAT,
	ERR_OUTRANGE,
	ERR_NOTVALID,

}CY_COMMANDEXEC_ERROR_CODE;

const int NEED_RESET_MEAS_RESULT = 1;

#define PARA_STR_CHAR1		'\''
#define PARA_STR_CHAR2		'\"'
#define PARA_SEPRATOR_CHAR	','

#ifdef __WINDOWS__
#define	STRCMP  stricmp
#define	STRNCMP  strnicmp
#else
#define	STRCMP 		strcasecmp
#define	STRNCMP 	strncasecmp
#endif
#endif

template <typename T>
struct SCPI_NODE 
{
    typedef void(T::*CLASS_FUN)(std::string &);
	const char *sNodeName;   
	const char *lNodeName;  
	short NodeNum;     
	struct SCPI_NODE<T> *NextNode;
	CLASS_FUN EntrySub;
	CLASS_FUN QuerySub;
	int needResetResult=0; 
	SCPI_NODE(const char*sname, const char* lname, short nnum, 
		SCPI_NODE<T> *next, CLASS_FUN setFun, CLASS_FUN getFun, int reset=0):
		sNodeName(sname), lNodeName(lname), NodeNum(nnum), NextNode(next), 
		EntrySub(setFun), QuerySub(getFun), needResetResult(reset){};
};

struct SCPISuffixPara
{
	char Name[32];
	int nValue;
};
//void CYICScpiParse<T>::SetScpiNode(const SCPI_NODE<T> *rootNode)
template<typename T>
class CYICScpiParse
{

public:
	CYICScpiParse ( SCPI_NODE<T> *rootNode, int n)
	{
		nSuffixNum	= 0;
		g_nRootNodeNums = n; //sizeof(rootNode)/sizeof(rootNode[0]);
		memset(Para,0,sizeof(Para));
		ParaCur = 0;
		ParaNext = 0;
		nIsCallProcessFunctionFlag = -1;
		pRootTable = rootNode;
	}
	virtual ~CYICScpiParse()
	{
	
	};
	
	int SetMeasContext(void* pstMeas)
	{
		pMeas = pstMeas;
		return 0;
	}
	
	int ResetParseContext()
	{
		nIsCallProcessFunctionFlag = -1;
		return 0;
	}
	
	int GetParseContextFlag()
	{
		return nIsCallProcessFunctionFlag;
	}

	virtual int CallProcessFuncion(void(T::*ProcessFunction)(std::string &), std::string &para)
	{
		ParaCur=ParaNext=0;
	
		int ret=setjmp (ErrRet);
		nIsCallProcessFunctionFlag = ret;
		if (ret==0)
		{
			T *pThat = dynamic_cast<T *>(this);
			(pThat->*ProcessFunction)(para);
			return 0;
		}
		else
		{
			// ...
			switch(ret)
			{
			case ERR_MISSPARA: VLE_LOGE(CONTROL_LEVEL_ERROR,"Error! - para or suffix missing.\n"); break;
			case ERR_FORMAT: VLE_LOGE(CONTROL_LEVEL_ERROR,"Error! - para format error: %s\n", Para+ParaCur); break;
			case ERR_OUTRANGE: 
				if (ParaCur!=ParaNext)
					VLE_LOGE(CONTROL_LEVEL_ERROR,"Error! - para out of range: %s\n", Para+ParaCur); 
				else
					VLE_LOGE(CONTROL_LEVEL_ERROR,"Error! - suffix maybe out of range\n");
				break;
			default: VLE_LOGE(CONTROL_LEVEL_ERROR,"Error! - unknown error code: %d\n", ret); break;
			};
			return ret;
		};
	};

	const SCPI_NODE<T> *GetCommandRootTable(int *nRootTableNodeNum)
	{
		if (g_nRootNodeNums<=0)
		{
			int n=0;
			const SCPI_NODE<T> *p= pRootTable;
			while(!(p->NodeNum==0&&p->EntrySub==0&&p->QuerySub==0)){
				n++; p++;
			};
	
			g_nRootNodeNums=n;
		};
		if(nRootTableNodeNum) *nRootTableNodeNum=g_nRootNodeNums;
		return pRootTable;
	}

	char *GetParaToken()
	{
		char *p=Para+ParaNext;
	
		while(*p&&isspace(*p)) p++;
	
		if (*p==0)
		{
			ParaNext=p-Para;
	
			longjmp (ErrRet, ERR_MISSPARA);
		};
	
		char *pe=p;
		int isStr;
		if (*pe==PARA_STR_CHAR1 || *pe==PARA_STR_CHAR2) isStr=1; else isStr=0;
	
		if (isStr)
		{
			pe++;
			while(*pe&&(*pe!=PARA_STR_CHAR1 && *pe!=PARA_STR_CHAR2)) pe++;
			// missing string char
			if (*pe==0) longjmp (ErrRet, ERR_FORMAT);
			pe++; // skip string char
		};
	
		// to token seprator
		while(*pe&&*pe!=PARA_SEPRATOR_CHAR) pe++;
	
		// mark para terminate
		if (*pe)  // has seprator char
			ParaNext=pe-Para+1;
		else
			ParaNext=pe-Para;
	
		// skip token terminal space char
		pe--;
		while(isspace(*pe)) pe--;
		pe[1]=0;
		// current para
		ParaCur=p-Para;
	
		if (isStr) {
			p++; pe[0]=0;
		};
		return p;
	}
	
    char* GetAllPara()
    {
        char *p=Para+ParaNext;
    
        while(*p&&isspace(*p)) p++;
    
        if (*p==0)
        {
            ParaNext=p-Para;
    
            longjmp (ErrRet, ERR_MISSPARA);
        };
        return p;
    }

	int CheckParaToken()
	{
		char *p=Para+ParaNext;
	
		while(*p&&isspace(*p)) p++;
		if(*p == 0)return ERR_NOTVALID;
		return 0;
	}

	int GetIntPara ()
	{
		char *p=GetParaToken();
		char *pe;
		int n=strtol(p, &pe, 10);
		if (*pe) longjmp(ErrRet, ERR_FORMAT);
	
		return n;
	}

	float GetFloatPara ()
	{
		char *p=GetParaToken();
		char *pe;
		double df=strtod(p, &pe);
		if (*pe) longjmp(ErrRet, ERR_FORMAT);
	
		return (float)df;
	}

	float GetUnitFloatPara ()
	{
		char *p=GetParaToken();
		char *pe;
		double df=strtod(p, &pe);
	
		return (float)df;
	}

	char* GetEnumPara ()
	{
		char *p=GetParaToken();
		return p;
	}

	char* GetStringPara ()
	{
		char *p=GetParaToken();
		return p;
	}

	char* GetHexStringPara ()
	{
		char *p=GetParaToken();
		if (!p || (p[0]!='#'&&p[1]!='H')|| (p[0]!='#'&&p[1]!='h')) longjmp(ErrRet, ERR_FORMAT);
		return p+2;
	}
	

	char* GetIPortPara ()
	{
		char *p=GetParaToken();
		// ...
		return p;
	}

	int GetSuffixPara(const char *name)
	{
		for(int i=0; i<nSuffixNum; i++)
		{
			if(STRCMP(name, SuffixList[i].Name)==0)
			{
				if (SuffixList[i].nValue<0)
					return 1;
				else
					return SuffixList[i].nValue;
			}
		};
#if 0
		longjmp(ErrRet, ERR_MISSPARA);
#else
		return 1; 
#endif
	}

	int GetParaNum()
	{
		int nCount = 0;
		char *p=Para+ParaNext;
	
		while(*p&&isspace(*p)) p++;
		if(*p == 0)return ERR_NOTVALID;
	
		while(*p)
		{
			if(*p==PARA_SEPRATOR_CHAR)nCount++;
			p++;
		}
		return nCount+1;
	}

	int GetIntListPara (int nlistsize,int list[])
	{
		char *p;
		for(int i = 0;i < nlistsize;i ++)
		{
			p = GetParaToken();
			if(p[0] == '0')
			{
				if(p[1] && (p[1] == 'x' || p[1] == 'X'))
				{		
					list[i] = strtol(p,NULL,16);
					continue;
				}
			}
			list[i] = strtol(p,NULL,10);
		}
		return 0;
	}

	int GetIntListPara (int list[])
	{
		int nlistsize = GetParaNum();
		GetIntListPara(nlistsize,list);
		return nlistsize;
	}

	int GetDoubleListPara (int nlistsize,double list[])
	{
		for(int i = 0;i < nlistsize;i ++)
		{
			list[i] = strtod(GetParaToken(),NULL);
		}
		return 0;
	}
	

	int GetDoubleListPara (double list[])
	{
		int nlistsize = GetParaNum();
		GetDoubleListPara(nlistsize,list);
		return nlistsize;
	}

	int GetFreqHzListPara (int nlistsize,double list[])
	{
		double dfFreq;
		char *p,*pe;
		for(int i = 0;i < nlistsize;i ++)
		{
			p = GetParaToken();
			pe = NULL;
			dfFreq = strtod(p,&pe); 
			if(pe)
			{
				while(*pe&&isspace(*pe)) pe++;
				if(*pe == 0)
				{
					list[i] = dfFreq;
					return ERR_NOTVALID;
				}			
				if(!STRCMP(pe,"GHz"))
				{
					dfFreq *= 1000000000;
				}
				else if(!STRCMP(pe,"MHz"))
				{
					dfFreq *= 1000000;
				}
				else if(!STRCMP(pe,"KHz"))
				{
					dfFreq *= 1000;
				}
			}
			list[i] = dfFreq;
		}
		return 0;
	}
	

	int GetFreqHzListPara (double list[])
	{
		int nlistsize = GetParaNum();
		GetFreqHzListPara(nlistsize,list);
		return nlistsize;
	}
	

	double GetFreqHzPara ()
	{
		double dfFreq;
		GetFreqHzListPara(1,&dfFreq);
		return dfFreq;
	}
	

	int GetTimeSecListPara (int nlistsize,double list[])
	{
		double dfTime;
		char *p,*pe;
		for(int i = 0;i < nlistsize;i ++)
		{
			p = GetParaToken();
			pe = NULL;
			dfTime = strtod(p,&pe); 
			if(pe)
			{
				while(*pe&&isspace(*pe)) pe++;
				if(*pe == 0)
				{
					list[i] = dfTime;
					return ERR_NOTVALID;
				}
	
				if(!STRCMP(pe,"ms"))
				{
					dfTime *= 0.001;
				}
				else if(!STRCMP(pe,"us"))
				{
					dfTime *= 0.000001;
				}
				else if(!STRCMP(pe,"ps"))
				{
					dfTime *= 0.000000001;
				}
			}
			list[i] = dfTime;
		}
		return 0;
	}

	double GetTimeSec (char *para, double &dfTime)
	{
		char *paraE=NULL;
		dfTime = strtod(para,&paraE);
		if(paraE)
		{
			while(*paraE&&isspace(*paraE)) paraE++;
			if(*paraE == 0)
			{
				return ERR_NOTVALID;
			}
	
			if(!STRCMP(paraE,"ms"))
			{
				dfTime *= 0.001;
			}
			else if(!STRCMP(paraE,"us"))
			{
				dfTime *= 0.000001;
			}
			else if(!STRCMP(paraE,"ps"))
			{
				dfTime *= 0.000000001;
			}
		}
		return 0;
	}

	int GetTimeSecListPara (double list[])
	{
		int nlistsize = GetParaNum();
		GetTimeSecListPara(nlistsize,list);
		return nlistsize;
	}

	double GetTimeSecPara ()
	{
		double dfTime;
		GetTimeSecListPara(1,&dfTime);
		return dfTime;
	};

	int GetEnumListPara (int nlistsize,int list[],const char *EnumList[],int enumsize)
	{
		int nIndex;
		char *p;
		for(int i = 0;i < nlistsize;i ++)
		{
			p = GetEnumPara();
			nIndex = CheckEnumPara(p,EnumList,enumsize);
			if(nIndex < 0)
			{
				return ERR_NOTVALID;
			}
			list[i] = nIndex;
		}
		return 0;
	};

	int GetEnumListPara (int list[],const char *EnumList[],int enumsize)
	{
		int nlistsize = GetParaNum();
		GetEnumListPara(nlistsize,list,EnumList,enumsize);
		return nlistsize;
	};

	int GetEnumPara (const char *EnumList[],int enumsize)
	{
		int Index = -1;
		GetEnumListPara(1,&Index,EnumList,enumsize);
		return Index;
	};
	

	int CheckIntPara (int para, int min, int max)
	{
		if(para<min || para>max)
		{
			return ERR_OUTRANGE;
		};
		return 0;
	};

	int CheckFloatPara (float para, float min, float max)
	{
		if(para<min || para>max)
		{
			return ERR_OUTRANGE;
		};
		return 0;
	};

	int CheckIntListParaRange (int list[],int nsize,int min, int max)
	{
		for(int i = 0;i < nsize;i ++)
		{
			if(list[i] < min || list[i] > max)
			{
				return ERR_OUTRANGE;
			}
		}
		return 0;
	};

	int CheckDoubleListParaRange (double list[],int nsize,double min, double max)
	{
		for(int i = 0;i < nsize;i ++)
		{
			if(list[i] < min || list[i] > max)
			{
				return ERR_OUTRANGE;
			}
		}
		return 0;
	};

	int CheckEnumPara (char *para, const char *EnumList[], int listSize)
	{
		int nLength = strlen(para);
		for(int i=0; i<listSize; i++)
		{
			if (STRNCMP(para, EnumList[i],nLength)==0 && 
				(EnumList[i][nLength] == 0 || islower(EnumList[i][nLength]))
				)
				return i;
		};
		return ERR_OUTRANGE;
	};

	int CheckIntListPara (int para, int IntList[], int listSize)
	{
		for(int i=0; i<listSize; i++)
		{
			if (para==IntList[i])
				return i;
		};
		return ERR_OUTRANGE;
	};

	int CheckFloatListPara (float para, float FloatList[], int listSize)
	{
		for(int i=0; i<listSize; i++)
		{
			if (para==FloatList[i])
				return i;
		};
		return ERR_OUTRANGE;
	};
	
	bool IMT(int ch)
	{
		return (ch==0||ch==';'||ch==0x0a||ch==0x0d);
	};

	bool GetNextName (char *Word, char *buff, int *Pos)
	{
		int num = 0;
		while(isspace(buff[(*Pos)]))(*Pos)++;
		if (buff[*Pos] == ':')	
		{
			(*Pos)++;
		}
	
		while((buff[*Pos]!=':')&&(buff[*Pos]!=' ')&&!IMT(buff[*Pos]))
		{
			Word[num] = buff[*Pos];
			(*Pos)++;
			num++;
		}
	
		Word[num] = '\0';
	
		while(isspace(buff[*Pos])) (*Pos)++;
		if (buff[*Pos]!=':')
		{
			return true;
		}
		else
			return false;
	
	};

	void UpperCase (char *buff)
	{
		int Len = strlen (buff);
		for (int i=0;i<Len;i++)
		{
			if ((buff[i] >= 0x61)&&(buff[i] <= 0x7A))
			{
				buff[i] = buff[i] - 0x20;
			}
		}
	};

	bool GetPara (char *Word, char *buff, int *Pos)
	{
		int num = 0;
	
		while (!IMT(buff[*Pos]))
		{
			Word[num] = buff[*Pos];
			num++;
			(*Pos)++;
		}
	
		Word[num] = '\0';
	
		return true;
	};
	virtual int ResetMeasResult()=0;

	virtual int RemoteCommandProcess(char *CommandBuf, std::string &output)
	{
		bool hasQuery=false;
		int  nRet = -1;
		bool IsEnd = false;
		bool IsCommOver = false;
		bool IsQuery = false;
		char CommandWord[50] = {0}; 
		int Pos = 0;
		//	int NodeMax = sizeof(RootNode)/sizeof(GLTE_NODE_ST);
		//	pRemoteNode = RootNode;
		int NodeMax=0;
		const SCPI_NODE<T> *pRemoteNode=GetCommandRootTable(&NodeMax);
		int nIsCommonCommand = 0;
		//UNUSED(nIsCommonCommand);
		nSuffixNum=0;
	
		/*if(m_pstMeas)
		{
			m_pstMeas->SetResultContext(pszData,pnDataLength);
		}*/
	
	
		while (!IsEnd)
		{
			IsCommOver = GetNextName (CommandWord, CommandBuf, &Pos);
	
			//		UpperCase (CommandWord);
	
			if (CommandWord[strlen(CommandWord)-1] == '?')
			{
				IsQuery = true;
				CommandWord[strlen(CommandWord)-1] = '\0';
			}
	
			int i = 0;
			
			int hasSuffix=0;
			int nSuffix=-1; 
			const SCPI_NODE<T> *pNode=pRemoteNode;
			while(i!=NodeMax)
			{
				hasSuffix=0;
				if(strchr(pNode->lNodeName, '<'))
					hasSuffix=1;
	
				if (hasSuffix)
				{
					char *p=CommandWord+strlen(CommandWord)-1;
					while(p>CommandWord && isdigit(*p)) p--;
					p++;
					if(*p) 
					{
						nSuffix=atoi(p);
						*p=0;
					};
				};
				if (STRCMP(CommandWord, pNode->sNodeName)==0)
					break;
				if (hasSuffix)
				{
					const char *p1=CommandWord;
					const char *p2=pNode->lNodeName;
					while(*p1&&*p2&&tolower(*p1)==tolower(*p2)) 
					{
						p1++; p2++;
					};
					// matched
					if(*p1==0&&*p2=='<')
						break;
				}
				else
				{
					if (STRCMP(CommandWord, pNode->lNodeName)==0)
						break;
				};
	
				i++;
				pNode++;
			};
			if (i!=NodeMax && hasSuffix)
			{
				if (nSuffixNum<(int)(sizeof(SuffixList)/sizeof(SuffixList[0])))
				{
					SuffixList[nSuffixNum].nValue=nSuffix;
					// copy suffix parameter name
					char *p1=SuffixList[nSuffixNum].Name;
					char *p2=strchr((char*) pNode->lNodeName, '<')+1;
					while(*p2&&*p2!='>')
					{
						*p1++=*p2++;
					};
					*p1=0;
	
					nSuffixNum++;
				}
				else
				{ 
				};
			};
	
			if (i != NodeMax)
			{
				if (((pRemoteNode+i)->NodeNum == 0)||IsCommOver)
				{
					GetPara (Para, CommandBuf, &Pos);  
	
					if (IsQuery) 
					{
						if ((pRemoteNode+i)->QuerySub != NULL)
						{
							nRet = CallProcessFuncion((pRemoteNode+i)->QuerySub, output);
						}
					}
					else
					{

						const SCPI_NODE<T> *pNode = (pRemoteNode+i);
						if(pNode->EntrySub != NULL)
						{
							if(pNode->needResetResult){
								this->ResetMeasResult();
							
							}
							nRet = CallProcessFuncion((pRemoteNode+i)->EntrySub, output);	
						}
					};
					IsEnd = true; 
				}
				else
				{
					NodeMax = (pRemoteNode+i)->NodeNum ;
					pRemoteNode = (pRemoteNode+i)->NextNode;
				}
			}
			else
			{
				IsEnd = true;
	
			}
			while(isspace(CommandBuf[Pos]))Pos++;
			if (CommandBuf[Pos] == ';')
			{
				Pos++;		
				while(isspace(CommandBuf[Pos]))Pos++;
				if ((CommandBuf[Pos] == ':')||(CommandBuf[Pos] == '*'))
				{
					pRemoteNode=GetCommandRootTable(&NodeMax);
					nSuffixNum=0;
	
					/*multi command result segment*/
					if (hasQuery)
					{
						//sprintf(pszData+*pnDataLength,";");
						//*pnDataLength += 1;
						output += ";";
					}
				}
				else
				{// if last node has suffix, remove it
					if (hasSuffix) nSuffixNum--;
				}
				if(!hasQuery) hasQuery=IsQuery;
	
				IsEnd = false; IsCommOver = false;IsQuery = false;
			};
	
			if(hasQuery)
			{
				//sprintf(pszData+*pnDataLength,"\n");
				//*pnDataLength += 1;
				output += ";";
			}
		}
	
		//	LeaveCriticalSection (&mRemoteCtrlServer.mCommandSection);
		return nRet;
	};

	#define USING_PARSE_FUNC() using CYICScpiParse::GetParaToken; \
	using CYICScpiParse::CheckParaToken; \
	using CYICScpiParse::GetIntPara; \
	using CYICScpiParse::GetFloatPara; \
	using CYICScpiParse::GetUnitFloatPara; \
	using CYICScpiParse::GetEnumPara; \
	using CYICScpiParse::GetAllPara; \
	using CYICScpiParse::GetStringPara;   \
	using CYICScpiParse::GetHexStringPara; \
	using CYICScpiParse::GetIPortPara; \
	using CYICScpiParse::GetSuffixPara; \
	using CYICScpiParse::GetParaNum	; \
	using CYICScpiParse::GetIntListPara; \
	using CYICScpiParse::ResetParseContext; \
	using CYICScpiParse::GetParseContextFlag; \
	using CYICScpiParse::GetCommandRootTable;  \
	using CYICScpiParse::CheckIntListPara; \
	using CYICScpiParse::GetTimeSecPara; \
	using CYICScpiParse::CheckFloatPara	; \
	using CYICScpiParse::CheckEnumPara ; \
	using CYICScpiParse::CheckIntPara;   \
	using CYICScpiParse::SetMeasContext; \
	using CYICScpiParse::CYICScpiParse

protected:
    int g_nRootNodeNums;
	jmp_buf ErrRet;
	
	char Para[1024];//  50000
	int ParaCur;
	int ParaNext;

	SCPISuffixPara SuffixList[10];
	int nSuffixNum;
	void *pMeas = nullptr;
	SCPI_NODE<T> *pRootTable = nullptr;

	int nIsCallProcessFunctionFlag;
};

#endif


