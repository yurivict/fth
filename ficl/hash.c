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
 * @(#)hash.c	1.17 10/17/13
 */

#include "ficl.h"
#include <ctype.h>

#define FICL_ASSERT_PHASH(expression) FICL_ASSERT((expression) != NULL)

/**************************************************************************
 **                      h a s h F o r g e t
 ** Unlink all words in the hash that have addresses greater than or
 ** equal to the address supplied. Implementation factor for FORGET
 ** and MARKER.
 **************************************************************************/
void ficlHashForget(ficlHash *hash, void *where)
{
  ficlWord *pWord;
  unsigned i;

  FICL_ASSERT_PHASH(hash);
  FICL_ASSERT_PHASH(where);

  for (i = 0; i < hash->size; i++)
  {
    pWord = hash->table[i];

    while ((void *)pWord >= where)
      pWord = pWord->link;

    hash->table[i] = pWord;
  }
}

/**************************************************************************
 **                      h a s h H a s h C o d e
 ** 
 ** Generate a 16 bit hashcode from a character string using a rolling
 ** shift and add stolen from PJ Weinberger of Bell Labs fame. Case folds
 ** the name before hashing it...
 ** N O T E : If string has zero length, returns zero.
 **************************************************************************/
ficlUnsigned ficlHashCode(ficlString s)
{   
  /* hashPJW */
  ficlUnsigned8 *trace;
  ficlUnsigned16 code = (ficlUnsigned16)s.length;
  ficlUnsigned16 shift = 0;

  if (s.length == 0)
    return 0;

  /* changed to run without errors under Purify -- lch */
  for (trace = (ficlUnsigned8 *)s.text; s.length && *trace; trace++, s.length--)
  {
    code = (ficlUnsigned16)((code << 4) + tolower(*trace));
    shift = (ficlUnsigned16)(code & 0xf000);

    if (shift)
    {
      code ^= (shift >> (ficlUnsigned16)8);
      code ^= shift;
    }
  }

  return ((ficlUnsigned)code);
}

/**************************************************************************
 **                      h a s h I n s e r t W o r d
 ** Put a word into the hash table using the word's hashcode as
 ** an index (modulo the table size).
 **************************************************************************/
void ficlHashInsertWord(ficlHash *hash, ficlWord *word)
{
  ficlWord **pList;

  FICL_ASSERT_PHASH(hash);
  FICL_ASSERT_PHASH(word);

  if (hash->size == 1)
  {
    pList = hash->table;
  }
  else
  {
    pList = hash->table + (word->hash % hash->size);
  }

  word->link = *pList;
  *pList = word;
}

/**************************************************************************
 **                      h a s h L o o k u p
 ** Find a name in the hash table given the hashcode and text of the name.
 ** Returns the address of the corresponding ficlWord if found, 
 ** otherwise NULL.
 ** Note: outer loop on link field supports inheritance in wordlists.
 ** It's not part of ANS Forth - Ficl only. hashReset creates wordlists
 ** with NULL link fields.
 **************************************************************************/
ficlWord *ficlHashLookup(ficlHash *hash, ficlString name, ficlUnsigned hashCode)
{
  ficlUnsigned nCmp = name.length;
  ficlWord *word;
  ficlUnsigned hashIdx;

  if (nCmp > FICL_NAME_LENGTH)
    nCmp = FICL_NAME_LENGTH;

  for (; hash != NULL; hash = hash->link)
  {
    if (hash->size > 1)
      hashIdx = hashCode % hash->size;
    else            /* avoid the modulo op for single threaded lists */
      hashIdx = 0;

    for (word = hash->table[hashIdx]; word != NULL; word = word->link)
      if ((word->length == name.length) &&
	  (ficlStrincmp(name.text, word->name, nCmp) == 0))
	return word;
  }
  return NULL;
}

/**************************************************************************
 **                             h a s h R e s e t
 ** Initialize a ficlHash to empty state.
 **************************************************************************/
void ficlHashReset(ficlHash *hash)
{
  unsigned i;

  FICL_ASSERT_PHASH(hash);

  for (i = 0; i < hash->size; i++)
    hash->table[i] = NULL;

  hash->link = NULL;
  hash->name = NULL;
}
