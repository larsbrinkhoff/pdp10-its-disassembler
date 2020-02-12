/* Copyright (C) 2018 Lars Brinkhoff <lars@nocrew.org>

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

char line[1024];

void convert (char *line)
{
  word_t x = 0;
  int i;

  for (i = 0; i < 12; i ++)
    {
      x <<= 3;
      x += (*line++ - '0');
    }

  write_word (stdout, x);
}

int main (void)
{
  int n = strlen ("<p id=\"u8lump\">");

  for (;fgets (line, sizeof line, stdin);)
    {
      if (strncmp (line, "<p id=\"u8lump\">", n) == 0)
	{
	  /* HTML format - https://www.saildart.org/SOMETHING_blob */
	  convert (line + n);
	  for (;fgets (line, sizeof line, stdin);)
	    {
	      if (strncmp (line, "</p>", 4) == 0)
		return 0;
	      convert (line);
	    }
	}
      else
	{
	  /* Octal format - https://www.saildart.org/SOMETHING_octal */
	  convert (line);
	}
    }

  flush_word (stdout);
  return 0;
}
