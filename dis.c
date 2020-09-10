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

#define _GNU_SOURCE /* for strcasestr */

#include <stdio.h>
#include <string.h>

#include "opcode/pdp10.h"
#include "dis.h"
#include "memory.h"
#include "symbols.h"
#include "timing.h"

#define OPCODE(word)	(int)(((word) >> 27) &    0777)
#define A(word)		(int)(((word) >> 23) &     017)
#define OPCODE_A(word)	(((OPCODE (word) << 4) | A (word)) << 2)
#define DEVICE(word)	((OPCODE_A (word) & 07740) >> 3)
#define I(word)		(int)(((word) >> 22) &       1)
#define X(word)		(int)(((word) >> 18) &     017)
#define Y(word)		(int)( (word)        & 0777777)
#define E(word)		((I (word) << 22) | (X (word) << 18) | Y (word))

#define POPJ_17 ((word_t)(0263740000000LL))
#define SETZ	((word_t)(0400000000000LL))
#define ITS_OPER 042
#define ITS_CALL 04300

struct its_oper
{
  const char *name;
  int opcode;
  int hint;
};

static struct its_oper opers[] =
{
  { ".ityi",	0000001, HINT_ACCUMULATOR },
  { ".listen",	0000002, HINT_ACCUMULATOR },
  { ".sleep",	0000003, HINT_ACCUMULATOR },
  { ".setmsk",	0000004, HINT_ACCUMULATOR },
  { ".setm2",	0000005, HINT_ACCUMULATOR },
#if 1
  { ".demon",	0000006, HINT_ACCUMULATOR },
#else
  { ".login",	0000006, HINT_ACCUMULATOR },
#endif
  { ".close",	0000007, HINT_CHANNEL },
  { ".uclose",	0000010, HINT_CHANNEL },
  { ".atty",	0000011, HINT_CHANNEL },
  { ".dtty",	0000012, HINT_ACCUMULATOR },
  { ".iopush",	0000013, HINT_ACCUMULATOR },
  { ".iopop",	0000014, HINT_CHANNEL },
  { ".dclose",	0000015, HINT_ACCUMULATOR },
  { ".dstop",	0000016, HINT_ACCUMULATOR },
  { ".rdtime",	0000017, HINT_ACCUMULATOR },
  { ".rdsw",	0000020, HINT_ACCUMULATOR },
  { ".gun",	0000021, HINT_ACCUMULATOR },
  { ".udismt",	0000022, HINT_ACCUMULATOR },
  { ".getsys",	0000023, HINT_ACCUMULATOR },
#if 1
  { ".ipdp",	0000024, HINT_CHANNEL },
#else
  { ".rd500",	0000024, HINT_ACCUMULATOR },
#endif
  { ".getloc",	0000025, HINT_ACCUMULATOR },
  { ".setloc",	0000026, HINT_ACCUMULATOR },
  { ".disown",	0000027, HINT_CHANNEL },
#if 1
  { ".dword",	0000030, HINT_ACCUMULATOR },
  { ".dstep",	0000031, HINT_ACCUMULATOR },
#else
  { ".rd760",	0000030, HINT_ACCUMULATOR }, /* Also .mspace. */
  { ".wr760",	0000031, HINT_ACCUMULATOR },
#endif
  { ".gensym",	0000032, HINT_ACCUMULATOR },
  { ".logout",	0000033, HINT_NUMBER },
#if 1
  { ".realt",	0000034, HINT_ACCUMULATOR },
#else
  { ".gsnam",	0000034, HINT_ACCUMULATOR },
#endif
  { ".wsname",	0000035, HINT_ACCUMULATOR },
  { ".upiset",	0000036, HINT_ACCUMULATOR },
  { ".reset",	0000037, HINT_CHANNEL },
  { ".armove",	0000040, HINT_ACCUMULATOR },
#if 1
  { ".dcontin",	0000041, HINT_ACCUMULATOR },
  { ".cblk",	0000042, HINT_ACCUMULATOR },
#else
  { ".wmar",	0000041, HINT_ACCUMULATOR },
  { ".rrtim",	0000042, HINT_ACCUMULATOR },
#endif
  { ".assign",	0000043, HINT_ACCUMULATOR },
  { ".design",	0000044, HINT_ACCUMULATOR },
  { ".rtime",	0000045, HINT_ACCUMULATOR },
  { ".rdate",	0000046, HINT_ACCUMULATOR },
#if 1
  { ".hang",	0000047, HINT_ACCUMULATOR },
#else
  { ".rd710",	0000047, HINT_ACCUMULATOR },
#endif
  { ".eofc",	0000050, HINT_ACCUMULATOR },
  { ".iotlsr",	0000051, HINT_ACCUMULATOR },
  { ".rsysi",	0000052, HINT_ACCUMULATOR },
  { ".supset",	0000053, HINT_ACCUMULATOR },
#if 1
  { ".pdtime",	0000054, HINT_ACCUMULATOR },
  { ".armrs",	0000055, HINT_ACCUMULATOR },
#else
  { ".orgi",	0000054, HINT_ACCUMULATOR },
  { ".orgo",	0000055, HINT_ACCUMULATOR },
#endif
  { ".ublat",	0000056, HINT_ACCUMULATOR },
  { ".iopdl",	0000057, HINT_ACCUMULATOR },
  { ".ityic",	0000060, HINT_ACCUMULATOR },
  { ".master",	0000061, HINT_ACCUMULATOR },
  { ".vstst",	0000062, HINT_ACCUMULATOR },
#if 1
  { ".netac",	0000063, HINT_CHANNEL },
  { ".nets",	0000064, HINT_CHANNEL },
  { ".revive",	0000065, HINT_ACCUMULATOR },
#else
  { ".dial",	0000063, HINT_ACCUMULATOR },
  { ".dialw",	0000064, HINT_NUMBER },
  { ".hangu",	0000065, HINT_NUMBER },
#endif
  { ".dietim",	0000066, HINT_ACCUMULATOR },
  { ".shutdn",	0000067, HINT_ACCUMULATOR },
  { ".armoff",	0000070, HINT_ACCUMULATOR },
  { ".ndis",	0000071, HINT_ACCUMULATOR },
  { ".feed",	0000072, HINT_CHANNEL },
  { ".eval",	0000073, HINT_ACCUMULATOR },
  { ".redef",	0000074, HINT_ACCUMULATOR },
  { ".ifset",	0000075, HINT_ACCUMULATOR },
  { ".utnam",	0000076, HINT_ACCUMULATOR },
  { ".uinit",	0000077, HINT_ACCUMULATOR },
  { ".ryear",	0000100, HINT_ACCUMULATOR },
  { ".rlpdtm",	0000101, HINT_ACCUMULATOR },
  { ".rdatim",	0000102, HINT_ACCUMULATOR },
  { ".rchst",	0000103, HINT_ACCUMULATOR },
  { ".rbtc",	0000104, HINT_ACCUMULATOR },
  { ".dmpch",	0000105, HINT_ACCUMULATOR },
  { ".swap",	0000106, HINT_ACCUMULATOR },
  { ".mtape",	0000107, HINT_ACCUMULATOR },
  { ".gennum",	0000110, HINT_ACCUMULATOR },
  { ".netint",	0000111, HINT_CHANNEL },
};

