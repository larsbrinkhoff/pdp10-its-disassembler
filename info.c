/* Copyright (C) 2013, 2019 Lars Brinkhoff <lars@nocrew.org>
   Copyright (C) 2018, 2020 Adam Sampson <ats@offog.org>

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
#include "symbols.h"

#define SQUOZE_MASK 0037777777777
#define SYHKL       0400000000000
#define SYKIL       0200000000000
#define SYLCL       0100000000000
#define SYGBL       0040000000000
#define SYFLG       0740000000000 /* all flags mask */

/* Information block types */
#define STBDEF      0
#define STBUND      1
#define STBFIL      2
#define STBINF      3

word_t
ascii_to_sixbit (char *ascii)
{
  int i;
  char c;
  word_t w;
  int zero = 0;

  w = 0;
  for (i = 0; i < 6; i++)
    {
      if (zero)
	c = ' ';
      else
        {
          c = *ascii++;
          if (c == 0)
            {
              c = ' ';
              zero = 1;
            }
        }
      w <<= 6;
      if (c >= 'a' && c <= 'z')
	c -= 32;
      w += (c - 040);
    }

  return w;
}

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

int supress_warning = 0;

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

  if ((year & 0600000) && !supress_warning)
    printf (" [WARNING: overflowed year field]");
}

void
print_time (FILE *f, word_t t)
{
  /* The right half of this word is the time of day since midnight in
     half-seconds. */

  int seconds = (t & 0777777) / 2;
  int minutes = (seconds / 60);
  int hours = (minutes / 60);

  fprintf (f, "%02u:%02u:%02u", hours, (minutes % 60), (seconds % 60));
}

void
print_datime (FILE *f, word_t t)
{
  print_date (f, t);
  fputc (' ', f);
  print_time (f, t);
}

static void
print_symbol (word_t word1, word_t word2)
{
  char str[7];
  int flags = 0;

  squoze_to_ascii (word1, str);
  printf ("    Symbol %s = ", str);
  printf ("%llo   (", word2);

  if (word1 & SYHKL)
    {
      printf (" halfkilled");
      flags |= SYMBOL_HALFKILLED;
    }
  if (word1 & SYKIL)
    {
      printf (" killed");
      flags |= SYMBOL_KILLED;
    }
  if (word1 & SYLCL)
    printf (" local");
  if (word1 & SYGBL)
    {
      printf (" global");
      flags |= SYMBOL_GLOBAL;
    }
  printf (")\n");

  add_symbol (str, word2, flags);
}

void
sblk_info (FILE *f, word_t word0, int cpu_model)
{
  int block_length;
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
	case STBDEF:
	  {
	    char str[7];

	    printf ("Symbol table:\n");

	    for (i = 0; i < block_length; i += 2)
	      {
		word_t word1, word2;

		word1 = get_checksummed_word (f);
		word2 = get_checksummed_word (f);
		if (word1 == -1 || word2 == -1) {
		  printf ("  [WARNING: early end of file]\n");
		  goto end;
		}

		if (word1 & SYFLG)
		  {
		    print_symbol (word1, word2);
		  }
		else
		  {
		    squoze_to_ascii (word1, str);
		    printf ("  Header: %s\n", str);
		  }
	      }
	    goto checksum;
	  }
	case STBUND:
	  printf ("Undefined symbol table:\n");
	  break;
	case STBFIL:
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
	case STBINF:
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
      if (word == -1LL)
	return;
      check_checksum (word);
    }

  printf ("\nDuplicate start instruction:\n");
  disassemble_word (NULL, word, -1, cpu_model);

 end:
  return;
}

void
dmp_info (struct pdp10_memory *memory, int cpu_model)
{
  word_t jbsa, jbda, jbsym;
  int p;

  /* For the PDP-6 monitor, JBSA is the start address. */
  jbsa = get_word_at (memory, 0120);
  printf ("Start address (PDP-6): %06llo\n", jbsa & 0777777);

  /* For WAITS, JBDA is a JRST to the start address. */
  jbda = get_word_at (memory, 0140);
  printf ("Start address (WAITS): %06llo\n", jbda & 0777777);

  /* .JBSYM is an AOBJN pointer to DDT's symbol table.  If the
     executable doesn't have one, it may be an pointer past the end of
     the executable. */
  jbsym = get_word_at (memory, 0116);
  p = jbsym & 0777777;
  if (jbsym != -1 && p != 0 && get_word_at (memory, p) != -1)
    {
      char str[7];
      int i;
      int length = -((jbsym >> 18) | ((-1) & ~0777777));

      printf ("\nSymbol table:\n");

      for (i = 0; i < length / 2; i++)
	{
	  word_t x = get_word_at (memory, p++);
	  squoze_to_ascii (x, str);
	  printf ("    Symbol %s = ", str);
	  printf ("%llo   (", get_word_at (memory, p++));
	  if (x & SYHKL)
	    printf (" halfkilled");
	  if (x & SYKIL)
	    printf (" killed");
	  if (x & SYLCL)
	    printf (" local");
	  if (x & SYGBL)
	    printf (" global");
	  printf (")\n");
	}
    }
}

int
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

void
ntsddt_info (struct pdp10_memory *memory, int ddt)
{
  extern word_t get_word_at ();
  char name[7];
  word_t a, w, v;
  int i, syms;

  a = get_word_at (memory, ddt - 1);
  if (a == -1LL)
    {
      fprintf (stderr, "No DDT symbol table.\n");
      return;
    }

  /* The word at DDT-1 points to an AOBJ pointer. */

  a = get_word_at (memory, a);
  syms = a >> 18;
  if (syms & 0400000)
    syms |= -1 << 18;
  syms = -syms;
  a &= 0777777;

  printf ("Symbol table:\n");

  for (i = 0; i < syms; i += 2)
    {
      w = get_word_at (memory, a);
      v = get_word_at (memory, a + 1);
      if (w >> 32)
	print_symbol (w, v);
      else
	{
	  squoze_to_ascii (w, name);
	  printf ("  Header: %s (%012llo)\n", name, v);
	}
      a += 2;
    }
}
