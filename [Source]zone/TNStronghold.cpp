/****************************************************************************

	파일명 : TNStronghold.cpp

	작성자 : 정재웅(spencerj@korea.com)
	작성일 : 2003-09-15

	Tab size : 4 spaces
	프로젝트명 : Tantra


	설명 : 
		- 
		- 

****************************************************************************/
#include <stdlib.h>
#include <string.h>

#include "TNStronghold.h"


TNStronghold::TNStronghold() : m_iIndex(0), m_iSize(0)
{
}



TNStronghold::~TNStronghold()
{
	if( NULL != m_pCards ) delete [] m_pCards ;
}


void TNStronghold::Init( int a_iMax )
{
	if( eDeck_MaxSize < a_iMax ) a_iMax = eDeck_MaxSize ;
	m_iSize = a_iMax ;
	m_iIndex = 0 ;
	m_pCards = new char[m_iSize] ;
	memset( m_pCards, 0, sizeof(m_pCards) ) ;
}


void TNStronghold::AddCard( char a_chCard, int a_iCount )
{
	if( NULL == m_pCards ) return ;
	for( int i = 0 ; i < a_iCount ; ++i )
	{
		if( m_iSize <= m_iIndex ) return ;
		m_pCards[m_iIndex] = a_chCard ;
		++m_iIndex ;
	}
}


void TNStronghold::Shuffle()
{
	if( NULL == m_pCards ) return ;
	if( 0 >= m_iSize ) return ;
	for( int i = 0 ; i < m_iSize ; ++i )
	{
		int iLoc = rand() % m_iSize ;
		char chTemp = m_pCards[i] ;
		m_pCards[i] = m_pCards[iLoc] ;
		m_pCards[iLoc] = chTemp ;
	}
	m_iIndex = 0 ;
}



char TNStronghold::Random()
{
	if( NULL == m_pCards ) return 0 ;
	if( 0 >= m_iSize ) return 0 ;
	if( 0 > m_iIndex || m_iIndex >= m_iSize ) return 0 ;

	char chCard = m_pCards[m_iIndex] ;
	++m_iIndex ;
	if( m_iIndex >= m_iSize ) Shuffle() ;
	return chCard ;
}


void TNStronghold::ClearCards()
{
	memset( m_pCards, 0, sizeof(m_pCards) ) ;
	m_iSize = 0 ;
	m_iIndex = 0 ;
}

