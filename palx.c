/* Copyright (C) 2018 Lars Brinkhoff <lars@nocrew.org>

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
#include <unistd.h>
#include <string.h>

#include "dis.h"

static void
usage (const char *x)
{
  fprintf (stderr, "Usage: %s -W<format>\n", x);
  exit (1);
}

static int
pdp11_in (FILE *f)
{
  word_t w1, w2;

  w1 = get_word (f);
  w2 = get_word (f);
  if (w1 == -1 || w2 == -1)
    exit (0);

  return (w1 & 0377) | ((w2 & 0377) << 8);
}

static void
pdp11_out (FILE *f, int word)
{
  fputc (word & 0377, f);
  fputc ((word >> 8) & 0377, f);
}

static void
block (FILE *in, FILE *out)
{
  int c, i, first, count, address;

  for (first = 0; ((first = pdp11_in (in)) & 0377) != 1; )
    ;
  
  count = pdp11_in (in);
  address = pdp11_in (in);

  pdp11_out (out, first);
  pdp11_out (out, count);
  pdp11_out (out, address);

  if (count == 6)
    exit (0);

  for (i = 6; i < count; i++)
    {
      c = get_word (in);
      fputc (c & 0377, out);
    }

  c = get_word (in);
  fputc (c & 0377, out);
}

int
main (int argc, char **argv)
{
  int opt;

  file_36bit_format = FORMAT_ITS;

  while ((opt = getopt (argc, argv, "W:")) != -1)
    {
      switch (opt)
	{
	case 'W':
	  if (parse_word_format (optarg))
	    usage (argv[0]);
	  break;
	default:
	  usage (argv[0]);
	}
    }

  if (optind != argc)
    usage ("optind");

  for (;;)
    block (stdin, stdout);

  return 0;
}
