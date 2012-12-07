/*
 * Adapted to work with FTH:
 *
 * Copyright (c) 2004-2012 Michael Scholz <mi-scholz@users.sourceforge.net>
 *
 * This file is part of FTH.
 * 
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <sys/stat.h>
#include <sys/types.h>
#include "ficl.h"

#include "fth.h"
#include "utils.h"

int
ficlFileTruncate(ficlFile *ff, ficlUnsigned size)
{  
#if defined(HAVE_FTRUNCATE)
  return ftruncate(fileno(ff->f), (off_t)size);
#else
  return 0;
#endif
}

/* ARGSUSED */
char *
ficlCallbackDefaultTextIn(ficlCallback *callback)
{
  static char buf[BUFSIZ];
  
  return fgets(buf, BUFSIZ, stdin);
}

/* ARGSUSED */
void
ficlCallbackDefaultTextOut(ficlCallback *callback, char *message)
{
  if (message != NULL)
    fputs(message, stdout);
  fflush(stdout);
}

/* ARGSUSED */
void
ficlCallbackDefaultErrorOut(ficlCallback *callback, char *message)
{
  if (message != NULL)
    fputs(message, stderr);
  fflush(stderr);
}

int
ficlFileStatus(char *filename, int *status)
{
  struct stat statbuf;

  
  if (filename != NULL && stat(filename, &statbuf) == 0)
  {
    *status = (int)statbuf.st_mode;
    return 0;
  }
  *status = ENOENT;
  return -1;
}

ficl2Integer
ficlFileSize(ficlFile *ff)
{
  struct stat statbuf;

  if (ff == NULL)
    return -1;
  
  statbuf.st_size = -1;
  if (fstat(fileno(ff->f), &statbuf) != 0)
    return -1;
  
  return (ficl2Integer)statbuf.st_size;
}

/*
**
** Platform-specific functions
**
*/
void
ficlSystemCompilePlatform(ficlSystem *system)
{
  ficlSystemCompileExtras(system);
}
