/* Copyright (C) 2022 Lars Brinkhoff <lars@nocrew.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include "tape-image.h"
#include "mkdirs.h"

#define OFS_MAGIC 60011
#define NFS_MAGIC 60012

#define TS_TAPE   1
#define TS_INODE  2
#define TS_BITS   3
#define TS_ADDR   4
#define TS_END    5
#define TS_CLRI   6

typedef struct {
  uint32_t inode;
  char name[256];
} dir_info;

typedef struct {
  uint32_t inode;
  uint32_t mode;
  uint32_t uid, gid;
  uint64_t size;
  uint32_t atime, mtime;
  dir_info *start, *end;
} inode_info;

/* This ought to be enough for anyone. */
static uint8_t record[640 * 1024];
static uint8_t *rend = record;
static uint8_t *rptr = record;

static char **tape_argv;
static int tape_argc;
static uint32_t tape_volume = 1;
static int tape_changed = 0;
static FILE *tape;
static jmp_buf jb;

static int big_endian = 0;
static int long_word = 0;
static uint32_t word_mask;
static uint32_t magic = 0;

static uint32_t h_type;
static uint32_t h_date;
static uint32_t h_ddate;
static uint32_t h_volume;
static uint32_t h_tapea;
static uint32_t h_inode;
static uint32_t h_magic;
static uint32_t h_checksum;
static uint32_t h_count;
static uint8_t  h_addr[8000];
static uint16_t i_mode;
static uint16_t i_nlink;
static uint16_t i_uid;
static uint16_t i_gid;
static uint64_t i_size;
static uint64_t remaining;
static uint32_t i_atime;
static uint32_t i_mtime;
static uint32_t i_ctime;

#define MAX_FILENAMES 1000000
#define MAX_INODES 1000000

static dir_info filenames[MAX_FILENAMES];
static dir_info *fptr = filenames;
static inode_info inodes[MAX_INODES];
static inode_info *iptr = inodes;
static inode_info *root = NULL;

static FILE *debug;
static FILE *list;

static void read_header (uint8_t *data);

static uint8_t *
get_block (void)
{
  uint8_t *data;
  int eof = 0;

  while (rptr >= rend)
    {
      uint32_t n = read_record (tape, record, sizeof record);
      eof = (n == 0) ? eof+1 : 0;
      if (n == 0)
        {
          printf ("Tape mark.\n");
          if (eof == 2)
            printf ("Logical end of tape.\n");
        }
      else if ((n >> 24) == 0xFF)
        printf ("End of medium.\n");
      else if (n & 0x80000000)
        printf ("Tape error.\n");
      else
        {
          rptr = record;
          rend = record + n;

          if (tape_changed)
            {
              data = get_block ();
              tape_volume++;
              read_header (data);
              tape_changed = 0;
            }

        }
    }

  data = rptr;
  rptr += 1024;
  return data;
}

static void
detect_format (uint8_t *image)
{
  if (memcmp (image + 18, "\x6B\xEA", 2) == 0)
    {
      printf ("16-bit old dump.\n");
      big_endian = 0;
      long_word = 0;
      word_mask = 0xFFFF;
      magic = 60011;
    }
  else if (memcmp (image + 24, "\x6C\xEA\x00\x00", 4) == 0)
    {
      printf ("Little endian 32-bit new dump.\n");
      big_endian = 0;
      long_word = 1;
      word_mask = 0xFFFFFFFF;
      magic = 60012;
    }
  else if (memcmp (image + 24, "\x6B\xEA\x00\x00", 4) == 0)
    {
      printf ("Little endian 32-bit old dump.\n");
      big_endian = 0;
      long_word = 1;
      word_mask = 0xFFFFFFFF;
      magic = 60011;
    }
  else if (memcmp (image + 24, "\x00\x00\xEA\x6C", 4) == 0)
    {
      printf ("Big endian 32-bit new dump.\n");
      big_endian = 1;
      long_word = 1;
      word_mask = 0xFFFFFFFF;
      magic = 60012;
    }
  else if (memcmp (image + 24, "\x00\x00\xEA\x6B", 4) == 0)
    {
      printf ("Big endian 32-bit old dump.\n");
      big_endian = 1;
      long_word = 1;
      word_mask = 0xFFFFFFFF;
      magic = 60011;
    }
  else
    {
      printf ("Unknown format.\n");
      exit (1);
    }
}

static void
read_block (uint8_t *data, uint32_t size, uint8_t **ptr)
{
  memcpy (data, *ptr, size);
  *ptr += size;
}

