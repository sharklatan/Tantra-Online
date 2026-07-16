#include "stdafx.h"
#include "TNDebug.h"
#include "TNKalaSystem.h"
#include "Basedef.h"
#include "CItem.h"
#include "CMob.h"
#include "Server.h"
#include "GetFunc.h"
#include "SendFunc.h"
#include "Language.h"
#include "CUser.h"

#if defined(__ZONE_SERVER__) && defined(__MEMORYMANAGER__)

#ifndef _HTMEMORYMANAGER_H_
#include "HTMemoryManager.h"
#endif

#endif //__ZONE_SERVER__, __MEMORYMANAGER__

extern CUser					pUser    [MAX_USER];
extern CMob						pMob     [MAX_MOB];
extern CItem					pItem    [MAX_ITEM];
extern TNITEM_DATA				pItemData[MAX_ITEM_DATA];
extern STRUCT_MOB				pMonsterData[MAX_MONSTER_DATA] ;
extern unsigned short			pItemGrid  [MAX_GRIDY][MAX_GRIDX];
//extern int						g_irgKalaAltar[eKalaAltar_MaxCount]  ;
extern TNKALA_ALTAR_OLD            g_krgKalaAltar[eKalaAltar_MaxCount] ;
extern unsigned short         pMobGrid   [MAX_GRIDY][MAX_GRIDX];

TNKalaSystem g_kKalaSystem ;
extern unsigned int           CurrentTime ;

TNKalaSystem::TNKalaSystem()
{
	Init() ;
}

TNKalaSystem::~TNKalaSystem()
{
}


void TNKalaSystem::Init()
{
	memset( m_krgKala, 0, sizeof(m_krgKala) ) ;
	m_iKalaOnAltar = 0 ;
	m_iKalaCoreInPC = 0 ;
	m_iKalaCoreOnGround = 0 ;
} 


// altar에 위치하고 있는 kala를 확인
int TNKalaSystem::CheckKalaOnAltar()
{
	for( int iKala = 1000 ; iKala < 1009 ; ++ iKala )
	{
		{
			char chBuf3[256] = { 0,0,0, } ;
			sprintf(chBuf3, "\t- Kala[%d]: FSM:%d, Mode:%d, HP:%d, \r\n"					
				, iKala, pMob[iKala].m_eFSM, pMob[iKala].Mode, pMob[iKala].MOB.nHP
				) ; 
			WriteLog( chBuf3, g_szrgLogFileName[eLogFileName_KalaSystem] ) ;
		}

		bool bFound = false ;
		for( int i = 0 ; i < eKalaAltar_MaxCount ; ++i )
		{
			if( m_krgAltar[i].iKala == iKala ) bFound = true ;
		}

		if( !bFound ) // 제단에 없는 kala instance라면, 확인 사살
		{
			KillMonster( iKala ) ;
		}
	}

	return eTNRes_Succeeded ;
}



