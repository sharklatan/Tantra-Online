/****************************************************************************

	파일명 : TNSiege.cpp

	작성자 : 정재웅(spencerj@korea.com)
	작성일 : 2005-07-07

	Tab size : 4 spaces
	프로젝트명 : Tantra


	설명 : 
		- 
		- 

****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include "TNDebug.h"
#include "Basedef.h"
#include "CMob.h"
#include "TNSiege.h"
#include "Server.h"
#include "Language.h"
#include "CGuild.h"
#include "SendFunc.h"

//extern CUser					pUser    [MAX_USER];

//extern int                      g_iZoneID ;
extern HWND                     hWndMain;
extern STRUCT_MOB				pMonsterData[MAX_MONSTER_DATA] ;
extern unsigned short         pMobGrid   [MAX_GRIDY][MAX_GRIDX];
extern int						g_iCastleOwner;

TNSiege g_kSiege ;


// static : (x,y), point
short g_srgSymbolForSiege[TNSiege::eSiege_SymbolCount][3] = 
{
	656, 618, 3,
	455, 609, 2,
	646, 761, 2,
	527, 770, 1,
	399, 762, 1,
	373, 618, 1,
	453, 456, 1,
	365, 387, 1,
	547, 436, 1,
	626, 418, 1,
	549, 308, 1,
} ;
/*
// 2005.09.14, 3번째 변경
373	619	1	---->	373	618
365	387	1			
453	456	1			
547	436	1			
626	419	1	---->	627	418
548	308	1	---->	549	308


// 2005.10.04, 4번째 변경
3점상징물
   게임좌표(655,618)
   게임좌표(454,609)
   게임좌표(646,761)
   게임좌표(527,770)
   게임좌표(398,763)
   게임좌표(373,619)
   게임좌표(452,460)
   게임좌표(365,386)
   게임좌표(547,437)
   게임좌표(626,419)
   게임좌표(549.308)
*/


TNSiege::TNSiege() : m_iStarted(0), m_iWinner(0), m_iDateToHold(0), m_iExpiryOftheTerm(0)
{
	Init() ;
}



TNSiege::~TNSiege()
{
	
}


void TNSiege::Init()
{
	memset( m_krgEntry, 0, sizeof(m_krgEntry) ) ;
	memset( m_irgSymbol, 0, sizeof(m_irgSymbol) ) ;
	memset( &m_kOwner, 0, sizeof(m_kOwner) ) ;
	//memset( m_sgrgLeader, 0, sizeof(m_sgrgLeader) ) ;
}


// entry
//int TNSiege::BuildEntry()
//{
//
//	return eTNRes_Succeeded ;
//}
// 공성전 초기화, 수성측 설정, 상징물 clan은 수성측
// entry 초기화
void TNSiege::InitEntry()
{
	memset( m_krgEntry, 0, sizeof(m_krgEntry) ) ;
	strcpy( m_krgEntry[eSide_Defense][0].szName, g_pMessageStringTable[_DefenderName] ); // 수성이라는 이름 등록	
}


void TNSiege::RegisterCastleOwner()
{
	m_krgEntry[eSide_Defense][0] = m_kOwner.kGuild ;
	int iAlly = m_kOwner.iOwnerFriend ;
	if( 0 > iAlly || MAX_USER <= iAlly ) iAlly = 0 ;
	RegisterEntry( iAlly, 0, 1, 1 ) ;
}


void TNSiege::set_Started( int a_iFlag ) //{ m_iStarted = a_iFlag ; } // 1이면 공성전 설정, 0이면 공성전 설정 해제
{
	//SYSTEMTIME st ;
	//GetLocalTime( &st ) ;

	RegisterCastleOwner() ;
	//m_krgEntry[eSide_Defense][0] = m_kOwner.kGuild ;
	//strcpy( m_sgrgLeader[eSiege_Army], g_pMessageStringTable[_Independent] ) ;

	m_iStarted = a_iFlag ;
	if( m_iStarted )
	{
		for( int i = 1 ; i < eSiege_Army ; ++i )
		{		
			int iGuildIndex = GetGuild( m_krgEntry[i][0].iID, FLAG_OPEN ) ;
			if( 0 > iGuildIndex || MAX_USER <= iGuildIndex )
			{
				#ifdef __TN_TOP_LOG__
				{
					char chBuf[512] = { 0,0,0, } ;
					sprintf(chBuf, "[%dMM%dDD %2dH%2dM%2dS] set_Started() > Invalid Guild Index(%d) \r\n"
						, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond, iGuildIndex
						) ;
					WriteLog( chBuf, g_szrgLogFileName[eLogFileName_Castle] ) ;
				}
				#endif 
				continue ;
			}

			//strcpy( m_sgrgLeader[i], pGuild[iGuildIndex].GUILD.GuildName ) ;
		}
	}
}


