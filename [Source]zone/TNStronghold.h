/****************************************************************************************

	�ۼ��� : �����(spencerj@korea.com)
	�ۼ��� : 2003-09-09

	������ :
	������ :

	������Ʈ�� : 

	���� : 

****************************************************************************************/
#ifndef __TNStronghold_h__
#define __TNStronghold_h__


class TNStronghold
{
public :
	TNStronghold() ;
	~TNStronghold() ;
	

	void Init( int a_iMax ) ;
	enum { eDeck_MaxSize = 10000, };

//Public Operations
public :
	char Random() ;
	void Shuffle() ;
	void AddCard( char a_chCard, int a_iCount ) ;
	void ClearCards() ;
	

private :
	int m_iID ;
	int m_iOwner ; // guild ID
	int m_iSize ;
	int m_iIndex ;
	char* m_pCards ;
};




#endif