/**********************************************************************************************************
	Changed
		char pData[128] = {0,}; // 이 배열의 크기를 크게 변경을 했음, edited by spencer(2006.04)

**********************************************************************************************************/



#include "yut.h"
#include "TNDebug.h"
#include "GetFunc.h"

//	** map Index **  //
//
//		10     9     8      7     6     5
//
//
//		11     20                 25    4
//
//
//		12         21        26         3
// 
//                      22           
//
//		13         27        23         2
//
//
//		14     28                 24    1     
//
//
//		15    16    17     18    19     0
#include "server.h"
extern CMob					pMob     [MAX_MOB];
CYut::CYut(void)
{
	m_byYutPosition = 0;
	m_dwOwnerMoney = 0;
	for(int i=0; i<MAX_POSITION; i++) { m_uniPosition[i].iBuf = 0; }
	for(i=0; i<6; i++)
	{
		m_uniPosition[i].x = 675;
		m_uniPosition[i+10].x = 658;
		m_uniPosition[i+5].y = 410;
		m_uniPosition[i+15].y = 427;	//	20번은 잘못들어감.
	}
	m_uniPosition[0].y = 427;

	for(i=1; i<5; i++)
	{
		m_uniPosition[i].y = 423 - 3*(i-1);
		m_uniPosition[i+5].x = 671 - 3*(i-1);
		m_uniPosition[i+10].y = 414 + 3*(i-1);
		m_uniPosition[i+15].x = 662 + 3*(i-1);;
	}

	m_uniPosition[20].x = 661;
	m_uniPosition[20].y = 413;
	m_uniPosition[21].x = 663;
	m_uniPosition[21].y = 416;
	m_uniPosition[22].x = 666;
	m_uniPosition[22].y = 419;
	m_uniPosition[23].x = 670;
	m_uniPosition[23].y = 421;
	m_uniPosition[24].x = 672;
	m_uniPosition[24].y = 424;
	m_uniPosition[25].x = 672;
	m_uniPosition[25].y = 413;
	m_uniPosition[26].x = 670;
	m_uniPosition[26].y = 416;
	m_uniPosition[27].x = 663;
	m_uniPosition[27].y = 421;
	m_uniPosition[28].x = 661;
	m_uniPosition[28].y = 424;

	m_clsYut.Init();	//	랜덤테이블을 초기화시킨다
	m_clsYut.AddCard(1, 2500);
	m_clsYut.AddCard(2, 3750);
	m_clsYut.AddCard(3, 2500);
	m_clsYut.AddCard(4, 625);
	m_clsYut.AddCard(5, 625);
	srand(timeGetTime());
	int iCount = GetRandom(1, 5);
	for(i=0; i<iCount; i++)
	{
		m_clsYut.Shuffle();
	}

	m_dwStep = 0;
}

CYut::~CYut(void)
{
	HT_SaveMoney();
}

void CYut::HT_Init()
{
	ZeroMemory(m_arrarroPosition, sizeof(m_arrarroPosition));
	m_dequePostPosition.clear();
	std::map<char*, std::map<WORD, WORD>, HTStr>::iterator it = m_mapTable.begin();
	while ( it != m_mapTable.end() )
	{
		std::map<WORD, WORD>& rmapChar = it->second;
		rmapChar.clear();
		it++;
	}
	m_mapTable.clear();

	m_dwTotBetMoney = m_dwSavedTotBetMoney;
	m_byYutPosition = 0;
	ZeroMemory(m_dwTotPositionBetMoney, sizeof(m_dwTotPositionBetMoney));

	m_dwStep = 0;
}

