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
 * @(#)extras.c	1.45 10/17/13
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ficl.h"

#include "fth.h"
#include "utils.h"

/*
** Ficl interface to _getcwd (Win32)
** Prints the current working directory using the VM's 
** textOut method...
*/
/* ARGSUSED */
static void ficlPrimitiveGetCwd(ficlVm *vm)
{
#define h_ficlPrimitiveGetCwd "( -- )  \
Ficl interface to getcwd(3).  \
Prints the current working directory using the VM's textOut method."
  if (getcwd(vm->pad, sizeof(vm->pad)) != NULL)
    fth_printf("%s", vm->pad);
  fth_printf("\n");
}

/*
** Ficl interface to _chdir (Win32)
** Gets a newline (or NULL) delimited string from the input
** and feeds it to the Win32 chdir function...
** Example:
**    cd c:\tmp
*/
static void ficlPrimitiveChDir(ficlVm *vm)
{
#define h_ficlPrimitiveChDir "( \"dir\" -- )  \
Ficl interface to chdir(2).  \
Gets a newline (or NULL) delimited string from the input and feeds it to the chdir function.\n\
Example:\n\
   cd /tmp"
  ficlCountedString *counted = (ficlCountedString *)vm->pad;
  char *path = getenv("HOME");
  
  ficlVmGetString(vm, counted, '\n');

  if (counted->length > 1)
  {
    if (counted->text[0] == '~')
    {
      char pwd[FICL_PAD_SIZE];

      if (path != NULL)
	snprintf(pwd, sizeof(pwd), "%s/%.*s",
	  path, (int)(counted->length - 1), counted->text + 1);
      else
	snprintf(pwd, sizeof(pwd), "%.*s",
	  (int)(counted->length - 1), counted->text + 1);
      /* [ms]
       * strncat(pwd, counted->text + 1, counted->length - 1);
       */
      path = pwd;
    }
    else
      path = counted->text;
  }

  if (path != NULL && chdir(path) == -1)
    FTH_SYSTEM_ERROR_ARG_THROW(chdir, path);
}

static void ficlPrimitiveClock(ficlVm *vm)
{
  clock_t now = clock();
  ficlStackPushUnsigned(vm->dataStack, (ficlUnsigned)now);
}

/*
** Ficl interface to system (ANSI)
** Gets a newline (or NULL) delimited string from the input
** and feeds it to the ANSI system function...
** Example:
**    system del *.*
**    \ ouch!
*/
static void ficlPrimitiveSystem(ficlVm *vm)
{
#define h_ficlPrimitiveSystem "( \"cmd\" -- )  \
Ficl interface to system(3) (ANSI).  \
Gets a newline (or NULL) delimited string from the input and feeds it to the system function.\n\
Example:\n\
   system del *.*\n\
   \\ ouch!\n\
  system tcsh\n\
   \\ starts a shell"
  ficlCountedString *counted = (ficlCountedString *)vm->pad;

  ficlVmGetString(vm, counted, '\n');

  if (FICL_COUNTED_STRING_GET_LENGTH(*counted) > 0)
  {
    int returnValue = system(FICL_COUNTED_STRING_GET_POINTER(*counted));

    returnValue = fth_set_exit_status(returnValue);
    if (returnValue)
    {
      fth_warning("%s returned %d", ficl_running_word(vm), returnValue);
      ficlVmThrow(vm, FICL_VM_STATUS_QUIT);
    }
  }
  else
    fth_warning("%s: nothing happened", ficl_running_word(vm));
}

/*
** Ficl add-in to load a text file and execute it...
** Cheesy, but illustrative.
** Line oriented... filename is newline (or NULL) delimited.
** Example:
**    load test.f
*/
#define BUFFER_SIZE 256
static void ficlPrimitiveLoad(ficlVm *vm)
{
#define h_ficlPrimitiveLoad "( \"fname\" -- )  \
Ficl add-in to load a text file and execute it.  \
Cheesy, but illustrative.  \
Line oriented.  \
FNAME is newline (or NULL) delimited.\n\
Example:\n\
   load test.f"
  char    buffer[BUFFER_SIZE];
  char    filename[BUFFER_SIZE];
  ficlCountedString *counted = FICL_POINTER_TO_COUNTED_STRING(filename);
  int     line = 0;
  FILE   *f;
  int     result = 0;
  ficlCell    oldSourceId;
  ficlString s;

  ficlVmGetString(vm, counted, '\n');

  if (FICL_COUNTED_STRING_GET_LENGTH(*counted) == 0)
  {
    fth_warning("%s: nothing happened", ficl_running_word(vm));
    return;
  }

  /*
  ** get the file's size and make sure it exists 
  */

  f = fopen(FICL_COUNTED_STRING_GET_POINTER(*counted), "r");

  if (f)
  {
    oldSourceId = vm->sourceId;
    CELL_VOIDP_SET(&vm->sourceId, f);

    /* feed each line to ficlExec */
    while (fgets(buffer, BUFFER_SIZE, f))
    {
      int length = (int)(strlen(buffer) - 1);

      line++;
      if (length <= 0)
	continue;

      if (buffer[length] == '\n')
	buffer[length--] = '\0';

      FICL_STRING_SET_POINTER(s, buffer);
      FICL_STRING_SET_LENGTH(s, length + 1);
      result = ficlVmExecuteString(vm, s);
      /* handle "bye" in loaded files. --lch */
      switch (result)
      {
      case FICL_VM_STATUS_OUT_OF_TEXT:
	break;
      case FICL_VM_STATUS_SKIP_FILE:
	goto finish;
      case FICL_VM_STATUS_USER_EXIT:
	/* handle "bye" in loaded files. --lch */
	ficlVmThrow(vm, result);
	break;
      default:
	vm->sourceId = oldSourceId;
	fclose(f);
	ficlVmThrowException(vm, result, "loading file \"%s\" line %d",
			     FICL_COUNTED_STRING_GET_POINTER(*counted), line);
	break; 
      }
    }
  }
  else
  {
    FTH_SYSTEM_ERROR_ARG_THROW(fopen, FICL_COUNTED_STRING_GET_POINTER(*counted));
    return;
  }
  /*
  ** Pass an empty line with SOURCE-ID == -1 to flush
  ** any pending REFILLs (as required by FILE wordset)
  */
finish:
  CELL_INT_SET(&vm->sourceId, -1);
  FICL_STRING_SET_FROM_CSTRING(s, "");
  ficlVmExecuteString(vm, s);

  vm->sourceId = oldSourceId;
  fclose(f);
}

/*
** Dump a tab delimited file that summarizes the contents of the
** dictionary hash table by hashcode...
*/
static void ficlPrimitiveSpewHash(ficlVm *vm)
{
#define h_ficlPrimitiveSpewHash "( \"fname\" -- )  \
Dump a tab delimited file that summarizes the contents of the dictionary hash table by hashcode."
  ficlHash *hash = ficlVmGetDictionary(vm)->forthWordlist;
  ficlWord *word;
  FILE *f;
  int i;
  int hashSize = (int)hash->size;

  if (!ficlVmGetWordToPad(vm))
    ficlVmThrow(vm, FICL_VM_STATUS_OUT_OF_TEXT);

  f = fopen(vm->pad, "w");

  if (!f)
  {
    fth_print("unable to open file\n");
    return;
  }

  for (i = 0; i < hashSize; i++)
  {
    int n = 0;

    word = hash->table[i];
    while (word)
    {
      n++;
      word = word->link;
    }

    fprintf(f, "%d\t%d", i, n);

    word = hash->table[i];
    while (word)
    {
      fprintf(f, "\t%s", word->name);
      word = word->link;
    }

    fprintf(f, "\n");
  }

  fclose(f);
}

/* ARGSUSED */
static void ficlPrimitiveBreak(ficlVm *vm)
{
  /* vm->state = vm->state; */
  return;
}

void ficlSystemCompileExtras(ficlSystem *system)
{
  ficlDictionary *dict = ficlSystemGetDictionary(system);

  FICL_PRIM(dict, "break",    ficlPrimitiveBreak);
  FICL_PRIM(dict, "clock",    ficlPrimitiveClock);
  ficlDictionaryAppendConstant(dict, "clocks/sec", (ficlInteger)CLOCKS_PER_SEC);
  FICL_PRIM_DOC(dict, "load",     ficlPrimitiveLoad);
  FICL_PRIM_DOC(dict, "spewhash", ficlPrimitiveSpewHash);
  FICL_PRIM_DOC(dict, "system",   ficlPrimitiveSystem);
  FICL_PRIM_DOC(dict, "pwd",      ficlPrimitiveGetCwd);
  FICL_PRIM_DOC(dict, "cd",       ficlPrimitiveChDir);
}
