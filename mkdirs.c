#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "mkdirs.h"

void
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
