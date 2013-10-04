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
 * @(#)utils.c	1.208 9/13/13
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "fth.h"
#include "utils.h"

#if defined(HAVE_SYS_STAT_H)
#include <sys/stat.h>
#endif
#if defined(HAVE_SIGNAL_H)
#include <signal.h>
#endif

static char    *do_strncat(char *d, size_t size, const char *s, size_t count);
static char    *do_strncpy(char *d, size_t size, const char *s, size_t count);
static void	ficl_cold(void);
static void	ficl_repl_cb(void);
static void    *fixup_null_alloc(size_t n, const char *name);
static int	get_pos_from_buffer(ficlVm *vm, char *delim);
static ficlString parse_input_buffer_0(ficlVm *vm, int pos);
#if defined(HAVE_LIBTECLA)
static int	repl_init_history(void);
#else
static char    *readline(char *prompt);
#endif
static void	utils_throw_error(FTH exc, FTH args, bool with_lisp_fmt_p);

char *
fth_strerror(int n)
{
#if defined(HAVE_STRERROR)
	return (strerror(n));
#else
	static char mesg[32];

	if (n < 0 || n >= sys_nerr) {
		snprintf(mesg, sizeof(mesg), "Unknown error (%d)", n);
		return (mesg);
	} else
		return ((char *)sys_errlist[n]);
#endif
}

/* === xmalloc.c === */

#if defined(STDC_HEADERS)
#include <stdio.h>
#include <stdlib.h>
#else
void           *malloc();
void           *realloc();
void		free();
#endif

static void *
fixup_null_alloc(size_t n, const char *name)
{
	void *p = NULL;

	if (n == 0)
		p = malloc(1L);
	if (p == NULL) {
		fprintf(stderr,
		    "FTH (%s): memory exhausted, last size %zu\n", name, n);
		abort();	/* Not exit() here, a debugger should have a
				 * chance. */
	}
	return (p);
}

void *
fth_malloc(size_t n)
{
	void *p;

	p = malloc(n);
	if (p == NULL)
		p = fixup_null_alloc(n, c__FUNCTION__);
	return (p);
}

void *
fth_realloc(void *p, size_t n)
{
	if (p == NULL)
		return (fth_malloc(n));
	p = realloc(p, n);
	if (p == NULL)
		p = fixup_null_alloc(n, c__FUNCTION__);
	return (p);
}

void *
fth_calloc(size_t count, size_t eltsize)
{
	size_t size;

	size = count * eltsize;
	return (memset(fth_malloc(size), 0, size));
}

void
fth_free(void *ptr)
{
	if (ptr != NULL)
		free(ptr);
	ptr = NULL;
}

char *
fth_strdup(const char *s)
{
	if (s != NULL) {
		size_t len;

		len = strlen(s) + 1;
		if (len > 1)
			return ((char *)memcpy(FTH_MALLOC(len), s, len));
	}
	return (NULL);
}

size_t
fth_strlen(const char *s)
{
	if (s != NULL)
		return (strlen(s));
	return (0);
}

/*
 * fth_strcpy
 * fth_strncpy
 * fth_strcat
 * fth_strncat
 *
 *       char *D: destination
 *   size_t SIZE: max size of destination
 * const char *S: source
 *  size_t COUNT: count chars to copy/append from s to d
 *
 *        return: address of filled destination
 *
 * Copy not more than SIZE - 1 chars to D and append '\0'.
 */

#define CHECK_STR3ARGS(Dst, Siz, Src)					\
	if ((Dst) == NULL || (Siz) == 0 || (Src) == NULL)		\
		return (Dst)

#define CHECK_STR4ARGS(Dst, Siz, Src, Cnt)				\
	if ((Dst) == NULL || (Siz) == 0 || (Src) == NULL || (Cnt) == 0)	\
		return (Dst)

static char *
do_strncpy(char *d, size_t size, const char *s, size_t count)
{
	size--;
	if (size < count)
		count = size;
	d[0] = '\0';
	return (strncat(d, s, count));
}

static char *
do_strncat(char *d, size_t size, const char *s, size_t count)
{
	size -= strlen(d);
	size--;
	if (size < count)
		count = size;
	return (strncat(d, s, count));
}

char *
fth_strcpy(char *d, size_t size, const char *s)
{
	CHECK_STR3ARGS(d, size, s);
	return (do_strncpy(d, size, s, strlen(s)));
}

char *
fth_strncpy(char *d, size_t size, const char *s, size_t count)
{
	CHECK_STR4ARGS(d, size, s, count);
	return (do_strncpy(d, size, s, count));
}

char *
fth_strcat(char *d, size_t size, const char *s)
{
	CHECK_STR3ARGS(d, size, s);
	return (do_strncat(d, size, s, strlen(s)));
}

char *
fth_strncat(char *d, size_t size, const char *s, size_t count)
{
	CHECK_STR4ARGS(d, size, s, count);
	return (do_strncat(d, size, s, count));
}

char *
fth_getenv(const char *name, char *def)
{
	char *value;

	value = NULL;
#if defined(HAVE_ISSETUGID)
	if (issetugid() == 0)
#endif
#if defined(HAVE_GETENV)
		value = getenv(name);
#endif
	return (value != NULL ? value : def);
}

/* === Eval and Execute Wrapper === */

#if !defined(_WIN32)
/* Evaluate input lines. */
int
fth_evaluate(ficlVm *vm, const char *buffer)
{
	volatile int status, sig;

	status = FICL_VM_STATUS_OUT_OF_TEXT;
	if (buffer != NULL) {
		gc_push(vm->runningWord);
		sig = sigsetjmp(fth_sig_toplevel, 1);
		if (sig == 0)
			status = ficlVmEvaluate(vm, (char *)buffer);
		else
			signal_check(sig);
		gc_pop();
	}
	return (status);
}

/* Execute one word. */
int
fth_execute_xt(ficlVm *vm, ficlWord *word)
{
	volatile int status, sig;

	status = FICL_VM_STATUS_OUT_OF_TEXT;
	if (word != NULL) {
		gc_push(word);
		sig = sigsetjmp(fth_sig_toplevel, 1);
		if (sig == 0)
			status = ficlVmExecuteXT(vm, word);
		else
			signal_check(sig);
		gc_pop();
	}
	return (status);
}

#else				/* _WIN32 */

int
fth_evaluate(ficlVm *vm, const char *buffer)
{
	int status;

	status = FICL_VM_STATUS_OUT_OF_TEXT;
	if (buffer != NULL) {
		status = ficlVmEvaluate(vm, (char *)buffer);
	}
	return (status);
}

int
fth_execute_xt(ficlVm *vm, ficlWord *word)
{
	int status;

	status = FICL_VM_STATUS_OUT_OF_TEXT;
	if (word != NULL) {
		status = ficlVmExecuteXT(vm, word);
	}
	return (status);
}
#endif				/* _WIN32 */

/* === Utilities === */

char *
pop_cstring(ficlVm *vm)
{
	return (fth_string_ref(ficlStackPopFTH(vm->dataStack)));
}

void
push_cstring(ficlVm *vm, char *s)
{
	ficlStackPushFTH(vm->dataStack, fth_make_string(s));
}

void
push_forth_string(ficlVm *vm, char *string)
{
	if (string == NULL)
		string = "";
	ficlStackPushPointer(vm->dataStack, string);
	ficlStackPushUnsigned(vm->dataStack, strlen(string));
}

void
fth_throw(FTH obj, const char *fmt,...)
{
	ficlVm *vm;
	FTH fs, exc;

	vm = FTH_FICL_VM();
	fs = fth_make_empty_string();
	exc = fth_symbol_or_exception_ref(obj);
	if (FTH_NOT_FALSE_P(exc))
		fth_variable_set("*last-exception*", fth_last_exception = exc);
	fth_hit_error_p = false;
	if (FTH_NOT_FALSE_P(exc))
		fth_string_sformat(fs, "%s", fth_exception_ref(exc));
	if (fmt != NULL) {
		va_list ap;

		if (FTH_NOT_FALSE_P(exc))
			fth_string_sformat(fs, " in ");
		va_start(ap, fmt);
		fth_string_vsformat(fs, fmt, ap);
		va_end(ap);
	} else if (FTH_FALSE_P(exc)) {
		fth_string_sformat(fs, "%s", c__FUNCTION__);
		if (errno != 0)
			fth_string_sformat(fs, ": %s", fth_strerror(errno));
	} else
		fth_string_sformat(fs, ": %S", fth_exception_message_ref(exc));
	fth_set_backtrace(exc);
	fth_exception_last_message_set(exc, fs);
	/* We don't come from fth-catch, do we? */
	if (!vm->fth_catch_p) {
		fth_hit_error_p = true;
		if (fth_eval_p)
			fth_errorf("\n");
		fth_errorf("#<%S>\n", fs);
		fth_show_backtrace(false);
		errno = 0;
		fth_reset_loop_and_depth();
		ficlVmReset(vm);
	}
	ficlVmThrow(vm, FICL_VM_STATUS_ABORT);
	/*
	 * XXX: abort/error-exit
	 * ficlVmThrow(vm, FICL_VM_STATUS_ERROR_EXIT);
	 */
}

/*
 * Throw exception EXC with text built from ARGS.
 * If ARGS is not an array, ARGS's string representation is used.
 * If ARGS is FTH_NIL or an empty array, a default string is used.
 * If ARGS is an array with one element, this string is used.
 * If ARGS is an array and its first element is a format string with N
 * %s-format signs, ARGS should have N more elements with corresponding
 * values.
 *
 * ARGS: any object		=> string representation of ARGS
 *       nil or #()		=> default exception message
 *       #( str )		=> STR
 *       #( fmt arg1 arg2 ... )	=> formatted string
 *
 * fth_throw_error(FTH_BAD_ARITY, proc);
 *   => #<bad-arity in test-proc> 
 *
 * fth_throw_error(FTH_BAD_ARITY, FTH_NIL);
 *   => #<bad-arity: proc has bad arity>
 *
 * fth_throw_error(FTH_BAD_ARITY, FTH_LIST_1(fth_make_string("test-proc")));
 *   => #<bad-arity in test-proc>
 *
 * fth_throw_error(FTH_BAD_ARITY,
 *     FTH_LIST_4(fth_make_string("%s: %s args required, got %s"),
 *                proc,
 *                FTH_TWO,
 *                FTH_THREE));
 *   => #<bad-arity in test-proc: 2 args required, got 2>
 */
void
fth_throw_error(FTH exc, FTH args)
{
	utils_throw_error(exc, args, false);
}

/*
 * The same like fth_throw_error except for replacing format signs ~A
 * and ~S with %s.  This Function exists for usage in Snd where these
 * signs appear.
 */
void
fth_throw_list(FTH exc, FTH args)
{
	utils_throw_error(exc, args, true);
}

static void
utils_throw_error(FTH exc, FTH args, bool with_lisp_fmt_p)
{
	ficlInteger len;
	FTH fmt;

	if (!fth_symbol_or_exception_p(exc))
		exc = FTH_FORTH_ERROR;
	len = fth_array_length(args);
	switch (len) {
	case -1:
		/*
		 * non-array object
		 */
		if (!FTH_NIL_P(args)) {
			fth_throw(exc, "%S", args);
			/* NOTREACHED */
			break;
		} /* else */
		/* FALLTHROUGH */
	case 0:
		/*
		 * nil, #()
		 */
		fth_throw(exc, NULL);
		/* NOTREACHED */
		break;
	case 1:
		/*
		 * #( str )
		 * [0]: simple string
		 */
		fth_throw(exc, "%S", fth_array_ref(args, 0L)); 
		/* NOTREACHED */
		break;
	default:
		/*
		 * #( fmt arg1 arg2 ... )
		 *     [0]: format string
		 * [1..-1]: arguments required by format string
		 */
		fmt = fth_array_shift(args);
		if (with_lisp_fmt_p && fth_string_length(fmt) > 1) {
			FTH fmt_s;

			fmt_s = fth_make_string("%s");
			fth_string_replace(fmt, fth_make_string("~A"), fmt_s);
			fth_string_replace(fmt, fth_make_string("~S"), fmt_s);
		}
		fth_throw(exc, "%S", fth_string_format(fmt, args));
		/* NOTREACHED */
		break;
	}
}

static int
get_pos_from_buffer(ficlVm *vm, char *delim)
{
	char *str, *tmp;

	tmp = str = ficlVmGetInBuf(vm);
	while ((tmp = strstr(tmp, delim)) != NULL && (*(tmp - 1) == '\\'))
		tmp++;
	return ((tmp != NULL && tmp >= str) ? (int)(tmp - str) : -1);
}

#define FTH_BUFFER_LENGTH               (8192 * sizeof(FTH))
static char	buffer_result[FTH_BUFFER_LENGTH + 1];

/*
 * Returned string must be freed.
 */
char *
parse_tib_with_restart(ficlVm *vm, char *delim, int skip,
    ficlString (*parse_buffer)(ficlVm *vm, int pos))
{
	char *buf = NULL;
	int pos;
	ficlString s;

	pos = get_pos_from_buffer(vm, delim);
	if (pos == -1)
		skip = -1;
	else if (pos >= skip)
		skip += pos;
	s = (*parse_buffer)(vm, skip);
	fth_strncat(buffer_result, sizeof(buffer_result), s.text, s.length);
	if (pos == -1)
		ficlVmThrow(vm, FICL_VM_STATUS_RESTART);
	else {
		if (strlen(buffer_result) > 0)
			buf = FTH_STRDUP(buffer_result);
		buffer_result[0] = '\0';
	}
	return (buf);
}

static ficlString
parse_input_buffer_0(ficlVm *vm, int pos)
{
	char *trace, *stop, *tmp;
	int esc, cur_pos;
	ficlString s;

	trace = ficlVmGetInBuf(vm);
	stop = ficlVmGetInBufEnd(vm);
	tmp = trace;
	FICL_STRING_SET_POINTER(s, trace);
	for (cur_pos = 0, esc = 0;
	    trace != stop && (esc != 0 || cur_pos != pos);
	    cur_pos++, trace++) {
		if (*trace == '\\' || esc == 1)
			esc++;
		if (esc == 1)
			continue;
		if (esc == 2) {
			switch (*trace) {
			case 'a':
				*tmp++ = '\a';
				break;
			case 'b':
				*tmp++ = '\b';
				break;
			case 'e':
				*tmp++ = '\e';
				break;
			case 'f':
				*tmp++ = '\f';
				break;
			case 'n':
				*tmp++ = '\n';
				break;
			case 'r':
				*tmp++ = '\r';
				break;
			case 't':
				*tmp++ = '\t';
				break;
			case 'v':
				*tmp++ = '\v';
				break;
			case '"':
				*tmp++ = '\"';
				break;
			case '\\':
				*tmp++ = '\\';
				break;
			case '\n':	/* discard trailing \n */
				break;
			default:
				*tmp++ = *trace;
				break;
			}
			esc = 0;
			continue;
		}
		*tmp++ = *trace;
	}
	FICL_STRING_SET_LENGTH(s, tmp - s.text);
	if (trace != stop && cur_pos == pos)
		trace++;
	ficlVmUpdateTib(vm, trace);
	return (s);
}

/*
 * Returned string must be freed.
 * TIB points after first char of delim.
 */
char *
parse_input_buffer(ficlVm *vm, char *delim)
{
	return (parse_tib_with_restart(vm, delim, 0, parse_input_buffer_0));
}

/* === Simple Array === */

simple_array *
make_simple_array(int incr)
{
	simple_array *ary;

	if (incr <= 0)
		incr = 8;
	if (incr > 128)
		incr = 128;
	ary = FTH_MALLOC(sizeof(simple_array));
	ary->incr = (unsigned int)incr;
	ary->length = 0;
	ary->data = NULL;
	return (ary);
}

simple_array *
make_simple_array_var(int len,...)
{
	int i;
	va_list list;
	simple_array *ary;

	ary = make_simple_array(len);
	va_start(list, len);
	for (i = 0; i < len; i++)
		simple_array_push(ary, (void *)va_arg(list, void *));
	va_end(list);
	return (ary);
}

int
simple_array_length(simple_array *ary)
{
	if (ary == NULL)
		return (-1);
	return ((int)ary->length);
}

bool
simple_array_equal_p(simple_array *ary1, simple_array *ary2)
{
	if ((ary1 != NULL && ary2 != NULL) &&
	    (ary1->length == ary2->length && ary1->incr == ary2->incr)) {
		unsigned int i;

		for (i = 0; i < ary1->length; i++)
			if (!fth_object_equal_p((FTH)ary1->data[i],
			    (FTH)ary2->data[i]))
				return (false);
		return (true);
	}
	return (false);
}

void *
simple_array_ref(simple_array *ary, int i)
{
	if (ary != NULL && i >= 0 && i < (int)ary->length)
		return (ary->data[i]);
	return (NULL);
}

void
simple_array_set(simple_array *ary, int i, void *obj)
{
	if (ary != NULL && i >= 0 && i < (int)ary->length)
		ary->data[i] = obj;
}

void
simple_array_push(simple_array *ary, void *obj)
{
	if (ary == NULL)
		return;
	if (ary->data == NULL || (ary->length % ary->incr) == 0)
		ary->data = FTH_REALLOC(ary->data,
		    (ary->length + ary->incr) * sizeof(void *));
	ary->data[ary->length++] = obj;
}

void *
simple_array_pop(simple_array *ary)
{
	void *obj = NULL;

	if (ary != NULL && ary->length > 0) {
		ary->length--;
		obj = ary->data[ary->length];
		if (ary->length == 0)
			simple_array_clear(ary);
	}
	return (obj);
}

int
simple_array_index(simple_array *ary, void *obj)
{
	if (ary != NULL && ary->length > 0) {
		unsigned int i;

		for (i = 0; i < ary->length; i++)
			if (ary->data[i] == obj)
				return ((int)i);
	}
	return (-1);
}

int
simple_array_rindex(simple_array *ary, void *obj)
{
	if (ary != NULL && ary->length > 0) {
		int i;

		for (i = (int)ary->length - 1; i >= 0; i--)
			if (ary->data[i] == obj)
				return (i);
	}
	return (-1);
}

bool
simple_array_member_p(simple_array *ary, void *obj)
{
	return (simple_array_index(ary, obj) != -1);
}

void *
simple_array_delete(simple_array *ary, void *obj)
{
	int i;
	unsigned int ui;

	i = simple_array_index(ary, obj);
	if (i == -1)
		return (NULL);
	ui = (unsigned int)i;
	ary->length--;
	if (ary->length == 0)
		simple_array_clear(ary);
	else if (ui < ary->length)
		memmove(ary->data + ui, ary->data + ui + 1,
		    sizeof(void *) * (ary->length - ui));
	return (obj);
}

void *
simple_array_rdelete(simple_array *ary, void *obj)
{
	int i;
	unsigned int ui;

	i = simple_array_rindex(ary, obj);
	if (i == -1)
		return (NULL);
	ui = (unsigned int)i;
	ary->length--;
	if (ary->length == 0)
		simple_array_clear(ary);
	else if (ui < ary->length)
		memmove(ary->data + ui, ary->data + ui + 1,
		    sizeof(void *) * (ary->length - ui));
	return (obj);
}

simple_array *
simple_array_reverse(simple_array *ary)
{
	if (ary != NULL && ary->length > 0) {
		int i;
		simple_array *new;

		new = make_simple_array((int)ary->incr);
		for (i = (int)ary->length - 1; i >= 0; i--)
			simple_array_push(new, ary->data[i]);
		return (new);
	}
	return (NULL);
}

simple_array *
simple_array_clear(simple_array *ary)
{
	if (ary == NULL)
		return (NULL);
	FTH_FREE(ary->data);
	ary->data = NULL;
	ary->length = 0;
	return (ary);
}

void
simple_array_free(simple_array *ary)
{
	if (ary == NULL)
		return;
	simple_array_clear(ary);
	FTH_FREE(ary);
	ary = NULL;
}

FTH
simple_array_to_array(simple_array *ary)
{
	if (ary != NULL && ary->length > 0) {
		ficlInteger i;
		FTH array;

		array = fth_make_array_len((ficlInteger)ary->length);
		for (i = 0; i < (int)ary->length; i++)
			fth_array_fast_set(array, i, (FTH)ary->data[i]);
		return (array);
	}
	return (fth_make_empty_array());
}

#include <stdio.h>
#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif

FTH
fth_set_argv(int from, int to, char **argv)
{
	static bool in_set_argv = false;
	int i;
	FTH args;

	if (from > to)
		return (FTH_FALSE);
	if (!in_set_argv) {
		in_set_argv = true;
		args = FTH_LIST_1(fth_make_string(fth_basename(argv[from++])));
		for (i = from; i < to; i++)
			fth_array_push(args, fth_make_string(argv[i]));
		fth_variable_set("*argc*",
		    fth_make_int(fth_array_length(args)));
		return (fth_variable_set("*argv*", args));
	}
	return (fth_variable_ref("*argv*"));
}

static FTH	before_repl_hook;
static FTH	after_repl_hook;
static FTH	before_prompt_hook;
static bool	fth_in_repl_p = false;

#if defined(HAVE_LIBTECLA)

#if defined(HAVE_LIBTECLA_H)
#include <libtecla.h>
#endif

/* ARGSUSED */
static int
repl_command_generator(WordCompletion *cpl,
    void *data, const char *line, int word_end)
{
	ficlDictionary *dict;
	ficlHash *hash;
	ficlWord *word;
	CplMatches *matches;
	const char *text;
	size_t len;
	int i, j;

	/*
	 * Find begin of word.
	 */
	for (i = word_end; i >= 0; i--)
		if (isspace((int)line[i]))
			break;
	len = (size_t)(word_end - ++i);
	text = line + i;
	dict = FTH_FICL_DICT();
	/*
	 * Search for words beginning with TEXT.
	 */
	for (i = (int)dict->wordlistCount - 1; i >= 0; i--)
		for (hash = dict->wordlists[i];
		    hash != NULL;
		    hash = hash->link)
			for (j = (int)hash->size - 1; j >= 0; j--)
				for (word = hash->table[j];
				    word != NULL;
				    word = word->link)
					if (word->length > 0 &&
					    strncmp(word->name, text, len) == 0)
						/*
						 * If found word is the only
						 * completion, append a
						 * space.
						 */
						cpl_add_completion(cpl,
						    line, i, word_end,
						    word->name + len,
						    NULL, " ");
	matches = cpl_recall_matches(cpl);
	/*
	 * If nothing was found, check for file name completion.
	 */
	if (matches->nmatch == 0)
		cpl_file_completions(cpl, NULL, line, word_end);
	return (0);
}

static char	hist_file[MAXPATHLEN];

static int
repl_init_history(void)
{
	char *h_file, *len_str;
	int hist_len;
	size_t h_len;

	h_len = sizeof(hist_file);
	h_file = getenv(FTH_ENV_HIST);
	if (h_file == NULL) {
		fth_strcpy(hist_file, h_len, fth_getenv("HOME", "/tmp"));
		fth_strcat(hist_file, h_len, "/" FTH_HIST_FILE);
	} else
		fth_strcpy(hist_file, h_len, h_file);
	hist_len = 0;
	len_str = getenv(FTH_ENV_HIST_LEN);
	if (len_str != NULL)
		hist_len = (int)strtol(len_str, NULL, 10);
	if (hist_len < FTH_HIST_LEN)
		hist_len = FTH_HIST_LEN;
	return (hist_len);
}

void
fth_repl(int argc, char **argv)
{
	static ficlInteger lineno;
	ficlVm *vm;
	char *line;
	char *volatile prompt;
	volatile bool compile_p;
	volatile int status;
#if !defined(_WIN32)
	volatile int sig;
#endif
	volatile int hist_len;
	int i;
	GetLine *gl;
	GlHistoryRange range;

	vm = FTH_FICL_VM();
	prompt = NULL;
	compile_p = false;
	status = FICL_VM_STATUS_OUT_OF_TEXT;
	fth_current_file = fth_make_string("repl-eval");
	fth_in_repl_p = true;
	hist_len = repl_init_history();
	gl = new_GetLine(1024L, (size_t)(128 * hist_len));
	if (gl == NULL)
		fth_exit(EXIT_FAILURE);
	gl_load_history(gl, hist_file, "\\");
	gl_automatic_history(gl, 0);
	gl_customize_completion(gl, NULL, repl_command_generator);
	gl_prompt_style(gl, GL_FORMAT_PROMPT);
	fth_set_argv(0, argc, argv);
	/*
	 * Call hook before starting repl.
	 */
	if (!fth_hook_empty_p(before_repl_hook))
		fth_run_hook(before_repl_hook, 0);
	gl_range_of_history(gl, &range);
	lineno = (ficlInteger)range.newest;
	fth_interactive_p = true;
	/*
	 * Main loop.
	 */
	while (status != FTH_BYE) {
		if (compile_p)
			prompt = "c> ";	/* continue prompt */
		else if (!fth_hook_empty_p(before_prompt_hook))
			prompt = fth_string_ref(
			    fth_run_hook_again(before_prompt_hook, 2,
			    fth_make_string(prompt), INT_TO_FIX(lineno)));
		else
			prompt = FTH_REPL_PROMPT;
		fth_print_p = false;
		fth_current_line = lineno++;
#if defined(_WIN32)
		if (true) {
#else

		sig = sigsetjmp(fth_sig_toplevel, 1);
		if (sig == 0) {
#endif
			line = gl_get_line(gl, prompt, NULL, -1);
			if (line == NULL || *line == '\0')
				break;
			if (line != NULL && *line != '\0') {
				line[strlen(line) - 1] = '\0';
				gl_append_history(gl, line);
				status = fth_catch_eval(line);
				if (status == FTH_ERROR)
					continue;
				if (status == FTH_BYE)
					break;
				compile_p =
				    (vm->state == FICL_VM_STATE_COMPILE);
				if (compile_p)
					continue;
				else
					status = FTH_OKAY;
				if (fth_true_repl_p) {
					int len;

					len = FTH_STACK_DEPTH(vm);
					switch (len) {
					case 0:
						if (!fth_print_p)
							fth_printf("%S",
							    FTH_UNDEF);
						break;
					case 1:
						fth_printf("%M",
						    fth_pop_ficl_cell(vm));
						break;
					default:
						for (i = len - 1;
						    i >= 0;
						    i--) {
							ficlStackRoll(
							    vm->dataStack, i);
							fth_printf("%M ",
							    fth_pop_ficl_cell(
							    vm));
						}
						break;
					}
					fth_print("\n");
				} else if (fth_print_p)	/* forth repl */
					fth_print("\n");
			} else if (fth_true_repl_p)	/* empty line */
				fth_printf("%S\n", FTH_UNDEF);
		} else {	/* sig != 0 */
#if !defined(_WIN32)
			signal_check(sig);
#endif
			errno = 0;
			ficlVmReset(vm);
		}
	}			/* while */
	gl_limit_history(gl, hist_len);
	gl_save_history(gl, hist_file, "\\", -1);
	del_GetLine(gl);
	if (fth_print_p)
		fth_print("\n");
	/*
	 * Call hook after finishing repl; for example to remove duplicates
	 * from history file.
	 */
	if (!fth_hook_empty_p(after_repl_hook))
		fth_run_hook(after_repl_hook, 1, fth_make_string(hist_file));
	fth_exit(EXIT_SUCCESS);
}

#else				/* !HAVE_LIBTECLA */

static char	utils_readline_buffer[BUFSIZ];

static char *
readline(char *prompt)
{
	char *buf;

	if (prompt != NULL)
		fth_print(prompt);
	buf = utils_readline_buffer;
	buf[0] = '\0';
	fgets(buf, BUFSIZ, stdin);
	if (*buf == '\0')	/* Ctrl-D */
		return (NULL);
	return (buf);
}

void
fth_repl(int argc, char **argv)
{
	static ficlInteger lineno;
	ficlVm *vm;
	char *volatile prompt;
	char *volatile rl_buffer;
	volatile bool compile_p;
	volatile int status;
#if !defined(_WIN32)
	volatile int sig;
#endif
	int i;

	lineno = 0;
	vm = FTH_FICL_VM();
	prompt = NULL;
	rl_buffer = NULL;
	compile_p = false;
	status = FICL_VM_STATUS_OUT_OF_TEXT;
	fth_current_file = fth_make_string("repl-eval");
	fth_in_repl_p = true;
	fth_set_argv(0, argc, argv);
	if (!fth_hook_empty_p(before_repl_hook))
		fth_run_hook(before_repl_hook, 0);
	fth_interactive_p = true;
	while (status != FTH_BYE) {
		if (compile_p)
			prompt = "c> ";	/* continue prompt */
		else if (!fth_hook_empty_p(before_prompt_hook))
			prompt = fth_string_ref(
			    fth_run_hook_again(before_prompt_hook, 2,
			    fth_make_string((char *)prompt),
			    INT_TO_FIX(lineno)));
		else
			prompt = FTH_REPL_PROMPT;
		rl_buffer = NULL;
		fth_print_p = false;
		fth_current_line = lineno++;
#if defined(_WIN32)
		if (true) {
#else
		sig = sigsetjmp(fth_sig_toplevel, 1);
		if (sig == 0) {
#endif

			rl_buffer = readline(prompt);
			if (rl_buffer == NULL) {	/* EOF/ctrl-d */
				status = FTH_BYE;
				fth_print("\n");
				continue;
			}
			if (rl_buffer != NULL) {
				status = fth_catch_eval(rl_buffer);
				if (status == FTH_BYE)
					break;
				if (status == FTH_ERROR)
					continue;
				compile_p =
				    (vm->state == FICL_VM_STATE_COMPILE);
				if (compile_p)
					continue;
				if (fth_true_repl_p) {
					int len;

					len = FTH_STACK_DEPTH(vm);
					switch (len) {
					case 0:
						if (!fth_print_p)
							fth_printf("%I",
							    FTH_UNDEF);
						break;
					case 1:
						fth_printf("%M",
						    fth_pop_ficl_cell(vm));
						break;
					default:
						for (i = len - 1;
						    i >= 0; i--) {
							ficlStackRoll(
							    vm->dataStack, i);
							fth_printf("%M ",
							    fth_pop_ficl_cell(
							    vm));
						}
						break;
					}
					fth_print("\n");
				} else if (fth_print_p)	/* forth repl */
					fth_print("\n");
			} else if (fth_true_repl_p)	/* empty line */
				fth_printf("%I\n", FTH_UNDEF);
		} else {	/* sig != 0 */
#if !defined(_WIN32)
			signal_check(sig);
#endif
			errno = 0;
			ficlVmReset(vm);
		}
	}			/* while */
	fth_exit(EXIT_SUCCESS);
}

#endif				/* !HAVE_LIBTECLA */

static void
ficl_repl_cb(void)
{
	if (FTH_TRUE_P(fth_variable_ref("*fth-verbose*"))) {
		fth_print("\\\n");
		fth_print("\\ type help <word> to get help, \
e.g. `help make-array'\n");
		fth_print("\\ type C-c for break\n");
		fth_print("\\ type C-\\ for fast exit\n");
		fth_print("\\ type C-d or `bye' for exit\n");
		fth_print("\\\n");
	}
}

static void
ficl_cold(void)
{
#define h_cold "( -- )  Resets ficl system."
	fth_reset_loop_and_depth();
	ficlVmReset(FTH_FICL_VM());
	if (fth_in_repl_p)
		if (!fth_hook_empty_p(before_repl_hook))
			fth_run_hook(before_repl_hook, 0);
	errno = 0;
	ficlVmThrow(FTH_FICL_VM(), FICL_VM_STATUS_QUIT);
}

void
init_utils(void)
{
#if defined(HAVE_LIBTECLA)
	fth_add_feature("libtecla");
#endif
	fth_define_variable("*argc*", FTH_ZERO,
	    "number of arguments in *argv*");
	fth_define_variable("*argv*", fth_make_empty_array(),
	    "command line args array");
	fth_define_variable("*line*", fth_make_string(""),
	    "current in-place processing line");
	fth_define_variable("*farray*", FTH_FALSE,
	    "auto-splitted array of current line");
	fth_define_variable("*fname*", fth_make_string("-"),
	    "current in-place file name");
	fth_define_variable("*fs*", fth_make_string(" "),
	    "input field separator");
	fth_define_variable("*fnr*", FTH_ZERO,
	    "input record number in current file");
	fth_define_variable("*nr*", FTH_ZERO,
	    "input record number since beginning");
	fth_define_void_procedure("cold", ficl_cold, 0, 0, false, h_cold);
	before_repl_hook = fth_make_hook("before-repl-hook", 0,
	    "before-repl-hook ( -- )  \
Called after initializing the tecla(7) command-line \
editing library but before starting the repl.  \
A predefined hook showing some help lines can be \
replaced by your own message:\n\
before-repl-hook reset-hook!\n\
before-repl-hook lambda: <{ -- }>\n\
  .\" \\\" cr\n\
  .\" \\ Starting FTH on \" date .string .\" !\" cr\n\
  .\" \\\" cr\n\
; add-hook!");
	fth_add_hook(before_repl_hook,
	    fth_make_proc_from_vfunc("repl-cb", ficl_repl_cb, 0, 0, false));
	after_repl_hook = fth_make_hook("after-repl-hook", 1,
	    "after-repl-hook ( history-file -- )  \
Called after leaving the repl and writing the \
history file but before leaving the program.  \
Its only argument is the history filename.  \
You may manipulate the history data entries.  \
One history entry consists of two lines: \
a time stamp preceded by a Forth comment backslash \
and the actual history line.\n\
after-repl-hook lambda: <{ history -- }>\n\
  \\ Remove duplicates from history file.\n\
  history readlines array-reverse! { hary }\n\
  #() \"\" \"\" { nhary hline tline }\n\
  hary array-length 0 ?do\n\
    hary i    array-ref to hline\n\
    hary i 1+ array-ref to tline\n\
    nhary hline array-member? unless\n\
      hary hline array-unshift tline array-unshift drop\n\
    then\n\
  2 +loop\n\
  history nhary writelines\n\
; add-hook!");
	before_prompt_hook = fth_make_hook("before-prompt-hook", 2,
	    "before-prompt-hook ( prompt pos -- new-prompt )  \
Called before printing a new prompt to customize the output of it.  \
PROMPT is the old prompt and POS the current history position.  \
The return value, preferable a string, is the \
PROMPT argument for the next hook procedure if any.\n\
before-prompt-hook lambda: <{ prompt pos -- new-prompt }>\n\
  \"fth (%d)> \" #( pos ) string-format\n\
; add-hook!");
}

/*
 * utils.c ends here
 */
