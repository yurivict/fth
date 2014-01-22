/*-
 * Copyright (c) 2005-2013 Michael Scholz <mi-scholz@users.sourceforge.net>
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
 * @(#)io.c	1.154 9/13/13
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "fth.h"
#include "utils.h"

#if !defined(WEXITSTATUS)
#define WEXITSTATUS(stat_val)	((unsigned)(stat_val) >> 8)
#endif
#if !defined(WIFEXITED)
#define WIFEXITED(stat_val)	(((stat_val) & 0xff) == 0)
#endif

/* === IO === */

static FTH	io_tag;

/* --- General IO Macros --- */

#define IO_NOT_CLOSED_P(Obj)	(FTH_IO_P(Obj) && !FTH_IO_CLOSED_P(Obj))
#define IO_INPUT_P(Obj)		(IO_NOT_CLOSED_P(Obj) && FTH_IO_INPUT_P(Obj))
#define IO_OUTPUT_P(Obj)	(IO_NOT_CLOSED_P(Obj) && FTH_IO_OUTPUT_P(Obj))

#define IO_FILE_ERROR(Func)						\
	FTH_ANY_ERROR_THROW(FTH_FILE_IO_ERROR, Func)
#define IO_FILE_ERROR_ARG(Func, Desc)					\
	FTH_ANY_ERROR_ARG_THROW(FTH_FILE_IO_ERROR, Func, Desc)

#define IO_SOCKET_ERROR(Func)	FTH_ANY_ERROR_THROW(FTH_SOCKET_ERROR, Func)
#define IO_SOCKET_ERROR_ARG(Func, Desc)					\
	FTH_ANY_ERROR_ARG_THROW(FTH_SOCKET_ERROR, Func, Desc)

/* --- STRING IO --- */

#define FTH_IO_STRING_LENGTH(Obj)					\
	fth_string_length((FTH)(Obj))
#define FTH_IO_STRING_INDEX_REF(Obj)					\
	FTH_INSTANCE_REF(Obj)->cycle
#define FTH_IO_STRING_INDEX_SET(Obj, Idx)				\
	FTH_INSTANCE_REF(Obj)->cycle = (Idx)

#define IO_STRING_REF(line)						\
	((fth_string_length(line) > 0) ? fth_string_ref(line) : "")

#define IO_ASSERT_ARGS(Cond, Arg, Str)					\
	FTH_ASSERT_ARGS(Cond, Arg, FTH_ARG1, Str)
#define IO_ASSERT_IO(Io)						\
	IO_ASSERT_ARGS(FTH_IO_P(Io), Io, "an io")
#define IO_ASSERT_IO_INPUT(Io)						\
	IO_ASSERT_ARGS(IO_INPUT_P(Io), Io, "an open input io")
#define IO_ASSERT_IO_OUTPUT(Io)						\
	IO_ASSERT_ARGS(IO_OUTPUT_P(Io), Io, "an open output io")
#define IO_ASSERT_IO_NOT_CLOSED(Io)					\
	IO_ASSERT_ARGS(IO_NOT_CLOSED_P(Io), Io, "an open io")

#define IO_ASSERT_STRING(Str, Arg)					\
	FTH_ASSERT_ARGS(fth_string_length(Str) > 0, (Str), (Arg), "a string")

/* --- SOCKET IO --- */

typedef struct {
	int		fd;	/* file descriptor */
	FTH		host;	/* host name */
} FIO_Socket;

#define SOCKET_REF(Obj)		((FIO_Socket *)(Obj))
#define FTH_IO_SOCKET_FD(Obj)	SOCKET_REF(Obj)->fd
#define FTH_IO_SOCKET_HOST(Obj)	SOCKET_REF(Obj)->host

static char    *fam_to_mode(int fam);
static void	ficl_accept(ficlVm *vm);
static void	ficl_bind(ficlVm *vm);
static void	ficl_connect(ficlVm *vm);
static void	ficl_exit_status(ficlVm *vm);
static void	ficl_io_close(ficlVm *vm);
static void	ficl_io_closed_p(ficlVm *vm);
static void	ficl_io_eof_p(ficlVm *vm);
static void	ficl_io_equal_p(ficlVm *vm);
static void	ficl_io_fdopen(ficlVm *vm);
static void	ficl_io_filename(ficlVm *vm);
static void	ficl_io_fileno(ficlVm *vm);
static void	ficl_io_flush(ficlVm *vm);
static void	ficl_io_getc(ficlVm *vm);
static void	ficl_io_input_p(ficlVm *vm);
static void	ficl_io_mode(ficlVm *vm);
static void	ficl_io_nopen(ficlVm *vm);
static void	ficl_io_open(ficlVm *vm);
static void	ficl_io_open_file(ficlVm *vm);
static void	ficl_io_open_input_file(ficlVm *vm);
static void	ficl_io_open_output_file(ficlVm *vm);
static void	ficl_io_open_read(ficlVm *vm);
static void	ficl_io_open_write(ficlVm *vm);
static void	ficl_io_output_p(ficlVm *vm);
static void	ficl_io_p(ficlVm *vm);
static void	ficl_io_popen(ficlVm *vm);
static void	ficl_io_popen_read(ficlVm *vm);
static void	ficl_io_popen_write(ficlVm *vm);
static void	ficl_io_pos_ref(ficlVm *vm);
static void	ficl_io_pos_set(ficlVm *vm);
static void	ficl_io_putc(ficlVm *vm);
static void	ficl_io_reopen(ficlVm *vm);
static void	ficl_io_rewind(ficlVm *vm);
static void	ficl_io_seek(ficlVm *vm);
static void	ficl_io_sopen(ficlVm *vm);
static void	ficl_io_sopen_read(ficlVm *vm);
static void	ficl_io_sopen_write(ficlVm *vm);
static void	ficl_io_stderr(ficlVm *vm);
static void	ficl_io_stdin(ficlVm *vm);
static void	ficl_io_stdout(ficlVm *vm);
static void	ficl_io_write(ficlVm *vm);
static void	ficl_io_write_format(ficlVm *vm);
static void	ficl_listen(ficlVm *vm);
static void	ficl_print_io(ficlVm *vm);
static void	ficl_readlines(ficlVm *vm);
static void	ficl_recv(ficlVm *vm);
static void	ficl_recvfrom(ficlVm *vm);
static void	ficl_send(ficlVm *vm);
static void	ficl_sendto(ficlVm *vm);
static void	ficl_set_io_stderr(ficlVm *vm);
static void	ficl_set_io_stdin(ficlVm *vm);
static void	ficl_set_io_stdout(ficlVm *vm);
static void	ficl_set_version_control(ficlVm *vm);
static void	ficl_shutdown(ficlVm *vm);
static void	ficl_socket(ficlVm *vm);
static void	ficl_version_control(ficlVm *vm);
static void	ficl_writelines(ficlVm *vm);
static void	file_close(void *ptr);
static bool	file_eof_p(void *ptr);
static void	file_flush(void *ptr);
static int	file_number(char *name);
static int	file_read_char(void *ptr);
static char    *file_read_line(void *ptr);
static void	file_rewind(void *ptr);
static ficl2Integer file_seek(void *ptr, ficl2Integer pos, int whence);
static ficl2Integer file_tell(void *ptr);
static void	file_version_rename(char *name);
static void	file_write_char(void *ptr, int c);
static void	file_write_line(void *ptr, const char *line);
static void	generic_close(void *ptr);
static bool	generic_eof_p(void *ptr);
static void	generic_flush(void *ptr);
static int	generic_read_char(void *ptr);
static char    *generic_read_line(void *ptr);
static void	generic_rewind(void *ptr);
static ficl2Integer generic_seek(void *ptr, ficl2Integer pos, int whence);
static ficl2Integer generic_tell(void *ptr);
static void	generic_write_char(void *ptr, int c);
static void	generic_write_line(void *ptr, const char *line);
static FTH	io_equal_p(FTH self, FTH obj);
static void	io_free(FTH self);
static void	io_if_exists(char *name, FTH exists, int fam);
static FTH	io_inspect(FTH self);
static FTH	io_length(FTH self);
static void	io_mark(FTH self);
static FTH	io_ref(FTH self, FTH idx);
static FTH	io_to_array(FTH self);
static FTH	io_to_string(FTH self);
static FTH	make_file_io(FILE * fp, const char *name, int fam);
static FTH	make_socket_io(const char *host, int port, int domain, int fd);
static void	pipe_close(void *ptr);
static bool	seek_constant_p(int whence);
static int	socket_accept(int domain, int fd);
static int	socket_bind(const char *host, int port, int domain, int fd);
static void	socket_close(void *ptr);
static int	socket_connect(const char *host, int port, int domain, int fd);
static bool	socket_eof_p(void *ptr);
static int	socket_open(int domain, int type);
static int	socket_read_char(void *ptr);
static char    *socket_read_line(void *ptr);
static void	socket_rewind(void *ptr);
static ficl2Integer socket_seek(void *ptr, ficl2Integer pos, int whence);
static ficl2Integer socket_tell(void *ptr);
static void	socket_unlink(const char *host);
static void	socket_write_char(void *ptr, int c);
static void	socket_write_line(void *ptr, const char *line);
static bool	string_eof_p(void *ptr);
static int	string_read_char(void *ptr);
static char    *string_read_line(void *ptr);
static void	string_rewind_and_close(void *ptr);
static ficl2Integer string_seek(void *ptr, ficl2Integer dpos, int whence);
static ficl2Integer string_tell(void *ptr);
static void	string_write_char(void *ptr, int c);
static void	string_write_line(void *ptr, const char *line);

#define h_list_of_io_functions "\
*** IO and FILE PRIMITIVES ***\n\
*stderr*            ( -- io )\n\
*stdin*             ( -- io )\n\
*stdout*            ( -- io )\n\
.io                 ( io -- )\n\
exit-status alias for io-exit-status\n\
io->string          ( io -- str )\n\
io-close            ( io -- )\n\
io-closed?          ( io -- f )\n\
io-eof?             ( io -- f )\n\
io-exit-status      ( -- n )\n\
io-fdopen           ( fd :key args -- io )\n\
io-filename         ( io -- fname )\n\
io-fileno           ( io -- fd )\n\
io-flush            ( io -- )\n\
io-getc             ( io -- c )\n\
io-input?           ( obj -- f )\n\
io-mode             ( io -- mode )\n\
io-open             ( name :key args -- io )\n\
io-open-file        ( :key args -- io )\n\
io-open-input-file  ( :key args -- io )\n\
io-open-output-file ( :key args -- io )\n\
io-open-read        ( name -- io )\n\
io-open-write       ( name :key args -- io )\n\
io-output?          ( obj -- f )\n\
io-popen            ( cmd :key args -- io )\n\
io-popen-read       ( cmd -- io )\n\
io-popen-write      ( cmd -- io )\n\
io-pos-ref          ( io -- pos )\n\
io-pos-set!         ( io pos -- )\n\
io-putc             ( io c -- )\n\
io-read             ( io -- line )\n\
io-readlines        ( io -- array-of-lines )\n\
io-reopen           ( io1 name :key args -- io2 )\n\
io-rewind           ( io -- )\n\
io-seek             ( io offset :key whence -- pos )\n\
io-sopen            ( str :key args -- io )\n\
io-sopen-read       ( str -- io )\n\
io-sopen-write      ( str -- io )\n\
io-tell alias for io-pos-ref\n\
io-write            ( io line -- )\n\
io-write-format     ( io fmt args -- )\n\
io-writelines       ( io array-of-lines -- )\n\
io=                 ( obj1 obj2 -- f )\n\
io?                 ( obj -- f )\n\
make-file-input-port alias for io-open-read\n\
make-file-output-port alias for io-open-write\n\
make-file-port alias for io-open\n\
make-pipe-input-port alias for io-popen-read\n\
make-pipe-output-port alias for io-popen-write\n\
make-pipe-port alias for io-popen\n\
make-string-input-port alias for io-sopen-read\n\
make-string-output-port alias for io-sopen-write\n\
make-string-port alias for io-sopen\n\
readlines           ( name -- array-of-lines )\n\
set-*stderr*        ( io1 -- io2 )\n\
set-*stdin*         ( io1 -- io2 )\n\
set-*stdout*        ( io1 -- io2 )\n\
set-version-control ( val -- )\n\
version-control     ( -- val )\n\
writelines          ( name array-of-lines -- )\n\
*** constants:\n\
SEEK_SET, SEEK_CUR, SEEK_END\n\
AF_*, SOCK_*, SHUT_*"

#if HAVE_SOCKET
#define h_list_of_io_socket_functions "\
*** IO SOCKET PRIMITIVES ***\n\
io-nopen            ( host :key args -- io )\n\
make-socket-port alias for io-nopen\n\
net-accept          ( fd host domain -- io )\n\
net-bind            ( fd host port domain -- )\n\
net-connect         ( fd host port domain -- io )\n\
net-listen          ( fd -- )\n\
net-recv            ( fd flags -- msg )\n\
net-recvfrom        ( fd flags host port domain -- msg )\n\
net-send            ( fd msg flags -- )\n\
net-sendto          ( fd msg flags host port domain -- )\n\
net-shutdown        ( fd how -- )\n\
net-socket          ( domain type -- fd )"
#endif				/* HAVE_SOCKET */

