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

#include <stdio.h>
#include "dis.h"
#include "memory.h"

/* An Atari DOS binary file is formatted into blocks.  The file begins
with two FF bytes, and then each block header has start and end
address (inclusive). */

static void
out_8 (FILE *f, int word)
{
  fputc (word & 0xFF, f);
}

static void
out_16 (FILE *f, int word)
{
  out_8 (f, word & 0xFF);
  out_8 (f, word >> 8);
}

static void
write_atari (FILE *f, struct pdp10_memory *memory)
{
  int start, length;
  int i, j;

  out_16 (f, 0xFFFF);

  for (i = 0; i < memory->areas; i++)
    {
      start = memory->area[i].start;
      length = memory->area[i].end - start;
      out_16 (f, start);
      out_16 (f, start + length - 1);
      for (j = start; j < start + length; j++)
        out_8 (f, get_word_at (memory, j));
    }
}

struct file_format atari_file_format = {
  "atari",
  NULL,
  write_atari
};
