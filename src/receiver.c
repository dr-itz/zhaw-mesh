/*
 * Handling of receiving side, one thread per connection
 *
 * Written by Daniel Ritz
 */

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "lib/utils.h"
#include "lib/net.h"

#include "receiver.h"
#include "packet.h"
#include "idcache.h"
#include "sendq.h"
#include "routing.h"

enum mesh_node_role node_role = normal_node;

static void process_C_packet(connection_t *conn, packet_t *packet)
{
	int seen, err;
	char dest = packet_get_dest(packet);
	unsigned short id = packet_get_id(packet);

	dbg("Received 'C' packet with id %hd for %hhd\n", id, dest);

	// check/add ID to cache
	seen = idcache_put(conn, dest, id);
	if (seen) {
		dbg("  Packet with ID %hd received before, dropping\n", id);
		return;
	}

	// check if destination reached
	if ((dest == 0 && node_role == src_node) ||
	    (dest == 1 && node_role == dest_node))
	{
		dbg("  Packet with ID %hd reached destination\n", id);
		fwrite(packet_get_content(packet), PACKET_CONTENT_SIZE, 1, stdout);
		fflush(stdout);

		// change the type from 'C' to 'O', send back
		packet_set_type(packet, 'O');
		connection_send_packet(conn, packet);

		return;
	}

	// send
	err = sendq_add(packet, conn);
	if (err)
		dbg("  Error writing packet to send queue\n");
	else
		dbg("  Packet put to send queue\n");
}

static void process_O_packet(connection_t *conn, packet_t *packet)
{
	connection_t *origin;
	char dest = packet_get_dest(packet);
	unsigned short id = packet_get_id(packet);
	ssize_t len;
	mstime_t time_sent;

	dbg("Received 'O' packet with id %hd for %hhd\n", id, dest);

	origin = idcache_get_origin(dest, id, &time_sent);
	if (!origin) {
		dbg("  Ack packet received for unknown ID or received before: %hd; dropped.\n", id);
		return;
	}

	// 'conn' is a good connection, update route to use it
	route_mark_alive(conn, dest, time_sent);

	// send ACK directly to the origination connection
	len = connection_send_packet(origin, packet);

	// release ownership of the cached connection
	connection_release(origin);

	if (len == PACKET_SIZE) {
		dbg("  Successfully sent Ack packet with id %hd back to origination connection\n", id);
	} else {
		dbg("  Failed sending Ack packet with ID %hd back to receiver (%zd/%d bytes sent)\n",
			id, len, PACKET_SIZE);
	}
}

static void process_N_packet(connection_t *conn, packet_t *packet)
{
	struct sockaddr_in addr;
	char hoststr[INET_ADDRSTRLEN];
	int err;
	connection_t *newconn;

	packet_parse_neighbor(packet, &addr);

	dbg("Received 'N' packet with %s:%d -> creating outbound connection\n",
		net_addr_str(&addr, hoststr, sizeof(hoststr)), ntohs(addr.sin_port));

	newconn = connection_create_unless_exists(&addr);
	if (!newconn) {
		dbg("  Already connected to the specified host. Ignored\n");
		return;
	}

	// let the new thread do the connect() to avoid blocking the current receiver
	err = receiver_create(newconn);
	if (check_error(err)) {
		dbg("  Cannot allocate receiver...\n");
		goto out_err;
	}

	return;

out_err:
	connection_close(newconn);
	return;
}

static void *receiver_thread(void *arg)
{
	connection_t *conn;
	packet_t *packet;
	ssize_t len;
	char hoststr[INET_ADDRSTRLEN];

	conn = arg;

	packet = packet_alloc();
	if (!packet)
		goto out;

	net_addr_str(connection_get_addr(conn), hoststr, sizeof(hoststr));

	if (connection_unconnected(conn)) {
		dbg("Receiver: trying to connect to %s:%hu\n", hoststr, connection_get_port(conn));
		int newfd = net_connect(connection_get_addr(conn));
		if (check_error(newfd)) {
			dbg("  Cannot connect to %s:%hu\n", hoststr, connection_get_port(conn));
			goto out;
		}
		connection_connect(conn, newfd);
		dbg("  Connected to %s:%hu\n", hoststr, connection_get_port(conn));
	}

	for (;;) {
		len = recv(connection_get_fd(conn), packet, PACKET_SIZE, MSG_WAITALL);

		if (len == -1 && errno == EINTR)
			continue;
		if (len != PACKET_SIZE)
			break;

		char type = packet_get_type(packet);
		switch (type) {
		case 'C':
			process_C_packet(conn, packet);
			break;

		case 'O':
			process_O_packet(conn, packet);
			break;

		case 'N':
			process_N_packet(conn, packet);
			break;

		default:
			dbg("Unknown packet type received: %c\n", type);
			break;
		}
	}
	dbg("Destroying receiver for %s:%hu\n", hoststr, connection_get_port(conn));

out:
	connection_close(conn);
	connection_release(conn);
	free(packet);
	return NULL;
}

int receiver_create(connection_t *conn)
{
	int err;
	pthread_t thr;

	if (!conn)
		return EINVAL;

	err = pthread_create(&thr, NULL, receiver_thread, conn);
	if (!err)
		pthread_detach(thr);

	return -err;
}
