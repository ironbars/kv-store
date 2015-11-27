#ifndef KV_CLI_H
#define KV_CLI_H
#include "kv.h"

const short FIRST_PARTITION = 3;
const short SECOND_PARTITION = 7;
const short THIRD_PARTITION = 9;

// Function declarations
struct addrinfo* get_addr_list(const char *port, char *host);
int prep_socket(struct addrinfo *list);
void* get_in_addr(struct sockaddr *sa);
int get_socket_index(char *key);

// Utility function; converts string to upper case
void str_upper(char *str);

#endif
