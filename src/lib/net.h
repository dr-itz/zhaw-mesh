#ifndef LIB_NET_H
#define LIB_NET_H

#include <netinet/in.h>


/**
 * opens a TCP socket to the given host/port
 * @param addr the address (port already set)
 */
int net_connect(struct sockaddr_in *addr);

/**
 * creates a listening TCP socket on the specified port (bind and listen)
 * @param port the port
 * @return socket fd or error code (negative)
 */
int net_listen(short port);

/**
 * accepts a new connection on the specfied listner. blocking.
 * @param listenfd the listening socket
 * @param addr pointer to sockaddr_in to receive the address of the new connection. NULL ok.
 * @return new socket
 */
int net_accept(int listenfd, struct sockaddr_in *addr);

/**
 * resolves a host name
 * @param host host name
 * @param pointer to sockaddr * receiving the actual address (must be free()d afterwards)
 * @return error code, 0 on success
 */
int net_resolve(const char *host, const char *port, struct sockaddr_in **addr);


/**
 * print IP address into buffer
 * @param addr the address to print
 * @param ipstr buffer receiving the IP
 * @param len len of the buffer
 * @return ipstr
 */
char *net_addr_str(struct sockaddr_in *sockaddr, char *ipstr, socklen_t len);

#endif
