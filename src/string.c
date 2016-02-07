/*-
 * Copyright (c) 2005-2016 Michael Scholz <mi-scholz@users.sourceforge.net>
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
 * @(#)string.c	1.164 2/7/16
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "fth.h"
#include "utils.h"

/* === STRING === */

static FTH	string_tag;

typedef struct {
	ficlInteger	length;	/* actual string length */
	ficlInteger	buf_length;	/* buffer length (bigger than actual
					 * string length) */
	ficlInteger	top;	/* begin of actual string in buffer */
	char           *data;	/* actual string */
	char           *buf;	/* entire string buffer */
} FString;

#define MAKE_STRING_MEMBER(Type, Member) MAKE_MEMBER(FString, str, Type, Member)

/*
 * Build words for scrutinizing strings:
 *
 * init_str_length     => str->length
 * init_str_buf_length => str->buf_length
 * init_str_top        => str->top
 */
MAKE_STRING_MEMBER(Integer, length)
MAKE_STRING_MEMBER(Integer, buf_length)
MAKE_STRING_MEMBER(Integer, top)

#define FTH_STRING_OBJECT(Obj)	FTH_INSTANCE_REF_GEN(Obj, FString)
#define FTH_STRING_LENGTH(Obj)	FTH_STRING_OBJECT(Obj)->length
#define FTH_STRING_BUF_LENGTH(Obj)					\
	FTH_STRING_OBJECT(Obj)->buf_length
#define FTH_STRING_TOP(Obj)	FTH_STRING_OBJECT(Obj)->top
#define FTH_STRING_DATA(Obj)	FTH_STRING_OBJECT(Obj)->data
#define FTH_STRING_BUF(Obj)	FTH_STRING_OBJECT(Obj)->buf
#define FTH_STRING_REF(Obj)						\
	(FTH_STRING_P(Obj) ? FTH_STRING_DATA(Obj) : NULL)
#define FTH_STRREG_P(Obj)	(FTH_STRING_P(Obj) || FTH_REGEXP_P(Obj))
#define FTH_TO_CCHAR(Obj)	((char)(FTH_TO_CHAR(Obj)))

static void	ficl_char_p(ficlVm *vm);
static void	ficl_cr_string(ficlVm *vm);
static void	ficl_die(ficlVm *vm);
static void	ficl_error(ficlVm *vm);
static void	ficl_error_object(ficlVm *vm);
static void	ficl_forth_string_to_string(ficlVm *vm);
static void	ficl_fth_die(FTH fmt, FTH args);
static void	ficl_fth_error(FTH fmt, FTH args);
static FTH	ficl_fth_format(FTH fmt, FTH args);
static void	ficl_fth_print(FTH fmt, FTH args);
static void	ficl_fth_warning(FTH fmt, FTH args);
static void	ficl_make_empty_string(ficlVm *vm);
static void	ficl_make_string(ficlVm *vm);
static void	ficl_make_string_im(ficlVm *vm);
static void	ficl_print_debug(ficlVm *vm);
static void	ficl_print_object(ficlVm *vm);
static void	ficl_print_stderr(ficlVm *vm);
static void	ficl_print_stdout(ficlVm *vm);
static void	ficl_space_string(ficlVm *vm);
static void	ficl_spaces_string(ficlVm *vm);
static void	ficl_string_capitalize(ficlVm *vm);
static void	ficl_string_capitalize_bang(ficlVm *vm);
static void	ficl_string_chomp(ficlVm *vm);
static void	ficl_string_chomp_bang(ficlVm *vm);
static void	ficl_string_delete(ficlVm *vm);
static void	ficl_string_downcase(ficlVm *vm);
static void	ficl_string_downcase_bang(ficlVm *vm);
static void	ficl_string_cmp(ficlVm *vm);
static void	ficl_string_equal_p(ficlVm *vm);
static void	ficl_string_eval(ficlVm *vm);
static void	ficl_string_eval_with_status(ficlVm *vm);
static void	ficl_string_greater_p(ficlVm *vm);
static void	ficl_string_immutable_paren(ficlVm *vm);
static void	ficl_string_insert(ficlVm *vm);
static void	ficl_string_length(ficlVm *vm);
static void	ficl_string_less_p(ficlVm *vm);
static void	ficl_string_member_p(ficlVm *vm);
static void	ficl_string_not_equal_p(ficlVm *vm);
static void	ficl_string_p(ficlVm *vm);
static void	ficl_string_ref(ficlVm *vm);
static void	ficl_string_replace(ficlVm *vm);
static void	ficl_string_replace_bang(ficlVm *vm);
static void	ficl_string_reverse(ficlVm *vm);
static void	ficl_string_reverse_bang(ficlVm *vm);
static void	ficl_string_set(ficlVm *vm);
static void	ficl_string_substring(ficlVm *vm);
static void	ficl_string_to_forth_string(ficlVm *vm);
static void	ficl_string_upcase(ficlVm *vm);
static void	ficl_string_upcase_bang(ficlVm *vm);
static void	ficl_values_to_string(ficlVm *vm);
static void	ficl_warning(ficlVm *vm);
static FTH	make_string_instance(FString *s);
static FString *make_string_len(ficlInteger len);
static FTH	str_dump(FTH self);
static FTH	str_equal_p(FTH self, FTH obj);
static void	str_free(FTH self);
static FTH	str_inspect(FTH self);
static FTH	str_length(FTH self);
static FTH	str_ref(FTH self, FTH fidx);
static FTH	str_set(FTH self, FTH fidx, FTH value);
static FTH	str_to_array(FTH self);
static FTH	str_to_string(FTH self);

#define h_list_of_string_functions "\
*** STRING PRIMITIVES ***\n\
$+ alias for string-append\n\
$>string            ( addr len -- str )\n\
$cr                 ( -- cr-str )\n\
$space              ( -- space-str )\n\
$spaces             ( len -- spaces-str )\n\
.$ alias for .string\n\
.debug              ( obj -- )\n\
.error              ( obj -- )\n\
.g alias for .string\n\
.stderr             ( obj -- )\n\
.stdout             ( obj -- )\n\
.string             ( obj -- )\n\
<< alias for string-push\n\
>string alias for string-concat\n\
\"\"           	    ( -- empty-string )\n\
char?               ( obj -- f )\n\
format alias for string-format\n\
fth-die             ( fmt :optional args -- )\n\
fth-error           ( fmt :optional args -- )\n\
fth-print           ( fmt :optional args -- )\n\
fth-warning         ( fmt :optional args -- )\n\
make-string         ( len :key initial-element -- str )\n\
string->array       ( str -- ary )\n\
string-append       ( str1 str2 -- str3 )\n\
string-capitalize   ( str1 -- str2 )\n\
string-capitalize!  ( str -- str' )\n\
string-chomp        ( str1 -- str2 )\n\
string-chomp!       ( str -- str' )\n\
string-concat       ( vals len -- str )\n\
string-copy         ( str1 -- str2 )\n\
string-delete!      ( str idx -- val )\n\
string-downcase     ( str1 -- str2 )\n\
string-downcase!    ( str -- str' )\n\
string-eval         ( str -- ?? )\n\
string-eval-with-status ( str -- ?? status )\n\
string-fill         ( str char -- str' )\n\
string-find         ( str1 key -- str2 )\n\
string-format       ( fmt args -- str )\n\
string-index        ( str key -- idx|-1 )\n\
string-insert!      ( str idx val -- str' )\n\
string-length       ( str -- len )\n\
string-member?      ( str key -- f )\n\
string-pop          ( str -- char )\n\
string-push         ( str str-or-else -- str' )\n\
string-ref          ( str idx -- val )\n\
string-replace      ( str1 c1 c2 -- str2 )\n\
string-replace!     ( str c1 c2 -- str' )\n\
string-reverse      ( str1 -- str2 )\n\
string-reverse!     ( str -- str' )\n\
string-set!         ( str idx val -- )\n\
string-shift        ( str -- char )\n\
string-split        ( str sep -- ary )\n\
string-substring    ( str start end -- substr )\n\
string-unshift      ( str val -- str' )\n\
string-upcase       ( str1 -- str2 )\n\
string-upcase!      ( str -- str' )\n\
string-cmp          ( str1 str2 -- n )\n\
string<             ( str1 str2 -- f )\n\
string<>            ( str1 str2 -- f )\n\
string=             ( str1 str2 -- f )\n\
string>             ( str1 str2 -- f )\n\
string>$            ( str -- addr len )\n\
string?             ( obj -- f )\n\
*** Eval exit constants:\n\
BREAK\n\
ERROR_EXIT\n\
INNER_EXIT\n\
OUT_OF_TEXT\n\
RESTART\n\
USER_EXIT"

static FTH
str_inspect(FTH self)
{
	return (fth_make_string_format("%s[%ld]: \"%s\"",
	    FTH_INSTANCE_NAME(self),
	    FTH_STRING_LENGTH(self),
	    FTH_STRING_DATA(self)));
}

static FTH
str_to_string(FTH self)
{
	return (fth_make_string(FTH_STRING_DATA(self)));
}

static FTH
str_dump(FTH self)
{
	return (fth_make_string_format("\"%s\"", FTH_STRING_DATA(self)));
}

static FTH
str_to_array(FTH self)
{
	ficlInteger i;
	FTH array;

	array = fth_make_array_len(FTH_STRING_LENGTH(self));
	for (i = 0; i < FTH_STRING_LENGTH(self); i++)
		fth_array_fast_set(array, i,
		    CHAR_TO_FTH(FTH_STRING_DATA(self)[i]));
	return (array);
}

static FTH
str_ref(FTH self, FTH fidx)
{
	ficlInteger idx;

	idx = FTH_INT_REF(fidx);
	if (idx < 0 || idx >= FTH_STRING_LENGTH(self))
		FTH_OUT_OF_BOUNDS(FTH_ARG2, idx);
	return (CHAR_TO_FTH(FTH_STRING_DATA(self)[idx]));
}

static FTH
str_set(FTH self, FTH fidx, FTH value)
{
	ficlInteger idx;

	idx = FTH_INT_REF(fidx);
	if (idx < 0 || idx >= FTH_STRING_LENGTH(self))
		FTH_OUT_OF_BOUNDS(FTH_ARG2, idx);
	FTH_ASSERT_ARGS(FTH_CHAR_P(value), value, FTH_ARG3, "a char");
	FTH_STRING_DATA(self)[idx] = FTH_TO_CCHAR(value);
	FTH_INSTANCE_CHANGED(self);
	return (value);
}

static FTH
str_equal_p(FTH self, FTH obj)
{
	if (FTH_STRING_LENGTH(self) == FTH_STRING_LENGTH(obj)) {
		if (FTH_STRING_LENGTH(self) == 0)
			return (FTH_TRUE);
		else {
			char *s, *o;

			s = FTH_STRING_DATA(self);
			o = FTH_STRING_DATA(obj);
			return (BOOL_TO_FTH(strcmp(s, o) == 0));
		}
	}
	return (FTH_FALSE);
}

static FTH
str_length(FTH self)
{
	return (fth_make_int(FTH_STRING_LENGTH(self)));
}

static void
str_free(FTH self)
{
	FTH_FREE(FTH_STRING_BUF(self));
	FTH_FREE(FTH_STRING_OBJECT(self));
}

