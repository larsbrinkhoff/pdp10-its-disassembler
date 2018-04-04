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

char line[1024];

void convert (char *line)
{
  long long x = 0;
  int i;

  for (i = 0; i < 12; i ++)
    {
      x <<= 3;
      x += (*line++ - '0');
    }

  putchar ((x >> 29) & 0177);
  putchar ((x >> 22) & 0177);
  putchar ((x >> 15) & 0177);
  putchar ((x >>  8) & 0177);
  putchar (((x >> 1) & 0177) + ((x & 1) << 7));
}

int main (void)
{
  int n = strlen ("<p id=\"u8lump\">");

  for (;fgets (line, sizeof line, stdin);)
    {
      if (strncmp (line, "<p id=\"u8lump\">", n) == 0)
	{
	  convert (line + n);
	  for (;fgets (line, sizeof line, stdin);)
	    {
	      if (strncmp (line, "</p>", 4) == 0)
		return 0;
	      convert (line);
	    }
	}
    }
  
  return 0;
}
