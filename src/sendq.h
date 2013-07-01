#ifndef SENDQ_H
#define SENDQ_H

#include "packet.h"
#include "connection.h"

/**
 * Sender Queue
 *
 * Written by Daniel Ritz
 */

/**
 * adds a new packet to the sending queue
 * @param packet the packet - this will be packet_dup()d
 * @param origin the origination connection - will be connection_own()d
 * @return 0 on success
 */
int sendq_add(packet_t *packet, connection_t *origin);

/**
 * Gets an element from the queue, blocking
 * @param packet pointer to packet_t * receving the packet
 * @param conn pointer to connection_t * receving the connection - this must be connection_release()d
 */
void sendq_get(packet_t **packet, connection_t **conn);

#endif