/*
 * Return C string from Fth OBJ or NULL.
 */
char *
fth_string_ref(FTH obj)
{
	return (FTH_STRING_P(obj) ? FTH_STRING_DATA(obj) : NULL);
}

/*
 * Return length of a Fth string object or -1 if not a string.
 *
 * FTH fs = fth_make_string("hello");
 * fth_string_length(fs);			=> 5
 * fth_string_length((FTH)5);			=> -1 (means false)
 */
ficlInteger
fth_string_length(FTH obj)
{
	return (FTH_STRING_P(obj) ? FTH_STRING_LENGTH(obj) : -1);
}

static void
ficl_string_length(ficlVm *vm)
{
#define h_string_length "( str -- len )  return STR length\n\
\"hello\" string-length => 5\n\
5       string-length => -1\n\
If STR is a string object, return its length, otherwise -1."
	FTH fs;

	FTH_STACK_CHECK(vm, 1, 1);
	fs = fth_pop_ficl_cell(vm);
	ficlStackPushInteger(vm->dataStack, fth_string_length(fs));
}

static void
ficl_string_p(ficlVm *vm)
{
#define h_string_p "( obj -- f )  test if OBJ is a string\n\
\"hello\" string? => #t\n\
nil     string? => #f\n\
Return #t if OBJ is a string object, otherwise #f."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, FTH_STRING_P(obj));
}

static void
ficl_char_p(ficlVm *vm)
{
#define h_char_p "( obj -- f )  test if OBJ is a character\n\
<char> A char? => #t\n\
65       char? => #t\n\
10       char? => #f\n\
Return #t if OBJ is a character, otherwise #f."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, FTH_CHAR_P(obj));
}

static FString *
make_string_len(ficlInteger len)
{
	FString *s;
	ficlInteger buf_len, top_len;

	if (len < 0)
		FTH_OUT_OF_BOUNDS_ERROR(FTH_ARG1, len, "negative");
	top_len = NEW_SEQ_LENGTH(len + 1) / 3;
	buf_len = NEW_SEQ_LENGTH(len + 1 + top_len);
	if (len > MAX_SEQ_LENGTH)
		FTH_OUT_OF_BOUNDS_ERROR(FTH_ARG1, len, "too long");
	s = FTH_MALLOC(sizeof(FString));
	s->length = len;
	s->buf_length = buf_len;
	s->top = top_len;
	s->buf = FTH_CALLOC(s->buf_length, sizeof(char));
	s->data = s->buf + s->top;
	return (s);
}

static FTH
make_string_instance(FString *s)
{
	if (s != NULL)
		return (fth_make_instance(string_tag, s));
	FTH_SYSTEM_ERROR_ARG_THROW(make_string, FTH_STR_STRING);
	return (FTH_FALSE);
}

/*
 * Return a new Fth string object constructed from C string STR.
 * If C string is "" or NULL, return Fth string "" in contrast to
 * fth_make_string_or_false().
 *
 * FTH fs = fth_make_string("hello");		=> "hello"
 * fth_string_length(fs);			=> 5
 * FTH fs = fth_make_string("");		=> ""
 * fth_string_length(fs);			=> 0
 */
FTH
fth_make_string(const char *str)
{
	FString *s;
	size_t len;

	if (str == NULL || *str == '\0')
		str = "";
	len = strlen(str);
	s = make_string_len((ficlInteger)len);
	memmove(s->data, str, len);
	return (make_string_instance(s));
}

/*
 * If C string is "", return #f, otherwise as fth_make_string().
 *
 * FTH fs = fth_make_string_or_false("hello");	=> "hello"
 * fth_string_length(fs);			=> 5
 * FTH fs = fth_make_string_or_false("");	=> #f
 * fth_string_length(fs);			=> -1 (means false)
 */
FTH
fth_make_string_or_false(const char *str)
{
	FString *s;
	size_t len;

	if (str == NULL || *str == '\0')
		return (FTH_FALSE);
	len = strlen(str);
	s = make_string_len((ficlInteger)len);
	memmove(s->data, str, len);
	return (make_string_instance(s));
}

/*
 * Return a new Fth string object constructed from C string STR
 * with at most LEN characters.  If the C string STR is shorter
 * than LEN, return a Fth string object of STR length only.
 *
 * FTH fs = fth_make_string_len("     ", 0);	=> ""
 * FTH fs = fth_make_string_len("     ", 3);	=> "   "
 * FTH fs = fth_make_string_len("xxxxx", 3);	=> "xxx"
 * FTH fs = fth_make_string_len("xxx", 5);	=> "xxx"
 */
FTH
fth_make_string_len(const char *str, ficlInteger len)
{
	FString *s;
	size_t slen, blen;

	if (str == NULL || *str == '\0')
		str = "";
	slen = strlen(str);
	blen = FICL_MIN((size_t)len, slen);
	s = make_string_len(blen);
	memmove(s->data, str, blen);
	s->data[blen] = '\0';
	return (make_string_instance(s));
}

/*
 * Return a Fth string object of length 0.
 *
 * FTH fs = fth_make_empty_string();		=> ""
 * fth_string_length(fs);			=> 0
 */
FTH
fth_make_empty_string(void)
{
	FString *s;

	s = make_string_len(0);
	s->data[0] = '\0';
	return (make_string_instance(s));
}

/*-
 * Return a Fth string object according to the (extended) printf(3)
 * fmt args.  The extensions are:
 *
 *	%I - fth_to_c_inspect	print inspect string of Fth object
 *	%S - fth_to_c_string	print string representation of object
 *      %M - fth_to_c_string_2  as fth_to_c_string but encloses
 *				strings in double quotes
 *	%D - fth_to_c_dump	print dump string of Fth object
 */
FTH
fth_make_string_format(const char *fmt,...)
{
	char *str;
	va_list ap;
	FTH fs;

	va_start(ap, fmt);
	fth_vasprintf(&str, fmt, ap);
	va_end(ap);
	fs = fth_make_string(str);
	FTH_FREE(str);
	return (fs);
}

/*-
 * These functions add C strings to an already existing Fth string
 * object, the extended printf(3) fmt args are available for the
 * last two of them.  See fth_make_string_format above.
 *
 * fth_string_scat    (FTH string, const char *str)
 * fth_string_sncat   (FTH string, const char *str, ficlInteger len)
 * fth_string_sformat (FTH string, const char *fmt, ...)
 * fth_string_vsformat(FTH string, const char *fmt, va_list ap)
 *
 * FTH fs = fth_make_empty_string();		=> ""
 * fth_string_scat(fs, "hello");		=> "hello"
 * fth_string_sncat(fs, ",  ", 2);		=> "hello, "
 * fth_string_sformat(fs, "%s!\n", "world");	=> "hello, world!\n"
 */
FTH
fth_string_scat(FTH fs, const char *str)
{
	return (fth_string_push(fs, fth_make_string(str)));
}

FTH
fth_string_sncat(FTH fs, const char *str, ficlInteger len)
{
	return (fth_string_push(fs, fth_make_string_len(str, len)));
}

/*
 * Add extended printf(3) fmt args to an already existing Fth string
 * object.  See fth_make_string_format above.
 *
 * FTH fs = fth_make_string("we want to ");	=> "we want to "
 * fth_string_sformat(fs, "print %d times %f\n", 10, 3.14);
 *	=> "we want to print 10 times 3.140000\n"
 */
FTH
fth_string_sformat(FTH fs, const char *fmt,...)
{
	FTH s;
	va_list ap;

	va_start(ap, fmt);
	s = fth_string_vsformat(fs, fmt, ap);
	va_end(ap);
	return (s);
}

/*
 * The same as fth_string_sformat except for va_list args.
 */
FTH
fth_string_vsformat(FTH fs, const char *fmt, va_list ap)
{
	char *str;
	FTH s;

	str = fth_vformat(fmt, ap);
	s = fth_make_string(str);
	FTH_FREE(str);
	return (fth_string_push(fs, s));
}

static void
ficl_make_string(ficlVm *vm)
{
#define h_make_string "( len :key initial-element ' ' -- str )  string\n\
0                           make-string => \"\"\n\
3                           make-string => \"   \"\n\
3 :initial-element <char> x make-string => \"xxx\"\n\
Return a new string of length LEN filled with INITIAL-ELEMENT characters, \
default space.  \
Raise OUT-OF-RANGE exception if LEN < 0."
	FTH size;
	ficlInteger len;
	int init;
	FString *s;

	init = fth_get_optkey_fix(FTH_KEYWORD_INIT, ' ');
	FTH_STACK_CHECK(vm, 1, 1);
	size = fth_pop_ficl_cell(vm);
	FTH_ASSERT_ARGS(FTH_INTEGER_P(size), size, FTH_ARG1, "an integer");
	len = FTH_INT_REF(size);
	s = make_string_len(len);
	memset(s->data, init, (size_t)len);
	ficlStackPushFTH(vm->dataStack, make_string_instance(s));
}

static void
ficl_values_to_string(ficlVm *vm)
{
#define h_values_to_string "( len-vals len -- str )  return string\n\
0 1 2 \" foo \" \"b\" \"a\" \"r\"  7 >string => \"012 foo bar\"\n\
Return new string with LEN objects from stack converted to their \
string representation.  \
Raise OUT-OF-RANGE exception if LEN < 0."
	ficlInteger i, len;
	FTH fs, obj;

	FTH_STACK_CHECK(vm, 1, 0);
	len = ficlStackPopInteger(vm->dataStack);
	if (len < 0)
		FTH_OUT_OF_BOUNDS_ERROR(FTH_ARG1, len, "negative");
	if (len > MAX_SEQ_LENGTH)
		FTH_OUT_OF_BOUNDS_ERROR(FTH_ARG1, len, "too long");
	FTH_STACK_CHECK(vm, len, 1);
	fs = fth_make_empty_string();
	for (i = 0; i < len; i++) {
		obj = fth_pop_ficl_cell(vm);
		fth_string_unshift(fs, fth_object_to_string(obj));
	}
	ficlStackPushFTH(vm->dataStack, fs);
}

static void
ficl_make_empty_string(ficlVm *vm)
{
#define h_empty_string "( -- str )  return empty string\n\
\"\" value s1\n\
s1 \"aa\" string-push => \"aa\"\n\
s1 \"bb\" string-push => \"aabb\"\n\
s1 \"cc\" string-push => \"aabbcc\"\n\
s1 => \"aabbcc\"\n\
Return empty string object."
	FTH_STACK_CHECK(vm, 0, 1);
	push_cstring(vm, "");
}

static void
ficl_space_string(ficlVm *vm)
{
#define h_space_string "( -- str )  return space string\n\
$space => \" \"\n\
Return string object of one space."
	FTH_STACK_CHECK(vm, 0, 1);
	push_cstring(vm, " ");
}

