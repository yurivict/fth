/*-
 * Copyright (c) 2012-2014 Michael Scholz <mi-scholz@users.sourceforge.net>
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
 * @(#)fth-dbm.c	1.15 1/22/14
 *
 * Commentary:
 *
 * dl-load dbm Init_dbm
 *
 * dbm.so provides the following functions:
 *
 * dbm?           ( obj -- f )
 * dbm-name       ( dbm -- name )
 * .dbm           ( dbm -- )
 * make-dbm       ( fname -- dbm )
 * dbm{}    alias for make-dbm
 * dbm-open alias for make-dbm
 * >dbm           ( key-vals len -- dbm )
 * dbm-close      ( dbm -- )
 * dbm-closed?    ( dbm -- f )
 * dbm-ref        ( dbm key -- data )
 * dbm-fetch alias for dbm-ref
 * dbm-set!       ( dbm key data -- )
 * dbm-store alias for dbm-set!
 * dbm-delete     ( dbm key -- data|#f )
 * dbm-member?    ( dbm key -- f )
 * dbm->array     ( dbm -- array )
 * dbm->hash      ( dbm -- hash )
 * hash->dbm      ( hash fname -- dbm )
 * dbm{ key-data pairs } ( fname <ccc>} -- ) "dbm-test" dbm{ 'foo 10 } value db
 *
 * and the general object functions:
 *
 * object->string
 * object-dump
 * object->array
 * object-copy
 * object-ref
 * object-length   (alias length)
 * object-apply    (alias apply)
 *
 * object-ref implies each ( #( key data ) ) ... end-each
 * db each { val }
 *   val 0 array-ref { key }
 *   val 1 array-ref { data }
 *   ...
 * end-each
 */

#if !defined(lint)
const char dbm_sccsid[] = "@(#)fth-dbm.c	1.15 1/22/14";
#endif /* not lint */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "fth.h"

#if HAVE_DBM

#if defined(HAVE_FCNTL_H)
#include <fcntl.h>
#endif
#if defined(HAVE_NDBM_H)
#include <ndbm.h>
#endif

static FTH	dbm_tag;
static int	dbm_type;

typedef struct {
	ficlInteger	length;	/* key-data pairs */
	DBM            *data;	/* DBM database */
	bool		closed;	/* flag if db is closed */
	FTH		filename;	/* db file name w/o suffix '.db' */
	FTH		array;	/* db content as array of arrays for each ...
				 * end-each */
} FDbm;

#define FTH_STR_DBM		"dbm"
#define STR_DBM_ERROR		"dbm-error"
#define FTH_DBM_ERROR		fth_exception(STR_DBM_ERROR)

#define FTH_DBM_OBJECT(Obj)	FTH_INSTANCE_REF_GEN(Obj, FDbm)
#define FTH_DBM_LENGTH(Obj)	FTH_DBM_OBJECT(Obj)->length
#define FTH_DBM_DATA(Obj)	FTH_DBM_OBJECT(Obj)->data
#define FTH_DBM_CLOSED_P(Obj)	FTH_DBM_OBJECT(Obj)->closed
#define FTH_DBM_FILENAME(Obj)	FTH_DBM_OBJECT(Obj)->filename
#define FTH_DBM_ARRAY(Obj)	FTH_DBM_OBJECT(Obj)->array

#define FTH_DBM_P(Obj)							\
	(fth_instance_p(Obj) && FTH_INSTANCE_TYPE(Obj) == (fobj_t)dbm_type)
#define FTH_DBM_NOT_CLOSED_P(Obj)					\
	(FTH_DBM_P(Obj) && !FTH_DBM_CLOSED_P(Obj))

#define FTH_DBM_THROW_ERROR(db) do {					\
	int _e;								\
									\
	_e = dbm_error(db);						\
	dbm_clearerr(db);						\
	fth_throw(FTH_DBM_ERROR, "%s (%s): %d", RUNNING_WORD(), __func__, _e);\
} while (0)

/*
 * XXX
 *   FreeBSD: int    datum.dsize
 * Others(?): size_t datum.dsize
 */

#if defined(__FreeBSD__)
#define dbm_strlen(Str)		(int)strlen(Str)
#else
#define dbm_strlen(Str)		strlen(Str)
#endif

