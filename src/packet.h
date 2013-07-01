#ifndef PACKET_H
#define PACKET_H


/*
 * Packet structure:
 * || 2 Bytes  || 1 Byte                   || 1 Byte                        || 128 Bytes ||
 * || Paket-ID || Ziel (1) oder Quelle (0) || Paket Typ ('C', 'O' oder 'N') || Inhalt    ||
 * || 0, 1     || 2                        || 3                             || 4-131     ||
 */

#define PACKET_SIZE				132
#define PACKET_CONTENT_SIZE		128

// struct representation of the packet. only works since this will have no padding
struct __packet {
	unsigned short id;
	char dest;
	char type;
	char content[PACKET_CONTENT_SIZE];
};

typedef union packet {
	char raw[PACKET_SIZE];
	struct __packet packet;
} packet_t;



// forward declarations
struct sockaddr_in;

/**
 * allocates a new packet, all 0. Must be free()d by caller
 * @return the new packet
 */
packet_t *packet_alloc();

/**
 * duplicates an existing packet. Must be free()d by caller
 * @param src the original packet
 * @return the duplicated packet
 */
packet_t *packet_dup(packet_t *src);

/**
 * returns the type of the packet
 * @param pack the packet
 * @return type as char
 */
static inline char packet_get_type(packet_t *pack)
{
	return pack->packet.type;
}
/**

 * sets the type in the packet
 * @param pack the packet
 * @parma type the type to set
 */
static inline void packet_set_type(packet_t *pack, char type)
{
	pack->packet.type = type;
}

/**
 * returns the destination (0 or 1)
 * @param pack the packet
 * @return dest as char
 */
static inline char packet_get_dest(packet_t *pack)
{
	return pack->packet.dest & 0x01;
}

/**
 * returns the packet ID
 * @param pack the packet
 * @return packet ID
 */
unsigned short packet_get_id(packet_t *pack);


/**
 * returns the pointer to the packet's conent
 */
static inline char *packet_get_content(packet_t *pack)
{
	return pack->packet.content;
}

/**
 * creates a new packet with an 'N' (add neighbor) request for the specified
 * IPv4 address. The packet returned must be free()d by caller
 * @param neighbor address
 * @return new packet
 */
packet_t *packet_cre_neighbor(struct sockaddr_in *neigh);

/**
 * parses a packet a 'N' (add neighbor)
 * @param pack the packet to parse
 * @param neigh pointer to sockaddr_in receiving the address
 */
void packet_parse_neighbor(packet_t *pack, struct sockaddr_in *neigh);

/**
 * creates a content packet (type 'C')
 * @param id the packet ID
 * @param dest the destination
 * @param buf pointer to the buffer with the content
 * @param len the length of buffer, anything > 128 gets truncated to 128 bytes
 * @return new packet
 */
packet_t *packet_cre_content(unsigned short id, char dest, void *buf, size_t len);

#endif
