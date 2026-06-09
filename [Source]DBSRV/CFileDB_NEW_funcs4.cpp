// ============================================================
// CFileDB_NEW_funcs4.cpp
// Implementaciones finales para paridad funcional con NEW:
// - CheckAccount (slot lookup completo)
// - CheckAccount2 (open + read completo)
// - DrawInformations extendida (country code)
// - AccountSaveFail log
// ============================================================

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include "Basedef.h"
#include "CFileDB.h"
#include "TNDebug.h"

// ============================================================
// Constantes de estructura de archivos .TAD
// Extraidas del binario NEW (VA 0x0042A316 / 0x426E56)
// ============================================================
#define TAD_SLOT_STRIDE     0x1D18   // bytes por slot en el archivo
#define TAD_MAX_SLOTS       0xC350   // maximo 50000 slots
#define TAD_NAME_OFFSET     0x00     // offset del nombre en el slot
#define TAD_NAME_LEN        0x34     // longitud del nombre (52 bytes)

// ============================================================
// HELPER: BuscarSlotEnArchivo
// Busca un nombre de cuenta en un archivo .TAD abierto
// Recorre slots de TAD_SLOT_STRIDE bytes buscando coincidencia
// Retorna: offset del slot encontrado, -1 si no existe
// Equivale a la funcion 0x427171 del binario NEW
// ============================================================
static long BuscarSlotEnArchivo(int fd, const char* pszName, int bExact)
{
    long nSlot = -1;
    char szSlotName[TAD_NAME_LEN + 4] = {0};

    long nFileLen = _filelength(fd);
    if (nFileLen <= 0) return -1;

    long nSlots = nFileLen / TAD_SLOT_STRIDE;
    if (nSlots > TAD_MAX_SLOTS) nSlots = TAD_MAX_SLOTS;

    for (long i = 0; i < nSlots; i++)
    {
        long nOffset = i * TAD_SLOT_STRIDE;
        if (_lseek(fd, nOffset + TAD_NAME_OFFSET, SEEK_SET) < 0) break;

        memset(szSlotName, 0, sizeof(szSlotName));
        if (_read(fd, szSlotName, TAD_NAME_LEN) != TAD_NAME_LEN) break;

        if (bExact)
        {
            if (_stricmp(szSlotName, pszName) == 0)
            { nSlot = nOffset; break; }
        }
        else
        {
            // Slot vacio (primer byte = 0)
            if (szSlotName[0] == '\0')
            { nSlot = nOffset; break; }
        }
    }

    return nSlot;
}

// ============================================================
// CheckAccount (version con slot lookup completo)
// Lee ./Check_account/<sub>/<nombre>.TAD
// Busca el slot del personaje y copia datos al caller
// Retorna: 1 OK, 0 error
// ============================================================
int CFileDB::CheckAccount(const char* pszAccountName, void* pOutData, int nFlag)
{
    if (IsReservedDeviceName(pszAccountName)) return 0;

    char szSub[4]       = {0};
    char szPath[256]    = {0};
    GetSubDir(pszAccountName, szSub, sizeof(szSub));
    sprintf(szPath, "./Check_account/%s/%s.TAD", szSub, pszAccountName);

    // Construir nombre normalizado para busqueda (tolower)
    char szNameLower[TAD_NAME_LEN + 4] = {0};
    strncpy(szNameLower, pszAccountName, TAD_NAME_LEN);
    CharLowerA(szNameLower);

    int fd = _open(szPath, _O_RDONLY | _O_BINARY);
    if (fd == -1)
    {
        // Log: "err UpdateOneTimeAccount write fail"
        // (mismo mensaje que el NEW en 0x4754C4)
        char szLog[128] = {0};
        sprintf(szLog, "err UpdateOneTimeAccount write fail [%s]", pszAccountName);
        // [removido: no va a EditHistory.txt]
        return 0;
    }

    // Buscar slot exacto por nombre
    long nSlotOffset = BuscarSlotEnArchivo(fd, szNameLower, 1);
    if (nSlotOffset < 0)
    {
        _close(fd);
        return 0;
    }

    // Copiar datos del slot al buffer del caller
    if (pOutData)
    {
        _lseek(fd, nSlotOffset, SEEK_SET);
        _read(fd, pOutData, TAD_SLOT_STRIDE);
    }

    // Si nFlag != 0: guardar el slot ID en el puntero de output
    if (nFlag && pOutData)
        *(int*)((char*)pOutData + 0x1B4) = (int)(nSlotOffset / TAD_SLOT_STRIDE);

    _close(fd);
    return 1;
}

