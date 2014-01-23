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
 * @(#)regexp.c	1.102 1/23/14
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "fth.h"
#include "utils.h"
#if defined(HAVE_REGEX_H)
#include <regex.h>
#endif

#if defined(HAVE_POSIX_REGEX)

/* === REGEXP === */

static FTH	regexp_tag;

typedef struct {
	ficlInteger	length;	/* length of regex string */
	char           *data;	/* regex string */
	regex_t		re_buf;	/* compiled regex buffer */
	FTH		results;/* array with matches of entire expression
				 * and possible subexpressions */
} FRegexp;

#define REGEXP_REGS		10L

#define FTH_REGEXP_OBJECT(Obj)	FTH_INSTANCE_REF_GEN(Obj, FRegexp)
#define FTH_REGEXP_LENGTH(Obj)	FTH_REGEXP_OBJECT(Obj)->length
#define FTH_REGEXP_DATA(Obj)	FTH_REGEXP_OBJECT(Obj)->data
#define FTH_REGEXP_RE_BUF(Obj)	FTH_REGEXP_OBJECT(Obj)->re_buf
#define FTH_REGEXP_RESULTS(Obj)	FTH_REGEXP_OBJECT(Obj)->results
#define FTH_REGSTR_P(Obj)	(FTH_REGEXP_P(Obj) || FTH_STRING_P(Obj))

#define FTH_REGEXP_THROW(Msg)						\
	fth_throw(FTH_REGEXP_ERROR, "%s: %s", RUNNING_WORD(), Msg)

static void	ficl_make_regexp(ficlVm *vm);
static void	ficl_make_regexp_im(ficlVm *vm);
static void	ficl_re(ficlVm *vm);
static void	ficl_re_match(ficlVm *vm);
static void	ficl_re_search(ficlVm *vm);
static void	ficl_regexp_match(ficlVm *vm);
static void	ficl_regexp_p(ficlVm *vm);
static void	ficl_regexp_search(ficlVm *vm);
static FTH	reg_copy(FTH self);
static void	reg_free(FTH self);
static FTH	reg_inspect(FTH self);
static FTH	reg_length(FTH self);
static void	reg_mark(FTH self);
static FTH	reg_ref(FTH self, FTH idx);
static FTH	reg_to_array(FTH self);
static FTH	reg_to_string(FTH self);
static ficlInteger regexp_search(FTH regexp, char *str, int kind);

#define h_list_of_regexp_functions "\
*** REGEXP PRIMITIVES ***\n\
make-regexp         ( str -- reg )\n\
re-match            ( reg str start -- n )\n\
re-search           ( reg str start range -- n )\n\
re= alias for regexp-match\n\
regexp-match        ( reg str -- len )\n\
regexp-replace      ( reg str1 replace -- str2 )\n\
regexp-search       ( reg str :key start range -- pos )\n\
regexp= alias for regexp-match\n\
regexp?             ( obj -- f )\n\
*** variables:\n\
10 global read-only variables:\n\
*re0*               ( -- $0 )\n\
...\n\
*re9*               ( -- $9 )\n\
global read-only array *re* with last results\n\
*re-syntax-options* and a set of regexp \
constants (see *re-syntax-options* and regex(3))."

static FTH
reg_inspect(FTH self)
{
	if (FTH_REGEXP_LENGTH(self) > 0)
		return (fth_make_string_format("%s /%s/, subexp[%ld]: %S",
		    FTH_INSTANCE_NAME(self),
		    FTH_REGEXP_DATA(self),
		    fth_array_length(FTH_REGEXP_RESULTS(self)),
		    FTH_REGEXP_RESULTS(self)));
	return (fth_make_string_format("%s //", FTH_INSTANCE_NAME(self)));
}

static FTH
reg_to_string(FTH self)
{
	if (FTH_REGEXP_LENGTH(self) > 0)
		return (fth_make_string_format("/%s/", FTH_REGEXP_DATA(self)));
	return (fth_make_string("//"));
}

static FTH
reg_to_array(FTH self)
{
	return (FTH_REGEXP_RESULTS(self));
}

static FTH
reg_copy(FTH self)
{
	return (fth_make_regexp(FTH_REGEXP_DATA(self)));
}

static FTH
reg_ref(FTH self, FTH idx)
{
	ficlInteger index;

	index = FTH_INT_REF(idx);
	if (index >= fth_array_length(FTH_REGEXP_RESULTS(self)))
		return (FTH_FALSE);
	return (fth_array_ref(FTH_REGEXP_RESULTS(self), index));
}

static FTH
reg_length(FTH self)
{
	return (fth_make_int(fth_array_length(FTH_REGEXP_RESULTS(self))));
}

static void
reg_mark(FTH self)
{
	fth_gc_mark(FTH_REGEXP_RESULTS(self));
}

static void
reg_free(FTH self)
{
	regfree(&FTH_REGEXP_RE_BUF(self));
	FTH_FREE(FTH_REGEXP_DATA(self));
	FTH_FREE(FTH_REGEXP_OBJECT(self));
}

static void
ficl_regexp_p(ficlVm *vm)
{
#define h_regexp_p "( obj -- f )  test if OBJ is a regexp\n\
\"x\" make-regexp regexp? => #t\n\
/^s/            regexp? => #t\n\
nil             regexp? => #f\n\
Return #t if OBJ is a regexp object, otherwise #f."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, FTH_REGEXP_P(obj));
}

