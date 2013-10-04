/*******************************************************************
 ** s t a c k . c
 ** Forth Inspired Command Language
 ** Author: John Sadler (john_sadler@alum.mit.edu)
 ** Created: 16 Oct 1997
 ** @(#)stack.c	1.48 9/13/13
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

/*
 * Adapted to work with FTH:
 *
 * Copyright (c) 2004-2012 Michael Scholz <mi-scholz@users.sourceforge.net>
 *
 * This file is part of FTH.
 * 
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdlib.h>
#include "ficl.h"

#include "fth.h"
#include "utils.h"

#define STKDEPTH(s) (((s)->top - (s)->base) + 1)

/*
** N O T E: Stack convention:
**
** THIS CHANGED IN FICL 4.0!
**
** top points to the *current* top data value
** push: increment top, store value at top
** pop:  fetch value at top, decrement top
** Stack grows from low to high memory
*/

/*******************************************************************
 **                  v m C h e c k S t a c k
 ** Check the parameter stack for underflow or overflow.
 ** size controls the type of check: if size is zero,
 ** the function checks the stack state for underflow and overflow.
 ** If size > 0, checks to see that the stack has room to push
 ** that many cells. If less than zero, checks to see that the
 ** stack has room to pop that many cells. If any test fails,
 ** the function throws (via vmThrow) a VM_ERREXIT exception.
 *******************************************************************/
/*
 * [ms]
 * raises 'stack-underflow exception
 */
void
ficlStackCheck(ficlStack *stack, int popCells, int pushCells)
{
	ficlInteger depth, nFree, pop, push;

	pop = (ficlInteger)popCells;
	push = (ficlInteger)pushCells;
	depth = STKDEPTH(stack);
	nFree = (ficlInteger)stack->size - depth;
	if (pop > depth) {
		if (depth < 0)
			ficlVmThrowException(stack->vm,
			    FICL_VM_STATUS_STACK_UNDERFLOW,
			    "not enough arguments, at least %ld required",
			    pop + -depth);
		else
			ficlVmThrowException(stack->vm,
			    FICL_VM_STATUS_STACK_UNDERFLOW,
			    "not enough arguments, %ld instead of %ld",
			    depth,
			    pop);
	}
	if (nFree < (push - pop)) {
		if (stack->name != NULL && strcmp(stack->name, "data") == 0)
			ficlVmThrowException(stack->vm,
			    FICL_VM_STATUS_STACK_OVERFLOW, NULL);
		else
			ficlVmThrowException(stack->vm,
			    FICL_VM_STATUS_RSTACK_OVERFLOW, NULL);
	}
}

/*******************************************************************
 **                  s t a c k C r e a t e
 ** 
 *******************************************************************/
ficlStack *ficlStackCreate(ficlVm *vm, char *name, unsigned size)
{
  size_t totalSize = sizeof(ficlStack) + (size * sizeof(ficlCell));
  ficlStack *stack;

  FICL_ASSERT(size > 0);
  stack = memset(FTH_MALLOC(totalSize), 0, totalSize);
  
  stack->size = size;
  stack->frame = NULL;

  stack->vm = vm;
  stack->name = name;
    
  ficlStackReset(stack);
  return stack;
}

/*******************************************************************
 **                  s t a c k D e p t h 
 ** 
 *******************************************************************/
int ficlStackDepth(ficlStack *stack)
{
  return (int)STKDEPTH(stack);
}

/*******************************************************************
 **                  s t a c k D r o p
 ** 
 *******************************************************************/
void ficlStackDrop(ficlStack *stack, int n)
{
  if (n > 0)
    stack->top -= n;
}

/*******************************************************************
 **                  s t a c k F e t c h
 ** 
 *******************************************************************/
ficlCell ficlStackFetch(ficlStack *stack, int n)
{
  return stack->top[-n];
}

void ficlStackStore(ficlStack *stack, int n, ficlCell c)
{
  stack->top[-n] = c;
}

/*******************************************************************
 **                    s t a c k G e t T o p
 ** 
 *******************************************************************/
ficlCell ficlStackGetTop(ficlStack *stack)
{
  return stack->top[0];
}

/*******************************************************************
 **                  s t a c k L i n k
 ** Link a frame using the stack's frame pointer. Allot space for
 ** size cells in the frame
 ** 1) Push frame
 ** 2) frame = top
 ** 3) top += size
 *******************************************************************/
void ficlStackLink(ficlStack *stack, int size)
{
  ficlStackPushPointer(stack, stack->frame);
  stack->frame = stack->top + 1;
  stack->top += size;
}

/*******************************************************************
 **                  s t a c k U n l i n k
 ** Unlink a stack frame previously created by stackLink
 ** 1) top = frame
 ** 2) frame = pop()
 *******************************************************************/
void ficlStackUnlink(ficlStack *stack)
{
  stack->top = stack->frame - 1;
  stack->frame = ficlStackPopPointer(stack);

}

/*******************************************************************
 **                  s t a c k P i c k
 ** 
 *******************************************************************/
void ficlStackPick(ficlStack *stack, int n)
{
  ficlCell cell;

  cell = stack->top[-n];
  *++stack->top = cell; 
  /*
   * With
   * *++stack->top = stack->top[-n];
   * gcc45+ complains:
   * warning: operation on `stack->top' may be undefined [-Wsequence-point]
   */
}

