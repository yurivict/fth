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
 * @(#)array.c	2.1 1/2/18
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "fth.h"
#include "utils.h"

/* === ARRAY === */

static FTH 	array_tag;
static FTH 	list_tag;
static FTH 	acell_tag;

typedef struct {
	int 		type;	/* array, list, assoc */
	ficlInteger 	length;	/* actual array length */
	ficlInteger 	buf_length;	/* entire buffer length */
	ficlInteger 	top;	/* begin of actual array in buffer */
	FTH            *data;	/* actual array */
	FTH            *buf;	/* entire array buffer */
} FArray;

#define MAKE_ARRAY_MEMBER(Type, Member)	MAKE_MEMBER(FArray, ary, Type, Member)

/*-
 * Build words for scrutinizing arrays:
 *
 * init_ary_length     => ary->length
 * init_ary_buf_length => ary->buf_length
 * init_ary_top        => ary->top
 */
MAKE_ARRAY_MEMBER(Integer, length)
MAKE_ARRAY_MEMBER(Integer, buf_length)
MAKE_ARRAY_MEMBER(Integer, top)

#define FTH_ARRAY_OBJECT(Obj)	FTH_INSTANCE_REF_GEN(Obj, FArray)
#define FTH_ARRAY_TYPE(Obj)	FTH_ARRAY_OBJECT(Obj)->type
#define FTH_ARRAY_LENGTH(Obj)	FTH_ARRAY_OBJECT(Obj)->length
#define FTH_ARRAY_BUF_LENGTH(Obj) FTH_ARRAY_OBJECT(Obj)->buf_length
#define FTH_ARRAY_TOP(Obj)	FTH_ARRAY_OBJECT(Obj)->top
#define FTH_ARRAY_DATA(Obj)	FTH_ARRAY_OBJECT(Obj)->data
#define FTH_ARRAY_BUF(Obj)	FTH_ARRAY_OBJECT(Obj)->buf

#define FTH_ARY_ARRAY		0x01
#define FTH_ARY_LIST		0x02
#define FTH_ARY_ASSOC		0x04

#define FTH_ARY_ARRAY_SET(Obj)	((Obj)->type |= FTH_ARY_ARRAY)
#define FTH_ARY_LIST_SET(Obj)	((Obj)->type |= FTH_ARY_LIST)
#define FTH_LIST_SET(Obj)	(FTH_ARRAY_TYPE(Obj) |= FTH_ARY_LIST)
#define FTH_ASSOC_SET(Obj)	(FTH_ARRAY_TYPE(Obj) |= FTH_ARY_ASSOC)
#define FTH_ARRAY_LIST_P(Obj)	(FTH_ARRAY_TYPE(Obj) & FTH_ARY_LIST)
#define FTH_ARRAY_ASSOC_P(Obj)	(FTH_ARRAY_TYPE(Obj) & FTH_ARY_ASSOC)

/*
 * Array
 */
static FTH 	ary_compact_each(FTH, FTH);
static FTH 	ary_copy(FTH);
static FTH 	ary_dump(FTH);
static FTH 	ary_dump_each(FTH, FTH);
static FTH 	ary_equal_p(FTH, FTH);
static void 	ary_free(FTH);
static FTH 	ary_inspect(FTH);
static FTH 	ary_inspect_each(FTH, FTH);
static FTH 	ary_length(FTH);
static void 	ary_mark(FTH);
static FTH 	ary_ref(FTH, FTH);
static FTH 	ary_set(FTH, FTH, FTH);
static FTH 	ary_to_array(FTH);
static FTH 	ary_to_string(FTH);
static FTH 	ary_uniq_each(FTH, FTH);
#if defined(HAVE_QSORT)
static int 	cmpit(const void *, const void *);
#endif
static void 	ficl_array_compact(ficlVm *);
static void 	ficl_array_copy(ficlVm *);
static void 	ficl_array_delete(ficlVm *);
static void 	ficl_array_equal_p(ficlVm *);
static void 	ficl_array_index(ficlVm *);
static void 	ficl_array_insert(ficlVm *);
static void 	ficl_array_insert_bang(ficlVm *);
static void 	ficl_array_length(ficlVm *);
static void 	ficl_array_member_p(ficlVm *);
static void 	ficl_array_p(ficlVm *);
static void 	ficl_array_ref(ficlVm *);
static void 	ficl_array_reject(ficlVm *);
static void 	ficl_array_reverse(ficlVm *);
static void 	ficl_array_set(ficlVm *);
static void 	ficl_array_sort(ficlVm *);
static void 	ficl_array_subarray(ficlVm *);
static void 	ficl_array_uniq(ficlVm *);
static void 	ficl_make_array(ficlVm *);
static void 	ficl_make_empty_array(ficlVm *);
static void 	ficl_print_array(ficlVm *);
static void 	ficl_values_to_array(ficlVm *);
static FArray  *make_array(ficlInteger);
static FTH 	make_array_instance(FArray *);

/*
 * Acell
 */
static FTH 	acl_dump(FTH);
static FTH 	acl_inspect(FTH);
static FTH 	acl_to_string(FTH);

/*
 * Assoc
 */
static FTH 	assoc_insert(FTH, FTH, FTH);
static void 	ficl_assoc_p(ficlVm *);
static void 	ficl_values_to_assoc(ficlVm *);
static ficlInteger assoc_index(FTH, FTH);

/*
 * List
 */
static void 	ficl_cons_p(ficlVm *);
static void 	ficl_last_pair(ficlVm *);
static void 	ficl_list_append(ficlVm *);
static void 	ficl_list_delete(ficlVm *);
static void 	ficl_list_delete_bang(ficlVm *);
static void 	ficl_list_equal_p(ficlVm *);
static void 	ficl_list_fill(ficlVm *);
static void 	ficl_list_head(ficlVm *);
static void 	ficl_list_index(ficlVm *);
static void 	ficl_list_insert(ficlVm *);
static void 	ficl_list_length(ficlVm *);
static void 	ficl_list_p(ficlVm *);
static void 	ficl_list_ref(ficlVm *);
static void 	ficl_list_set(ficlVm *);
static void 	ficl_list_slice(ficlVm *);
static void 	ficl_list_slice_bang(ficlVm *);
static void 	ficl_list_tail(ficlVm *);
static void 	ficl_make_list(ficlVm *);
static void 	ficl_nil_p(ficlVm *);
static void 	ficl_pair_p(ficlVm *);
static void 	ficl_print_list(ficlVm *);
static void 	ficl_set_car(ficlVm *);
static void 	ficl_set_cdr(ficlVm *);
static void 	ficl_values_to_list(ficlVm *);
static FTH 	ls_append_each(FTH, FTH);
static FTH 	ls_delete_each(FTH, FTH);
static FTH 	make_list_instance(FArray *);

/*
 * Alist
 */
static void 	ficl_values_to_alist(ficlVm *);

#define h_list_of_array_functions "\
*** ARRAY PRIMITIVES ***\n\
#()            	    ( -- ary )\n\
.array         	    ( ary -- )\n\
>array              ( vals len -- ary )\n\
array->array   	    ( ary1 -- ary2 )\n\
array->list   	    ( ary -- lst )\n\
array-append   	    ( ary1 ary2 -- ary1+ary2 )\n\
array-clear    	    ( ary -- )\n\
array-compact       ( ary1 prc args -- ary2 )\n\
array-compact!      ( ary prc args -- ary' )\n\
array-concat alias for >array\n\
array-copy     	    ( ary1 -- ary2 )\n\
array-delete!  	    ( ary idx -- val )\n\
array-delete-key    ( ary idx -- val )\n\
array-fill     	    ( ary val -- )\n\
array-find          ( ary key -- key )\n\
array-index    	    ( ary key -- idx )\n\
array-insert  	    ( ary1 idx val -- ary2 )\n\
array-insert!  	    ( ary idx val -- ary' )\n\
array-join     	    ( ary sep -- str )\n\
array-length   	    ( ary -- len )\n\
array-member?  	    ( ary key -- f )\n\
array-pop      	    ( ary -- val )\n\
array-push     	    ( ary val -- ary' )\n\
array-ref      	    ( ary idx -- val )\n\
array-reject        ( ary1 prc args -- ary2 )\n\
array-reject!       ( ary prc args -- ary' )\n\
array-reverse  	    ( ary1 -- ary2 )\n\
array-reverse! 	    ( ary -- ary' )\n\
array-set!     	    ( ary idx val -- )\n\
array-shift    	    ( ary -- val )\n\
array-sort     	    ( ary1 cmp-xt -- ary2 )\n\
array-sort!    	    ( ary cmp-xt -- ary' )\n\
array-subarray 	    ( ary start end -- subary )\n\
array-uniq     	    ( ary1 -- ary2 )\n\
array-uniq!    	    ( ary -- ary' )\n\
array-unshift  	    ( ary val -- ary' )\n\
array=         	    ( ary1 ary2 -- f )\n\
array?         	    ( obj -- f )\n\
make-array     	    ( len :key initial-element -- ary )\n\
Assoc arrays:\n\
>assoc              ( vals len -- ary )\n\
array-assoc         ( ary key -- ret )\n\
array-assoc-ref     ( ary key -- val )\n\
array-assoc-remove! ( ary key -- 'ary )\n\
array-assoc-set!    ( ary key val -- 'ary )\n\
assoc               ( ary key val -- 'ary )\n\
assoc?              ( obj -- f )"

FTH
fth_array_each(FTH array, FTH (*fnc) (FTH value, FTH data), FTH data)
{
	ficlInteger 	i;

	FTH_ASSERT_ARGS(FTH_ARRAY_P(array), array, FTH_ARG1, "an array");

	for (i = 0; i < FTH_ARRAY_LENGTH(array); i++)
		data = (*fnc) (FTH_ARRAY_DATA(array)[i], data);

	return (data);
}

FTH
fth_array_each_with_index(FTH array, FTH (*fnc) (FTH value, FTH data, ficlInteger idx), FTH data)
{
	ficlInteger 	i;

	FTH_ASSERT_ARGS(FTH_ARRAY_P(array), array, FTH_ARG1, "an array");

	for (i = 0; i < FTH_ARRAY_LENGTH(array); i++)
		data = (*fnc) (FTH_ARRAY_DATA(array)[i], data, i);

	return (data);
}

/*
 * Create new array filled with values from FNC.
 */
FTH
fth_array_map (FTH array, FTH (*f) (FTH value, FTH data), FTH data)
{
	ficlInteger 	i, len;
	FTH 		ary;

	FTH_ASSERT_ARGS(FTH_ARRAY_P(array), array, FTH_ARG1, "an array");
	len = FTH_ARRAY_LENGTH(array);
	ary = fth_make_array_len(len);

	for (i = 0; i < len; i++)
		FTH_ARRAY_DATA(ary)[i] = (*f) (FTH_ARRAY_DATA(array)[i], data);

	return (ary);
}

static FTH
ary_inspect_each(FTH value, FTH data)
{
	return (fth_string_sformat(data, "  %I", value));
}

static FTH
ary_inspect(FTH self)
{
	char           *name;
	ficlInteger 	len;
	FTH 		fs;

	name = FTH_INSTANCE_NAME(self);
	len = FTH_ARRAY_LENGTH(self);

	if (len == 0)
		return (fth_make_string_format("%s empty", name));

	fs = fth_make_string_format("%s[%ld]:", name, len);
	return (fth_array_each(self, ary_inspect_each, fs));
}

static FTH
ary_to_string(FTH self)
{
	ficlInteger 	i, len;
	FTH 		fs;

	len = FTH_ARRAY_LENGTH(self);

	/* Negative fth_print_length shows all entries! */
	if (fth_print_length >= 0 && len > fth_print_length)
		len = FICL_MIN(len, fth_print_length);

	fs = fth_make_string_format("%c%s(",
	    FTH_ARRAY_LIST_P(self) ? '\'' : '#',
	    FTH_ARRAY_ASSOC_P(self) ? "a" : "");

	if (len > 0) {
		for (i = 0; i < len; i++)
			fth_string_sformat(fs, " %M", FTH_ARRAY_DATA(self)[i]);

		if (len < FTH_ARRAY_LENGTH(self))
			fth_string_sformat(fs, " ...");

		fth_string_sformat(fs, " ");
	}
	return (fth_string_sformat(fs, ")"));
}

static FTH
ary_dump_each(FTH value, FTH data)
{
	return (fth_string_sformat(data, " %D ", value));
}

static FTH
ary_dump(FTH self)
{
	FTH 		fs;

	fs = fth_make_string_format("%c%s(",
	    FTH_ARRAY_LIST_P(self) ? '\'' : '#',
	    FTH_ARRAY_ASSOC_P(self) ? "a" : "");

	if (FTH_ARRAY_LENGTH(self) > 0)
		fth_array_each(self, ary_dump_each, fs);

	return (fth_string_sformat(fs, ")"));
}

static FTH
ary_to_array(FTH self)
{
	ficlInteger 	len;
	size_t 		size;
	FTH 		new;

	len = FTH_ARRAY_LENGTH(self);
	new = fth_make_array_len(len);
	size = sizeof(FTH) * (size_t) len;
	memmove(FTH_ARRAY_DATA(new), FTH_ARRAY_DATA(self), size);
	return (new);
}

static FTH
ary_copy(FTH self)
{
	ficlInteger 	i;
	FTH 		new, el;

	new = fth_make_array_len(FTH_ARRAY_LENGTH(self));

	for (i = 0; i < FTH_ARRAY_LENGTH(self); i++) {
		el = fth_object_copy(FTH_ARRAY_DATA(self)[i]);
		FTH_ARRAY_DATA(new)[i] = el;
	}

	return (new);
}

static FTH
ary_ref(FTH self, FTH fidx)
{
	ficlInteger 	idx, len;

	idx = FTH_INT_REF(fidx);
	len = FTH_ARRAY_LENGTH(self);

	if (idx < 0 || idx >= len)
		FTH_OUT_OF_BOUNDS(FTH_ARG2, idx);

	return (FTH_ARRAY_DATA(self)[idx]);
}

static FTH
ary_set(FTH self, FTH fidx, FTH value)
{
	ficlInteger 	idx, len;

	idx = FTH_INT_REF(fidx);
	len = FTH_ARRAY_LENGTH(self);

	if (idx < 0 || idx >= len)
		FTH_OUT_OF_BOUNDS(FTH_ARG2, idx);

	FTH_INSTANCE_CHANGED(self);
	return (FTH_ARRAY_DATA(self)[idx] = value);
}

static FTH
ary_equal_p(FTH self, FTH obj)
{
	ficlInteger 	i;
	FTH 		a, b;

	if (self == obj)
		return (FTH_TRUE);

	if (FTH_ARRAY_LENGTH(self) == FTH_ARRAY_LENGTH(obj)) {
		for (i = 0; i < FTH_ARRAY_LENGTH(self); i++) {
			a = FTH_ARRAY_DATA(self)[i];
			b = FTH_ARRAY_DATA(obj)[i];

			if (!fth_object_equal_p(a, b))
				return (FTH_FALSE);
		}

		return (FTH_TRUE);
	}
	return (FTH_FALSE);
}

static FTH
ary_length(FTH self)
{
	return (fth_make_int(FTH_ARRAY_LENGTH(self)));
}

static void
ary_mark(FTH self)
{
	ficlInteger 	i;

	for (i = 0; i < FTH_ARRAY_LENGTH(self); i++)
		fth_gc_mark(FTH_ARRAY_DATA(self)[i]);
}

static void
ary_free(FTH self)
{
	FTH_FREE(FTH_ARRAY_BUF(self));
	FTH_FREE(FTH_ARRAY_OBJECT(self));
}

/*
 * If OBJ is an Array object, return its length, otherwise -1.
 */
ficlInteger
fth_array_length(FTH obj)
{
	if (FTH_ARRAY_P(obj))
		return (FTH_ARRAY_LENGTH(obj));
	return (-1);
}

static void
ficl_array_length(ficlVm *vm)
{
#define h_array_length "( obj -- len )  return array length\n\
#( 0 1 2 ) array-length => 3\n\
5          array-length => -1\n\
If OBJ is an array object, return its length, otherwise -1."
	ficlInteger 	len;

	FTH_STACK_CHECK(vm, 1, 1);
	len = fth_array_length(fth_pop_ficl_cell(vm));
	ficlStackPushInteger(vm->dataStack, len);
}

/*
 * |---------buf_length (buf)-----|
 * |---top----|-length (data)-|
 * v          v               v
 * |----------****************----|
 *
 * buf + top --> start of data
 */
static FArray  *
make_array(ficlInteger len)
{
	FArray         *ary;
	ficlInteger 	buf_len, top_len;

	if (len < 0)
		FTH_OUT_OF_BOUNDS_ERROR(FTH_ARG1, len, "negative");

	top_len = NEW_SEQ_LENGTH(len) / 3;
	buf_len = NEW_SEQ_LENGTH(len + top_len);

	if (buf_len > MAX_SEQ_LENGTH)
		FTH_OUT_OF_BOUNDS_ERROR(FTH_ARG1, len, "too long");

	ary = FTH_MALLOC(sizeof(FArray));
	ary->type = 0;
	ary->length = len;
	ary->buf_length = buf_len;
	ary->top = top_len;
	ary->buf = FTH_CALLOC(ary->buf_length, sizeof(FTH));
	ary->data = ary->buf + ary->top;
	return (ary);
}

static FTH
make_array_instance(FArray *ary)
{
	FTH_ARY_ARRAY_SET(ary);
	return (fth_make_instance(array_tag, ary));
}

FTH
fth_make_array_var(int len,...)
{
	int 		i;
	FArray         *ary;
	va_list 	list;

	ary = make_array((ficlInteger) len);
	va_start(list, len);

	for (i = 0; i < len; i++)
		ary->data[i] = va_arg(list, FTH);

	va_end(list);
	return (make_array_instance(ary));
}

FTH
fth_make_array_len(ficlInteger len)
{
	return (make_array_instance(make_array(len)));
}

/*
 * Return Array with LEN entries each initialized to INIT.
 */
FTH
fth_make_array_with_init(ficlInteger len, FTH init)
{
	ficlInteger 	i;
	FArray         *ary;

	ary = make_array(len);

	for (i = 0; i < ary->length; i++)
		ary->data[i] = init;

	return (make_array_instance(ary));
}

FTH
fth_make_empty_array(void)
{
	return (fth_make_array_len(0L));
}

static void
ficl_array_p(ficlVm *vm)
{
#define h_array_p "( obj -- f )  test if OBJ is an array\n\
#( 0 1 2 ) array? => #t\n\
nil        array? => #f\n\
Return #t if OBJ is an array object, otherwise #f."
	FTH 		obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, FTH_ARRAY_P(obj));
}

static void
ficl_make_array(ficlVm *vm)
{
#define h_make_array "( len :key initial-element nil -- ary )  return array\n\
0                     make-array => #()\n\
3                     make-array => #( nil nil nil )\n\
3 :initial-element 10 make-array => #( 10 10 10 )\n\
Return array of length LEN filled with keyword INITIAL-ELEMENT values.  \
INITIAL-ELEMENT defaults to nil if not specified.  \
Raise OUT-OF-RANGE exception if LEN < 0."
	FTH 		size, init, ary;

	init = fth_get_optkey(FTH_KEYWORD_INIT, FTH_NIL);
	FTH_STACK_CHECK(vm, 1, 1);
	size = fth_pop_ficl_cell(vm);
	FTH_ASSERT_ARGS(FTH_INTEGER_P(size), size, FTH_ARG1, "an integer");
	ary = fth_make_array_with_init(FTH_INT_REF(size), init);
	ficlStackPushFTH(vm->dataStack, ary);
}

static void
ficl_values_to_array(ficlVm *vm)
{
#define h_values_to_array "( vals len -- ary )  return array\n\
0 1 2   3 >array => #( 0 1 2 )\n\
Return array object with LEN objects found on parameter stack.  \
Raise OUT-OF-RANGE exception if LEN < 0."
	ficlInteger 	i, len;
	FTH 		array;

	FTH_STACK_CHECK(vm, 1, 0);
	len = ficlStackPopInteger(vm->dataStack);

	if (len < 0)
		FTH_OUT_OF_BOUNDS_ERROR(FTH_ARG1, len, "too long");

	if (len > MAX_SEQ_LENGTH)
		FTH_OUT_OF_BOUNDS_ERROR(FTH_ARG1, len, "too long");

	FTH_STACK_CHECK(vm, len, 1);
	array = fth_make_array_len(len);

	for (i = len - 1; i >= 0; i--)
		FTH_ARRAY_DATA(array)[i] = fth_pop_ficl_cell(vm);

	ficlStackPushFTH(vm->dataStack, array);
}

static void
ficl_make_empty_array(ficlVm *vm)
{
#define h_empty_array "( -- empty-ary )  return empty array\n\
#() value ary\n\
ary 0 array-push => #( 0 )\n\
ary 1 array-push => #( 0 1 )\n\
ary 2 array-push => #( 0 1 2 )\n\
ary              => #( 0 1 2 )\n\
Return array of length 0 for array-append, array-push etc."
	FTH_STACK_CHECK(vm, 0, 1);
	ficlStackPushFTH(vm->dataStack, fth_make_empty_array());
}

static void
ficl_print_array(ficlVm *vm)
{
#define h_print_array "( ary -- )  print array\n\
#( 0 1 2 ) .array => #( 0 1 2 )\n\
Print array object ARY to current output."
	FTH 		obj;

	FTH_STACK_CHECK(vm, 1, 0);
	obj = fth_pop_ficl_cell(vm);
	FTH_ASSERT_ARGS(FTH_ARRAY_P(obj), obj, FTH_ARG1, "an array");
	fth_print(fth_string_ref(ary_to_string(obj)));
}

int
fth_array_equal_p(FTH obj1, FTH obj2)
{
	if (FTH_ARRAY_P(obj1) && FTH_ARRAY_P(obj2))
		return (FTH_TO_BOOL(ary_equal_p(obj1, obj2)));
	return (0);
}

static void
ficl_array_equal_p(ficlVm *vm)
{
#define h_array_equal_p "( ary1 ary2 -- f )  compare arrays\n\
#( 0 1 2 ) value a1\n\
#( 0 1 2 ) value a2\n\
#( 0 1 3 ) value a3\n\
a1 a1 array= #t\n\
a1 a2 array= #t\n\
a1 a3 array= #f\n\
Return #t if ARY1 and ARY2 are array objects of same length and content, \
otherwise #f."
	FTH 		obj1, obj2;

	FTH_STACK_CHECK(vm, 2, 1);
	obj2 = fth_pop_ficl_cell(vm);
	obj1 = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, fth_array_equal_p(obj1, obj2));
}

