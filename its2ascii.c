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

extern word_t get_its_word (FILE *f);
extern word_t write_aa_word (FILE *f, word_t);

int
main (int argc, char **argv)
{
  word_t word;

  if (argc != 1)
    {
      fprintf (stderr, "Usage: %s < infile > outfile\n", argv[0]);
      exit (1);
    }

  file_36bit_format = FORMAT_ITS;

  while ((word = get_its_word (stdin)) != -1)
    {
      write_aa_word (stdout, word);
    }

  return 0;
}
