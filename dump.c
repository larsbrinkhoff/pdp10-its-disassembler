/* Copyright (C) 2026 Lars Brinkhoff <lars@nocrew.org>

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
#include <getopt.h>

#include "dis.h"
#include "memory.h"

static int
octal (const char *address)
{
  char *end;
  unsigned long x = strtoul(address, &end, 8);
  if (*end != 0)
    {
      fprintf (stderr, "Invalid address: %s\n", address);
      exit (1);
    }
  return x;
}

static void
usage (char **argv)
{
  fprintf (stderr, "Usage: %s [-F<input file format>] [-W<input word format>] [-s<start address>] [-e<end address>]\n"
                   "   [-O<output file format>] [-X<output word format>] [<files...>]\n\n", argv[0]);
  usage_file_format ();
  usage_word_format ();
  exit (1);
}

int
main (int argc, char **argv)
{
  struct pdp10_memory memory;
  FILE *file;
  int opt;

  output_file = stderr;
  file = stdin;

  while ((opt = getopt (argc, argv, "e:s:W:X:F:O:")) != -1)
    {
      switch (opt)
        {
        case 'W':
          if (parse_input_word_format (optarg))
            usage (argv);
          break;
        case 'X':
          if (parse_output_word_format (optarg))
            usage (argv);
          break;
        case 'F':
          if (parse_input_file_format (optarg))
            usage (argv);
          break;
        case 'O':
          if (parse_output_file_format (optarg))
            usage (argv);
          break;
        case 's':
          output_file_image_start_address = octal (optarg);
          break;
        case 'e':
          output_file_image_end_address = octal (optarg);
          break;
        default:
          usage (argv);
        }
    }

  if (!output_file_format)
    usage (argv);

  if (output_file_format->write == NULL)
    {
      fprintf (stderr, "File format \"%s\" not supported for output\n",
               output_file_format->name);
      exit (1);
    }

  init_memory (&memory);

  while (optind < argc)
    {
      fprintf (stderr, "File: %s\n", argv[optind]);
      file = fopen (argv[optind], "rb");
      if (file == NULL)
        {
          fprintf (stderr, "Error opening input file %s\n", argv[optind]);
          exit (1);
        }
      optind++;

      if (!input_file_format)
        guess_input_file_format (file);

      input_file_format->read (file, &memory, 0);
      fclose (file);
    }

  fprintf (stderr, "Core image address range: %o - %o\n",
           memory.area[0].start, memory.area[memory.areas-1].end);
  fprintf (stderr, "Writing file format: %s\n", output_file_format->name);

  output_file_format->write (stdout, &memory);
  flush_word (stdout);

  return 0;
}
