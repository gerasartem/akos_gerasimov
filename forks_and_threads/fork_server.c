#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>


int main() {
	int s, listener, client_id = 0;
	struct sockaddr_in addr;

	listener = socket(AF_INET, SOCK_STREAM, 0);
	if (listener < 0) {
		perror("socket");
		exit(1);
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(2048);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("bind");
		exit(1);
	}

	if (listen(listener, 1) == -1) {
		perror("listen");
		exit(1);
	}

	while (1) {
		s = accept(listener, NULL, NULL);
		if (s < 0) {
			perror("accept");
			exit(1);
		}

		int a = fork();

		if (a == -1) {
			perror("fork");
			if (close(s)) {
				perror("close");
			}
			exit(1);
		}

		if (a == 0) {
			char filename[128];
			sprintf(filename, "client_%d", client_id);
			FILE* fp = fopen(filename, "w");
			while (1) {
				char temp;
				int bytes_read = recv(s, &temp, sizeof(char), 0);
				if (bytes_read <= 0) {
					break;
				}
				fputc(temp, fp);
			}

			if (close(listener)) {
				perror("close");
			}
			if (close(s)) {
				perror("close");
			}

			return 0;
		}
		else
		{
			++client_id;
			if (close(s)) {
				perror("close");
			}
		}
	}

	if (close(listener)) {
		perror("close");
	}
	return 0;
}