/*******************************************************************
** f i c l . h
** Forth Inspired Command Language
** Author: John Sadler (john_sadler@alum.mit.edu)
** Created: 19 July 1997
** Dedicated to RHS, in loving memory
** $Id: ficl.h,v 1.25 2010/10/03 09:52:12 asau Exp $
********************************************************************
**
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
 * Copyright (c) 2004-2014 Michael Scholz <mi-scholz@users.sourceforge.net>
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
 * @(#)ficl.h	1.95 1/23/14
 */

#if !defined (__FICL_H__)
#define __FICL_H__
/*
** Ficl (Forth-inspired command language) is an ANS Forth
** interpreter written in C. Unlike traditional Forths, this
** interpreter is designed to be embedded into other systems
** as a command/macro/development prototype language. 
**
** Where Forths usually view themselves as the center of the system
** and expect the rest of the system to be coded in Forth, Ficl
** acts as a component of the system. It is easy to export 
** code written in C or ASM to Ficl in the style of TCL, or to invoke
** Ficl code from a compiled module. This allows you to do incremental
** development in a way that combines the best features of threaded 
** languages (rapid development, quick code/test/debug cycle,
** reasonably fast) with the best features of C (everyone knows it,
** easier to support large blocks of code, efficient, type checking).
**
** Ficl provides facilities for interoperating
** with programs written in C: C functions can be exported to Ficl,
** and Ficl commands can be executed via a C calling interface. The
** interpreter is re-entrant, so it can be used in multiple instances
** in a multitasking system. Unlike Forth, Ficl's outer interpreter
** expects a text block as input, and returns to the caller after each
** text block, so the "data pump" is somewhere in external code. This
** is more like TCL than Forth, which usually expects to be at the center
** of the system, requesting input at its convenience. Each Ficl virtual 
** machine can be bound to a different I/O channel, and is independent
** of all others in in the same address space except that all virtual
** machines share a common dictionary (a sort or open symbol table that
** defines all of the elements of the language).
**
** Code is written in ANSI C for portability. 
**
** Summary of Ficl features and constraints:
** - Standard: Implements the ANSI Forth CORE word set and part 
**   of the CORE EXT word-set, SEARCH and SEARCH EXT, TOOLS and
**   TOOLS EXT, LOCAL and LOCAL ext and various extras.
** - Extensible: you can export code written in Forth, C, 
**   or asm in a straightforward way. Ficl provides open
**   facilities for extending the language in an application
**   specific way. You can even add new control structures!
** - Ficl and C can interact in two ways: Ficl can encapsulate
**   C code, or C code can invoke Ficl code.
** - Thread-safe, re-entrant: The shared system dictionary 
**   uses a locking mechanism that you can either supply
**   or stub out to provide exclusive access. Each Ficl
**   virtual machine has an otherwise complete state, and
**   each can be bound to a separate I/O channel (or none at all).
** - Simple encapsulation into existing systems: a basic implementation
**   requires three function calls (see the example program in testmain.c).
** - ROMable: Ficl is designed to work in RAM-based and ROM code / RAM data
**   environments. It does require somewhat more memory than a pure
**   ROM implementation because it builds its system dictionary in 
**   RAM at startup time.
** - Written an ANSI C to be as simple as I can make it to understand,
**   support, debug, and port. Compiles without complaint at /Az /W4 
**   (require ANSI C, max warnings) under Microsoft VC++ 5.
** - Does full 32 bit math (but you need to implement
**   two mixed precision math primitives (see sysdep.c))
** - Indirect threaded interpreter is not the fastest kind of
**   Forth there is (see pForth 68K for a really fast subroutine
**   threaded interpreter), but it's the cleanest match to a
**   pure C implementation.
**
** P O R T I N G   F i c l
**
** To install Ficl on your target system, you need an ANSI C compiler
** and its runtime library. Inspect the system dependent macros and
** functions in sysdep.h and sysdep.c and edit them to suit your
** system. For example, INT16 is a short on some compilers and an
** int on others. Check the default CELL alignment controlled by
** FICL_ALIGN. If necessary, add new definitions of ficlMalloc, ficlFree,
** ficlLockDictionary, and ficlCallbackDefaultTextOut to work with your
** operating system.  Finally, use testmain.c as a guide to installing the
** Ficl system and one or more virtual machines into your code. You do not
** need to include testmain.c in your build.
**
** T o   D o   L i s t
**
** 1. Unimplemented system dependent CORE word: key
** 2. Ficl uses the PAD in some CORE words - this violates the standard,
**    but it's cleaner for a multithreaded system. I'll have to make a
**    second pad for reference by the word PAD to fix this.
**
** F o r   M o r e   I n f o r m a t i o n
**
** Web home of Ficl
**   http://ficl.sourceforge.net
** Check this website for Forth literature (including the ANSI standard)
**   http://www.taygeta.com/forthlit.html
** and here for software and more links
**   http://www.taygeta.com/forth.html
*/

#undef __BEGIN_DECLS
#undef __END_DECLS
#if defined(__cplusplus)
#define __BEGIN_DECLS		extern "C" {
#define __END_DECLS		}
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <limits.h>
#include <setjmp.h>

/*
** Put all your local defines in ficllocal.h,
** rather than editing the makefile/project/etc.
** ficllocal.h will always ship as an inert file.
*/
#include "ficllocal.h"

/*
** P L A T F O R M   S E T T I N G S
**
** The FICL_PLATFORM_* settings.
** These indicate attributes about the local platform.
*/

#define FICL_NAME		"ficl"
#define FICL_VERSION		"4.0.31"

/*
** 
** Forth name.
*/
#if !defined (FICL_FORTH_NAME)
#define FICL_FORTH_NAME		FICL_NAME
#endif

/*
** 
** Forth version.
*/
#if !defined (FICL_FORTH_VERSION)
#define FICL_FORTH_VERSION	FICL_VERSION
#endif

/*
** FICL_PLATFORM_ARCHITECTURE
** String constant describing the current hardware architecture.
*/
#if !defined (FICL_PLATFORM_ARCHITECTURE)
#define FICL_PLATFORM_ARCHITECTURE FTH_TARGET_CPU
#endif

/*
** FICL_PLATFORM_OS
** String constant describing the current operating system.
*/
#if !defined (FICL_PLATFORM_OS)
#define FICL_PLATFORM_OS	FTH_TARGET_OS
#endif

/*
** FICL_PLATFORM_VENDOR
** String constant describing the current vendor.
*/
#if !defined (FICL_PLATFORM_VENDOR)
#define FICL_PLATFORM_VENDOR	FTH_TARGET_VENDOR
#endif

/*
** FICL_EXTERN
** Must be defined, should be a keyword used to declare
** a function prototype as being a genuine prototype.
** You should only have to fiddle with this setting if
** you're not using an ANSI-compliant compiler, in which
** case, good luck!
**
** [ms] FICL_EXTERN removed
*/
#if !defined (FICL_EXTERN)
#define FICL_EXTERN extern
#endif /* !defined FICL_EXTERN */

/*
** FICL_PLATFORM_BASIC_TYPES
**
** If not defined yet, 
*/
#if !defined (FICL_PLATFORM_BASIC_TYPES)
typedef char ficlInteger8;
typedef unsigned char ficlUnsigned8;
typedef short ficlInteger16;
typedef unsigned short ficlUnsigned16;
typedef long ficlInteger32;
typedef unsigned long ficlUnsigned32;

typedef ficlInteger32 ficlInteger;
typedef ficlUnsigned32 ficlUnsigned;
typedef float ficlFloat;
#endif /* !defined(FICL_PLATFORM_BASIC_TYPES) */

/*
** FICL_ROBUST enables bounds checking of stacks and the dictionary.
** This will detect stack over and underflows and dictionary overflows.
** Any exceptional condition will result in an assertion failure.
** (As generated by the ANSI assert macro)
** FICL_ROBUST == 1 --> stack checking in the outer interpreter
** FICL_ROBUST == 2 also enables checking in many primitives
*/
/* FICL_ROBUST removed [ms] */

/*
** FICL_DEFAULT_STACK_SIZE Specifies the default size (in CELLs) of
** a new virtual machine's stacks, unless overridden at 
** create time.
*/
#if !defined (FICL_DEFAULT_STACK_SIZE)
#define FICL_DEFAULT_STACK_SIZE	(128)
#endif

/*
** FICL_DEFAULT_DICTIONARY_SIZE specifies the number of ficlCells to allocate
** for the system dictionary by default. The value
** can be overridden at startup time as well.
*/
#if !defined (FICL_DEFAULT_DICTIONARY_SIZE)
#define FICL_DEFAULT_DICTIONARY_SIZE (12288)
#endif

