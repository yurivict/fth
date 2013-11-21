/*-
 * Copyright (c) 2005-2013 Michael Scholz <mi-scholz@users.sourceforge.net>
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
 * @(#)fth.h	1.210 11/20/13
 */

#if !defined(_FTH_H_)
#define _FTH_H_

#include "fth-config.h"

#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <stddef.h>
#if defined(HAVE_STRING_H)
#include <string.h>
#endif
#if defined(HAVE_MEMORY_H)
#include <memory.h>
#endif
#if defined(HAVE_STRINGS_H)
#include <strings.h>
#endif
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#if defined(HAVE_ERRNO_H)
#include <errno.h>
#endif
#if defined(HAVE_SETJMP_H)
#include <setjmp.h>
#endif
#if defined(HAVE_STDARG_H)
#include <stdarg.h>
#endif

#include "ficllocal.h"
#include "ficl.h"

#undef __BEGIN_DECLS
#undef __END_DECLS
#if defined(__cplusplus)
#define __BEGIN_DECLS		extern "C" {
#define __END_DECLS		}
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif

#include "fth-lib.h"

/* from ruby/ruby.h */
#if defined(__STDC__)
#include <limits.h>
#else				/* !__STDC__ */
#if !defined(LONG_MAX)
#if defined(HAVE_LIMITS_H)
#include <limits.h>
#else
#if (FTH_SIZEOF_LONG == 4)
/* assuming 32bit (2's compliment) long */
#define LONG_MAX		0x7fffffffL
#else
#define LONG_MAX		0x7fffffffffffffffL
#endif
#endif
#endif				/* !LONG_MAX */
#if !defined(LONG_MIN)
#define LONG_MIN		(-LONG_MAX - 1)
#endif
#if !defined(ULONG_MAX)
#if (FTH_SIZEOF_LONG == 4)
#define ULONG_MAX		0xffffffffUL
#else
#define ULONG_MAX		0x7fffffffffffffffUL
#endif
#endif				/* !ULONG_MAX */
#endif				/* __STDC__ */

#if defined(HAVE_LONG_LONG)
#if !defined(LLONG_MAX)
#if defined(LONG_LONG_MAX)
#define LLONG_MAX		LONG_LONG_MAX
#else
#if defined(_I64_MAX)
#define LLONG_MAX		_I64_MAX
#else
/* assuming 64bit (2's complement) long long */
#define LLONG_MAX		0x7fffffffffffffffLL
#endif				/* _I64_MAX */
#endif				/* LONG_LONG_MAX */
#endif				/* !LLONG_MAX */
#if !defined(LLONG_MIN)
#if defined(LONG_LONG_MIN)
#define LLONG_MIN		LONG_LONG_MIN
#else
#if defined(_I64_MIN)
#define LLONG_MIN		_I64_MIN
#else
#define LLONG_MIN		(-LLONG_MAX - 1)
#endif				/* _I64_MIN */
#endif				/* LONG_LONG_MIN */
#endif				/* !LLONG_MIN */
#if !defined(ULLONG_MAX)
#define ULLONG_MAX		0xffffffffffffffffULL
#endif
#endif				/* HAVE_LONG_LONG */

#if (FTH_SIZEOF_LONG_LONG == FTH_SIZEOF_VOID_P)
#define FIXNUM_MAX		(LLONG_MAX >> 1L)
#define FIXNUM_MIN		(((SIGNED_FTH)(LLONG_MIN)) >> 1L)
#define FIXNUM_UMAX		(ULLONG_MAX >> 1L)
#else
#define FIXNUM_MAX		(LONG_MAX >> 1L)
#define FIXNUM_MIN		(((SIGNED_FTH)(LONG_MIN)) >> 1L)
#define FIXNUM_UMAX		(ULONG_MAX >> 1L)
#endif

#define POSFIXABLE_P(Obj)	((Obj) <= FIXNUM_MAX)
#define NEGFIXABLE_P(Obj)	((Obj) >= FIXNUM_MIN)
#define FIXABLE_P(Obj)		(POSFIXABLE_P(Obj) && NEGFIXABLE_P(Obj))
#define UFIXABLE_P(Obj)		((Obj) <= FIXNUM_UMAX)

#define FIXNUM_FLAG		0x01
#define FIXNUM_P(Obj)		(((SIGNED_FTH)(Obj)) & FIXNUM_FLAG)

#define INT_TO_FIX(Obj)		((FTH)(((SIGNED_FTH)(Obj)) << 1L | FIXNUM_FLAG))
#define FIX_TO_INT(Obj)		(((SIGNED_FTH)(Obj)) >> 1L)
#define FIX_TO_INT32(Obj)	(int)FIX_TO_INT(Obj)
#define UNSIGNED_TO_FIX(Obj)	((FTH)(((FTH)(Obj)) << 1UL | FIXNUM_FLAG))
#define FIX_TO_UNSIGNED(Obj)	(((FTH)(Obj)) >> 1UL)

#define IMMEDIATE_MASK		0x03
#define IMMEDIATE_P(Obj)	((FTH)(Obj) & IMMEDIATE_MASK)

#define CHAR_TO_FTH(Obj)	INT_TO_FIX(Obj)
#define FTH_TO_CHAR(Obj)	((int)FIX_TO_INT(Obj))

#if (FTH_SIZEOF_LONG == 4)
#define FTH_INT_REF(Obj)	fth_integer_ref(Obj)
#else
#define FTH_INT_REF(Obj)	FIX_TO_INT(Obj)
#endif

#define FTH_ARGn 		0L
#define FTH_ARG1 		1L
#define FTH_ARG2 		2L
#define FTH_ARG3 		3L
#define FTH_ARG4 		4L
#define FTH_ARG5 		5L
#define FTH_ARG6 		6L
#define FTH_ARG7 		7L
#define FTH_ARG8 		8L
#define FTH_ARG9 		9L

#define FTH_ONE_NEG		INT_TO_FIX(-1)
#define FTH_ZERO		INT_TO_FIX(0)
#define FTH_ONE			INT_TO_FIX(1)
#define FTH_TWO			INT_TO_FIX(2)
#define FTH_THREE		INT_TO_FIX(3)
#define FTH_FOUR		INT_TO_FIX(4)
#define FTH_FIVE		INT_TO_FIX(5)
#define FTH_SIX			INT_TO_FIX(6)
#define FTH_SEVEN		INT_TO_FIX(7)
#define FTH_EIGHT		INT_TO_FIX(8)
#define FTH_NINE		INT_TO_FIX(9)

#define FTH_FILE_EXTENSION	"fs"

enum {
	FTH_UNKNOWN,
	FTH_OKAY,
	FTH_BYE,
	FTH_ERROR
};

