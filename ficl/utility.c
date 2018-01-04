/* 
 * From ficl/double.c [ms]
 */
/*******************************************************************
** m a t h 6 4 . c
** Forth Inspired Command Language - 64 bit math support routines
** Authors: Michael A. Gauland (gaulandm@mdhost.cse.tek.com)
**          Larry Hastings (larry@hastings.org)
**          John Sadler (john_sadler@alum.mit.edu)
** Created: 25 January 1998
** Rev 2.03: Support for 128 bit DP math. This file really ouught to
** be renamed!
** $Id: double.c,v 1.2 2010/09/12 15:18:07 asau Exp $
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
 * Copyright (c) 2004-2018 Michael Scholz <mi-scholz@users.sourceforge.net>
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
 * @(#)utility.c	1.26 1/4/18
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <ctype.h>
#include "ficl.h"
#include "fth.h"

/**************************************************************************
 **                      a l i g n P t r
 ** Aligns the given pointer to FICL_ALIGN address units.
 ** Returns the aligned pointer value.
 **************************************************************************/
void *ficlAlignPointer(void *ptr)
{
#if FICL_PLATFORM_ALIGNMENT > 1
  SIGNED_FTH p = (SIGNED_FTH)ptr;
  
  if (p & (FICL_PLATFORM_ALIGNMENT - 1))
    ptr = (void *)((p & ~(FICL_PLATFORM_ALIGNMENT - 1)) + FICL_PLATFORM_ALIGNMENT);
#endif
  return ptr;
}

/**************************************************************************
 **                      s t r r e v
 ** 
 **************************************************************************/
char *ficlStringReverse( char *string )    
{                               /* reverse a string in-place */
  int i = (int)strlen(string);
  char *p1 = string;          /* first char of string */
  char *p2 = string + i - 1;  /* last non-NULL char of string */
  char c;

  if (i > 1)
    while (p1 < p2)
    {
      c = *p2;
      *p2 = *p1;
      *p1 = c;
      p1++; p2--;
    }

  return string;
}

/* From double.c. */
ficl2UnsignedQR ficl2UnsignedDivide(ficl2Unsigned q, ficlUnsigned y)
{
  ficl2UnsignedQR result;

  result.quotient = q / y;
  /*
  ** Once we have the quotient, it's cheaper to calculate the
  ** remainder this way than with % (mod).  --lch
  */
  result.remainder = (ficlUnsigned)(q - (result.quotient * y));
  return result;
}

/**************************************************************************
 **                      ficl2IntegerDivideSymmetric
 ** Divide an ficl2Unsigned by a ficlInteger and return a ficlInteger quotient and a
 ** ficlInteger remainder. The absolute values of quotient and remainder are not
 ** affected by the signs of the numerator and denominator (the operation
 ** is symmetric on the number line)
 **************************************************************************/
ficl2IntegerQR ficl2IntegerDivideSymmetric(ficl2Integer num, ficlInteger den)
{
  ficl2IntegerQR qr;
  ficl2UnsignedQR uqr;
  int signRem = 1;
  int signQuot = 1;

  if (num < 0)
  {
    num = -num;
    signRem  = -signRem;
    signQuot = -signQuot;
  }

  if (den < 0)
  {
    den      = -den;
    signQuot = -signQuot;
  }

  uqr = ficl2UnsignedDivide((ficl2Unsigned)num, (ficlUnsigned)den);
  qr.quotient = (ficl2Integer)uqr.quotient;
  qr.remainder = (ficlInteger)uqr.remainder;

  if (signRem < 0)
    qr.remainder = -qr.remainder;

  if (signQuot < 0)
    qr.quotient = -qr.quotient;

  return qr;
}

/**************************************************************************
 **                      d i g i t _ t o _ c h a r
 ** 
 **************************************************************************/
static char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";

char ficlDigitToCharacter(int value)
{
  return digits[value];
}

/**************************************************************************
 **                      i s P o w e r O f T w o
 ** Tests whether supplied argument is an integer power of 2 (2**n)
 ** where 32 > n > 1, and returns n if so. Otherwise returns zero.
 **************************************************************************/