static FTH
io_inspect(FTH self)
{
	FTH fs;

	fs = fth_make_string(FTH_INSTANCE_NAME(self));
	if (fth_io_pos_ref(self) >= 0)
		fth_string_sformat(fs, "[%lld]", fth_io_pos_ref(self));
	if (FTH_STRING_P(FTH_IO_FILENAME(self)))
		fth_string_sformat(fs, ": \"%S\",", FTH_IO_FILENAME(self));
	fth_string_sformat(fs, " %S", FTH_IO_NAME(self));
	if (FTH_IO_INPUT_P(self)) {
		fth_string_sformat(fs, "-input");
		if (FTH_IO_OUTPUT_P(self))
			fth_string_sformat(fs, "/output");
	} else if (FTH_IO_OUTPUT_P(self))
		fth_string_sformat(fs, "-output");
	if (FTH_IO_CLOSED_P(self))
		fth_string_sformat(fs, " closed");
	return (fs);
}

static FTH
io_to_string(FTH self)
{
	return (fth_make_string_format("#<%S>", io_inspect(self)));
}

static FTH	string_empty;
static FTH	string_cr;
static FTH	string_space;

static FTH
io_to_array(FTH self)
{
	FTH ary;

	if (FTH_ARRAY_P(FTH_IO_BUFFER(self)))
		return (FTH_IO_BUFFER(self));
	if (FTH_IO_TYPE(self) == FTH_IO_STRING)
		ary = fth_string_split_2((FTH)FTH_IO_DATA(self), string_cr);
	else
		ary = fth_io_readlines(self);
	FTH_IO_BUFFER(self) = ary;
	return (FTH_IO_BUFFER(self));
}

/* io 2 object-ref => 3rd line of io object */
static FTH
io_ref(FTH self, FTH idx)
{
	return (fth_array_ref(fth_object_to_array(self), FTH_INT_REF(idx)));
}

static FTH
io_equal_p(FTH self, FTH obj)
{
	bool flag;

	flag = fth_string_equal_p(FTH_IO_FILENAME(self),
	    FTH_IO_FILENAME(obj)) &&
	    FTH_IO_FAM(self) == FTH_IO_FAM(obj) &&
	    fth_io_pos_ref(self) == fth_io_pos_ref(obj);
	return (BOOL_TO_FTH(flag));
}

static FTH
io_length(FTH self)
{
	return (fth_make_long_long(fth_io_length(self)));
}

static void
io_mark(FTH self)
{
	fth_gc_mark(FTH_IO_NAME(self));
	fth_gc_mark(FTH_IO_FILENAME(self));
	fth_gc_mark(FTH_IO_BUFFER(self));
}

static void
io_free(FTH self)
{
	if (!FTH_IO_CLOSED_P(self))
		FTH_IO_CLOSE(self);
	switch (FTH_IO_TYPE(self)) {
	case FTH_IO_SOCKET:
	case FTH_IO_PORT:
		FTH_FREE(FTH_IO_DATA(self));
		break;
	case FTH_IO_FILE:	/* FILE *fp */
	case FTH_IO_PIPE:	/* FILE *fp */
	case FTH_IO_STRING:	/* FTH_STRING */
	default:
		break;
	}
	FTH_FREE(FTH_IO_OBJECT(self));
}

static char *
fam_to_mode(int fam)
{
#define WARN_STR "\
%s accepts only r/o (r), w/o (w), r/w (w+), a/o (a), and r/a (a+)"
	static char mode[3];
	int i;

	i = 0;
	switch (fam) {
	case FICL_FAM_READ:
		mode[i++] = 'r';
		break;
	case FICL_FAM_WRITE:
		mode[i++] = 'w';
		break;
	case FICL_FAM_READ | FICL_FAM_WRITE:
		mode[i++] = 'w';
		mode[i++] = '+';
		break;
	case FICL_FAM_APPEND:	/* append, write */
		mode[i++] = 'a';
		break;
	case FICL_FAM_READ | FICL_FAM_APPEND:	/* append, read-write */
		mode[i++] = 'a';
		mode[i++] = '+';
		break;
	default:
		mode[i++] = 'r';
		fth_warning(WARN_STR, RUNNING_WORD());
		break;
	}
	mode[i] = '\0';
	return (mode);
}

/* --- Generic --- */

/* ARGSUSED */
static int
generic_read_char(void *ptr)
{
	return (EOF);
}

/* ARGSUSED */
static void
generic_write_char(void *ptr, int c)
{
}

/* ARGSUSED */
static char *
generic_read_line(void *ptr)
{
	return (NULL);
}

/* ARGSUSED */
static void
generic_write_line(void *ptr, const char *line)
{
}

/* ARGSUSED */
static bool
generic_eof_p(void *ptr)
{
	return (false);
}

/* ARGSUSED */
static ficl2Integer
generic_tell(void *ptr)
{
	return (0);
}

/* ARGSUSED */
static ficl2Integer
generic_seek(void *ptr, ficl2Integer pos, int whence)
{
	return (0);
}

/* ARGSUSED */
static void
generic_flush(void *ptr)
{
}

/* ARGSUSED */
static void
generic_rewind(void *ptr)
{
}

/* ARGSUSED */
static void
generic_close(void *ptr)
{
}

FTH
make_io_base(int fam)
{
	FIO *io;

	io = FTH_CALLOC(1, sizeof(FIO));
	io->type = FTH_IO_UNDEF;
	io->name = FTH_FALSE;
	io->filename = FTH_FALSE;
	io->buffer = FTH_FALSE;
	io->fam = fam;
	io->data = NULL;
	io->length = 0;
	io->input_p = (fam & FICL_FAM_READ);
	io->output_p = (fam & (FICL_FAM_WRITE | FICL_FAM_APPEND));
	io->closed_p = false;
	io->read_char = generic_read_char;
	io->write_char = generic_write_char;
	io->read_line = generic_read_line;
	io->write_line = generic_write_line;
	io->eof_p = generic_eof_p;
	io->tell = generic_tell;
	io->seek = generic_seek;
	io->flush = generic_flush;
	io->rewind = generic_rewind;
	io->close = generic_close;
	return (fth_make_instance(io_tag, io));
}

/* === FILE-IO === */

static int
file_read_char(void *ptr)
{
	FILE *fp = (FILE *)ptr;
	int c;

	c = fgetc(fp);
	if (c == EOF) {
		if (feof(fp))
			return (EOF);
		if (ferror(fp)) {
			clearerr(fp);
			IO_FILE_ERROR(fgetc);
		}
	}
	return (c);
}

static void
file_write_char(void *ptr, int c)
{
	FILE *fp = (FILE *)ptr;

	if (fputc(c, fp) == EOF)
		if (ferror(fp)) {
			clearerr(fp);
			IO_FILE_ERROR(fputc);
		}
}

static char io_scratch[BUFSIZ];

static char *
file_read_line(void *ptr)
{
	FILE *fp = (FILE *)ptr;
	char *p;

	p = fgets(io_scratch, (int)sizeof(io_scratch), fp);
	if (p != NULL)
		return (p);
	else {
		if (feof(fp))
			return (NULL);
		if (ferror(fp)) {
			clearerr(fp);
			IO_FILE_ERROR(fgets);
		}
	}
	/* NOTREACHED */
	return (NULL);
}

static void
file_write_line(void *ptr, const char *line)
{
	FILE *fp = (FILE *)ptr;

	if (fputs(line, fp) == EOF)
		if (ferror(fp)) {
			clearerr(fp);
			IO_FILE_ERROR(fputs);
		}
}

static bool
file_eof_p(void *ptr)
{
	return ((bool)feof((FILE *)ptr));
}

static ficl2Integer
file_tell(void *ptr)
{
	FILE *fp = (FILE *)ptr;

	fflush(fp);
	return ((ficl2Integer)lseek(fileno(fp), (off_t)0, SEEK_CUR));
}

static ficl2Integer
file_seek(void *ptr, ficl2Integer pos, int whence)
{
	FILE *fp = (FILE *)ptr;

	fflush(fp);
	return ((ficl2Integer)lseek(fileno(fp), (off_t)pos, whence));
}

static void
file_flush(void *ptr)
{
	fflush((FILE *)ptr);
}

static void
file_rewind(void *ptr)
{
	FILE *fp = (FILE *)ptr;

	fflush(fp);
	rewind(fp);
}

static void
file_close(void *ptr)
{
	fclose((FILE *)ptr);
}

static FTH
make_file_io(FILE *fp, const char *name, int fam)
{
	char *mode;
	FTH io;

	mode = fam_to_mode(fam);
	if (fp == NULL) {
		fp = fopen(name, mode);
		if (fp == NULL) {
			IO_FILE_ERROR_ARG(fopen, name);
			/* NOTREACHED */
			return (FTH_FALSE);
		}
	}
	io = make_io_base(fam);
	FTH_IO_TYPE(io) = FTH_IO_FILE;
	FTH_IO_NAME(io) = fth_make_string("file");
	FTH_IO_FILENAME(io) = fth_make_string(name);
	FTH_IO_DATA(io) = (void *)fp;
	FTH_IO_OBJECT(io)->read_char = file_read_char;
	FTH_IO_OBJECT(io)->write_char = file_write_char;
	FTH_IO_OBJECT(io)->read_line = file_read_line;
	FTH_IO_OBJECT(io)->write_line = file_write_line;
	FTH_IO_OBJECT(io)->eof_p = file_eof_p;
	FTH_IO_OBJECT(io)->tell = file_tell;
	FTH_IO_OBJECT(io)->seek = file_seek;
	FTH_IO_OBJECT(io)->flush = file_flush;
	FTH_IO_OBJECT(io)->rewind = file_rewind;
	FTH_IO_OBJECT(io)->close = file_close;
	return (io);
}

/* === PIPE-IO === */

static void
pipe_close(void *ptr)
{
	fth_set_exit_status(pclose((FILE *)ptr));
}

/* cmd: string or array of strings */
FTH
fth_io_popen(FTH cmd, int fam)
{
	char *name;
	FTH io;
	FILE *fp;

	FTH_ASSERT_ARGS((FTH_STRING_P(cmd) || FTH_ARRAY_P(cmd)),
	    cmd, FTH_ARG1, "a string or an array of strings");
	io = make_io_base(fam);
	name = NULL;
	if (FTH_STRING_P(cmd)) {
		name = fth_string_ref(cmd);
		FTH_IO_FILENAME(io) = cmd;
	} else if (FTH_ARRAY_P(cmd)) {
		name = fth_string_ref(fth_array_join(cmd, string_space));
		FTH_IO_FILENAME(io) = fth_array_ref(cmd, 0L); 
	}
	if (name == NULL) {
		FTH_ASSERT_ARGS(false, cmd, FTH_ARG1,
		    "a string or an array of strings");
		/* NOTREACHED */
		return (FTH_FALSE);
	}
	fp = popen(name, fam_to_mode(fam));
	if (fp == NULL) {
		IO_FILE_ERROR_ARG(popen, name);
		/* NOTREACHED */
		return (FTH_FALSE);
	}
	FTH_IO_TYPE(io) = FTH_IO_PIPE;
	FTH_IO_NAME(io) = fth_make_string("pipe");
	FTH_IO_DATA(io) = (void *)fp;
	FTH_IO_OBJECT(io)->read_char = file_read_char;
	FTH_IO_OBJECT(io)->write_char = file_write_char;
	FTH_IO_OBJECT(io)->read_line = file_read_line;
	FTH_IO_OBJECT(io)->write_line = file_write_line;
	FTH_IO_OBJECT(io)->eof_p = file_eof_p;
	FTH_IO_OBJECT(io)->flush = file_flush;
	FTH_IO_OBJECT(io)->close = pipe_close;
	return (io);
}

/* === STRING-IO === */

static int
string_read_char(void *ptr)
{
	if (FTH_IO_STRING_INDEX_REF(ptr) < FTH_IO_STRING_LENGTH(ptr))
		return (fth_string_c_char_fast_ref((FTH)ptr,
		    FTH_IO_STRING_INDEX_REF(ptr)++));
	return (EOF);
}

static void
string_write_char(void *ptr, int c)
{
	if (FTH_IO_STRING_INDEX_REF(ptr) < (FTH_IO_STRING_LENGTH(ptr) - 1))
		fth_string_c_char_fast_set((FTH)ptr,
		    FTH_IO_STRING_INDEX_REF(ptr)++, (char)c);
	else
		fth_string_push((FTH)ptr,
		    fth_make_string_format("%c", (char)c));
}

static char *
string_read_line(void *ptr)
{
	char *line;
	size_t i, size;
	char c;

	if (FTH_IO_STRING_INDEX_REF(ptr) >= FTH_IO_STRING_LENGTH(ptr))
		return (NULL);
	line = io_scratch;
	size = sizeof(io_scratch);
	for (i = 0;
	    i < size &&
	    FTH_IO_STRING_INDEX_REF(ptr) < FTH_IO_STRING_LENGTH(ptr);
	    i++) {
		c = fth_string_c_char_fast_ref((FTH)ptr,
		    FTH_IO_STRING_INDEX_REF(ptr)++);
		if (c == '\n')
			break;
		line[i] = c;
	}
	line[i] = '\0';
	return (line);
}

static void
string_write_line(void *ptr, const char *line)
{
	if (line == NULL)
		return;
	if (FTH_IO_STRING_INDEX_REF(ptr) < FTH_IO_STRING_LENGTH(ptr)) {
		ficlInteger i, len;

		len = (ficlInteger)fth_strlen(line);
		for (i = 0;
		    i < len &&
		    FTH_IO_STRING_INDEX_REF(ptr) < FTH_IO_STRING_LENGTH(ptr);
		    i++)
			fth_string_c_char_fast_set((FTH)ptr,
			    FTH_IO_STRING_INDEX_REF(ptr)++, line[i]);
		if (i < len)
			fth_string_push((FTH)ptr, fth_make_string(line + i));
	} else
		fth_string_push((FTH)ptr, fth_make_string(line));
}

static bool
string_eof_p(void *ptr)
{
	return (FTH_IO_STRING_INDEX_REF(ptr) >= FTH_IO_STRING_LENGTH(ptr));
}

