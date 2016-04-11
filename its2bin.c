/* Copyright (C) 2013 Lars Brinkhoff <lars@nocrew.org>

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

extern word_t get_its_word (FILE *f);

int
main (int argc, char **argv)
{
  word_t word1, word2;
  FILE *f;

  if (argc != 2)
    {
      fprintf (stderr, "Usage: %s <file>\n", argv[0]);
      exit (1);
    }

  f = fopen (argv[1], "rb");

  while ((word1 = get_its_word (f)) != -1)
    {
      putchar ((word1 >> 28) & 0xff);
      putchar ((word1 >> 20) & 0xff);
      putchar ((word1 >> 12) & 0xff);
      putchar ((word1 >>  4) & 0xff);

      word2 = get_its_word (f);
      if (word2 == -1)
	{
	  putchar ((word1 << 4) & 0xf0);
	}
      else
	{
	  putchar (((word1 << 4) & 0xf0) | ((word2 >> 32) & 0x0f));
	  putchar ((word2 >> 24) & 0xff);
	  putchar ((word2 >> 16) & 0xff);
	  putchar ((word2 >>  8) & 0xff);
	  putchar ((word2 >>  0) & 0xff);
	}
    }

  return 0;
}
