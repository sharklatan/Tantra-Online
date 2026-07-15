#include "stdafx.h"

#include <stdio.h>
#include "BaseDef.h"
#include "DrawInfo.h"
#include "Server.h"

#define TEXT_ROW_POINTS 20

int g_nCurrentTextY = 10;

extern LONG g_nChargedJob;
extern LONG g_nWorkingThreades;

void PrintText( HDC hDC, COLORREF crText, char *pszFormat, ... );
void LineFeed();

void DrawInformations( HDC hDC )
{
	if ( hDC == NULL ) return;
	PrintText( hDC, RGB(128, 128, 0), "SQL DAEMON");
	LineFeed();
	PrintText( hDC, RGB(0, 0, 255), "Constants" );
	PrintText( hDC, RGB(0, 0, 0), "MAX Threads: %d",  MAX_THREAD );
	PrintText( hDC, RGB(0, 0, 0), "MAX Jobs: %d", MAX_JOB );
	LineFeed();
	PrintText( hDC, RGB(0, 0, 255), "Variables" );
	PrintText( hDC, RGB(0, 0, 0), "Working Threads : %d/%d", g_nWorkingThreades, MAX_THREAD );
	PrintText( hDC, RGB(0, 0, 0), "Charged Job Queue : %d/%d", g_nChargedJob, MAX_JOB );
}			

void PrintText( HDC hDC, COLORREF crText, char *pszFormat, ... )
{
	static char szText[512] = {0,};
	static COLORREF crCur = RGB( 0, 0, 0 );

	va_list vl;
	va_start( vl, pszFormat );
    vsprintf( szText, pszFormat, vl );
	if ( crText != crCur ) 
	{
		SetTextColor( hDC, crText );
		crCur = crText;
	}
	TextOut( hDC, 10, g_nCurrentTextY, szText, strlen(szText) );
	va_end( vl );

	g_nCurrentTextY += TEXT_ROW_POINTS;
}

void LineFeed()
{
	g_nCurrentTextY += TEXT_ROW_POINTS;
}