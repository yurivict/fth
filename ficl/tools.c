/*******************************************************************
** t o o l s . c
** Forth Inspired Command Language - programming tools
** Author: John Sadler (john_sadler@alum.mit.edu)
** Created: 20 June 2000
** $Id: tools.c,v 1.12 2010/08/12 13:57:22 asau Exp $
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
 * Copyright (c) 2004-2016 Michael Scholz <mi-scholz@users.sourceforge.net>
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
 * @(#)tools.c	1.55 3/22/16
 */

/*
** NOTES:
** SEE needs information about the addresses of functions that
** are the CFAs of colon definitions, constants, variables, DOES>
** words, and so on. It gets this information from a table and supporting
** functions in words.c.
** fiColonParen fiDoDoes createParen fiVariableParen fiUserParen fiConstantParen
**
** Step and break debugger for Ficl
** debug  ( xt -- )   Start debugging an xt
** Set a breakpoint
** Specify breakpoint default action
*/

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>          /* sprintf */
#include <string.h>
#include <ctype.h>
#include "ficl.h"

#include "fth.h"
#include "utils.h"

static void ficlPrimitiveStepIn(ficlVm *vm);
static void ficlPrimitiveStepOver(ficlVm *vm);
static void ficlPrimitiveStepBreak(ficlVm *vm);

void ficlCallbackAssert(int expression, char *exp_str, char *fname, int line)
{
  if (!expression)
  {
    fprintf(stderr, "#<assertion failed at %s:%d: \"%s\">\n", fname, line, exp_str);
    abort();
  }
}

/**************************************************************************
 **                      v m S e t B r e a k
 ** Set a breakpoint at the current value of IP by
 ** storing that address in a BREAKPOINT record
 **************************************************************************/
static void ficlVmSetBreak(ficlVm *vm, ficlBreakpoint *pBP)
{
  ficlWord *pStep = ficlSystemLookup(vm->callback.system, "step-break");
  FICL_ASSERT(pStep != NULL);

  pBP->address = vm->ip;
  pBP->oldXT = *vm->ip;
  *vm->ip = pStep;
}

/**************************************************************************
 **                      d e b u g P r o m p t
 **************************************************************************/
/* ARGSUSED */
static void ficlDebugPrompt(ficlVm *vm)
{
	(void)vm;
	fth_print("dbg> ");
}

/**************************************************************************
 **                        d i c t H a s h S u m m a r y
 ** Calculate a figure of merit for the dictionary hash table based
 ** on the average search depth for all the words in the dictionary,
 ** assuming uniform distribution of target keys. The figure of merit
 ** is the ratio of the total search depth for all keys in the table
 ** versus a theoretical optimum that would be achieved if the keys
 ** were distributed into the table as evenly as possible. 
 ** The figure would be worse if the hash table used an open
 ** addressing scheme (i.e. collisions resolved by searching the
 ** table for an empty slot) for a given size table.
 **************************************************************************/
static void ficlPrimitiveHashSummary(ficlVm *vm)
{
#define h_ficlPrimitiveHashSummary "( -- )  \
Calculate a figure of merit for the dictionary hash table based \
on the average search depth for all the words in the dictionary, \
assuming uniform distribution of target keys.  \
The figure of merit is the ratio of the total search depth \
for all keys in the table versus a theoretical optimum that would be achieved if the keys \
were distributed into the table as evenly as possible.  \
The figure would be worse if the hash table used an open addressing \
scheme (i.e. collisions resolved by searching the table for an empty slot) for a given size table."
  ficlDictionary *dictionary = ficlVmGetDictionary(vm);
  ficlHash *pFHash;
  ficlWord **hash;
  unsigned int i, size, nFilled, nWords = 0;
  ficlWord *word;
  int nMax = 0;
  double avg = 0.0;
  double best;
  unsigned int nAvg, nRem, nDepth;

  ficlVmDictionaryCheck(dictionary, 0);
  pFHash  = dictionary->wordlists[dictionary->wordlistCount - 1];
  hash    = pFHash->table;
  size    = pFHash->size;
  nFilled = size;

  for (i = 0; i < size; i++)
  {
    int n = 0;
    word = hash[i];

    while (word)
    {
      ++n;
      ++nWords;
      word = word->link;
    }

    avg += (double)(n * (n+1)) / 2.0;

    if (n > nMax)
      nMax = n;
    if (n == 0)
      --nFilled;
  }

  /* Calc actual avg search depth for this hash */
  avg = avg / nWords;

  /* [ms] silence scan-build's ccc-analyzer (Division by zero) */
  size = FICL_MAX(size, 1);

  /* Calc best possible performance with this size hash */
  nAvg = nWords / size;
  nRem = nWords % size;
  nDepth = size * (nAvg * (nAvg + 1)) / 2 + (nAvg + 1) * nRem;
  best = (double)nDepth/nWords;

  fth_printf("%d bins, %2.0f%% filled, Depth: Max=%d, Avg=%2.1f, Best=%2.1f, Score: %2.0f%%\n", 
	     size,
	     (double)nFilled * 100.0 / size, nMax,
	     avg, 
	     best,
	     100.0 * best / avg);
}

