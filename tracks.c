/* Copyright (C) 2019 Lars Brinkhoff <lars@nocrew.org>

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
  int eof = 0;

  if (argc != 1)
    {
      fprintf (stderr, "Usage: %s < input > output\n", argv[0]);
      exit (1);
    }

  for (;;)
    {
      int n;
      n = get_9track_record (stdin, &buffer);
      fprintf (stderr, "record: %d words\n", n);
      if (n == 0)
	{
          write_7track_record (stdout, buffer, 0);
	  if (eof)
	    exit (0);
	  eof = 1;
	}
      else
	{
          write_7track_record (stdout, buffer, n);
          free (buffer);
	}
    }

  return 0;
}
