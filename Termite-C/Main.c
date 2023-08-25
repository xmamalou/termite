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

void trmTest(void* pParams)
{
    printf("Hello, World!");
    for(int i = 0; i < 100; i++)
    {
        printf("I love threads!\n");
    }
}

int main()
{
    TrmThreadProcess proc = &trmTest;

    struct TrmThreadInfo info = {
        .paramSize = 0,
        .pProc = proc
    };

    TrmThread thread = trmThreadCreate(&info);
    if(trmThreadErrorGet(thread) < 0)
    {
        printf("Error: %d", trmThreadErrorGet(thread));
    }

    trmThreadWait(thread);

    return 0;
}