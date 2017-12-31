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
 * @(#)hash.c	1.90 12/31/17
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "fth.h"
#include "utils.h"

/* === HASH === */

static FTH	hash_tag;
#define FTH_DEFAULT_HASH_SIZE	101

typedef struct FItem {
	struct FItem   *next;
	FTH		key;
	FTH		value;
} FItem;

typedef struct {
	int		hash_size;
	ficlInteger	length;
	FItem         **data;
} FHash;

#define FTH_HASH_OBJECT(Obj)	FTH_INSTANCE_REF_GEN(Obj, FHash)
#define FTH_HASH_HASH_SIZE(Obj)	FTH_HASH_OBJECT(Obj)->hash_size
#define FTH_HASH_LENGTH(Obj)	FTH_HASH_OBJECT(Obj)->length
#define FTH_HASH_DATA(Obj)	FTH_HASH_OBJECT(Obj)->data

#define hash_key_to_val(Hash, Key)					\
	((unsigned int)(fth_hash_id(Key) %				\
	    (unsigned int)FTH_HASH_HASH_SIZE(Hash)))

static void	ficl_hash_each(ficlVm *vm);
static void	ficl_hash_equal_p(ficlVm *vm);
static void	ficl_hash_map(ficlVm *vm);
static void	ficl_hash_member_p(ficlVm *vm);
static void	ficl_hash_p(ficlVm *vm);
static void	ficl_hash_print(ficlVm *vm);
static void	ficl_make_hash_with_len(ficlVm *vm);
static void	ficl_values_to_hash(ficlVm *vm);
static FTH	hs_copy(FTH self);
static FTH	hs_dump(FTH self);
static FTH	hs_dump_each(FTH key, FTH value, FTH data);
static FTH	hs_each(FTH key, FTH value, FTH data);
static FTH	hs_equal_p(FTH self, FTH obj);
static void	hs_free(FTH self);
static FTH	hs_inspect(FTH self);
static FTH	hs_inspect_each(FTH key, FTH value, FTH data);
static FTH	hs_keys_each(FTH key, FTH value, FTH obj);
static FTH	hs_length(FTH self);
static FTH	hs_map(FTH key, FTH value, FTH data);
static void	hs_mark(FTH self);
static FTH	hs_mark_each(FTH key, FTH value, FTH data);
static FTH	hs_ref(FTH self, FTH idx);
static FTH	hs_set(FTH self, FTH idx, FTH value);
static FTH	hs_to_array(FTH self);
static FTH	hs_to_array_each(FTH key, FTH value, FTH data);
static FTH	hs_to_string(FTH self);
static FTH	hs_values_each(FTH key, FTH value, FTH obj);
static FItem   *make_item(FItem *next, FTH key, FTH value);
static FHash   *make_hash(int hashsize);

#define h_list_of_hash_functions "\
*** HASH PRIMITIVES ***\n\
#{} alias for make-hash\n\
.hash               ( hash -- )\n\
>hash         	    ( len-vals len -- hash )\n\
hash->array         ( hash -- ass )\n\
hash-clear          ( hash -- )\n\
hash-copy           ( hash1 -- hash2 )\n\
hash-delete!        ( hash key -- #( key value ) )\n\
hash-each           ( hash proc -- )\n\
hash-find           ( hash key -- #( key value ) )\n\
hash-keys           ( hash -- keys )\n\
hash-map            ( hash1 proc -- hash2 )\n\
hash-member?        ( hash key -- f )\n\
hash-ref            ( hash key -- value )\n\
hash-set!           ( hash key value -- )\n\
hash-values         ( hash -- values )\n\
hash=               ( obj1 obj2 -- f )\n\
hash?               ( obj -- f )\n\
make-hash           ( -- hash )\n\
make-hash-with-len  ( size -- hash )\n\
*** Properties:\n\
object-properties   ( obj -- props )\n\
object-property-ref ( obj key -- val )\n\
object-property-set!( obj key val -- )\n\
properties          ( obj -- props )\n\
property-ref        ( obj key -- val )\n\
property-set!       ( obj key val -- )\n\
word-properties     ( xt -- props )\n\
word-property-ref   ( xt key -- val )\n\
word-property-set!  ( xt key val -- )"

/*
 * Loop through entire hash and calls func on every key-value.
 */
FTH
fth_hash_each(FTH hash,
    FTH (*func) (FTH key, FTH value, FTH data),
    FTH data)
{
	ficlInteger i;
	FItem *entry;

	FTH_ASSERT_ARGS(FTH_HASH_P(hash), hash, FTH_ARG1, "a hash");
	for (i = 0; i < FTH_HASH_HASH_SIZE(hash); i++)
		for (entry = FTH_HASH_DATA(hash)[i];
		    entry != NULL;
		    entry = entry->next)
			if (entry->key)
				data = (*func) (entry->key, entry->value, data);
	return (data);
}

/*
 * Create new hash filled with keys of original and values from FUNC.
 */
FTH
fth_hash_map(FTH hash,
    FTH (*func) (FTH key, FTH value, FTH data),
    FTH data)
{
	ficlInteger i;
	FTH hs;
	FItem *entry;

	FTH_ASSERT_ARGS(FTH_HASH_P(hash), hash, FTH_ARG1, "a hash");
	hs = fth_make_hash_len(FTH_HASH_HASH_SIZE(hash));
	for (i = 0; i < FTH_HASH_HASH_SIZE(hash); i++)
		for (entry = FTH_HASH_DATA(hash)[i];
		    entry != NULL;
		    entry = entry->next)
			if (entry->key)
				fth_hash_set(hs, entry->key,
				    (*func) (entry->key, entry->value, data));
	return (hs);
}

static FTH
hs_inspect_each(FTH key, FTH value, FTH data)
{
	return (fth_string_sformat(data, "\n%I => %I", key, value));
}

static FTH
hs_inspect(FTH self)
{
	FTH fs;

	fs = fth_make_string(FTH_INSTANCE_NAME(self));
	if (FTH_HASH_LENGTH(self) == 0)
		return (fth_string_sformat(fs, " empty"));
	fth_string_sformat(fs, "[%ld]:", FTH_HASH_LENGTH(self));
	return (fth_hash_each(self, hs_inspect_each, fs));
}

static FTH
hs_to_string(FTH self)
{
	FTH fs;

	fs = fth_make_string("#{");
	if (FTH_HASH_LENGTH(self) > 0) {
		ficlInteger i, len, n;
		FItem *entry;

		len = FTH_HASH_LENGTH(self);
		/* Negative fth_print_length shows all entries! */
		if (fth_print_length >= 0 && len > fth_print_length)
			len = FICL_MIN(len, fth_print_length);
		for (i = 0, n = 0; n < FTH_HASH_HASH_SIZE(self); n++) {
			for (entry = FTH_HASH_DATA(self)[n];
			    entry != NULL && i < len;
			    entry = entry->next)
				if (entry->key) {
					fth_string_sformat(fs, " %M => %M ",
					    entry->key, entry->value);
					i++;
				}
		}
		if (len < FTH_HASH_LENGTH(self))
			fth_string_sformat(fs, "... ");
	}
	return (fth_string_sformat(fs, "}"));
}

static FTH
hs_dump_each(FTH key, FTH value, FTH data)
{
	return (fth_string_sformat(data, " %D %D ", key, value));
}

static FTH
hs_dump(FTH self)
{
	FTH fs;

	fs = fth_make_string("#{");
	if (FTH_HASH_LENGTH(self) > 0)
		fth_hash_each(self, hs_dump_each, fs);
	return (fth_string_sformat(fs, "}"));
}

static FTH
hs_to_array_each(FTH key, FTH value, FTH data)
{
	return (fth_array_push(data, fth_make_array_var(2, key, value)));
}

static FTH
hs_to_array(FTH self)
{
	return (fth_hash_each(self, hs_to_array_each, fth_make_empty_array()));
}

static FTH
hs_copy(FTH self)
{
	FTH new;

	new = fth_make_hash_len(FTH_HASH_HASH_SIZE(self));
	if (FTH_HASH_LENGTH(self) > 0) {
		ficlInteger i;
		FItem *entry;

		for (i = 0; i < FTH_HASH_HASH_SIZE(self); i++)
			for (entry = FTH_HASH_DATA(self)[i];
			    entry != NULL;
			    entry = entry->next)
				if (entry->key)
					fth_hash_set(new,
					    fth_object_copy(entry->key),
					    fth_object_copy(entry->value));
	}
	return (new);
}

/*
 * Return #( key value ) array for object-ref and each ... end-each.
 */
static FTH
hs_ref(FTH self, FTH idx)
{
	return (fth_array_ref(fth_object_to_array(self), FTH_INT_REF(idx)));
}

/*
 * Value may #( key value ) or a single argument for object-set! and
 * map ... end-map
 */
static FTH
hs_set(FTH self, FTH idx, FTH value)
{
	if (fth_array_length(value) == 2)
		fth_hash_set(self,
		    fth_array_fast_ref(value, 0L),
		    fth_array_fast_ref(value, 1L));
	else
		fth_hash_set(self,
		    fth_array_ref(hs_ref(self, idx), 0L),
		    value);
	return (value);
}

static FTH
hs_equal_p(FTH self, FTH obj)
{
	if (FTH_HASH_HASH_SIZE(self) == FTH_HASH_HASH_SIZE(obj) &&
	    FTH_HASH_LENGTH(self) == FTH_HASH_LENGTH(obj)) {
		FItem *entry;
		FTH tmp;
		int i;

		for (i = 0; i < FTH_HASH_HASH_SIZE(self); i++) {
			for (entry = FTH_HASH_DATA(self)[i];
			    entry;
			    entry = entry->next)
				if (entry->key) {
					tmp = fth_hash_find(obj, entry->key);
					if (FTH_FALSE_P(tmp) ||
					    !fth_object_equal_p(entry->value,
					    fth_array_ref(tmp, 1L)))
						return (FTH_FALSE);
				}
		}
		return (FTH_TRUE);
	}
	return (FTH_FALSE);
}

static FTH
hs_length(FTH self)
{
	return (fth_make_int(FTH_HASH_LENGTH(self)));
}

static FTH
hs_mark_each(FTH key, FTH value, FTH data)
{
	fth_gc_mark(key);
	fth_gc_mark(value);
	return (data);
}

static void
hs_mark(FTH self)
{
	fth_hash_each(self, hs_mark_each, FTH_UNDEF);
}

static void
hs_free(FTH self)
{
	FItem *p, *entry;
	int i;

	for (i = 0; i < FTH_HASH_HASH_SIZE(self); i++) {
		entry = FTH_HASH_DATA(self)[i];
		while (entry != NULL) {
			p = entry;
			entry = entry->next;
			FTH_FREE(p);
		}
	}
	FTH_FREE(FTH_HASH_DATA(self));
	FTH_FREE(FTH_HASH_OBJECT(self));
}

static FItem *
make_item(FItem *next, FTH key, FTH value)
{
	FItem *item;

	item = FTH_MALLOC(sizeof(FItem));
	item->key = key;
	item->value = value;
	item->next = next;
	return (item);
}

static FHash *
make_hash(int hashsize)
{
	FHash *h;

	if (hashsize < 1)
		hashsize = FTH_DEFAULT_HASH_SIZE;
	h = FTH_MALLOC(sizeof(FHash));
	h->length = 0;
	h->hash_size = hashsize;
	h->data = FTH_CALLOC(h->hash_size, sizeof(FItem *));
	return (h);
}

static void
ficl_hash_p(ficlVm *vm)
{
#define h_hash_p "( obj -- f )  test if OBJ is a hash\n\
nil hash? => #f\n\
#{} hash? => #t\n\
Return #t if OBJ is a hash object, otherwise #f."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, FTH_HASH_P(obj));
}

/*
 * Don't confuse it with make-hash-with-len below!
 *
 * Return new hash object of HASHSIZE buckets.  HASHSIZE should be a
 * prime number.  If HASHSIZE is less than one, FTH_DEFAULT_HASH_SIZE
 * will be used.
 */
FTH
fth_make_hash_len(int hashsize)
{
	return (fth_make_instance(hash_tag, make_hash(hashsize)));
}

FTH
fth_make_hash(void)
{
#define h_make_hash "( -- hash )  create a hash\n\
make-hash => #{}\n\
Return empty hash object."
	return (fth_make_hash_len(FTH_DEFAULT_HASH_SIZE));
}

static void
ficl_make_hash_with_len(ficlVm *vm)
{
#define h_mhwl "( size -- hash )  create hash\n\
3 make-hash-with-len => #{ 0 => nil  1 => nil  2 => nil }\n\
Return hash object with SIZE key-value pairs.  \
Keys are 0, 1, 2, ... and values are NIL."
	FTH hash;
	ficlInteger len;

	FTH_STACK_CHECK(vm, 1, 1);
	len = ficlStackPopInteger(vm->dataStack);
	hash = fth_make_hash();
	while (len-- > 0)
		fth_hash_set(hash, fth_make_int(len), FTH_NIL);
	ficlStackPushFTH(vm->dataStack, hash);
}

static void
ficl_values_to_hash(ficlVm *vm)
{
#define h_values_to_hash "( vals len -- hash )  create hash\n\
'foo 0  'bar 1  'baz 2   6 >hash => #{ 'foo => 0  'bar => 1  'baz => 2 }\n\
Take LEN/2 key-value pairs from parameter stack and return hash object.  \
Raise OUT-OF-RANGE exception if LEN < 0 or LEN is not even."
	FTH hs, key, val;
	ficlInteger i, len;

	FTH_STACK_CHECK(vm, 1, 0);
	len = ficlStackPopInteger(vm->dataStack);
	if (len < 0)
		FTH_OUT_OF_BOUNDS_ERROR(FTH_ARGn, len, "negative");
	if (len % 2)
		FTH_OUT_OF_BOUNDS_ERROR(FTH_ARGn, len, "odd");
	FTH_STACK_CHECK(vm, len, 1);
	hs = fth_make_hash();
	len /= 2;
	for (i = 0; i < len; i++) {
		val = fth_pop_ficl_cell(vm);
		key = fth_pop_ficl_cell(vm);
		fth_hash_set(hs, key, val);
	}
	ficlStackPushFTH(vm->dataStack, hs);
}

static void
ficl_hash_print(ficlVm *vm)
{
#define h_hash_print "( hash -- )  print hash\n\
make-hash .hash\n\
Print HASH object to current output."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 0);
	obj = fth_pop_ficl_cell(vm);
	FTH_ASSERT_ARGS(FTH_HASH_P(obj), obj, FTH_ARG1, "a hash");
	fth_print(fth_string_ref(hs_to_string(obj)));
}

int
fth_hash_equal_p(FTH obj1, FTH obj2)
{
	if (FTH_HASH_P(obj1) && FTH_HASH_P(obj2))
		return (FTH_TO_BOOL(hs_equal_p(obj1, obj2)));
	return (0);
}

static void
ficl_hash_equal_p(ficlVm *vm)
{
#define h_hash_equal_p "( obj1 obj2 -- f )  compare two hashs\n\
#{ 'foo 0 'bar 1 } value h1\n\
#{ 'foo 0 'bar 1 } value h2\n\
#{ 'foo 0 }        value h3\n\
h1 h1 hash= => #t\n\
h1 h2 hash= => #t\n\
h1 h3 hash= => #f\n\
Return #t if OBJ1 and OBJ2 are hash objects \
with same length and content, otherwise #f."
	FTH obj1, obj2;

	FTH_STACK_CHECK(vm, 2, 1);
	obj2 = fth_pop_ficl_cell(vm);
	obj1 = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, fth_hash_equal_p(obj1, obj2));
}

FTH
fth_hash_copy(FTH hash)
{
#define h_hash_copy "( hash1 -- hash2 )  copy hash\n\
#{ 'foo 0 'bar 1 } value h1\n\
h1 hash-copy       value h2\n\
h1 h2 hash= => #t\n\
Return copy of HASH1 using object-copy for all elements."
	FTH_ASSERT_ARGS(FTH_HASH_P(hash), hash, FTH_ARG1, "a hash");
	return (hs_copy(hash));
}

FTH
fth_hash_ref(FTH hash, FTH key)
{
#define h_hash_ref "( hash key -- value|#f )  return hash value\n\
#{ 'foo 0 'bar 1 } value h1\n\
h1 'bar hash-ref => 1\n\
h1 'baz hash-ref => #f\n\
Return associated value or #f if not found."
	FItem *entry;
	unsigned hval;

	FTH_ASSERT_ARGS(FTH_HASH_P(hash), hash, FTH_ARG1, "a hash");
	hval = hash_key_to_val(hash, key);
	for (entry = FTH_HASH_DATA(hash)[hval];
	    entry != NULL;
	    entry = entry->next)
		if (entry->key && fth_object_equal_p(key, entry->key))
			return (entry->value);
	return (FTH_FALSE);
}

void
fth_hash_set(FTH hash, FTH key, FTH value)
{
#define h_hash_set "( hash key value -- )  set value\n\
#{ 'foo 0 'bar 1 } value h1\n\
h1 'baz 2 hash-set\n\
h1 'baz hash-ref => 2\n\
Set KEY-VALUE pair of HASH.  \
If key exists, overwrite existing value, \
otherwise create new key-value entry."
	FItem *entry;
	unsigned hval;

	FTH_ASSERT_ARGS(FTH_HASH_P(hash), hash, FTH_ARG1, "a hash");
	hval = hash_key_to_val(hash, key);
	FTH_INSTANCE_CHANGED(hash);
	for (entry = FTH_HASH_DATA(hash)[hval];
	    entry != NULL;
	    entry = entry->next)
		if (entry->key && fth_object_equal_p(key, entry->key)) {
			entry->value = value;
			return;
		}
	FTH_HASH_DATA(hash)[hval] = make_item(FTH_HASH_DATA(hash)[hval],
	    key, value);
	FTH_HASH_OBJECT(hash)->length++;
}

FTH
fth_hash_delete(FTH hash, FTH key)
{
#define h_hash_delete "( hash key -- #( key value )|#f )  delete hash key\n\
#{ 'foo 0 'bar 1 } value h1\n\
h1 'baz hash-delete => #f\n\
h1 'bar hash-delete => #( 'bar 1 )\n\
Delete key-value pair associated with KEY \
and return key-value array or #f if not found."
	FTH_ASSERT_ARGS(FTH_HASH_P(hash), hash, FTH_ARG1, "a hash");
	if (FTH_HASH_LENGTH(hash) > 0) {
		unsigned hval;
		FItem *entry, *prev;
		FTH vals;

		hval = hash_key_to_val(hash, key);

		for (prev = entry = FTH_HASH_DATA(hash)[hval];
		    entry != NULL;
		    entry = entry->next) {
			if (entry->key &&
			    fth_object_equal_p(key, entry->key)) {
				vals = FTH_LIST_2(entry->key, entry->value);

				if (entry == prev)
					FTH_HASH_DATA(hash)[hval] = entry->next;
				else
					prev->next = entry->next;

				FTH_INSTANCE_CHANGED(hash);
				FTH_FREE(entry);
				FTH_HASH_OBJECT(hash)->length--;
				return (vals);
			}
			prev = entry;
		}
	}
	return (FTH_FALSE);
}

int
fth_hash_member_p(FTH hash, FTH key)
{
	if (FTH_HASH_P(hash) && FTH_HASH_LENGTH(hash) > 0) {
		unsigned hval;
		FItem *entry;

		hval = hash_key_to_val(hash, key);
		for (entry = FTH_HASH_DATA(hash)[hval];
		    entry != NULL;
		    entry = entry->next)
			if (entry->key && fth_object_equal_p(key, entry->key))
				return (1);
	}
	return (0);
}

static void
ficl_hash_member_p(ficlVm *vm)
{
#define h_hash_member_p "( hash key -- f )  test for hash key\n\
#{ 'foo 0 'bar 1 } value h1\n\
h1 'baz hash-member? => #f\n\
h1 'bar hash-member? => #t\n\
Return #t if KEY exists, otherwise #f."
	FTH hash, key;

	FTH_STACK_CHECK(vm, 2, 1);
	key = fth_pop_ficl_cell(vm);
	hash = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, fth_hash_member_p(hash, key));
}

