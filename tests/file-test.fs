\ Copyright (c) 2006-2012 Michael Scholz <mi-scholz@users.sourceforge.net>
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
\ @(#)file-test.fs	1.26 9/13/13

require test-utils.fs

: file-test ( -- )
	nil { old-verb }
	nil { cur-path }
	nil nil { dir1 dir2 }
	nil { lines }
	nil { io }
	nil { dname }
	nil { fifo }
	nil { sfile }
	\ file-delete
	fname current-time file-touch
	fname file-delete
	fname file-exists? "file-delete fname not deleted" test-expr
	\ file-chmod
	fname current-time file-touch
	fname 0o700 file-chmod
	fname file-executable? not "chmod 0700" test-expr
	fname file-delete
	\ file-mkdir|rmdir
	"foo" file-exists? if
		"foo" file-directory? if
			"foo" file-rmdir
		else
			"foo" file-delete
		then
	then
	"foo" 0o755 file-mkdir
	"foo" file-directory? not "mkdir foo" test-expr
	"foo" file-rmdir
	"foo" file-exists? "rmdir: foo not deleted?" test-expr
	\ file-mkfifo
	"foo" 0o644 file-mkfifo
	"foo" file-fifo? not "mkfifo foo" test-expr
	"foo" file-delete
	"foo" file-exists? "mkfifo: foo not deleted?" test-expr
	\ file-symlink
	fname "foo" file-symlink
	"foo" file-symlink? not "symlink foo" test-expr
	"foo" file-delete
	"foo" file-exists? "symlink: foo not deleted?" test-expr
	\ file-rename
	fname current-time file-touch
	fname "foo" file-rename
	fname file-exists?     "rename -> foo: not renamed" test-expr
	"foo" file-exists? not "rename -> foo: not renamed" test-expr
	"foo" fname file-rename
	fname file-exists? not "rename <- foo: not renamed" test-expr
	"foo" file-exists?     "rename <- foo: not renamed" test-expr
	\ file-copy
	fname "foo" file-copy
	fname file-exists? not "copy -> foo: not copied" test-expr
	"foo" file-exists? not "copy -> foo: not copied" test-expr
	"foo" file-delete
	\ file-install
	*fth-verbose* to old-verb
	#f to *fth-verbose*
	fname "foo" 0o644 file-install not
	    "install -> foo: not installed (1)" test-expr
	fname file-exists? not "install -> foo: not installed (2)" test-expr
	"foo" file-exists? not "install -> foo: not installed (3)" test-expr
	fname "foo" 0o755 file-install "install -> foo: reinstalled" test-expr
	old-verb to *fth-verbose*
	"foo" file-delete
	\ file-split
	"/home/mike/cage.snd" file-split #( "/home/mike" "cage.snd" ) array= not
	    "file-split (1)" test-expr
	"../cage.snd" file-split #( ".." "cage.snd" ) array= not
	    "file-split (2)" test-expr
	\ file-basename
	"/home/mike/cage.snd" ".snd" file-basename "cage" string<>
	    "file-basename (1)" test-expr
	"../cage.snd" ".snd" file-basename "cage" string<>
	    "file-basename (2)" test-expr
	"../cage.snd" #f file-basename "cage.snd" string<>
	    "file-basename (3)" test-expr
	"/home/mike/cage.snd" nil file-basename "cage" string<>
	    "file-basename (4)" test-expr
	"/home/mike/cage.snd" #f file-basename "cage.snd" string<>
	    "file-basename (5)" test-expr
	"/home/mike/cage.snd" "nd" file-basename "cage.s" string<>
	    "file-basename (6)" test-expr
	\ file-dirname
	"/home/mike/cage.snd" file-dirname "/home/mike" string<>
	    "file-dirname (1)" test-expr
	"../cage.snd" file-dirname ".." string<> "file-dirname (2)" test-expr
	\ file-fullpath, file-pwd
	"cage.snd" file-fullpath file-pwd "/cage.snd" $+ string<>
	    "file-fullpath (1)" test-expr
	file-pwd "/cage.snd" $+ file-fullpath file-pwd "/cage.snd" $+ string<>
	    "file-fullpath (2)" test-expr
	\ file-realpath
	file-pwd to cur-path
	\ NetBSD has no /usr/local but /usr/pkg
	"/usr/local" file-exists? if
		"/usr/local"
	else
		"/usr/pkg"
	then file-chdir
	"../bin" file-realpath "/usr/bin" string<> "file-realpath" test-expr
	cur-path file-chdir
	\ file-chdir
	"foo" 0o755 file-mkdir
	file-pwd to dir1
	"foo" file-chdir
	file-pwd to dir2
	".." file-chdir
	dir1 dir2 string= "chdir" test-expr
	"foo" file-rmdir
	\ file-truncate
	fname file-delete
	fname #( "hello" ) writelines
	fname file-length 5 d<> "file-truncate (before)" test-expr
	fname 3 file-truncate
	fname file-length 3 d<> "file-truncate (after)" test-expr
	fname file-delete
	\ file-eval
	#( "30 20 +\n"
	   "10 * \n" ) to lines
	fname io-open-write to io
	io lines io-writelines
	io io-close
	fname file-eval 500 <> "file-eval" test-expr
	\ file-shell
	"pwd" file-shell string? not "file-shell" test-expr
	\ file test functions
	fname file-delete
	fname #( "empty\n" ) writelines
	\ file-block?
	fname file-block? "fname file-block?" test-expr
	nil   file-block? "nil file-block?"   test-expr
	10    file-block? "10 file-block?"    test-expr
	\ file-character?
	fname file-character? "fname file-character?" test-expr
	nil  file-character? "nil file-character?"    test-expr
	10   file-character? "10 file-character?"     test-expr
	\ file-directory?
	"new-dir" to dname
	dname 0o755 file-mkdir
	dname file-directory? not "dname file-directory?" test-expr
	nil  file-directory?      "nil file-directory?"   test-expr
	10   file-directory?      "10 file-directory?"    test-expr
	dname file-rmdir
	\ file-exists?
	fname file-exists? not "fname file-exists?" test-expr
	nil   file-exists?     "nil file-exists?"   test-expr
	10    file-exists?     "10 file-exists?"    test-expr
	\ file-fifo?
	"new-fifo" to fifo
	fifo 0o644 file-mkfifo
	fifo file-fifo? not "fifo file-fifo?" test-expr
	nil  file-fifo?     "nil file-fifo?"  test-expr
	10   file-fifo?     "10 file-fifo?"   test-expr
	fifo file-delete
	\ file-symlink?
	"new-symlink" to sfile
	fname sfile file-symlink
	sfile file-symlink? not "sfile file-symlink?" test-expr
	nil   file-symlink?     "nil file-symlink?"   test-expr
	10    file-symlink?     "10 file-symlink?"    test-expr
	sfile file-delete
	\ file-executable? file-readable? file-writable?
	fname 0o700 file-chmod
	fname file-executable? not "file0700 (exec)" test-expr
	fname file-readable?   not "file0700 (read)" test-expr
	fname file-writable?   not "file0700 (wrt)"  test-expr
	\ file-setuid?, file-setguid?
	fname 0o4700 file-chmod
	fname file-setuid? not "file4700 (suid)" test-expr
	fname 0o2700 file-chmod
	fname file-setgid? not "file2700 (sgid)" test-expr
	\ file-sticky?, file-zero?, file-length
	fname file-delete
	fname #( "hello" ) writelines
	fname file-zero?       "file-zero? (5 chars)"  test-expr
	fname file-length 5 d<> "file-length (5 chars)" test-expr
	dname 0o755 file-mkdir
	dname 0o1777 file-chmod
	dname file-sticky? not "file-sticky?" test-expr
	dname file-rmdir
	fname file-delete
;

'apple provided? [unless] *fth-test-count* 0> [if] file-test [then] [then]

\ file-test.fs ends here
