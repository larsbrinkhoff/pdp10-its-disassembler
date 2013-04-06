#include <stdio.h>

#include "dis.h"

extern word_t get_its_word (FILE *f);

int
main (int argc, char **argv)
{
  word_t word1, word2;
  FILE *f;

  if (argc != 2)
    {
      fprintf (stderr, "Usage: %s <file>\n", argv[0]);
      exit (1);
    }

  f = fopen (argv[1], "rb");

  while ((word1 = get_its_word (f)) != -1)
    {
      putchar ((word1 >> 28) & 0xff);
      putchar ((word1 >> 20) & 0xff);
      putchar ((word1 >> 12) & 0xff);
      putchar ((word1 >>  4) & 0xff);

      word2 = get_its_word (f);
      if (word2 == -1)
	{
	  putchar ((word1 << 4) & 0xf0);
	}
      else
	{
	  putchar (((word1 << 4) & 0xf0) | ((word2 >> 32) & 0x0f));
	  putchar ((word2 >> 24) & 0xff);
	  putchar ((word2 >> 16) & 0xff);
	  putchar ((word2 >>  8) & 0xff);
	  putchar ((word2 >>  0) & 0xff);
	}
    }

  return 0;
}
