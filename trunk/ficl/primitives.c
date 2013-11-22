/*******************************************************************
** w o r d s . c
** Forth Inspired Command Language
** ANS Forth CORE word-set written in C
** Author: John Sadler (john_sadler@alum.mit.edu)
** Created: 19 July 1997
** $Id: primitives.c,v 1.4 2010/09/13 18:43:04 asau Exp $
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
 * @(#)primitives.c	1.107 11/22/13
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "ficl.h"

#include "fth.h"
#include "utils.h"

/*
** Control structure building words use these
** strings' addresses as markers on the stack to 
** check for structure completion.
*/
static char doTag[] = "do";
static char colonTag[] = "colon";
static char leaveTag[] = "leave";

static char destTag[] = "target";
static char origTag[] = "origin";

static char caseTag[] = "case";
static char ofTag[] = "of";
static char fallthroughTag[] = "fallthrough";

#define PRIMITIVE_APPEND_UNSIGNED(Dict, Inst)				\
	ficlDictionaryAppendUnsigned(Dict, (ficlUnsigned)(Inst))
/*
** C O N T R O L   S T R U C T U R E   B U I L D E R S
**
** Push current dictionary location for later branch resolution.
** The location may be either a branch target or a patch address...
*/
static void markBranch(ficlDictionary *dict, ficlVm *vm, char *tag)
{
  ficlStackPushPointer(vm->dataStack, dict->here);
  ficlStackPushPointer(vm->dataStack, tag);
}

static void markControlTag(ficlVm *vm, char *tag)
{
  ficlStackPushPointer(vm->dataStack, tag);
}

static void matchControlTag(ficlVm *vm, char *wantTag)
{
  char *tag;

  tag = (char *)ficlStackPopPointer(vm->dataStack);
  /*
  ** Changed the code below to compare the pointers first (by popular demand)
  */
  if ((tag != wantTag) && tag && *tag && strcmp(tag, wantTag))
    ficlVmThrowException(vm,
	FICL_VM_STATUS_CONTROL_MISMATCH,
	"unmatched control structure \"%s\"", wantTag);
}

/*
** Expect a branch target address on the param stack,
** FICL_VM_STATE_COMPILE a literal offset from the current dictionary location
** to the target address
*/
static void resolveBackBranch(ficlDictionary *dict, ficlVm *vm, char *tag)
{
  ficlInteger offset;
  ficlCell *patchAddr;

  matchControlTag(vm, tag);
  FICL_STACK_CHECK(vm->dataStack, 1, 0);
  patchAddr = (ficlCell *)ficlStackPopPointer(vm->dataStack);
  offset = patchAddr - dict->here;
  ficlDictionaryAppendInteger(dict, offset);
}

/*
** Expect a branch patch address on the param stack,
** FICL_VM_STATE_COMPILE a literal offset from the patch location
** to the current dictionary location
*/
static void resolveForwardBranch(ficlDictionary *dict, ficlVm *vm, char *tag)
{
  ficlInteger offset;
  ficlCell *patchAddr;

  matchControlTag(vm, tag);
  FICL_STACK_CHECK(vm->dataStack, 1, 0);
  patchAddr = (ficlCell *)ficlStackPopPointer(vm->dataStack);
  offset = dict->here - patchAddr;
  CELL_INT_SET(patchAddr, offset);
}

/*
** Match the tag to the top of the stack. If success,
** sopy "here" address into the ficlCell whose address is next
** on the stack. Used by do..leave..loop.
*/
static void resolveAbsBranch(ficlDictionary *dict, ficlVm *vm, char *wantTag)
{
  ficlCell *patchAddr;
  char *tag;

  tag = ficlStackPopPointer(vm->dataStack);
  /*
  ** Changed the comparison below to compare the pointers first (by
  ** popular demand)
  */
  if ((tag != wantTag) && tag && *tag && strcmp(tag, wantTag))
    ficlVmThrowException(vm,
	FICL_VM_STATUS_CONTROL_MISMATCH,
	"unmatched control structure \"%s\"", wantTag);
  patchAddr = (ficlCell *)ficlStackPopPointer(vm->dataStack);
  CELL_VOIDP_SET(patchAddr, dict->here);
}

/**************************************************************************
 **                      c o l o n   d e f i n i t i o n s
 ** Code to begin compiling a colon definition
 ** This function sets the state to FICL_VM_STATE_COMPILE, then creates a
 ** new word whose name is the next word in the input stream
 ** and whose code is colonParen.
 **************************************************************************/
static void ficlPrimitiveColon(ficlVm *vm)
{
#define h_ficlPrimitiveColon "\
Code to begin compiling a colon definition.  \
This function sets the state to FICL_VM_STATE_COMPILE, then creates a \
new word whose name is the next word in the input stream \
and whose code is colonParen."
  ficlDictionary *dict;
  ficlString s;

  dict = ficlVmGetDictionary(vm);
  s = ficlVmGetWord(vm);
  vm->state = FICL_VM_STATE_COMPILE;
  markControlTag(vm, colonTag);
  ficlDictionaryAppendWord(dict,
      s,
      (ficlPrimitive)ficlInstructionColonParen,
      (ficlUnsigned)(FICL_WORD_DEFAULT | FICL_WORD_SMUDGED));
  vm->callback.system->localsCount = 0;
}

static void ficlPrimitiveSemicolonCoIm(ficlVm *vm)
{
  ficlDictionary *dict;

  dict = ficlVmGetDictionary(vm);
  matchControlTag(vm, colonTag);
  if (vm->callback.system->localsCount > 0)
  {
    ficlDictionary *locals;

    locals = ficlSystemGetLocals(vm->callback.system);
    ficlDictionaryEmpty(locals, locals->forthWordlist->size);
    PRIMITIVE_APPEND_UNSIGNED(dict, ficlInstructionUnlinkParen);
  }
  vm->callback.system->localsCount = 0;
  PRIMITIVE_APPEND_UNSIGNED(dict, ficlInstructionSemiParen);
  vm->state = FICL_VM_STATE_INTERPRET;
  ficlDictionaryUnsmudge(dict);
}

/**************************************************************************
 **                      e x i t
 ** CORE
 ** This function simply pops the previous instruction
 ** pointer and returns to the "next" loop. Used for exiting from within
 ** a definition. Note that exitParen is identical to semiParen - they
 ** are in two different functions so that "see" can correctly identify
 ** the end of a colon definition, even if it uses "exit".
 **************************************************************************/
static void ficlPrimitiveExitCoIm(ficlVm *vm)
{
#define h_ficlPrimitiveExitCoIm "\
This function simply pops the previous instruction \
pointer and returns to the NEXT loop.  \
Used for exiting from within a definition.  \
Note that exitParen is identical to semiParen---they \
are in two different functions so that SEE can correctly identify \
the end of a colon definition, even if it uses EXIT."
  ficlDictionary *dict = ficlVmGetDictionary(vm);

  if (vm->callback.system->localsCount > 0)
    PRIMITIVE_APPEND_UNSIGNED(dict, ficlInstructionUnlinkParen);
  PRIMITIVE_APPEND_UNSIGNED(dict, ficlInstructionExitParen);
}

/**************************************************************************
 **                      c o n s t a n t
 ** IMMEDIATE
 ** Compiles a constant into the dictionary. Constants return their
 ** value when invoked. Expects a value on top of the parm stack.
 **************************************************************************/
static void ficlPrimitiveConstant(ficlVm *vm)
{
#define h_ficlPrimitiveConstant "\
Compiles a constant into the dictionary.  \
Constants return their value when invoked.  \
Expects a value on top of the parm stack."
  ficlDictionary *dict = ficlVmGetDictionary(vm);
  ficlString s = ficlVmGetWord(vm);

  FICL_STACK_CHECK(vm->dataStack, 1, 0);
  ficlDictionaryAppendConstantInstruction(dict,
    s, (ficlUnsigned)ficlInstructionConstantParen,
    ficlStackPopInteger(vm->dataStack));
}

/**************************************************************************
 **                      d i s p l a y C e l l
 ** Drop and print the contents of the ficlCell at the top of the param
 ** stack
 **************************************************************************/
static void ficlPrimitiveDot(ficlVm *vm)
{
#define h_ficlPrimitiveDot "( x -- )  \
Drop and print the contents of the ficlCell at the top of the param stack."
  ficlCell c;

  FICL_STACK_CHECK(vm->dataStack, 1, 0);
  c = ficlStackPop(vm->dataStack);

  if (fth_instance_p(CELL_FTH_REF(&c)))
    fth_printf("%M ", CELL_FTH_REF(&c));
  else
    fth_printf("%s ", ficlLtoa(CELL_INT_REF(&c), vm->pad, (int)vm->base));
}

static void ficlPrimitiveUDot(ficlVm *vm)
{
#define h_ficlPrimitiveUDot "( x -- )  \
Drop and print the contents of the ficlCell at the top of the param stack as unsigned."
  FICL_STACK_CHECK(vm->dataStack, 1, 0);
  fth_printf("%s ", ficlUltoa(ficlStackPopUnsigned(vm->dataStack), vm->pad, (int)vm->base));
}

/* [ms] b. o. d. added */
static void ficlPrimitiveBinDot(ficlVm *vm)
{
#define h_ficlPrimitiveBinDot "( x -- )  \
Drop and print the contents of the ficlCell at the top of the param stack in binary format."
  FICL_STACK_CHECK(vm->dataStack, 1, 0);
  fth_printf("%s ", ficlUltoa(ficlStackPopUnsigned(vm->dataStack), vm->pad, 2));
}

static void ficlPrimitiveOctDot(ficlVm *vm)
{
#define h_ficlPrimitiveOctDot "( x -- )  \
Drop and print the contents of the ficlCell at the top of the param stack in octal format."
  FICL_STACK_CHECK(vm->dataStack, 1, 0);
  fth_printf("%s ", ficlUltoa(ficlStackPopUnsigned(vm->dataStack), vm->pad, 8));
}

static void ficlPrimitiveHexDot(ficlVm *vm)
{
#define h_ficlPrimitiveHexDot "( x -- )  \
Drop and print the contents of the ficlCell at the top of the param stack in hex format."
  FICL_STACK_CHECK(vm->dataStack, 1, 0);
  fth_printf("%s ", ficlUltoa(ficlStackPopUnsigned(vm->dataStack), vm->pad, 16));
}

/**************************************************************************
 **                      s t r l e n
 ** Ficl   ( c-string -- length )
 **
 ** Returns the length of a C-style (zero-terminated) string.
 **
 ** --lch
 **/
static void ficlPrimitiveStrlen(ficlVm *vm)
{
#define h_ficlPrimitiveStrlen "( c-string -- length )  \
Returns the length of a C-style (zero-terminated) string."
  char *address = (char *)ficlStackPopPointer(vm->dataStack);

  ficlStackPushInteger(vm->dataStack, (ficlInteger)(address ? strlen(address) : 0));
}

/**************************************************************************
 **                      s p r i n t f
 ** Ficl   ( i*x c-addr-fmt u-fmt c-addr-buffer u-buffer -- c-addr-buffer u-written success-flag )
 ** Similar to the C sprintf() function.  It formats into a buffer based on
 ** a "format" string.  Each character in the format string is copied verbatim
 ** to the output buffer, until SPRINTF encounters a percent sign ("%").
 ** SPRINTF then skips the percent sign, and examines the next character
 ** (the "format character").  Here are the valid format characters:
 **    s - read a C-ADDR U-LENGTH string from the stack and copy it to
 **        the buffer
 **    d - read a ficlCell from the stack, format it as a string (base-10,
 **        signed), and copy it to the buffer
 **    x - same as d, except in base-16
 **    u - same as d, but unsigned
 **    % - output a literal percent-sign to the buffer
 ** SPRINTF returns the c-addr-buffer argument unchanged, the number of bytes
 ** written, and a flag indicating whether or not it ran out of space while
 ** writing to the output buffer (FICL_TRUE if it ran out of space).
 **
 ** If SPRINTF runs out of space in the buffer to store the formatted string,
 ** it still continues parsing, in an effort to preserve your stack (otherwise
 ** it might leave uneaten arguments behind).
 **
 ** --lch
 **************************************************************************/
static void ficlPrimitiveSprintf(ficlVm *vm) /*  */
{
#define h_ficlPrimitiveSprintf "( i*x c-addr-fmt u-fmt c-addr-buffer u-buffer -- c-addr-buffer u-written success-flag )  \
Similar to the C sprintf() function.\n\
100 allocate drop value buf\n\
10 s\" hello %d\" buf 100 sprintf drop type => hello 10\n\
$\" bucks\" string>$ 10 s\" hello %d %s\" buf 100 sprintf drop type => hello 10 bucks\n\
It formats into a buffer based on a FORMAT string.  \
Each character in the format string is copied verbatim \
to the output buffer, until SPRINTF encounters a percent sign ('%').  \
SPRINTF then skips the percent sign, and examines the next character (the \"format character\").  \
Here are the valid format characters:\n\
   s - read a C-ADDR U-LENGTH string from the stack and copy it to the buffer\n\
   d - read a ficlCell from the stack, format it as a string (base-10, signed), \
and copy it to the buffer\n\
   x - same as d, except in base-16\n\
   u - same as d, but unsigned\n\
   % - output a literal percent-sign to the buffer\n\
SPRINTF returns the c-addr-buffer argument unchanged, the number of bytes \
written, and a flag indicating whether or not it ran out of space while \
writing to the output buffer (FICL_TRUE if it ran out of space).\n\
If SPRINTF runs out of space in the buffer to store the formatted string, \
it still continues parsing, in an effort to preserve your stack (otherwise \
it might leave uneaten arguments behind)."
  int bufferLength = (int)ficlStackPopInteger(vm->dataStack);
  char *buffer = (char *)ficlStackPopPointer(vm->dataStack);
  char *bufferStart = buffer;

  int formatLength = (int)ficlStackPopInteger(vm->dataStack);
  char *format = (char *)ficlStackPopPointer(vm->dataStack);
  char *formatStop = format + formatLength;

  int base = 10;
  int unsignedInteger = FICL_FALSE;

  ficlInteger append = FICL_TRUE;

  while (format < formatStop)
  {
    char scratch[64];
    char *source = NULL;
    int actualLength = 0;
    int desiredLength;
    int leadingZeroes;

    if (*format != '%')
    {
      source = format;
      actualLength = desiredLength = 1;
      leadingZeroes = 0;
    }
    else
    {
      format++;
      if (format == formatStop)
	break;

      leadingZeroes = (*format == '0');
      if (leadingZeroes)
      {
	format++;
	if (format == formatStop)
	  break;
      }

      desiredLength = isdigit((int)*format);
      if (desiredLength)
      {
	desiredLength = (int)strtoul(format, &format, 10);
	if (format == formatStop)
	  break;
      }
      else if (*format == '*')
      {
	desiredLength = (int)ficlStackPopInteger(vm->dataStack);
	format++;
	if (format == formatStop)
	  break;
      }

      switch (*format)
      {
      case 's':
      case 'S':
      {
	actualLength = (int)ficlStackPopInteger(vm->dataStack);
	source = (char *)ficlStackPopPointer(vm->dataStack);
	break;
      }
      case 'x':
      case 'X':
	base = 16;
	/* fall through */
      case 'u':
      case 'U':
	unsignedInteger = FICL_TRUE;
	/* fall through */
      case 'd':
      case 'D':
      {
	ficlInteger integer = ficlStackPopInteger(vm->dataStack);

	if (unsignedInteger)
	  ficlUltoa((ficlUnsigned)integer, scratch, base);
	else
	  ficlLtoa(integer, scratch, base);
	base = 10;
	unsignedInteger = FICL_FALSE;
	source = scratch;
	actualLength = (int)strlen(scratch);
	break;
      }
      case '%':
	source = format;
	actualLength = 1;
	break;
      default:
	continue;
      }
    }

    if (append == FICL_TRUE)
    {
      if (!desiredLength)
	desiredLength = actualLength;

      if (desiredLength > bufferLength)
      {
	append = FICL_FALSE;
	desiredLength = bufferLength;
      }

      while (desiredLength > actualLength)
      {
	*buffer++ = (char)((leadingZeroes) ? '0' : ' ');
	bufferLength--;
	desiredLength--;
      }

      if (source)
	memcpy(buffer, source, (size_t)actualLength);

      buffer += actualLength;
      bufferLength -= actualLength;
    }
    format++;
  }

  ficlStackPushPointer(vm->dataStack, bufferStart);
  ficlStackPushInteger(vm->dataStack, buffer - bufferStart);
  ficlStackPushInteger(vm->dataStack, append);
}

/**************************************************************************
 **                      d u p   &   f r i e n d s
 ** 
 **************************************************************************/
static void ficlPrimitiveDepth(ficlVm *vm)
{
  int i;

  FICL_STACK_CHECK(vm->dataStack, 0, 1);
  i = ficlStackDepth(vm->dataStack);
  ficlStackPushInteger(vm->dataStack, (ficlInteger)i);
}

/**************************************************************************
 **                      e m i t   &   f r i e n d s
 ** 
 **************************************************************************/
static void ficlPrimitiveEmit(ficlVm *vm)
{
  FICL_STACK_CHECK(vm->dataStack, 1, 0);
  fth_printf("%c", (int)ficlStackPopInteger(vm->dataStack));
}

/* ARGSUSED */
static void ficlPrimitiveCR(ficlVm *vm)
{
  fth_print("\n");
}

static void ficlPrimitiveBackslash(ficlVm *vm)
{
  char *trace = ficlVmGetInBuf(vm);
  char *stop = ficlVmGetInBufEnd(vm);
  char c = *trace;

  while ((trace != stop) && (c != '\r') && (c != '\n'))
    c = *++trace;

  /*
  ** Cope with DOS or UNIX-style EOLs -
  ** Check for /r, /n, /r/n, or /n/r end-of-line sequences,
  ** and point trace to next char. If EOL is \0, we're done.
  */
  if (trace != stop)
  {
    trace++;

    if ( (trace != stop) && (c != *trace) 
	 && ((*trace == '\r') || (*trace == '\n')) )
      trace++;
  }

  ficlVmUpdateTib(vm, trace);
}

/*
** paren CORE 
** Compilation: Perform the execution semantics given below.
** Execution: ( "ccc<paren>" -- )
** Parse ccc delimited by ) (right parenthesis). ( is an immediate word. 
** The number of characters in ccc may be zero to the number of characters
** in the parse area. 
** 
*/
static void ficlPrimitiveParenthesis(ficlVm *vm)
{
#define h_ficlPrimitiveParenthesis "( \"ccc<paren>\" -- )  \
Parse ccc delimited by ) (right parenthesis).  \
( is an immediate word.  \
The number of characters in ccc may be zero to the number of characters in the parse area."
  ficlVmParseStringEx(vm, ')', 0);
}

/**************************************************************************
 **                      F E T C H   &   S T O R E
 ** 
 **************************************************************************/

/**************************************************************************
 **                      i f C o I m
 ** IMMEDIATE
 ** Compiles code for a conditional branch into the dictionary
 ** and pushes the branch patch address on the stack for later
 ** patching by ELSE or THEN/ENDIF. 
 **************************************************************************/
static void ficlPrimitiveIfCoIm(ficlVm *vm)
{
#define h_ficlPrimitiveIfCoIm "\
Compiles code for a conditional branch into the dictionary \
and pushes the branch patch address on the stack for later \
patching by ELSE or THEN/ENDIF."
  ficlDictionary *dict = ficlVmGetDictionary(vm);

  PRIMITIVE_APPEND_UNSIGNED(dict, ficlInstructionBranch0ParenWithCheck);
  markBranch(dict, vm, origTag);
  PRIMITIVE_APPEND_UNSIGNED(dict, 1);
}

/**************************************************************************
 **                      e l s e C o I m
 ** 
 ** IMMEDIATE -- compiles an "else"...
 ** 1) FICL_VM_STATE_COMPILE a branch and a patch address; the address gets patched
 **    by "endif" to point past the "else" code.
 ** 2) Pop the the "if" patch address
 ** 3) Patch the "if" branch to point to the current FICL_VM_STATE_COMPILE address.
 ** 4) Push the "else" patch address. ("endif" patches this to jump past 
 **    the "else" code.
 **************************************************************************/