/*
** Here's the outer part of the decompiler. It's 
** just a big nested conditional that checks the
** CFA of the word to decompile for each kind of
** known word-builder code, and tries to do 
** something appropriate. If the CFA is not recognized,
** just indicate that it is a primitive.
*/
static void ficlPrimitiveSeeXT(ficlVm *vm)
{
#define h_ficlPrimitiveSeeXT "( xt -- )  \
Here's the outer part of the decompiler.  \
It's just a big nested conditional that checks the CFA of the word \
to decompile for each kind of known word-builder code, and tries to do something appropriate.  \
If the CFA is not recognized, just indicate that it is a primitive."
  ficlWord *word;
  ficlWordKind kind;

  word = (ficlWord *)ficlStackPopPointer(vm->dataStack);
  kind = ficlWordClassify(word);

  switch (kind)
  {
  case FICL_WORDKIND_COLON:
    fth_printf(": %.*s", word->length, word->name);
    ficlDictionarySee(ficlVmGetDictionary(vm), word);
    break;

  case FICL_WORDKIND_DOES:
    fth_print("does>");
    ficlDictionarySee(ficlVmGetDictionary(vm), CELL_VOIDP_REF(word->param));
    break;

  case FICL_WORDKIND_CREATE:
    fth_print("create");
    break;

  case FICL_WORDKIND_VARIABLE:
    fth_printf("variable = %S", ficl_to_fth(CELL_FTH_REF(word->param)));
    break;

  case FICL_WORDKIND_USER:
    fth_printf("user variable = %S", ficl_to_fth(CELL_FTH_REF(word->param)));
    break;

  case FICL_WORDKIND_CONSTANT:
    fth_printf("value = %S", ficl_to_fth(CELL_FTH_REF(word->param)));
    break;

  case FICL_WORDKIND_2CONSTANT:
    fth_printf("constant = %ld %ld (%p %p)",
	       CELL_INT_REF(&word->param[1]), CELL_INT_REF(word->param),
	       CELL_VOIDP_REF(&word->param[1]), CELL_VOIDP_REF(word->param));
    break;

  default:
    fth_printf("%s is a primitive", FICL_WORD_NAME(word));
    break;
  }

  if (word->flags & FICL_WORD_IMMEDIATE)
    fth_print(" immediate");

  if (word->flags & FICL_WORD_COMPILE_ONLY)
    fth_print(" compile-only");
  if (CELL_INT_REF(&vm->sourceId) != 0) fth_print("\n");
}

static void ficlPrimitiveSee(ficlVm *vm)
{
#define h_ficlPrimitiveSee "( \"name\" -- )  Decompiles word named NAME."
  ficlPrimitiveTick(vm);
  ficlPrimitiveSeeXT(vm);
}

/**************************************************************************
 **                      f i c l D e b u g X T
 ** debug  ( xt -- )
 ** Given an xt of a colon definition or a word defined by DOES>, set the
 ** VM up to debug the word: push IP, set the xt as the next thing to execute,
 ** set a breakpoint at its first instruction, and run to the breakpoint.
 ** Note: the semantics of this word are equivalent to "step in"
 **************************************************************************/
