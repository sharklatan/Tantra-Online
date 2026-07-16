/****************************************************************************************

	작성자 : 정재웅(spencerj@korea.com)
	작성일 : 2003-09-09

	수정자 :
	수정일 :

	프로젝트명 : 

	설명 : TNDeck class를 size 10000으로만 고정해서 static으로 만들어놓은 것.

****************************************************************************************/
#ifndef __TNDeck10000_h__
#define __TNDeck10000_h__

#include <windows.h>

class TNDeck10000
{
public :
	TNDeck10000() ;
	~TNDeck10000() ;
	
	void Init() ;
	enum { eDeck_MaxSize = 10000, };
//Public Operations
public :
	byte Random() ;
	void Shuffle() ;
	void AddCard( byte a_byNum, int a_iCount ) ;
	void ClearCards() ;
	

private :
	int m_iIndex ;
	//int m_iSize ;

	byte m_byrgCard[10000] ;
};




#endif