static int
spaces (int n)
{
  int i;

  for (i = 0; i < n; i++)
    putchar (' ');

  return i < 0 ? 0 : i;
}

static const struct pdp10_instruction *
lookup (word_t word, int cpu_model)
{
  int i, opcode;

  for (i = 0; i < pdp10_num_instructions; i++)
    {
      if (pdp10_instruction[i].type & PDP10_A_OPCODE)
	opcode = OPCODE_A (word);
      else if (pdp10_instruction[i].type & PDP10_IO)
	opcode = OPCODE_A (word) & 070034;
      else
	opcode = OPCODE (word);
      if (opcode == pdp10_instruction[i].opcode &&
	  (pdp10_instruction[i].model & cpu_model))
	return &pdp10_instruction[i];
    }

  return NULL;
}

static const struct pdp10_device *
lookup_device (int number, int cpu_model)
{
  int i;

  for (i = 0; i < pdp10_num_devices; i++)
    {
      if (number == pdp10_device[i].number &&
	  (pdp10_instruction[i].model & cpu_model))
	return &pdp10_device[i];
    }

  return NULL;
}

static const struct its_oper *
lookup_oper (word_t word)
{
  unsigned int i;
  int e;

  e = E (word);

  for (i = 0; i < sizeof opers / sizeof opers[0]; i++)
    {
      if (e == opers[i].opcode)
	return &opers[i];
    }

  return NULL;
}

