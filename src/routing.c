/**
 * Packet routing
 *
 * Written by Daniel Ritz
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>

#include "lib/utils.h"
#include "lib/net.h"

#include "routing.h"
#include "idcache.h"

// configurable timeout in milliseconds
int route_timeout = 200;

struct route_entry {
	// time the route was last requested
	mstime_t last_requested;

	// time the route was last validated
	mstime_t last_validated;

	// the associated connection
	connection_t *conn;
};

static struct route_entry routes[2];
static pthread_mutex_t route_lock = PTHREAD_MUTEX_INITIALIZER;


static int route_ok(int idx, mstime_t now)
{
	if (connection_ok(routes[idx].conn)) {
		// requested but not validated: ok if request is max. route_timeout seconds old
		if (routes[idx].last_validated == 0)
			return routes[idx].last_requested + route_timeout > now;
		// requested and later validated
		return 1;
	}
	return 0;
}

connection_t *route_get(packet_t *packet)
{
	connection_t *route = NULL; // NULL: broadcast
	char dest = packet_get_dest(packet);
	unsigned short id = packet_get_id(packet);
	int idx = dest & 0x01;
	mstime_t now = time_current();

	// update the timestamp in the cache, used for health check
	idcache_set_timestamp(dest, id);

	pthread_mutex_lock(&route_lock);

	if (route_ok(idx, now)) {
		route = routes[idx].conn;
		connection_own(route);
	}

	// update timestamps. only reset last_validate if not in the same 5 milliseconds
	if (routes[idx].last_validated + 5 < now)
		routes[idx].last_validated = 0;
	if (routes[idx].last_requested + route_timeout < now)
		routes[idx].last_requested = now;

	pthread_mutex_unlock(&route_lock);

	return route;
}

void route_mark_alive(connection_t *conn, char dest, mstime_t time_sent)
{
	int idx = dest & 0x01;
	mstime_t now = time_current();

	char hoststr[INET_ADDRSTRLEN];
	unsigned short port = connection_get_port(conn);
	net_addr_str(connection_get_addr(conn), hoststr, sizeof(hoststr));

	// the route works, but is too slow?
	if (now - time_sent > route_timeout) {
		dbg("Route alive, but too slow: %s:%hu\n", hoststr, port);
		return;
	}

	pthread_mutex_lock(&route_lock);

	if (!route_ok(idx, now)) {
		// new route found, set
		if (routes[idx].conn != conn)
			connection_release(conn);
		connection_own(conn);
		routes[idx].conn = conn;
		routes[idx].last_validated = now;

		dbg("New route for dest %hhd: %s:%hu\n", dest, hoststr, port);

	} else if (routes[idx].conn == conn){
		// route is the current, update timestamp
		routes[idx].last_validated = now;

		dbg("Re-validate current route for dest %hhd: %s:%hu\n", dest, hoststr, port);
	}

	pthread_mutex_unlock(&route_lock);
}

void route_set_timeout(int timeout)
{
	route_timeout = timeout;
}