FTH
fth_make_regexp(const char *reg)
{
	FRegexp *r;
	int ret, flags;

	if (reg == NULL)
		reg = "";
	r = FTH_CALLOC(1, sizeof(FRegexp));
	flags = FIX_TO_INT32(fth_variable_ref("*re-syntax-options*"));
	ret = regcomp(&r->re_buf, reg, flags);
	if (ret != 0) {
		char errbuf[128];

		regerror(ret, &r->re_buf, errbuf, sizeof(errbuf));
		regfree(&r->re_buf);
		FTH_FREE(r);
		FTH_REGEXP_THROW(errbuf);
		/* NOTREACHED */
		return (FTH_FALSE);
	}
	r->data = FTH_STRDUP(reg);
	r->length = (ficlInteger)fth_strlen(reg);
	r->results = fth_make_array_with_init(r->re_buf.re_nsub + 1, FTH_FALSE);
	return (fth_make_instance(regexp_tag, r));
}

static void
ficl_make_regexp(ficlVm *vm)
{
#define h_make_regexp "( str -- reg )  create a regexp\n\
\"bar\"    make-regexp value re1\n\
\"(B|b)+\" make-regexp value re2\n\
re1 \"foobar\" regexp-search => 3\n\
re2 \"foobar\" regexp-search => 3\n\
Return new regexp object from STR which may contain regular expression.\n\
See regex(3) for more information."
	FTH_STACK_CHECK(vm, 1, 1);
	ficlStackPushFTH(vm->dataStack, fth_make_regexp(pop_cstring(vm)));
}

/*
 * Array of length 10, contains the whole match and all submatches or
 * #f if no submatches found.
 */
static FTH	regexp_results;

enum {
	FREG_SEARCH,
	FREG_MATCH
};

/*
 * FREG_SEARCH: match-index  or -1
 * FREG_MATCH:  match-length or -1
 */
static ficlInteger
regexp_search(FTH regexp, char *str, int kind)
{
	regmatch_t *pmatch;
	regoff_t beg, end;
	regex_t *re_buf;
	ficlInteger i, pos, slen;
	size_t nmatch;
	int ret;
	FTH fs;

	pos = -1;
	re_buf = &FTH_REGEXP_RE_BUF(regexp);
	nmatch = re_buf->re_nsub + 1;
	pmatch = FTH_MALLOC(nmatch * sizeof(regmatch_t));
	ret = regexec(re_buf, str, nmatch, pmatch, 0);
	if (ret != 0) {
		if (ret != REG_NOMATCH) {
			char errbuf[128];

			FTH_FREE(pmatch);
			regerror(ret, re_buf, errbuf, sizeof(errbuf));
			FTH_REGEXP_THROW(errbuf);
		}
		FTH_FREE(pmatch);
		return (pos);
	}
	beg = pmatch[0].rm_so;
	end = pmatch[0].rm_eo;
	switch (kind) {
	case FREG_SEARCH:
		pos = (ficlInteger)beg;
		break;
	case FREG_MATCH:
	default:
		pos = (ficlInteger)(end - beg);
		break;
	}
	/*
	 * Fill global regexp_results and results with subexpressions if any.
	 */
	for (i = 0; i < (int)nmatch; i++) {
		beg = pmatch[i].rm_so;
		end = pmatch[i].rm_eo;
		slen = (ficlInteger)(end - beg);
		if (slen < 0)
			break;
		fs = fth_make_string_len(str + beg, slen);
		fth_array_set(FTH_REGEXP_RESULTS(regexp), i, fs);
		if (i < REGEXP_REGS)
			fth_array_set(regexp_results, i, fs);
	}
	FTH_FREE(pmatch);
	return (pos);
}

