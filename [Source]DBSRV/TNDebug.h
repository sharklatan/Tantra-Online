/****************************************************************************************

	파일명 : TNDebug.h
	작성자 : 정재웅(spencerj@korea.com)
	작성일 : 2003-11-22

	수정자 :
	수정일 :

	프로젝트명 : 

	설명 : 

****************************************************************************************/
#ifndef __TNDebug_h__
#define __TNDebug_h__

#include <windows.h>

void WriteLog( char* pLog, char* chFileName ) ;
//void TimeWriteLog( char* pLog, char* chFileName ) ; //antes TimeWriteLog(const char* pszText, const char* pszFilePath);
void TimeWriteLog(const char* pszText, const char* pszFilePath); //se implemento pero comento rompia compilacion mal implementada

#endif //__TNDebug_h__