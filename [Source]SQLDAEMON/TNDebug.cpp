/****************************************************************************

	파일명 : TNDebug.h

	작성자 : 정재웅(spencerj@korea.com)
	작성일 : 2003-11-22

	Tab size : 4 spaces
	프로젝트명 : Tantra


	설명 : 
		-  
		- 

****************************************************************************/

#include "TNDebug.h"


void WriteLog( char* pLog, char* chFileName )
{	
	HANDLE		hFile;
	DWORD		dwRetWrite;
	int			nBufLength;

	CreateDirectory( "\\Data", NULL) ;
	nBufLength	= strlen(pLog);
	hFile = CreateFile(chFileName,					  // file name
                                GENERIC_WRITE|GENERIC_READ,   // open for writing 
                                FILE_SHARE_WRITE,             // do not share 
                                NULL,                         // no security 
                                OPEN_ALWAYS,                  // open or create 
                                FILE_ATTRIBUTE_NORMAL,        // normal file 
                                NULL);

	if(hFile == INVALID_HANDLE_VALUE)
	{
	}
	else
	{
		SetFilePointer(hFile, 0, NULL, FILE_END);
		WriteFile(hFile, pLog, nBufLength, &dwRetWrite, NULL);
		CloseHandle(hFile);
	}
}