/*
 * Return match-index or -1.
 */
int
fth_regexp_find(const char *reg, const char *str)
{
	int ret, found;
	char errbuf[128];
	regmatch_t match[REGEXP_REGS];
	regex_t re;

	found = -1;
	if (str == NULL || reg == NULL)
		return (found);
	ret = regcomp(&re, reg, REG_EXTENDED);
	if (ret != 0) {
		regerror(ret, &re, errbuf, sizeof(errbuf));
		regfree(&re);
		FTH_REGEXP_THROW(errbuf);
		/* NOTREACHED */
		return (found);
	}
	ret = regexec(&re, str, 1L, match, 0);
	if (ret != 0) {
		if (ret != REG_NOMATCH) {
			regerror(ret, &re, errbuf, sizeof(errbuf));
			regfree(&re);
			FTH_REGEXP_THROW(errbuf);
			/* NOTREACHED */
			return (found);
		}
	} else
		found = (int)match[0].rm_so;
	regfree(&re);
	return (found);
}

/*
 * Return match-length or -1.
 */
ficlInteger
fth_regexp_match(FTH regexp, FTH string)
{
	FTH_ASSERT_ARGS(FTH_REGSTR_P(regexp), regexp, FTH_ARG1, "a regexp");
	FTH_ASSERT_ARGS(FTH_STRING_P(string), string, FTH_ARG2, "a string");
	if (fth_string_length(string) == 0)
		return (-1);
	if (FTH_STRING_P(regexp))
		regexp = fth_make_regexp(fth_string_ref(regexp));
	fth_array_clear(FTH_REGEXP_RESULTS(regexp));
	fth_array_clear(regexp_results);
	return (regexp_search(regexp, fth_string_ref(string), FREG_MATCH));
}

/*
 * Return match-length or #f.
 */
static void
ficl_regexp_match(ficlVm *vm)
{
#define h_regexp_match "( reg str -- len|#f )  return match count\n\
/foo/   \"foobar\" regexp-match => 3\n\
/(bar)/ \"foobar\" regexp-match => 3\n\
/.*(bar)/ value reg\n\
reg \"foobar\" regexp-match => 6\n\
reg 0 apply => foobar\n\
reg 1 apply => bar\n\
reg 2 apply => #f\n\
Return count of matched characters or #f.  \
Possible matched group results or #f can be found in regexp object REG, \
in read-only variables *RE1* to *RE9* and in read-only array *RE*.\n\
See regex(3) for more information."
	FTH reg, str;
	ficlInteger result;

	FTH_STACK_CHECK(vm, 2, 1);
	str = fth_pop_ficl_cell(vm);
	reg = fth_pop_ficl_cell(vm);
	result = fth_regexp_match(reg, str);
	if (result == -1)
		ficlStackPushBoolean(vm->dataStack, false);
	else if (result == 0)
		ficlStackPushBoolean(vm->dataStack, true);
	else
		ficlStackPushInteger(vm->dataStack, result);
}

static char regexp_scratch[BUFSIZ];

/*
 * Return match-index or -1.
 * If range == -1, search entire string.
 */