static ficl2Integer
string_tell(void *ptr)
{
	return (FTH_IO_STRING_INDEX_REF(ptr));
}

static ficl2Integer
string_seek(void *ptr, ficl2Integer dpos, int whence)
{
	ficlInteger pos, end;

	pos = (ficlInteger)dpos;
	switch (whence) {
	case SEEK_SET:
		FTH_IO_STRING_INDEX_SET(ptr, pos);
		break;
	case SEEK_CUR:
		FTH_IO_STRING_INDEX_SET(ptr,
		    FTH_IO_STRING_INDEX_REF(ptr) + pos);
		break;
	case SEEK_END:
	default:
		end = FTH_IO_STRING_LENGTH(ptr) - 1;
		if (pos > 0)
			end -= pos;
		else
			end += pos;
		FTH_IO_STRING_INDEX_SET(ptr, end);
		break;
	}
	return ((ficl2Integer)FTH_IO_STRING_INDEX_REF(ptr));
}

static void
string_rewind_and_close(void *ptr)
{
	fth_cycle_pos_0((FTH)ptr);
}

FTH
fth_io_sopen(FTH string, int fam)
{
	FTH io;

	if (!FTH_STRING_P(string))
		string = fth_make_empty_string();
	io = make_io_base(fam);
	FTH_IO_TYPE(io) = FTH_IO_STRING;
	FTH_IO_NAME(io) = fth_make_string("string");
	FTH_IO_DATA(io) = (void *)string;
	FTH_IO_LENGTH(io) = fth_string_length(string);
	FTH_IO_OBJECT(io)->read_char = string_read_char;
	FTH_IO_OBJECT(io)->write_char = string_write_char;
	FTH_IO_OBJECT(io)->read_line = string_read_line;
	FTH_IO_OBJECT(io)->write_line = string_write_line;
	FTH_IO_OBJECT(io)->eof_p = string_eof_p;
	FTH_IO_OBJECT(io)->tell = string_tell;
	FTH_IO_OBJECT(io)->seek = string_seek;
	FTH_IO_OBJECT(io)->rewind = string_rewind_and_close;
	FTH_IO_OBJECT(io)->close = string_rewind_and_close;
	if (fam & FICL_FAM_APPEND)
		FTH_IO_SEEK(io, 0L, SEEK_END);
	return (io);
}

ficl2Integer
fth_io_length(FTH io)
{
	ficl2Integer io_len, pos;

	IO_ASSERT_IO(io);
	if (!FTH_INSTANCE_CHANGED_P(io))
		return (FTH_IO_LENGTH(io));
	switch (FTH_IO_TYPE(io)) {
	case FTH_IO_FILE:
		if (fth_io_fileno(io) < 3)
			io_len = FTH_IO_TELL(io);
		else {
			pos = FTH_IO_TELL(io);
			io_len = FTH_IO_SEEK(io, (off_t)0, SEEK_END);
			FTH_IO_SEEK(io, pos, SEEK_SET);
		}
		break;
	case FTH_IO_STRING:
		io_len = FTH_IO_STRING_LENGTH(FTH_IO_DATA(io));
		break;
	case FTH_IO_PORT:
		io_len = 0;
		break;
	case FTH_IO_SOCKET:
	case FTH_IO_PIPE:
	default:
		io_len = FTH_IO_TELL(io);
		break;
	}
	return (FTH_IO_LENGTH(io) = io_len);
}

static void
ficl_io_filename(ficlVm *vm)
{
#define h_io_filename "( io -- filename|#f )  return file name\n\
\".fthrc\" io-open-read value io\n\
io io-filename => \".fthrc\"\n\
Return file name of IO object or #f if file name is not available."
	FTH io;

	FTH_STACK_CHECK(vm, 1, 1);
	io = fth_pop_ficl_cell(vm);
	IO_ASSERT_IO(io);
	ficlStackPushFTH(vm->dataStack, FTH_IO_FILENAME(io));
}

static void
ficl_io_mode(ficlVm *vm)
{
#define h_io_mode "( io -- mode )  return access mode\n\
\".fthrc\" io-open-read value io\n\
io io-mode => \"r\"\n\
Return access mode of IO object."
	FTH io;

	FTH_STACK_CHECK(vm, 1, 1);
	io = fth_pop_ficl_cell(vm);
	IO_ASSERT_IO(io);
	push_cstring(vm, fam_to_mode(FTH_IO_FAM(io)));
}

static void
ficl_io_fileno(ficlVm *vm)
{
#define h_io_fileno "( io -- fd )  return file descriptor\n\
\".fthrc\" io-open-read value io\n\
io io-fileno => 3\n\
Return file descriptor of IO."
	FTH io;

	FTH_STACK_CHECK(vm, 1, 1);
	io = fth_pop_ficl_cell(vm);
	ficlStackPushInteger(vm->dataStack, (ficlInteger)fth_io_fileno(io));
}

FTH
fth_io_to_string(FTH io)
{
#define h_io_to_string "( io -- str )  return IO content as string\n\
\".fthrc\" io-open-read value io\n\
io io->string => \"...\"\n\
Return content of IO object as string if possible."
	IO_ASSERT_IO(io);
	if (FTH_IO_TYPE(io) == FTH_IO_STRING)
		return (fth_string_copy((FTH)FTH_IO_DATA(io)));
	return (fth_array_join(fth_object_to_array(io), string_empty));
}

static void
ficl_io_p(ficlVm *vm)
{
#define h_io_p "( obj -- f )  test if OBJ is an IO object\n\
\".fthrc\" io-open-read value io\n\
io io? => #t\n\
10 io? => #f\n\
Return #t if OBJ is an IO object, otherwise #f."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, FTH_IO_P(obj));
}

bool
fth_io_input_p(FTH obj)
{
	return (IO_INPUT_P(obj));
}

static void
ficl_io_input_p(ficlVm *vm)
{
#define h_io_input_p "( obj -- f )  test if OBJ is an input IO object\n\
\".fthrc\" io-open-read value io1\n\
\"foo\" io-open-write value io2\n\
io1 io-input? => #t\n\
io2 io-input? => #f\n\
Return #t if OBJ is an input IO object, otherwise #f."
	FTH io;

	FTH_STACK_CHECK(vm, 1, 1);
	io = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, IO_INPUT_P(io));
}

bool
fth_io_output_p(FTH obj)
{
	return (IO_OUTPUT_P(obj));
}

static void
ficl_io_output_p(ficlVm *vm)
{
#define h_io_output_p "( obj -- f )  test if OBJ is an output IO object\n\
\".fthrc\" io-open-read value io1\n\
\"foo\" io-open-write value io2\n\
io1 io-output? => #f\n\
io2 io-output? => #t\n\
Return #t if OBJ is an output IO object, otherwise #f."
	FTH io;

	FTH_STACK_CHECK(vm, 1, 1);
	io = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, IO_OUTPUT_P(io));
}

bool
fth_io_closed_p(FTH obj)
{
	return (FTH_IO_P(obj) && FTH_IO_CLOSED_P(obj));
}

static void
ficl_io_closed_p(ficlVm *vm)
{
#define h_io_closed_p "( io -- f )  test if IO is closed\n\
\".fthrc\" io-open-read value io\n\
io io-closed? => #f\n\
io io-close\n\
io io-closed? => #t\n\
Return #t if IO object is closed, otherwise #f."
	FTH io;

	FTH_STACK_CHECK(vm, 1, 1);
	io = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, fth_io_closed_p(io));
}

static FTH	version_control;
static FTH	version_number_string;

static int
file_number(char *name)
{
	FTH dir, files;
	char *bname, *path, *s;
	size_t flen, blen, plen;
	ficlInteger i, len;
	int numb, x;

	if (name == NULL)
		return (0);
	dir = fth_file_dirname(name);
	files = fth_file_match_dir(dir, version_number_string);
	flen = (size_t)fth_array_length(files);
	bname = fth_basename(name);
	blen = fth_strlen(bname);
	if (flen <= 0 || blen <= 0)
		return (0);
	numb = 0;
	len = (ficlInteger)flen;
	path = fth_format("%S/%.*s", dir, (int)blen, bname);
	plen = fth_strlen(path);
	for (i = 0; i < len; i++) {
		s = fth_string_ref(fth_array_ref(files, i));
		if (s != NULL && (strncmp(s, path, plen) == 0)) {
			x = (int)strtol(s + plen + 2, NULL, 10);
			if (++x > numb)
				numb = x;
		}
	}
	FTH_FREE(path);
	return (numb);
}

static void
file_version_rename(char *name)
{
	char *new_name;
	int numb;

	if (FTH_TRUE_P(version_control)) {
		/* numbered backups */
		numb = file_number(name);
		if (numb == 0)
			numb++;
		new_name = fth_format("%s.~%d~", name, numb);
		fth_file_rename(name, new_name);
		FTH_FREE(new_name);
		return;
	}
	if (FTH_NIL_P(version_control)) {
		/* numbered/simple backups */
		numb = file_number(name);
		if (numb > 0)
			new_name = fth_format("%s.~%d~", name, numb);
		else
			new_name = fth_format("%s~", name);
		fth_file_rename(name, new_name);
		FTH_FREE(new_name);
		return;
	}
	if (FTH_FALSE_P(version_control)) {
		/* simple backups */
		new_name = fth_format("%s~", name);
		fth_file_rename(name, new_name);
		FTH_FREE(new_name);
	}
	/* else no backups */
}

static void
io_if_exists(char *name, FTH exists, int fam)
{
	if ((fam & (FICL_FAM_WRITE | FICL_FAM_APPEND)) &&
	    fth_file_exists_p(name)) {
		if (exists == FTH_KEYWORD_RENAME)
			file_version_rename(name);
		else if (exists == FTH_KEYWORD_ERROR)
			fth_throw(FTH_FILE_IO_ERROR, "%s: \"%s\" exists",
			    RUNNING_WORD(), name);
		/* else overwrite */
	}
}

static void
ficl_version_control(ficlVm *vm)
{
#define h_version_control "( -- val )  version-control\n\
Return current version control style.\n\
#t    => numbered backups\n\
nil   => numbered/simple backups\n\
#f    => simple backups\n\
undef => no backups"
	fth_push_ficl_cell(vm, version_control);
}

static void
ficl_set_version_control(ficlVm *vm)
{
#define h_set_vc "( val -- )  set version-control\n\
Set current version control style.  \
Accepted values:\n\
#t    => numbered backups\n\
nil   => numbered/simple backups\n\
#f    => simple backups\n\
undef => no backups"
	FTH val;

	FTH_STACK_CHECK(vm, 1, 0);
	val = fth_pop_ficl_cell(vm);
	if (FTH_BOOLEAN_P(val) || FTH_NIL_TYPE_P(val))
		version_control = val;
	else
		version_control = FTH_UNDEF;
}

static void
ficl_io_open_file(ficlVm *vm)
{
#define h_io_open_file "( :key fam r/o args -- io )  open IO\n\
:filename \"foo\"          io-open-file value io1\n\
:filename \"foo\" :fam r/o io-open-file value io2\n\
:filename \"bar\" :fam r/w io-open-file value io3\n\
:command \"ls -lF\"        io-open-file value io4\n\
:command #( \"ls\" \"-lF\" ) io-open-file value io5\n\
:string \"test string\"    io-open-file value io6\n\
:soft-port \"test\"        io-open-file value io7\n\
Keyword argument :fam defaults to r/o.\n\
General IO open function.  \
Return IO object for io-read/io-write etc.\n\
" keyword_args_string
	int arg;

	arg = fth_get_optkey_fix(FTH_KEYWORD_FAM, FICL_FAM_READ);
	fth_push_ficl_cell(vm, io_keyword_args_ref(arg));
}

static void
ficl_io_open_input_file(ficlVm *vm)
{
#define h_io_oi_file "( :key args -- io )  open IO for reading\n\
:filename \"foo\"          io-open-input-file value io1\n\
:command \"ls -lF\"        io-open-input-file value io2\n\
:command #( \"ls\" \"-lF\" ) io-open-input-file value io3\n\
:string \"test string\"    io-open-input-file value io4\n\
:soft-port \"input-test\"  io-open-input-file value io5\n\
General IO open function.  \
Return IO object for reading.\n\
" keyword_args_string
	fth_push_ficl_cell(vm, io_keyword_args_ref(FICL_FAM_READ));
}

static void
ficl_io_open_output_file(ficlVm *vm)
{
#define h_io_oo_file "( :key args -- io )  open IO for writing\n\
:filename \"foo\"          io-open-output-file value io1\n\
:command \"cat\"           io-open-output-file value io2\n\
:command #( \"cat\" )      io-open-output-file value io3\n\
\"\" value s1\n\
:string s1               io-open-output-file value io4\n\
:soft-port \"output-test\" io-open-output-file value io5\n\
General IO open function.  \
Return IO object for writing.\n\
" keyword_args_string
	fth_push_ficl_cell(vm, io_keyword_args_ref(FICL_FAM_WRITE));
}

FTH
fth_io_open(const char *name, int fam)
{
	return (make_file_io(NULL, name, fam));
}

#define h_fam_values "\
Possible FAM values:\n\
r/o  --  read-only         \"r\"\n\
w/o  --  write-only        \"w\"\n\
r/w  --  read/write        \"w+\"\n\
a/o  --  append            \"a\"\n\
r/a  --  read/write-append \"a+\""

#define h_if_exist_help "\
If keyword IF-EXISTS was not specified, \
overwrite possible existing file before open it for writing or appending.\n\
Possible IF-EXISTS values:\n\
:overwrite  --  overwrite existing file (default)\n\
:error      --  raise IO-ERROR exception if NAME already exist\n\
:rename     --  depends on environment variable $VERSION_CONTROL\n\
                and global Fth variable VERSION-CONTROL,\n\
                see version-control and set-version-control for details."

