/* Copyright (C) 2018 Lars Brinkhoff <lars@nocrew.org>

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
#include <string.h>

#include "dis.h"

#define MFD_OCT    0551646164416LL /* M.F.D. */
#define FILE1_OCT  0104651544511LL /* (FILE) */
#define FILE2_OCT  0164651544516LL /* .FILE. */
#define DIR_OCT    0104451621100LL /* (DIR) */

static word_t year;
static int month;
static int day;

static char *tape_type (int n)
{
  if (n == 0)
    return "RANDOM";
  else if ((n & 0700000) == 0700000)
    return "INCREMENTAL";
  else
    return "FULL";
}

static void print_timestamp (word_t word)
{
  int date, y, m, d;

  if ((word >> 27) < 2)
    word |= (year&0776) << 27;

  date = (word >> 18);
  d = (date & 037);
  m = (date & 0740) >> 5;
  y = (date & 0777000) >> 9;

  if (y > year || (y == year && m > month) || (y == year && m == month && d > day))
    y-= 2;

  printf ("%u-%02u-%02u", y + 1900, m, d);
  fputc (' ', stdout);
  print_time (stdout, word);
}

static word_t read_tape_header (FILE *f)
{
  word_t word, word2;
  int count;
  char str[7];

  word = get_word (f);
  count = (word >> 18);
  count ^= 0400000;
  count -= 0400000;
  count = -count;
  if (count != 4)
    printf ("Count: %d\n", count);

  word = get_word (f);
  word2 = get_word (f);
  printf ("TAPE NO %6lld ", (word >> 18) & 0777777);
  sixbit_to_ascii (word2, str);
  year = 10*((word2>>30)-020) + ((word2>>24)&077)-020;
  month = 10*(((word2>>18)&077)-020) + ((word2>>12)&077)-020;
  day = 10*(((word2>>6)&077)-020) + (word2&077)-020;

  printf ("CREATION DATE  %s\n", str);
  printf ("REEL NO %6lld ", word & 0777777);
  word = get_word (f);
  printf ("OF %s DUMP\n", tape_type (word));

  word = get_word (f);
  printf ("Word: %llo\n", word);
  word = get_word (f);
  if (word)
    printf ("Old format tape database.\n");
  else
    printf ("New format tape database.\n");
  return word;
}

static int not_dir_name (word_t fn1, word_t fn2)
{
  if (fn1 == MFD_OCT && fn2 == FILE1_OCT)
    return 0;
  if (fn1 == FILE2_OCT && fn2 == DIR_OCT)
    return 0;
  return 1;
}

static void read_directory (FILE *f, word_t old)
{
  word_t fn1, fn2, word;
  char ufd[7];
  char str[7];

  if (!old)
    {
      word = get_word (f);
      sixbit_to_ascii (word, ufd);
    }
  else
    {
      sixbit_to_ascii (old, ufd);
    }

  for (;;)
    {
      fn1 = get_word (f);
      if (fn1 == -1)
        exit (0);
      else if (fn1 == 0 || fn1 == 0777777777777LL)
        return;

      fn2 = get_word (f);
      word = get_word (f);
      if (old)
        word = get_word (f);
      if (not_dir_name (fn1, fn2))
        {
          sixbit_to_ascii (fn1, str);
          printf (" %s  %s ", ufd, str);
          sixbit_to_ascii (fn2, str);
          printf (" %s ", str);
          //word &= 0177777777777LL;
          if (word == 0777777777777LL || word == 0400000000000LL)
            printf ("-");
          else
            print_timestamp (word);
          putchar ('\n');
        }

      if (old)
        {
          word = get_word (f);
          sixbit_to_ascii (word, ufd);
        }
    }
}

static void read_info (FILE *f)
{
  word_t old = read_tape_header (f);
  for (;;)
    read_directory (f, old);
}

static void
usage (const char *x)
{
  fprintf (stderr, "Usage: %s -o|-n <file>\n", x);
  exit (1);
}

int
main (int argc, char **argv)
{
  FILE *f;

  if (argc != 2)
    usage (argv[0]);

  f = fopen (argv[1], "rb");
  if (f == NULL)
    {
      fprintf (stderr, "error\n");
      exit (1);
    }

  read_info (f);

  return 0;
}
