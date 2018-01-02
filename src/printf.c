/*-
 * tc.printf.c: A public-domain, minimal printf/sprintf routine that prints
 *	       through the putchar() routine.  Feel free to use for
 *	       anything...  -- 7/17/87 Paul Placeway
 */
/*-
 * Copyright (c) 1980, 1991 The Regents of the University of California.
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
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*-
 * Copyright (c) 2012-2018 Michael Scholz <mi-scholz@users.sourceforge.net>
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
 * @(#)printf.c	2.1 1/2/18
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "fth.h"
#include "utils.h"

#if defined(lint)
#undef va_arg
#define va_arg(a, b)		(a ? (b) 0 : (b) 0)
#endif

/*
 * limits.h
 *   INT_MAX
 *   CHAR_BIT
 */

#if !defined(INT_MAX)
#define INT_MAX			LONG_MAX
#endif

#if !defined(CHAR_BIT)
#define CHAR_BIT		8
#endif

/*
 * float.h
 *   DBL_MANT_DIG
 *   DBL_MAX_EXP
 *   DBL_MAX_10_EXP
 */
#if !defined(DBL_MAX_10_EXP)
#define DBL_MAX_10_EXP		308
#endif

#define DBL_BUF_LEN		(DBL_MAX_10_EXP + DBL_MANT_DIG + 3)

/* should be bigger than any field to print */
#define INF			INT_MAX

static char 	snil[] = "(nil)";

static void 	doprnt(void (*) (int), const char *, va_list);
static void 	xaddchar(int);
static int 	fth_basic_printf(void *, int, const char *);
static int 	fth_basic_vprintf(void *, int, const char *, va_list);
static FTH 	format_argument_error(const char *);
static void 	string_doprnt(void (*) (int), const char *, FTH);

