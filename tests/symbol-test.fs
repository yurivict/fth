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
\ @(#)symbol-test.fs	1.12 9/13/13

require test-utils.fs

: symbol-test ( -- )
  \ symbol?
  nil symbol? "nil symbol?" test-expr
  #() symbol? "#() symbol?" test-expr
  0   symbol? "0 symbol?"   test-expr
  'symbol symbol? not "'symbol symbol?" test-expr
  \ symbol=
  "sym1" make-symbol { s1 }
  "sym1" make-symbol { s2 }
  "sym2" make-symbol { s3 }
  s1 s1 symbol= not "s1 s1 symbol=" test-expr
  s1 s2 symbol= not "s1 s2 symbol=" test-expr
  s1 s3 symbol=     "s1 s3 symbol=" test-expr
  \ make-symbol
  "hello" make-symbol { sym }
  sym symbol? not "make-symbol" test-expr
  \ symbol-name
  sym symbol-name "hello" string<> "symbol-name" test-expr
  \ keyword?
  nil keyword? "nil keyword?" test-expr
  #() keyword? "#() keyword?" test-expr
  0   keyword? "0 keyword?"   test-expr
  :keyword keyword? not ":keyword keyword?" test-expr
  \ keyword=
  "key1" make-keyword { k1 }
  "key1" make-keyword { k2 }
  "key2" make-keyword { k3 }
  k1 k1 keyword= not "k1 k1 keyword=" test-expr
  k1 k2 keyword= not "k1 k2 keyword=" test-expr
  k1 k3 keyword=     "k1 k3 keyword=" test-expr
  \ make-keyword
  "hello" make-keyword { key }
  key keyword? not "make-keyword" test-expr
  \ keyword-name
  key keyword-name "hello" string<> "keyword-name" test-expr
  \ exception?
  nil exception? "nil exception?" test-expr
  #() exception? "#() exception?" test-expr
  0   exception? "0 exception?"   test-expr
  'ex symbol->exception exception? not "'ex exception?" test-expr
  \ exception=
  "exc1" #f make-exception { e1 }
  "exc1" #f make-exception { e2 }
  "exc2" #f make-exception { e3 }
  e1 e1 exception= not "e1 e1 exception=" test-expr
  e1 e2 exception= not "e1 e2 exception=" test-expr
  e1 e3 exception=     "e1 e3 exception=" test-expr
  \ make-exception, symbol->exception
  'foo symbol->exception { ex }
  ex exception? not "'foo exception?" test-expr
  "ex-bar" "no message" make-exception to ex
  ex exception? not "'ex-bar exception?" test-expr
  \ exception-name, exception-message-ref|set!
  e1 exception-name "exc1" string<> "exception-name" test-expr
  e1 exception-message-ref "exc1" string<> "exception-message-ref" test-expr
  e1 "new message" exception-message-set!
  e1 exception-message-ref "new message" string<> "exception-message-set!" test-expr
  e1 #f exception-message-set!
  e1 exception-message-ref "exc1" string<> "exception-message-set! (reset)" test-expr
  \ exception-last-message-ref|set!
  e1 "new last message" exception-last-message-set!
  e1 exception-last-message-ref "new last message" string<>
  "exception-last-message-set!" test-expr
  e1 #f exception-last-message-set!
  e1 exception-last-message-ref "exc1" string<> "exception-last-message-set! (reset)" test-expr
;

*fth-test-count* 0 [do] symbol-test [loop]

\ symbol-test.fs ends here
