
#ifndef __CSQL__
#include <Sql.h>
#include <Sqlext.h>
#include <Sqltypes.h>


class CSQL
{
public:
	SQLHENV     g_henv;
	SQLHDBC     g_hdbc;
	SQLHSTMT    g_hstmt;

	CSQL();
	~CSQL();
	BOOL DBConnect(unsigned char * Server,unsigned char * User,unsigned char* Pass);
	BOOL ExecuteStart(char * szQuery);
	BOOL ExecuteEnd(void);
	void DBRelease();
	BOOL GetStmt(void);
	BOOL FreeStmt(void);
	BOOL IsUserIn(BYTE byWorld, char * pAccountID);

};
#endif