static void
doprnt(void (*addchar) (int), const char *sfmt, va_list ap)
{
	char           *bp;
	const char     *f;

#if defined(HAVE_LONG_LONG)
	long long 	l;
	unsigned long long u;
#else
	long 		l;
	unsigned long 	u;
#endif
	FTH 		obj;
	char 		tb[DBL_BUF_LEN];
	ficlFloat 	d;
	char 		tf[7];
	int 		idx;
	size_t 		fs = DBL_BUF_LEN;

	/* Octal: 3 bits per char */
	char 		buf[(CHAR_BIT * sizeof(l) + 2) / 3 + 1];
	int 		i;
	int 		fmt;
	unsigned char 	pad = ' ';
	int 		flush_left = 0, f_width = 0, prec = INF, hash = 0;
	int 		do_long = 0, do_size_t = 0;
	int 		sign = 0;

	f = sfmt;
	for (; *f != '\0'; f++) {
		if (*f != '%') {
			/* then just out the char */
			(*addchar) (*f);
			continue;
		}
		f++;		/* skip the % */

		if (*f == '#') {/* alternate form */
			hash = 1;
			f++;
		}
		if (*f == '-') {/* minus: flush left */
			flush_left = 1;
			f++;
		}
		if (*f == '0') {/* || *f == '.') XXX [ms] */
			pad = '0';	/* padding with 0 rather than blank */
			f++;
		}
		if (*f == '*') {/* field width */
			f_width = va_arg(ap, int);
			f++;
		} else if (isdigit((unsigned char) *f)) {
			f_width = atoi(f);
			while (isdigit((unsigned char) *f))
				f++;	/* skip the digits */
		}
		if (*f == '.') {/* precision */
			f++;

			if (*f == '*') {
				prec = va_arg(ap, int);
				f++;
			} else if (isdigit((unsigned char) *f)) {
				prec = atoi(f);

				while (isdigit((unsigned char) *f))
					f++;	/* skip the digits */
			}
		}
		if (*f == 'l') {/* long format */
			do_long++;
			f++;

			if (*f == 'l') {
				do_long++;
				f++;
			}
		}
		if (*f == 'z') {/* size_t format */
			do_size_t++;
			f++;
		}
		fmt = (unsigned char) *f;
		bp = buf;

		switch (fmt) {	/* do the format */
		case 'c':
			i = va_arg(ap, int);
			f_width--;	/* adjust for one char [ms] */

			if (!flush_left)
				while (f_width-- > 0)
					(*addchar) (pad);

			(*addchar) (i);

			if (flush_left)
				while (f_width-- > 0)
					(*addchar) (' ');
			break;
		case 'd':
			switch (do_long) {
			case 0:
				if (do_size_t)
					l = (long) (va_arg(ap, size_t));
				else
					l = (long) (va_arg(ap, int));
				break;
			case 1:
#if !defined(HAVE_LONG_LONG)
			default:
#endif
				l = va_arg(ap, long);
				break;
#if defined(HAVE_LONG_LONG)
			default:
				l = va_arg(ap, long long);
				break;
#endif
			}

			if (l < 0) {
				sign = 1;
				l = -l;
			}

			do {
				*bp++ = (char) (l % 10 + '0');
			} while ((l /= 10) > 0);

			if (sign)
				*bp++ = '-';

			f_width = f_width - (int) (bp - buf);

			if (!flush_left)
				while (f_width-- > 0)
					(*addchar) (pad);

			for (bp--; bp >= buf; bp--)
				(*addchar) (*bp);

			if (flush_left)
				while (f_width-- > 0)
					(*addchar) (' ');
			break;
		case 'p':
			do_long = 1;
			hash = 1;
			fmt = 'x';
			/* FALLTHROUGH */
		case 'b':	/* [ms] %b added */
		case 'B':
		case 'o':
		case 'O':
		case 'x':
		case 'X':
		case 'u':
		case 'U':
			switch (do_long) {
			case 0:
				if (do_size_t)
					u = va_arg(ap, size_t);
				else
					u = va_arg(ap, unsigned int);
				break;
			case 1:
#if !defined(HAVE_LONG_LONG)
			default:
#endif
				u = va_arg(ap, unsigned long);
				break;
#if defined(HAVE_LONG_LONG)
			default:
				u = va_arg(ap, unsigned long long);
				break;
#endif
			}
			switch (fmt) {
			case 'u':	/* unsigned decimal */
			case 'U':
				do {
					*bp++ = (char) (u % 10 + '0');
				} while ((u /= 10) > 0);
				goto out_d;
				break;
			case 'o':	/* octal */
			case 'O':
				do {
					*bp++ = (char) (u % 8 + '0');
				} while ((u /= 8) > 0);

				if (hash)
					*bp++ = '0';

				goto out_d;
				break;
			case 'b':	/* binary added [ms] */
			case 'B':
				do {
					*bp++ = (char) (u % 2 + '0');
				} while ((u /= 2) > 0);

				if (hash) {
					*bp++ = (fmt == 'b') ? 'b' : 'B';
					*bp++ = '0';
				}
				goto out_d;
				break;
			case 'x':	/* hex */
			case 'X':
				do {
					char 		cn;

					cn = (fmt == 'x') ? 'a' : 'A';
					i = (int) (u % 16);

					if (i < 10)
						*bp++ = (char) (i + '0');
					else
						*bp++ = (char) (i - 10 + cn);
				} while ((u /= 16) > 0);

				if (hash) {
					*bp++ = (fmt == 'x') ? 'x' : 'X';
					*bp++ = '0';
				}
				goto out_d;
				break;
			}
out_d:
			f_width = f_width - (int) (bp - buf);

			if (!flush_left)
				while (f_width-- > 0)
					(*addchar) (pad);

			for (bp--; bp >= buf; bp--)
				(*addchar) (*bp);

			if (flush_left)
				while (f_width-- > 0)
					(*addchar) (' ');
			break;

			/* [ms] ficlFloat added */
		case 'A':
		case 'E':
		case 'F':
		case 'G':
		case 'a':
		case 'e':
		case 'f':
		case 'g':
			d = va_arg(ap, ficlFloat);
			idx = 0;
			tf[idx++] = '%';

			if (hash)
				tf[idx++] = '#';

			if (f_width == 0) {
				if (prec == INF) {
					tf[idx++] = *f;
					tf[idx] = '\0';
					snprintf(tb, fs, tf, d);
				} else {
					tf[idx++] = '.';
					tf[idx++] = '*';
					tf[idx++] = *f;
					tf[idx] = '\0';
					snprintf(tb, fs, tf, prec, d);
				}
			} else {
				if (prec == INF) {
					tf[idx++] = '*';
					tf[idx++] = *f;
					tf[idx] = '\0';
					snprintf(tb, fs, tf, f_width, d);
				} else {
					tf[idx++] = '*';
					tf[idx++] = '.';
					tf[idx++] = '*';
					tf[idx++] = *f;
					tf[idx] = '\0';
					snprintf(tb, fs, tf, f_width, prec, d);
				}
			}
			bp = tb;

			while (*bp != '\0')
				(*addchar) (*bp++);

			break;

			/*-
			 * FTH additions:
			 *
			 * %I -- fth_object_inspect
			 * %S -- fth_object_to_string
			 * %M -- fth_object_to_string_2
			 * %D -- fth_object_dump
			 */
		case 'I':
		case 'S':
		case 'M':
		case 'D':
			obj = va_arg(ap, FTH);

			switch (*f) {
			case 'I':
				bp = fth_to_c_inspect(obj);
				break;
			case 'S':
				bp = fth_to_c_string(obj);
				break;
			case 'M':
				bp = fth_to_c_string_2(obj);
				break;
			case 'D':
				bp = fth_to_c_dump(obj);
				break;
			}

			goto out_s;
			break;
		case 'Q':
		case 'q':
		case 's':
			bp = va_arg(ap, char *);
out_s:

			if (bp == NULL)
				bp = snil;

			if (prec == INF)
				f_width -= (int) strlen(bp);
			else
				f_width -= prec;

			if (!flush_left)
				while (f_width-- > 0)
					(*addchar) (pad);

			for (i = 0; *bp && i < prec; i++)
				(*addchar) ((unsigned char) *bp++);

			if (flush_left)
				while (f_width-- > 0)
					(*addchar) (' ');
			break;

		case '%':
			(*addchar) ('%');
			break;

		default:
			break;
		}		/* switch(fmt) */

		flush_left = 0;
		f_width = 0;
		prec = INF;
		hash = 0;
		do_size_t = 0;
		do_long = 0;
		sign = 0;
		pad = ' ';
	}			/* for */
}