ficlInteger
fth_regexp_search(FTH regexp, FTH string, ficlInteger start, ficlInteger range)
{
	char *s;
	ficlInteger len, pos;
	size_t size;
	bool allocated;

	FTH_ASSERT_ARGS(FTH_REGSTR_P(regexp), regexp, FTH_ARG1, "a regexp");
	FTH_ASSERT_ARGS(FTH_STRING_P(string), string, FTH_ARG2, "a string");
	len = fth_string_length(string);
	if (len == 0)
		return (-1);
	if (FTH_STRING_P(regexp))
		regexp = fth_make_regexp(fth_string_ref(regexp));
	if (start < 0)
		start += len;
	if (range == -1)
		range = len;
	if (start < 0 || start >= len)
		FTH_OUT_OF_BOUNDS(FTH_ARG2, start);
	range++;
	if (range < 0)
		range = -range;
	if (start + range >= len)
		range = len - start;
	if (range >= BUFSIZ) {
		size = (size_t)(range + 1);
		s = FTH_MALLOC(range + 1);
		allocated = true;
	} else {
		size = sizeof(regexp_scratch);
		s = regexp_scratch;
		allocated = false;
	}
	fth_array_clear(FTH_REGEXP_RESULTS(regexp));
	fth_array_clear(regexp_results);
	fth_strncpy(s, size, fth_string_ref(string) + start, (size_t)range);
	pos = regexp_search(regexp, s, FREG_SEARCH);
	if (allocated)
		FTH_FREE(s);
	if (pos >= 0)
		pos += start;
	return (pos);
}

/*
 * Return match-index or #f.
 */
static void
ficl_regexp_search(ficlVm *vm)
{
#define h_regexp_search "( reg str :key start 0 range -1 -- pos|f )  index\n\
/foo/ \"foobar\" :start 0 :range 6 regexp-search => #t (position 0)\n\
/(bar)/ value reg\n\
reg \"foobar\" :start 0 :range 2 regexp-search => #f\n\
reg \"foobar\" :start 3 :range 2 regexp-search => 3\n\
reg 0 apply => bar\n\
reg 1 apply => bar\n\
reg 2 apply => #f\n\
Return index of match or #f.  \
If index is zero, return #t to fool Forth' IF.  \
If keyword RANGE is -1 (default), the entire string will be searched.  \
Possible matched group results or #f can be found in regexp object REG, \
in read-only variables *RE1* to *RE9* and in read-only array *RE*.\n\
See regex(3) for more information."
	FTH reg, str;
	ficlInteger start, range, result;

	range = fth_get_optkey_int(FTH_KEYWORD_RANGE, -1L);
	start = fth_get_optkey_int(FTH_KEYWORD_START, 0L);
	FTH_STACK_CHECK(vm, 2, 1);
	str = fth_pop_ficl_cell(vm);
	reg = fth_pop_ficl_cell(vm);
	result = fth_regexp_search(reg, str, start, range);
	if (result == -1)
		ficlStackPushBoolean(vm->dataStack, false);
	else if (result == 0)
		ficlStackPushBoolean(vm->dataStack, true);
	else
		ficlStackPushInteger(vm->dataStack, result);
}

