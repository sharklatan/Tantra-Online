// ============================================================
// CFileDB_NEW_funcs5.cpp  -- Fase 1: DB account/char I/O
// Reconstruidas de DBSRV_NEW.exe
// ============================================================

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include "Basedef.h"
#include "CFileDB.h"
#include "TNDebug.h"

// ============================================================
// Tamanios de registro extraidos del binario NEW
// ============================================================
#define ACCOUNT_RECORD_SIZE     0x1BD4   // sizeof STRUCT_ACCOUNTFILE
#define ACCOUNT2_RECORD_SIZE    0x129C   // version reducida
#define ONETIME_RECORD_SIZE     0x13C    // Check_account record
#define CHAR_HEADER_SIZE        0x34     // nombre account en .TCD

// Codigos errno del binario NEW (winsock errno map)
#define ERR_EEXIST  0x11
#define ERR_EACCES  0x0D
#define ERR_EINVAL  0x16
#define ERR_EMFILE  0x18
#define ERR_ENOENT  0x02

// Log helper
static void LogErrno(const char* pszPrefix, const char* pszAccount, int nErrno)
{
    const char* pszErr = "UNKNOWN";
    if      (nErrno == ERR_EEXIST) pszErr = "EEXIST";
    else if (nErrno == ERR_EACCES) pszErr = "EACCES";
    else if (nErrno == ERR_EINVAL) pszErr = "EINVAL";
    else if (nErrno == ERR_EMFILE) pszErr = "EMFILE";
    else if (nErrno == ERR_ENOENT) pszErr = "ENOENT";

    char szMsg[256] = {0};
    sprintf(szMsg, "%s %s", pszPrefix, pszErr);
    Log(szMsg, (char*)pszAccount, 0);
}

// ============================================================
// DBWriteAccount2  (VA 0x00433C80)
// Igual que DBWriteAccount pero usa ./account2/ y 0x129C bytes
// ============================================================
int CFileDB::DBWriteAccount2(STRUCT_ACCOUNTFILE* pData)
{
    const char* pszName = pData->AccountName;
    if (!pszName || !pszName[0])
    {
        Log("err writeacctount2 NULL_ACCOUNT", (char*)pszName, 0);
        return 0;
    }

    char szSub[4]    = {0};
    char szPath[256] = {0};
    GetSubDir(pszName, szSub, sizeof(szSub));
    sprintf(szPath, "./account2/%s/%s.TAD", szSub, pszName);

    char szDir[256] = {0};
    sprintf(szDir, "./account2/%s", szSub);
    CreateDirectoryA(szDir, NULL);

    int fd = _open(szPath, _O_RDWR | _O_CREAT | _O_BINARY, _S_IREAD | _S_IWRITE);
    if (fd == -1)
    {
        int err = errno;
        if      (err == EEXIST) Log("err writeaccount2 EEXIST", (char*)pszName, 0);
        else if (err == EACCES) Log("err writeaccount2 EACCES", (char*)pszName, 0);
        else if (err == EINVAL) Log("err writeaccount2 EINVAL", (char*)pszName, 0);
        else if (err == EMFILE) Log("err writeaccount2 EMFILE", (char*)pszName, 0);
        else if (err == ENOENT) Log("err writeaccount2 ENOENT", (char*)pszName, 0);
        else                    Log("err writeaccount2 UNKNOWN",(char*)pszName, 0);
        return 0;
    }

    if (_lseek(fd, 0, SEEK_END) == -1L)
    {
        Log("err writeaccount2 lseek fail", (char*)pszName, 0);
        _close(fd);
        return 0;
    }

    // 0x129C bytes (version reducida del record)
    int nWritten = _write(fd, pData, ACCOUNT2_RECORD_SIZE);
    _close(fd);

    if (nWritten == -1)
    {
        int err = errno;
        Log("CreateAccount2 write fail", (char*)pszName, 0);
        if      (err == EEXIST) Log("CreateAccount2 EEXIST", (char*)pszName, 0);
        else if (err == EACCES) Log("CreateAccount2 EACCES", (char*)pszName, 0);
        else if (err == EINVAL) Log("CreateAccount2 EINVAL", (char*)pszName, 0);
        else if (err == EMFILE) Log("CreateAccount2 EMFILE", (char*)pszName, 0);
        else if (err == ENOENT) Log("CreateAccount2 ENOENT", (char*)pszName, 0);
        return 0;
    }

    return 1;
}