static void
ficl_spaces_string(ficlVm *vm)
{
#define h_spaces_string "( len -- str )  return spaces string\n\
3 $spaces => \"   \"\n\
0 $spaces => \"\"\n\
Return string object of LEN spaces.  \
Raise OUT-OF-RANGE exception if LEN < 0."
	ficlInteger len;
	FTH fs;

	FTH_STACK_CHECK(vm, 1, 1);
	len = ficlStackPopInteger(vm->dataStack);
	if (len < 0)
		FTH_OUT_OF_BOUNDS_ERROR(FTH_ARG1, len, "negative");
	if (len > MAX_SEQ_LENGTH)
		FTH_OUT_OF_BOUNDS_ERROR(FTH_ARG1, len, "too long");
	if (len == 0) {
		ficlStackPushFTH(vm->dataStack, fth_make_empty_string());
		return;
	}
	fs = fth_make_string_format("%*c", (int)len, ' ');
	ficlStackPushFTH(vm->dataStack, fs);
}

static void
ficl_cr_string(ficlVm *vm)
{
#define h_cr_string "( -- str )  return cr string\n\
$cr => \"\\n\"\n\
Return carriage return string object."
	FTH_STACK_CHECK(vm, 0, 1);
	push_cstring(vm, "\n");
}

static FTH
ficl_fth_format(FTH fmt, FTH args)
{
#define h_fth_format "( fmt :optional args -- str )  return string\n\
\"hello\" fth-format => \"hello\"\n\
\"hello %s %d times\\n\" #( \"pumpkin\" 10 ) fth-format\n\
  => \"hello pumpkin 10 times\\n\"\n\
Return string object from sprintf(3) FMT string and ARGS \
array containing corresponding arguments; ARGS is optional.\n\
See string-format for FMT description."
	if (FTH_UNDEF_P(args))
		return (fmt);
	return (fth_string_format(fmt, args));
}

static void
ficl_fth_print(FTH fmt, FTH args)
{
#define h_fth_print "( fmt :optional args -- )  print string\n\
\"hello\" fth-print => prints hello\n\
\"hello %s %d times\" #( \"pumpkin\" 10 ) fth-print\n\
  => hello pumpkin 10 times\n\
Print FMT string with corresponding ARGS array to current stdout; \
ARGS is optional."
	fth_printf("%S", ficl_fth_format(fmt, args));
}

static void
ficl_fth_warning(FTH fmt, FTH args)
{
#define h_fth_warning "( fmt :optional args -- )  print string\n\
\"%d does not fit\" #( 3 ) fth-warning => #<warning: 3 does not fit>\n\
Print FMT string with corresponding ARGS array \
wrapped in #<warning: ...> to current stderr; \
ARGS is optional."
	fth_warning("%S", ficl_fth_format(fmt, args));
}

static void
ficl_fth_error(FTH fmt, FTH args)
{
#define h_fth_error "( fmt :optional args -- )  print formatted string\n\
\"%d does not fit\" #( 3 ) fth-error => #<error: 3 does not fit>\n\
Print FMT string with corresponding ARGS array wrapped in #<error: ...> \
to current stderr and throw exception; ARGS is optional."
	fth_errorf("#<error: %S>\n", ficl_fth_format(fmt, args));
	ficlVmThrow(FTH_FICL_VM(), FICL_VM_STATUS_ERROR_EXIT);
}

static void
ficl_fth_die(FTH fmt, FTH args)
{
#define h_fth_die "( fmt :optional args -- )  print string and exit\n\
\"%d does not fit\" #( 3 ) fth-die => #<die: 3 does not fit>\n\
Print FMT string with corresponding ARGS array wrapped in #<die: ...> \
to current stderr and exit interpreter with return code 1; ARGS is optional."
	fth_errorf("#<die: %S>\n", ficl_fth_format(fmt, args));
	fth_exit(EXIT_FAILURE);
}

static void
ficl_warning(ficlVm *vm)
{
#define h_warning "( str -- )  print warning string\n\
\"trouble\" warning => #<warning: trouble>\n\
Print STR wrapped in #<warning: ...> to current stderr."
	FTH_STACK_CHECK(vm, 1, 0);
	ficl_fth_warning(fth_pop_ficl_cell(vm), FTH_UNDEF);
}

static void
ficl_error(ficlVm *vm)
{
#define h_error "( str -- )  print error string\n\
\"trouble\" error => #<error: trouble>\n\
Print STR wrapped in #<error: ...> to current stderr and throw exception."
	FTH_STACK_CHECK(vm, 1, 0);
	ficl_fth_error(fth_pop_ficl_cell(vm), FTH_UNDEF);
}

static void
ficl_die(ficlVm *vm)
{
#define h_die "( str -- )  print string and exit\n\
\"trouble\" die => #<die: trouble>\n\
Print STR wrapped in #<die: ...> to current stderr \
and exit interpreter with return code 1."
	FTH_STACK_CHECK(vm, 1, 0);
	ficl_fth_die(fth_pop_ficl_cell(vm), FTH_UNDEF);
}

static void
ficl_print_object(ficlVm *vm)
{
#define h_print_object "( obj -- )  print OBJ\n\
#{ 'foo 10 } .string => #{ 'foo => 10 }\n\
#{ 'foo 10 } .$ => #{ 'foo => 10 }\n\
#{ 'foo 10 } .g => #{ 'foo => 10 }\n\
Print string representation of OBJ to current output."
	FTH_STACK_CHECK(vm, 1, 0);
	fth_print(fth_to_c_string(fth_pop_ficl_cell(vm)));
}

static void
ficl_error_object(ficlVm *vm)
{
#define h_error_object "( obj -- )  print OBJ\n\
#{ 'foo 10 } .error => #{ 'foo => 10 }\n\
Print string representation of OBJ to current error output."
	FTH_STACK_CHECK(vm, 1, 0);
	fth_error(fth_to_c_string(fth_pop_ficl_cell(vm)));
}

static void
ficl_print_stdout(ficlVm *vm)
{
#define h_print_stdout "( obj -- )  print OBJ\n\
#{ 'foo 10 } .stdout => #{ 'foo => 10 }\n\
Print string representation of OBJ to stdout.\n\
See also .stderr."
	FTH_STACK_CHECK(vm, 1, 0);
	fth_fprintf(stdout, "%S", fth_pop_ficl_cell(vm));
	fflush(stdout);
}

static void
ficl_print_stderr(ficlVm *vm)
{
#define h_print_stderr "( obj -- )  print OBJ\n\
#{ 'foo 10 } .stderr => #{ 'foo => 10 }\n\
Print string representation of OBJ to stderr.\n\
See also .stdout."
	FTH_STACK_CHECK(vm, 1, 0);
	fth_fprintf(stderr, "%S", fth_pop_ficl_cell(vm));
}

static void
ficl_print_debug(ficlVm *vm)
{
#define h_print_debug "( obj -- )  print OBJ\n\
#{ 'foo 10 } .debug => #<DEBUG(F): #{ 'foo => 10 }>\n\
Print string representation of OBJ wrapped in #<DEBUG(F): ...> to stderr.\n\
See also .stdout and .stderr."
	FTH_STACK_CHECK(vm, 1, 0);
	fth_fprintf(stderr, "#<DEBUG(F): %M>\n", fth_pop_ficl_cell(vm));
}

/*
 * Compare two strings with strcmp(3) and return 1 for equal and
 * 0 for not equal (not -1 0 1 like strcmp).
 *
 * FTH s1 = fth_make_string("foo");
 * FTH s2 = fth_make_string("bar");
 * FTH s3 = fth_make_string("foo");
 * fth_string_equal_p(s1, s2);			=> 0
 * fth_string_equal_p(s1, s3);			=> 1
 * fth_string_equal_p(s3, s3);			=> 1
 */
bool
fth_string_equal_p(FTH obj1, FTH obj2)
{
	if (FTH_STRING_P(obj1) && FTH_STRING_P(obj2)) {
		char *b, *o;

		b = FTH_STRING_DATA(obj1);
		o = FTH_STRING_DATA(obj2);
		return (strcmp(b, o) == 0);

	}
	return (false);
}

static void
ficl_string_cmp(ficlVm *vm)
{
#define h_string_cmp "( str1 str2 -- n )  compare strings\n\
\"foo\" value s1\n\
\"bar\" value s2\n\
\"foo\" value s3\n\
s1 s2 string-cmp => 1\n\
s1 s3 string-cmp => 0\n\
s2 s3 string-cmp => -1\n\
Return -1 if STR1 is less than STR2, \
1 if STR1 is greater than STR2, \
and 0 if STR1 is equal to STR2.  \
It may be used with sort functions.\n\
See also string=, string<>, string<, string>."
	char *s1, *s2;
	int n;

	FTH_STACK_CHECK(vm, 2, 1);
	s2 = pop_cstring(vm);
	s1 = pop_cstring(vm);
	if (s1 == NULL)
		s1 = "";
	if (s2 == NULL)
		s2 = "";
	n = strcmp(s1, s2);
	if (n < 0)
		ficlStackPushInteger(vm->dataStack, -1);
	else if (n > 0)
		ficlStackPushInteger(vm->dataStack, 1);
	else
		ficlStackPushInteger(vm->dataStack, 0);
}

static void
ficl_string_equal_p(ficlVm *vm)
{
#define h_string_equal_p "( str1 str2 -- f )  compare strings\n\
\"foo\" value s1\n\
\"bar\" value s2\n\
\"foo\" value s3\n\
s1 s2 string= => #f\n\
s1 s3 string= => #t\n\
s3 s3 string= => #t\n\
Return #t if strings are equal, otherwise #f.\n\
See also string<>, string<, string>, string-cmp."
	FTH obj1, obj2;

	FTH_STACK_CHECK(vm, 2, 1);
	obj2 = fth_pop_ficl_cell(vm);
	obj1 = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, fth_string_equal_p(obj1, obj2));
}

/*
 * Compare two strings with strcmp(3) and return 0 for equal and
 * 1 for not equal (not -1 0 1 like strcmp).
 *
 * FTH s1 = fth_make_string("foo");
 * FTH s2 = fth_make_string("bar");
 * FTH s3 = fth_make_string("foo");
 * fth_string_not_equal_p(s1, s2);		=> 1
 * fth_string_not_equal_p(s1, s3);		=> 0
 * fth_string_not_equal_p(s3, s3);		=> 0
 */
bool
fth_string_not_equal_p(FTH obj1, FTH obj2)
{
	if (FTH_STRING_P(obj1) && FTH_STRING_P(obj2)) {
		char *b, *o;

		b = FTH_STRING_DATA(obj1);
		o = FTH_STRING_DATA(obj2);
		return (strcmp(b, o) != 0);

	}
	return (false);
}

static void
ficl_string_not_equal_p(ficlVm *vm)
{
#define h_string_not_equal_p "( str1 str2 -- f )  compare strings\n\
\"foo\" value s1\n\
\"bar\" value s2\n\
\"foo\" value s3\n\
s1 s2 string<> => #t\n\
s1 s3 string<> => #f\n\
s3 s3 string<> => #f\n\
Return #t if strings are not equal, otherwise #f.\n\
See also string=, string<, string>, string-cmp."
	FTH obj1, obj2;

	FTH_STACK_CHECK(vm, 2, 1);
	obj2 = fth_pop_ficl_cell(vm);
	obj1 = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, fth_string_not_equal_p(obj1, obj2));
}

