/*******************************************************************
** v m . c
** Forth Inspired Command Language - virtual machine methods
** Author: John Sadler (john_sadler@alum.mit.edu)
** Created: 19 July 1997
** $Id: vm.c,v 1.17 2010/09/13 18:43:04 asau Exp $
*******************************************************************/
/*
** This file implements the virtual machine of Ficl. Each virtual
** machine retains the state of an interpreter. A virtual machine
** owns a pair of stacks for parameters and return addresses, as
** well as a pile of state variables and the two dedicated registers
** of the interpreter.
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
 * @(#)vm.c	1.149 10/17/13
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "ficl.h"

#include "fth.h"
#include "utils.h"

#define FICL_VM_CHECK(vm) FICL_ASSERT((*(vm->ip - 1)) == vm->runningWord)
#define FICL_FW_CHECK(Check) if (!(Check)) return;

/**************************************************************************
 **                      v m B r a n c h R e l a t i v e 
 ** 
 **************************************************************************/
void ficlVmBranchRelative(ficlVm *vm, int offset)
{
  vm->ip += offset;
}

/**************************************************************************
 **                      v m C r e a t e
 ** Creates a virtual machine either from scratch (if vm is NULL on entry)
 ** or by resizing and reinitializing an existing VM to the specified stack
 ** sizes.
 **************************************************************************/
ficlVm *ficlVmCreate(ficlVm *vm, unsigned nPStack, unsigned nRStack)
{
  if (vm == NULL)
    vm = (ficlVm *)FTH_CALLOC(1, sizeof(ficlVm));
  else
  {
    FTH_FREE(vm->dataStack);
    FTH_FREE(vm->returnStack);
  }
  vm->dataStack   = ficlStackCreate(vm, "data",   nPStack);
  vm->returnStack = ficlStackCreate(vm, "return", nRStack);
  ficlVmReset(vm);
  return vm;
}

/**************************************************************************
 **                      v m D e l e t e
 ** Free all memory allocated to the specified VM and its subordinate 
 ** structures.
 **************************************************************************/
void ficlVmDestroy(ficlVm *vm)
{
  if (vm)
  {
    FTH_FREE(vm->dataStack);
    FTH_FREE(vm->returnStack);
    FTH_FREE(vm);
  }
}

/**************************************************************************
 **                      v m E x e c u t e
 ** Sets up the specified word to be run by the inner interpreter.
 ** Executes the word's code part immediately, but in the case of
 ** colon definition, the definition itself needs the inner interpreter
 ** to complete. This does not happen until control reaches ficlExec
 **************************************************************************/
/* ficlVmExecuteWord is an alias for ficlVmInnerLoop. */

static void ficlVmOptimizeJumpToJump(ficlVm *vm, ficlIp ip)
{
  ficlIp destination;

  switch ((ficlInstruction)(*ip))
  {
  case ficlInstructionBranchParenWithCheck:
    *ip = (ficlWord *)ficlInstructionBranchParen;
    goto RUNTIME_FIXUP;

  case ficlInstructionBranch0ParenWithCheck:
    *ip = (ficlWord *)ficlInstructionBranch0Paren;
  RUNTIME_FIXUP:
    ip++;
    destination = ip + *(long *)ip;
    switch ((ficlInstruction)*destination)
    {
    case ficlInstructionBranchParenWithCheck:
      /* preoptimize where we're jumping to */
      ficlVmOptimizeJumpToJump(vm, destination);
      /* fall through */
    case ficlInstructionBranchParen:
    {
      destination++;
      destination += *(long *)destination;
      *ip = (ficlWord *)(destination - ip);
      break;
    }
    default:
      break;
    }
    /* fall through */
  default:
    break;
  }
}

/**************************************************************************
 **                      v m I n n e r L o o p
 ** the mysterious inner interpreter...
 ** This loop is the address interpreter that makes colon definitions
 ** work. Upon entry, it assumes that the IP points to an entry in 
 ** a definition (the body of a colon word). It runs one word at a time
 ** until something does vmThrow. The catcher for this is expected to exist
 ** in the calling code.
 ** vmThrow gets you out of this loop with a longjmp()
 **************************************************************************/
#if 0
inline void ficlStackCheckNospill(ficlStack *stack, ficlCell *top, int popCells, int pushCells)
{
  /*
  ** Why save and restore stack->top?
  ** So the simple act of stack checking doesn't force a "register" spill,
  ** which might mask bugs (places where we needed to spill but didn't).
  ** --lch
  */
  ficlCell *oldTop = stack->top;

  stack->top = top;
  ficlStackCheck(stack, popCells, pushCells);
  stack->top = oldTop;
}
#endif

#define _CHECK_STACK(stack_vm, top_vm, in_vm, out_vm)	\
  do {							\
    ficlStack *__stack__ = (ficlStack *)(stack_vm);	\
    ficlCell *__old_top__ = __stack__->top;		\
							\
    __stack__->top = (top_vm);				\
    ficlStackCheck(__stack__, (in_vm), (out_vm));	\
    (stack_vm)->top = __old_top__;			\
  } while (0)

#define CHECK_STACK(pop, push)          _CHECK_STACK(vm->dataStack, dataTop, pop, push)
#define CHECK_RETURN_STACK(pop, push)   _CHECK_STACK(vm->returnStack, returnTop, pop, push)

/*
 * These are tests for raw C, i.e. do not pass the filter
 * ficl_to_fth().
 */
#define VM_FALSE_OR_NULL_P(Obj)						\
	((Obj) == FICL_FALSE || FTH_FALSE_P(Obj) || FTH_NIL_P(Obj))
#define VM_NOT_FALSE_P(Obj)	!VM_FALSE_OR_NULL_P(Obj)

#define FUNC_ARGS() do {						\
	ficlInteger idx;						\
	int all;							\
									\
	LOCAL_VARIABLE_SPILL();						\
	if (req > depth)						\
		fth_throw(FTH_WRONG_NUMBER_OF_ARGS,			\
		    "%s: not enough arguments, %d instead of %d",	\
		    RUNNING_WORD_VM(vm),				\
		    depth,						\
		    req);						\
	all = req + opt;						\
	if (rest) {							\
		FTH ls;							\
									\
		ls = FTH_NIL;						\
		if (FTH_NIL_P(STACK_FTH_REF(vm->dataStack)))		\
			ficlStackDrop(vm->dataStack, 1);		\
		else {							\
			ficlInteger size;				\
									\
			size = (ficlInteger)(depth - all);		\
			if (size > 0) {					\
				ls = fth_make_list_len(size);		\
									\
				for (idx = size - 1; idx >= 0; idx--)	\
					fth_array_fast_set(ls,		\
					    idx,			\
					    fth_pop_ficl_cell(vm));	\
			}						\
		}							\
		args[length - 1] = ls;					\
	}								\
	if (all > 0)							\
		for (idx = FICL_MIN(depth, all) - 1; idx >= 0; idx--)	\
			args[idx] = fth_pop_ficl_cell(vm);		\
	LOCAL_VARIABLE_REFILL();					\
} while (0)

#define LOCAL_VARIABLE_SPILL()			\
  do {						\
    vm->ip = (ficlIp)ip;			\
    vm->dataStack->top = dataTop;		\
    vm->returnStack->top = returnTop;		\
    vm->returnStack->frame = frame;		\
  } while (0)

#define LOCAL_VARIABLE_REFILL()			\
  do {						\
    ip = (ficlInstruction *)vm->ip;		\
    dataTop = vm->dataStack->top;		\
    returnTop = vm->returnStack->top;		\
    frame = vm->returnStack->frame;		\
  } while (0)

