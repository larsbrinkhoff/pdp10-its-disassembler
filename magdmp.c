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

int
main (int argc, char **argv)
{
  word_t *buffer;
  word_t word;
  FILE *f;
  int eof = 0;
  int files = 0;
  int words = 0;

  if (argc != 2)
    {
      fprintf (stderr, "Usage: %s <file>\n", argv[0]);
      exit (1);
    }

  f = fopen (argv[1], "rb");

  for (;;)
    {
      int n;
      n = get_9track_record (f, &buffer);
      words += n;
      if (n == 0)
	{
          files++;
          if (files & 1)
            {
              fprintf (stderr, " %d words\n", words);
            }
          words = 0;
	  if (eof)
	    exit (0);
	  eof = 1;
	}
      else
	{
	  char ascii[8];
	  eof = 0;
          if (files == 0)
            {
              fprintf (stderr, "Boot record:");
            }
          if (files & 1)
            {
              fprintf (stderr, "File %d: ", (files >> 1) + 1);
              sixbit_to_ascii (buffer[0], ascii);
              fprintf (stderr, "%s ", ascii);
              sixbit_to_ascii (buffer[1], ascii);
              fprintf (stderr, "%s ", ascii);
              sixbit_to_ascii (buffer[2], ascii);
              fprintf (stderr, "%s", ascii);
            }
          free (buffer);
	}
    }

  return 0;
}
