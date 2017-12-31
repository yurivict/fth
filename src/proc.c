/*-
 * Copyright (c) 2005-2017 Michael Scholz <mi-scholz@users.sourceforge.net>
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
 * @(#)proc.c	1.173 12/31/17
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "fth.h"
#include "utils.h"

#define FICL_WORD_DOC(Obj)						\
	fth_word_property_ref((FTH)(Obj), FTH_SYMBOL_DOCUMENTATION)
#define FICL_WORD_SRC(Obj)						\
	fth_word_property_ref((FTH)(Obj), FTH_SYMBOL_SOURCE)
#define FICL_WORD_FILE(Obj)						\
	FICL_WORD_REF(Obj)->file
#define FICL_WORD_LINE(Obj)						\
	FICL_WORD_REF(Obj)->line
#define FICL_WORD_CURRENT_WORD(Obj)					\
	FICL_WORD_REF(Obj)->current_word
#define FICL_WORD_CURRENT_FILE(Obj)					\
	FICL_WORD_REF(Obj)->current_file
#define FICL_WORD_CURRENT_LINE(Obj)					\
	FICL_WORD_REF(Obj)->current_line
#define FICL_WORD_CURRENT_NAME(Obj)					\
	FICL_WORD_NAME(FICL_WORD_CURRENT_WORD(Obj))

static char 	proc_scratch[FICL_PAD_SIZE];

static void 	ficl_proc_p(ficlVm *vm);
static void 	ficl_thunk_p(ficlVm *vm);
static void 	ficl_word_p(ficlVm *vm);
static void 	ficl_xt_p(ficlVm *vm);

static FTH	execute_proc(ficlVm *vm, ficlWord *word, int depth,
    const char *caller);
static void 	ficl_args_keys_paren_co(ficlVm *vm);
static void 	ficl_args_optional_paren_co(ficlVm *vm);
static void 	ficl_begin_definition(ficlVm *vm);
static void 	ficl_constant(ficlVm *vm);
static void 	ficl_defined_p(ficlVm *vm);
static void 	ficl_doc_quote_co_im(ficlVm *vm);
static void 	ficl_empty_extended_args_co_im(ficlVm *vm);
static void 	ficl_extended_args_co_im(ficlVm *vm);
static void 	ficl_filename_im(ficlVm *vm);
static void 	ficl_get_func_name_co_im(ficlVm *vm);
static void 	ficl_get_help(ficlVm *vm);
static void 	ficl_get_optarg(ficlVm *vm);
static void 	ficl_get_optargs(ficlVm *vm);
static void 	ficl_get_optkey(ficlVm *vm);
static void 	ficl_get_optkeys(ficlVm *vm);
static void 	ficl_help_add(ficlVm *vm);
static void 	ficl_help_ref(ficlVm *vm);
static void 	ficl_help_set(ficlVm *vm);
static void 	ficl_lambda_definition(ficlVm *vm);
static void 	ficl_latestxt(ficlVm *vm);
static void 	ficl_latestxt_co_im(ficlVm *vm);
static void 	ficl_lineno_im(ficlVm *vm);
static void 	ficl_local_variables_co_im(ficlVm *vm);
static void 	ficl_local_variables_paren_co(ficlVm *vm);
static void 	ficl_make_proc(ficlVm *vm);
static void 	ficl_print_proc(ficlVm *vm);
static void 	ficl_proc_apply(ficlVm *vm);
static void 	ficl_proc_arity(ficlVm *vm);
static void 	ficl_proc_create_co(ficlVm *vm);
static void 	ficl_proc_name(ficlVm *vm);
static void 	ficl_proc_to_xt(ficlVm *vm);
static void 	ficl_see(ficlVm *vm);
static void 	ficl_set_bang_im(ficlVm *vm);
static void 	ficl_set_execute(ficlVm *vm);
static void 	ficl_set_xt(ficlVm *vm);
static void 	ficl_tick_set_im(ficlVm *vm);
static void 	ficl_value(ficlVm *vm);
static void 	ficl_word_create_co(ficlVm *vm);
static FTH 	ficl_word_to_source(ficlDictionary *dict, ficlCell *cell);
static void 	ficl_xt_to_name(ficlVm *vm);
static void 	ficl_xt_to_origin(ficlVm *vm);
static char    *get_help(FTH obj, char *name);
static ficlString get_string_from_tib(ficlVm *vm, int pos);
static char    *help_formatted(char *str, int line_length);
static FTH 	local_vars_cb(FTH key, FTH value, FTH data);
static ficlWord *make_procedure(const char *name, void *func, int void_p,
    int req, int opt, int rest, const char *doc);
static void 	word_documentation(ficlVm *vm, ficlWord *word);

/* === PROC === */

#define h_list_of_proc_functions "\
*** PROC PRIMITIVES ***\n\
*filename*          ( -- name )\n\
*lineno*            ( -- n )\n\
.proc               ( proc -- )\n\
<'set>              ( \"name\" -- set-xt|#f )\n\
<{ ... }>           ( ?? -- ?? )\n\
defined?            ( \"name\" -- f )\n\
doc\"               ( <ccc>\" -- )\n\
documentation-ref   ( obj -- str )\n\
documentation-set!  ( obj str -- )\n\
get-func-name       ( -- name )\n\
get-optarg          ( req def -- val )\n\
get-optargs         ( lst req -- vals )\n\
get-optkey          ( key def -- val )\n\
get-optkeys         ( lst req -- vals )\n\
help                ( \"name\" -- )\n\
help-add!           ( obj str -- )\n\
help-ref            ( obj -- str )\n\
help-set!           ( obj str -- )\n\
lambda-create       ( -- )\n\
lambda:             ( -- xt )\n\
latestxt            ( -- xt )\n\
local-variables     ( -- vars )\n\
make-proc           ( xt arity -- proc )\n\
proc->xt            ( proc -- xt )\n\
proc-apply          ( proc args -- value )\n\
proc-arity          ( proc -- arity-list )\n\
proc-create         ( arity -- proc )\n\
proc-name           ( proc -- name )\n\
proc-source-ref     ( proc -- str )\n\
proc-source-set!    ( proc str -- )\n\
proc?               ( obj -- f )\n\
run-proc alias for proc-apply\n\
running-word        ( -- xt )\n\
see2                ( \"name\" -- )\n\
set!                ( \"name\" -- )\n\
set-execute         ( xt -- ?? )\n\
set-xt              ( xt -- set-xt|#f )\n\
source-file         ( xt -- file )\n\
source-line         ( xt -- line )\n\
source-ref          ( obj -- str )\n\
source-set!         ( obj str -- )\n\
thunk?              ( obj -- f )\n\
trace-var           ( variable-xt proc-or-xt -- )\n\
untrace-var         ( variable-xt -- )\n\
word?               ( obj -- f )\n\
xt->name            ( xt -- str )\n\
xt->origin          ( xt -- str )\n\
xt?                 ( obj -- f )"

/*-
 * Test if OBJ is an already defined word, a variable or constant
 * or any other object in the dictionary.
 *
 * FTH x = (FTH)FICL_WORD_NAME_REF("bye");
 * fth_word_defined_p(x);			=> true
 * fth_variable_set("hello", FTH_FALSE);
 * FTH x = (FTH)FICL_WORD_NAME_REF("hello");
 * fth_word_defined_p(x);			=> true
 * fth_word_defined_p(FTH_FALSE);		=> false
 */
int
fth_word_defined_p(FTH obj)
{
	if (obj == 0 || IMMEDIATE_P(obj))
		return (0);
	return (ficlDictionaryIncludes(FTH_FICL_DICT(), FICL_WORD_REF(obj)));
}

/*-
 * Test if OBJ is of TYPE where type can be:
 *	FW_WORD
 *	FW_PROC
 *	FW_SYMBOL
 *	FW_KEYWORD
 *	FW_EXCEPTION
 *	FW_VARIABLE
 *	FW_TRACE_VAR
 *
 * FTH x = (FTH)FICL_WORD_NAME_REF("bye");
 * fth_word_type_p(x, FW_WORD);			=> true
 * fth_word_type_p(x, FW_KEYWORD);		=> false
 */
int
fth_word_type_p(FTH obj, int type)
{
	return (FICL_WORD_DEFINED_P(obj) && FICL_WORD_TYPE(obj) == type);
}

static void
ficl_proc_p(ficlVm *vm)
{
#define h_proc_p "( obj -- f )  test if OBJ is a proc object\n\
<'> + proc? => #f\n\
<'> source-file proc? => #t\n\
Return #t if OBJ is a proc object, otherwise #f.\n\
See also thunk?, xt?, word?."
	FTH 		obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = ficlStackPopFTH(vm->dataStack);
	ficlStackPushBoolean(vm->dataStack, FTH_PROC_P(obj));
}

static void
ficl_thunk_p(ficlVm *vm)
{
#define h_thunk_p "( obj -- f )  test if OBJ is proc object\n\
<'> source-file thunk? => #f\n\
lambda: <{ -- }> ; thunk? => #t\n\
Return #t if OBJ is a proc object with arity #( 0 0 #f ), otherwise #f.\n\
See also proc?, xt?, word?."
	FTH 		obj;
	int 		flag;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = ficlStackPopFTH(vm->dataStack);
	flag = FTH_PROC_P(obj) &&
	    FICL_WORD_REQ(obj) == 0 &&
	    FICL_WORD_OPT(obj) == 0 &&
	    !FICL_WORD_REST(obj);
	ficlStackPushBoolean(vm->dataStack, flag);
}

static void
ficl_xt_p(ficlVm *vm)
{
#define h_xt_p "( obj -- f )  test if OBJ is an xt\n\
lambda: <{}> ; xt? => #f\n\
<'> make-array xt? => #t\n\
Return #t if OBJ is an xt (execution token, address of a Ficl word), \
otherwise #f.\n\
See also proc?, thunk?, word?."
	FTH 		obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = ficlStackPopFTH(vm->dataStack);
	ficlStackPushBoolean(vm->dataStack, FTH_WORD_P(obj));
}

static void
ficl_word_p(ficlVm *vm)
{
#define h_word_p "( obj -- f )  test if OBJ is proc or xt\n\
10 word? => #f\n\
lambda: <{}> ; word? => #t\n\
Return #t if OBJ is a proc object or xt (execution token, \
address of a Ficl word), otherwise #f.\n\
See also proc?, thunk?, xt?."
	FTH 		obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = ficlStackPopFTH(vm->dataStack);
	ficlStackPushBoolean(vm->dataStack, FICL_WORD_P(obj));
}

static ficlWord *
make_procedure(const char *name, void *func, int void_p,
    int req, int opt, int rest, const char *doc)
{
	ficlWord       *word;

	if (FICL_WORD_P(func))
		word = FICL_WORD_REF(func);
	else {
		FTH 		fnc;

		fnc = void_p ? ficl_execute_void_func : ficl_execute_func;
		word = ficlDictionaryAppendPrimitive(FTH_FICL_DICT(),
		    (char *) name, (ficlPrimitive) fnc, FICL_WORD_DEFAULT);
	}
	word->req = req;
	word->opt = opt;
	word->rest = rest;
	word->argc = req + opt + rest;
	word->kind = FW_PROC;
	if (void_p)
		word->vfunc = (void (*) ()) func;
	else
		word->func = (FTH (*) ()) func;
	if (doc != NULL && FTH_FALSE_P(FICL_WORD_DOC(word)))
		fth_word_doc_set(word, doc);
	return (word);
}

FTH
fth_make_proc(ficlWord *word, int req, int opt, int rest)
{
	if (word != NULL && FICL_WORD_P(word)) {
		ficlWord       *prc;
		char           *name;

		name = FICL_WORD_NAME(word);
		prc = make_procedure(name, word, 0, req, opt, rest, NULL);
		return ((FTH) prc);
	}
	FTH_ASSERT_ARGS(0, (FTH) word, FTH_ARG1, "an existing ficl word");
	return (FTH_FALSE);
}

FTH
fth_make_proc_from_func(const char *name,
    FTH (*func) (), int void_p, int req, int opt, int rest)
{
	ficlWord       *prc;

	prc = make_procedure(name, (void *) func, void_p, req, opt, rest, NULL);
	return ((FTH) prc);
}

/*
 * Return new ficlWord NAME tied to C function FUNC with REQ required
 * arguments, OPT optional arguments and REST (1) or no REST (0)
 * arguments with optional documentation string DOC.  FUNC takes zero
 * or more FTH objects and returns a FTH object.
 */
ficlWord       *
fth_define_procedure(const char *name,
    FTH (*func) (), int req, int opt, int rest, const char *doc)
{
	return (make_procedure(name, (void *) func, 0, req, opt, rest, doc));
}

/*
 * Return new ficlWord NAME tied to C function FUNC with REQ required
 * arguments, OPT optional arguments and REST (1) or no REST (0)
 * arguments with optional documentation string DOC.  FUNC takes zero
 * or more FTH objects and doesn't return any (void).
 */
ficlWord       *
fth_define_void_procedure(const char *name,
    void (*f) (), int req, int opt, int rest, const char *doc)
{
	return (make_procedure(name, (void *) f, 1, req, opt, rest, doc));
}

