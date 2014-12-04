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
 */

#if !defined(lint)
const char fth_sccsid[] = "@(#)fth.c	1.106 12/3/14";
#endif /* not lint */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "fth.h"
#include "utils.h"
#include <getopt.h>

#define FTH_COPYRIGHT	"(c) 2004-2014 Michael Scholz"

static FTH	 eval_with_error_exit(void *p, int kind);
static void	 repl_in_place(char *in, FTH out, ficlWord *word,
		     bool auto_split_p, bool print_p, bool chomp_p);
static ficlWord *source_to_word(const char *buffer);
static FTH	 string_split(char *str, char *delim);

enum {
	REPL_COMPILE,
	REPL_INTERPRET
};

static ficlWord *
source_to_word(const char *buffer)
{
	ficlWord *word = NULL;
	char *bstr;
	int status;

	bstr = fth_format("lambda: ( ?? -- ?? ) %s ;", buffer);
	status = fth_catch_eval(bstr);
	FTH_FREE(bstr);
	switch (status) {
	case FTH_BYE:
		fth_exit(EXIT_SUCCESS);
		break;
	case FTH_ERROR:
		fth_exit(EXIT_FAILURE);
		break;
	default:
		word = ficlStackPopPointer(FTH_FICL_STACK());
		break;
	}
	return (word);
}

/*
 * Like fth_eval() in misc.c but exit(0) if 'bye' and exit(1) if error.
 */
static FTH
eval_with_error_exit(void *p, int kind)
{
	ficlVm *vm;
	FTH val;
	ficlInteger depth, i;
	int status;

	if (p == NULL)
		return (FTH_UNDEF);
	fth_eval_p = true;
	vm = FTH_FICL_VM();
	val = FTH_UNDEF;
	depth = FTH_STACK_DEPTH(vm);
	switch (kind) {
	case REPL_COMPILE:
		status = fth_catch_exec((ficlWord *)p);
		break;
	case REPL_INTERPRET:
	default:
		status = fth_catch_eval((const char *)p);
		break;
	}
	switch (status) {
	case FTH_BYE:
		fth_exit(EXIT_SUCCESS);
		break;
	case FTH_ERROR:
		if (fth_die_on_signal_p)
			fth_exit(EXIT_FAILURE);
		break;
	default:
		depth = FTH_STACK_DEPTH(vm) - depth;
		if (depth == 1)
			val = fth_pop_ficl_cell(vm);
		else if (depth > 1) {
			val = fth_make_array_len(depth);
			for (i = 0; i < depth; i++)
				val = fth_array_set(val, i,
				    fth_pop_ficl_cell(vm));
		}
		break;
	}
	fth_eval_p = false;
	return (val);
}

static FTH
string_split(char *str, char *delim)
{
	char *p, *s, *t;
	FTH result;

	s = t = FTH_STRDUP(str);
	result = fth_make_empty_array();
	while ((p = strsep(&s, delim)) != NULL)
		if (*p != '\0')
			fth_array_push(result, fth_make_string(p));
	FTH_FREE(t);
	return (result);
}

static char fth_scratch[BUFSIZ];

static void
repl_in_place(char *in, FTH out, ficlWord *word,
    bool auto_split_p, bool print_p, bool chomp_p)
{
	char *delim, *buf;
	size_t len;
	ficlInteger line_no;
	FTH line;
	FILE *ifp;

	ifp = stdin;
	if (in != NULL) {
		ifp = fopen(in, "r");
		if (ifp == NULL) {
			FTH_SYSTEM_ERROR_ARG_THROW(fopen, in);
			return;
		}
	}
	gc_push(FTH_FICL_VM()->runningWord);
	delim = fth_string_ref(fth_variable_ref("*fs*"));
	line_no = 0;
	buf = fth_scratch;
	while (fgets(buf, BUFSIZ, ifp) != NULL) {
		if (print_p)
			fth_print(buf);
		if (chomp_p) {
			len = fth_strlen(buf);
			if (buf[len - 1] == '\n')
				buf[len - 1] = '\0';
		}
		if (auto_split_p)
			fth_variable_set("*farray*", string_split(buf, delim));
		fth_variable_set("*line*", fth_make_string(buf));
		fth_variable_set("*fnr*", fth_make_int(line_no++));
		line = eval_with_error_exit(word, REPL_COMPILE);
		fth_variable_set("*nr*",
		    fth_number_add(fth_variable_ref("*nr*"), FTH_ONE));
		if (FTH_NOT_FALSE_P(out))
			fth_array_push(out, line);
		gc_loop_reset();
	}
	gc_pop();
	if (in != NULL)
		fclose(ifp);
}

