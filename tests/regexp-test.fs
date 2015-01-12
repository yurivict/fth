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
\ @(#)regexp-test.fs	1.17 1/12/15

'regexp provided? [unless] skip-file [then]

require test-utils.fs

: regexp-test ( -- )
	\ regexp?
	"x" make-regexp regexp? not "regexp? (1)" test-expr
	/^s/ regexp? not "regexp? (2)" test-expr
	nil  regexp? "regexp? (3)" test-expr
	\ make-regexp
	"(bar)"  make-regexp { re1 }
	"(B|b)+" make-regexp { re2 }
	re1 regexp? not "re1 make-regexp not" test-expr
	re2 regexp? not "re2 make-regexp not" test-expr
	\ regexp-match (alias regexp= and re=)
	re1 "foobar" regexp-match { match }
	match not "regexp-match (1)" test-expr
	match fixnum? not "regexp-match (2)" test-expr
	re/ (aa*)(b*)(c*)/ "aaabbbccc" regexp-match 9 <>
	    "regexp-match (3)" test-expr
	*re0* "aaabbbccc" string<> "*re0* (1)" test-expr
	*re1* "aaa"       string<> "*re1* (1)" test-expr
	*re2* "bbb"       string<> "*re2* (1)" test-expr
	*re3* "ccc"       string<> "*re3* (1)" test-expr
	/foo/ "foobar" regexp-match 3 <> "regexp-match (4)" test-expr
	re1 "foobar" regexp-match 3 <> "regexp-match (5)" test-expr
	/.*(bar)/ "foobar" regexp-match 6 <> "regexp-match (6)" test-expr
	*re0* "foobar" string<> "*re0* (2)" test-expr
	*re1* "bar"    string<> "*re1* (2)" test-expr
	*re2* #f             <> "*re2* (2)" test-expr
	\ regexp-search
	re1 "foobar" regexp-search 3 <> "regexp-search (1)" test-expr
	re2 "foobar" regexp-search 3 <> "regexp-search (2)" test-expr
	/foo/ "foobar" :start 0 :range 6 regexp-search not
	    "regexp-search (3)" test-expr
	re1 "foobar" :start 0 :range 2 regexp-search
	    "regexp-search (4)" test-expr
	re1 "foobar" :start 3 :range 2 regexp-search 3 <>
	    "regexp-search (5)" test-expr
	re1 0 apply "bar" string<> "re1 0 apply 'bar' not" test-expr
	re1 1 apply "bar" string<> "re1 1 apply 'bar' not" test-expr
	re1 2 apply #f          <> "re1 2 apply #f not"    test-expr
	re2 "foobar" regexp-search 3 <> "regexp-search (6)" test-expr
	\ regexp-replace
	/bar/ "foobar" "BAR" regexp-replace "fooBAR" string<>
	    "regexp-replace (1)" test-expr
	/(foo)/ "foo-bar" "***\\1***" regexp-replace
	    "***foo***-bar" string<>
	    "regexp-replace (2)" test-expr
	/(fo(O|o))/ "bar-foo-bar" "***\\0***\\1***\\2***" regexp-replace
	    "bar-***foo***foo***o***-bar" string<>
	    "regexp-replace (3)" test-expr
	/(foo)/ "foo-bar" "***\\2***" <'> regexp-replace
	    'regexp-error dup fth-catch 'regexp-error <>
	    "regexp-replace (4)" test-expr
	/(foo)/ "foo-bar" "***\\***" <'> regexp-replace
	    'regexp-error dup fth-catch 'regexp-error <>
	    "regexp-replace (5)" test-expr
	\ re-match
	/a*/ "aaaaab" 0 re-match 5 <> "/a*/ 0 re-match 5 <>" test-expr
	/a*/ "aaaaab" 2 re-match 3 <> "/a*/ 2 re-match 3 <>" test-expr
	/a*/ "aaaaab" 5 re-match 0 <> "/a*/ 5 re-match 0 <>" test-expr
	/a*/ "aaaaab" 6 re-match 0 <> "/a*/ 6 re-match 0 <>" test-expr
	\ re-search
	/a*/ "aaaaab" 0 1 re-search 0 <> "/a*/ 0 1 re-search 0 <>" test-expr
	/a*/ "aaaaab" 2 4 re-search 2 <> "/a*/ 2 4 re-search 2 <>" test-expr
;

*fth-test-count* 0 [do] regexp-test [loop]

\ regexp-test.fs ends here