static char    *xstring;
static char    *xestring;

static void
xaddchar(int c)
{
	if (xestring == xstring)
		*xstring = '\0';
	else
		*xstring++ = (char) c;
}

#if 0
static void
xputchar(int c)
{
	(void) fputc(c, stdout);
}

/*
 * From tcsh-src/tc.printf.c
 */
void
xsnprintf(char *str, size_t size, const char *fmt,...)
{
	va_list 	va;

	va_start(va, fmt);
	xstring = str;
	xestring = str + size - 1;
	doprnt(xaddchar, fmt, va);
	va_end(va);
	*xstring++ = '\0';
}

void
xprintf(const char *fmt,...)
{
	va_list 	va;

	va_start(va, fmt);
	doprnt(xputchar, fmt, va);
	va_end(va);
}

void
xvprintf(const char *fmt, va_list va)
{
	doprnt(xputchar, fmt, va);
}

void
xvsnprintf(char *str, size_t size, const char *fmt, va_list va)
{
	xstring = str;
	xestring = str + size - 1;
	doprnt(xaddchar, fmt, va);
	*xstring++ = '\0';
}

char           *
xvasprintf(const char *fmt, va_list va)
{
	size_t 		size;
	char           *buf;

	buf = NULL;
	size = 2048;		/* Arbitrary */

	for (;;) {
		va_list 	copy;

		buf = fth_realloc(buf, size);
		xstring = buf;
		xestring = buf + size - 1;
		va_copy(copy, va);
		doprnt(xaddchar, fmt, copy);
		va_end(copy);

		if (xstring < xestring)
			break;

		size *= 2;
	}

	*xstring++ = '\0';
	return (fth_realloc(buf, xstring - buf));
}

