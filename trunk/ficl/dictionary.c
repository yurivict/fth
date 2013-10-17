/*******************************************************************
** d i c t . c
** Forth Inspired Command Language - dictionary methods
** Author: John Sadler (john_sadler@alum.mit.edu)
** Created: 19 July 1997
** $Id: dictionary.c,v 1.2 2010/09/12 15:14:52 asau Exp $
*******************************************************************/
/*
** This file implements the dictionary -- Ficl's model of 
** memory management. All Ficl words are stored in the
** dictionary. A word is a named chunk of data with its
** associated code. Ficl treats all words the same, even
** precompiled ones, so your words become first-class
** extensions of the language. You can even define new 
** control structures.
**
** 29 jun 1998 (sadler) added variable sized hash table support
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
 * @(#)dictionary.c	1.71 10/17/13
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ficl.h"

#include "fth.h"
#include "utils.h"

#define FICL_SAFE_CALLBACK_FROM_SYSTEM(system)				\
	(((system) != NULL) ? &((system)->callback) : NULL)
#define FICL_SAFE_SYSTEM_FROM_DICTIONARY(dictionary)			\
	(((dictionary) != NULL) ? (dictionary)->system : NULL)

/**************************************************************************
 **                      d i c t A b o r t D e f i n i t i o n
 ** Abort a definition in process: reclaim its memory and unlink it
 ** from the dictionary list. Assumes that there is a smudged 
 ** definition in process...otherwise does nothing.
 ** NOTE: this function is not smart enough to unlink a word that
 ** has been successfully defined (ie linked into a hash). It
 ** only works for defs in process. If the def has been unsmudged,
 ** nothing happens.
 **************************************************************************/
void ficlDictionaryAbortDefinition(ficlDictionary *dict)
{
  ficlWord *word = dict->smudge;

  if (word->flags & FICL_WORD_SMUDGED)
    CELL_VOIDP_SET(dict->here, word->name);
}

/**************************************************************************
 **                      d i c t A l i g n
 ** Align the dictionary's free space pointer
 **************************************************************************/
void ficlDictionaryAlign(ficlDictionary *dict)
{
  dict->here = ficlAlignPointer(dict->here);
}

/**************************************************************************
 **                      d i c t A l l o t
 ** Allocate or remove n chars of dictionary space, with
 ** checks for underrun and overrun
 **************************************************************************/
void ficlDictionaryAllot(ficlDictionary *dict, int n)
{
  char *here = (char *)dict->here;

  here += n;
  dict->here = FICL_POINTER_TO_CELL(here);
}

/**************************************************************************
 **                      d i c t A l l o t C e l l s
 ** Reserve space for the requested number of ficlCells in the
 ** dictionary. If nficlCells < 0 , removes space from the dictionary.
 **************************************************************************/
void ficlDictionaryAllotCells(ficlDictionary *dict, int nficlCells)
{
  dict->here += nficlCells;
}

/**************************************************************************
 **                      d i c t A p p e n d C e l l
 ** Append the specified ficlCell to the dictionary
 **************************************************************************/
void ficlDictionaryAppendCell(ficlDictionary *dict, ficlCell c)
{
  *dict->here++ = c;
}

void ficlDictionaryAppendPointer(ficlDictionary *dict, void *p)
{
  ficlCell c;

  CELL_VOIDP_SET(&c, p);
  *dict->here++ = c;
}

void ficlDictionaryAppendFTH(ficlDictionary *dict, FTH fp)
{
  ficlCell c;

  CELL_FTH_SET(&c, fp);
  *dict->here++ = c;
}

void ficlDictionaryAppendInteger(ficlDictionary *dict, ficlInteger i)
{
  ficlCell c;

  CELL_INT_SET(&c, i);
  *dict->here++ = c;
}

/**************************************************************************
 **                      d i c t A p p e n d C h a r
 ** Append the specified char to the dictionary
 **************************************************************************/
void ficlDictionaryAppendCharacter(ficlDictionary *dict, char ch)
{
  ficlCell c;

  CELL_INT_SET(&c, (ficlInteger)ch);
  *dict->here++ = c;
}

