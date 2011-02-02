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

#include "session.h"

char *session_file = NULL;

void
session_load(const char *name)
{
   FILE  *f;
   char   line[2000];

   /* TODO this will be a setting.
    * TODO create dir if not exist
    */
   const char *tab_dir = "/home/ryan/.xtabs";
   if (asprintf(&session_file, "%s/%s", tab_dir, name) == 0)
      err(1, "%s: failed to create session file name.", __FUNCTION__);

   if ((f = fopen(session_file, "r")) == NULL)
      return;

   while (fgets(line, sizeof(line), f) != NULL) {
      line[strlen(line) - 1] = '\0';
      spawn(line);
   }

   fclose(f);
}

void
session_save()
{
   FILE  *f;
   size_t c;

   if ((f = fopen(session_file, "w")) == NULL)
      err(1, "%s: failed to save session to '%s'.", __FUNCTION__, session_file);

   for (c = 0; c < clients_get_size(); c++) {
      if (client_get_command(c) != NULL)
         fprintf(f, "%s\n", client_get_command(c));
   }

   fclose(f);
}
