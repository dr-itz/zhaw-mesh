/**
 * Packet ID cache
 *
 * Uses a pre-alloated array of hash entries acting as a ring on writing.
 * Also uses a fixed-size hash using entry-embedded doubly linked lists to
 * provide fast lookup. The hash function used is "((id & 0xFF) * 33 + id >> 8)"
 *
 * Written by Daniel Ritz
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#include "lib/utils.h"

#include "idcache.h"
#include "connection.h"

#define PACKET_ID_CACHE_SIZE	1024
#define PACKET_ID_HASH_BITS		   6

#define HASH_ENTRIES			(1 << PACKET_ID_HASH_BITS)
#define HASH_MASK				(HASH_ENTRIES - 1)

struct idcache_entry {
	connection_t *conn;
	list_head_t hashnode;
	mstime_t time;
	unsigned short id;
	char dest;
};

static pthread_mutex_t idcache_lock = PTHREAD_MUTEX_INITIALIZER;
static struct idcache_entry *idcache;
static list_head_t *idhash;
static int ptr_write;

int idcache_initialize()
{
	idcache = calloc(PACKET_ID_CACHE_SIZE, sizeof(struct idcache_entry));
	if (!idcache)
		goto out_free;

	idhash = malloc(HASH_ENTRIES * sizeof(list_head_t));
	if (!idhash)
		goto out_free;
	for (unsigned int i = 0; i < HASH_ENTRIES; i++)
		INIT_LIST_HEAD(&idhash[i]);

	return 0;

out_free:
	free(idcache);
	return -ENOMEM;
}

static list_head_t *get_hash_bucket(unsigned short id)
{
	unsigned int bucket = ((id & 0xFF) * 33 + (id >> 8)) & HASH_MASK;
	return idhash + bucket;
}

static struct idcache_entry *get_bucket_entry(list_head_t *bucket,
	char dest, unsigned short id)
{
	struct idcache_entry *entry;

	list_for_each_entry(entry, bucket, hashnode) {
		if (entry->id == id && entry->dest == dest)
			return entry;
	}

	return NULL;
}

int idcache_put(connection_t *conn, char dest, unsigned short id)
{
	struct idcache_entry *cache;
	list_head_t *bucket;

	pthread_mutex_lock(&idcache_lock);

	bucket = get_hash_bucket(id);
	cache = get_bucket_entry(bucket, dest, id);
	if (!cache) {
		// release old, own new connection
		if (idcache[ptr_write].conn != NULL)
			connection_release(idcache[ptr_write].conn);
		connection_own(conn);

		// update entry
		idcache[ptr_write].conn = conn;
		idcache[ptr_write].time = 0;
		idcache[ptr_write].dest = dest;
		idcache[ptr_write].id = id;

		// put into hash
		if (idcache[ptr_write].hashnode.next)
			list_remove(&idcache[ptr_write].hashnode);
		list_add(&idcache[ptr_write].hashnode, bucket);

		// increment write pointer
		ptr_write = (ptr_write + 1) % PACKET_ID_CACHE_SIZE;
	}

	pthread_mutex_unlock(&idcache_lock);

	return cache != NULL;
}


connection_t *idcache_get_origin(char dest, unsigned short id, mstime_t *time_sent)
{
	struct idcache_entry *cache;
	connection_t *ret = NULL;
	list_head_t *bucket;

	pthread_mutex_lock(&idcache_lock);

	bucket = get_hash_bucket(id);
	cache = get_bucket_entry(bucket, dest, id);
	if (cache && cache->conn) {
		// Just return the connection. the resulting connection must
		// be connection_released() by the caller. Since the the cache will
		// clear the connection from it's entry, the ownership is just handed
		// over...no need for connection_own()/connection_release()
		ret = cache->conn;

		// Clear the connection in the cache to prevent acks for
		// the same packet received multiple times from being sent more
		// than once. (this should not happen, but the thing is supposed
		// to work with implementations from other sudents...)
		cache->conn = NULL;

		if (time_sent)
			*time_sent = cache->time;
	}

	pthread_mutex_unlock(&idcache_lock);

	return ret;
}

void idcache_set_timestamp(char dest, unsigned short id)
{
	struct idcache_entry *cache;
	list_head_t *bucket;

	pthread_mutex_lock(&idcache_lock);

	bucket = get_hash_bucket(id);
	cache = get_bucket_entry(bucket, dest, id);
	if (cache)
		cache->time = time_current();

	pthread_mutex_unlock(&idcache_lock);
}
