// ============================================================
// CFileDB_NEW_funcs3.cpp
// ReadConfig + configuracion Settings.ini
// Reconstruida de DBSRV_NEW.exe (VA 0x439FA0, 0x442F30)
// ============================================================

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include "Basedef.h"
#include "CFileDB.h"
#include "TNDebug.h"

// ============================================================
// Globals de configuracion (Settings.ini)
// Equivalentes a las VAs del binario NEW
// ============================================================

// CountryID: 0=KOREA 1=CHINA 2=JAPAN 3=INDONESIA 4=PHILIPPINES 5=TAIWAN 6=MEXICO 7=GLOBAL
// VA 0x47D1C4 en NEW - ya existe en Basedef como g_eCountryID, pero el NEW lo guarda aqui
int  g_nCountryID   = 1;   // espejo de g_eCountryID para compatibilidad

// Indices de servidores (Settings.ini [Server])
// SWorld1: primer grupo de world servers
// SWorld2: segundo grupo
// DWorld:  servidor de DB
int  g_nSWorld1     = -1;  // VA 0x47D1CC
int  g_nSWorld2     = -1;  // VA 0x47D1D0
int  g_nDWorld      = -1;  // VA 0x47D1D4

// Tamanos de mundo (Settings.ini [Size])
int  g_nWorld1Size  = 0;   // VA 0x48C72C
int  g_nWorld2Size  = 0;   // VA 0x48C730

// Limite de edad (Settings.ini [Limit] Age)
int  g_nAgeLimit    = 10;  // VA 0x48C6EC

// Nombre del archivo Settings.ini (VA 0x477550 en NEW)
// El binario lo hardcodea como string en la seccion .rdata
static const char s_szSettingsIni[] = ".\\Settings.ini";

// ============================================================
// ReadConfig_Country  (VA 0x442F30)
// Lee [Country] Name de Settings.ini
// Determina g_nCountryID y g_nAgeLimit
// ============================================================
static void ReadSettings_Country()
{
    // El NEW tiene g_nCountryID=1 como valor inicial en .data (VA 0x47D1C4)
    // Si no existe Settings.ini, mantener ese default
    char szCountry[64] = {0};

    GetPrivateProfileStringA(
        "Country",
        "Name",
        "GLOBAL",   // default = CHINA (igual que el .data del NEW)
        szCountry,
        sizeof(szCountry),
        s_szSettingsIni);

    // _strupr equivalente (el NEW llama 0x426744 = _strupr antes de comparar)
    CharUpperA(szCountry);

    if      (!strncmp(szCountry, "KOREA",       40)) g_nCountryID = 0;
    else if (!strncmp(szCountry, "CHINA",       40)) g_nCountryID = 1;
    else if (!strncmp(szCountry, "JAPAN",       40)) g_nCountryID = 2;
    else if (!strncmp(szCountry, "INDONESIA",   40)) g_nCountryID = 3;
    else if (!strncmp(szCountry, "PHILIPPINES", 40)) g_nCountryID = 4;
    else if (!strncmp(szCountry, "TAIWAN",      40)) g_nCountryID = 5;
    else if (!strncmp(szCountry, "MEXICO",      40)) g_nCountryID = 6;
    else if (!strncmp(szCountry, "GLOBAL",      40)) g_nCountryID = 7;

    // [Limit] Age = 10 (default)
    char szAge[20] = {0};
    GetPrivateProfileStringA("Limit", "Age", "10", szAge, sizeof(szAge), s_szSettingsIni);
    g_nAgeLimit = atoi(szAge);
}

// ============================================================
// ReadConfig_Servers  (VA 0x439FA0)
// Lee [Server] SWorld1/SWorld2/DWorld y [Size] World1/World2
// ============================================================
static void ReadSettings_Servers()
{
    char szVal[20] = {0};

    // [Server] SWorld1 = -1 (default)
    GetPrivateProfileStringA("Server", "SWorld1", "-1", szVal, sizeof(szVal), s_szSettingsIni);
    g_nSWorld1 = atoi(szVal);

    // [Server] SWorld2 = -1
    memset(szVal, 0, sizeof(szVal));
    GetPrivateProfileStringA("Server", "SWorld2", "-1", szVal, sizeof(szVal), s_szSettingsIni);
    g_nSWorld2 = atoi(szVal);

    // [Server] DWorld = -1
    memset(szVal, 0, sizeof(szVal));
    GetPrivateProfileStringA("Server", "DWorld", "-1", szVal, sizeof(szVal), s_szSettingsIni);
    g_nDWorld = atoi(szVal);

    // [Size] World1 = 0
    memset(szVal, 0, sizeof(szVal));
    GetPrivateProfileStringA("Size", "World1", "0", szVal, sizeof(szVal), s_szSettingsIni);
    g_nWorld1Size = atoi(szVal);

    // [Size] World2 = 0
    memset(szVal, 0, sizeof(szVal));
    GetPrivateProfileStringA("Size", "World2", "0", szVal, sizeof(szVal), s_szSettingsIni);
    g_nWorld2Size = atoi(szVal);
}

// ============================================================
// ReadConfig  (llamada desde InitInstance del NEW)
// Punto de entrada principal - lee Settings.ini completo
// En Server.cpp se llama como ReadConfig() despues de "start log"
// ============================================================
void ReadSettings()
{
    // Verificar que el archivo exista
    FILE* fp = fopen(s_szSettingsIni, "r");
    if (!fp)
    {
        // El binario muestra MessageBox si no existe Settings.ini
        // pero no aborta - usa defaults
        char szLog[256] = {0};
        sprintf(szLog, "Settings.ini not found, using defaults");
        // [removido: no va a EditHistory.txt]
    }
    else
    {
        fclose(fp);
    }

    ReadSettings_Servers();
    ReadSettings_Country();

    // Log de lo leido
    char szLog[256] = {0};
    sprintf(szLog, "ReadConfig: Country=%d SWorld1=%d SWorld2=%d DWorld=%d AgeLimit=%d",
            g_nCountryID, g_nSWorld1, g_nSWorld2, g_nDWorld, g_nAgeLimit);
    // [removido: no va a EditHistory.txt]

    if (g_pLogFile)
    {
        fprintf(g_pLogFile, "%s\n", szLog);
        fflush(g_pLogFile);
    }
}
