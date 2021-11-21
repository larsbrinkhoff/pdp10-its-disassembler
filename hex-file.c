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

static int get_hex (FILE *f)
{
  int c;

  c = fgetc (f);
  if (c == EOF)
    {
      fprintf (stderr, "Unexpected end of input file.\n");
      exit (1);
    }

  switch (c)
    {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      return c - '0';
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
      return c - 'A' + 10;
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
      return c - 'a' + 10;
    default:
      fprintf (stderr, "Bad hex digit: %c\n", c);
      exit (1);
    }
}

static int get_8 (FILE *f)
{
  int data = (get_hex (f) << 4) | get_hex (f);
  checksum += data;
  return data;
}

static int get_16 (FILE *f)
{
  return (get_8 (f) << 8) | get_8 (f);
}

static void
read_hex (FILE *f, struct pdp10_memory *memory, int cpu_model)
{
  int length, address, data;
  word_t *core;
  int c, i;

  (void)cpu_model;

  for (;;)
    {
      do
        {
          c = fgetc (f);
          if (c == EOF)
            return;
        }
      while (c != ';');

      checksum = 0;
      length = get_8 (f);
      address = get_16 (f);

      if (length == 0)
        return;

      core = malloc (length * sizeof (word_t));
      if (core == NULL)
        {
          fprintf (stderr, "Out of memory.\n");
          exit (1);
        }

      for (i = 0; i < length; i++)
        core[i] = get_8 (f);
      add_memory (memory, address, length, core);

      checksum &= 0xFFFF;
      data = checksum;
      if (data != get_16 (f))
        fprintf (stderr, "Bad checksum: %04X.\n", data);
    }
}

static void
out_hex (FILE *f, int word)
{
  word &= 0xF;
  if (word <= 9)
    word += '0';
  else
    word += 'A';
  fputc (word, f);
}

static void
out_8 (FILE *f, int word)
{
  out_hex (f, word >> 4);
  out_hex (f, word);
}

static void
out_16 (FILE *f, int word)
{
  out_8 (f, word >> 8);
  out_8 (f, word);
}

static void
write_hex (FILE *f, struct pdp10_memory *memory)
{
  int start, length;
  int i, j;

  for (i = 0; i < memory->areas; i++)
    {
      start = memory->area[i].start;
      length = memory->area[i].end - start;
      fputc (';', f);
      checksum = 0;
      out_8 (f, length);
      out_16 (f, start);
      for (j = start; j < start + length; j++)
        out_8 (f, get_word_at (memory, j));
      out_8 (f, checksum);
      fputc ('\n', f);
    }

  fputc (';', f);
  out_8 (f, 0);
  out_16 (f, 0);
  fputc ('\n', f);
}

struct file_format hex_file_format = {
  "hex",
  read_hex,
  write_hex
};
