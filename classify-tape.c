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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include "tape-image.h"

typedef unsigned long long word_t;

static uint8_t image[1024 * 1024];
static uint8_t *ptr = image;

static int eot_flag = 0;
static int unknown_flag = 0;
static int tape_format_flag = 0;

static int big_endian = 0;
static int track_7 = 0;
static int track_9 = 0;
static int simh = 0;
static int e11 = 0;
static int ansi = 0;

static uint32_t
read_16bits_l (uint8_t *start)
{
  return start[0] | (start[1] << 8);
}

static uint32_t
read_32bits_l (uint8_t *start)
{
  return read_16bits_l (start) | (read_16bits_l (start + 2) << 16);
}

static word_t
read_36bits (uint8_t *x)
{
  if (track_7)
    {
      return (((word_t)x[0] & 077) << 30) |
             (((word_t)x[1] & 077) << 24) |
             (((word_t)x[2] & 077) << 18) |
             (((word_t)x[3] & 077) << 12) |
             (((word_t)x[4] & 077) <<  6) |
             (((word_t)x[5] & 077) <<  0);
    }
  else if (track_9)
    {
      return (((word_t)x[0] & 0377) << 28) |
             (((word_t)x[1] & 0377) << 20) |
             (((word_t)x[2] & 0377) << 12) |
             (((word_t)x[3] & 0377) <<  4) |
             (((word_t)x[4] & 0017) <<  0);
    }
  else
    {
      printf ("Uknown track number\n");
      exit (1);
    }
}

static void
check_eot (void)
{
  int eof = 0;
  uint32_t len;

  for (;;)
    {
      len = read_record (stdin, image, sizeof image);
      if (len == 0 && eof)
        {
          int n = 0;
          while (getchar () != EOF)
            n++;
          if (n > 4)
            printf ("Data after logical end of tape: %d octets.\n", n);
          exit (0);
        }
      eof = (len == 0);
    }
}

static int
vms_backup (uint8_t *data, uint32_t len)
{
  uint32_t x;
  if (memcmp (data + 0, "\x00\x01", 2) != 0)
    return 0;
  x = data[40] | (data[41] << 8) | (data[42] << 16) | (data[43] << 24);
  if (x != 0 && x != len)
    return 0;
  return 1;
}

static int
asciz_text (uint8_t *data, uint32_t len)
{
  uint8_t *p = data;
  uint8_t *end = data + len;
  
  while (p < end)
    {
      if (*p == 0)
        break;
      if (*p != 9 && *p != 10 && *p != 13)
        {
          if (*p < 32 || *p == 127)
            return 0;
        }
      p++;
    }

  while (p < end)
    {
      if (*p != 0)
        return 0;
      p++;
    }
  
  return 1;
}

static int
octal_number (uint8_t *data, uint32_t len)
{
  uint8_t *p = data;
  uint8_t *end = data + len;
  
  while (p < end)
    {
      if (*p == 0)
        break;
      if (!(*p == 32 || (*p >= '0' && *p <= '7')))
        return 0;
      p++;
    }

  while (p < end)
    {
      if (*p != 0)
        return 0;
      p++;
    }

  return 1;
}

static int
unix_16bit_tar (uint8_t *data, uint32_t len)
{
  if (len < 140)
    return 0;
  if (!asciz_text (data, 100))
    return 0;
  if (!octal_number (data + 100, 7))
    return 0;
  if (!octal_number (data + 108, 7))
    return 0;
  if (!octal_number (data + 116, 7))
    return 0;
  return 1;
}

static int
unix_cpio (uint8_t *data)
{
  if (memcmp (data, "070707", 6) == 0)
    return 1;
  if (memcmp (data, "\xC7\x71", 2) == 0)
    return 1;
  return 0;
}

static int
digit (uint8_t x)
{
  return strchr ("0123456789", x) != NULL;
}

static int
ansi_label (uint8_t *data, uint32_t len)
{
  if (len != 80)
    return 0;
  if (memcmp (data, "VOL", 3) == 0 && digit(data[3]))
    return 1;
  if (memcmp (data, "HDR", 3) == 0 && digit(data[3]))
    return 1;
  if (memcmp (data, "UHL", 3) == 0 && digit(data[3]))
    return 1;
  return 0;
}

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

static int
its_dump_header (uint8_t *data)
{
  word_t x;
  x = read_36bits (data) >> 18;
  if (x >= 0777772 && x <= 0777776)
    return 1;
  x = read_36bits (data + 1) >> 18;
  if (x >= 0777772 && x <= 0777776)
    return 1;
  return 0;
}

static int
its_dump (uint8_t *data, uint32_t len)
{
  if (!track_7)
    {
      track_9 = 1;
      if (its_dump_header (data))
        return 1;
      track_9 = 0;
    }

  track_7 = 1;
  if (its_dump_header (data))
    return 1;
  if (len == 6145 && its_dump_header (data + 1))
    return 1;
  if (len == 6146 && its_dump_header (data + 2))
    return 1;
  if (len == 6147 && its_dump_header (data + 3))
    return 1;
  track_7 = 0;

  return 0;
}

static int
its_dump_label (uint8_t *data, uint32_t len)
{
  if (len != 60)
    return 0;

  track_7 = 1;
 again:
  if (read_36bits (data + 1 * 6) != 0)
    return 0;
  if (read_36bits (data + 3 * 6) != 0)
    return 0;
  if (read_36bits (data + 5 * 6) != 0)
    return 0;
  if (read_36bits (data + 7 * 6) != 0)
    return 0;
  if (read_36bits (data + 9 * 6) != 0)
    return 0;

  len = read_record (stdin, ptr, sizeof image - (ptr - image));
  if (len < 0x80000000)
    ptr += len;
  while (len == 0 || (len & 0x80000000) != 0)
    {
      len = read_record (stdin, ptr, sizeof image - (ptr - image));
      if (len < 0x80000000)
        ptr += len;
    }
  if (len == 60)
    goto again;

  return its_dump (data, len);
}