WORD CYut::HT_wUserBet(char* pName, WORD wPosition, int iMoney)		//	포지션별 케릭터의 베팅갯수를 올린다.
{
	if(pName==NULL) return 1;
	if(wPosition<0 || wPosition>=MAX_POSITION) return 1;

	char pData[512] = {0,};
	sprintf(pData, "Userbet char :%s, position:%d, count:%d \r\n", pName, wPosition, iMoney);
	TimeWriteLog(pData, ".\\Event\\[Log]Yut.txt");

	std::map<char*, std::map<WORD, WORD>, HTStr>::iterator it = m_mapTable.find( pName );
	if ( it != m_mapTable.end() )
	{	
		sprintf(pData, "Find in map char :%s mapsize:%d \r\n", pName, m_mapTable.size());
		TimeWriteLog(pData, ".\\Event\\[Log]Yut.txt");

		//	해당 케릭터가 기베팅멤버일 경우
		std::map<WORD, WORD>& rmapChar = it->second;
		std::map<WORD, WORD>::iterator itMapChar = rmapChar.find( wPosition );
		if ( itMapChar != rmapChar.end() )
		{	// 이미 해당 위치에 배팅을 한경우
			WORD wIndex =  itMapChar->second;

			sprintf(pData, "already bet that position char :%s money:%d \r\n", m_arrarroPosition[wPosition][wIndex].pName, m_arrarroPosition[wPosition][wIndex].dwCount);
			TimeWriteLog(pData, ".\\Event\\[Log]Yut.txt");

			if(wIndex<0 || wIndex>=MAX_USER) return 2;					//	맵데이터오류

			m_arrarroPosition[wPosition][wIndex].dwCount += (DWORD)iMoney;		//	기베팅숫자를 증가시킨다.
			m_dwTotBetMoney += iMoney;
			m_dwTotPositionBetMoney[wPosition] += iMoney;
			return 0;
		}
		else
		{	// 이 위치에 처음 배팅을 할경우

			sprintf(pData, "first time bet that position char :%s \r\n", pName);
			TimeWriteLog(pData, ".\\Event\\[Log]Yut.txt");
			for(WORD w=0; w<MAX_USER; w++)
			{
				if(m_arrarroPosition[wPosition][w].dwCount == 0) break;
			}

			if(w == MAX_USER) return 3;									//	인원이 풀일경우 

			strncpy(m_arrarroPosition[wPosition][w].pName, pName, SZNAME_LENGTH);	//	베팅가능시 데이터 세팅
			m_arrarroPosition[wPosition][w].dwCount = iMoney;

			rmapChar.insert( std::map<WORD, WORD>::value_type( wPosition, w ) );	//	생성된 포지션정보를 맵에 추가한다
		}
	}	
	else
	{	//	해당 케릭터가 처음베팅하는 경우
		std::map<WORD, WORD> tempMap;
		for(WORD w=0; w<MAX_USER; w++)
		{
			if(m_arrarroPosition[wPosition][w].dwCount == 0) break;
		}

		if(w == MAX_USER) return 3;									//	인원이 풀일경우 

		strncpy(m_arrarroPosition[wPosition][w].pName, pName, SZNAME_LENGTH);	//	베팅가능시 데이터 세팅
		m_arrarroPosition[wPosition][w].dwCount = iMoney;

		tempMap.insert(std::map<WORD, WORD>::value_type(wPosition, w));									//	해당 케릭터의 포지션정보를 맵에 추가한다
		m_mapTable.insert(std::map<char*, std::map<WORD, WORD>, HTStr>::value_type(m_arrarroPosition[wPosition][w].pName, tempMap));	//	케릭터의 맵을 추가한다

		sprintf(pData, "Make map char:%s size:%d size:%d \r\n", pName, m_mapTable.size(), tempMap.size());
		TimeWriteLog(pData, ".\\Event\\[Log]Yut.txt");
	}

	m_dwTotBetMoney += (DWORD)iMoney;
	m_dwTotPositionBetMoney[wPosition] += (DWORD)iMoney;

	return 0;	//	1:입력데이터오류, 2:맵데이터오류, 3:해당포지션베팅인원풀
}

bool CYut::HT_bIsBetOn()
{
	return (m_dwProgress & eYut_bet) != 0 ;
}

void CYut::HT_SetBet(DWORD dwFlag)
{
	if(dwFlag>0) m_dwProgress |= eYut_bet;
	else	m_dwProgress &= 0x11111110;
}