static void ficlPrimitiveDebugXT(ficlVm *vm)
{
#define h_ficlPrimitiveDebugXT "( xt -- )  \
Given an XT of a colon definition or a word defined by DOES>, set the \
VM up to debug the word: push IP, set the xt as the next thing to execute, \
set a breakpoint at its first instruction, and run to the breakpoint.\n\
Note: the semantics of this word are equivalent to \"step in\"."
  ficlWord *xt    = ficlStackPopPointer(vm->dataStack);
  ficlWordKind   wk    = ficlWordClassify(xt);

  ficlStackPushPointer(vm->dataStack, xt);
  ficlPrimitiveSeeXT(vm);

  switch (wk)
  {
  case FICL_WORDKIND_COLON:
  case FICL_WORDKIND_DOES:
    /*
    ** Run the colon code and set a breakpoint at the next instruction
    */
    ficlVmExecuteWord(vm, xt);
    ficlVmSetBreak(vm, &(vm->callback.system->breakpoint));
    break;

  default:
    ficlVmExecuteWord(vm, xt);
    break;
  }
}

/**************************************************************************
 **                      s t e p I n
 ** Ficl 
 ** Execute the next instruction, stepping into it if it's a colon definition 
 ** or a does> word. This is the easy kind of step.
 **************************************************************************/
static void ficlPrimitiveStepIn(ficlVm *vm)
{
  /*
  ** Do one step of the inner loop
  */
  ficlVmExecuteWord(vm, *vm->ip++);

  /*
  ** Now set a breakpoint at the next instruction
  */
  ficlVmSetBreak(vm, &(vm->callback.system->breakpoint));
}

/**************************************************************************
 **                        s t e p O v e r
 ** Ficl 
 ** Execute the next instruction atomically. This requires some insight into 
 ** the memory layout of compiled code. Set a breakpoint at the next instruction
 ** in this word, and run until we hit it
 **************************************************************************/
static void ficlPrimitiveStepOver(ficlVm *vm)
{
  ficlWord *word;
  ficlWordKind kind;
  ficlWord *pStep = ficlSystemLookup(vm->callback.system, "step-break");
  FICL_ASSERT(pStep != NULL);

  word = *vm->ip;
  kind = ficlWordClassify(word);

  switch (kind)
  {
  case FICL_WORDKIND_COLON: 
  case FICL_WORDKIND_DOES:
    /*
    ** assume that the next ficlCell holds an instruction 
    ** set a breakpoint there and return to the inner interpreter
    */
    vm->callback.system->breakpoint.address = vm->ip + 1;
    vm->callback.system->breakpoint.oldXT =  vm->ip[1];
    vm->ip[1] = pStep;
    break;

  default:
    ficlPrimitiveStepIn(vm);
    break;
  }
}

/**************************************************************************
 **                        s t e p - b r e a k
 ** Ficl
 ** Handles breakpoints for stepped execution.
 ** Upon entry, breakpoint contains the address and replaced instruction
 ** of the current breakpoint.
 ** Clear the breakpoint
 ** Get a command from the console. 
 ** i (step in) - execute the current instruction and set a new breakpoint 
 **    at the IP
 ** o (step over) - execute the current instruction to completion and set
 **    a new breakpoint at the IP
 ** g (go) - execute the current instruction and exit
 ** q (quit) - abort current word
 ** b (toggle breakpoint)
 **************************************************************************/
extern char *ficlDictionaryInstructionNames[];

