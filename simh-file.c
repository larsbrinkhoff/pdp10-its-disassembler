/* Copyright (C) 2024 Lars Brinkhoff <lars@nocrew.org>

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

/*
  File format to output SIMH deposit commands.

  Converts a core image to file to a series of SIMH deposit commands.
  If there is a start address, there will also be a GO command at the end.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dis.h"
#include "memory.h"

static void
write_location (FILE *f, struct pdp10_memory *memory, int address)
{
  word_t data = get_word_at (memory, address);
  if (data >= 0)
    {
      data &= 0777777777777LL;
      fprintf (f, "d %06o %012llo\n", address, data);
    }
}

static void
write_simh (FILE *f, struct pdp10_memory *memory)
{
  int i;

  /* Memory contents, as deposit commands. */
  for (i = 0; i <= 0777777; i++)
    write_location (f, memory, i);

  /* Start. */
  if (start_instruction <= 0)
    ;
  else if (((start_instruction & 0777000000000LL) == JRST) ||
	   start_instruction <= 0777777)
    fprintf (f, "go %06llo\n", start_instruction & 0777777);
  else
    fprintf (f, ";execute %012llo\n", start_instruction);
}

static int
whitespace(char c)
{
  switch (c)
    {
    case ' ':
    case '\t':
    case '\r':
    case '\n':
      return 1;
    default:
      return 0;
    }
}

static int
whitespace_or_nul(char c)
{
  return whitespace (c) || c == 0;
}

static void
fatal (char *format, char *line)
{
  fprintf (stderr, format, line);
  exit (1);
}

static void
deposit (char *line, struct pdp10_memory *memory)
{
  char *p;
  word_t *data;
  unsigned long x, address = strtoul (line, &p, 8);
  if (!whitespace (*p))
    fatal ("Invalid DEPOSIT arguments: \"%s\"\n", line);

  while (whitespace(*p))
    p++;
  if (*p == 0)
    fatal ("Invalid DEPOSIT arguments: \"%s\"\n", line);

  data = malloc (sizeof (word_t));
  if (data == NULL)
    {
      fprintf (stderr, "Out of memory\n");
      exit (1);
    }
  x = strtoul (p, &p, 8);
  if (!whitespace_or_nul (*p))
    fatal ("Invalid DEPOSIT arguments: \"%s\"\n", line);

  *data = x;
  add_memory (memory, address, 1, data);
}

static void
start (char *line)
{
  char *p;
  unsigned long address = strtoul (line, &p, 8);
  if (p == line || !whitespace_or_nul (*p))
    fatal ("Invalid GO argument: \"%s\"\n", line);
  start_instruction = JRST | address;
}

static void
read_line (char *line, struct pdp10_memory *memory)
{
  char *p = line;
  size_t n;

  while (whitespace(*p))
    p++;
  if (*p == 0 || *p == ';' || *p == '#')
    return;
  line = p++;
  while (!whitespace_or_nul(*p))
    p++;
  if (*p == 0)
    fatal ("SIMH command has no argument: %s\n", line);
  *p++ = 0;
  n = strlen(line);
  if (strncasecmp (line, "deposit", n) == 0)
    deposit (p, memory);
  else if (strncasecmp (line, "go", n) == 0)
    start (p);
  else
    fatal ("Unsupported SIMH command: %s\n", line);
}

static void
read_simh (FILE *f, struct pdp10_memory *memory, int cpu_model)
{
  static char line[100];
  (void)cpu_model;

  fprintf (output_file, ";SIMH script\n\n");

  for (;;)
    {
      if (fgets (line, sizeof line, f) == NULL)
	return;
      read_line (line, memory);
    }
}

struct file_format simh_file_format = {
  "simh",
  read_simh,
  write_simh
};