static void ficlPrimitiveElseCoIm(ficlVm *vm)
{
#define h_ficlPrimitiveElseCoIm "\
1) FICL_VM_STATE_COMPILE a branch and a patch address; the address gets patched \
by THEN/ENDIF to point past the ELSE code.\n\
2) Pop the the IF patch address.\n\
3) Patch the IF branch to point to the current FICL_VM_STATE_COMPILE address.\n\
4) Push the ELSE patch address. THEN/ENDIF patches this to jump past the ELSE code."
  ficlCell *patchAddr;
  ficlInteger offset;
  ficlDictionary *dict = ficlVmGetDictionary(vm);

  /* (1) FICL_VM_STATE_COMPILE branch runtime */
  PRIMITIVE_APPEND_UNSIGNED(dict, ficlInstructionBranchParenWithCheck);
  matchControlTag(vm, origTag);
  patchAddr = (ficlCell *)ficlStackPopPointer(vm->dataStack); /* (2) pop "if" patch addr */
  markBranch(dict, vm, origTag); /* (4) push "else" patch addr */
  PRIMITIVE_APPEND_UNSIGNED(dict, 1); /* (1) FICL_VM_STATE_COMPILE patch placeholder */
  offset = dict->here - patchAddr;
  CELL_INT_SET(patchAddr, offset);
}

/**************************************************************************
 **                      e n d i f C o I m
 ** 
 **************************************************************************/
static void ficlPrimitiveEndifCoIm(ficlVm *vm)
{
  resolveForwardBranch(ficlVmGetDictionary(vm), vm, origTag);
}

/**************************************************************************
 **                      c a s e C o I m
 ** IMMEDIATE FICL_VM_STATE_COMPILE-ONLY
 **
 **
 ** At FICL_VM_STATE_COMPILE-time, a CASE-SYS (see DPANS94 6.2.0873) looks like this:
 **			i*addr i caseTag
 ** and an OF-SYS (see DPANS94 6.2.1950) looks like this:
 **			i*addr i caseTag addr ofTag
 ** The integer under caseTag is the count of fixup addresses that branch
 ** to ENDCASE.
 **************************************************************************/
static void ficlPrimitiveCaseCoIm(ficlVm *vm)
{
  FICL_STACK_CHECK(vm->dataStack, 0, 2);
  ficlStackPushUnsigned(vm->dataStack, 0UL);
  markControlTag(vm, caseTag);
}

/**************************************************************************
 **                      e n d c a s eC o I m
 ** IMMEDIATE FICL_VM_STATE_COMPILE-ONLY
 **************************************************************************/
static void ficlPrimitiveEndcaseCoIm(ficlVm *vm)
{
  ficlUnsigned fixupCount;
  ficlDictionary *dict;
  ficlInteger offset;
  ficlCell *patchAddr, c = ficlStackGetTop(vm->dataStack);

  /*
  ** if the last OF ended with FALLTHROUGH,
  ** just add the FALLTHROUGH fixup to the
  ** ENDOF fixups
  */
  
  if (CELL_VOIDP_REF(&c) == fallthroughTag)
  {
    matchControlTag(vm, fallthroughTag);
    patchAddr = ficlStackPopPointer(vm->dataStack);
    matchControlTag(vm, caseTag);
    fixupCount = ficlStackPopUnsigned(vm->dataStack);
    ficlStackPushPointer(vm->dataStack, patchAddr);
    ficlStackPushUnsigned(vm->dataStack, fixupCount + 1);
    markControlTag(vm, caseTag);
  }

  matchControlTag(vm, caseTag);
  FICL_STACK_CHECK(vm->dataStack, 1, 0);
  fixupCount = ficlStackPopUnsigned(vm->dataStack);
  FICL_STACK_CHECK(vm->dataStack, (int)fixupCount, 0);
  dict = ficlVmGetDictionary(vm);
  PRIMITIVE_APPEND_UNSIGNED(dict, ficlInstructionDrop);

  while (fixupCount--)
  {
    patchAddr = (ficlCell *)ficlStackPopPointer(vm->dataStack);
    offset = dict->here - patchAddr;
    CELL_INT_SET(patchAddr, offset);
  }
}

/**************************************************************************
 **                      o f C o I m
 ** IMMEDIATE FICL_VM_STATE_COMPILE-ONLY
 **************************************************************************/
static void ficlPrimitiveOfCoIm(ficlVm *vm)
{
  ficlDictionary *dict = ficlVmGetDictionary(vm);
  ficlCell *fallthroughFixup = NULL, c = ficlStackGetTop(vm->dataStack);

  FICL_STACK_CHECK(vm->dataStack, 1, 3);

  if (CELL_VOIDP_REF(&c) == fallthroughTag)
  {
    matchControlTag(vm, fallthroughTag);
    fallthroughFixup = ficlStackPopPointer(vm->dataStack);
  }

  matchControlTag(vm, caseTag);
  markControlTag(vm, caseTag);
  PRIMITIVE_APPEND_UNSIGNED(dict, ficlInstructionOfParen);
  markBranch(dict, vm, ofTag);
  PRIMITIVE_APPEND_UNSIGNED(dict, 2);

  if (fallthroughFixup != NULL)
    CELL_INT_SET(fallthroughFixup, dict->here - fallthroughFixup);
}

/**************************************************************************
 **                  e n d o f C o I m
 ** IMMEDIATE FICL_VM_STATE_COMPILE-ONLY
 **************************************************************************/
static void ficlPrimitiveEndofCoIm(ficlVm *vm)
{
  ficlCell *patchAddr;
  ficlUnsigned fixupCount;
  ficlInteger offset;
  ficlDictionary *dict = ficlVmGetDictionary(vm);

  FICL_STACK_CHECK(vm->dataStack, 4, 3);

  /* ensure we're in an OF, */
  matchControlTag(vm, ofTag);
  /* grab the address of the branch location after the OF */
  patchAddr = (ficlCell *)ficlStackPopPointer(vm->dataStack);
  /* ensure we're also in a "case" */
  matchControlTag(vm, caseTag);
  /* grab the current number of ENDOF fixups */
  fixupCount = ficlStackPopUnsigned(vm->dataStack);

  /* FICL_VM_STATE_COMPILE branch runtime */
  PRIMITIVE_APPEND_UNSIGNED(dict, ficlInstructionBranchParenWithCheck);

  /* push a new ENDOF fixup, the updated count of ENDOF fixups, and the caseTag */
  ficlStackPushPointer(vm->dataStack, dict->here);
  ficlStackPushUnsigned(vm->dataStack, fixupCount + 1);
  markControlTag(vm, caseTag);

  /* reserve space for the ENDOF fixup */
  PRIMITIVE_APPEND_UNSIGNED(dict, 2);

  /* and patch the original OF */
  offset = dict->here - patchAddr;
  CELL_INT_SET(patchAddr, offset);
}

/**************************************************************************
 **                  f a l l t h r o u g h C o I m
 ** IMMEDIATE FICL_VM_STATE_COMPILE-ONLY
 **************************************************************************/
static void ficlPrimitiveFallthroughCoIm(ficlVm *vm)
{
  ficlCell *patchAddr;
  ficlInteger offset;
  ficlDictionary *dict = ficlVmGetDictionary(vm);

  FICL_STACK_CHECK(vm->dataStack, 4, 3);

  /* ensure we're in an OF, */
  matchControlTag(vm, ofTag);
  /* grab the address of the branch location after the OF */
  patchAddr = (ficlCell *)ficlStackPopPointer(vm->dataStack);
  /* ensure we're also in a "case" */
  matchControlTag(vm, caseTag);

  /* okay, here we go.  put the case tag back. */
  markControlTag(vm, caseTag);

  /* FICL_VM_STATE_COMPILE branch runtime */
  PRIMITIVE_APPEND_UNSIGNED(dict, ficlInstructionBranchParenWithCheck);

  /* push a new FALLTHROUGH fixup and the fallthroughTag */
  ficlStackPushPointer(vm->dataStack, dict->here);
  markControlTag(vm, fallthroughTag);

  /* reserve space for the FALLTHROUGH fixup */
  PRIMITIVE_APPEND_UNSIGNED(dict, 2);

  /* and patch the original OF */
  offset = dict->here - patchAddr;
  CELL_INT_SET(patchAddr, offset);
}

/**************************************************************************
 **                      h a s h
 ** hash ( c-addr u -- code)
 ** calculates hashcode of specified string and leaves it on the stack
 **************************************************************************/
static void ficlPrimitiveHash(ficlVm *vm)
{
  ficlString s;

  FICL_STACK_CHECK(vm->dataStack, 2, 1);
  FICL_STRING_SET_LENGTH(s, ficlStackPopUnsigned(vm->dataStack));
  FICL_STRING_SET_POINTER(s, ficlStackPopPointer(vm->dataStack));
  ficlStackPushUnsigned(vm->dataStack, ficlHashCode(s));
}

/**************************************************************************
 **                      i n t e r p r e t 
 ** This is the "user interface" of a Forth. It does the following:
 **   while there are words in the VM's Text Input Buffer
 **     Copy next word into the pad (ficlVmGetWord)
 **     Attempt to find the word in the dictionary (ficlDictionaryLookup)
 **     If successful, execute the word.
 **     Otherwise, attempt to convert the word to a number (isNumber)
 **     If successful, push the number onto the parameter stack.
 **     Otherwise, print an error message and exit loop...
 **   End Loop
 **
 ** From the standard, section 3.4
 ** Text interpretation (see 6.1.1360 EVALUATE and 6.1.2050 QUIT) shall
 ** repeat the following steps until either the parse area is empty or an 
 ** ambiguous condition exists: 
 ** a) Skip leading spaces and parse a name (see 3.4.1); 
 **************************************************************************/
static void ficlPrimitiveInterpret(ficlVm *vm)
{
  ficlString s;
  ficlSystem *sys;
  int i;

  FICL_ASSERT(vm != NULL);
  sys = vm->callback.system;
  s = ficlVmGetWord0(vm);

  /*
  ** Get next word...if out of text, we're done.
  */
  if (s.length == 0)
    ficlVmThrow(vm, FICL_VM_STATUS_OUT_OF_TEXT);

  /*
  ** Run the parse chain against the incoming token until somebody eats it.
  ** Otherwise emit an error message and give up.
  */
  for (i = 0; i < FICL_MAX_PARSE_STEPS; i++)
  {
    ficlWord *word = sys->parseList[i];
           
    if (word == NULL)
      break;

    if (word->code == ficlPrimitiveParseStepParen)
    {
      ficlParseStep pStep = (ficlParseStep)CELL_FN_REF(word->param);

      if ((*pStep)(vm, s))
	return;
    }
    else
    {
      ficlStackPushPointer(vm->dataStack, FICL_STRING_GET_POINTER(s));
      ficlStackPushUnsigned(vm->dataStack, FICL_STRING_GET_LENGTH(s));
      ficlVmExecuteXT(vm, word);

      if (ficlStackPopInteger(vm->dataStack))
	return;
    }
  }
  ficlVmThrowException(vm, FICL_VM_STATUS_UNDEFINED, "%.*s",
    (int)s.length, s.text);
}

/*
** Surrogate precompiled parse step for ficlParseWord (this step is hard coded in 
** FICL_VM_STATE_INTERPRET)
*/
static void ficlPrimitiveLookup(ficlVm *vm)
{
  ficlString name;

  FICL_STRING_SET_LENGTH(name, ficlStackPopUnsigned(vm->dataStack));
  FICL_STRING_SET_POINTER(name, ficlStackPopPointer(vm->dataStack));
  ficlStackPushInteger(vm->dataStack, (ficlInteger)ficlVmParseWord(vm, name));
}

/**************************************************************************
 **                      p a r e n P a r s e S t e p
 ** (parse-step)  ( c-addr u -- flag )
 ** runtime for a precompiled parse step - pop a counted string off the
 ** stack, run the parse step against it, and push the result flag (FICL_TRUE
 ** if success, FICL_FALSE otherwise).
 **************************************************************************/
void ficlPrimitiveParseStepParen(ficlVm *vm)
{
#define h_ficlPrimitiveParseStepParen "( c-addr u -- flag )  \
Runtime for a precompiled parse step---pop a counted string off the \
stack, run the parse step against it, and push the result flag (FICL_TRUE \
if success, FICL_FALSE otherwise)."
  ficlString s;
  ficlWord *word;
  ficlParseStep pStep;

  word = vm->runningWord;
  pStep = (ficlParseStep)CELL_FN_REF(word->param);
  FICL_STRING_SET_LENGTH(s, ficlStackPopInteger(vm->dataStack));
  FICL_STRING_SET_POINTER(s, ficlStackPopPointer(vm->dataStack));
  ficlStackPushInteger(vm->dataStack, (ficlInteger)(*pStep)(vm, s));
}

static void ficlPrimitiveAddParseStep(ficlVm *vm)
{
  ficlWord *pStep;

  FICL_STACK_CHECK(vm->dataStack, 1, 0);
  pStep = ficlStackPopPointer(vm->dataStack);
  if ((pStep != NULL) && ficlDictionaryIsAWord(ficlVmGetDictionary(vm), pStep))
    ficlSystemAddParseStep(vm->callback.system, pStep);
}

/**************************************************************************
 **                        l i t e r a l I m
 ** 
 ** IMMEDIATE code for "literal". This function gets a value from the stack 
 ** and compiles it into the dictionary preceded by the code for "(literal)".
 ** IMMEDIATE
 **************************************************************************/
void ficlPrimitiveLiteralIm(ficlVm *vm)
{
#define h_ficlPrimitiveLiteralIm "\
IMMEDIATE code for LITERAL.  \
This function gets a value from the stack and compiles it \
into the dictionary preceded by the code for (LITERAL)."
  ficlDictionary *dict = ficlVmGetDictionary(vm);
  ficlInteger value;

  value = ficlStackPopInteger(vm->dataStack);

  switch (value)
  {
  case 1:
  case 2:
  case 3:
  case 4:
  case 5:
  case 6:
  case 7:
  case 8:
  case 9:
  case 10:
  case 11:
  case 12:
  case 13:
  case 14:
  case 15:
  case 16:
    PRIMITIVE_APPEND_UNSIGNED(dict, value);
    break;

  case 0:
  case -1:
  case -2:
  case -3:
  case -4:
  case -5:
  case -6:
  case -7:
  case -8:
  case -9:
  case -10:
  case -11:
  case -12:
  case -13:
  case -14:
  case -15:
  case -16:
    PRIMITIVE_APPEND_UNSIGNED(dict, (ficlInteger)ficlInstruction0 - value);
    break;
  default:
    PRIMITIVE_APPEND_UNSIGNED(dict, ficlInstructionLiteralParen);
    PRIMITIVE_APPEND_UNSIGNED(dict, value);
    break;
  }
}

/**************************************************************************
 **                             D o  /  L o o p
 ** do -- IMMEDIATE FICL_VM_STATE_COMPILE ONLY
 **    Compiles code to initialize a loop: FICL_VM_STATE_COMPILE (do), 
 **    allot space to hold the "leave" address, push a branch
 **    target address for the loop.
 ** (do) -- runtime for "do"
 **    pops index and limit from the p stack and moves them
 **    to the r stack, then skips to the loop body.
 ** loop -- IMMEDIATE FICL_VM_STATE_COMPILE ONLY
 ** +loop
 **    Compiles code for the test part of a loop:
 **    FICL_VM_STATE_COMPILE (loop), resolve forward branch from "do", and
 **    copy "here" address to the "leave" address allotted by "do"
 ** i,j,k -- FICL_VM_STATE_COMPILE ONLY
 **    Runtime: Push loop indices on param stack (i is innermost loop...)
 **    Note: each loop has three values on the return stack:
 **    ( R: leave limit index )
 **    "leave" is the absolute address of the next ficlCell after the loop
 **    limit and index are the loop control variables.
 ** leave -- FICL_VM_STATE_COMPILE ONLY
 **    Runtime: pop the loop control variables, then pop the
 **    "leave" address and jump (absolute) there.
 **************************************************************************/
static void ficlPrimitiveDoCoIm(ficlVm *vm)
{
  ficlDictionary *dict = ficlVmGetDictionary(vm);

  PRIMITIVE_APPEND_UNSIGNED(dict, ficlInstructionDoParen);
  /*
  ** Allot space for a pointer to the end
  ** of the loop - "leave" uses this...
  */
  markBranch(dict, vm, leaveTag);
  PRIMITIVE_APPEND_UNSIGNED(dict, 0);
  /*
  ** Mark location of head of loop...
  */
  markBranch(dict, vm, doTag);
}

static void ficlPrimitiveQDoCoIm(ficlVm *vm)
{
  ficlDictionary *dict = ficlVmGetDictionary(vm);

  PRIMITIVE_APPEND_UNSIGNED(dict, ficlInstructionQDoParen);
  /*
  ** Allot space for a pointer to the end
  ** of the loop - "leave" uses this...
  */
  markBranch(dict, vm, leaveTag);
  PRIMITIVE_APPEND_UNSIGNED(dict, 0);
  /*
  ** Mark location of head of loop...
  */
  markBranch(dict, vm, doTag);
}

static void ficlPrimitiveLoopCoIm(ficlVm *vm)
{
  ficlDictionary *dict = ficlVmGetDictionary(vm);

  PRIMITIVE_APPEND_UNSIGNED(dict, ficlInstructionLoopParen);
  resolveBackBranch(dict, vm, doTag);
  resolveAbsBranch(dict, vm, leaveTag);
}

static void ficlPrimitivePlusLoopCoIm(ficlVm *vm)
{
  ficlDictionary *dict = ficlVmGetDictionary(vm);

  PRIMITIVE_APPEND_UNSIGNED(dict, ficlInstructionPlusLoopParen);
  resolveBackBranch(dict, vm, doTag);
  resolveAbsBranch(dict, vm, leaveTag);
}

/**************************************************************************
 **                      v a r i a b l e
 ** 
 **************************************************************************/
static void ficlPrimitiveVariable(ficlVm *vm)
{
  ficlDictionary *dict = ficlVmGetDictionary(vm);
  ficlString s = ficlVmGetWord(vm);

  ficlDictionaryAppendWord(dict, s, (ficlPrimitive)ficlInstructionVariableParen, FICL_WORD_DEFAULT);
  ficlVmDictionaryAllotCells(dict, 1);
}

/**************************************************************************
 **                      b a s e   &   f r i e n d s
 ** 
 **************************************************************************/
static void ficlPrimitiveBase(ficlVm *vm)
{
  FICL_STACK_CHECK(vm->dataStack, 0, 1);
  ficlStackPushPointer(vm->dataStack, &vm->base);
}

static void ficlPrimitiveDecimal(ficlVm *vm)
{
  vm->base = 10;
}

static void ficlPrimitiveHex(ficlVm *vm)
{
  vm->base = 16;
}

/**************************************************************************
 **                      a l l o t   &   f r i e n d s
 ** 
 **************************************************************************/
static void ficlPrimitiveAllot(ficlVm *vm)
{
  ficlDictionary *dict = ficlVmGetDictionary(vm);
  int i;

  FICL_STACK_CHECK(vm->dataStack, 1, 0);
  i = (int)ficlStackPopInteger(vm->dataStack);
  ficlVmDictionaryCheck(dict, i);
  ficlVmDictionaryAllot(dict, i);
}

static void ficlPrimitiveHere(ficlVm *vm)
{
  FICL_STACK_CHECK(vm->dataStack, 0, 1);
  ficlStackPushPointer(vm->dataStack, ficlVmGetDictionary(vm)->here);
}

/**************************************************************************
 **                      t i c k
 ** tick         CORE ( "<spaces>name" -- xt )
 ** Skip leading space delimiters. Parse name delimited by a space. Find
 ** name and return xt, the execution token for name. An ambiguous condition
 ** exists if name is not found. 
 **************************************************************************/
void ficlPrimitiveTick(ficlVm *vm)
{
#define h_ficlPrimitiveTick "( \"<spaces>name\" -- xt )  \
Skip leading space delimiters.  \
Parse name delimited by a space.  \
Find name and return xt, the execution token for name.  \
An ambiguous condition exists if name is not found."
  ficlWord *word = NULL;
  ficlString name = ficlVmGetWord(vm);

  FICL_STACK_CHECK(vm->dataStack, 0, 1);
  word = ficlDictionaryLookup(ficlVmGetDictionary(vm), name);

  if (word)
    ficlStackPushPointer(vm->dataStack, word);
  else
    ficlVmThrowException(vm, FICL_VM_STATUS_UNDEFINED, "%.*s", (int)name.length, name.text);
}

