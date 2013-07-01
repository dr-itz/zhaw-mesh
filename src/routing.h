#ifndef ROUTING_H
#define ROUTING_H

/**
 * Packet routing interface
 *
 * Written by Daniel Ritz
 */

#include "connection.h"
#include "packet.h"

/**
 * returns the route for the packet to send
 *   the connection must be connection_release()d
 * @param packet the packet to get the route for
 * @return the route for the given destination or null for broadcast.
 */
connection_t *route_get(packet_t *packet);

/**
 * mark a route alive for a connection and a given dest. called whenever an
 * 'O' packet is received from a connection
 * @param conn the connection the 'O' packet was received from
 * @param dest the destination
 * @param time_sent the time the original packet was sent
 */
void route_mark_alive(connection_t *conn, char dest, mstime_t time_sent);

/**
 * sets the routing timeout
 * @param timeout the timeout in seconds
 */
void route_set_timeout(int timeout);

#endif
