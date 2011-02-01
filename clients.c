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

#include "clients.h"

typedef struct {
   char          *name;
   char          *command;
   xcb_window_t   window;
} client;

struct client_list_t {
   client  *cs;
   size_t   capacity;
   size_t   size;
   size_t   curr;
   size_t   offset;
};
struct client_list_t clients;


client*
client_geti(size_t c)
{
   if (c > clients.size)
      errx(1, "%s: index out-of-bounds (%zd,%zd).", __FUNCTION__, c, clients.size);

   return &(clients.cs[c]);
}

client*
client_getw(xcb_window_t w)
{
   size_t i;
   for (i = 0; i < clients.size; i++) {
      if (client_geti(i)->window == w)
         return client_geti(i);
   }
   errx(1, "%s: window not found.", __FUNCTION__);
}


void
clients_init()
{
   static const size_t init_size = 100;

   if ((clients.cs = calloc(init_size, sizeof(client))) == NULL)
      err(1, "%s: calloc(3) failed", __FUNCTION__);

   clients.capacity = init_size;
   clients.size = 0;
   clients.curr = 0;
   clients.offset = 0;
}

void
clients_free()
{
   size_t  i;
   client *c;

   for (i = 0; i < clients.size; i++) {
      c = client_geti(i);
      xevent_send_kill(c->window);
      if (c->name != NULL) free(c->name);
      if (c->name != NULL) free(c->command);
   }

   xcb_flush(X.connection);
   free(clients.cs);
   clients.capacity = 0;
   clients.size = 0;
   clients.curr = 0;
   clients.offset = 0;
}

void
clients_update_offset()
{
   int32_t start, end;

   for (clients.offset = 0;
        clients.offset < clients.size;
        clients.offset++) {
      client_get_xbounds(clients.curr, &start, &end);
      if (start >= 0 && end <= X.width)
         return;
   }
}

void
clients_resize_all()
{
   size_t i;
   for (i = 0; i < clients.size; i++)
      client_resize(i);
}

size_t clients_get_size()  { return clients.size; }
size_t clients_get_curr()  { return clients.curr; }
size_t clients_get_offset(){ return clients.offset; }


size_t
client_add(xcb_window_t w)
{
   size_t  new_capacity;
   client *new_list;
   client *c;

   if (clients.size == clients.capacity) {
      new_capacity = clients.capacity + 50;
      if ((new_list = realloc(clients.cs, new_capacity)) == NULL)
         err(1, "%s: reallocation failed (%zd).", __FUNCTION__, new_capacity);
      clients.capacity = new_capacity;
   }

   c = client_geti(clients.size++);
   c->window  = w;
   c->name    = NULL;
   c->command = NULL;
   client_focus(clients.size - 1);
   return clients.size - 1;
}

void
client_remove(xcb_window_t w)
{
   size_t i, c = clients.size;
   for (i = 0; i < clients.size; i++) {
      if (client_geti(i)->window == w)
         c = i;
   }

   if (c == clients.size)
      errx(1, "out-o-bounds in remove");

   for (i = c; i < clients.size; i++) {
      clients.cs[i] = clients.cs[i+1];
   }
   clients.size--;

   if (c == clients.curr && c > 0) {
      clients.curr--;
      client_focus(clients.curr);
   }

}

void
client_next(size_t n)
{
   clients.curr += n;
   clients.curr %= clients.size;
   client_focus(clients.curr);
}

void
client_prev(size_t n)
{
   n %= clients.size;
   if (n <= clients.curr)
      clients.curr -= n;
   else
      clients.curr = clients.size - n + clients.curr;

   client_focus(clients.curr);
}

void
client_resize(size_t c)
{
   xevent_send_resize(client_geti(c)->window);
}

void
client_focus(size_t c)
{
   int32_t start, end;

   xevent_send_raise(client_geti(c)->window);
   x_set_window_name(client_geti(c)->name, X.window);
   client_get_xbounds(c, &start, &end);
   clients.curr = c;
   if (start < 0 || end > X.width)
      clients_update_offset();

   REDRAW = true;
}

bool
client_is_focused(size_t c)
{
   return c == clients.curr;
}

void
client_set_window(size_t c, xcb_window_t w)
{
   client_geti(c)->window = w;
}

void
client_set_name(size_t i, const char *name)
{
   client *c = client_geti(i);
   if (c->name != NULL)
      free(c->name);

   if ((c->name = strdup(name)) == NULL)
      err(1, "%s: strdup failed.", __FUNCTION__);
}

void
client_set_command(size_t i, const char *command)
{
   client *c = client_geti(i);
   if (c->command != NULL)
      free(c->command);

   if ((c->command = strdup(command)) == NULL)
      err(1, "%s: strdup failed.", __FUNCTION__);
}

void
client_get_xbounds(size_t c, int32_t *start, int32_t *end)
{
   *start = (c - clients.offset) * X.tab_width;
   *end   = *start + X.tab_width;
}

xcb_window_t
client_get_window(size_t c)
{
   return client_geti(c)->window;
}

const char *
client_get_name(size_t c)
{
   static const char *def = "(no-name)";
   if (client_geti(c)->name == NULL)
      return def;
   else
      return client_geti(c)->name;
}

const char *
client_get_command(size_t c)
{
   return client_geti(c)->command;
}

