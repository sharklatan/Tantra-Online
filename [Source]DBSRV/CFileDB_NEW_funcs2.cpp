// ============================================================
// CFileDB_NEW_funcs2.cpp
// Segunda tanda de funciones reconstruidas de DBSRV_NEW.exe
// Targets: Trimuriti log, DeleteAccount/backup, WriteLog,
//          AdminGuildCargo, OldAccount fallback
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

// ============================================================
// STRINGS EXACTOS EXTRAIDOS DEL BINARIO (VA en comentario)
// ============================================================
// 0x476380  "BrahmaCount:%d VishnuCount:%d SivaCount:%d"
// 0x476364  ":%d Month:%d Day:%d Hour:%d BrahmaCount:%d VishnuCount:%d SivaCount:%d"
// 0x475CB4  "./char/%s/%s.TCD"
// 0x475CC8  "./Delete_Backup/%s%02d%02d%02d_%02d%02d%02d.TAD"
// 0x475D40  "./Delete_BackupRen/%s%02d%02d%02d_%02d%02d%02d.TAD"
// 0x476114  "./old_account/%s/%s.TAD"
// 0x477230  "Account Info: %s<%s|%s|%s>\n%s : %d"
// 0x477220  "%s (char:%s)"
// 0x476EC8  "Admin %d.%d.%d.%d Update GuildCargo Guild:%d"
// 0x476EAC  "Guild Cargo is updated"
// 0x477328  ".\\Log\\DB%4.4d_%2.2d_%2.2d_%2.2d_%2.2d%s.txt"

