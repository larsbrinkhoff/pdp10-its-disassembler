#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define RECORD_SIZE 5120
static uint8_t buffer[2 * RECORD_SIZE];
static uint32_t buf_size = RECORD_SIZE;
static uint8_t *ptr = &buffer[RECORD_SIZE];
static char name[65535];
static int extract = 0;

static uint8_t
read_frame (void)
{
  int c;
  c = getchar ();
  if (c == EOF)
    {
      printf ("Physical end of tape.\n");
      exit (0);
    }
  return c;
}

static uint32_t
read_size (void)
{
  uint8_t x1, x2, x3, x4;
  x1 = read_frame ();
  x2 = read_frame ();
  x3 = read_frame ();
  x4 = read_frame ();
  return x1 | (x2 << 8) | (x3 << 16) | (x4 << 24);
}

static uint32_t
read_record (uint8_t *buffer)
{
  uint32_t i, size;

  size = read_size ();
  if (size & 0x80000000)
    return size;
  else if (size > 0)
    {
      for (i = 0; i < size; i++)
        *buffer++ = read_frame ();
      if (size != read_size ())
        {
          printf ("Error in image.\n");
          exit (1);
        }
    }

  return size;
}

static uint8_t *
read_data (uint32_t size)
{
  if (ptr >= &buffer[RECORD_SIZE])
    {
      memcpy (buffer, &buffer[RECORD_SIZE], RECORD_SIZE);
      buf_size -= RECORD_SIZE;
      ptr -= RECORD_SIZE;
    }

  if (ptr + size > &buffer[buf_size])
    {
      int eof = 0;
      uint32_t n;

      for (;;)
        {
          n = read_record (&buffer[buf_size]);
          eof = (n == 0) ? eof+1 : 0;
          if (n == 0)
            {
              printf ("Tape mark.\n");
              if (eof == 2)
                printf ("Logical end of tape.\n");
            }
          else if (n & 0x80000000)
            printf ("Tape error.\n");
          else
            break;
        }
      buf_size += n;
    }

  ptr += size;
  return ptr - size;
}

static uint32_t
read_16bit (uint8_t *data)
{
  return data[0] | (data[1] << 8);
}

static uint32_t
read_32bit (uint8_t *data)
{
  return (read_16bit (data) << 16) | read_16bit (data + 2);
}

static uint32_t
read_octal (uint8_t *data, int size)
{
  char tmp[12];
  memcpy (tmp, data, size);
  tmp[size] = 0;
  return strtoul (tmp, NULL, 8);
}

static void
mkdirs (char *dir)
{
  char *p = dir;
  for (;;)
    {
      p = strchr (p, '/');
      if (p == NULL)
        return;
      *p = 0;
      mkdir (dir, 0700);
      *p++ = '/';
    }
}

static FILE *
open_file (char *name, mode_t mode)
{
  int fd;

  if (*name == '/')
    name++;

  mkdirs (name);

  if (mode & 040000)
    {
      mkdir (name, mode & 0777);
      return NULL;
    }

  fd = creat (name, mode & 0777);
  return fdopen (fd, "w");
}

static void
read_file (void)
{
  uint32_t i, mode, uid, gid, links, mtime, name_size, file_size;
  uint8_t *data;
  uint8_t adjust = 0;
  FILE *f = NULL;
  time_t t;
  char *s;

 again:
  data = read_data (100);

  if (read_16bit (data) == 070707)
    {
      mode = read_16bit (data + 6);
      uid = read_16bit (data + 8);
      gid = read_16bit (data + 10);
      links = read_16bit (data + 12);
      mtime = read_32bit (data + 16);
      name_size = read_16bit (data + 20);
      file_size = read_32bit (data + 22);
      memcpy (name, data + 26, name_size);
      ptr = data + 26 + name_size;
      adjust = 1;
    }
  else if (memcmp (data, "070707", 6) == 0)
    {
      mode = read_octal (data + 18, 6);
      uid = read_octal (data + 24, 6);
      gid = read_octal (data + 30, 6);
      links = read_octal (data + 36, 6);
      mtime = read_octal (data + 48, 11);
      name_size = read_octal (data + 59, 6);
      file_size = read_octal (data + 65, 11);
      memcpy (name, data + 76, name_size);
      ptr = data + 76 + name_size;
      adjust = 0;
    }
  else
    {
      printf ("Not a cpio record, spacing forward to next.\n");
      read_data (RECORD_SIZE - (ptr - buffer));
      goto again;
    }

  if (file_size == 0 && strcmp (name, "TRAILER!!!") == 0)
    {
      printf ("Saveset trailer.\n");
      read_data (RECORD_SIZE - (ptr - buffer));
    }
  else
    {
      t = mtime;
      s = ctime (&t);
      s[strlen (s) - 1] = 0;
      printf ("%06o %3u %5u %5u %7u %s %s\n",
              mode, links, uid, gid, file_size, s, name);

      if (extract)
        f = open_file (name, mode);

      /* The file data must start on an even boundary. */
      if ((ptr - buffer) & 1)
        read_data (adjust);
      for (i = 0; i < file_size; i++)
        {
          data = read_data (1);
          if (f)
            fputc (*data, f);
        }
      if (f)
        fclose (f);
    }

  /* The next file header must start on an even boundary. */
  if ((ptr - buffer) & 1)
    read_data (adjust);

}

int main (int argc, char **argv)
{
  if (argc == 2 && strcmp (argv[1], "-x") == 0)
    {
      extract = 1;
      umask (0);
    }

  for (;;)
    read_file ();

  return 0;
}
