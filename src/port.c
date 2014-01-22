/*-
 * Copyright (c) 2007-2014 Michael Scholz <mi-scholz@users.sourceforge.net>
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
 * @(#)port.c	1.62 1/22/14
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "fth.h"
#include "utils.h"

/* === PORT-IO === */

typedef struct {
	FTH		read_char;	/* Procs */
	FTH		write_char;	/* ... */
	FTH		read_line;
	FTH		write_line;
	FTH		flush;
	FTH		close;
} FIO_Softport;

#define FTH_SOFT_PORT_REF(Obj)		((FIO_Softport *)(Obj))
#define FTH_SOFT_PORT_READ_CHAR(Obj)	FTH_SOFT_PORT_REF(Obj)->read_char
#define FTH_SOFT_PORT_WRITE_CHAR(Obj)	FTH_SOFT_PORT_REF(Obj)->write_char
#define FTH_SOFT_PORT_READ_LINE(Obj)	FTH_SOFT_PORT_REF(Obj)->read_line
#define FTH_SOFT_PORT_WRITE_LINE(Obj)	FTH_SOFT_PORT_REF(Obj)->write_line
#define FTH_SOFT_PORT_FLUSH(Obj)	FTH_SOFT_PORT_REF(Obj)->flush
#define FTH_SOFT_PORT_CLOSE(Obj)	FTH_SOFT_PORT_REF(Obj)->close

static void	default_error_cb(ficlVm *vm, char *str);
static void	default_print_cb(ficlVm *vm, char *str);
static char    *default_read_cb(ficlVm *vm);
static void	ficl_make_soft_input_port(ficlVm *vm);
static void	ficl_make_soft_output_port(ficlVm *vm);
static void	ficl_make_soft_port(ficlVm *vm);
static void	ficl_port_close(ficlVm *vm);
static void	ficl_port_closed_p(ficlVm *vm);
static void	ficl_port_display(ficlVm *vm);
static void	ficl_port_flush(ficlVm *vm);
static void	ficl_port_getc(ficlVm *vm);
static void	ficl_port_gets(ficlVm *vm);
static void	ficl_port_input_p(ficlVm *vm);
static void	ficl_port_output_p(ficlVm *vm);
static void	ficl_port_p(ficlVm *vm);
static void	ficl_port_putc(ficlVm *vm);
static void	ficl_port_puts(ficlVm *vm);
static void	ficl_port_puts_format(ficlVm *vm);
static void	ficl_port_to_string(ficlVm *vm);
static void	ficl_with_error_to_port(ficlVm *vm);
static void	ficl_with_input_from_port(ficlVm *vm);
static void	ficl_with_input_port(ficlVm *vm);
static void	ficl_with_output_port(ficlVm *vm);
static void	ficl_with_output_to_port(ficlVm *vm);
static FTH	fth_init_soft_port_procs(void);
static FTH	fth_make_soft_port(FTH prcs, char *name, int fam);
static FTH	fth_set_soft_port_from_optkey(FTH prcs, int kind);
static void	port_close(void *ptr);
static void	port_flush(void *ptr);
static int	port_read_char(void *ptr);
static char    *port_read_line(void *ptr);
static void	port_write_char(void *ptr, int c);
static void	port_write_line(void *ptr, const char *line);
static void	soft_close(void);
static void	soft_flush(void);
static FTH	soft_read_char(void);
static FTH	soft_read_line(void);
static void	soft_write_char(FTH c);
static void	soft_write_line(FTH line);

#define h_list_of_port_functions "\
*** PORT PRIMITIVES ***\n\
make-soft-input-port    ( :key args -- prt )\n\
make-soft-output-port   ( :key args -- prt )\n\
make-soft-port          ( :key args -- prt )\n\
port->string            ( prt -- str )\n\
port-close              ( prt -- )\n\
port-closed?            ( obj -- f )\n\
port-display            ( prt obj -- )\n\
port-flush              ( prt -- )\n\
port-getc               ( prt -- c )\n\
port-gets               ( prt -- str )\n\
port-input?             ( obj -- f )\n\
port-output?            ( obj -- f )\n\
port-putc               ( prt c -- )\n\
port-puts               ( prt str -- )\n\
port-puts-format        ( prt fmt fmt-args -- )\n\
port-read alias for port-gets\n\
port-write alias for port-puts\n\
port-write-format alias for port-puts-format\n\
port?                   ( obj -- f )\n\
with-error-to-port      ( obj :key args -- )\n\
with-input-from-port    ( obj :key args -- str )\n\
with-input-port         ( obj :key args -- str )\n\
with-output-port        ( obj :key args -- )\n\
with-output-to-port     ( obj :key args -- )"

static int
port_read_char(void *ptr)
{
	FTH ch;

	ch = fth_proc_call(FTH_SOFT_PORT_READ_CHAR(ptr), __func__, 0);
	if (FTH_FALSE_P(ch))
		return (EOF);
	return (FTH_TO_CHAR(ch));
}

static void
port_write_char(void *ptr, int c)
{
	FTH ch;

	ch = CHAR_TO_FTH(c);
	fth_proc_call(FTH_SOFT_PORT_WRITE_CHAR(ptr), __func__, 1, ch);
}

