/* Copyright (C) 2024 Lars Brinkhoff <lars@nocrew.org>

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

/*
  File format to output SIMH deposit commands.

  Converts a core image to file to a series of SIMH deposit commands.
  If there is a start address, there will also be a GO command at the end.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dis.h"
#include "memory.h"

static void
write_location (FILE *f, struct pdp10_memory *memory, int address)
{
  word_t data = get_word_at (memory, address);
  if (data >= 0)
    {
      data &= 0777777777777LL;
      fprintf (f, "d %06o %012llo\n", address, data);
    }
}

static void
write_simh (FILE *f, struct pdp10_memory *memory)
{
  int i;

  /* Memory contents, as deposit commands. */
  for (i = 0; i <= 0777777; i++)
    write_location (f, memory, i);

  /* Start. */
  if (start_instruction <= 0)
    ;
  else if (((start_instruction & 0777000000000LL) == JRST) ||
	   start_instruction <= 0777777)
    fprintf (f, "go %06llo\n", start_instruction & 0777777);
  else
    fprintf (f, ";execute %012llo\n", start_instruction);
}

struct file_format simh_file_format = {
  "simh",
  NULL,
  write_simh
};
