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

#define MAXTAP 5000  /* Maximum number of tapes. */
#define TPINFL 10    /* Words of info per tape. */

word_t image[TPINFL * MAXTAP];
int db_tapes;

static word_t *
get_tape (int tape)
{
  return &image[TPINFL * tape];
}

static int read_database (FILE *f)
{
  word_t *p = image;
  int n = 0;

  for (;;)
    {
      *p = get_word (f);
      if (*p == -1)
	return n;
      p++;
      n++;
    }
}

static char *tape_type (int n)
{
  if (n == 0)
    return "Random";
  else if ((n & 0700000) == 0700000)
    return "Incremental";
  else
    return "Full";
}

static void print_tape (int n)
{
  word_t *word = get_tape (n);
  char str[7];

  if (word[1] == 0)
    return;

  printf ("Tape number: %lld\n", (word[0] >> 18) & 0777777);
  printf ("Reel number: %lld\n",  word[0]        & 0777777);
  sixbit_to_ascii (word[1], str);
  printf ("Last written: %s\n", str);
  printf ("Tape type: %s\n", tape_type (word[2]));
  sixbit_to_ascii (word[3], str);
  printf ("First directory: %s\n", str);
  sixbit_to_ascii (word[4], str);
  printf ("Last directory: %s\n", str);
  sixbit_to_ascii (word[5], str);
  printf ("Operator: %s\n", str);
  
  /*
STAPEN:	0	;TAPE NUMBER,,REEL NUMBER
STDATE:	0	;DATE TAPE LAST WRITTEN
STYPE:	0	;TYPE OF DATA LAST WRITTEN AS THTYPE ABOVE
SFUSNM:	0	;FIRST USER ON THIS TAPE
SLUSNM:	0	;LAST USER ON THIS TAPE
SUSER:	0	;UNAME OF PERSON WHO DUMPED THIS TAPE
SDONE:	0	;ZERO IF DUMP NOT COMPLETED,-1 REGULAR, 1 ARCHIVE
  */
}

static void
usage (const char *x)
{
  fprintf (stderr, "Usage: %s -x|-t <file>\n", x);
  exit (1);
}

int
main (int argc, char **argv)
{
  FILE *f;
  int i;

  if (argc != 3)
    usage (argv[0]);

  if (argv[1][0] != '-')
    usage (argv[0]);

  switch (argv[1][1])
    {
    case 't':
      break;
    case 'x':
      break;
    default:
      usage (argv[0]);
      break;
    }

  f = fopen (argv[2], "rb");
  if (f == NULL)
    {
      fprintf (stderr, "error\n");
      exit (1);
    }

  db_tapes = read_database (f) / TPINFL;
  printf ("%d tapes in database\n", db_tapes);

  for (i = 0; i < db_tapes; i++)
    {
      print_tape (i);
    }

  return 0;
}