static void
ficl_io_open(ficlVm *vm)
{
#define h_io_open "( name :key fam r/o if-exists overwrite -- io )  open\n\
\"in-test\"                                 io-open value read-io1\n\
\"in-test\"  :fam r/o                       io-open value read-io2\n\
\"out-test\" :fam w/o                       io-open value overwrite-io1\n\
\"out-test\" :fam w/o :if-exists :overwrite io-open value overwrite-io2\n\
\"out-test\" :fam r/w :if-exists :error     io-open value read-write-io1\n\
\"out-test\" :fam r/w :if-exists :rename    io-open value rename-io1\n\
Open file NAME and return new IO object.  \
If keyword FAM was not specified, open file read-only, otherwise take FAM.\n\
" h_fam_values "\n" h_if_exist_help "\n\
See also io-open-read and io-open-write."
	FTH exists, fs;
	int fam;
	char *name;

	exists = fth_get_optkey(FTH_KEYWORD_IF_EXISTS, FTH_KEYWORD_OVERWRITE);
	fam = fth_get_optkey_fix(FTH_KEYWORD_FAM, FICL_FAM_READ);
	FTH_STACK_CHECK(vm, 1, 1);
	fs = fth_pop_ficl_cell(vm);
	IO_ASSERT_STRING(fs, FTH_ARG1);
	name = IO_STRING_REF(fs);
	io_if_exists(name, exists, fam);
	ficlStackPushFTH(vm->dataStack, fth_io_open(name, fam));
}

static void
ficl_io_open_read(ficlVm *vm)
{
#define h_io_open_read "( name -- io )  open\n\
\"in-test\" io-open-read value read-io1\n\
Open file NAME for reading and return new IO object.\n\
See also io-open and io-open-write."
	FTH fs;
	char *name;

	FTH_STACK_CHECK(vm, 1, 1);
	fs = fth_pop_ficl_cell(vm);
	IO_ASSERT_STRING(fs, FTH_ARG1);
	name = IO_STRING_REF(fs);
	if (!fth_file_exists_p(name))
		fth_throw(FTH_NO_SUCH_FILE, "%s: \"%s\" doesn't exist",
		    RUNNING_WORD(), name);
	ficlStackPushFTH(vm->dataStack, fth_io_open(name, FICL_FAM_READ));
}

static void
ficl_io_open_write(ficlVm *vm)
{
#define h_io_open_write "( name :key if-exists overwrite -- io )  open\n\
\"out-test\"                       io-open-write value overwrite-io1\n\
\"out-test\" :if-exists :overwrite io-open-write value overwrite-io2\n\
\"out-test\" :if-exists :error     io-open-write value error-io1\n\
\"out-test\" :if-exists :rename    io-open-write value rename-io1\n\
Open file NAME for writing and return new IO object.\n\
" h_if_exist_help "\n\
See also io-open and io-open-read."
	FTH exists, fs;
	char *name;

	exists = fth_get_optkey(FTH_KEYWORD_IF_EXISTS, FTH_KEYWORD_OVERWRITE);
	FTH_STACK_CHECK(vm, 1, 1);
	fs = fth_pop_ficl_cell(vm);
	name = IO_STRING_REF(fs);
	io_if_exists(name, exists, FICL_FAM_WRITE);
	ficlStackPushFTH(vm->dataStack, fth_io_open(name, FICL_FAM_WRITE));
}

static void
ficl_io_reopen(ficlVm *vm)
{
#define h_io_reopen "( io1 name :key fam io1-fam -- io2 )  copy IO1\n\
\"1-test\" io-open-write value io1\n\
io1 \"hello\" io-write\n\
io1 \"2-test\" io-reopen value io2\n\
io1 io-closed? => #t\n\
io2 \"world\" io-write\n\
io2 io-close\n\
io2 io-closed? => #t\n\
\"1-test\" readlines => #( \"hello\" )\n\
\"2-test\" readlines => #( \"world\" )\n\
*stderr* \"error.log\" io-reopen value err-io\n\
Return new IO object as copy of IO1 and close IO1.  \
If NAME is not a string, e.g. #f or nil, use file name from IO1.  \
If keyword FAM was not given, use mode from IO1, otherwise use FAM.  \
All restrictions on reopen apply, \
ie. a file opened for reading can't reopened for writing etc.\n\
See freopen(3) for more information."
	FTH io, fname, ffam;
	char *name;
	int fam;
	FILE *fp;

	ffam = fth_get_optkey(FTH_KEYWORD_FAM, FTH_UNDEF);
	FTH_STACK_CHECK(vm, 2, 1);
	fname = ficlStackPopFTH(vm->dataStack);
	io = ficlStackPopFTH(vm->dataStack);
	IO_ASSERT_IO(io);
	if (FTH_IO_TYPE(io) != FTH_IO_FILE) {
		ficlStackPushFTH(vm->dataStack, io);
		return;
	}
	if (FTH_STRING_P(fname))
		name = fth_string_ref(fname);
	else
		name = IO_STRING_REF(FTH_IO_FILENAME(io));
	if (FTH_BOUND_P(ffam))
		fam = FIX_TO_INT32(ffam);
	else
		fam = FTH_IO_FAM(io);
	fp = freopen(name, fam_to_mode(fam), FTH_IO_DATA(io));
	/* The original file pointer is already closed by freopen. */
	FTH_IO_CLOSED_P(io) = true;
	if (fp == NULL) {
		IO_FILE_ERROR_ARG(freopen, name);
		/* NOTREACHED */
		return;
	}
	ficlStackPushFTH(vm->dataStack, make_file_io(fp, name, fam));
}

static void
ficl_io_fdopen(ficlVm *vm)
{
#define h_io_fdopen "( fd :key fam r/o -- io )  return IO with FD\n\
2 :fam w/o io-fdopen value err-io\n\
err-io \"our error log\" io-write\n\
err-io io-flush => \"our error log\" (on stderr)\n\
Connect file descriptor FD to new IO object.  \
If keyword FAM was not given, open file read-only, otherwise use FAM.\n\
See fdopen(3) for more information."
	FTH io;
	FILE *fp;
	int fd, fam;

	fam = fth_get_optkey_fix(FTH_KEYWORD_FAM, FICL_FAM_READ);
	FTH_STACK_CHECK(vm, 1, 1);
	fd = (int)ficlStackPopInteger(vm->dataStack);
	fp = fdopen(fd, fam_to_mode(fam));
	if (fp == NULL) {
		IO_FILE_ERROR(fdopen);
		/* NOTREACHED */
		return;
	}
	io = make_file_io(fp, RUNNING_WORD(), fam);
	ficlStackPushFTH(vm->dataStack, io);
}

static void
ficl_io_popen(ficlVm *vm)
{
#define h_io_popen "( cmd :key fam r/o -- io )  open pipe\n\
\"ls -lAF ~/\"         io-popen value read-io1\n\
#( \"ls\" \"-lAF\" \"~/\") io-popen value read-io2\n\
read-io1 io->string => \"...\"\n\
read-io1 io-close\n\
\"cat\" :fam w/o       io-popen-write value write-io1\n\
write-io1 \"hello\" io-write\n\
write-io1 io-close\n\
Open pipe for command CMD and return new IO object.  \
CMD may be a string (with shell expansion) or an array of strings.  \
If keyword FAM was not given, open pipe read-only, otherwise use FAM.\n\
" h_fam_values "\n\
See also io-popen-read, io-popen-write and exec."
	FTH obj, io;
	int fam;

	fam = fth_get_optkey_fix(FTH_KEYWORD_FAM, FICL_FAM_READ);
	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	io = fth_io_popen(obj, fam);
	ficlStackPushFTH(vm->dataStack, io);
}

static void
ficl_io_popen_read(ficlVm *vm)
{
#define h_io_popen_read "( cmd -- io )  open pipe for reading\n\
\"ls -lAF ~/\"         io-popen-read value io1\n\
#( \"ls\" \"-lAF\" \"~/\") io-popen-read value io2\n\
io1 io->string => \"...\"\n\
io1 io-close\n\
Open read-only pipe for command CMD and return new IO object.  \
CMD may be a string (with shell expansion) or an array of strings.\n\
See also io-popen, io-popen-write and exec."
	FTH obj, io;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	io = fth_io_popen(obj, FICL_FAM_READ);
	ficlStackPushFTH(vm->dataStack, io);
}

static void
ficl_io_popen_write(ficlVm *vm)
{
#define h_io_powrite "( cmd -- io )  open pipe for writing\n\
\"cat\"      io-popen-write value io1\n\
#( \"cat\" ) io-popen-write value io2\n\
io1 \"hello\" io-write\n\
io1 io-close\n\
Open write-only pipe for command CMD and return new IO object.  \
CMD may be a string (with shell expansion) or an array of strings.\n\
See also io-popen, io-popen-read and exec."
	FTH obj, io;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	io = fth_io_popen(obj, FICL_FAM_WRITE);
	ficlStackPushFTH(vm->dataStack, io);
}

static void
ficl_io_sopen(ficlVm *vm)
{
#define h_io_sopen "( string :key fam r/o -- io )  open string\n\
\"test-string\" value s1\n\
s1 io-sopen value read-io1\n\
read-io1 io-read => \"test-string\"\n\
read-io1 io-close\n\
s1 :fam r/a io-sopen value append-io1\n\
append-io1 \" with append content\" io-write\n\
append-io1 io-rewind\n\
append-io1 io-read => \"test-string with append content\"\n\
append-io1 io-close\n\
s1 .string => \"test-string with append content\"\n\
Open string with content STRING and return new IO object.  \
If keyword FAM was not given, opens string read-only, otherwise takes FAM.\n\
" h_fam_values
	FTH fs, io;
	int fam;

	fam = fth_get_optkey_fix(FTH_KEYWORD_FAM, FICL_FAM_READ);
	FTH_STACK_CHECK(vm, 1, 1);
	fs = fth_pop_ficl_cell(vm);
	io = fth_io_sopen(fs, fam); 
	ficlStackPushFTH(vm->dataStack, io);
	    
}

static void
ficl_io_sopen_read(ficlVm *vm)
{
#define h_io_soread "( string -- io )  open string for reading\n\
\"test-string\" value s1\n\
s1 io-sopen-read value io1\n\
io1 io-read => \"test-string\"\n\
io1 io-close\n\
Open read-only string with content STRING and return new IO object.\n\
See also io-sopen."
	FTH fs, io;

	FTH_STACK_CHECK(vm, 1, 1);
	fs = fth_pop_ficl_cell(vm);
	io = fth_io_sopen(fs, FICL_FAM_READ);
	ficlStackPushFTH(vm->dataStack, io);
}

static void
ficl_io_sopen_write(ficlVm *vm)
{
#define h_io_sowrite "( string -- io )  open string for writing\n\
\"\" value s1\n\
s1 io-sopen-write value io1\n\
io1 \"test-string\" io-write\n\
io1 io-close\n\
s1 .string => \"test-string\"\n\
Open STRING for writing and return new IO object.\n\
See also io-sopen."
	FTH fs, io;

	FTH_STACK_CHECK(vm, 1, 1);
	fs = fth_pop_ficl_cell(vm);
	io = fth_io_sopen(fs, FICL_FAM_WRITE);
	ficlStackPushFTH(vm->dataStack, io);
}

#if HAVE_SOCKET

/* === SOCKET-IO === */

static int
socket_read_char(void *ptr)
{
	char msg[2];
	ssize_t len;

	msg[0] = '\0';
	len = recv(FTH_IO_SOCKET_FD(ptr), msg, 1L, 0);
	if (len == -1) {
		IO_SOCKET_ERROR(recv);
		/* NOTREACHED */
		return (-1);
	}
	if (len == 1)
		return ((int)msg[0]);
	return (-1);
}

static void
socket_write_char(void *ptr, int c)
{
	char msg[2];

	msg[0] = (char)c;
	msg[1] = '\0';
	if (send(FTH_IO_SOCKET_FD(ptr), msg, 1L, 0) == -1)
		IO_SOCKET_ERROR(send);
}

static char *
socket_read_line(void *ptr)
{
	char *line;
	size_t size;
	ssize_t len;
	socklen_t slen;

	line = io_scratch;
	size = sizeof(io_scratch);
	len = recvfrom(FTH_IO_SOCKET_FD(ptr), line, size, 0, NULL, &slen);
	if (len == -1) {
		IO_SOCKET_ERROR(recvfrom);
		/* NOTREACHED */
		return (NULL);
	}
	if (len > 0) {
		line[len] = '\0';
		return (line);
	}
	return (NULL);
}

static void
socket_write_line(void *ptr, const char *line)
{
	if (send(FTH_IO_SOCKET_FD(ptr), line, fth_strlen(line), 0) == -1)
		IO_SOCKET_ERROR(send);
}

static bool
socket_eof_p(void *ptr)
{
	int fd;
	off_t cur, end;

	fd = FTH_IO_SOCKET_FD(ptr);
	cur = lseek(fd, (off_t)0, SEEK_CUR);
	if (cur == -1)
		IO_SOCKET_ERROR(lseek);
	end = lseek(fd, (off_t)0, SEEK_END);
	if (end == -1)
		IO_SOCKET_ERROR(lseek);
	if (lseek(fd, cur, SEEK_SET) == -1)
		IO_SOCKET_ERROR(lseek);
	return (cur == end);
}

static ficl2Integer
socket_tell(void *ptr)
{
	off_t pos;

	pos = lseek(FTH_IO_SOCKET_FD(ptr), (off_t)0, SEEK_CUR);
	return ((ficl2Integer)pos);
}

