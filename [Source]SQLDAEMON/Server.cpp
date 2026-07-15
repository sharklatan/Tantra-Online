#define _WIN32_WINNT 0x500 // for use "TryEnterCriticalSection"

#include <windows.h>
#include <windowsx.h>
#include <process.h>
#include <stdlib.h>
#include <stdio.h> 
#include <fcntl.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <string.h>        
#include <time.h> 

#include "Basedef.h"
#include "CPSock.h"
#include "ItemEffect.h"
#include "CSql.H"
#include "Server.h"
#include "DrawInfo.h"

#define				DBTABLE				"TantraBackup"

uintptr_t			ThreadHandle[MAX_THREAD];	// keep thread handle, for release it. when, server shut down.	
CSQL				mSql[MAX_THREAD];
CRITICAL_SECTION	cspJob;					// is [pJob] taken, by thread?
unsigned char		sqlDSN[32]		=	"defaultSQL";
unsigned char		sqlID[32]		=	"defaultAccount";
unsigned char		sqlPASS[32]		=	"defaultPass";

//#define	__DEADLOCK_DETECT_LOG__
//#define __JOB_SETTING_LOG__

#define EnterCS() _EnterCS(__FILE__,__LINE__)
#define LeaveCS() _LeaveCS(__FILE__,__LINE__)
void _EnterCS(char *szFile,int nLine);
void _LeaveCS(char *szFile,int nLine);
int ThreadIDTable[MAX_THREAD];
int LastRankDay = 0;

// JOB MODE
#define	JOB_EMPTY			0				// Threads is empty
#define	JOB_CHARGED			1				// a thread charge this job
#define	JOB_GET				2				// processing
#define	JOB_COMPLETE		3				// a thread complete process this job. (Threads[].Result is valid)

#define JOBTYPE_ACCOUNT		0
#define JOBTYPE_RANKING		1

struct	STRUCT_JOB
{		int		Mode;						// orocessing state (0:empty  1:requested  2:processing  3:complete)
		int		Type;
		int		ServerGroup;
		STRUCT_ACCOUNTFILE Account;
		char	GuildName[GUILDNAME_LENGTH];
		int		GuildRank;
};
STRUCT_JOB			pJob[MAX_JOB];				// main_thread register job here.

unsigned char	LocalIP[4]={0,};
int				LocalPort;
unsigned		LocalIPBin=0;
CPSock			DBServerSocket[MAX_SERVERGROUP];

//CMob			pMob [MAX_MOB];
int				SecCounter		=0;
int				MinCounter		=0;
int				HourCounter		=0;
int				ServerIndex		=-1;
int				TransperCharacter=0;
int				GuildID			=0;	
//CItem			pItem[MAX_ITEM];



HINSTANCE		hInst;     
HWND			hWndMain;
unsigned int	CurrentTime;
unsigned int	LastSendTime;
HMENU			hMainMenu;
char			temp[1024];

LONG g_nChargedJob;
LONG g_nWorkingThreades;

LONG APIENTRY	MainWndProc(HWND, UINT, UINT, LONG);

void ProcessClientMessage	( int conn , char*msg);

void ProcessSecTimer		( void );
void ProcessMinTimer		( void );

int  GetUserFromSocket		( int Sock );
int  GetAdminFromSocket		( int Sock );

int  GetEmptyUser			( void );
void StartLog				( char * cccc );
void ImportItem				( void );

void DisableAccount			(int conn,char * account);
void EnableAccount			(int conn,char * account);

void Log					( char * String1, char * String2 ,unsigned int ip);
FILE * fLogFile = NULL;

void ProcessUser(void);

int  g_HeightWidth  = MAX_GRIDX;
int  g_HeightHeight = MAX_GRIDY;
int  g_HeightPosX   = 0;
int  g_HeightPosY   = 0;

uintptr_t _stdcall iThreadProc(LPVOID lpParameter);
BOOL SelectUser(int idx, int ServerGroup, char* pAccount);
BOOL UpdateUser(int idx, int ServerGroup, BYTE bySlot, STRUCT_ACCOUNTFILE * file, char* GuildName, int GuildRank);
BOOL InsertUser(int idx, int ServerGroup, BYTE bySlot, STRUCT_ACCOUNTFILE * file, char* GuildName, int GuildRank);
BOOL ExportRanking(int idx);
void GetSqlError(SQLHSTMT hStmt, unsigned char *query);

