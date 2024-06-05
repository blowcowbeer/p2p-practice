#ifndef HEADER_H
#define HEADER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <arpa/inet.h>

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef volatile long uautomic;
typedef void*(*CALLBACK)(void* arg);

typedef enum {
    RESULT_FAILED = -1,
    RESULT_SUCCESS = 0,
} RESULT;

typedef enum {
    STATUS_NULL,
    STATUS_LOGIN,
    STATUS_HERATBEAT,
    STAUTS_CONNECT,
    STATUS_MESSAGE,
    STATUS_NOTIFY,
    STATUS_P2P_CONNECT,
    STATUS_P2P_MESSAGE,
}STATUS_SET;

#define CLIENT_MAX 1024
#define CLIENT_ADDR_LENGTH 6
#define BUFFER_LENGTH 512
#define NUMBER_ID_LENGTH 4

typedef struct ClientTable {
    u8 addr[CLIENT_ADDR_LENGTH];
    u32 client_id;
    long stamp;
}ClientTable;

/**************************** status define ****************************/
#define PROTO_LOGIN_REQ				0x01
#define PROTO_LOGIN_ACK				0x81
#define PROTO_HEARTBEAT_REQ			0x02
#define PROTO_HEARTBEAT_ACK			0x82
#define PROTO_CONNECT_REQ			0x11
#define PROTO_CONNECT_ACK			0x91
#define NTY_PROTO_NOTIFY_REQ		0x12
#define NTY_PROTO_NOTIFY_ACK		0x92
#define NTY_PROTO_P2P_CONNECT_REQ	0x13
#define NTY_PROTO_P2P_CONNECT_ACK	0x93
#define NTY_RPOTO_MESSAGE_REQ		0x21
#define NTY_RPOTO_MESSAGE_ACK		0xA1

/**************************** context define ****************************/

#define PROTO_BUFFER_VERSION_IDX          0
#define PROTO_BUFFER_STATUS_IDX		      1
#define PROTO_BUFFER_LENGTH_IDX		      (PROTO_BUFFER_STATUS_IDX+1) // 2
#define PROTO_BUFFER_SELFID_IDX		      (PROTO_BUFFER_LENGTH_IDX+2) // 3
//login
#define PROTO_LOGIN_SELFID_IDX			  PROTO_BUFFER_SELFID_IDX // 3
//heartbeat
#define PROTO_HEARTBEAT_SELFID_IDX		  PROTO_BUFFER_SELFID_IDX // 3
//connect
#define PROTO_CONNECT_SELFID_IDX		  PROTO_BUFFER_SELFID_IDX // 3
#define PROTO_CONNECT_OTHERID_IDX		  (PROTO_BUFFER_SELFID_IDX+NUMBER_ID_LENGTH) // 7
//notify
#define PROTO_NOTIFY_SELFID_IDX			  PROTO_BUFFER_SELFID_IDX
#define PROTO_NOTIFY_ADDR_IDX			  (PROTO_BUFFER_SELFID_IDX+NUMBER_ID_LENGTH) // 7
//p2p connect
#define PROTO_P2P_CONNECT_SELFID_IDX	  PROTO_BUFFER_SELFID_IDX // 3
//p2p connect ack
#define PROTO_P2P_CONNECT_ACK_SELFID_IDX  PROTO_BUFFER_SELFID_IDX // 3
//message
#define RPORO_MESSAGE_SELFID_IDX		  PROTO_BUFFER_SELFID_IDX // 3
#define PROTO_MESSAGE_OTHERID_IDX		  (RPORO_MESSAGE_SELFID_IDX+NUMBER_ID_LENGTH) // 7
#define RPORO_MESSAGE_CONTENT_IDX		  (PROTO_MESSAGE_OTHERID_IDX+NUMBER_ID_LENGTH) // 11
//message ack
#define RPORO_MESSAGE_ACK_SELFID_IDX	  PROTO_BUFFER_SELFID_IDX // 3


static unsigned long cmpxchg(uautomic *addr, unsigned long _old, unsigned long _new) {
    u8 res;

    __asm__ volatile (
        "lock; cmpxchg %3, %1;sete %0;"
        : "=a" (res)
        : "m" (*addr), "a" (_old), "r" (_new)
        : "cc", "memory");

    return res;
}