typedef struct {
	ficlSystem     *system;
	ficlVm         *vm;
	FTH		current_file;
	ficlInteger	current_line;
	int		print_length;
	FTH		last_exception;
	FTH		_false;
	FTH		_true;
	FTH		_nil;
	FTH		_undef;
	bool		print_p;
	bool		eval_p;
	bool		hit_error_p;
	bool		true_repl_p;
	bool		die_on_signal_p;
	bool		interactive_p;
} Ficl;

/* defined in misc.c */
extern Ficl    *fth_ficl;

#define FTH_FICL_VAR()		fth_ficl
#define FTH_FICL_SYSTEM()	FTH_FICL_VAR()->system
#define FTH_FICL_VM()		FTH_FICL_VAR()->vm
#define FTH_FICL_STACK()	FTH_FICL_VAR()->vm->dataStack
#define FTH_FICL_DICT()		FTH_FICL_VAR()->system->dictionary
#define FTH_FICL_ENV()		FTH_FICL_VAR()->system->environment

#define fth_current_file	FTH_FICL_VAR()->current_file
#define fth_current_line	FTH_FICL_VAR()->current_line
#define fth_print_length	FTH_FICL_VAR()->print_length

#define fth_last_exception	FTH_FICL_VAR()->last_exception

#define FTH_FALSE		FTH_FICL_VAR()->_false
#define FTH_TRUE		FTH_FICL_VAR()->_true
#define FTH_NIL			FTH_FICL_VAR()->_nil
#define FTH_UNDEF		FTH_FICL_VAR()->_undef

#define fth_print_p		FTH_FICL_VAR()->print_p
#define fth_eval_p		FTH_FICL_VAR()->eval_p
#define fth_hit_error_p		FTH_FICL_VAR()->hit_error_p
#define fth_true_repl_p		FTH_FICL_VAR()->true_repl_p
#define fth_die_on_signal_p	FTH_FICL_VAR()->die_on_signal_p
#define fth_interactive_p	FTH_FICL_VAR()->interactive_p

typedef char   *(*in_cb) (ficlVm *);
typedef void    (*out_cb) (ficlVm *, char *);
typedef void    (*exit_cb) (int);
extern in_cb	fth_read_hook;
extern out_cb	fth_print_hook;
extern out_cb	fth_error_hook;
extern exit_cb	fth_exit_hook;

/* === Predicates === */
#define FTH_INSTANCE_FLAG_P(Obj, Type)					\
	fth_instance_flag_p(Obj, Type)
#define FTH_INSTANCE_TYPE_P(Obj, Type)					\
	fth_instance_type_p(Obj, Type)

#define FTH_CHAR_P(Obj)		fth_char_p(Obj)
#define FTH_EXACT_P(Obj)	fth_exact_p(Obj)
#define FTH_FIXNUM_P(Obj)	fth_fixnum_p(Obj)
#define FTH_INTEGER_P(Obj)	fth_integer_p(Obj)
#define FTH_NUMBER_P(Obj)	fth_number_p(Obj)
#define FTH_UNSIGNED_P(Obj)	fth_unsigned_p(Obj)
#define FTH_ULLONG_P(Obj)	fth_ullong_p(Obj)

#define FTH_ARRAY_P(Obj)	FTH_INSTANCE_TYPE_P(Obj, FTH_ARRAY_T)
#define FTH_HASH_P(Obj)		FTH_INSTANCE_TYPE_P(Obj, FTH_HASH_T)
#define FTH_HOOK_P(Obj)		FTH_INSTANCE_TYPE_P(Obj, FTH_HOOK_T)
#define FTH_IO_P(Obj)		FTH_INSTANCE_TYPE_P(Obj, FTH_IO_T)
#define FTH_REGEXP_P(Obj)	FTH_INSTANCE_TYPE_P(Obj, FTH_REGEXP_T)
#define FTH_STRING_P(Obj)	FTH_INSTANCE_TYPE_P(Obj, FTH_STRING_T)

#define FTH_ASSOC_P(Obj)	FTH_ARRAY_P(Obj)
#define FTH_CONS_P(Obj)		FTH_ARRAY_P(Obj)
#define FTH_LIST_P(Obj)		(FTH_NIL_P(Obj) || FTH_CONS_P(Obj))
#define FTH_PAIR_P(Obj)		FTH_CONS_P(Obj)

#define FTH_BIGNUM_P(Obj)	FTH_INSTANCE_TYPE_P(Obj, FTH_BIGNUM_T)
#define FTH_COMPLEX_P(Obj)	FTH_INSTANCE_TYPE_P(Obj, FTH_COMPLEX_T)
#define FTH_FLOAT_P(Obj)	FTH_INSTANCE_TYPE_P(Obj, FTH_FLOAT_T)
#define FTH_LONG_LONG_P(Obj)	FTH_INTEGER_P(Obj)
#define FTH_LLONG_P(Obj)	FTH_INSTANCE_TYPE_P(Obj, FTH_LLONG_T)
#define FTH_RATIO_P(Obj)	FTH_INSTANCE_TYPE_P(Obj, FTH_RATIO_T)

#define N_NUMBER_T		0x01
#define N_EXACT_T		0x02
#define N_INEXACT_T		0x04

#define FTH_EXACT_T_P(Obj)	FTH_INSTANCE_FLAG_P(Obj, N_EXACT_T)
#define FTH_INEXACT_P(Obj)	FTH_INSTANCE_FLAG_P(Obj, N_INEXACT_T)
#define FTH_NUMBER_T_P(Obj)	FTH_INSTANCE_FLAG_P(Obj, N_NUMBER_T)

/* === Word === */

#define FICL_WORD_DICT_P(Obj)						\
	(FICL_WORD_REF(Obj) >= FICL_WORD_REF(FTH_FICL_DICT()->base) &&	\
	FICL_WORD_REF(Obj) <						\
	(FICL_WORD_REF(FTH_FICL_DICT()->base + FTH_FICL_DICT()->size)))
#define FICL_WORD_DEFINED_P(Obj)					\
	((Obj) && FICL_WORD_DICT_P(Obj))

#define FICL_WORD_TYPE_P(Obj, Type)					\
	(FICL_WORD_DEFINED_P(Obj) && FICL_WORD_TYPE(Obj) == (Type))

enum {
	FW_WORD,
	FW_PROC,
	FW_SYMBOL,
	FW_KEYWORD,
	FW_EXCEPTION,
	FW_VARIABLE,
	FW_TRACE_VAR
};