bool CYut::HT_bIsPlayOn()
{
	return (m_dwProgress & eYut_play) != 0 ;
}

void CYut::HT_SetPlay(DWORD dwFlag)
{
	if(dwFlag>0) m_dwProgress |= eYut_play;
	else	m_dwProgress &= 0x11111011;
}

BYTE CYut::HT_byYutPlay()
{
	m_byResult = m_clsYut.Random();

	return m_byResult;
}

BYTE CYut::HT_byGetResult()
{
	return m_byResult;
}

BYTE CYut::HT_byMoveYut()
{
	if(m_byYutPosition == 5)
	{
		if(m_byResult==1 || m_byResult==2) 
			m_byYutPosition += (m_byResult+19);
		else if(m_byResult==3) 
			m_byYutPosition = 22;
		else 
			m_byYutPosition += (m_byResult+18);
	}
	else if(m_byYutPosition == 10)
	{
		m_byYutPosition += (m_byResult+9);
	}
	else if(m_byYutPosition == 22)
	{
		if(m_byResult>3) 
			m_byYutPosition = MAX_POSITION;
		else if(m_byResult==3) 
			m_byYutPosition = 0;
		else 
			m_byYutPosition += m_byResult;
	}
	else if(m_byYutPosition == 25)
	{
		if(m_byResult == 1) 
			m_byYutPosition += m_byResult;
		else if(m_byResult == 2) 
			m_byYutPosition = 22;
		else if(m_byResult == 3 || m_byResult == 4) 
			m_byYutPosition += (m_byResult-1);
		else 
			m_byYutPosition = 15;
	}
	else if(m_byYutPosition == 26)
	{
		if(m_byResult == 1) 
			m_byYutPosition = 22;
		else if(m_byResult == 2 || m_byResult == 3) 
			m_byYutPosition += (m_byResult-1);
		else 
			m_byYutPosition -= (15-m_byResult);
	}
	else if(m_byYutPosition == 27)
	{
		if(m_byResult == 1) 
			m_byYutPosition += m_byResult;
		else
			m_byYutPosition -= (14-m_byResult);
	}
	else if(m_byYutPosition == 28)
	{
		m_byYutPosition -= (14-m_byResult);
	}
	else
	{
		if(m_byYutPosition==0 && !m_dequePostPosition.empty())
		{
			m_byYutPosition = MAX_POSITION;
		}
		else if(m_byYutPosition<20)
		{	m_byYutPosition += m_byResult;
			if(m_byYutPosition==20) m_byYutPosition = 0;
			else if(m_byYutPosition>20) m_byYutPosition = MAX_POSITION;
		}	
		else
		{	m_byYutPosition += m_byResult;
			if(m_byYutPosition==25) m_byYutPosition = 0;
			else if(m_byYutPosition>25) m_byYutPosition = MAX_POSITION;
		}
	}

	//	fx도 추가해줘야 한다.
	m_dequePostPosition.push_back(m_byYutPosition);

	return m_byYutPosition;
}

void CYut::HT_ResetMoney()
{
	m_mapUserMoney.clear();
	ZeroMemory(m_arrUserMoney, sizeof(m_arrUserMoney));

	char pData[512] = {0,};
	sprintf(pData, "HT_ResetMoney started. All users money erased !!! \r\n"); 
	TimeWriteLog(pData, ".\\Event\\[Log]Yut.txt");
}

void CYut::HT_SaveMoney()
{
	FILE *fp = fopen("Data\\Gamble_Yut.txt","wt");
	if ( fp != NULL )
	{
		fprintf(fp, "%d \n", m_dwOwnerMoney);
		for(int i=0; i<MAX_BETUSER; i++)
		{
			if(m_arrUserMoney[i].pName[0] == 0) continue;
			fprintf(fp, "%s %d \n", m_arrUserMoney[i].pName, m_arrUserMoney[i].dwCount);
		}
		fclose(fp);
	}
}

