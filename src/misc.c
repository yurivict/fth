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
 */

#if !defined(lint)
const char libfth_sccsid[] = "@(#)misc.c	1.688 12/3/17";
#endif /* not lint */

#define FTH_DATE		"2017/12/03"

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "fth.h"
#include "utils.h"

#if defined(HAVE_SYS_WAIT_H)
#include <sys/wait.h>
#endif
#if defined(HAVE_FCNTL_H)
#include <fcntl.h>
#endif
#if defined(HAVE_SYS_STAT_H)
#include <sys/stat.h>
#endif
#if defined(HAVE_DLFCN_H)
#include <dlfcn.h>
#endif
#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif
#if defined(HAVE_TIME_H)
#include <time.h>
#endif
#if defined(HAVE_SYS_TIMES_H)
#include <sys/times.h>
#endif
#if defined(HAVE_NETINET_IN_H)
#include <netinet/in.h>
#endif
#if defined(HAVE_SIGNAL_H)
#include <signal.h>
#endif

#define FTH_INIT_FILE		".fthrc"
#define FTH_PATH_SEPARATOR	":"

#define FTH_GLOBAL_INIT_FILE	FTH_PREFIX_PATH "/etc/fth.conf"
#define FTH_SO_LIB_PATH		FTH_PREFIX_PATH "/lib/" FTH_PROG_NAME

#define FTH_SHARE_PATH		FTH_PREFIX_PATH "/share/" FTH_PROG_NAME
#define FTH_FS_LIB_PATH		FTH_SHARE_PATH "/fth-lib"
#define FTH_SITE_FTH_PATH	FTH_SHARE_PATH "/site-fth"

static bool	apropos(ficlWord *word, FTH data);
static FTH	at_exit_each(FTH proc, FTH name);
static void	ficl_add_feature(ficlVm *vm);
static void	ficl_add_load_lib_path(ficlVm *vm);
static void	ficl_add_load_path(ficlVm *vm);
static void	ficl_at_exit(ficlVm *vm);
static void	ficl_catch(ficlVm *vm);
static void	ficl_config_cflags(ficlVm *vm);
static void	ficl_config_libs(ficlVm *vm);
static void	ficl_config_prefix(ficlVm *vm);
static void	ficl_configure_args(ficlVm *vm);
static void	ficl_current_time(ficlVm *vm);
static void	ficl_date(ficlVm *vm);
static void	ficl_dl_load(ficlVm *vm);
static void	ficl_dot_cflags(ficlVm *vm);
static void	ficl_dot_libs(ficlVm *vm);
static void	ficl_dot_lversion(ficlVm *vm);
static void	ficl_dot_prefix(ficlVm *vm);
static void	ficl_dot_version(ficlVm *vm);
static void	ficl_environ(ficlVm *vm);
static void	ficl_exec(ficlVm *vm);
static void	ficl_exit(ficlVm *vm);
static void	ficl_features(ficlVm *vm);
static void	ficl_fetch_paren(ficlVm *vm);
static void	ficl_fork(ficlVm *vm);
static void	ficl_getegid(ficlVm *vm);
static void	ficl_getenv(ficlVm *vm);
static void	ficl_geteuid(ficlVm *vm);
static void	ficl_getgid(ficlVm *vm);
static void	ficl_gethostname(ficlVm *vm);
static void	ficl_getlogin(ficlVm *vm);
static void	ficl_getopt(ficlVm *vm);
static void	ficl_getopt_long(ficlVm *vm);
static void	ficl_getpid(ficlVm *vm);
static void	ficl_getppid(ficlVm *vm);
static void	ficl_getservbyname(ficlVm *vm);
static void	ficl_getservbyport(ficlVm *vm);
static void	ficl_getuid(ficlVm *vm);
static void	ficl_gmtime(ficlVm *vm);
static void	ficl_include_file(ficlVm *vm);
static void	ficl_kill(ficlVm *vm);
static void	ficl_load_init_file(ficlVm *vm);
static void	ficl_localtime(ficlVm *vm);
static void	ficl_mktime(ficlVm *vm);
static void	ficl_provided_p(ficlVm *vm);
static void	ficl_putenv(ficlVm *vm);
static void	ficl_raise(ficlVm *vm);
static void	ficl_require_file(ficlVm *vm);
static void	ficl_reset_each_paren(ficlVm *vm);
static void	ficl_reset_map_paren(ficlVm *vm);
static void	ficl_set_begin_paren(ficlVm *vm);
static void	ficl_set_each_loop_paren(ficlVm *vm);
static void	ficl_set_end_paren(ficlVm *vm);
static void	ficl_set_map_loop_paren(ficlVm *vm);
static void	ficl_setegid(ficlVm *vm);
static void	ficl_seteuid(ficlVm *vm);
static void	ficl_setgid(ficlVm *vm);
static void	ficl_sethostname(ficlVm *vm);
static void	ficl_setuid(ficlVm *vm);
static void	ficl_show_memory(ficlVm *vm);
static void	ficl_signal(ficlVm *vm);
static void	ficl_signal_handler(ficlVm *vm);
static void	ficl_sleep(ficlVm *vm);
static void	ficl_stack_reset(ficlVm *vm);
static void	ficl_store_paren(ficlVm *vm);
static void	ficl_strftime(ficlVm *vm);
static void	ficl_strptime(ficlVm *vm);
static void	ficl_throw(ficlVm *vm);
static void	ficl_time(ficlVm *vm);
static void	ficl_time_reset(ficlVm *vm);
static void	ficl_time_to_string(ficlVm *vm);
static void	ficl_unshift_load_lib_path(ficlVm *vm);
static void	ficl_unshift_load_path(ficlVm *vm);
static void	ficl_utime(ficlVm *vm);
static void	ficl_values_end(ficlVm *vm);
static void	ficl_version(ficlVm *vm);
static void	ficl_wait(ficlVm *vm);
static void	ficl_waitpid(ficlVm *vm);
static bool	find_in_wordlist(ficlWord *word, FTH data);
static void	forth_pre_init(void);
static RETSIGTYPE fth_toplevel_handler(int sig);
static void	handler_exec(int sig);
static FTH	load_file(const char *name, const char *caller);
#if defined(HAVE_DLOPEN)
static FTH	load_lib(const char *name, const char *func,
		    const char *caller);
#endif
static void	run_at_exit(void);
static void	set_and_show_signal_backtrace(int kind);

/* global ficl variable; access via FTH_FICL_VAR() etc (fth.h) */
Ficl           *fth_ficl = NULL;

/*
 * exit callback void (*)(int);
 * snd/xen.c use it.
 */
exit_cb		fth_exit_hook;

/* === Init FTH === */

#if !defined(_WIN32)
sigjmp_buf	fth_sig_toplevel;

static RETSIGTYPE
fth_toplevel_handler(int sig)
{
	siglongjmp(fth_sig_toplevel, sig);
}

#endif

#if !defined(HAVE_SIG_T)
typedef void    (*sig_t)(int sig);
#endif

#if !defined(SIG_DFL)
#define SIG_DFL		((sig_t)0)
#endif
#if !defined(SIG_IGN)
#define SIG_IGN		((sig_t)1)
#endif
#if !defined(SIG_ERR)
#define SIG_ERR		((sig_t)-1)
#endif

/* required in object.c */
bool		fth_signal_caught_p;

#if !defined(_WIN32)
static void
set_and_show_signal_backtrace(int kind)
{
	fth_set_backtrace(FTH_SIGNAL_CAUGHT);
	fth_show_backtrace(true);

	switch (kind) {
	case EXIT_SUCCESS:
	case EXIT_FAILURE:
		fth_exit(kind);
		break;
	case EXIT_ABORT:
	default:
		if (fth_die_on_signal_p || !fth_interactive_p)
			abort();
		break;
	}
}

void
signal_check(int sig)
{
	char *func;
	sig_t cb;

	func = RUNNING_WORD();
	cb = signal(sig, SIG_DFL);

	switch (sig) {
	case SIGINT:		/* C-c break */
		fth_printf("\n#<%s: break (C-c)>\n", func);
		if (!fth_interactive_p)
			set_and_show_signal_backtrace(EXIT_SUCCESS);
		break;
	case SIGUSR1:		/* C-g abort */
		fth_printf("\n#<%s: abort (C-g)>\n", func);
		if (!fth_interactive_p)
			set_and_show_signal_backtrace(EXIT_SUCCESS);
		break;
	case SIGQUIT:		/* C-\ quit */
		fth_printf("\n#<%s: quit (C-\\)>\n", func);
		fth_exit(EXIT_SUCCESS);
		break;
	case SIGFPE:
		fth_warning("%s => %s", func, strsignal(sig));
#if defined(FTH_DEBUG)
		set_and_show_signal_backtrace(EXIT_ABORT);
#endif
		break;
	default:
		fth_signal_caught_p = true;
		fth_errorf("#<%s: %s>\n", func, strsignal(sig));
		set_and_show_signal_backtrace(EXIT_ABORT);
		/*
		 * Skip reset signal handler if not aborted.
		 */
		return;
		break;
	}
	fth_signal_caught_p = false;
	signal(sig, cb);
}

#define FTH_SIGNALS 7
static int	fth_signals[] = {
	SIGINT, SIGQUIT, SIGILL, SIGSEGV, SIGBUS, SIGFPE, SIGUSR1
};

#endif				/* !_WIN32 */

#define INIT_ASSERT(Cond)						\
	if (!(Cond)) {							\
		fprintf(stderr, "fth: init failed in %s[%d]\n",		\
		    __FILE__, __LINE__);				\
		abort();						\
	}

void
fth_make_ficl(unsigned int dict_size, unsigned int stack_size,
    unsigned int return_size, unsigned int locals_size)
{
#if !defined(_WIN32)
	int i;
#endif

	if (FTH_FICL_VAR() == NULL) {
		ficlSystemInformation fsi;
		ficlSystem *sys;
		ficlVm *vm;

		FTH_FICL_VAR() = FTH_MALLOC(sizeof(Ficl));
		fth_last_exception = 0L;
		fth_current_file = 0L;
		fth_current_line = -1;
		fth_print_p = false;
		fth_eval_p = false;
		fth_hit_error_p = false;
		fth_true_repl_p = true;
		fth_die_on_signal_p = false;
		fth_interactive_p = false;
		fth_signal_caught_p = false;
		ficlSystemInformationInitialize(&fsi);
		fsi.context = NULL;
		fsi.dictionarySize = dict_size;
		fsi.stackSize = stack_size;
		fsi.returnSize = return_size;
		fsi.localsSize = locals_size;
		fsi.environmentSize = FICL_DEFAULT_ENVIRONMENT_SIZE;
		/* we use our own io-functions from port.c */
		fsi.textIn = NULL;
		fsi.textOut = NULL;
		fsi.errorOut = NULL;
		/* unistd.h */
		fsi.stdin_fileno = STDIN_FILENO;
		fsi.stdout_fileno = STDOUT_FILENO;
		fsi.stderr_fileno = STDERR_FILENO;
		/* stdio.h */
		fsi.stdin_ptr = stdin;
		fsi.stdout_ptr = stdout;
		fsi.stderr_ptr = stderr;
		sys = ficlSystemCreate(&fsi);
		INIT_ASSERT(sys);
		FTH_FICL_SYSTEM() = sys;
		vm = ficlSystemCreateVm(sys);
		INIT_ASSERT(vm);
		FTH_FICL_VM() = vm;
		INIT_ASSERT(FTH_FICL_DICT());
		INIT_ASSERT(FTH_FICL_ENV());
		INIT_ASSERT(FTH_FICL_STACK());
		fth_set_read_cb(NULL);
		fth_set_print_cb(NULL);
		fth_set_error_cb(NULL);
	}
#if !defined(_WIN32)
	for (i = 0; i < FTH_SIGNALS; i++)
		signal(fth_signals[i], fth_toplevel_handler);
#endif
}

static FTH	load_path;
static FTH	load_lib_path;
static FTH	loaded_files;
static FTH	before_load_hook;
static FTH	after_load_hook;
static FTH	eval_string;
static FTH	fth_at_exit_procs;
static simple_array *depth_array;
static simple_array *loop_array;

static char	misc_scratch[MAXPATHLEN];
static char	misc_scratch_02[MAXPATHLEN];
static char	misc_scratch_03[MAXPATHLEN];
static char	misc_scratch_04[MAXPATHLEN];

/*
 * Boolean and Nil object types.
 */
static void	init_boolean_type(void);
static FTH	bl_inspect(FTH self);
static FTH	bl_to_string(FTH self);

enum {
	BOOLEAN_FALSE,
	BOOLEAN_TRUE,
	BOOLEAN_NIL,
	BOOLEAN_UNDEF
};

#define B_ISTR_FALSE		"#f"
#define B_ISTR_TRUE		"#t"
#define B_ISTR_NIL		"nil"
#define B_ISTR_UNDEF		"undef"
#define B_SSTR_NIL		"#<" B_ISTR_NIL ">"
#define B_SSTR_UNDEF		"#<" B_ISTR_UNDEF ">"

static FTH	b_istr_false;
static FTH	b_istr_true;
static FTH	b_istr_nil;
static FTH	b_istr_undef;
static FTH	b_sstr_nil;
static FTH	b_sstr_undef;

static FTH
bl_inspect(FTH self)
{
	FTH fs;

	fs = fth_make_string(FTH_INSTANCE_NAME(self));
	switch(FTH_INT_OBJECT(self)) {
	case BOOLEAN_FALSE:
		return (fth_string_sformat(fs, ": %S", b_istr_false));
		break;
	case BOOLEAN_TRUE:
		return (fth_string_sformat(fs, ": %S", b_istr_true));
		break;
	case BOOLEAN_NIL:
		return (fth_string_sformat(fs, ": %S", b_istr_nil));
		break;
	case BOOLEAN_UNDEF:
	default:
		return (fth_string_sformat(fs, ": %S", b_istr_undef));
		break;
	}
}

static FTH
bl_to_string(FTH self)
{
	switch(FTH_INT_OBJECT(self)) {
	case BOOLEAN_FALSE:
		return (b_istr_false);
		break;
	case BOOLEAN_TRUE:
		return (b_istr_true);
		break;
	case BOOLEAN_NIL:
		return (b_sstr_nil);
		break;
	case BOOLEAN_UNDEF:
	default:
		return (b_sstr_undef);
		break;
	}
}

static void
init_boolean_type(void)
{
	FTH boolean_tag; 
	FTH nil_tag;	

	boolean_tag = make_object_type(FTH_STR_BOOLEAN, FTH_BOOLEAN_T);
	fth_set_object_inspect(boolean_tag, bl_inspect);
	fth_set_object_to_string(boolean_tag, bl_to_string);
	/*
	 * Boolean object type.
	 */
	FTH_FALSE = fth_make_instance(boolean_tag, NULL);
	FTH_INT_OBJECT_SET(FTH_FALSE, BOOLEAN_FALSE);
	FTH_TRUE = fth_make_instance(boolean_tag, NULL);
	FTH_INT_OBJECT_SET(FTH_TRUE, BOOLEAN_TRUE);
	/*
	 * Nil object type.
	 */
	nil_tag = make_object_type(FTH_STR_NIL, FTH_NIL_T);
	fth_set_object_inspect(nil_tag, bl_inspect);
	fth_set_object_to_string(nil_tag, bl_to_string);
	FTH_NIL = fth_make_instance(nil_tag, NULL);
	FTH_INT_OBJECT_SET(FTH_NIL, BOOLEAN_NIL);
	FTH_UNDEF = fth_make_instance(nil_tag, NULL);
	FTH_INT_OBJECT_SET(FTH_UNDEF, BOOLEAN_UNDEF);
	/*
	 * #f, #t, nil, undef
	 */
	fth_define(B_ISTR_FALSE, FTH_FALSE);
	fth_define(B_ISTR_TRUE, FTH_TRUE);
	fth_define(B_ISTR_NIL, FTH_NIL);
	fth_define(B_ISTR_UNDEF, FTH_UNDEF);
}

