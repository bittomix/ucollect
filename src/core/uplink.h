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
 * Create and connect an uplink. It is expected to be called only once on a given loop.
 *
 * The remote_name and service represent the machine and port to connect to. It can
 * be numerical address and port, or DNS and service name.
 */
struct uplink *uplink_create(struct loop *loop, const char *remote_name, const char *service) __attribute__((malloc)) __attribute__((nonnull));
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

// Some parsing functions

// Get a string from buffer. Returns NULL if badly formatted. The buffer position is updated.
const char *uplink_parse_string(struct mem_pool *pool, const uint8_t **buffer, size_t *length) __attribute__((nonnull));

#endif