void ficlVmInnerLoop(ficlVm *vm, ficlWord *volatile fw)
{
  ficlInstruction *volatile ip = NULL;
  ficlCell *volatile dataTop;
  ficlCell *volatile returnTop;
  ficlCell *volatile frame;
  jmp_buf *volatile oldExceptionHandler;
  jmp_buf exceptionHandler;
  volatile int except;
  volatile int once;
  volatile int count = 0;
  volatile ficlInstruction instruction = 0;
  volatile ficlInteger i;
  volatile ficlUnsigned u;
  volatile ficlFloat df;
  volatile ficlCell c;
  ficlCountedString *volatile s;
  char *volatile cp;
  ficlCell *cell = NULL;

  if ((once = (fw != NULL)))
    count = 1;

  LOCAL_VARIABLE_REFILL();
  oldExceptionHandler = vm->exceptionHandler;
  vm->exceptionHandler = &exceptionHandler; /* This has to come before the setjmp! */
  except = setjmp(exceptionHandler);

  if (except)
  {
    LOCAL_VARIABLE_SPILL();
    vm->exceptionHandler = oldExceptionHandler;
    ficlVmThrow(vm, except);
  }

  for (;;)
  {
    if (once)
    {
      if (!count--)
	break;

      instruction = (ficlInstruction)((void *)fw);
    }
    else
    {
      if (ip)
      {
	instruction = *ip++;
	fw = (ficlWord *)instruction;
      }
      else
	ficlVmThrow(vm, FICL_VM_STATUS_QUIT);
    }
      
  AGAIN:
    switch (instruction)
    {
    case ficlInstructionInvalid:
      ficlVmThrowError(vm, "invalid instruction detected");
      return;

    case ficlInstruction1:
    case ficlInstruction2:
    case ficlInstruction3:
    case ficlInstruction4:
    case ficlInstruction5:
    case ficlInstruction6:
    case ficlInstruction7:
    case ficlInstruction8:
    case ficlInstruction9:
    case ficlInstruction10:
    case ficlInstruction11:
    case ficlInstruction12:
    case ficlInstruction13:
    case ficlInstruction14:
    case ficlInstruction15:
    case ficlInstruction16:
      CHECK_STACK(0, 1);
      ++dataTop;
      VM_STACK_INT_SET(dataTop, instruction);
      continue;

    case ficlInstruction0:
    case ficlInstructionNeg1:
    case ficlInstructionNeg2:
    case ficlInstructionNeg3:
    case ficlInstructionNeg4:
    case ficlInstructionNeg5:
    case ficlInstructionNeg6:
    case ficlInstructionNeg7:
    case ficlInstructionNeg8:
    case ficlInstructionNeg9:
    case ficlInstructionNeg10:
    case ficlInstructionNeg11:
    case ficlInstructionNeg12:
    case ficlInstructionNeg13:
    case ficlInstructionNeg14:
    case ficlInstructionNeg15:
    case ficlInstructionNeg16:
      CHECK_STACK(0, 1);
      ++dataTop;
      VM_STACK_INT_SET(dataTop, ficlInstruction0 - instruction);
      continue;

      /**************************************************************************
       ** stringlit: Fetch the count from the dictionary, then push the address
       ** and count on the stack. Finally, update ip to point to the first
       ** aligned address after the string text.
       **************************************************************************/
    case ficlInstructionStringLiteralParen:
    {
      ficlUnsigned length;

      CHECK_STACK(0, 2);
      s = (ficlCountedString *)(ip);

      if (s)
      {
	length = s->length;
	cp = s->text;
      }
      else
      {
	length = 0;
	cp = "";
      }
      ++dataTop;
      VM_STACK_VOIDP_SET(dataTop, cp);
      ++dataTop;
      VM_STACK_UINT_SET(dataTop, length);
      cp += length + 1;
      cp = ficlAlignPointer(cp);
      ip = (void *)cp;
      continue;
    }

    case ficlInstructionCStringLiteralParen:
      CHECK_STACK(0, 1);
      s = (ficlCountedString *)(ip);

      if (s)
	cp = s->text + s->length + 1;
      else
	cp = "";
      cp = ficlAlignPointer(cp);
      ip = (void *)cp;
      ++dataTop;
      VM_STACK_VOIDP_SET(dataTop, s);
      continue;

#define PUSH_CELL_POINTER(cp)        cell = (cp); *++dataTop = *cell; continue
#define POP_CELL_POINTER(cp)         cell = (cp); *cell = *dataTop--; continue
      
#define BRANCH()                     if (ip) ip += *(long *)ip; continue
#define EXIT_FUNCTION() ip = (ficlInstruction *)(VM_STACK_VOIDP_REF(returnTop)); returnTop--; continue

      /**************************************************************************
       ** This is the runtime for (literal). It assumes that it is part of a colon
       ** definition, and that the next ficlCell contains a value to be pushed on the
       ** parameter stack at runtime. This code is compiled by "literal".
       **************************************************************************/
    case ficlInstructionLiteralParen:
      CHECK_STACK(0, 1);
      ++dataTop;
      VM_STACK_INT_SET(dataTop, *ip);
      ip++;
      continue;

      /**************************************************************************
       ** Link a frame on the return stack, reserving nCells of space for
       ** locals - the value of nCells is the next ficlCell in the instruction
       ** stream.
       ** 1) Push frame onto returnTop
       ** 2) frame = returnTop
       ** 3) returnTop += nCells
       **************************************************************************/
    case ficlInstructionLinkParen:
    {
      ficlInteger nCells = *ip++;

      ++returnTop;
      VM_STACK_VOIDP_SET(returnTop, frame);
      frame = returnTop + 1;
      returnTop += nCells;
      continue;
    }

    /**************************************************************************
     ** Unlink a stack frame previously created by stackLink
     ** 1) dataTop = frame
     ** 2) frame = pop()
     *******************************************************************/
    case ficlInstructionUnlinkParen:
      returnTop = frame - 1;
      frame = VM_STACK_VOIDP_REF(returnTop);
      returnTop--;
      continue;

      /**************************************************************************
       ** Immediate - cfa of a local while compiling - when executed, compiles
       ** code to fetch the value of a local given the local's index in the
       ** word's pfa
       **************************************************************************/
    case ficlInstructionGetLocalParen:
      PUSH_CELL_POINTER(frame + *ip++);

      /*
      ** Silly little minor optimizations.
      ** --lch
      */
    case ficlInstructionGetLocal0:
      PUSH_CELL_POINTER(frame);

    case ficlInstructionGetLocal1:
      PUSH_CELL_POINTER(frame + 1);

      /**************************************************************************
       ** Immediate - cfa of a local while compiling - when executed, compiles
       ** code to store the value of a local given the local's index in the
       ** word's pfa
       **************************************************************************/
    case ficlInstructionToLocalParen:
      POP_CELL_POINTER(frame + *ip++);

    case ficlInstructionToLocal0:
      POP_CELL_POINTER(frame);

    case ficlInstructionToLocal1:
      POP_CELL_POINTER(frame + 1);

#define POP_LOCAL_VAR_ADD(cp)					\
      do {							\
	FTH x;							\
								\
	cell = (cp);						\
	x = ficl_to_fth(CELL_FTH_REF(cell));			\
	if (FTH_FLOAT_P(x))					\
	  FTH_FLOAT_OBJECT(x) += VM_STACK_FLOAT_REF(dataTop);	\
	else if (FTH_LLONG_P(x))				\
	  FTH_LONG_OBJECT(x) += VM_STACK_LONG_REF(dataTop);	\
	else							\
	  CELL_INT_REF(cell) += VM_STACK_INT_REF(dataTop);	\
	dataTop--;						\
      } while (0)
	  
      /*
       * plus-store local vars [ms]
       * +to ( val "name" -- )
       */
    case ficlInstructionPlusToLocalParen:
      POP_LOCAL_VAR_ADD(frame + *ip++);
      continue;

    case ficlInstructionPlusToLocal0:
      POP_LOCAL_VAR_ADD(frame);
      continue;

    case ficlInstructionPlusToLocal1:
      POP_LOCAL_VAR_ADD(frame + 1);
      continue;

#define POP_LOCAL_VAR_FADD(cp)					\
      do {							\
	FTH x;							\
								\
	cell = (cp);						\
	x = ficl_to_fth(CELL_FTH_REF(cell));			\
	if (FTH_FLOAT_P(x))					\
	  FTH_FLOAT_OBJECT(x) += VM_STACK_FLOAT_REF(dataTop);	\
	else							\
	  VM_STACK_FLOAT_SET(cell, VM_STACK_FLOAT_REF(cell) + VM_STACK_FLOAT_REF(dataTop)); \
	dataTop--;						\
      } while (0)
	  
      /*
       * fplus-store local vars [ms]
       * f+to ( val "name" -- )
       */
    case ficlInstructionFPlusToLocalParen:
      POP_LOCAL_VAR_FADD(frame + *ip++);
      continue;

    case ficlInstructionFPlusToLocal0:
      POP_LOCAL_VAR_FADD(frame);
      continue;

    case ficlInstructionFPlusToLocal1:
      POP_LOCAL_VAR_FADD(frame + 1);
      continue;

    case ficlInstructionDup:
    case ficlInstructionFDup:
      CHECK_STACK(1, 2);
      i = VM_STACK_INT_REF(dataTop);
      ++dataTop;
      VM_STACK_INT_SET(dataTop, i);
      continue;

    /* WITHIN for all numbers. */
    /* [ms] ( test low high -- flag ) */
    case ficlInstructionWithin:
    {
      FTH test, low, high, tmp1, tmp2;
      
      CHECK_STACK(3, 1);
      high = ficl_to_fth(VM_STACK_FTH_REF(dataTop));
      dataTop--;
      low  = ficl_to_fth(VM_STACK_FTH_REF(dataTop));
      dataTop--;
      test = ficl_to_fth(VM_STACK_FTH_REF(dataTop));
      tmp2 = fth_number_sub(high, low);
      tmp1 = fth_number_sub(test, low);
      VM_STACK_BOOL_SET(dataTop, fth_number_less_p(tmp1, tmp2));
      continue;
    }
	
    case ficlInstructionQuestionDup:
      CHECK_STACK(1, 2);

      if (VM_NOT_FALSE_P(VM_STACK_FTH_REF(dataTop)))
      {
	dataTop[1] = dataTop[0];
	dataTop++;
      }
      continue;

    case ficlInstructionSwap:
    case ficlInstructionFSwap:
    {
      ficlCell swap;

      CHECK_STACK(2, 2);
      swap = dataTop[0];
      dataTop[0] = dataTop[-1];
      dataTop[-1] = swap;
      continue;
    }

    case ficlInstructionDrop:
    case ficlInstructionFDrop:
      CHECK_STACK(1, 0);
      dataTop--;
      continue;

      /* [ms] ( y x -- x ) */
    case ficlInstructionNip:
      CHECK_STACK(2, 1);
      dataTop[-1] = dataTop[0];
      dataTop--;
      continue;
      /* [ms] ( y x -- x y x ) */
    case ficlInstructionTuck:
    {
      ficlCell swap;
	  
      CHECK_STACK(2, 3);
      swap = dataTop[-1];			 /* y */
      dataTop[-1] = dataTop[1] = dataTop[0]; /* x */
      dataTop[0] = swap;			 /* y */
      dataTop++;
      continue;
    }

    case ficlInstruction2Drop:
      CHECK_STACK(2, 0);
      dataTop -= 2;
      continue;

    case ficlInstruction2Dup:
      CHECK_STACK(2, 4);
      dataTop[1] = dataTop[-1];
      dataTop[2] = *dataTop;
      dataTop += 2;
      continue;

    case ficlInstructionOver:
    case ficlInstructionFOver:
      CHECK_STACK(2, 3);
      dataTop[1] = dataTop[-1];
      dataTop++;
      continue;

    case ficlInstruction2Over:
      CHECK_STACK(4, 6);
      dataTop[1] = dataTop[-3];
      dataTop[2] = dataTop[-2];
      dataTop += 2;
      continue;

    case ficlInstructionPick:
      CHECK_STACK(1, 0);
      i = VM_STACK_INT_REF(dataTop);
      if (i < 0)
	continue;
      CHECK_STACK((int)i + 1, (int)i + 2);
      *dataTop = dataTop[-i];
      continue;

      /*******************************************************************
       ** Do stack rot.
       ** rot ( 1 2 3  -- 2 3 1 )
       *******************************************************************/
    case ficlInstructionRot:
    case ficlInstructionFRot:
      i = 2;
      goto ROLL;

      /*******************************************************************
       ** Do stack roll.
       ** roll ( n -- )
       *******************************************************************/
    case ficlInstructionRoll:
      CHECK_STACK(1, 0);
      i = VM_STACK_INT_REF(dataTop);
      dataTop--;
      if (i < 1)
	continue;
ROLL:
      CHECK_STACK((int)i + 1, (int)i + 2);
      c = dataTop[-i];
      memmove(dataTop - i, dataTop - (i - 1), (size_t)i * sizeof(ficlCell));
      *dataTop = c;
      continue;

      /*******************************************************************
       ** Do stack -rot.
       ** -rot ( 1 2 3  -- 3 1 2 )
       *******************************************************************/
    case ficlInstructionMinusRot:
      i = 2;
      goto MINUSROLL;

      /*******************************************************************
       ** Do stack -roll.
       ** -roll ( n -- )
       *******************************************************************/
    case ficlInstructionMinusRoll:
      CHECK_STACK(1, 0);
      i = VM_STACK_INT_REF(dataTop);
      dataTop--;
      if (i < 1)
	continue;
MINUSROLL:
      CHECK_STACK((int)i + 1, (int)i + 2);
      c = *dataTop;
      memmove(dataTop - (i - 1), dataTop - i, (size_t)i * sizeof(ficlCell));
      dataTop[-i] = c;
      continue;

      /*******************************************************************
       ** Do stack 2swap
       ** 2swap ( 1 2 3 4  -- 3 4 1 2 )
       *******************************************************************/
    case ficlInstruction2Swap:
    {
      ficlCell c2;

      CHECK_STACK(4, 4);
      c = *dataTop;
      c2 = dataTop[-1];
      *dataTop = dataTop[-2];
      dataTop[-1] = dataTop[-3];
      dataTop[-2] = c;
      dataTop[-3] = c2;
      continue;
    }

    case ficlInstructionPlusStore:
    {
      FTH x;
	    
      CHECK_STACK(2, 0);
      cell = VM_STACK_VOIDP_REF(dataTop);
      dataTop--;
      x = ficl_to_fth(CELL_FTH_REF(cell));
      if (FTH_FLOAT_P(x))
	FTH_FLOAT_OBJECT(x) += VM_STACK_FLOAT_REF(dataTop);
      else if (FTH_LLONG_P(x))
	FTH_LONG_OBJECT(x) += VM_STACK_LONG_REF(dataTop);
      else
	CELL_INT_REF(cell) += VM_STACK_INT_REF(dataTop);
      dataTop--;
    }
    continue;

    case ficlInstructionCFetch:
    {
      ficlUnsigned8 *integer8;

      CHECK_STACK(1, 1);
      integer8 = (ficlUnsigned8 *)VM_STACK_VOIDP_REF(dataTop);
      VM_STACK_UINT_SET(dataTop, *integer8);
      continue;
    }

    case ficlInstructionCStore:
    {
      ficlUnsigned8 *integer8;

      CHECK_STACK(2, 0);
      integer8 = (ficlUnsigned8 *)VM_STACK_VOIDP_REF(dataTop);
      dataTop--;
      *integer8 = (ficlUnsigned8)VM_STACK_UINT_REF(dataTop);
      dataTop--;
      continue;
    }

    case ficlInstructionAnd:
      CHECK_STACK(2, 1);
      if (fth_instance_p(VM_STACK_FTH_REF(dataTop)))
      {
	FTH val1, val2;

	val2 = VM_STACK_FTH_REF(dataTop);
	dataTop--;
	val1 = VM_STACK_FTH_REF(dataTop);

	if (VM_NOT_FALSE_P(val1) && VM_NOT_FALSE_P(val2))
	  VM_STACK_FTH_SET(dataTop, val2);
	else
	  VM_STACK_FTH_SET(dataTop, FTH_FALSE);
      }
      else
      {
	i = VM_STACK_INT_REF(dataTop);
	dataTop--;
	VM_STACK_INT_REF(dataTop) &= i;
      }
      continue;

    case ficlInstructionOr:
      CHECK_STACK(2, 1);
      if (fth_instance_p(VM_STACK_FTH_REF(dataTop)))
      {
	FTH val1, val2;

	val2 = VM_STACK_FTH_REF(dataTop);
	dataTop--;
	val1 = VM_STACK_FTH_REF(dataTop);

	if (VM_NOT_FALSE_P(val1))
	  VM_STACK_FTH_SET(dataTop, val1);
	else if (VM_NOT_FALSE_P(val2))
	  VM_STACK_FTH_SET(dataTop, val2);
	else
	  VM_STACK_FTH_SET(dataTop, FTH_FALSE);
      }
      else
      {
	i = VM_STACK_INT_REF(dataTop);
	dataTop--;
	VM_STACK_INT_REF(dataTop) |= i;
      }
      continue;

    case ficlInstructionXor:
      CHECK_STACK(2, 1);
      i = VM_STACK_INT_REF(dataTop);
      dataTop--;
      VM_STACK_INT_REF(dataTop) ^= i;
      continue;

    case ficlInstructionInvert:
      CHECK_STACK(1, 1);
      VM_STACK_INT_REF(dataTop) = ~VM_STACK_INT_REF(dataTop);
      continue;

      /**************************************************************************
       ** r e t u r n   s t a c k
       ** 
       **************************************************************************/
    case ficlInstructionToRStack:
      CHECK_STACK(1, 0);
      CHECK_RETURN_STACK(0, 1);
      *++returnTop = *dataTop--;
      continue;

    case ficlInstructionFromRStack:
      CHECK_STACK(0, 1);
      CHECK_RETURN_STACK(1, 0);
      *++dataTop = *returnTop--;
      continue;

    case ficlInstructionFetchRStack:
      CHECK_STACK(0, 1);
      CHECK_RETURN_STACK(1, 1);
      *++dataTop = *returnTop;
      continue;

    case ficlInstruction2ToR:
      CHECK_STACK(2, 0);
      CHECK_RETURN_STACK(0, 2);
      *++returnTop = dataTop[-1];
      *++returnTop = dataTop[0];
      dataTop -= 2;
      continue;

    case ficlInstruction2RFrom:
      CHECK_STACK(0, 2);
      CHECK_RETURN_STACK(2, 0);
      *++dataTop = returnTop[-1];
      *++dataTop = returnTop[0];
      returnTop -= 2;
      continue;

    case ficlInstruction2RFetch:
      CHECK_STACK(0, 2);
      CHECK_RETURN_STACK(2, 2);
      *++dataTop = returnTop[-1];
      *++dataTop = returnTop[0];
      continue;

      /**************************************************************************
       **				f i l l
       ** CORE ( c-addr u char -- )
       ** If u is greater than zero, store char in each of u consecutive
       ** characters of memory beginning at c-addr. 
       **************************************************************************/
    case ficlInstructionFill:
    {
      char ch;
      char *memory;

      CHECK_STACK(3, 0);
      ch = (char)VM_STACK_INT_REF(dataTop);
      dataTop--;
      u = VM_STACK_UINT_REF(dataTop);
      dataTop--;
      memory = (char *)VM_STACK_VOIDP_REF(dataTop);
      dataTop--;
      /* memset() is faster than the previous hand-rolled solution.  --lch */
      memset(memory, ch, u);
      continue;
    }

    /**************************************************************************
     **				m o v e
     ** CORE ( addr1 addr2 u -- )
     ** If u is greater than zero, copy the contents of u consecutive address
     ** units at addr1 to the u consecutive address units at addr2. After MOVE
     ** completes, the u consecutive address units at addr2 contain exactly
     ** what the u consecutive address units at addr1 contained before the move. 
     ** NOTE! This implementation assumes that a char is the same size as
     **       an address unit.
     **************************************************************************/
    case ficlInstructionMove:
    {
      char *addr2;
      char *addr1;

      CHECK_STACK(3, 0);
      u = VM_STACK_UINT_REF(dataTop);
      dataTop--;
      addr2 = VM_STACK_VOIDP_REF(dataTop);
      dataTop--;
      addr1 = VM_STACK_VOIDP_REF(dataTop);
      dataTop--;

      if (u == 0) 
	continue;
      /*
      ** Do the copy carefully, so as to be
      ** correct even if the two ranges overlap
      */
      /* Which ANSI C's memmove() does for you!  Yay!  --lch */
      memmove(addr2, addr1, u);
      continue;
    }

    /**************************************************************************
     **						c o m p a r e 
     ** STRING ( c-addr1 u1 c-addr2 u2 -- n )
     ** Compare the string specified by c-addr1 u1 to the string specified by
     ** c-addr2 u2. The strings are compared, beginning at the given addresses,
     ** character by character, up to the length of the shorter string or until a
     ** difference is found. If the two strings are identical, n is zero. If the two
     ** strings are identical up to the length of the shorter string, n is minus-one
     ** (-1) if u1 is less than u2 and one (1) otherwise. If the two strings are not
     ** identical up to the length of the shorter string, n is minus-one (-1) if the 
     ** first non-matching character in the string specified by c-addr1 u1 has a
     ** lesser numeric value than the corresponding character in the string specified
     ** by c-addr2 u2 and one (1) otherwise. 
     **************************************************************************/
    case ficlInstructionCompare:
      i = FICL_FALSE;
      goto COMPARE;

    case ficlInstructionCompareInsensitive:
      i = FICL_TRUE;
      goto COMPARE;
    COMPARE:
      {
	char *cp1, *cp2;
	ficlUnsigned u1, u2, uMin;
	int n = 0;

	CHECK_STACK(4, 1);
	u2  = VM_STACK_UINT_REF(dataTop);
	dataTop--;
	cp2 = (char *)VM_STACK_VOIDP_REF(dataTop);
	dataTop--;
	u1  = VM_STACK_UINT_REF(dataTop);
	dataTop--;
	cp1 = (char *)VM_STACK_VOIDP_REF(dataTop);
	dataTop--;
	uMin = (u1 < u2)? u1 : u2;

	for (; (uMin > 0) && (n == 0); uMin--)
	{
	  char c1 = *cp1++;
	  char c2 = *cp2++;

	  if (i)
	  {
	    c1 = (char)tolower((int)c1);
	    c2 = (char)tolower((int)c2);
	  }

	  n = (int)(c1 - c2);
	}

	if (n == 0)
	  n = (int)(u1 - u2);

	if (n < 0) 
	  n = -1;
	else if (n > 0)
	  n = 1;

	++dataTop;
	VM_STACK_INT_SET(dataTop, n);
	continue;
      }

    /**************************************************************************
     ** This function simply pops the previous instruction
     ** pointer and returns to the "next" loop. Used for exiting from within
     ** a definition. Note that exitParen is identical to semiParen - they
     ** are in two different functions so that "see" can correctly identify
     ** the end of a colon definition, even if it uses "exit".
     **************************************************************************/
    case ficlInstructionExitParen:
    case ficlInstructionSemiParen:
      EXIT_FUNCTION();

      /**************************************************************************
       ** The first time we run "(branch)", perform a "peephole optimization" to
       ** see if we're jumping to another unconditional jump.  If so, just jump
       ** directly there.
       **************************************************************************/
    case ficlInstructionBranchParenWithCheck:
      LOCAL_VARIABLE_SPILL();
      ficlVmOptimizeJumpToJump(vm, vm->ip - 1);
      LOCAL_VARIABLE_REFILL();
      goto BRANCH_PAREN;

      /**************************************************************************
       ** Same deal with branch0.
       **************************************************************************/
    case ficlInstructionBranch0ParenWithCheck:
      LOCAL_VARIABLE_SPILL();
      ficlVmOptimizeJumpToJump(vm, vm->ip - 1);
      LOCAL_VARIABLE_REFILL();
      /* intentional fall-through */

      /**************************************************************************
       ** Runtime code for "(branch0)"; pop a flag from the stack,
       ** branch if 0. fall through otherwise.  The heart of "if" and "until".
       **************************************************************************/
      /* fall through */
    case ficlInstructionBranch0Paren:
    {
      FTH val;
	    
      CHECK_STACK(1, 0);
      val = VM_STACK_FTH_REF(dataTop); /* macro calls val 3 times */
      dataTop--;
	    
      if (VM_NOT_FALSE_P(val))
      {
	/* don't branch, but skip over branch relative address */
	ip += 1;
	continue;
      }
    }
    /* otherwise, take branch (to else/endif/begin) */
    /* intentional fall-through! */

    /**************************************************************************
     ** Runtime for "(branch)" -- expects a literal offset in the next
     ** compilation address, and branches to that location.
     **************************************************************************/
    /* fall through */
    case ficlInstructionBranchParen:
    {
    BRANCH_PAREN:
      BRANCH();
    }

    case ficlInstructionOfParen:
    {
      ficlUnsigned a, b;

      CHECK_STACK(2, 1);
      a = VM_STACK_UINT_REF(dataTop);
      dataTop--;
      b = VM_STACK_UINT_REF(dataTop);

      if (a == b)
      {
	/* fall through */
	ip++;
	/* remove CASE argument */
	dataTop--;
      }
      else 
      {
	/* take branch to next of or endcase */
	BRANCH();
      }
      continue;
    }

    case ficlInstructionDoParen:
    {
      ficlCell index, limit;

      CHECK_STACK(2, 0);
      index = *dataTop--;
      limit = *dataTop--;
      /* copy "leave" target addr to stack */
      ++returnTop;
      VM_STACK_INT_SET(returnTop, *ip);
      ip++;
      *++returnTop = limit;
      *++returnTop = index;
      gc_push(vm->runningWord);	/* [ms] okay */
      continue;
    }

    case ficlInstructionQDoParen:
    {
      volatile ficlCell index, limit, leave;

      CHECK_STACK(2, 0);
      index = *dataTop--;
      limit = *dataTop--;
      FICL_FW_CHECK(ip);
      CELL_INT_SET(&leave, *ip);

      if (CELL_LONG_REF(&limit) == CELL_LONG_REF(&index))
	ip = CELL_VOIDP_REF(&leave);
      else
      {
	ip++;
	*++returnTop = leave;
	*++returnTop = limit;
	*++returnTop = index;
	gc_push(vm->runningWord); /* [ms] okay */
      }
      continue;
    }

    case ficlInstructionLoopParen:
    case ficlInstructionPlusLoopParen:
    {
      ficl2Integer index;
      ficl2Integer limit;
      int direction = 0;

      index = VM_STACK_LONG_REF(returnTop);
      limit = CELL_LONG_REF(&returnTop[-1]);

      if (instruction == ficlInstructionLoopParen)
	index++;
      else
      {
	ficlInteger increment;
		
	CHECK_STACK(1, 0);
	increment = VM_STACK_INT_REF(dataTop);
	dataTop--;
	index += increment;
	direction = (increment < 0);
      }

      if (direction ^ (index >= limit))
      {
	returnTop -= 3; /* nuke the loop indices & "leave" addr */
	ip++;  /* fall through the loop */
	gc_pop();		/* [ms] okay */
      }
      else 
      {
	gc_loop_reset();
	/* update index, branch to loop head */
	if (FIXABLE_P(index))
	  VM_STACK_INT_SET(returnTop, index);
	else
	  VM_STACK_LONG_SET(returnTop, index);
	BRANCH();
      }
      continue;
    }

    /*
    ** Runtime code to break out of a do..loop construct
    ** Drop the loop control variables; the branch address
    ** past "loop" is next on the return stack.
    */
    case ficlInstructionLeave:
      /* almost unloop */
      returnTop -= 2;
      /* exit */
      gc_pop();			/* [ms] okay */
      EXIT_FUNCTION();

    case ficlInstructionUnloop:
      returnTop -= 3;
      continue;

    case ficlInstructionI:
      *++dataTop = *returnTop;
      continue;

    case ficlInstructionJ:
      *++dataTop = returnTop[-3];
      continue;

    case ficlInstructionK:
      *++dataTop = returnTop[-6];
      continue;

    case ficlInstructionDoesParen:
    {
      ficlDictionary *dict = ficlVmGetDictionary(vm);

      dict->smudge->code = (ficlPrimitive)ficlInstructionDoDoes;
      CELL_VOIDP_SET(dict->smudge->param, ip);
      ip = (ficlInstruction *)(VM_STACK_VOIDP_REF(returnTop));
      returnTop--;
      /* [ms] DOES>: ends init part starting at `:' */
      continue;
    }

    case ficlInstructionDoDoes:
    {
      ficlIp tempIP;

      CHECK_STACK(0, 1);
      FICL_FW_CHECK(fw);
      cell = fw->param;
      tempIP = (ficlIp)CELL_VOIDP_REF(cell);
      ++dataTop;
      VM_STACK_VOIDP_SET(dataTop, cell + 1);
      ++returnTop;
      VM_STACK_VOIDP_SET(returnTop, ip);
      ip = (ficlInstruction *)tempIP;
      /* [ms] DOES>: starts body up to `;' */
      continue;
    }

    /*
    ** fetch CORE ( a-addr -- x )
    **
    ** x is the value stored at a-addr.
    */
    case ficlInstructionFetch:
      CHECK_STACK(1, 1);
      *dataTop = *((ficlCell *)VM_STACK_VOIDP_REF(dataTop));
      continue;

      /*
      ** store        CORE ( x a-addr -- )
      ** Store x at a-addr. 
      */
    case ficlInstructionStore:
    {
      FTH out;
      
      CHECK_STACK(2, 0);
      cell = VM_STACK_VOIDP_REF(dataTop);
      dataTop--;
      out = ficl_to_fth(CELL_FTH_REF(cell));
      *cell = *dataTop--;
      fth_gc_protect_set(out, ficl_to_fth(CELL_FTH_REF(cell)));
      continue;
    }

    case ficl_execute_trace_var:
    {
      ficlWord *word;
      FTH out;
      
      CHECK_STACK(3, 0);
      word = VM_STACK_VOIDP_REF(dataTop);	/* word */
      dataTop--;
      cell = VM_STACK_VOIDP_REF(dataTop);	/* address of variable */
      dataTop--;
      out = ficl_to_fth(CELL_FTH_REF(cell));
      *cell = *dataTop--;	/* new value */
      fth_gc_protect_set(out, ficl_to_fth(CELL_FTH_REF(cell)));
      if (FTH_TRACE_VAR_P(word))
	fth_trace_var_execute(word);
      continue;
    }

    case ficlInstructionComma:
    case ficlInstructionCompileComma:
    {
      ficlDictionary *dict;

      CHECK_STACK(1, 0);
      dict = ficlVmGetDictionary(vm);
      cell = dataTop--;
      fth_gc_permanent(ficl_to_fth(CELL_FTH_REF(cell)));
      ficlDictionaryAppendCell(dict, *cell);
      continue;
    }

    case ficlInstructionCComma:
    {
      ficlDictionary *dict;
      char ch;

      CHECK_STACK(1, 0);
      dict = ficlVmGetDictionary(vm);
      ch = (char)VM_STACK_INT_REF(dataTop);
      dataTop--;
      ficlDictionaryAppendCharacter(dict, ch);
      continue;
    }

    /**************************************************************************
     **					l s h i f t
     ** l-shift CORE ( x1 u -- x2 )
     ** Perform a logical left shift of u bit-places on x1, giving x2.
     ** Put zeroes into the least significant bits vacated by the shift.
     ** An ambiguous condition exists if u is greater than or equal to the
     ** number of bits in a ficlCell. 
     **
     ** r-shift CORE ( x1 u -- x2 )
     ** Perform a logical right shift of u bit-places on x1, giving x2.
     ** Put zeroes into the most significant bits vacated by the shift. An
     ** ambiguous condition exists if u is greater than or equal to the
     ** number of bits in a ficlCell. 
     **************************************************************************/
    case ficlInstructionLShift:
    {
      ficlUnsigned nBits;

      CHECK_STACK(2, 1);
      nBits = VM_STACK_UINT_REF(dataTop);
      dataTop--;
      VM_STACK_UINT_REF(dataTop) <<= nBits;
      continue;
    }

    case ficlInstructionRShift:
    {
      ficlUnsigned nBits;

      CHECK_STACK(2, 1);
      nBits = VM_STACK_UINT_REF(dataTop);
      dataTop--;
      VM_STACK_UINT_REF(dataTop) >>= nBits;
      continue;
    }

    case ficlInstructionMax:
    {
      ficlInteger n2;
      ficlInteger n1;

      CHECK_STACK(2, 1);
      n2 = VM_STACK_INT_REF(dataTop);
      dataTop--;
      n1 = VM_STACK_INT_REF(dataTop);
      VM_STACK_INT_SET(dataTop, FICL_MAX(n1, n2));
      continue;
    }

    case ficlInstructionMin:
    {
      ficlInteger n2;
      ficlInteger n1;

      CHECK_STACK(2, 1);
      n2 = VM_STACK_INT_REF(dataTop);
      dataTop--;
      n1 = VM_STACK_INT_REF(dataTop);
      VM_STACK_INT_SET(dataTop, FICL_MIN(n1, n2));
      continue;
    }

    case ficlInstructionCells:
      CHECK_STACK(1, 1);
      VM_STACK_INT_REF(dataTop) *= (ficlInteger)sizeof(ficlCell);
      continue;

    case ficlInstructionCellPlus:
      CHECK_STACK(1, 1);
      VM_STACK_INT_REF(dataTop) += (ficlInteger)sizeof(ficlCell);
      continue;

    /**************************************************************************
     ** l o g i c   a n d   c o m p a r i s o n s
     ** 
     **************************************************************************/
    case ficlInstructionEquals:
      CHECK_STACK(2, 1);
      i = VM_STACK_INT_REF(dataTop);
      dataTop--;
      VM_STACK_BOOL_SET(dataTop, VM_STACK_INT_REF(dataTop) == i);
      continue;

    /* <> ( n -- f ) */
    case ficl_not_eql:
      CHECK_STACK(2, 1);
      i = VM_STACK_INT_REF(dataTop);
      dataTop--;
      VM_STACK_BOOL_SET(dataTop, VM_STACK_INT_REF(dataTop) != i);
      continue;

    case ficlInstructionLess:
      CHECK_STACK(2, 1);
      i = VM_STACK_INT_REF(dataTop);
      dataTop--;
      VM_STACK_BOOL_SET(dataTop, VM_STACK_INT_REF(dataTop) < i);
      continue;

    case ficlInstructionGreaterThan:
    {
      ficlInteger x, y;

      CHECK_STACK(2, 1);
      y = VM_STACK_INT_REF(dataTop);
      dataTop--;
      x = VM_STACK_INT_REF(dataTop);
      VM_STACK_BOOL_SET(dataTop, x > y);
      continue;
    }

      /* <= ( n1 n2 -- f ) */
    case ficl_less_eql:
      CHECK_STACK(2, 1);
      i = VM_STACK_INT_REF(dataTop);
      dataTop--;
      VM_STACK_BOOL_SET(dataTop, VM_STACK_INT_REF(dataTop) <= i);
      continue;

      /* >= ( n1 n2 -- f ) */
    case ficl_greater_eql:
      CHECK_STACK(2, 1);
      i = VM_STACK_INT_REF(dataTop);
      dataTop--;
      VM_STACK_BOOL_SET(dataTop, VM_STACK_INT_REF(dataTop) >= i);
      continue;
 
   /* zero? ( n -- f ) */
    case ficl_zero_p:
    case ficlInstruction0Equals:
      CHECK_STACK(1, 1);
      VM_STACK_BOOL_SET(dataTop, VM_STACK_INT_REF(dataTop) == 0);
      continue;

      /* 0<> ( n -- f ) */
    case ficl_0_not_eql:
      CHECK_STACK(1, 1);
      VM_STACK_BOOL_SET(dataTop, VM_STACK_INT_REF(dataTop) != 0);
      continue;

      /* negative? ( n -- f ) */
    case ficl_negative_p:
    case ficlInstruction0Less:
      CHECK_STACK(1, 1);
      VM_STACK_BOOL_SET(dataTop, VM_STACK_INT_REF(dataTop) < 0);
      continue;

    case ficlInstruction0Greater:
      CHECK_STACK(1, 1);
      VM_STACK_BOOL_SET(dataTop, VM_STACK_INT_REF(dataTop) > 0);
      continue;

      /* 0<= ( n -- f ) */
    case ficl_0_less_eql:
      CHECK_STACK(1, 1);
      VM_STACK_BOOL_SET(dataTop, VM_STACK_INT_REF(dataTop) <= 0);
      continue;

      /* positive? ( n -- f ) */
      /* 0>= ( n -- f ) */
    case ficl_positive_p:
    case ficl_0_greater_eql:
      CHECK_STACK(1, 1);
      VM_STACK_BOOL_SET(dataTop, VM_STACK_INT_REF(dataTop) >= 0);
      continue;

    case ficlInstructionPlus:
      CHECK_STACK(2, 1);
      i = VM_STACK_INT_REF(dataTop);
      dataTop--;
      VM_STACK_INT_REF(dataTop) += i;
      continue;

    case ficlInstructionMinus:
      CHECK_STACK(2, 1);
      i = VM_STACK_INT_REF(dataTop);
      dataTop--;
      VM_STACK_INT_REF(dataTop) -= i;
      continue;

    case ficlInstructionStar:
      CHECK_STACK(2, 1);
      i = VM_STACK_INT_REF(dataTop);
      dataTop--;
      VM_STACK_INT_REF(dataTop) *= i;
      continue;

    case ficlInstructionSlash:
      CHECK_STACK(2, 1);
      i = VM_STACK_INT_REF(dataTop);
      dataTop--;
      VM_STACK_INT_REF(dataTop) /= i;
      continue;

    case ficlInstruction1Plus:
      CHECK_STACK(1, 1);
      VM_STACK_INT_REF(dataTop)++;
      continue;

    case ficlInstruction1Minus:
      CHECK_STACK(1, 1);
      VM_STACK_INT_REF(dataTop)--;
      continue;

    case ficlInstruction2Plus:
      CHECK_STACK(1, 1);
      VM_STACK_INT_REF(dataTop) += 2;
      continue;

    case ficlInstruction2Minus:
      CHECK_STACK(1, 1);
      VM_STACK_INT_REF(dataTop) -= 2;
      continue;

    case ficlInstruction2Star:
      CHECK_STACK(1, 1);
      VM_STACK_INT_REF(dataTop) <<= 1;
      continue;

    case ficlInstruction2Slash:
      CHECK_STACK(1, 1);
      VM_STACK_INT_REF(dataTop) >>= 1;
      continue;

    case ficlInstructionNegate:
      CHECK_STACK(1, 1);
      VM_STACK_INT_SET(dataTop, -VM_STACK_INT_REF(dataTop));
      continue;

      /* abs ( n1 -- n2 ) */
    case ficl_abs:
      CHECK_STACK(1, 1);
      VM_STACK_INT_SET(dataTop, abs((int)VM_STACK_INT_REF(dataTop)));
      continue;

      /*
      ** slash-mod        CORE ( n1 n2 -- n3 n4 )
      ** Divide n1 by n2, giving the single-ficlCell remainder n3 and the single-ficlCell
      ** quotient n4. An ambiguous condition exists if n2 is zero. If n1 and n2
      ** differ in sign, the implementation-defined result returned will be the
      ** same as that returned by either the phrase
      ** >R S>D R> FM/MOD or the phrase >R S>D R> SM/REM . 
      ** NOTE: Ficl complies with the second phrase (symmetric division)
      */
    case ficlInstructionSlashMod:
    {
      ficl2Integer d1;
      ficlInteger n2;
      ficl2IntegerQR qr;

      CHECK_STACK(2, 2);
      n2 = VM_STACK_INT_REF(dataTop);
      dataTop--;
      d1 = VM_STACK_LONG_REF(dataTop);
      qr = ficl2IntegerDivideSymmetric(d1, n2);
      VM_STACK_INT_SET(dataTop, qr.remainder);
      dataTop++;
      VM_STACK_INT_SET(dataTop, (ficlInteger)qr.quotient);
      continue;
    }

    case ficlInstructionStarSlash:
    {
      ficlInteger x, y, z;
      ficl2IntegerQR qr;

      CHECK_STACK(3, 1);
      z = VM_STACK_INT_REF(dataTop);
      dataTop--;
      y = VM_STACK_INT_REF(dataTop);
      dataTop--;
      x = VM_STACK_INT_REF(dataTop);
      qr = ficl2IntegerDivideSymmetric((ficl2Integer)(x * y), z);
      VM_STACK_INT_SET(dataTop, (ficlInteger)qr.quotient);
      continue;
    }

    case ficlInstructionStarSlashMod:
    {
      ficlInteger x, y, z;
      ficl2IntegerQR qr;

      CHECK_STACK(3, 2);
      z = VM_STACK_INT_REF(dataTop);
      dataTop--;
      y = VM_STACK_INT_REF(dataTop);
      dataTop--;
      x = VM_STACK_INT_REF(dataTop);
      qr = ficl2IntegerDivideSymmetric((ficl2Integer)(x * y), z);
      VM_STACK_INT_SET(dataTop, qr.remainder);
      dataTop++;
      VM_STACK_INT_SET(dataTop, (ficlInteger)qr.quotient);
      continue;
    }

    case ficlInstructionF0:
      CHECK_STACK(0, 1);
      ++dataTop;
      VM_STACK_FLOAT_SET(dataTop, 0.0);
      continue;

    case ficlInstructionF1:
      CHECK_STACK(0, 1);
      ++dataTop;
      VM_STACK_FLOAT_SET(dataTop, 1.0);
      continue;

    case ficlInstructionFNeg1:
      CHECK_STACK(0, 1);
      ++dataTop;
      VM_STACK_FLOAT_SET(dataTop, -1.0);
      continue;

      /*******************************************************************
       ** Do float addition r1 + r2.
       ** f+ ( r1 r2 -- r )
       *******************************************************************/
    case ficlInstructionFPlus:
      CHECK_STACK(2, 1);
      df = VM_STACK_FLOAT_REF(dataTop);
      dataTop--;
      VM_STACK_FLOAT_SET(dataTop, VM_STACK_FLOAT_REF(dataTop) + df);
      continue;

      /*******************************************************************
       ** Do float subtraction r1 - r2.
       ** f- ( r1 r2 -- r )
       *******************************************************************/
    case ficlInstructionFMinus:
      CHECK_STACK(2, 1);
      df = VM_STACK_FLOAT_REF(dataTop);
      dataTop--;
      VM_STACK_FLOAT_SET(dataTop, VM_STACK_FLOAT_REF(dataTop) - df);
      continue;

      /*******************************************************************
       ** Do float multiplication r1 * r2.
       ** f* ( r1 r2 -- r )
       *******************************************************************/
    case ficlInstructionFStar:
      CHECK_STACK(2, 1);
      df = VM_STACK_FLOAT_REF(dataTop);
      dataTop--;
      VM_STACK_FLOAT_SET(dataTop, VM_STACK_FLOAT_REF(dataTop) * df);
      continue;

      /*******************************************************************
       ** Do float negation.
       ** fnegate ( r -- r )
       *******************************************************************/
    case ficlInstructionFNegate:
      CHECK_STACK(1, 1);
      VM_STACK_FLOAT_SET(dataTop, -(VM_STACK_FLOAT_REF(dataTop)));
      continue;

      /*******************************************************************
       ** Do float division r1 / r2.
       ** f/ ( r1 r2 -- r )
       *******************************************************************/
    case ficlInstructionFSlash:
      CHECK_STACK(2, 1);
      df = VM_STACK_FLOAT_REF(dataTop);
      dataTop--;
      VM_STACK_FLOAT_SET(dataTop, VM_STACK_FLOAT_REF(dataTop) / df);
      continue;

      /*******************************************************************
       ** Add a floating point number to contents of a variable.
       ** f+! ( r n -- )
       *******************************************************************/
    case ficlInstructionFPlusStore:
    {
      ficlCell *ce;
      FTH x;

      CHECK_STACK(2, 0);
      ce = VM_STACK_VOIDP_REF(dataTop);
      dataTop--;
      x = ficl_to_fth(CELL_FTH_REF(ce));
      if (FTH_FLOAT_P(x))
	FTH_FLOAT_OBJECT(x) += VM_STACK_FLOAT_REF(dataTop);
      else
	VM_STACK_FLOAT_SET(ce, VM_STACK_FLOAT_REF(ce) + VM_STACK_FLOAT_REF(dataTop));
      dataTop--;
      continue;
    }

    /*******************************************************************
     ** Do float 0= comparison r = 0.0.
     ** f0= ( r -- T/F )
     *******************************************************************/
    /* fzero? ( r -- f ) */
    case ficl_f_zero_p:
    case ficlInstructionF0Equals:
      CHECK_STACK(1, 1);
      VM_STACK_BOOL_SET(dataTop, VM_STACK_FLOAT_REF(dataTop) == 0.0);
      continue;

      /*******************************************************************
       ** Do float 0< comparison r < 0.0.
       ** f0< ( r -- T/F )
       *******************************************************************/
      /* fnegative? ( r -- f ) */
    case ficl_f_negative_p:
    case ficlInstructionF0Less:
      CHECK_STACK(1, 1);
      VM_STACK_BOOL_SET(dataTop, VM_STACK_FLOAT_REF(dataTop) < 0.0f);
      continue;

      /*******************************************************************
       ** Do float 0> comparison r > 0.0.
       ** f0> ( r -- T/F )
       *******************************************************************/
    case ficlInstructionF0Greater:
      CHECK_STACK(1, 1);
      VM_STACK_BOOL_SET(dataTop, VM_STACK_FLOAT_REF(dataTop) > 0.0);
      continue;

      /*******************************************************************
       ** Do float = comparison r1 = r2.
       ** f= ( r1 r2 -- T/F )
       *******************************************************************/
    case ficlInstructionFEquals:
      CHECK_STACK(2, 1);
      df = VM_STACK_FLOAT_REF(dataTop);
      dataTop--;
      VM_STACK_BOOL_SET(dataTop, VM_STACK_FLOAT_REF(dataTop) == df);
      continue;

      /*******************************************************************
       ** Do float < comparison r1 < r2.
       ** f< ( r1 r2 -- T/F )
       *******************************************************************/
    case ficlInstructionFLess:
      CHECK_STACK(2, 1);
      df = VM_STACK_FLOAT_REF(dataTop);
      dataTop--;
      VM_STACK_BOOL_SET(dataTop, VM_STACK_FLOAT_REF(dataTop) < df);
      continue;

      /*******************************************************************
       ** Do float > comparison r1 > r2.
       ** f> ( r1 r2 -- T/F )
       *******************************************************************/
    case ficlInstructionFGreater:
      CHECK_STACK(2, 1);
      df = VM_STACK_FLOAT_REF(dataTop);
      dataTop--;
      VM_STACK_BOOL_SET(dataTop, VM_STACK_FLOAT_REF(dataTop) > df);
      continue;

      /**************************************************************************
       **				c o l o n P a r e n
       ** This is the code that executes a colon definition. It assumes that the
       ** virtual machine is running a "next" loop (See the vm.c
       ** for its implementation of member function vmExecute()). The colon
       ** code simply copies the address of the first word in the list of words
       ** to interpret into IP after saving its old value. When we return to the
       ** "next" loop, the virtual machine will call the code for each word in 
       ** turn.
       **
       **************************************************************************/
    case ficlInstructionColonParen:
      ++returnTop;
      VM_STACK_VOIDP_SET(returnTop, ip);
      FICL_FW_CHECK(fw);
      ip = (ficlInstruction *)(fw->param);
      continue;

    case ficlInstructionCreateParen:
      CHECK_STACK(0, 1);
      FICL_FW_CHECK(fw);
      ++dataTop;
      VM_STACK_VOIDP_SET(dataTop, fw->param + 1);
      continue;

    case ficlInstructionVariableParen:
      CHECK_STACK(0, 1);
      FICL_FW_CHECK(fw);
      ++dataTop;
      VM_STACK_VOIDP_SET(dataTop, fw->param);
      continue;

      /*
       * [ms]
       * 
       * Sets word->current_word|file|line to actual source location.
       * Takes values preserved in ficlVmParse() at compile time for
       * later use:
       *
       * dict->smudge
       * fth_current_file
       * fth_current_line
       *
       * The next entry in instruction pointer is tempFW, file and
       * line number.
       */
    case ficl_word_location:
    {
      ficlWord *word, *next;
      FTH file;
      ficlInteger line;

      FICL_FW_CHECK(ip);
      word = (ficlWord *)*ip++;
      file = *ip++;
      line = *ip++;
      next = (ficlWord *)*ip;
      next->current_word = word;
      next->current_file = file;
      next->current_line = line;
      continue;
    }

    /* Executes C functions (0 to 20 args). */
    case ficl_execute_func:
    {
      FTH ret = FTH_FALSE;
      FTH args[20] = {FTH_UNDEF, FTH_UNDEF, FTH_UNDEF, FTH_UNDEF, FTH_UNDEF,
		      FTH_UNDEF, FTH_UNDEF, FTH_UNDEF, FTH_UNDEF, FTH_UNDEF,
		      FTH_UNDEF, FTH_UNDEF, FTH_UNDEF, FTH_UNDEF, FTH_UNDEF,
		      FTH_UNDEF, FTH_UNDEF, FTH_UNDEF, FTH_UNDEF, FTH_UNDEF};
      FTH (*func)();
      int  req, opt, length, depth;
      bool rest;
	    
      FICL_FW_CHECK(fw);
      vm->runningWord = fw;
      req    = fw->req;
      opt    = fw->opt;
      rest   = fw->rest;
      length = fw->argc;
      func   = fw->func;
      depth  = (int)(dataTop - vm->dataStack->base + 1);

      switch (length)			
      {					
      case 0:				
	ret = (*func)();	
	break;				
      case 1:				
	FUNC_ARGS();			
	ret = (*func)(args[0]);
	break;				
      case 2:				
	FUNC_ARGS();			
	ret = (*func)(args[0], args[1]);
	break;				
      case 3:				
	FUNC_ARGS();			
	ret = (*func)(args[0], args[1], args[2]);
	break;				
      case 4:				
	FUNC_ARGS();			
	ret = (*func)(args[0], args[1], args[2], args[3]);
	break;				
      case 5:				
	FUNC_ARGS();			
	ret = (*func)(args[0], args[1], args[2], args[3], args[4]);
	break;				
      case 6:				
	FUNC_ARGS();			
	ret = (*func)(args[0], args[1], args[2], args[3], args[4], args[5]);
	break;				
      case 7:				
	FUNC_ARGS();			
	ret = (*func)(args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
	break;				
      case 8:				
	FUNC_ARGS();			
	ret = (*func)(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
	break;				
      case 9:				
	FUNC_ARGS();			
	ret = (*func)(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
	break;				
      case 10:				
	FUNC_ARGS();			
	ret = (*func)(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9]);
	break;				
      case 11:				
	FUNC_ARGS();			
	ret = (*func)(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10]);
	break;				
      case 12:				
	FUNC_ARGS();			
	ret = (*func)(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11]);
	break;				
      case 13:				
	FUNC_ARGS();			
	ret = (*func)(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12]);
	break;				
      case 14:				
	FUNC_ARGS();			
	ret = (*func)(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13]);
	break;				
      case 15:				
	FUNC_ARGS();			
	ret = (*func)(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14]);
	break;				
      case 16:				
	FUNC_ARGS();			
	ret = (*func)(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14], args[15]);
	break;				
      case 17:				
	FUNC_ARGS();			
	ret = (*func)(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14], args[15], args[16]);
	break;
      case 18:				
	FUNC_ARGS();			
	ret = (*func)(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17]);
	break;				
      case 19:				
	FUNC_ARGS();			
	ret = (*func)(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18]);
	break;
      case 20:
	FUNC_ARGS();
	ret = (*func)(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19]);
	break;
      default:
	break;
      }
      ++dataTop;
      VM_STACK_FTH_SET(dataTop, fth_to_ficl(ret));
      continue;
    }

    /* Executes void C functions (0 to 10 args). */
    case ficl_execute_void_func:
    {
      FTH args[10] = {FTH_UNDEF, FTH_UNDEF, FTH_UNDEF, FTH_UNDEF, FTH_UNDEF,
		      FTH_UNDEF, FTH_UNDEF, FTH_UNDEF, FTH_UNDEF, FTH_UNDEF};
      void (*vfunc)();
      int  req, opt, length, depth;
      bool rest;
	    
      FICL_FW_CHECK(fw);
      vm->runningWord = fw;
      req    = fw->req;
      opt    = fw->opt;
      rest   = fw->rest;
      length = fw->argc;
      vfunc  = fw->vfunc;
      depth  = (int)(dataTop - vm->dataStack->base + 1);
	    
      switch (length)		
      {				
      case 0:			
	(*vfunc)();
	break;			
      case 1:			
	FUNC_ARGS();		
	(*vfunc)(args[0]);
	break;			
      case 2:			
	FUNC_ARGS();		
	(*vfunc)(args[0], args[1]);	
	break;			
      case 3:			
	FUNC_ARGS();		
	(*vfunc)(args[0], args[1], args[2]);	
	break;			
      case 4:			
	FUNC_ARGS();		
	(*vfunc)(args[0], args[1], args[2], args[3]);	
	break;			
      case 5:			
	FUNC_ARGS();		
	(*vfunc)(args[0], args[1], args[2], args[3], args[4]);	
	break;			
      case 6:			
	FUNC_ARGS();		
	(*vfunc)(args[0], args[1], args[2], args[3], args[4], args[5]);	
	break;			
      case 7:			
	FUNC_ARGS();		
	(*vfunc)(args[0], args[1], args[2], args[3], args[4], args[5], args[6]);	
	break;			
      case 8:			
	FUNC_ARGS();		
	(*vfunc)(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);	
	break;			
      case 9:			
	FUNC_ARGS();		
	(*vfunc)(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
	break;			
      case 10:			
	FUNC_ARGS();		
	(*vfunc)(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9]);
	break;
      default:
	break;
      }
      continue;
    }

    /**************************************************************************
     **				c o n s t a n t P a r e n
     ** This is the run-time code for "constant". It simply returns the 
     ** contents of its word's first data ficlCell.
     **
     **************************************************************************/
    case ficlInstructionConstantParen:
      CHECK_STACK(0, 1);
      FICL_FW_CHECK(fw);
      PUSH_CELL_POINTER(fw->param);

    case ficlInstructionUserParen:
      FICL_FW_CHECK(fw);
      ++dataTop;
      VM_STACK_VOIDP_SET(dataTop, &vm->user[CELL_INT_REF(fw->param)]);
      continue;
 
      /* === Begin of FTH Additions === */
      /* false ( -- 0 ) */
    case ficl_bool_false:
      CHECK_STACK(0, 1);
      ++dataTop;
      VM_STACK_INT_SET(dataTop, FICL_FALSE);
      continue;

      /* true ( -- -1 ) */
    case ficl_bool_true:
      CHECK_STACK(0, 1);
      ++dataTop;
      VM_STACK_INT_SET(dataTop, FICL_TRUE);
      continue;

      /* && ( obj1 obj2 -- obj2|#f ) */
    case ficl_bool_and:
    {
      FTH val1, val2;

      CHECK_STACK(2, 1);
      val2 = VM_STACK_FTH_REF(dataTop);
      dataTop--;
      val1 = VM_STACK_FTH_REF(dataTop);
      if (VM_NOT_FALSE_P(val1) && VM_NOT_FALSE_P(val2))
	VM_STACK_FTH_SET(dataTop, val2);
      else
	VM_STACK_FTH_SET(dataTop, FTH_FALSE);
      continue;
    }

    /* || ( obj1 obj2 -- obj1|2|#f ) */
    case ficl_bool_or:
    {
      FTH val1, val2;

      CHECK_STACK(2, 1);
      val2 = VM_STACK_FTH_REF(dataTop);
      dataTop--;
      val1 = VM_STACK_FTH_REF(dataTop);
      if (VM_NOT_FALSE_P(val1))
	VM_STACK_FTH_SET(dataTop, val1);
      else if (VM_NOT_FALSE_P(val2))
	VM_STACK_FTH_SET(dataTop, val2);
      else
	VM_STACK_FTH_SET(dataTop, FTH_FALSE);
      continue;
    }

    /* not ( obj -- f ) */
    case ficl_bool_not:
      CHECK_STACK(1, 1);
      VM_STACK_BOOL_SET(dataTop, VM_FALSE_OR_NULL_P(VM_STACK_FTH_REF(dataTop)));
      continue;

      /* fmax ( r1 r2 -- r1|r2 ) */
    case ficl_f_max:
    {
      ficlFloat f1;
      ficlFloat f2;

      CHECK_STACK(2, 1);
      f2 = VM_STACK_FLOAT_REF(dataTop);
      dataTop--;
      f1 = VM_STACK_FLOAT_REF(dataTop);
      VM_STACK_FLOAT_SET(dataTop, (f1 > f2) ? f1 : f2);
      continue;
    }

    /* fmin ( r1 r2 -- r1|r2 ) */
    case ficl_f_min:
    {
      ficlFloat f1;
      ficlFloat f2;

      CHECK_STACK(2, 1);
      f2 = VM_STACK_FLOAT_REF(dataTop);
      dataTop--;
      f1 = VM_STACK_FLOAT_REF(dataTop);
      VM_STACK_FLOAT_SET(dataTop, (f1 < f2) ? f1 : f2);
      continue;
    }

    /* f0<> ( r -- f ) */
    case ficl_f0_not_eql:
      CHECK_STACK(1, 1);
      VM_STACK_BOOL_SET(dataTop, VM_STACK_FLOAT_REF(dataTop) != 0.0);
      continue;

      /* f0<= ( r -- f ) */
    case ficl_f0_less_eql:
      CHECK_STACK(1, 1);
      VM_STACK_BOOL_SET(dataTop, VM_STACK_FLOAT_REF(dataTop) <= 0.0);
      continue;

      /* fpositive? ( r -- f ) */
      /* f0>= ( r -- f ) */
    case ficl_f_positive_p:
    case ficl_f0_greater_eql:
      CHECK_STACK(1, 1);
      VM_STACK_BOOL_SET(dataTop, VM_STACK_FLOAT_REF(dataTop) >= 0.0);
      continue;

      /* f>= ( r1 r2 -- f ) */
    case ficl_f_greater_eql:
      CHECK_STACK(2, 1);
      df = VM_STACK_FLOAT_REF(dataTop);
      dataTop--;
      VM_STACK_BOOL_SET(dataTop, VM_STACK_FLOAT_REF(dataTop) >= df);
      continue;

      /* f<= ( r1 r2 -- f ) */
    case ficl_f_less_eql:
      CHECK_STACK(2, 1);
      df = VM_STACK_FLOAT_REF(dataTop);
      dataTop--;
      VM_STACK_BOOL_SET(dataTop, VM_STACK_FLOAT_REF(dataTop) <= df);
      continue;

      /* f<> ( r1 r2 -- f ) */
    case ficl_f_not_eql:
      CHECK_STACK(2, 1);
      df = VM_STACK_FLOAT_REF(dataTop);
      dataTop--;
      VM_STACK_BOOL_SET(dataTop, VM_STACK_FLOAT_REF(dataTop) != df);
      continue;

      /* f2* ( r -- r*2.0 ) */
    case ficl_f2_star:
      CHECK_STACK(1, 1);
      VM_STACK_FLOAT_SET(dataTop, VM_STACK_FLOAT_REF(dataTop) * 2.0);
      continue;

      /* f2/ ( r -- r/2.0 ) */
    case ficl_f2_slash:
      CHECK_STACK(1, 1);
      VM_STACK_FLOAT_SET(dataTop, VM_STACK_FLOAT_REF(dataTop) * 0.5);
      continue;

      /* 1/f ( r -- 1.0/r ) */
    case ficl_1_slash_f:
      CHECK_STACK(1, 1);
      VM_STACK_FLOAT_SET(dataTop, 1.0 / VM_STACK_FLOAT_REF(dataTop));
      continue;
      /* === End of FTH Additions === */

    default:
      /*
      ** Clever hack, or evil coding?  You be the judge.
      **
      ** If the word we've been asked to execute is in fact
      ** an *instruction*, we grab the instruction, stow it
      ** in "i" (our local cache of *ip), and *jump* to the
      ** top of the switch statement.  --lch
      */
      if (fw && ((ficlInstruction)fw->code < ficlInstructionLast))
      {
	instruction = (ficlInstruction)fw->code;
	goto AGAIN;
      }

      LOCAL_VARIABLE_SPILL();
      FICL_FW_CHECK(fw);
      vm->runningWord = fw;
      fw->code(vm);
      LOCAL_VARIABLE_REFILL();
      continue;
    }
  }
  LOCAL_VARIABLE_SPILL();
  vm->exceptionHandler = oldExceptionHandler;
}