static void
forth_pre_init(void)
{
	INIT_ASSERT(FTH_FICL_VAR());	
	/*
	 * Theses object types are required before defining further
	 * functions.
	 */
	init_gc();
	init_boolean_type();
	init_array_type();
	init_hash_type();
	init_io_type();
	init_hook_type();
	init_string_type();
	init_regexp_type();
	init_number_types();
	b_istr_false = fth_gc_permanent(fth_make_string(B_ISTR_FALSE));
	b_istr_true = fth_gc_permanent(fth_make_string(B_ISTR_TRUE));
	b_istr_nil = fth_gc_permanent(fth_make_string(B_ISTR_NIL));
	b_istr_undef = fth_gc_permanent(fth_make_string(B_ISTR_UNDEF));
	b_sstr_nil = fth_gc_permanent(fth_make_string(B_SSTR_NIL));
	b_sstr_undef = fth_gc_permanent(fth_make_string(B_SSTR_UNDEF));
	fth_current_file = FTH_FALSE;
	fth_last_exception = FTH_FALSE;
	fth_at_exit_procs = FTH_FALSE;
	/* global Forth variables, protected with fth_define_variable() */
	loaded_files = fth_make_empty_array();
	load_path = fth_make_empty_array();
	load_lib_path = fth_make_empty_array();
	/* global C variables */
	depth_array = make_simple_array(8);
	loop_array = make_simple_array(8);
	/* fth_current_file in fth_eval() */
	eval_string = fth_make_string("eval");

	{
		char *libs, *pname;

		/* Set path for Forth script files: */
		libs = fth_getenv(FTH_ENV_FTHPATH, NULL);
		if (libs != NULL)
			while ((pname = strsep(&libs, FTH_PATH_SEPARATOR)))
				if (pname[0] != '\0')
					fth_add_load_path(pname);
		fth_add_load_path(FTH_FS_LIB_PATH);
		fth_add_load_path(FTH_SITE_FTH_PATH);

		/* Set path for C dynamic so libraries: */
		libs = fth_getenv(FTH_ENV_LIBPATH, NULL);
		if (libs != NULL)
			while ((pname = strsep(&libs, FTH_PATH_SEPARATOR)))
				if (pname[0] != '\0')
					fth_add_load_lib_path(pname);
		fth_add_load_lib_path(FTH_SO_LIB_PATH);
	}

	/* variables */
	fth_define_variable("*load-path*", load_path,
	    "( -- load-path-array )");
	fth_define_variable("*load-lib-path*", load_lib_path,
	    "( -- load-path-lib-array )");
	fth_define_variable("*loaded-files*", loaded_files,
	    "( -- files-array )");
	before_load_hook = fth_make_hook("before-load-hook", 1,
	    "( filename -- f )  \
Called before loading FILENAME.  \
If hook returns #f, FILENAME won't be loaded.\n\
before-load-hook lambda: <{ fname -- f }>\n\
  \"\\\\ loading %s\\n\" #( fname ) fth-print\n\
  #t\n\
; add-hook!");
	after_load_hook = fth_make_hook("after-load-hook", 1,
	    "( filename -- )  \
Called after loading FILENAME and updating global variable *loaded-files*.\n\
after-load-hook lambda: <{ fname -- }>\n\
  \"\\\\ %s loaded\\n\" #( fname ) fth-print\n\
; add-hook!");
#if defined(FTH_STATIC)
	fth_add_feature("static");
#endif
	fth_add_feature(FTH_STR_BOOLEAN);
	fth_add_feature(FTH_STR_NIL);
	fth_add_feature(FICL_NAME);
	fth_add_feature(FICL_FORTH_NAME);
	fth_add_feature(FTH_TARGET_CPU);

	{
		char *s, *p, *r;

		s = misc_scratch;
		fth_strcpy(s, sizeof(misc_scratch), FTH_TARGET_OS);
		p = s;
		/*-
	         * Operating system name without version number.
	         * 'minix provided? if
	         *   ...
	         * else
	         *   'netbsd provided? if ... else ... then
	         * then
	         */
		r = strsep(&p, "0123456789-_");
		if (r != NULL)
			fth_add_feature(r);
	}

	ficlSystemCompileCore(FTH_FICL_SYSTEM());
	ficlSystemCompilePrefix(FTH_FICL_SYSTEM());
	ficlSystemCompilePlatform(FTH_FICL_SYSTEM());
	ficlSystemAddPrimitiveParseStep(FTH_FICL_SYSTEM(),
	    "?word", ficlVmParseWord);
	ficlSystemAddPrimitiveParseStep(FTH_FICL_SYSTEM(),
	    "?prefix", ficlVmParsePrefix);
	ficlSystemAddPrimitiveParseStep(FTH_FICL_SYSTEM(),
	    "?number", ficl_parse_number);
#if HAVE_COMPLEX
	ficlSystemAddPrimitiveParseStep(FTH_FICL_SYSTEM(),
	    "?complex", ficl_parse_complex);
#endif
#if HAVE_BN
	ficlSystemAddPrimitiveParseStep(FTH_FICL_SYSTEM(),
	    "?bignum", ficl_parse_bignum);
	ficlSystemAddPrimitiveParseStep(FTH_FICL_SYSTEM(),
	    "?ratio", ficl_parse_ratio);
#endif				/* HAVE_BN */
	init_object();
	init_proc();
	init_array();
	init_hash();
	init_io();
	init_file();
	init_port();
	init_number();
	init_hook();
	init_string();
	init_regexp();
	init_symbol();
	init_utils();
	fth_define_variable("*fth-verbose*", FTH_FALSE, NULL);
	fth_define_variable("*fth-debug*", FTH_FALSE, NULL);
	fth_current_file = fth_make_string("-");
	fth_current_line = -1;
}

/*
 * Shell environment variables.
 *
 * $FTH_DICTIONARY_SIZE
 * $FTH_STACK_SIZE
 * $FTH_RETURN_SIZE
 * $FTH_LOCALS_SIZE
 */

/* splitted init function for fth.c */
void
forth_init_before_load(void)
{
	unsigned int dict_size, stack_size, return_size, locals_size;
	char *env;

	dict_size = FICL_DEFAULT_DICTIONARY_SIZE;
	stack_size = FICL_DEFAULT_STACK_SIZE;
	return_size = FICL_DEFAULT_RETURN_SIZE;
	locals_size = FICL_MAX_LOCALS;
	env = fth_getenv(FTH_ENV_DICTIONARY_SIZE, NULL);
	if (env != NULL)
		dict_size = (unsigned int)strtol(env, NULL, 10);
	env = fth_getenv(FTH_ENV_STACK_SIZE, NULL);
	if (env != NULL)
		stack_size = (unsigned int)strtol(env, NULL, 10);
	env = fth_getenv(FTH_ENV_RETURN_SIZE, NULL);
	if (env != NULL)
		return_size = (unsigned int)strtol(env, NULL, 10);
	env = fth_getenv(FTH_ENV_LOCALS_SIZE, NULL);
	if (env != NULL)
		locals_size = (unsigned int)strtol(env, NULL, 10);
	fth_make_ficl(dict_size, stack_size, return_size, locals_size);
	forth_pre_init();
}

/*
 * Init libfth.so.
 *
 * This function must be called before any libfth.so action can take
 * place.
 *
 * Used for example in snd.c and xen.c (snd(1)).
 */
void
fth_init(void)
{
	forth_init_before_load();
	forth_init();
}

void
fth_reset(void)
{
#if !defined(_WIN32)
	int i;

	for (i = 0; i < FTH_SIGNALS; i++)
		signal(fth_signals[i], SIG_DFL);
#endif
	ficlVmDestroy(FTH_FICL_VM());
	FTH_FICL_VAR() = NULL;
	fth_init();
}

void
fth_exit(int n)
{
	if (fth_exit_hook != NULL)
		(*fth_exit_hook)(n);
	exit(n);
}

/* === EVAL === */

int
fth_catch_exec(ficlWord *word)
{
	int status;

	if (word == NULL)
		return (FTH_OKAY);
	gc_push(word);
	status = ficlVmExecuteXT(FTH_FICL_VM(), word);
	gc_pop();
	switch (status) {
	case FICL_VM_STATUS_INNER_EXIT:
	case FICL_VM_STATUS_OUT_OF_TEXT:
	case FICL_VM_STATUS_RESTART:
	case FICL_VM_STATUS_BREAK:
	case FICL_VM_STATUS_ABORT:
	case FICL_VM_STATUS_ABORTQ:
	case FICL_VM_STATUS_QUIT:
		return (FTH_OKAY);
		break;
	case FICL_VM_STATUS_USER_EXIT:
		return (FTH_BYE);
		break;
	case FICL_VM_STATUS_ERROR_EXIT:
	default:
		return (FTH_ERROR);
		break;
	}
}

int
fth_catch_eval(const char *buffer)
{
	int status;
	ficlCell id;
	ficlString s;
	ficlVm *vm;
	char *buf;

	if (buffer == NULL)
		return (FTH_OKAY);
	vm = FTH_FICL_VM();
	id = vm->sourceId;
	CELL_INT_SET(&vm->sourceId, -1);
	buf = FTH_STRDUP(buffer);
	FICL_STRING_SET_FROM_CSTRING(s, buf);
	gc_push(vm->runningWord);
	status = ficlVmExecuteString(vm, s);
	gc_pop();
	FTH_FREE(buf);
	vm->sourceId = id;
	switch (status) {
	case FICL_VM_STATUS_INNER_EXIT:
	case FICL_VM_STATUS_OUT_OF_TEXT:
	case FICL_VM_STATUS_RESTART:
	case FICL_VM_STATUS_BREAK:
	case FICL_VM_STATUS_ABORT:
	case FICL_VM_STATUS_ABORTQ:
	case FICL_VM_STATUS_QUIT:
		return (FTH_OKAY);
		break;
	case FICL_VM_STATUS_USER_EXIT:
		return (FTH_BYE);
		break;
	case FICL_VM_STATUS_ERROR_EXIT:
	default:
		return (FTH_ERROR);
		break;
	}
}

/*
 * Evaluate C string BUFFER.
 * If BUFFER is NULL,
 *      return #<undef>,
 * if BUFFER evaluates to FTH_BYE,
 *	exit program,
 * if BUFFER evaluates to no value,
 *	return #<undef>,
 * if BUFFER evaluates to a single value,
 *	remove it from stack and return it,
 * if BUFFER evaluates to multiple values,
 *	remove them from stack and return them all in an array.
 */
FTH
fth_eval(const char *buffer)
{
	static ficlInteger lineno = 0;
	ficlInteger old_line, new_depth, i;
	ficlVm *vm;
	FTH val, old_file;
	int depth;

	if (buffer == NULL)
		return (FTH_UNDEF);
	old_line = fth_current_line;
	old_file = fth_current_file;
	vm = FTH_FICL_VM();
	depth = FTH_STACK_DEPTH(vm);
	fth_eval_p = true;
	fth_current_file = eval_string;
	fth_current_line = ++lineno;
	if (fth_catch_eval(buffer) == FTH_BYE)
		fth_exit(EXIT_SUCCESS);
	vm = FTH_FICL_VM();
	new_depth = FTH_STACK_DEPTH(vm) - depth;
	switch(new_depth) {
	case 0:
		val = FTH_UNDEF;
		break;
	case 1:
		val = fth_pop_ficl_cell(vm);
		break;
	default:
		val = fth_make_array_len(new_depth);
		for (i = 0; i < new_depth; i++)
			fth_array_set(val, i, fth_pop_ficl_cell(vm));
		break;
	}
	fth_current_file = old_file;
	fth_current_line = old_line;
	fth_eval_p = false;
	return (val);
}

/*
 * Push C string NAME to environment word list for later review with
 * provided? and *features*.
 */
void
fth_add_feature(const char *name)
{
	if (fth_strlen(name) > 0)
		ficlDictionaryAppendConstant(FTH_FICL_ENV(),
		    (char *)name,
		    (ficlInteger)fth_symbol(name));
}

static void
ficl_add_feature(ficlVm *vm)
{
#define h_add_feature "( obj -- )  add feature\n\
'snd add-feature\n\
Add OBJ, a string or symbol, to *features* list.\n\
See also provided? and *features*."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 0);
	obj = fth_pop_ficl_cell(vm);
	FTH_ASSERT_ARGS(fth_string_or_symbol_p(obj), obj, FTH_ARG1,
	    "a string or a symbol");
	fth_add_feature(fth_string_or_symbol_ref(obj));
}

/*
 * Test if feature NAME exists in environment word list.
 */
bool
fth_provided_p(const char *name)
{
	if (fth_strlen(name) > 0) {
		ficlString s;

		FICL_STRING_SET_FROM_CSTRING(s, name);
		return (ficlDictionaryLookup(FTH_FICL_ENV(), s) != NULL);
	}
	return (false);
}

static void
ficl_provided_p(ficlVm *vm)
{
#define h_provided_p "( obj -- f )  test if OBJ already exist\n\
'fth provided? => #t\n\
'foo provided? => #f\n\
Return #t if OBJ, a string or symbol, exists in *features* list, \
otherwise #f.\n\
See also add-feature and *features*."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	FTH_ASSERT_ARGS(fth_string_or_symbol_p(obj), obj, FTH_ARG1,
	    "a string or a symbol");
	ficlStackPushBoolean(vm->dataStack,
	    fth_provided_p(fth_string_or_symbol_ref(obj)));
}

static void
ficl_features(ficlVm *vm)
{
#define h_features "( -- ary )  return feature array\n\
*features* => #( \"timer\" \"exception\" ...)\n\
Return array of all features.\n\
See also add-feature and provided?."
	ficlHash *hash;
	ficlWord *word;
	unsigned int i;
	FTH features;

	FTH_STACK_CHECK(vm, 0, 1);
	hash = ficlSystemGetEnvironment(vm->callback.system)->forthWordlist;
	features = fth_make_empty_array();

	for (i = 0; i < hash->size; i++)
		for (word = hash->table[i]; word != NULL; word = word->link)
			fth_array_push(features,
			    fth_make_string(FICL_WORD_NAME(word)));
	ficlStackPushFTH(vm->dataStack, features);
}

/* === Load Files === */

char *
fth_basename(const char *path)
{
	char *base;

	if (path == NULL)
		return ("");
	return ((base = strrchr(path, '/')) ? ++base : (char *)path);
}

#define ADD_TO_LOAD_PATH(Lp, Kind, Path)				\
	if (fth_strlen(Path) > 0) {					\
		FTH fs;							\
									\
		fs = fth_make_string(Path);				\
		if (!fth_array_member_p(Lp, fs))			\
			fth_array_ ## Kind(Lp, fs);			\
	}								\

/*
 * Push PATH at the end of global array variable *load-path* if not
 * already there.
 */
void
fth_add_load_path(const char *path)
{
	ADD_TO_LOAD_PATH(load_path, push, path);
}

/*
 * Prepend PATH to the beginning of global array variable *load-path*
 * if not already there.
 */
void
fth_unshift_load_path(const char *path)
{
	ADD_TO_LOAD_PATH(load_path, unshift, path);
}

/*
 * Push PATH at the end of global array variable *load-lib-path* if
 * not already there.
 */
void
fth_add_load_lib_path(const char *path)
{
	ADD_TO_LOAD_PATH(load_lib_path, push, path);
}

/*
 * Prepend PATH to the beginning of global array variable *load-lib-path*
 * if not already there.
 */
void
fth_unshift_load_lib_path(const char *path)
{
	ADD_TO_LOAD_PATH(load_lib_path, unshift, path);
}

/*
 * Push FILE at the end of global array variable *loaded-files* if not
 * already there.
 */
void
fth_add_loaded_files(const char *file)
{
	ADD_TO_LOAD_PATH(loaded_files, push, file);
}

static void
ficl_add_load_path(ficlVm *vm)
{
#define h_add_load_path "( path -- )  add PATH to *load-path*\n\
\"/home/mike/share/fth\" add-load-path\n\
Add string PATH to *load-path* array if not already there."
	FTH_STACK_CHECK(vm, 1, 0);
	fth_add_load_path(pop_cstring(vm));
}

static void
ficl_unshift_load_path(ficlVm *vm)
{
#define h_uns_load_path "( path -- )  prepend PATH to *load-path*\n\
\"/home/mike/share/fth\" unshift-load-path\n\
Prepend string PATH to *load-path* array if not already there."
	FTH_STACK_CHECK(vm, 1, 0);
	fth_unshift_load_path(pop_cstring(vm));
}

static void
ficl_add_load_lib_path(ficlVm *vm)
{
#define h_add_load_lib_path "( path -- )  add PATH to *load-lib-path*\n\
\"/home/mike/lib/fth\" add-load-lib-path\n\
Add string PATH to *load-lib-path* array if not already there."
	FTH_STACK_CHECK(vm, 1, 0);
	fth_add_load_lib_path(pop_cstring(vm));
}

static void
ficl_unshift_load_lib_path(ficlVm *vm)
{
#define h_uns_load_lib_path "( path -- )  prepend PATH to *load-lib-path*\n\
\"/home/mike/lib/fth\" unshift-load-lib-path\n\
Prepend string PATH to *load-lib-path* array if not already there."
	FTH_STACK_CHECK(vm, 1, 0);
	fth_unshift_load_lib_path(pop_cstring(vm));
}

#define FINISH_LOAD() do {						\
	vm->sourceId = old_source_id;					\
	fth_current_file = old_file;					\
	fth_current_line = old_line;					\
} while (0)

/*
 * load_file(name, caller)
 *
 * Two load hooks:
 *
 *	before_load_hook ( fname -- val )
 *	after_load_hook  ( fname -- val )
 *
 * Return values:
 *
 * 	#f if before_load_hook returned #f
 * 	#t if load was successful
 * 	throw exception if something went wrong
 */
static FTH
load_file(const char *name, const char *caller)
{
	ficlVm *vm;
	ficlInteger old_line, len, i, status;
	FTH fname, old_file, content, ret, fs;
	ficlCell old_source_id;
	ficlString s;

	if (name == NULL)
		return (FTH_FALSE);
	fname = fth_make_string(name);
	if (!fth_hook_empty_p(before_load_hook)) {
		ret = fth_run_hook_bool(before_load_hook, 1, fname);
		if (FTH_FALSE_P(ret))
			return (FTH_FALSE);
	}
	old_line = fth_current_line;
	old_file = fth_current_file;
	content = fth_readlines(name);
	len = fth_array_length(content);
	vm = FTH_FICL_VM();
	old_source_id = vm->sourceId;
	fth_current_file = fname;
	CELL_VOIDP_SET(&vm->sourceId, name);
	fth_add_loaded_files(name);
	for (i = 0; i < len; i++) {
		fth_current_line = i + 1;
		FICL_STRING_SET_FROM_CSTRING(s,
		    fth_string_ref(fth_array_fast_ref(content, i)));
		status = ficlVmExecuteString(vm, s);
		switch (status) {
		case FICL_VM_STATUS_INNER_EXIT:
		case FICL_VM_STATUS_OUT_OF_TEXT:
		case FICL_VM_STATUS_RESTART:
		case FICL_VM_STATUS_BREAK:
		case FICL_VM_STATUS_QUIT:
			continue;
			break;
		case FICL_VM_STATUS_SKIP_FILE:
			FINISH_LOAD();
			return (FTH_TRUE);
			break;
		case FICL_VM_STATUS_USER_EXIT:
			FINISH_LOAD();
			fth_exit(EXIT_SUCCESS);
			break;
		default:
			fs = fth_make_string_format("%S at line %ld",
			    fth_current_file, fth_current_line);
			FINISH_LOAD();
			fth_throw(ficl_ans_real_exc((int)status),
			    "%s: can't load file %S", caller, fs);
			/* NOTREACHED */
			return (FTH_FALSE);
			break;
		}
	}
	CELL_INT_SET(&vm->sourceId, -1);
	FICL_STRING_SET_FROM_CSTRING(s, "");
	ficlVmExecuteString(vm, s);
	if (!fth_hook_empty_p(after_load_hook))
		fth_run_hook(after_load_hook, 1, fname);
	FINISH_LOAD();
	return (FTH_TRUE);
}