static char *
port_read_line(void *ptr)
{
	FTH fs;

	fs = fth_proc_call(FTH_SOFT_PORT_READ_LINE(ptr), __func__, 0);
	if (FTH_FALSE_P(fs))
		return (NULL);
	return (fth_string_ref(fs));
}

static void
port_write_line(void *ptr, const char *line)
{
	FTH fs;

	fs = fth_make_string(line);
	fth_proc_call(FTH_SOFT_PORT_WRITE_LINE(ptr), __func__, 1, fs);
}

static void
port_flush(void *ptr)
{
	fth_proc_call(FTH_SOFT_PORT_FLUSH(ptr), __func__, 0);
}

static void
port_close(void *ptr)
{
	fth_proc_call(FTH_SOFT_PORT_CLOSE(ptr), __func__, 0);
}

/* --- Soft Port Procs --- */

static FTH	gn_read_char;
static FTH	gn_write_char;
static FTH	gn_read_line;
static FTH	gn_write_line;
static FTH	gn_flush;
static FTH	gn_close;

static FTH
soft_read_char(void)
{
	return (FTH_FALSE);
}

/* ARGSUSED */
static void
soft_write_char(FTH c)
{
}

static FTH
soft_read_line(void)
{
	return (FTH_FALSE);
}

/* ARGSUSED */
static void
soft_write_line(FTH line)
{
}

static void
soft_flush(void)
{
}

static void
soft_close(void)
{
}

static FTH
fth_init_soft_port_procs(void)
{
	FTH prcs;

	prcs = fth_make_array_len((ficlInteger)PORT_TYPE_LAST);
	fth_array_set(prcs, (ficlInteger)PORT_READ_CHAR, gn_read_char);
	fth_array_set(prcs, (ficlInteger)PORT_WRITE_CHAR, gn_write_char);
	fth_array_set(prcs, (ficlInteger)PORT_READ_LINE, gn_read_line);
	fth_array_set(prcs, (ficlInteger)PORT_WRITE_LINE, gn_write_line);
	fth_array_set(prcs, (ficlInteger)PORT_FLUSH, gn_flush);
	fth_array_set(prcs, (ficlInteger)PORT_CLOSE, gn_close);
	return (prcs);
}

static FTH
fth_set_soft_port_from_optkey(FTH prcs, int kind)
{
	FTH proc;
	ficlInteger type;

	type = (ficlInteger)kind;
	FTH_ASSERT_ARGS(fth_array_length(prcs) >= PORT_TYPE_LAST,
	    prcs, FTH_ARG1, "an array of length 6");
	FTH_ASSERT_ARGS((type >= 0) || (type < PORT_TYPE_LAST),
	    fth_make_int(type), FTH_ARG2, "an int");

	switch (type) {
	case PORT_READ_CHAR:
		proc = fth_get_optkey(FTH_KEYWORD_READ_CHAR, gn_read_char);
		break;
	case PORT_WRITE_CHAR:
		proc = fth_get_optkey(FTH_KEYWORD_WRITE_CHAR, gn_write_char);
		break;
	case PORT_READ_LINE:
		proc = fth_get_optkey(FTH_KEYWORD_READ_LINE, gn_read_line);
		break;
	case PORT_WRITE_LINE:
		proc = fth_get_optkey(FTH_KEYWORD_WRITE_LINE, gn_write_line);
		break;
	case PORT_FLUSH:
		proc = fth_get_optkey(FTH_KEYWORD_FLUSH, gn_flush);
		break;
	case PORT_CLOSE:
	default:
		proc = fth_get_optkey(FTH_KEYWORD_CLOSE, gn_close);
		break;
	}
	fth_array_set(prcs, type, proc);
	return (prcs);
}

static FTH
fth_make_soft_port(FTH prcs, char *name, int fam)
{
	FTH io;
	FIO_Softport *prt;

	io = make_io_base(fam);
	prt = FTH_MALLOC(sizeof(FIO_Softport));
	prt->read_char = fth_array_ref(prcs, (ficlInteger)PORT_READ_CHAR);
	prt->write_char = fth_array_ref(prcs, (ficlInteger)PORT_WRITE_CHAR);
	prt->read_line = fth_array_ref(prcs, (ficlInteger)PORT_READ_LINE);
	prt->write_line = fth_array_ref(prcs, (ficlInteger)PORT_WRITE_LINE);
	prt->flush = fth_array_ref(prcs, (ficlInteger)PORT_FLUSH);
	prt->close = fth_array_ref(prcs, (ficlInteger)PORT_CLOSE);
	FTH_IO_FILENAME(io) = fth_make_string(name);
	FTH_IO_NAME(io) = fth_make_string("port");
	FTH_IO_TYPE(io) = FTH_IO_PORT;
	FTH_IO_DATA(io) = (void *)prt;
	FTH_IO_OBJECT(io)->read_char = port_read_char;
	FTH_IO_OBJECT(io)->write_char = port_write_char;
	FTH_IO_OBJECT(io)->read_line = port_read_line;
	FTH_IO_OBJECT(io)->write_line = port_write_line;
	FTH_IO_OBJECT(io)->flush = port_flush;
	FTH_IO_OBJECT(io)->close = port_close;
	return (io);
}

