/*
 * Adapted to work with FTH:
 *
 * Copyright (c) 2004-2012 Michael Scholz <mi-scholz@users.sourceforge.net>
 *
 * This file is part of FTH.
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