/*
 * fth_load_file(name)
 *
 * Two load hooks:
 *
 *	before_load_hook ( fname -- val )
 *	after_load_hook  ( fname -- val )
 *
 * Return values:
 *
 * 	#f if before_load_hook returned #f
 * 	#t if load was successful
 * 	throw exception if something went wrong
 *
 * Load C string NAME as Fth source file and add NAME to global array
 * *loaded-files*.  Before loading source file run hook before-load-hook
 * if not empty.  If this hook returns #f, return immediately without
 * loading the source file and return FTH_FALSE.  If loading finishes
 * successfully, return FTH_TRUE, otherwise return error string.  After
 * loading source file run hook after-load-hook if not empty.  If NAME
 * has no file extension, FTH_FILE_EXTENSION ('.fs') will be added.
 * If NAME or NAME plus FTH_FILE_EXTENSION doesn't exist, try all path
 * names from *load-path* with NAME.
 */
FTH
fth_load_file(const char *name)
{
	char *func, *path, *tname, *fname;
	ficlInteger i, alen, slen;
	size_t size;
	FTH fs;

	if (name == NULL)
		return (FTH_TRUE);
	func = RUNNING_WORD();
	if (fth_file_exists_p(name))
		return (load_file(name, func));
	/*
	 * If first char is a dot ('.') or slash ('/') or NAME contains a dot
	 * ('name.fs'), the name is complete, otherwise add file extension
	 * ".fs".
	 */
	tname = misc_scratch;
	size = sizeof(misc_scratch);
	fth_strcpy(tname, size, name);
	if (*name != '.' && *name != '/' && !strchr(name, '.'))
		fth_strcat(tname, size, "." FTH_FILE_EXTENSION);
	if (fth_file_exists_p(tname))
		return (load_file(tname, func));
	/*
	 * If not found, try every path from load_path with NAME and probably
	 * added file extension to find source file in Fth's environment.
	 */
	alen = fth_array_length(load_path); 
	fname = misc_scratch_02;
	fth_strcpy(fname, size, tname);
	for (i = 0; i < alen; i++) {
		fs = fth_array_fast_ref(load_path, i);
		slen = fth_string_length(fs);
		if (slen <= 0)
			continue;
		path = fth_string_ref(fs);
		fth_strcpy(tname, size, path);
		if (path[slen - 1] != '/')
			fth_strcat(tname, size, "/");
		fth_strcat(tname, size, fname);
		if (fth_file_exists_p(tname))
			return (load_file(tname, func));
	}
	fth_throw(FTH_NO_SUCH_FILE, "%s: \"%s\" not found", func, name);
	/* NOTREACHED */
	return (FTH_TRUE);
}

static void
ficl_include_file(ficlVm *vm)
{
#define h_include_file "( \"name\" -- )  load filename (parse word)\n\
include hello\n\
Load Forth source file NAME and add NAME to *loaded-files* \
if it wasn't already there.  \
If file extension wasn't specified, use \".fs\".  \
If NAME doesn't exist, try each entry of *load-path* with NAME.  \
With INCLUDE one can load a file more than once.\n\
Before loading NAME, run hook before-load-hook ( fname -- f ).  \
After loading NAME, run hook after-load-hook ( fname -- ).  \
Raise NO-SUCH-FILE exception if file doesn't exist.  \
Raise LOAD-ERROR if an error occured during load.\n\
See also require, before-load-hook and after-load-hook."
	ficlVmGetWordToPad(vm);
	fth_load_file(vm->pad);
}

/*
 * fth_require_file(name)
 *
 * Two load hooks:
 *
 *	before_load_hook ( fname -- val )
 *	after_load_hook  ( fname -- val )
 *
 * Return values:
 *
 * 	#f if before_load_hook returned #f
 * 	#t if load was successful
 * 	throw exception if something went wrong
 *
 * Load C string NAME as Fth source file if not already loaded and add
 * NAME to global array *loaded-files*.  Before loading source file
 * run hook before-load-hook if not empty.  If this hook returns #f,
 * return immediately without loading the source file and return
 * FTH_FALSE.  If loading finishes successfully, return FTH_TRUE,
 * otherwise return error string.  After loading source file run hook
 * after-load-hook if not empty.  If NAME has no file extension,
 * FTH_FILE_EXTENSION ('.fs') will be added.  If NAME or NAME plus
 * FTH_FILE_EXTENSION doesn't exist, try all path names from *load-path*
 * with NAME.
 */
FTH
fth_require_file(const char *name)
{
	FTH fs;

	if (name == NULL)
		return (FTH_TRUE);
	fs = fth_make_string(name);
	if (fth_array_member_p(loaded_files, fs))
		return (FTH_TRUE);
	/*
	 * If first char is a dot ('.') or slash ('/') or 'name' contains a
	 * dot, the name is complete, otherwise, add file extension '.fs'.
	 */
	if (*name != '.' && *name != '/' && !strchr(name, '.'))
		fth_string_scat(fs, "." FTH_FILE_EXTENSION);
	if (fth_array_member_p(loaded_files, fs))
		return (FTH_TRUE);
	if (FTH_STRING_P(fth_find_file(fs)))
		return (FTH_TRUE);
	return (fth_load_file(name));
}

static void
ficl_require_file(ficlVm *vm)
{
#define h_require_file "( \"name\" -- )  load filename (parse word)\n\
require hello\n\
If Forth source file NAME doesn't exist in the array *loaded-files*, \
load it and add NAME to *loaded-files*.  \
If file extension wasn't specified, use \".fs\".  \
If NAME doesn't exist, try each entry of *load-path* with NAME.  \
With REQUIRE one can load a file only one time.\n\
Before loading NAME, run hook before-load-hook ( fname -- f ).  \
After loading NAME, run hook after-load-hook ( fname -- ).  \
Raise NO-SUCH-FILE exception if file doesn't exist.  \
Raise LOAD-ERROR if an error occured during load.\n\
See also include, before-load-hook and after-load-hook."
	ficlVmGetWordToPad(vm);
	fth_require_file(vm->pad);
}

/*
 * fth_load_init_file(init_file)
 *
 * if init file ~/.fthrc or $FTH_INIT_FILE exists:
 *
 *   before_load_hook ( fname -- val )
 *   after_load_hook  ( fname -- val )
 *
 * #f if before_load_hook returned false
 * #t if loaded successful
 * error string if something went wrong
 *
 * #t if init file doesn't exist
 *
 * Load C string INIT_FILE as Forth source file if it exists, otherwise
 * do nothing.  If INIT_FILE is NULL, try to load ${FTH_INIT_FILE}.
 * If ${FTH_INIT_FILE} is not set, try to load ${HOME}/.fthrc instead.
 * Run before-load-hook and after-load-hook if not empty.
 */
FTH
fth_load_init_file(const char *init_file)
{
	/* If no filename was given ... */
	if (init_file == NULL) {
		/* ... and no environment variable was set ... */
		init_file = fth_getenv(FTH_ENV_INIT_FILE, NULL);
		if (init_file == NULL) {
			char *home, *file;
			size_t size;

			/* ... take ${HOME}/.fthrc */
			home = fth_getenv("HOME", "/tmp");
			file = misc_scratch;
			size = sizeof(misc_scratch);
			fth_strcpy(file, size, home);
			fth_strcat(file, size, "/" FTH_INIT_FILE);
			init_file = file;
		}
	}
	if (fth_file_exists_p(init_file))
		return (load_file(init_file, RUNNING_WORD()));
	/* If no file exists, do nothing and pretend all is okay. */
	return (FTH_TRUE);
}

/*
 * fth_load_global_init_file()
 *
 * if init file ${prefix}/etc/fth.conf exists:
 *
 *   before_load_hook ( fname -- val )
 *   after_load_hook  ( fname -- val )
 *
 * #f if before_load_hook returned false
 * #t if loaded successful
 * error string if something went wrong
 *
 * #t if init file doesn't exist
 *
 * Load FTH_GLOBAL_INIT_FILE (${prefix}/etc/fthrc) as Forth source
 * file if it exists, otherwise do nothing.  Run before-load-hook and
 * after-load-hook if not empty.
 */
FTH
fth_load_global_init_file(void)
{
	if (fth_file_exists_p(FTH_GLOBAL_INIT_FILE))
		return (load_file(FTH_GLOBAL_INIT_FILE, RUNNING_WORD()));
	return (FTH_TRUE);
}

static void
ficl_load_init_file(ficlVm *vm)
{
#define h_load_init_file "( file -- )  load init file\n\
\".my-fth-init\" load-init-file\n\
If Forth source FILE exists in current or $HOME dir, load it, \
otherwise do nothing.\n\
See also include and require."
	char *str, *file, *home;
	size_t size;
	FTH fs;

	FTH_STACK_CHECK(vm, 1, 0);
	fs = fth_pop_ficl_cell(vm);
	FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
	str = fth_string_ref(fs);
	if (str == NULL)
		return;
	file = str;
	if (fth_file_exists_p(file)) {
		load_file(file, RUNNING_WORD_VM(vm));
		return;
	}
	file = misc_scratch;
	size = sizeof(misc_scratch);
	fth_strcpy(file, size, "./");
	fth_strcat(file, size, str);
	if (fth_file_exists_p(file)) {
		load_file(file, RUNNING_WORD_VM(vm));
		return;
	}
	home = fth_getenv("HOME", "/tmp");
	if (home == NULL)
		return;
	fth_strcpy(file, size, home);
	fth_strcat(file, size, "/");
	fth_strcat(file, size, str);
	if (fth_file_exists_p(file))
		load_file(file, RUNNING_WORD_VM(vm));
}

#if defined(HAVE_DLOPEN)
/*
 * load_lib(name, func, caller)
 *
 *   before_load_hook ( fname -- val )
 *   after_load_hook  ( fname -- val )
 *
 * #f if before_load_hook returned false
 * #t if loaded successful
 * *so-file-error* exception if something went wrong
 *
 */
static FTH
load_lib(const char *name, const char *func, const char *caller)
{
	FTH fname, old_file;
	void *handle;
	void (*init_fnc) (void);
	ficlVm *vm;
	ficlInteger old_line;
	ficlCell old_source_id;

	handle = (void *)dlopen(name, RTLD_LAZY | RTLD_GLOBAL);
	if (handle == NULL) {
		fth_throw(FTH_SO_FILE_ERROR, "%s: %s", caller, dlerror());
		/* NOTREACHED */
		return (FTH_FALSE);
	}
	fname = fth_make_string(name);
	if (!fth_hook_empty_p(before_load_hook) &&
	    FTH_FALSE_P(fth_run_hook_bool(before_load_hook, 1, fname))) {
		dlclose(handle);
		return (FTH_FALSE);
	}
	init_fnc = (void (*)(void))dlsym(handle, func);
	if (init_fnc == NULL) {
		dlclose(handle);
		fth_throw(FTH_SO_FILE_ERROR, "%s: %s", caller, dlerror());
		/* NOTREACHED */
		return (FTH_FALSE);
	}
	old_file = fth_current_file;
	old_line = fth_current_line;
	vm = FTH_FICL_VM();
	old_source_id = vm->sourceId;
	CELL_VOIDP_SET(&vm->sourceId, name);
	fth_current_file = fname;
	fth_current_line = 0;
	fth_add_loaded_files(name);
	/* Calling Init_dbm() etc. */
	(*init_fnc)();
	if (!fth_hook_empty_p(after_load_hook))
		fth_run_hook(after_load_hook, 1, fname);
	FINISH_LOAD();
	return (FTH_TRUE);
}

/*
 * fth_dl_load(name, func)
 *
 *   before_load_hook ( fname -- val )
 *   after_load_hook  ( fname -- val )
 *
 * #f if before_load_hook returned false
 * #t if loaded successful or file already loaded
 * *so-file-error* exception if something went wrong
 *
 * Load C string NAME as dynamic library if not already loaded and add
 * NAME to global array *loaded-files*.  C string FUNC will be called
 * after load was successful.  Before loading the dynamic library run
 * hook before-load-hook if not empty.  If this hook returns #f, return
 * immediately without loading the library and return FTH_FALSE.  If
 * loading finishes successfully or library was already loaded, return
 * FTH_TRUE, otherwise raise SO_FILE_ERROR exception.  After loading
 * the dynamic library run hook after-load-hook if not empty.  If NAME
 * has no file extension, '.so' will be added.  If NAME or NAME plus
 * '.so' doesn't exist, try all path names from *load-lib-path* with
 * NAME.
 */
FTH
fth_dl_load(const char *name, const char *func)
{
	char *caller, *path, *tname, *fname;
	ficlInteger i, alen, slen;
	size_t size;
	FTH fs;

	tname = misc_scratch;
	fname = misc_scratch_02;
	size = sizeof(misc_scratch);
	caller = RUNNING_WORD();
	fth_strcpy(tname, size, name);
	if (strstr(name, ".so") == NULL)
		fth_strcat(tname, size, ".so");
	if (fth_array_member_p(loaded_files, fth_make_string(tname)))
		return (FTH_TRUE);
	if (fth_file_exists_p(tname))
		return (load_lib(tname, func, caller));
	alen = fth_array_length(load_lib_path); 
	for (i = 0; i < alen; i++) {
		fs = fth_array_fast_ref(load_lib_path, i);
		slen = fth_string_length(fs);
		if (slen <= 0)
			continue;
		path = fth_string_ref(fs);
		fth_strcpy(fname, size, path);
		if (path[slen - 1] != '/')
			fth_strcat(fname, size, "/");
		fth_strcat(fname, size, tname);
		if (fth_array_member_p(loaded_files, fth_make_string(fname)))
			return (FTH_TRUE);
		if (fth_file_exists_p(fname))
			return (load_lib(fname, func, caller));
	}
	fth_throw(FTH_NO_SUCH_FILE, "%s: \"%s\" not found", caller, name);
	/* NOTREACHED */
	return (FTH_TRUE);
}

#else	/* !HAVE_DLOPEN */

/* ARGSUSED */
FTH
fth_dl_load(const char *name, const char *func)
{
	(void)name;
	(void)func;
	FTH_NOT_IMPLEMENTED_ERROR(dlopen);
	return (FTH_FALSE);
}
#endif	/* HAVE_DLOPEN */

static void
ficl_dl_load(ficlVm *vm)
{
#define h_dl_load "( \"lib\" \"func\" -- )  load dynamic lib (parse word)\n\
dl-load dbm Init_dbm\n\
Load dynamic library LIB and call its function FUNC.\n\
See also include and require."
	char *lib, *fnc;
	ficlString s;
	size_t size;

 	lib = misc_scratch_03;
	fnc = misc_scratch_04;
	size = sizeof(misc_scratch_03);
	s = ficlVmGetWord0(vm);
	fth_strncpy(lib, size, s.text, s.length);
	s = ficlVmGetWord0(vm);
	fth_strncpy(fnc, size, s.text, s.length);
#if defined(FTH_STATIC)
	return;
#endif
	if (!fth_provided_p(fth_basename(lib)))
		fth_dl_load(lib, fnc);
}

#if !defined(_WIN32)
void
fth_install_file(FTH fname)
{
#define h_install_file "( file -- )  install library\n\
\"snd-test.fs\" install-file\n\
\"sndlib.so\" install-file\n\
Install FILE in first writeable path \
found in *load-path* (*.fs[m]) or *load-lib-path* (*.so).  \
A warning is given if no writable path was found.\n\
See also install and file-install."
	char *lname, *pname, *tname;
	FTH path_array, fs;
	mode_t mode;
	size_t size;
	ficlInteger i, alen, slen;

	FTH_ASSERT_ARGS(FTH_STRING_P(fname), fname, FTH_ARG1, "a string");
	lname = fth_string_ref(fname);
	slen = fth_string_length(fname);
	if (lname == NULL)
		return;
	if (!fth_file_exists_p(lname)) {
		fth_warning("%s: file \"%s\" does not exist, nothing done",
		    RUNNING_WORD(), lname);
		return;
	}
	if (strncmp(lname + slen - 3, ".fs", 3L) == 0 ||
	    strncmp(lname + slen - 4, ".fsm", 4L) == 0) {
		/* forth-source.fs */
		/* mode = 0644 */
		mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
		path_array = load_path;
	} else if (strncmp(lname + slen - 3, ".so", 3L) == 0) {
		/* c-library.so */
		/* mode = 0755 */
		mode = S_IXUSR | S_IRUSR | S_IWUSR | S_IXGRP | S_IRGRP |
		    S_IXOTH | S_IROTH;
		path_array = load_lib_path;
	} else {
		fth_warning("%s: \"%s\" not a library, nothing done",
		    RUNNING_WORD(), lname);
		return;
	}
	alen = fth_array_length(path_array); 
	tname = misc_scratch;
	size = sizeof(misc_scratch);
	for (i = 0; i < alen; i++) {
		fs = fth_array_fast_ref(path_array, i);
		slen = fth_string_length(fs);
		if (slen <= 0)
			continue;
		pname = fth_string_ref(fs);
		if (*pname == '.' || !fth_file_writable_p(pname))
			continue;
		fth_strcpy(tname, size, pname);
		if (pname[slen - 1] != '/')
			fth_strcat(tname, size, "/");
		fth_strcat(tname, size, fth_basename(lname));
		if (fth_file_install(lname, tname, mode))
			if (FTH_TO_BOOL(fth_variable_ref("*fth-verbose*")))
				fth_printf("\\ %s --> %04o %s\n",
				    lname, mode, tname);
		return;
	}
	fth_warning("%s: no path found for \"%s\", nothing done",
	    RUNNING_WORD(), lname);
}