static void
ficl_port_p(ficlVm *vm)
{
#define h_port_p "( obj -- f )  test if OBJ is a port\n\
nil port? => #f\n\
#f  port? => #t\n\
\"foo\" io-open-input-file port? => #t\n\
Return #t if OBJ is an IO object or #f, otherwise #f.\n\
See also port-input? and port-output?."
	FTH obj;
	bool flag;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	flag = FTH_FALSE_P(obj) || FTH_IO_P(obj); 
	ficlStackPushBoolean(vm->dataStack, flag);
}

static void
ficl_port_input_p(ficlVm *vm)
{
#define h_port_input_p "( obj -- f )  test if OBJ is input port\n\
nil port-input? => #f\n\
#f  port-input? => #t\n\
\"foo\" io-open-input-file port-input? => #t\n\
Return #t if OBJ is an input IO object or #f, otherwise #f.\n\
See also port? and port-output?."
	FTH obj;
	bool flag;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	flag = FTH_FALSE_P(obj) || fth_io_input_p(obj);
	ficlStackPushBoolean(vm->dataStack, flag);
}

static void
ficl_port_output_p(ficlVm *vm)
{
#define h_port_output_p "( obj -- f )  test if OBJ is output port\n\
nil port-output? => #f\n\
#f  port-output? => #t\n\
\"foo\" io-open-output-file port-output? => #t\n\
Return #t if OBJ is an output IO object or #f, otherwise #f.\n\
See also port? and port-input?."
	FTH obj;
	bool flag;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	flag = FTH_FALSE_P(obj) || fth_io_output_p(obj);
	ficlStackPushBoolean(vm->dataStack, flag);
}

static void
ficl_port_closed_p(ficlVm *vm)
{
#define h_port_closed_p "( io -- f )  test if IO is closed\n\
\"foo\" io-open-output-file value o1\n\
o1 port-closed? => #f\n\
o1 port-close\n\
o1 port-closed? => #t\n\
Return #t if IO object is closed, otherwise #f."
	FTH obj;
	bool flag;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	flag = FTH_FALSE_P(obj) || fth_io_closed_p(obj);
	ficlStackPushBoolean(vm->dataStack, flag);
}

#define h_port_keywords "\
Keywords:\n\
:fam          r/o, w/o (default), r/w\n\
:port-name    \"soft-port\"\n\
:read-char    proc ( -- c )\n\
:write-char   proc ( c -- )\n\
:read-line    proc ( -- line )\n\
:write-line   proc ( line -- )\n\
:flush        proc ( -- )\n\
:close        proc ( -- )\n\
Not all procs are required.  \
If you want an object for reading, provide read procs, \
the same for writing.\n\
See also make-soft-port, make-soft-input-port, make-soft-output-port."

static void
ficl_make_soft_port(ficlVm *vm)
{
#define h_make_soft_port "( :key args -- io )  return soft port\n\
Input example:\n\
:port-name \"sndin\"\n\
:read-char lambda: <{ -- c }> *stdin* io-getc ;\n\
:read-line lambda: <{ -- line }> *stdin* io-read ;\n\
make-soft-port set-*stdin* value stdin-io\n\
Return soft port for reading.  \
The *stdin* IO object is preserved in stdin-io for later use.\n\
Output example:\n\
:port-name \"sndout\"\n\
:write-char lambda: <{ c -- }> c snd-print .stdout ;\n\
:write-line lambda: <{ line -- }> line snd-print .stdout ;\n\
make-soft-port set-*stdout* value stdout-io\n\
Return soft port for writing.  \
The *stdout* IO object is preserved in stdout-io for later use.\n\
Return new soft port IO object with corresponding procs.\n\
" h_port_keywords
	int fam;
	char *name;
	FTH prcs, port;

	fam = fth_get_optkey_fix(FTH_KEYWORD_FAM, FICL_FAM_WRITE);
	name = fth_get_optkey_str(FTH_KEYWORD_PORT_NAME, "soft-port");
	prcs = fth_init_soft_port_procs();
	fth_set_soft_port_from_optkey(prcs, PORT_READ_CHAR);
	fth_set_soft_port_from_optkey(prcs, PORT_WRITE_CHAR);
	fth_set_soft_port_from_optkey(prcs, PORT_READ_LINE);
	fth_set_soft_port_from_optkey(prcs, PORT_WRITE_LINE);
	fth_set_soft_port_from_optkey(prcs, PORT_FLUSH);
	fth_set_soft_port_from_optkey(prcs, PORT_CLOSE);
	port = fth_make_soft_port(prcs, name, fam);
	ficlStackPushFTH(vm->dataStack, port);
}

