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
#include <stdlib.h>
#include <getopt.h>

#include "dis.h"

static word_t read_word (FILE *f)
{
  word_t word = get_word (f);
  if (word == -1)
    {
      fprintf (stderr, "Unexpected end of file in input.\n");
      exit (1);
    }
  return word;
}

static double read_float (FILE *f)
{
  word_t word = read_word (f);
  int sign = word >> 35;
  int exponent, fraction;
  double exp = 1.0, y = 2.0;
  int i;

  if (sign)
    {
      word = (word ^ 0777777777777LL) + 1;
      word &= 0777777777777LL;
    }

  exponent = (word >> 27) & 0377;
  fraction = word & 0777777777;

  exponent -= 0233;
  if (exponent < 0)
    {
      exponent = -exponent;
      y = 0.5;
    }

  for (i = 0; i < exponent; i++)
    exp *= y;

  return (sign ? -1.0 : 1.0) * (double)fraction * exp;
}

static void process (FILE *f)
{
  word_t word;
  double x;
  int i, n;

  word = read_word (f);
  fprintf(stderr, "Word %012llo\n", word);
  word = read_word (f);
  n = word;
  fprintf(stderr, "Word %012llo\n", word);
  word = read_word (f);
  fprintf(stderr, "Word %012llo\n", word);
  word = read_word (f);
  fprintf(stderr, "Word %012llo\n", word);
  word = read_word (f);
  fprintf(stderr, "Name %012llo\n", word);
  word = read_word (f);
  fprintf(stderr, "Word %012llo\n", word);

  for (i = 0; i < n; i++)
    {
      x = read_float (f);
      fprintf(stderr, "Floats %20f ", x);
      x = read_float (f);
      fprintf(stderr, "%20f ", x);
      x = read_float (f);
      fprintf(stderr, "%20f\n", x);
    }

  while ((word = get_word (f)) != -1)
    fprintf(stderr, "Word %012llo\n", word);
}

static void usage (char *name)
{
  fprintf (stderr, "Usage: %s [-W<word format>]\n\n", name);
  usage_word_format ();
  exit (1);
}

int main (int argc, char **argv)
{
  int opt;

  while ((opt = getopt (argc, argv, "W:")) != -1)
    {
      switch (opt)
        {
        case 'W':
          if (parse_input_word_format (optarg))
            usage (argv[0]);
          break;
        default:
          usage (argv[0]);
        }
    }

  process(stdin);

  return 0;
}
