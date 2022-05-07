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

#include <stdio.h>
#include <string.h>

#include "dis.h"

#define PAGE             512
#define WORDS            128
#define SECTORS           10
#define SURFACES          20
#define CYLINDERS        406
#define RP03_IMAGE_SIZE  (CYLINDERS*SURFACES*SECTORS*WORDS)

#define FDBCTL      001
#define FDBEXT      002
#define FDBADR      003
#define FDBVER      007
#define FDBBYV      011
#define FDBSIZ      012

#define FDBTMP      0400000000000LL
#define FDBPRM      0200000000000LL
#define FDBNEX      0100000000000LL
#define FDBDEL      0040000000000LL
#define FDBNXF      0020000000000LL
#define FDBLNG      0010000000000LL
#define FDBSHT      0004000000000LL

static word_t image[7][RP03_IMAGE_SIZE];
static word_t buffer[PAGE];
static word_t index_index[PAGE];
static word_t index_page[PAGE];
static int dispatch[128];
static word_t subdirectory[3][8*PAGE];
static word_t directory_indirect[PAGE];
static word_t directory_index[16][PAGE];
static word_t directory[16*PAGE];
static word_t file_indirect[PAGE];
static word_t file_index[PAGE];
static char string[5*8*PAGE];

static word_t sign_extend (word_t x)
{
  return (x ^ 0400000000000LL) - 0400000000000LL;
}

static int
get_byte (FILE *f)
{
  int c = fgetc (f);
  return c == EOF ? 0 : c;
}

static word_t
get_disk_word (FILE *f)
{
  int i, x;
  word_t w = 0;

  for (i = 0; i < 8; i++)
    {
      x = get_byte (f);
      if (x == -1)
	return -1;

      w += (word_t)x << (i * 8);
    }

  return w;
}

static word_t
ildb (word_t **w, int *position, int size)
{
  int shift = 36 - size;
  *position -= size;
  if (*position < 0)
    {
      *position = shift;
      (*w)++;
    }
  return (**w >> *position) & (0777777777777 >> shift);
}

static void
read_pack (word_t *image, const char *file)
{
  word_t data;
  FILE *f;
  int i, n;

  f = fopen (file, "rb");
  n = 0;
  for (i = 0; i < RP03_IMAGE_SIZE; i++)
    {
      data = get_disk_word (f);
      if (feof (f))
	break;
      image[i] = data;
      n++;
    }
  fclose (f);

  printf ("Pack %s: %u words (%u%%)\n",
	  file, n, 100 * n / RP03_IMAGE_SIZE);
}

static void
get_hardware (word_t linear, int *sector, int *surface,
	      int *cylinder, int *drive)
{
  *sector = linear % SECTORS;
  *surface = (linear / SECTORS) % SURFACES;
  *cylinder = (linear / SECTORS / SURFACES) % CYLINDERS;
  *drive = linear / SECTORS / SURFACES / CYLINDERS;
#if 0
  printf ("Linear %llo -> %u, %u, %u, %u\n",
	  linear, *sector, *surface, *cylinder, *drive);
#endif
}

static word_t *
get_sector (int sector, int surface, int cylinder, int drive)
{
  surface += cylinder * SURFACES;
  sector += surface * SECTORS;
  return image[drive] + sector * WORDS;
}

static void
get_page (int linear, word_t *data)
{
  int sector, surface, cylinder, drive;
  int size = WORDS * sizeof (word_t);
  int i;

  linear &= 07777777;
  for (i = 0; i < PAGE/WORDS; i++)
    {
      get_hardware (linear++, &sector, &surface, &cylinder, &drive);
      memcpy (data, get_sector (sector, surface, cylinder, drive), size);
      data += WORDS;
    }
}

static void
print_page (word_t *data)
{
  int i;
  for (i = 0; i < PAGE; )
    {
      printf ("%03o/ ", i);
      printf ("%012llo ", data[i++]);
      printf ("%012llo ", data[i++]);
      printf ("%012llo ", data[i++]);
      printf ("%012llo\n", data[i++]);
    }
}

static int
maybe_index_index (word_t *page)
{
  int i;
  for (i = 0; i < PAGE; i++)
    {
      switch (i)
	{
	case 0:
	case 010: case 011: case 012: case 013:
	case 014: case 015: case 016: case 017:
	case 020: case 021: case 022: case 023:
	case 024: case 025: case 026: case 027:
	  if (page[i] == 0)
	    return 0;
	  break;
	default:
	  if (page[i] != 0)
	    return 0;
	  break;
	}
    }

  return 1;
}

static int
maybe_index (word_t *page)
{
  word_t *data;
  int i, p, x;

  data = &page[032];
  p = 044;
  for (i = 0; i < 128; i++)
    {
      x = ildb (&data, &p, 7);
      if (x < 1 || x > 10)
	return 0;
    }

  return 1;
}

