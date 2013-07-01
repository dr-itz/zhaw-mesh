#ifndef CONNECTION_H
#define CONNECTION_H

#include <pthread.h>
#include <netinet/in.h>

#include "lib/list.h"

#include "packet.h"

/*
 * Definition of connection API. A connection is a socket FD and additional status
 */

/** state of a connection */
enum connection_state {
	unconnected = 0,
	active = 1,
	closed = 2,
};

/** one connection */
typedef struct connection {
	int fd;
	struct sockaddr_in addr;

	enum connection_state state;
	pthread_mutex_t lock;
	pthread_mutex_t sendlock;
	unsigned int refs;
	list_head_t list_entry;
} connection_t;

/**
 * returns the FD associated with a connection
 * @param conn the connection
 * @return the FD
 */
static inline int connection_get_fd(connection_t *conn)
{
	return conn->fd;
}

/**
 * returns the remote address associated with a connection
 * @param conn the connection
 * @return the pointer to the remote address (owned by the connection)
 */
static inline struct sockaddr_in *connection_get_addr(connection_t *conn)
{
	return &conn->addr;
}

/**
 * returns the remote port associated with a connection
 * @param conn the connection
 * @return the remote port, host byte order
 */
static inline unsigned short connection_get_port(connection_t *conn)
{
	return ntohs(conn->addr.sin_port);
}

/**
 * checks if a connection is unconnceted, lock free (cannot ever happen concurrently)
 * @param conn the connection
 * @return true value if the connection state is unconnceted
 */
static inline int connection_unconnected(connection_t *conn)
{
	return conn->state == unconnected;
}

/**
 * checks if a connection is OK (active)
 * @param conn the connection
 * @return true value if connection is active
 */
int connection_ok(connection_t *conn);

/**
 * bind an FD to an unconnected connection
 * @param conn the connection
 * @param fd the file descriptor
 */
void connection_connect(connection_t *conn, int fd);

/**
 * creates a connection from an fd and adds it to the connection list
 * @param fd the socket fd
 * @param addr address of the connection
 * @return connection
 */
connection_t *connection_create(int fd, struct sockaddr_in *addr);


/**
 * creates an unconnected connection unless a connection for this address
 * already exists
 * @param addr the socket address
 * @return connection if newlycreated, NULL if already exists
 */
connection_t *connection_create_unless_exists(struct sockaddr_in *addr);

/**
 * closes a connection and removes it from the table. The caller also
 * has to call connection_release() to release ownership.
 * @param conn the connection
 */
void connection_close(connection_t *conn);

/**
 * Increases the reference counter by one
 * @param conn the connection to own
 */
void connection_own(connection_t *conn);

/**
 * Releases a previously owned connection. Decreases the reference counter.
 * If the reference counter drops to zero, the connection will be free()d.
 * @param conn the connection to release
 */
void connection_release(connection_t *conn);

/**
 * sends a packet, locking the connection before sending
 * @param conn the connection
 * @param packet the packet to send
 * @return the number of bytes actually sent or -1 on error
 */
ssize_t connection_send_packet(connection_t *conn, packet_t *packet);

/**
 * gets all current connections as an array (NULL-terminated)
 * return value must be free()d
 * @return array of current connections
 */
connection_t **connection_get_array();

#endif
