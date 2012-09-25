#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "event.h"
#include "times.h"
#include "dlist.h"
#include "srv.h"
#include "net.h"
#include "dns.h"

#define MAX_BUFFER_SIZE     2048
#define READ_TIMEOUT        3000    /* ms */

typedef struct client_st {
    evtent_t            sock;
    struct sockaddr_in  cli_addr;
    long long           expire_at;
} client_t;

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

static void delete_client(client_t *cli) {
    dlist_node *node;
    event_regis(&fnsproxy_srv.evt, &cli->sock, EVENT_DEL);
    node = dlist_search_key(&fnsproxy_srv.clis, &cli->sock);
    if (!node) {
        return;
    }

    dlist_delete_node(&fnsproxy_srv.clis, node);
    close(cli->sock.fd);
    free(cli);
}

static void handle_dns_response(void *arg, int ev) {
    int nrecv, nsent;
    char buf[MAX_BUFFER_SIZE];
    struct sockaddr_in  saddr;
    socklen_t addr_len = sizeof(struct sockaddr_in);

    evtent_t *sock = (evtent_t *)arg;
    if (ev != EVENT_RD) {
        return;
    }

    nrecv = recvfrom(sock->fd, buf, MAX_BUFFER_SIZE, 0,
            (struct sockaddr *)&saddr, &addr_len);
    /* TODO: CHECK PACKET SOURCE */
    if (nrecv < 0) {
        return;
    }

    /* TODO */
    dns_parse_proxy((unsigned char *)buf, nrecv);

    nsent = sendto(fnsproxy_srv.sock.fd, buf, nrecv, 0,
            (struct sockaddr *)&(((client_t *)(sock->x))->cli_addr), addr_len);
    if (nsent < 0 || nsent != nrecv) {
        return;
    }

    delete_client((client_t *)sock->x);
}

void read_from_client(void *arg, int ev) {
    char buf[MAX_BUFFER_SIZE];
    socklen_t addr_len = sizeof(struct sockaddr_in);
    int ret;
    int dns_fd;
    client_t *cli;
    evtent_t *sock = (evtent_t *)arg;

    cli = (client_t *)malloc(sizeof(*cli));
    if (!cli) {
        fprintf(stderr, "OOM");
        return;
    }

    ret = recvfrom(sock->fd, buf, sizeof(buf), 0,
            (struct sockaddr *)&cli->cli_addr, &addr_len);
    if (ret < 0) {
        free(cli);
        return;
    }

    dns_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (dns_fd < 0) {
        free(cli);
        return;
    }
    set_non_block(dns_fd);

    if (sendto(dns_fd, buf, ret, 0, 
            (struct sockaddr *)&fnsproxy_srv.server_addr, addr_len) < 0) {
        free(cli);
        close(dns_fd);
        return;
    }

    cli->sock.x = (void *)cli;
    cli->sock.f = (handle_fn)handle_dns_response;
    cli->sock.fd = dns_fd;
    cli->sock.added = 0;
    cli->expire_at = fnsproxy_srv.clock + READ_TIMEOUT;

    ret = event_regis(&fnsproxy_srv.evt, &cli->sock, EVENT_RD);
    if (ret < 0) {
        free(cli);
        return;
    }

    if (!dlist_add_node_tail(&fnsproxy_srv.clis, cli)) {
        free(cli);
        return;
    }
}

void check_timeout() {
    dlist_iter iter;
    dlist_node *node;
    client_t *cli;
    dlist_rewind(&fnsproxy_srv.clis, &iter);

    while ((node = dlist_next(&iter))) {
        cli = (client_t *)node->value;
        if (cli->expire_at > fnsproxy_srv.clock) {
            delete_client(cli);
        }
    }
}
