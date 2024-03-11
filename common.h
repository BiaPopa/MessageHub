#ifndef __COMMON_H__
#define __COMMON_H__

#include <stddef.h>
#include <stdint.h>
#include <vector>
#include <string>

using namespace std;

int send_all(int sockfd, void *buff, size_t len);
int recv_all(int sockfd, void *buff, size_t len);

#define MSG_MAXSIZE 1601

struct chat_packet {
  	char command[12];
  	char topic[51];
  	int sf;
};

struct message {
	char ip[16];
  	uint16_t port;
  	char topic[51];
  	char type;

  	char string_payload[1501];
  	int int_payload;
	float float_payload;
	int decimals;
	double short_real_payload;
};

struct client {
  	string id;
  	vector<string> topics;
  	vector<int> sf;
  	vector<int> blocked;
	vector<message> messages;
};

#endif
