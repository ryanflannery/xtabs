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
 *    1. cleanup client & event chunks
 *    1.1   -> split up into different files
 *    2. add sessions
 *    3. figure out xembed stuff
 *    4. create client api
 *    5. FIGURE OUT XCB ERROR HANDLING! (and, of course, use it)
 */

#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_atom.h>

#include <sys/types.h> /* for fork */
#include <sys/wait.h> /* for waitpid */
#include <unistd.h> /* for fork */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <err.h>

volatile sig_atomic_t REDRAW = false;
volatile sig_atomic_t SIG_QUIT = 0;
void signal_handler(int);

/****************************************************************************/

typedef struct {
   char          *name;
   char          *command;
   xcb_window_t   window;
} client;

struct client_list_t {
   client  *clients;
   size_t   capacity;
   size_t   size;
   size_t   curr;
   size_t   offset;
};
struct client_list_t client_list;

void   client_list_init();
void   client_list_free();
size_t client_list_add(xcb_window_t w);
void   client_list_remove(xcb_window_t w);
void   client_list_next(size_t n);
void   client_list_prev(size_t n);
void   client_list_update_offset();
void   client_resize(size_t c);
void   client_list_resize_all();
void   client_focus(size_t c);
void   client_get_xbounds(size_t c, int32_t *start, int32_t *end);

/****************************************************************************/
void xevent_recv_configure_notify(xcb_configure_notify_event_t *e);
void xevent_recv_create_notify(xcb_create_notify_event_t *e);
void xevent_recv_destroy_notify(xcb_destroy_notify_event_t *e);
void xevent_recv_keypress(xcb_key_press_event_t *e);
void xevent_recv_buttonpress(xcb_button_press_event_t *e);
void xevent_recv_property_notify(xcb_property_notify_event_t *e);
void xevent_send_raise(client c);
void xevent_send_kill(client c);

/****************************************************************************/
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
xinfo X;


void     x_init();
void     x_free();
void     x_set_window_name(const char *name);
char*    x_get_window_name(xcb_window_t w);
char*    x_get_command(xcb_window_t w);
int32_t  x_get_strwidth(const char *s);
xcb_alloc_color_reply_t*       x_load_color(uint16_t r, uint16_t g, uint16_t b);
xcb_alloc_named_color_reply_t* x_load_strcolor(const char *name);
xcb_gcontext_t                 x_load_gc(const char *fg, const char *bg);