// 공성전 신청을 위한 조건이 있을 것이다. 이것은 메시지를 받았을 때, 처리하는 방식으로 수행한다.
// 하나의 guild가 신청을 하면, 연합 길드도 찾아서 자동으로 등록을 시켜줘야 한다.
// 이미 등록되어 있다면, fail 시켜줘야 한다.
int TNSiege::RegisterEntry( int a_iGuildID, int a_iClanSlot, int a_iExpandSlot, int a_iPC )
{
	if( 1 == m_iExpiryOftheTerm ) return eTNRes_ExpiryOftheTerm ; // 신청기간이 종료되었다.
	if( 0 > a_iClanSlot || eSiege_Army <= a_iClanSlot ) return eTNRes_Failed ;
	if( 0 > a_iExpandSlot || eSiege_MaxEntry <= a_iExpandSlot ) return eTNRes_Failed ;
	if( 0 == a_iClanSlot && 0 == a_iExpandSlot ) return eTNRes_Failed; // 수성 맹주로는 등록신청할 수 없다.
	if( 0 >= a_iGuildID ) return eTNRes_InvalidGuild ;
	if( 0 >= a_iPC || MAX_USER <= a_iPC ) return eTNRes_Failed ;
	if( 0 < m_krgEntry[a_iClanSlot][a_iExpandSlot].iID ) return eTNRes_NotEmpty; // 이미 slot이 등록되어 있다.

	if( 0 < a_iExpandSlot ) // assist로 등록 신청을 한다면, 맹주가 이미 등록되어 있는지 확인해야한다.
	{
		if( 0 == m_krgEntry[a_iClanSlot][0].iID )
		{// 메시지 출력, _NoClanLeader
			SendClientMessage( a_iPC, g_pMessageStringTable[_NoClanLeader] ) ;
			return eTNRes_Failed;
		}
	}

	int iRes = SearchEntry( a_iGuildID ) ; // entry에서 해당 길드가 이미 등록되어 있는지 찾는다.
	if( -1 < iRes ) return eTNRes_AlreadyRegisteredInSiegeEntry ; // 이미 등록되어 있다.

	int iGuildIndex = GetGuild( a_iGuildID, FLAG_OPEN ) ; // guild 정보를 알아온다.
	if( 0 > iGuildIndex || MAX_USER <= iGuildIndex )
	{
		return eTNRes_InvalidGuild ;
	}

	m_krgEntry[a_iClanSlot][a_iExpandSlot].iID = a_iGuildID ;
	strcpy( m_krgEntry[a_iClanSlot][a_iExpandSlot].szName, pGuild[iGuildIndex].GUILD.GuildName ) ;
	m_krgEntry[a_iClanSlot][a_iExpandSlot].dwMark = pGuild[iGuildIndex].GUILD.Mark;

	#ifdef __TN_TOP_LOG__
	{
		//SYSTEMTIME st ;
		//GetLocalTime( &st ) ;
		char chBuf[512] = { 0,0,0, } ;
		sprintf(chBuf, "[%dMM%dDD %2dH%2dM%2dS] RegisterEntry() > %s applies to the siege. guild:%d, ClanSlot:%d, ExpandSlot:%d, Name:%s, Mark:%d \r\n"
			, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
			, pMob[a_iPC].MOB.szName
			, a_iGuildID, a_iClanSlot, a_iExpandSlot
			, m_krgEntry[a_iClanSlot][a_iExpandSlot].szName
			, m_krgEntry[a_iClanSlot][a_iExpandSlot].dwMark
			) ;
		WriteLog( chBuf, g_szrgLogFileName[eLogFileName_Castle] ) ;
	}
	#endif  

	if( 0 == a_iExpandSlot ) // 맹주이라면, ...
	{// 맹주 신청을 했다고 공지를 해줘야 한다.
		int iTxtIndex = _Post1stAllyForSiege ;
		if( 1 == a_iClanSlot ) iTxtIndex = _Post1stAllyForSiege ;
		else if( 2 == a_iClanSlot ) iTxtIndex = _Post2ndAllyForSiege ;
		else if( 3 == a_iClanSlot ) iTxtIndex = _Post3rdAllyForSiege ;

		char szNotify[1024] = { 0,0,0, } ;
		sprintf( szNotify, g_pMessageStringTable[iTxtIndex], m_krgEntry[a_iClanSlot][a_iExpandSlot].szName ) ;
		PostMessageToWorld( szNotify, eTNClr_White, eTNClr_BG, iTxtIndex ) ;
	}

	SaveData() ; // 등록 신청한 것을 기록한다.
	return eTNRes_Succeeded ;
}


//@Return
//	-1 : Failed, 19~23 : entry에 들어 있다.
int TNSiege::SearchEntry( int a_iGuildID )
{
	if( 0 >= a_iGuildID ) return -1;// 입력된 guild ID가 잘못되어 있을 경우

	for( int iClanSlot = 0 ; iClanSlot < eSiege_Army ; ++iClanSlot )
	{
		for( int iExpandSlot = 0 ; iExpandSlot < eSiege_MaxEntry ; ++iExpandSlot )
		{
			if( a_iGuildID == m_krgEntry[iClanSlot][iExpandSlot].iID ) return (iClanSlot + eTNClan_CastleOwner) ;
		}
	}

	return -1 ;
}


