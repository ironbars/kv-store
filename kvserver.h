#ifndef KV_SERV_H
#define KV_SERV_H
#include <sys/wait.h> /* waitpid() */
#include <signal.h> /* struct sigaction, sigaction(), sigemptyset() */

#include "kv.h"

// Function declarations
void handle_request(void*);
void put(void*); //Insert a key value pair
void get(void*); //Retrieve the value for a given key
void del(void*); //Remove a key value pair

// TODO: Evaluate the necessity of this function
void sigchld_handler(int s); //Handle forked processes

struct addrinfo* get_addr_list(const char *port); //Get list of interfaces
int prep_socket(struct addrinfo *list, int yes); //Get an open socket

// Generic hashing function
uint32_t hashkey(char *key);

// Variable declarations
// Pointer to our key array
kv_node **keys;
// Lock for kv_args
pthread_mutex_t kv_lock;

#endif