char           *
xasprintf(const char *fmt,...)
{
	va_list 	va;
	char           *ret;

	va_start(va, fmt);
	ret = xvasprintf(fmt, va);
	va_end(va);
	return (ret);
}

#endif				/* tcsh-src/tc.printf.c */

/* === FTH Printf Function Set === */

/*-
 * fth_printf and friends
 *
 * Extra options:
 *
 * %I -- fth_object_inspect
 * %S -- fth_object_to_string
 * %M -- fth_object_to_string_2
 * %D -- fth_object_dump
 * %b -- binary
 */

enum {
	PORT_FICLOUT,		/* ficlVm *vm */
	PORT_FICLERR,		/* ficlVm *vm */
	PORT_FILE,		/* FILE   *fp */
	PORT_IO			/* FTH     io */
};

/* defined in port.c */
extern out_cb 	fth_print_hook;
extern out_cb 	fth_error_hook;

static int
fth_basic_printf(void *port, int type, const char *sstr)
{
	char           *str;
	int 		len;

	str = (char *) sstr;
	len = (int) fth_strlen(str);

	if (len <= 0)
		return (0);

	switch (type) {
	case PORT_FICLOUT:
		fth_print_p = 1;
		(*fth_print_hook) ((ficlVm *) port, str);
		break;
	case PORT_FICLERR:
		fth_print_p = 1;
		(*fth_error_hook) ((ficlVm *) port, str);
		break;
	case PORT_FILE:
		len = fputs(str, (FILE *) port);
		fflush(port);
		break;
	case PORT_IO:
	default:
		fth_io_write_and_flush((FTH) port, str);
		break;
	}

	return (len);
}

static int
fth_basic_vprintf(void *port, int type, const char *fmt, va_list ap)
{
	char           *str;
	int 		len;

	str = fth_vformat(fmt, ap);
	len = fth_basic_printf(port, type, str);
	FTH_FREE(str);
	return (len);
}

/*
 * Writes to Ficl output!
 */
int
fth_print(const char *str)
{
	return (fth_basic_printf(FTH_FICL_VM(), PORT_FICLOUT, str));
}

/*
 * Writes to Ficl error output!
 */
int
fth_error(const char *str)
{
	return (fth_basic_printf(FTH_FICL_VM(), PORT_FICLERR, str));
}

/*
 * Writes to Ficl output!
 * Use fth_fprintf(stdout, ...) for explicit stdout.
 */
int
fth_printf(const char *fmt,...)
{
	int 		len;
	va_list 	ap;

	va_start(ap, fmt);
	len = fth_vprintf(fmt, ap);
	va_end(ap);
	return (len);
}

/*-
 * Writes to Ficl output!
 * Use fth_vfprintf(stdout, ...) for explicit stdout.
 */
int
fth_vprintf(const char *fmt, va_list ap)
{
	return (fth_basic_vprintf(FTH_FICL_VM(), PORT_FICLOUT, fmt, ap));
}

/*-
 * Writes to Ficl error output!
 * Use fth_fprintf(stderr, ...) for explicit stderr.
 */
int
fth_errorf(const char *fmt,...)
{
	int 		len;
	va_list 	ap;

	va_start(ap, fmt);
	len = fth_verrorf(fmt, ap);
	va_end(ap);
	return (len);
}

/*-
 * Writes to Ficl error output!
 * Use fth_vfprintf(stderr, ...) for explicit stderr.
 */
int
fth_verrorf(const char *fmt, va_list ap)
{
	return (fth_basic_vprintf(FTH_FICL_VM(), PORT_FICLERR, fmt, ap));
}

/*
 * Writes to FILE pointer.
 */
