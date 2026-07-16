#include "stdafx.h"
#include "assert.h"
#include "TNScheduler.h"
#include "TNDebug.h"
#include "Server.h"

#if defined(__ZONE_SERVER__) && defined(__MEMORYMANAGER__)

#ifndef _HTMEMORYMANAGER_H_
#include "HTMemoryManager.h"
#endif

#endif //__ZONE_SERVER__, __MEMORYMANAGER__


TNScheduler g_kScheduler ;


TNScheduler::TNScheduler()
{
	Init() ;
}

TNScheduler::~TNScheduler()
{
}


void TNScheduler::Init()
{
	m_iLast = 0 ;
	memset( m_krgSchedule, 0, sizeof(m_krgSchedule) ) ;
} 



int TNScheduler::AddSchedule( unsigned int a_uiNow, unsigned int a_uiGap, unsigned int a_uiFreq, int a_iEvent )
{
	//SYSTEMTIME st ;
	//GetLocalTime( &st ) ;

	for( int i = 0 ; i < eSchedule_MaxCount ; ++i )
	{
		if( (0 < m_krgSchedule[i].uiTime) && (0 < m_krgSchedule[i].iEvent) && (eEvent_MaxCount > m_krgSchedule[i].iEvent) ) continue ;
		m_krgSchedule[i].uiTime = a_uiNow + a_uiGap ;		
		m_krgSchedule[i].uiFreq = a_uiFreq ;
		m_krgSchedule[i].iEvent = a_iEvent ;
		if( m_iLast < i ) m_iLast = i;

		{
			char chBuf[512] = { 0,0,0, } ;
			sprintf(chBuf, "\n\n\n[%d월%d일%d요일 %d시%d분%d초] TNScheduler::AddSchedule() > now:%u, gap:%u, freq:%u, event:%d, last:%d \r\n"
				, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wDayOfWeek, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
				, a_uiNow, a_uiGap, a_uiFreq, a_iEvent, m_iLast
				) ;
			WriteLog( chBuf, ".\\Monster_Log\\[Log]LoadSchedule.txt" ) ;
		}

		return eTNRes_Succeeded ;
	}


	{
		char chBuf[512] = { 0,0,0, } ;
		sprintf(chBuf, "\n\n\n[%d월%d일%d요일 %d시%d분%d초] TNScheduler::AddSchedule() Failed > now:%u, gap:%u, freq:%u, event:%d, last:%d \r\n"
			, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wDayOfWeek, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond
			, a_uiNow, a_uiGap, a_uiFreq, a_iEvent, m_iLast
			) ;
		WriteLog( chBuf, ".\\Monster_Log\\[Log]LoadSchedule.txt" ) ;
	}

	return eTNRes_Failed;

	//if( eSchedule_MaxCount <= m_iLast ) return eTNRes_Failed ;

	//m_krgSchedule[m_iLast].uiTime = a_uiNow + a_uiGap ;		
	//m_krgSchedule[m_iLast].uiFreq = a_uiFreq ;
	//m_krgSchedule[m_iLast].iEvent = a_iEvent ;

	//++m_iLast ;

	//return eTNRes_Succeeded ;
}



//@Caller
//	- 16초나 32초 time tic에서 CheckNRunSchedule(...) 을 호출해준다.
void TNScheduler::CheckNRunSchedule( unsigned int a_uiNow )
{
	//if( 0 >= m_iLast ) return ;
	//int iLastElement = m_iLast - 1 ;
	for( int i = 0 ; i < eSchedule_MaxCount ; ++i )
	{
		if( 0 == m_krgSchedule[i].uiTime ) continue ;
		if( 0 >= m_krgSchedule[i].iEvent ) continue ;

		if( a_uiNow > m_krgSchedule[i].uiTime )
		{
			//m_krgSchedule[i].uiTime += m_krgSchedule[i].uiFreq ;			
			TriggerEvent( 0, m_krgSchedule[i].iEvent, 0, 0, 0, 40 ) ;

			m_krgSchedule[i].uiTime = 0 ; // startup시에 딱 1회만 수행
			m_krgSchedule[i].iEvent = 0 ;
			
			//if( 0 == m_krgSchedule[i].uiFreq )
			//{
			//	m_krgSchedule[i].uiTime = 0 ; // startup시에 딱 1회만 수행
			//	m_krgSchedule[i].iEvent = 0 ;
			//}
		}
	}
}


