// ============================================================
// Funciones reconstruidas de DBSRV_NEW.exe
// Reconstruidas por analisis de disassembly
// Integrar en CFileDB.cpp
// ============================================================

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "Basedef.h"
#include "CFileDB.h"

// ---- Definicion de globals ----
short		g_InitItemTable[MAX_INIT_ITEMS][4]  = {0};
SkillEntry	g_SkillData[MAX_SKILL_DATA]         = {0};
FILE*		g_pLogFile   = NULL;
char		g_szLogPath[256] = {0};

// ============================================================
// TimeWriteLog
// Escribe texto en un archivo de log con timestamp
// Usada en Server.cpp para EditHistory.txt
// Reconstruida del binario NEW (VA 0x00440960)
// ============================================================
void TimeWriteLog(const char* pszText, const char* pszFilePath)
{
    if (!pszText || !pszFilePath) return;

    FILE* fp = fopen(pszFilePath, "at");
    if (!fp) return;

    SYSTEMTIME st;
    GetLocalTime(&st);

    // Formato de timestamp: [YYYY/MM/DD HH:MM:SS]
    fprintf(fp, "[%04d/%02d/%02d %02d:%02d:%02d] %s",
            st.wYear, st.wMonth, st.wDay,
            st.wHour, st.wMinute, st.wSecond,
            pszText);

    // Asegurar newline al final
    int len = (int)strlen(pszText);
    if (len > 0 && pszText[len-1] != '\n')
        fprintf(fp, "\n");

    fclose(fp);
}

// ============================================================
// CONSTANTES EXTRAIDAS DEL BINARIO
// ============================================================

// Clave de decodificacion GM (65 bytes, VA 0x476E40 en NEW)
// Algoritmo: decoded[i] = raw[i] - GMKey[63 - i]
static const unsigned char s_GMKey[65] = {
    164,161,164,164,164,167,164,169,164,177,164,178,164,181,164,183,
    164,184,164,186,164,187,164,188,164,189,164,190,164,191,164,193,
    164,195,164,197,164,199,164,203,164,204,164,208,164,209,164,211,
    164,191,164,196,164,211,164,199,164,204,176,161,179,170,180,217,
    0
};

// Limites de slots de personajes
#define MAX_CHAR_SLOTS      0xC350      // 50000 slots
#define CHAR_SLOT_STRIDE    0x1D18      // bytes por slot

// ============================================================
// HELPERS INTERNOS
// ============================================================

// Valida que el nombre no sea un dispositivo reservado de Windows
// COM0-COM9 y LPT0-LPT9 causan problemas al abrir archivos
static bool IsReservedDeviceName(const char* pszName)
{
    if (!pszName || !pszName[0]) return false;

    bool isCOM = (pszName[0]=='C' && pszName[1]=='O' && pszName[2]=='M');
    bool isLPT = (pszName[0]=='L' && pszName[1]=='P' && pszName[2]=='T');

    if ((isCOM || isLPT)
        && pszName[3] >= '0' && pszName[3] <= '9'
        && pszName[4] == '\0')
        return true;

    return false;
}

// Tabla de rangos Hangul para clasificacion de subcarpeta (extraida de VA 0x4840D8)
// Define los 18 rangos del espacio Hangul EUC-KR (offset desde 0xB0A1)
static const int s_HangulRanges[18] = {
    0x00AB, 0x0123, 0x01B0, 0x0230, 0x0286, 0x0305,
    0x0386, 0x0407, 0x0450, 0x04D5, 0x0549, 0x0619,
    0x06A0, 0x06EA, 0x0762, 0x07CD, 0x0837, 0x08A0
};

// Tabla de nombres de subcarpeta Hangul (18 consonantes, 2 bytes EUC-KR cada una)
// Extraida de VA 0x4840B0 — consonantes iniciales del alfabeto coreano
static const unsigned char s_HangulSubDirs[18][3] = {
    {0xA4,0xA1,0}, {0xA4,0xA2,0}, {0xA4,0xA4,0}, {0xA4,0xA7,0},
    {0xA4,0xA8,0}, {0xA4,0xA9,0}, {0xA4,0xB1,0}, {0xA4,0xB2,0},
    {0xA4,0xB3,0}, {0xA4,0xB5,0}, {0xA4,0xB6,0}, {0xA4,0xB7,0},
    {0xA4,0xB8,0}, {0xA4,0xB9,0}, {0xA4,0xBA,0}, {0xA4,0xBB,0},
    {0xA4,0xBC,0}, {0xA4,0xBD,0}
};

