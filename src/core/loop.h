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

#ifndef UCOLLECT_LOOP_H
#define UCOLLECT_LOOP_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>

struct loop;
struct loop_configurator;

struct plugin;
struct plugin_activation;
struct context;
struct uplink;
struct config_node;

struct epoll_handler {
	void (*handler)(void *data, uint32_t events);
};

struct loop *loop_create() __attribute__((malloc)) __attribute__((returns_nonnull));
// Warning: This one is not reentrant, due to signal handling :-(
void loop_run(struct loop *loop) __attribute__((nonnull));
void loop_break(struct loop *loop) __attribute__((nonnull));
void loop_destroy(struct loop *loop) __attribute__((nonnull));
// Like fork, but closes FDs and stuff in the child. Do not use the loop in the child! Designed to exec in child afterwards.
pid_t loop_fork(struct loop *loop) __attribute__((nonnull));

/*
 * Get statistics of the interfaces of the loop.
 *
 * It returns an array allocated from the temporary memory pool.
 * First, there's number of interfaces. Then, each iterface has 3 items:
 * (received, dropped, dropped by interface driver).
 *
 * In case the statistics for an interface fail, all the three items of it
 * are set to maximum value.
 *
 * The statistics are diff from the last time this function was called.
 */
size_t *loop_pcap_stats(struct context *context) __attribute__((nonnull)) __attribute__((malloc)) __attribute__((returns_nonnull));
/*
 * When you want to configure the loop, you start by loop_config_start. You get
 * a handle to the configurator. You can then call loop_add_pcap and
 * loop_add_plugin functions with it. After you are done, you call loop_config_commit,
 * which will make the changes available.
 *
 * You can call loop_config_abort instead, which will throw out all the changes.
 *
 * The whole operation (from _start to _commit or _abort) must happen at once, before
 * any callback or so is left.
 *
 * You need to list all the plugins, addresses, etc. that should be available in the
 * new configuration. The ones that were available in the old config are copied over
 * (not initialized again). The new ones are created and the old ones removed on the
 * commit.
 */
struct loop_configurator *loop_config_start(struct loop *loop) __attribute__((nonnull)) __attribute__((malloc)) __attribute__((returns_nonnull));
void loop_config_commit(struct loop_configurator *configurator) __attribute__((nonnull));
void loop_config_abort(struct loop_configurator *configurator) __attribute__((nonnull));

bool loop_add_pcap(struct loop_configurator *configurator, const char *interface, bool promiscuous) __attribute__((nonnull));
// Add a plugin. Provide the name of the library to load.
bool loop_add_plugin(struct loop_configurator *configurator, const char *plugin) __attribute__((nonnull));
// Set the remote endpoint of the uplink
void loop_uplink_configure(struct loop_configurator *configurator, const char *remote, const char *service, const char *login, const char *password, const char *cert) __attribute__((nonnull(1,2,3)));
/*
 * Provide a configuration option for a plugin. This will be given to the next plugin loaded by loop_add_plugin.
 *
 * If it is a list, call this with the same name for each value in the list.
 */
void loop_set_plugin_opt(struct loop_configurator *configurator, const char *name, const char *value) __attribute__((nonnull));
/*
 * Require a pluglib library for the next loaded plugin. The libname is the file name of the library.
 */
void loop_set_pluglib(struct loop_configurator *configurator, const char *libname) __attribute__((nonnull));
/*
 * Look up a plugin configuration value (must be called from within a plugin.
 *
 * It returns the currently active value usually. Only, during plugin configuration
 * (especially from within the config_check_callback), it returns the candidate value
 * to be checked.
 */
const struct config_node *loop_plugin_option_get(struct context *context, const char *name) __attribute__((nonnull));
/*
 * Reinitialize the current plugin. Must not be called from outside of a plugin.
 *
 * It'll not return to the plugin, the plugin will be terminated at that moment.
 */
void loop_plugin_reinit(struct context *context) __attribute__((nonnull)) __attribute__((noreturn));

const char *loop_plugin_get_name(const struct context *context) __attribute__((nonnull)) __attribute__((const));
bool loop_plugin_active(const struct context *context) __attribute__((nonnull));
/*
 * Set the uplink used by this loop. This may be called at most once on
 * a given loop.
 */
void loop_uplink_set(struct loop *loop, struct uplink *uplink) __attribute__((nonnull));
// Called by the uplink when connection is made
void loop_uplink_connected(struct loop *loop) __attribute__((nonnull));
// Called by the uplink when connection is lost
void loop_uplink_disconnected(struct loop *loop) __attribute__((nonnull));

// Register a file descriptor for reading & closing events. Removed on close.
void loop_register_fd(struct loop *loop, int fd, struct epoll_handler *handler) __attribute__((nonnull));
/*
 * Remove the FD from epoll.
 *
 * This is a good idea to do so, even when epoll removes closed ones by itself.
 * It may happen the file descriptor would be still called, due to various reasons.
 * After this, the handler won't get falled for this file descriptor any more.
 */
void loop_unregister_fd(struct loop *loop, int fd) __attribute__((nonnull));

/*
 * Create a new memory pool.
 *
 * The pool will be destroyed with the loop or with the owner of current context,
 * whatever happens first. The current context may be NULL, which means it's out
 * of plugin context (eg. from the framework).
 */
struct mem_pool *loop_pool_create(struct loop *loop, struct context *current_context, const char *name) __attribute__((nonnull(1, 3))) __attribute__((malloc)) __attribute__((returns_nonnull));
// Get a pool that lives for the whole life of the loop
struct mem_pool *loop_permanent_pool(struct loop *loop) __attribute__((nonnull)) __attribute__((pure)) __attribute__((returns_nonnull));
// Get a temporary pool that may be freed any time the control returns to main loop
struct mem_pool *loop_temp_pool(struct loop *loop) __attribute__((nonnull)) __attribute__((pure)) __attribute__((returns_nonnull));

/*
 * Send some data from uplink to a plugin. Plugin is specified by name.
 * Returns true if the plugin exists and is active, false if not.
 */
bool loop_plugin_send_data(struct loop *loop, const char *plugin, const uint8_t *data, size_t length) __attribute__((nonnull));

/*
 * Have a file descriptor watched by the main loop. When it is ready to be read or
 * has been closed, call the plugin's fd_callback with provided fd and a data tag (may be NULL).
 *
 * If the plugin is destroyed, the fds registered this way are closed automatically.
 */
void loop_plugin_register_fd(struct context *context, int fd, void *tag) __attribute__((nonnull (1)));
/*
 * No longer watch the given fd for events. Call this before closing the FD.
 */
void loop_plugin_unregister_fd(struct context *context, int fd) __attribute__((nonnull));

/*
 * Have a function called after given number of milliseconds.
 * Context may be NULL in case this is called by something else than a plugin.
 * It returns an id that is then passed to the callback. It can also be used to cancel the timeout
 * before it happens.
 *
 * The timeouts are not expected to happen often, so this function is not very optimised.
 */
size_t loop_timeout_add(struct loop *loop, uint32_t after, struct context *context, void *data, void (*callback)(struct context *context, void *data, size_t id)) __attribute__((nonnull(1)));
// Cancel a timeout. It must not have been called yet.
void loop_timeout_cancel(struct loop *loop, size_t id) __attribute__((nonnull));
// Return number of milliseconds since some unspecified time in the past
uint64_t loop_now(struct loop *loop) __attribute__((pure));

// Activate or deactivate plugins. If needed, send update of plugin versions and/or errors.
void loop_plugin_activation(struct loop *loop, struct plugin_activation *plugins, size_t count) __attribute__((nonnull));

#endif
