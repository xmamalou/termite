#include "../Internal.h"

#include <stdlib.h>

TrmThread trmThreadCreate(struct TrmThreadInfo* info)
{
    struct TrmThread_T* thread = calloc(1, sizeof(struct TrmThread_T));
    
    if (thread == NULL)
        return NULL;
    thread->info = *info;

#ifdef _WIN32
    thread->hThread = CreateThread(NULL, info->stackSize, info->pProc, info->pParam, 0, NULL);

    if (thread->hThread == NULL)
    {
        thread->error = TRM_THREAD_COULDNT_CREATE_ERROR;
    }
#else 
    thread->id = pthread_create(&thread->hThread, NULL, info->pProc, info->pParam);
#endif

    return (TrmThread)thread;
}

inline void trmThreadWait(TrmThread hThread)
{
#ifdef _WIN32
    WaitForSingleObject(TRM_HANDLE(Thread)->hThread, INFINITE);
#else
    pthread_join(TRM_HANDLE(Thread)->hThread, NULL);
#endif
}

inline bool trmThreadIsRunning(TrmThread hThread)
{
#ifdef _WIN32
    return WaitForSingleObject(TRM_HANDLE(Thread)->hThread, 0) == WAIT_TIMEOUT;
#else 
    return pthread_kill(TRM_HANDLE(Thread)->hThread, 0) == 0;
#endif
}

inline int trmThreadErrorGet(TrmThread hThread)
{
    return TRM_HANDLE(Thread)->error;
}
