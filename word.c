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

struct word_format *input_word_format = &its_word_format;
struct word_format *output_word_format = &its_word_format;

static struct word_format *word_formats[] = {
  &aa_word_format,
  &bin_word_format,
  &cadr_word_format,
  &core_word_format,
  &data8_word_format,
  &dta_word_format,
  &its_word_format,
  &oct_word_format,
  &pt_word_format,
  &sail_word_format,
  &tape_word_format,
  &tape7_word_format,
  &x_word_format,
  NULL
};
static word_t checksum;

void
usage_word_format (void)
{
  int i;

  fprintf (stderr, "Valid word formats are:");
  for (i = 0; word_formats[i] != NULL; i++)
    fprintf (stderr, " %s", word_formats[i]->name);
  fprintf (stderr, "\n");
}

static int
parse_word_format (const char *string, struct word_format **word_format)
{
  int i;

  for (i = 0; word_formats[i] != NULL; i++)
    if (strcmp (string, word_formats[i]->name) == 0)
      {
        *word_format = word_formats[i];
        return 0;
      }

  return -1;
}

int
parse_input_word_format (const char *string)
{
  return parse_word_format (string, &input_word_format);
}

int
parse_output_word_format (const char *string)
{
  return parse_word_format (string, &output_word_format);
}

word_t
get_word (FILE *f)
{
  if (input_word_format->get_word == NULL)
    {
      fprintf (stderr, "word format \"%s\" not supported for input\n", input_word_format->name);
      exit (1);
    }
  return input_word_format->get_word (f);
}

void
rewind_word (FILE *f)
{
  if (input_word_format->rewind_word == NULL)
    {
      rewind (f);
      return;
    }

  input_word_format->rewind_word (f);
}

void
write_word (FILE *f, word_t word)
{
  if (output_word_format->write_word == NULL)
    {
      fprintf (stderr, "word format \"%s\" not supported for output\n", output_word_format->name);
      exit (1);
    }
  output_word_format->write_word (f, word);
}

void
flush_word (FILE *f)
{
  if (output_word_format->flush_word == NULL)
    return;

  output_word_format->flush_word (f);
}

void
reset_checksum (word_t word)
{
  checksum = word;
}

void
check_checksum (word_t word)
{
  if (word != checksum)
    printf ("  [WARNING: bad checksum, %012llo /= %012llo]\n", word, checksum);
}

word_t
get_checksummed_word (FILE *f)
{
  word_t word = get_word (f);

  checksum = (checksum << 1) + (checksum >> 35) + word;
  checksum &= 0777777777777ULL;

  return word;
}
