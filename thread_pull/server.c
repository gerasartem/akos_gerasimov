#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <signal.h>

#include <list>

#define THREAD_NUM 20

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
std::list<int> queue;
int client_id = 0;
bool flag = 1, breaker = 0;

void termination_handler (int signum) {
	printf("Now you can see that signals really work.\n");
	flag = 0;
	return;
}

void* entry (void *) {
	while (!breaker) {
		pthread_mutex_lock(&mutex);
		while (!queue.size()) {
			assert(!pthread_cond_wait(&cond, &mutex));
			if (breaker) {
				pthread_mutex_unlock(&mutex);
				break;
			}
		}
		if (!breaker) {
			int sock = queue.front();
			queue.pop_front();
			char filename[128];
			sprintf(filename, "client_%d", client_id++);
			FILE* f = fopen(filename, "w");
			pthread_mutex_unlock(&mutex);

			if (!sock)
			{
				break;
			}

			while (true) {
				char temp;
				int bytes_read = recv(sock, &temp, sizeof(char), 0);
				if (bytes_read <= 0)
					break;
				fputc(temp, f);
			}
			fclose(f);
			close(sock);
		}
	}
	return NULL;
}

int main() {
	struct sigaction new_action, old_action;
	new_action.sa_handler = termination_handler;
	sigemptyset(&new_action.sa_mask);
	sigaddset(&new_action.sa_mask, SIGINT);
	new_action.sa_flags = 0;
	if (sigaction(SIGINT, &new_action, &old_action) < 0) {
		perror("sigaction");
		exit(1);
	}

	int listener;
	struct sockaddr_in addr;
	pthread_t* pool = new pthread_t[THREAD_NUM];

	listener = socket(AF_INET, SOCK_STREAM, 0);
	if (listener < 0) {
		perror("socket");
		exit(1);
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(2048);
	addr.sin_addr.s_addr = htons(INADDR_ANY);

	if (bind(listener, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		perror("bind");
		exit(1);
	}

	if (listen(listener, 1) == -1) {
		perror("listen");
		exit(1);
	}

	for (int i = 0; i < THREAD_NUM; ++i) {
		pthread_create(pool + i, NULL, entry, NULL);
	}

	while (flag) {
		int sock = accept(listener, NULL, NULL);
		if (sock < 0) {
			perror("accept");
			continue;
		}
		pthread_mutex_lock(&mutex);
		queue.push_back(sock);
		pthread_mutex_unlock(&mutex);
		pthread_cond_signal(&cond);
	}
/*
	pthread_mutex_lock(&mutex);
	while (queue.size()) {
		sleep(1);
	}
	pthread_mutex_unlock(&mutex);
*/
	breaker = 1;
	close(listener);
	pthread_cond_broadcast(&cond);

	for (int i = 0; i < THREAD_NUM; ++i) {
		if (pthread_join(pool[i], NULL) != 0) {
			perror("pthread_join");
			exit(1);
		}
	}

	delete[] pool;
	return 0;
}