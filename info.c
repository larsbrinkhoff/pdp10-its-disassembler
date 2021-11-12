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

word_t start_instruction;
FILE *output_file;

word_t
ascii_to_sixbit (const char *ascii)
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

word_t
ascii_to_squoze (const char *ascii)
{
  int i;
  word_t word = 0;

  for (i = 0; i < 6; i++)
    {
      word *= 40;
      switch (*ascii)
	{
	default:
	case ' ': word += 000; break;
	case '0': word += 001; break;
	case '1': word += 002; break;
	case '2': word += 003; break;
	case '3': word += 004; break;
	case '4': word += 005; break;
	case '5': word += 006; break;
	case '6': word += 007; break;
	case '7': word += 010; break;
	case '8': word += 011; break;
	case '9': word += 012; break;
	case 'A':
	case 'a': word += 013; break;
	case 'B':
	case 'b': word += 014; break;
	case 'C':
	case 'c': word += 015; break;
	case 'D':
	case 'd': word += 016; break;
	case 'E':
	case 'e': word += 017; break;
	case 'F':
	case 'f': word += 020; break;
	case 'G':
	case 'g': word += 021; break;
	case 'H':
	case 'h': word += 022; break;
	case 'I':
	case 'i': word += 023; break;
	case 'J':
	case 'j': word += 024; break;
	case 'K':
	case 'k': word += 025; break;
	case 'L':
	case 'l': word += 026; break;
	case 'M':
	case 'm': word += 027; break;
	case 'N':
	case 'n': word += 030; break;
	case 'O':
	case 'o': word += 031; break;
	case 'P':
	case 'p': word += 032; break;
	case 'Q':
	case 'q': word += 033; break;
	case 'R':
	case 'r': word += 034; break;
	case 'S':
	case 's': word += 035; break;
	case 'T':
	case 't': word += 036; break;
	case 'U':
	case 'u': word += 037; break;
	case 'V':
	case 'v': word += 040; break;
	case 'W':
	case 'w': word += 041; break;
	case 'X':
	case 'x': word += 042; break;
	case 'Y':
	case 'y': word += 043; break;
	case 'Z':
	case 'z': word += 044; break;
	case '.': word += 045; break;
	case '$': word += 046; break;
	case '%': word += 047; break;
	}
      if (*ascii != 0)
	ascii++;
    }

  return word;
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
    fprintf (output_file, " [WARNING: overflowed year field]");
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
  fprintf (output_file, "    Symbol %s = ", str);
  fprintf (output_file, "%llo   (", word2);

  if (word1 & SYHKL)
    {
      fprintf (output_file, " halfkilled");
      flags |= SYMBOL_HALFKILLED;
    }
  if (word1 & SYKIL)
    {
      fprintf (output_file, " killed");
      flags |= SYMBOL_KILLED;
    }
  if (word1 & SYLCL)
    fprintf (output_file, " local");
  if (word1 & SYGBL)
    {
      fprintf (output_file, " global");
      flags |= SYMBOL_GLOBAL;
    }
  fprintf (output_file, ")\n");

  add_symbol (str, word2, flags);
}