/*******************************************************************
 **                  s t a c k P o p
 ** 
 *******************************************************************/
ficlCell ficlStackPop(ficlStack *stack)
{
  return *stack->top--;
}

ficlInteger ficlStackPopInteger(ficlStack *stack)
{
  ficlInteger i = STACK_INT_REF(stack);

  stack->top--;
  return i;
}

ficlUnsigned ficlStackPopUnsigned(ficlStack *stack)
{
  ficlUnsigned u = STACK_UINT_REF(stack);

  stack->top--;
  return u;
}

ficl2Integer ficlStackPop2Integer(ficlStack *stack)
{
  ficl2Integer di = STACK_LONG_REF(stack);

  stack->top--;
  return di;
}

ficl2Unsigned ficlStackPop2Unsigned(ficlStack *stack)
{
  ficl2Unsigned ud = STACK_ULONG_REF(stack);

  stack->top--;
  return ud;
}

bool ficlStackPopBoolean(ficlStack *stack)
{
  bool i = STACK_BOOL_REF(stack);

  stack->top--;
  return i;
}

void *ficlStackPopPointer(ficlStack *stack)
{
  void *p = STACK_VOIDP_REF(stack);

  stack->top--;
  return p;
}

FTH ficlStackPopFTH(ficlStack *stack)
{
  FTH fp = STACK_FTH_REF(stack);

  stack->top--;
  return fp;
}

ficlFloat ficlStackPopFloat(ficlStack *stack)
{
  ficlFloat f = STACK_FLOAT_REF(stack);

  stack->top--;
  return f;
}
/*******************************************************************
 **                  s t a c k P u s h
 ** 
 *******************************************************************/
void ficlStackPush(ficlStack *stack, ficlCell c)
{
  *++stack->top = c;
}

void ficlStackPushInteger(ficlStack *stack, ficlInteger i)
{
  ++stack->top;
  STACK_INT_SET(stack, i);
}

void ficlStackPushUnsigned(ficlStack *stack, ficlUnsigned u)
{
  ++stack->top;
  STACK_UINT_SET(stack, u);
}
void ficlStackPush2Integer(ficlStack *stack, ficl2Integer di)
{
  ++stack->top;
  if (FIXABLE_P(di))
    STACK_INT_SET(stack, di);
  else
    STACK_LONG_SET(stack, di);
}

void ficlStackPush2Unsigned(ficlStack *stack, ficl2Unsigned ud)
{
  ++stack->top;
  if (POSFIXABLE_P((long)ud))
    STACK_UINT_SET(stack, ud);
  else
    STACK_ULONG_SET(stack, ud);
}

void ficlStackPushBoolean(ficlStack *stack, bool b)
{
  ++stack->top;
  STACK_BOOL_SET(stack, b);
}

void ficlStackPushPointer(ficlStack *stack, void *p)
{
  ++stack->top;
  STACK_VOIDP_SET(stack, p);
}

void ficlStackPushFTH(ficlStack *stack, FTH fp)
{
  ++stack->top;
  STACK_FTH_SET(stack, fp);
}

void ficlStackPushFloat(ficlStack *stack, ficlFloat f)
{
  ++stack->top;
  STACK_FLOAT_SET(stack, f);
}

/*******************************************************************
 **                  s t a c k R e s e t
 ** 
 *******************************************************************/
void ficlStackReset(ficlStack *stack)
{
  stack->top = stack->base - 1;
}

/*******************************************************************
 **                  s t a c k R o l l 
 ** Roll nth stack entry to the top (counting from zero), if n is 
 ** >= 0. Drop other entries as needed to fill the hole.
 ** If n < 0, roll top-of-stack to nth entry, pushing others
 ** upward as needed to fill the hole.
 *******************************************************************/
void ficlStackRoll(ficlStack *stack, int n)
{
  ficlCell c;
  ficlCell *cell;

  if (n == 0)
    return;
  
  else if (n > 0)
  {
    cell = stack->top - n;
    c = *cell;

    for (;n > 0; --n, cell++)
      *cell = cell[1];

    *cell = c;
  }
  else
  {
    cell = stack->top;
    c = *cell;

    for (; n < 0; ++n, cell--)
      *cell = cell[-1];

    *cell = c;
  }
}

/*******************************************************************
 **                  s t a c k S e t T o p
 ** 
 *******************************************************************/
void ficlStackSetTop(ficlStack *stack, ficlCell c)
{
  FICL_STACK_CHECK(stack, 1, 1);
  stack->top[0] = c;
}

void ficlStackWalk(ficlStack *stack,
		   ficlStackWalkFunction callback,
		   void *context,
		   ficlInteger bottomToTop)
{
  int i;
  int depth;
  ficlCell *cell;
    
  FICL_STACK_CHECK(stack, 0, 0);

  depth = ficlStackDepth(stack);
  cell = bottomToTop ? stack->base : stack->top;

  for (i = 0; i < depth; i++)
  {
    if (callback(context, cell) == FICL_FALSE)
      break;

    cell += bottomToTop ? 1 : -1;
  }
}
