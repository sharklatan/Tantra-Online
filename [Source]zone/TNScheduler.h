/****************************************************************************************

	파일명 : TNScheduler.h
	작성자 : 정재웅(spencerj@korea.com)
	작성일 : 2004-10-19

	수정자 :
	수정일 :

	프로젝트명 : 

	설명 : 
	
	보완사항 : 등록되어 있는 schedule이 1시간 주기로 반복적으로 실행한다.(cycle 형태)
	           임시적으로 등록되어 수행되는 event들은 1회만 수행이 될 필요가 있다.
			   이런 수행에 있어서의 불일치로 인해 실제 제작에 불편이 있다.

****************************************************************************************/
#ifndef __TNScheduler_h__
#define __TNScheduler_h__

struct TNSchedule
{
	unsigned int uiTime ; // 0이면 disable 상태이다.
	unsigned int uiFreq ; // uiTerm이 0이면 오직 1회만 수행하는 것이다.
	int iEvent ; // 0이면 등록된 event가 없는 것이다.
} ;


class TNScheduler
{
public :
	TNScheduler() ;
	~TNScheduler() ;

	void Init() ;

	enum { eSchedule_MaxCount = 512, eTerm_Day = 86400000, eTerm_Hour = 3600000, eTerm_Minute = 60000, eTerm_Second = 1000, } ;

// Public Operations
public :
	void Load() ;
	void CheckNRunSchedule( unsigned int a_uiNow ) ;
	int AddSchedule( unsigned int a_uiNow, unsigned int a_uiGap, unsigned int a_uiFreq, int a_iEvent ) ;

// Public Properties
public :
	

private :	
	TNSchedule m_krgSchedule[eSchedule_MaxCount] ;
	int m_iLast ;
} ;


extern TNScheduler g_kScheduler ;

#endif //__TNScheduler_h__