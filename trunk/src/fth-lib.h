/*-
 * Copyright (c) 2005-2014 Michael Scholz <mi-scholz@users.sourceforge.net>
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
 * @(#)fth-lib.h	1.131 1/22/14
 */

#if !defined(_FTH_LIB_H_)
#define _FTH_LIB_H_

#define FTH_VERSION		fth_short_version()

#define DEFAULT_SEQ_LENGTH	128
#define NEW_SEQ_LENGTH(Len) 						\
	((((Len) / DEFAULT_SEQ_LENGTH) + 1) * DEFAULT_SEQ_LENGTH)
/*
 * 1 cells 20 lshift
 * 1 cells 8 = (64bit addr): 0x800000
 * 1 cells 4 = (32bit addr): 0x400000
 */
#define MAX_SEQ_LENGTH		((ficlInteger)(FTH_SIZEOF_VOID_P << 20))

/* non-object names */
#define FTH_STR_EXCEPTION	"exception"
#define FTH_STR_KEYWORD		"keyword"
#define FTH_STR_OBJECT		"object"
#define FTH_STR_PORT		"port"
#define FTH_STR_PROC		"proc"
#define FTH_STR_SOCKET		"socket"
#define FTH_STR_SYMBOL		"symbol"
#define FTH_STR_WORD		"word"

/* C object-type names */
#define FTH_STR_ACELL		"acell"
#define FTH_STR_ARRAY		"array"
#define FTH_STR_BIGNUM		"bignum"
#define FTH_STR_BOOLEAN		"boolean"
#define FTH_STR_COMPLEX		"complex"
#define FTH_STR_FLOAT		"float"
#define FTH_STR_HASH		"hash"
#define FTH_STR_HOOK		"hook"
#define FTH_STR_IO		"io"
#define FTH_STR_LIST		"list"
#define FTH_STR_LLONG		"llong"
#define FTH_STR_NIL		"nil"
#define FTH_STR_RATIO		"ratio"
#define FTH_STR_REGEXP		"regexp"
#define FTH_STR_STRING		"string"

/* Predefined symbols. */
#define FTH_DOCUMENTATION_SYMBOL fth_symbol("documentation")
#define FTH_LAST_MESSAGE_SYMBOL	fth_symbol("last-message")
#define FTH_MESSAGE_SYMBOL	fth_symbol("message")
#define FTH_SOURCE_SYMBOL	fth_symbol("source")
#define FTH_TRACE_VAR_SYMBOL	fth_symbol("trace-var-hook")

/* Predefined keywords. */
#define FTH_KEYWORD_CLOSE	fth_keyword("close")
#define FTH_KEYWORD_COMMAND	fth_keyword("command")
#define FTH_KEYWORD_COUNT	fth_keyword("count")
#define FTH_KEYWORD_DOMAIN	fth_keyword("domain")
#define FTH_KEYWORD_FAM		fth_keyword("fam")
#define FTH_KEYWORD_FILENAME	fth_keyword("filename")
#define FTH_KEYWORD_FLUSH	fth_keyword("flush")
#define FTH_KEYWORD_IF_EXISTS	fth_keyword("if-exists")
#define FTH_KEYWORD_INIT	fth_keyword("initial-element")
#define FTH_KEYWORD_N		fth_keyword("n")
#define FTH_KEYWORD_PORT	fth_keyword("port")
#define FTH_KEYWORD_PORT_NAME	fth_keyword("port-name")
#define FTH_KEYWORD_RANGE	fth_keyword("range")
#define FTH_KEYWORD_READ_CHAR	fth_keyword("read-char")
#define FTH_KEYWORD_READ_LINE	fth_keyword("read-line")
#define FTH_KEYWORD_REPS	fth_keyword("reps")
#define FTH_KEYWORD_SOCKET	fth_keyword("socket")
#define FTH_KEYWORD_SOFT_PORT	fth_keyword("soft-port")
#define FTH_KEYWORD_START	fth_keyword("start")
#define FTH_KEYWORD_STRING	fth_keyword("string")
#define FTH_KEYWORD_WHENCE	fth_keyword("whence")
#define FTH_KEYWORD_WRITE_CHAR	fth_keyword("write-char")
#define FTH_KEYWORD_WRITE_LINE	fth_keyword("write-line")

