#include "winkv.h"

os_kv_strtok kv_strtok = winkv_strtok;

char* winkv_strtok(char *msg, const char *delim, char **NEXT)
{
    return (char*)strtok_s(msg, delim, NEXT);
}