static void
ficl_make_proc(ficlVm *vm)
{
#define h_make_proc "( xt arity -- proc )  return proc object\n\
<'> + 2 make-proc value plus1\n\
lambda: ( a b -- c ) + ; #( 2 0 #f ) make-proc value plus2\n\
plus1 #( 0 1 ) proc-apply => 1\n\
plus2 #( 2 3 ) proc-apply => 5\n\
Return new proc object.  \
ARITY can be an integer or an array of length 3, #( req opt rest )."
	FTH 		arity, prc;
	ficlWord       *word;
	int 		req, opt, rest;

	FTH_STACK_CHECK(vm, 2, 1);
	arity = fth_pop_ficl_cell(vm);
	word = ficlStackPopPointer(vm->dataStack);
	opt = 0;
	rest = 0;
	if (fth_array_length(arity) == 3) {
		req = FIX_TO_INT32(fth_array_ref(arity, 0L));
		opt = FIX_TO_INT32(fth_array_ref(arity, 1L));
		rest = FTH_TO_BOOL(fth_array_ref(arity, 2L));
	} else
		req = FIX_TO_INT32(arity);
	prc = fth_make_proc(word, req, opt, rest);
	ficlStackPushFTH(vm->dataStack, prc);
}

static void
ficl_print_proc(ficlVm *vm)
{
#define h_print_proc "( prc -- )  print proc\n\
<'> + 2 make-proc .proc => #<proc +: 2/0/#f>\n\
Print proc object PRC to current output."
	FTH 		proc;

	FTH_STACK_CHECK(vm, 1, 0);
	proc = ficlStackPopFTH(vm->dataStack);
	if (FTH_PROC_P(proc))
		fth_printf("#<proc %s: %d/%d/%s>",
		    FICL_WORD_NAME(proc),
		    FICL_WORD_REQ(proc),
		    FICL_WORD_OPT(proc),
		    FICL_WORD_REST(proc) ? "#t" : "#f");
	else
		fth_print("not a proc object");
}

/*
 * If PROC is a Proc object, return required arguments as C int,
 * otherwise return 0.
 */
int
fth_proc_arity(FTH proc)
{
	if (FTH_PROC_P(proc))
		return (FICL_WORD_REQ(proc));
	return (0);
}

static void
ficl_proc_arity(ficlVm *vm)
{
#define h_proc_arity "( prc -- arity )  return arity of PROC\n\
<'> + proc-arity => #f\n\
<'> source-line proc-arity => #( 1 0 #f )\n\
Return arity array #( req opt rest ) of proc object PRC, \
or #f if not a proc object."
	ficlWord       *proc;

	FTH_STACK_CHECK(vm, 1, 1);
	proc = ficlStackPopPointer(vm->dataStack);
	if (FTH_PROC_P(proc)) {
		FTH 		ary;

		ary = FTH_LIST_3(INT_TO_FIX(FICL_WORD_REQ(proc)),
		    INT_TO_FIX(FICL_WORD_OPT(proc)),
		    BOOL_TO_FTH(FICL_WORD_REST(proc)));
		ficlStackPushFTH(vm->dataStack, ary);
	} else
		ficlStackPushBoolean(vm->dataStack, 0);
}

/*
 * If OBJ is a ficlWord, return name as C string, otherwise return
 * "not-a-proc".
 */
static char 	proc_not_a_word[] = "not-a-proc";
static char 	proc_no_name[] = "noname";
char           *
fth_proc_name(FTH obj)
{
	if (FICL_WORD_P(obj)) {
		if (FICL_WORD_REF(obj)->length > 0)
			return (FICL_WORD_NAME(obj));
		return (proc_no_name);
	}
	return (proc_not_a_word);
}

static void
ficl_proc_name(ficlVm *vm)
{
#define h_proc_name "( prc -- name )  return name of PROC\n\
<'> + proc-name => \"+\"\n\
Return name of proc object PRC if found, otherwise an empty string."
	FTH 		fs;
	ficlWord       *proc;

	FTH_STACK_CHECK(vm, 1, 1);
	proc = ficlStackPopPointer(vm->dataStack);
	if (FICL_WORD_P(proc)) {
		if (FICL_WORD_REF(proc)->length > 0)
			fs = FTH_WORD_NAME(proc);
		else
			fs = fth_make_string("noname");
	} else
		fs = fth_make_empty_string();
	ficlStackPushFTH(vm->dataStack, fs);
}

/*
 * Return source string property of PROC, or FTH_FALSE if not available.
 */
FTH
fth_proc_source_ref(FTH proc)
{
#define h_proc_source_ref "( proc-or-xt -- str )  return source string\n\
<'> + 2 make-proc value plus\n\
plus proc-source-ref => \"+\"\n\
Return source string of PROC-OR-XT, or #f if not available."
	FTH 		src;

	FTH_ASSERT_ARGS(FICL_WORD_P(proc), proc, FTH_ARG1, "a proc or xt");
	src = fth_source_ref(proc);
	return (FTH_NOT_FALSE_P(src) ? src : fth_word_to_string(proc));
}

/*
 * Set source string property of PROC to SOURCE.
 */
void
fth_proc_source_set(FTH proc, FTH source)
{
#define h_proc_source_set "( prc str -- )  set source string\n\
<'> + 2 make-proc value plus\n\
plus #( 1 2 ) proc-apply => 3\n\
plus \": plus ( n1 n2 -- n3 ) + ;\" proc-source-set!\n\
plus proc-source-ref => \": plus ( n1 n2 -- n3 ) + ;\"\n\
Set source string property of PRC to STR."
	FTH_ASSERT_ARGS(FICL_WORD_P(proc), proc, FTH_ARG1, "a proc or xt");
	fth_source_set(proc, source);
}

ficlWord       *
fth_proc_to_xt(FTH proc)
{
	if (FICL_WORD_P(proc))
		return (FICL_WORD_REF(proc));
	return (NULL);
}

static void
ficl_proc_to_xt(ficlVm *vm)
{
#define h_proc_to_xt "( prc -- xt )  return xt\n\
<'> source-line proc->xt => source-line\n\
Return the actual word (the execution token xt) of PRC."
	ficlWord       *proc;

	FTH_STACK_CHECK(vm, 1, 1);
	proc = ficlStackPopPointer(vm->dataStack);
	if (FICL_WORD_P(proc))
		ficlStackPushPointer(vm->dataStack, FICL_WORD_REF(proc));
	else
		ficlStackPushBoolean(vm->dataStack, 0);
}

static FTH
execute_proc(ficlVm *vm, ficlWord *word, int depth, const char *caller)
{
	int 		status;
	ficlInteger 	new_depth, i;
	FTH 		ret;
	char           *s;

	s = caller != NULL ? (char *) caller : "execute_proc";
	status = fth_execute_xt(vm, word);
	switch (status) {
	case FICL_VM_STATUS_ERROR_EXIT:
	case FICL_VM_STATUS_ABORT:
	case FICL_VM_STATUS_ABORTQ:
		if (word->length > 0)
			ficlVmThrowException(vm, status,
			    "%s: can't execute %S",
			    s, fth_word_inspect((FTH) word));
		else
			ficlVmThrowException(vm, status,
			    "%s: can't execute word %p", s, word);
		break;
	}
	/* collect values from stack */
	if (FTH_STACK_DEPTH(vm) > depth) {
		/* One entry: return single value. */
		new_depth = FTH_STACK_DEPTH(vm) - depth;
		if (new_depth == 1)
			return (fth_pop_ficl_cell(vm));
		/* More than one entries: -> return array with values.  */
		ret = fth_make_array_len(new_depth);
		for (i = 0; i < new_depth; i++)
			fth_array_fast_set(ret, i, fth_pop_ficl_cell(vm));
		return (ret);
	}
	/* If nothing added to stack (no return value), return #f. */
	return (FTH_FALSE);
}

/*-
 * Executes NAME, a C string, with LEN arguments of type FTH.
 * CALLER can be any C string used for error message.  Raise an
 * EVAL_ERROR exception if an error occured during evaluation.
 *
 * If the xt with NAME doesn't leave a return value on stack, return
 * FTH_FALSE, if a single value remains on stack, return it, if
 * more than one values remain on stack, return them as Array
 * object.
 *
 * no ret	=> #f
 * ret		=> ret
 * ret1 ret2	=> #( ret1 ret2 ... )
 *
 * FTH fs = fth_make_string("hello, world!");
 * FTH re = fth_make_regexp(", (.*)!");
 *
 * fth_xt_call("regexp-match", __func__, 2, re, fs);	=> 8
 * return (fth_xt_call("*re1*", __func__, 0));		=> "world"
 */
FTH
fth_xt_call(const char *name, const char *caller, int len,...)
{
	int 		depth;
	va_list 	list;
	ficlInteger 	i;
	ficlWord       *xt;
	ficlVm         *vm;
	ficlString 	s;

	if (name == NULL || *name == '\0')
		return (FTH_FALSE);
	s.text = (char *) name;
	s.length = strlen(name);
	xt = ficlDictionaryLookup(FTH_FICL_DICT(), s);
	if (xt == NULL)
		return (FTH_FALSE);
	vm = FTH_FICL_VM();
	depth = FTH_STACK_DEPTH(vm);
	va_start(list, len);
	for (i = 0; i < len; i++)
		fth_push_ficl_cell(vm, va_arg(list, FTH));
	va_end(list);
	return (execute_proc(vm, xt, depth, caller));
}

/*-
 * Executes Name, a C string, with array length arguments of type
 * FTH.  CALLER can be any C string used for error message.  Raise
 * an EVAL_ERROR exception if an error occured during evaluation.
 *
 * If the xt with NAME doesn't leave a return value on stack, return
 * FTH_FALSE, if a single value remains on stack, return it, if
 * more than one values remain on stack, return them as Array
 * object.
 *
 * no ret	=> #f
 * ret		=> ret
 * ret1 ret2	=> #( ret1 ret2 ... )
 *
 * FTH fs = fth_make_string("hello, world!");
 * FTH re = fth_make_regexp(", (.*)!");
 * FTH ary = fth_make_array_var(2, re, fs);
 *
 * fth_xt_apply("regexp-match", ary, __func__);		=> 8
 * return (fth_xt_apply("*re1*", FTH_FALSE, __func__));	=> "world"
 */
FTH
fth_xt_apply(const char *name, FTH args, const char *caller)
{
	int 		depth, len;
	ficlInteger 	i;
	ficlWord       *xt;
	ficlVm         *vm;
	ficlString 	s;

	if (name == NULL || *name == '\0')
		return (FTH_FALSE);
	s.text = (char *) name;
	s.length = strlen(name);
	xt = ficlDictionaryLookup(FTH_FICL_DICT(), s);
	if (xt == NULL)
		return (FTH_FALSE);
	len = 0;
	if (FTH_ARRAY_P(args))
		len = (int) fth_array_length(args);
	vm = FTH_FICL_VM();
	depth = FTH_STACK_DEPTH(vm);
	for (i = 0; i < len; i++)
		fth_push_ficl_cell(vm, fth_array_fast_ref(args, i));
	return (execute_proc(vm, xt, depth, caller));
}

/*-
 * If PROC is a Proc object, execute its ficlWord with LEN arguments
 * on stack.  CALLER can be any C string used for error message.  Raise a
 * BAD_ARITY exception if PROC has more required arguments than LEN.
 * Raise an EVAL_ERROR exception if an error occured during evaluation.
 *
 * If PROC is not a Proc object, return FTH_FALSE, if PROC doesn't
 * leave a return value on stack, return FTH_FALSE, if PROC leaves a
 * single value on stack, return it, if PROC leaves more than one
 * values on stack, return them as Array object.
 *
 * !proc	=> #f
 * no ret	=> #f
 * ret		=> ret
 * ret1 ret2	=> #( ret1 ret2 ... )
 */
FTH
fth_proc_call(FTH proc, const char *caller, int len,...)
{
	int 		depth;
	va_list 	list;
	ficlInteger 	i;
	ficlVm         *vm;

	if (!FTH_PROC_P(proc))
		return (FTH_FALSE);

	if (FICL_WORD_REQ(proc) > len)
		FTH_BAD_ARITY_ERROR_ARGS(FTH_ARG1,
		    proc, len, 0, 0,
		    FICL_WORD_REQ(proc),
		    FICL_WORD_OPT(proc),
		    FICL_WORD_REST(proc));

	if (len > FICL_WORD_LENGTH(proc))
		len = FICL_WORD_LENGTH(proc);

	vm = FTH_FICL_VM();
	depth = FTH_STACK_DEPTH(vm);
	va_start(list, len);

	for (i = 0; i < len; i++)
		fth_push_ficl_cell(vm, va_arg(list, FTH));

	va_end(list);
	return (execute_proc(vm, FICL_WORD_REF(proc), depth, caller));
}

/*-
 * If PROC is a Proc object, execute its ficlWord with Array object
 * ARGS as arguments on stack.  CALLER can be any C string used for
 * error message.  Raise a BAD_ARITY exception if PROC has more required
 * arguments than length of ARGS.  Raise an EVAL_ERROR exception if an
 * error occured during evaluation.
 *
 * If PROC is not a Proc object, return FTH_FALSE, if PROC doesn't
 * leave a return value on stack, return FTH_FALSE, if PROC leaves a
 * single value on stack, return it, if PROC leaves more than one
 * values on stack, return them as Array object.
 *
 * !proc	=> #f
 * no ret	=> #f
 * ret		=> ret
 * ret1 ret2	=> #( ret1 ret2 ... )
 */
