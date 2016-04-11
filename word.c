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

extern word_t get_bin_word (FILE *f);
extern word_t get_its_word (FILE *f);
extern word_t get_x_word (FILE *f);
extern void rewind_bin_word (FILE *f);
extern void rewind_its_word (FILE *f);
extern void rewind_x_word (FILE *f);

int file_36bit_format = FORMAT_ITS;

word_t
get_word (FILE *f)
{
  switch (file_36bit_format)
    {
    case FORMAT_BIN:	return get_bin_word (f);
    case FORMAT_ITS:	return get_its_word (f);
    case FORMAT_X:	return get_x_word (f);
    }

  return -1;
}

void
rewind_word (FILE *f)
{
  switch (file_36bit_format)
    {
    case FORMAT_BIN:	return rewind_bin_word (f);
    case FORMAT_ITS:	return rewind_its_word (f);
    case FORMAT_X:	return rewind_x_word (f);
    }
}
