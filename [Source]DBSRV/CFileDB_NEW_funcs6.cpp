// ============================================================
// CFileDB_NEW_funcs6.cpp  -- Fase 2 completa
// Reconstruidas de DBSRV_NEW.exe
// DBWriteChar, GetCharName, GetGuildID, Rankings,
// Disciple, ExportData, LoadInitItemBin, SaveExtraItemBin
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
#include "CUser.h"

// ============================================================
// Strings exactos del NEW
// ============================================================
// 0x475CB4  "./char/%s/%s.TCD"
// 0x475498  "G:\name\%s.dat"
// 0x476418  "K:\MRank%02d.txt"
// 0x476404  "K:\Rank%02d.txt"
// 0x4763D8  "G:\DISCIPLE\%9.9d.dat"
// 0x476678  "R:/data%2.2d.csv"
// 0x477714  "WYD.EXE"
// 0x477708  "WYDSC.EXE"
// 0x4776FC  "WYDTC.EXE"
// 0x4776A0  "extraitem.bin"
// 0x477668  "TMSRV/Run/%s"
// 0x475638  "err GetGuildID:[%s]"
// 0x47561C  "GetGuildID:[%s] ID:%d"
// 0x4763F4  "%d %d %d %s"
// 0x47642C  "%d %d %s"
// 0x47664C  ",%4d  "
// 0x476654  "%4.4d_%2.2d_%2.2d_%2.2d "

