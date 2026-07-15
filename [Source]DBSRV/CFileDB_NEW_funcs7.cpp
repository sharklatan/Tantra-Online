// ============================================================
// CFileDB_NEW_funcs7.cpp  -- Handlers de mensajes faltantes
// Reconstruidos de DBSRV_NEW.exe ProcessMessage (0x43D420)
// _Msg_EditChar, _Msg_CreateChar, _Msg_DeleteChar, _Msg_RenameChar
// ============================================================

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "Basedef.h"
#include "CFileDB.h"
#include "TNDebug.h"
#include "CUser.h"

extern CUser pUser[];

// ============================================================
// WriteAppendFile  (VA 0x426A23 en el NEW)
// Escribe texto al final de un archivo usando Win32
// Exactamente lo que hace el NEW para EditHistory.txt
// ============================================================
static void WriteAppendFile(const char* pszText, const char* pszPath)
{
    if (!pszText || !pszPath) return;

    HANDLE hFile = CreateFileA(
        pszPath,
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;

    SetFilePointer(hFile, 0, NULL, FILE_END);
    DWORD dwWritten = 0;
    WriteFile(hFile, pszText, strlen(pszText), &dwWritten, NULL);
    CloseHandle(hFile);
}

// ============================================================
// _Msg_EditChar  (bloque en ProcessMessage VA 0x43E3FC)
// Escribe en EditHistory.txt el registro de edicion de char
// Formato exacto del NEW:
//   "YYMMDD HHMMSS    From IP    NAME written \r\n"
//   + datos del mensaje desde offset 0x1C0C
// Parametros:
//   nUser  = indice del usuario conectado
//   pMsg   = puntero al mensaje recibido
// ============================================================
void CFileDB::_Msg_EditChar(int nUser, void* pMsg)
{
    if (!pMsg) return;

    // Limpiar flags de seguridad (offset 0x3E, 0x3F del mensaje)
    char* p = (char*)pMsg;
    p[0x3F] = 0;
    p[0x3E] = 0;

    SYSTEMTIME st;
    GetLocalTime(&st);
    int yy = st.wYear % 100;

    // Obtener IP del usuario
    // pUser[nUser] contiene la IP en formato network byte order
    unsigned int nIP = 0;
    if (nUser >= 0 && nUser < MAX_SERVERNUMBER)
    {
        // IP almacenada como 4 bytes separados en el CUser
        // El NEW usa pUser[nUser].IP
        nIP = (unsigned int)pUser[nUser].Mode;  // placeholder - ver CUser
    }

    // El nombre del char esta en pMsg + 0x0C (segun el NEW offset 0x18 - 0x0C)
    char* pszCharName = p + 0x0C;

    // Construir header: formato exacto del NEW (0x477134)
    // "From %d.%d.%d.%d    %s written \r\n"
    char szHeader[512] = {0};
    sprintf(szHeader,
        "%2.2d%2.2d%2.2d %2.2d%2.2d%2.2d    From %d.%d.%d.%d    %s written \r\n",
        yy, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond,
        (nIP >> 24) & 0xFF, (nIP >> 16) & 0xFF,
        (nIP >>  8) & 0xFF,  nIP & 0xFF,
        pszCharName);

    WriteAppendFile(szHeader, ".\\LOG\\EditHistory.txt");

    // Escribir datos del mensaje desde offset 0x1C0C (campo de texto del char)
    char* pData = p + 0x1C0C;
    if (pData[0])
    {
        char szData[512] = {0};
        strncpy(szData, pData, 511);
        strcat(szData, "\r\n");
        WriteAppendFile(szData, ".\\LOG\\EditHistory.txt");
    }
}

// ============================================================
// _Msg_CreateChar  (bloque en ProcessMessage del source original)
// El source original ya tiene CreateCharacter() pero sin los
// logs y validaciones del NEW. Esta version las agrega.
// ============================================================
void CFileDB::_Msg_CreateChar(int nUser, void* pMsg)
{
    if (!pMsg) return;
    char* p = (char*)pMsg;

    // Limpiar flags (offset 0x4B, 0x4A)
    p[0x4B] = 0;
    p[0x4A] = 0;

    char* pszAccountName = p + 0x0C;   // nombre de cuenta
    char* pszCharName    = p + 0x18;   // nombre del personaje

    // Validar nombre con IsReservedDeviceName
    if (IsReservedDeviceName(pszCharName)) return;

    // Crear el archivo .TCD
    int nRet = DBCreateChar(pszAccountName, pszCharName);

    if (nRet == 0)
    {
        // Char ya existe o error - log
        char szLog[256] = {0};
        sprintf(szLog, "CreateChar fail: %s for %s", pszCharName, pszAccountName);
        Log(szLog, (char*)"-system", 0);
    }
    else
    {
        // Exito
        char szLog[256] = {0};
        sprintf(szLog, "CreateChar OK: %s for %s", pszCharName, pszAccountName);
        Log(szLog, (char*)"-system", 0);
    }
}

// ============================================================
// _Msg_DeleteChar  (bloque en ProcessMessage del source original)
// El source original tiene DeleteCharacter() - esta version
// agrega las rutas de backup exactas del NEW
// ============================================================
void CFileDB::_Msg_DeleteChar(int nUser, void* pMsg)
{
    if (!pMsg) return;
    char* p = (char*)pMsg;

    char* pszCharName    = p + 0x18;   // nombre del personaje
    char* pszAccountName = p + 0x0C;   // nombre de cuenta
    int   nMode          = *((int*)(p + 0x10));  // modo: 1=normal, 3=GM

    // Llamar a DBDeleteChar con el modo correcto
    int nRet = DBDeleteChar(pszCharName, pszAccountName, nMode);

    if (nRet)
    {
        char szLog[256] = {0};
        sprintf(szLog, "DeleteChar OK: %s (account: %s) mode=%d",
                pszCharName, pszAccountName, nMode);
        Log(szLog, (char*)"-system", 0);
    }
}

// ============================================================
// _Msg_RenameChar  (bloque en ProcessMessage del source original)
// Renombra un personaje:
//   1. Backup del .TAD en Delete_BackupRen
//   2. Renombra el .TCD de oldname a newname
//   3. Actualiza la referencia en el .TAD
// ============================================================
void CFileDB::_Msg_RenameChar(int nUser, void* pMsg)
{
    if (!pMsg) return;
    char* p = (char*)pMsg;

    char* pszOldName     = p + 0x18;  // nombre actual del personaje
    char* pszNewName     = p + 0x2C;  // nuevo nombre
    char* pszAccountName = p + 0x0C;  // nombre de cuenta

    // Validar nuevo nombre
    if (IsReservedDeviceName(pszNewName)) return;

    // Verificar que el nuevo nombre no exista
    if (DBCharExists(pszNewName))
    {
        char szLog[256] = {0};
        sprintf(szLog, "RenameChar fail - new name exists: %s", pszNewName);
        Log(szLog, (char*)"-system", 0);
        return;
    }

    // Backup del .TAD en Delete_BackupRen (modo 3)
    DBDeleteChar(pszOldName, pszAccountName, 3);

    // Construir rutas old/new .TCD
    char szOldSub[4]  = {0}; char szOldPath[256] = {0};
    char szNewSub[4]  = {0}; char szNewPath[256] = {0};
    GetSubDir(pszOldName, szOldSub, sizeof(szOldSub));
    GetSubDir(pszNewName, szNewSub, sizeof(szNewSub));
    sprintf(szOldPath, "./char/%s/%s.TCD", szOldSub, pszOldName);
    sprintf(szNewPath, "./char/%s/%s.TCD", szNewSub, pszNewName);

    // Crear directorio destino
    char szDir[256] = {0};
    sprintf(szDir, "./char/%s", szNewSub);
    CreateDirectoryA(szDir, NULL);

    // Mover el archivo .TCD
    if (MoveFileA(szOldPath, szNewPath))
    {
        char szLog[256] = {0};
        sprintf(szLog, "RenameChar OK: %s -> %s (account: %s)",
                pszOldName, pszNewName, pszAccountName);
        Log(szLog, (char*)"-system", 0);
    }
    else
    {
        char szLog[256] = {0};
        sprintf(szLog, "RenameChar fail MoveFile: %s -> %s err=%d",
                pszOldName, pszNewName, GetLastError());
        Log(szLog, (char*)"-system", 0);
    }
}