/*
 * Compare two strings with strcmp(3) and return 1 for less than and
 * 0 for equal or greater.
 *
 * FTH s1 = fth_make_string("foo");
 * FTH s2 = fth_make_string("bar");
 * FTH s3 = fth_make_string("foo");
 * fth_string_less_p(s1, s2);			=> 0
 * fth_string_less_p(s1, s3);			=> 0
 * fth_string_less_p(s3, s3);			=> 0
 */
bool
fth_string_less_p(FTH obj1, FTH obj2)
{
	if (FTH_STRING_P(obj1) && FTH_STRING_P(obj2)) {
		char *b, *o;

		b = FTH_STRING_DATA(obj1);
		o = FTH_STRING_DATA(obj2);
		return (strcmp(b, o) < 0);

	}
	return (false);
}

static void
ficl_string_less_p(ficlVm *vm)
{
#define h_string_less_p "( str1 str2 -- f )  compare strings\n\
\"foo\" value s1\n\
\"bar\" value s2\n\
\"foo\" value s3\n\
s1 s2 string< => #f\n\
s1 s3 string< => #f\n\
s3 s3 string< => #f\n\
Return #t if STR1 is lexicographically lesser than STR2, otherwise #f.\n\
See also string=, string<>, string>, string-cmp."
	FTH obj1, obj2;

	FTH_STACK_CHECK(vm, 2, 1);
	obj2 = fth_pop_ficl_cell(vm);
	obj1 = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, fth_string_less_p(obj1, obj2));
}

/*
 * Compare two strings with strcmp(3) and return 1 for greater than and
 * 0 for equal or less than.
 *
 * FTH s1 = fth_make_string("foo");
 * FTH s2 = fth_make_string("bar");
 * FTH s3 = fth_make_string("foo");
 * fth_string_greater_p(s1, s2);		=> 1
 * fth_string_greater_p(s1, s3);		=> 0
 * fth_string_greater_p(s3, s3);		=> 0
 */
bool
fth_string_greater_p(FTH obj1, FTH obj2)
{
	if (FTH_STRING_P(obj1) && FTH_STRING_P(obj2)) {
		char *b, *o;

		b = FTH_STRING_DATA(obj1);
		o = FTH_STRING_DATA(obj2);
		return (strcmp(b, o) > 0);

	}
	return (false);
}

static void
ficl_string_greater_p(ficlVm *vm)
{
#define h_string_greater_p "( str1 str2 -- f )  compare strings\n\
\"foo\" value s1\n\
\"bar\" value s2\n\
\"foo\" value s3\n\
s1 s2 string> => #t\n\
s1 s3 string> => #f\n\
s3 s3 string> => #f\n\
Return #t if STR1 is lexicographically greater than STR2, otherwise #f.\n\
See also string=, string<>, string<, string-cmp."
	FTH obj1, obj2;

	FTH_STACK_CHECK(vm, 2, 1);
	obj2 = fth_pop_ficl_cell(vm);
	obj1 = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, fth_string_greater_p(obj1, obj2));
}

/*
 * FTH fs = fth_make_string("foo");
 * fth_string_to_array(fs);			=> #( 102 111 111 )
 */
FTH
fth_string_to_array(FTH fs)
{
#define h_str_to_ary "( str -- ary )  return array\n\
\"foo\" string->array => #( 102 111 111 )\n\
Convert STR to an array of characters."
	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	return (str_to_array(fs));
}

/*
 * Return a new copy of the Fth string object.
 *
 * FTH s1 = fth_make_string("foo");
 * FTH s2 = fth_string_copy(s1);
 * s1 == s2					=> 0
 * fth_string_equal_p(s1, s2);			=> 1
 */
FTH
fth_string_copy(FTH fs)
{
#define h_string_copy "( str1 -- str2 )  duplicate string\n\
\"foo\" value s1\n\
s1 string-copy value s2\n\
s1 string-reverse! drop\n\
s1 => \"oof\"\n\
s2 => \"foo\"\n\
Return copy of STR1."
	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	return (str_to_string(fs));
}

/*
 * FIXME: to be continued
 */
char
fth_string_c_char_ref(FTH fs, ficlInteger idx)
{
	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	if (idx < 0)
		idx += FTH_STRING_LENGTH(fs);
	if (idx < 0 || idx >= FTH_STRING_LENGTH(fs))
		FTH_OUT_OF_BOUNDS(FTH_ARG2, idx);
	return (FTH_STRING_DATA(fs)[idx]);
}

/* Dangerous! No check for string or string bounds. */
char
fth_string_c_char_fast_ref(FTH fs, ficlInteger idx)
{
	return (FTH_STRING_DATA(fs)[idx]);
}

FTH
fth_string_char_ref(FTH fs, ficlInteger idx)
{
	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	if (idx < 0)
		idx += FTH_STRING_LENGTH(fs);
	return (str_ref(fs, fth_make_int(idx)));
}

static void
ficl_string_ref(ficlVm *vm)
{
#define h_string_ref "( string idx -- value )  return char\n\
\"foo\" 1 string-ref => 111\n\
Return character at position IDX; negative index counts from backward.  \
Raise OUT-OF-RANGE exception if index is not in range of string."
	ficlInteger idx;
	FTH string;

	FTH_STACK_CHECK(vm, 2, 1);
	idx = ficlStackPopInteger(vm->dataStack);
	string = fth_pop_ficl_cell(vm);
	fth_push_ficl_cell(vm, fth_string_char_ref(string, idx));
}

char
fth_string_c_char_set(FTH fs, ficlInteger idx, char c)
{
	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	if (idx < 0)
		idx += FTH_STRING_LENGTH(fs);
	if (idx < 0 || idx >= FTH_STRING_LENGTH(fs))
		FTH_OUT_OF_BOUNDS(FTH_ARG2, idx);
	FTH_INSTANCE_CHANGED(fs);
	return (FTH_STRING_DATA(fs)[idx] = c);
}

/* Dangerous! No check for string or string bounds. */
char
fth_string_c_char_fast_set(FTH fs, ficlInteger idx, char c)
{
	FTH_INSTANCE_CHANGED(fs);
	return (FTH_STRING_DATA(fs)[idx] = c);
}

FTH
fth_string_char_set(FTH fs, ficlInteger idx, FTH value)
{
	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	if (idx < 0)
		idx += FTH_STRING_LENGTH(fs);
	return (str_set(fs, fth_make_int(idx), value));
}

static void
ficl_string_set(ficlVm *vm)
{
#define h_string_set "( string idx char -- )  set CHAR\n\
\"foo\" value s1\n\
s1 1 <char> e string-set!\n\
s1 => \"feo\"\n\
Store character CHAR at index IDX; negative index counts from backward.  \
Raise OUT-OF-RANGE exception if index is not in range of string."
	ficlInteger idx;
	FTH string, value;

	FTH_STACK_CHECK(vm, 3, 0);
	value = fth_pop_ficl_cell(vm);
	idx = ficlStackPopInteger(vm->dataStack);
	string = fth_pop_ficl_cell(vm);
	fth_string_char_set(string, idx, value);
}

FTH
fth_string_push(FTH fs, FTH add)
{
#define h_string_push "( string value -- string' )  add to STRING\n\
\"foo\" value s1\n\
s1 $space string-push drop\n\
s1 10     string-push drop\n\
s1 => \"foo 10\"\n\
Append string representation of VALUE to STRING \
and return changed string object.\n\
See also string-pop, string-unshift, string-shift."
	ficlInteger new_buf_len, l, sl, al;
	size_t st;
	char *b;

	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	if (!FTH_STRING_P(add))
		add = fth_object_to_string(add);
	if (FTH_STRING_LENGTH(add) == 0)
		return (fs);
	sl = FTH_STRING_LENGTH(fs);
	al = FTH_STRING_LENGTH(add);
	new_buf_len = FTH_STRING_TOP(fs) + sl + al + 1;
	if (new_buf_len > FTH_STRING_BUF_LENGTH(fs)) {
		l = NEW_SEQ_LENGTH(new_buf_len);
		if (l > MAX_SEQ_LENGTH)
			FTH_OUT_OF_BOUNDS_ERROR(FTH_ARG1, l, "too long");
		FTH_STRING_BUF_LENGTH(fs) = l;
		st = (size_t)l;
		FTH_STRING_BUF(fs) = FTH_REALLOC(FTH_STRING_BUF(fs), st);
		FTH_STRING_DATA(fs) = FTH_STRING_BUF(fs) + FTH_STRING_TOP(fs);
	}
	st = (size_t)FTH_STRING_LENGTH(add);
	b = FTH_STRING_DATA(add);
	memmove(FTH_STRING_DATA(fs) + sl, b, st);
	FTH_STRING_LENGTH(fs) += al;
	FTH_STRING_DATA(fs)[FTH_STRING_LENGTH(fs)] = '\0';
	FTH_INSTANCE_CHANGED(fs);
	return (fs);
}

FTH
fth_string_pop(FTH fs)
{
#define h_string_pop "( string -- char )  return last char\n\
\"foo\" value s1\n\
s1 string-pop => 111\n\
s1 string-pop => 111\n\
s1 string-pop => 102\n\
s1 string-pop => #f\n\
Remove and return last character.  \
If STR is empty, return #f.\n\
See also string-push, string-unshift, string-shift."
	ficlInteger l;
	FTH c = FTH_FALSE;

	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	if (FTH_STRING_LENGTH(fs) == 0)
		return (c);
	/* - 1 (length--) + 1 ('\0') */
	l = NEW_SEQ_LENGTH(FTH_STRING_TOP(fs) + FTH_STRING_LENGTH(fs));
	FTH_STRING_LENGTH(fs)--;
	c = CHAR_TO_FTH(FTH_STRING_DATA(fs)[FTH_STRING_LENGTH(fs)]);
	FTH_STRING_DATA(fs)[FTH_STRING_LENGTH(fs)] = '\0';
	if (l < FTH_STRING_BUF_LENGTH(fs)) {
		FTH_STRING_BUF_LENGTH(fs) = l;
		FTH_STRING_BUF(fs) = FTH_REALLOC(FTH_STRING_BUF(fs), (size_t)l);
		FTH_STRING_DATA(fs) = FTH_STRING_BUF(fs) + FTH_STRING_TOP(fs);
	}
	FTH_INSTANCE_CHANGED(fs);
	return (c);
}