static long time_genrator(void) {
    static long lTimeStamp = 0;
    static long timeStampMutex = 0;

    if(cmpxchg(&timeStampMutex, 0, 1)) {
        lTimeStamp = time(NULL);
        timeStampMutex = 0;
    }

    return lTimeStamp;
}

static int addr_to_array(u8* array, struct sockaddr_in* paddr) {
    int i;
    for (i = 0; i < 4; ++i) {
        array[i] = *((unsigned char*)(&paddr->sin_addr.s_addr)+i);
    }
    for (i = 0; i < 2; ++i) {
        array[4+i] = *((unsigned char*)(&paddr->sin_port)+i);
    }

    return RESULT_SUCCESS;
}

static int array_to_addr(u8* array, struct sockaddr_in* paddr) {
    int i;
    for (i = 0; i < 4; i ++) {
        *((unsigned char*)(&paddr->sin_addr.s_addr) + i) = array[i];
    }
    for (i = 0; i < 2; i ++) {
        *((unsigned char*)(&paddr->sin_port)+i) = array[4+i];
    }

    return RESULT_SUCCESS;
}


static int send_login(int sockfd, int self_id, struct sockaddr_in* paddr) {
    u8 buf[BUFFER_LENGTH] = {0};
    buf[PROTO_BUFFER_STATUS_IDX] = PROTO_LOGIN_REQ;
    *(int*)(buf + PROTO_LOGIN_SELFID_IDX) = self_id;

    int n = PROTO_LOGIN_SELFID_IDX + NUMBER_ID_LENGTH;
    n = sendto(sockfd, buf, n, 0, (struct sockaddr*)paddr, sizeof(struct sockaddr_in));
    if (n < 0) {
        perror("sendto");
        return RESULT_FAILED;
    }

    return RESULT_SUCCESS;
}


static int send_heartbeat(int sockfd, int self_id, struct sockaddr_in* paddr) {
    u8 buf[BUFFER_LENGTH] = {0};
    buf[PROTO_BUFFER_STATUS_IDX] = PROTO_HEARTBEAT_REQ;
    *(int*)(buf + PROTO_HEARTBEAT_SELFID_IDX) = self_id;
    int n = PROTO_HEARTBEAT_SELFID_IDX + NUMBER_ID_LENGTH;
    n = sendto(sockfd, buf, n, 0, (struct sockaddr*)paddr, sizeof(struct sockaddr_in));
    if (n < 0) {
        perror("sendto");
        return RESULT_FAILED;
    }
    return RESULT_SUCCESS;
}

static int send_connect(int sockfd, int self_id, int other_id, struct sockaddr_in *paddr) {
	u8 buffer[BUFFER_LENGTH] = {0};
	buffer[PROTO_BUFFER_STATUS_IDX] = PROTO_CONNECT_REQ;
	*(int *)(buffer+PROTO_CONNECT_SELFID_IDX) = self_id;
	*(int *)(buffer+PROTO_CONNECT_OTHERID_IDX) = other_id;

	int n = PROTO_CONNECT_OTHERID_IDX + NUMBER_ID_LENGTH;
	n = sendto(sockfd, buffer, n, 0, (struct sockaddr*)paddr, sizeof(struct sockaddr_in));
	if (n < 0) {
		perror("sendto");
	}

	return n;
}

static int send_p2pconnect(int sockfd, int self_id, struct sockaddr_in *paddr) {
	u8 buffer[BUFFER_LENGTH] = {0};
	buffer[PROTO_BUFFER_STATUS_IDX] = NTY_PROTO_P2P_CONNECT_REQ;
	*(int *)(buffer+PROTO_P2P_CONNECT_SELFID_IDX) = self_id;

	int n = PROTO_P2P_CONNECT_SELFID_IDX + NUMBER_ID_LENGTH;
	n = sendto(sockfd, buffer, n, 0, (struct sockaddr*)paddr, sizeof(struct sockaddr_in));
	if (n < 0) {
		perror("sendto");
	}

	return n;
}