// kala 의 전체 개수를 파악하고 현존하는 총 개수가 틀린 경우, 이를 보정해줘야 한다.
int TNKalaSystem::CheckCountNRecovery()
{
	int irgKalaCount[eRealm_MaxCount] = { 0,0,0, } ;
	// 1. slot 정보를 이용해서 9개의 정보가 모두 valid한지 검사
	// 2. 틀린 정보를 발견하면, slot을 clear하고 신규 kala를 생성시켜줘야 한다.
	//	 - 어떤 주신의 kala인지를 먼저 알아야 한다. 그러기 위해서는 한번 돌려서 전체적인 개수를 파악해야 한다.
	//	 - 모자른 개수의 kala-core를 맵 중앙에 pop 시켜준다.

	m_iKalaOnAltar = 0 ;
	m_iKalaCoreInPC = 0 ;
	m_iKalaCoreOnGround = 0 ;

	//SYSTEMTIME st ;
	//GetLocalTime( &st ) ;

	{
		char chBuf[256] = { 0,0,0, } ;
		sprintf(chBuf, "\r\n\r\n[%dmm%ddd %dhh%dms%dss] CheckCountNRecovery() > \r\n"
			, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
			) ; 
		WriteLog( chBuf, g_szrgLogFileName[eLogFileName_KalaSystem] ) ;

		for( int i = 0 ; i < eKalaAltar_MaxCount ; ++i )
		{
			{
				char chBuf2[256] = { 0,0,0, } ;
				sprintf(chBuf2, "\t- Altar[%d]: %d\r\n"					
					, i, m_krgAltar[i].iKala
					) ; 
				WriteLog( chBuf2, g_szrgLogFileName[eLogFileName_KalaSystem] ) ;
			}
		}
	}

	CheckKalaOnAltar() ;

	for( int i = 0 ; i < eKala_MaxCount ; ++i )
	{// 각각 검사를 해서 slot 내의 정보가 valid한지를 검사해야하고 invalid할 경우 slot을 비우는 처리를 해줘야 한다.
		{
			char chBuf2[512] = { 0,0,0, } ;
			sprintf( chBuf2, "\r\n\t- %d. where:%d, altar:%d, monsterhandle:%d, coord(%d,%d), porter:%d(%u)"
				, i, m_krgKala[i].iWhere, m_krgKala[i].iAltar, m_krgKala[i].iMonsterHandle, m_krgKala[i].iX, m_krgKala[i].iY, m_krgKala[i].iPCHandle, m_krgKala[i].uiTimePossessed
				) ; 
			WriteLog( chBuf2, g_szrgLogFileName[eLogFileName_KalaSystem] ) ;
		}

		//int a_iSrcWhere, int a_iSrcAltar, int a_iSrcMonsterHandle, int a_iSrcX, int a_iSrcY, int a_iSrcPCHandle, int a_iDestWher

		switch( m_krgKala[i].iWhere )
		{
		case eLoc_Altar :
			{
				int iKalaHandle = m_krgAltar[m_krgKala[i].iAltar].iKala ;
				if( m_krgKala[i].iMonsterHandle == iKalaHandle )
				{
					if( MAX_USER > iKalaHandle || MAX_MOB <= iKalaHandle )
					{
						ClearKalaLoc( i ) ;
						break ;
					}
					
					if( pMob[iKalaHandle].IsDead() )
					{
						ClearKalaLoc( i ) ;
						break ;
					}

					short sTribe = pMob[m_krgKala[i].iMonsterHandle].MOB.snTribe ;
					if( eKala_Brahma == sTribe ) ++irgKalaCount[eTNClan_Brahma] ;
					else if( eKala_Vishnu == sTribe ) ++irgKalaCount[eTNClan_Vishnu] ;
					else if( eKala_Siva == sTribe ) ++irgKalaCount[eTNClan_Siva] ;
					else
					{ // 제단에 있는 monster가 kala가 아니다.
						KillMonster( iKalaHandle ) ;

						{
							char chBuf2[256] = { 0,0,0, } ;
							sprintf( chBuf2, "<<ERROR CheckCountNRecovery() > Invalid Tribe ID (Kala-Handle:%d, Tribe:%d), LocSlot:%d \r\n"
								, iKalaHandle, sTribe, i
								) ; 
							WriteLog( chBuf2, g_szrgLogFileName[eLogFileName_KalaSystem] ) ;
						}
						//Print() ;
						ClearKalaLoc( i ) ;
						break ;
					}

					++m_iKalaOnAltar ;
				}
				else
				{// 동일하지 않다. 이럴 경우는 없을 것이다.
					{
						char chBuf2[256] = { 0,0,0, } ;
						sprintf( chBuf2, "<<ERROR CheckCountNRecovery() > The value of handle is different. (Kala-Handle:%d, Handle at altar:%d), LocSlot:%d \r\n"
							, m_krgKala[i].iMonsterHandle, iKalaHandle, i
							) ; 
						WriteLog( chBuf2, g_szrgLogFileName[eLogFileName_KalaSystem] ) ;
					}
					//Print() ;
					ClearKalaLoc( i ) ;
					break ;
				}	
			}
			break ;
		case eLoc_Ground :
			{
				int iRes = FindKalaCoreOnTheGround( m_krgKala[i].iX, m_krgKala[i].iY ) ;
				if( eTNRes_Failed == iRes )
				{
					{
						char chBuf2[256] = { 0,0,0, } ;
						sprintf( chBuf2, "<<ERROR CheckCountNRecovery() > can't find a kala-core on the ground. LocSlot:%d, coord(%d,%d) \r\n"
							, i, m_krgKala[i].iX, m_krgKala[i].iY
							) ; 
						WriteLog( chBuf2, g_szrgLogFileName[eLogFileName_KalaSystem] ) ;
					}
					//Print() ;
					ClearKalaLoc( i ) ;
					break ;
				}

				//int iSlot = pItemGrid[m_krgKala[i].iY][m_krgKala[i].iX] ;
				//int iID = pItemData[pItem[iSlot].ITEM.snIndex].sID ;

				if( eKalaCore_Brahma == iRes )  ++irgKalaCount[eTNClan_Brahma] ;
				else if( eKalaCore_Vishnu == iRes ) ++irgKalaCount[eTNClan_Vishnu] ;
				else if( eKalaCore_Siva == iRes ) ++irgKalaCount[eTNClan_Siva] ;
				else
				{
					//Print() ;
					ClearKalaLoc( i ) ;
					// ground에서 item을 제거해준다.
					pItemGrid[m_krgKala[i].iY][m_krgKala[i].iX] = 0 ;
					break ;
				}

				++m_iKalaCoreOnGround ;
			}
			break ;
		case eLoc_Inventory :
			{ // pc inventory를 뒤져서 어떤 kala-core인지 알아야 한다.				
				int iCount = 0 ;
				for( int iInvenSlot = 0 ; iInvenSlot < MAX_INVEN ; ++iInvenSlot )
				{
					++iCount ;
					int iDataSlot = pMob[m_krgKala[i].iPCHandle].MOB.Inven[iInvenSlot].snIndex ;
					int iID = pItemData[iDataSlot].sID ;
					if( eKalaCore_Brahma == iID )  ++irgKalaCount[eTNClan_Brahma] ;
					else if( eKalaCore_Vishnu == iID ) ++irgKalaCount[eTNClan_Vishnu] ;
					else if( eKalaCore_Siva == iID ) ++irgKalaCount[eTNClan_Siva] ;
					else --iCount ;
				}

				if( 0 >= iCount )
				{ // inventory에 있어야 하는데 하나도 검색이 되지 않은 경우
					//Print() ;
					ClearKalaLoc( i ) ;
					break ;
				}

				++m_iKalaCoreInPC ;
			}
			break ;
		case eLoc_NoWhere :
			{
			}
			break ;
		default :
			Print() ;
			ClearKalaLoc( i ) ;
			break ;
		} // switch( m_krgKala[i].iWhere )
	} // for( int i = 0 ; i < eKala_MaxCount ; ++i )


	for( int iKala = 1000; iKala < 1009; ++iKala )
	{
		bool bNotFound = true;
		for( int i = 0; i < eKala_MaxCount; ++i )
		{
			if( iKala == m_krgKala[i].iMonsterHandle ) bNotFound = false;
		}

		if( bNotFound )// 없다면, 죽어 있는 것이다.
		{// 확인 사살한다.
			KillMonster( iKala );
		}
	}


	{
		char chBuf2[256] = { 0,0,0, } ;
		sprintf(chBuf2, "\r\n\r\n[%dmm%ddd %dhh%dms%dss] CheckCountNRecovery() > Brahma:%d, Vishnu:%d, Siva:%d\r\n"
			, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
			, irgKalaCount[eTNClan_Brahma], irgKalaCount[eTNClan_Vishnu], irgKalaCount[eTNClan_Siva]
			) ; 
		WriteLog( chBuf2, g_szrgLogFileName[eLogFileName_KalaSystem] ) ;
	}

	// 총 개수가 9개보다 작다면, error, 보정 처리를 해줘야 한다.
	int x, y ;
	if( 3 > irgKalaCount[eTNClan_Brahma] ) 
	{
		int iSlot = FindEmptySlot() ;
		if( 0 <= iSlot && eKala_MaxCount > iSlot )
		{
			x = 532 ;
			y = 472 ;
			STRUCT_ITEM kItem ;
			memset( &kItem, 0, sizeof(kItem) ) ;
			kItem.snIndex = eKalaCore_Brahma - HT_PARAMTYPE_ITEM_START + 1 ;
			kItem.snDurability = eDur_Indestructible ;
			kItem.byCount = 1 ;

			CreateItem( x, y, &kItem, 0, 0, 0, 0 ) ;

			{
				char chBuf[512] = { 0,0,0, } ;
				sprintf( chBuf, "\t- CheckCountNRecovery()> Create a eKalaCore_Brahma - previous data(slot:%d)\r\n", iSlot ) ;
				WriteLog( chBuf, g_szrgLogFileName[eLogFileName_KalaSystem] ) ;
			}

			ChangeInfo( iSlot, eLoc_Ground, 0, 0, x, y, 0, 0 ) ;
		}
	}

	if( 3 > irgKalaCount[eTNClan_Vishnu] ) 
	{	
		int iSlot = FindEmptySlot() ;
		if( 0 <= iSlot && eKala_MaxCount > iSlot )
		{
			x = 490 ;
			y = 472 ;
			STRUCT_ITEM kItem ;
			memset( &kItem, 0, sizeof(kItem) ) ;
			kItem.snIndex = eKalaCore_Vishnu - HT_PARAMTYPE_ITEM_START + 1 ;
			kItem.snDurability = eDur_Indestructible ;
			kItem.byCount = 1 ;

			CreateItem( x, y, &kItem, 0, 0, 0, 0 ) ;
			{
				char chBuf[512] = { 0,0,0, } ;
				sprintf( chBuf, "\t- CheckCountNRecovery()> Create a eKalaCore_Vishnu - previous data(slot:%d)\r\n", iSlot ) ;
				WriteLog( chBuf, g_szrgLogFileName[eLogFileName_KalaSystem] ) ;
			}

			ChangeInfo( iSlot, eLoc_Ground, 0, 0, x, y, 0, 0 ) ;
		}
	}

	if( 3 > irgKalaCount[eTNClan_Siva] ) 
	{	
		int iSlot = FindEmptySlot() ;
		if( 0 <= iSlot && eKala_MaxCount > iSlot )
		{
			x = 512 ;
			y = 508 ;
			STRUCT_ITEM kItem ;
			memset( &kItem, 0, sizeof(kItem) ) ;
			kItem.snIndex = eKalaCore_Siva - HT_PARAMTYPE_ITEM_START + 1 ;
			kItem.snDurability = eDur_Indestructible ;
			kItem.byCount = 1 ;

			CreateItem( x, y, &kItem, 0, 0, 0, 0 ) ;
			{
				char chBuf[512] = { 0,0,0, } ;
				sprintf( chBuf, "\t- CheckCountNRecovery()> Create a eKalaCore_Siva - previous data(slot:%d)\r\n", iSlot ) ;
				WriteLog( chBuf, g_szrgLogFileName[eLogFileName_KalaSystem] ) ;
			}

			ChangeInfo( iSlot, eLoc_Ground, 0, 0, x, y, 0, 0 ) ;
		}
	}

	return eTNRes_Succeeded ;
}


