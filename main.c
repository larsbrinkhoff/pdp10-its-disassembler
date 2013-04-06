#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "dis.h"
#include "opcode/pdp10.h"
#include "memory.h"

int
main (int argc, char **argv)
{
  int cpu_model = PDP10_KS10_ITS;
  struct pdp10_memory memory;
  FILE *file;
  word_t word;

  if (argc != 2)
    {
      fprintf (stderr, "Usage: %s <file>\n", argv[0]);
      exit (1);
    }

  file = fopen (argv[1], "rb");
  if (file == NULL)
    {
      fprintf (stderr, "%s: Error opening %s: %s\n",
	       argv[0], argv[1], strerror (errno));
      return 1;
    }

  init_memory (&memory);

  word = get_word (file);
  rewind_word (file);
  if (word == 0)
    read_pdump (file, &memory, cpu_model);
  else
    read_sblk (file, &memory, cpu_model);

  while ((word = get_word (file)) != -1)
    printf ("(extra word: %012llo)\n", word);

  printf ("\nDisassembly:\n\n");
  dis (&memory, cpu_model);

  return 0;
}