static void
ficl_make_soft_input_port(ficlVm *vm)
{
#define h_msiport "( :key args -- io )  return in soft port\n\
:port-name \"sndin\"\n\
:read-char lambda: <{ -- c }> *stdin* io-getc ;\n\
:read-line lambda: <{ -- line }> *stdin* io-read ;\n\
make-soft-port set-*stdin* value stdin-io\n\
Return soft port IO object for reading.  \
The *stdin* IO object is preserved in stdin-io for later use.\n\
" h_port_keywords
	char *name;
	FTH prcs, port;

	name = fth_get_optkey_str(FTH_KEYWORD_PORT_NAME, "soft-port");
	prcs = fth_init_soft_port_procs();
	fth_set_soft_port_from_optkey(prcs, PORT_READ_CHAR);
	fth_set_soft_port_from_optkey(prcs, PORT_READ_LINE);
	fth_set_soft_port_from_optkey(prcs, PORT_FLUSH);
	fth_set_soft_port_from_optkey(prcs, PORT_CLOSE);
	port = fth_make_soft_port(prcs, name, FICL_FAM_READ);
	ficlStackPushFTH(vm->dataStack, port);
}

static void
ficl_make_soft_output_port(ficlVm *vm)
{
#define h_msop "( :key args -- io )  return out soft port\n\
:port-name \"sndout\"\n\
:write-char lambda: <{ c -- }> c snd-print .stdout ;\n\
:write-line lambda: <{ line -- }> line snd-print .stdout ;\n\
make-soft-port set-*stdout* value stdout-io\n\
Return soft port IO object for writing.  \
The *stdout* IO object is preserved in stdout-io for later use.\n\
" h_port_keywords
	char *name;
	FTH prcs, port;

	name = fth_get_optkey_str(FTH_KEYWORD_PORT_NAME, "soft-port");
	prcs = fth_init_soft_port_procs();
	fth_set_soft_port_from_optkey(prcs, PORT_WRITE_CHAR);
	fth_set_soft_port_from_optkey(prcs, PORT_WRITE_LINE);
	fth_set_soft_port_from_optkey(prcs, PORT_FLUSH);
	fth_set_soft_port_from_optkey(prcs, PORT_CLOSE);
	port = fth_make_soft_port(prcs, name, FICL_FAM_WRITE);
	ficlStackPushFTH(vm->dataStack, port);
}

int
fth_port_getc(FTH port)
{
	if (FTH_FALSE_P(port))
		port = ficlVmGetPortIn(FTH_FICL_VM());
	if (FTH_IO_P(port))
		return (fth_io_getc(port));
	FTH_ASSERT_ARGS(false, port, FTH_ARG1, "an open IO object or #f");
	/* NOTREACHED */
	return (-1);
}

static void
ficl_port_getc(ficlVm *vm)
{
#define h_port_getc "( prt -- c )  return next char\n\
#f port-getc\n\
1 => 49\n\
Return next character from PRT IO object.  \
If PRT is #f, read from current input port (stdin).\n\
See also port-gets."
	int c;

	FTH_STACK_CHECK(vm, 1, 1);
	c = fth_port_getc(fth_pop_ficl_cell(vm));
	ficlStackPushInteger(vm->dataStack, (ficlInteger)c);
}

char *
fth_port_gets(FTH port)
{
	if (FTH_FALSE_P(port))
		port = ficlVmGetPortIn(FTH_FICL_VM());
	if (FTH_IO_P(port))
		return (fth_io_read(port));
	FTH_ASSERT_ARGS(false, port, FTH_ARG1, "an open IO object or #f");
	/* NOTREACHED */
	return (NULL);
}

static void
ficl_port_gets(ficlVm *vm)
{
#define h_port_gets "( prt -- str )  return next line\n\
#f port-gets\n\
hello => \"hello\n\"\n\
Return one line from PRT IO object.  \
If PRT is #f, read from current input port (stdin).\n\
See also port-getc."
	FTH_STACK_CHECK(vm, 1, 1);
	push_cstring(vm, fth_port_gets(fth_pop_ficl_cell(vm)));
}

void
fth_port_putc(FTH port, int c)
{
	if (FTH_FALSE_P(port))
		port = ficlVmGetPortOut(FTH_FICL_VM());
	if (FTH_IO_P(port)) {
		fth_io_putc(port, c);
		return;
	}
	FTH_ASSERT_ARGS(false, port, FTH_ARG1, "an open IO object or #f");
}

static void
ficl_port_putc(ficlVm *vm)
{
#define h_port_putc "( prt c -- )  write char\n\
#f <char> a port-putc => a\n\
#f <char> b port-putc => b\n\
#f <char> c port-putc => c\n\
Write character C to PRT IO object.  \
If PRT is #f, write to current output port (stout).\n\
See also port-puts and port-puts-format."
	FTH prt;
	int c;

	FTH_STACK_CHECK(vm, 2, 0);
	c = (int)ficlStackPopInteger(vm->dataStack);
	prt = fth_pop_ficl_cell(vm);
	fth_port_putc(prt, c);
}

/*
 * Put C string STR to output port PORT.  If PORT is FTH_FALSE, print
 * to standard out.
 */
void
fth_port_puts(FTH port, const char *str)
{
	if (FTH_FALSE_P(port))
		port = ficlVmGetPortOut(FTH_FICL_VM());
	if (FTH_IO_P(port)) {
		fth_io_write_and_flush(port, str);
		return;
	}
	FTH_ASSERT_ARGS(false, port, FTH_ARG1, "an open IO object or #f");
}

