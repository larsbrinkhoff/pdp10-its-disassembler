/* Copyright (C) 2017 Lars Brinkhoff <lars@nocrew.org>

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

#include <stdio.h>

#include "dis.h"

extern word_t get_core_word (FILE *f);

static int
get_byte (FILE *f)
{
  int c = fgetc (f);
  return c == EOF ? 0 : c;
}

static int get_reclen (FILE *f)
{
  return get_byte (f) |
    ((word_t)get_byte (f) << 8) |
    (get_byte (f) << 16) |
    (get_byte (f) << 24);
}

static int get_record (FILE *f, void *buffer)
{
  int i, x, reclen;
  word_t *p = buffer;

  reclen = get_reclen (f);
  if (reclen == 0)
    return 0;
  if (reclen % 5)
    {
      fprintf (stderr, "Not a CORE DUMP tape image.\n"
	       "reclen = %d\n", reclen);
      exit (1);
    }
  
  for (i = 0; i < (reclen / 5); i++)
    *p++ = get_core_word (f);

  if (reclen & 1)
    get_byte (f);

  x = get_reclen (f);
  if (x != reclen)
    {
      fprintf (stderr, "Error in tape image format.\n"
	       "%d != %d\n", reclen, x);
      exit (1);
    }

  return reclen / 5;
}

word_t buffer[5 * 1024];

int
main (int argc, char **argv)
{
  word_t word;
  FILE *f;
  int eof = 0;

  if (argc != 2)
    {
      fprintf (stderr, "Usage: %s <file>\n", argv[0]);
      exit (1);
    }

  f = fopen (argv[1], "rb");

  for (;;)
    {
      int n;
      n = get_record (f, buffer);
      fprintf (stderr, "record: %d words\n", n);
      if (n == 0)
	{
	  if (eof)
	    exit (0);
	  eof = 1;
	}
      else
	{
	  char ascii[8];
	  eof = 0;
	  sixbit_to_ascii (buffer[0], ascii);
	  fprintf (stderr, "%s ", ascii);
	  sixbit_to_ascii (buffer[1], ascii);
	  fprintf (stderr, "%s ", ascii);
	  sixbit_to_ascii (buffer[2], ascii);
	  fprintf (stderr, "%s\n", ascii);
	}
    }

  return 0;
}
