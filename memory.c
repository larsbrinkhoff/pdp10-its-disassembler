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
#include <stdlib.h>
#include <string.h>
#include "memory.h"

#define IMPURE(area) (((area)->flags & MEMORY_PURE) == 0)

static struct pdp10_area *
find_area (struct pdp10_memory *memory, int address)
{
  int i, j, k;

  if (memory->areas == 0)
    return NULL;

  i = 0;
  j = memory->areas - 1;

  while (i <= j)
    {
      k = (i + j) / 2;

      if (address < memory->area[k].start)
	j = k - 1;
      else if (address >= memory->area[k].end)
	i = k + 1;
      else
	return &memory->area[k];
    }

  return NULL;
}

void
init_memory (struct pdp10_memory *memory)
{
  memory->areas = 0;
  memory->area = NULL;
  memory->current_area = NULL;
  memory->current_address = 0;
}

static struct pdp10_area *
insert_area (struct pdp10_memory *memory, int i)
{
  memory->areas++;
  memory->area = realloc (memory->area, memory->areas * sizeof (struct pdp10_area));
  if (memory->area == NULL)
    {
      fprintf (stderr, "realloc failed\n");
      exit (1);
    }

  memmove (&memory->area[i+1], &memory->area[i],
	   (memory->areas - i - 1) * sizeof (struct pdp10_area));
  return &memory->area[i];
}

int
add_memory (struct pdp10_memory *memory, int address, int length, word_t *data)
{
  struct pdp10_area *area;
  int i;

  if (find_area (memory, address) != NULL)
    return -2;

  for (i = 0; i < memory->areas; i++)
    {
      if (address > memory->area[i].start)
	continue;
      break;
    }

  if (i > 0 && address == memory->area[i-1].end && IMPURE(&memory->area[i-1]))
    {
      int new_length;
      area = &memory->area[i-1];
      new_length = length + area->end - area->start;
      area->data = realloc (area->data, new_length * sizeof (word_t));
      if (area->data == NULL)
	{
	  fprintf (stderr, "realloc failed\n");
	  exit (1);
	}
      memcpy (&area->data[area->end - area->start], data, sizeof (word_t) * length);
      area->end += length;
      free (data);
      return 0;
    }

  area = insert_area (memory, i);
  area->start = address;
  area->end = address + length;
  area->flags = 0;
  area->data = data;

  return 0;
}

static void
remove_area (struct pdp10_memory *memory, int i)
{
  free (memory->area[i].data);  
  memmove (&memory->area[i], &memory->area[i+1],
	   (memory->areas - i - 1) * sizeof (struct pdp10_area));
  memory->areas--;
}

void
remove_memory (struct pdp10_memory *memory, int address, int length)
{
  struct pdp10_area *area;
  int end = address + length;
  int i;

  for (i = 0; i < memory->areas; )
    {
      area = &memory->area[i];
      if (area->start >= address && area->end <= end)
	/* The area is entirely within the range, so remove it completely */
	remove_area (memory, i);
      else if (area->end <= address || area->start >= end)
	/* The area is entirely outside the range; leave it alone. */
	i++;
      else if (area->start < address)
	{
	  /* The area is partly inside the range; remove last part. */
	  area->end = address;
	  i++;
	}
      else if (area->end > end)
	{
	  /* The area is partly inside the range; remove first part. */
	  area->start = end;
	  i++;
	}
      else
	{
	  /* Above should cover all cases! */
	  fprintf (stderr, "Bug!  Sholdn't get here.\n");
	  exit (1);
	}
    }
}

void
purify_memory (struct pdp10_memory *memory, int address, int length)
{
  struct pdp10_area *area;
  word_t *data;
  int i, end;

  end = address + length;
  for (i = address; i < end; i = area->end)
    {
      area = find_area (memory, i);
      if (area == NULL)
	return;
      if (!IMPURE (area)) /* Is the area already pure? */
	continue;

      if (area->start < i)
	{
	  /* Impure area needs to split off first part. */
	  area = insert_area (memory, area - memory->area);
	  area->end = i;
	  data = area->data + area->end - area->start;

	  /* New pure area. */
	  area++;
	  area->start = i;
	  area->flags |= MEMORY_PURE;
	  length = area->end - area->start;
	  area->data = malloc (length * sizeof (word_t));
	  memcpy (area->data, data, length);
	}
      if (area->end > end)
	{
	  /* Impure area needs to split off last part. */
	  area = insert_area (memory, area - memory->area);
	  area->end = i;
	  area->flags |= MEMORY_PURE;

	  /* New impure area. */
	  area++;
	  area->start = end;
	  length = area->end - area->start;
	  area->data = malloc (length * sizeof (word_t));
	  area->flags = 0;
	  memcpy (area->data, area[-1].data + area[-1].end - area[-1].start,
		  length);
	}
      else
	{
	  /* Whole area is inside the pure range. */
	  area->flags |= MEMORY_PURE;
	}
    }
}

int
set_address (struct pdp10_memory *memory, int address)
{
  struct pdp10_area *area;

  if (address == -1)
    {
      memory->current_address = -1;
      memory->current_area = NULL;
      return 0;
    }

  area = find_area (memory, address);
  if (area == NULL)
    return -1;

  memory->current_address = address;
  memory->current_area = area;

  return 0;
}

int
get_address (struct pdp10_memory *memory)
{
  return memory->current_address;
}

static word_t
getword (struct pdp10_area *area, int address)
{
  return area->data[address - area->start];
}

word_t
get_next_word (struct pdp10_memory *memory)
{
  if (memory->current_address == -1)
    {
      if (memory->areas == 0)
	return -1;

      memory->current_address = memory->area[0].start;
      memory->current_area = &memory->area[0];
    }
  else
    {
      memory->current_address++;
      if (memory->current_address >= memory->current_area->end)
	{
	  memory->current_area++;
	  if (memory->current_area - memory->area >= memory->areas)
	    return -1;
	  memory->current_address = memory->current_area->start;
	}
    }

  return getword (memory->current_area, memory->current_address);
}

word_t
get_word_at (struct pdp10_memory *memory, int address)
{
  struct pdp10_area *area;

  area = find_area (memory, address);
  if (area == NULL)
    return -1;

  return getword (area, address);
}

static void
setword (struct pdp10_area *area, int address, word_t word)
{
  area->data[address - area->start] = word;
}

void
set_word_at (struct pdp10_memory *memory, int address, word_t word)
{
  struct pdp10_area *area;

  area = find_area (memory, address);
  if (area == NULL) {
    word_t *data = malloc (sizeof word);
    *data = word;
    add_memory (memory, address, 1, data);
    return;
  }

  setword (area, address, word);
}

int
pure_word_at (struct pdp10_memory *memory, int address)
{
  struct pdp10_area *area;

  area = find_area (memory, address);
  if (area == NULL)
    return 0;

  return !IMPURE (area);
}