FTH
fth_string_unshift(FTH fs, FTH add)
{
#define h_str_unshift "( string value -- string' )  prepend to STRING\n\
\"foo\" value s1\n\
s1 $space string-unshift drop\n\
s1 10     string-unshift drop\n\
s1 => \"10 foo\"\n\
Prepends string representation of VALUE to STRING \
and return changed string object.\n\
See also string-push, string-pop, string-shift."
	ficlInteger l, new_top, new_len, new_buf_len, al;
	char *b;
	size_t st;

	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	if (!FTH_STRING_P(add))
		add = fth_object_to_string(add);
	if (FTH_STRING_LENGTH(add) == 0)
		return (fs);
	al = FTH_STRING_LENGTH(add);
	new_top = FTH_STRING_TOP(fs) - al;
	new_len = FTH_STRING_LENGTH(fs) + al;
	new_buf_len = new_top + new_len + 1;
	if (new_top < 1) {
		new_top = FTH_STRING_BUF_LENGTH(fs) / 3;
		new_buf_len = new_top + new_len + 1;
		if (new_buf_len > FTH_STRING_BUF_LENGTH(fs)) {
			l = NEW_SEQ_LENGTH(new_buf_len);
			if (l > MAX_SEQ_LENGTH)
				FTH_OUT_OF_BOUNDS_ERROR(1, l, "too long");
			FTH_STRING_BUF_LENGTH(fs) = l;
			st = (size_t)l;
			FTH_STRING_BUF(fs) = FTH_REALLOC(FTH_STRING_BUF(fs),st);
			FTH_STRING_DATA(fs) =
			    FTH_STRING_BUF(fs) + FTH_STRING_TOP(fs);
		}
		b = FTH_STRING_DATA(fs);
		st = (size_t)FTH_STRING_LENGTH(fs);
		memmove(FTH_STRING_BUF(fs) + new_top + al, b, st);
	} else if (new_buf_len > FTH_STRING_BUF_LENGTH(fs)) {
		l = NEW_SEQ_LENGTH(new_buf_len);
		if (l > MAX_SEQ_LENGTH)
			FTH_OUT_OF_BOUNDS_ERROR(FTH_ARG1, l, "too long");
		FTH_STRING_BUF_LENGTH(fs) = l;
		st = (size_t)l;
		FTH_STRING_BUF(fs) = FTH_REALLOC(FTH_STRING_BUF(fs), st);
		FTH_STRING_DATA(fs) = FTH_STRING_BUF(fs) + FTH_STRING_TOP(fs);
	}
	FTH_STRING_TOP(fs) = new_top;
	FTH_STRING_LENGTH(fs) = new_len;
	b = FTH_STRING_DATA(add);
	st = (size_t)al;
	memmove(FTH_STRING_BUF(fs) + FTH_STRING_TOP(fs), b, st);
	FTH_STRING_DATA(fs) = FTH_STRING_BUF(fs) + FTH_STRING_TOP(fs);
	FTH_STRING_DATA(fs)[FTH_STRING_LENGTH(fs)] = '\0';
	FTH_INSTANCE_CHANGED(fs);
	return (fs);
}

FTH
fth_string_shift(FTH fs)
{
#define h_string_shift "( string -- char )  return first char\n\
\"foo\" value s1\n\
s1 string-shift => 102\n\
s1 string-shift => 111\n\
s1 string-shift => 111\n\
s1 string-shift => #f\n\
Remove and return first character.  \
If STR is empty, return #f.\n\
See also string-push, string-pop, string-unshift."
	ficlInteger l;
	size_t st;
	char *b;
	FTH c = FTH_FALSE;

	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	if (FTH_STRING_LENGTH(fs) == 0)
		return (c);
	b = FTH_STRING_DATA(fs);
	c = CHAR_TO_FTH(b[0]);
	if ((FTH_STRING_TOP(fs) + 1) > (FTH_STRING_BUF_LENGTH(fs) / 2)) {
		FTH_STRING_TOP(fs) = FTH_STRING_BUF_LENGTH(fs) / 3;
		st = (size_t)FTH_STRING_LENGTH(fs);
		memmove(FTH_STRING_BUF(fs) + FTH_STRING_TOP(fs), b, st);
	}
	/* - 1 (length--) + 1 ('\0') */
	l = NEW_SEQ_LENGTH(FTH_STRING_TOP(fs) + FTH_STRING_LENGTH(fs));
	FTH_STRING_LENGTH(fs)--;
	FTH_STRING_TOP(fs)++;
	if (l < FTH_STRING_BUF_LENGTH(fs)) {
		FTH_STRING_BUF_LENGTH(fs) = l;
		st = (size_t)l;
		FTH_STRING_BUF(fs) = FTH_REALLOC(FTH_STRING_BUF(fs), st);
	}
	FTH_STRING_DATA(fs) = FTH_STRING_BUF(fs) + FTH_STRING_TOP(fs);
	FTH_INSTANCE_CHANGED(fs);
	return (c);
}

FTH
fth_string_append(FTH fs1, FTH fs2)
{
#define h_string_append "( str1 str2 -- str3 )  return string STR1+STR2\n\
\"foo\" value s1\n\
\"bar\" value s2\n\
s1 s2 string-append value s3\n\
s1 => \"foo\"\n\
s2 => \"bar\"\n\
s3 => \"foobar\"\n\
Return new string from STR1+STR2.\n\
See also string-concat and string-push."
	char *b1, *b2;

	FTH_ASSERT_ARGS(FTH_STRING_P(fs1), fs1, FTH_ARG1, "a string");
	FTH_ASSERT_ARGS(FTH_STRING_P(fs2), fs2, FTH_ARG2, "a string");
	b1 = FTH_STRING_DATA(fs1);
	b2 = FTH_STRING_DATA(fs2);
	return (fth_make_string_format("%s%s", b1, b2));
}

FTH
fth_string_reverse(FTH fs)
{
	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	ficlStringReverse(FTH_STRING_REF(fs));
	FTH_INSTANCE_CHANGED(fs);
	return (fs);
}

static void
ficl_string_reverse(ficlVm *vm)
{
#define h_string_reverse "( str1 -- str2 )  reverse string\n\
\"foo\" value s1\n\
s1 string-reverse value s2\n\
s1 => \"foo\"\n\
s2 => \"oof\"\n\
Return STR1 reversed as new string object.\n\
See also string-reverse!."
	FTH fs;

	FTH_STACK_CHECK(vm, 1, 1);
	fs = fth_string_copy(fth_pop_ficl_cell(vm));
	ficlStackPushFTH(vm->dataStack, fth_string_reverse(fs));
}

static void
ficl_string_reverse_bang(ficlVm *vm)
{
#define h_str_rev_bang "( str -- str' )  reverse string\n\
\"foo\" value s1\n\
s1 string-reverse! drop\n\
s1 => \"oof\"\n\
Return STR reversed.\n\
See also string-reverse."
	FTH fs;

	FTH_STACK_CHECK(vm, 1, 1);
	fs = fth_pop_ficl_cell(vm);
	ficlStackPushFTH(vm->dataStack, fth_string_reverse(fs));
}

FTH
fth_string_insert(FTH fs, ficlInteger idx, FTH ins)
{
	ficlInteger sl, il, rl, new_buf_len, l;
	size_t st;

	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	sl = FTH_STRING_LENGTH(fs);
	if (idx < 0)
		idx += sl;
	if (idx == 0) {
		fth_string_unshift(fs, ins);
		return (fs);
	}
	if (idx < 0 || idx >= sl)
		FTH_OUT_OF_BOUNDS(FTH_ARG2, idx);
	if (!FTH_STRING_P(ins))
		ins = fth_object_to_string(ins);
	il = FTH_STRING_LENGTH(ins);
	if (il == 0)
		return (fs);
	rl = sl + il + 1;
	new_buf_len = FTH_STRING_TOP(fs) + rl;
	if (new_buf_len > FTH_STRING_BUF_LENGTH(fs)) {
		l = NEW_SEQ_LENGTH(new_buf_len);
		if (l > MAX_SEQ_LENGTH)
			FTH_OUT_OF_BOUNDS_ERROR(FTH_ARG1, l, "too long");
		FTH_STRING_BUF_LENGTH(fs) = l;
		st = (size_t)l;
		FTH_STRING_BUF(fs) = FTH_REALLOC(FTH_STRING_BUF(fs), st);
		FTH_STRING_DATA(fs) = FTH_STRING_BUF(fs) + FTH_STRING_TOP(fs);
	}
	st = (size_t)(sl - idx);
	memmove(FTH_STRING_DATA(fs) + idx + il, FTH_STRING_DATA(fs) + idx, st);
	st = (size_t)il;
	memmove(FTH_STRING_DATA(fs) + idx, FTH_STRING_DATA(ins), st);
	FTH_STRING_LENGTH(fs) += FTH_STRING_LENGTH(ins);
	FTH_STRING_DATA(fs)[FTH_STRING_LENGTH(fs)] = '\0';
	FTH_INSTANCE_CHANGED(fs);
	return (fs);
}

static void
ficl_string_insert(ficlVm *vm)
{
#define h_string_insert "( string idx value -- string' )  insert element\n\
\"foo\" value s1\n\
s1 1 10 string-insert! drop\n\
s1 => \"f10oo\"\n\
Insert string representation of VALUE to STRING at position IDX; \
negative index counts from backward.  \
Raise OUT-OF-RANGE exception if index is not in range of string."
	FTH str, ins;
	ficlInteger idx;

	FTH_STACK_CHECK(vm, 3, 1);
	ins = fth_pop_ficl_cell(vm);
	idx = ficlStackPopInteger(vm->dataStack);
	str = fth_pop_ficl_cell(vm);
	ficlStackPushFTH(vm->dataStack, fth_string_insert(str, idx, ins));
}

FTH
fth_string_delete(FTH fs, ficlInteger idx)
{
	ficlInteger l;
	size_t st;
	FTH c = FTH_FALSE;

	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	if (FTH_STRING_LENGTH(fs) == 0)
		return (c);
	if (idx < 0)
		idx += FTH_STRING_LENGTH(fs);
	if (idx < 0 || idx >= FTH_STRING_LENGTH(fs))
		FTH_OUT_OF_BOUNDS(FTH_ARG2, idx);
	if (idx == 0)
		return (fth_string_shift(fs));
	if (idx == (FTH_STRING_LENGTH(fs) - 1))
		return (fth_string_pop(fs));
	c = CHAR_TO_FTH(FTH_STRING_DATA(fs)[idx]);
	FTH_STRING_LENGTH(fs)--;
	l = NEW_SEQ_LENGTH(FTH_STRING_TOP(fs) + FTH_STRING_LENGTH(fs) + 1);
	if (l < FTH_STRING_BUF_LENGTH(fs)) {
		FTH_STRING_BUF_LENGTH(fs) = l;
		st = (size_t)l;
		FTH_STRING_BUF(fs) = FTH_REALLOC(FTH_STRING_BUF(fs), st);
		FTH_STRING_DATA(fs) = FTH_STRING_BUF(fs) + FTH_STRING_TOP(fs);
	}
	st = (size_t)(FTH_STRING_LENGTH(fs) - idx);
	memmove(FTH_STRING_DATA(fs) + idx, FTH_STRING_DATA(fs) + idx + 1, st);
	FTH_STRING_DATA(fs)[FTH_STRING_LENGTH(fs)] = '\0';
	FTH_INSTANCE_CHANGED(fs);
	return (c);
}

static void
ficl_string_delete(ficlVm *vm)
{
#define h_string_delete "( string idx -- char )  delete char\n\
\"foo\" value s1\n\
s1 1 string-delete! => 111\n\
s1 => \"fo\"\n\
Delete and return character at position IDX from STRING; \
negative index counts from backward.  \
Raise OUT-OF-RANGE exception if index is not in range of string."
	FTH str;
	ficlInteger idx;

	FTH_STACK_CHECK(vm, 2, 1);
	idx = ficlStackPopInteger(vm->dataStack);
	str = fth_pop_ficl_cell(vm);
	fth_push_ficl_cell(vm, fth_string_delete(str, idx));
}

