#include <stdio.h>

#include "dis.h"

static inline int
get_byte (FILE *f)
{
  int c = fgetc (f);
  return c == EOF ? 0 : c;
}

word_t
get_x_word (FILE *f)
{
  word_t word;

  if (feof (f))
    return -1;

  word =  (word_t)get_byte (f) << 32;
  if (feof (f))
    return -1;
  word |= (word_t)get_byte (f) << 24;
  word |= (word_t)get_byte (f) << 16;
  word |= (word_t)get_byte (f) <<  8;
  word |= (word_t)get_byte (f) <<  0;

  if (word > WORDMASK)
    {
      fprintf (stderr, "[error in 36/8 format]\n");
      exit (1);
    }

  return word;
}

void
rewind_x_word (FILE *f)
{
  rewind (f);
}