/*
** FICL_DEFAULT_ENVIRONMENT_SIZE specifies the number of cells
** to allot for the environment-query dictionary.
*/
#if !defined (FICL_DEFAULT_ENVIRONMENT_SIZE)
#define FICL_DEFAULT_ENVIRONMENT_SIZE (512)
#endif

/*
** FICL_MAX_WORDLISTS specifies the maximum number of wordlists in 
** the dictionary search order. See Forth DPANS sec 16.3.3
** (file://dpans16.htm#16.3.3)
*/
#if !defined (FICL_MAX_WORDLISTS)
#define FICL_MAX_WORDLISTS	(16)
#endif

/*
** FICL_MAX_PARSE_STEPS controls the size of an array in the FICL_SYSTEM structure
** that stores pointers to parser extension functions. I would never expect to have
** more than 8 of these, so that's the default limit. Too many of these functions
** will probably exact a nasty performance penalty.
*/
#if !defined (FICL_MAX_PARSE_STEPS)
#define FICL_MAX_PARSE_STEPS	(8)
#endif

/*
** Maximum number of local variables per definition.
** This only affects the size of the locals dictionary,
** and there's only one per entire ficlSystem, so it
** doesn't make sense to be a piker here.
*/
#if !defined (FICL_MAX_LOCALS)
#define FICL_MAX_LOCALS		(64)
#endif

/*
** The pad is a small scratch area for text manipulation. ANS Forth
** requires it to hold at least 84 characters.
*/
#if !defined (FICL_PAD_SIZE)
#define FICL_PAD_SIZE		(256)
#endif

/* 
** ANS Forth requires that a word's name contain {1..31} characters.
*/
#if !defined (FICL_NAME_LENGTH)
#define FICL_NAME_LENGTH	(31)
#endif

/*
** Default size of hash table. For most uniform
** performance, use a prime number!
*/
#if !defined (FICL_HASH_SIZE)
#define FICL_HASH_SIZE		(241)
#endif

/*
** Default number of USER flags.
*/
#if !defined (FICL_USER_CELLS)
#define FICL_USER_CELLS		(16)
#endif

/*
** Forward declarations... read on.
*/
struct ficlWord;
typedef struct ficlWord ficlWord;
struct ficlVm;
typedef struct ficlVm ficlVm;
struct ficlDictionary;
typedef struct ficlDictionary ficlDictionary;
struct ficlSystem;
typedef struct ficlSystem ficlSystem;
struct ficlSystemInformation;
typedef struct ficlSystemInformation ficlSystemInformation;
struct ficlCallback;
typedef struct ficlCallback ficlCallback;
struct ficlCountedString;
typedef struct ficlCountedString ficlCountedString;
struct ficlString;
typedef struct ficlString ficlString;


__BEGIN_DECLS

/*
** System dependent routines:
** Edit the implementations in your appropriate ficlplatform/xxx.c to be
** compatible with your runtime environment.
**
** ficlCallbackDefaultTextOut sends a zero-terminated string to the 
**   default output device - used for system error messages.
**
** ficlMalloc(), ficlRealloc() and ficlFree() have the same semantics
**   as the functions malloc(), realloc(), and free()
**   from the standard C library.
*/
char           *ficlCallbackDefaultTextIn(ficlCallback *);
void		ficlCallbackDefaultTextOut(ficlCallback *, char *);
void		ficlCallbackDefaultErrorOut(ficlCallback *, char *);

/* 
** the Good Stuff starts here...
*/

/*
** ANS Forth requires false to be zero, and true to be the ones
** complement of false... that unifies logical and bitwise operations
** nicely.
*/
#define FICL_TRUE		(-1)
#define FICL_FALSE		(0)
#define FICL_BOOL(x)		((x) ? FICL_TRUE : FICL_FALSE)

#if !defined (NULL)
#define NULL			((void *)0)
#endif

/*
** These structures represent the result of division.
*/
typedef struct {
	ficl2Unsigned	quotient;
	ficlUnsigned	remainder;
} ficl2UnsignedQR;

typedef struct {
	ficl2Integer	quotient;
	ficlInteger	remainder;
} ficl2IntegerQR;

/*
** 64 bit integer math support routines: multiply two UNS32s
** to get a 64 bit product, & divide the product by an UNS32
** to get an UNS32 quotient and remainder. Much easier in asm
** on a 32 bit CPU than in C, which usually doesn't support 
** the double length result (but it should).
*/
ficl2IntegerQR	ficl2IntegerDivideSymmetric(ficl2Integer, ficlInteger);
ficl2UnsignedQR	ficl2UnsignedDivide(ficl2Unsigned, ficlUnsigned);

/*
** A ficlCell is the main storage type. It must be large enough
** to contain a pointer or a scalar. In order to accommodate 
** 32 bit and 64 bit processors, use abstract types for int, 
** unsigned, and float.
**
** A ficlUnsigned, ficlInteger, and ficlFloat *MUST* be the same
** size as a "void *" on the target system.  (Sorry, but that's
** a design constraint of FORTH.)
*/

typedef union {
	ficlInteger	i;
	ficlUnsigned	u;
	FTH		fp;
	void           *p;
	void            (*fn) (void);
} ficlCell;

#define CELL_REF(Obj)		((ficlCell *)(Obj))
#define CELL_FICL_TO_FTH(Obj)	ficl_to_fth(CELL_FTH_REF(Obj))

#define CELL_INT_REF(Obj)	CELL_REF(Obj)->i
#define CELL_UINT_REF(Obj)	CELL_REF(Obj)->u
#define CELL_FTH_REF(Obj)	CELL_REF(Obj)->fp
#define CELL_VOIDP_REF(Obj)	CELL_REF(Obj)->p
#define CELL_FN_REF(Obj)	CELL_REF(Obj)->fn
#define CELL_LONG_REF(Obj)	fth_long_long_ref(CELL_FICL_TO_FTH(Obj))
#define CELL_ULONG_REF(Obj)	fth_ulong_long_ref(CELL_FICL_TO_FTH(Obj))
#define CELL_FLOAT_REF(Obj)	fth_float_ref(CELL_FICL_TO_FTH(Obj))
#define CELL_BOOL_REF(Obj)	(FTH_TO_BOOL((CELL_FTH_REF(Obj))))

#define CELL_INT_SET(Obj, Val)						\
	CELL_INT_REF(Obj) = (ficlInteger)(Val)
#define CELL_UINT_SET(Obj, Val)						\
	CELL_UINT_REF(Obj) = (ficlUnsigned)(Val)
#define CELL_FTH_SET(Obj, Val)						\
	CELL_FTH_REF(Obj) = (FTH)(Val)
#define CELL_VOIDP_SET(Obj, Val)					\
	CELL_VOIDP_REF(Obj) = (void *)(Val)
#define CELL_FN_SET(Obj, Val)						\
	CELL_FN_REF(Obj) = (void (*fn)(void))(Val)
#define CELL_LONG_SET(Obj, Val)						\
	CELL_FTH_REF(Obj) = fth_make_llong((ficl2Integer)(Val))
#define CELL_ULONG_SET(Obj, Val)					\
	CELL_FTH_REF(Obj) = fth_make_ullong((ficl2Unsigned)(Val))
#define CELL_FLOAT_SET(Obj, Val)					\
	CELL_FTH_REF(Obj) = fth_make_float((ficlFloat)(Val))
#define CELL_BOOL_SET(Obj, Val)						\
	CELL_FTH_REF(Obj) = BOOL_TO_FTH(Val)

#define FICL_BITS_PER_CELL	(sizeof(ficlCell) * 8)

/*
** FICL_PLATFORM_ALIGNMENT is the number of bytes to which
** the dictionary pointer address must be aligned. This value
** is usually either 2 or 4, depending on the memory architecture
** of the target system; 4 is safe on any 16 or 32 bit
** machine.  8 would be appropriate for a 64 bit machine.
*/
#if !defined (FICL_PLATFORM_ALIGNMENT)
#define FICL_PLATFORM_ALIGNMENT	FTH_ALIGNOF_VOID_P
#endif

/*
** FICL_LVALUE_TO_CELL does a little pointer trickery to cast any CELL sized
** lvalue (informal definition: an expression whose result has an
** address) to CELL. Remember that constants and casts are NOT
** themselves lvalues!
*/
#define FICL_LVALUE_TO_CELL(v)	(*(ficlCell *)&(v))

/*
** PTRtoCELL is a cast through void * intended to satisfy the
** most outrageously pedantic compiler... (I won't mention 
** its name)
*/
#define FICL_POINTER_TO_CELL(p)	((ficlCell *)(void *)p)

