/*
 * Copyright (c) 2011 Ryan Flannery <ryan.flannery@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef CLIENTS_H
#define CLIENTS_H

#include <xcb/xcb.h>
#include <stdbool.h>
#include <stdlib.h>
#include <err.h>

#include "events.h"
#include "xtabs.h"
#include "xutil.h"

void    clients_init();
void    clients_free();
void    clients_update_offset();
void    clients_resize_all();
size_t  clients_get_size();
size_t  clients_get_curr();
size_t  clients_get_offset();

size_t  client_add(xcb_window_t w);
void    client_remove(xcb_window_t w);
void    client_next(size_t n);
void    client_prev(size_t n);
void    client_resize(size_t c);
void    client_focus(size_t c);
bool    client_is_focused(size_t c);

void  client_set_window(size_t c, xcb_window_t w);
void  client_set_name(size_t c, const char *name);
void  client_set_command(size_t c, const char *command);

void         client_get_xbounds(size_t c, int32_t *start, int32_t *end);
xcb_window_t client_get_window(size_t c);
const char*  client_get_name(size_t c);
const char*  client_get_command(size_t c);

#endif