void CYut::HT_ReadSaveMoney()
{
	FILE *fp = fopen("Data\\Gamble_Yut.txt","rt");
	ZeroMemory(m_arrUserMoney, sizeof(m_arrUserMoney));
	m_mapUserMoney.clear();

	if ( fp != NULL )
	{
		if( fscanf(fp, "%d", &m_dwOwnerMoney) == EOF ) 
		{	fclose(fp);
			return;
		}
		for(WORD w=0; w<MAX_BETUSER; w++)
		{
			if( fscanf(fp, "%s %d", m_arrUserMoney[w].pName, &(m_arrUserMoney[w].dwCount)) == EOF ) break;
			m_mapUserMoney.insert(std::map<char*, WORD, HTStr>::value_type(m_arrUserMoney[w].pName, w));
		}
		fclose(fp);
	}
}

void CYut::HT_ShareMoney()
{
	char pData[512] = {0,};
	BYTE byPos = 0;
	DWORD dwWinTotMoney = 0;
	double dMyMoney, dTotMoney, dTotWinMoney = 0;
	double dBonusTotMoney = 0;
	WORD  wIndex = 0;

	std::deque<BYTE>::iterator it;
	for(it=m_dequePostPosition.begin(); it!=m_dequePostPosition.end(); it++)
	{
		byPos = *it;
		if(byPos<0 || byPos>=MAX_POSITION) continue;

		dwWinTotMoney += (DWORD)(dYut_Bounus[byPos]*m_dwTotPositionBetMoney[byPos]);
	}

	if(dwWinTotMoney==0)
	{
		m_dwSavedTotBetMoney = m_dwTotBetMoney;
		sprintf(pData, "ShareMoney started but have no win users!! \r\n"); 
		TimeWriteLog(pData, ".\\Event\\[Log]Yut.txt");
		return;
	}

	dTotMoney = (double)m_dwTotBetMoney;
	dTotWinMoney = (double)dwWinTotMoney;
	if(m_dwOwnerMoney<MAX_INT) m_dwOwnerMoney += (DWORD)(m_dwTotBetMoney * 0.0125);
	m_dwSavedTotBetMoney = 0;

	for(it=m_dequePostPosition.begin(); it!=m_dequePostPosition.end(); it++)
	{
		byPos = *it;
		if(byPos<0 || byPos>=MAX_POSITION) continue;

		for(int i=0; i<MAX_USER; i++)
		{
			if(m_arrarroPosition[byPos][i].pName[0] == 0) break;	//	데이터가 없을경우 해당 위치 정산종한다.
			
			dMyMoney = (double)m_arrarroPosition[byPos][i].dwCount;
			m_arrarroPosition[byPos][i].dwWinMoney = (DWORD)(dMyMoney * 0.95 * dTotMoney * dYut_Bounus[byPos] / dTotWinMoney) ;

			sprintf(pData, "Winner char:%s at pos:%d money:%d betme:%f betpostot:%f bettot:%f \r\n", 
				m_arrarroPosition[byPos][i].pName, byPos, m_arrarroPosition[byPos][i].dwWinMoney, dMyMoney, dTotMoney, dTotWinMoney);
			TimeWriteLog(pData, ".\\Event\\[Log]Yut.txt");

			std::map<char*, WORD, HTStr>::iterator it = m_mapUserMoney.find( m_arrarroPosition[byPos][i].pName );
			if( it != m_mapUserMoney.end() )	//	기존 저축액이 존재할 경우
			{	
				wIndex = it->second;
				m_arrUserMoney[wIndex].dwCount += m_arrarroPosition[byPos][i].dwWinMoney;
			}
			else								//	기존 저축액이 없을 경우
			{	
				for(int n=0; n<MAX_BETUSER; n++)
				{
					if(m_arrUserMoney[n].pName[0] == 0) break;
				}
				if(n == MAX_BETUSER)			//	상금을 분배해야 하는데 분배대상 메모리가 풀일 경우(설마 2만명이 찰라구..)
				{
					char pch[128] = {0,};
					sprintf(pch, "GambleMoney share-error is broken. char:%s gold:%d \r\n", m_arrarroPosition[byPos][i].pName, m_arrarroPosition[byPos][i].dwWinMoney); 
					TimeWriteLog(pch, "Gamble-Error.txt");
				}
				strncpy(m_arrUserMoney[n].pName, m_arrarroPosition[byPos][i].pName, SZNAME_LENGTH);
				m_arrUserMoney[n].dwCount = m_arrarroPosition[byPos][i].dwWinMoney;

				m_mapUserMoney.insert(std::map<char*, WORD, HTStr>::value_type(m_arrUserMoney[n].pName, n));
			}
		}
	}
	sprintf(pData, "Save Money List  \r\n"); 
	TimeWriteLog(pData, ".\\Event\\[Log]Yut.txt");

	for(int n=0; n<MAX_BETUSER; n++)
	{
		if(m_arrUserMoney[n].pName[0] == 0) continue;
		sprintf(pData, "Saved Money char:%s money:%d  \r\n", m_arrUserMoney[n].pName, m_arrUserMoney[n].dwCount); 
		TimeWriteLog(pData, ".\\Event\\[Log]Yut.txt");
	}
	HT_SaveMoney();
}