void TNScheduler::Load()
{
	//SYSTEMTIME st ;
	//GetLocalTime( &st ) ;
	char szFileName[24] = { 0,0,0, } ;
	sprintf( szFileName, ".\\Data\\Schedule.txt" ) ;

	FILE* fin = fopen( szFileName, "rt") ;
	if( NULL == fin ) return ;

	unsigned int uiNow = timeGetTime() ;

	int iDayOfWeek, iHour, iMinute, iSecond, iEvent ;
	unsigned int uiFreq ;

	char szLine[256] = { 0,0,0, } ;
	char szrgElement[7][12] = { 0,0,0, } ;
	char* pReturn = NULL ;	
	int iSeq = 0 ;

	{
		char chBuf[512] = { 0,0,0, } ;
		sprintf(chBuf, "\n\n\n[%d월%d일%d요일 %d시%d분%d초] Scheduler.Load() > Start\r\n"
			, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wDayOfWeek, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond ) ;
		WriteLog( chBuf, ".\\Monster_Log\\[Log]LoadSchedule.txt" ) ;
	}

	while( 1 )
	{
		pReturn = fgets( szLine, 255, fin ) ;
		if( NULL == pReturn ) break ; // end of file
		if( 0 == szLine[0] ) continue ; // NUL
		if( 10 == szLine[0] ) continue ; // LF
		if( 13 == szLine[0] ) continue ; // CR
		if( '/' == szLine[0] ) continue ; // comment
		if( ' ' == szLine[0] ) continue ; // blank

		iDayOfWeek = iHour = iMinute = iSecond = uiFreq = iEvent = 0 ;
		memset( szrgElement, 0, sizeof(szrgElement) ) ;

		sscanf( szLine, "%s %s %s %s %s %s", szrgElement[0], szrgElement[1], szrgElement[2], szrgElement[3], szrgElement[4], szrgElement[5] ) ;	
		iDayOfWeek = atoi( szrgElement[0] ) ;
		iHour = atoi( szrgElement[1] ) ;
		iMinute = atoi( szrgElement[2] ) ;
		iSecond = atoi( szrgElement[3] ) ;
		uiFreq = atoi( szrgElement[4] ) ;
		iEvent = atoi( szrgElement[5] ) ;

		
		if( 0 >= iEvent )
		{
			assert( !"[TNScheduler] Check the schedule! -> The event to be triggered is equal to 0" ) ;
			continue ;
		}

		int iDay = iDayOfWeek - g_kSystemTime.wDayOfWeek ;
		if( 0 > iDay ) iDay += 7 ;
		--iDay ;

		if( g_kSystemTime.wDayOfWeek == iDayOfWeek ) // 서버 startup 날짜와 동일하다.
		{
			if( (g_kSystemTime.wHour>iHour) || ( (g_kSystemTime.wHour==iHour)&&(g_kSystemTime.wMinute>iMinute) ) || ( (g_kSystemTime.wHour==iHour)&&(g_kSystemTime.wMinute==iMinute)&&(g_kSystemTime.wSecond>iSecond) ) )
				continue ;
		}

		unsigned int uiGap = iDay*eTerm_Day + (23-g_kSystemTime.wHour+iHour)*eTerm_Hour + (59-g_kSystemTime.wMinute+iMinute)*eTerm_Minute + (60-g_kSystemTime.wSecond+iSecond)*eTerm_Second ;
		// 시간 gap을 계산해서 uiTime에 trigger시간을 입력해준다.

		int iRes = AddSchedule( uiNow, uiGap, uiFreq, iEvent );
		if( iRes )
		{
			assert( !"[TNScheduler] Check the schedule.(schedule.txt)" ) ;
			exit( 0 );

		}

		//m_krgSchedule[iSeq].uiTime = uiNow + uiGap ;		
		//m_krgSchedule[iSeq].uiFreq = uiFreq ;
		//m_krgSchedule[iSeq].iEvent = iEvent ;

		//{
		//	SYSTEMTIME st ;
		//	GetLocalTime( &st ) ;

		//	char chBuf[512] = { 0,0,0, } ;
		//	sprintf(chBuf, "\t%d. DayOfWeek:%d, Hour:%d, Minute:%d, Second:%d, Freq:%d, Event:%d \r\n"
		//		, iSeq, iDayOfWeek, iHour, iMinute, iSecond, uiFreq, iEvent ) ;
		//	WriteLog( chBuf, ".\\Monster_Log\\[Log]LoadSchedule.txt" ) ;
		//}

		++iSeq ;
		if( eSchedule_MaxCount <= iSeq )
		{
			assert( !"[TNScheduler] Check the count of schedule.(schedule.txt; Max 272)" ) ;
			exit( 0 );
		}
	}

	{
		char chBuf[512] = { 0,0,0, } ;
		sprintf(chBuf, "\n\n\n[%d월%d일%d요일 %d시%d분%d초] Scheduler.Load() > End\r\n"
			, g_kSystemTime.wMonth, g_kSystemTime.wDay, g_kSystemTime.wDayOfWeek, g_kSystemTime.wHour, g_kSystemTime.wMinute, g_kSystemTime.wSecond ) ;
		WriteLog( chBuf, ".\\Monster_Log\\[Log]LoadSchedule.txt" ) ;
	}

	
	//m_iLast = iSeq ;

	fclose( fin ) ;
}