//2191360
BOOL InitApplication(HANDLE hInstance)   
{    WNDCLASS  wc;  wc.style = CS_VREDRAW|CS_HREDRAW|CS_DBLCLKS;    
     wc.lpfnWndProc = (WNDPROC)MainWndProc;  wc.cbClsExtra = 0;  wc.cbWndExtra = 0;                    
     wc.hIcon = (HICON)LoadIcon ((HINSTANCE)hInstance, "MAINICON");
     wc.hInstance = (HINSTANCE) hInstance;  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
     wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);  wc.lpszMenuName =  NULL;           
     wc.lpszClassName = "MainClass"; if(!RegisterClass(&wc)) return RegisterClass(&wc);
     return TRUE;
}

BOOL InitInstance( HANDLE hInstance, int nCmdShow)  
{    hMainMenu = CreateMenu();
 	 hWndMain = CreateWindow(   "MainClass",  "DB Server", 
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN ,    // Window style. 
        CW_USEDEFAULT,   CW_USEDEFAULT, 640+8,   480,            // W,H[Menu합치면 480]
        NULL,    hMainMenu,  (HINSTANCE)hInstance,   NULL     );
     if (!hWndMain) return (FALSE);
     ShowWindow(hWndMain, nCmdShow);  
     UpdateWindow(hWndMain);          
   return (TRUE);     
}