#define FICL_INSTRUCTION_P(Obj)						\
	(((ficlInstruction)(Obj)) > ficlInstructionInvalid &&		\
	 ((ficlInstruction)(Obj)) < ficlInstructionLast)

#define FICL_WORD_P(Obj)						\
	(FICL_WORD_DEFINED_P(Obj) &&					\
	 ((FICL_WORD_TYPE(Obj) == FW_WORD) ||				\
	  (FICL_WORD_TYPE(Obj) == FW_PROC)))

#define FICL_SYMBOLS_P(Obj)						\
	(FICL_WORD_DEFINED_P(Obj) &&					\
	 ((FICL_WORD_TYPE(Obj) == FW_SYMBOL) ||				\
	  (FICL_WORD_TYPE(Obj) == FW_KEYWORD) ||			\
	  (FICL_WORD_TYPE(Obj) == FW_EXCEPTION)))

#define FICL_VARIABLES_P(Obj)						\
	(FICL_WORD_DEFINED_P(Obj) &&					\
	 ((FICL_WORD_TYPE(Obj) == FW_VARIABLE) ||			\
	  (FICL_WORD_TYPE(Obj) == FW_TRACE_VAR)))

#define FTH_EXCEPTION_P(Obj)	FICL_WORD_TYPE_P(Obj, FW_EXCEPTION)
#define FTH_KEYWORD_P(Obj)	FICL_WORD_TYPE_P(Obj, FW_KEYWORD)
#define FTH_PROC_P(Obj)		FICL_WORD_TYPE_P(Obj, FW_PROC)
#define FTH_SYMBOL_P(Obj)	FICL_WORD_TYPE_P(Obj, FW_SYMBOL)
#define FTH_TRACE_VAR_P(Obj)	FICL_WORD_TYPE_P(Obj, FW_TRACE_VAR)
#define FTH_VARIABLE_P(Obj)	FICL_WORD_TYPE_P(Obj, FW_VARIABLE)
#define FTH_WORD_P(Obj)		FICL_WORD_TYPE_P(Obj, FW_WORD)

/* === Boolean === */
#define FTH_BOOLEAN_P(Obj)	FTH_INSTANCE_TYPE_P(Obj, FTH_BOOLEAN_T)
#define FTH_NIL_TYPE_P(Obj)	FTH_INSTANCE_TYPE_P(Obj, FTH_NIL_T)
#define FTH_BOUND_P(Obj)	((Obj) != FTH_UNDEF)
#define FTH_FALSE_P(Obj)	((Obj) == FTH_FALSE)
#define FTH_NIL_P(Obj)		((Obj) == FTH_NIL)
#define FTH_TRUE_P(Obj)		((Obj) == FTH_TRUE)
#define FTH_UNDEF_P(Obj)	((Obj) == FTH_UNDEF)
#define FTH_NOT_BOUND_P(Obj)	((Obj) == FTH_UNDEF)
#define FTH_NOT_FALSE_P(Obj)	((Obj) != FTH_FALSE)
#define FTH_NOT_NIL_P(Obj)	((Obj) != FTH_NIL)
#define FTH_NOT_TRUE_P(Obj)	((Obj) != FTH_TRUE)
#define BOOL_TO_FTH(Obj)	((FTH)((Obj) ? FTH_TRUE : FTH_FALSE))
#define FTH_TO_BOOL(Obj)	((Obj) != FTH_FALSE)

#define FTH_CADR(Obj)		fth_cadr(Obj)
#define FTH_CADDR(Obj)		fth_caddr(Obj)
#define FTH_CADDDR(Obj)		fth_cadddr(Obj)
#define FTH_CDDR(Obj)		fth_cddr(Obj)

#define FTH_LIST_1(A)		fth_make_list_var(1, A)
#define FTH_LIST_2(A, B)	fth_make_list_var(2, A, B)
#define FTH_LIST_3(A, B, C)	fth_make_list_var(3, A, B, C)
#define FTH_LIST_4(A, B, C, D)	fth_make_list_var(4, A, B, C, D)
#define FTH_LIST_5(A, B, C, D, E)					\
	fth_make_list_var(5, A, B, C, D, E)
#define FTH_LIST_6(A, B, C, D, E, F)					\
	fth_make_list_var(6, A, B, C, D, E, F)
#define FTH_LIST_7(A, B, C, D, E, F, G)					\
	fth_make_list_var(7, A, B, C, D, E, F, G)
#define FTH_LIST_8(A, B, C, D, E, F, G, H)				\
	fth_make_list_var(8, A, B, C, D, E, F, G, H)
#define FTH_LIST_9(A, B, C, D, E, F, G, H, I)				\
	fth_make_list_var(9, A, B, C, D, E, F, G, H, I)

#define FTH_ASSERT_STRING(Obj)						\
	if ((Obj) == 0)							\
		fth_throw(FTH_NULL_STRING,				\
		    "%s (%s): got null string",				\
		    RUNNING_WORD(),					\
		    c__FUNCTION__)

#define FTH_WRONG_TYPE_ARG_ERROR(Caller, Pos, Arg, Desc)		\
	fth_throw(FTH_WRONG_TYPE_ARG,					\
	    "%s (%s): wrong type arg %d, %s (%S), wanted %s",		\
	    (Caller),							\
	    c__FUNCTION__,						\
	    (Pos),							\
	    fth_object_name(Arg),					\
	    (Arg),							\
	    (Desc))

#define FTH_ASSERT_TYPE(Assertion, Arg, Pos, Caller, Desc)		\
	if ((Assertion) == 0)						\
		FTH_WRONG_TYPE_ARG_ERROR(Caller, Pos, Arg, Desc)

#define FTH_ASSERT_ARGS(Assertion, Arg, Pos, Desc)			\
	FTH_ASSERT_TYPE(Assertion, Arg, Pos, RUNNING_WORD(), Desc)

#define FTH_OUT_OF_BOUNDS_ERROR(Pos, Arg, Desc)				\
	fth_throw(FTH_OUT_OF_RANGE,					\
	    "%s (%s) arg %d: %ld is %s",				\
	    RUNNING_WORD(),						\
	    c__FUNCTION__,						\
	    (int)(Pos),							\
	    (ficlInteger)(Arg),						\
	    (Desc))

#define FTH_OUT_OF_BOUNDS(Pos, Arg)					\
	FTH_OUT_OF_BOUNDS_ERROR(Pos, Arg, "out of range")

/*
 * This is special for Snd:
 *	(char *Caller, int Pos, FTH Arg, char *Desc)
 */
#define FTH_OUT_OF_RANGE_ERROR(Caller, Pos, Arg, Desc)			\
	fth_throw(FTH_OUT_OF_RANGE,					\
	    "%s (%s) arg %d: %S is out of range (%s)",			\
	    (Caller),							\
	    c__FUNCTION__,						\
	    (int)(Pos),							\
	    (Arg),							\
	    (Desc))