void TNKalaSystem::Print( int a_iLocSlot, int a_iCaller )
{
	if( 0 > a_iLocSlot || eKala_MaxCount <= a_iLocSlot ) return ;

	/*
	{
		SYSTEMTIME st ;
		GetLocalTime( &st ) ;
		char chBuf2[256] = { 0,0,0, } ;
		sprintf(chBuf2, "\r\n\r\n[%dmm%ddd %dhh%dms%dss] Kala Print()\r\n"
			, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
			, m_krgKala[i].iWhere,

			) ; 
		WriteLog( chBuf2, g_szrgLogFileName[eLogFileName_KalaSystem] ) ;
	}
	*/

	{
		char chBuf2[512] = { 0,0,0, } ;
		sprintf(chBuf2, "\t=>%d. where:%d, altar:%d, monsterhandle:%d, coord(%d,%d), porter:%d(%u), Caller:%d \r\n"
			, a_iLocSlot, m_krgKala[a_iLocSlot].iWhere, m_krgKala[a_iLocSlot].iAltar, m_krgKala[a_iLocSlot].iMonsterHandle, m_krgKala[a_iLocSlot].iX, m_krgKala[a_iLocSlot].iY, m_krgKala[a_iLocSlot].iPCHandle, m_krgKala[a_iLocSlot].uiTimePossessed
			, a_iCaller
			) ; 
		WriteLog( chBuf2, g_szrgLogFileName[eLogFileName_KalaSystem] ) ;
	}
}