static ficl2Integer
socket_seek(void *ptr, ficl2Integer pos, int whence)
{
	off_t npos;

	npos = lseek(FTH_IO_SOCKET_FD(ptr), (off_t)pos, whence);
	return ((ficl2Integer)npos);
}

static void
socket_rewind(void *ptr)
{
	lseek(FTH_IO_SOCKET_FD(ptr), (off_t)0, SEEK_SET);
}

static void
socket_unlink(const char *host)
{
	char *host_del;
	int old_errno;

	host_del = fth_format("%.*s", fth_strlen(host) - 1, host);
	old_errno = errno;
	unlink(host_del);
	errno = old_errno;
	FTH_FREE(host_del);
}

static void
socket_close(void *ptr)
{
	if (close(FTH_IO_SOCKET_FD(ptr)) == -1)
		IO_SOCKET_ERROR(close);
	socket_unlink(IO_STRING_REF(FTH_IO_SOCKET_HOST(ptr)));
}

#if defined(HAVE_STRUCT_SOCKADDR_UN_SUN_LEN)
#define IO_SOCKADDR(Addr, Host, Port, Domain, Len) do {			\
	if (Domain == AF_UNIX) {					\
		struct sockaddr_un sun;					\
									\
		Addr = (struct sockaddr *)&sun;				\
		sun.sun_family = (sa_family_t)AF_UNIX;			\
		sun.sun_path[0] = '\0';					\
		fth_strcat(sun.sun_path, sizeof(sun.sun_path), Host);	\
		Len = (socklen_t)(sizeof(sun.sun_family) +		\
		    fth_strlen(sun.sun_path));				\
		sun.sun_len = (unsigned char)Len;			\
	} else {							\
		struct sockaddr_in sin;					\
		struct hostent *hp;					\
									\
		Addr = (struct sockaddr *)&sin;				\
		sin.sin_family = (sa_family_t)AF_INET;			\
		sin.sin_port = (in_port_t)htons((in_port_t)Port);	\
		Len = sin.sin_len = (socklen_t)sizeof(sin);		\
		hp = gethostbyname(Host);				\
		if (hp != NULL)						\
			memmove(&sin.sin_addr, hp->h_addr,		\
			    (size_t)hp->h_length);			\
		else							\
			IO_SOCKET_ERROR_ARG(gethostbyname, Host);	\
	}								\
} while (0)
#else /* !HAVE_STRUCT_SOCKADDR_UN_SUN_LEN */
#define IO_SOCKADDR(Addr, Host, Port, Domain, Len) do {			\
	if (Domain == AF_UNIX) {					\
		struct sockaddr_un sun;					\
									\
		Addr = (struct sockaddr *)&sun;				\
		sun.sun_family = (sa_family_t)AF_UNIX;			\
		sun.sun_path[0] = '\0';					\
		fth_strcat(sun.sun_path, sizeof(sun.sun_path), Host);	\
		Len = (socklen_t)(sizeof(sun.sun_family) +		\
		    fth_strlen(sun.sun_path));				\
	} else {							\
		struct sockaddr_in sin;					\
		struct hostent *hp;					\
									\
		Addr = (struct sockaddr *)&sin;				\
		sin.sin_family = (sa_family_t)AF_INET;			\
		sin.sin_port = (in_port_t)htons((in_port_t)Port);	\
		Len = (socklen_t)sizeof(sin);				\
		hp = gethostbyname(Host);				\
		if (hp != NULL)						\
			memmove(&sin.sin_addr, hp->h_addr,		\
			    (size_t)hp->h_length);			\
		else							\
			IO_SOCKET_ERROR_ARG(gethostbyname, Host);	\
	}								\
} while (0)
#endif /* HAVE_STRUCT_SOCKADDR_UN_SUN_LEN */

static int
socket_bind(const char *host, int port, int domain, int fd)
{
	struct sockaddr *addr;
	socklen_t len;

	if (host == NULL)
		return (-1);
	socket_unlink(host);
	IO_SOCKADDR(addr, host, port, domain, len);
	return (bind(fd, addr, len));
}

static int
socket_accept(int domain, int fd)
{
	struct sockaddr_un sun;
	struct sockaddr_in sin;
	struct sockaddr *addr;
	socklen_t len;

	len = 0;
	if (domain == AF_UNIX)
		addr = (struct sockaddr *)&sun;
	else
		addr = (struct sockaddr *)&sin;
	return (accept(fd, addr, &len));
}

static int
socket_connect(const char *host, int port, int domain, int fd)
{
	struct sockaddr *addr;
	socklen_t len;

	if (host == NULL)
		return (-1);
	IO_SOCKADDR(addr, host, port, domain, len);
	return (connect(fd, addr, len));
}

static int
socket_open(int domain, int type)
{
	if (domain < AF_UNSPEC || domain > AF_MAX)
		domain = FTH_DEFAULT_ADDRFAM;
	if (type < SOCK_STREAM || type > SOCK_SEQPACKET)
		type = SOCK_DGRAM;
	return (socket(domain, type, 0));
}

static FTH
make_socket_io(const char *host, int port, int domain, int fd)
{
	FIO_Socket *s;
	FTH io;

	if (fd == -1) {
		fd = socket_open(domain, SOCK_STREAM);
		if (fd == -1) {
			IO_SOCKET_ERROR_ARG(socket, host);
			/* NOTREACHED */
			return (FTH_FALSE);
		}
		if (socket_connect(host, port, domain, fd) == -1) {
			close(fd);
			IO_SOCKET_ERROR_ARG(connect, host);
			/* NOTREACHED */
			return (FTH_FALSE);
		}
	}
	s = FTH_MALLOC(sizeof(FIO_Socket));
	s->fd = fd;
	s->host = fth_make_string(host);
	io = make_io_base(FICL_FAM_READ | FICL_FAM_WRITE);
	FTH_IO_TYPE(io) = FTH_IO_SOCKET;
	FTH_IO_FILENAME(io) = s->host;
	FTH_IO_NAME(io) = fth_make_string("socket");
	FTH_IO_DATA(io) = (void *)s;
	FTH_IO_OBJECT(io)->read_char = socket_read_char;
	FTH_IO_OBJECT(io)->write_char = socket_write_char;
	FTH_IO_OBJECT(io)->read_line = socket_read_line;
	FTH_IO_OBJECT(io)->write_line = socket_write_line;
	FTH_IO_OBJECT(io)->eof_p = socket_eof_p;
	FTH_IO_OBJECT(io)->tell = socket_tell;
	FTH_IO_OBJECT(io)->seek = socket_seek;
	FTH_IO_OBJECT(io)->rewind = socket_rewind;
	FTH_IO_OBJECT(io)->close = socket_close;
	return (io);
}

#else				/* !HAVE_SOCKET */

static FTH
make_socket_io(const char *host, int port, int domain, int fd)
{
	FTH io;

	io = make_io_base(FICL_FAM_READ | FICL_FAM_WRITE);
	FTH_IO_FILENAME(io) = fth_make_string(host);
	FTH_IO_TYPE(io) = FTH_IO_SOCKET;
	FTH_IO_NAME(io) = fth_make_string("socket");
	return (io);
}

#endif				/* HAVE_SOCKET */

FTH
fth_io_nopen(const char *host, int port, int domain)
{
	return (make_socket_io(host, port, domain, -1));
}

#if HAVE_SOCKET

#define h_socket_open_info "\
HOST is a host name (AF_INET) or a path name (AF_UNIX).  \
If HOST is not a string, \"localhost\" will be used.  \
PORT is the connection port (default 1024) if DOMAIN is AF_INET, \
otherwise unused, \
and DOMAIN can be one of AF_INET (default) or AF_UNIX.  \
The socket is opend with DOMAIN, \
hardcoded second argument SOCK_STREAM, \
and hardcoded third argument of 0.\n\
See socket(2) and connect(2) for more information."

static void
ficl_io_nopen(ficlVm *vm)
{
#define h_io_nopen "( host :key port 1024 domain AF_INET -- io )  open socket\n\
\"localhost\" :port 25 io-nopen value io\n\
io io-read => \"220 fth-devel.net ESMTP Sendmail ...\"\n\
io \"HELP\\r\\n\" io-write\n\
io io-read => \"... (sendmail help output)\"\n\
io io-close\n\
\"ftp.freebsd.org\" :port 21 io-nopen to io\n\
io io-read\n\
  => \"220 ftp.beastie.tdk.net FTP server (Version 6.00LS) ready.\"\n\
io \"HELP\\r\\n\" io-write\n\
io io-read => \"... (ftpd help output)\"\n\
io io-close\n\
Connect to an already established server and return new IO object.  \
Raise an SOCKET-ERROR exception if an error occured.\n\
" h_socket_open_info
	int domain, port;
	FTH fs, io;

	domain = fth_get_optkey_fix(FTH_KEYWORD_DOMAIN, FTH_DEFAULT_ADDRFAM);
	port = fth_get_optkey_fix(FTH_KEYWORD_PORT, FTH_DEFAULT_PORT);
	FTH_STACK_CHECK(vm, 1, 1);
	fs = fth_pop_ficl_cell(vm);
	io = fth_io_nopen(IO_STRING_REF(fs), port, domain);
	ficlStackPushFTH(vm->dataStack, io);
}

/*
  : connection-handler { io -- }
    begin
      io io-read ( line ) dup
    while
        \ real work here
        ( line ) .$ cr
    repeat
    ( line ) drop
    io io-close
  ;

  : io-server <{ proc host :key port 1024 domain AF_INET -- }>
    domain SOCK_STREAM net-socket { fd }
    fd host port domain net-bind
    fd net-listen
    $" === stop with: %d SIGHUP kill\n" #( getpid ) fth-print
    begin
      fd host domain net-accept ( io ) proc fork
    again
  ;
  <'> connection-handler "localhost" io-server


  : io-client <{ host :key port 1024 domain AF_INET -- io }>
    domain SOCK_STREAM net-socket { fd }
    fd host port domain net-bind
    fd host port domain net-connect ( io )
  ;
*/

static void
ficl_socket(ficlVm *vm)
{
#define h_socket "( domain type -- fd )  return socket descriptor\n\
AF_INET SOCK_STREAM net-socket value fd\n\
fd \"localhost\" 1024 AF_INET net-bind\n\
fd net-listen\n\
fd \"localhost\" AF_INET net-accept value io\n\
Return socket descriptor.  \
DOMAIN can be AF_INET or AF_UNIX.  \
TYPE can be SOCK_STREAM or SOCK_DGRAM.  \
The third argument to socket is 0 and can't be set by the user.  \
Raise SOCKET-ERROR exception if socket fails.\n\
See socket(2) for more information."
	int fd, domain, type;

	FTH_STACK_CHECK(vm, 2, 1);
	type = (int)ficlStackPopInteger(vm->dataStack);
	domain = (int)ficlStackPopInteger(vm->dataStack);
	fd = socket_open(domain, type);
	if (fd == -1) {
		IO_SOCKET_ERROR(socket);
		/* NOTREACHED */
		return;
	}
	ficlStackPushInteger(vm->dataStack, (ficlInteger)fd);
}

static void
ficl_bind(ficlVm *vm)
{
#define h_bind "( fd host port domain -- )  bind to FD\n\
AF_INET SOCK_STREAM net-socket value fd\n\
fd \"localhost\" 1024 AF_INET net-bind\n\
fd net-listen\n\
fd \"localhost\" AF_INET net-accept value io\n\
Assign a name to a socket.  \
FD is the socket descriptor, \
HOST is a host name (AF_INET) or a path name (AF_UNIX), \
PORT is the port if DOMAIN is AF_INET, otherwise unused, \
and DOMAIN is one of AF_INET or AF_UNIX.  \
This is used on the server side of a socket connection.  \
Raise SOCKET-ERROR exception if bind fails.\n\
See bind(2) for more information."
	FTH fs;
	char *host;
	int fd, domain, port;

	FTH_STACK_CHECK(vm, 4, 0);
	domain = (int)ficlStackPopInteger(vm->dataStack);
	port = (int)ficlStackPopInteger(vm->dataStack);
	fs = fth_pop_ficl_cell(vm);
	fd = (int)ficlStackPopInteger(vm->dataStack);
	host = IO_STRING_REF(fs);
	if (socket_bind(host, port, domain, fd) == -1) {
		close(fd);
		IO_SOCKET_ERROR_ARG(bind, host);
	}
}

static void
ficl_listen(ficlVm *vm)
{
#define h_listen "( fd -- )  listen on socket FD\n\
AF_INET SOCK_STREAM net-socket value fd\n\
fd \"localhost\" 1024 AF_INET net-bind\n\
fd net-listen\n\
fd \"localhost\" AF_INET net-accept value io\n\
Inform the operating system that connection requests should be delivered.  \
FD is the previously opened socket descriptor.  \
This is used on the server side of a socket connection.  \
Raise SOCKET-ERROR exception if listen fails.\n\
See listen(2) for more information."
	int fd;

	FTH_STACK_CHECK(vm, 1, 0);
	fd = (int)ficlStackPopInteger(vm->dataStack);
	if (listen(fd, -1) == -1)
		IO_SOCKET_ERROR(listen);
}

static void
ficl_shutdown(ficlVm *vm)
{
#define h_shutdown "( fd how -- )  close socket connection\n\
AF_INET SOCK_STREAM net-socket value fd\n\
fd \"localhost\" 1024 AF_INET net-bind\n\
fd net-listen\n\
fd SHUT_RDWR net-shutdown\n\
Close socket connection.  \
FD is the socket descriptor and HOW is one of SHUT_RD, SHUT_WR or SHUT_RDWR.  \
Raise SOCKET-ERROR exception if shutdown fails.\n\
See shutdown(2) for more information."
	int fd, how;

	FTH_STACK_CHECK(vm, 2, 0);
	how = (int)ficlStackPopInteger(vm->dataStack);
	fd = (int)ficlStackPopInteger(vm->dataStack);
	if (shutdown(fd, how) == -1)
		IO_SOCKET_ERROR(shutdown);
}