void
fth_install(void)
{
#define h_install "( \"file\" -- )  install library (parse word)\n\
install snd-test.fs\n\
install sndlib.so\n\
fth -ve 'install sndlib.so' -e ''\n\
Install FILE in first writeable path found \
in *load-path* (*.fs[m]) or *load-lib-path* (*.so).\n\
In the last example the trailing \"-e ''\" is necessary \
because the last occurrence of \"-e pattern\" will be compiled.  \
INSTALL is a parse word and won't work in compile state.\n\
See also install-file and file-install."
	ficlVmGetWordToPad(FTH_FICL_VM());
	fth_install_file(fth_make_string(FTH_FICL_VM()->pad));
}

#endif				/* !_WIN32 */

/*
 * Return full path of NAME or #f.
 */
FTH
fth_find_file(FTH name)
{
	ficlInteger i, alen;
	FTH fp, fn, fs;

	FTH_ASSERT_ARGS(FTH_STRING_P(name), name, FTH_ARG1, "a string");
	alen = fth_array_length(load_path); 
	for (i = 0; i < alen; i++) {
		fp = fth_array_fast_ref(load_path, i);
		fn = fth_make_string_format("%S/%S", fp, name);
		fs = fth_array_find(loaded_files, fn);
		if (FTH_STRING_P(fs))
			return (fs);
	}
	return (FTH_FALSE);
}

/*
 * Find NAME in Forth dictionary and return ficlWord.
 */
FTH
fth_word_ref(const char *name)
{
	return ((FTH)FICL_WORD_NAME_REF(name));
}

/*
 * Used by snd/snd-help.c and repl/repl.c.
 */
char *
fth_parse_word(void)
{
	ficlVm *vm;
	ficlWord *w;

	vm = FTH_FICL_VM();
	ficlVmGetWordToPad(vm);
	w = FICL_WORD_NAME_REF(vm->pad);
	if (w != NULL)
		return (w->name);
	if (fth_strlen(vm->pad) > 0)
		return (vm->pad);
	return (NULL);
}

FTH
fth_wordlist_each(
    bool (*func) (ficlWord *word, FTH data),
    FTH data)
{
	FTH ret;
	ficlDictionary *dict;
	ficlHash *hash;
	ficlWord *word;
	int i, j;

	dict = FTH_FICL_SYSTEM()->dictionary;
	ret = fth_make_empty_array();

	for (i = (int)dict->wordlistCount - 1; i >= 0; i--)
		for (hash = dict->wordlists[i];
		    hash != NULL;
		    hash = hash->link)
			for (j = (int)hash->size - 1; j >= 0; j--)
				for (word = hash->table[j];
				    word != NULL;
				    word = word->link)
					if ((*func) (word, data))
						fth_array_push(ret,
						    FTH_WORD_NAME(word));
	return (fth_array_uniq(ret));
}

static bool
find_in_wordlist(ficlWord *word, FTH data)
{
	char *text = (char *)data;

	return (word->length > 0 &&
	    ficlStrincmp(word->name, text, fth_strlen(text)) == 0);
}

FTH
fth_find_in_wordlist(const char *text)
{
	if (text != NULL && *text != '\0')
		return (fth_wordlist_each(find_in_wordlist, (FTH)text));
	return (fth_make_empty_array());
}

static bool
apropos(ficlWord *word, FTH data)
{
	if (word->length > 0)
		return (fth_regexp_match(data, FTH_WORD_NAME(word)) >= 0);
	return (false);
}

FTH
fth_apropos(FTH regexp)
{
#define h_apropos "( obj -- ary )  search in wordlist\n\
/do/ apropos => #( \"doLocal\" ... )\n\
Return array of dictionary entries matching regexpression or string OBJ."
	return (fth_wordlist_each(apropos, regexp));
}

/* === MISCELLANEOUS === */

/* partly stolen from ficl/primitive.c, ficlPrimitiveCatch() */
static void
ficl_catch(ficlVm *vm)
{
#define h_fth_catch "( ?? proc-or-xt|#f exc|#t arg -- ?? #f|exc|arg )  catch\n\
3 :initial-element 0.3 make-array value ary\n\
ary .array => #( 0.3 0.3 0.3 )\n\
ary 2 <'> array-ref  #t  nil  fth-catch => 0.3 #f\n\
ary 4 <'> array-ref  'out-of-range  #t  fth-catch => #( 0.3 0.3 0.3 ) 4 #t\n\
: ary-handler { retval -- val }\n\
	\"from handler: %S\\n\" #( retval ) fth-print\n\
	#t ( return value )\n\
;\n\
ary 4 0.4 <'> array-set!  'out-of-range  <'> ary-handler  fth-catch\n\
  => from handler: \\\n\
    #( 'out-of-range \"array-set! (ary_set) arg 2: 4 is out of range\" )\n\
  0.4 #t\n\
ary 2 0.4 <'> array-set! 'out-of-range <'> ary-handler fth-catch => #f\n\
Run PROC-OR-XT in save environment.  \
If PROC-OR-XT fails with an exception, \
data and return stack environments are recovered \
to the state before execution.\n\
PROC-OR-XT:\n\
If PROC-OR-XT is #f, FTH-CATCH finishs immediately and returns #f.\n\
EXC:\n\
The second argument EXC may be a symbol, an exception, or #t.  \
If it's a symbol or an exception, this will be catched, \
if it's #t, all exceptions will be catched.\n\
ARG:\n\
The third argument ARG may be NIL or a return value.  \
If ARG is NIL, the catched exception or #f will be returned, \
if ARG is any other than NIL, ARG will be returned instead \
of the catched exception; \
if ARG is a proc or xt, this will be executed instead of simply returned.  \
The stack effect must be ( retval -- val ).\n\
See also fth-throw and fth-raise."
	FTH prc, obj;
	volatile FTH arg;
	jmp_buf jmp_env;
	ficlWord *volatile word;
	volatile ficlVm vm_copy;
	volatile ficlStack data_stack_copy;
	volatile ficlStack return_stack_copy;
	volatile FTH exc, rval, result;

	FTH_STACK_CHECK(vm, 3, 1);
	arg = fth_pop_ficl_cell(vm);
	obj = fth_pop_ficl_cell(vm);
	prc = fth_pop_ficl_cell(vm);
	if (FTH_FALSE_P(prc)) {
		ficlStackPushBoolean(vm->dataStack, false);
		return;
	}
	FTH_ASSERT_ARGS(FICL_WORD_P(prc), prc, FTH_ARG1, "a proc or an xt");
	exc = FTH_TRUE_P(obj) ? obj : fth_symbol_or_exception_ref(obj);
	FTH_ASSERT_ARGS(FTH_NOT_FALSE_P(exc), obj, FTH_ARG2,
	    "a symbol, an exception, or #t");
	word = FICL_WORD_REF(prc);
	memcpy((void *)&vm_copy, vm, sizeof(ficlVm));
	memcpy((void *)&data_stack_copy, vm->dataStack, sizeof(ficlStack));
	memcpy((void *)&return_stack_copy, vm->returnStack, sizeof(ficlStack));
	vm->exceptionHandler = &jmp_env;
	vm->fth_catch_p = true;
	result = FTH_FALSE;
	switch (setjmp(jmp_env)) {
	case 0:
		ficlVmPushIP(vm, &(vm->callback.system->exitInnerWord));
		ficlVmInnerLoop(vm, word);
		ficlVmInnerLoop(vm, 0);
		break;
	case FICL_VM_STATUS_INNER_EXIT:	/* okay => return #f */
		ficlVmPopIP(vm);
		vm->exceptionHandler = vm_copy.exceptionHandler;
		vm->fth_catch_p = false;
		result = FTH_FALSE;
		break;
	default:		/* exception */
		memcpy(vm, (void *)&vm_copy, sizeof(ficlVm));
		memcpy(vm->dataStack, (void *)&data_stack_copy,
		    sizeof(ficlStack));
		memcpy(vm->returnStack, (void *)&return_stack_copy,
		    sizeof(ficlStack));
		vm->exceptionHandler = vm_copy.exceptionHandler;
		vm->fth_catch_p = false;
		if (FTH_TRUE_P(exc)) {
			FTH ex;

			ex = fth_variable_ref("*last-exception*");
			rval = FTH_LIST_2(ex,
			    fth_exception_last_message_ref(ex));
		} else
			rval = FTH_LIST_2(exc,
			    fth_exception_last_message_ref(exc));
		if (FTH_TRUE_P(exc) ||
		    fth_exception_equal_p(fth_car(rval), exc)) {
			if (FICL_WORD_P(arg)) {
				FTH a;

				a = proc_from_proc_or_xt(arg, 1, 0, false);
				if (FTH_PROC_P(a))
					result = fth_proc_call(a,
					    RUNNING_WORD_VM(vm), 1, rval);
				else
					result = rval;
			} else
				result = FTH_NIL_P(arg) ? rval : arg;
		} else {
			if (fth_object_length(rval) > 1)
				fth_throw_error(fth_car(rval), fth_cdr(rval));
			else
				fth_throw_error(fth_car(rval), rval);
		}
		break;
	}
	fth_push_ficl_cell(vm, result);
}

static void
ficl_throw(ficlVm *vm)
{
#define h_fth_throw "( exc args -- )  throw exception\n\
\\ any object\n\
'bad-arity proc fth-throw\n\
  => #<bad-arity in test-proc>\n\
\\ nil or #()\n\
'bad-arity nil fth-throw\n\
  => #<bad-arity: proc has bad arity>\n\
\\ #( string )\n\
'bad-arity #( \"test-proc\" ) fth-throw\n\
  => #<bad-arity in test-proc>\n\
\\ #( fmt arg1 arg2 arg3 )\n\
'bad-arity #( \"%s: %s args required, got %s\"\n\
              proc\n\
              2\n\
              3 ) fth-throw\n\
  => #<bad-arity in test-proc: 2 args required, got 3>\n\
Throw exception EXC with text built from ARGS.  \
If ARGS is not an array, ARGS's string representation is used.  \
If ARGS is NIL or an empty array, a default string is used.  \
If ARGS is an array with one element, this string is used.  \
If ARGS is an array and its first element is a format string with N \
%s-format signs, ARGS should have N more elements with corresponding values.\n\
See also fth-raise and fth-catch."
	FTH obj, args;

	FTH_STACK_CHECK(vm, 2, 0);
	args = fth_pop_ficl_cell(vm);
	obj = fth_pop_ficl_cell(vm);
	/* Set to calling word. */
	vm->runningWord = vm->runningWord->current_word;
	fth_throw_error(obj, args);
}

static void
ficl_raise(ficlVm *vm)
{
#define h_fth_raise "( exc fmt args -- )  raise exception\n\
'bad-arity \"%s: %s args require, got %s\" #( proc 3 2 ) fth-raise\n\
  => #<bad-arity in test-proc: 3 args required, got 2>\n\
#f #f #f fth-raise => reraise last exception\n\
Raise exception EXC with text built from FMT and ARGS.  \
If FMT is a format string with N %s-format signs, \
ARGS should have N elements with corresponding values.  \
If EXC is #f, reraise last exception.\n\
See also fth-throw and fth-catch."
	FTH exc, fmt, args;

	FTH_STACK_CHECK(vm, 3, 0);
	args = fth_pop_ficl_cell(vm);
	fmt = fth_pop_ficl_cell(vm);
	exc = fth_pop_ficl_cell(vm);
	if (FTH_NOT_FALSE_P(exc)) {
		/* Set to calling word. */
		vm->runningWord = vm->runningWord->current_word;
		fth_throw(exc, "%S", fth_string_format(fmt, args));
		/* NOTREACHED */
	}
	/* #f #f #f fth-raise: reraise last exception */
	if (FTH_EXCEPTION_P(fth_last_exception)) {
		FTH msg;

		msg = fth_exception_last_message_ref(fth_last_exception);
		if (FTH_FALSE_P(msg))
			fth_errorf("#<%s>\n",
			    fth_exception_ref(fth_last_exception));
		else
			fth_errorf("#<%S>\n", msg);
	} else
		fth_errorf("#<no last exception found>\n");
	fth_show_backtrace(false);
	errno = 0;
	fth_reset_loop_and_depth();
	ficlVmReset(vm);
	ficlVmThrow(vm, FICL_VM_STATUS_ERROR_EXIT);
}

static void
ficl_stack_reset(ficlVm *vm)
{
#define h_stack_reset "( ?? -- )  reset stack\n\
stack-reset\n\
Reset the data stack to initial state."
	ficlStackReset(vm->dataStack);
}

#if defined(HAVE_GETTIMEOFDAY)
static struct timeval fth_timeval_tv;

static void
ficl_time(ficlVm *vm)
{
#define h_time "( -- r )  return time\n\
time => 4055.3\n\
Return real time, a ficlFloat.\n\
See gettimeofday(2) for more information.\n\
See also reset-time."
	struct timeval tv;
	double f;

	FTH_STACK_CHECK(vm, 0, 1);
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	if (gettimeofday(&tv, NULL) == -1)
		FTH_SYSTEM_ERROR_THROW(gettimeofday);
	f = (double)tv.tv_sec - (double)fth_timeval_tv.tv_sec;
	f += ((double)tv.tv_usec - (double)fth_timeval_tv.tv_usec) * 1e-6;
	ficlStackPushFloat(vm->dataStack, f);
}

/* ARGSUSED */
static void
ficl_time_reset(ficlVm *vm)
{
#define h_time_reset "( -- )  reset time\n\
time-reset\n\
Set global timeval struct variable to current time.\n\
See gettimeofday(2) for more information.\n\
See also time."
	(void)vm;
	if (gettimeofday(&fth_timeval_tv, NULL) == -1)
		FTH_SYSTEM_ERROR_THROW(gettimeofday);
}
#endif

static void
ficl_utime(ficlVm *vm)
{
#define h_utime "( -- utime stime )  return user and system time\n\
utime => 0.171875 0.0234375\n\
Return user and system time as ficlFloats.  \
Raise NOT-IMPLEMENTED exception if times(3) is not available.\n\
See times(3) for more information."
	double hertz;
#if defined(HAVE_TIMES) && defined(HAVE_STRUCT_TMS)
	struct tms buf;

#if defined(HAVE_SYSCONF) && defined(HAVE_DECL__SC_CLK_TCK)
	hertz = (double)sysconf(_SC_CLK_TCK);
#else
#if !defined(HZ)
#if defined(CLK_TCK)
#define HZ CLK_TCK
#else
#define HZ 60
#endif
#endif
	hertz = (double)HZ;
#endif
	if (hertz < 0.0)
		FTH_SYSTEM_ERROR_THROW(sysconf);
	times(&buf);
	ficlStackPushFloat(vm->dataStack, (double)buf.tms_utime / hertz);
	ficlStackPushFloat(vm->dataStack, (double)buf.tms_stime / hertz);
#else
	FTH_NOT_IMPLEMENTED_ERROR(times);
#endif
}

static void
ficl_current_time(ficlVm *vm)
{
#define h_current_time "( -- time-count )  return seconds since 1970\n\
currnet-time => 1326505228\n\
current-time time->string => \"Sat Jan 14 02:40:28 CET 2012\"\n\
Return time in seconds since 1970/01/01 as ficl2Unsigned.\n\
See time(3) for more information.\n\
See also strftime, strptime and time->string."
	FTH_STACK_CHECK(vm, 0, 1);
	ficlStackPush2Unsigned(vm->dataStack, (ficl2Unsigned)time(NULL));
}

static void
ficl_time_to_string(ficlVm *vm)
{
#define h_time_to_string "( secs -- str )  convert number to string\n\
current-time time->string => \"Sat Jan 14 02:40:28 CET 2012\"\n\
Convert ficl2Unsigned SECS in a date string in current local time.\n\
See also strftime, strptime and current-time."
	time_t tp;

	FTH_STACK_CHECK(vm, 1, 1);
	tp = (time_t)ficlStackPop2Unsigned(vm->dataStack);
	strftime(vm->pad, sizeof(vm->pad), "%a %b %d %H:%M:%S %Z %Y",
	    localtime(&tp));
	push_cstring(vm, vm->pad);
}

