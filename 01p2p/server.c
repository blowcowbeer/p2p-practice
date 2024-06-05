#include "header.h"

int parse_message(int sockfd, u8* buffer, u32 length, struct sockaddr_in* paddr) {
    u8 status = buffer[PROTO_BUFFER_STATUS_IDX];
    printf("status: %d\n", status);

    switch (status) {
        case PROTO_LOGIN_REQ: {
            int old = client_count;
            int now = old + 1;
            if (0 == cmpxchg((uautomic*)&client_count, old, now)) {
                printf("client count: %d, old: %d, now : %d\n", client_count, old, now);
                return RESULT_FAILED;
            }
            u8 array[CLIENT_ADDR_LENGTH] = {0};
            addr_to_array(array, paddr);
            printf("login: %d.%d.%d.%d:%d\n",
                *(unsigned char*)(&paddr->sin_addr.s_addr),
                *((unsigned char*)(&paddr->sin_addr.s_addr)+1),
                *((unsigned char*)(&paddr->sin_addr.s_addr)+2),
                *((unsigned char*)(&paddr->sin_addr.s_addr)+3),
                paddr->sin_port);
            table[now].client_id =  *(u32*)(buffer+PROTO_LOGIN_SELFID_IDX);
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
        case NTY_RPOTO_MESSAGE_REQ: {
            u8 *msg = buffer+RPORO_MESSAGE_CONTENT_IDX;
            int client_id = *(unsigned int*)(buffer+RPORO_MESSAGE_SELFID_IDX);
            int other_id = *(unsigned int*)(buffer+PROTO_MESSAGE_OTHERID_IDX);
            printf(" from client:%d --> %s\n", client_id, msg);
            send_message(sockfd, other_id, buffer, length);
            break;
        }
    }

    return RESULT_SUCCESS;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: <Server> <Port>\n");
        return -1;
    }
    printf("This is UDP server\n");

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return -1;
    }

    char buf[BUFFER_LENGTH] = {0};
    struct sockaddr_in cliaddr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    int n;
    while(1) {
        n = recvfrom(sockfd, buf, BUFFER_LENGTH, 0, (struct sockaddr*)&cliaddr, &addrlen);
        if (n == -1) {
            perror("recv");
            break;
        }
        else if (n == 0) {
            printf("peer closed\n");
        }
        else {
            buf[n] = 0;
            printf("%d.%d.%d.%d:%d say: %s\n",
                *(unsigned char*)(&cliaddr.sin_addr.s_addr),
                *((unsigned char*)(&cliaddr.sin_addr.s_addr)+1),
                *((unsigned char*)(&cliaddr.sin_addr.s_addr)+2),
                *((unsigned char*)(&cliaddr.sin_addr.s_addr)+3),
                cliaddr.sin_port,
                buf);

            int ret = parse_message(sockfd, buf, n, &cliaddr);
            if (ret == RESULT_FAILED) {
                continue;
            }

            buf[PROTO_BUFFER_STATUS_IDX] += 0x80;
            n = sendto(sockfd, buf, n, 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
            if (n == -1) {
                perror("sendto");
                return -1;
            }
        }
    }

    close(sockfd);

    return 0;
}