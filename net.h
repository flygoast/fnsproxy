#ifndef __NET_H_INCLUDED__
#define __NET_H_INCLUDED__

int create_udp_socket(char *addr, int port);
int set_non_block(int fd);
void read_listen_fd(void *arg, int ev);

#endif /* __NET_H_INCLUDED__ */
