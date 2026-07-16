/****************************************************************************************

	작성자 : 정재웅(spencerj@korea.com)
	작성일 : 2003-09-09

	수정자 :
	수정일 :

	프로젝트명 : 

	설명 : 

// 상징물
     - static : (x,y), point
     - dynamic : owner, clan(or tribe)
// 공성전이 시작하면, 지원자 data를 분석해서 entry table을 생성한다.
// entry table : 길드 ID, clan
// entry size : 200 = (수성*1 + 공성*4) * 20(각 위치당 최대 slot수) * 2(연합개수)
// 공성전 zone에 접속을 하게 되면 entry table에서 자신의 길드 ID를 검색해서 clan을 결정한다.(공성전 개시)
// 검색 결과, entry에 없으면 무소속 clan을 가지게 된다.
// 지속적인 entry 등록 관리
// 최종 etnry table 확정

// PC 접속 시, 공성 상태여부에 따라서 clan 설정을 변경시켜준다.
  - 일반 상태일 때는 default를 고수한다.
  - 공성 상태일 때는 the entry of siege를  검색해서 clan을 변경시켜준다.

// 공성전의 단계?
  - 공성전 일정 공지

  - 공성전 신청
	'신청 UI'를 통해서 참가 guild를 신청 받음. => client/server 간의 참가 신청 메시지 & 처리 routine
	'신청 UI' 마감은 언제까지? 영범씨한테 전달되는 날짜는? 나와 연계작업을 할 수 있는 날짜는?
	MSG_APPLY_SIEGE

  - 공성전 신청 마감

  - 공성전 준비 단계
    - 개시 이전에 30분 전쯤에 모든 PC를 save zone으로 이동시킨다.(or 특정 위치로 이동을 시킨다. 특정 위치가 충분히 넓어야하는 조건이 만족해야한다.) => event/task
	- 전체 zone에 safety property 추가(재접속을 해서도 서로 공격이 안되게 한다.) => event/task
	- 공성전 관련 모든 객체에 all immunity를 추가해준다.(이것은 모든 field에 안전 속성을 넣었기 때문에 안해도 될 듯하다. 확인 필요!) => event/task
	- 재접속하는 user들에게 zone settings 메시지 전송(공성전에 대한 bit flag)
	  // client targeting 변경
	  MSG_SET_ZONE_SETTINGS

  - 공성전 개시
    - 전체 zone에서 safety property 제거 => event/task
	- 상징물 몬스터 죽을 때, 이벤트를 call 해서 다른 상징물을 pop 하게 한다.(상징물 소유에 대한 정보를 이용, 10초 이상의 delay가 있어야 한다.) => monster AI & event/task
		// symbol이 죽은 후에 바로(즉시) pop 된다면 어색할 듯하다. 그래서 delay가 10초 이상 있어야 한다. 따라서 예약하기 위한 방법이 제공되어야 한다.
		// 35(=7*5) 개의 event/task
		// 35개의 event를 1분 후에 예약하기 위한 event/task
		=> eTNAct_PopSymbolForSiege(21020)
	
  - 공성전 종료	
	- 모든 지역을 safety field로 변경해서 더 이상 전투가 진행되지 않게 한다. => event/task
	- 종료 공지 => event/task
	- 종료 시상에 대한 예약 => event/task
	
  - 공성전 시상    
    - 상징물 소유에 대한 계산을 수행해서 승자를 결정한다. => event/task, eTNAct_JudgeTheSiege(21010)
		각 clan의 점수를 모두 출력을 해주고 최후의 승자를 언급한다.
		출력을 시스템 내부에서 처리한다.
    - 모든 PC를 save zone으로 이동시킨다.(성 owner만을 남기고 모든 다른 PC를 save zone으로 이동시킬 수 있다.) => event/task
		성 owner만을 남기고 나머지를 옮기기 위한 routine이 필요 => eTNAct_KickOutLosser(21000)
    - 원래 safety field 설정 but 어렵다.
	- zone에 남아 있는 모든 PC들의 clan을 일반 clan으로 변경한다. => eTNAct_RecoverClanToOriginal(21030)
	=> 누구를 어떤 clan으로 변경하는 generic routine을 만든다.
	MSG_CHANGE_TRIMURITI

  - 공성전에 대한 메리트 부분 개발
	????

  - 부활에 대해서는 윤선임님과 작업
	// 진지에 따른 부활 장소 가변화, 진지 설치에 대한 특수조건?
	
=> 존에 접속해 있는 모든 PC들의 clan을 강제로 변경해주는 routine이 있어야 한다. => ChangePCClanAll()

* 무적이 공성전에서 이겨서 성이 무소유 상태로 되는 것에 대한 집중적인 테스팅이 필요하다.

****************************************************************************************/
#ifndef __TNSiege_h__
#define __TNSiege_h__

// 요새전과 많이 비슷할 수 있다.

// if( eCountryKorea == g_eCountryID ) strncpy( kMsg.szMsg, g_pMessageStringTable[_KickedByGM], sizeof(kMsg.szMsg)) ;
// else


struct TNGUILD_INFO
{
	int iID ;
	DWORD dwMark ;
	char szName[SZGUILD_LENGTH] ;
} ;