/*
** FORTH defines the "counted string" data type.  This is
** a "Pascal-style" string, where the first byte is an unsigned
** count of characters, followed by the characters themselves.
** The Ficl structure for this is ficlCountedString.
** Ficl also often zero-terminates them so that they work with the
** usual C runtime library string functions... strlen(), strcmp(),
** and the like.  (Belt & suspenders?  You decide.)
**
** The problem is, this limits strings to 255 characters, which
** can be a bit constricting to us wordy types.  So FORTH only
** uses counted strings for backwards compatibility, and all new
** words are "c-addr u" style, where the address and length are
** stored separately, and the length is a full unsigned "cell" size.
** (For more on this trend, see DPANS94 section A.3.1.3.4.)
** Ficl represents this with the ficlString structure.  Note that
** these are frequently *not* zero-terminated!  Don't depend on
** it--that way lies madness.
*/

/*
 * XXX: char text[FICL_COUNTED_STRING_MAX + 1];
 */
#define FICL_COUNTED_STRING_MAX  (FICL_PAD_SIZE)

struct ficlCountedString {
	ficlUnsigned	length;
	char		text[FICL_COUNTED_STRING_MAX + 1];
};

#define FICL_COUNTED_STRING_GET_LENGTH(cs)  ((cs).length)
#define FICL_COUNTED_STRING_GET_POINTER(cs) ((cs).text)

#define FICL_POINTER_TO_COUNTED_STRING(p)   ((ficlCountedString *)(void *)(p))

struct ficlString {
	ficlUnsigned	length;
	char           *text;
};

#define FICL_STRING_GET_LENGTH(fs)	((fs).length)
#define FICL_STRING_GET_POINTER(fs)	((fs).text)
#define FICL_STRING_SET_LENGTH(fs, l)	((fs).length = (ficlUnsigned)(l))
#define FICL_STRING_SET_POINTER(fs, p)	((fs).text = (char *)(p))
#define FICL_STRING_SET_FROM_COUNTED_STRING(string, countedstring)	\
	{								\
		(string).text = (countedstring).text;			\
		(string).length = (countedstring).length;		\
	}
/* 
** Init a FICL_STRING from a pointer to a zero-terminated string
*/
#define FICL_STRING_SET_FROM_CSTRING(string, cstring)			\
	{								\
		(string).text = ((char *)(cstring));			\
		(string).length = (ficlUnsigned)fth_strlen(cstring);	\
	}

/*
** Ficl uses this little structure to hold the address of 
** the block of text it's working on and an index to the next
** unconsumed character in the string. Traditionally, this is
** done by a Text Input Buffer, so I've called this struct TIB.
**
** Since this structure also holds the size of the input buffer,
** and since evaluate requires that, let's put the size here.
** The size is stored as an end-pointer because that is what the
** null-terminated string aware functions find most easy to deal
** with.
** Notice, though, that nobody really uses this except evaluate,
** so it might just be moved to ficlVm instead. (sobral)
*/
typedef struct {
	ficlInteger	index;
	char           *end;
	char           *text;
} ficlTIB;

/*
** Stacks get heavy use in Ficl and Forth...
** Each virtual machine implements two of them:
** one holds parameters (data), and the other holds return
** addresses and control flow information for the virtual
** machine. (Note: C's automatic stack is implicitly used,
** but not modeled because it doesn't need to be...)
** Here's an abstract type for a stack
*/

/* [ms] */

typedef struct {
	ficlUnsigned	size;	/* size of the stack, in cells */
	ficlCell       *frame;	/* link reg for stack frame */
	ficlCell       *top;	/* stack pointer */
	ficlVm         *vm;	/* used for debugging */
	char           *name;	/* used for debugging */
	ficlCell	base[1];/* Top of stack */
} ficlStack;

#define STACK_REF(Obj)		((ficlStack *)(Obj))
#define STACK_TOP_REF(Obj)	STACK_REF(Obj)->top
#define STACK_FRAME_REF(Obj)	STACK_REF(Obj)->frame
#define STACK_BASE_REF(Obj)	STACK_REF(Obj)->base[1]

#define STACK_INT_REF(Obj)	CELL_INT_REF(STACK_TOP_REF(Obj))
#define STACK_UINT_REF(Obj)	CELL_UINT_REF(STACK_TOP_REF(Obj))
#define STACK_FTH_REF(Obj)	CELL_FTH_REF(STACK_TOP_REF(Obj))
#define STACK_VOIDP_REF(Obj)	CELL_VOIDP_REF(STACK_TOP_REF(Obj))
#define STACK_FN_REF(Obj)	CELL_FN_REF(STACK_TOP_REF(Obj))
#define STACK_LONG_REF(Obj)	CELL_LONG_REF(STACK_TOP_REF(Obj))
#define STACK_ULONG_REF(Obj)	CELL_ULONG_REF(STACK_TOP_REF(Obj))
#define STACK_FLOAT_REF(Obj)	CELL_FLOAT_REF(STACK_TOP_REF(Obj))
#define STACK_BOOL_REF(Obj)	CELL_BOOL_REF(STACK_TOP_REF(Obj))

#define STACK_INT_SET(Obj, Val)	CELL_INT_SET(STACK_TOP_REF(Obj), Val)
#define STACK_UINT_SET(Obj, Val) CELL_UINT_SET(STACK_TOP_REF(Obj), Val)
#define STACK_LONG_SET(Obj, Val) CELL_LONG_SET(STACK_TOP_REF(Obj), Val)
#define STACK_ULONG_SET(Obj, Val) CELL_ULONG_SET(STACK_TOP_REF(Obj), Val)
#define STACK_FLOAT_SET(Obj, Val) CELL_FLOAT_SET(STACK_TOP_REF(Obj), Val)
#define STACK_FTH_SET(Obj, Val)	CELL_FTH_SET(STACK_TOP_REF(Obj), Val)
#define STACK_VOIDP_SET(Obj, Val) CELL_VOIDP_SET(STACK_TOP_REF(Obj), Val)
#define STACK_FN_SET(Obj, Val)	CELL_FN_SET(STACK_TOP_REF(Obj), Val)
#define STACK_BOOL_SET(Obj, Val) CELL_BOOL_SET(STACK_TOP_REF(Obj), Val)

#define VM_STACK_INT_REF(Obj)	CELL_INT_REF(Obj)
#define VM_STACK_UINT_REF(Obj)	CELL_UINT_REF(Obj)
#define VM_STACK_FTH_REF(Obj)	CELL_FTH_REF(Obj)
#define VM_STACK_VOIDP_REF(Obj)	CELL_VOIDP_REF(Obj)
#define VM_STACK_LONG_REF(Obj)	CELL_LONG_REF(Obj)
#define VM_STACK_ULONG_REF(Obj)	CELL_ULONG_REF(Obj)
#define VM_STACK_FLOAT_REF(Obj)	CELL_FLOAT_REF(Obj)
#define VM_STACK_BOOL_REF(Obj)	CELL_BOOL_REF(Obj)

#define VM_STACK_INT_SET(Obj, Val)	CELL_INT_SET(Obj, Val)
#define VM_STACK_UINT_SET(Obj, Val)	CELL_UINT_SET(Obj, Val)
#define VM_STACK_FTH_SET(Obj, Val)	CELL_FTH_SET(Obj, Val)
#define VM_STACK_VOIDP_SET(Obj, Val)	CELL_VOIDP_SET(Obj, Val)
#define VM_STACK_LONG_SET(Obj, Val)	CELL_LONG_SET(Obj, Val)
#define VM_STACK_ULONG_SET(Obj, Val)	CELL_ULONG_SET(Obj, Val)
#define VM_STACK_FLOAT_SET(Obj, Val)	CELL_FLOAT_SET(Obj, Val)
#define VM_STACK_BOOL_SET(Obj, Val)	CELL_BOOL_SET(Obj, Val)

#define STACK_FTH_INDEX_REF(Stack, Idx)					\
	CELL_FTH_REF(&STACK_REF(Stack)->top[-Idx])
#define STACK_FTH_INDEX_SET(Stack, Idx, Val)				\
	CELL_FTH_SET(&STACK_REF(Stack)->top[-Idx], Val)

/*
** Stack methods... many map closely to required Forth words.
*/

