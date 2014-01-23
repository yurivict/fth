/*******************************************************************
** f i c l . c
** Forth Inspired Command Language - external interface
** Author: John Sadler (john_sadler@alum.mit.edu)
** Created: 19 July 1997
** $Id: system.c,v 1.2 2010/09/10 10:35:54 asau Exp $
*******************************************************************/
/*
** This is an ANS Forth interpreter written in C.
** Ficl uses Forth syntax for its commands, but turns the Forth 
** model on its head in other respects.
** Ficl provides facilities for interoperating
** with programs written in C: C functions can be exported to Ficl,
** and Ficl commands can be executed via a C calling interface. The
** interpreter is re-entrant, so it can be used in multiple instances
** in a multitasking system. Unlike Forth, Ficl's outer interpreter
** expects a text block as input, and returns to the caller after each
** text block, so the data pump is somewhere in external code in the 
** style of TCL.
**
** Code is written in ANSI C for portability. 
*/
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
 * Copyright (c) 2004-2014 Michael Scholz <mi-scholz@users.sourceforge.net>
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
 * @(#)system.c	1.42 1/23/14
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "ficl.h"

#include "fth.h"
#include "utils.h"

/**************************************************************************
 **                      f i c l I n i t S y s t e m
 ** Binds a global dictionary to the interpreter system. 
 ** You specify the address and size of the allocated area.
 ** After that, Ficl manages it.
 ** First step is to set up the static pointers to the area.
 ** Then write the "precompiled" portion of the dictionary in.
 ** The dictionary needs to be at least large enough to hold the
 ** precompiled part. Try 1K cells minimum. Use "words" to find
 ** out how much of the dictionary is used at any time.
 **************************************************************************/
ficlSystem *ficlSystemCreate(ficlSystemInformation *fsi)
{
  unsigned int dictionarySize;
  unsigned int environmentSize;
  unsigned int stackSize;
  unsigned int returnSize;
  ficlSystem *sys;
  ficlSystemInformation fauxInfo;

  if (fsi == NULL)
  {
    fsi = &fauxInfo;
    ficlSystemInformationInitialize(fsi);
  }

  sys = FTH_CALLOC(1, sizeof(ficlSystem));
  FICL_ASSERT(sys != NULL);

  dictionarySize  = FICL_MAX(fsi->dictionarySize,  FICL_MIN_DICTIONARY_SIZE);
  environmentSize = FICL_MAX(fsi->environmentSize, FICL_MIN_ENVIRONMENT_SIZE);
  stackSize       = FICL_MAX(fsi->stackSize,       FICL_MIN_STACK_SIZE);
  returnSize      = FICL_MAX(fsi->returnSize,      FICL_MIN_RETURN_SIZE);

  sys->dictionary = ficlDictionaryCreateHashed(sys, dictionarySize, FICL_HASH_SIZE);
  sys->dictionary->forthWordlist->name = "forth-wordlist";

  sys->environment = ficlDictionaryCreate(sys, environmentSize);
  sys->environment->forthWordlist->name = "environment";

  sys->callback.textIn = fsi->textIn;
  sys->callback.textOut = fsi->textOut;
  sys->callback.errorOut = fsi->errorOut;
  sys->callback.stdin_fileno = fsi->stdin_fileno;
  sys->callback.stdout_fileno = fsi->stdout_fileno;
  sys->callback.stderr_fileno = fsi->stderr_fileno;
  sys->callback.stdin_ptr = fsi->stdin_ptr;
  sys->callback.stdout_ptr = fsi->stdout_ptr;
  sys->callback.stderr_ptr = fsi->stderr_ptr;
  sys->callback.context = fsi->context;
  sys->callback.system = sys;
  sys->callback.vm = NULL;
  sys->stackSize = stackSize;
  sys->returnSize = returnSize;

  /*
  ** The locals dictionary is only searched while compiling,
  ** but this is where speed is most important. On the other
  ** hand, the dictionary gets emptied after each use of locals
  ** The need to balance search speed with the cost of the 'empty'
  ** operation led me to select a single-threaded list...
  */
  sys->locals = ficlDictionaryCreate(sys, (unsigned)(fsi->localsSize * FICL_CELLS_PER_WORD));

  /*
  ** Now create a temporary VM to compile the softwords. Since all VMs are
  ** linked into the vmList of ficlSystem, we don't have to pass the VM
  ** to ficlCompileSoftCore -- it just hijacks whatever it finds in the VM list.
  ** Ficl 2.05: vmCreate no longer depends on the presence of INTERPRET in the
  ** dictionary, so a VM can be created before the dictionary is built. It just
  ** can't do much...
  */
  /* XXX moved to src/misc.c [ms] */
/*
  ficlSystemCreateVm(sys);
#define ADD_COMPILE_FLAG(name) ficlDictionaryAppendConstant(env, #name, name)
  ADD_COMPILE_FLAG(FICL_PLATFORM_ALIGNMENT);

#define ADD_COMPILE_STRING(name) ficlDictionaryAppendConstant(env, #name, 0)
  ADD_COMPILE_STRING(FICL_NAME);
  ADD_COMPILE_STRING(FICL_FORTH_NAME);
  ADD_COMPILE_STRING(FICL_PLATFORM_ARCHITECTURE);
  ADD_COMPILE_STRING(FICL_PLATFORM_OS);
  ADD_COMPILE_STRING(FICL_PLATFORM_VENDOR);
  ficlSystemDestroyVm(sys->vmList);
*/
  return sys;
}

/**************************************************************************
 **                      f i c l T e r m S y s t e m
 ** Tear the system down by deleting the dictionaries and all VMs.
 ** This saves you from having to keep track of all that stuff.
 **************************************************************************/
void ficlSystemDestroy(ficlSystem *system)
{
  if (system->dictionary)
    ficlDictionaryDestroy(system->dictionary);

  system->dictionary = NULL;

  if (system->environment)
    ficlDictionaryDestroy(system->environment);

  system->environment = NULL;

  if (system->locals)
    ficlDictionaryDestroy(system->locals);

  system->locals = NULL;

  while (system->vmList != NULL)
  {
    ficlVm *vm = system->vmList;

    system->vmList = system->vmList->link;
    ficlVmDestroy(vm);
  }

  FTH_FREE(system);
  system = NULL;
}

/**************************************************************************
 **                      f i c l A d d P a r s e S t e p
 ** Appends a parse step function to the end of the parse list (see 
 ** ficlParseStep notes in ficl.h for details). Returns 0 if successful,
 ** nonzero if there's no more room in the list.
 **************************************************************************/
int ficlSystemAddParseStep(ficlSystem *system, ficlWord *word)
{
  int i;

  for (i = 0; i < FICL_MAX_PARSE_STEPS; i++)
  {
    if (system->parseList[i] == NULL)
    {
      system->parseList[i] = word;
      return 0;
    }
  }

  return 1;
}

/*
** Compile a word into the dictionary that invokes the specified ficlParseStep
** function. It is up to the user (as usual in Forth) to make sure the stack 
** preconditions are valid (there needs to be a counted string on top of the stack)
** before using the resulting word.
*/
void ficlSystemAddPrimitiveParseStep(ficlSystem *system, char *name, ficlParseStep pStep)
{
  ficlDictionary *dict = system->dictionary;
  ficlWord *word = ficlDictionaryAppendPrimitive(dict, name, ficlPrimitiveParseStepParen,
						 FICL_WORD_DEFAULT);

  ficlDictionaryAppendPointer(dict, (void *)pStep);
  ficlSystemAddParseStep(system, word);
}

/**************************************************************************
 **                      f i c l N e w V M
 ** Create a new virtual machine and link it into the system list
 ** of VMs for later cleanup by ficlTermSystem.
 **************************************************************************/
ficlVm *ficlSystemCreateVm(ficlSystem *system)
{
  ficlVm *vm = ficlVmCreate(NULL, (unsigned)system->stackSize, (unsigned)system->returnSize);

  vm->link = system->vmList;
  memcpy(&(vm->callback), &(system->callback), sizeof(system->callback));
  vm->callback.vm = vm;
  vm->callback.system = system;
  system->vmList = vm;
  return vm;
}

/**************************************************************************
 **                      f i c l F r e e V M
 ** Removes the VM in question from the system VM list and deletes the
 ** memory allocated to it. This is an optional call, since ficlTermSystem
 ** will do this cleanup for you. This function is handy if you're going to
 ** do a lot of dynamic creation of VMs.
 **************************************************************************/
void ficlSystemDestroyVm(ficlVm *vm)
{
  ficlSystem *sys;
  ficlVm *pList;

  FICL_ASSERT(vm != NULL);
  sys = vm->callback.system;
  pList = sys->vmList;

  if (sys->vmList == vm)
    sys->vmList = sys->vmList->link;
  else
    for (; pList != NULL; pList = pList->link)
    {
      if (pList->link == vm)
      {
	pList->link = vm->link;
	break;
      }
    }
  
  if (pList)
    ficlVmDestroy(vm);
}

/**************************************************************************
 **                      f i c l L o o k u p
 ** Look in the system dictionary for a match to the given name. If
 ** found, return the address of the corresponding ficlWord. Otherwise
 ** return NULL.
 **************************************************************************/
ficlWord *ficlSystemLookup(ficlSystem *system, char *name)
{
  ficlString s;

  FICL_STRING_SET_FROM_CSTRING(s, name);
  return ficlDictionaryLookup(system->dictionary, s);
}

/**************************************************************************
 **                      f i c l G e t D i c t
 ** Returns the address of the system dictionary
 **************************************************************************/
ficlDictionary *ficlSystemGetDictionary(ficlSystem *system)
{
  return system->dictionary;
}

/**************************************************************************
 **                      f i c l G e t E n v
 ** Returns the address of the system environment space
 **************************************************************************/
ficlDictionary *ficlSystemGetEnvironment(ficlSystem *system)
{
  return system->environment;
}

/**************************************************************************
 **                      f i c l G e t L o c
 ** Returns the address of the system locals dictionary. This dictionary is
 ** only used during compilation, and is shared by all VMs.
 **************************************************************************/
ficlDictionary *ficlSystemGetLocals(ficlSystem *system)
{
  return system->locals;
}

/**************************************************************************
 **                      f i c l L o o k u p L o c
 ** Same as dictLookup, but looks in system locals dictionary first...
 ** Assumes locals dictionary has only one wordlist...
 **************************************************************************/
ficlWord *ficlSystemLookupLocal(ficlSystem *system, ficlString name)
{
  ficlWord *word = NULL;
  ficlDictionary *dictionary = system->dictionary;
  ficlHash *hash = ficlSystemGetLocals(system)->forthWordlist;
  ficlUnsigned hashCode = ficlHashCode(name);
  int i;

  FICL_ASSERT(hash != NULL);
  FICL_ASSERT(dictionary != NULL);

  /* 
  ** check the locals dictionary first... 
  */
  word = ficlHashLookup(hash, name, hashCode);

  /* 
  ** If no joy, (!word) ------------------------------v
  ** iterate over the search list in the main dictionary
  */
  for (i = (int)dictionary->wordlistCount - 1; (i >= 0) && (!word); --i)
  {
    hash = dictionary->wordlists[i];
    word = ficlHashLookup(hash, name, hashCode);
  }
  return word;
}