static void ficlPrimitiveStepBreak(ficlVm *vm)
{
#define h_ficlPrimitiveStepBreak "( -- )  \
Handles breakpoints for stepped execution.  \
Upon entry, breakpoint contains the address and replaced instruction of the current breakpoint.  \
Clear the breakpoint, get a command from the console.\n\
i (step in) - execute the current instruction and set a new breakpoint at the IP\n\
o (step over) - execute the current instruction to completion and set a new breakpoint at the IP\n\
g (go) - execute the current instruction and exit\n\
q (quit) - abort current word\n\
b (toggle breakpoint)"
  ficlString command;
  ficlWord *word;
  ficlWord *pOnStep;
  ficlWordKind kind;

  if (!vm->restart)
  {
    FICL_ASSERT(vm->callback.system->breakpoint.address != NULL);
    FICL_ASSERT(vm->callback.system->breakpoint.oldXT != NULL);
    /*
    ** Clear the breakpoint that caused me to run
    ** Restore the original instruction at the breakpoint, 
    ** and restore the IP
    */
    vm->ip = (ficlIp)(vm->callback.system->breakpoint.address);
    *vm->ip = vm->callback.system->breakpoint.oldXT;

    /*
    ** If there's an onStep, do it
    */
    pOnStep = ficlSystemLookup(vm->callback.system, "on-step");
    if (pOnStep)
      ficlVmExecuteXT(vm, pOnStep);

    /*
    ** Print the name of the next instruction
    */
    word = vm->callback.system->breakpoint.oldXT;

    kind = ficlWordClassify(word);

    switch (kind)
    {
    case FICL_WORDKIND_INSTRUCTION:
    case FICL_WORDKIND_INSTRUCTION_WITH_ARGUMENT:
      fth_printf("next: %s (instruction %ld)\n",
		 ficlDictionaryInstructionNames[(long)word], (long)word);
      break;
    default:
      if (ficlDictionaryIncludes(ficlVmGetDictionary(vm), word))
	fth_printf("next: %s\n", word->name);
      else
	fth_printf("next: %ld (%p)\n", (long)word, word);
      break;
    }
    ficlDebugPrompt(vm);
  }
  else
  {
    vm->restart = 0;
  }

  command = ficlVmGetWord(vm);

  switch (command.text[0])
  {
  case 'i':
    ficlPrimitiveStepIn(vm);
    break;

  case 'o':
    ficlPrimitiveStepOver(vm);
    break;

  case 'g':
    break;

  case 'l':
  {
    ficlWord *xt;
    xt = ficlDictionaryFindEnclosingWord(ficlVmGetDictionary(vm), (ficlCell *)(vm->ip));
    if (xt)
    {
      ficlStackPushPointer(vm->dataStack, xt);
      ficlPrimitiveSeeXT(vm);
    }
    else
      fth_print("sorry, can't do that\n");
    ficlVmThrow(vm, FICL_VM_STATUS_RESTART);
    break;
  }

  case 'q':
  {
    ficlVmThrow(vm, FICL_VM_STATUS_ABORT);
    break;
  }

  case 'x':
  {
    /*
    ** Take whatever's left in the TIB and feed it to a subordinate ficlVmExecuteString
    */ 
    int returnValue;
    ficlString s;
    ficlWord *oldRunningWord = vm->runningWord;

    FICL_STRING_SET_POINTER(s, vm->tib.text + vm->tib.index);
    FICL_STRING_SET_LENGTH(s, vm->tib.end - FICL_STRING_GET_POINTER(s));

    returnValue = ficlVmExecuteString(vm, s);

    if (returnValue == FICL_VM_STATUS_OUT_OF_TEXT)
    {
      returnValue = FICL_VM_STATUS_RESTART;
      vm->runningWord = oldRunningWord;
      fth_print("\n");
    }

    ficlVmThrow(vm, returnValue);
    break;
  }

  default:
  {
    fth_print("i -- step In\n"
	      "o -- step Over\n"
	      "g -- Go (execute to completion)\n"
	      "l -- List source code\n"
	      "q -- Quit (stop debugging and abort)\n"
	      "x -- eXecute the rest of the line as Ficl words\n");
    ficlDebugPrompt(vm);
    ficlVmThrow(vm, FICL_VM_STATUS_RESTART);
    break;
  }
  }
}

/**************************************************************************
 **                      b y e
 ** TOOLS
 ** Signal the system to shut down - this causes ficlExec to return
 ** VM_USEREXIT. The rest is up to you.
 **************************************************************************/
static void ficlPrimitiveBye(ficlVm *vm)
{
#define h_ficlPrimitiveBye "( -- )  \
Signal the system to shut down - this causes ficlVmExecute to return FICL_VM_STATUS_USER_EXIT.  \
The rest is up to you."
  ficlVmThrow(vm, FICL_VM_STATUS_USER_EXIT);
}

static void
ficlPrimitiveSkipFile(ficlVm *vm)
{
#define h_ficlPrimitiveSkipFile "( -- )  \
Stops loading of current file."
  ficlVmThrow(vm, FICL_VM_STATUS_SKIP_FILE);
}

/**************************************************************************
 **                      d i s p l a y S t a c k
 ** TOOLS 
 ** Display the parameter stack (code for ".s")
 **************************************************************************/
struct stackContext
{
  ficlVm *vm;
  ficlDictionary *dictionary;
  int count;
};