/**************************************************************************
 **                      d i c t A p p e n d U N S
 ** Append the specified ficlUnsigned to the dictionary
 **************************************************************************/
void ficlDictionaryAppendUnsigned(ficlDictionary *dict, ficlUnsigned u)
{
  ficlCell c;

  CELL_UINT_SET(&c, u);
  *dict->here++ = c;
}

void *ficlDictionaryAppendData(ficlDictionary *dict, void *data, ficlInteger length)
{
  char *here    = (char *)dict->here;
  char *oldHere = here;
  char *from    = (char *)data;
 
  if (length == 0)
  {
    ficlDictionaryAlign(dict);
    return (char *)dict->here;
  }

  while (length)
  {
    *here++ = *from++;
    length--;
  }

  *here++ = '\0';
  dict->here = FICL_POINTER_TO_CELL(here);
  ficlDictionaryAlign(dict);
  return oldHere;
}

/**************************************************************************
 **                      d i c t C o p y N a m e
 ** Copy up to FICL_NAME_LENGTH characters of the name specified by s into
 ** the dictionary starting at "here", then NULL-terminate the name,
 ** point "here" to the next available byte, and return the address of
 ** the beginning of the name. Used by dictAppendWord.
 ** N O T E S :
 ** 1. "here" is guaranteed to be aligned after this operation.
 ** 2. If the string has zero length, align and return "here"
 **************************************************************************/
char *ficlDictionaryAppendString(ficlDictionary *dict, ficlString s)
{
  void *data = FICL_STRING_GET_POINTER(s);
  ficlInteger length = (ficlInteger)FICL_STRING_GET_LENGTH(s);

  if (length > FICL_NAME_LENGTH)
    length = FICL_NAME_LENGTH;
  return ficlDictionaryAppendData(dict, data, length);
}

ficlWord *ficlDictionaryAppendConstantInstruction(ficlDictionary *dict,
						  ficlString name,
						  ficlInstruction inst,
						  ficlInteger value)
{
  ficlWord *word = ficlDictionaryAppendWord(dict, name, (ficlPrimitive)inst, FICL_WORD_DEFAULT);

  if (word != NULL)
  {
    ficlDictionaryAppendUnsigned(dict, (ficlUnsigned)value);
    word->kind = FW_VARIABLE;
  }
  return word;
}

ficlWord *ficlDictionaryAppendConstant(ficlDictionary *dict, char *name, ficlInteger value)
{
  ficlString s;

  FICL_STRING_SET_FROM_CSTRING(s, name);
  return (ficlDictionaryAppendConstantInstruction(dict,
      s, (ficlInstruction)ficlInstructionConstantParen, value));
}

ficlWord *ficlDictionarySetConstantInstruction(ficlDictionary *dict,
					       ficlString name,
					       ficlInstruction inst,
					       ficlInteger value)
{
  ficlWord *word = ficlDictionaryLookup(dict, name);

  if (word == NULL)
    word = ficlDictionaryAppendConstantInstruction(dict, name, inst, value);
  else
  {
    word->code = (ficlPrimitive)inst;
    CELL_INT_SET(word->param, value);
  }
  return word;
}

ficlWord *ficlDictionarySetConstant(ficlDictionary *dict, char *name, ficlInteger value)
{
  ficlString s;
  
  FICL_STRING_SET_FROM_CSTRING(s, name);
  return (ficlDictionarySetConstantInstruction(dict,
      s, (ficlInstruction)ficlInstructionConstantParen, value));
}

ficlWord *ficlDictionaryAppendFTHConstantInstruction(ficlDictionary *dict,
						     ficlString name,
						     ficlInstruction inst,
						     FTH fp)
{
  ficlWord *word = ficlDictionaryAppendWord(dict, name, (ficlPrimitive)inst, FICL_WORD_DEFAULT);
  ficlCell c;
      
  CELL_FTH_SET(&c, fp);
  *dict->here++ = c;
  return word;
}