//--------------------------------------------------------------------------------
// kala의 현황을 log로 남긴다. 인자로 버퍼가 제공되면 그 안에도 함께 남긴다.
//@Param
//	- a_pBuffer : kala 현황을 담을 buffer
//--------------------------------------------------------------------------------
void TNKalaSystem::Print( char* a_pBuffer )
{
	{
		//SYSTEMTIME st ;
		//GetLocalTime( &st ) ;
		char chBuf2[256] = { 0,0,0, } ;
		sprintf(chBuf2, "\r\n\r\n[%dmm%ddd %dhh%dms%dss] Kala Print(), Altar:%d, PC:%d, Ground:%d \r\n"
			, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
			, m_iKalaOnAltar, m_iKalaCoreInPC, m_iKalaCoreOnGround
			) ; 
		WriteLog( chBuf2, g_szrgLogFileName[eLogFileName_KalaSystem] ) ;
	}

	for( int i = 0  ; i < eKala_MaxCount ; ++i )
	{
		char chBuf[256] = { 0,0,0, } ;

		switch( m_krgKala[i].iWhere )
		{
		case eLoc_Altar :
			{
				int iKala = m_krgKala[i].iMonsterHandle ;
				sprintf(chBuf, "   LocSlot:%d, Altar> Altar:%d, Monster:%d, tribe:%d, clan:%d \r\n"
					, i, m_krgKala[i].iAltar, iKala, pMob[iKala].MOB.snTribe, pMob[iKala].m_byClan ) ; 
			}
			break ;
		case eLoc_Ground :
			{
				sprintf(chBuf, "   LocSlot:%d, Ground> X:%d, Y:%d \r\n", i, m_krgKala[i].iX, m_krgKala[i].iY ) ; 

			}
			break ;
		case eLoc_Inventory :
			{
				if( 0 < m_krgKala[i].iPCHandle && MAX_USER > m_krgKala[i].iPCHandle )
					sprintf(chBuf, "   LocSlot:%d, Porter> PC:%d, %s \r\n", i, m_krgKala[i].iPCHandle, pMob[m_krgKala[i].iPCHandle].MOB.szName ) ; 
			}
			break ;
		case eLoc_NoWhere :
			{
				sprintf(chBuf, "   LocSlot:%d, Empty \r\n", i ) ;
			}
			break ;
		default :
			{
				sprintf(chBuf, "   LocSlot:%d, Unknown!!! \r\n", i ) ;
			}
			break ;
		}

		WriteLog( chBuf, g_szrgLogFileName[eLogFileName_KalaSystem] ) ;
		if( NULL != a_pBuffer ) strcat( a_pBuffer, chBuf ) ;
	}
}


void TNKalaSystem::PrintScreen( char* a_pBuffer )
{
	char chBuf2[256] = { 0,0,0, } ;
	sprintf(chBuf2, "Altar:%d, PC:%d, Ground:%d"
		, m_iKalaOnAltar, m_iKalaCoreInPC, m_iKalaCoreOnGround
		) ; 

	if( NULL != a_pBuffer ) strcat( a_pBuffer, chBuf2 ) ;
}

int TNKalaSystem::FindEmptySlot()
{
	for( int i = 0 ; i < eKala_MaxCount ; ++i )
	{
		if( eLoc_NoWhere == m_krgKala[i].iWhere ) return i ;
	}

	return -1 ;
}



// PC가 소유하고 있는 kala-core를 땅에 떨어뜨린다.
void TNKalaSystem::DropKalaCoreInPC()
{
	// 1. PC가 소유하고 있는 kala-core를 모두 drop 시켜준다. 그럼 altar or ground에 kala-core가 위치하고 있을 것이다.
	for( int i = 0 ; i < eKala_MaxCount ; ++i )
	{ // PC가 소유하고 있는, ...
		if( eLoc_Inventory == m_krgKala[i].iWhere )
		{
			int iUser = m_krgKala[i].iPCHandle ; 
			if( 0 < iUser && MAX_USER > iUser )
			{
				if( eTNVSAfn_HaveKalaCore & pMob[iUser].m_iAffections )
				{
					if( m_krgKala[i].uiTimePossessed < CurrentTime ) pMob[iUser].DropKalaCore( eReturnToShrine ) ;
				}					
			}
		}
	}
}



// server start up시에 altar에 배치를 할 때만 쓰인다.
void TNKalaSystem::Set( int a_iWhere, int a_iAltar, int a_iMonsterHandle )
{
	//SYSTEMTIME st ;
	//GetLocalTime( &st ) ;

	int iSlot = FindEmptySlot() ;
	if( -1 == iSlot )
	{
		char chBuf[256] = { 0,0,0, } ;
		sprintf(chBuf, "[%dmm%ddd %dhh%dms%dss] Set> all slots are full. argument(where:%d, altar:%d, monsterHandle:%d) \r\n"
			, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
			, a_iWhere, a_iAltar, a_iMonsterHandle
			) ; 
		WriteLog( chBuf, g_szrgLogFileName[eLogFileName_KalaSystem] ) ;
	}
	
	/*
	int i = 0 ;
	for( ; i < eKala_MaxCount ; ++i ) if( eLoc_NoWhere == m_krgKala[i].iWhere ) break ;
	*/
	m_krgKala[iSlot].iWhere = a_iWhere ;
	m_krgKala[iSlot].iAltar = a_iAltar ;
	m_krgKala[iSlot].iMonsterHandle = a_iMonsterHandle ;
	m_krgKala[iSlot].iX = 0 ;
	m_krgKala[iSlot].iY = 0 ;
	m_krgKala[iSlot].iPCHandle = 0 ;
	m_krgKala[iSlot].iInventorySlot = 0 ;
	m_krgKala[iSlot].uiTimePossessed = 0 ; // CurrentTime + 1800000 ; // 30분

	m_krgAltar[a_iAltar].iKala = a_iMonsterHandle ;

	{
		char chBuf[256] = { 0,0,0, } ;
		sprintf(chBuf, "[%dmm%ddd %dhh%dms%dss] Set> Slot:%d(Where:%d, Altar:%d, Monster:%d) \r\n"
			, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
			, iSlot
			, a_iWhere, a_iAltar, a_iMonsterHandle
			) ; 
		WriteLog( chBuf, g_szrgLogFileName[eLogFileName_KalaSystem] ) ;
	}
}


