#include <stdio.h>
#include <getopt.h>

#include "dis.h"
#include "memory.h"

static void
usage (char **argv)
{
  fprintf (stderr, "Usage: %s [-F<input file format>] [-W<input word format>]\n"
                   "   [-O<output file format>] [-X<output word format>] [<files...>]\n\n", argv[0]);
  usage_file_format ();
  usage_word_format ();
  exit (1);
}

int
main (int argc, char **argv)
{
  struct pdp10_memory memory;
  FILE *file;
  int opt;

  output_file = stderr;
  file = stdin;

  while ((opt = getopt (argc, argv, "W:X:F:O:")) != -1)
    {
      switch (opt)
        {
        case 'W':
          if (parse_input_word_format (optarg))
            usage (argv);
          break;
        case 'X':
          if (parse_output_word_format (optarg))
            usage (argv);
          break;
        case 'F':
          if (parse_input_file_format (optarg))
            usage (argv);
          break;
        case 'O':
          if (parse_output_file_format (optarg))
            usage (argv);
          break;
        default:
          usage (argv);
        }
    }

  if (!output_file_format)
    usage (argv);

  if (output_file_format->write == NULL)
    {
      fprintf (stderr, "File format \"%s\" not supported for output\n",
               output_file_format->name);
      exit (1);
    }

  init_memory (&memory);

  while (optind < argc)
    {
      fprintf (stderr, "File: %s\n", argv[optind]);
      file = fopen (argv[optind], "rb");
      if (file == NULL)
        {
          fprintf (stderr, "Error opening input file %s\n", argv[optind]);
          exit (1);
        }
      optind++;

      if (!input_file_format)
        guess_input_file_format (file);

      input_file_format->read (file, &memory, 0);
      fclose (file);
    }

  fprintf (stderr, "Core image address range: %o - %o\n",
           memory.area[0].start, memory.area[memory.areas-1].end);
  fprintf (stderr, "Writing file format: %s\n", output_file_format->name);

  output_file_format->write (stdout, &memory);
  flush_word (stdout);

  return 0;
}