WINAPI WinMain(  HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{   MSG msg;
    UNREFERENCED_PARAMETER( lpCmdLine );
    hInst = hInstance;
    if (!hPrevInstance)
	if (!InitApplication(hInstance)) return (FALSE);             
    if (!InitInstance(hInstance, nCmdShow)) return (FALSE);


	////////////////////////////////////////////////
	BASE_InitModuleDir();
	BASE_InitializeServerList();

	////////////////////////////////////////////////
	InitializeCriticalSection (&cspJob);
	StartLog("A");
	Log("start log","-system",0);

  	
    /////////////SetLocalAddress////////////////////////////////////////

    char name[255];
    //PHOSTENT hostinfo;
	FILE * fp = fopen("localip.txt","rt");
	if	(fp) 
	{	fscanf(fp,"%s %d",name, &LocalPort);fclose(fp);
	}	else	
	{	MessageBox(hWndMain,"Can't find localip.txt","REBOOTING ERROR",NULL);
	}
	int r1,r2,r3,r4;
	sscanf(name,"%d.%d.%d.%d",&r1,&r2,&r3,&r4);
	LocalIP[0]=r1;	LocalIP[1]=r2;	LocalIP[2]=r3;	LocalIP[3]=r4;	
	unsigned * tmp = (unsigned*)LocalIP;
	LocalIPBin = *tmp;

	for	(int i=0;i<MAX_SERVERGROUP;i++)
	{
		if	(DBServerSocket[i].pSendBuffer==NULL) DBServerSocket[i].pSendBuffer = (char*)malloc(SEND_BUFFER_SIZE);
		if	(DBServerSocket[i].pSendBuffer==NULL) 
		{	sprintf(temp,"err get send buffer fail %d",i);
			Log(temp,"system",0);
		}
		if	(DBServerSocket[i].pRecvBuffer==NULL) DBServerSocket[i].pRecvBuffer = (char*)malloc(RECV_BUFFER_SIZE);
		if	(DBServerSocket[i].pRecvBuffer==NULL) 
		{	sprintf(temp,"err get send buffer fail %d",i);
			Log(temp,"system",0);
		}
	}

    for (int i=0;i<MAX_SERVERNUMBER;i++)
	{	if (!strcmp(g_pServerList[i][INDEXOFDAEMON],name))  {ServerIndex=i;break;}
	}
	if	(ServerIndex==-1)
	{	MessageBox(hWndMain,"Can't get Server Group Index LocalIP:",name,MB_OK|MB_SYSTEMMODAL);
		MessageBox(hWndMain,"Can't get Server Group Index TestServerIP:",g_pServerList[i][INDEXOFDAEMON],MB_OK|MB_SYSTEMMODAL);
		return TRUE;
	}
	int * pip = (int*)LocalIP;
	for (int i=0;i<MAX_SERVERGROUP;i++)
	{	bool bContinue=true;
		if(g_pServerList[i][0][0]==0)	continue;
		if(DBServerSocket[i].Sock!=NULL)		continue;
		for(int j=0;j<i;++j)	//	기연결 대상 dba일경우 생략
		{	if(!strcmp(g_pServerList[i][0], g_pServerList[j][0])) { bContinue=false; break; }
		}
		if(!bContinue) continue;
		int ret = DBServerSocket[i].ConnectServer(g_pServerList[i][0],g_pServerListPort[i][INDEXOFDAEMON]+1000,*pip,WSA_READ,hWndMain);
		if	(ret==NULL)
		{	Log("err Can't connect to DB-Server.","-system",0);
			MessageBox(hWndMain,"Can't connect to DB-SERVER","REBOOTING ERROR",NULL);
			return 0;
		}
	}

	memset(pJob,0,sizeof(pJob));
	memset(ThreadHandle,0,sizeof(ThreadHandle));
	fp = fopen("config.txt","rt");
	if	(fp)
	{
		fscanf(fp,"%s %s %s",sqlDSN,sqlID,sqlPASS);
		fclose(fp);
	}
	char parm[256];
	for ( i=0;i<MAX_THREAD;i++)
	{	sprintf(parm,"%d",i);
		ThreadIDTable[i]=i;
		unsigned ThreadID;
		ThreadHandle[i] = _beginthreadex(NULL, 0, iThreadProc, &ThreadIDTable[i], 0, &ThreadID);
		if	(ThreadHandle[i]==0) MessageBox(hWndMain,"Create Thread Failed",parm,MB_OKCANCEL);
	}

	Sleep(2000);
    SetTimer(hWndMain, 0, 1000,NULL); 
    while (GetMessage(&msg, NULL, 0, 0))
    {     TranslateMessage(&msg);
          DispatchMessage(&msg); 
    }
    return (msg.wParam);  
}
void ProcessMessage(char*msg,int servergroup);

LONG APIENTRY MainWndProc( HWND hWnd, UINT message, UINT wParam, LONG lParam) 
{
	switch (message)    
	{
		case WM_TIMER:
		{ 
			ProcessSecTimer();
		}	break;
		case WSA_READ:
		{
			int nIndex=-1;
			for(int i=0; i<MAX_SERVERGROUP; i++)
			{	if(DBServerSocket[i].Sock == wParam) 
				{	nIndex=i;	break;
				}
			}

			if(nIndex==-1)
			{	closesocket(wParam);
				Log("err wsa_read unregistered socket","-system",0);
				break;
			}

            //
			if	(WSAGETSELECTEVENT(lParam) != FD_READ)
			{	char temp[256];sprintf(temp,"clo server fd %d",DBServerSocket[nIndex].Sock);
				Log(temp,"-system",0);
			}

			CurrentTime = timeGetTime();
			if	(DBServerSocket[nIndex].Receive()!=1)
			{	char temp[256];sprintf(temp,"clo server receive %d",DBServerSocket[nIndex].Sock);
				Log(temp,"-system",0);
				DBServerSocket[nIndex].nRecvPosition = 0;
				DBServerSocket[nIndex].nProcPosition = 0;
				int * pip = (int*)LocalIP;
				DBServerSocket[nIndex].CloseSocket();
				DBServerSocket[nIndex].ConnectServer(g_pServerList[i][0],g_pServerListPort[i][INDEXOFDAEMON]+1000,*pip,WSA_READ,hWndMain);
				// pUser[User].ModeDBServerSocket.=MODE_SAVEEMPTY;
				// DB 서버에 REQ_SAVE를 날리고 
				// MODE를 REQ_SAVE로 바꾼다.
				// Confirm이 오면  MODE_EMPTY
				break;
			}
			int Error;
			int ErrorCode;
			do 
			{	char * Msg = DBServerSocket[nIndex].ReadMessage(&Error,&ErrorCode); 
				if (Msg==NULL) break;
				///////////////////
				if	(Error==1 || Error==2)
				{	sprintf(temp,"err wsa_read (%d),%d",Error,ErrorCode );
					Log(temp,"-system",0);
					int * pip = (int*)LocalIP;
					DBServerSocket[nIndex].CloseSocket();
					DBServerSocket[nIndex].ConnectServer(g_pServerList[i][0],g_pServerListPort[i][INDEXOFDAEMON]+1000,*pip,WSA_READ,hWndMain);
					break;
				} 
				ProcessMessage(Msg,nIndex);// , Manage.ReceiveSize);
			} while(1);
		}	break;     
		case WM_PAINT:
			g_nCurrentTextY = 10;
			PAINTSTRUCT ps;
			BeginPaint(hWnd,&ps);
			DrawInformations(ps.hdc);
			EndPaint(hWnd,&ps);
			break; 
		case WM_CLOSE:
		{	int rret = MessageBox(hWndMain,"서버를 종료합니다. 사용자 데이터를 저장하는 것을 잊지 말아 주세요.","경고!! - 서버 종료",MB_OKCANCEL);
			if	(rret==1)
			{	if (fLogFile) fclose(fLogFile); 
				DefWindowProc(hWnd,message,wParam,lParam);
			}
			return TRUE;
		}    break;
		case WM_DESTROY:
			WSACleanup();
			PostQuitMessage(0);
			break;
		default:                      
			return (DefWindowProc(hWnd, message, wParam, lParam));
		break;
	}
	return (0);
}


void ProcessMessage(char*msg,int ServerGroup)
{	
	MSG_STANDARD * std= (MSG_STANDARD*)msg;
	switch(std->wType)
	{

		case _MSG_NPReqSaveAccount:
		{	MSG_NPAccountInfo * m = (MSG_NPAccountInfo *) msg;
			EnterCS();
			
			for (int t=0;t<MAX_JOB;t++)	{if	(pJob[t].Mode==JOB_EMPTY) break;}
			if	(t==MAX_JOB) // too much connection, try it later
			{	sprintf(temp,"err no empty job, fail to save %s",m->account.AccountName);
				Log(temp,"-system",0);
				LeaveCS();
				return;
			}
			pJob[t].Mode		=	JOB_CHARGED;
			pJob[t].Type		=	JOBTYPE_ACCOUNT;
			memcpy(&pJob[t].Account,&m->account,sizeof(STRUCT_ACCOUNTFILE));
			pJob[t].ServerGroup = ServerGroup;
			strncpy(pJob[t].GuildName,m->GuildName,GUILDNAME_LENGTH);
			pJob[t].GuildRank = m->GuildRank;
			InterlockedIncrement(&g_nChargedJob);
			LeaveCS();
		}	break;
	}
}



void RecoverConnection(void)
{
	int * pip = (int*)LocalIP;
	for (int i=0;i<MAX_SERVERGROUP;i++)
	{	bool bContinue=true;
		if	(g_pServerList[i][0][0]==0)	continue;
		if	(DBServerSocket[i].Sock!=NULL) continue;	
		for(int j=0;j<i;++j)	//	기연결 대상 dba일경우 생략
		{	if(!strcmp(g_pServerList[i][0], g_pServerList[j][0])) { bContinue=false; break; }
		}

		if(!bContinue) continue;
		int ret = DBServerSocket[i].ConnectServer(g_pServerList[i][0],g_pServerListPort[i][INDEXOFDAEMON]+1000,*pip,WSA_READ,hWndMain);
		if	(ret==NULL)
		{	Log("err reconnect dbsrv file xxxxxxxxxxxxxxxxxx","-system",0);
			return;
		}
	}
}



void ProcessSecTimer( void )
{
	
	SecCounter++;
	// AccountLogin에 딜레이를 주기 위한 코드?
	if	(SecCounter%60==0)
	{	//
		MinCounter++;
		RecoverConnection();

		// 1분에 한번씩 날자, 시간 체크해서 오전 5시면 Ranking 만듦.
		struct tm when; time_t now;
		time(&now); when= *localtime(&now);
		if(when.tm_hour>=5&&LastRankDay!=when.tm_mday)
		{
			EnterCS();
			for (int t=0;t<MAX_JOB;t++)	{if	(pJob[t].Mode==JOB_EMPTY) break;}
			if	(t==MAX_JOB) // too much connection, try it later
			{	sprintf(temp,"err no empty job, fail to ranking");
				Log(temp,"-system",0);
				LeaveCS();
				return;
			}
			ZeroMemory(&pJob[t],sizeof(STRUCT_JOB));
			pJob[t].Mode		=	JOB_CHARGED;
			pJob[t].Type		=	JOBTYPE_RANKING;
			InterlockedIncrement(&g_nChargedJob);
			LastRankDay=when.tm_mday;
			LeaveCS();
		}		

		if	(MinCounter%60==0)
		{	//
			HourCounter++;
		}
	}

	
}
//csConnecters[MAX_SERVERGROUP]
int	ProcessJob(int idx,STRUCT_ACCOUNTFILE account,int ServerGroup,char* GuildName, int GuildRank)
{	
	int ret=0;
	BYTE byIndex=0; int nBrahmanPoint=0; BYTE byLevel=0;
	for(BYTE i=0;i<MOB_PER_ACCOUNT;i++)
	{	if(account.Char[i].nBramanPoint<nBrahmanPoint || account.Char[i].szName[0] == 0) continue;
		if(account.Char[i].nBramanPoint==nBrahmanPoint && account.Char[i].byLevel<byLevel) continue;
		byIndex=i;
		nBrahmanPoint=account.Char[i].nBramanPoint;
		byLevel=account.Char[i].byLevel;
	}

	int exist = SelectUser(idx, ServerGroup, account.AccountName);
	if	(exist==-1)
	{	EnterCS();
		pJob[idx].Mode = JOB_EMPTY;
		Log("Error selecting ", account.AccountName, 0); 
		LeaveCS();
		return -1;
	}

	if	(exist)
	{	ret = UpdateUser(idx, ServerGroup, byIndex, &account, GuildName, GuildRank);
	}	else
	{	ret = InsertUser(idx, ServerGroup, byIndex, &account, GuildName, GuildRank);
	}
	if	(ret==FALSE)
	{	EnterCS();
		pJob[idx].Mode = JOB_EMPTY;
		Log("Error importing ", account.AccountName, 0); 
		LeaveCS();
		return -1;
	}

	EnterCS();
	pJob[idx].Mode = JOB_EMPTY;
	LeaveCS();
	return TRUE;
}


uintptr_t __stdcall iThreadProc(void* lpParameter)
{
	int ThreadID = *((int*)lpParameter);
	
	// db connect
	EnterCS();
	int ret = mSql[ThreadID].DBConnect(sqlDSN,sqlID,sqlPASS);
	if	(ret==FALSE)
	{
		char temp[256];
		sprintf(temp,"Thread: %d fail to connect sql. DSN:%s ID:%s PASS:*****",ThreadID,sqlDSN,sqlID,sqlPASS);
		MessageBox(hWndMain,temp,"error",MB_OK|MB_SYSTEMMODAL);
	}
	LeaveCS();
	
	while(1)
	{	
		EnterCS();
		InterlockedIncrement(&g_nWorkingThreades);
		for	(int idx=0;idx<MAX_JOB;idx++)
		{	if	(pJob[idx].Mode==JOB_CHARGED) break;
		}
			
		if	(idx==MAX_JOB)
		{	InterlockedDecrement(&g_nWorkingThreades);
			LeaveCS();
			Sleep(100);
			continue;
		}

		#ifdef __JOB_SETTING_LOG__
			char logtemp[256];
			sprintf(logtemp,"dbg [iThreadProc] ThreadID(%d) idx(%d) JobType(%d) Charged(%d) WorkThread(%d)",ThreadID,idx,pJob[idx].Type,g_nChargedJob,g_nWorkingThreades);
			Log(logtemp,"-system",0);
		#endif

		switch(pJob[idx].Type)
		{
			case JOBTYPE_ACCOUNT:
			{	pJob[idx].Mode		=	JOB_GET;
				STRUCT_ACCOUNTFILE	account = pJob[idx].Account;
				int					servergroup	= pJob[idx].ServerGroup;
				char				GuildName[GUILDNAME_LENGTH];
				strncpy(GuildName,pJob[idx].GuildName,GUILDNAME_LENGTH);
				int GuildRank = pJob[idx].GuildRank;
				InterlockedDecrement(&g_nChargedJob);
				InterlockedDecrement(&g_nWorkingThreades);
				LeaveCS();
				ProcessJob(ThreadID,account,servergroup,GuildName, GuildRank);
				EnterCS();
				pJob[idx].Mode		=	JOB_EMPTY;
				LeaveCS();
			}	break;
			case JOBTYPE_RANKING:
			{	pJob[idx].Mode		=	JOB_GET;
				InterlockedDecrement(&g_nChargedJob);
				InterlockedDecrement(&g_nWorkingThreades);
				LeaveCS();
				ExportRanking(ThreadID);
				EnterCS();
				pJob[idx].Mode		=	JOB_EMPTY;
				LeaveCS();
			}	break;
			default:
			{	pJob[idx].Mode		=	JOB_EMPTY;
				InterlockedDecrement(&g_nChargedJob);
				InterlockedDecrement(&g_nWorkingThreades);
				LeaveCS();
			}	break;
		}
		Sleep(100);
	}
	return 1;
}
void _EnterCS(char *szFile, int nLine)
{
	EnterCriticalSection(&cspJob);
	#ifdef __DEADLOCK_DETECT_LOG__
		sprintf(temp,"lck EnterCS('%s',%d)",szFile,nLine);
		Log(temp,"-system",0);
	#endif
}
void _LeaveCS(char *szFile, int nLine)
{	
	#ifdef __DEADLOCK_DETECT_LOG__
		sprintf(temp,"lck LeaveCS('%s',%d)",szFile,nLine);
		Log(temp,"-system",0);
	#endif
	LeaveCriticalSection (&cspJob);
}

/////////////////////////////////////////////////////////////////////////////////
//
//  로그 파일 관련 펑션 StartLog(), Log()
//
//////////////////////////////////////////////////////////////////////////////////
void StartLog ( char * cccc )
{	 if (fLogFile!=NULL)
	 {  int ret=fclose(fLogFile);
	    if (ret) Log("Logfile close fail!!","-system",0);
	 }
	 struct tm when;
	 time_t now;
	 time(&now); when= *localtime(&now);
     sprintf(temp,".\\Log\\DB%4.4d_%2.2d_%2.2d_%2.2d_%2.2d%s.txt",when.tm_year+1900,when.tm_mon+1,when.tm_mday,when.tm_hour,when.tm_min,cccc);
   	 fLogFile=fopen(temp,"wt");
}		   

void Log(char * str1,char * str2,unsigned int ip)
{	 struct tm when;
	 time_t now;
	 time(&now); when= *localtime(&now);
	 char LogTemp[1024];
     sprintf(LogTemp,"%2.2d%2.2d%2.2d %2.2d%2.2d%2.2d",when.tm_year-100,when.tm_mon+1,when.tm_mday,when.tm_hour,when.tm_min,when.tm_sec);
     sprintf(LogTemp,"%s %s\t\t%s \n",LogTemp,str2,str1);
	 if (fLogFile)
	 {  fprintf(fLogFile,LogTemp);
		fflush(fLogFile);
	 }
	 //SetWindowText(hWndMain,LogTemp);
}

BOOL SelectUser(int idx, int ServerGroup, char* pAccount)
{
	mSql[idx].GetStmt();
	unsigned char query[1024];
	sprintf((char*)query,"SELECT idx FROM %s%02d WHERE UserID = '%s'", DBTABLE, ServerGroup, pAccount);
	SQLRETURN retcode = SQLExecDirect(mSql[idx].g_hstmt,query, SQL_NTS);
	if	(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) 
	{	GetSqlError(mSql[idx].g_hstmt, query);
		mSql[idx].FreeStmt();
		return -1;
	}
    retcode = SQLFetch(mSql[idx].g_hstmt);
	SQLCloseCursor(mSql[idx].g_hstmt);
	if (retcode == SQL_NO_DATA) { mSql[idx].FreeStmt();return FALSE; }
	mSql[idx].FreeStmt();
	return TRUE;
}

BOOL InsertUser(int idx, int ServerGroup, BYTE bySlot, STRUCT_ACCOUNTFILE * file, char* GuildName, int GuildRank)
{
	mSql[idx].GetStmt();
	int sz =sizeof(*file);
	char tmp[] ="INSERT INTO %s%02d (UserID, CharacterName,"
				"CharacterLevel, BrahmanPoint, Tribe, Trimurity, GuildName, GuildID, GuildRank, curtime,"
				"Name1, Name2, Name3, Level1, Level2, Level3, TotalMoney,"
				"Blocked)"
				"VALUES ('%s','%s',%d,%d,%d,%d,'%s',%d,%d,getdate(),'%s','%s','%s',%d,%d,%d,%f,%d)"; //fors_debug 쉥account俚뙈伽쫠

	double dbTotalMoney = file->Coin;
	for(int i = 0 ; i < MOB_PER_ACCOUNT ; i++) { dbTotalMoney += file->Char[i].nRupiah; }

	char szGuildName[256] = {0, };
	char *pPos = GuildName;
	char *pPos2 = GuildName;
	while ( ( pPos = strchr( pPos, '\'' ) ) != NULL )	
	{
		strncat( szGuildName, pPos2, pPos - pPos2 );
		strcat( szGuildName, "''" );
		pPos++;
		pPos2 = pPos;
	}
	strcat( szGuildName, pPos2 );

	char szCharName[3][256] = {0, };
	for ( int i = 0 ; i < MOB_PER_ACCOUNT ; i++ )
	{
		pPos = file->Char[i].szName;
		pPos2 = file->Char[i].szName;
		while( ( pPos = strchr( pPos, '\'' ) ) != NULL )
		{	strncat( szCharName[i], pPos2, pPos - pPos2 );
			strcat( szCharName[i], "''" );
			pPos++;
			pPos2 = pPos;
		}
		strcat( szCharName[i], pPos2 );
	}

	unsigned char query[2048];
	sprintf((char*)query,tmp,DBTABLE,ServerGroup,file->AccountName,szCharName[bySlot],
		file->Char[bySlot].byLevel,file->Char[bySlot].nBramanPoint,file->Char[bySlot].snTribe,
		file->Char[bySlot].byTrimuriti,szGuildName,file->Char[bySlot].nGuildID,GuildRank,
		szCharName[0],szCharName[1],szCharName[2],
		file->Char[0].byLevel,file->Char[1].byLevel,file->Char[2].byLevel,dbTotalMoney,
		file->AccountPass[0] != 1 ? 0 : 1 );

	SQLRETURN retcode = SQLPrepare(mSql[idx].g_hstmt,(unsigned char*)query, SQL_NTS);
	if	(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) 
	{	GetSqlError(mSql[idx].g_hstmt, query);
		SQLCloseCursor(mSql[idx].g_hstmt);
		mSql[idx].FreeStmt();
		return FALSE;
	}

	SQLPOINTER	pAddr; 
	SQLINTEGER	cbData		=0;
	retcode = SQLBindParameter(mSql[idx].g_hstmt,1,SQL_PARAM_INPUT, SQL_C_BINARY,SQL_LONGVARBINARY,sz,0,(SQLPOINTER)file,0,&cbData);	
	if	(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) 
	{	GetSqlError(mSql[idx].g_hstmt, query);
		SQLCloseCursor(mSql[idx].g_hstmt);
		mSql[idx].FreeStmt();
		return FALSE;
	}

	cbData = SQL_LEN_DATA_AT_EXEC(0);
	retcode = SQLExecute(mSql[idx].g_hstmt);
	if	(retcode == SQL_NEED_DATA) 
	{	while ((retcode = SQLParamData(mSql[idx].g_hstmt, &pAddr)) == SQL_NEED_DATA)
		{	retcode = SQLPutData(mSql[idx].g_hstmt, file, sz);
		};
		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)  
		{	GetSqlError(mSql[idx].g_hstmt, query);
			SQLCloseCursor(mSql[idx].g_hstmt);
			mSql[idx].FreeStmt();
			return FALSE;
		}   
	}	else
	{	if	(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) 
		{	GetSqlError(mSql[idx].g_hstmt, query);
			SQLCloseCursor(mSql[idx].g_hstmt);
			mSql[idx].FreeStmt();
			return FALSE;
		}
	}
	SQLCloseCursor(mSql[idx].g_hstmt);
	mSql[idx].FreeStmt();
	return TRUE;
}