int TNKalaSystem::ChangeInfo( int a_iKalaSlot, int a_iWhere, int a_iAltar, int a_iMonsterHandle, int a_iX, int a_iY, int a_iPCHandle, int a_iInventorySlot )
{
	if( 0 > a_iKalaSlot || eKala_MaxCount <= a_iKalaSlot ) return eTNRes_Failed ;
/*
	SYSTEMTIME st ;
	GetLocalTime( &st ) ;

	{
		char chBuf[256] = { 0,0,0, } ;
		sprintf(chBuf, "[%dmm%ddd %dhh%dms%dss] Slot:%d(Where:%d, Altar:%d, Monster:%d, X:%d, Y:%d, PC:%d, Inven:%d, Time:%u) -> (Where:%d, Altar:%d, Monster:%d, X:%d, Y:%d, PC:%d, Inven:%d)\r\n"
			, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
			, a_iKalaSlot
			, m_krgKala[a_iKalaSlot].iWhere, m_krgKala[a_iKalaSlot].iAltar, m_krgKala[a_iKalaSlot].iMonsterHandle, m_krgKala[a_iKalaSlot].iX, m_krgKala[a_iKalaSlot].iY, m_krgKala[a_iKalaSlot].iPCHandle, m_krgKala[a_iKalaSlot].iInventorySlot
			, a_iWhere, a_iAltar, a_iMonsterHandle, a_iX, a_iY, a_iPCHandle, a_iInventorySlot, CurrentTime 
			) ; 
		WriteLog( chBuf, g_szrgLogFileName[eLogFileName_KalaSystem] ) ;
	}
*/
	m_krgKala[a_iKalaSlot].iWhere = a_iWhere ;
	m_krgKala[a_iKalaSlot].iAltar = a_iAltar ;
	m_krgKala[a_iKalaSlot].iMonsterHandle = a_iMonsterHandle ;
	m_krgKala[a_iKalaSlot].iX = a_iX ;
	m_krgKala[a_iKalaSlot].iY = a_iY ;
	m_krgKala[a_iKalaSlot].iPCHandle = a_iPCHandle ;
	m_krgKala[a_iKalaSlot].iInventorySlot = a_iInventorySlot ;
	m_krgKala[a_iKalaSlot].uiTimePossessed = CurrentTime + 1800000 ; // 30분

	return eTNRes_Succeeded ;
}



// kala는 항상 형 전이가 일어난다. (단 예외는 최소 제단에 배치할때뿐이다.)
// 따라서 source와 destination을 명시하여 이동을 관리한다. 
//@Param
//  - a_iTaker : 이런 것을 발생시킨 PC
int TNKalaSystem::ChangeKalaLoc( int a_iSrcWhere, int a_iSrcAltar, int a_iSrcMonsterHandle, int a_iSrcX, int a_iSrcY, int a_iSrcPCHandle, int a_iDestWhere, int a_iDestAltar, int a_iDestMonsterHandle, int a_iDestX, int a_iDestY, int a_iDestPCHandle, int a_iTaker )
{
	//SYSTEMTIME st ;
	//GetLocalTime( &st ) ;

	if( eLoc_NoWhere == a_iSrcWhere ) return eTNRes_InvalidLocation ;

	int iAltarSlot = -1;
	int iKalaLocSlot = FindKalaLoc( a_iSrcWhere, a_iSrcAltar, a_iSrcMonsterHandle, a_iSrcX, a_iSrcY, a_iSrcPCHandle, 0 ) ;

	{
		char chBuf[512] = { 0,0,0, } ;
		sprintf(chBuf, "[%dmm%ddd %dhh%dms%dss] ChangeKalaLoc() > Slot:(kala:%d, altar ID:%d), Src> (Where:%d, Altar:%d, Monster:%d, X:%d, Y:%d, PC:%d) -> Dest> (Where:%d, Altar:%d, Monster:%d, X:%d, Y:%d, PC:%d)\r\n"
			, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
			, iKalaLocSlot, m_krgKala[iKalaLocSlot].iAltar
			, a_iSrcWhere, a_iSrcAltar, a_iSrcMonsterHandle, a_iSrcX, a_iSrcY, a_iSrcPCHandle
			, a_iDestWhere, a_iDestAltar, a_iDestMonsterHandle, a_iDestX, a_iDestY, a_iDestPCHandle
			) ; 
		WriteLog( chBuf, g_szrgLogFileName[eLogFileName_KalaSystem] ) ;
	}

	if( -1 == iKalaLocSlot )
	{
		WriteLog( "\r\n[Error] KalaSystem is broken!!!\r\n", g_szrgLogFileName[eLogFileName_KalaSystem] ) ;
		return eTNRes_InvalidSlotIndex ;
	}
	else
	{
		if( eLoc_Altar == a_iSrcWhere )
		{
			iAltarSlot = m_krgKala[iKalaLocSlot].iAltar ;
			m_krgAltar[iAltarSlot].iKala = 0 ;

			// kala 몬스터가 죽어서 altar에서 떨어져나가게 된다.
			if( 0 == iAltarSlot ) TriggerEvent( a_iTaker, 96/*event id*/, 0, 0, 0, 15770 );
			else if( 1 == iAltarSlot ) TriggerEvent( a_iTaker, 97/*event id*/, 0, 0, 0, 15771 );
			else if( 2 == iAltarSlot ) TriggerEvent( a_iTaker, 98/*event id*/, 0, 0, 0, 15772 );
			else if( 3 == iAltarSlot ) TriggerEvent( a_iTaker, 99/*event id*/, 0, 0, 0, 15773 );
			else if( 4 == iAltarSlot ) TriggerEvent( a_iTaker, 100/*event id*/, 0, 0, 0, 15774 );
			else if( 5 == iAltarSlot ) TriggerEvent( a_iTaker, 101/*event id*/, 0, 0, 0, 15775 );
			else if( 6 == iAltarSlot ) TriggerEvent( a_iTaker, 102/*event id*/, 0, 0, 0, 15776 );
			else if( 7 == iAltarSlot ) TriggerEvent( a_iTaker, 103/*event id*/, 0, 0, 0, 15777 );
			else if( 8 == iAltarSlot ) TriggerEvent( a_iTaker, 104/*event id*/, 0, 0, 0, 15778 );
			else if( 9 == iAltarSlot ) TriggerEvent( a_iTaker, 105/*event id*/, 0, 0, 0, 15779 );
			else if( 10 == iAltarSlot ) TriggerEvent( a_iTaker, 106/*event id*/, 0, 0, 0, 15780 );
			else if( 11 == iAltarSlot ) TriggerEvent( a_iTaker, 107/*event id*/, 0, 0, 0, 15781 );
			else if( 12 == iAltarSlot ) TriggerEvent( a_iTaker, 108/*event id*/, 0, 0, 0, 15782 );
			else if( 13 == iAltarSlot ) TriggerEvent( a_iTaker, 109/*event id*/, 0, 0, 0, 15783 );
			else if( 14 == iAltarSlot ) TriggerEvent( a_iTaker, 110/*event id*/, 0, 0, 0, 15784 );
		}

		// destination 내용을 덮어쓰기 한다.
		int iRes = ChangeInfo( iKalaLocSlot, a_iDestWhere, a_iDestAltar, a_iDestMonsterHandle, a_iDestX, a_iDestY, a_iDestPCHandle, 0 ) ;
		if( iRes ) // fail는 절대로 일어나지 않을 것이다. 왜냐하면 위의 FindKalaLoc()를 통해서 1차적으로 걸러지기 때문이다.
		{
			WriteLog( "<<ERROR iKalaLocSlot is invalid!", g_szrgLogFileName[eLogFileName_KalaSystem] ) ;
			return -2;
		}

		if( eLoc_Altar == a_iDestWhere )
		{
			int iAltarSlot = m_krgKala[iKalaLocSlot].iAltar ;
			m_krgAltar[iAltarSlot].iKala = m_krgKala[iKalaLocSlot].iMonsterHandle ;
		}
	}

	return iAltarSlot;
}



