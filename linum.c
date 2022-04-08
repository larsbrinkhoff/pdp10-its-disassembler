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
#include <unistd.h>

#include "dis.h"

#define NUL 000
#define TAB 011
#define LF  012
#define FF  014
#define CR  015

#define NEWLINE(CH)  ((CH) == LF || (CH) == FF)

static int spacing = 100;
static word_t input_word;
static int input_characters = 0;
static word_t output_word = 0;
static int output_characters = 0;
static char buffer[10000];
static char *start;
static int length;

static void (*delete_number) (FILE *f);
static void (*add_number) (int *line);

static void
usage (char **argv)
{
  fprintf (stderr, "Usage: %s [-adf] [-W<word format>] [<file>]\n\n", argv[0]);
  usage_word_format ();
  exit (1);
}

static int
readc (FILE *f)
{
  int c;

  if (input_characters == 0)
    {
      input_word = get_word (f);
      if (input_word == -1)
        return -1;
      input_characters = 5;
    }

  c = (input_word >> 29) & 0177;
  input_characters--;
  input_word <<= 7;
  input_word &= 0777777777777LL;
  return c;
}

static void
read_line (FILE *f)
{
  int c;
  start = buffer;
  length = 0;
  for (;;)
    {
      c = readc (f);
      if (c == -1)
        return;
      buffer[length++] = c;
      if (NEWLINE (c))
        return;
    }
}

static int
end (void)
{
  int i;
  for (i = 0; i < length; i++)
    {
      if (buffer[i] != NUL)
        return 0;
    }
  return 1;
}

static void
writec (int c)
{
  c &= 0177;
  output_word |= (word_t)c << (29 - 7*output_characters);
  output_characters++;
  if (output_characters == 5)
    {
      write_word (stdout, output_word);
      output_word = 0;
      output_characters = 0;
    }
}

static void
write_line (void)
{
  int i;
  for (i = 0; i < length; i++)
    writec (*start++);
}

static void
discard (FILE *f)
{
  while (input_characters > 0)
    readc (f);
}

static void
pad (void)
{
  while (output_characters != 0)
    writec (0);
}

static void
line_number (int n)
{
  word_t word = 0;
  int i;

  for (i = 0; i < 5; i++)
    {
      word >>= 7;
      word |= (word_t)((n % 10) + '0') << 29;
      n /= 10;
    }

  write_word (stdout, word | 1);
}

static int
digits (void)
{
  int i;
  for (i = 0; i < 5; i++)
    {
      if (buffer[i] < '0' || buffer[i] > '9')
        return i;
    }
  return i;
}

static void
normal_add (int *line)
{
  line_number (*line);
  writec (TAB);
}

static void
fix_add (int *line)
{
  if (length >= 5 && digits () == 5)
    {
      /* If there are exactly five digits, output with bit 35 set. */
      word_t word = 0;
      word |= (word_t)buffer[0] << 29;
      word |= (word_t)buffer[1] << 22;
      word |= (word_t)buffer[2] << 15;
      word |= (word_t)buffer[3] << 8;
      word |= (word_t)buffer[4] << 1;
      write_word (stdout, word | 1);
      *line = (*start++ - '0') * 10000;
      *line += (*start++ - '0') * 1000;
      *line += (*start++ - '0') * 100;
      *line += (*start++ - '0') * 10;
      *line += *start++ - '0';

      /* Add following TAB. */
      if (length >= 6 && buffer[5] != TAB)
        writec (TAB);
      length -= 5;
    }
  else
    normal_add (line);
}

static void
add (FILE *f)
{
  int line = spacing;

  for (;;)
    {
      read_line (f);
      if (end ())
        return;

      pad ();
      add_number (&line);
      write_line ();

      if (!NEWLINE (start[-1]))
        return;
      else if (start[-1] == FF)
        line = spacing;
      else
        line += spacing;
    }
}

static int
digits2 (FILE *f, char *number)
{
  int n, c;

  for (n = 0; n < 5;)
    {
      c = readc (f);
      if (c == -1)
        return n;
      number[n] = c;
      if (c == NUL)
        ;
      else if (c < '0' || c > '9')
        return n;
      else
        n++;
    }
  
  return n;
}

static void
fix_delete (FILE *f)
{
  char number[5];
  int c, i, n;

  /* If there are exactly five digits, discard them. */
  n = digits2 (f, number);
  if (n < 5)
    {
      for (i = 0; i < n + 1; i++)
        writec (number[i]);
    }

  /* Discard following TAB. */
  c = readc (f);
  if (c != -1 && c != TAB)
    writec (c);
}

static void
normal_delete (FILE *f)
{
  /* If exactly on word boundary, or remainder of word is 0. */
  if (input_characters == 0 || input_word == 0)
    {
      /* Ignore remainder of current input word. */
      discard (f);
            
      input_word = get_word (f);
      if (input_word != -1)
        {
          input_characters = 5;

          /* Discard next word if bit 35 is set. */
          if (input_word & 1)
            discard (f);
        }
    }
}

static void
delete (FILE *f)
{
  int c;

  delete_number (f);

  for (;;)
    {
      c = readc (f);
      if (c == -1)
        return;
      if (c == LF || c == FF)
        {
          writec (c);
          delete_number (f);
        }
      else
        writec (c);
    }
}

static void
nothing (FILE *f)
{
  (void)f;
  fprintf (stderr, "No operation specified; use one of -a or -d.\n");
  exit (1);
}

int
main (int argc, char **argv)
{
  void (*process) (FILE *f) = nothing;
  int opt;

  delete_number = normal_delete;
  add_number = normal_add;

  while ((opt = getopt (argc, argv, "adfW:")) != -1)
    {
      switch (opt)
        {
        case 'a':
          process = add;
          break;
        case 'd':
          process = delete;
          break;
        case 'f':
          delete_number = fix_delete;
          add_number = fix_add;
          break;
        case 'W':
          if (parse_input_word_format (optarg))
            usage (argv);
          if (parse_output_word_format (optarg))
            usage (argv);
          break;
        default:
          usage (argv);
        }
    }

  if (optind == argc)
    process (stdin);
  else if (argc > optind + 1)
    {
      fprintf (stderr, "Too many files on the command line\n");
      exit (1);
    }
  else
    {
      FILE *f = fopen (argv[optind], "rb");
      if (f == NULL)
        {
          fprintf (stderr, "Error opening %s\n", argv[optind]);
          exit (1);
        }
      process (f);
    }

  pad ();
  flush_word (stdout);
  return 0;
}
