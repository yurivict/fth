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
 * @(#)word.c	1.13 10/17/13
 */

#include "ficl.h"

/**************************************************************************
 **                      w o r d I s I m m e d i a t e
 ** 
 **************************************************************************/
int ficlWordIsImmediate(ficlWord *word)
{
  return ((word != NULL) && (word->flags & FICL_WORD_IMMEDIATE));
}

/**************************************************************************
 **                      w o r d I s C o m p i l e O n l y
 ** 
 **************************************************************************/
int ficlWordIsCompileOnly(ficlWord *word)
{
  return ((word != NULL) && (word->flags & FICL_WORD_COMPILE_ONLY));
}

/**************************************************************************
 **                      f i c l W o r d C l a s s i f y
 ** This public function helps to classify word types for SEE
 ** and the debugger in tools.c. Given an pointer to a word, it returns
 ** a member of WOR
 **************************************************************************/
ficlWordKind ficlWordClassify(ficlWord *word)
{
  ficlPrimitive code = 0;
  long i = 0;
  ficlWordKind iType = FICL_WORDKIND_PRIMITIVE;

  if ((((ficlInstruction)word) > ficlInstructionInvalid) &&
      (((ficlInstruction)word) < ficlInstructionLast))
  {
    i = (long)word;
    iType = FICL_WORDKIND_INSTRUCTION;
    goto IS_INSTRUCTION;
  }

  if (word)
    code = word->code;

  if ((ficlInstruction)code < ficlInstructionLast)
  {
    i = (long)code;
    iType = FICL_WORDKIND_INSTRUCTION_WORD;
    goto IS_INSTRUCTION;
  }

  return iType;

IS_INSTRUCTION:

  switch (i)
  {
  case ficlInstructionConstantParen:
    return FICL_WORDKIND_CONSTANT;
  case ficlInstructionToLocalParen:
    return FICL_WORDKIND_INSTRUCTION_WITH_ARGUMENT;
  case ficlInstructionUserParen:
    return FICL_WORDKIND_USER;
  case ficlInstructionCreateParen:
    return FICL_WORDKIND_CREATE;
  case ficlInstructionCStringLiteralParen:
    return FICL_WORDKIND_CSTRING_LITERAL;
  case ficlInstructionStringLiteralParen:
    return FICL_WORDKIND_STRING_LITERAL;
  case ficlInstructionColonParen:
    return FICL_WORDKIND_COLON;
  case ficlInstructionDoDoes:
    return FICL_WORDKIND_DOES;
  case ficlInstructionDoParen:
    return FICL_WORDKIND_DO;
  case ficlInstructionQDoParen:
    return FICL_WORDKIND_QDO;
  case ficlInstructionVariableParen:
    return FICL_WORDKIND_VARIABLE;
  case ficlInstructionBranchParenWithCheck:
  case ficlInstructionBranchParen:
    return FICL_WORDKIND_BRANCH;
  case ficlInstructionBranch0ParenWithCheck:
  case ficlInstructionBranch0Paren:
    return FICL_WORDKIND_BRANCH0;
  case ficlInstructionLiteralParen:
    return FICL_WORDKIND_LITERAL;
  case ficlInstructionLoopParen:
    return FICL_WORDKIND_LOOP;
  case ficlInstructionOfParen:
    return FICL_WORDKIND_OF;
  case ficlInstructionPlusLoopParen:
    return FICL_WORDKIND_PLOOP;
  default:
    return iType;
  }
}
