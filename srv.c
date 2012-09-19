#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include "srv.h"
#include "net.h"
#include "times.h"

#define DEFAULT_PORT    53
#define DEFAULT_CONF    "fnsproxy.conf"

/* global server configure */
server_t fnsproxy_srv;

static void srv_su(const char *user) {
    int ret;
    struct passwd   *pwent;

    errno = 0;
    pwent = getpwnam(user);
    if (errno) {
        fprintf(stderr, "getpwname %s failed:%s\n", user, strerror(errno));
        return;
    }

    if (!pwent) {
        fprintf(stderr, "getpwname %s: no such user\n", user);
        return;
    }

    ret = setgid(pwent->pw_gid);
    if (ret == -1) {
        fprintf(stderr, "setgid %d for %s failed:%s\n", pwent->pw_gid,
                user, strerror(errno));
        return;
    }

    ret = setuid(pwent->pw_uid);
    if (ret == -1) {
        fprintf(stderr, "setuid %d for %s failed:%s\n", pwent->pw_uid,
                user, strerror(errno));
        return;
    }
}

void srv_init() {
    fnsproxy_srv.port = DEFAULT_PORT;
    fnsproxy_srv.addr = NULL;
    fnsproxy_srv.user = NULL;
    fnsproxy_srv.conf = DEFAULT_CONF;
    fnsproxy_srv.range = 0;
    dlist_init(&fnsproxy_srv.fds);
}

void srv_serve() {
    int listen_fd;
    int ret;

    if (fnsproxy_srv.user) srv_su(fnsproxy_srv.user);

    /* 10 ms */
    if (event_init(&fnsproxy_srv.evt, srv_cron, &fnsproxy_srv, 10) != 0) {
        fprintf(stderr, "event_init failed\n");
        exit(1);
    }

    if ((listen_fd = create_udp_socket(fnsproxy_srv.addr, 
            fnsproxy_srv.port)) < 0) {
        fprintf(stderr, "create_socket failed\n");
        exit(1);
    }

    fnsproxy_srv.sock.x = &fnsproxy_srv.sock;
    fnsproxy_srv.sock.f = (handle_fn)read_listen_fd;
    fnsproxy_srv.sock.fd = listen_fd;
    fnsproxy_srv.sock.added = 0;

    ret = event_regis(&fnsproxy_srv.evt, &fnsproxy_srv.sock, EVENT_RD);
    if (ret < 0) {
        fprintf(stderr, "event_regis failed:%s\n", strerror(errno));
        exit(1);
    }

    event_loop(&fnsproxy_srv.evt);
}

void srv_destroy() {
    if (fnsproxy_srv.addr) free(fnsproxy_srv.addr);
    if (fnsproxy_srv.user) free(fnsproxy_srv.user);
    if (fnsproxy_srv.conf) free(fnsproxy_srv.conf);

    event_destroy(&fnsproxy_srv.evt);
    dlist_destroy(&fnsproxy_srv.fds);
}

void srv_cron(void *arg, int event) {
    static int i = 0;
    printf("%d\n", ++i);
}