static uint32_t
read_16bit (uint8_t **ptr)
{
  uint8_t *data = *ptr;
  *ptr += 2;
  if (big_endian)
    return (data[0] << 8) | data[1];
  else
    return data[0] | (data[1] << 8);
}

static uint32_t
read_32bit (uint8_t **ptr)
{
  uint32_t x = read_16bit (ptr);
  if (!long_word || big_endian)
    return (x << 16) | read_16bit (ptr);
  else
    return x | (read_16bit (ptr) << 16);
}

static uint64_t
read_64bit (uint8_t **ptr)
{
  uint64_t x = read_32bit (ptr);
  if (big_endian)
    return (x << 32) | read_32bit (ptr);
  else
    return x | ((uint64_t)read_32bit (ptr) << 32);
}

static uint32_t
read_int (uint8_t **ptr)
{
  if (long_word)
    return read_32bit (ptr);
  else
    return read_16bit (ptr);
}


static char *
timestamp (uint32_t x, int seconds)
{
  static char string[100];
  time_t t = x;
  struct tm *tm = gmtime (&t);
  strftime (string, sizeof string,
            seconds ? "%Y-%m-%d %H:%M:%S" : "%Y-%m-%d", tm);
  return string;
}

#if 0
static int
printable (uint8_t c)
{
  return c >= 32 && c <= 126;
}

static void
hexdump (uint8_t *data, uint32_t len)
{
  int i;
  uint32_t j;
      
  if (data == NULL)
    return;

  for (j = 0; j < len; )
    {
      for (i = 0; i < 16; i++)
        {
          if (i + j < len)
            printf("%02X ", data[i + j]);
          else
            printf("   ");
        }
      for (i = 0; i < 16; i++, j++)
        {
          if (j == len)
            break;
          printf("%c", printable(data[j]) ? data[j] : '.');
        }
      putchar ('\n');
    }
}
#endif

static inode_info *
find_inode (uint32_t inode)
{
  inode_info *i;
  for (i = inodes; i < iptr; i++)
    {
      if (i->inode == inode)
        return i;
    }
  return NULL;
}

static dir_info *
find_file (inode_info *dir, uint32_t inode)
{
  dir_info *i;
  for (i = dir->start; i < dir->end; i++)
    if (i->inode == inode)
      return i;
  return NULL;
}

static char *make_filename (char *string, inode_info *dir, dir_info *file);

static char *
make_dirname (char *string, inode_info *dir)
{
  const char *unknown = "<unknown>";
  inode_info *parent = find_inode (dir->start[1].inode);
  dir_info *name = NULL;
  if (parent != NULL)
    {
      name = find_file (parent, dir->inode);
      if (name != NULL)
        return make_filename (string, parent, name);
    }
  strcpy (string, unknown);
  return string + strlen (unknown);
}

static char *
make_filename (char *string, inode_info *dir, dir_info *file)
{
  if (dir != root)
    {
      string = make_dirname (string, dir);
      *string++ = '/';
    }
  strcpy (string, file->name);
  string += strlen (file->name);
  return string;
}

static void
read_ufs_file (uint8_t **data)
{
  fptr->inode = read_16bit (data);
  if (fptr->inode != 0)
    {
      strncpy (fptr->name, (char *)*data, 14);
      fptr->name[14] = 0;
      fptr++;
    }
  *data += 14;
}

static int
read_ffs_file (uint8_t **data)
{
  uint8_t *entry;
  uint32_t elen, nlen;
  entry = *data;
  fptr->inode = read_32bit (data);
  elen = read_16bit (data);
  if (fptr->inode != 0)
    {
      nlen = read_16bit (data);
      strncpy (fptr->name, (char *)*data, nlen);
      fptr->name[nlen] = 0;
      fptr++;
    }
  *data = entry + elen;
  return elen > 0;
}

static void
read_directory (uint8_t *data, uint32_t size)
{
  uint8_t *end = data + size;

  if (root == NULL)
    root = iptr;

  while (data < end)
    {
      if (h_magic == OFS_MAGIC)
        read_ufs_file (&data);
      else
        {
          if (!read_ffs_file (&data))
            return;
        }
    }
}

static void
read_file (FILE *f, uint8_t *data, uint32_t size)
{
  if (f != NULL)
    fwrite (data, size, 1, f);
}

static void
hole_file (FILE *f, uint32_t size)
{
  if (f != NULL)
    fseek (f, size, SEEK_CUR);
}

static uint32_t
checksum (uint8_t *data)
{
  uint8_t *end = data + 1024;
  uint32_t sum = 0;
  while (data < end)
    sum += read_int (&data);
  return sum & word_mask;
}