// ============================================================
// DBWriteOneTimeAccount  (VA 0x00434D80)
// Escribe en ./Check_account/<sub>/<name>.TAD
// Ademas guarda el tick actual en pData[0x138] antes de escribir
// Escribe 0x13C bytes
// ============================================================
int CFileDB::DBWriteOneTimeAccount(void* pData)
{
    char* pszName = (char*)pData;
    if (!pszName || !pszName[0])
    {
        Log("err DBWriteOneTimeAccount NULL_ACCOUNT", pszName, 0);
        return 0;
    }

    // Guardar tick en offset 0x138 del mensaje (exacto del binario)
    // VA 0x488E28 = GetTickCount global del binario
    *((DWORD*)((char*)pData + 0x138)) = GetTickCount();

    char szSub[4]    = {0};
    char szPath[256] = {0};
    GetSubDir(pszName, szSub, sizeof(szSub));
    sprintf(szPath, "./Check_account/%s/%s.TAD", szSub, pszName);

    char szDir[256] = {0};
    sprintf(szDir, "./Check_account/%s", szSub);
    CreateDirectoryA(szDir, NULL);

    int fd = _open(szPath, _O_RDWR | _O_CREAT | _O_BINARY, _S_IREAD | _S_IWRITE);
    if (fd == -1)
    {
        int err = errno;
        if      (err == EEXIST) Log("err DBWriteOneTimeAccount EEXIST", pszName, 0);
        else if (err == EACCES) Log("err DBWriteOneTimeAccount EACCES", pszName, 0);
        else if (err == EINVAL) Log("err DBWriteOneTimeAccount EINVAL", pszName, 0);
        else if (err == EMFILE) Log("err DBWriteOneTimeAccount EMFILE", pszName, 0);
        else if (err == ENOENT) Log("err DBWriteOneTimeAccount ENOENT", pszName, 0);
        else                    Log("err DBWriteOneTimeAccount UNKNOWN",pszName, 0);
        return 0;
    }

    if (_lseek(fd, 0, SEEK_END) == -1L)
    {
        Log("err DBWriteOneTimeAccount lseek fail", pszName, 0);
        _close(fd);
        return 0;
    }

    int nWritten = _write(fd, pData, ONETIME_RECORD_SIZE);
    _close(fd);

    if (nWritten == -1)
    {
        int err = errno;
        Log("CreateAccount write fail", pszName, 0);
        if      (err == EEXIST) Log("CreateAccount EEXIST", pszName, 0);
        else if (err == EACCES) Log("CreateAccount EACCES", pszName, 0);
        else if (err == EINVAL) Log("CreateAccount EINVAL", pszName, 0);
        else if (err == EMFILE) Log("CreateAccount EMFILE", pszName, 0);
        else if (err == ENOENT) Log("CreateAccount ENOENT", pszName, 0);
        return 0;
    }

    return 1;
}

// ============================================================
// DBCreateChar  (VA 0x00432DF0)
// Crea ./char/<sub>/<charname>.TCD con el nombre de cuenta
// Parametros: pszAccountName, pszCharName
// Si el archivo ya existe: retorna 0 (no sobreescribir)
// Si no existe: crea y escribe pszAccountName (0x34 bytes)
// Retorna: 1 creado, 0 ya existe o error
// ============================================================
int CFileDB::DBCreateChar(const char* pszAccountName, const char* pszCharName)
{
    char szSub[4]    = {0};
    char szPath[256] = {0};
    GetSubDir(pszCharName, szSub, sizeof(szSub));
    sprintf(szPath, "./char/%s/%s.TCD", szSub, pszCharName);

    // Crear directorio
    char szDir[256] = {0};
    sprintf(szDir, "./char/%s", szSub);
    CreateDirectoryA(szDir, NULL);

    // Intentar abrir solo lectura: si existe, retornar 0
    int fd = _open(szPath, _O_RDONLY | _O_BINARY);
    if (fd != -1)
    {
        _close(fd);
        return 0;  // ya existe
    }
    _close(fd);

    // Crear nuevo archivo
    fd = _open(szPath, _O_CREAT | _O_WRONLY | _O_BINARY, _S_IREAD | _S_IWRITE);
    if (fd == -1)
    {
        int err = errno;
        if      (err == EEXIST) Log("err createchar EEXIST", (char*)pszCharName, 0);
        else if (err == EACCES) Log("err createchar EACCES", (char*)pszCharName, 0);
        else if (err == EINVAL) Log("err createchar EINVAL", (char*)pszCharName, 0);
        else if (err == EMFILE) Log("err createchar EMFILE", (char*)pszCharName, 0);
        else if (err == ENOENT) Log("err createchar ENOENT", (char*)pszCharName, 0);
        else                    Log("err createchar UNKNOWN",(char*)pszCharName, 0);
        return 0;
    }

    // Escribir nombre de cuenta como header (0x34 bytes)
    char szHeader[CHAR_HEADER_SIZE] = {0};
    strncpy(szHeader, pszAccountName, CHAR_HEADER_SIZE - 1);
    _write(fd, szHeader, CHAR_HEADER_SIZE);
    _close(fd);

    return 1;
}

