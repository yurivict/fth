/*******************************************************************
** s e a r c h . c
** Forth Inspired Command Language
** ANS Forth SEARCH and SEARCH-EXT word-set written in C
** Author: John Sadler (john_sadler@alum.mit.edu)
** Created: 6 June 2000
** $Id: search.c,v 1.10 2010/08/12 13:57:22 asau Exp $
*******************************************************************/
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
 * @(#)search.c	1.31 10/17/13
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <string.h>
#include "ficl.h"

#include "fth.h"
#include "utils.h"

/**************************************************************************
 **                      d e f i n i t i o n s
 ** SEARCH ( -- )
 ** Make the compilation word list the same as the first word list in the
 ** search order. Specifies that the names of subsequent definitions will
 ** be placed in the compilation word list. Subsequent changes in the search
 ** order will not affect the compilation word list. 
 **************************************************************************/
static void ficlPrimitiveDefinitions(ficlVm *vm)
{
#define h_ficlPrimitiveDefinitions "( -- )  \
Make the compilation word list the same as the first word list in the search order.  \
Specifies that the names of subsequent definitions will be placed in the compilation word list.  \
Subsequent changes in the search order will not affect the compilation word list."
  ficlDictionary *dictionary = ficlVmGetDictionary(vm);

  FICL_ASSERT(dictionary != NULL);

  if (dictionary->wordlistCount < 1)
    ficlVmThrowException(vm, FICL_VM_STATUS_SEARCH_UNDERFLOW, NULL);

  dictionary->compilationWordlist = dictionary->wordlists[dictionary->wordlistCount-1];
}

/**************************************************************************
 **                      f o r t h - w o r d l i s t
 ** SEARCH ( -- wid )
 ** Return wid, the identifier of the word list that includes all standard
 ** words provided by the implementation. This word list is initially the
 ** compilation word list and is part of the initial search order. 
 **************************************************************************/
static void ficlPrimitiveForthWordlist(ficlVm *vm)
{
#define h_ficlPrimitiveForthWordlist "( -- wid )  \
Return wid, the identifier of the word list that includes all standard \
words provided by the implementation.  \
This word list is initially the compilation word list and is part of the initial search order."
  FICL_STACK_CHECK(vm->dataStack, 0, 1);
  ficlStackPushPointer(vm->dataStack, ficlVmGetDictionary(vm)->forthWordlist);
}

/**************************************************************************
 **                      g e t - c u r r e n t
 ** SEARCH ( -- wid )
 ** Return wid, the identifier of the compilation word list. 
 **************************************************************************/
static void ficlPrimitiveGetCurrent(ficlVm *vm)
{
#define h_ficlPrimitiveGetCurrent "( -- wid )  \
Return wid, the identifier of the compilation word list."
  FICL_STACK_CHECK(vm->dataStack, 0, 1);
  ficlStackPushPointer(vm->dataStack, ficlVmGetDictionary(vm)->compilationWordlist);
}

/**************************************************************************
 **                      g e t - o r d e r
 ** SEARCH ( -- widn ... wid1 n )
 ** Returns the number of word lists n in the search order and the word list
 ** identifiers widn ... wid1 identifying these word lists. wid1 identifies
 ** the word list that is searched first, and widn the word list that is
 ** searched last. The search order is unaffected.
 **************************************************************************/
static void ficlPrimitiveGetOrder(ficlVm *vm)
{
#define h_ficlPrimitiveGetOrder "( -- widn ... wid1 n )  \
Returns the number of word lists n in the search order and the word list \
identifiers WIDN ... WID1 identifying these word lists.  \
WID1 identifies the word list that is searched first, \
and widn the word list that is searched last.  \
The search order is unaffected."
  ficlDictionary *dictionary = ficlVmGetDictionary(vm);
  ficlInteger i, wordlistCount = dictionary->wordlistCount;

  FICL_STACK_CHECK(vm->dataStack, 0, wordlistCount + 1);
  for (i = 0; i < wordlistCount; i++)
    ficlStackPushPointer(vm->dataStack, dictionary->wordlists[i]);
  ficlStackPushInteger(vm->dataStack, wordlistCount);
}

/**************************************************************************
 **                      s e a r c h - w o r d l i s t
 ** SEARCH ( c-addr u wid -- 0 | xt 1 | xt -1 )
 ** Find the definition identified by the string c-addr u in the word list
 ** identified by wid. If the definition is not found, return zero. If the
 ** definition is found, return its execution token xt and one (1) if the
 ** definition is immediate, minus-one (-1) otherwise. 
 **************************************************************************/
static void ficlPrimitiveSearchWordlist(ficlVm *vm)
{
#define h_ficlPrimitiveSearchWordlist "( c-addr u wid -- 0 | xt 1 | xt -1 )  \
Find the definition identified by the string C-ADDR U in the word list identified by WID.  \
If the definition is not found, return zero.  \
If the definition is found, return its execution token XT and one (1) if the \
definition is immediate, minus-one (-1) otherwise."
  ficlString name;
  ficlUnsigned hashCode;
  ficlWord *word;
  ficlHash *hash = ficlStackPopPointer(vm->dataStack);

  FICL_STACK_CHECK(vm->dataStack, 3, 1);
  name.length = ficlStackPopUnsigned(vm->dataStack);
  name.text   = ficlStackPopPointer(vm->dataStack);
  hashCode    = ficlHashCode(name);
  word        = ficlHashLookup(hash, name, hashCode);

  if (word)
  {
    ficlStackPushPointer(vm->dataStack, word);
    ficlStackPushInteger(vm->dataStack, (ficlWordIsImmediate(word) ? 1L : -1L));
  }
  else
    ficlStackPushUnsigned(vm->dataStack, 0UL);
}

/**************************************************************************
 **                      s e t - c u r r e n t
 ** SEARCH ( wid -- )
 ** Set the compilation word list to the word list identified by wid. 
 **************************************************************************/
static void ficlPrimitiveSetCurrent(ficlVm *vm)
{
#define h_ficlPrimitiveSetCurrent "( wid -- )  \
Set the compilation word list to the word list identified by WID."
  FICL_STACK_CHECK(vm->dataStack, 1, 0);
  ficlVmGetDictionary(vm)->compilationWordlist = ficlStackPopPointer(vm->dataStack);
}

/**************************************************************************
 **                      s e t - o r d e r
 ** SEARCH ( widn ... wid1 n -- )
 ** Set the search order to the word lists identified by widn ... wid1.
 ** Subsequently, word list wid1 will be searched first, and word list
 ** widn searched last. If n is zero, empty the search order. If n is minus
 ** one, set the search order to the implementation-defined minimum
 ** search order. The minimum search order shall include the words
 ** FORTH-WORDLIST and SET-ORDER. A system shall allow n to
 ** be at least eight.
 **************************************************************************/
static void ficlPrimitiveSetOrder(ficlVm *vm)
{
#define h_ficlPrimitiveSetOrder "( widn ... wid1 n -- )  \
Set the search order to the word lists identified by WIDN ... WID1.  \
Subsequently, word list WID1 will be searched first, and word list widn searched last.  \
If N is zero, empty the search order.  \
If N is minus one, set the search order to the implementation-defined minimum search order.  \
The minimum search order shall include the words FORTH-WORDLIST and SET-ORDER.  \
A system shall allow N to be at least eight."
  ficlInteger i, wordlistCount = ficlStackPopInteger(vm->dataStack);
  ficlDictionary *dictionary = ficlVmGetDictionary(vm);

  if (wordlistCount > FICL_MAX_WORDLISTS)
    ficlVmThrowException(vm, FICL_VM_STATUS_DICT_OVERFLOW, NULL);

  if (wordlistCount >= 0)
  {
    dictionary->wordlistCount = wordlistCount;

    for (i = wordlistCount-1; i >= 0; --i)
      dictionary->wordlists[i] = ficlStackPopPointer(vm->dataStack);
  }
  else
    ficlDictionaryResetSearchOrder(dictionary);
}

/**************************************************************************
 **                      f i c l - w o r d l i s t
 ** SEARCH ( -- wid )
 ** Create a new empty word list, returning its word list identifier wid.
 ** The new word list may be returned from a pool of preallocated word
 ** lists or may be dynamically allocated in data space. A system shall
 ** allow the creation of at least 8 new word lists in addition to any
 ** provided as part of the system. 
 ** Notes: 
 ** 1. Ficl creates a new single-list hash in the dictionary and returns
 **    its address.
 ** 2. ficl-wordlist takes an arg off the stack indicating the number of
 **    hash entries in the wordlist. Ficl 2.02 and later define WORDLIST as
 **    : wordlist 1 ficl-wordlist ;
 **************************************************************************/
static void ficlPrimitiveFiclWordlist(ficlVm *vm)
{
#define h_ficlPrimitiveFiclWordlist "( u -- wid )  \
Create a new empty word list, returning its word list identifier WID.  \
The new word list may be returned from a pool of preallocated word \
lists or may be dynamically allocated in data space.  \
A system shall allow the creation of at least 8 new word lists \
in addition to any provided as part of the system.\n\
Notes:\n\
1. Ficl creates a new single-list hash in the dictionary and returns its address.\n\
2. ficl-wordlist takes an arg off the stack indicating \
the number of hash entries in the wordlist.  \
Ficl 2.02 and later define WORDLIST as : WORDLIST 1 FICL-WORDLIST ;"
  ficlDictionary *dictionary = ficlVmGetDictionary(vm);
  ficlHash *hash;
  ficlUnsigned nBuckets;
    
  FICL_STACK_CHECK(vm->dataStack, 1, 1);
  nBuckets = ficlStackPopUnsigned(vm->dataStack);
  hash = ficlDictionaryCreateWordlist(dictionary, (int)nBuckets);
  ficlStackPushPointer(vm->dataStack, hash);
}

/**************************************************************************
 **                      S E A R C H >
 ** Ficl  ( -- wid )
 ** Pop wid off the search order. Error if the search order is empty
 **************************************************************************/
static void ficlPrimitiveSearchPop(ficlVm *vm)
{
#define h_ficlPrimitiveSearchPop "( -- wid )  \
Pop WID off the search order.  \
Error if the search order is empty."
  ficlDictionary *dictionary = ficlVmGetDictionary(vm);
  ficlInteger wordlistCount;

  FICL_STACK_CHECK(vm->dataStack, 0, 1);
  wordlistCount = dictionary->wordlistCount;

  if (wordlistCount == 0)
    ficlVmThrowException(vm, FICL_VM_STATUS_SEARCH_UNDERFLOW, NULL);
  ficlStackPushPointer(vm->dataStack, dictionary->wordlists[--dictionary->wordlistCount]);
}

/**************************************************************************
 **                      > S E A R C H
 ** Ficl  ( wid -- )
 ** Push wid onto the search order. Error if the search order is full.
 **************************************************************************/
static void ficlPrimitiveSearchPush(ficlVm *vm)
{
#define h_ficlPrimitiveSearchPush "( wid -- )  \
Push WID onto the search order.  \
Error if the search order is full."
  ficlDictionary *dictionary = ficlVmGetDictionary(vm);

  FICL_STACK_CHECK(vm->dataStack, 1, 0);
  if (dictionary->wordlistCount > FICL_MAX_WORDLISTS)
    ficlVmThrowException(vm, FICL_VM_STATUS_SEARCH_OVERFLOW, NULL);
  dictionary->wordlists[dictionary->wordlistCount++] = ficlStackPopPointer(vm->dataStack);
}

/**************************************************************************
 **                      W I D - G E T - N A M E
 ** Ficl  ( wid -- c-addr u )
 ** Get wid's (optional) name and push onto stack as a counted string
 **************************************************************************/
static void ficlPrimitiveWidGetName(ficlVm *vm)
{
#define h_ficlPrimitiveWidGetName "( wid -- c-addr u )  \
Get WID's (optional) name and push onto stack as a counted string."
  ficlHash *hash;

  FICL_STACK_CHECK(vm->dataStack, 1, 2);
  hash = ficlStackPopPointer(vm->dataStack);

  if (hash == NULL)
    push_forth_string(vm, "no hash");
  else
    push_forth_string(vm, hash->name);
}

/**************************************************************************
 **                      W I D - S E T - N A M E
 ** Ficl  ( wid c-addr -- )
 ** Set wid's name pointer to the \0 terminated string address supplied
 **************************************************************************/
static void ficlPrimitiveWidSetName(ficlVm *vm)
{
#define h_ficlPrimitiveWidSetName "( wid c-addr -- )  \
Set WID's name pointer to the NULL terminated string address supplied."
  ficlCell cn, ch;
  char *name;
  ficlHash *hash;

  FICL_STACK_CHECK(vm->dataStack, 2, 0);
  cn = ficlVmPop(vm);
  ch = ficlVmPop(vm);
  name = VM_STACK_VOIDP_REF(&cn);
  hash = VM_STACK_VOIDP_REF(&ch);
  hash->name = name;
}

/**************************************************************************
 **                      setParentWid
 ** Ficl
 ** setparentwid   ( parent-wid wid -- )
 ** Set WID's link field to the parent-wid. search-wordlist will 
 ** iterate through all the links when finding words in the child wid.
 **************************************************************************/
static void ficlPrimitiveSetParentWid(ficlVm *vm)
{
#define h_ficlPrimitiveSetParentWid "( parent-wid wid -- )  \
Set WID's link field to the PARENT-WID.  \
SEARCH-WORDLIST will iterate through all the links when finding words in the child wid."
  ficlHash *parent, *child;

  FICL_STACK_CHECK(vm->dataStack, 2, 0);
  child  = (ficlHash *)ficlStackPopPointer(vm->dataStack);
  parent = (ficlHash *)ficlStackPopPointer(vm->dataStack);
  child->link = parent;
}

/**************************************************************************
 **                    f i c l C o m p i l e S e a r c h
 ** Builds the primitive wordset and the environment-query namespace.
 **************************************************************************/
void ficlSystemCompileSearch(ficlSystem *system)
{
  ficlDictionary *dict = ficlSystemGetDictionary(system);
  ficlDictionary *env = ficlSystemGetEnvironment(system);

  FICL_ASSERT(dict != NULL);
  FICL_ASSERT(env != NULL);

  /*
  ** optional SEARCH-ORDER word set 
  */
  FICL_PRIM_DOC(dict, ">search",         ficlPrimitiveSearchPush);
  FICL_PRIM_DOC(dict, "search>",         ficlPrimitiveSearchPop);
  FICL_PRIM_DOC(dict, "definitions",     ficlPrimitiveDefinitions);
  FICL_PRIM_DOC(dict, "forth-wordlist",  ficlPrimitiveForthWordlist);
  FICL_PRIM_DOC(dict, "get-current",     ficlPrimitiveGetCurrent);
  FICL_PRIM_DOC(dict, "get-order",       ficlPrimitiveGetOrder);
  FICL_PRIM_DOC(dict, "search-wordlist", ficlPrimitiveSearchWordlist);
  FICL_PRIM_DOC(dict, "set-current",     ficlPrimitiveSetCurrent);
  FICL_PRIM_DOC(dict, "set-order",       ficlPrimitiveSetOrder);
  FICL_PRIM_DOC(dict, "ficl-wordlist",   ficlPrimitiveFiclWordlist);

  /*
  ** Set SEARCH environment query values
  */
  ficlDictionaryAppendConstant(env, "search-order", (ficlInteger)FICL_TRUE);
  ficlDictionaryAppendConstant(env, "search-order-ext", (ficlInteger)FICL_TRUE);
  ficlDictionaryAppendConstant(env, "wordlists", (ficlInteger)FICL_MAX_WORDLISTS);

  FICL_PRIM_DOC(dict, "wid-get-name",  ficlPrimitiveWidGetName);
  FICL_PRIM_DOC(dict, "wid-set-name",  ficlPrimitiveWidSetName);
  FICL_PRIM_DOC(dict, "wid-set-super", ficlPrimitiveSetParentWid);
}
