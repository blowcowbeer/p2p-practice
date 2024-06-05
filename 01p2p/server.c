#include "header.h"

int buffer_parser(int sockfd, U8 *buffer, U32 length, struct sockaddr_in *addr) {
	U8 status = buffer[PROTO_BUFFER_STATUS_IDX];
	printf("buffer_parser --> %x\n", status);
	switch (status) {
		case PROTO_LOGIN_REQ: {
#if 1
			int old = client_count;
			int now = old+1;
			if(0 == cmpxchg((UATOMIC*)&client_count, old, now)) {
				printf("client_count --> %d, old:%d, now:%d\n", client_count, old, now);
				return RESULT_FAILED;
			}
#else
			client_count = client_count+1;
			int now = client_count;
#endif
			U8 array[CLIENT_ADDR_LENGTH] = {0};
			addr_to_array(array, addr);
			printf("login --> %d.%d.%d.%d:%d\n",
				*(unsigned char*)(&addr->sin_addr.s_addr),
				*((unsigned char*)(&addr->sin_addr.s_addr)+1),
				*((unsigned char*)(&addr->sin_addr.s_addr)+2),
				*((unsigned char*)(&addr->sin_addr.s_addr)+3),
				addr->sin_port);
			table[now].client_id =  *(U32*)(buffer+PROTO_LOGIN_SELFID_IDX);
			memcpy(table[now].addr, array, CLIENT_ADDR_LENGTH);
			break;
		}
		case PROTO_HEARTBEAT_REQ: {
			int client_id = *(unsigned int*)(buffer+PROTO_HEARTBEAT_SELFID_IDX);
			int index = get_index_by_clientid(client_id);
			table[index].stamp = time_genrator();
			break;
		}
		case PROTO_CONNECT_REQ: {
			int client_id = *(unsigned int*)(buffer+PROTO_CONNECT_SELFID_IDX);
			int other_id = *(unsigned int*)(buffer+PROTO_CONNECT_OTHERID_IDX);
			send_notify(sockfd, other_id, client_id);
			break;
		}
		case NTY_RPORO_MESSAGE_REQ: {
			U8 *msg = buffer+RPORO_MESSAGE_CONTENT_IDX;
			int client_id = *(unsigned int*)(buffer+RPORO_MESSAGE_SELFID_IDX);
			int other_id = *(unsigned int*)(buffer+PROTO_MESSAGE_OTHERID_IDX);
			printf(" from client:%d --> %s\n", client_id, msg);
#if 0
			send_message(sockfd, other_id, buffer, length);
#endif
			break;
		}
	}

	return RESULT_SUCCESS;
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("Usage: <Server> <Port>\n");
		return -1;
	}
	printf(" This is a UDP Server\n");
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("socket");
		exit(0);
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(argv[1]));
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("bind");
		exit(1);
	}

	char buffer[BUFFER_LENGTH] = {0};
	struct sockaddr_in c_addr;
	int n;
	int length = sizeof(struct sockaddr_in);
	while(1) {
		n = recvfrom(sockfd, buffer, BUFFER_LENGTH, 0, (struct sockaddr*)&c_addr, &length);
		if (n > 0) {
			buffer[n] = 0x0;
			printf("%d.%d.%d.%d:%d say: %s\n", *(unsigned char*)(&c_addr.sin_addr.s_addr), *((unsigned char*)(&c_addr.sin_addr.s_addr)+1),
				*((unsigned char*)(&c_addr.sin_addr.s_addr)+2), *((unsigned char*)(&c_addr.sin_addr.s_addr)+3),
				c_addr.sin_port, buffer);
			int ret = buffer_parser(sockfd, buffer, n, &c_addr);
			if (ret == RESULT_FAILED) continue;
			buffer[PROTO_BUFFER_STATUS_IDX] += 0x80;
			n = sendto(sockfd, buffer, n, 0, (struct sockaddr*)&c_addr, sizeof(c_addr));
			if (n < 0) {
				perror("sendto");
				break;
			}
		} else if (n == 0) {
			printf("server closed\n");
		} else {
			perror("recv");
			break;
		}
	}

	return 0;
}
