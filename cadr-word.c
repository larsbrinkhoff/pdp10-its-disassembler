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

#include <stdio.h>
#include "dis.h"

/* Data files for CADR Lisp machines were apparently stored as 32 bits
   left aligned in a 36-bit word.  The 32-bit word is in PDP-11 endian
   format, i.e. 16-bit words are little endian but the most
   significant 16-bit word comes before the least significant.  Maybe
   this is a legacy from the CONS machine which was attached to the AI
   lab PDP-10 through Unibus. */

static void
write_cadr_word (FILE *f, word_t word)
{
  fputc ((word >> 20) & 0377, f);
  fputc ((word >> 28) & 0377, f);
  fputc ((word >>  4) & 0377, f);
  fputc ((word >> 12) & 0377, f);
}

struct word_format cadr_word_format = {
  "cadr",
  NULL,
  NULL,
  write_cadr_word,
  NULL
};
