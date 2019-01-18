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
};

static struct its_oper opers[] =
{
  { ".ityi",	0000001 },
  { ".listen",	0000002 },
  { ".sleep",	0000003 },
  { ".setmsk",	0000004 },
  { ".setm2",	0000005 },
#if 1
  { ".demon",	0000006 },
#else
  { ".login",	0000006 },
#endif
  { ".close",	0000007 },
  { ".uclose",	0000010 },
  { ".atty",	0000011 },
  { ".dtty",	0000012 },
  { ".iopush",	0000013 },
  { ".iopop",	0000014 },
  { ".dclose",	0000015 },
  { ".dstop",	0000016 },
  { ".rdtime",	0000017 },
  { ".rdsw",	0000020 },
  { ".gun",	0000021 },
  { ".udismt",	0000022 },
  { ".getsys",	0000023 },
#if 1
  { ".ipdp",	0000024 },
#else
  { ".rd500",	0000024 },
#endif
  { ".getloc",	0000025 },
  { ".setloc",	0000026 },
  { ".disown",	0000027 },
#if 1
  { ".dword",	0000030 },
  { ".dstep",	0000031 },
#else
  { ".rd760",	0000030 },
  { ".wr760",	0000031 },
#endif
  { ".gensym",	0000032 },
  { ".logout",	0000033 },
#if 1
  { ".realt",	0000034 },
#else
  { ".gsnam",	0000034 },
#endif
  { ".wsname",	0000035 },
  { ".upiset",	0000036 },
  { ".reset",	0000037 },
  { ".armove",	0000040 },
#if 1
  { ".dcontin",	0000041 },
  { ".cblk",	0000042 },
#else
  { ".wmar",	0000041 },
  { ".rrtim",	0000042 },
#endif
  { ".assign",	0000043 },
  { ".design",	0000044 },
  { ".rtime",	0000045 },
  { ".rdate",	0000046 },
#if 1
  { ".hang",	0000047 },
#else
  { ".rd710",	0000047 },
#endif
  { ".eofc",	0000050 },
  { ".iotlsr",	0000051 },
  { ".rsysi",	0000052 },
  { ".supset",	0000053 },
  { ".pdtime",	0000054 },
  { ".armrs",	0000055 },
  { ".ublat",	0000056 },
  { ".iopdl",	0000057 },
  { ".ityic",	0000060 },
  { ".master",	0000061 },
  { ".vstst",	0000062 },
#if 1
  { ".netac",	0000063 },
  { ".nets",	0000064 },
  { ".revive",	0000065 },
#else
  { ".dial",	0000063 },
  { ".dialw",	0000064 },
  { ".hangu",	0000065 },
#endif
  { ".dietim",	0000066 },
  { ".shutdn",	0000067 },
  { ".armoff",	0000070 },
  { ".ndis",	0000071 },
  { ".feed",	0000072 },
  { ".eval",	0000073 },
  { ".redef",	0000074 },
  { ".ifset",	0000075 },
  { ".utnam",	0000076 },
  { ".uinit",	0000077 },
  { ".ryear",	0000100 },
  { ".rlpdtm",	0000101 },
  { ".rdatim",	0000102 },
  { ".rchst",	0000103 },
  { ".rbtc",	0000104 },
  { ".dmpch",	0000105 },
  { ".swap",	0000106 },
  { ".mtape",	0000107 },
  { ".gennum",	0000110 },
  { ".netint",	0000111 },
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
  word_t word;

  set_address (memory, -1);
  while ((word = get_next_word (memory)) != -1)
    disassemble_word (memory, word, get_address (memory), cpu_model);
}

int
print_val (const char *format, int field)
{
  const char *p;
  const struct symbol *sym;
  int n = 0;

  for (p = format; *p; p++)
    {
      if (*p == '%')
	{
	  p++; /* assume %o */

	  sym = get_symbol_by_value (field);
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
  int printable;
  char ch[5];
  int i, n;

  sym = get_symbol_by_value (address);
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
      /* nothing */;
    }
  else if (OPCODE (word) == ITS_OPER)
    {
      const struct its_oper *oper;
      oper = lookup_oper (word);
      if (oper)
	{
	  n += printf ("%-8s ", oper->name);
	  n += print_val ("%o,", A (word));
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
	  n += print_val ("%o", Y (word));
	  if (X (word))
	    n += print_val ("(%o)", X (word));
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
	    n += print_val ("%o, ", DEVICE (word));
	}
      else if (!(op->type & PDP10_A_OPCODE))
	{
	  if (A (word) != 0 || !(op->type & PDP10_A_UNUSED))
	    n += print_val ("%o, ", A (word));
	}

      if (E (word) != 0 || !(op->type & PDP10_E_UNUSED))
	{
	  if (I (word))
	    n += printf ("@");
	  if (Y (word) != 0 ||
	      (I (word) != 0 && X (word) == 0) ||
	      (!(op->type & PDP10_E_UNUSED) && X (word) == 0))
	    n += print_val ("%o", Y (word));
	  if (X (word))
	    n += print_val ("(%o)", X (word));
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
