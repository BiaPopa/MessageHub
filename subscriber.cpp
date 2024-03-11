#include <poll.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <string>
#include <vector>
#include <cmath>
#include "common.h"
#include "helpers.h"

void run_client(int sockfd) {
	char buf[MSG_MAXSIZE];
	memset(buf, 0, MSG_MAXSIZE);

	// pachetul trimis de client
	struct chat_packet sent_packet;
	// mesajul primit de client
	struct message msg;

	struct pollfd fds[32];
	int nfds = 2;
	int rc;

	// se adauga stdin in vectorul de descriptori
	fds[0].fd = STDIN_FILENO;
	fds[0].events = POLLIN;

	// se adauga socketul in vectorul de descriptori
	fds[1].fd = sockfd;
	fds[1].events = POLLIN;

	while (1) {
		rc = poll(fds, nfds, -1);
		DIE(rc < 0, "poll");

		if ((fds[1].revents & POLLIN) != 0) {
			// se primesc date de la server
			rc = recv(sockfd, &msg, sizeof(msg), 0);
			DIE(rc < 0, "recv");
			
			// serverul s-a inchis
			if (rc == 0) {
				break;
			}

			// se afiseaza mesajul primit de la server
			if (msg.type == 0) {
				printf("%s:%hu - %s - INT - %d\n", msg.ip, msg.port, msg.topic, msg.int_payload);
			} else if (msg.type == 1) {
				printf("%s:%hu - %s - SHORT_REAL - %.2f\n", msg.ip, msg.port, msg.topic, msg.short_real_payload);
			} else if (msg.type == 2) {
				printf("%s:%hu - %s - FLOAT - %.*f\n", msg.ip, msg.port, msg.topic, msg.decimals, msg.float_payload);
			} else if (msg.type == 3) {
				printf("%s:%hu - %s - STRING - %s\n", msg.ip, msg.port, msg.topic, msg.string_payload);
			}
		} else if ((fds[0].revents & POLLIN) != 0) {
			// se primesc date de la tastatura
			struct chat_packet sent_packet;
			scanf("%s", buf);
			
			// s-a primit comanda de subscribe
			if (strcmp(buf, "subscribe") == 0) {
				memset(&sent_packet, 0, sizeof(sent_packet));
				strncpy(sent_packet.command, "subscribe", 10);

				// se adauga in pachet topicul si sf-ul
				scanf("%s", sent_packet.topic);
				char sf[2];
				scanf("%s", sf);
				sent_packet.sf = atoi(sf);

				printf("Subscribed to topic.\n");

				// se trimite pachetul
				rc = send_all(sockfd, &sent_packet, sizeof(sent_packet));  
				DIE(rc < 0, "send");  
			} else if (strcmp(buf, "unsubscribe") == 0) {
				// s-a primit comanda de unsubscribe
				strncpy(sent_packet.command, "unsubscribe", 12);

				// se adauga in pachet topicul
				scanf("%s", sent_packet.topic);

				printf("Unsubscribed from topic.\n");

				// se trimite pachetul
				rc = send_all(sockfd, &sent_packet, sizeof(sent_packet));
				DIE(rc < 0, "send");
			} else if (strcmp(buf, "exit") == 0) {
				// se inchide conexiunea
				break;
			}
		}
	}
}

int main(int argc, char *argv[]) {
	int sockfd = -1;
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	// verificarea numarului de argumente
	if (argc != 4) {
		printf("\n Usage: %s <id> <ip> <port>\n", argv[0]);
		return 1;
	}
	
	char *id = argv[1];

	// parsarea portului
	uint16_t port;
	int rc = sscanf(argv[3], "%hu", &port);
	DIE(rc != 1, "Given port is invalid");

	// se obtine un socket TCP pentru conectarea la server
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	// se dezactiveaza algoritmul lui Nagle
	int aux = 1;
	rc = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&aux, sizeof(int));
	DIE(rc < 0, "disable nagle");

	// se completeaza in serv_addr adresa serverului, familia de adrese si portul
	// pentru conectare
	struct sockaddr_in serv_addr;
	socklen_t socket_len = sizeof(struct sockaddr_in);

	memset(&serv_addr, 0, socket_len);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
	DIE(rc <= 0, "inet_pton");

	// se realizeaza conectarea la server
	rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	DIE(rc < 0, "connect");

	// se trimite id-ul la server
	rc = send(sockfd, id, strlen(id) + 1, 0);
	DIE(rc < 0, "send id");

	run_client(sockfd);

	// se inchide socketul creat
	close(sockfd);

	return 0;
}
