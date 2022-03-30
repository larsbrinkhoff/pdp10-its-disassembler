/* Copyright (C) 2022 Lars Brinkhoff <lars@nocrew.org>

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
#include <stdlib.h>
#include <string.h>

#include "dis.h"
#include "memory.h"
#include "symbols.h"

#define PAGESIZE  512

#define MAGIC     01000LL

#define PAGE_READ	(0100000LL)
#define PAGE_WRITE	(0040000LL)
#define PAGE_XCT	(0020000LL)
#define PAGE_TRAP	(0001000LL)
#define PAGE_COPY	(0000400LL)
#define PAGE_ACCESS	(0000040LL)

#define HALF      0777777
#define LH(WORD)  (((WORD) >> 18) & HALF)
#define RH(WORD)  ((WORD) & HALF)

static void
read_page (FILE *f, word_t *data)
{
  word_t word;
  int i;
  for (i = 0; i < PAGESIZE; i++)
    {
      word = get_word (f);
      if (word == -1)
	return;
      data[i] = word;
    }
}

static void
get_page (FILE *f, int page, word_t *map, struct pdp10_memory *memory)
{
  word_t *data, *core = NULL;
  int i, address;

  for (i = 0; i < PAGESIZE; i++)
    {
      if (RH (map[i]) == 0 || RH (map[i]) != page)
	continue;

      address = (LH (map[i]) & 0777) * PAGESIZE;
      data = malloc (PAGESIZE * sizeof (word_t));
      if (core == NULL)
	read_page (f, core = data);
      else
	memcpy (data, core, PAGESIZE * sizeof (word_t));
      add_memory (memory, address, PAGESIZE, data);
      if ((map[i] & PAGE_WRITE) == 0)
	purify_memory (memory, address, PAGESIZE);
    }
}

static void
read_tenex (FILE *f, struct pdp10_memory *memory, int cpu_model)
{
  char symbol[7];
  word_t map[PAGESIZE];
  word_t word;
  int count, access;
  int i, end = 0;

  word = get_word (f);

  if (LH (word) != MAGIC)
    {
      fprintf (stderr, "Not TENEX sharable save format.\n");
      exit (1);
    }

  fprintf (output_file, "TENEX format\n\n");

  count = RH (word);
  memset (map, 0, sizeof map);

  fprintf (output_file, "Page map:\n");
  fprintf (output_file, "File Page  Memory Page  Access\n");
  for (i = 0; i < count; i++)
    {
      word = get_word (f);
      map[i] = word;

      access = LH (word) & 0777000LL;
      if (access & 0400000LL)
	{
	  access &= ~0400000LL;
	  access |= PAGE_COPY;
	}
      fprintf (output_file, "%06llo     %03llo          %06o ",
	       RH (word), LH (word) & 0777, access);

      if (RH (word) > end)
	end = RH (word);

      fprintf (output_file, access & PAGE_READ ?  "r" : "-");
      fprintf (output_file, access & PAGE_WRITE ? "w" : "-");
      fprintf (output_file, access & PAGE_XCT ?   "x" : "-");
      fprintf (output_file, access & PAGE_TRAP ?  "t" : "-");
      fprintf (output_file, access & PAGE_COPY ?  "c" : "-");
      fprintf (output_file, access & PAGE_ACCESS ?"a" : "-");
      fprintf (output_file, "\n");
    }

  start_instruction = get_word (f);
  fprintf (output_file, "Start instruction:\n");
  disassemble_word (NULL, start_instruction, -1, cpu_model);

  /* The rest of the first two pages are unused. */
  for (i = 0; i < 2 * PAGESIZE - count - 2; i++)
    get_word (f);

  for (i = 2; i <= end; i++)
    get_page (f, i, map, memory);

  while (!feof (f))
    {
      squoze_to_ascii (get_word (f), symbol);
      add_symbol (symbol, get_word (f), 0);
    }
}

static void
write_page (FILE *f, struct pdp10_memory *memory, int page)
{
  int address, end = PAGESIZE * page + PAGESIZE;
  for (address = PAGESIZE * page; address < end; address++)
    write_word (f, get_word_at (memory, address));
}

static void
write_symbols (FILE *f)
{
  int i;
  for (i = 0; i < num_symbols; i++)
    {
      word_t word = ascii_to_squoze (symbols[i].name);
      write_word (f, word);
      write_word (f, symbols[i].value);
    }
}

static int
page_access (struct pdp10_memory *memory, int page)
{
  int address, end, access;

  end = PAGESIZE * page + PAGESIZE;
  access = 0;

  for (address = PAGESIZE * page; address < end; address++)
    {
      word_t word = get_word_at (memory, address);
      if (word != 0 && word != -1)
	{
	  access |= PAGE_READ | PAGE_XCT;
	  if (!pure_word_at (memory, address))
	    access |= PAGE_WRITE;
	}
    }

  return access;
}

static int
make_map (word_t *map, struct pdp10_memory *memory)
{
  int i, n = 0;

  for (i = 0; i < 512; i++)
    {
      word_t access = page_access (memory, i);
      map[i] = access << 18;
      if (access != 0)
	{
	  map[i] |= i << 18;
	  map[i] |= 2 + n++;
	}
    }

  return n;
}

static void
write_tenex (FILE *f, struct pdp10_memory *memory)
{
  word_t map[256];
  int i, n;

  n = make_map (map, memory);

  /* Type and count word. */
  write_word (f, (MAGIC << 18) | n);

  /* Page map follows. */
  for (i = 0; i < n; i++)
    {
      if (map[i] != 0)
	write_word (f, map[i]);
    }

  /* Start instruction. */
  write_word (f, start_instruction);

  /* The rest of the first two pages is unused. */
  for (i = 0; i < 2 * PAGESIZE - n - 2; i++)
    write_word (f, 0);

  /* Data pages follow. */
  for (i = 0; i < n; i++)
    write_page (f, memory, i);

  /* Last comes the symbol table. */
  write_symbols (f);
}

struct file_format tenex_file_format = {
  "tenex",
  read_tenex,
  write_tenex
};
