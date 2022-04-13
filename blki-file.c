/* Copyright (C) 2022 Lars Brinkhoff <lars@nocrew.org>

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
#include <stdlib.h>

#include "dis.h"
#include "memory.h"

static void
read_blki (FILE *f, struct pdp10_memory *memory, int cpu_model)
{
  int start, length;
  word_t word;

  fprintf (output_file, "Hardware read-in, BLKI format:\n");
  word = get_word (f);
  start = (word + 1) & 0777777;
  length = (word >> 18) & 0777777;
  length = 01000000 - length;
  read_raw_region (f, memory, start, start + length);

  fprintf (output_file, "Start instruction:\n");
  word = get_word_at (memory, start + length - 1);
  disassemble_word (NULL, word, -1, cpu_model);
}

static void
write_blki (FILE *f, struct pdp10_memory *memory)
{
  int start, end;
  word_t word = start - end;

  if (start_instruction > 0)
    word--;
  word &= 0777777;
  word <<= 18;
  word |= (start - 1) & 0777777;
  write_word (f, word);

  write_raw_region (f, memory, start, end);

  /* Write the start instruction last.  If not supplied, there better
     be one at the end of the previous region! */
  if (start_instruction > 0)
    write_word (f, start_instruction);
}

struct file_format blki_file_format = {
  "blki",
  read_blki,
  write_blki
};
