#include <stdio.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "event.h"
#include "srv.h"
#include "net.h"

int create_udp_socket(char *addr, int port) {
    struct sockaddr_in srv;
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        return -1;
    }
    srv.sin_family = AF_INET;
    if (addr == NULL) {
        srv.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        inet_aton(addr, &srv.sin_addr);
    }
    srv.sin_port = htons(port);
    if (bind(fd, (struct sockaddr *)&srv, sizeof(srv)) < 0) {
        return -1;
    }
    return fd;
}

int set_non_block(int fd) {
    int opt = fcntl(fd, F_GETFL, 0);
    if (opt < 0) {
        return -1;
    }
    opt |= O_NONBLOCK;
    return (fcntl(fd, F_SETFL, opt));
}

void read_listen_fd(void *arg, int ev) {
    char buf[2048];
    struct sockaddr_in caddr;
    socklen_t caddr_len = sizeof(caddr);
    evtent_t *sock = (evtent_t *)arg;
    int ret = recvfrom(sock->fd, buf, sizeof(buf), 0,
            (struct sockaddr *)&caddr, &caddr_len);
    if (ret < 0) {
        return;
    }

    printf("Hello, world\n");
}