static void ficlPrimitiveBracketTickCoIm(ficlVm *vm)
{
  ficlPrimitiveTick(vm);
  ficlPrimitiveLiteralIm(vm);
}

static void
ficl_tick_stateless_im(ficlVm *vm)
{
  ficlPrimitiveTick(vm);

  if (vm->state == FICL_VM_STATE_COMPILE)
    ficlPrimitiveLiteralIm(vm);
}

/**************************************************************************
 **                      p o s t p o n e
 ** Lookup the next word in the input stream and FICL_VM_STATE_COMPILE code to 
 ** insert it into definitions created by the resulting word
 ** (defers compilation, even of immediate words)
 **************************************************************************/
static ficlWord *postpone_none_im;
static void
ficl_postpone_none_im(ficlVm *vm)
{
  ficlDictionary *dict = ficlVmGetDictionary(vm);

  /* Preparation for word location, see also vm.c.  [ms] */
  PRIMITIVE_APPEND_UNSIGNED(dict, ficl_word_location);
  ficlDictionaryAppendPointer(dict, dict->smudge);
  ficlDictionaryAppendFTH(dict, fth_string_copy(fth_current_file));
  ficlDictionaryAppendInteger(dict, fth_current_line);
  ficlDictionaryAppendCell(dict, ficlStackPop(vm->dataStack));
}

static void
ficlPrimitivePostponeCoIm(ficlVm *vm)
{
#define h_ficlPrimitivePostponeCoIm "\
Lookup the next word in the input stream and FICL_VM_STATE_COMPILE code to \
insert it into definitions created by the resulting word \
(defers compilation, even of immediate words)."
  ficlWord *word;
  ficlCell c;

  ficlPrimitiveTick(vm);
  c = ficlStackGetTop(vm->dataStack);
  word = CELL_VOIDP_REF(&c);

  if (ficlWordIsImmediate(word))
    ficl_postpone_none_im(vm);
  else
  {
    ficlPrimitiveLiteralIm(vm);
    ficlDictionaryAppendPointer(ficlVmGetDictionary(vm), postpone_none_im);
  }
}

/**************************************************************************
 **                      e x e c u t e
 ** Pop an execution token (pointer to a word) off the stack and
 ** run it
 **************************************************************************/
static void ficlPrimitiveExecute(ficlVm *vm)
{
#define h_ficlPrimitiveExecute "( xt -- )  \
Pop an execution token (pointer to a word) off the stack and run it."
  ficlWord *word;

  FICL_STACK_CHECK(vm->dataStack, 1, 0);
  word = ficlStackPopPointer(vm->dataStack);
  ficlVmExecuteWord(vm, word);
}

/**************************************************************************
 **                      i m m e d i a t e
 ** Make the most recently compiled word IMMEDIATE -- it executes even
 ** in FICL_VM_STATE_COMPILE state (most often used for control compiling words
 ** such as IF, THEN, etc)
 **************************************************************************/
static void ficlPrimitiveImmediate(ficlVm *vm)
{
#define h_ficlPrimitiveImmediate "\
Make the most recently compiled word IMMEDIATE---it executes even \
in FICL_VM_STATE_COMPILE state (most often used for control compiling words \
such as IF, THEN, etc)."
  ficlDictionarySetImmediate(ficlVmGetDictionary(vm));
}

static void ficlPrimitiveCompileOnly(ficlVm *vm)
{
  ficlDictionarySetFlags(ficlVmGetDictionary(vm), FICL_WORD_COMPILE_ONLY);
}

static void ficlPrimitiveSetObjectFlag(ficlVm *vm)
{
  ficlDictionarySetFlags(ficlVmGetDictionary(vm), FICL_WORD_OBJECT);
}

static void ficlPrimitiveIsObject(ficlVm *vm)
{
  ficlInteger flag;
  ficlWord *word = (ficlWord *)ficlStackPopPointer(vm->dataStack);
    
  flag = ((word != NULL) && (word->flags & FICL_WORD_OBJECT)) ? FICL_TRUE : FICL_FALSE;
  ficlStackPushInteger(vm->dataStack, flag);
}

static void ficlPrimitiveCountedStringQuoteIm(ficlVm *vm)
{
  ficlDictionary *dict = ficlVmGetDictionary(vm);

  if (vm->state == FICL_VM_STATE_INTERPRET)
  {
    ficlCountedString *counted = (ficlCountedString *)dict->here;
    
    ficlVmGetString(vm, counted, '\"');
    ficlStackPushPointer(vm->dataStack, counted);
    /* move HERE past string so it doesn't get overwritten.  --lch */
    ficlVmDictionaryAllot(dict, (int)(counted->length + sizeof(ficlUnsigned8)));
  }
  else    /* FICL_VM_STATE_COMPILE state */
  {
    PRIMITIVE_APPEND_UNSIGNED(dict, ficlInstructionCStringLiteralParen);
    dict->here = FICL_POINTER_TO_CELL(ficlVmGetString(vm, (ficlCountedString *)dict->here, '\"'));
    ficlDictionaryAlign(dict);
  }
}

/**************************************************************************
 **                      d o t Q u o t e
 ** IMMEDIATE word that compiles a string literal for later display
 ** FICL_VM_STATE_COMPILE fiStringLiteralParen, then copy the bytes of the string from the
 ** TIB to the dictionary. Backpatch the count byte and align the dictionary.
 **************************************************************************/
static void ficlPrimitiveDotQuoteCoIm(ficlVm *vm)
{
#define h_ficlPrimitiveDotQuoteCoIm "\
IMMEDIATE word that compiles a string literal for later display."
  ficlDictionary *dict = ficlVmGetDictionary(vm);
  ficlWord *pType = ficlSystemLookup(vm->callback.system, "type");
    
  FICL_ASSERT(pType != NULL);
  PRIMITIVE_APPEND_UNSIGNED(dict, ficlInstructionStringLiteralParen);
  dict->here = FICL_POINTER_TO_CELL(ficlVmGetString(vm, (ficlCountedString *)dict->here, '\"'));
  ficlDictionaryAlign(dict);
  ficlDictionaryAppendPointer(dict, pType);
}

static void ficlPrimitiveDotParen(ficlVm *vm)
{
  char *from = ficlVmGetInBuf(vm);
  char *stop = ficlVmGetInBufEnd(vm);
  char *to = vm->pad;
  char c;

  /*
  ** Note: the standard does not want leading spaces skipped.
  */
  for (c = *from; (from != stop) && (c != ')'); c = *++from)
    *to++ = c;

  *to = '\0';

  if ((from != stop) && (c == ')'))
    from++;

  fth_print(vm->pad);
  ficlVmUpdateTib(vm, from);
}

/**************************************************************************
 **                      s l i t e r a l
 ** STRING 
 ** Interpretation: Interpretation semantics for this word are undefined.
 ** Compilation: ( c-addr1 u -- )
 ** Append the run-time semantics given below to the current definition.
 ** Run-time:       ( -- c-addr2 u )
 ** Return c-addr2 u describing a string consisting of the characters
 ** specified by c-addr1 u during compilation. A program shall not alter
 ** the returned string. 
 **************************************************************************/
static void ficlPrimitiveSLiteralCoIm(ficlVm *vm)
{
#define h_ficlPrimitiveSLiteralCoIm "\
Interpretation: Interpretation semantics for this word are undefined.\n\
Compilation: ( c-addr1 u -- )\n\
Append the run-time semantics given below to the current definition.\n\
Run-time:    ( -- c-addr2 u )\n\
Return C-ADDR2 U describing a string consisting of the characters \
specified by C-ADDR1 U during compilation.  \
A program shall not alter the returned string."
  ficlDictionary *dict;
  char *from;
  char *to;
  ficlUnsigned length;

  FICL_STACK_CHECK(vm->dataStack, 2, 0);
  dict = ficlVmGetDictionary(vm);
  length  = ficlStackPopUnsigned(vm->dataStack);
  from = ficlStackPopPointer(vm->dataStack);
  PRIMITIVE_APPEND_UNSIGNED(dict, ficlInstructionStringLiteralParen);
  to = (char *)dict->here;
  *to++ = (char)length;

  for (; length > 0; --length)
    *to++ = *from++;

  *to++ = 0;
  dict->here = FICL_POINTER_TO_CELL(ficlAlignPointer(to));
}

/**************************************************************************
 **                      s t a t e
 ** Return the address of the VM's state member (must be sized the
 ** same as a ficlCell for this reason)
 **************************************************************************/
static void ficlPrimitiveState(ficlVm *vm)
{
#define h_ficlPrimitiveState "( -- addr )  \
Return the address of the VM's state member."
  FICL_STACK_CHECK(vm->dataStack, 0, 1);
  ficlStackPushPointer(vm->dataStack, &vm->state);
}

/**************************************************************************
 **                      c r e a t e . . . d o e s >
 ** Make a new word in the dictionary with the run-time effect of 
 ** a variable (push my address), but with extra space allotted
 ** for use by does> .
 **************************************************************************/
static void ficlPrimitiveCreate(ficlVm *vm)
{
  ficlDictionary *dict = ficlVmGetDictionary(vm);
  ficlString s = ficlVmGetWord(vm);

  ficlDictionaryAppendWord(dict, s, (ficlPrimitive)ficlInstructionCreateParen, FICL_WORD_DEFAULT);
  ficlVmDictionaryAllotCells(dict, 1);
}

static void ficlPrimitiveDoesCoIm(ficlVm *vm)
{
  ficlDictionary *dict = ficlVmGetDictionary(vm);

  if (vm->callback.system->localsCount > 0)
  {
    ficlDictionary *locals = ficlSystemGetLocals(vm->callback.system);
    
    ficlDictionaryEmpty(locals, locals->forthWordlist->size);
    PRIMITIVE_APPEND_UNSIGNED(dict, ficlInstructionUnlinkParen);
  }
  vm->callback.system->localsCount = 0;
  PRIMITIVE_APPEND_UNSIGNED(dict, ficlInstructionDoesParen);
  /* defined in src/proc.c [ms] */
  ficl_init_locals(vm, dict);
}

/**************************************************************************
 **                      t o   b o d y
 ** to-body      CORE ( xt -- a-addr )
 ** a-addr is the data-field address corresponding to xt. An ambiguous
 ** condition exists if xt is not for a word defined via CREATE. 
 **************************************************************************/
static void ficlPrimitiveToBody(ficlVm *vm)
{
#define h_ficlPrimitiveToBody "( xt -- a-addr )  \
A-ADDR is the data-field address corresponding to XT.  \
An ambiguous condition exists if XT is not for a word defined via CREATE."
  ficlWord *word;

  FICL_STACK_CHECK(vm->dataStack, 1, 1);
  word = ficlStackPopPointer(vm->dataStack);
  ficlStackPushPointer(vm->dataStack, word->param + 1);
}

/*
** from-body       Ficl ( a-addr -- xt )
** Reverse effect of >body
*/
static void ficlPrimitiveFromBody(ficlVm *vm)
{
#define h_ficlPrimitiveFromBody "( a-addr -- xt )  Reverse effect of >body."
  char *ptr;

  FICL_STACK_CHECK(vm->dataStack, 1, 1);
  ptr = (char *)ficlStackPopPointer(vm->dataStack) - sizeof (ficlWord);
  ficlStackPushPointer(vm->dataStack, ptr);
}

/*
** >name        Ficl ( xt -- c-addr u )
** Push the address and length of a word's name given its address
** xt. 
*/
static void ficlPrimitiveToName(ficlVm *vm)
{
#define h_ficlPrimitiveToName "( xt -- c-addr u )  \
Push the address and length of a word's name given its address XT."
  ficlWord *word;

  FICL_STACK_CHECK(vm->dataStack, 1, 2);
  word = ficlStackPopPointer(vm->dataStack);

  if (word && word->length > 0)
  {
    ficlStackPushPointer(vm->dataStack, word->name);
    ficlStackPushUnsigned(vm->dataStack, word->length);
  }
  else
  {
    ficlStackPushPointer(vm->dataStack, "noname");
    ficlStackPushUnsigned(vm->dataStack, strlen("noname"));
  }
}

static void ficlPrimitiveLastWord(ficlVm *vm)
{
  ficlDictionary *dict = ficlVmGetDictionary(vm);
  ficlWord *wp = dict->smudge;

  FICL_ASSERT(wp != NULL);
  ficlStackPushPointer(vm->dataStack, wp);
}

/**************************************************************************
 **                      l b r a c k e t   e t c
 ** 
 **************************************************************************/
static void ficlPrimitiveLeftBracketCoIm(ficlVm *vm)
{
  vm->state = FICL_VM_STATE_INTERPRET;
}

static void ficlPrimitiveRightBracket(ficlVm *vm)
{
  vm->state = FICL_VM_STATE_COMPILE;
}

/**************************************************************************
 **                      p i c t u r e d   n u m e r i c   w o r d s
 **
 ** less-number-sign CORE ( -- )
 ** Initialize the pictured numeric output conversion process. 
 ** (clear the pad)
 **************************************************************************/
static void ficlPrimitiveLessNumberSign(ficlVm *vm)
{
#define h_ficlPrimitiveLessNumberSign "( -- )  \
Initialize the pictured numeric output conversion process (clear the pad)."
  ficlCountedString *counted = FICL_POINTER_TO_COUNTED_STRING(vm->pad);

  counted->length = 0;
}

/*
** number-sign      CORE ( ud1 -- ud2 )
** Divide ud1 by the number in BASE giving the quotient ud2 and the remainder
** n. (n is the least-significant digit of ud1.) Convert n to external form
** and add the resulting character to the beginning of the pictured numeric
** output  string. An ambiguous condition exists if # executes outside of a
** <# #> delimited number conversion. 
*/
static void ficlPrimitiveNumberSign(ficlVm *vm)
{
#define h_ficlPrimitiveNumberSign "( ud1 -- ud2 )  \
Divide UD1 by the number in BASE giving the quotient UD2 and the remainder N \
(N is the least-significant digit of UD1).  \
Convert n to external form and add the resulting character \
to the beginning of the pictured numeric output  string.  \
An ambiguous condition exists if # executes outside of a <# #> delimited number conversion."
  ficlCountedString *counted;
  ficl2Unsigned ud;
  ficl2UnsignedQR uqr;

  FICL_STACK_CHECK(vm->dataStack, 1, 1);
  counted = FICL_POINTER_TO_COUNTED_STRING(vm->pad);
  ud = ficlStackPop2Unsigned(vm->dataStack);
  uqr = ficl2UnsignedDivide(ud, (ficlUnsigned)(vm->base));
  counted->text[counted->length++] = ficlDigitToCharacter((int)uqr.remainder);
  ficlStackPush2Unsigned(vm->dataStack, uqr.quotient);
}

/*
** number-sign-greater CORE ( xd -- c-addr u )
** Drop xd. Make the pictured numeric output string available as a character
** string. c-addr and u specify the resulting character string. A program
** may replace characters within the string. 
*/
static void ficlPrimitiveNumberSignGreater(ficlVm *vm)
{
#define h_ficlPrimitiveNumberSignGreater "( xd -- c-addr u )  \
Drop xd.  \
Make the pictured numeric output string available as a character string.  \
C-ADDR and U specify the resulting character string.  \
A program may replace characters within the string."
  ficlCountedString *counted;

  FICL_STACK_CHECK(vm->dataStack, 1, 2);
  counted = FICL_POINTER_TO_COUNTED_STRING(vm->pad);
  counted->text[counted->length] = 0;
  ficlStringReverse(counted->text);
  ficlStackDrop(vm->dataStack, 1);
  ficlStackPushPointer(vm->dataStack, counted->text);
  ficlStackPushUnsigned(vm->dataStack, counted->length);
}

/*
** number-sign-s    CORE ( ud1 -- ud2 )
** Convert one digit of ud1 according to the rule for #. Continue conversion
** until the quotient is zero. ud2 is zero. An ambiguous condition exists if
** #S executes outside of a <# #> delimited number conversion. 
** TO DO: presently does not use ud1 hi ficlCell - use it!
*/
static void ficlPrimitiveNumberSignS(ficlVm *vm)
{
#define h_ficlPrimitiveNumberSignS "( ud1 -- ud2 )  \
Convert one digit of UD1 according to the rule for #.  \
Continue conversion until the quotient is zero.  \
UD2 is zero.  \
An ambiguous condition exists if #S executes outside of a <# #> delimited number conversion."
  ficlCountedString *counted;
  ficl2Unsigned ud;
  ficl2UnsignedQR uqr;

  FICL_STACK_CHECK(vm->dataStack, 1, 1);
  counted = FICL_POINTER_TO_COUNTED_STRING(vm->pad);
  ud = ficlStackPop2Unsigned(vm->dataStack);

  do {
    uqr = ficl2UnsignedDivide(ud, (ficlUnsigned)(vm->base));
    counted->text[counted->length++] = ficlDigitToCharacter((int)uqr.remainder);
    ud = uqr.quotient;
  } while (ud != 0);
  
  ficlStackPush2Unsigned(vm->dataStack, ud);
}

/*
** HOLD             CORE ( char -- )
** Add char to the beginning of the pictured numeric output string. An ambiguous
** condition exists if HOLD executes outside of a <# #> delimited number conversion.
*/
static void ficlPrimitiveHold(ficlVm *vm)
{
#define h_ficlPrimitiveHold "( char -- )  \
Add char to the beginning of the pictured numeric output string.  \
An ambiguous condition exists if HOLD executes outside of a <# #> delimited number conversion."
  ficlCountedString *counted;
  int i;

  FICL_STACK_CHECK(vm->dataStack, 1, 0);
  counted = FICL_POINTER_TO_COUNTED_STRING(vm->pad);
  i = (int)ficlStackPopInteger(vm->dataStack);
  counted->text[counted->length++] = (char)i;
}

/*
** SIGN             CORE ( n -- )
** If n is negative, add a minus sign to the beginning of the pictured
** numeric output string. An ambiguous condition exists if SIGN
** executes outside of a <# #> delimited number conversion. 
*/
static void ficlPrimitiveSign(ficlVm *vm)
{
#define h_ficlPrimitiveSign "( n -- )  \
If N is negative, add a minus sign to the beginning of the pictured numeric output string.  \
An ambiguous condition exists if SIGN executes outside of a <# #> delimited number conversion."
  ficlCountedString *counted;
  ficlInteger i;

  FICL_STACK_CHECK(vm->dataStack, 1, 0);
  counted = FICL_POINTER_TO_COUNTED_STRING(vm->pad);
  i = ficlStackPopInteger(vm->dataStack);

  if (i < 0)
    counted->text[counted->length++] = '-';
}

/**************************************************************************
 **                        t o   N u m b e r
 ** to-number CORE ( ud1 c-addr1 u1 -- ud2 c-addr2 u2 )
 ** ud2 is the unsigned result of converting the characters within the
 ** string specified by c-addr1 u1 into digits, using the number in BASE,
 ** and adding each into ud1 after multiplying ud1 by the number in BASE.
 ** Conversion continues left-to-right until a character that is not
 ** convertible, including any + or -, is encountered or the string is
 ** entirely converted. c-addr2 is the location of the first unconverted
 ** character or the first character past the end of the string if the string
 ** was entirely converted. u2 is the number of unconverted characters in the
 ** string. An ambiguous condition exists if ud2 overflows during the
 ** conversion. 
 **************************************************************************/
static void ficlPrimitiveToNumber(ficlVm *vm)
{
#define h_ficlPrimitiveToNumber "( ud1 c-addr1 u1 -- ud2 c-addr2 u2 )  \
UD2 is the unsigned result of converting the characters within the \
string specified by C-ADDR1 U1 into digits, using the number in BASE, \
and adding each into UD1 after multiplying UD1 by the number in BASE.  \
Conversion continues left-to-right until a character that is not \
convertible, including any + or -, is encountered or the string is entirely converted.  \
C-ADDR2 is the location of the first unconverted \
character or the first character past the end of the string if the string \
was entirely converted.  \
U2 is the number of unconverted characters in the string.  \
An ambiguous condition exists if ud2 overflows during the conversion."
  ficlUnsigned length;
  char *trace;
  ficl2Unsigned accumulator;
  ficlUnsigned digit, base = vm->base;
  int c;

  FICL_STACK_CHECK(vm->dataStack, 3, 3);
  length = ficlStackPopUnsigned(vm->dataStack);
  trace = (char *)ficlStackPopPointer(vm->dataStack);
  accumulator = ficlStackPop2Unsigned(vm->dataStack);

  for (c = *trace; length > 0; c = *++trace, length--)
  {
    if (c < (int)'0')
      break;

    digit = (ficlUnsigned)(c - '0');

    if (digit > 9)
      digit = (ficlUnsigned)(tolower(c) - 'a' + 10);
    /* 
    ** Note: following test also catches chars between 9 and a
    ** because 'digit' is unsigned! 
    */
    if (digit >= base)
      break;

    accumulator = accumulator * base + digit;
  }

  ficlStackPush2Unsigned(vm->dataStack, accumulator);
  ficlStackPushPointer(vm->dataStack, trace);
  ficlStackPushUnsigned(vm->dataStack, length);
}