// entry에서 등록을 취소한다. 공성 탈퇴
int TNSiege::GiveUpSiege( int a_iGuildID )
{
	for( int iClanSlot = 0 ; iClanSlot < eSiege_Army ; ++iClanSlot )
	{
		for( int iExpandSlot = 0 ; iExpandSlot < eSiege_MaxEntry ; ++iExpandSlot )
		{
			if( a_iGuildID == m_krgEntry[iClanSlot][iExpandSlot].iID ) return eTNRes_Succeeded;
		}
	}

	return eTNRes_Failed ;
}



// entry에서 등록을 취소한다. 공성 탈퇴
int TNSiege::RemoveEntry( int a_iGuildID )
{
	for( int iClanSlot = 0 ; iClanSlot < eSiege_Army ; ++iClanSlot )
	{
		for( int iExpandSlot = 0 ; iExpandSlot < eSiege_MaxEntry ; ++iExpandSlot )
		{
			if( a_iGuildID == m_krgEntry[iClanSlot][iExpandSlot].iID )
			{
				memset( &m_krgEntry[iClanSlot][iExpandSlot], 0, sizeof(m_krgEntry[iClanSlot][iExpandSlot]) );
				return eTNRes_Succeeded;
			}
		}
	}

	return eTNRes_Failed ;
}


// 공성전에 사용되는 상징물들을 배치한다.
void TNSiege::InstallSymbols()
{
	#ifdef __TN_TOP_LOG__
	{
		//SYSTEMTIME st ;
		//GetLocalTime( &st ) ;
		char chBuf[512] = { 0,0,0, } ;
		sprintf(chBuf, "[%dMM%dDD %2dH%2dM%2dS] InstallSymbols() > \r\n"
			, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
			) ;
		WriteLog( chBuf, g_szrgLogFileName[eLogFileName_Castle] ) ;
	}
	#endif  

	for( int i = 0 ; i < eSiege_SymbolCount ; ++ i )
	{
		m_irgSymbol[i][0] = pMob[1000].Summon( eSmblTrb_CastleOwner, 1, eTNPrdt_PopRaise, eTNCls_Warrior, 0, eTNClan_CastleOwner, 0, g_srgSymbolForSiege[i][0],  g_srgSymbolForSiege[i][1], 0, false, 0, MAX_USER+i, 710 ) ;
		m_irgSymbol[i][1] = eTNClan_CastleOwner ;

		#ifdef __TN_TOP_LOG__
		{
			char chBuf[512] = { 0,0,0, } ;
			sprintf(chBuf, "\tHndl:%d, Clan:%d, (%d,%d)\r\n"
				, m_irgSymbol[i][0], m_irgSymbol[i][1], g_srgSymbolForSiege[i][0],  g_srgSymbolForSiege[i][1]
				) ;
			WriteLog( chBuf, g_szrgLogFileName[eLogFileName_Castle] ) ;
		}
		#endif  
	}
}

void TNSiege::DestroySymbols()
{
	for( int i = 0 ; i < eSiege_SymbolCount ; ++ i )
	{
		KillMonster( m_irgSymbol[i][0] ) ;
		m_irgSymbol[i][1] = 0 ;
	}
}


// 32초 주기마다 symbol들을 검사한다.
void TNSiege::CheckSymbols()
{
	if( !m_iStarted ) return ;

	for( int i = 0 ; i < eSiege_SymbolCount ; ++ i )
	{
		int iMob = m_irgSymbol[i][0] ;
		if( MAX_USER > iMob || MAX_MOB <= iMob )
		{
			// 에러 로그를 남겨야 한다. 심각한 것이다.
			continue ;
		}

		if( pMob[iMob].IsDead() )
		{
			if( pMob[iMob].IsWaitToPop() ) continue ;
			// 죽어 있고 대기 시간이 완료됐다. 따라서 pop 시켜줘야 한다.
			int iTribe = eSmblTrb_CastleOwner ;
			switch( m_irgSymbol[i][1] )
			{
			case eTNClan_Siege1 :
				iTribe = eSmblTrb_Symbol1 ;
				break ;
			case eTNClan_Siege2 :
				iTribe = eSmblTrb_Symbol2 ;
				break ;
			case eTNClan_Siege3 :
				iTribe = eSmblTrb_Symbol3 ;
				break ;
			case eTNClan_Siege4 :
				iTribe = eSmblTrb_Symbol4 ;
				break ;
			default :
				iTribe = eSmblTrb_CastleOwner ;
				break ; 
			}

			int iSymbol = pMob[1000].Summon( iTribe, 1, eTNPrdt_PopRaise, eTNCls_Warrior, 0, m_irgSymbol[i][1], 0, g_srgSymbolForSiege[i][0],  g_srgSymbolForSiege[i][1], 0, false, 0, iMob, 730 ) ;

			#ifdef __TN_TOP_LOG__
			{
				//SYSTEMTIME st ;
				//GetLocalTime( &st ) ;
				char chBuf[512] = { 0,0,0, } ;
				sprintf(chBuf, "[%dMM%dDD %2dH%2dM%2dS] CheckSymbols() > %dth symbol(%d) is poped at (%d,%d). Tribe:%d, Clan:%d, (symbol:%d) \r\n"
					, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
					, i, iMob, pMob[iMob].TargetX, pMob[iMob].TargetY
					, iTribe, pMob[iMob].m_byClan
					, iSymbol
					) ;
				WriteLog( chBuf, g_szrgLogFileName[eLogFileName_Castle] ) ;
			}
			#endif 
		}
	}
}