void
x_init()
{
   xcb_query_font_reply_t *font_reply;
   uint32_t                mask;
   uint32_t                values[2];
   char                   *font_name = "fixed";

   /* TODO */
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

void
x_set_window_name(const char *name)
{
   xcb_set_wm_name(X.connection, X.window, STRING, strlen(name), name);
}

char*
x_get_command(xcb_window_t w)
{
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


/****************************************************************************/

void
client_list_init()
{
   static const size_t init_size = 100;

   if ((client_list.clients = calloc(init_size, sizeof(client))) == NULL)
      err(1, "%s: calloc(3) failed", __FUNCTION__);

   client_list.capacity = init_size;
   client_list.size = 0;
   client_list.curr = 0;
   client_list.offset = 0;
}

void
client_list_free()
{
   size_t i;

   for (i = 0; i < client_list.size; i++) {
      xevent_send_kill(client_list.clients[i]);
      free(client_list.clients[i].name);
   }

   xcb_flush(X.connection);
   free(client_list.clients);
   client_list.capacity = 0;
   client_list.size = 0;
   client_list.curr = 0;
   client_list.offset = 0;
}

size_t
client_list_add(xcb_window_t w)
{
   client_list.clients[client_list.size].name = x_get_window_name(w);
   client_list.clients[client_list.size].window = w;
   client_list.size++;
   client_focus(client_list.size - 1);
   return client_list.size - 1;
}

void
client_list_remove(xcb_window_t w)
{
   size_t i, c = client_list.size;
   for (i = 0; i < client_list.size; i++) {
      if (client_list.clients[i].window == w)
         c = i;
   }

   if (c == client_list.size)
      errx(1, "out-o-bounds in remove");

   for (i = c; i < client_list.size; i++) {
      client_list.clients[i] = client_list.clients[i+1];
   }
   client_list.size--;

   if (c == client_list.curr && c > 0) {
      client_list.curr--;
      client_focus(client_list.curr);
   }

}

void
client_list_next(size_t n)
{
   client_list.curr += n;
   client_list.curr %= client_list.size;
   client_focus(client_list.curr);
}

void
client_list_prev(size_t n)
{
   n %= client_list.size;
   if (n <= client_list.curr)
      client_list.curr -= n;
   else
      client_list.curr = client_list.size - n + client_list.curr;

   client_focus(client_list.curr);
}

void
client_list_update_offset()
{
   int32_t start, end;

   for (client_list.offset = 0; client_list.offset < client_list.size; client_list.offset++) {
      client_get_xbounds(client_list.curr, &start, &end);
      if (start >= 0 && end <= X.width)
         break;
   }
}

void
client_focus(size_t c)
{
   int32_t start, end;
   size_t  i;

   for (i = 0; i < client_list.size; i++) {
      if (client_list.clients[i].window == client_list.clients[c].window) {
         client_list.curr = i;
         xevent_send_raise(client_list.clients[i]);
         x_set_window_name(client_list.clients[i].name);
         client_get_xbounds(i, &start, &end);
         if (start < 0 || end > X.width)
            client_list_update_offset();

         REDRAW = true;
      }
   }
}

void
client_get_xbounds(size_t c, int32_t *start, int32_t *end)
{
   *start = (c - client_list.offset) * X.tab_width;
   *end   = *start + X.tab_width;
}

void
client_resize(size_t c)
{
   uint16_t mask = XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
   uint32_t values[2] = { X.width, X.height - X.bar_height };

   xcb_configure_window(X.connection, client_list.clients[c].window,
      mask, values);
   /* TODO: replace with call to xevent_send_configure_notify */
}

void
client_list_resize_all()
{
   size_t i;
   for (i = 0; i < client_list.size; i++)
      client_resize(i);
}


/****************************************************************************/

void spawn();

void
spawn()
{
   const char *program = "/home/ryan/Downloads/vimprobable2/vimprobable/vimprobable2";
   const char *argv[] = { program, "-e", X.str_window, NULL};

   switch (fork()) {
   case -1:
      err(1, "failed to fork");
   case 0:
      setsid();
      execvp(program, (char**)argv);
      err(1, "failed to exec '%s'", program);
   }
}


/****************************************************************************/


void
xevent_recv_configure_notify(xcb_configure_notify_event_t *e)
{
   if (e->window == X.window) {
      X.width  = e->width;
      X.height = e->height;

      xcb_free_pixmap(X.connection, X.bar);
      xcb_create_pixmap(X.connection, X.screen->root_depth, X.bar,
         X.window, X.width, X.height);

      client_list_resize_all();
      REDRAW = true;
   }
}

void
xevent_send_raise(client c)
{
   static const uint32_t values[] = { XCB_STACK_MODE_ABOVE };

   xcb_configure_window (X.connection, c.window, XCB_CONFIG_WINDOW_STACK_MODE,
      values);
}

void
xevent_send_kill(client c)
{
   xcb_kill_client(X.connection, c.window);
}

void
xevent_recv_create_notify(xcb_create_notify_event_t *e)
{
   uint16_t mask = XCB_CW_EVENT_MASK;
   uint32_t values[1] = { XCB_EVENT_MASK_PROPERTY_CHANGE };
   size_t   c;

   if (e->window != X.window) {
      xcb_unmap_window(X.connection, e->window);
      xcb_reparent_window(X.connection, e->window, X.window, 0, X.bar_height);
      xcb_map_window(X.connection, e->window);
      xcb_change_window_attributes(X.connection, e->window, mask, values);

      c = client_list_add(e->window);
      client_resize(c);
      REDRAW = true;
   }
}

void
xevent_recv_destroy_notify(xcb_destroy_notify_event_t *e)
{
   client_list_remove(e->window);
   REDRAW = true;
}

void
xevent_recv_property_notify(xcb_property_notify_event_t *e)
{
   size_t i, c = 0;

   if (X.window == e->window || (e->atom != WM_NAME && e->atom != WM_COMMAND))
      return;

   for (i = 0; i < client_list.size; i++) {
      if (client_list.clients[i].window == e->window)
         c = i;
   }

   if (e->atom == WM_NAME) {
      free(client_list.clients[c].name);
      client_list.clients[c].name = x_get_window_name(e->window);
      if (c == client_list.curr)
         x_set_window_name(client_list.clients[c].name);

      REDRAW = true;
   } else if (e->atom == WM_COMMAND) {
printf("command: '%s'\n", x_get_command(e->window));fflush(stdout);
   }
}

void
xevent_recv_buttonpress(xcb_button_press_event_t *e)
{
   size_t c = 0;
   int16_t i;

   if (e->event_y > X.bar_height)
      return;

   for (i = client_list.offset; i < (int16_t)client_list.size; i++) {
      if (e->event_x < (i - (int)client_list.offset + 1) * (int)X.tab_width) {
         c = i;
         break;
      }
   }

   client_focus(c);
   /* TODO: figure out why e->state is always 0.
    * TODO: eventually, right-click should close a window.
    */
}

void
xevent_recv_keypress(xcb_key_press_event_t *e)
{
   /* XXX BEGIN DIAGNOSTIC STUFF */
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
   /* XXX END DIAGNOSTIC STUFF */


   switch (e->detail) {
   case 43: /* 'h' */
   case 44: /* 'j' */
      client_list_prev(1);
      REDRAW = true;
      break;
   case 45: /* 'k' */
   case 46: /* 'l' */
      client_list_next(1);
      REDRAW = true;
      break;
   case 53: /* 'x' */
      SIG_QUIT = 1;
      break;
   case 57: /* 'n' */
      spawn();
      break;
   }
}


/****************************************************************************/

void
draw_bar()
{
   xcb_rectangle_t whole_window = { 0, 0, X.width, X.height };
   xcb_rectangle_t whole_tab = { 0, 0, X.tab_width, X.bar_height };
   xcb_gcontext_t  gc_fg, gc_bg;
   xcb_point_t     p[2];
   int16_t         xoff = 0;
   int32_t         num_width;
   size_t          i;
   char           *num;

   xcb_poly_fill_rectangle(X.connection, X.bar, X.gc_bar_norm_bg, 1, &whole_window);

   for (i = client_list.offset; i < client_list.size && xoff <= X.width; i++) {
      gc_fg = i == client_list.curr ? X.gc_bar_curr_fg : X.gc_bar_norm_fg;
      gc_bg = i == client_list.curr ? X.gc_bar_curr_bg : X.gc_bar_norm_bg;

      if (asprintf(&num, "%zd: ", i) == -1)
         err(1, "asprintf(3) num failed");

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
                       strlen(client_list.clients[i].name),
                       X.tab,
                       gc_fg,
                       X.font_padding + 1 + num_width,
                       X.bar_height - (X.font_descent + X.font_padding + 1),
                       client_list.clients[i].name);
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
   client_list_init();
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

   client_list_free();
   x_free();
   return 0;
}
