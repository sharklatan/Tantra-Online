/****************************************************************************************

	작성자 : 정재웅(spencerj@korea.com)
	작성일 : 2005-12-27

	수정자 :
	수정일 :

	프로젝트명 : 

	설명 : 

****************************************************************************************/
#ifndef __TNMagicLamp_h__
#define __TNMagicLamp_h__


#include "TNDeck1000.h"


class TNMagicLamp
{
public :
	TNMagicLamp();
	~TNMagicLamp();

	void Init();
	

//Public Operations
public :
	int DrawMonsterCard();
	void AddMonsterCard( int a_iIndex, int a_iMonsterID, int a_iRating );
	void ShuffleMonsterCard();

	int LoadData( char* a_pFileName );

// Attributes
private :
	enum { eML_CountOfMonsterToBeSummon = 13, };

	TNDeck1000 m_kRating;
	int m_irgMonsterToBeSummon[eML_CountOfMonsterToBeSummon];


};

extern TNMagicLamp g_krgMagicLamp[5];


#endif