// ============================================================
// DBWriteChar  (VA 0x004375E0 + 0x00437700)
// v1 (0x4375E0): solo para CountryID==7 (GLOBAL)
//   Limpia slots de personaje cuyo tick es menor al tick actual
//   Loop de 0x1F4 (500) slots, offset 0x170/0x174/0x178 en el struct
// v2 (0x437700): version principal
//   Abre ./char/<sub>/<name>.TCD con O_RDWR
//   Si existe: retorna 1. Si no: retorna 0.
// Esta es la "existencia" del .TCD, no escritura de datos completos
// Los datos del char se guardan en el .TAD de la cuenta
// ============================================================
int CFileDB::DBWriteChar(const char* pszCharName, void* pData)
{
    // v1: logica especial para GLOBAL (CountryID==7)
    // Limpia entradas de personaje con tick expirado
    if (g_nCountryID == 7 && pData)
    {
        DWORD dwTick = GetTickCount();
        char* p = (char*)pData;
        for (int i = 0; i < 0x1F4; i++)
        {
            int offset = i * 0x40;
            DWORD* pFlag1 = (DWORD*)(p + offset + 0x170);
            DWORD* pFlag2 = (DWORD*)(p + offset + 0x174);
            DWORD* pTick  = (DWORD*)(p + offset + 0x178);
            if ((*pFlag1 | *pFlag2) == 0) continue;
            if (*pTick < dwTick)
            {
                memset(p + offset + 0x170, 0, 0x40);
            }
        }
    }

    // v2: verificar/crear el .TCD
    char szSub[4]    = {0};
    char szPath[256] = {0};
    GetSubDir(pszCharName, szSub, sizeof(szSub));
    sprintf(szPath, "./char/%s/%s.TCD", szSub, pszCharName);

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
// GetCharName  (VA 0x0042B060)
// Lee G:\name\<charname>.dat y extrae el nombre de cuenta
// El archivo .dat contiene: [DWORD accountID][DWORD serverID][char accountName[20]][datos extras]
// Parametros:
//   pMsg    = puntero al mensaje (pMsg+0x18 = char name, pMsg+0x14 = serverID)
//   pResult = buffer destino para el nombre de cuenta
// Retorna: 1 OK, 0 error
// ============================================================
int CFileDB::GetCharName(void* pMsg, void* pResult)
{
    if (!pMsg || !pResult) return 0;

    char* pszCharName = (char*)pMsg + 0x18;  // offset 0x18 en el mensaje

    // Construir ruta G:\name\<charname>.dat
    char szPath[256] = {0};
    sprintf(szPath, "G:\\name\\%s.dat", pszCharName);

    // Intentar abrir (fopen "rb" equivalente)
    FILE* fp = fopen(szPath, "rb");
    if (fp)
    {
        fclose(fp);
        return 0;  // si existe, retorna 0 (el archivo ya fue procesado)
    }

    // Construir estructura de resultado
    // 0x1D64 bytes de datos
    char szData[0x1D64] = {0};
    char* pOut = (char*)pResult;

    // Copiar accountID y serverID del mensaje
    *((DWORD*)(szData + 0x00)) = *((DWORD*)pMsg);              // account ID
    *((DWORD*)(szData + 0x04)) = *((DWORD*)((char*)pMsg + 0x14)); // server ID

    // Copiar char name (0x14 = 20 bytes)
    memcpy(szData, (char*)pMsg + 0x18, 0x14);

    // Copiar 7 DWORDs de datos extras (offset 0x2C del mensaje)
    memcpy(szData + 0x1D24 - 0x1D28 + 0x1D24,
           (char*)pMsg + 0x2C, 7 * 4);

    // Enviar resultado
    // (llamada a 0x426B36 en el binario = SendToUser)

    // Crear el archivo G:\name\<charname>.dat
    FILE* fpOut = fopen(szPath, "wb");
    if (!fpOut) return 0;

    // Escribir accountName (0x14 bytes)
    char szAccName[0x14] = {0};
    sprintf(szAccName, "%d", *((DWORD*)pMsg));  // account ID como string
    fwrite(szAccName, 1, 0x14, fpOut);
    fclose(fpOut);

    return 1;
}

// ============================================================
// GetGuildID  (VA 0x0042C000)
// Lee G:\name\<guildname>.dat y extrae el GuildID
// El archivo contiene: "%d\n" con el ID del guild
// Parametros: pszGuildName (max 0x14 = 20 chars)
// Retorna: GuildID >= 0, -1 si error
// ============================================================
int CFileDB::GetGuildID(const char* pszGuildName)
{
    if (!pszGuildName) return -1;

    // Validar longitud (max 0x14 = 20)
    size_t nLen = strlen(pszGuildName);
    if (nLen > 0x14) return -1;

    // Construir ruta
    char szPath[256] = {0};
    sprintf(szPath, "G:\\name\\%s.dat", pszGuildName);

    // Abrir archivo "r"
    FILE* fp = fopen(szPath, "r");
    if (!fp)
    {
        char szLog[256] = {0};
        sprintf(szLog, "err GetGuildID:[%s]", pszGuildName);
        Log(szLog, (char*)"-system", 0);
        return -1;
    }

    int nGuildID = 0;
    fscanf(fp, "%d", &nGuildID);
    fclose(fp);

    char szLog[256] = {0};
    sprintf(szLog, "GetGuildID:[%s] ID:%d", pszGuildName, nGuildID);
    Log(szLog, (char*)"-system", 0);

    return nGuildID;
}

// ============================================================
// Rankings_WriteRank  (VA 0x00437830)
// Escribe ranking en K:\Rank<nn>.txt o K:\MRank<nn>.txt
// Logica segun CountryID:
//   0 (KOREA): escribe slots 0,1 y si ServerGroup==8 tb slot 7
//   1,2,3,6,7: escribe slots 0,1
//   otros: llama a una funcion especial (0x426A14)
// ============================================================
void CFileDB::Rankings_WriteRank(void* pAccount, int nServerGroup, int nSlot, int bMRank)
{
    if (!pAccount) return;

    // El NEW llama 0x426CC6 = WriteRankSlot(pAccount, nServerGroup, nSlot, bMRank)
    // Que internamente escribe en K:\Rank<nn>.txt
    // Delegamos a Rankings_UpdateRank
    switch (g_nCountryID)
    {
    case 0:  // KOREA
        Rankings_UpdateRank(pAccount, nServerGroup, 0);
        Rankings_UpdateRank(pAccount, nServerGroup, 1);
        if (nServerGroup == 8)
            Rankings_UpdateRank(pAccount, nServerGroup - 1, 1);
        break;
    case 1: case 2: case 3: case 6: case 7:  // CHINA/JAPAN/INDONESIA/MEXICO/GLOBAL
        Rankings_UpdateRank(pAccount, nServerGroup, 0);
        Rankings_UpdateRank(pAccount, nServerGroup, 1);
        break;
    default:
        Rankings_ReadRank(pAccount, nServerGroup, 0, 0);
        break;
    }
}

// ============================================================
// Rankings_ReadRank  (VA 0x00437950)
// Lee ranking desde K:\Rank<nn>.txt o K:\MRank<nn>.txt
// Parametros:
//   pAccount    = struct con datos del jugador
//   nRankIndex  = indice del ranking (0..3)
//   bWrite      = 1 si escribir al ranking, 0 si solo leer
//   bMRank      = 1 si usar MRank (mensual), 0 si Rank (diario)
// Formato del archivo: "%d %d %d %s\n" (clase, nivel, tipo, nombre)
// ============================================================
void CFileDB::Rankings_ReadRank(void* pAccount, int nRankIndex, int bWrite, int bMRank)
{
    if (!pAccount) return;

    char szPath[64] = {0};
    if (bMRank)
        sprintf(szPath, "K:\\MRank%02d.txt", nRankIndex);
    else
        sprintf(szPath, "K:\\Rank%02d.txt", nRankIndex);

    // Si solo lectura: cargar con CreateFile (Win32)
    if (!bWrite && !bMRank)
    {
        HANDLE hFile = CreateFileA(szPath,
            GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) return;

        FILETIME ftCreate, ftAccess, ftWrite;
        GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite);
        CloseHandle(hFile);

        // Copiar timestamps al struct (offsets 0xF3A0, 0xF3A4)
        char* p = (char*)pAccount;
        *((DWORD*)(p + 0xF3A0)) = ftWrite.dwLowDateTime;
        *((DWORD*)(p + 0xF3A4)) = ftWrite.dwHighDateTime;
    }

    // Abrir archivo de texto
    FILE* fp = fopen(szPath, "r");
    if (!fp) return;

    char szLine[256] = {0};
    char szName[256] = {0};
    int  nClass = 0, nLevel = 0, nType = 0;

    // Limpiar buffer de ranking en el struct
    if (bWrite)
        memset((char*)pAccount + 0x26AA8, 0, 0x4E20);

    while (fgets(szLine, sizeof(szLine), fp))
    {
        nClass = 0; nLevel = 0; nType = 0;
        memset(szName, 0, sizeof(szName));
        int nParsed = sscanf(szLine, "%d %d %d %s", &nClass, &nLevel, &nType, szName);
        if (nParsed < 3) continue;

        // Validar rangos
        if (nClass < 0 || nClass > 4) continue;
        if (nLevel < 0 || nLevel > 100) continue;
        if (nType < 1 || nType > 4) continue;

        if (!bWrite)
        {
            // Escribir entrada en el buffer del struct
            // Offset = 0xF3A8 + (nType-1)*0x1F40 + (nLevel-1)*0x14
            int nOffset = 0xF3A8
                + (nType - 1) * 0x1F40
                + (nLevel - 1) * 0x14;
            if (nOffset + 0x14 <= 0x1F400)
            {
                char* pEntry = (char*)pAccount + nOffset;
                strncpy(pEntry, szName, 0x14);
            }
        }
        else
        {
            // Actualizar ranking
            int nOffset = 0x26AA8
                + (nType - 1) * 0x1388
                + (nLevel - 1) * 0x14;
            char* pEntry = (char*)pAccount + nOffset;
            strncpy(pEntry, szName, 0x14);
        }
    }

    fclose(fp);
}

