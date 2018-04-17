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
extern word_t get_dta_word (FILE *f);
extern void rewind_bin_word (FILE *f);
extern void rewind_its_word (FILE *f);
extern void rewind_x_word (FILE *f);
extern void rewind_dta_word (FILE *f);

int file_36bit_format = FORMAT_ITS;
static word_t checksum;

word_t
get_word (FILE *f)
{
  switch (file_36bit_format)
    {
    case FORMAT_BIN:	return get_bin_word (f);
    case FORMAT_ITS:	return get_its_word (f);
    case FORMAT_X:	return get_x_word (f);
    case FORMAT_DTA:	return get_dta_word (f);
    case FORMAT_AA:	return get_aa_word (f);
    }

  return -1;
}

void
reset_checksum (word_t word)
{
  checksum = word;
}

void
check_checksum (word_t word)
{
  if (word != checksum)
    printf ("  [WARNING: bad checksum, %012llo /= %012llo]\n", word, checksum);
}

word_t
get_checksummed_word (FILE *f)
{
  word_t word = get_word (f);

  checksum = (checksum << 1) + (checksum >> 35) + word;
  checksum &= 0777777777777ULL;

  return word;
}

void
rewind_word (FILE *f)
{
  switch (file_36bit_format)
    {
    case FORMAT_BIN:	return rewind_bin_word (f);
    case FORMAT_ITS:	return rewind_its_word (f);
    case FORMAT_X:	return rewind_x_word (f);
    case FORMAT_DTA:	return rewind_dta_word (f);
    case FORMAT_AA:	return rewind_aa_word (f);
    }
}