ficlWord *ficlDictionarySetFTHConstantInstruction(ficlDictionary *dict,
						  ficlString name,
						  ficlInstruction inst,
						  FTH fp)
{
  ficlWord *word = ficlDictionaryLookup(dict, name);

  if (word == NULL)
    word = ficlDictionaryAppendFTHConstantInstruction(dict, name, inst, fp);

  word->code = (ficlPrimitive)inst;
  CELL_FTH_SET(word->param, fp);
  return word;
}

ficlWord *ficlDictionarySetFTHConstant(ficlDictionary *dict, char *name, FTH obj)
{
  ficlString s;
  
  FICL_STRING_SET_FROM_CSTRING(s, name);
  return (ficlDictionarySetFTHConstantInstruction(dict,
      s, (ficlInstruction)ficlInstructionConstantParen, obj));
}

ficlWord *ficlDictionaryAppendPointerConstant(ficlDictionary *dict, char *name, void *p)
{
  ficlString s;
  ficlWord *word;

  FICL_STRING_SET_FROM_CSTRING(s, name);
  word = ficlDictionaryAppendWord(dict,
				  s,
				  (ficlPrimitive)ficlInstructionConstantParen,
				  FICL_WORD_DEFAULT);
  if (word != NULL)
  {
    ficlCell c;
      
    CELL_VOIDP_SET(&c, p);
    *dict->here++ = c;
  }
  return word;
}

ficlWord *ficlDictionaryAppendFTHConstant(ficlDictionary *dict, char *name, FTH fp)
{
  ficlString s;
  ficlWord *word;

  FICL_STRING_SET_FROM_CSTRING(s, name);
  word = ficlDictionaryAppendWord(dict,
				  s,
				  (ficlPrimitive)ficlInstructionConstantParen,
				  FICL_WORD_DEFAULT);
  if (word != NULL)
  {
    ficlCell c;
      
    CELL_FTH_SET(&c, fp);
    *dict->here++ = c;
  }
  return word;
}

/**************************************************************************
 **                      d i c t A p p e n d W o r d
 ** Create a new word in the dictionary with the specified
 ** ficlString, code, and flags. Does not require a NULL-terminated
 ** name.
 **************************************************************************/
ficlWord *ficlDictionaryAppendWord(ficlDictionary *dict, 
				   ficlString name, 
				   ficlPrimitive code,
				   ficlUnsigned flags)
{
  char *nameCopy;
  ficlWord *word;

  /*
  ** NOTE: ficlDictionaryAppendString advances "here" as a side-effect.
  ** It must execute before word is initialized.
  */
  nameCopy           = ficlDictionaryAppendString(dict, name);
  nameCopy[name.length] = '\0';
  word               = (ficlWord *)dict->here;
  dict->smudge       = word;
  word->hash         = ficlHashCode(name);
  word->code         = code;
  word->semiParen    = ficlInstructionSemiParen;
  word->flags        = (flags | FICL_WORD_SMUDGED);
  word->length       = name.length;
  word->name         = nameCopy;
  word->properties   = FTH_FALSE;
  word->file         = fth_current_file;
  word->line         = fth_current_line;
  word->current_word = word;
  word->current_file = word->file;
  word->current_line = word->line;
  word->primitive_p  = 1;
  word->req          = 0;
  word->opt          = 0;
  word->rest         = 0;
  word->argc         = -1;
  word->kind         = FW_WORD;
  word->func         = NULL;
  word->vfunc        = NULL;
  word->link         = NULL;
  /*
  ** Point "here" to first ficlCell of new word's param area...
  */
  dict->here         = word->param;

  if (!(flags & FICL_WORD_SMUDGED))
    ficlDictionaryUnsmudge(dict);
  return word;
}

/**************************************************************************
 **                      d i c t A p p e n d W o r d
 ** Create a new word in the dictionary with the specified
 ** name, code, and flags. Name must be NULL-terminated.
 **************************************************************************/
ficlWord *ficlDictionaryAppendPrimitive(ficlDictionary *dict, 
					char *name, 
					ficlPrimitive code,
					ficlUnsigned flags)
{
  ficlString s;

  FICL_STRING_SET_FROM_CSTRING(s, name);
  return ficlDictionaryAppendWord(dict, s, code, flags);
}

