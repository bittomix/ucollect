/*
    Ucollect - small utility for real-time analysis of network data
    Copyright (C) 2014 CZ.NIC, z.s.p.o. (http://www.nic.cz/)

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

#ifndef UCOLLECT_FLOW_FILTER_H
#define UCOLLECT_FLOW_FILTER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

struct filter;
struct packet_info;
struct mem_pool;

bool filter_apply(const struct filter *filter, const struct packet_info *packet) __attribute__((nonnull));
struct filter *filter_parse(struct mem_pool *pool, const uint8_t *desc, size_t size) __attribute__((nonnull));

#endif
