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
 * @(#)symbol.c	1.99 12/31/17
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "fth.h"
#include "utils.h"

/* === SYMBOL === */

/* symbol */
static void 	ficl_create_symbol(ficlVm *vm);
static void 	ficl_print_symbol(ficlVm *vm);
static void 	ficl_symbol_intern_im(ficlVm *vm);
static void 	ficl_symbol_name(ficlVm *vm);
static void 	ficl_symbol_p(ficlVm *vm);
static void 	ficl_symbol_paren(ficlVm *vm);
static int 	fth_any_symbol_p(const char *name, int kind);
static FTH 	fth_make_symbol(FTH name);
static FTH	make_symbol(const char *n, const char *m, char prefix, int k);
/* keyword */
static void 	ficl_create_keyword(ficlVm *vm);
static void 	ficl_keyword_intern_im(ficlVm *vm);
static void 	ficl_keyword_name(ficlVm *vm);
static void 	ficl_keyword_p(ficlVm *vm);
static void 	ficl_keyword_paren(ficlVm *vm);
static void 	ficl_print_keyword(ficlVm *vm);
static FTH 	fth_make_keyword(FTH name);
/* exception */
static void 	ficl_create_exception(ficlVm *vm);
static void 	ficl_exception_last_message_ref(ficlVm *vm);
static void 	ficl_exception_last_message_set(ficlVm *vm);
static void 	ficl_exception_message_ref(ficlVm *vm);
static void 	ficl_exception_message_set(ficlVm *vm);
static void 	ficl_exception_name(ficlVm *vm);
static void 	ficl_exception_p(ficlVm *vm);
static void 	ficl_make_exception(ficlVm *vm);
static void 	ficl_print_exception(ficlVm *vm);

#define MAKE_REF_AND_EQUAL_P(name, NAME)				\
char *									\
fth_ ## name ## _ref(FTH obj)						\
{									\
	return (FTH_ ## NAME ## _P(obj) ?				\
	    (FICL_WORD_NAME(obj) + 1) : NULL);				\
}									\
int									\
fth_ ## name ## _equal_p(FTH obj1, FTH obj2)				\
{									\
	return (FTH_ ## NAME ## _P(obj1) &&				\
	    (obj1 == obj2 ||						\
	     strcmp(FICL_WORD_NAME(obj1), FICL_WORD_NAME(obj2)) == 0));	\
}									\
static void								\
ficl_ ## name ## _equal_p(ficlVm *vm)					\
{									\
	FTH obj1, obj2;							\
									\
	FTH_STACK_CHECK(vm, 2, 0);					\
	obj2 = ficlStackPopFTH(vm->dataStack);				\
	obj1 = ficlStackPopFTH(vm->dataStack);				\
	ficlStackPushBoolean(vm->dataStack,				\
	    fth_ ## name ## _equal_p(obj1, obj2));			\
}									\
static char* h_ ## name ## _equal_p = "( obj1 obj2 -- f )  compare\n\
'test :test " #name "= #f\n\
Return #t if OBJ1 and OBJ2 are " #name "s with identical name, otherwise #f."

/*-
 * MAKE_REF_AND_EQUAL_P(name, NAME) builds:
 *
 * char  *fth_symbol_ref(FTH obj);
 * int    fth_symbol_equal_p(FTH obj1, FTH obj2);
 * static void ficl_symbol_equal_p(ficlVm *vm);
 * static char *h_symbol_equal_p = "help string";
 *
 * char  *fth_keyword_ref(FTH obj);
 * int    fth_keyword_equal_p(FTH obj1, FTH obj2);
 * static void ficl_keyword_equal_p(ficlVm *vm);
 * static char *h_keyword_equal_p = "help string";
 *
 * char  *fth_exception_ref(FTH obj);
 * int    fth_exception_equal_p(FTH obj1, FTH obj2);
 * static void ficl_exception_equal_p(ficlVm *vm);
 * static char *h_exception_equal_p = "help string";
 */

MAKE_REF_AND_EQUAL_P(symbol, SYMBOL);
MAKE_REF_AND_EQUAL_P(keyword, KEYWORD);
MAKE_REF_AND_EQUAL_P(exception, EXCEPTION);

char           *
fth_string_or_symbol_ref(FTH obj)
{
	if (FTH_STRING_P(obj))
		return (fth_string_ref(obj));

	return (fth_symbol_ref(obj));
}

int
fth_string_or_symbol_p(FTH obj)
{
	return (FTH_STRING_P(obj) || FTH_SYMBOL_P(obj));
}

static int
fth_any_symbol_p(const char *name, int kind)
{
	int 		flag;

	flag = 0;

	if (name != NULL && *name != '\0') {

		if (name[0] == kind)
			flag = FICL_NAME_DEFINED_P(name);
		else {
			char 		sname[FICL_PAD_SIZE];

			snprintf(sname, sizeof(sname), "%c%s", kind, name);
			flag = FICL_NAME_DEFINED_P(sname);
		}

	}
	return (flag);
}

#define h_list_of_symbol_functions "\
*** SYMBOL PRIMITIVES ***\n\
.symbol             ( sym -- )\n\
create-symbol       ( \"name\" -- )\n\
make-symbol         ( name -- sym )\n\
symbol-name         ( sym -- name )\n\
symbol=             ( obj1 obj2 -- f )\n\
symbol?             ( obj -- f )"

#define SYMBOL_PREFIX '\''

int
fth_symbol_p(const char *name)
{
	return (fth_any_symbol_p(name, SYMBOL_PREFIX));
}

static void
ficl_symbol_p(ficlVm *vm)
{
#define h_symbol_p "( obj -- f )  test if OBJ is a symbol\n\
'test  symbol? => #t\n\
\"test\" symbol? =? #f\n\
Return #t if OBJ is a symbol, otherwise #f."
	FTH 		obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = ficlStackPopFTH(vm->dataStack);
	ficlStackPushBoolean(vm->dataStack, FTH_SYMBOL_P(obj));
}

static FTH
make_symbol(const char *name, const char *message, char prefix, int kind)
{
	char           *sname;
	ficlWord       *word;

	if (name == NULL || *name == '\0') {
		FTH_ASSERT_STRING(0);
		return (FTH_FALSE);
	}
	sname = (*name != prefix) ?
	    fth_format("%c%s", prefix, name) :
	    FTH_STRDUP(name);

	word = ficlDictionarySetConstant(FTH_FICL_DICT(), sname, 0L);
	FTH_FREE(sname);

	if (word != NULL) {
		word->kind = kind;
		CELL_VOIDP_SET(word->param, word);
		if (kind == FW_EXCEPTION && message != NULL)
			fth_word_property_set((FTH) word,
			    FTH_SYMBOL_MESSAGE, fth_make_string(message));
		return ((FTH) word);
	}
	FTH_SYSTEM_ERROR_ARG_THROW(make_symbol, FTH_STR_SYMBOL);
	return (FTH_FALSE);
}

/*
 * Return value, the word address, of symbol NAME; if symbol doesn't
 * exist, creat it.
 */
FTH
fth_symbol(const char *name)
{
	return (make_symbol(name, NULL, SYMBOL_PREFIX, FW_SYMBOL));
}

static void
ficl_create_symbol(ficlVm *vm)
{
#define h_create_symbol "( \"name\" -- )  create symbol (parse word)\n\
create-symbol new-symbol\n\
'new-symbol => 'new-symbol\n\
Create symbol NAME prepended by '.  \
Symbols are actually values (variables) named 'NAME.\n\
See also make-symbol and symbol-intern."
	ficlVmGetWordToPad(vm);
	fth_symbol(vm->pad);
}

static FTH
fth_make_symbol(FTH name)
{
#define h_make_symbol "( name -- sym )  return symbol\n\
\"new-symbol\" make-symbol drop\n\
'new-symbol => 'new-symbol\n\
An alternative way to create symbols is:\n\
'NAME => 'NAME\n\
Return symbol NAME prepended by '.  \
Symbols are actually values (variables) named 'NAME.\n\
See also create-symbol and symbol-intern."
	FTH_ASSERT_ARGS(FTH_STRING_P(name), name, FTH_ARG1, "a string");
	return (fth_symbol(fth_string_ref(name)));
}

static void
ficl_print_symbol(ficlVm *vm)
{
#define h_print_symbol "( sym -- )  print symbol\n\
'test .symbol => test\n\
Print symbol SYM to current output."
	FTH 		obj;

	FTH_STACK_CHECK(vm, 1, 0);
	obj = ficlStackPopFTH(vm->dataStack);

	if (FTH_SYMBOL_P(obj))
		fth_print(fth_symbol_ref(obj));
	else
		fth_print("not a symbol");
}

static void
ficl_symbol_name(ficlVm *vm)
{
#define h_symbol_name "( sym -- name )  return symbol name\n\
'test symbol-name => \"test\"\n\
Return name of symbol SYM."
	FTH 		obj;

	FTH_STACK_CHECK(vm, 1, 0);
	obj = ficlStackPopFTH(vm->dataStack);
	FTH_ASSERT_ARGS(fth_symbol_or_exception_p(obj), obj, FTH_ARG1,
	    "a symbol or exception");
	push_cstring(vm, fth_symbol_ref(obj));
}

static void
ficl_symbol_paren(ficlVm *vm)
{
#define h_symbol_paren "( str -- sym )  return symbol\n\
In compile state SYMBOL-INTERN used to create a symbol."
	FTH_STACK_CHECK(vm, 1, 1);
	ficlStackPushFTH(vm->dataStack, fth_symbol(pop_cstring(vm)));
}

static ficlWord *symbol_paren;

static void
ficl_symbol_intern_im(ficlVm *vm)
{
#define h_symbol_intern_im "( \"str\" -- sym )  return symbol\n\
'test symbol? => #t\n\
Prefix word; return new or existing symbol.  \
Predefined is:\n\
: ' postpone symbol-intern ; immediate\n\
See also create-symbol and make-symbol."
	ficlVmGetWordToPad(vm);

	if (vm->state == FICL_VM_STATE_COMPILE) {
		ficlDictionary *dict;
		ficlUnsigned 	u;

		dict = ficlVmGetDictionary(vm);
		u = (ficlUnsigned) ficlInstructionLiteralParen;
		ficlDictionaryAppendUnsigned(dict, u);
		ficlDictionaryAppendFTH(dict, fth_make_string(vm->pad));
		ficlDictionaryAppendPointer(dict, symbol_paren);
	} else
		ficlStackPushFTH(vm->dataStack, fth_symbol(vm->pad));
}

/* === KEYWORD === */

#define h_list_of_keyword_functions "\
*** KEYWORD PRIMITIVES ***\n\
.keyword            ( kw -- )\n\
create-keyword      ( \"name\" -- )\n\
keyword-name        ( kw -- name )\n\
keyword=            ( obj1 obj2 -- f )\n\
keyword?            ( obj -- f )\n\
make-keyword        ( name -- kw )"

#define KEYWORD_PREFIX ':'

int
fth_keyword_p(const char *name)
{
	return (fth_any_symbol_p(name, KEYWORD_PREFIX));
}

static void
ficl_keyword_p(ficlVm *vm)
{
#define h_keyword_p "( obj -- f )  test if OBJ is a keyword\n\
:test  keyword? => #t\n\
\"test\" keyword? => #f\n\
Return #t if OBJ is a keyword, otherwise #f."
	FTH 		obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = ficlStackPopFTH(vm->dataStack);
	ficlStackPushBoolean(vm->dataStack, FTH_KEYWORD_P(obj));
}

FTH
fth_keyword(const char *name)
{
	return (make_symbol(name, NULL, KEYWORD_PREFIX, FW_KEYWORD));
}

static void
ficl_create_keyword(ficlVm *vm)
{
#define h_create_keyword "( \"name\" -- )  create keyword (parse word)\n\
create-keyword new-keyword\n\
:new-keyword => :new-keyword\n\
An alternative way to create keywords is:\n\
:NAME => :NAME\n\
Create keyword NAME prepended by a :.  \
Keywords are actually values (variables) named :NAME.\n\
See also make-keyword and keyword-intern."
	ficlVmGetWordToPad(vm);
	fth_keyword(vm->pad);
}

static FTH
fth_make_keyword(FTH name)
{
#define h_make_keyword "( name -- kw )  return keyword\n\
\"new-keyword\" make-keyword drop\n\
:new-keyword => :new-keyword\n\
An alternative way to create keywords is:\n\
:NAME => :NAME\n\
Return keyword NAME prepended by a :.  \
Keywords are actually values (variables) named :NAME.\n\
See also create-keyword and keyword-intern."
	FTH_ASSERT_ARGS(FTH_STRING_P(name), name, FTH_ARG1, "a string");
	return (fth_keyword(fth_string_ref(name)));
}

static void
ficl_print_keyword(ficlVm *vm)
{
#define h_print_keyword "( kw -- )  print keyword\n\
:test .keyword => test\n\
Print keyword KW to current output."
	FTH 		obj;

	FTH_STACK_CHECK(vm, 1, 0);
	obj = ficlStackPopFTH(vm->dataStack);

	if (FTH_KEYWORD_P(obj))
		fth_print(fth_keyword_ref(obj));
	else
		fth_print("not a keyword");
}

static void
ficl_keyword_name(ficlVm *vm)
{
#define h_keyword_name "( kw -- name )  return keyword name\n\
:test keyword-name => \"test\"\n\
Return name of keyword KW."
	FTH 		obj;

	FTH_STACK_CHECK(vm, 1, 0);
	obj = ficlStackPopFTH(vm->dataStack);
	FTH_ASSERT_ARGS(FTH_KEYWORD_P(obj), obj, FTH_ARG1, "a keyword");
	push_cstring(vm, fth_keyword_ref(obj));
}

static void
ficl_keyword_paren(ficlVm *vm)
{
#define h_keyword_paren "( str -- kw )  return keyword\n\
In compile state KEYWORD-INTERN used to create a keyword."
	FTH_STACK_CHECK(vm, 1, 1);
	ficlStackPushFTH(vm->dataStack, fth_keyword(pop_cstring(vm)));
}

static ficlWord *keyword_paren;

static void
ficl_keyword_intern_im(ficlVm *vm)
{
#define h_keyword_intern_im "( \"str\" -- sym )  return keyword\n\
:hello keyword? => #t\n\
Prefix word; return new or existing keyword.  \
Predefined is:\n\
: : postpone keyword-intern ; immediate\n\
See also create-keyword and make-keyword."
	ficlVmGetWordToPad(vm);

	if (vm->state == FICL_VM_STATE_COMPILE) {
		ficlDictionary *dict;
		ficlUnsigned	u;

		dict = ficlVmGetDictionary(vm);
		u = (ficlUnsigned) ficlInstructionLiteralParen;
		ficlDictionaryAppendUnsigned(dict, u);
		ficlDictionaryAppendFTH(dict, fth_make_string(vm->pad));
		ficlDictionaryAppendPointer(dict, keyword_paren);
	} else
		ficlStackPushFTH(vm->dataStack, fth_keyword(vm->pad));
}

/* === EXCEPTION === */

FTH
fth_symbol_or_exception_ref(FTH obj)
{
	if (FTH_SYMBOL_P(obj))
		return (fth_symbol_to_exception(obj));

	if (FTH_EXCEPTION_P(obj))
		return (obj);

	return (FTH_FALSE);
}

int
fth_symbol_or_exception_p(FTH obj)
{
	return (FTH_SYMBOL_P(obj) || FTH_EXCEPTION_P(obj));
}

#define h_list_of_exception_functions "\
*** EXCEPTION PRIMITIVES ***\n\
.exception             	    ( ex -- )\n\
create-exception       	    ( msg \"name\" -- )\n\
exception-last-message-ref  ( ex -- msg )\n\
exception-last-message-set! ( ex msg -- )\n\
exception-message-ref  	    ( ex -- msg )\n\
exception-message-set! 	    ( ex msg -- )\n\
exception-name         	    ( ex -- name )\n\
exception=             	    ( obj1 obj2 -- f )\n\
exception?             	    ( obj -- f )\n\
make-exception         	    ( name msg -- ex )\n\
symbol->exception      	    ( sym -- ex )\n\
*** VARIABLE ***\n\
*last-exception*"

static void
ficl_exception_p(ficlVm *vm)
{
#define h_exception_p "( obj -- f )  test if OBJ is an exception\n\
'test symbol->exception exception? => #t\n\
\"test\" exception? => #f\n\
Return #t if OBJ is an exception, otherwise #f."
	FTH 		obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = ficlStackPopFTH(vm->dataStack);
	ficlStackPushBoolean(vm->dataStack, FTH_EXCEPTION_P(obj));
}

static FTH 	exception_list;

FTH
fth_make_exception(const char *name, const char *message)
{
	FTH 		ex;

	ex = make_symbol(name, message, SYMBOL_PREFIX, FW_EXCEPTION);

	if (!fth_array_member_p(exception_list, ex))
		fth_array_push(exception_list, ex);

	return (ex);
}

/*
 * Return a new Exception object NAME.
 */
FTH
fth_exception(const char *name)
{
	return (fth_make_exception(name, NULL));
}

static void
ficl_create_exception(ficlVm *vm)
{
#define h_create_exception "( msg \"name\" -- )  create exception\n\
\"a special exception\" create-exception special-exception\n\
'special-exception exception-message-ref => \"a special exception\"\n\
#f create-exception exception-w/o-message\n\
'exception-w/o-message exception-message-ref => #f\n\
Create exception named NAME with message MSG, MSG can be #f.  \
The exception has a symbol name, that means it has prefix ' before NAME.\n\
See also make-exception."
	FTH_STACK_CHECK(vm, 1, 0);
	ficlVmGetWordToPad(vm);
	/* msg may be a string or #f */
	fth_make_exception(vm->pad, fth_string_ref(fth_pop_ficl_cell(vm)));
}

static void
ficl_make_exception(ficlVm *vm)
{
#define h_make_exception "( name msg -- ex )  return exception\n\
\"special-excpetion\" \"a special exception\" make-exception drop\n\
'special-exception exception-message-ref => \"a special exception\"\n\
\"exception-w/o-message\" #f make-exception drop\n\
'exception-w/o-message exception-message-ref => #f\n\
Return exception named NAME with message MSG, MSG can be #f.  \
The exception has a symbol name, that means it has prefix ' before NAME.\n\
See also create-exception."
	char           *name;
	FTH 		msg, ex;

	FTH_STACK_CHECK(vm, 2, 1);
	/* msg may be a string or #f */
	msg = fth_pop_ficl_cell(vm);
	name = pop_cstring(vm);
	ex = fth_make_exception(name, fth_string_ref(msg));
	ficlStackPushFTH(vm->dataStack, ex);
}

FTH
fth_symbol_to_exception(FTH symbol)
{
#define h_symbol_to_exception "( sym -- ex )  return exception\n\
'test symbol? => #t\n\
'test symbol->exception => 'test\n\
'test exception? => #t\n\
'test symbol? => #f\n\
Return symbol SYM as exception."
	ficlWord       *sym;

	if (FTH_EXCEPTION_P(symbol))
		return (symbol);

	if (!FTH_SYMBOL_P(symbol)) {
		FTH_ASSERT_ARGS(FTH_SYMBOL_P(symbol), symbol, FTH_ARG1,
		    "an exception or symbol");
		/* NOTREACHED */
		return (FTH_FALSE);
	}
	sym = FICL_WORD_REF(fth_symbol(FICL_WORD_NAME(symbol)));

	if (sym == NULL)
		return (FTH_FALSE);

	sym->kind = FW_EXCEPTION;
	return ((FTH) sym);
}

static void
ficl_print_exception(ficlVm *vm)
{
#define h_print_exception "( ex -- )  print exception\n\
'test .exception => test\n\
Print exception EX to current output."
	FTH 		obj;

	FTH_STACK_CHECK(vm, 1, 0);
	obj = ficlStackPopFTH(vm->dataStack);

	if (FTH_EXCEPTION_P(obj))
		fth_print(fth_exception_ref(obj));
	else
		fth_print("not an exception");
}

static void
ficl_exception_name(ficlVm *vm)
{
#define h_exception_name "( ex -- name )  return exception name\n\
'test exception-name => \"test\"\n\
Return name of exception EX."
	FTH 		obj;

	FTH_STACK_CHECK(vm, 1, 0);
	obj = ficlStackPopFTH(vm->dataStack);
	FTH_ASSERT_ARGS(fth_symbol_or_exception_p(obj), obj, FTH_ARG1,
	    "an exception or symbol");
	push_cstring(vm, fth_exception_ref(obj));
}

FTH
fth_exception_message_ref(FTH exc)
{
	return (fth_word_property_ref(exc, FTH_SYMBOL_MESSAGE));
}

static void
ficl_exception_message_ref(ficlVm *vm)
{
#define h_exc_msg_ref "( ex -- msg )  return exception message\n\
'test exception-message-ref => \"test's special message\"\n\
Return message of exception EX.\n\
See also exception-message-set!."
	FTH 		msg;

	FTH_STACK_CHECK(vm, 1, 1);
	/* msg may be a string or #f */
	msg = fth_exception_message_ref(ficlStackPopFTH(vm->dataStack));
	fth_push_ficl_cell(vm, msg);
}

void
fth_exception_message_set(FTH exc, FTH msg)
{
	fth_word_property_set(exc, FTH_SYMBOL_MESSAGE, msg);
}

static void
ficl_exception_message_set(ficlVm *vm)
{
#define h_exc_msg_set "( ex msg|#f -- )  set exception message\n\
'test \"new special message\" exception-message-set!\n\
'test #f                    exception-message-set!\n\
Set MSG, a string or #f, to exception EX.  \
See also exception-message-ref."
	FTH 		exc, msg;

	FTH_STACK_CHECK(vm, 2, 0);
	/* msg may be a string or #f */
	msg = fth_pop_ficl_cell(vm);
	exc = ficlStackPopFTH(vm->dataStack);
	fth_exception_message_set(exc, msg);
}

/* Last message is the output string built from args in fth_throw(). */
FTH
fth_exception_last_message_ref(FTH exc)
{
	FTH 		msg;

	msg = fth_word_property_ref(exc, FTH_SYMBOL_LAST_MESSAGE);

	if (FTH_FALSE_P(msg))
		msg = fth_word_property_ref(exc, FTH_SYMBOL_MESSAGE);

	return (msg);
}

static void
ficl_exception_last_message_ref(ficlVm *vm)
{
#define h_exc_last_msg_ref "( ex -- msg )  return last message\n\
'test exception-last-message-ref => #f\n\
'test #( \"testing: %s\" \"checking last message\" ) fth-throw\n\
'test exception-last-message-ref => \"testing: checking last message\"\n\
Return last message of exception EX.  \
Last message was set after an exception was thrown \
with e.g. fth-throw or fth-raise.\n\
See also exception-last-message-set!."
	FTH 		msg;

	FTH_STACK_CHECK(vm, 1, 1);
	/* msg may be a string or #f */
	msg = fth_exception_last_message_ref(ficlStackPopFTH(vm->dataStack));
	fth_push_ficl_cell(vm, msg);
}

void
fth_exception_last_message_set(FTH exc, FTH msg)
{
	fth_word_property_set(exc, FTH_SYMBOL_LAST_MESSAGE, msg);
}

static void
ficl_exception_last_message_set(ficlVm *vm)
{
#define h_exc_last_msg_set "( ex msg -- )  set last message\n\
'test \"new special message\" exception-last-message-set!\n\
'test #f                    exception-last-message-set!\n\
Set MSG, a string or #f, as last message of exception EX.  \
This will be set automatically after an exception was thrown \
with e.g. fth-throw or fth-raise.\n\
See also exception-last-message-ref."
	FTH 		exc, msg;

	FTH_STACK_CHECK(vm, 2, 0);
	/* msg may be a string or #f */
	msg = fth_pop_ficl_cell(vm);
	exc = ficlStackPopFTH(vm->dataStack);
	fth_exception_last_message_set(exc, msg);
}

static FTH 	ans_exc_list[FICL_VM_STATUS_LAST_ANS];
static FTH 	ficl_exc_list[FICL_VM_STATUS_LAST_FICL];

FTH
ficl_ans_real_exc(int exc)
{
	if (exc <= FICL_VM_STATUS_ABORT &&
	    exc > FICL_VM_STATUS_LAST_ERROR)
		return (ans_exc_list[-exc]);

	if (exc <= FICL_VM_STATUS_INNER_EXIT &&
	    exc > FICL_VM_STATUS_LAST_FICL_ERROR) {
		exc += FICL_VM_STATUS_OFFSET;
		return (ficl_exc_list[-exc]);
	}
	return (FTH_FALSE);
}

void
init_symbol(void)
{
	/* symbol */
	FTH_SYMBOL_DOCUMENTATION;
	FTH_SYMBOL_LAST_MESSAGE;
	FTH_SYMBOL_MESSAGE;
	FTH_SYMBOL_SOURCE;
	FTH_SYMBOL_TRACE_VAR;
	FTH_PRI1("symbol?", ficl_symbol_p, h_symbol_p);
	FTH_PRI1("symbol=", ficl_symbol_equal_p, h_symbol_equal_p);
	FTH_PRI1("create-symbol", ficl_create_symbol, h_create_symbol);
	FTH_PROC("make-symbol", fth_make_symbol, 1, 0, 0, h_make_symbol);
	FTH_PRI1(".symbol", ficl_print_symbol, h_print_symbol);
	FTH_PRI1("symbol-name", ficl_symbol_name, h_symbol_name);
	FTH_PRIM_IM("symbol-intern", ficl_symbol_intern_im,
	    h_symbol_intern_im);
	symbol_paren = ficlDictionaryAppendPrimitive(FTH_FICL_DICT(),
	    "(symbol)", ficl_symbol_paren, FICL_WORD_DEFAULT);
	FTH_ADD_FEATURE_AND_INFO(FTH_STR_SYMBOL, h_list_of_symbol_functions);

	/* keyword */
	FTH_KEYWORD_CLOSE;
	FTH_KEYWORD_COMMAND;
	FTH_KEYWORD_COUNT;
	FTH_KEYWORD_DOMAIN;
	FTH_KEYWORD_FAM;
	FTH_KEYWORD_FILENAME;
	FTH_KEYWORD_FLUSH;
	FTH_KEYWORD_IF_EXISTS;
	FTH_KEYWORD_INIT;
	FTH_KEYWORD_N;
	FTH_KEYWORD_PORT;
	FTH_KEYWORD_PORT_NAME;
	FTH_KEYWORD_RANGE;
	FTH_KEYWORD_READ_CHAR;
	FTH_KEYWORD_READ_LINE;
	FTH_KEYWORD_REPS;
	FTH_KEYWORD_SOCKET;
	FTH_KEYWORD_SOFT_PORT;
	FTH_KEYWORD_START;
	FTH_KEYWORD_STRING;
	FTH_KEYWORD_WHENCE;
	FTH_KEYWORD_WRITE_CHAR;
	FTH_KEYWORD_WRITE_LINE;
	FTH_PRI1("keyword?", ficl_keyword_p, h_keyword_p);
	FTH_PRI1("keyword=", ficl_keyword_equal_p, h_keyword_equal_p);
	FTH_PRI1("create-keyword", ficl_create_keyword, h_create_keyword);
	FTH_PROC("make-keyword", fth_make_keyword, 1, 0, 0, h_make_keyword);
	FTH_PRI1(".keyword", ficl_print_keyword, h_print_keyword);
	FTH_PRI1("keyword-name", ficl_keyword_name, h_keyword_name);
	FTH_PRIM_IM("keyword-intern", ficl_keyword_intern_im,
	    h_keyword_intern_im);
	keyword_paren = ficlDictionaryAppendPrimitive(FTH_FICL_DICT(),
	    "(keyword)", ficl_keyword_paren, FICL_WORD_DEFAULT);
	FTH_ADD_FEATURE_AND_INFO(FTH_STR_KEYWORD, h_list_of_keyword_functions);

	/* exceptions */
	exception_list = fth_make_empty_array();
	fth_define_variable("*exception-list*", exception_list,
	    "( -- exceptions-array )");
	fth_make_exception(STR_BAD_ARITY, "proc has bad arity");
	fth_make_exception(STR_BAD_SYNTAX, "syntax error");
	fth_make_exception(STR_BIGNUM_ERROR, "bignum error");
	fth_make_exception(STR_CATCH_ERROR, "catch--throw mismatch");
	fth_make_exception(STR_EVAL_ERROR, "evaluation error");
	fth_make_exception(STR_FICL_ERROR, "Ficl error");
	fth_make_exception(STR_FORTH_ERROR, "Forth error");
	fth_make_exception(STR_LOAD_ERROR, "load error");
	fth_make_exception(STR_MATH_ERROR, "math error");
	fth_make_exception(STR_NULL_STRING, "null string");
	fth_make_exception(STR_NO_MEMORY_ERROR, "no more memory available");
	fth_make_exception(STR_OPTKEY_ERROR, "optkey error");
	fth_make_exception(STR_OUT_OF_RANGE, "args out of range");
	fth_make_exception(STR_REGEXP_ERROR, "regular expression error");
	fth_make_exception(STR_SIGNAL_CAUGHT, "signal received");
	fth_make_exception(STR_SO_FILE_ERROR, "dynamic library load error");
	fth_make_exception(STR_SYSTEM_ERROR, "system error");
	fth_make_exception(STR_WRONG_NUMBER_OF_ARGS,
	    "wrong number of arguments");
	fth_make_exception(STR_WRONG_TYPE_ARG, "wrong argument type");

	{
		char           *n, *m;
		int 		i, j;

		ans_exc_list[0] = FTH_FALSE;
		/* ANS Exceptions. */
		j = -1;
		for (i = 1; i < FICL_VM_STATUS_LAST_ANS; i++, j--) {
			n = ficl_ans_exc_name(j);
			m = ficl_ans_exc_msg(j);
			ans_exc_list[i] = fth_make_exception(n, m);
		}
		/* Ficl Exceptions. */
		j = -FICL_VM_STATUS_OFFSET;
		for (i = 0; i < FICL_VM_STATUS_LAST_FICL; i++, j--) {
			n = ficl_ans_exc_name(j);
			m = ficl_ans_exc_msg(j);
			ficl_exc_list[i] = fth_make_exception(n, m);
		}
	}

	FTH_PRI1("exception?", ficl_exception_p, h_exception_p);
	FTH_PRI1("exception=", ficl_exception_equal_p, h_exception_equal_p);
	FTH_PRI1("create-exception", ficl_create_exception,
	    h_create_exception);
	FTH_PRI1("make-exception", ficl_make_exception, h_make_exception);
	FTH_PROC("symbol->exception", fth_symbol_to_exception,
	    1, 0, 0, h_symbol_to_exception);
	FTH_PRI1(".exception", ficl_print_exception, h_print_exception);
	FTH_PRI1("exception-name", ficl_exception_name, h_exception_name);
	FTH_PRI1("exception-message-ref", ficl_exception_message_ref,
	    h_exc_msg_ref);
	FTH_PRI1("exception-message-set!", ficl_exception_message_set,
	    h_exc_msg_set);
	FTH_PRI1("exception-last-message-ref", ficl_exception_last_message_ref,
	    h_exc_last_msg_ref);
	FTH_PRI1("exception-last-message-set!", ficl_exception_last_message_set,
	    h_exc_last_msg_set);
	fth_define_variable("*last-exception*", fth_last_exception,
	    "( -- last set exc )");
	FTH_ADD_FEATURE_AND_INFO(FTH_STR_EXCEPTION,
	    h_list_of_exception_functions);
}

/*
 * symbol.c ends here
 */