ficlStack      *ficlStackCreate(ficlVm *, char *, unsigned);
int		ficlStackDepth(ficlStack *);
void		ficlStackDrop(ficlStack *, int);
ficlCell	ficlStackFetch(ficlStack *, int);
ficlCell	ficlStackGetTop(ficlStack *);
void		ficlStackPick(ficlStack *, int);
void		ficlStackReset(ficlStack *);
void		ficlStackRoll(ficlStack *, int);
void		ficlStackSetTop(ficlStack *, ficlCell);
void		ficlStackStore(ficlStack *, int, ficlCell);
void		ficlStackLink(ficlStack *, int);
void		ficlStackUnlink(ficlStack *);
void		ficlStackCheck(ficlStack *, int, int);
ficlCell	ficlStackPop(ficlStack *);
ficlInteger	ficlStackPopInteger(ficlStack *);
ficlUnsigned	ficlStackPopUnsigned(ficlStack *);
ficl2Unsigned	ficlStackPop2Unsigned(ficlStack *);
ficl2Integer	ficlStackPop2Integer(ficlStack *);
bool		ficlStackPopBoolean(ficlStack *);
void           *ficlStackPopPointer(ficlStack *);
FTH		ficlStackPopFTH(ficlStack *);
ficlFloat	ficlStackPopFloat(ficlStack *);

void		ficlStackPush(ficlStack *, ficlCell);
void		ficlStackPushInteger(ficlStack *, ficlInteger);
void		ficlStackPushUnsigned(ficlStack *, ficlUnsigned);
void		ficlStackPush2Integer(ficlStack *, ficl2Integer);
void		ficlStackPush2Unsigned(ficlStack *, ficl2Unsigned);
void		ficlStackPushBoolean(ficlStack *, bool);
void		ficlStackPushPointer(ficlStack *, void *);
void		ficlStackPushFTH(ficlStack *, FTH);
void		ficlStackPushFloat(ficlStack *, ficlFloat);

#define FICL_STACK_CHECK(stack, popCells, pushCells)			\
	ficlStackCheck(stack, popCells, pushCells)

typedef ficlInteger (*ficlStackWalkFunction)(void *constant, ficlCell *cell);

void		ficlStackWalk(ficlStack *,
		    ficlStackWalkFunction, void *, ficlInteger);
void		ficlStackDisplay(ficlStack *, ficlStackWalkFunction, void *);

typedef ficlWord **ficlIp;	/* the VM's instruction pointer */
typedef void (*ficlPrimitive)(ficlVm *vm);
typedef char *(*ficlInputFunction)(ficlCallback *callback);
typedef void (*ficlOutputFunction)(ficlCallback *callback, char *text);

/*
** Each VM has a placeholder for an output function -
** this makes it possible to have each VM do I/O
** through a different device. If you specify no
** ficlOutputFunction, it defaults to ficlCallbackDefaultTextOut.
**
** You can also set a specific handler just for errors.
** If you don't specify one, it defaults to using textOut.
*/
struct ficlCallback {
	void           *context;
	ficlInputFunction textIn;
	ficlOutputFunction textOut;
	ficlOutputFunction errorOut;
	ficlSystem     *system;
	ficlVm         *vm;
	FTH		port_in;
	FTH		port_out;
	FTH		port_err;
	int		stdin_fileno;
	int		stdout_fileno;
	int		stderr_fileno;
	FILE           *stdin_ptr;
	FILE           *stdout_ptr;
	FILE           *stderr_ptr;
};

/*
** Starting with Ficl 4.0, Ficl uses a "switch-threaded" inner loop,
** where each primitive word is represented with a numeric constant,
** and words are (more or less) arrays of these constants.  In Ficl
** these constants are an enumerated type called ficlInstruction.
*/
typedef enum {
#define FICL_TOKEN(token, description)                    token,
#define FICL_INSTRUCTION_TOKEN(token, description, flags) token,
#include "ficltokens.h"
#undef FICL_TOKEN
#undef FICL_INSTRUCTION_TOKEN
	ficlInstructionLast,
#if (FTH_SIZEOF_LONG == 4)
	ficlInstructionFourByteTrick = 0x10000000
#else
	ficlInstructionEightByteTrick = 0x1000000010000000
#endif
} ficlInstruction;

/* 
** The virtual machine (VM) contains the state for one interpreter.
** Defined operations include:
** Create & initialize
** Delete
** Execute a block of text
** Parse a word out of the input stream
** Call return, and branch 
** Text output
** Throw an exception
*/

#define GC_FRAME_SIZE		128

struct ficlVm {
	void           *context;
	void           *repl;	/* [ms] for libtecla's GetLine *gl */
	ficlCallback	callback;
	ficlVm         *link;	/* Ficl keeps a VM list for simple teardown */
	jmp_buf        *exceptionHandler;	/* crude exception
						 * mechanism... */
	short		restart;/* Set TRUE to restart runningWord */
	ficlIp		ip;	/* instruction pointer */
	ficlWord       *runningWord;	/* address of currently running word
					 * (often just *(ip-1) ) */
	ficlUnsigned	state;	/* compiling or interpreting */
	ficlUnsigned	base;	/* number conversion base */
	ficlStack      *dataStack;
	ficlStack      *returnStack;	/* return stack */
	int		fth_catch_p;	/* are we in fth-catch? [ms] */
	int		gc_frame_level;	/* [ms] gc_push/pop */
	ficlCell	sourceId;	/* -1 if EVALUATE, 0 if normal input,
					 * >0 if a file */
	ficlTIB		tib;	/* address of incoming text string */
	ficlCell	user[FICL_USER_CELLS];
	ficlWord       *gc_word[GC_FRAME_SIZE];	/* [ms] gc_push/pop */
	void           *gc_inst[GC_FRAME_SIZE];	/* [ms] gc_push/pop */
	char		pad_eval[FICL_PAD_SIZE + 1];	/* second scratch area
							 * (ficlVmEvaluate) */
	char		pad[FICL_PAD_SIZE + 1];	/* the scratch area
						 * (see above) */
};

/*
** Each VM operates in one of two non-error states: interpreting
** or compiling. When interpreting, words are simply executed.
** When compiling, most words in the input stream have their
** addresses inserted into the word under construction. Some words
** (known as IMMEDIATE) are executed in the compile state, too.
*/
/* values of STATE */
#define FICL_VM_STATE_INTERPRET	(0)
#define FICL_VM_STATE_COMPILE	(1)

/*
** Exit codes for vmThrow
*/
#define FICL_VM_STATUS_OFFSET	256
#define FICL_VM_STATUS_INNER_EXIT					\
	(-(FICL_VM_STATUS_OFFSET + 0)) /* tell ficlVmExecuteXT
					* to exit inner loop */
#define FICL_VM_STATUS_OUT_OF_TEXT					\
	(-(FICL_VM_STATUS_OFFSET + 1)) /* hungry - normal exit */
#define FICL_VM_STATUS_RESTART						\
	(-(FICL_VM_STATUS_OFFSET + 2)) /* word needs more text
					* to succeed -- re-run it */
#define FICL_VM_STATUS_USER_EXIT					\
	(-(FICL_VM_STATUS_OFFSET + 3)) /* user wants to quit */
#define FICL_VM_STATUS_ERROR_EXIT					\
	(-(FICL_VM_STATUS_OFFSET + 4)) /* interpreter found
					* an error */
#define FICL_VM_STATUS_BREAK						\
	(-(FICL_VM_STATUS_OFFSET + 5)) /* debugger breakpoint */
#define FICL_VM_STATUS_SKIP_FILE					\
	(-(FICL_VM_STATUS_OFFSET + 6)) /* [ms] skip loading file */
#define FICL_VM_STATUS_LAST_FICL_ERROR					\
	(-(FICL_VM_STATUS_OFFSET + 7))
#define FICL_VM_STATUS_LAST_FICL					\
	(-FICL_VM_STATUS_LAST_FICL_ERROR - FICL_VM_STATUS_OFFSET)


/* [ms] Access to the ANS exception strings. */
char           *ficl_ans_exc_name(int);
char           *ficl_ans_exc_msg(int);

/* [ms] Full list of ANS exceptions. */
#define FICL_VM_STATUS_ABORT		(-1) /* like FICL_VM_STATUS_ERROR_EXIT
					      * -- abort */
#define FICL_VM_STATUS_ABORTQ		(-2) /* like FICL_VM_STATUS_ERROR_EXIT
					      * -- abort" */
#define FICL_VM_STATUS_STACK_OVERFLOW	(-3) /* stack overflow */
#define FICL_VM_STATUS_STACK_UNDERFLOW	(-4) /* stack underflow */
#define FICL_VM_STATUS_RSTACK_OVERFLOW	(-5) /* return stack overflow */
#define FICL_VM_STATUS_RSTACK_UNDERFLOW	(-6) /* return stack underflow */
#define FICL_VM_STATUS_TOO_DEEP		(-7) /* do-loops nested too deeply
					      * during execution */