static ficlInteger ficlStackDisplayCallback(void *c, ficlCell *cell)
{
  struct stackContext *context = (struct stackContext *)c;
  fth_printf("[%p %3d]: %12ld (0x%08lx)\n",
	     cell, context->count++, CELL_INT_REF(cell), CELL_INT_REF(cell));
  return FICL_TRUE;
}

void ficlStackDisplay(ficlStack *stack, ficlStackWalkFunction cb, void *context)
{
  ficlVm *vm = stack->vm;
  struct stackContext myContext;

  FICL_STACK_CHECK(stack, 0, 0);
  fth_printf("[%s stack has %d entries, top at %p]\n",
	     stack->name, ficlStackDepth(stack), stack->top);

  if (cb == NULL)
  {
    myContext.vm = vm;
    myContext.count = 0;
    context = &myContext;
    cb = ficlStackDisplayCallback;
  }
  ficlStackWalk(stack, cb, context, (ficlInteger)FICL_FALSE);
  fth_printf("[%s stack base at %p]\n", stack->name, stack->base);
}

void ficlVmDisplayDataStack(ficlVm *vm)
{
#define h_ficlVmDisplayDataStack "( -- )  Display the parameter stack."
  ficlStackDisplay(vm->dataStack, NULL, NULL);
}

static ficlInteger ficlStackDisplaySimpleCallback(void *c, ficlCell *cell)
{
  struct stackContext *context = (struct stackContext *)c;

  if (fth_instance_p(CELL_FTH_REF(cell)))
    fth_printf("%M ", CELL_FTH_REF(cell));
  else
    fth_printf("%ld ", CELL_INT_REF(cell));
  context->count++;
  return FICL_TRUE;
}

void ficlVmDisplayDataStackSimple(ficlVm *vm)
{
#define h_ficlVmDisplayDataStackSimple "( -- )  Display the parameter stack."
  ficlStack *stack = vm->dataStack;
  struct stackContext context;
  int depth = 0;

  FICL_STACK_CHECK(stack, 0, 0);

  if ((depth = ficlStackDepth(stack)) == 0)
    fth_print("#<data stack empty>");
  else
    fth_printf("<%d> ", depth);

  context.vm = vm;
  context.count = 0;
  ficlStackWalk(stack, ficlStackDisplaySimpleCallback,
      &context, (ficlInteger)FICL_TRUE);
}

static ficlInteger ficlReturnStackDisplayCallback(void *c, ficlCell *cell)
{
  struct stackContext *context = (struct stackContext *)c;

  fth_printf("[%p %3d]: %12ld (0x%08lx)\n",
	     cell, context->count++, CELL_INT_REF(cell), CELL_INT_REF(cell));

  /*
  ** Attempt to find the word that contains the return
  ** stack address (as if it is part of a colon definition).
  ** If this works, also print the name of the word.
  */
  if (ficlDictionaryIncludes(context->dictionary, CELL_VOIDP_REF(cell)))
  {
    ficlWord *word = ficlDictionaryFindEnclosingWord(context->dictionary, CELL_VOIDP_REF(cell));
    if (word)
    {
      ficlInteger offset = (ficlCell *)CELL_VOIDP_REF(cell) - &word->param[0];
      fth_printf(", %s + %ld ", word->name, offset);
    }
  }
  fth_printf("\n");
  return FICL_TRUE;
}

void ficlVmDisplayReturnStack(ficlVm *vm)
{
#define h_ficlVmDisplayReturnStack "( -- )  Display the return stack."
  struct stackContext context;
  context.vm = vm;
  context.count = 0;
  context.dictionary = ficlVmGetDictionary(vm);
  ficlStackDisplay(vm->returnStack, ficlReturnStackDisplayCallback, &context);
}

static void ficlVmDisplayReturnStackSimple(ficlVm *vm)
{
#define h_ficlVmDisplayReturnStackSimple "( -- )  Display the return stack."
  ficlStack *stack = vm->returnStack;
  struct stackContext context;
  int depth = 0;

  FICL_STACK_CHECK(stack, 0, 0);

  if ((depth = ficlStackDepth(stack)) == 0)
    fth_print("#<return stack empty>");
  else
    fth_printf("<%d> ", depth);
  context.vm = vm;
  context.count = 0;
  ficlStackWalk(stack, ficlStackDisplaySimpleCallback,
      &context, (ficlInteger)FICL_TRUE);
}