FTH
fth_regexp_replace(FTH regexp, FTH string, FTH replace)
{
#define h_regexp_replace "( reg str1 replace -- str2 )  string replace\n\
/(bar)/ \"foobar\" \"BAR\" regexp-replace => fooBAR\n\
/(foo)/ \"foo-bar\" \"***\\\\1***\" regexp-replace => ***foo***-bar\n\
Note the double quotes on back reference characters.\n\
Replace 1st occurrence of REG in STR1 with REPLACE if found.  \
References \\1 to \\9 in REPLACE will be replaced by \
corresponding subexpressions.  \
If no corresponding subexpression exist, raise REGEXP-ERROR exception.  \
Return a new string in any case, with or without replacement."
	ficlInteger pos, fs_len, rpl_len, reg_len, found_len;
	char *str, *rpl;
	FTH res_ary;

	FTH_ASSERT_ARGS(FTH_REGSTR_P(regexp), regexp, FTH_ARG1, "a regexp");
	FTH_ASSERT_ARGS(FTH_STRING_P(string), string, FTH_ARG2, "a string");
	FTH_ASSERT_ARGS(FTH_STRING_P(replace), replace, FTH_ARG3, "a string");
	fs_len = fth_string_length(string);
	if (fs_len == 0)
		return (fth_make_empty_string());
	if (FTH_STRING_P(regexp))
		regexp = fth_make_regexp(fth_string_ref(regexp));
	res_ary = FTH_REGEXP_RESULTS(regexp);
	pos = fth_regexp_search(regexp, string, 0L, fs_len);
	if (pos < 0)
		return (fth_string_copy(string));
	str = fth_string_ref(string);
	rpl = fth_string_ref(replace);
	rpl_len = fth_string_length(replace);
	reg_len = fth_array_length(res_ary);
	found_len = fth_string_length(fth_array_ref(res_ary, 0L));
	if (reg_len > 1 && strchr(rpl, '\\')) {
		int i;
		ficlInteger digit;
		FTH fs, ds;

		fs = fth_make_empty_string();
		for (i = 0; i < rpl_len; i++) {
			if (rpl[i] != '\\') {
				fth_string_sformat(fs, "%c", rpl[i]);
				continue;
			}
			if (++i < rpl_len && isdigit((int)rpl[i])) {
				digit = rpl[i] - 0x30;
				if (digit < reg_len) {
					ds = fth_array_ref(res_ary, digit);
					fth_string_push(fs, ds);
					continue;
				}
				FTH_REGEXP_THROW("wrong backward ref index");
			}
			FTH_REGEXP_THROW("backward ref without number");
		}
		rpl = fth_string_ref(fs);
		rpl_len = (ficlInteger)fth_strlen(rpl);
	}
	return (fth_make_string_format("%.*s%.*s%.*s",
	    pos, str,
	    rpl_len, rpl,
	    fs_len - pos, str + pos + found_len));
}

static void
ficl_re_match(ficlVm *vm)
{
#define h_re_match "( reg str start -- n )  return match length\n\
/a*/ \"aaaaab\" 0 re-match => 5\n\
/a*/ \"aaaaab\" 2 re-match => 3\n\
/a*/ \"aaaaab\" 5 re-match => 0\n\
/a*/ \"aaaaab\" 6 re-match => 0\n\
Return count of matched characters or -1 for no match."
	FTH reg, str;
	ficlInteger start, len, res;

	FTH_STACK_CHECK(vm, 3, 1);
	start = ficlStackPopInteger(vm->dataStack);
	str = fth_pop_ficl_cell(vm);
	reg = fth_pop_ficl_cell(vm);
	FTH_ASSERT_ARGS(FTH_REGSTR_P(reg), reg, FTH_ARG1, "a regexp");
	FTH_ASSERT_ARGS(FTH_STRING_P(str), str, FTH_ARG2, "a string");
	len = fth_string_length(str);
	if (len == 0) {
		ficlStackPushInteger(vm->dataStack, -1);
		return;
	}
	if (FTH_STRING_P(reg))
		reg = fth_make_regexp(fth_string_ref(reg));
	if (start < 0)
		start += len;
	if (start < 0)
		start = 0;
	else if (start >= len)
		start = len - 1;
	fth_array_clear(FTH_REGEXP_RESULTS(reg));
	fth_array_clear(regexp_results);
	res = regexp_search(reg, fth_string_ref(str) + start, FREG_MATCH);
	ficlStackPushInteger(vm->dataStack, res);
}

