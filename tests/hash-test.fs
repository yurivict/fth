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
\ @(#)hash-test.fs	1.20 1/11/15

require test-utils.fs

lambda: <{ key val -- x }> val 10 + ; value hash-map-cb

: hash-test ( -- )
	nil nil nil { h1 h2 h3 }
	\ hash?
	nil hash?     "nil hash?" test-expr
	#{} hash? not "#{} hash?" test-expr
	#() hash?     "#() hash?" test-expr
	0   hash?     "0 hash?"   test-expr
	\ make-hash, >hash, make-hash-with-len
	make-hash hash? not "make-hash" test-expr
	0 >hash   hash? not ">hash"     test-expr
	'foo 10 'bar 20 4 >hash to h1
	h1 hash{ 'foo 10 'bar 20 } hash= not ">hash h1 h2" test-expr
	3 make-hash-with-len to h2
	h2 length                 3 <>      "make-hash-with-len (3)" test-expr
	h2 #{ 0 nil 1 nil 2 nil } hash= not "make-hash-with-len (=)" test-expr
	\ hash=
	hash{ 'foo 0 'bar 1 } to h1
	hash{ 'foo 0 'bar 1 } to h2
	hash{ 'foo 0 } to h3
	h1 h1 hash= not "h1 h1 hash=" test-expr
	h1 h2 hash= not "h1 h2 hash=" test-expr
	h1 h3 hash=     "h1 h3 hash=" test-expr
	\ hash-copy
	#{ 'foo 0 'bar 1 } to h1
	h1 hash-copy to h2
	h1 h2 hash= not "hash-copy (1)" test-expr
	hash{ 'foo #( 0 1 2 ) 'bar hash{ 'foo 10 'bar 20 } 'baz 30 } to h1
	h1 hash-copy to h2
	h1 h2 hash= not "hash-copy (2)" test-expr
	h2 'bar hash-ref 'foo 30 hash-set!
	h1 h2 hash=     "hash-copy (3)" test-expr
	\ hash-ref
	hash{ 'foo 0 'bar 1 } to h1
	h1 'bar hash-ref  1 <> "hash-ref ('bar)" test-expr
	h1 'baz hash-ref #f <> "hash-ref ('baz)" test-expr
	10 'foo <'> hash-ref 'wrong-type-arg nil fth-catch
	    car 'wrong-type-arg <> "10 'foo hash-ref" test-expr
	\ hash-set!
	hash{ 'foo 0 'bar 1 } to h1
	h1 'baz 2 hash-set!
	h1 'baz hash-ref 2 <> "hash-set!" test-expr
	10 'baz 30 <'> hash-set! 'wrong-type-arg nil fth-catch
	    car 'wrong-type-arg <> "10 'baz hash-set!" test-expr
	\ hash-delete!
	h1 'baz hash-delete! #( 'baz 2 ) array= not "hash-delete! (0)" test-expr
	h1 'baz hash-delete! #f <> "hash-delete! (1)" test-expr
	\ hash-member?
	h1 'baz hash-member? #f <> "hash-member? ('baz)" test-expr
	h1 'bar hash-member? not "hash-member? ('bar)" test-expr
	\ hash-find
	h1 'baz hash-find #f <> "hash-find ('baz)" test-expr
	h1 'bar hash-find #( 'bar 1 ) array= not "hash-find ('bar)" test-expr
	\ hash->array
	h1 hash->array { ary }
	ary #( #( 'foo 0 ) #( 'bar 1 ) ) array=
	ary #( #( 'bar 1 ) #( 'foo 0 ) ) array= || not "hash->array" test-expr
	\ hash-keys|values
	h1 hash-keys to ary
	ary #( 'bar 'foo ) array=
	ary #( 'foo 'bar ) array= || not "hash-keys"   test-expr
	h1 hash-values to ary
	ary #( 1 0 ) array=
	ary #( 0 1 ) array= || not "hash-values" test-expr
	\ hash-clear
	h1 hash-clear
	h1 length 0<> "hash-clear (1)" test-expr
	h1 #{} hash= not "hash-clear (2)" test-expr
	\ hash-each|map
	#{ 'foo 0 'bar 1 } to h1
	h1 hash-map-cb hash-map to h2
	#{ 'foo 10 'bar 11 } h2 hash= not "hash-map" test-expr
	\ object-id (fixnums: x << 1 | 1)
	10 object-id 10 1 lshift 1 or <> "object-id 10" test-expr
	\ properties, property-ref|set!
	"hello" { obj }
	obj 'hey "joe" property-set!
	obj 'hey property-ref "joe" string<> "property-set!" test-expr
	obj 'joe property-ref #f <> "property-ref ('joe)" test-expr
	obj properties #{ 'hey "joe" } hash= not "properties" test-expr
	#f properties #{ "hello" #{ 'hey "joe" } } hash= not
	    "properties (all)" test-expr
	\ word-properties, word-property-ref|set!
	<'> noop 'hey "joe" word-property-set!
	<'> noop 'hey word-property-ref "joe" string<>
	    "word-property-set!" test-expr
	<'> noop 'joe word-property-ref #f <>
	    "word-property-ref ('joe)" test-expr
	<'> noop word-properties to obj
	obj #{ 'hey "joe" 'documentation "noop" } hash=
	obj #{ 'documentation "noop" 'hey "joe" } hash= || not
	    "word-properties (all)" test-expr
	\ object-properties, object-property-ref|set!
	"hello" to obj
	obj 'hey "joe" object-property-set!
	obj 'hey object-property-ref "joe" string<>
	    "object-property-set!" test-expr
	obj 'joe object-property-ref #f <>
	    "object-property-ref ('joe)" test-expr
	obj object-properties #{ 'hey "joe" } hash= not
	    "object-properties" test-expr
;

*fth-test-count* 0 [do] hash-test [loop]

\ hash-test.fs ends here
