/****************************************************************************

	���ϸ� : TriggerEvent.cpp

	�ۼ��� : �����(spencerj@korea.com)
	�ۼ��� : 2005-12-07

	Tab size : 4 spaces
	������Ʈ�� : Tantra


	���� : 
		- 
		- 

****************************************************************************/

#include "Basedef.h"
#include "Server.h"
#include "CPSock.h"
#include "Language.h"

#include "CUser.h"
#include "CMob.h"
#include "CItem.h"
#include "SendFunc.h"
#include "GetFunc.h"
#include "HTParamMgr.h"
#include "TNKalaSystem.h"
#include "TNSiege.h"
#include "Quest.h"
#include "TNArena.h"
#include "TNScheduler.h"
#include "TNDeck.h"
#include "Yut.h"
#ifndef eTNAct_ReturnToSaveZone
#define eTNAct_ReturnToSaveZone 23020
#endif

extern unsigned int CurrentTime;
extern TNEVENT g_krgEventList[eEvent_MaxCount];
extern TNTASK g_krgTaskList[eTask_MaxCount];
extern int g_iSwitch;
extern int g_iTimeSlot;
extern short g_srgTimeSchedule[eTS_MaxSlot];
extern TNScheduler g_kScheduler;
extern CYut g_Yut;
extern unsigned int YutCounter;
extern int g_iGambleHorse;
extern TNDeck g_krgEventDeck[eSiz_EventDeck];
extern int g_irgEntryCount[5];
extern TNSTRONGHOLD g_krgStronghold[eStronghold_MaxCount];
extern int g_bIsKaliaOpend;
extern int g_irgGateOfDungeon[3];
extern int g_iBattleForStronghold;
extern DWORD g_dwArenaMoney;