/**************************************************************************
 **                      f o r g e t - w i d
 ** 
 **************************************************************************/
static void ficlPrimitiveForgetWid(ficlVm *vm)
{
#define h_ficlPrimitiveForgetWid "( \"wid\" -- )  Forget WID."
  ficlDictionary *dictionary = ficlVmGetDictionary(vm);
  ficlHash *hash;

  hash = (ficlHash *)ficlStackPopPointer(vm->dataStack);
  ficlHashForget(hash, dictionary->here);
}

/**************************************************************************
 **                      f o r g e t
 ** TOOLS EXT  ( "<spaces>name" -- )
 ** Skip leading space delimiters. Parse name delimited by a space.
 ** Find name, then delete name from the dictionary along with all
 ** words added to the dictionary after name. An ambiguous
 ** condition exists if name cannot be found. 
 ** 
 ** If the Search-Order word set is present, FORGET searches the
 ** compilation word list. An ambiguous condition exists if the
 ** compilation word list is deleted. 
 **************************************************************************/
static void ficlPrimitiveForget(ficlVm *vm)
{
#define h_ficlPrimitiveForget "( \"<spaces>name\" -- )  \
Skip leading space delimiters. Parse name delimited by a space.  \
Find name, then delete name from the dictionary along with all \
words added to the dictionary after name.  \
An ambiguous condition exists if name cannot be found.  \
If the Search-Order word set is present, FORGET searches the compilation word list.  \
An ambiguous condition exists if the compilation word list is deleted."
  void *where;
  ficlDictionary *dictionary = ficlVmGetDictionary(vm);
  ficlHash *hash = dictionary->compilationWordlist;

  ficlPrimitiveTick(vm);
  where = ((ficlWord *)ficlStackPopPointer(vm->dataStack))->name;
  ficlHashForget(hash, where);
  dictionary->here = FICL_POINTER_TO_CELL(where);
}

/**************************************************************************
 **                        w o r d s
 ** 
 **************************************************************************/
#define nCOLWIDTH 8
static void ficlPrimitiveWords(ficlVm *vm)
{
#define h_ficlPrimitiveWords "( -- )  Prints all word of the dictionary."
  ficlDictionary *dictionary = ficlVmGetDictionary(vm);
  ficlHash *hash = dictionary->wordlists[dictionary->wordlistCount - 1];
  ficlWord *wp;
  int nChars = 0;
  int len;
  unsigned i;
  int nWords = 0;
  char *cp;
  char *pPad = vm->pad;

  for (i = 0; i < hash->size; i++)
  {
    for (wp = hash->table[i]; wp != NULL; wp = wp->link, nWords++)
    {
      if (wp->length == 0) /* ignore :noname defs */
	continue;

      cp = wp->name;
      nChars += snprintf(pPad + nChars, (size_t)(FICL_PAD_SIZE - nChars - 1), "%s", cp);

      if (nChars > 70)
      {
	pPad[nChars++] = '\n';
	pPad[nChars] = '\0';
	nChars = 0;
	fth_print(pPad);
      }
      else
      {
	len = nCOLWIDTH - nChars % nCOLWIDTH;
	while (len-- > 0)
	  pPad[nChars++] = ' ';
      }

      if (nChars > 70)
      {
	pPad[nChars++] = '\n';
	pPad[nChars] = '\0';
	nChars = 0;
	fth_print(pPad);
      }
    }
  }

  if (nChars > 0)
  {
    pPad[nChars++] = '\n';
    pPad[nChars] = '\0';
    fth_print(pPad);
  }

  fth_printf("Dictionary: %d words, %ld cells used of %u total\n",
	     nWords, (long)(dictionary->here - dictionary->base), (unsigned int)dictionary->size);
}

/**************************************************************************
 **                      l i s t E n v
 ** Print symbols defined in the environment 
 **************************************************************************/
static void ficlPrimitiveListEnv(ficlVm *vm)
{
#define h_ficlPrimitiveListEnv "( -- )  Print symbols defined in the environment."
  ficlDictionary *dictionary = vm->callback.system->environment;
  ficlHash *hash = dictionary->forthWordlist;
  ficlWord *word;
  unsigned i;
  int counter = 0;

  for (i = 0; i < hash->size; i++)
    for (word = hash->table[i]; word != NULL; word = word->link, counter++)
      fth_printf("%s\n", word->name);

  fth_printf("Environment: %d words, %ld cells used of %u total\n",
	     counter, (long)(dictionary->here - dictionary->base), (unsigned int)dictionary->size);
}

