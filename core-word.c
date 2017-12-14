/* Copyright (C) 2013 Lars Brinkhoff <lars@nocrew.org>

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

static int
get_byte (FILE *f)
{
  int c = fgetc (f);
  return c == EOF ? 0 : c;
}

word_t
get_core_word (FILE *f)
{
  word_t word;

  if (feof (f))
    return -1;

  word = ((word_t)get_byte (f) << 28) |
         ((word_t)get_byte (f) << 20) |
         ((word_t)get_byte (f) << 12) |
         ((word_t)get_byte (f) <<  4) |
          (word_t)get_byte (f);

  return word;
}

void
rewind_core_word (FILE *f)
{
  rewind (f);
}
