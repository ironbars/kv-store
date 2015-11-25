#include <pthread.h>

#include "unixkv.h"

os_kv_strtok kv_strtok = unixkv_strtok;
os_kv_new_thread kv_new_thread = unixkv_new_thread;

char* unixkv_strtok(char *msg, const char *delim, char **NEXT)
{
    return (char*)strtok_r(msg, delim, NEXT);
}

void* unixkv_new_thread(void (*thread_main)(void*), void *args)
{
    void *(*unix_thread_main) (void *) = (void*(*)(void*))thread_main;
    pthread_t *thread_handle = (pthread_t*) malloc(sizeof(pthread_t));

    pthread_create(thread_handle, NULL, unix_thread_main, args);
    return (void*)thread_handle;
}