FTH
fth_array_to_array(FTH array)
{
#define h_array_to_array "( ary1 -- ary2 )  return array\n\
#( 0 #{ 'foo 10 } 2 ) value ary1\n\
ary1 array->array value ary2\n\
ary1 1 array-ref 'foo 30 hash-set!\n\
ary1 => #( 0 #{ 'foo 30 } 2 )\n\
ary2 => #( 0 #{ 'foo 30 } 2 )\n\
Return copy of ARY1 only with references of each element \
in contrary to array-copy.  \
If ARY1 is not an array, return #( ary1 ).\n\
See also array-copy."
	if (FTH_ARRAY_P(array))
		return (ary_to_array(array));
	return (fth_make_array_var(1, array));
}

/*
 * Return OBJ as List object.
 */
FTH
fth_array_to_list(FTH obj)
{
#define h_array_to_list "( ary -- lst )  return list\n\
#( 0 #{ 'foo 10 } 2 ) value ary1\n\
ary1 array->list value lst1\n\
ary1 1 array-ref 'foo 30 hash-set!\n\
ary1 => #( 0 #{ 'foo 30 } 2 )\n\
lst1 => '( 0 #{ 'foo 30 } 2 )\n\
Return copy of ARY as list only with references of each element \
in contrary to array-copy.  \
If ARY is not an array, return '( ary ).\n\
See also array-copy."
	FTH 		ls;
	ficlInteger 	i;

	if (FTH_ARRAY_P(obj)) {
		ls = fth_make_list_len(FTH_ARRAY_LENGTH(obj));

		for (i = 0; i < FTH_ARRAY_LENGTH(obj); i++)
			FTH_ARRAY_DATA(ls)[i] = FTH_ARRAY_DATA(obj)[i];
	} else
		ls = fth_make_list_var(1, obj);

	return (ls);
}

FTH
fth_array_copy(FTH array)
{
	FTH_ASSERT_ARGS(FTH_ARRAY_P(array), array, FTH_ARG1, "an array");
	return (ary_copy(array));
}

static void
ficl_array_copy(ficlVm *vm)
{
#define h_array_copy "( ary1 -- ary2 )  duplicate array\n\
#( 0 #{ 'foo 10 } 2 ) value ary1\n\
ary1 array-copy value ary2\n\
ary1 1 array-ref 'foo 30 hash-set!\n\
ary1 => #( 0 #{ 'foo 30 } 2 )\n\
ary2 => #( 0 #{ 'foo 10 } 2 )\n\
Return copy of ARY1 with all elements new created \
in contrary to array->array.\n\
See also array->array."
	FTH 		obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushFTH(vm->dataStack, fth_array_copy(obj));
}

/*
 * Return element at position IDX.  Negative IDX counts from backward.
 * Raise OUT_OF_RANGE exception if IDX is not in ARRAY's range.
 */
FTH
fth_array_ref(FTH array, ficlInteger idx)
{
	FTH_ASSERT_ARGS(FTH_ARRAY_P(array), array, FTH_ARG1, "an array");

	if (idx < 0)
		idx += FTH_ARRAY_LENGTH(array);

	return (ary_ref(array, fth_make_int(idx)));
}

/* Dangerous! No check for array or array bounds. */
FTH
fth_array_fast_ref(FTH array, ficlInteger idx)
{
	return (FTH_ARRAY_DATA(array)[idx]);
}

static void
ficl_array_ref(ficlVm *vm)
{
#define h_array_ref "( ary idx -- val )  return value at IDX\n\
#( 'a 'b 'c ) 1 array-ref => 'b\n\
Return element at position IDX.  \
Negative index counts from backward.  \
Raise OUT-OF-RANGE exception if IDX is not in ARY's range."
	ficlInteger 	idx;
	FTH 		ary;

	FTH_STACK_CHECK(vm, 2, 1);
	idx = ficlStackPopInteger(vm->dataStack);
	ary = fth_pop_ficl_cell(vm);
	fth_push_ficl_cell(vm, fth_array_ref(ary, idx));
}

/*
 * Store VALUE at position IDX and return VALUE.  Negative index counts
 * from backward.  Raise OUT_OF_RANGE exception if IDX is not in ARRAY's
 * range.
 */
FTH
fth_array_set(FTH array, ficlInteger idx, FTH value)
{
	FTH_ASSERT_ARGS(FTH_ARRAY_P(array), array, FTH_ARG1, "an array");

	if (idx < 0)
		idx += FTH_ARRAY_LENGTH(array);

	return (ary_set(array, fth_make_int(idx), value));
}

/* Dangerous! No check for array or array bounds. */
FTH
fth_array_fast_set(FTH array, ficlInteger idx, FTH value)
{
	FTH_INSTANCE_CHANGED(array);
	return (FTH_ARRAY_DATA(array)[idx] = value);
}

static void
ficl_array_set(ficlVm *vm)
{
#define h_array_set "( ary idx val -- )  set value at IDX\n\
#( 'a 'b 'c ) value ary\n\
ary 1 'e array-set!\n\
ary => #( 'a 'e 'c )\n\
Store VAL at position IDX.  \
Negative index counts from backward.  \
Raise OUT-OF-RANGE exception if IDX is not in ARY's range."
	ficlInteger 	idx;
	FTH 		value;

	FTH_STACK_CHECK(vm, 3, 0);
	value = fth_pop_ficl_cell(vm);
	idx = ficlStackPopInteger(vm->dataStack);
	fth_array_set(fth_pop_ficl_cell(vm), idx, value);
}

FTH
fth_array_push(FTH array, FTH value)
{
#define h_array_push "( ary val -- ary' )  append VAL\n\
#( 0 1 2 ) 10 array-push => #( 0 1 2 10 )\n\
Append VAL to ARY.\n\
See also array-pop, array-unshift, array-shift."
	ficlInteger 	new_buf_len, lenp1;

	FTH_ASSERT_ARGS(FTH_ARRAY_P(array), array, FTH_ARG1, "an array");
	/* HINT: top + length + 1 is the new length (thanks Bill!) */
	lenp1 = FTH_ARRAY_LENGTH(array) + 1;
	new_buf_len = FTH_ARRAY_TOP(array) + lenp1;

	if (new_buf_len > FTH_ARRAY_BUF_LENGTH(array)) {
		ficlInteger 	len;
		size_t 		size;

		len = NEW_SEQ_LENGTH(new_buf_len);

		if (len > MAX_SEQ_LENGTH)
			FTH_OUT_OF_BOUNDS_ERROR(FTH_ARG1, len, "too long");

		FTH_ARRAY_BUF_LENGTH(array) = len;
		size = sizeof(FTH) * (size_t) len;
		FTH_ARRAY_BUF(array) = FTH_REALLOC(FTH_ARRAY_BUF(array), size);
		FTH_ARRAY_DATA(array) = FTH_ARRAY_BUF(array) +
		    FTH_ARRAY_TOP(array);
	}
	FTH_ARRAY_DATA(array)[FTH_ARRAY_LENGTH(array)] = value;
	FTH_ARRAY_LENGTH(array)++;
	FTH_INSTANCE_CHANGED(array);
	return (array);
}

FTH
fth_array_pop(FTH array)
{
#define h_array_pop "( ary -- val )  remove last entry\n\
#( 0 1 2 ) value ary\n\
ary array-pop => 2\n\
ary array-pop => 1\n\
ary array-pop => 0\n\
ary array-pop => #f\n\
Remove and return last element from ARY.  \
If ARY is empty, return #f.\n\
See also array-push, array-unshift, array-shift."
	FTH 		result;
	ficlInteger 	len;

	FTH_ASSERT_ARGS(FTH_ARRAY_P(array), array, FTH_ARG1, "an array");

	if (FTH_ARRAY_LENGTH(array) == 0)
		return (FTH_FALSE);

	FTH_ARRAY_LENGTH(array)--;
	result = FTH_ARRAY_DATA(array)[FTH_ARRAY_LENGTH(array)];
	len = NEW_SEQ_LENGTH(FTH_ARRAY_TOP(array) + FTH_ARRAY_LENGTH(array));

	if (len < FTH_ARRAY_BUF_LENGTH(array)) {
		size_t 		size;

		FTH_ARRAY_BUF_LENGTH(array) = len;
		size = sizeof(FTH) * (size_t) len;
		FTH_ARRAY_BUF(array) = FTH_REALLOC(FTH_ARRAY_BUF(array), size);
		FTH_ARRAY_DATA(array) = FTH_ARRAY_BUF(array) +
		    FTH_ARRAY_TOP(array);
	}
	FTH_INSTANCE_CHANGED(array);
	return (result);
}

