#include "server.h"


int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: <Server> <Port>" << endl;
        return -1;
    }
    Server server;
    if (!server.init(atoi(argv[1]))) {
        cout << "server init failed" << endl;
        return -1;
    }
    if (!server.run()) {
        cout << "server run failed" << endl;
        return -1;
    }

    return 0;
}