static FTH	dbm_copy(FTH self);
static FTH	dbm_dump(FTH self);
static FTH	dbm_dump_each(datum key, datum val, FTH fs);
static void	dbm_free(FTH self);
static FTH	dbm_length(FTH self);
static void	dbm_mark(FTH self);
static FTH	dbm_ref(FTH self, FTH idx);
static FTH	dbm_set(FTH self, FTH fkey, FTH fdata);
static FTH	dbm_to_array(FTH self);
static FTH	dbm_to_array_each(datum key, datum val, FTH array);
static FTH	dbm_to_hash_each(datum key, datum val, FTH hash);
static FTH	dbm_to_string(FTH self);
static FTH	dbm_to_string_each(datum key, datum val, FTH fs);
static void	ficl_begin_dbm(ficlVm *vm);
static void	ficl_dbm_close(ficlVm *vm);
static void	ficl_dbm_closed_p(ficlVm *vm);
static void	ficl_dbm_delete(ficlVm *vm);
static void	ficl_dbm_member_p(ficlVm *vm);
static void	ficl_dbm_name(ficlVm *vm);
static void	ficl_dbm_p(ficlVm *vm);
static void	ficl_dbm_print(ficlVm *vm);
static void	ficl_dbm_ref(ficlVm *vm);
static void	ficl_dbm_set(ficlVm *vm);
static void	ficl_dbm_to_array(ficlVm *vm);
static void	ficl_dbm_to_hash(ficlVm *vm);
static void	ficl_hash_to_dbm(ficlVm *vm);
static void	ficl_make_dbm(ficlVm *vm);
static void	ficl_values_to_dbm(ficlVm *vm);
static FTH	fth_dbm_each(FTH db,
		    FTH (*func) (datum key, datum val, FTH data), FTH data);
static FTH	hash_to_dbm_each(FTH key, FTH val, FTH db);
static FTH	make_dbm(FTH name);
void		Init_dbm(void);

static FTH
make_dbm(FTH name)
{
	DBM *dbm;
	FDbm *db;
	datum k;
	char *s;

	s = fth_string_ref(name);
	dbm = dbm_open(s, O_RDWR | O_CREAT, 0660);
	if (dbm == NULL) {
		dbm = dbm_open(s, O_RDONLY, 0660);
		if (dbm == NULL)
			FTH_DBM_THROW_ERROR(dbm);
	}
	db = FTH_MALLOC(sizeof(FDbm));
	db->length = 0;
	db->data = dbm;
	db->closed = false;
	db->filename = name;
	db->array = FTH_FALSE;
	for (k = dbm_firstkey(dbm); k.dptr != NULL; k = dbm_nextkey(dbm))
		db->length++;
	return (fth_make_instance(dbm_tag, db));
}

/*
 * Loops through entire data base and calls func on every key.
 */
static FTH
fth_dbm_each(FTH db,
    FTH (*func) (datum key, datum val, FTH data),
    FTH data)
{
	DBM *d;
	datum k, v;

	FTH_ASSERT_ARGS(FTH_DBM_NOT_CLOSED_P(db), db, FTH_ARG1, "open dbm");
	d = FTH_DBM_DATA(db);
	for (k = dbm_firstkey(d); k.dptr != NULL; k = dbm_nextkey(d)) {
		v = dbm_fetch(d, k);
		data = (*func) (k, v, data);
	}
	return (data);
}

static FTH
dbm_to_string_each(datum k, datum v, FTH fs)
{
	return (fth_string_sformat(fs, " \"%.*s\" => \"%.*s\" ",
	    k.dsize, k.dptr,
	    v.dsize, v.dptr));
}

/*
 * dbm object->string => dbm{ "key" => "data"  "key" => "data" ... }
 */
static FTH
dbm_to_string(FTH self)
{
	FTH fs;

	fs = fth_make_string("dbm{");
	if (FTH_DBM_CLOSED_P(self))
		return (fth_string_scat(fs, " closed "));
	if (FTH_DBM_LENGTH(self) > 0)
		fth_dbm_each(self, dbm_to_string_each, fs);
	return (fth_string_scat(fs, "}"));
}

static FTH
dbm_dump_each(datum k, datum v, FTH fs)
{
	return (fth_string_sformat(fs, " \"%.*s\" \"%.*s\" ",
	    k.dsize, k.dptr,
	    v.dsize, v.dptr));
}

/*
 * dbm object-dump => "dbm-filename" dbm{ "key" "data"  "key" "data" ... }
 */
static FTH
dbm_dump(FTH self)
{
	FTH fs;

	fs = fth_make_string_format("\"%S\" dbm{", FTH_DBM_FILENAME(self));
	if (FTH_DBM_LENGTH(self) > 0)
		fth_dbm_each(self, dbm_dump_each, fs);
	return (fth_string_scat(fs, "}"));
}

