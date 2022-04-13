/* Copyright (C) 2021 Lars Brinkhoff <lars@nocrew.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* This understands words as octal numbers, one word per line. */

#include <stdio.h>
#include <string.h>

#include "dis.h"

static word_t
get_oct_word (FILE *f)
{
  static char line[100];
  word_t word;
  char *p;
  int i;

  for (;;)
    {
    next:
      p = fgets (line, sizeof line, f);
      if (p == NULL)
        return -1;

      while (strchr (" \t", *p))
        p++;

      word = 0;
      for (i = 0; i < 12; i++)
        {
          if (!strchr ("01234567", *p))
            goto next;
          word <<= 3;
          word += *p++ - '0';
        }

      if (strchr ("01234567", *p))
        goto next;

      return word;
    }
}

static void
write_oct_word (FILE *f, word_t word)
{
  fprintf (f, "%012llo\n", word & 0777777777777LL);
}

struct word_format oct_word_format = {
  "oct",
  get_oct_word,
  NULL,
  NULL,
  write_oct_word,
  NULL
};