FTH
fth_array_unshift(FTH array, FTH value)
{
#define h_array_unshift "( ary val -- ary' )  prepend VAL\n\
#( 0 1 2 ) value ary\n\
ary 10 array-unshift drop\n\
ary 20 array-unshift drop\n\
ary => #( 20 10 0 1 2 )\n\
Prepend VAL to ARY.\n\
See also array-push, array-pop, array-shift."
	ficlInteger 	len, new_top, new_len, new_buf_len;
	size_t 		size;

	FTH_ASSERT_ARGS(FTH_ARRAY_P(array), array, FTH_ARG1, "an array");
	new_top = FTH_ARRAY_TOP(array) - 1;
	new_len = FTH_ARRAY_LENGTH(array) + 1;
	new_buf_len = new_top + new_len;

	if (new_top < 1) {
		new_top = FTH_ARRAY_BUF_LENGTH(array) / 3;
		new_buf_len = new_top + new_len;

		if (new_buf_len > FTH_ARRAY_BUF_LENGTH(array)) {
			len = NEW_SEQ_LENGTH(new_buf_len);

			if (len > MAX_SEQ_LENGTH)
				FTH_OUT_OF_BOUNDS_ERROR(FTH_ARG1, len,
				    "too long");

			FTH_ARRAY_BUF_LENGTH(array) = len;
			size = sizeof(FTH) * (size_t) len;
			FTH_ARRAY_BUF(array) = FTH_REALLOC(FTH_ARRAY_BUF(array),
			    size);
			FTH_ARRAY_DATA(array) = FTH_ARRAY_BUF(array) +
			    FTH_ARRAY_TOP(array);
		}
		memmove(FTH_ARRAY_BUF(array) + new_top + 1,
		    FTH_ARRAY_DATA(array),
		    sizeof(FTH) * (size_t) FTH_ARRAY_LENGTH(array));
	} else if (new_buf_len > FTH_ARRAY_BUF_LENGTH(array)) {
		len = NEW_SEQ_LENGTH(new_buf_len);

		if (len > MAX_SEQ_LENGTH)
			FTH_OUT_OF_BOUNDS_ERROR(FTH_ARG1, len, "too long");

		FTH_ARRAY_BUF_LENGTH(array) = len;
		size = sizeof(FTH) * (size_t) len;
		FTH_ARRAY_BUF(array) = FTH_REALLOC(FTH_ARRAY_BUF(array), size);
		FTH_ARRAY_DATA(array) = FTH_ARRAY_BUF(array) +
		    FTH_ARRAY_TOP(array);
	}
	FTH_ARRAY_TOP(array) = new_top;
	FTH_ARRAY_LENGTH(array) = new_len;
	FTH_ARRAY_DATA(array) = FTH_ARRAY_BUF(array) + FTH_ARRAY_TOP(array);
	FTH_ARRAY_DATA(array)[0] = value;
	FTH_INSTANCE_CHANGED(array);
	return (array);
}

FTH
fth_array_shift(FTH array)
{
#define h_array_shift "( ary -- val )  remove first element\n\
#( 0 1 2 ) value ary\n\
ary array-shift => 0\n\
ary array-shift => 1\n\
ary array-shift => 2\n\
ary array-shift => #f\n\
Remove and return first element from ARY.  \
If ARY is empty, return #f.\n\
See also array-push, array-pop, array-unshift."
	FTH 		result;
	ficlInteger 	len;

	FTH_ASSERT_ARGS(FTH_ARRAY_P(array), array, FTH_ARG1, "an array");

	if (FTH_ARRAY_LENGTH(array) == 0)
		return (FTH_FALSE);

	result = FTH_ARRAY_DATA(array)[0];

	if ((FTH_ARRAY_TOP(array) + 1) > (FTH_ARRAY_BUF_LENGTH(array) / 2)) {
		FTH_ARRAY_TOP(array) = FTH_ARRAY_BUF_LENGTH(array) / 3;
		memmove(FTH_ARRAY_BUF(array) + FTH_ARRAY_TOP(array),
		    FTH_ARRAY_DATA(array),
		    sizeof(FTH) * (size_t) FTH_ARRAY_LENGTH(array));
	}
	FTH_ARRAY_LENGTH(array)--;
	len = NEW_SEQ_LENGTH(FTH_ARRAY_TOP(array) + FTH_ARRAY_LENGTH(array));
	FTH_ARRAY_TOP(array)++;

	if (len < FTH_ARRAY_BUF_LENGTH(array)) {
		size_t 		size;

		FTH_ARRAY_BUF_LENGTH(array) = len;
		size = sizeof(FTH) * (size_t) len;
		FTH_ARRAY_BUF(array) = FTH_REALLOC(FTH_ARRAY_BUF(array), size);
	}
	FTH_ARRAY_DATA(array) = FTH_ARRAY_BUF(array) + FTH_ARRAY_TOP(array);
	FTH_INSTANCE_CHANGED(array);
	return (result);
}

FTH
fth_array_append(FTH array, FTH value)
{
#define h_array_append "( ary1 ary2 -- ary3 )  append ARY2 to ARY1\n\
#( 0 1 2 ) value ary1\n\
#( 3 4 5 ) value ary2\n\
ary1 ary2 array-append value ary3\n\
ary1 => #( 0 1 2 )\n\
ary2 => #( 3 4 5 )\n\
ary3 => #( 0 1 2 3 4 5 )\n\
ary1 10 array-append => #( 0 1 2 10 )\n\
Append two arrays and return new one.  \
If ARY2 is not an array, append it as a single element.\n\
See also array-concat (alias >array) and array-push."
	ficlInteger 	i, j, alen, vlen;
	FTH 		result;

	FTH_ASSERT_ARGS(FTH_ARRAY_P(array), array, FTH_ARG1, "an array");
	alen = FTH_ARRAY_LENGTH(array);

	if (!FTH_ARRAY_P(value)) {
		result = fth_make_array_len(alen + 1);

		for (i = 0; i < alen; i++)
			FTH_ARRAY_DATA(result)[i] = FTH_ARRAY_DATA(array)[i];

		FTH_ARRAY_DATA(result)[i] = value;
		return (result);
	}
	vlen = FTH_ARRAY_LENGTH(value);
	result = fth_make_array_len(alen + vlen);

	for (i = 0; i < alen; i++)
		FTH_ARRAY_DATA(result)[i] = FTH_ARRAY_DATA(array)[i];

	if (vlen > 0)
		for (i = alen, j = 0; j < vlen; i++, j++)
			FTH_ARRAY_DATA(result)[i] = FTH_ARRAY_DATA(value)[j];

	return (result);
}

FTH
fth_array_reverse(FTH array)
{
#define h_ary_rev_bang "( ary -- ary' )  reverse array elements\n\
#( 0 1 2 ) value ary\n\
ary array-reverse! drop\n\
ary => #( 2 1 0 )\n\
Return ARY in reversed order.\n\
See also array-reverse."
	ficlInteger 	i, j, len;
	FTH 		tmp;

	FTH_ASSERT_ARGS(FTH_ARRAY_P(array), array, FTH_ARG1, "an array");

	if (FTH_ARRAY_LENGTH(array) == 0)
		return (array);

	tmp = ary_copy(array);
	len = FTH_ARRAY_LENGTH(array);

	for (i = 0, j = len - 1; i < len; i++, j--)
		FTH_ARRAY_DATA(array)[i] = FTH_ARRAY_DATA(tmp)[j];

	return (array);
}

static void
ficl_array_reverse(ficlVm *vm)
{
#define h_array_reverse "( ary1 -- ary2 )  reverse array elements\n\
#( 0 1 2 ) value ary1\n\
ary1 array-reverse value ary2\n\
ary1 => #( 0 1 2 )\n\
ary2 => #( 2 1 0 )\n\
Return new array with reversed order of ARY1.\n\
See also array-reverse!."
	FTH 		ary;

	FTH_STACK_CHECK(vm, 1, 1);
	ary = fth_array_copy(fth_pop_ficl_cell(vm));
	ficlStackPushFTH(vm->dataStack, fth_array_reverse(ary));
}

FTH
fth_array_insert(FTH array, ficlInteger idx, FTH value)
{
	ficlInteger 	ary_len, ins_len, res_len, new_buf_len;
	FTH 		ins;

	FTH_ASSERT_ARGS(FTH_ARRAY_P(array), array, FTH_ARG1, "an array");
	ary_len = FTH_ARRAY_LENGTH(array);

	if (idx < 0)
		idx += ary_len;

	if (idx == 0) {
		ficlInteger 	i;

		if (!FTH_ARRAY_P(value))
			return (fth_array_unshift(array, value));

		for (i = FTH_ARRAY_LENGTH(value) - 1; i >= 0; i--)
			fth_array_unshift(array, FTH_ARRAY_DATA(value)[i]);

		return (array);
	}
	if (idx < 0 || idx >= ary_len)
		FTH_OUT_OF_BOUNDS(FTH_ARG2, idx);

	if (FTH_ARRAY_P(value))
		ins = value;
	else
		ins = fth_make_array_var(1, value);

	ins_len = FTH_ARRAY_LENGTH(ins);
	res_len = ary_len + ins_len;
	new_buf_len = FTH_ARRAY_TOP(array) + res_len;

	if (new_buf_len > FTH_ARRAY_BUF_LENGTH(array)) {
		ficlInteger 	len;
		size_t 		size;

		len = NEW_SEQ_LENGTH(new_buf_len);

		if (len > MAX_SEQ_LENGTH)
			FTH_OUT_OF_BOUNDS_ERROR(FTH_ARG1, len, "too long");

		FTH_ARRAY_BUF_LENGTH(array) = len;
		size = sizeof(FTH) * (size_t) len;
		FTH_ARRAY_BUF(array) = FTH_REALLOC(FTH_ARRAY_BUF(array), size);
		FTH_ARRAY_DATA(array) = FTH_ARRAY_BUF(array) +
		    FTH_ARRAY_TOP(array);
	}
	memmove(FTH_ARRAY_DATA(array) + idx + ins_len,
	    FTH_ARRAY_DATA(array) + idx,
	    sizeof(FTH) * (size_t) (ary_len - idx));
	memmove(FTH_ARRAY_DATA(array) + idx,
	    FTH_ARRAY_DATA(ins), sizeof(FTH) * (size_t) ins_len);
	FTH_ARRAY_LENGTH(array) += FTH_ARRAY_LENGTH(ins);
	FTH_INSTANCE_CHANGED(array);
	return (array);
}

static void
ficl_array_insert(ficlVm *vm)
{
#define h_array_insert "( ary1 idx val -- ary2 )  insert element\n\
#( 0 1 2 ) value ary1\n\
ary1 1 10 array-insert value ary2\n\
ary1 => #( 0 1 2 )\n\
ary2 => #( 0 10 1 2 )\n\
ary2 1 #( 4 5 6 ) array-insert => #( 0 4 5 6 10 1 2 )\n\
Insert VAL to ARY1 at position IDX and return new array.  \
VAL can be an array or any other object.  \
Negative IDX counts from backward.  \
Raise OUT-OF-RANGE exception if IDX is not in ARY1's range."
	FTH 		ary, val;
	ficlInteger 	idx;

	FTH_STACK_CHECK(vm, 3, 1);
	val = fth_pop_ficl_cell(vm);
	idx = ficlStackPopInteger(vm->dataStack);
	ary = fth_array_copy(fth_pop_ficl_cell(vm));
	ficlStackPushFTH(vm->dataStack, fth_array_insert(ary, idx, val));
}

static void
ficl_array_insert_bang(ficlVm *vm)
{
#define h_array_insert_bang "( ary idx val -- ary' )  insert element\n\
#( 0 1 2 ) value ary\n\
ary 1 10 array-insert! drop\n\
ary => #( 0 10 1 2 )\n\
ary 1 #( 4 5 6 ) array-insert! => #( 0 4 5 6 10 1 2 )\n\
Insert VAL to ARY at position IDX and return changed array.  \
VAL can be a single object or an array.  \
Negative IDX counts from backward.  \
Raise OUT-OF-RANGE exception if IDX is not in ARY's range."
	FTH 		ary, val;
	ficlInteger 	idx;

	FTH_STACK_CHECK(vm, 3, 1);
	val = fth_pop_ficl_cell(vm);
	idx = ficlStackPopInteger(vm->dataStack);
	ary = fth_array_insert(fth_pop_ficl_cell(vm), idx, val);
	ficlStackPushFTH(vm->dataStack, ary);
}

FTH
fth_array_delete(FTH array, ficlInteger idx)
{
	FTH 		value;
	ficlInteger 	len, cur_len;

	FTH_ASSERT_ARGS(fth_array_length(array) > 0,
	    array, FTH_ARG1, "a nonempty array");
	cur_len = FTH_ARRAY_LENGTH(array);

	if (idx < 0)
		idx += cur_len;

	if (idx < 0 || idx >= cur_len)
		FTH_OUT_OF_BOUNDS(FTH_ARG2, idx);

	if (idx == 0)
		return (fth_array_shift(array));

	if (idx == (cur_len - 1))
		return (fth_array_pop(array));

	value = FTH_ARRAY_DATA(array)[idx];
	FTH_ARRAY_LENGTH(array)--;
	len = NEW_SEQ_LENGTH(FTH_ARRAY_TOP(array) + FTH_ARRAY_LENGTH(array));

	if (len < FTH_ARRAY_BUF_LENGTH(array)) {
		FTH_ARRAY_BUF_LENGTH(array) = len;
		FTH_ARRAY_BUF(array) = FTH_REALLOC(FTH_ARRAY_BUF(array),
		    sizeof(FTH) * (size_t) len);
		FTH_ARRAY_DATA(array) = FTH_ARRAY_BUF(array) +
		    FTH_ARRAY_TOP(array);
	}
	memmove(FTH_ARRAY_DATA(array) + idx,
	    FTH_ARRAY_DATA(array) + idx + 1,
	    sizeof(FTH) * (size_t) (FTH_ARRAY_LENGTH(array) - idx));
	FTH_INSTANCE_CHANGED(array);
	return (value);
}

static void
ficl_array_delete(ficlVm *vm)
{
#define h_array_delete "( ary idx -- val )  delete element\n\
#( 'a 'b 'c ) value ary\n\
ary 1 array-delete! => 'b\n\
ary => #( 'a 'c )\n\
Delete and return one element from ARY at position IDX.  \
Negative index counts from backward.  \
Raise OUT-OF-RANGE exception if IDX is not in ARY's range.\n\
See also array-delete-key."
	FTH 		ary;
	ficlInteger 	idx;

	FTH_STACK_CHECK(vm, 2, 1);
	idx = ficlStackPopInteger(vm->dataStack);
	ary = fth_pop_ficl_cell(vm);
	fth_push_ficl_cell(vm, fth_array_delete(ary, idx));
}

