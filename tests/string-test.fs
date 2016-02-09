\ Copyright (c) 2006-2016 Michael Scholz <mi-scholz@users.sourceforge.net>
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
\ @(#)string-test.fs	1.33 2/9/16

require test-utils.fs

: string-test ( -- )
	\ string-length
	"hello" string-length 5 <> "string-length 5" test-expr
	\ string?, char?
	nil string? "nil string?" test-expr
	#() string? "#() string?" test-expr
	0   string? "0 string?"   test-expr
	"fth-test" string? not "\"fth-test\" string?" test-expr
	nil char? "nil char?" test-expr
	#() char? "#() char?" test-expr
	0   char? "0 char?"   test-expr
	<char> m char? not "m char?" test-expr
	\ make-string, >string, string-concat (alias for >string)
	0 make-string ""    string<> "make-string ''"      test-expr
	3 make-string "   " string<> "3 make-string '   '" test-expr
	3 :initial-element <char> x make-string "xxx" string<>
	    "3 x make-string: 'xxx'" test-expr
	0 1 2 " foo " "b" "a" "r"  7 >string "012 foo bar" string<>
	    ">string: '012 foo bar'" test-expr
	\ "" (empty string), $space, $spaces, $cr
	"" 0 :initial-element <char> x make-string string<>
	    "\"\" (empty string)" test-expr
	$space " " string<> "$space" test-expr
	0 $spaces "" string<> "0 $spaces" test-expr
	3 $spaces "   " string<> "3 $spaces" test-expr
	$cr "\n" string<> "$cr" test-expr
	\ fth-format
	"hello" fth-format "hello" string<> "fth-format (1)" test-expr
	"hello %s %d times\n" #( "pumpkin" 10 ) fth-format
	    "hello pumpkin 10 times\n" string<> "fth-format (2)" test-expr
	\ string-cmp
	"foo" { s1 }
	"bar" { s2 }
	"foo" { s3 }
	s1  s2  string-cmp  1 <> "s1 s2 string-cmp 1"   test-expr
	s1  s3  string-cmp  0<>  "s1 s2 string-cmp 0"   test-expr
	s2  s3  string-cmp -1 <> "s1 s2 string-cmp -1"  test-expr
	s1  nil string-cmp  1 <> "s1 nil string-cmp 1"  test-expr
	nil nil string-cmp  0<>  "nil nil string-cmp 0" test-expr
	nil s3  string-cmp -1 <> "nil s3 string-cmp -1" test-expr
	\ string=, string<>, string<, string>
	s1 s2 string=      "s1 s2 string="  test-expr
	s1 s3 string=  not "s1 s3 string="  test-expr
	s3 s3 string=  not "s3 s3 string="  test-expr
	s1 s2 string<> not "s1 s2 string<>" test-expr
	s1 s3 string<>     "s1 s3 string<>" test-expr
	s3 s3 string<>     "s3 s3 string<>" test-expr
	s1 s2 string<      "s1 s2 string<" test-expr
	s1 s3 string<      "s1 s3 string<" test-expr
	s3 s3 string<      "s3 s3 string<" test-expr
	s1 s2 string>  not "s1 s2 string>" test-expr
	s1 s3 string>      "s1 s3 string>" test-expr
	s3 s3 string>      "s3 s3 string>" test-expr
	\ string->array, string-copy
	s1 string->array #( 102 111 111 ) array= not "string->array" test-expr
	s1 string-copy to s2
	s1 s2 string<> "string-copy (1)" test-expr
	s2 string-reverse! drop
	s1 "foo" string<> "string-copy (2)" test-expr
	s2 "oof" string<> "string-copy (3)" test-expr
	\ string-ref|set!
	s1  0 string-ref <char> f <> "string-ref (0)"   test-expr
	s1 -1 string-ref <char> o <> "string-ref (-1)"  test-expr
	s1 0 <char> z string-set!
	s1  0 string-ref <char> z <> "string-set! (0)"  test-expr
	s1 -1 <char> y string-set!
	s1 -1 string-ref <char> y <> "string-set! (-1)" test-expr
	s1 0 <char> f string-set!
	s1 -1 <char> o string-set!
	s1 "foo" string<> "reset after string-set!" test-expr
	\ string-push|pop|unshift|shift
	s1 $space string-push drop
	s1 10 string-push drop
	s1 "foo 10" string<> "string-push (1)" test-expr
	"fth-test" "-string-test" string-push "fth-test-string-test" string<>
	    "string-push (2)" test-expr
	"foo" to s1
	s1 string-pop 111 <> "string-pop (1)" test-expr
	s1 string-pop 111 <> "string-pop (2)" test-expr
	s1 string-pop 102 <> "string-pop (3)" test-expr
	s1 string-pop #f  <> "string-pop (4)" test-expr
	"fth-test" string-pop <char> t <> "string-pop (5)" test-expr
	"foo" to s1
	s1 $space string-unshift drop
	s1 10 string-unshift drop
	s1 "10 foo" string<> "string-unshift (1)" test-expr
	"fth-test" "string-test-" string-unshift "string-test-fth-test" string<>
	    "string-unshift (2)" test-expr
	"foo" to s1
	s1 string-shift 102 <> "string-shift (1)" test-expr
	s1 string-shift 111 <> "string-shift (2)" test-expr
	s1 string-shift 111 <> "string-shift (3)" test-expr
	s1 string-shift #f  <> "string-shift (4)" test-expr
	"fth-test" string-shift <char> f <> "string-shift (5)" test-expr
	"" { str }
	128 { len }
	len 0 do str str string-push to str loop
	len 0 do str string-pop drop loop
	str "" string= unless
		"128 string-push|pop: \"\" <> %s?" #( str ) fth-display
	then
	len 0 do str str string-unshift to str loop
	len 0 do str string-shift drop loop
	str "" string= unless
		"128 string-unshift|shift: \"\" <> %s?" #( str ) fth-display
	then
	"" ( str ) 0 len 1- do
		( str ) i string-unshift $space string-unshift
	-1 +loop dup string-shift drop to str
	str #f string-split each ( x )
		string->number i <> "string-unshift loop" test-expr
	end-each
	\ string-append, string-reverse(!)
	"foo" to s1
	"bar" to s2
	s1 s2 string-append to s3
	s1 "foo" string<> "string-append (1)" test-expr
	s2 "bar" string<> "string-append (2)" test-expr
	s3 "foobar" string<> "string-append (3)" test-expr
	"fth-test" { str1 }
	str1 "-string-append" string-append "fth-test-string-append" string<>
	    "string-append" test-expr
	s1 string-reverse to s2
	s1 "foo" string<> "string-reverse (1)" test-expr
	s2 "oof" string<> "string-reverse (2)" test-expr
	s1 string-reverse! to s3
	s1 "oof" string<> "string-reverse! (1)" test-expr
	s3 "oof" string<> "string-reverse! (2)" test-expr
	str1 string-reverse "tset-htf" string<> "string-reverse (3)" test-expr
	str1 string-reverse! "tset-htf" string<> "string-reverse! (3)" test-expr
	str1 string-reverse! "fth-test" string<> "string-reverse! (4)" test-expr
	\ string-insert!, string-delete!
	"foo" to s1
	s1 1 10 string-insert! "f10oo" string<> "string-insert! (1)" test-expr
	s1 "f10oo" string<> "string-insert! (2)" test-expr
	"foo" to s1
	s1 1 string-delete! 111 <> "string-delete! (1)" test-expr
	s1 "fo" string<> "string-delete! (2)" test-expr
	"fth-test" 0 "beg-" string-insert! { str2 }
	str2 "beg-fth-test" string<> "string-insert! (3)" test-expr
	str2 0 string-delete! drop
	str2 "eg-fth-test"  string<> "string-insert! (4)" test-expr
	str2 0 string-delete! drop
	str2 0 string-delete! drop
	str2 0 string-delete! drop
	"" ( str ) 0 len 1- do
		( str ) 0 i object->string $space $+ string-insert!
	-1 +loop dup string-pop drop to str
	str #f string-split each ( x )
		string->number i <> "string-insert! loop" test-expr
	end-each
	\ string-fill, string-index, string-member?, string-find
	"foo" to s1
	s1 <char> a string-fill "aaa" string<> "string-fill (1)" test-expr
	s1 "aaa" string<> "string-fill (2)" test-expr
	3 :initial-element <char> p make-string <char> f string-fill to s1
	    s1 "fff" string<> "string-fill (3)" test-expr
	"hello world" "l"   string-index  2 <> "string-index (1)" test-expr
	"hello world" "orl" string-index  7 <> "string-index (2)" test-expr
	"hello world" "k"   string-index -1 <> "string-index (3)" test-expr
	"hello world" "l"   string-member? #t <> "string-member? (1)" test-expr
	"hello world" "orl" string-member? #t <> "string-member? (2)" test-expr
	"hello world" "k"   string-member? #f <> "string-member? (3)" test-expr
	"hello world" "l" string-find "llo world" string<>
	    "string-find (1)" test-expr
	"hello world" /ell/ string-find "ello world" string<>
	    "string-find (2)" test-expr
	"hello world" /k/ string-find #f <>
	    "string-find (3)" test-expr
	\ string-split
	"foo:bar:baz" ":" string-split #( "foo" "bar" "baz" ) array= not
	    "string-split (:)" test-expr
	"foo:bar:baz" "/:/" string-split #( "foo" "bar" "baz" ) array= not
	    "string-split (/:/)" test-expr
	"foo bar baz" nil string-split #( "foo" "bar" "baz" ) array= not
	    "string-split (n)" test-expr
	"foo bar baz" #f string-split #( "foo" "bar" "baz" ) array= not
	    "string-split (#f)" test-expr
	\ string-substring
	"hello world" 2 4 string-substring "ll" string<>
	    "string-substring (1)" test-expr
	"hello world" -4 -2 string-substring "or" string<>
	    "string-substring (2)" test-expr
	"hello world" -4 nil string-substring "orld" string<>
	    "string-substring (3)" test-expr
	\ string-upcase(!), string-downcase(!), string-capitalize(!)
	"Foo" to s1
	s1 string-upcase to s2
	s1 "Foo" string<> "string-upcase (1)" test-expr
	s2 "FOO" string<> "string-upcase (2)" test-expr
	s1 string-upcase! to s2
	s1 "FOO" string<> "string-upcase! (1)" test-expr
	s2 "FOO" string<> "string-upcase! (2)" test-expr
	"Foo" to s1
	s1 string-downcase to s2
	s1 "Foo" string<> "string-downcase (1)" test-expr
	s2 "foo" string<> "string-downcase (2)" test-expr
	s1 string-downcase! to s2
	s1 "foo" string<> "string-downcase! (1)" test-expr
	s2 "foo" string<> "string-downcase! (2)" test-expr
	"foO" to s1
	s1 string-capitalize to s2
	s1 "foO" string<> "string-capitalize (1)" test-expr
	s2 "Foo" string<> "string-capitalize (2)" test-expr
	s1 string-capitalize! to s2
	s1 "Foo" string<> "string-capitalize! (1)" test-expr
	s2 "Foo" string<> "string-capitalize! (2)" test-expr
	\ string-replace(!)
	"foo" to s1
	s1 "o" "a" string-replace to s2
	s1 "oo" "a" string-replace to s3
	s1 "foo" string<> "string-replace (1)" test-expr
	s2 "faa" string<> "string-replace (2)" test-expr
	s3 "fa"  string<> "string-replace (3)" test-expr
	"foo" to s1
	"foo" to s2
	s1 "o" "a" string-replace! drop
	s2 "oo" "a" string-replace! drop
	s1 "faa" string<> "string-replace (1)" test-expr
	s2 "fa"  string<> "string-replace (2)" test-expr
	"foo" "o" "oo" string-replace "foooo" string<>
	    "string-replace (with same char)" test-expr
	"foo" "o" "oo" string-replace! "foooo" string<>
	    "string-replace! (with same char)" test-expr
	"foo" "o" "" string-replace "f" string<>
	    "string-replace (delete)" test-expr
	"foo" "o" "" string-replace! "f" string<>
	    "string-replace! (delete)" test-expr
	\ string-chomp(!)
	"foo\n" to s1
	"bar" to s2
	s1 string-chomp "foo" string<> "string-chomp (1)" test-expr
	s2 string-chomp "bar" string<> "string-chomp (2)" test-expr
	s1 string-chomp! drop
	s2 string-chomp! drop
	s1 "foo" string<> "string-chomp! (1)" test-expr
	s2 "bar" string<> "string-chomp! (2)" test-expr
	\ string-format
	"%04d %8.2f %b %X %o" #( 128 pi 255 255 255 ) string-format
	    "0128     3.14 11111111 FF 377" string<>
	    "string-format (1)" test-expr
	precision { prec }
	5 set-precision
	"0x%x\t%p  %p\n"  #( 127 #( pi ) #( pi ) ) string-format
	    "\
0x7f\t#<array[1]:  #<float: 3.14159>>  #<array[1]:  #<float: 3.14159>>\n"
	    string<> "string-format (2)" test-expr
	prec set-precision
	\ string-eval(-with-status)
	"3 4 +"     string-eval        7 <> "string-eval (1)" test-expr
	7 "3 4 + +" string-eval       14 <> "string-eval (2)" test-expr
	7 "3 4 + +" string-eval 10 * 140 <> "string-eval (3)" test-expr
	"3 4 +"     string-eval-with-status nip OUT_OF_TEXT <>
	    "string-eval-with-status (1)" test-expr
	7 "3 4 + +" string-eval-with-status nip OUT_OF_TEXT <>
	    "string-eval-with-status (2)" test-expr
	\ string>$, $>string
	"hello" string>$ s" hello" compare 0<> "string>$?" test-expr
	s" hello" $>string "hello" string<>    "$>string?" test-expr
;

*fth-test-count* 0 [do] string-test [loop]

\ string-test.fs ends here