int
calc_e (struct pdp10_memory *memory, word_t word)
{
  int e;

  for (;;)
    {
      if (X (word) != 0)
	return -1;

      e = Y (word);
      if (!I (word))
	break;

      word = get_word_at (memory, e);
    }
  
  return e;
}

void
dis (struct pdp10_memory *memory, int cpu_model)
{
  word_t mask = 0777777777777LL;
  word_t word;

  set_address (memory, -1);
  while ((word = get_next_word (memory)) != -1)
    {
      if (word & START_FILE)
	printf ("Start of file.\n");
      else if (word & START_RECORD)
	printf ("Start of record.\n");
      disassemble_word (memory, word & mask, get_address (memory), cpu_model);
    }
}

int
print_val (const char *format, int field, int hint)
{
  const char *p;
  const struct symbol *sym;
  int n = 0;

  for (p = format; *p; p++)
    {
      if (*p == '%')
	{
	  p++; /* assume %o */

	  sym = get_symbol_by_value (field, hint);
	  if (sym == NULL)
	    n += printf ("%o", field);
	  else
	    n += printf ("%s", sym->name);
	}
      else
	{
	  n += printf ("%c", *p);
	}
    }

  return n;
}

void
disassemble_word (struct pdp10_memory *memory, word_t word,
		  int address, int cpu_model)
{
  const struct symbol *sym;
  const struct pdp10_instruction *op;
  int printable, hint;
  char ch[5];
  int i, n;

  sym = get_symbol_by_value (address, HINT_ADDRESS);
  if (sym != NULL)
    printf ("%s:\n", sym->name);

  if (address == -1)
    printf ("         ");
  else
    printf ("%06o:  ", address);

  printf ("%012llo  ", word);

  n = 0;

  op = lookup (word, cpu_model);
  if (op == NULL)
    {
      if ((word >> 18) == 0 && Y (word) != 0)
	{
	  /* If no opcode found and left half is 0, print as symbol. */
	  sym = get_symbol_by_value (Y (word), HINT_ADDRESS);
	  if (sym != NULL)
	    n += printf ("%s", sym->name);
	}
    }
  else if (OPCODE (word) == ITS_OPER)
    {
      const struct its_oper *oper;
      oper = lookup_oper (word);
      if (oper)
	{
	  n += printf ("%-8s ", oper->name);
	  if (oper->hint == HINT_CHANNEL || A (word) != 0)
	    n += print_val ("%o,", A (word), oper->hint);
	}
    }
#if 1
  else if (OPCODE_A (word) == ITS_CALL)
    {
      int address;
      
      n += printf ("%-8s ", ".call");

      address = calc_e (memory, word);
      if (address == -1 ||
	  get_word_at (memory, address) != SETZ)
	{
	  if (I (word))
	    n += printf ("@");
	  n += print_val ("%o", Y (word), HINT_ADDRESS);
	  if (X (word))
	    n += print_val ("(%o)", X (word), HINT_ACCUMULATOR);
	}
      else
	{
	  char name[7];
	  int i;
	  word_t w;

	  n = 0;
	  printf ("[setz\n");
	  w = get_word_at (memory, address + 1);
	  sixbit_to_ascii (w, name);
	  printf ("                                 SIXBIT/%s/\n", name);
	  i = 2;
	  while (((w = get_word_at (memory, address + i)) & SIGNBIT) == 0)
	    {
	      printf ("                                 %012llo\n", w);
	      i++;
	    }
	  printf ("                                 %012llo]\n", w);
	  printf ("                       ");
	}
    }
#endif
  else
    {
      n += printf ("%-8s ", op->name);

      if (op->type & PDP10_IO)
	{
	  const struct pdp10_device *dev;
	  dev = lookup_device (DEVICE (word), cpu_model);
	  if (dev != NULL)
	    n += printf ("%s, ", dev->name);
	  else
	    n += print_val ("%o, ", DEVICE (word), HINT_DEVICE);
	}
      else if (!(op->type & PDP10_A_OPCODE))
	{
	  if (A (word) != 0 || !(op->type & PDP10_A_UNUSED))
	    {
	      hint = op->ac_hint ? op->ac_hint : HINT_ACCUMULATOR;
	      n += print_val ("%o, ", A (word), hint);
	    }
	}

      if (E (word) != 0 || !(op->type & PDP10_E_UNUSED))
	{
	  if (I (word))
	    n += printf ("@");

	  if (Y (word) != 0 && X (word) != 0)
	    n += print_val ("%o", Y (word), HINT_OFFSET);
	  else if (op->addr_hint != 0 && X (word) == 0)
	    n += print_val ("%o", Y (word), op->addr_hint);
	  else if (Y (word) != 0 ||
		   (I (word) != 0 && X (word) == 0) ||
		   (!(op->type & PDP10_E_UNUSED) && X (word) == 0))
	    n += print_val ("%o", Y (word), HINT_ADDRESS);

	  if (X (word))
	    n += print_val ("(%o)", X (word), HINT_ACCUMULATOR);
	}
    }

  spaces (25 - n);

#if 0
  /* Print instruction execution time. */
  {
    int t = instruction_time (word, PDP10_KI10);
    if (t > 0)
      printf (";%5dns", t);
    else
      spaces (8);
  }
#endif

#if 1
  /* Print word as six SIXBIT characters. */
  printf (";\"");
  for (i = 0; i < 6; i++)
    {
      int c = (int)(word >> (6 * (5 - i))) & 077;
      putchar (c + ' ');
    }
  putchar ('"');
#endif

#if 1
  /* Print word as five ASCII characters, if it seems likely to be a
   * printable string. */
  printable = 1;
  for (i = 0; i < 5; i++)
    {
      ch[i] = (word >> (((4 - i) * 7) + 1)) & 0177;
      switch (ch[i])
	{
	case '\0':
	case '\t':
	case '\n':
	case '\f':
	case '\r':
	case '\033':
	  break;
	default:
	  if (ch[i] < 040 || ch[i] > 0176)
	    printable = 0;
	  break;
	}
    }

  if ((word & 1) == 1 ||
      word == 0 ||
      word == POPJ_17)
    printable = 0;

  {
    int seen_nonzero = 0;
    for (i = 4; i >= 0; i--)
      {
	if (ch[i] == 0)
	  {
	    if (seen_nonzero)
	      printable = 0;
	  }
	else
	  seen_nonzero = 1;
      }
  }

  if (printable)
    {
      printf (" \"");
      for (i = 0; i < 5; i++)
	{
	  switch (ch[i])
	    {
	    case '\0':
	      printf ("\\0");
	      break;
	    case '\t':
	      printf ("\\t");
	      break;
	    case '\n':
	      printf ("\\n");
	      break;
	    case '\f':
	      printf ("\\f");
	      break;
	    case '\r':
	      printf ("\\r");
	      break;
	    case '\\':
	      printf ("\\\\");
	      break;
	    case '\"':
	      printf ("\\\"");
	      break;
	    default:
	      if (ch[i] < 040 || ch[i] > 0176)
		printf ("\\%03o", ch[i]);
	      else
		putchar (ch[i]);
	      break;
	    }
	}
      printf ("\"");
    }
#endif

#if 0
  /* Print word as six SQUOZE characters. */
  squoze_to_ascii (word, ch);
  printf (" \"%s\"", ch);
#endif

  printf ("\n");
}

void
usage_machine (void)
{
  fprintf (stderr, "Valid machines are: 166, KA10, KI10, KL10, and KS10.\n");
  fprintf (stderr, "For KA/KL/KS, append ITS to get the MIT instructions.\n");
}

int
parse_machine (const char *string, int *machine)
{
  int its = 0;

  /* Do some lax matching to find CPU and variants. */

  if (strcasestr (string, "ITS"))
    its = 1;

  if (strcasestr (string, "166"))
    *machine = PDP6_166;
  else if (strcasestr (string, "pdp6"))
    *machine = PDP6_166;
  else if (strcasestr (string, "KA"))
    *machine = its ? PDP10_KA10_ITS : PDP10_KA10;
  else if (strcasestr (string, "KI"))
    *machine = PDP10_KI10;
  else if (strcasestr (string, "KL"))
    *machine = its ? PDP10_KL10_ITS : (PDP10_KL10|PDP10_KL10_271);
  else if (strcasestr (string, "KS"))
    *machine = its ? PDP10_KS10_ITS : PDP10_KS10;
  else
    return -1;

  return 0;
}