BOOL UpdateUser(int idx, int ServerGroup, BYTE bySlot, STRUCT_ACCOUNTFILE * file, char* GuildName, int GuildRank)
{
	mSql[idx].GetStmt();
	int sz =sizeof(*file);
	char temp[] ="UPDATE %s%02d SET UserID='%s', CharacterName='%s',"
				 "CharacterLevel=%d, BrahmanPoint=%d, Tribe=%d, Trimurity=%d, GuildName='%s',"
				 "GuildID=%d,GuildRank=%d, curtime=getdate(),"
				 "Name1='%s',Name2='%s',Name3='%s',Level1=%d,Level2=%d,Level3=%d,TotalMoney=%f,"
				 "Blocked=%d WHERE UserID='%s' ";

	double dbTotalMoney = file->Coin;
	for(int i = 0 ; i < MOB_PER_ACCOUNT ; i++) { dbTotalMoney += file->Char[i].nRupiah; }

	char szGuildName[256] = {0, };
	char *pPos = GuildName;
	char *pPos2 = GuildName;
	while ( ( pPos = strchr( pPos, '\'' ) ) != NULL )	
	{	strncat( szGuildName, pPos2, pPos - pPos2 );
		strcat( szGuildName, "''" );
		pPos++;
		pPos2 = pPos;
	}
	strcat( szGuildName, pPos2 );

	char szCharName[3][256] = {0, };
	for ( int i = 0 ; i < MOB_PER_ACCOUNT ; i++ )
	{
		pPos = file->Char[i].szName;
		pPos2 = file->Char[i].szName;
		while( ( pPos = strchr( pPos, '\'' ) ) != NULL )
		{	strncat( szCharName[i], pPos2, pPos - pPos2 );
			strcat( szCharName[i], "''" );
			pPos++;
			pPos2 = pPos;
		}
		strcat( szCharName[i], pPos2 );
	}

	unsigned char query[2048];
	sprintf((char*)query,temp,DBTABLE,ServerGroup,file->AccountName,szCharName[bySlot],
		file->Char[bySlot].byLevel, file->Char[bySlot].nBramanPoint, file->Char[bySlot].snTribe,
		file->Char[bySlot].byTrimuriti,szGuildName,file->Char[bySlot].nGuildID,GuildRank,
		szCharName[0],szCharName[1],szCharName[2],file->Char[0].byLevel,file->Char[1].byLevel,file->Char[2].byLevel,dbTotalMoney,
		file->AccountPass[0] != 1 ? 0 : 1,file->AccountName);

	SQLRETURN retcode;
	retcode = SQLPrepare(mSql[idx].g_hstmt,(unsigned char*)query, SQL_NTS);
	if	(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) 
	{	GetSqlError(mSql[idx].g_hstmt, query);
		SQLCloseCursor(mSql[idx].g_hstmt);
		mSql[idx].FreeStmt();
		return FALSE;
	}
	SQLPOINTER	pAddr; 
	SQLINTEGER	cbData	=0;
	retcode = SQLBindParameter(mSql[idx].g_hstmt,1,SQL_PARAM_INPUT, SQL_C_BINARY,SQL_LONGVARBINARY,sz,0,(SQLPOINTER)file,0,&cbData);	
	if	(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) 
	{	GetSqlError(mSql[idx].g_hstmt, query);
		SQLCloseCursor(mSql[idx].g_hstmt);
		mSql[idx].FreeStmt();
		return FALSE;
	}

	int len = strlen((char*)query);

	cbData = SQL_LEN_DATA_AT_EXEC(0);
	retcode = SQLExecute(mSql[idx].g_hstmt);

	if	(retcode == SQL_NEED_DATA) 
	{	while ((retcode = SQLParamData(mSql[idx].g_hstmt, &pAddr)) == SQL_NEED_DATA)
		{	retcode = SQLPutData(mSql[idx].g_hstmt, file, sz);
		};
		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)  
		{	GetSqlError(mSql[idx].g_hstmt, query);
			SQLCloseCursor(mSql[idx].g_hstmt);
			mSql[idx].FreeStmt();
			return FALSE;
		}   
	}	else
	{	if	(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) 
		{	GetSqlError(mSql[idx].g_hstmt, query);
			SQLCloseCursor(mSql[idx].g_hstmt);
			mSql[idx].FreeStmt();
			return FALSE;
		}
	}
	if	(retcode==SQL_NO_DATA) 
	{	GetSqlError(mSql[idx].g_hstmt, query);
		SQLCloseCursor(mSql[idx].g_hstmt);
		mSql[idx].FreeStmt();
		return retcode;
	}
	SQLCloseCursor(mSql[idx].g_hstmt);
	mSql[idx].FreeStmt();
	return TRUE;
}

