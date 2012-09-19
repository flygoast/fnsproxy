#ifndef __SRV_H_INCLUDED__
#define __SRV_H_INCLUDED__

#include "event.h"
#include "dlist.h"

typedef struct server_st {
    int         port;
    char        *addr;
    char        *user;
    char        *conf;
    int         range;
    evtent_t    sock;
    event_t     evt;
    dlist       fds;
} server_t;

extern server_t fnsproxy_srv;

void srv_init();
void srv_serve();
void srv_destroy();
void srv_cron(void *arg, int event);

#endif /* __SRV_H_INCLUDED__ */
