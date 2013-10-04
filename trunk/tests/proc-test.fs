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
\ @(#)proc-test.fs	1.25 9/13/13

require test-utils.fs

*filename*                value proc-test-filename
: proc-test-xt ; *lineno* value proc-test-lineno

: proc-test ( -- )
  \ word?, proc?, thunk?, xt?, make-proc
  <'> f** word? not "word? f**?" test-expr
  <'> f** xt?   not "xt? f**?"   test-expr
  nil proc?         "proc? nil"  test-expr
  #() proc?         "proc? #()"  test-expr
  <'> noop 0 make-proc { prc0 }
  prc0 proc? not "proc? noop" test-expr
  <'> f+ 2 make-proc { prc2 }
  prc2 "" proc-source-set!
  prc0 thunk? not "prc0 not thunk?" test-expr
  prc2 thunk?     "prc2 thunk?"     test-expr
  \ proc-arity
  <'> noop 0 make-proc proc-arity #( 0 0 #f ) array= not "proc-arity #( 0 0 #f )" test-expr
  <'> noop 3 make-proc proc-arity #( 3 0 #f ) array= not "proc-arity #( 3 0 #f )" test-expr
  \ proc-name
  <'> f+ 2 make-proc proc-name "f+" string<> "proc-name (+)" test-expr
  \ proc-source-ref|set!
  <'> noop #f proc-source-set!
  <'> f+   #f proc-source-set!
  <'> noop 0 make-proc to prc0
  <'> f+   2 make-proc to prc2
  prc0 proc-source-ref "noop" string<> "proc-source-ref (noop)" test-expr
  prc2 proc-source-ref "f+"   string<> "proc-source-ref (f+)"   test-expr
  prc0 "we do nothing" proc-source-set!
  prc0 proc-source-ref "we do nothing" string<> "proc-source-set! (we do nothing)" test-expr
  prc2 "( a b -- c ) f+ " proc-source-set!
  prc2 proc-source-ref "( a b -- c ) f+ " string<> "proc-source-set! ( a b -- c ) f+" test-expr
  \ proc->xt
  <'> noop 0 make-proc proc->xt <'> noop <> "proc->xt" test-expr
  <'> f* 2 make-proc { f*prc }
  f*prc #( 1.5 1.5 ) run-proc 2.25 f<> "run-proc f*" test-expr
  <'> noop 0 make-proc { noop-prc }
  noop-prc #() run-proc #f <> "run-proc noop" test-expr
  \ run-proc, proc-apply (alias)
  prc2 #( 0 1 ) run-proc 1 f<> "0 1 run-proc" test-expr
  prc1 #( 2 3 ) run-proc 5  <> "2 3 run-proc" test-expr
  \ <'set>|set!|set-xt|set-execute
  object-print-length { old-len }
  \ <'set>
  32 <'set> object-print-length execute
  object-print-length 32 <> "<'set> 32" test-expr
  old-len <'set> object-print-length execute
  \ set!
  31 set! object-print-length
  object-print-length 31 <> "set! 31" test-expr
  old-len set! object-print-length
  \ set-xt
  30 <'> object-print-length set-xt execute
  object-print-length 30 <> "set-execute: 30" test-expr
  old-len <'> object-print-length set-xt execute
  \ set-execute
  29 <'> object-print-length set-execute
  object-print-length 29 <> "set-execute: 29" test-expr
  old-len set! object-print-length
  object-print-length old-len <> "set! reset" test-expr
  \ xt->name
  <'> noop xt->name "noop" string<> "xt->name" test-expr
  \ xt->origin
  <'> proc-test-xt { xt }
  "%s:%s:%d" #( xt xt->name proc-test-filename proc-test-lineno ) string-format { str }
  xt xt->origin str string<> "xt->origin" test-expr
  \ help-ref, help-set!
  "( n1 n2 -- n3 )  this is our doc string" to str
  <'> + str help-set!
  <'> + help-ref "+  " str $+ string<> "help-ref|set!" test-expr
  \ documentation-ref|set!
  'fth-test "fth-test doc" documentation-set!
  'fth-test documentation-ref "fth-test doc" string<> "documentation-set! (symbol)" test-expr
  <'> hash1-test "fth-test doc" documentation-set!
  <'> hash1-test documentation-ref "fth-test doc" string<> "documentation-set! (xt)" test-expr
  make-hash { hs }
  hs "fth-test doc" documentation-set!
  hs documentation-ref "fth-test doc" string<> "documentation-set! (object)" test-expr
  \ source-ref|set!
  <'> f+ to xt
  xt 2 make-proc { prc }
  "( r1 r2 -- r3 ) f+" { src }
  xt  src source-set!
  xt  source-ref src string<> "source-set! (xt)"  test-expr
  prc src source-set!
  prc source-ref src string<> "source-set! (prc)" test-expr
  xt "" source-set!
  \ get-optkey
  0 1 optkey-test { val }
  val #( 0 1 440.0 pi ) array= not "get-optkey (1)" test-expr
  0 2 :frequency 330.0 optkey-test to val
  val #( 0 2 330.0 pi ) array= not "get-optkey (2)" test-expr
  \ get-optarg
  1 2 optarg-test to val
  val #( 1 2 33 44 55 ) array= not "get-optarg (1)" test-expr
  1 2 3 4 optarg-test to val
  val #( 1 2 3 4 55 )   array= not "get-optarg (2)" test-expr
  1 2 3 4 5 6 7 optarg-test to val
  { rest1 rest2 }			\ 1 2
  val #( 3 4 5 6 7 )    array= not "get-optarg (3)" test-expr
  rest1 1 <> "get-optarg (3) rest1" test-expr
  rest2 2 <> "get-optarg (3) rest2" test-expr
  \ get-optkeys
  0 1 optkeys-test to val
  val #( 0 1 440.0 pi ) array= not "get-optkeys (1)" test-expr
  0 2 :frequency 330.0 optkeys-test to val
  val #( 0 2 330.0 pi ) array= not "get-optkeys (2)" test-expr
  \ get-optargs
  1 2 optargs-test to val
  val #( 1 2 33 44 55 ) array= not "get-optargs (1)" test-expr
  1 2 3 4 optargs-test to val
  val #( 1 2 3 4 55 )   array= not "get-optargs (2)" test-expr
  1 2 3 4 5 6 7 optargs-test to val
  { rest1 rest2 }			\ 1 2
  val #( 3 4 5 6 7 )    array= not "get-optargs (3)" test-expr
  rest1 1 <> "get-optargs (3) rest1" test-expr
  rest2 2 <> "get-optargs (3) rest2" test-expr
;

*fth-test-count* 0 [do] proc-test [loop]

\ proc-test.fs ends here