FTH
fth_hash_find(FTH hash, FTH key)
{
#define h_hash_find "( hash key -- #( key value )|#f )  return key-value\n\
#{ 'foo 0 'bar 1 } value h1\n\
h1 'baz hash-find => #f\n\
h1 'bar hash-find => #( 'bar 1 )\n\
Return key-value array if KEY exist or #f if not found."
	FTH_ASSERT_ARGS(FTH_HASH_P(hash), hash, FTH_ARG1, "a hash");
	if (FTH_HASH_LENGTH(hash) > 0) {
		unsigned hval;
		FItem *entry;

		hval = hash_key_to_val(hash, key);
		for (entry = FTH_HASH_DATA(hash)[hval];
		    entry != NULL;
		    entry = entry->next)
			if (entry->key && fth_object_equal_p(key, entry->key))
				return (FTH_LIST_2(entry->key, entry->value));
	}
	return (FTH_FALSE);
}

FTH
fth_hash_to_array(FTH hash)
{
#define h_hash_to_array "( hash -- ass )  return hash as array\n\
#{ 'foo 0 'bar 1 } value h1\n\
h1 hash->array => #( #( 'foo  0 ) #( 'bar  1 ) )\n\
Return array with #( key  value ) pairs of HASH's contents."
	FTH_ASSERT_ARGS(FTH_HASH_P(hash), hash, FTH_ARG1, "a hash");
	return (hs_to_array(hash));
}

