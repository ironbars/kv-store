#ifndef KV_CLI_H
#define KV_CLI_H
#include "kv.h"

// Function declarations
struct addrinfo* get_addr_list(const char *port, char *host);
int prep_socket(struct addrinfo *list);
void* get_in_addr(struct sockaddr *sa);

// Utility function; converts string to upper case
void str_upper(char *str);

#endif