// symbol이 onkilled()시에 연결된 event에 의해서 call 될 것이다.
// 이것에 의해서 clan이 기록되고, 정기적인 check시에 확인되어서 관련된 clan의 symbol을 pop하게 될 것이다.
int TNSiege::CaptureSymbol( int a_iClan, int a_iSymbol, int a_iCapturer/*npc일수도 있다.*/ )
{
	if( eTNClan_CastleOwner > a_iClan || eTNClan_Siege4 < a_iClan ) return eTNRes_Failed ;
	if( MAX_USER > a_iSymbol || MAX_MOB <= a_iSymbol ) return eTNRes_InvalidHandle ;
	if( 0 >= a_iCapturer || MAX_MOB <= a_iCapturer ) return eTNRes_InvalidHandle ;

	//SYSTEMTIME st ;
	//GetLocalTime( &st ) ;

	for( int i = 0 ; i < eSiege_SymbolCount ; ++i )
	{
		if( a_iSymbol == m_irgSymbol[i][0] )
		{
			m_irgSymbol[i][1] = a_iClan ;

			#ifdef __TN_TOP_LOG__
			{
				char chBuf[512] = { 0,0,0, } ;
				sprintf(chBuf, "[%dMM%dDD %2dH%2dM%2dS] CaptureSymbol() > %dth symbol(%d, L(%d,%d)) is captured by Mob(%d/%s). capture's clan:%d \r\n"
					, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
					, i, m_irgSymbol[i][0]
					, pMob[m_irgSymbol[i][0]].TargetX, pMob[m_irgSymbol[i][0]].TargetY
					, a_iCapturer, pMob[a_iCapturer].MOB.szName
					, m_irgSymbol[i][1]
					) ;
				WriteLog( chBuf, g_szrgLogFileName[eLogFileName_Castle] ) ;
			}
			#endif 

			int iTxtIndex = _PostTheStateOfCapturingTheSymbol1 ;
			if( 3 == g_srgSymbolForSiege[i][2] ) iTxtIndex = _PostTheStateOfCapturingTheSymbol3 ;
			else if( 2 == g_srgSymbolForSiege[i][2] ) iTxtIndex = _PostTheStateOfCapturingTheSymbol2 ;

			char* szGuildName = NULL ;
			//int iClanLeader = 0 ;
			if( eTNClan_CastleOwner == a_iClan ) szGuildName = (char*)m_krgEntry[0][0].szName ; //iClanLeader = 1 ;
			else if( eTNClan_Siege1 == a_iClan ) szGuildName = (char*)m_krgEntry[1][0].szName ; //iClanLeader = 1 ;
			else if( eTNClan_Siege2 == a_iClan ) szGuildName = (char*)m_krgEntry[2][0].szName ; //iClanLeader = 2 ;
			else if( eTNClan_Siege3 == a_iClan ) szGuildName = (char*)m_krgEntry[3][0].szName ; //iClanLeader = 3 ;
			else if( eTNClan_Siege4 == a_iClan ) szGuildName = (char*)g_pMessageStringTable[_Independent] ; //iClanLeader = 4 ;

			char szMsg[1024] = { 0,0,0, } ;
			sprintf( szMsg, g_pMessageStringTable[iTxtIndex], szGuildName, g_srgSymbolForSiege[i][0], g_srgSymbolForSiege[i][1] ) ;
			PostMessageToZone( szMsg ) ;

			return eTNRes_Succeeded ;
		}
	}

	// 로그를 남겨야 한다.
	// a_iSymbol을 찾을 수 없다는 것이다.

	return eTNRes_Failed ;
}