static char *
type_name (int type)
{
  switch (type)
    {
    case TS_TAPE:  return "TS_TAPE";
    case TS_INODE: return "TS_INODE";
    case TS_BITS:  return "TS_BITS";
    case TS_ADDR:  return "TS_ADDR";
    case TS_END:   return "TS_END";
    case TS_CLRI:  return "TS_CLRI";
    default:       return "Unknown";
    }
}

static void
read_header (uint8_t *data)
{
  char name[256];
  uint8_t *start = data;
  uint32_t type, mode, count;
  uint32_t i;
  int more;

  h_type = type = read_int (&data);
  h_date = read_32bit (&data);
  h_ddate = read_32bit (&data);
  h_volume = read_int (&data);
  h_tapea = read_32bit (&data);
  h_inode = read_int (&data);
  h_magic = read_int (&data);
  h_checksum = read_int (&data);

  i_mode = mode = read_16bit (&data);
  i_nlink = read_16bit (&data);
  i_uid = read_16bit (&data);
  i_gid = read_16bit (&data);
  if (h_magic == OFS_MAGIC)
    i_size = read_32bit (&data);
  else if (h_magic == NFS_MAGIC)
    i_size = read_64bit (&data);
  if (h_magic == OFS_MAGIC)
    data += 40;
  i_atime = read_32bit (&data);
  if (h_magic == NFS_MAGIC)
    data += 4;
  i_mtime = read_32bit (&data);
  if (h_magic == NFS_MAGIC)
    data += 4;
  i_ctime = read_32bit (&data);
  if (h_magic == NFS_MAGIC)
    data += 92;
  h_count = count = read_int (&data);
  if (h_magic == magic
      && checksum (start) == (84446 & word_mask)
      && (h_type == TS_ADDR || h_type == TS_INODE)
      && h_count < 1024)
    read_block (h_addr, h_count, &data);

  if (h_magic == magic)
    {
      fputc ('\n', debug);
      fprintf (debug, "Checksum is %s.\n",
              checksum (start) == (84446 & word_mask) ? "ok" : "not ok");

      fprintf (debug, "Type: %u %s\n", h_type, type_name (h_type));
      fprintf (debug, "Date: %u %s\n", h_date, timestamp (h_date, 1));
      fprintf (debug, "Ddate: %u\n", h_ddate);
      fprintf (debug, "Volume: %u\n", h_volume);
          if (tape_volume != h_volume)
            printf ("Incorrect volume; expected %d, got %d.\n", tape_volume, h_volume);
      fprintf (debug, "Tapea: %u\n", h_tapea);
      fprintf (debug, "Inode: %u\n", h_inode);
      fprintf (debug, "Magic: %u\n", h_magic);
      fprintf (debug, "Checksum: %u\n", h_checksum);

      if (h_type == TS_INODE || h_type == TS_ADDR)
        {
          fprintf (debug, "Mode: %o\n", i_mode);
          fprintf (debug, "Nlink: %u\n", i_nlink);
          fprintf (debug, "Uid: %u\n", i_uid);
          fprintf (debug, "Gid: %u\n", i_gid);
          fprintf (debug, "Size: %lu\n", i_size);
          fprintf (debug, "Atime: %u %s\n", i_atime, timestamp (i_atime, 1));
          fprintf (debug, "Mtime: %u %s\n", i_mtime, timestamp (i_mtime, 1));
          fprintf (debug, "Ctime: %u %s\n", i_ctime, timestamp (i_ctime, 1));
        }

      fprintf (debug, "Count: %u\n", h_count);
    }

  more = 0;
  if (h_magic == magic)
    {
      if (h_type == TS_ADDR || h_type == TS_INODE)
        {
          FILE *f = NULL;

          if (h_type == TS_INODE)
            {
              iptr->inode = h_inode;
              iptr->mode = i_mode;
              iptr->uid = i_uid;
              iptr->gid = i_gid;
              iptr->size = i_size;
              iptr->atime = i_atime;
              iptr->mtime = i_mtime;
              remaining = i_size;
            }

          if (i_mode & 0100000)
            {
              sprintf (name, ".inodes/%u", h_inode);
              f = fopen (name, h_type == TS_INODE ? "wb" : "ab");
            }

          if ((h_type == TS_INODE) && (i_mode & 040000) != 0)
            iptr->start = fptr;
          for (i = 0; i < count; i++)
            {
              if (h_addr[i])
                {
                  data = get_block ();
                  more++;
                  if (i_mode & 040000)
                    read_directory (data, remaining > 1024 ? 1024 : remaining);
                  else
                    read_file (f, data, remaining > 1024 ? 1024 : remaining);
                  if (remaining < 1024)
                    break;
                  remaining -= 1024;
                }
              else
                hole_file (f, 1024);
            }
          if (mode & 040000)
            iptr->end = fptr;

          if (type == TS_INODE)
            iptr++;
          if (f != NULL)
            {
              struct timeval t[2];
              fclose (f);
              t[0].tv_sec = i_atime;
              t[1].tv_sec = i_mtime;
              t[0].tv_usec = t[1].tv_usec = 0;
              utimes (name, t);
            }
        }
      else if (h_type == TS_BITS || h_type == TS_CLRI)
        {
          for (i = 0; i < count; i++)
            {
              data = get_block ();
              more++;
            }
        }
    }

  if (h_magic == magic)
    fprintf (debug, "Additional blocks: %d\n", more);
}