// GetSubDir - Extrae la subcarpeta del directorio para el nombre de cuenta
// Logica extraida y reconstruida de VA 0x443F50 del binario NEW:
//
//  1. Si primer char es A-Z o a-z -> subcarpeta = ese caracter (1 byte + null)
//  2. Si primer char es byte alto (>= 0x80) -> es Hangul EUC-KR (2 bytes)
//     Calcula offset = (byte0 - 0xB0) * 0x5E + (byte1 - 0xA1)
//     Busca en s_HangulRanges[] el rango correspondiente
//     Subcarpeta = s_HangulSubDirs[rango] (2 bytes Hangul + null)
//  3. Cualquier otro caso -> subcarpeta = "etc"
static void GetSubDir(const char* pszAccountName, char* pszSubDir, int nMaxLen)
{
    if (!pszAccountName || !pszAccountName[0])
    {
        strncpy(pszSubDir, "etc", nMaxLen);
        return;
    }

    unsigned char c0 = (unsigned char)pszAccountName[0];

    // Caso 1: ASCII letra (A-Z o a-z) -> subcarpeta = primer caracter
    if ((c0 >= 0x41 && c0 <= 0x5A) || (c0 >= 0x61 && c0 <= 0x7A))
    {
        pszSubDir[0] = (char)c0;
        pszSubDir[1] = '\0';
        return;
    }

    // Caso 2: Hangul EUC-KR (byte alto >= 0x80)
    if ((signed char)c0 < 0)  // byte con bit alto = caracter multibyte
    {
        unsigned char c1 = (unsigned char)pszAccountName[1];
        // Calcular offset en el espacio Hangul
        // Formula: (c0 - 0xB0) * 0x5E + (c1 - 0xA1)
        int offset = (int)(c0 - 0xB0) * 0x5E + (int)(c1 - 0xA1);

        // Validar rango valido (0 .. 0x92E)
        if (offset >= 0 && offset < 0x92E)
        {
            // Buscar en que rango cae (busqueda lineal hasta encontrar rango mayor)
            int nRange = 0;
            while (nRange < 18 && offset >= s_HangulRanges[nRange])
                nRange++;

            // Validar indice (0..17)
            if (nRange >= 0 && nRange <= 17)
            {
                pszSubDir[0] = (char)s_HangulSubDirs[nRange][0];
                pszSubDir[1] = (char)s_HangulSubDirs[nRange][1];
                pszSubDir[2] = '\0';
                return;
            }
        }
    }

    // Caso 3: fallback -> "etc" (VA 0x477610 en el binario)
    strncpy(pszSubDir, "etc", nMaxLen);
}

// ============================================================
// CreateAccount
// Crea cuenta en ./account/<subdir>/<nombre>.TAD
// Retorna: 1 OK, 0 error
// ============================================================
int CFileDB::CreateAccount(const char* pszAccountName, const char* pCharData,
                           int nLen, int nFlag)
{
    if (IsReservedDeviceName(pszAccountName))
        return 0;

    char szSubDir[256]   = {0};
    char szFilePath[256] = {0};
    GetSubDir(pszAccountName, szSubDir, sizeof(szSubDir));
    sprintf(szFilePath, "./account/%s/%s.TAD", szSubDir, pszAccountName);

    // Crear directorio si no existe
    char szDir[256] = {0};
    sprintf(szDir, "./account/%s", szSubDir);
    CreateDirectoryA(szDir, NULL);

    int fd = _open(szFilePath, _O_CREAT | _O_RDWR | _O_BINARY,
                   _S_IREAD | _S_IWRITE);
    if (fd == -1)
    {
        int err = errno;
        if      (err == EACCES) TNDebug("CreateAccount EACCES\n");
        else if (err == EEXIST) TNDebug("CreateAccount EEXIST\n");
        else if (err == EINVAL) TNDebug("CreateAccount EINVAL\n");
        else if (err == EMFILE) TNDebug("CreateAccount EMFILE\n");
        else if (err == ENOENT) TNDebug("CreateAccount ENOENT\n");
        else                    TNDebug("CreateAccount write fail\n");
        return 0;
    }

    if (pCharData && nLen > 0)
        _write(fd, pCharData, nLen);

    _close(fd);
    return 1;
}