// symbol 마다의 점수를 합산하여 공성전 최종 승자를 결정한다.
void TNSiege::JudgeSiege()
{
	int irgClan[eTNClan_Reserved] = { 0,0,0, } ;
	//SYSTEMTIME st ;
	//GetLocalTime( &st ) ;
	#ifdef __TN_TOP_LOG__
	{
		char chBuf[512] = { 0,0,0, } ;
		sprintf(chBuf, "[%dMM%dDD %2dH%2dM%2dS] JudgeSiege() > \r\n"
			, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
			) ;
		WriteLog( chBuf, g_szrgLogFileName[eLogFileName_Castle] ) ;
	}
	#endif 

	for( int i = 0 ; i < eSiege_SymbolCount ; ++i )
	{
		int iClan = m_irgSymbol[i][1] ;
		if( eTNClan_CastleOwner > iClan || eTNClan_Reserved <= iClan )
		{
			// 로그를 남긴다.
			continue ;
		}

		// 누가 어떻게 몇 점을 획득했는지 로그를 남긴다.
		irgClan[iClan] += g_srgSymbolForSiege[i][2] ;

		#ifdef __TN_TOP_LOG__
		{
			char chBuf[512] = { 0,0,0, } ;
			sprintf(chBuf, "\tclan %d: %d\r\n", iClan, irgClan[iClan] ) ;
			WriteLog( chBuf, g_szrgLogFileName[eLogFileName_Castle] ) ;
		}
		#endif 
	}

	// 각 symbol의 소유주, 각 clan의 총 합계
	m_iWinner = eTNClan_CastleOwner ;
	int iHighestPoint = 0 ;
	for( int iClan = eTNClan_CastleOwner ; iClan <= eTNClan_Siege4 ; ++iClan )
	{
		if( iHighestPoint < irgClan[iClan] )
		{
			iHighestPoint = irgClan[iClan] ;
			m_iWinner = iClan ;
		}
		// 동점자가 나왔을 경우에 대한 추가 처리가 필요하다.
	}

	char szMsg[1024] = { 0,0,0, } ;	
	sprintf( szMsg, g_pMessageStringTable[_PostTheResultOfTheSiege], irgClan[eTNClan_CastleOwner], irgClan[eTNClan_Siege1], irgClan[eTNClan_Siege2], irgClan[eTNClan_Siege3], irgClan[eTNClan_Siege4] ) ;
	PostMessageToWorld( szMsg ) ;

	#ifdef __TN_TOP_LOG__
	{
		char chBuf[512] = { 0,0,0, } ;
		sprintf(chBuf, "\r\nWinner > clan:(%d,%d), CastleOwner:%d, Siege1:%d, Siege2:%d, Siege3:%d, Siege4:%d  \r\n"
			, m_iWinner, iHighestPoint
			, irgClan[eTNClan_CastleOwner], irgClan[eTNClan_Siege1], irgClan[eTNClan_Siege2], irgClan[eTNClan_Siege3], irgClan[eTNClan_Siege4]
			) ;
		WriteLog( chBuf, g_szrgLogFileName[eLogFileName_Castle] ) ;
	}
	#endif 

	char szPreviousOwner[GUILDNAME_LENGTH] = { 0,0,0, } ;
	strcpy( szPreviousOwner, m_kOwner.kGuild.szName ) ;

	m_iDateToHold = eDate_NotSelected ;
	int iWinnerGuild = m_kOwner.kGuild.iID ;
	if( eTNClan_Siege4 == m_iWinner ) // 무적의 공성 연합이 이긴 경우, 성은 무소유 상태로 된다.
	{
		iWinnerGuild = 0;
		m_iDateToHold = eDate_Saturday10HH ;
		PostMessageToWorld( g_pMessageStringTable[_PostTheCastleIsFree] ) ;
	}
	else if( eTNClan_CastleOwner == m_iWinner )
	{
		char szMsg[1024] = { 0,0,0, } ;	
		sprintf( szMsg, g_pMessageStringTable[_PostSuccessToDefense], szPreviousOwner ) ;
		PostMessageToWorld( szMsg ) ;
	}
	else
	{
		if( eTNClan_Siege1 == m_iWinner ) iWinnerGuild = m_krgEntry[1][0].iID ;
		else if( eTNClan_Siege2 == m_iWinner ) iWinnerGuild = m_krgEntry[2][0].iID ;
		else if( eTNClan_Siege3 == m_iWinner ) iWinnerGuild = m_krgEntry[3][0].iID ;

		int iGuildIndex = GetGuild( iWinnerGuild, FLAG_OPEN ) ;
		if( 0 < iGuildIndex || MAX_USER > iGuildIndex )
		{
			char szMsg[1024] = { 0,0,0, } ;	
			sprintf( szMsg, g_pMessageStringTable[_PostTheWinnerOfTheSiege], szPreviousOwner, pGuild[iGuildIndex].GUILD.GuildName ) ;
			PostMessageToWorld( szMsg ) ;			
		}
		else
		{
			// 로그를 남긴다.
			#ifdef __TN_TOP_LOG__
			{
				char chBuf[512] = { 0,0,0, } ;
				sprintf(chBuf, "\r\nERROR >> 공성측이 이겼는데, 공성측 길드ID(%d)가 invalid하다.\r\n", iWinnerGuild ) ;
				WriteLog( chBuf, g_szrgLogFileName[eLogFileName_Castle] ) ;
			}
			#endif 
			iWinnerGuild = 0 ;
			m_iDateToHold = eDate_Saturday10HH ;
			PostMessageToWorld( g_pMessageStringTable[_PostTheCastleIsFree] ) ;
		}
	}

	ChangeOwner( iWinnerGuild ) ;
	
	InitEntry() ;// m_kOwner는 초기화하지 않고 entry 만을 초기화한다.
	RegisterCastleOwner();// m_kOwner 정보를 이용하여 수성맹주 설정을 해준다.
	SaveData() ; // 변경된 entry를 저장한다.
}