static int
find_index (void)
{
  int index_address = -1;
  int i, j;

  for (i = 0; i < 7 * RP03_IMAGE_SIZE / WORDS; i++)
    {
      get_page (i, buffer);
      for (j = 0; j < PAGE; j++)
	{
	  if ((buffer[j] & 0777777) == 0451520)
	    printf ("Pointer to index index?  %o/%o\n", i, j);
	}
      if (maybe_index_index (buffer))
	{
	  get_page (buffer[0], index_page);
	  if (maybe_index (index_page))
	    {
	      if (index_address == -1)
		index_address = i;
	      else
		index_address = -2;
	    }
	}
    }
  if (index_address < 0)
    printf ("Could not find unique INDEX index.\n");
  return index_address;
}

static void
subindex_directory_dispatch_table (word_t *page)
{
  word_t *data;
  int i, p;

  data = &page[032];
  p = 044;
  for (i = 0; i < 128; i++)
    dispatch[i] = ildb (&data, &p, 7);
}

static char *
get_string (word_t *data)
{
  word_t *start = data;
  char *ptr = string;
  int length;
  int p, c;

  p = 044;
  length = *data++ & 0777777;
  while (data - start < length)
    {
      c = ildb (&data, &p, 7);
      if (c == 0)
	break;
      *ptr++ = c;
    }
  *ptr = 0;

  return string;
}

static void
print_string (word_t *data)
{
  fputs (get_string (data), stdout);
}

static void
print_fdb (word_t *data)
{
  printf ("FDB @ %lo\n", data - directory);
  printf ("Control bits:");
  if (data[FDBCTL] & FDBTMP)
    printf (" temporary");
  if (data[FDBCTL] & FDBPRM)
    printf (" permanent");
  if (data[FDBCTL] & FDBNEX)
    printf (" no-extension");
  if (data[FDBCTL] & FDBDEL)
    printf (" deleted");
  if (data[FDBCTL] & FDBNXF)
    printf (" non-existing");
  if (data[FDBCTL] & FDBLNG)
    printf (" long");
  if (data[FDBCTL] & FDBSHT)
    printf (" compressed");
  printf ("\n");
  printf ("Name: %s.", get_string (&directory[data[FDBCTL] & 0777777]));
  printf ("%s;", get_string (&directory[data[FDBEXT] >> 18]));
  printf ("%lld\n", data[FDBVER] >> 18);
  printf ("Byte size: %lld\n", (data[FDBBYV] >> 24) & 077);
  printf ("Pages: %lld\n", data[FDBBYV] & 0777777);
  printf ("Bytes: %lld\n", data[FDBSIZ]);

  get_page (data[FDBADR], buffer);
  printf("Index block at %012llo:\n", data[FDBADR]);
  print_page (buffer);

/*
 Word 0: 400100000025  header
 Word 1: 000000000333  control,,name
 Word 2: 000335000000  extension,,other extensions
 Word 3: 007671550210  file address
 Word 4: 500000777752  protection
 Word 5: 164473234231  creation timestamp
 Word 6: 000001000000  last writer directory,,use count
 Word 7: 000001000000  version,,other versions
 Word10: 500000655704  account
 Word11: 044400000040  versions to keep/byte size,,pages
 Word12: 000000041000  length in bytes
 Word13: 164473234231  creation timestamp this version
 Word14: 164473234231  last write timestamp
 Word15: 254000000074  last access timestamp
 Word16: 000001000002  writes,,refereneces
*/
}

static void
print_indirect (word_t *data)
{
  printf ("Directory item @ %lo: %012llo\n", data - directory, *data);
}

static void
print_backup (word_t *data)
{
  printf ("Directory item @ %lo: %012llo\n", data - directory, *data);
}

static void
print_account (word_t *data)
{
  printf ("Directory item @ %lo: %012llo\n", data - directory, *data);
}

static void
print_protection (word_t *data)
{
  printf ("Directory item @ %lo: %012llo\n", data - directory, *data);
}

static void
print_directory_item (int offset)
{
  word_t *data = &directory[offset];
  switch (directory[offset] >> 18)
    {
    case 0400000:
    case 0400001:
    case 0777777: print_string (data); break;
    case 0400100: print_fdb (data); break;
    case 0400101: print_indirect (data); break;
    case 0400102: print_backup (data); break;
    case 0400200: print_account (data); break;
    case 0400201: print_protection (data); break;
    default: printf ("Unknown directory item: %llo\n", directory[offset]);
    }
}

static void
print_directory_number (int number)
{
  printf ("%o\n", number);
}