FTH
fth_array_delete_key(FTH array, FTH key)
{
#define h_adk "( ary key -- val )  delete element\n\
#( 'a 'b 'c ) value ary\n\
ary 'c array-delete-key => 'c\n\
ary 'c array-delete-key => #f\n\
Delete and return KEY from ARY if found, otherwise return #f.\n\
See also array-delete!."
	ficlInteger 	i;
	FTH 		res;

	FTH_ASSERT_ARGS(FTH_ARRAY_P(array), array, FTH_ARG1, "an array");
	res = FTH_FALSE;

	for (i = 0; i < FTH_ARRAY_LENGTH(array); i++) {
		if (fth_object_equal_p(FTH_ARRAY_DATA(array)[i], key)) {
			res = fth_array_delete(array, i);
			break;
		}
	}

	return (res);
}

FTH
fth_array_reject(FTH array, FTH proc_or_xt, FTH args)
{
#define h_ary_reject_bang "( ary proc-or-xt args -- ary' )  remove elements\n\
#( 0 1 2 ) value ary\n\
ary lambda: <{ n1 n2 -- f }> n1 n2 > ; #( 2 ) array-reject! drop\n\
ary => #( 0 1 )\n\
\\ N1 corresponds to the current array element \
and N2 comes from args, here 2.\n\
\\ The same a bit shorter:\n\
#( 0 1 2 ) value ary\n\
ary <'> > #( 2 ) array-reject!\n\
ary => #( 0 1 )\n\
PROC-OR-XT will be called with ARGS, an array of zero or more proc arguments, \
and the current array element set as first arg in ARGS array.  \
The length of ARGS + 1 is the required arity of PROC-OR-XT.  \
If PROC-OR-XT returns neither #f nor nil nor 0, the element will be removed.\n\
See also array-reject."
	char           *caller = RUNNING_WORD();
	ficlInteger 	i, len;
	FTH 		proc, tmp;

	FTH_ASSERT_ARGS(FTH_ARRAY_P(array), array, FTH_ARG1, "an array");

	if (FTH_ARRAY_LENGTH(array) < 2)
		return (array);

	if (FTH_NIL_P(args))
		args = fth_make_empty_array();
	else if (!FTH_ARRAY_P(args))
		args = fth_make_array_var(1, args);

	len = FTH_ARRAY_LENGTH(args);
	proc = proc_from_proc_or_xt(proc_or_xt, (int) len + 1, 0, 0);
	FTH_ASSERT_ARGS(FTH_PROC_P(proc), proc, FTH_ARG2, "a proc");
	tmp = ary_copy(args);
	fth_array_unshift(tmp, FTH_UNDEF);

	for (i = 0; i < FTH_ARRAY_LENGTH(array); i++) {
		FTH 		ret;

		FTH_ARRAY_DATA(tmp)[0] = FTH_ARRAY_DATA(array)[i];
		ret = fth_proc_apply(proc, tmp, caller);

		if (!(FTH_FALSE_P(ret) || FTH_NIL_P(ret) || FTH_ZERO == ret))
			fth_array_delete(array, i--);
	}

	return (array);
}

static void
ficl_array_reject(ficlVm *vm)
{
#define h_array_reject "( ary1 proc-or-xt args -- ary2 )  remove elementes\n\
#( 0 1 2 ) value ary\n\
ary lambda: <{ n1 n2 -- f }> n1 n2 > ; #( 2 ) array-reject => #( 0 1 )\n\
\\ N1 corresponds to the current array element \
and N2 comes from args, here 2.\n\
\\ The same a bit shorter:\n\
ary <'> > #( 2 ) array-reject => #( 0 1 )\n\
PROC-OR-XT will be called with ARGS, an array of zero or more proc arguments, \
and the current array element set as first arg in ARGS array.  \
The length of ARGS + 1 is the required arity of PROC-OR-XT.  \
If PROC-OR-XT returns neither #f nor nil nor 0, \
the element will be pushed in a new array object.  \
The new array object will be returned.\n\
See also array-reject!."
	FTH 		ary, proc, args;

	FTH_STACK_CHECK(vm, 3, 1);
	args = fth_pop_ficl_cell(vm);
	proc = fth_pop_ficl_cell(vm);
	ary = fth_array_copy(fth_pop_ficl_cell(vm));
	ficlStackPushFTH(vm->dataStack, fth_array_reject(ary, proc, args));
}

static FTH
ary_compact_each(FTH value, FTH new)
{
	if (FTH_NOT_NIL_P(value))
		fth_array_push(new, value);
	return (new);
}

FTH
fth_array_compact(FTH array)
{
#define h_ary_comp_bang "( ary -- ary' )  remove nil elements\n\
#( 0 nil 1 nil 2 ) value ary\n\
ary array-compact! drop\n\
ary => #( 0 1 2 )\n\
Remove all nil elements from ARY and return changed array object.\n\
See also array-compact."
	FTH 		old;

	FTH_ASSERT_ARGS(FTH_ARRAY_P(array), array, FTH_ARG1, "an array");
	old = fth_array_copy(array);
	FTH_ARRAY_LENGTH(array) = 0;
	FTH_INSTANCE_CHANGED(array);
	return (fth_array_each(old, ary_compact_each, array));
}

static void
ficl_array_compact(ficlVm *vm)
{
#define h_array_compact "( ary1 -- ary2 )  remove nil elements\n\
#( 0 nil 1 nil 2 ) value ary1\n\
ary1 array-compact value ary2\n\
ary1 => #( 0 nil 1 nil 2 )\n\
ary2 => #( 0 1 2 )\n\
Return new array object with nil elements removed.\n\
See also array-compact!."
	FTH 		ary, new;

	FTH_STACK_CHECK(vm, 1, 1);
	ary = fth_pop_ficl_cell(vm);
	new = fth_array_each(ary, ary_compact_each, fth_make_empty_array());
	ficlStackPushFTH(vm->dataStack, new);
}

FTH
fth_array_fill(FTH array, FTH value)
{
#define h_array_fill "( ary val -- ary' )  fill array\n\
#( 0 1 2 ) value ary\n\
ary 10 array-fill drop\n\
ary => #( 10 10 10 )\n\
Set all elements of ARY to VAL."
	ficlInteger 	i, len;

	FTH_ASSERT_ARGS(FTH_ARRAY_P(array), array, FTH_ARG1, "an array");
	len = FTH_ARRAY_LENGTH(array);

	for (i = 0; i < len; i++)
		FTH_ARRAY_DATA(array)[i] = value;

	FTH_INSTANCE_CHANGED(array);
	return (array);
}

ficlInteger
fth_array_index(FTH array, FTH key)
{
	ficlInteger 	i, len;

	if (!FTH_ARRAY_P(array))
		return (-1);

	len = FTH_ARRAY_LENGTH(array);

	for (i = 0; i < len; i++)
		if (fth_object_equal_p(FTH_ARRAY_DATA(array)[i], key))
			return (i);

	return (-1);
}

static void
ficl_array_index(ficlVm *vm)
{
#define h_array_index "( ary key -- idx|-1 )  find KEY\n\
#( 'a 'b 'c ) 'b array-index => 1\n\
#( 'a 'b 'c ) 'f array-index => -1\n\
Return index of KEY in ARY or -1 if not found.\n\
See also array-member? and array-find."
	FTH 		ary, key;

	FTH_STACK_CHECK(vm, 2, 1);
	key = fth_pop_ficl_cell(vm);
	ary = fth_pop_ficl_cell(vm);
	ficlStackPushInteger(vm->dataStack, fth_array_index(ary, key));
}

int
fth_array_member_p(FTH array, FTH item)
{
	ficlInteger 	i, len;

	FTH_ASSERT_ARGS(FTH_ARRAY_P(array), array, FTH_ARG1, "an array");
	len = FTH_ARRAY_LENGTH(array);

	for (i = 0; i < len; i++)
		if (fth_object_equal_p(FTH_ARRAY_DATA(array)[i], item))
			return (1);

	return (0);
}

static void
ficl_array_member_p(ficlVm *vm)
{
#define h_array_member_p "( ary key -- f )  find KEY\n\
#( 'a 'b 'c ) 'b array-member? => #t\n\
#( 'a 'b 'c ) 'f array-member? => #f\n\
Return #t if KEY exists in ARY, otherwise #f.\n\
See also array-index and array-find."
	FTH 		ary, key;

	FTH_STACK_CHECK(vm, 2, 1);
	key = fth_pop_ficl_cell(vm);
	ary = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, fth_array_member_p(ary, key));
}

FTH
fth_array_find(FTH array, FTH key)
{
#define h_array_find "( ary key -- key|#f )  find KEY\n\
#( 'a 'b 'c ) 'b array-find => 'b\n\
#( 'a 'b 'c ) 'f array-find => #f\n\
Return key if KEY exists in ARY, otherwise #f.\n\
See also array-index and array-member?."
	ficlInteger 	i, len;

	FTH_ASSERT_ARGS(FTH_ARRAY_P(array), array, FTH_ARG1, "an array");
	len = FTH_ARRAY_LENGTH(array);

	for (i = 0; i < len; i++)
		if (fth_object_equal_p(FTH_ARRAY_DATA(array)[i], key))
			return (FTH_ARRAY_DATA(array)[i]);

	return (FTH_FALSE);
}

static FTH
ary_uniq_each(FTH value, FTH new)
{
	if (!fth_array_member_p(new, value))
		fth_array_push(new, value);
	return (new);
}

FTH
fth_array_uniq(FTH array)
{
#define h_array_uniq_bang "( ary -- ary' )  remove duplicates\n\
#( 0 1 2 3 2 1 0 ) value ary\n\
ary array-uniq! drop\n\
ary => #( 0 1 2 3 )\n\
Return ARY without duplicated elements.\n\
See also array-uniq."
	FTH 		old;

	FTH_ASSERT_ARGS(FTH_ARRAY_P(array), array, FTH_ARG1, "an array");
	old = fth_array_copy(array);
	FTH_ARRAY_LENGTH(array) = 0;
	FTH_INSTANCE_CHANGED(array);
	return (fth_array_each(old, ary_uniq_each, array));
}

static void
ficl_array_uniq(ficlVm *vm)
{
#define h_array_uniq "( ary1 -- ary2 )  remove duplicates\n\
#( 0 1 2 3 2 1 0 ) array-uniq => #( 0 1 2 3 )\n\
Return new array without duplicated elements of ARY1.\n\
See also array-uniq!."
	FTH 		ary, new;

	FTH_STACK_CHECK(vm, 1, 1);
	ary = fth_pop_ficl_cell(vm);
	new = fth_array_each(ary, ary_uniq_each, fth_make_empty_array());
	ficlStackPushFTH(vm->dataStack, new);
}

#if defined(HAVE_QSORT)

static FTH 	fth_cmp_proc;
static int
cmpit(const void *a, const void *b)
{
	FTH 		r;

	r = fth_proc_call(fth_cmp_proc, "array-sort", 2, *(FTH *)a, *(FTH *)b);
	return (FIX_TO_INT32(r));
}

#endif				/* HAVE_QSORT */

FTH
fth_array_sort(FTH array, FTH proc_or_xt)
{
#define h_array_sort_bang "( ary proc-or-xt -- ary' )  sort array\n\
#( 2 1 0 ) value ary\n\
ary lambda: <{ a b -- f }>\n\
	a b < if\n\
		-1\n\
	else\n\
		a b > if\n\
			1\n\
		else\n\
			0\n\
		then\n\
	then\n\
; array-sort! drop\n\
ary => #( 0 1 2 )\n\
Return the sorted ARY.  \
PROC-OR-XT compares two elements A and B \
and should return a negative integer if A < B, \
0 if A == B, and a positive integer if A > B.  \
Raise BAD-ARITY exception if PROC-OR-XT doesn't take two arguments.\n\
See also array-sort."
	FTH 		proc;
	size_t 		len;

	FTH_ASSERT_ARGS(FTH_ARRAY_P(array), array, FTH_ARG1, "an array");
	len = (size_t) FTH_ARRAY_LENGTH(array);

	if (len < 2)
		return (array);

	proc = proc_from_proc_or_xt(proc_or_xt, 2, 0, 0);
	FTH_ASSERT_ARGS(FTH_PROC_P(proc), proc, FTH_ARG2, "a compare proc");
#if defined(HAVE_QSORT)
	fth_cmp_proc = proc;
	qsort((void *) FTH_ARRAY_DATA(array), len, sizeof(FTH), cmpit);
#endif
	FTH_INSTANCE_CHANGED(array);
	return (array);
}

static void
ficl_array_sort(ficlVm *vm)
{
#define h_array_sort "( ary1 proc-or-xt -- ary2 )  sort array\n\
#( 2 1 0 ) value ary\n\
ary lambda: <{ a b -- f }>\n\
	a b < if\n\
		-1\n\
	else\n\
		a b > if\n\
			1\n\
		else\n\
			0\n\
		then\n\
	then\n\
; array-sort => #( 0 1 2 )\n\
Return new sorted array.  \
PROC-OR-XT compares two elements A and B \
and should return a negative integer if A < B, \
0 if A == B, and a positive integer if A > B.  \
Raise BAD-ARITY exception if PROC-OR-XT doesn't take two arguments.\n\
See also array-sort!."
	FTH 		ary1, ary2, proc;

	FTH_STACK_CHECK(vm, 2, 1);
	proc = fth_pop_ficl_cell(vm);
	ary1 = fth_pop_ficl_cell(vm);
	ary2 = fth_array_sort(fth_array_copy(ary1), proc);
	ficlStackPushFTH(vm->dataStack, ary2);
}

FTH
fth_array_join(FTH array, FTH sep)
{
#define h_array_join "( ary sep -- str )  join array to string\n\
#( 0 1 2 ) \"--\" array-join => \"0--1--2\"\n\
#( 0 1 2 ) nil  array-join => \"0 1 2\"\n\
Return string with all elements of ARY \
converted to their string representation \
and joined together separated by the string SEP.  \
If SEP is not a string, a space will be used as separator."
	ficlInteger 	i, len;
	FTH 		fs, el;

	FTH_ASSERT_ARGS(FTH_ARRAY_P(array), array, FTH_ARG1, "an array");
	fs = fth_make_empty_string();
	len = FTH_ARRAY_LENGTH(array);

	if (len == 0)
		return (fs);

	if (!FTH_STRING_P(sep))
		sep = fth_make_string(" ");

	fth_string_push(fs, fth_object_to_string(FTH_ARRAY_DATA(array)[0]));

	for (i = 1; i < len; i++) {
		fth_string_push(fs, sep);
		el = fth_object_to_string(FTH_ARRAY_DATA(array)[i]);
		fth_string_push(fs, el);
	}

	return (fs);
}

FTH
fth_array_subarray(FTH array, ficlInteger start, ficlInteger end)
{
	FTH 		result;
	ficlInteger 	len;
	size_t 		size;

	FTH_ASSERT_ARGS(FTH_ARRAY_P(array), array, FTH_ARG1, "an array");
	len = FTH_ARRAY_LENGTH(array);

	if (start < 0)
		start += len;

	if (start < 0 || start >= len)
		FTH_OUT_OF_BOUNDS(FTH_ARG2, start);

	if (end < 0) {
		/*
		 * We are looking for end length not last entry, hence end++.
		 */
		end += len;
		end++;
	}
	if (end < start || end > len)
		end = len;

	result = fth_make_array_len(end - start);
	size = sizeof(FTH) * (size_t) len;
	memmove(FTH_ARRAY_DATA(result), FTH_ARRAY_DATA(array) + start, size);
	return (result);
}