ficlWord *ficlDictionarySetPrimitive(ficlDictionary *dict, 
				     char *name, 
				     ficlPrimitive code,
				     ficlUnsigned flags)
{
  ficlString s;
  ficlWord *word;

  FICL_STRING_SET_FROM_CSTRING(s, name);
  word = ficlDictionaryLookup(dict, s);

  if (word == NULL)
    word = ficlDictionaryAppendPrimitive(dict, name, code, flags);
  else
  {
    word->code = (ficlPrimitive)code;
    word->flags = flags;
  }
  return word;
}

ficlWord *ficlDictionaryAppendInstruction(ficlDictionary *dict, 
					  char *name, 
					  ficlInstruction i,
					  ficlUnsigned flags)
{
  return ficlDictionaryAppendPrimitive(dict,
				       name,
				       (ficlPrimitive)i,
				       (ficlUnsigned)(FICL_WORD_INSTRUCTION | flags));
}

ficlWord *ficlDictionarySetInstruction(ficlDictionary *dict, 
				       char *name, 
				       ficlInstruction i,
				       ficlUnsigned flags)
{
  return ficlDictionarySetPrimitive(dict,
				    name,
				    (ficlPrimitive)i,
				    (ficlUnsigned)(FICL_WORD_INSTRUCTION | flags));
}

/**************************************************************************
 **                      d i c t C e l l s A v a i l
 ** Returns the number of empty ficlCells left in the dictionary
 **************************************************************************/
int ficlDictionaryCellsAvailable(ficlDictionary *dict)
{
  return ((int)dict->size - ficlDictionaryCellsUsed(dict));
}

/**************************************************************************
 **                      d i c t C e l l s U s e d
 ** Returns the number of ficlCells consumed in the dicionary
 **************************************************************************/
int ficlDictionaryCellsUsed(ficlDictionary *dict)
{
  return (int)(dict->here - dict->base);
}

/**************************************************************************
 **                      d i c t C r e a t e
 ** Create and initialize a dictionary with the specified number
 ** of ficlCells capacity, and no hashing (hash size == 1).
 **************************************************************************/
ficlDictionary *ficlDictionaryCreate(ficlSystem *system, unsigned size)
{
  return ficlDictionaryCreateHashed(system, size, 1);
}

ficlDictionary *ficlDictionaryCreateHashed(ficlSystem *system, unsigned size, unsigned bucketCount)
{
  ficlDictionary *dictionary;
  size_t nAlloc;

  nAlloc = sizeof(ficlDictionary) + (size * sizeof(ficlCell))
    + sizeof(ficlHash) + (bucketCount - 1) * sizeof(ficlWord *);

  dictionary = FTH_CALLOC(1, nAlloc);
  dictionary->size = size;
  dictionary->system = system;
  ficlDictionaryEmpty(dictionary, bucketCount);
  return dictionary;
}

/**************************************************************************
 **                      d i c t C r e a t e W o r d l i s t
 ** Create and initialize an anonymous wordlist
 **************************************************************************/
ficlHash *ficlDictionaryCreateWordlist(ficlDictionary *dict, int bucketCount)
{
  ficlHash *hash;
    
  ficlDictionaryAlign(dict);
  hash = (ficlHash *)dict->here;
  ficlDictionaryAllot(dict, (int)(sizeof(ficlHash) +
      (size_t)(bucketCount - 1) * sizeof(ficlWord *)));
  hash->size = (unsigned int)bucketCount;
  ficlHashReset(hash);
  return hash;
}

/**************************************************************************
 **                      d i c t D e l e t e 
 ** Free all memory allocated for the given dictionary 
 **************************************************************************/
void ficlDictionaryDestroy(ficlDictionary *dict)
{
  FTH_FREE(dict);
}

/**************************************************************************
 **                      d i c t E m p t y
 ** Empty the dictionary, reset its hash table, and reset its search order.
 ** Clears and (re-)creates the hash table with the size specified by nHash.
 **************************************************************************/