#define LIBSLEN		48
#define WARN_STR	"#<warning: too much calls for -%c, ignoring \"%s\">\n"
#define FTH_USAGE	"\
usage: fth [-DdQqrv] [-C so-lib-path] [-Ee pattern] [-F fs] [-f init-file]\n\
           [-I fs-path] [-S \"lib init\"] [-s file] [file ...]\n\
       fth [-al] [-i [suffix]] [-n | -p] -e pattern [file | -]\n\
       fth [-h | -V]\n"

int
main(int argc, char **argv)
{
	bool die, no_init_file, auto_split, debug, in_place_p, ficl_repl;
	bool line_end, implicit_loop, loop_print, script_p, finish_getopt;
	int i, c, exit_value, stay_in_repl, verbose;
	int lp_len, llp_len, bufs_len, libs_len;
	char *field_separator, *init_file, *suffix, *script;
	char *buffers[LIBSLEN], *load_lib_paths[LIBSLEN];
	char *libraries[LIBSLEN], *load_paths[LIBSLEN];
	FTH ret;

	/*
	 * environment variable POSIXLY_CORRECT: if set, disable permutation
	 * optstring starting with `-': RETURN_IN_ORDER optstring starting
	 * with `+': REQUIRE_ORDER (posix)
	 * 
	 * optional arguments: append two colons x:: (see i:: in char *args)
	 */
	char *args = "C:DE:F:I:QS:Vade:f:hi::lnpqrs:v";

	/*
	 * Long options are gone with version 1.3.3 but --eval and
	 * --no-init-file remain for backwards compatibility for old fth.m4
	 * files.
	 */
	struct option opts[] = {
		{"eval", required_argument, NULL, 'e'},
		{"no-init-file", no_argument, NULL, 'Q'},
		{0, 0, 0, 0}
	};

	exit_value = EXIT_SUCCESS;
	llp_len = 0;		/* -C path */
	die = false;		/* -D */
	/*
	 * stay_in_repl: -1 not set 0 -e || -s 1 -E || -r
	 */
	stay_in_repl = -1;	/* -Er 1 || -es 0 */
	bufs_len = 0;		/* -Ee pattern */
	field_separator = NULL;	/* -F fs */
	lp_len = 0;		/* -I path */
	no_init_file = false;	/* -Q */
	libs_len = 0;		/* -S path */
	auto_split = false;	/* -a */
	debug = false;		/* -d */
	init_file = NULL;	/* -f file */
	in_place_p = false;	/* -i[suffix] */
	suffix = NULL;		/* -isuffix */
	line_end = false;	/* -l */
	implicit_loop = false;	/* -n || -p */
	loop_print = false;	/* -n 0 || -p 1 */
	ficl_repl = false;	/* -r */
	script_p = false;	/* -s */
	finish_getopt = false;	/* -s */
	script = NULL;		/* -s file */
	/*
	 * verbose: -1 not set --> true in interactive repl 0 -q quiet 1 -v
	 * verbose
	 */
	verbose = -1;		/* -v 1 || -q 0 */
	opterr = 1;		/* show getopt's error message */
	optind = 1;		/* initialize getopt */

	while (!finish_getopt &&
	    (c = getopt_long(argc, argv, args, opts, NULL)) != -1) {
		switch (c) {
		case 'C':	/* -C PATH */
			if (llp_len < LIBSLEN)
				load_lib_paths[llp_len++] = optarg;
			else
				fprintf(stderr, WARN_STR, c, optarg);
			break;
		case 'D':	/* -D */
			die = true;
			break;
		case 'E':	/* -E PATTERN */
			stay_in_repl = 1;
			if (bufs_len < LIBSLEN)
				buffers[bufs_len++] = optarg;
			else
				fprintf(stderr, WARN_STR, c, optarg);
			break;
		case 'F':	/* -F SEP */
			field_separator = optarg;
			break;
		case 'I':	/* -I PATH */
			if (lp_len < LIBSLEN)
				load_paths[lp_len++] = optarg;
			else
				fprintf(stderr, WARN_STR, c, optarg);
			break;
		case 'Q':	/* -Q */
			no_init_file = true;
			break;
		case 'S':	/* -S "LIB FUNC" */
			if (libs_len < LIBSLEN)
				libraries[libs_len++] = optarg;
			else
				fprintf(stderr, WARN_STR, c, optarg);
			break;
		case 'V':	/* -V */
			fprintf(stdout, "%s %s\n", PACKAGE_NAME, fth_version());
			fprintf(stdout, "Copyright %s\n", FTH_COPYRIGHT);
			exit(EXIT_SUCCESS);
			break;
		case 'a':	/* -a */
			auto_split = true;
			break;
		case 'd':	/* -d */
			debug = true;
			break;
		case 'e':	/* -e PATTERN */
			stay_in_repl = 0;
			if (bufs_len < LIBSLEN)
				buffers[bufs_len++] = optarg;
			else
				fprintf(stderr, WARN_STR, c, optarg);
			break;
		case 'f':	/* -f FILE */
			init_file = optarg;
			break;
		case 'h':	/* -h */
			fprintf(stdout, FTH_USAGE);
			exit(EXIT_SUCCESS);
			break;
		case 'i':	/* -i [SUFFIX] */
			in_place_p = true;
			if (optarg)
				suffix = optarg;
			break;
		case 'l':	/* -l */
			line_end = true;
			break;
		case 'n':	/* -n */
			loop_print = false;
			implicit_loop = true;
			break;
		case 'p':	/* -p */
			loop_print = true;
			implicit_loop = true;
			break;
		case 'q':	/* -q */
			verbose = 0;
			break;
		case 'r':	/* -r */
			stay_in_repl = 1;
			ficl_repl = true;
			break;
		case 's':	/* -s FILE */
			stay_in_repl = 0;
			script_p = true;
			script = optarg;
			finish_getopt = true;
			break;
		case 'v':	/* -v */
			verbose = 1;
			break;
		case '?':
		default:
			fprintf(stderr, FTH_USAGE);
			exit(EXIT_FAILURE);
			break;
		}
	}

	/*
	 * Start init forth.
	 */
	forth_init_before_load();
	fth_variable_set("*fth-verbose*", BOOL_TO_FTH(verbose > 0));
	fth_variable_set("*fth-debug*", BOOL_TO_FTH(debug));
	fth_die_on_signal_p = die;
	fth_true_repl_p = !ficl_repl;
	/*
	 * If -I or -C was given, we have to add these paths before finishing
	 * Forth init; calling 'make test' before 'make install' requires
	 * this for finding system scripts in the source tree instead in
	 * ${prefix}/share/fth.
	 */
	if (lp_len > 0)		/* -I PATH */
		for (i = 0; i < lp_len; i++)
			fth_unshift_load_path(load_paths[i]);

	if (llp_len > 0)	/* -C PATH */
		for (i = 0; i < llp_len; i++)
			fth_unshift_load_lib_path(load_lib_paths[i]);
	/*
	 * Finish init forth.
	 */
	forth_init();
	/*
	 * Adjust command line array.
	 */
	fth_set_argv(script_p ? optind - 1 : 0, argc, argv);
	argc -= optind;
	argv += optind;
	/*
	 * Reset getopt for further use in Forth scripts.
	 */
	optind = 1;
	if (field_separator != NULL)	/* -F SEP */
		fth_variable_set("*fs*", fth_make_string(field_separator));
	if (libs_len > 0) {	/* -S "LIB FUNC" */
		char *lib, *name, *fnc;

		for (i = 0; i < libs_len; i++) {
			lib = libraries[i];
			name = strsep(&lib, " \t");
			fnc = strsep(&lib, " \t");
			fth_dl_load(name, fnc);
		}
	}
	/*
	 * Run script and exit.
	 */
	if (script_p) {		/* -s FILE */
		ret = fth_load_file(script);
		if (FTH_STRING_P(ret)) {
			fth_throw(FTH_LOAD_ERROR, "%S", ret);
			fth_exit(EXIT_FAILURE);
		}
		fth_exit(EXIT_SUCCESS);
	}
	/*
	 * In-place or implicit-loop action and exit.
	 */
	if (in_place_p || implicit_loop) {	/* -inp */
		ficlWord *word;
		char *in_file, out_file[MAXPATHLEN];
		FTH out;

		if (bufs_len < 1) {
			fth_errorf("#<%s: in-place require -e PATTERN!>\n",
			    fth_exception_ref(FTH_FORTH_ERROR));
			fth_exit(EXIT_FAILURE);
			/*
			 * To silence ccc-analyzer (uninitialized
			 * 'buffer[i]' after for-loop below).
			 */
			/* NOTREACHED */
			return (EXIT_FAILURE);
		}
		/*
		 * If multiple -e, eval all but last.
		 */
		for (i = 0; i < bufs_len - 1; i++)
			eval_with_error_exit(buffers[i], REPL_INTERPRET);
		/*
		 * Last or only -e: compile and use it for in-place.
		 */
		word = source_to_word(buffers[i]);
		/*
		 * Read from stdin ...
		 */
		if (*argv == NULL) {
			repl_in_place(NULL, FTH_FALSE, word,
			    auto_split, loop_print, line_end);
			fth_exit(EXIT_SUCCESS);
		}
		/*
		 * ... or process all remaining files in order.
		 */
		for (i = 0; argv[i]; i++) {
			in_file = argv[i];
			fth_variable_set("*fname*", fth_make_string(in_file));
			if (in_place_p) {	/* -i [SUFFIX] */
				out = fth_make_empty_array();
				repl_in_place(in_file, out, word,
				    auto_split, loop_print, line_end);
				if (suffix != NULL) {	/* -i SUFFIX */
					fth_strcpy(out_file, sizeof(out_file),
					    in_file);
					fth_strcat(out_file, sizeof(out_file),
					    suffix);
					fth_file_rename(in_file, out_file);
				}
				fth_writelines(in_file, out);
			} else
				repl_in_place(in_file, FTH_FALSE, word,
				    auto_split, loop_print, line_end);
		}
		fth_exit(EXIT_SUCCESS);
	}
	/*
	 * Load remaining args as fth source files.
	 */
	for (i = 0; argv[i]; i++) {
		/* read words from stdin and exit */
		if (strcmp(argv[i], "-") == 0) {
			/*-
			 * % echo "80 .f2c cr" | fth -	==> 26.67
			 * 
			 * % cat foo
			 * 80 .f2c cr
			 * 
			 * % fth - < foo		==> 26.67
			 * 
			 * % fth -
			 * 80 .f2c cr <enter>		==> 26.67
			 * bye <enter>
			 * %
			 */
			char *buf;

			buf = fth_scratch;
			while (fgets(buf, BUFSIZ, stdin) != NULL)
				eval_with_error_exit(buf, REPL_INTERPRET);
			fth_exit(EXIT_SUCCESS);
		} else {
			ret = fth_load_file(argv[i]);
			if (FTH_STRING_P(ret)) {
				exit_value++;
				fth_throw(FTH_LOAD_ERROR, "%S", ret);
			}
		}
	}
	/*
	 * Adjust exit-value; if -D, exit.
	 */
	if (exit_value != EXIT_SUCCESS || fth_hit_error_p) {
		exit_value = EXIT_FAILURE;
		if (die)
			fth_exit(exit_value);
	}
	/*
	 * Eval strings from command line.
	 */
	if (bufs_len > 0) {	/* -Ee PATTERN */
		/*
		 * If multiple -e, eval all but last.
		 */
		for (i = 0; i < bufs_len - 1; i++)
			eval_with_error_exit(buffers[i], REPL_INTERPRET);
		/*
		 * Compile last or only -e.
		 */
		eval_with_error_exit(source_to_word(buffers[i]), REPL_COMPILE);
	}
	/*
	 * Adjust stay_in_repl if not set.
	 */
	if (stay_in_repl == -1)
		stay_in_repl = (argc == 0) ? 1 : 0;
	if (stay_in_repl > 0) {
		/*
		 * If not set, be verbose in repl (and while loading init
		 * files).
		 */
		if (verbose == -1)
			fth_variable_set("*fth-verbose*", FTH_TRUE);
		/*
		 * Print banner if we are still here.
		 */
		if (verbose != 0) {	/* not -q */
			fth_printf("\\ This is %s, %s\n",
			    PACKAGE_NAME, FTH_COPYRIGHT);
			fth_printf("\\ %s %s\n", PACKAGE_NAME, fth_version());
		}
		/*
		 * Load init files if not -Q.
		 */
		if (!no_init_file) {	/* -f FILE */
			ret = fth_load_global_init_file();
			if (FTH_STRING_P(ret)) {
				exit_value++;
				fth_throw(FTH_LOAD_ERROR, "%S", ret);
			}
			ret = fth_load_init_file(init_file);
			if (FTH_STRING_P(ret)) {
				exit_value++;
				fth_throw(FTH_LOAD_ERROR, "%S", ret);
			}
			if (fth_hit_error_p)
				exit_value = EXIT_FAILURE;
		}
	}
	if (!stay_in_repl || ((exit_value != EXIT_SUCCESS) && die))
		fth_exit(exit_value);
	/*
	 * Finally, start interactive mode.
	 */
	fth_repl(argc, argv);
	return (EXIT_SUCCESS);
}

/*
 * fth.c ends here
 */
