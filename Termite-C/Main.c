/*
* NOTE: THIS EXISTS ONLY AS A DUMMY FILE!
* WHEN TERMITE REACHES A DESIRED WORKING STATE, THIS FILE WILL BE DELETED!
* WHEN THAT HAPPENS, TERMITE WILL BE SET TO COMPILE AS A DYNAMIC LIBRARY!
*/

#include <stdio.h>
#include <stdint.h>

#ifdef _WIN32
    #include <windows.h>
    #include <VersionHelpers.h>
    #include <shellapi.h>
#endif

#ifdef _WIN32
    #pragma comment (lib, "shell32.lib")
    #pragma comment (lib, "user32.lib")
    #pragma comment (lib, "gdi32.lib")
    #pragma comment (lib, "dwmapi.lib")
#endif

#include "Termite.h"

int main()
{
    TrmMemoryPool memPool = NULL;
    TrmBuffer     buffer  = NULL;

    struct TrmMemoryPoolInfo memPoolInfo = {
        .size = 1024,
        .device = NULL,
    };

    memPool = trmMemoryPoolCreate(&memPoolInfo);
    if (trmMemoryPoolErrorGet(memPool) < TRM_SUCCESS)
        return 1;

    printf("MEMORY SIZE: %d\nMEMORY BLOCKS: %d", trmMemoryPoolSizeGet(memPool), trmMemoryPoolBlockCountGet(memPool));
}