void ficlDictionaryEmpty(ficlDictionary *dict, unsigned bucketCount)
{
  ficlHash *hash;

  dict->here = dict->base;
  ficlDictionaryAlign(dict);
  hash = (ficlHash *)dict->here;
  ficlDictionaryAllot(dict, (int)(sizeof (ficlHash) + (bucketCount - 1) * sizeof (ficlWord *)));
  hash->size = bucketCount;
  ficlHashReset(hash);
  dict->forthWordlist = hash;
  dict->smudge = NULL;
  ficlDictionaryResetSearchOrder(dict);
}

/**************************************************************************
 **                      i s A F i c l W o r d
 ** Vet a candidate pointer carefully to make sure
 ** it's not some chunk o' inline data...
 ** It has to have a name, and it has to look
 ** like it's in the dictionary address range.
 ** NOTE: this excludes :noname words!
 **************************************************************************/
int ficlDictionaryIsAWord(ficlDictionary *dict, ficlWord *word)
{
  if ((((ficlInstruction)word) > ficlInstructionInvalid) &&
      (((ficlInstruction)word) < ficlInstructionLast))
    return 1;
  if (!ficlDictionaryIncludes(dict, word))
    return 0;
  if (!ficlDictionaryIncludes(dict, word->name))
    return 0;
  if ((word->link != NULL) && !ficlDictionaryIncludes(dict, word->link))
    return 0;
  if ((word->length == 0) || (word->name[word->length] != '\0'))
    return 0;
  if (word->name && (ficlUnsigned)strlen(word->name) != word->length)
    return 0;
  if (!word->name && word->length != 0)
    return 0;
  return 1;
}

/**************************************************************************
 **                      f i n d E n c l o s i n g W o r d
 ** Given a pointer to something, check to make sure it's an address in the 
 ** dictionary. If so, search backwards until we find something that looks
 ** like a dictionary header. If successful, return the address of the 
 ** ficlWord found. Otherwise return NULL.
 ** nSEARCH_CELLS sets the maximum neighborhood this func will search before giving up
 **************************************************************************/
#define nSEARCH_CELLS 100

ficlWord *ficlDictionaryFindEnclosingWord(ficlDictionary *dict, ficlCell *cell)
{
  ficlWord *word;
  int i;

  if (!ficlDictionaryIncludes(dict, (void *)cell))
    return NULL;

  for (i = nSEARCH_CELLS; i > 0; --i, --cell)
  {
    word = (ficlWord *)(cell + 1 - (sizeof(ficlWord) / sizeof(ficlCell)));
    if (ficlDictionaryIsAWord(dict, word))
      return word;
  }
  return NULL;
}

/**************************************************************************
 **                      d i c t I n c l u d e s
 ** Returns FICL_TRUE iff the given pointer is within the address range of 
 ** the dictionary.
 **************************************************************************/
int ficlDictionaryIncludes(ficlDictionary *dict, void *p)
{
  return (dict != NULL && p != NULL && ((p >= (void *)&dict->base) && (p < (void *)(&dict->base + dict->size))));
}

/**************************************************************************
 **                      d i c t L o o k u p
 ** Find the ficlWord that matches the given name and length.
 ** If found, returns the word's address. Otherwise returns NULL.
 ** Uses the search order list to search multiple wordlists.
 **************************************************************************/
ficlWord *ficlDictionaryLookup(ficlDictionary *dict, ficlString name)
{
  ficlWord *word = NULL;
  ficlHash *hash;
  int i;
  ficlUnsigned hashCode;

  FICL_ASSERT(dict != NULL);
  hashCode = ficlHashCode(name);

  for (i = (int)dict->wordlistCount - 1; (i >= 0) && (word == NULL); --i)
  {
    hash = dict->wordlists[i];
    word = ficlHashLookup(hash, name, hashCode);
  }
  return word;
}

/**************************************************************************
 **                        s e e 
 ** TOOLS ( "<spaces>name" -- )
 ** Display a human-readable representation of the named word's definition.
 ** The source of the representation (object-code decompilation, source
 ** block, etc.) and the particular form of the display is implementation
 ** defined. 
 **************************************************************************/
