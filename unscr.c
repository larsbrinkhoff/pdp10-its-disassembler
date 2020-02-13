/* Copyright (C) 2020 Lars Brinkhoff <lars@nocrew.org>

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

/* The unscr program takes a single argument, which is a file name.
   It will proceed to unscramble the contents with all possible
   passwords.  If the result is printable ASCII text, it's printed to
   stdout. */

#include <time.h>
#include <stdio.h>
#include <unistd.h>

#include "dis.h"

/* Just allocate a few words to hold the start of the file. */
static word_t buffer[100];

static void usage (const char *x)
{
  fprintf (stderr, "Usage: %s <file>\n", x);
  exit (1);
}

static void
type7 (word_t x)
{
  putchar ((x >> 29) & 0177);
  putchar ((x >> 22) & 0177);
  putchar ((x >> 15) & 0177);
  putchar ((x >>  8) & 0177);
  putchar ((x >>  1) & 0177);
}

static void
type6 (word_t x)
{
  putchar (((x >> 30) & 077) + 040);
  putchar (((x >> 24) & 077) + 040);
  putchar (((x >> 18) & 077) + 040);
  putchar (((x >> 12) & 077) + 040);
  putchar (((x >>  6) & 077) + 040);
  putchar (((x >>  0) & 077) + 040);
}

static int
bad (int ch)
{
  if (ch >= 0 && ch < 8)
    return 1;
  if (ch == 11)
    return 1;
  if (ch > 13 && ch <= 31)
    return 1;
  return 0;
}

static int
accepted (word_t *output, int n)
{
  int i;
  for (i = 0; i < n; i++)
    {
      if (output[i] & 1)
	return 0;
      if (bad ((output[i] >> 29) & 0177))
	return 0;
      if (bad ((output[i] >> 22) & 0177))
	return 0;
      if (bad ((output[i] >> 15) & 0177))
	return 0;
      if (bad ((output[i] >>  8) & 0177))
	return 0;
      if (bad ((output[i] >>  1) & 0177))
	return 0;
    }
  return 1;
} 

static void
decrypt (word_t key, int n)
{
  static word_t output[100];
  int i;

  scramble (1, 0, key, buffer, output, n);

  if (accepted(output, n))
    {
      printf ("\nKEY: ");
      type6 (key);
      putchar ('\n');
      for (i = 0; i < n; i++)
	type7 (output[i]);
    }
}

int
main (int argc, char **argv)
{
  word_t word;
  word_t *p;
  word_t key = 0;
  FILE *f;
  int i, n;

  input_word_format = &its_word_format;

  if (argc != 2)
    usage (argv[0]);

  f = fopen (argv[1], "rb");

  /* Only unscramble this many words. */
  n = 30;

  for (p = buffer, i = 0; i < n; i++)
    {
      if ((word = get_word (f)) == -1)
	break;
      *p++ = word;
    }
  fclose (f);

  for (key = 0; key <= 0777777777777LL; key++)
    {
      if ((key & 077777777) == 0)
	{
	  putchar ('.');
	  fflush (stdout);
	}
      decrypt (key, n);
    }

  return 0;
}