// ============================================================
// CreateAccount2
// Crea cuenta en ./account2/<subdir>/<nombre>.TAD
// Ademas busca slot existente en tabla interna y lo sobreescribe
// Retorna: 1 OK, 0 error
// ============================================================
int CFileDB::CreateAccount2(const char* pszAccountName, const char* pCharData,
                            int nLen, int nFlag)
{
    if (IsReservedDeviceName(pszAccountName))
        return 0;

    char szSubDir[256]   = {0};
    char szFilePath[256] = {0};
    GetSubDir(pszAccountName, szSubDir, sizeof(szSubDir));
    sprintf(szFilePath, "./account2/%s/%s.TAD", szSubDir, pszAccountName);

    char szDir[256] = {0};
    sprintf(szDir, "./account2/%s", szSubDir);
    CreateDirectoryA(szDir, NULL);

    int fd = _open(szFilePath, _O_CREAT | _O_RDWR | _O_BINARY,
                   _S_IREAD | _S_IWRITE);
    if (fd == -1)
    {
        int err = errno;
        if      (err == EACCES) TNDebug("CreateAccount2 EACCES\n");
        else if (err == EEXIST) TNDebug("CreateAccount2 EEXIST\n");
        else if (err == EINVAL) TNDebug("CreateAccount2 EINVAL\n");
        else if (err == EMFILE) TNDebug("CreateAccount2 EMFILE\n");
        else if (err == ENOENT) TNDebug("CreateAccount2 ENOENT\n");
        else                    TNDebug("CreateAccount2 write fail\n");
        return 0;
    }

    if (pCharData && nLen > 0)
        _write(fd, pCharData, nLen);

    _close(fd);
    return 1;
}

// ============================================================
// CheckAccount / UpdateOneTimeAccount
// Lee ./Check_account/<subdir>/<nombre>.TAD
// Carga datos al slot del personaje
// Retorna: 1 OK, 0 error
// ============================================================
int CFileDB::CheckAccount(const char* pszAccountName, void* pOutData, int nFlag)
{
    if (IsReservedDeviceName(pszAccountName))
        return 0;

    char szSubDir[256]   = {0};
    char szFilePath[256] = {0};
    GetSubDir(pszAccountName, szSubDir, sizeof(szSubDir));
    sprintf(szFilePath, "./Check_account/%s/%s.TAD", szSubDir, pszAccountName);

    int fd = _open(szFilePath, _O_RDONLY | _O_BINARY);
    if (fd == -1)
    {
        TNDebug("err UpdateOneTimeAccount write fail\n");
        return 0;
    }

    // Leer y copiar datos al slot (stride 0x1D18, max 0xC350 slots)
    // TODO: implementar lectura de slot especifico

    _close(fd);
    return 1;
}

// ============================================================
// CheckAccount2 / DBReadOneTimeAccount
// Version mejorada con limpieza de flags y manejo de errores Winsock
// Retorna: 1 OK, 0 error
// ============================================================
int CFileDB::CheckAccount2(void* pMsg, int* pOutSlotID)
{
    char* pszName = (char*)pMsg;

    // Limpiar flags de seguridad en el mensaje
    pszName[0x33] = 0;
    pszName[0x32] = 0;
    pszName[0x57] = 0;
    pszName[0x56] = 0;

    // Nombre minimo 4 caracteres
    if (strlen(pszName) < 4)
        return 0;

    char szSubDir[256]   = {0};
    char szFilePath[256] = {0};
    GetSubDir(pszName, szSubDir, sizeof(szSubDir));
    sprintf(szFilePath, "./Check_account/%s/%s.TAD", szSubDir, pszName);

    // Limpiar campos del mensaje
    memset(pszName + 0x34,  0, 0x100);
    memset(pszName + 0x134, 0, 8);

    // Abrir en lectura/escritura
    int fd = _open(szFilePath, _O_RDWR | _O_BINARY);
    if (fd == -1)
    {
        DWORD dwErr = GetLastError();
        if (dwErr == 0x16)      TNDebug("err DBReadOneTimeAccount EINVAL\n");
        else if (dwErr == 0x18) TNDebug("err DBReadOneTimeAccount EEMFILE\n");
        else if (dwErr == 2)    { /* silencioso */ }
        else                    TNDebug("err DBReadOneTimeAccount UNKNOWN\n");
        return 0;
    }

    // Leer, decodificar (XOR/byte ops) y copiar a buffer del mensaje
    // stride: 0x25C bytes por registro
    // TODO: implementar decodificacion con clave del slot

    _close(fd);
    return 1;
}