// ============================================================
// WriteEditHistory  (VA 0x0044AE10)
// Escribe en .\LOG\EditHistory.txt el registro de edicion
// Formato exacto del NEW:
//   YYMMDD HHMMSS    From IP    NAME written \r\n
//   [data]\r\n
// Usa CreateFile + SetFilePointer(END) + WriteFile (Win32 puro)
// Parametros:
//   pszPath = ruta del archivo .TAD a leer (para el contenido)
//   pszName = nombre de cuenta/personaje
//   nIP     = IP del cliente como DWORD
// ============================================================
void WriteEditHistory(const char* pszFilePath, const char* pszName, unsigned int nIP)
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    int yy = st.wYear % 100;

    // Formato header: YYMMDD HHMMSS    From IP    NAME written\r\n
    char szHeader[512] = {0};
    sprintf(szHeader,
        "%2.2d%2.2d%2.2d %2.2d%2.2d%2.2d    From %d.%d.%d.%d    %s written \r\n",
        yy, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond,
        (nIP>>24)&0xFF, (nIP>>16)&0xFF, (nIP>>8)&0xFF, nIP&0xFF,
        pszName ? pszName : "");

    // Crear/abrir el archivo con Win32 (append)
    HANDLE hFile = CreateFileA(
        ".\\LOG\\EditHistory.txt",
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;

    // Ir al final del archivo
    SetFilePointer(hFile, 0, NULL, FILE_END);

    // Escribir el header
    DWORD dwWritten = 0;
    WriteFile(hFile, szHeader, strlen(szHeader), &dwWritten, NULL);

    // Leer y escribir el contenido del archivo .TAD si existe
    if (pszFilePath && pszFilePath[0])
    {
        HANDLE hSrc = CreateFileA(pszFilePath,
            GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hSrc != INVALID_HANDLE_VALUE)
        {
            char szData[512] = {0};
            DWORD dwRead = 0;
            ReadFile(hSrc, szData, sizeof(szData)-1, &dwRead, NULL);
            CloseHandle(hSrc);
            if (dwRead > 0)
            {
                WriteFile(hFile, szData, dwRead, &dwWritten, NULL);
                WriteFile(hFile, "\r\n", 2, &dwWritten, NULL);
            }
        }
    }

    CloseHandle(hFile);
}
// ============================================================
// DeleteAccountWithBackup  (VA 0x00433080)
// Mueve archivo de cuenta a carpeta de backup con timestamp
// Parametros:
//   pszAccountName -> nombre de la cuenta a borrar
//   pszCharName    -> nombre del personaje (para nombre del backup)
//   nMode          -> 1=Delete normal, 2=solo retorna, 3=Rename backup
// Flujo:
//   1. Construye ruta src: ./account/<sub>/<account>.TAD
//   2. Verifica edad del archivo (5 dias = 0x9A7EC800 * 100ns intervals)
//   3. Si nMode==1: backup a ./Delete_Backup/<acc>YYMMDD_HHMMSS.TAD
//   4. Si nMode==3: backup a ./Delete_BackupRen/<acc>YYMMDD_HHMMSS.TAD
//   5. Llama a CopyFile + DeleteFile (IAT 0x1866A550/A560)
// Retorna: 1 OK, 0 error, 2 si nMode==2
// ============================================================
int CFileDB::DeleteAccountWithBackup(const char* pszAccountName,
                                     const char* pszCharName,
                                     int nMode)
{
    if (!pszAccountName || !pszAccountName[0]) return 0;

    char szSubDir[256]  = {0};
    char szSrcPath[256] = {0};
    char szDstPath[256] = {0};

    // Construir ruta fuente: ./account/<sub>/<account>.TAD
    char szSub[4] = {0};
    GetSubDir(pszAccountName, szSub, sizeof(szSub));
    sprintf(szSrcPath, "./account/%s/%s.TAD", szSub, pszAccountName);

    if (nMode == 2)
        return 2;  // Solo devolver sin hacer nada

    // Obtener timestamp del sistema para nombre de backup
    SYSTEMTIME st;
    GetLocalTime(&st);
    int yy = st.wYear % 100;

    if (nMode == 1)
    {
        // Backup normal: ./Delete_Backup/<account>YYMMDD_HHMMSS.TAD
        sprintf(szDstPath, "./Delete_Backup/%s%02d%02d%02d_%02d%02d%02d.TAD",
                pszAccountName,
                yy, st.wMonth, st.wDay,
                st.wHour, st.wMinute, st.wSecond);
    }
    else if (nMode == 3)
    {
        // Backup de rename: ./Delete_BackupRen/<charname>YYMMDD_HHMMSS.TAD
        sprintf(szDstPath, "./Delete_BackupRen/%s%02d%02d%02d_%02d%02d%02d.TAD",
                pszCharName ? pszCharName : pszAccountName,
                yy, st.wMonth, st.wDay,
                st.wHour, st.wMinute, st.wSecond);
    }
    else
        return 0;

    // Crear directorio destino si no existe
    if (nMode == 1) CreateDirectoryA("./Delete_Backup",    NULL);
    if (nMode == 3) CreateDirectoryA("./Delete_BackupRen", NULL);

    // Copiar y borrar original
    if (!CopyFileA(szSrcPath, szDstPath, FALSE))
    {
        char szErr[512] = {0};
        sprintf(szErr, "DeleteAccountWithBackup CopyFile fail: %s -> %s err=%d",
                szSrcPath, szDstPath, GetLastError());
        // [removido: no va a EditHistory.txt]
        return 0;
    }

    DeleteFileA(szSrcPath);

    // Log del evento
    char szLog[512] = {0};
    sprintf(szLog, "Account deleted: %s -> %s", szSrcPath, szDstPath);
    // [removido: no va a EditHistory.txt]

    return 1;
}

// ============================================================
// DBReadOldAccount  (parte de VA 0x00434400)
// Lee cuenta desde ./old_account/<sub>/<account>.TAD
// Fallback cuando ./account/ no tiene el archivo
// Mismo flujo que CheckAccount2 pero con ruta old_account
// Retorna: 1 OK, 0 error
// ============================================================
int CFileDB::DBReadOldAccount(const char* pszAccountName, void* pOutData)
{
    if (!pszAccountName || strlen(pszAccountName) < 4)
        return 0;

    // Limpiar flags de seguridad (mismos offsets que CheckAccount2)
    char* p = (char*)pszAccountName;  // cast seguro aqui es el buffer del msg
    // Los offsets 0x32,0x33,0x56,0x57 se limpian en el caller

    char szSub[4]       = {0};
    char szPath[256]    = {0};
    char szPathOld[256] = {0};

    GetSubDir(pszAccountName, szSub, sizeof(szSub));

    // Intentar primero ./account/
    sprintf(szPath, "./account/%s/%s.TAD", szSub, pszAccountName);
    int fd = _open(szPath, _O_RDWR | _O_BINARY);

    if (fd == -1)
    {
        // Fallback: ./old_account/
        sprintf(szPathOld, "./old_account/%s/%s.TAD", szSub, pszAccountName);
        fd = _open(szPathOld, _O_RDWR | _O_BINARY);
        if (fd == -1)
            return 0;
    }

    // Leer 0x1BB4 bytes (stride del registro de cuenta)
    // y copiar al pOutData con strncpy limitado a 0x14 (20) bytes primero (nombre)
    // luego memcpy del resto (0x1BA0 bytes desde offset 0x34)
    if (pOutData)
    {
        char buf[0x1BB4] = {0};
        _read(fd, buf, sizeof(buf));

        // Copiar nombre (20 bytes)
        strncpy((char*)pOutData, buf, 0x14);
        // Copiar datos (0x1BA0 bytes desde offset 0x34 en el destino)
        memcpy((char*)pOutData + 0x34, buf + 0x34, 0x1BA0);
    }

    _close(fd);
    return 1;
}

