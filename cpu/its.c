#include "memory.h"

typedef long long word_t;

void its_muuo (word_t IR, word_t MA)
{
  switch (IR >> 27)
    {
    case 043:
      fprintf (stderr, ".CALL\n");
      exit (0);
    default:
      fprintf (stderr, "Unknown MUUO\n");
      exit (0);
    }
}
