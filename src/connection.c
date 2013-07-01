/*
 * Connection handling
 *
 * Written by Daniel Ritz
 */

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include "connection.h"

static LIST_HEAD(connection_list);
static int connection_list_size;
pthread_mutex_t connection_list_lock = PTHREAD_MUTEX_INITIALIZER;

int connection_ok(connection_t *conn)
{
	int ret;
	if (!conn)
		return 0;

	pthread_mutex_lock(&conn->lock);
	ret = conn->state == active;
	pthread_mutex_unlock(&conn->lock);
	return ret;
}

static connection_t *create_unconnected(struct sockaddr_in *addr)
{
	connection_t *conn = calloc(1, sizeof(connection_t));
	if (!conn)
		return NULL;

	memcpy(&conn->addr, addr, sizeof(struct sockaddr_in));
	conn->refs = 2; // caller, connection list
	conn->fd = -1;

	pthread_mutex_init(&conn->lock, NULL);
	pthread_mutex_init(&conn->sendlock, NULL);

	list_add(&conn->list_entry, &connection_list);
	connection_list_size++;

	return conn;
}

void connection_connect(connection_t *conn, int fd)
{
	pthread_mutex_lock(&conn->lock);
	conn->state = active;
	conn->fd = fd;
	pthread_mutex_unlock(&conn->lock);
}

connection_t *connection_create(int fd, struct sockaddr_in *addr)
{
	connection_t *conn = NULL;

	pthread_mutex_lock(&connection_list_lock);
	conn = create_unconnected(addr);
	if (conn)
		connection_connect(conn, fd);
	pthread_mutex_unlock(&connection_list_lock);

	return conn;
}

connection_t *connection_create_unless_exists(struct sockaddr_in *addr)
{
	int found = 0;
	connection_t *conn = NULL;

	pthread_mutex_lock(&connection_list_lock);
	list_for_each_entry(conn, &connection_list, list_entry) {
		if (conn->addr.sin_addr.s_addr == addr->sin_addr.s_addr &&
			conn->addr.sin_port == addr->sin_port)
		{
			found = 1;
			break;
		}
	}
	if (!found)
		conn = create_unconnected(addr);
	pthread_mutex_unlock(&connection_list_lock);

	return conn;
}

void connection_close(connection_t *conn)
{
	pthread_mutex_lock(&conn->lock);

	// remove from list
	pthread_mutex_lock(&connection_list_lock);
	list_remove(&conn->list_entry);
	connection_list_size--;
	pthread_mutex_unlock(&connection_list_lock);
	conn->refs--;

	// shutdown and close socket
	if (conn->state == active) {
		pthread_mutex_lock(&conn->sendlock);
		shutdown(conn->fd, SHUT_RDWR);
		close(conn->fd);
		conn->fd = -1;
		conn->state = closed;
		pthread_mutex_unlock(&conn->sendlock);
	}

	pthread_mutex_unlock(&conn->lock);
}

void connection_own(connection_t *conn)
{
	pthread_mutex_lock(&conn->lock);
	conn->refs++;
	pthread_mutex_unlock(&conn->lock);
}

void connection_release(connection_t *conn)
{
	unsigned int refs;
	pthread_mutex_lock(&conn->lock);
	refs = --(conn->refs);
	pthread_mutex_unlock(&conn->lock);

	if (refs == 0) {
		pthread_mutex_destroy(&conn->sendlock);
		pthread_mutex_destroy(&conn->lock);
		free(conn);
	}
}

ssize_t connection_send_packet(connection_t *conn, packet_t *packet)
{
	ssize_t len = 0;

	/*
	 * This seems racy - conn->state can change between conn->lock and conn->sendlock
	 * But it's safe:
	 * - if a connection is not yet active but becomes active during the "race",
	 *   we just fail to send - as we would if there was no race
	 * - if a connection is active but becomes inactive during the "race", the FD
	 *   stays around (updating it is locked with conn->sendlock) and write() will just file
	 * The only point is never to write() in unconnected state where there's no valid FD
	 */
	pthread_mutex_lock(&conn->lock);
	if (conn->state != active) {
		pthread_mutex_unlock(&conn->lock);
		return 0;
	}
	pthread_mutex_unlock(&conn->lock);

	pthread_mutex_lock(&conn->sendlock);
	do {
		len = write(connection_get_fd(conn), packet, PACKET_SIZE);
	} while (len < 0 && errno == EINTR);
	pthread_mutex_unlock(&conn->sendlock);

	return len;
}

connection_t **connection_get_array()
{
	connection_t **array, **iter, *conn;

	pthread_mutex_lock(&connection_list_lock);
	array = calloc(connection_list_size + 1, sizeof(connection_t *));
	if (array) {
		iter = array;
		list_for_each_entry(conn, &connection_list, list_entry) {
			connection_own(conn);
			*iter = conn;
			iter++;
		}
	}
	pthread_mutex_unlock(&connection_list_lock);
	return array;
}
