#ifndef __DNS_H_INCLUDED__
#define __DNS_H_INCLUDED__

#include <stdint.h>

#define DNS_A_RECORD    0x01
#define DNS_MX_RECORD   0x0f

typedef struct dns_header_st {
    uint16_t        tid;            /* transaction id */
    uint16_t        flags;          /* flags */
    uint16_t        nquestions;     /* question number */
    uint16_t        nanswers;       /* answer number */
    uint16_t        nauths;         /* authority RR number */
    uint16_t        nadditionals;   /* additional RR number */
    unsigned char   data[0];        /* data stub */
} dns_header_t;

#endif /* __DNS_H_INCLUDED__ */
