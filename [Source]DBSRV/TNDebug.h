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

void WriteLog( const char* pLog, const char* chFileName ) ;
void TimeWriteLog( const char* pLog, const char* chFileName ) ;

void WriteEditHistory(const char* pszFilePath, const char* pszName, unsigned int nIP);

#endif //__TNDebug_h__