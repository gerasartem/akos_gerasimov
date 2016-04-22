#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdlib.h>

struct connection {
	int sock;
	int client_id;
};

void* thread_func(struct connection *connect) {
	char filename[128];
	sprintf(filename, "client_%d", connect->client_id);
	FILE* f = fopen(filename, "w");
	while (1) {
		char temp;
		int bytes_read = recv(connect->sock, &temp, sizeof(char), 0);
		if (bytes_read <= 0)
			break;
		fputc(temp, f);
	}
	fclose(f);
	close(connect->sock);
	free(connect);
}

int main() {
	int listener;
	struct sockaddr_in addr;

	listener = socket(AF_INET, SOCK_STREAM, 0);
	if (listener < 0) {
		perror("socket");
		exit(1);
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(2048);
	addr.sin_addr.s_addr = htons(INADDR_ANY);

	if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		exit(1);
	}

	if (listen(listener, 1) == -1) {
		perror("listen");
		exit(1);
	}

	int client_id = 0;

	while (1) {
		struct connection* connect = (struct connection*)malloc(sizeof(struct connection));
		connect->sock = accept(listener, NULL, NULL);
		connect->client_id = client_id;
		if (connect->sock < 0) {
			perror("accept");
			exit(1);
		}

		pthread_t thread;
		thread = pthread_create(&thread, NULL, (void*)thread_func, connect);
		++client_id;
	}

	return 0;
}