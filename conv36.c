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
  fprintf (stderr, "Usage: %s [-W<input word format>] [-X<output word format>] [<input file>]\n\n", argv[0]);
  usage_word_format ();
  exit (1);
}

/* If invoked with a name like "its2bin", default to those formats. */
static void
default_formats (const char *argv0)
{
  int bad = 0;
  const char *basename;
  const char *two;
  char *first;

  basename = strrchr (argv0, '/');
  if (basename == NULL)
    basename = argv0;
  else
    basename++;

  two = strchr (basename, '2');
  if (two == NULL)
    return;

  first = strndup (basename, two - basename);
  if (first == NULL)
    {
      fprintf (stderr, "out of memory\n");
      exit (1);
    }
  if (parse_input_word_format (first))
    bad = 1;
  if (parse_output_word_format (two + 1))
    bad = 1;
  free (first);

  if (bad)
    {
      fprintf (stderr, "%s: unrecognised word format in executable name\n", argv0);
      exit (1);
    }
}

int
main (int argc, char **argv)
{
  FILE *file;
  int opt;
  word_t word;

  default_formats (argv[0]);

  while ((opt = getopt (argc, argv, "W:X:")) != -1)
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
        default:
          usage (argv);
        }
    }

  if (optind == argc)
    file = stdin;
  else if (optind == argc - 1)
    {
      file = fopen (argv[optind], "rb");
      if (file == NULL)
        {
          fprintf (stderr, "%s: Error opening %s: %s\n",
                   argv[0], argv[optind], strerror (errno));
          return 1;
        }
    }
  else
    usage (argv);

  while ((word = get_word (file)) != -1)
    write_word (stdout, word);
  flush_word (stdout);

  if (file != stdin)
    fclose (file);

  return 0;
}
