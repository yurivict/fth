\ Copyright (c) 2006-2014 Michael Scholz <mi-scholz@users.sourceforge.net>
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
\ @(#)string-test.fs	1.31 2/19/14

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
  \ string-cmp
  "foo" { s1 }
  "bar" { s2 }
  "baz" { s3 }
  s1  s2  string-cmp  1 <> "s1 s2 string-cmp 1"   test-expr
  s2  s2  string-cmp  0<>  "s1 s2 string-cmp 0"   test-expr
  s2  s3  string-cmp -1 <> "s1 s2 string-cmp -1"  test-expr
  s1  nil string-cmp  1 <> "s1 nil string-cmp 1"  test-expr
  nil nil string-cmp  0<>  "nil nil string-cmp 0" test-expr
  nil s3  string-cmp -1 <> "nil s3 string-cmp -1" test-expr
  \ string=, string<>, string<, string>
  "hello" to s1
  "hello" to s2
  "hallo" to s3
  s1 s1 string=  not "s1 s1 string="  test-expr
  s1 s2 string=  not "s1 s2 string="  test-expr
  s1 s3 string=      "s1 s3 string="  test-expr
  s1 s1 string<>     "s1 s1 string<>" test-expr
  s1 s2 string<>     "s1 s2 string<>" test-expr
  s1 s3 string<> not "s1 s3 string<>" test-expr
  "hello" to s1
  "abs"   to s2
  "zeta"  to s3
  s1 s1 string<     "s1 s1 string<" test-expr
  s1 s2 string<     "s1 s2 string<" test-expr
  s1 s3 string< not "s1 s3 string<" test-expr
  s1 s1 string>     "s1 s1 string>" test-expr
  s1 s2 string> not "s1 s2 string>" test-expr
  s1 s3 string>     "s1 s3 string>" test-expr
  \ string->array, string-copy
  "abcdefg" string->array #( 97 98 99 100 101 102 103 ) array= not
    "string->array" test-expr
  s1 string-copy to s2
  s1 s2 string<> "string-copy" test-expr
  \ string-ref|set!
  s1  0 string-ref <char> h <> "string-ref (0)"   test-expr
  s1 -1 string-ref <char> o <> "string-ref (-1)"  test-expr
  s1 0 <char> z string-set!
  s1  0 string-ref <char> z <> "string-set! (0)"  test-expr
  s1 -1 <char> y string-set!
  s1 -1 string-ref <char> y <> "string-set! (-1)" test-expr
  s1 0 <char> h string-set!
  s1 -1 <char> o string-set!
  \ string-push|pop|unshift|shift
  "fth-test" "-string-test" string-push "fth-test-string-test" string<>
    "string-push" test-expr
  "fth-test" string-pop <char> t <> "string-pop" test-expr
  "fth-test" "string-test-" string-unshift "string-test-fth-test" string<>
    "string-unshift" test-expr
  "fth-test" string-shift <char> f <> "string-shift" test-expr
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
  "fth-test" { str1 }
  str1 "-string-append" string-append "fth-test-string-append" string<>
    "string-append" test-expr
  str1 string-reverse "tset-htf" string<> "string-reverse" test-expr
  str1 string-reverse! "tset-htf" string<> "string-reverse! (1)" test-expr
  str1 string-reverse! "fth-test" string<> "string-reverse! (2)" test-expr
  \ string-insert!, string-delete!
  "fth-test" 0 "beg-" string-insert! { str2 }
  str2 "beg-fth-test" string<> "string-insert! (1)" test-expr
  str2 0 string-delete! drop
  str2 "eg-fth-test"  string<> "string-insert! (2)" test-expr
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
  3 :initial-element <char> p make-string <char> f string-fill to s1
  s1 "fff" string<> "string-fill" test-expr
  "fth-test" "test" string-index 4 <>  "string-index"   test-expr
  "fth-test" "test" string-member? not "string-member?" test-expr
  "fth-test" "test" string-find    not "string-find"    test-expr
  \ string-split
  "foo:bar:baz" ":" string-split #( "foo" "bar" "baz" ) array= not
    "string-split (:)"   test-expr
  "foo:bar:baz" ";" string-split #( "foo:bar:baz" ) array= not
    "string-split (;)"   test-expr
  "foo bar baz" nil string-split #( "foo" "bar" "baz" ) array= not
    "string-split (n)" test-expr
  "foo bar baz" #f string-split #( "foo" "bar" "baz" ) array= not
    "string-split (#f)" test-expr
  \ string-substring
  "foo:bar:baz"    4   7 string-substring "bar" string<>
    "string-substring (1)" test-expr
  "hello world"    2   4 string-substring "ll"  string<>
    "string-substring (2)" test-expr
  "hello world" -4  -1 string-substring "orl"   string<>
    "string-substring (3)" test-expr
  "hello world" -4 nil string-substring "orld"  string<>
    "string-substring (4)" test-expr
  \ string-upcase(!), string-downcase(!), string-capitalize(!)
  "AbCdEfG" string-upcase      "ABCDEFG" string<> "string-upcase"      test-expr
  "AbCdEfG" string-downcase    "abcdefg" string<> "string-downcase"    test-expr
  "AbCdEfG" string-capitalize  "Abcdefg" string<> "string-capitalize"  test-expr
  "AbCdEfG" string-upcase!     "ABCDEFG" string<> "string-upcase!"     test-expr
  "AbCdEfG" string-downcase!   "abcdefg" string<> "string-downcase!"   test-expr
  "AbCdEfG" string-capitalize! "Abcdefg" string<> "string-capitalize!" test-expr
  \ string-replace(!)
  "aaabbbccc" "a" "z" string-replace  "zzzbbbccc" string<>
    "string-replace"  test-expr
  "aaabbbccc" "a" "z" string-replace! "zzzbbbccc" string<>
    "string-replace!" test-expr
  "aaabbbcaa" "a" ""  string-replace  "bbbc"      string<>
    "string-replace"  test-expr
  "aaabbbcaa" "a" ""  string-replace! "bbbc"      string<>
    "string-replace!" test-expr
  \ string-chomp(!)
  "fth-test\n" string-chomp  "fth-test" string<> "string-chomp (1)"  test-expr
  "fth-test"   string-chomp  "fth-test" string<> "string-chomp (2)"  test-expr
  "fth-test\n" string-chomp! "fth-test" string<> "string-chomp! (1)" test-expr
  "fth-test"   string-chomp! "fth-test" string<> "string-chomp! (2)" test-expr
  \ string-format
  "%04d %8.2f %b %X %o"  #( 128 pi 255 255 255 ) string-format
  "0128     3.14 11111111 FF 377" string<> "string-format (1)" test-expr
  precision { prec }
  5 set-precision
  "0x%x\t%p  %p\n"  #( 127 #( pi ) #( pi ) ) string-format
  "0x7f\t#<array[1]:  #<float: 3.14159>>  #<array[1]:  #<float: 3.14159>>\n"
    string<> "string-format (2)" test-expr
  prec set-precision
  \ string-eval
  "3 4 +"     string-eval        7 <> "string-eval (1)" test-expr
  7 "3 4 + +" string-eval       14 <> "string-eval (2)" test-expr
  7 "3 4 + +" string-eval 10 * 140 <> "string-eval (3)" test-expr
  \ string>$, $>string
  "hello" string>$ s" hello" compare 0<> "string>$?" test-expr
  s" hello" $>string "hello" string<>    "$>string?" test-expr
;

*fth-test-count* 0 [do] string-test [loop]

\ string-test.fs ends here