/**************************************************************************
 **                      v m G e t D i c t
 ** Returns the address dictionary for this VM's system
 **************************************************************************/
ficlDictionary  *ficlVmGetDictionary(ficlVm *vm)
{
  FICL_ASSERT(vm != NULL);
  return vm->callback.system->dictionary;
}

/**************************************************************************
 **                       v m G e t S t r i n g
 ** Parses a string out of the VM input buffer and copies up to the first
 ** FICL_COUNTED_STRING_MAX characters to the supplied destination buffer, a
 ** ficlCountedString. The destination string is NULL terminated.
 ** 
 ** Returns the address of the first unused character in the dest buffer.
 **************************************************************************/
char *ficlVmGetString(ficlVm *vm, ficlCountedString *counted, char delimiter)
{
  ficlString s = ficlVmParseStringEx(vm, delimiter, 0);

  if (FICL_STRING_GET_LENGTH(s) > FICL_COUNTED_STRING_MAX)
  {
    FICL_STRING_SET_LENGTH(s, FICL_COUNTED_STRING_MAX);
  }

  strncpy(counted->text, FICL_STRING_GET_POINTER(s), FICL_STRING_GET_LENGTH(s));
  counted->text[FICL_STRING_GET_LENGTH(s)] = '\0';
  counted->length = FICL_STRING_GET_LENGTH(s);
  return counted->text + FICL_STRING_GET_LENGTH(s) + 1;
}

