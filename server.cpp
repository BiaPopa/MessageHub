#include <stdint.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
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

#define MAX_CONNECTIONS 1<<16

using namespace std;

void run_chat_multi_server(int listenfd, int udpfd) {
	struct pollfd poll_fds[MAX_CONNECTIONS];
	// vector ce contine id-urile clientilor conectati
  	vector <string> clientId;
	// vector ce contine informatiile clientilor conectati
  	vector <client> clients;
	// vector ce contine informatiile clientilor deconectati
	vector <client> clients_disconnected;
  	int num_clients = 3;
  	int rc;

  	struct chat_packet received_packet;

  	// setam socket-ul listenfd pentru ascultare
  	rc = listen(listenfd, MAX_CONNECTIONS);
  	DIE(rc < 0, "listen");

  	// se adauga stdin in vectorul de descriptori
  	poll_fds[0].fd = STDIN_FILENO;
  	poll_fds[0].events = POLLIN;

	// se adauga socketul listenfd in vectorul de descriptori
  	poll_fds[1].fd = listenfd;
  	poll_fds[1].events = POLLIN;

	// se adauga socketul udpfd in vectorul de descriptori
  	poll_fds[2].fd = udpfd;
  	poll_fds[2].events = POLLIN;

  	while (1) {
		rc = poll(poll_fds, num_clients, -1);
		DIE(rc < 0, "poll");

		for (int i = 0; i < num_clients; i++) {
	  		if (poll_fds[i].revents & POLLIN) {
				if (poll_fds[i].fd == listenfd) {
		  			// s-au primit date pe socketul TCP
		  			struct sockaddr_in cli_addr;
		  			socklen_t cli_len = sizeof(cli_addr);
					int already_connected = 0;
		  			int newsockfd = accept(listenfd, (struct sockaddr *)&cli_addr, &cli_len);
		  			DIE(newsockfd < 0, "accept");

		  			char id[10];
		  			rc = recv(newsockfd, id, sizeof(id), 0);
					DIE(rc < 0, "recv");

					// se verifica daca clientul este deja conectat
					for (int j = 0; j < num_clients - 3; j++) {
			  			if (strcmp(id, clientId[j].c_str()) == 0) {
							printf("Client %s already connected.\n", id);
							close(newsockfd);
							already_connected = 1;
			  			}
					}

					if (already_connected == 0) {
						int client_disconnected = 0;

						// se verifica daca clientul a mai fost conectat
						for (int j = 0; j < clients_disconnected.size(); j++) {
							if (strcmp(id, clients_disconnected[j].id.c_str()) == 0) {
								// se adauga noul socket la vectorul de descriptori si
								// id-ul clientului la vectorul clientilor conectati
								poll_fds[num_clients].fd = newsockfd;
								poll_fds[num_clients].events = POLLIN;
								clientId.push_back(id);

								// se iau informatiile clientului din vectorul de clienti deconectati
								struct client new_client;
								new_client = clients_disconnected[j];
								clients.push_back(new_client);

								// se sterge clientul din vectorul de clienti deconectati
								for (int k = j; k < clients_disconnected.size() - 1; k++) {
									clients_disconnected[k] = clients_disconnected[k + 1];
								}
								clients_disconnected.pop_back();

								// se trimit mesajele primite cat timp a fost deconectat
								for (int k = 0; k < new_client.messages.size(); k++) {
									struct message msg = new_client.messages[k];
									int rc = send(poll_fds[num_clients].fd, &msg, sizeof(msg), 0);
									DIE(rc < 0, "send");
								}

								client_disconnected = 1;
								num_clients++;

								printf("New client %s connected from %s:%d.\n", id, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
								break;
							}
						}

						// clientul nu a mai fost conectat
						if (client_disconnected == 0) {
							// se adauga noul socket la vectorul de descriptori si
							// id-ul clientului la vectorul clientilor conectati
		  					poll_fds[num_clients].fd = newsockfd;
		  					poll_fds[num_clients].events = POLLIN;
		  					clientId.push_back(id);

							// se creeaza o structura de tip client pentru noul client,
							// ce va retine informatii despre acesta
		  					struct client new_client;
							new_client.id = clientId[num_clients - 3];
							clients.push_back(new_client);
		  					num_clients++;

		  					printf("New client %s connected from %s:%d.\n", id, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
							break;
						} else {
							break;
						}
					}
				} else if (poll_fds[i].fd == STDIN_FILENO) {
		  			// s-au primit date pe socketul de la tastatura
		  			char buffer[7];
		  			memset(buffer, 0, 7);
		  			fgets(buffer, 6, stdin);

		  			if (strncmp(buffer, "exit", 4) == 0) {
						// se inchid toate conexiunile
						for (int j = 3; j < num_clients; j++) {
			  				close(poll_fds[j].fd);
						}
						
						return;
		  			}								
				} else if (poll_fds[i].fd == udpfd) {
					// s-au primit date pe socketul UDP
					struct sockaddr_in serv_addr;
  					socklen_t socket_len = sizeof(struct sockaddr_in);
					char buf[MSG_MAXSIZE] = {0};

					int rc = recvfrom(udpfd, &buf, sizeof(buf), 0,	(struct sockaddr *)&serv_addr, &socket_len);
					DIE(rc < 0, "recvfrom");

					// parsarea mesajului primit
					struct message msg;
					strcpy(msg.ip, inet_ntoa(serv_addr.sin_addr));
					msg.port = ntohs(serv_addr.sin_port);

					strncpy(msg.topic, buf, 50);
					msg.topic[50] = '\0';

					msg.type = buf[50];

					if (msg.type == 0) {
						uint8_t sign = buf[51];
						uint32_t value = ntohl(*(uint32_t *)(buf + 52));

						if (sign == 0) {
							msg.int_payload = value;
						} else {
						msg.int_payload = -value;
						}
					} else if (msg.type == 1) {
						uint16_t value = ntohs(*(uint16_t *)(buf + 51));
						msg.short_real_payload = value / 100.0;
					} else if (msg.type == 2) {
						uint8_t sign = buf[51];
						uint32_t value = ntohl(*(uint32_t *)(buf + 52));
						uint8_t power = buf[56];

						if (sign == 0) {
							msg.float_payload = value / pow(10, power);
						} else {
							float aux = value / pow(10, power);
							msg.float_payload = -aux;
						}
						msg.decimals = power;
					} else if (msg.type == 3) {
						strncpy(msg.string_payload, buf + 51, 1500);
						msg.string_payload[1500] = '\0';
					}

					// se trimite mesajul catre toti clientii conectati si abonati la topic 
					for (int k = 3; k < num_clients; k++) {
						for (int j = 0; j < clients[k - 3].topics.size(); j++) {
							if (clients[k - 3].topics[j] == msg.topic && clients[k - 3].blocked[j] == 0) {
								int rc = send(poll_fds[k].fd, &msg, sizeof(msg), 0);
								DIE(rc < 0, "send");
							}
						}
					}

					// se salveaza mesajul pentru toti clientii deconectati si abonati la topic cu sf-ul 1
					for (int k = 0; k < clients_disconnected.size(); k++) {
						for (int j = 0; j < clients_disconnected[k].topics.size(); j++) {
							if (clients_disconnected[k].topics[j] == msg.topic
								&& clients_disconnected[k].blocked[j] == 0 && clients_disconnected[k].sf[j] == 1) {
								clients_disconnected[k].messages.push_back(msg);
							}
						}
					}
				} else {
		  			// s-au primit date pe unul din socketii de client
					memset(&received_packet, 0, sizeof(received_packet));
		  			int rc = recv_all(poll_fds[i].fd, &received_packet, sizeof(received_packet));
		  			DIE(rc < 0, "recv");

		  			if (rc == 0) {
						// conexiunea s-a inchis
						printf("Client %s disconnected.\n", clientId[i - 3].c_str());
						close(poll_fds[i].fd);

						// se salveaza drept client deconectat
						clients_disconnected.push_back(clients[i-3]);

						// se elimina informatiile despre client din vectorul de descriptori,
						// vectorul cu id-uri si vectorul de clienti
						for (int j = i; j < num_clients - 1; j++) { 
				  			poll_fds[j] = poll_fds[j + 1];
				  			clientId[j - 3] = clientId[j - 2];
							clients[j - 3] = clients[j - 2];
						}
						clientId.pop_back();
						clients.pop_back();

						num_clients--;
					} else {
						// s-a primit comanda de subscribe
						if (strncmp(received_packet.command, "subscribe", 9) == 0) {
							int already_subscribed = 0;

							// se verifica daca clientul este deja abonat la topic
							for (int j = 0; j < clients[i - 3].topics.size(); j++) {
								if (clients[i - 3].topics[j] == received_packet.topic) {
									// se actualizeaza datele
									clients[i - 3].sf[j] = received_packet.sf;
									clients[i - 3].blocked[j] = 0;

									already_subscribed = 1;
									break;
								}
							}

							// nu este abonat 
							if (already_subscribed == 0) {
								// se adauga datele in vectorii corespunzatori
								clients[i - 3].topics.push_back(received_packet.topic);
								clients[i - 3].sf.push_back(received_packet.sf);
								clients[i - 3].blocked.push_back(0);
							}
						} else if (strncmp(received_packet.command, "unsubscribe", 11) == 0) {
							// s-a primit comanda de unsubscribe
							for (int j = 0; j < clients[i - 3].topics.size(); j++) {
								if (clients[i - 3].topics[j] == received_packet.topic) {
									// se actualizeaza datele
									clients[i - 3].blocked[j] = 1;
									break;
								}
							}
						}
		  			}
				}
	  		}
		}
  	}
}

int main(int argc, char *argv[]) {
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	// verificarea numarului de argumente
  	if (argc != 2) {
		printf("\n Usage: %s <port>\n", argv[0]);
		return 1;
  	}

  	// parsarea portului
  	uint16_t port;
  	int rc = sscanf(argv[1], "%hu", &port);
  	DIE(rc != 1, "Given port is invalid");

  	// se obtine un socket TCP pentru receptionarea conexiunilor
  	int listenfd = socket(AF_INET, SOCK_STREAM, 0);
  	DIE(listenfd < 0, "socket");

	// se dezactiveaza algoritmul lui Nagle
	int aux = 1;
  	rc = setsockopt(listenfd, IPPROTO_TCP, TCP_NODELAY, (char *)&aux, sizeof(int));
  	DIE(rc < 0, "disable nagle");

  	struct sockaddr_in serv_addr;
  	socklen_t socket_len = sizeof(struct sockaddr_in);

  	// se face adresa socket-ului reutilizabila, ca sa nu primim eroare in caz ca
  	// rulam de 2 ori rapid
  	int enable = 1;
  	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");

	// se completeaza in serv_addr adresa serverului, familia de adrese si portul
  	// pentru conectarea clientilor TCP
  	memset(&serv_addr, 0, socket_len);
  	serv_addr.sin_family = AF_INET;
  	serv_addr.sin_port = htons(port);
  	rc = inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr.s_addr);
  	DIE(rc <= 0, "inet_pton");

  	// se asociaza adresa serverului cu socketul pentru TCP folosind bind
  	rc = bind(listenfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
  	DIE(rc < 0, "bind");

	// se obtine un socket UDP pentru receptionarea conexiunilor
  	int udpfd = socket(AF_INET, SOCK_DGRAM, 0);
  	DIE(listenfd < 0, "socket");

  	// se face adresa socket-ului reutilizabila, ca sa nu primim eroare in caz ca
  	// rulam de 2 ori rapid
  	enable = 1;
  	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");

	// se completeaza in serv_addr adresa serverului, familia de adrese si portul
  	// pentru conectarea clientilor UDP
  	memset(&serv_addr, 0, socket_len);
  	serv_addr.sin_family = AF_INET;
  	serv_addr.sin_port = htons(port);
  	rc = inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr.s_addr);
  	DIE(rc <= 0, "inet_pton");

  	// se asociaza adresa serverului cu socketul pentru UDP folosind bind
  	rc = bind(udpfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
  	DIE(rc < 0, "bind");

  	run_chat_multi_server(listenfd, udpfd);

  	// se inchid socketii TCP si UDP
  	close(listenfd);
	close(udpfd);
  	return 0;
}