#define FICL_VM_STATUS_DICT_OVERFLOW	(-8) /* dictionary overflow */
#define FICL_VM_STATUS_MEMORY_ACCESS	(-9) /* invalid memory address */
#define FICL_VM_STATUS_DIVISION_BY_ZERO	(-10) /* division by zero */
#define FICL_VM_STATUS_RANGE_ERROR	(-11) /* result out of range */
#define FICL_VM_STATUS_ARGUMENT_ERROR	(-12) /* argument type mismatch */
#define FICL_VM_STATUS_UNDEFINED	(-13) /* undefined word */
#define FICL_VM_STATUS_COMPILE_ONLY	(-14) /* interpreting a compile-only
					       * word */
#define FICL_VM_STATUS_INVALID_FORGET	(-15) /* invalid FORGET */
#define FICL_VM_STATUS_ZERO_STRING	(-16) /* attempt to use zero-length
					       * string as a name */
#define FICL_VM_STATUS_PNO_OVERFLOW	(-17) /* pictured numeric output
					       * string overflow */
#define FICL_VM_STATUS_PARSE_OVERFLOW	(-18) /* parsed string overflow */
#define FICL_VM_STATUS_NAME_TOO_LONG	(-19) /* definition name too long */
#define FICL_VM_STATUS_MEMORY_WRITE_ERROR (-20) /* write to a read-only
                                                 * location */
#define FICL_VM_STATUS_NOT_IMPLEMENTED	(-21) /* unsupported operation */
#define FICL_VM_STATUS_CONTROL_MISMATCH	(-22) /* control structure mismatch */
#define FICL_VM_STATUS_ALIGNMENT_ERROR	(-23) /* address alignment exception */
#define FICL_VM_STATUS_NUMERIC_ARG_ERROR (-24) /* invalid numeric argument */
#define FICL_VM_STATUS_RSTACK_IMBALANCE	(-25) /* return stack imbalance */
#define FICL_VM_STATUS_MISSING_LPARAMETER (-26) /* loop parameters
						 * unavailable */
#define FICL_VM_STATUS_RECURSION_ERROR	(-27) /* invalid recursion */
#define FICL_VM_STATUS_INTERRUPT	(-28) /* user interrupt */
#define FICL_VM_STATUS_COMPILER_NESTING	(-29) /* compiler nesting */
#define FICL_VM_STATUS_OBSOLETE		(-30) /* obsolescent feature */
#define FICL_VM_STATUS_TO_BODY_ERROR	(-31) /* >BODY used on non-CREATEd
					       * definition */
#define FICL_VM_STATUS_NAME_ARG_ERROR	(-32) /* invalid name argument
					       * (e.g., TO xxx) */
#define FICL_VM_STATUS_BREAD_ERROR	(-33) /* block read exception */
#define FICL_VM_STATUS_BWRITE_ERROR	(-34) /* block write exception */
#define FICL_VM_STATUS_BNUMBER_ERROR	(-35) /* invalid block number */
#define FICL_VM_STATUS_FPOSITION_ERROR	(-36) /* invalid file position */
#define FICL_VM_STATUS_FILE_IO_ERROR	(-37) /* file I/O exception */
#define FICL_VM_STATUS_NO_SUCH_FILE	(-38) /* non-existent file */
#define FICL_VM_STATUS_EOF_ERROR	(-39) /* unexpected end of file */
#define FICL_VM_STATUS_FBASE_ERROR	(-40) /* invalid BASE for
					       * floating point conversion */
#define FICL_VM_STATUS_PRECISION_ERROR	(-41) /* loss of precision */
#define FICL_VM_STATUS_FDIVIDE_BY_ZERO	(-42) /* floating-point divide
					       * by zero */
#define FICL_VM_STATUS_FRANGE_ERROR	(-43) /* floating-point result
					       * out of range */
#define FICL_VM_STATUS_FSTACK_OVERFLOW	(-44) /* floating-point stack
					       * overflow */
#define FICL_VM_STATUS_FSTACK_UNDERFLOW	(-45) /* floating-point stack underflow */
#define FICL_VM_STATUS_FNUMBER_ERROR	(-46) /* floating-point invalid
					       * argument */
#define FICL_VM_STATUS_WORD_LIST_ERROR	(-47) /* compilation word list
					       * deleted */
#define FICL_VM_STATUS_POSTPONE_ERROR	(-48) /* invalid POSTPONE */
#define FICL_VM_STATUS_SEARCH_OVERFLOW	(-49) /* search-order overflow */
#define FICL_VM_STATUS_SEARCH_UNDERFLOW	(-50) /* search-order underflow */
#define FICL_VM_STATUS_WORD_LIST_CHANGED (-51) /* compilation word list
						* changed */
#define FICL_VM_STATUS_CS_OVERFLOW	(-52) /* control-flow stack overflow */
#define FICL_VM_STATUS_ES_OVERFLOW	(-53) /* exception stack overflow */
#define FICL_VM_STATUS_FP_UNDERFLOW	(-54) /* floating-point underflow */
#define FICL_VM_STATUS_FP_ERROR		(-55) /* floating-point unidentified
					       * fault */
#define FICL_VM_STATUS_QUIT		(-56) /* like FICL_VM_STATUS_ERROR_EXIT,
					       * but leave dataStack &
					       * base alone */
#define FICL_VM_STATUS_CHAR_ERROR	(-57) /* exception in sending or
					       * receiving a character */
#define FICL_VM_STATUS_BRANCH_ERROR	(-58) /* [IF], [ELSE], or [THEN]
					       * exception */
#define FICL_VM_STATUS_LAST_ERROR	(-59)
#define FICL_VM_STATUS_LAST_ANS		(-FICL_VM_STATUS_LAST_ERROR)

void		ficlVmBranchRelative(ficlVm *, int);
ficlVm         *ficlVmCreate(ficlVm *, unsigned, unsigned);
void		ficlVmDestroy(ficlVm *);
ficlDictionary *ficlVmGetDictionary(ficlVm *);
char           *ficlVmGetString(ficlVm *, ficlCountedString *, char);
ficlString	ficlVmGetWord(ficlVm *);
ficlString	ficlVmGetWord0(ficlVm *);
int		ficlVmGetWordToPad(ficlVm *);
void		ficlVmInnerLoop(ficlVm *, ficlWord *volatile);
ficlString	ficlVmParseString(ficlVm *, char);
ficlString	ficlVmParseStringEx(ficlVm *, char, int);
ficlCell	ficlVmPop(ficlVm *);
void		ficlVmPush(ficlVm *, ficlCell);
void		ficlVmPopIP(ficlVm *);
void		ficlVmPushIP(ficlVm *, ficlIp);
void		ficlVmQuit(ficlVm *);
void		ficlVmReset(ficlVm *);
void		ficlVmSetTextIn(ficlVm *, ficlInputFunction);
void		ficlVmSetTextOut(ficlVm *, ficlOutputFunction);
void		ficlVmSetErrorOut(ficlVm *, ficlOutputFunction);
void		ficlVmThrow(ficlVm *, int);
void		ficlVmThrowError(ficlVm *, const char *,...);
void		ficlVmThrowErrorVararg(ficlVm *, int, const char *, va_list);
/* [ms]*/
void		ficlVmThrowException(ficlVm *, int, const char *,...);

#define ficlVmGetContext(vm)	((vm)->context)
#define ficlVmGetDataStack(vm)	((vm)->dataStack)
#define ficlVmGetFloatStack(vm)	((vm)->dataStack)
#define ficlVmGetRepl(vm)	((vm)->repl)
#define ficlVmGetReturnStack(vm) ((vm)->returnStack)
#define ficlVmGetRunningWord(vm) ((vm)->runningWord)

#define ficlVmGetPortIn(vm)	((vm)->callback.port_in)
#define ficlVmGetPortOut(vm)	((vm)->callback.port_out)
#define ficlVmGetPortErr(vm)	((vm)->callback.port_err)
#define ficlVmGetStdin(vm)	((vm)->callback.stdin_ptr)
#define ficlVmGetStdout(vm)	((vm)->callback.stdout_ptr)
#define ficlVmGetStderr(vm)	((vm)->callback.stderr_ptr)
#define ficlVmGetFilenoIn(vm)	((vm)->callback.stdin_fileno)
#define ficlVmGetFilenoOut(vm)	((vm)->callback.stdout_fileno)
#define ficlVmGetFilenoErr(vm)	((vm)->callback.stderr_fileno)

char           *ficl_running_word(ficlVm *);
void		ficlVmDisplayDataStack(ficlVm *);
void		ficlVmDisplayDataStackSimple(ficlVm *);
void		ficlVmDisplayReturnStack(ficlVm *);

