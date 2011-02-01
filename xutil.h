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

#ifndef XUTIL_H
#define XUTIL_H

#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_atom.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>

typedef struct {
   xcb_connection_t  *connection;
   xcb_screen_t      *screen;
   xcb_colormap_t     colormap;
   xcb_window_t       window;
   char              *str_window; /* string form of window id */

   uint16_t           width, height, bar_height, tab_width;
   uint16_t           font_ascent, font_descent, font_padding;
   xcb_pixmap_t       bar;
   xcb_pixmap_t       tab;
   xcb_font_t         font;

   xcb_gcontext_t     gc_bar_norm_fg, gc_bar_norm_bg;
   xcb_gcontext_t     gc_bar_curr_fg, gc_bar_curr_bg;
   xcb_gcontext_t     gc_bar_border;
} xinfo;
extern xinfo X;


void     x_init();
void     x_free();

void     x_set_window_name(const char *name, xcb_window_t);
char*    x_get_window_name(xcb_window_t w);
char*    x_get_command(xcb_window_t w);
int32_t  x_get_strwidth(const char *s);

xcb_alloc_color_reply_t*       x_load_color(uint16_t r, uint16_t g, uint16_t b);
xcb_alloc_named_color_reply_t* x_load_strcolor(const char *name);
xcb_gcontext_t                 x_load_gc(const char *fg, const char *bg);

#endif