static void
ficl_accept(ficlVm *vm)
{
#define h_accept "( fd host domain -- io )  accept socket and return IO\n\
AF_INET SOCK_STREAM net-socket value fd\n\
fd \"localhost\" 1024 AF_INET net-bind\n\
fd net-listen\n\
fd \"localhost\" AF_INET net-accept value io\n\
Accept a connection on a socket and return an IO object \
after established connection.  \
FD is the socket descriptor, \
HOST can be an arbitrary name, \
its only use is the name for the IO object, \
DOMAIN is one of AF_INET or AF_UNIX.  \
This is used on the server side of a socket connection.  \
Raise SOCKET-ERROR exception if accept fails.\n\
See accept(2) for more information."
	FTH fs, io;
	int fd, nd, domain;

	FTH_STACK_CHECK(vm, 3, 1);
	domain = (int)ficlStackPopInteger(vm->dataStack);
	fs = fth_pop_ficl_cell(vm);
	fd = (int)ficlStackPopInteger(vm->dataStack);
	nd = socket_accept(domain, fd);
	if (nd == -1) {
		close(fd);
		IO_SOCKET_ERROR(accept);
		/* NOTREACHED */
		return;
	}
	io = make_socket_io(IO_STRING_REF(fs), -1, -1, nd);
	ficlStackPushFTH(vm->dataStack, io);
}

static void
ficl_connect(ficlVm *vm)
{
#define h_connect "( fd host port domain -- io )  connect to socket\n\
AF_INET SOCK_STREAM net-socket value fd\n\
fd \"localhost\" 1024 AF_INET net-bind\n\
fd \"localhost\" 1024 AF_INET net-connect value io\n\
Connect to a server and return IO object after an established connection.  \
FD is the socket descriptor, \
HOST is a host name (AF_INET) or a name (AF_UNIX), \
PORT is the port if DOMAIN is AF_INET, otherwise unused, \
and DOMAIN is one of AF_INET or AF_UNIX.  \
This is used on the client side of a socket connection.  \
Raise SOCKET-ERROR exception if connect fails.\n\
See connet(2) for more information."
	FTH fs, io;
	char *host;
	int domain, fd, port;

	FTH_STACK_CHECK(vm, 4, 1);
	domain = (int)ficlStackPopInteger(vm->dataStack);
	port = (int)ficlStackPopInteger(vm->dataStack);
	fs = fth_pop_ficl_cell(vm);
	fd = (int)ficlStackPopInteger(vm->dataStack);
	host = IO_STRING_REF(fs);
	if (socket_connect(host, port, domain, fd) == -1) {
		close(fd);
		IO_SOCKET_ERROR_ARG(connect, host);
		/* NOTREACHED */
		return;
	}
	io = make_socket_io(host, -1, -1, fd);
	ficlStackPushFTH(vm->dataStack, io);
}

static void
ficl_send(ficlVm *vm)
{
#define h_send "( fd msg flags -- )  send data\n\
fd \"data message\" 0 net-send\n\
Send data via a socket.  \
FD is the socket descriptor, \
MSG is the data to send \
and FLAGS can be 0, MSG_PEEK or MSG_OOB.  \
Raise SOCKET-ERROR exception if send fails.\n\
See send(2) for more information."
	int fd, flags;
	size_t size;
	FTH msg;
	char *text;

	FTH_STACK_CHECK(vm, 3, 0);
	flags = (int)ficlStackPopInteger(vm->dataStack);
	msg = fth_pop_ficl_cell(vm);
	fd = (int)ficlStackPopInteger(vm->dataStack);
	size = (size_t)fth_string_length(msg);
	text = IO_STRING_REF(msg);
	if (send(fd, text, size, flags) == -1)
		IO_SOCKET_ERROR_ARG(send, text);
}

static void
ficl_recv(ficlVm *vm)
{
#define h_recv "( fd flags -- msg )  receive data\n\
fd 0 net-recv => \"data message ...\"\n\
Receive data via a socket and return it.  \
FD is the socket descriptor \
and FLAGS can be 0, MSG_PEEK, or MSG_OOB.  \
Raise SOCKET-ERROR exception if recv fails.\n\
See recv(2) for more information."
	FTH fs;
	int fd, flags;
	ssize_t len;

	FTH_STACK_CHECK(vm, 2, 1);
	flags = (int)ficlStackPopInteger(vm->dataStack);
	fd = (int)ficlStackPopInteger(vm->dataStack);
	len = recv(fd, vm->pad, sizeof(vm->pad), flags);
	if (len == -1) {
		IO_SOCKET_ERROR(recv);
		/* NOTREACHED */
		return;
	}
	fs = fth_make_string_len(vm->pad, (ficlInteger)len);
	ficlStackPushFTH(vm->dataStack, fs);
}

static void
ficl_sendto(ficlVm *vm)
{
#define h_sendto "( fd msg flags host port domain -- )  send data\n\
fd \"data message\" \"localhost\" 1024 AF_INET net-sendto\n\
Send data via a socket.  \
FD is the socket descriptor, \
MSG is the data to send, \
FLAGS can be 0, MSG_PEEK, or MSG_OOB.  \
HOST is a host name (AF_INET) or a name (AF_UNIX), \
PORT is the port if DOMAIN is AF_INET, otherwise unused, \
and DOMAIN is one of AF_INET or AF_UNIX.  \
Raise SOCKET-ERROR exception if sendto fails.\n\
See sendto(2) for more information."
	FTH fhost, fmsg;
	int fd, flags, domain;
	char *host, *msg;
	ficlUnsigned16 port;
	struct sockaddr *addr;
	socklen_t len = 0;

	FTH_STACK_CHECK(vm, 6, 0);
	domain = (int)ficlStackPopInteger(vm->dataStack);
	port = (ficlUnsigned16)ficlStackPopUnsigned(vm->dataStack);
	fhost = fth_pop_ficl_cell(vm);
	flags = (int)ficlStackPopInteger(vm->dataStack);
	fmsg = fth_pop_ficl_cell(vm);
	fd = (int)ficlStackPopInteger(vm->dataStack);
	host = IO_STRING_REF(fhost);
	msg = IO_STRING_REF(fmsg);
	IO_SOCKADDR(addr, host, port, domain, len);
	if (sendto(fd, msg, fth_strlen(msg), flags, addr, len) == -1)
		IO_SOCKET_ERROR_ARG(sendto, host);
}

static void
ficl_recvfrom(ficlVm *vm)
{
#define h_recvfrom "( fd flags host port domain -- msg )  receive data\n\
fd 0 \"localhost\" 1024 AF_INET net-recvfrom => \"data message\"\n\
Receive data via a socket and return it.  \
FD is the socket descriptor, \
FLAGS can be 0, MSG_PEEK, or MSG_OOB.  \
HOST is a host name (AF_INET) or a path name (AF_UNIX), \
PORT is the port if DOMAIN is AF_INET, otherwise unused, \
and DOMAIN is one of AF_INET or AF_UNIX.  \
Raise SOCKET-ERROR exception if recvfrom fails.\n\
See recvfrom(2) for more information."
	FTH fhost, fs;
	int fd, flags, domain;
	ficlUnsigned16 port;
	char *host;
	struct sockaddr *addr;
	socklen_t len = 0;

	FTH_STACK_CHECK(vm, 5, 1);
	domain = (int)ficlStackPopInteger(vm->dataStack);
	port = (ficlUnsigned16)ficlStackPopUnsigned(vm->dataStack);
	fhost = fth_pop_ficl_cell(vm);
	flags = (int)ficlStackPopInteger(vm->dataStack);
	fd = (int)ficlStackPopInteger(vm->dataStack);
	host = IO_STRING_REF(fhost);
	IO_SOCKADDR(addr, host, port, domain, len);
	if (recvfrom(fd, vm->pad, sizeof(vm->pad), flags, addr, &len) == -1) {
		IO_SOCKET_ERROR_ARG(recvfrom, host);
		/* NOTREACHED */
		return;
	}
	fs = fth_make_string_len(vm->pad, (ficlInteger)len);
	ficlStackPushFTH(vm->dataStack, fs);
}

#endif				/* HAVE_SOCKET */

static int	fth_exit_status = 0;

int
fth_set_exit_status(int status)
{
	if (WIFEXITED(status))
		fth_exit_status = (int)WEXITSTATUS(status);
	else
		fth_exit_status = -1;
	return (fth_exit_status);
}

static void
ficl_exit_status(ficlVm *vm)
{
#define h_exit_status "( -- n )  return exit-status\n\
\"ls -lAF\" file-system drop\n\
exit-status => 0\n\
\"lx\" file-system drop\n\
exit-status => 127\n\
Return exit status of last extern process \
called via file-shell, file-system, etc."
	ficlStackPushInteger(vm->dataStack, (ficlInteger)fth_exit_status);
}

void
fth_io_close(FTH io)
{
	if (FTH_NOT_FALSE_P(io) && FTH_IO_P(io)) {
		if (!FTH_IO_CLOSED_P(io))
			FTH_IO_CLOSE(io);
		FTH_IO_CLOSED_P(io) = true;
		return;
	}
	IO_ASSERT_IO(io);
}

static void
ficl_io_close(ficlVm *vm)
{
#define h_io_close "( io -- )  close IO\n\
\"test\" io-open value io\n\
io io-close\n\
If necessary, flush IO, close IO object and set closed? to #t."
	FTH_STACK_CHECK(vm, 1, 0);
	fth_io_close(fth_pop_ficl_cell(vm));
}

static void
ficl_print_io(ficlVm *vm)
{
#define h_print_io "( io -- )  print IO object\n\
\"test\" io-open value io\n\
io .io => #<io[0]: \"test\", file-input>\n\
Print IO object to current output."
	FTH io;

	FTH_STACK_CHECK(vm, 1, 0);
	io = fth_pop_ficl_cell(vm);
	IO_ASSERT_IO(io);
	fth_print(fth_string_ref(io_to_string(io)));
}

bool
fth_io_equal_p(FTH obj1, FTH obj2)
{
	if (FTH_IO_P(obj1) && FTH_IO_P(obj2))
		return (FTH_TO_BOOL(io_equal_p(obj1, obj2)));
	return (false);
}

static void
ficl_io_equal_p(ficlVm *vm)
{
#define h_io_equal_p "( obj1 obj2 -- f )  compare OBJ1 with OBJ2\n\
\"test\" io-open value io1\n\
\"test\" io-open value io2\n\
\"foo\"  io-open value io3\n\
io1 io1 io= => #t\n\
io1 io2 io= => #t\n\
io1 io3 io= => #f\n\
Return #t if OBJ1 and OBJ2 are IO objects with equal file names, \
modes and file positions, otherwise #f."
	FTH obj1, obj2;

	FTH_STACK_CHECK(vm, 2, 1);
	obj2 = fth_pop_ficl_cell(vm);
	obj1 = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, fth_io_equal_p(obj1, obj2));
}

int
fth_io_getc(FTH io)
{
	IO_ASSERT_IO_INPUT(io);
	return (FTH_IO_READ_CHAR(io));
}

static void
ficl_io_getc(ficlVm *vm)
{
#define h_io_getc "( io -- c|#f )  return next character from IO\n\
\"test\" io-open value io\n\
io io-getc => 104\n\
io io-getc => 101\n\
io io-close\n\
Return next character from IO object or #f if EOF.\n\
See also io-read."
	FTH io;
	int c;

	FTH_STACK_CHECK(vm, 1, 1);
	io = fth_pop_ficl_cell(vm);
	c = fth_io_getc(io);
	fth_push_ficl_cell(vm, (c == EOF) ? FTH_FALSE : CHAR_TO_FTH(c));
}

void
fth_io_putc(FTH io, int c)
{
	IO_ASSERT_IO_OUTPUT(io);
	FTH_IO_WRITE_CHAR(io, c);
	FTH_INSTANCE_CHANGED(io);
}

static void
ficl_io_putc(ficlVm *vm)
{
#define h_io_putc "( io c -- )  write character C to IO\n\
\"test\" io-open-write value io\n\
io <char> a io-putc\n\
io <char> b io-putc\n\
io <char> c io-putc\n\
io io-close\n\
Write character C to IO object.\n\
See also io-write, io-write-format."
	FTH ch, io;

	FTH_STACK_CHECK(vm, 2, 0);
	ch = fth_pop_ficl_cell(vm);
	io = fth_pop_ficl_cell(vm);
	FTH_ASSERT_ARGS(FTH_INTEGER_P(ch), ch, FTH_ARG2, "a char");
	fth_io_putc(io, FTH_TO_CHAR(ch));
}

char *
fth_io_read(FTH io)
{
	IO_ASSERT_IO_INPUT(io);
	return (FTH_IO_READ_LINE(io));
}

/* For object-apply. */
FTH
fth_io_read_line(FTH io)
{
#define h_io_read "( io -- line|#f )  read next line from IO\n\
\"test\" io-open value io\n\
io io-read => \"content of entire line\"\n\
io io-close\n\
Return next line from IO object or #f if EOF.\n\
See also io-getc."
	char *line;

	line = fth_io_read(io);
	if (line == NULL)
		return (FTH_FALSE);
	return (fth_make_string(line));
}

void
fth_io_write(FTH io, const char *line)
{
	IO_ASSERT_IO_OUTPUT(io);
	FTH_IO_WRITE_LINE(io, line);
	FTH_INSTANCE_CHANGED(io);
}