static FTH
dbm_to_array_each(datum k, datum v, FTH array)
{
	FTH fsk, fsv;

	fsk = fth_make_string_len(k.dptr, (ficlInteger)k.dsize);
	fsv = fth_make_string_len(v.dptr, (ficlInteger)v.dsize);
	return (fth_array_push(array, FTH_LIST_2(fsk, fsv)));
}

/*
 * dbm object->array => dbm as array of arrays
 */
static FTH
dbm_to_array(FTH self)
{
	FTH array;

	array = FTH_DBM_ARRAY(self);
	if (FTH_FALSE_P(array) || FTH_INSTANCE_CHANGED_P(self)) {
		FTH a;

		a = fth_make_empty_array();
		array = fth_dbm_each(self, dbm_to_array_each, a);
		FTH_INSTANCE_CHANGED_CLR(self);
	}
	return (array);
}

static int	copy_number;

/*
 * dbm object-copy => dbm-copy with dbm-filename-0x
 */
static FTH
dbm_copy(FTH self)
{
	FTH fs, nfs, copy;
	DBM *d;
	datum k, v;

	fs = FTH_DBM_FILENAME(self);
	nfs = fth_make_string_format("%S-%02d", fs, ++copy_number);
	copy = make_dbm(nfs);
	d = FTH_DBM_DATA(self);
	for (k = dbm_firstkey(d); k.dptr != NULL; k = dbm_nextkey(d)) {
		v = dbm_fetch(d, k);
		if (dbm_store(FTH_DBM_DATA(copy), k, v, DBM_REPLACE) == -1)
			FTH_DBM_THROW_ERROR(FTH_DBM_DATA(copy));
	}
	return (copy);
}

/*
 * dbm object-ref => #( "key" "data" )
 * Returns #( "key" "data" ) pair for object-ref and each ... end-each.
 */
static FTH
dbm_ref(FTH self, FTH idx)
{
	return (fth_array_ref(dbm_to_array(self), FIX_TO_INT(idx)));
}

static FTH
dbm_set(FTH self, FTH fkey, FTH fdata)
{
	datum k, v, f;

	k.dptr = fth_to_c_string(fkey);
	k.dsize = dbm_strlen(k.dptr);
	v.dptr = fth_to_c_string(fdata);
	v.dsize = dbm_strlen(v.dptr);
	f = dbm_fetch(FTH_DBM_DATA(self), k);
	if (f.dptr == NULL)
		FTH_DBM_LENGTH(self)++;
	if (dbm_store(FTH_DBM_DATA(self), k, v, DBM_REPLACE) == -1)
		FTH_DBM_THROW_ERROR(FTH_DBM_DATA(self));
	FTH_INSTANCE_CHANGED(self);
	return (self);
}

/*
 * dbm object-length => count of key-data pairs
 */
static FTH
dbm_length(FTH self)
{
	return (INT_TO_FIX(FTH_DBM_LENGTH(self)));
}

static void
dbm_mark(FTH self)
{
	fth_gc_mark(FTH_DBM_ARRAY(self));
	fth_gc_mark(FTH_DBM_FILENAME(self));
}

static void
dbm_free(FTH self)
{
	if (FTH_DBM_OBJECT(self) == NULL)
		return;
	if (!FTH_DBM_CLOSED_P(self))
		dbm_close(FTH_DBM_DATA(self));
	FTH_FREE(FTH_DBM_OBJECT(self));
}

static void
ficl_dbm_p(ficlVm *vm)
{
#define h_dbm_p "( obj -- f )  test if OBJ is a dbm\n\
nil dbm? => #f\n\
dbm dbm? => #t\n\
Return #t if OBJ is a dbm object, otherwise #f."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, FTH_DBM_P(obj));
}

static void
ficl_dbm_name(ficlVm *vm)
{
#define h_dbm_name "( dbm -- name )  return file name of dbm\n\
dbm dbm-name => \"dbm-filename\"\n\
Return file name of DBM object."
	FTH db;

	FTH_STACK_CHECK(vm, 1, 1);
	db = fth_pop_ficl_cell(vm);
	FTH_ASSERT_ARGS(FTH_DBM_P(db), db, FTH_ARG1, "a dbm");
	ficlStackPushFTH(vm->dataStack, FTH_DBM_FILENAME(db));
}