struct TNCastle
{
	//int iOwner ; // guild id
	//char	szGuildName[SZNAME_LENGTH] ;
	//DWORD	dwMark;

	TNGUILD_INFO kGuild ;
	int		iOwnerFriend;
	CTime	kTimeOccupied ;
} ;




class TNSiege
{
public :
	TNSiege() ;
	~TNSiege() ;

	void Init() ;

/*
수성 상징물	K-3	2435 
공성1 상징물	K-3	2436 
공성2 상징물	K-3	2437 
공성3 상징물	K-3	2438 
무소속 상징물	K-3	2439 
*/

	enum { eSiege_SymbolCount = 11, eSiege_MaxEntry = 40, eSiege_Army = 4, } ; // 공성전 관련
	enum { eSide_Defense = 0, } ;
	enum { eSmblTrb_CastleOwner = 2435 , eSmblTrb_Symbol1 = 2436, eSmblTrb_Symbol2 = 2437, eSmblTrb_Symbol3 = 2438, eSmblTrb_Symbol4 = 2439, } ;
	enum { eApplyFee_Defense = 1000000, eApplyFee_Siege1 = 12000000, eApplyFee_Siege2 = 10000000, eApplyFee_Siege3 = 8000000, eApplyFee_SiegeSupport = 1000000, } ;
	enum { eDate_NotSelected = 0, eDate_Friday8HH, eDate_Friday10HH, eDate_Saturday8HH, eDate_Saturday10HH, eDate_Sunday8HH, } ;

// Public Operations
public :
	int GetEntry( int a_iClanSlot, int a_iExpandSlot, TNGUILD_INFO& a_kGuild ) ;
	int FindFreeSlot( int a_iClanSlot ) ;
	int LoadData() ;
	//int BuildEntry() ;
	void InitEntry() ;
	int RegisterEntry( int a_iGuildID, int a_iClanSlot, int a_iExpandSlot, int a_iPC ) ;
	int SearchEntry( int a_iGuildID ) ; // #clan. if failed, return -1 ;
	int GiveUpSiege( int a_iGuildID );
	int RemoveEntry( int a_iGuildID );
	int ChangePCClanAll( int a_iClan ) ; // 현재 접속하고 있는 모든 PC의 clan을 변경해준다. 인자가 0이면, SearchEntry()를 이용해서 자신의 clan을 확인한다. 0이 아니면, 모든 PC를 그 #clan으로 변경, -1이면 고유 clan으로 복귀
	void CheckSymbols() ;
	int CaptureSymbol( int a_iClan, int a_iSymbol, int a_iCapturer ) ; // 어떤 clan이 어떤 symbol을 빼앗았다.
	void JudgeSiege() ;
	void SelectDate( int a_iDateNum, bool a_bCheck=true ) ;
	void InstallSymbols() ; // symbol들을 pop 시키고 handle을 유지한다.
	void DestroySymbols() ;
	void Print() ;
	int SaveData( char* a_pFileName =".\\Data\\Castle.txt" ) ;
	void RefreshEntry();

private :
	int ChangeOwner( int a_iGuildID ) ;
	
	

// Public Properties
public :
	inline int get_Started() { return m_iStarted ; }
	void set_Started( int a_iFlag ) ; //{ m_iStarted = a_iFlag ; } // 1이면 공성전 설정, 0이면 공성전 설정 해제
	inline int get_Winner() { return m_iWinner ; } 
	inline int get_DateToHold() { return m_iDateToHold ; }
	inline void set_ExpiryOftheTerm( int a_iFlag ) { m_iExpiryOftheTerm = a_iFlag ; } // 1이면 기간만료(공성신청불가), 0이면 기간중(공성신청가능)
	inline int get_ExpiryOftheTerm() { return m_iExpiryOftheTerm ; }
	void get_Symbols( int* a_irgSymbol ) ;
	void get_Owner( TNCastle* a_kOwner ) { (*a_kOwner) = m_kOwner ; } ;
	int get_OwnerGuild() { return m_kOwner.kGuild.iID; }
	void RegisterCastleOwner() ;
	
// Attributes
private :

	int m_iStarted ; // 공성전 시작 여부
	int m_iExpiryOftheTerm ; // 공성신청기간 만료
	int m_iWinner ; // clan 정보, 19~23
	TNCastle m_kOwner ; //  소유자 정보
	//int m_irgEntry[eSiege_Army][eSiege_MaxEntry] ; // owner, clan // 공성전이 시작될 때, 확정된다.
	TNGUILD_INFO m_krgEntry[eSiege_Army][eSiege_MaxEntry] ;
	//char m_sgrgLeader[eSiege_Army+1][SZNAME_LENGTH] ;
	//int m_irgLeader[eSiege_Army] ; // 공성측의 leader들 목록
	int m_irgSymbol[eSiege_SymbolCount][2] ; // handles of the symbol // handle을 관리해야지 여러 가지로 사용할 수 있다.
	//int m_irgSymbolClan[eSiege_SymbolCount] ; // 각 symbol들의 clan, 이것이 위의 m_irgSymbol보다 더욱 중요한 정보
	int m_iDateToHold ; // 공성전하는 시간, 0/1/2/3(0은 default로 선정하지 않은 것이다.)
};



extern TNSiege g_kSiege ;

#endif