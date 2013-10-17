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
 * @(#)fileaccess.c	1.37 10/17/13
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "ficl.h"

#include "fth.h"
#include "utils.h"

/*
**
** fileaccess.c
**
** Implements all of the File Access word set that can be implemented in
** portable C.
**
*/

#define pushIor(Vm, Success)					\
	ficlStackPushInteger((Vm)->dataStack, (Success) ? 0L : 1L)

/* ( c-addr u fam -- fileid ior ) */
static void ficlFileOpen(ficlVm *vm, char *writeMode)
{
  ficlInteger fam;
  size_t length;
  void *address;
  char mode[4];
  char filename[FICL_MAXPATHLEN];
  FILE *f;
  
  FICL_STACK_CHECK(vm->dataStack, 3, 2);
  fam     = ficlStackPopInteger(vm->dataStack);
  length  = (size_t)ficlStackPopUnsigned(vm->dataStack);
  address = (void *)ficlStackPopPointer(vm->dataStack);

  if (FICL_MAXPATHLEN <= length)
  {
    ficlStackPushPointer(vm->dataStack, NULL);
    ficlStackPushInteger(vm->dataStack, (ficlInteger)EINVAL);
    return;
  }
  
  memcpy(filename, address, length);
  filename[length] = '\0';
  *mode = '\0';
  
  switch (FICL_FAM_OPEN_MODE(fam))
  {
  case 0:
    ficlStackPushPointer(vm->dataStack, NULL);
    ficlStackPushInteger(vm->dataStack, (ficlInteger)EINVAL);
    return;
  case FICL_FAM_READ:
    fth_strcat(mode, sizeof(mode), "r");
    break;
  case FICL_FAM_WRITE:
    fth_strcat(mode, sizeof(mode), writeMode);
    break;
  case FICL_FAM_READ | FICL_FAM_WRITE:
    fth_strcat(mode, sizeof(mode), writeMode);
    fth_strcat(mode, sizeof(mode), "+");
    break;
  }

  fth_strcat(mode, sizeof(mode), (fam & FICL_FAM_BINARY) ? "b" : "t");
  f = fopen(filename, mode);

  if (f == NULL)
    ficlStackPushPointer(vm->dataStack, NULL);
  else
  {
    ficlFile *ff = (ficlFile *)FTH_MALLOC(sizeof(ficlFile));

    ff->filename[0] = '\0';
    fth_strcat(ff->filename, sizeof(ff->filename), filename);
    ff->f = f;
    ficlStackPushPointer(vm->dataStack, ff);
    fseek(f, 0L, SEEK_SET);
  }
  pushIor(vm, f != NULL);
}

/* ( c-addr u fam -- fileid ior ) */
static void ficlPrimitiveOpenFile(ficlVm *vm)
{
#define h_ficlPrimitiveOpenFile "( c-addr u fam -- fileid ior )"
  ficlFileOpen(vm, "a");
}

/* ( c-addr u fam -- fileid ior ) */
static void ficlPrimitiveCreateFile(ficlVm *vm)
{
#define h_ficlPrimitiveCreateFile "( c-addr u fam -- fileid ior )"
  ficlFileOpen(vm, "w");
}

static int ficlFileClose(ficlFile *ff) /* ( fileid -- ior ) */
{
  if (ff)
  {
    FILE *f = ff->f;
    
    FTH_FREE(ff);
    if (f) return !fclose(f);
  }
  return 0;
}

/* ( fileid -- ior ) */
static void ficlPrimitiveCloseFile(ficlVm *vm)
{
#define h_ficlPrimitiveCloseFile "( fileid -- ior )"
  ficlFile *ff;

  FICL_STACK_CHECK(vm->dataStack, 1, 1);
  ff = (ficlFile *)ficlStackPopPointer(vm->dataStack);
  pushIor(vm, ficlFileClose(ff));
}

/* ( c-addr u -- ior ) */
static void ficlPrimitiveDeleteFile(ficlVm *vm)
{
#define h_ficlPrimitiveDeleteFile "( c-addr u -- ior )"
  ficlUnsigned length;
  char *address;

  FICL_STACK_CHECK(vm->dataStack, 2, 1);
  length = ficlStackPopUnsigned(vm->dataStack);
  address = (char *)ficlStackPopPointer(vm->dataStack);
  snprintf(vm->pad, sizeof(vm->pad), "%.*s", (int)length, address);
  pushIor(vm, unlink(vm->pad) == 0);
}

/* ( c-addr1 u1 c-addr2 u2 -- ior ) */
static void ficlPrimitiveRenameFile(ficlVm *vm)
{
#define h_ficlPrimitiveRenameFile "( c-addr1 u1 c-addr2 u2 -- ior )"
  ficlUnsigned length;
  char *address;
  char from[FICL_PAD_SIZE];
  char to[FICL_PAD_SIZE];

  FICL_STACK_CHECK(vm->dataStack, 4, 1);
  length = ficlStackPopInteger(vm->dataStack);
  address = (char *)ficlStackPopPointer(vm->dataStack);
  snprintf(to, sizeof(to), "%.*s", (int)length, address);
  length = ficlStackPopUnsigned(vm->dataStack);
  address = (char *)ficlStackPopPointer(vm->dataStack);
  snprintf(from, sizeof(from), "%.*s", (int)length, address);
  pushIor(vm, rename(from, to) == 0);
}

/* ( c-addr u -- x ior ) */
static void ficlPrimitiveFileStatus(ficlVm *vm)
{
#define h_ficlPrimitiveFileStatus "( c-addr u -- x ior )"
  int status = 0;
  int ior;
  ficlUnsigned length;
  char *address;

  FICL_STACK_CHECK(vm->dataStack, 2, 2);
  length = ficlStackPopUnsigned(vm->dataStack);
  address = (char *)ficlStackPopPointer(vm->dataStack);
  snprintf(vm->pad, sizeof(vm->pad), "%.*s", (int)length, address);
  ior = ficlFileStatus(vm->pad, &status);
  ficlStackPushInteger(vm->dataStack, (ficlInteger)status);
  ficlStackPushInteger(vm->dataStack, (ficlInteger)ior);
}

/* ( fileid -- ud ior ) */
static void ficlPrimitiveFilePosition(ficlVm *vm)
{
#define h_ficlPrimitiveFilePosition "( fileid -- ud ior )"
  ficlFile *ff;
  long ud;
  
  FICL_STACK_CHECK(vm->dataStack, 1, 2);
  ff = (ficlFile *)ficlStackPopPointer(vm->dataStack);
  ud = ftell(ff->f);
  ficlStackPushInteger(vm->dataStack, ud);
  pushIor(vm, ud != -1);
}

/* ( fileid -- ud ior ) */
static void ficlPrimitiveFileSize(ficlVm *vm)
{
#define h_ficlPrimitiveFileSize "( fileid -- ud ior )"
  ficlFile *ff;
  ficl2Integer ud;

  FICL_STACK_CHECK(vm->dataStack, 1, 2);
  ff = (ficlFile *)ficlStackPopPointer(vm->dataStack);
  ud = ficlFileSize(ff);
  ficlStackPush2Integer(vm->dataStack, ud);
  pushIor(vm, ud != -1);
}

#define nLINEBUF 1024
/* ( i*x fileid -- j*x ) */
static void ficlPrimitiveIncludeFile(ficlVm *vm)
{
#define h_ficlPrimitiveIncludeFile "( i*x fileid -- j*x )"
  ficlFile *ff;
  ficlCell id = vm->sourceId;
  int except = FICL_VM_STATUS_OUT_OF_TEXT;
  ficl2Integer currentPosition, totalSize;
  size_t size;
  ficlString s;

  FICL_STACK_CHECK(vm->dataStack, 1, 0);
  ff = (ficlFile *)ficlStackPopPointer(vm->dataStack);
  CELL_VOIDP_SET(&vm->sourceId, ff);
  currentPosition = ftell(ff->f);
  totalSize = ficlFileSize(ff);
  size = (size_t)(totalSize - currentPosition);

  if ((totalSize != -1) && (currentPosition != -1) && (size > 0))
  {
    char buffer[nLINEBUF + 1];

    if (size > nLINEBUF) size = nLINEBUF;

    if ((fread(buffer, 1L, size, ff->f)) == size)
    {
      FICL_STRING_SET_POINTER(s, buffer);
      FICL_STRING_SET_LENGTH(s, size);
      except = ficlVmExecuteString(vm, s);
    }
  }

  if ((except < 0) && (except != FICL_VM_STATUS_OUT_OF_TEXT))
    ficlVmThrow(vm, except);
	
  /*
  ** Pass an empty line with SOURCE-ID == -1 to flush
  ** any pending REFILLs (as required by FILE wordset)
  */
  CELL_INT_SET(&vm->sourceId, -1);
  FICL_STRING_SET_FROM_CSTRING(s, "");
  ficlVmExecuteString(vm, s);

  vm->sourceId = id;
  ficlFileClose(ff);
}

/* ( c-addr u1 fileid -- u2 ior ) */
static void ficlPrimitiveReadFile(ficlVm *vm)
{
#define h_ficlPrimitiveReadFile "( c-addr u1 fileid -- u2 ior )"
  ficlFile *ff;
  size_t length, result, count;
  void *address;

  FICL_STACK_CHECK(vm->dataStack, 3, 2);
  ff = (ficlFile *)ficlStackPopPointer(vm->dataStack);
  length = (size_t)ficlStackPopUnsigned(vm->dataStack);
  address = (void *)ficlStackPopPointer(vm->dataStack);
  clearerr(ff->f);
  count = 1;
  result = fread(address, count, length, ff->f);
  ficlStackPushInteger(vm->dataStack, (ficlInteger)result);
  pushIor(vm, ferror(ff->f) == 0);
}

/* ( c-addr u1 fileid -- u2 flag ior ) */
static void ficlPrimitiveReadLine(ficlVm *vm)
{
#define h_ficlPrimitiveReadLine "( c-addr u1 fileid -- u2 flag ior )"
  ficlFile *ff;
  int length;
  char *address;
  int error;
  int flag;

  FICL_STACK_CHECK(vm->dataStack, 3, 3);
  ff = (ficlFile *)ficlStackPopPointer(vm->dataStack);
  length = (int)ficlStackPopUnsigned(vm->dataStack);
  address = (char *)ficlStackPopPointer(vm->dataStack);

  if (feof(ff->f))
  {
    ficlStackPushInteger(vm->dataStack, -1L);
    ficlStackPushInteger(vm->dataStack, 0L);
    ficlStackPushInteger(vm->dataStack, 0L);
    return;
  }

  clearerr(ff->f);
  *address = '\0';
  if (fgets(address, length, ff->f) == NULL)
    perror("fgets");

  error = ferror(ff->f);
  if (error != 0)
  {
    ficlStackPushInteger(vm->dataStack, -1L);
    ficlStackPushInteger(vm->dataStack, 0L);
    ficlStackPushInteger(vm->dataStack, (ficlInteger)error);
    return;
  }

  length = (int)strlen(address);
  flag = (length > 0);
  if (length && ((address[length - 1] == '\r') ||
      (address[length - 1] == '\n')))
    length--;
    
  ficlStackPushInteger(vm->dataStack, (ficlInteger)length);
  ficlStackPushInteger(vm->dataStack, (ficlInteger)flag);
  ficlStackPushInteger(vm->dataStack, 0L); /* ior */
}

/* ( c-addr u1 fileid -- ior ) */
static void ficlPrimitiveWriteFile(ficlVm *vm)
{
#define h_ficlPrimitiveWriteFile "( c-addr u fileid -- ior )"
  ficlFile *ff;
  size_t length, count;
  void *address;

  FICL_STACK_CHECK(vm->dataStack, 3, 1);
  ff = (ficlFile *)ficlStackPopPointer(vm->dataStack);
  length = (size_t)ficlStackPopUnsigned(vm->dataStack);
  address = (void *)ficlStackPopPointer(vm->dataStack);
  clearerr(ff->f);
  count = 1;
  fwrite(address, count, length, ff->f);
  pushIor(vm, ferror(ff->f) == 0);
}

/* ( c-addr u1 fileid -- ior ) */
static void ficlPrimitiveWriteLine(ficlVm *vm)
{
#define h_ficlPrimitiveWriteLine "( c-addr u fileid -- ior )"
  ficlFile *ff;
  size_t length, count;
  void *address;

  FICL_STACK_CHECK(vm->dataStack, 3, 1);
  ff = (ficlFile *)ficlStackPopPointer(vm->dataStack);
  length = (size_t)ficlStackPopInteger(vm->dataStack);
  address = (void *)ficlStackPopPointer(vm->dataStack);
  clearerr(ff->f);
  count = 1;
  if (fwrite(address, count, length, ff->f) == length)
    fwrite("\n", count, count, ff->f);
  pushIor(vm, ferror(ff->f) == 0);
}

/* ( ud fileid -- ior ) */
static void ficlPrimitiveRepositionFile(ficlVm *vm)
{
#define h_ficlPrimitiveRepositionFile "( ud fileid -- ior )"
  ficlFile *ff;
  long loc;

  FICL_STACK_CHECK(vm->dataStack, 2, 1);
  ff = (ficlFile *)ficlStackPopPointer(vm->dataStack);
  loc = (long)ficlStackPopUnsigned(vm->dataStack);
  pushIor(vm, fseek(ff->f, loc, SEEK_SET) == 0);
}

/* ( fileid -- ior ) */
static void ficlPrimitiveFlushFile(ficlVm *vm)
{
#define h_ficlPrimitiveFlushFile "( fileid -- ior )"
  ficlFile *ff;

  FICL_STACK_CHECK(vm->dataStack, 1, 1);
  ff = (ficlFile *)ficlStackPopPointer(vm->dataStack);
  pushIor(vm, fflush(ff->f) == 0);
}

/* ( ud fileid -- ior ) */
static void ficlPrimitiveResizeFile(ficlVm *vm)
{
#define h_ficlPrimitiveResizeFile "( ud fileid -- ior )"
  ficlFile *ff;
  size_t ud;

  FICL_STACK_CHECK(vm->dataStack, 2, 1);
  ff = (ficlFile *)ficlStackPopPointer(vm->dataStack);
  ud = (size_t)ficlStackPopInteger(vm->dataStack);
  pushIor(vm, ficlFileTruncate(ff, ud) == 0);
}

void ficlSystemCompileFile(ficlSystem *sys)
{
  ficlDictionary *dict = ficlSystemGetDictionary(sys);
  ficlDictionary *env = ficlSystemGetEnvironment(sys);

  FICL_ASSERT(dict != NULL);
  FICL_ASSERT(env != NULL);

  FICL_PRIM_DOC(dict, "create-file",     ficlPrimitiveCreateFile);
  FICL_PRIM_DOC(dict, "open-file",       ficlPrimitiveOpenFile);
  FICL_PRIM_DOC(dict, "close-file",      ficlPrimitiveCloseFile);
  FICL_PRIM_DOC(dict, "include-file",    ficlPrimitiveIncludeFile);
  FICL_PRIM_DOC(dict, "read-file",       ficlPrimitiveReadFile);
  FICL_PRIM_DOC(dict, "read-line",       ficlPrimitiveReadLine);
  FICL_PRIM_DOC(dict, "write-file",      ficlPrimitiveWriteFile);
  FICL_PRIM_DOC(dict, "write-line",      ficlPrimitiveWriteLine);
  FICL_PRIM_DOC(dict, "file-position",   ficlPrimitiveFilePosition);
  FICL_PRIM_DOC(dict, "file-size",       ficlPrimitiveFileSize);
  FICL_PRIM_DOC(dict, "reposition-file", ficlPrimitiveRepositionFile);
  FICL_PRIM_DOC(dict, "file-status",     ficlPrimitiveFileStatus);
  FICL_PRIM_DOC(dict, "flush-file",      ficlPrimitiveFlushFile);
  FICL_PRIM_DOC(dict, "delete-file",     ficlPrimitiveDeleteFile);
  FICL_PRIM_DOC(dict, "rename-file",     ficlPrimitiveRenameFile);
  FICL_PRIM_DOC(dict, "resize-file",     ficlPrimitiveResizeFile);

  ficlDictionaryAppendConstant(env, "file", (ficlInteger)FICL_TRUE);
  ficlDictionaryAppendConstant(env, "file-ext", (ficlInteger)FICL_TRUE);
}
