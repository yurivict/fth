/*
**	ficllocal.h
**
** Put all local settings here.  This file will always ship empty.
**
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
 * Copyright (c) 2004-2017 Michael Scholz <mi-scholz@users.sourceforge.net>
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
 * @(#)ficllocal.h	1.70 12/31/17
 */

#if !defined(_FICLLOCAL_H_)
#define _FICLLOCAL_H_

#include "fth-config.h"

typedef char	ficlInteger8;
typedef unsigned char ficlUnsigned8;
typedef short	ficlInteger16;
typedef unsigned short ficlUnsigned16;
typedef int	ficlInteger32;
typedef unsigned int ficlUnsigned32;
#if (FTH_SIZEOF_LONG == 4)
typedef long long ficlInteger64;
typedef unsigned long long ficlUnsigned64;
#else
typedef long	ficlInteger64;
typedef unsigned long ficlUnsigned64;
#endif

/* It's not a pointer but the base for type FTH. */
#if (FTH_SIZEOF_LONG == FTH_SIZEOF_VOID_P)
typedef unsigned long ficlPointer;
typedef long	ficlSignedPointer;
#elif (FTH_SIZEOF_LONG_LONG == FTH_SIZEOF_VOID_P)
typedef unsigned long long ficlPointer;
typedef long long ficlSignedPointer;
#else
#error *** FTH requires sizeof(void*) == (sizeof(long) || sizeof(long long))
#endif

typedef long ficlInteger;
typedef unsigned long ficlUnsigned;
typedef ficlInteger64 ficl2Integer;
typedef ficlUnsigned64 ficl2Unsigned;
typedef double ficlFloat;
typedef ficlPointer FTH;
typedef ficlSignedPointer SIGNED_FTH;

#if defined(lint)
/* numbers.c */
#undef HAVE_COMPLEX_H
#undef HAVE_COMPLEX_I
#undef HAVE_1_0_FI
#endif

#if defined(HAVE_COMPLEX_H)
#include <complex.h>
#endif

#if defined(HAVE_1_0_FI)
#define HAVE_COMPLEX	1
/*
 * Minix doesn't have complex.h.
 * With gcc Minix provides 1.0fi, cimag(), and creal().
 */
#if !defined(HAVE_COMPLEX_I)
/* snippet from /usr/include/complex.h */
#if defined(__GNUC__)
#if __STDC_VERSION__ < 199901
#define	_Complex	__complex__
#endif
#define	_Complex_I	1.0fi
#endif /* __GNUC__ */

#define	complex		_Complex
#define	I		_Complex_I

double		cimag(double complex);
double		creal(double complex);
/* end /usr/include/complex.h */
#endif /* !HAVE_COMPLEX_I */
#else				/* !HAVE_1_0_FI */
#define HAVE_COMPLEX	0
#endif				/* HAVE_1_0_FI */

#if HAVE_COMPLEX
typedef complex double ficlComplex;
#endif

#if defined(HAVE_OPENSSL_BN_H)
#include <openssl/bn.h>
#define HAVE_BN		1
/* bn(3) */
typedef BIGNUM * ficlBignum;

typedef struct {
	ficlBignum num;
	ficlBignum den;
} FRatio;
typedef FRatio * ficlRatio;
#else
#define HAVE_BN		0
#endif

#if !defined(true)
#define false 		0
#define true 		1
#endif

#define FICL_FORTH_NAME			FTH_PACKAGE_TARNAME
#define FICL_FORTH_VERSION		FTH_PACKAGE_VERSION
#define FICL_PLATFORM_BASIC_TYPES	1
#define FICL_DEFAULT_DICTIONARY_SIZE	(1024 * 1024)
#define FICL_DEFAULT_STACK_SIZE		(1024 * 8)
#define FICL_DEFAULT_RETURN_SIZE	1024
#define FICL_DEFAULT_ENVIRONMENT_SIZE	(1024 * 8)
#define FICL_MIN_DICTIONARY_SIZE	(1024 * 512)
#define FICL_MIN_STACK_SIZE		512
#define FICL_MIN_RETURN_SIZE		512
#define FICL_MIN_ENVIRONMENT_SIZE	(1024 * 4)
#define FICL_MAX_LOCALS			2048
#define FICL_MAX_WORDLISTS		32
#define FICL_MAX_PARSE_STEPS		16
#define FICL_PAD_SIZE			1024
#define FICL_NAME_LENGTH		256
#define FICL_HASH_SIZE			241
#define FICL_USER_CELLS			1024
#define FICL_PLATFORM_ALIGNMENT		FTH_ALIGNOF_VOID_P
#define FICL_PLATFORM_EXTERN		/* empty */

#define FICL_PRIMITIVE_SET(Dict, Name, Code, Type)			\
	ficlDictionaryAppendPrimitive(Dict, Name, Code, (ficlUnsigned)(Type))

#define FICL_PRIM(Dict, Name, Code)					\
	FICL_PRIMITIVE_SET(Dict, Name, Code, FICL_WORD_DEFAULT)

#define FICL_PRIM_IM(Dict, Name, Code)					\
	FICL_PRIMITIVE_SET(Dict, Name, Code, FICL_WORD_IMMEDIATE)

#define FICL_PRIM_CO(Dict, Name, Code)					\
	FICL_PRIMITIVE_SET(Dict, Name, Code, FICL_WORD_COMPILE_ONLY)

#define FICL_PRIM_CO_IM(Dict, Name, Code)				\
	FICL_PRIMITIVE_SET(Dict, Name, Code, FICL_WORD_COMPILE_ONLY_IMMEDIATE)

#define FICL_PRIMITIVE_DOC_SET(Dict, Name, Code, Type, Docs) do {	\
	ficlWord *word;							\
									\
	word = ficlDictionaryAppendPrimitive(Dict, Name, Code,		\
	    (ficlUnsigned)(Type));					\
									\
	fth_word_doc_set(word, Docs);					\
} while (0)

#define FICL_PRIM_DOC(Dict, Name, Code)					\
	FICL_PRIMITIVE_DOC_SET(Dict, Name, Code,			\
	    FICL_WORD_DEFAULT, h_ ## Code)
#define FICL_PRIM_IM_DOC(Dict, Name, Code)				\
	FICL_PRIMITIVE_DOC_SET(Dict, Name, Code,			\
	    FICL_WORD_IMMEDIATE, h_ ## Code)
#define FICL_PRIM_CO_DOC(Dict, Name, Code)				\
	FICL_PRIMITIVE_DOC_SET(Dict, Name, Code,			\
	    FICL_WORD_COMPILE_ONLY, h_ ## Code)
#define FICL_PRIM_CO_IM_DOC(Dict, Name, Code)				\
	FICL_PRIMITIVE_DOC_SET(Dict, Name, Code,			\
	    FICL_WORD_COMPILE_ONLY_IMMEDIATE, h_ ## Code)

#endif				/* _FICLLOCAL_H_ */

/* ficllocal.h ends here */
