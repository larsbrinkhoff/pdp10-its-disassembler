/* Copyright (C) 2013, 2022 Lars Brinkhoff <lars@nocrew.org>
   Copyright (C) 2020 Adam Sampson <ats@offog.org>

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

struct file_format *input_file_format = NULL;
struct file_format *output_file_format = NULL;

static struct file_format *file_formats[] = {
  &atari_file_format,
  &cross_file_format,
  &csave_file_format,
  &dmp_file_format,
  &exb_file_format,
  &fasl_file_format,
  &hex_file_format,
  &iml_file_format,
  &lda_file_format,
  &mdl_file_format,
  &nsave_file_format,
  &palx_file_format,
  &pdump_file_format,
  &raw_file_format,
  &rim10_file_format,
  &sblk_file_format,
  &exe_file_format,
  &tenex_file_format,
  NULL
};

void
usage_file_format (void)
{
  int i;

  fprintf (stderr, "Valid file formats are:");
  for (i = 0; file_formats[i] != NULL; i++)
    fprintf (stderr, " %s", file_formats[i]->name);
  fprintf (stderr, "\n");
}

static struct file_format *
parse_file_format (const char *string)
{
  int i;

  for (i = 0; file_formats[i] != NULL; i++)
    if (strcmp (string, file_formats[i]->name) == 0)
      {
        return file_formats[i];
      }

  return NULL;
}

int
parse_input_file_format (const char *string)
{
  input_file_format = parse_file_format (string);
  return input_file_format == NULL ? -1 : 0;
}

int
parse_output_file_format (const char *string)
{
  output_file_format = parse_file_format (string);
  return output_file_format == NULL ? -1 : 0;
}

void
guess_input_file_format (FILE *file)
{
  word_t word = get_word (file);
  rewind_word (file);

  if ((word >> 18) == 01776)
    input_file_format = &exe_file_format;
  else if ((word >> 18) == 01000)
    input_file_format = &tenex_file_format;
  else if (word == 0)
    input_file_format = &pdump_file_format;
  else
    input_file_format = &sblk_file_format;
}
