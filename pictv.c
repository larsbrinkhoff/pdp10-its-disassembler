#include "dis.h"
#include "lodepng.h"

#define WIDTH 576
#define BIT0 0400000000000LL

static unsigned height;

static void white (unsigned char *x)
{
  *x++ = 0xFF;
  *x++ = 0xFF;
  *x++ = 0xFF;
  *x++ = 0xFF;
}

static void black (unsigned char *x)
{
  *x++ = 0x00;
  *x++ = 0x00;
  *x++ = 0x00;
  *x++ = 0xFF;
}

static void convert_word (FILE *f, unsigned char *x)
{
  word_t word = get_word (f);
  int i;

  for (i = 0; i < 32; i++)
    {
      if (word & BIT0)
        white (x);
      else
        black (x);
      x += 4;
      word <<= 1;
    }
}

static void convert_line (FILE *f, unsigned char *x)
{
  int i;
  
  for (i = 0; i < 18; i++)
    {
      convert_word (f, x);
      if (feof (f))
        break;
      x += 32 * 4;
    }
}

int main (int argc, char *argv[])
{
  unsigned error;
  unsigned char *image = NULL;
  FILE *f;

  if (argc != 3)
    {
      fprintf (stderr, "Usage: %s infile outfile\n", argv[0]);
      exit (1);
    }

  f = fopen (argv[1], "rb");
  if (f == NULL)
    {
      fprintf (stderr, "Error opening %s\n", argv[1]);
      exit (1);
    }

  height = 0;
  for (;;)
    {
      image = realloc (image, WIDTH * 4 * (height + 1));
      convert_line (f, image + WIDTH * 4 * height);
      if (feof (f))
        break;
      height++;
    }

  fprintf (stderr, "Picture is %d pixels high.\n", height);
  error = lodepng_encode32_file(argv[2], image, WIDTH, height);
  if(error)
    {
      fprintf (stderr, "error %u: %s\n", error, lodepng_error_text (error));
      exit (1);
    }

  return 0;
}