FTH
fth_proc_apply(FTH proc, FTH args, const char *caller)
{
	int 		depth    , len;
	ficlInteger 	i;
	ficlVm         *vm;

	if (proc == 0 || !FTH_PROC_P(proc))
		return (FTH_FALSE);

	FTH_ASSERT_ARGS(FTH_ARRAY_P(args), args, FTH_ARG2, "an array");
	len = (int) fth_array_length(args);

	if (FICL_WORD_REQ(proc) > len)
		FTH_BAD_ARITY_ERROR_ARGS(FTH_ARG1,
		    proc, len, 0, 0,
		    FICL_WORD_REQ(proc),
		    FICL_WORD_OPT(proc),
		    FICL_WORD_REST(proc));

	if (len > FICL_WORD_LENGTH(proc))
		len = FICL_WORD_LENGTH(proc);

	vm = FTH_FICL_VM();
	depth = FTH_STACK_DEPTH(vm);

	for (i = 0; i < len; i++)
		fth_push_ficl_cell(vm, fth_array_fast_ref(args, i));

	return (execute_proc(vm, FICL_WORD_REF(proc), depth, caller));
}

static void
ficl_proc_apply(ficlVm *vm)
{
#define h_proc_apply "( prc args -- res )  run PRC with ARGS\n\
<'> + 2 make-proc value plus\n\
plus #( 1 2 ) proc-apply => 3\n\
Execute proc object PRC with argument ARGS and return result or #f.  \
ARGS can be an array of arguments or a single argument.  \
If execution fails, raise EVAL-ERROR exception, \
if length of ARGS is less than required arity of PRC, \
raise BAD-ARITY exception."
	FTH 		proc_or_xt, args, proc;

	FTH_STACK_CHECK(vm, 2, 1);
	args = fth_pop_ficl_cell(vm);
	proc_or_xt = ficlStackPopFTH(vm->dataStack);
	if (!FTH_ARRAY_P(args))
		args = fth_make_array_var(1, args);
	if (FTH_PROC_P(proc_or_xt))
		proc = proc_or_xt;
	else if (FTH_WORD_P(proc_or_xt))
		proc = fth_make_proc(FICL_WORD_REF(proc_or_xt),
		    (int) fth_array_length(args), 0, 0);
	else
		proc = FTH_FALSE;
	fth_push_ficl_cell(vm, fth_proc_apply(proc, args, RUNNING_WORD_VM(vm)));
}

static void
ficl_tick_set_im(ficlVm *vm)
{
#define h_tick_set_im "( \"name\" -- set-name|#f )  SET-NAME (parse word)\n\
<'set> object-print-length => set-object-print-length\n\
Parse NAME and search for word SET-NAME.  \
Return xt of SET-NAME or #f if not found.\n\
See also set!, set-xt and set-execute."
	ficlWord       *set_word;
	ficlString 	s;

	s = ficlVmGetWord(vm);
	snprintf(vm->pad, sizeof(vm->pad), "set-%.*s", (int) s.length, s.text);
	set_word = FICL_WORD_NAME_REF(vm->pad);
	if (set_word == NULL)
		set_word = FICL_WORD_REF(FTH_FALSE);
	if (vm->state == FICL_VM_STATE_COMPILE) {
		ficlDictionary *dict;
		ficlUnsigned	u;

		dict = ficlVmGetDictionary(vm);
		u = (ficlUnsigned) ficlInstructionLiteralParen;
		ficlDictionaryAppendUnsigned(dict, u);
		ficlDictionaryAppendPointer(dict, set_word);
	} else
		ficlStackPushPointer(vm->dataStack, set_word);
}

static void
ficl_set_bang_im(ficlVm *vm)
{
#define h_set_bang_im "( ?? \"name\" -- ?? )  execute SET-NAME (parse word)\n\
object-print-length => 12\n\
128 set! object-print-length \\ execute actually: \
128 set-object-print-length\n\
object-print-length => 128\n\
Parse NAME and execute word SET-NAME if found, \
otherwise raise UNDEFINED-WORD exception.\n\
See also <'set>, set-xt and set-execute."
	ficlWord       *word;
	ficlString 	s;

	s = ficlVmGetWord(vm);
	snprintf(vm->pad, sizeof(vm->pad), "set-%.*s", (int) s.length, s.text);
	word = FICL_WORD_NAME_REF(vm->pad);
	if (word == NULL) {
		fth_throw(FTH_UNDEFINED, "%s: %s not found",
		    RUNNING_WORD_VM(vm), vm->pad);
		/* NOTREACHED */
		return;
	}
	if (vm->state == FICL_VM_STATE_COMPILE)
		ficlDictionaryAppendPointer(ficlVmGetDictionary(vm), word);
	else
		fth_execute_xt(vm, word);
}

static void
ficl_set_xt(ficlVm *vm)
{
#define h_set_xt "( xt -- set-xt|#f )  return SET-XT\n\
<'> object-print-length set-xt => set-object-print-length\n\
Return SET-XT if found, otherwise #f.  \
See also <'set>, set! and set-execute."
	FTH 		obj;
	ficlWord       *word, *set_word;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = ficlStackPopFTH(vm->dataStack);
	if (!FICL_WORD_P(obj)) {
		FTH_ASSERT_ARGS(FICL_WORD_P(obj), obj, FTH_ARG1,
		    "a proc or xt");
		/* NOTREACHED */
		return;
	}
	word = FICL_WORD_REF(obj);
	snprintf(vm->pad, sizeof(vm->pad), "set-%s", word->name);
	set_word = FICL_WORD_NAME_REF(vm->pad);
	if (set_word == NULL)
		set_word = FICL_WORD_REF(FTH_FALSE);
	if (vm->state == FICL_VM_STATE_COMPILE) {
		ficlDictionary *dict;
		ficlUnsigned	u;

		dict = ficlVmGetDictionary(vm);
		u = (ficlUnsigned) ficlInstructionLiteralParen;
		ficlDictionaryAppendUnsigned(dict, u);
		ficlDictionaryAppendPointer(dict, set_word);
	} else
		ficlStackPushPointer(vm->dataStack, set_word);
}

static void
ficl_set_execute(ficlVm *vm)
{
#define h_set_execute "( ?? xt -- ?? )  execute SET-XT\n\
object-print-length => 12\n\
128 <'> object-print-length set-execute\n\
object-print-length => 128\n\
Execute SET-XT if found, otherwise raise UNDEFINED-WORD exception.\n\
See also <'set>, set! and set-xt."
	FTH 		obj;
	ficlWord       *word, *set_word;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = ficlStackPopFTH(vm->dataStack);
	if (!FICL_WORD_P(obj)) {
		FTH_ASSERT_ARGS(FICL_WORD_P(obj), obj, FTH_ARG1,
		    "a proc or xt");
		/* NOTREACHED */
		return;
	}
	word = FICL_WORD_REF(obj);
	snprintf(vm->pad, sizeof(vm->pad), "set-%s", word->name);
	set_word = FICL_WORD_NAME_REF(vm->pad);
	if (set_word == NULL) {
		fth_throw(FTH_UNDEFINED, "%s: %s not found",
		    RUNNING_WORD_VM(vm), vm->pad);
		/* NOTREACHED */
		return;
	}
	if (vm->state == FICL_VM_STATE_COMPILE)
		ficlDictionaryAppendPointer(ficlVmGetDictionary(vm), set_word);
	else
		fth_execute_xt(vm, set_word);
}

static void
ficl_xt_to_name(ficlVm *vm)
{
#define h_xt_to_name "( xt -- str )  return name of XT\n\
<'> + xt->name => \"+\"\n\
Return name of XT if found, otherwise an empty string."
	FTH 		fs;
	ficlWord       *word;

	FTH_STACK_CHECK(vm, 1, 1);
	word = ficlStackPopPointer(vm->dataStack);
	if (FICL_WORD_DEFINED_P(word))
		fs = FTH_WORD_NAME(word);
	else
		fs = fth_make_empty_string();
	ficlStackPushFTH(vm->dataStack, fs);
}

static void
ficl_xt_to_origin(ficlVm *vm)
{
#define h_xt_to_origin "( xt -- str )  return source origin\n\
<'> + xt->origin => \"+:primitive\"\n\
<'> make-timer xt->origin\n\
  => \"make-timer:/usr/local/share/fth/fth-lib/fth.fs:463\"\n\
Return name, source file and source line number where XT \
was defined (name:file:line).  \
If XT is a C-primitive, return (name:primitive), if not defined, \
return an empty string."
	FTH 		fs;
	ficlWord       *word;

	FTH_STACK_CHECK(vm, 1, 1);
	word = ficlStackPopPointer(vm->dataStack);
	if (FICL_WORD_DEFINED_P(word)) {
		fs = FTH_WORD_NAME(word);
		if (FICL_WORD_PRIMITIVE_P(word))
			fth_string_sformat(fs, ":primitive");
		else
			fth_string_sformat(fs, ":%S:%ld",
			    FICL_WORD_FILE(word), FICL_WORD_LINE(word));
	} else
		fs = fth_make_empty_string();
	ficlStackPushFTH(vm->dataStack, fs);
}

#define MAX_LINE_LENGTH		78

/*
 * Returned string must be freed.
 */
static char    *
help_formatted(char *str, int line_length)
{
	size_t 		len, line_len, size;
	char           *buffer, *pw, *pstr;

	if (str == NULL)
		return (NULL);
	size = strlen(str) * 2;
	buffer = FTH_CALLOC(size, sizeof(char));
	pstr = str;
	len = 0L;
	line_len = (size_t) line_length;
	while (pstr != NULL && *pstr != '\0') {
		pw = proc_scratch;
		while (*pstr != '\0' && !isspace((int) *pstr)) {
			*pw++ = *pstr++;
			len++;
		}
		while (*pstr != '\n' && isspace((int) *pstr)) {
			*pw++ = *pstr++;
			len++;
		}
		*pw = '\0';
		if (len >= line_len) {
			len = strlen(proc_scratch);
			fth_strcat(buffer, size, "\n");
		}
		fth_strcat(buffer, size, proc_scratch);
		if (*pstr == '\n') {
			len = 0L;
			pstr++;
			fth_strcat(buffer, size, "\n");
		}
	}
	return (buffer);
}

/*
 * Returned string must be freed.
 */
static char    *
get_help(FTH obj, char *name)
{
	FTH 		fs;
	char           *buf, *str, *help;
	size_t 		len;

	fs = fth_documentation_ref(obj);
	/* symbol? */
	if (!FTH_STRING_P(fs) && fth_symbol_p(name))
		fs = fth_documentation_ref(fth_symbol(name));
	buf = fth_string_ref(fs);
	if (buf == NULL)
		return (FTH_STRDUP("no documentation available"));
	if (name == NULL)
		return (help_formatted(buf, MAX_LINE_LENGTH));
	/*
	 * "play-sound ..."
	 * "(make-oscil ..."
	 * name is already in buf
	 */
	len = strlen(name);
	if (strncmp(name, buf, len) == 0 || strncmp(name, buf + 1, len) == 0)
		return (help_formatted(buf, MAX_LINE_LENGTH));
	/*
	 * FTH_PRI1|PROC ...
	 */
	str = fth_format("%s  %s", name, buf);
	help = help_formatted(str, MAX_LINE_LENGTH);
	FTH_FREE(str);
	return (help);
}

