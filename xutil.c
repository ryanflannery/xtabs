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

#include "xutil.h"

xinfo X;

void
x_init()
{
   xcb_query_font_reply_t *font_reply;
   uint32_t                mask;
   uint32_t                values[2];
   char                   *font_name = "fixed";

   /* TODO Eventually these will be settings & storable */
   X.width = 100;
   X.height = 100;
   X.tab_width = 100;
   X.font_padding = 1;

   /* setup connection, screen, colormap */
   X.connection = xcb_connect(NULL,NULL);
   if (xcb_connection_has_error(X.connection))
      errx(1, "%s: failed to connect to display", __FUNCTION__);

   X.screen = xcb_setup_roots_iterator( xcb_get_setup(X.connection) ).data;
   X.colormap = X.screen->default_colormap;

   /* setup window and string-form of window-id */
   X.window = xcb_generate_id(X.connection);
   mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
   values[0] = X.screen->white_pixel;
   values[1] = XCB_EVENT_MASK_EXPOSURE
             | XCB_EVENT_MASK_KEY_PRESS
             | XCB_EVENT_MASK_BUTTON_PRESS
             | XCB_EVENT_MASK_STRUCTURE_NOTIFY
             | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
             | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT;

   xcb_create_window(X.connection, X.screen->root_depth,
      X.window, X.screen->root,
      0, 0, X.width, X.height, 1,
      XCB_WINDOW_CLASS_INPUT_OUTPUT,
      X.screen->root_visual,
      mask, values);

   if (asprintf(&X.str_window, "%d", X.window) == -1)
      errx(1, "failed to asprintf(3) window id");

   /* load font */
   X.font = xcb_generate_id(X.connection);
   xcb_open_font(X.connection, X.font, strlen(font_name), font_name);

   font_reply = xcb_query_font_reply(X.connection,
         xcb_query_font(X.connection, X.font),
         NULL);
   X.font_ascent  = font_reply->font_ascent;
   X.font_descent = font_reply->font_descent;
   X.bar_height = 2 + X.font_ascent + X.font_descent + 2 * X.font_padding;

   /* normal tab gc's */
   X.gc_bar_norm_fg = xcb_generate_id(X.connection);
   X.gc_bar_norm_bg = xcb_generate_id(X.connection);
   X.gc_bar_norm_fg = x_load_gc("gray60", "gray9");
   X.gc_bar_norm_bg = x_load_gc("gray9",  "gray9");

   /* current tab gc's */
   X.gc_bar_curr_fg = xcb_generate_id(X.connection);
   X.gc_bar_curr_bg = xcb_generate_id(X.connection);
   X.gc_bar_curr_fg = x_load_gc("red",   "black");
   X.gc_bar_curr_bg = x_load_gc("black", "black");

   /* border gc */
   X.gc_bar_border = xcb_generate_id(X.connection);
   X.gc_bar_border = x_load_gc("black", "black");

   /* bar & tab pixmap's */
   X.bar = xcb_generate_id(X.connection);
   X.tab = xcb_generate_id(X.connection);

   xcb_create_pixmap(X.connection, X.screen->root_depth, X.bar,
      X.window, X.width, X.bar_height);
   xcb_create_pixmap(X.connection, X.screen->root_depth, X.tab,
      X.window, X.tab_width, X.bar_height);

   xcb_map_window(X.connection, X.window);
   xcb_flush(X.connection);
}

void
x_free()
{
   xcb_close_font(X.connection, X.font);
   xcb_free_gc(X.connection, X.gc_bar_norm_fg);
   xcb_free_gc(X.connection, X.gc_bar_norm_bg);
   xcb_free_gc(X.connection, X.gc_bar_curr_fg);
   xcb_free_gc(X.connection, X.gc_bar_curr_bg);
   xcb_free_gc(X.connection, X.gc_bar_border);
   xcb_free_pixmap(X.connection, X.bar);
   xcb_free_pixmap(X.connection, X.tab);
   xcb_destroy_window(X.connection, X.window);
   xcb_disconnect(X.connection);
   free(X.str_window);
   xcb_flush(X.connection);
}

void
x_set_window_name(const char *name, xcb_window_t w)
{
   const char *def = "(no-name)";

   if (name == NULL)
      xcb_set_wm_name(X.connection, w, STRING, strlen(def), def);
   else
      xcb_set_wm_name(X.connection, w, STRING, strlen(name), name);
}

char*
x_get_window_name(xcb_window_t w)
{
   xcb_get_property_cookie_t c;
   xcb_get_text_property_reply_t reply;
   xcb_generic_error_t *err;
   char *name;

   c = xcb_get_text_property(X.connection, w, WM_NAME);
   if (xcb_get_text_property_reply(X.connection, c, &reply, &err) == 0)
      errx(1, "failed to get window property");

   name = strndup(reply.name, reply.name_len);
   xcb_get_text_property_reply_wipe(&reply);
   return name;
}

char*
x_get_command(xcb_window_t w)
{
   /* TODO This isn't working.  WM_COMMAND is stored as a list-o-strings.
    * Can't figure out with xcb how to get access to these.... (the below
    * only gives the first string, argv[0]).
    */
   xcb_get_property_cookie_t c;
   xcb_get_text_property_reply_t reply;
   xcb_generic_error_t *err;
   char *name;

   c = xcb_get_text_property(X.connection, w, WM_COMMAND);
   if (xcb_get_text_property_reply(X.connection, c, &reply, &err) == 0)
      errx(1, "failed to get window property");

   name = strndup(reply.name, reply.name_len);
   xcb_get_text_property_reply_wipe(&reply);
   return name;
}

int32_t
x_get_strwidth(const char *s)
{
   xcb_query_text_extents_reply_t *reply;
   int32_t w;

   reply = xcb_query_text_extents_reply(X.connection,
         xcb_query_text_extents(X.connection, X.font, strlen(s), (xcb_char2b_t*)s),
         NULL);
   if (!reply)
      errx(1, "xcb_query_text_extents failed");

   w = reply->overall_width;
   free(reply);

   return w;
}

xcb_alloc_color_reply_t*
x_load_color(uint16_t r, uint16_t g, uint16_t b)
{
   xcb_alloc_color_reply_t *c;

   c = xcb_alloc_color_reply(X.connection,
      xcb_alloc_color(X.connection, X.colormap, r, g, b),
      NULL);
   if (!c)
      errx(1, "failed to load color (r,g,b) = (%d,%d,%d)", r, g, b);

   return c;
}

xcb_alloc_named_color_reply_t*
x_load_strcolor(const char *name)
{
   xcb_alloc_named_color_reply_t *c;

   c = xcb_alloc_named_color_reply(X.connection,
      xcb_alloc_named_color(X.connection, X.colormap, strlen(name), name),
      NULL);
   if (!c)
      errx(1, "failed to parse color '%s'", name);

   return c;
}

xcb_gcontext_t
x_load_gc(const char *fg, const char *bg)
{
   xcb_alloc_named_color_reply_t *color_fg, *color_bg;
   xcb_gcontext_t                 gc;
   uint32_t                       mask;
   uint32_t                       values[4];

   color_fg = x_load_strcolor(fg);
   color_bg = x_load_strcolor(bg);

   gc = xcb_generate_id(X.connection);
   mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT | XCB_GC_GRAPHICS_EXPOSURES;
   values[0] = color_fg->pixel;
   values[1] = color_bg->pixel;
   values[2] = X.font;
   values[3] = 0;

   xcb_create_gc(X.connection, gc, X.screen->root, mask, values);

   free(color_fg);
   free(color_bg);

   return gc;
}
