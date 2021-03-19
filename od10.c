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
#include <unistd.h>
#include "dis.h"

unsigned int address = 0;
int offset = -1;

static void
print_octal (word_t word)
{
  if (word == -1)
    {
      printf ("               ");
      return;
    }
  
  printf (" %06llo,,%06llo", word >> 18, word & 0777777LL);
  address++;
  if (offset != -1)
    offset++;
}

static void
print_sixbit (word_t word)
{
  char string[7];

  if (word == -1)
    {
      printf ("       ");
      return;
    }
  
  sixbit_to_ascii (word, string);
  printf (" %s", string);
}

static void
print_ascii (word_t word)
{
  int i;

  if (word == -1)
    return;

  for (i = 0; i < 5; i++)
    {
      char c = (word >> 29) & 0177;
      if (c < 32 || c == 0177)
	c = '.';
      putchar (c);
      word <<= 7;
    }
}

static void
print_double (word_t even, word_t odd)
{
  printf ("%010o", address);
  if (offset != -1)
    printf ("/%06o", offset);
  printf (": ");

  print_octal (even);
  print_octal (odd);
  putchar (' ');

  print_sixbit (even);
  print_sixbit (odd);
  printf ("  ");

  print_ascii (even);
  print_ascii (odd);
  putchar ('\n');
}

static void
od (FILE *f)
{
  word_t even, odd;
  for (;;)
    {
      even = get_word (f);
      if (even == -1)
	exit (0);
    record:
      if (even & START_TAPE)
	printf ("Logical end of tape.\n");
      else if (even & START_FILE)
	printf ("Start of file.\n");
      else if (even & START_RECORD)
	printf ("Start of record.\n");
      if (even & (START_TAPE|START_FILE|START_RECORD))
	offset = 0;
      even &= WORDMASK;

      odd = get_word (f);
      if (odd != -1 && odd & (START_TAPE|START_FILE|START_RECORD))
	{
	  print_double (even, -1);
	  even = odd;
	  goto record;
	}
      print_double (even, odd);
      if (odd == -1)
	exit (0);
    }
}

static void
usage (const char *x)
{
  fprintf (stderr, "Usage: %s [-Wformat] < file\n", x);
  usage_word_format ();
  exit (1);
}

int
main (int argc, char **argv)
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

  od (stdin);

  return 0;
}
