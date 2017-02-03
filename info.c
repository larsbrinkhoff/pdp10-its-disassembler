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
#include <string.h>

#include "dis.h"
#include "memory.h"

#define SQUOZE_MASK 0037777777777
#define SYHKL       0400000000000
#define SYKIL       0200000000000
#define SYLCL       0100000000000
#define SYGBL       0040000000000

void
sixbit_to_ascii (word_t sixbit, char *ascii)
{
  int i;

  for (i = 0; i < 6; i++)
    {
      ascii[i] = 040 + ((sixbit >> (6 * (5 - i))) & 077);
    }
  ascii[6] = 0;
}

void
squoze_to_ascii (word_t squoze, char *ascii)
{
  static char table[] =
    {
      ' ', '0', '1', '2', '3', '4', '5', '6',
      '7', '8', '9', 'a', 'b', 'c', 'd', 'e',
      'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
      'n', 'o', 'p', 'q', 'r', 's', 't', 'u',
      'v', 'w', 'x', 'y', 'z', '.', '$', '%'
    };
  int i;

  squoze &= SQUOZE_MASK;

  for (i = 0; i < 6; i++)
    {
      ascii[5-i] = table[squoze % 40];
      squoze /= 40;
    }
  ascii[6] = 0;
}

void
print_date (FILE *f, word_t t)
{
  /* Bits 3.1-3.5 are the day, bits 3.6-3.9 are the month, and bits
     4.1-4.7 are the year. */

  int date = (t >> 18);
  int day = (date & 037);
  int month = (date & 0740);
  int year = (date & 0777000);

  fprintf (f, "%u-%02u-%02u", (year >> 9) + 1900, (month >> 5), day);

  if (year & 0600000)
    printf (" [WARNING: overflowed year field]");
}

void
print_datime (FILE *f, word_t t)
{
  /* The right half of this word is the time of day since midnight in
     half-seconds. */

  int seconds = (t & 0777777) / 2;
  int minutes = (seconds / 60);
  int hours = (minutes / 60);

  print_date (f, t);
  fprintf (f, " %02u:%02u:%02u", hours, (minutes % 60), (seconds % 60));
}

void
sblk_info (FILE *f, word_t word0, int cpu_model)
{
  int block_length, block_count;
  word_t word;
  int i;

  printf ("Start instruction:\n");
  disassemble_word (NULL, word0, -1, cpu_model);

  while ((word = get_word (f)) & SIGNBIT)
    {
      printf ("\n");
      reset_checksum (word);
      block_length = -((word >> 18) | ((-1) & ~0777777));
      switch ((int)word & 0777777)
	{
	case 0:
	  {
	    char str[7];
	    int subblock_length;
	    int global = 0;

	    printf ("Symbol table:\n");

	    block_count = 0;
	    while (block_count < block_length && !global)
	      {
		word = get_checksummed_word (f);
		if (word == -1) {
		  printf ("  [WARNING: early end of file]\n");
		  goto end;
		}
		squoze_to_ascii (word, str);
		printf ("  Header: %s\n", str);
		global = (strcmp (str, "global") == 0);

		subblock_length = get_checksummed_word (f);
		block_count += 2;

		for (i = 0; i < (-(subblock_length >> 18) - 2) / 2; i++)
		  {
		    word_t x = get_checksummed_word (f);
		    squoze_to_ascii (x , str);
		    printf ("    Symbol %s = ", str);
		    printf ("%llo   (", get_checksummed_word (f));

		    if (x & SYHKL)
		      printf (" halfkilled");
		    if (x & SYKIL)
		      printf (" killed");
		    if (x & SYLCL)
		      printf (" local");
		    if (x & SYGBL)
		      printf (" global");
		    printf (")\n");

		    block_count += 2;
		  }
	      }

	    goto checksum;
	  }
	case 1:
	  printf ("Undefined symbol table:\n");
	  break;
	case 2:
	  {
	    char str[7];

	    printf ("Indirect symbol table pointer:\n");

	    if (block_length != 4)
	      {
		printf ("  (unknown table format)\n");
		break;
	      }

	    sixbit_to_ascii (get_checksummed_word (f), str);
	    printf ("  Device name: %s\n", str);
	    sixbit_to_ascii (get_checksummed_word (f), str);
	    printf ("  File name 1: %s\n", str);
	    sixbit_to_ascii (get_checksummed_word (f), str);
	    printf ("  File name 2: %s\n", str);
	    sixbit_to_ascii (get_checksummed_word (f), str);
	    printf ("  File sname:  %s\n", str);
	    goto checksum;
	  }
	case 3:
	  {
	    int subblock_length;
	    char str[7];

	    word = get_checksummed_word (f);
	    subblock_length = -((word >> 18) | ((-1) & ~0777777));
	    switch ((int)word & 0777777)
	      {
	      case 1:
		printf ("Assembly info:\n");
		sixbit_to_ascii (get_checksummed_word (f), str);
		printf ("  User name:          %s\n", str);
		printf ("  Creation time:      ");
		print_datime(stdout, get_checksummed_word (f));
		putchar ('\n');
		sixbit_to_ascii (get_checksummed_word (f), str);
		printf ("  Source file device: %s\n", str);
		sixbit_to_ascii (get_checksummed_word (f), str);
		printf ("  Source file name 1: %s\n", str);
		sixbit_to_ascii (get_checksummed_word (f), str);
		printf ("  Source file name 2: %s\n", str);
		sixbit_to_ascii (get_checksummed_word (f), str);
		printf ("  Source file sname:  %s\n", str);
		for (i = 0; i < block_length - subblock_length - 1; i++)
		  {
		    printf ("  (%012llo)\n", get_checksummed_word(f));
		  }
		goto checksum;
	      case 2:
		printf ("Debugging info:\n");
		break;
	      default:
		printf ("Unknown miscellaneous info:\n");
		break;
	      }

	    printf ("    (%d words)\n", subblock_length);
	    for (i = 0; i < subblock_length; i++)
	      {
		get_checksummed_word (f);
	      }

	    goto checksum;
	  }
	default:
	  printf ("Unknown information:\n");
	  break;
	}

      printf ("(%d words)\n", block_length);
      for (i = 0; i < block_length; i++)
	{
	  get_checksummed_word (f);
	}

    checksum:
      word = get_word (f);
      check_checksum (word);
    }

  printf ("\nDuplicate start instruction:\n");
  disassemble_word (NULL, word, -1, cpu_model);

 end:
  return;
}