static void
ficl_port_puts(ficlVm *vm)
{
#define h_port_puts "( prt str -- )  write STR to PRT\n\
#f \"hello\" port-puts => hello\n\
Write STR to PRT IO object.  \
If PRT is #f, write to current output port (stout).\n\
See also port-putc and port-puts-format."
	FTH prt;
	char *str;

	FTH_STACK_CHECK(vm, 2, 0);
	str = pop_cstring(vm);
	prt = fth_pop_ficl_cell(vm);
	fth_port_puts(prt, str);
}

static void
ficl_port_puts_format(ficlVm *vm)
{
#define h_port_puts_fmt "( prt fmt fmt-args -- )  write formatted line\n\
#f \"hello, %s\" #( \"world\" ) port-puts-format => hello, world\n\
Write string built from FMT and array FMT-ARGS to PRT IO object.  \
If PRT is #f, write to current output port (stout).\n\
See also port-putc and port-puts."
	FTH prt, fmt, args;

	FTH_STACK_CHECK(vm, 3, 0);
	args = fth_pop_ficl_cell(vm);
	fmt = fth_pop_ficl_cell(vm);
	prt = fth_pop_ficl_cell(vm);
	fth_port_puts(prt, fth_string_ref(fth_string_format(fmt, args)));
}

/*
 * Put string representation of Fth object OBJ to output port PORT.
 * If PORT is FTH_FALSE, print to standard out.
 */
void
fth_port_display(FTH port, FTH obj)
{
	if (FTH_FALSE_P(port))
		port = ficlVmGetPortOut(FTH_FICL_VM());
	if (FTH_IO_P(port)) {
		fth_io_write_and_flush(port, fth_to_c_string(obj));
		return;
	}
	FTH_ASSERT_ARGS(false, port, FTH_ARG1, "an open IO object or #f");
}

static void
ficl_port_display(ficlVm *vm)
{
#define h_port_display "( prt obj -- )  write OBJ to PRT\n\
#f #( 0 1 2 ) port-display => #( 0 1 2 )\n\
Write the string representation of OBJ to PRT object.  \
If PRT is #f, write to current output port (stout).\n\
See also port-puts and port-puts-format."
	FTH prt, obj;

	FTH_STACK_CHECK(vm, 2, 0);
	obj = fth_pop_ficl_cell(vm);
	prt = fth_pop_ficl_cell(vm);
	fth_port_display(prt, obj);
}

/*
 * Return content of PORT as Fth string.  If PORT is FTH_FALSE, return
 * FTH_FALSE.
 */
FTH
fth_port_to_string(FTH port)
{
	if (FTH_FALSE_P(port))
		return (FTH_FALSE);
	return (fth_io_to_string(port));
}

static void
ficl_port_to_string(ficlVm *vm)
{
#define h_port_to_string "( prt -- str|#f )  return PRT as string\n\
\".fthrc\" io-open-read value prt\n\
prt port->string => \"...\"\n\
#f port->string => #f\n\
Return content of PRT object as string if available, otherwise #f."
	FTH_STACK_CHECK(vm, 1, 1);
	fth_push_ficl_cell(vm, fth_port_to_string(fth_pop_ficl_cell(vm)));
}

/*
 * Flush PORT; if PORT is FTH_FALSE, do nothing.
 */
void
fth_port_flush(FTH port)
{
	if (FTH_FALSE_P(port))
		return;
	fth_io_flush(port);
}

static void
ficl_port_flush(ficlVm *vm)
{
#define h_port_flush "( prt -- )  flush PRT\n\
#f port-flush \\ does nothing\n\
File and IO ports flush their streams, other kind of ports do nothing."
	FTH_STACK_CHECK(vm, 1, 0);
	fth_port_flush(fth_pop_ficl_cell(vm));
}

/*
 * Close PORT; if PORT is FTH_FALSE, do nothing.
 */
void
fth_port_close(FTH port)
{
	if (FTH_FALSE_P(port))
		return;
	fth_io_close(port);
}

static void
ficl_port_close(ficlVm *vm)
{
#define h_port_close "( prt -- )  close PRT\n\
#f port-close \\ does nothing\n\
File and IO ports close their streams, other kind of ports do nothing."
	FTH_STACK_CHECK(vm, 1, 0);
	fth_port_close(fth_pop_ficl_cell(vm));
}

/* --- with-input-port, with-output-port --- */

/*
 * :filename     file name         (string)
 *   :fam        r/o
 * :command      cmd               (string or array-of-strings)
 *   :fam        r/o
 * :string       string            (string)
 *   :fam        r/o
 * :socket       host              (string)
 *   :domain     AF_INET (AF_UNIX) (integer)
 *   :port       1024              (integer)
 * :soft-port    port name         (string)
 *   :fam        r/o
 *   :port-name  "soft-port-name"
 *   :read-char
 *   :write-char
 *   :read-line
 *   :write-line
 *   :flush
 *   :close
 */

/*
 * io-open-file in io.c needs this function too.
 */

/*
 * Usage:
 *
 * FTH io = io_keyword_args_ref(FICL_FAM_READ)
 * FTH io = io_keyword_args_ref(
 *              fth_get_optkey_fix(FTH_KEYWORD_FAM, FICL_FAM_READ));
 */