/* ARGSUSED */
static FTH
hs_keys_each(FTH key, FTH value, FTH obj)
{
	(void)value;
	return (fth_array_push(obj, key));
}

FTH
fth_hash_keys(FTH hash)
{
#define h_hash_keys "( hash -- keys )  return array of keys\n\
#{ 'foo 0 'bar 1 } value h1\n\
h1 hash-keys => #( 'foo 'bar )\n\
Return array of keys."
	FTH_ASSERT_ARGS(FTH_HASH_P(hash), hash, FTH_ARG1, "a hash");
	return (fth_hash_each(hash, hs_keys_each, fth_make_empty_array()));
}

/* ARGSUSED */
static FTH
hs_values_each(FTH key, FTH value, FTH obj)
{
	(void)key;
	return (fth_array_push(obj, value));
}

FTH
fth_hash_values(FTH hash)
{
#define h_hash_values "( hash -- values )  return array of values\n\
#{ 'foo 0 'bar 1 } value h1\n\
h1 hash-values => #( 0 1 )\n\
Return array of values."
	FTH_ASSERT_ARGS(FTH_HASH_P(hash), hash, FTH_ARG1, "a hash");
	return (fth_hash_each(hash, hs_values_each, fth_make_empty_array()));
}

void
fth_hash_clear(FTH hash)
{
#define h_hash_clear "( hash -- )  clear HASH\n\
#{ 'foo 0 'bar 1 } value h1\n\
h1 hash-clear\n\
h1 .$ => #{}\n\
Remove all entries from HASH, HASH's length is zero."
	FTH_ASSERT_ARGS(FTH_HASH_P(hash), hash, FTH_ARG1, "a hash");
	if (FTH_HASH_LENGTH(hash) > 0) {
		FItem *p, *entry;
		int i;

		for (i = 0; i < FTH_HASH_HASH_SIZE(hash); i++) {
			entry = FTH_HASH_DATA(hash)[i];
			
			while (entry != NULL) {
				p = entry;
				entry = entry->next;
				FTH_FREE(p);
			}
			FTH_HASH_DATA(hash)[i] = NULL;
		}
		FTH_HASH_LENGTH(hash) = 0;
		FTH_INSTANCE_CHANGED(hash);
	}
}