/**************************************************************************
 **                      v m G e t W o r d
 ** vmGetWord calls vmGetWord0 repeatedly until it gets a string with 
 ** non-zero length.
 **************************************************************************/
ficlString ficlVmGetWord(ficlVm *vm)
{
  ficlString s = ficlVmGetWord0(vm);

  if (FICL_STRING_GET_LENGTH(s) == 0)
    ficlVmThrow(vm, FICL_VM_STATUS_RESTART);

  return s;
}

/**************************************************************************
 **                      v m G e t W o r d 0
 ** Skip leading whitespace and parse a space delimited word from the tib.
 ** Returns the start address and length of the word. Updates the tib
 ** to reflect characters consumed, including the trailing delimiter.
 ** If there's nothing of interest in the tib, returns zero. This function
 ** does not use vmParseString because it uses isspace() rather than a
 ** single  delimiter character.
 **************************************************************************/
ficlString ficlVmGetWord0(ficlVm *vm)
{
  char *trace = ficlVmGetInBuf(vm);
  char *stop = ficlVmGetInBufEnd(vm);
  ficlString s;
  ficlUnsigned length = 0;
  char c = 0;

  trace = ficlStringSkipSpace(trace, stop);
  FICL_STRING_SET_POINTER(s, trace);

  /* Please leave this loop this way; it makes Purify happier.  --lch */
  for (;;)
  {
    if (trace == stop)
      break;
    c = *trace;
    if (isspace((int)c))
      break;
    length++;
    trace++;
  }

  FICL_STRING_SET_LENGTH(s, length);

  if ((trace != stop) && isspace((int)c))    /* skip one trailing delimiter */
    trace++;

  ficlVmUpdateTib(vm, trace);
  return s;
}

