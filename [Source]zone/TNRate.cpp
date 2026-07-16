/****************************************************************************

	파일명 : TNRate.cpp

	작성자 : 정재웅(spencerj@korea.com)
	작성일 : 2003-09-15

	Tab size : 4 spaces
	프로젝트명 : Tantra


	설명 : 
		- 
		- 

****************************************************************************/
#include <stdlib.h>

#include "TNRate.h"
#include "string.h"

#if defined(__ZONE_SERVER__) && defined(__MEMORYMANAGER__)

#ifndef _HTMEMORYMANAGER_H_
#include "HTMemoryManager.h"
#endif

#endif //__ZONE_SERVER__, __MEMORYMANAGER__

TNRate::TNRate() : m_iRate(0), m_iMaxRate(0), m_iIndex(0), m_chOffset(0), m_sSize(0)//, m_pFlags(NULL)
	, m_sRange(0)
{
}



TNRate::~TNRate()
{
	//if( NULL != m_pFlags ) delete [] m_pFlags ;
}



void TNRate::Init( int a_iRate, int a_iMaxRate )
{
	m_iRate = a_iRate ;
	m_iMaxRate = a_iMaxRate ;
	if( 100 < m_iMaxRate ) m_iMaxRate = 100 ;
	if( 0 >= m_iRate ) return ; // 0% 확률을 가진다.
	if( m_iRate >= m_iMaxRate )
	{
		m_iRate = m_iMaxRate ; // 100% 확률을 가진다.
		return ;
	}

	//m_sRange = (m_iMaxRate + m_iRate/2) / m_iRate ;

	double dTemp = m_iMaxRate / m_iRate ;
	m_sRange = (short)dTemp ;
	if( 0.5 <= (double)(dTemp - m_sRange) ) ++m_sRange ;

	if( 0 >= m_sRange )
	{
		m_iRate = 0 ;
		return ;
	}	

	m_sSize = m_sRange / 8 ;
	if( 0 != m_sRange % 8 ) ++m_sSize ;
	//if( NULL == m_pFlags ) m_pFlags = new char[m_sSize] ;
	m_chLastOffset = 0x01 ;
	int iLast = m_sRange % 8 ;
	if( 0 != iLast )
	{
		for( int k = 0 ; k < m_sRange ; ++ k ) m_chLastOffset = m_chLastOffset << 1 ;
	}

	Rolling() ;
}



void TNRate::Rolling()
{
	if( 0 >= m_iRate ) return ;
	if( m_iRate >= m_iMaxRate ) return ;
	//if( NULL == m_pFlags ) return ;

	//for( int i = 0 ; i < m_sSize ; ++i ) m_pFlags[i] = 0 ;
	memset( m_chrgFlag, 0, sizeof(m_chrgFlag) ) ;
	m_iIndex = rand() % m_sSize ;
	int iRange = 8 ;
	if( m_iIndex == (m_sSize-1) ) iRange = m_sRange - (8*(m_sRange/8)) ;
	if( 0 == iRange ) iRange = 8 ;
	int iOffset = rand() % iRange ;
	char chIndex = 0x01 ;
	chIndex = chIndex << iOffset ;
	//m_pFlags[m_iIndex] = chIndex ;
	m_chrgFlag[m_iIndex] = chIndex ;
	m_iIndex = 0 ;
	m_chOffset = 0x01 ;
}



bool TNRate::Random()
{
	if( 0 >= m_iRate ) return false ;
	if( m_iRate >= m_iMaxRate ) return true ;
	//if( NULL == m_pFlags ) return false ;

	bool bFlag = false ;
	//if( m_pFlags[m_iIndex] & m_chOffset ) bFlag = true ;
	if( m_chrgFlag[m_iIndex] & m_chOffset ) bFlag = true ;
	m_chOffset = m_chOffset << 1 ;
	if( (m_iIndex == (m_sSize-1)) && (m_chLastOffset == m_chOffset) ) 
	{
		Rolling() ;
		return bFlag ;
	}

	if( 0x80 & m_chOffset )
	{
		m_chOffset = 0x01 ;
		++m_iIndex ;
		if( m_sSize <= m_iIndex ) Rolling() ;
	}

	return bFlag ;
}

