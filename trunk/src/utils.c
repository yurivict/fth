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
 * @(#)utils.c	2.1 1/2/18
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

static char    *do_strncat(char *, size_t, const char *, size_t);
static char    *do_strncpy(char *, size_t, const char *, size_t);
static void	ficl_cold(ficlVm *);
#if defined(HAVE_LIBTECLA)
static void	ficl_ps_cb(ficlVm *);
#endif
static void	ficl_repl_cb(ficlVm *);
static void    *fixup_null_alloc(size_t, const char *);
static int	get_pos_from_buffer(ficlVm *, char *);
static ficlString parse_input_buffer_0(ficlVm *, int pos);
static void	utils_throw_error(FTH, FTH, int);

char           *
fth_strerror(int n)
{
#if defined(HAVE_STRERROR)
	return (strerror(n));
#else
	static char 	mesg[32];

	if (n < 0 || n >= sys_nerr) {
		snprintf(mesg, sizeof(mesg), "Unknown error (%d)", n);
		return (mesg);
	}
	return ((char *) sys_errlist[n]);
#endif
}

/* === xmalloc.c === */

#if defined(STDC_HEADERS)
#include <stdio.h>
#include <stdlib.h>
#else
void           *malloc();
void           *realloc();
void 		free();
#endif

static void    *
fixup_null_alloc(size_t n, const char *name)
{
	void           *p = NULL;

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

void           *
fth_malloc(size_t n)
{
	void           *p;

	p = malloc(n);

	if (p == NULL)
		p = fixup_null_alloc(n, "fth_malloc");

	return (p);
}

void           *
fth_realloc(void *p, size_t n)
{
	if (p == NULL)
		return (fth_malloc(n));

	p = realloc(p, n);

	if (p == NULL)
		p = fixup_null_alloc(n, "fth_realloc");

	return (p);
}

void           *
fth_calloc(size_t count, size_t eltsize)
{
	void           *p;

	p = calloc(count, eltsize);

	if (p == NULL) {
		p = fixup_null_alloc(count * eltsize, "fth_calloc");
		memset(p, 0, 1);
	}
	return (p);
}

void
fth_free(void *ptr)
{
	if (ptr != NULL)
		free(ptr);

	ptr = NULL;
}

char           *
fth_strdup(const char *s)
{
	if (s != NULL) {
		size_t 		len;

		len = strlen(s) + 1;

		if (len > 1)
			return ((char *) memcpy(FTH_MALLOC(len), s, len));
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

/*-
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

static char    *
do_strncpy(char *d, size_t size, const char *s, size_t count)
{
	size--;

	if (size < count)
		count = size;

	d[0] = '\0';
	return (strncat(d, s, count));
}

static char    *
do_strncat(char *d, size_t size, const char *s, size_t count)
{
	size -= strlen(d);
	size--;

	if (size < count)
		count = size;

	return (strncat(d, s, count));
}

char           *
fth_strcpy(char *d, size_t size, const char *s)
{
	CHECK_STR3ARGS(d, size, s);
	return (do_strncpy(d, size, s, strlen(s)));
}

char           *
fth_strncpy(char *d, size_t size, const char *s, size_t count)
{
	CHECK_STR4ARGS(d, size, s, count);
	return (do_strncpy(d, size, s, count));
}

char           *
fth_strcat(char *d, size_t size, const char *s)
{
	CHECK_STR3ARGS(d, size, s);
	return (do_strncat(d, size, s, strlen(s)));
}

char           *
fth_strncat(char *d, size_t size, const char *s, size_t count)
{
	CHECK_STR4ARGS(d, size, s, count);
	return (do_strncat(d, size, s, count));
}

char           *
fth_getenv(const char *name, char *def)
{
	char           *value;

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
	volatile int 	status, sig;

	status = FICL_VM_STATUS_OUT_OF_TEXT;

	if (buffer != NULL) {
		gc_push(vm->runningWord);
		sig = sigsetjmp(fth_sig_toplevel, 1);

		if (sig == 0)
			status = ficlVmEvaluate(vm, (char *) buffer);
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
	volatile int 	status, sig;

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
	int 		status;

	status = FICL_VM_STATUS_OUT_OF_TEXT;

	if (buffer != NULL)
		status = ficlVmEvaluate(vm, (char *) buffer);

	return (status);
}

int
fth_execute_xt(ficlVm *vm, ficlWord *word)
{
	int 		status;

	status = FICL_VM_STATUS_OUT_OF_TEXT;

	if (word != NULL)
		status = ficlVmExecuteXT(vm, word);

	return (status);
}
#endif				/* _WIN32 */

/* === Utilities === */

char           *
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
	ficlVm         *vm;
	FTH 		fs, exc;

	vm = FTH_FICL_VM();
	fs = fth_make_empty_string();
	exc = fth_symbol_or_exception_ref(obj);

	if (FTH_NOT_FALSE_P(exc))
		fth_variable_set("*last-exception*", fth_last_exception = exc);

	fth_hit_error_p = 0;

	if (FTH_NOT_FALSE_P(exc))
		fth_string_sformat(fs, "%s", fth_exception_ref(exc));

	if (fmt != NULL) {
		va_list 	ap;

		if (FTH_NOT_FALSE_P(exc))
			fth_string_sformat(fs, " in ");

		va_start(ap, fmt);
		fth_string_vsformat(fs, fmt, ap);
		va_end(ap);
	} else if (FTH_FALSE_P(exc)) {
		fth_string_sformat(fs, "fth_throw");

		if (errno != 0)
			fth_string_sformat(fs, ": %s", fth_strerror(errno));
	} else
		fth_string_sformat(fs, ": %S", fth_exception_message_ref(exc));

	fth_set_backtrace(exc);
	fth_exception_last_message_set(exc, fs);

	/* We don't come from fth-catch, do we? */
	if (!vm->fth_catch_p) {
		fth_hit_error_p = 1;
		if (fth_eval_p)
			fth_errorf("\n");
		fth_errorf("#<%S>\n", fs);
		fth_show_backtrace(0);
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

/*-
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
	utils_throw_error(exc, args, 0);
}

/*
 * The same like fth_throw_error except for replacing format signs ~A
 * and ~S with %s.  This Function exists for usage in Snd where these
 * signs appear.
 */
void
fth_throw_list(FTH exc, FTH args)
{
	utils_throw_error(exc, args, 1);
}

static void
utils_throw_error(FTH exc, FTH args, int with_lisp_fmt_p)
{
	ficlInteger 	len;
	FTH 		fmt;

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
		}		/* else */
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
			FTH 		fmt_s;

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
	char           *str, *tmp;

	tmp = str = ficlVmGetInBuf(vm);

	while ((tmp = strstr(tmp, delim)) != NULL && (*(tmp - 1) == '\\'))
		tmp++;

	return ((tmp != NULL && tmp >= str) ? (int) (tmp - str) : -1);
}

#define FTH_BUFFER_LENGTH               (8192 * sizeof(FTH))
static char 	buffer_result[FTH_BUFFER_LENGTH + 1];

/*
 * Returned string must be freed.
 */
char           *
parse_tib_with_restart(ficlVm *vm, char *delim, int skip,
    ficlString (*parse_buffer) (ficlVm *vm, int pos))
{
	char           *buf = NULL;
	int 		pos;
	ficlString 	s;

	pos = get_pos_from_buffer(vm, delim);

	if (pos == -1)
		skip = -1;
	else if (pos >= skip)
		skip += pos;

	s = (*parse_buffer) (vm, skip);
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
	char           *trace, *stop, *tmp;
	int 		esc, cur_pos;
	ficlString 	s;

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
char           *
parse_input_buffer(ficlVm *vm, char *delim)
{
	return (parse_tib_with_restart(vm, delim, 0, parse_input_buffer_0));
}

/* === Simple Array === */

simple_array   *
make_simple_array(int incr)
{
	simple_array   *ary;

	if (incr <= 0)
		incr = 8;

	if (incr > 128)
		incr = 128;

	ary = FTH_MALLOC(sizeof(simple_array));
	ary->incr = (unsigned int) incr;
	ary->length = 0;
	ary->data = NULL;
	return (ary);
}

simple_array   *
make_simple_array_var(int len,...)
{
	int 		i;
	va_list 	list;
	simple_array   *ary;

	ary = make_simple_array(len);
	va_start(list, len);

	for (i = 0; i < len; i++)
		simple_array_push(ary, (void *) va_arg(list, void *));

	va_end(list);
	return (ary);
}

int
simple_array_length(simple_array *ary)
{
	if (ary == NULL)
		return (-1);

	return ((int) ary->length);
}

int
simple_array_equal_p(simple_array *ary1, simple_array *ary2)
{
	if ((ary1 != NULL && ary2 != NULL) &&
	    (ary1->length == ary2->length &&
		ary1->incr == ary2->incr)) {
		unsigned int 	i;

		for (i = 0; i < ary1->length; i++)
			if (!fth_object_equal_p((FTH) ary1->data[i],
				(FTH) ary2->data[i]))
				return (0);

		return (1);
	}
	return (0);
}

void           *
simple_array_ref(simple_array *ary, int i)
{
	if (ary != NULL && i >= 0 && i < (int) ary->length)
		return (ary->data[i]);

	return (NULL);
}

void
simple_array_set(simple_array *ary, int i, void *obj)
{
	if (ary != NULL && i >= 0 && i < (int) ary->length)
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

void           *
simple_array_pop(simple_array *ary)
{
	void           *obj = NULL;

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
		unsigned int 	i;

		for (i = 0; i < ary->length; i++)
			if (ary->data[i] == obj)
				return ((int) i);
	}
	return (-1);
}

int
simple_array_rindex(simple_array *ary, void *obj)
{
	if (ary != NULL && ary->length > 0) {
		int 		i;

		for (i = (int) ary->length - 1; i >= 0; i--)
			if (ary->data[i] == obj)
				return (i);
	}
	return (-1);
}

int
simple_array_member_p(simple_array *ary, void *obj)
{
	return (simple_array_index(ary, obj) != -1);
}

void           *
simple_array_delete(simple_array *ary, void *obj)
{
	int 		i;
	unsigned int 	ui;

	i = simple_array_index(ary, obj);

	if (i == -1)
		return (NULL);

	ui = (unsigned int) i;
	ary->length--;

	if (ary->length == 0)
		simple_array_clear(ary);
	else if (ui < ary->length)
		memmove(ary->data + ui, ary->data + ui + 1,
		    sizeof(void *) * (ary->length - ui));

	return (obj);
}

void           *
simple_array_rdelete(simple_array *ary, void *obj)
{
	int 		i;
	unsigned int 	ui;

	i = simple_array_rindex(ary, obj);

	if (i == -1)
		return (NULL);

	ui = (unsigned int) i;
	ary->length--;

	if (ary->length == 0)
		simple_array_clear(ary);
	else if (ui < ary->length)
		memmove(ary->data + ui, ary->data + ui + 1,
		    sizeof(void *) * (ary->length - ui));

	return (obj);
}

simple_array   *
simple_array_reverse(simple_array *ary)
{
	if (ary != NULL && ary->length > 0) {
		int 		i;
		simple_array   *new;

		new = make_simple_array((int) ary->incr);

		for (i = (int) ary->length - 1; i >= 0; i--)
			simple_array_push(new, ary->data[i]);

		return (new);
	}
	return (NULL);
}

simple_array   *
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
		ficlInteger 	i;
		FTH 		array;

		array = fth_make_array_len((ficlInteger) ary->length);

		for (i = 0; i < (int) ary->length; i++)
			fth_array_fast_set(array, i, (FTH) ary->data[i]);

		return (array);
	}
	return (fth_make_empty_array());
}

FTH
fth_set_argv(int from, int to, char **argv)
{
	int 		i;
	FTH 		args;

	if (from >= to || argv == NULL) {
		fth_variable_set("*argc*", FTH_ZERO);
		return (fth_variable_set("*argv*", FTH_NIL));
	}
	args = FTH_LIST_1(fth_make_string(fth_basename(argv[from++])));

	for (i = from; i < to; i++)
		fth_array_push(args, fth_make_string(argv[i]));

	fth_variable_set("*argc*", INT_TO_FIX(fth_array_length(args)));
	return (fth_variable_set("*argv*", args));
}

static FTH 	before_repl_hook;
static FTH 	after_repl_hook;
static FTH 	before_prompt_hook;
static int	fth_in_repl_p = 0;

#define FTH_REPL_PROMPT		"ok "
#define FTH_REPL_PROMPT2	"c> "
#define FTH_HIST_FILE		".fth-history"
#define FTH_HIST_LEN		100

#define FGL_COMMENT		"\\"
#define FGL_BUFFER		4096

static FTH 	fgl_all;	/* gl-all to *histdup* */
static FTH 	fgl_prev;	/* gl-prev to *histdup* */
static FTH 	fgl_erase;	/* gl-erase to *histdup* */

static FTH 	fgl_vi;		/* edit-mode vi */
static FTH 	fgl_emacs;	/* edit-mode emacs */
static FTH 	fgl_none;	/* edit-mode none */
static FTH 	fgl_nobeep;	/* nobeep */

/* constant (string) */
#define FGL_HISTDUP_REF()	fth_variable_ref("*histdup*")
#define FGL_HISTDUP_ALL_P()	(FGL_HISTDUP_REF() == fgl_all)
#define FGL_HISTDUP_PREV_P()	(FGL_HISTDUP_REF() == fgl_prev)
#define FGL_HISTDUP_ERASE_P()	(FGL_HISTDUP_REF() == fgl_erase)
#define FGL_HISTDUP_UNDEF_P()	FTH_UNDEF_P(FGL_HISTDUP_REF())

/* string */
#define FGL_HISTFILE_REF()	fth_variable_ref("*histfile*")
#define FGL_HISTFILE_SET(Val)	fth_variable_set("*histfile*", (Val))
#define FGL_HISTFILE_CSTR()	fth_string_ref(FGL_HISTFILE_REF())

/* integer */
#define FGL_HISTORY_REF()	fth_variable_ref("*history*")
#define FGL_HISTORY_SET(Val)	fth_variable_set("*history*", (Val))
#define FGL_HISTORY_CINT()	FIX_TO_INT(FGL_HISTORY_REF())
#define FGL_HISTORY_CSET(Val)	FGL_HISTORY_SET(INT_TO_FIX(Val))

/* boolean */
#define FGL_PROMPTSTYLE_P()	FTH_TRUE_P(fth_variable_ref("*promptstyle*"))
#define FGL_SAVEHIST_P()	FTH_TRUE_P(fth_variable_ref("*savehist*"))

#if defined(HAVE_LIBTECLA)

#if defined(HAVE_LIBTECLA_H)
#include <libtecla.h>
#endif

static GetLine *repl_gl_init(void);
static void 	ficl_history(ficlVm *);
static void 	ficl_history_lineno(ficlVm *);
static void 	ficl_history_next(ficlVm *);
static void 	ficl_history_prev(ficlVm *);
static int 	gl_config(GetLine *, FTH);
static void 	ficl_bindkey(ficlVm *);

static int	repl_command_generator(WordCompletion *,
		    void *, const char *, int);
static int 	repl_init_history(void);
static int 	repl_unique_history(GetLine *, char *);
static int 	repl_append_history(GetLine *, char *);
static char    *repl_expand_history(GetLine *, char *);
static char    *repl_replace_history(GetLine *, char *);

static simple_array *fgl_getline_config;
static simple_array *fgl_getline_bindkey;

#define FTH_GET_LINE_ERROR	fth_exception("getline-error")
#define FTH_GL_ERROR(gl)						\
	fth_throw(FTH_GET_LINE_ERROR,					\
	    "%s: %s", RUNNING_WORD(), gl_error_message(gl, NULL, 0))

static FTH 	fgl_show;	/* gl-show [n] history */
static FTH 	fgl_load;	/* gl-load [fname] history */
static FTH 	fgl_save;	/* gl-save [fname] history */
static FTH 	fgl_add;	/* gl-add line history */
static FTH 	fgl_clear;	/* gl-clear history */

static ficlUnsigned gl_cur_event;

static GetLine *
repl_gl_init(void)
{
	ficlVm         *vm;
	GetLine        *gl;

	vm = FTH_FICL_VM();
	gl = ficlVmGetRepl(vm);

	if (gl == NULL) {
		gl = new_GetLine(FGL_BUFFER, FGL_BUFFER * repl_init_history());

		if (gl == NULL) {
			fprintf(stderr, "cannot initialize GetLine\n");
			return (NULL);
		}
		gl_automatic_history(gl, 0);
		gl_customize_completion(gl, NULL, repl_command_generator);
		gl_prompt_style(gl, GL_FORMAT_PROMPT);
		gl_bind_keyseq(gl, GL_USER_KEY, "^C", "user-interrupt");
		gl_bind_keyseq(gl, GL_USER_KEY, "^G", "ring-bell");
		ficlVmGetRepl(vm) = gl;
	}
	return (gl);
}

static void
ficl_history(ficlVm *vm)
{
#define h_history "( :optional action arg -- )  handle histoy events\n\
             history => return entire history as string\n\
gl-show      history => same as above\n\
        10   history => show 10 last history events\n\
gl-show 10   history => same as above\n\
gl-load      history => load from *histfile*\n\
gl-load nil  history => same as above\n\
gl-load file history => load from file\n\
gl-save      history => save to *histfile*\n\
gl-save nil  history => same as above\n\
gl-save file history => save to file\n\
gl-add  line history => add line to history\n\
gl-clear     history => clear all history events\n\
gl-show: return ARG or all history events as string.\n\
gl-load: load from ARG or *histfile* (~/" FTH_HIST_FILE ").\n\
gl-save: save to ARG or *histfile*.\n\
gl-add: add event ARG to history.\n\
gl-clear: clear all history events."
	GetLine        *gl;
	int 		len, n;
	FTH 		action, arg;

	arg = FTH_FALSE;
	n = -1;
	gl = ficlVmGetRepl(vm);

	if (gl == NULL)
		gl = repl_gl_init();

	len = FTH_STACK_DEPTH(vm);

	if (len == 0)
		action = fgl_show;
	else if (len == 1) {
		action = fth_pop_ficl_cell(vm);

		if (FTH_INTEGER_P(action)) {
			n = (int) FTH_INT_REF(action);
			action = fgl_show;
		}
	} else {
		arg = fth_pop_ficl_cell(vm);
		action = fth_pop_ficl_cell(vm);
	}
	if (action == fgl_load) {
		char           *fname;

		fname = fth_string_ref(arg);

		if (fname == NULL)
			fname = FGL_HISTFILE_CSTR();

		if (gl_load_history(gl, fname, FGL_COMMENT) != 0) {
			fth_warning("%s: can't load %s", RUNNING_WORD(), fname);
			return;
		}
	} else if (action == fgl_save) {
		char           *fname;

		fname = fth_string_ref(arg);

		if (fname == NULL)
			fname = FGL_HISTFILE_CSTR();

		if (gl_save_history(gl, fname, FGL_COMMENT, -1) != 0) {
			fth_warning("%s: can't save %s", RUNNING_WORD(), fname);
			return;
		}
	} else if (action == fgl_add) {
		char           *line;

		line = fth_string_ref(arg);

		if (line == NULL || *line == '\0')
			return;

		repl_append_history(gl, line);
	} else if (action == fgl_clear)
		gl_clear_history(gl, 1);
	else {
		FILE           *fp;
		int 		fd;
		char 		b[16] = "";
		char 		buf[BUFSIZ];
		FTH 		fs;

		snprintf(b, sizeof(b), "/tmp/fth.XXXXXX");
		fp = NULL;
		fd = mkstemp(b);

		if (fd != -1)
			fp = fdopen(fd, "w+");

		if (fp == NULL) {
			if (fd != -1) {
				unlink(b);
				close(fd);
			}
			fth_warning("%s: %s %s",
			    RUNNING_WORD(), b, fth_strerror(errno));
			return;
		}
		if (FTH_INTEGER_P(arg))
			n = (int) FTH_INT_REF(arg);

		/*
		 * XXX: gl_show_history and stdout
		 *      We send stdout to a tmp file, read it from
		 *      there and put it on stack.
		 *	This works well in Snd's listener.
		 */
		gl_show_history(gl, fp, "%N  %T   %H\n", 1, n);
		rewind(fp);
		fs = fth_make_empty_string();

		while (fgets(buf, sizeof(buf), fp) != NULL)
			fth_string_scat(fs, buf);

		unlink(b);
		fclose(fp);
		ficlStackPushFTH(vm->dataStack, fs);
	}
}

static void
ficl_history_lineno(ficlVm *vm)
{
#define h_hist_lineno "( -- line )"
	GetLine        *gl;
	GlHistoryRange 	range;

	gl = ficlVmGetRepl(vm);

	if (gl == NULL)
		gl = repl_gl_init();

	gl_range_of_history(gl, &range);
	gl_cur_event = range.newest;
	ficlStackPushUnsigned(vm->dataStack, gl_cur_event);
}

static void
ficl_history_next(ficlVm *vm)
{
#define h_hist_next "( -- line )"
	const char     *line;
	GetLine        *gl;
	GlHistoryRange 	range;
	GlHistoryLine 	hline;

	gl = ficlVmGetRepl(vm);

	if (gl == NULL)
		gl = repl_gl_init();

	gl_range_of_history(gl, &range);
	gl_cur_event++;

	/*
	 * If greater than newest, start from the end.
	 */
	if (gl_cur_event > range.newest || gl_cur_event < range.oldest)
		gl_cur_event = range.oldest;

	line = "";

	if (gl_lookup_history(gl, gl_cur_event, &hline))
		line = hline.line;

	fth_push_ficl_cell(vm, fth_make_string(line));
}

static void
ficl_history_prev(ficlVm *vm)
{
#define h_hist_prev "( -- line )"
	const char     *line;
	GetLine        *gl;
	GlHistoryRange 	range;
	GlHistoryLine 	hline;

	gl = ficlVmGetRepl(vm);

	if (gl == NULL)
		gl = repl_gl_init();

	gl_range_of_history(gl, &range);
	gl_cur_event--;

	/*
	 * If less than oldest, start from the beginning.
	 */
	if (gl_cur_event < range.oldest || gl_cur_event > range.newest)
		gl_cur_event = range.newest;

	line = "";

	if (gl_lookup_history(gl, gl_cur_event, &hline))
		line = hline.line;

	fth_push_ficl_cell(vm, fth_make_string(line));
}

#define FGL_TECLA_RC		"~/.teclarc"

static int
gl_config(GetLine * gl, FTH action)
{
	char           *app;

	if (action == fgl_vi)
		app = "edit-mode vi";
	else if (action == fgl_emacs)
		app = "edit-mode emacs";
	else if (action == fgl_none)
		app = "edit-mode none";
	else if (action == fgl_nobeep)
		app = "nobeep";
	else
		app = fth_string_ref(action);

	simple_array_push(fgl_getline_config, app);

	if (gl != NULL)
		return (gl_configure_getline(gl, app, NULL, NULL));

	return (0);
}

static void
ficl_bindkey(ficlVm *vm)
{
#define h_bindkey "( :optional key action -- )  bind or unbind keys\n\
                         bindkey => show user-defined key-bindings\n\
\"edit-mode vi \\n nobeep\" bindkey => configure getline\n\
gl-vi                    bindkey => set getline to vi mode\n\
\"^G\" \"user-interrupt\"    bindkey => bind user-interrupt to Ctrl-G\n\
\"^G\" #f                  bindkey => unbind last bind from Ctrl-G\n\
No argument:\n\
Show user-defined key-bindings set for example in ~/.fthrc.\n\
One argument (KEY):\n\
If KEY is a string, take it as configure string.  \
If KEY is a predefined constant, set specific value as configure string.\n\
Valid constants:\n\
	gl-vi		=> edit-mode vi\n\
	gl-emacs	=> edit-mode emacs\n\
	gl-none		=> edit-mode none\n\
	gl-nobeep	=> nobeep\n\
Two arguments (KEY ACTION):\n\
If KEY and ACTION are strings, bind ACTION to KEY.  \
If KEY is a string and ACTION is anything else, unbind KEY from last bind.\n\
See tecla(7) for key-bindings and actions."
	GetLine        *gl;
	FTH 		key, action;
	char           *k, *a;
	int 		i, l, len;

	gl = ficlVmGetRepl(vm);
	len = FTH_STACK_DEPTH(vm);

	switch (len) {
	case 0:
		/*-
		 * show current key-bindings and configurations
		 */
		l = simple_array_length(fgl_getline_config);

		for (i = 0; i < l; i++) {
			k = simple_array_ref(fgl_getline_config, i);
			fth_printf("config[%d]: %s\n", i, k);
		}

		l = simple_array_length(fgl_getline_bindkey);

		for (i = 0; i < l; i += 2) {
			k = simple_array_ref(fgl_getline_bindkey, i);
			a = simple_array_ref(fgl_getline_bindkey, i + 1);
			if (a == NULL)
				a = "undef";
			fth_printf("bindkey[%d]: %s => %s\n", i, k, a);
		}
		break;
	case 1:
		/*-
		 * gl_configure_getline()
		 *
		 * KEY is string:
		 *	"edit-mode vi \n nobeep" bindkey
		 *
		 * KEY is one of the predefined constants:
		 *	gl-vi		bindkey
		 *	gl-emacs	bindkey
		 *	gl-none		bindkey
		 *	gl-nobeep	bindkey
		 */
		key = fth_pop_ficl_cell(vm);

		if (gl_config(gl, key)) {
			FTH_GL_ERROR(gl);
			/* NOTREACHED */
			return;
		}
		break;
	case 2:
	default:
		/*-
		 * gl_bind_keyseq()
		 *
		 * KEY must be a string!
		 * ACTION can be a string: "^G" "user-interrupt" bindkey
		 *	binds ACTION to KEY
		 * ACTION is anything else: "^G" #f bindkey
		 *	unbinds KEY from previous bound ACTION
		 */
		action = fth_pop_ficl_cell(vm);
		key = fth_pop_ficl_cell(vm);
		FTH_ASSERT_ARGS(FTH_STRING_P(key), key, FTH_ARG1, "a string");
		k = fth_string_ref(key);
		a = fth_string_ref(action);
		simple_array_push(fgl_getline_bindkey, k);
		simple_array_push(fgl_getline_bindkey, a);

		if (gl != NULL)
			if (gl_bind_keyseq(gl, GL_USER_KEY, k, a)) {
				FTH_GL_ERROR(gl);
				/* NOTREACHED */
				return;
			}
		break;
	}
}

/* ARGSUSED */
static int
repl_command_generator(WordCompletion * cpl,
    void *data, const char *line, int word_end)
{
	ficlDictionary *dict;
	ficlHash       *hash;
	ficlWord       *word;
	CplMatches     *matches;
	const char     *text;
	size_t 		len;
	int 		i, j;

	(void) data;

	/*
	 * Find begin of word.
	 */
	for (i = word_end; i >= 0; i--)
		if (isspace((int) line[i]))
			break;

	len = (size_t) (word_end - ++i);
	text = line + i;
	dict = FTH_FICL_DICT();

	/*
	 * Search for words beginning with TEXT.
	 */
	for (i = (int) dict->wordlistCount - 1; i >= 0; i--)
		for (hash = dict->wordlists[i];
		    hash != NULL;
		    hash = hash->link)
			for (j = (int) hash->size - 1; j >= 0; j--)
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
	 * If nothing was found, check for filename completion.
	 */
	if (matches->nmatch == 0)
		cpl_file_completions(cpl, NULL, line, word_end);

	return (0);
}

static int
repl_init_history(void)
{
	int 		hist_len;
	char           *tmp_str;
	FTH 		fs       , fn;

	/*-
	 * Set history file.
	 *
	 * Check for:
	 * 	- Fth variable *histfile*
	 * 	- environment variable $FTH_HISTORY
	 * 	- otherwise default to FTH_HIST_FILE
	 */
	fs = FGL_HISTFILE_REF();

	if (fth_string_length(fs) <= 0) {
		tmp_str = getenv(FTH_ENV_HIST);

		if (tmp_str == NULL)
			fs = fth_make_string_format("%s/" FTH_HIST_FILE,
			    fth_getenv("HOME", "/tmp"));
		else
			fs = fth_make_string(tmp_str);

		FGL_HISTFILE_SET(fs);
	}

	/*-
	 * Set history length.
	 *
	 * Check for:
	 * 	- Fth variable *history*
	 * 	- environment variable $FTH_HISTORY_LENGTH
	 * 	- otherwise default to FTH_HIST_LEN
	 */
	fn = FGL_HISTORY_REF();
	hist_len = (int) fth_int_ref_or_else(fn, -1);

	if (hist_len == -1) {
		tmp_str = getenv(FTH_ENV_HIST_LEN);

		if (tmp_str != NULL)
			hist_len = (int) strtol(tmp_str, NULL, 10);
	}
	if (hist_len < FTH_HIST_LEN)
		hist_len = FTH_HIST_LEN;

	FGL_HISTORY_CSET(hist_len);
	return (hist_len);
}

static int
repl_unique_history(GetLine * gl, char *line)
{
	char           *hf;
	FTH 		hary, nhary, cline, hline;
	ficlInteger 	i, len;

	hf = FGL_HISTFILE_CSTR();
	gl_save_history(gl, hf, FGL_COMMENT, -1);
	hary = fth_array_reverse(fth_readlines(hf));
	cline = fth_make_string_format("%s\n", line);
	nhary = fth_make_empty_array();
	len = fth_array_length(hary) - 1;

	for (i = 0; i < len; i += 2) {
		hline = fth_array_ref(hary, i);

		if (fth_string_equal_p(cline, hline))
			continue;

		if (fth_array_member_p(nhary, hline))
			continue;

		/* history line */
		fth_array_unshift(nhary, hline);
		/* time line */
		fth_array_unshift(nhary, fth_array_ref(hary, i + 1));
	}

	fth_writelines(hf, nhary);
	gl_clear_history(gl, 1);
	return (gl_load_history(gl, hf, FGL_COMMENT));
}

/*-
 * XXX: gl_append_history() (Wed Jan 15 18:10:07 CET 2014)
 *
 * According to libtecla's source files, gl_append_history() adds only
 * unique history lines.  But this doesn't seem to work.
 *
 * Here we try a tcsh-like scheme where *histdup* can be set to gl-all,
 * gl-prev, gl-erase, or undef.
 */
static int
repl_append_history(GetLine * gl, char *line)
{
	unsigned long 	id;
	GlHistoryRange 	range;
	GlHistoryLine 	hline;

	/* replace '\n' */
	line[strlen(line)] = '\0';

	if (FGL_HISTDUP_ALL_P()) {
		gl_range_of_history(gl, &range);
		for (id = range.newest; id > range.oldest; id--)
			if (gl_lookup_history(gl, id, &hline))
				if (strcmp(hline.line, line) == 0)
					return (0);
	} else if (FGL_HISTDUP_PREV_P()) {
		gl_range_of_history(gl, &range);
		if (gl_lookup_history(gl, range.newest, &hline))
			if (strcmp(hline.line, line) == 0)
				return (0);
	} else if (FGL_HISTDUP_ERASE_P()) {
		gl_range_of_history(gl, &range);
		for (id = range.newest; id > range.oldest; id--)
			if (gl_lookup_history(gl, id, &hline))
				if (strcmp(hline.line, line) == 0) {
					repl_unique_history(gl, line);
					break;
				}
	}
	return (gl_append_history(gl, line));
}

/*-
 * If command line starts with !, try to substitute with commands from
 * previous history events.
 *
 * !123			repeat event 123
 * !-123		repeat 123rd last event
 * !!			repeat last event (same as !-1)
 * !?sub_string(?)	repeat last event containing SUB_STRING
 * !start_of_string	repeat last event starting with START_OF_STRING
 */
static char    *
repl_expand_history(GetLine * gl, char *line)
{
	unsigned long 	id;
	GlHistoryRange 	range;
	GlHistoryLine 	hline;
	long 		ld;
	size_t 		ln;
	char 		s[FGL_BUFFER + 1], *res;

	ln = strlen(line);

	if (ln < 2)
		return (line);

	/* skip '!' sign */
	strcpy(s, line + 1);
	ln--;
	/* adjust length to minus trailing '\n' */
	ln--;
	/* remove trailing '\n' */
	s[ln] = '\0';
	res = NULL;
	gl_range_of_history(gl, &range);
	id = range.newest;

	if (isdigit((int) *s) || *s == '-') {
		/* !123 or !-123 */
		ld = strtol(s, NULL, 10);

		if (ld < 0)
			id += ++ld;
		else
			id = ld;

		if (gl_lookup_history(gl, id, &hline))
			res = (char *) hline.line;
	} else if (*s == '!') {
		/* !! */
		if (gl_lookup_history(gl, id, &hline))
			res = (char *) hline.line;
	} else if (*s == '?') {
		/* !?sub_string(?) */
		size_t 		sl;
		char           *r;

		r = s + 1;
		sl = strlen(r);

		if (r[sl] == '?')
			r[sl] = '\0';

		for (; id > range.oldest; id--)
			if (gl_lookup_history(gl, id, &hline))
				if (fth_regexp_find(r, hline.line) != -1) {
					res = (char *) hline.line;
					break;
				}
	} else
		/* !start_of_string */
		for (; id > range.oldest; id--)
			if (gl_lookup_history(gl, id, &hline))
				if (strncmp(s, hline.line, ln) == 0) {
					res = (char *) hline.line;
					break;
				}
	if (res == NULL) {
		int 		i;

		i = 0;

		if (*s == '!')
			i++;

		if (*s == '?')
			i++;

		fth_printf("%s: event not found\n", s + i);
	}
	return (res);
}

/*-
 * If command line starts with ^, try a search from previous history
 * events and replace it with second part of ^search^replace.
 *
 * ^search^replace(^)
 */
static char    *
repl_replace_history(GetLine * gl, char *line)
{
	unsigned long 	id;
	GlHistoryRange 	range;
	GlHistoryLine 	hline;
	size_t 		i, j;
	char 		src[FGL_BUFFER], dst[FGL_BUFFER], *res;

	if (strlen(line) < 4)
		return (line);

	i = 0;
	j = 1;			/* skip 1st ^ */

	while (line[j] != '^' && !isspace((int) line[j]))
		src[i++] = line[j++];

	src[i] = '\0';
	i = 0;
	j++;			/* skip 2nd ^ */

	while (line[j] != '^' && !isspace((int) line[j]))
		dst[i++] = line[j++];

	dst[i] = '\0';
	res = NULL;
	gl_range_of_history(gl, &range);
	id = range.newest;

	for (; id > range.oldest; id--)
		if (gl_lookup_history(gl, id, &hline))
			if (fth_regexp_find(src, hline.line) != -1) {
				FTH 		reg, str, rep, rpl;

				reg = fth_make_regexp(src);
				str = fth_make_string(hline.line);
				rep = fth_make_string(dst);
				rpl = fth_regexp_replace(reg, str, rep);
				res = fth_string_ref(rpl);
				break;
			}

	if (res == NULL)
		fth_printf("%s: event not found\n", src);

	return (res);
}

#else				/* !HAVE_LIBTECLA */

static void	ficl_bindkey(ficlVm *vm);
static char    *get_line(char *prompt, char *dummy);
static char	utils_readline_buffer[BUFSIZ];

static void
ficl_bindkey(ficlVm *vm)
{
#define h_bindkey "( :optional key action -- )  noop without libtecla"
	int 		len;

	/* clear at most 2 stack entries */
	len = FTH_STACK_DEPTH(vm);

	switch (len) {
	case 2:
		fth_pop_ficl_cell(vm);
		/* FALLTHROUGH */
	case 1:
		fth_pop_ficl_cell(vm);
		/* FALLTHROUGH */
	default:
		break;
	}
}

/* ARGSUSED */
static char    *
get_line(char *prompt, char *dummy)
{
	char           *buf;

	(void) dummy;

	if (prompt != NULL)
		fth_print(prompt);

	buf = utils_readline_buffer;
	buf[0] = '\0';
	fgets(buf, BUFSIZ, stdin);

	if (*buf == '\0')	/* Ctrl-D */
		return (NULL);

	return (buf);
}

#endif				/* !HAVE_LIBTECLA */

void
fth_repl(int argc, char **argv)
{
	static ficlInteger lineno;
	ficlVm         *vm;
	char           *line;
	char           *volatile prompt;
	char           *volatile err_line;
	volatile int 	compile_p;
	volatile int 	status;
#if !defined(_WIN32)
	volatile int 	sig;
#endif
	int 		i, len;
#if defined(HAVE_LIBTECLA)
	GetLine        *gl;
	GlHistoryRange 	range;
	GlReturnStatus 	rs;
#endif

	vm = FTH_FICL_VM();
	prompt = NULL;
	err_line = NULL;
	compile_p = 0;
	status = FICL_VM_STATUS_OUT_OF_TEXT;
	fth_current_file = fth_make_string("repl-eval");
	fth_in_repl_p = 1;
#if defined(HAVE_LIBTECLA)
	gl = new_GetLine(FGL_BUFFER, FGL_BUFFER * repl_init_history());

	if (gl == NULL) {
		fprintf(stderr, "cannot initialize GetLine\n");
		fth_exit(EXIT_FAILURE);
	}
	ficlVmGetRepl(vm) = gl;

	/*
	 * We have to load explicitely ~/.teclarc.  Subsequent calls of
	 * gl_configure_getline() will trigger gl->configure = 1 and GetLine
	 * will skip further implicite calls of gl_configure_getline() (via
	 * gl_get_line() etc).
	 */
	if (gl_configure_getline(gl, NULL, NULL, FGL_TECLA_RC)) {
		FTH_GL_ERROR(gl);
		/* NOTREACHED */
		return;
	}

	/*
	 * Delayed init of config and bindkey sequences from ~/.fthrc.
	 */
	len = simple_array_length(fgl_getline_config);

	for (i = 0; i < len; i++) {
		char           *app;

		app = simple_array_ref(fgl_getline_config, i);

		if (gl_configure_getline(gl, app, NULL, NULL)) {
			FTH_GL_ERROR(gl);
			/* NOTREACHED */
			return;
		}
	}
	len = simple_array_length(fgl_getline_bindkey);

	for (i = 0; i < len; i += 2) {
		char           *k, *a;

		k = simple_array_ref(fgl_getline_bindkey, i);
		a = simple_array_ref(fgl_getline_bindkey, i + 1);

		if (gl_bind_keyseq(gl, GL_USER_KEY, k, a)) {
			FTH_GL_ERROR(gl);
			/* NOTREACHED */
			return;
		}
	}

	gl_automatic_history(gl, 0);
	gl_customize_completion(gl, NULL, repl_command_generator);
	gl_prompt_style(gl,
	    FGL_PROMPTSTYLE_P() ? GL_FORMAT_PROMPT : GL_LITERAL_PROMPT);
	gl_load_history(gl, FGL_HISTFILE_CSTR(), FGL_COMMENT);
	gl_range_of_history(gl, &range);
	lineno = (ficlInteger) range.newest;
#else
	lineno = 1;
#endif
	fth_set_argv(0, argc, argv);

	/*
	 * Call hook before starting repl.
	 */
	if (!fth_hook_empty_p(before_repl_hook))
		fth_run_hook(before_repl_hook, 0);

	fth_current_line = lineno;
	fth_interactive_p = 1;

	/*
	 * Main loop.
	 */
	while (status != FTH_BYE) {
		line = NULL;
		prompt = NULL;
#if defined(HAVE_LIBTECLA)
		gl_range_of_history(gl, &range);
		lineno = (ficlInteger) range.newest + 1;
#endif
		if (compile_p)
			prompt = FTH_REPL_PROMPT2;	/* continue prompt */
		else if (!fth_hook_empty_p(before_prompt_hook)) {
			FTH 		fs;

			fs = fth_run_hook_again(before_prompt_hook,
			    2,
			    fth_make_string(prompt),
			    INT_TO_FIX(lineno));
			prompt = fth_string_ref(fs);
		}
		if (prompt == NULL)
			prompt = FTH_REPL_PROMPT;

		fth_print_p = 0;
#if !defined(_WIN32)
		sig = sigsetjmp(fth_sig_toplevel, 1);

		if (sig != 0) {
			signal_check(sig);
			errno = 0;
			ficlVmReset(vm);
			continue;
		}
#endif
#if defined(HAVE_LIBTECLA)
		line = gl_get_line(gl, prompt, err_line, -1);

		if (line == NULL) {
			rs = gl_return_status(gl);

			if (rs == GLR_EOF) {
				status = FTH_BYE;
				continue;
			}
			if (rs == GLR_ERROR) {
				FTH_GL_ERROR(gl);
				/* NOTREACHED */
				return;
			}
			continue;
		}

		/*
		 * If command line starts with !, try to substitute with
		 * commands from previous history events.
		 */
		if (*line == '!') {
			line = repl_expand_history(gl, line);

			if (line == NULL)
				continue;

			fth_printf("%s\n", line);
		}

		/*
		 * If command line starts with ^, try a search from previous
		 * history events and replace it with second part of
		 * ^search^replace.
		 */
		if (*line == '^') {
			line = repl_replace_history(gl, line);

			if (line == NULL)
				continue;

			fth_printf("%s\n", line);
		}
#else
		line = get_line(prompt, err_line);

		if (line == NULL)	/* Ctrl-D finishes repl */
			break;
#endif
		if (*line == '\n') {	/* empty line */
			if (fth_true_repl_p)
				fth_printf("%S\n", FTH_UNDEF);
			continue;
		}
#if defined(HAVE_LIBTECLA)
		repl_append_history(gl, line);
#endif
		status = fth_catch_eval(line);

		if (status == FTH_ERROR) {
			/*
			 * If an error occures, show the wrong command again.
			 */
			err_line = line;
			continue;
		}
		err_line = NULL;

		if (status == FTH_BYE)
			break;

		fth_current_line = lineno++;
		compile_p = (vm->state == FICL_VM_STATE_COMPILE);

		if (compile_p)
			continue;

		status = FTH_OKAY;

		if (!fth_true_repl_p) {	/* forth repl */
			if (fth_print_p)
				fth_print("\n");
			continue;
		}
		len = FTH_STACK_DEPTH(vm);

		switch (len) {
		case 0:
			if (!fth_print_p)
				fth_printf("%S", FTH_UNDEF);
			break;
		case 1:
			fth_printf("%S", fth_pop_ficl_cell(vm));
			break;
		default:
			for (i = len - 1; i >= 0; i--) {
				ficlStackRoll(vm->dataStack, i);
				fth_printf("%S ", fth_pop_ficl_cell(vm));
			}
			break;
		}

		fth_print("\n");
	}			/* while */
#if defined(HAVE_LIBTECLA)
	if (FGL_SAVEHIST_P())
		gl_save_history(gl, FGL_HISTFILE_CSTR(), FGL_COMMENT,
		    FGL_HISTORY_CINT());

	ficlVmGetRepl(vm) = gl = del_GetLine(gl);
#endif
	if (fth_print_p)
		fth_print("\n");

	/*
	 * Call hook after finishing repl.
	 */
	if (!fth_hook_empty_p(after_repl_hook))
		fth_run_hook(after_repl_hook, 1, FGL_HISTFILE_REF());

	fth_exit(EXIT_SUCCESS);
}

static void
ficl_repl_cb(ficlVm *vm)
{
#define h_repl_cb "( -- )  show some hints at startup\n\
A hard coded before-repl-hook.  \
Before adding your own, do:\n\
before-repl-hook reset-hook!."
	(void) vm;
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
ficl_cold(ficlVm *vm)
{
#define h_cold "( -- )  reset ficl system."
	fth_reset_loop_and_depth();
	ficlVmReset(vm);

	if (fth_in_repl_p)
		if (!fth_hook_empty_p(before_repl_hook))
			fth_run_hook(before_repl_hook, 0);

	errno = 0;
	ficlVmThrow(vm, FICL_VM_STATUS_QUIT);
}

#if defined(HAVE_LIBTECLA)
static void
ficl_ps_cb(ficlVm *vm)
{
#define h_ps_cb "( val -- res )  callback for setting *promptstyle*."
	int		fp;
	GetLine        *gl;

	FTH_STACK_CHECK(vm, 1, 1);
	fp = ficlStackPopBoolean(vm->dataStack);
	gl = ficlVmGetRepl(vm);
	gl_prompt_style(gl, fp ? GL_FORMAT_PROMPT : GL_LITERAL_PROMPT);
	ficlStackPushBoolean(vm->dataStack, fp);
}
#endif

void
init_utils(void)
{
#define FGL_MAKE_STRING_CONSTANT(Str)					\
	fth_define_constant(Str, fth_make_string(Str), NULL)
#define FGL_SET_CONSTANT(Name) 						\
	fgl_ ##Name = FGL_MAKE_STRING_CONSTANT("gl-" #Name)
#if defined(HAVE_LIBTECLA)
	fth_add_feature("tecla");
	fth_add_feature("libtecla");

	FTH_GET_LINE_ERROR;
	fgl_getline_config = make_simple_array(16);
	fgl_getline_bindkey = make_simple_array(16);

	/* history constants */
	FGL_SET_CONSTANT(show);
	FGL_SET_CONSTANT(load);
	FGL_SET_CONSTANT(save);
	FGL_SET_CONSTANT(add);
	FGL_SET_CONSTANT(clear);
	FTH_PRI1("history-lineno", ficl_history_lineno, h_hist_lineno);
	FTH_PRI1("history-next", ficl_history_next, h_hist_next);
	FTH_PRI1("history-prev", ficl_history_prev, h_hist_prev);
	FTH_PRI1("history", ficl_history, h_history);
#endif
	/* bindkey constants */
	FGL_SET_CONSTANT(vi);
	FGL_SET_CONSTANT(emacs);
	FGL_SET_CONSTANT(none);
	FGL_SET_CONSTANT(nobeep);
	FTH_PRI1("bindkey", ficl_bindkey, h_bindkey);

	/* *histdup* constants */
	FGL_SET_CONSTANT(all);
	FGL_SET_CONSTANT(prev);
	FGL_SET_CONSTANT(erase);
	fth_define_variable("*histdup*", FTH_UNDEF,
	    "History variable (constant).\n\
If set to GL-ALL, only unique history events are entered in the history list.  \
If set to GL-PREV and the last history event is the same as the current, \
the current command is not entered.  \
If set to GL-ERASE and the same event is found in the history list, \
that old event gets erased and the current one gets inserted.  \
If not defined (undef, the default), all history events are entered.\n\
Default is undef.");
	fth_define_variable("*histfile*", FTH_UNDEF,
	    "History variable (string).\n\
Can be set to the pathname where history is going to be saved and restored.  \
If not set, use $FTH_HISTORY or ~/" FTH_HIST_FILE ".\n\
Default is undef.");
	fth_define_variable("*history*", FTH_UNDEF,
	    "History variable (numeric).\n\
Can be given a numeric value to control the size of the history list.  \
If not set, use $FTH_HISTORY_LENGTH or " FTH_XString(FTH_HIST_LEN) ".\n\
Default is undef.");
	fth_define_variable("*savehist*", FTH_TRUE,
	    "History variable (boolean).\n\
If true, save at most *history*, $FTH_HISTORY_LENGTH, or \
" FTH_XString(FTH_HIST_LEN) " history events to *history*, $FTH_HISTORY, or ~/\
" FTH_HIST_FILE ", if false, don't save history events.\n\
Default is #t.");
	fth_define_variable("*argc*", FTH_ZERO,
	    "number of arguments in *argv*");
	fth_define_variable("*argv*", fth_make_empty_array(),
	    "command line args array");
	fth_define_variable("*line*", fth_make_string(""),
	    "current in-place processing line");
	fth_define_variable("*farray*", FTH_FALSE,
	    "auto-splitted array of current line");
	fth_define_variable("*fname*", fth_make_string("-"),
	    "current in-place filename");
	fth_define_variable("*fs*", fth_make_string(" "),
	    "input field separator");
	fth_define_variable("*fnr*", FTH_ZERO,
	    "input record number in current file");
	fth_define_variable("*nr*", FTH_ZERO,
	    "input record number since beginning");
	FTH_PRI1("cold", ficl_cold, h_cold);
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
	{
		ficlWord       *ps, *rcb;
#if defined(HAVE_LIBTECLA)
		ficlWord       *pscb;
#endif

		ps = FTH_CONSTANT_SET("*promptstyle*", FTH_FALSE);
		fth_word_doc_set(ps, "Prompt style variable (boolean).\n\
If true, enable special formatting directives within the prompt, \
see gl_prompt_style(3).\n\
Default is #f.");
#if defined(HAVE_LIBTECLA)
		pscb = FTH_PRI1("prompt-style-cb", ficl_ps_cb, h_ps_cb);
		fth_trace_var((FTH) ps, (FTH) pscb);
#endif
		rcb = FTH_PRI1("repl-cb", ficl_repl_cb, h_repl_cb);
		fth_add_hook(before_repl_hook, (FTH) rcb);
	}
}

/*
 * utils.c ends here
 */