/**************************************************************************
 **                      v m G e t W o r d T o P a d
 ** Does vmGetWord and copies the result to the pad as a NULL terminated
 ** string. Returns the length of the string. If the string is too long 
 ** to fit in the pad, it is truncated.
 **************************************************************************/
int ficlVmGetWordToPad(ficlVm *vm)
{
  ficlString s;
  char *pad = (char *)vm->pad;
  s = ficlVmGetWord(vm);

  if (FICL_STRING_GET_LENGTH(s) > FICL_PAD_SIZE)
  {
    FICL_STRING_SET_LENGTH(s, FICL_PAD_SIZE);
  }

  strncpy(pad, FICL_STRING_GET_POINTER(s), FICL_STRING_GET_LENGTH(s));
  pad[FICL_STRING_GET_LENGTH(s)] = '\0';
  return (int)(FICL_STRING_GET_LENGTH(s));
}

/**************************************************************************
 **                      v m P a r s e S t r i n g
 ** Parses a string out of the input buffer using the delimiter
 ** specified. Skips leading delimiters, marks the start of the string,
 ** and counts characters to the next delimiter it encounters. It then 
 ** updates the vm input buffer to consume all these chars, including the
 ** trailing delimiter. 
 ** Returns the address and length of the parsed string, not including the
 ** trailing delimiter.
 **************************************************************************/
