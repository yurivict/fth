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
 */

#if !defined(lint)
const char fth_sccsid[] = "@(#)fth.c	2.1 1/2/18";
#endif /* not lint */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "fth.h"
#include "utils.h"
#include <getopt.h>

#define FTH_COPYRIGHT	"(c) 2004-2018 Michael Scholz"

static FTH 	eval_with_error_exit(void *, int);
static void 	repl_in_place(char *, FTH, ficlWord *, int, int, int);
static ficlWord *source_to_word(const char *);
static FTH 	string_split(char *, char *);

enum {
	REPL_COMPILE,
	REPL_INTERPRET
};

static ficlWord *
source_to_word(const char *buffer)
{
	ficlWord       *word;
	char           *bstr;
	int 		status;

	word = NULL;
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
	ficlVm         *vm;
	FTH 		val;
	ficlInteger 	depth, new_depth, i;
	int 		status;

	val = FTH_UNDEF;

	if (p == NULL)
		return (val);

	fth_eval_p = 1;
	vm = FTH_FICL_VM();
	depth = FTH_STACK_DEPTH(vm);

	switch (kind) {
	case REPL_COMPILE:
		status = fth_catch_exec((ficlWord *) p);
		break;
	case REPL_INTERPRET:
	default:
		status = fth_catch_eval((const char *) p);
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
		new_depth = FTH_STACK_DEPTH(vm) - depth;

		switch (new_depth) {
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
		break;
	}

	fth_eval_p = 0;
	return (val);
}

static FTH
string_split(char *str, char *delim)
{
	char           *p, *s, *t;
	FTH 		result;

	s = t = FTH_STRDUP(str);
	result = fth_make_empty_array();

	while ((p = strsep(&s, delim)) != NULL)
		if (*p != '\0')
			fth_array_push(result, fth_make_string(p));

	FTH_FREE(t);
	return (result);
}

static char 	fth_scratch[BUFSIZ];

static void
repl_in_place(char *in, FTH out, ficlWord *word, int auto_split_p, int print_p, int chomp_p)
{
	char           *delim, *buf;
	size_t 		len;
	ficlInteger 	line_no;
	FTH 		line;
	FILE           *ifp;

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
       fth -V\n"

extern char    *optarg;
extern int 	opterr;
extern int 	optind;
extern int 	optopt;

int
main(int argc, char **argv)
{
	int 		die, no_init_file, auto_split, debug;
	int 		in_place_p, ficl_repl;
	int 		line_end, implicit_loop, loop_print;
	int 		script_p, finish_getopt;
	int 		i, c, exit_value, stay_in_repl, verbose;
	int 		lp_len, llp_len, bufs_len, libs_len;
	char           *field_separator, *init_file, *suffix, *script;
	char           *buffers[LIBSLEN], *load_lib_paths[LIBSLEN];
	char           *libraries[LIBSLEN], *load_paths[LIBSLEN];
	FTH 		ret;

	/*
	 * environment variable POSIXLY_CORRECT: if set, disable permutation
	 * optstring starting with `-': RETURN_IN_ORDER optstring starting
	 * with `+': REQUIRE_ORDER (posix)
	 *
	 * optional arguments: append two colons x:: (see i:: in char *args)
	 */
	char           *args = "C:DE:F:I:QS:Vade:f:i::lnpqrs:v";

	/*
	 * Long options are gone with version 1.3.3 but --eval and
	 * --no-init-file remain for backwards compatibility for old fth.m4
	 * files.
	 */
	struct option 	opts[] = {
		{"eval", required_argument, NULL, 'e'},
		{"no-init-file", no_argument, NULL, 'Q'},
		{0, 0, 0, 0}
	};

	exit_value = EXIT_SUCCESS;
	llp_len = 0;		/* -C path */
	die = 0;		/* -D */
	/*
	 * stay_in_repl:	-1 not set
	 *			0 -e (eval)
	 *			0 -s (script)
	 *			1 -E (eval-and-stay)
	 *			1 -r (ficl-repl)
	 */
	stay_in_repl = -1;	/* -Er 1 || -es 0 */
	bufs_len = 0;		/* -Ee pattern */
	field_separator = NULL;	/* -F fs */
	lp_len = 0;		/* -I path */
	no_init_file = 0;	/* -Q */
	libs_len = 0;		/* -S path */
	auto_split = 0;		/* -a */
	debug = 0;		/* -d */
	init_file = NULL;	/* -f file */
	in_place_p = 0;		/* -i[suffix] */
	suffix = NULL;		/* -isuffix */
	line_end = 0;		/* -l */
	implicit_loop = 0;	/* -n || -p */
	loop_print = 0;		/* -n 0 || -p 1 */
	ficl_repl = 0;		/* -r */
	script_p = 0;		/* -s */
	finish_getopt = 0;	/* -s */
	script = NULL;		/* -s file */

	/*-
	 * verbose:		-1 not set --> true in interactive repl
	 *			0 -q quiet
	 *			1 -v verbose
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
			die = 1;
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
			no_init_file = 1;
			break;
		case 'S':	/* -S "LIB FUNC" */
			if (libs_len < LIBSLEN)
				libraries[libs_len++] = optarg;
			else
				fprintf(stderr, WARN_STR, c, optarg);
			break;
		case 'V':	/* -V */
			fprintf(stdout, "%s %s\n",
			    FTH_PACKAGE_NAME, fth_version());
			fprintf(stdout, "Copyright %s\n", FTH_COPYRIGHT);
			exit(EXIT_SUCCESS);
			break;
		case 'a':	/* -a */
			auto_split = 1;
			break;
		case 'd':	/* -d */
			debug = 1;
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
		case 'i':	/* -i [SUFFIX] */
			in_place_p = 1;
			if (optarg)
				suffix = optarg;
			break;
		case 'l':	/* -l */
			line_end = 1;
			break;
		case 'n':	/* -n */
			loop_print = 0;
			implicit_loop = 1;
			break;
		case 'p':	/* -p */
			loop_print = 1;
			implicit_loop = 1;
			break;
		case 'q':	/* -q */
			verbose = 0;
			break;
		case 'r':	/* -r */
			stay_in_repl = 1;
			ficl_repl = 1;
			break;
		case 's':	/* -s FILE */
			stay_in_repl = 0;
			script_p = 1;
			script = optarg;
			finish_getopt = 1;
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
		char           *lib, *name, *fnc;

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
		ficlWord       *word;
		char           *in_file, out_file[MAXPATHLEN];
		FTH 		out;

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
			char           *buf;

			buf = fth_scratch;

			while (fgets(buf, BUFSIZ, stdin) != NULL)
				eval_with_error_exit(buf, REPL_INTERPRET);

			fth_exit(EXIT_SUCCESS);
		} else {
			ret = fth_load_file(argv[i]);

			if (FTH_STRING_P(ret))
				fth_throw(FTH_LOAD_ERROR, "%S", ret);
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
			    FTH_PACKAGE_NAME, FTH_COPYRIGHT);
			fth_printf("\\ %s %s\n",
			    FTH_PACKAGE_NAME, fth_version());
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