/*
** ficlSeeColon (for proctologists only)
** Walks a colon definition, decompiling
** on the fly. Knows about primitive control structures.
*/
char *ficlDictionaryInstructionNames[] =
{
#define FICL_TOKEN(token, description) description,
#define FICL_INSTRUCTION_TOKEN(token, description, flags) description,
#include "ficltokens.h"
#undef FICL_TOKEN
#undef FICL_INSTRUCTION_TOKEN
};

void ficlDictionarySee(ficlDictionary *dict, ficlWord *wrd)
{
  ficlCell *cell = wrd->param;
  ficlCell *param0 = cell;

  for (; CELL_INT_REF(cell) != ficlInstructionSemiParen; cell++)
  {
    ficlWord *word = CELL_VOIDP_REF(cell);

    fth_printf("%3d  ", (int)(cell - param0));
        
    if (ficlDictionaryIsAWord(dict, word))
    {
      ficlWordKind kind = ficlWordClassify(word);
      ficlCell c, c2;

      switch (kind)
      {
      case FICL_WORDKIND_INSTRUCTION:
	fth_printf("%s (instruction %ld)",
		   ficlDictionaryInstructionNames[(long)word], (long)word);
	break;
      case FICL_WORDKIND_INSTRUCTION_WITH_ARGUMENT:
	c = *++cell;
	fth_printf("%s (instruction %ld), with argument %ld (%p)",
		   ficlDictionaryInstructionNames[(long)word], (long)word,
		   CELL_INT_REF(&c), CELL_VOIDP_REF(&c));
	break;
      case FICL_WORDKIND_INSTRUCTION_WORD:
	fth_printf("%s :: executes %s (instruction word %ld)",
		   word->name,
		   ficlDictionaryInstructionNames[(long)word->code],
		   (long)word->code);
	break;
      case FICL_WORDKIND_LITERAL:
	c = *++cell;
	if (ficlDictionaryIsAWord(dict, CELL_VOIDP_REF(&c))
	    && (CELL_INT_REF(&c) >= ficlInstructionLast))
	{
	  ficlWord *w = (ficlWord *)CELL_VOIDP_REF(&c);

	  fth_printf("%.*s (%p literal)", w->length, w->name, CELL_VOIDP_REF(&c));
	}
	else
	  fth_printf("literal %ld (%p)", CELL_INT_REF(&c), CELL_VOIDP_REF(&c));
	break;
      case FICL_WORDKIND_2LITERAL:
	c = *++cell;
	c2 = *++cell;
	fth_printf("2literal %ld %ld (%p %p)",
		   CELL_INT_REF(&c2), CELL_INT_REF(&c),
		   CELL_VOIDP_REF(&c2), CELL_VOIDP_REF(&c));
	break;
      case FICL_WORDKIND_STRING_LITERAL:
      {
	ficlCountedString *counted = (ficlCountedString *)(void *)++cell;

	cell = (ficlCell *)ficlAlignPointer(counted->text + counted->length + 1) - 1;
	fth_printf("s\" %.*s\"", counted->length, counted->text);
      }
      break;
      case FICL_WORDKIND_CSTRING_LITERAL:
      {
	ficlCountedString *counted = (ficlCountedString *)(void *)++cell;

	cell = (ficlCell *)ficlAlignPointer(counted->text + counted->length + 1) - 1;
	fth_printf("c\" %.*s\"", counted->length, counted->text);
      }
      break;
      case FICL_WORDKIND_BRANCH0:
	c = *++cell;
	fth_printf("branch0 %d", (int)(cell + CELL_INT_REF(&c) - param0));
	break;                                                           
      case FICL_WORDKIND_BRANCH:
	c = *++cell;
	fth_printf("branch %d", (int)(cell + CELL_INT_REF(&c) - param0));
	break;
      case FICL_WORDKIND_QDO:
	c = *++cell;
	fth_printf("?do (leave %d)", (int)((ficlCell *)CELL_VOIDP_REF(&c) - param0));
	break;
      case FICL_WORDKIND_DO:
	c = *++cell;
	fth_printf("do (leave %d)", (int)((ficlCell *)CELL_VOIDP_REF(&c) - param0));
	break;
      case FICL_WORDKIND_LOOP:
	c = *++cell;
	fth_printf("loop (branch %d)", (int)( cell + CELL_INT_REF(&c) - param0));
	break;
      case FICL_WORDKIND_OF:
	c = *++cell;
	fth_printf("of (branch %d)", (int)(cell + CELL_INT_REF(&c) - param0));
	break;
      case FICL_WORDKIND_PLOOP:
	c = *++cell;
	fth_printf("+loop (branch %d)", (int)(cell + CELL_INT_REF(&c) - param0));
	break;
      default:
	if (FICL_WORD_P(word))
	  fth_printf("%.*s", word->length, word->name);
	else
	  fth_printf("%S", ficl_to_fth(CELL_FTH_REF(cell)));
	break;
      }
    }
    else /* probably not a word - punt and print value */
      fth_printf("%S", CELL_FTH_REF(cell));
    fth_print("\n");
  }
  fth_print(";");
}