//@Return
//	- if -1 is returned, the kala is not found at the list.
int TNKalaSystem::FindKalaLoc( int a_iWhere, int a_iAltar, int a_iMonsterHandle, int a_iX, int a_iY, int a_iPCHandle, int a_iInventorySlot )
{
	for( int i = 0 ; i < eKala_MaxCount ; ++i )
	{
		if( a_iWhere != m_krgKala[i].iWhere ) continue ;

		switch( a_iWhere )
		{
		case eLoc_Altar :
			{
				if( m_krgKala[i].iMonsterHandle == a_iMonsterHandle ) return i ;
			}
			break ;
		case eLoc_Ground :
			{				
				if( (m_krgKala[i].iX == a_iX) && (m_krgKala[i].iY == a_iY) ) return i ;

				int iRes = FindKalaCoreOnTheGround( a_iX, a_iY ) ;
				if( eTNRes_Failed != iRes ) return i ;
			}
			break ;
		case eLoc_Inventory :
			{
				if( m_krgKala[i].iPCHandle == a_iPCHandle ) return i ;
			}
			break ;
		}
	}


	{
		char chBuf[512] = { 0,0,0, } ;
		sprintf(chBuf, "<<ERROR FindKalaLoc() > where:%d, altar:%d, MonsterHandle:%d, Coord(%d,%d), PCHandle:%d, inventorySlot:%d \r\n"
			, a_iWhere, a_iAltar, a_iMonsterHandle, a_iX, a_iY, a_iPCHandle, a_iInventorySlot ) ;
		WriteLog( chBuf, g_szrgLogFileName[eLogFileName_KalaSystem] ) ;
		Print() ;
	}


	return -1 ;
}


// 실패하면 1을 return하고 성공하면 어떤 kala-core인지 리턴한다.
int TNKalaSystem::FindKalaCoreOnTheGround( int& a_iX, int& a_iY )
{
	int iSlot = pItemGrid[a_iY][a_iX] ;
	int iID = 0;
	if( 0 < iSlot && MAX_ITEM > iSlot ) 
	{
		iID = pItemData[pItem[iSlot].ITEM.snIndex].sID ;
		if( eKalaCore_Brahma == iID || eKalaCore_Vishnu == iID || eKalaCore_Siva == iID )
		{// 보정을 해준다.
			{
				char chBuf[512] = { 0,0,0, } ;
				sprintf(chBuf, " >> FindKalaCoreOnTheGround() > Exact Pos(%d,%d)", a_iX, a_iY ) ;
				WriteLog( chBuf, g_szrgLogFileName[eLogFileName_KalaSystem] ) ;
			}
			return iID ; // eTNRes_Succeeded ;
		}
	}

	for( int y = a_iY - 3 ; y < a_iY + 3 ; ++y )
	{
		for( int x = a_iX -3 ; x < a_iX + 3 ; ++ x )
		{
			if	( x<0 || y<0 || x>MAX_GRIDX || y>MAX_GRIDY ) continue ;

			iSlot = pItemGrid[y][x] ;
			if( 0 > iSlot || MAX_ITEM <= iSlot ) continue ;
			iID = pItemData[pItem[iSlot].ITEM.snIndex].sID ;
			if( eKalaCore_Brahma == iID || eKalaCore_Vishnu == iID || eKalaCore_Siva == iID )
			{// 보정을 해준다.
				{
					char chBuf[512] = { 0,0,0, } ;
					sprintf(chBuf, " >> FindKalaCoreOnTheGround() > Before(%d,%d) -> After(%d,%d)", a_iX, a_iY, x, y ) ;
					WriteLog( chBuf, g_szrgLogFileName[eLogFileName_KalaSystem] ) ;
				}
				a_iX = x ;
				a_iY = y ;
				return iID ; // eTNRes_Succeeded ;
			}
		}
	}

	return eTNRes_Failed ;
}