static int send_p2pconnectack(int sockfd, int self_id, struct sockaddr_in *paddr) {
	u8 buffer[BUFFER_LENGTH] = {0};
	buffer[PROTO_BUFFER_STATUS_IDX] = NTY_PROTO_P2P_CONNECT_ACK;
	*(int*)(buffer+PROTO_P2P_CONNECT_ACK_SELFID_IDX) = self_id;

	int n = PROTO_P2P_CONNECT_ACK_SELFID_IDX + NUMBER_ID_LENGTH;
	n = sendto(sockfd, buffer, n, 0, (struct sockaddr*)paddr, sizeof(struct sockaddr_in));
	if (n < 0) {
		perror("sendto");
	}

	return n;
}


static int client_send_message(int sockfd, int self_id, int other_id, struct sockaddr_in *paddr, u8 *msg, int length) {
	u8 buffer[BUFFER_LENGTH] = {0};

	buffer[PROTO_BUFFER_STATUS_IDX] = NTY_RPOTO_MESSAGE_REQ;
	*(int*)(buffer+RPORO_MESSAGE_SELFID_IDX) = self_id;
	*(int*)(buffer+PROTO_MESSAGE_OTHERID_IDX) = other_id;

	memcpy(buffer+RPORO_MESSAGE_CONTENT_IDX, msg, length);

	int n = RPORO_MESSAGE_CONTENT_IDX + length;
	*(u16*)(buffer+PROTO_BUFFER_LENGTH_IDX) = (u16)n;

	n = sendto(sockfd, buffer, n, 0, (struct sockaddr*)paddr, sizeof(struct sockaddr_in));
	if (n < 0) {
		perror("sendto");
	}
	return n;
}

static int send_messageack(int sockfd, int self_id, struct sockaddr_in *paddr) {
	u8 buffer[BUFFER_LENGTH] = {0};

	buffer[PROTO_BUFFER_STATUS_IDX] = NTY_RPOTO_MESSAGE_ACK;
	*(int*)(buffer+RPORO_MESSAGE_ACK_SELFID_IDX) = self_id;

	int n = RPORO_MESSAGE_ACK_SELFID_IDX + NUMBER_ID_LENGTH;
	n = sendto(sockfd, buffer, n, 0, (struct sockaddr*)paddr, sizeof(struct sockaddr_in));
	if (n < 0) {
		perror("sendto");
	}

	return n;
}

ClientTable table[CLIENT_MAX] = {0};
int client_count = 0;


static int get_index_by_clientid(int client_id) {
	int i = 0;

	int now_count = client_count;
	for (i = 0;i < now_count;i ++) {
		if (table[i].client_id == client_id) return i;
	}

	return RESULT_SUCCESS;
}


static int send_message(int sockfd, int client_id, u8 *buffer, int length) {
	int index = get_index_by_clientid(client_id);
	struct sockaddr_in c_addr;
	c_addr.sin_family = AF_INET;
	array_to_addr(table[index].addr, &c_addr);

	int n = sendto(sockfd, buffer, length, 0, (struct sockaddr*)&c_addr, sizeof(c_addr));
	if (n < 0) {
		perror("sendto");
	}
	return n;
}


static int send_notify(int sockfd, int client_id, int self_id) {
	u8 buffer[BUFFER_LENGTH] = {0};
	int index = get_index_by_clientid(self_id);

	buffer[PROTO_BUFFER_STATUS_IDX] = NTY_PROTO_NOTIFY_REQ;
	*(int*)(buffer+PROTO_NOTIFY_SELFID_IDX) = self_id;
	memcpy(buffer+PROTO_NOTIFY_ADDR_IDX, table[index].addr, CLIENT_ADDR_LENGTH);

	index = get_index_by_clientid(client_id);
	struct sockaddr_in c_addr;
	c_addr.sin_family = AF_INET;
	array_to_addr(table[index].addr, &c_addr);

	int n = PROTO_NOTIFY_ADDR_IDX + CLIENT_ADDR_LENGTH;
	n = sendto(sockfd, buffer, n, 0, (struct sockaddr*)&c_addr, sizeof(c_addr));
	if (n < 0) {
		perror("sendto");
	}

	return n;
}

#endif //HEADER_H