void
sblk_info (FILE *f, word_t word0, int cpu_model)
{
  int block_length;
  word_t word;
  int i;

  fprintf (output_file, "Start instruction:\n");
  disassemble_word (NULL, word0, -1, cpu_model);

  while ((word = get_word (f)) & SIGNBIT)
    {
      fprintf (output_file, "\n");
      reset_checksum (word);
      block_length = -((word >> 18) | ((-1) & ~0777777));
      switch ((int)word & 0777777)
	{
	case STBDEF:
	  {
	    char str[7];

	    fprintf (output_file, "Symbol table:\n");

	    for (i = 0; i < block_length; i += 2)
	      {
		word_t word1, word2;

		word1 = get_checksummed_word (f);
		word2 = get_checksummed_word (f);
		if (word1 == -1 || word2 == -1) {
		  fprintf (output_file, "  [WARNING: early end of file]\n");
		  goto end;
		}

		if (word1 & SYFLG)
		  {
		    print_symbol (word1, word2);
		  }
		else
		  {
		    squoze_to_ascii (word1, str);
		    fprintf (output_file, "  Header: %s\n", str);
		  }
	      }
	    goto checksum;
	  }
	case STBUND:
	  fprintf (output_file, "Undefined symbol table:\n");
	  break;
	case STBFIL:
	  {
	    char str[7];

	    fprintf (output_file, "Indirect symbol table pointer:\n");

	    if (block_length != 4)
	      {
		fprintf (output_file, "  (unknown table format)\n");
		break;
	      }

	    sixbit_to_ascii (get_checksummed_word (f), str);
	    fprintf (output_file, "  Device name: %s\n", str);
	    sixbit_to_ascii (get_checksummed_word (f), str);
	    fprintf (output_file, "  File name 1: %s\n", str);
	    sixbit_to_ascii (get_checksummed_word (f), str);
	    fprintf (output_file, "  File name 2: %s\n", str);
	    sixbit_to_ascii (get_checksummed_word (f), str);
	    fprintf (output_file, "  File sname:  %s\n", str);
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
		fprintf (output_file, "Assembly info:\n");
		sixbit_to_ascii (get_checksummed_word (f), str);
		fprintf (output_file, "  User name:          %s\n", str);
		fprintf (output_file, "  Creation time:      ");
		print_datime (output_file, get_checksummed_word (f));
		fputc ('\n', output_file);
		sixbit_to_ascii (get_checksummed_word (f), str);
		fprintf (output_file, "  Source file device: %s\n", str);
		sixbit_to_ascii (get_checksummed_word (f), str);
		fprintf (output_file, "  Source file name 1: %s\n", str);
		sixbit_to_ascii (get_checksummed_word (f), str);
		fprintf (output_file, "  Source file name 2: %s\n", str);
		sixbit_to_ascii (get_checksummed_word (f), str);
		fprintf (output_file, "  Source file sname:  %s\n", str);
		for (i = 0; i < block_length - subblock_length - 1; i++)
		  {
		    fprintf (output_file, "  (%012llo)\n", get_checksummed_word(f));
		  }
		goto checksum;
	      case 2:
		fprintf (output_file, "Debugging info:\n");
		break;
	      default:
		fprintf (output_file, "Unknown miscellaneous info:\n");
		break;
	      }

	    fprintf (output_file, "    (%d words)\n", subblock_length);
	    for (i = 0; i < subblock_length; i++)
	      {
		get_checksummed_word (f);
	      }

	    goto checksum;
	  }
	default:
	  fprintf (output_file, "Unknown information:\n");
	  break;
	}

      fprintf (output_file, "(%d words)\n", block_length);
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

  fprintf (output_file, "\nDuplicate start instruction:\n");
  disassemble_word (NULL, word, -1, cpu_model);

 end:
  return;
}

static char *
unpad (char *str)
{
  for (; *str == ' '; str++)
    ;
  return str;
}

static void
print_sail_symbol (word_t word, word_t block, word_t value)
{
  char *str, str1[7], str2[7];
  int flags = word >> 32;

  squoze_to_ascii (word, str1);
  /* Remove space padding. */
  str = unpad (str1);

  switch (flags)
    {
    case 000: /* Program name. */
      if (*str != 0)
	fprintf (output_file, "  Program: %s\n", str);
      break;
    case 003: /* Block name. */
      fprintf (output_file, "  Block: %s\n", str);
      break;
    default:
      fprintf (output_file, "    Symbol %s ", str);
      squoze_to_ascii (block, str2);
      if (block != 0)
	fprintf (output_file, "[%s] ", unpad (str2));
      fprintf (output_file, "= %llo (%02o)\n", value, flags);
      add_symbol (str, value, 0); /* Don't know what flags mean. */
    }
}