BYTE CYut::HT_byGetPosition()
{
	return m_byYutPosition;
}

BYTE CYut::HT_byGetPrePosition()
{
	if(m_dequePostPosition.empty()) return MAX_POSITION;
	
	int index = m_dequePostPosition.size() - 1;

	return (BYTE)m_dequePostPosition[index];
}

void CYut::HT_GetPositionBetMoney(DWORD* pdwMoney)
{
	memcpy(pdwMoney, m_dwTotPositionBetMoney, sizeof(m_dwTotPositionBetMoney));
}

void CYut::HT_GetPositionBetMoney(char* pName, DWORD* pdwMoney)
{
	DWORD dwMoney[MAX_POSITION] = {0,};

	std::map<char*, std::map<WORD, WORD>, HTStr>::iterator it = m_mapTable.find( pName );
	if ( it != m_mapTable.end() )	//	특정 칸에 베팅을 했다는 의미이다
	{
		std::map<WORD, WORD>& rmapChar = it->second;
		std::map<WORD, WORD>::iterator pos;
		WORD wPosition = 0;
		WORD wIndex = 0;
		for(pos=rmapChar.begin(); pos!=rmapChar.end(); ++pos)
		{
			wPosition = pos->first;
			wIndex = pos->second;
			if(wPosition>=MAX_POSITION || wIndex>MAX_USER)	//	맵에 잘못된 값이 들어있는 경우(있을수 없다)
			{
				char pData[512] = {0,};
				sprintf(pData, "err HT_GetPositionBetMoney mapindex:%d, value:%d \r\n", wPosition, wIndex);
				TimeWriteLog(pData, ".\\Event\\[Log]Yut.txt");
				ZeroMemory(pdwMoney, sizeof(DWORD)*MAX_POSITION);
				return;
			}
			dwMoney[wPosition] = m_arrarroPosition[wPosition][wIndex].dwCount;
		}
		memcpy(pdwMoney, dwMoney, sizeof(dwMoney));
	}
	else							//	베팅된 칸이 없다는 말이다
	{
		ZeroMemory(pdwMoney, sizeof(DWORD)*MAX_POSITION);
	}
}

