#include "kvclient.h"

int main(int argc, char **argv)
{
	int sockfd;
	int index;
	const short NUM_HOSTS = 3;
	int sockets[NUM_HOSTS], i;
	unsigned char send_buf[BUF_SIZE], recv_buf[BUF_SIZE], *ptr;
	struct addrinfo *servinfo;
	char *hosts[3] = {"pi-host1", "pi-host2", "pi-host3"};
	char operation[32];
	char *command = '\0';

	/*if(argc != 2) {*/
		/*fprintf(stderr, "Usage: kv-cli <hostname>\n");*/
		/*exit(EXIT_FAILURE);*/
	/*}*/

	for(i = 0; i < NUM_HOSTS; i++) {

		servinfo = get_addr_list(PORT, hosts[i]);

		if((sockets[i] = prep_socket(servinfo)) == -1) {
			fprintf(stderr, "kv-cli: failed to connect\n");
			exit(EXIT_FAILURE);
		}
	}

	printf("Type 'exit' to end\n");

	/* TODO: Implement choice of which server to send to. */
	while((!command) || strcmp("EXIT", command) != 0) {
		printf("kv-cli> ");
		fgets(operation, 100, stdin);

		char *p_operation = operation;
		char *next_token = NULL;
		command = kv_strtok(p_operation, " \n", &next_token);

		str_upper(command);

		if(command == NULL)
			continue;

		else if(strcmp("PUT", command) == 0) {
			char *key = kv_strtok(NULL, " \n", &next_token);
			char *value = kv_strtok(NULL, " \n", &next_token);

			if(!key || !value) {
				fprintf(stderr, "Error: command format is PUT <KEY> <VALUE>\n");
			}

			else {
				kv_args *args = (kv_args*) malloc(sizeof(kv_args));
				args->c_key = key;
				args->i_value = strtol(value, NULL, 0);
				args->status = KV_PUT;
				index = get_socket_index(args->c_key);
				sockfd = sockets[index];
				printf("Index: %i\n", index);

				ptr = serialize_kv_message(send_buf, args);

				if(send(sockfd, send_buf, ptr - send_buf, 0) == -1) {
					free(args);
					perror("send");
					continue;
				}

				if(recv(sockfd, recv_buf, BUF_SIZE, 0) == -1) {
					free(args);
					perror("recv");
					continue;
				}

				else {
					deserialize_kv_message(recv_buf, args);

					if(args->status != KV_OK) {
						fprintf(stderr, "PUT operation failed.\n");
					}

					else {
						printf("PUT operation succeeded.\n");
					}

					free(args);
				}
			}
		}

		else if(strcmp("GET", command) == 0) {
			char *key = kv_strtok(NULL, " \n", &next_token);

			if(!key) {
				fprintf(stderr, "Error: command format is GET <KEY>\n");
			}

			else {
				kv_args *args = (kv_args*) malloc(sizeof(kv_args));
				args->c_key = key;
				args->i_value = 0;
				args->status = KV_GET;
				ptr = serialize_kv_message(send_buf, args);
				index = get_socket_index(args->c_key);
				sockfd = sockets[index];
				printf("Index: %i\n", index);

				if(send(sockfd, send_buf, ptr - send_buf, 0) == -1) {
					free(args);
					perror("send");
					continue;
				}

				if(recv(sockfd, recv_buf, BUF_SIZE, 0) == -1) {
					free(args);
					perror("recv");
					continue;
				}

				else {
					deserialize_kv_message(recv_buf, args);

					if(args->status != KV_OK) {
						fprintf(stderr, "Key <%s> is not in store.\n",
								args->c_key);
					}

					else {
						printf("Key has value: %u\n", args->i_value);
					}
				}

				free(args);
			}
		}

		else if(strcmp("DEL", command) == 0) {
			char *key = kv_strtok(NULL, " \n", &next_token);

			if(!key) {
				fprintf(stderr, "Error: command format is DEL <KEY>.\n");
			}

			else {
				kv_args *args = (kv_args*) malloc(sizeof(kv_args*));
				args->c_key = key;
				args->i_value = 0;
				args->status = KV_DEL;
				ptr = serialize_kv_message(send_buf, args);
				index = get_socket_index(args->c_key);
				sockfd = sockets[index];
				printf("Index: %i\n", index);

				if(send(sockfd, send_buf, ptr - send_buf, 0) == -1) {
					free(args);
					perror("send");
					continue;
				}

				if(recv(sockfd, recv_buf, BUF_SIZE, 0) == -1) {
					free(args);
					perror("recv");
					continue;
				}

				else {
					deserialize_kv_message(recv_buf, args);

					if(args->status != KV_OK) {
						fprintf(stderr, "Key <%s> not in store.\n",
								args->c_key);
					}

					else {
						printf("DEL operation succeeded.\n");
					}
				}

				free(args);
			}
		}

		else if(strcmp("EXIT", command) == 0) {
			continue;
		}

		else {
			fprintf(stderr, "Invalid command.\n");
			continue;
		}
	}

	close(sockfd);

	printf("Connection closed\n");

	return 0;
}


struct addrinfo* get_addr_list(const char *port, char *host)
{
	int rv;
	struct addrinfo hints, *serv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if((rv = getaddrinfo(host, port, &hints, &serv)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(EXIT_FAILURE);
	}

	return serv;
}


int prep_socket(struct addrinfo *list)
{
	struct addrinfo *p;
	int sock;
	char s[INET6_ADDRSTRLEN];

	for(p = list; p != NULL; p = p->ai_next) {
		if((sock = socket(p->ai_family, p->ai_socktype,
						p->ai_protocol)) == -1) {
			perror("kvclient: socket");
			continue;
		}

		if(connect(sock, p->ai_addr, p->ai_addrlen) == -1) {
			close(sock);
			perror("kvclient: connect");
			continue;
		}

		break;
	}

	if(p != NULL) {
		inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
				s, sizeof s);
		printf("kvclient: connecting to %s\n", s);

		freeaddrinfo(list);

		return sock;
	}

	freeaddrinfo(list);

	return -1;
}


void* get_in_addr(struct sockaddr *sa)
{
	if(sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}


void str_upper(char *c)
{
	if(c == NULL)
		return;

	for (int i = 0; c[i] != '\0'; i++) {
		c[i] = (char) toupper(c[i]);
	}
}


int get_socket_index(char *key)
{
	int sock_index;
	int index = hashkey(key) % ARRAYLEN;

	if(0 <= index && index <= FIRST_PARTITION)
		sock_index = 0;

	else if(FIRST_PARTITION < index && index <= SECOND_PARTITION)
		sock_index = 1;

	else if(SECOND_PARTITION < index && index <= THIRD_PARTITION)
		sock_index = 2;

	else
		sock_index = -1;

	return sock_index;
}
