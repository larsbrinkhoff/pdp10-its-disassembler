/* Copyright (C) 2013 Lars Brinkhoff <lars@nocrew.org>
   Copyright (C) 2020 Adam Sampson <ats@offog.org>

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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dis.h"

static void
usage (char **argv)
{
  fprintf (stderr, "Usage: %s [-d] [-v] [-W<input word format>] [-X<output word format>] <password> <input file> <output file>\n\n", argv[0]);
  usage_word_format ();
  exit (1);
}

int
main (int argc, char **argv)
{
  int i;
  word_t *input = NULL;
  int input_count = 0;
  int input_size = 0;
  int decrypt = 0;
  FILE *file;
  int opt;
  word_t *output = NULL;
  word_t password;
  int verbose = 0;
  word_t word;

  while ((opt = getopt (argc, argv, "dvW:X:")) != -1)
    {
      switch (opt)
        {
        case 'd':
          decrypt = 1;
          break;
        case 'v':
          verbose = 1;
          break;
        case 'W':
          if (parse_input_word_format (optarg))
            usage (argv);
          break;
        case 'X':
          if (parse_output_word_format (optarg))
            usage (argv);
          break;
        default:
          usage (argv);
        }
    }

  if (optind != argc - 3)
    usage (argv);

  password = ascii_to_sixbit (argv[optind]);

  /* Read the whole input into memory. */
  file = fopen (argv[optind + 1], "rb");
  if (file == NULL)
    {
      fprintf (stderr, "%s: Error opening %s: %s\n",
               argv[0], argv[optind + 1], strerror (errno));
      return 1;
    }
  while ((word = get_word (file)) != -1)
    {
      if (input_count >= input_size)
        {
          input_size += 1024;
          input = realloc (input, input_size * sizeof (*input));
          if (input == NULL)
            {
              fprintf (stderr, "out of memory\n");
              return 1;
            }
        }

      input[input_count++] = word;
    }
  fclose (file);

  output = calloc (input_count, sizeof (*output));
  if (output == NULL)
    {
      fprintf (stderr, "out of memory\n");
      return 1;
    }

  scramble (decrypt, verbose, password, input, output, input_count);

  file = fopen (argv[optind + 2], "wb");
  for (i = 0; i < input_count; i++)
    {
      write_word (file, output[i]);
    }
  flush_word (file);
  fclose (file);

  free (input);
  free (output);

  return 0;
}
