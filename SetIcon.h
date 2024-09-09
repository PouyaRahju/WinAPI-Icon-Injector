/*
* Pouya Rahju
* https://github.com/PouyaRahju/WinAPI-Icon-Injector  
* Inject Icons to .exe Files with WinAPI - windows.h
*/

#ifndef SETICON_H
#define SETICON_H

#include <windows.h>
#include <stdio.h>

#define DEBUG_PRINT 0
#define SUCCESS 0
#define ERROR_CANT_OPEN_FILE 1
#define ERROR_CANT_EXTRACT_ICONS 2
#define ERROR_CANT_ADD_ICONS 3
#define ERROR_CANT_ADD_ICONGROUP 4
#define ERROR_CANT_UPDATE_RESOURCE 5

// Set target dll and all different size of Icons.
struct IconDLLIDs
{
    LPCSTR dllFile;
    WORD i128px;
    WORD i64px;
    WORD i48px;
    WORD i40px;
    WORD i32px;
    WORD i24px;
    WORD i20px;
    WORD i16px;
};

// Function to print error messages while DEBUG_PRINT 1
void PrintDebugLog(LPCSTR msg)
{
    if (!DEBUG_PRINT)
        return;

    DWORD error = GetLastError();
    LPVOID lpMsgBuf;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&lpMsgBuf,
        0, NULL);
    printf("%s: %s\n", msg, (LPSTR)lpMsgBuf);
    LocalFree(lpMsgBuf);
}

// Function to extract an icon from DLL and save it to a file
BOOL ExtractIconFromDll(LPCSTR dllFile, int iconIndex, LPCSTR outputFile)
{
    HMODULE hModule = LoadLibraryA(dllFile);
    if (hModule == NULL)
    {
        PrintDebugLog("Failed to load DLL");
        return FALSE;
    }

    HRSRC hRes = FindResourceA(hModule, MAKEINTRESOURCEA(iconIndex), RT_ICON);
    if (hRes == NULL)
    {
        PrintDebugLog("Failed to find resource");
        FreeLibrary(hModule);
        return FALSE;
    }

    HGLOBAL hResData = LoadResource(hModule, hRes);
    if (hResData == NULL)
    {
        PrintDebugLog("Failed to load resource");
        FreeLibrary(hModule);
        return FALSE;
    }

    void *pResData = LockResource(hResData);
    DWORD resSize = SizeofResource(hModule, hRes);
    if (pResData == NULL || resSize == 0)
    {
        PrintDebugLog("Failed to lock resource");
        FreeLibrary(hModule);
        return FALSE;
    }

    HANDLE hFile = CreateFileA(outputFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        PrintDebugLog("Failed to create file");
        FreeLibrary(hModule);
        return FALSE;
    }

    DWORD written;
    if (!WriteFile(hFile, pResData, resSize, &written, NULL))
    {
        PrintDebugLog("Failed to write file");
        CloseHandle(hFile);
        FreeLibrary(hModule);
        return FALSE;
    }

    CloseHandle(hFile);
    FreeLibrary(hModule);
    return TRUE;
}

// Function to add icon resources to an EXE file
BOOL AddIconResource(HANDLE hUpdateRes, LPCSTR iconFile, WORD iconId)
{
    HANDLE hIconFile = CreateFileA(iconFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hIconFile == INVALID_HANDLE_VALUE)
    {
        PrintDebugLog("Failed to open ICO file");
        return FALSE;
    }

    DWORD iconFileSize = GetFileSize(hIconFile, NULL);
    if (iconFileSize == INVALID_FILE_SIZE)
    {
        PrintDebugLog("Failed to get ICO file size");
        CloseHandle(hIconFile);
        return FALSE;
    }

    BYTE *iconData = (BYTE *)malloc(iconFileSize);
    if (iconData == NULL)
    {
        PrintDebugLog("Memory allocation failed");
        CloseHandle(hIconFile);
        return FALSE;
    }

    DWORD bytesRead;
    if (!ReadFile(hIconFile, iconData, iconFileSize, &bytesRead, NULL))
    {
        PrintDebugLog("Failed to read ICO file");
        free(iconData);
        CloseHandle(hIconFile);
        return FALSE;
    }
    CloseHandle(hIconFile);

    if (!UpdateResourceA(hUpdateRes, RT_ICON, MAKEINTRESOURCEA(iconId), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), iconData, iconFileSize))
    {
        PrintDebugLog("Failed to update resource");
        free(iconData);
        return FALSE;
    }

    free(iconData);
    return TRUE;
}