/**************************************************************************
 **                      q u i t   &   a b o r t
 ** quit CORE   ( -- )  ( R:  i*x -- )
 ** Empty the return stack, store zero in SOURCE-ID if it is present, make
 ** the user input device the input source, and enter interpretation state. 
 ** Do not display a message. Repeat the following: 
 **
 **   Accept a line from the input source into the input buffer, set >IN to
 **   zero, and FICL_VM_STATE_INTERPRET. 
 **   Display the implementation-defined system prompt if in
 **   interpretation state, all processing has been completed, and no
 **   ambiguous condition exists. 
 **************************************************************************/
static void ficlPrimitiveQuit(ficlVm *vm)
{
#define h_ficlPrimitiveQuit "( -- )  ( R:  i*x -- )  \
Empty the return stack, store zero in SOURCE-ID if it is present, make \
the user input device the input source, and enter interpretation state.  \
Do not display a message.  \
Repeat the following:\n\
Accept a line from the input source into the input buffer, set >IN to zero, \
and FICL_VM_STATE_INTERPRET.  \
Display the implementation-defined system prompt if in \
interpretation state, all processing has been completed, and no \
ambiguous condition exists."
  ficlVmThrow(vm, FICL_VM_STATUS_QUIT);
}


static void ficlPrimitiveAbort(ficlVm *vm)
{
  ficlVmThrow(vm, FICL_VM_STATUS_ABORT);
}

/**************************************************************************
 **                      a c c e p t
 ** accept       CORE ( c-addr +n1 -- +n2 )
 ** Receive a string of at most +n1 characters. An ambiguous condition
 ** exists if +n1 is zero or greater than 32,767. Display graphic characters
 ** as they are received. A program that depends on the presence or absence
 ** of non-graphic characters in the string has an environmental dependency.
 ** The editing functions, if any, that the system performs in order to
 ** construct the string are implementation-defined. 
 **
 ** (Although the standard text doesn't say so, I assume that the intent 
 ** of 'accept' is to store the string at the address specified on
 ** the stack.)
 ** Implementation: if there's more text in the TIB, use it. Otherwise
 ** throw out for more text. Copy characters up to the max count into the
 ** address given, and return the number of actual characters copied.
 ** 
 ** Note (sobral) this may not be the behavior you'd expect if you're
 ** trying to get user input at load time!
 **************************************************************************/