static void
ficl_dbm_print(ficlVm *vm)
{
#define h_dbm_print "( dbm -- )  print dbm\n\
dbm .dbm\n\
Print DBM object to current output."
	FTH db;

	FTH_STACK_CHECK(vm, 1, 0);
	db = fth_pop_ficl_cell(vm);
	FTH_ASSERT_ARGS(FTH_DBM_P(db), db, FTH_ARG1, "a dbm");
	fth_print(fth_string_ref(dbm_to_string(db)));
}

static void
ficl_make_dbm(ficlVm *vm)
{
#define h_make_dbm "( fname -- dbm )  create new dbm\n\
\"dbm-name\" make-dbm value dbm\n\
Create FNAME as new database."
	FTH fs;

	FTH_STACK_CHECK(vm, 1, 1);
	fs = fth_pop_ficl_cell(vm);
	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	ficlStackPushFTH(vm->dataStack, make_dbm(fs));
}

static void
ficl_values_to_dbm(ficlVm *vm)
{
#define h_values_to_dbm "( key-vals len name -- dbm )  create dbm\n\
'foo 0  2 \"test-db\" >dbm => dbm{ \"'foo\" => \"0\" }\n\
Return a new dbm with LEN/2 key-data pairs found on stack."
	FTH key, val, fs, db;
	ficlInteger i, len;

	FTH_STACK_CHECK(vm, 2, 0);
	fs = fth_pop_ficl_cell(vm);
	len = ficlStackPopInteger(vm->dataStack);
	if (len < 0)
		FTH_OUT_OF_BOUNDS_ERROR(FTH_ARG1, len, "negative");
	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG2, "a string");
	FTH_STACK_CHECK(vm, len, 1);
	db = make_dbm(fs);
	for (i = 0; i < len; i += 2) {
		val = fth_pop_ficl_cell(vm);
		key = fth_pop_ficl_cell(vm);
		dbm_set(db, key, val);
	}
	fth_push_ficl_cell(vm, db);
}

static void
ficl_dbm_close(ficlVm *vm)
{
#define h_dbm_close "( dbm -- )  close dbm\n\
dbm dbm-close\n\
Close DBM database."
	FTH db;

	FTH_STACK_CHECK(vm, 1, 0);
	db = fth_pop_ficl_cell(vm);
	FTH_ASSERT_ARGS(FTH_DBM_P(db), db, FTH_ARG1, "a dbm");
	if (!FTH_DBM_CLOSED_P(db))
		dbm_close(FTH_DBM_DATA(db));
	FTH_DBM_CLOSED_P(db) = true;
}

static void
ficl_dbm_closed_p(ficlVm *vm)
{
#define h_dbm_closed_p "( dbm -- f )  test if dbm is closed\n\
dbm dbm-closed? => flag\n\
Return #t if DBM is closed, otherwise #f."
	FTH db;

	FTH_STACK_CHECK(vm, 1, 1);
	db = fth_pop_ficl_cell(vm);
	FTH_ASSERT_ARGS(FTH_DBM_P(db), db, FTH_ARG1, "a dbm");
	ficlStackPushBoolean(vm->dataStack, FTH_DBM_CLOSED_P(db));
}

static void
ficl_dbm_ref(ficlVm *vm)
{
#define h_dbm_ref "( dbm key -- data|#f )  return KEY's data\n\
dbm 'foo dbm-ref => \"#( 0 1 2 )\"\n\
dbm 'bar dbm-ref => #f\n\
Return data associated with KEY or #f if no key was found."
	datum k, v;
	FTH db, fkey, fdata;

	FTH_STACK_CHECK(vm, 2, 1);
	fkey = fth_pop_ficl_cell(vm);
	db = fth_pop_ficl_cell(vm);
	FTH_ASSERT_ARGS(FTH_DBM_NOT_CLOSED_P(db), db, FTH_ARG1, "open dbm");
	k.dptr = fth_to_c_string(fkey);
	k.dsize = dbm_strlen(k.dptr);
	fdata = FTH_FALSE;
	v = dbm_fetch(FTH_DBM_DATA(db), k);
	if (v.dptr != NULL)
		fdata = fth_make_string_len(v.dptr, (ficlInteger)v.dsize);
	fth_push_ficl_cell(vm, fdata);
}

