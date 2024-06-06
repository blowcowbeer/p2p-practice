#include "header.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cout << "Usage: <Client> <IP> <Port>" << endl;
        return -1;
    }

    Client client;
    if (!client.init()) {
        cout << "client init failed" << endl;
        return -1;
    }
    if (!client.run(argv[1], atoi(argv[2]))) {
        cout << "client failed" << endl;
        return -1;
    }

    return 0;
}
