/****************************************************************************

	작성자 : 정재웅(spencerj@korea.com)
	작성일 : 2003-10-09

	Tab size : 4 spaces
	프로젝트명 : Tantra


	설명 : 
		- 
		- 

****************************************************************************/
#include <stdio.h>
#include "TNDebug.h"
#include "TNPointBar.h"

#if defined(__ZONE_SERVER__) && defined(__MEMORYMANAGER__)

#ifndef _HTMEMORYMANAGER_H_
#include "HTMemoryManager.h"
#endif

#endif //__ZONE_SERVER__, __MEMORYMANAGER__

//#define __TN_RECOVER_HEALTH_LOG__

TNPointBar::TNPointBar() : m_iMax(0), m_iCur(0)
{
//	InitializeCriticalSection( &m_cs ) ;
	
}



TNPointBar::~TNPointBar()
{
//	DeleteCriticalSection( &m_cs ) ;
}


void TNPointBar::Init( int a_iMax, int a_iCur )
{
	m_iMax = a_iMax ;
	set_Cur( a_iCur ) ;
}


void TNPointBar::set_Cur( int a_iCur ) 
{ 
	if( m_iMax < a_iCur ) a_iCur = m_iMax ; 
	else if( 0 > a_iCur ) a_iCur = 0 ;

	m_iCur = a_iCur ; 
}


void TNPointBar::set_Max( int a_iMax )
{ 
	m_iMax = a_iMax ;
	if( m_iMax < m_iCur ) m_iCur = m_iMax ; 
}


int TNPointBar::Inc( int a_iPoint )
{
	if( MAX_INT <= m_iCur ) return MAX_INT ;
	//if( 0 >= m_iCur ) return 0 ; // 한번 0이 된 HP는 일반적인 회복으로는 올라가지 않는다.

	//EnterCriticalSection( &m_cs ) ;
	m_iCur += a_iPoint ;
	if( m_iMax < m_iCur ) m_iCur = m_iMax ;
	//LeaveCriticalSection( &m_cs ) ;

	return m_iCur ;
}



int TNPointBar::Dec( int a_iPoint )
{
	//EnterCriticalSection( &m_cs ) ;
	m_iCur -= a_iPoint ;
	if( 0 > m_iCur ) m_iCur = 0 ;
	//LeaveCriticalSection( &m_cs ) ;

	return m_iCur ;
}

