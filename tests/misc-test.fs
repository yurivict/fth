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
\ @(#)misc-test.fs	1.24 1/11/15

require test-utils.fs

: fth-test-proc { n -- }
	n 0= if
		'math-error '( "%s: %s" get-func-name n ) fth-throw
	then
	n 1 = if
		'math-error "(fth-raise) %s: %s" '( get-func-name n ) fth-raise
	then
;

: misc-test ( -- )
	\ add-load-path
	*load-path* "/tmp" array-member?
	    "*load-path* has \"/tmp\"" test-expr
	"/tmp" add-load-path
	*load-path* "/tmp" array-member? not
	    "add-load-path: /tmp not set" test-expr
	*load-path* array-pop drop
	\ fth-catch|throw|raise
	\ no throw/raise
	10 <'> fth-test-proc 'math-error nil fth-catch
	    "fth-catch (1)" test-expr
	10 <'> fth-test-proc 'math-error 'yes fth-catch
	    "fth-catch (2)" test-expr
	10 <'> fth-test-proc #t nil fth-catch
	    "fth-catch (3)" test-expr
	10 <'> fth-test-proc #t 'yes fth-catch
	    "fth-catch (4)" test-expr
	\ throw
	0 <'> fth-test-proc 'math-error 'yes fth-catch 'yes <>
	    "fth-catch-fth-throw (1)" test-expr
	0 <'> fth-test-proc 'math-error nil fth-catch 0 array-ref 'math-error <>
	    "fth-catch-fth-throw (2)" test-expr
	0 <'> fth-test-proc #t 'yes fth-catch 'yes <>
	    "fth-catch-fth-throw (3)" test-expr
	0 <'> fth-test-proc #t nil fth-catch 0 array-ref 'math-error <>
	    "fth-catch-fth-throw (4)" test-expr
	\ raise
	1 <'> fth-test-proc 'math-error 'yes fth-catch 'yes <>
	    "fth-catch-fth-raise (1)" test-expr
	1 <'> fth-test-proc 'math-error nil fth-catch 0 array-ref 'math-error <>
	    "fth-catch-fth-raise (2)" test-expr
	1 <'> fth-test-proc #t 'yes fth-catch 'yes <>
	    "fth-catch-fth-raise (3)" test-expr
	1 <'> fth-test-proc #t nil fth-catch 0 array-ref 'math-error <>
	    "fth-catch-fth-raise (4)" test-expr
	#( 0 1 2 ) { ary }
	512 0 do
		ary i 4 + <'> array-ref #t nil fth-catch
		stack-reset
	loop
	\ each|map(!)
	#( 0 1 2 3 ) each { val }
		val i <> if
			"each (array): %d %s?" #( i val ) fth-display
		then
	end-each
	hash{ 'foo 10 'bar 20 } each { val }
		val 0 array-ref { k }
		val 1 array-ref { v }
		k 'foo = if
			v 10 <> if
				"each (hash): %s %s?" #( k v ) fth-display
			then
		else
			k 'bar = if
				v 20 <> if
					"each (hash): %s %s?"
					    #( k v ) fth-display
				then
			else
				"each (hash): no key 'foo or 'bar?"
				    #() fth-display
			then
		then
	end-each
	pi object->array each ( val )
		pi f<> "each (pi) pi val f<>" test-expr
		i 0<> "each (pi) i 0<>" test-expr
	end-each
	array( 0 1 ) { ary }
	hash{ 'foo 10 'bar 20 } { hs }
	list( 0 1 ) { ls }
	ary each { aval }
		hs each { hval }
			ls each { lval }
			end-each
		end-each
	end-each
	3 make-array map
		i f2*
	end-map #( 0.0 2.0 4.0 ) array= not "map (array) i f2*" test-expr
;

*fth-test-count* 0 [do] misc-test [loop]

\ misc-test.fs ends here
