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
#include <string.h>
#include "dis.h"

static int height;

static void 
character (FILE *f)
{
  int i, width, bytes, words;
  word_t user, word;

  user = get_word (f);
  fprintf (stderr, "USER ID: %012llo\n", user);
  word = get_word (f);
  if (user == 0777777777777LL && word == 0777777777777LL)
    exit (0);
  fprintf (stderr, "LEFT KERN,,CODE: %06llo,,%06llo\n",
           word >> 18, word & 0777777);
  word = get_word (f);
  width = word >> 18;
  fprintf (stderr, "RASTER WIDTH,,CHARACTER WIDTH: %06o,,%06llo\n",
           width, word & 0777777);

  bytes = (width + 7) / 8;
  words = (height * bytes + 3) / 4;

  for (i = 0; i < words; i++)
    {
      word = get_word (f);
      fprintf (stderr, "Data: %012llo\n", word);
    }
}

int
main (int argc, char **argv)
{
  word_t word;

  word = get_word (stdin);
  fprintf (stderr, "KSTID: %012llo\n", word);
  word = get_word (stdin);
  height = word & 0777777;
  fprintf (stderr, "Column position adjustment: %03llo\n", word >> 27);
  fprintf (stderr, "Base line: %03llo\n", (word >> 18) & 0777);
  fprintf (stderr, "Character height: %06o\n", height);

  for (;;)
    {
      if (feof (stdin))
        exit (0);
      character (stdin);
    }

  return 0;
}