static void
ficl_get_help(ficlVm *vm)
{
#define h_get_help "( \"name\" -- str )  return documentation (parse word)\n\
help make-array => make-array  ( len ...\n\
help *features* => *features*  ( -- )  return ...\n\
help array      => array  *** ARRAY PRIMITIVES *** ...\n\
Print documentation of NAME (Forth word or topic) \
or \"no documentation available\".  \
See also help-set!, help-add! and help-ref."
	char           *help;
	ficlWord       *word;

	ficlVmGetWordToPad(vm);
	word = FICL_WORD_NAME_REF(vm->pad);
	if (word == NULL)
		help = get_help(fth_make_string(vm->pad), vm->pad);
	else
		help = get_help((FTH) word, vm->pad);
	/* help == NULL is no issue here! */
	push_cstring(vm, help);
	FTH_FREE(help);
}

static void
ficl_help_ref(ficlVm *vm)
{
#define h_help_ref "( obj -- str )  return documentation\n\
\"make-array\"   help-ref => \"make-array  ( len ...\"\n\
<'> make-array help-ref => \"make-array  ( len ...\"\n\
\"*features*\"   help-ref => \"*features*  ( -- )  return ...\"\n\
<'> *features* help-ref => \"*features*  ( -- )  return ...\"\n\
\"array\"        help-ref => \"array  *** ARRAY PRIMITIVES *** ...\"\n\
Return documentation of OBJ (Forth word, object or topic) \
or \"no documentation available\".\n\
See also help-set!, help-add! and help."
	FTH 		obj;
	char           *help;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = ficlStackPopFTH(vm->dataStack);
	if (FICL_WORD_DEFINED_P(obj))
		help = get_help(obj, FICL_WORD_NAME(obj));
	else
		help = get_help(obj, fth_string_or_symbol_ref(obj));
	/* help == NULL is no issue here! */
	push_cstring(vm, help);
	FTH_FREE(help);
}

static void
ficl_help_set(ficlVm *vm)
{
#define h_help_set "( obj str -- )  set documentation\n\
#( \"behemoth\" \"pumpkin\" \"mugli\" ) value hosts\n\
hosts \"local-net hostnames\" help-set!\n\
hosts help-ref => \"local-net hostnames\"\n\
<'> make-array \"make-array description...\" help-set!\n\
Set documentation of OBJ (Forth word or object) to STR.\n\
See also help, help-add! and help-ref."
	FTH 		obj, str;

	FTH_STACK_CHECK(vm, 2, 0);
	str = fth_pop_ficl_cell(vm);
	obj = fth_pop_ficl_cell(vm);
	fth_documentation_set(obj, str);
}

static void
ficl_help_add(ficlVm *vm)
{
#define h_help_add "( obj str -- )  append documentation\n\
<'> make-array \"make-array description...\" help-set!\n\
<'> make-array \"further infos\" help-add!\n\
Append STR to documentation of OBJ.\n\
See also help, help-ref, help-set!."
	FTH 		obj, str, help;

	FTH_STACK_CHECK(vm, 2, 0);
	str = fth_pop_ficl_cell(vm);
	obj = fth_pop_ficl_cell(vm);
	help = fth_documentation_ref(obj);
	if (FTH_STRING_P(help))
		fth_string_sformat(help, "\n%S", str);
	else
		fth_documentation_set(obj, str);
}

/*
 * Return documentation property string of OBJ or FTH_FALSE.
 */
FTH
fth_documentation_ref(FTH obj)
{
#define h_doc_ref "( obj -- str|#f )  return documentation\n\
\"make-array\"   documentation-ref => \"make-array  ( len ...\"\n\
<'> make-array documentation-ref => \"make-array  ( len ...\"\n\
Return documentation string of OBJ (Forth word, object or topic) or #f.  \
See also documentation-set!."
	if (FTH_STRING_P(obj)) {
		ficlWord       *word;

		word = FICL_WORD_NAME_REF(fth_string_ref(obj));
		if (word != NULL)
			return (fth_word_property_ref((FTH) word,
				FTH_SYMBOL_DOCUMENTATION));
		return (fth_property_ref(obj, FTH_SYMBOL_DOCUMENTATION));
	}
	if (FICL_WORD_DEFINED_P(obj))
		return (fth_word_property_ref(obj, FTH_SYMBOL_DOCUMENTATION));
	return (fth_object_property_ref(obj, FTH_SYMBOL_DOCUMENTATION));
}

/*
 * Set documentation property string of any OBJ to DOC.
 */
void
fth_documentation_set(FTH obj, FTH doc)
{
#define h_doc_set "( obj str -- )  set documentation\n\
#( \"behemoth\" \"pumpkin\" \"mugli\" ) value hosts\n\
hosts \"local-net hostnames\" documentation-set!\n\
hosts documentation-ref => \"local-net hostnames\"\n\
<'> make-array \"make-array description...\" documentation-set!\n\
Set documentation of any OBJ (Forth word, object or topic) to STR.  \
See also documentation-ref."
	FTH_ASSERT_ARGS(FTH_STRING_P(doc), doc, FTH_ARG2, "a string");
	if (FTH_STRING_P(obj)) {
		ficlWord       *word;

		word = FICL_WORD_NAME_REF(fth_string_ref(obj));
		if (word != NULL)
			fth_word_property_set((FTH) word,
			    FTH_SYMBOL_DOCUMENTATION, doc);
		else
			fth_property_set(obj, FTH_SYMBOL_DOCUMENTATION, doc);
	} else if (FICL_WORD_DEFINED_P(obj))
		fth_word_property_set(obj, FTH_SYMBOL_DOCUMENTATION, doc);
	else
		fth_object_property_set(obj, FTH_SYMBOL_DOCUMENTATION, doc);
}

/* obj: ficlWord or proc */
FTH
fth_source_ref(FTH obj)
{
#define h_source_ref "( obj -- str|#f )  return source\n\
<'> + 2 make-proc value plus\n\
plus source-ref => \"+\"\n\
Return source string of OBJ, a proc or xt, or #f if not found.  \
See also source-set!."
	if (FICL_WORD_DEFINED_P(obj))
		return (FICL_WORD_SRC(obj));
	return (FTH_FALSE);
}

/* obj: ficlWord or proc */
void
fth_source_set(FTH obj, FTH source)
{
#define h_source_set "( obj str -- )  set source\n\
<'> + 2 make-proc value plus\n\
plus #( 1 2 ) proc-apply => 3\n\
plus \": plus ( n1 n2 -- n3 ) + ;\" source-set!\n\
plus source-ref => \": plus ( n1 n2 -- n3 ) + ;\"\n\
Set source string of OBJ, a proc or xt, to STR.  \
See also source-ref."
	if (FICL_WORD_DEFINED_P(obj))
		fth_word_property_set(obj, FTH_SYMBOL_SOURCE, source);
}

/* obj: ficlWord */
FTH
fth_source_file(FTH obj)
{
#define h_source_file "( xt -- fname )  return source file\n\
<'> + source-file => #f\n\
<'> make-timer source-file => \"/usr/opt/share/nfth/fth-lib/fth.fs\"\n\
Return source file where XT was created or #f if XT is a \
primitive or not defined.\n\
See also source-line."
	if (FICL_WORD_DEFINED_P(obj))
		return (FICL_WORD_FILE(obj));
	return (FTH_FALSE);
}

/* obj: ficlWord */
FTH
fth_source_line(FTH obj)
{
#define h_source_line "( xt -- source-line )  return source line\n\
<'> + source-line => #f\n\
<'> make-timer source-line => 770\n\
Return source line number where XT was created or #f if XT is a \
primitive or not defined.\n\
See also source-file."
	if (FICL_WORD_DEFINED_P(obj) && FICL_WORD_LINE(obj) >= 0)
		return (fth_make_int(FICL_WORD_LINE(obj)));
	return (FTH_FALSE);
}

/* === Word specific functions. === */

ficlWord       *
fth_word_doc_set(ficlWord *word, const char *str)
{
	if (word != NULL && str != NULL)
		fth_word_property_set((FTH) word,
		    FTH_SYMBOL_DOCUMENTATION, fth_make_string(str));
	return (word);
}

/*
 * Takes first comment after word name (on same line) as documentation
 * string.
 */
static void
word_documentation(ficlVm *vm, ficlWord *word)
{
	char           *name, *doc;
	ficlInteger 	idx;
	ficlString 	s, s1;

	idx = ficlVmGetTibIndex(vm);
	s = ficlVmGetWord0(vm);
	/*
	 * Reset the terminal input index.  We need only the length of the
	 * next word.
	 */
	ficlVmSetTibIndex(vm, idx);
	if (s.length <= 0)
		return;
	name = word->length > 0 ? word->name : "noname";
	if (s.text[0] == '(') {
		s1 = ficlVmParseString(vm, ')');
		doc = fth_format("%s  %.*s)", name, (int) s1.length, s1.text);
	} else if (s.text[0] == '\\') {
		s1 = ficlVmParseString(vm, '\n');
		doc = fth_format("%s  %.*s", name, (int) s1.length, s1.text);
	} else
		doc = fth_format("%s", name);
	fth_word_doc_set(word, doc);
	FTH_FREE(doc);
}

/* defined in ficl/dictionary.c */
extern char    *ficlDictionaryInstructionNames[];

/* from ficl/dictionary.c */
static FTH
ficl_word_to_source(ficlDictionary *dict, ficlCell *cell)
{
	FTH 		fs;
	ficlWord       *word;
	ficlWordKind 	kind;
	ficlCell 	c;

	fs = fth_make_empty_string();
	for (; CELL_INT_REF(cell) != ficlInstructionSemiParen; cell++) {
		word = CELL_VOIDP_REF(cell);
		if (CELL_INT_REF(cell) == ficl_word_location) {
			cell += 3;
			continue;
		}
		if (!ficlDictionaryIsAWord(dict, word)) {
			/* probably not a word - punt and print value */
			fth_string_sformat(fs, "%D ",
			    ficl_to_fth(CELL_FTH_REF(cell)));
			continue;
		}
		switch ((kind = ficlWordClassify(word))) {
		case FICL_WORDKIND_INSTRUCTION:
			fth_string_sformat(fs, "%s",
			    ficlDictionaryInstructionNames[(long) word]);
			break;
		case FICL_WORDKIND_INSTRUCTION_WITH_ARGUMENT:
			c = *++cell;
			fth_string_sformat(fs, "%D %s",
			    ficl_to_fth(CELL_FTH_REF(&c)),
			    ficlDictionaryInstructionNames[(long) word]);
			break;
		case FICL_WORDKIND_INSTRUCTION_WORD:
			fth_string_sformat(fs, "%s", FICL_WORD_NAME(word));
			break;
		case FICL_WORDKIND_LITERAL:
			c = *++cell;
			if (ficlDictionaryIsAWord(dict, CELL_VOIDP_REF(&c)) &&
			    (CELL_INT_REF(&c) >= ficlInstructionLast))
				fth_string_sformat(fs, "%s",
				    FICL_WORD_NAME(CELL_VOIDP_REF(&c)));
			else
				fth_string_sformat(fs, "%D",
				    ficl_to_fth(CELL_FTH_REF(&c)));
			break;
		case FICL_WORDKIND_STRING_LITERAL:
			{
				ficlCountedString *counted;

				counted = (ficlCountedString *) (void *) ++cell;
				cell = (ficlCell *) ficlAlignPointer(
				    counted->text + counted->length + 1) - 1;
				fth_string_sformat(fs, "s\" %.*s\"",
				    counted->length, counted->text);
				break;
			}
		case FICL_WORDKIND_CSTRING_LITERAL:
			{
				ficlCountedString *counted;

				counted = (ficlCountedString *) (void *) ++cell;
				cell = (ficlCell *) ficlAlignPointer(
				    counted->text + counted->length + 1) - 1;
				fth_string_sformat(fs, "c\" %.*s\"",
				    counted->length, counted->text);
				break;
			}
		case FICL_WORDKIND_BRANCH0:
		case FICL_WORDKIND_BRANCH:
		case FICL_WORDKIND_QDO:
		case FICL_WORDKIND_DO:
		case FICL_WORDKIND_LOOP:
		case FICL_WORDKIND_OF:
		case FICL_WORDKIND_PLOOP:
			c = *++cell;
			fth_string_sformat(fs, "%s", FICL_WORD_NAME(word));
			break;
		default:
			if (FICL_WORD_DEFINED_P(word))
				fth_string_sformat(fs, "%s",
				    FICL_WORD_NAME(word));
			else
				fth_string_sformat(fs, "%D",
				    ficl_to_fth(CELL_FTH_REF(cell)));
			break;
		}
		fth_string_sformat(fs, " ");
	}
	return (fth_string_sformat(fs, ";"));
}

/*
 * XXX: fth_word_to_source
 * That's not enough to reconstruct the source...
 */
FTH
fth_word_to_source(ficlWord *word)
{
	ficlDictionary *dict;
	FTH 		fs;

	fs = fth_make_empty_string();
	dict = FTH_FICL_DICT();
	switch (ficlWordClassify(word)) {
	case FICL_WORDKIND_COLON:
		if (strncmp(FICL_WORD_NAME(word), "lambda", 6L) == 0)
			fth_string_sformat(fs, "lambda: ");
		else
			fth_string_sformat(fs, ": %s ", FICL_WORD_NAME(word));
		fth_string_sformat(fs, "%S",
		    ficl_word_to_source(dict, word->param));
		break;
	case FICL_WORDKIND_DOES:
		fth_string_sformat(fs, "does> %S",
		    ficl_word_to_source(dict,
			FICL_WORD_REF(CELL_VOIDP_REF(word->param))->param));
		break;
	case FICL_WORDKIND_CREATE:
		fth_string_sformat(fs, "create");
		break;
	case FICL_WORDKIND_VARIABLE:
	case FICL_WORDKIND_USER:
		fth_string_sformat(fs, "%D %s !",
		    FTH_WORD_PARAM(word), FICL_WORD_NAME(word));
		break;
	case FICL_WORDKIND_CONSTANT:
		fth_string_sformat(fs, "%D to %s",
		    FTH_WORD_PARAM(word), FICL_WORD_NAME(word));
		break;
	default:
		fth_string_sformat(fs, "%s \\ primitive ",
		    FICL_WORD_NAME(word));
		break;
	}
	if (word->flags & FICL_WORD_IMMEDIATE)
		fth_string_sformat(fs, " immediate");
	if (word->flags & FICL_WORD_COMPILE_ONLY)
		fth_string_sformat(fs, " compile-only");
	return (fs);
}

static void 
ficl_see(ficlVm *vm)
{
#define h_see "( \"name\" -- )  show definition (parse word)\n\
see2 make-timer => \": make-timer timer% %alloc ...\"\n\
Show word definition of NAME.\n\
See also see."
	ficlWord       *word;

	ficlPrimitiveTick(vm);
	word = ficlStackPopPointer(vm->dataStack);
	ficlStackPushFTH(vm->dataStack, fth_word_to_source(word));
}

static ficlWord *fth_latest_xt;

static void
ficl_latestxt(ficlVm *vm)
{
#define h_latestxt "( -- xt )  return latest xt\n\
latestxt => object-sort>\n\
Return latest defined xt.\n\
See also running-word and get-func-name."
	FTH_STACK_CHECK(vm, 0, 1);
	ficlStackPushPointer(vm->dataStack, fth_latest_xt);
}

static void
ficl_latestxt_co_im(ficlVm *vm)
{
#define h_latestxt_co_im "( -- xt )  return current xt\n\
: new-word\n\
	running-word xt->name .$ space\n\
	10\n\
;\n\
new-word => new-word 10\n\
Return current xt in word definitions.  \
This word is immediate and compile only and can only be \
used in word definitions.\n\
See also get-func-name and latestxt."
	ficlDictionary *dict;
	ficlUnsigned	u;

	dict = ficlVmGetDictionary(vm);
	u = (ficlUnsigned) ficlInstructionLiteralParen;
	ficlDictionaryAppendUnsigned(dict, u);
	ficlDictionaryAppendPointer(dict, dict->smudge);
}

static void
ficl_get_func_name_co_im(ficlVm *vm)
{
#define h_get_func_name_co_im "( -- str )  return name of current xt\n\
: new-word\n\
	get-func-name .$ space\n\
	10\n\
;\n\
new-word => new-word 10\n\
Return name of current xt in word definition as string.  \
This word is immediate and compile only and can only be \
used in word definitions.\n\
See also running-word and latestxt."
	ficlDictionary *dict;
	ficlUnsigned	u;

	dict = ficlVmGetDictionary(vm);
	u = (ficlUnsigned) ficlInstructionLiteralParen;
	ficlDictionaryAppendUnsigned(dict, u);
	ficlDictionaryAppendFTH(dict, FTH_WORD_NAME(dict->smudge));
}

static void
ficl_filename_im(ficlVm *vm)
{
#define h_filename_im "( -- str )  return current filename\n\
*filename* => \"repl-eval\"\n\
Return currently read filename.\n\
See also *lineno*."
	if (vm->state == FICL_VM_STATE_COMPILE) {
		ficlDictionary *dict;
		ficlUnsigned	u;

		dict = ficlVmGetDictionary(vm);
		u = (ficlUnsigned) ficlInstructionLiteralParen;
		ficlDictionaryAppendUnsigned(dict, u);
		ficlDictionaryAppendFTH(dict, fth_current_file);
	} else
		ficlStackPushFTH(vm->dataStack, fth_current_file);
}

static void
ficl_lineno_im(ficlVm *vm)
{
#define h_lineno_im "( -- u )  return current line number\n\
*lineno* => 3\n\
Return current read line number.\n\
See also *filename*."
	if (vm->state == FICL_VM_STATE_COMPILE) {
		ficlDictionary *dict;
		ficlUnsigned	u;

		dict = ficlVmGetDictionary(vm);
		u = (ficlUnsigned) ficlInstructionLiteralParen;
		ficlDictionaryAppendUnsigned(dict, u);
		ficlDictionaryAppendInteger(dict, fth_current_line);
	} else
		ficlStackPushInteger(vm->dataStack, fth_current_line);
}

static void
ficl_doc_quote_co_im(ficlVm *vm)
{
#define h_doc_quote_co_im "( space<ccc>\" -- )  set documentation\n\
: new-word ( -- )\n\
	doc\" our documentation may contain \\\"double quotes\\\".  \\\n\
Escape them with a backslash.\"\n\
  \\ we do nothing\n\
;\n\
help new-word => new-word  ( -- )  our documentation may contain \
\"double quotes\".  \
Escape them with a backslash.\n\
Add input buffer up to next double quote character '\"' to \
documentation of current word.  \
Escape double quote character with backslash if required in documentation.  \
It is not necessary to repeat the stack effect if \
it already exist in the word definition.  \
This word is immediate and compile only and can \
only be used in word definitions."
	ficlWord       *smudge;
	char           *ndoc;
	FTH 		odoc;

	/*-
	 * doc" open file with \"oboe.snd\" open-sound"
	 * Escape double quote character with backslash.
	 */
	ndoc = parse_input_buffer(vm, "\"");	/* must be freed */
	smudge = ficlVmGetDictionary(vm)->smudge;
	odoc = FICL_WORD_DOC(smudge);
	if (FTH_FALSE_P(odoc))
		fth_word_doc_set(smudge, ndoc);
	else
		fth_string_sformat(odoc, "  %s", ndoc);
	FTH_FREE(ndoc);
}

FTH
fth_get_optkey(FTH key, FTH def)
{
	ficlVm         *vm;
	int 		i, depth;
	FTH 		tmp, val;

	vm = FTH_FICL_VM();
	depth = FTH_STACK_DEPTH(vm);
	for (i = 1; i < depth; i++) {
		tmp = STACK_FTH_INDEX_REF(vm->dataStack, i);
		if (FTH_KEYWORD_P(tmp) && tmp == key) {
			/* key */
			ficlStackRoll(vm->dataStack, i);
			/* drop key */
			ficlStackDrop(vm->dataStack, 1);
			/* value on top of stack */
			ficlStackRoll(vm->dataStack, i - 1);
			val = fth_pop_ficl_cell(vm);
			return (FTH_UNDEF_P(val) ? def : val);
		}
	}
	return (def);
}

int
fth_get_optkey_fix(FTH key, int def)
{
	FTH 		res;

	res = fth_get_optkey(key, INT_TO_FIX(def));
	return (FIX_TO_INT32(res));
}

ficlInteger
fth_get_optkey_int(FTH key, ficlInteger def)
{
	return (fth_int_ref(fth_get_optkey(key, fth_make_int(def))));
}

ficl2Integer
fth_get_optkey_2int(FTH key, ficl2Integer def)
{
	return (fth_long_long_ref(fth_get_optkey(key,
		    fth_make_long_long(def))));
}

char           *
fth_get_optkey_str(FTH key, char *def)
{
	return (fth_string_ref(fth_get_optkey(key, fth_make_string(def))));
}

static void
ficl_get_optkey(ficlVm *vm)
{
#define h_get_optkey "( key def -- val )  return key value\n\
: optkey-test ( start dur keyword-args -- ary )\n\
	:frequency     440.0 get-optkey { freq }\n\
	:initial-phase pi    get-optkey { phase }\n\
	{ start dur }\n\
	#( start dur freq phase )\n\
;\n\
0 1                  optkey-test => #( 0.0 1.0 440.0 3.14159 )\n\
0 2 :frequency 330.0 optkey-test => #( 0.0 2.0 330.0 3.14159 )\n\
Return either default value DEF or a value found on \
stack determined by keyword KEY.  \
It simulates the :key keyword in Lisp/Scheme.\n\
See also <{, get-optarg, get-optkeys, get-optargs."
	FTH 		key, tmp;
	int 		i, depth;

	FTH_STACK_CHECK(vm, 2, 1);
	ficlStackRoll(vm->dataStack, 1);	/* swap */
	key = fth_pop_ficl_cell(vm);
	depth = FTH_STACK_DEPTH(vm);
	for (i = 2; i < depth; i++) {
		tmp = STACK_FTH_INDEX_REF(vm->dataStack, i);
		if (FTH_KEYWORD_P(tmp) && tmp == key) {
			/* key */
			ficlStackRoll(vm->dataStack, i);
			/* drop key */
			ficlStackDrop(vm->dataStack, 1);
			/* value on top of stack */
			ficlStackRoll(vm->dataStack, i - 1);
			/*
			 * If user specified undef, drop that and use default
			 * value.
			 */
			if (!FTH_UNDEF_P(STACK_FTH_INDEX_REF(vm->dataStack, 0)))
				ficlStackRoll(vm->dataStack, 1);
			ficlStackDrop(vm->dataStack, 1);
			break;
		}
	}
}

FTH
fth_get_optarg(ficlInteger req, FTH def)
{
	ficlVm         *vm;
	FTH 		val;

	vm = FTH_FICL_VM();
	if (FTH_STACK_DEPTH(vm) > req) {
		val = fth_pop_ficl_cell(vm);
		return (FTH_UNDEF_P(val) ? def : val);
	}
	return (def);
}

static void
ficl_get_optarg(ficlVm *vm)
{
#define h_get_optarg "( req def -- val )  return value\n\
: optarg-test ( a b c=33 d=44 e=55 -- ary )\n\
	4 55 get-optarg { e }\n\
	3 44 get-optarg { d }\n\
	2 33 get-optarg { c }\n\
	{ a b }\n\
	#( a b c d e )\n\
;\n\
1 2           optarg-test => #( 1 2 33 44 55 )\n\
1 2 3 4       optarg-test => #( 1 2  3  4 55 )\n\
1 2 3 4 5 6 7 optarg-test => 1 2 #( 3 4  5  6  7 )\n\
Numbers 1 and 2 remain on stack, they are not needed here.\n\
Return either default value DEF or a value found on stack.  \
REQ is the sum of required and following optional arguments.  \
It simulates the :optional keyword in Lisp/Scheme.\n\
See also <{, get-optkey, get-optkeys, get-optargs."
	ficlInteger 	req;

	FTH_STACK_CHECK(vm, 2, 1);
	ficlStackRoll(vm->dataStack, 1);	/* swap */
	req = ficlStackPopInteger(vm->dataStack);
	if (FTH_STACK_DEPTH(vm) - 1 > req) {
		/* If user specified undef, drop that and use default value. */
		if (FTH_UNDEF_P(STACK_FTH_INDEX_REF(vm->dataStack, 1)))
			ficlStackRoll(vm->dataStack, 1);
		ficlStackDrop(vm->dataStack, 1);
	}
}

static void
ficl_get_optkeys(ficlVm *vm)
{
#define h_get_optkeys "( ary req -- vals )  return key values\n\
: optkeys-test ( start dur keyword-args -- ary )\n\
	#( :frequency     440.0\n\
	   :initial-phase pi ) 2  get-optkeys { start dur freq phase }\n\
	#( start dur freq phase )\n\
;\n\
0 1                  optkeys-test => #( 0.0 1.0 440.0 3.14159 )\n\
0 2 :frequency 330.0 optkeys-test => #( 0.0 2.0 330.0 3.14159 )\n\
The plural form of get-optkey.  \
ARY is an array of key-value pairs, REQ is number of required arguments.  \
Return REQ + ARY length / 2 values on stack, either \
default ones or from stack.\n\
See also <{, get-optkey, get-optarg, get-optargs."
	FTH 		array;
	ficlInteger 	req, len, i;

	FTH_STACK_CHECK(vm, 2, 0);
	req = ficlStackPopInteger(vm->dataStack);
	array = fth_pop_ficl_cell(vm);

	len = fth_array_length(array);
	if (len == 0)
		return;
	FTH_ASSERT_ARGS(!(len % 2), array, FTH_ARG1, "an array (key/value)");
	FTH_ASSERT_ARGS(FTH_ARRAY_P(array), array, FTH_ARG1, "an array");
	FTH_STACK_CHECK(vm, req, len);
	for (i = 0; i < len; i += 2) {
		FTH 		key, val, tmp;
		int 		j, depth;

		key = fth_array_ref(array, i);
		val = fth_array_ref(array, i + 1);
		fth_push_ficl_cell(vm, val);
		depth = FTH_STACK_DEPTH(vm);
		for (j = 2; j < depth; j++) {
			tmp = STACK_FTH_INDEX_REF(vm->dataStack, j);
			if (FTH_KEYWORD_P(tmp) && tmp == key) {
				ficlStackRoll(vm->dataStack, j);
				ficlStackDrop(vm->dataStack, 1);
				ficlStackRoll(vm->dataStack, (int) i - 1);
				/*
				 * If user specified undef, drop that and use
				 * default value.
				 */
				if (!FTH_UNDEF_P(
					STACK_FTH_INDEX_REF(vm->dataStack, 0)))
					ficlStackRoll(vm->dataStack, 1);
				ficlStackDrop(vm->dataStack, 1);
				break;
			}
		}
	}
}

static void
ficl_get_optargs(ficlVm *vm)
{
#define h_get_optargs "( args req -- vals )  return values\n\
: optargs-test ( a b c=33 d=44 e=55 -- ary )\n\
	#( 33 44 55 ) 2 get-optargs { a b c d e }\n\
	#( a b c d e )\n\
;\n\
1 2           optargs-test => #( 1 2 33 44 55 )\n\
1 2 3 4       optargs-test => #( 1 2  3  4 55 )\n\
1 2 3 4 5 6 7 optargs-test => 1 2 #( 3 4  5  6  7 )\n\
Numbers 1 and 2 remain on stack, they are not needed here.\n\
The plural form of get-optarg.  \
ARGS is an array with default values, REQ is number of required arguments.  \
Return REQ + ARGS length values on stack, \
either default ones or from stack.\n\
See also <{, get-optkey, get-optarg, get-optkeys."
	FTH 		args;
	ficlInteger 	req, pos, i, j, len, depth;

	FTH_STACK_CHECK(vm, 2, 0);
	req = ficlStackPopInteger(vm->dataStack);
	args = ficlStackPopFTH(vm->dataStack);
	FTH_ASSERT_ARGS(FTH_ARRAY_P(args), args, FTH_ARG1, "an array");
	len = fth_array_length(args);
	if (len == 0)
		return;
	FTH_STACK_CHECK(vm, req, len);
	depth = FTH_STACK_DEPTH(vm) - req;
	if (depth > len)
		depth = len;
	for (i = 0, j = depth - 1, pos = req; i < len; i++, j--, pos++) {
		/* Replace undef with default values. */
		if (FTH_UNDEF_P(STACK_FTH_INDEX_REF(vm->dataStack, j)))
			STACK_FTH_INDEX_SET(vm->dataStack,
			    j, fth_to_ficl(fth_array_fast_ref(args, i)));
		/* Missing args filled with default values. */
		if (FTH_STACK_DEPTH(vm) <= pos)
			fth_push_ficl_cell(vm, fth_array_fast_ref(args, i));
	}
}

static ficlWord *args_keys_paren;
static ficlWord *args_optional_paren;
static ficlWord *local_paren;

#define FICL_EVAL_STRING_AND_THROW(Vm, Buffer) do {			\
	int status;							\
									\
	status = ficlVmEvaluate(Vm, fth_string_ref(Buffer));		\
	if (status == FICL_VM_STATUS_ERROR_EXIT)			\
		ficlVmThrowError(Vm, "can't execute %S", Buffer);	\
} while (0)

#define FTH_OPTKEY_ERROR_THROW(Desc)					\
	fth_throw(FTH_OPTKEY_ERROR, "%s: wrong optkey array, %S",	\
	    RUNNING_WORD(), Desc)

static void
ficl_args_keys_paren_co(ficlVm *vm)
{
	FTH 		keys;
	ficlInteger 	req, i, len;

	FTH_STACK_CHECK(vm, 2, 0);
	req = ficlStackPopInteger(vm->dataStack);
	keys = fth_pop_ficl_cell(vm);
	FTH_ASSERT_ARGS(FTH_ARRAY_P(keys), keys, FTH_ARG1, "an array");
	len = fth_array_length(keys);
	FTH_STACK_CHECK(vm, req, len);
	for (i = 0; i < len; i++) {
		FTH 		key, val, arg, tmp;
		int 		j, depth;

		arg = fth_array_fast_ref(keys, i);
		if (fth_array_length(arg) != 2)
			FTH_OPTKEY_ERROR_THROW(keys);
		key = fth_keyword(fth_string_ref(fth_array_ref(arg, 0L)));
		val = fth_array_ref(arg, 1L);
		FICL_EVAL_STRING_AND_THROW(vm, val);
		depth = FTH_STACK_DEPTH(vm);
		for (j = 2; j < depth; j++) {
			tmp = STACK_FTH_INDEX_REF(vm->dataStack, j);
			if (FTH_KEYWORD_P(tmp) && tmp == key) {
				ficlStackRoll(vm->dataStack, j);
				ficlStackDrop(vm->dataStack, 1);
				ficlStackRoll(vm->dataStack, j - 1);
				/*
				 * If user specified undef, drop that and use
				 * default value.
				 */
				if (!FTH_UNDEF_P(
					STACK_FTH_INDEX_REF(vm->dataStack, 0)))
					ficlStackRoll(vm->dataStack, 1);
				ficlStackDrop(vm->dataStack, 1);
				break;
			}
		}
	}
}

static void
ficl_args_optional_paren_co(ficlVm *vm)
{
	FTH 		defs;
	ficlInteger 	req, pos, i, j, len, depth;

	FTH_STACK_CHECK(vm, 2, 0);
	req = ficlStackPopInteger(vm->dataStack);
	defs = fth_pop_ficl_cell(vm);
	len = fth_array_length(defs);
	if (len == 0)
		return;
	FTH_STACK_CHECK(vm, req, len);
	depth = FTH_STACK_DEPTH(vm) - req;
	if (depth > len)
		depth = len;
	for (i = 0, j = depth - 1, pos = req; i < len; i++, j--, pos++) {
		/* Replace undef with evaled default values. */
		if (FTH_UNDEF_P(STACK_FTH_INDEX_REF(vm->dataStack, j))) {
			FICL_EVAL_STRING_AND_THROW(vm,
			    fth_array_fast_ref(defs, i));
			STACK_FTH_INDEX_SET(vm->dataStack, j,
			    ficlStackPopFTH(vm->dataStack));
		}
		/* Missing args filled with evaled default values. */
		if (FTH_STACK_DEPTH(vm) <= pos)
			FICL_EVAL_STRING_AND_THROW(vm,
			    fth_array_fast_ref(defs, i));
	}
}

static void
ficl_empty_extended_args_co_im(ficlVm *vm)
{
#define h_empty_ext_args_co_im "( -- )  turn colon definition in proc\n\
: we-dont-need-args <{}> ;\n\
Turn current colon definition in a proc object.  \
This word is immediate and compile only and can only be used \
in word definitions.\n\
See also <{."
	ficlWord       *smudge;

	smudge = ficlVmGetDictionary(vm)->smudge;
	if (!FTH_PROC_P(smudge))
		smudge = FICL_WORD_REF(fth_make_proc(smudge, 0, 0, 0));
	if (FTH_FALSE_P(FICL_WORD_DOC(smudge)))
		fth_word_doc_set(smudge, "( -- )");
}

static ficlString
get_string_from_tib(ficlVm *vm, int pos)
{
	char           *trace, *stop, *tmp;
	int 		cur_pos;
	ficlString 	s;

	trace = ficlVmGetInBuf(vm);
	stop = ficlVmGetInBufEnd(vm);
	tmp = trace;
	FICL_STRING_SET_POINTER(s, trace);
	for (cur_pos = 0; trace != stop && cur_pos != pos; cur_pos++, trace++)
		if (*trace == '\n')
			*tmp++ = ' ';
		else
			*tmp++ = *trace;
	FICL_STRING_SET_LENGTH(s, tmp - s.text);
	if (trace != stop && cur_pos == pos)
		trace++;
	ficlVmUpdateTib(vm, trace);
	return (s);
}

static void
ficl_extended_args_co_im(ficlVm *vm)
{
#define h_extended_args_co_im "( -- )  turn colon definition into a proc\n\
: optkey-test\n\
  <{ a b c\n\
     :key d 10 e 20\n\
     :optional f 30 g 40 -- ary }>\n\
	#( a b c d e f g )\n\
;\n\
1 2 3               optkey-test => #( 1 2 3 10 20 30 40 )\n\
:d 11 1 :e 22 2 3 4 optkey-test => #( 1 2 3 11 22  4 40 )\n\
Turn current colon definition into a proc object.  \
Take tokens up to closing '}>' as local variables, \
'--' start a comment ignoring rest to closing '}>'.  \
In addition to other local variable words like { } and {{ }} \
this form handles two keywords, :key and :optional.  \
Variable names are taken from keyword and optional names.  \
This word can span over more than one lines but without \
empty lines or comments in between.  \
If :key and :optional is used together, :key must come first.  \
All keyword and optional variables must have default values.  \
This word is immediate and compile only and can only be used \
in word definitions.\n\
See also <{}>, get-optkey, get-optart, get-optkeys, get-optargs."
	ficlDictionary *dict;
	ficlWord       *word;
	ficlInteger 	i, len, req, opt, in_lst;
	ficlUnsigned	u;
	int 		keyslen, defslen;
	int 		key_p, opt_p;
	char           *buffer, *s;
	FTH 		arg, arg1, arg2, keys, defs, args, fs;

	u = (ficlUnsigned) ficlInstructionLiteralParen;
	req = opt = 0;
	key_p = opt_p = 0;
	dict = FTH_FICL_DICT();
	word = dict->smudge;
	keys = fth_make_empty_array();
	defs = fth_make_empty_array();
	buffer = parse_tib_with_restart(vm, "}>", 2, get_string_from_tib);
	args = fth_string_split(fth_make_string(buffer), FTH_FALSE);
	FTH_FREE(buffer);
	fth_array_pop(args);	/* eliminate delimiter `}>' */
	len = fth_array_length(args);
	/*-
	 * Use local variables and optional values as stack effect for
	 * building a default doc string:
	 *
	 * : clm-mix <{ infile :key output #f output-frame 0 frames #f -- }>
	 *   ...
	 * ;
	 *
	 * automatically create this doc string:
	 *
	 *   clm-mix  ( infile :key output #f output-frame 0 frames #f -- )
	 */
	fs = fth_make_string("( ");
	for (i = 0; i < len; i++)
		fth_string_sformat(fs, "%S ", fth_array_fast_ref(args, i));
	fth_string_sformat(fs, ")");
	fth_word_doc_set(word, fth_string_ref(fs));
	/*
	 * Eliminate elements after `--'.
	 */
	for (i = 0; i < len; i++)
		if (strncmp(fth_string_ref(fth_array_fast_ref(args, i)),
			"--", 2L) == 0) {
			if (i == 0)
				args = fth_make_empty_array();
			else
				args = fth_array_subarray(args, 0L, i);
			break;
		}
	/*-
	 * Count required arguments and push their local variable names on
	 * stack.
	 *
	 * Example: name <{ a b :key c 1 :optional d 2 -- }>
	 *
	 * After processing the next while-loop the result would be req = 2
	 * with variable names 'a' and 'b'.  The array 'args' contains
	 * ':key c 1 :optional d 2'.
	 */
	while (FTH_NOT_FALSE_P(arg = fth_array_shift(args))) {
		s = fth_string_ref(arg);
		if (s != NULL && strncmp(s, ":key", 4L) == 0) {
			key_p = 1;
			break;
		}
		if (s != NULL && strncmp(s, ":optional", 9L) == 0) {
			opt_p = 1;
			break;
		}
		/*
		 * Push local variable name on stack.
		 */
		push_forth_string(vm, s);
		req++;
	}
#define LIST_START_P(Str)						\
	((Str) != NULL && strlen(Str) == 2 &&				\
	    Str[1] == '(' && (Str[0] == '#' || Str[0] == '\''))
	/*
	 * Keyword arguments.
	 */
	if (key_p) {
		while (FTH_NOT_FALSE_P(arg1 = fth_array_shift(args))) {
			s = fth_string_ref(arg1);
			if (s != NULL && strncmp(s, ":optional", 9L) == 0) {
				opt_p = 1;
				break;
			}
			/*
			 * Push local variable name on stack.
			 */
			push_forth_string(vm, s);
			arg2 = fth_array_shift(args);
			if (FTH_FALSE_P(arg2)) {
				FTH_OPTKEY_ERROR_THROW(args);
				/* NOTREACHED */
				return;
			}
			s = fth_string_ref(arg2);
			if (LIST_START_P(s)) {
				in_lst = 1;
				do {
					fs = fth_array_shift(args);
					s = fth_string_ref(fs);
					if (s == NULL) {
						FTH_OPTKEY_ERROR_THROW(args);
						/* NOTREACHED */
						return;
					}
					if (LIST_START_P(s))
						in_lst++;
					if (*s == ')')
						in_lst--;
					fth_string_sformat(arg2, " %s", s);
				} while (in_lst);
			}
			fth_array_push(keys, fth_make_array_var(2, arg1, arg2));
		}
		/*-
		 * keys postpone literal
		 * req postpone literal
		 * postpone (args-keys)
		 */
		ficlDictionaryAppendUnsigned(dict, u);
		ficlDictionaryAppendFTH(dict, keys);
		ficlDictionaryAppendUnsigned(dict, u);
		ficlDictionaryAppendInteger(dict, req);
		ficlDictionaryAppendPointer(dict, args_keys_paren);
		keyslen = fth_array_length(keys);
		for (i = 0; i < keyslen; i++)
			ficlVmExecuteXT(vm, local_paren);
	}
	/*
	 * Optional arguments.
	 */
	if (opt_p) {
		while (FTH_NOT_FALSE_P(arg1 = fth_array_shift(args))) {
			/*
			 * Push local variable name on stack.
			 */
			push_forth_string(vm, fth_string_ref(arg1));
			opt++;
			arg2 = fth_array_shift(args);
			if (FTH_FALSE_P(arg2)) {
				FTH_OPTKEY_ERROR_THROW(args);
				/* NOTREACHED */
				return;
			}
			s = fth_string_ref(arg2);
			if (LIST_START_P(s)) {
				in_lst = 1;
				do {
					fs = fth_array_shift(args);
					s = fth_string_ref(fs);
					if (s == NULL) {
						FTH_OPTKEY_ERROR_THROW(args);
						/* NOTREACHED */
						return;
					}
					if (LIST_START_P(s))
						in_lst++;
					if (*s == ')')
						in_lst--;
					fth_string_sformat(arg2, " %s", s);
				} while (in_lst);
			}
			fth_array_push(defs, arg2);
		}
		/*-
		 * defs postpone literal
		 * req postpone literal
		 * postpone (args-optional)
		 */
		ficlDictionaryAppendUnsigned(dict, u);
		ficlDictionaryAppendFTH(dict, defs);
		ficlDictionaryAppendUnsigned(dict, u);
		ficlDictionaryAppendInteger(dict, req);
		ficlDictionaryAppendPointer(dict, args_optional_paren);
		defslen = fth_array_length(defs);
		for (i = 0; i < defslen; i++)
			ficlVmExecuteXT(vm, local_paren);
	}
	/* Takes local variable names from stack in reverse order. */
	for (i = 0; i < req; i++)
		ficlVmExecuteXT(vm, local_paren);
	ficlStackPushInteger(vm->dataStack, 0L);
	ficlStackPushInteger(vm->dataStack, 0L);
	ficlVmExecuteXT(vm, local_paren);	/* 0 0 (local) */
	if (!FTH_PROC_P(word))
		fth_make_proc(word, (int) req, (int) opt, key_p);
}

/* from ficl/primitive.c */
static char 	colon_tag[] = "colon";
static char 	locals_dummy[] = " ___dummy___ ";

/*-
 * Creates a dummy local variable in ":", "lambda:", and "does>".  If
 * the first local variable of a word definition was created in a loop,
 * it wasn't handled correctly.  ficl_init_locals solves this issue.
 *
 * called in:
 * src/proc.c        ficl_begin_definition  -- ":"
 *                   ficl_lambda_definition -- "lambda:"
 * ficl/primitives.c ficlPrimitiveDoesCoIm  -- "does>"
 */
void
ficl_init_locals(ficlVm *vm, ficlDictionary *dict)
{
	ficlUnsigned	u;

	/*-
	 * 0 postpone literal ( dummy value for dummy variable )
	 * s" ___dummy___" (local)
	 * 0 0 (local)
	 */
	u = (ficlUnsigned) ficlInstructionLiteralParen;
	ficlDictionaryAppendUnsigned(dict, u);
	ficlDictionaryAppendUnsigned(dict, 0UL);
	push_forth_string(vm, locals_dummy);
	ficlVmExecuteXT(vm, local_paren);
	ficlStackPushInteger(vm->dataStack, 0L);
	ficlStackPushInteger(vm->dataStack, 0L);
	ficlVmExecuteXT(vm, local_paren);
}

/*-
 * Redefinition of ':' (colon)
 *
 * The stack-effect comment or a normal comment after the word name
 * will be used as a simple documentation entry.
 *
 * : new-word1 ( a b c -- d ) ... ;
 * help new-word1 => new-word1  ( a b c -- d )
 *
 * : new-word2 \ here comes some comment
 *   ...
 * ;
 * help new-word2 => new-word2  \ here comes some comment
 */
static void
ficl_begin_definition(ficlVm *vm)
{
#define h_begin_definition "( \"name\" -- )  start colon definition\n\
: plus ( n1 n2 -- n3 ) + ;\n\
help plus => plus  ( n1 n2 -- n3 )\n\
: mult \\ multiplies two args\n\
  *\n\
;\n\
help mult => mult  \\ multiplies two args\n\
Redefinition of standard colon `:'.  \
Start word definition and set variable LATESTXT to word.  \
In addition to standard colon `:' \
stack-effect comment or normal comment immediately after the word name \
will be used as documentation.\n\
See also lambda:."
	ficlDictionary *dict;
	ficlString 	s;
	ficlPrimitive 	code;
	ficlUnsigned 	flag;
	ficlWord       *word;

	dict = ficlVmGetDictionary(vm);
	s = ficlVmGetWord(vm);
	code = (ficlPrimitive) ficlInstructionColonParen;
	flag = FICL_WORD_DEFAULT | FICL_WORD_SMUDGED;
	vm->state = FICL_VM_STATE_COMPILE;
	ficlStackPushPointer(vm->dataStack, colon_tag);
	word = ficlDictionaryAppendWord(dict, s, code, flag);
	vm->callback.system->localsCount = 0;
	word->primitive_p = 0;
	word_documentation(vm, word);
	fth_latest_xt = word;
	ficl_init_locals(vm, dict);
}

static void
ficl_lambda_definition(ficlVm *vm)
{
#define h_lambda_definition "( -- xt )  start word definition\n\
lambda: ( a b -- c ) + ; value plus\n\
1 2 plus execute => 3\n\
1 2 lambda: ( a b -- c ) * ; execute => 2\n\
Start nameless word definition and set variable LATESTXT to word.  \
Stack-effect or normal comment immediately at the beginning \
will be used as documentation.  \
Return xt (after closing semicolon `;') and set variable LATESTXT to xt.\n\
See also `:'."
	ficlDictionary *dict;
	ficlPrimitive 	code;
	ficlUnsigned 	flag;
	ficlWord       *word;
	ficlString 	name;

	dict = ficlVmGetDictionary(vm);
	code = (ficlPrimitive) ficlInstructionColonParen;
	flag = FICL_WORD_DEFAULT | FICL_WORD_SMUDGED;
	vm->state = FICL_VM_STATE_COMPILE;
	FICL_STRING_SET_LENGTH(name, 0);
	FICL_STRING_SET_POINTER(name, NULL);
	word = ficlDictionaryAppendWord(dict, name, code, flag);
	vm->callback.system->localsCount = 0;
	ficlStackPushPointer(vm->dataStack, word);
	ficlStackPushPointer(vm->dataStack, colon_tag);
	word->primitive_p = 0;
	word_documentation(vm, word);
	fth_latest_xt = word;
	ficl_init_locals(vm, dict);
}

static void
ficl_proc_create_co(ficlVm *vm)
{
#define h_proc_create "( arity -- prc )  create nameless proc\n\
: input-fn ( gen -- proc; dir self -- r )\n\
	{ gen }\n\
	1 proc-create	\\ return proc with one required argument\n\
	gen ,		\\ store gen for later use in DOES part\n\
  does> { dir self -- r  } \\ dir (ignored here) self (address)\n\
	self @		\\ return our gen\n\
	readin		\\ return readin value\n\
;\n\
Return nameless proc object with ARITY.  \
Like CREATE it goes with DOES>.\n\
This word is compile only and can only be used in word definitions."
	ficlDictionary *dict;
	ficlPrimitive 	code;
	ficlUnsigned 	flag;
	ficlWord       *word;
	ficlString 	name;
	FTH 		prc;
	int 		arity;

	dict = ficlVmGetDictionary(vm);
	code = (ficlPrimitive) ficlInstructionCreateParen;
	flag = FICL_WORD_DEFAULT;
	FICL_STRING_SET_LENGTH(name, 0);
	FICL_STRING_SET_POINTER(name, NULL);
	word = ficlDictionaryAppendWord(dict, name, code, flag);
	word->primitive_p = 0;
	fth_latest_xt = word;
	arity = (int) ficlStackPopInteger(vm->dataStack);
	prc = fth_make_proc(word, arity, 0, 0);
	ficlStackPushFTH(vm->dataStack, prc);
	ficlVmDictionaryAllotCells(dict, 1);
}

static void
ficl_word_create_co(ficlVm *vm)
{
#define h_word_create "( name -- )  create word NAME\n\
: make-setter ( name -- ; hs val self -- )\n\
	{ name }\n\
	name \"!\" $+ word-create\n\
	name ,\n\
  does> { hs val self -- }\n\
	self @ { slot }\n\
	hs slot val hash-set!\n\
;\n\
\"user-time\" make-setter\n\
\\ creates word user-time!\n\
#{} value hs\n\
hs 3.2 user-time!\n\
hs => #{ 'user-time => 3.2 }\n\
Create word NAME in dictionary with DOES>-part as body.\n\
See also proc-create."
	ficlDictionary *dict;
	ficlPrimitive 	code;
	ficlUnsigned 	flag;
	ficlWord       *word;
	char           *name;
	FTH 		fs;

	FTH_STACK_CHECK(vm, 1, 0);
	fs = fth_pop_ficl_cell(vm);
	dict = ficlVmGetDictionary(vm);
	code = (ficlPrimitive) ficlInstructionCreateParen;
	flag = FICL_WORD_DEFAULT;
	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	name = fth_string_ref(fs);
	word = ficlDictionaryAppendPrimitive(dict, name, code, flag);
	word->primitive_p = 0;
	fth_latest_xt = word;
	ficlVmDictionaryAllotCells(dict, 1);
}

static void
ficl_constant(ficlVm *vm)
{
#define h_constant "( val \"name\" -- )  set constant\n\
#( \"behemoth\" \"pumpkin\" \"mugli\" ) constant hosts\n\
Redefinition of Ficl's CONSTANT (parse word).  \
In addition protect permanently VAL from garbage collection.\n\
See also value."
	FTH 		obj;

	FTH_STACK_CHECK(vm, 1, 0);
	ficlVmGetWordToPad(vm);
	obj = fth_pop_ficl_cell(vm);
	fth_define(vm->pad, obj);
}

static void
ficl_value(ficlVm *vm)
{
#define h_value "( val \"name\" -- )  set value\n\
#( \"behemoth\" \"pumpkin\" \"mugli\" ) value hosts\n\
Redefinition of Ficl's VALUE (parse word).  \
In addition protect temporarily VAL from garbage collection.\n\
See also constant."
	FTH 		obj;

	FTH_STACK_CHECK(vm, 1, 0);
	ficlVmGetWordToPad(vm);
	obj = fth_pop_ficl_cell(vm);
	fth_define_variable(vm->pad, obj, NULL);
}

static ficlWord *local_variables_paren;

static FTH
local_vars_cb(FTH key, FTH value, FTH data)
{
	ficlInteger 	offset;
	ficlCell       *frame;

	(void) key;
	offset = (ficlInteger) value;
	frame = ((ficlCell *) (data)) + offset;
	return (ficl_to_fth(CELL_FTH_REF(frame)));
}

static void
ficl_local_variables_paren_co(ficlVm *vm)
{
	FTH 		vars, frm, res;

	FTH_STACK_CHECK(vm, 1, 1);
	vars = ficlStackPopFTH(vm->dataStack);
	frm = (FTH) (vm->returnStack->frame);
	res = fth_hash_map(vars, local_vars_cb, frm);
	ficlStackPushFTH(vm->dataStack, res);

}

static void
ficl_local_variables_co_im(ficlVm *vm)
{
#define h_lvars "( -- vars )  return local variables\n\
: word-with-locals { foo -- }\n\
	10 { bar }\n\
	local-variables each .$ space end-each\n\
;\n\
20 word-with-locals => #{ \"bar\" => 10  \"foo\" => 20 }\n\
Returns a hash of local variable name-value pairs up to \
the location in definition.  \
This word is immediate and compile only and can only \
be used in word definitions."
	ficlDictionary *dict, *locals;
	ficlSystem     *sys;
	ficlHash       *hash;
	ficlWord       *word;
	ficlInteger 	i;
	ficlUnsigned	u;
	FTH 		vars, name, offset;

	vars = fth_make_hash();
	sys = vm->callback.system;
	if (sys->localsCount == 0)
		goto finish;
	/*
	 * At compile time we can only collect the names and the
	 * offset to frame of every local variable.  At interpret
	 * time we can find the current values on frame + offset (in
	 * ficl_local_variables_paren_co).
	 */
	locals = ficlSystemGetLocals(sys);
	hash = locals->wordlists[0];
	while (hash != NULL) {
		for (i = (int) hash->size - 1; i >= 0; i--) {
			word = hash->table[i];
			while (word != NULL) {
				if (strcmp(word->name, locals_dummy) == 0) {
					word = word->link;
					continue;
				}
				/*-
				 *  key: FTH (name)
				 *  val: ficlInteger (offset)
				 */
				name = FTH_WORD_NAME(word);
				offset = (FTH) FICL_WORD_PARAM(word);
				fth_hash_set(vars, name, offset);
				word = word->link;
			}
		}
		hash = hash->link;
	}
finish:
	dict = ficlVmGetDictionary(vm);
	u = (ficlUnsigned) ficlInstructionLiteralParen;
	ficlDictionaryAppendUnsigned(dict, u);
	ficlDictionaryAppendFTH(dict, vars);
	ficlDictionaryAppendPointer(dict, local_variables_paren);
}

FTH
proc_from_proc_or_xt(FTH proc_or_xt, int req, int opt, int rest)
{
	if (FTH_PROC_P(proc_or_xt)) {
		if ((FICL_WORD_REQ(proc_or_xt) != req) ||
		    (FICL_WORD_OPT(proc_or_xt) != opt) ||
		    (FICL_WORD_REST(proc_or_xt) != rest))
			FTH_BAD_ARITY_ERROR_ARGS(FTH_ARG1, proc_or_xt,
			    req, opt, rest,
			    FICL_WORD_REQ(proc_or_xt),
			    FICL_WORD_OPT(proc_or_xt),
			    FICL_WORD_REST(proc_or_xt));
		return (proc_or_xt);
	}
	if (FICL_WORD_P(proc_or_xt)) {
		FTH 		prc;

		prc = fth_make_proc(FICL_WORD_REF(proc_or_xt), req, opt, rest);
		return (prc);
	}
	return (FTH_FALSE);
}

/*
 * Return name of word with possible source filename and source line
 * number of the definition as FTH String if defined in the dictionary
 * or "unknown word 0xxxxxxxx".
 */
FTH
fth_word_inspect(FTH obj)
{
	FTH 		fs, cfs, msg;

	if (obj == 0)
		return (FTH_FALSE);
	if (FICL_INSTRUCTION_P(obj))
		return (fth_make_string(ficlDictionaryInstructionNames[obj]));
	if (!FICL_WORD_DEFINED_P(obj))
		return (fth_make_string_format("unknown word %p", (void *) obj));
	fs = FTH_WORD_NAME(obj);
	if (FTH_FALSE_P(fs))
		fs = fth_make_string("noname");
	if (FICL_WORD_REF(obj) != FICL_WORD_CURRENT_WORD(obj)) {
		cfs = FTH_WORD_NAME(FICL_WORD_CURRENT_WORD(obj));
		if (FTH_FALSE_P(cfs))
			fth_string_sformat(fs, " in noname");
		else
			fth_string_sformat(fs, " in %S", cfs);
		if (FICL_WORD_CURRENT_LINE(obj) > 0)
			fth_string_sformat(fs, " (%S:%ld)",
			    FICL_WORD_CURRENT_FILE(obj),
			    FICL_WORD_CURRENT_LINE(obj));
	} else if (FICL_WORD_LINE(obj) > 0)
		fth_string_sformat(fs, " (%S:%ld)",
		    FICL_WORD_FILE(obj),
		    FICL_WORD_LINE(obj));
	if (FICL_VARIABLES_P(obj))
		fth_string_sformat(fs, " (%S)", FTH_WORD_PARAM(obj));
	else if (FTH_EXCEPTION_P(obj)) {
		msg = fth_exception_last_message_ref(obj);
		if (FTH_NOT_FALSE_P(msg))
			fth_string_sformat(fs, " (%S)", msg);
	}
	return (fs);
}

/*
 * Return name of word as FTH String if defined in the dictionary, or
 * FTH_FALSE.
 */
FTH
fth_word_to_string(FTH obj)
{
	if (obj == 0)
		return (FTH_FALSE);
	if (FICL_INSTRUCTION_P(obj))
		return (fth_make_string(ficlDictionaryInstructionNames[obj]));
	if (FICL_WORD_DEFINED_P(obj)) {
		if (FICL_WORD_REF(obj)->length > 0)
			return (FTH_WORD_NAME(obj));
		return (fth_make_string("noname"));
	}
	return (FTH_FALSE);
}

/*
 * Return source of word as FTH String if defined in the dictionary.
 * If no source was found, try to return at least the name of word as
 * FTH String.
 */
FTH
fth_word_dump(FTH obj)
{
	FTH 		src;

	src = fth_source_ref(obj);
	if (FTH_FALSE_P(src))
		return (fth_word_to_string(obj));
	return (src);
}

/* === Define (constant, variable) === */

/*
 * Return true if NAME is defined in the dictionary, otherwise false.
 */

int
fth_defined_p(const char *name)
{
	return (name != NULL && FICL_NAME_DEFINED_P(name));
}

static void
ficl_defined_p(ficlVm *vm)
{
#define h_defined_p "( \"name\" -- f )  test if NAME is defined\n\
defined?  10 => #f\n\
defined? nil => #t\n\
defined?   + => #t\n\
Return #t if NAME is defined in the dictionary, otherwise #f."
	ficlVmGetWordToPad(vm);
	ficlStackPushBoolean(vm->dataStack, fth_defined_p(vm->pad));
}

/*
 * Define constant NAME to VALUE which can be a FTH Fixnum (in contrast
 * to fth_define_constant below) or any other FTH object.  Return
 * VALUE.
 */
FTH
fth_define(const char *name, FTH value)
{
	FTH_CONSTANT_SET(name, fth_to_ficl(value));
	return (fth_gc_permanent(value));
}

/*
 * Define constant NAME to VALUE which can be a C integer (in contrast
 * to fth_define above) or any other FTH object.  DOC can be NULL or
 * a description of the constant for the 'help' word.  Return VALUE
 * where C integers are converted to FTH Fixnums, any other objects
 * remain untouched.
 */
FTH
fth_define_constant(const char *name, FTH value, const char *doc)
{
	FTH_CONSTANT_SET_WITH_DOC(name, value, doc);
	return (fth_gc_permanent(ficl_to_fth(value)));
}

/*
 * Define global variable NAME to VALUE which can be a FTH Fixnum or
 * any other FTH object, see the similar function fth_define for
 * constants above.  DOC can be NULL or a description of the variable
 * for the 'help' word.  Return VALUE.
 */
FTH
fth_define_variable(const char *name, FTH value, const char *doc)
{
	FTH_CONSTANT_SET_WITH_DOC(name, fth_to_ficl(value), doc);
	return (fth_gc_protect(value));
}

/*
 * Return FTH value from global variable or constant NAME.
 */
FTH
fth_variable_ref(const char *name)
{
	return (fth_var_ref((FTH) FICL_WORD_NAME_REF(name)));
}

/*
 * Set (or create if not existing) global variable NAME to VALUE.
 * Return VALUE.
 */
FTH
fth_variable_set(const char *name, FTH value)
{
	ficlWord       *word;

	word = FICL_WORD_NAME_REF(name);
	if (word != NULL)
		return (fth_var_set((FTH) word, value));
	return (fth_define_variable(name, value, NULL));
}

FTH
fth_var_ref(FTH obj)
{
	if (FICL_VARIABLES_P(obj))
		return (FTH_WORD_PARAM(obj));
	return (FTH_FALSE);
}

FTH
fth_var_set(FTH obj, FTH value)
{
	FTH 		out;

	if (FICL_VARIABLES_P(obj)) {
		out = FTH_WORD_PARAM(obj);
		FICL_WORD_PARAM(obj) = fth_to_ficl(value);
		fth_gc_protect_set(out, value);
	}
	return (value);
}

/*-
 * Trace global variable:
 *
 * TRACE-VAR Installs a hook on the specified global variable and adds
 * procs with stack effect ( val -- res ).  The hook will be executed
 * after every variable set via TO.
 *
 * UNTRACE-VAR removes the hook.
 *
 * 8 value *clm-array-print-length*
 * <'> *clm-array-print-length* lambda: <{ val -- res }>
 *   val set-mus-array-print-length
 * ; trace-var
 * ...
 * <'> *clm-array-print-length* untrace-var
 *
 */
void
fth_trace_var(FTH obj, FTH proc_or_xt)
{
#define h_trace_var "( var proc-or-xt -- )  install trace xt\n\
8 value *clm-array-print-length*\n\
<'> *clm-array-print-length* lambda: <{ val -- res }>\n\
    val set-mus-array-print-length\n\
; trace-var\n\
24 to *clm-array-print-length*\n\
*clm-array-print-length* => 24\n\
mus-array-print-length   => 24\n\
<'> *clm-array-print-length* untrace-var\n\
Add PROC-OR-XT to global VAR hook which is utilized on every call of TO.  \
The stack effect of PROC-OR-XT must be ( val -- res ).\n\
See also untrace-var."
	FTH 		hook;

	if (!FICL_VARIABLES_P(obj)) {
		FTH_ASSERT_ARGS(FICL_VARIABLES_P(obj), obj, FTH_ARG1,
		    "a global variable");
		/* NOTREACHED */
		return;
	}
	hook = fth_word_property_ref(obj, FTH_SYMBOL_TRACE_VAR);
	if (!FTH_HOOK_P(hook))
		hook = fth_make_simple_hook(1);
	fth_add_hook(hook, proc_or_xt);
	fth_word_property_set(obj, FTH_SYMBOL_TRACE_VAR, hook);
	FICL_WORD_TYPE(obj) = FW_TRACE_VAR;
}

void
fth_untrace_var(FTH obj)
{
#define h_untrace_var "( var -- )  remove trace xt\n\
8 value *clm-array-print-length*\n\
<'> *clm-array-print-length* lambda: <{ val -- res }>\n\
    val set-mus-array-print-length\n\
; trace-var\n\
24 to *clm-array-print-length*\n\
*clm-array-print-length* => 24\n\
mus-array-print-length   => 24\n\
<'> *clm-array-print-length* untrace-var\n\
Remove previously installed hook from VAR.\n\
See also trace-var."
	if (!FTH_TRACE_VAR_P(obj)) {
		FTH_ASSERT_ARGS(FTH_TRACE_VAR_P(obj), obj, FTH_ARG1,
		    "a global traced variable");
		/* NOTREACHED */
		return;
	}
	fth_word_property_set(obj, FTH_SYMBOL_TRACE_VAR, FTH_FALSE);
	FICL_WORD_TYPE(obj) = FW_VARIABLE;
}

FTH
fth_trace_var_execute(ficlWord *word)
{
	FTH 		hook, res, args;

	FTH_ASSERT_ARGS(FTH_TRACE_VAR_P(word), (FTH) word, FTH_ARG1,
	    "a global variable");
	hook = fth_word_property_ref((FTH) word, FTH_SYMBOL_TRACE_VAR);
	if (!FTH_HOOK_P(hook))
		return (FTH_FALSE);
	args = fth_make_array_var(1, FTH_WORD_PARAM(word));
	res = fth_hook_apply(hook, args, RUNNING_WORD());
	return (res);
}

void
init_proc(void)
{
	ficlString 	s;

	s.text = "(local)";
	s.length = strlen(s.text);
	local_paren = ficlDictionaryLookup(FTH_FICL_DICT(), s);
	/* proc */
	FTH_PRI1("proc?", ficl_proc_p, h_proc_p);
	FTH_PRI1("thunk?", ficl_thunk_p, h_thunk_p);
	FTH_PRI1("xt?", ficl_xt_p, h_xt_p);
	FTH_PRI1("word?", ficl_word_p, h_word_p);
	FTH_PRI1("make-proc", ficl_make_proc, h_make_proc);
	FTH_PRI1(".proc", ficl_print_proc, h_print_proc);
	FTH_PRI1("proc-arity", ficl_proc_arity, h_proc_arity);
	FTH_PRI1("proc-name", ficl_proc_name, h_proc_name);
	FTH_PROC("proc-source-ref", fth_proc_source_ref,
	    1, 0, 0, h_proc_source_ref);
	FTH_VOID_PROC("proc-source-set!", fth_proc_source_set,
	    2, 0, 0, h_proc_source_set);
	FTH_PRI1("proc->xt", ficl_proc_to_xt, h_proc_to_xt);
	FTH_PRI1("proc-apply", ficl_proc_apply, h_proc_apply);
	FTH_PRI1("run-proc", ficl_proc_apply, h_proc_apply);
	FTH_PRIM_IM("<'set>", ficl_tick_set_im, h_tick_set_im);
	FTH_PRIM_IM("set!", ficl_set_bang_im, h_set_bang_im);
	FTH_PRI1("set-xt", ficl_set_xt, h_set_xt);
	FTH_PRI1("set-execute", ficl_set_execute, h_set_execute);
	FTH_PRI1("xt->name", ficl_xt_to_name, h_xt_to_name);
	FTH_PRI1("xt->origin", ficl_xt_to_origin, h_xt_to_origin);
	FTH_PRI1("help", ficl_get_help, h_get_help);
	FTH_PRI1("help-ref", ficl_help_ref, h_help_ref);
	FTH_PRI1("help-set!", ficl_help_set, h_help_set);
	FTH_PRI1("help-add!", ficl_help_add, h_help_add);
	FTH_PROC("documentation-ref", fth_documentation_ref,
	    1, 0, 0, h_doc_ref);
	FTH_VOID_PROC("documentation-set!", fth_documentation_set,
	    2, 0, 0, h_doc_set);
	FTH_PROC("source-ref", fth_source_ref, 1, 0, 0, h_source_ref);
	FTH_VOID_PROC("source-set!", fth_source_set, 2, 0, 0, h_source_set);
	FTH_PROC("source-file", fth_source_file, 1, 0, 0, h_source_file);
	FTH_PROC("source-line", fth_source_line, 1, 0, 0, h_source_line);
	/* ficl system */
	FTH_PRI1("see2", ficl_see, h_see);
	FTH_PRI1("latestxt", ficl_latestxt, h_latestxt);
	FTH_PRIM_CO_IM("running-word", ficl_latestxt_co_im, h_latestxt_co_im);
	FTH_PRIM_CO_IM("get-func-name", ficl_get_func_name_co_im,
	    h_get_func_name_co_im);
	FTH_PRIM_IM("*filename*", ficl_filename_im, h_filename_im);
	FTH_PRIM_IM("*lineno*", ficl_lineno_im, h_lineno_im);
	FTH_PRIM_CO_IM("doc\"", ficl_doc_quote_co_im, h_doc_quote_co_im);
	FTH_PRI1("get-optkey", ficl_get_optkey, h_get_optkey);
	FTH_PRI1("get-optarg", ficl_get_optarg, h_get_optarg);
	FTH_PRI1("get-optkeys", ficl_get_optkeys, h_get_optkeys);
	FTH_PRI1("get-optargs", ficl_get_optargs, h_get_optargs);
	FTH_PRIM_CO_IM("<{}>", ficl_empty_extended_args_co_im,
	    h_empty_ext_args_co_im);
	FTH_PRIM_CO_IM("<{", ficl_extended_args_co_im,
	    h_extended_args_co_im);
	args_keys_paren = FTH_PRIM_CO("(args-keys)",
	    ficl_args_keys_paren_co, NULL);
	args_optional_paren = FTH_PRIM_CO("(args-optional)",
	    ficl_args_optional_paren_co, NULL);
	/* redefinition of colon (:) */
	FTH_PRIMITIVE_SET(":", ficl_begin_definition,
	    FICL_WORD_DEFAULT, h_begin_definition);
	FTH_PRI1("lambda:", ficl_lambda_definition, h_lambda_definition);
	FTH_PRIM_CO("proc-create", ficl_proc_create_co, h_proc_create);
	FTH_PRIM_CO("word-create", ficl_word_create_co, h_word_create);
	/* redefinition of constant and value with gc protection */
	FTH_PRIMITIVE_SET("constant", ficl_constant,
	    FICL_WORD_DEFAULT, h_constant);
	FTH_PRIMITIVE_SET("value", ficl_value, FICL_WORD_DEFAULT, h_value);
	local_variables_paren = FTH_PRIM_CO("(local-variables)",
	    ficl_local_variables_paren_co, NULL);
	FTH_PRIM_CO_IM("local-variables", ficl_local_variables_co_im, h_lvars);
	FTH_PRI1("defined?", ficl_defined_p, h_defined_p);
	FTH_VOID_PROC("trace-var", fth_trace_var, 2, 0, 0, h_trace_var);
	FTH_VOID_PROC("untrace-var", fth_untrace_var, 1, 0, 0, h_untrace_var);
	FTH_ADD_FEATURE_AND_INFO(FTH_STR_PROC, h_list_of_proc_functions);
}

/*
 * proc.c ends here
 */
