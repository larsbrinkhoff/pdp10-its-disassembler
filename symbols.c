/* Copyright (C) 2018, 2020 Lars Brinkhoff <lars@nocrew.org>
   Copyright (C) 2018, 2019, 2020 Adam Sampson <ats@offog.org>

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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dis.h"
#include "symbols.h"

#define MAX_SYMBOLS 16384

static int symbols_mode = SYMBOLS_NONE;

void
usage_symbols_mode (void)
{
  fprintf (stderr, "Valid symbol modes are: none, ddt, all.\n");
}

int
parse_symbols_mode (const char *string)
{
  if (strcmp (string, "none") == 0)
    symbols_mode = SYMBOLS_NONE;
  else if (strcmp (string, "ddt") == 0)
    symbols_mode = SYMBOLS_DDT;
  else if (strcmp (string, "all") == 0)
    symbols_mode = SYMBOLS_ALL;
  else
    return -1;

  return 0;
}

struct symbol symbols[MAX_SYMBOLS];
int num_symbols = 0;

typedef enum {
  SORT_NONE,
  SORT_NAME,
  SORT_VALUE
} sort_mode_t;
static sort_mode_t sorted = SORT_NONE;

void
add_symbol (const char *name, word_t value, int flags)
{
  int i = num_symbols++;
  char *p;

  if (num_symbols > MAX_SYMBOLS)
    {
      fprintf (stderr, "Too many symbols\n");
      exit (1);
    }

  /* Copy the name, with trailing spaces stripped off. */
  p = strdup (name);
  symbols[i].name = p;
  while (*p)
    p++;
  while (p > symbols[i].name && *(p - 1) == ' ')
    *--p = '\0';

  symbols[i].value = value;
  symbols[i].sequence = num_symbols;
  symbols[i].flags = flags;

  sorted = SORT_NONE;
}

/* When searching symbols, we can't assume that anything other than
   the field we're searching for is valid, as one of the arguments
   might be the key. */

static int
compare_name_search (const void *a, const void *b)
{
  const struct symbol *sa = a;
  const struct symbol *sb = b;

  return strcmp (sa->name, sb->name);
}

static int
compare_value_search (const void *a, const void *b)
{
  const struct symbol *sa = a;
  const struct symbol *sb = b;

  if (sa->value == sb->value)
    return 0;
  else if (sa->value < sb->value)
    return -1;
  else
    return 1;
}

/* When sorting, all the fields are valid. Order first by the key we're
   sorting by, then by visibility so the most visible symbols come
   first, then by their original declaration order. */

static int
concealment (const struct symbol *s)
{
  if (s->flags & SYMBOL_KILLED)
    return 2;
  if (s->flags & SYMBOL_HALFKILLED)
    return 1;
  return 0;
}

static int
compare_default (const void *a, const void *b)
{
  const struct symbol *sa = a;
  const struct symbol *sb = b;

  int r = concealment (sa) - concealment (sb);
  if (r != 0)
    return r;

  if (sa->sequence < sb->sequence)
    return -1;
  else
    return 1;
}

static int
compare_name_sort (const void *a, const void *b)
{
  int r = compare_name_search (a, b);

  if (r == 0)
    return compare_default (a, b);
  else
    return r;
}

static int
compare_value_sort (const void *a, const void *b)
{
  int r = compare_value_search (a, b);

  if (r == 0)
    return compare_default (a, b);
  else
    return r;
}

static void
sort_by (sort_mode_t wanted)
{
  if (sorted != wanted)
    {
      qsort (symbols, num_symbols, sizeof *symbols,
	     wanted == SORT_NAME ? compare_name_sort : compare_value_sort);
      sorted = wanted;
    }
}

static const struct symbol *
hint_accumulator (const struct symbol *first, word_t value)
{
  const struct symbol *symbol = first;

  /* Look for a single-letter symbol that matches. */
  do
    {
      if (strlen (symbol->name) == 1)
	return symbol;
      symbol++;
    }
  while (symbol < symbols + num_symbols && symbol->value == value);

  /* Failing that, try two-letter symbols. */
  symbol = first;
  do
    {
      if (strlen (symbol->name) == 2)
	return symbol;
      symbol++;
    }
  while (symbol < symbols + num_symbols && symbol->value == value);

  return first;
}

static const struct symbol *
hint_address (const struct symbol *first, word_t value)
{
  if (value < 020)
    return hint_accumulator (first, value);

  return first;
}

static const struct symbol *
hint_offset (const struct symbol *first, word_t value)
{
  const struct symbol *symbol = first;

  /* Look for a symbol with more than one letter. */
  do
    {
      if (strlen (symbol->name) > 1)
	return symbol;
      symbol++;
    }
  while (symbol < symbols + num_symbols && symbol->value == value);

  return first;
}

static const struct symbol *
hint_channel (const struct symbol *first, word_t value)
{
  const struct symbol *symbol = first;

  /* Look for a symbol containing CH. */
  do
    {
      if (strstr (symbol->name, "ch"))
	return symbol;
      symbol++;
    }
  while (symbol < symbols + num_symbols && symbol->value == value);

  /* Second try, symbols that end with C. */
  symbol = first;
  do
    {
      if (symbol->name[strlen (symbol->name) - 1] == 'c')
	return symbol;
      symbol++;
    }
  while (symbol < symbols + num_symbols && symbol->value == value);

  return first;
}

static const struct symbol *
hint_xctr (const struct symbol *first, word_t value)
{
  const struct symbol *symbol = first;

  /* Look for matching symbol that begins with X. */
  do
    {
      if (*symbol->name == 'x')
	return symbol;
      symbol++;
    }
  while (symbol < symbols + num_symbols && symbol->value == value);

  return first;
}

const struct symbol *
get_symbol_by_value (word_t value, int hint)
{
  struct symbol key = { NULL, value, -1, 0 };
  const struct symbol *first;

  if (symbols_mode == SYMBOLS_NONE || hint == HINT_NUMBER)
    return NULL;

  sort_by (SORT_VALUE);
  first = bsearch (&key, symbols, num_symbols, sizeof *symbols,
		   compare_value_search);

  if (first == NULL)
    return NULL;

  /* Wind the pointer back to find the first symbol that matches. */
  while (first > symbols && (first - 1)->value == value)
    first--;

  switch (hint)
    {
    case HINT_ACCUMULATOR: first = hint_accumulator (first, value); break;
    case HINT_CHANNEL:     first = hint_channel (first, value); break;
    case HINT_ADDRESS:     first = hint_address (first, value); break;
    case HINT_OFFSET:      first = hint_offset (first, value); break;
    case HINT_IMMEDIATE:   first = hint_offset (first, value); break;
    case HINT_XCTR:        first = hint_xctr (first, value); break;
    }

  if (symbols_mode == SYMBOLS_DDT)
    {
      if (first != NULL && first->flags & (SYMBOL_KILLED | SYMBOL_HALFKILLED))
	return NULL;
    }

  return first;
}

const struct symbol *
get_symbol_by_name (const char *name)
{
  struct symbol key = { name, 0, -1, 0 };
  const struct symbol *first;

  sort_by (SORT_NAME);
  first = bsearch (&key, symbols, num_symbols, sizeof *symbols,
		   compare_name_search);

  if (first == NULL)
    return NULL;

  /* Wind the pointer back to find the first symbol that matches. */
  while (first > symbols && strcmp ((first - 1)->name, name) == 0)
    first--;

  return first;
}

word_t
get_symbol_value (const char *name)
{
  const struct symbol *symbol = get_symbol_by_name (name);
  if (symbol == NULL)
    return -1;
  else
    return symbol->value;
}