/* Predefined exceptions. */
#define STR_BAD_ARITY		"bad-arity"
#define STR_BAD_SYNTAX		"bad-syntax"
#define STR_BIGNUM_ERROR	"bignum-error"
#define STR_CATCH_ERROR		"catch-error"
#define STR_EVAL_ERROR		"eval-error"
#define STR_FICL_ERROR		"ficl-error"
#define STR_FORTH_ERROR		"forth-error"
#define STR_LOAD_ERROR		"load-error"
#define STR_MATH_ERROR		"math-error"
#define STR_NO_MEMORY_ERROR	"no-memory-error"
#define STR_NULL_STRING		"null-string"
#define STR_OPTKEY_ERROR	"optkey-error"
#define STR_OUT_OF_RANGE	"out-of-range"
#define STR_REGEXP_ERROR	"regexp-error"
#define STR_SIGNAL_CAUGHT	"signal-caught"
#define STR_SOCKET_ERROR	"socket-error"
#define STR_SO_FILE_ERROR	"so-file-error"
#define STR_SYSTEM_ERROR	"system-error"
#define STR_WRONG_NUMBER_OF_ARGS "wrong-number-of-args"
#define STR_WRONG_TYPE_ARG	"wrong-type-arg"

#define FTH_BAD_ARITY		fth_exception(STR_BAD_ARITY)
#define FTH_BAD_SYNTAX		fth_exception(STR_BAD_SYNTAX)
#define FTH_BIGNUM_ERROR	fth_exception(STR_BIGNUM_ERROR)
#define FTH_CATCH_ERROR		fth_exception(STR_CATCH_ERROR)
#define FTH_EVAL_ERROR		fth_exception(STR_EVAL_ERROR)
#define FTH_FICL_ERROR		fth_exception(STR_FICL_ERROR)
#define FTH_FORTH_ERROR		fth_exception(STR_FORTH_ERROR)
#define FTH_LOAD_ERROR		fth_exception(STR_LOAD_ERROR)
#define FTH_MATH_ERROR		fth_exception(STR_MATH_ERROR)
#define FTH_NO_MEMORY_ERROR	fth_exception(STR_NO_MEMORY_ERROR)
#define FTH_NULL_STRING		fth_exception(STR_NULL_STRING)
#define FTH_OPTKEY_ERROR	fth_exception(STR_OPTKEY_ERROR)
#define FTH_OUT_OF_RANGE	fth_exception(STR_OUT_OF_RANGE)
#define FTH_REGEXP_ERROR	fth_exception(STR_REGEXP_ERROR)
#define FTH_SIGNAL_CAUGHT	fth_exception(STR_SIGNAL_CAUGHT)
#define FTH_SOCKET_ERROR	fth_exception(STR_SOCKET_ERROR)
#define FTH_SO_FILE_ERROR	fth_exception(STR_SO_FILE_ERROR)
#define FTH_SYSTEM_ERROR	fth_exception(STR_SYSTEM_ERROR)
#define FTH_WRONG_NUMBER_OF_ARGS fth_exception(STR_WRONG_NUMBER_OF_ARGS)
#define FTH_WRONG_TYPE_ARG	fth_exception(STR_WRONG_TYPE_ARG)

