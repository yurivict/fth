/*
 * Adapted to work with FTH:
 *
 * Copyright (c) 2004-2012 Michael Scholz <mi-scholz@users.sourceforge.net>
 *
 * This file is part of FTH.
 * 
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
