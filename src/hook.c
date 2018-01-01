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
 * @(#)hook.c	1.85 1/1/18
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "fth.h"
#include "utils.h"

/* === HOOK === */

static FTH	hook_tag;

typedef struct {
	char           *name;	/* hook name */
	simple_array   *data;	/* array of hook functions */
	int		req;	/* required arguments */
	int		opt;	/* optional arguments */
	int		rest;	/* rest arguments */
} FHook;

#define FTH_HOOK_OBJECT(Obj)	FTH_INSTANCE_REF_GEN(Obj, FHook)
#define FTH_HOOK_NAME(Obj)	FTH_HOOK_OBJECT(Obj)->name
#define FTH_HOOK_DATA(Obj)	FTH_HOOK_OBJECT(Obj)->data
#define FTH_HOOK_REQ(Obj)	FTH_HOOK_OBJECT(Obj)->req
#define FTH_HOOK_OPT(Obj)	FTH_HOOK_OBJECT(Obj)->opt
#define FTH_HOOK_REST(Obj)	FTH_HOOK_OBJECT(Obj)->rest
#define FTH_HOOK_LENGTH(Obj)	simple_array_length(FTH_HOOK_DATA(Obj))

static void	ficl_create_hook(ficlVm *);
static void	ficl_hook_apply(ficlVm *);
static void	ficl_hook_arity(ficlVm *);
static void	ficl_hook_empty_p(ficlVm *);
static void	ficl_hook_equal_p(ficlVm *);
static void	ficl_hook_member_p(ficlVm *);
static void	ficl_hook_name(ficlVm *);
static void	ficl_hook_p(ficlVm *);
static void	ficl_make_hook(ficlVm *);
static void	ficl_print_hook(ficlVm *);
static FTH	hk_apply(FTH, FTH);
static FTH	hk_dump(FTH);
static FTH	hk_equal_p(FTH, FTH);
static void	hk_free(FTH);
static FTH	hk_inspect(FTH);
static FTH	hk_length(FTH);
static FTH	hk_ref(FTH, FTH);
static FTH	hk_to_array(FTH);
static FTH	hk_to_string(FTH);
static FTH	make_hook(const char *, int, int, int, const char *);

#define h_list_of_hook_functions "\
*** HOOK PRIMITIVES ***\n\
.hook               ( hook -- )\n\
add-hook! alias for hook-add\n\
create-hook         ( arity help \"name\" -- )\n\
hook->array         ( hook -- ary )\n\
hook->list alias for hook->array\n\
hook-add            ( hook prc -- )\n\
hook-apply          ( hook args -- value-list )\n\
hook-arity          ( hook -- arity )\n\
hook-clear          ( hook -- )\n\
hook-delete         ( hook prc-or-name -- prc )\n\
hook-empty?         ( hook -- f )\n\
hook-member?        ( hook prc-or-name -- f )\n\
hook-name           ( hook -- name )\n\
hook-names          ( hook -- name-list )\n\
hook-procs alias for hook->array\n\
hook=               ( obj1 obj2 -- f )\n\
hook?               ( obj -- f )\n\
make-hook           ( arity -- hook )\n\
remove-hook! alias for hook-delete\n\
reset-hook! alias for hook-clear\n\
run-hook alias for hook-apply"

#define FTH_HOOK_REST_STR(hook) (FTH_HOOK_REST(hook) ? "#t" : "#f")

static FTH
hk_inspect(FTH self)
{
	int len, i;
	FTH fs, prc;

	len = FTH_HOOK_LENGTH(self);
	fs = fth_make_string_format("%s %s: %d/%d/%s, procs[%d]",
	    FTH_INSTANCE_NAME(self),
	    FTH_HOOK_NAME(self),
	    FTH_HOOK_REQ(self),
	    FTH_HOOK_OPT(self),
	    FTH_HOOK_REST_STR(self),
	    len);
	if (len > 0) {
		fth_string_sformat(fs, ":");
		for (i = 0; i < len; i++) {
			prc = (FTH)simple_array_ref(FTH_HOOK_DATA(self), i);
			fth_string_sformat(fs, " %s", fth_proc_name(prc));
		}
	}
	return (fs);
}

static FTH
hk_to_string(FTH self)
{
	return (fth_make_string(FTH_HOOK_NAME(self)));
}

static FTH
hk_dump(FTH self)
{
	int len, i;
	FTH fs, prc;
	ficlWord *word;

	fs = fth_make_string_format("\\ Doesn't work with lambda: words!\n");
	fth_string_sformat(fs, "[ifundef] %s\n", FTH_HOOK_NAME(self));
	fth_string_sformat(fs, "\t#( %d %d %s ) \"no doc\" create-hook %s\n",
	    FTH_HOOK_REQ(self),
	    FTH_HOOK_OPT(self),
	    FTH_HOOK_REST_STR(self),
	    FTH_HOOK_NAME(self));
	fth_string_sformat(fs, "[then]\n");
	len = FTH_HOOK_LENGTH(self);
	for (i = 0; i < len; i++) {
		prc = (FTH)simple_array_ref(FTH_HOOK_DATA(self), i);
		word = FICL_WORD_REF(prc);
		if (word->length > 0)
			fth_string_sformat(fs, "%s <'> %s add-hook!\n",
			    FTH_HOOK_NAME(self), word->name);
	}
	return (fs);
}

/* return array of procs */
static FTH
hk_to_array(FTH self)
{
	return (simple_array_to_array(FTH_HOOK_DATA(self)));
}

static FTH
hk_ref(FTH self, FTH idx)
{
	FTH prc;

	prc = (FTH)simple_array_ref(FTH_HOOK_DATA(self), FIX_TO_INT32(idx));
	return (prc);
}

static FTH
hk_equal_p(FTH self, FTH obj)
{
	int flag;

	flag = FTH_HOOK_REQ(self) == FTH_HOOK_REQ(obj) &&
	    FTH_HOOK_OPT(self) == FTH_HOOK_OPT(obj) &&
	    FTH_HOOK_REST(self) == FTH_HOOK_REST(obj) &&
	    simple_array_equal_p(FTH_HOOK_DATA(self), FTH_HOOK_DATA(obj));
	return (BOOL_TO_FTH(flag));
}

static FTH
hk_length(FTH self)
{
	int len;

	len = FTH_HOOK_LENGTH(self);
	return (fth_make_int((ficlInteger)len));
}

static void
hk_free(FTH self)
{
	FTH_FREE(FTH_HOOK_NAME(self));
	simple_array_free(FTH_HOOK_DATA(self));
	FTH_FREE(FTH_HOOK_OBJECT(self));
}

static FTH
hk_apply(FTH self, FTH args)
{
	return (fth_hook_apply(self, args, RUNNING_WORD()));
}

static void
ficl_hook_p(ficlVm *vm)
{
#define h_hook_p "( obj -- f )  test if OBJ is a hook\n\
2 make-hook hook? => #t\n\
nil         hook? => #f\n\
Return #t if OBJ is a hook object, otherwise #f."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, FTH_HOOK_P(obj));
}

static FTH
make_hook(const char *name, int req, int opt, int rest, const char *doc)
{
	FHook *hk;
	FTH hook;

	hk = FTH_MALLOC(sizeof(FHook));
	hk->name = FTH_STRDUP(name);
	hk->req = req;
	hk->opt = opt;
	hk->rest = rest;
	hk->data = make_simple_array(8);
	hook = fth_make_instance(hook_tag, hk);
	fth_word_doc_set(ficlDictionaryAppendConstant(FTH_FICL_DICT(),
	    (char *)hk->name,
	    (ficlInteger)hook),
	    doc);
	return (hook);
}

FTH
fth_make_hook_with_arity(const char *name, int req, int opt, int rest,
    const char *doc)
{
	return (make_hook(name, req, opt, rest, doc));
}

/*
 * Return a Hook object called NAME with ARITY required arguments, 0
 * optional arguments and no rest arguments.  An optional documentation
 * DOC can be provided.
 */
FTH
fth_make_hook(const char *name, int arity, const char *doc)
{
	return (make_hook(name, arity, 0, 0, doc));
}

static int	simple_hook_number = 0;

/*
 * Return a Hook object with ARITY required arguments, 0 optional
 * arguments and no rest arguments.
 */
FTH
fth_make_simple_hook(int arity)
{
	char *name;
	FTH hk;

	name = fth_format("simple-%02d-hook", simple_hook_number++);
	hk = make_hook(name, arity, 0, 0, NULL);
	FTH_FREE(name);
	return (hk);
}

static void
ficl_create_hook(ficlVm *vm)
{
#define h_create_hook "( arity help \"name\" -- )  create hook\n\
2 \"A simple hook.\" create-hook my-new-hook\n\
#( 2 0 #f ) \"A simple hook.\" create-hook my-new-hook\n\
my-new-hook <'> + 2 make-proc add-hook!\n\
my-new-hook #( 2 3 ) run-hook => #( 5 )\n\
my-new-hook      => #<hook my-new-hook: 2/0/0, procs[1]: +>\n\
help my-new-hook => A simple hook.\n\
Create hook variable NAME with ARITY and documentation HELP.  \
ARITY can be an integer or an array of length 3, #( req opt rest ).\n\
See also make-hook."
	FTH arity;
	char *help;
	int req, opt, rest;

	FTH_STACK_CHECK(vm, 2, 0);
	ficlVmGetWordToPad(vm);
	help = pop_cstring(vm);
	arity = fth_pop_ficl_cell(vm);
	opt = 0;
	rest = 0;
	if (fth_array_length(arity) == 3) {
		req = FIX_TO_INT32(fth_array_fast_ref(arity, 0L));
		opt = FIX_TO_INT32(fth_array_fast_ref(arity, 1L));
		rest = FTH_TO_BOOL(fth_array_fast_ref(arity, 2L));
	} else
		req = FIX_TO_INT32(arity);
	make_hook(vm->pad, req, opt, rest, help);
}

static void
ficl_make_hook(ficlVm *vm)
{
#define h_make_hook "( arity -- hk )  create hook\n\
2 make-hook value my-new-hook\n\
#( 2 0 #f ) make-hook value my-new-hook\n\
my-new-hook <'> + 2 make-proc add-hook!\n\
my-new-hook #( 2 3 ) run-hook => #( 5 )\n\
my-new-hook => #<hook simple-00-hook: 2/0/0, procs[1]: +>\n\
Return hook object for procs accepting ARITY arguments.  \
ARITY can be an integer or an array of length 3, #( req opt rest ).\n\
See also create-hook."
	FTH arity;
	char *name;
	int req, opt, rest;

	FTH_STACK_CHECK(vm, 1, 1);
	arity = fth_pop_ficl_cell(vm);
	opt = 0;
	rest = 0;
	if (fth_array_length(arity) == 3) {
		req = FIX_TO_INT32(fth_array_fast_ref(arity, 0L));
		opt = FIX_TO_INT32(fth_array_fast_ref(arity, 1L));
		rest = FTH_TO_BOOL(fth_array_fast_ref(arity, 2L));
	} else
		req = FIX_TO_INT32(arity);
	name = fth_format("simple-%02d-hook", simple_hook_number++);
	ficlStackPushFTH(vm->dataStack, make_hook(name, req, opt, rest, NULL));
	FTH_FREE(name);
}

static void
ficl_print_hook(ficlVm *vm)
{
#define h_print_hook "( hk -- )  print hook\n\
2 make-hook .hook => hook simple-01-hook: 2/0/#f, procs[0]\n\
Print hook object HK to current output."
	FTH hook;

	FTH_STACK_CHECK(vm, 1, 0);
	hook = ficlStackPopFTH(vm->dataStack);
	FTH_ASSERT_ARGS(FTH_HOOK_P(hook), hook, FTH_ARG1, "a hook");
	fth_print(fth_string_ref(hk_inspect(hook)));
}

int
fth_hook_equal_p(FTH obj1, FTH obj2)
{
	if (FTH_HOOK_P(obj1) && FTH_HOOK_P(obj2))
		return (FTH_TO_BOOL(hk_equal_p(obj1, obj2)));
	return (0);
}

static void
ficl_hook_equal_p(ficlVm *vm)
{
#define h_hook_equal_p "( hk1 hk2 -- f )  compare hooks\n\
2 make-hook value hk1\n\
2 make-hook value hk2\n\
3 make-hook value hk3\n\
hk1 hk2 hook= => #t\n\
hk1 hk3 hook= => #f\n\
Return #t if HK1 and HK2 are hook objects of same arity and procedures, \
otherwise #f."
	FTH obj1, obj2;

	FTH_STACK_CHECK(vm, 2, 1);
	obj2 = fth_pop_ficl_cell(vm);
	obj1 = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, fth_hook_equal_p(obj1, obj2));
}

/*
 * Return array of all HOOK procedures.
 */
FTH
fth_hook_to_array(FTH hook)
{
#define h_hook_to_array "( hook -- ary )  return hook procedures\n\
2 make-hook value hk1\n\
hk1  <'> + 2 make-proc  add-hook!\n\
hk1 hook->array => #( + )\n\
Return array of all HOOK procedures."
	FTH_ASSERT_ARGS(FTH_HOOK_P(hook), hook, FTH_ARG1, "a hook");
	return (hk_to_array(hook));
}

int
fth_hook_arity(FTH hook)
{
	if (FTH_HOOK_P(hook))
		return (FTH_HOOK_REQ(hook));
	return (0);
}

static void
ficl_hook_arity(ficlVm *vm)
{
#define h_hook_arity "( hook -- arity )  return hook arity\n\
2 make-hook hook-arity => #( 2 0 #f )\n\
Return arity array of HOOK, #( req opt rest )."
	FTH hook, arity;

	FTH_STACK_CHECK(vm, 1, 1);
	hook = ficlStackPopFTH(vm->dataStack);
	FTH_ASSERT_ARGS(FTH_HOOK_P(hook), hook, FTH_ARG1, "a hook");
	arity = fth_make_array_var(3,
	    INT_TO_FIX(FTH_HOOK_REQ(hook)),
	    INT_TO_FIX(FTH_HOOK_OPT(hook)),
	    BOOL_TO_FTH(FTH_HOOK_REST(hook)));
	ficlStackPushFTH(vm->dataStack, arity);
}

static void
ficl_hook_name(ficlVm *vm)
{
#define h_hook_name "( obj -- name|#f )  return name of OBJ\n\
2 make-hook hook-name => \"simple-01-hook\"\n\
Return name of OBJ as string if hook object, otherwise #f."
	FTH hook;

	FTH_STACK_CHECK(vm, 1, 1);
	hook = ficlStackPopFTH(vm->dataStack);
	if (FTH_HOOK_P(hook))
		push_cstring(vm, FTH_HOOK_NAME(hook));
	else
		ficlStackPushBoolean(vm->dataStack, 0);
}

/*
 * Add hook procedure PROC_OR_XT to HOOK.  Raise BAD_ARITY exception
 * if PROC_OR_XT's arity doesn't match HOOK's arity.
 */
void
fth_add_hook(FTH hook, FTH proc_or_xt)
{
#define h_add_hook "( hook proc-or-xt -- )  add PROC-OR-XT to HOOK\n\
2 make-hook value hk1\n\
<'> + 2 make-proc value prc\n\
hk1 prc add-hook!\n\
hk1 hook-names => #( \"+\" )\n\
Add hook procedure PROC-OR-XT to HOOK.  \
Raise BAD-ARITY exception if PROC-OR-XT's arity doesn't match HOOK's arity."
	FTH 		proc;

	FTH_ASSERT_ARGS(FTH_HOOK_P(hook), hook, FTH_ARG1, "a hook");
	proc = proc_from_proc_or_xt(proc_or_xt,
	    FTH_HOOK_REQ(hook),
	    FTH_HOOK_OPT(hook),
	    FTH_HOOK_REST(hook));

	if (!FTH_PROC_P(proc)) {
		FTH_ASSERT_ARGS(FTH_PROC_P(proc), proc, FTH_ARG2,
		    "a proc or xt");
		/* NOTREACED */
		return;
	}
	if (FTH_HOOK_REQ(hook) == FICL_WORD_REQ(proc) &&
	    FTH_HOOK_OPT(hook) == FICL_WORD_OPT(proc) &&
	    FTH_HOOK_REST(hook) == FICL_WORD_REST(proc))
		simple_array_push(FTH_HOOK_DATA(hook), (void *) proc);
	else
		FTH_BAD_ARITY_ERROR_ARGS(FTH_ARG2, proc,
		    FTH_HOOK_REQ(hook),
		    FTH_HOOK_OPT(hook),
		    FTH_HOOK_REST(hook),
		    FICL_WORD_REQ(proc),
		    FICL_WORD_OPT(proc),
		    FICL_WORD_REST(proc));
}

FTH
fth_remove_hook(FTH hook, FTH proc_or_name)
{
#define h_remove_hook "( hook proc-or-name -- proc|#f )  remove PROC-OR-NAME\n\
2 make-hook value hk1\n\
hk1  <'> + 2 make-proc  add-hook!\n\
hk1 \"+\" remove-hook!\n\
hk1 <'> + remove-hook!\n\
Remove hook procedure PROC-OR-NAME from HOOK and return it.  \
PROC-OR-NAME can be a string, an xt or a proc."
	char *name;
	ficlWord *word;

	FTH_ASSERT_ARGS(FTH_HOOK_P(hook), hook, FTH_ARG1, "a hook");
	if (FICL_WORD_P(proc_or_name)) {
		word = FICL_WORD_REF(proc_or_name);
		return ((FTH)simple_array_delete(FTH_HOOK_DATA(hook), word));
	}
	name = fth_string_ref(proc_or_name);
	if (name == NULL)
		return (FTH_FALSE);
	word = FICL_WORD_NAME_REF(name);
	if (word == NULL)
		return (FTH_FALSE);
	return ((FTH)simple_array_delete(FTH_HOOK_DATA(hook), word));
}

int
fth_hook_member_p(FTH hook, FTH proc_or_name)
{
	char *name;
	ficlWord *word;

	FTH_ASSERT_ARGS(FTH_HOOK_P(hook), hook, FTH_ARG1, "a hook");
	if (FICL_WORD_P(proc_or_name)) {
		word = FICL_WORD_REF(proc_or_name);
		return (simple_array_member_p(FTH_HOOK_DATA(hook), word));
	}
	name = fth_string_ref(proc_or_name);
	if (name == NULL)
		return (0);
	word = FICL_WORD_NAME_REF(name);
	if (word == NULL)
		return (0);
	return (simple_array_member_p(FTH_HOOK_DATA(hook), word));
}

static void
ficl_hook_member_p(ficlVm *vm)
{
#define h_hook_member_p "( hook proc-or-name -- f )  find PROC-OR-NAME\n\
2 make-hook value hk1\n\
hk1  <'> + 2 make-proc  add-hook!\n\
hook \"+\" hook-member? => #t\n\
hook <'> + hook-member? => #t\n\
Return #t if procedure PROC-OR-NAME exist in HOOK, otherwise #f.  \
PROC-OR-NAME can be a string, an xt or a proc."
	FTH hook, proc_or_name;
	int flag;

	FTH_STACK_CHECK(vm, 2, 1);
	proc_or_name = fth_pop_ficl_cell(vm);
	hook = ficlStackPopFTH(vm->dataStack);
	flag = fth_hook_member_p(hook, proc_or_name);
	ficlStackPushBoolean(vm->dataStack, flag);
}

/*
 * Return true if no hook procedure exist in HOOK, otherwise false.
 */
int
fth_hook_empty_p(FTH hook)
{
	if (FTH_HOOK_P(hook))
		return (FTH_HOOK_LENGTH(hook) == 0);
	return (1);
}

static void
ficl_hook_empty_p(ficlVm *vm)
{
#define h_hook_empty_p "( hook -- f )  test if HOOK is empty\n\
2 make-hook value hk1\n\
hk1  <'> + 2 make-proc  add-hook!\n\
hk1 hook-empty? => #f\n\
hk1 <'> + remove-hook! => +\n\
hk1 hook-empty? => #t\n\
Return #t if no hook procedure exist in HOOK, otherwise #f."
	FTH hook;
	int flag;

	FTH_STACK_CHECK(vm, 1, 1);
	hook = ficlStackPopFTH(vm->dataStack);
	flag = fth_hook_empty_p(hook);
	ficlStackPushBoolean(vm->dataStack, flag);
}

/*
 * fth_run_hook(hook, num-of-args, ...)
 *
 * hook: hook object
 *  len: number of arguments, must equal arity
 *  ...: len args
 *
 * return array of results of all hook-procedures
 */
FTH
fth_run_hook(FTH hook, int len,...)
{
	ficlInteger i;
	FTH args;
	va_list list;

	FTH_ASSERT_ARGS(FTH_HOOK_P(hook), hook, FTH_ARG1, "a hook");
	if (FTH_HOOK_REQ(hook) > len)
		FTH_BAD_ARITY_ERROR_ARGS(FTH_ARG1, hook, len, 0, 0,
		    FTH_HOOK_REQ(hook),
		    FTH_HOOK_OPT(hook),
		    FTH_HOOK_REST(hook));
	args = fth_make_array_len((ficlInteger)len);
	va_start(list, len);
	for (i = 0; i < len; i++)
		fth_array_fast_set(args, i, va_arg(list, FTH));
	va_end(list);
	return (fth_hook_apply(hook, args, RUNNING_WORD()));
}

/*
 * For concatenating strings; the return value of a proc_n is set as
 * index 0 of the input args-list for proc_n+1.
 *
 * args = #( name pos )
 *
 * for (i = 0; i < len; i++)
 *   fth_array_set(args, 0, fth_proc_apply(proc, args, func))
 */
FTH
fth_run_hook_again(FTH hook, int len,...)
{
	FTH args, prc, res;
	int i;
	va_list list;

	FTH_ASSERT_ARGS(FTH_HOOK_P(hook), hook, FTH_ARG1, "a hook");
	if (FTH_HOOK_REQ(hook) > len)
		FTH_BAD_ARITY_ERROR_ARGS(FTH_ARG1, hook, len, 0, 0,
		    FTH_HOOK_REQ(hook),
		    FTH_HOOK_OPT(hook),
		    FTH_HOOK_REST(hook));
	args = fth_make_array_len((ficlInteger)len);
	va_start(list, len);
	for (i = 0; i < len; i++)
		fth_array_fast_set(args, (ficlInteger)i, va_arg(list, FTH));
	va_end(list);
	for (i = 0; i < FTH_HOOK_LENGTH(hook); i++) {
		prc = (FTH)simple_array_ref(FTH_HOOK_DATA(hook), i);
		res = fth_proc_apply(prc, args, RUNNING_WORD());
		fth_array_fast_set(args, 0L, res);
	}
	return (fth_array_ref(args, 0L));
}

/*
 * Return #t if hook is empty or any of the procs didn't return #f,
 * otherwise #f.
 */
FTH
fth_run_hook_bool(FTH hook, int len,...)
{
	FTH args, prc, res, ret;
	int i;
	va_list list;

	FTH_ASSERT_ARGS(FTH_HOOK_P(hook), hook, FTH_ARG1, "a hook");
	if (FTH_HOOK_REQ(hook) > len)
		FTH_BAD_ARITY_ERROR_ARGS(FTH_ARG1, hook, len, 0, 0,
		    FTH_HOOK_REQ(hook),
		    FTH_HOOK_OPT(hook),
		    FTH_HOOK_REST(hook));
	args = fth_make_array_len((ficlInteger)len);
	va_start(list, len);
	for (i = 0; i < len; i++)
		fth_array_fast_set(args, (ficlInteger)i, va_arg(list, FTH));
	va_end(list);
	ret = FTH_TRUE;
	for (i = 0; i < FTH_HOOK_LENGTH(hook); i++) {
		prc = (FTH)simple_array_ref(FTH_HOOK_DATA(hook), i);
		res = fth_proc_apply(prc, args, RUNNING_WORD());
		if (FTH_FALSE_P(res))
			ret = FTH_FALSE;
	}
	return (ret);
}

/*
 * fth_hook_apply
 * returns array of results of all hook-procedures
 */
FTH
fth_hook_apply(FTH hook, FTH args, const char *caller)
{
	FTH prc, res, ret;
	int i, len;

	FTH_ASSERT_ARGS(FTH_HOOK_P(hook), hook, FTH_ARG1, "a hook");
	len = FTH_HOOK_LENGTH(hook);
	ret = fth_make_array_len((ficlInteger)len);
	for (i = 0; i < len; i++) {
		prc = (FTH)simple_array_ref(FTH_HOOK_DATA(hook), i);
		res = fth_proc_apply(prc, args, caller);
		fth_array_fast_set(ret, (ficlInteger)i, res);
	}
	return (ret);
}

static void
ficl_hook_apply(ficlVm *vm)
{
#define h_hook_apply "( hook args -- value-list )  run procs of HOOK\n\
2 make-hook value hk1\n\
hk1  <'> + 2 make-proc  add-hook!\n\
hk1 #( 1 2 ) run-hook => #( 3 )\n\
Run all hook procedures with ARGS, an array of arguments.  \
ARGS can be an array of arguments or a single argument.  \
Raise BAD-ARITY exception if ARGS's length doesn't match HOOK's arity."
	FTH hook, res, args;

	FTH_STACK_CHECK(vm, 2, 1);
	args = fth_pop_ficl_cell(vm);
	hook = ficlStackPopFTH(vm->dataStack);
	if (!FTH_ARRAY_P(args))
		args = fth_make_empty_array();
	res = fth_hook_apply(hook, args, RUNNING_WORD_VM(vm));
	ficlStackPushFTH(vm->dataStack, res);
}

/*
 * Remove all hook procedures from HOOK.
 */
void
fth_hook_clear(FTH hook)
{
#define h_hook_clear "( hook -- )  clear hook\n\
2 make-hook value hk1\n\
hk1 hook-clear\n\
Remove all hook procedures from HOOK."
	FTH_ASSERT_ARGS(FTH_HOOK_P(hook), hook, FTH_ARG1, "a hook");
	simple_array_clear(FTH_HOOK_DATA(hook));
}

FTH
fth_hook_names(FTH hook)
{
#define h_hook_names "( hook -- name-list )  return proc-names\n\
2 make-hook value hk1\n\
hk1  <'> + 2 make-proc  add-hook!\n\
hk1 hook-names => #( \"+\" )\n\
Return array of hook procedure names (strings)."
	FTH names, prc, fs;
	int i, len;

	FTH_ASSERT_ARGS(FTH_HOOK_P(hook), hook, FTH_ARG1, "a hook");
	len = FTH_HOOK_LENGTH(hook);
	names = fth_make_array_len((ficlInteger)len);
	for (i = 0; i < len; i++) {
		prc = (FTH)simple_array_ref(FTH_HOOK_DATA(hook), i);
		fs = fth_make_string(fth_proc_name(prc));
		fth_array_fast_set(names, (ficlInteger)i, fs);
	}
	return (names);
}

void
init_hook_type(void)
{
	/* init hook */
	hook_tag = make_object_type(FTH_STR_HOOK, FTH_HOOK_T);
	fth_set_object_inspect(hook_tag, hk_inspect);
	fth_set_object_to_string(hook_tag, hk_to_string);
	fth_set_object_dump(hook_tag, hk_dump);
	fth_set_object_to_array(hook_tag, hk_to_array);
	fth_set_object_value_ref(hook_tag, hk_ref);
	fth_set_object_equal_p(hook_tag, hk_equal_p);
	fth_set_object_length(hook_tag, hk_length);
	fth_set_object_free(hook_tag, hk_free);
}

void
init_hook(void)
{
	fth_set_object_apply(hook_tag, (void *)hk_apply, 1, 0, 0);
	/* hook */
	FTH_PRI1("hook?", ficl_hook_p, h_hook_p);
	FTH_PRI1("create-hook", ficl_create_hook, h_create_hook);
	FTH_PRI1("make-hook", ficl_make_hook, h_make_hook);
	FTH_PRI1(".hook", ficl_print_hook, h_print_hook);
	FTH_PRI1("hook=", ficl_hook_equal_p, h_hook_empty_p);
	FTH_PROC("hook->array", fth_hook_to_array, 1, 0, 0, h_hook_to_array);
	FTH_PROC("hook-procs", fth_hook_to_array, 1, 0, 0, h_hook_to_array);
	FTH_PROC("hook->list", fth_hook_to_array, 1, 0, 0, h_hook_to_array);
	FTH_PRI1("hook-arity", ficl_hook_arity, h_hook_arity);
	FTH_PRI1("hook-name", ficl_hook_name, h_hook_name);
	FTH_VOID_PROC("hook-add", fth_add_hook, 2, 0, 0, h_add_hook);
	FTH_VOID_PROC("add-hook!", fth_add_hook, 2, 0, 0, h_add_hook);
	FTH_PROC("hook-delete", fth_remove_hook, 2, 0, 0, h_remove_hook);
	FTH_PROC("remove-hook!", fth_remove_hook, 2, 0, 0, h_remove_hook);
	FTH_PRI1("hook-member?", ficl_hook_member_p, h_hook_member_p);
	FTH_PRI1("hook-empty?", ficl_hook_empty_p, h_hook_empty_p);
	FTH_PRI1("hook-apply", ficl_hook_apply, h_hook_apply);
	FTH_PRI1("run-hook", ficl_hook_apply, h_hook_apply);
	FTH_VOID_PROC("hook-clear", fth_hook_clear, 1, 0, 0, h_hook_clear);
	FTH_VOID_PROC("reset-hook!", fth_hook_clear, 1, 0, 0, h_hook_clear);
	FTH_PROC("hook-names", fth_hook_names, 1, 0, 0, h_hook_names);
	FTH_ADD_FEATURE_AND_INFO(FTH_STR_HOOK, h_list_of_hook_functions);
}

/*
 * hook.c ends here
 */