/*
** f i c l E v a l u a t e
** Evaluates a block of input text in the context of the
** specified interpreter. Also sets SOURCE-ID properly.
**
** PLEASE USE THIS FUNCTION when throwing a hard-coded
** string to the Ficl interpreter.
*/
int		ficlVmEvaluate(ficlVm *, char *);

/*
** f i c l V m E x e c *
** Evaluates a block of input text in the context of the
** specified interpreter. Emits any requested output to the
** interpreter's output function. If the input string is NULL
** terminated, you can pass -1 as nChars rather than count it.
** Execution returns when the text block has been executed,
** or an error occurs.
** Returns one of the FICL_VM_STATUS_... codes defined in ficl.h:
** FICL_VM_STATUS_OUT_OF_TEXT is the normal exit condition
** FICL_VM_STATUS_ERROR_EXIT means that the interpreter encountered a
**      syntax error and the vm has been reset to recover (some or all
**      of the text block got ignored
** FICL_VM_STATUS_USER_EXIT means that the user executed the "bye" command
**      to shut down the interpreter. This would be a good
**      time to delete the vm, etc -- or you can ignore this
**      signal.
** FICL_VM_STATUS_ABORT and FICL_VM_STATUS_ABORTQ are generated by
**      'abort' and 'abort"' commands.
** Preconditions: successful execution of ficlInitSystem,
**      Successful creation and init of the VM by ficlNewVM (or equivalent)
**
** If you call ficlExec() or one of its brothers, you MUST
** ensure vm->sourceId was set to a sensible value.
** ficlExec() explicitly DOES NOT manage SOURCE-ID for you.
*/
int		ficlVmExecuteString(ficlVm *, ficlString);
int		ficlVmExecuteXT(ficlVm *, ficlWord *);
void		ficlVmExecuteInstruction(ficlVm *, ficlInstruction);
#if 1
#define ficlVmExecuteWord(vm, word) ficlVmInnerLoop(vm, word)
#else
void		ficlVmExecuteWord(ficlVm *, ficlWord *);
#endif
void		ficlVmDictionaryAllot(ficlDictionary *, int);
void		ficlVmDictionaryAllotCells(ficlDictionary *, int);
int		ficlVmParseWord(ficlVm *, ficlString);

/*
** TIB access routines...
** ANS forth seems to require the input buffer to be represented 
** as a pointer to the start of the buffer, and an index to the
** next character to read.
** PushTib points the VM to a new input string and optionally
**  returns a copy of the current state
** PopTib restores the TIB state given a saved TIB from PushTib
** GetInBuf returns a pointer to the next unused char of the TIB
*/
void		ficlVmPushTib(ficlVm *, char *, ficlInteger, ficlTIB *);
void		ficlVmPopTib(ficlVm *, ficlTIB *);

#define ficlVmGetInBuf(vm)	((vm)->tib.text + (vm)->tib.index)
#define ficlVmGetInBufLen(vm)	((vm)->tib.end - (vm)->tib.text)
#define ficlVmGetInBufEnd(vm)	((vm)->tib.end)
#define ficlVmGetTibIndex(vm)	((vm)->tib.index)
#define ficlVmSetTibIndex(vm, i) ((vm)->tib.index = i)
#define ficlVmUpdateTib(vm, str) ((vm)->tib.index = (str) - (vm)->tib.text)

void		ficlVmDictionaryCheck(ficlDictionary *, int);
void		ficlVmDictionarySimpleCheck(ficlDictionary *, int);
void		ficlPrimitiveLiteralIm(ficlVm *);

/*
** A FICL_CODE points to a function that gets called to help execute
** a word in the dictionary. It always gets passed a pointer to the
** running virtual machine, and from there it can get the address
** of the parameter area of the word it's supposed to operate on.
** For precompiled words, the code is all there is. For user defined
** words, the code assumes that the word's parameter area is a list
** of pointers to the code fields of other words to execute, and
** may also contain inline data. The first parameter is always
** a pointer to a code field.
*/

/* 
** Ficl models memory as a contiguous space divided into
** words in a linked list called the dictionary.
** A ficlWord starts each entry in the list.
** Version 1.02: space for the name characters is allotted from
** the dictionary ahead of the word struct, rather than using
** a fixed size array for each name.
*/

struct ficlWord {
	struct ficlWord *link;	/* Previous word in the dictionary      */
	struct ficlWord *current_word;	/* word where ficlWord is used [ms] */
	ficlPrimitive	code;	/* Native code to execute the word      */
	ficlInstruction	semiParen;	/* native code to execute the word */
	char           *name;	/* First nFICLNAME chars of word name   */
	void            (*vfunc) ();	/* void function to use [ms]    */
	FTH             (*func) ();	/* function to use [ms]         */
	FTH		properties;	/* property-hash [ms]           */
	FTH		current_file;	/* file where ficlWord is used [ms] */
	FTH		file;	/* origin file of word [ms]             */
	int		primitive_p;	/* C-primitive or not [ms]      */
	int		req;	/* required args [ms]                   */
	int		opt;	/* optional args [ms]                   */
	int		rest;	/* 1 if rest args, 0 otherwise [ms]     */
	int		argc;	/* number of all args [ms]              */
	int		kind;	/* word, proc, symbol, keyword, exception
				 * [ms] */
	ficlInteger	current_line;	/* line where ficlWord is used [ms] */
	ficlInteger	line;	/* line in source [ms]                  */
	ficlUnsigned	flags;	/* Immediate, Smudge, Compile-only, IsOjbect,
				 * Instruction */
	ficlUnsigned	length;	/* Number of chars in word name         */
	ficlUnsigned	hash;
	ficlCell	param[1];	/* First data cell of the word  */
};

/*
** ficlWord.flag bitfield values:
*/

/*
** FICL_WORD_IMMEDIATE:
** This word is always executed immediately when
** encountered, even when compiling.
*/
#define FICL_WORD_IMMEDIATE	(1UL)

/*
** FICL_WORD_COMPILE_ONLY:
** This word is only valid during compilation.
** Ficl will throw a runtime error if this word executed
** while not compiling.
*/
#define FICL_WORD_COMPILE_ONLY	(2UL)

/*
** FICL_WORD_SMUDGED
** This word's definition is in progress.
** The word is hidden from dictionary lookups
** until it is "un-smudged".
*/
#define FICL_WORD_SMUDGED	(4UL)

/*
** FICL_WORD_OBJECT
** This word is an object or object member variable.
** (Currently only used by "my=[".)
*/
#define FICL_WORD_OBJECT	(8UL)

/*
** FICL_WORD_INSTRUCTION
** This word represents a ficlInstruction, not a normal word.
** param[0] is the instruction.
** When compiled, Ficl will simply copy over the instruction,
** rather than executing the word as normal.
**
** (Do *not* use this flag for words that need their PFA pushed
** before executing!)
*/
#define FICL_WORD_INSTRUCTION	(16UL)

/*
** FICL_WORD_COMPILE_ONLY_IMMEDIATE
** Most words that are "immediate" are also
** "compile-only".
*/
#define FICL_WORD_COMPILE_ONLY_IMMEDIATE				\
	(FICL_WORD_IMMEDIATE | FICL_WORD_COMPILE_ONLY)
#define FICL_WORD_DEFAULT	(0UL)

/*
** Worst-case size of a word header: FICL_NAME_LENGTH chars in name
*/
#define FICL_CELLS_PER_WORD						\
	((sizeof(ficlWord) + FICL_NAME_LENGTH + sizeof(ficlCell)) /	\
	(sizeof(ficlCell)))

int		ficlWordIsImmediate(ficlWord *);
int		ficlWordIsCompileOnly(ficlWord *);

void ficlCallbackAssert(int expression, char *expr, char *file, int line);
/*
 * XXX: FICL_ASSERT()
 */
#if 0
#define FICL_ASSERT(Expr)						\		ficlCallbackAssert(((int)(Expr)), #Expr, __FILE__, __LINE__)
#else
#define FICL_ASSERT(Expr)	/* empty */
#endif

/*
** Generally useful string manipulators omitted by ANSI C...
** ltoa complements strtol
*/

int		ficlIsPowerOfTwo(ficlUnsigned);
char           *ficlLtoa(ficlInteger, char *, int);
char           *ficlUltoa(ficlUnsigned, char *, int);
char		ficlDigitToCharacter(int);
char           *ficlStringReverse(char *);
char           *ficlStringSkipSpace(char *, char *);
char           *ficlStringCaseFold(char *);
void           *ficlAlignPointer(void *);
int		intern_ficlStrincmp(char *, char *, ficlUnsigned);