/**************************************************************************
 **                  d i c t R e s e t S e a r c h O r d e r
 ** Initialize the dictionary search order list to sane state
 **************************************************************************/
void ficlDictionaryResetSearchOrder(ficlDictionary *dict)
{
  FICL_ASSERT(dict != NULL);
  dict->compilationWordlist = dict->forthWordlist;
  dict->wordlistCount = 1;
  dict->wordlists[0] = dict->forthWordlist;
}

/**************************************************************************
 **                      d i c t S e t F l a g s
 ** Changes the flags field of the most recently defined word:
 ** Set all bits that are ones in the set parameter.
 **************************************************************************/
void ficlDictionarySetFlags(ficlDictionary *dict, ficlUnsigned set)
{
  FICL_ASSERT(dict->smudge != NULL);
  dict->smudge->flags |= set;
}

/**************************************************************************
 **                      d i c t C l e a r F l a g s
 ** Changes the flags field of the most recently defined word:
 ** Clear all bits that are ones in the clear parameter.
 **************************************************************************/
void ficlDictionaryClearFlags(ficlDictionary *dict, ficlUnsigned clear)
{
  FICL_ASSERT(dict->smudge != NULL);
  dict->smudge->flags &= ~clear;
}

/**************************************************************************
 **                      d i c t S e t I m m e d i a t e 
 ** Set the most recently defined word as IMMEDIATE
 **************************************************************************/
void ficlDictionarySetImmediate(ficlDictionary *dict)
{
  FICL_ASSERT(dict->smudge != NULL);
  dict->smudge->flags |= FICL_WORD_IMMEDIATE;
}

/**************************************************************************
 **                      d i c t U n s m u d g e 
 ** Completes the definition of a word by linking it
 ** into the main list
 **************************************************************************/
void ficlDictionaryUnsmudge(ficlDictionary *dict)
{
  ficlWord *word;
  ficlHash *hash;

  FICL_ASSERT(dict != NULL);
  word = dict->smudge;
  hash = dict->compilationWordlist;

  FICL_ASSERT(hash != NULL);
  FICL_ASSERT(word != NULL);

  /*
  ** :noname words never get linked into the list...
  */
  if (word->length > 0) {
  /* [ms] ficlHashInsertWord is only called here.
   * ficlHashInsertWord(hash, word);
   */
    ficlWord **pList;

    if (hash->size == 1)
      pList = hash->table;
    else
      pList = hash->table + (word->hash % hash->size);

    word->link = *pList;
    *pList = word;
  }
  word->flags &= ~(FICL_WORD_SMUDGED);
}

/**************************************************************************
 **                      d i c t W h e r e
 ** Returns the value of the HERE pointer -- the address
 ** of the next free ficlCell in the dictionary
 **************************************************************************/
ficlCell *ficlDictionaryWhere(ficlDictionary *dict)
{
  return dict->here;
}