FTH
io_keyword_args_ref(int fam)
{
	FTH arg;

	arg = fth_get_optkey(FTH_KEYWORD_FILENAME, FTH_UNDEF);
	if (FTH_BOUND_P(arg))
		return (fth_io_open(fth_string_ref(arg), fam));
	arg = fth_get_optkey(FTH_KEYWORD_COMMAND, FTH_UNDEF);
	if (FTH_BOUND_P(arg))
		return (fth_io_popen(arg, fam));
	arg = fth_get_optkey(FTH_KEYWORD_STRING, FTH_UNDEF);
	if (FTH_BOUND_P(arg))
		return (fth_io_sopen(arg, fam));
#if HAVE_SOCKET
	arg = fth_get_optkey(FTH_KEYWORD_SOCKET, FTH_UNDEF);
	if (FTH_BOUND_P(arg)) {
		int d, p;

		d = fth_get_optkey_fix(FTH_KEYWORD_DOMAIN, FTH_DEFAULT_ADDRFAM);
		p = fth_get_optkey_fix(FTH_KEYWORD_PORT, FTH_DEFAULT_PORT);
		return (fth_io_nopen(fth_string_ref(arg), p, d));
	}
#endif	/* HAVE_SOCKET */
	arg = fth_get_optkey(FTH_KEYWORD_SOFT_PORT, FTH_UNDEF);
	if (FTH_BOUND_P(arg)) {
		char *name, *s;
		FTH prcs;

		s = fth_string_ref(arg);
		name = fth_get_optkey_str(FTH_KEYWORD_PORT_NAME, s);
		prcs = fth_init_soft_port_procs();
		fth_set_soft_port_from_optkey(prcs, PORT_READ_CHAR);
		fth_set_soft_port_from_optkey(prcs, PORT_WRITE_CHAR);
		fth_set_soft_port_from_optkey(prcs, PORT_READ_LINE);
		fth_set_soft_port_from_optkey(prcs, PORT_WRITE_LINE);
		fth_set_soft_port_from_optkey(prcs, PORT_FLUSH);
		fth_set_soft_port_from_optkey(prcs, PORT_CLOSE);
		return (fth_make_soft_port(prcs, name, fam));
	}
	fth_throw(FTH_ARGUMENT_ERROR,
	    "%s (%s): wrong or empty keyword args",
	    RUNNING_WORD(), __func__);
	return (FTH_FALSE);
}

static void
ficl_with_input_port(ficlVm *vm)
{
#define h_with_iport "( obj :key args -- str )  read string\n\
% cat file.test\n\
hello\n\
%\n\
lambda: <{ io -- str }> io io-read ; :filename \"file.test\" with-input-port => \"hello\n\"\n\
\"hello\" value s\n\
lambda: <{ io -- str }> io io-read ; :string s with-input-port => \"hello\"\n\
nil :filename \"file.test\" with-input-port => \"hello\n\"\n\
Open IO object for input.  \
If OBJ is NIL, read first line from IO object, \
otherwise execute OBJ as a proc or xt with stack effect ( io -- str ).  \
Close IO object and return resulting string.\n\
" keyword_args_string "\n\
See also with-output-port, with-input-from-port, with-output-to-port, \
with-error-to-port."
	FTH arg, res, io, proc;

	io = io_keyword_args_ref(FICL_FAM_READ);
	FTH_STACK_CHECK(vm, 1, 1);
	arg = fth_pop_ficl_cell(vm);
	if (FTH_NIL_P(arg))
		res = fth_io_read_line(io);
	else {
		proc = proc_from_proc_or_xt(arg, 1, 0, false);
		FTH_ASSERT_ARGS(FTH_PROC_P(proc), proc, FTH_ARG1, "a proc");
		res = fth_proc_call(proc, RUNNING_WORD(), 1, io);
	}
	fth_io_close(io);
	fth_push_ficl_cell(vm, res);
}

static void
ficl_with_output_port(ficlVm *vm)
{
#define h_with_oport "( obj :key args -- )  write string\n\
lambda: <{ io -- }>\n\
  io \"hello\\n\" io-write\n\
; :filename \"file.test\" with-output-port\n\
% cat file.test\n\
hello\n\
%\n\
\"\" value s\n\
lambda: <{ io -- }>\n\
  io \"hello\" io-write\n\
; :string s with-output-port\n\
s => \"hello\"\n\
\"file.test\" file-delete\n\
\"hello\\n\" :filename \"file.test\" with-output-port\n\
% cat file.test\n\
hello\n\
%\n\
Open IO object for output.  \
If OBJ is a string, write string to IO object, \
otherwise execute OBJ as proc or xt with stack effect ( io -- ).  \
Close IO object.\n\
" keyword_args_string "\n\
See also with-input-port, with-input-from-port, with-output-to-port, \
with-error-to-port."
	FTH arg, io, proc;

	io = io_keyword_args_ref(FICL_FAM_WRITE);
	FTH_STACK_CHECK(vm, 1, 1);
	arg = fth_pop_ficl_cell(vm);
	if (FTH_STRING_P(arg))
		fth_io_write(io, fth_string_ref(arg));
	else {
		proc = proc_from_proc_or_xt(arg, 1, 0, false);
		FTH_ASSERT_ARGS(FTH_PROC_P(proc), proc, FTH_ARG1, "a proc");
		fth_proc_call(proc, RUNNING_WORD(), 1, io);
	}
	fth_io_close(io);
}