ficlString ficlVmParseString(ficlVm *vm, char delimiter)
{ 
  return ficlVmParseStringEx(vm, delimiter, 1);
}

ficlString ficlVmParseStringEx(ficlVm *vm, char delimiter, int skipLeadingDelimiters)
{
  ficlString s;
  char *trace = ficlVmGetInBuf(vm);
  char *stop  = ficlVmGetInBufEnd(vm);
  char c;

  if (skipLeadingDelimiters)
  {
    while ((trace != stop) && (*trace == delimiter))
      trace++;
  }

  FICL_STRING_SET_POINTER(s, trace);    /* mark start of text */

  for (c = *trace;
       (trace != stop) && (c != delimiter)
	 && (c != '\r') && (c != '\n');
       c = *++trace)
  {
    ;                   /* find next delimiter or end of line */
  }

  /* set length of result */
  FICL_STRING_SET_LENGTH(s, trace - FICL_STRING_GET_POINTER(s));

  if ((trace != stop) && (*trace == delimiter))     /* gobble trailing delimiter */
    trace++;

  ficlVmUpdateTib(vm, trace);
  return s;
}

/**************************************************************************
 **                       v m P o p
 ** 
 **************************************************************************/
ficlCell ficlVmPop(ficlVm *vm)
{
  return ficlStackPop(vm->dataStack);
}

