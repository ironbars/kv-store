#ifndef UNIX_KV_H
#define UNIX_KV_H
#include <string.h>

#include "kv.h"

//Wrapper for strtok_r
char* unixkv_strtok(char*,const char*, char**);

//Wrapper for new thread function
void* unixkv_new_thread(void (*thread_main)(void*), void*);

#endif
