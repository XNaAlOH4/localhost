#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include <poll.h>

#define PORT 9999

int sockfd = -1, client = -1;

void my_exit() {
	if(sockfd != -1) {
		shutdown(sockfd, SHUT_RDWR);
		close(sockfd);
	}
	if(client != -1) {
		shutdown(client, SHUT_RD);
		close(client);
	}
}

int main(void) {

	atexit(my_exit);

	char buf[1024], msg[400] = {0}, getbuffer[1024];
	char OK[400] = "HTTP/1.1 200 \nContent-Type:text/html\n";
	struct sockaddr_in address = {0};
	socklen_t addrlen = sizeof(address);
	time_t t = time(NULL);

	address.sin_family = AF_INET;
	address.sin_port=htons(PORT);

	FILE* fp = fopen("/home/coolberry/localhost/index.html", "r");

	int len = fread(msg, 1, 400, fp);
	msg[len-1] = 0;

	fclose(fp);

	sprintf(OK+strlen(OK), "Content-Length: %d\nCache-Control: no-store\n", strlen(msg));

	strcpy(buf, OK);

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Socket error\n");
		return 1;
	}
	
	if(bind(sockfd, (struct sockaddr*)&address, addrlen) < 0) {
		printf("Bind error: %d\n", errno);
		return 2;
	}

	struct pollfd fds[] = {
		{
			0,
			POLLIN,
			0
		},
		{
			sockfd,
			POLLIN,
			0
		},
		{
			-1,
			POLLIN,
			0
		}
	};

	if(listen(sockfd, 10) < 0) {
		printf("Listen error\n");
		return 3;
	}

	while(1) {

		poll(fds, 3, -1);

		if(fds[0].revents & POLLIN) {
			read(0, getbuffer, 1024-1);
			if(*getbuffer == 'q')
				break;
		}

		if(fds[1].revents & POLLIN) {
			client = accept(sockfd, (struct sockaddr*)&address, &addrlen);
			fds[2].fd = client;
			char ip[INET6_ADDRSTRLEN];
			printf("Accepting new client\nAddr: %s\n", inet_ntop(address.sin_family, (struct sockaddr*)&address.sin_addr, ip, sizeof(ip)));
		}

		if(fds[2].revents & POLLIN) {
			ssize_t bytes_read = 0;
			while( (bytes_read = read(client, getbuffer, 1024-1)) ) {
				if(getbuffer[bytes_read-1] =='\n') break;
			}

			if(bytes_read == 0) {
				shutdown(client, SHUT_RD);
				close(client);
				client = -1;
				fds[2].fd = -1;
			}

			printf("Client: %s\n", getbuffer);

			struct tm *tm = localtime(&t);

			strftime(buf+strlen(OK), sizeof(buf)-strlen(OK), "Date: %a, %d %b %Y", tm);
			time(&t);
			gmtime(&t);
			strftime(buf+strlen(buf), sizeof(buf)-strlen(buf), " %X GMT\r\n\r\n", tm);

			sprintf(buf+strlen(buf), "%s\r\n\r\n", msg);

			if(write(client, buf, strlen(buf)) != strlen(buf) ) {
				printf("Write error\n");
			}
		}
	}

	return 0;
}
