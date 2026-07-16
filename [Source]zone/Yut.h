#pragma once

#include "BaseDef.h"
#include "EventDefines.h"
#include "TNDeck10000.h"

#include <map>
#include <deque>

class CYut
{
private:
	struct BETUSER
	{
		char	pName[SZNAME_LENGTH];	//	베팅케릭터이름
		DWORD	dwCount;				//	베팅갯수
		DWORD	dwWinMoney;				//	배당금액
	};

	struct SAVEBETUSER
	{
		char	pName[SZNAME_LENGTH];	//	베팅케릭터이름
		DWORD	dwCount;				//	금전누적갯수
	};

	std::deque<BYTE> m_dequePostPosition;
	std::map<char*, WORD, HTStr> m_mapUserMoney;						//	유저별 적립금액
	std::map<char*, std::map<WORD, WORD>, HTStr> m_mapTable;			//	포지션당 케릭터에 해당하는 맵배열인덱스를 관리한다.
	
	TNDeck10000	m_clsYut;
	BETUSER	m_arrarroPosition[MAX_POSITION][MAX_USER];					//	포지션당 케릭터베팅 저장공간
	DWORD	m_dwProgress;												//	진행상황(basedef.h에 정의되어있음)
	DWORD	m_dwTotBetMoney;											//	전체 베팅금액
	DWORD	m_dwSavedTotBetMoney;										//	당첨자가 없어서 이월되는 전체 베팅금액
	DWORD	m_dwTotPositionBetMoney[MAX_POSITION];						//	전체 위치별베팅금액
	BYTE	m_byResult;
	SAVEBETUSER	m_arrUserMoney[MAX_BETUSER];
	int		m_byYutPosition;
	DWORD	m_dwOwnerMoney;
public:
	CYut(void);
	~CYut(void);

	void	HT_Init();
	WORD	HT_wUserBet(char* pName, WORD wPosition, int iMoney);		//	포지션별 케릭터의 베팅갯수를 올린다.


	bool	HT_bIsBetOn();
	bool	HT_bIsPlayOn();
	void	HT_SetBet(DWORD dwFlag);
	void	HT_SetPlay(DWORD dwFlag);

	void	HT_ResetMoney();
	void	HT_SaveMoney();
	void	HT_ReadSaveMoney();
	void	HT_ShareMoney();

	BYTE	HT_byYutPlay();
	BYTE	HT_byGetResult();
	BYTE	HT_byMoveYut();
	BYTE	HT_byGetPosition();
	BYTE	HT_byGetPrePosition();
	HS2D_COORD	m_uniPosition[MAX_POSITION];
	void	HT_GetPositionBetMoney(DWORD* pdwMoney);
	void	HT_GetPositionBetMoney(char* pName, DWORD* pdwMoney);
	void	HT_WriteLog();
	void	HT_WriteLog(char* pName);
	DWORD	HT_GetMoney(char* pName);
	void	HT_SetYutMoney(char* pName, DWORD dwMoney);
	DWORD	GetOwnerMoney();
	void	SetOwnerMoney(DWORD dwMoney);

	DWORD	m_dwStep;
};
