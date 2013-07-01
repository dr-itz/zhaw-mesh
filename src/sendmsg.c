/**
 * Utility to send messages to meshy
 *
 * Written by Daniel Ritz
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <poll.h>

#include "lib/net.h"
#include "lib/utils.h"

#include "packet.h"


// response wait time in milli seconds
#define RESPONSE_WAIT_TIME		5000

static void usage()
{
	printf("Usage: \n");
	printf("  sendmsg N <host> <port> <addhost> <addport>\n");
	printf("    sends an 'N' message with <addhost>:<addport> to meshy at <host>:<port>\n");
	printf("  sendmsg C <host> <port> (q|z) <id> <msg>\n");
	printf("    sends an 'C' message towards dest 'q' or 'z' with <msg> to meshy at <host>:<port>\n");
	printf("  sendmsg O <host> <port> (q|z) <id>\n");
	printf("    sends an 'O' message towards dest 'q' or 'z' to meshy at <host>:<port>\n");
	exit(1);
}


int main(int argc, char *argv[])
{
	struct sockaddr_in *host = NULL;
	int err;
	ssize_t len;
	char cmd = 0;
	char hoststr[INET_ADDRSTRLEN];
	packet_t *packet = NULL;
	int fd = 0;
	int resp = 0;

	if (argc < 2)
		usage();

	if (!strcmp(argv[1], "N")) {
		if (argc != 6)
			usage();
		cmd = 'N';

	} else if (!strcmp(argv[1], "C")) {
		if (argc != 7)
			usage();
		cmd = 'C';

	} else if (!strcmp(argv[1], "O")) {
		if (argc != 6)
			usage();
		cmd = 'O';
	} else {
		usage();
	}

	// the meshy instance to send the command to
	err = net_resolve(argv[2], argv[3], &host);
	if (err) {
		fprintf(stderr, "Unknown host/port: %s/%s\n", argv[2], argv[3]);
		goto out;
	}

	// construct packet
	if (cmd == 'N') {
		struct sockaddr_in *addhost = NULL;
		char addhoststr[INET_ADDRSTRLEN];

		err = net_resolve(argv[4], argv[5], &addhost);
		if (err) {
			fprintf(stderr, "Unknown host/port: %s/%s\n", argv[3], argv[4]);
			goto out_free;
		}
		packet = packet_cre_neighbor(addhost);

		printf("Sending 'N' packet with %s:%d to %s:%d\n",
			net_addr_str(addhost, addhoststr, sizeof(addhoststr)), ntohs(addhost->sin_port),
			net_addr_str(host, hoststr, sizeof(hoststr)), ntohs(host->sin_port));

		free(addhost);

	} else if (cmd == 'C') {
		char dest = !strcmp(argv[4], "z") ? 1 : 0;
		unsigned short id = (short) atoi(argv[5]);
		char *cont = argv[6];
		packet = packet_cre_content(id, dest, cont, strlen(cont));

		printf("Sending 'C' packet to %s:%d\n",
			net_addr_str(host, hoststr, sizeof(hoststr)), ntohs(host->sin_port));

		resp = 1;

	} else if (cmd == 'O') {
		char dest = !strcmp(argv[4], "z") ? 1 : 0;
		short id = (short) atoi(argv[5]);
		char *cont = "some ok packet";
		packet = packet_cre_content(id, dest, cont, strlen(cont));
		packet_set_type(packet, 'O');

		printf("Sending 'O' packet to %s:%d\n",
			net_addr_str(host, hoststr, sizeof(hoststr)), ntohs(host->sin_port));
	}

	// connect and send packet
	if (!packet) {
		err = -1;
		goto out_close;
	}

	fd = net_connect(host);
	if (check_error(fd))
		goto out_free;


	len = write(fd, packet, PACKET_SIZE);
	if (len == PACKET_SIZE) {
		printf("packet sent\n");
	} else {
		printf("packet incomplete, only %zd/%d bytes sent\n", len, PACKET_SIZE);
		err = -1;
		resp = 0;
	}

	// await response?
	if (resp) {
		struct pollfd polldescr = {
			.fd = fd,
			.events = POLLIN,
		};

		packet_t *respacket = packet_alloc();
		if (!respacket)
			goto out_close;

		// first poll() to have a timeout
		err = poll(&polldescr, 1, RESPONSE_WAIT_TIME);
		if (err == 0) {
			printf("Timeout waiting for a response\n");
		} else if (err == -1) {
			check_error(-errno);

		} else {
			len = recv(fd, respacket, PACKET_SIZE, MSG_WAITALL);
			if (len == -1) {
			   check_error(-errno);
			} else {
				char dest = packet_get_dest(respacket);
				unsigned short id = packet_get_id(respacket);

				printf("Response receveived for ID: %hd to %hhd\n", id, dest);
			}
		}
	}

	// cleanup
out_close:
	shutdown(fd, SHUT_RDWR);
	close(fd);
	free(packet);
out_free:
	free(host);
out:
	return -err;
}
