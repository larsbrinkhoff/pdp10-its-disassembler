/* Copyright (C) 2019 Lars Brinkhoff <lars@nocrew.org>

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

/*
DIRNAM==BUF+0		;FILE NAME, 2 WORDS OF PDP11 SQUOZE
DIREXT==BUF+1		;LH EXTENSION, 1 WORD OF PDP11 SQUOZE
DIRDAT==BUF+1		;RH SMITHSONIAN DATE
DIRPBN==BUF+2		;PHYS BLOCK NUMBER, BYTE(18)CYL(10)SURF(8)SECT
DIRALC==BUF+3		;WORDS ALLOC LH MOST SIGNIFICANT 16 BITS, RH LEAST
DIRWRT==BUF+4		;WORDS WRITTEN SAME FORMAT AS DIRALC
DIRLOD==BUF+5		;LH 11 LOAD ADR, RH 11 START ADDR
DIRSTS==BUF+6		;LH "FILE TYPE & STATUS", RH CHECKSUM
DIR7==BUF+7		;RESERVED MUST BE ZERO
*/

static void
write_empty_dir (FILE *f)
{
  int i;

  /* A sector is 128 36-bit words.  Every directory entry is 8 36-bit
     words, or really, 16 16-bit words. */
  for (i = 0; i < 128; i += 8)
    {
      write_its_word (f, 0777776777776LL);  /* I see no file here. */
      write_its_word (f, 0777776000000LL);
      write_its_word (f, 0);
      write_its_word (f, 0);
      write_its_word (f, 0);
      write_its_word (f, 0);
      write_its_word (f, 0);
      write_its_word (f, 0);
    }
}

int
main (int argc, char **argv)
{
  int i;

  (void)argc;
  (void)argv;

  file_36bit_format = FORMAT_ITS;

  /* The KLDCP direcory is 64 sectors. */
  for (i = 0; i < 64; i++)
    {
      write_empty_dir (stdout);
    }

  return 0;
}
