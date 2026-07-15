#include "stdafx.h"
#include <windows.h>
#include <Sql.h>
#include <Sqlext.h>
#include <Sqltypes.h>

#include "CSQL.H"

CSQL::CSQL()
{
	g_henv	=	NULL;
	g_hdbc	=	NULL;
	g_hstmt	=	NULL;
}
CSQL::~CSQL()
{
	DBRelease();
}

BOOL CSQL::DBConnect(unsigned char * Server,unsigned char * User,unsigned char* Pass)
{
	SQLRETURN ret;
	ret = SQLAllocHandle	(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &g_henv);				if	(ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) return FALSE;
	ret = SQLSetEnvAttr		(g_henv,SQL_ATTR_ODBC_VERSION,(void*)SQL_OV_ODBC3, 0);	if	(ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) return FALSE;
	ret = SQLAllocHandle	(SQL_HANDLE_DBC, g_henv, &g_hdbc);						if	(ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) return FALSE;
	ret = SQLSetConnectAttr	(g_hdbc,SQL_LOGIN_TIMEOUT,(SQLPOINTER)5, 0);			if	(ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) return FALSE;
	ret = SQLConnect		(g_hdbc,Server,SQL_NTS,User,SQL_NTS,Pass,SQL_NTS);		if	(ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) return FALSE;
	return TRUE;
} 
void CSQL::DBRelease()
{
	if (g_hdbc)  SQLDisconnect(g_hdbc);
	if (g_hdbc)  SQLFreeHandle(SQL_HANDLE_DBC, g_hdbc);
	if (g_henv)	 SQLFreeHandle(SQL_HANDLE_ENV, g_henv);
	if (g_hstmt) SQLFreeHandle(SQL_HANDLE_STMT, g_hstmt);
}
	
// szQuery = "select	map_name map_info map_size init_money, need_money1, need_money2 from maplist order by map_name";
BOOL CSQL::ExecuteStart(char * szQuery)
{	
	SQLRETURN	ret;
	ret = SQLAllocHandle(SQL_HANDLE_STMT, g_hdbc, &g_hstmt);	if	(ret!=SQL_SUCCESS && ret!=SQL_SUCCESS_WITH_INFO)	return FALSE;
	ret = SQLPrepare(g_hstmt, (SQLCHAR*)szQuery, SQL_NTS);		if	( ret!=SQL_SUCCESS && ret!=SQL_SUCCESS_WITH_INFO )	return FALSE;
	ret = SQLExecute(g_hstmt);									if	( ret!=SQL_SUCCESS && ret!=SQL_SUCCESS_WITH_INFO )	return FALSE;
	return TRUE;
}

BOOL CSQL::GetStmt(void)
{	SQLRETURN	ret;
	ret = SQLAllocHandle(SQL_HANDLE_STMT, g_hdbc, &g_hstmt);	if	(ret!=SQL_SUCCESS && ret!=SQL_SUCCESS_WITH_INFO)	return FALSE;
	return TRUE;
}
BOOL CSQL::FreeStmt(void)
{	if (g_hstmt) SQLFreeHandle(SQL_HANDLE_STMT /*|SQL_RESET_PARAMS */, g_hstmt);
	g_hstmt=NULL;
	return TRUE;
}

BOOL CSQL::ExecuteEnd()
{	SQLFreeHandle(SQL_HANDLE_STMT, g_hstmt);
	return TRUE;
}


/*

	#define NAME_LEN 50
	#define PHONE_LEN 10

	SQLCHAR		szName[NAME_LEN], szPhone[PHONE_LEN];
	SQLINTEGER	sCustID, cbName, cbCustID, cbPhone;
	SQLHSTMT	hstmt;
	SQLRETURN	retcode;

	retcode = SQLExecDirect(hstmt,"SELECT CUSTID, NAME, PHONE FROM CUSTOMERS ORDER BY 2, 1, 3",SQL_NTS);
	if	(retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) 
	{

		SQLBindCol(hstmt, 1, SQL_C_ULONG, &sCustID, 0, &cbCustID);
		SQLBindCol(hstmt, 2, SQL_C_CHAR, szName, NAME_LEN, &cbName);
		SQLBindCol(hstmt, 3, SQL_C_CHAR, szPhone, PHONE_LEN, &cbPhone);
		while (TRUE) 
		{	retcode = SQLFetch(hstmt);
			if	(retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO) 
			{	show_error();
			}
			if	(retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
			{	fprintf(out, "%-*s %-5d %*s", NAME_LEN-1, szName,sCustID, PHONE_LEN-1, szPhone);
			}	else 
			{	break;
			}
		}
	}
*/

BOOL CSQL::IsUserIn(BYTE byWorld, char * pAccountID)
{
	//char szBuf[256]={0,};
	//SQLRETURN	retcode;

	//sprintf(szBuf, "Select * from %s%02d where UserID = '%s'", DBTABLE, byWorld, pAccountID);
	//retcode = SQLExecDirect(g_hstmt, szBuf, SQL_NTS);
	//if	(retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) 
	//{
	//	SQLBindCol(hstmt, 1, SQL_C_ULONG, &sCustID, 0, &cbCustID);
	//	SQLBindCol(hstmt, 2, SQL_C_CHAR, szName, NAME_LEN, &cbName);
	//	SQLBindCol(hstmt, 3, SQL_C_CHAR, szPhone, PHONE_LEN, &cbPhone);
	//	while (TRUE) 
	//	{	retcode = SQLFetch(g_hstmt);
	//		if	(retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO) 
	//		{	show_error();
	//		}
	//		if	(retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
	//		{	fprintf(out, "%-*s %-5d %*s", NAME_LEN-1, szName,sCustID, PHONE_LEN-1, szPhone);
	//		}	else 
	//		{	break;
	//		}
	//	}
	//}

	return TRUE;
}