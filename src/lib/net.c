/*
 * Generic network releated functions
 *
 * Written by Daniel Ritz
 */

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "net.h"

int net_connect(struct sockaddr_in *addr)
{
	int fd, err;

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd == -1)
		return -errno;

	err = connect(fd, (struct sockaddr *) addr, sizeof(*addr));
	if (err == -1)
		goto out_close;

	return fd;

out_close:
	close(fd);
	return -1;
}


int net_listen(short port)
{
	int fd;
	struct sockaddr_in addr;
	int yes = 1;

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd == -1)
		return -errno;

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
		goto out_close;

	if (listen(fd, 10) < 0)
		goto out_close;

	return fd;

out_close:
	close(fd);
	return -errno;
}

int net_accept(int listenfd, struct sockaddr_in *addr)
{
	int fd;
	socklen_t addr_len;

	addr_len = sizeof(struct sockaddr_in);

	fd = accept(listenfd, (struct sockaddr *) addr, addr ? &addr_len : NULL);
	if (fd <= 0)
		return -errno;
	return fd;
}

int net_resolve(const char *host, const char *port, struct sockaddr_in **addr)
{
	int err;
	struct addrinfo hints;
	struct addrinfo *ret;

	*addr = NULL;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	err = getaddrinfo(host, port, &hints, &ret);
	if (err)
		return err;

	if (ret) {
		*addr = malloc(ret->ai_addrlen);
		if (!*addr)
			goto out_free;
		memcpy(*addr, ret->ai_addr, ret->ai_addrlen);
	}

out_free:
	freeaddrinfo(ret);
	return *addr == NULL ? -1 : 0;
}

char *net_addr_str(struct sockaddr_in *sockaddr, char *ipstr, socklen_t len)
{
	inet_ntop(sockaddr->sin_family, &sockaddr->sin_addr, ipstr, len);
	return ipstr;
}