#if defined(HAVE_STRNCASECMP)
#if defined(HAVE_STRINGS_H)
#include <strings.h>
#endif
#define ficlStrincmp(s1, s2, len)	strncasecmp(s1, s2, len)
#else
#define ficlStrincmp(s1, s2, len)	intern_ficlStrincmp(s1, s2, len)
#endif

/*
** Ficl hash table - variable size.
** assert(size > 0)
** If size is 1, the table degenerates into a linked list.
** A WORDLIST (see the search order word set in DPANS) is
** just a pointer to a FICL_HASH in this implementation.
*/
typedef struct ficlHash {
	struct ficlHash *link;	/* link to parent class wordlist for OO */
	char           *name;	/* optional pointer to \0 terminated wordlist
				 * name */
	unsigned	size;	/* number of buckets in the hash */
	ficlWord       *table[1];
} ficlHash;

void		ficlHashForget(ficlHash *, void *);
ficlUnsigned	ficlHashCode(ficlString);
void		ficlHashInsertWord(ficlHash *, ficlWord *);
ficlWord       *ficlHashLookup(ficlHash *, ficlString, ficlUnsigned);
void		ficlHashReset(ficlHash *);

/*
** A Dictionary is a linked list of FICL_WORDs. It is also Ficl's
** memory model. Description of fields:
**
** here -- points to the next free byte in the dictionary. This
**      pointer is forced to be CELL-aligned before a definition is added.
**      Do not assume any specific alignment otherwise - Use dictAlign().
**
** smudge -- pointer to word currently being defined (or last defined word)
**      If the definition completes successfully, the word will be
**      linked into the hash table. If unsuccessful, dictUnsmudge
**      uses this pointer to restore the previous state of the dictionary.
**      Smudge prevents unintentional recursion as a side-effect: the
**      dictionary search algo examines only completed definitions, so a 
**      word cannot invoke itself by name. See the Ficl word "recurse".
**      NOTE: smudge always points to the last word defined. IMMEDIATE
**      makes use of this fact. Smudge is initially NULL.
**
** forthWordlist -- pointer to the default wordlist (FICL_HASH).
**      This is the initial compilation list, and contains all
**      Ficl's precompiled words.
**
** compilationWordlist -- compilation wordlist -
**      initially equal to forthWordlist
** wordlists  -- array of pointers to wordlists. Managed as a stack.
**      Highest index is the first list in the search order.
** wordlistCount   -- number of lists in wordlists.
**      wordlistCount-1 is the highest 
**      filled slot in wordlists, and points to the first wordlist
**      in the search order
** size -- number of cells in the dictionary (total)
** base -- start of data area. Must be at the end of the struct.
*/
struct ficlDictionary {
	ficlCell       *here;
	void           *context;/* for your use, particularly with
				 * ficlDictionaryLock() */
	ficlWord       *smudge;
	ficlHash       *forthWordlist;
	ficlHash       *compilationWordlist;
	ficlHash       *wordlists[FICL_MAX_WORDLISTS];
	ficlInteger	wordlistCount;
	ficlUnsigned	size;	/* Number of cells in dictionary (total) */
	ficlSystem     *system;	/* used for debugging */
	ficlCell	base[1];	/* Base of dictionary memory */
};

void		ficlDictionaryAbortDefinition(ficlDictionary *);
void		ficlDictionaryAlign(ficlDictionary *);
void		ficlDictionaryAllot(ficlDictionary *, int);
void		ficlDictionaryAllotCells(ficlDictionary *, int);
void		ficlDictionaryAppendCell(ficlDictionary *, ficlCell);
void		ficlDictionaryAppendPointer(ficlDictionary *, void *);
void		ficlDictionaryAppendInteger(ficlDictionary *, ficlInteger);
void		ficlDictionaryAppendFTH(ficlDictionary *, FTH);
void		ficlDictionaryAppendCharacter(ficlDictionary *, char);
void		ficlDictionaryAppendUnsigned(ficlDictionary *, ficlUnsigned);
void           *ficlDictionaryAppendData(ficlDictionary *, void *, ficlInteger);
char           *ficlDictionaryAppendString(ficlDictionary *, ficlString);
ficlWord       *ficlDictionaryAppendWord(ficlDictionary *,
		    ficlString, ficlPrimitive, ficlUnsigned);
ficlWord       *ficlDictionaryAppendPrimitive(ficlDictionary *, char *,
		    ficlPrimitive, ficlUnsigned);
ficlWord       *ficlDictionaryAppendInstruction(ficlDictionary *, char *,
		    ficlInstruction, ficlUnsigned);
ficlWord       *ficlDictionaryAppendConstantInstruction(ficlDictionary *,
		    ficlString, ficlInstruction, ficlInteger);
ficlWord       *ficlDictionaryAppendConstant(ficlDictionary *, char *,
		    ficlInteger);
ficlWord       *ficlDictionaryAppendPointerConstant(ficlDictionary *,
		    char *, void *);
ficlWord       *ficlDictionaryAppendFTHConstant(ficlDictionary *, char *, FTH);
ficlWord       *ficlDictionarySetConstantInstruction(ficlDictionary *,
		    ficlString, ficlInstruction, ficlInteger);
ficlWord       *ficlDictionarySetConstant(ficlDictionary *,
		    char *, ficlInteger);
ficlWord       *ficlDictionaryAppendFTHConstantInstruction(ficlDictionary *,
		    ficlString, ficlInstruction, FTH);
ficlWord       *ficlDictionarySetFTHConstantInstruction(ficlDictionary *,
		    ficlString, ficlInstruction, FTH);
ficlWord       *ficlDictionarySetFTHConstant(ficlDictionary *, char *, FTH);
ficlWord       *ficlDictionarySetPrimitive(ficlDictionary *, char *,
		    ficlPrimitive, ficlUnsigned);
ficlWord       *ficlDictionarySetInstruction(ficlDictionary *, char *,
		    ficlInstruction, ficlUnsigned);
int		ficlDictionaryCellsAvailable(ficlDictionary *);
int		ficlDictionaryCellsUsed(ficlDictionary *);
ficlDictionary *ficlDictionaryCreate(ficlSystem *, unsigned);
ficlDictionary *ficlDictionaryCreateHashed(ficlSystem *, unsigned, unsigned);
ficlHash       *ficlDictionaryCreateWordlist(ficlDictionary *, int);
void		ficlDictionaryDestroy(ficlDictionary *);
void		ficlDictionaryEmpty(ficlDictionary *, unsigned);
int		ficlDictionaryIncludes(ficlDictionary *, void *);
ficlWord       *ficlDictionaryLookup(ficlDictionary *, ficlString);
void		ficlDictionaryResetSearchOrder(ficlDictionary *);
void		ficlDictionarySetFlags(ficlDictionary *, ficlUnsigned);
void		ficlDictionaryClearFlags(ficlDictionary *, ficlUnsigned);
void		ficlDictionarySetImmediate(ficlDictionary *);
void		ficlDictionaryUnsmudge(ficlDictionary *);
ficlCell       *ficlDictionaryWhere(ficlDictionary *);

int		ficlDictionaryIsAWord(ficlDictionary *, ficlWord *);
void		ficlDictionarySee(ficlDictionary *, ficlWord *);
ficlWord       *ficlDictionaryFindEnclosingWord(ficlDictionary *, ficlCell *);

/* 
** P A R S E   S T E P
** (New for 2.05)
** See words.c: interpWord
** By default, Ficl goes through two attempts to parse each token from its input
** stream: it first attempts to match it with a word in the dictionary, and
** if that fails, it attempts to convert it into a number. This mechanism is now
** extensible by additional steps. This allows extensions like floating point
** and double number support to be factored cleanly.
**
** Each parse step is a function that receives the next input token as a
** STRINGINFO.  If the parse step matches the token, it must apply semantics
** to the token appropriate to the present value of VM.state
** (compiling or interpreting), and return FICL_TRUE.
** Otherwise it returns FICL_FALSE. See words.c: isNumber for an example
**
** Note: for the sake of efficiency, it's a good idea both to limit the number
** of parse steps and to code each parse step so that it rejects tokens that
** do not match as quickly as possible.
*/
typedef int (*ficlParseStep)(ficlVm *vm, ficlString s);

/*
** FICL_BREAKPOINT record.
** oldXT - if NULL, this breakpoint is unused. Otherwise it stores the xt 
** that the breakpoint overwrote. This is restored to the dictionary when the
** BP executes or gets cleared
** address - the location of the breakpoint (address of the instruction that
**           has been replaced with the breakpoint trap
** oldXT  - The original contents of the location with the breakpoint
** Note: address is NULL when this breakpoint is empty
*/
typedef struct {
  void     *address;
  ficlWord *oldXT;
} ficlBreakpoint;

