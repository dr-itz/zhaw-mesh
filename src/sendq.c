/**
 * Sender Queue
 *
 * Written by Daniel Ritz
 */

#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

#include "lib/list.h"
#include "connection.h"
#include "sendq.h"

struct sendq_entry {
	packet_t *packet;
	connection_t *origin;
};

#define SEND_QUEUE_SIZE	100

static struct sendq_entry send_queue[SEND_QUEUE_SIZE];
static pthread_mutex_t sendq_list_lock = PTHREAD_MUTEX_INITIALIZER;
static int qpos_write;
static int qpos_read;
static int qsize;

static pthread_cond_t sendq_cond_notfull = PTHREAD_COND_INITIALIZER;
static pthread_cond_t sendq_cond_notempty = PTHREAD_COND_INITIALIZER;

int sendq_add(packet_t *packet, connection_t *origin)
{
	packet = packet_dup(packet);
	if (!packet)
		return -ENOMEM;

	pthread_mutex_lock(&sendq_list_lock);
	while (qsize == SEND_QUEUE_SIZE)
		pthread_cond_wait(&sendq_cond_notfull, &sendq_list_lock);

	connection_own(origin);
	send_queue[qpos_write].packet = packet;
	send_queue[qpos_write].origin = origin;

	qsize++;
	qpos_write++;
	if (qpos_write == SEND_QUEUE_SIZE)
		qpos_write = 0;

	pthread_mutex_unlock(&sendq_list_lock);
	pthread_cond_broadcast(&sendq_cond_notempty);

	return 0;
}

void sendq_get(packet_t **packet, connection_t **origin)
{
	pthread_mutex_lock(&sendq_list_lock);

	while (qsize == 0)
		pthread_cond_wait(&sendq_cond_notempty, &sendq_list_lock);

	*packet = send_queue[qpos_read].packet;
	*origin = send_queue[qpos_read].origin;

	qsize--;
	qpos_read++;
	if (qpos_read == SEND_QUEUE_SIZE)
		qpos_read = 0;

	pthread_mutex_unlock(&sendq_list_lock);
	pthread_cond_broadcast(&sendq_cond_notfull);
}
