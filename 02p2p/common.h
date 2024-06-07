#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

using namespace std;

constexpr int BUF_LEN = 256;
constexpr uint8_t MSG_VERSION = 0x1;
constexpr uint8_t MSG_LOGIN_REQ = 0x10;
constexpr uint8_t MSG_LOGIN_ACK = 0x11;
constexpr uint8_t MSG_LOGIN_CONFIRM = 0x12;
constexpr uint8_t MSG_P2P = 0x20;
constexpr uint8_t MSG_P2P_REQ = 0x21;
constexpr uint8_t MSG_P2P_ACK = 0x22;
constexpr uint8_t MSG_NOTIFY_PEER = 0x30;

struct Peer {
    Peer(int id, sockaddr_in& addr) : _id(id), _addr(addr) {}
    int _id;
    sockaddr_in _addr;
};

#endif //COMMON_H