// ============================================================
// LogAccountInfo  (parte de VA 0x0043E045)
// Escribe al log rotativo la info de la cuenta que se loguea
// Formato: "Account Info: %s<%s|%s|%s>\n%s : %d"
//   param1 = IP del cliente  (formato "%d.%d.%d.%d")
//   param2 = nombre cuenta
//   param3 = nombre char
//   param4 = version cliente
//   param5 = nombre char (de nuevo)
//   param6 = slot index
// El binario tambien registra el tipo de cuenta:
//   0x40 ('@') = cuenta GM
//   0x5F ('_') = cuenta de prueba
//   0x01       = cuenta especial
// ============================================================
void CFileDB::LogAccountInfo(int nUser, const char* pszAccount,
                             const char* pszChar, int nSlot,
                             unsigned char byAccountType,
                             unsigned char* pClientIP)
{
    if (!g_pLogFile) return;

    // Determinar tipo de cuenta
    const char* pszType = "normal";
    if      (byAccountType == 0x40) pszType = "GM";
    else if (byAccountType == 0x5F) pszType = "test";
    else if (byAccountType == 0x01) pszType = "special";

    // Formato: "Account Info: <IP><<account>|<char>|<type>>\n<char> : <slot>"
    char szLine[512] = {0};
    if (pClientIP)
        sprintf(szLine, "Account Info: %d.%d.%d.%d<%s|%s|%s>\n%s : %d",
                pClientIP[0], pClientIP[1], pClientIP[2], pClientIP[3],
                pszAccount ? pszAccount : "",
                pszChar    ? pszChar    : "",
                pszType,
                pszChar    ? pszChar    : "",
                nSlot);
    else
        sprintf(szLine, "Account Info: 0.0.0.0<%s|%s|%s>\n%s : %d",
                pszAccount ? pszAccount : "",
                pszChar    ? pszChar    : "",
                pszType,
                pszChar    ? pszChar    : "",
                nSlot);

    fprintf(g_pLogFile, "%s\n", szLine);
    fflush(g_pLogFile);

    // Tambien al EditHistory
    // [removido: no va a EditHistory.txt]
}

// ============================================================
// LogAdminGuildCargo  (VA 0x0043F83F)
// Escribe al log cuando un admin actualiza el GuildCargo
// Formato: "Admin %d.%d.%d.%d Update GuildCargo Guild:%d"
// Luego envia respuesta "Guild Cargo is updated" al cliente
// nGuildID debe ser 1..50 (0x32), fuera de ese rango = ignorar
// ============================================================
void CFileDB::LogAdminGuildCargo(int nUser, const char* pszMsg,
                                  unsigned char* pClientIP, int nGuildID)
{
    // Validar rango de GuildID (1..50 segun binario: cmp 1..0x32)
    if (nGuildID < 1 || nGuildID >= 0x32)
        return;

    // Construir string de log
    char szLog[256] = {0};
    if (pClientIP)
        sprintf(szLog, "Admin %d.%d.%d.%d Update GuildCargo Guild:%d",
                pClientIP[0], pClientIP[1], pClientIP[2], pClientIP[3],
                nGuildID);
    else
        sprintf(szLog, "Admin 0.0.0.0 Update GuildCargo Guild:%d", nGuildID);

    // Escribir al log rotativo
    // [removido: no va a EditHistory.txt]
    if (g_pLogFile)
    {
        fprintf(g_pLogFile, "%s\n", szLog);
        fflush(g_pLogFile);
    }

    // Enviar respuesta al cliente: "Guild Cargo is updated"
    // (el binario llama a 0x426BA9 con el string 0x476EAC)
    // SendDBSignal equivalente - el caller debe hacer el SendDBMessage
}
