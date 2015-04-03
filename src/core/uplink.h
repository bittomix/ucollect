/*
    Ucollect - small utility for real-time analysis of network data
    Copyright (C) 2013-2015 CZ.NIC, z.s.p.o. (http://www.nic.cz/)

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

#ifndef UCOLLECT_UPLINK_H
#define UCOLLECT_UPLINK_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

struct uplink;
struct loop;
struct mem_pool;
struct context;

/*
 * Create an uplink. It is expected to be called only once on a given loop.
 */
struct uplink *uplink_create(struct loop *loop) __attribute__((malloc)) __attribute__((nonnull)) __attribute__((returns_nonnull));
/*
 * Set the file where status changes should be stored.
 *
 * This is expected to be called at most once per the lifetime of uplink.
 * If it is not called at all, it won't dump the status changes.
 * The file will be removed on uplink_destroy.
 *
 * It'll keep the pointer provided, so don't free it.
 */
void uplink_set_status_file(struct uplink *uplink, const char *path) __attribute__((nonnull));
/*
 * Set or change the remote endpoint of the uplink. It'll (re)connect.
 *
 * The remote_name and service represent the machine and port to connect to. It can
 * be numerical address and port, or DNS and service name.
 *
 * The remote_name and service must be allocated so it lasts until destroy or next configure.
 * It is not copied.
 */
void uplink_configure(struct uplink *uplink, const char *remote_name, const char *service, const char *login, const char *password, const char *cert);
// Move configuration to the provided pool (so the old may be reset)
void uplink_realloc_config(struct uplink *uplink, struct mem_pool *pool) __attribute__((nonnull));
// Disconnect and connect.
void uplink_reconnect(struct uplink *uplink) __attribute__((nonnull));
/*
 * Disconnect and destroy an uplink. It is expected to be called just before the loop
 * is destroyed.
 */
void uplink_destroy(struct uplink *uplink) __attribute__((nonnull));

/*
 * Send a single message to the server through the uplink connection.
 *
 * The message will have the given type and carry the provided data. The data may be
 * NULL in case size is 0.
 *
 * Blocking, we expect to send small amounts of data, so the link should not get filled.
 *
 * Returns if the message was successfully sent. The connection might have been lost
 * during the send, or not exist at all, in which case it returns false. The message
 * is dropped!
 */
bool uplink_send_message(struct uplink *uplink, char type, const void *data, size_t size) __attribute__((nonnull(1)));

/*
 * Send a message from plugin to the server. Semantics is similar as above, but the header is
 * generated by the function.
 */
bool uplink_plugin_send_message(struct context *context, const void *data, size_t size) __attribute__((nonnull(1)));

// Some parsing & rendering functions

// Get a string from buffer. Returns NULL if badly formatted. The buffer position is updated.
char *uplink_parse_string(struct mem_pool *pool, const uint8_t **buffer, size_t *length) __attribute__((nonnull)) __attribute__((malloc)) __attribute__((returns_nonnull));
// Parses a uint32_t from the buffer. Aborts if not enough data. The buffer position is updated.
uint32_t uplink_parse_uint32(const uint8_t **buffer, size_t *length) __attribute__((nonnull));
// Render string to the wire format. Update position and length of buffer.
void uplink_render_string(const void *string, uint32_t string_len, uint8_t **buffer_pos, size_t *buffer_length) __attribute__((nonnull));
// Render a uint32_t value to wire formate. Update position and length of the buffer.
void uplink_render_uint32(uint32_t value, uint8_t **buffer_pol, size_t *buffer_length) __attribute__((nonnull));

// Get the addresses of uplink, to check the values against packets. It should include all available addresses.
struct addrinfo;
struct addrinfo *uplink_addrinfo(struct uplink *uplink) __attribute__((nonnull));

#endif