FTH
fth_string_fill(FTH fs, FTH fill_char)
{
#define h_string_fill "( string char -- string' )  fill string\n\
\"foo\" value s1\n\
s1 <char> a string-fill drop\n\
s1 => \"aaa\"\n\
Fill STRING with CHAR and return changed string object."
	size_t st;

	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	FTH_ASSERT_ARGS(FTH_CHAR_P(fill_char), fill_char, FTH_ARG2, "a char");
	st = (size_t)FTH_STRING_LENGTH(fs);
	memset(FTH_STRING_DATA(fs), FTH_TO_CCHAR(fill_char), st);
	FTH_INSTANCE_CHANGED(fs);
	return (fs);
}

FTH
fth_string_index(FTH fs, FTH key)
{
#define h_string_index "( str key -- index|-1 )  find KEY\n\
\"hello world\" \"l\"   string-index => 2\n\
\"hello world\" \"orl\" string-index => 7\n\
\"hello world\" \"k\"   string-index => -1\n\
Return index of string KEY in STR or -1 if not found.\n\
See also string-member? and string-find."
	char *res;

	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	FTH_ASSERT_ARGS(FTH_STRING_P(key), key, FTH_ARG2, "a string");
	res = strstr(FTH_STRING_DATA(fs), FTH_STRING_DATA(key));
	if (res != NULL)
		return (fth_make_int(res - FTH_STRING_DATA(fs)));
	return (FTH_ONE_NEG);
}

bool
fth_string_member_p(FTH fs, FTH key)
{
	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	FTH_ASSERT_ARGS(FTH_STRING_P(key), key, FTH_ARG2, "a string");
	return (strstr(FTH_STRING_DATA(fs), FTH_STRING_DATA(key)) != NULL);
}

static void
ficl_string_member_p(ficlVm *vm)
{
#define h_string_member_p "( str key -- f )  find KEY\n\
\"hello world\" \"l\"   string-member? => #t\n\
\"hello world\" \"ell\" string-member? => #t\n\
\"hello world\" \"k\"   string-member? => #f\n\
Return #t if string KEY exist in STR, otherwise #f.\n\
See also string-index and string-find."
	FTH str, key;

	FTH_STACK_CHECK(vm, 2, 1);
	key = fth_pop_ficl_cell(vm);
	str = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, fth_string_member_p(str, key));
}

FTH
fth_string_find(FTH fs, FTH key)
{
#define h_string_find "( str1 key -- str2|#f )  find KEY\n\
\"hello world\" \"l\"   string-find => \"llo world\"\n\
\"hello world\" /ell/ string-find => \"lo world\"\n\
\"hello world\" /k/   string-find => #f\n\
Return match if string or regexp KEY exist in STR, otherwise #f.\n\
See also string-index and string-member?."
	ficlInteger pos;

	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	FTH_ASSERT_ARGS(FTH_STRREG_P(key), key, FTH_ARG2, "a string or regexp");
	if (FTH_STRING_P(key)) {
		char *str, *k, *substr;

		str = FTH_STRING_REF(fs);
		k = FTH_STRING_REF(key);
		substr = NULL;
		if (str != NULL && k != NULL)
			substr = strstr(str, k);
		if (substr != NULL)
			return (fth_make_string(substr));
		return (FTH_FALSE);
	}
	pos = fth_regexp_match(key, fs);
	if (pos == -1)
		return (FTH_FALSE);
	if (pos == 0)
		return (fth_make_empty_string());
	if (FTH_FALSE_P(fth_variable_ref("*re0*")))
		return (fth_string_substring(fs, pos, FTH_STRING_LENGTH(fs)));
	return (fth_variable_ref("*re0*"));
}

FTH
fth_string_split(FTH fs, FTH reg)
{
#define h_string_split "( str sep -- ary )  return array\n\
\"foo:bar:baz\" \":\" string-split => #( \"foo\" \"bar\" \"baz\" )\n\
\"foo:bar:baz\" \"/:/\" string-split => #( \"foo\" \"bar\" \"baz\" )\n\
\"foo bar baz\" nil string-split => #( \"foo\" \"bar\" \"baz\" )\n\
Split STR using SEP as delimiter and return result as array of strings.  \
If SEP is not a string or regexp, delimiter is space."
	FTH result;
	char *delim, *p, *s, *str;

	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	if (FTH_STRING_LENGTH(fs) == 0)
		return (fth_make_array_var(1, fs));
	result = fth_make_empty_array();
	if (FTH_REGEXP_P(reg)) {
		FTH s;
		char *b;
		ficlInteger start, range, pos;

		start = 0;
		range = FTH_STRING_LENGTH(fs);
		b = FTH_STRING_DATA(fs);
		while ((pos = fth_regexp_search(reg, fs, start, range)) >= 0) {
			s = fth_make_string_len(b + start, pos - start);
			fth_array_push(result, s);
			if (fth_array_length(fth_object_to_array(reg)) > 0) {
				s = fth_object_value_ref(reg, 0L);
				start = pos + fth_string_length(s);
			} else
				start = pos + 1;
		}
		if ((range - start) >= 0) {
			s = fth_make_string_len(b + start, range - start);
			fth_array_push(result, s);
		}
		return (result);
	}
	s = str = FTH_STRDUP(FTH_STRING_DATA(fs));
	delim = FTH_STRING_P(reg) ? FTH_STRING_DATA(reg) : " ";
	while ((p = strsep(&s, delim)) != NULL)
		if (*p != '\0')
			fth_array_push(result, fth_make_string(p));
	FTH_FREE(str);
	return (result);
}

/*
 * The split string or regexp won't be removed (for io.c, fth_readlines()).
 *
 * "foo\nbar\nbaz" => #( "foo\n" "bar\n" "baz" )
 */
FTH
fth_string_split_2(FTH fs, FTH reg)
{
	char *b;
	ficlInteger start, range, pos, len;
	FTH result, s;

	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	FTH_ASSERT_ARGS(FTH_STRREG_P(reg), reg, FTH_ARG2, "a string or regexp");
	if (FTH_STRING_LENGTH(fs) == 0)
		return (fth_make_array_var(1, fs));
	if (FTH_STRING_P(reg))
		reg = fth_make_regexp(FTH_STRING_DATA(reg));
	start = 0;
	range = FTH_STRING_LENGTH(fs);
	b = FTH_STRING_DATA(fs);
	result = fth_make_empty_array();
	while ((pos = fth_regexp_search(reg, fs, start, range)) >= 0) {
		s = fth_object_value_ref(reg, 0L);
		len = fth_string_length(s);
		if ((pos + len - start) > 0) {
			s = fth_make_string_len(b + start, pos + len - start);
			fth_array_push(result, s);
		}
		start = pos + 1;
	}
	if ((range - start) > 0) {
		s = fth_make_string_len(b + start, range - start);
		fth_array_push(result, s);
	}
	return (result);
}

FTH
fth_string_substring(FTH fs, ficlInteger start, ficlInteger end)
{
	FTH res;
	size_t st;

	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	if (start < 0)
		start += FTH_STRING_LENGTH(fs);
	if (start < 0 || start >= FTH_STRING_LENGTH(fs))
		FTH_OUT_OF_BOUNDS(FTH_ARG2, start);
	if (end < 0)
		end += FTH_STRING_LENGTH(fs);
	if (end < start || end > FTH_STRING_LENGTH(fs))
		end = FTH_STRING_LENGTH(fs);
	res = fth_make_string_len(FTH_STRING_DATA(fs), end - start);
	st = (size_t)FTH_STRING_LENGTH(res);
	memmove(FTH_STRING_DATA(res), FTH_STRING_DATA(fs) + start, st);
	return (res);
}

static void
ficl_string_substring(ficlVm *vm)
{
#define h_string_substring "( str1 start end -- str2 )  return substring\n\
\"hello world\"  2   4 string-substring => \"ll\"\n\
\"hello world\" -4  -2 string-substring => \"or\"\n\
\"hello world\" -4 nil string-substring => \"orld\"\n\
Return new string from STR index START to but excluding index END.  \
If END is not an integer, END will be set to length of STR1; \
negative index counts from backward.  \
Raise OUT-OF-RANGE exception if index is not in range of string."
	FTH fs, last;
	ficlInteger beg, end;

	FTH_STACK_CHECK(vm, 3, 1);
	last = fth_pop_ficl_cell(vm);
	beg = ficlStackPopInteger(vm->dataStack);
	fs = ficlStackPopFTH(vm->dataStack);
	end = (FTH_INTEGER_P(last)) ? FTH_INT_REF(last) : fth_string_length(fs);
	ficlStackPushFTH(vm->dataStack, fth_string_substring(fs, beg, end));
}

FTH
fth_string_upcase(FTH fs)
{
	ficlInteger i;
	char *b;

	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	if (FTH_STRING_LENGTH(fs) == 0)
		return (fs);
	b = FTH_STRING_DATA(fs);
	for (i = 0; i < FTH_STRING_LENGTH(fs); i++)
		b[i] = (char)toupper((int)b[i]);
	FTH_INSTANCE_CHANGED(fs);
	return (fs);
}

static void
ficl_string_upcase(ficlVm *vm)
{
#define h_string_upcase "( str1 -- str2 )  convert chars\n\
\"Foo\" value s1\n\
s1 string-upcase value s2\n\
s1 => \"Foo\"\n\
s2 => \"FOO\"\n\
Return new string with all characters uppercase.\n\
See also string-upcase!, string-downcase, string-capitalize."
	FTH fs;

	FTH_STACK_CHECK(vm, 1, 1);
	fs = fth_string_copy(fth_pop_ficl_cell(vm));
	ficlStackPushFTH(vm->dataStack, fth_string_upcase(fs));
}

static void
ficl_string_upcase_bang(ficlVm *vm)
{
#define h_str_up_bang "( str -- str' )  convert chars\n\
\"Foo\" value s1\n\
s1 string-upcase! drop\n\
s1 => \"FOO\"\n\
Return STR changed to all characters uppercase.\n\
See also string-upcase, string-downcase, string-capitalize."
	FTH fs;

	FTH_STACK_CHECK(vm, 1, 1);
	fs = fth_pop_ficl_cell(vm);
	ficlStackPushFTH(vm->dataStack, fth_string_upcase(fs));
}

FTH
fth_string_downcase(FTH fs)
{
	ficlInteger i;
	char *b;

	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	if (FTH_STRING_LENGTH(fs) == 0)
		return (fs);
	b = FTH_STRING_DATA(fs);
	for (i = 0; i < FTH_STRING_LENGTH(fs); i++)
		b[i] = (char)tolower((int)b[i]);
	FTH_INSTANCE_CHANGED(fs);
	return (fs);
}

static void
ficl_string_downcase(ficlVm *vm)
{
#define h_string_downcase "( str1 -- str2 )  convert chars\n\
\"Foo\" value s1\n\
s1 string-downcase value s2\n\
s1 => \"Foo\"\n\
s2 => \"foo\"\n\
Return new string with all characters lowercase.\n\
See also string-downcase!, string-upcase, string-capitalize."
	FTH fs;

	FTH_STACK_CHECK(vm, 1, 1);
	fs = fth_string_copy(fth_pop_ficl_cell(vm));
	ficlStackPushFTH(vm->dataStack, fth_string_downcase(fs));
}

