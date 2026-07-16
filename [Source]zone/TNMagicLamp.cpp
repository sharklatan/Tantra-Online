/****************************************************************************

	파일명 : TNMagicLamp.cpp

	작성자 : 정재웅(spencerj@korea.com)
	작성일 : 2005-12-27

	Tab size : 4 spaces
	프로젝트명 : Tantra


	설명 : 
		- 
		- 

****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "TNDebug.h"
#include "Basedef.h"
#include "TNMagicLamp.h"
#include "Server.h"


TNMagicLamp g_krgMagicLamp[5];

TNMagicLamp::TNMagicLamp()
{
	Init();
}



TNMagicLamp::~TNMagicLamp()
{

}


void TNMagicLamp::Init()
{
	memset( m_irgMonsterToBeSummon, 0, sizeof(m_irgMonsterToBeSummon) );
}


//@Return
//	성공이라면 소환될 monster ID, 실패라면 0을 return
int TNMagicLamp::DrawMonsterCard()
{
	// deck에서 monster card 한장을 꺼낸다.
	int iMonsterIndex = m_kRating.Random();
	if( 0 > iMonsterIndex || eML_CountOfMonsterToBeSummon <= iMonsterIndex )
	{
		// 에러 로그를 남긴다.
		iMonsterIndex = 0;
		return 0;
	}

	int iMonsterID = m_irgMonsterToBeSummon[iMonsterIndex];

	return iMonsterID;
}


void TNMagicLamp::AddMonsterCard( int a_iIndex, int a_iMonsterID, int a_iRating )
{
	if( 0 > a_iIndex || eML_CountOfMonsterToBeSummon <= a_iIndex ) return;
	if( HT_PARAMTYPE_MONSTER_START > a_iMonsterID || HT_PARAMTYPE_MONSTER_END < a_iMonsterID ) return;
	if( 0 >= a_iRating ) return;
	
	m_irgMonsterToBeSummon[a_iIndex] = a_iMonsterID;
	m_kRating.AddCard( a_iIndex, a_iRating );

	#ifdef __TN_TOP_LOG__
	{
		char chBuf[512] = { 0,0,0, };
		sprintf(chBuf, "[%dMM%dDD %2dH%2dM%2dS] TNMagicLamp::AddMonsterCard() Index:%d, MonsterID:%d, Rating:%d \r\n"
			, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
			, a_iIndex, a_iMonsterID, a_iRating
			);
		WriteLog( chBuf, g_szrgLogFileName[eLogFileName_MagicLamp] );
	}
	#endif
}


void TNMagicLamp::ShuffleMonsterCard()
{
	//srand( timeGetTime() );
	for( int i = 0; i < eML_CountOfMonsterToBeSummon; ++i )
	{
		if( 0 == m_irgMonsterToBeSummon[i] )
		{
			assert( !"The Magic lamp have to include 13 elements. check .\\Data\\MagicLamp.txt" );
			exit( 0 );
		}
	}

	int iMaxShuffleCount = rand() % 10;
	iMaxShuffleCount += 3; // 최소 3회는 보장한다.
	for( int i = 0; i < iMaxShuffleCount; ++i ) m_kRating.Shuffle(); // 여러번 shuffle을 시켜서 random성을 증대시킨다.
}


int TNMagicLamp::LoadData( char* a_pFileName )
{
	char szData[64] = { 0,0,0, };
    char szToken1[32] = { 0,0,0, };
    char szToken2[32] = { 0,0,0, };
	char szToken3[32] = { 0,0,0, };
	char szToken4[32] = { 0,0,0, };
	char szToken5[32] = { 0,0,0, };
	char szToken6[32] = { 0,0,0, };
	char szToken7[32] = { 0,0,0, };
    int iToken1 = 0, iToken2 = 0, iToken3 = 0, iToken4 = 0, iToken5 = 0;
	
	char szFileName[256] = { 0,0,0, };
	sprintf( szFileName, ".\\Data\\%s", a_pFileName );


	FILE* fin;
	fin = fopen( szFileName, "rt" );
	if( NULL == fin ) 
	{ 
		char szError[512] = { 0,0,0, };
		sprintf( szFileName, "Not found %s in data folder", a_pFileName );
		MessageBox( hWndMain, szError, "Failed to initialize data about the Lamp of the Magic.", NULL ); 
		return eTNRes_Failed; 
	}

	while( true )
	{
		char* ret = fgets( (char*)szData, 64, fin );
		if( NULL == ret ) break;

		iToken1 = iToken2 = iToken3 = 0;
		sscanf( szData, "%s %s %s", szToken1, szToken2, szToken3 );
		if( '/' == szToken1[0] && '/' == szToken1[1] ) continue;
	
		iToken1 = atoi( szToken1 ); // index
		iToken2 = atoi( szToken2 ); // monster ID
		iToken3 = atoi( szToken3 ); // rating

		AddMonsterCard( iToken1, iToken2, iToken3 );
	}


	return eTNRes_Succeeded;
}