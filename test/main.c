#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


int get_addr_by_domain(char* addr) {
    struct hostent* phost = gethostbyname2(addr, AF_INET);
    printf("name: %s\n", phost->h_name);
    printf("length: %d\n", phost->h_length);
    printf("addr type: %d\n", phost->h_addrtype);
    char** p;

    struct in_addr in;
    printf("----------addr list-------------\n");
    for (p = phost->h_addr_list; *p; p++) {
        memcpy(&in, p, phost->h_length);
        printf("%s\n",inet_ntoa(in));
    }
    printf("-----------aliases------------\n");
    p = NULL;
    for (p = phost->h_aliases; *p; p++) {
        printf("%s\n", *p);
    }

    return 0;
}

int main(void) {
    get_addr_by_domain("www.baidu.com");
    return 0;
}