/* --- with-input-from-port, with-output-to-port, with-error-to-port --- */

static void
ficl_with_input_from_port(ficlVm *vm)
{
#define h_wifport "( obj :key args -- str )  read string\n\
% cat file.test\n\
hello\n\
%\n\
lambda: <{ -- str }>\n\
  *stdin* io-read\n\
; :filename \"file.test\" with-input-from-port => \"hello\n\"\n\
\"hello\" value s\n\
lambda: <{ -- str }>\n\
  *stdin* io-read\n\
; :string s with-input-from-port => \"hello\"\n\
nil :filename \"file.test\" with-input-from-port => \"hello\"\n\
Open IO object for input and point IO to *STDIN*.  \
If OBJ is NIL, read first line from IO object, \
otherwise execute OBJ as proc or xt with stack effect ( -- str ).  \
Close IO object and return resulting string.  \
*STDIN* will be reset to its previous value.\n\
" keyword_args_string "\n\
See also with-input-port, with-output-port, with-output-to-port, \
with-error-to-port."
	FTH arg, res, old_io, io, proc;

	io = io_keyword_args_ref(FICL_FAM_READ);
	FTH_STACK_CHECK(vm, 1, 1);
	arg = fth_pop_ficl_cell(vm);
	old_io = fth_set_io_stdin(io);
	if (FTH_NIL_P(arg))
		res = fth_io_read_line(io);
	else {
		proc = proc_from_proc_or_xt(arg, 0, 0, false);
		if (!FTH_PROC_P(proc)) {
			fth_io_close(fth_set_io_stdin(old_io));
			FTH_ASSERT_ARGS(false, proc, FTH_ARG1, "a proc");
		}
		res = fth_proc_call(proc, RUNNING_WORD(), 0);
	}
	fth_io_close(fth_set_io_stdin(old_io));
	fth_push_ficl_cell(vm, res);
}

static void
ficl_with_output_to_port(ficlVm *vm)
{
#define h_wotport "( obj :key args -- )  write string\n\
lambda: <{ -- }>\n\
  .\" hello\" cr\n\
; :filename \"file.test\" with-output-to-port\n\
% cat test.file\n\
hello\n\
%\n\
\"\" value s\n\
lambda: <{ -- }>\n\
  *stdout* \"hello\" io-write\n\
; :string s with-output-to-port\n\
s => \"hello\"\n\
\"file.test\" file-delete\n\
\"hello\\n\" :filename \"file.test\" with-output-to-port\n\
% cat file.test\n\
hello\n\
%\n\
Open IO object for output and point IO to *STDOUT*.  \
If OBJ is a string, write string to IO object, \
otherwise execute OBJ as proc or xt with stack effect ( -- ).  \
Close IO object.  \
*STDOUT* will be reset to its previous value.\n\
" keyword_args_string "\n\
See also with-input-port, with-output-port, with-input-from-port, \
with-error-to-port."
	FTH arg, old_io, io, proc;

	io = io_keyword_args_ref(FICL_FAM_WRITE);
	FTH_STACK_CHECK(vm, 1, 1);
	arg = fth_pop_ficl_cell(vm);
	old_io = fth_set_io_stdout(io);
	if (FTH_STRING_P(arg))
		fth_io_write(io, fth_string_ref(arg));
	else {
		proc = proc_from_proc_or_xt(arg, 0, 0, false);
		if (!FTH_PROC_P(proc)) {
			fth_io_close(fth_set_io_stdout(old_io));
			FTH_ASSERT_ARGS(false, proc, FTH_ARG1, "a proc");
		}
		fth_proc_call(proc, RUNNING_WORD(), 0);
	}
	fth_io_close(fth_set_io_stdout(old_io));
}

static void
ficl_with_error_to_port(ficlVm *vm)
{
#define h_wetport "( obj :key args -- )  write string\n\
lambda: <{ -- }>\n\
  \"hello\\n\" .stderr\n\
; :filename \"file.test\" with-error-to-port\n\
% cat test.file\n\
hello\n\
%\n\
\"\" value s\n\
lambda: <{ -- }>\n\
  *stderr* \"hello\" io-write\n\
; :string s with-error-to-port\n\
s => \"hello\"\n\
\"file.test\" file-delete\n\
\"hello\\n\" :filename \"file.test\" with-error-to-port\n\
% cat file.test\n\
hello\n\
%\n\
Open IO object for output and point IO to *STDERR*.  \
If OBJ is a string, write string to IO object, \
otherwise execute OBJ as proc or xt with stack effect ( -- ).  \
Close IO object.  \
*STDERR* will be reset to its previous value.\n\
" keyword_args_string "\n\
See also with-input-port, with-output-port, with-input-from-port, \
with-output-to-port."
	FTH arg, old_io, io, proc;

	io = io_keyword_args_ref(FICL_FAM_WRITE);
	FTH_STACK_CHECK(vm, 1, 1);
	arg = fth_pop_ficl_cell(vm);
	old_io = fth_set_io_stderr(io);
	if (FTH_STRING_P(arg))
		fth_io_write(io, fth_string_ref(arg));
	else {
		proc = proc_from_proc_or_xt(arg, 0, 0, false);
		if (!FTH_PROC_P(proc)) {
			fth_io_close(fth_set_io_stderr(old_io));
			FTH_ASSERT_ARGS(false, proc, FTH_ARG1, "a proc");
		}
		fth_proc_call(proc, RUNNING_WORD(), 0);
	}
	fth_io_close(fth_set_io_stderr(old_io));
}