static void
ficl_dbm_set(ficlVm *vm)
{
#define h_dbm_set "( dbm key data -- )  set KEY to DATA\n\
dbm 'foo #( 0 1 2 ) dbm-set!\n\
Set KEY-DATA pair."
	FTH db, fkey, fdata;

	FTH_STACK_CHECK(vm, 3, 0);
	fdata = fth_pop_ficl_cell(vm);
	fkey = fth_pop_ficl_cell(vm);
	db = fth_pop_ficl_cell(vm);
	FTH_ASSERT_ARGS(FTH_DBM_NOT_CLOSED_P(db), db, FTH_ARG1, "open dbm");
	dbm_set(db, fkey, fdata);
}

static void
ficl_dbm_delete(ficlVm *vm)
{
#define h_dbm_delete "( dbm key -- data|#f )  delete KEY-data pair\n\
dbm 'foo dbm-delete => \"#( 0 1 2 )\"\n\
dbm 'bar dbm-delete => #f\n\
Delete KEY and associated data from DBM.  \
If KEY exists, return associated data, otherwise #f."
	FTH db, fkey, fdata;
	datum k, v;

	FTH_STACK_CHECK(vm, 2, 1);
	fkey = fth_pop_ficl_cell(vm);
	db = fth_pop_ficl_cell(vm);
	FTH_ASSERT_ARGS(FTH_DBM_NOT_CLOSED_P(db), db, FTH_ARG1, "open dbm");
	k.dptr = fth_to_c_string(fkey);
	k.dsize = dbm_strlen(k.dptr);
	fdata = FTH_FALSE;
	v = dbm_fetch(FTH_DBM_DATA(db), k);
	if (v.dptr != NULL)
		fdata = fth_make_string_len(v.dptr, (ficlInteger)v.dsize);
	if (dbm_delete(FTH_DBM_DATA(db), k) == -1)
		FTH_DBM_THROW_ERROR(FTH_DBM_DATA(db));
	fth_push_ficl_cell(vm, fdata);
}

static void
ficl_dbm_member_p(ficlVm *vm)
{
#define h_dbm_member_p "( dbm key -- f )  test if KEY exist in DBM\n\
dbm 'foo dbm-member? => #t\n\
dbm 'bar dbm-member? => #f\n\
If KEY exists in DBM, return #t, otherwise #f."
	FTH db, fkey;
	datum k, v;

	FTH_STACK_CHECK(vm, 2, 1);
	fkey = fth_pop_ficl_cell(vm);
	db = fth_pop_ficl_cell(vm);
	FTH_ASSERT_ARGS(FTH_DBM_NOT_CLOSED_P(db), db, FTH_ARG1, "open dbm");
	k.dptr = fth_to_c_string(fkey);
	k.dsize = dbm_strlen(k.dptr);
	v = dbm_fetch(FTH_DBM_DATA(db), k);
	ficlStackPushBoolean(vm->dataStack, v.dptr != NULL);
}

static void
ficl_dbm_to_array(ficlVm *vm)
{
#define h_dbm_to_array "( dbm -- ary )  return DBM as array\n\
dbm dbm->array => #( #( \"'foo\" \"#( 0 1 2 )\" ) )\n\
Return an array of #( key data ) arrays."
	FTH db;

	FTH_STACK_CHECK(vm, 1, 1);
	db = fth_pop_ficl_cell(vm);
	FTH_ASSERT_ARGS(FTH_DBM_P(db), db, FTH_ARG1, "a dbm");
	ficlStackPushFTH(vm->dataStack, dbm_to_array(db));
}

static FTH
dbm_to_hash_each(datum k, datum v, FTH hash)
{
	FTH fsk, fsv;

	fsk = fth_make_string_len(k.dptr, (ficlInteger)k.dsize);
	fsv = fth_make_string_len(v.dptr, (ficlInteger)v.dsize);
	fth_hash_set(hash, fsk, fsv);
	return (hash);
}

static void
ficl_dbm_to_hash(ficlVm *vm)
{
#define h_dbm_to_hash "( dbm -- hash )  return DBM as hash\n\
dbm dbm->hash => #{ \"'foo\" => \"#( 0 1 2 )\" }\n\
Return content of DBM as a hash."
	FTH db, d;

	FTH_STACK_CHECK(vm, 1, 1);
	db = fth_pop_ficl_cell(vm);
	FTH_ASSERT_ARGS(FTH_DBM_NOT_CLOSED_P(db), db, FTH_ARG1, "open dbm");
	d = fth_dbm_each(db, dbm_to_hash_each, fth_make_hash());
	ficlStackPushFTH(vm->dataStack, d);
}

