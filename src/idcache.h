#ifndef IDCACHE_H
#define IDCACHE_H

/**
 * Pack ID cache - a buffer caching packet IDs with it's origin
 *
 * Written by Daniel Ritz
 */

#include "connection.h"
#include "lib/utils.h"

/**
 * initializes the ID cache
 */
int idcache_initialize();

/**
 * puts a new ID into the cache if not already there
 * @param conn the connection the packet originates from
 * @param dest the destination of the packet
 * @param the packet ID
 * @return true value if ID already in cache, 0 otherwise
 */
int idcache_put(connection_t *conn, char dest, unsigned short id);

/**
 * Searches a packet ID in the cache. Returns the originating connection that
 * must be connection_release()d by the caller. A subsequent call to this function
 * will return NULL to prevent sending 'O' packets multiple times.
 * @param dest the destination
 * @param id the packet ID
 * @return the originating connection of NULL if not found or already found before
 */
connection_t *idcache_get_origin(char dest, unsigned short id, mstime_t *time_sent);

/**
 * Sets the timestamp of a cached entry before sending.
 * @param dest the destination
 * @param id the packet ID
 */
void idcache_set_timestamp(char dest, unsigned short id);

#endif