/* --- in- and output callbacks --- */

in_cb		fth_read_hook;
out_cb		fth_print_hook;
out_cb		fth_error_hook;

static char *
default_read_cb(ficlVm *vm)
{
	FTH io;

	io = ficlVmGetPortIn(vm);
	return (FTH_IO_READ_LINE(io));
}

static void
default_print_cb(ficlVm *vm, char *str)
{
	FTH io;

	io = ficlVmGetPortOut(vm);
	FTH_IO_WRITE_LINE(io, str);
	FTH_IO_FLUSH(io);
}

static void
default_error_cb(ficlVm *vm, char *str)
{
	FTH io;

	io = ficlVmGetPortErr(vm);
	FTH_IO_WRITE_LINE(io, str);
	FTH_IO_FLUSH(io);
}

/* char *(*out_cb)(ficlVm *vm); */
in_cb
fth_set_read_cb(in_cb cb)
{
	in_cb old_cb;

	old_cb = fth_read_hook;
	fth_read_hook = (cb != NULL) ? cb : default_read_cb;
	return (old_cb);
}

/* void (*out_cb)(ficlVm *vm, char *msg); */
out_cb
fth_set_print_cb(out_cb cb)
{
	out_cb old_cb;

	old_cb = fth_print_hook;
	fth_print_hook = (cb != NULL) ? cb : default_print_cb;
	return (old_cb);
}

out_cb
fth_set_error_cb(out_cb cb)
{
	out_cb old_cb;

	old_cb = fth_error_hook;
	fth_error_hook = (cb != NULL) ? cb : default_error_cb;
	return (old_cb);
}

out_cb
fth_set_print_and_error_cb(out_cb cb)
{
	out_cb old_cb;

	old_cb = fth_print_hook;
	fth_print_hook = (cb != NULL) ? cb : default_print_cb;
	fth_error_hook = (cb != NULL) ? cb : default_error_cb;
	return (old_cb);
}

void
init_port(void)
{
#define MPFF(Name, Args) fth_make_proc_from_func(NULL, Name, false, Args, 0, 0)
#define MPFVF(Name, Args) fth_make_proc_from_vfunc(NULL, Name, Args, 0, 0)
	gn_read_char = MPFF(soft_read_char, 0);
	gn_write_char = MPFVF(soft_write_char, 1);
	gn_read_line = MPFF(soft_read_line, 0);
	gn_write_line = MPFVF(soft_write_line, 1);
	gn_flush = MPFVF(soft_flush, 0);
	gn_close = MPFVF(soft_close, 0);
	FTH_PRI1("port?", ficl_port_p, h_port_p);
	FTH_PRI1("port-input?", ficl_port_input_p, h_port_input_p);
	FTH_PRI1("port-output?", ficl_port_output_p, h_port_output_p);
	FTH_PRI1("port-closed?", ficl_port_closed_p, h_port_closed_p);
	FTH_PRI1("make-soft-port", ficl_make_soft_port, h_make_soft_port);
	FTH_PRI1("make-soft-input-port", ficl_make_soft_input_port, h_msiport);
	FTH_PRI1("make-soft-output-port", ficl_make_soft_output_port, h_msop);
	FTH_PRI1("port-getc", ficl_port_getc, h_port_getc);
	FTH_PRI1("port-putc", ficl_port_putc, h_port_putc);
	FTH_PRI1("port-gets", ficl_port_gets, h_port_gets);
	FTH_PRI1("port-read", ficl_port_gets, h_port_gets);
	FTH_PRI1("port-puts", ficl_port_puts, h_port_puts);
	FTH_PRI1("port-write", ficl_port_puts, h_port_puts);
	FTH_PRI1("port-puts-format", ficl_port_puts_format, h_port_puts_fmt);
	FTH_PRI1("port-write-format", ficl_port_puts_format, h_port_puts_fmt);
	FTH_PRI1("port-display", ficl_port_display, h_port_display);
	FTH_PRI1("port->string", ficl_port_to_string, h_port_to_string);
	FTH_PRI1("port-flush", ficl_port_flush, h_port_flush);
	FTH_PRI1("port-close", ficl_port_close, h_port_close);
	FTH_PRI1("with-input-port", ficl_with_input_port, h_with_iport);
	FTH_PRI1("with-output-port", ficl_with_output_port, h_with_oport);
	FTH_PRI1("with-input-from-port", ficl_with_input_from_port, h_wifport);
	FTH_PRI1("with-output-to-port", ficl_with_output_to_port, h_wotport);
	FTH_PRI1("with-error-to-port", ficl_with_error_to_port, h_wetport);
	FTH_ADD_FEATURE_AND_INFO(FTH_STR_PORT, h_list_of_port_functions);
}

/*
 * port.c ends here
 */
