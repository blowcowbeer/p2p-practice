#ifndef NODE_H
#define NODE_H

#include "common.h"

void* broadcast_peer(void*);



class Server {
public:
    explicit Server() {}
    ~Server() {}

    bool init(unsigned short port) {
        _sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (_sockfd < 0) {
            return false;
        }
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(_sockfd, (sockaddr*)&addr, sizeof(addr)) < 0) {
            return false;
        }

        return true;
    }

    bool run() {
        if (0 != pthread_create(&_tid, NULL, broadcast_peer, this)) {
            cout << "pthread_create error" << endl;
            return false;
        }

        char buf[BUF_LEN]{};
        ssize_t n = 0;
        sockaddr_in from_addr{};
        socklen_t addrlen = sizeof(from_addr);
        while (true) {
            n = recvfrom(_sockfd, buf, BUF_LEN, 0, (sockaddr*)&from_addr, &addrlen);
            if (n < 0) {
                perror("recvfrom");
                return false;
            }
            if (n == 0) {
                cout << "peer closed" << endl;
                continue;
            }
            if (buf[0] != MSG_VERSION) {
                cout << "invalid msg version" << endl;
                continue;
            }
            switch (buf[1]) {
                case MSG_LOGIN_REQ: {
                    cout << "<- msg_login_req" << endl;
                    bzero(buf, BUF_LEN);
                    buf[0] = MSG_VERSION;
                    buf[1] = MSG_LOGIN_ACK;
                    n = sendto(_sockfd, buf, BUF_LEN, 0, (sockaddr*)&from_addr, addrlen);
                    if (n < 0) {
                        perror("sendto");
                        return false;
                    }
                    cout << "-> msg_login_ack" << endl;
                    break;
                }
                case MSG_LOGIN_CONFIRM: {
                    cout << "<- msg_login_confirm" << endl;
                    char* from = inet_ntoa(from_addr.sin_addr);
                    int id = *(int*)(buf+2);
                    cout << from << ":" << ntohs(from_addr.sin_port) << "-" << id << " login." << endl;
                    _peers.push_back(Peer(id, from_addr));
                    break;
                }
                case MSG_P2P_REQ: {
                    break;
                }
            }
        }
    }


    int _sockfd;
    pthread_t _tid;
    vector<Peer> _peers;
};

void* broadcast_peer(void* arg) {
    socklen_t addrlen = sizeof(sockaddr_in);
    Server* server = static_cast<Server *>(arg);
    char buf[BUF_LEN]{};
    int n = 0;
    while (true) {
        if (server->_peers.empty()) {
            sleep(10);
            continue;
        }

        bzero(buf, BUF_LEN);
        buf[0] = MSG_VERSION;
        buf[1] = MSG_NOTIFY_PEER;
        *(int*)(buf+2) = server->_peers.size();
        Peer* p = (Peer*)(buf + 6);
        for (int i = 0; i < server->_peers.size(); ++i) {
            *(p+i) = server->_peers[i];
        }
        for (int i = 0; i < server->_peers.size(); ++i) {
            n = sendto(server->_sockfd, buf, BUF_LEN, 0, (sockaddr*)&server->_peers[i]._addr, addrlen);
            if (n < 0) {
                perror("sendto");
                pthread_exit((void*)-1);
            }
        }
        sleep(10);
    }

    pthread_exit(NULL);
}


#endif //NODE_H
