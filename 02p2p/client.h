#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"

#define MSG_LEN (BUF_LEN-sizeof(int))

void* handle_msg(void* arg);
class Client {
public:
    explicit Client(int id) : _id(id) {}

    bool init() {
        _sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (_sockfd < 0) {
            perror("socket");
            return false;
        }

        return true;
    }

    bool run(const char* domain, unsigned short port) {
        if (0 != pthread_create(&_tid,NULL, handle_msg, this)) {
            cout << "pthread_crate failed" << endl;
            return false;
        }

        sockaddr_in server_addr{};
        socklen_t addrlen = sizeof(sockaddr_in);
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = inet_addr(domain);

        char buf[BUF_LEN]{};
        ssize_t n = 0;
        buf[0] = MSG_VERSION;
        buf[1] = MSG_LOGIN_REQ;
        *(int*)(buf+2) = _id;
        n = sendto(_sockfd, buf, BUF_LEN, 0, (sockaddr*)&server_addr, addrlen);
        if (n < 0) {
            perror("sendto");
            return false;
        }
        cout << "-> msg_login_req" << endl;
        char tmp[MSG_LEN]{};
        while(true) {
            bzero(tmp, MSG_LEN);
            cin >> tmp;
            for (vector<Peer>::iterator it = _peers.begin(); it != _peers.end(); ++it) {
                bzero(buf, BUF_LEN);
                buf[0] = MSG_VERSION;
                buf[1] = MSG_P2P;
                *(int*)(buf+2) = _id;
                strcpy(buf+2+sizeof(int), tmp);
                n = sendto(_sockfd, buf, BUF_LEN, 0, (sockaddr*)&it->_addr, addrlen);
                if (n < 0) {
                    perror("sendto");
                    return false;
                }
            }
        }
    }

    bool contains(int peer_id) {
        for (int i = 0; i < _peers.size(); ++i) {
            if (_peers[i]._id == peer_id) {
                return true;
            }
        }
        return false;
    }

    int _sockfd;
    pthread_t _tid;
    unsigned int _id;
    vector<Peer> _peers;
};

void* handle_msg(void* arg) {
    sockaddr_in addr{};
    socklen_t addrlen = sizeof(sockaddr_in);
    int n = 0;
    Client* client = static_cast<Client*>(arg);
    char buf[BUF_LEN]{};
    while(true) {
        bzero(buf, BUF_LEN);
        n = recvfrom(client->_sockfd, buf, BUF_LEN, 0, (sockaddr*)&addr, &addrlen);
        if (n < 0) {
            perror("recvfrom");
            pthread_exit((void*)-1);
        }
        if (MSG_VERSION != buf[0]) {
            cout << "invalid msg version" << endl;
            continue;
        }
        switch(buf[1]) {
            case MSG_LOGIN_ACK: {
                cout << "<- msg_login_ack" << endl;
                bzero(buf, BUF_LEN);
                buf[0] = MSG_VERSION;
                buf[1] = MSG_LOGIN_CONFIRM;
                *(int*)(buf+2) = client->_id;
                n = sendto(client->_sockfd, buf, BUF_LEN, 0, (sockaddr*)&addr,addrlen);
                if (n < 0) {
                    perror("sendto");
                    pthread_exit((void*)-1);
                }
                cout << "-> msg_login_confirm" << endl;
                cout << "login ok." << endl;
                break;
            }
            case MSG_NOTIFY_PEER: {
                int* pCount = (int*)(buf+2);
                int count = *pCount;
                Peer* p = (Peer*)(pCount + 1);
                for (int i = 0; i < count; ++i) {
                    int peer_id = (p+i)->_id;
                    sockaddr_in peer_addr = (p+i)->_addr;
                    if (peer_id != client->_id && !client->contains(peer_id)) {
                        printf("[New peer: %d] %s:%d\n", peer_id, inet_ntoa(peer_addr.sin_addr), ntohs(peer_addr.sin_port));
                        client->_peers.push_back(Peer(peer_id, peer_addr));
                    }
                }
                break;
            }
            case MSG_P2P: {
                buf[n] = 0;
                int peer_id = *(int*)(buf+2);
                if (peer_id != client->_id) {
                    printf("<-[%d] %s\n", peer_id, buf + 2 + sizeof(int));
                }
                break;
            }
        }
    }
    pthread_exit(NULL);
}

#endif //CLIENT_H