static void
ficl_re_search(ficlVm *vm)
{
#define h_re_search "( reg str start range -- n )  retun match index\n\
/a*/ \"aaaaab\" 0 1 re-search => 0\n\
/a*/ \"aaaaab\" 2 4 re-search => 2\n\
Return index of match or -1 for no match."
	char *s;
	FTH reg, str;
	ficlInteger start, range, pos, len;
	size_t size;
	bool allocated;

	FTH_STACK_CHECK(vm, 4, 1);
	range = ficlStackPopInteger(vm->dataStack);
	start = ficlStackPopInteger(vm->dataStack);
	str = fth_pop_ficl_cell(vm);
	reg = fth_pop_ficl_cell(vm);
	FTH_ASSERT_ARGS(FTH_REGSTR_P(reg), reg, FTH_ARG1, "a regexp");
	FTH_ASSERT_ARGS(FTH_STRING_P(str), str, FTH_ARG2, "a string");
	len = fth_string_length(str);
	if (len == 0) {
		ficlStackPushInteger(vm->dataStack, -1);
		return;
	}
	if (FTH_STRING_P(reg))
		reg = fth_make_regexp(fth_string_ref(reg));
	if (start < 0 || start >= len)
		FTH_OUT_OF_BOUNDS(FTH_ARG2, start);
	range++;
	if (range < 0)
		range = -range;
	if (start + range >= len)
		range = len - start;
	if (range >= BUFSIZ) {
		size = (size_t)(range + 1);
		s = FTH_MALLOC(size);
		allocated = true;
	} else {
		size = sizeof(regexp_scratch);
		s = regexp_scratch;
		allocated = false;
	}
	fth_array_clear(FTH_REGEXP_RESULTS(reg));
	fth_array_clear(regexp_results);
	fth_strncpy(s, size, fth_string_ref(str) + start, (size_t)range);
	pos = regexp_search(reg, s, FREG_SEARCH);
	if (allocated)
		FTH_FREE(s);
	if (pos >= 0)
		pos += start;
	ficlStackPushInteger(vm->dataStack, pos);
}

static void
ficl_make_regexp_im(ficlVm *vm)
{
#define h_make_regexp_im "( space<ccc>/ -- reg )  regexp (parse word)\n\
re/ ^foo$/ => /^foo$/\n\
Parse regexp CCC delimited by '/' at compile time \
and at interpret time return parsed regexp.  \
Note the space after the initial RE/.  \
It exist to satisfy fontifying in Emacs forth-mode."
	char *buf;
	FTH rs;

	/* ficlVmParseString(vm, '/') doesn't work. */
	buf = parse_input_buffer(vm, "/");	/* must be freed */
	rs = fth_make_regexp(buf);
	FTH_FREE(buf);
	if (vm->state == FICL_VM_STATE_COMPILE) {
		ficlDictionary *dict;

		dict = ficlVmGetDictionary(vm);
		ficlDictionaryAppendUnsigned(dict,
		    (ficlUnsigned)ficlInstructionLiteralParen);
		ficlDictionaryAppendFTH(dict, rs);
	} else
		ficlStackPushFTH(vm->dataStack, rs);
}

#define h_syntax_options "\
REG_EXTENDED REG_ICASE or  to *re-syntax-options*\n\
The following constants exist for oring to *re-syntax-options*:\n\
REG_EXTENDED (default)\n\
REG_ICASE\n\
REG_NOSUB\n\
REG_NEWLINE\n\
See regex(3) for more information."

/*
 * Global read-only variables *RE* and *RE0* ... *RE9* are actually
 * functions.
 */
#define FTH_RE_VAR(numb)						\
static void								\
ficl_re_ ## numb (ficlVm *vm)						\
{									\
	fth_push_ficl_cell(vm,						\
	    fth_array_ref(regexp_results, (ficlInteger)numb));		\
}

static void
ficl_re(ficlVm *vm)
{
	ficlStackPushFTH(vm->dataStack, regexp_results);
}

/* Create ten functions: static void ficl_re_0(ficl *vm); */
FTH_RE_VAR(0)
FTH_RE_VAR(1)
FTH_RE_VAR(2)
FTH_RE_VAR(3)
FTH_RE_VAR(4)
FTH_RE_VAR(5)
FTH_RE_VAR(6)
FTH_RE_VAR(7)
FTH_RE_VAR(8)
FTH_RE_VAR(9)

void
init_regexp_type(void)
{
	regexp_tag = make_object_type(FTH_STR_REGEXP, FTH_REGEXP_T);
	fth_set_object_inspect(regexp_tag, reg_inspect);
	fth_set_object_to_string(regexp_tag, reg_to_string);
	fth_set_object_to_array(regexp_tag, reg_to_array);
	fth_set_object_copy(regexp_tag, reg_copy);
	fth_set_object_value_ref(regexp_tag, reg_ref);
	fth_set_object_length(regexp_tag, reg_length);
	fth_set_object_mark(regexp_tag, reg_mark);
	fth_set_object_free(regexp_tag, reg_free);
}