static FILE *
mount_next (void)
{
  printf ("Physical end of tape.\n");
  fclose (tape);

  if (tape_argc-- == 0)
    longjmp (jb, 1);

  tape = fopen (*tape_argv, "rb");
  if (tape == NULL)
    {
      fprintf (stderr, "Error opening %s\n", *tape_argv);
      exit (1);
    }

  tape_changed = 1;
  tape_argv++;
  return tape;
}

static void
process_tapes (const char *file)
{
  uint8_t *data;

  tape = fopen (file, "rb");
  if (tape == NULL)
    {
      fprintf (stderr, "Error opening %s\n", file);
      exit (1);
    }

  for (;;)
    {
      data = get_block ();
      if (magic == 0)
        detect_format (data);

      read_header (data);
      if (h_magic != magic)
        fprintf (debug, "Skipping a block.\n");
    }
}

static void
print_perm (FILE *f, uint32_t perm)
{
  fputc (perm & 4 ? 'r' : '-', f);
  fputc (perm & 2 ? 'w' : '-', f);
  fputc (perm & 1 ? 'x' : '-', f);
}

static void
print_mode (FILE *f, uint32_t mode)
{
  switch (mode & 0140000)
    {
    case 0000000: fputc ('?', f); break;
    case 0040000: fputc ('d', f); break;
    case 0100000: fputc ('-', f); break;
    case 0140000: fputc ('?', f); break;
    }
  print_perm (f, mode >> 6);
  print_perm (f, mode >> 3);
  print_perm (f, mode);
}

static void
write_file (char *name, uint32_t inode)
{
  char old[256];
  sprintf (old, ".inodes/%u", inode);
  if (*name == '/')
    name++;
  mkdirs (name);
  link (old, name);
}

static void
list_inode (uint32_t n, inode_info *inode)
{
  fprintf (list, "%6u ", n);
  if (inode == NULL)
    {
      fputs ("~~~~~~~~~~ ~~~~ ~~~~ ~~~~~~~~ ~~~~~~~~~~ ", list);
    }
  else
    {
      print_mode (list, inode->mode);
      fprintf (list, " %4u %4u ", inode->uid, inode->gid);
      fprintf (list, "%8lu %s ", inode->size, timestamp (inode->mtime, 0));
    }
}

static void
process_files (inode_info *dir)
{
  char name[1024];
  dir_info *i;

  for (i = dir->start; i < dir->end; i++)
    {
      inode_info *inode = find_inode (i->inode);

      if (strcmp (i->name, ".") == 0 || strcmp (i->name, "..") == 0)
        continue;

      list_inode (i->inode, inode);
      if (inode != NULL)
        inode->mode = 0;
      make_filename (name, dir, i);
      fprintf (list, "%s\n", name);
      write_file (name, i->inode);
    }
}

static void
process_lost_file (inode_info *inode)
{
  list_inode (inode->inode, inode);
  fputc ('\n', list);
}

static void
process_directories (void)
{
  inode_info *i;
  for (i = inodes; i < iptr; i++)
    {
      if (i->mode & 040000)
        process_files (i);
    }
}

static void
process_lost_inodes (void)
{
  inode_info *i;
  for (i = inodes; i < iptr; i++)
    {
      if (i->mode & 0100000)
        process_lost_file (i);
    }
}

int
main (int argc, char **argv)
{
  debug = stdout;
  list = stdout;

  end_of_tape (mount_next);

  mkdir (".inodes", 0700);

  tape_argv = argv + 2;
  tape_argc = argc - 2;

  if (setjmp (jb) == 0)
    process_tapes (argv[1]);

  if (h_type == TS_END)
    printf ("\nEnd of dump.\n");
  else
    printf ("\nNot last tape; maybe incomplete dump.\n");

  process_directories ();
  process_lost_inodes ();

  printf ("\nTotal number of inodes: %ld\n", iptr - inodes);
  printf ("Total number of file names: %ld\n", fptr - filenames);
}
