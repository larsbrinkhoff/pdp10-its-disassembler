#include <stdio.h>

#include "dis.h"
#include "memory.h"

#define SQUOZE_MASK 0037777777777
#define SYHKL       0400000000000
#define SYKIL       0200000000000
#define SYLCL       0100000000000
#define SYGBL       0040000000000

void
sixbit_to_ascii (word_t sixbit, char *ascii)
{
  int i;

  for (i = 0; i < 6; i++)
    {
      ascii[i] = 040 + ((sixbit >> (6 * (5 - i))) & 077);
    }
  ascii[6] = 0;
}

void
squoze_to_ascii (word_t squoze, char *ascii)
{
  static char table[] =
    {
      ' ', '0', '1', '2', '3', '4', '5', '6',
      '7', '8', '9', 'a', 'b', 'c', 'd', 'e',
      'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
      'n', 'o', 'p', 'q', 'r', 's', 't', 'u',
      'v', 'w', 'x', 'y', 'z', '.', '$', '%'
    };
  int i;

  squoze &= SQUOZE_MASK;

  for (i = 0; i < 6; i++)
    {
      ascii[5-i] = table[squoze % 40];
      squoze /= 40;
    }
  ascii[6] = 0;
}

void
sblk_info (FILE *f, word_t word0, int cpu_model)
{
  int block_length;
  word_t word;
  int i;

  printf ("Start instruction:\n");
  disassemble_word (NULL, word0, -1, cpu_model);

  while ((word = get_word (f)) & SIGNBIT)
    {
      printf ("\n");
      block_length = -((word >> 18) | ((-1) & ~0777777));
      switch ((int)word & 0777777)
	{
	case 0:
	  {
	    char str[7];
	    int subblock_length;
	    int global = 0;

	    printf ("Symbol table:\n");

	    while (!global)
	      {
		squoze_to_ascii (get_word (f), str);
		printf ("  Header: %s\n", str);
		global = (strcmp (str, "global") == 0);
		subblock_length = get_word (f);
		for (i = 0; i < (-(subblock_length >> 18) - 2) / 2; i++)
		  {
		    word_t x = get_word (f);
		    squoze_to_ascii (x , str);
		    printf ("    Symbol %s = ", str);
		    printf ("%llo   (", get_word (f));
		    if (x & SYHKL)
		      printf (" halfkilled");
		    if (x & SYKIL)
		      printf (" killed");
		    if (x & SYLCL)
		      printf (" local");
		    if (x & SYGBL)
		      printf (" global");
		    printf (")\n");
		  }
	      }

	    goto checksum;
	  }
	case 1:
	  printf ("Undefined symbol table:\n");
	  break;
	case 2:
	  {
	    char str[7];

	    printf ("Indirect symbol table pointer:\n");

	    if (block_length != 4)
	      {
		printf ("  (unknown table format)\n");
		break;
	      }

	    sixbit_to_ascii (get_word (f), str);
	    printf ("  Device name: %s\n", str);
	    sixbit_to_ascii (get_word (f), str);
	    printf ("  File name 1: %s\n", str);
	    sixbit_to_ascii (get_word (f), str);
	    printf ("  File name 2: %s\n", str);
	    sixbit_to_ascii (get_word (f), str);
	    printf ("  File sname:  %s\n", str);
	    goto checksum;
	  }
	case 3:
	  {
	    int subblock_length;
	    char str[7];

	    word = get_word (f);
	    subblock_length = -((word >> 18) | ((-1) & ~0777777));
	    switch ((int)word & 0777777)
	      {
	      case 1:
		printf ("Assembly info:\n");
		sixbit_to_ascii (get_word (f), str);
		printf ("  User name:          %s\n", str);
		printf ("  Disk format time:   %012llo\n", get_word (f));
		sixbit_to_ascii (get_word (f), str);
		printf ("  Source file device: %s\n", str);
		sixbit_to_ascii (get_word (f), str);
		printf ("  Source file name 1: %s\n", str);
		sixbit_to_ascii (get_word (f), str);
		printf ("  Source file name 2: %s\n", str);
		sixbit_to_ascii (get_word (f), str);
		printf ("  Source file sname:  %s\n", str);
		goto checksum;
	      case 2:
		printf ("Debugging info:\n");
		break;
	      default:
		printf ("Unknown miscellaneous info:\n");
		break;
	      }

	    printf ("    (%d words)\n", subblock_length);
	    for (i = 0; i < subblock_length; i++)
	      {
		get_word (f);
	      }

	    goto checksum;
	  }
	default:
	  printf ("Unknown information:\n");
	  break;
	}

      printf ("(%d words)\n", block_length);
      for (i = 0; i < block_length; i++)
	{
	  get_word (f);
	}

    checksum:
      word = get_word (f);
      /*printf ("checksum: %012llo\n", word);*/
    }

  printf ("\nDuplicate start instruction:\n");
  disassemble_word (NULL, word, -1, cpu_model);
}