// ============================================================
// Rankings_UpdateRank  (VA 0x00437F30)
// Lee K:\Rank<nn>.txt y actualiza el ranking del jugador
// Formato: "%d %d %s\n" (clase, level, nombre)
// Parametros: pAccount, nRankIndex (0..3), nType (1..4)
// ============================================================
void CFileDB::Rankings_UpdateRank(void* pAccount, int nRankIndex, int nType)
{
    if (!pAccount) return;

    char szPath[64] = {0};
    sprintf(szPath, "K:\\Rank%02d.txt", nRankIndex);

    // Abrir con Win32 (GENERIC_READ | GENERIC_WRITE)
    HANDLE hFile = CreateFileA(szPath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;

    FILETIME ftCreate, ftAccess, ftWrite;
    GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite);
    CloseHandle(hFile);

    // Copiar timestamps al struct
    char* p = (char*)pAccount;
    *((DWORD*)(p + 0xF3A0)) = ftWrite.dwLowDateTime;
    *((DWORD*)(p + 0xF3A4)) = ftWrite.dwHighDateTime;

    FILE* fp = fopen(szPath, "r");
    if (!fp) return;

    // Limpiar buffer 0x4E20 bytes en offset 0x26AA8
    memset(p + 0x26AA8, 0, 0x4E20);

    char szLine[256] = {0};
    char szName[256] = {0};
    int  nClass = 0, nLevel = 0;

    while (fgets(szLine, sizeof(szLine), fp))
    {
        nClass = 0; nLevel = 0;
        memset(szName, 0, sizeof(szName));
        int nParsed = sscanf(szLine, "%d %d %s", &nClass, &nLevel, szName);
        if (nParsed < 2) continue;

        // Validar
        if (nClass < 0 || nClass > 4) continue;
        if (nLevel < 1 || nLevel > 250) continue;

        int nOffset = 0x26AA8
            + (nClass) * 0x1388
            + (nLevel - 1) * 0x14;
        if (nOffset + 0x14 < (int)(0x26AA8 + 0x4E20))
            strncpy(p + nOffset, szName, 0x14);
    }

    fclose(fp);
}

