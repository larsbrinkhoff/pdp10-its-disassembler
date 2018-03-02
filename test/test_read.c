#include "dis.h"

int main (void)
{
  int i;

  for (i = 0; i < 10; i++)
    {
      word_t word = get_word (stdin);
      fprintf (stderr, "%012llo\n", word);
    }

  return 0;
}