static FTH
hs_each(FTH key, FTH value, FTH data)
{
	fth_proc_call(data, "hash-each", 2, key, value);
	return (data);
}

static void
ficl_hash_each(ficlVm *vm)
{
#define h_hash_each "( hash proc-or-xt -- )  run proc for each key-value\n\
#{ 'foo 0 'bar 1 } value h1\n\
h1 lambda: <{ key value -- }>\n\
  \"%s=%s\\n\" #( key value ) fth-print\n\
; hash-each\n\
Run PROC-OR-XT for each key-value pair.  \
PROC-OR-XT's stack effect must be ( key value -- ).\n\
See also hash-map."
	FTH hash, proc;

	FTH_STACK_CHECK(vm, 2, 0);
	proc = fth_pop_ficl_cell(vm);
	hash = fth_pop_ficl_cell(vm);
	proc = proc_from_proc_or_xt(proc, 2, 0, 0);
	FTH_ASSERT_ARGS(FTH_PROC_P(proc), proc, FTH_ARG2, "a proc");
	fth_hash_each(hash, hs_each, proc);
}

static FTH
hs_map(FTH key, FTH value, FTH data)
{
	return (fth_proc_call(data, "hash-map", 2, key, value));
}

static void
ficl_hash_map(ficlVm *vm)
{
#define h_hash_map "( hash1 proc-or-xt -- hash2 )  return new hash\n\
#{ 'foo 0 'bar 1 } value h1\n\
h1 lambda: <{ key value -- val }>\n\
  value 10 +\n\
; hash-map value h2\n\
h2 .$ => #{ 'foo => 10  'bar => 11 }\n\
Run PROC-OR-XT for each key-value pair.  \
PROC-OR-XT's stack effect must be ( key value -- val ) \
where VAL is the new value for key.\n\
See also hash-each."
	FTH hash, proc;

	FTH_STACK_CHECK(vm, 2, 1);
	proc = fth_pop_ficl_cell(vm);
	hash = fth_pop_ficl_cell(vm);
	proc = proc_from_proc_or_xt(proc, 2, 0, 0);
	FTH_ASSERT_ARGS(FTH_PROC_P(proc), proc, FTH_ARG2, "a proc");
	fth_push_ficl_cell(vm, fth_hash_map(hash, hs_map, proc));
}