// ============================================================
// DBDeleteChar  (VA 0x00433080)
// Hace backup del .TAD de cuenta y del .TCD del char
// Parametros:
//   pszCharName    -> nombre del personaje
//   pszAccountName -> nombre de la cuenta
//   nMode          -> 1=Delete normal, 2=retorna inmediato, 3=Rename
// Verifica edad del archivo: si < 5 dias (CountryID==5/TAIWAN) rechaza
// Retorna: 1 OK, 0 error, nMode si nMode==2
// ============================================================
int CFileDB::DBDeleteChar(const char* pszCharName, const char* pszAccountName, int nMode)
{
    char szCharSub[4]  = {0};
    char szCharPath[256] = {0};
    GetSubDir(pszCharName, szCharSub, sizeof(szCharSub));
    sprintf(szCharPath, "./char/%s/%s.TCD", szCharSub, pszCharName);

    // Obtener file time del .TCD
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAA fd2 = {0};
    hFind = FindFirstFileA(szCharPath, &fd2);
    if (hFind == INVALID_HANDLE_VALUE) return 0;
    FindClose(hFind);

    // Convertir a local time
    FILETIME ftLocal = {0};
    FileTimeToLocalFileTime(&fd2.ftLastWriteTime, &ftLocal);

    // Obtener tiempo actual
    SYSTEMTIME stNow;
    GetLocalTime(&stNow);
    FILETIME ftNow = {0};
    SystemTimeToFileTime(&stNow, &ftNow);

    // Si CountryID == 5 (TAIWAN): verificar que el archivo tenga al menos 5 dias
    // 5 dias en 100-nanosecond intervals = 5 * 24 * 3600 * 10000000 = 0x9A7EC800 * 1 (approx)
    if (g_nCountryID == 5)
    {
        ULONGLONG ullFile = ((ULONGLONG)ftLocal.dwHighDateTime << 32) | ftLocal.dwLowDateTime;
        ULONGLONG ullNow  = ((ULONGLONG)ftNow.dwHighDateTime  << 32) | ftNow.dwLowDateTime;
        const ULONGLONG FIVE_DAYS = 0x9A7EC800ULL;  // 5 dias en 100ns intervals
        if (ullNow - ullFile < FIVE_DAYS)
            return 0;  // demasiado reciente
    }

    // Obtener GetLocalTime para nombre de backup
    SYSTEMTIME st = {0};
    GetLocalTime(&st);
    int yy = st.wYear % 100;

    // Numero de DeleteFile
    int nDeleteCount = DeleteFileA(szCharPath) ? 1 : 0;
    if (nDeleteCount != 1) return 0;

    // Construir ruta de cuenta para backup
    char szAccSub[4]    = {0};
    char szAccPath[256] = {0};
    char szDstPath[256] = {0};
    GetSubDir(pszAccountName, szAccSub, sizeof(szAccSub));
    sprintf(szAccPath, "./account/%s/%s.TAD", szAccSub, pszAccountName);

    if (nMode == 2) return nDeleteCount;

    if (nMode == 1)
        sprintf(szDstPath, "./Delete_BackupRen/%s%02d%02d%02d_%02d%02d%02d.TAD",
                pszCharName, yy, st.wMonth, st.wDay,
                st.wHour, st.wMinute, st.wSecond);
    else if (nMode == 3)
        sprintf(szDstPath, "./Delete_BackupGM/%s%02d%02d%02d_%02d%02d%02d.TAD",
                pszCharName, yy, st.wMonth, st.wDay,
                st.wHour, st.wMinute, st.wSecond);
    else
        sprintf(szDstPath, "./Delete_Backup/%s%02d%02d%02d_%02d%02d%02d.TAD",
                pszCharName, yy, st.wMonth, st.wDay,
                st.wHour, st.wMinute, st.wSecond);

    CopyFileA(szAccPath, szDstPath, FALSE);

    return nDeleteCount;
}

