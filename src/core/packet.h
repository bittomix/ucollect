/*
    Ucollect - small utility for real-time analysis of network data
    Copyright (C) 2013 CZ.NIC, z.s.p.o. (http://www.nic.cz/)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef UCOLLECT_PACKET_H
#define UCOLLECT_PACKET_H

#include <stddef.h>
#include <stdint.h>

// Some forward declarations
struct mem_pool;
struct address_list;

// One endpoint of communication.
enum endpoint {
	END_SRC,  // The source endpoint
	END_DST,  // The destination endpoint
	END_COUNT // Not a real endpoint, but a stop-mark to know the count.
};

// Direction of the packet
enum direction {
	DIR_IN,		// The packet goes in
	DIR_OUT,        // The packet goes out
	DIR_UNKNOWN,    // Can't guess the direction (it might be a packet in internal network)
	DIR_COUNT       // Not a real direction, a stop-mark.
};

enum tcp_flags {
	TCP_FIN = 1 << 0,
	TCP_SYN = 1 << 1,
	TCP_RESET = 1 << 2,
	TCP_PUSH = 1 << 3,
	TCP_ACK = 1 << 4,
	TCP_URG = 1 << 5
};

struct packet_info {
	// The parsed embedded packet, in case of app_protocol == '4' || '6'
	const struct packet_info *next;
	// Length and raw data of the packet (starts with IP header or similar on the same level)
	size_t length;
	const void *data;
	// Textual name of the interface it was captured on
	const char *interface;
	/*
	 * Length of headers (IP+TCP (or equivalent) together).
	 * Can be used to find application data.
	 *
	 * This is 0 in case ip_protocol != 4 && 6 or app_protocol != 'T' && 'U'.
	 */
	size_t hdr_length;
	// Packet timestamp in microseconds since epoch
	uint64_t timestamp;
	/*
	 * Source and destination address. Raw data (addr_len bytes each).
	 * Is set only with ip_protocol == 4 || 6, or with ethernet frames.
	 */
	const void *addresses[END_COUNT];
	/*
	 * Source and destination ports. Converted to the host byte order.
	 * Filled in only in case the app_protocol is T or U. Otherwise, it is 0.
	 */
	uint16_t ports[END_COUNT];
	/*
	 * The layer of the packet:
	 * - 'E': Ethernet.
	 * - 'I': IP layer.
	 * - 'S': Linux SLL.
	 * - '?': Some other layer (unknown).
	 */
	char layer;
	// The raw value of the datalink of the layer.
	int layer_raw;
	// As in iphdr, 6 for IPv6, 4 for IPv4. Others may be present.
	unsigned char ip_protocol;
	/*
	 * The application-facing protocol. Currently, these are recognized for IP layer:
	 * - 'T': TCP
	 * - 'U': UDP
	 * - 'i': ICMP
	 * - 'I': ICMPv6
	 * - '4': Encapsulated IPv4 packet
	 * - '6': Encapsulated IPv6 packet
	 * - '?': Other, not recognized protocol.
	 *
	 * This is set only with ip_protocol == 4 || 6, otherwise it is
	 * zero.
	 *
	 * These are on the ethernet and SLL layer:
	 * - 'I': An IP packet is below.
	 * - 'A': ARP.
	 * - 'R': Reverse ARP.
	 * - 'W': Wake On Lan
	 * - 'X': IPX
	 * - 'E': EAP
	 * - 'P': PPPoE
	 * - '?': Unrecognized
	 * Beware that we may add more known protocols in future.
	 */
	char app_protocol;
	/*
	 * The raw byte specifying what protocol is used below IP. The app_proto
	 * is more friendly.
	 *
	 * In case the ip_protocol is not 4 nor 6, it 255 (which is "Reserved").
	 */
	uint8_t app_protocol_raw;
	// Length of one address field. 0 in case ip_protocol != 4 && 6
	unsigned char addr_len;
	// Direction of the packet.
	enum direction direction;
	/*
	 * The flag byte from TCP packets. The Nonce is not included here.
	 * If the packet isn't IP/TCP, it is left zero.
	 */
	uint8_t tcp_flags;
	// If non-zero, it holds the IEEE 802.1Q VLAN tag. The AD tags are not preserved here.
	uint16_t vlan_tag;
        // Fragment offset as in struct iphdr (including flags)
        uint16_t frag_off;
};

/*
 * Parse the stuff in the passed packet. It expects length and data are already
 * set, it fills the addresses, protocols, etc.
 */
void uc_parse_packet(struct packet_info *packet, struct mem_pool *pool, int datalink) __attribute__((nonnull));

/*
 * Which endpoint is the local one for the given direction?
 *
 * This returns END_COUNT in case the direction is not DIR_IN or DIR_OUT.
 */
static inline enum endpoint local_endpoint(enum direction direction) {
	switch (direction) {
		case DIR_OUT:
			return END_SRC;
		case DIR_IN:
			return END_DST;
		default:
			return END_COUNT;
	}
}

/*
 * Which endpoint is the remote one for the given direction?
 *
 * This returns END_COUNT in case the direction is not DIR_IN or DIR_OUT.
 */
static inline enum endpoint remote_endpoint(enum direction direction) {
	switch (direction) {
		case DIR_IN:
			return END_SRC;
		case DIR_OUT:
			return END_DST;
		default:
			return END_COUNT;
	}
}

#endif
