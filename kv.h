#ifndef KV_H
#define KV_H
#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h> /* fork(), close() */
#include <sys/types.h>
#include <sys/socket.h> /* socket(), accept(), bind(), listen(),
						   struct addrinfo, sockaddr */
#include <netdb.h> /* getaddrinfo(), freeaddrinfo(), gai_strerror(),
					  struct addrinfo */
#include <netinet/in.h> /* struct sockaddr_in, in_addr */
#include <arpa/inet.h> /* inet_ntop() */
#include <errno.h> /* errno */

// Include system specific function headers if necessary
#if defined(_WIN32)
#include "winkv.h"
#endif
#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || \
		(defined(__APPLE__) && defined(__MACH__)))
/* UNIX-style OS. ------------------------------------------- */
#include "unixkv.h"
#endif

#define ARRAYLEN 10
#define KEYLEN 32

typedef struct kv_node kv_node;
typedef struct kv_args kv_args;
typedef enum kv_status kv_status;

// Pointers to OS specific functions
// Create new thread
typedef void* (*os_kv_new_thread)(void (*kv_new_thread)(void*), void*);

// String token function
typedef char* (*os_kv_strtok)(char*, const char*, char**);

os_kv_strtok kv_strtok;
os_kv_new_thread kv_new_thread;

extern const int BUF_SIZE;
extern const char *PORT;

// Serialization functions
unsigned char* serialize_int(unsigned char *buffer, uint32_t value);
unsigned char* serialize_char(unsigned char *buffer, char value);
unsigned char* serialize_string(unsigned char *buffer, char *value);
unsigned char* serialize_kv_status(unsigned char *buffer, kv_status value);
unsigned char* serialize_kv_message(unsigned char *buffer, kv_args *msg);

// Deserialization functions
unsigned char* deserialize_int(unsigned char *buffer, uint32_t *value);
unsigned char* deserialize_char(unsigned char *buffer, char *value);
unsigned char* deserialize_string(unsigned char *buffer, char *value);
unsigned char* deserialize_kv_status(unsigned char *buffer, kv_status *value);
unsigned char* deserialize_kv_message(unsigned char *buffer, kv_args *msg);

// Generic hashing function
uint32_t hashkey(char *key);

enum kv_status {
	// Key-value status codes
	KV_OK = 0,
	KV_NOK = 1,

	// Key-value opcodes
	KV_GET = 10,
	KV_PUT = 11,
	KV_DEL = 12
};

struct kv_node {
	char c_key[KEYLEN];
	uint32_t i_value;
	struct kv_node *next;
};

// This structure will also be used for message passing
struct kv_args {
	char *c_key;
	uint32_t i_value;
	kv_status status;
};

#endif
