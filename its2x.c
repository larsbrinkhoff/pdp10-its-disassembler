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

int
main (int argc, char **argv)
{
  word_t word;
  FILE *f;

  input_word_format = &its_word_format;

  if (argc != 2)
    {
      fprintf (stderr, "Usage: %s <file>\n", argv[0]);
      exit (1);
    }

  f = fopen (argv[1], "rb");

  while ((word = get_word (f)) != -1)
    {
      putchar ((word >> 32) & 0x0f);
      putchar ((word >> 24) & 0xff);
      putchar ((word >> 16) & 0xff);
      putchar ((word >>  8) & 0xff);
      putchar ((word >>  0) & 0xff);
    }

  return 0;
}
