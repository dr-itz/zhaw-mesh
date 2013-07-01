/*
 * meshy.c - main() for meshy
 *
 * Written by Daniel Ritz
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/socket.h>

#include "lib/utils.h"
#include "lib/net.h"

#include "connection.h"
#include "receiver.h"
#include "sender.h"
#include "idcache.h"
#include "routing.h"

/** number of sender threads */
#define NUM_SENDERS		3

static void usage()
{
	printf("Usage: meshy <port> [-z|-q] [-v] [-t <route-timeout]\n");
	printf("	-z: Node is the 'destination'\n");
	printf("	-q: Node is the 'source'\n");
	printf("	-v: Enable verbose mode\n");
	printf("	-t: Sets the routing timeout in milliseconds\n");
	exit(1);
}


int main(int argc, char *argv[])
{
	int optchar, err;
	int port = 3333;
	int timeout = -1;
	char dbg_prefix[50];
	char *role_str = " ";
	char *verbose;
	int has_port = 0;

	if (argc > 1) {
		has_port = strlen(argv[1]) && isdigit(argv[1][0]);
		if (has_port) {
			has_port = 1;
			port = atoi(argv[1]);
			if (port == 0)
				usage();
		}
	}

	while ((optchar = getopt(argc-has_port, argv+has_port, "hqzvt:")) != -1) {
		switch (optchar) {
		case 'z':
			node_role = dest_node;
			role_str = "Z";
			break;

		case 'q':
			node_role = src_node;
			role_str = "Q";
			break;

		case 'v':
			set_debug(1);
			break;

		case 't':
			timeout = atoi(optarg);
			break;

		case 'h':
		case '?':
		default:
			usage();
			break;
		}
	}

	// test script sets verbose mode via env
	verbose = getenv("BE_VERBOSE");
	if (verbose && !strcmp(verbose, "1"))
		set_debug(1);

	if (port == 0)
		usage();

	if (timeout != -1) {
		if (timeout < 10) {
			dbg("Invalid route timeout; ignored\n");
		} else {
			dbg("Setting route timeout to %d milliseconds\n", timeout);
			route_set_timeout(atoi(optarg));
		}
	}

	sprintf(dbg_prefix, "Node %s % 5d: ", role_str, port);
	debug_prefix(dbg_prefix);

	// initialize
	err = idcache_initialize();
	if (check_error(err))
		return 1;

	int listenfd = net_listen(port);
	if (check_error(listenfd))
		exit(1);
	dbg("Listening on port %d (fd: %d)\n", port, listenfd);

	// create sender threads
	dbg("Creating %d sender thread(s)\n", NUM_SENDERS);
	for (unsigned int i = 0; i < NUM_SENDERS; i++)
		sender_create();

	// main loop: accept new connections
	for (;;) {
		struct sockaddr_in addr;
		char hoststr[INET_ADDRSTRLEN];
		int newfd;

		newfd = net_accept(listenfd, &addr);

		// EINTR is not a problem, happens when attaching lldb
		if (newfd == -EINTR)
			continue;

		if (check_error(newfd)) {
			if (newfd == -EMFILE || newfd == ENFILE)
				continue;
			exit(1);
		}

		dbg("New inbound connection from %s:%hu\n",
			net_addr_str(&addr, hoststr, sizeof(hoststr)),
			ntohs(addr.sin_port));

		// create new thread handling the connection
		connection_t *conn = connection_create(newfd, &addr);
		if (conn) {
			int err = receiver_create(conn);
			check_error(err);
		} else {
			dbg("Cannot allocate connection...");
			shutdown(newfd, SHUT_RDWR);
			close(newfd);
		}
	}

	return 0;
}