int
fth_fprintf(FILE *fp, const char *fmt,...)
{
	int 		len;
	va_list 	ap;

	va_start(ap, fmt);
	len = fth_vfprintf(fp, fmt, ap);
	va_end(ap);
	return (len);
}

/*
 * Writes to FILE pointer.
 */
int
fth_vfprintf(FILE *fp, const char *fmt, va_list ap)
{
	return (fth_basic_vprintf(fp, PORT_FILE, fmt, ap));
}

/*
 * Writes to IO object (io.c, port.c).
 */
int
fth_ioprintf(FTH io, const char *fmt,...)
{
	int 		len;
	va_list 	ap;

	va_start(ap, fmt);
	len = fth_vioprintf(io, fmt, ap);
	va_end(ap);
	return (len);
}

/*
 * Writes to IO object (io.c, port.c).
 */
int
fth_vioprintf(FTH io, const char *fmt, va_list ap)
{
	return (fth_basic_vprintf((void *) io, PORT_IO, fmt, ap));
}

/*-
 * Writes to PORT object (port.c).
 * PORT can be #f for Ficl stdout.
 */
int
fth_port_printf(FTH port, const char *fmt,...)
{
	int 		len;
	va_list 	ap;

	va_start(ap, fmt);
	len = fth_port_vprintf(port, fmt, ap);
	va_end(ap);
	return (len);
}

/*-
 * Writes to PORT object (port.c).
 * PORT can be #f for Ficl stdout.
 */
int
fth_port_vprintf(FTH port, const char *fmt, va_list ap)
{
	int 		len;

	if (FTH_FALSE_P(port)) {
		len = fth_basic_vprintf(FTH_FICL_VM(), PORT_FICLOUT, fmt, ap);
		return (len);
	}
	if (FTH_IO_P(port)) {
		len = fth_basic_vprintf((void *) port, PORT_IO, fmt, ap);
		return (len);
	}
	FTH_ASSERT_ARGS(0, port, FTH_ARG1, "an io or #f");
	return (-1);
}

/*
 * Returned string must be freed!
 */
char           *
fth_format(const char *fmt,...)
{
	char           *str;
	va_list 	ap;

	va_start(ap, fmt);
	str = fth_vformat(fmt, ap);
	va_end(ap);
	return (str);
}

/*
 * Returned string must be freed!
 */
char           *
fth_vformat(const char *fmt, va_list ap)
{
	char           *prev_xstring, *prev_xestring, *buf;
	size_t 		size;

	prev_xstring = xstring;
	prev_xestring = xestring;
	size = 2048;		/* arbitrary */
	buf = NULL;

	for (;;) {
		va_list 	copy;

		buf = FTH_REALLOC(buf, size);
		xstring = buf;
		xestring = buf + size - 1;
		va_copy(copy, ap);
		doprnt(xaddchar, fmt, copy);
		va_end(copy);

		if (xstring < xestring)
			break;

		size *= 2;
	}

	*xstring++ = '\0';
	size = (size_t) (xstring - buf);
	xstring = prev_xstring;
	xestring = prev_xestring;
	return (FTH_REALLOC(buf, size));
}

/*
 * Buffer must be big enough to hold data!
 */
int
fth_sprintf(char *buffer, const char *fmt,...)
{
	int 		len;
	va_list 	ap;

	va_start(ap, fmt);
	len = fth_vsprintf(buffer, fmt, ap);
	va_end(ap);
	return (len);
}

/*
 * Buffer must be big enough to hold data!
 */
int
fth_vsprintf(char *buffer, const char *fmt, va_list ap)
{
	if (buffer == NULL)
		return (-1);
	return (fth_vsnprintf(buffer, sizeof(buffer), fmt, ap));
}

/*
 * Writes at most size - 1 characters to buffer.
 */
int
fth_snprintf(char *buffer, size_t size, const char *fmt,...)
{
	int 		len;
	va_list 	ap;

	va_start(ap, fmt);
	len = fth_vsnprintf(buffer, size, fmt, ap);
	va_end(ap);
	return (len);
}

