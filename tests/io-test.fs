\ Copyright (c) 2006-2017 Michael Scholz <mi-scholz@users.sourceforge.net>
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
\ @(#)io-test.fs	1.48 12/2/17

require test-utils.fs

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

\ io-test.fs ends here
\ Check if pwd accepts option -P:
"sh -c pwd -P 2>&1 /dev/null" file-shell drop
exit-status [if] "pwd" [else] "pwd -P" [then] constant *io-test-pwd-cmd*

: test-rewind { io -- }
	'minix provided? if
		io io-close
		io io-filename io-open-read to io
	else
		io io-rewind
	then
;

'socket provided? [if]
	: socket-test ( -- )
		\ io-nopen, make-socket-port (alias)
		nil { io }
		"localhost" :port 21 :domain AF_INET <'> io-nopen #t nil
		    fth-catch if
			stack-reset
		else
			to io
			io io-input? not
			    "io-nopen not input? (r/w inet-socket)"  test-expr
			io io-output? not
			    "io-nopen not output? (r/w inet-socket)" test-expr
			io io-close
		then
		"localhost" <'> io-nopen #t nil fth-catch if
			stack-reset
		else
			to io
			io io-input? not
			    "io-nopen not input? (r/w def socket)"  test-expr
			io io-output? not
			    "io-nopen not output? (r/w def socket)" test-expr
			io io-close
		then
	;
[else]
	: socket-test ( -- ) ;
[then]

: io-test ( -- )
	"file.test" { ftest }
	nil nil nil { io io1 io2 }
	nil nil nil { s1 s2 slen }
	nil nil nil { line lines lines1 }
	nil nil     { version-name version-name2 }
	nil nil     { old-stdin old-stdout }
	\ io-filename
	*stdin*  io-filename "*stdin*" string<>
	    "*stdin* io-filename" test-expr
	*stdout* io-filename "*stdout*" string<>
	    "*stdout* io-filename" test-expr
	*stderr* io-filename "*stderr*" string<>
	    "*stderr* io-filename" test-expr
	\ io-mode
	*stdin*  io-mode "r" string<> "*stdin* io-mode"  test-expr
	*stdout* io-mode "w" string<> "*stdout* io-mode" test-expr
	*stderr* io-mode "w" string<> "*stderr* io-mode" test-expr
	\ io-fileno
	*stdin*  io-fileno 0 <> "*stdin* io-fileno"  test-expr
	*stdout* io-fileno 1 <> "*stdout* io-fileno" test-expr
	*stderr* io-fileno 2 <> "*stderr* io-fileno" test-expr
	\ io->string, io-rewind
	fname file-delete
	fname :fam r/w io-open to io
	io "io->string-test" io-write
	io io-rewind
	io io-read to s1
	io io->string to s2
	s1 s2 string<> "io->string" test-expr
	io io-close
	\ io?
	nil io? "nil io?" test-expr
	#() io? "#() io?" test-expr
	0   io? "0 io?"   test-expr
	fname io-open-write to io
	io io? not "io?" test-expr
	io io-close
	\ *stdin*|out*|err* io?|io-input?|output?|closed? 
	*stdin*  io? 	    not "*stdin* not io?"      test-expr
	*stdout* io? 	    not "*stdout* not io?"     test-expr
	*stderr* io? 	    not "*stderr* not io?"     test-expr
	*stdin*  io-input?  not "*stdin* not input?"   test-expr
	*stdin*  io-output?     "*stdin* output?"      test-expr
	*stdin*  io-closed?     "*stdin* closed?"      test-expr
	*stdout* io-input?      "*stdout* input?"      test-expr
	*stdout* io-output? not "*stdout* not output?" test-expr
	*stdout* io-closed?     "*stdout* closed?"     test-expr
	*stderr* io-input?      "*stderr* input?"      test-expr
	*stderr* io-output? not "*stderr* not output?" test-expr
	*stderr* io-closed?     "*stderr* closed?"     test-expr
	\ verion-control, set-version-control, :if-exists :rename
	fname file-delete
	fname :fam w/o io-open io-close
	#t set-version-control
	fname :if-exists :rename :fam w/o io-open io-close
	fname ".~1~" $+ to version-name
	version-name file-exists? not "io-open rename #t" test-expr
	nil set-version-control
	fname :if-exists :rename :fam w/o io-open io-close
	fname ".~2~" $+ to version-name2
	version-name2 file-exists? not "io-open rename nil numb" test-expr
	version-name2 file-delete
	version-name file-delete
	fname :if-exists :rename :fam w/o io-open io-close
	fname "~" $+ to version-name
	version-name file-exists? not "io-open rename nil" test-expr
	version-name file-delete
	#f set-version-control
	fname :if-exists :rename :fam w/o io-open io-close
	fname "~" $+ to version-name
	version-name file-exists? not "io-open rename #f" test-expr
	version-name file-delete
	undef set-version-control
	fname :if-exists :rename :fam w/o io-open io-close
	fname to version-name
	version-name file-exists? not "io-open rename undef" test-expr
	version-name file-delete
	\ io-open-file, io-open-input-file, io-open-output-file
	fname io-open-write io-close
	:filename fname io-open-file to io
	io io-input? not "io-open-file not readable?" test-expr
	io io-close
	:string "io-open-input-file" io-open-input-file to io
	io io-input? not "io-open-input-file not readable?" test-expr
	io io-read to s1
	io io->string to s2
	s1 s2 string<> "io-open-input-file" test-expr
	io io-close
	:filename fname io-open-output-file to io
	io io-output? not "io-open-output-file not writeable?" test-expr
	io io-close
	\ io-open :if-exists :error,  make-file-port (alias), io-close
	fname :fam r/w io-open to io
	io io-input?  not "not io-input? (r/w)"  test-expr
	io io-output? not "not io-output? (r/w)" test-expr
	io "fth-test" io-write
	io io-rewind
	io io-read "fth-test" string<> "r/w io-open" test-expr
	io io-closed? "io-closed? (open)" test-expr
	io io-close
	io io-closed? not "io-closed? (closed)" test-expr
	fname :if-exists :error :fam w/o <'> io-open 'io-error nil fth-catch if
		stack-reset
	else
		dup io? if
			( io ) io-close
			#t "io-open :error still open?" test-expr
		else
			( ex ) 1 >list "io-open :error %s?" swap
			    test-expr-format
		then
	then
	fname :fam r/o io-open io-close
	fname io-open-read     io-close
	fname :fam w/o io-open io-close
	fname io-open-write    io-close
	fname :fam a/o io-open io-close
	fname :fam r/a io-open io-close
	\ io-open-read,  make-file-input-port (alias)
	fname io-open-read to io
	io io-input?  not "io-open-read not io-input? (r/o)" test-expr
	io io-output?     "io-open-read io-output? (r/o)"    test-expr
	io io-close
	\ io-open-write,  make-file-output-port (alias)
	fname io-open-write to io
	io io-input?      "io-open-write io-input? (r/o)"      test-expr
	io io-output? not "io-open-write not io-output? (r/o)" test-expr
	io io-close
	\ io-reopen
	fname io-open-write to io
	io io-input?      "io-input? (w/o)"      test-expr
	io io-output? not "io-output? (w/o) not" test-expr
	"foo, bar, baz\n" to s1
	"baz, bar, foo\n" to s2
	io s1 io-write
	nil to io1
	io "new-name" io-reopen to io1
	io1 s2 io-write
	io1 io-close
	fname readlines #( s1 ) array= not "freopen" test-expr
	"new-name" readlines #( s2 ) array= not "freopen new-name" test-expr
	"new-name" file-delete
	\ io-fdopen
	fname io-open-read to io
	io io-fileno :fam r/o <'> io-fdopen #t nil fth-catch if
		stack-reset
	else
		to io1
		io1 io? if
			io1 io-close
		else
			"fdopen %s?" #( io1 ) test-expr-format
		then
		io1 io-closed? not "fdopen not closed?" test-expr
	then
	io io-close
	\ XXX: Minix pipe
	\ Pipe-issues in scripts with Minix.
	\ It works on command line.
	'minix provided? unless
		\ io-popen, make-pipe-port (alias)
		*io-test-pwd-cmd* :fam r/o io-popen to io
		io io-input?  not "io-popen not input? (r/o pipe)" test-expr
		io io-output?     "io-popen output? (r/o pipe)"    test-expr
		io io-read string-chomp file-pwd string<>
		    "io-popen `pwd`" test-expr
		io io-close
		"cat > " fname $+ :fam w/o io-popen to io
		io io-input?      "io-popen input? (w/o pipe)"      test-expr
		io io-output? not "io-popen not output? (w/o pipe)" test-expr
		io "fth-test pipe input (w/o)\n" io-write
		io io-close
		fname io-open-read to io
		io io-read to line
		io io-close
		line "fth-test pipe input (w/o)\n" string<>
		    "io-popen (w/o) `cat > name`" test-expr
		\ io-popen-read, make-pipe-input-port (alias)
		*io-test-pwd-cmd* io-popen-read to io
		io io-read string-chomp file-pwd string<>
		    "io-popen-read `pwd`" test-expr
		io io-close
		\ io-popen-write, make-pipe-output-port (alias)
		"cat > " fname $+ io-popen-write to io
		io "fth-test pipe input (io-popen-write)\n" io-write
		io io-closed?     "io-closed? (io-popen-write 1)" test-expr
		io io-close
		io io-closed? not "io-closed? (io-popen-write 2)" test-expr
		fname io-open-read to io
		io io-read to line
		line "fth-test pipe input (io-popen-write)\n" string<>
		    "io-popen-write `cat > name`" test-expr
		io io-close
	then
	\ io-sopen, make-string-port (alias)
	"out string comes\nhere" to s1
	s1 string-copy to s2
	s1 string-length to slen
	s1 :fam r/w io-sopen to io
	io io?        not "io-sopen not io?"    test-expr
	io io-input?  not "io-sopen not input?" test-expr
	io io-output? not "io-sopen output?"    test-expr
	io 3 :whence SEEK_SET io-seek 3 <> "io-sopen (io-seek 3)" test-expr
	io io-rewind
	io io-pos-ref 0 <> "io-sopen (io-seek 0)" test-expr
	io 4 io-pos-set!
	io "strang" io-write
	s1 string-length slen <> "io-sopen(4 len slen)" test-expr
	s1 s2 string= "io-sopen(4 s1 s2)" test-expr
	io io->string s1 string<> "io-sopen(4 io->string s1)" test-expr
	s1 "out strang comes\nhere" string<> "io-sopen(4 s1 value)" test-expr
	io 17 io-pos-set!
	io "there and here" io-write
	s1 string-length slen = "io-sopen(17 len slen)" test-expr
	s1 s2 string= "io-sopen(17 s1 s2)" test-expr
	io io->string s1 string<> "io-sopen(17 io->string s1)" test-expr
	s1 "out strang comes\nthere and here" string<>
	    "io-sopen(17 s1 value)" test-expr
	io io-close
	\ io-sopen-read, make-string-input-port (alias)
	s1 io-sopen-read to io
	io io-input? not "io-sopen-read not readable?" test-expr
	io io-close
	\ io-sopen-write, make-string-output-port (alias)
	s1 io-sopen-write to io
	io io-output? not "io-sopen-write not writeable?" test-expr
	io io-close
	\ sockets
	socket-test
	\ io-exit-status, exit-status (alias)
	"ls" file-shell drop
	exit-status 0<> "exit-status -> \"ls\" shell <> 0" test-expr
	"ls aGZ 2>&1 > /dev/null" file-shell drop
	exit-status 0= "exit-status -> \"ls aGZ\" shell == 0" test-expr
	\ io=
	fname io-open-read to io
	fname io-open-write to io1
	fname io-open-write to io2
	io  io  io= not "io  io  io=" test-expr
	io  io1 io=     "io  io1 io=" test-expr
	io1 io2 io= not "io1 io2 io=" test-expr
	#( io io1 io2 ) each
		io-close
	end-each
	\ io-putc|getc, io-flush
	fname :fam r/w io-open to io
	io <char> f io-putc
	io <char> t io-putc
	io <char> h io-putc
	\ XXX: Minix "w+"
	\ Issues with "w+" (more such cases below)
	'minix provided? if
		io io-close
		fname io-open-read to io
	else
		io io-flush
		io io-rewind
	then
	io io-getc <char> f <> "io-putc|getc f" test-expr
	io io-getc <char> t <> "io-putc|getc t" test-expr
	io io-getc <char> h <> "io-putc|getc h" test-expr
	io io-close
	\ io-read|write(-format)
	fname :fam r/w io-open to io
	io "fth-test io-write" io-write
	io test-rewind
	io io-read "fth-test io-write" string<> "io-read|write" test-expr
	io test-rewind
	io "fth-test %s" #( <'> io-write-format xt->name ) io-write-format
	io test-rewind
	io io-read "fth-test io-write-format" string<>
	    "io-read|write-format" test-expr
	io io-close
	\ io-readlines|writelines|eof?
	#( "line 1\n" "line 2\n" "line 3\n" ) to lines
	fname :fam r/w io-open to io
	io lines io-writelines
	'minix provided? if
		io io-close
		fname io-open-read to io
	then
	io io-readlines to lines1
	lines1 lines array= not "io-readlines|writelines" test-expr
	io io-eof? not "io-eof?" test-expr
	io io-close
	\ io-seek|pos
	fname io-open-write to io
	io io-pos-ref 0<> "io-open-write io-pos-ref (0)" test-expr
	io "fth-test" io-write
	io io-pos-ref 8 <> "io-write io-pos-ref (8)" test-expr
	io 3 :whence SEEK_SET io-seek 3 <> "3 SEEK_SET io-seek" test-expr
	io io-pos-ref 3 <> "3 io-pos-ref" test-expr
	io -3 :whence SEEK_END io-seek 8 3 - <>
	    "8 3 - SEEK_END io-seek" test-expr
	io io-pos-ref 8 3 - <> "8 3 - io-pos-ref" test-expr
	io 2 :whence SEEK_CUR io-seek 8 3 - 2 + <>
	    "8 3 - 2 + SEEK_CUR io-seek" test-expr
	io io-pos-ref 8 3 - 2 + <> "8 3 - 2 + io-pos-ref" test-expr
	io 4 io-pos-set!
	io io-pos-ref 4 <> "4 io-pos-set!" test-expr
	io io-close
	\ readlines|writelines
	#( "line 1\n" "line 2\n" "line 3\n" ) to lines
	fname lines writelines
	fname readlines to lines1
	lines1 lines array= not "readlines|writelines" test-expr
	\ write soft port, *stdout*, set-*stdout*
	fname :fam w/o io-open to *io*
	io-test-stdout-port set-*stdout* to old-stdout
	." \ "
	"Hello" .string
	." , World! (stdout)"
	cr
	*io* io-close
	old-stdout set-*stdout* drop
	\ read soft port, *stdin*, set-*stdin*
	fname :fam r/o io-open to *io*
	io-test-stdin-port set-*stdin* to old-stdin
	*stdin* io-read to line
	line "\\ Hello, World! (stdout)\n" string<> "soft port" test-expr
	*io* io-close
	old-stdin set-*stdin* drop
	\ with-input|output-port
	ftest file-delete
	<'> test-with-output-port :filename ftest with-output-port
	<'> test-with-input-port :filename ftest with-input-port
	    "hello" string<>
	    "with-input|output-port (:file and return)" test-expr
	ftest readlines 0 array-ref "hello" string<>
	    "with-input|output-port (:file)" test-expr
	ftest file-delete
	"hello" :filename ftest with-output-port
	nil     :filename ftest with-input-port "hello" string<>
	    "with-input|output-port (:file and str-return)" test-expr
	ftest readlines 0 array-ref "hello" string<>
	    "with-input|output-port (:file str)" test-expr
	"" to s1
	<'> test-with-output-port :string s1 with-output-port
	<'> test-with-input-port  :string s1 with-input-port "hello" string<>
	"with-input|output-port (:string and return)" test-expr
	s1 "hello" string<> "with-input|output-port (:string)" test-expr
	"" to s1
	"hello" :string s1 with-output-port
	nil     :string s1 with-input-port "hello" string<>
	    "with-input|output-port (:string and str-return)" test-expr
	s1 "hello" string<> "with-input|output-port (:string str)" test-expr
	\ with-input-from-port, with-output|error-to-port
	ftest file-delete
	<'> test-with-output-to-port  :filename ftest with-output-to-port
	<'> test-with-input-from-port :filename ftest with-input-from-port
	    "hello" string<>
	    "with-input|output-to-port (:file and return)" test-expr
	ftest readlines 0 array-ref "hello" string<>
	    "with-input|output-to-port (:file)" test-expr
	ftest file-delete
	"hello" :filename ftest with-output-to-port
	nil     :filename ftest with-input-from-port "hello" string<>
	    "with-input|output-to-port (:file and str-return)" test-expr
	ftest readlines 0 array-ref "hello" string<>
	    "with-input|output-to-port (:file str)" test-expr
	ftest file-delete
	<'> test-with-error-to-port   :filename ftest with-error-to-port
	<'> test-with-input-from-port :filename ftest with-input-from-port
	    "hello" string<>
	    "with-input|error-to-port (:file and return)" test-expr
	ftest readlines 0 array-ref "hello" string<>
	    "with-input|error-to-port (:file)" test-expr
	ftest file-delete
	"hello" :filename ftest with-error-to-port
	nil     :filename ftest with-input-from-port "hello" string<>
	    "with-input|error-to-port (:file and str-return)" test-expr
	ftest readlines 0 array-ref "hello" string<>
	    "with-input|error-to-port (:file str)" test-expr
	"" to s1
	<'> test-with-output-to-port  :string s1 with-output-to-port
	<'> test-with-input-from-port :string s1 with-input-from-port
	    "hello" string<>
	    "with-input|output-to-port (:string and return)" test-expr
	s1 "hello" string<> "with-input|output-to-port (:string)" test-expr
	"" to s1
	"hello" :string s1 with-output-to-port
	nil     :string s1 with-input-from-port "hello" string<>
	    "with-input|output-to-port (:string and str-return)" test-expr
	s1 "hello" string<> "with-input|output-to-port (:string str)" test-expr
	"" to s1
	<'> test-with-error-to-port   :string s1 with-error-to-port
	<'> test-with-input-from-port :string s1 with-input-from-port
	    "hello" string<>
	    "with-input|error-to-port (:string and return)" test-expr
	s1 "hello" string<> "with-input|error-to-port (:string)" test-expr
	"" to s1
	"hello" :string s1 with-error-to-port
	nil     :string s1 with-input-from-port "hello" string<>
	    "with-input|error-to-port (:string and str-return)" test-expr
	s1 "hello" string<> "with-input|error-to-port (:string str)" test-expr
	ftest file-delete
	fname file-delete
;

*fth-test-count* 0> [if] io-test [then]

\ io-test.fs ends here