static void
ficl_array_subarray(ficlVm *vm)
{
#define h_array_subarray "( ary start end -- subary )  return part of array\n\
#( 0 1 2 3 4 )  2   4 array-subarray => #( 2 3 )\n\
#( 0 1 2 3 4 ) -3  -1 array-subarray => #( 2 3 4 )\n\
#( 0 1 2 3 4 ) -3 nil array-subarray => #( 2 3 4 )\n\
Return array built from ARY \
beginning with index START up to but excluding index END.  \
If END is NIL, up to end of array will be returned.  \
Negative index counts from backward.  \
Raise OUT-OF-RANGE exception if START is not in ARY's range."
	FTH 		ary, last;
	ficlInteger 	beg, end;

	FTH_STACK_CHECK(vm, 3, 1);
	last = fth_pop_ficl_cell(vm);
	beg = ficlStackPopInteger(vm->dataStack);
	ary = fth_pop_ficl_cell(vm);
	end = FTH_INTEGER_P(last) ? FTH_INT_REF(last) : fth_array_length(ary);
	ficlStackPushFTH(vm->dataStack, fth_array_subarray(ary, beg, end));
}

void
fth_array_clear(FTH array)
{
#define h_array_clear "( ary -- )  clear array\n\
#( 0 1 2 ) value ary\n\
ary array-clear\n\
ary => #( #f #f #f )\n\
Clear array and set all elements to #f."
	ficlInteger 	i, len;

	FTH_ASSERT_ARGS(FTH_ARRAY_P(array), array, FTH_ARG1, "an array");
	len = FTH_ARRAY_LENGTH(array);

	for (i = 0; i < len; i++)
		FTH_ARRAY_DATA(array)[i] = FTH_FALSE;

	FTH_INSTANCE_CHANGED(array);
}

/*
 * ACELLS, elements for assocs and alists.
 */

#define FTH_ACELL_P(Obj)						\
	(FTH_ARRAY_P(Obj) && (FTH_ARRAY_LENGTH(Obj) == 2))
#define FTH_ACELL_KEY(Obj)	FTH_ARRAY_DATA(Obj)[0]
#define FTH_ACELL_VAL(Obj)	FTH_ARRAY_DATA(Obj)[1]

static FTH
acl_inspect(FTH self)
{
	FTH 		key, val;

	key = FTH_ACELL_KEY(self);
	val = FTH_ACELL_VAL(self);
	return (fth_make_string_format("'( %I . %I )", key, val));
}

static FTH
acl_to_string(FTH self)
{
	FTH 		key, val;

	key = FTH_ACELL_KEY(self);
	val = FTH_ACELL_VAL(self);
	return (fth_make_string_format("'( %M . %M )", key, val));
}

static FTH
acl_dump(FTH self)
{
	FTH 		key, val;

	key = FTH_ACELL_KEY(self);
	val = FTH_ACELL_VAL(self);
	return (fth_make_string_format("%D %D", key, val));
}

/* Acell is a special two element array. */
FTH
fth_make_acell(FTH key, FTH value)
{
	FArray         *ary;

	ary = FTH_MALLOC(sizeof(FArray));
	ary->type = FTH_ARY_ARRAY;
	ary->length = 2;
	ary->buf_length = 2;
	ary->top = 0;
	ary->buf = FTH_MALLOC(sizeof(FTH) * 2);
	ary->data = ary->buf;
	ary->data[0] = key;
	ary->data[1] = value;
	return (fth_make_instance(acell_tag, ary));
}

/* fth_car and fth_cdr return FTH_FALSE if not a list. */
FTH
fth_acell_key(FTH cell)
{
	if (FTH_ACELL_P(cell))
		return (FTH_ACELL_KEY(cell));
	return (FTH_FALSE);
}

FTH
fth_acell_value(FTH cell)
{
	if (FTH_ACELL_P(cell))
		return (FTH_ACELL_VAL(cell));
	return (FTH_FALSE);
}

/*
 * ASSOC: sorted associative arrays as an alternative to hashs.
 */
static ficlInteger
assoc_index(FTH assoc, FTH key)
{
	FTH 		id, kid;
	ficlInteger 	i, beg, end;

	if (FTH_NIL_P(assoc) || FTH_FALSE_P(assoc))
		return (-1);

	FTH_ASSERT_ARGS(FTH_ARRAY_P(assoc), assoc, FTH_ARG1, "an array");

	if (FTH_ARRAY_LENGTH(assoc) == 0)
		return (-1);

	beg = 0;
	end = FTH_ARRAY_LENGTH(assoc) - 1;
	id = fth_hash_id(key);

	while (beg <= end) {
		i = (beg + end) / 2;
		kid = fth_hash_id(fth_acell_key(FTH_ARRAY_DATA(assoc)[i]));

		if (id == kid)
			return (i);
		else if (id < kid)
			end = i - 1;
		else
			beg = i + 1;
	}

	return (-1);
}

static void
ficl_assoc_p(ficlVm *vm)
{
#define h_assoc_p "( obj -- f )  test if OBJ is an assoc\n\
#a( 'a 0 'b 1 'c 2 ) assoc? => #t\n\
nil                  assoc? => #f\n\
Return #t if OBJ is an assoc array object, otherwise #f."
	FTH 		obj;
	int 		flag;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	flag = FTH_ARRAY_P(obj) && FTH_ARRAY_ASSOC_P(obj);
	ficlStackPushBoolean(vm->dataStack, flag);
}

static void
ficl_values_to_assoc(ficlVm *vm)
{
#define h_values_to_assoc "( vals len -- ary )  return assoc array\n\
'foo 0  'bar 1  4 >assoc => #a( '( 'foo . 0 ) '( 'bar . 1 ) )\n\
Return assoc array object with LEN/2 key-value pairs \
found on parameter stack.  \
Raise OUT-OF-RANGE exception if LEN < 0 or not even."
	ficlInteger 	i, len;
	FTH 		assoc, key, val;

	FTH_STACK_CHECK(vm, 1, 0);
	len = ficlStackPopInteger(vm->dataStack);

	if (len < 0)
		FTH_OUT_OF_BOUNDS_ERROR(FTH_ARGn, len, "negative");

	if (len % 2)
		FTH_OUT_OF_BOUNDS_ERROR(FTH_ARGn, len, "odd");

	if (len > MAX_SEQ_LENGTH)
		FTH_OUT_OF_BOUNDS_ERROR(FTH_ARGn, len, "too long");

	FTH_STACK_CHECK(vm, len, 1);
	assoc = fth_make_empty_array();
	FTH_ASSOC_SET(assoc);
	len /= 2;

	for (i = 0; i < len; i++) {
		val = fth_pop_ficl_cell(vm);
		key = fth_pop_ficl_cell(vm);
		fth_assoc(assoc, key, val);
	}

	ficlStackPushFTH(vm->dataStack, assoc);
}

static FTH
assoc_insert(FTH assoc, FTH id, FTH val)
{
	ficlInteger 	i, len, alen;

	FTH_ASSERT_ARGS(FTH_ARRAY_P(assoc), assoc, FTH_ARG1, "an array");

	if (!FTH_ARRAY_ASSOC_P(assoc))
		FTH_ASSOC_SET(assoc);

	alen = FTH_ARRAY_LENGTH(assoc);

	for (i = 0; i < alen; i++)
		if (id < fth_hash_id(fth_acell_key(FTH_ARRAY_DATA(assoc)[i])))
			break;

	len = NEW_SEQ_LENGTH(FTH_ARRAY_TOP(assoc) + alen + 1);

	if (len > MAX_SEQ_LENGTH)
		FTH_OUT_OF_BOUNDS_ERROR(FTH_ARG1, len, "too long");

	if (len > FTH_ARRAY_BUF_LENGTH(assoc)) {
		size_t 		size;

		FTH_ARRAY_BUF_LENGTH(assoc) = len;
		size = sizeof(FTH) * (size_t) len;
		FTH_ARRAY_BUF(assoc) = FTH_REALLOC(FTH_ARRAY_BUF(assoc), size);
		FTH_ARRAY_DATA(assoc) = FTH_ARRAY_BUF(assoc) +
		    FTH_ARRAY_TOP(assoc);
	}
	memmove(FTH_ARRAY_DATA(assoc) + i + 1,
	    FTH_ARRAY_DATA(assoc) + i,
	    sizeof(FTH) * (size_t) (FTH_ARRAY_LENGTH(assoc) - i));
	FTH_ARRAY_LENGTH(assoc)++;
	FTH_ARRAY_DATA(assoc)[i] = val;
	FTH_INSTANCE_CHANGED(assoc);
	return (assoc);
}

