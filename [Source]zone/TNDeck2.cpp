/****************************************************************************

	파일명 : TNDeck2.cpp

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

#include "TNDeck2.h"


TNDeck2::TNDeck2() : m_iIndex(0), m_iSize(100)
{
}



TNDeck2::~TNDeck2()
{
}


void TNDeck2::Init()
{
	m_iSize = 100 ;
	m_iIndex = 0 ;
	memset( m_chrgCard, m_iSize, sizeof(char) ) ;
}


void TNDeck2::AddCard( char a_chNum, int a_iCount )
{
	for( int i = 0 ; i < a_iCount ; ++i )
	{
		if( m_iSize <= m_iIndex ) return ;
		m_chrgCard[m_iIndex] = a_chNum ;
		++m_iIndex ;
	}
}


void TNDeck2::Shuffle()
{
	for( int i = 0 ; i < m_iSize ; ++i )
	{
		int iLoc = rand() % m_iSize ;
		char chTemp = m_chrgCard[i] ;
		m_chrgCard[i] = m_chrgCard[iLoc] ;
		m_chrgCard[iLoc] = chTemp ;
	}
	m_iIndex = 0 ;
}



char TNDeck2::Random()
{
	char chCard = m_chrgCard[m_iIndex] ;
	++m_iIndex ;
	if( m_iIndex >= m_iSize ) Shuffle() ;
	return chCard ;
}


void TNDeck2::ClearCards()
{
	memset( m_chrgCard, m_iSize, sizeof(char) ) ;
	m_iSize = 100 ;
	m_iIndex = 0 ;
}

