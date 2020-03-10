/* Copyright (C) 2020 Lars Brinkhoff <lars@nocrew.org>

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

/* Support functions to translate ITS file names to acceptable Unix
   file names, compatible with how itstar does it. */

#include <ctype.h>
#include <string.h>
#include "dis.h"

/* Compatible with itstar. */
void
weenixname (char *name)
{
  char *p;

  /* Use characters compatible with Unix. */
  for (p = name; *p != 0; p++)
    {
      switch (*p)
        {
        case '.': *p = '_'; break;
        case '/': *p = '{'; break;
        case '_': *p = '}'; break;
        case ' ': *p = '~'; break;
        default: *p = tolower (*p); break;
        }
    }

  /* Remove trailing spaces. */
  while (*--p == '~')
    *p = 0;
}

void
weenixpath (char *output, word_t dir, word_t fn1, word_t fn2)
{
  char s1[7], s2[7], s3[7];

  sixbit_to_ascii(fn1, s2);
  sixbit_to_ascii(fn2, s3);
  weenixname (s2);
  weenixname (s3);

  if (dir == -1)
    sprintf (output, "%s.%s", s2, s3);
  else
    {
      sixbit_to_ascii(dir, s1);
      weenixname (s1);
      sprintf (output, "%s/%s.%s", s1, s2, s3);
    }
}