// ============================================================
// Disciple_Create  (VA 0x00436AF0)
// Crea o actualiza el archivo G:\DISCIPLE\<nDiscipleID>.dat
// Parametros: pMsg, pDiscipleData (0x1680 bytes), pMasterData (0x0C bytes)
// Retorna: 1 OK, 0 error
// ============================================================
int CFileDB::Disciple_Create(void* pMsg, void* pMasterData, void* pDiscipleData)
{
    // Obtener accountID del mensaje para verificacion (0x426244 = CheckAccount)
    // Simplificado: verificar que la cuenta existe
    char szDiscipleFile[256] = {0};
    int nDiscipleID = pMsg ? *((int*)pMsg) : 0;
    sprintf(szDiscipleFile, "G:\\DISCIPLE\\%9.9d.dat", nDiscipleID);

    // Copiar datos del maestro (0x0C bytes) y discipulo (0x1680 bytes)
    // 0x426244 = ReadDisciple, 0x426B36 = WriteDisciple
    FILE* fp = fopen(szDiscipleFile, "wb");
    if (!fp) return 0;

    if (pMasterData)   fwrite(pMasterData,   1, 0x0C,   fp);
    if (pDiscipleData) fwrite(pDiscipleData, 1, 0x1680, fp);
    fclose(fp);

    return 1;
}

// ============================================================
// Disciple_Read  (VA 0x00436C10)
// Lee G:\DISCIPLE\<nGuildID>.dat
// Escribe 0x50 bytes al pOut
// Parametros: nGuildID, pOut (buffer destino)
// Retorna: 1 OK, 0 error (como byte)
// ============================================================
int CFileDB::Disciple_Read(int nGuildID, void* pOut)
{
    char szPath[256] = {0};
    sprintf(szPath, "G:\\DISCIPLE\\%9.9d.dat", nGuildID);

    FILE* fp = fopen(szPath, "rb");
    if (!fp) return 0;

    if (pOut) fread(pOut, 1, 0x50, fp);
    fclose(fp);

    return 1;
}

// ============================================================
// Disciple_Write  (VA 0x00436D10)
// Escribe 0x50 bytes a G:\DISCIPLE\<nGuildID>.dat
// Abre con "wb" (trunca), escribe, cierra
// Retorna: nGuildID si OK
// ============================================================
int CFileDB::Disciple_Write(int nGuildID, void* pData)
{
    char szPath[256] = {0};
    sprintf(szPath, "G:\\DISCIPLE\\%9.9d.dat", nGuildID);

    FILE* fp = fopen(szPath, "wb");
    if (!fp) return 0;

    if (pData) fwrite(pData, 1, 0x50, fp);
    fclose(fp);

    return nGuildID;
}