// ============================================================
// CheckAccount2 (version con open + read completo)
// Lee ./Check_account/<sub>/<nombre>.TAD con flags 0x8000 (O_RDWR)
// Busca slot libre si pOutSlotID != NULL
// Limpia campos de seguridad del mensaje antes de procesar
// Retorna: 1 OK, 0 error
// ============================================================
int CFileDB::CheckAccount2(void* pMsg, int* pOutSlotID)
{
    char* pszName = (char*)pMsg;

    // Limpiar flags de seguridad (exacto del binario NEW)
    pszName[0x33] = 0;
    pszName[0x32] = 0;
    pszName[0x57] = 0;
    pszName[0x56] = 0;

    // Validar longitud minima 4
    if (strlen(pszName) < 4) return 0;

    char szSub[4]    = {0};
    char szPath[256] = {0};
    GetSubDir(pszName, szSub, sizeof(szSub));
    sprintf(szPath, "./Check_account/%s/%s.TAD", szSub, pszName);

    // Limpiar campos del mensaje (exacto del binario NEW)
    memset(pszName + 0x34,  0, 0x100);
    pszName[0x134] = 0; pszName[0x135] = 0;
    pszName[0x136] = 0; pszName[0x137] = 0;
    pszName[0x138] = 0; pszName[0x139] = 0;

    // Buscar slot en tabla en memoria si pOutSlotID != NULL
    // (llamada a 0x426622 en el binario)
    if (pOutSlotID)
    {
        // Buscar en pAccountList por nombre
        int nFound = -1;
        for (int i = 0; i < MAX_DBACCOUNT; i++)
        {
            if (pAccountList[i].Login == 0) continue;
            if (_stricmp(pAccountList[i].File.AccountName, pszName) == 0)
            { nFound = i; break; }
        }
        if (nFound != -1)
            *pOutSlotID = nFound;
        // Si no encuentra, pOutSlotID queda sin modificar
    }

    // Abrir archivo: O_RDWR | O_BINARY (flags 0x8000 en el binario)
    int fd = _open(szPath, _O_RDWR | _O_BINARY);
    if (fd == -1)
    {
        // Mapeo de errores exacto del binario NEW (VA 0x434BD3)
        int err = errno;
        if (err == EINVAL)
        {
            WriteLog("err DBReadOneTimeAccount EINVAL", ".\\LOG\\debug.txt");
            return 0;
        }
        else if (err == EMFILE)
        {
            WriteLog("err DBReadOneTimeAccount EEMFILE", ".\\LOG\\debug.txt");
            return 0;
        }
        else if (err == ENOENT)
        {
            return 0;  // silencioso - cuenta no existe
        }
        else
        {
            WriteLog("err DBReadOneTimeAccount UNKNOWN", ".\\LOG\\debug.txt");
            return 0;
        }
    }

    // Obtener tamanio del archivo
    long nFileLen = _filelength(fd);
    if (nFileLen <= 0) { _close(fd); return 0; }

    // Buscar slot por nombre
    long nSlotOffset = BuscarSlotEnArchivo(fd, pszName, 1);
    if (nSlotOffset < 0) { _close(fd); return 0; }

    // Leer el slot completo al buffer del mensaje
    // (stride 0x1D18, copiado a pMsg desde el inicio)
    _lseek(fd, nSlotOffset, SEEK_SET);
    _read(fd, pMsg, TAD_SLOT_STRIDE);

    _close(fd);
    return 1;
}

// ============================================================
// LogAccountSaveFail
// Registra cuando falla el guardado de cuenta al salir un personaje
// String del NEW: "[%s] Account saving is fail by exit charname."
// Llamada desde el handler de desconexion del personaje
// ============================================================
void CFileDB::LogAccountSaveFail(const char* pszAccountName)
{
    char szLog[512] = {0};
    sprintf(szLog, "[%s] Account saving is fail by exit charname.", 
            pszAccountName ? pszAccountName : "unknown");
    // [removido: no va a EditHistory.txt]
    if (g_pLogFile) { fprintf(g_pLogFile, "%s\n", szLog); fflush(g_pLogFile); }
}
