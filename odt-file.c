/* Copyright (C) 2025 Lars Brinkhoff <lars@nocrew.org>

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
  File format to output ODT deposit commands.

  Converts a core image to a series of ODT deposit commands.  If there
  is a start address, there will also be a G command at the end.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dis.h"
#include "memory.h"

#define LF 012
#define CR 015
#define MASK 0177777
#define NO_ADDRESS -2
static int previous_address;

static int quantity, address;

static void
write_location (FILE *f, struct pdp10_memory *memory, int address)
{
  word_t data = get_word_at (memory, address);
  if (data < 0)
    return;

  /* First, close the previous location (if there was one).  If this
  location follows the previous, close with "^J" to open the next
  location.  Otherwise, close with "^M" and open a new location with
  "/".  Then enter one word of data, and leave the location open. */

  data &= MASK;
  if (address == previous_address + 1)
    fputc (LF, f);
  else if (previous_address != NO_ADDRESS)
    fputc (CR, f);
  if (address != previous_address + 1)
    fprintf (f, "%06o/", address);
  fprintf (f, "%06llo", data);
  previous_address = address;
}

static void
write_odt (FILE *f, struct pdp10_memory *memory)
{
  int i;

  previous_address = NO_ADDRESS;

  /* Memory contents, as deposit commands. */
  for (i = 0; i <= MASK; i++)
    write_location (f, memory, i);

  /* Make sure to close last location. */
  if (previous_address != NO_ADDRESS)
    fputc (CR, f);

  /* Start. */
  if (start_instruction > 0)
    fprintf (f, "%06lloG", start_instruction & MASK);
}

static void
deposit (struct pdp10_memory *memory)
{
  word_t *data = malloc (sizeof (word_t));
  if (data == NULL)
    {
      fprintf (stderr, "Out of memory\n");
      exit (1);
    }
  *data = quantity;
  add_memory (memory, address, 1, data);
}

static void
read_char (char c, struct pdp10_memory *memory)
{
  switch (c)
    {
    case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7':
      if (quantity < 0)
	quantity = 0;
      quantity *= 010;
      quantity += c - '0';
      break;
    case LF:
    case CR:
      if (address == -1)
        break;
      if (quantity < 0)
	quantity = -quantity;
      deposit (memory);
      quantity = 0;
      if (c == LF)
        address++;
      else
        address = -1;
      break;
    case '/':
      address = quantity;
      quantity = get_word_at (memory, address);
      if (quantity == -1)
	quantity = 0;
      else
	quantity = -quantity;
      break;
    case 'G':
      start_instruction = quantity;
      quantity = 0;
      break;
    }
}

static void
read_odt (FILE *f, struct pdp10_memory *memory, int cpu_model)
{
  (void)cpu_model;

  fprintf (output_file, ";ODT commands\n\n");

  quantity = 0;
  address = -1;

  for (;;)
    {
      int c = fgetc (f);
      if (c == EOF)
	return;
      read_char (c, memory);
    }
}

struct file_format odt_file_format = {
  "odt",
  read_odt,
  write_odt
};