/* ANS Exception. */
#define __ANS_EXC(Exc) 							\
	fth_exception(ficl_ans_exc_name(FICL_VM_STATUS_ ## Exc))
#define FTH_ABORT		__ANS_EXC(ABORT)
#define FTH_ABORTQ		__ANS_EXC(ABORTQ)
#define FTH_ALIGNMENT_ERROR	__ANS_EXC(ALIGNMENT_ERROR)
#define FTH_ARGUMENT_ERROR	__ANS_EXC(ARGUMENT_ERROR)
#define FTH_BNUMBER_ERROR	__ANS_EXC(BNUMBER_ERROR)
#define FTH_BRANCH_ERROR	__ANS_EXC(BRANCH_ERROR)
#define FTH_BREAD_ERROR		__ANS_EXC(BREAD_ERROR)
#define FTH_BWRITE_ERROR	__ANS_EXC(BWRITE_ERROR)
#define FTH_CHAR_ERROR		__ANS_EXC(CHAR_ERROR)
#define FTH_COMPILER_NESTING	__ANS_EXC(COMPILER_NESTING)
#define FTH_COMPILE_ONLY	__ANS_EXC(COMPILE_ONLY)
#define FTH_CONTROL_MISMATCH	__ANS_EXC(CONTROL_MISMATCH)
#define FTH_CS_OVERFLOW		__ANS_EXC(CS_OVERFLOW)
#define FTH_DICT_OVERFLOW	__ANS_EXC(DICT_OVERFLOW)
#define FTH_DIVISION_BY_ZERO	__ANS_EXC(DIVISION_BY_ZERO)
#define FTH_EOF_ERROR		__ANS_EXC(EOF_ERROR)
#define FTH_ES_OVERFLOW		__ANS_EXC(ES_OVERFLOW)
#define FTH_FBASE_ERROR		__ANS_EXC(FBASE_ERROR)
#define FTH_FDIVIDE_BY_ZERO	__ANS_EXC(FDIVIDE_BY_ZERO)
#define FTH_FILE_IO_ERROR	__ANS_EXC(FILE_IO_ERROR)
#define FTH_FNUMBER_ERROR	__ANS_EXC(FNUMBER_ERROR)
#define FTH_FPOSITION_ERROR	__ANS_EXC(FPOSITION_ERROR)
#define FTH_FP_ERROR		__ANS_EXC(FP_ERROR)
#define FTH_FP_UNDERFLOW	__ANS_EXC(FP_UNDERFLOW)
#define FTH_FRANGE_ERROR	__ANS_EXC(FRANGE_ERROR)
#define FTH_FSTACK_OVERFLOW	__ANS_EXC(FSTACK_OVERFLOW)
#define FTH_FSTACK_UNDERFLOW	__ANS_EXC(FSTACK_UNDERFLOW)
#define FTH_INTERRUPT		__ANS_EXC(INTERRUPT)
#define FTH_INVALID_FORGET	__ANS_EXC(INVALID_FORGET)
#define FTH_MEMORY_ACCESS	__ANS_EXC(MEMORY_ACCESS)
#define FTH_MEMORY_WRITE_ERROR	__ANS_EXC(MEMORY_WRITE_ERROR)
#define FTH_MISSING_LPARAMETER	__ANS_EXC(MISSING_LPARAMETER)
#define FTH_NAME_ARG_ERROR	__ANS_EXC(NAME_ARG_ERROR)
#define FTH_NAME_TOO_LONG	__ANS_EXC(NAME_TOO_LONG)
#define FTH_NOT_IMPLEMENTED	__ANS_EXC(NOT_IMPLEMENTED)
#define FTH_NO_SUCH_FILE	__ANS_EXC(NO_SUCH_FILE)
#define FTH_NUMERIC_ARG_ERROR	__ANS_EXC(NUMERIC_ARG_ERROR)
#define FTH_OBSOLETE		__ANS_EXC(OBSOLETE)
#define FTH_PARSE_OVERFLOW	__ANS_EXC(PARSE_OVERFLOW)
#define FTH_PNO_OVERFLOW	__ANS_EXC(PNO_OVERFLOW)
#define FTH_POSTPONE_ERROR	__ANS_EXC(POSTPONE_ERROR)
#define FTH_PRECISION_ERROR	__ANS_EXC(PRECISION_ERROR)
#define FTH_QUIT		__ANS_EXC(QUIT)
#define FTH_RANGE_ERROR		__ANS_EXC(RANGE_ERROR)
#define FTH_RECURSION_ERROR	__ANS_EXC(RECURSION_ERROR)
#define FTH_RSTACK_IMBALANCE	__ANS_EXC(RSTACK_IMBALANCE)
#define FTH_RSTACK_OVERFLOW	__ANS_EXC(RSTACK_OVERFLOW)
#define FTH_RSTACK_UNDERFLOW	__ANS_EXC(RSTACK_UNDERFLOW)
#define FTH_SEARCH_OVERFLOW	__ANS_EXC(SEARCH_OVERFLOW)
#define FTH_SEARCH_UNDERFLOW	__ANS_EXC(SEARCH_UNDERFLOW)
#define FTH_STACK_OVERFLOW	__ANS_EXC(STACK_OVERFLOW)
#define FTH_STACK_UNDERFLOW	__ANS_EXC(STACK_UNDERFLOW)
#define FTH_TOO_DEEP		__ANS_EXC(TOO_DEEP)
#define FTH_TO_BODY_ERROR	__ANS_EXC(TO_BODY_ERROR)
#define FTH_UNDEFINED		__ANS_EXC(UNDEFINED)
#define FTH_WORD_LIST_CHANGED	__ANS_EXC(WORD_LIST_CHANGED)
#define FTH_WORD_LIST_ERROR	__ANS_EXC(WORD_LIST_ERROR)
#define FTH_ZERO_STRING		__ANS_EXC(ZERO_STRING)

/* Soft port prcs array indexes, globally required. */
enum {
	PORT_READ_CHAR,
	PORT_WRITE_CHAR,
	PORT_READ_LINE,
	PORT_WRITE_LINE,
	PORT_FLUSH,
	PORT_CLOSE,
	PORT_TYPE_LAST
};

#if !defined(EXIT_SUCCESS)
#define EXIT_SUCCESS	0
#endif
#if !defined(EXIT_FAILURE)
#define EXIT_FAILURE	1
#endif
#if !defined(BUFSIZ)
#define BUFSIZ		1024
#endif
#if !defined(MAXPATHLEN)
#define MAXPATHLEN	1024
#endif

#define EXIT_ABORT	2

#define FTH_MALLOC(N)		fth_malloc((size_t)(N))
#define FTH_REALLOC(P, N)	fth_realloc(P, N)
#define FTH_CALLOC(M, N)	fth_calloc((size_t)(M), (size_t)(N))
#define FTH_FREE(P)		fth_free(P)
#define FTH_STRDUP(S)		fth_strdup(S)

/* from snd/_sndlib.h */
#if (!defined(__NetBSD__) && (defined(_MSC_VER) || !defined(__STC__) || (defined(__STDC_VERSION__) && (__STDC_VERSION__ < 199901L))))
#define __func__		__FUNCTION__
#endif

/* from ruby/defines.h */
#if defined(__cplusplus)
#define ANYARGS 		...
#else
#define ANYARGS
#endif

#if defined(lint)
/* misc.c */
#define FTH_PROG_NAME		"fth"
#define FTH_PREFIX_PATH		"/usr/local"
#define FTH_LOCALEDIR		FTH_PREFIX_PATH "/share/locale"
#endif

#if defined(HAVE_FLOAT_H)
#include <float.h>
#endif
#if !defined(DBL_MANT_DIG)
#define DBL_MANT_DIG		53
#endif

#if HAVE_COMPLEX
#if defined(HAVE_MISSING_COMPLEX_H)
#include <missing_complex.h>
#endif
#if defined(HAVE_MISSING_MATH_H)
#include <missing_math.h>
#endif
/*
 * While NetBSD/OpenBSD/GNU libc do provide complex trigonometric
 * functions, others like FreeBSD/Minix don't (but FBSD's
 * ports/math/libmissing fills the gap).
 */

/* Trigonometric functions.  */

#if !defined(HAVE_CSIN)
ficlComplex	csin(ficlComplex z);
#endif
#if !defined(HAVE_CCOS)
ficlComplex	ccos(ficlComplex z);
#endif
#if !defined(HAVE_CTAN)
ficlComplex	ctan(ficlComplex z);
#endif
#if !defined(HAVE_CASIN)
ficlComplex	casin(ficlComplex z);
#endif
#if !defined(HAVE_CACOS)
ficlComplex	cacos(ficlComplex z);
#endif
#if !defined(HAVE_CATAN)
ficlComplex	catan(ficlComplex z);
#endif
#if !defined(HAVE_CATAN2)
ficlComplex	catan2(ficlComplex z, ficlComplex x);
#endif

/* Hyperbolic functions.  */

#if !defined(HAVE_CSINH)
ficlComplex	csinh(ficlComplex z);
#endif
#if !defined(HAVE_CCOSH)
ficlComplex	ccosh(ficlComplex z);
#endif
#if !defined(HAVE_CTANH)
ficlComplex	ctanh(ficlComplex z);
#endif
#if !defined(HAVE_CASINH)
ficlComplex	casinh(ficlComplex z);
#endif
#if !defined(HAVE_CACOSH)
ficlComplex	cacosh(ficlComplex z);
#endif
#if !defined(HAVE_CATANH)
ficlComplex	catanh(ficlComplex z);
#endif

/* Exponential and logarithmic functions.  */

#if !defined(HAVE_CEXP)
ficlComplex	cexp(ficlComplex z);
#endif
#if !defined(HAVE_CLOG)
ficlComplex	clog(ficlComplex z);
#endif
#if !defined(HAVE_CLOG10)
ficlComplex	clog10(ficlComplex z);
#endif

/* Power functions.  */

#if !defined(HAVE_CPOW)
ficlComplex	cpow(ficlComplex x, ficlComplex y);
#endif
#if !defined(HAVE_CSQRT)
ficlComplex	csqrt(ficlComplex z);
#endif

/* Absolute value and conjugates.  */

#if !defined(HAVE_CABS)
ficlFloat	cabs  (ficlComplex z);
#endif
#if !defined(HAVE_CABS2)
ficlFloat	cabs2 (ficlComplex z);
#endif
#if !defined(HAVE_CARG)
ficlFloat	carg  (ficlComplex z);
#endif
#if !defined(HAVE_CONJ)
ficlComplex	conj(ficlComplex z);
#endif
#endif				/* HAVE_COMPLEX */

/* === Object === */

/* C object-type numbers */
typedef enum {
	FTH_ARRAY_T,
	FTH_BOOLEAN_T,
	FTH_HASH_T,
	FTH_HOOK_T,
	FTH_IO_T,
	FTH_NIL_T,
	FTH_REGEXP_T,
	FTH_STRING_T,
	/* number types */
	FTH_LLONG_T,
	FTH_FLOAT_T,
	FTH_COMPLEX_T,
	FTH_BIGNUM_T,
	FTH_RATIO_T,
	FTH_LAST_ENTRY_T
} fobj_t;

typedef struct {
	fobj_t		type;	/* uniq object-type number (0, 1, ...) */
	int		flag;	/* number types */
	char		name[32];	/* object-type name */
	/* methods for C object-types */
	FTH             (*inspect)(FTH self);
	FTH             (*to_string)(FTH self);
	FTH             (*dump)(FTH self);
	FTH             (*to_array)(FTH self);
	FTH             (*copy)(FTH self);
	FTH             (*value_ref)(FTH self, FTH index);
	FTH             (*value_set)(FTH self, FTH index, FTH value);
	FTH             (*equal_p)(FTH self, FTH obj);
	FTH             (*length)(FTH self);
	void            (*mark)(FTH self);
	void            (*free)(FTH self);
	/* procs for Forth object-types */
	FTH		inspect_proc;
	FTH		to_string_proc;
	FTH		dump_proc;
	FTH		to_array_proc;
	FTH		copy_proc;
	FTH		value_ref_proc;
	FTH		value_set_proc;
	FTH		equal_p_proc;
	FTH		length_proc;
	FTH		mark_proc;
	FTH		free_proc;
	FTH		apply;	/* proc object */
} FObject;

#define FTH_OBJECT_REF(Obj)	((FObject *)(Obj))
#define FTH_OBJECT_NAME(Obj)	((char *)(FTH_OBJECT_REF(Obj)->name))
#define FTH_OBJECT_TYPE(Obj)	FTH_OBJECT_REF(Obj)->type
#define FTH_OBJECT_FLAG(Obj)	FTH_OBJECT_REF(Obj)->flag

/* === Instance === */
typedef enum {
	INT_T,
	UINT_T,
	LONG_T,
	ULONG_T,
	FLOAT_T,
	COMPLEX_T,
	BIGNUM_T,
	RATIO_T,
	FTH_T,
	VOIDP_T
} instance_t;

typedef struct FInstance {
	instance_t	type;
	int		gc_mark;
	struct FInstance *next;
	void           *gen;
	FObject        *obj;
	FTH		properties;
	FTH		values;
	FTH		debug_hook;	/* ( inspect-string obj -- str ) */
	ficlInteger	cycle;
	bool		changed_p;
	bool		extern_p;
	union {
		ficlInteger	i;
		ficlUnsigned	u;
		ficl2Integer	di;
		ficl2Unsigned	ud;
		ficlFloat	f;
#if HAVE_COMPLEX
		ficlComplex	cp;
#endif
#if HAVE_BN
		ficlBignum	bi;
		ficlRatio	rt;
#endif
		FTH		fp;
		void           *p;
	}		fcell;
} FInstance;

#define FTH_INSTANCE_REF(Obj)		((FInstance *)(Obj))

#define FTH_INSTANCE_CELL_TYPE(Obj)	FTH_INSTANCE_REF(Obj)->type
#define FTH_INSTANCE_CELL_TYPE_SET(Obj, Type)				\
	(FTH_INSTANCE_CELL_TYPE(Obj) = (instance_t)(Type))

#define FTH_INT_OBJECT(Obj)		FTH_INSTANCE_REF(Obj)->fcell.i
#define FTH_UINT_OBJECT(Obj)		FTH_INSTANCE_REF(Obj)->fcell.u
#define FTH_LONG_OBJECT(Obj)		FTH_INSTANCE_REF(Obj)->fcell.di
#define FTH_ULONG_OBJECT(Obj)		FTH_INSTANCE_REF(Obj)->fcell.ud
#define FTH_FLOAT_OBJECT(Obj)		FTH_INSTANCE_REF(Obj)->fcell.f
#if HAVE_COMPLEX
#define FTH_COMPLEX_OBJECT(Obj)		FTH_INSTANCE_REF(Obj)->fcell.cp
#endif
#if HAVE_BN
#define FTH_BIGNUM_OBJECT(Obj)		FTH_INSTANCE_REF(Obj)->fcell.bi
#define FTH_RATIO_OBJECT(Obj)		FTH_INSTANCE_REF(Obj)->fcell.rt
#endif
#define FTH_FTH_OBJECT(Obj)		FTH_INSTANCE_REF(Obj)->fcell.fp
#define FTH_VOIDP_OBJECT(Obj)		FTH_INSTANCE_REF(Obj)->fcell.p

#define FTH_INT_OBJECT_SET(Obj, Val)					\
	FTH_INSTANCE_CELL_TYPE_SET(Obj, INT_T);				\
	(FTH_INT_OBJECT(Obj) = (ficlInteger)(Val))
#define FTH_UINT_OBJECT_SET(Obj, Val)					\
	FTH_INSTANCE_CELL_TYPE_SET(Obj, UINT_T);			\
	(FTH_UINT_OBJECT(Obj) = (ficlUnsigned)(Val))
#define FTH_LONG_OBJECT_SET(Obj, Val)					\
	FTH_INSTANCE_CELL_TYPE_SET(Obj, LONG_T);			\
	(FTH_LONG_OBJECT(Obj) = (ficl2Integer)(Val))
#define FTH_ULONG_OBJECT_SET(Obj, Val)					\
	FTH_INSTANCE_CELL_TYPE_SET(Obj, ULONG_T);			\
	(FTH_ULONG_OBJECT(Obj) = (ficl2Unsigned)(Val))
#define FTH_FLOAT_OBJECT_SET(Obj, Val)					\
	FTH_INSTANCE_CELL_TYPE_SET(Obj, FLOAT_T);			\
	(FTH_FLOAT_OBJECT(Obj) = (ficlFloat)(Val))
#if HAVE_COMPLEX
#define FTH_COMPLEX_OBJECT_SET(Obj, Val)				\
	FTH_INSTANCE_CELL_TYPE_SET(Obj, COMPLEX_T);			\
	(FTH_COMPLEX_OBJECT(Obj) = (ficlComplex)(Val))
#endif
#if HAVE_BN
#define FTH_BIGNUM_OBJECT_SET(Obj, Val)					\
	FTH_INSTANCE_CELL_TYPE_SET(Obj, BIGNUM_T);			\
	(FTH_BIGNUM_OBJECT(Obj) = (ficlBignum)(Val))
#define FTH_RATIO_OBJECT_SET(Obj, Val)					\
	FTH_INSTANCE_CELL_TYPE_SET(Obj, RATIO_T);			\
	(FTH_RATIO_OBJECT(Obj) = (ficlRatio)(Val))
#endif
#define FTH_FTH_OBJECT_SET(Obj, Val)					\
	FTH_INSTANCE_CELL_TYPE_SET(Obj, FTH_T);				\
	(FTH_FTH_OBJECT(Obj) = (FTH)(Val))
#define FTH_VOIDP_OBJECT_SET(Obj, Val)					\
	FTH_INSTANCE_CELL_TYPE_SET(Obj, VOIDP_T);			\
	(FTH_VOIDP_OBJECT(Obj) = (void *)(Val))

#define FTH_INSTANCE_REF_GEN(Obj, Type)	((Type *)(FTH_INSTANCE_REF(Obj)->gen))
#define FTH_INSTANCE_REF_OBJ(Obj)					\
	FTH_OBJECT_REF(FTH_INSTANCE_REF(Obj)->obj)
#define FTH_INSTANCE_TYPE(Obj)						\
	FTH_OBJECT_TYPE(FTH_INSTANCE_REF_OBJ(Obj))
#define FTH_INSTANCE_NAME(Obj)						\
	FTH_OBJECT_NAME(FTH_INSTANCE_REF_OBJ(Obj))
#define FTH_INSTANCE_FLAG(Obj)						\
	FTH_OBJECT_FLAG(FTH_INSTANCE_REF_OBJ(Obj))
#define FTH_INSTANCE_PROPERTIES(Obj)	FTH_INSTANCE_REF(Obj)->properties
#define FTH_INSTANCE_DEBUG_HOOK(Obj)	FTH_INSTANCE_REF(Obj)->debug_hook
#define FTH_INSTANCE_CHANGED_P(Obj)	FTH_INSTANCE_REF(Obj)->changed_p
#define FTH_INSTANCE_CHANGED(Obj)					\
	(FTH_INSTANCE_REF(Obj)->changed_p = true)
#define FTH_INSTANCE_CHANGED_CLR(Obj)					\
	(FTH_INSTANCE_REF(Obj)->changed_p = false)

/* === Word === */
#define FICL_WORD_NAME_REF(Name)					\
	ficlSystemLookup(FTH_FICL_SYSTEM(), (char *)(Name))
#define FICL_NAME_DEFINED_P(Name)	(FICL_WORD_NAME_REF(Name) != NULL)
#define FICL_WORD_REF(Obj)		((ficlWord *)(Obj))
#define FICL_WORD_TYPE(Obj)		FICL_WORD_REF(Obj)->kind
#define FICL_WORD_PRIMITIVE_P(Obj)	FICL_WORD_REF(Obj)->primitive_p
#define FICL_WORD_NAME(Obj)		FICL_WORD_REF(Obj)->name
#define FICL_WORD_LENGTH(Obj)		FICL_WORD_REF(Obj)->argc
#define FICL_WORD_PROPERTIES(Obj)	FICL_WORD_REF(Obj)->properties
#define FICL_WORD_REQ(Obj)		FICL_WORD_REF(Obj)->req
#define FICL_WORD_OPT(Obj)		FICL_WORD_REF(Obj)->opt
#define FICL_WORD_REST(Obj)		((bool)FICL_WORD_REF(Obj)->rest)
#define FICL_WORD_FUNC(Obj)		FICL_WORD_REF(Obj)->func
#define FICL_WORD_VFUNC(Obj)		FICL_WORD_REF(Obj)->vfunc
#define FICL_WORD_CODE(Obj)		FICL_WORD_REF(Obj)->code
#define FICL_WORD_PARAM(Obj)		CELL_FTH_REF(FICL_WORD_REF(Obj)->param)

/* return FTH string and FTH int */
#define FTH_WORD_NAME(Obj)						\
	fth_make_string_or_false(FICL_WORD_NAME(Obj))

#define FTH_WORD_PARAM(Obj)						\
	ficl_to_fth(FICL_WORD_PARAM(Obj))

#define FTH_STACK_CHECK(Vm, Pop, Push) do {				\
	ficlInteger _depth;						\
	ficlInteger _req;						\
	ficlStack *_stack;						\
									\
	_stack = (Vm)->dataStack;					\
	_req = (ficlInteger)(Pop);					\
	_depth = (_stack->top - _stack->base) + 1;			\
	if (_req > _depth)						\
		fth_throw(FTH_WRONG_NUMBER_OF_ARGS,			\
		    "%s: not enough arguments, %ld instead of %ld",	\
		    RUNNING_WORD_VM(Vm),				\
		    _depth,						\
		    _req);						\
} while (0)

#define FTH_STACK_DEPTH(Vm)						\
	(((Vm)->dataStack->top - (Vm)->dataStack->base) + 1)

#define RUNNING_WORD_VM(Vm)						\
	(((Vm)->runningWord && (Vm)->runningWord->length > 0) ?		\
	    (Vm)->runningWord->name :					\
	    (char *)__func__)
#define RUNNING_WORD()			RUNNING_WORD_VM(FTH_FICL_VM())

#define FTH_ADD_FEATURE_AND_INFO(Name, Docs)				\
	fth_add_feature(Name);						\
	fth_word_doc_set((ficlWord *)fth_symbol(Name), Docs "\n\
Other topics include:\n\
array               list                file\n\
hash                hook                io\n\
off-t               float               complex\n\
ratio               bignum              object\n\
port                proc                regexp\n\
string              symbol              keyword\n\
exception")

#define FTH_CONSTANT_SET(Name, Value)					\
	ficlDictionaryAppendConstant(FTH_FICL_DICT(),			\
	    (char *)(Name), (ficlInteger)(Value))

#define FTH_CONSTANT_SET_WITH_DOC(Name, Value, Docs)			\
	fth_word_doc_set(FTH_CONSTANT_SET(Name, Value), Docs)

#define FTH_PRIMITIVE_SET(Name, Code, Type, Docs)			\
	fth_word_doc_set(ficlDictionaryAppendPrimitive(FTH_FICL_DICT(),	\
	    (char *)(Name), Code, (ficlUnsigned)(Type)), Docs)

#define FTH_PRI1(Name, Code, Docs)					\
	FTH_PRIMITIVE_SET(Name, Code, FICL_WORD_DEFAULT, Docs)

#define FTH_PRIM_IM(Name, Code, Docs)					\
	FTH_PRIMITIVE_SET(Name, Code, FICL_WORD_IMMEDIATE, Docs)

#define FTH_PRIM_CO(Name, Code, Docs)					\
FTH_PRIMITIVE_SET(Name, Code, FICL_WORD_COMPILE_ONLY, Docs)
#define FTH_PRIM_CO_IM(Name, Code, Docs)				\
	FTH_PRIMITIVE_SET(Name, Code, FICL_WORD_COMPILE_ONLY_IMMEDIATE, Docs)

#define FTH_PROC(Name, Code, Req, Opt, Rest, Docs)			\
	fth_define_procedure(Name, Code, Req, Opt, Rest, Docs)

#define FTH_VOID_PROC(Name, Code, Req, Opt, Rest, Docs)			\
	fth_define_void_procedure(Name, Code, Req, Opt, Rest, Docs)

#define fth_show(Obj)							\
	fprintf(stderr, "#<SHOW %s[%d]: %s>\n", __func__, __LINE__,	\
	    fth_to_c_inspect(Obj))

/*
 * Old names partly required elsewhere.
 */
#define FTH_PRIM(Dict, Name, Code, Docs)				\
	fth_word_doc_set(ficlDictionaryAppendPrimitive(Dict,		\
	    Name,							\
	    Code,							\
	    FICL_WORD_DEFAULT), Docs)
#define ficlStackPop2Float(Stack)	ficlStackPopFloat(Stack)
#define fth_false()			FTH_FALSE
#define fth_hook_procedure_list(Obj)	fth_hook_to_array(Obj)
#define fth_make_off_t(Obj)		fth_make_llong(Obj)
#define fth_make_uoff_t(Obj)		fth_make_ullong(Obj)
#define fth_obj_id(Obj)			fth_object_id(Obj)
#define fth_off_t_copy(Obj)		fth_llong_copy(Obj)
#define fth_set_object_equal(Obj, Func)	fth_set_object_equal_p(Obj, Func)
#define fth_uoff_t_p(Obj)		fth_ullong_p(Obj)

#endif				/* _FTH_LIB_H_ */

/*
 * fth-lib.h ends here
 */