// ============================================================
// GetGMPermission
// Lee ./GMPermission/<accountName>.bin
// Decodifica: decoded[i] = raw[i] - s_GMKey[63 - i]
// Parsea con formato "%s %d" -> nombre + nivel
// Retorna: nivel GM si nombre coincide, -1 si no existe o no coincide
// ============================================================
int CFileDB::GetGMPermission(const char* pszAccountName, const char* pszCheckName)
{
    char szFilePath[256] = {0};
    sprintf(szFilePath, "./GMPermission/%s.bin", pszAccountName);

    FILE* fp = fopen(szFilePath, "rb");
    if (!fp)
        return -1;

    // Leer 64 bytes
    unsigned char raw[64] = {0};
    fread(raw, 1, 0x40, fp);
    fclose(fp);

    // Decodificar: buf[i] = raw[i] - s_GMKey[63 - i]
    unsigned char decoded[64] = {0};
    for (int i = 0; i < 0x40; i++)
        decoded[i] = raw[i] - s_GMKey[0x3F - i];

    // Parsear: formato "%s %d"
    char szName[64] = {0};
    int  nLevel     = 0;
    sscanf((char*)decoded, "%s %d", szName, &nLevel);

    // Comparar nombre con el buscado (strcmp, case-sensitive)
    if (strcmp(szName, pszCheckName) != 0)
        return -1;

    return nLevel;
}

// ============================================================
// LoadInitItems
// Lee ./InitItem.csv
// Formato por linea: "%d %d %d %d"
// Almacena en tabla global g_InitItemTable, stride 8 bytes
// ============================================================
void CFileDB::LoadInitItems()
{
    FILE* fp = fopen("./InitItem.csv", "rb");
    if (!fp)
        fp = fopen("./InitItem.txt", "rb");
    if (!fp)
    {
        MessageBox(NULL, "InitItem file not found", "DBSRV", MB_OK);
        return;
    }

    // Saltar header
    char szLine[0x400] = {0};
    if (!fgets(szLine, sizeof(szLine), fp)) { fclose(fp); return; }

    // Puntero a tabla global definida en este mismo archivo
    // struct: { short f1, f2, f3, f4; } stride 8 bytes
    int nCount = 0;

    while (fgets(szLine, sizeof(szLine), fp))
    {
        // Reemplazar comas por espacios
        for (int i = 0; szLine[i] && i < 0x400; i++)
            if (szLine[i] == ',') szLine[i] = ' ';

        int f1 = -1, f2 = 0, f3 = 0, f4 = 0;
        sscanf(szLine, "%d %d %d %d", &f1, &f2, &f3, &f4);
        if (f1 == -1) break;

        g_InitItemTable[nCount][0] = (short)f1;
        g_InitItemTable[nCount][1] = (short)f2;
        g_InitItemTable[nCount][2] = (short)f3;
        g_InitItemTable[nCount][3] = (short)f4;
        nCount++;
    }

    fclose(fp);
}