static void
ficl_strftime(ficlVm *vm)
{
#define h_strftime "( fmt secs -- str )  convert TIME according to FMT\n\
\"%a %b %d %H:%M:%S %Z %Y\" current-time strftime\n\
  => \"Sat Jan 14 02:40:28 CET 2012\"\n\
Convert ficl2Unsigned SECS in a date string corresponding to FMT.  \
The FMT string will be interpreted by strftime(3).\n\
See strftime(3) for more information.\n\
See also strptime, time->string and current-time."
	FTH fmt;
	time_t tp;

	FTH_STACK_CHECK(vm, 2, 1);
	tp = (time_t)ficlStackPop2Unsigned(vm->dataStack);
	fmt = fth_pop_ficl_cell(vm);
	FTH_ASSERT_ARGS(FTH_STRING_P(fmt), fmt, FTH_ARG1, "a string");
	strftime(vm->pad, sizeof(vm->pad), fth_string_ref(fmt), localtime(&tp));
	push_cstring(vm, vm->pad);
}

static void
ficl_strptime(ficlVm *vm)
{
#define h_strptime "( str fmt -- secs )  parse STR according to FMT\n\
\"2012 01 14\" \"%Y %m %d\" strptime time->string\n\
  => \"Sat Jan 14 02:40:28 CET 2012\"\n\
Parse STR according to FMT and return TIME as ficl2Unsigned.\n\
See strptime(3) for more information.\n\
See also strftime, time->string and current-time."
	FTH fmt, str;
	time_t tp;
	struct tm *tm;

	FTH_STACK_CHECK(vm, 2, 1);
	fmt = fth_pop_ficl_cell(vm);
	str = fth_pop_ficl_cell(vm);
	FTH_ASSERT_ARGS(FTH_STRING_P(str), str, FTH_ARG1, "a string");
	FTH_ASSERT_ARGS(FTH_STRING_P(fmt), fmt, FTH_ARG2, "a string");
	tp = time(NULL);
	tm = gmtime(&tp);
#if defined(HAVE_STRPTIME)
	if (strptime(fth_string_ref(str), fth_string_ref(fmt), tm) == NULL)
		FTH_SYSTEM_ERROR_ARG_THROW(strptime, fth_string_ref(str));
#endif
	ficlStackPush2Unsigned(vm->dataStack, (ficl2Unsigned)mktime(tm));
}

static void
ficl_localtime(ficlVm *vm)
{
#define h_localtime "( secs -- ary )  return SECS as local time array\n\
current-time localtime => #( 28 40 2 14 0 112 6 13 #f 3600 \"CET\" )\n\
Return array of eleven elements with SECS converted to local time:\n\
sec  	  -- seconds after minute [0-60]\n\
min  	  -- minutes after the hour [0-59]\n\
hour 	  -- hours since midnight [0-23]\n\
mday 	  -- day of the month [1-31]\n\
mon  	  -- months since January [0-11]\n\
year 	  -- years since 1900\n\
wday 	  -- days since Sunday [0-6]\n\
yday 	  -- days since January 1 [0-365]\n\
isdst     -- Daylight Savings Time flag\n\
tm_gmtoff -- offset from UTC in seconds\n\
tm_zone   -- timezone abbreviation\n\
See localtime(3) for more information.\n\
See also gmtime, mktime, strftime, strptime, time->string and current-time."
	FTH array;
	time_t tp;
	struct tm *tm;

	FTH_STACK_CHECK(vm, 1, 1);
	tp = (time_t)ficlStackPop2Unsigned(vm->dataStack);
	tm = localtime(&tp);
	array = fth_make_array_var(11,
	    INT_TO_FIX(tm->tm_sec),	/* seconds after minute [0-60] */
	    INT_TO_FIX(tm->tm_min),	/* minutes after the hour [0-59] */
	    INT_TO_FIX(tm->tm_hour),	/* hours since midnight [0-23] */
	    INT_TO_FIX(tm->tm_mday),	/* day of the month [1-31] */
	    INT_TO_FIX(tm->tm_mon),	/* months since January [0-11] */
	    INT_TO_FIX(tm->tm_year),	/* years since 1900 */
	    INT_TO_FIX(tm->tm_wday),	/* days since Sunday [0-6] */
	    INT_TO_FIX(tm->tm_yday),	/* days since January 1 [0-365] */
	    BOOL_TO_FTH(tm->tm_isdst),	/* Daylight Savings Time flag */
#if defined(HAVE_STRUCT_TM_TM_GMTOFF)
	/* offset from UTC in seconds */
	    fth_make_long_long((ficl2Integer)tm->tm_gmtoff),
#else
	    FTH_FALSE,
#endif
#if defined(HAVE_STRUCT_TM_TM_ZONE)
	    fth_make_string(tm->tm_zone)
#else
	    FTH_FALSE
#endif
	    );			/* timezone abbreviation */
	ficlStackPushFTH(vm->dataStack, array);
}

static void
ficl_gmtime(ficlVm *vm)
{
#define h_gmtime "( secs -- ary )  return TIME as an array in GMT\n\
current-time gmtime => #( 28 40 1 14 0 112 6 13 #f 0 \"UTC\" )\n\
Return array of eleven elements with SECS converted to Greenwich Mean Time:\n\
sec  	  -- seconds after minute [0-60]\n\
min  	  -- minutes after the hour [0-59]\n\
hour 	  -- hours since midnight [0-23]\n\
mday 	  -- day of the month [1-31]\n\
mon  	  -- months since January [0-11]\n\
year 	  -- years since 1900\n\
wday 	  -- days since Sunday [0-6]\n\
yday 	  -- days since January 1 [0-365]\n\
isdst     -- Daylight Savings Time flag\n\
tm_gmtoff -- offset from UTC in seconds\n\
tm_zone   -- timezone abbreviation\n\
See gmtime(3) for more information.\n\
See also localtime, mktime, strftime, strptime, time->string and current-time."
	FTH array;
	time_t tp;
	struct tm *tm;

	FTH_STACK_CHECK(vm, 1, 1);
	tp = (time_t)ficlStackPop2Unsigned(vm->dataStack);
	tm = gmtime(&tp);
	array = fth_make_array_var(11,
	    INT_TO_FIX(tm->tm_sec),	/* seconds after minute [0-60] */
	    INT_TO_FIX(tm->tm_min),	/* minutes after the hour [0-59] */
	    INT_TO_FIX(tm->tm_hour),	/* hours since midnight [0-23] */
	    INT_TO_FIX(tm->tm_mday),	/* day of the month [1-31] */
	    INT_TO_FIX(tm->tm_mon),	/* months since January [0-11] */
	    INT_TO_FIX(tm->tm_year),	/* years since 1900 */
	    INT_TO_FIX(tm->tm_wday),	/* days since Sunday [0-6] */
	    INT_TO_FIX(tm->tm_yday),	/* days since January 1 [0-365] */
	    BOOL_TO_FTH(tm->tm_isdst),	/* Daylight Savings Time flag */
#if defined(HAVE_STRUCT_TM_TM_GMTOFF)
	/* offset from UTC in seconds */
	    fth_make_long_long((ficl2Integer)tm->tm_gmtoff),
#else
	    FTH_FALSE,
#endif
#if defined(HAVE_STRUCT_TM_TM_ZONE)
	    fth_make_string(tm->tm_zone)	/* timezone abbreviation */
#else
	    FTH_FALSE
#endif
	    );
	ficlStackPushFTH(vm->dataStack, array);
}

enum {
	TM_SEC,
	TM_MIN,
	TM_HOUR,
	TM_MDAY,
	TM_MON,
	TM_YEAR,
	TM_WDAY,
	TM_YDAY,
	TM_ISDST,
	TM_GMTOFF,
	TM_ZONE
};

static void
ficl_mktime(ficlVm *vm)
{
#define h_mktime "( ary -- secs )  return time in seconds\n\
#( 28 40 2 14 0 112 6 13 #f 3600 \"CET\" ) mktime => 1326505228\n\
Return time constructed from values of ARY.  \
ARY may be #f or an array of up to eleven elements \
where single elements may be #f.\n\
sec  	  -- seconds after minute [0-60]\n\
min  	  -- minutes after the hour [0-59]\n\
hour 	  -- hours since midnight [0-23]\n\
mday 	  -- day of the month [1-31]\n\
mon  	  -- months since January [0-11]\n\
year 	  -- years since 1900\n\
wday 	  -- days since Sunday [0-6]\n\
yday 	  -- days since January 1 [0-365]\n\
isdst     -- Daylight Savings Time flag\n\
tm_gmtoff -- offset from UTC in seconds\n\
tm_zone   -- timezone abbreviation\n\
See mktime(3) for more information.\n\
See also localtime, gmtime, strftime, strptime, \
time->string and current-time."
	FTH array, el;
	ficlInteger len;
	struct tm tm;

	FTH_STACK_CHECK(vm, 1, 1);
	array = fth_pop_ficl_cell(vm);
	len = fth_array_length(array);
	if (len > TM_SEC) {
		el = fth_array_fast_ref(array, (ficlInteger)TM_SEC);
		if (FTH_NOT_FALSE_P(el))
			tm.tm_sec = FIX_TO_INT32(el);
	}
	if (len > TM_MIN) {
		el = fth_array_fast_ref(array, (ficlInteger)TM_MIN);
		if (FTH_NOT_FALSE_P(el))
			tm.tm_min = FIX_TO_INT32(el);
	}
	if (len > TM_HOUR) {
		el = fth_array_fast_ref(array, (ficlInteger)TM_HOUR);
		if (FTH_NOT_FALSE_P(el))
			tm.tm_hour = FIX_TO_INT32(el);
	}
	if (len > TM_MDAY) {
		el = fth_array_fast_ref(array, (ficlInteger)TM_MDAY);
		if (FTH_NOT_FALSE_P(el))
			tm.tm_mday = FIX_TO_INT32(el);
	}
	if (len > TM_MON) {
		el = fth_array_fast_ref(array, (ficlInteger)TM_MON);
		if (FTH_NOT_FALSE_P(el))
			tm.tm_mon = FIX_TO_INT32(el);
	}
	if (len > TM_YEAR) {
		el = fth_array_fast_ref(array, (ficlInteger)TM_YEAR);
		if (FTH_NOT_FALSE_P(el))
			tm.tm_year = FIX_TO_INT32(el);
	}
	if (len > TM_WDAY) {
		el = fth_array_fast_ref(array, (ficlInteger)TM_WDAY);
		if (FTH_NOT_FALSE_P(el))
			tm.tm_wday = FIX_TO_INT32(el);
	}
	if (len > TM_YDAY) {
		el = fth_array_fast_ref(array, (ficlInteger)TM_YDAY);
		if (FTH_NOT_FALSE_P(el))
			tm.tm_yday = FIX_TO_INT32(el);
	}
	if (len > TM_ISDST) {
		el = fth_array_fast_ref(array, (ficlInteger)TM_ISDST);
		tm.tm_isdst = FTH_TO_BOOL(el);
	}
#if defined(HAVE_STRUCT_TM_TM_GMTOFF)
	if (len > TM_GMTOFF) {
		el = fth_array_fast_ref(array, (ficlInteger)TM_GMTOFF);
		if (FTH_NOT_FALSE_P(el))
			tm.tm_gmtoff = (long)fth_long_long_ref(el);
	}
#endif
#if defined(HAVE_STRUCT_TM_TM_ZONE)
	if (len > TM_ZONE) {
		el = fth_array_fast_ref(array, (ficlInteger)TM_ZONE);
		if (FTH_NOT_FALSE_P(el))
			tm.tm_zone = fth_string_ref(el);
	}
#endif
	ficlStackPush2Unsigned(vm->dataStack, (ficl2Unsigned)mktime(&tm));
}

static void
ficl_getenv(ficlVm *vm)
{
#define h_getenv "( name -- value|#f )  return environment variable\n\
\"HOME\" getenv => /home/mike\n\
Return content of shell environment variable NAME as string \
or #f if variable is not defined.\n\
See getenv(3) for more information.\n\
See also putenv and environ."
	char *val = NULL;

	FTH_STACK_CHECK(vm, 1, 1);
	val = fth_getenv(pop_cstring(vm), NULL);
	if (val != NULL)
		push_cstring(vm, val);
	else
		ficlStackPushBoolean(vm->dataStack, false);
}

static void
ficl_putenv(ficlVm *vm)
{
#define h_putenv "( name value -- )  set environment variable\n\
\"my_var\" 10 putenv\n\
\"my_var\" getenv => 10\n\
Set VALUE to shell environment variable NAME.\n\
See putenv(3) for more information.\n\
See also getenv and environ."
	char *name, *val;

	FTH_STACK_CHECK(vm, 2, 0);
	val = fth_to_c_string(fth_pop_ficl_cell(vm));
	name = pop_cstring(vm);
#if defined(HAVE_SETENV)
	setenv(name, val, 1);
#endif
}

extern char   **environ;

static void
ficl_environ(ficlVm *vm)
{
#define h_environ "( -- ary )  return environment variables\n\
environ => #a( '( \"HOME\" . \"/home/mike\" ) ... )\n\
Return assoc array of all shell environment variables and their values.\n\
See also getenv and putenv."
	char **env;
	FTH vals;
	ficlInteger sep;

	env = environ;
	vals = fth_make_empty_array();
	for (; *env; env++) {
		sep = strchr(*env, '=') - *env;
		fth_assoc(vals, fth_make_string_len(*env, sep),
		    fth_make_string(*env + sep + 1));
	}
	ficlStackPushFTH(vm->dataStack, vals);
}

static void
ficl_getpid(ficlVm *vm)
{
#define h_getpid "( -- id )  return PID\n\
getpid => 49507\n\
Return process id.\n\
See getpid(2) for more information.\n\
See also getppid."
	FTH_STACK_CHECK(vm, 0, 1);
#if defined(HAVE_GETPID)
	ficlStackPushInteger(vm->dataStack, (ficlInteger)getpid());
#else
	ficlStackPushInteger(vm->dataStack, -1);
#endif
}

static void
ficl_getppid(ficlVm *vm)
{
#define h_getppid "( -- id )  return parent PID\n\
getppid => 49056\n\
Return parent process id.\n\
See getppid(2) for more information.\n\
See also getpid."
	FTH_STACK_CHECK(vm, 0, 1);
#if defined(HAVE_GETPPID)
	ficlStackPushInteger(vm->dataStack, (ficlInteger)getppid());
#else
	ficlStackPushInteger(vm->dataStack, -1);
#endif
}

static void
ficl_getuid(ficlVm *vm)
{
#define h_getuid "( -- id )  return UID\n\
getuid => 1001\n\
Return real user id of calling process.\n\
See getuid(2) for more information.\n\
See also geteuid, getgid and getegid."
	FTH_STACK_CHECK(vm, 0, 1);
#if defined(HAVE_GETUID)
	ficlStackPushInteger(vm->dataStack, (ficlInteger)getuid());
#else
	ficlStackPushInteger(vm->dataStack, -1);
#endif
}

static void
ficl_geteuid(ficlVm *vm)
{
#define h_geteuid "( -- id )  return effective UID\n\
geteuid => 1001\n\
Return effective user id of calling process.\n\
See geteuid(2) for more information.\n\
See also getuid, getgid and getegid."
#if defined(HAVE_GETEUID)
	FTH_STACK_CHECK(vm, 0, 1);
	ficlStackPushInteger(vm->dataStack, (ficlInteger)geteuid());
#else
	ficlStackPushInteger(vm->dataStack, -1);
#endif
}

static void
ficl_getgid(ficlVm *vm)
{
#define h_getgid "( -- id )  return GID\n\
getgid => 1001\n\
Return real group id of calling process.\n\
See getgid(2) for more information.\n\
See also getegid, getuid and geteuid."
	FTH_STACK_CHECK(vm, 0, 1);
#if defined(HAVE_GETGID)
	ficlStackPushInteger(vm->dataStack, (ficlInteger)getgid());
#else
	ficlStackPushInteger(vm->dataStack, -1);
#endif
}

static void
ficl_getegid(ficlVm *vm)
{
#define h_getegid "( -- id )  return effective GUI\n\
getegid => 1001\n\
Return effective group id of calling process.\n\
See getegid(2) for more information.\n\
See also getgid, getuid and geteuid."
	FTH_STACK_CHECK(vm, 0, 1);
#if defined(HAVE_GETEGID)
	ficlStackPushInteger(vm->dataStack, (ficlInteger)getegid());
#else
	ficlStackPushInteger(vm->dataStack, -1);
#endif
}

#if defined(HAVE_SETUID)
static void
ficl_setuid(ficlVm *vm)
{
#define h_setuid "( id -- )  set UID\n\
1001 setuid\n\
Set real user ID.  \
This is only permitted if ID is equal real UID \
or effective UID \
or the effective UID is that of the super user.\n\
See setuid(2) for more information.\n\
See also seteuid, setgid and setegid."
	FTH_STACK_CHECK(vm, 1, 0);
	if (setuid((uid_t)ficlStackPopInteger(vm->dataStack)) == -1)
		FTH_SYSTEM_ERROR_THROW(setuid);
}

#endif

#if defined(HAVE_SETEUID)
static void
ficl_seteuid(ficlVm *vm)
{
#define h_seteuid "( id -- )  set effective UID\n\
1001 seteuid\n\
Set effective user ID.  \
This is only permitted if ID is equal real UID \
or effective UID \
or the effective UID is that of the super user.\n\
See seteuid(2) for more information.\n\
See also setuid, setgid and setegid."
	FTH_STACK_CHECK(vm, 1, 0);
	if (seteuid((uid_t)ficlStackPopInteger(vm->dataStack)) == -1)
		FTH_SYSTEM_ERROR_THROW(seteuid);
}

#endif

#if defined(HAVE_SETGID)
static void
ficl_setgid(ficlVm *vm)
{
#define h_setgid "( id -- )  set GID\n\
1001 setgid\n\
Set real group ID.  \
This is only permitted if ID is equal real GID \
or effective GID \
or the effective UID is that of the super user.\n\
See setgid(2) for more information.\n\
See also setegid, setuid and seteuid."
	FTH_STACK_CHECK(vm, 1, 0);
	if (setgid((gid_t)ficlStackPopInteger(vm->dataStack)) == -1)
		FTH_SYSTEM_ERROR_THROW(setgid);
}

