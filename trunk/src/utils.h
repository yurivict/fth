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
 * @(#)utils.h	2.1 1/2/18
 */

#if !defined(_UTILS_H_)
#define _UTILS_H_

#define PROPERTY_HASH_SIZE	101

/*
 * Shell environment variables and default values:
 *
 * $FTH_INIT_FILE		~/.fthrc
 * $FTH_HISTORY			~/.fth-history
 * $FTH_HISTORY_LENGTH		100
 * $FTH_FTHPATH			""
 * $FTH_LIBPATH			""
 *
 * $FTH_DICTIONARY_SIZE		1024 * 1024
 * $FTH_STACK_SIZE		1024 * 8
 * $FTH_RETURN_SIZE		1024
 * $FTH_LOCALS_SIZE		2048
 */

#define FTH_ENV_INIT_FILE	"FTH_INIT_FILE"
#define FTH_ENV_HIST		"FTH_HISTORY"
#define FTH_ENV_HIST_LEN	"FTH_HISTORY_LENGTH"
#define FTH_ENV_FTHPATH		"FTH_FTHPATH"
#define FTH_ENV_LIBPATH		"FTH_LIBPATH"

#define FTH_ENV_DICTIONARY_SIZE	"FTH_DICTIONARY_SIZE"
#define FTH_ENV_STACK_SIZE	"FTH_STACK_SIZE"
#define FTH_ENV_RETURN_SIZE	"FTH_RETURN_SIZE"
#define FTH_ENV_LOCALS_SIZE	"FTH_LOCALS_SIZE"

#define _FTH_String(X)		#X
#define FTH_XString(X)		_FTH_String(X)

/* proc.c */
#define fth_make_proc_from_vfunc(Name, Func, Req, Opt, Rest)		\
	fth_make_proc_from_func(Name, (FTH (*)())Func, 1, Req, Opt, Rest)

#define FTH_SET_CONSTANT(Name)						\
	fth_define_constant(#Name, (FTH)(Name), NULL)

/*-
 * MAKE_MEMBER(Object, Name, Type, Member)
 *
 * Object = object type, e.g. FArray, FHash etc.
 * Name   = prefix string, e.g. ary, str
 * Type   = what ficl/stack.c provides, e.g. Integer, Float etc.
 * Member = struct member name, e.g. length
 *
 *   MAKE_MEMBER(FArray, ary, Integer, length)
 *
 * creates two static C functions:
 *
 *   cb_ary_length()
 *   init_ary_length()
 *
 * The latter can be called e.g. in init_array() to create the Ficl
 * word:
 *
 *   ary->length
 *
 */
#define MAKE_MEMBER(Object, Name, Type, Member)				\
static void								\
cb_ ## Name ## _ ## Member(ficlVm *vm)					\
{									\
	FTH		obj;						\
									\
	FTH_STACK_CHECK(vm, 1, 1);					\
	obj = fth_pop_ficl_cell(vm);					\
									\
	if (fth_instance_p(obj))					\
		ficlStackPush ## Type(vm->dataStack,			\
		    FTH_INSTANCE_REF_GEN(obj, Object)->Member);		\
	else								\
		ficlStackPushBoolean(vm->dataStack, 0);			\
}									\
static void								\
init_ ## Name ## _ ## Member(void)					\
{									\
	ficlDictionaryAppendPrimitive(FTH_FICL_DICT(),			\
	    #Name "->" #Member, cb_ ## Name ## _ ## Member,		\
	    FICL_WORD_DEFAULT);						\
}

/* Simple Array */
typedef struct {
	void          **data;
	unsigned	length;
	unsigned	incr;
} simple_array;

/* === IO === */

#if defined(HAVE_SYS_SOCKET_H)
#include <sys/socket.h>
#endif
#if defined(HAVE_SYS_UN_H)
#include <sys/un.h>
#endif
#if defined(HAVE_NETINET_IN_H)
#include <netinet/in.h>
#endif
#if defined(HAVE_NETDB_H)
#include <netdb.h>
#endif
#if defined(HAVE_SYS_UIO_H)
#include <sys/uio.h>
#endif

#if !defined(HAVE_STRUCT_SOCKADDR_UN) || defined(_WIN32)
#define HAVE_SOCKET		0
#define FTH_DEFAULT_ADDRFAM	0
#else				/* !_WIN32 */
#define HAVE_SOCKET		1	
#if defined(AF_INET)
#define FTH_DEFAULT_ADDRFAM	AF_INET
#else
#define FTH_DEFAULT_ADDRFAM	2
#endif
#endif				/* _WIN32 */

#define FTH_DEFAULT_PORT	1024

/* Keywords for xxx_open(:if-exists). */
#define FTH_KEYWORD_ERROR	fth_keyword("error")
#define FTH_KEYWORD_RENAME	fth_keyword("rename")
#define FTH_KEYWORD_OVERWRITE	fth_keyword("overwrite")

typedef enum {
	FTH_IO_UNDEF,
	FTH_IO_FILE,
	FTH_IO_PIPE,
	FTH_IO_SOCKET,
	FTH_IO_STRING,
	FTH_IO_PORT
} io_t;

typedef struct {
	io_t 		type;
	FTH 		name;
	FTH 		filename;
	FTH 		buffer;
	int 		fam;
	void           *data;
	ficl2Integer 	length;
	int 		input_p;
	int 		output_p;
	int 		closed_p;
	int             (*read_char) (void *);
	void            (*write_char) (void *, int);
	char           *(*read_line) (void *);
	void            (*write_line) (void *, const char *);
	int             (*eof_p) (void *);
	ficl2Integer    (*tell) (void *);
	ficl2Integer    (*seek) (void *, ficl2Integer, int);
	void            (*flush) (void *);
	void            (*rewind) (void *);
	void            (*close) (void *);
} FIO;

#define FTH_IO_OBJECT(Obj)	FTH_INSTANCE_REF_GEN(Obj, FIO)

#define FTH_IO_TYPE(Obj)	FTH_IO_OBJECT(Obj)->type
#define FTH_IO_NAME(Obj)	FTH_IO_OBJECT(Obj)->name
#define FTH_IO_FILENAME(Obj)	FTH_IO_OBJECT(Obj)->filename
#define FTH_IO_FAM(Obj)		FTH_IO_OBJECT(Obj)->fam
#define FTH_IO_DATA(Obj)	FTH_IO_OBJECT(Obj)->data
#define FTH_IO_LENGTH(Obj)	FTH_IO_OBJECT(Obj)->length
#define FTH_IO_BUFFER(Obj)	FTH_IO_OBJECT(Obj)->buffer
#define FTH_IO_INPUT_P(Obj)	FTH_IO_OBJECT(Obj)->input_p
#define FTH_IO_OUTPUT_P(Obj)	FTH_IO_OBJECT(Obj)->output_p
#define FTH_IO_CLOSED_P(Obj)	FTH_IO_OBJECT(Obj)->closed_p

#define FTH_IO_READ_CHAR(Obj)						\
	(*FTH_IO_OBJECT(Obj)->read_char)(FTH_IO_DATA(Obj))
#define FTH_IO_WRITE_CHAR(Obj, val)					\
	(*FTH_IO_OBJECT(Obj)->write_char)(FTH_IO_DATA(Obj), val)
#define FTH_IO_READ_LINE(Obj)						\
	(*FTH_IO_OBJECT(Obj)->read_line)(FTH_IO_DATA(Obj))
#define FTH_IO_WRITE_LINE(Obj, val)					\
	(*FTH_IO_OBJECT(Obj)->write_line)(FTH_IO_DATA(Obj), val)
#define FTH_IO_EOF_P(Obj)						\
	(*FTH_IO_OBJECT(Obj)->eof_p)(FTH_IO_DATA(Obj))
#define FTH_IO_TELL(Obj)						\
	(*FTH_IO_OBJECT(Obj)->tell)(FTH_IO_DATA(Obj))
#define FTH_IO_SEEK(Obj, pos, w)					\
	(*FTH_IO_OBJECT(Obj)->seek)(FTH_IO_DATA(Obj), pos, w)
#define FTH_IO_FLUSH(Obj)						\
	(*FTH_IO_OBJECT(Obj)->flush)(FTH_IO_DATA(Obj))
#define FTH_IO_REWIND(Obj)						\
	(*FTH_IO_OBJECT(Obj)->rewind)(FTH_IO_DATA(Obj))
#define FTH_IO_CLOSE(Obj)						\
	(*FTH_IO_OBJECT(Obj)->close)(FTH_IO_DATA(Obj))

#define keyword_args_string "\
The following keyword arguments exist:\n\
:filename    file name (string)                     => file IO\n\
   :fam          default r/o\n\
:command     cmd       (string or array-of-strings) => pipe IO\n\
   :fam          default r/o\n\
:string      string    (string)                     => string IO\n\
   :fam          default r/o\n\
:socket      host      (string)                     => socket IO\n\
   :domain       default AF_INET\n\
   :port         default 1024\n\
:soft-port   port name (string)                     => soft-port IO\n\
   :fam          default r/o\n\
   :port-name    default \"soft-port-name\"\n\
   :read-char    proc ( -- c )\n\
   :write-char   proc ( c -- )\n\
   :read-line    proc ( -- line )\n\
   :write-line   proc ( line -- )\n\
   :flush        proc ( -- )\n\
   :close        proc ( -- )\n\
See also io-open-file, io-open-input-file, io-open-output-file, \
with-input-port, with-output-port, with-input-from-port, \
with-output-to-port, with-error-to-port."

__BEGIN_DECLS

void		init_gc(void);
void		init_array_type(void);
void		init_hash_type(void);
void		init_io_type(void);
void		init_hook_type(void);
void		init_string_type(void);
void		init_regexp_type(void);
void		init_number_types(void);
#if HAVE_BN
void		free_number_types(void);
#endif
void		init_array(void);
void		init_hash (void);
void		init_io   (void);
void		init_port (void);
void		init_file (void);
void		init_number(void);
void		init_object(void);
void		init_proc (void);
void		init_hook (void);
void		init_string(void);
void		init_regexp(void);
void		init_symbol(void);
void		init_utils(void);

/* array.c */
/* Next two have no bound checks! */
FTH		fth_array_fast_set(FTH, ficlInteger, FTH);
FTH		fth_array_fast_ref(FTH, ficlInteger);

/* io.c */
FTH		make_io_base(int);

/* misc.c */
void		forth_init(void);
void		forth_init_before_load(void);
extern int	fth_signal_caught_p;
#if !defined(_WIN32)
extern sigjmp_buf fth_sig_toplevel;
void		signal_check(int);
#endif
void		fth_reset_loop_and_depth(void);

/* numbers.c */
int		ficl_parse_number(ficlVm *, ficlString);
#if HAVE_COMPLEX
int		ficl_parse_complex(ficlVm *, ficlString);
#endif
#if HAVE_BN
int		ficl_parse_bignum(ficlVm *, ficlString);
int		ficl_parse_ratio(ficlVm *, ficlString);
#endif

/* object.c */
FTH		make_object_type(const char *, fobj_t);
FTH		make_object_type_from(const char *, fobj_t, FTH);

void 		gc_free_all(void);
void 		gc_push(ficlWord *);
void 		gc_pop(void);
void 		gc_loop_reset(void);
void 		fth_set_backtrace(FTH);
void 		fth_show_backtrace(int);

/* port.c */
FTH		io_keyword_args_ref(int);

/* proc.c */
FTH		fth_word_dump(FTH);
FTH		fth_word_inspect(FTH);
FTH		fth_word_to_source(ficlWord *);
FTH		fth_word_to_string(FTH);
void		ficl_init_locals(ficlVm *, ficlDictionary *);

/* string.c */
/* Next two have no bound checks! */
char		fth_string_c_char_fast_ref(FTH, ficlInteger);
char		fth_string_c_char_fast_set(FTH, ficlInteger, char);
/* Doesn't remove sep_str. */
FTH		fth_string_split_2(FTH, FTH);

/* symbol.c */
FTH		ficl_ans_real_exc(int);

/* utils.c */
simple_array   *make_simple_array(int);
simple_array   *make_simple_array_var(int,...);
int		simple_array_length(simple_array *);
int		simple_array_equal_p(simple_array *, simple_array *);
void           *simple_array_ref(simple_array *, int);
void		simple_array_set(simple_array *, int, void *);
void		simple_array_push(simple_array *, void *);
void           *simple_array_pop(simple_array *);
int		simple_array_index(simple_array *, void *);
int		simple_array_rindex(simple_array *, void *);
int		simple_array_member_p(simple_array *, void *);
void           *simple_array_delete(simple_array *, void *);
void           *simple_array_rdelete(simple_array *, void *);
simple_array   *simple_array_reverse(simple_array *);
simple_array   *simple_array_clear(simple_array *);
void		simple_array_free(simple_array *);
FTH		simple_array_to_array(simple_array *);

void		push_forth_string(ficlVm *, char *);

char           *parse_input_buffer(ficlVm *, char *);
char           *parse_tib_with_restart(ficlVm *, char *, int,
		    ficlString (*) (ficlVm *, int));

__END_DECLS

#endif				/* _UTILS_H_ */

/*
 * utils.h ends here
 */
