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
\ @(#)hook-test.fs	1.18 9/13/13

require test-utils.fs

: hook-test ( -- )
  \ hook?
  0 make-hook { hook0 }
  0 make-hook { hook1 }
  2 make-hook { hook2 }
  nil hook? "nil hook?" test-expr
  #() hook? "#() hook?" test-expr
  hook0 hook? not "hook? hook0" test-expr
  \ create-hook
  test-hook hook? not "create-hook: not a hook" test-expr
  test-hook <'> test-hook-proc1 add-hook!
  test-hook <'> test-hook-proc2 <'> add-hook! #t nil fth-catch not
  "add-hook with arity #( 1 0 0 ) proc with arity #( 2 0 0 )" test-expr
  test-hook object-length 1 <> "create-hook length 1 <>" test-expr
  test-hook <'> test-hook-proc1 hook-delete drop
  test-hook object-length 0<> "create-hook length 0<>" test-expr
  \ make-hook
  #( 2 0 #f ) make-hook { hook4 }
  hook4 hook? not "make-hook: not a hook" test-expr
  hook4 <'> test-hook-proc1 <'> add-hook! #t nil fth-catch not
  "make-hook #( 1 0 0 ) add-hook! #( 2 0 0 )" test-expr
  hook4 <'> test-hook-proc2 add-hook!
  hook4 object-length 1 <> "make-hook length 1 <>" test-expr
  \ hook=
  hook0 hook0 hook= not "hook= 0 0" test-expr
  hook0 hook1 hook= not "hook= 0 1" test-expr
  hook0 hook2 hook=     "hook= 0 2" test-expr
  \ hook->array, hook-arity
  hook0 hook->array #() array= not "hook->array (0)" test-expr
  <'> noop 0 make-proc { prc0 }
  hook0 prc0 add-hook!
  hook0 hook->array #( prc0 ) array= not "hook->array (1)" test-expr
  hook0 hook-arity #( 0 0 #f ) array= not "hook-arity (0)" test-expr
  hook2 hook-arity #( 2 0 #f ) array= not "hook-arity (2)" test-expr
  \ hook-name
  test-hook hook-name "test-hook" string<> "hook-name: test-hook <>" test-expr
  \ add-hook!
  <'> f+ 2 make-proc { prc2 }
  hook2 prc2 add-hook!
  hook2 hook-names #( "f+" ) object-equal? not "add-hook! (\"f+\")" test-expr
  hook2 hook-procs #( prc2 ) object-equal? not "add-hook! (f+)" test-expr
  hook2 object-length 1 <> "add-hook! (len)" test-expr
  hook2 <'> f+ 3 make-proc <'> add-hook! #t nil fth-catch not "add-hook! (2 != 3)" test-expr
  \ remove-hook!, hook-member?, hook-empty?
  <'> noop 0 make-proc { prc }
  hook1 prc add-hook!
  hook1 prc      hook-member? not "hook-member? (prc)"  test-expr
  hook1 <'> noop hook-member? not "hook-member? (xt)"   test-expr
  hook1 "noop"   hook-member? not "hook-member? (name)" test-expr
  hook1 prc remove-hook! { prc1 }
  prc1 prc <> "remove-hook! (prc)"  test-expr
  hook1 prc add-hook!
  hook1 <'> noop remove-hook! { prc1 }
  prc1 prc <> "remove-hook! (xt)"   test-expr
  hook1 prc add-hook!
  hook1 hook-empty? "hook-empty?"   test-expr
  hook1 "noop" remove-hook! { prc1 }
  prc1 prc <> "remove-hook! (name)" test-expr
  hook1 hook-empty? not "hook-empty?" test-expr
  \ run-hook, reset-hook!
  hook2 object-length { len }	\ contains <'> f+ from section add-hook! above
  hook2 reset-hook!
  hook2 object-length { len1 }
  len1 len = "reset-hook!" test-expr
  hook2 <'> f* 2 make-proc add-hook!
  hook2 #( 2.0 pi ) run-hook 0 array-ref two-pi f<> "run-hook (1)" test-expr
  hook2 <'> f+ 2 make-proc add-hook!
  hook2 #( 2.0 pi ) run-hook { lst }
  lst two-pi array-member?
  lst pi 2.0 f+ array-member? && not "run-hook (2)" test-expr
  \ hook-names
  hook2 hook-names to lst
  lst <'> f* xt->name array-member?
  lst <'> f+ xt->name array-member? && not "hook-names" test-expr
  hook2 object-length 2 <> "hook length 2 <>" test-expr
  hook2 reset-hook!
  hook2 object-length  0<> "hook length 0<>"  test-expr
  \ hook-procs
  hook2 <'> f+ 2 make-proc add-hook!
  hook2 <'> f* 2 make-proc add-hook!
  hook2 hook-procs to lst
  lst <'> f* array-member?
  lst <'> f+ array-member? && not "hook-procs" test-expr
  hook2 object-length 2 <> "hook length 2 <>" test-expr
  hook2 reset-hook!
  hook2 object-length  0<> "hook length 0<>"  test-expr
;

*fth-test-count* 0 [do] hook-test [loop]

\ hook-test.fs ends here
