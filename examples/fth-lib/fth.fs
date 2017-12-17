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
\ @(#)fth.fs	1.67 12/16/17

\ alias			( xt "name" -- ; self -- ?? )
\ shell-alias		( cmd-str "cmd" -- ; self -- f )
\ unless		( f -- )
\ [unless]		( f -- )
\ ?leave		( f -- )
\ [defined]		( "name" -- f )
\ [undefined]		( "name" -- f )
\ [ifdef]		( "name" -- )
\ [ifundef]		( "name" -- )
\
\ flogn			( r1 b -- r2 )
\ fround->s		( r -- d )
\ floor->s		( r -- d )
\ fceil->s		( r -- d )
\
\ let: ... ;let		( -- )
\
\ f2c			( fahrenheit -- celsius )
\ c2f			( celsius -- fahrenheit )
\ .f2c			( fahrenheit -- )
\ .c2f			( celsius -- )
\ 
\ each ... end-each	( obj -- )
\ map  ... end-map	( obj1 -- obj2 )
\ map! ... end-map	( obj -- obj' )
\ 
\ [do] ... [loop]	( limit start -- )
\ [i] [j] [k]		( -- n )
\ [leave]		( -- )
\ [each] ... [end-each]	( obj -- val )
\ [map]	 ... [end-map]	( obj -- )
\ [map!] ... [end-map]	( obj -- )
\
\ save-stack		( ?? -- ary )
\ restore-stack		( ary -- ?? )
\ .stack		( -- )
\ 
\ getopts		( long-arg short-arg arg-type defval -- val )
\ 
\ set-local-variables	( -- )
\ get-local-variables	( xt -- vars )
\ display-variables	( xt -- )
\
\ string->number	( str -- obj )
\ number->string	( obj -- str )
\ 
\ ?dup-unless		( obj -- obj| )
\ assert-type		( condition obj pos msg -- )
\ stack-check		( req -- )
\ 
\ make-?obj		( class-var "name" --; obj self -- f )
\ create-struct		( names "name" -- ; self -- obj )
\ create-instance-struct ( names "name" -- ; class-var self -- obj )
\ struct?               ( obj -- f )
\ secs->hh:mm:ss	( secs -- str )
\
\ timer?		( obj -- f )
\ make-timer		( -- tm )
\ start-timer		( tm -- )
\ stop-timer		( tm -- )
\ real-time@		( tm -- rtime )
\ user-time@		( tm -- utime )
\ system-time@		( tm -- stime )
\ with-timer		( xt -- tm )
\
\ object-sort<		( obj -- ary )
\ object-sort>		( obj -- ary )

\ missing words
\ 1 10 bounds ( 11 1 ) do ... loop
: bounds ( n1 n2 -- n1+n2 n1 )   over + swap ;
: bs ( -- )   8 emit ;
: on ( w -- )   true swap ! ;
: off ( w -- )   false swap ! ;
: rdrop  ( R:w -- )   r> drop ;
: 2rdrop ( R:w R:w -- )   2r> 2drop ;

: alias ( xt "name" --; self -- )
	create ,
  does> ( self -- )
	@ execute
;

: shell-alias ( cmd-str "cmd" -- ; self -- f )
	doc" alias for shell command\n\
\"ls -AFlo\" shell-alias l\n\
\"vi\"       shell-alias vi\n\
\\ use them to execute these shell commands:\n\
l                => ls -AFlo\n\
l /usr/local/bin => ls -loAFG /usr/local/bin\n\
vi ~/.fthrc      => vi ~/.fthrc\n\
Create word CMD which feeds CMD-STR to file-system.  \
Appends rest of line as arguments to CMD-STR if any.  \
Returns #t for success and #f otherwise.\n\
See also file-system."
	create ( cmd-str ) $space $+ ,
  does> ( self -- f )
	@ parse-word $>string $+ file-system
;

<'> @ alias f@
<'> ! alias f!
<'> @ alias df@
<'> ! alias df!
<'> @ alias sf@
<'> ! alias sf!
<'> current-time alias time-count
<'> get-optkey alias get-args

: unless ( f -- )   postpone not postpone if ; immediate
: [unless] ( f -- )   not postpone [if] ; immediate
: ?leave ( f -- )   postpone if postpone leave postpone then ; immediate

: [defined] ( "name" -- f )
	doc" interpret state test
[defined] clm-print [if]\n\
	\"hello, world\\n\" clm-print\n\
[else]\n\
	\"hello, world\\n\" fth-print\n\
[then]\n\
For use in interpret state (scripts or repl), not in word definitions.  \
Return #t if NAME does exist in dictionary, otherwise #f.\n\
See also [ifdef]."
	bl word find nip 0<>
; immediate

: [undefined] ( "name" -- f )
	doc" interpret state test
[undefined] clm-print [if]\n\
	\"hello, world\\n\" fth-print\n\
[else]\n\
	\"hello, world\\n\" clm-print\n\
[then]\n\
For use in interpret state (scripts or repl), not in word definitions.  \
Return #t if NAME doesn't exist in dictionary, otherwise #f.\n\
See also [ifundef]."
	bl word find nip 0=
; immediate

: [ifdef] ( "name" -- )
	doc" interpret state test
[ifdef] clm-print\n\
	\"hello, world\\n\" clm-print\n\
[else]\n\
	\"hello, world\\n\" fth-print\n\
[then]\n\
For use in interpret state (scripts or repl), not in word definitions.  \
Start bracket-if-branch if NAME does exist in dictionary, \
bracket-else-branch otherwise.\n\
See also [defined]."
	postpone [defined] postpone [if]
; immediate

: [ifundef] ( "name" -- )
	doc" interpret state test
[ifundef] clm-print\n\
	\"hello, world\\n\" fth-print\n\
[else]\n\
	\"hello, world\\n\" clm-print\n\
[then]\n\
For use in interpret state (scripts or repl), not in word definitions.  \
Start bracket-if-branch if NAME doesn't exist in dictionary, \
bracket-else-branch otherwise.\n\
See also [undefined]."
	postpone [undefined] postpone [if]
; immediate

: float+ ( n1 -- n2 )   1 floats + ;

: faligned ( addr -- addr' )
	[ 1 floats 1- ] literal + [ -1 floats ] literal and
;

: sfloat+ ( n1 -- n2 )   1 sfloats + ;

: sfaligned ( addr -- addr' )
	[ 1 sfloats 1- ] literal + [ -1 sfloats ] literal and
;

: dfloat+ ( n1 -- n2 )   1 dfloats + ;

: dfaligned ( addr -- addr' )
	[ 1 dfloats 1- ] literal + [ -1 dfloats ] literal and
;

: nalign ( addr1 n -- addr2 )   1- tuck +  swap invert and ;

: dofield ( -- )
  does> ( self -- addr )
	@ +
;

: dozerofield ( -- )
	immediate
  does> ( self -- )
	drop
;

: create-field ( align1 offset1 align size "name" --  align2 offset2 )
	create swap rot over nalign dup , ( align1 size align offset )
	rot + >r nalign r>
;

: field ( align1 offset1 align size "name" --  align2 offset2 )
	2 pick >r
	create-field
	r> if
		dofield
	else
		dozerofield
	then
;

: 2@ { addr -- n1 n2 }   addr @ addr cell+ @ ;
: 2! { n1 n2 addr -- }   n1 addr ! n2 addr cell+ ! ;
: 2, ( d -- )   here 2 cells allot 2! ;

: 2constant ( d "name" --; -- d )
	create 2,
  does> ( -- d )
	2@
;

: end-struct ( align size "name" -- ) over nalign 2constant ;

\ an empty struct
1 chars 0 end-struct struct

\ type descriptors, all ( -- align size )
1 aligned   1 cells   2constant cell%
1 chars     1 chars   2constant char%
1 faligned  1 floats  2constant float%
1 dfaligned 1 dfloats 2constant dfloat%
1 sfaligned 1 sfloats 2constant sfloat%
cell% 2*              2constant double%

\ memory allocation words
: %alignment ( align size -- align )   drop ;
: %size ( align size -- size )   nip ;
: %align ( align size -- )   drop here swap nalign here - allot ;
: %allot ( align size -- addr )   tuck %align here swap allot ;
: %allocate ( align size -- addr ior )   nip allocate ;
: %alloc ( size align -- addr )   %allocate throw ;

start-prefixes
: '   postpone symbol-intern  ; immediate
: :   postpone keyword-intern ; immediate
: "   postpone $" ; immediate
: /   postpone re/ ; immediate
end-prefixes

: flogn ( r1 b -- r2 )   ( b ) flog swap ( r1 ) flog swap f/ ;
: fround->s ( r -- n )   fround f>s ;
: floor->s ( r -- n )   floor  f>s ;
: fceil->s ( r -- n )   fceil  f>s ;

[ifundef] ceil
	<'> fceil alias ceil
	<'> ftrunc alias trunc
	<'> f** alias **
	<'> fsqrt alias sqrt
[then]

[ifundef] inexact->exact
	<'> f>s alias inexact->exact
	<'> s>f alias exact->inexact
[then]

<'> lambda: alias let: ( -- )
<'> let: "( -- )  start compile state up to ;let\n\
let: 10 { var } var .  ;let => 10\n\
Takes code up to ;let and executes it immediately after reading ;let.  \
It acts like:\n\
lambda: ... ; execute\n\
See also ;let." help-set!

: ;let ( -- )
	doc" execute code from let: up to here\n\
let: 10 { var } var .  ;let => 10\n\
Ends a previous let: and executes code immedately after reading ;let.\n\
See also let:."
	postpone ; execute
; immediate

hide
5.0 9.0 f/ constant five/ninth

\ Takes precision N and an XT which should return a float and returns a
\ new word NAME.  The new word prints float with precision N.
: print-float-with-precision ( n xt "name" -- ; val self -- )
	create ( xt ) , ( n ) ,
  does> { val self -- }
	self       @ { xt }
	self cell+ @ { prec }
	val xt execute { res }
	precision { tmp }
	prec set-precision
	res f.
	tmp set-precision
;
set-current

: f2c ( fahrenheit -- celsius )   32.0 f-  five/ninth  f* ;
: c2f ( celsius -- fahrenheit )   five/ninth  f/  32.0 f+ ;

2 <'> f2c print-float-with-precision .f2c ( fahrenheit -- )
2 <'> c2f print-float-with-precision .c2f ( celsius -- )
previous

\
\ Compile state loops.
\
\ : pseudo-each { obj -- val }   obj length 0 ?do obj i object-ref ;
: each ( obj -- val )
	doc" compile state each-loop\n\
#( 0 1 2 ) each  .  end-each => 0 1 2\n\
Push each element of OBJ in order on stack and repeat execution of body.  \
It can only be used in word definitions, \
for an interpret state version see [each]/[end-each].\n\
See also end-each, map, map! and end-map."
	postpone (set-each-loop) 0 postpone literal postpone ?do
	postpone r@ postpone (fetch)
; immediate compile-only

\ : pseudo-end-each ( -- )   loop ;
: end-each ( -- )
	doc" compile state end of each-loop\n\
#( 0 1 2 ) each  .  end-each => 0 1 2\n\
See also each, map, map! and end-map."
	postpone loop postpone (reset-each)
; immediate compile-only

#f value *key*
<'> *key* "global variable\n\
Hold current element in a map/end-map loop.\n\
See also map, map! and end-map." help-set!

hide
: (set-key-var) ( val -- )   to *key* ;
set-current

\ : pseudo-map { obj -- }   obj length 0 ?do obj i object-ref to *key* ;
: map ( obj -- )
	doc" compile state map-loop\n\
#( 0 1 2 ) map  i *key* +  end-map => #( 0 2 4 )\n\
Set each element of OBJ in order to global variable *key* \
and repeat execution of body.  \
The current element of a copy of OBJ is set to top of stack before end-map.  \
It can only be used in word definitions, \
for an interpret state version see [map]/[end-map].\n\
See also end-map, map!, each and end-each."
	postpone (set-map-loop) 0 postpone literal postpone ?do
	postpone r@ ( i ) postpone (fetch) postpone (set-key-var)
; immediate compile-only

: map! ( obj -- )
	doc" compile state map!-loop\n\
#( 0 1 2 ) value a1\n\
a1 map!  i *key* +  end-map => #( 0 2 4 )\n\
a1 .$ => #( 0 2 4 )\n\
Set each element of OBJ in order to global variable *key* \
and repeat execution of body.  \
The current element of OBJ is set to top of stack before end-map.  \
It can only be used in word definitions, \
for an interpret state version see [map]/[end-map].\n\
See also end-map, map, each and end-each."
	postpone (set-map!-loop) 0 postpone literal postpone ?do
	postpone r@ ( i ) postpone (fetch) postpone (set-key-var)
; immediate compile-only
previous

\ OBJ: object from (set-map-loop) above
\ VAL: top of stack before end-map
\ : pseudo-end-map ( -- obj )   obj i val object-set! loop obj ;
: end-map ( -- obj )
	doc" compile state end of map-loop\n\
#( 0 1 2 ) map  i *key* +  end-map => #( 0 2 4 )\n\
Return changed copy (map) or changed original (map!) object OBJ.\n\
See also map, map!, each and end-each."
	postpone r@ ( i ) postpone (store) postpone loop postpone (reset-map)
; immediate compile-only

\
\ Interpret state loops.
\
: [do] ( limit start -- )
	doc" interpret state loop\n\
3 0 [do]  i .  [loop] => 0 1 2\n\
Loop for interpret state (scripts or repl).  \
The body of [do]-[loop] is in compile state, \
loop indexes I, J, K and LEAVE work as expected.  \
For a compile state version see do/loop.\n\
See also [each], [map] and [map!]."
	let: postpone do
; immediate

: [loop] ( -- )
	doc" interpret state end of loop\n\
3 0 [do]  i .  [loop] => 0 1 2\n\
See also [do], [end-each] and [end-map]."
	postpone loop postpone ;let
; immediate

: [i] ( -- n )   postpone i ; immediate
: [j] ( -- n )   postpone j ; immediate
: [k] ( -- n )   postpone k ; immediate
: [leave] ( -- )   postpone leave ; immediate

: [each] ( obj -- val )
	doc" interpret state each-loop\n\
#( 0 1 2 ) [each]  .  [end-each] => 0 1 2\n\
Loop for interpret state (scripts or repl).  \
The body of [each]-[end-each] is in compile state, \
loop indexes I, J, K and LEAVE work as expected.  \
Pushs each element of OBJ in order on stack and repeat execution of body.  \
For a compile state version see each/end-each.\n\
See also [end-each], [map], [map!] and [end-map]."
	let: postpone each
; immediate

: [end-each] ( -- )
	doc" interpret state end of each-loop\n\
#( 0 1 2 ) [each]  .  [end-each] => 0 1 2\n\
See also [each], [map], [map!] and [end-map]."
	postpone end-each postpone ;let
; immediate

: [map] ( obj -- )
	doc" interpret state map-loop\n\
#( 0 1 2 ) [map]  i *key* +  [end-map] => #( 0 2 4 )\n\
Loop for interpret state (scripts or repl).  \
The body of [map]-[end-map] is in compile state, \
loop indexes I, J, K and LEAVE work as expected.  \
Set each element of OBJ in order to global variable *key* \
and repeat execution of body.  \
The current element of a copy of OBJ is set to top of stack before [end-map].  \
For a compile state version see map/end-map.\n\
See also [end-map], [map!], [each] and [end-each]."
	let: postpone map
; immediate

: [map!] ( obj -- )
	doc" interpret state map-loop\n\
#( 0 1 2 ) value a1\n\
a1 [map!]  i *key* +  [end-map] => #( 0 2 4 )\n\
a1 .$ => #( 0 2 4 )\n\
Loop for interpret state (scripts or repl).  \
The body of [map!]-[end-map] is in compile state, \
loop indexes I, J, K and LEAVE work as expected.  \
Set each element of OBJ in order to global variable *key* \
and repeat execution of body.  \
The current element of OBJ is set to top of stack before [end-map].  \
Top of stack before [end-map] is set to new current element of OBJ.  \
For a compile state version see map!/end-map.\n\
See also [end-map], [map], [each] and [end-each]."
	let: postpone map!
; immediate

: [end-map] ( -- obj )
	doc" interpret state end of map-loop\n\
#( 0 1 2 ) [map]  i *key* +  [end-map] => #( 0 2 4 )\n\
Return changed copy ([map]) or changed original ([map!]) object OBJ.\n\
See also [map], [map!], [each] and [end-each]."
	postpone end-map postpone ;let
; immediate

: save-stack ( ?? -- ary )   depth ?dup-if >array else #f then ;
: restore-stack ( ary -- ?? )   ?dup-if each ( val ) end-each then ;

hide
: (show-stack) ( ?? filename lineno -- ?? )
	{ filename lineno }
	save-stack { s }
	"\\ #<stack depth %d at %s:%d>\n"
	    #( s length filename lineno ) fth-print
	s array-reverse each { obj }
		"\\ #<stack [%d]: %p>\n" #( i obj ) fth-print
	end-each
	s restore-stack
;

: (.stack) ( filename lineno -- )
	{ filename lineno }
	depth 0> if
		." \" cr
		filename lineno (show-stack)
		." \" cr
	then
;
set-current

: .stack ( -- )
	postpone *filename* postpone *lineno* postpone (.stack)
; immediate
previous

\ === GETOPTS ===
0 constant no-argument
1 constant required-argument
2 constant optional-argument

: getopts ( long-arg short-arg arg-type defval -- val )
	doc" Scan *ARGV* for options LONG-ARG and SHORT-ARG.  \
If found, deletes it from *ARGV* and returns a 
value corresponding to constant ARG-TYPE.  \
LONG-ARG must be a string, SHORT-ARG a character, \
ARG-TYPE one of NO-ARGUMENT, REQUIRED-ARGUMENT, \
OPTIONAL-ARGUMENT, and DEFVAL a default value.  \
The returned value may be a string or the DEFVAL.  \
If a number is needed, use string->number to get a correct number.  \
If string->number gets a string, it evals it and returns the result, \
otherwise returns the unevaled value.\n\
\"play\"     char p required-argument #f    getopts value play-file\n\
\"record\"   char r required-argument #f    getopts value record-file\n\
\"duration\" char d required-argument 10.0  getopts string->number value dur\n\
\"channels\" char c required-argument 1     getopts string->number value chns\n\
\"srate\"    char s required-argument 44100 getopts string->number value srt\n\
\"quiet\"    char q no-argument       #f    getopts not to *clm-verbose*"
	{ l-arg s-arg arg-type defval }
	"--%s" #( l-arg ) string-format { long-arg }
	"%c"   #( s-arg ) string-format { short-arg }
	\ first: --long-arg
	*argv* long-arg array-index { idx }
	idx 0>= if
		*argv* idx array-delete! drop
	else
		\ second: -c
		*argv* "-" short-arg << array-index to idx
		idx 0>= if
			*argv* idx array-delete! drop
		then
	then
	idx 0>= if
		arg-type case
		no-argument of
			#t
		endof
		required-argument of
			*argv* length 0> if
				/^-+/ *argv* idx array-ref regexp-match unless
					*argv* idx array-delete! ( val )
				else
					'forth-error
					    #( "%s: %s needs an argument"
					       get-func-name
					       long-arg )
					    fth-throw
				then
			else
				'forth-error
				    #( "%s: %s needs an argument"
				       get-func-name
				       long-arg )
				    fth-throw
			then
		endof
		optional-argument of
			/^-+/ *argv* idx array-ref regexp-match if
				defval
			else
				*argv* idx array-delete! ( val )
			then
		endof
		endcase
	else
		defval
		*argv* each { arg }
			/^-[[:alpha:]]+/ arg regexp-match if
				arg short-arg string-index to idx
				idx 0> if
					drop ( defval )
					arg idx string-delete! drop
					#t
					leave
				then
			then
	 	end-each
	then
	*argv* length to *argc*
;

hide
: (set-local-variables) ( xt vars -- )
	'debug-property swap word-property-set!
;
set-current

: set-local-variables ( -- )
	doc" set word-property\n\
If this function appears in a colon definition, \
it sets LOCAL-VARIABLES and stores the result \
in an associative array for later use.  \
GET-LOCAL-VARIABLES retrieves the content and \
DISPLAY-VARIABLES prints it.  \
This word is immediate and compile only, \
it can only be used in colon definitions.\n\
See also local-variables, display-variables, and get-local-variables."
	postpone running-word
	postpone local-variables
	postpone (set-local-variables)
; immediate compile-only
previous

: get-local-variables ( xt -- vars )
	 doc" get word-property\n\
If XT was prepared with SET-LOCAL-VARIABLES \
and was called before, one can retrieve the content of the property list, \
a lists of variable names and their current values, with this word.\n\
See also set-local-variables, display-variables, and local-variables."
	:debug-property swap word-property-ref
;

: display-variables ( xt -- )
	doc" display word-property\n\
: word-with-locals { foo -- }\n\
	10 { bar }\n\
	set-local-variables\n\
;\n\
20 word-with-locals\n\
<'> word-with-locals display-variables\n\
\\\n\
\\ local variables for: word-with-locals\n\
\\ bar = 10\n\
\\ foo = 20\n\
If XT was prepared with SET-LOCAL-VARIABLES \
and was called before, one can print the content of the property list, \
a lists of variable names and their current values, with this word.\n\
See also set-local-variables, get-local-variables, and local-variables."
	{ xt }
	"\\\n" '() fth-print
	"\\ local variables for: %s\n" '( xt xt->name ) fth-print
	"\\\n" '() fth-print
	xt 'debug-property word-property-ref each { vals }
		"\\ %s = %s\n" vals fth-print
	end-each
;

\ === String ===
<'> string-eval alias string->number ( str -- obj )
<'> object->string alias number->string ( obj -- str )
<'> string->number <'> string-eval    help-ref help-set!
<'> number->string <'> object->string help-ref help-set!

hide
: dup-if-not ( obj -- obj #t | #f )
	{ obj }
	obj 0=
	obj false? ||
	obj nil?   || if
		obj #t
	else
		#f
	then
;
set-current

: ?dup-unless ( obj -- )
	postpone dup-if-not postpone if
; immediate
previous

hide
: (assert-type) ( condition obj pos msg xt -- )
	{ condition obj pos msg xt }
	condition unless
		'wrong-type-arg
		    #( "%s: wrong type arg %s, %s (%s), wanted %s"
		       xt xt->name
		       pos
		       obj object-name
		       obj
		       msg ) fth-throw
	then
;

: (stack-check) ( req xt -- )
	{ req xt }
	depth req < if
		depth { d }
		'wrong-number-of-args
		    #( "%s: not enough arguments, %s instead of %s"
		       xt xt->name
		       d
		       req ) fth-throw
	then
;
set-current

: assert-type ( condition obj pos msg -- )
	doc" assertion\n\
: new-word { obj -- f }\n\
	obj list?  obj  1  \"a list\"  assert-type\n\
	...\n\
;\n\
Throw WRONG-TYPE-ARG exception if CONDITION evals to #f.  \
OBJ is the object in question, \
POS the argument position in the function call, \
MSG is a description what type was required.  \
This word is immediate and compile only, \
it can only be used in colon definitions.\n\
See also fth-throw, fth-raise, and fth-catch."
	postpone running-word postpone (assert-type)
; immediate compile-only

: stack-check ( req -- )
	doc" assertion\n\
: new-word ( obj -- f )\n\
	1 stack-check\n\
	{ obj }\n\
	...\n\
;\n\
Throw WRONG-NUMBER-OF-ARGS exception if stack-depth is less than REQ.\n\
See also fth-throw, fth-raise, and fth-catch."
	postpone running-word postpone (stack-check)
; immediate compile-only
previous

\ fth-timer make-?obj timer?
: make-?obj  ( class-var "name" --; obj self -- f )
	create ,
  does> { obj self }
	obj instance? if
		obj instance-obj-ref self @ =
	else
		#f
	then
;

hide
: create-setter ( slot idx --; gen val self -- )
	{ slot idx }
	slot "!" $+ word-create
	idx ,
  does> { gen val self -- }
	gen instance-gen-ref @ ( ary ) self @ ( idx ) val array-set!
;

: create-getter ( slot idx --; gen self -- val )
	{ slot idx }
	slot "@" $+ word-create
	idx ,
  does> { gen self -- val }
	gen instance-gen-ref @ ( ary ) self @ ( idx ) array-ref
;

: build-getter-and-setter { names -- len }
	names each { name }
		name i create-getter
		name i create-setter
	end-each
	names length
;
set-current

"struct" make-object-type constant fth-struct
fth-struct make-?obj struct?

: create-struct ( names "name" --; self -- obj )
	doc" create a struct\n\
In two steps one can create getter, setter, and actual objects.  \
In the first step one creates the getter and setter:\n\
#( \"timer-base-real\" ... ) create-struct make-timer-struct\n\
Now these words exist:\n\
timer-base-real@ ( tm -- val ) getter\n\
timer-base-real! ( tm val -- ) setter\n\
etc. and\n\
make-timer-struct ( -- tm )\n\
In the second step actual objects can be created with MAKE-TIMER-STRUCT:\n\
: make-timer ( -- tm )\n\
  make-timer-struct { tm }\n\
  tm 0.0 timer-base-real!\n\
  [...]\n\
  tm
;\n\
See also create-instance-struct."
	( names ) build-getter-and-setter ( len )
	create ( len ) ,
  does> { self -- obj }
	1 cells allocate throw { gen }
	self @ ( length ) :initial-element 0 make-array ( ary ) gen !
	gen fth-struct make-instance
;

: create-instance-struct ( names "name" --; class-var self -- obj )
	doc" create an instance struct\n\
In two steps one can create getter, setter, and actual objects.  \
In the first step one creates the getter and setter:\n\
#( \"timer-base-real\" ... ) create-instance-struct make-timer-instance\n\
Now these words exist:\n\
timer-base-real@ ( tm -- val ) getter\n\
timer-base-real! ( tm val -- ) setter\n\
etc. and\n\
make-timer-instance ( class-var -- obj )\n\ 
In the second step actual objects can be created with MAKE-TIMER-INSTANCE:\n\
: make-timer ( -- tm )\n\
  fth-timer make-timer-instance { tm }\n\
  tm 0.0 timer-base-real!\n\
  [...]\n\
  tm
;\n\
See also create-struct."
	( names ) build-getter-and-setter ( len )
	create ( len ) ,
  does> { class-var self -- obj }
	1 cells allocate throw { gen }
	self @ ( length ) :initial-element 0 make-array ( ary ) gen !
	gen class-var make-instance
;
previous

: secs->hh:mm:ss ( secs -- str )
	doc" Take SECS, a number (presumable representing seconds), \
and return a string like \"1:23:45.678 (5025.678)\"."
	exact->inexact { secs }
	secs floor->s 3600 /mod { hh } 60 /mod { ss mm }
	secs secs floor f- 1000.0 f* f>s { ms }
	hh 0> if
		"%d:%02d:%02d.%03d (%.3f)" #( hh mm ss ms secs )
	else
		mm 0> if
			"%d:%02d.%03d (%.3f)" #( mm ss ms secs )
		else
			"%.3f" #( secs )
		then
	then string-format
;

\ === TIMER OBJECT TYPE ===
"timer" make-object-type constant fth-timer
fth-timer make-?obj timer?

#( "timer-base-real"
   "timer-base-user"
   "timer-base-system"
   "timer-result-real"
   "timer-result-user"
   "timer-result-system" ) create-instance-struct make-timer-instance

\ TIMER-INSPECT
lambda: <{ tm -- str }>
	"#<%s real: %s, user: %s, system: %s>"
	    #( tm object-name
	       tm timer-result-real@ secs->hh:mm:ss
	       tm timer-result-user@ secs->hh:mm:ss
	       tm timer-result-system@ secs->hh:mm:ss ) string-format