// ============================================================
// LoadSkillData
// Lee ./SkillData.csv
// Formato: "%d %d %d %d %d %d %d %d %d %d %d %d %d %s %d %d %d %d %d %d %d"
//   campo 0:  ID skill (0..100)
//   campo 13: nombre del skill (string)
//   campo 9:  movimiento -> dividir por 4 (con redondeo)
// Sub-campos adicionales (campo 8): "%d.%d.%d.%d.%d.%d"
// Almacena en tabla global g_SkillData, stride 0x54 (84 bytes = 21 ints)
// ============================================================
void CFileDB::LoadSkillData()
{
    FILE* fp = fopen("./SkillData.csv", "rb");
    if (!fp)
        fp = fopen("./SkillData.txt", "rb");
    if (!fp)
    {
        MessageBox(NULL, "SkillData file not found", "DBSRV", MB_OK);
        return;
    }

    // Saltar header
    char szLine[0x400] = {0};
    if (!fgets(szLine, sizeof(szLine), fp)) { fclose(fp); return; }

    // Estructura de cada entrada: 21 ints = 84 bytes (0x54)
    // VA base: 0x186126C0 en binario

    // Buffer temporario de 0x54 bytes (inicializado a 0)
    SkillEntry entry;

    while (fgets(szLine, sizeof(szLine), fp))
    {
        // Reemplazar comas
        for (int i = 0; szLine[i] && i < 0x400; i++)
            if (szLine[i] == ',') szLine[i] = ' ';

        memset(&entry, 0, sizeof(entry));
        char szSkillName[64] = {0};

        // Formato completo: 21 campos, campo 13 es string
        sscanf(szLine,
            "%d %d %d %d %d %d %d %d %d %d %d %d %d %s %d %d %d %d %d %d %d",
            &entry.f[0],  &entry.f[1],  &entry.f[2],  &entry.f[3],
            &entry.f[4],  &entry.f[5],  &entry.f[6],  &entry.f[7],
            &entry.f[8],  &entry.f[9],  &entry.f[10], &entry.f[11],
            &entry.f[12], szSkillName,
            &entry.f[14], &entry.f[15], &entry.f[16], &entry.f[17],
            &entry.f[18], &entry.f[19], &entry.f[20]);

        int nSkillID = entry.f[0];

        // Validar rango 0..100
        if (nSkillID < 0 || nSkillID >= 0x65) continue;

        // Campo 9 (movimiento): dividir por 4 con redondeo hacia arriba para negativos
        // ASM: (eax + (eax>>31 & 3)) >> 2
        int mv = entry.f[9];
        entry.f[9] = (mv + (mv < 0 ? 3 : 0)) >> 2;

        // Parsear sub-campo 8 (efectos): formato "%d.%d.%d.%d.%d.%d"
        // El string en szSkillName puede contener los sub-efectos
        {
            int e1=0,e2=0,e3=0,e4=0,e5=0,e6=0;
            sscanf(szSkillName, "%d.%d.%d.%d.%d.%d", &e1,&e2,&e3,&e4,&e5,&e6);
            // Almacenar en campos de efectos del entry
            // El binario los guarda en offsets -0x468..-0x463 (6 bytes)
            // que corresponden a campos adicionales del struct
        }

        // Copiar a tabla global
        memcpy(&g_SkillData[nSkillID], &entry, sizeof(SkillEntry));
    }

    fclose(fp);
}

// ============================================================
// OpenLogFile
// Abre/rota archivo de log diario
// Formato: ".\Log\DB<year>_<month>_<day>_<hour>_<min><hex_suffix>.txt"
// Modo apertura: "wt" (write text)
// Si habia log previo abierto lo cierra primero
// ============================================================

void CFileDB::OpenLogFile(int nSuffix)
{
    // Cerrar log anterior si estaba abierto
    if (g_pLogFile)
    {
        // El binario loguea "Logfile close fail!!" si fclose falla
        if (fclose(g_pLogFile) != 0)
            TNDebug("Logfile close fail!!\n");
        g_pLogFile = NULL;
    }

    SYSTEMTIME st;
    GetLocalTime(&st);

    // Formato: .\Log\DB%4.4d_%2.2d_%2.2d_%2.2d_%2.2d<hex>.txt
    // El binario suma 0x76C al parametro como sufijo hex
    sprintf(g_szLogPath, ".\\Log\\DB%04d_%02d_%02d_%02d_%02d%X.txt",
            st.wYear, st.wMonth, st.wDay,
            st.wHour, st.wMinute,
            nSuffix + 0x76C);

    g_pLogFile = fopen(g_szLogPath, "wt");
}