static FTH
hash_to_dbm_each(FTH key, FTH val, FTH db)
{
	return (dbm_set(db, key, val));
}

static void
ficl_hash_to_dbm(ficlVm *vm)
{
#define h_hash_to_dbm "( hash fname -- dbm )  create dbm from HASH\n\
#{ 'foo 10 'bar 20 } \"test-db\" hash->dbm =>\n\
  dbm{ \"'foo\" => \"10\"  \"'bar\" => \"20\" }\n\
Create FNAME as new database with content of HASH."
	FTH fs, hs, d;

	FTH_STACK_CHECK(vm, 2, 1);
	fs = fth_pop_ficl_cell(vm);
	hs = fth_pop_ficl_cell(vm);
	FTH_ASSERT_ARGS(FTH_HASH_P(hs), hs, FTH_ARG1, "a hash");
	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG2, "a string");
	d = fth_hash_each(hs, hash_to_dbm_each, make_dbm(fs));
	ficlStackPushFTH(vm->dataStack, d);
}

static void
ficl_begin_dbm(ficlVm *vm)
{
#define h_begin_dbm "( fname -- )  create new dbm FNAME\n\
\"new-db\" dbm{ 'foo 10 'bar 20 } =>\n\
  dbm{ \"'foo\" => \"10\"  \"'bar\" => \"20\" }\n\
Return a new dbm object."
	FTH_STACK_CHECK(vm, 1, 0);
	fth_begin_values_to_obj(vm, ">dbm", fth_pop_ficl_cell(vm));
}

void
Init_dbm(void)
{
	FTH_DBM_ERROR;
	/* filename suffix number */
	copy_number = 0;
	/* init dbm object type and add 'dbm' to *features* */
	dbm_tag = fth_make_object_type(FTH_STR_DBM);
	dbm_type = FTH_OBJECT_TYPE(dbm_tag);
	fth_set_object_to_string(dbm_tag, dbm_to_string);
	fth_set_object_dump(dbm_tag, dbm_dump);
	fth_set_object_to_array(dbm_tag, dbm_to_array);
	fth_set_object_copy(dbm_tag, dbm_copy);
	fth_set_object_value_ref(dbm_tag, dbm_ref);
	fth_set_object_length(dbm_tag, dbm_length);
	fth_set_object_mark(dbm_tag, dbm_mark);
	fth_set_object_free(dbm_tag, dbm_free);
	fth_set_object_apply(dbm_tag, (void *)dbm_ref, 1, 0, 0);
	/* dbm exception */
	fth_make_exception(STR_DBM_ERROR, "DBM error");
	/* dbm words */
	FTH_PRI1("dbm?", ficl_dbm_p, h_dbm_p);
	FTH_PRI1("dbm-name", ficl_dbm_name, h_dbm_name);
	FTH_PRI1(".dbm", ficl_dbm_print, h_dbm_print);
	FTH_PRI1("make-dbm", ficl_make_dbm, h_make_dbm);
	FTH_PRI1("dbm{}", ficl_make_dbm, h_make_dbm);
	FTH_PRI1("dbm-open", ficl_make_dbm, h_make_dbm);
	FTH_PRI1(">dbm", ficl_values_to_dbm, h_values_to_dbm);
	FTH_PRI1("dbm-close", ficl_dbm_close, h_dbm_close);
	FTH_PRI1("dbm-closed?", ficl_dbm_closed_p, h_dbm_closed_p);
	FTH_PRI1("dbm-ref", ficl_dbm_ref, h_dbm_ref);
	FTH_PRI1("dbm-fetch", ficl_dbm_ref, h_dbm_ref);
	FTH_PRI1("dbm-set!", ficl_dbm_set, h_dbm_set);
	FTH_PRI1("dbm-store", ficl_dbm_set, h_dbm_set);
	FTH_PRI1("dbm-delete", ficl_dbm_delete, h_dbm_delete);
	FTH_PRI1("dbm-member?", ficl_dbm_member_p, h_dbm_member_p);
	FTH_PRI1("dbm->array", ficl_dbm_to_array, h_dbm_to_array);
	FTH_PRI1("dbm->hash", ficl_dbm_to_hash, h_dbm_to_hash);
	FTH_PRI1("hash->dbm", ficl_hash_to_dbm, h_hash_to_dbm);
	FTH_PRI1("dbm{", ficl_begin_dbm, h_begin_dbm);
}

#endif				/* HAVE_DBM */

/*
 * fth-dbm.c ends here
 */
