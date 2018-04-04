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

  for (;gets (line);)
    {
      if (strncmp (line, "<p id=\"u8lump\">", n) == 0)
	{
	  convert (line + n);
	  for (;gets (line);)
	    {
	      if (strncmp (line, "</p>", 4) == 0)
		return 0;
	      convert (line);
	    }
	}
    }
  
  return 0;
}
