/****************************************************************************************

	작성자 : 정재웅(spencerj@korea.com)
	작성일 : 2003-10-09

	수정자 :
	수정일 :

	프로젝트명 : 

	설명 : 

****************************************************************************************/
#ifndef __TNPointBar_h__
#define __TNPointBar_h__

#include <windows.h>
#define MAX_INT	2000000000

class TNPointBar
{
public :
	TNPointBar() ;
	~TNPointBar() ;

	void Init( int* a_pCur ) ;
	void Init( int a_iMax, int a_iCur ) ;

	enum { eMot_Sit = 0, eMot_Stand = 1, } ;
//Public Operations
public :
	
	int Inc( int a_iPoint ) ;
	int Dec( int a_iPoint ) ;

//Public Properties
public :
	inline int get_Cur() { return m_iCur ; }
	inline int get_Max() { return m_iMax ; }
	inline bool IsFull() { if( m_iMax == m_iCur ) return true ; return false ; }
	inline bool IsZero() { if( 0 >= m_iCur ) return true ; return false ; }
	void set_Cur( int a_iCur ) ;
	void set_Max( int a_iMax ) ;
	void set_Recovery( int a_iStand, int a_iSit ) { m_irgRecv[eMot_Stand] = a_iStand ; m_irgRecv[eMot_Sit] = a_iSit ; }
	int get_Recovery( int a_iMotion = TNPointBar::eMot_Stand ) { return m_irgRecv[a_iMotion] ; }

protected :
	int m_iMax ;
	int m_iCur ;
//	int* m_pCur ;

	int m_irgRecv[2] ;

	//CRITICAL_SECTION m_cs ;
};



#endif