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
 * @(#)object.c	1.216 3/22/16
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "fth.h"
#include "utils.h"

#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif
#if defined(HAVE_TIME_H)
#include <time.h>
#endif

/* === OBJECT === */

typedef enum {
	OBJ_INSPECT,
	OBJ_TO_STRING,
	OBJ_DUMP
} fth_inspect_type_t;

#define FTH_INSPECT_P(Obj)	FTH_INSTANCE_REF_OBJ(Obj)->inspect
#define FTH_TO_STRING_P(Obj)	FTH_INSTANCE_REF_OBJ(Obj)->to_string
#define FTH_DUMP_P(Obj)		FTH_INSTANCE_REF_OBJ(Obj)->dump
#define FTH_TO_ARRAY_P(Obj)	FTH_INSTANCE_REF_OBJ(Obj)->to_array
#define FTH_COPY_P(Obj)		FTH_INSTANCE_REF_OBJ(Obj)->copy
#define FTH_VALUE_REF_P(Obj)	FTH_INSTANCE_REF_OBJ(Obj)->value_ref
#define FTH_VALUE_SET_P(Obj)	FTH_INSTANCE_REF_OBJ(Obj)->value_set
#define FTH_EQUAL_P_P(Obj)	FTH_INSTANCE_REF_OBJ(Obj)->equal_p
#define FTH_LENGTH_P(Obj)	FTH_INSTANCE_REF_OBJ(Obj)->length

#define FTH_INSPECT(Obj)	(*FTH_INSPECT_P(Obj))(Obj)
#define FTH_TO_STRING(Obj)	(*FTH_TO_STRING_P(Obj))(Obj)
#define FTH_DUMP(Obj)		(*FTH_DUMP_P(Obj))(Obj)
#define FTH_TO_ARRAY(Obj)	(*FTH_TO_ARRAY_P(Obj))(Obj)
#define FTH_COPY(Obj)		(*FTH_COPY_P(Obj))(Obj)
#define FTH_VALUE_REF(Obj, Idx)	(*FTH_VALUE_REF_P(Obj))(Obj, Idx)
#define FTH_VALUE_SET(Obj, Idx, Val) 					\
	(*FTH_VALUE_SET_P(Obj))(Obj, Idx, Val)
#define FTH_EQUAL_P(Obj1, Obj2)	(*FTH_EQUAL_P_P(Obj1))(Obj1, Obj2)
#define FTH_LENGTH(Obj)		(*FTH_LENGTH_P(Obj))(Obj)

#define OBJ_CHUNK_SIZE		64
#define GC_CHUNK_SIZE		(1024 * 8)
#define GC_MAX_OBJECTS		(GC_CHUNK_SIZE * 4)

#define GC_FREED		1
#define GC_MARK			2
#define GC_PROTECT		4
#define GC_PERMANENT		8

#define GC_MARK_SET(inst)	((inst)->gc_mark |=  GC_MARK)
#define GC_MARK_CLR(inst)	((inst)->gc_mark &= ~GC_MARK)
#define GC_PROTECT_SET(inst)	((inst)->gc_mark |=  GC_PROTECT)
#define GC_PROTECT_CLR(inst)	((inst)->gc_mark &= ~GC_PROTECT)
#define GC_PERMANENT_SET(inst)	((inst)->gc_mark |=  GC_PERMANENT)
#define GC_ANY_MARK_P(inst)	((inst)->gc_mark  >  GC_FREED)
#define GC_MARKED_P(inst)	((inst)->gc_mark  &  GC_MARK)
#define GC_PROTECTED_P(inst)	((inst)->gc_mark  &  GC_PROTECT)
#define GC_PERMANENT_P(inst)	((inst)->gc_mark  &  GC_PERMANENT)

#define GC_FREED_SET(inst)	((inst)->gc_mark  = GC_FREED)
#define GC_FREED_P(inst)	((inst)->gc_mark == GC_FREED)

#define gc_frame_level		FTH_FICL_VM()->gc_frame_level
#define GC_FRAME_WORD(Idx)	FTH_FICL_VM()->gc_word[Idx]
#define GC_FRAME_INST(Idx)	FTH_FICL_VM()->gc_inst[Idx]
#define GC_FRAME_CURRENT_WORD()	GC_FRAME_WORD(gc_frame_level)
#define GC_FRAME_CURRENT_INST()	GC_FRAME_INST(gc_frame_level)

static void	ficl_add_store_object(ficlVm *vm);
static void	ficl_backtrace(ficlVm *vm);
static void	ficl_boolean_p(ficlVm *vm);
static void	ficl_cycle_pos_0(ficlVm *vm);
static void	ficl_cycle_pos_ref(ficlVm *vm);
static void	ficl_cycle_pos_set(ficlVm *vm);
static void	ficl_div_store_object(ficlVm *vm);
static void	ficl_false_p(ficlVm *vm);
static void	ficl_first_object_ref(ficlVm *vm);
static void	ficl_first_object_set(ficlVm *vm);
static void	ficl_frame_depth(ficlVm *vm);
static void	ficl_gc_mark(ficlVm *vm);
static void	ficl_gc_marked_p(ficlVm *vm);
static void	ficl_gc_permanent_objects(ficlVm *vm);
static void	ficl_gc_permanent_p(ficlVm *vm);
static void	ficl_gc_protected_objects(ficlVm *vm);
static void	ficl_gc_protected_p(ficlVm *vm);
static void	ficl_gc_run(ficlVm *vm);
static void	ficl_gc_stats(ficlVm *vm);
static void	ficl_gc_unmark(ficlVm *vm);
static void	ficl_instance_gen_ref(ficlVm *vm);
static void	ficl_instance_obj_ref(ficlVm *vm);
static void	ficl_instance_p(ficlVm *vm);
static void	ficl_last_object_ref(ficlVm *vm);
static void	ficl_last_object_set(ficlVm *vm);
static void	ficl_make_instance(ficlVm *vm);
static void	ficl_make_object_type(ficlVm *vm);
static void	ficl_mul_store_object(ficlVm *vm);
static void	ficl_nil_p(ficlVm *vm);
static void	ficl_object_cycle_set(ficlVm *vm);
static void	ficl_object_debug_hook(ficlVm *vm);
static void	ficl_object_empty_p(ficlVm *vm);
static void	ficl_object_equal_p(ficlVm *vm);
static void	ficl_object_is_instance_of(ficlVm *vm);
static void	ficl_object_length(ficlVm *vm);
static void	ficl_object_member_p(ficlVm *vm);
static void	ficl_object_name(ficlVm *vm);
static void	ficl_object_range_p(ficlVm *vm);
static void	ficl_object_ref(ficlVm *vm);
static void	ficl_object_set(ficlVm *vm);
static void	ficl_object_type_p(ficlVm *vm);
static void	ficl_object_type_ref(ficlVm *vm);
static void	ficl_object_types(ficlVm *vm);
static void	ficl_print_inspect(ficlVm *vm);
static void	ficl_print_length(ficlVm *vm);
static void	ficl_print_object_name(ficlVm *vm);
static void	ficl_second_object_ref(ficlVm *vm);
static void	ficl_second_object_set(ficlVm *vm);
static void	ficl_set_apply(ficlVm *vm);
static void	ficl_set_print_length(ficlVm *vm);
static void	ficl_sub_store_object(ficlVm *vm);
static void	ficl_third_object_ref(ficlVm *vm);
static void	ficl_third_object_set(ficlVm *vm);
static void	ficl_true_p(ficlVm *vm);
static void	ficl_undef_p(ficlVm *vm);
static void	ficl_xmobj_p(ficlVm *vm);
static FInstance *gc_next_instance(void);
static FInstance *gc_run(void);
static FTH	print_object(FTH obj, fth_inspect_type_t type);
static bool	xmobj_p(FTH obj);
static FTH	xmobj_to_string(FTH obj);

#define h_list_of_object_functions "\
*** OBJECT PRIMITIVES ***\n\
backtrace           ( -- )\n\
bt alias for backtrace\n\
frame-depth         ( -- n )\n\
object-print-length ( -- n )\n\
set-object-print-length ( n -- )\n\
stack-level alias for frame-depth\n\
*** garbage collection:\n\
gc-mark             ( obj -- obj )\n\
gc-marked?          ( obj -- f )\n\
gc-off              ( -- )\n\
gc-on               ( -- )\n\
gc-permanent-objects ( -- ary )\n\
gc-permanent?       ( obj -- f )\n\
gc-protect          ( obj -- obj )\n\
gc-protected-objects ( -- ary )\n\
gc-protected?       ( obj -- f )\n\
gc-run              ( -- )\n\
gc-stats            ( -- )\n\
gc-unmark           ( obj -- obj )\n\
gc-unprotect        ( obj -- obj )\n\
*** object type and instance words:\n\
instance-gen-ref    ( obj -- gen )\n\
instance-obj-ref    ( obj -- gen )\n\
instance-of?        ( obj type -- f )\n\
instance?           ( obj -- f )\n\
make-instance       ( gen obj -- instance )\n\
make-object-type    ( name -- object-type )\n\
object-type-ref     ( object-type -- struct )\n\
object-type?        ( obj -- f )\n\
object-types        ( -- ary )\n\
*** object set words:\n\
set-object->array   ( xt obj -- )\n\
set-object->string  ( xt obj -- )\n\
set-object-apply    ( xt obj arity -- )\n\
set-object-copy     ( xt obj -- )\n\
set-object-dump     ( xt obj -- )\n\
set-object-equal-p  ( xt obj -- )\n\
set-object-free     ( xt obj -- )\n\
set-object-inspect  ( xt obj -- )\n\
set-object-length   ( xt obj -- )\n\
set-object-mark     ( xt obj -- )\n\
set-object-value-ref ( xt obj -- )\n\
set-object-value-set ( xt obj -- )\n\
*** general object words:\n\
.inspect            ( obj -- )\n\
.object-name        ( obj -- )\n\
apply alias for object-apply\n\
cycle-ref           ( obj -- val )\n\
cycle-set!          ( obj val -- )\n\
cycle-start!        ( obj index -- )\n\
cycle-start0        ( obj -- )\n\
cycle-start@        ( obj -- index )\n\
detect alias for object-find\n\
empty? alias for object-empty?\n\
equal? alias for object-equal?\n\
first-ref           ( obj -- val )\n\
first-set!          ( obj val -- )\n\
hash-id             ( obj -- id )\n\
index alias for object-index\n\
last-ref            ( obj -- val )\n\
last-set!           ( obj val -- )\n\
length alias for object-length\n\
member? alias for object-member?\n\
object->array       ( obj -- ary )\n\
object->string      ( obj -- str )\n\
object-apply        ( obj args -- result )\n\
object-copy         ( obj1 -- obj2 )\n\
object-debug-hook   ( obj -- hook|#f )\n\
object-dump         ( obj -- str )\n\
object-empty?       ( obj -- f )\n\
object-equal?       ( obj1 obj2 -- f )\n\
object-find         ( obj key -- value|#f )\n\
object-id           ( obj -- id )\n\
object-index        ( obj key -- index|-1 )\n\
object-inspect      ( obj -- str )\n\
object-length       ( obj -- len )\n\
object-member?      ( obj key -- f )\n\
object-name         ( obj -- name )\n\
object-range?       ( obj index -- f )\n\
object-ref          ( obj index -- val )\n\
object-set!         ( obj index value -- )\n\
object-set*!        ( obj index value -- )\n\
object-set+!        ( obj index value -- )\n\
object-set-!        ( obj index value -- )\n\
object-set/!        ( obj index value -- )\n\
object-sort         ( obj cmp-xt -- ary )\n\
range? alias for object-range?\n\
second-ref          ( obj -- val )\n\
second-set!         ( obj val -- )\n\
sort alias for object-sort\n\
third-ref           ( obj -- val )\n\
third-set!          ( obj val -- )\n\
xmobj?              ( obj -- f )\n\
*** predicates:\n\
boolean?            ( obj -- f )\n\
false?              ( obj -- f )\n\
nil?                ( obj -- f )\n\
true?               ( obj -- f )\n\
undef?              ( obj -- f )"

void
gc_push(ficlWord *word)
{
	if (++gc_frame_level >= GC_FRAME_SIZE) {
#if defined(FTH_DEBUG)
		fprintf(stderr, "#<GC_FRAME (gc_push): above max?>\n");
#endif
		gc_frame_level = GC_FRAME_SIZE - 1;
	}
	GC_FRAME_CURRENT_WORD() = word;
	GC_FRAME_CURRENT_INST() = NULL;
}

void
gc_pop(void)
{
	if (--gc_frame_level < 0) {
#if defined(FTH_DEBUG)
		fprintf(stderr, "#<GC_FRAME (gc_pop): below zero?>\n");
#endif
		gc_frame_level = 0;
	}
}

void
gc_loop_reset(void)
{
	GC_FRAME_CURRENT_INST() = NULL;
}

void
init_gc(void)
{
	int i;

	for (i = 0; i < GC_FRAME_SIZE; i++)
		GC_FRAME_INST(i) = NULL;
	GC_FRAME_WORD(0) = NULL;
	gc_frame_level = 0;
}

static void
ficl_frame_depth(ficlVm *vm)
{
#define h_frame_depth "( -- n )  return frame depth\n\
frame-depth => 0\n\
Internal global variable.  \
Return the current frame depth."
	ficlStackPushInteger(vm->dataStack, (ficlInteger)gc_frame_level);
}

static simple_array *last_frames = NULL;

void
fth_set_backtrace(FTH exception)
{
	char *exc;
	int i;
	ficlVm *vm;
	ficlWord *word;
	FTH fs;

	vm = FTH_FICL_VM();
	if (vm->state == FICL_VM_STATE_COMPILE)
		return;
	exc = fth_exception_ref(exception);
	word = vm->runningWord;
	if (last_frames == NULL)
		last_frames = make_simple_array(16);
	else
		simple_array_clear(last_frames);
	if (exc == NULL)
		exc = "break";
	/*
	 * First line: repeat exception
	 */
	fs = fth_make_string(exc);
	if (FICL_WORD_P(word))
		fth_string_sformat(fs, " in %s", FICL_WORD_NAME(word));
	simple_array_push(last_frames, (void *)fs);
	/*
	 * Second line: last line from terminal input buffer (TIB)
	 */
	if (vm->tib.text == NULL)
		fs = fth_make_string("empty");
	else
		fs = fth_string_chomp(fth_make_string(vm->tib.text));
	simple_array_push(last_frames, (void *)fs);
	/*
	 * Rest: last called words
	 */
	for (i = 0; i < 20 && word != NULL; i++) {
		fs = fth_word_inspect((FTH)word);
		simple_array_push(last_frames, (void *)fs);
		if (word == word->current_word)
			break;
		word = word->current_word;
	}
}

void
fth_show_backtrace(bool verbose)
{
	int len, i;
	FTH fs;

	len = simple_array_length(last_frames);
	if (len <= 0)
		return;
	i = 0;
	/*
	 * First line: repeat exception
	 */
	fs = (FTH)simple_array_ref(last_frames, i++);
	fth_errorf("#<bt: %S>\n", fs);
	/*
	 * Second line: last line from terminal input buffer (TIB)
	 */
	fs = (FTH)simple_array_ref(last_frames, i++);
	fth_errorf("#<bt: TIB %S>\n", fs);
	if (!verbose && !FTH_TRUE_P(fth_variable_ref("*fth-verbose*")))
		return;
	/*
	 * Rest: last called words
	 */
	for (; i < len; i++) {
		fs = (FTH)simple_array_ref(last_frames, i);
		fth_errorf("#<bt[%d]: %S>\n", i - 2, fs);
	}
}

static void
ficl_backtrace(ficlVm *vm)
{
#define h_backtrace "( -- )  print simple backtrace\n\
backtrace =>\n\
#<break in backtrace>\n\
#<TIB: backtrace >\n\
#<bt 0: at repl-eval:2>\n\
Print last word list from stack frame to error output.\n\
TIB: last line in Terminal Input Buffer before backtrace call\n\
 BT: level of backtrace, here 0\n\
 AT: filename:line number, source filename and line number of definition.  \
filename here is the REPL."
	FTH_STACK_CHECK(vm, 0, 0);
	if (last_frames == NULL)
		fth_set_backtrace(fth_last_exception);
	fth_show_backtrace(true);
}

static void
ficl_print_length(ficlVm *vm)
{
#define h_print_length "( -- n )  return object print length\n\
object-print-length => 12\n\
Return the number of objects to print for objects like array, list, hash.  \
Default value is 12.\n\
See also set-object-print-length."
	FTH_STACK_CHECK(vm, 0, 1);
	ficlStackPushInteger(vm->dataStack, (ficlInteger)fth_print_length);
}

static void
ficl_set_print_length(ficlVm *vm)
{
#define h_set_print_length "( n -- )  set object print length\n\
128 set-object-print-length\n\
Set number of objects to print for objects like array, list, hash to N.  \
If N is negative, print all elements of a given object.\n\
See also object-print-length."
	FTH_STACK_CHECK(vm, 1, 0);
	fth_print_length = (int)ficlStackPopInteger(vm->dataStack);
}

/* === GC === */

static FObject *obj_minmem = (void *)1;
static FObject *obj_maxmem = NULL;
static FObject **obj_types;
static int	last_object = 0;

static FInstance *inst_free_list = NULL;
static FInstance *inst_minmem = (void *)1;
static FInstance *inst_maxmem = NULL;
static FInstance **instances;
static int	last_instance = 0;

#define OBJECT_P(Obj)		(((FTH)(Obj)) & ~FTH_NIL)

#define OBJECT_TYPE_P(Obj)						\
	(FTH_OBJECT_REF(Obj) >= obj_minmem &&				\
	 FTH_OBJECT_REF(Obj) <= obj_maxmem)

#define INSTANCE_P(Obj)							\
	(FTH_INSTANCE_REF(Obj) >= inst_minmem &&			\
	 FTH_INSTANCE_REF(Obj) <= inst_maxmem &&			\
	 OBJECT_TYPE_P(FTH_INSTANCE_REF_OBJ(Obj)) &&			\
	 !GC_FREED_P(FTH_INSTANCE_REF(Obj)))

#define OBJECT_MARK(Inst)						\
	if ((Inst)->obj->mark)						\
		(*(Inst)->obj->mark)((FTH)(Inst))

#define OBJECT_FREE(Inst) do {						\
	if ((Inst)->obj->free)						\
		(*(Inst)->obj->free)((FTH)(Inst));			\
	else								\
		FTH_FREE((Inst)->gen);					\
									\
	GC_FREED_SET(Inst);						\
	(Inst)->gen = NULL;						\
	(Inst)->obj = NULL;						\
	(Inst)->properties = 0;						\
	(Inst)->values = 0;						\
	(Inst)->next = inst_free_list;					\
	inst_free_list = (Inst);					\
} while (0)

#if 0
#define FTH_DEBUG true
#endif

static FInstance *
gc_run(void)
{
	int i, freed;
	FInstance *inst, *free_inst = NULL;
	ficlStack *stack;
#if defined(FTH_DEBUG)
	int stk_marked, frm_marked;

	stk_marked = frm_marked = 0;
	fprintf(stderr, "\\ gc[%02d:%06d]: marking ... ",
	    gc_frame_level, last_instance);
#endif
	freed = 0;
	stack = FTH_FICL_STACK();

	/* Mark possible instances on stack. */
	for (i = -((int)(stack->top - stack->base)); i <= 0; i++) {
		inst = FTH_INSTANCE_REF(STACK_FTH_INDEX_REF(stack, i));

		if (INSTANCE_P(inst)) {
#if defined(FTH_DEBUG)
			stk_marked++;
#endif
			GC_MARK_SET(inst);
		}
	}

	/* Mark collected instances. */
	for (i = 0; i <= gc_frame_level; i++)
		for (inst = GC_FRAME_INST(i); inst; inst = inst->next) {
#if defined(FTH_DEBUG)
			frm_marked++;
#endif
			GC_MARK_SET(inst);
		}

	/* Mark elements of already marked sequences (array etc). */
	for (i = 1, inst = instances[0];
	    i < last_instance;
	    inst = instances[i++])
		if (GC_ANY_MARK_P(inst) && inst->obj->mark)
			(*inst->obj->mark) ((FTH)inst);
#if defined(FTH_DEBUG)
	if (stk_marked)
		fprintf(stderr, "(stack %d) ", stk_marked);
	if (frm_marked)
		fprintf(stderr, "(frame %d) ", frm_marked);
	fprintf(stderr, "done (%d)\n", stk_marked + frm_marked);
	fprintf(stderr, "\\ gc[%02d:%06d]: freeing ... ",
	    gc_frame_level, last_instance);
#endif
	/* Free all unmarked instances. */
	for (i = 1, inst = instances[0];
	    i < last_instance;
	    inst = instances[i++]) {
		if (GC_FREED_P(inst))
			continue;
		if (GC_ANY_MARK_P(inst)) {
			GC_MARK_CLR(inst);
			continue;
		}
		OBJECT_FREE(inst);
		freed++;
	}
#if defined(FTH_DEBUG)
	fprintf(stderr, "done (%d)\n", freed);
#endif
	free_inst = inst_free_list;

	if (freed > GC_CHUNK_SIZE && free_inst)
		inst_free_list = inst_free_list->next;
	else
		free_inst = NULL;
	return (free_inst);
}

void
gc_free_all(void)
{
	int i, last;

	simple_array_free(last_frames);
	if (instances != NULL) {
		for (i = 0; i < last_instance; i++) {
			if (!GC_FREED_P(instances[i]))
				OBJECT_FREE(instances[i]);
			FTH_FREE(instances[i]);
		}
		if (last_instance % GC_CHUNK_SIZE != 0)
			last = (last_instance / GC_CHUNK_SIZE) + 1;
		else
			last = last_instance / GC_CHUNK_SIZE;
		last *= GC_CHUNK_SIZE;
		for (i = last_instance; i < last; i++)
			FTH_FREE(instances[i]);
		FTH_FREE(instances);
	}
	if (obj_types != NULL) {
		if (last_object % OBJ_CHUNK_SIZE != 0)
			last = (last_object / OBJ_CHUNK_SIZE) + 1;
		else
			last = last_object / OBJ_CHUNK_SIZE;
		last *= OBJ_CHUNK_SIZE;
		for (i = 0; i < last; i++)
			FTH_FREE(obj_types[i]);
		FTH_FREE(obj_types);
	}
}

/* ARGSUSED */
static void
ficl_gc_run(ficlVm *vm)
{
#define h_gc_run "( -- )  run gc\n\
gc-run\n\
Run garbage collection immediately.\n\
See also gc-stats."
	(void)vm;
	gc_run();
}

static bool	fth_gc_on_p;

/* ARGSUSED */
static void
ficl_gc_stats(ficlVm *vm)
{
#define h_gc_stats "( -- )  print gc stats\n\
gc-stats =>\n\
\\ gc-stats (gc on):\n\
\\ permanent:   3629\n\
\\ protected:     51\n\
\\    marked:     33\n\
\\     freed:      9\n\
\\     insts:  53895\n\
\\    buffer:  57617\n\
\\  gc stack:      0\n\
Print garbage collection statistics.\n\
PERMANENT: permanent protected objects like constants\n\
PROTECTED: temporary protected objects like gc-protected\n\
   MARKED: marked to protect from next freeing\n\
    FREED: freed objects\n\
    INSTS: all other nonfreed objects\n\
   BUFFER: size of entire allocated buffer-array\n\
 GC STACK: stack frame level\n\
See also gc-run."
	int i, permanent, protected, marked, freed, rest;
	FInstance *inst;

	(void)vm;
	permanent = protected = marked = freed = rest = 0;
	for (i = 1, inst = instances[0];
	    i < last_instance;
	    inst = instances[i++]) {
		if (GC_MARKED_P(inst))
			marked++;
		else if (GC_PERMANENT_P(inst))
			permanent++;
		else if (GC_PROTECTED_P(inst))
			protected++;
		else if (GC_FREED_P(inst))
			freed++;
		else if (INSTANCE_P(inst))
			rest++;
	}
	fth_printf("\\ %s (gc %s):\n", RUNNING_WORD(),
	    fth_gc_on_p ? "on" : "off");
	fth_printf("\\ permanent: %6d\n", permanent);
	fth_printf("\\ protected: %6d\n", protected);
	fth_printf("\\    marked: %6d\n", marked);
	fth_printf("\\     freed: %6d\n", freed);
	fth_printf("\\     insts: %6d\n", rest);
	fth_printf("\\    buffer: %6d\n", last_instance - 1);
	fth_printf("\\  gc stack: %6d", gc_frame_level);
	if (CELL_INT_REF(&FTH_FICL_VM()->sourceId))
		fth_print("\n");
}

static void
ficl_gc_marked_p(ficlVm *vm)
{
#define h_gc_marked_p "( obj -- f )  test if OBJ is gc-marked\n\
#( 0 1 2 ) value a1\n\
a1 gc-marked? => #t\n\
Return #t if OBJ is an instance and mark flag is set.  \
All new created objects have mark flag set.\n\
See also gc-protected? and gc-permanent?."
	FInstance *inst;
	bool flag;

	FTH_STACK_CHECK(vm, 1, 1);
	inst = ficlStackPopPointer(vm->dataStack);
	flag = INSTANCE_P(inst) && GC_MARKED_P(inst);
	ficlStackPushBoolean(vm->dataStack, flag);
}

static void
ficl_gc_protected_p(ficlVm *vm)
{
#define h_gc_protected_p "( obj -- f )  test if OBJ is gc-protected\n\
#( 0 1 2 ) value a1\n\
a1 gc-protected? => #f\n\
a1 gc-protect drop\n\
a1 gc-protected? => #t\n\
a1 gc-unprotect drop\n\
a1 gc-protected? => #f\n\
Return #t if OBJ is an instance and protected flag is set.\n\
See also gc-marked? and gc-permanent?."
	FInstance *inst;
	bool flag;

	FTH_STACK_CHECK(vm, 1, 1);
	inst = ficlStackPopPointer(vm->dataStack);
	flag = INSTANCE_P(inst) && GC_PROTECTED_P(inst);
	ficlStackPushBoolean(vm->dataStack, flag);
}

static void
ficl_gc_permanent_p(ficlVm *vm)
{
#define h_gc_permanent_p "( obj -- f )  test if OBJ is permanent\n\
#( 0 1 2 ) value a1\n\
a1 gc-permanent? => #f\n\
#( 0 1 2 ) constant a2\n\
a2 gc-permanent? => #t\n\
Return #t if OBJ is an instance and permanent flag is set like constants.\n\
See also gc-marked? and gc-protected?."
	FInstance *inst;
	bool flag;

	FTH_STACK_CHECK(vm, 1, 1);
	inst = ficlStackPopPointer(vm->dataStack);
	flag = INSTANCE_P(inst) && GC_PERMANENT_P(inst);
	ficlStackPushBoolean(vm->dataStack, flag);
}

static void
ficl_gc_protected_objects(ficlVm *vm)
{
#define h_gc_protected_objects "( -- ary )  return all protected objects\n\
gc-protected-objects length => 55\n\
Return array of all protected objects.\n\
See also gc-permanent-objects."
	FInstance *inst;
	FTH ary;
	int i;

	ary = fth_make_empty_array();
	for (i = 1, inst = instances[0];
	    i < last_instance;
	    inst = instances[i++])
		if (GC_PROTECTED_P(inst))
			fth_array_push(ary, (FTH)inst);
	ficlStackPushFTH(vm->dataStack, ary);
}

static void
ficl_gc_permanent_objects(ficlVm *vm)
{
#define h_gc_permanent_objects "( -- ary )  return all permanent objects\n\
gc-permanent-objects length => 3634\n\
Return array of all permanent objects.\n\
See also gc-protected-objects."
	FInstance *inst;
	FTH ary;
	int i;

	ary = fth_make_empty_array();
	for (i = 1, inst = instances[0];
	    i < last_instance;
	    inst = instances[i++])
		if (GC_PERMANENT_P(inst))
			fth_array_push(ary, (FTH)inst);
	ficlStackPushFTH(vm->dataStack, ary);
}

/* gc-on, gc-off are names in snd-xen.c (but they do the same) */
FTH
fth_gc_on(void)
{
#define h_gc_on "( -- #f )  turn gc on\n\
gc-on => #f\n\
Turn on garbage collection.  \
The return code is meaningless in Forth.\n\
See also gc-off."
	fth_gc_on_p = true;
	return (FTH_FALSE);
}

FTH
fth_gc_off(void)
{
#define h_gc_off "( -- #f )  turn gc off\n\
gc-off => #f\n\
Turn off garbage collection.  \
The return code is meaningless in Forth.\n\
See also gc-on."
	fth_gc_on_p = false;
	return (FTH_FALSE);
}

void
fth_gc_mark(FTH obj)
{
	if (INSTANCE_P(obj)) {
		GC_MARK_SET(FTH_INSTANCE_REF(obj));
		OBJECT_MARK(FTH_INSTANCE_REF(obj));
	}
}

static void
ficl_gc_mark(ficlVm *vm)
{
#define h_gc_mark "( obj -- obj )  set gc-mark flag\n\
#( 0 1 2 ) value a1\n\
a1 gc-mark\n\
a1 gc-marked => #t\n\
Mark OBJ to protect it from garbage collection on next gc-run.\n\
See also gc-unmark."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = ficlStackPopFTH(vm->dataStack);
	if (INSTANCE_P(obj))
		GC_MARK_SET(FTH_INSTANCE_REF(obj));
	ficlStackPushFTH(vm->dataStack, obj);
}

void
fth_gc_unmark(FTH obj)
{
	if (INSTANCE_P(obj))
		GC_MARK_CLR(FTH_INSTANCE_REF(obj));
}

static void
ficl_gc_unmark(ficlVm *vm)
{
#define h_gc_unmark "( obj -- obj )  clear gc-mark flag\n\
#( 0 1 2 ) value a1\n\
a1 gc-marked => #t\n\
a1 gc-unmark\n\
a1 gc-marked => #f\n\
Unmark OBJ to unprotect it from garbage collection on next gc-run.\n\
See also gc-mark."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = ficlStackPopFTH(vm->dataStack);
	if (INSTANCE_P(obj))
		GC_MARK_CLR(FTH_INSTANCE_REF(obj));
	ficlStackPushFTH(vm->dataStack, obj);
}

/*
 * Protect OBJ from garbage collection until fth_gc_unprotect.
 */
FTH
fth_gc_protect(FTH obj)
{
#define h_gc_protect "( obj -- obj )  set gc-protect flag\n\
#( 0 1 2 ) value a1\n\
a1 gc-protect drop\n\
a1 gc-protected? => #t\n\
Protect OBJ from garbage collection until gc-unprotect.\n\
See also gc-unprotect."
	if (INSTANCE_P(obj))
		GC_PROTECT_SET(FTH_INSTANCE_REF(obj));
	return (obj);
}

/*
 * Unprotect OBJ from garbage collection.
 */
FTH
fth_gc_unprotect(FTH obj)
{
#define h_gc_unprotect "( obj -- obj )  clear gc-protect flag\n\
#( 0 1 2 ) value a1\n\
a1 gc-protect drop\n\
a1 gc-protected? => #t\n\
a1 gc-unprotect drop\n\
a1 gc-protected? => #f\n\
Unprotect OBJ from garbage collection.\n\
See also gc-protect."
	if (INSTANCE_P(obj))
		GC_PROTECT_CLR(FTH_INSTANCE_REF(obj));
	return (obj);
}

FTH
fth_gc_protect_set(FTH out, FTH in)
{
	if (INSTANCE_P(out))
		GC_PROTECT_CLR(FTH_INSTANCE_REF(out));
	if (INSTANCE_P(in))
		GC_PROTECT_SET(FTH_INSTANCE_REF(in));
	return (in);
}

FTH
fth_gc_permanent(FTH obj)
{
	if (INSTANCE_P(obj))
		GC_PERMANENT_SET(FTH_INSTANCE_REF(obj));
	return (obj);
}

/* === OBJECT-TYPE === */

bool
fth_object_type_p(FTH obj)
{
	return (OBJECT_TYPE_P(obj));
}

static void
ficl_object_type_p(ficlVm *vm)
{
#define h_object_type_p "( obj -- f )  test if OBJ is an Object type\n\
\"enved\" make-object-type object-type? => #t\n\
Return #t if OBJ is an Object type, otherwise #f.\n\
See also make-object-type."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = ficlStackPopFTH(vm->dataStack);
	ficlStackPushBoolean(vm->dataStack, OBJECT_TYPE_P(obj));
}

FTH
make_object_type(const char *name, fobj_t type)
{
	FObject *current;

	FTH_ASSERT_STRING(name);
	if (last_object % OBJ_CHUNK_SIZE == 0) {
		int i, size;

		size = OBJ_CHUNK_SIZE + last_object;
		obj_types = FTH_REALLOC(obj_types,
		    sizeof(FObject *) * (size_t)size);
		for (i = last_object; i < size; i++)
			obj_types[i] = FTH_CALLOC(1, sizeof(FObject));
		if (last_object == 0)
			obj_minmem = *obj_types;
	}
	current = obj_types[last_object++];
	if (obj_minmem > current)
		obj_minmem = current;
	if (obj_maxmem < current)
		obj_maxmem = current;
	current->type = type;
	fth_strcpy(current->name, sizeof(current->name), name);
	/*-
	 * constant with class definition, e.g. "fth-Mus":
	 * 10 make-oscil  fth-Mus   instance-of? => #t
	 * 10 make-array  fth-Mus   instance-of? => #f
	 * 10 make-array  fth-array instance-of? => #t
	 */
	fth_strcpy(FTH_FICL_VM()->pad, sizeof(FTH_FICL_VM()->pad), "fth-");
	fth_strcat(FTH_FICL_VM()->pad, sizeof(FTH_FICL_VM()->pad), name);
	ficlDictionaryAppendConstant(FTH_FICL_DICT(),
	    FTH_FICL_VM()->pad, (ficlInteger)current);
	return ((FTH)current);
}

FTH
make_object_type_from(const char *name, fobj_t type, FTH obj)
{
	FTH new;
	FObject *current, *base;

	new = make_object_type(name, type);
	current = FTH_OBJECT_REF(new);
	base = FTH_OBJECT_REF(obj);
	current->inspect = base->inspect;
	current->to_string = base->to_string;
	current->dump = base->dump;
	current->to_array = base->to_array;
	current->copy = base->copy;
	current->value_ref = base->value_ref;
	current->value_set = base->value_set;
	current->equal_p = base->equal_p;
	current->length = base->length;
	current->mark = base->mark;
	current->free = base->free;
	current->inspect_proc = base->inspect_proc;
	current->to_string_proc = base->to_string_proc;
	current->dump_proc = base->dump_proc;
	current->to_array_proc = base->to_array_proc;
	current->copy_proc = base->copy_proc;
	current->value_ref_proc = base->value_ref_proc;
	current->value_set_proc = base->value_set_proc;
	current->equal_p_proc = base->equal_p_proc;
	current->length_proc = base->length_proc;
	current->mark_proc = base->mark_proc;
	current->free_proc = base->free_proc;
	return (new);
}

static fobj_t		object_type_counter = FTH_LAST_ENTRY_T;

/*
 * Add NAME to feature environment list, create a constant "fth-NAME"
 * of object-type and return new object-type NAME.
 */
FTH
fth_make_object_type(const char *name)
{
	fth_add_feature(name);
	return (make_object_type(name, object_type_counter++));
}

FTH
fth_make_object_type_from(const char *name, FTH base)
{
	fth_add_feature(name);
	return (make_object_type_from(name, object_type_counter++, base));
}

static void
ficl_make_object_type(ficlVm *vm)
{
#define h_make_object_type "( name -- object-type )  create Object type\n\
\"enved\" make-object-type constant fth-enved\n\
\\ ...\n\
<'> enved-inspect fth-enved set-object-inspect\n\
<'> enved-equal?  fth-enved set-object-equal-p\n\
\\ ...\n\
#( 0.0 1.0 1.0 1.0 ) make-enved value enved\n\
enved fth-enved instance-of? => #t\n\
#() fth-enved instance-of?   => #f\n\
Create new object type NAME.  \
Add NAME to feature environment list, \
create a constant \"fth-NAME\" \
of object-type and return new object-type NAME.  \
The new created OBJECT-TYPE can be used to bind functions to it.\n\
See examples/site-lib/enved.fs for examples."
	FTH tp, fs;

	FTH_STACK_CHECK(vm, 1, 1);
	fs = fth_pop_ficl_cell(vm);
	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	tp = fth_make_object_type(fth_string_ref(fs));
	ficlStackPushFTH(vm->dataStack, tp);
}

static void
ficl_object_type_ref(ficlVm *vm)
{
#define h_object_type_ref "( obj -- obj-struct )  return object struct\n\
fth-enved object-type-ref => Object% struct\n\
Return object struct of object-type OBJ.\n\
See also make-object-type."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = ficlStackPopFTH(vm->dataStack);
	FTH_ASSERT_ARGS(OBJECT_TYPE_P(obj), obj, FTH_ARG1, "an object-type");
	ficlStackPushPointer(vm->dataStack, FTH_OBJECT_REF(obj));
}

static void
ficl_object_types(ficlVm *vm)
{
#define h_object_types "( -- ary )  return all object names\n\
object-types => #( \"array\" \"list\" \"acell\"... )\n\
Return array of all object names known to the system."
	FTH objs;
	ficlInteger i;

	FTH_STACK_CHECK(vm, 0, 1);
	objs = fth_make_array_len((ficlInteger)last_object);
	for (i = 0; i < last_object; i++)
		fth_array_set(objs, i, fth_make_string(obj_types[i]->name));
	ficlStackPushFTH(vm->dataStack, objs);
}

/*
 * Return GEN-struct of OBJ.
 */
void *
fth_instance_ref_gen(FTH obj)
{
	if (INSTANCE_P(obj)) {
		GC_MARK_SET(FTH_INSTANCE_REF(obj));
		return ((void *)FTH_INSTANCE_REF_GEN(obj, void));
	}
	return (NULL);
}

/* === INSTANCE === */

static FInstance *
gc_next_instance(void)
{
	FInstance *current = NULL;

	if (last_instance && (last_instance % GC_MAX_OBJECTS) == 0) {
		current = inst_free_list;
		if (current != NULL)
			inst_free_list = inst_free_list->next;
		else if (fth_gc_on_p && !fth_signal_caught_p)
			current = gc_run();
		else
			current = NULL;
	}
	if (current == NULL) {
		if (last_instance % GC_CHUNK_SIZE == 0) {
			int i, size;

			size = GC_CHUNK_SIZE + last_instance;
			instances = FTH_REALLOC(instances,
			    sizeof(FInstance *) * (size_t)size);

			for (i = last_instance; i < size; i++) {
				instances[i] = FTH_CALLOC(1, sizeof(FInstance));
				instances[i]->gc_mark = GC_FREED;
			}
			if (last_instance == 0)
				inst_minmem = *instances;
		}
		current = instances[last_instance++];
	}
	if (inst_minmem > current)
		inst_minmem = current;
	if (inst_maxmem < current)
		inst_maxmem = current;
	return (current);
}

bool
fth_instance_type_p(FTH obj, fobj_t type)
{
	if (obj != 0 && INSTANCE_P(obj)) {
		GC_MARK_SET(FTH_INSTANCE_REF(obj));
		return (FTH_INSTANCE_TYPE(obj) == type);
	}
	return (false);
}

bool
fth_instance_flag_p(FTH obj, int flag)
{
	if (INSTANCE_P(obj)) {
		GC_MARK_SET(FTH_INSTANCE_REF(obj));
		return (FTH_INSTANCE_FLAG(obj) & flag);
	}
	return (false);
}

bool
fth_instance_p(FTH obj)
{
	if (obj != 0 && INSTANCE_P(obj)) {
		GC_MARK_SET(FTH_INSTANCE_REF(obj));
		return (true);
	}
	return (false);
}

static void
ficl_instance_p(ficlVm *vm)
{
#define h_instance_p "( obj -- f )  test if OBJ is an Instance\n\
gen fth-enved make-instance instance? => #t\n\
Return #t if OBJ is an instance, otherwise #f.\n\
See also make-instance."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, fth_instance_p(obj));
}

/*
 * Return new instance of Object type OBJ with GEN wrapped in.
 */
FTH
fth_make_instance(FTH obj, void *gen)
{
	FInstance *inst;

	if (!OBJECT_TYPE_P(obj)) {
		ficlVmThrowError(FTH_FICL_VM(), "no object type %#x", obj);
		/* NOTREACHED */
		return (FTH_FALSE);
	}
	inst = gc_next_instance();
	inst->type = FTH_T;
	inst->gen = gen;
	inst->obj = FTH_OBJECT_REF(obj);
	inst->properties = FTH_FALSE;
	inst->values = FTH_FALSE;
	inst->debug_hook = FTH_FALSE;
	inst->changed_p = true;
	inst->extern_p = (FTH_OBJECT_TYPE(obj) >= FTH_LAST_ENTRY_T);
	inst->cycle = 0;
	inst->gc_mark = GC_MARK;
	inst->next = GC_FRAME_CURRENT_INST();
	GC_FRAME_CURRENT_INST() = inst;
	return ((FTH)inst);
}

static void
ficl_make_instance(ficlVm *vm)
{
#define h_make_instance "( gen obj -- instance )  return new instance\n\
gen fth-enved make-instance value enved\n\
Return new instance of Object type OBJ with GEN wrapped in.\n\
See also instance-gen-ref and instance-obj-ref.\n\
See examples/site-lib/enved.fs for examples."
	void *gen;
	FTH obj;

	FTH_STACK_CHECK(vm, 2, 1);
	obj = ficlStackPopFTH(vm->dataStack);
	gen = ficlStackPopPointer(vm->dataStack);
	ficlStackPushFTH(vm->dataStack, fth_make_instance(obj, gen));
}

static void
ficl_instance_gen_ref(ficlVm *vm)
{
#define h_instance_gen_ref "( obj -- gen )  return struct of OBJ\n\
gen fth-enved make-instance value enved\n\
enved instance-gen-ref => Enved% struct\n\
Return GEN-struct of OBJ.\n\
See also make-instance and instance-obj-ref.\n\
See examples/site-lib/enved.fs for examples."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = ficlStackPopFTH(vm->dataStack);
	if (!INSTANCE_P(obj)) {
		FTH_ASSERT_ARGS(INSTANCE_P(obj), obj, FTH_ARG1, "an instance");
		/* for ccc-analyzer */
		/* NOTREACHED */
		return;
	}
	GC_MARK_SET(FTH_INSTANCE_REF(obj));
	ficlStackPushPointer(vm->dataStack, FTH_INSTANCE_REF_GEN(obj, void));
}

static void
ficl_instance_obj_ref(ficlVm *vm)
{
#define h_instance_obj_ref "( obj -- gen )  return object type\n\
gen fth-enved make-instance value enved\n\
enved instance-obj-ref => enved (object-type)\n\
enved instance-obj-ref object-name => \"object-type\"\n\
Return object type of OBJ.\n\
See also instance-gen-ref and instance-obj-ref."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = ficlStackPopFTH(vm->dataStack);
	if (!INSTANCE_P(obj)) {
		FTH_ASSERT_ARGS(INSTANCE_P(obj), obj, FTH_ARG1, "an instance");
		/* for ccc-analyzer */
		/* NOTREACHED */
		return;
	}
	GC_MARK_SET(FTH_INSTANCE_REF(obj));
	ficlStackPushPointer(vm->dataStack, FTH_INSTANCE_REF_OBJ(obj));
}

/*
 * Return true if OBJ is an instance of TYPE, otherwise false.
 */
bool
fth_object_is_instance_of(FTH obj, FTH type)
{
	if (INSTANCE_P(obj) && OBJECT_TYPE_P(type)) {
		GC_MARK_SET(FTH_INSTANCE_REF(obj));
		return (FTH_INSTANCE_TYPE(obj) == FTH_OBJECT_TYPE(type));
	}
	return (false);
}

static void
ficl_object_is_instance_of(ficlVm *vm)
{
#define h_object_is_instance_of "( obj type -- f )  test if OBJ is of TYPE\n\
#( 0 1 2 ) value a1\n\
a1 fth-array instance-of? => #t\n\
a1 fth-hash  instance-of? => #f\n\
Return #t if OBJ is an instance of TYPE, otherwise #f."
	FTH obj, type;

	FTH_STACK_CHECK(vm, 2, 1);
	type = ficlStackPopFTH(vm->dataStack);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack,
	    fth_object_is_instance_of(obj, type));
}

/* === Object Set Functions === */

#define SET_OBJECT_FUNC1(Name)						\
FTH									\
fth_set_object_ ## Name (FTH obj, FTH (*Name)(FTH self))		\
{									\
	if (OBJECT_TYPE_P(obj))						\
		FTH_OBJECT_REF(obj)->Name = Name;			\
	return (obj);							\
}									\
static FTH								\
ficl_obj_ ## Name (FTH self)						\
{									\
	FTH res, prc;							\
									\
	prc = FTH_INSTANCE_REF_OBJ(self)->Name ## _proc;		\
	res = fth_proc_call(prc, # Name, 1, self);			\
	return (res);							\
}									\
static void								\
ficl_set_ ## Name (ficlVm *vm)						\
{									\
	FTH obj, prc;							\
	ficlWord *word;							\
									\
	FTH_STACK_CHECK(vm, 2, 0);					\
	obj = ficlStackPopFTH(vm->dataStack);				\
	word = ficlStackPopPointer(vm->dataStack);			\
	if (OBJECT_TYPE_P(obj))	{					\
		prc = fth_make_proc(word, 1, 0, false);			\
		FTH_OBJECT_REF(obj)->Name = ficl_obj_ ## Name;		\
		FTH_OBJECT_REF(obj)->Name ## _proc = prc;		\
	}								\
	else								\
		fth_warning("%s: %S seems not to be an object-type",	\
		    # Name, obj);					\
}

#define SET_OBJECT_FUNC2(Name)						\
FTH									\
fth_set_object_ ## Name (FTH obj, FTH (*Name)(FTH self, FTH val))	\
{									\
	if (OBJECT_TYPE_P(obj))						\
		FTH_OBJECT_REF(obj)->Name = Name;			\
	return (obj);							\
}									\
static FTH								\
ficl_obj_ ## Name (FTH self, FTH val)					\
{									\
	FTH res, prc;							\
									\
	prc = FTH_INSTANCE_REF_OBJ(self)->Name ## _proc;		\
	res = fth_proc_call(prc, # Name, 2, self, val);			\
	return (res);							\
}									\
static void								\
ficl_set_ ## Name (ficlVm *vm)						\
{									\
	FTH obj, prc;							\
	ficlWord *word;							\
									\
	FTH_STACK_CHECK(vm, 2, 0);					\
	obj = ficlStackPopFTH(vm->dataStack);				\
	word = ficlStackPopPointer(vm->dataStack);			\
	if (OBJECT_TYPE_P(obj))	{					\
		prc = fth_make_proc(word, 2, 0, false);			\
		FTH_OBJECT_REF(obj)->Name = ficl_obj_ ## Name;		\
		FTH_OBJECT_REF(obj)->Name ## _proc = prc;		\
	}								\
	else								\
		fth_warning("%s: %S seems not to be an object-type",	\
		    # Name, obj);					\
}

#define SET_OBJECT_FUNC3(Name)						\
FTH									\
fth_set_object_ ## Name (FTH obj, FTH (*Name)(FTH s, FTH o1, FTH o2))	\
{									\
	if (OBJECT_TYPE_P(obj))						\
		FTH_OBJECT_REF(obj)->Name = Name;			\
	return (obj);							\
}									\
static FTH								\
ficl_obj_ ## Name (FTH self, FTH o1, FTH o2)				\
{									\
	FTH res, prc;							\
									\
	prc = FTH_INSTANCE_REF_OBJ(self)->Name ## _proc;		\
	res = fth_proc_call(prc, # Name, 3, self, o1, o2);		\
	return (res);							\
}									\
static void								\
ficl_set_ ## Name (ficlVm *vm)						\
{									\
	FTH obj, prc;							\
	ficlWord *word;							\
									\
	FTH_STACK_CHECK(vm, 2, 0);					\
	obj = ficlStackPopFTH(vm->dataStack);				\
	word = ficlStackPopPointer(vm->dataStack);			\
	if (OBJECT_TYPE_P(obj))	{					\
		prc =  fth_make_proc(word, 3, 0, false);		\
		FTH_OBJECT_REF(obj)->Name = ficl_obj_ ## Name;		\
		FTH_OBJECT_REF(obj)->Name ## _proc = prc;		\
	}								\
	else								\
		fth_warning("%s: %S seems not to be an object-type",	\
		# Name, obj);						\
}

#define SET_OBJECT_FUNCV(Name)						\
FTH									\
fth_set_object_ ## Name (FTH obj, void (*Name)(FTH self))		\
{									\
	if (OBJECT_TYPE_P(obj))						\
		FTH_OBJECT_REF(obj)->Name = Name;			\
	return (obj);							\
}									\
static void								\
ficl_obj_ ## Name (FTH self)						\
{									\
	FTH prc;							\
									\
	prc = FTH_INSTANCE_REF_OBJ(self)->Name ## _proc;		\
	fth_proc_call(prc, # Name, 1, self);				\
}									\
static void								\
ficl_set_ ## Name (ficlVm *vm)						\
{									\
	FTH obj, prc;							\
	ficlWord *word;							\
									\
	FTH_STACK_CHECK(vm, 2, 0);					\
	obj = ficlStackPopFTH(vm->dataStack);				\
	word = ficlStackPopPointer(vm->dataStack);			\
	if (OBJECT_TYPE_P(obj))	{					\
		prc = fth_make_proc(word, 1, 0, false);			\
		FTH_OBJECT_REF(obj)->Name = ficl_obj_ ## Name;		\
		FTH_OBJECT_REF(obj)->Name ## _proc = prc;		\
	}								\
	else								\
		fth_warning("%s: %S seems not to be an object-type",	\
		    # Name, obj);					\
}

SET_OBJECT_FUNC1(inspect)
SET_OBJECT_FUNC1(to_string)
SET_OBJECT_FUNC1(dump)
SET_OBJECT_FUNC1(to_array)
SET_OBJECT_FUNC1(copy)
SET_OBJECT_FUNC2(value_ref)
SET_OBJECT_FUNC3(value_set)
SET_OBJECT_FUNC2(equal_p)
SET_OBJECT_FUNC1(length)
SET_OBJECT_FUNCV(mark)
SET_OBJECT_FUNCV(free)

#define h_set_insepct "( xt obj -- )  set XT as .inspect function\n\
<'> enved-inspect fth-enved set-object-inspect\n\
Set XT as OBJECT-INSPECT function for OBJ type.\n\
See examples/site-lib/enved.fs for examples."

#define h_set_to_string "( xt obj -- )  set XT as object->string function\n\
<'> enved->string fth-enved set-object->string\n\
Set XT as OBJECT->STRING function for OBJ type.\n\
See examples/site-lib/enved.fs for examples."

#define h_set_dump "( xt obj -- )  set XT as object-dump function\n\
<'> enved->dump fth-enved set-object-dump\n\
Set XT as OBJECT-DUMP function for OBJ type.\n\
See examples/site-lib/enved.fs for examples."

#define h_set_to_array "( xt obj -- )  set XT as object->array function\n\
<'> enved->array fth-enved set-object->array\n\
Set XT as OBJECT->ARRAY function for OBJ type.\n\
See examples/site-lib/enved.fs for examples."

#define h_set_copy "( xt obj -- )  set XT as object-copy function\n\
<'> enved-copy fth-enved set-object-copy\n\
Set XT as OBJECT-COPY function for OBJ type.\n\
See examples/site-lib/enved.fs for examples."

#define h_set_value_ref "( xt obj -- )  set XT as object-ref function\n\
<'> enved-ref fth-enved set-object-value-ref\n\
Set XT as OBJECT-REF function for OBJ type.\n\
See examples/site-lib/enved.fs for examples."

#define h_set_value_set "( xt obj -- )  set XT as object-set! function\n\
<'> enved-set! fth-enved set-object-value-set\n\
Set XT as OBJECT-SET! function for OBJ type.\n\
See examples/site-lib/enved.fs for examples."

#define h_set_equal_p "( xt obj -- )  set XT as object-equal? function\n\
<'> enved-equal? fth-enved set-object-equal-p\n\
Set XT as OBJECT-EQUAL? function for OBJ type.\n\
See examples/site-lib/enved.fs for examples."

#define h_set_length "( xt obj -- )  set XT as object-length function\n\
<'> enved-length fth-enved set-object-length\n\
Set XT as OBJECT-LENGTH function for OBJ type.\n\
See examples/site-lib/enved.fs for examples."

#define h_set_mark "( xt obj -- )  set XT as gc-mark function\n\
<'> enved-mark fth-enved set-object-mark\n\
Set XT as GC mark function for OBJ type.\n\
See examples/site-lib/enved.fs for examples."

#define h_set_free "( xt obj -- )  set XT as gc-free function\n\
<'> enved-free fth-enved set-object-free\n\
Set XT as GC free function for OBJ type.\n\
See examples/site-lib/enved.fs for examples."

/* handles C objects */
FTH
fth_set_object_apply(FTH obj, void *apply, int req, int opt, int rest)
{
	if (OBJECT_TYPE_P(obj)) {
		FTH prc;

		/*
		 * req + 1: self in addition to args length
		 */
		prc = fth_make_proc_from_func(NULL, (FTH(*)())apply,
		    false, req + 1, opt, rest);
		FTH_OBJECT_REF(obj)->apply = prc;
	}
	return (obj);
}

/* handles Forth objects */
static void
ficl_set_apply(ficlVm *vm)
{
#define h_set_apply "( xt obj arity -- )  set XT as object-apply function\n\
<'> enved-ref fth-enved 1 set-object-apply\n\
Set XT as OBJECT-APPLY function for OBJ type.\n\
See examples/site-lib/enved.fs for examples."
	FTH obj, arity;
	ficlWord *word;
	int req, opt;
	bool rest;

	FTH_STACK_CHECK(vm, 3, 0);
	arity = fth_pop_ficl_cell(vm);
	obj = ficlStackPopFTH(vm->dataStack);
	word = ficlStackPopPointer(vm->dataStack);
	opt = 0;
	rest = false;
	/*
	 * arity + 1: req + self
	 */
	if (fth_array_length(arity) == 3) {
		req = FIX_TO_INT32(fth_array_ref(arity, 0L));
		opt = FIX_TO_INT32(fth_array_ref(arity, 1L));
		rest = FTH_TO_BOOL(fth_array_ref(arity, 2L));
	} else
		req = FIX_TO_INT32(arity) + 1;
	if (OBJECT_TYPE_P(obj)) {
		FTH prc;

		prc = fth_make_proc(word, req, opt, rest);
		FTH_OBJECT_REF(obj)->apply = prc;
	} else
		fth_warning("%s: %S seems not to be an object-type",
		    RUNNING_WORD(), obj);
}

/* Very special Snd XM case. */
static bool
xmobj_p(FTH obj)
{
	ficlInteger len;

#define X_SYMBOL_P(Obj)							\
	(FTH_SYMBOL_P(Obj) && isupper((int)(fth_symbol_ref(Obj)[0])))

	len = fth_list_length(obj);
	/*
	 * Xen Object (2|5) '( 'Name pointer ) or '( 'Name pointer bool zero
	 * zero )
	 */
	if ((len == 2 || len == 5) && fth_unsigned_p(fth_array_ref(obj, 1L)))
		return (X_SYMBOL_P(fth_array_ref(obj, 0L)));
	/*
	 * Xen Object (3) '( 'Name pointer object )
	 */
	if (len == 3 &&
	    fth_unsigned_p(fth_array_ref(obj, 1L) &&
	    fth_instance_p(fth_array_ref(obj, 2L))))
		return (X_SYMBOL_P(fth_array_ref(obj, 0L)));
	/*
	 * XEvent '( 'XXxxEvent pointer object 'XEvent )
	 */
	if (len == 4 && X_SYMBOL_P(fth_array_ref(obj, 3L)))
		return (strncmp(fth_symbol_ref(fth_array_ref(obj, 3L)),
		    "XEvent", 6L) == 0);
	return (false);
}

static void
ficl_xmobj_p(ficlVm *vm)
{
#define h_xmobj_p "( obj -- f )  test if OBJ is a XmObj\n\
nil xmobj? => #f\n\
#( 'Atom 0 ) xmobj? => #t\n\
Return #t if OBJ is an XmObj (xm.c), otherwise #f.  \
It is a very special Snd XM test.\n\
See snd(1) for more information."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, xmobj_p(obj));
}

static FTH
xmobj_to_string(FTH obj)
{
	if (xmobj_p(obj)) {
		ficlInteger len;
		char *symbol;

		len = fth_array_length(obj);
		symbol = ((len > 0) ?
		    fth_symbol_ref(fth_array_ref(obj, 0L)) : NULL);

		switch (len) {
		case 2:
			/* #( symbol addr ) */
			return (fth_make_string_format("#( %s %p )",
			    symbol, (void *)fth_array_ref(obj, 1L)));
			break;
		case 3:
			/* #( symbol addr xm-obj-from-addr ) */
			if (fth_instance_p(fth_array_ref(obj, 2L)))
				return (fth_make_string_format("#( XmObj %s )",
				    symbol));
			break;
		case 4:
			/* #( type addr xm-obj-from-addr symbol ) */
			if (fth_instance_p(fth_array_ref(obj, 2L)))
				return (fth_make_string_format("#( XEvent %S )",
				    fth_array_ref(obj, 0L)));
			break;
		case 5:
			/* #( symbol code context prop-atom prot-atom ) */
			return (fth_make_string_format("#( Callback %s )",
			    fth_symbol_ref(fth_array_ref(obj, 3L))));
			break;
		default:
			break;
		}
	}
	return (FTH_FALSE);
}

/* === Object Functions === */

FTH
fth_hash_id(FTH obj)
{
#define h_hash_id "( obj -- id )  return hash id\n\
\"hello\" hash-id => 3583\n\
\"hello\" hash-id => 3583\n\
Return hash id computed from string representation of OBJ.  \
Objects with the same content have the same ID.\n\
See also object-id."
	if (FTH_FIXNUM_P(obj))
		return (INT_TO_FIX(obj));
	if (FICL_WORD_DEFINED_P(obj))
		return (INT_TO_FIX(FICL_WORD_REF(obj)->hash));
	if (fth_instance_p(obj)) {
		char *str;
		ficlString s;

		str = fth_to_c_string(obj);
		FICL_STRING_SET_FROM_CSTRING(s, str);
		return (INT_TO_FIX(ficlHashCode(s)));
	}
	return ((FTH)((ficlInteger)obj | FIXNUM_FLAG));
}

FTH
fth_object_id(FTH obj)
{
#define h_object_id "( obj -- id )  return uniq id\n\
\"hello\" object-id => 378357376\n\
\"hello\" object-id => 378358144\n\
Return object id of OBJ, a uniq number.\n\
See also hash-id."
	if (IMMEDIATE_P(obj))
		return (INT_TO_FIX(obj));
	return ((FTH)((ficlInteger)obj | FIXNUM_FLAG));
}

char *
fth_object_name(FTH obj)
{
	if (obj == 0 || (IMMEDIATE_P(obj) && FIXNUM_P(obj)))
		return ("fixnum");
	if (INSTANCE_P(obj)) {
		if (FTH_ULLONG_P(obj))
			return ("unsigned llong");
		if (FTH_UNSIGNED_P(obj))
			return ("unsigned integer");
		return (FTH_INSTANCE_NAME(obj));
	}
	if (FICL_WORD_DEFINED_P(obj)) {
		switch (FICL_WORD_TYPE(obj)) {
		case FW_WORD:
			return ("word");
			break;
		case FW_PROC:
			return ("proc");
			break;
		case FW_SYMBOL:
			return ("symbol");
			break;
		case FW_KEYWORD:
			return ("keyword");
			break;
		case FW_EXCEPTION:
			return ("exception");
			break;
		case FW_VARIABLE:
			return ("variable");
			break;
		case FW_TRACE_VAR:
			return ("trace-var");
			break;
		default:
			return ("unknown-word-type");
			break;
		}
	}
	if (OBJECT_TYPE_P(obj))
		return ("object-type");
	return ("addr");
}

static void
ficl_object_name(ficlVm *vm)
{
#define h_object_name "( obj -- name )  return name of OBJ\n\
#( 0 1 2 ) object-name => \"array\"\n\
1+i        object-name => \"complex\"\n\
1          object-name => \"fixnum\"\n\
Return object type name of OBJ as a string.\n\
See also .object-name."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	push_cstring(vm, fth_object_name(obj));
}

static void
ficl_print_object_name(ficlVm *vm)
{
#define h_print_object_name "( obj -- )  print name of OBJ\n\
#( 0 1 2 ) .object-name => array\n\
1+i        .object-name => complex\n\
1          .object-name => fixnum\n\
Print object name of OBJ to current stdout.\n\
See also object-name."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 0);
	obj = fth_pop_ficl_cell(vm);
	fth_print(fth_object_name(obj));
}

static FTH
print_object(FTH obj, fth_inspect_type_t type)
{
	FTH fs;

	fs = FTH_FALSE;
	if (obj == 0 || (IMMEDIATE_P(obj) && FIXNUM_P(obj))) {
			if (type == OBJ_INSPECT)
				fs = fth_make_string_format("%s: %ld",
				    fth_object_name(obj),
				    FIX_TO_INT(obj));
			else
				fs = fth_make_string_format("%ld",
				    FIX_TO_INT(obj));
	} else if (INSTANCE_P(obj)) {
		/* xm.c */
		if (xmobj_p(obj))
			fs = xmobj_to_string(obj);
		else
			switch (type) {
			case OBJ_INSPECT:
				if (FTH_INSPECT_P(obj))
					fs = FTH_INSPECT(obj);
				else if (FTH_TO_STRING_P(obj))
					fs = FTH_TO_STRING(obj);
				else if (FTH_DUMP_P(obj))
					fs = FTH_DUMP(obj);
				else
					fs = fth_make_string_format("%s: %p",
					    FTH_INSTANCE_NAME(obj),
					    (void *)obj);
				if (!fth_hook_empty_p(
				    FTH_INSTANCE_DEBUG_HOOK(obj)))
					fs = fth_run_hook_again(
					    FTH_INSTANCE_DEBUG_HOOK(obj),
					    2, fs, obj);
				break;
			case OBJ_TO_STRING:
				if (FTH_TO_STRING_P(obj))
					fs = FTH_TO_STRING(obj);
				else if (FTH_INSPECT_P(obj))
					fs = FTH_INSPECT(obj);
				else if (FTH_DUMP_P(obj))
					fs = FTH_DUMP(obj);
				else
					fs = fth_make_string_format("#<%s:%p>",
					    FTH_INSTANCE_NAME(obj),
					    (void *)obj);
				break;
			case OBJ_DUMP:
			default:
				if (FTH_DUMP_P(obj))
					fs = FTH_DUMP(obj);
				else if (FTH_TO_STRING_P(obj))
					fs = FTH_TO_STRING(obj);
				else if (FTH_INSPECT_P(obj))
					fs = FTH_INSPECT(obj);
				else
					fs = fth_make_string_format("%s:%p",
					    FTH_INSTANCE_NAME(obj),
					    (void *)obj);
				break;
			}
	} else if (FICL_WORD_DEFINED_P(obj))
		switch (type) {
		case OBJ_INSPECT:
			fs = fth_make_string_format("%s: %S",
			    fth_object_name(obj), fth_word_inspect(obj));
			break;
		case OBJ_TO_STRING:
			fs = fth_word_to_string(obj);
			break;
		case OBJ_DUMP:
		default:
			fs = fth_word_dump(obj);
			break;
		}
	else if (OBJECT_TYPE_P(obj))
		switch (type) {
		case OBJ_INSPECT:
			fs = fth_make_string_format("%s: %s",
			    fth_object_name(obj), FTH_OBJECT_NAME(obj));
			break;
		default:
			fs = fth_make_string_format("%s",
			    FTH_OBJECT_NAME(obj));
			break;
		}
	if (FTH_FALSE_P(fs))
		fs = fth_make_string_format("addr %p", obj);
	if (type == OBJ_INSPECT) {
		if (fth_string_length(fs) < 3 ||
		    (FTH_TO_CHAR(fth_string_char_ref(fs, 0L)) !=
		    (int)'#' && FTH_TO_CHAR(fth_string_char_ref(fs, 1L)) !=
		    (int)'<'))
			fs = fth_make_string_format("#<%S>", fs);
	}
	return (fs);
}

static void
ficl_print_inspect(ficlVm *vm)
{
#define h_print_inspect "( obj -- )  print inspect string\n\
#( 0 ) .inspect => #<array[1]:  #<fixnum: 0>>\n\
Print inspect string of OBJ.\n\
See also object-inspect, object->string and object-dump."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 0);
	obj = fth_pop_ficl_cell(vm);
	fth_print(fth_to_c_string(print_object(obj, OBJ_INSPECT)));
}

FTH
fth_object_inspect(FTH obj)
{
#define h_object_inspect "( obj -- str )  return inspect string\n\
#( 0 ) object-inspect => \"#<array[1]:  #<fixnum: 0>>\"\n\
Return inspect string of OBJ.\n\
See also .inspect, object->string and object-dump."
	return (print_object(obj, OBJ_INSPECT));
}

/*
 * Return Fth string representation of OBJ.
 */
FTH
fth_object_to_string(FTH obj)
{
#define h_object_to_string "( obj -- str )  return OBJ as string\n\
#( 0 ) object->string => \"#( 0 )\"\n\
Return string representation of OBJ.\n\
See also .inspect, object-inspect and object-dump."
	return (print_object(obj, OBJ_TO_STRING));
}

/* For object type function obj_to_string. */
FTH
fth_object_to_string_2(FTH obj)
{
	if (FTH_STRING_P(obj))
		return (fth_make_string_format("\"%S\"", obj));
	return (print_object(obj, OBJ_TO_STRING));
}

FTH
fth_object_dump(FTH obj)
{
#define h_object_dump "( obj -- str )  return OBJ as string\n\
#( 0 ) object-dump => \"#( 0 )\"\n\
#( 0 ) object-dump string-eval value a1\n\
a1 object-name => \"array\"\n\
a1 object->string => \"#( 0 )\"\n\
Return dump string of OBJ which one can eval to get the object back.\n\
See also .inspect, object-inspect and object->string."
	return (print_object(obj, OBJ_DUMP));
}

char *
fth_to_c_inspect(FTH obj)
{
	return (fth_string_ref(print_object(obj, OBJ_INSPECT)));
}

char *
fth_to_c_string(FTH obj)
{
	return (fth_string_ref(print_object(obj, OBJ_TO_STRING)));
}

/* For object type function obj_to_string. */
char *
fth_to_c_string_2(FTH obj)
{
	return (fth_string_ref(fth_object_to_string_2(obj)));
}

char *
fth_to_c_dump(FTH obj)
{
	return (fth_string_ref(print_object(obj, OBJ_DUMP)));
}

FTH
fth_object_to_array(FTH obj)
{
#define h_object_to_array "( obj -- ary )  return OBJ as array\n\
\"foo\" object->array => #( 102 111 111 )\n\
1 object->array => #( 1 )\n\
Return OBJ as array."
	if (INSTANCE_P(obj) && FTH_TO_ARRAY_P(obj)) {
		FInstance *inst;

		GC_MARK_SET(FTH_INSTANCE_REF(obj));
		inst = FTH_INSTANCE_REF(obj);
		if (inst->changed_p || inst->extern_p) {
			inst->values = FTH_TO_ARRAY(obj);
			inst->changed_p = false;
		}
		return (inst->values);
	}
	return (fth_make_array_var(1, obj));
}

FTH
fth_object_copy(FTH obj)
{
#define h_object_copy "( obj1 -- obj2 )  return copy of OBJ1\n\
#( 0 1 2 ) value a1\n\
a1 object-copy value a2\n\
a1 a2 object-equal? => #t\n\
Return copy of OBJ1.  \
Copy any element if OBJ1 is an instance."
	if (obj != 0 && INSTANCE_P(obj) && FTH_COPY_P(obj)) {
		FTH new;
		FInstance *inst, *new_inst;

		GC_MARK_SET(FTH_INSTANCE_REF(obj));
		if (FTH_BOOLEAN_P(obj) || FTH_NIL_TYPE_P(obj))
			return (obj);
		new = FTH_COPY(obj);
		inst = FTH_INSTANCE_REF(obj);
		new_inst = FTH_INSTANCE_REF(new);
		new_inst->properties = fth_object_copy(inst->properties);
		new_inst->extern_p = inst->extern_p;
		new_inst->cycle = inst->cycle;
		return (new);
	}
	return (obj);
}

FTH
fth_object_value_ref(FTH obj, ficlInteger idx)
{
	if (INSTANCE_P(obj) && FTH_VALUE_REF_P(obj)) {
		GC_MARK_SET(FTH_INSTANCE_REF(obj));
		if (idx < 0)
			idx += fth_object_length(obj);
		return (FTH_VALUE_REF(obj, fth_make_int(idx)));
	}
	return (obj);
}

static void
ficl_object_ref(ficlVm *vm)
{
#define h_object_ref "( obj index -- value)  return value at INDEX\n\
#( 0 1 2 ) 1 object-ref => 1\n\
1 1 object-ref => 1\n\
Return value at INDEX from OBJ.  \
If OBJ is of a type which can have multiple elements, \
an array for example, return value at INDEX.  \
If OBJ is of a type which consists of only one element, \
a fixnum for example, ignore INDEX and return OBJ itself.\n\
See also object-set!."
	FTH obj;
	ficlInteger idx;

	FTH_STACK_CHECK(vm, 2, 1);
	idx = ficlStackPopInteger(vm->dataStack);
	obj = fth_pop_ficl_cell(vm);
	fth_push_ficl_cell(vm, fth_object_value_ref(obj, idx));
}

FTH
fth_object_value_set(FTH obj, ficlInteger idx, FTH value)
{
	if (INSTANCE_P(obj) && FTH_VALUE_SET_P(obj)) {
		GC_MARK_SET(FTH_INSTANCE_REF(obj));
		if (idx < 0)
			idx += fth_object_length(obj);
		return (FTH_VALUE_SET(obj, fth_make_int(idx), value));
	}
	return (value);
}

static void
ficl_object_set(ficlVm *vm)
{
#define h_object_set "( obj index value -- )  set VALUE at INDEX\n\
#( 0 1 2 ) value a1\n\
a1 1 \"a\" object-set!\n\
a1 .$ => #( 0 \"a\" 2 )\n\
1 value a2\n\
a2 1 \"a\" object-set!\n\
a2 .$ => 1\n\
Set VALUE at INDEX to OBJ.  \
If OBJ is of a type which can have multiple elements, \
an array for example, set VALUE at position INDEX.  \
If OBJ is of a type which consists of only one element, \
a fixnum for example, do nothing.\n\
See also object-ref."
	FTH obj, val;
	ficlInteger idx;

	FTH_STACK_CHECK(vm, 3, 0);
	val = fth_pop_ficl_cell(vm);
	idx = ficlStackPopInteger(vm->dataStack);
	obj = fth_pop_ficl_cell(vm);
	fth_object_value_set(obj, idx, val);
}

static void
ficl_add_store_object(ficlVm *vm)
{
#define h_add_store_object "( obj index value -- )  add VALUE\n\
#( 0 1 2 ) value a1\n\
a1 1 2 object-set+!\n\
a1 .$ => #( 0 3 2 )\n\
Add VALUE to value at INDEX of OBJ.  \
Value may be any number (ficlInteger, ficlFloat, ficlRatio, \
or ficlComplex).\n\
See also object-set-!, object-set*! and object-set/!."
	FTH obj, value;
	ficlInteger idx;

	FTH_STACK_CHECK(vm, 3, 0);
	value = fth_pop_ficl_cell(vm);
	idx = ficlStackPopInteger(vm->dataStack);
	obj = fth_pop_ficl_cell(vm);
	fth_object_value_set(obj, idx,
	    fth_number_add(fth_object_value_ref(obj, idx), value));
}

static void
ficl_sub_store_object(ficlVm *vm)
{
#define h_sub_store_object "( obj index value -- )  subtract VALUE\n\
#( 0 1 2 ) value a1\n\
a1 1 2 object-set-!\n\
a1 .$ => #( 0 -1 2 )\n\
Subtract VALUE from value at INDEX of OBJ.  \
Value may be any number (ficlInteger, ficlFloat, ficlRatio, \
or ficlComplex).\n\
See also object-set+!, object-set*! and object-set/!."
	FTH obj, value;
	ficlInteger idx;

	FTH_STACK_CHECK(vm, 3, 0);
	value = fth_pop_ficl_cell(vm);
	idx = ficlStackPopInteger(vm->dataStack);
	obj = fth_pop_ficl_cell(vm);
	fth_object_value_set(obj, idx,
	    fth_number_sub(fth_object_value_ref(obj, idx), value));
}

static void
ficl_mul_store_object(ficlVm *vm)
{
#define h_mul_store_object "( obj index value -- )  multiply with VALUE\n\
#( 0 1 2 ) value a1\n\
a1 2 2 object-set*!\n\
a1 .$ => #( 0 1 4 )\n\
Multiply VALUE with value at INDEX of OBJ.  \
Value may be any number (ficlInteger, ficlFloat, ficlRatioi, \
or ficlComplex).\n\
See also object-set+!, object-set-! and object-set/!."
	FTH obj, value;
	ficlInteger idx;

	FTH_STACK_CHECK(vm, 3, 0);
	value = fth_pop_ficl_cell(vm);
	idx = ficlStackPopInteger(vm->dataStack);
	obj = fth_pop_ficl_cell(vm);
	fth_object_value_set(obj, idx,
	    fth_number_mul(fth_object_value_ref(obj, idx), value));
}

static void
ficl_div_store_object(ficlVm *vm)
{
#define h_div_store_object "( obj index value -- )  divide by VALUE\n\
#( 0 1 2 ) value a1\n\
a1 2 2 object-set/!\n\
a1 .$ => #( 0 1 1 )\n\
Divide value at INDEX of OBJ by VALUE.  \
Value may be any number (ficlInteger, ficlFloat, ficlRatio, \
or ficlComplex).\n\
See also object-set+!, object-set-! and object-set*!."
	FTH obj, value;
	ficlInteger idx;

	FTH_STACK_CHECK(vm, 3, 0);
	value = fth_pop_ficl_cell(vm);
	idx = ficlStackPopInteger(vm->dataStack);
	obj = fth_pop_ficl_cell(vm);
	fth_object_value_set(obj, idx,
	    fth_number_div(fth_object_value_ref(obj, idx), value));
}

/*
 * Test if OBJ1 is equal OBJ2.
 */
bool
fth_object_equal_p(FTH obj1, FTH obj2)
{
	if (obj1 == obj2)
		return (true);
	if (INSTANCE_P(obj1) && INSTANCE_P(obj2))
		if (FTH_INSTANCE_TYPE(obj1) == FTH_INSTANCE_TYPE(obj2))
			if (FTH_EQUAL_P_P(obj1))
				return (FTH_TO_BOOL(FTH_EQUAL_P(obj1, obj2)));
	return (false);
}

static void
ficl_object_equal_p(ficlVm *vm)
{
#define h_object_equal_p "( obj1 obj2 -- f )  compare OBJ1 with OBJ2\n\
#( 0 1 2 ) value a1\n\
#( 0 1 2 ) value a2\n\
#{ 0 0  1 1  2 2 } value h1\n\
a1 a2 object-equal? => #t\n\
a1 h1 object-equal? => #f\n\
Return #t if OBJ1 and OBJ2 have equal content, otherwise #f."
	FTH obj1, obj2;
	bool flag;

	FTH_STACK_CHECK(vm, 2, 1);
	obj2 = fth_pop_ficl_cell(vm);
	obj1 = fth_pop_ficl_cell(vm);
	flag = fth_object_equal_p(obj1, obj2);
	ficlStackPushBoolean(vm->dataStack, flag);
}

ficlInteger
fth_object_length(FTH obj)
{
	if (INSTANCE_P(obj) && FTH_LENGTH_P(obj)) {
		GC_MARK_SET(FTH_INSTANCE_REF(obj));
		return (fth_int_ref(FTH_LENGTH(obj)));
	}
	return (0);
}

static void
ficl_object_length(ficlVm *vm)
{
#define h_object_length "( obj -- len )  return length\n\
#( 0 1 2 ) object-length => 3\n\
#{ 0 0  1 1  2 2 } object-length => 3\n\
10 object-length => 0\n\
Return length of OBJ."
	FTH_STACK_CHECK(vm, 1, 1);
	ficlStackPushInteger(vm->dataStack,
	    fth_object_length(fth_pop_ficl_cell(vm)));
}

FTH
fth_object_apply(FTH obj, FTH args)
{
#define h_object_apply "( obj args -- result )  run apply with ARGS\n\
#( 0.3 1.0 2.0 ) value a1\n\
a1    0   array-ref    => 0.3\n\
a1    0   object-ref   => 0.3\n\
a1    0   object-apply => 0.3\n\
a1 #( 0 ) object-apply => 0.3\n\
Run apply on OBJ with ARGS as arguments.  \
ARGS can be an array of arguments or a single argument.  \
The number of ARGS must fit apply's definition.  \
The next two examples require each 1 argument:\n\
    C: fth_set_object_apply(vct_tag, vct_ref, 1, 0, 0);\n\
Forth: <'> enved-ref fth-enved 1 set-object-apply\n\
See examples/site-lib/enved.fs for Forth examples \
and eg. src/array.c for C examples."
	FTH prc, r;

	if (!INSTANCE_P(obj))
		return (FTH_FALSE);
	GC_MARK_SET(FTH_INSTANCE_REF(obj));
	prc = FTH_INSTANCE_REF_OBJ(obj)->apply;
	if (!FTH_PROC_P(prc))
		return (FTH_FALSE);
	if (!FTH_ARRAY_P(args)) {
		if (FICL_WORD_LENGTH(prc) > 1)
			args = fth_make_array_var(1, args);
		else
			args = fth_make_empty_array();
	}
	r = fth_proc_apply(prc, fth_array_unshift(args, obj), RUNNING_WORD());
	return (r);
}

bool
fth_object_empty_p(FTH obj)
{
	return (fth_object_length(obj) == 0);
}

static void
ficl_object_empty_p(ficlVm *vm)
{
#define h_object_empty_p "( obj -- f )  test if OBJ is empty\n\
#() object-empty? => #t\n\
#{ 'a 10 } object-empty? => #f\n\
1 object-empty? => #f\n\
Return #t if length of OBJ is zero, otherwise #f."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, fth_object_empty_p(obj));
}

bool
fth_object_range_p(FTH obj, ficlInteger idx)
{
	return (idx >= 0 && idx < fth_object_length(obj));
}

static void
ficl_object_range_p(ficlVm *vm)
{
#define h_object_range_p "( obj index -- f )  test if INDEX is in range\n\
#( 0 1 2 )  0 object-range? => #t\n\
#( 0 1 2 )  3 object-range? => #f\n\
#( 0 1 2 ) -3 object-range? => #f\n\
Return #t if INDEX is in range of OBJ, otherwise #f.  \
If INDEX is negative, return #f."
	ficlInteger idx;
	FTH obj;

	FTH_STACK_CHECK(vm, 2, 1);
	idx = ficlStackPopInteger(vm->dataStack);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, fth_object_range_p(obj, idx));
}

bool
fth_object_member_p(FTH obj, FTH key)
{
	ficlInteger i;
	FTH val;

	for (i = 0; i < fth_object_length(obj); i++) {
		val = fth_object_value_ref(obj, i);
		if (val == key || fth_object_equal_p(val, key))
			return (true);
	}
	return (false);
}

static void
ficl_object_member_p(ficlVm *vm)
{
#define h_object_member_p "( obj key -- f )  test if KEY exist\n\
#( \"0\" \"1\" \"2\" ) \"0\" object-member? => #t\n\
#{ 'a 10  'b 20 } #( 'b 20 ) object-member? => #t\n\
#{ 'a 10  'b 20 } 'b object-member? => #f\n\
\"foo\" <char> f object-member? => #t\n\
Return #t if KEY is present in OBJ, otherwise #f.\n\
See also object-find and object-index."
	FTH key, obj;

	FTH_STACK_CHECK(vm, 2, 1);
	key = fth_pop_ficl_cell(vm);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, fth_object_member_p(obj, key));
}

FTH
fth_object_find(FTH obj, FTH key)
{
#define h_object_find "( obj key -- value|#f )  return value of KEY\n\
#( \"0\" \"1\" \"2\" ) \"0\" object-find => \"0\"\n\
#{ 'a 10  'b 20 } #( 'b 20 ) object-find => '( 'b . 20 )\n\
#{ 'a 10  'b 20 } 'b object-find => #f\n\
\"foo\" <char> f object-find => 102\n\
Search for KEY in OBJ and return corresponding value or #f if not found.\n\
See also object-member? and object-index."
	ficlInteger i;
	FTH val;

	for (i = 0; i < fth_object_length(obj); i++) {
		val = fth_object_value_ref(obj, i);
		if (val == key || fth_object_equal_p(val, key))
			return (val);
	}
	return (FTH_FALSE);
}

FTH
fth_object_index(FTH obj, FTH key)
{
#define h_object_index "( obj key -- index|-1 )  return index of KEY\n\
#( \"0\" \"1\" \"2\" ) \"0\" object-index => 0\n\
#{ 'a 10  'b 20 } #( 'b 20 ) object-index => 1\n\
#{ 'a 10  'b 20 } 'b object-index => -1\n\
\"foo\" <char> f object-index => 0\n\
Search for KEY in OBJ and return index or -1 if not found.\n\
See also object-member? and object-find."
	ficlInteger i;
	FTH val;

	for (i = 0; i < fth_object_length(obj); i++) {
		val = fth_object_value_ref(obj, i);
		if (val == key || fth_object_equal_p(val, key))
			return (fth_make_int(i));
	}
	return (FTH_ONE_NEG);
}

FTH
fth_object_sort(FTH obj, FTH proc_or_xt)
{
#define h_object_sort "( obj cmd-xt -- ary )  sort OBJ\n\
: numb-sort { val1 val2 -- n }\n\
  val1 val2 < if\n\
    -1\n\
  else\n\
    val1 val2 > if\n\
      1\n\
    else\n\
      0\n\
    then\n\
  then\n\
;\n\
#( 6 2 8 1 ) <'> numb-sort object-sort => #( 1 2 6 8 )\n\
Convert OBJ to an array, sort and return it.  \
CMP-XT compares two items A and B and should return \
a negative integer if A < B, \
0 if A == B, and a positive integer if A > B."
	return (fth_array_sort(fth_object_to_array(obj), proc_or_xt));
}

/* === Cycle === */

#define FTH_CYCLE_NEXT(obj)						\
	if ((fth_object_length(obj) - 1) > FTH_INSTANCE_REF(obj)->cycle)\
		FTH_INSTANCE_REF(obj)->cycle++;				\
	else								\
		FTH_INSTANCE_REF(obj)->cycle = 0

ficlInteger
fth_cycle_pos_ref(FTH obj)
{
	if (INSTANCE_P(obj))
		return (FTH_INSTANCE_REF(obj)->cycle);
	return (0);
}

ficlInteger
fth_cycle_pos_set(FTH obj, ficlInteger idx)
{
	if (!INSTANCE_P(obj))
		return (0);
	if (idx < 0)
		FTH_INSTANCE_REF(obj)->cycle = 0;
	else if (idx >= fth_object_length(obj))
		FTH_INSTANCE_REF(obj)->cycle = fth_object_length(obj) - 1;
	else
		FTH_INSTANCE_REF(obj)->cycle = idx;
	return (FTH_INSTANCE_REF(obj)->cycle);
}

ficlInteger
fth_cycle_pos_0(FTH obj)
{
	if (INSTANCE_P(obj))
		return (FTH_INSTANCE_REF(obj)->cycle = 0);
	return (0);
}

ficlInteger
fth_cycle_next(FTH obj)
{
	if (INSTANCE_P(obj)) {
		FTH_CYCLE_NEXT(obj);
		return (FTH_INSTANCE_REF(obj)->cycle);
	}
	return (0);
}

FTH
fth_object_cycle_ref(FTH obj)
{
#define h_object_cycle_ref "( obj -- val )  return next value\n\
#( 0 1 2 ) value a\n\
a cycle-ref => 0\n\
a cycle-ref => 1\n\
a cycle-ref => 2\n\
a cycle-ref => 0\n\
a cycle-ref => 1\n\
Return value at current cycle-index of OBJ and increment cycle-index.  \
Cycle through content of OBJ from first to last entry \
and start again at the beginning etc.\n\
See also cycle-set!"
	FTH value;

	if (!INSTANCE_P(obj))
		return (FTH_FALSE);
	value = fth_object_value_ref(obj, FTH_INSTANCE_REF(obj)->cycle);
	FTH_CYCLE_NEXT(obj);
	return (value);
}

FTH
fth_object_cycle_set(FTH obj, FTH value)
{
	if (!INSTANCE_P(obj))
		return (value);
	fth_object_value_set(obj, FTH_INSTANCE_REF(obj)->cycle, value);
	FTH_CYCLE_NEXT(obj);
	return (value);
}

static void
ficl_object_cycle_set(ficlVm *vm)
{
#define h_object_cycle_set "( obj value -- )  store VALUE and incr index\n\
#( 0 1 2 ) value a\n\
a .$ => #( 0 1 2 )\n\
a 10 cycle-set!\n\
a 11 cycle-set!\n\
a 12 cycle-set!\n\
a .$ => #( 10 11 12 )\n\
a 13 cycle-set!\n\
a 14 cycle-set!\n\
a .$ => #( 13 14 12 )\n\
Store VALUE at current cycle-index of OBJ and increment cycle-index.  \
Cycle through content of OBJ from first to last entry \
and start again at the beginning etc.\n\
See also cycle-ref."
	FTH obj, val;

	FTH_STACK_CHECK(vm, 2, 0);
	val = fth_pop_ficl_cell(vm);
	obj = fth_pop_ficl_cell(vm);
	fth_object_cycle_set(obj, val);
}

static void
ficl_cycle_pos_ref(ficlVm *vm)
{
#define h_cycle_pos_ref "( obj -- index )  return cycle-index\n\
#( 1 2 3 ) value a\n\
a cycle-start@ => 0\n\
a cycle-ref => 1\n\
a cycle-start@ => 1\n\
a cycle-ref => 2\n\
a cycle-start@ => 2\n\
a cycle-ref => 3\n\
a cycle-start@ => 0\n\
Return current cycle-index of OBJ.\n\
See also cycle-start! and cycle-start0."
	FTH_STACK_CHECK(vm, 1, 1);
	ficlStackPushInteger(vm->dataStack,
	    fth_cycle_pos_ref(fth_pop_ficl_cell(vm)));
}

static void
ficl_cycle_pos_set(ficlVm *vm)
{
#define h_cycle_pos_set "( obj index -- )  set cycle-index\n\
#( 1 2 3 ) value a\n\
a cycle-start@ => 0\n\
a 2 cycle-start!\n\
a cycle-ref => 3\n\
a cycle-start@ => 0\n\
Set cycle-index of OBJ to INDEX.\n\
See also cycle-start@ and cycle-start0."
	FTH obj;
	ficlInteger idx;

	FTH_STACK_CHECK(vm, 2, 0);
	idx = ficlStackPopInteger(vm->dataStack);
	obj = fth_pop_ficl_cell(vm);
	fth_cycle_pos_set(obj, idx);
}

static void
ficl_cycle_pos_0(ficlVm *vm)
{
#define h_cycle_pos_0 "( obj -- )  set cycle-index to zero\n\
#( 1 2 3 ) value a\n\
a cycle-ref => 1\n\
a cycle-ref => 2\n\
a cycle-start@ => 2\n\
a cycle-start0\n\
a cycle-ref => 1\n\
a cycle-start@ => 1\n\
Set cycle-index of OBJ to zero.\n\
See also cycle-start@ and cycle-start!."
	FTH_STACK_CHECK(vm, 1, 0);
	fth_cycle_pos_0(fth_pop_ficl_cell(vm));
}

static void
ficl_first_object_ref(ficlVm *vm)
{
#define h_first_object_ref "( obj -- val )  return first element\n\
#( 0 1 2 ) first-ref => 0\n\
Return first element of OBJ.  \
Raise OUT-OF-RANGE exception if length of OBJ is less than 1.\n\
See also second-ref, third-ref and last-ref."
	FTH_STACK_CHECK(vm, 1, 1);
	fth_push_ficl_cell(vm,
	    fth_object_value_ref(fth_pop_ficl_cell(vm), 0L));
}

static void
ficl_first_object_set(ficlVm *vm)
{
#define h_first_object_set "( obj value -- )  store VALUE to first element\n\
#( 0 1 2 ) value a1\n\
a1 20 first-set!\n\
a1 .$ => #( 20 1 2 )\n\
Store VALUE to first element of OBJ.  \
Raise an OUT-OF-RANGE exception if length of OBJ is less than 1.\n\
See also second-set!, third-set! and last-set!."
	FTH obj, val;

	FTH_STACK_CHECK(vm, 2, 0);
	val = fth_pop_ficl_cell(vm);
	obj = fth_pop_ficl_cell(vm);
	fth_object_value_set(obj, 0L, val);
}

static void
ficl_second_object_ref(ficlVm *vm)
{
#define h_second_object_ref "( obj -- val )  return second element\n\
#( 0 1 2 ) second-ref => 1\n\
Return second element of OBJ.  \
Raise OUT-OF-RANGE exception if length of OBJ is less than 2.\n\
See also first-ref, third-ref and last-ref."
	FTH_STACK_CHECK(vm, 1, 1);
	fth_push_ficl_cell(vm,
	    fth_object_value_ref(fth_pop_ficl_cell(vm), 1L));
}

static void
ficl_second_object_set(ficlVm *vm)
{
#define h_second_object_set "( obj value -- )  store VALUE to second element\n\
#( 0 1 2 ) value a1\n\
a1 20 second-set!\n\
a1 .$ => #( 0 20 2 )\n\
Store VALUE to second entry of OBJ.  \
Raise OUT-OF-RANGE exception if length of OBJ is less than 2.\n\
See also first-set!, third-set! and last-set!."
	FTH obj, val;

	FTH_STACK_CHECK(vm, 2, 0);
	val = fth_pop_ficl_cell(vm);
	obj = fth_pop_ficl_cell(vm);
	fth_object_value_set(obj, 1L, val);
}

static void
ficl_third_object_ref(ficlVm *vm)
{
#define h_third_object_ref "( obj -- val )  return third element\n\
#( 0 1 2 ) third-ref => 2\n\
Return third entry of OBJ.  \
Raise OUT-OF-RANGE exception if length of OBJ is less than 3.\n\
See also first-ref, second-ref and last-ref."
	FTH_STACK_CHECK(vm, 1, 1);
	fth_push_ficl_cell(vm,
	    fth_object_value_ref(fth_pop_ficl_cell(vm), 2L));
}

static void
ficl_third_object_set(ficlVm *vm)
{
#define h_third_object_set "( obj value -- )  store VALUE to third element\n\
#( 0 1 2 ) value a1\n\
a1 20 third-set!\n\
a1 .$ => #( 0 1 20 )\n\
Store VALUE to third entry of OBJ.  \
Raise OUT-OF-RANGE exception if length of OBJ is less than 3.\n\
See also first-set!, second-set! and last-set!."
	FTH obj, val;

	FTH_STACK_CHECK(vm, 2, 0);
	val = fth_pop_ficl_cell(vm);
	obj = fth_pop_ficl_cell(vm);
	fth_object_value_set(obj, 2L, val);
}

static void
ficl_last_object_ref(ficlVm *vm)
{
#define h_last_object_ref "( obj -- val )  return last element\n\
#( 0 1 2 ) last-ref => 2\n\
Return last entry of OBJ.\n\
Raise OUT-OF-RANGE exception if length of OBJ is less than 1.\n\
See also first-ref, second-ref and third-ref."
	FTH_STACK_CHECK(vm, 1, 1);
	fth_push_ficl_cell(vm,
	    fth_object_value_ref(fth_pop_ficl_cell(vm), -1L));
}

static void
ficl_last_object_set(ficlVm *vm)
{
#define h_last_object_set "( obj value -- )  store VALUE to last element\n\
#( 0 1 2 ) value a1\n\
a1 20 last-set!\n\
a1 .$ => #( 0 1 20 )\n\
Store VALUE to last entry of OBJ.  \
Raise OUT-OF-RANGE exception if length of OBJ is less than 1.\n\
See also first-set!, second-set! and third-set!."
	FTH obj, val;

	FTH_STACK_CHECK(vm, 2, 0);
	val = fth_pop_ficl_cell(vm);
	obj = fth_pop_ficl_cell(vm);
	fth_object_value_set(obj, -1L, val);
}

static void
ficl_object_debug_hook(ficlVm *vm)
{
#define h_object_debug_hook "( obj -- hook|#f )  return debug-hook member\n\
#( 0 1 2 ) value ary\n\
ary .inspect => #<array[3]:  #<fixnum: 0>  #<fixnum: 1>  #<fixnum: 2>>\n\
ary object-debug-hook lambda: <{ str obj -- new-str }>\n\
    $\" debug-inspect: %s\" #( obj ) string-format\n\
; add-hook!\n\
ary .inspect => #<debug-inspect: #( 0 1 2 )>\n\
ary object-debug-hook hook-clear\n\
ary .inspect => #<array[3]:  #<fixnum: 0>  #<fixnum: 1>  #<fixnum: 2>>\n\
Return debug-hook member of OBJ if there is any, otherwise #f.  \
The hook has the stack effect ( inspect-string obj -- new-str ).  \
Every object can set this hook.  \
If set, it will be called on inspecting the object \
with the inspect string as first argument.  \
If there are more than one hook procedures, \
all of them will be called feeded with the new string previously returned."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	if (INSTANCE_P(obj)) {
		if (!FTH_HOOK_P(FTH_INSTANCE_DEBUG_HOOK(obj)))
			FTH_INSTANCE_DEBUG_HOOK(obj) = fth_make_simple_hook(2);
		ficlStackPushFTH(vm->dataStack, FTH_INSTANCE_DEBUG_HOOK(obj));
	} else
		ficlStackPushBoolean(vm->dataStack, false);
}

/* === Predicates === */

static void
ficl_false_p(ficlVm *vm)
{
#define h_false_p "( obj -- f )  test if OBJ is #f\n\
#f false? => #t\n\
-1 false? => #f\n\
0  false? => #f\n\
Return #t if OBJ is #f, otherwise #f.\n\
See also boolean?, true?, nil?, undef?."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, FTH_FALSE_P(obj));
}

static void
ficl_true_p(ficlVm *vm)
{
#define h_true_p "( obj -- f )  test if OBJ is #t\n\
#t true? => #t\n\
-1 true? => #f\n\
0  true? => #f\n\
Return #t if OBJ is #t, otherwise #f.\n\
See also false?, boolean?, nil?, undef?."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, FTH_TRUE_P(obj));
}

static void
ficl_nil_p(ficlVm *vm)
{
#define h_nil_p "( obj -- f )  test if OBJ is nil\n\
nil nil? => #t\n\
0   nil? => #f\n\
#() nil? => #f\n\
'() nil? => #t\n\
Return #t if OBJ is nil, otherwise #f.\n\
See also false?, true?, boolean?, undef?."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, FTH_NIL_P(obj));
}

static void
ficl_undef_p(ficlVm *vm)
{
#define h_undef_p "( obj -- f )  test if OBJ is undef\n\
undef undef? => #t\n\
0     undef? => #f\n\
\"\"    undef? => #f\n\
Return #t if OBJ is undef, otherwise #f.\n\
See also false?, true?, nil?, boolean?."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, FTH_UNDEF_P(obj));
}

static void
ficl_boolean_p(ficlVm *vm)
{
#define h_boolean_p "( obj -- f )  test if OBJ is #t or #f\n\
#t    boolean? => #t\n\
#f    boolean? => #t\n\
nil   boolean? => #f\n\
undef boolean? => #f\n\
-1    boolean? => #f\n\
0     boolean? => #f\n\
Return #t if OBJ is #t or #f, otherwise #f.\n\
See also false?, true?, nil?, undef?."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, FTH_BOOLEAN_P(obj));
}

FTH
ficl_to_fth(FTH obj)
{
	if (obj &&
	    (FICL_WORD_DICT_P(obj) || OBJECT_TYPE_P(obj) || INSTANCE_P(obj)))
		return (obj);
	return (fth_make_int((ficlInteger)obj));
}

FTH
fth_pop_ficl_cell(ficlVm *vm)
{
	ficlStack *stack;
	FTH obj;

	stack = vm->dataStack;
	obj = STACK_FTH_REF(stack);
	stack->top--;
	if (obj &&
	    (FICL_WORD_DICT_P(obj) || OBJECT_TYPE_P(obj) || INSTANCE_P(obj)))
		return (obj);
	return (fth_make_int((ficlInteger)obj));
}

FTH
fth_to_ficl(FTH obj)
{
	if (FTH_FIXNUM_P(obj))
		return ((FTH)FIX_TO_INT(obj));
	return (obj);
}

void
fth_push_ficl_cell(ficlVm *vm, FTH obj)
{
	ficlStack *stack;

	stack = vm->dataStack;
	++stack->top;
	if (FTH_FIXNUM_P(obj))
		STACK_INT_SET(stack, FIX_TO_INT(obj));
	else
		STACK_FTH_SET(stack, obj);
}

void
init_object(void)
{
	fth_print_length = 12;
	fth_gc_on_p = true;
	/* frame */
	FTH_PRI1("frame-depth", ficl_frame_depth, h_frame_depth);
	FTH_PRI1("stack-level", ficl_frame_depth, h_frame_depth);
	FTH_PRI1("backtrace", ficl_backtrace, h_backtrace);
	FTH_PRI1("bt", ficl_backtrace, h_backtrace);
	FTH_PRI1("object-print-length", ficl_print_length, h_print_length);
	FTH_PRI1("set-object-print-length", ficl_set_print_length,
	    h_set_print_length);
	/* gc */
	FTH_PRI1("gc-run", ficl_gc_run, h_gc_run);
	FTH_PRI1("gc-stats", ficl_gc_stats, h_gc_stats);
	FTH_PRI1("gc-marked?", ficl_gc_marked_p, h_gc_marked_p);
	FTH_PRI1("gc-protected?", ficl_gc_protected_p, h_gc_protected_p);
	FTH_PRI1("gc-permanent?", ficl_gc_permanent_p, h_gc_permanent_p);
	FTH_PRI1("gc-protected-objects", ficl_gc_protected_objects,
	    h_gc_protected_objects);
	FTH_PRI1("gc-permanent-objects", ficl_gc_permanent_objects,
	    h_gc_permanent_objects);
	FTH_PROC("gc-on", fth_gc_on, 0, 0, 0, h_gc_on);
	FTH_PROC("gc-off", fth_gc_off, 0, 0, 0, h_gc_off);
	FTH_PRI1("gc-mark", ficl_gc_mark, h_gc_mark);
	FTH_PRI1("gc-unmark", ficl_gc_unmark, h_gc_unmark);
	FTH_PROC("gc-protect", fth_gc_protect, 1, 0, 0, h_gc_protect);
	FTH_PROC("gc-unprotect", fth_gc_unprotect, 1, 0, 0, h_gc_unprotect);
	/* object-type */
	FTH_PRI1("object-type?", ficl_object_type_p, h_object_type_p);
	FTH_PRI1("make-object-type", ficl_make_object_type, h_make_object_type);
	FTH_PRI1("object-type-ref", ficl_object_type_ref, h_object_type_ref);
	FTH_PRI1("object-types", ficl_object_types, h_object_types);
	/* instance */
	FTH_PRI1("instance?", ficl_instance_p, h_instance_p);
	FTH_PRI1("make-instance", ficl_make_instance, h_make_instance);
	FTH_PRI1("instance-gen-ref", ficl_instance_gen_ref, h_instance_gen_ref);
	FTH_PRI1("instance-obj-ref", ficl_instance_obj_ref, h_instance_obj_ref);
	FTH_PRI1("instance-of?", ficl_object_is_instance_of,
	    h_object_is_instance_of);
	/* object set words */
	FTH_PRI1("set-object-inspect", ficl_set_inspect, h_set_insepct);
	FTH_PRI1("set-object->string", ficl_set_to_string, h_set_to_string);
	FTH_PRI1("set-object-dump", ficl_set_dump, h_set_dump);
	FTH_PRI1("set-object->array", ficl_set_to_array, h_set_to_array);
	FTH_PRI1("set-object-copy", ficl_set_copy, h_set_copy);
	FTH_PRI1("set-object-value-ref", ficl_set_value_ref, h_set_value_ref);
	FTH_PRI1("set-object-value-set", ficl_set_value_set, h_set_value_set);
	FTH_PRI1("set-object-equal-p", ficl_set_equal_p, h_set_equal_p);
	FTH_PRI1("set-object-length", ficl_set_length, h_set_length);
	FTH_PRI1("set-object-mark", ficl_set_mark, h_set_mark);
	FTH_PRI1("set-object-free", ficl_set_free, h_set_free);
	FTH_PRI1("set-object-apply", ficl_set_apply, h_set_apply);
	/* special xm.c */
	FTH_PRI1("xmobj?", ficl_xmobj_p, h_xmobj_p);
	/* general object words */
	FTH_PROC("hash-id", fth_hash_id, 1, 0, 0, h_hash_id);
	FTH_PROC("object-id", fth_object_id, 1, 0, 0, h_object_id);
	FTH_PRI1("object-name", ficl_object_name, h_object_name);
	FTH_PRI1(".object-name", ficl_print_object_name, h_print_object_name);
	FTH_PRI1(".inspect", ficl_print_inspect, h_print_inspect);
	FTH_PROC("object-inspect", fth_object_inspect, 1, 0, 0,
	    h_object_inspect);
	FTH_PROC("object->string", fth_object_to_string, 1, 0, 0,
	    h_object_to_string);
	FTH_PROC("object-dump", fth_object_dump, 1, 0, 0,
	    h_object_dump);
	FTH_PROC("object->array", fth_object_to_array, 1, 0, 0,
	    h_object_to_array);
	FTH_PROC("object-copy", fth_object_copy, 1, 0, 0,
	    h_object_copy);
	FTH_PRI1("object-ref", ficl_object_ref, h_object_ref);
	FTH_PRI1("object-set!", ficl_object_set, h_object_set);
	FTH_PRI1("object-set+!", ficl_add_store_object, h_add_store_object);
	FTH_PRI1("object-set-!", ficl_sub_store_object, h_sub_store_object);
	FTH_PRI1("object-set*!", ficl_mul_store_object, h_mul_store_object);
	FTH_PRI1("object-set/!", ficl_div_store_object, h_div_store_object);
	FTH_PRI1("object-equal?", ficl_object_equal_p, h_object_equal_p);
	FTH_PRI1("equal?", ficl_object_equal_p, h_object_equal_p);
	FTH_PRI1("object-length", ficl_object_length, h_object_length);
	FTH_PRI1("length", ficl_object_length, h_object_length);
	FTH_PROC("object-apply", fth_object_apply, 2, 0, 0, h_object_apply);
	FTH_PROC("apply", fth_object_apply, 2, 0, 0, h_object_apply);
	FTH_PRI1("object-empty?", ficl_object_empty_p, h_object_empty_p);
	FTH_PRI1("empty?", ficl_object_empty_p, h_object_empty_p);
	FTH_PRI1("object-range?", ficl_object_range_p, h_object_range_p);
	FTH_PRI1("range?", ficl_object_range_p, h_object_range_p);
	FTH_PRI1("object-member?", ficl_object_member_p, h_object_member_p);
	FTH_PRI1("member?", ficl_object_member_p, h_object_member_p);
	FTH_PROC("object-find", fth_object_find, 2, 0, 0, h_object_find);
	FTH_PROC("detect", fth_object_find, 2, 0, 0, h_object_find);
	FTH_PROC("object-index", fth_object_index, 2, 0, 0, h_object_index);
	FTH_PROC("index", fth_object_index, 2, 0, 0, h_object_index);
	FTH_PROC("object-sort", fth_object_sort, 2, 0, 0, h_object_sort);
	FTH_PROC("sort", fth_object_sort, 2, 0, 0, h_object_sort);
	FTH_PRI1("cycle-start@", ficl_cycle_pos_ref, h_cycle_pos_ref);
	FTH_PRI1("cycle-start!", ficl_cycle_pos_set, h_cycle_pos_set);
	FTH_PRI1("cycle-start0", ficl_cycle_pos_0, h_cycle_pos_0);
	FTH_PROC("cycle-ref", fth_object_cycle_ref, 1, 0, 0,
	    h_object_cycle_ref);
	FTH_PRI1("cycle-set!", ficl_object_cycle_set, h_object_cycle_set);
	FTH_PRI1("first-ref", ficl_first_object_ref, h_first_object_ref);
	FTH_PRI1("first-set!", ficl_first_object_set, h_first_object_set);
	FTH_PRI1("second-ref", ficl_second_object_ref, h_second_object_ref);
	FTH_PRI1("second-set!", ficl_second_object_set, h_second_object_set);
	FTH_PRI1("third-ref", ficl_third_object_ref, h_third_object_ref);
	FTH_PRI1("third-set!", ficl_third_object_set, h_third_object_set);
	FTH_PRI1("last-ref", ficl_last_object_ref, h_last_object_ref);
	FTH_PRI1("last-set!", ficl_last_object_set, h_last_object_set);
	FTH_PRI1("object-debug-hook", ficl_object_debug_hook,
	    h_object_debug_hook);
	/* predicat */
	FTH_PRI1("false?", ficl_false_p, h_false_p);
	FTH_PRI1("true?", ficl_true_p, h_true_p);
	FTH_PRI1("nil?", ficl_nil_p, h_nil_p);
	FTH_PRI1("undef?", ficl_undef_p, h_undef_p);
	FTH_PRI1("boolean?", ficl_boolean_p, h_boolean_p);
	FTH_ADD_FEATURE_AND_INFO(FTH_STR_OBJECT, h_list_of_object_functions);
}

/*
 * object.c ends here
 */
