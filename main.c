#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include "version.h"
#include "errdsp.h"
#include "geo.h"
#include "srv.h"

static void set_sig_handlers() {
    int ret;
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    ret = sigemptyset(&sa.sa_mask);
    if (ret < 0) {
        err_notify(ED_FATAL, "%s:%d sigemptyset failed", __FILE__, __LINE__);
        exit(1);
    }
    ret = sigaction(SIGPIPE, &sa, NULL);
    if (ret < 0) {
        err_notify(ED_FATAL, "sigaction(SIGPIPE) failed");
        exit(1);
    }
}

static void version() {
    printf("fnsproxy: %s, compiled at %s %s\n\n%s\n", 
        FNSPROXY_VERSION, __DATE__, __TIME__, FNSPROXY_COPYRIGHT);
}

static void usage() {
    fprintf(stderr, "Usage: %s [OPTIONS]\n", "fnsproxy");
    fprintf(stderr, "  -d       daemonize\n");
    fprintf(stderr, "  -r       change geo mode to range, default is CIDR\n");
    fprintf(stderr, "  -p NUM   listen port, default is %d\n", 
            DEFAULT_LISTEN_PORT);
    fprintf(stderr, "  -l ADDR  listen address, default is INADDR_ANY\n");
    fprintf(stderr, "  -n NUM   DNS server port, default is %d\n",
            DEFAULT_DNS_PORT);
    fprintf(stderr, "  -a ADDR  DNS server address, default is %s\n", 
            DEFAULT_DNS_ADDR);
    fprintf(stderr, "  -u STR   run as the user\n");
    fprintf(stderr, "  -g STR   geo file, default is %s\n",
            DEFAULT_GEO_FILE);
    fprintf(stderr, "  -o STR   log file, deefault is %s\n",
            DEFAULT_LOG_FILE);
    fprintf(stderr, "  -h       show help information\n");
    fprintf(stderr, "  -v       show version information\n");
}

static void option_parse(int argc, char **argv) {
    char *end;
    int c;

    while ((c = getopt(argc, argv, "drp:l:n:a:u:g:o:hv")) != -1) {
        switch (c) {
        case 'o':
            fnsproxy_srv.log_file = strdup(optarg);
            if (!fnsproxy_srv.log_file) {
                err_notify(ED_FATAL, "%s:%d Out of memory when strdup(\"%s\")", 
                        __FILE__, __LINE__, optarg);
                exit(1);
            }
            break;
        case 'd':
            fnsproxy_srv.daemon = 1;
            break;
        case 'r':
            fnsproxy_srv.geo_mode = GEO_RANGE;
            break;
        case 'p':
            fnsproxy_srv.port = strtol(optarg, &end, 10);
            if (end == optarg || (*end != ' ' && *end != '\0')) {
                usage();
                exit(1);
            }
            break;
        case 'l':
            fnsproxy_srv.addr = strdup(optarg);
            if (!fnsproxy_srv.addr) {
                err_notify(ED_FATAL, "%s:%d Out of memory when strdup(\"%s\")", 
                        __FILE__, __LINE__, optarg);
                exit(1);
            }
            break;
        case 'n':
            fnsproxy_srv.dns_port = strtol(optarg, &end, 10);
            if (end == optarg || (*end != ' ' && *end != '\0')) {
                usage();
                exit(1);
            }
            break;
        case 'a':
            fnsproxy_srv.dns_addr = strdup(optarg);
            if (!fnsproxy_srv.dns_addr) {
                err_notify(ED_FATAL, "%s:%d Out of memory when strdup(\"%s\")", 
                        __FILE__, __LINE__, optarg);
                exit(1);
            }
            break;
        case 'u':
            fnsproxy_srv.user = strdup(optarg);
            if (!fnsproxy_srv.user) {
                err_notify(ED_FATAL, "%s:%d Out of memory when strdup(\"%s\")", 
                        __FILE__, __LINE__, optarg);
                exit(1);
            }
            break;
        case 'g':
            fnsproxy_srv.geo_file = strdup(optarg);
            if (!fnsproxy_srv.geo_file) {
                err_notify(ED_FATAL, "%s:%d Out of memory when strdup(\"%s\")", 
                        __FILE__, __LINE__, optarg);
                exit(1);
            }
            break;
        case 'v':
            version();
            exit(0);
            break;
        case 'h':
            usage();
            exit(0);
            break;
        default:
            usage();
            exit(1);
        }
    }
 
    if (argc != optind) {
        usage();
        exit(1);
    }
}

int main(int argc, char **argv) {
    srv_init();
    option_parse(argc, argv);
    set_sig_handlers();
    srv_serve();
    srv_destroy();
    exit(0);
}