FTH
fth_assoc(FTH assoc, FTH key, FTH value)
{
#define h_assoc "( ass key val -- 'ass )  return assoc array\n\
#() value ass\n\
ass 'a 10 assoc => #a( '( 'a . 10 ) )\n\
ass 'b 20 assoc => #a( '( 'a . 10 ) '( 'b . 20 ) )\n\
Build sorted assoc array.  \
ASS must be an assoc array or an empty array #().\n\
See also array-assoc, array-assoc-ref, array-assoc-set!, array-assoc-remove!."
	FTH 		val;

	val = fth_make_acell(key, value);

	if (FTH_NIL_P(assoc) || FTH_FALSE_P(assoc)) {
		assoc = fth_make_array_var(1, val);
		FTH_ASSOC_SET(assoc);
		return (assoc);
	}
	return (assoc_insert(assoc, fth_hash_id(key), val));
}

FTH
fth_array_assoc(FTH assoc, FTH key)
{
#define h_array_ass "( ass key -- key-val|#f )  find KEY\n\
#() 'a #( 0 1 ) assoc value ass\n\
ass => #a( '( 'a . #( 0 1 ) ) )\n\
ass 'a array-assoc => '( 'a . #( 0 1 ) )\n\
ass  0 array-assoc => #f\n\
ass  1 array-assoc => #f\n\
If KEY matches, return corresponding key-value pair, otherwise #f."
	ficlInteger 	idx;

	idx = assoc_index(assoc, key);

	if (idx >= 0)
		return (ary_ref(assoc, fth_make_int(idx)));

	return (FTH_FALSE);
}

FTH
fth_array_assoc_ref(FTH assoc, FTH key)
{
#define h_aryass_ref "( ass key -- val|#f )  find KEY\n\
#() 'a #( 0 1 ) assoc value ass\n\
ass => #a( '( 'a . #( 0 1 ) ) )\n\
ass 'a array-assoc-ref => #( 0 1 )\n\
ass  0 array-assoc-ref => #f\n\
ass  1 array-assoc-ref => #f\n\
If KEY matches, return corresponding value, otherwise #f."
	ficlInteger 	idx;
	FTH 		as;

	idx = assoc_index(assoc, key);

	if (idx >= 0) {
		as = ary_ref(assoc, fth_make_int(idx));

		if (FTH_ACELL_P(as))
			return (FTH_ACELL_VAL(as));
		/* else fall through */
	}
	return (FTH_FALSE);
}

FTH
fth_array_assoc_set(FTH assoc, FTH key, FTH value)
{
#define h_array_ass_set "( ass key val -- 'ass )  set KEY to VAL\n\
#() 'a #( 0 1 ) assoc value ass\n\
ass => #a( '( 'a . #( 0 1 ) ) )\n\
ass 'a 10 array-assoc-set! => #a( '( 'a . 10 ) )\n\
ass  0 10 array-assoc-set! => #a( '( 0 . 10 ) '( 'a . 10 ) )\n\
ass  1 10 array-assoc-set! => #a( '( 0 . 10 ) '( 1 . 10 ) '( 'a . 10 ) )\n\
ass => #a( '( 0 . 10 ) '( 1 . 10 ) '( 'a . 10 ) )\n\
If KEY matches, set key-value pair, otherwise add new pair to ASS."
	ficlInteger 	idx;

	idx = assoc_index(assoc, key);

	if (idx >= 0) {
		fth_array_set(assoc, idx, fth_make_acell(key, value));
		return (assoc);
	}
	return (fth_assoc(assoc, key, value));
}

FTH
fth_array_assoc_remove(FTH assoc, FTH key)
{
#define h_aar "( ass key -- 'ass )  remove KEY\n\
#() 'a #( 0 1 ) assoc 'd 10 assoc value ass\n\
ass => #a( '( 'a . #( 0 1 ) ) '( 'd . 10 ) )\n\
ass  'a  array-assoc-remove! => #a( '( 'd . 10 ) )\n\
ass   0  array-assoc-remove! => #a( '( 'd . 10 ) )\n\
ass   1  array-assoc-remove! => #a( '( 'd . 10 ) )\n\
If KEY matches, remove key-value pair from ASS."
	ficlInteger 	idx;

	idx = assoc_index(assoc, key);

	if (idx >= 0)
		fth_array_delete(assoc, idx);

	return (assoc);
}

/*
 * LIST: Built with arrays.
 */

#define h_list_of_list_functions "\
*** LIST PRIMITIVES ***\n\
'()            	    ( -- lst )\n\
.list         	    ( lst -- )\n\
>list         	    ( vals len -- lst )\n\
cadddr         	    ( lst -- val )\n\
caddr         	    ( lst -- val )\n\
cadr         	    ( lst -- val )\n\
car         	    ( lst -- val )\n\
cddr         	    ( lst -- val )\n\
cdr         	    ( lst -- val )\n\
cons         	    ( val lst1 -- lst2 )\n\
cons2         	    ( val1 val2 lst1 -- lst2 )\n\
cons?         	    ( obj -- f )\n\
last-pair    	    ( lst -- val )\n\
list->array   	    ( lst -- ary )\n\
list-append    	    ( arg0 arg1 ... argn n -- lst )\n\
list-copy     	    ( lst1 -- ary2 )\n\
list-delete  	    ( lst1 key -- lst2 )\n\
list-delete!  	    ( lst key -- lst' )\n\
list-fill     	    ( lst val -- lst' )\n\
list-head           ( lst1 idx -- lst2 )\n\
list-index    	    ( lst key -- idx )\n\
list-insert  	    ( lst1 idx val -- lst2 )\n\
list-length   	    ( lst -- len )\n\
list-member?  	    ( lst key -- f )\n\
list-ref      	    ( lst idx -- val )\n\
list-reverse  	    ( lst1 -- ary2 )\n\
list-set!     	    ( lst idx val -- )\n\
list-slice          ( lst1 idx :key count 1 -- lst2 )\n\
list-slice!         ( lst idx :key count 1 -- lst' )\n\
list-tail     	    ( lst1 idx -- lst2 )\n\
list=         	    ( obj1 obj2 -- f )\n\
list?         	    ( obj -- f )\n\
make-list     	    ( len :key initial-element nil -- lst )\n\
nil?         	    ( obj -- f )\n\
null? alias for nil?\n\
pair?         	    ( obj -- f )\n\
set-car!       	    ( lst val -- lst' )\n\
set-cdr!       	    ( lst val -- lst' )\n\
Assoc lists:\n\
>alist         	    ( vals len -- alst )\n\
acons         	    ( key val alst1 -- alst2 )\n\
list-assoc    	    ( alst key -- ret )\n\
list-assoc-ref      ( alst key -- val )\n\
list-assoc-remove!  ( alst key -- alst' )\n\
list-assoc-set!     ( alst key val -- alst' )"

/*
 * If OBJ is a list (or array), return length of list, if OBJ is nil,
 * return 0 otherwise -1.
 */
ficlInteger
fth_list_length(FTH obj)
{
	if (FTH_CONS_P(obj))
		return (FTH_ARRAY_LENGTH(obj));
	return (FTH_NIL_P(obj) ? 0 : -1);
}

static void
ficl_list_length(ficlVm *vm)
{
#define h_list_length "( obj -- len )  return list length\n\
'( 0 1 2 ) list-length => 3\n\
nil        list-length => 0\n\
'()        list-length => 0\n\
5          list-length => -1\n\
If OBJ is a list (or array), return length of list, \
if OBJ is nil, return 0 otherwise -1."
	FTH 		obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushInteger(vm->dataStack, fth_list_length(obj));
}

static FTH
make_list_instance(FArray *ary)
{
	FTH_ARY_LIST_SET(ary);
	return (fth_make_instance(list_tag, ary));
}

FTH
fth_make_list_var(int len,...)
{
	int 		i;
	FArray         *ary;
	va_list 	list;

	ary = make_array((ficlInteger) len);
	va_start(list, len);

	for (i = 0; i < len; i++)
		ary->data[i] = va_arg(list, FTH);

	va_end(list);
	return (make_list_instance(ary));
}

FTH
fth_make_list_len(ficlInteger len)
{
	return (make_list_instance(make_array(len)));
}

FTH
fth_make_list_with_init(ficlInteger len, FTH init)
{
	ficlInteger 	i;
	FArray         *ary;

	ary = make_array(len);

	for (i = 0; i < ary->length; i++)
		ary->data[i] = init;

	return (make_list_instance(ary));
}

FTH
fth_make_empty_list(void)
{
	return (fth_make_list_len(0L));
}

static void
ficl_nil_p(ficlVm *vm)
{
#define h_nil_p "( obj -- f )  test if OBJ is nil\n\
'( 0 1 2 ) nil? => #f\n\
nil        nil? => #t\n\
'()        nil? => #t\n\
0          nil? => #f\n\
Return #t if OBJ is nil, otherwise #f."
	FTH 		obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, FTH_NIL_P(obj));
}

static void
ficl_list_p(ficlVm *vm)
#define h_list_p "( obj -- f )  test if OBJ is a list\n\
#( 0 1 2 ) list? => #f\n\
'( 0 1 2 ) list? => #t\n\
nil        list? => #t\n\
'()        list? => #t\n\
0          list? => #f\n\
Return #t if OBJ is a list (nil or a cons pointer), otherwise #f."
{
	FTH 		obj;
	int 		flag;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	flag = FTH_NIL_P(obj) || (FTH_CONS_P(obj) && FTH_ARRAY_LIST_P(obj));
	ficlStackPushBoolean(vm->dataStack, flag);
}

static void
ficl_cons_p(ficlVm *vm)
#define h_cons_p "( obj -- f )  test if OBJ is cons\n\
#( 0 1 2 ) cons? => #f\n\
'( 0 1 2 ) cons? => #t\n\
nil        cons? => #f\n\
'()        cons? => #f\n\
0          cons? => #f\n\
Return #t if OBJ is a cons pointer, otherwise #f."
{
	FTH 		obj;
	int 		flag;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	flag = FTH_CONS_P(obj) && FTH_ARRAY_LIST_P(obj);
	ficlStackPushBoolean(vm->dataStack, flag);
}

static void
ficl_pair_p(ficlVm *vm)
#define h_pair_p "( obj -- f )  test if OBJ is a pair\n\
#( 0 1 2 ) pair? => #f\n\
'( 0 1 2 ) pair? => #t\n\
nil        pair? => #f\n\
'()        pair? => #f\n\
0          pair? => #f\n\
Return #t if OBJ is a pair (a cons pointer), otherwise #f."
{
	FTH 		obj;
	int 		flag;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	flag = FTH_PAIR_P(obj) && FTH_ARRAY_LIST_P(obj);
	ficlStackPushBoolean(vm->dataStack, flag);
}

static void
ficl_make_list(ficlVm *vm)
{
#define h_make_list "( len :key initial-element nil -- lst )  return list\n\
0                     make-list => '()\n\
3                     make-list => '( nil nil nil )\n\
3 :initial-element 10 make-list => '( 10 10 10 )\n\
Return list of length LEN filled with keyword INITIAL-ELEMENT values.  \
INITIAL-ELEMENT defaults to nil if not specified.  \
Raise OUT-OF-RANGE exception if LEN < 0."
	FTH 		size, init, ls;

	init = fth_get_optkey(FTH_KEYWORD_INIT, FTH_NIL);
	FTH_STACK_CHECK(vm, 1, 1);
	size = fth_pop_ficl_cell(vm);
	FTH_ASSERT_ARGS(FTH_INTEGER_P(size), size, FTH_ARG1, "an integer");
	ls = fth_make_list_with_init(FTH_INT_REF(size), init);
	fth_push_ficl_cell(vm, ls);
}

static void
ficl_values_to_list(ficlVm *vm)
{
#define h_values_to_list "( len-vals len -- lst )  return list\n\
0 1 2   3 >list => '( 0 1 2 )\n\
Return list object with LEN objects found on parameter stack.  \
Raise OUT-OF-RANGE exception if LEN < 0."
	ficlInteger 	i, len;
	FTH 		ls;

	FTH_STACK_CHECK(vm, 1, 0);
	len = ficlStackPopInteger(vm->dataStack);
	ls = fth_make_list_len(len);
	FTH_STACK_CHECK(vm, len, 1);

	for (i = len - 1; i >= 0; i--)
		FTH_ARRAY_DATA(ls)[i] = fth_pop_ficl_cell(vm);

	ficlStackPushFTH(vm->dataStack, ls);
}

/*
 * Return Lisp-like cons pointer with VALUE as car and LIST as cdr.
 */
FTH
fth_cons(FTH val, FTH list)
{
#define h_list_cons "( val list1 -- list2 )  return list\n\
0 nil               cons => '( 0 )\n\
0 1 nil        cons cons => '( 0 1 )\n\
0 1 2 nil cons cons cons => '( 0 1 2 ) etc.\n\
Return Lisp-like cons pointer with VAL as car and LIST as cdr.\n\
See also cons2."
	if (FTH_NIL_P(list))
		return (fth_make_list_var(1, val));

	if (FTH_CONS_P(list))
		return (fth_array_unshift(list, val));

	return (fth_make_list_var(2, val, list));
}

/*
 * Return Lisp-like cons pointer with VALUE1 as car, VALUE2 as cadr
 * and LIST as cddr.
 */
FTH
fth_cons_2(FTH val1, FTH val2, FTH list)
{
#define h_list_cons_2 "( val1 val2 list1 -- list2 )  return list\n\
0 1 nil cons2 value lst1\n\
lst1 => '( 0 1 )\n\
lst1 car => 0\n\
lst1 cdr => '( 1 )\n\
0 1 2 nil cons cons2 value lst2\n\
lst2 => '( 0 1 2 )\n\
lst2 car => 0\n\
lst2 cdr => '( 1 2 )\n\
Return Lisp-like cons pointer with VAL1 as car, \
VAL2 as cadr and LIST as cddr.\n\
See also cons."
	if (FTH_NIL_P(list))
		return (fth_make_list_var(2, val1, val2));

	if (FTH_CONS_P(list))
		return (fth_array_unshift(fth_array_unshift(list, val2), val1));

	return (fth_make_list_var(3, val1, val2, list));

}

#define MAKE_CAR(Name, Idx)						\
FTH									\
fth_ ## Name (FTH list)							\
{									\
	FTH ret = FTH_NIL;						\
									\
	if (fth_list_length(list) > Idx)				\
		ret = FTH_ARRAY_DATA(list)[Idx];			\
									\
	return (ret);							\
}

MAKE_CAR(car, 0L)
MAKE_CAR(cadr, 1L)
MAKE_CAR(caddr, 2L)
MAKE_CAR(cadddr, 3L)

#define h_list_car "( list -- val )  return first list entry\n\
'( 0 1 2 3 4 ) car => 0\n\
'()            car => nil\n\
Return first entry, the car, of LIST or nil.\n\
See also cadr, caddr, cadddr."
#define h_list_cadr "( lst -- val )  return second list entry\n\
'( 0 1 2 3 4 ) cadr => 1\n\
'()            cadr => nil\n\
Return second entry, the cadr, of LIST or nil.\n\
See also car, caddr, cadddr."
#define h_list_caddr "( lst -- val )  return third list entry\n\
'( 0 1 2 3 4 ) caddr => 2\n\
'()            caddr => nil\n\
Return third entry, the caddr, of LIST or nil.\n\
See also car, cadr, cadddr."
#define h_list_cadddr "( lst -- val )  return fourth list entry\n\
'( 0 1 2 3 4 ) cadddr => 3\n\
'()            cadddr => nil\n\
Return fourth entry, the cadddr, of LIST or nil.\n\
See also car, cadr, caddr."

#define MAKE_CDR(Name, Idx)						\
FTH									\
fth_ ## Name (FTH list)							\
{									\
	FTH ls = FTH_NIL;						\
									\
	if (fth_list_length(list) > Idx) {				\
		ls = fth_array_subarray(list, Idx, -1L);		\
		FTH_LIST_SET(ls);					\
	}								\
	return (ls);							\
}

MAKE_CDR(cdr, 1L)
MAKE_CDR(cddr, 2L)

#define h_list_cdr "( list -- val )  return rest of list\n\
'( 0 1 2 ) cdr => '( 1 2 )\n\
'( 0 )     cdr => nil\n\
'()        cdr => nil\n\
Return rest, the cdr, of LIST without its first entry.\n\
See also cddr and car, cadr, caddr, cadddr."
#define h_list_cddr "( list -- val )  return rest of list\n\
'( 0 1 2 ) cddr => '( 2 )\n\
'( 0 )     cddr => nil\n\
'()        cddr => nil\n\
Return rest, the cddr, of LIST without its first and second entries.\n\
See also cdr and car, cadr, caddr, cadddr."

static void
ficl_print_list(ficlVm *vm)
{
#define h_print_list "( lst -- )  print list\n\
'( 0 1 2 ) .list => '( 0 1 2 )\n\
Print list object LST to current output."
	FTH 		obj;

	FTH_STACK_CHECK(vm, 1, 0);
	obj = fth_pop_ficl_cell(vm);
	FTH_ASSERT_ARGS(FTH_LIST_P(obj), obj, FTH_ARG1, "a list");
	fth_print(fth_string_ref(ary_to_string(obj)));
}

static void
ficl_list_equal_p(ficlVm *vm)
{
#define h_list_equal_p "( lst1 lst2 -- f )  compare lists\n\
'( 0 1 2 ) value l1\n\
'( 0 1 2 ) value l2\n\
'( 0 1 3 ) value l3\n\
l1 l1 list= #t\n\
l1 l2 list= #t\n\
l1 l3 list= #f\n\
Return #t if LST1 and LST2 are list objects of same length and content, \
otherwise #f."
	FTH 		obj1, obj2;
	int 		flag;

	FTH_STACK_CHECK(vm, 2, 1);
	obj2 = fth_pop_ficl_cell(vm);
	obj1 = fth_pop_ficl_cell(vm);
	/*
	 * nil == '()
	 * 0 make-list == '() (list object with 0 entries)
	 */
	flag = ((fth_list_length(obj1) == 0 && fth_list_length(obj2) == 0) ||
	    fth_array_equal_p(obj1, obj2));
	ficlStackPushBoolean(vm->dataStack, flag);
}

static void
ficl_set_car(ficlVm *vm)
{
#define h_list_set_car "( lst val -- lst' )  set car\n\
'( 0 1 2 ) 10 set-car! => '( 10 1 2 )\n\
       '() 10 set-car! => nil\n\
Set VAL to car of LST.\n\
See also set-cdr!."
	FTH 		lst, val;

	FTH_STACK_CHECK(vm, 2, 1);
	val = fth_pop_ficl_cell(vm);
	lst = fth_pop_ficl_cell(vm);

	if (FTH_CONS_P(lst) && FTH_ARRAY_LENGTH(lst) > 0) {
		fth_array_set(lst, 0L, val);
		ficlStackPushFTH(vm->dataStack, lst);
	} else
		fth_push_ficl_cell(vm, FTH_NIL);
}

static void
ficl_set_cdr(ficlVm *vm)
{
#define h_list_set_cdr "( lst val -- lst' )  set cdr\n\
'( 0 1 2 ) 10 set-cdr! => '( 0 10 )\n\
       '() 10 set-cdr! => nil\n\
Set VAL to cdr of LST.\n\
See also set-car!."
	FTH 		lst, val, ls;

	FTH_STACK_CHECK(vm, 2, 1);
	val = fth_pop_ficl_cell(vm);
	lst = fth_pop_ficl_cell(vm);
	ls = FTH_NIL;

	if (FTH_CONS_P(lst)) {
		if (FTH_ARRAY_LENGTH(lst) == 1)
			fth_array_push(lst, val);
		else {
			fth_array_set(lst, 1L, val);
			FTH_ARRAY_LENGTH(lst) = 2;
		}
		ls = fth_list_append(fth_make_acell(fth_acell_key(lst), val));
	}
	fth_push_ficl_cell(vm, ls);
}

FTH
fth_list_to_array(FTH list)
{
#define h_list_to_array "( lst -- ary|#f )  return array\n\
#( 0 #{ 'foo 10 } 2 ) value lst1\n\
lst1 list->array value ary2\n\
lst1 1 list-ref 'foo 30 hash-set!\n\
lst1 => '( 0 #{ 'foo 30 } 2 )\n\
ary2 => #( 0 #{ 'foo 30 } 2 )\n\
Return copy of LST as array only with references of each element \
in contrary to list-copy.  \
If LST is not a cons pointer, return #( lst )."
	if (FTH_CONS_P(list))
		return (ary_to_array(list));
	return (fth_make_array_var(1, list));
}

/*
 * Return copy of LIST using object-copy for all elements.
 */
FTH
fth_list_copy(FTH list)
{
#define h_list_copy "( lst1 -- lst2 )  duplicate list\n\
#( 0 #{ 'foo 10 } 2 ) value lst1\n\
lst1 list-copy value lst2\n\
lst1 1 list-ref 'foo 30 hash-set!\n\
lst1 => '( 0 #{ 'foo 30 } 2 )\n\
lst2 => '( 0 #{ 'foo 10 } 2 )\n\
Return copy of LST1 with all elements new created \
in contrary to list->array.\n\
See also list->array."
	FTH 		ls;

	ls = FTH_NIL;

	if (FTH_CONS_P(list)) {
		ls = ary_copy(list);
		FTH_LIST_SET(ls);
	}
	return (ls);
}

/*
 * Return element at position IDX.  Negative IDX counts from backward.
 * Raise OUT_OF_RANGE exception if IDX is not in LIST's range.
 */
FTH
fth_list_ref(FTH list, ficlInteger idx)
{
	if (FTH_CONS_P(list))
		return (fth_array_ref(list, idx));
	return (FTH_NIL);
}

static void
ficl_list_ref(ficlVm *vm)
{
#define h_list_ref "( lst idx -- val )  return value at IDX\n\
'( 'a 'b 'c ) 1 list-ref => 'b\n\
Return value at position IDX of LST.  \
Negative IDX counts from backward.  \
Raise OUT-OF-RANGE exception if IDX is not in LST's range."
	FTH 		lst;
	ficlInteger 	idx;

	FTH_STACK_CHECK(vm, 2, 1);
	idx = ficlStackPopInteger(vm->dataStack);
	lst = fth_pop_ficl_cell(vm);
	fth_push_ficl_cell(vm, fth_list_ref(lst, idx));
}

/*
 * Store VALUE at position IDX and return VALUE.  Negative IDX counts
 * from backward.  Raise OUT_OF_RANGE exception if IDX is not in LST's
 * range.
 */
FTH
fth_list_set(FTH list, ficlInteger idx, FTH value)
{
	if (FTH_CONS_P(list))
		fth_array_set(list, idx, value);
	return (value);
}

static void
ficl_list_set(ficlVm *vm)
{
#define h_list_set "( lst idx val -- )  set value at IDX\n\
'( 'a 'b 'c ) value lst\n\
lst 1 'e list-set!\n\
lst => '( 'a 'e 'c )\n\
Store VAL at position IDX in LST.  \
Negative IDX counts from backward.  \
Raise OUT-OF-RANGE exception if IDX is not in LST's range."
	FTH 		lst, val;
	ficlInteger 	idx;

	FTH_STACK_CHECK(vm, 3, 0);
	val = fth_pop_ficl_cell(vm);
	idx = ficlStackPopInteger(vm->dataStack);
	lst = fth_pop_ficl_cell(vm);
	fth_list_set(lst, idx, val);
}

static FTH
ls_append_each(FTH value, FTH ls)
{
	if (FTH_NOT_NIL_P(value)) {
		ls = fth_array_append(ls, value);
		FTH_LIST_SET(ls);
	}
	return (ls);
}

/*
 * If ARGS is not an Array or List object, return FTH_NIL, otherwise
 * return new List object with each element of ARGS append with
 * fth_array_append.
 */
FTH
fth_list_append(FTH args)
{
	FTH 		ls;

	ls = FTH_NIL;

	if (FTH_CONS_P(args)) {
		ls = fth_make_empty_list();
		ls = fth_array_each(args, ls_append_each, ls);
		FTH_LIST_SET(ls);
	}
	return (ls);
}

static void
ficl_list_append(ficlVm *vm)
{
#define h_list_append "( arg0 arg1 ... argn n -- lst )  return list\n\
0 1 '( 2 3 4 ) 5 6         5 list-append => '( 0 1 2 3 4 5 6 )\n\
'( 0 ) '( 1 )              2 list-append => '( 0 1 )\n\
'( 0 ) '( 1 2 3 )          2 list-append => '( 0 1 2 3 )\n\
'( 0 '( 1 ) ) '( '( 2 ) )  2 list-append => '( 0 '( 1 ) '( 2 ) )\n\
'( 0 1 ) 2 3 nil acons     2 list-append => '( 0 1 '( 2 . 3 ) )\n\
'() 0                      2 list-append => '( 0 )\n\
                           0 list-append => '()\n\
Return list object with N objects found on parameter stack.  \
Raise OUT-OF-RANGE exception if N < 0."
	ficlInteger 	i, len;
	FTH 		ls;

	FTH_STACK_CHECK(vm, 1, 1);
	ls = FTH_NIL;
	len = ficlStackPopInteger(vm->dataStack);

	if (len < 0)
		FTH_OUT_OF_BOUNDS_ERROR(FTH_ARGn, len, "negative");

	if (len > MAX_SEQ_LENGTH)
		FTH_OUT_OF_BOUNDS_ERROR(FTH_ARGn, len, "too long");

	if (len == 0) {
		fth_push_ficl_cell(vm, ls);
		return;
	}
	FTH_STACK_CHECK(vm, len, 1);
	ls = fth_make_list_len(len);

	for (i = len - 1; i >= 0; i--)
		FTH_ARRAY_DATA(ls)[i] = fth_pop_ficl_cell(vm);

	fth_push_ficl_cell(vm, fth_list_append(ls));
}

