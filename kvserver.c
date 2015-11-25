#include <pthread.h>

#include "kvserver.h"

int main()
{
	int sockfd, clientfd;
	struct addrinfo *serv_info;
	struct sockaddr_storage client_addr;
	socklen_t sin_size;
	struct sigaction sa;
	int yes = 1;
	kv_node *arrkeys[ARRAYLEN] = {NULL};
	keys = &arrkeys[0];

	if(pthread_mutex_init(&kv_lock, NULL) != 0) {
		fprintf(stderr, "Mutex initialization failed.\n");
		exit(EXIT_FAILURE);
	}

	serv_info = get_addr_list(PORT);

	if((sockfd = prep_socket(serv_info, yes)) == -1) {
		exit(EXIT_FAILURE);
	}

	if(listen(sockfd, 1) == -1) {
		perror("kvserver: listen");
		exit(EXIT_FAILURE);
	}

	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	if(sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("kvserver: sigaction");
		exit(EXIT_FAILURE);
	}

	printf("kvserver: Ready for connections...\n");

	while(1) {
		sin_size = sizeof(client_addr);
		clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &sin_size);

		if(clientfd == -1) {
			perror("kvserver: accept");
			continue;
		}

		kv_new_thread(handle_request, &clientfd);
	}

	close(sockfd);

	return 0;
}


void handle_request(void *targs)
{
	int recvd;
	unsigned char recv_buf[BUF_SIZE], send_buf[BUF_SIZE], *ptr;
	int sock = *((int*) targs);

	while((recvd = recv(sock, recv_buf, BUF_SIZE, 0)) != 0) {
		if(recvd > 0) {
			kv_args *kvargs = (kv_args*) malloc(sizeof(kv_args));
			kvargs->c_key = (char*) malloc(sizeof(char) * KEYLEN);
			ptr = deserialize_kv_message(recv_buf, kvargs);

			switch(kvargs->status) {
				case KV_PUT:
					put(kvargs);
					break;

				case KV_GET:
					get(kvargs);
					break;

				case KV_DEL:
					del(kvargs);
					break;

				default:
					break;
			}

			ptr = serialize_kv_message(send_buf, kvargs);

			if(send(sock, send_buf, ptr - send_buf, 0) == -1) {
				perror("kvserver: send");
			}

			free(kvargs->c_key);
			free(kvargs);
		}

		else {
			perror("kvserver: recv");
		}
	}

	close(sock);
}


void put(void *args)
{
	int i;
	char *key = ((kv_args*) args)->c_key;
	int value = ((kv_args*) args)->i_value;
	uint32_t index = hashkey(key) % ARRAYLEN;
	kv_node *new_node = (kv_node*) malloc(sizeof(kv_node));

	for(i = 0; (i < KEYLEN || *(key + i) != '\0'); i++) {
		new_node->c_key[i] = *(key + i);
	}

	new_node->i_value = value;
	new_node->next = NULL;

	kv_node **node;
	node = &keys[index];

	pthread_mutex_lock(&kv_lock);

	while(*node) {
		if(strcmp((*node)->c_key, key) == 0) {
			(*node)->i_value = value;
		}

		node = &((*node)->next);
	}

	(*node) = new_node;

	pthread_mutex_unlock(&kv_lock);

	((kv_args*) args)->status = KV_OK;
}


void get(void* args)
{
	char *key = ((kv_args*) args)->c_key;
	uint32_t index = hashkey(key) % ARRAYLEN;
	kv_node *node = keys[index];

	pthread_mutex_lock(&kv_lock);

	while(node) {
		if (strcmp(node->c_key, key) == 0) {
			((kv_args*) args)->i_value = node->i_value;
			((kv_args*) args)->status = KV_OK;
			pthread_mutex_unlock(&kv_lock);
			return;
		}

		else
			node = node->next;
	}

	pthread_mutex_unlock(&kv_lock);

	((kv_args*) args)->status = KV_NOK;
}


void del(void* args)
{
	char *key = ((kv_args*) args)->c_key;

	//Grab hash and mod it w/ array length
	uint32_t index = hashkey(key) % ARRAYLEN;

	//Iterate through list and try to find the key
	kv_node **node = &keys[index];
	kv_node *prev_node = *node; //pointer to previous node in list

	pthread_mutex_lock(&kv_lock);

	while(*node) {
		// Node to be deleted has been found
		if (strcmp((*node)->c_key, key) == 0) {

			// Make sure list isn't broken if deletion node isn't last in the
			// list
			if ((*node)->next) {
				// This condition implies that the node to be deleted is the
				// first node on this chain
				if(*node == prev_node) {
					*node = (*node)->next;
					free(prev_node);
				}

				else {
					kv_node *del_node = prev_node->next; //Node to be deleted
					prev_node->next = (*node)->next;
					free(del_node);
				}
			}

			else {
				free(*node);
			}

			((kv_args*) args)->status = KV_OK;
			pthread_mutex_unlock(&kv_lock);
			return;
		}

		else {
			prev_node = *node;
			node = &((*node)->next);
		}
	}

	pthread_mutex_unlock(&kv_lock);

	((kv_args*) args)->status = KV_NOK;
}


uint32_t hashkey(char *key)
{
	uint32_t hashval = 0; //our hash
	int i = 0;

	/* Convert our string to an integer */
	while(hashval < ULONG_MAX && i < strlen(key)) {
		hashval = hashval << 8;
		hashval += key[i];
		i++;
	}

	return hashval;
}


void sigchld_handler(int s)
{
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}


struct addrinfo* get_addr_list(const char *port)
{
	int status;
	struct addrinfo hints, *serv;

	memset(&hints, 0, sizeof(hints));

	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if((status = getaddrinfo(NULL, port, &hints, &serv)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		exit(EXIT_FAILURE);
	}

	return serv;
}


int prep_socket(struct addrinfo *list, int yes)
{
	struct addrinfo *p;
	int sock;

	for(p = list; p!= NULL; p = p->ai_next) {
		if((sock = socket(p->ai_family, p->ai_socktype,
						p->ai_protocol)) == -1) {
			perror("kvserver: socket:");
			continue;
		}

		if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes,
					sizeof(int)) == -1) {
			perror("kvserver: setsockopt");
			exit(EXIT_FAILURE);
		}

		if(bind(sock, p->ai_addr, p->ai_addrlen) == -1) {
			close(sock);
			perror("kvserver: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(list);

	return sock;
}