static void ficlPrimitiveAccept(ficlVm *vm)
{
#define h_ficlPrimitiveAccept "( c-addr +n1 -- +n2 )  \
Receive a string of at most +n1 characters.  An ambiguous condition \
exists if +n1 is zero or greater than 32,767.  Display graphic characters \
as they are received.  A program that depends on the presence or absence \
of non-graphic characters in the string has an environmental dependency.  \
The editing functions, if any, that the system performs in order to \
construct the string are implementation-defined.\n\
(Although the standard text doesn't say so, I assume that the intent \
of 'accept' is to store the string at the address specified on the stack.)\n\
Implementation: if there's more text in the TIB, use it.  Otherwise \
throw out for more text.  Copy characters up to the max count into the \
address given, and return the number of actual characters copied.\n\
Note (sobral) this may not be the behavior you'd expect if you're \
trying to get user input at load time!"
  ficlInteger size, length;
  char *address;
  char *trace;
  char *end;

  FICL_STACK_CHECK(vm->dataStack, 2, 1);
  trace = ficlVmGetInBuf(vm);
  end = ficlVmGetInBufEnd(vm);
  length = end - trace;

  if (length == 0)
    ficlVmThrow(vm, FICL_VM_STATUS_RESTART);

  /*
  ** Now we have something in the text buffer - use it 
  */
  size = ficlStackPopInteger(vm->dataStack);
  address = ficlStackPopPointer(vm->dataStack);
  length = (size < length) ? size : length;
  strncpy(address, trace, (size_t)length);
  trace += length;
  ficlVmUpdateTib(vm, trace);
  ficlStackPushInteger(vm->dataStack, length);
}

/**************************************************************************
 **                      a l i g n
 ** 6.1.0705 ALIGN       CORE ( -- )
 ** If the data-space pointer is not aligned, reserve enough space to
 ** align it. 
 **************************************************************************/
static void ficlPrimitiveAlign(ficlVm *vm)
{
#define h_ficlPrimitiveAlign "( -- )  \
If the data-space pointer is not aligned, reserve enough space to align it."
  ficlDictionaryAlign(ficlVmGetDictionary(vm));
}


/**************************************************************************
 **                      a l i g n e d
 ** 
 **************************************************************************/
static void ficlPrimitiveAligned(ficlVm *vm)
{
  void *addr;

  FICL_STACK_CHECK(vm->dataStack, 1, 1);
  addr = ficlStackPopPointer(vm->dataStack);
  ficlStackPushPointer(vm->dataStack, ficlAlignPointer(addr));
}

/**************************************************************************
 **                      b e g i n   &   f r i e n d s
 ** Indefinite loop control structures
 ** A.6.1.0760 BEGIN 
 ** Typical use: 
 **      : X ... BEGIN ... test UNTIL ;
 ** or 
 **      : X ... BEGIN ... test WHILE ... REPEAT ;
 **************************************************************************/
static void ficlPrimitiveBeginCoIm(ficlVm *vm)
{
  markBranch(ficlVmGetDictionary(vm), vm, destTag);
}

static void ficlPrimitiveUntilCoIm(ficlVm *vm)
{
  ficlDictionary *dict = ficlVmGetDictionary(vm);

  PRIMITIVE_APPEND_UNSIGNED(dict, ficlInstructionBranch0ParenWithCheck);
  resolveBackBranch(dict, vm, destTag);
}

static void ficlPrimitiveWhileCoIm(ficlVm *vm)
{
  ficlDictionary *dict = ficlVmGetDictionary(vm);

  FICL_STACK_CHECK(vm->dataStack, 2, 5);
  PRIMITIVE_APPEND_UNSIGNED(dict, ficlInstructionBranch0ParenWithCheck);
  markBranch(dict, vm, origTag);
  /* equivalent to 2swap */
  ficlStackRoll(vm->dataStack, 3L);
  ficlStackRoll(vm->dataStack, 3L);
  PRIMITIVE_APPEND_UNSIGNED(dict, 1UL);
}

static void ficlPrimitiveRepeatCoIm(ficlVm *vm)
{
  ficlDictionary *dict = ficlVmGetDictionary(vm);

  PRIMITIVE_APPEND_UNSIGNED(dict, ficlInstructionBranchParenWithCheck);
  /* expect "begin" branch marker */
  resolveBackBranch(dict, vm, destTag);
  /* expect "while" branch marker */
  resolveForwardBranch(dict, vm, origTag);
}


static void ficlPrimitiveAgainCoIm(ficlVm *vm)
{
  ficlDictionary *dict = ficlVmGetDictionary(vm);

  PRIMITIVE_APPEND_UNSIGNED(dict, ficlInstructionBranchParenWithCheck);
  /* expect "begin" branch marker */
  resolveBackBranch(dict, vm, destTag);
}

/**************************************************************************
 **                      c h a r   &   f r i e n d s
 ** 6.1.0895 CHAR    CORE ( "<spaces>name" -- char )
 ** Skip leading space delimiters. Parse name delimited by a space.
 ** Put the value of its first character onto the stack. 
 **
 ** bracket-char     CORE 
 ** Interpretation: Interpretation semantics for this word are undefined.
 ** Compilation: ( "<spaces>name" -- )
 ** Skip leading space delimiters. Parse name delimited by a space.
 ** Append the run-time semantics given below to the current definition. 
 ** Run-time: ( -- char )
 ** Place char, the value of the first character of name, on the stack. 
 **************************************************************************/
static void ficlPrimitiveChar(ficlVm *vm)
{
#define h_ficlPrimitiveChar "( \"<spaces>name\" -- char )  \
Skip leading space delimiters. Parse name delimited by a space.  \
Put the value of its first character onto the stack."
  ficlString s;

  FICL_STACK_CHECK(vm->dataStack, 0, 1);
  s = ficlVmGetWord(vm);
  ficlStackPushUnsigned(vm->dataStack, (ficlUnsigned)(s.text[0]));
}

static void ficlPrimitiveCharCoIm(ficlVm *vm)
{
#define h_ficlPrimitiveCharCoIm "\
Interpretation: Interpretation semantics for this word are undefined.\n\
Compilation: ( \"<spaces>name\" -- )\n\
Skip leading space delimiters.  \
Parse name delimited by a space.  \
Append the run-time semantics given below to the current definition.\n\
Run-time: ( -- char )\n\
Place char, the value of the first character of name, on the stack."
  ficlPrimitiveChar(vm);
  ficlPrimitiveLiteralIm(vm);
}

static void
ficl_char_stateless_im(ficlVm *vm)
{
  ficlPrimitiveChar(vm);

  if (vm->state == FICL_VM_STATE_COMPILE)
    ficlPrimitiveLiteralIm(vm);
}

/**************************************************************************
 **                      c h a r P l u s
 ** char-plus        CORE ( c-addr1 -- c-addr2 )
 ** Add the size in address units of a character to c-addr1, giving c-addr2. 
 **************************************************************************/
static void ficlPrimitiveCharPlus(ficlVm *vm)
{
#define h_ficlPrimitiveCharPlus "( c-addr1 -- c-addr2 )  \
Add the size in address units of a character to C-ADDR1, giving C-ADDR2."
  char *p;

  FICL_STACK_CHECK(vm->dataStack, 1, 1);
  p = ficlStackPopPointer(vm->dataStack);
  ficlStackPushPointer(vm->dataStack, p + 1);
}

/**************************************************************************
 **                      c h a r s
 ** chars        CORE ( n1 -- n2 )
 ** n2 is the size in address units of n1 characters. 
 ** For most processors, this function can be a no-op. To guarantee
 ** portability, we'll multiply by sizeof (char).
 **************************************************************************/
#if defined (_M_IX86)
#pragma warning(disable: 4127)
#endif
static void ficlPrimitiveChars(ficlVm *vm)
{
#define h_ficlPrimitiveChars "( n1 -- n2 )  \
N2 is the size in address units of n1 characters.  \
For most processors, this function can be a no-op.  \
To guarantee portability, we'll multiply by sizeof(char)."
  if (sizeof(char) > 1)
  {
    ficlInteger i;

    FICL_STACK_CHECK(vm->dataStack, 1, 1);
    i = ficlStackPopInteger(vm->dataStack);
    ficlStackPushInteger(vm->dataStack, i * (ficlInteger)sizeof(char));
  }
  /* otherwise no-op! */
}
#if defined (_M_IX86)
#pragma warning(default: 4127)
#endif

/**************************************************************************
 **                      c o u n t
 ** COUNT    CORE ( c-addr1 -- c-addr2 u )
 ** Return the character string specification for the counted string stored
 ** at c-addr1. c-addr2 is the address of the first character after c-addr1.
 ** u is the contents of the character at c-addr1, which is the length in
 ** characters of the string at c-addr2. 
 **************************************************************************/
static void ficlPrimitiveCount(ficlVm *vm)
{
#define h_ficlPrimitiveCount "( c-addr1 -- c-addr2 u )  \
Return the character string specification for the counted string stored \
at C-ADDR1.  C-ADDR2 is the address of the first character after c-addr1.  \
U is the contents of the character at C-ADDR1, which is the length in \
characters of the string at C-ADDR2."
  ficlCountedString *counted;

  FICL_STACK_CHECK(vm->dataStack, 1, 2);
  counted = ficlStackPopPointer(vm->dataStack);
  ficlStackPushPointer(vm->dataStack, counted->text);
  ficlStackPushUnsigned(vm->dataStack, counted->length);
}

/**************************************************************************
 **                      e n v i r o n m e n t ?
 ** environment-query CORE ( c-addr u -- FICL_FALSE | i*x FICL_TRUE )
 ** c-addr is the address of a character string and u is the string's
 ** character count. u may have a value in the range from zero to an
 ** implementation-defined maximum which shall not be less than 31. The
 ** character string should contain a keyword from 3.2.6 Environmental
 ** queries or the optional word sets to be checked for correspondence
 ** with an attribute of the present environment. If the system treats the
 ** attribute as unknown, the returned flag is FICL_FALSE; otherwise, the flag
 ** is FICL_TRUE and the i*x returned is of the type specified in the table for
 ** the attribute queried. 
 **************************************************************************/
static void ficlPrimitiveEnvironmentQ(ficlVm *vm)
{
#define h_ficlPrimitiveEnvironmentQ "( c-addr u -- FICL_FALSE | i*x FICL_TRUE )  \
C-ADDR is the address of a character string and u is the string's \
character count.  U may have a value in the range from zero to an \
implementation-defined maximum which shall not be less than 31.  The \
character string should contain a keyword from 3.2.6 Environmental \
queries or the optional word sets to be checked for correspondence \
with an attribute of the present environment.  If the system treats the \
attribute as unknown, the returned flag is FICL_FALSE; otherwise, the flag \
is FICL_TRUE and the i*x returned is of the type specified in the table for \
the attribute queried."
  ficlDictionary *environment;
  ficlWord *word;
  ficlString name;
  ficlInteger flag;

  FICL_STACK_CHECK(vm->dataStack, 2, 1);
  environment = vm->callback.system->environment;
  name.length = ficlStackPopUnsigned(vm->dataStack);
  name.text   = ficlStackPopPointer(vm->dataStack);
  word = ficlDictionaryLookup(environment, name);

  if (word != NULL)
  {
    flag = FICL_TRUE;
    ficlVmExecuteWord(vm, word);
  }
  else
    flag = FICL_FALSE;
  ficlStackPushInteger(vm->dataStack, flag);
}

/**************************************************************************
 **                      e v a l u a t e
 ** EVALUATE CORE ( i*x c-addr u -- j*x )
 ** Save the current input source specification. Store minus-one (-1) in
 ** SOURCE-ID if it is present. Make the string described by c-addr and u
 ** both the input source and input buffer, set >IN to zero, and FICL_VM_STATE_INTERPRET.
 ** When the parse area is empty, restore the prior input source
 ** specification. Other stack effects are due to the words EVALUATEd. 
 **
 **************************************************************************/
static void ficlPrimitiveEvaluate(ficlVm *vm)
{
#define h_ficlPrimitiveEvaluate "( i*x c-addr u -- j*x )  \
Save the current input source specification.  Store minus-one (-1) in \
SOURCE-ID if it is present.  Make the string described by C-ADDR and U \
both the input source and input buffer, set >IN to zero, and FICL_VM_STATE_INTERPRET.  \
When the parse area is empty, restore the prior input source \
specification.  Other stack effects are due to the words EVALUATEd."
  ficlCell id;
  int result;
  ficlString string;

  FICL_STACK_CHECK(vm->dataStack, 2, 0);
  FICL_STRING_SET_LENGTH(string, ficlStackPopUnsigned(vm->dataStack));
  FICL_STRING_SET_POINTER(string, ficlStackPopPointer(vm->dataStack));
  id = vm->sourceId;
  CELL_INT_SET(&vm->sourceId, -1);
  result = ficlVmExecuteString(vm, string);
  vm->sourceId = id;

  if (result != FICL_VM_STATUS_OUT_OF_TEXT)
    ficlVmThrow(vm, result);
}

/**************************************************************************
 **                      s t r i n g   q u o t e
 ** Interpreting: get string delimited by a quote from the input stream,
 ** copy to a scratch area, and put its count and address on the stack.
 ** Compiling: FICL_VM_STATE_COMPILE code to push the address and count of a string
 ** literal, FICL_VM_STATE_COMPILE the string from the input stream, and align the dictionary
 ** pointer.
 **************************************************************************/
static void ficlPrimitiveStringQuoteIm(ficlVm *vm)
{
#define h_ficlPrimitiveStringQuoteIm "\
Interpreting: get string delimited by a quote from the input stream, \
copy to a scratch area, and put its count and address on the stack.\n\
Compiling: FICL_VM_STATE_COMPILE code to push the address and count of a string \
literal, FICL_VM_STATE_COMPILE the string from the input stream, and align the dictionary pointer."
  ficlDictionary *dict = ficlVmGetDictionary(vm);

  if (vm->state == FICL_VM_STATE_INTERPRET)
  {
    ficlCountedString *counted = (ficlCountedString *)dict->here;
    
    ficlVmGetString(vm, counted, '\"');
    ficlStackPushPointer(vm->dataStack, counted->text);
    ficlStackPushUnsigned(vm->dataStack, counted->length);
  }
  else    /* FICL_VM_STATE_COMPILE state */
  {
    PRIMITIVE_APPEND_UNSIGNED(dict, ficlInstructionStringLiteralParen);
    dict->here = FICL_POINTER_TO_CELL(ficlVmGetString(vm, (ficlCountedString *)dict->here, '\"'));
    ficlDictionaryAlign(dict);
  }
}

/**************************************************************************
 **                      t y p e
 ** Pop count and char address from stack and print the designated string.
 **************************************************************************/
static void ficlPrimitiveType(ficlVm *vm)
{
#define h_ficlPrimitiveType "( c-addr u -- )  \
Pop count and char address from stack and print the designated string."
  ficlUnsigned length;
  char *s;

  FICL_STACK_CHECK(vm->dataStack, 2, 0);
  length = ficlStackPopUnsigned(vm->dataStack);
  s = ficlStackPopPointer(vm->dataStack);
	
  if ((s == NULL) || (length == 0))
    return;

  /* 
  ** Since we don't have an output primitive for a counted string
  ** (oops), make sure the string is null terminated. If not, copy
  ** and terminate it.
  */
  if ((int)s[length] != 0)
  {
    char *here = (char *)ficlVmGetDictionary(vm)->here;

    if (s != here)
      strncpy(here, s, length);

    here[length] = '\0';
    s = here;
  }
  fth_print(s);
}

/**************************************************************************
 **                      w o r d
 ** word CORE ( char "<chars>ccc<char>" -- c-addr )
 ** Skip leading delimiters. Parse characters ccc delimited by char. An
 ** ambiguous condition exists if the length of the parsed string is greater
 ** than the implementation-defined length of a counted string. 
 ** 
 ** c-addr is the address of a transient region containing the parsed word
 ** as a counted string. If the parse area was empty or contained no
 ** characters other than the delimiter, the resulting string has a zero
 ** length. A space, not included in the length, follows the string. A
 ** program may replace characters within the string. 
 ** NOTE! Ficl also NULL-terminates the dest string.
 **************************************************************************/
static void ficlPrimitiveWord(ficlVm *vm)
{
#define h_ficlPrimitiveWord "( char \"<chars>ccc<char>\" -- c-addr )  \
Skip leading delimiters.  Parse characters ccc delimited by char.  \
An ambiguous condition exists if the length of the parsed string is greater \
than the implementation-defined length of a counted string.\n\
C-ADDR is the address of a transient region containing the parsed word \
as a counted string.  If the parse area was empty or contained no \
characters other than the delimiter, the resulting string has a zero \
length.  A space, not included in the length, follows the string.  \
A program may replace characters within the string.\n\
NOTE! Ficl also NULL-terminates the dest string."
  ficlCountedString *counted;
  char delim;
  ficlString name;

  FICL_STACK_CHECK(vm->dataStack, 1, 1);
  counted = (ficlCountedString *)vm->pad;
  delim = (char)ficlStackPopInteger(vm->dataStack);
  name = ficlVmParseStringEx(vm, delim, 1);

  if (FICL_STRING_GET_LENGTH(name) > FICL_PAD_SIZE - 1)
    FICL_STRING_SET_LENGTH(name, FICL_PAD_SIZE - 1);

  counted->length = FICL_STRING_GET_LENGTH(name);
  strncpy(counted->text, FICL_STRING_GET_POINTER(name), FICL_STRING_GET_LENGTH(name));
  /* store an extra space at the end of the primitive... why? dunno yet.  Guy Carver did it. */
  counted->text[counted->length] = ' ';
  counted->text[counted->length + 1] = 0;
  ficlStackPushPointer(vm->dataStack, counted);
}

/**************************************************************************
 **                      p a r s e - w o r d
 ** Ficl   PARSE-WORD  ( <spaces>name -- c-addr u )
 ** Skip leading spaces and parse name delimited by a space. c-addr is the
 ** address within the input buffer and u is the length of the selected 
 ** string. If the parse area is empty, the resulting string has a zero length.
 **************************************************************************/
static void ficlPrimitiveParseNoCopy(ficlVm *vm)
{
#define h_ficlPrimitiveParseNoCopy "( <spaces>name -- c-addr u )  \
Skip leading spaces and parse name delimited by a space.  C-ADDR is the \
address within the input buffer and U is the length of the selected string.  \
If the parse area is empty, the resulting string has a zero length."
  ficlString s;

  FICL_STACK_CHECK(vm->dataStack, 0, 2);
  s = ficlVmGetWord0(vm);
  ficlStackPushPointer(vm->dataStack, FICL_STRING_GET_POINTER(s));
  ficlStackPushUnsigned(vm->dataStack, FICL_STRING_GET_LENGTH(s));
}

/**************************************************************************
 **                      p a r s e
 ** CORE EXT  ( char "ccc<char>" -- c-addr u )
 ** Parse ccc delimited by the delimiter char. 
 ** c-addr is the address (within the input buffer) and u is the length of 
 ** the parsed string. If the parse area was empty, the resulting string has
 ** a zero length. 
 ** NOTE! PARSE differs from WORD: it does not skip leading delimiters.
 **************************************************************************/
static void ficlPrimitiveParse(ficlVm *vm)
{
#define h_ficlPrimitiveParse "( char \"ccc<char>\" -- c-addr u )  \
Parse CCC delimited by the delimiter char.  \
C-ADDR is the address (within the input buffer) and U is the length of the parsed string.  \
If the parse area was empty, the resulting string has a zero length.\n\
NOTE! PARSE differs from WORD: it does not skip leading delimiters."
  ficlString s;
  char delim;

  FICL_STACK_CHECK(vm->dataStack, 1, 2);
  delim = (char)ficlStackPopInteger(vm->dataStack);
  s = ficlVmParseStringEx(vm, delim, 0);
  ficlStackPushPointer(vm->dataStack, FICL_STRING_GET_POINTER(s));
  ficlStackPushUnsigned(vm->dataStack, FICL_STRING_GET_LENGTH(s));
}

/**************************************************************************
 **                      f i n d
 ** FIND CORE ( c-addr -- c-addr 0  |  xt 1  |  xt -1 )
 ** Find the definition named in the counted string at c-addr. If the
 ** definition is not found, return c-addr and zero. If the definition is
 ** found, return its execution token xt. If the definition is immediate,
 ** also return one (1), otherwise also return minus-one (-1). For a given
 ** string, the values returned by FIND while compiling may differ from
 ** those returned while not compiling. 
 **************************************************************************/
static void do_find(ficlVm *vm, ficlString name, void *returnForFailure)
{
  ficlWord *word;

  word = ficlDictionaryLookup(ficlVmGetDictionary(vm), name);

  if (word)
  {
    ficlStackPushPointer(vm->dataStack, word);
    ficlStackPushInteger(vm->dataStack, (ficlWordIsImmediate(word) ? 1L : -1L));
  }
  else
  {
    ficlStackPushPointer(vm->dataStack, returnForFailure);
    ficlStackPushUnsigned(vm->dataStack, 0UL);
  }
}

/**************************************************************************
 **                      f i n d
 ** FIND CORE ( c-addr -- c-addr 0  |  xt 1  |  xt -1 )
 ** Find the definition named in the counted string at c-addr. If the
 ** definition is not found, return c-addr and zero. If the definition is
 ** found, return its execution token xt. If the definition is immediate,
 ** also return one (1), otherwise also return minus-one (-1). For a given
 ** string, the values returned by FIND while compiling may differ from
 ** those returned while not compiling. 
 **************************************************************************/
static void ficlPrimitiveCFind(ficlVm *vm)
{
#define h_ficlPrimitiveCFind "( c-addr -- c-addr 0  |  xt 1  |  xt -1 )  \
Find the definition named in the counted string at C-ADDR.  If the \
definition is not found, return C-ADDR and zero.  If the definition is \
found, return its execution token XT.  If the definition is immediate, \
also return one (1), otherwise also return minus-one (-1).  For a given \
string, the values returned by FIND while compiling may differ from \
those returned while not compiling."
  ficlCountedString *counted;
  ficlString name;

  FICL_STACK_CHECK(vm->dataStack, 1, 2);
  counted = ficlStackPopPointer(vm->dataStack);
  FICL_STRING_SET_FROM_COUNTED_STRING(name, *counted);
  do_find(vm, name, counted);
}

/**************************************************************************
 **                        s f i n d
 ** Ficl   ( c-addr u -- 0 0  |  xt 1  |  xt -1 )
 ** Like FIND, but takes "c-addr u" for the string.
 **************************************************************************/
static void ficlPrimitiveSFind(ficlVm *vm)
{
#define h_ficlPrimitiveSFind "( c-addr u -- 0 0  |  xt 1  |  xt -1 )  \
Like FIND, but takes C-ADDR U for the string."
  ficlString name;

  FICL_STACK_CHECK(vm->dataStack, 2, 2);
  name.length = ficlStackPopUnsigned(vm->dataStack);
  name.text = ficlStackPopPointer(vm->dataStack);
  do_find(vm, name, NULL);
}

/**************************************************************************
 **                      r e c u r s e
 ** 
 **************************************************************************/
static void ficlPrimitiveRecurseCoIm(ficlVm *vm)
{
  ficlDictionary *dict = ficlVmGetDictionary(vm);
  
  ficlDictionaryAppendPointer(dict, dict->smudge);
}

/**************************************************************************
 **                      s o u r c e
 ** CORE ( -- c-addr u )
 ** c-addr is the address of, and u is the number of characters in, the
 ** input buffer. 
 **************************************************************************/
static void ficlPrimitiveSource(ficlVm *vm)
{
#define h_ficlPrimitiveSource "( -- c-addr u )  \
C-ADDR is the address of, and U is the number of characters in, the input buffer."
  FICL_STACK_CHECK(vm->dataStack, 0, 2);
  ficlStackPushPointer(vm->dataStack, vm->tib.text);
  ficlStackPushInteger(vm->dataStack, ficlVmGetInBufLen(vm));
}

/**************************************************************************
 **                      v e r s i o n
 ** non-standard...
 **************************************************************************/
/* ARGSUSED */
static void ficlPrimitiveVersion(ficlVm *vm)
{
  fth_print("Ficl version " FICL_VERSION "\n");
}

/**************************************************************************
 **                      t o I n
 ** to-in CORE
 **************************************************************************/
static void ficlPrimitiveToIn(ficlVm *vm)
{
  FICL_STACK_CHECK(vm->dataStack, 0, 1);
  ficlStackPushPointer(vm->dataStack, &vm->tib.index);
}

/**************************************************************************
 **                      c o l o n N o N a m e
 ** CORE EXT ( C:  -- colon-sys )  ( S:  -- xt )
 ** Create an unnamed colon definition and push its address.
 ** Change state to FICL_VM_STATE_COMPILE.
 **************************************************************************/
static void ficlPrimitiveColonNoName(ficlVm *vm)
{
#define h_ficlPrimitiveColonNoName "( C:  -- colon-sys )  ( S:  -- xt )  \
Create an unnamed colon definition and push its address.  \
Change state to FICL_VM_STATE_COMPILE."
  ficlWord *word;
  ficlString name;

  FICL_STRING_SET_LENGTH(name, 0);
  FICL_STRING_SET_POINTER(name, NULL);
  vm->state = FICL_VM_STATE_COMPILE;
  word = ficlDictionaryAppendWord(ficlVmGetDictionary(vm),
				  name,
				  (ficlPrimitive)ficlInstructionColonParen,
				  FICL_WORD_DEFAULT | FICL_WORD_SMUDGED);
  ficlStackPushPointer(vm->dataStack, word);
  markControlTag(vm, colonTag);
}

/**************************************************************************
 **                      u s e r   V a r i a b l e
 ** user  ( u -- )  "<spaces>name"  
 ** Get a name from the input stream and create a user variable
 ** with the name and the index supplied. The run-time effect
 ** of a user variable is to push the address of the indexed ficlCell
 ** in the running vm's user array. 
 **
 ** User variables are vm local cells. Each vm has an array of
 ** FICL_USER_CELLS of them when FICL_WANT_USER is nonzero.
 ** Ficl's user facility is implemented with two primitives,
 ** "user" and "(user)", a variable ("nUser") (in softcore.c) that 
 ** holds the index of the next free user ficlCell, and a redefinition
 ** (also in softcore) of "user" that defines a user word and increments
 ** nUser.
 **************************************************************************/
static void ficlPrimitiveUser(ficlVm *vm)
{
#define h_ficlPrimitiveUser "( u -- )  \
Get a name from the input stream and create a user variable \
with the name and the index supplied.  The run-time effect \
of a user variable is to push the address of the indexed ficlCell \
in the running vm's user array.\n\
User variables are vm local cells.  Each vm has an array of \
FICL_USER_CELLS of them when FICL_WANT_USER is nonzero.  \
Ficl's user facility is implemented with two primitives, \
USER and (USER), a variable (nUSER) (in softcore.c) that \
holds the index of the next free user ficlCell, and a redefinition \
(also in softcore) of USER that defines a user word and increments nUser."
  ficlDictionary *dict;
  ficlString s;
  ficlInteger n;
 
  dict = ficlVmGetDictionary(vm);
  s = ficlVmGetWord(vm);
  FICL_STACK_CHECK(vm->dataStack, 1, 0);
  n = ficlStackPopInteger(vm->dataStack);
  if (n >= FICL_USER_CELLS)
    ficlVmThrowError(vm, "out of user space (%ld)", n);
  ficlDictionaryAppendWord(dict,
      s,
      (ficlPrimitive)ficlInstructionUserParen,
      FICL_WORD_DEFAULT);
  ficlDictionaryAppendInteger(dict, n);
}

/*
** Each local is recorded in a private locals dictionary as a 
** word that does doLocalIm at runtime. DoLocalIm compiles code
** into the client definition to fetch the value of the 
** corresponding local variable from the return stack.
** The private dictionary gets initialized at the end of each block
** that uses locals (in ; and does> for example).
*/
static void ficlLocalParenIm(ficlVm *vm)
{
#define h_ficlLocalParenIm "\
Each local is recorded in a private locals dictionary as a \
word that does doLocalIm at runtime.  DoLocalIm compiles code \
into the client definition to fetch the value of the \
corresponding local variable from the return stack.  \
The private dictionary gets initialized at the end of each block \
that uses locals (in ; and does> for example)."
  ficlInteger nLocal;

  nLocal = CELL_INT_REF(vm->runningWord->param);
  if (vm->state == FICL_VM_STATE_INTERPRET)
    ficlStackPush(vm->dataStack, vm->returnStack->frame[nLocal]);
  else
  {
    ficlInstruction instruction;
    ficlInteger appendLocalOffset;

    if (nLocal == 0)
    {
      instruction = ficlInstructionGetLocal0;
      appendLocalOffset = FICL_FALSE;
    }
    else if (nLocal == 1)
    {
      instruction = ficlInstructionGetLocal1;
      appendLocalOffset = FICL_FALSE;
    }
    else
    {
      instruction = ficlInstructionGetLocalParen;
      appendLocalOffset = FICL_TRUE;
    }
    PRIMITIVE_APPEND_UNSIGNED(ficlVmGetDictionary(vm), instruction);
    if (appendLocalOffset)
      ficlDictionaryAppendInteger(ficlVmGetDictionary(vm), nLocal);
  }
}

/**************************************************************************
 **                      l o c a l P a r e n
 ** paren-local-paren LOCAL 
 ** Interpretation: Interpretation semantics for this word are undefined.
 ** Execution: ( c-addr u -- )
 ** When executed during compilation, (LOCAL) passes a message to the 
 ** system that has one of two meanings. If u is non-zero,
 ** the message identifies a new local whose definition name is given by
 ** the string of characters identified by c-addr u. If u is zero,
 ** the message is last local and c-addr has no significance. 
 **
 ** The result of executing (LOCAL) during compilation of a definition is
 ** to create a set of named local identifiers, each of which is
 ** a definition name, that only have execution semantics within the scope
 ** of that definition's source. 
 **
 ** local Execution: ( -- x )
 **
 ** Push the local's value, x, onto the stack. The local's value is
 ** initialized as described in 13.3.3 Processing locals and may be
 ** changed by preceding the local's name with TO. An ambiguous condition
 ** exists when local is executed while in interpretation state. 
 **************************************************************************/
static void ficlLocalParen(ficlVm *vm)
{
#define h_ficlLocalParen "\
Interpretation: Interpretation semantics for this word are undefined.\n\
Execution: ( c-addr u -- )\n\
When executed during compilation, (LOCAL) passes a message to the \
system that has one of two meanings.  If U is non-zero, \
the message identifies a new local whose definition name is given by \
the string of characters identified by C-ADDR U.  If U is zero, \
the message is last local and C-ADDR has no significance.\n\
The result of executing (LOCAL) during compilation of a definition is \
to create a set of named local identifiers, each of which is \
a definition name, that only have execution semantics within the scope \
of that definition's source.\n\
local Execution: ( -- x )\n\
Push the local's value, x, onto the stack.  The local's value is \
initialized as described in 13.3.3 Processing locals and may be \
changed by preceding the local's name with TO.  An ambiguous condition \
exists when local is executed while in interpretation state."
  ficlDictionary *dict;
  ficlString name;

  FICL_STACK_CHECK(vm->dataStack, 2, 0);  
  dict = ficlVmGetDictionary(vm);
  FICL_STRING_SET_LENGTH(name, ficlStackPopUnsigned(vm->dataStack));
  FICL_STRING_SET_POINTER(name, (char *)ficlStackPopPointer(vm->dataStack));

  if (FICL_STRING_GET_LENGTH(name) > 0)
  {   /* add a local to the **locals** dictionary and update localsCount */
    ficlDictionary *locs = ficlSystemGetLocals(vm->callback.system);
	
    if (vm->callback.system->localsCount >= FICL_MAX_LOCALS)
      ficlVmThrowError(vm, "out of local space (%d)", (int)vm->callback.system->localsCount);

    ficlDictionaryAppendWord(locs, name, ficlLocalParenIm, FICL_WORD_COMPILE_ONLY_IMMEDIATE);
    ficlDictionaryAppendInteger(locs,  vm->callback.system->localsCount);

    if (vm->callback.system->localsCount == 0)
    {   /* FICL_VM_STATE_COMPILE code to create a local stack frame */
      PRIMITIVE_APPEND_UNSIGNED(dict, ficlInstructionLinkParen);
      /* save location in dictionary for #locals */
      vm->callback.system->localsFixup = dict->here;
      ficlDictionaryAppendInteger(dict, vm->callback.system->localsCount);
    }

    PRIMITIVE_APPEND_UNSIGNED(dict, ficlInstructionToLocalParen);
    ficlDictionaryAppendInteger(dict,  vm->callback.system->localsCount);
    vm->callback.system->localsCount += 1;
  }
  else if (vm->callback.system->localsCount > 0)
    /* write localsCount to (link) param area in dictionary */
    *(ficlInteger *)(vm->callback.system->localsFixup) = vm->callback.system->localsCount;
}

/**************************************************************************
 **                      t o V a l u e
 ** CORE EXT 
 ** Interpretation: ( x "<spaces>name" -- )
 ** Skip leading spaces and parse name delimited by a space. Store x in 
 ** name. An ambiguous condition exists if name was not defined by VALUE. 
 ** NOTE: In Ficl, VALUE is an alias of CONSTANT
 **************************************************************************/
static void ficlPrimitiveToValue(ficlVm *vm)
{
#define h_ficlPrimitiveToValue "( x \"<spaces>name\" -- )  \
Skip leading spaces and parse name delimited by a space.  \
Store x in name.  \
An ambiguous condition exists if name was not defined by VALUE.\n\
NOTE: In Ficl, VALUE is an alias of CONSTANT."
  ficlString      name = ficlVmGetWord(vm);
  ficlDictionary *dict = ficlVmGetDictionary(vm);
  ficlWord       *word;

  if (vm->callback.system->localsCount > 0)
  {
    ficlDictionary *locals            = ficlSystemGetLocals(vm->callback.system);
    ficlInstruction inst              = ficlInstructionToLocalParen;
    ficlInteger     appendLocalOffset = FICL_TRUE;
    ficlInteger     nLocal;

    word = ficlDictionaryLookup(locals, name);

    if (!word)
      goto TO_GLOBAL;

    if (word->code != ficlLocalParenIm)
    {
      ficlVmThrowError(vm, "local %.*s is of unknown type", (int)name.length, name.text);
      return;
    }

    nLocal = CELL_INT_REF(word->param);

    if (nLocal == 0)
    {
      inst = ficlInstructionToLocal0;
      appendLocalOffset = FICL_FALSE;
    }
    else if (nLocal == 1)
    {
      inst = ficlInstructionToLocal1;
      appendLocalOffset = FICL_FALSE;
    }
		
    PRIMITIVE_APPEND_UNSIGNED(dict, inst);

    if (appendLocalOffset)
      ficlDictionaryAppendInteger(dict, nLocal);
    return;
  }

TO_GLOBAL:
  word = ficlDictionaryLookup(dict, name);

  if (word)
  {
    if (vm->state == FICL_VM_STATE_INTERPRET)
    {
      FTH out = FTH_WORD_PARAM(word);

      word->param[0] = ficlStackPop(vm->dataStack);
      /* [ms] */
      fth_gc_protect_set(out, FTH_WORD_PARAM(word));

      if (FTH_TRACE_VAR_P(word))
	fth_trace_var_execute(word);
    }
    else        /* FICL_VM_STATE_COMPILE code to store to word's param */
    {
      ficlStackPushPointer(vm->dataStack, &word->param[0]);
      ficlPrimitiveLiteralIm(vm);

      if (word->code == (ficlPrimitive)ficlInstructionConstantParen)
      {
	if (FTH_TRACE_VAR_P(word))
	{
	  ficlStackPushPointer(vm->dataStack, word);
	  ficlPrimitiveLiteralIm(vm);
	  PRIMITIVE_APPEND_UNSIGNED(dict, ficl_execute_trace_var);
	}
	else
	  PRIMITIVE_APPEND_UNSIGNED(dict, ficlInstructionStore);
      }
      else
	ficlVmThrowError(vm, "value %.*s is of unknown type", (int)name.length, name.text);
    }
  }
  else
    ficlVmThrowException(vm, FICL_VM_STATUS_UNDEFINED, "%.*s", (int)name.length, name.text);
}

/* [ms] */
static void
ficlPrimitivePlusToValue(ficlVm *vm)
{
  ficlString      name = ficlVmGetWord(vm);
  ficlDictionary *dict = ficlVmGetDictionary(vm);
  ficlWord       *word;

  if (vm->callback.system->localsCount > 0)
  {
    ficlDictionary *locals            = ficlSystemGetLocals(vm->callback.system);
    ficlInstruction inst              = ficlInstructionPlusToLocalParen;
    ficlInteger     appendLocalOffset = FICL_TRUE;
    ficlInteger     nLocal;

    word = ficlDictionaryLookup(locals, name);

    if (!word)
      goto TO_GLOBAL;

    if (word->code != ficlLocalParenIm)
    {
      ficlVmThrowError(vm, "local %.*s is of unknown type", (int)name.length, name.text);
      return;
    }

    nLocal = CELL_INT_REF(word->param);

    if (nLocal == 0)
    {
      inst = ficlInstructionPlusToLocal0;
      appendLocalOffset = FICL_FALSE;
    }
    else if (nLocal == 1)
    {
      inst = ficlInstructionPlusToLocal1;
      appendLocalOffset = FICL_FALSE;
    }
		
    PRIMITIVE_APPEND_UNSIGNED(dict, inst);

    if (appendLocalOffset)
      ficlDictionaryAppendInteger(dict, nLocal);

    return;
  }

TO_GLOBAL:
  word = ficlDictionaryLookup(dict, name);

  if (word)
  {
    if (vm->state == FICL_VM_STATE_INTERPRET)
    {
      FTH x = CELL_FTH_REF(word->param);
      
      if (FTH_FLOAT_P(x))
	FTH_FLOAT_OBJECT(x) += fth_float_ref(fth_pop_ficl_cell(vm));
      else if (FTH_LLONG_P(x))
	FTH_LONG_OBJECT(x) += fth_long_long_ref(fth_pop_ficl_cell(vm));
      else
	CELL_INT_REF(word->param) += ficlStackPopInteger(vm->dataStack);
    }
    else        /* FICL_VM_STATE_COMPILE code to store to word's param */
    {
      ficlStackPushPointer(vm->dataStack, word->param);
      ficlPrimitiveLiteralIm(vm);
      PRIMITIVE_APPEND_UNSIGNED(dict, ficlInstructionPlusStore);
    }
  }
  else
    ficlVmThrowException(vm, FICL_VM_STATUS_UNDEFINED, "%.*s", (int)name.length, name.text);
}

/* [ms] */
static void
ficlPrimitiveFPlusToValue(ficlVm *vm)
{
  ficlString      name = ficlVmGetWord(vm);
  ficlDictionary *dict = ficlVmGetDictionary(vm);
  ficlWord       *word;

  if (vm->callback.system->localsCount > 0)
  {
    ficlDictionary *locals            = ficlSystemGetLocals(vm->callback.system);
    ficlInstruction inst              = ficlInstructionFPlusToLocalParen;
    ficlInteger     appendLocalOffset = FICL_TRUE;
    ficlInteger     nLocal;

    word = ficlDictionaryLookup(locals, name);

    if (!word)
      goto TO_GLOBAL;

    if (word->code != ficlLocalParenIm)
    {
      ficlVmThrowError(vm, "local %.*s is of unknown type", (int)name.length, name.text);
      return;
    }

    nLocal = CELL_INT_REF(word->param);

    if (nLocal == 0)
    {
      inst = ficlInstructionFPlusToLocal0;
      appendLocalOffset = FICL_FALSE;
    }
    else if (nLocal == 1)
    {
      inst = ficlInstructionFPlusToLocal1;
      appendLocalOffset = FICL_FALSE;
    }
		
    PRIMITIVE_APPEND_UNSIGNED(dict, inst);

    if (appendLocalOffset)
      ficlDictionaryAppendInteger(dict, nLocal);

    return;
  }

TO_GLOBAL:
  word = ficlDictionaryLookup(dict, name);

  if (word)
  {
    if (vm->state == FICL_VM_STATE_INTERPRET)
    {
      FTH x = CELL_FTH_REF(word->param);
      
      if (FTH_FLOAT_P(x))
	FTH_FLOAT_OBJECT(x) += fth_float_ref(fth_pop_ficl_cell(vm));
      else if (FTH_LLONG_P(x))
	FTH_LONG_OBJECT(x) += fth_long_long_ref(fth_pop_ficl_cell(vm));
      else
	CELL_INT_REF(word->param) += ficlStackPopInteger(vm->dataStack);
    }
    else        /* FICL_VM_STATE_COMPILE code to store to word's param */
    {
      ficlStackPushPointer(vm->dataStack, word->param);
      ficlPrimitiveLiteralIm(vm);
      PRIMITIVE_APPEND_UNSIGNED(dict, ficlInstructionFPlusStore);
    }
  }
  else
    ficlVmThrowException(vm, FICL_VM_STATUS_UNDEFINED, "%.*s", (int)name.length, name.text);
}

/**************************************************************************
 **				f m S l a s h M o d
 ** f-m-slash-mod CORE ( d1 n1 -- n2 n3 )
 ** Divide d1 by n1, giving the floored quotient n3 and the remainder n2.
 ** Input and output stack arguments are signed. An ambiguous condition
 ** exists if n1 is zero or if the quotient lies outside the range of a
 ** single-ficlCell signed integer. 
 **************************************************************************/
/* fm/mod ( d1 n1 -- n2 n3 ) */
static void ficlPrimitiveFMSlashMod(ficlVm *vm)
{
#define h_ficlPrimitiveFMSlashMod "( d1 n1 -- n2 n3 )  \
Divide D1 by N1, giving the floored quotient N3 and the remainder N2.  \
Input and output stack arguments are signed.  An ambiguous condition \
exists if N1 is zero or if the quotient lies outside the range of a \
single-ficlCell signed integer."
  ficl2Integer d1;
  ficlInteger n1, n2, n3;

  FICL_STACK_CHECK(vm->dataStack, 2, 2);
  n1 = ficlStackPopInteger(vm->dataStack);
  d1 = ficlStackPop2Integer(vm->dataStack);
  n2 = (ficlInteger)(d1 % n1);
  n3 = (ficlInteger)(d1 / n1);

  if (1 % -3 > 0 && (d1 < 0) != (n1 < 0) && n2 != 0)
  {
    n3--;
    n2 += n1;
  }

  ficlStackPushInteger(vm->dataStack, n2);
  ficlStackPushInteger(vm->dataStack, n3);
}

/**************************************************************************
 **				s m S l a s h R e m
 ** s-m-slash-remainder CORE ( d1 n1 -- n2 n3 )
 ** Divide d1 by n1, giving the symmetric quotient n3 and the remainder n2.
 ** Input and output stack arguments are signed. An ambiguous condition
 ** exists if n1 is zero or if the quotient lies outside the range of a
 ** single-ficlCell signed integer. 
 **************************************************************************/
/* sm/rem ( d1 n1 -- n2 n3 ) */
static void ficlPrimitiveSMSlashRem(ficlVm *vm)
{
#define h_ficlPrimitiveSMSlashRem "( d1 n1 -- n2 n3 )  \
Divide D1 by N1, giving the symmetric quotient N3 and the remainder N2.  \
Input and output stack arguments are signed.  An ambiguous condition \
exists if N1 is zero or if the quotient lies outside the range of a \
single-ficlCell signed integer."
  ficl2Integer d1;
  ficlInteger n1;
  ficl2IntegerQR qr;
	
  FICL_STACK_CHECK(vm->dataStack, 2, 2);
  n1 = ficlStackPopInteger(vm->dataStack);
  d1 = ficlStackPop2Integer(vm->dataStack);
  qr = ficl2IntegerDivideSymmetric(d1, n1);
  ficlStackPushInteger(vm->dataStack, qr.remainder);
  ficlStackPushInteger(vm->dataStack, (ficlInteger)qr.quotient);
}

/* mod ( n1 n2 -- n ) */
static void ficlPrimitiveMod(ficlVm *vm)
{
#define h_ficlPrimitiveMod "( n1 n2 -- n )  N is the result from N1 % N2 (modulus)."
  ficlInteger n1, n2;

  FICL_STACK_CHECK(vm->dataStack, 2, 1);
  n2 = ficlStackPopInteger(vm->dataStack);
  n1 = ficlStackPopInteger(vm->dataStack);
  ficlStackPushInteger(vm->dataStack, n1 % n2);
}

/**************************************************************************
 **				u m S l a s h M o d
 ** u-m-slash-mod CORE ( ud u1 -- u2 u3 )
 ** Divide ud by u1, giving the quotient u3 and the remainder u2.
 ** All values and arithmetic are unsigned. An ambiguous condition
 ** exists if u1 is zero or if the quotient lies outside the range of a
 ** single-ficlCell unsigned integer. 
 *************************************************************************/
/* um/mod ( ud u1 -- u2 u3 ) */
static void ficlPrimitiveUMSlashMod(ficlVm *vm)
{
#define h_ficlPrimitiveUMSlashMod "( ud u1 -- u2 u3 )  \
Divide UD by U1, giving the quotient U3 and the remainder U2.  \
All values and arithmetic are unsigned.  An ambiguous condition \
exists if U1 is zero or if the quotient lies outside the range of a \
single-ficlCell unsigned integer."
  ficl2Unsigned ud;
  ficlUnsigned u1;

  FICL_STACK_CHECK(vm->dataStack, 2, 2);
  u1 = ficlStackPopUnsigned(vm->dataStack);
  ud = ficlStackPop2Unsigned(vm->dataStack);
  ficlStackPushUnsigned(vm->dataStack, (ficlUnsigned)(ud / u1));
  ficlStackPushUnsigned(vm->dataStack, (ficlUnsigned)(ud % u1));
}

/**************************************************************************
 **				m S t a r
 ** m-star CORE ( n1 n2 -- d )
 ** d is the signed product of n1 times n2. 
 **************************************************************************/
/* m* ( n1 n2 -- d ) */
static void ficlPrimitiveMStar(ficlVm *vm)
{
#define h_ficlPrimitiveMStar "( n1 n2 -- d )  \
D is the signed product of N1 times N2."
  ficlInteger n1;
  ficlInteger n2;

  FICL_STACK_CHECK(vm->dataStack, 2, 1);
  n2 = ficlStackPopInteger(vm->dataStack);
  n1 = ficlStackPopInteger(vm->dataStack);
  ficlStackPush2Integer(vm->dataStack, (ficl2Integer)(n1 * n2));
}

/* um* ( u1 u2 -- ud ) */
static void ficlPrimitiveUMStar(ficlVm *vm)
{
#define h_ficlPrimitiveUMStar "( u1 u2 -- ud )  \
UD is the unsigned product of U1 times U2."
  ficlUnsigned u1;
  ficlUnsigned u2;

  FICL_STACK_CHECK(vm->dataStack, 2, 1);
  u2 = ficlStackPopUnsigned(vm->dataStack);
  u1 = ficlStackPopUnsigned(vm->dataStack);
  ficlStackPush2Unsigned(vm->dataStack, (ficl2Unsigned)u1 * (ficl2Unsigned)u2);
}

/**************************************************************************
 **                      p a d
 ** CORE EXT  ( -- c-addr )
 ** c-addr is the address of a transient region that can be used to hold
 ** data for intermediate processing.
 **************************************************************************/
static void ficlPrimitivePad(ficlVm *vm)
{
#define h_ficlPrimitivePad "( -- c-addr )  \
C-ADDR is the address of a transient region that can be used \
to hold data for intermediate processing."
  ficlStackPushPointer(vm->dataStack, vm->pad);
}

/**************************************************************************
 **                      s o u r c e - i d
 ** CORE EXT, FILE   ( -- 0 | -1 | fileid )
 **    Identifies the input source as follows:
 **
 ** SOURCE-ID       Input source
 ** ---------       ------------
 ** fileid          Text file fileid
 ** -1              String (via EVALUATE)
 ** 0               User input device
 **************************************************************************/
static void ficlPrimitiveSourceID(ficlVm *vm)
{
#define h_ficlPrimitiveSourceID "( -- 0 | -1 | fileid )  \
Identifies the input source as follows:\n\
SOURCE-ID       Input source\n\
---------       ------------\n\
fileid          Text file fileid\n\
-1              String (via EVALUATE)\n\
0               User input device"
  ficlStackPushInteger(vm->dataStack, CELL_INT_REF(&vm->sourceId));
}

/**************************************************************************
 **                      r e f i l l
 ** CORE EXT   ( -- flag )
 ** Attempt to fill the input buffer from the input source, returning a FICL_TRUE
 ** flag if successful. 
 ** When the input source is the user input device, attempt to receive input
 ** into the terminal input buffer. If successful, make the result the input
 ** buffer, set >IN to zero, and return FICL_TRUE. Receipt of a line containing no
 ** characters is considered successful. If there is no input available from
 ** the current input source, return FICL_FALSE. 
 ** When the input source is a string from EVALUATE, return FICL_FALSE and
 ** perform no other action. 
 **************************************************************************/
static void ficlPrimitiveRefill(ficlVm *vm)
{
#define h_ficlPrimitiveRefill "( -- flag )  \
Attempt to fill the input buffer from the input source, returning a FICL_TRUE flag if successful.  \
When the input source is the user input device, attempt to receive input \
into the terminal input buffer.  If successful, make the result the input \
buffer, set >IN to zero, and return FICL_TRUE.  Receipt of a line containing no \
characters is considered successful.  If there is no input available from \
the current input source, return FICL_FALSE.\n\
When the input source is a string from EVALUATE, return FICL_FALSE and \
perform no other action."
  ficlInteger ret = (CELL_INT_REF(&vm->sourceId) == -1) ? FICL_FALSE : FICL_TRUE;

  if (ret && (vm->restart == 0))
    ficlVmThrow(vm, FICL_VM_STATUS_RESTART);
  ficlStackPushInteger(vm->dataStack, ret);
}

/**************************************************************************
 **                      freebsd exception handling words
 ** Catch, from ANS Forth standard. Installs a safety net, then EXECUTE
 ** the word in ToS. If an exception happens, restore the state to what
 ** it was before, and pushes the exception value on the stack. If not,
 ** push zero.
 **
 ** Notice that Catch implements an inner interpreter. This is ugly,
 ** but given how Ficl works, it cannot be helped. The problem is that
 ** colon definitions will be executed *after* the function returns,
 ** while "code" definitions will be executed immediately. I considered
 ** other solutions to this problem, but all of them shared the same
 ** basic problem (with added disadvantages): if Ficl ever changes it's
 ** inner thread modus operandi, one would have to fix this word.
 **
 ** More comments can be found throughout catch's code.
 **
 ** Daniel C. Sobral Jan 09/1999
 ** sadler may 2000 -- revised to follow ficl.c:ficlExecXT.
 **************************************************************************/
static void ficlPrimitiveCatch(ficlVm *vm)
{
#define h_ficlPrimitiveCatch "\
Catch, from ANS Forth standard.  Installs a safety net, then EXECUTE \
the word in ToS.  If an exception happens, restore the state to what \
it was before, and pushes the exception value on the stack.  If not, push zero.\n\
Notice that Catch implements an inner interpreter.  This is ugly, \
but given how Ficl works, it cannot be helped.  The problem is that \
colon definitions will be executed *after* the function returns, \
while CODE definitions will be executed immediately.  I considered \
other solutions to this problem, but all of them shared the same \
basic problem (with added disadvantages): if Ficl ever changes it's \
inner thread modus operandi, one would have to fix this word.\n\
Daniel C. Sobral Jan 09/1999\n\
sadler may 2000 -- revised to follow ficl.c:ficlExecXT."
  jmp_buf vmState;
  volatile int except;
  volatile ficlVm vmCopy;
  volatile ficlStack dataStackCopy;
  volatile ficlStack returnStackCopy;
  ficlWord *volatile word;

  FICL_ASSERT(vm != NULL);
  FICL_ASSERT(vm->callback.system->exitInnerWord != NULL);
  FICL_STACK_CHECK(vm->dataStack, 1, 0);

  /*
  ** Get xt.
  ** We need this *before* we save the stack pointer, or
  ** we'll have to pop one element out of the stack after
  ** an exception. I prefer to get done with it up front. :-)
  */
  word = ficlStackPopPointer(vm->dataStack);

  /* 
  ** Save vm's state -- a catch will not back out environmental
  ** changes.
  **
  ** We are *not* saving dictionary state, since it is
  ** global instead of per vm, and we are not saving
  ** stack contents, since we are not required to (and,
  ** thus, it would be useless). We save vm, and vm
  ** "stacks" (a structure containing general information
  ** about it, including the current stack pointer).
  */
  memcpy((void*)&vmCopy, (void*)vm, sizeof(ficlVm));
  memcpy((void*)&dataStackCopy, (void*)vm->dataStack, sizeof(ficlStack));
  memcpy((void*)&returnStackCopy, (void*)vm->returnStack, sizeof(ficlStack));

  /*
  ** Give vm a jmp_buf
  */
  vm->exceptionHandler = &vmState;

  /*
  ** Safety net
  */
  except = setjmp(vmState);

  switch (except)
  {
    /*
    ** Setup condition - push poison pill so that the VM throws
    ** VM_INNEREXIT if the XT terminates normally, then execute
    ** the XT
    */
  case 0:
    ficlVmPushIP(vm, &(vm->callback.system->exitInnerWord)); /* Open mouth, insert emetic */
    ficlVmExecuteWord(vm, word);
    ficlVmInnerLoop(vm, 0);
    break;

    /*
    ** Normal exit from XT - lose the poison pill, 
    ** restore old setjmp vector and push a zero. 
    */
  case FICL_VM_STATUS_INNER_EXIT:
    ficlVmPopIP(vm);                   /* Gack - hurl poison pill */
    vm->exceptionHandler = vmCopy.exceptionHandler;        /* Restore just the setjmp vector */
    ficlStackPushInteger(vm->dataStack, 0L);   /* Push 0 -- everything is ok */
    break;

    /*
    ** Some other exception got thrown - restore pre-existing VM state
    ** and push the exception code
    */
  default:
    /* Restore vm's state */
    memcpy((void*)vm, (void*)&vmCopy, sizeof(ficlVm));
    memcpy((void*)vm->dataStack, (void*)&dataStackCopy, sizeof(ficlStack));
    memcpy((void*)vm->returnStack, (void*)&returnStackCopy, sizeof(ficlStack));

    ficlStackPushInteger(vm->dataStack, (ficlInteger)except);/* Push error */
    break;
  }
}

/**************************************************************************
 **                     t h r o w
 ** EXCEPTION
 ** Throw --  From ANS Forth standard.
 **
 ** Throw takes the ToS and, if that's different from zero,
 ** returns to the last executed catch context. Further throws will
 ** unstack previously executed "catches", in LIFO mode.
 **
 ** Daniel C. Sobral Jan 09/1999
 **************************************************************************/
static void ficlPrimitiveThrow(ficlVm *vm)
{
#define h_ficlPrimitiveThrow "\
Throw --  From ANS Forth standard.\n\
Throw takes the ToS and, if that's different from zero, \
returns to the last executed catch context.  \
Further throws will unstack previously executed CATCHES, in LIFO mode.\n\
Daniel C. Sobral Jan 09/1999"
  int except;
    
  if ((except = (int)ficlStackPopInteger(vm->dataStack)) != 0)
    ficlVmThrow(vm, except);
}

/**************************************************************************
 **                     a l l o c a t e
 ** MEMORY
 **************************************************************************/
static void ficlPrimitiveAllocate(ficlVm *vm)
{
  size_t size;
  void *p;

  FICL_STACK_CHECK(vm->dataStack, 1, 2);
  size = (size_t)ficlStackPopUnsigned(vm->dataStack);
  p = FTH_CALLOC(1, size);
  ficlStackPushPointer(vm->dataStack, p);
  ficlStackPushInteger(vm->dataStack, (ficlInteger)(p == NULL));
}

/**************************************************************************
 **                     f r e e 
 ** MEMORY
 **************************************************************************/
static void ficlPrimitiveFree(ficlVm *vm)
{
  void *p;

  FICL_STACK_CHECK(vm->dataStack, 1, 1);
  p = ficlStackPopPointer(vm->dataStack);
  FTH_FREE(p);
  ficlStackPushInteger(vm->dataStack, 0L);
}

/**************************************************************************
 **                     r e s i z e
 ** MEMORY
 **************************************************************************/
static void ficlPrimitiveResize(ficlVm *vm)
{
  size_t size;
  void *new, *old;

  FICL_STACK_CHECK(vm->dataStack, 2, 2);
  size = (size_t)ficlStackPopUnsigned(vm->dataStack);
  old = ficlStackPopPointer(vm->dataStack);
  new = FTH_REALLOC(old, size);

  if (new) 
  {
    ficlStackPushPointer(vm->dataStack, new);
    ficlStackPushInteger(vm->dataStack, 0L);
  } 
  else 
  {
    ficlStackPushPointer(vm->dataStack, old);
    ficlStackPushInteger(vm->dataStack, 1L);
  }
}

/**************************************************************************
 **                     e x i t - i n n e r 
 ** Signals execXT that an inner loop has completed
 **************************************************************************/
static void ficlPrimitiveExitInner(ficlVm *vm)
{
  ficlVmThrow(vm, FICL_VM_STATUS_INNER_EXIT);
}

/* === float.c === */

int ficl_float_precision = 15;

/*******************************************************************
 ** Display a float in decimal format.
 ** f. ( r -- )
 *******************************************************************/
static void ficlPrimitiveFDot(ficlVm *vm)
{
#define h_ficlPrimitiveFDot "( r -- )  Display a float in decimal format."
  FICL_STACK_CHECK(vm->dataStack, 1, 0);
  fth_printf("%.*f ", ficl_float_precision, ficlStackPopFloat(vm->dataStack));
}

/*******************************************************************
 ** Display a float in engineering format.
 ** fe. ( r -- )
 *******************************************************************/
static void ficlPrimitiveEDot(ficlVm *vm)
{
#define h_ficlPrimitiveEDot "( r -- )  Display a float in engineering format."
  FICL_STACK_CHECK(vm->dataStack, 1, 0);
  fth_printf("%#.*e ", ficl_float_precision, ficlStackPopFloat(vm->dataStack));
}

static void ficlPrimitiveSDot(ficlVm *vm)
{
#define h_ficlPrimitiveSDot "( r -- )  Display a float in scientific format."
  FICL_STACK_CHECK(vm->dataStack, 1, 0);
  fth_printf("%.*e ", ficl_float_precision, ficlStackPopFloat(vm->dataStack));
}

static void
ficlPrimitivePrecision(ficlVm *vm)
{
#define h_ficlPrimitivePrecision "( -- u )  \
Return the number of significant digits currently used by F., FE., and FS. as U."
  FICL_STACK_CHECK(vm->dataStack, 0, 1);
  ficlStackPushUnsigned(vm->dataStack, (ficlUnsigned)ficl_float_precision);
}

static void
ficlPrimitiveSetPrecision(ficlVm *vm)
{
#define h_ficlPrimitiveSetPrecision "( u -- )  \
Set the number of significant digits currently used by F., FE., and FS. to U."
  FICL_STACK_CHECK(vm->dataStack, 1, 0);
  ficl_float_precision = (int)ficlStackPopUnsigned(vm->dataStack);
}

static void
ficlPrimitiveToFloat(ficlVm *vm)
{
#define h_ficlPrimitiveToFloat "( c-addr u -- r true | false)  \
An attempt is made to convert the string specified by c-addr and u to \
internal floating-point representation.  If the string represents a valid \
floating-point number in the syntax below, its value r and true are \
returned.  If the string does not represent a valid floating-point number \
only false is returned.\n\
A string of blanks should be treated as a special case representing zero."
  char *buf, *str, *test;
  size_t len;
  
  FICL_STACK_CHECK(vm->dataStack, 2, 1);
  len = (size_t)ficlStackPopUnsigned(vm->dataStack);
  str = ficlStackPopPointer(vm->dataStack);
  if (len == 0)
  {
    ficlStackPushFloat(vm->dataStack, 0.0f);
    ficlStackPushBoolean(vm->dataStack, true);
  }
  else
  {
    buf = FTH_CALLOC(len + 1, sizeof(char));
    strncpy(buf, str, len);
    ficlFloat r = strtod(buf, &test);

    if (*test == '\0' || errno != ERANGE)
    {
      ficlStackPushFloat(vm->dataStack, r);
      ficlStackPushBoolean(vm->dataStack, true);
    }
    else
      ficlStackPushBoolean(vm->dataStack, false);
    FTH_FREE(buf);
  }
}

static void ficlPrimitiveFProximate(ficlVm *vm)
{
#define h_ficlPrimitiveFProximate "( r1 r2 r3 -- flag )  \
If R3 is positive, flag is true if the absolute value of (R1 minus R2) is less than R3.\n\
If R3 is zero, flag is true if the implementation-dependent encoding of \
R1 and R2 are exactly identical (positive and negative zero are unequal \
if they have distinct encodings).\n\
If R3 is negative, flag is true if the absolute value of (R1 minus R2) \
is less than the absolute value of R3 times the sum of the absolute \
values of R1 and R2."
  ficlFloat r1, r2, r3;
  
  FICL_STACK_CHECK(vm->dataStack, 3, 1);
  r3 = ficlStackPopFloat(vm->dataStack);
  r2 = ficlStackPopFloat(vm->dataStack);
  r1 = ficlStackPopFloat(vm->dataStack);

  if (r3 >= 0)
    ficlStackPushBoolean(vm->dataStack, fabs(r1 - r2) < r3);
  else if (r3 == 0.0)
    ficlStackPushBoolean(vm->dataStack, r1 == r2);
  else
    ficlStackPushBoolean(vm->dataStack, fabs(r1 - r2) < fabs(r3 * (fabs(r1) + fabs(r2))));
}

/**************************************************************************
 **                     F l o a t P a r s e S t a t e
 ** Enum to determine the current segement of a floating point number
 ** being parsed.
 **************************************************************************/
#define NUMISNEG 1
#define EXPISNEG 2

typedef enum _floatParseState
{
  FPS_START,
  FPS_ININT,
  FPS_INMANT,
  FPS_STARTEXP,
  FPS_INEXP
} FloatParseState;

/**************************************************************************
 **                     f i c l P a r s e F l o a t N u m b e r
 ** vm -- Virtual Machine pointer.
 ** s -- String to parse.
 ** Returns 1 if successful, 0 if not.
 **************************************************************************/
int ficlVmParseFloatNumber(ficlVm *vm, ficlString s)
{
  unsigned char c;
  unsigned char digit;
  char *trace;
  ficlUnsigned length;
  double power;
  double accum = 0.0f;
  double mant = 0.1f;
  ficlInteger exponent = 0;
  char flag = 0;
  FloatParseState estate = FPS_START;

  FICL_STACK_CHECK(vm->dataStack, 0, 1);
	
  /*
  ** floating point numbers only allowed in base 10 
  */
  if (vm->base != 10)
    return FICL_FALSE;

  trace = FICL_STRING_GET_POINTER(s);
  length = FICL_STRING_GET_LENGTH(s);

  {
    /* [ms] */
    char *test, str[FICL_PAD_SIZE + 1];
    
    strncpy(str, trace, FICL_MIN(FICL_PAD_SIZE, length));
    str[length] = '\0';
    accum = strtod(str, &test);
    
    if (*test == '\0' && errno != ERANGE)
    {
      
      ficlStackPushFloat(vm->dataStack, accum);
      
      if (vm->state == FICL_VM_STATE_COMPILE)
	ficlPrimitiveLiteralIm(vm);
      return FICL_TRUE;
    }
    errno = 0;
  }

  /* Loop through the string's characters. */
  while ((length--) && ((int)(c = (unsigned char)*trace++) != 0))
  {
    switch (estate)
    {
      /* At start of the number so look for a sign. */
    case FPS_START:
    {
      estate = FPS_ININT;
      if (c == '-')
      {
	flag |= NUMISNEG;
	break;
      }
      if (c == '+')
      {
	break;
      }
    } /* Note!  Drop through to FPS_ININT */
    /*
    **Converting integer part of number.
    ** Only allow digits, decimal and 'E'. 
    */
    /* fall through */
    case FPS_ININT:
    {
      if (c == '.')
      {
	estate = FPS_INMANT;
      }
      else if ((c == 'e') || (c == 'E'))
      {
	estate = FPS_STARTEXP;
      }
      else
      {
	digit = (unsigned char)(c - '0');
	if ((int)digit > 9)
	  return FICL_FALSE;

	accum = accum * 10 + digit;

      }
      break;
    }
    /*
    ** Processing the fraction part of number.
    ** Only allow digits and 'E' 
    */
    case FPS_INMANT:
    {
      if ((c == 'e') || (c == 'E'))
      {
	estate = FPS_STARTEXP;
      }
      else
      {
	digit = (unsigned char)(c - '0');
	if ((int)digit > 9)
	  return FICL_FALSE;

	accum += digit * mant;
	mant *= 0.1f;
      }
      break;
    }
    /* Start processing the exponent part of number. */
    /* Look for sign. */
    case FPS_STARTEXP:
    {
      estate = FPS_INEXP;

      if (c == '-')
      {
	flag |= EXPISNEG;
	break;
      }
      else if (c == '+')
      {
	break;
      }
    }       /* Note!  Drop through to FPS_INEXP */
    /*
    ** Processing the exponent part of number.
    ** Only allow digits. 
    */
    /* fall through */
    case FPS_INEXP:
    {
      digit = (unsigned char)(c - '0');
      if ((int)digit > 9)
	return FICL_FALSE;

      exponent = exponent * 10 + digit;

      break;
    }
    }
  }

  /* If parser never made it to the exponent this is not a float. */
  if (estate < FPS_STARTEXP)
    return FICL_FALSE;

  /* Set the sign of the number. */
  if (flag & NUMISNEG)
    accum = -accum;

  /* If exponent is not 0 then adjust number by it. */
  if (exponent != 0)
  {
    /* Determine if exponent is negative. */
    if (flag & EXPISNEG)
    {
      exponent = -exponent;
    }
    /* power = 10^x */
    power = (double)pow(10.0, (double)exponent);
    accum *= power;
  }

  ficlStackPushFloat(vm->dataStack, accum);
  
  if (vm->state == FICL_VM_STATE_COMPILE)
    ficlPrimitiveLiteralIm(vm);

  return FICL_TRUE;
}

/* [ms] */
static void
ficlPrimitiveMStarSlash(ficlVm *vm)
{
#define h_ficlPrimitiveMStarSlash "( d1 n1 +n2 -- d2 )  \
Multiply D1 by N1 producing the triple-cell intermediate result T.  \
Divide T by +N2 giving the double-cell quotient D2.  \
An ambiguous condition exists if +N2 is zero or negative, \
or the quotient lies outside of the range of a double-precision signed integer."
  ficl2Integer d;
  ficlInteger n1, n2;

  FICL_STACK_CHECK(vm->dataStack, 3, 1);
  n2 = ficlStackPopInteger(vm->dataStack);
  n1 = ficlStackPopInteger(vm->dataStack);
  d = ficlStackPop2Integer(vm->dataStack);
  if (n2 <= 0)
    ficlVmThrowException(vm, FICL_VM_STATUS_NUMERIC_ARG_ERROR, "n2 (%ld) <= 0", n2);
  ficlStackPush2Integer(vm->dataStack, (d * n1) / n2);
}

static void
ficlPrimitiveMPlus(ficlVm *vm)
{
#define h_ficlPrimitiveMPlus "( d1|ud1 n -- d2|ud2 )  \
Add N to D1|UD1, giving the sum D2|UD2."
  ficl2Integer d;
  ficlInteger n;

  FICL_STACK_CHECK(vm->dataStack, 2, 1);
  n = ficlStackPopInteger(vm->dataStack);
  d = ficlStackPop2Integer(vm->dataStack);
  ficlStackPush2Integer(vm->dataStack, d + n);
}

static void
ficlPrimitiveSaveInput(ficlVm *vm)
{
#define h_ficlPrimitiveSaveInput "( -- xn ... x1 n )  \
X1 through XN describe the current state of the input source \
specification for later use by RESTORE-INPUT."
  FICL_STACK_CHECK(vm->dataStack, 0, 2);
  ficlStackPushFTH(vm->dataStack, vm->callback.port_in);
  ficlStackPushInteger(vm->dataStack, 1L);
}

static void
ficlPrimitiveRestoreInput(ficlVm *vm)
{
#define h_ficlPrimitiveRestoreInput "( xn ... x1 n -- flag )  \
Attempt to restore the input source specification to the state described by X1 through XN.  \
FLAG is true if the input source specification cannot be so restored."
  FICL_STACK_CHECK(vm->dataStack, 2, 1);
  ficlStackDrop(vm->dataStack, 1);
  fth_set_io_stdin(ficlStackPopFTH(vm->dataStack));
  ficlStackPushBoolean(vm->dataStack, false);
}

static void
ficlPrimitiveUnused(ficlVm *vm)
{
#define h_ficlPrimitiveUnused "( -- u )  \
U is the amount of space remaining in the region addressed by HERE, in address units."
  ficlDictionary *dict = ficlVmGetDictionary(vm);

  FICL_STACK_CHECK(vm->dataStack, 0, 1);
  ficlStackPushUnsigned(vm->dataStack, (ficlUnsigned)ficlDictionaryCellsAvailable(dict));
}

static char    *forth_exception_name_list[] = {
	"no-error",
	"abort",
	"abort\"",
	"stack-overflow",
	"stack-underflow",
	"return-stack-overflow",
	"return-stack-underflow",
	"do-loops-nested-too-deeply",
	"dictionary-overflow",
	"invalid-memory-address",
	"division-by-zero",
	"result-out-of-range",
	"argument-type-mismatch",
	"undefined-word",
	"compile-only",
	"invalid-forget",
	"zero-length-string",
	"pno-string-overflow",
	"parsed-string-overflow",
	"name-too-long",
	"memory-write-error",
	"not-implemented",
	"control-structure-mismatch",
	"address-alignment-exception",
	"invalid-numeric-argument",
	"return-stack-imbalance",
	"missing-loop-parameters",
	"invalid-recursion",
	"user-interrupt",
	"compiler-nesting",
	"obsolet",
	">body-error",
	"invalid-name-argument",
	"block-read-exception",
	"block-write-exception",
	"invalid-block-number",
	"invalid-file-position",
	"file-I/O-exception",
	"file-not-found",
	"unexpected-eof",
	"fp-base-error",
	"loss-of-precision",
	"fp-divide-by-zero",
	"fp-result-out-of-range",
	"fp-stack-overflow",
	"fp-stack-underflow",
	"fp-invalid-argument",
	"compilation-word-list-deleted",
	"invalid-postpone",
	"search-order-overflow",
	"search-order-underflow",
	"compilation-word-list-changed",
	"cs-overflow",
	"es-overflow",
	"fp-underflow",
	"fp-unidentified-fault",
	"quit",
	"char-exception",
	"bracket-branch-error",
	NULL
};

static char    *forth_exception_list[] = {
	"no error",
	"abort",
	"abort\"",
	"stack overflow",
	"stack underflow",
	"return stack overflow",
	"return stack underflow",
	"do-loops nested too deeply during execution",
	"dictionary overflow",
	"invalid memory address",
	"division by zero",
	"result out of range",
	"argument type mismatch",
	"undefined word",
	"interpreting a compile-only word",
	"invalid FORGET",
	"attempt to use zero-length string as a name",
	"pictured numeric output string overflow",
	"parsed string overflow",
	"definition name too long",
	"write to a read-only location",
	"unsupported operation",
	"control structure mismatch",
	"address alignment exception",
	"invalid numeric argument",
	"return stack imbalance",
	"loop parameters unavailable",
	"invalid recursion",
	"user interrupt",
	"compiler nesting",
	"obsolescent feature",
	">BODY used on non-CREATEd definition",
	"invalid name argument (e.g., TO xxx)",
	"block read exception",
	"block write exception",
	"invalid block number",
	"invalid file position",
	"file I/O exception",
	"non-existent file",
	"unexpected end of file",
	"invalid BASE for floating point conversion",
	"loss of precision",
	"floating-point divide by zero",
	"floating-point result out of range",
	"floating-point stack overflow",
	"floating-point stack underflow",
	"floating-point invalid argument",
	"compilation word list deleted",
	"invalid POSTPONE",
	"search-order overflow",
	"search-order underflow",
	"compilation word list changed",
	"control-flow stack overflow",
	"exception stack overflow",
	"floating-point underflow",
	"floating-point unidentified fault",
	"quit",
	"exception in sending or receiving a character",
	"[IF], [ELSE], or [THEN] exception",
	NULL
};

static char    *ficl_exception_name_list[] = {
	"inner-exit",
	"out-of-text",
	"restart",
	"user-exit",
	"error-exit",
	"break",
	"skip-file",
	NULL
};

static char    *ficl_exception_list[] = {
	"exit inner loop",
	"hungry, normal exit",
	"word needs more text to succeed",
	"user wants to quit",
	"interpreter found an error",
	"debugger breakpoint",
	"skip loading file",
	NULL
};

static char	forth_system_error[] = "system-error";

char *
ficl_ans_exc_name(int exc)
{
	if (exc <= FICL_VM_STATUS_ABORT &&
	    exc > FICL_VM_STATUS_LAST_ERROR)
		return (forth_exception_name_list[-exc]);
	if (exc <= FICL_VM_STATUS_INNER_EXIT &&
	    exc > FICL_VM_STATUS_LAST_FICL_ERROR) {
		exc += FICL_VM_STATUS_OFFSET;
		return (ficl_exception_name_list[-exc]);
	}
	return (forth_system_error);
}

char *
ficl_ans_exc_msg(int exc)
{
	if (exc <= FICL_VM_STATUS_ABORT &&
	    exc > FICL_VM_STATUS_LAST_ERROR)
		return (forth_exception_list[-exc]);
	if (exc <= FICL_VM_STATUS_INNER_EXIT &&
	    exc > FICL_VM_STATUS_LAST_FICL_ERROR) {
		exc += FICL_VM_STATUS_OFFSET;
		return (ficl_exception_list[-exc]);
	}
	return (fth_strerror(exc));
}

static void
ficlPrimitiveDotThrow(ficlVm *vm)
{
#define h_ficlPrimitiveDotThrow "( n -- )  \
Prints description of exception N.  \
N may be an ANS exception number or a C error number."
  int exc;
  
  FICL_STACK_CHECK(vm->dataStack, 1, 0);

  if ((exc = (int)ficlStackPopInteger(vm->dataStack)) != 0)
  {
    fth_errorf("#<%s>\n", ficl_ans_exc_msg(exc));
    ficlVmThrow(vm, exc);
  }
}

#define PRIMITIVE_APPEND_CONSTANT(Dict, Name, Value)			\
	ficlDictionaryAppendConstant(Dict, Name, (ficlInteger)Value)

/**************************************************************************
 **                      f i c l C o m p i l e C o r e
 ** Builds the primitive wordset and the environment-query namespace.
 **************************************************************************/
void ficlSystemCompileCore(ficlSystem *system)
{
  ficlWord *interpret;
  ficlDictionary *dict = ficlSystemGetDictionary(system);
  ficlDictionary *env = ficlSystemGetEnvironment(system);

  FICL_ASSERT(dict != NULL);
  FICL_ASSERT(env != NULL);

#define FICL_TOKEN(token, description) 
#define FICL_INSTRUCTION_TOKEN(token, description, flags)		\
  ficlDictionaryAppendInstruction(dict, description,			\
      (ficlInstruction)token, flags);
#include "ficltokens.h"
#undef FICL_TOKEN
#undef FICL_INSTRUCTION_TOKEN
  postpone_none_im = ficlDictionaryAppendPrimitive(dict,
						   "(postpone-none-im)",
						   ficl_postpone_none_im,
						   FICL_WORD_DEFAULT);
  /*
  ** The Core word set
  ** see softcore.c for definitions of: abs bl space spaces abort"
  */
  FICL_PRIM_DOC(dict, "#",             ficlPrimitiveNumberSign);
  FICL_PRIM_DOC(dict, "#>",            ficlPrimitiveNumberSignGreater);
  FICL_PRIM_DOC(dict, "#s",            ficlPrimitiveNumberSignS);
  FICL_PRIM_DOC(dict, "\'",            ficlPrimitiveTick);
  FICL_PRIM_IM_DOC(dict, "(",          ficlPrimitiveParenthesis);
  FICL_PRIM_CO_IM(dict, "+loop",       ficlPrimitivePlusLoopCoIm);
  FICL_PRIM_DOC(dict, ".",             ficlPrimitiveDot);
  FICL_PRIM_CO_IM_DOC(dict, ".\"",     ficlPrimitiveDotQuoteCoIm);
  FICL_PRIM_DOC(dict, ":",             ficlPrimitiveColon);
  FICL_PRIM_CO_IM(dict, ";",           ficlPrimitiveSemicolonCoIm);
  FICL_PRIM_DOC(dict, "<#",            ficlPrimitiveLessNumberSign);
  FICL_PRIM_DOC(dict, ">body",         ficlPrimitiveToBody);
  FICL_PRIM(dict, ">in",               ficlPrimitiveToIn);
  FICL_PRIM_DOC(dict, ">number",       ficlPrimitiveToNumber);
  FICL_PRIM(dict, "abort",             ficlPrimitiveAbort);
  FICL_PRIM_DOC(dict, "accept",        ficlPrimitiveAccept);
  FICL_PRIM_DOC(dict, "align",         ficlPrimitiveAlign);
  FICL_PRIM(dict, "aligned",           ficlPrimitiveAligned);
  FICL_PRIM(dict, "allot",             ficlPrimitiveAllot);
  FICL_PRIM(dict, "base",              ficlPrimitiveBase);
  FICL_PRIM_CO_IM(dict, "begin",       ficlPrimitiveBeginCoIm);
  FICL_PRIM_CO_IM(dict, "case",        ficlPrimitiveCaseCoIm);
  FICL_PRIM_DOC(dict, "char",          ficlPrimitiveChar);
  FICL_PRIM_DOC(dict, "char+",         ficlPrimitiveCharPlus);
  FICL_PRIM_DOC(dict, "chars",         ficlPrimitiveChars);
  FICL_PRIM_DOC(dict, "constant",      ficlPrimitiveConstant);
  FICL_PRIM_DOC(dict, "count",         ficlPrimitiveCount);
  FICL_PRIM(dict, "cr",                ficlPrimitiveCR);
  FICL_PRIM(dict, "create",            ficlPrimitiveCreate);
  FICL_PRIM(dict, "decimal",           ficlPrimitiveDecimal);
  FICL_PRIM(dict, "depth",             ficlPrimitiveDepth);
  FICL_PRIM_CO_IM(dict, "do",          ficlPrimitiveDoCoIm);
  FICL_PRIM_CO_IM(dict, "does>",       ficlPrimitiveDoesCoIm);
  FICL_PRIM_CO_IM_DOC(dict, "else",    ficlPrimitiveElseCoIm);
  FICL_PRIM(dict, "emit",              ficlPrimitiveEmit);
  FICL_PRIM_CO_IM(dict, "endcase",     ficlPrimitiveEndcaseCoIm);
  FICL_PRIM_CO_IM(dict, "endof",       ficlPrimitiveEndofCoIm);
  FICL_PRIM_DOC(dict, "environment?",  ficlPrimitiveEnvironmentQ);
  FICL_PRIM_DOC(dict, "evaluate",      ficlPrimitiveEvaluate);
  FICL_PRIM_DOC(dict, "execute",       ficlPrimitiveExecute);
  FICL_PRIM_CO_IM_DOC(dict, "exit",    ficlPrimitiveExitCoIm);
  FICL_PRIM_CO_IM(dict, "fallthrough", ficlPrimitiveFallthroughCoIm);
  FICL_PRIM_DOC(dict, "find",          ficlPrimitiveCFind);
  FICL_PRIM_DOC(dict, "fm/mod",        ficlPrimitiveFMSlashMod);
  FICL_PRIM(dict, "here",              ficlPrimitiveHere);
  FICL_PRIM_DOC(dict, "hold",          ficlPrimitiveHold);
  FICL_PRIM_CO_IM_DOC(dict, "if",      ficlPrimitiveIfCoIm);
  FICL_PRIM_DOC(dict, "immediate",     ficlPrimitiveImmediate);
  FICL_PRIM_IM_DOC(dict, "literal",    ficlPrimitiveLiteralIm);
  FICL_PRIM_CO_IM(dict, "loop",        ficlPrimitiveLoopCoIm);
  FICL_PRIM_DOC(dict, "m*",            ficlPrimitiveMStar);
  FICL_PRIM_DOC(dict, "mod",           ficlPrimitiveMod);
  FICL_PRIM_CO_IM(dict, "of",          ficlPrimitiveOfCoIm);
  FICL_PRIM_CO_IM_DOC(dict, "postpone", ficlPrimitivePostponeCoIm);
  FICL_PRIM_DOC(dict, "quit",          ficlPrimitiveQuit);
  FICL_PRIM_CO_IM(dict, "recurse",     ficlPrimitiveRecurseCoIm);
  FICL_PRIM_CO_IM(dict, "repeat",      ficlPrimitiveRepeatCoIm);
  FICL_PRIM_IM_DOC(dict, "s\"",        ficlPrimitiveStringQuoteIm);
  FICL_PRIM_DOC(dict, "sign",          ficlPrimitiveSign);
  FICL_PRIM_DOC(dict, "sm/rem",        ficlPrimitiveSMSlashRem);
  FICL_PRIM_DOC(dict, "source",        ficlPrimitiveSource);
  FICL_PRIM_DOC(dict, "state",         ficlPrimitiveState);
  FICL_PRIM_CO_IM(dict, "then",        ficlPrimitiveEndifCoIm);
  FICL_PRIM_DOC(dict, "type",          ficlPrimitiveType);
  FICL_PRIM_DOC(dict, "u.",            ficlPrimitiveUDot);
  FICL_PRIM_DOC(dict, "um*",           ficlPrimitiveUMStar);
  FICL_PRIM_DOC(dict, "um/mod",        ficlPrimitiveUMSlashMod);
  FICL_PRIM_CO_IM(dict, "until",       ficlPrimitiveUntilCoIm);
  FICL_PRIM(dict, "variable",          ficlPrimitiveVariable);
  FICL_PRIM_CO_IM(dict, "while",       ficlPrimitiveWhileCoIm);
  FICL_PRIM_DOC(dict, "word",          ficlPrimitiveWord);
  FICL_PRIM_CO_IM(dict, "[",           ficlPrimitiveLeftBracketCoIm);
  FICL_PRIM_CO_IM(dict, "[\']",        ficlPrimitiveBracketTickCoIm);
  FICL_PRIM_IM(dict, "<\'>",           ficl_tick_stateless_im);
  FICL_PRIM_CO_IM_DOC(dict, "[char]",  ficlPrimitiveCharCoIm);
  FICL_PRIM_IM(dict, "<char>",         ficl_char_stateless_im);
  FICL_PRIM(dict, "]",                 ficlPrimitiveRightBracket);

  /* 
  ** The Core Extensions word set...
  ** see softcore.fr for other definitions
  */
  /* "#tib" */
  FICL_PRIM_IM(dict, ".(",             ficlPrimitiveDotParen);
  FICL_PRIM_DOC(dict, ":noname",       ficlPrimitiveColonNoName);
  FICL_PRIM_CO_IM(dict, "?do",         ficlPrimitiveQDoCoIm);
  FICL_PRIM_CO_IM(dict, "again",       ficlPrimitiveAgainCoIm);
  FICL_PRIM_IM(dict, "c\"",            ficlPrimitiveCountedStringQuoteIm);
  FICL_PRIM(dict, "hex",               ficlPrimitiveHex);
  FICL_PRIM_DOC(dict, "pad",           ficlPrimitivePad);
  FICL_PRIM_DOC(dict, "parse",         ficlPrimitiveParse);
  /* query restore-input save-input tib u.r u> unused [FICL_VM_STATE_COMPILE] */
  FICL_PRIM_DOC(dict, "refill",        ficlPrimitiveRefill);
  FICL_PRIM_DOC(dict, "source-id",     ficlPrimitiveSourceID);
  FICL_PRIM_IM_DOC(dict, "to",         ficlPrimitiveToValue);
  FICL_PRIM_DOC(dict, "value",         ficlPrimitiveConstant);
  FICL_PRIM_IM(dict, "\\",             ficlPrimitiveBackslash);
  /* [ms] */
  FICL_PRIM_IM(dict, "+to",            ficlPrimitivePlusToValue);
  FICL_PRIM_IM(dict, "f+to",           ficlPrimitiveFPlusToValue);
  FICL_PRIM(dict, "#!",                ficlPrimitiveBackslash);
  FICL_PRIM_DOC(dict, "m*/",           ficlPrimitiveMStarSlash);
  FICL_PRIM_DOC(dict, "m+",            ficlPrimitiveMPlus);
  FICL_PRIM_DOC(dict, "save-input",    ficlPrimitiveSaveInput);
  FICL_PRIM_DOC(dict, "restore-input", ficlPrimitiveRestoreInput);
  FICL_PRIM_DOC(dict, "unused",        ficlPrimitiveUnused);
  FICL_PRIM_DOC(dict, ".throw",        ficlPrimitiveDotThrow);

  /*
  ** Environment query values for the Core word set
  */
  PRIMITIVE_APPEND_CONSTANT(env, "core", FICL_TRUE);
  PRIMITIVE_APPEND_CONSTANT(env, "core-ext", FICL_TRUE);
  PRIMITIVE_APPEND_CONSTANT(env, "/counted-string", FICL_COUNTED_STRING_MAX);
  PRIMITIVE_APPEND_CONSTANT(env, "/hold", FICL_PAD_SIZE);
  PRIMITIVE_APPEND_CONSTANT(env, "/pad", FICL_PAD_SIZE);
  PRIMITIVE_APPEND_CONSTANT(env, "address-unit-bits", 8L);
  /* [ms]
   * PRIMITIVE_APPEND_CONSTANT(env, "floored", FICL_FALSE);
   */
  PRIMITIVE_APPEND_CONSTANT(env, "max-char", UCHAR_MAX);
  /* [ms] moved to src/numbers.c
   * PRIMITIVE_APPEND_CONSTANT(env, "max-n", 0x7fffffff);
   * PRIMITIVE_APPEND_CONSTANT(env, "max-u", 0xffffffff);
   */
  PRIMITIVE_APPEND_CONSTANT(env, "stack-cells", FICL_DEFAULT_STACK_SIZE);
  PRIMITIVE_APPEND_CONSTANT(env, "return-stack-cells", FICL_DEFAULT_RETURN_SIZE);

  /* [ms]
   * PRIMITIVE_APPEND_CONSTANT(env, "double", FICL_FALSE);
   * PRIMITIVE_APPEND_CONSTANT(env, "double-ext", FICL_FALSE);
   */

  /*
  ** The optional Exception and Exception Extensions word set
  */
  FICL_PRIM_DOC(dict, "catch",     ficlPrimitiveCatch);
  FICL_PRIM_DOC(dict, "throw",     ficlPrimitiveThrow);

  PRIMITIVE_APPEND_CONSTANT(env, "exception", FICL_TRUE);
  PRIMITIVE_APPEND_CONSTANT(env, "exception-ext", FICL_TRUE);

  /* [ms] ANS exception constants. */
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-abort", FICL_VM_STATUS_ABORT);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-abortq", FICL_VM_STATUS_ABORTQ);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-stack-overflow", FICL_VM_STATUS_STACK_OVERFLOW);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-stack-underflow", FICL_VM_STATUS_STACK_UNDERFLOW);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-rstack-overflow", FICL_VM_STATUS_RSTACK_OVERFLOW);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-rstack-underflow", FICL_VM_STATUS_RSTACK_UNDERFLOW);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-too-deep", FICL_VM_STATUS_TOO_DEEP);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-dict-overflow", FICL_VM_STATUS_DICT_OVERFLOW);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-memory-access", FICL_VM_STATUS_MEMORY_ACCESS);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-division-by-zero", FICL_VM_STATUS_DIVISION_BY_ZERO);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-range-error", FICL_VM_STATUS_RANGE_ERROR);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-argument-error", FICL_VM_STATUS_ARGUMENT_ERROR);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-undefined", FICL_VM_STATUS_UNDEFINED);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-compile-only", FICL_VM_STATUS_COMPILE_ONLY);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-invalid-forget", FICL_VM_STATUS_INVALID_FORGET);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-zero-string", FICL_VM_STATUS_ZERO_STRING);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-pno-overflow", FICL_VM_STATUS_PNO_OVERFLOW);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-parse-overflow", FICL_VM_STATUS_PARSE_OVERFLOW);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-name-too-long", FICL_VM_STATUS_NAME_TOO_LONG);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-memory-write-error", FICL_VM_STATUS_MEMORY_WRITE_ERROR);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-not-implemented", FICL_VM_STATUS_NOT_IMPLEMENTED);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-control-mismatch", FICL_VM_STATUS_CONTROL_MISMATCH);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-alignment-error", FICL_VM_STATUS_ALIGNMENT_ERROR);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-numeric-arg-error", FICL_VM_STATUS_NUMERIC_ARG_ERROR);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-rstack-imbalance", FICL_VM_STATUS_RSTACK_IMBALANCE);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-missing-lparameter", FICL_VM_STATUS_MISSING_LPARAMETER);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-recursion-error", FICL_VM_STATUS_RECURSION_ERROR);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-interrupt", FICL_VM_STATUS_INTERRUPT);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-compiler-nesting", FICL_VM_STATUS_COMPILE_ONLY);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-obsolete", FICL_VM_STATUS_OBSOLETE);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-to-body-error", FICL_VM_STATUS_TO_BODY_ERROR);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-name-arg-error", FICL_VM_STATUS_NAME_ARG_ERROR);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-bread-error", FICL_VM_STATUS_BREAD_ERROR);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-bwrite-error", FICL_VM_STATUS_BWRITE_ERROR);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-bnumber-error", FICL_VM_STATUS_BNUMBER_ERROR);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-fposition-error", FICL_VM_STATUS_FPOSITION_ERROR);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-file-io-error", FICL_VM_STATUS_FILE_IO_ERROR);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-no-such-file", FICL_VM_STATUS_NO_SUCH_FILE);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-eof-error", FICL_VM_STATUS_EOF_ERROR);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-fbase-error", FICL_VM_STATUS_FBASE_ERROR);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-precision-error", FICL_VM_STATUS_PRECISION_ERROR);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-fdivide-by-zero", FICL_VM_STATUS_FDIVIDE_BY_ZERO);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-frange-error", FICL_VM_STATUS_FRANGE_ERROR);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-fstack-overflow", FICL_VM_STATUS_FSTACK_OVERFLOW);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-fstack-underflow", FICL_VM_STATUS_FSTACK_UNDERFLOW);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-fnumber-error", FICL_VM_STATUS_FNUMBER_ERROR);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-word-list-error", FICL_VM_STATUS_WORD_LIST_ERROR);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-postpone-error", FICL_VM_STATUS_POSTPONE_ERROR);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-search-overflow", FICL_VM_STATUS_SEARCH_OVERFLOW);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-search-underflow", FICL_VM_STATUS_SEARCH_UNDERFLOW);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-word-list-changed", FICL_VM_STATUS_WORD_LIST_CHANGED);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-cs-overflow", FICL_VM_STATUS_CS_OVERFLOW);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-fp-underflow", FICL_VM_STATUS_FP_UNDERFLOW);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-fp-error", FICL_VM_STATUS_FP_ERROR);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-quit", FICL_VM_STATUS_QUIT);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-char-error", FICL_VM_STATUS_CHAR_ERROR);
  PRIMITIVE_APPEND_CONSTANT(dict, "exc-branch-error", FICL_VM_STATUS_BRANCH_ERROR);

  /*
  ** The optional Locals and Locals Extensions word set
  ** see softcore.c for implementation of locals|
  */
  FICL_PRIM_CO_IM_DOC(dict, "doLocal", ficlLocalParenIm);
  FICL_PRIM_CO_DOC(dict, "(local)",    ficlLocalParen);

  PRIMITIVE_APPEND_CONSTANT(env, "locals", FICL_TRUE);
  PRIMITIVE_APPEND_CONSTANT(env, "locals-ext", FICL_TRUE);
  PRIMITIVE_APPEND_CONSTANT(env, "#locals", FICL_MAX_LOCALS);

  /*
  ** The optional Memory-Allocation word set
  */
  FICL_PRIM(dict, "allocate",  ficlPrimitiveAllocate);
  FICL_PRIM(dict, "free",      ficlPrimitiveFree);
  FICL_PRIM(dict, "resize",    ficlPrimitiveResize);
    
  PRIMITIVE_APPEND_CONSTANT(env, "memory-alloc", FICL_TRUE);

  /*
  ** The optional Search-Order word set 
  */
  ficlSystemCompileSearch(system);

  /*
  ** The optional Programming-Tools and Programming-Tools Extensions word set
  */
  ficlSystemCompileTools(system);

  /*
  ** The optional File-Access and File-Access Extensions word set
  */
  ficlSystemCompileFile(system);

  /*
  ** Ficl extras
  */
  FICL_PRIM(dict, ".ver",               ficlPrimitiveVersion);
  FICL_PRIM_DOC(dict, ">name",          ficlPrimitiveToName);
  FICL_PRIM(dict, "add-parse-step",     ficlPrimitiveAddParseStep);
  FICL_PRIM_DOC(dict, "body>",          ficlPrimitiveFromBody);
  FICL_PRIM(dict, "compile-only",       ficlPrimitiveCompileOnly);
  FICL_PRIM_CO_IM(dict, "endif",        ficlPrimitiveEndifCoIm);
  FICL_PRIM(dict, "last-word",          ficlPrimitiveLastWord);
  FICL_PRIM(dict, "hash",               ficlPrimitiveHash);
  FICL_PRIM(dict, "objectify",          ficlPrimitiveSetObjectFlag);
  FICL_PRIM(dict, "?object",            ficlPrimitiveIsObject);
  FICL_PRIM_DOC(dict, "parse-word",     ficlPrimitiveParseNoCopy);
  FICL_PRIM_DOC(dict, "sfind",          ficlPrimitiveSFind);
  FICL_PRIM_CO_IM_DOC(dict, "sliteral", ficlPrimitiveSLiteralCoIm);
  FICL_PRIM_DOC(dict, "sprintf",        ficlPrimitiveSprintf);
  FICL_PRIM_DOC(dict, "strlen",         ficlPrimitiveStrlen);
  FICL_PRIM_DOC(dict, "b.",             ficlPrimitiveBinDot);
  FICL_PRIM_DOC(dict, "o.",             ficlPrimitiveOctDot);
  FICL_PRIM_DOC(dict, "x.",             ficlPrimitiveHexDot);
  FICL_PRIM_DOC(dict, "h.",             ficlPrimitiveHexDot);
  FICL_PRIM_DOC(dict, "user",           ficlPrimitiveUser);

  /* === float.c === */
  FICL_PRIM_DOC(dict, "f.",             ficlPrimitiveFDot);
  FICL_PRIM_DOC(dict, "fe.",            ficlPrimitiveEDot);
  FICL_PRIM_DOC(dict, "fs.",            ficlPrimitiveSDot);
  FICL_PRIM_DOC(dict, ">float",         ficlPrimitiveToFloat);
  FICL_PRIM_DOC(dict, "f~",             ficlPrimitiveFProximate);
  FICL_PRIM_DOC(dict, "precision",      ficlPrimitivePrecision);
  FICL_PRIM_DOC(dict, "set-precision",  ficlPrimitiveSetPrecision);
  FICL_PRIM_DOC(dict, "fconstant",      ficlPrimitiveConstant);
  FICL_PRIM_IM_DOC(dict, "fliteral",    ficlPrimitiveLiteralIm);
  FICL_PRIM(dict, "fvariable",          ficlPrimitiveVariable);

  /* not all required words are present */
  /* TODO: represent ( r c-addr u -- n flag1 flag2 ) */
  PRIMITIVE_APPEND_CONSTANT(env, "floating", FICL_TRUE);
  PRIMITIVE_APPEND_CONSTANT(env, "floating-ext", FICL_TRUE);
  /* [ms]
   * PRIMITIVE_APPEND_CONSTANT(env, "floating-stack", FICL_FALSE);
   */
  /*
  ** internal support words
  */
  interpret = ficlDictionaryAppendPrimitive(dict, "interpret", ficlPrimitiveInterpret,
					 FICL_WORD_DEFAULT);
  FICL_PRIM(dict, "lookup",             ficlPrimitiveLookup);
  FICL_PRIM_DOC(dict, "(parse-step)",   ficlPrimitiveParseStepParen);
  system->exitInnerWord = ficlDictionaryAppendPrimitive(dict, "exit-inner", ficlPrimitiveExitInner,
							FICL_WORD_DEFAULT);

  /*
  ** Set constants representing the internal instruction words
  ** If you want all of 'em, turn that "#if 0" to "#if 1".
  ** By default you only get the numbers (fi0, fiNeg1, etc).
  */
#define FICL_TOKEN(token, description)					\
	ficlDictionarySetConstant(dict, #token, (ficlInteger)token);
#define FICL_INSTRUCTION_TOKEN(token, description, flags) 
#include "ficltokens.h"
#undef FICL_TOKEN
#undef FICL_INSTRUCTION_TOKEN

  /*
  ** Set up system's outer interpreter loop - maybe this should be in initSystem?
  */
  system->interpreterLoop[0] = interpret;
  system->interpreterLoop[1] = (ficlWord *)ficlInstructionBranchParen;
  system->interpreterLoop[2] = (ficlWord *)(void *)(-2);

  FICL_ASSERT(ficlDictionaryCellsAvailable(dict) > 0);
}
