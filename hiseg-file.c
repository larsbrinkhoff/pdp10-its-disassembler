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
#include "jobdat.h"

static void
read_hiseg (FILE *f, struct pdp10_memory *memory, int cpu_model)
{
  fprintf (output_file, "Hiseg format\n");

  read_raw_at (f, memory, JBHGH);
  set_word_at (memory, JBSA, get_word_at (memory, JBHGH+JBHSA));
  set_word_at (memory, JB41, get_word_at (memory, JBHGH+JBH41));
  set_word_at (memory, JBCOR, get_word_at (memory, JBHGH+JBHCR));
  set_word_at (memory, JBHRL, get_word_at (memory, JBHGH+JBHRN) & 0777777000000LL);
  set_word_at (memory, JBREN, get_word_at (memory, JBHGH+JBHRN) & 0777777);
  set_word_at (memory, JBVER, get_word_at (memory, JBHGH+JBHVR));
  dec_info (memory, 0254000, -1, cpu_model);
}

static void
write_hiseg (FILE *f, struct pdp10_memory *memory)
{
  set_word_at (memory, JBHSA, start_instruction & 0777777);
  set_word_at (memory, JBHGA, (word_t)JBHGH << 9);
  write_raw_at (f, memory, JBHGH);
}

struct file_format hiseg_file_format = {
  "hiseg",
  read_hiseg,
  write_hiseg
};
