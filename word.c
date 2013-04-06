#include <stdio.h>

#include "dis.h"

extern word_t get_bin_word (FILE *f);
extern word_t get_its_word (FILE *f);
extern word_t get_x_word (FILE *f);
extern void rewind_bin_word (FILE *f);
extern void rewind_its_word (FILE *f);
extern void rewind_x_word (FILE *f);

int file_36bit_format = FORMAT_ITS;

word_t
get_word (FILE *f)
{
  switch (file_36bit_format)
    {
    case FORMAT_BIN:	return get_bin_word (f);
    case FORMAT_ITS:	return get_its_word (f);
    case FORMAT_X:	return get_x_word (f);
    }

  return -1;
}

void
rewind_word (FILE *f)
{
  switch (file_36bit_format)
    {
    case FORMAT_BIN:	return rewind_bin_word (f);
    case FORMAT_ITS:	return rewind_its_word (f);
    case FORMAT_X:	return rewind_x_word (f);
    }
}
