#include <stdio.h>

#include "dis.h"

extern word_t get_its_word (FILE *f);

int
main (int argc, char **argv)
{
  word_t word;
  FILE *f;

  if (argc != 2)
    {
      fprintf (stderr, "Usage: %s <file>\n", argv[0]);
      exit (1);
    }

  f = fopen (argv[1], "rb");
  file_36bit_format = FORMAT_ITS;

  while ((word = get_its_word (f)) != -1)
    {
      putchar ((word >> 32) & 0x0f);
      putchar ((word >> 24) & 0xff);
      putchar ((word >> 16) & 0xff);
      putchar ((word >>  8) & 0xff);
      putchar ((word >>  0) & 0xff);
    }

  return 0;
}