/**************************************************************************
 **                      v m P u s h
 ** 
 **************************************************************************/
void ficlVmPush(ficlVm *vm, ficlCell c)
{
  ficlStackPush(vm->dataStack, c);
}

/**************************************************************************
 **                      v m P o p I P
 ** 
 **************************************************************************/
void ficlVmPopIP(ficlVm *vm)
{
  vm->ip = (ficlIp)(ficlStackPopPointer(vm->returnStack));
}

/**************************************************************************
 **                      v m P u s h I P
 ** 
 **************************************************************************/
void ficlVmPushIP(ficlVm *vm, ficlIp newIP)
{
  ficlStackPushPointer(vm->returnStack, (void *)vm->ip);
  vm->ip = newIP;
}

/**************************************************************************
 **                      v m P u s h T i b
 ** Binds the specified input string to the VM and clears >IN (the index)
 **************************************************************************/
void ficlVmPushTib(ficlVm *vm, char *text, ficlInteger nChars, ficlTIB *pSaveTib)
{
  if (pSaveTib)
  {
    *pSaveTib = vm->tib;
  }

  vm->tib.text = text;
  vm->tib.end = text + nChars;
  vm->tib.index = 0;
}

void ficlVmPopTib(ficlVm *vm, ficlTIB *pTib)
{
  if (pTib)
    vm->tib = *pTib;
}

/**************************************************************************
 **                        v m Q u i t
 ** 
 **************************************************************************/
void ficlVmQuit(ficlVm *vm)
{
  ficlStackReset(vm->returnStack);
  vm->restart     = 0;
  vm->ip          = NULL;
  vm->runningWord = NULL;
  vm->state       = FICL_VM_STATE_INTERPRET;
  vm->fth_catch_p = 0;
  vm->tib.text    = NULL;
  vm->tib.end     = NULL;
  vm->tib.index   = 0;
  vm->pad[0]      = '\0';
  vm->pad_eval[0] = '\0';
  CELL_INT_SET(&vm->sourceId, 0);
}

/**************************************************************************
 **                      v m R e s e t 
 ** 
 **************************************************************************/
void ficlVmReset(ficlVm *vm)
{
  ficlVmQuit(vm);
  ficlStackReset(vm->dataStack);
  vm->base = 10;
}

/**************************************************************************
 **                      v m S e t T e x t O u t
 ** Binds the specified output callback to the vm. If you pass NULL,
 ** binds the default output function (ficlTextOut)
 **************************************************************************/
void
ficlVmSetTextIn(ficlVm *vm, ficlInputFunction textIn)
{
  vm->callback.textIn = textIn;
}

void
ficlVmSetTextOut(ficlVm *vm, ficlOutputFunction textOut)
{
  vm->callback.textOut = textOut;
}

void
ficlVmSetErrorOut(ficlVm *vm, ficlOutputFunction errorOut)
{
  vm->callback.errorOut = errorOut;
}

/* ARGSUSED */
char *
ficlVmTextIn(ficlVm *vm)
{
  static char str[BUFSIZ];
  
  return fgets(str, BUFSIZ, stdin);
}

/* ARGSUSED */
void
ficlVmTextOut(ficlVm *vm, char *text)
{
  fputs(text, stdout);
}

/* ARGSUSED */
void
ficlVmErrorOut(ficlVm *vm, char *text)
{
  fputs(text, stderr);
}

/**************************************************************************
 **                      v m T h r o w
 ** 
 **************************************************************************/
void
ficlVmThrow(ficlVm *vm, int except)
{
	if (vm->exceptionHandler)
		longjmp(*(vm->exceptionHandler), except);
}

void
ficlVmThrowError(ficlVm *vm, const char *fmt, ...)
{
	va_list ap;

	/* [ms] */
	va_start(ap, fmt);
	ficlVmThrowErrorVararg(vm, FICL_VM_STATUS_ERROR_EXIT, fmt, ap);
	va_end(ap);
}

void
ficlVmThrowErrorVararg(ficlVm *vm, int exc, const char *fmt, va_list ap)
{
	FTH fs, ex;

	fs = fth_make_empty_string();
	ex = ficl_ans_real_exc(exc);
	if (FTH_NOT_FALSE_P(ex)) {
		fth_variable_set("*last-exception*", fth_last_exception = ex);
		fth_string_sformat(fs, "%s in %s: ",
		    ficl_ans_exc_name(exc), RUNNING_WORD_VM(vm));
	}
	fth_hit_error_p = false;
	if (fmt != NULL) {
		fth_string_vsformat(fs, fmt, ap);
		va_end(ap);
	} else if (FTH_NOT_FALSE_P(ex))
		fth_string_sformat(fs, "%s", ficl_ans_exc_msg(exc));
	fth_set_backtrace(ex);
	fth_exception_last_message_set(ex, fs);
	/* We don't come from fth-catch, do we? */
	if (!vm->fth_catch_p) {
		fth_hit_error_p = true;
		if (fth_eval_p)
			fth_errorf("\n");
		fth_errorf("#<%S>\n", fs);
		fth_show_backtrace(false);
		errno = 0;
		fth_reset_loop_and_depth();
		ficlVmReset(vm);
	}
	ficlVmThrow(vm, exc);
}

/* [ms] */
void
ficlVmThrowException(ficlVm *vm, int exc, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	ficlVmThrowErrorVararg(vm, exc, fmt, ap);
	va_end(ap);
}

/**************************************************************************
 **                  f i c l E v a l u a t e
 ** Wrapper for ficlExec() which sets SOURCE-ID to -1.
 **************************************************************************/
int ficlVmEvaluate(ficlVm *vm, char *buffer)
{
  int returnValue;

  returnValue = FICL_VM_STATUS_OUT_OF_TEXT;
  if (buffer != NULL)
  {
    ficlCell id = vm->sourceId;
    ficlString s;
    
    CELL_INT_SET(&vm->sourceId, -1);
    FICL_STRING_SET_FROM_CSTRING(s, buffer);
    returnValue = ficlVmExecuteString(vm, s);
    vm->sourceId = id;
  }
  return returnValue;
}

/**************************************************************************
 **                      f i c l E x e c
 ** Evaluates a block of input text in the context of the
 ** specified interpreter. Emits any requested output to the
 ** interpreter's output function.
 **
 ** Contains the "inner interpreter" code in a tight loop
 **
 ** Returns one of the VM_XXXX codes defined in ficl.h:
 ** VM_OUTOFTEXT is the normal exit condition
 ** VM_ERREXIT means that the interpreter encountered a syntax error
 **      and the vm has been reset to recover (some or all
 **      of the text block got ignored
 ** VM_USEREXIT means that the user executed the "bye" command
 **      to shut down the interpreter. This would be a good
 **      time to delete the vm, etc -- or you can ignore this
 **      signal.
 **************************************************************************/