// Function to add an icon group resource to the EXE file
BOOL AddIconGroupResource(HANDLE hUpdateRes, WORD *iconIds, int count)
{
    struct IconDir
    {
        WORD idReserved;
        WORD idType;
        WORD idCount;
        struct IconDirEntry
        {
            BYTE bWidth;
            BYTE bHeight;
            BYTE bColorCount;
            BYTE bReserved;
            WORD wPlanes;
            WORD wBitCount;
            DWORD dwBytesInRes;
            DWORD dwImageOffset;
        } idEntries[1];
    } *iconDir;

    // Calculate size for icon directory
    DWORD dirSize = sizeof(struct IconDir) + sizeof(struct IconDirEntry) * (count - 1);
    iconDir = (struct IconDir *)malloc(dirSize);
    if (iconDir == NULL)
    {
        PrintDebugLog("Memory allocation failed");
        return FALSE;
    }

    iconDir->idReserved = 0;
    iconDir->idType = 1; // Icon type
    iconDir->idCount = count;
    WORD iconsSizes[] = {128, 64, 48, 40, 32, 24, 20, 16};
    // Populate the entries with actual values
    for (int i = 0; i < count; i++)
    {
        // Assuming each icon is 256x256 and has 32-bit color depth (this may need to be adjusted based on actual icons)
        iconDir->idEntries[i].bWidth = iconsSizes[i];  // Width in pixels
        iconDir->idEntries[i].bHeight = iconsSizes[i]; // Height in pixels
        iconDir->idEntries[i].bColorCount = 0;         // 0 for 24-bit or higher color depth
        iconDir->idEntries[i].bReserved = 0;
        iconDir->idEntries[i].wPlanes = 1;
        iconDir->idEntries[i].wBitCount = 32;    // Assuming 32-bit color depth
        iconDir->idEntries[i].dwBytesInRes = i;  // This will be updated later
        iconDir->idEntries[i].dwImageOffset = i; // This will be updated later
    }

    // Update the icon group resource
    if (!UpdateResourceA(hUpdateRes, RT_GROUP_ICON, MAKEINTRESOURCEA(1), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), iconDir, dirSize))
    {
        PrintDebugLog("Failed to update icon group resource");
        free(iconDir);
        return FALSE;
    }

    free(iconDir);
    return TRUE;
}

// Inject Icon to exeFile
int SetIcon(LPCSTR exeFile, struct IconDLLIDs iconDllNums)
{

    HANDLE hUpdateRes = BeginUpdateResourceA(exeFile, FALSE);
    if (hUpdateRes == NULL)
    {
        PrintDebugLog("Cannot open EXE file for updating");
        return ERROR_CANT_OPEN_FILE;
    }
    char *iconFNames[] = {"C:\\ProgramData\\128i", "C:\\ProgramData\\64i", "C:\\ProgramData\\48i", "C:\\ProgramData\\40i", "C:\\ProgramData\\32i", "C:\\ProgramData\\24i", "C:\\ProgramData\\20i", "C:\\ProgramData\\16i"};

  
    // Extract icons from DLL
    if (!ExtractIconFromDll(iconDllNums.dllFile, iconDllNums.i128px, iconFNames[0]) ||
        !ExtractIconFromDll(iconDllNums.dllFile, iconDllNums.i64px, iconFNames[1]) ||
        !ExtractIconFromDll(iconDllNums.dllFile, iconDllNums.i48px, iconFNames[2]) ||
        !ExtractIconFromDll(iconDllNums.dllFile, iconDllNums.i40px, iconFNames[3]) ||
        !ExtractIconFromDll(iconDllNums.dllFile, iconDllNums.i32px, iconFNames[4]) ||
        !ExtractIconFromDll(iconDllNums.dllFile, iconDllNums.i24px, iconFNames[5]) ||
        !ExtractIconFromDll(iconDllNums.dllFile, iconDllNums.i20px, iconFNames[6]) ||
        !ExtractIconFromDll(iconDllNums.dllFile, iconDllNums.i16px, iconFNames[7]))
    {
        EndUpdateResourceA(hUpdateRes, TRUE);
        return ERROR_CANT_EXTRACT_ICONS;
    }

    // Add icons to EXE with specific IDs
    if (!AddIconResource(hUpdateRes, iconFNames[0], 1) ||
        !AddIconResource(hUpdateRes, iconFNames[1], 2) ||
        !AddIconResource(hUpdateRes, iconFNames[2], 3) ||
        !AddIconResource(hUpdateRes, iconFNames[3], 4) ||
        !AddIconResource(hUpdateRes, iconFNames[4], 5) ||
        !AddIconResource(hUpdateRes, iconFNames[5], 6) ||
        !AddIconResource(hUpdateRes, iconFNames[6], 7) ||
        !AddIconResource(hUpdateRes, iconFNames[7], 8))
    {
        EndUpdateResourceA(hUpdateRes, TRUE);
        return ERROR_CANT_ADD_ICONS;
    }
    for (int i = 0; i < 8; i++)
    {
        remove(iconFNames[i]);
    }
    // Define icon IDs for the group
    WORD iconIds[] = {1, 2, 3, 4, 5, 6, 7, 8};
    if (!AddIconGroupResource(hUpdateRes, iconIds, sizeof(iconIds) / sizeof(iconIds[0])))
    {
        EndUpdateResourceA(hUpdateRes, TRUE);
        return ERROR_CANT_ADD_ICONGROUP;
    }

    if (!EndUpdateResourceA(hUpdateRes, FALSE))
    {
        PrintDebugLog("Failed to finalize resource update");
        return ERROR_CANT_UPDATE_RESOURCE;
    }

    PrintDebugLog("Icons added successfully.\n");
    return SUCCESS;
}

#endif