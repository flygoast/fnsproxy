#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include "srv.h"
#include "geo.h"
#include "net.h"
#include "errdsp.h"
#include "times.h"
#include "log.h"

/* global server configure */
server_t fnsproxy_srv;

static void srv_su(const char *user) {
    int ret;
    struct passwd   *pwent;

    errno = 0;
    pwent = getpwnam(user);
    if (errno) {
        err_notify(ED_ERROR, "%s:%d getpwname(\"%s\") failed", 
                __FILE__, __LINE__, user);
        return;
    }

    if (!pwent) {
        err_notify(ED_ERROR, "%s:%d getpwname(\"%s\") failed", 
                __FILE__, __LINE__, user);
        return;
    }

    ret = setgid(pwent->pw_gid);
    if (ret == -1) {
        err_notify(ED_ERROR, "%s:%d setgid %d for \"%s\" failed", 
                __FILE__, __LINE__, pwent->pw_gid, user);
        return;
    }

    ret = setuid(pwent->pw_uid);
    if (ret == -1) {
        err_notify(ED_ERROR, "%s:%d setuid %d for \"%s\" failed", 
                __FILE__, __LINE__, pwent->pw_uid, user);
        return;
    }
}

void srv_init() {
    fnsproxy_srv.port = DEFAULT_LISTEN_PORT;
    fnsproxy_srv.addr = NULL;
    fnsproxy_srv.user = NULL;
    fnsproxy_srv.geo_file = NULL;
    fnsproxy_srv.geo_mode = GEO_CIDR;
    fnsproxy_srv.log_file = NULL;
    fnsproxy_srv.daemon = 0;
    fnsproxy_srv.dns_addr = NULL;
    fnsproxy_srv.dns_port = DEFAULT_DNS_PORT;
    fnsproxy_srv.clock = mstime();
    dlist_init(&fnsproxy_srv.clis);
}

void srv_serve() {
    int listen_fd;
    int ret;

    if (fnsproxy_srv.user) {
        srv_su(fnsproxy_srv.user);
    }

    if (event_init(&fnsproxy_srv.evt, srv_cron, &fnsproxy_srv, 
            DEFAULT_EVENT_INTERVAL) != 0) {
        err_notify(ED_ERROR, "%s:%d event_init failed", __FILE__, __LINE__);
        exit(1);
    }

    if ((listen_fd = create_udp_socket(fnsproxy_srv.addr, 
            fnsproxy_srv.port)) < 0) {
        err_notify(ED_ERROR, "%s:%d create_udp_socket failed", 
                __FILE__, __LINE__);
        exit(1);
    }

    fnsproxy_srv.sock.x = &fnsproxy_srv.sock;
    fnsproxy_srv.sock.f = (handle_fn)read_from_client;
    fnsproxy_srv.sock.fd = listen_fd;
    fnsproxy_srv.sock.added = 0;

    ret = event_regis(&fnsproxy_srv.evt, &fnsproxy_srv.sock, EVENT_RD);
    if (ret < 0) {
        err_notify(ED_ERROR, "%s:%d event_regis failed", 
                __FILE__, __LINE__);
        exit(1);
    }

    bzero(&fnsproxy_srv.server_addr, sizeof(struct sockaddr_in));
    fnsproxy_srv.server_addr.sin_family = AF_INET;
    fnsproxy_srv.server_addr.sin_port = htons(fnsproxy_srv.dns_port);
    fnsproxy_srv.server_addr.sin_addr.s_addr = inet_addr(fnsproxy_srv.dns_addr ?
        fnsproxy_srv.dns_addr : DEFAULT_DNS_ADDR);

    fnsproxy_srv.geo = geo_load(NULL, 
            fnsproxy_srv.geo_file ? fnsproxy_srv.geo_file : DEFAULT_GEO_FILE,
            fnsproxy_srv.geo_mode);
    if (!fnsproxy_srv.geo) {
        err_notify(ED_ERROR, "%s:%d load geo file \"%s\" failed", 
            __FILE__, __LINE__, 
            fnsproxy_srv.geo_file ? fnsproxy_srv.geo_file : DEFAULT_GEO_FILE);
        exit(1);
    }

    if (log_init(".", fnsproxy_srv.log_file ? 
                fnsproxy_srv.log_file : DEFAULT_LOG_FILE,
                LOG_LEVEL_ALL,
                LOG_FILE_SIZE,
                LOG_FILE_NUM,
                LOG_MULTI_NO) < 0) {
        err_notify(ED_ERROR, "%s:%d log_init failed", __FILE__, __LINE__);
        exit(1);
    }

    if (fnsproxy_srv.daemon) {
        daemon(1, 1);
    }

    event_loop(&fnsproxy_srv.evt);
}

void srv_destroy() {
    if (fnsproxy_srv.addr) free(fnsproxy_srv.addr);
    if (fnsproxy_srv.user) free(fnsproxy_srv.user);
    if (fnsproxy_srv.geo_file) free(fnsproxy_srv.geo_file);
    if (fnsproxy_srv.dns_addr) free(fnsproxy_srv.dns_addr);
    if (fnsproxy_srv.log_file) free(fnsproxy_srv.log_file);

    event_destroy(&fnsproxy_srv.evt);
    dlist_destroy(&fnsproxy_srv.clis);
    if (fnsproxy_srv.geo) {
        geo_unload(fnsproxy_srv.geo);
    }
    log_close();
}

void srv_cron(void *arg, int event) {
    fnsproxy_srv.clock = mstime();

    if (fnsproxy_srv.clock - fnsproxy_srv.last_check > CRON_INTERVAL) {
        fnsproxy_srv.last_check = fnsproxy_srv.clock;
        check_timeout();
    }
}
