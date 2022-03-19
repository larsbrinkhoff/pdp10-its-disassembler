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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dis.h"

static word_t data;
static int words = 0;
static int records = 0;
static int files = 0;
static int marks = 0;

static void
count (word_t word)
{
  if (word == -1)
    return;

  if (words > 0)
    {
      if (word & (START_TAPE | START_FILE | START_RECORD))
        fprintf (stderr, "Files: %d, Records: %d, Words: %d\n",
                 files, records, words);
    }
  words++;
  if (word & (START_FILE | START_RECORD))
    records++;
  if (word & START_FILE)
    files++;
}

static void
output (word_t word)
{
#if 0
  if (word & START_TAPE)
    fprintf (stderr, "Start of tape\n");
  else if (word & START_FILE)
    fprintf (stderr, "Start of file\n");
  else if (word & START_RECORD)
    fprintf (stderr, "Start of record\n");
  fprintf (stderr, "Output: %012llo\n", word);
#endif
  if (word == -1)
    return;
  write_word (stdout, word);
  marks = 0;
}

static void end (void)
{
  count (START_TAPE);
  flush_word (stdout);
  exit (0);
}

static void skip_word (FILE *f, word_t n)
{
  int i;

  for (i = 0; i < n; i++)
    {
      count (data);
      data = get_word (f);
      if (data == -1)
        end ();
    }
}

static void skip_record (FILE *f, word_t n)
{
  int i;

  for (i = 0; i < n; )
    {
      count (data);
      data = get_word (f);
      if (data == -1)
        end ();
      if (data & (START_RECORD | START_FILE))
        i++;
    }
}

static void skip_file (FILE *f, word_t n)
{
  int i;

  for (i = 0; i < n; )
    {
      count (data);
      data = get_word (f);
      if (data == -1)
        end ();
      if (data & START_FILE)
        i++;
    }
}

static void
skip_all (FILE *f, word_t n)
{
  (void)n;
  for (;;)
    {
      count (data);
      data = get_word (f);
      if (data == -1)
        end ();
    }
}

static void insert_word (FILE *f, word_t n)
{
  (void)f;
  fprintf (stderr, "Insert word\n");
  count (data);
  output (data);
  data = n;
}

static void copy_word (FILE *f, word_t n)
{
  int i;

  for (i = 0; i < n; i++)
    {
      count (data);
      output (data);
      data = get_word (f);
      if (data == -1)
        end ();
    }
}

static void copy_record (FILE *f, word_t n)
{
  int i;

  for (i = 0; i < n; )
    {
      count (data);
      output (data);
      data = get_word (f);
      if (data == -1)
        end ();
      if (data & (START_RECORD | START_FILE))
        i++;
    }
}

static void copy_file (FILE *f, word_t n)
{
  int i;

  for (i = 0; i < n; )
    {
      count (data);
      output (data);
      data = get_word (f);
      if (data == -1)
        end ();
      if (data & START_FILE)
        i++;
    }
}

static void
copy_all (FILE *f, word_t n)
{
  (void)n;
  for (;;)
    {
      count (data);
      output (data);
      data = get_word (f);
      if (data == -1)
        end ();
    }
}

static void
record (FILE *f, word_t n)
{
  (void)f;
  (void)n;
  data |= START_RECORD;
}

static void
mark (FILE *f, word_t n)
{
  (void)f;
  (void)n;
  marks++;
  if (marks == 1)
    data |= START_FILE;
  else if (marks >= 0)
    data |= START_TAPE;
}

static void
gap (FILE *f, word_t n)
{
  (void)f;
  write_tape_gap (stdout, n);
}

static void
error (FILE *f, word_t n)
{
  (void)f;
  write_tape_error (stdout, n);
}

struct dispatch
{
  const char *command;
  void (*action) (FILE *f, word_t n);
};

static struct dispatch table[] =
  {
    { "sw", skip_word },
    { "sr", skip_record },
    { "sf", skip_file },
    { "sa", skip_all },
    { "iw", insert_word },
    { "cw", copy_word },
    { "cr", copy_record },
    { "cf", copy_file },
    { "ca", copy_all },
    { "r", record },
    { "m", mark },
    { "g", gap },
    { "e", error }
  };

static void
execute (FILE *f, char *command)
{
  char *end;
  size_t i;

  for (i = 0; i < sizeof table / sizeof table[0]; i++)
    {
      int n = strlen (table[i].command);
      if (strncmp (command, table[i].command, n) == 0) {
        unsigned arg = strtol (command + n, &end, 10);
        if (end == command + n)
          arg = 1;
        table[i].action (f, arg);
      }
    }
}

static void
convert_line (FILE *f, char **command)
{
  while (*command)
    execute (f, *command++);
  end ();
}

static void
convert_stdin (FILE *f)
{
  while (!feof (stdin))
    {
      char command[100];
      if (fgets (command, sizeof command, stdin))
        execute (f, command);
    }
  end ();
}

static void
usage (char **argv)
{
  fprintf (stderr, "Usage: %s [-79] [-W<input word format>] [-O<output word format>] [<input files...>]\n\n", argv[0]);
  usage_word_format ();
  exit (1);
}

int
main (int argc, char **argv)
{
  FILE *f;
  int opt;

  output_word_format = &tape_word_format;

  while ((opt = getopt (argc, argv, "79W:O:")) != -1)
    {
      switch (opt)
        {
        case '7':
          input_word_format = output_word_format = &tape7_word_format;
          break;
        case '9':
          input_word_format = output_word_format = &tape_word_format;
          break;
        case 'W':
          if (parse_input_word_format (optarg))
            usage (argv);
          break;
        case 'O':
          if (parse_output_word_format (optarg))
            usage (argv);
          break;
        default:
          usage (argv);
        }
    }

  if (optind == argc || strcmp (argv[optind], "-") == 0)
    f = stdin;
  else
    {
      f = fopen (argv[optind++], "rb");
      if (f == NULL)
        {
          fprintf (stderr, "%s: Error opening %s: %s\n",
                   argv[0], argv[optind - 1], strerror (errno));
          exit (1);
        }
    }

  data = get_word (f);
  data |= START_TAPE | START_FILE | START_RECORD;
  if (optind == argc)
    convert_stdin (f);
  else
    convert_line (f, &argv[optind]);
  return 0;
}