/*
 * Return new list with elements reversed.
 */
FTH
fth_list_reverse(FTH list)
{
#define h_list_reverse "( lst1 -- lst2 )  reverse list elements\n\
'( 0 1 2 ) value l1\n\
l1 list-reverse value l2\n\
l1 => '( 0 1 2 )\n\
l2 => '( 2 1 0 )\n\
Return new list with elements reversed."
	FTH 		ls;

	ls = list;

	if (FTH_CONS_P(list)) {
		ls = fth_array_reverse(ary_copy(list));
		FTH_LIST_SET(ls);
	}
	return (ls);
}

static void
ficl_list_insert(ficlVm *vm)
{
#define h_list_insert "( lst1 idx val -- lst2 )  insert element\n\
'( 0 1 2 ) value l1\n\
l1 0 10 list-insert value l2\n\
l1 => '( 0 1 2 )\n\
l2 => '( 10 0 1 2 )\n\
l1 1 '( 4 5 6 ) list-insert => '( 0 4 5 6 10 1 2 )\n\
Insert VAL to LST1 at position IDX and return new list.  \
VAL can be a list or any other object.  \
Negative IDX counts from backward.  \
Raise OUT-OF-RANGE exception if IDX is not in LST1's range."
	ficlInteger 	idx;
	FTH 		lst, val, ls;

	FTH_STACK_CHECK(vm, 3, 1);
	val = fth_pop_ficl_cell(vm);
	idx = ficlStackPopInteger(vm->dataStack);
	lst = fth_pop_ficl_cell(vm);
	ls = FTH_NIL;

	if (FTH_CONS_P(lst)) {
		ls = fth_array_insert(ary_copy(lst), idx, val);
		FTH_LIST_SET(ls);
	}
	fth_push_ficl_cell(vm, ls);
}

static FTH
ls_delete_each(FTH value, FTH data)
{
	FTH 		key, ls;

	key = FTH_ARRAY_DATA(data)[0];
	ls = FTH_ARRAY_DATA(data)[1];

	if (!fth_object_equal_p(value, key))
		fth_array_push(ls, value);

	return (data);
}

static void
ficl_list_delete(ficlVm *vm)
{
#define h_list_delete "( lst1 key -- lst2 )  delete element\n\
'( 0 0 1 2 ) 0 list-delete => '( 1 2 )\n\
Return new list without all elements equal KEY.\n\
See also list-delete!."
	FTH 		lst, key, data, ls;

	FTH_STACK_CHECK(vm, 2, 1);
	key = fth_pop_ficl_cell(vm);
	lst = fth_pop_ficl_cell(vm);
	ls = FTH_NIL;

	if (FTH_CONS_P(lst)) {
		ls = fth_make_empty_list();
		data = fth_make_array_var(2, key, ls);
		fth_array_each(lst, ls_delete_each, data);
	}
	fth_push_ficl_cell(vm, ls);
}

static void
ficl_list_delete_bang(ficlVm *vm)
{
#define h_list_delete_bang "( lst key -- lst' )  delete element\n\
'( 'a 'b 'c ) value ls\n\
ls 1 list-delete! => 'b\n\
ls => '( 'a 'c )\n\
Return LST without all elements equal KEY.\n\
See also list-delete."
	ficlInteger 	i, len;
	FTH 		lst, key;

	FTH_STACK_CHECK(vm, 2, 1);
	key = fth_pop_ficl_cell(vm);
	lst = fth_pop_ficl_cell(vm);

	if (FTH_CONS_P(lst)) {
		len = FTH_ARRAY_LENGTH(lst);

		for (i = 0; i < len; i++)
			if (fth_object_equal_p(FTH_ARRAY_DATA(lst)[i], key))
				fth_array_delete(lst, i--);
	}
	fth_push_ficl_cell(vm, lst);
}

static void
ficl_list_slice(ficlVm *vm)
{
#define h_list_slice "( lst1 idx :key count 1 -- lst2 )  remove elements\n\
'( 0 1 1 2 ) 1 :count 2 list-slice => '( 0 2 )\n\
Return new list without COUNT elements from IDX on.  \
Raise OUT-OF-RANGE exception if IDX is not in LST1's range.\n\
See also list-slice!."
	ficlInteger 	idx, cnt;
	FTH 		lst, ls;

	cnt = fth_get_optkey_int(FTH_KEYWORD_COUNT, 1L);
	FTH_STACK_CHECK(vm, 2, 1);
	idx = ficlStackPopInteger(vm->dataStack);
	lst = fth_pop_ficl_cell(vm);
	ls = FTH_NIL;

	if (FTH_CONS_P(lst)) {
		ficlInteger 	i , end, len;

		len = FTH_ARRAY_LENGTH(lst);

		if (idx < 0)
			idx += len;

		if (idx < 0 || idx >= len)
			FTH_OUT_OF_BOUNDS(FTH_ARG2, idx);

		end = idx + cnt;
		ls = fth_make_empty_list();

		for (i = 0; i < len; i++)
			if (i < idx || i >= end)
				fth_array_push(ls, FTH_ARRAY_DATA(lst)[i]);
	}
	fth_push_ficl_cell(vm, ls);
}

static void
ficl_list_slice_bang(ficlVm *vm)
{
#define h_list_slice_bang "( lst idx :key count 1 -- lst' )  remove elements\n\
'( 0 1 1 2 ) value ls\n\
ls 1 :count 2 list-slice! drop\n\
ls => '( 0 2 )\n\
Return LST without COUNT elements from IDX on.  \
Raise OUT-OF-RANGE exception if IDX is not in LST's range.\n\
See also list-slice."
	FTH 		lst;
	ficlInteger 	idx, cnt;

	cnt = fth_get_optkey_int(FTH_KEYWORD_COUNT, 1L);
	FTH_STACK_CHECK(vm, 2, 1);
	idx = ficlStackPopInteger(vm->dataStack);
	lst = fth_pop_ficl_cell(vm);

	if (FTH_CONS_P(lst)) {
		ficlInteger 	i, j, end, len;
		FTH 		el;

		len = FTH_ARRAY_LENGTH(lst);

		if (idx < 0)
			idx += len;

		if (idx < 0 || idx >= len)
			FTH_OUT_OF_BOUNDS(FTH_ARG2, idx);

		end = idx + cnt;

		for (i = 0, j = 0; i < len; i++) {
			if (i < idx || i >= end) {
				el = FTH_ARRAY_DATA(lst)[i];
				FTH_ARRAY_DATA(lst)[j++] = el;
			}
		}
		FTH_ARRAY_LENGTH(lst) = j;
	}
	fth_push_ficl_cell(vm, lst);
}

static void
ficl_list_fill(ficlVm *vm)
{
#define h_list_fill "( lst val -- lst' )  fill list\n\
'( 0 1 2 ) value ls\n\
ls 10 list-fill drop\n\
ls => '( 10 10 10 )\n\
Set all elements of LST to VAL."
	FTH 		lst, val;

	FTH_STACK_CHECK(vm, 2, 1);
	val = fth_pop_ficl_cell(vm);
	lst = fth_pop_ficl_cell(vm);

	if (FTH_CONS_P(lst))
		fth_array_fill(lst, val);

	fth_push_ficl_cell(vm, lst);
}

static void
ficl_list_index(ficlVm *vm)
{
#define h_list_index "( lst key -- idx|-1 )  find KEY\n\
'( 'a 'b 'c ) 'b list-index => 1\n\
'( 'a 'b 'c ) 'f list-index => -1\n\
Return index of KEY in LST or -1 if not found.\n\
See also list-member?."
	FTH 		lst, key;
	ficlInteger 	idx;

	FTH_STACK_CHECK(vm, 2, 1);
	key = fth_pop_ficl_cell(vm);
	lst = fth_pop_ficl_cell(vm);
	idx = -1;

	if (FTH_CONS_P(lst))
		idx = fth_array_index(lst, key);

	ficlStackPushInteger(vm->dataStack, idx);
}

/*
 * Return FTH_TRUE if KEY exists in LIST, otherwise FTH_FALSE.
 */
FTH
fth_list_member_p(FTH list, FTH key)
{
#define h_list_member_p "( lst key -- f )  find KEY\n\
'( 'a 'b 'c ) 'b list-member? => #t\n\
'( 'a 'b 'c ) 'f list-member? => #f\n\
Return #t if KEY exists in LST, otherwise #f.\n\
See also list-index."
	if (FTH_CONS_P(list))
		return (BOOL_TO_FTH(fth_array_member_p(list, key)));
	return (FTH_FALSE);
}

static void
ficl_list_head(ficlVm *vm)
{
#define h_list_head "( lst1 idx -- lst2 )  return part of list\n\
'( 0 1 2 3 ) 2 list-head => '( 0 1 )\n\
Return first IDX entries of LST1 in a new list or nil.\n\
See also list-tail and list-pair."
	ficlInteger 	idx;
	FTH 		lst, ls;

	FTH_STACK_CHECK(vm, 2, 1);
	idx = ficlStackPopInteger(vm->dataStack);
	lst = fth_pop_ficl_cell(vm);
	ls = FTH_NIL;

	if (FTH_CONS_P(lst)) {
		ls = fth_array_subarray(lst, 0L, idx);
		FTH_LIST_SET(ls);
	}
	fth_push_ficl_cell(vm, ls);
}

static void
ficl_list_tail(ficlVm *vm)
{
#define h_list_tail "( lst1 idx -- lst2 )  return part of list\n\
'( 0 1 2 3 ) 2 list-tail => '( 2 3 )\n\
Return IDX'th cdr of LST1 up to the last entry in a new list or nil.\n\
See also list-head and list-pair."
	ficlInteger 	idx, len;
	FTH 		lst, ls;

	FTH_STACK_CHECK(vm, 2, 1);
	idx = ficlStackPopInteger(vm->dataStack);
	lst = fth_pop_ficl_cell(vm);
	ls = FTH_NIL;

	if (FTH_CONS_P(lst)) {
		len = FTH_ARRAY_LENGTH(lst);

		if (idx >= 0 && len >= idx) {
			ficlInteger 	i , j;

			ls = fth_make_list_len(len - idx);

			for (i = idx, j = 0; i < len; i++, j++)
				FTH_ARRAY_DATA(ls)[j] = FTH_ARRAY_DATA(lst)[i];
		} else
			ls = lst;
	}
	fth_push_ficl_cell(vm, ls);
}

