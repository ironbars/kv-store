#include <time.h>

#include "kvclient.h"

int prep_kv_args(char *cmd_str, kv_args *args);
int kv_transaction(kv_args *args, int *sock_list);
kv_status get_kv_status(char *op);

int cache_hit = 0;
int cache_miss = 0;

int main(int argc, char **argv)
{
	float hit_ratio;
	float throughput;
	/*const short NUM_HOSTS = 3;*/
	/*int sockets[NUM_HOSTS], i;*/
	struct addrinfo *servinfo;
	/*char *hosts[3] = {"pi-host1", "pi-host2", "pi-host3"};*/
	int sockfd; /* temporary */
	char operation[64];
	FILE *sim_file;
	clock_t start, end;
	kv_args *args;

	servinfo = get_addr_list(PORT, "localhost");

	if((sockfd = prep_socket(servinfo)) == -1) {
		fprintf(stderr, "kvsim: failed to connect\n");
		exit(EXIT_FAILURE);
	}

	/*for(i = 0; i < NUM_HOSTS; i++) {*/
		/*servinfo = get_addr_list(PORT, hosts[i]);*/

		/*if((sockets[i] = prep_socket(servinfo)) == -1) {*/
			/*fprintf(stderr, "kvsim: failed to connect\n");*/
			/*exit(EXIT_FAILURE);*/
		/*}*/
	/*}*/

	if((sim_file = fopen("simulation.txt", "r")) == NULL) {
		fprintf(stderr, "Error opening simulation file");
		exit(EXIT_FAILURE);
	}

	printf("Starting simulation...\n");

	start = clock();

	while(fgets(operation, 64, sim_file) != NULL) {
		args = (kv_args*) malloc(sizeof(kv_args));

		if(prep_kv_args(operation, args) == -1)
			continue;

		if(kv_transaction(args, &sockfd) == -1)
		/*if(kv_transaction(args, sockets) == -1)*/
			continue;

		free(args);
	}

	if(!feof(sim_file)) {
		fprintf(stderr, "End of file not reached.\n");
		exit(EXIT_FAILURE);
	}

	end = clock();

	/*for(i = 0; i < NUM_HOSTS; i++) {*/
		/*close(sockets[i]);*/
	/*}*/

	close(sockfd);

	printf("Connection closed.\n");

	hit_ratio = (cache_hit * 100) / (cache_hit + cache_miss);
	throughput = ((float) (end - start)) / CLOCKS_PER_SEC;

	printf("\nStatistics\n----------\n");
	printf("Execution time: %.2f s\n", throughput);
	printf("Hit ratio: %.2f%%\n\n", hit_ratio);

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
			perror("kvsim: socket");
			continue;
		}

		if(connect(sock, p->ai_addr, p->ai_addrlen) == -1) {
			close(sock);
			perror("kvsim: connect");
			continue;
		}

		break;
	}

	if(p != NULL) {
		inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
				s, sizeof s);
		printf("kvsim: connecting to %s\n", s);

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


int prep_kv_args(char *cmd_str, kv_args *args)
{
	char *p_operation = cmd_str;
	char *next_token = NULL;
	char *command = kv_strtok(p_operation, " \n", &next_token);
	char *key = kv_strtok(NULL, " \n", &next_token);

	args->status = get_kv_status(command);
	args->c_key = key;

	if(args->status == KV_NOK) {
		return -1;
	}

	else if(args->status == KV_PUT) {
		char *value = kv_strtok(NULL, " \n", &next_token);
		args->i_value = strtol(value, NULL, 0);
	}

	else {
		args->i_value = 0;
	}

	return 0;
}

kv_status get_kv_status(char *op)
{
	if(strcmp("GET", op) == 0)
		return KV_GET;

	else if(strcmp("PUT", op) == 0)
		return KV_PUT;

	else if(strcmp("DEL", op) == 0)
		return KV_DEL;

	else
		return KV_NOK;
}

int kv_transaction(kv_args *args, int *sock_list)
{
	/*int index = get_socket_index(args->c_key);*/
	/*int sockfd = sock_list[index];*/
	int sockfd = *sock_list;
	kv_status op_type = args->status;
	unsigned char send_buf[BUF_SIZE], recv_buf[BUF_SIZE], *ptr;

	ptr = serialize_kv_message(send_buf, args);

	if(send(sockfd, send_buf, ptr - send_buf, 0) == -1) {
		perror("send");
		return -1;
	}

	if(recv(sockfd, recv_buf, BUF_SIZE, 0) == -1) {
		perror("recv");
		return -1;
	}

	else {
		deserialize_kv_message(recv_buf, args);

		if(op_type == KV_GET) {
			if(args->status == KV_OK)
				cache_hit++;

			else
				cache_miss++;
		}
	}

	return 0;
}