int TNKalaSystem::FindEmptyAltar( int a_iClan )
{
	//m_krgAltar
	int iAltarSlot = 0 ;
	for(  ; iAltarSlot < eKalaAltar_MaxCount ; ++ iAltarSlot )
	{
		if( 0 != m_krgAltar[iAltarSlot].iKala ) continue ;
		if( 0 < a_iClan )
			if( a_iClan != g_krgKalaAltar[iAltarSlot].sTrimuriti ) continue ;

		return iAltarSlot ;
	}

	return -1 ;
	//if( eKalaAltar_MaxCount < iAltarSlot ) return eTNRes_Failed ;
	//return eTNRes_Succeeded ;
}



int TNKalaSystem::InstallKala( int a_iTribe, int a_iClan, int a_iAltarSlot, int a_iPCHandle )
{
	//SYSTEMTIME st ;
	//GetLocalTime( &st ) ;

	if( 0 > a_iAltarSlot || eKalaAltar_MaxCount < a_iAltarSlot )
	{
		{
			char chBuf[256] = { 0,0,0, } ;
			sprintf(chBuf, "<<ERROR [%dmm%ddd %dhh%dms%dss] Invalid Slot! Argument(Tribe:%d, Clan:%d, Slot:%d, PC:%d)\r\n"
				, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
				, a_iTribe, a_iClan, a_iAltarSlot, a_iPCHandle
				) ; 
			WriteLog( chBuf, g_szrgLogFileName[eLogFileName_KalaSystem] ) ;
		}
		
		return eTNRes_InvalidSlotIndex ;
	}

	// 원하는 위치가 비어 있는지 확인
	if( (MAX_USER <= m_krgAltar[a_iAltarSlot].iKala) && (MAX_MOB > m_krgAltar[a_iAltarSlot].iKala) )
	{// 다른 칼라가 이미 놓여있다.
		{
			char chBuf[256] = { 0,0,0, } ;
			sprintf(chBuf, "<<ERROR [%dmm%ddd %dhh%dms%dss] The altar is not empty! Argument(Tribe:%d, Clan:%d, Slot:%d, PC:%d)\r\n"
				, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
				, a_iTribe, a_iClan, a_iAltarSlot, a_iPCHandle
				) ; 
			WriteLog( chBuf, g_szrgLogFileName[eLogFileName_KalaSystem] ) ;
		}

		return eTNRes_AltarIsNotEmpty ;
	}

	int iMob = 0 ;
	int iStart = 0 ;
	if( eKala_Brahma == a_iTribe ) iStart = 1000 ;
	else if( eKala_Vishnu == a_iTribe ) iStart = 1003 ;
	else if( eKala_Siva == a_iTribe ) iStart = 1006 ;

	for( int i = 0 ; i < 3 ; ++i )
	{
		if( pMob[iStart].IsDead() )
		{
			iMob = iStart ;
			break ;
		}
		++iStart ;
	}

	if( 1000 > iMob || 1008 < iMob )
	{
		{
			char chBuf[256] = { 0,0,0, } ;
			sprintf(chBuf, "<<ERROR [%dmm%ddd %dhh%dms%dss] All handle are used! HandleFounded(%d), Argument(Tribe:%d, Clan:%d, Slot:%d, PC:%d) \r\n"
				, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
				, iMob
				, a_iTribe, a_iClan, a_iAltarSlot, a_iPCHandle
				) ; 
			WriteLog( chBuf, g_szrgLogFileName[eLogFileName_KalaSystem] ) ;
		}
		return eTNRes_Failed ;
	}


	if( 0 == a_iPCHandle )
	{// 최소 배치
		Set( eLoc_Altar, a_iAltarSlot, iMob ) ;
	}
	else
	{
		ChangeKalaLoc( eLoc_Inventory, 0, 0, 0, 0, a_iPCHandle, eLoc_Altar, a_iAltarSlot, iMob, 0, 0, 0 ) ;

		// kala-core를 altar에 꽂아서 kala 몬스터가 생성되게 된다.
		if( 0 == a_iAltarSlot ) TriggerEvent( a_iPCHandle, 81/*event id*/, 0, 0, 0, 15750 );
		else if( 1 == a_iAltarSlot ) TriggerEvent( a_iPCHandle, 82/*event id*/, 0, 0, 0, 15751 );
		else if( 2 == a_iAltarSlot ) TriggerEvent( a_iPCHandle, 83/*event id*/, 0, 0, 0, 15752 );
		else if( 3 == a_iAltarSlot ) TriggerEvent( a_iPCHandle, 84/*event id*/, 0, 0, 0, 15753 );
		else if( 4 == a_iAltarSlot ) TriggerEvent( a_iPCHandle, 85/*event id*/, 0, 0, 0, 15754 );
		else if( 5 == a_iAltarSlot ) TriggerEvent( a_iPCHandle, 86/*event id*/, 0, 0, 0, 15755 );
		else if( 6 == a_iAltarSlot ) TriggerEvent( a_iPCHandle, 87/*event id*/, 0, 0, 0, 15756 );
		else if( 7 == a_iAltarSlot ) TriggerEvent( a_iPCHandle, 88/*event id*/, 0, 0, 0, 15757 );
		else if( 8 == a_iAltarSlot ) TriggerEvent( a_iPCHandle, 89/*event id*/, 0, 0, 0, 15758 );
		else if( 9 == a_iAltarSlot ) TriggerEvent( a_iPCHandle, 90/*event id*/, 0, 0, 0, 15759 );
		else if( 10 == a_iAltarSlot ) TriggerEvent( a_iPCHandle, 91/*event id*/, 0, 0, 0, 15760 );
		else if( 11 == a_iAltarSlot ) TriggerEvent( a_iPCHandle, 92/*event id*/, 0, 0, 0, 15761 );
		else if( 12 == a_iAltarSlot ) TriggerEvent( a_iPCHandle, 93/*event id*/, 0, 0, 0, 15762 );
		else if( 13 == a_iAltarSlot ) TriggerEvent( a_iPCHandle, 94/*event id*/, 0, 0, 0, 15763 );
		else if( 14 == a_iAltarSlot ) TriggerEvent( a_iPCHandle, 95/*event id*/, 0, 0, 0, 15764 );
	}

	//m_krgAltar[a_iAltarSlot].iKala = iMob ;



	int iMonsterDataIndex = a_iTribe - 2000 ; // the index of monster data
	pMob[iMob].MOB = pMonsterData[iMonsterDataIndex] ;
	pMob[iMob].MOB.nTP = iMonsterDataIndex ;
	pMob[iMob].Init( iMob ) ; // 초기화

	int x, y ;
	x = g_krgKalaAltar[a_iAltarSlot].x ;
	y = g_krgKalaAltar[a_iAltarSlot].y ;

	if( 0 < pMobGrid[y][x] && MAX_MOB > pMobGrid[y][x] ) // 다른 mob이 위치하고 있다.
	{ // dest 자리를 검사하고, 다른 mob이 있으면, 처리(제거)한 후에, 꼭 그 위치에 위치를 시킨다.
		int iOccupiedMob = pMobGrid[y][x] ;
		int iNewX, iNewY ;
		iNewX = x ; iNewY = y ;
		int tret = GetEmptyMobGrid( iOccupiedMob, &iNewX, &iNewY ) ;
		if( FALSE == tret )
		{ // user이면 kick out 시켜주고, monster이면 단순히 죽인다.
			if( 0 < iOccupiedMob && MAX_USER > iOccupiedMob ) // user이면
			{
				char temp[256] = {0,};
				SendClientMessage( iOccupiedMob, g_pMessageStringTable[_Bad_Network_Packets] ) ;
				sprintf("clo TNKalaSuatem InstallKala mob:%s", pMob[iOccupiedMob].MOB.szName);
				Log(temp,pUser[iOccupiedMob].AccountName,pUser[iOccupiedMob].IP);  
				CloseUser( iOccupiedMob ) ;
			}
			else 
			{
				KillMonster( iOccupiedMob ) ;
			}
		}
		else // 기존의 위치를 바꾼다.
		{			
			if( 0 < iOccupiedMob && MAX_USER > iOccupiedMob ) // user이면
			{
				MSG_Action sm;	GetAction( iOccupiedMob, iNewX, iNewY ,&sm ) ;
				pUser[iOccupiedMob].cSock.SendOneMessage( (char*)&sm, sizeof(sm) ) ;					
			}
			else
			{
				//pMobGrid[iNewY][iNewX] = iOccupiedMob ;
			}

			pMobGrid[iNewY][iNewX] = iOccupiedMob ;				
		}

		pMobGrid[y][x] = iMob ;
	}

	pMob[iMob].LastX = pMob[iMob].TargetX = pMob[iMob].SegmentX = x ;
	pMob[iMob].LastY = pMob[iMob].TargetY = pMob[iMob].SegmentY = y ;

	// default settings
	for( int i = 0 ; i < MAX_SEGMENT ; ++i )
	{
		pMob[iMob].SegmentListX[i] = x ;
		pMob[iMob].SegmentListY[i] = y ;
		pMob[iMob].SegmentWait[i] = rand() % 8 ;
		pMob[iMob].SegmentRange[i] = rand() % 20 ;
	}
	pMob[iMob].RouteType = eTNRout_MoveNRoam ;
	pMob[iMob].Formation = 0 ;

	pMob[iMob].MOB.byClass1 = (byte)eTNCls_Event ; // 소환되는 NPC의 직업
	pMob[iMob].MOB.byClass2 = 0 ; // 소환되는 NPC의 상세 직업            
	pMob[iMob].m_byClan = pMob[iMob].MOB.byTrimuriti = (byte)a_iClan ;

	S_SCP_INIT_OTHER_MOB sm ;
	GetCreateMob( iMob, &sm ) ;
	sm.byPopType = eTNPrdt_PopRaise ; // 소환될 때의 연출 번호
	pMobGrid[y][x] = iMob ;
	GridMulticast( x, y, (MSG_STANDARD*)&sm, 0) ;

	{
		char chBuf[256] = { 0,0,0, } ;
		sprintf(chBuf, "[%dmm%ddd %dhh%dms%dss] InstallKala() > summon a kala successfully. Argument(Tribe:%d, Clan:%d, Altar:%d, PC:%d)\r\n"
			, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
			, a_iTribe, a_iClan, a_iAltarSlot, a_iPCHandle
			) ; 
		WriteLog( chBuf, g_szrgLogFileName[eLogFileName_KalaSystem] ) ;
	}

	return eTNRes_Succeeded ;
}

/*
* kala-core가 신전 내부에서 drop되면, 순간 ground로 갔다가, altar로 위치를 변경한다. 

1. monster의 형태로 되어 있는 경우
	- kala-core를 altar에 놓게 될 때
		- server.cpp > register_kala ***
2. ground에 있을 경우
	- user가 kala를 destroy를 하였을 때
	- user가 어떠한 이유이든 kala-core를 떨어뜨렸을 때,
		- DropKalaCore() ***
3. user가 kala-core를 집었을 때,
	- ground->inventory ***









*/