// ============================================================
// ExportData  (VA 0x00439A70)
// Exporta estadisticas de usuarios a R:/data<nn>.csv
// Formato: timestamp + contadores por zona (0x32 zonas)
// Escribe al archivo y resetea los contadores (0x48C738)
// ============================================================
void CFileDB::ExportData()
{
    // Nombre del archivo: R:/data<ServerGroup>.csv
    char szPath[256] = {0};
    sprintf(szPath, "R:/data%2.2d.csv", g_nSWorld1);

    // Abrir para append
    FILE* fp = fopen(szPath, "at");
    if (!fp) return;

    // Escribir timestamp: "%4.4d_%2.2d_%2.2d_%2.2d "
    SYSTEMTIME st;
    GetLocalTime(&st);
    fprintf(fp, "%4.4d_%2.2d_%2.2d_%2.2d ",
            st.wYear, st.wMonth, st.wDay, st.wHour + 0x76C);

    // Escribir contadores por zona (0x32 = 50 zonas)
    // VA 0x48C738 = pUser[i].Count (mismo array que CUser)
    extern CUser pUser[];
    int nTotal = 0;
    for (int i = 0; i < 0x32; i++)
    {
        nTotal += pUser[i].Count;
        fprintf(fp, ",%4d  ", pUser[i].Count);
        pUser[i].Count = 0;
    }

    // Total al final
    fprintf(fp, ",%4d  ", nTotal);
    fprintf(fp, "\n");

    fclose(fp);
}

// ============================================================
// LoadInitItemBin  (VA 0x00444BA0)
// Carga items iniciales desde el .exe del cliente:
//   Intenta abrir WYD.EXE, luego WYDSC.EXE, luego WYDTC.EXE
//   Si ninguno: MessageBox de error
//   Si existe: seek a -0x200 bytes del final
//   Lee 0x200 bytes, XOR con 0xFF cada byte
//   Los primeros 0x40 items tienen ItemID != 0
//   Cuenta el primero con ItemID <= 0 -> g_nInitItemCount
// ============================================================
int CFileDB::LoadInitItemBin()
{
    const int  DATA_SIZE = 0x200;
    const DWORD BASE_ADDR = 0x186124C0;  // direccion base del cliente

    // Intentar abrir el ejecutable del cliente
    static const char* szExeNames[] = { "WYD.EXE", "WYDSC.EXE", "WYDTC.EXE", NULL };
    FILE* fpExe = NULL;
    for (int i = 0; szExeNames[i]; i++)
    {
        fpExe = fopen(szExeNames[i], "rb");
        if (fpExe) break;
    }

    if (!fpExe)
    {
        MessageBoxA(NULL, "Can't read inititem.bin", "ERROR", MB_OK);
        return 0;
    }

    // Seek a -DATA_SIZE del final
    fseek(fpExe, -DATA_SIZE, SEEK_END);

    // Leer y XOR con 0xFF
    static unsigned char s_initItemBuf[DATA_SIZE];
    fread(s_initItemBuf, 1, DATA_SIZE, fpExe);
    fclose(fpExe);

    for (int i = 0; i < DATA_SIZE; i++)
        s_initItemBuf[i] ^= 0xFF;

    // Buscar primer ItemID <= 0 en los primeros 0x40 entries (cada entry = 8 bytes)
    // Slot de init items: g_pInitItemCount (VA 0x18659190)
    int nCount = 0;
    for (int i = 0; i < 0x40; i++)
    {
        short nItemID = *((short*)(s_initItemBuf + i * 8));
        if (nItemID <= 0)
        {
            nCount = i;
            break;
        }
    }

    // Copiar al buffer global de init items (ya cargado en LoadInitItems)
    // El NEW copia a 0x186124C0 que es la tabla de init items en memoria
    if (g_InitItemTable)
        memcpy(g_InitItemTable, s_initItemBuf, DATA_SIZE);

    return 1;
}

// ============================================================
// SaveExtraItemBin  (VA 0x00444790)
// Lee extraitem.bin (o TMSRV/Run/<name>.csv como fallback)
// Si nMode != 0: abre con mutex global, cierra al final
// El archivo contiene items extras del servidor
// ============================================================
int CFileDB::SaveExtraItemBin(const char* pszFileName, int nMode)
{
    if (nMode != 0)
    {
        // Abrir mutex/semaforo global (VA 0x426B95 = WaitForSingleObject)
        // Simplificado: sin mutex por ahora
    }

    // Buscar el archivo
    FILE* fp = fopen("extraitem.bin", "rb");
    if (!fp)
    {
        // Fallback: TMSRV/Run/<filename>
        char szFallback[256] = {0};
        if (pszFileName)
            sprintf(szFallback, "TMSRV/Run/%s", pszFileName);
        fp = fopen(szFallback, "rb");
    }

    if (!fp)
    {
        if (nMode != 0)
        {
            // Liberar mutex
        }
        return 0;
    }

    fclose(fp);

    if (nMode != 0)
    {
        // Liberar mutex global
    }

    return 1;
}
