#ifndef RECEIVER_H
#define RECEIVER_H

#include "connection.h"

enum mesh_node_role {
	normal_node = 0,
	dest_node = 1,
	src_node = 2,
};

extern enum mesh_node_role node_role;

/**
 * Create a receiver thread for the given connection
 * @param conn the connection
 * @return return value of pthread_create
 */
int receiver_create(connection_t *conn);

#endif
