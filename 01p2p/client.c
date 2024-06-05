#include "header.h"

static int status_machine = STATUS_LOGIN;
static int client_selfid = 0;
struct sockaddr_in addr;
ClientTable peers[CLIENT_MAX] = {0};
static int peer_count = 0;

static int parse_message(int sockfd, u8* buf, u32 length, struct sockaddr_in* paddr) {
    u8 status = buf[PROTO_BUFFER_STATUS_IDX];
    switch(status) {
        case NTY_PROTO_NOTIFY_REQ: {
            struct sockaddr_in other_addr;
            other_addr.sin_family = AF_INET;
            array_to_addr(buf+PROTO_NOTIFY_ADDR_IDX, &other_addr);
            send_p2pconnect(sockfd, client_selfid, &other_addr);
            break;
        }
        case NTY_PROTO_P2P_CONNECT_REQ: {
            int now_count = peer_count++;
            peers[now_count].stamp = time_genrator();
            peers[now_count].client_id = *(int*)(buf+PROTO_P2P_CONNECT_SELFID_IDX);
            addr_to_array(peers[now_count].addr, paddr);
            send_p2pconnectack(sockfd, client_selfid, paddr);
            printf("Enter P2P Model\n");
            status_machine = STATUS_P2P_MESSAGE;
            break;
        }
        case NTY_PROTO_P2P_CONNECT_ACK: {
            int now_count = peer_count++;
            peers[now_count].stamp = time_genrator();
            peers[now_count].client_id = *(int*)(buf+PROTO_P2P_CONNECT_SELFID_IDX);
            addr_to_array(peers[now_count].addr, paddr);
            printf("Enter P2P Model\n");
            status_machine = STATUS_P2P_MESSAGE;
            break;
        }
        case NTY_RPOTO_MESSAGE_REQ: {
            u8 *msg = buf+RPORO_MESSAGE_CONTENT_IDX;
            u32 other_id = *(u32*)(buf+RPORO_MESSAGE_SELFID_IDX);
            printf(" from client: %d --> %s\n", other_id, msg);
            send_messageack(sockfd, client_selfid, paddr);
            //status_machine = STATUS_P2P_MESSAGE;
            break;
        }
        case PROTO_LOGIN_ACK: {
            printf(" Connect Server Success\nPlease Enter Message : ");
            status_machine = STATUS_MESSAGE;
            break;
        }
        case PROTO_HEARTBEAT_ACK:
        case PROTO_CONNECT_ACK:
        case NTY_PROTO_NOTIFY_ACK:
            break;
        case NTY_RPOTO_MESSAGE_ACK:
            break;
        default:
            break;
    }

    return RESULT_SUCCESS;
}

void* task_recv(void *arg) {
    int sockfd = *(int*)arg;
    struct sockaddr_in addr;
    socklen_t length = sizeof(struct sockaddr_in);
    u8 buffer[BUFFER_LENGTH] = {0};
    //printf("recv_callback --> enter\n");
    while (1) {
        int n = recvfrom(sockfd, buffer, BUFFER_LENGTH, 0, (struct sockaddr*)&addr, &length);
        if (n > 0) {
            buffer[n] = 0;
            parse_message(sockfd, buffer, n, &addr);
        } else if (n == 0) {
            printf("server closed\n");
            close(sockfd);
            break;
        } else if (n == -1) {
            perror("recvfrom");
            close(sockfd);
            break;
        }
    }
    pthread_exit(NULL);
}

void* task_send(void *arg) {
    int sockfd = *(int*)arg;
    char buffer[BUFFER_LENGTH] = {0};
    //printf("send_callback --> enter\n");

    while (1) {
        bzero(buffer, BUFFER_LENGTH);
        scanf("%s", buffer);
        //getchar();
        if (status_machine == STATUS_MESSAGE) {
            printf(" --> please enter bt : ");
            int other_id = buffer[1]-0x30;
            if (buffer[0] == 'C') {
                //printf("send_connect");
                send_connect(sockfd, client_selfid, other_id, &addr);
            } else {
                int length = strlen(buffer);
                client_send_message(sockfd, client_selfid, other_id, &addr, buffer, length);
            }
        } else if (status_machine == STATUS_P2P_MESSAGE) {
            printf(" --> please enter message to send : ");
            int now_count = peer_count;
            struct sockaddr_in c_addr;
            c_addr.sin_family = AF_INET;
            array_to_addr(peers[now_count-1].addr, &c_addr);
            int length = strlen(buffer);
            client_send_message(sockfd, client_selfid, 0, &c_addr, buffer, length);
        }
    }

    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    printf(" This is a UDP Client\n");

    if (argc != 4) {
        printf("Usage: <Client> <IP> <Port> <ID>\n", argv[0]);
        exit(1);
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    pthread_t thread_id[2] = {0};
    CALLBACK cb[2] = {task_send, task_recv};
    int i = 0;
    for (i = 0;i < 2;i ++) {
        int ret = pthread_create(&thread_id[i], NULL, cb[i], &sockfd);
        if (ret) {
            perror("pthread_create");
            exit(1);
        }
        sleep(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    client_selfid = atoi(argv[3]);
    send_login(sockfd, client_selfid, &addr);
    for (i = 0;i < 2;i ++) {
        pthread_join(thread_id[i], NULL);
    }

    return 0;
}

