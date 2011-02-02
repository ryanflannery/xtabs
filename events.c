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

#include "events.h"

void
xevent_recv_buttonpress(xcb_button_press_event_t *e)
{
   size_t i, c = 0;

   if (SIG_QUIT) return;

   if ((int32_t)e->event_y > (int32_t) X.bar_height)
      return;

   for (i = clients_get_offset(); i < clients_get_size(); i++) {
      if ((int32_t)e->event_x < (int32_t)((i - clients_get_offset() + 1) * X.tab_width)) {
         c = i;
         break;
      }
   }

   client_focus(c);
   /* TODO: figure out why e->state is always 0.
    * TODO: eventually, right-click should close a window.
    * TODO: also remove hideous casting above
    */
}

void
xevent_recv_configure_notify(xcb_configure_notify_event_t *e)
{
   if (SIG_QUIT) return;

   if (e->window == X.window) {
      X.width  = e->width;
      X.height = e->height;

      xcb_free_pixmap(X.connection, X.bar);
      xcb_create_pixmap(X.connection, X.screen->root_depth, X.bar,
         X.window, X.width, X.height);

      clients_resize_all();
      REDRAW = true;
   }
}

void
xevent_recv_create_notify(xcb_create_notify_event_t *e)
{
   uint16_t mask = XCB_CW_EVENT_MASK;
   uint32_t values[1] = { XCB_EVENT_MASK_PROPERTY_CHANGE };
   size_t   c;

   if (SIG_QUIT) return;

   if (e->window != X.window) {
      xcb_unmap_window(X.connection, e->window);
      xcb_reparent_window(X.connection, e->window, X.window, 0, X.bar_height);
      xcb_map_window(X.connection, e->window);
      xcb_change_window_attributes(X.connection, e->window, mask, values);

      c = client_add(e->window);
      client_resize(c);
      REDRAW = true;
   }
}

void
xevent_recv_destroy_notify(xcb_destroy_notify_event_t *e)
{
   if (SIG_QUIT) return;

   client_remove(e->window);
   session_save();
   REDRAW = true;
}

void
xevent_recv_keypress(xcb_key_press_event_t *e)
{
   if (SIG_QUIT) return;

   /* TODO Still need to figure out keysym's in xcb. */
   const char *MODIFIERS[] = {
      "Shift", "Lock", "Ctrl", "Alt",
      "Mod2", "Mod3", "Mod4", "Mod5",
      "Button1", "Button2", "Button3", "Button4", "Button5"
   };
   uint32_t mask = e->state;
   size_t i;

   printf("k->detail = %d [%c]   Modifier: ", e->detail, e->detail);
   for (i = 0; i < 13 && mask; mask >>= 1, i++) {
      if (mask & 1) {
         printf("%s", MODIFIERS[i]);
      }
   }
   printf ("\n");
   /* XXX end test code */


   switch (e->detail) {
   case 43: /* 'h' */
   case 44: /* 'j' */
      client_prev(1);
      REDRAW = true;
      break;
   case 45: /* 'k' */
   case 46: /* 'l' */
      client_next(1);
      REDRAW = true;
      break;
   case 53: /* 'x' */
      SIG_QUIT = 1;
      break;
   case 57: /* 'n' */
      spawn(NULL);
      break;
   case 25: /* 'w' */
      session_save();
      break;
   }
}

void
xevent_recv_property_notify(xcb_property_notify_event_t *e)
{
   size_t i, c = 0;

   if (SIG_QUIT) return;

   if (X.window == e->window || (e->atom != WM_NAME && e->atom != WM_COMMAND))
      return;

   for (i = 0; i < clients_get_size(); i++) {
      if (client_get_window(i) == e->window)
         c = i;
   }

   /* Each if block after this should check for an atom type and return within
    * the block
    */

   if (e->atom == WM_NAME) {
      client_set_name(c, x_get_window_name(e->window));
      if (client_is_focused(c))
         x_set_window_name(client_get_name(c), X.window);

      REDRAW = true;
      return;
   }
   
   if (e->atom == WM_COMMAND) {
      client_set_command(c, x_get_command(e->window));
      session_save();
      return;
   }
}

void
xevent_send_kill(xcb_window_t w)
{
   /* This should kill windows (and their subs), without error, but doesn't
   xcb_destroy_subwindows(X.connection, w);
   */

   /* So should this
   xcb_destroy_window(X.connection, w);
   */
   
   /* And this */
   xcb_kill_client(X.connection, w);

   /* All generate BadWindow errors from vimprobable2.  FML */
}

void
xevent_send_raise(xcb_window_t w)
{
   static const uint32_t values[] = { XCB_STACK_MODE_ABOVE };

   xcb_configure_window (X.connection, w, XCB_CONFIG_WINDOW_STACK_MODE,
         values);
}

void
xevent_send_resize(xcb_window_t w)
{
   uint16_t mask = XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
   uint32_t values[2] = { X.width, X.height - X.bar_height };

   xcb_configure_window(X.connection, w, mask, values);
}