/*
 * Writes at most size - 1 characters to buffer.
 */
int
fth_vsnprintf(char *str, size_t size, const char *fmt, va_list ap)
{
	char           *prev_xstring, *prev_xestring;
	int 		len;

	prev_xstring = xstring;
	prev_xestring = xestring;
	xstring = str;
	xestring = str + size - 1;
	doprnt(xaddchar, fmt, ap);
	*xstring++ = '\0';
	len = (int) (xstring - str);
	xstring = prev_xstring;
	xestring = prev_xestring;
	return (len);
}

/*
 * Returned string must be freed!
 */
int
fth_asprintf(char **result, const char *fmt,...)
{
	int 		len;
	va_list 	ap;

	va_start(ap, fmt);
	len = fth_vasprintf(result, fmt, ap);
	va_end(ap);
	return (len);
}

/*
 * Returned string must be freed!
 */
int
fth_vasprintf(char **result, const char *fmt, va_list ap)
{
	*result = fth_vformat(fmt, ap);
	return ((int) fth_strlen(*result));
}

/*
 * Prints message wrapped in #<warning: ...> to Ficl error output.
 */
int
fth_warning(const char *fmt,...)
{
	int 		len;
	va_list 	ap;

	len = fth_errorf("#<warning: ");
	va_start(ap, fmt);
	len += fth_verrorf(fmt, ap);
	va_end(ap);
	len += fth_errorf(">\n");
	return (len);
}

/*
 * Print message wrapped in #<DEBUG(C): ...> to stderr.
 */
int
fth_debug(const char *fmt,...)
{
	va_list 	ap;
	int 		len;

	len = fth_fprintf(stderr, "#<DEBUG(C): ");
	va_start(ap, fmt);
	len += fth_vfprintf(stderr, fmt, ap);
	va_end(ap);
	len += fth_fprintf(stderr, ">\n");
	return (len);
}

/*
 * Doprnt for Fth source side with extra options %p, %s, %S, %m, %b.
 */

static FTH
format_argument_error(const char *format)
{
	fth_throw(FTH_ARGUMENT_ERROR,
	    "%s: too few arguments for format string \"%s\"",
	    RUNNING_WORD(), format);
	return (FTH_FALSE);
}

#define FTH_LONG_REF(X)							\
	(FTH_FIXNUM_P(X) ?						\
	(ficl2Integer)FIX_TO_INT(X) :					\
	(FTH_LLONG_P(X) ? FTH_LONG_OBJECT(X) : (ficl2Integer)(X)))

#define FTH_ULONG_REF(X)						\
	(FTH_FIXNUM_P(X) ?						\
	(ficl2Unsigned)FIX_TO_UNSIGNED(X) :				\
	(FTH_ULLONG_P(X) ? FTH_ULONG_OBJECT(X) : (ficl2Unsigned)(X)))

#define FTH_FLOAT_REF(X)						\
	(((X) && FTH_FLOAT_T_P(X)) ? FTH_FLOAT_OBJECT(X) : (ficlFloat)(X))

