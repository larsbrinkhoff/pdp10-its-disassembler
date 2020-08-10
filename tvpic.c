#include "dis.h"
#include "lodepng.h"

static void convert_word (unsigned char *x)
{
  word_t word = 0;
  int i;

  for (i = 0; i < 32; i++)
    {
      word <<= 1;
      if (x[0] > 10)
        word |= 1;
      x += 4;
    }

  word <<= 4;
  write_word (stdout, word);
}

static void convert_line (unsigned char *x)
{
  int i;
  
  for (i = 0; i < 18; i++)
    convert_word (x + i * 4*32);
}

int main (int argc, char *argv[])
{
  unsigned error;
  unsigned char* image = 0;
  unsigned width, height;
  int y;

  error = lodepng_decode32_file(&image, &width, &height, argv[1]);
  if(error)
    {
      fprintf (stderr, "error %u: %s\n", error, lodepng_error_text (error));
      exit (1);
    }

  if (width != 01100)
    {
      fprintf (stderr, "Picture must be 576 pixels wide.\n");
      exit (1);
    }

  fprintf (stderr, "Picture is %d pixels high.\n", height);

  for (y = 0; y < height; y++)
    {
      convert_line (image + y * 576 * 4);
    }

  return 0;
}
