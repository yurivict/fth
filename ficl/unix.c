/*
** Copyright (c) 1997-2001 John Sadler (john_sadler@alum.mit.edu)
** All rights reserved.
**
** Get the latest Ficl release at http://ficl.sourceforge.net
**
** I am interested in hearing from anyone who uses Ficl. If you have
** a problem, a success story, a defect, an enhancement request, or
** if you would like to contribute to the Ficl release, please
** contact me by email at the address above.
**
** L I C E N S E  and  D I S C L A I M E R
** 
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
** OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
*/

/*-
 * Adapted to work with FTH
 *
 * Copyright (c) 2004-2013 Michael Scholz <mi-scholz@users.sourceforge.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @(#)unix.c	1.22 10/17/13
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
