#include "client.h"

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cout << "Usage: <Client> <IP> <Port> <ID>" << endl;
        return -1;
    }
    int id = atoi(argv[3]);
    Client client(id);
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
