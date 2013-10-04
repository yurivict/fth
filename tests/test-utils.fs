\ Copyright (c) 2009-2012 Michael Scholz <mi-scholz@users.sourceforge.net>
\ All rights reserved.
\
\ Redistribution and use in source and binary forms, with or without
\ modification, are permitted provided that the following conditions
\ are met:
\ 1. Redistributions of source code must retain the above copyright
\    notice, this list of conditions and the following disclaimer.
\ 2. Redistributions in binary form must reproduce the above copyright
\    notice, this list of conditions and the following disclaimer in the
\    documentation and/or other materials provided with the distribution.
\ 
\ THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
\ ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
\ IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
\ ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
\ FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
\ DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
\ OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
\ HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
\ LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
\ OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
\ SUCH DAMAGE.
\ 
\ @(#)test-utils.fs	1.9 9/13/13

[ifundef] *fth-test-count*
	100 value *fth-test-count*
[then]

"test.file" value fname
lambda: <{ -- }>
	fname file-delete
; at-exit

: fth-display ( fmt args-lst -- )
	." \ " string-format .error cr
;

[ifundef] test-expr
	: test-expr <{ expr info -- }>
		stack-reset
		expr if
			"%s: %s" #( info expr ) fth-display
			\ skipping rest of current test file
			77 (bye)
		then
	;
[then]

: test-expr-format ( expr fmt args -- )
	string-format test-expr
;

\ array-test.fs
: fnumb-cmp ( a b -- -1|0|1 )
	{ a b }
	a b f< if
		-1
	else
		a b f= if
			0
		else
			1
		then
	then
;

\ hash-test.fs
<'> noop alias array1-test
<'> noop alias hash1-test

\ hook-test.fs
#( 1 0 #f ) "our test hook" create-hook test-hook
: test-hook-proc1 <{ arg -- val }>
	arg 2*
;

: test-hook-proc2 <{ arg1 arg2 -- val }>
	arg1 arg2 *
;

\ io-test.fs
undef set-version-control

'socket provided? [if]
	: unix-server { name -- io }
		AF_UNIX SOCK_STREAM net-socket { fd }
		fd name -1 AF_UNIX net-bind
		fd net-listen
		fd name AF_UNIX net-accept
	;

	: unix-client { name -- io }
		AF_UNIX SOCK_STREAM net-socket { fd }
		fd name -1 AF_UNIX net-connect
	;

	: inet-server { host port -- io }
		AF_INET SOCK_STREAM net-socket { fd }
		fd host port AF_INET net-bind
		fd net-listen
		fd host AF_INET net-accept
	;

	: inet-client { host port -- io }
		AF_INET SOCK_STREAM net-socket { fd }
		fd host port AF_INET net-connect
	;
[then]

nil value *io*

:port-name "fth-test"
:write-line lambda: <{ line -- }>
	*io* line io-write
; make-soft-port value io-test-stdout-port

:port-name "fth-test"
:fam r/o
:read-line lambda: <{ -- line }>
	*io* io-read
; make-soft-port value io-test-stdin-port

: test-with-input-port <{ io -- val }>
	io io-read
;

: test-with-output-port <{ io -- val }>
	io "hello" io-write
;

: test-with-input-from-port <{ -- val }>
	*stdin* io-read
;

: test-with-output-to-port <{ -- val }>
	*stdout* "hello" io-write
;

: test-with-error-to-port <{ -- val }>
	*stderr* "hello" io-write
;

\ list-test.fs
: test-* <{ o1 o2 -- n }>
	o1 undef? if
		1 to o1
	then
	o2 undef? if
		1 to o2
	then
	o1 o2 * 
;

\ misc-test.fs
*lineno* 1+ value fth-test-line
: fth-test-proc { n -- }
	n 0= if
		'math-error '( "%s: %s" get-func-name n ) fth-throw
	then
	n 1 = if
		'math-error "(fth-raise) %s: %s" '( get-func-name n ) fth-raise
	then
;

\ numbers-test.fs
: fneq-err ( r1 r2 err -- f )
	-rot f- fabs f<=
;

: cneq-err { c1 c2 err -- f }
	c1 real-ref c2 real-ref err fneq-err
	c1 imag-ref c2 imag-ref err fneq-err or
;

: fneq ( a b -- f )
	0.001 fneq-err
;

: cneq ( a b -- f )
	0.001 cneq-err
;

\ proc-test.fs
lambda: <{ a b -- c }>
	a b +
; value prc1

: optkey-test ( start dur keyword-args -- ary )
	:frequency     440.0 get-optkey { freq }
	:initial-phase pi    get-optkey { phase }
	{ start dur }
	#( start dur freq phase )
;

: optarg-test ( a b c=33 d=44 e=55 -- ary )
	4 55 get-optarg { e }
	3 44 get-optarg { d }
	2 33 get-optarg { c }
	{ a b }
	#( a b c d e )
;

: optkeys-test ( start dur keyword-args -- ary )
	#( :frequency     440.0
	   :initial-phase pi ) 2  get-optkeys { start dur freq phase }
	#( start dur freq phase )
;

: optargs-test ( a b c=33 d=44 e=55 -- ary )
	#( 33 44 55 ) 2 get-optargs { a b c d e }
	#( a b c d e )
;

\ test-utils.fs ends here
