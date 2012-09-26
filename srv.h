#ifndef __SRV_H_INCLUDED__
#define __SRV_H_INCLUDED__

#include <arpa/inet.h>
#include "geo.h"
#include "net.h"
#include "event.h"
#include "dlist.h"

#define DEFAULT_LISTEN_PORT     53
#define DEFAULT_DNS_PORT        53
#define DEFAULT_GEO_FILE        "fnsproxy.geo"
#define DEFAULT_DNS_ADDR        "114.114.114.114"
#define DEFAULT_LOG_FILE        "fnsproxy.log"
#define CRON_INTERVAL           1000    /* ms */

typedef struct server_st {
    int                 port;
    char                *addr;
    char                *user;
    char                *geo_file;
    int                 geo_mode;
    char                *log_file;
    int                 daemon;
    char                *dns_addr;
    int                 dns_port;
    long long           clock;
    long long           last_check;
    evtent_t            sock;
    event_t             evt;
    dlist               clis;
    geo_t               *geo;
    struct sockaddr_in  server_addr;
} server_t;

extern server_t fnsproxy_srv;

void srv_init();
void srv_serve();
void srv_destroy();
void srv_cron(void *arg, int event);

#endif /* __SRV_H_INCLUDED__ */
