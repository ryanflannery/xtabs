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

#ifndef EVENTS_H
#define EVENTS_H

#include <xcb/xcb.h>
#include <err.h>

#include "clients.h"
#include "xtabs.h"
#include "xutil.h"

void xevent_recv_buttonpress(xcb_button_press_event_t *e);
void xevent_recv_configure_notify(xcb_configure_notify_event_t *e);
void xevent_recv_create_notify(xcb_create_notify_event_t *e);
void xevent_recv_destroy_notify(xcb_destroy_notify_event_t *e);
void xevent_recv_keypress(xcb_key_press_event_t *e);
void xevent_recv_property_notify(xcb_property_notify_event_t *e);

void xevent_send_kill(xcb_window_t w);
void xevent_send_raise(xcb_window_t w);
void xevent_send_resize(xcb_window_t w);

#endif
