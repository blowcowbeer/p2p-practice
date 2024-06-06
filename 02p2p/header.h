#ifndef NODE_H
#define NODE_H

#include <iostream>
#include <map>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
using namespace std;

constexpr int BUF_LEN = 256;

class Server {
public:
    explicit Server() :  _sockfd(-1) {}

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
            }
            buf[n] = 0;
            printf(">: %s\n", buf);
        }
    }

private:
    int _sockfd;
    map<unsigned int, sockaddr_in> _peers;
};


class Client {
public:
    explicit Client() : _sockfd(-1) {}

    bool init() {
        _sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (_sockfd < 0) {
            perror("socket");
            return false;
        }
        return true;
    }

    bool run(const char* addr, unsigned short port) {
        sockaddr_in server_addr{};
        socklen_t addrlen = sizeof(server_addr);
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = inet_addr(addr);

        char buf[BUF_LEN]{};
        ssize_t n = 0;
        while(true) {
            cout << "<: ";
            cin >> buf;
            //fgets(buf ,BUF_LEN, stdin);
            n = sendto(_sockfd, buf, BUF_LEN, 0, (sockaddr*)&server_addr, addrlen);
            if (n < 0) {
                perror("sendto");
                return false;
            }
        }
    }

private:
    int _sockfd;
    unsigned int _id;
    map<unsigned int, sockaddr_in> _peers;
};

#endif //NODE_H
