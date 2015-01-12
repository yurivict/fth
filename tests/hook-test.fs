\ Copyright (c) 2006-2015 Michael Scholz <mi-scholz@users.sourceforge.net>
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
\ @(#)hook-test.fs	1.21 1/12/15

require test-utils.fs

#( 1 0 #f ) "our test hook" create-hook test-hook

: test-hook-proc1 <{ arg -- val }>
	arg 2*
;

: test-hook-proc2 <{ arg1 arg2 -- val }>
	arg1 arg2 *
;

: hook-test ( -- )
	nil nil nil nil { hk0 hk1 hk2 hk3 }
	nil nil nil nil { prc0 prc1 prc2 }
	nil nil { len lst }
	\ hook?
	0 make-hook to hk0
	0 make-hook to hk1
	2 make-hook to hk2
	nil hook? "nil hook?" test-expr
	#() hook? "#() hook?" test-expr
	hk0 hook? not "hook? hk0" test-expr
	\ create-hook
	test-hook hook? not "create-hook (1)" test-expr
	test-hook <'> test-hook-proc1 add-hook!
	test-hook <'> test-hook-proc2 <'> add-hook! #t nil fth-catch not
	    "add-hook #( 1 0 0 ) with #( 2 0 0 ) arity" test-expr
	test-hook object-length 1 <> "hook length (0)" test-expr
	test-hook <'> test-hook-proc1 hook-delete <'> test-hook-proc1 <>
	    "hook-delete (1)" test-expr
	test-hook object-length 0<> "hook-delete (2)" test-expr
	\ make-hook
	#( 2 0 #f ) make-hook to hk3
	hk3 hook? not "make-hook (1)" test-expr
	hk3 <'> test-hook-proc1 <'> add-hook! #t nil fth-catch not
	    "make-hook #( 1 0 0 ) with #( 2 0 0 ) arity" test-expr
	hk3 <'> test-hook-proc2 add-hook!
	hk3 object-length 1 <> "make-hook length 1 <>" test-expr
	\ hook=
	hk0 hk0 hook= not "hook= 0 0" test-expr
	hk0 hk1 hook= not "hook= 0 1" test-expr
	hk0 hk2 hook=     "hook= 0 2" test-expr
	\ hook->array, hook-arity
	hk0 hook->array #() array= not "hook->array (0)" test-expr
	<'> noop 0 make-proc to prc0
	hk0 prc0 add-hook!
	hk0 hook->array #( prc0 ) array= not "hook->array (1)" test-expr
	hk0 hook-arity #( 0 0 #f ) array= not "hook-arity (0)" test-expr
	hk2 hook-arity #( 2 0 #f ) array= not "hook-arity (2)" test-expr
	\ hook-name
	test-hook hook-name "test-hook" string<> "hook-name" test-expr
	\ add-hook! (alias hook-add), hook-names, hook-procs
	<'> f+ 2 make-proc to prc1
	hk2 prc1 add-hook!
	hk2 hook-names #( "f+" ) object-equal? not "add-hook! (1)" test-expr
	hk2 hook-procs #( prc1 ) object-equal? not "add-hook! (2)" test-expr
	hk2 object-length 1 <> "add-hook! (3)" test-expr
	hk2 <'> f+ 3 make-proc <'> add-hook! #t nil fth-catch not
	    "add-hook! (4)" test-expr
	\ remove-hook! (alias hook-delete), hook-member?, hook-empty?
	<'> noop 0 make-proc to prc2
	hk1 prc2 add-hook!
	hk1 prc2     hook-member? not "hook-member? (1)" test-expr
	hk1 <'> noop hook-member? not "hook-member? (2)" test-expr
	hk1 "noop"   hook-member? not "hook-member? (3)" test-expr
	hk1 prc2 remove-hook! to prc1
	prc1 prc2 <> "remove-hook! (1)" test-expr
	hk1 prc2 add-hook!
	hk1 <'> noop remove-hook! to prc1
	prc1 prc2 <> "remove-hook! (2)" test-expr
	hk1 prc2 add-hook!
	hk1 hook-empty? "hook-empty? (1)" test-expr
	hk1 "noop" remove-hook! to prc1
	prc1 prc2 <> "remove-hook! (3)" test-expr
	hk1 hook-empty? not "hook-empty? (2)" test-expr
	\ reset-hook! (alias hook-clear), run-hook (alias hook-apply)
	hk2 object-length to len \ contains <'> f+ from section add-hook! above
	hk2 reset-hook!
	hk2 object-length len = "reset-hook!" test-expr
	hk2 <'> f* 2 make-proc add-hook!
	hk2 #( 2.0 pi ) run-hook 0 array-ref two-pi f<> "run-hook (1)" test-expr
	hk2 <'> f+ 2 make-proc add-hook!
	hk2 #( 2.0 pi ) run-hook to lst
	lst two-pi array-member?
	lst pi 2.0 f+ array-member? && not "run-hook (2)" test-expr
	\ hook-names
	hk2 hook-names to lst
	lst <'> f* xt->name array-member?
	lst <'> f+ xt->name array-member? && not "hook-names" test-expr
	hk2 object-length 2 <> "hook-names (1)" test-expr
	hk2 reset-hook!
	hk2 object-length 0<> "hook-names (2)" test-expr
	\ hook-procs (aliases hook->array and hook->list)
	hk2 <'> f+ 2 make-proc add-hook!
	hk2 <'> f* 2 make-proc add-hook!
	hk2 hook-procs to lst
	lst <'> f* array-member?
	lst <'> f+ array-member? && not "hook-procs" test-expr
	hk2 object-length 2 <> "hook length (3)" test-expr
	hk2 reset-hook!
	hk2 object-length 0<> "hook length (4)" test-expr
;

*fth-test-count* 0 [do] hook-test [loop]

\ hook-test.fs ends here
