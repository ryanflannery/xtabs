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

/*
 * TODO's
 * Major:
 *    1. figure out fatal IO error when exiting.
 *    2. add sessions
 *    3. figure out xembed stuff (?)
 *    4. create simple client api
 *    5. FIGURE OUT XCB ERROR HANDLING!  DAMNIT WHY ISN'T THIS DOCUMENTED!?!?
 *    6. XXX figure out how xcb parses string/text-list atom values.
 *           currently, my support for WM_COMMAND only works if it's a single
 *           string, which is non-standard.
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "session.h"
#include "clients.h"
#include "events.h"
#include "xtabs.h"
#include "xutil.h"

volatile sig_atomic_t REDRAW = false;
volatile sig_atomic_t SAVE_SESSION = false;
volatile sig_atomic_t SIG_QUIT = 0;

void signal_handler(int);
void draw_bar();

void
spawn(const char *cmd)
{
   const char *program = "vimprobable2";
   const char *argv[] = { program, "-e", X.str_window, NULL};

   switch (fork()) {
   case -1:
      err(1, "failed to fork");
   case 0:
      setsid();
      if (cmd == NULL)
         execvp(program, (char**)argv);
      else
         execvp(cmd, NULL);

      err(1, "failed to exec '%s'", program);
   }
}

void
draw_bar()
{
   /* TODO replace asprintf with snpritnf to a fixed pad */
   xcb_rectangle_t whole_window = { 0, 0, X.width, X.height };
   xcb_rectangle_t whole_tab = { 0, 0, X.tab_width, X.bar_height };
   xcb_gcontext_t  gc_fg, gc_bg;
   xcb_point_t     p[2];
   uint16_t        xoff = 0;
   int32_t         num_width;
   size_t          i;
   char           *num;

   xcb_poly_fill_rectangle(X.connection, X.bar, X.gc_bar_norm_bg, 1, &whole_window);

   for (i = clients_get_offset(); i < clients_get_size() && xoff <= X.width; i++) {

      if (client_is_focused(i)) {
         gc_fg = X.gc_bar_curr_fg;
         gc_bg = X.gc_bar_curr_bg;
      } else {
         gc_fg = X.gc_bar_norm_fg;
         gc_bg = X.gc_bar_norm_bg;
      }

      if (asprintf(&num, "%zd: ", i) == -1)
         err(1, "%s: asprintf(3) num failed", __FUNCTION__);

      num_width = x_get_strwidth(num);

      xcb_poly_fill_rectangle(X.connection, X.tab, gc_bg, 1, &whole_tab);
      xcb_image_text_8(X.connection,
                       strlen(num),
                       X.tab,
                       gc_fg,
                       X.font_padding + 1,
                       X.bar_height - (X.font_descent + X.font_padding + 1),
                       num);
      xcb_image_text_8(X.connection,
                       strlen(client_get_name(i)),
                       X.tab,
                       gc_fg,
                       X.font_padding + 1 + num_width,
                       X.bar_height - (X.font_descent + X.font_padding + 1),
                       client_get_name(i));
      xcb_poly_rectangle(X.connection, X.tab, X.gc_bar_border, 1, &whole_tab);
      xcb_copy_area(X.connection, X.tab, X.bar, X.gc_bar_norm_bg,
         0, 0, xoff, 0, X.tab_width, X.bar_height);

      xoff += X.tab_width;
      free(num);
   }

   p[0].x = xoff;
   p[0].y = 0;
   p[1].x = xoff;
   p[1].y = X.bar_height;
   xcb_poly_line(X.connection, XCB_COORD_MODE_ORIGIN, X.bar, X.gc_bar_border,
         2, p);

   xcb_copy_area(X.connection, X.bar, X.window, X.gc_bar_norm_bg,
      0, 0, 0, 0, X.width, X.height);
}

void
signal_handler(int sig)
{
   switch (sig) {
   case SIGHUP:
   case SIGINT:
   case SIGQUIT:
      SIG_QUIT = 1;
      break;
   case SIGCHLD:
      while(0 < waitpid(-1, NULL, WNOHANG));
      break;
   }
}
 
int main(void)
{
   xcb_generic_event_t *e;

   signal(SIGCHLD, signal_handler);
   signal(SIGHUP,  signal_handler);
   signal(SIGINT,  signal_handler);
   signal(SIGQUIT, signal_handler);

   x_init();
   clients_init();
   printf("%s\n", X.str_window);

   REDRAW = true;
   while (!SIG_QUIT && (e = xcb_wait_for_event(X.connection))) {

      switch (e->response_type & ~0x80) {
      case XCB_EXPOSE:
         REDRAW = true;
         break;
      case XCB_KEY_PRESS:
         xevent_recv_keypress((xcb_key_press_event_t*)e);
         break;
      case XCB_BUTTON_PRESS:
         xevent_recv_buttonpress((xcb_button_press_event_t*)e);
         break;
      case XCB_CONFIGURE_NOTIFY:
         xevent_recv_configure_notify((xcb_configure_notify_event_t*)e);
         break;
      case XCB_CREATE_NOTIFY:
         xevent_recv_create_notify((xcb_create_notify_event_t*)e);
         break;
      case XCB_DESTROY_NOTIFY:
         xevent_recv_destroy_notify((xcb_destroy_notify_event_t*)e);
         break;
      case XCB_PROPERTY_NOTIFY:
         xevent_recv_property_notify((xcb_property_notify_event_t*)e);
         break;
      }

      if (REDRAW) {
         draw_bar();
         xcb_flush(X.connection);
         REDRAW = false;
      }
      free(e);
   }

   clients_free();
   x_free();
   return 0;
}
