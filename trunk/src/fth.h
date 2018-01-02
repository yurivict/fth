/*-
 * Copyright (c) 2005-2018 Michael Scholz <mi-scholz@users.sourceforge.net>
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
 * @(#)fth.h	2.1 1/2/18
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

#define INT_TO_FIX(Obj)		((FTH)(((FTH)(Obj)) << 1L | FIXNUM_FLAG))
#define FIX_TO_INT(Obj)		(((SIGNED_FTH)(Obj)) >> 1L)
#define FIX_TO_INT32(Obj)	(int)FIX_TO_INT(Obj)
#define UNSIGNED_TO_FIX(Obj)	INT_TO_FIX(Obj)
#define FIX_TO_UNSIGNED(Obj)	(((FTH)(Obj)) >> 1L)

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
	FTH 		current_file;
	ficlInteger 	current_line;
	int 		print_length;
	FTH 		last_exception;
	FTH 		_false;
	FTH 		_true;
	FTH 		_nil;
	FTH 		_undef;
	int 		print_p;
	int 		eval_p;
	int 		hit_error_p;
	int 		true_repl_p;
	int 		die_on_signal_p;
	int 		interactive_p;
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
extern in_cb 	fth_read_hook;
extern out_cb 	fth_print_hook;
extern out_cb 	fth_error_hook;
extern exit_cb 	fth_exit_hook;

/* === Predicates === */
#define FTH_INSTANCE_FLAG_P(Obj, Type)	fth_instance_flag_p(Obj, Type)
#define FTH_INSTANCE_TYPE_P(Obj, Type)	fth_instance_type_p(Obj, Type)

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
#define FTH_FLOAT_T_P(Obj)	FTH_INSTANCE_TYPE_P(Obj, FTH_FLOAT_T)
#define FTH_LONG_LONG_P(Obj)	FTH_INTEGER_P(Obj)
#define FTH_FLOAT_P(Obj)	FTH_NUMBER_P(Obj)
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
		fth_throw(FTH_NULL_STRING, "%s: null string", RUNNING_WORD())

#define FTH_WRONG_TYPE_ARG_ERROR(Caller, Pos, Arg, Desc)		\
	fth_throw(FTH_WRONG_TYPE_ARG,					\
	    "%s: wrong type arg %d, %s (%S), wanted %s",		\
	    (Caller), 							\
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
	    "%s arg %d: %ld is %s", RUNNING_WORD(), (int)(Pos),		\
	    (ficlInteger)(Arg), (Desc))

#define FTH_OUT_OF_BOUNDS(Pos, Arg)					\
	FTH_OUT_OF_BOUNDS_ERROR(Pos, Arg, "out of range")

/*
 * This is special for Snd:
 *	(char *Caller, int Pos, FTH Arg, char *Desc)
 */
#define FTH_OUT_OF_RANGE_ERROR(Caller, Pos, Arg, Desc)			\
	fth_throw(FTH_OUT_OF_RANGE,					\
	    "%s arg %d: %S is out of range (%s)",			\
	    (Caller), (int)(Pos), (Arg), (Desc))

#define FTH_BAD_ARITY_ERROR(Pos, Arg, Desc)				\
	fth_throw(FTH_BAD_ARITY,					\
	    "%s arg %d: %S, %s", RUNNING_WORD(), (Pos), (Arg), (Desc))