static void
string_doprnt(void (*addchar)(int), const char *sfmt, FTH ap)
{
	char           *bp;
	const char     *f;
	ficl2Integer 	l;
	ficl2Unsigned 	u;

	/* Octal: 3 bits per char */
	char 		buf[(CHAR_BIT * sizeof(l) + 2) / 3 + 1];
	ficlInteger 	i;
	int 		fmt;
	unsigned char 	pad = ' ';
	int 		flush_left = 0, f_width = 0, prec = INF, hash = 0;
	int 		sign = 0;
	ficlInteger 	index = 0, len = fth_array_length(ap);
	FTH 		val;
	ficlFloat 	d;
	char 		tb[DBL_BUF_LEN];
	char 		tf[7];
	int 		idx;
	size_t 		fs = DBL_BUF_LEN;

#define VA_ARG()							\
	((index < len) ?						\
	    fth_array_ref(ap, index++) : format_argument_error(sfmt))

	f = sfmt;

	for (; *f != '\0'; f++) {
		if (*f != '%') {
			/* then just out the char */
			(*addchar) (*f);
			continue;
		}
		f++;		/* skip the % */

		if (*f == '#') {/* alternate form */
			hash = 1;
			f++;
		}
		if (*f == '-') {/* minus: flush left */
			flush_left = 1;
			f++;
		}
		if (*f == '0') {/* || *f == '.') XXX [ms] */
			pad = '0';	/* padding with 0 rather than blank */
			f++;
		}
		if (*f == '*') {/* field width */
			val = VA_ARG();
			f_width = (int) FTH_INT_REF(val);
			f++;
		} else if (isdigit((unsigned char) *f)) {
			f_width = atoi(f);

			while (isdigit((unsigned char) *f))
				f++;	/* skip the digits */
		}
		if (*f == '.') {/* precision */
			f++;

			if (*f == '*') {
				val = VA_ARG();
				prec = (int) FTH_INT_REF(val);
				f++;
			} else if (isdigit((unsigned char) *f)) {
				prec = atoi(f);
				while (isdigit((unsigned char) *f))
					f++;	/* skip the digits */
			}
		}
		if (*f == 'l') {/* long format */
			/* skip it */
			f++;

			if (*f == 'l') {
				/* skip it */
				f++;
			}
		}
		if (*f == 'z') {/* size_t format */
			/* skip it */
			f++;
		}
		fmt = (unsigned char) *f;
		bp = buf;

		switch (fmt) {	/* do the format */
		case 'c':
			val = VA_ARG();
			i = FTH_INT_REF(val);
			f_width--;	/* adjust for one char [ms] */

			if (!flush_left)
				while (f_width-- > 0)
					(*addchar) (pad);

			(*addchar) ((int) i);

			if (flush_left)
				while (f_width-- > 0)
					(*addchar) (' ');
			break;
		case 'd':
			val = VA_ARG();
			l = FTH_LONG_REF(val);

			if (l < 0) {
				sign = 1;
				l = -l;
			}

			do {
				*bp++ = (char) (l % 10 + '0');
			} while ((l /= 10) > 0);

			if (sign)
				*bp++ = '-';

			f_width = f_width - (int) (bp - buf);

			if (!flush_left)
				while (f_width-- > 0)
					(*addchar) (pad);

			for (bp--; bp >= buf; bp--)
				(*addchar) (*bp);

			if (flush_left)
				while (f_width-- > 0)
					(*addchar) (' ');
			break;
		case 'b':	/* [ms] %b added */
		case 'B':
		case 'o':
		case 'O':
		case 'x':
		case 'X':
		case 'u':
		case 'U':
			val = VA_ARG();
			u = FTH_ULONG_REF(val);

			switch (fmt) {
			case 'u':	/* unsigned decimal */
			case 'U':
				do {
					*bp++ = (char) (u % 10 + '0');
				} while ((u /= 10) > 0);

				goto out_d;
				break;
			case 'o':	/* octal */
			case 'O':
				do {
					*bp++ = (char) (u % 8 + '0');
				} while ((u /= 8) > 0);

				if (hash)
					*bp++ = '0';

				goto out_d;
				break;
			case 'b':	/* binary added [ms] */
			case 'B':
				do {
					*bp++ = (char) (u % 2 + '0');
				} while ((u /= 2) > 0);

				if (hash) {
					*bp++ = (fmt == 'b') ? 'b' : 'B';
					*bp++ = '0';
				}
				goto out_d;
				break;
			case 'x':	/* hex */
			case 'X':
				do {
					char 		cn;

					cn = (fmt == 'x') ? 'a' : 'A';
					i = (int) (u % 16);

					if (i < 10)
						*bp++ = (char) (i + '0');
					else
						*bp++ = (char) (i - 10 + cn);
				} while ((u /= 16) > 0);

				if (hash) {
					*bp++ = (fmt == 'x') ? 'x' : 'X';
					*bp++ = '0';
				}
				goto out_d;
				break;
			}
out_d:
			f_width = f_width - (int) (bp - buf);

			if (!flush_left)
				while (f_width-- > 0)
					(*addchar) (pad);

			for (bp--; bp >= buf; bp--)
				(*addchar) (*bp);

			if (flush_left)
				while (f_width-- > 0)
					(*addchar) (' ');
			break;
			/* [ms] ficlFloat added */
		case 'f':
		case 'F':
		case 'e':
		case 'E':
		case 'g':
		case 'G':
		case 'a':
		case 'A':
			val = VA_ARG();
			d = FTH_FLOAT_REF(val);
			idx = 0;
			tf[idx++] = '%';

			if (hash)
				tf[idx++] = '#';

			if (f_width == 0) {
				if (prec == INF) {
					tf[idx++] = *f;
					tf[idx] = '\0';
					snprintf(tb, fs, tf, d);
				} else {
					tf[idx++] = '.';
					tf[idx++] = '*';
					tf[idx++] = *f;
					tf[idx] = '\0';
					snprintf(tb, fs, tf, prec, d);
				}
			} else {
				if (prec == INF) {
					tf[idx++] = '*';
					tf[idx++] = *f;
					tf[idx] = '\0';
					snprintf(tb, fs, tf, f_width, d);
				} else {
					tf[idx++] = '*';
					tf[idx++] = '.';
					tf[idx++] = '*';
					tf[idx++] = *f;
					tf[idx] = '\0';
					snprintf(tb, fs, tf, f_width, prec, d);
				}
			}
			bp = tb;

			while (*bp != '\0')
				(*addchar) (*bp++);
			break;
			/*-
			 * FTH additions:
			 *
			 * %p -- object-inspect
			 * %s -- object->string
			 * %S -- object-dump
			 * %m -- object-dump (old style)
			 */
		case 'p':
		case 's':
		case 'S':
		case 'm':
			val = VA_ARG();

			switch (*f) {
			case 'p':
				bp = fth_to_c_inspect(val);
				break;
			case 's':
				bp = fth_to_c_string(val);
				break;
			case 'S':
			case 'm':
			default:
				bp = fth_to_c_dump(val);
				break;
			}

			if (!bp)
				bp = snil;

			if (prec == INF)
				f_width -= (int) strlen(bp);
			else
				f_width -= prec;

			if (!flush_left)
				while (f_width-- > 0)
					(*addchar) (pad);

			for (i = 0; *bp && i < prec; i++)
				(*addchar) (*bp++);

			if (flush_left)
				while (f_width-- > 0)
					(*addchar) (' ');
			break;
		case '%':
			(*addchar) ('%');
			break;
		default:
			break;
		}		/* switch(fmt) */

		flush_left = 0;
		f_width = 0;
		prec = INF;
		hash = 0;
		sign = 0;
		pad = ' ';
	}			/* for */
}

/*
 * Return formatted Fth string corresponding to C string 'fmt' and
 * Fth array 'args' containing as much arguments as 'fmt' requires.
 *
 * FTH fs, args;
 * args = fth_make_array_var(2, fth_make_int(10), fth_make_float(3.14));
 * fs = fth_string_vformat("print %d times %f\n", args);
 */
FTH
fth_string_vformat(const char *fmt, FTH args)
{
	char           *prev_xstring, *prev_xestring, *buf;
	size_t 		size;
	FTH 		fs;

	prev_xstring = xstring;
	prev_xestring = xestring;
	size = 2048;		/* arbitrary */
	buf = NULL;

	for (;;) {
		buf = FTH_REALLOC(buf, size);
		xstring = buf;
		xestring = buf + size - 1;
		string_doprnt(xaddchar, fmt, args);

		if (xstring < xestring)
			break;

		size *= 2;
	}

	buf[xstring - buf] = '\0';
	fs = fth_make_string(buf);
	FTH_FREE(buf);
	xstring = prev_xstring;
	xestring = prev_xestring;
	return (fs);
}

/*
 * printf.c ends here
 */
