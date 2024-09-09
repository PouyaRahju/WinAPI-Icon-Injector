/*
* Pouya Rahju
* https://github.com/PouyaRahju/WinAPI-Icon-Injector 
* Inject Icons to .exe Files with WinAPI - windows.h 
*/

#include <stdio.h>
#include "SetIcon.h"

int main(int argc , char* argv[])
{
    struct IconDLLIDs iconsID;
    iconsID.i128px = 251; // Icon Number in DLL  256*256 or 128*128
    iconsID.i64px = 252;  // Icon Number in DLL   64*64
    iconsID.i48px = 253;  // Icon Number in DLL   48*48
    iconsID.i40px = 254;  // Icon Number in DLL   40*40
    iconsID.i32px = 255;  // Icon Number in DLL   32*32
    iconsID.i24px = 256;  // Icon Number in DLL   24*24
    iconsID.i20px = 257;  // Icon Number in DLL   20*20
    iconsID.i16px = 258;  // Icon Number in DLL   16*16
    iconsID.dllFile = "shell32.dll"; // Target DLL to extract icons
    
    // Try to add or update the Icons of target.exe
    int result = SetIcon("target.exe",iconsID);
    // Check Result
    if (result == SUCCESS )
    {
        printf("Icons Successfuly added.")
    }else{
        printf("Error Code: %d", result )
    }
    
    return (0);
}