#endif

#if defined(HAVE_SETEGID)
static void
ficl_setegid(ficlVm *vm)
{
#define h_setegid "( id -- )  set effective GID\n\
1001 setegid\n\
Set effective group ID.  \
This is only permitted if ID is equal real GID \
or effective GID \
or the effective UID is that of the super user.\n\
See setegid(2) for more information.\n\
See also setgid, setuid and seteuid."
	FTH_STACK_CHECK(vm, 1, 0);
	if (setegid((gid_t)ficlStackPopInteger(vm->dataStack)) == -1)
		FTH_SYSTEM_ERROR_THROW(setegid);
}

#endif

#define FTH_SIGNAL_HANDLER	fth_symbol("signal-handler")

static ficlWord *sig_to_xt[40];

static void
handler_exec(int sig)
{
	ficlStackPushInteger(FTH_FICL_STACK(), (ficlInteger)sig);
	fth_execute_xt(FTH_FICL_VM(), sig_to_xt[sig]);
}

/*
 * old-handler ( sig -- )
 * A preserved old handler is wrapped in that XT.
 */
static void
ficl_signal_handler(ficlVm *vm)
{
	int sig;
	sig_t func;

	FTH_STACK_CHECK(vm, 1, 0);
	sig = (int)ficlStackPopInteger(vm->dataStack);
	func = (sig_t)fth_word_property_ref((FTH)vm->runningWord,
	    FTH_SIGNAL_HANDLER);
	if (func && func != SIG_DFL && func != SIG_IGN && func != SIG_ERR)
		(*func) (sig);
}

static void
ficl_signal(ficlVm *vm)
{
#define h_signal "( sig xt -- old-xt )  install signal handler\n\
SIGINT lambda: { sig -- }\n\
  .\" SIGINT received\" cr\n\
; signal value old-xt\n\
\\ later on you can reset signal to the previous handler:\n\
SIGINT old-xt signal drop\n\
Install XT for signal SIG as an signal handler and return old handler.  \
XT must take one value from the stack, the signal, \
and must not return any value; \
its stack effect is ( sig -- ).  \
The old xt handler can be preserved for later use.\n\
See signal(3) for more information."
	int sig;
	sig_t func;
	ficlWord *xt1, *xt2;

	FTH_STACK_CHECK(vm, 2, 1);
	xt1 = ficlStackPopPointer(vm->dataStack);
	sig = (int)ficlStackPopInteger(vm->dataStack);
	sig_to_xt[sig] = xt1;

	if (xt1 == (ficlWord *)SIG_DFL ||
	    xt1 == (ficlWord *)SIG_IGN ||
	    xt1 == (ficlWord *)SIG_ERR)
		func = signal(sig, (sig_t)xt1);
	else
		func = signal(sig, handler_exec);

	/* If something went wrong, signal returns SIG_ERR and sets errno. */
	if (func == SIG_ERR && errno != 0)
		FTH_SYSTEM_ERROR_THROW(signal);

	/* preserves old handler in xt2 */
	xt2 = FTH_PRI1("", ficl_signal_handler, NULL);
	fth_word_property_set((FTH)xt2, FTH_SIGNAL_HANDLER, (FTH)func);
	ficlStackPushPointer(vm->dataStack, xt2);
}

#if defined(HAVE_KILL)
static void
ficl_kill(ficlVm *vm)
{
#define h_kill "( pid sig -- )  send SIGnal to PID\n\
1234 SIGKILL kill\n\
0 SIGUSR1 kill\n\
Send signal SIG to process ID PID.  \
If PID is zero, send SIG to current process.  \
SIG is a number or a constant like SIGKILL.\n\
See kill(2) for more information."
	pid_t pid;
	int sig;

	FTH_STACK_CHECK(vm, 2, 0);
	sig = (int)ficlStackPopInteger(vm->dataStack);
	pid = (pid_t)ficlStackPopInteger(vm->dataStack);
	if (kill(pid, sig) == -1)
		FTH_SYSTEM_ERROR_THROW(kill);
}

#endif

static void
ficl_wait(ficlVm *vm)
{
#define h_wait "( -- pid )  wait for child\n\
wait => 1234\n\
Wait for child process and return its process ID.  \
Set global read only variable exit-status to wait status.\n\
See wait(2) for more information."
	pid_t pid = 0;
	int status = 0;

	FTH_STACK_CHECK(vm, 0, 1);
#if defined(HAVE_WAIT)
	pid = wait(&status);
	if (pid == -1)
		FTH_SYSTEM_ERROR_THROW(wait);
#endif
	fth_set_exit_status(status);
	ficlStackPushInteger(vm->dataStack, (ficlInteger)pid);
}

static void
ficl_waitpid(ficlVm *vm)
{
#define h_waitpid "( pid flags -- )  wait for specified child\n\
1234 0                    waitpid\n\
1234 WNOHANG              waitpid\n\
1234 WNOHANG WUNTRACED or waitpid\n\
Wait for child process PID.  \
Set global read only variable exit-status to wait status.  \
FLAGS may be 0, or WNOHANG and WUNTRACED ored.\n\
See waitpid(2) for more information."
	pid_t pid;
	int status = 0, flags;

	FTH_STACK_CHECK(vm, 2, 0);
	flags = (int)ficlStackPopInteger(vm->dataStack);
	pid = (pid_t)ficlStackPopInteger(vm->dataStack);
#if defined(HAVE_WAITPID)
	if (waitpid(pid, &status, flags) == -1)
		FTH_SYSTEM_ERROR_THROW(waitpid);
#endif
	fth_set_exit_status(status);
}

#if defined(HAVE_FORK)
static void
ficl_fork(ficlVm *vm)
{
#define h_fork "( xt -- pid )  execute XT in a child process\n\
lambda: <{}> \"ls -lAF\" exec ; fork value pid\n\
pid SIGKILL kill\n\
Create child process and execute XT in the child.  \
The child process returns nothing, parent returns child's process ID.\n\
See fork(2) for more information.\n\
See also exec."
	pid_t pid;
	FTH proc_or_xt, proc;

	FTH_STACK_CHECK(vm, 1, 1);
	proc_or_xt = fth_pop_ficl_cell(vm);
	proc = proc_from_proc_or_xt(proc_or_xt, 0, 0, false);
	FTH_ASSERT_ARGS(FTH_PROC_P(proc), proc, FTH_ARG1, "a proc");
	pid = fork();
	if (pid == -1)
		FTH_SYSTEM_ERROR_THROW(fork);

	if (pid == 0)		/* child */
		fth_proc_call(proc, RUNNING_WORD_VM(vm), 0);
	else			/* parent */
		ficlStackPushInteger(vm->dataStack, (ficlInteger)pid);
}
#endif

#if defined(HAVE_EXECLP) && defined(HAVE_EXECVP)
static void
ficl_exec(ficlVm *vm)
{
#define h_exec "( cmd -- )  replace current process by CMD\n\
lambda: <{}> #( \"ls\" \"-lAF\" ) exec ; fork\n\
lambda: <{}> \"ls -lAF [A-Z]*\" exec ; fork\n\
Replace current process by running CMD as shell command.  \
If CMD is a string, shell expansion takes place and $SHELL--\
or \"sh\" if $SHELL is empty--executes CMD.  \
If CMD is an array of strings, no shell expansion takes place \
and \"CMD 0 array-ref\" should be a program name.\n\
See exec(3) for more information.\n\
See also fork."
	FTH cmd;
	char *prog;

	FTH_STACK_CHECK(vm, 1, 0);
	cmd = fth_pop_ficl_cell(vm);
	FTH_ASSERT_ARGS(FTH_STRING_P(cmd) || FTH_ARRAY_P(cmd),
	    cmd, FTH_ARG1, "a string or an array of strings");
	/* cmd == string: execute shell expansion */
	if (FTH_STRING_P(cmd)) {
		char *shell;

		prog = fth_string_ref(cmd);
		shell = fth_getenv("SHELL", "sh");

		if (execlp(shell, shell, "-c", prog, NULL) == -1)
			FTH_SYSTEM_ERROR_ARG_THROW(execlp, prog);
	} else {
		ficlInteger argc;

		argc = fth_array_length(cmd);
		if (argc > 0) {
			ficlInteger i;
			int status;
			char **argv;

			prog = fth_string_ref(fth_array_fast_ref(cmd, 0L));
			argv = FTH_MALLOC(sizeof(char *) * (size_t)(argc + 1));

			for (i = 0; i < argc; i++)
				argv[i] =
				    fth_string_ref(fth_array_fast_ref(cmd, i));

			argv[i] = NULL;
			status = execvp(prog, argv);
			FTH_FREE(argv);

			if (status == -1)
				FTH_SYSTEM_ERROR_ARG_THROW(execvp, prog);
		}
	}
}
#endif

static void
ficl_getlogin(ficlVm *vm)
{
#define h_getlogin "( -- str )  return login name\n\
getlogin => \"mike\"\n\
Return name of user associated with current session.\n\
See getlogin(2) for more information."
	char *name;

	FTH_STACK_CHECK(vm, 0, 1);
	errno = 0;
#if defined(HAVE_GETLOGIN)
	name = getlogin();
	if (name == NULL)
#endif
		name = fth_getenv("LOGNAME", "anonymous");
	push_cstring(vm, name);
}

static void
ficl_gethostname(ficlVm *vm)
{
#define h_gethostname "( -- str )  return host name\n\
gethostname => \"pumpkin.fth-devel.net\"\n\
Return name of current host.\n\
See gethostname(3) for more information."
#if defined(HAVE_GETHOSTNAME)
	FTH_STACK_CHECK(vm, 0, 1);
	if (gethostname(vm->pad, (size_t)FICL_PAD_SIZE) == -1)
		FTH_SYSTEM_ERROR_THROW(gethostname);
	push_cstring(vm, vm->pad);
#else
	push_cstring(vm, "localhost");
#endif
}

static void
ficl_sethostname(ficlVm *vm)
{
#define h_sethostname "( str -- )  set host name\n\
\"pumpkin.fth-devel.net\" sethostname\n\
Set name of current host to STR.  \
This call is restricted to the super-user.  \
Raise NOT-IMPLEMENTED exception if sethostname(3) is not available.\n\
See sethostname(3) for more information."
	char *name;

	FTH_STACK_CHECK(vm, 1, 0);
	name = pop_cstring(vm);
#if defined(HAVE_SETHOSTNAME)
#if defined(__FreeBSD__)
	/* int sethostname(const char *, int) */
	if (sethostname(name, (int)fth_strlen(name)) == -1)
#else
	/* int sethostname(const char *, size_t) */
	if (sethostname(name, fth_strlen(name)) == -1)
#endif
		FTH_SYSTEM_ERROR_ARG_THROW(sethostname, name);
#else
	FTH_NOT_IMPLEMENTED_ERROR(sethostname);
#endif
}

#if defined(HAVE_NETDB_H)
#include <netdb.h>
#endif

static void
ficl_getservbyname(ficlVm *vm)
{
#define h_getservbyname "( str -- ary )  return service info array\n\
\"smtp\" getservbyname => #( \"smtp\" #( \"mail\" ) 25 \"tcp\" )\n\
Return array containing the service, an array of aliases, \
the port number and the protocol.  \
Raise NOT-IMPLEMENTED exception if getservbyname(3) is not available.\n\
See getservbyname(3) for more information."
	char *service, **s_aliases;
	struct servent *se;
	FTH aliases, res;

	FTH_STACK_CHECK(vm, 1, 1);
	service = pop_cstring(vm);
#if defined(HAVE_GETSERVBYNAME)
	se = getservbyname(service, NULL);
	if (se == NULL) {
		if (errno != 0) {
			FTH_SYSTEM_ERROR_ARG_THROW(getservbyname, service);
			/* NOTREACHED */
			return;
		}
		ficlStackPushBoolean(vm->dataStack, false);
		return;
	}
	aliases = fth_make_empty_array();
	s_aliases = se->s_aliases;
	while (*s_aliases)
		fth_array_push(aliases, fth_make_string(*s_aliases++));
	res = FTH_LIST_4(fth_make_string(se->s_name),
	    aliases,
	    INT_TO_FIX(ntohs((uint16_t)se->s_port)),
	    fth_make_string(se->s_proto));
	ficlStackPushFTH(vm->dataStack, res);
#else
	FTH_NOT_IMPLEMENTED_ERROR(getservbyname);
#endif
}

static void
ficl_getservbyport(ficlVm *vm)
{
#define h_getservbyport "( port -- ary )  return service info array\n\
25 getservbyport => #( \"smtp\" #( \"mail\" ) 25 \"tcp\" )\n\
Return array containing the service, an array of aliases, \
the port number and the protocol.  \
Raise NOT-IMPLEMENTED exception if getservbyport(3) is not available.\n\
See getservbyport(3) for more information."
	uint16_t port;
	struct servent *se;
	FTH aliases, res;
	char **s_aliases;

	FTH_STACK_CHECK(vm, 1, 1);
	port = (uint16_t)ficlStackPopInteger(vm->dataStack);
#if defined(HAVE_GETSERVBYPORT)
	se = getservbyport((int)htons(port), NULL);
	if (se == NULL) {
		if (errno != 0) {
			FTH_SYSTEM_ERROR_THROW(getservbyport);
			/* NOTREACHED */
			return;
		}
		ficlStackPushBoolean(vm->dataStack, false);
		return;
	}
	aliases = fth_make_empty_array();
	s_aliases = se->s_aliases;
	while (*s_aliases)
		fth_array_push(aliases, fth_make_string(*s_aliases++));
	res = FTH_LIST_4(fth_make_string(se->s_name),
  	    aliases,
	    INT_TO_FIX(ntohs((uint16_t)se->s_port)),
	    fth_make_string(se->s_proto));
	ficlStackPushFTH(vm->dataStack, res);
#else
	FTH_NOT_IMPLEMENTED_ERROR(getservbyport);
#endif
}

static void
ficl_date(ficlVm *vm)
{
#define h_date "( -- str )  return date as string\n\
date => Mon Nov 21 04:34:07 CET 2005\n\
Return date in default Unix/C format as a string."
	time_t tp;

	FTH_STACK_CHECK(vm, 0, 1);
	time(&tp);
	strftime(vm->pad, (size_t)FICL_PAD_SIZE,
	    "%a %b %d %H:%M:%S %Z %Y", localtime(&tp));
	push_cstring(vm, vm->pad);
}

#if defined(HAVE_SLEEP)
static void
ficl_sleep(ficlVm *vm)
{
#define h_sleep "( secs -- )  wait for SECS seconds\n\
3 sleep\n\
Pause for SECS seconds.\n\
See sleep(3) for more information."
	FTH_STACK_CHECK(vm, 1, 0);
	sleep((unsigned)ficlStackPopUnsigned(vm->dataStack));
}
#endif

/* === GETOPT === */

#include <getopt.h>

/*
 * external variables (unistd.h) for getopt(3)
 */
extern char    *optarg;
extern int 	opterr;
extern int 	optind;
extern int 	optopt;

#define h_opterr "\
If #t, the default, getopt print error message in case of an error, \
if #f, no message will be printed.  \
See getopt(3) for more information."

#define h_optopt "\
If getopt finds unknown options or getopt misses required arguments, \
it stores that option in this variable.  \
See getopt(3) for more information."

#define h_optind "\
Getopt set this variable to the index of the next \
element of the *argv* array.  \
See getopt(3) for more information."

#define h_optarg "\
Getopt set this variable to the option string of \
an argument which accepts options, \
otherwise to #f.  \
See getopt(3) for more information."

#define GETOPT_MAX_ARGS		24

static void
ficl_getopt(ficlVm *vm)
{
#define h_getopt "( argv opt-str -- c )  command line options\n\
% cat ./getopt-test.fth\n\
#! /usr/local/bin/fth -s\n\
: main\n\
  #f #f { bflag ffile }\n\
  #t to opterr                          \\ getopt prints error messages\n\
  begin\n\
    *argv* \"bf:\" getopt ( ch ) dup\n\
  while ( ch )\n\
      case\n\
        <char> b of #t     to bflag endof\n\
        <char> f of optarg to ffile endof\n\
        <char> ? of\n\
          $\" usage: [-b] [-f file]\\n\" #() fth-print\n\
          1 (bye)                       \\ exit with return code 1\n\
        endof\n\
      endcase\n\
  repeat ( ch ) drop\n\
  optind 0 ?do *argv* array-shift drop loop\n\
  *argv* array-length to *argc*\n\
  $\" -b: %s, -f: %s\\n\" #( bflag ffile ) fth-print\n\
;\n\
main\n\
0 (bye)                                 \\ exit with return code 0\n\
% ./getopt-test.fth             => -b: #f, -f: #f\n\
% ./getopt-test.fth -b          => -b: #t, -f: #f\n\
% ./getopt-test.fth -bf outfile => -b: #t, -f: outfile\n\
% ./getopt-test.fth -f\n\
=> fth: option requires an argument -- f\n\
=> usage: [-b] [-f file]\n\
% ./getopt-test.fth -h\n\
=> fth: illegal option -- h\n\
=> usage: [-b] [-f file]\n\
Return next option character from command line options.  \
This is the example from getopt(3).\n\
See getopt(3) for more information."
	FTH args;
	char *options, *argv[GETOPT_MAX_ARGS];
	ficlInteger i;
	int argc, c;

	FTH_STACK_CHECK(vm, 2, 1);
	options = pop_cstring(vm);
	args = fth_pop_ficl_cell(vm);
	/* prepare argv */
	argc = (int)fth_array_length(args);
	argc = FICL_MIN(GETOPT_MAX_ARGS - 1, argc);
	for (i = 0; i < argc; i++)
		argv[i] = fth_string_ref(fth_array_fast_ref(args, i));
	argv[i] = NULL;
	opterr = FTH_NOT_FALSE_P(fth_variable_ref("opterr"));
	optind = FIX_TO_INT32(fth_variable_ref("optind"));
	if (optind < 1)
		optind = 1;
	c = getopt(argc, argv, options);
	if (c == -1) {
		optind = 1;	/* reset getopt for further use */
		ficlStackPushBoolean(vm->dataStack, false);
	} else {
		fth_variable_set("optind", INT_TO_FIX(optind));
		fth_variable_set("optopt",
		    optopt ? INT_TO_FIX(optopt) : FTH_FALSE);
		fth_variable_set("optarg",
		    optarg ? fth_make_string(optarg) : FTH_FALSE);
		ficlStackPushInteger(vm->dataStack, (ficlInteger)c);
	}
}

