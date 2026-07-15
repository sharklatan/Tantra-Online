#ifndef _CFILEDB_
#define _CFILEDB_
#include <windows.h>
#include "BaseDef.h"


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//                  USERFILE
//
// account : ��ī��Ʈ DB ����
// char    : ĳ���� DB ����
//
//  
// account �� char-name �� ex-filename �� ������ �ѵ����� ����.
// ( * ? " ~ `) �Ұ� ,  ( ' ) �� ����.       
// ID
// PASS
// ��Ÿ ��������
// ��������
// MOB Index
// 
// MOB1    // 1 K //
// MOB2    // 1 K //
// MOB3    // 1 K //
// MOB4    // 1 K //
// MOB5    // 1 K //
// MOB6    // 1 K //
//
//            total : 8K
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
/*
typedef struct 
{
////////// MSG_DBNewAccount
        char AccountName[ACCOUNTNAME_LENGTH];
	    char AccountPass[ACCOUNTPASS_LENGTH];
	    char RealName[REALNAME_LENGTH];
        unsigned int SSN1;
	    unsigned int SSN2;
        char Email[EMAIL_LENGTH];
        char Telephone[TELEPHONE_LENGTH];
        char Address[ADDRESS_LENGTH];
////////// �� �κ��� �׻� BaseDEF�� MSG_DBNewAccount �� ��ġ���Ѿ� �Ѵ� .
		unsigned short GameServer;// conn�� ���Ӽ����ε�����. ���Ӽ����� ������ ���� �����ϰ� conn�� �����Ѵ�.
        char MobName[MOB_PER_ACCOUNT][MOBNAME_LENGTH];
		unsigned short Login;
}  STRUCT_ACCOUNT; 
*/

typedef struct _HTStr
{
	template<typename PtrType>
		bool operator()( PtrType pStr1, PtrType pStr2 ) const
	{
		int i = _stricmp( pStr1, pStr2 );
		if ( i < 0 )
			return ( TRUE );
		else
			return ( FALSE );
	}
} HTStr;

typedef struct
{    int  Login;
     int  Slot;
     STRUCT_ACCOUNTFILE File;
} STRUCT_ACCOUNTLIST;

class CFileDB
{
private:
	int m_iTrimuriti[3][25];	//	3�ֽ��� 24�ð� �����ڼ�(�ð����� üũ�Ѵ�), 25��°�� �հ踦 ����.

public:
    STRUCT_ACCOUNTLIST pAccountList[MAX_DBACCOUNT];
	//std::map<char*, int, HTStr> m_mapAccTable;
	//std::map<char*, int, HTStr> m_mapCharTable;

   	CFileDB();
	~CFileDB();
	void	InitGuild();
	BOOL   ProcessMessage			(char * Msg, int User);
    void   Remove					(void);
	BOOL   SendDBMessage			(int svr,unsigned short id, char * Msg);
	BOOL   SendDBSignal				(int svr,unsigned short id,unsigned short signal);
	BOOL   SendDBSignalParm			(int svr,unsigned short id, unsigned short signal,int parm);
	BOOL   SendDBSignalParm2		(int svr,unsigned short id, unsigned short signal,int parm1,int parm2);
	BOOL   SendDBSignalParm3		(int svr,unsigned short id, unsigned short signal,int parm1,int parm2,int parm3);

	BOOL   AddAccount(char *id,char*pass,int ssn1,int ssn2);
	BOOL   UpdateAccount(char *id,char*pass,int ssn1,int ssn2);
	BOOL	DBExportAccount(STRUCT_ACCOUNTFILE * account);
     
	int    GetIndex                  (char * account);                                  // -1 : ��ġ�ϴ� ��ī��Ʈ ����.
	int    GetIndex                  (int server ,int id);
	int	   GetIndexFromName			 (char * szMob);
    void   AddAccountList            (int Idx);   // -1 : fail, empty�� ã�� ä��� idx����
	void   RemoveAccountList         (int Idx);
	void   SendDBSavingQuit          (int Idx,int mode);
     
	// ����� ���� (account ���丮)
	BOOL   DBWriteAccount            (STRUCT_ACCOUNTFILE * account);
    BOOL   DBReadAccount             (STRUCT_ACCOUNTFILE * account,time_t *ptLastWrite = NULL);

	// Account�� MOB�� Charactor�� ����. ( char ���丮)
    BOOL   DBCheckImpleName          (char **source,char * name);
    void   DBGetSelChar              (S_SSP_RESP_CHAR_LIST * sel,STRUCT_ACCOUNTFILE * file);
	void   InitAccountList           (int idx);
    
	BOOL   CreateCharacter( char *account, char * character);
    BOOL   DeleteCharacter( char *account, char * character);
	void   GetAccountByChar(char *acc, char *cha);