/*
** F I C L _ S Y S T E M
** The top level data structure of the system - ficl_system ties a list of
** virtual machines with their corresponding dictionaries. Ficl 3.0 added
** support for multiple Ficl systems, allowing multiple concurrent sessions 
** to separate dictionaries with some constraints. 
** Note: the context pointer is there to provide context for applications.
** It is copied to each VM's context field as that VM is created.
*/
struct ficlSystemInformation {
	int		size;	/* structure size tag for versioning */
	void           *context;/* Initializes VM's context pointer - for
				 * application use */
	unsigned int	dictionarySize;	/* Size of system's Dictionary, in
					 * cells */
	unsigned int	environmentSize;	/* Size of Environment
						 * dictionary, in cells */
	unsigned int	stackSize;	/* Size of all stacks created, in
					 * cells */
	unsigned int	returnSize;	/* [ms] */
	unsigned int	localsSize;	/* [ms] */
	ficlInputFunction textIn;	/* default textIn function [ms] */
	ficlOutputFunction textOut;	/* default textOut function */
	ficlOutputFunction errorOut;	/* textOut function used for errors */
	FTH		port_in;	/* rest added by [ms] */
	FTH		port_out;
	FTH		port_err;
	int		stdin_fileno;
	int		stdout_fileno;
	int		stderr_fileno;
	FILE           *stdin_ptr;
	FILE           *stdout_ptr;
	FILE           *stderr_ptr;
};

#define ficlSystemInformationInitialize(x)				\
	{								\
		memset((x), 0, sizeof(ficlSystemInformation));		\
		(x)->size = (int)sizeof(ficlSystemInformation);		\
	}

struct ficlSystem {
	void           *context;
	ficlCallback	callback;
	ficlSystem     *link;
	ficlVm         *vmList;
	ficlDictionary *dictionary;
	ficlDictionary *environment;
	ficlDictionary *symbols;/* [ms] */
	ficlWord       *interpreterLoop[3];
	ficlWord       *parseList[FICL_MAX_PARSE_STEPS];
	ficlWord       *exitInnerWord;
	ficlWord       *interpretWord;
	ficlDictionary *locals;
	ficlInteger	localsCount;
	ficlCell       *localsFixup;
	unsigned	stackSize;
	unsigned	returnSize;	/* [ms] */
	ficlBreakpoint	breakpoint;
};

#define ficlSystemGetContext(system)	((system)->context)

/*
** External interface to Ficl...
*/
/* 
** f i c l S y s t e m C r e a t e
** Binds a global dictionary to the interpreter system and initializes
** the dictionary to contain the ANSI CORE wordset. 
** You can specify the address and size of the allocated area.
** You can also specify the text output function at creation time.
** After that, Ficl manages it.
** First step is to set up the static pointers to the area.
** Then write the "precompiled" portion of the dictionary in.
** The dictionary needs to be at least large enough to hold the
** precompiled part. Try 1K cells minimum. Use "words" to find
** out how much of the dictionary is used at any time.
*/
ficlSystem     *ficlSystemCreate(ficlSystemInformation *);

/*
** f i c l S y s t e m D e s t r o y
** Deletes the system dictionary and all virtual machines that
** were created with ficlNewVM (see below). Call this function to
** reclaim all memory used by the dictionary and VMs.
*/
void		ficlSystemDestroy(ficlSystem *);

/*
** Create a new VM from the heap, and link it into the system VM list.
** Initializes the VM and binds default sized stacks to it. Returns the
** address of the VM, or NULL if an error occurs.
** Precondition: successful execution of ficlInitSystem
*/
ficlVm         *ficlSystemCreateVm(ficlSystem *);

/*
** Force deletion of a VM. You do not need to do this 
** unless you're creating and discarding a lot of VMs.
** For systems that use a constant pool of VMs for the life
** of the system, ficltermSystem takes care of VM cleanup
** automatically.
*/
void		ficlSystemDestroyVm(ficlVm *);

/*
** Returns the address of the most recently defined word in the system
** dictionary with the given name, or NULL if no match.
** Precondition: successful execution of ficlInitSystem
*/
ficlWord       *ficlSystemLookup(ficlSystem *, char *);

/*
** f i c l G e t D i c t
** Utility function - returns the address of the system dictionary.
** Precondition: successful execution of ficlInitSystem
*/
ficlDictionary *ficlSystemGetDictionary(ficlSystem *);
ficlDictionary *ficlSystemGetEnvironment(ficlSystem *);
ficlDictionary *ficlSystemGetLocals(ficlSystem *);
ficlDictionary *ficlSystemGetSymbols(ficlSystem *);	/* [ms] */

/* 
** f i c l C o m p i l e C o r e
** Builds the ANS CORE wordset into the dictionary - called by
** ficlInitSystem - no need to waste dictionary space by doing it again.
*/
void		ficlSystemCompileCore(ficlSystem *);
void		ficlSystemCompilePrefix(ficlSystem *);
void		ficlSystemCompileSearch(ficlSystem *);
void		ficlSystemCompileSoftCore(ficlSystem *);
void		ficlSystemCompileTools(ficlSystem *);
void		ficlSystemCompileFile(ficlSystem *);
int		ficlVmParseFloatNumber(ficlVm *, ficlString);
void		ficlSystemCompilePlatform(ficlSystem *);
void		ficlSystemCompileExtras(ficlSystem *);

int		ficlVmParsePrefix(ficlVm *, ficlString);
ficlWord       *ficlSystemLookupLocal(ficlSystem *, ficlString);

/*
** from words.c...
*/
int		ficlVmParseNumber(ficlVm *, ficlString);
void		ficlPrimitiveTick(ficlVm *);
void		ficlPrimitiveParseStepParen(ficlVm *);

/*
** Appends a parse step function to the end of the parse list (see 
** FICL_PARSE_STEP notes in ficl.h for details). Returns 0 if successful,
** nonzero if there's no more room in the list. Each parse step is a word in 
** the dictionary. Precompiled parse steps can use (PARSE-STEP) as their 
** CFA - see parenParseStep in words.c.
*/
int		ficlSystemAddParseStep(ficlSystem *, ficlWord *); /* ficl.c */
void		ficlSystemAddPrimitiveParseStep(ficlSystem *,
		    char *, ficlParseStep);

/*
** From tools.c
*/

/* 
** The following supports SEE and the debugger.
*/
typedef enum {
	FICL_WORDKIND_BRANCH,
	FICL_WORDKIND_BRANCH0,
	FICL_WORDKIND_COLON,
	FICL_WORDKIND_CONSTANT,
	FICL_WORDKIND_2CONSTANT,
	FICL_WORDKIND_CREATE,
	FICL_WORDKIND_DO,
	FICL_WORDKIND_DOES,
	FICL_WORDKIND_LITERAL,
	FICL_WORDKIND_2LITERAL,
	FICL_WORDKIND_LOOP,
	FICL_WORDKIND_OF,
	FICL_WORDKIND_PLOOP,
	FICL_WORDKIND_PRIMITIVE,
	FICL_WORDKIND_QDO,
	FICL_WORDKIND_STRING_LITERAL,
	FICL_WORDKIND_CSTRING_LITERAL,
	FICL_WORDKIND_USER,
	FICL_WORDKIND_VARIABLE,
	FICL_WORDKIND_INSTRUCTION,
	FICL_WORDKIND_INSTRUCTION_WORD,
	FICL_WORDKIND_INSTRUCTION_WITH_ARGUMENT
}		ficlWordKind;

ficlWordKind ficlWordClassify(ficlWord *);

/*
** Used with File-Access wordset.
*/
#define FICL_FAM_READ		1
#define FICL_FAM_WRITE		2
#define FICL_FAM_APPEND		4
#define FICL_FAM_BINARY		8

#define FICL_FAM_OPEN_MODE(fam)						\
	((fam) & (FICL_FAM_READ | FICL_FAM_WRITE | FICL_FAM_APPEND))

#define FICL_MAXPATHLEN		1024

typedef struct {
	FILE           *f;
	char		filename[FICL_MAXPATHLEN];
} ficlFile;

int		ficlFileTruncate(ficlFile *, ficlUnsigned);
int		ficlFileStatus(char *, int *);
ficl2Integer	ficlFileSize(ficlFile *);

#define FICL_MIN(a, b)		(((a) < (b)) ? (a) : (b))
#define FICL_MAX(a, b)		(((a) > (b)) ? (a) : (b))

__END_DECLS

#endif /* __FICL_H__ */
