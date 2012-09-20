#ifndef __SRV_H_INCLUDED__
#define __SRV_H_INCLUDED__

#include <arpa/inet.h>
#include "net.h"
#include "event.h"
#include "dlist.h"

typedef struct server_st {
    int                 port;
    char                *addr;
    char                *user;
    char                *geo_file;
    int                 range;
    int                 daemon;
    char                *dns_addr;
    int                 dns_port;
    evtent_t            sock;
    event_t             evt;
    dlist               clis;
    struct sockaddr_in  server_addr;
} server_t;

extern server_t fnsproxy_srv;

void srv_init();
void srv_serve();
void srv_destroy();
void srv_cron(void *arg, int event);

#endif /* __SRV_H_INCLUDED__ */