static void
ficl_string_downcase_bang(ficlVm *vm)
{
#define h_str_dn_bang "( str -- str' )  convert chars\n\
\"Foo\" value s1\n\
s1 string-downcase! drop\n\
s1 => \"foo\"\n\
Return STR changed to all characters lowercase.\n\
See also string-downcase, string-upcase, string-capitalize."
	FTH fs;

	FTH_STACK_CHECK(vm, 1, 1);
	fs = fth_pop_ficl_cell(vm);
	ficlStackPushFTH(vm->dataStack, fth_string_downcase(fs));
}

FTH
fth_string_capitalize(FTH fs)
{
	ficlInteger i;
	char *b;

	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	if (FTH_STRING_LENGTH(fs) == 0)
		return (fs);
	b = FTH_STRING_DATA(fs);
	b[0] = (char)toupper((int)b[0]);
	for (i = 1; i < FTH_STRING_LENGTH(fs); i++)
		b[i] = (char)tolower((int)b[i]);
	FTH_INSTANCE_CHANGED(fs);
	return (fs);
}

static void
ficl_string_capitalize(ficlVm *vm)
{
#define h_str_cap "( str1 -- str2 )  capitalize first char\n\
\"foO\" value s1\n\
s1 string-capitalize value s2\n\
s1 => \"foO\"\n\
s2 => \"Foo\"\n\
Return new string with first character capitalized and \
remaining characters in lowercase.\n\
See also string-capitalize!, string-upcase, string-downcase."
	FTH fs;

	FTH_STACK_CHECK(vm, 1, 1);
	fs = fth_string_copy(fth_pop_ficl_cell(vm));
	ficlStackPushFTH(vm->dataStack, fth_string_capitalize(fs));
}

static void
ficl_string_capitalize_bang(ficlVm *vm)
{
#define h_str_c_b "( str -- str' )  capitalize first char\n\
\"foO\" value s1\n\
s1 string-capitalize! drop\n\
s1 => \"Foo\"\n\
Return STR changed to first character capitalized and \
remaining characters to lowercase.\n\
See also string-capitalize, string-upcase, string-downcase."
	FTH fs;

	FTH_STACK_CHECK(vm, 1, 1);
	fs = fth_pop_ficl_cell(vm);
	ficlStackPushFTH(vm->dataStack, fth_string_capitalize(fs));
}

FTH
fth_string_replace(FTH fs, FTH from, FTH to)
{
	char *tmp, *b, *cf, *ct;
	ficlInteger i, l, lf, lt;
	size_t st;

	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	if (FTH_STRING_LENGTH(fs) == 0)
		return (fs);
	FTH_ASSERT_ARGS(FTH_STRING_P(from), from, FTH_ARG2, "a string");
	FTH_ASSERT_ARGS(FTH_STRING_P(to), to, FTH_ARG3, "a string");
	if (FTH_STRING_LENGTH(from) == 1 && FTH_STRING_LENGTH(to) == 1) {
		char f, t;

		b = FTH_STRING_DATA(fs);
		f = FTH_STRING_DATA(from)[0];
		t = FTH_STRING_DATA(to)[0];
		for (i = 0; i < FTH_STRING_LENGTH(fs); i++)
			if (b[i] == f)
				b[i] = t;
		FTH_INSTANCE_CHANGED(fs);
		return (fs);
	}
	tmp = b = FTH_STRING_DATA(fs);
	cf = FTH_STRING_DATA(from);
	lf = FTH_STRING_LENGTH(from);
	ct = FTH_STRING_DATA(to);
	lt = FTH_STRING_LENGTH(to);
	if (lt == 0) {
		while ((tmp = strstr(tmp, cf))) {
			i = tmp - b;
			/* delete */
			FTH_STRING_LENGTH(fs) -= lf;
			st = (size_t)(FTH_STRING_LENGTH(fs) - i);
			memmove(tmp, tmp + lf, st);
			b[FTH_STRING_LENGTH(fs)] = '\0';
		}
		FTH_INSTANCE_CHANGED(fs);
		return (fs);
	}
	while ((tmp = strstr(tmp, cf))) {
		i = tmp - b;
		/* delete */
		FTH_STRING_LENGTH(fs) -= lf;
		st = (size_t)(FTH_STRING_LENGTH(fs) - i);
		memmove(tmp, tmp + lf, st);
		/* insert */
		st = FTH_STRING_TOP(fs) + FTH_STRING_LENGTH(fs) + lt + 1;
		l = NEW_SEQ_LENGTH(st);
		if (l > MAX_SEQ_LENGTH)
			FTH_OUT_OF_BOUNDS_ERROR(FTH_ARG1, l, "too long");
		if (l > FTH_STRING_BUF_LENGTH(fs)) {
			FTH_STRING_BUF_LENGTH(fs) = l;
			st = (size_t)l;
			FTH_STRING_BUF(fs) = FTH_REALLOC(FTH_STRING_BUF(fs),st);
			FTH_STRING_DATA(fs) =
			    FTH_STRING_BUF(fs) + FTH_STRING_TOP(fs);
		}
		st = (size_t)(FTH_STRING_LENGTH(fs) - i);
		memmove(FTH_STRING_DATA(fs) + i + lt, b + i, st);
		st = (size_t)lt;
		memmove(b + i, FTH_STRING_DATA(to), st);
		FTH_STRING_LENGTH(fs) += lt;
		b[FTH_STRING_LENGTH(fs)] = '\0';
		/* skip over added "to" string */
		tmp += lt;
	}
	FTH_INSTANCE_CHANGED(fs);
	return (fs);
}

static void
ficl_string_replace(ficlVm *vm)
{
#define h_string_replace "( str1 from to -- str2 )  replace in string\n\
\"foo\" value s1\n\
s1 \"o\"  \"a\" string-replace value s2\n\
s1 \"oo\" \"a\" string-replace value s3\n\
s1 \"o\"  \"\"  string-replace value s4\n\
s1 => \"foo\"\n\
s2 => \"faa\"\n\
s3 => \"fa\"\n\
s4 => \"f\"\n\
Return new string object with string FROM replaced by string TO.  \
If TO is the empty string, delete the FROM part from STR1.\n\
See also string-replace!."
	FTH str, from, to;

	FTH_STACK_CHECK(vm, 3, 1);
	to = fth_pop_ficl_cell(vm);
	from = fth_pop_ficl_cell(vm);
	str = fth_string_copy(fth_pop_ficl_cell(vm));
	ficlStackPushFTH(vm->dataStack, fth_string_replace(str, from, to));
}

static void
ficl_string_replace_bang(ficlVm *vm)
{
#define h_str_rep_bang "( str from to -- str' )  replace in string\n\
\"foo\" value s1\n\
\"foo\" value s2\n\
\"foo\" value s3\n\
s1 \"o\"  \"a\" string-replace! drop\n\
s2 \"oo\" \"a\" string-replace! drop\n\
s3 \"o\"  \"\"  string-replace! drop\n\
s1 => \"faa\"\n\
s2 => \"fa\"\n\
s3 => \"f\"\n\
Return changed STR with string FROM replaced by string TO.  \
If TO is the empty string, delete the FROM part from STR.\n\
See also string-replace."
	FTH str, from, to;

	FTH_STACK_CHECK(vm, 3, 1);
	to = fth_pop_ficl_cell(vm);
	from = fth_pop_ficl_cell(vm);
	str = fth_pop_ficl_cell(vm);
	ficlStackPushFTH(vm->dataStack, fth_string_replace(str, from, to));
}

FTH
fth_string_chomp(FTH fs)
{
	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	if (FTH_STRING_DATA(fs)[FTH_STRING_LENGTH(fs) - 1] == '\n')
		fth_string_pop(fs);
	return (fs);
}

static void
ficl_string_chomp(ficlVm *vm)
{
#define h_string_chomp "( str1 -- str2 )  remove CR\n\
\"foo\\n\" value s1\n\
\"bar\" value s2\n\
s1 string-chomp => \"foo\"\n\
s2 string-chomp => \"bar\"\n\
Return new string object with possible trailing CR removed.\n\
See also string-chomp!."
	FTH fs;

	FTH_STACK_CHECK(vm, 1, 1);
	fs = fth_string_copy(fth_pop_ficl_cell(vm));
	ficlStackPushFTH(vm->dataStack, fth_string_chomp(fs));
}

static void
ficl_string_chomp_bang(ficlVm *vm)
{
#define h_string_chomp_bang "( str -- str' )  remove CR\n\
\"foo\\n\" value s1\n\
\"bar\" value s2\n\
s1 string-chomp drop\n\
s2 string-chomp drop\n\
s1 => \"foo\"\n\
s2 => \"bar\"\n\
Return changed STR with possible trailing CR removed.\n\
See also string-chomp."
	FTH fs;

	FTH_STACK_CHECK(vm, 1, 1);
	fs = fth_pop_ficl_cell(vm);
	ficlStackPushFTH(vm->dataStack, fth_string_chomp(fs));
}

FTH
fth_string_format(FTH fs, FTH args)
{
#define h_string_format "( fmt args -- str )  return formatted string\n\
\"%04d %8.2f %b %X %o\"  #( 128 pi 255 255 255 ) string-format\n\
  => \"0128     3.14 11111111 FF 377\"\n\
FMT is a sprintf(3) format string and ARGS the needed arguments \
which may be an array, a single argument or #f.\n\
%n.mX     n: entire format length\n\
          m: precision (float)\n\
          X: one of the following flags\n\
aAeEfFgG  float\n\
bB        binary\n\
c         char\n\
d         decimal\n\
oO        octal\n\
p         object-inspect (like Ruby's %p)\n\
s         object->string (char *)\n\
S         object-dump    (like Emacs' %S)\n\
u         unsigned\n\
xX        hex\n\
See also fth-format."
	char *fmt;

	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	if (FTH_FALSE_P(args) || FTH_NIL_P(args))
		return (fs);
	else if (!FTH_ARRAY_P(args))
		args = fth_make_array_var(1, args);
	fmt = FTH_STRING_REF(fs);
	if (fmt == NULL)
		return (fth_make_empty_string());
	return (fth_string_vformat(fmt, args));
}

int
fth_string_eval(FTH fs)
{
	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	return (fth_evaluate(FTH_FICL_VM(), FTH_STRING_REF(fs)));
}

static void
ficl_string_eval(ficlVm *vm)
{
#define h_string_eval "( string -- ?? )  evaluate string\n\
  \"3 4 +\"     string-eval => 7\n\
7 \"3 4 + +\"   string-eval => 14\n\
7 \"3 4 + + .\" string-eval \\ prints 14\n\
Evaluate STRING; values already on stack can be accessed, \
resulting values remain on stack.\n\
See also string-eval-with-status."
	FTH_STACK_CHECK(vm, 1, 0);
	if (fth_evaluate(vm, pop_cstring(vm)) == FICL_VM_STATUS_USER_EXIT)
		fth_exit(EXIT_SUCCESS);
}