#define FTH_BAD_ARITY_ERROR(Pos, Arg, Desc)				\
	fth_throw(FTH_BAD_ARITY,					\
	    "%s (%s) arg %d: %S, %s",					\
	    RUNNING_WORD(),						\
	    c__FUNCTION__,						\
	    (Pos),							\
	    (Arg),							\
	    (Desc))

#define FTH_BAD_ARITY_ERROR_ARGS(Pos, Arg, Wr, Wo, Wrst, Gr, Go, Grst)	\
	fth_throw(FTH_BAD_ARITY,					\
	    "%s (%s) arg %d: %S (%d/%d/%s), wanted %d/%d/%s",		\
	    RUNNING_WORD(),						\
	    c__FUNCTION__,						\
	    (Pos),							\
	    (Arg),							\
	    (Gr),							\
	    (Go),							\
	    (Grst) ? "#t" : "#f",					\
	    (Wr),							\
	    (Wo),							\
	    (Wrst) ? "#t" : "#f")

#define FTH_ANY_ERROR_THROW(Exc, Func)					\
	((errno != 0) ?							\
	 fth_throw(Exc,	"%s (%s): %s",					\
	     c__FUNCTION__, #Func, fth_strerror(errno)) :		\
	 fth_throw(Exc, "%s: %s", c__FUNCTION__, #Func))

#define FTH_ANY_ERROR_ARG_THROW(Exc, Func, Desc)			\
	((errno != 0) ?							\
	 fth_throw(Exc, "%s (%s:\"%s\"): %s",				\
	     c__FUNCTION__, #Func, Desc, fth_strerror(errno)) :		\
	 fth_throw(Exc, "%s (%s): %s", c__FUNCTION__, #Func, Desc))

#define FTH_SYSTEM_ERROR_THROW(Func)					\
	FTH_ANY_ERROR_THROW(FTH_SYSTEM_ERROR, Func)

#define FTH_SYSTEM_ERROR_ARG_THROW(Func, Desc)				\
	FTH_ANY_ERROR_ARG_THROW(FTH_SYSTEM_ERROR, Func, Desc)

#define FTH_NOT_IMPLEMENTED_ERROR(Func)					\
	fth_throw(FTH_NOT_IMPLEMENTED,					\
	    "%s (%s): %S",						\
	    c__FUNCTION__,						\
	    #Func,							\
	    fth_exception_message_ref(FTH_NOT_IMPLEMENTED))

#define FTH_BAD_SYNTAX_ERROR(Desc)					\
	fth_throw(FTH_BAD_SYNTAX,					\
	    "%s (%s): %s",						\
	    RUNNING_WORD(),						\
	    c__FUNCTION__,						\
	    (Desc))

#define FTH_NO_MEMORY_THROW()						\
	fth_throw(FTH_NO_MEMORY_ERROR,					\
	    "%s (%s): can't allocate memory",				\
	    RUNNING_WORD(),						\
	    c__FUNCTION__)

__BEGIN_DECLS

/* === array.c === */
/* array */
FTH		fth_array_append(FTH array1, FTH array2);
void		fth_array_clear(FTH array);
FTH		fth_array_compact(FTH array);
FTH		fth_array_copy(FTH array);
FTH		fth_array_delete(FTH array, ficlInteger index);
FTH		fth_array_delete_key(FTH array, FTH key);
FTH		fth_array_each(FTH array,
		    FTH (*func) (FTH value, FTH data), FTH data);
FTH		fth_array_each_with_index(FTH array,
		    FTH (*func) (FTH value, FTH data, ficlInteger idx),
		FTH data);
bool		fth_array_equal_p(FTH obj1, FTH obj2);
FTH		fth_array_fill(FTH array, FTH value);
FTH		fth_array_find(FTH array, FTH key);
ficlInteger	fth_array_index(FTH array, FTH key);
FTH		fth_array_insert(FTH array, ficlInteger index, FTH value);
FTH		fth_array_join(FTH array, FTH sep);
ficlInteger	fth_array_length(FTH obj);
FTH		fth_array_map(FTH array,
		    FTH (*func) (FTH value, FTH data), FTH data);
bool		fth_array_member_p(FTH array, FTH key);
FTH		fth_array_pop(FTH array);
FTH		fth_array_push(FTH array, FTH value);
FTH		fth_array_ref(FTH array, ficlInteger index);
FTH		fth_array_reject(FTH array, FTH proc_or_xt, FTH args);
FTH		fth_array_reverse(FTH array);
FTH		fth_array_set(FTH array, ficlInteger index, FTH value);
FTH		fth_array_shift(FTH array);
FTH		fth_array_sort(FTH array, FTH proc_or_xt);
FTH		fth_array_subarray(FTH array,
		    ficlInteger start, ficlInteger end);
FTH		fth_array_to_array(FTH array);
FTH		fth_array_to_list(FTH list);
FTH		fth_array_uniq(FTH array);
FTH		fth_array_unshift(FTH array, FTH value);
FTH		fth_make_array_len(ficlInteger len);
FTH		fth_make_array_var(int len,...);
FTH		fth_make_array_with_init(ficlInteger len, FTH init);
FTH		fth_make_empty_array(void);
/* assoc */
FTH		fth_acell_key(FTH cell);
FTH		fth_acell_value(FTH cell);
/* returns => #( key value ) */
FTH		fth_array_assoc(FTH assoc, FTH key);
/* returns => value */
FTH		fth_array_assoc_ref(FTH assoc, FTH key);
FTH		fth_array_assoc_remove(FTH assoc, FTH key);
FTH		fth_array_assoc_set(FTH assoc, FTH key, FTH value);
FTH		fth_assoc  (FTH assoc, FTH key, FTH value);
FTH		fth_make_acell(FTH key, FTH value);
/* list */
FTH		fth_acons  (FTH key, FTH value, FTH alist);
FTH		fth_cadddr (FTH list);
FTH		fth_caddr  (FTH list);
FTH		fth_cadr   (FTH list);
FTH		fth_car    (FTH list);
FTH		fth_cddr   (FTH list);
FTH		fth_cdr    (FTH list);
FTH		fth_cons   (FTH value, FTH list);
FTH		fth_cons_2 (FTH obj1, FTH obj2, FTH list);
FTH		fth_list_append(FTH args);
/* returns => '( key value ) */
FTH		fth_list_assoc(FTH alist, FTH key);
/* returns => value */
FTH		fth_list_assoc_ref(FTH alist, FTH key);
FTH		fth_list_assoc_remove(FTH alist, FTH key);
FTH		fth_list_assoc_set(FTH alist, FTH key, FTH value);
FTH		fth_list_copy(FTH list);
ficlInteger	fth_list_length(FTH obj);
FTH		fth_list_member_p(FTH list, FTH key);
FTH		fth_list_ref(FTH list, ficlInteger index);
FTH		fth_list_reverse(FTH list);
FTH		fth_list_set(FTH list, ficlInteger index, FTH value);
FTH		fth_list_to_array(FTH list);
FTH		fth_make_empty_list(void);
FTH		fth_make_list_len(ficlInteger len);
FTH		fth_make_list_var(int len,...);
FTH		fth_make_list_with_init(ficlInteger len, FTH init);

/* === file.c === */
/* general file functions */
FTH		fth_file_atime(const char *name);
FTH		fth_file_basename(const char *name, const char *ext);
void		fth_file_chmod(const char *name, mode_t mode);
void		fth_file_copy(const char *src, const char *dst);
FTH		fth_file_ctime(const char *name);
void		fth_file_delete(const char *name);
FTH		fth_file_dirname(const char *name);
bool		fth_file_install(const char *src, const char *dst, mode_t mode);
FTH		fth_file_length(const char *name);
FTH		fth_file_match_dir(FTH string, FTH regexp);
void		fth_file_mkdir(const char *name, mode_t mode);
void		fth_file_mkfifo(const char *name, mode_t mode);
FTH		fth_file_mtime(const char *name);
FTH		fth_file_realpath(const char *name);
void		fth_file_rename(const char *src, const char *dst);
void		fth_file_rmdir(const char *name);
FTH		fth_file_split(const char *name);
void		fth_file_symlink(const char *src, const char *dst);
/* file test */
bool		fth_file_block_p(const char *name);
bool		fth_file_character_p(const char *name);
bool		fth_file_directory_p(const char *name);
bool		fth_file_executable_p(const char *name);
bool		fth_file_exists_p(const char *name);
bool		fth_file_fifo_p(const char *name);
bool		fth_file_grpowned_p(const char *name);
bool		fth_file_owned_p(const char *name);
bool		fth_file_readable_p(const char *name);
bool		fth_file_setgid_p(const char *name);
bool		fth_file_setuid_p(const char *name);
bool		fth_file_socket_p(const char *name);
bool		fth_file_sticky_p(const char *name);
bool		fth_file_symlink_p(const char *name);
bool		fth_file_writable_p(const char *name);
bool		fth_file_zero_p(const char *name);

/* === hash.c === */
/* hash */
void		fth_hash_clear(FTH hash);
FTH		fth_hash_copy(FTH hash);
FTH		fth_hash_delete(FTH hash, FTH key);
FTH		fth_hash_each(FTH hash,
		    FTH (*f) (FTH key, FTH val, FTH data), FTH data);
bool		fth_hash_equal_p(FTH obj1, FTH obj2);
FTH		fth_hash_find(FTH hash, FTH key);
FTH		fth_hash_keys(FTH hash);
FTH		fth_hash_map(FTH hash,
		    FTH (*f) (FTH key, FTH val, FTH data), FTH data);
bool		fth_hash_member_p(FTH hash, FTH key);
FTH		fth_hash_ref(FTH hash, FTH key);
void		fth_hash_set(FTH hash, FTH key, FTH value);
FTH		fth_hash_to_array(FTH hash);
FTH		fth_hash_values(FTH hash);
FTH		fth_make_hash(void);
FTH		fth_make_hash_len(int hashsize);
/* properties */
FTH		fth_hash_id(FTH obj);
FTH		fth_object_id(FTH obj);
FTH		fth_object_properties(FTH obj);
FTH		fth_object_property_ref(FTH obj, FTH key);
void		fth_object_property_set(FTH obj, FTH key, FTH value);
FTH		fth_properties(FTH obj);
FTH		fth_property_ref(FTH obj, FTH key);
void		fth_property_set(FTH obj, FTH key, FTH value);
FTH		fth_word_properties(FTH obj);
FTH		fth_word_property_ref(FTH obj, FTH key);
void		fth_word_property_set(FTH obj, FTH key, FTH value);

/* === hook.c === */
/* hook */
void		fth_add_hook(FTH hook, FTH proc);
FTH		fth_hook_apply(FTH hook, FTH args, const char *caller);
int		fth_hook_arity(FTH hook);
void		fth_hook_clear(FTH hook);
bool		fth_hook_empty_p(FTH hook);
bool		fth_hook_equal_p(FTH obj1, FTH obj2);
bool		fth_hook_member_p(FTH hook, FTH name);
FTH		fth_hook_names(FTH hook);
FTH		fth_hook_to_array(FTH hook);
FTH		fth_make_hook(const char *name, int arity, const char *doc);
FTH		fth_make_hook_with_arity(const char *n,
		    int req, int opt, bool rest, const char *d);
FTH		fth_make_simple_hook(int arity);
FTH		fth_remove_hook(FTH hook, FTH name);
FTH		fth_run_hook(FTH hook, int len,...);
FTH		fth_run_hook_again(FTH hook, int len,...);
FTH		fth_run_hook_bool(FTH hook, int len,...);

/* === io.c === */
/* io */
void		fth_io_close(FTH io);
bool		fth_io_closed_p(FTH obj);
bool		fth_io_eof_p(FTH io);
bool		fth_io_equal_p(FTH obj1, FTH obj2);
char           *fth_io_filename(FTH io);
int		fth_io_fileno(FTH io);
void		fth_io_flush(FTH io);
int		fth_io_getc(FTH io);
bool		fth_io_input_p(FTH obj);
ficl2Integer	fth_io_length(FTH obj);
int		fth_io_mode(FTH io);
FTH		fth_io_nopen(const char *host, int port, int type);
FTH		fth_io_open(const char *name, int fam);
bool		fth_io_output_p(FTH obj);
FTH		fth_io_popen(FTH cmd, int fam);
ficl2Integer	fth_io_pos_ref(FTH io);
void		fth_io_pos_set(FTH io, ficl2Integer pos);
void           *fth_io_ptr(FTH io);
void		fth_io_putc(FTH io, int c);
char           *fth_io_read(FTH io);
FTH		fth_io_read_line(FTH io);
FTH		fth_io_readlines(FTH io);
void		fth_io_rewind(FTH io);
FTH		fth_io_sopen(FTH string, int fam);
FTH		fth_io_to_string(FTH io);
void		fth_io_write(FTH io, const char *line);
void		fth_io_write_and_flush(FTH io, const char *line);
void		fth_io_write_format(FTH io, FTH fmt, FTH args);
void		fth_io_writelines(FTH io, FTH array);
FTH		fth_readlines(const char *name);
int		fth_set_exit_status(int status);
FTH		fth_set_io_stderr(FTH io);
FTH		fth_set_io_stdin(FTH io);
FTH		fth_set_io_stdout(FTH io);
void		fth_writelines(const char *name, FTH array);

/* === misc.c === */
void		fth_exit  (int n);
void		fth_make_ficl(unsigned int d_size, unsigned int s_size,
		    unsigned int r_size, unsigned int l_size);
void		fth_reset (void);
/* eval */
void		fth_add_feature(const char *name);
int		fth_catch_eval(const char *buffer);
int		fth_catch_exec(ficlWord *word);
FTH		fth_eval   (const char *buffer);
void		fth_init  (void);
bool		fth_provided_p(const char *name);
/* load */
void		fth_add_load_lib_path(const char *path);
void		fth_add_load_path(const char *path);
void		fth_add_loaded_files(const char *file);
char           *fth_basename(const char *path);
FTH		fth_dl_load(const char *lib, const char *func);
FTH		fth_find_file(FTH name);
void		fth_install(void);	/* parse word */
void		fth_install_file(FTH fname);
FTH		fth_load_file(const char *name);
FTH		fth_load_global_init_file(void);
FTH		fth_load_init_file(const char *init_file);
FTH		fth_require_file(const char *name);
void		fth_unshift_load_lib_path(const char *path);
void		fth_unshift_load_path(const char *path);
/* words */
FTH		fth_apropos(FTH regexp);
void		fth_begin_values_to_obj(ficlVm *vm, char *name, FTH args);
FTH		fth_find_in_wordlist(const char *name);
char           *fth_parse_word(void);
char           *fth_short_version(void);
char           *fth_version(void);
FTH		fth_word_ref(const char *name);
FTH		fth_wordlist_each(bool(*func) (ficlWord *word, FTH data),
		    FTH data);

/* === numbers.c === */
bool		fth_char_p(FTH obj);
bool		fth_exact_p(FTH obj);
bool		fth_fixnum_p(FTH obj);
bool		fth_integer_p(FTH obj);
bool		fth_number_p(FTH obj);
bool		fth_ullong_p(FTH obj);
bool		fth_unsigned_p(FTH obj);
#if HAVE_COMPLEX
ficlComplex	fth_complex_ref(FTH x);
#endif				/* HAVE_COMPLEX */
ficlFloat	fth_float_ref(FTH x);
ficlFloat	fth_float_ref_or_else(FTH x, ficlFloat fallback);
ficlInteger	fth_integer_ref(FTH x);
ficlInteger	fth_int_ref(FTH x);
ficlInteger	fth_int_ref_or_else(FTH x, ficlInteger fallback);
bool		fth_isinf (ficlFloat f);
bool		fth_isnan (ficlFloat f);
FTH		fth_llong_copy(FTH obj);
ficl2Integer	fth_long_long_ref(FTH x);
FTH		fth_make_int(ficlInteger n);
FTH		fth_make_llong(ficl2Integer d);
FTH		fth_make_long_long(ficl2Integer d);
ficlFloat	fth_real_ref(FTH x);
FTH		fth_make_ullong(ficl2Unsigned ud);
FTH		fth_make_ulong_long(ficl2Unsigned ud);
FTH		fth_make_unsigned(ficlUnsigned u);
ficl2Unsigned	fth_ulong_long_ref(FTH x);
ficlUnsigned	fth_unsigned_ref(FTH x);
/* random */
ficlFloat	fth_frandom(ficlFloat f);	/* -f...f */
ficlFloat	fth_random(ficlFloat f);	/* 0...f */
void		fth_srand (ficlUnsigned u);
/* float */
FTH		fth_float_copy(FTH x);
FTH		fth_make_float(ficlFloat f);
#if HAVE_COMPLEX
/* complex */
ficlComplex	ficlStackPopComplex(ficlStack *stack);
void		ficlStackPushComplex(ficlStack *stack, ficlComplex cp);
FTH		fth_make_complex(ficlComplex z);
FTH		fth_make_polar(ficlFloat real, ficlFloat theta);
FTH		fth_make_rectangular(ficlFloat real, ficlFloat image);
#endif				/* HAVE_COMPLEX */
#if HAVE_BN
FTH		fth_make_bignum(ficlBignum bn);
FTH		fth_make_big(FTH m);
#endif				/* HAVE_BN */
/* ratio */
FTH		fth_exact_to_inexact(FTH x);
FTH		fth_inexact_to_exact(FTH x);
FTH		fth_denominator(FTH x);
FTH		fth_numerator(FTH x);
#if HAVE_BN
FTH		fth_make_ratio(FTH num, FTH den);
FTH		fth_make_ratio_from_float(ficlFloat f);
FTH		fth_make_ratio_from_int(ficlInteger num, ficlInteger den);
FTH		fth_make_rational(ficlRatio r);
FTH		fth_ratio_floor(FTH x);
FTH		fth_rationalize(FTH x, FTH err);
#endif				/* HAVE_BN */
FTH		fth_number_add(FTH x, FTH y);
FTH		fth_number_div(FTH x, FTH y);
bool		fth_number_equal_p(FTH x, FTH y);
bool		fth_number_less_p(FTH x, FTH y);
FTH		fth_number_mul(FTH x, FTH y);
FTH		fth_number_sub(FTH x, FTH y);

/* === object.c === */
/* gc */
void		fth_gc_mark(FTH obj);
FTH		fth_gc_off (void);
FTH		fth_gc_on  (void);
FTH		fth_gc_permanent(FTH obj);
FTH		fth_gc_protect(FTH obj);
FTH		fth_gc_protect_set(FTH out, FTH in);
void		fth_gc_unmark(FTH obj);
FTH		fth_gc_unprotect(FTH obj);
/* object */
FTH		fth_make_object_type(const char *name);
FTH		fth_make_object_type_from(const char *name, FTH base);
bool		fth_object_type_p(FTH obj);
/* instance */
bool		fth_instance_flag_p(FTH obj, int flags);
bool		fth_instance_p(FTH obj);
void           *fth_instance_ref_gen(FTH obj);
bool		fth_instance_type_p(FTH obj, fobj_t type);
FTH		fth_make_instance(FTH obj, void *gen);
bool		fth_object_is_instance_of(FTH obj, FTH type);
/* object set functions */
/* NOSTRICT */
FTH		fth_set_object_apply(FTH obj, void *apply,
		    int req, int opt, int rest);
FTH		fth_set_object_copy(FTH obj, FTH (*copy) (FTH obj));
FTH		fth_set_object_dump(FTH obj, FTH (*dump) (FTH obj));
FTH		fth_set_object_equal_p(FTH obj,
		    FTH (*equal_p) (FTH obj1, FTH obj2));
FTH		fth_set_object_free(FTH obj, void (*free) (FTH obj));
FTH		fth_set_object_inspect(FTH obj, FTH (*inspect) (FTH obj));
FTH		fth_set_object_length(FTH obj, FTH (*length) (FTH obj));
FTH		fth_set_object_mark(FTH obj, void (*mark) (FTH obj));
FTH		fth_set_object_to_array(FTH obj, FTH (*to_array) (FTH obj));
FTH		fth_set_object_to_string(FTH obj,
		    FTH (*to_string) (FTH obj));
FTH		fth_set_object_value_ref(FTH obj,
		    FTH (*value_ref) (FTH obj, FTH index));
FTH		fth_set_object_value_set(FTH obj,
		    FTH (*value_set) (FTH obj, FTH index, FTH value));
/* object functions */
FTH		fth_object_apply(FTH obj, FTH args);
FTH		fth_object_copy(FTH obj);
FTH		fth_object_dump(FTH obj);
bool		fth_object_empty_p(FTH obj);
bool		fth_object_equal_p(FTH obj1, FTH obj2);
FTH		fth_object_find(FTH obj, FTH key);
FTH		fth_object_index(FTH obj, FTH key);
FTH		fth_object_inspect(FTH obj);
ficlInteger	fth_object_length(FTH obj);
bool		fth_object_member_p(FTH obj, FTH key);
char           *fth_object_name(FTH obj);
bool		fth_object_range_p(FTH obj, ficlInteger index);
FTH		fth_object_sort(FTH obj, FTH proc);
FTH		fth_object_to_array(FTH obj);
FTH		fth_object_to_string(FTH obj);
/* Wrap String object type to "string". */
FTH		fth_object_to_string_2(FTH obj);
FTH		fth_object_value_ref(FTH obj, ficlInteger index);
FTH		fth_object_value_set(FTH obj, ficlInteger index, FTH value);
char           *fth_to_c_dump(FTH obj);
char           *fth_to_c_inspect(FTH obj);
char           *fth_to_c_string(FTH obj);
/* Wrap String object type to "string". */
char           *fth_to_c_string_2(FTH obj);
/* cycle */
ficlInteger	fth_cycle_next(FTH obj);
ficlInteger	fth_cycle_pos_0(FTH obj);
ficlInteger	fth_cycle_pos_ref(FTH obj);
ficlInteger	fth_cycle_pos_set(FTH obj, ficlInteger index);
FTH		fth_object_cycle_ref(FTH obj);
FTH		fth_object_cycle_set(FTH obj, FTH value);
/* stack access */
FTH		ficl_to_fth(FTH obj);
FTH		fth_pop_ficl_cell(ficlVm *vm);
void		fth_push_ficl_cell(ficlVm *vm, FTH obj);
FTH		fth_to_ficl(FTH obj);

/* === port.c === */
/* port */
void		fth_port_close(FTH port);
void		fth_port_display(FTH port, FTH obj);
void		fth_port_flush(FTH port);
int		fth_port_getc(FTH port);
char           *fth_port_gets(FTH port);
void		fth_port_putc(FTH port, int c);
void		fth_port_puts(FTH port, const char *str);
out_cb		fth_set_error_cb(out_cb cb);
out_cb		fth_set_print_and_error_cb(out_cb cb);
/* output callbacks */
/* void (*out_cb)(ficlVm *vm, char *msg); */
out_cb		fth_set_print_cb(out_cb cb);
/* input callback */
/* char *(*in_cb)(ficlVm *vm); */
in_cb		fth_set_read_cb(in_cb cb);
FTH		fth_port_to_string(FTH port);

/* === printf.c === */
int		fth_asprintf(char **result, const char *fmt,...);
int		fth_debug  (const char *fmt,...);
int		fth_error  (const char *str);
int		fth_errorf (const char *fmt,...);
char           *fth_format(const char *fmt,...);
int		fth_fprintf(FILE * fp, const char *fmt,...);
int		fth_ioprintf(FTH io, const char *fmt,...);
int		fth_port_printf(FTH port, const char *fmt,...);
int		fth_port_vprintf(FTH port, const char *fmt, va_list ap);
int		fth_print  (const char *str);
int		fth_printf (const char *fmt,...);
int		fth_snprintf(char *buffer, size_t size, const char *fmt,...);
int		fth_sprintf(char *buffer, const char *fmt,...);
FTH		fth_string_vformat(const char *fmt, FTH args);
int		fth_vasprintf(char **result, const char *fmt, va_list ap);
int		fth_verrorf(const char *fmt, va_list ap);
char           *fth_vformat(const char *fmt, va_list ap);
int		fth_vfprintf(FILE * fp, const char *fmt, va_list ap);
int		fth_vioprintf(FTH io, const char *fmt, va_list ap);
int		fth_vprintf(const char *fmt, va_list ap);
int		fth_vsnprintf(char *buffer, size_t size,
		    const char *fmt, va_list ap);
int		fth_vsprintf(char *buffer, const char *fmt, va_list ap);
int		fth_warning(const char *fmt,...);

/* === proc.c === */
bool		fth_word_defined_p(FTH obj);
bool		fth_word_type_p(FTH obj, int type);
/* proc */
ficlWord       *fth_define_procedure(const char *,
		    FTH (*) (), int, int, int, const char *);
ficlWord       *fth_define_void_procedure(const char *, 
		    void (*) (), int, int, int, const char *);
FTH		fth_documentation_ref(FTH obj);
void		fth_documentation_set(FTH obj, FTH doc);
FTH		fth_get_optarg(ficlInteger req, FTH def);
FTH		fth_get_optkey(FTH key, FTH def);
ficl2Integer	fth_get_optkey_2int(FTH key, ficl2Integer def);
ficlInteger	fth_get_optkey_int(FTH key, ficlInteger def);
int		fth_get_optkey_fix(FTH key, int def);
char           *fth_get_optkey_str(FTH key, char *def);
FTH		fth_make_proc(ficlWord *word, int req, int opt, bool rest);
FTH		fth_make_proc_from_func(const char *,
		    FTH (*) (), bool, int, int, bool);
FTH		fth_proc_apply(FTH proc, FTH args, const char *caller);
int		fth_proc_arity(FTH proc);
FTH		fth_proc_call(FTH proc, const char *caller, int len,...);
char           *fth_proc_name(FTH obj);
FTH		fth_proc_source_ref(FTH proc);
void		fth_proc_source_set(FTH proc, FTH source);
ficlWord       *fth_proc_to_xt(FTH proc);
FTH		fth_source_file(FTH obj);
FTH		fth_source_line(FTH obj);
FTH		fth_source_ref(FTH obj);
void		fth_source_set(FTH obj, FTH source);
ficlWord       *fth_word_doc_set(ficlWord *word, const char *str);
FTH		proc_from_proc_or_xt(FTH proc_or_xt,
		    int req, int opt, bool rest);
/* define, variable */
FTH		fth_define (const char *name, FTH value);
FTH		fth_define_constant(const char *name,
		    FTH value, const char *doc);
FTH		fth_define_variable(const char *name,
		    FTH value, const char *doc);
bool		fth_defined_p(const char *name);
void		fth_trace_var(FTH obj, FTH proc_or_xt);
FTH		fth_trace_var_execute(ficlWord *word);
void		fth_untrace_var(FTH obj);
FTH		fth_var_ref(FTH obj);
FTH		fth_var_set(FTH obj, FTH value);
FTH		fth_variable_ref(const char *name);
FTH		fth_variable_set(const char *name, FTH value);

/* === regexp.c === */
/* regexp */
FTH		fth_make_regexp(const char *str);
int		fth_regexp_find(const char *reg, const char *str);
ficlInteger	fth_regexp_match(FTH regexp, FTH string);
FTH		fth_regexp_replace(FTH regexp, FTH string, FTH replace);
ficlInteger	fth_regexp_search(FTH regexp, FTH string,
		    ficlInteger beg, ficlInteger len);

/* === string.c === */
/* string */

FTH		fth_make_empty_string(void);
FTH		fth_make_string(const char *str);
FTH		fth_make_string_format(const char *fmt,...);
FTH		fth_make_string_len(const char *str, ficlInteger len);
FTH		fth_make_string_or_false(const char *str);
FTH		fth_string_append(FTH string1, FTH string2);
FTH		fth_string_capitalize(FTH string);
FTH		fth_string_char_ref(FTH string, ficlInteger index);
FTH		fth_string_char_set(FTH string, ficlInteger index, FTH value);
FTH		fth_string_chomp(FTH string);
FTH		fth_string_copy(FTH string);
char		fth_string_c_char_ref(FTH string, ficlInteger index);
char		fth_string_c_char_set(FTH string, ficlInteger index, char c);
FTH		fth_string_delete(FTH string, ficlInteger index);
FTH		fth_string_downcase(FTH string);
bool		fth_string_equal_p(FTH obj1, FTH obj2);
int		fth_string_eval(FTH string);
FTH		fth_string_fill(FTH string, FTH fill_char);
FTH		fth_string_find(FTH string, FTH key);
FTH		fth_string_format(FTH string, FTH args_list);
bool		fth_string_greater_p(FTH obj1, FTH obj2);
FTH		fth_string_index(FTH string, FTH key);
FTH		fth_string_insert(FTH string, ficlInteger index, FTH value);
ficlInteger	fth_string_length(FTH string);
bool		fth_string_less_p(FTH obj1, FTH obj2);
bool		fth_string_member_p(FTH string, FTH key);
bool		fth_string_not_equal_p(FTH obj1, FTH obj2);
FTH		fth_string_pop(FTH string);
FTH		fth_string_push(FTH string, FTH add_str);
char           *fth_string_ref(FTH string);
FTH		fth_string_replace(FTH string, FTH from, FTH to);
FTH		fth_string_reverse(FTH string);
FTH		fth_string_scat(FTH string, const char *str);
FTH		fth_string_sformat(FTH string, const char *fmt,...);
FTH		fth_string_shift(FTH string);
FTH		fth_string_sncat(FTH string, const char *str, ficlInteger len);
FTH		fth_string_split(FTH string, FTH sep_str);
FTH		fth_string_substring(FTH string, ficlInteger start, ficlInteger end);
FTH		fth_string_to_array(FTH string);
FTH		fth_string_unshift(FTH string, FTH add_str);
FTH		fth_string_upcase(FTH string);
FTH		fth_string_vsformat(FTH string, const char *fmt, va_list ap);

/* === symbol.c === */
/* symbol */
bool		fth_string_or_symbol_p(FTH obj);
char           *fth_string_or_symbol_ref(FTH obj);
FTH		fth_symbol (const char *name);
bool		fth_symbol_equal_p(FTH obj1, FTH obj2);
bool		fth_symbol_p(const char *name);
char           *fth_symbol_ref(FTH obj);
/* keyword */
FTH		fth_keyword(const char *name);
bool		fth_keyword_equal_p(FTH obj1, FTH obj2);
bool		fth_keyword_p(const char *name);
char           *fth_keyword_ref(FTH obj);
/* exception */
FTH		fth_exception(const char *name);
bool		fth_exception_equal_p(FTH obj1, FTH obj2);
FTH		fth_exception_last_message_ref(FTH exc);
void		fth_exception_last_message_set(FTH exc, FTH msg);
FTH		fth_exception_message_ref(FTH exc);
void		fth_exception_message_set(FTH exc, FTH msg);
char           *fth_exception_ref(FTH obj);
FTH		fth_make_exception(const char *name, const char *message);
bool		fth_symbol_or_exception_p(FTH obj);
FTH		fth_symbol_or_exception_ref(FTH obj);
FTH		fth_symbol_to_exception(FTH obj);

/* === utils.c === */
void           *fth_calloc(size_t size, size_t eltsize);
int		fth_evaluate(ficlVm *vm, const char *buffer);
int		fth_execute_xt(ficlVm *vm, ficlWord *word);
void		fth_free  (void *p);
char           *fth_getenv(const char *name, char *def);
void           *fth_malloc(size_t size);
void           *fth_realloc(void *p, size_t size);
void		fth_repl  (int argc, char **argv);
char	       *fth_strcat(char *d, size_t size, const char *s);
char	       *fth_strcpy(char *d, size_t size, const char *s);
char           *fth_strdup(const char *s);
char           *fth_strerror(int n);
size_t		fth_strlen(const char *s);
char	       *fth_strncat(char *d, size_t size, const char *s, size_t count);
char	       *fth_strncpy(char *d, size_t size, const char *s, size_t count);
void		fth_throw (FTH exc, const char *fmt,...);
void		fth_throw_error(FTH exc, FTH args);
void		fth_throw_list(FTH exc, FTH args);
char           *pop_cstring(ficlVm *vm);
void		push_cstring(ficlVm *vm, char *s);

__END_DECLS

#endif				/* _FTH_H_ */

/*
 * fth.h ends here
 */
