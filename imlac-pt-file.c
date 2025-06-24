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

/* IMLAC papertape boostrap loader. */
static char loader[124] =
  "\002\032\027\340\047\277\077\360\204\001\027\303\200\006\047\377"
  "\077\350\047\376\177\330\027\315\000\000\077\350\247\376\077\331"
  "\067\376\067\377\027\315\077\350\167\277\204\001\027\346\000\000"
  "\377\377\037\320\200\010\157\277\004\004\200\004\047\277\227\331"
  "\002\061\147\374\047\361\012\032\377\375\047\361\200\011\027\302"
  "\037\311\200\011\077\360\006\003\006\003\006\002\077\360\227\350"
  "\037\357\002\032\205\000\027\362\005\000\027\364\002\051\227\360"
  "\004\040\027\370\002\033\227\360\027\370\000\044";

static int checksum = 0;

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
get_8 (FILE *f)
{
  int c = fgetc (f);
  if (c == EOF)
    {
      fprintf (stderr, "Unexpected end of input file.\n");
      exit (1);
    }
  return c;
}

static int
get_8_not_0 (FILE *f)
{
  for (;;)
    {
      int c = get_8 (f);
      if (c != 0)
        return c;
    }      
}

static int
get_16 (FILE *f)
{
  int data = (get_8 (f) << 8) | get_8 (f);
  update_sum (data);
  return data;
}

static void
read_imlac_pt (FILE *f, struct pdp10_memory *memory, int cpu_model)
{
  int length, address, data;
  word_t *core;
  int c, i;

  (void)cpu_model;
  start_instruction = 0;

  /* Discard block loader. */
  for (c = 0; c != 2;)
    c = get_8_not_0 (f);
  for (i = 0; i < 123; i++)
    c = get_8 (f);

  for (;;)
    {
      length = get_8_not_0 (f);
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
out_8 (FILE *f, int word)
{
  fputc (word & 0377, f);
}

static void
out_16 (FILE *f, int word)
{
  update_sum (word);
  out_8 (f, word >> 8);
  out_8 (f, word);
}

static void
blank (FILE *f, int n)
{
  int i;
  for (i = 0; i < n; i++)
    fputc (0, f);
}

static void
write_imlac_pt (FILE *f, struct pdp10_memory *memory)
{
  int start, length;
  int i, j;

  blank (f, 32);

  fprintf (stderr, "Loader: %lu\n", sizeof loader);
  for (i = 0; i < (int)sizeof loader; i++)
    fputc (loader[i], f);

  for (i = 0; i < memory->areas; i++)
    {
      start = memory->area[i].start;
      length = memory->area[i].end - start;
      fprintf (stderr, "Area: %04o %06o\n", start, length);

      while (length > 0)
        {
          int n = length;
          if (n > 0377) {
            if (n < 0777)
              n /= 2;
            else
              n = 0377;
          }
          length -= n;
          blank (f, 10);
          out_8 (f, n);
          out_16 (f, start);
          fprintf (stderr, "PT: %04o %06o\n", n, start);
          checksum = 0;
          for (j = 0; j < n; j++)
            out_16 (f, get_word_at (memory, start + j));
          out_16 (f, checksum);
          start += n;
        }
    }

  if (start_instruction != 0)
    {
      blank (f, 10);
      out_8 (f, 2);
      out_16 (f, 037713);
      checksum = 0;
      out_16 (f, 0113714); 
      out_16 (f, start_instruction & 037777);
      out_16 (f, checksum);
    }

  blank (f, 10);
  out_8 (f, 0377);
  out_16 (f, 0177777);
  blank (f, 10);
}

struct file_format imlac_pt_file_format = {
  "imlac-pt",
  read_imlac_pt,
  write_imlac_pt
};
