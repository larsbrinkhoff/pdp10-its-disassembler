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

/* Given a memory range (inclusive), list all references to each
location in that range.  If a reference is from the left half, put
parentheses around the address.  If a reference isn't in ascending
order, put exclamation marks after. */

#include <stdio.h>
#include <getopt.h>

#include "dis.h"
#include "memory.h"
#include "opcode/pdp10.h"

static int ascending = 0;

static int
reference (const char *format, int first, int address)
{
  printf (format, address);
  if (first)
    {
      if (address > ascending)
        ascending = address;
      else
        printf ("!!!");
    }
  return 0;
}

static void
check (struct pdp10_memory *memory, int address)
{
  int first = 1;
  word_t data;
  int i;

  for (i = 0; i <= 0777777; i++)
    {
      data = get_word_at (memory, i);
      if (data == -1)
        continue;
      if ((data & 0777777) == address)
        first = reference (" %06o", first, i);
      if (((data >> 18) & 0777777) == address)
        first = reference (" (%06o)", first, i);
    }
}

static void
analyse_consta (struct pdp10_memory *memory, int start, int end)
{
  word_t data;
  int i;
  for (i = start; i <= end; i++)
    {
      data = get_word_at (memory, i);
      if (data == -1)
        continue;
      printf ("%06o/%012llo: ", i, data);
      check (memory, i);
      printf ("\n");
    }
}

static void
usage (char **argv)
{
  fprintf (stderr, "Usage: %s <start> <end> [file]\n\n", argv[0]);
  exit (1);
}

int
main (int argc, char **argv)
{
  int opt;
  int cpu_model = PDP10_KA10_ITS;
  struct pdp10_memory memory;
  FILE *file = stdin;
  int start, end;
  input_file_format = &sblk_file_format;

  while ((opt = getopt (argc, argv, "F:W:")) != -1)
    {
      switch (opt)
	{
        case 'F':
          if (parse_input_file_format (optarg))
            usage (argv);
          break;
        case 'W':
          if (parse_input_word_format (optarg))
            usage (argv);
          break;
        }
    }

  if (optind < argc)
    start = strtoul (argv[optind++], NULL, 8);
  else
    usage (argv);

  if (optind < argc)
    end = strtoul (argv[optind++], NULL, 8);
  else
    usage (argv);

  if (optind < argc)
    file = fopen (argv[optind++], "rb");

  if (optind != argc)
    usage (argv);

  output_file = fopen ("/dev/null", "w");
  init_memory (&memory);
  input_file_format->read (file, &memory, cpu_model);
  analyse_consta (&memory, start, end);
}