int TNSiege::ChangeOwner( int a_iGuildID )
{
	//SYSTEMTIME st ;
	//GetLocalTime( &st ) ;
	CTime kTime( g_kSystemTime ) ;

	memset( &m_kOwner, 0, sizeof(m_kOwner) ) ;

	m_kOwner.kGuild.iID = a_iGuildID ;
	m_kOwner.kTimeOccupied = kTime ;
	g_iCastleOwner = a_iGuildID;

	#ifdef __TN_TOP_LOG__
	{
		char chBuf[512] = { 0,0,0, } ;
		sprintf(chBuf, "\r\n\r\n[UpdateCastle] [%dYY%dMM%dDD %2dH%2dM%2dS] > The castle is occupied by a guild(%d) at %dYY%dMM%dDD %dHH%dMI \r\n"
			, g_kSystemTime.wYear, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
			, m_kOwner.kGuild.iID
			, m_kOwner.kTimeOccupied.GetYear(), m_kOwner.kTimeOccupied.GetMonth()
			, m_kOwner.kTimeOccupied.GetDay(), m_kOwner.kTimeOccupied.GetHour()
			, m_kOwner.kTimeOccupied.GetMinute()
			) ; 
		WriteLog( chBuf, g_szrgLogFileName[eLogFileName_Castle] ) ;
	}
	#endif

	int iGuildIndex = GetGuild( a_iGuildID, FLAG_OPEN ) ;
	if( 0 > iGuildIndex || MAX_USER <= iGuildIndex )
	{
		return eTNRes_Failed ;
	}

	m_kOwner.kGuild.dwMark = pGuild[iGuildIndex].GUILD.Mark; //수성맹주 길드 문장
	strcpy( m_kOwner.kGuild.szName, pGuild[iGuildIndex].GUILD.GuildName ) ;// 수성맹주 길드명을 기록
	m_kOwner.iOwnerFriend = GetGuildID( pGuild[iGuildIndex].GUILD.AlliedGuildName1 ) ; // 수성맹주 연합길드 ID 확인
	

	//strcpy( m_sgrgLeader[0], m_kOwner.szGuildName ) ;
	//strcpy( m_sgrgLeader[4], g_pMessageStringTable[_Independent] ) ;

	{
		char chBuf[512] = { 0,0,0, } ;
		sprintf(chBuf, "\t Information about Guild > ID:%d, Name:%s, Mark:%d, Ally:%d \r\n\r\n"
			, m_kOwner.kGuild.iID, m_kOwner.kGuild.szName, m_kOwner.kGuild.dwMark, m_kOwner.iOwnerFriend
			) ; 
		WriteLog( chBuf, g_szrgLogFileName[eLogFileName_Castle] ) ;
	}

	return eTNRes_Succeeded ;
}


void TNSiege::SelectDate( int a_iDateNum, bool a_bCheck )
{
	if( 0 >= a_iDateNum || 5 < a_iDateNum ) return ;
	if( a_bCheck && (0 < m_iDateToHold) ) return ; //  1회 결정을 하면 변경할 수 없다.

	m_iDateToHold = a_iDateNum ;
	SaveData() ;

	//SYSTEMTIME st ;
	//GetLocalTime( &st ) ;
	#ifdef __TN_TOP_LOG__
	{
		char chBuf[512] = { 0,0,0, } ;
		sprintf(chBuf, "\r\n\r\n[SelectDate] [%dYY%dMM%dDD %2dHH%2dMI%2dSS] > Date:%d(1:friday 8hh, 2:friday 10hh, 3:saturday 8hh, 4:saturday 10hh, 5:sunday 8hh)\r\n"
			, g_kSystemTime.wYear, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
			, m_iDateToHold
			) ; 
		WriteLog( chBuf, g_szrgLogFileName[eLogFileName_Castle] ) ;
	}
	#endif

	TriggerEvent( 0, 98/*공성 날짜 공지 이벤트*/, 0, 0, 0, 1231 ) ;
}