static void
ficl_getopt_long(ficlVm *vm)
{
#define h_getopt_long "( argv opts long-opts -- c )  command line options\n\
: long-test\n\
  #f #f { bflag ffile }\n\
  #f to opterr\n\
  #( #( \"flag\"       no-argument <char> b )\n\
     #( \"file\" required-argument <char> f ) ) { opts }\n\
  begin\n\
    *argv* \"bf:\" opts getopt-long ( ch ) dup\n\
  while ( ch )\n\
      case\n\
	<char> b of #t     to bflag endof\n\
	<char> f of optarg to ffile endof\n\
	<char> ? of\n\
	  \"-%c requires an argument\" #( optopt ) fth-warning\n\
	endof\n\
      endcase\n\
  repeat\n\
  drop ( ch )\n\
  optind 0 ?do *argv* array-shift drop loop\n\
  *argv* array-length to *argc*\n\
  \"-b, --flag (default #f): %s\\n\" #( bflag ) fth-print\n\
  \"-f, --file (default #f): %s\\n\" #( ffile ) fth-print\n\
;\n\
Return next option character from command line options.  \
The example may show differences and similarities to getopt_long(3).\n\
See getopt_long(3) for more information."
	FTH args, long_args;
	char *options, *argv[GETOPT_MAX_ARGS];
	struct option opts[GETOPT_MAX_ARGS];
	int argc, optc, c;
	ficlInteger i;

	FTH_STACK_CHECK(vm, 3, 1);
	long_args = fth_pop_ficl_cell(vm);
	options = pop_cstring(vm);
	args = fth_pop_ficl_cell(vm);
	argc = (int)fth_array_length(args);
	argc = FICL_MIN(GETOPT_MAX_ARGS - 1, argc);
	optc = (int)fth_array_length(long_args);
	optc = FICL_MIN(GETOPT_MAX_ARGS - 1, optc);
	/* prepare argv */
	for (i = 0; i < argc; i++)
		argv[i] = fth_string_ref(fth_array_fast_ref(args, i));
	argv[i] = NULL;
	/* prepare opts */
	for (i = 0; i < optc; i++) {
		FTH opt;

		opt = fth_array_fast_ref(long_args, i);
		if (fth_array_length(opt) == 3) {
			opts[i].name =
			    fth_string_ref(fth_array_fast_ref(opt, 0L));
			opts[i].has_arg =
			    FIX_TO_INT32(fth_array_fast_ref(opt, 1L));
			opts[i].flag = NULL;
			opts[i].val =
			    FIX_TO_INT32(fth_array_fast_ref(opt, 2L));
		} else
			FTH_ASSERT_ARGS(false, opt, FTH_ARG3,
			    "an array of length 3");
	}
	opts[i].name = 0;
	opts[i].has_arg = 0;
	opts[i].flag = 0;
	opts[i].val = 0;
	opterr = FTH_NOT_FALSE_P(fth_variable_ref("opterr"));
	optind = FIX_TO_INT32(fth_variable_ref("optind"));
	if (optind < 1)
		optind = 1;
	c = getopt_long(argc, argv, options, opts, NULL);
	if (c == -1) {
		optind = 1;	/* reset getopt for further use */
		ficlStackPushBoolean(vm->dataStack, false);
	} else {
		fth_variable_set("optind", INT_TO_FIX(optind));
		fth_variable_set("optopt",
		    optopt ? INT_TO_FIX(optopt) : FTH_FALSE);
		fth_variable_set("optarg",
		    optarg ? fth_make_string(optarg) : FTH_FALSE);
		ficlStackPushInteger(vm->dataStack, (ficlInteger)c);
	}
}

static void
ficl_show_memory(ficlVm *vm)
{
#define h_show_memory "( -- )  print memory stats\n\
.memory => 398461 cells used, 650115 cells free\n\
Print used and free dictionary cells."
	fth_printf("%d cells used, %d cells free",
	    ficlDictionaryCellsUsed(ficlVmGetDictionary(vm)),
	    ficlDictionaryCellsAvailable(ficlVmGetDictionary(vm)));
}

char *
fth_version(void)
{
	if (strncmp("unknown", FTH_TARGET_VENDOR, 7L) == 0)
		return (FTH_PACKAGE_VERSION " (" FTH_DATE ") [" FTH_TARGET_CPU
		    "-" FTH_TARGET_OS "]");
	return (FTH_PACKAGE_VERSION " (" FTH_DATE ") [" FTH_TARGET "]");
}

char *
fth_short_version(void)
{
	return (FTH_PACKAGE_VERSION " (" FTH_DATE ")");
}

static void
ficl_version(ficlVm *vm)
{
#define h_ficl_version "( -- addr len )  return FTH version\n\
ver type => \"1.2.9 (28-Jan-2012) [i386-portbld-freebsd9.0]\"\n\
Return fth-version as a Forth string with ADDR LEN.\n\
See also .version and .long-version."
	FTH_STACK_CHECK(vm, 0, 2);
	push_forth_string(vm, fth_version());
}

/* ARGSUSED */
static void
ficl_dot_lversion(ficlVm *vm)
{
#define h_dot_lversion "( -- )  print FTH version\n\
.long-version => FTH 1.2.9 (28-Jan-2012) [i386-portbld-freebsd9.0]\n\
Print long package version.\n\
See also .version and ver"
	(void)vm;
	fth_printf("FTH %s\n", fth_version());
}

/* ARGSUSED */
static void
ficl_dot_version(ficlVm *vm)
{
#define h_dot_version "( -- )  print FTH version\n\
.version => 1.2.9\n\
Print package version number.\n\
See also .long-version and ver."
	(void)vm;
	fth_print(FTH_PACKAGE_VERSION "\n");
}

/* ARGSUSED */
static void
ficl_dot_prefix(ficlVm *vm)
{
#define h_dot_prefix "( -- )  print install prefix\n\
.prefix => /usr/local\n\
Print installation prefix path.\n\
See also .cflags and .libs."
	(void)vm;
	fth_print(FTH_PREFIX_PATH "\n");
}

/* ARGSUSED */
static void
ficl_dot_cflags(ficlVm *vm)
{
#define h_dot_cflags "( -- )  print compile flags\n\
.cflags => -I/usr/local/include/fth\n\
Print compiler flags to compile libfth.so to other applications.\n\
See also .prefix and .libs."
	(void)vm;
	fth_print("-I" FTH_PREFIX_PATH "/include/" FTH_PROG_NAME "\n");
}

/* ARGSUSED */
static void
ficl_dot_libs(ficlVm *vm)
{
#define h_dot_libs "( -- )  print link flags\n\
.libs => -L/usr/local/lib -lfth -lm\n\
Print linker flags to link libfth.so to other applications.\n\
See also .cflags and .prefix."
	(void)vm;
	fth_print("-L" FTH_PREFIX_PATH "/lib -l" FTH_PROG_NAME " ");
#if defined(FTH_STATIC)
	fth_print(FTH_LIBS "\n");
#else
	fth_print("-lm\n");
#endif
}

static void
ficl_config_prefix(ficlVm *vm)
{
#define h_config_prefix "( -- str )  return install prefix\n\
config-prefix => \"/usr/local\"\n\
Return installation prefix path.\n\
See also config-cflags and config-libs."
	FTH_STACK_CHECK(vm, 0, 1);
	push_cstring(vm, FTH_PREFIX_PATH);
}

static void
ficl_config_cflags(ficlVm *vm)
{
#define h_config_cflags "( -- str )  return compile flags\n\
config-cflags => \"-I/usr/local/include/fth\"\n\
Return compiler flags to compile libfth.so to other applications.\n\
See also config-prefix and config-libs."
	FTH_STACK_CHECK(vm, 0, 1);
	push_cstring(vm, "-I" FTH_PREFIX_PATH "/include/" FTH_PROG_NAME);
}

static void
ficl_config_libs(ficlVm *vm)
{
#define h_config_libs "( -- str )  return link flags\n\
config-libs => \"-L/usr/local/lib -lfth -lm\"\n\
Return linker flags to link libfth.so to other applications.\n\
See also config-prefix and config-cflags."
	FTH_STACK_CHECK(vm, 0, 1);
	push_cstring(vm, "-L" FTH_PREFIX_PATH "/lib -l" FTH_PROG_NAME " -lm");
}

static void
ficl_configure_args(ficlVm *vm)
{
#define h_configure_args "( -- str )  return configure arguments\n\
configure-args => \" '--with-tecla-prefix=/usr/local' ...\"\n\
Return configure arguments."
	FTH_STACK_CHECK(vm, 0, 1);
	push_cstring(vm, FTH_CONFIGURE_ARGS);
}

static FTH
at_exit_each(FTH proc, FTH name)
{
	fth_proc_call(proc, (char *)name, 0);
	return (name);
}

static void
run_at_exit(void)
{
	FTH rw;
#if !defined(_WIN32)
	int i;

	for (i = 0; i < FTH_SIGNALS; i++)
		signal(fth_signals[i], SIG_DFL);
#endif
	rw = (FTH)RUNNING_WORD();
	if (fth_array_length(fth_at_exit_procs) > 0)
		fth_array_each(fth_at_exit_procs, at_exit_each, rw);
	fth_reset_loop_and_depth();
	simple_array_free(depth_array);
	simple_array_free(loop_array);
	gc_free_all();
#if HAVE_BN
	free_number_types();
#endif
	ficlSystemDestroy(FTH_FICL_SYSTEM());
	FTH_FREE(FTH_FICL_VAR());
}

static void
ficl_at_exit(ficlVm *vm)
{
#define h_at_exit "( obj -- )  set clean-up function\n\
lambda: <{ -- }> \"test.file\" file-delete ; at-exit\n\
OBJ, a proc or xt, will be called by Fth's exit function.  \
More than one calls to AT-EXIT are possible, \
all procs or xts will be called in order.  \
The stack effect of OBJ must be ( -- ).\n\
See atexit(3) for more information."
	FTH proc_or_xt, proc;

	FTH_STACK_CHECK(vm, 1, 0);
	proc_or_xt = fth_pop_ficl_cell(vm);
	proc = proc_from_proc_or_xt(proc_or_xt, 0, 0, false);
	FTH_ASSERT_ARGS(FTH_PROC_P(proc), proc, FTH_ARG1, "a proc");
	if (fth_array_length(fth_at_exit_procs) > 0)
		fth_array_push(fth_at_exit_procs, proc);
	else
		fth_at_exit_procs = fth_make_array_var(1, proc);
}

static void
ficl_exit(ficlVm *vm)
{
#define h_exit "( n -- )  exit FTH\n\
2 (bye) => \\ exits FTH with exit code 2\n\
The exit hook fth_exit_hook will be called if set, \
all procs registered for at-exit will be executed \
and the current process will be terminated with exit code N.\n\
See exit(3) for more information.\n\
See also bye and at-exit."
	FTH_STACK_CHECK(vm, 1, 0);
	fth_exit((int)ficlStackPopInteger(vm->dataStack));
}

/*
 * Convenient functions for
 * >array     >assoc       >list      >alist     >hash
 * #( ... )   #a( ... )   '( ... )   'a( ... )   #{ ... }
 *
 * Creating an array:
 *
 * 'a 'b 'c 'd  4  >array
 *
 * or
 *
 * #( 'a 'b 'c 'd )
 *
 */

static ficlWord *set_begin_paren;
static ficlWord *set_end_paren;
static ficlWord *loop_begin;
static ficlWord *loop_until;

void
fth_reset_loop_and_depth(void)
{
	unsigned int i;

	for (i = 0; i < depth_array->length; i++)
		simple_array_free(simple_array_pop(depth_array));
	simple_array_clear(depth_array);
	simple_array_clear(loop_array);
}

static void
ficl_set_begin_paren(ficlVm *vm)
{
	ficlWord *to_obj;
	FTH args;

	FTH_STACK_CHECK(vm, 2, 0);
	to_obj = ficlStackPopPointer(vm->dataStack);
	args = fth_pop_ficl_cell(vm);
	simple_array_push(depth_array,
	    make_simple_array_var(3, FTH_STACK_DEPTH(vm), to_obj,
	    (void *)args));
}

static void
ficl_set_end_paren(ficlVm *vm)
{
	simple_array *ary;
	ficlInteger depth;

	ary = simple_array_pop(depth_array);
	depth = FTH_STACK_DEPTH(vm) - (ficlInteger)simple_array_ref(ary, 0);
	simple_array_free(ary);
	ficlStackPushInteger(vm->dataStack, depth);
}

/* dbm.c needs it too. */
void
fth_begin_values_to_obj(ficlVm *vm, char *name, FTH args)
{
	ficlWord *to_obj;

	to_obj = FICL_WORD_NAME_REF(name);
	if (vm->state == FICL_VM_STATE_COMPILE) {
		ficlDictionary *dict;

		dict = ficlVmGetDictionary(vm);
		ficlDictionaryAppendUnsigned(dict,
		    (ficlUnsigned)ficlInstructionLiteralParen);
		ficlDictionaryAppendFTH(dict, args);
		ficlDictionaryAppendUnsigned(dict,
		    (ficlUnsigned)ficlInstructionLiteralParen);
		ficlDictionaryAppendPointer(dict, to_obj);
		ficlDictionaryAppendPointer(dict, set_begin_paren);
		ficlDictionaryAppendPointer(dict, loop_begin);
	} else
		simple_array_push(depth_array,
		    make_simple_array_var(3, FTH_STACK_DEPTH(vm), to_obj,
		    (void *)args));
}