    void	SetNewCharacter( STRUCT_MOB *pMob, S_SSP_REQ_CHAR_CREATE * pData );
	void	SendToAll(MSG_STANDARD * msg);
	BOOL	ReadGuildFile		(int gid,	STRUCT_GUILD* guild);
	BOOL	WriteGuildFile		(int gid,	STRUCT_GUILD* guild);
	BOOL	CreateGuildFile		(int gid,	MSG_CreateGuild* pData);
	BOOL	AddGuildMember		(int gid,	MSG_AddGuildMember * pData);
	int		RemoveGuildMember	(int gid,	char * user, char* pMaster);
	BOOL	UpdateGuild			(int gid,	MSG_GuildUpdate * pData);
	BOOL	UpdateGuildMember	(int gid,	MSG_GuildUpdateMember * pData);
	BOOL	UpdateGuildMark(int gid, MSG_GuildUpdateMark * pData);
	BOOL	UpdateGuildMemberRank(int gid, MSG_GuildSetRanking * pData);
	int		GetGuildID			(char* pName);
	void	CheckTrimuriti();
	BOOL	UpdateGuildCargoLevel	(int nID, BYTE byLevel1, BYTE byLevel2, BYTE byLevel3);
	BOOL	UpdateGuildCargoTime	(int nID, DWORD dwTime1, DWORD dwTime2, DWORD dwTime3);
	BOOL	UpdateGuildCargoItem	(int nID, int nIndex, STRUCT_ITEM* pstItem);

	// ---- Funciones reconstruidas de DBSRV_NEW ----
	int		CreateAccount		(const char* pszAccountName, const char* pCharData, int nLen, int nFlag);
	int		CreateAccount2		(const char* pszAccountName, const char* pCharData, int nLen, int nFlag);
	int		GetGMPermission		(const char* pszAccountName, const char* pszCheckName);
	void	LoadInitItems		();
	void	LoadSkillData		();
	void	OpenLogFile			(int nSuffix);
	// -- funcs2 --
	int		DeleteAccountWithBackup	(const char* pszAccountName, const char* pszCharName, int nMode);
	int		DBReadOldAccount		(const char* pszAccountName, void* pOutData);
	void	LogAccountInfo			(int nUser, const char* pszAccount, const char* pszChar, int nSlot, unsigned char byAccountType, unsigned char* pClientIP);
	void	LogAdminGuildCargo		(int nUser, const char* pszMsg, unsigned char* pClientIP, int nGuildID);
	// -- funcs4 --
	int		CheckAccount			(const char* pszAccountName, void* pOutData, int nFlag);
	int		CheckAccount2			(void* pMsg, int* pOutSlotID);
	void	LogAccountSaveFail		(const char* pszAccountName);
	// -- funcs5 --
	int		DBWriteAccount2			(STRUCT_ACCOUNTFILE* pData);
	int		DBWriteOneTimeAccount	(void* pData);
	int		DBCreateChar			(const char* pszAccountName, const char* pszCharName);
	int		DBDeleteChar			(const char* pszCharName, const char* pszAccountName, int nMode);
	void	DBReadChar				(char* pszOutAccount, const char* pszCharName);
	int		DBCharExists			(const char* pszCharName);
	// -- funcs6 --
	int		DBWriteChar				(const char* pszCharName, void* pData);
	int		GetCharName				(void* pMsg, void* pResult);
	int		GetGuildID				(const char* pszGuildName);
	void	Rankings_WriteRank		(void* pAccount, int nServerGroup, int nSlot, int bMRank);
	void	Rankings_ReadRank		(void* pAccount, int nRankIndex, int bWrite, int bMRank);
	void	Rankings_UpdateRank		(void* pAccount, int nRankIndex, int nType);
	int		Disciple_Create			(void* pMsg, void* pMasterData, void* pDiscipleData);
	int		Disciple_Read			(int nGuildID, void* pOut);
	int		Disciple_Write			(int nGuildID, void* pData);
	void	ExportData				();
	int		LoadInitItemBin			();
	int		SaveExtraItemBin		(const char* pszFileName, int nMode);
	// -- funcs7 --
	void	_Msg_EditChar			(int nUser, void* pMsg);
	void	_Msg_CreateChar			(int nUser, void* pMsg);
	void	_Msg_DeleteChar			(int nUser, void* pMsg);
	void	_Msg_RenameChar			(int nUser, void* pMsg);
};

// ---- Globals de DBSRV_NEW ----
// Tabla de items iniciales: stride 8 bytes (4 x short)
// Equivale a VA 0x186124C0 del binario NEW
#define MAX_INIT_ITEMS		512
extern short	g_InitItemTable[MAX_INIT_ITEMS][4];

// Tabla de skills: stride 0x54 bytes (21 x int)
// Equivale a VA 0x186126C0 del binario NEW
struct SkillEntry { int f[21]; };
extern SkillEntry	g_SkillData[MAX_SKILL_DATA];

// Log global
extern FILE*	g_pLogFile;
extern char		g_szLogPath[256];

// Globals de configuracion (Settings.ini)
extern int  g_nCountryID;
extern int  g_nSWorld1;
extern int  g_nSWorld2;
extern int  g_nDWorld;
extern int  g_nWorld1Size;
extern int  g_nWorld2Size;
extern int  g_nAgeLimit;

// ReadConfig - lee Settings.ini completo
void ReadSettings();
bool IsReservedDeviceName(const char* pszName);
void ReadAdminTxt();
void Log(char* str1, char* str2, unsigned int ip);
extern unsigned int pAdminIP[]; // lee Settings.ini (NEW)

// Funcion de log con timestamp (usada en Server.cpp para EditHistory)
// se implemento pero comento rompia compilacion mal implementada
void GetSubDir(const char* pszAccountName, char* pszSub, int nMaxLen);
#endif