// ============================================================
// DBReadChar  (VA 0x00435CA0)
// Lee header del .TCD: pszOutAccount recibe el nombre de cuenta
// Primero limpia byte[0] del buffer de output
// Luego abre O_RDONLY, lee 0x34 bytes
// Retorna: nada (void en el binario, pero la funcion termina en RET 8)
// ============================================================
void CFileDB::DBReadChar(char* pszOutAccount, const char* pszCharName)
{
    // Limpiar primer byte del output (exacto del binario: mov byte[eax],0)
    if (pszOutAccount) pszOutAccount[0] = 0;

    char szSub[4]    = {0};
    char szPath[256] = {0};
    GetSubDir(pszCharName, szSub, sizeof(szSub));
    sprintf(szPath, "./char/%s/%s.TCD", szSub, pszCharName);

    // Flags exactos del binario: 0x4000 = O_RDONLY
    int fd = _open(szPath, _O_RDONLY | _O_BINARY);
    if (fd == -1)
    {
        _close(fd);
        return;
    }

    // Leer 0x34 bytes (nombre de cuenta)
    _read(fd, pszOutAccount, CHAR_HEADER_SIZE);
    _close(fd);
}

// ============================================================
// DBCharExists  (VA 0x00437700 = DBWriteChar_v2)
// Verifica si ./char/<sub>/<charname>.TCD existe
// O_RDWR | O_BINARY = 0x8000
// Si existe: retorna 1. Si no: retorna 0.
// ============================================================
int CFileDB::DBCharExists(const char* pszCharName)
{
    char szSub[4]    = {0};
    char szPath[256] = {0};
    GetSubDir(pszCharName, szSub, sizeof(szSub));
    sprintf(szPath, "./char/%s/%s.TCD", szSub, pszCharName);

    // Flags exactos del binario: 0x8000 = _O_RDWR | _O_BINARY
    int fd = _open(szPath, _O_RDWR | _O_BINARY);
    if (fd != -1)
    {
        _close(fd);
        return 1;  // existe
    }
    _close(fd);
    return 0;
}

// ============================================================
// ReadAdminTxt  (VA 0x00439CD0)
// Lee Admin.txt: formato por linea "%d %d %d %d %d"
//   campo -1 = indice del admin (0..49)
//   campos 0-3 = octetos de la IP
// Los puntos en la linea se reemplazan por espacios antes de parsear
// Almacena en pAdminIP[idx] (VA 0x48C410 en binario)
// ============================================================
void ReadAdminTxt()
{
    char szPath[256] = {0};
    sprintf(szPath, "Admin.txt");  // Sin ruta, relativo al directorio del exe

    FILE* fp = fopen(szPath, "rt");
    if (!fp) return;

    char szLine[256] = {0};
    while (fgets(szLine, sizeof(szLine), fp))
    {
        // Reemplazar puntos por espacios (exacto del binario)
        for (int i = 0; i < 255 && szLine[i]; i++)
            if (szLine[i] == '.') szLine[i] = ' ';

        int nIdx = -1;
        int b0=0, b1=0, b2=0, b3=0;
        int nParsed = sscanf(szLine, "%d %d %d %d %d", &nIdx, &b0, &b1, &b2, &b3);

        // Validar indice: 0..49 (0x32 en el binario)
        if (nIdx < 0 || nIdx >= 0x32) continue;

        // Construir IP como DWORD: (b0<<24)|(b1<<16)|(b2<<8)|b3
        unsigned int nIP = ((unsigned int)b0 << 24)
                         | ((unsigned int)b1 << 16)
                         | ((unsigned int)b2 << 8)
                         |  (unsigned int)b3;

        pAdminIP[nIdx] = nIP;
    }

    fclose(fp);
}