/* === PROPERTIES === */

#define PROPERTY_IS_HASH_P	1

#if PROPERTY_IS_HASH_P
#define PROPERTY_P(Obj)		FTH_HASH_P(Obj)
#define MAKE_PROPERTY()		fth_make_hash()
#define PROPERTY_REF(Obj, Key)	fth_hash_ref(Obj, Key)
#define PROPERTY_SET(Obj, Key, Value)	fth_hash_set(Obj, Key, Value)
#else
#define PROPERTY_P(Obj)		FTH_ARRAY_P(Obj)
#define MAKE_PROPERTY()		fth_make_empty_array()
#define PROPERTY_REF(Obj, Key)	fth_array_assoc_ref(Obj, Key)
#define PROPERTY_SET(Obj, Key, Value)	fth_array_assoc_set(Obj, Key, Value)
#endif

static FTH	properties;

#define h_properties_help "\
\"hello\" value obj\n\
obj 'hey \"joe\" property-set!\n\
obj 'hey property-ref => \"joe\"\n\
obj 'joe property-ref => #f\n\
obj      properties   => #{ 'hey => \"joe\" }\n\
#f       properties   => #{ \"hello\" => #{ 'hey => \"joe\" } }"

FTH
fth_properties(FTH obj)
{
#define h_props "( obj -- props )  return global properties\n\
" h_properties_help "\n\
Return OBJ's global properties or #f if empty.  \
If OBJ is #f, return entire global property object.\n\
See also object-properties and word-properties."
	return (FTH_FALSE_P(obj) ?
	    properties :
	    fth_hash_ref(properties, obj));
}