; fth-timer set-object-inspect

: start-timer ( tm -- )
	{ tm }
	tm timer? tm 1 "a timer" assert-type
	tm time          ( rt ) timer-base-real!
	tm utime >r ( r:st ut ) timer-base-user!
	tm r>            ( st ) timer-base-system!
;

: make-timer ( -- tm )
	fth-timer make-timer-instance { tm }
	tm 0.0 timer-result-real!
	tm 0.0 timer-result-user!
	tm 0.0 timer-result-system!
	tm start-timer
	tm
;

: stop-timer ( tm -- )
	{ tm }
	tm timer? tm 1 "a timer" assert-type
	tm  time          ( rt ) tm timer-base-real@   f-  timer-result-real!
	tm  utime >r ( r:st ut ) tm timer-base-user@   f-  timer-result-user!
	tm  r>            ( st ) tm timer-base-system@ f-  timer-result-system!
;

\ Next three words are required by .timer and .timer-ratio in clm.fs.
: real-time@ ( tm -- )
	{ tm }
	tm timer? tm 1 "a timer" assert-type
	tm timer-result-real@
;

: user-time@ ( tm -- )
	{ tm }
	tm timer? tm 1 "a timer" assert-type
	tm timer-result-user@
;

: system-time@ ( tm -- )
	{ tm }
	tm timer? tm 1 "a timer" assert-type
	tm timer-result-system@
;

: with-timer ( xt -- tm )
	doc" timing\n\
<'> words with-timer\n\
Return timer object with execution time of XT."
	{ xt }
	xt xt? xt proc? || xt 1 "a proc or xt" assert-type
	make-timer { tm }
	xt proc? if
		xt proc->xt
	else
		xt
	then execute
	tm stop-timer
	tm
;

hide
: int-cmp { n1 n2 -- n3 }
	n1 n2 < if
		-1
	else
		n1 n2 > if
			1
		else
			0
		then
	then
;

: float-cmp { r1 r2 -- n }
	r1 r2 f< if
		-1
	else
		r1 r2 f> if
			1
		else
			0
		then
	then
;

: obj-sort< { val1 val2 -- n }
	val1 string? if
		val1 val2 string-cmp
	else
		val1 float? if
			val1 val2 float-cmp
		else
			val1 val2 int-cmp
		then
	then
;

: obj-sort> ( val1 val2 -- n )   obj-sort< negate ;
set-current

: object-sort< ( obj -- ary )   <'> obj-sort< object-sort ;
: object-sort> ( obj -- ary )   <'> obj-sort> object-sort ;
previous

\ fth.fs ends here