static int
tops20_install (uint8_t *data, uint32_t len)
{
  word_t x;

  if ((len % 5) != 0)
    return 0;

  track_9 = 1;
  x = read_36bits (data);
  if ((x >> 18) == 01776)
    return 1;

  return 0;
}

static int
tops10_failsafe (uint8_t *data, uint32_t len)
{
  if (len < 25)
    return 0;

  track_9 = 1;
  if (read_36bits (data + 5) != 0124641515463LL)
    return 0;
  if ((read_36bits (data + 10) >> 18) != 0414645LL)
    return 0;

  return 1;
}


static int
dos_fat (uint8_t *data)
{
  if (read_16bits_l (data + 11) != 512)
    return 0;
  switch (data[13])
    {
    case 1: case 2: case 4: case 8: case 16: case 32: case 64:
      break;
    default:
      return 0;
    }
  if (data[16] < 1 || data[16] > 3)
    return 0;
  return 1;
}

int main (int argc, char **argv)
{
  uint32_t len;
  uint8_t *data;
  int opt;

  while ((opt = getopt (argc, argv, "etu")) != -1)
    switch (opt)
      {
      case 'e':
        eot_flag = 1;
        break;
      case 'u':
        unknown_flag = 1;
        break;
      case 't':
        tape_format_flag = 1;
        break;
      default:
        fprintf (stderr, "Usage: %s [-eut]\n", argv[0]);
        exit (1);
      }
  
  for (;;)
    {
      data = ptr;
      len = read_record (stdin, ptr, sizeof image - (ptr - image));
      if ((len == 0) || (len & 0x80000000) != 0)
        continue;
      ptr += len;
      if (ansi_label (data, len))
        {
          ansi = 1;
          continue;
        }
      if (len < 4)
        continue;
      break;
    }

  if ((len % (518 * 5)) == 0)
    {
      printf ("TOPS-20 DUMPER");
    }
#if 0
  else if (len == 5120)
    {
      printf ("ITS DUMP");
    }
  else if (len == 6144)
    {
      printf ("ITS DUMP");
    }
#endif
  else if (its_dump (data, len))
    {
      printf ("ITS DUMP");
    }
  else if (its_dump_label (data, len))
    {
      printf ("ITS DUMP (with label)");
    }
  else if (strstr ((const char *)data, "TAPE-SYSTEM-VERSION") != NULL)
    {
      printf ("Symbolics LMFS dump");
    }
  else if (strstr ((const char *)data, "LMFL(") != NULL)
    {
      printf ("MIT/LMI dump");
    }
  else if (vms_backup (data, len))
    {
      printf ("VMS BACKUP");
    }
  else if (memcmp (data + 257, "ustar", 5) == 0)
    {
      printf ("Unix ustar");
    }
  else if (unix_16bit_tar (data, len))
    {
      printf ("Unix 16-bit tar");
    }
  else if (memcmp (data + 24, "\x6C\xEA\x00\x00", 4) == 0)
    {
      printf ("Unix little endian 32-bit dump");
    }
  else if (memcmp (data + 24, "\x00\x00\xEA\x6C", 4) == 0)
    {
      printf ("Unix big endian 32-bit dump");
    }
  else if (memcmp (data + 24, "\x6B\xEA\x00\x00", 4) == 0)
    {
      printf ("Unix little endian 32-bit old dump");
    }
  else if (memcmp (data + 24, "\x00\x00\xEA\x6B", 4) == 0)
    {
      printf ("Unix big endian 32-bit old dump");
    }
  else if (memcmp (data + 18, "\x6B\xEA", 2) == 0)
    {
      printf ("Unix 16-bit dump");
    }
  else if (tops20_install (data, len))
    {
      printf ("TOPS-20 install");
    }
  else if (len == 544 * 5)
    {
      printf ("TOPS-10 BACKUP");
    }
  else if (tops10_failsafe (data, len))
    {
      printf ("TOPS-10 FAILSAFE");
    }
  else if (dos_fat (data))
    {
      printf ("FAT file system");
    }
  else if (unix_cpio (data))
    {
      printf ("Unix cpio");
    }
#if 0
  else if (asciz_text (data, len))
    {
      printf ("Raw text");
    }
#endif
  else
    {
      printf ("Unknown");

      if (unknown_flag)
        {
          printf ("\nRecord size: %u %x\n", len, len);
          hexdump (data, len);
          len = read_record (stdin, image, sizeof image);
          printf ("Second size: %u %x\n", len, len);
          if ((len & 0x80000000) == 0)
            hexdump (data, len);
          len = read_record (stdin, image, sizeof image);
          printf ("Third size: %u %x\n", len, len);
          if ((len & 0x80000000) == 0)
            hexdump (data, len);
        }
    }

  if (ansi)
    printf (" (ANSI label)");

  if (tape_format_flag)
    {
      printf ("  [");
      if (track_7)
        printf ("7-track, ");
      if (track_9)
        printf ("9-track, ");
      printf ("%s-endian",
              big_endian ? "big" : "little");
      if (e11)
        printf (", E11");
      if (simh)
        printf (", SIMH");
      printf ("]");
    }

  if (eot_flag)
    check_eot ();

  putchar ('\n');

  return 0;
}