#define FTH_BAD_ARITY_ERROR_ARGS(Pos, Arg, Wr, Wo, Wrst, Gr, Go, Grst)	\
	fth_throw(FTH_BAD_ARITY,					\
	    "%s arg %d: %S (%d/%d/%s), wanted %d/%d/%s",		\
	    RUNNING_WORD(),						\
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
	 fth_throw(Exc,	"%s: %s", #Func, fth_strerror(errno)) :		\
	 fth_throw(Exc, "%s", #Func))

#define FTH_ANY_ERROR_ARG_THROW(Exc, Func, Desc)			\
	((errno != 0) ?							\
	 fth_throw(Exc, "%s (%s): %s", #Func, Desc, fth_strerror(errno)) :\
	 fth_throw(Exc, "%s: %s", #Func, Desc))

#define FTH_SYSTEM_ERROR_THROW(Func)					\
	FTH_ANY_ERROR_THROW(FTH_SYSTEM_ERROR, Func)

#define FTH_SYSTEM_ERROR_ARG_THROW(Func, Desc)				\
	FTH_ANY_ERROR_ARG_THROW(FTH_SYSTEM_ERROR, Func, Desc)

#define FTH_NOT_IMPLEMENTED_ERROR(Func)					\
	fth_throw(FTH_NOT_IMPLEMENTED,					\
	    "%s: %S", #Func, fth_exception_message_ref(FTH_NOT_IMPLEMENTED))

#define FTH_BAD_SYNTAX_ERROR(Desc)					\
	fth_throw(FTH_BAD_SYNTAX, "%s: %s", RUNNING_WORD(), (Desc))

#define FTH_NO_MEMORY_THROW()						\
	fth_throw(FTH_NO_MEMORY_ERROR,					\
	    "%s: can't allocate memory", RUNNING_WORD())

__BEGIN_DECLS

/* === array.c === */
/* array */
FTH		fth_array_append(FTH, FTH);
void		fth_array_clear(FTH);
FTH		fth_array_compact(FTH);
FTH		fth_array_copy(FTH);
FTH		fth_array_delete(FTH, ficlInteger);
FTH		fth_array_delete_key(FTH, FTH);
FTH		fth_array_each(FTH, FTH (*) (FTH, FTH), FTH);
FTH		fth_array_each_with_index(FTH,
		    FTH (*) (FTH, FTH, ficlInteger), FTH);
int		fth_array_equal_p(FTH, FTH);
FTH		fth_array_fill(FTH, FTH);
FTH		fth_array_find(FTH, FTH);
ficlInteger	fth_array_index(FTH, FTH);
FTH		fth_array_insert(FTH, ficlInteger, FTH);
FTH		fth_array_join(FTH, FTH);
ficlInteger	fth_array_length(FTH);
FTH		fth_array_map(FTH, FTH (*) (FTH, FTH), FTH);
int		fth_array_member_p(FTH, FTH);
FTH		fth_array_pop(FTH);
FTH		fth_array_push(FTH, FTH);
FTH		fth_array_ref(FTH, ficlInteger);
FTH		fth_array_reject(FTH, FTH, FTH);
FTH		fth_array_reverse(FTH);
FTH		fth_array_set(FTH, ficlInteger, FTH);
FTH		fth_array_shift(FTH);
FTH		fth_array_sort(FTH, FTH);
FTH		fth_array_subarray(FTH, ficlInteger, ficlInteger);
FTH		fth_array_to_array(FTH);
FTH		fth_array_to_list(FTH);
FTH		fth_array_uniq(FTH);
FTH		fth_array_unshift(FTH, FTH);
FTH		fth_make_array_len(ficlInteger);
FTH		fth_make_array_var(int,...);
FTH		fth_make_array_with_init(ficlInteger, FTH);
FTH		fth_make_empty_array(void);
/* assoc */
FTH		fth_acell_key(FTH);
FTH		fth_acell_value(FTH);
/* returns => #( key value ) */
FTH		fth_array_assoc(FTH, FTH);
/* returns => value */
FTH		fth_array_assoc_ref(FTH, FTH);
FTH		fth_array_assoc_remove(FTH, FTH);
FTH		fth_array_assoc_set(FTH, FTH, FTH);
FTH		fth_assoc(FTH, FTH, FTH);
FTH		fth_make_acell(FTH, FTH);
/* list */
FTH		fth_acons(FTH, FTH, FTH);
FTH		fth_cadddr(FTH);
FTH		fth_caddr(FTH);
FTH		fth_cadr(FTH);
FTH		fth_car(FTH);
FTH		fth_cddr(FTH);
FTH		fth_cdr(FTH);
FTH		fth_cons(FTH, FTH);
FTH		fth_cons_2(FTH, FTH, FTH);
FTH		fth_list_append(FTH);
/* returns => '( key value ) */
FTH		fth_list_assoc(FTH, FTH);
/* returns => value */
FTH		fth_list_assoc_ref(FTH, FTH);
FTH		fth_list_assoc_remove(FTH, FTH);
FTH		fth_list_assoc_set(FTH, FTH, FTH);
FTH		fth_list_copy(FTH);
ficlInteger	fth_list_length(FTH);
FTH		fth_list_member_p(FTH, FTH);
FTH		fth_list_ref(FTH, ficlInteger);
FTH		fth_list_reverse(FTH);
FTH		fth_list_set(FTH, ficlInteger, FTH);
FTH		fth_list_to_array(FTH);
FTH		fth_make_empty_list(void);
FTH		fth_make_list_len(ficlInteger);
FTH		fth_make_list_var(int,...);
FTH		fth_make_list_with_init(ficlInteger, FTH);

/* === file.c === */
/* general file functions */
FTH		fth_file_atime(const char *);
FTH		fth_file_basename(const char *, const char *);
void		fth_file_chmod(const char *, mode_t);
void		fth_file_copy(const char *, const char *);
FTH		fth_file_ctime(const char *);
void		fth_file_delete(const char *);
FTH		fth_file_dirname(const char *);
int		fth_file_install(const char *, const char *, mode_t);
FTH		fth_file_length(const char *);
FTH		fth_file_match_dir(FTH, FTH);
void		fth_file_mkdir(const char *, mode_t);
void		fth_file_mkfifo(const char *, mode_t);
FTH		fth_file_mtime(const char *);
FTH		fth_file_realpath(const char *);
void		fth_file_rename(const char *, const char *);
void		fth_file_rmdir(const char *);
FTH		fth_file_split(const char *);
void		fth_file_symlink(const char *, const char *);
/* file test */
int		fth_file_block_p(const char *);
int		fth_file_character_p(const char *);
int		fth_file_directory_p(const char *);
int		fth_file_executable_p(const char *);
int		fth_file_exists_p(const char *);
int		fth_file_fifo_p(const char *);
int		fth_file_grpowned_p(const char *);
int		fth_file_owned_p(const char *);
int		fth_file_readable_p(const char *);
int		fth_file_setgid_p(const char *);
int		fth_file_setuid_p(const char *);
int		fth_file_socket_p(const char *);
int		fth_file_sticky_p(const char *);
int		fth_file_symlink_p(const char *);
int		fth_file_writable_p(const char *);
int		fth_file_zero_p(const char *);

/* === hash.c === */
/* hash */
void		fth_hash_clear(FTH);
FTH		fth_hash_copy(FTH);
FTH		fth_hash_delete(FTH, FTH);
FTH		fth_hash_each(FTH, FTH (*) (FTH, FTH, FTH), FTH);
int		fth_hash_equal_p(FTH, FTH);
FTH		fth_hash_find(FTH, FTH);
FTH		fth_hash_keys(FTH);
FTH		fth_hash_map(FTH, FTH (*) (FTH, FTH, FTH), FTH);
int		fth_hash_member_p(FTH, FTH);
FTH		fth_hash_ref(FTH, FTH);
void		fth_hash_set(FTH, FTH, FTH);
FTH		fth_hash_to_array(FTH);
FTH		fth_hash_values(FTH);
FTH		fth_make_hash(void);
FTH		fth_make_hash_len(int);
/* properties */
FTH		fth_hash_id(FTH);
FTH		fth_object_id(FTH);
FTH		fth_object_properties(FTH);
FTH		fth_object_property_ref(FTH, FTH);
void		fth_object_property_set(FTH, FTH, FTH);
FTH		fth_properties(FTH);
FTH		fth_property_ref(FTH, FTH);
void		fth_property_set(FTH, FTH, FTH);
FTH		fth_word_properties(FTH);
FTH		fth_word_property_ref(FTH, FTH);
void		fth_word_property_set(FTH, FTH, FTH);

/* === hook.c === */
/* hook */
void		fth_add_hook(FTH, FTH);
FTH		fth_hook_apply(FTH, FTH, const char *);
int		fth_hook_arity(FTH);
void		fth_hook_clear(FTH);
int		fth_hook_empty_p(FTH);
int		fth_hook_equal_p(FTH, FTH);
int		fth_hook_member_p(FTH, FTH);
FTH		fth_hook_names(FTH);
FTH		fth_hook_to_array(FTH);
FTH		fth_make_hook(const char *, int, const char *);
FTH		fth_make_hook_with_arity(const char *,
		    int, int, int, const char *);
FTH		fth_make_simple_hook(int);
FTH		fth_remove_hook(FTH, FTH);
FTH		fth_run_hook(FTH, int,...);
FTH		fth_run_hook_again(FTH, int,...);
FTH		fth_run_hook_bool(FTH, int,...);

/* === io.c === */
/* io */
void		fth_io_close(FTH);
int		fth_io_closed_p(FTH);
int		fth_io_eof_p(FTH);
int		fth_io_equal_p(FTH, FTH);
char           *fth_io_filename(FTH);
int		fth_io_fileno(FTH);
void		fth_io_flush(FTH);
int		fth_io_getc(FTH);
int		fth_io_input_p(FTH);
ficl2Integer	fth_io_length(FTH);
int		fth_io_mode(FTH);
FTH		fth_io_nopen(const char *, int, int);
FTH		fth_io_open(const char *, int);
int		fth_io_output_p(FTH);
FTH		fth_io_popen(FTH, int);
ficl2Integer	fth_io_pos_ref(FTH);
void		fth_io_pos_set(FTH, ficl2Integer);
void           *fth_io_ptr(FTH);
void		fth_io_putc(FTH, int);
char           *fth_io_read(FTH);
FTH		fth_io_read_line(FTH);
FTH		fth_io_readlines(FTH);
void		fth_io_rewind(FTH);
FTH		fth_io_sopen(FTH, int);
FTH		fth_io_to_string(FTH);
void		fth_io_write(FTH, const char *);
void		fth_io_write_and_flush(FTH, const char *);
void		fth_io_write_format(FTH, FTH, FTH);
void		fth_io_writelines(FTH, FTH);
FTH		fth_readlines(const char *);
int		fth_set_exit_status(int);
FTH		fth_set_io_stderr(FTH);
FTH		fth_set_io_stdin(FTH);
FTH		fth_set_io_stdout(FTH);
void		fth_writelines(const char *, FTH);

/* === misc.c === */
void		fth_exit  (int);
void		fth_make_ficl(unsigned, unsigned, unsigned, unsigned);
void		fth_reset (void);
/* eval */
void		fth_add_feature(const char *);
int		fth_catch_eval(const char *);
int		fth_catch_exec(ficlWord *);
FTH		fth_eval(const char *);
void		fth_init(void);
int		fth_provided_p(const char *);
/* load */
void		fth_add_load_lib_path(const char *);
void		fth_add_load_path(const char *);
void		fth_add_loaded_files(const char *);
char           *fth_basename(const char *);
FTH		fth_dl_load(const char *, const char *);
FTH		fth_find_file(FTH);
void		fth_install(void);	/* parse word */
void		fth_install_file(FTH);
FTH		fth_load_file(const char *);
FTH		fth_load_global_init_file(void);
FTH		fth_load_init_file(const char *);
FTH		fth_require_file(const char *);
void		fth_unshift_load_lib_path(const char *);
void		fth_unshift_load_path(const char *);
/* words */
FTH		fth_apropos(FTH);
void		fth_begin_values_to_obj(ficlVm *, char *, FTH);
FTH		fth_find_in_wordlist(const char *);
char           *fth_parse_word(void);
char           *fth_short_version(void);
char           *fth_version(void);
FTH		fth_word_ref(const char *);
FTH		fth_wordlist_each(int (*) (ficlWord *, FTH), FTH);

/* === numbers.c === */
int		fth_char_p(FTH);
int		fth_exact_p(FTH);
int		fth_fixnum_p(FTH);
int		fth_integer_p(FTH);
int		fth_number_p(FTH);
int		fth_ullong_p(FTH);
int		fth_unsigned_p(FTH);
#if HAVE_COMPLEX
ficlComplex	fth_complex_ref(FTH);
#endif
ficlFloat	fth_float_ref(FTH);
ficlFloat	fth_float_ref_or_else(FTH, ficlFloat);
ficlInteger	fth_integer_ref(FTH);
ficlInteger	fth_int_ref(FTH);
ficlInteger	fth_int_ref_or_else(FTH, ficlInteger);
int		fth_isinf (ficlFloat);
int		fth_isnan (ficlFloat);
FTH		fth_llong_copy(FTH);
ficl2Integer	fth_long_long_ref(FTH);
FTH		fth_make_int(ficlInteger);
FTH		fth_make_llong(ficl2Integer);
FTH		fth_make_long_long(ficl2Integer);
ficlFloat	fth_real_ref(FTH);
FTH		fth_make_ullong(ficl2Unsigned);
FTH		fth_make_ulong_long(ficl2Unsigned);
FTH		fth_make_unsigned(ficlUnsigned);
ficl2Unsigned	fth_ulong_long_ref(FTH);
ficlUnsigned	fth_unsigned_ref(FTH);
/* random */
ficlFloat	fth_frandom(ficlFloat);	/* -f...f */
ficlFloat	fth_random(ficlFloat);	/* 0...f */
void		fth_srand (ficlUnsigned);
/* float */
FTH		fth_float_copy(FTH);
FTH		fth_make_float(ficlFloat);
#if HAVE_COMPLEX
/* complex */
ficlComplex 	ficlStackPopComplex(ficlStack *);
void 		ficlStackPushComplex(ficlStack *, ficlComplex);
FTH 		fth_make_complex(ficlComplex);
FTH 		fth_make_polar(ficlFloat, ficlFloat);
FTH 		fth_make_rectangular(ficlFloat, ficlFloat);
#endif				/* HAVE_COMPLEX */
#if HAVE_BN
FTH 		fth_make_bignum(ficlBignum);
FTH 		fth_make_big(FTH);
#endif				/* HAVE_BN */
/* ratio */
FTH		fth_exact_to_inexact(FTH);
FTH		fth_inexact_to_exact(FTH);
FTH		fth_denominator(FTH);
FTH		fth_numerator(FTH);
#if HAVE_BN
FTH 		fth_make_ratio(FTH, FTH);
FTH 		fth_make_ratio_from_float(ficlFloat);
FTH 		fth_make_ratio_from_int(ficlInteger, ficlInteger);
FTH 		fth_make_rational(ficlRatio);
FTH 		fth_ratio_floor(FTH);
FTH 		fth_rationalize(FTH, FTH);
#endif				/* HAVE_BN */
FTH		fth_number_add(FTH, FTH);
FTH		fth_number_div(FTH, FTH);
int		fth_number_equal_p(FTH, FTH);
int		fth_number_less_p(FTH, FTH);
FTH		fth_number_mul(FTH, FTH);
FTH		fth_number_sub(FTH, FTH);

/* === object.c === */
/* gc */
void		fth_gc_mark(FTH);
FTH		fth_gc_off(void);
FTH		fth_gc_on(void);
FTH		fth_gc_permanent(FTH);
FTH		fth_gc_protect(FTH);
FTH		fth_gc_protect_set(FTH, FTH);
void		fth_gc_unmark(FTH);
FTH		fth_gc_unprotect(FTH);
/* object */
FTH		fth_make_object_type(const char *);
FTH		fth_make_object_type_from(const char *, FTH);
int		fth_object_type_p(FTH);
/* instance */
int		fth_instance_flag_p(FTH, int);
int		fth_instance_p(FTH);
void           *fth_instance_ref_gen(FTH);
int		fth_instance_type_p(FTH, fobj_t);
FTH		fth_make_instance(FTH, void *);
int		fth_object_is_instance_of(FTH, FTH);
/* object set functions */
/* NOSTRICT */
FTH		fth_set_object_apply(FTH, void *, int, int, int);
FTH		fth_set_object_copy(FTH, FTH (*) (FTH));
FTH		fth_set_object_dump(FTH, FTH (*) (FTH));
FTH		fth_set_object_equal_p(FTH, FTH (*) (FTH, FTH));
FTH		fth_set_object_free(FTH, void (*) (FTH));
FTH		fth_set_object_inspect(FTH, FTH (*) (FTH));
FTH		fth_set_object_length(FTH, FTH (*) (FTH));
FTH		fth_set_object_mark(FTH, void (*) (FTH));
FTH		fth_set_object_to_array(FTH, FTH (*) (FTH));
FTH		fth_set_object_to_string(FTH, FTH (*) (FTH));
FTH		fth_set_object_value_ref(FTH, FTH (*) (FTH, FTH));
FTH		fth_set_object_value_set(FTH, FTH (*) (FTH, FTH, FTH));
/* object functions */
FTH		fth_object_apply(FTH, FTH);
FTH		fth_object_copy(FTH);
FTH		fth_object_dump(FTH);
int		fth_object_empty_p(FTH);
int		fth_object_equal_p(FTH, FTH);
FTH		fth_object_find(FTH, FTH);
FTH		fth_object_index(FTH, FTH);
FTH		fth_object_inspect(FTH);
ficlInteger	fth_object_length(FTH);
int		fth_object_member_p(FTH, FTH);
char           *fth_object_name(FTH);
int		fth_object_range_p(FTH, ficlInteger);
FTH		fth_object_sort(FTH, FTH);
FTH		fth_object_to_array(FTH);
FTH		fth_object_to_string(FTH);
/* Wrap String object type to "string". */
FTH		fth_object_to_string_2(FTH);
FTH		fth_object_value_ref(FTH, ficlInteger);
FTH		fth_object_value_set(FTH, ficlInteger, FTH);
char           *fth_to_c_dump(FTH);
char           *fth_to_c_inspect(FTH);
char           *fth_to_c_string(FTH);
/* Wrap String object type to "string". */
char           *fth_to_c_string_2(FTH);
/* cycle */
ficlInteger	fth_cycle_next(FTH);
ficlInteger	fth_cycle_pos_0(FTH);
ficlInteger	fth_cycle_pos_ref(FTH);
ficlInteger	fth_cycle_pos_set(FTH, ficlInteger);
FTH		fth_object_cycle_ref(FTH);
FTH		fth_object_cycle_set(FTH, FTH);
/* stack access */
FTH		ficl_to_fth(FTH);
FTH		fth_pop_ficl_cell(ficlVm *);
void		fth_push_ficl_cell(ficlVm *, FTH);
FTH		fth_to_ficl(FTH);

/* === port.c === */
/* port */
void		fth_port_close(FTH);
void		fth_port_display(FTH, FTH);
void		fth_port_flush(FTH);
int		fth_port_getc(FTH);
char           *fth_port_gets(FTH);
void		fth_port_putc(FTH, int);
void		fth_port_puts(FTH, const char *);
out_cb		fth_set_error_cb(out_cb);
out_cb		fth_set_print_and_error_cb(out_cb);
/* output callbacks */
/* void (*)(ficlVm *, char *); */
out_cb		fth_set_print_cb(out_cb);
/* input callback */
/* char *(*)(ficlVm *); */
in_cb		fth_set_read_cb(in_cb);
FTH		fth_port_to_string(FTH);

/* === printf.c === */
int		fth_asprintf(char **, const char *,...);
int		fth_debug(const char *,...);
int		fth_error(const char *);
int		fth_errorf(const char *,...);
char           *fth_format(const char *,...);
int		fth_fprintf(FILE *, const char *,...);
int		fth_ioprintf(FTH, const char *,...);
int		fth_port_printf(FTH, const char *,...);
int		fth_port_vprintf(FTH, const char *, va_list);
int		fth_print(const char *);
int		fth_printf(const char *,...);
int		fth_snprintf(char *, size_t, const char *,...);
int		fth_sprintf(char *, const char *,...);
FTH		fth_string_vformat(const char *, FTH);
int		fth_vasprintf(char **, const char *, va_list);
int		fth_verrorf(const char *, va_list);
char           *fth_vformat(const char *, va_list);
int		fth_vfprintf(FILE *, const char *, va_list);
int		fth_vioprintf(FTH, const char *, va_list);
int		fth_vprintf(const char *, va_list);
int		fth_vsnprintf(char *, size_t, const char *, va_list);
int		fth_vsprintf(char *, const char *, va_list);
int		fth_warning(const char *,...);

/* === proc.c === */
int		fth_word_defined_p(FTH);
int		fth_word_type_p(FTH, int);
/* proc */
ficlWord       *fth_define_procedure(const char *,
		    FTH (*) (), int, int, int, const char *);
ficlWord       *fth_define_void_procedure(const char *,
		    void (*) (), int, int, int, const char *);
FTH		fth_documentation_ref(FTH);
void		fth_documentation_set(FTH, FTH);
FTH		fth_get_optarg(ficlInteger, FTH);
FTH		fth_get_optkey(FTH, FTH);
ficl2Integer	fth_get_optkey_2int(FTH, ficl2Integer);
ficlInteger	fth_get_optkey_int(FTH, ficlInteger);
int		fth_get_optkey_fix(FTH, int);
char           *fth_get_optkey_str(FTH, char *);
FTH		fth_make_proc(ficlWord *, int, int, int);
FTH		fth_make_proc_from_func(const char *,
		    FTH (*) (), int, int, int, int);
FTH		fth_proc_apply(FTH, FTH, const char *);
int		fth_proc_arity(FTH);
FTH		fth_proc_call(FTH, const char *, int,...);
char           *fth_proc_name(FTH);
FTH		fth_proc_source_ref(FTH);
void		fth_proc_source_set(FTH, FTH);
ficlWord       *fth_proc_to_xt(FTH);
FTH		fth_source_file(FTH);
FTH		fth_source_line(FTH);
FTH		fth_source_ref(FTH);
void		fth_source_set(FTH, FTH);
ficlWord       *fth_word_doc_set(ficlWord *, const char *);
FTH		fth_xt_apply(const char *, FTH, const char *);
FTH		fth_xt_call(const char *, const char *, int,...);
FTH		proc_from_proc_or_xt(FTH, int, int, int);
/* define, variable */
FTH		fth_define (const char *, FTH);
FTH		fth_define_constant(const char *, FTH, const char *);
FTH		fth_define_variable(const char *, FTH, const char *);
int		fth_defined_p(const char *);
void		fth_trace_var(FTH, FTH);
FTH		fth_trace_var_execute(ficlWord *);
void		fth_untrace_var(FTH);
FTH		fth_var_ref(FTH);
FTH		fth_var_set(FTH, FTH);
FTH		fth_variable_ref(const char *);
FTH		fth_variable_set(const char *, FTH);

/* === regexp.c === */
/* regexp */
FTH		fth_make_regexp(const char *);
int		fth_regexp_find(const char *, const char *);
ficlInteger	fth_regexp_match(FTH, FTH);
FTH		fth_regexp_replace(FTH, FTH, FTH);
ficlInteger	fth_regexp_search(FTH, FTH, ficlInteger, ficlInteger);
FTH		fth_regexp_var_ref(ficlInteger);

/* === string.c === */
/* string */
FTH		fth_make_empty_string(void);
FTH		fth_make_string(const char *);
FTH		fth_make_string_format(const char *,...);
FTH		fth_make_string_len(const char *, ficlInteger);
FTH		fth_make_string_or_false(const char *);
FTH		fth_string_append(FTH, FTH);
FTH		fth_string_capitalize(FTH);
FTH		fth_string_char_ref(FTH, ficlInteger);
FTH		fth_string_char_set(FTH, ficlInteger, FTH);
FTH		fth_string_chomp(FTH);
FTH		fth_string_copy(FTH);
char		fth_string_c_char_ref(FTH, ficlInteger);
char		fth_string_c_char_set(FTH, ficlInteger, char);
FTH		fth_string_delete(FTH, ficlInteger);
FTH		fth_string_downcase(FTH);
int		fth_string_equal_p(FTH, FTH);
int		fth_string_eval(FTH);
FTH		fth_string_fill(FTH, FTH);
FTH		fth_string_find(FTH, FTH);
FTH		fth_string_format(FTH, FTH);
int		fth_string_greater_p(FTH, FTH);
FTH		fth_string_index(FTH, FTH);
FTH		fth_string_insert(FTH, ficlInteger, FTH);
ficlInteger	fth_string_length(FTH);
int		fth_string_less_p(FTH, FTH);
int		fth_string_member_p(FTH, FTH);
int		fth_string_not_equal_p(FTH, FTH);
FTH		fth_string_pop(FTH);
FTH		fth_string_push(FTH, FTH);
char           *fth_string_ref(FTH);
FTH		fth_string_replace(FTH, FTH, FTH);
FTH		fth_string_reverse(FTH);
FTH		fth_string_scat(FTH, const char *);
FTH		fth_string_sformat(FTH, const char *,...);
FTH		fth_string_shift(FTH);
FTH		fth_string_sncat(FTH, const char *, ficlInteger);
FTH		fth_string_split(FTH, FTH);
FTH		fth_string_substring(FTH, ficlInteger, ficlInteger);
FTH		fth_string_to_array(FTH);
FTH		fth_string_unshift(FTH, FTH);
FTH		fth_string_upcase(FTH);
FTH		fth_string_vsformat(FTH, const char *, va_list);

/* === symbol.c === */
/* symbol */
int		fth_string_or_symbol_p(FTH);
char           *fth_string_or_symbol_ref(FTH);
FTH		fth_symbol (const char *);
int		fth_symbol_equal_p(FTH, FTH);
int		fth_symbol_p(const char *);
char           *fth_symbol_ref(FTH);
/* keyword */
FTH		fth_keyword(const char *);
int		fth_keyword_equal_p(FTH, FTH);
int		fth_keyword_p(const char *);
char           *fth_keyword_ref(FTH);
/* exception */
FTH		fth_exception(const char *);
int		fth_exception_equal_p(FTH, FTH);
FTH		fth_exception_last_message_ref(FTH);
void		fth_exception_last_message_set(FTH, FTH);
FTH		fth_exception_message_ref(FTH);
void		fth_exception_message_set(FTH, FTH);
char           *fth_exception_ref(FTH);
FTH		fth_make_exception(const char *, const char *);
int		fth_symbol_or_exception_p(FTH);
FTH		fth_symbol_or_exception_ref(FTH);
FTH		fth_symbol_to_exception(FTH);

/* === utils.c === */
void           *fth_calloc(size_t, size_t);
int		fth_evaluate(ficlVm *, const char *);
int		fth_execute_xt(ficlVm *, ficlWord *);
void		fth_free(void *);
char           *fth_getenv(const char *, char *);
void           *fth_malloc(size_t);
void           *fth_realloc(void *, size_t);
void		fth_repl(int, char **);
char	       *fth_strcat(char *, size_t, const char *);
char	       *fth_strcpy(char *, size_t, const char *);
char           *fth_strdup(const char *);
char           *fth_strerror(int);
size_t		fth_strlen(const char *);
char	       *fth_strncat(char *, size_t, const char *, size_t);
char	       *fth_strncpy(char *, size_t, const char *, size_t);
void		fth_throw (FTH, const char *,...);
void		fth_throw_error(FTH, FTH);
void		fth_throw_list(FTH, FTH);
char	       *pop_cstring(ficlVm *);
void		push_cstring(ficlVm *, char *);
FTH		fth_set_argv(int, int, char **);

__END_DECLS

#endif				/* _FTH_H_ */

/*
 * fth.h ends here
 */
