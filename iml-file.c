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

static int checksum = 0;

/* "Special TTY" boostrap loader. */
static char *loader =
  "@BAJH@@IBGOLBGOMCONGCONGH@@FBGONCOMOGOOHAGLL@@@@BGOOCOMOJGOOCOMHCGOO\r\n"
  "CGONAGLMCOMOGGOLHD@AAGL@@@@@@@@@H@@HFOOL@D@DH@@DBGOLIGMH@@@@H@@IBGOM\r\n"
  "CONGCONGCONGCONGIGMO@@@@@DB@AGNHH@@I@BAKBGKODOOIGOOJAGNHFGKODOOKBOOM\r\n"
  "@F@C@F@AEGOMBGOMIGNGOOOO@@G@@@D@@@@O@@@@@@@@@@@@@@@@OOL@\r\n\r\n";

static void
update_sum (int word)
{
  checksum += word;
  if (checksum & 0200000)
    {
      checksum++;
      checksum &= 0177777;
    }
}

static int
get_4 (FILE *f)
{
  for (;;)
    {
      int c = fgetc (f);
      if (c == EOF)
        {
          fprintf (stderr, "Unexpected end of input file.\n");
          exit (1);
        }

      if ((c & 0160) == 0100)
        return c & 017;
    }
}

static int
get_8 (FILE *f)
{
  return (get_4 (f) << 4) | get_4 (f);
}

static int
get_16 (FILE *f)
{
  int data = (get_8 (f) << 8) | get_8 (f);
  update_sum (data);
  return data;
}

static void
read_iml (FILE *f, struct pdp10_memory *memory, int cpu_model)
{
  int length, address, data;
  word_t *core;
  int i;

  (void)cpu_model;
  start_instruction = 0;

  /* Discard block loader. */
  for (i = 0; i < 65; i++)
    get_16 (f);

  for (;;)
    {
      length = get_8 (f);
      address = get_16 (f);

      /* All ones signals end of data. */
      if (address == 0177777)
        {
          /* Search for a start address at 037714 modulo 4K. */
          address = 037714;
          while (address >= 07714)
            {
              start_instruction = get_word_at (memory, address);
              if (start_instruction == -1)
                start_instruction = 0;
              else
                break;
              address -= 4096;
            }
          return;
        }

      core = malloc (length * sizeof (word_t));
      if (core == NULL)
        {
          fprintf (stderr, "Out of memory.\n");
          exit (1);
        }

      checksum = 0;
      for (i = address; i < address + length; i++)
        core[i] = get_16 (f);
      add_memory (memory, address, length, core);

      data = checksum;
      if (data != get_16 (f))
        fprintf (stderr, "Bad checksum: %04X.\n", data);
    }
}

static void
out_4 (FILE *f, int word)
{
  fputc ((word & 017) + 0100, f);
}

static void
out_8 (FILE *f, int word)
{
  out_4 (f, word >> 4);
  out_4 (f, word);
}

static void
out_16 (FILE *f, int word)
{
  update_sum (word);
  out_8 (f, word >> 8);
  out_8 (f, word);
}

static void
write_iml (FILE *f, struct pdp10_memory *memory)
{
  int start, length;
  int i, j;

  for (i = 0; i < (int)sizeof loader; i++)
    fputc (loader[i], f);

  for (i = 0; i < memory->areas; i++)
    {
      start = memory->area[i].start;
      length = memory->area[i].end - start;
      out_8 (f, length);
      out_16 (f, start);

      checksum = 0;
      for (j = 0; j < length; j++)
        out_16 (f, get_word_at (memory, start + j));
      out_16 (f, checksum);
    }

  if (start_instruction != 0)
    {
      /* The special TTY loader will end up executing a jump indrected
         through 37714. */
      out_8 (f, 2);
      out_16 (f, 037713);
      checksum = 0;
      out_16 (f, 0113714); 
      out_16 (f, start_instruction & 037777);
      out_16 (f, checksum);
    }

  out_8 (f, 0377);
  out_16 (f, 0177777);
  fprintf (f, "\r\n");
}

struct file_format iml_file_format = {
  "iml",
  read_iml,
  write_iml
};