int ficlVmExecuteString(ficlVm *vm, ficlString s)
{
  ficlSystem *volatile sys = vm->callback.system;
  ficlDictionary *volatile dict = sys->dictionary;
  volatile int except;
  jmp_buf vmState;
  jmp_buf *volatile oldState;
  volatile ficlTIB saveficlTIB;

  FICL_ASSERT(vm != NULL);
  FICL_ASSERT(sys->interpreterLoop[0] != NULL);

  ficlVmPushTib(vm,
		FICL_STRING_GET_POINTER(s),
		(ficlInteger)FICL_STRING_GET_LENGTH(s),
		(ficlTIB *)&saveficlTIB);
  /*
  ** Save and restore VM's jmp_buf to enable nested calls to ficlExec 
  */
  oldState = vm->exceptionHandler;
  vm->exceptionHandler = &vmState; /* This has to come before the setjmp! */
  except = setjmp(vmState);
    
  switch (except)
  {
  case 0:
    if (vm->restart)
    {
      vm->runningWord->code(vm);
      vm->restart = 0;
    }
    else			/* set VM up to interpret text */
      ficlVmPushIP(vm, &(sys->interpreterLoop[0]));

    ficlVmInnerLoop(vm, 0);
    break;

  case FICL_VM_STATUS_RESTART:
    vm->restart = 1;
    except = FICL_VM_STATUS_OUT_OF_TEXT;
    break;

  case FICL_VM_STATUS_OUT_OF_TEXT:
    ficlVmPopIP(vm);
    break;

  case FICL_VM_STATUS_SKIP_FILE:
  case FICL_VM_STATUS_USER_EXIT:
  case FICL_VM_STATUS_INNER_EXIT:
  case FICL_VM_STATUS_BREAK:
    break;

  case FICL_VM_STATUS_QUIT:
    if (vm->state == FICL_VM_STATE_COMPILE)
    {
      ficlDictionaryAbortDefinition(dict);
      ficlDictionaryEmpty(sys->locals, sys->locals->forthWordlist->size);
    }
    ficlVmQuit(vm);
    break;

  case FICL_VM_STATUS_ERROR_EXIT:
  case FICL_VM_STATUS_ABORT:
  case FICL_VM_STATUS_ABORTQ:
  default:    /* user defined exit code?? */
    if (vm->state == FICL_VM_STATE_COMPILE)
    {
      ficlDictionaryAbortDefinition(dict);
      ficlDictionaryEmpty(sys->locals, sys->locals->forthWordlist->size);
    }
    ficlDictionaryResetSearchOrder(dict);
    ficlVmReset(vm);
  break;
  }

  vm->exceptionHandler = oldState;
  ficlVmPopTib(vm, (ficlTIB *)&saveficlTIB);
  return (except);
}

/**************************************************************************
 **                      f i c l E x e c X T
 ** Given a pointer to a ficlWord, push an inner interpreter and
 ** execute the word to completion. This is in contrast with vmExecute,
 ** which does not guarantee that the word will have completed when
 ** the function returns (ie in the case of colon definitions, which
 ** need an inner interpreter to finish)
 **
 ** Returns one of the VM_XXXX exception codes listed in ficl.h. Normal
 ** exit condition is VM_INNEREXIT, Ficl's private signal to exit the
 ** inner loop under normal circumstances. If another code is thrown to
 ** exit the loop, this function will re-throw it if it's nested under
 ** itself or ficlExec.
 **
 ** NOTE: this function is intended so that C code can execute ficlWords
 ** given their address in the dictionary (xt).
 **************************************************************************/
int ficlVmExecuteXT(ficlVm *vm, ficlWord *pWord)
{
  volatile int except;
  jmp_buf vmState;
  jmp_buf *volatile oldState;
  ficlWord *volatile oldRunningWord;
    
  FICL_ASSERT(vm != NULL);
  FICL_ASSERT(vm->callback.system->exitInnerWord != NULL);
    
  /* 
  ** Save the runningword so that RESTART behaves correctly
  ** over nested calls.
  */
  oldRunningWord = vm->runningWord;
  /*
  ** Save and restore VM's jmp_buf to enable nested calls
  */
  oldState = vm->exceptionHandler;
  vm->exceptionHandler = &vmState; /* This has to come before the setjmp! */
  except = setjmp(vmState);

  if (except)
    ficlVmPopIP(vm);
  else
    ficlVmPushIP(vm, &(vm->callback.system->exitInnerWord));

  switch (except)
  {
  case 0:
  {
    pWord->current_word = oldRunningWord;
    pWord->current_file = fth_current_file;
    pWord->current_line = fth_current_line;
    ficlVmInnerLoop(vm, pWord);
    ficlVmInnerLoop(vm, 0);
    break;
  }

  case FICL_VM_STATUS_INNER_EXIT:
  case FICL_VM_STATUS_BREAK:
    break;

  case FICL_VM_STATUS_RESTART:
  case FICL_VM_STATUS_OUT_OF_TEXT:
  case FICL_VM_STATUS_SKIP_FILE:
  case FICL_VM_STATUS_USER_EXIT:
  case FICL_VM_STATUS_QUIT:
  case FICL_VM_STATUS_ERROR_EXIT:
  case FICL_VM_STATUS_ABORT:
  case FICL_VM_STATUS_ABORTQ:
  default:    /* user defined exit code?? */
    if (oldState)
    {
      vm->exceptionHandler = oldState;
      ficlVmThrow(vm, except);
    }
    break;
  }

  vm->exceptionHandler = oldState;
  vm->runningWord = oldRunningWord; 
  return (except);
}

/**************************************************************************
 **                       f i c l P a r s e N u m b e r
 ** Attempts to convert the NULL terminated string in the VM's pad to 
 ** a number using the VM's current base. If successful, pushes the number
 ** onto the param stack and returns FICL_TRUE. Otherwise, returns FICL_FALSE.
 ** (jws 8/01) Trailing decimal point causes a zero ficlCell to be pushed. (See
 ** the standard for DOUBLE wordset.
 **************************************************************************/

int
ficlVmParseNumber(ficlVm *vm, ficlString s)
{
  ficlInteger accumulator = 0;
  char isNegative = 0;
  char isDouble = 0;
  char *trace = FICL_STRING_GET_POINTER(s);
  ficlUnsigned length = FICL_STRING_GET_LENGTH(s);
  ficlUnsigned accu, digit, base = vm->base;
  int c;

  if (errno == ERANGE)
  {
    errno = 0;
    return FICL_FALSE;
  }

  if (length > 1)
  {
    switch (*trace)
    {
    case '-':
      trace++;
      length--;
      isNegative = 1;
      break;
    case '+':
      trace++;
      length--;
      isNegative = 0;
      break;
    default:
      break;
    }
  }

  /* detect & remove trailing decimal */
  if ((length > 0) && (trace[length - 1] == 'd'))
  {
    isDouble = 1;
    length--;
  }

  if (length == 0)        /* detect "+", "-", ".", "+." etc */
    return FICL_FALSE;

  accu = 0;
  while ((length--) && ((c = *trace++) != (int)'\0'))
  {
    if (!isalnum(c))
      return FICL_FALSE;

    digit = (ficlUnsigned)(c - '0');

    if (digit > 9)
      digit = (ficlUnsigned)(tolower(c) - 'a' + 10);

    if (digit >= base)
      return FICL_FALSE;

    accu = accu * base + digit;
  }
  accumulator = (ficlInteger)(isNegative ? -accu : accu);

  if (isDouble)
    ficlStackPushFTH(vm->dataStack, fth_make_llong((ficl2Integer)accumulator));
  else
    ficlStackPushInteger(vm->dataStack, accumulator);
    
  if (vm->state == FICL_VM_STATE_COMPILE)
    ficlPrimitiveLiteralIm(vm);
	
  return FICL_TRUE;
}

/**************************************************************************
 **                      d i c t C h e c k
 ** Checks the dictionary for corruption and throws appropriate
 ** errors.
 ** Input: +n number of ADDRESS UNITS (not ficlCells) proposed to allot
 **        -n number of ADDRESS UNITS proposed to de-allot
 **         0 just do a consistency check
 **************************************************************************/
void ficlVmDictionarySimpleCheck(ficlDictionary *dictionary, int cells)
{
  ficlVm *vm = FTH_FICL_VM();
  
  if ((cells >= 0) && ((ficlDictionaryCellsAvailable(dictionary) * (int)sizeof(ficlCell)) < cells))
    ficlVmThrowException(vm, FICL_VM_STATUS_DICT_OVERFLOW, NULL);
  if ((cells <= 0) && ((ficlDictionaryCellsUsed(dictionary) * (int)sizeof(ficlCell)) < -cells))
    ficlVmThrowError(vm, "dictionary underflow");
}

void ficlVmDictionaryCheck(ficlDictionary *dictionary, int cells)
{
  ficlVm *vm = FTH_FICL_VM();
  
  ficlVmDictionarySimpleCheck(dictionary, cells);

  if (dictionary->wordlistCount > FICL_MAX_WORDLISTS)
  {
    ficlDictionaryResetSearchOrder(dictionary);
    ficlVmThrowException(vm, FICL_VM_STATUS_SEARCH_OVERFLOW, NULL);
  }
  else if (dictionary->wordlistCount < 0)
  {
    ficlDictionaryResetSearchOrder(dictionary);
    ficlVmThrowException(vm, FICL_VM_STATUS_SEARCH_UNDERFLOW, NULL);
  }
}

void ficlVmDictionaryAllot(ficlDictionary *dictionary, int n)
{
  ficlVmDictionarySimpleCheck(dictionary, n);
  ficlDictionaryAllot(dictionary, n);
}


void ficlVmDictionaryAllotCells(ficlDictionary *dictionary, int cells)
{
  ficlVmDictionarySimpleCheck(dictionary, cells);
  ficlDictionaryAllotCells(dictionary, cells);
}

/**************************************************************************
 **                      f i c l P a r s e W o r d
 ** From the standard, section 3.4
 ** b) Search the dictionary name space (see 3.4.2). If a definition name
 ** matching the string is found: 
 **  1.if interpreting, perform the interpretation semantics of the definition
 **  (see 3.4.3.2), and continue at a); 
 **  2.if compiling, perform the compilation semantics of the definition
 **  (see 3.4.3.3), and continue at a). 
 **
 ** c) If a definition name matching the string is not found, attempt to
 ** convert the string to a number (see 3.4.1.3). If successful: 
 **  1.if interpreting, place the number on the data stack, and continue at a); 
 **  2.if compiling, FICL_VM_STATE_COMPILE code that when executed will place the number on
 **  the stack (see 6.1.1780 LITERAL), and continue at a); 
 **
 ** d) If unsuccessful, an ambiguous condition exists (see 3.4.4). 
 **
 ** (jws 4/01) Modified to be a ficlParseStep
 **************************************************************************/
int ficlVmParseWord(ficlVm *vm, ficlString name)
{
  ficlDictionary *dict = ficlVmGetDictionary(vm);
  ficlWord *tempFW;

  if (vm->callback.system->localsCount > 0)
    tempFW = ficlSystemLookupLocal(vm->callback.system, name);
  else
    tempFW = ficlDictionaryLookup(dict, name);
  
  if (tempFW != NULL)
  {
    if (vm->state == FICL_VM_STATE_INTERPRET)
    {
      if (ficlWordIsCompileOnly(tempFW))
	ficlVmThrowException(vm, FICL_VM_STATUS_COMPILE_ONLY, "%s", tempFW->name);

      tempFW->current_word = vm->runningWord;
      tempFW->current_file = fth_current_file;
      tempFW->current_line = fth_current_line;
      ficlVmInnerLoop(vm, tempFW);
      return FICL_TRUE;
    }
    else /* (vm->state == FICL_VM_STATE_COMPILE) */
    {
      if (ficlWordIsImmediate(tempFW))
      {
	tempFW->current_word = dict->smudge;
	tempFW->current_file = fth_current_file;
	tempFW->current_line = fth_current_line;
	ficlVmInnerLoop(vm, tempFW);
      }
      else
      {
	if (tempFW->flags & FICL_WORD_INSTRUCTION)
	  ficlDictionaryAppendPointer(dict, (void *)tempFW->code);
	else
	{
	  /* Preparation for word location, see also primitive.c.  [ms] */
	  ficlDictionaryAppendUnsigned(dict, (ficlUnsigned)ficl_word_location);
	  ficlDictionaryAppendPointer(dict, dict->smudge);
	  ficlDictionaryAppendFTH(dict, fth_string_copy(fth_current_file));
	  ficlDictionaryAppendInteger(dict, fth_current_line);
	  ficlDictionaryAppendPointer(dict, tempFW);
	}
      }
      return FICL_TRUE;
    }
  }
  return FICL_FALSE;
}