/*
** This word lists the parse steps in order
*/
static void ficlPrimitiveParseStepList(ficlVm *vm)
{
#define h_ficlPrimitiveParseStepList "( -- )  This word lists the parse steps in order."
  int i;
  ficlSystem *sys = vm->callback.system;
  FICL_ASSERT(sys != NULL);

  fth_print("Parse steps:\n");
  fth_print("lookup\n");

  for (i = 0; i < FICL_MAX_PARSE_STEPS; i++)
  {
    if (sys->parseList[i] != NULL)
      fth_printf("%s\n", sys->parseList[i]->name);
    else
      break;
  }
}

/**************************************************************************
 **                      e n v C o n s t a n t
 ** Ficl interface to ficlSystemSetEnvironment and ficlSetEnvD - allow Ficl code to set
 ** environment constants...
 **************************************************************************/
static void ficlPrimitiveEnvConstant(ficlVm *vm)
{
#define h_ficlPrimitiveEnvConstant "( -- )  \
Ficl interface to ficlSystemSetEnvironment and ficlSetEnvD---\
allow Ficl code to set environment constants."
  ficlUnsigned value;

  FICL_STACK_CHECK(vm->dataStack, 1, 0);
  ficlVmGetWordToPad(vm);
  value = ficlStackPopUnsigned(vm->dataStack);
  ficlDictionaryAppendConstant(ficlSystemGetEnvironment(vm->callback.system), vm->pad, (ficlInteger)value);
}

/**************************************************************************
 **                      f i c l C o m p i l e T o o l s
 ** Builds wordset for debugger and TOOLS optional word set
 **************************************************************************/
void ficlSystemCompileTools(ficlSystem *system)
{
  ficlDictionary *dict = ficlSystemGetDictionary(system);
  ficlDictionary *env = ficlSystemGetEnvironment(system);

  FICL_ASSERT(dict != NULL);
  FICL_ASSERT(env != NULL);

  /*
  ** TOOLS and TOOLS EXT
  */
  FICL_PRIM_DOC(dict, ".s",        ficlVmDisplayDataStackSimple);
  FICL_PRIM_DOC(dict, ".s-simple", ficlVmDisplayDataStackSimple);
  FICL_PRIM_DOC(dict, "bye",       ficlPrimitiveBye);
  FICL_PRIM_DOC(dict, "skip-file", ficlPrimitiveSkipFile);
  FICL_PRIM_DOC(dict, "forget",    ficlPrimitiveForget);
  FICL_PRIM_DOC(dict, "see",       ficlPrimitiveSee);
  FICL_PRIM_DOC(dict, "words",     ficlPrimitiveWords);

  /*
  ** Set TOOLS environment query values
  */
  ficlDictionaryAppendConstant(env, "tools", (ficlInteger)FICL_TRUE);
  /* [ms]
   * ficlDictionaryAppendConstant(env, "tools-ext", (ficlInteger)FICL_FALSE);
   */

  /*
  ** Ficl extras
  */
  FICL_PRIM_DOC(dict, "r.s",           ficlVmDisplayReturnStackSimple);
  FICL_PRIM_DOC(dict, "r.s-long",      ficlVmDisplayReturnStack);
  FICL_PRIM_DOC(dict, ".env",          ficlPrimitiveListEnv);
  FICL_PRIM_DOC(dict, "env-constant",  ficlPrimitiveEnvConstant);
  FICL_PRIM_DOC(dict, "debug-xt",      ficlPrimitiveDebugXT);
  FICL_PRIM_DOC(dict, "parse-order",   ficlPrimitiveParseStepList);
  FICL_PRIM_DOC(dict, "step-break",    ficlPrimitiveStepBreak);
  FICL_PRIM_DOC(dict, "forget-wid",    ficlPrimitiveForgetWid);
  FICL_PRIM_DOC(dict, "see-xt",        ficlPrimitiveSeeXT);
  FICL_PRIM_DOC(dict, ".hash-summary", ficlPrimitiveHashSummary);
}