void
fth_io_write_and_flush(FTH io, const char *line)
{
	fth_io_write(io, line);
	FTH_IO_FLUSH(io);
}

static void
ficl_io_write(ficlVm *vm)
{
#define h_io_write "( io line -- )  write LINE to IO\n\
\"test\" io-open-write value io\n\
io \"content of entire line\" io-write\n\
io io-close\n\
Write LINE to IO object.\n\
See also io-putc, io-write-format."
	FTH io, fs;

	FTH_STACK_CHECK(vm, 2, 0);
	fs = fth_pop_ficl_cell(vm);
	io = fth_pop_ficl_cell(vm);
	fth_io_write(io, IO_STRING_REF(fs));
}

void
fth_io_write_format(FTH io, FTH fmt, FTH args)
{
	fth_io_write(io, IO_STRING_REF(fth_string_format(fmt, args)));
}

static void
ficl_io_write_format(ficlVm *vm)
{
#define h_io_write_format "( io fmt fmt-args -- )  write formatted line\n\
\"test\" io-open-write value io\n\
io  \"%d %d + .\\n\"  #( 10 20 )  io-write-format\n\
io  \"%d %d + .\\n\"  #( 10 20 )  string-format  io-write\n\
io io-close\n\
Write string built from FMT and array FMT-ARGS to IO object.\n\
See also io-putc, io-write."
	FTH io, fmt, args;

	FTH_STACK_CHECK(vm, 3, 0);
	args = fth_pop_ficl_cell(vm);
	fmt = fth_pop_ficl_cell(vm);
	io = fth_pop_ficl_cell(vm);
	fth_io_write_format(io, fmt, args);
}

FTH
fth_io_readlines(FTH io)
{
#define h_io_rlns "( io -- array-of-lines )  return content as array\n\
\"test\" io-open value io\n\
io io-readlines => #( \"1st line\\n\" \"2nd line\\n\" ...)\n\
io io-close\n\
Return the entire IO object content as an array of strings, line by line.\n\
See also io-getc, io-read."
	FTH array;
	char *line;
	ficl2Integer pos;

	IO_ASSERT_IO_NOT_CLOSED(io);
	array = fth_make_empty_array();
	pos = FTH_IO_TELL(io);
	FTH_IO_REWIND(io);
	while ((line = FTH_IO_READ_LINE(io)) != NULL)
		fth_array_push(array, fth_make_string(line));
	FTH_IO_SEEK(io, pos, SEEK_SET);
	return (array);
}

void
fth_io_writelines(FTH io, FTH array)
{
#define h_io_wlns "( io array-of-lines -- )  write array of strings\n\
\"test\" io-open-write value io\n\
io #( \"1st line\\n\" \"2nd line\\n\" ...) io-writelines\n\
io io-close\n\
Write ARRAY-OF-LINES to IO object.\n\
See also io-putc, io-write, io-write-format."
	FTH fs;
	ficlInteger i, len;
	ficl2Integer pos;

	IO_ASSERT_IO_OUTPUT(io);
	FTH_ASSERT_ARGS(FTH_ARRAY_P(array), array, FTH_ARG2, "an array");
	pos = FTH_IO_TELL(io);
	FTH_IO_REWIND(io);
	len = fth_array_length(array);
	for (i = 0; i < len; i++) {
		fs = fth_array_fast_ref(array, i);
		FTH_IO_WRITE_LINE(io, IO_STRING_REF(fs));
	}
	FTH_INSTANCE_CHANGED(io);
	FTH_IO_SEEK(io, pos, SEEK_SET);
}

bool
fth_io_eof_p(FTH io)
{
	IO_ASSERT_IO(io);
	return (FTH_IO_EOF_P(io));
}

static void
ficl_io_eof_p(ficlVm *vm)
{
#define h_io_eof_p "( io -- f )  test if IO reached EOF\n\
\"test\" io-open value io\n\
io io-eof? => #f\n\
io io-readlines => #( \"1st line\\n\" \"2nd line\\n\" ...)\n\
io io-eof? => #t\n\
io io-close\n\
Return #t if EOF is reached, otherwise #f."
	FTH io;
	bool flag;

	FTH_STACK_CHECK(vm, 1, 1);
	io = fth_pop_ficl_cell(vm);
	flag = fth_io_eof_p(io);
	ficlStackPushBoolean(vm->dataStack, flag);
}

char *
fth_io_filename(FTH io)
{
	IO_ASSERT_IO(io);
	return (IO_STRING_REF(FTH_IO_FILENAME(io)));
}

void *
fth_io_ptr(FTH io)
{
	IO_ASSERT_IO(io);
	return (FTH_IO_DATA(io));
}

int
fth_io_mode(FTH io)
{
	IO_ASSERT_IO(io);
	return (FTH_IO_FAM(io));
}

int
fth_io_fileno(FTH io)
{
	IO_ASSERT_IO(io);
	switch (FTH_IO_TYPE(io)) {
	case FTH_IO_FILE:
	case FTH_IO_PIPE:
		return (fileno((FILE *)FTH_IO_DATA(io)));
		break;
	case FTH_IO_SOCKET:
		return (FTH_IO_SOCKET_FD(FTH_IO_DATA(io)));
		break;
	default:
		return (-1);
		break;
	}
}

static bool
seek_constant_p(int whence)
{
	switch (whence) {
	case SEEK_SET:
	case SEEK_CUR:
	case SEEK_END:
		return (true);
		break;
	}
	return (false);
}

static void
ficl_io_seek(ficlVm *vm)
{
#define h_io_seek "( io offset :key whence io-seek-set -- pos )  set pos\n\
\"test\" io-open value io\n\
io 10 io-seek => 10\n\
io 10 :whence SEEK_CUR io-seek => 20\n\
io io-close\n\
Add OFFSET to the file position in IO object and return new position.  \
Keyword WHENCE can have the following values:\n\
SEEK_SET  --  offset counts from begin of file (default)\n\
SEEK_CUR  --  offset counts from current position\n\
SEEK_END  --  offset counts from end of file."
	FTH io, pos;
	int whence;
	ficl2Integer where;

	whence = fth_get_optkey_fix(FTH_KEYWORD_WHENCE, SEEK_SET);
	FTH_STACK_CHECK(vm, 2, 1);
	pos = fth_pop_ficl_cell(vm);
	io = fth_pop_ficl_cell(vm);
	IO_ASSERT_IO(io);
	FTH_ASSERT_ARGS(FTH_INTEGER_P(pos), pos, FTH_ARG2, "an integer");
	FTH_ASSERT_ARGS(seek_constant_p(whence), INT_TO_FIX(whence), FTH_ARG3,
	    "one of SEEK_SET, SEEK_CUR, SEEK_END");
	where = FTH_IO_SEEK(io, fth_long_long_ref(pos), whence);
	ficlStackPush2Integer(vm->dataStack, where);
}

ficl2Integer
fth_io_pos_ref(FTH io)
{
	IO_ASSERT_IO(io);
	return (FTH_IO_TELL(io));
}

static void
ficl_io_pos_ref(ficlVm *vm)
{
#define h_io_pos_ref "( io -- pos )  return file position\n\
\"test\" io-open value io\n\
io io-pos-ref => 0\n\
io io-close\n\
Return current IO object position.\n\
See also io-pos-set!, io-seek and io-rewind."
	FTH io;
	ficl2Integer pos;

	FTH_STACK_CHECK(vm, 1, 1);
	io = fth_pop_ficl_cell(vm);
	pos = fth_io_pos_ref(io);
	ficlStackPush2Integer(vm->dataStack, pos);
}

void
fth_io_pos_set(FTH io, ficl2Integer pos)
{
	IO_ASSERT_IO(io);
	FTH_IO_SEEK(io, pos, SEEK_SET);
}

static void
ficl_io_pos_set(ficlVm *vm)
{
#define h_io_pos_set "( io pos -- )  set file position\n\
\"test\" io-open value io\n\
io 10 io-pos-set!\n\
io io-close\n\
Set IO object position to POS.\n\
See also io-pos-ref, io-seek and io-rewind."
	FTH io;
	ficl2Integer pos;

	FTH_STACK_CHECK(vm, 2, 0);
	pos = ficlStackPop2Integer(vm->dataStack);
	io = fth_pop_ficl_cell(vm);
	fth_io_pos_set(io, pos);
}

void
fth_io_rewind(FTH io)
{
	IO_ASSERT_IO(io);
	FTH_IO_REWIND(io);
}

static void
ficl_io_rewind(ficlVm *vm)
{
#define h_io_rewind "( io -- )  set file position to 0\n\
\"test\" io-open value io\n\
io io-readlines => #( \"...\" ...)\n\
io io-rewind\n\
io io-pos-ref => 0\n\
io io-close\n\
Rewind position to begin of IO object.\n\
See also io-pos-ref, io-pos-set!, io-seek."
	FTH io;

	FTH_STACK_CHECK(vm, 1, 0);
	io = fth_pop_ficl_cell(vm);
	fth_io_rewind(io);
}

FTH
fth_readlines(const char *name)
{
	FTH io, array;

	io = fth_io_open(name, FICL_FAM_READ);
	array = fth_io_readlines(io);
	fth_io_close(io);
	return (array);
}

static void
ficl_readlines(ficlVm *vm)
{
#define h_readlines "( fname -- array-of-lines )  content as array\n\
\"test\" readlines => #( \"1st line\\n\" \"2nd line\\n\" ...)\n\
Open file FNAME, read its content in an array, \
close file and return the array."
	FTH fs, array;

	FTH_STACK_CHECK(vm, 1, 1);
	fs = fth_pop_ficl_cell(vm);
	array = fth_readlines(IO_STRING_REF(fs));
	ficlStackPushFTH(vm->dataStack, array);
}

void
fth_writelines(const char *name, FTH array)
{
	FTH io;

	FTH_ASSERT_ARGS(FTH_ARRAY_P(array), array, FTH_ARG2, "an array");
	io = fth_io_open(name, FICL_FAM_WRITE);
	fth_io_writelines(io, array);
	fth_io_close(io);
}

static void
ficl_writelines(ficlVm *vm)
{
#define h_writelines "( fname array-of-lines -- )  write array\n\
\"test\" #( \"1st line\\n\" \"2nd line\\n\" ) writelines\n\
Open file FNAME, write the content of ARRAY-OF-LINES to it and close file."
	FTH array, fs;

	FTH_STACK_CHECK(vm, 2, 1);
	array = fth_pop_ficl_cell(vm);
	fs = fth_pop_ficl_cell(vm);
	fth_writelines(IO_STRING_REF(fs), array);
}

void
fth_io_flush(FTH io)
{
	IO_ASSERT_IO_NOT_CLOSED(io);
	FTH_IO_FLUSH(io);
}

static void
ficl_io_flush(ficlVm *vm)
{
#define h_io_flush "( io -- )  flush IO\n\
\"test\" io-open-write value io\n\
io #( \"1st line\\n\" \"2nd line\\n\" ...) io-writelines\n\
io io-flush\n\
io io-close\n\
Flushe IO object if possible."
	FTH io;

	FTH_STACK_CHECK(vm, 1, 0);
	io = fth_pop_ficl_cell(vm);
	fth_io_flush(io);
}

FTH
fth_set_io_stdin(FTH io)
{
	FTH old_io;

	if (!IO_INPUT_P(io))
		return (FTH_FALSE);
	old_io = ficlVmGetPortIn(FTH_FICL_VM());
	ficlVmGetPortIn(FTH_FICL_VM()) = io;
	ficlVmGetStdin(FTH_FICL_VM()) = FTH_IO_DATA(io);
	ficlVmGetFilenoIn(FTH_FICL_VM()) = fileno((FILE *)FTH_IO_DATA(io));
	return (old_io);
}

FTH
fth_set_io_stdout(FTH io)
{
	FTH old_io;

	if (!IO_OUTPUT_P(io))
		return (FTH_FALSE);
	old_io = ficlVmGetPortOut(FTH_FICL_VM());
	ficlVmGetPortOut(FTH_FICL_VM()) = io;
	ficlVmGetStdout(FTH_FICL_VM()) = FTH_IO_DATA(io);
	ficlVmGetFilenoOut(FTH_FICL_VM()) = fileno((FILE *)FTH_IO_DATA(io));
	return (old_io);
}

FTH
fth_set_io_stderr(FTH io)
{
	FTH old_io;

	if (!IO_OUTPUT_P(io))
		return (FTH_FALSE);
	old_io = ficlVmGetPortErr(FTH_FICL_VM());
	ficlVmGetPortErr(FTH_FICL_VM()) = io;
	ficlVmGetStderr(FTH_FICL_VM()) = FTH_IO_DATA(io);
	ficlVmGetFilenoErr(FTH_FICL_VM()) = fileno((FILE *)FTH_IO_DATA(io));
	return (old_io);
}

static void
ficl_io_stdin(ficlVm *vm)
{
#define h_io_stdin "( -- stdin )  return stdin IO\n\
*stdin* #f io-reopen value new-stdin\n\
Return current standard input IO object.\n\
See also set-*stdin*, *stdout*, set-*stdout*, *stderr*, set-*stderr*."
	FTH_STACK_CHECK(vm, 0, 1);
	fth_push_ficl_cell(vm, ficlVmGetPortIn(vm));
}

static void
ficl_set_io_stdin(ficlVm *vm)
{
#define h_set_io_stdin "( io -- old )  set stdin to IO\n\
\"input-file\" io-open-read set-*stdin* value old-stdin\n\
Set IO to current standard input and return old IO object.\n\
See also *stdin*, *stdout*, set-*stdout*, *stderr*, set-*stderr*."
	FTH io;

	FTH_STACK_CHECK(vm, 1, 1);
	io = fth_pop_ficl_cell(vm);
	fth_push_ficl_cell(vm, fth_set_io_stdin(io));
}