static void
print_directory (void (*print) (int))
{
  int i;

  printf ("Directory number: %lld\n", sign_extend (directory[2]));
  printf ("Symbol table: %llo-%llo\n", directory[3], directory[4]);
  printf ("Free storage: %llo\n", directory[5] >> 18);
  printf ("Top of free storage: %llo\n", directory[12]);
  printf ("Default file protection: %llo\n", directory[13]);
  printf ("Directory protection: %llo\n", directory[14]);
  printf ("Default backup: %llo\n", directory[15]);
  printf ("Groups: %llo\n", directory[16]);
  printf ("Allocation: %llo\n", directory[18]);

  for (i = directory[3]; i < directory[4]; i++)
    {
      printf ("Symbol: ");
      print_directory_item (directory[i] >> 18);
      printf ("\n");
      print (directory[i] & 0777777);
    }
}

static void
get_directory_number (int number)
{
  number *= 8;
  printf ("Directory %u disk address: %llo\n",
	  number / 8, directory_index[number / PAGE][number % PAGE]);
  get_page (directory_index[number / PAGE][number % PAGE],
	    directory);
}

static int
lookup (const char *name)
{
  int i;
  for (i = directory[3]; i < directory[4]; i++)
    {
      if (strcasecmp (get_string (&directory[directory[i] >> 18]), name) == 0)
	return directory[i] & 0777777;
    }
  return -1;
}

static void
get_directory_name (const char *name)
{
  int i = name[0];
  printf ("Directory %s is in subdirectory %d\n", name, -dispatch[i]);
  memcpy (directory, subdirectory[dispatch[i]], 8*PAGE*sizeof (word_t));
  i = lookup (name);
  printf ("Directory number: %o\n", i);
  if (i > 0)
    get_directory_number (i);
}

static void
print_file (const char *dir,
	    const char *name,
	    const char *type,
	    int version)
{
  int i, j, pages, position;
  word_t *fdb;

  get_directory_name (dir);
  i = lookup (name);
  printf ("Look up %s: %o\n", name, i);
  if (i <= 0)
    return;

  fdb = &directory[i];
  print_fdb (fdb);
  {
    int sector, surface, cylinder, drive;
    get_hardware (fdb[FDBADR] & 07777777,
		  &sector, &surface, &cylinder, &drive);
    printf ("Hardware address: sector %u, surface %u, cylinder %u, drive %u\n",
	    sector, surface, cylinder, drive);
  }

  if (fdb[FDBCTL] & FDBLNG)
    {
      get_page (fdb[FDBADR], file_indirect);
      get_page (file_indirect[0], file_index);
    }
  else
    get_page (fdb[FDBADR], file_index);

  position = 0;
  pages = fdb[FDBBYV] & 0777777;
  for (i = j = 0; pages > 0; i++, position += PAGE)
    {
      if (i == PAGE)
	{
	  if ((fdb[FDBCTL] & FDBLNG) == 0)
	    break;
	  get_page (file_indirect[++j], file_index);
	  i = 0;
	}
      if (file_index[i] == 0)
	continue;
      printf ("Page %d, position %d:\n", i, position);
      get_page (file_index[i], buffer);
      print_page (buffer);
      pages--;
    }
}

int
main (int argc, char **argv)
{
  int i;

  if (argc != 8)
    {
      fprintf (stderr, "Usage: %s <file>\n", argv[0]);
      exit (1);
    }

  output_file = stdout;

  for (i = 1; i < 8; i++)
    read_pack (image[i-1], argv[i]);

  i = find_index ();
  printf ("INDEX index: %o\n", i);
  get_page (i, index_index);
  printf ("Directory number hash table: %llo %llo\n",
	  index_page[015], index_page[016]);
  printf ("Disk address of DIRECTORY indirect index: %llo\n", index_page[017]);
  printf ("Last assigned directory number: %llo\n", index_page[020]);

  subindex_directory_dispatch_table (index_page);

  for (i = 0; i < 8; i++)
    {
      get_page (index_index[010+i], &subdirectory[1][i*PAGE]);
      get_page (index_index[020+i], &subdirectory[2][i*PAGE]);
    }

  get_page (index_page[017], directory_indirect);

  for (i = 0; i < 16; i++)
    get_page (directory_indirect[i], directory_index[i]);

  get_directory_name ("SYSTEM");
  print_directory (print_directory_item);

  i = lookup ("DSKBTTBL");
  get_page (directory[i+3], file_index);
  printf ("Disk bit table:\n");
  for (i = 0; file_index[i] != 0; i++)
    {
      get_page (file_index[i], buffer);
      print_page (buffer);
    }

  print_file ("SYSTEM", "EXEC", "SAV", 1);
  print_file ("SYSTEM", "USERS", "TXT", 1);
  //print_file ("SUBSYS", "DUMPER", "SAV", 1);

  return 0;
}
