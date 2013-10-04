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
\ @(#)array-test.fs	1.27 9/13/13

require test-utils.fs

: array<> ( x y -- f )
	array= not
;

: array-test ( -- )
	nil         { ary }
	nil nil nil { a1 a2 a3 }
	nil         { x }
	nil         { len }
	nil         { greater }
	nil         { ass }
	\ array-length
	nil array-length -1 <> "nil -1" test-expr
	#f  array-length -1 <> "#f -1"  test-expr
	#() array-length   0<> "#()"    test-expr
	#( 0 1 2 ) array-length 3 <> "#( 0 1 2 ) length 3 <>" test-expr
	\ array?
	nil array?     "nil array?" test-expr
	#() array? not "#() array?" test-expr
	0   array?     "0 array?"   test-expr
	\ make-array
	0 make-array #()              array<> "0 make-array" test-expr
	3 make-array #( nil nil nil ) array<> "3 make-array" test-expr
	3 :initial-element pi make-array #( pi pi pi ) array<>
	    "3 pi make-array" test-expr
	1000 make-array map!
		i
	end-map ( ary ) each to x
		i x <> "1000 array-each: %d %s\n" #( i x ) test-expr-format
	end-each
	'foo <'> make-array 'wrong-type-arg nil fth-catch car 'wrong-type-arg <>
	    "'foo make-array" test-expr
	\ >array
	0 1 2 3 >array #( 0 1 2 ) array<> "0 1 2 3 >array" test-expr
	\ #()
	#() 0 array-push 1 array-push 2 array-push #( 0 1 2 ) array<>
	    "#() 0 array-push 1 array-push 2 array-push" test-expr
	\ array=
	#( 0 1 2 ) to a1
	#( 0 1 2 ) to a2
	#( 0 1 3 ) to a3
	a1 a1 array<> "a1 a1 array=" test-expr
	a1 a2 array<> "a1 a2 array=" test-expr
	a1 a3 array=  "a1 a3 array=" test-expr
	\ array->array
	#( #( 0 1 2 ) hash{ 'foo 10 } 2 ) to a1
	a1 array->array to a2
	a2 1 array-ref 'foo 30 hash-set!
	a1 a2 array<> "array->array" test-expr
	\ array->list
	#( #( 0 1 2 ) hash{ 'foo 10 } 2 ) to a1
	a1 array->list to a2
	a2 1 list-ref 'foo 30 hash-set!
	a1 a2 array<> "array->list" test-expr
	\ array-copy
	#( #( 0 1 2 ) hash{ 'foo 10 } 2 ) to a1
	a1 array-copy to a2
	a2 1 array-ref 'foo 30 hash-set!
	a1 a2 array= "array-copy" test-expr
	\ array-ref
	#( 0 1 2 3 ) to a1
	a1 -4 array-ref 0<> "array-ref -4" test-expr
	a1 -5 <'> array-ref 'out-of-range nil fth-catch car 'out-of-range <>
	    "array-ref -5" test-expr
	\ array-set!
	#( 0 1 2 3 ) to a1
	a1 -4 4 array-set!
	a1 -4 array-ref 4 <> "array-set! -4" test-expr
	a1 -5 4 <'> array-set! 'out-of-range nil fth-catch car 'out-of-range <>
	    "array-set! -5" test-expr
	\ array-push
	#( 0 1 ) to ary
	ary 2 array-push #( 0 1 2 ) array<> "array-push" test-expr
	\ array-pop
	ary array-pop 2 <> "array-pop" test-expr
	#() array-pop #f <> "array-pop" test-expr
	\ array-unshift
	#( 1 2 ) to ary
	ary 0 array-unshift #( 0 1 2 ) array<> "array-unshift" test-expr
	\ array-shift
	ary array-shift 0<> "array-shift" test-expr
	#() array-shift #f <> "#() array-shift" test-expr
	\ array-append
	#( 0 ) ary array-append #( 3 4 5 ) array-append
	    #( 0 1 2 3 4 5 ) array<> "array-append (0)" test-expr
	#() #() array-append #() array<>
	    "#() #() array-append (1)" test-expr
	#( 0 1 ) #() array-append #( 0 1 ) array<> "array-append (2)" test-expr
	#() #( 0 1 ) array-append #( 0 1 ) array<> "array-append (3)" test-expr
	#() to ary
	128 to len
	len 0 do
		ary ary array-push drop
	loop
	len 0 do
		ary array-pop drop
	loop
	ary #() array<> "128 array-push/pop" test-expr
	len 0 do
		ary ary array-unshift drop
	loop
	len 0 do
		ary array-shift drop
	loop
	ary #() array<> "128 array-unshift/shift" test-expr
	#() ( ary ) 0 len 1- do
		( ary ) i array-unshift
	-1 +loop ( ary ) each to x
		i x <> "array-unshift [%d]: %s\n" #( i x ) test-expr-format
	end-each
	\ array-reverse(!)
	#( 0 1 2 ) array-reverse #( 2 1 0 ) array<> "array-reverse" test-expr
	#( 0 1 2 ) to a1
	a1 array-reverse! #( 2 1 0 ) array<> "array-reverse! (0)" test-expr
	a1 #( 2 1 0 ) array<> "array-reverse! (1)" test-expr
	\ array-insert(!)
	#( 0 1 2 ) to a1
	a1 2 10 array-insert to a1
	a1 #( 0 1 10 2 ) array<> "array-insert (0)" test-expr
	a1 1 #( 4 5 6 ) array-insert to a1
	a1 #( 0 4 5 6 1 10 2 ) array<> "array-insert (1)" test-expr
	#( 0 1 2 ) to a1
	a1 0 #( 3 4 5 ) array-insert to a1
	a1 #( 3 4 5 0 1 2 ) array<> "array-insert (2)" test-expr
	#( 0 1 2 ) to a1
	a1 -1 #( 3 4 5 ) array-insert to a1
	a1 #( 0 1 3 4 5 2 ) array<> "array-insert (3)" test-expr
	#( 0 1 2 ) to a1
	a1 2 10 array-insert! drop
	a1 #( 0 1 10 2 ) array<> "array-insert! (0)" test-expr
	a1 1 #( 4 5 6 ) array-insert! #( 0 4 5 6 1 10 2 ) array<>
	    "array-insert! (1)" test-expr
	#( 0 1 2 ) to a1
	a1 0 #( 3 4 5 ) array-insert! #( 3 4 5 0 1 2 ) array<>
	    "array-insert! (2)" test-expr
	#( 0 1 2 ) to a1
	a1 -1 #( 3 4 5 ) array-insert! #( 0 1 3 4 5 2 ) array<>
	    "array-insert! (3)" test-expr
	#() ( ary ) 0 len 1- do
		( ary ) 0 i array-insert!
	-1 +loop ( ary ) each to x
		i x <> "array-insert! [%d]: %s\n" #( i x ) test-expr-format
	end-each
	\ array-delete!
	#( 0 1 2 ) to a1
	a1 0 array-delete! 0<> "array-delete! (0)" test-expr
	#( 0 1 2 ) to a1
	a1 2 array-delete! drop
	a1 #( 0 1 ) array<> "array-delete! (1)" test-expr
	\ array-delete-key
	#( 'a 'b 10 ) to a1
	a1 'b array-delete-key 'b <> "array-delete-key 'b" test-expr
	a1 #( 'a 10 ) array<> "array-delete-key 'b <>?" test-expr
	a1 10 array-delete-key 10 <> "array-delete-key 10" test-expr
	a1 #( 'a ) array<> "array-delete-key 10 <>?" test-expr
	a1 10 array-delete-key #f <> "array-delete-key 10 (2)" test-expr
	a1 #( 'a ) array<> "array-delete-key 10 <> (2)?" test-expr
	\ array-reject(!)
	#( 0 1 2 ) to a1
	<'> > 2 make-proc to greater
	a1 greater 1 array-reject #( 0 1 ) array<> "array-reject" test-expr
	a1 greater 1 array-reject! #( 0 1 ) array<> "array-reject!" test-expr
	\ array-compact(!)
	#( 0 1 2 ) to a1
	#( nil 0 1 nil nil 2 nil nil ) to a2
	a1 array-compact #( 0 1 2 ) array<> "array-compact (1)" test-expr
	a2 array-compact #( 0 1 2 ) array<> "array-compact (2)" test-expr
	a1 array-compact! #( 0 1 2 ) array<> "array-compact! (1)" test-expr
	a2 array-compact! #( 0 1 2 ) array<> "array-compact! (2)" test-expr
	\ array-fill
	3 make-array to a1
	a1 3.2 array-fill drop
	a1 #( 3.2 3.2 3.2 ) array<> "array-fill" test-expr
	\ array-index|member?|find
	#( 0 1 2 ) 2 array-index  2 <> "array-index (0)" test-expr
	#( 0 1 2 ) 3 array-index -1 <> "array-index (1)" test-expr
	#( 0 1 2 ) 2 array-member? #t <> "array-member? (0)" test-expr
	#( 0 1 2 ) 3 array-member? #f <> "array-member? (1)" test-expr
	#( 0 1 2 ) 2 array-find  2 <> "array-find (0)" test-expr
	#( 0 1 2 ) 3 array-find #f <> "array-find (1)" test-expr
	\ array-uniq(!)
	#( 0 1 2 3 2 1 0 ) to a1
	a1 array-uniq #( 0 1 2 3 ) array<> "array-uniq (1)" test-expr
	#( 0 1 2 3 2 1 0 ) to a1
	a1 array-uniq! drop
	a1  #( 0 1 2 3 ) array<> "array-uniq! (2)" test-expr
	\ array-sort(!) 
	#( 3.2 2 7 9 ) to a1
	a1 <'> fnumb-cmp array-sort #( 2 3.2 7 9 ) array<>
	    "array-sort" test-expr
	#( 3.2 2 7 9 ) to a1
	a1 <'> fnumb-cmp array-sort! drop
	a1 #( 2 3.2 7 9 ) array<> "array-sort!" test-expr
	\ array-join
	#( 0 1 2 ) to a1
	a1 "--" array-join "0--1--2" string<> "a1 -- array-join" test-expr
	a1 nil  array-join "0 1 2" string<> "a1 nil array-join" test-expr
	a1 #f   array-join "0 1 2" string<> "a1 #fa1 #f  array-join" test-expr
	\ array-subarray
	#( 0 1 2 3 4 )  2   4 array-subarray #( 2 3 ) array<>
	    "array-subarray (1)" test-expr
	#( 0 1 2 3 4 ) -3  -1 array-subarray #( 2 3 4 ) array<>
	    "array-subarray (2)" test-expr
	#( 0 1 2 3 4 ) -3 nil array-subarray #( 2 3 4 ) array<>
	    "array-subarray (3)" test-expr
	\ array-clear
	#( 0 1 2 ) to a1
	a1 array-clear
	a1 #( #f #f #f ) array<> "array-clear" test-expr
	\ assoc?
	nil        assoc?     "assoc? nil" test-expr
	#()        assoc?     "assoc? #()" test-expr
	#a( 'a 0 ) assoc? not "assoc? #a( 'a 0 )" test-expr
	#( 'a 0 )  assoc?     "assoc? #( 'a 0 )" test-expr
	\ >assoc
	'foo 0 'bar 1 4 >assoc to ass
	ass assoc? not "assoc? >assoc" test-expr
	ass #a( 'foo 0 'bar 1 ) array<> ">assoc: %s" #( a1 ) test-expr-format
	\ array-assoc
	#() 'a 10 assoc to ass
	ass #a( 'a 10 ) array<> "assoc (1)" test-expr
	ass 'b 20 assoc to ass
	ass #a( 'a 10 'b 20 ) array<> "assoc (2)" test-expr
	\ array-assoc
	ass 'a array-assoc #( 'a 10 ) array<> "array-assoc 'a" test-expr
	ass 10 array-assoc "array-assoc 10" test-expr
	#() 'a 100 assoc 'b 200 assoc to ass
	ass 'b array-assoc #( 'b 200 ) array<> "array-assoc 'b" test-expr
	#() 'a #( 0 1 ) assoc to ass
	\ array-assoc-ref
	ass 'a array-assoc-ref #( 0 1 ) array<>
	    "array-assoc-ref 'a -> #( 0 1 )" test-expr
	ass 0 array-assoc-ref "array-assoc-ref 0 -> #f" test-expr
	ass 1 array-assoc-ref "array-assoc-ref 1 -> #f" test-expr
	\ array-assoc-set!
	#() 'a #( 0 1 ) assoc to ass
	ass 'a 10 array-assoc-set! #( #( 'a 10 ) ) array<>
	    "array-assoc-set! 'a 10" test-expr
	ass 0 10 array-assoc-set! #( #( 0 10 ) #( 'a 10 ) ) array<>
	    "array-assoc-set! 0 10" test-expr
	ass 1 10 array-assoc-set! #( #( 0 10 ) #( 1 10 ) #( 'a 10 ) ) array<>
	    "array-assoc-set! 1 10" test-expr
	\ array-assoc-remove!
	#() 'a #( 0 1 ) assoc 'd 10 assoc to ass
	ass 'a array-assoc-remove! #a( 'd 10 ) array<>
	    "array-assoc-remove! 'a" test-expr
	ass 0 array-assoc-remove! #a( 'd 10 ) array<>
	    "array-assoc-remove! 0" test-expr
	ass 1 array-assoc-remove! #a( 'd 10 ) array<>
	    "array-assoc-remove! 1" test-expr
;

*fth-test-count* 0 [do] array-test [loop]

\ array-test.fs ends here
