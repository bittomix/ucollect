#ifndef UCOLLECT_LOOP_H
#define UCOLLECT_LOOP_H

#include <stdbool.h>
#include <stdint.h>

struct loop;
struct plugin;
struct context;

struct epoll_handler {
	void (*handler)(void *data, uint32_t events);
};

struct loop *loop_create() __attribute__((malloc));
void loop_run(struct loop *loop) __attribute__((nonnull));
void loop_break(struct loop *loop) __attribute__((nonnull));
void loop_destroy(struct loop *loop) __attribute__((nonnull));

bool loop_add_pcap(struct loop *loop, const char *interface) __attribute__((nonnull));
// Add a local address for the last added pcap interface. Can be net address (eg. 192.168.0.0/16).
bool loop_pcap_add_address(struct loop *loop, const char *address) __attribute__((nonnull));
void loop_add_plugin(struct loop *loop, struct plugin *plugin) __attribute__((nonnull));

// Register a file descriptor for reading & closing events. Removed on close.
void loop_register_fd(struct loop *loop, int fd, struct epoll_handler *handler) __attribute__((nonnull));

/*
 * Create a new memory pool.
 *
 * The pool will be destroyed with the loop or whith the owner of current context,
 * whatever happens first. The current context may be NULL, which means it's out
 * of plugin context (eg. from the framework).
 */
struct mem_pool *loop_pool_create(struct loop *loop, struct context *current_context, const char *name) __attribute__((nonnull(1, 3))) __attribute__((malloc));
// Get a pool that lives for the whole life of the loop
struct mem_pool *loop_permanent_pool(struct loop *loop) __attribute__((nonnull)) __attribute__((pure));
// Get a temporary pool that may be freed any time the control returns to main loop
struct mem_pool *loop_temp_pool(struct loop *loop) __attribute__((nonnull)) __attribute__((pure));

#endif