static void
ficl_last_pair(ficlVm *vm)
{
#define h_last_pair "( list -- last-pair )  return part of list\n\
'( 0 1 2 3 ) last-pair => '( 3 )\n\
'( 0 ) 1 2 nil acons 2 list-append value ls\n\
ls => '( 0 '( 1 . 2 ) )\n\
ls last-pair => '( '( 1 . 2 ) )\n\
Return last pair in LIST.\n\
See also list-head and list-tail."
	FTH 		lst, ls;

	FTH_STACK_CHECK(vm, 1, 1);
	lst = fth_pop_ficl_cell(vm);
	ls = FTH_NIL;

	if (FTH_CONS_P(lst) && FTH_ARRAY_LENGTH(lst) > 0)
		ls = fth_make_list_var(1, fth_array_ref(lst, -1L));

	fth_push_ficl_cell(vm, ls);
}

/*
 * ALIST: sorted associative lists as an alternative to hashs.
 */

static void
ficl_values_to_alist(ficlVm *vm)
{
#define h_values_to_alist "( vals len -- ary )  return assoc list\n\
'foo 0  'bar 1  4 >alist => 'a( '( 'foo . 0 ) '( 'bar . 1 ) )\n\
Return assoc list object with LEN/2 key-value pairs \
found on parameter stack.  \
Raise OUT-OF-RANGE exception if LEN < 0 or not even."
	ficlInteger 	i, len;
	FTH 		alist, key, val;

	FTH_STACK_CHECK(vm, 1, 0);
	len = ficlStackPopInteger(vm->dataStack);

	if (len < 0)
		FTH_OUT_OF_BOUNDS_ERROR(FTH_ARGn, len, "negative");

	if (len % 2)
		FTH_OUT_OF_BOUNDS_ERROR(FTH_ARGn, len, "odd");

	if (len > MAX_SEQ_LENGTH)
		FTH_OUT_OF_BOUNDS_ERROR(FTH_ARGn, len, "too long");

	FTH_STACK_CHECK(vm, len, 1);
	alist = fth_make_empty_list();
	len /= 2;

	for (i = 0; i < len; i++) {
		val = fth_pop_ficl_cell(vm);
		key = fth_pop_ficl_cell(vm);
		assoc_insert(alist, fth_hash_id(key), fth_make_acell(key, val));
	}

	ficlStackPushFTH(vm->dataStack, alist);
}

FTH
fth_acons(FTH key, FTH value, FTH alist)
{
#define h_list_acons "( key val alist -- alist' )  return assoc list\n\
'() value ass\n\
'a 10 ass acons to ass\n\
ass => 'a( '( 'a . 10 ) )\n\
'b 20 ass acons to ass\n\
ass => 'a( '( 'a . 10 ) '( 'b . 20 ) )\n\
Return new Lisp-like associated list from key-value pair and ALIST.\n\
See also list-assoc, list-assoc-ref, list-assoc-set!, list-assoc-remove!."
	FTH 		ls, val;

	val = fth_make_acell(key, value);

	if (FTH_NIL_P(alist))
		ls = fth_make_list_var(1, val);
	else if (FTH_CONS_P(alist))
		ls = fth_array_unshift(alist, val);
	else
		ls = fth_make_list_var(2, val, alist);

	FTH_ASSOC_SET(ls);
	return (ls);
}

/*
 * If KEY matches, return corresponding key-value pair, otherwise
 * FTH_FALSE.
 */
FTH
fth_list_assoc(FTH alist, FTH key)
{
#define h_list_ass "( alist key -- key-val|#f )  find KEY\n\
'a '( 0 1 ) nil acons value ass\n\
ass => 'a( '( 'a . '( 0 1 ) ) )\n\
ass 'a list-assoc => '( 'a . '( 0 1 ) )\n\
ass 0 list-assoc => #f\n\
ass 1 list-assoc => #f\n\
If KEY matches, return corresponding key-value pair, otherwise #f."
	if (FTH_CONS_P(alist))
		return (fth_array_assoc(alist, key));
	return (FTH_FALSE);
}

/*
 * If KEY matches, return corresponding value, otherwise FTH_FALSE.
 */
FTH
fth_list_assoc_ref(FTH alist, FTH key)
{
#define h_list_ass_ref "( alist key -- val|#f )  find KEY\n\
'a '( 0 1 ) nil acons value ass\n\
ass => 'a( '( 'a . '( 0 1 ) ) )\n\
ass 'a list-assoc-ref => '( 0 1 )\n\
ass 0 list-assoc-ref => #f\n\
ass 1 list-assoc-ref => #f\n\
If KEY matches, return corresponding value, otherwise #f."
	if (FTH_CONS_P(alist))
		return (fth_array_assoc_ref(alist, key));
	return (FTH_FALSE);
}

/*
 * If KEY matches, set key-value pair, otherwise add new pair to ALIST.
 * Return current assoc-list.
 */
FTH
fth_list_assoc_set(FTH alist, FTH key, FTH value)
{
#define h_list_ass_set "( alist key val -- alist' )  set KEY to VAL\n\
'a '( 0 1 ) nil acons value ass\n\
ass => 'a( '( 'a . '( 0 1 ) ) )\n\
ass 'a 10 list-assoc-set! => 'a( '( 'a . 10 ) )\n\
ass  0 10 list-assoc-set! => 'a( '( 0 . 10 ) '( 'a . 10 ) )\n\
ass  1 10 list-assoc-set! => 'a( '( 0 . 10 ) '( 1 . 10 ) '( 'a . 10 ) )\n\
ass => 'a( '( 0 . 10 ) '( 1 . 10 ) '( 'a . 10 ) )\n\
If KEY matches, set key-value pair, otherwise add new pair to ALIST."
	ficlInteger 	idx;
	FTH 		val;

	val = fth_make_acell(key, value);

	if (FTH_CONS_P(alist)) {
		idx = assoc_index(alist, key);

		if (idx >= 0)
			fth_array_set(alist, idx, val);
		else
			assoc_insert(alist, fth_hash_id(key), val);
	} else {
		alist = fth_make_list_var(1, val);
		FTH_ASSOC_SET(alist);
	}
	return (alist);
}

/*
 * If KEY matches, remove key-value pair from ALIST.  Return current
 * assoc-list.
 */
FTH
fth_list_assoc_remove(FTH alist, FTH key)
{
#define h_list_ass_rem "( alist key -- alist' )  remove element\n\
'a '( 0 1 )  'd 10 nil acons  acons value ass\n\
ass => 'a( '( 'a . '( 0 1 ) ) '( 'd . 10 ) )\n\
ass 'a list-assoc-remove! => 'a( '( 'd . 10 ) )\n\
ass  0 list-assoc-remove! => 'a( '( 'd . 10 ) )\n\
ass  1 list-assoc-remove! => 'a( '( 'd . 10 ) )\n\
If KEY matches, remove key-value pair from ALIST.  \
Return current ALIST."
	if (FTH_CONS_P(alist))
		return (fth_array_assoc_remove(alist, key));
	return (alist);
}

void
init_array_type(void)
{
	/* array */
	array_tag = make_object_type(FTH_STR_ARRAY, FTH_ARRAY_T);
	fth_set_object_inspect(array_tag, ary_inspect);
	fth_set_object_to_string(array_tag, ary_to_string);
	fth_set_object_dump(array_tag, ary_dump);
	fth_set_object_to_array(array_tag, ary_to_array);
	fth_set_object_copy(array_tag, ary_copy);
	fth_set_object_value_ref(array_tag, ary_ref);
	fth_set_object_value_set(array_tag, ary_set);
	fth_set_object_equal_p(array_tag, ary_equal_p);
	fth_set_object_length(array_tag, ary_length);
	fth_set_object_mark(array_tag, ary_mark);
	fth_set_object_free(array_tag, ary_free);
	/* list */
	list_tag = make_object_type_from(FTH_STR_LIST,
	    FTH_ARRAY_T, array_tag);
	/* acell */
	acell_tag = make_object_type_from(FTH_STR_ACELL,
	    FTH_ARRAY_T, array_tag);
	fth_set_object_inspect(acell_tag, acl_inspect);
	fth_set_object_to_string(acell_tag, acl_to_string);
	fth_set_object_dump(acell_tag, acl_dump);
}

void
init_array(void)
{
	fth_set_object_apply(array_tag, (void *) ary_ref, 1, 0, 0);

	/* struct members */
	init_ary_length();
	init_ary_buf_length();
	init_ary_top();

	/* array */
	FTH_PRI1("array-length", ficl_array_length, h_array_length);
	FTH_PRI1("array?", ficl_array_p, h_array_p);
	FTH_PRI1("make-array", ficl_make_array, h_make_array);
	FTH_PRI1("array-concat", ficl_values_to_array, h_values_to_array);
	FTH_PRI1(">array", ficl_values_to_array, h_values_to_array);
	FTH_PRI1("#()", ficl_make_empty_array, h_empty_array);
	FTH_PRI1(".array", ficl_print_array, h_print_array);
	FTH_PRI1("array=", ficl_array_equal_p, h_array_equal_p);
	FTH_PROC("array->array", fth_array_to_array, 1, 0, 0, h_array_to_array);
	FTH_PROC("array->list", fth_array_to_list, 1, 0, 0, h_array_to_list);
	FTH_PRI1("array-copy", ficl_array_copy, h_array_copy);
	FTH_PRI1("array-ref", ficl_array_ref, h_array_ref);
	FTH_PRI1("array-set!", ficl_array_set, h_array_set);
	FTH_PROC("array-push", fth_array_push, 2, 0, 0, h_array_push);
	FTH_PROC("array-pop", fth_array_pop, 1, 0, 0, h_array_pop);
	FTH_PROC("array-unshift", fth_array_unshift, 2, 0, 0, h_array_unshift);
	FTH_PROC("array-shift", fth_array_shift, 1, 0, 0, h_array_shift);
	FTH_PROC("array-append", fth_array_append, 2, 0, 0, h_array_append);
	FTH_PRI1("array-reverse", ficl_array_reverse, h_array_reverse);
	FTH_PROC("array-reverse!", fth_array_reverse, 1, 0, 0, h_ary_rev_bang);
	FTH_PRI1("array-insert", ficl_array_insert, h_array_insert);
	FTH_PRI1("array-insert!", ficl_array_insert_bang, h_array_insert_bang);
	FTH_PRI1("array-delete!", ficl_array_delete, h_array_delete);
	FTH_PROC("array-delete-key", fth_array_delete_key, 2, 0, 0, h_adk);
	FTH_PRI1("array-reject", ficl_array_reject, h_array_reject);
	FTH_PROC("array-reject!", fth_array_reject, 3, 0, 0, h_ary_reject_bang);
	FTH_PRI1("array-compact", ficl_array_compact, h_array_compact);
	FTH_PROC("array-compact!", fth_array_compact, 1, 0, 0, h_ary_comp_bang);
	FTH_PROC("array-fill", fth_array_fill, 2, 0, 0, h_array_fill);
	FTH_PRI1("array-index", ficl_array_index, h_array_index);
	FTH_PRI1("array-member?", ficl_array_member_p, h_array_member_p);
	FTH_PROC("array-find", fth_array_find, 2, 0, 0, h_array_find);
	FTH_PRI1("array-uniq", ficl_array_uniq, h_array_uniq);
	FTH_PROC("array-uniq!", fth_array_uniq, 1, 0, 0, h_array_uniq_bang);
	FTH_PRI1("array-sort", ficl_array_sort, h_array_sort);
	FTH_PROC("array-sort!", fth_array_sort, 2, 0, 0, h_array_sort_bang);
	FTH_PROC("array-join", fth_array_join, 2, 0, 0, h_array_join);
	FTH_PRI1("array-subarray", ficl_array_subarray, h_array_subarray);
	FTH_VOID_PROC("array-clear", fth_array_clear, 1, 0, 0, h_array_clear);

	/* assoc */
	FTH_PRI1("assoc?", ficl_assoc_p, h_assoc_p);
	FTH_PRI1(">assoc", ficl_values_to_assoc, h_values_to_assoc);
	FTH_PROC("assoc", fth_assoc, 3, 0, 0, h_assoc);
	FTH_PROC("array-assoc", fth_array_assoc, 2, 0, 0, h_array_ass);
	FTH_PROC("array-assoc-ref", fth_array_assoc_ref, 2, 0, 0, h_aryass_ref);
	FTH_PROC("array-assoc-set!", fth_array_assoc_set, 3, 0, 0,
	    h_array_ass_set);
	FTH_PROC("array-assoc-remove!", fth_array_assoc_remove, 2, 0, 0, h_aar);
	FTH_ADD_FEATURE_AND_INFO(FTH_STR_ARRAY, h_list_of_array_functions);

	/* list */
	FTH_PRI1("list-length", ficl_list_length, h_list_length);
	FTH_PRI1("nil?", ficl_nil_p, h_nil_p);
	FTH_PRI1("null?", ficl_nil_p, h_nil_p);
	FTH_PRI1("list?", ficl_list_p, h_list_p);
	FTH_PRI1("cons?", ficl_cons_p, h_cons_p);
	FTH_PRI1("pair?", ficl_pair_p, h_pair_p);
	FTH_PRI1("make-list", ficl_make_list, h_make_list);
	FTH_PRI1(">list", ficl_values_to_list, h_values_to_list);
	fth_define_constant("'()", FTH_NIL, "( -- empty-lst )");
	FTH_PROC("cons", fth_cons, 2, 0, 0, h_list_cons);
	FTH_PROC("cons2", fth_cons_2, 3, 0, 0, h_list_cons_2);
	FTH_PROC("car", fth_car, 1, 0, 0, h_list_car);
	FTH_PROC("cadr", fth_cadr, 1, 0, 0, h_list_cdr);
	FTH_PROC("caddr", fth_caddr, 1, 0, 0, h_list_cadr);
	FTH_PROC("cadddr", fth_cadddr, 1, 0, 0, h_list_caddr);
	FTH_PROC("cdr", fth_cdr, 1, 0, 0, h_list_cdr);
	FTH_PROC("cddr", fth_cddr, 1, 0, 0, h_list_cddr);
	FTH_PRI1(".list", ficl_print_list, h_print_list);
	FTH_PRI1("list=", ficl_list_equal_p, h_list_equal_p);
	FTH_PRI1("set-car!", ficl_set_car, h_list_set_car);
	FTH_PRI1("set-cdr!", ficl_set_cdr, h_list_set_cdr);
	FTH_PROC("list->array", fth_list_to_array, 1, 0, 0, h_list_to_array);
	FTH_PROC("list-copy", fth_list_copy, 1, 0, 0, h_list_copy);
	FTH_PRI1("list-ref", ficl_list_ref, h_list_ref);
	FTH_PRI1("list-set!", ficl_list_set, h_list_set);
	FTH_PRI1("list-append", ficl_list_append, h_list_append);
	FTH_PROC("list-reverse", fth_list_reverse, 1, 0, 0, h_list_reverse);
	FTH_PROC("list-member?", fth_list_member_p, 2, 0, 0, h_list_member_p);
	FTH_PRI1("list-insert", ficl_list_insert, h_list_insert);
	FTH_PRI1("list-delete", ficl_list_delete, h_list_delete);
	FTH_PRI1("list-delete!", ficl_list_delete_bang, h_list_delete_bang);
	FTH_PRI1("list-slice", ficl_list_slice, h_list_slice);
	FTH_PRI1("list-slice!", ficl_list_slice_bang, h_list_slice_bang);
	FTH_PRI1("list-fill", ficl_list_fill, h_list_fill);
	FTH_PRI1("list-index", ficl_list_index, h_list_index);
	FTH_PRI1("list-head", ficl_list_head, h_list_head);
	FTH_PRI1("list-tail", ficl_list_tail, h_list_tail);
	FTH_PRI1("last-pair", ficl_last_pair, h_last_pair);

	/* alist */
	FTH_PRI1(">alist", ficl_values_to_alist, h_values_to_alist);
	FTH_PROC("acons", fth_acons, 3, 0, 0, h_list_acons);
	FTH_PROC("list-assoc", fth_list_assoc, 2, 0, 0, h_list_ass);
	FTH_PROC("list-assoc-ref", fth_list_assoc_ref, 2, 0, 0,
	    h_list_ass_ref);
	FTH_PROC("list-assoc-set!", fth_list_assoc_set, 3, 0, 0,
	    h_list_ass_set);
	FTH_PROC("list-assoc-remove!", fth_list_assoc_remove, 2, 0, 0,
	    h_list_ass_rem);
	FTH_ADD_FEATURE_AND_INFO(FTH_STR_LIST, h_list_of_list_functions);
}

/*
 * array.c ends here
 */
