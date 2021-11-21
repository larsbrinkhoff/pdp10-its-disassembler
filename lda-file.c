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

int checksum;

static void
out_8 (FILE *f, int word)
{
  word &= 0377;
  checksum += word;
  fputc (word & 0377, f);
}

static void
out_16 (FILE *f, int word)
{
  out_8 (f, word);
  out_8 (f, word >> 8);
}


static void
write_lda (FILE *f, struct pdp10_memory *memory)
{
  int start, length;
  int i, j;

  for (i = 0; i < memory->areas; i++)
    {
      start = memory->area[i].start;
      length = memory->area[i].end - start;
      checksum = 0;
      out_16 (f, 1);
      out_16 (f, length + 6);
      out_16 (f, start);
      for (j = start; j < start + length; j++)
        out_8 (f, get_word_at (memory, j));
      out_8 (f, -checksum & 0377);
    }

  out_16 (f, 1);
  out_16 (f, length + 6);
  out_16 (f, start_instruction & 1 ? 0 : start_instruction);
}

struct file_format lda_file_format = {
  "lda",
  NULL,
  write_lda
};
