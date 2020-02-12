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
  word_t word;

  input_word_format = &its_word_format;
  output_word_format = &aa_word_format;

  if (argc != 1)
    {
      fprintf (stderr, "Usage: %s < infile > outfile\n", argv[0]);
      exit (1);
    }

  while ((word = get_word (stdin)) != -1)
    {
      write_word (stdout, word);
    }

  flush_word (stdout);
  return 0;
}