int ficlIsPowerOfTwo(ficlUnsigned u)
{
  int i = 1;
  ficlUnsigned t = 2;

  for (; ((t <= u) && (t != 0)); i++, t <<= 1)
    if (u == t)
      return i;

  return 0;
}

/**************************************************************************
 **                      l t o a
 ** 
 **************************************************************************/
char *ficlLtoa( ficlInteger value, char *string, int radix )
{                               /* convert long to string, any base */
  char *cp = string;
  int sign = ((radix == 10) && (value < 0));
  int pwr;

  FICL_ASSERT(radix > 1);
  FICL_ASSERT(radix < 37);
  FICL_ASSERT(string != NULL);

  pwr = ficlIsPowerOfTwo((ficlUnsigned)radix);

  if (sign)
    value = -value;

  if (value == 0)
    *cp++ = '0';
  else if (pwr != 0)
  {
    ficlUnsigned v = (ficlUnsigned) value;
    ficlUnsigned mask = (ficlUnsigned) ~(((ficlUnsigned)(-1)) << pwr);

    while (v)
    {
      *cp++ = digits[v & mask];
      v >>= pwr;
    }
  }
  else
  {
    ficl2UnsignedQR result;
    ficl2Unsigned v = (ficl2Unsigned)value;

    while (v != 0)
    {
      result = ficl2UnsignedDivide(v, (ficlUnsigned)radix);
      *cp++ = digits[result.remainder];
      v = result.quotient;
    }
  }

  if (sign)
    *cp++ = '-';

  *cp++ = '\0';
  return ficlStringReverse(string);
}

/**************************************************************************
 **                      u l t o a
 ** 
 **************************************************************************/
char *ficlUltoa(ficlUnsigned value, char *string, int radix )
{                               /* convert long to string, any base */
  char *cp = string;
  ficl2Unsigned ud;
  ficl2UnsignedQR result;

  FICL_ASSERT(radix > 1);
  FICL_ASSERT(radix < 37);
  FICL_ASSERT(string != NULL);

  if (value == 0)
    *cp++ = '0';
  else
  {
    ud = (ficl2Unsigned)value;

    while (ud != 0)
    {
      result = ficl2UnsignedDivide(ud, (ficlUnsigned)radix);
      ud = result.quotient;
      *cp++ = digits[result.remainder];
    }
  }

  *cp++ = '\0';
  return ficlStringReverse(string);
}

/**************************************************************************
 **                      c a s e F o l d
 ** Case folds a NULL terminated string in place. All characters
 ** get converted to lower case.
 **************************************************************************/
char *ficlStringCaseFold(char *cp)
{
  char *oldCp = cp;

  while (*cp)
  {
    if (isupper((int)*cp))
      *cp = (char)tolower((int)*cp);
      
    cp++;
  }
  return oldCp;
}

/**************************************************************************
 **                      s t r i n c m p
 ** (jws) simplified the code a bit in hopes of appeasing Purify
 **************************************************************************/
int intern_ficlStrincmp(char *cp1, char *cp2, ficlUnsigned count)
{
  int i = 0;

  for (; 0 < count; ++cp1, ++cp2, --count)
  {
    i = tolower((int)*cp1) - tolower((int)*cp2);
      
    if (i != 0)
      return i;
    else if (*cp1 == '\0')
      return 0;
  }
  return 0;
}

/**************************************************************************
 **                      s k i p S p a c e
 ** Given a string pointer, returns a pointer to the first non-space
 ** char of the string, or to the NULL terminator if no such char found.
 ** If the pointer reaches "end" first, stop there. Pass NULL to 
 ** suppress this behavior.
 **************************************************************************/
char *ficlStringSkipSpace(char *cp, char *end)
{
  if (cp)
    while ((cp != end) && isspace((int)*cp))
      cp++;
  return cp;
}

/* [ms] */
char *ficl_running_word(ficlVm *vm)
{
  ficlWord *word = ficlVmGetRunningWord(vm);
  
  return ((word && word->length > 0) ? word->name : NULL);
}