int TNSiege::LoadData()
{
	if( eZone_Castle != g_iZoneID ) return eTNRes_Failed ;

	char szData[64] = { 0,0,0, } ;
	char szCommand[32] = { 0,0,0, } ;
    char szArgument1[32] = { 0,0,0, } ;
    char szArgument2[32] = { 0,0,0, } ;
	char szArgument3[32] = { 0,0,0, } ;
	char szArgument4[32] = { 0,0,0, } ;
	char szArgument5[32] = { 0,0,0, } ;
	char szArgument6[32] = { 0,0,0, } ;
	char szArgument7[32] = { 0,0,0, } ;
    int iArgument1 = 0, iArgument2 = 0, iArgument3 = 0, iArgument4 = 0, iArgument5 = 0 ;
	
	FILE* fin ;
	fin = fopen( ".\\Data\\Castle.txt", "rt" ) ;
	if( NULL == fin ) 
	{ 
		MessageBox( hWndMain, "Not found Castle.txt in data folder", "Failed to initialize data about the castle.", NULL ) ; 
		return eTNRes_Failed ; 
	}

	//SYSTEMTIME st ;
	//GetLocalTime( &st ) ;
	int iYear, iMonth, iDay, iHour, iMinute ;
	bool bDateTimePatched = true ;
	bool bLoaded = false ;

	while( true )
	{
		char* ret = fgets( (char*)szData, 64, fin ) ;
		if( NULL == ret ) break ;

		iYear = iMonth = iDay = iHour = iMinute = 0 ;
		sscanf( szData, "%s %s %s %s %s %s %s %s", szCommand, szArgument1, szArgument2, szArgument3, szArgument4, szArgument5, szArgument6, szArgument7 ) ;
	
		switch( szData[1] )
		{
		case 'C' : // owner of the castle
			{
				iArgument1 = atoi( szArgument1 ) ; // owner #
				iYear = atoi( szArgument2 ) ; // date occupied
				iMonth = atoi( szArgument3 ) ;
				iDay = atoi( szArgument4 ) ;
				iHour = atoi( szArgument5 ) ;
				iMinute = atoi( szArgument6 ) ;

				bLoaded = true ;
				m_kOwner.kGuild.iID = iArgument1 ;
				g_iCastleOwner = iArgument1 ;

				if( (0 == iYear) && (0 == iMonth) && (0 == iDay) )
				{
					bDateTimePatched = false ;
					CTime kTime( g_kSystemTime ) ;
					m_kOwner.kTimeOccupied = kTime ;
				}
				else
				{
					CTime kTime( iYear, iMonth, iDay, iHour, iMinute, 0 ) ;
					m_kOwner.kTimeOccupied = kTime ;
				}

				{
					char chBuf[512] = { 0,0,0, } ;
					sprintf(chBuf, "\r\n\r\n[LoadCastleData] [%dYY%dMM%dDD %2dH%2dM%2dS] > %The castle is occupied by a guild(%d) in %d-%d-%d-%d-%d\r\n"
						, g_kSystemTime.wYear, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
						, iArgument1
						, iYear, iMonth, iDay, iHour, iMinute
						) ; 
					WriteLog( chBuf, g_szrgLogFileName[eLogFileName_Castle] ) ;
				}

				CGuild guild;
				ReadGuildFile(m_kOwner.kGuild.iID, &guild);
				strncpy( m_kOwner.kGuild.szName, guild.GUILD.GuildName, SZGUILD_LENGTH);
				m_kOwner.iOwnerFriend = GetGuildID(guild.GUILD.AlliedGuildName1);
				m_kOwner.kGuild.dwMark = guild.GUILD.Mark;

				m_krgEntry[0][0] = m_kOwner.kGuild ;

				//strcpy( m_sgrgLeader[0], m_kOwner.szGuildName ) ;
				//strcpy( m_sgrgLeader[4], g_pMessageStringTable[_Independent] ) ;
			}
			break ;
		case 'D' : //date
			{
				iArgument1 = atoi( szArgument1 ) ; // #Date to hold the siege.
				m_iDateToHold = iArgument1 ; // 개최될 날짜
				if( eDate_NotSelected > m_iDateToHold || eDate_Sunday8HH < m_iDateToHold ) m_iDateToHold = eDate_NotSelected ;
			}
			break ;
		case 'E' : // entry
			{
				iArgument1 = atoi( szArgument1 ) ; // level
				iArgument2 = atoi( szArgument2 ) ; // 
				iArgument3 = atoi( szArgument3 ) ; // 
				iArgument4 = atoi( szArgument4 ) ;
				iArgument5 = atoi( szArgument5 ) ;

				m_krgEntry[0][iArgument1].iID = iArgument2 ;
				m_krgEntry[1][iArgument1].iID = iArgument3 ;
				m_krgEntry[2][iArgument1].iID = iArgument4 ;
				m_krgEntry[3][iArgument1].iID = iArgument5 ;

			}
			break ;
		}
	}

	fclose( fin ) ;

	RefreshEntry();
	//for( int iClanSlot = 0 ; iClanSlot < eSiege_Army ; ++iClanSlot )
	//{
	//	for( int iExpandSlot = 0 ; iExpandSlot < eSiege_MaxEntry ; ++iExpandSlot )
	//	{
	//		if( 0 >= m_krgEntry[iClanSlot][iExpandSlot].iID ) continue ;
	//		CGuild kGuild ;
	//		ReadGuildFile( m_krgEntry[iClanSlot][iExpandSlot].iID, &kGuild ) ;
	//		strncpy( m_krgEntry[iClanSlot][iExpandSlot].szName, kGuild.GUILD.GuildName, SZGUILD_LENGTH ) ;
	//		m_krgEntry[iClanSlot][iExpandSlot].dwMark = kGuild.GUILD.Mark ;
	//	}
	//}

	if( (false == bDateTimePatched) || (false == bLoaded) ) SaveData() ;

	return eTNRes_Succeeded ;
}


int TNSiege::SaveData( char* a_pFileName )
{
	HANDLE hFile ;
	//hFile = CreateFile( ".\\Data\\Castle.txt",					  // file name
	hFile = CreateFile( a_pFileName,					  // file name
                                GENERIC_WRITE,   // open for writing 
                                FILE_SHARE_WRITE,             // do not share 
                                NULL,                         // no security 
                                CREATE_ALWAYS,                  // open or create 
                                FILE_ATTRIBUTE_NORMAL,        // normal file 
                                NULL ) ;

	if(hFile == INVALID_HANDLE_VALUE)
	{
		return eTNRes_Failed ;
	}

	DWORD dwRetWrite = 0 ;
	int nBufLength = 0 ;	
	char szData[256] = { 0,0,0, } ;

	memset( szData, 0, sizeof(szData) ) ;
	sprintf( szData, "/C %d %d %d %d %d %d\r\n/D %d\r\n"
		, m_kOwner.kGuild.iID
		, m_kOwner.kTimeOccupied.GetYear(), m_kOwner.kTimeOccupied.GetMonth()
		, m_kOwner.kTimeOccupied.GetDay(), m_kOwner.kTimeOccupied.GetHour()
		, m_kOwner.kTimeOccupied.GetMinute()
		, m_iDateToHold
		) ;
	nBufLength = strlen( szData ) ;
	WriteFile( hFile, szData, nBufLength, &dwRetWrite, NULL ) ;

	for( int i = 0 ; i < eSiege_MaxEntry ; ++i )
	{
		memset( szData, 0, sizeof(szData) ) ;
		sprintf( szData, "/E %2d: %4d %4d %4d %4d\r\n"
			, i, m_krgEntry[0][i].iID, m_krgEntry[1][i].iID, m_krgEntry[2][i].iID, m_krgEntry[3][i].iID
			) ;
		nBufLength = strlen( szData ) ;
		WriteFile( hFile, szData, nBufLength, &dwRetWrite, NULL ) ;
	}

	CloseHandle( hFile ) ;

	return eTNRes_Succeeded ;
}


