#include <time.h>
#include <stdlib.h>
#include "dis.h"

word_t rand36 (void)
{
  int i;
  word_t x = 0;

  for (i = 0; i < 36; i++)
    x ^= (word_t)rand() << i;

  return x & 0777777777777LL;
}

int main (void)
{
  int i;
  time_t t;
  
  srand (time (&t));

  for (i = 0; i < 10; i++)
    {
      word_t word = rand36 ();
      fprintf (stderr, "%012llo\n", word);
      write_its_word (stdout, word);
    }

  flush_its_word (stdout);
  return 0;
}
