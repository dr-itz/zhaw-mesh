/**
 * Packet handling
 *
 * Written by Daniel Ritz
 */

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "packet.h"

packet_t *packet_alloc()
{
	return calloc(1, PACKET_SIZE);
}

packet_t *packet_dup(packet_t *src)
{
	packet_t *copy = malloc(PACKET_SIZE);
	if (!copy)
		return NULL;
	return memcpy(copy, src, PACKET_SIZE);
}

unsigned short packet_get_id(packet_t *pack)
{
	return ntohs(pack->packet.id);
}

packet_t *packet_cre_neighbor(struct sockaddr_in *neigh)
{
	packet_t *pack = packet_alloc();
	if (!pack)
		return NULL;

	pack->packet.type = 'N';
	memcpy(&pack->packet.content[0], &neigh->sin_addr, 4);
	memcpy(&pack->packet.content[4], &neigh->sin_port, 2);

	return pack;
}

void packet_parse_neighbor(packet_t *pack, struct sockaddr_in *neigh)
{
	memset(neigh, 0, sizeof(*neigh));
	neigh->sin_family = AF_INET;
	memcpy(&neigh->sin_addr, &pack->packet.content[0], 4);
	memcpy(&neigh->sin_port, &pack->packet.content[4], 2);
}

packet_t *packet_cre_content(unsigned short id, char dest, void *buf, size_t len)
{
	packet_t *pack = packet_alloc();
	if (!pack)
		return NULL;

	if (len > 128)
		len = 128;

	pack->packet.id = htons(id);
	pack->packet.dest = dest;
	pack->packet.type = 'C';
	memcpy(&pack->packet.content, buf, len);

	return pack;
}
