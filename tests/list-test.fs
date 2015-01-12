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
\ @(#)list-test.fs	1.22 1/11/15

require test-utils.fs

: list<> ( x y -- f )
	list= not
;

: list-test ( -- )
	nil nil nil { l1 l2 l3 }
	nil         { x }
	\ list-length
	'( 0 1 2 ) list-length  3 <> "list-length '( 0 1 2 )?" test-expr
	nil        list-length   0<> "list-length nil?" test-expr
	'()        list-length   0<> "list-length '()?" test-expr
	5          list-length -1 <> "list-length -5?" test-expr
	\ nil?
	'( 0 1 2 ) nil?     "'( 0 1 2 ) nil?" test-expr
	nil        nil? not "nil nil?" test-expr
	'()        nil? not "'() nil?" test-expr
	0          nil?     "0 nil?" test-expr
	\ list?
	#( 0 1 2 ) list?     "#( 0 1 2 ) list?" test-expr
	'( 0 1 2 ) list? not "'( 0 1 2 ) list?" test-expr
	nil        list? not "nil list?" test-expr
	'()        list? not "'() list?" test-expr
	0          list?     "0 list?" test-expr
	\ cons?
	#( 0 1 2 ) cons?     "#( 0 1 2 ) cons?" test-expr
	'( 0 1 2 ) cons? not "'( 0 1 2 ) cons?" test-expr
	nil        cons?     "nil cons?" test-expr
	'()        cons?     "'() cons?" test-expr
	0          cons?     "0 cons?" test-expr
	\ pair?
	#( 0 1 2 ) pair?     "#( 0 1 2 ) pair?" test-expr
	'( 0 1 2 ) pair? not "'( 0 1 2 ) pair?" test-expr
	nil        pair?     "nil pair?" test-expr
	'()        pair?     "'() pair?" test-expr
	0          pair?     "0 pair?" test-expr
	\ make-list
	0 make-list '() list<> "0 make-list" test-expr
	3 make-list '( nil nil nil ) list<> "3 make-list" test-expr
	3 :initial-element pi make-list '( pi pi pi ) list<>
	    "3 :initial-element pi make-list" test-expr
	100 make-list map!
		i
	end-map ( lst ) each to x
		i x <> "list-each [%d]: %s\n" #( i x ) test-expr-format
	end-each
	'foo <'> make-list 'wrong-type-arg nil fth-catch car 'wrong-type-arg <>
	    "'foo make-list" test-expr
	\ >list
	0 1 2 3 >list '( 0 1 2 ) list<> "0 1 2 3 >list" test-expr
	\ cons
	0 nil cons '( 0 ) list<> "0 nil cons" test-expr
	0 1 nil cons cons '( 0 1 ) list<> "0 1 nil cons cons" test-expr
	0 1 2 '() cons cons cons '( 0 1 2 ) list<>
	    "0 1 2 '() cons cons cons" test-expr
	\ cons2
	0 1 nil cons2 '( 0 1 ) list<> "0 1 nil cons2" test-expr
	0 1 2 nil cons cons2 '( 0 1 2 ) list<> "0 1 2 nil cons cons2" test-expr
	\ car
	'( 0 1 ) car 0<> "'( 0 1 ) car 0<>" test-expr
	'() car nil? not "'() car nil?" test-expr
	\ cadr
	'( 0 1 ) cadr 1 <> "'( 0 1 ) cadr 1 <>" test-expr
	'() cadr nil? not "'() cadr nil?" test-expr
	\ caddr
	'( 0 1 2 ) caddr 2 <> "'( 0 1 2 ) caddr 2 <>" test-expr
	'( 0 1 ) caddr nil? not "'( 0 1 ) caddr nil?" test-expr
	'() caddr nil? not "'() caddr nil?" test-expr
	\ cadddr
	'( 0 1 2 3 ) cadddr 3 <> "'( 0 1 2 3 ) caddr 3 <>" test-expr
	'( 0 1 2 ) cadddr nil? not "'( 0 1 2 ) cadddr nil?" test-expr
	'() cadddr nil? not "'() cadddr nil?" test-expr
	\ cdr
	'( 0 1 2 ) cdr '( 1 2 ) list<> "'( 0 1 2 ) cdr '( 1 2 )" test-expr
	'( 0 ) cdr nil? not "'( 0 ) cdr nil?" test-expr
	'() cdr nil? not "'() cdr nil?" test-expr
	\ cddr
	'( 0 1 2 ) cddr '( 2 ) list<> "'( 0 1 2 ) cddr '( 2 )" test-expr
	'( 0 1 ) cddr nil? not "'( 0 1 ) cddr nil?" test-expr
	'() cddr nil? not "'() cddr nil?" test-expr
	\ list=
	'( 0 1 2 ) to l1
	'( 0 1 2 ) to l2
	'( 0 1 3 ) to l3
	l1 l1 list<> "l1 l1 list<>" test-expr
	l1 l2 list<> "l1 l2 list<>" test-expr
	l1 l3 list=  "l1 l3 list=" test-expr
	\ set-car
	'( 0 1 2 ) 10 set-car! '( 10 1 2 ) list<>
	    "'( 0 1 2 ) 10 set-car! '( 10 1 2 )" test-expr
	'() 10 set-car! nil? not "'() 10 set-car! nil?" test-expr
	\ set-cdr!
	'( 0 1 2 ) 10 set-cdr! '( 0 ) 10 set-cdr! list<>
	    "'( 0 1 2 ) 10 set-cdr! '( 0 )" test-expr
	'() 10 set-cdr! nil? not "'() 10 set-cdr! nil?" test-expr
	'( 0 1 2 ) to l1
	l1 l1 length 1- list-tail '( 3 4 5 ) set-cdr! to l1
	l1 '( 2 3 4 5 ) list<> "'( 2 ) '( 3 4 5 ) set-cdr!" test-expr
	\ list->array
	'( list( 0 1 2 ) hash{ 'foo 10 } 2 ) to l1
	l1 list->array to l2
	l1 1 list-ref 'foo 30 hash-set!
	l1 l2 array= not "list->array list<>?" test-expr
	\ list-copy
	'( list( 0 1 2 ) hash{ 'foo 10 } 2 ) to l1
	l1 list-copy to l2
	l1 1 list-ref 'foo 30 hash-set!
	l1 l2 list= "list-copy list=?" test-expr
	\ list-ref
	'( 0 1 2 3 ) to l1
	l1 -4 list-ref 0<> "list-ref -4" test-expr
	l1 -5 <'> list-ref 'out-of-range nil fth-catch car 'out-of-range <>
	    "list-ref -5" test-expr
	\ list-set!
	'( 0 1 2 3 ) to l1
	l1 -4 4 list-set!
	l1 -4 list-ref 4 <> "list-set! -4" test-expr
	l1 -5 4 <'> list-set! 'out-of-range nil fth-catch car 'out-of-range <>
	    "list-set! -5" test-expr
	\ list-append
	0 1 '( 2 3 4 ) 5 6  5 list-append '( 0 1 2 3 4 5 6 ) list<>
	    "0 1 '( 2 3 4 ) 5 6  5 list-append" test-expr
	'( 0 ) '( 1 ) 2 list-append '( 0 1 ) list<>
	    "'( 0 ) '( 1 )  2 list-append" test-expr
	'( 0 ) '( 1 2 3 )  2 list-append '( 0 1 2 3 ) list<>
	    "'( 0 ) '( 1 2 3 )  2 list-append" test-expr
	'( 0 '( 1 ) ) '( '( 2 ) )  2 list-append '( 0 '( 1 ) '( 2 ) ) list<>
	    "'( 0 '( 1 ) ) '( '( 2 ) )  2 list-append" test-expr
	'( 0 1 ) 2 3 nil acons 2 list-append '( 0 1  2 nil cons 3 set-cdr! )
	    list<> "'( 0 1 ) 2 3 nil acons  2 list-append" test-expr
	'() 0  2 list-append '( 0 ) list<> "'() 0  2 list-append" test-expr
	\ list-reverse
	'( 0 1 2 ) list-reverse '( 2 1 0 ) list<> "list-reverse" test-expr
	\ list-insert
	'( 0 1 2 ) to l1
	l1 0 10 list-insert '( 10 0 1 2 ) list<> "0 10 list-insert" test-expr
	l1 1 '( 4 5 6 ) list-insert '( 0 4 5 6 1 2 ) list<>
	    "1 '( 4 5 6 ) list-insert" test-expr
	l1 -1 10 list-insert '( 0 1 10 2 ) list<> "-1 10 list-insert" test-expr
	\ list-delete(!)
	'( 0 0 1 2 ) 0 list-delete '( 1 2 ) list<> "list-delete" test-expr
	'( 0 1 1 2 ) to l1
	l1 1 list-delete! drop
	l1 '( 0 2 ) list<> "list-delete!" test-expr
	\ list-slice(!)
	'( 0 0 1 2 ) 0 :count 2 list-slice '( 1 2 ) list<>
	    "0 2 list-slice" test-expr
	'( 0 0 1 2 ) -1 :count 4 list-slice '( 0 0 1 ) list<>
	    "-1 4 list-slice" test-expr
	'( 0 1 1 2 ) to l1
	l1 1 :count 2 list-slice! drop
	l1 '( 0 2 ) list<> "list-slice!" test-expr
	\ list-fill
	3 make-list to l1
	l1 pi list-fill '( pi pi pi ) list<> "list-fill" test-expr
	\ list-index
	'( 0 1 2 ) 2 list-index  2 <> "2 list-index" test-expr
	'( 0 1 2 ) 3 list-index -1 <> "3 list-index" test-expr
	\ list-member?
	'( 0 1 2 ) 2 list-member? #t <> "2 list-member?" test-expr
	'( 0 1 2 ) 3 list-member? #f <> "3 list-member?" test-expr
	\ list-head
	'( 0 1 2 3 ) 0 list-head nil list<> "0 list-head" test-expr
	'( 0 1 2 3 ) 2 list-head '( 0 1 ) list<> "2 list-head" test-expr
	'( 0 1 2 3 ) 4 list-head '( 0 1 2 3 ) list<> "4 list-head" test-expr
	'( 0 1 2 3 ) 6 list-head '( 0 1 2 3 ) list<> "6 list-head" test-expr
	\ list-tail
	'( 0 1 2 3 ) -10 list-tail '( 0 1 2 3 ) list<> "-10 list-tail" test-expr
	'( 0 1 2 3 ) 0 list-tail '( 0 1 2 3 ) list<> "0 list-tail" test-expr
	'( 0 1 2 3 ) 2 list-tail '( 2 3 ) list<> "2 list-tail" test-expr
	'( 0 1 2 3 ) 3 list-tail '( 3 ) list<> "3 list-tail" test-expr
	'( 0 1 2 3 ) 4 list-tail nil list<> "4 list-tail" test-expr
	\ last-pair
	'( 0 1 2 3 ) last-pair '( 3 ) list<> "last-pair (1)" test-expr
	0 1 2 3 nil acons acons last-pair 'a( 2 3 ) list<>
	    "last-pair (2)" test-expr
	\ acons, list-assoc, list-assoc-ref, list-assoc-set!, list-assoc-remove!
	0 1 2 3 nil acons acons to l1
	'( 0 nil cons 1 set-cdr! 2 nil cons 3 set-cdr! ) to l2
	l1 l2 list<> "acons" test-expr
	l1 2 list-assoc 2 nil cons 3 set-cdr! list<> "list-assoc" test-expr
	l1 2 list-assoc-ref 3 <> "list-assoc-ref" test-expr
	l1 0 10 list-assoc-set! 0 10 2 3 nil acons acons list<>
	    "list-assoc-set! (0)" test-expr
	l1 0 list-assoc-ref 10 <> "list-assoc-set! (0 ref)" test-expr
	l1 7 20 list-assoc-set! to l1
	0 10 2 3 7 20 nil acons acons acons to l2
	l1 l2 list<> "list-assoc-set! (7)" test-expr
	l1 7 list-assoc-ref 20 <> "list-assoc-set! (7 ref)" test-expr
	l1 0 list-assoc-remove! to l1
	2 3 7 20 nil acons acons to l2
	l1 l2 list<> "list-assoc-remove!" test-expr
;

*fth-test-count* 0 [do] list-test [loop]

\ list-test.fs ends here
