/* Copyright (C) 2017 Lars Brinkhoff <lars@nocrew.org>

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

#include "dis.h"

#define NEW_ARC ((word_t)(0416243210101LL)) /* Sixbit ARC1!! */

extern word_t get_its_word (FILE *f);

/* Just allocate a full moby to hold the file. */
static word_t buffer[256 * 1024];

static int
byte_size (int code, int *leftovers)
{
  if (code <= 17)
    {
      *leftovers = 0;
      return 044 - code;
    }
  else if (code <= 111)
    {
      *leftovers = (code & 3);
      return (code - 044) >> 2;
    }
  else if (code <= 248)
    {
      *leftovers = (code & 013);
      return (code - 0200) >> 4;
    }
  else
    {
      *leftovers = (code & 077);
      return (code - 0400) >> 6;
    }
}

int
main (int argc, char **argv)
{
  char string[7];
  word_t word;
  word_t *p;
  FILE *f;

  if (argc != 2)
    {
      fprintf (stderr, "Usage: %s <file>\n", argv[0]);
      exit (1);
    }

  f = fopen (argv[1], "rb");
  file_36bit_format = FORMAT_ITS;

  p = buffer;
  while ((word = get_its_word (f)) != -1)
    {
      *p++ = word;
    }

  /* word_t arc_size = p - buffer; */

  if (buffer[0] != NEW_ARC)
    {
      sixbit_to_ascii(buffer[0], string);
      fprintf (stderr, "First word: %012llo \"%s\"\n", buffer[0], string);
      fprintf (stderr, "Old ARC format.  Not supported.\n");
      exit (1);
    }

  word_t name_beg = buffer[1];
  /* word_t data_end = buffer[2]; */

  fprintf (stderr, "Last cleanup: ");
  print_datime (stderr, buffer[3]);
  fputc ('\n', stderr);

  fprintf (stderr, "Created: ");
  print_datime (stderr, buffer[4]);
  fputc ('\n', stderr);

  word_t dumped = buffer[5];
  fprintf (stderr, "Dumped: %llo\n", dumped);

  fprintf (stderr, "\nFile name       Words  Modified            Referenced    Byte\n");

  int i;
  for (i = name_beg; i < 02000; i += 5)
    {
      sixbit_to_ascii(buffer[i], string);
      fprintf (stderr, "%s ", string);
      sixbit_to_ascii(buffer[i+1], string);
      fprintf (stderr, "%s  ", string);

      /* word_t flags = buffer[i+2] >> 18; */
      word_t data = buffer[i+2] & 0777777;

      word_t length = buffer[data];
      /* word_t header = buffer[data+3]; */
      fprintf (stderr, "%6lld  ", length);

      print_datime (stderr, buffer[i+3]);
      fprintf (stderr, " (");
      print_date (stderr, buffer[i+4]);
      fputc (')', stderr);

      int author = (buffer[i+4] >> 9) & 0777;
      if (author != 0 && author != 0777)
	fprintf (stderr, "  Author %03o", author);

      int leftovers;
      fprintf (stderr, "  %d\n",
	       byte_size (buffer[i+4] & 0777, &leftovers));
    }

  return 0;
}
