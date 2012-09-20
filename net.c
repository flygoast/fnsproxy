#include <stdio.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "event.h"
#include "srv.h"
#include "net.h"

#define MAX_BUFFER_SIZE     2048

typedef struct client {
    evtent_t            sock;
    struct sockaddr_in  caddr;
} client;

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

static void handle_dns_response(void *arg, int ev) {
    int ret, ret1;
    char buf[MAX_BUFFER_SIZE];
    struct sockaddr_in  caddr;
    struct sockaddr_in  saddr;
    socklen_t           caddr_len;
    socklen_t           saddr_len;

    evtent_t *sock = (evtent_t *)arg;
    if (ev != EVENT_RD) {
        return;
    }

    ret = recvfrom(sock->fd, buf, MAX_BUFFER_SIZE, 0,
            (struct sockaddr *)&saddr, &saddr_len);
    if (ret > 0) {
        ret1 = sendto(fnsproxy_srv.sock.fd, buf, ret1, 0,
            (struct sockaddr *)&caddr, sizeof(caddr_len));
        if (ret1 < 0 || ret1 != ret) {
            return;
        }
    }
}

void read_from_client(void *arg, int ev) {
    char buf[MAX_BUFFER_SIZE];
    struct sockaddr_in caddr;
    struct sockaddr_in saddr;
    socklen_t caddr_len = sizeof(caddr);
    evtent_t *sock = (evtent_t *)arg;
    int ret;
    int dns_fd;
    client *cli;
    
    ret = recvfrom(sock->fd, buf, sizeof(buf), 0,
            (struct sockaddr *)&caddr, &caddr_len);
    if (ret < 0) {
        return;
    }

    dns_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (dns_fd < 0) {
        return;
    }
    set_non_block(dns_fd);

    if (sendto(dns_fd, buf, ret, 0,
            (struct sockaddr *)&saddr, saddr_len) < 0) {
        close(dns_fd);
        return;
    }

    cli = (client *)malloc(sizeof(*cli));
    if (!cli) {
        return;
    }

    cli->sock.x = cli;
    cli->sock.f = (handle_fn)handle_dns_response;
    cli->sock.fd = dns_fd;
    cli->sock.added = 0;
    cli->caddr = caddr;

    ret = event_regis(&fnsproxy_srv.evt, &cli->sock, EVENT_RD);
    if (ret < 0) {
        free(cli);
        return;
    }
}