BOOL ExportRanking(int idx)
{
	char temp[] ="SELECT TOP 250 CharacterName,Trimurity,Tribe "
		" FROM TantraBackup%02d"
		" WHERE Trimurity = %d AND BrahmanPoint > 10000 AND DATEDIFF(day, curtime, GETDATE() ) < 60"
		" ORDER BY BrahmanPoint DESC";
	char query[2048] = {0,};
	char filename[256] = {0,};
	char logtemp[256] = {0,};

	for ( int nSrv = 0 ; nSrv <= MAX_SERVERGROUP ; nSrv++ )
	{
		if ( g_pServerList[nSrv][0][0] == 0 ) continue;
		sprintf(filename,".\\Rank\\Rank%02d.txt",nSrv);
		FILE *pFp = fopen(filename,"wt");
		if ( pFp == NULL )
		{	Log("err Rank file open failed", "-system", 0);
			continue;
		}
		for ( int nTri = 0 ; nTri < 3 ; nTri++ )
		{
			int nTrim = 0x01 << nTri;
			mSql[idx].GetStmt();
			sprintf( query, temp, nSrv, nTrim );

			int nRank = 1;
			SQLRETURN retcode = FALSE;

			SQLINTEGER cbCharName;
			SQLCHAR szCharName[256];
			SQLBindCol(mSql[idx].g_hstmt,1,SQL_C_CHAR,&szCharName,64,&cbCharName);

			retcode = SQLExecDirect(mSql[idx].g_hstmt,(unsigned char*)query,SQL_NTS);
			if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
			{	GetSqlError(mSql[idx].g_hstmt,(unsigned char*)query);
				SQLCloseCursor(mSql[idx].g_hstmt);
				mSql[idx].FreeStmt();
				fclose(pFp);
				return FALSE;
			}

            while ( ( retcode = SQLFetch(mSql[idx].g_hstmt) ) != SQL_NO_DATA )
			{	fprintf( pFp, "%d %d %s\n", nTrim, nRank, szCharName );
				nRank++;
			}

			SQLCloseCursor(mSql[idx].g_hstmt);
			mSql[idx].FreeStmt();
		}
		sprintf(logtemp,"dbg Rank for serv%02d is exported",nSrv);
		Log(logtemp,"-system",0);
		fclose( pFp );
	}

	return TRUE;
}

void GetSqlError(SQLHSTMT hStmt, unsigned char *query)
{
	SQLCHAR SQLState[6], SQLMessage[SQL_MAX_MESSAGE_LENGTH];
	SQLINTEGER NativeError;
	SQLSMALLINT MsgLen;
	SQLGetDiagRec( SQL_HANDLE_STMT, hStmt, 1, SQLState, &NativeError, SQLMessage, sizeof(SQLMessage), &MsgLen );

	sprintf(temp,"qry %s",query);
	Log(temp, "-system", 0);
	sprintf(temp, "sql %s", SQLMessage);
	Log(temp, "-system", 0);
}