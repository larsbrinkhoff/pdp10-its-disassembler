#include <string.h>
#include "dis.h"

/* Convert a DEC timestamp to year, month, day. */
static void
dec_date (word_t timestamp, int *year, int *month, int *day)
{
  *day = timestamp % 31;
  *month = (timestamp / 31) % 12;
  *year = (timestamp / 31 / 12) + 1964;
}

/* Print DEC timestamp. */
void
print_dec_timestamp (FILE *f, word_t timestamp)
{
  int year, month, day;
  dec_date (timestamp, &year, &month, &day);
  fprintf (f, "%4d-%02d-%02d", year, month + 1, day + 1);
}

/* Convert a DEC timesamp date to a struct tm. */
void
timestamp_from_dec (struct tm *tm, word_t timestamp)
{
  memset (tm, 0, sizeof (struct tm));
  dec_date (timestamp, &tm->tm_year, &tm->tm_mon, &tm->tm_mday);
  tm->tm_year -= 1900;
}