#define FTH_BEGIN_OBJ(name)						\
static void								\
ficl_begin_ ## name (ficlVm *vm)					\
{									\
	fth_begin_values_to_obj(vm, ">" # name, FTH_FALSE);		\
}

FTH_BEGIN_OBJ(array)
FTH_BEGIN_OBJ(assoc)
FTH_BEGIN_OBJ(list)
FTH_BEGIN_OBJ(alist)
FTH_BEGIN_OBJ(hash)

static void
ficl_values_end(ficlVm *vm)
{
	ficlWord *to_obj;
	FTH args;
	int len;

	len = simple_array_length(depth_array);
	if (len <= 0)
		FTH_BAD_SYNTAX_ERROR("orphaned closing paren found");
	to_obj = FICL_WORD_REF(simple_array_ref(simple_array_ref(depth_array,
	    len - 1), 1));
	args = (FTH)simple_array_ref(simple_array_ref(depth_array,
	    len - 1), 2);
	if (vm->state == FICL_VM_STATE_COMPILE) {
		ficlDictionary *dict;

		dict = ficlVmGetDictionary(vm);
		ficlDictionaryAppendUnsigned(dict,
		    (ficlUnsigned)ficlInstructionLiteralParen);
		ficlDictionaryAppendInteger(dict, (ficlInteger)FICL_TRUE);
		ficlDictionaryAppendPointer(dict, loop_until);
		ficlDictionaryAppendPointer(dict, set_end_paren);
		/* >dbm needs an argument, the filename */
		if (FTH_NOT_FALSE_P(args)) {
			ficlDictionaryAppendUnsigned(dict,
			    (ficlUnsigned)ficlInstructionLiteralParen);
			ficlDictionaryAppendFTH(dict, args);
		}
		ficlDictionaryAppendPointer(dict, to_obj);
	} else {
		simple_array *ary;
		ficlInteger depth;

		ary = simple_array_pop(depth_array);
		depth = FTH_STACK_DEPTH(vm) - 
		    (ficlInteger)simple_array_ref(ary, 0);
		simple_array_free(ary);
		ficlStackPushInteger(vm->dataStack, depth);
		/* >dbm needs an argument, the filename */
		if (FTH_NOT_FALSE_P(args))
			fth_push_ficl_cell(vm, args);
		ficlVmExecuteXT(vm, to_obj);
	}
}

/* === EACH-MAP === */

static void
ficl_set_each_loop_paren(ficlVm *vm)
{
#define h_set_each_l_paren "( obj -- len )  set each/end-each object\n\
Helper function for each ... end-each.  \
Sets array representation of OBJ to loop_array and returns OBJ's length.\n\
See also (reset-each), (fetch) and source file fth.fs."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	simple_array_push(loop_array, (void *)fth_object_copy(obj));
	ficlStackPushInteger(vm->dataStack, fth_object_length(obj));
}

static void
ficl_set_map_loop_paren(ficlVm *vm)
{
#define h_set_map_l_paren "( obj -- len )  set map/end-map object\n\
Helper function for map ... end-map.  \
Sets copy of OBJ to loop_array and returns OBJ's length.\n\
See also (set-map-loop), (reset-map), (fetch), (store) and source file fth.fs."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	simple_array_push(loop_array, (void *)obj);
	ficlStackPushInteger(vm->dataStack, fth_object_length(obj));
}

/* ARGSUSED */
static void
ficl_reset_each_paren(ficlVm *vm)
{
#define h_re_each_paren "( -- )  reset each/end-each object\n\
Helper function for each ... end-each.  \
Removes last object from loop-array.\n\
See also (set-each-loop), (fetch) and source file fth.fs."
	(void)vm;
	simple_array_pop(loop_array);
}

static void
ficl_reset_map_paren(ficlVm *vm)
{
#define h_re_map_paren "( -- obj )  reset map/end-map object\n\
Helper function for map ... end-map.  \
Removes last object from loop-array and retuns it.\n\
See also (set-map-loop), (fetch), (store) and source file fth.fs."
	FTH_STACK_CHECK(vm, 0, 1);
	ficlStackPushFTH(vm->dataStack, (FTH)simple_array_pop(loop_array));
}

static void
ficl_fetch_paren(ficlVm *vm)
{
#define h_fetch_paren "( index -- value )  fetch next value from object\n\
Helper function for each ... end-each and map ... end-map.  \
Returns value on INDEX of last object in loop-array.\n\
See also (set-each-loop), (reset-each), (store) and source file fth.fs."
	ficlInteger idx;
	int len;

	FTH_STACK_CHECK(vm, 1, 1);
	idx = ficlStackPopInteger(vm->dataStack);
	len = simple_array_length(loop_array);
	if (len > 0)
		fth_push_ficl_cell(vm,
		    fth_object_value_ref((FTH)simple_array_ref(loop_array,
		    len - 1), idx));
	else
		ficlStackPushBoolean(vm->dataStack, false);
}

static void
ficl_store_paren(ficlVm *vm)
{
#define h_store_paren "( value index -- )  store value to object\n\
Helper function for map ... end-map.  \
Stores VALUE on INDEX of last object in loop-array.\n\
See also (set-map-loop), (reset-map), (fetch) and source file fth.fs."
	FTH value;
	ficlInteger idx;
	int len;

	FTH_STACK_CHECK(vm, 2, 0);
	idx = ficlStackPopInteger(vm->dataStack);
	value = fth_pop_ficl_cell(vm);
	len = simple_array_length(loop_array);
	if (len > 0)
		fth_object_value_set((FTH)simple_array_ref(loop_array,
		    len - 1), idx, value);
}

/* === Initialize === */
void
forth_init(void)
{
	ficlDictionary *dict;

	INIT_ASSERT(FTH_FICL_VAR());
	dict = FTH_FICL_DICT();
	FTH_FICL_VM()->runningWord = NULL;
	if (atexit(run_at_exit) == -1)
		FTH_SYSTEM_ERROR_THROW(atexit);
	/* Load Ficl source files. */
	{
		char *sf[] = {"softcore.fr",
			"ifbrack.fr",
			"prefix.fr",
			"ficl.fr",
			"jhlocal.fr",
			"marker.fr",
			"fileaccess.fr",
			"assert.fs",
			"compat.fs",
		NULL};
		char **softcore = sf;

		while (*softcore) {
			fth_var_set(fth_last_exception, FTH_FALSE);
			fth_require_file(*softcore++);
		}
	}
	/* constants */
	fth_define_constant("cell", sizeof(ficlCell), NULL);
	fth_define_constant("float", sizeof(ficlFloat), NULL);
	fth_define_constant("sfloat", sizeof(ficlFloat), NULL);
	fth_define_constant("dfloat", sizeof(ficlFloat), NULL);
	fth_define_constant("ficl-version", fth_make_string(FICL_VERSION),
	    NULL);
	fth_define_constant("fth-version", fth_make_string(fth_version()),
	    NULL);
	fth_define_constant("fth-date", fth_make_string(FTH_DATE), NULL);
	fth_define_constant("INTERPRET_STATE", FICL_VM_STATE_INTERPRET, NULL);
	fth_define_constant("COMPILE_STATE", FICL_VM_STATE_COMPILE, NULL);
	fth_define_constant("FICL_VM_STATE_INTERPRET", FICL_VM_STATE_INTERPRET,
	    NULL);
	fth_define_constant("FICL_VM_STATE_COMPILE", FICL_VM_STATE_COMPILE,
	    NULL);
	/* misc */
	FTH_PRI1("add-feature", ficl_add_feature, h_add_feature);
	FTH_PRI1("provided?", ficl_provided_p, h_provided_p);
	FTH_PRI1("*features*", ficl_features, h_features);
	FTH_PRI1("add-load-path", ficl_add_load_path, h_add_load_path);
	FTH_PRI1("unshift-load-path", ficl_unshift_load_path, h_uns_load_path);
	FTH_PRI1("add-load-lib-path", ficl_add_load_lib_path,
	    h_add_load_lib_path);
	FTH_PRI1("unshift-load-lib-path", ficl_unshift_load_lib_path,
	    h_uns_load_lib_path);
	FTH_PRI1("include", ficl_include_file, h_include_file);
	FTH_PRI1("require", ficl_require_file, h_require_file);
	FTH_PRI1("load-init-file", ficl_load_init_file, h_load_init_file);
	FTH_PRI1("dl-load", ficl_dl_load, h_dl_load);
#if !defined(_WIN32)
	FTH_VOID_PROC("install-file", fth_install_file, 1, 0, 0,
	    h_install_file);
	FTH_VOID_PROC("install", fth_install, 0, 0, 0, h_install);
#endif
	FTH_PROC("apropos", fth_apropos, 1, 0, 0, h_apropos);
	FTH_PRI1("fth-catch", ficl_catch, h_fth_catch);
	FTH_PRI1("fth-throw", ficl_throw, h_fth_throw);
	FTH_PRI1("fth-raise", ficl_raise, h_fth_raise);
	FTH_PRI1("stack-reset", ficl_stack_reset, h_stack_reset);
#if defined(HAVE_GETTIMEOFDAY)
	FTH_PRI1("time", ficl_time, h_time);
	FTH_PRI1("time-reset", ficl_time_reset, h_time_reset);
#endif
	FTH_PRI1("utime", ficl_utime, h_utime);
	FTH_PRI1("current-time", ficl_current_time, h_current_time);
	FTH_PRI1("time->string", ficl_time_to_string, h_time_to_string);
	FTH_PRI1("strftime", ficl_strftime, h_strftime);
	FTH_PRI1("strptime", ficl_strptime, h_strptime);
	FTH_PRI1("localtime", ficl_localtime, h_localtime);
	FTH_PRI1("gmtime", ficl_gmtime, h_gmtime);
	FTH_PRI1("mktime", ficl_mktime, h_mktime);
	FTH_PRI1("getenv", ficl_getenv, h_getenv);
	FTH_PRI1("putenv", ficl_putenv, h_putenv);
	FTH_PRI1("environ", ficl_environ, h_environ);
	FTH_PRI1("getpid", ficl_getpid, h_getpid);
	FTH_PRI1("getppid", ficl_getppid, h_getppid);
	FTH_PRI1("getuid", ficl_getuid, h_getuid);
	FTH_PRI1("geteuid", ficl_geteuid, h_geteuid);
	FTH_PRI1("getgid", ficl_getgid, h_getgid);
	FTH_PRI1("getegid", ficl_getegid, h_getegid);
#if defined(HAVE_SETUID)
	FTH_PRI1("setuid", ficl_setuid, h_setuid);
#endif
#if defined(HAVE_SETEUID)
	FTH_PRI1("seteuid", ficl_seteuid, h_seteuid);
#endif
#if defined(HAVE_SETGID)
	FTH_PRI1("setgid", ficl_setgid, h_setgid);
#endif
#if defined(HAVE_SETEGID)
	FTH_PRI1("setegid", ficl_setegid, h_setegid);
#endif
	FTH_PRI1("signal", ficl_signal, h_signal);
#if defined(HAVE_KILL)
	FTH_PRI1("kill", ficl_kill, h_kill);
#endif
	FTH_PRI1("wait", ficl_wait, h_wait);
	FTH_PRI1("waitpid", ficl_waitpid, h_waitpid);
#if defined(HAVE_FORK)
	FTH_PRI1("fork", ficl_fork, h_fork);
#endif
#if defined(HAVE_EXECLP) && defined(HAVE_EXECVP)
	FTH_PRI1("exec", ficl_exec, h_exec);
#endif
	FTH_PRI1("getlogin", ficl_getlogin, h_getlogin);
	FTH_PRI1("hostname", ficl_gethostname, h_gethostname);
	FTH_PRI1("gethostname", ficl_gethostname, h_gethostname);
	FTH_PRI1("sethostname", ficl_sethostname, h_sethostname);
	FTH_PRI1("getservbyname", ficl_getservbyname, h_getservbyname);
	FTH_PRI1("getservbyport", ficl_getservbyport, h_getservbyport);
	FTH_PRI1("date", ficl_date, h_date);
#if defined(HAVE_SLEEP)
	FTH_PRI1("sleep", ficl_sleep, h_sleep);
#endif
	fth_define_variable("opterr", FTH_TRUE, h_opterr);
	fth_define_variable("optopt", FTH_FALSE, h_optopt);
	fth_define_variable("optind", FTH_ONE, h_optind);
	fth_define_variable("optarg", FTH_FALSE, h_optarg);
	FTH_PRI1("getopt", ficl_getopt, h_getopt);
	FTH_PRI1("getopt-long", ficl_getopt_long, h_getopt_long);
	FTH_PRI1(".memory", ficl_show_memory, h_show_memory);
	FTH_PRI1("ver", ficl_version, h_ficl_version);
	FTH_PRI1(".long-version", ficl_dot_lversion, h_dot_lversion);
	FTH_PRI1(".version", ficl_dot_version, h_dot_version);
	FTH_PRI1(".prefix", ficl_dot_prefix, h_dot_prefix);
	FTH_PRI1(".cflags", ficl_dot_cflags, h_dot_cflags);
	FTH_PRI1(".libs", ficl_dot_libs, h_dot_libs);
	FTH_PRI1("config-prefix", ficl_config_prefix, h_config_prefix);
	FTH_PRI1("config-cflags", ficl_config_cflags, h_config_cflags);
	FTH_PRI1("config-libs", ficl_config_libs, h_config_libs);
	FTH_PRI1("configure-args", ficl_configure_args, h_configure_args);
	FTH_PRI1("at-exit", ficl_at_exit, h_at_exit);
	FTH_PRI1("(bye)", ficl_exit, h_exit);
#if !defined(_WIN32)
	FTH_SET_CONSTANT(WNOHANG);
	FTH_SET_CONSTANT(WUNTRACED);
#endif
	FTH_SET_CONSTANT(SIG_DFL);
	FTH_SET_CONSTANT(SIG_IGN);
	FTH_SET_CONSTANT(SIG_ERR);
#if defined(SIGHUP)
	FTH_SET_CONSTANT(SIGHUP);	/* hangup */
#endif
#if defined(SIGINT)
	FTH_SET_CONSTANT(SIGINT);	/* interrupt */
#endif
#if defined(SIGQUIT)
	FTH_SET_CONSTANT(SIGQUIT);	/* quit */
#endif
#if defined(SIGILL)
	FTH_SET_CONSTANT(SIGILL);	/* illegal instr. */
#endif
#if defined(SIGTRAP)
	FTH_SET_CONSTANT(SIGTRAP);	/* trace trap */
#endif
#if defined(SIGABRT)
	FTH_SET_CONSTANT(SIGABRT);	/* abort */
#endif
#if defined(SIGIOT)
	FTH_SET_CONSTANT(SIGIOT);	/* compatibility (SIGABRT) */
#endif
#if defined(SIGEMT)
	FTH_SET_CONSTANT(SIGEMT);	/* EMT instruction */
#endif
#if defined(SIGFPE)
	FTH_SET_CONSTANT(SIGFPE);	/* floating point exception */
#endif
#if defined(SIGKILL)
	FTH_SET_CONSTANT(SIGKILL);	/* kill */
#endif
#if defined(SIGBUS)
	FTH_SET_CONSTANT(SIGBUS);	/* bus error */
#endif
#if defined(SIGSEGV)
	FTH_SET_CONSTANT(SIGSEGV);	/* segmentation violation */
#endif
#if defined(SIGSYS)
	FTH_SET_CONSTANT(SIGSYS);	/* non-existent system call invoked */
#endif
#if defined(SIGPIPE)
	FTH_SET_CONSTANT(SIGPIPE);	/* write on a pipe with no one to
					 * read it */
#endif
#if defined(SIGALRM)
	FTH_SET_CONSTANT(SIGALRM);	/* alarm clock */
#endif
#if defined(SIGTERM)
	FTH_SET_CONSTANT(SIGTERM);	/* software termination signal from
					 * kill */
#endif
#if defined(SIGURG)
	FTH_SET_CONSTANT(SIGURG);	/* urgent condition on IO channel */
#endif
#if defined(SIGSTOP)
	FTH_SET_CONSTANT(SIGSTOP);	/* sendable stop signal not from tty */
#endif
#if defined(SIGTSTP)
	FTH_SET_CONSTANT(SIGTSTP);	/* stop signal from tty */
#endif
#if defined(SIGCONT)
	FTH_SET_CONSTANT(SIGCONT);	/* continue a stopped process */
#endif
#if defined(SIGCHLD)
	FTH_SET_CONSTANT(SIGCHLD);	/* to parent on child stop or exit */
#endif
#if defined(SIGTTIN)
	FTH_SET_CONSTANT(SIGTTIN);	/* to readers pgrp upon background
					 * tty read */
#endif
#if defined(SIGTTOU)
	FTH_SET_CONSTANT(SIGTTOU);	/* like TTIN if
					 * (tp->t_local&LTOSTOP) */
#endif
#if defined(SIGIO)
	FTH_SET_CONSTANT(SIGIO);	/* input/output possible signal */
#endif
#if defined(SIGXCPU)
	FTH_SET_CONSTANT(SIGXCPU);	/* exceeded CPU time limit */
#endif
#if defined(SIGXFSZ)
	FTH_SET_CONSTANT(SIGXFSZ);	/* exceeded file size limit */
#endif
#if defined(SIGVTALRM)
	FTH_SET_CONSTANT(SIGVTALRM);	/* virtual time alarm */
#endif
#if defined(SIGPROF)
	FTH_SET_CONSTANT(SIGPROF);	/* profiling time alarm */
#endif
#if defined(SIGWINCH)
	FTH_SET_CONSTANT(SIGWINCH);	/* window size changes */
#endif
#if defined(SIGINFO)
	FTH_SET_CONSTANT(SIGINFO);	/* information request */
#endif
#if defined(SIGUSR1)
	FTH_SET_CONSTANT(SIGUSR1);	/* user defined signal 1 */
#endif
#if defined(SIGUSR2)
	FTH_SET_CONSTANT(SIGUSR2);	/* user defined signal 2 */
#endif
#if defined(SIGTHR)
	FTH_SET_CONSTANT(SIGTHR);	/* Thread interrupt. */
#endif
	/* >array, >assoc, >list, >hash */
	loop_begin = FICL_WORD_NAME_REF("begin");
	loop_until = FICL_WORD_NAME_REF("until");
	set_begin_paren = ficlDictionaryAppendPrimitive(dict,
	    "(set-begin)", ficl_set_begin_paren, FICL_WORD_DEFAULT);
	set_end_paren = ficlDictionaryAppendPrimitive(dict,
	    "(set-end)", ficl_set_end_paren, FICL_WORD_DEFAULT);
	FTH_PRI1("#(", ficl_begin_array, NULL);
	FTH_PRI1("array(", ficl_begin_array, NULL);
	FTH_PRI1("#a(", ficl_begin_assoc, NULL);
	FTH_PRI1("assoc(", ficl_begin_assoc, NULL);
	FTH_PRI1("'(", ficl_begin_list, NULL);
	FTH_PRI1("list(", ficl_begin_list, NULL);
	FTH_PRI1("'a(", ficl_begin_alist, NULL);
	FTH_PRI1("alist(", ficl_begin_alist, NULL);
	FTH_PRI1("#{", ficl_begin_hash, NULL);
	FTH_PRI1("hash{", ficl_begin_hash, NULL);
	FTH_PRI1(")", ficl_values_end, NULL);
	FTH_PRI1("}", ficl_values_end, NULL);
	/* each/map */
	FTH_PRI1("(set-each-loop)", ficl_set_each_loop_paren,
	    h_set_each_l_paren);
	FTH_PRI1("(set-map-loop)", ficl_set_each_loop_paren,
	    h_set_map_l_paren);
	FTH_PRI1("(set-map!-loop)", ficl_set_map_loop_paren,
	    h_set_map_l_paren);
	FTH_PRI1("(reset-each)", ficl_reset_each_paren, h_re_each_paren);
	FTH_PRI1("(reset-map)", ficl_reset_map_paren, h_re_map_paren);
	FTH_PRI1("(fetch)", ficl_fetch_paren, h_fetch_paren);
	FTH_PRI1("(store)", ficl_store_paren, h_store_paren);
	fth_require_file("fth.fs");
#if defined(HAVE_TZSET)
	tzset();
#endif
#if defined(HAVE_GETTIMEOFDAY)
	if (gettimeofday(&fth_timeval_tv, NULL) == -1)
		FTH_SYSTEM_ERROR_THROW(gettimeofday);
#endif
}

/*
 * misc.c ends here
 */
