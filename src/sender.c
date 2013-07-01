/**
 * Sender thread
 *
 * Written by Daniel Ritz
 */

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "lib/utils.h"
#include "lib/net.h"

#include "sender.h"
#include "sendq.h"
#include "routing.h"

static void send_unicast(connection_t *conn, packet_t *packet)
{
	ssize_t len;
	char hoststr[INET_ADDRSTRLEN];

	len = connection_send_packet(conn, packet);

	if (len == PACKET_SIZE) {
		dbg("Successfully sent packet with id %hd to %s:%hu\n",
			packet_get_id(packet),
			net_addr_str(connection_get_addr(conn), hoststr, sizeof(hoststr)),
			connection_get_port(conn));
	} else {
		dbg("Failed sending packet with id %hd to %s:%hu (%zd/%d bytes sent)\n",
			packet_get_id(packet),
			net_addr_str(connection_get_addr(conn), hoststr, sizeof(hoststr)),
			connection_get_port(conn), len, PACKET_SIZE);
	}
}

static void send_broadcast(packet_t *packet, connection_t *origin)
{
	connection_t **array, **iter;

	array = connection_get_array();
	for (iter = array; *iter != NULL; iter++) {
		connection_t *conn = *iter;

		if (conn != origin)
			send_unicast(conn, packet);

		connection_release(conn);
	}
	free(array);
}

static void *sender_thread(void *arg)
{
	packet_t *packet;
	connection_t *origin;
	connection_t *route;

	for (;;) {
		sendq_get(&packet, &origin);

		route = route_get(packet);
		if (route != NULL) {
			dbg("Unicast for packet ID %hd to %hhd\n",
				packet_get_id(packet), packet_get_dest(packet));
			send_unicast(route, packet);
			connection_release(route);

		} else {
			dbg("Broadcast for packet ID %hd to %hhd\n",
				packet_get_id(packet), packet_get_dest(packet));
			send_broadcast(packet, origin);
		}

		connection_release(origin);
		free(packet);
	}

	return NULL;
}

int sender_create()
{
	int err;
	pthread_t thr;

	err = pthread_create(&thr, NULL, sender_thread, NULL);
	if (!err)
		pthread_detach(thr);

	return err;
}