FTH
fth_property_ref(FTH obj, FTH key)
{
#define h_prop_ref "( obj key -- value )  return value from global property\n\
" h_properties_help "\n\
Return OBJ's global property VALUE associated with KEY or #f if not found.\n\
See also object-property-ref and word-property-ref."
	FTH props;

	props = PROPERTY_REF(properties, obj);
	return (PROPERTY_P(props) ?
	    PROPERTY_REF(props, key) :
	    FTH_FALSE);
}

void
fth_property_set(FTH obj, FTH key, FTH value)
{
#define h_prop_set "( obj key value -- )  set key-value to global property\n\
" h_properties_help "\n\
Set KEY-VALUE pair to OBJ'S global property object.\n\
See also object-property-set! and word-property-set!."
	FTH props;

	props = PROPERTY_REF(properties, obj);
	if (PROPERTY_P(props))
		PROPERTY_SET(props, key, value);
	else {
		props = MAKE_PROPERTY();
		PROPERTY_SET(props, key, value);
		PROPERTY_SET(properties, obj, props);
	}
}

#define h_word_properties_help "\
<'> noop 'hey \"joe\" word-property-set!\n\
<'> noop 'hey word-property-ref => \"joe\"\n\
<'> noop 'joe word-property-ref => #f\n\
<'> noop      word-properties\n\
  => #{ 'hey => \"joe\"  'documentation => \"noop\" }"