void CYut::HT_WriteLog()
{
	int iIndex = 1;
	std::map<char*, std::map<WORD, WORD>, HTStr>::iterator it = m_mapTable.find( pMob[iIndex].MOB.szName );
	if(it != m_mapTable.end())
	{
		std::map<WORD, WORD>& rmapChar = it->second;
		std::map<WORD, WORD>::iterator pos;
		WORD wPosition = 0;
		WORD wIndex = 0;
		int iSize = rmapChar.size();
		for(pos=rmapChar.begin(); pos!=rmapChar.end(); ++pos)
		{
			wPosition = pos->first;
			wIndex = pos->second;
		}
	}

	char pData[512] = {0,};
	sprintf(pData, "TotalMoney:%d \r\n", m_dwTotBetMoney);
	TimeWriteLog(pData, ".\\Event\\[Log]Yut.txt");

	for(int i=0; i<MAX_POSITION; i++)
	{
		sprintf(pData, "Position:%d TotalMoney:%d \r\n", i+1, m_dwTotPositionBetMoney[i]);
		TimeWriteLog(pData, ".\\Event\\[Log]Yut.txt");
		if(m_dwTotPositionBetMoney[i]>0)
		{
			for(int k=0; k<MAX_USER; k++)
			{
				if(m_arrarroPosition[i][k].pName[0]!=0)
				{
					sprintf(pData, "          char:%s betmoney:%d winmoney:%d \r\n", m_arrarroPosition[i][k].pName, m_arrarroPosition[i][k].dwCount, m_arrarroPosition[i][k].dwWinMoney);
					TimeWriteLog(pData, ".\\Event\\[Log]Yut.txt"); continue;
				}
				break;
			}
		}
	}
}

void CYut::HT_WriteLog(char* pName)
{
	char pData[512] = {0,};
	sprintf(pData, "char:%s Bet-Status Log : \r\n", pName);
	TimeWriteLog(pData, ".\\Event\\[Log]Yut.txt");

	std::map<char*, std::map<WORD, WORD>, HTStr>::iterator it = m_mapTable.find( pName );
	if ( it != m_mapTable.end() )
	{
		std::map<WORD, WORD>& rmapChar = it->second;
		std::map<WORD, WORD>::iterator pos;
		WORD wPosition = 0;
		WORD wIndex = 0;

		sprintf(pData, "map size:%d \r\n", rmapChar.size() );
		TimeWriteLog(pData, ".\\Event\\[Log]Yut.txt");

		for(pos=rmapChar.begin(); pos!=rmapChar.end(); ++pos)
		{
			wPosition = pos->first;
			wIndex = pos->second;
			if(wPosition>=MAX_POSITION || wIndex>MAX_USER)	//	맵에 잘못된 값이 들어있는 경우(있을수 없다)
			{
				sprintf(pData, "err HT_WriteLog mapindex:%d, value:%d \r\n", wPosition, wIndex);
				TimeWriteLog(pData, ".\\Event\\[Log]Yut.txt");
				return;
			}
			sprintf(pData, "char:%s Pos:%d betmoney:%d winmoney:%d \r\n", m_arrarroPosition[wPosition][wIndex].pName, wPosition, m_arrarroPosition[wPosition][wIndex].dwCount, m_arrarroPosition[wPosition][wIndex].dwWinMoney);
			TimeWriteLog(pData, ".\\Event\\[Log]Yut.txt");
			//if(strncmp(pName, m_arrarroPosition[wPosition][wIndex].pName, 20) != 0) 
			//	MessageBox(NULL, "ERROR","ERROR", MB_OK);
		}
	}
	else
	{
		sprintf(pData, "char:%s have no betting data \r\n", pName);
		TimeWriteLog(pData, ".\\Event\\[Log]Yut.txt");
	}
}

DWORD CYut::HT_GetMoney(char* pName)
{
	std::map<char*, WORD, HTStr>::iterator it = m_mapUserMoney.find( pName );
	if( it != m_mapUserMoney.end() )	//	기존 저축액이 존재할 경우
	{	
		WORD wIndex = it->second;
		return m_arrUserMoney[wIndex].dwCount;
	}
	else								//	기존 저축액이 없을 경우
		return 0;
}

void CYut::HT_SetYutMoney(char* pName, DWORD dwMoney)
{
	std::map<char*, WORD, HTStr>::iterator it = m_mapUserMoney.find( pName );
	if( it != m_mapUserMoney.end() )	//	기존 저축액이 존재할 경우
	{	
		WORD wIndex = it->second;
		m_arrUserMoney[wIndex].dwCount = dwMoney;
	}
}

DWORD CYut::GetOwnerMoney()
{
	return m_dwOwnerMoney;
}

void CYut::SetOwnerMoney(DWORD dwMoney)
{
	m_dwOwnerMoney = dwMoney;
}