static void
ficl_string_eval_with_status(ficlVm *vm)
{
#define h_es "( string -- ?? eval-status )  eval\n\
  \"3 4 +\"     string-eval-with-status drop => 7\n\
7 \"3 4 + +\"   string-eval-with-status drop => 14\n\
7 \"3 4 + + .\" string-eval-with-status drop \\ prints 14\n\
Evaluate STRING and return EVAL-STATUS on top of stack; \
values already on stack can be accessed, resulting values remain on stack.  \
EVAL-STATUS can be one of the following constants:\n\
BREAK         Ficl Break\n\
ERROR_EXIT    Ficl Error Exit\n\
INNER_EXIT    Ficl Inner Exit\n\
OUT_OF_TEXT   Ficl Out of Text\n\
RESTART       Ficl Restart\n\
USER_EXIT     Ficl User Exit\n\
See also string-eval."
	ficlInteger i;

	FTH_STACK_CHECK(vm, 1, 1);
	i = (ficlInteger)fth_evaluate(vm, pop_cstring(vm));
	ficlStackPushInteger(vm->dataStack, i);
}

static void
ficl_string_to_forth_string(ficlVm *vm)
{
#define h_string_to_fstring "( str -- addr len )  return Forth string\n\
\"10 20 + .\" string>$ evaluate => 30\n\
\"hello\" string>$ type => hello\n\
Return string object STR converted to a Forth string with ADDR LEN.  \
Standard words like TYPE and EVALUATE require this kind of string.\n\
See also $>string."
	FTH_STACK_CHECK(vm, 1, 2);
	push_forth_string(vm, pop_cstring(vm));
}

static void
ficl_forth_string_to_string(ficlVm *vm)
{
#define h_fstring_to_string "( addr len -- str )  return string\n\
s\" 10 20 + .\" $>string string-eval => 30\n\
s\" hello\" $>string .string => hello\n\
Return Forth string ADDR LEN as string object.  \
Standard words like TYPE and EVALUATE require this kind of string.\n\
See also string>$."
	char *str;
	size_t len;

	FTH_STACK_CHECK(vm, 2, 1);
	len = (size_t)ficlStackPopUnsigned(vm->dataStack);
	str = ficlStackPopPointer(vm->dataStack);
	ficlStackPushFTH(vm->dataStack, fth_make_string_len(str, len));
}

static ficlWord *string_immutable_paren;
/*
 * Push a fresh copy of the saved string back on stack.  So the
 * original content of the string won't be changed or destroyed.
 */
static void
ficl_string_immutable_paren(ficlVm *vm)
{
	push_cstring(vm, pop_cstring(vm));
}

static void
ficl_make_string_im(ficlVm *vm)
{
#define h_make_string_im "( space<ccc>\" -- str )  string (parse word)\n\
$\" foo\" => \"foo\"\n\
Parse string CCC delimited by '\"' at compile time and \
at interpret time return parsed string.  \
Note the space after the initial $\".  \
It exist to satisfy fontifying in Emacs forth-mode."
	char *buf;
	FTH fs;

	buf = parse_input_buffer(vm, "\"");	/* must be freed */
	fs = fth_make_string(buf);
	FTH_FREE(buf);
	if (vm->state == FICL_VM_STATE_COMPILE) {
		ficlDictionary *dict;
		ficlUnsigned u;

		dict = ficlVmGetDictionary(vm);
		u = (ficlUnsigned)ficlInstructionLiteralParen;
		ficlDictionaryAppendUnsigned(dict, u);
		ficlDictionaryAppendFTH(dict, fs);
		ficlDictionaryAppendPointer(dict, string_immutable_paren);
		return;
	}
	ficlStackPushFTH(vm->dataStack, fs);
}

void
init_string_type(void)
{
	string_tag = make_object_type(FTH_STR_STRING, FTH_STRING_T);
	fth_set_object_inspect(string_tag, str_inspect);
	fth_set_object_to_string(string_tag, str_to_string);
	fth_set_object_dump(string_tag, str_dump);
	fth_set_object_to_array(string_tag, str_to_array);
	fth_set_object_copy(string_tag, str_to_string);
	fth_set_object_value_ref(string_tag, str_ref);
	fth_set_object_value_set(string_tag, str_set);
	fth_set_object_equal_p(string_tag, str_equal_p);
	fth_set_object_length(string_tag, str_length);
	fth_set_object_free(string_tag, str_free);
}

void
init_string(void)
{
	fth_set_object_apply(string_tag, (void *)str_ref, 1, 0, 0);
	/* struct members */
	init_str_length();
	init_str_buf_length();
	init_str_top();
	/* string */
	FTH_PRI1("string-length", ficl_string_length, h_string_length);
	FTH_PRI1("string?", ficl_string_p, h_string_p);
	FTH_PRI1("char?", ficl_char_p, h_char_p);
	FTH_PRI1("make-string", ficl_make_string, h_make_string);
	FTH_PRI1("string-concat", ficl_values_to_string, h_values_to_string);
	FTH_PRI1(">string", ficl_values_to_string, h_values_to_string);
	FTH_PRI1("\"\"", ficl_make_empty_string, h_empty_string);
	FTH_PRI1("$space", ficl_space_string, h_space_string);
	FTH_PRI1("$spaces", ficl_spaces_string, h_spaces_string);
	FTH_PRI1("$cr", ficl_cr_string, h_cr_string);
	FTH_PROC("fth-format", ficl_fth_format, 1, 1, 0, h_fth_format);
	FTH_VOID_PROC("fth-print", ficl_fth_print, 1, 1, 0, h_fth_print);
	FTH_VOID_PROC("fth-warning", ficl_fth_warning, 1, 1, 0, h_fth_warning);
	FTH_VOID_PROC("fth-error", ficl_fth_error, 1, 1, 0, h_fth_error);
	FTH_VOID_PROC("fth-die", ficl_fth_die, 1, 1, 0, h_fth_die);
	FTH_PRI1("warning", ficl_warning, h_warning);
	FTH_PRI1("warn", ficl_warning, h_warning);
	FTH_PRI1("error", ficl_error, h_error);
	FTH_PRI1("die", ficl_die, h_die);
	FTH_PRI1(".string", ficl_print_object, h_print_object);
	FTH_PRI1(".$", ficl_print_object, h_print_object);
	FTH_PRI1(".g", ficl_print_object, h_print_object);
	FTH_PRI1(".error", ficl_error_object, h_error_object);
	FTH_PRI1(".stdout", ficl_print_stdout, h_print_stdout);
	FTH_PRI1(".stderr", ficl_print_stderr, h_print_stderr);
	FTH_PRI1(".debug", ficl_print_debug, h_print_debug);
	FTH_PRI1("string-cmp", ficl_string_cmp, h_string_cmp);
	FTH_PRI1("string=", ficl_string_equal_p, h_string_equal_p);
	FTH_PRI1("string<>", ficl_string_not_equal_p, h_string_not_equal_p);
	FTH_PRI1("string<", ficl_string_less_p, h_string_less_p);
	FTH_PRI1("string>", ficl_string_greater_p, h_string_greater_p);
	FTH_PROC("string->array", fth_string_to_array, 1, 0, 0, h_str_to_ary);
	FTH_PROC("string-copy", fth_string_copy, 1, 0, 0, h_string_copy);
	FTH_PRI1("string-ref", ficl_string_ref, h_string_ref);
	FTH_PRI1("string-set!", ficl_string_set, h_string_set);
	FTH_PROC("string-push", fth_string_push, 2, 0, 0, h_string_push);
	FTH_PROC("<<", fth_string_push, 2, 0, 0, h_string_push);
	FTH_PROC("string-pop", fth_string_pop, 1, 0, 0, h_string_pop);
	FTH_PROC("string-unshift", fth_string_unshift, 2, 0, 0, h_str_unshift);
	FTH_PROC("string-shift", fth_string_shift, 1, 0, 0, h_string_shift);
	FTH_PROC("string-append", fth_string_append, 2, 0, 0, h_string_append);
	FTH_PROC("$+", fth_string_append, 2, 0, 0, h_string_append);
	FTH_PRI1("string-reverse", ficl_string_reverse, h_string_reverse);
	FTH_PRI1("string-reverse!", ficl_string_reverse_bang, h_str_rev_bang);
	FTH_PRI1("string-insert!", ficl_string_insert, h_string_insert);
	FTH_PRI1("string-delete!", ficl_string_delete, h_string_delete);
	FTH_PROC("string-fill", fth_string_fill, 2, 0, 0, h_string_fill);
	FTH_PROC("string-index", fth_string_index, 2, 0, 0, h_string_index);
	FTH_PRI1("string-member?", ficl_string_member_p, h_string_member_p);
	FTH_PROC("string-find", fth_string_find, 2, 0, 0, h_string_find);
	FTH_PROC("string-split", fth_string_split, 2, 0, 0, h_string_split);
	FTH_PRI1("string-substring", ficl_string_substring, h_string_substring);
	FTH_PRI1("string-upcase", ficl_string_upcase, h_string_upcase);
	FTH_PRI1("string-upcase!", ficl_string_upcase_bang, h_str_up_bang);
	FTH_PRI1("string-downcase", ficl_string_downcase, h_string_downcase);
	FTH_PRI1("string-downcase!", ficl_string_downcase_bang, h_str_dn_bang);
	FTH_PRI1("string-capitalize", ficl_string_capitalize, h_str_cap);
	FTH_PRI1("string-capitalize!", ficl_string_capitalize_bang, h_str_c_b);
	FTH_PRI1("string-replace", ficl_string_replace, h_string_replace);
	FTH_PRI1("string-replace!", ficl_string_replace_bang, h_str_rep_bang);
	FTH_PRI1("string-chomp", ficl_string_chomp, h_string_chomp);
	FTH_PRI1("string-chomp!", ficl_string_chomp_bang, h_string_chomp_bang);
	FTH_PROC("string-format", fth_string_format, 2, 0, 0, h_string_format);
	FTH_PROC("format", fth_string_format, 2, 0, 0, h_string_format);
	FTH_PRI1("string-eval", ficl_string_eval, h_string_eval);
	FTH_PRI1("string-eval-with-status", ficl_string_eval_with_status, h_es);
	FTH_PRI1("string>$", ficl_string_to_forth_string, h_string_to_fstring);
	FTH_PRI1("$>string", ficl_forth_string_to_string, h_fstring_to_string);
	FTH_PRIM_IM("$\"", ficl_make_string_im, h_make_string_im);
	fth_define_constant("INNER_EXIT", (FTH)FICL_VM_STATUS_INNER_EXIT, NULL);
	fth_define_constant("OUT_OF_TEXT", (FTH)FICL_VM_STATUS_OUT_OF_TEXT, "");
	fth_define_constant("RESTART", (FTH)FICL_VM_STATUS_RESTART, NULL);
	fth_define_constant("USER_EXIT", (FTH)FICL_VM_STATUS_USER_EXIT, NULL);
	fth_define_constant("ERROR_EXIT", (FTH)FICL_VM_STATUS_ERROR_EXIT, NULL);
	fth_define_constant("BREAK", (FTH)FICL_VM_STATUS_BREAK, NULL);
	string_immutable_paren = ficlDictionaryAppendPrimitive(FTH_FICL_DICT(),
	    "(string-immutable)", ficl_string_immutable_paren,
	    FICL_WORD_COMPILE_ONLY);
	FTH_ADD_FEATURE_AND_INFO(FTH_STR_STRING, h_list_of_string_functions);
}

/*
 * string.c ends here
 */