FTH
fth_word_properties(FTH obj)
{
#define h_wprops "( xt -- props )  return word properties\n\
" h_word_properties_help "\n\
Return XT's properties or #f if empty.\n\
See also properties and object-properties."
	return (FICL_WORD_DEFINED_P(obj) ?
	    FICL_WORD_PROPERTIES(obj) :
	    FTH_FALSE);
}

FTH
fth_word_property_ref(FTH obj, FTH key)
{
#define h_wpr "( xt key -- value )  return value from word property\n\
" h_word_properties_help "\n\
Return XT's property VALUE associated with KEY or #f if not found.\n\
See also property-ref and object-property-ref."
	if (FICL_WORD_DEFINED_P(obj) && PROPERTY_P(FICL_WORD_PROPERTIES(obj)))
		return (PROPERTY_REF(FICL_WORD_PROPERTIES(obj), key));
	return (FTH_FALSE);
}

void
fth_word_property_set(FTH obj, FTH key, FTH value)
{
#define h_wps "( xt key value -- )  set key-value to word property\n\
" h_word_properties_help "\n\
Set KEY-VALUE pair to XT's property object.\n\
See also property-set! and object-property-set!."
	if (FICL_WORD_DEFINED_P(obj)) {
		if (!PROPERTY_P(FICL_WORD_PROPERTIES(obj))) {
			FTH prop = MAKE_PROPERTY();

			FICL_WORD_PROPERTIES(obj) = fth_gc_permanent(prop);
		}
		PROPERTY_SET(FICL_WORD_PROPERTIES(obj), key, value);
	}
}

#define h_object_properties_help "\
\"hello\" value obj\n\
obj 'hey \"joe\" object-property-set!\n\
obj 'hey object-property-ref => \"joe\"\n\
obj 'joe object-property-ref => #f\n\
obj      object-properties   => #{ 'hey => \"joe\" }"

FTH
fth_object_properties(FTH obj)
{
#define h_oprops "( obj -- props|#f )  return object properties\n\
" h_object_properties_help "\n\
Return OBJ's properties or #f if empty.\n\
See also properties and word-properties."
	return (fth_instance_p(obj) ?
	    FTH_INSTANCE_PROPERTIES(obj) :
	    FTH_FALSE);
}

FTH
fth_object_property_ref(FTH obj, FTH key)
{
#define hr "( obj key -- value|#f )  return property value\n\
" h_object_properties_help "\n\
Return OBJ's property VALUE associated with KEY or #f if not found.\n\
See also property-ref and word-property-ref."
	if (fth_instance_p(obj) && PROPERTY_P(FTH_INSTANCE_PROPERTIES(obj)))
		return (PROPERTY_REF(FTH_INSTANCE_PROPERTIES(obj), key));
	return (FTH_FALSE);
}