void TNSiege::get_Symbols( int* a_irgSymbol )
{
	for( int i = 0 ; i < eSiege_SymbolCount ; ++i )
        a_irgSymbol[i] = m_irgSymbol[i][1] ;
}

void TNSiege::Print()
{
	#ifdef __TN_TOP_LOG__
	{
		//SYSTEMTIME st ;
		//GetLocalTime( &st ) ;
		char chBuf[512] = { 0,0,0, } ;
		sprintf(chBuf, "\r\n[%dMM%dDD %2dH%2dM%2dS] Print() > \r\n"
			, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
			) ;
		WriteLog( chBuf, g_szrgLogFileName[eLogFileName_Castle] ) ;
	}
	#endif  

	for( int i = 0 ; i < eSiege_SymbolCount ; ++i )
	{
		int iMob = m_irgSymbol[i][0] ;
		if( MAX_USER > iMob || MAX_MOB <= iMob ) continue ;
		#ifdef __TN_TOP_LOG__
		{
			char chBuf[512] = { 0,0,0, } ;
			sprintf(chBuf, "\t%d. Hndl:%d, Clan:%d, L(%d,%d), Mode:%d, FSM:%d\r\n"
				, i, m_irgSymbol[i][0], m_irgSymbol[i][1], pMob[iMob].TargetX, pMob[iMob].TargetY
				, pMob[iMob].Mode, pMob[iMob].m_eFSM
				) ;
			WriteLog( chBuf, g_szrgLogFileName[eLogFileName_Castle] ) ;
		}
		#endif  
	}
}



int TNSiege::GetEntry( int a_iClanSlot, int a_iExpandSlot, TNGUILD_INFO& a_kGuild  )
{
	if( 0 > a_iClanSlot || eSiege_Army <= a_iClanSlot ) return eTNRes_Failed ;
	if( 0 > a_iExpandSlot || eSiege_MaxEntry <= a_iExpandSlot ) return eTNRes_Failed ;

	a_kGuild = m_krgEntry[a_iClanSlot][a_iExpandSlot] ;
/*
	int iGuildIndex = GetGuild( a_iGuildID, FLAG_OPEN ) ;
	if( 0 > iGuildIndex || MAX_USER <= iGuildIndex )
	{
		#ifdef __TN_TOP_LOG__
		{
			SYSTEMTIME st ;
			GetLocalTime( &st ) ;
			char chBuf[512] = { 0,0,0, } ;
			sprintf(chBuf, "[%dMM%dDD %2dH%2dM%2dS] GetEntry() > Invalid Guild Index(%d,ID:%d), ClanSlot:%d, ExpandSlot:%d \r\n"
				, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond, iGuildIndex, a_iGuildID, a_iClanSlot, a_iExpandSlot
				) ;
			WriteLog( chBuf, g_szrgLogFileName[eLogFileName_Castle] ) ;
		}
		#endif 
		return eTNRes_InvalidGuild ;
	}

	strcpy( a_szGuildName, pGuild[iGuildIndex].GUILD.GuildName ) ;
	a_dwGuildMark = pGuild[iGuildIndex].GUILD.Mark;
*/
	return eTNRes_Succeeded ;
}


int TNSiege::FindFreeSlot( int a_iClanSlot )
{
	for( int i = 0 ; i < eSiege_MaxEntry ; i += 2 )
	{
		if( 0 == m_krgEntry[a_iClanSlot][i].iID ) return i ;
	}
	
	return -1 ;
}


void TNSiege::RefreshEntry()
{
	for( int iClanSlot = 0 ; iClanSlot < eSiege_Army ; ++iClanSlot )
	{
		for( int iExpandSlot = 0 ; iExpandSlot < eSiege_MaxEntry ; ++iExpandSlot )
		{
			if( 0 >= m_krgEntry[iClanSlot][iExpandSlot].iID ) continue ;
			CGuild kGuild ;
			ReadGuildFile( m_krgEntry[iClanSlot][iExpandSlot].iID, &kGuild ) ;
			strncpy( m_krgEntry[iClanSlot][iExpandSlot].szName, kGuild.GUILD.GuildName, SZGUILD_LENGTH ) ;
			m_krgEntry[iClanSlot][iExpandSlot].dwMark = kGuild.GUILD.Mark ;
		}
	}
}