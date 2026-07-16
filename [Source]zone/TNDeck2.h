/****************************************************************************************

	작성자 : 정재웅(spencerj@korea.com)
	작성일 : 2003-09-09

	수정자 :
	수정일 :

	프로젝트명 : 

	설명 : TNDeck class를 size 100으로만 고정해서 static으로 만들어놓은 것.

****************************************************************************************/
#ifndef __TNDeck2_h__
#define __TNDeck2_h__



class TNDeck2
{
public :
	TNDeck2() ;
	~TNDeck2() ;
	
	void Init() ;

//Public Operations
public :
	char Random() ;
	void Shuffle() ;
	void AddCard( char a_chNum, int a_iCount ) ;
	void ClearCards() ;
	

private :
	int m_iIndex ;
	int m_iSize ;

	char m_chrgCard[100] ;
};




#endif