void
fth_object_property_set(FTH obj, FTH key, FTH value)
{
#define hs "( obj key value -- )  set key-value to property\n\
" h_object_properties_help "\n\
Set KEY-VALUE pair to OBJ's property object.\n\
See also property-set! and word-property-set!."
	if (fth_instance_p(obj)) {
		if (!PROPERTY_P(FTH_INSTANCE_PROPERTIES(obj)))
			FTH_INSTANCE_PROPERTIES(obj) = MAKE_PROPERTY();
		PROPERTY_SET(FTH_INSTANCE_PROPERTIES(obj), key, value);
	}
}

void
init_hash_type(void)
{
	hash_tag = make_object_type(FTH_STR_HASH, FTH_HASH_T);
	fth_set_object_inspect(hash_tag, hs_inspect);
	fth_set_object_to_string(hash_tag, hs_to_string);
	fth_set_object_dump(hash_tag, hs_dump);
	fth_set_object_to_array(hash_tag, hs_to_array);
	fth_set_object_copy(hash_tag, hs_copy);
	fth_set_object_value_ref(hash_tag, hs_ref);
	fth_set_object_value_set(hash_tag, hs_set);
	fth_set_object_equal_p(hash_tag, hs_equal_p);
	fth_set_object_length(hash_tag, hs_length);
	fth_set_object_mark(hash_tag, hs_mark);
	fth_set_object_free(hash_tag, hs_free);
}

void
init_hash(void)
{
#define FTH_VPROC FTH_VOID_PROC
	fth_set_object_apply(hash_tag, (void *)hs_ref, 1, 0, 0);
	properties = fth_gc_permanent(MAKE_PROPERTY());
	/* hash */
	FTH_PRI1("hash?", ficl_hash_p, h_hash_p);
	FTH_PROC("make-hash", fth_make_hash, 0, 0, 0, h_make_hash);
	FTH_PRI1("make-hash-with-len", ficl_make_hash_with_len, h_mhwl);
	FTH_PRI1(">hash", ficl_values_to_hash, h_values_to_hash);
	FTH_PROC("#{}", fth_make_hash, 0, 0, 0, h_make_hash);
	FTH_PRI1(".hash", ficl_hash_print, h_hash_print);
	FTH_PRI1("hash=", ficl_hash_equal_p, h_hash_equal_p);
	FTH_PROC("hash-copy", fth_hash_copy, 1, 0, 0, h_hash_copy);
	FTH_PROC("hash-ref", fth_hash_ref, 2, 0, 0, h_hash_ref);
	FTH_VPROC("hash-set!", fth_hash_set, 3, 0, 0, h_hash_set);
	FTH_PROC("hash-delete!", fth_hash_delete, 2, 0, 0, h_hash_delete);
	FTH_PRI1("hash-member?", ficl_hash_member_p, h_hash_member_p);
	FTH_PROC("hash-find", fth_hash_find, 2, 0, 0, h_hash_find);
	FTH_PROC("hash->array", fth_hash_to_array, 1, 0, 0, h_hash_to_array);
	FTH_PROC("hash-keys", fth_hash_keys, 1, 0, 0, h_hash_keys);
	FTH_PROC("hash-values", fth_hash_values, 1, 0, 0, h_hash_values);
	FTH_VPROC("hash-clear", fth_hash_clear, 1, 0, 0, h_hash_clear);
	FTH_PRI1("hash-each", ficl_hash_each, h_hash_each);
	FTH_PRI1("hash-map", ficl_hash_map, h_hash_map);
	FTH_ADD_FEATURE_AND_INFO(FTH_STR_HASH, h_list_of_hash_functions);
	/* properties */
	FTH_PROC("properties", fth_properties, 1, 0, 0, h_props);
	FTH_PROC("property-ref", fth_property_ref, 2, 0, 0, h_prop_ref);
	FTH_VPROC("property-set!", fth_property_set, 3, 0, 0, h_prop_set);
	FTH_PROC("word-properties", fth_word_properties, 1, 0, 0, h_wprops);
	FTH_PROC("word-property-ref", fth_word_property_ref, 2, 0, 0, h_wpr);
	FTH_VPROC("word-property-set!", fth_word_property_set, 3, 0, 0, h_wps);
	FTH_PROC("object-properties", fth_object_properties, 1, 0, 0, h_oprops);
	FTH_PROC("object-property-ref", fth_object_property_ref, 2, 0, 0, hr);
	FTH_VPROC("object-property-set!", fth_object_property_set, 3, 0, 0, hs);
}

/*
 * hash.c ends here
 */