void
init_regexp(void)
{
	fth_set_object_apply(regexp_tag, (void *)reg_ref, 1, 0, 0);
	regexp_results = fth_make_array_with_init(REGEXP_REGS, FTH_FALSE);
	fth_gc_permanent(regexp_results);
	/* regexp */
	FTH_PRI1("regexp?", ficl_regexp_p, h_regexp_p);
	FTH_PRI1("make-regexp", ficl_make_regexp, h_make_regexp);
	FTH_PRI1("regexp-match", ficl_regexp_match, h_regexp_match);
	FTH_PRI1("regexp=", ficl_regexp_match, h_regexp_match);
	FTH_PRI1("re=", ficl_regexp_match, h_regexp_match);
	FTH_PRI1("regexp-search", ficl_regexp_search, h_regexp_search);
	FTH_PROC("regexp-replace", fth_regexp_replace,
	    3, 0, 0, h_regexp_replace);
	FTH_PRI1("re-match", ficl_re_match, h_re_match);
	FTH_PRI1("re-search", ficl_re_search, h_re_search);
	FTH_PRIM_IM("re/", ficl_make_regexp_im, h_make_regexp_im);
	FTH_PRI1("*re*", ficl_re, NULL);
	FTH_PRI1("*re0*", ficl_re_0, NULL);
	FTH_PRI1("*re1*", ficl_re_1, NULL);
	FTH_PRI1("*re2*", ficl_re_2, NULL);
	FTH_PRI1("*re3*", ficl_re_3, NULL);
	FTH_PRI1("*re4*", ficl_re_4, NULL);
	FTH_PRI1("*re5*", ficl_re_5, NULL);
	FTH_PRI1("*re6*", ficl_re_6, NULL);
	FTH_PRI1("*re7*", ficl_re_7, NULL);
	FTH_PRI1("*re8*", ficl_re_8, NULL);
	FTH_PRI1("*re9*", ficl_re_9, NULL);
	fth_define_variable("*re-syntax-options*",
	    INT_TO_FIX(REG_EXTENDED), h_syntax_options);
	FTH_SET_CONSTANT(REG_EXTENDED);
	FTH_SET_CONSTANT(REG_ICASE);
	FTH_SET_CONSTANT(REG_NOSUB);
	FTH_SET_CONSTANT(REG_NEWLINE);
	FTH_ADD_FEATURE_AND_INFO(FTH_STR_REGEXP, h_list_of_regexp_functions);
}

#else				/* !HAVE_POSIX_REGEX */

FTH
fth_make_regexp(const char *str)
{
	return (fth_make_string(str));
}

/* ARGSUSED */
int
fth_regexp_find(const char *reg, const char *str)
{
	return (-1);
}

/* ARGSUSED */
ficlInteger
fth_regexp_match(FTH regexp, FTH string)
{
	return (-1);
}

/* ARGSUSED */
ficlInteger
fth_regexp_search(FTH regexp, FTH string, ficlInteger beg, ficlInteger len)
{
	return (-1);
}

/* ARGSUSED */
FTH
fth_regexp_replace(FTH regexp, FTH string, FTH replace)
{
	return (string);
}

static void
ficl_make_regexp_im(ficlVm *vm)
{
	char *buf;
	FTH rs;

	buf = parse_input_buffer(vm, "/");
	rs = fth_make_regexp(buf);
	FTH_FREE(buf);
	if (vm->state == FICL_VM_STATE_COMPILE) {
		ficlDictionary *dict;

		dict = ficlVmGetDictionary(vm);
		ficlDictionaryAppendUnsigned(dict,
		    (ficlUnsigned)ficlInstructionLiteralParen);
		ficlDictionaryAppendFTH(dict, rs);
	} else
		ficlStackPushFTH(vm->dataStack, rs);
}

void
init_regexp_type(void)
{
}

void
init_regexp(void)
{
	FTH_PRIM_IM("re/", ficl_make_regexp_im, NULL);
}


#endif				/* HAVE_POSIX_REGEX */

/*
 * regexp.c ends here
 */