//----------------------------------------------------------------------
//@Param
//	- a_iUser : event�� trigger �ϴ� ���
//	- a_iEventID : trigger�Ǵ� event
//	- a_iX, a_iY : event�� �߻��� cell ��ġ
//	- a_iNPC : event�� ���� task�� �����ϴ� NPC
//----------------------------------------------------------------------
int TriggerEvent( int a_iUser, int a_iEventID, int a_iX, int a_iY, int a_iNPC, int a_iCaller )
{
	if( 0 >= a_iEventID || eEvent_MaxCount <= a_iEventID ) return eTNRes_EvntInvalidEventNo;
	if( CurrentTime < g_krgEventList[a_iEventID].uiAvailableTime ) return eTNRes_EvntNotCoolYet; // ���� cool-down�� �Ϸ���� �ʾҴ�.
	if( eEvntSwitch_Off == g_krgEventList[a_iEventID].iDuration ) return eTNRes_EvntSwitchOff; // �̿�Ұ��̸� �� �� ����.
	if( eEvntSwitch_On < g_krgEventList[a_iEventID].iDuration && CurrentTime > g_krgEventList[a_iEventID].uiDurationTime )
	{
		g_krgEventList[a_iEventID].iDuration = eEvntSwitch_Off; // �̿�Ұ��� �����Ѵ�.
		g_krgEventList[a_iEventID].uiDurationTime = 0;
		return eTNRes_EvntUseTimeOver;
	}
	if( 0 <= g_krgEventList[a_iEventID].sClan )
	{
		if( 0 < a_iUser && MAX_USER > a_iUser )
			if( g_krgEventList[a_iEventID].sClan != pMob[a_iUser].m_byClan ) return eTNRes_EvntDiffClan; // Ư�� clan �������� �����Ǿ� �ִ� ���
	}

	int iStartIndex = 0, iEndIndex = eTask_Sort;
	if( eEvntPrcdType_Random == g_krgEventList[a_iEventID].sProceedType )
	{
		iStartIndex = rand() % eTask_Sort; // ��� task ��Ͽ� ���� ä�����־�� �Ѵ�.
		iEndIndex = iStartIndex + 1;
	}

	//SYSTEMTIME st;
	//GetLocalTime( &st );

	if( eTNSwitch_EventLog & g_iSwitch )
	{
		char chBuf[2048] = { 0,0,0, };
		sprintf(chBuf, "[TriggerEvent] %dmm��%ddd %dhh%dms%dss, Event:%d, Caller:%d, User:%d, NPC:%d, Coord(%d,%d) \r\n", g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
			, a_iEventID, a_iCaller, a_iUser, a_iNPC, a_iX, a_iY );
		WriteLog( chBuf, g_szrgLogFileName[eLogFileName_EventTriggered] );
	}

	int iProceed = 1;
	
	for( int i = iStartIndex; i < iEndIndex; ++i )
	{
		int iTaskID = g_krgEventList[a_iEventID].srgTask[i];
		if( 0 >= iTaskID || eTask_MaxCount <= iTaskID ) continue;
		if( 0 >= iProceed ) break; // ���� ���� ������ ������ ���, ...

		switch( g_krgTaskList[iTaskID].iActionID )
		{
		case eTNAct_ResetChakra :
			{
				if( 0 >= a_iUser || MAX_USER <= a_iUser ) return eTNRes_EvntInvalidUserHandle;
				pMob[a_iUser].ResetStat();
			}
			break;
		case eTNAct_ResetSkillBook :
			{
				if( 0 >= a_iUser || MAX_USER <= a_iUser ) return eTNRes_EvntInvalidUserHandle;
				pMob[a_iUser].ResetSkill();
			}
			break;
		case eTNAct_ResetClass :
			{
				if( 0 >= a_iUser || MAX_USER <= a_iUser ) return eTNRes_EvntInvalidUserHandle;
				pMob[a_iUser].ResetClass();
			}
			break;
		case eTNAct_CheckItemOnly :
			{
				if( 0 >= a_iUser || MAX_USER <= a_iUser ) return eTNRes_EvntInvalidUserHandle;
				int irgSlot[5] = { -1,-1,-1,-1,-1 }; // item �˻�/���Ÿ� ���� slot No ���
				int iCount = 0;
				iCount = pMob[a_iUser].CheckItem( g_krgTaskList[iTaskID].irgParam[0], g_krgTaskList[iTaskID].irgParam[1], irgSlot ); // �䱸������ �������� ���ϸ� 0�� return
				if( 0 >= iCount ) iProceed = 0;
			}
			break;
		case eTNAct_CheckWeapon :
			{
				if( 0 >= a_iUser || MAX_USER <= a_iUser ) return eTNRes_EvntInvalidUserHandle;
				int iWeapon = pMob[a_iUser].get_WeaponID();
				if( iWeapon != g_krgTaskList[iTaskID].irgParam[0] )
				{ // check�� item�� ����. NPC�� ���� �Ѵ�.(�ʿ��� �������� �����ϴ�.)
					if( MAX_USER <= a_iNPC && MAX_MOB > a_iNPC )
                        pMob[a_iNPC].Speak( 200, a_iUser, 0 );
					return eTNRes_EvntItemNotFound;
				}
			}
			break;

		case eTNAct_CheckRemoveItem :
			{
				if( 0 >= a_iUser || MAX_USER <= a_iUser ) return eTNRes_EvntInvalidUserHandle;
				int irgSlot[5] = { -1,-1,-1,-1,-1 }; // item �˻�/���Ÿ� ���� slot No ���
				int iCount = 0;
				iCount = pMob[a_iUser].CheckItem( g_krgTaskList[iTaskID].irgParam[0], g_krgTaskList[iTaskID].irgParam[1], irgSlot ); // �䱸������ �������� ���ϸ� 0�� return
				int iRes = eTNRes_Succeeded;
				if( iCount < g_krgTaskList[iTaskID].irgParam[1] ) iRes = eTNRes_Failed;
				if( eTNRes_Succeeded == iRes )
                    iRes = pMob[a_iUser].RemoveItem( g_krgTaskList[iTaskID].irgParam[1], irgSlot ); 

				if( iRes )
				{ // check�� item�� ����. NPC�� ���� �Ѵ�.(�ʿ��� �������� �����ϴ�.)
					if( MAX_USER <= a_iNPC && MAX_MOB > a_iNPC )
                        pMob[a_iNPC].Speak( 200, a_iUser, 0 );
					return eTNRes_EvntItemNotFound;
				}
				else
				{ // ã������ item�� �ִ�.
					if( 7205 == g_krgTaskList[iTaskID].irgParam[0] ) // ��Ǫ�� ������
					{
						//SYSTEMTIME st;
						//GetLocalTime( &st );

						char chBuf[512] = { 0,0,0, };
						sprintf(chBuf, "[eTNAct_CheckRemoveItem] %d��%d��%d��%d��%d��, %s unlocked the gate of the king Raphu!!\r\n", g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
							, pMob[a_iUser].MOB.szName );
						WriteLog( chBuf, g_szrgLogFileName[eLogFileName_BossSystem] );
					}
				}
			}
			break;
		case eTNAct_DropItem :
			{
				if( HT_PARAMTYPE_ITEM_START <= g_krgTaskList[iTaskID].irgParam[0] && HT_PARAMTYPE_ITEM_END >= g_krgTaskList[iTaskID].irgParam[0] )
				{
					STRUCT_ITEM kItem;
					memset( &kItem, 0, sizeof(kItem) );
					kItem.snIndex = g_krgTaskList[iTaskID].irgParam[0] - HT_PARAMTYPE_ITEM_START + 1;
					kItem.snDurability = pItemData[kItem.snIndex].sMaxDur;
					kItem.byCount = 1;

					int iRes = CreateItem( g_krgTaskList[iTaskID].irgParam[1], g_krgTaskList[iTaskID].irgParam[2], &kItem, 0, 0, 0, 0 );
					char chBuf[512] = { 0,0,0, };
					sprintf(chBuf, "[eTNAct_DropItem] %d��%d��%d��%d��%d��, A item(%d) is dropped at (%d,%d)! Result:%d(0:Failed, other:Succeeded) \r\n"
						, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
						, kItem.snIndex
						, g_krgTaskList[iTaskID].irgParam[1], g_krgTaskList[iTaskID].irgParam[2]
						, iRes
						 );
					WriteLog( chBuf, g_szrgLogFileName[eLogFileName_ItemDroppedByEvent] );				
				}
			}
			break;
		case eTNAct_CheckQuest :
			{
				if( 0 >= a_iUser || MAX_USER <= a_iUser ) return eTNRes_EvntInvalidUserHandle;
				int iQuestID = g_krgTaskList[iTaskID].irgParam[0];
				if( 0 > iQuestID || MAX_EVENT_FLAG <= iQuestID ) return eTNRes_EvntInvalidQuestID;
				//if( (0 >= pMob[a_iUser].MOB.byQuest[iQuestID]) || (255 <= pMob[a_iUser].MOB.byQuest[iQuestID] ) )
				if( g_krgTaskList[iTaskID].irgParam[1] != pMob[a_iUser].MOB.byQuest[iQuestID] )
				{
					iProceed = 0;
				}
			}
			break;
		case eTNAct_SetClan :
			{
				if( 0 >= a_iUser || MAX_USER <= a_iUser ) return eTNRes_EvntInvalidUserHandle;
				int iTargetEvent = g_krgTaskList[iTaskID].irgParam[0];
				if( 0 > iTargetEvent || eEvent_MaxCount <= iTargetEvent ) return eTNRes_EvntInvalidEventNo;
				g_krgEventList[iTargetEvent].sClan = pMob[a_iUser].m_byClan;
				//if( -1 == g_krgTaskList[iTaskID].irgParam[0] ) g_krgTaskList[iTaskID].irgParam[0] = pMob[a_iUser].m_byClan;
				//else iProceed = pMob[a_iUser].CheckClan( g_krgTaskList[iTaskID].irgParam[0] );
			}
			break;
		case eTNAct_SetDuration :
			{
				int iTargetEvent = g_krgTaskList[iTaskID].irgParam[0];
				if( 0 > iTargetEvent || eEvent_MaxCount <= iTargetEvent ) return eTNRes_EvntInvalidEventNo;

				g_krgEventList[iTargetEvent].iDuration = g_krgTaskList[iTaskID].irgParam[1];
				if( 0 < g_krgEventList[iTargetEvent].iDuration ) g_krgEventList[iTargetEvent].uiDurationTime = CurrentTime + (g_krgTaskList[iTaskID].irgParam[1]*1000);
			}
			break;
		case eTNAct_SetAvailableTime :
			{
				int iTargetEvent = g_krgTaskList[iTaskID].irgParam[0];
				if( 0 > iTargetEvent || eEvent_MaxCount <= iTargetEvent ) return eTNRes_EvntInvalidEventNo;
				g_krgEventList[iTargetEvent].uiAvailableTime = CurrentTime + (g_krgTaskList[iTaskID].irgParam[1]*1000);
			}
			break;
		case eTNAct_TriggerEvent :
			{
				TriggerEvent( a_iUser, g_krgTaskList[iTaskID].irgParam[0], a_iX, a_iY, a_iNPC, 80 );
			}
			break;
		case eTNAct_AddEventOnScheduler  :
			{
				int iTimeSlot = g_iTimeSlot + (g_krgTaskList[iTaskID].irgParam[1] / 4);
				if( eTS_MaxSlot <= iTimeSlot ) iTimeSlot -= eTS_MaxSlot;
				if( 0 < g_srgTimeSchedule[iTimeSlot] ) // �ٸ� event�� ����� �Ǿ� �ִٸ�, ...
				{
					int iPrevSlot = iTimeSlot-1;
					if( 0 > iPrevSlot ) iPrevSlot = 0;
					if( 0 == g_srgTimeSchedule[iPrevSlot] ) g_srgTimeSchedule[iPrevSlot] = g_krgTaskList[iTaskID].irgParam[0];
					else
					{
						int iCount = 0;
						while(0 < g_srgTimeSchedule[iTimeSlot])
						{
							if( eTS_MaxSlot < iCount ) break;
							++iCount;
							++iTimeSlot;
							if( eTS_MaxSlot <= iTimeSlot ) iTimeSlot = 0;
						}
						g_srgTimeSchedule[iTimeSlot] = g_krgTaskList[iTaskID].irgParam[0];
					}
				}
				else g_srgTimeSchedule[iTimeSlot] = g_krgTaskList[iTaskID].irgParam[0];
			}
			break;
		case eTNAct_RemoveEventOnScheduler :
			{
				for( int i = 0; i < eTS_MaxSlot; ++i )
					if( g_krgTaskList[iTaskID].irgParam[0] == g_srgTimeSchedule[i] ) g_srgTimeSchedule[i] = 0;
			}
			break;

		case eTNAct_AddInstantEventOnScheduler :
			{
				unsigned int uiGap = g_krgTaskList[iTaskID].irgParam[1] * 1000;
				g_kScheduler.AddSchedule( CurrentTime, uiGap, 0, g_krgTaskList[iTaskID].irgParam[0] );
			}
			break;

		case eTNAct_Speak :
			{
				if( MAX_USER > a_iNPC || MAX_MOB <= a_iNPC ) return eTNRes_EvntInvalidNPCHandle;
				pMob[a_iNPC].Speak( g_krgTaskList[iTaskID].irgParam[0], a_iUser, 0 );
			}
			break;
		case eTNAct_Help :
			{
				if( MAX_USER > a_iNPC || MAX_MOB <= a_iNPC ) return eTNRes_EvntInvalidNPCHandle;
				pMob[a_iNPC].CallOthers( eTNGrp_Help, a_iUser );
			}
			break;
		case eTNAct_Link :
			{
				if( MAX_USER > a_iNPC || MAX_MOB <= a_iNPC ) return eTNRes_EvntInvalidNPCHandle;
				pMob[a_iNPC].CallOthers( eTNGrp_Link, a_iUser );
			}
			break;
		case eTNAct_Flee :
			{
				if( MAX_USER > a_iNPC || MAX_MOB <= a_iNPC ) return eTNRes_EvntInvalidNPCHandle;
				pMob[a_iNPC].LetsFlee( pMob[a_iNPC].CurrentTarget );
			}
			break;

		case eTNAct_UseSkill :
			{ // Ư�� NPC�� event�� trigger�� user���� skill�� ����. ���� user�� npc handle�� ��� �����ؾ��Ѵ�.
				if( 0 >= a_iUser || MAX_USER <= a_iUser ) return eTNRes_EvntInvalidUserHandle;
				short sSkillID = g_krgTaskList[iTaskID].irgParam[0];
				if( 0 < sSkillID )
				{
					if( MAX_USER > a_iNPC || MAX_MOB <= a_iNPC ) return eTNRes_EvntInvalidNPCHandle;
					if( pMob[a_iNPC].set_Skill( sSkillID ) ) return eTNRes_EvntInvalidSkillID;
					pMob[a_iNPC].CurrentTarget = a_iUser;
					pMob[a_iNPC].UseSkill();
				}
			}
			break;
		case eTNAct_ReturnPrevPos :
			{
				if( 0 >= a_iUser || MAX_USER <= a_iUser ) continue;
				HS2D_COORD kPos, kDest;
				kPos.x = pMob[a_iUser].TargetX;
				kPos.y = pMob[a_iUser].TargetY;
				kDest.x = pMob[a_iUser].LastX;
				kDest.y = pMob[a_iUser].LastY;
				
				int iWidth, iHeight;
				int iCorrect = 1;
				int iDist = CalDistance( kPos, kDest, 0, 0 );
				if( 2 > iDist ) iCorrect = 4; // ��ġ�� 4�� �ڷ�
				else if( 3 > iDist ) iCorrect = 3; // ��ġ�� 3�� �ڷ�
				else if( 4 > iDist ) iCorrect = 2; // ��ġ�� 2�� �ڷ�

				if( 1 < iCorrect )
				{
					iWidth = kDest.x - kPos.x;					
					iHeight = kDest.y - kPos.x;
					iWidth *= iCorrect;
					iHeight *= iCorrect;
					kDest.x += iWidth;
					kDest.y += iHeight;
				}

				if	( kDest.x<0 || kDest.y<0 || kDest.x>MAX_GRIDX || kDest.y>MAX_GRIDY ) continue;

				MSG_Action nm; nm.wType = _MSG_Action;
				nm.wPDULength = sizeof(nm)-sizeof(HEADER);
				nm.TargetX = kDest.x; nm.TargetY = kDest.y;
				nm.dwKeyID = a_iUser;
				nm.PosX=0; nm.PosY=0; nm.Direction = 0; 
				nm.Effect = 6; // knock-back motion

				pUser[a_iUser].cSock.AddMessage( (char*)&nm, sizeof(nm) );
				pUser[a_iUser].cSock.SendMessage();
			}
			break;
		case eTNAct_KnockBack :
			break;
		case eTNAct_Teleport :
			{ // player�� Ư�� cell�� ��ġ�� �ϰԵǸ�, target���� �Ÿ��� ������, �̵�(teleport type)���� ó���� �Ѵ�.
				if( 0 >= a_iUser || MAX_USER <= a_iUser ) return eTNRes_EvntInvalidUserHandle;
				//if( 0 < g_krgTaskList[iTaskID].irgParam[0] ) // Ư�� clan�� ����Ѵ�. 0�� ���� ��� permit
				//	if( g_krgTaskList[iTaskID].irgParam[0] != pMob[a_iUser].m_byClan ) return;

				HS2D_COORD kCell, kDest;
				kCell.x = a_iX;
				kCell.y = a_iY;
				kDest.x = g_krgTaskList[iTaskID].irgParam[1]; //pMob[a_iUser].TargetX;
				kDest.y = g_krgTaskList[iTaskID].irgParam[2]; // pMob[a_iUser].TargetY;
				int iDist = CalDistance( kCell, kDest, 0, 0 );
				if( 100 >= iDist )
				{
					pUser[a_iUser].nPreX = kDest.x;
					pUser[a_iUser].nPreY = kDest.y;

					MSG_Action nm; nm.wType = _MSG_Action;
					nm.wPDULength = sizeof(nm)-sizeof(HEADER);
					nm.TargetX=kDest.x; nm.TargetY= kDest.y;
					nm.dwKeyID = a_iUser;
					nm.PosX=0; nm.PosY=0; nm.Direction = 0; 
					nm.Effect = g_krgTaskList[iTaskID].irgParam[0]; // �̵� ����, // 0:�ɱ�  1:����  2:�ȱ�  3:�ٱ�  4:����  5:�ڷ���Ʈ,	6:�и���(knock-back), 7:�̲�������(�̵��ִϾ���)  8:����, 9:�λ�, 10:���� 

					pUser[a_iUser].cSock.AddMessage( (char*)&nm, sizeof(nm) );
					pUser[a_iUser].cSock.SendMessage();
				}
				else Teleport( a_iUser, kDest.x, kDest.y );
			}
			break;
		case eTNAct_TeleportParty :
			{
				int iLeader = pMob[a_iUser].Leader ;
				if( 0 >= iLeader || MAX_USER <= iLeader ) iLeader = a_iUser ; // �ڽ��� �����̴�. 

				// ��Ƽ���� ��ȯ
				for( int d = 0 ; d < MAX_PARTY+1 ; ++d )
				{   
					int fol = 0;
					if(d==0) fol = iLeader;
					else fol = pMob[iLeader].m_irgParty[d-1] ;
					if( 0 > fol || MAX_USER <= fol ) continue ;					
					Teleport( fol, g_krgTaskList[iTaskID].irgParam[0], g_krgTaskList[iTaskID].irgParam[1]);
				}		
			}
			break;
		case eTNAct_RangeUp :
			{

			}
			break;
		case eTNAct_Mine :
			{// mine�� ��޿� ���� event ID�� Ʋ����. 5����� mine�� �����Ѵٸ�, event ID�� 5�����̸� �� ���̴�.

			}
			break; 
		case eTNAct_PopMonster :
			{// 0:monster ID, 1:x, 2:y, 3:pop type, 4:direction, 5:clan
				int iSummoner = 0;
				int iClan = g_krgTaskList[iTaskID].irgParam[5];
				if( eTNClan_NoTrimuritiy >= iClan || eTNClan_NPC < iClan ) iClan = eTNClan_Aggressive;
				if( MAX_USER <= a_iNPC && MAX_MOB > a_iNPC ) iSummoner = a_iNPC;
				int iMonsterHandle = pMob[iSummoner].Summon( g_krgTaskList[iTaskID].irgParam[0], 1, g_krgTaskList[iTaskID].irgParam[3], eTNCls_Warrior, 0, iClan, g_krgTaskList[iTaskID].irgParam[4] , g_krgTaskList[iTaskID].irgParam[1], g_krgTaskList[iTaskID].irgParam[2], pMob[iSummoner].CurrentTarget, false, 0, 0, 60 );
			}
			break;
		case eTNAct_PopMonster2 :
			{// 0: monster ID, 1:class1, 2:clan, 3:direction, 4:x, 5:y
				int iMonsterHandle = pMob[1000].Summon( g_krgTaskList[iTaskID].irgParam[0], 1, eTNPrdt_PopNormal, g_krgTaskList[iTaskID].irgParam[1], 0, g_krgTaskList[iTaskID].irgParam[2], g_krgTaskList[iTaskID].irgParam[3] , g_krgTaskList[iTaskID].irgParam[4], g_krgTaskList[iTaskID].irgParam[5], 0, false, 0, 0, 61 );
			}
			break;
		case eTNAct_KillMonster : // ��ǥ ������ �ִ� Ư�� ID ���͸� ���δ�.
			{
				int iPosX = g_krgTaskList[iTaskID].irgParam[0];
				int iPosY = g_krgTaskList[iTaskID].irgParam[1];

				// ������ ��ǥ�� ���̷��� ���Ͱ� �ִ� ���
				int iMonsterHandle = pMobGrid[iPosY][iPosX];				
				if( pMob[iMonsterHandle].MOB.snTribe == g_krgTaskList[iTaskID].irgParam[2] ) KillMonster( iMonsterHandle );

				int iMaxIndex = g_pDetectEnemyRadius[eRds_MaxDetectEnemy];
				int x = 0, y = 0;			
				for( int i = 0; i < iMaxIndex; ++i )
				{
					x = iPosX + g_pDetectEnemyTable[i][0];
					y = iPosY + g_pDetectEnemyTable[i][1];
					if	( x<0 || y<0 || x>MAX_GRIDX || y>MAX_GRIDY ) continue;
					iMonsterHandle = pMobGrid[y][x];
					if( MAX_USER > iMonsterHandle || MAX_MOB <= iMonsterHandle ) continue;
					if( pMob[iMonsterHandle].MOB.snTribe == g_krgTaskList[iTaskID].irgParam[2] )
					{
						KillMonster( iMonsterHandle );
					}
				}
			}
			break;
		case eTNAct_KillMonsterAll : // Ư�� ID�� ���͸� ��� ���δ�.
			{//0:tribe
				for( int i = MAX_USER; i < MAX_MOB; ++i )
				{
					if( pMob[i].MOB.snTribe == g_krgTaskList[iTaskID].irgParam[0] )
					{
						KillMonster( i );
					}
				}
			}
			break;
		case eTNAct_ChangeClanByTribe :
			{
				for( int i = MAX_USER; i < MAX_MOB; ++i )
				{
					if( pMob[i].MOB.snTribe == g_krgTaskList[iTaskID].irgParam[0] )
					{
						pMob[i].MOB.byTrimuriti = pMob[i].m_byClan = g_krgTaskList[iTaskID].irgParam[1];
						pMob[i].ClearCurrentTarget();
						pMob[i].ClearAttacker( 0 );
						pMob[i].Mode = MOB_PEACE;
					}
				}
			}
			break;

		case eTNAct_ChangeClanByTribe2 :
			{
				int iSiege = g_kSiege.get_Started();
				if( iSiege ) continue;

				for( int i = MAX_USER; i < MAX_MOB; ++i )
				{
					if( pMob[i].MOB.snTribe == g_krgTaskList[iTaskID].irgParam[0] )
					{
						pMob[i].MOB.byTrimuriti = pMob[i].m_byClan = g_krgTaskList[iTaskID].irgParam[1];
						pMob[i].ClearCurrentTarget();
						pMob[i].ClearAttacker( 0 );
						pMob[i].Mode = MOB_PEACE;
					}
				}
			}
			break;

		case eTNAct_AffectAEffectToClanMonsters :
			{
				for( int i = MAX_USER; i < MAX_MOB; ++i )
				{
					if( eTNCls_NPC == pMob[i].MOB.byClass1 ) continue;
					if( eTNCls_Event == pMob[i].MOB.byClass1 ) continue;

					if( pMob[i].m_byClan == g_krgTaskList[iTaskID].irgParam[0] ) 
					{
						TNEFFECT kEffect;
						kEffect.iID = g_krgTaskList[iTaskID].irgParam[1];
						kEffect.kFunc.iData = g_krgTaskList[iTaskID].irgParam[2];
						kEffect.iDuration = g_krgTaskList[iTaskID].irgParam[3];
						kEffect.iParam1 = g_krgTaskList[iTaskID].irgParam[4];
						kEffect.iParam2 = g_krgTaskList[iTaskID].irgParam[5];
						pMob[i].AddEffect( kEffect, i, i );
						pMob[i].BroadcastUpdateStatusMsg();
					}
				}
			}
			break;
		case eTNAct_GambleResetMomey:
			{
#ifdef __YUT_LOG__
				sprintf(temp, "gamble GambleMoney Reset \r\n"); TimeWriteLog(temp,".\\LOG\\[Log]Yut.txt");
#endif	//	#ifdef __YUT_LOG__
				g_Yut.HT_ResetMoney();
			}	break;
		case eTNAct_GambleBetOn:		//	������ ���ý���
			{
#ifdef __YUT_LOG__
				sprintf(temp, "gamble GambleBetOn \r\n"); TimeWriteLog(temp,".\\LOG\\[Log]Yut.txt");
#endif	//	#ifdef __YUT_LOG__
				g_Yut.HT_SetBet(1);
				g_Yut.HT_Init();
			}	break;
		case eTNAct_GambleBetOff:		//	������ ��������
			{
#ifdef __YUT_LOG__
				sprintf(temp, "gamble GambleBetOff \r\n"); TimeWriteLog(temp,".\\LOG\\[Log]Yut.txt");
#endif	//	#ifdef __YUT_LOG__
				g_Yut.HT_SetBet(0);
			}	break;
		case eTNAct_GambleSetOn:		//	������ ���ý���
			{
#ifdef __YUT_LOG__
				sprintf(temp, "gamble GambleSetOn \r\n"); TimeWriteLog(temp,".\\LOG\\[Log]Yut.txt");
#endif	//	#ifdef __YUT_LOG__
			}	break;
		case eTNAct_GambleSetOff:		//	������ ��������
			{
#ifdef __YUT_LOG__
				sprintf(temp, "gamble GambleSetOff \r\n"); TimeWriteLog(temp,".\\LOG\\[Log]Yut.txt");
#endif	//	#ifdef __YUT_LOG__
			}	break;
		case eTNAct_GambleResultOn:		//	������ �������
			{
				g_Yut.HT_ShareMoney();
#ifdef __YUT_LOG__
				sprintf(temp, "gamble GambleResultOn \r\n"); TimeWriteLog(temp,".\\LOG\\[Log]Yut.txt");
#endif	//	#ifdef __YUT_LOG__
			}	break;
		case eTNAct_GambleResultOff:	//	������ �������
			{
#ifdef __YUT_LOG__
				sprintf(temp, "gamble GambleResultOff \r\n"); TimeWriteLog(temp,".\\LOG\\[Log]Yut.txt");
#endif	//	#ifdef __YUT_LOG__
			}	break;
		case eTNAct_GamblePlayOn:		//	������ ��������(�������� �����Ų��)
			{
#ifdef __YUT_LOG__
				sprintf(temp, "gamble GamblePlayOn \r\n"); TimeWriteLog(temp,".\\LOG\\[Log]Yut.txt");
				for(int x=653; x<678; x++)
				{
					for(int y=407; y<432; y++)
					{
						if(pMobGrid[y][x]==0) continue;
						sprintf(temp, "YutGrid[%d][%d] - ID:%d, MobPosX:%d, MobPosX:%d HP:%d, Mode:%d \r\n", y, x, pMobGrid[y][x], pMob[pMobGrid[y][x]].TargetX, pMob[pMobGrid[y][x]].TargetY, pMob[pMobGrid[y][x]].MOB.nHP, pMob[pMobGrid[y][x]].Mode); 
						TimeWriteLog(temp,".\\LOG\\[Log]Yut.txt");
					}
				}
#endif	//	#ifdef __YUT_LOG__
				//g_Yut.m_iYutID = 


				g_Yut.HT_SetPlay(1);
				YutCounter = 225;		//	15�� ���� ����ȴ�.
				//YutTimer = 0;
			}	break;
		case eTNAct_GamblePlayOff:		//	������ ��������
			{
				g_Yut.HT_SetPlay(0);
#ifdef __YUT_LOG__
				sprintf(temp, "gamble GamblePlayOff \r\n"); TimeWriteLog(temp,".\\LOG\\[Log]Yut.txt");
				for(int x=653; x<678; x++)
				{
					for(int y=407; y<432; y++)
					{
						if(pMobGrid[y][x]==0) continue;
						sprintf(temp, "YutGrid[%d][%d] - ID:%d, MobPosX:%d, MobPosX:%d HP:%d, Mode:%d \r\n", y, x, pMobGrid[y][x], pMob[pMobGrid[y][x]].TargetX, pMob[pMobGrid[y][x]].TargetY, pMob[pMobGrid[y][x]].MOB.nHP, pMob[pMobGrid[y][x]].Mode); 
						TimeWriteLog(temp,".\\LOG\\[Log]Yut.txt");
					}
				}
#endif	//	#ifdef __YUT_LOG__

			}	break;
		case eTNAct_GambleBroadcast:	//	������ ���� ����
			{
				short snX,snY=0;

				WORD wMsg = g_krgTaskList[iTaskID].irgParam[0]; 
				WORD wNPC = g_krgTaskList[iTaskID].irgParam[1];

#ifdef __YUT_LOG__
				sprintf(temp, "Yut_Broadcast NPCID:%d, msgID:%d \r\n", wNPC, wMsg);
				TimeWriteLog(temp,".\\LOG\\[Log]Yut.txt");
#endif	//	#ifdef __YUT_LOG__

				if(!g_ParamMgr.HT_bGetNPCPosition( wNPC, &snX, &snY )) continue;		//	�ش� NPC������ ���� ���Ұ��.

				MSG_Broadcast sm; sm.wType = _MSG_Broadcast;						//	���� �����ǰ���
				sm.byMsgType = MESSAGE_EXTRA;
				sm.byTextColor = 12;
				sm.wPDULength = sizeof(MSG_Broadcast) - sizeof(HEADER);
				strncpy(sm.szMsg, g_pMessageStringTable[wMsg], sizeof(sm.szMsg));
				strncpy(sm.szName, g_pMessageStringTable[_YutNPCName], sizeof(sm.szName)); 
				SendToAll((MSG_STANDARD*)&sm);

				S_SCP_NOTIFY_CHAT nm;	nm.wType = SCP_NOTIFY_CHAT;					//	NPC Chat
				nm.wPDULength = sizeof(S_SCP_NOTIFY_CHAT) - sizeof(HEADER);
				nm.nID=10000+wNPC;													//	NPC + 10000(NPC�� ��� +10000�ϱ����)
				nm.byTextColor=12; nm.byBgColor = 0; nm.byTrimuriti=0;				//	���� �������� ����
				strncpy(nm.szMsg, g_pMessageStringTable[wMsg], sizeof(nm.szMsg));
				GridMulticast(snX, snY, (MSG_STANDARD*)&nm, 0, 200);
			}	break;
		case etnAct_MoveGambleHorse :
			{
				Teleport( g_iGambleHorse, g_krgTaskList[iTaskID].irgParam[0], g_krgTaskList[iTaskID].irgParam[1] );
			}
			break;
		case eTNAct_KillMonsterWithClan : // Ư�� clan monster���� ��� ���δ�.
			{// 0:clan
				for( int i = MAX_USER; i < MAX_MOB; ++i )
				{
					if( eTNCls_NPC == pMob[i].MOB.byClass1 ) continue;
					if( eTNCls_Event == pMob[i].MOB.byClass1 ) continue;

					if( pMob[i].m_byClan == g_krgTaskList[iTaskID].irgParam[0] ) 
					{
						KillMonster( i );
					}
				}
			}
			break;
		case eTNAct_KillPC :
			{
				if( 0 >= a_iUser || MAX_USER <= a_iUser ) continue;
				pMob[a_iUser].MOB.nHP = 0;
				pMob[a_iUser].CurrentTarget = 0;
				pMob[a_iUser].m_eFSM = eTNFsm_Dead;
				pMob[a_iUser].NotifyUpdateStatusMsg();

				int iPosX = pMob[a_iUser].TargetX;
				int iPosY = pMob[a_iUser].TargetY;

				if( eZone_Chaturanka == g_iZoneID )
				{
					//a_iNPC�� ã�ƾ� �Ѵ�.
					int iMonsterHandle = 0;
					int iMaxIndex = g_pDetectEnemyRadius[eRds_MaxDetectEnemy];
					int x = 0, y = 0;			
					for( int i = 0; i < iMaxIndex; ++i )
					{
						x = iPosX + g_pDetectEnemyTable[i][0];
						y = iPosY + g_pDetectEnemyTable[i][1];
						if	( x<0 || y<0 || x>MAX_GRIDX || y>MAX_GRIDY ) continue;
						iMonsterHandle = pMobGrid[y][x];
						if( MAX_USER > iMonsterHandle || MAX_MOB <= iMonsterHandle ) continue;
						if( 2934 <= pMob[iMonsterHandle].MOB.snTribe && 2937 >= pMob[iMonsterHandle].MOB.snTribe )
						{
							a_iNPC = iMonsterHandle;
							pMob[a_iUser].OnKilled( a_iNPC, 8 );
							break;
						}
					}
				}
				else
				{
					pMob[a_iUser].OnKilled( a_iNPC, 8 );
				}
				
			}
			break;
		case eTNAct_Vanish :
			{
				KillMonster( a_iNPC );
			}
		case eTNAct_KillMonsterInSquare :
			{
				int iStartX, iEndX, iStartY, iEndY;

				iStartX = g_krgTaskList[iTaskID].irgParam[1];
				iEndX = g_krgTaskList[iTaskID].irgParam[0];
				if( g_krgTaskList[iTaskID].irgParam[0] < g_krgTaskList[iTaskID].irgParam[1] )
				{
					iStartX = g_krgTaskList[iTaskID].irgParam[0];
					iEndX = g_krgTaskList[iTaskID].irgParam[1];
				}

				iStartY = g_krgTaskList[iTaskID].irgParam[3];
				iEndY = g_krgTaskList[iTaskID].irgParam[2];
				if( g_krgTaskList[iTaskID].irgParam[2] < g_krgTaskList[iTaskID].irgParam[3] )
				{
					iStartY = g_krgTaskList[iTaskID].irgParam[2];
					iEndY = g_krgTaskList[iTaskID].irgParam[3];
				}

				for( int y = iStartY; y < iEndY; ++y )
				{
					for( int x = iStartX; x < iEndX; ++x )
					{
						int iMob = pMobGrid[y][x];
						if( MAX_USER > iMob || MAX_MOB <= iMob ) continue;
						if( eTNCls_NPC == pMob[iMob].MOB.byClass1 ) continue;
						if( eTNCls_Event == pMob[iMob].MOB.byClass1 ) continue;

						KillMonster( iMob );
					}
				}
			}
			break;

		case eTNAct_KillAll :
			{
				int iStartX, iEndX, iStartY, iEndY;

				iStartX = g_krgTaskList[iTaskID].irgParam[1];
				iEndX = g_krgTaskList[iTaskID].irgParam[0];
				if( g_krgTaskList[iTaskID].irgParam[0] < g_krgTaskList[iTaskID].irgParam[1] )
				{
					iStartX = g_krgTaskList[iTaskID].irgParam[0];
					iEndX = g_krgTaskList[iTaskID].irgParam[1];
				}

				iStartY = g_krgTaskList[iTaskID].irgParam[3];
				iEndY = g_krgTaskList[iTaskID].irgParam[2];
				if( g_krgTaskList[iTaskID].irgParam[2] < g_krgTaskList[iTaskID].irgParam[3] )
				{
					iStartY = g_krgTaskList[iTaskID].irgParam[2];
					iEndY = g_krgTaskList[iTaskID].irgParam[3];
				}

				for( int y = iStartY; y < iEndY; ++y )
				{
					for( int x = iStartX; x < iEndX; ++x )
					{
						int iMob = pMobGrid[y][x];
						if( 0 >= iMob || MAX_MOB <= iMob ) continue;
						if( eTNClan_GM == pMob[iMob].m_byClan ) continue; //  GM�� ���� �̵���Ű�� �ʴ´�.
						if( eTNCls_NPC == pMob[iMob].MOB.byClass1 ) continue;
						if( eTNCls_Event == pMob[iMob].MOB.byClass1 ) continue;

						if( eTNMob_PC == pMob[iMob].m_eMobType )
						{
							pMob[iMob].MOB.nHP = 0;
							pMob[iMob].NotifyUpdateStatusMsg();

							pMob[iMob].OnKilled( a_iNPC, 9 );
						}
						else
						{
							KillMonster( iMob );
						}
					}
				}
			}
			break;
		case eTNAct_CheckKalaSystem :
			{
				CheckKalaSystem();
			}
			break;
		case eTNAct_PrintKalaCoreInfo :
			{
				MoveKalaRewarder();
			}
			break;
		case eTNAct_DrawCard: // deck
			{
				int iDeck = g_krgTaskList[iTaskID].irgParam[0];
				int iEvent = g_krgEventDeck[iDeck].Random();

				TriggerEvent( a_iUser, iEvent, a_iX, a_iY, a_iNPC, 1205 );		
			}
			break;
		case eTNAct_KickOut :
			{
				int iStartX, iEndX, iStartY, iEndY;

				iStartX = g_krgTaskList[iTaskID].irgParam[1];
				iEndX = g_krgTaskList[iTaskID].irgParam[0];
				if( g_krgTaskList[iTaskID].irgParam[0] < g_krgTaskList[iTaskID].irgParam[1] )
				{
					iStartX = g_krgTaskList[iTaskID].irgParam[0];
					iEndX = g_krgTaskList[iTaskID].irgParam[1];
				}

				iStartY = g_krgTaskList[iTaskID].irgParam[3];
				iEndY = g_krgTaskList[iTaskID].irgParam[2];
				if( g_krgTaskList[iTaskID].irgParam[2] < g_krgTaskList[iTaskID].irgParam[3] )
				{
					iStartY = g_krgTaskList[iTaskID].irgParam[2];
					iEndY = g_krgTaskList[iTaskID].irgParam[3];
				}
				for( int y = iStartY; y < iEndY; ++y )
				{
					for( int x = iStartX; x < iEndX; ++x )
					{
						int iMob = pMobGrid[y][x];
						if( 0 >= iMob || MAX_MOB <= iMob ) continue;
						if( eTNClan_GM == pMob[iMob].m_byClan ) continue; //  GM�� ���� �̵���Ű�� �ʴ´�.

						if( eTNMob_PC == pMob[iMob].m_eMobType )
						{// save �������� ���ư���. �������� zone�� �����ϱ� user���� ��� ������ �������� �Ѵ�.
							ReturnPCToSaveZone( iMob );
						}
					}
				}
			}
			break;
		case eTNAct_CountRealmEntry :
			{ // ������ �ֽ��� �����ο����� ����Ѵ�.
				int iCountCheckSum = 10000;
				if( (iCountCheckSum < g_irgEntryCount[eTNClan_Siva]) && (iCountCheckSum < g_irgEntryCount[eTNClan_Brahma]) && (iCountCheckSum < g_irgEntryCount[eTNClan_Vishnu]) )
				{
					g_irgEntryCount[eTNClan_Siva] -= iCountCheckSum;
					g_irgEntryCount[eTNClan_Brahma] -= iCountCheckSum;
					g_irgEntryCount[eTNClan_Vishnu] -= iCountCheckSum;
				}

				for( int i = 1; i < MAX_USER; ++i )
				{
					if( MOB_EMPTY == pMob[i].Mode ) continue;
					if( USER_PLAY == pUser[i].Mode )
					{
						int iClan = pMob[i].m_byClan;
						if( 1 > iClan || 4 < iClan ) continue;
						++g_irgEntryCount[iClan];
					}
				}

				g_irgEntryCount[0] = g_irgEntryCount[eTNClan_Brahma] + g_irgEntryCount[eTNClan_Vishnu] + g_irgEntryCount[eTNClan_Siva];

				#ifdef __TN_EMERGENCY_LOG__
				{
					//SYSTEMTIME st;
					//GetLocalTime( &st );

					char chBuf[512] = { 0,0,0, };
					sprintf(chBuf, "%[dyy%dmm%ddd %dhh%dmi%dss] eTNAct_CountRealmEntry >  Total: %d, brama:%d, vishunu:%d, siva:%d \r\n"
						, g_kSystemTime.wYear, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
						, g_irgEntryCount[0], g_irgEntryCount[eTNClan_Brahma], g_irgEntryCount[eTNClan_Vishnu], g_irgEntryCount[eTNClan_Siva] );
					WriteLog( chBuf, g_szrgLogFileName[eLogFileName_RvREntry] );
				}
				#endif //__TN_EMERGENCY_LOG__
			}
			break;
		case eTNAct_PostMessageToZone :
			{
				if( 0 > g_krgTaskList[iTaskID].irgParam[0] || MAX_STRING <= g_krgTaskList[iTaskID].irgParam[0] ) break;
				PostMessageToZone( g_pMessageStringTable[g_krgTaskList[iTaskID].irgParam[0]], eTNClr_White, eTNClr_BG, g_krgTaskList[iTaskID].irgParam[0] );
			}
			break;
		case eTNAct_PostMessageToWorld :
			{
				if( 0 > g_krgTaskList[iTaskID].irgParam[0] || MAX_STRING <= g_krgTaskList[iTaskID].irgParam[0] ) break;
				PostMessageToWorld( g_pMessageStringTable[g_krgTaskList[iTaskID].irgParam[0]], eTNClr_White, eTNClr_BG, g_krgTaskList[iTaskID].irgParam[0] );
			}
			break;
		case eTNAct_RecordName :
			{// Ư�� �簢 ���� ���� ��� PC name�� ����Ѵ�.
				int iStartX, iEndX, iStartY, iEndY;
				// Param0 : �� X
				// Param1 : ���� X
				// Param2 : �� Y
				// Param3 : ���� Y
				// Param4 : ��ǥ X
				// Param5 : ��ǥ Y

				iStartX = g_krgTaskList[iTaskID].irgParam[1];
				iEndX = g_krgTaskList[iTaskID].irgParam[0];
				if( g_krgTaskList[iTaskID].irgParam[0] < g_krgTaskList[iTaskID].irgParam[1] )
				{
					iStartX = g_krgTaskList[iTaskID].irgParam[0];
					iEndX = g_krgTaskList[iTaskID].irgParam[1];
				}

				iStartY = g_krgTaskList[iTaskID].irgParam[3];
				iEndY = g_krgTaskList[iTaskID].irgParam[2];
				if( g_krgTaskList[iTaskID].irgParam[2] < g_krgTaskList[iTaskID].irgParam[3] )
				{
					iStartY = g_krgTaskList[iTaskID].irgParam[2];
					iEndY = g_krgTaskList[iTaskID].irgParam[3];
				}

				#ifdef __TN_TOP_LOG__
				{
					//SYSTEMTIME st;
					//GetLocalTime( &st );
					char chBuf[512] = { 0,0,0, };
					sprintf(chBuf, "\r\n\n\n[%dMM%dDD %dH%dM%dS] Record PCs in the event(%d)\r\n", g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond, a_iEventID );
					WriteLog( chBuf, g_szrgLogFileName[eLogFileName_RecordPCName] );
				}
				#endif

				for( int y = iStartY; y < iEndY; ++y )
				{
					for( int x = iStartX; x < iEndX; ++x )
					{
						if( x<0 || y<0 || x>MAX_GRIDX || y>MAX_GRIDY ) continue;
						int iMob = pMobGrid[y][x];
						if( 0 >= iMob || MAX_USER <= iMob ) continue;
						if( eTNClan_GM == pMob[iMob].m_byClan ) continue; //  GM�� ���� skip

						if( (eTNMob_PC == pMob[iMob].m_eMobType) && (0 < pMob[iMob].MOB.nHP) )
						{
							#ifdef __TN_TOP_LOG__
							{
								char chBuf[256] = { 0,0,0, };
								sprintf(chBuf, "\t- %s\r\n", pMob[iMob].MOB.szName );
								WriteLog( chBuf, g_szrgLogFileName[eLogFileName_RecordPCName] );
							}
							#endif
						}
					}
				}
			}
			break;
		case eTNAct_RecordPCNameInZone :
			{
				#ifdef __TN_TOP_LOG__
				{
					//SYSTEMTIME st;
					//GetLocalTime( &st );
					char chBuf[256] = { 0,0,0, };
					sprintf(chBuf, "\r\n\n[%dMM%dDD %dHH%dMS%dSS] eTNAct_RecordPCNameInZone>\r\n", g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond );
					WriteLog( chBuf, g_szrgLogFileName[eLogFileName_RecordPCName] );
				}
				#endif

				for( int iPC = 0; iPC < MAX_USER; ++iPC )
				{
					if( USER_PLAY != pUser[iPC].Mode ) continue;
					if( eTNClan_GM == pMob[iPC].m_byClan ) continue;
					if( eTNCls_NPC == pMob[iPC].MOB.byClass1 ) continue;

					#ifdef __TN_TOP_LOG__
					{
						char chBuf[256] = { 0,0,0, };
						sprintf(chBuf, "\t- %s, HP:%d \r\n", pMob[iPC].MOB.szName, pMob[iPC].MOB.nHP );
						WriteLog( chBuf, g_szrgLogFileName[eLogFileName_RecordPCName] );
					}
					#endif
				}
			}
			break;

		case eTNAct_PostStrongholdOwner :
			{
				char szMsg[1024] = { 0,0,0, };

				// �� 2���� ������ 8���� stronghold�� �ִ�. �ϵ��ʿ���� 2���� �ִ�. ��� �̸��� �������� �Ѵ�.
				// �θ��� �����̶�� �׷��µ�, �������� �ϳ��� zone�� �ϳ��� �������� �����ϰ� �־��µ�
				// ����� �ΰ��� zone�� �θ��� �����̶�� �θ��� �������� �߻��Ѵ�.
				if( 15 == ServerIndex ) // �θ��� 1����
				{
					if( eStronghold_Northeast == g_krgTaskList[iTaskID].irgParam[0] )
					{
						if( 0 >= g_krgStronghold[eStronghold_Northeast].iOwner )
							sprintf( szMsg, g_pMessageStringTable[_StrongholdIsNotCaptured], g_pMessageStringTable[_1NorthEasternStronghold] );
						else
						{
							sprintf( szMsg, g_pMessageStringTable[_StrongholdIsOccupied], g_pMessageStringTable[_1NorthEasternStronghold], g_krgStronghold[eStronghold_Northeast].szGuildName/*����*/ );
						}						
					}
					else if( eStronghold_Northwest == g_krgTaskList[iTaskID].irgParam[0] )
					{
						if( 0 >= g_krgStronghold[eStronghold_Northwest].iOwner )
							sprintf( szMsg, g_pMessageStringTable[_StrongholdIsNotCaptured], g_pMessageStringTable[_1NorthWesternStronghold] );
						else
						{
							//g_krgStronghold[eStronghold_Northeast].iOwner �� ���� guild ID�� �˻��ؾ� �Ѵ�.
							sprintf( szMsg, g_pMessageStringTable[_StrongholdIsOccupied], g_pMessageStringTable[_1NorthWesternStronghold], g_krgStronghold[eStronghold_Northwest].szGuildName/*����*/ );
						}
					}
					else if( eStronghold_Southeast == g_krgTaskList[iTaskID].irgParam[0] )
					{
						if( 0 >= g_krgStronghold[eStronghold_Southeast].iOwner )
							sprintf( szMsg, g_pMessageStringTable[_StrongholdIsNotCaptured], g_pMessageStringTable[_1SouthEasternStronghold] );
						else
						{
							//g_krgStronghold[eStronghold_Northeast].iOwner �� ���� guild ID�� �˻��ؾ� �Ѵ�.
							sprintf( szMsg, g_pMessageStringTable[_StrongholdIsOccupied], g_pMessageStringTable[_1SouthEasternStronghold], g_krgStronghold[eStronghold_Southeast].szGuildName/*����*/ );
						}
					}
					else if( eStronghold_Southwest == g_krgTaskList[iTaskID].irgParam[0] )
					{
						if( 0 >= g_krgStronghold[eStronghold_Southwest].iOwner )
							sprintf( szMsg, g_pMessageStringTable[_StrongholdIsNotCaptured], g_pMessageStringTable[_1SouthWesternStronghold] );
						else
						{
							//g_krgStronghold[eStronghold_Northeast].iOwner �� ���� guild ID�� �˻��ؾ� �Ѵ�.
							sprintf( szMsg, g_pMessageStringTable[_StrongholdIsOccupied], g_pMessageStringTable[_1SouthWesternStronghold], g_krgStronghold[eStronghold_Southwest].szGuildName/*����*/ );
						}
					}
				}
				else if( 16 == ServerIndex ) // �θ��� 2����
				{
					if( eStronghold_Northeast == g_krgTaskList[iTaskID].irgParam[0] )
					{
						if( 0 >= g_krgStronghold[eStronghold_Northeast].iOwner )
							sprintf( szMsg, g_pMessageStringTable[_StrongholdIsNotCaptured], g_pMessageStringTable[_1NorthEasternStronghold] );
						else
						{
							//g_krgStronghold[eStronghold_Northeast].iOwner �� ���� guild ID�� �˻��ؾ� �Ѵ�.
							sprintf( szMsg, g_pMessageStringTable[_StrongholdIsOccupied], g_pMessageStringTable[_1NorthEasternStronghold], g_krgStronghold[eStronghold_Northeast].szGuildName/*����*/ );
						}						
					}
					else if( eStronghold_Northwest == g_krgTaskList[iTaskID].irgParam[0] )
					{
						if( 0 >= g_krgStronghold[eStronghold_Northwest].iOwner )
							sprintf( szMsg, g_pMessageStringTable[_StrongholdIsNotCaptured], g_pMessageStringTable[_1NorthWesternStronghold] );
						else
						{
							//g_krgStronghold[eStronghold_Northeast].iOwner �� ���� guild ID�� �˻��ؾ� �Ѵ�.
							sprintf( szMsg, g_pMessageStringTable[_StrongholdIsOccupied], g_pMessageStringTable[_1NorthWesternStronghold], g_krgStronghold[eStronghold_Northwest].szGuildName/*����*/ );
						}
					}
					else if( eStronghold_Southeast == g_krgTaskList[iTaskID].irgParam[0] )
					{
						if( 0 >= g_krgStronghold[eStronghold_Southeast].iOwner )
							sprintf( szMsg, g_pMessageStringTable[_StrongholdIsNotCaptured], g_pMessageStringTable[_1SouthEasternStronghold] );
						else
						{
							//g_krgStronghold[eStronghold_Northeast].iOwner �� ���� guild ID�� �˻��ؾ� �Ѵ�.
							sprintf( szMsg, g_pMessageStringTable[_StrongholdIsOccupied], g_pMessageStringTable[_1SouthEasternStronghold], g_krgStronghold[eStronghold_Southeast].szGuildName/*����*/ );
						}
					}
					else if( eStronghold_Southwest == g_krgTaskList[iTaskID].irgParam[0] )
					{
						if( 0 >= g_krgStronghold[eStronghold_Southwest].iOwner )
							sprintf( szMsg, g_pMessageStringTable[_StrongholdIsNotCaptured], g_pMessageStringTable[_1SouthWesternStronghold] );
						else
						{
							//g_krgStronghold[eStronghold_Northeast].iOwner �� ���� guild ID�� �˻��ؾ� �Ѵ�.
							sprintf( szMsg, g_pMessageStringTable[_StrongholdIsOccupied], g_pMessageStringTable[_1SouthWesternStronghold], g_krgStronghold[eStronghold_Southwest].szGuildName/*����*/ );
						}
					}
				}

				{
					//SYSTEMTIME st;
					//GetLocalTime( &st );

					char chBuf[2048] = { 0,0,0, };
					sprintf(chBuf, "[eTNAct_PostStrongholdOwner] %d��%d��%d��%d��%d��, MSG(ID:%d) : %s\r\n", g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
						, g_krgTaskList[iTaskID].irgParam[0], szMsg );
					WriteLog( chBuf, g_szrgLogFileName[eLogFileName_EventTriggered] );
				}

				PostMessageToWorld( szMsg );
			}
			break;				   
		case eTNAct_KillKingAtChaturangka : // kill a king
			{
				for( int iHandle = MAX_USER; iHandle < MAX_MOB; ++iHandle )
				{
					if( (2113 == pMob[iHandle].MOB.snTribe) || (2112 == pMob[iHandle].MOB.snTribe) ) // 
					{
						if( 0 < pMob[iHandle].MOB.nHP ) // alive ���¿� �ִ� ���� ���� ���� �α׿� �����.
						{
							pMob[iHandle].MOB.nHP = 0; 
							pMob[iHandle].CurrentTarget = 0;
							DeleteMob( iHandle, 125, 0, eTNPrdt_RemoveNormal, 250 );

							pMob[iHandle].Mode = MOB_EMPTY;
							pMobGrid[pMob[iHandle].TargetY][pMob[iHandle].TargetX] = 0;


							#ifdef __TN_TOP_LOG__
							{
								//SYSTEMTIME st;
								//GetLocalTime( &st );
								char chBuf[512] = { 0,0,0, };
								sprintf(chBuf, "\r\n\n[%dmm%ddd %dhh%dms%dss] kill the king \r\n", g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond );
								WriteLog( chBuf, g_szrgLogFileName[eLogFileName_BossSystem] );
							}
							#endif    
						}
					}					
				}
			}
			break;
		case eTNAct_CloseKingRoomAtChaturangka : // close �������� �չ�
			{// ��� user�� ƨ�ܳ��� �Ѵ�. -> ���������� �� portal field�� teleport �����ش�. or �ֽ��� �������� ������?
				int iStartX, iEndX, iStartY, iEndY;
				iStartX = 371;
				iEndX = 523;
				iStartY = 497;
				iEndY = 652;

				#ifdef __TN_TOP_LOG__
				{
					//SYSTEMTIME st;
					//GetLocalTime( &st );
					char chBuf[512] = { 0,0,0, };
					sprintf(chBuf, "\r\n\n\n[%dmm%ddd %dhh%dms%dss] Close the zone of the king!\r\n", g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond );
					WriteLog( chBuf, g_szrgLogFileName[eLogFileName_BossSystem] );
				}
				#endif    
				int irgPopPos[4][2]  = { 868, 815, 833, 813, 833, 848, 868, 851 };
				int iIndex = 0;
				
				for( int y = iStartY; y < iEndY; ++y )
					for( int x = iStartX; x < iEndX; ++x )
					{
						int iMob = pMobGrid[y][x];
						if( 0 >= iMob || MAX_USER <= iMob ) continue;
						if( eTNClan_GM == pMob[iMob].m_byClan ) continue;

						iIndex = rand() % 4;
						Teleport( iMob, irgPopPos[iIndex][0], irgPopPos[iIndex][1] );
					}
			}
			break;
		case eTNAct_CheckMonster2 : // Ư�� ID�� ���Ͱ� ���� ������ �ִ��� �ľ��Ѵ�.
			{
				int iCount = 0;
				for( int iMob = MAX_USER; iMob < MAX_MOB; ++iMob )
				{
					if( pMob[iMob].MOB.snTribe == g_krgTaskList[iTaskID].irgParam[0] )
					{
						if( (0 < pMob[iMob].MOB.nHP) && (MOB_EMPTY!=pMob[iMob].Mode) ) ++iCount;
					}
				}		

				if( 0 == iCount ) iProceed = 0;
			}
			break;

		case eTNAct_CountMonster :
			{
				int iStartX, iEndX, iStartY, iEndY;

				iStartX = g_krgTaskList[iTaskID].irgParam[1];
				iEndX = g_krgTaskList[iTaskID].irgParam[0];
				if( g_krgTaskList[iTaskID].irgParam[0] < g_krgTaskList[iTaskID].irgParam[1] )
				{
					iStartX = g_krgTaskList[iTaskID].irgParam[0];
					iEndX = g_krgTaskList[iTaskID].irgParam[1];
				}

				iStartY = g_krgTaskList[iTaskID].irgParam[3];
				iEndY = g_krgTaskList[iTaskID].irgParam[2];
				if( g_krgTaskList[iTaskID].irgParam[2] < g_krgTaskList[iTaskID].irgParam[3] )
				{
					iStartY = g_krgTaskList[iTaskID].irgParam[2];
					iEndY = g_krgTaskList[iTaskID].irgParam[3];
				}

				int iMonsterCount = 0;
				for( int y = iStartY; y < iEndY; ++y )
				{
					for( int x = iStartX; x < iEndX; ++x )
					{
						int iMob = pMobGrid[y][x];
						if( MAX_USER > iMob || MAX_MOB <= iMob ) continue;
						if( eTNMob_NPC == pMob[iMob].m_eMobType )
						{
							if( (0 < pMob[iMob].MOB.nHP) && (MOB_EMPTY!=pMob[iMob].Mode) ) ++iMonsterCount;
						}
					}
				}

				if( g_krgTaskList[iTaskID].irgParam[4] < iMonsterCount )
				{
					iProceed = 0;
				}
			}
			break;
	   
		case eTNAct_CountMonsterByTribe :
			{
				int iStartX, iEndX, iStartY, iEndY;

				iStartX = g_krgTaskList[iTaskID].irgParam[1];
				iEndX = g_krgTaskList[iTaskID].irgParam[0];
				if( g_krgTaskList[iTaskID].irgParam[0] < g_krgTaskList[iTaskID].irgParam[1] )
				{
					iStartX = g_krgTaskList[iTaskID].irgParam[0];
					iEndX = g_krgTaskList[iTaskID].irgParam[1];
				}

				iStartY = g_krgTaskList[iTaskID].irgParam[3];
				iEndY = g_krgTaskList[iTaskID].irgParam[2];
				if( g_krgTaskList[iTaskID].irgParam[2] < g_krgTaskList[iTaskID].irgParam[3] )
				{
					iStartY = g_krgTaskList[iTaskID].irgParam[2];
					iEndY = g_krgTaskList[iTaskID].irgParam[3];
				}

				int iMonsterCount = 0;
				for( int y = iStartY; y < iEndY; ++y )
				{
					for( int x = iStartX; x < iEndX; ++x )
					{
						int iMob = pMobGrid[y][x];
						if( MAX_USER > iMob || MAX_MOB <= iMob ) continue;
						if( eTNMob_NPC == pMob[iMob].m_eMobType )
						{
							if( g_krgTaskList[iTaskID].irgParam[4] == pMob[iMob].MOB.snTribe )
								if( (0 < pMob[iMob].MOB.nHP) && (MOB_EMPTY!=pMob[iMob].Mode) ) ++iMonsterCount;
						}
					}
				}

				if( g_krgTaskList[iTaskID].irgParam[5] < iMonsterCount )
				{
					iProceed = 0;
				}
			}
			break;

		case eTNAct_TeleportAll :
			{ // Ư�� �簢 ���� ���� ��� PC�� ��ǥ�������� teleport��Ų��.
				int iStartX, iEndX, iStartY, iEndY;
				// Param0 : �� X
				// Param1 : ���� X
				// Param2 : �� Y
				// Param3 : ���� Y
				// Param4 : ��ǥ X
				// Param5 : ��ǥ Y

				iStartX = g_krgTaskList[iTaskID].irgParam[1];
				iEndX = g_krgTaskList[iTaskID].irgParam[0];
				if( g_krgTaskList[iTaskID].irgParam[0] < g_krgTaskList[iTaskID].irgParam[1] )
				{
					iStartX = g_krgTaskList[iTaskID].irgParam[0];
					iEndX = g_krgTaskList[iTaskID].irgParam[1];
				}

				iStartY = g_krgTaskList[iTaskID].irgParam[3];
				iEndY = g_krgTaskList[iTaskID].irgParam[2];
				if( g_krgTaskList[iTaskID].irgParam[2] < g_krgTaskList[iTaskID].irgParam[3] )
				{
					iStartY = g_krgTaskList[iTaskID].irgParam[2];
					iEndY = g_krgTaskList[iTaskID].irgParam[3];
				}

				for( int y = iStartY; y < iEndY; ++y )
				{
					for( int x = iStartX; x < iEndX; ++x )
					{
						int iMob = pMobGrid[y][x];
						if( 0 >= iMob || MAX_USER <= iMob ) continue;
						if( eTNClan_GM == pMob[iMob].m_byClan ) continue; //  GM�� ���� �̵���Ű�� �ʴ´�.

						if( eTNMob_PC == pMob[iMob].m_eMobType )
						{
							Teleport( iMob, g_krgTaskList[iTaskID].irgParam[4], g_krgTaskList[iTaskID].irgParam[5] );
							/*
							if( 0 >= pMob[iMob].MOB.nHP )
							{ // �׾� �ִ� ĳ���ʹ� save zone���� ������.
								MSG_MoveOtherZone sm; sm.wType=_MSG_MoveOtherZone;
								sm.byType=CONNECT_TYPE_PUSTICA; sm.snPositionID= pMob[iMob].m_snSaveNPC;
								pUser[iMob].cSock.SendOneMessage((char*)&sm, sizeof(sm));
							}
							else
							{ // PC�� �̵���Ų��.
								Teleport( iMob, g_krgTaskList[iTaskID].irgParam[4], g_krgTaskList[iTaskID].irgParam[5] );
							}
							*/
						}
					}
				}
			}
			break;
		case eTNAct_ReturnPCToSaveZone :
			{
				for( int i = 1; i < MAX_USER; ++i )
				{
					if( eTNClan_GM == pMob[i].m_byClan ) continue; //  GM�� ���� �̵���Ű�� �ʴ´�.
					ReturnPCToSaveZone( i );
				}
			}
			break;
		case eTNAct_ReturnToSaveZone :
			{
				//SYSTEMTIME st;
				//GetLocalTime( &st );

				/*
				char chBuf2[512] = { 0,0,0, };
				sprintf(chBuf2, "[eTNAct_ReturnToSaveZone] %d��%d��%d��%d��%d��, �ֽŴ��� ����. \r\n", g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond );
				WriteLog( chBuf2, g_szrgLogFileName[eLogFileName_EventTriggered] );

				{
					char chBuf[2048] = { 0,0,0, };
					sprintf(chBuf, "\n\n\n[eTNAct_ReturnToSaveZone] %d��%d��%d��%d��%d��\r\n", g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond );
					WriteLog( chBuf, ".\\Log\\[Log]ReturnToSaveZone.txt" );
				}
				*/

				for( int i = 1; i < MAX_USER; ++i )
				{
					if( USER_PLAY == pUser[i].Mode )
					{
						int iPortalID = 163;
						if( eTNClan_Brahma == pMob[i].m_byClan ) iPortalID = 161;
						else if( eTNClan_Vishnu == pMob[i].m_byClan ) iPortalID = 162;

						//{
						//	char chBuf[2048] = { 0,0,0, };
						//	sprintf(chBuf, "\t- %s\r\n", pMob[i].MOB.szName );
						//	WriteLog( chBuf, ".\\Log\\[Log]ReturnToSaveZone.txt" );
						//}

						S_SCP_RESP_MOVE_PORTAL sm; sm.wType=SCP_RESP_MOVE_PORTAL;

						bool bRet=true;
						DWORD dwServerID=0; short snStartX=0; short snStartZ=0; short snEndX=0; short snEndZ=0; DWORD dwZoneLevel=0;
						bRet &= g_ParamMgr.HT_bGetPortalInfo( iPortalID, &dwServerID, &dwZoneLevel, &snStartX, &snStartZ, &snEndX, &snEndZ );

						BYTE byMoveZone = (BYTE)( dwServerID - HT_MAP_START + 1 );
						//if( bRet && (ServerDown==-1000) )	//	�ٸ� ���̸�, ...
						if( bRet && (byMoveZone != (ServerIndex+1)) && (ServerDown==-1000) )	//	�ٸ� ���̸�, ...						
						{
							pMob[i].m_kWaitAction.iAction = eWaitAct_ZonePortal;

							sm.byResult=REPLY_MOVE_PORTAL_OUTAREA; 
							sm.byZone=byMoveZone; sm.nMoney=pMob[i].MOB.nRupiah;
							pUser[i].cSock.SendOneMessage((char*)&sm, sizeof(S_SCP_RESP_MOVE_PORTAL));
							
							pMob[i].m_eFSM = eTNFsm_Stand;
							pMob[i].m_kLastTime.uiSitted = 0;

							pMob[i].MOB.snX = GetRandom(snStartX, snEndX);
							pMob[i].MOB.snZ = GetRandom(snStartZ, snEndZ);

						}
					}
				}
			}
			break;
		case eTNAct_ChangeField :
			{ // Ư�� �簢 ���� �Ӽ��� �����Ѵ�.
				int iStartX, iEndX, iStartY, iEndY;
				// Param0 : �� X
				// Param1 : ���� X
				// Param2 : �� Y
				// Param3 : ���� Y
				// Param4 : ��ǥ X
				// Param5 : ��ǥ Y

				iStartX = g_krgTaskList[iTaskID].irgParam[1];
				iEndX = g_krgTaskList[iTaskID].irgParam[0];
				if( g_krgTaskList[iTaskID].irgParam[0] < g_krgTaskList[iTaskID].irgParam[1] )
				{
					iStartX = g_krgTaskList[iTaskID].irgParam[0];
					iEndX = g_krgTaskList[iTaskID].irgParam[1];
				}

				iStartY = g_krgTaskList[iTaskID].irgParam[3];
				iEndY = g_krgTaskList[iTaskID].irgParam[2];
				if( g_krgTaskList[iTaskID].irgParam[2] < g_krgTaskList[iTaskID].irgParam[3] )
				{
					iStartY = g_krgTaskList[iTaskID].irgParam[2];
					iEndY = g_krgTaskList[iTaskID].irgParam[3];
				}

				for( int y = iStartY; y < iEndY; ++y )
				{
					for( int x = iStartX; x < iEndX; ++x )
					{
						if( 1 == g_krgTaskList[iTaskID].irgParam[5] ) g_krgCell[y][x].usProperty = g_krgCell[y][x].usProperty | (unsigned short)(g_krgTaskList[iTaskID].irgParam[4]); // �߰�
						else if( 0 == g_krgTaskList[iTaskID].irgParam[5] )
						{
							unsigned short usNewProperty = (unsigned short)(g_krgTaskList[iTaskID].irgParam[4]);
							if( usNewProperty & g_krgCell[y][x].usProperty  )							
								g_krgCell[y][x].usProperty = g_krgCell[y][x].usProperty ^ usNewProperty; // XOR�� ����
						}
					}
				}
			}
			break;

		case eTNAct_AffectAll :
			{ // Ư�� �簢 ���� �Ӽ��� �����Ѵ�.
				int iStartX, iEndX, iStartY, iEndY;
				iStartX = g_krgTaskList[iTaskID].irgParam[1];
				iEndX = g_krgTaskList[iTaskID].irgParam[0];
				if( g_krgTaskList[iTaskID].irgParam[0] < g_krgTaskList[iTaskID].irgParam[1] )
				{
					iStartX = g_krgTaskList[iTaskID].irgParam[0];
					iEndX = g_krgTaskList[iTaskID].irgParam[1];
				}

				iStartY = g_krgTaskList[iTaskID].irgParam[3];
				iEndY = g_krgTaskList[iTaskID].irgParam[2];
				if( g_krgTaskList[iTaskID].irgParam[2] < g_krgTaskList[iTaskID].irgParam[3] )
				{
					iStartY = g_krgTaskList[iTaskID].irgParam[2];
					iEndY = g_krgTaskList[iTaskID].irgParam[3];
				}

				for( int y = iStartY; y < iEndY; ++y )
				{
					for( int x = iStartX; x < iEndX; ++x )
					{
						int iMob = pMobGrid[y][x];
						if( 0 >= iMob || MAX_USER <= iMob ) continue;
						if( eTNClan_GM == pMob[iMob].m_byClan ) continue; //  GM�� ���� �̵���Ű�� �ʴ´�.
						if( eTNMob_PC == pMob[iMob].m_eMobType )
						{
							if( 0 >= pMob[iMob].MOB.nHP ) continue;

							TNEFFECT kEffect;
							kEffect.iID = g_krgTaskList[iTaskID].irgParam[4];
							kEffect.iDuration = g_krgTaskList[iTaskID].irgParam[5];
							kEffect.iParam1 = 100;
							kEffect.iParam2 = 0;
							pMob[iMob].AddEffect( kEffect, iMob, iMob );
							pMob[iMob].BroadcastUpdateStatusMsg();
							pUser[iMob].cSock.SendMessage();
						}
					}
				}
			}
			break;

		case eTNAct_DebufferAll :
			{ // Ư�� �簢 ���� �Ӽ��� �����Ѵ�.
				int iStartX, iEndX, iStartY, iEndY;
				iStartX = g_krgTaskList[iTaskID].irgParam[1];
				iEndX = g_krgTaskList[iTaskID].irgParam[0];
				if( g_krgTaskList[iTaskID].irgParam[0] < g_krgTaskList[iTaskID].irgParam[1] )
				{
					iStartX = g_krgTaskList[iTaskID].irgParam[0];
					iEndX = g_krgTaskList[iTaskID].irgParam[1];
				}

				iStartY = g_krgTaskList[iTaskID].irgParam[3];
				iEndY = g_krgTaskList[iTaskID].irgParam[2];
				if( g_krgTaskList[iTaskID].irgParam[2] < g_krgTaskList[iTaskID].irgParam[3] )
				{
					iStartY = g_krgTaskList[iTaskID].irgParam[2];
					iEndY = g_krgTaskList[iTaskID].irgParam[3];
				}

				for( int y = iStartY; y < iEndY; ++y )
				{
					for( int x = iStartX; x < iEndX; ++x )
					{
						int iMob = pMobGrid[y][x];
						if( 0 >= iMob || MAX_USER <= iMob ) continue;
						if( eTNClan_GM == pMob[iMob].m_byClan ) continue; //  GM�� ���� �̵���Ű�� �ʴ´�.
						if( eTNMob_PC == pMob[iMob].m_eMobType )
						{
							if( 0 >= pMob[iMob].MOB.nHP ) continue;
							pMob[iMob].TurnOffAffection( g_krgTaskList[iTaskID].irgParam[4] );
						}
					}
				}
			}
			break;
		case eTNAct_ChangeImmunity :
			{
				for( int i = MAX_USER; i < MAX_MOB; ++i )
				{
					if( pMob[i].MOB.snTribe == g_krgTaskList[iTaskID].irgParam[1] )
					{
						if( 1 == g_krgTaskList[iTaskID].irgParam[0] )  // �߰�
						{
							pMob[i].m_iImmunity = g_krgTaskList[iTaskID].irgParam[2];
						}
						else if( 0 == g_krgTaskList[iTaskID].irgParam[0] ) // ���Ÿ� �ϰԵǸ� ������ �ʱ� �⺻ �Ӽ����� ���ư���.
						{
							pMob[i].m_iImmunity = pMonsterData[pMob[i].MOB.nTP].nGuildID;
						}
					}
				}
			}
			break;

		case eTNAct_CountBrahmaMonster :
			{
				int iCount = 0;
				for( int i = MAX_USER; i < MAX_MOB; ++i )
				{
					if( eTNClan_BrahmaSoldier == pMob[i].m_byClan )
					{
						if( pMob[i].IsDead() ) continue;
						++iCount;
						//if( 0 < pMob[i].MOB.nHP ) ++iCount;
					}
				}

				if( 0 < iCount )
				{
					iProceed = 0;

					if( 6> iCount )
					{
						char chBuf[1024] = { 0,0,0, };
						sprintf(chBuf, "[eTNAct_CountBrahmaMonster] %d��%d��%d��%d��%d��> \r\n"
									, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
									);
						WriteLog( chBuf, g_szrgLogFileName[eLogFileName_CountMonster] );

						for( int i = MAX_USER; i < MAX_MOB; ++i )
						{
							if( eTNClan_BrahmaSoldier == pMob[i].m_byClan )
							{
								if( pMob[i].IsDead() ) continue;

								{
									char chBuf[1024] = { 0,0,0, };
									sprintf(chBuf, "\t[eTNAct_CountBrahmaMonster] h:%d, Trb:%d, pos(%d,%d) \r\n"
												, i, pMob[i].MOB.snTribe, pMob[i].TargetX, pMob[i].TargetY												
												);
									WriteLog( chBuf, g_szrgLogFileName[eLogFileName_CountMonster] );

								}
							}
						}
					}
				}
			}
			break;
		case eTNAct_CountVishnuMonster :
			{
				int iCount = 0;
				for( int i = MAX_USER; i < MAX_MOB; ++i )
				{
					if( eTNClan_VishnuSoldier == pMob[i].m_byClan )
					{
						if( pMob[i].IsDead() ) continue;
						++iCount;
						//if( 0 < pMob[i].MOB.nHP ) ++iCount;
					}
				}

				if( 0 < iCount )
				{
					iProceed = 0;

					if( 6 > iCount )
					{
						char chBuf[1024] = { 0,0,0, };
						sprintf(chBuf, "[eTNAct_CountVishnuMonster] %d��%d��%d��%d��%d��> \r\n"
									, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
									);
						WriteLog( chBuf, g_szrgLogFileName[eLogFileName_CountMonster] );

						for( int i = MAX_USER; i < MAX_MOB; ++i )
						{
							if( eTNClan_VishnuSoldier == pMob[i].m_byClan )
							{
								if( pMob[i].IsDead() ) continue;

								{
									char chBuf[1024] = { 0,0,0, };
									sprintf(chBuf, "\t[eTNAct_CountVishnuMonster] h:%d, Trb:%d, pos(%d,%d) \r\n"
												, i, pMob[i].MOB.snTribe, pMob[i].TargetX, pMob[i].TargetY												
												);
									WriteLog( chBuf, g_szrgLogFileName[eLogFileName_CountMonster] );

								}
							}
						}
					}
				}
			}
			break;
		case eTNAct_CountSivaMonster :
			{
				int iCount = 0;
				for( int i = MAX_USER; i < MAX_MOB; ++i )
				{
					if( eTNClan_SivaSoldier == pMob[i].m_byClan )
					{
						if( pMob[i].IsDead() ) continue;
						++iCount;
						//if( 0 < pMob[i].MOB.nHP ) ++iCount;
					}
				}

				if( 0 < iCount )
				{
					iProceed = 0;

					if( 6 > iCount )
					{
						char chBuf[1024] = { 0,0,0, };
						sprintf(chBuf, "[eTNAct_CountSivaMonster] %d��%d��%d��%d��%d��> \r\n"
									, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
									);
						WriteLog( chBuf, g_szrgLogFileName[eLogFileName_CountMonster] );

						for( int i = MAX_USER; i < MAX_MOB; ++i )
						{
							if( eTNClan_SivaSoldier == pMob[i].m_byClan )
							{
								if( pMob[i].IsDead() ) continue;

								{
									char chBuf[1024] = { 0,0,0, };
									sprintf(chBuf, "\t[eTNAct_CountSivaMonster] h:%d, Trb:%d, pos(%d,%d)\r\n"
												, i, pMob[i].MOB.snTribe, pMob[i].TargetX, pMob[i].TargetY											
												);
									WriteLog( chBuf, g_szrgLogFileName[eLogFileName_CountMonster] );

								}
							}
						}
					}
				}
			}
			break;
		case eTNAct_MoveTheGateOfDungeon :
			{
				{
					//SYSTEMTIME st;
					//GetLocalTime( &st );
					char chBuf[1024] = { 0,0,0, };
					sprintf(chBuf, "[eTNAct_MoveTheGateOfDungeon] %d��%d��%d��%d��%d��, �ֽŴ��� ����\
								   ����帶(%d,%d), �񽴴�(%d,%d), �ù�(%d,%d)\r\n"
								   , g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
								   , g_krgTaskList[iTaskID].irgParam[0], g_krgTaskList[iTaskID].irgParam[1]
								   , g_krgTaskList[iTaskID].irgParam[2], g_krgTaskList[iTaskID].irgParam[3]
								   , g_krgTaskList[iTaskID].irgParam[4], g_krgTaskList[iTaskID].irgParam[5]
								   );
					WriteLog( chBuf, g_szrgLogFileName[eLogFileName_EventTriggered] );
				}

				if( 100 < g_krgTaskList[iTaskID].irgParam[0] ) g_bIsKaliaOpend = true;
				else g_bIsKaliaOpend = false;

				Teleport( g_irgGateOfDungeon[0], g_krgTaskList[iTaskID].irgParam[0], g_krgTaskList[iTaskID].irgParam[1] ); // ����帶
				Teleport( g_irgGateOfDungeon[1], g_krgTaskList[iTaskID].irgParam[2], g_krgTaskList[iTaskID].irgParam[3] ); // �񽴴�
				Teleport( g_irgGateOfDungeon[2], g_krgTaskList[iTaskID].irgParam[4], g_krgTaskList[iTaskID].irgParam[5] ); // �ù�
			}
			break;
		case eTNAct_OnKilledLeftGeneral : // �ϵ��¼��� ���� �� ����
			{
				int iStartX, iEndX, iStartY, iEndY;
				// StartXZ(66,76),endXZ(166,164)
				iStartX = 66;
				iEndX = 166;
				iStartY = 76;
				iEndY = 164;

				#ifdef __TN_TOP_LOG__
				{
					//SYSTEMTIME st;
					//GetLocalTime( &st );
					char chBuf[512] = { 0,0,0, };
					sprintf(chBuf, "\r\n\n\n[%dMM%dDD %dH%dM%dS] The Left-Side General is killed !\r\n", g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond );
					WriteLog( chBuf, g_szrgLogFileName[eLogFileName_BossSystem] );
				}
				#endif    
				// StartXZ(658,959),endXZ(638,954))

				int iDestX, iDestY;
				for( int y = iStartY; y < iEndY; ++y )
				{
					for( int x = iStartX; x < iEndX; ++x )
					{
						int iMob = pMobGrid[y][x];
						if( 0 >= iMob || MAX_USER <= iMob ) continue;

						iDestX = PlayDice( 638, 658 );
						iDestY = PlayDice( 954, 959 );
						Teleport( iMob, iDestX, iDestY );
					}
				}
			}
			break;
		case eTNAct_OnKilledRightGeneral : // �ϵο켺�� ���� �� ����
			{
				int iStartX, iEndX, iStartY, iEndY;
				//StartXZ(856,68),endXZ(958,158)
				iStartX = 856;
				iEndX = 958;
				iStartY = 68;
				iEndY = 158;

				#ifdef __TN_TOP_LOG__
				{
					//SYSTEMTIME st;
					//GetLocalTime( &st );
					char chBuf[512] = { 0,0,0, };
					sprintf(chBuf, "\r\n\n\n[%dMM%dDD %dH%dM%dS] The Right-Side General is killed !\r\n", g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond );
					WriteLog( chBuf, g_szrgLogFileName[eLogFileName_BossSystem] );
				}
				#endif    
				//StartXZ(658,959),endXZ(638,954))

				int iDestX, iDestY;
				for( int y = iStartY; y < iEndY; ++y )
				{
					for( int x = iStartX; x < iEndX; ++x )
					{
						int iMob = pMobGrid[y][x];
						if( 0 >= iMob || MAX_USER <= iMob ) continue;

						iDestX = PlayDice( 638, 658 );
						iDestY = PlayDice( 954, 959 );
						Teleport( iMob, iDestX, iDestY );
					}
				}
			}
			break;
		case eTNAct_KickOutOtherClanInMyStronghold :
			{

				#ifdef __TN_TOP_LOG__
				{
					//SYSTEMTIME st;
					//GetLocalTime( &st );
					char chBuf[256] = { 0,0,0, };
					sprintf(chBuf, "\r\n\n[eTNAct_KickOutOtherClanInMyStronghold] %dMM%dDD %dHH%dMS%dSS >  Param:(%d,%d,%d) 0(%d,%d), 1(%d,%d), 2(%d,%d), 3(%d,%d) \r\n"
						, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
						, g_krgTaskList[iTaskID].irgParam[0], g_krgTaskList[iTaskID].irgParam[1], g_krgTaskList[iTaskID].irgParam[2]
						, g_krgStronghold[eStronghold_Northwest].iOwner, g_krgStronghold[eStronghold_Northwest].iOwnerFriend
						, g_krgStronghold[eStronghold_Northeast].iOwner, g_krgStronghold[eStronghold_Northeast].iOwnerFriend
						, g_krgStronghold[eStronghold_Southwest].iOwner, g_krgStronghold[eStronghold_Southwest].iOwnerFriend
						, g_krgStronghold[eStronghold_Southeast].iOwner, g_krgStronghold[eStronghold_Southeast].iOwnerFriend
						);
					WriteLog( chBuf, g_szrgLogFileName[eLogFileName_Stronghold] );
				}
				#endif

				for( int iPC = 0; iPC < MAX_USER; ++iPC )
				{
					if( eTNClan_GM == pMob[iPC].m_byClan ) continue;
					if( eTNCls_NPC == pMob[iPC].MOB.byClass1 ) continue;	
					if( USER_PLAY != pUser[iPC].Mode ) continue;

					int iGuildID = pMob[iPC].MOB.nGuildID;

					//�ƴ�����eZone_Shambala, 132,510
					HS2D_COORD kDest;
					kDest.x = 132;
					kDest.y = 510;

					int x = pMob[iPC].TargetX;
					int y = pMob[iPC].TargetY;

					#ifdef __TN_TOP_LOG__
					{
						char chBuf[256] = { 0,0,0, };
						sprintf(chBuf, "\t- PC(%s, H:%d), Guild:%d, Pos(%d,%d) \r\n"
							, pMob[iPC].MOB.szName, iPC
							, iGuildID, x, y
							);
						WriteLog( chBuf, g_szrgLogFileName[eLogFileName_Stronghold] );
					}
					#endif

					// stronghold ���� ��ġ�ϰ� �ִٸ�, clan�� Ȯ���ؼ� Ÿ guild�̸�, kick out~
					if( eTNCell_Shrine & g_krgCell[y][x].usProperty )
					{						
						if( eStronghold_Northwest == g_krgTaskList[iTaskID].irgParam[0] )
						{
							if( (x < 316) && (y < 707) )
							{
								if( g_krgStronghold[eStronghold_Northwest].iOwner != iGuildID && g_krgStronghold[eStronghold_Northwest].iOwnerFriend != iGuildID)
								{
									//if( !pMob[iPC].IsDead() ) Teleport( iPC, g_krgTaskList[iTaskID].irgParam[1], g_krgTaskList[iTaskID].irgParam[2] );
									Teleport( iPC, g_krgTaskList[iTaskID].irgParam[1], g_krgTaskList[iTaskID].irgParam[2] );
									
									//pMob[iPC].MoveZone( eZone_Shambala, kDest.x, kDest.y );
								}
							}
						}
						else if( eStronghold_Northeast == g_krgTaskList[iTaskID].irgParam[0] )
						{
							if( (x > 316) && (y < 707) )
							{
								if( g_krgStronghold[eStronghold_Northeast].iOwner != iGuildID && g_krgStronghold[eStronghold_Northeast].iOwnerFriend != iGuildID)
								{
									//if( !pMob[iPC].IsDead() ) Teleport( iPC, g_krgTaskList[iTaskID].irgParam[1], g_krgTaskList[iTaskID].irgParam[2] );
									Teleport( iPC, g_krgTaskList[iTaskID].irgParam[1], g_krgTaskList[iTaskID].irgParam[2] );
									//pMob[iPC].MoveZone( eZone_Shambala, kDest.x, kDest.y );
								}
							}
						}
						else if( eStronghold_Southwest == g_krgTaskList[iTaskID].irgParam[0] )
						{
							if( (x < 316) && (y > 707) )
							{
								if( g_krgStronghold[eStronghold_Southwest].iOwner != iGuildID && g_krgStronghold[eStronghold_Southwest].iOwnerFriend  != iGuildID)
								{
									//if( !pMob[iPC].IsDead() ) Teleport( iPC, g_krgTaskList[iTaskID].irgParam[1], g_krgTaskList[iTaskID].irgParam[2] );
									Teleport( iPC, g_krgTaskList[iTaskID].irgParam[1], g_krgTaskList[iTaskID].irgParam[2] );
									//pMob[iPC].MoveZone( eZone_Shambala, kDest.x, kDest.y );
								}
							}
						}
						else if( eStronghold_Southeast == g_krgTaskList[iTaskID].irgParam[0] )
						{
							if( (x > 316) && (y > 707) )
							{
								if( g_krgStronghold[eStronghold_Southeast].iOwner != iGuildID && g_krgStronghold[eStronghold_Southeast].iOwnerFriend != iGuildID)
								{
									//if( !pMob[iPC].IsDead() ) Teleport( iPC, g_krgTaskList[iTaskID].irgParam[1], g_krgTaskList[iTaskID].irgParam[2] );
									Teleport( iPC, g_krgTaskList[iTaskID].irgParam[1], g_krgTaskList[iTaskID].irgParam[2] );
									//pMob[iPC].MoveZone( eZone_Shambala, kDest.x, kDest.y );
								}
							}
						}
					}
				}
			}
			break;
		case eTNAct_CheckElapsedTimeAboutStronghold :
			{
				iProceed = 0;
				int iLocation = g_krgTaskList[iTaskID].irgParam[0];
				if( 0 > iLocation || eStronghold_Southeast < iLocation ) break;
				CTime kNow( g_kSystemTime );
				//kNow.GetCurrentTime();
				CTimeSpan kElapsedTime = kNow - g_krgStronghold[iLocation].kTimeOccupied;

				if( /*7200*/30240 > kElapsedTime.GetTotalMinutes() ) iProceed = 1; // ����ð��� 3�ְ� ���� �ʾҴ�. 21��*24�ð�*60��					
			}
			break;
		case eTNAct_SwitchBattleForStronghold :
			{
				g_iBattleForStronghold = g_krgTaskList[iTaskID].irgParam[0];

				Msg_StrongHoldStatus sm; sm.wType = _Msg_StrongHoldStatus;
				sm.byMode = g_iBattleForStronghold;
				sm.byDummy = 0; sm.snDummy = 0;
				MSGServerSocket.SendOneMessage((char*)&sm, sizeof(sm));

				if( 1 == g_iBattleForStronghold )			//	����� ����
				{

				}
				else if( 0 == g_iBattleForStronghold )		//	����� ����
				{

				}
			}
			break;
		case eTNAct_SetDuelFieldAtStronghold :
			{//345	285	736	677
				for( int y = 677; y < 736; ++y )
				{
					for( int x = 285; x < 345; ++x )
					{
						g_krgCell[y][x].usProperty = g_krgCell[y][x].usProperty | eTNCell_DuelZone;
						//if( 20 == g_krgCell[y][x].usEvent ) g_krgCell[y][x].usProperty = g_krgCell[y][x].usProperty | eTNCell_DuelZone;
						//else if( 21 == g_krgCell[y][x].usEvent ) g_krgCell[y][x].usProperty = g_krgCell[y][x].usProperty | eTNCell_SealedZone;
					}
				}
			}
			break;
		case eTNAct_CloseDuelFieldAtStronghold :
			{
				for( int y = 677; y < 736; ++y )
				{
					for( int x = 285; x < 345; ++x )
					{
						unsigned short usNewProperty = (unsigned short)eTNCell_DuelZone;
						if( usNewProperty & g_krgCell[y][x].usProperty  )							
							g_krgCell[y][x].usProperty = g_krgCell[y][x].usProperty ^ usNewProperty; // XOR�� ����
						/*
						if( 20 == g_krgCell[y][x].usEvent )
						{
							unsigned short usNewProperty = (unsigned short)eTNCell_DuelZone;
							if( usNewProperty & g_krgCell[y][x].usProperty  )							
								g_krgCell[y][x].usProperty = g_krgCell[y][x].usProperty ^ usNewProperty; // XOR�� ����
						}
						else if( 21 == g_krgCell[y][x].usEvent )
						{
							unsigned short usNewProperty = (unsigned short)eTNCell_SealedZone;
							if( usNewProperty & g_krgCell[y][x].usProperty  )							
								g_krgCell[y][x].usProperty = g_krgCell[y][x].usProperty ^ usNewProperty; // XOR�� ����
						}
						*/
					}
				}
			}
			break;
		case eTNAct_RegisterPCtoArenaEntry :
			{
				iProceed = 0;
				if( 0 >= a_iUser || MAX_USER <= a_iUser ) break; //return eTNRes_EvntInvalidUserHandle;
				if( eTNClan_GM == pMob[a_iUser].m_byClan ) break;

				if( g_kArena.IsFull() )
				{
					SendClientMessage( a_iUser, g_pMessageStringTable[_ArenaEntryIsFull] );
					break; //return eTNRes_ArenaEntryIsFull;
				}

				//g_kArena.AddEntrant( a_iUser );
				/*
				if( true == pMob[a_iUser].m_bIsInArenaEntry ) break; //return eTNRes_AlreadyRegisteredInArenaEntry;
				g_irgArenaEntry[g_iArenaEntryCount] = a_iUser;
				++g_iArenaEntryCount; // �����ڼ� 1 ����
				pMob[a_iUser].m_bIsInArenaEntry = true;
				*/

				iProceed = 1;
			}
			break;

		case eTNAct_StartSuvivalFight :
			{
				g_kArena.StartSurvival();
			}
			break;
		case eTNAct_CheckWinner :
			{
				if( !g_kArena.IsProgressed() ) break;

				int iRes = g_kArena.CheckWinner();
				if( !iRes ) // �Ʒ��� �����̹� ���ڰ� �����Ǿ���. ���� �̺�Ʈ�� trigger
				{
					TriggerEvent(0, 68, 0, 0, 0 ); // �Ʒ��� �����̹� �̺�Ʈ ����
				}
			}
			break;

		case eTNAct_PostThePrize :
			{
				if( !g_kArena.IsProgressed() ) break;

				g_kArena.Award();
				g_dwArenaMoney = 0;			

				char szMsg[1024] = { 0,0,0, };		
				char szWinners[512] = { 0,0,0, };
				g_kArena.OutputWinners( szWinners );
				sprintf( szMsg, g_pMessageStringTable[_PostThePrize], szWinners, g_kArena.get_GoldToAward() ); // g_dwArenaMoney, �ӽ÷� 1���� �Է��ߴ�. ���߿� ������ ������ ����� �Ѵ�.

				{
					char chBuf[2048] = { 0,0,0, };
					sprintf(chBuf, "[eTNAct_PostThePrize] %d��%d��%d��%d��%d��, %s\r\n", g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
						, szMsg );
					WriteLog( chBuf, g_szrgLogFileName[eLogFileName_Stronghold] );
				}

				PostMessageToWorld( szMsg );
			}
			break;
		case eTNAct_KickOutLosser :
			{ // ���������� �� PC���� ƨ���.
				int iWinner = g_kSiege.get_Winner();

				for( int i = 1; i < MAX_USER; ++i )
				{
					if( USER_PLAY != pUser[i].Mode ) continue;
					if( eTNClan_GM == pMob[i].m_byClan ) continue; //  GM�� ���� �̵���Ű�� �ʴ´�
					if( iWinner == pMob[i].m_byClan ) continue;

					/*losser �̶��*/
					//ReturnPCToSaveZone( i );

					 //x: 114~138  y: 124:~192
					int iOrder = i % 3; // 0,1,2
					if( 0 == iOrder) 
					{
						Teleport( i, 126, 138 );
					}
					else if( 1 == iOrder )
					{
						Teleport( i, 126, 158 );
					}
					else
					{
						Teleport( i, 126, 177 );
					}

					/*

					bool bFound = false;
					HS2D_COORD kPos;
					for( int y = 124; y < 192; ++y )
					{
						bFound = false;
						for( int x = 114; x < 138; ++x )
						{
							if( 0 != pMobGrid[y][x] ) continue; // mob�� �� �ڸ��� �����ϰ� ������, ...
							if( pMob[i].m_iBlockedCell & g_krgCell[y][x].usProperty ) continue;
							// x, y�� ���ڸ��̶��, ...						
							bFound = true;
							kPos.x = x;
							kPos.y = y;
							break;
						}
						if( bFound ) break;
					}

					Teleport( i, kPos.x, kPos.y );
					*/
				}
			}
			break;
		case eTNAct_JudgeTheSiege :
			{ // symbol �� �����ָ� ã�Ƽ� ���� ���
				g_kSiege.JudgeSiege();

				//	���� ����� ������ ������ ��ȭ�� ������ �˸���.
				TNCastle kOwner;
				g_kSiege.get_Owner( &kOwner);
				Msg_CastleUpdate sm; ZeroMemory(&sm, sizeof(sm));
				sm.wType = _Msg_CastleUpdate;
				sm.iCastleOwner = kOwner.kGuild.iID;
				MSGServerSocket.SendOneMessage((char*)&sm, sizeof(sm));
			}
			break;
		case eTNAct_PopSymbolForSiege :
			{
				if( 0 >= a_iUser || MAX_MOB <= a_iUser ) continue;
				if( MAX_USER <= a_iUser )
				{
					
					//if( (eTNCls2_Retainer == pMob[a_iUser].MOB.byClass2) || (eTNCls2_Familiar == pMob[a_iUser].MOB.byClass2) || (eTNCls_Fellow == pMob[a_iUser].MOB.byClass1) ) 
					if( eTNAIO_HaveMaster & pMob[a_iUser].m_iAIOption )
					{
						a_iUser = pMob[a_iUser].Leader;
					}
				}

				g_kSiege.CaptureSymbol( pMob[a_iUser].m_byClan, a_iNPC/*symbol*/, a_iUser/*capturer*/ );

				// symbol�� ���� �Ŀ� �ٷ�(���) pop �ȴٸ� �ణ ����� ���ϴ�. �׷��� delay�� �־�� �Ѵ�. ���� �����ϱ� ���� ����� �����Ǿ�� �Ѵ�.
				// 35(=7*5) ���� event/task
				// 35���� event�� 1�� �Ŀ� �����ϱ� ���� event/task
				//int TriggerEvent( int a_iUser, int a_iEventID, int a_iX, int a_iY, int a_iNPC, int a_iCaller )
				// a_iUser�� symbol�� owner�� �˾Ƴ� �� �ִ�. a_iX, a_iY�� ���ؼ� pop��ġ�� �˾Ƴ� �� �ִ�.

			}
			break;
		case eTNAct_RecoverClanToOriginal :
			{ // call a global routine
				ChangeClan( -1, -1 );
			}
			break;
		case eTNAct_ChangeClanForSiege :
			{
				if( !g_kSiege.get_Started() ) break; // �������� �ƴ϶��, ...

				#ifdef __TN_TOP_LOG__
				{
					char chBuf[512] = { 0,0,0, };
					sprintf(chBuf, "[eTNAct_ChangeClanForSiege] %dYY%dMM%dDD %2dHH%2dMI%2dSS> \r\n"
						, g_kSystemTime.wYear, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
						); 
					WriteLog( chBuf, g_szrgLogFileName[eLogFileName_Castle] );
				}
				#endif


				for( int i = 1; i < MAX_USER; ++i )
				{
					if( eTNClan_GM == pMob[i].m_byClan ) continue;
					//if( eTNClan_CastleOwner <= pMob[i].m_byClan && eTNClan_Siege4 >= pMob[i].m_byClan ) continue;
					if( USER_PLAY != pUser[i].Mode ) continue;

					byte byOld = pMob[i].m_byClan;

					int iFlag = g_kSiege.SearchEntry( pMob[i].MOB.nGuildID );
					if( -1 == iFlag ) pMob[i].m_byClan = eTNClan_Siege4; // ���Ҽ�
					else pMob[i].m_byClan = iFlag;

					MSG_SET_ZONE_SETTINGS kZoneSettingMsg;
					kZoneSettingMsg.wType = MSG_SET_ZONE_SETTINGS_ID;
					kZoneSettingMsg.wPDULength = sizeof(MSG_SET_ZONE_SETTINGS)-sizeof(HEADER);
					kZoneSettingMsg.snSiege = 1; //g_kSiege.get_Started(); ��������
					pUser[i].cSock.AddMessage( (char*)&kZoneSettingMsg, sizeof(MSG_SET_ZONE_SETTINGS) );

					S_SCP_INIT_OTHER_MOB sm;
					GetCreateMob( i, &sm );
					GridMulticast( pMob[i].TargetX, pMob[i].TargetY, (MSG_STANDARD*)&sm, i);


					MSG_CHANGE_CLAN kMsg;
					kMsg.wType = MSG_CHANGE_CLAN_ID;
					kMsg.wPDULength = sizeof(MSG_CHANGE_CLAN)-sizeof(HEADER);
					kMsg.snKeyID = i;
					kMsg.byClan = pMob[i].m_byClan;
					pUser[i].cSock.AddMessage( (char*)&kMsg, sizeof(MSG_CHANGE_CLAN) );

					pUser[i].cSock.SendMessage();

					#ifdef __TN_TOP_LOG__
					{
						char chBuf[512] = { 0,0,0, };
						sprintf(chBuf, "[eTNAct_ChangeClanForSiege] PC(%d, Name:%s, Guild:%d), clan(now:%d, old:%d) \r\n"
							,i , pMob[i].MOB.szName, pMob[i].MOB.nGuildID
							, pMob[i].m_byClan, byOld
							); 
						WriteLog( chBuf, g_szrgLogFileName[eLogFileName_Castle] );
					}
					#endif


/*
					S_SCP_RESP_CHAR_INIT kMsg;
					kMsg.wType = SCP_RESP_CHAR_INIT;
					kMsg.wPDULength = sizeof(S_SCP_RESP_CHAR_INIT)-sizeof(HEADER);

					kMsg.byResult = 1;
					kMsg.dwGameTime = time(NULL);
					kMsg.dwKeyID = i;
					kMsg.snX = pMob[i].TargetX;
					kMsg.snZ = pMob[i].TargetY;
					memcpy( kMsg.bySkill, pMob[i].MOB.bySkill, sizeof(BYTE)*MAX_SKILL );
					memcpy( kMsg.byQuest, pMob[i].MOB.byQuest, sizeof(BYTE)*MAX_EVENT_FLAG );
					memcpy( kMsg.Inven, pMob[i].MOB.Inven, sizeof(STRUCT_ITEM)*MAX_INVEN );
					memcpy( kMsg.Equip, pMob[i].MOB.Equip, sizeof(STRUCT_ITEM)*MAX_EQUIP );
					memcpy( kMsg.Cargo, pUser[i].Cargo, sizeof(STRUCT_ITEM)*MAX_CARGO );
					memcpy( kMsg.dwTimeStamp, pUser[i].m_time, sizeof(kMsg.dwTimeStamp));
					kMsg.nCargoMoney = pUser[i].Coin;
					kMsg.nGuildID = pMob[i].MOB.nGuildID;
					kMsg.byClan = pMob[i].m_byClan;
					kMsg.dwEvent = pUser[i].m_dwEvent;

					pUser[i].cSock.SendOneMessage((char*)&kMsg,sizeof(S_SCP_RESP_CHAR_INIT));

					S_SCP_INIT_OTHER_MOB sm;
					GetCreateMob( i, &sm );
					GridMulticast( pMob[i].TargetX, pMob[i].TargetY, (MSG_STANDARD*)&sm, 0);
*/
				}
			}
			break;
		case eTNAct_SwitchExpireOfTheTermForSiege :
			{
				g_kSiege.set_ExpiryOftheTerm( g_krgTaskList[iTaskID].irgParam[0] ); // 1�̸� �Ⱓ����(������û�Ұ�), 0�̸� �Ⱓ��(������û����)
				if( 0 < g_krgTaskList[iTaskID].irgParam[0] ) // ������û �Ұ�, ������ ����ǥ ������
				{
					char szFileName[64] = { 0,0,0, };
					sprintf( szFileName, ".\\Event\\SiegeEntry_%dYY%dMM%dDD %2dHH%2dMI%2dSS.txt", g_kSystemTime.wYear, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond );
					g_kSiege.SaveData( szFileName );
				}

				#ifdef __TN_TOP_LOG__
				{
					char chBuf[512] = { 0,0,0, };
					sprintf(chBuf, "\r\n\r\n[Switch ������û�Ⱓ] [%dYY%dMM%dDD %2dHH%2dMI%2dSS] > state:%d(0:������û����, 1:������û�Ұ�)\r\n"
						, g_kSystemTime.wYear, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
						, g_krgTaskList[iTaskID].irgParam[0]
						); 
					WriteLog( chBuf, g_szrgLogFileName[eLogFileName_Castle] );
				}
				#endif
			}
			break;
		case eTNAct_SwitchSiege :
			{ // call a global routine
				g_kSiege.set_Started( g_krgTaskList[iTaskID].irgParam[0] ); // 0�̸� ��������, 1�̸� ��������

				if( 0 >= g_krgTaskList[iTaskID].irgParam[0] ) // off
				{
					g_kSiege.DestroySymbols();
					ChangeClan( -1, -1 );

					for( int i = 1; i < MAX_USER; ++i )
					{
						if( USER_PLAY != pUser[i].Mode ) continue;

						MSG_SET_ZONE_SETTINGS kZoneSettingMsg;
						kZoneSettingMsg.wType = MSG_SET_ZONE_SETTINGS_ID;
						kZoneSettingMsg.wPDULength = sizeof(MSG_SET_ZONE_SETTINGS)-sizeof(HEADER);

						kZoneSettingMsg.snSiege = 0; //g_kSiege.get_Started(); ��������
						pUser[i].cSock.SendOneMessage( (char*)&kZoneSettingMsg, sizeof(MSG_SET_ZONE_SETTINGS) );
					}
				}
				else // turn on
				{
					g_kSiege.InstallSymbols();
					g_kSiege.SaveData( ".\\Event\\[Bak]Castle.txt" );
				}

				#ifdef __TN_TOP_LOG__
				{
					char chBuf[512] = { 0,0,0, };
					sprintf(chBuf, "\r\n\r\n[Start/End the siege] [%dYY%dMM%dDD %2dHH%2dMI%2dSS] > state:%d(0:��������, 1:��������)\r\n"
						, g_kSystemTime.wYear, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
						, g_krgTaskList[iTaskID].irgParam[0]
						); 
					WriteLog( chBuf, g_szrgLogFileName[eLogFileName_Castle] );
				}
				#endif
			}
			break;
		case eTNAct_FixTheDateOfTheSiege :
			{
				int iSelected = g_kSiege.get_DateToHold();
				//eDate_NotSelected = 0, eDate_Friday8HH, eDate_Friday10HH, eDate_Saturday8HH, eDate_Saturday10HH, eDate_Sunday8HH,

				if( TNSiege::eDate_Friday8HH == iSelected )
				{
					g_krgEventList[100].iDuration = eEvntSwitch_On;
				}
				else if( TNSiege::eDate_Friday10HH == iSelected )
				{
					g_krgEventList[101].iDuration = eEvntSwitch_On;
				}
				else if( TNSiege::eDate_Saturday8HH == iSelected )
				{
					g_krgEventList[102].iDuration = eEvntSwitch_On;
				}
				else if( TNSiege::eDate_Sunday8HH == iSelected )
				{
					g_krgEventList[104].iDuration = eEvntSwitch_On;
				}
				else
				{
					iSelected = TNSiege::eDate_Saturday10HH;
					g_krgEventList[103].iDuration = eEvntSwitch_On;
				}

				g_kSiege.SelectDate( iSelected );
			}
			break;
		case eTNAct_PostTheResultOfTheSiege :
			{

			}
			break;
		case eTNAct_PostTheScheduleForTheSiege : // schedule�� �־� �������� ������ �Ѵ�. ȭ���� ��¥ ���� �����̹Ƿ� ��,��, �� ������ �ϸ� �� ��
			{// ������ �������� ����� ���� 9�ÿ� ���ֵ˴ϴ�. �ڼ��� ������ ������� ������ �߽��ٿ��� ������ �ּ���.
				int iSelected = g_kSiege.get_DateToHold();
				if( 0 == iSelected ) break;

				int iMessageID = _PostTheDateForSiege1;
				if( 2 == iSelected ) iMessageID = _PostTheDateForSiege2;
				else if( 3 == iSelected ) iMessageID = _PostTheDateForSiege3;
				else if( 4 == iSelected ) iMessageID = _PostTheDateForSiege4;
				else if( 5 == iSelected ) iMessageID = _PostTheDateForSiege5;
				else iMessageID = _PostTheDateForSiege1;

				PostMessageToWorld( g_pMessageStringTable[iMessageID], eTNClr_White, eTNClr_BG, iMessageID );
			}
			break;

		case eTNAct_QuestEvent :
			{
				int iTribe = pMob[a_iNPC].MOB.snTribe;
				BOOL bRes = QUEST_OnEvent( a_iUser, iTribe, iTribe );
				if( FALSE == bRes )	return eTNRes_Failed;
			}
			break;
		} // end of switch

		if( eEvntPrcdType_Sequential == g_krgEventList[a_iEventID].sProceedType ) iProceed = 1;
	} // end of for statement

	if( iProceed ) g_krgEventList[a_iEventID].uiAvailableTime = g_krgEventList[a_iEventID].uiCoolDownTime + CurrentTime;

	return eTNRes_Succeeded;
}