static void
ficl_io_stdout(ficlVm *vm)
{
#define h_io_stdout "( -- stdout)  return stdout IO\n\
*stdout* \"stdout.log\" io-reopen value new-stdout\n\
Return current standard output IO object.\n\
See also *stdin*, set-*stdin*, set-*stdout*, *stderr*, set-*stderr*."
	FTH_STACK_CHECK(vm, 0, 1);
	fth_push_ficl_cell(vm, ficlVmGetPortOut(vm));
}

static void
ficl_set_io_stdout(ficlVm *vm)
{
#define h_set_io_stdout "( io -- old )  set stdout to IO\n\
\"stdout.log\" io-open-write set-*stdout* value old-stdout\n\
Set IO to current standard output and return old IO object.\n\
See also *stdin*, set-*stdin*, *stdout*, *stderr*, set-*stderr*."
	FTH io;

	FTH_STACK_CHECK(vm, 1, 1);
	io = fth_pop_ficl_cell(vm);
	fth_push_ficl_cell(vm, fth_set_io_stdout(io));
}

static void
ficl_io_stderr(ficlVm *vm)
{
#define h_io_stderr "( -- stderr )  return stderr IO\n\
*stderr* \"stderr.log\" io-reopen value new-stderr\n\
Return current standard error IO object.\n\
See also *stdin*, set-*stdin*, *stdout*, set-*stdout*, set-*stderr*."
	FTH_STACK_CHECK(vm, 0, 1);
	fth_push_ficl_cell(vm, ficlVmGetPortErr(vm));
}

static void
ficl_set_io_stderr(ficlVm *vm)
{
#define h_set_io_stderr "( io -- old )  set stderr to IO\n\
\"error.log\" io-open-write set-*stderr* value old-stderr\n\
Set IO to current standard error and return old IO object.\n\
See also *stdin*, set-*stdin*, *stdout*, set-*stdout*, *stderr*."
	FTH io;

	FTH_STACK_CHECK(vm, 1, 1);
	io = fth_pop_ficl_cell(vm);
	fth_push_ficl_cell(vm, fth_set_io_stderr(io));
}

void
init_io_type(void)
{
	/* init io */
	io_tag = make_object_type(FTH_STR_IO, FTH_IO_T);
	fth_set_object_inspect(io_tag, io_inspect);
	fth_set_object_to_string(io_tag, io_to_string);
	fth_set_object_to_array(io_tag, io_to_array);
	fth_set_object_value_ref(io_tag, io_ref);
	fth_set_object_equal_p(io_tag, io_equal_p);
	fth_set_object_length(io_tag, io_length);
	fth_set_object_mark(io_tag, io_mark);
	fth_set_object_free(io_tag, io_free);
}

void
init_io(void)
{
	char *vc;

	vc = fth_getenv("VERSION_CONTROL", NULL);
	if (vc != NULL) {
		if ((strncmp(vc, "t", 1L) == 0) ||
		    (strncmp(vc, "numbered", 8L) == 0))
			version_control = FTH_TRUE;
		else if ((strncmp(vc, "nil", 3L) == 0) ||
		    (strncmp(vc, "existing", 8L) == 0))
			version_control = FTH_NIL;
		else if ((strncmp(vc, "never", 5L) == 0) ||
		    (strncmp(vc, "simple", 6L) == 0))
			version_control = FTH_FALSE;
		else		/* "off" || "none" || any */
			version_control = FTH_UNDEF;
	} else
		version_control = FTH_UNDEF;
	fth_set_object_apply(io_tag, (void *)fth_io_read_line, 0, 0, 0);
	version_number_string = fth_gc_permanent(fth_make_string("~[0-9]+~$"));
	string_empty = fth_gc_permanent(fth_make_string(""));
	string_cr = fth_gc_permanent(fth_make_string("\n"));
	string_space = fth_gc_permanent(fth_make_string(" "));
	/* io */
	fth_set_io_stdin(make_file_io(stdin, "*stdin*", FICL_FAM_READ));
	fth_set_io_stdout(make_file_io(stdout, "*stdout*", FICL_FAM_WRITE));
	fth_set_io_stderr(make_file_io(stderr, "*stderr*", FICL_FAM_WRITE));
	FTH_PRI1("io-filename", ficl_io_filename, h_io_filename);
	FTH_PRI1("io-mode", ficl_io_mode, h_io_mode);
	FTH_PRI1("io-fileno", ficl_io_fileno, h_io_fileno);
	FTH_PROC("io->string", fth_io_to_string, 1, 0, 0, h_io_to_string);
	FTH_PRI1("io?", ficl_io_p, h_io_p);
	FTH_PRI1("io-input?", ficl_io_input_p, h_io_input_p);
	FTH_PRI1("io-output?", ficl_io_output_p, h_io_output_p);
	FTH_PRI1("io-closed?", ficl_io_closed_p, h_io_closed_p);
	FTH_PRI1("version-control", ficl_version_control, h_version_control);
	FTH_PRI1("set-version-control", ficl_set_version_control, h_set_vc);
	/* file */
	FTH_PRI1("io-open-file", ficl_io_open_file, h_io_open_file);
	FTH_PRI1("io-open-input-file", ficl_io_open_input_file, h_io_oi_file);
	FTH_PRI1("io-open-output-file", ficl_io_open_output_file, h_io_oo_file);
	FTH_PRI1("io-open", ficl_io_open, h_io_open);
	FTH_PRI1("make-file-port", ficl_io_open, h_io_open);
	FTH_PRI1("io-open-read", ficl_io_open_read, h_io_open_read);
	FTH_PRI1("make-file-input-port", ficl_io_open_read, h_io_open_read);
	FTH_PRI1("io-open-write", ficl_io_open_write, h_io_open_write);
	FTH_PRI1("make-file-output-port", ficl_io_open_write, h_io_open_write);
	FTH_PRI1("io-reopen", ficl_io_reopen, h_io_reopen);
	FTH_PRI1("io-fdopen", ficl_io_fdopen, h_io_fdopen);
	/* pipe */
	FTH_PRI1("io-popen", ficl_io_popen, h_io_popen);
	FTH_PRI1("make-pipe-port", ficl_io_popen, h_io_popen);
	FTH_PRI1("io-popen-read", ficl_io_popen_read, h_io_popen_read);
	FTH_PRI1("make-pipe-input-port", ficl_io_popen_read, h_io_popen_read);
	FTH_PRI1("io-popen-write", ficl_io_popen_write, h_io_powrite);
	FTH_PRI1("make-pipe-output-port", ficl_io_popen_write, h_io_powrite);
	/* io-string */
	FTH_PRI1("io-sopen", ficl_io_sopen, h_io_sopen);
	FTH_PRI1("make-string-port", ficl_io_sopen, h_io_sopen);
	FTH_PRI1("io-sopen-read", ficl_io_sopen_read, h_io_soread);
	FTH_PRI1("make-string-input-port", ficl_io_sopen_read, h_io_soread);
	FTH_PRI1("io-sopen-write", ficl_io_sopen_write, h_io_sowrite);
	FTH_PRI1("make-string-output-port", ficl_io_sopen_write, h_io_sowrite);
#if HAVE_SOCKET
	/* socket */
	FTH_PRI1("io-nopen", ficl_io_nopen, h_io_nopen);
	FTH_PRI1("make-socket-port", ficl_io_nopen, h_io_nopen);
	FTH_PRI1("net-socket", ficl_socket, h_socket);
	FTH_PRI1("net-bind", ficl_bind, h_bind);
	FTH_PRI1("net-listen", ficl_listen, h_listen);
	FTH_PRI1("net-shutdown", ficl_shutdown, h_shutdown);
	FTH_PRI1("net-accept", ficl_accept, h_accept);
	FTH_PRI1("net-connect", ficl_connect, h_connect);
	FTH_PRI1("net-send", ficl_send, h_send);
	FTH_PRI1("net-recv", ficl_recv, h_recv);
	FTH_PRI1("net-sendto", ficl_sendto, h_sendto);
	FTH_PRI1("net-recvfrom", ficl_recvfrom, h_recvfrom);
	FTH_ADD_FEATURE_AND_INFO(FTH_STR_SOCKET, h_list_of_io_socket_functions);
#endif				/* HAVE_SOCKET */
	/* io words */
	FTH_PRI1("io-exit-status", ficl_exit_status, h_exit_status);
	FTH_PRI1("exit-status", ficl_exit_status, h_exit_status);
	FTH_PRI1("io-close", ficl_io_close, h_io_close);
	FTH_PRI1(".io", ficl_print_io, h_print_io);
	FTH_PRI1("io=", ficl_io_equal_p, h_io_equal_p);
	FTH_PRI1("io-getc", ficl_io_getc, h_io_getc);
	FTH_PRI1("io-putc", ficl_io_putc, h_io_putc);
	FTH_PROC("io-read", fth_io_read_line, 1, 0, 0, h_io_read);
	FTH_PRI1("io-write", ficl_io_write, h_io_write);
	FTH_PRI1("io-write-format", ficl_io_write_format, h_io_write_format);
	FTH_PROC("io-readlines", fth_io_readlines, 1, 0, 0, h_io_rlns);
	FTH_VOID_PROC("io-writelines", fth_io_writelines, 2, 0, 0, h_io_wlns);
	FTH_PRI1("io-eof?", ficl_io_eof_p, h_io_eof_p);
	FTH_PRI1("io-seek", ficl_io_seek, h_io_seek);
	FTH_PRI1("io-tell", ficl_io_pos_ref, h_io_pos_ref);
	FTH_PRI1("io-pos-ref", ficl_io_pos_ref, h_io_pos_ref);
	FTH_PRI1("io-pos-set!", ficl_io_pos_set, h_io_pos_set);
	FTH_PRI1("io-rewind", ficl_io_rewind, h_io_rewind);
	FTH_PRI1("readlines", ficl_readlines, h_readlines);
	FTH_PRI1("writelines", ficl_writelines, h_writelines);
	FTH_PRI1("io-flush", ficl_io_flush, h_io_flush);
	FTH_PRI1("*stdin*", ficl_io_stdin, h_io_stdin);
	FTH_PRI1("set-*stdin*", ficl_set_io_stdin, h_set_io_stdin);
	FTH_PRI1("*stdout*", ficl_io_stdout, h_io_stdout);
	FTH_PRI1("set-*stdout*", ficl_set_io_stdout, h_set_io_stdout);
	FTH_PRI1("*stderr*", ficl_io_stderr, h_io_stderr);
	FTH_PRI1("set-*stderr*", ficl_set_io_stderr, h_set_io_stderr);
	FTH_ADD_FEATURE_AND_INFO(FTH_STR_IO, h_list_of_io_functions);
	FTH_SET_CONSTANT(SEEK_SET);
	FTH_SET_CONSTANT(SEEK_CUR);
	FTH_SET_CONSTANT(SEEK_END);
#if defined(AF_UNSPEC)
	FTH_SET_CONSTANT(AF_UNSPEC);
#endif
#if defined(AF_LOCAL)
	FTH_SET_CONSTANT(AF_LOCAL);
#endif
#if defined(AF_UNIX)
	FTH_SET_CONSTANT(AF_UNIX);
#endif
#if defined(AF_INET)
	FTH_SET_CONSTANT(AF_INET);
#endif
#if defined(AF_PUP)
	FTH_SET_CONSTANT(AF_PUP);
#endif
#if defined(AF_APPLETALK)
	FTH_SET_CONSTANT(AF_APPLETALK);
#endif
#if defined(AF_ROUTE)
	FTH_SET_CONSTANT(AF_ROUTE);
#endif
#if defined(AF_LINK)
	FTH_SET_CONSTANT(AF_LINK);
#endif
#if defined(AF_IPX)
	FTH_SET_CONSTANT(AF_IPX);
#endif
#if defined(AF_SIP)
	FTH_SET_CONSTANT(AF_SIP);
#endif
#if defined(AF_ISDN)
	FTH_SET_CONSTANT(AF_ISDN);
#endif
#if defined(AF_INET6)
	FTH_SET_CONSTANT(AF_INET6);
#endif
#if defined(AF_NATM)
	FTH_SET_CONSTANT(AF_NATM);
#endif
#if defined(AF_ATM)
	FTH_SET_CONSTANT(AF_ATM);
#endif
#if defined(AF_NETGRAPH)
	FTH_SET_CONSTANT(AF_NETGRAPH);
#endif
#if defined(AF_MAX)
	FTH_SET_CONSTANT(AF_MAX);
#endif
#if defined(SOCK_STREAM)
	FTH_SET_CONSTANT(SOCK_STREAM);
#endif
#if defined(SOCK_DGRAM)
	FTH_SET_CONSTANT(SOCK_DGRAM);
#endif
#if defined(SOCK_RAW)
	FTH_SET_CONSTANT(SOCK_RAW);
#endif
#if defined(SOCK_RDM)
	FTH_SET_CONSTANT(SOCK_RDM);
#endif
#if defined(SOCK_SEQPACKET)
	FTH_SET_CONSTANT(SOCK_SEQPACKET);
#endif
#if defined(SHUT_RD)
	FTH_SET_CONSTANT(SHUT_RD);
#endif
#if defined(SHUT_WR)
	FTH_SET_CONSTANT(SHUT_WR);
#endif
#if defined(SHUT_RDWR)
	FTH_SET_CONSTANT(SHUT_RDWR);
#endif
#if defined(MSG_PEEK)
	FTH_SET_CONSTANT(MSG_PEEK);
#endif
#if defined(MSG_OOB)
	FTH_SET_CONSTANT(MSG_OOB);
#endif
}

/*
 * io.c ends here
 */