void
dmp_new_symbols (struct pdp10_memory *memory, int table)
{
  word_t word1, word2, block, value;
  int bname, class1, class2, class3, class4, lastv;
  int i;

  bname = get_word_at (memory, table + 1);
  class1 = get_word_at (memory, table + 5);
  class2 = get_word_at (memory, table + 6);
  class3 = get_word_at (memory, table + 7);
  class4 = get_word_at (memory, table + 8);
  lastv = get_word_at (memory, table + 9);

  fprintf (output_file, "\nSymbol table:\n");

  for (i = class1; i < class2; i += 2)
    {
      word1 = get_word_at (memory, table + i);
      word2 = get_word_at (memory, table + i + 1);
      block = get_word_at (memory, table + bname + (word2 >> 23));
      print_sail_symbol (word1, block, word2 & 0777777LL);
    }
  for (i = class2; i < class3; i += 2)
    {
      word1 = get_word_at (memory, table + i);
      word2 = get_word_at (memory, table + i + 1);
      block = get_word_at (memory, table + bname + (word2 >> 23));
      print_sail_symbol (word1, block, word2 & 0777777LL);
    }
  for (i = class3; i < class4; i += 2)
    {
      word1 = get_word_at (memory, table + i);
      word2 = get_word_at (memory, table + i + 1);
      block = get_word_at (memory, table + bname + (word2 >> 23));
      print_sail_symbol (word1, block, (word2 & 0777777LL) << 18);
    }
  for (i = class4; i < lastv; i += 2)
    {
      word1 = get_word_at (memory, table + i);
      word2 = get_word_at (memory, table + i + 1);
      block = get_word_at (memory, table + bname + (word2 >> 23));
      value = get_word_at (memory, table + (word2 & 0777777LL));
      print_sail_symbol (word1, block, value);
    }
}

void
dmp_info (struct pdp10_memory *memory, int cpu_model)
{
  word_t jbsa, jbda, jbsym;
  int p;

  /* For the PDP-6 monitor, JBSA is the start address. */
  jbsa = get_word_at (memory, 0120);
  fprintf (output_file, "Start address (PDP-6): %06llo\n", jbsa & 0777777);
  start_instruction = JRST + (jbsa & 0777777);

  /* For WAITS, JBDA is a JRST to the start address. */
  jbda = get_word_at (memory, 0140);
  fprintf (output_file, "Start address (WAITS): %06llo\n", jbda & 0777777);

  /* .JBSYM is an AOBJN pointer to DDT's symbol table.  If the
     executable doesn't have one, it may be an pointer past the end of
     the executable. */
  jbsym = get_word_at (memory, 0116);
  p = jbsym & 0777777;
  if (jbsym == -1 || p == 0 || get_word_at (memory, p) == -1)
    return;

  if (get_word_at (memory, p) == 0777777777777LL)
    dmp_new_symbols (memory, p);
  else
    {
      int i;
      int length = 01000000 - (jbsym >> 18);

      fprintf (output_file, "\nSymbol table:\n");

      for (i = length - 2; i >= 0; i -= 2)
	{
	  word_t word1 = get_word_at (memory, p + i);
	  word_t word2 = get_word_at (memory, p + i + 1);
	  print_sail_symbol (word1, 0LL, word2);
	}
    }
}

void
dec_info (struct pdp10_memory *memory,
	  word_t entry_vec_len, word_t entry_vec_addr,
	  int cpu_model)
{
  if (entry_vec_addr == -1 || entry_vec_len == 0254000)
    {
      word_t word;

      word = get_word_at (memory, 0120) & 0777777;
      if (word != 0)
	{
	  fprintf (output_file, "Start address: %06llo\n", word);
	  start_instruction = JRST + word;
	}

      word = get_word_at (memory, 0124) & 0777777;
      if (word != 0)
	fprintf (output_file, "Reentry address: %06llo\n", word);

      word = get_word_at (memory, 0137);
      if (word != 0)
	fprintf (output_file, "Version: %012llo\n", word);
    }
  else
    {
      fprintf (output_file, "Entry vector at %06llo length %llo:\n",
	      entry_vec_addr, entry_vec_len);

      if (entry_vec_len == 1)
	{
	  fprintf (output_file, "Start address: %06llo\n", entry_vec_addr);
	}
      else if (entry_vec_len == 3)
	{
	  int addr;

	  fprintf (output_file, "Start instruction:\n");
	  addr = entry_vec_addr;
	  disassemble_word (memory, get_word_at (memory, addr),
			    addr, cpu_model);

	  fprintf (output_file, "Reentry instruction:\n");
	  addr = entry_vec_addr + 1;
	  disassemble_word (memory, get_word_at (memory, addr),
			    addr, cpu_model);

	  fprintf (output_file, "Version: %012llo\n",
		  get_word_at (memory, entry_vec_addr + 2));
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

  fprintf (output_file, "Symbol table:\n");

  for (i = 0; i < syms; i += 2)
    {
      w = get_word_at (memory, a);
      v = get_word_at (memory, a + 1);
      if (w >> 32)
	print_symbol (w, v);
      else
	{
	  squoze_to_ascii (w, name);
	  fprintf (output_file, "  Header: %s (%012llo)\n", name, v);
	}
      a += 2;
    }
}
