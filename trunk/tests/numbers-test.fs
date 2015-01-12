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
\ @(#)numbers-test.fs	1.27 1/11/15

require test-utils.fs

: fneq-err ( r1 r2 err -- f )
	-rot f- fabs f<=
;

: cneq-err { c1 c2 err -- f }
	c1 real-ref c2 real-ref err fneq-err
	c1 imag-ref c2 imag-ref err fneq-err or
;

: fneq ( a b -- f )
	0.001 fneq-err
;

: cneq ( a b -- f )
	0.001 cneq-err
;

'complex provided? [if]
	: complex-test ( -- )
		1.0+1.0i complex?  not "complex? (complex)?"    test-expr
		1.0+1.0i inexact?  not "inexact? (complex)?"    test-expr
		1.0+1.0i number?   not "number? (complex)?"     test-expr
		1.0+1.0i bignum?       "bignum? (complex)?"     test-expr
		1.0+1.0i exact?        "exact? (complex)?"      test-expr
		1.0+1.0i fixnum?       "fixnum? (complex)?"     test-expr
		1.0+1.0i float?        "float? (complex)?"      test-expr
		1.0+1.0i integer?      "integer? (complex)?"    test-expr
		1.0+1.0i long-long?    "long-long? (complex)?"  test-expr
		1.0+1.0i rational?     "rational? (complex)?"   test-expr
		1.0+1.0i ulong-long?   "ulong-long? (complex)?" test-expr
		1.0+1.0i unsigned?     "unsigned? (complex)?"   test-expr
		\ real-ref
		1+1i real-ref 1.0 f<> "1+1i real-ref 1.0 f<>?" test-expr
		1    real-ref 1.0 f<> "1 real-ref 1.0 f<>?" test-expr
		\ imag-ref
		1+1i imag-ref 1.0 f<> "1+1i imag-ref 1.0 f<>?" test-expr
		1    imag-ref 0.0 f<> "1 imag-ref 0.0 f<>?" test-expr
		\ make-rectangular (alias >complex)
		0.5 0.5 make-rectangular 0.5+0.5i c<>
		"0.5 0.5 make-rectangular 0.5+0.5i c<>?" test-expr
		3 1 >complex 3+1i c<> "3 1 >complex 3+1i c<>?" test-expr
		\ make-polar
		0.5 0.5 make-polar 0.438791+0.239713i cneq
		"0.5 0.5 make-polar 0.438791+0.239713i cneq?" test-expr
		3 1 make-polar 1.620906+2.524413i cneq
		"3 1 make-polar 1.620906+2.524413i cneq?" test-expr
		\ s>c (aliases: >c f>c q>c)
		1   s>c 1+0i c<> "1 s>c 1+0i c<>?" test-expr
		1.0 f>c 1+0i c<> "1.0 f>c 1+0i c<>?" test-expr
		\ c>s
		1+1i c>s 1 <> "1+1i c>s 1 <>?" test-expr 
		1+1i c>s fixnum? not "1+1i c>s (fixnum?)" test-expr
		\ c>f
		1+1i c>f 1.0 f<> "1+1i c>f 1.0 f<>?" test-expr
		1+1i c>f float? not "1+1i c>f (float?)" test-expr
		\ c0=
		0+0i c0= not "0+0i c0= not?" test-expr
		0    c0= not "0 c0= not?" test-expr
		0.0  c0= not "0.0 c0= not?" test-expr
		\ c0<>
		0+0i c0<> "0+0i c0<>?" test-expr
		0    c0<> "0 c0<>?" test-expr
		0.0  c0<> "0.0 c0<>?" test-expr
		\ c=
		1+1i 1.0+1.0i c= not "1+1i 1.0+1.0i c= not?" test-expr
		1    1+0i     c= not "1 1+0i c= not?" test-expr
		1.0  1+0i     c= not "1.0 1+0i c= not?" test-expr
		\ c<>
		1+1i 1.0+1.0i c<> "1+1i 1.0+1.0i c<>?" test-expr
		1    1+0i     c<> "1 1+0i c<>?" test-expr
		1.0  1+0i     c<> "1.0 1+0i c<>?" test-expr
		\ c+
		1i 1i c+ 2+2i c<> "1i 1i c+ 2+2i c<>?" test-expr
		1  1  c+ 2+0i c<> "1 1 c+ 2+0i c<>?" test-expr
		1e 1e c+ 2+0i c<> "1e 1e c+ 2+0i c<>?" test-expr
		\ c-
		1i 1i c- c0<> "1i 1i c- c0<>?" test-expr
		1  1  c- c0<> "1 1 c- c0<>?" test-expr
		1e 1e c- c0<> "1e 1e c- c0<>?" test-expr
		\ c*
		1i 1i c* 0+2i c<> "1i 1i c* 0+2i c<>?" test-expr
		1  1  c* 1+0i c<> "1 1 c* 1+0i c<>?" test-expr
		1e 1e c* 1+0i c<> "1e 1e c* 1+0i c<>?" test-expr
		\ c/
		1i 1i c/ 1+0i c<> "1i 1i c/ 1+0i c<>?" test-expr
		1  1  c/ 1+0i c<> "1 1 c/ 1+0i c<>?" test-expr
		1e 1e c/ 1+0i c<> "1e 1e c/ 1+0i c<>?" test-expr
		\ 1/c
		1i 1/c 0.5-0.5i c<> "1i 1/c 0.5-0.5i c<>?" test-expr
		1  1/c 1+0i     c<> "1 1/c 1+0i c<>?" test-expr
		1e 1/c 1+0i     c<> "1e 1/c 1+0i c<>?" test-expr
		\ math.h/fth.h complex functions
		1+1i carg       0.785398 fneq "carg?" test-expr
		1+1i cabs       1.41421  fneq "cabs?" test-expr
		1-1i cabs2      2.0      f<>  "cabs2?" test-expr
		1+1i 2+0.5i c+  3.0+1.5i cneq "c+?" test-expr
		1+1i 2+0.5i c- -1.0+0.5i cneq "c-?" test-expr
		1+1i 2+0.5i c*  1.5+2.5i cneq "c*?" test-expr
		1+1i 2+0.5i c/  0.588235+0.352941i cneq "c/?" test-expr
		1.5+2.5i   1/c  0.176471-0.294118i cneq "1/c?" test-expr
		1+1i 2+0.5i c** -0.232848+1.33024i cneq "c**?" test-expr
		1+1i conjugate  1-1i               cneq "conjugate?" test-expr
		1+1i csqrt      1.09868+0.45509i   cneq "csqrt?" test-expr
		1+1i cexp       1.46869+2.28736i   cneq "cexp?" test-expr
		1+1i clog       0.34657+0.7854i    cneq "clog?" test-expr
		1+1i clog10     0.150515+0.341094i cneq "clog10?" test-expr
		1+1i csin       1.29846+0.634964i  cneq "csin?" test-expr
		1+1i ccos       0.83373-0.988898i  cneq "ccos?" test-expr
		1+1i ctan       0.271753+1.08392i  cneq "ctan?" test-expr
		1+1i casin      0.666239+1.06128i  cneq "casin?" test-expr
		1+1i cacos      0.904557-1.06128i  cneq "cacos?" test-expr
		1+1i catan      1.01722+0.402359i  cneq "catan?" test-expr
		1+1i 1+1i catan2 0.785398+0i       cneq "catan2?" test-expr
		1+1i csinh      0.634964+1.29846i  cneq "csinh?" test-expr
		1+1i ccosh      0.83373+0.988898i  cneq "ccosh?" test-expr
		1+1i ctanh      1.08392+0.271753i  cneq "ctanh?" test-expr
		1+1i casinh     1.06128+0.666239i  cneq "casinh?" test-expr
		1+1i cacosh     1.06128+0.904557i  cneq "cacosh?" test-expr
		1+1i catanh     0.402359+1.01722i  cneq "catanh?" test-expr
	;
[else]
	<'> noop alias complex-test ( -- )
[then]

'bignum provided? [if]
	: bignum-test ( -- )
		123456789012345678901234567890 bignum? not
		    "bignum? (bignum)?" test-expr
		123456789012345678901234567890 exact? not
		    "exact? (bignum)?" test-expr
		123456789012345678901234567890 integer? not
		    "integer? (bignum)?" test-expr
		123456789012345678901234567890 number? not
		    "number? (bignum)?" test-expr
		123456789012345678901234567890 unsigned? not
		    "unsigned? (bignum)?" test-expr
		123456789012345678901234567890 complex?
		    "complex? (bignum)?" test-expr
		123456789012345678901234567890 fixnum?
		    "fixnum? (bignum)?" test-expr
		123456789012345678901234567890 float?
		    "float? (bignum)?" test-expr
		123456789012345678901234567890 inexact?
		    "inexact? (bignum)?" test-expr
		123456789012345678901234567890 long-long?
		    "long-long? (big)?" test-expr
		123456789012345678901234567890 rational?
		    "rational? (bignum)?" test-expr
		123456789012345678901234567890 ulong-long?
		    "ulong-long? (big)?" test-expr
		\ >bignum
		12 s>d { d12 }
		123 >bignum bignum? not "123 >bignum (bignum)?" test-expr
		12e >bignum bignum? not "12e >bignum (bignum)?" test-expr
		d12 >bignum bignum? not "12d >bignum (bignum)?" test-expr
		1/2 >bignum bignum? not "1/2 >bignum (bignum)?" test-expr
		\ s>b f>b
		123 s>b bignum? not "123 s>b (bignum)?" test-expr
		12e f>b bignum? not "12e f>b (bignum)?" test-expr
		\ b>s b>f
		12 s>b { b12 }
		b12 b>s 12   <> "b12 b>s 12 <>?" test-expr
		b12 b>f 12e f<> "b12 f>s 12e f<>?" test-expr
		\ b0=
		0 b0= not "0 b0=" test-expr
		123456789012345678901234567890 b0= "big b0=" test-expr
		\ b0<>
		0 b0<> "0 b0<>" test-expr
		123456789012345678901234567890 b0<> not "big b0<>" test-expr
		\ b0<
		-1 b0< not "-1 b0<" test-expr
		-123456789012345678901234567890 b0< not "-big b0<" test-expr
		123456789012345678901234567890 b0< "big b0<" test-expr
		\ b0>
		1 b0> not "1 b0>" test-expr
		123456789012345678901234567890 b0> not "big b0>" test-expr
		-123456789012345678901234567890 b0> "-big b0>" test-expr
		\ b0<=
		0 b0<= not "0 b0<=" test-expr
		-123456789012345678901234567890 b0<= not "-big b0<=" test-expr
		123456789012345678901234567890 b0<= "big b0<=" test-expr
		\ b0>=
		0 b0>= not "0 b0>=" test-expr
		123456789012345678901234567890 b0>= not "big b0>=" test-expr
		-123456789012345678901234567890 b0>= "-big b0>=" test-expr
		\ b=
		1 1 b= not "1 1 b=?" test-expr
		123456789012345678901234567890 1 b= "big 1 b=?" test-expr
		123456789012345678901234567890
		    123456789012345678901234567890 b= not
		    "big big b=?" test-expr
		\ b<>
		1 1 b<> "1 1 b<>?" test-expr
		123456789012345678901234567890 1 b<> not "big 1 b<>?" test-expr
		123456789012345678901234567890
		    123456789012345678901234567890 b<> "big big b<>?" test-expr
		\ b<
		1 1 b< "1 1 b<?" test-expr
		123456789012345678901234567890 1 b< "big 1 b<?" test-expr
		123456789012345678901234567890
		    123456789012345678901234567890 b< "big big b<?" test-expr
		\ b>
		1 1 b> "1 1 b>?" test-expr
		123456789012345678901234567890 1 b> not "big 1 b>?" test-expr
		123456789012345678901234567890
		    123456789012345678901234567890 b> "big big b>?" test-expr
		\ b<=
		1 1 b<= not "1 1 b<=?" test-expr
		123456789012345678901234567890 1 b<= "big 1 b<=?" test-expr
		123456789012345678901234567890
		    123456789012345678901234567890 b<= not
		    "big big b<=?" test-expr
		\ b>=
		1 1 b>= not "1 1 b>=?" test-expr
		123456789012345678901234567890 1 b>= not "big 1 b>=?" test-expr
		123456789012345678901234567890
		    123456789012345678901234567890 b>= not
		    "big big b>=?" test-expr
		\ b+
		1 1 b+ 2 b= not "1 1 b+ 2 b=?" test-expr
		1 1 b+ bignum? not "1 1 b+ (bignum)?" test-expr
		123456789012345678901234567890 1 b+
		    123456789012345678901234567891 b= not
		    "big 1 b+ big+1 b=?" test-expr
		1 123456789012345678901234567890 b+
		    123456789012345678901234567891 b= not
		    "1 big b+ 1+big b=?" test-expr
		123456789012345678901234567890 1 b+ bignum? not 
		    "big 1 b+ (bignum)?" test-expr
		1 123456789012345678901234567890 b+ bignum? not 
		    "1 big b+ (bignum)?" test-expr
		123456789012345678901234567890
		    123456789012345678901234567890 b+
		    246913578024691357802469135780 b= not
		    "big big b+ big+big b=?" test-expr
		123456789012345678901234567890
		    123456789012345678901234567890 b+ bignum? not
		    "big big b+ (bignum)?" test-expr
		\ b-
		1 1 b- 0 b= not "1 1 b- 0 b=?" test-expr
		1 1 b- bignum? not "1 1 b- (bignum)?" test-expr
		123456789012345678901234567890 1 b-
		    123456789012345678901234567889 b= not
		    "big 1 b- big-1 b=?" test-expr
		1 123456789012345678901234567890 b-
		    -123456789012345678901234567889 b= not
		    "1 big b- 1-big b=?" test-expr
		123456789012345678901234567890 1 b- bignum? not
		    "big 1 b- (bignum)?" test-expr
		1 123456789012345678901234567890 b- bignum? not
		    "1 big b- (bignum)?" test-expr
		123456789012345678901234567890
		    123456789012345678901234567890 b- 0 b= not
		    "big big b- big-big b=?" test-expr
		123456789012345678901234567890
		    123456789012345678901234567890 b- bignum? not
		    "big big b- (bignum)?" test-expr
		\ b*
		1 -1 b* -1 b= not "1 -1 b* -1 b=?" test-expr
		1 -1 b* bignum? not "1 -1 b* (bignum)?" test-expr
		123456789012345678901234567890 10 b*
		    1234567890123456789012345678900 b= not
		    "big 10 b* big*10 b=?" test-expr
		10 123456789012345678901234567890 b*
		    1234567890123456789012345678900 b= not
		    "10 big b* 10*big b=?" test-expr
		123456789012345678901234567890 10 b* bignum? not
		    "big 10 b* (bignum)?" test-expr
		10 123456789012345678901234567890 b* bignum? not
		    "10 big b* (bignum)?" test-expr
		123456789012345678901234567890
		    123456789012345678901234567890 b*
		    15241578753238836750495351562536198787501905199875019052100
		    b= not "big big b* big*big b=?" test-expr
		123456789012345678901234567890
		    123456789012345678901234567890 b* bignum? not
		    "big big b* (bignum)?" test-expr
		\ b/
		1 -1 b/ -1 b= not "1 -1 b/ -1 b=?" test-expr
		1 -1 b/ bignum? not "1 -1 b/ (bignum)?" test-expr
		123456789012345678901234567890 10 b/
		    12345678901234567890123456789 b= not
		    "big 10 b/ big/10 b=?" test-expr
		10 123456789012345678901234567890 b/ 0 b= not
		    "10 big b/ 0 b=?" test-expr
		123456789012345678901234567890 10 b/ bignum? not
		    "big 10 b/ (bignum)?" test-expr
		10 123456789012345678901234567890 b/ bignum? not
		    "10 big b/ (bignum)?" test-expr
		246913578024691357802469135780
		    123456789012345678901234567890 b/ 2 b= not
		    "big big b/ 2 b=?" test-expr
		246913578024691357802469135780
		    123456789012345678901234567890 b/ bignum? not
		    "big big b/ (bignum)?" test-expr
		\ b**
		123456789012345678901234567890 10 b**
		    822526259969628839104253165869933624624768975718986341753117113191672345101686635234711078432787527087114699126238380568851450669625883238384735536304145587136095844229774592556217075848515269880288897142287955821529180675549369033497201746908666410370342866279796500763077997366010000000000 b<>
		    "big 10 b** big^10 b=?" test-expr
		10 10 b** 10000000000 b<> "10 10 b** 10^10 b<>?" test-expr
		1024 10 b** 1267650600228229401496703205376 b<>
		    "1024 10 b** 1024^10 b<>?" test-expr
		\ bnegate
		123456789012345678901234567890 bnegate
		    -123456789012345678901234567890 b<>
		    "big dnegate -big b=?" test-expr
		-123456789012345678901234567890 bnegate
		    123456789012345678901234567890 b<>
		    "-big dnegate big b=?" test-expr
		10 bnegate -10 b<> "10 dnegate -10 b<>?" test-expr
		-10 bnegate 10 b<> "-10 dnegate 10 b<>?" test-expr
		10 bnegate bignum? not "10 bnegate (bignum)?" test-expr
		-10 bnegate bignum? not "-10 bnegate (bignum)?" test-expr
		\ babs
		123456789012345678901234567890 babs
		    123456789012345678901234567890 b<>
		    "big babs big b<>?" test-expr
		-123456789012345678901234567890 babs
		    123456789012345678901234567890 b<>
		    "-big babs big b<>?" test-expr
		10 babs 10 b<> "10 babs 10 b<>?" test-expr
		-10 babs 10 b<> "-10 babs 10 b<>?" test-expr
		10 babs bignum? not "10 babs (bignum)?" test-expr
		-10 babs bignum? not "-10 babs (bignum)?" test-expr
		\ bmin
		1 2 bmin 1 b<> "1 2 bmin 1 b<>?" test-expr
		2 1 bmin 1 b<> "2 1 bmin 1 b<>?" test-expr
		1 2 bmin bignum? not "1 2 bmin (bignum)?" test-expr
		\ bmax
		1 2 bmax 2 b<> "1 2 bmax 2 b<>?" test-expr
		2 1 bmax 2 b<> "2 1 bmax 2 b<>?" test-expr
		1 2 bmax bignum? not "1 2 bmax (bignum)?" test-expr
		\ b2*
		10 b2* 20 b<> "10 b2* 20 b<>?" test-expr
		10 b2* bignum? not "10 b2* (bignum)?" test-expr
		\ b2/
		10 b2/ 5 b<> "10 b2/ 5 b<>?" test-expr
		10 b2/ bignum? not "10 b2/ (bignum)?" test-expr
		\ blshift
		1234 4 blshift 19744 b<> "1234 4 blshift 19744 b<>?" test-expr
		1234 4 blshift bignum? not "1234 4 blshift (bignum)?" test-expr
		\ brshift
		1232 4 brshift 77 b<> "1232 4 brshift 77 b<>?" test-expr
		1232 4 brshift bignum? not "1232 4 brshift (bignum)?" test-expr
		\ random => 0 .. amp
		0.0 { rnd }
		512 0 do
			3.2 random to rnd
			rnd    f0< "3.2 random f0<?"    test-expr
			rnd 3.2 f> "3.2 random 3.2 f>?" test-expr
		loop
		\ frandom => -amp ... amp
		0.0 to rnd
		512 0 do
			3.2 frandom to rnd
			rnd -3.2 f< "3.2 frandom -3.2 f<?" test-expr
			rnd  3.2 f> "3.2 frandom 3.2 f>?"  test-expr
		loop
	;
[else]
	<'> noop alias bignum-test ( -- )
[then]

'ratio provided? [if]  
	: ratio-test ( -- )
		12345/6789 exact?    not "exact? (ratio)?"      test-expr
		12345/6789 number?   not "number? (ratio)?"     test-expr
		12345/6789 rational? not "rational? (ratio)?"   test-expr
		12345/6789 bignum?       "bignum? (ratio)?"     test-expr
		12345/6789 complex?      "complex? (ratio)?"    test-expr
		12345/6789 fixnum?       "fixnum? (ratio)?"     test-expr
		12345/6789 float?        "float? (ratio)?"      test-expr
		12345/6789 inexact?      "inexact? (ratio)?"    test-expr
		12345/6789 integer?      "integer? (ratio)?"    test-expr
		12345/6789 long-long?    "long-long? (ratio)?"  test-expr
		12345/6789 ulong-long?   "ulong-long? (ratio)?" test-expr
		12345/6789 unsigned?     "unsigned? (ratio)?"   test-expr
		\ make-ratio
		123 456 make-ratio  41/152 q<>
		    "123 456 make-ratio 41/152 q<>?" test-expr
		355 113 make-ratio 355/113 q<>
		    "355 113 make-ratio 355/113 q<>?" test-expr
		\ >ratio
		123/456 >ratio 123/456 q<>
		    "123/456 >ratio 123/456 q<>?" test-expr
		2 >ratio 2/1 q<> "2 >ratio 2/1 q<>?" test-expr
		0.25 >ratio 1/4 q<> "0.25 >ratio 1/4 q<>?" test-expr
		10 >llong >ratio  10/1 q<> "10d >ratio 10/1 q<>?" test-expr
		10 >bignum >ratio 10/1 q<> "10b >ratio 10/1 q<>?" test-expr
		\ q>s q>f
		2/1 q>s 2    <> "2/1 q>s 2 <>?" test-expr
		2/1 q>f 2.0 f<> "2/1 q>f 2.0 f<>?" test-expr
		\ s>q f>q c>q 
		10  s>q 10/1 q<> "10 s>q 10/1 q<>?" test-expr
		10e f>q 10/1 q<> "10e f>q 10/1 q<>?" test-expr
		\ q0=
		0/1 q0= not "0/1 q0= not?" test-expr
		123456789012345678901234567890  q0= "big q0=?" test-expr
		-123456789012345678901234567890 q0= "-big q0=?" test-expr
		10  q0=     "10 q0=?" test-expr
		-10 q0=     "-10 q0=?" test-expr
		\ q0<>
		0/1 q0<>     "0/1 q0<>?" test-expr
		123456789012345678901234567890 q0<> not
		    "big q0<> not?" test-expr
		-123456789012345678901234567890 q0<> not
		    "-big q0<> not?" test-expr
		10  q0<> not "10 q0<> not?" test-expr
		-10 q0<> not "-10 q0<> not?" test-expr
		\ q0<
		0/1 q0<     "0/1 q0<?" test-expr
		123456789012345678901234567890 q0< "big q0<?" test-expr
		-123456789012345678901234567890 q0< not
		    "-big q0< not?" test-expr
		10  q0<     "10 q0<?" test-expr
		-10 q0< not "-10 q0< not?" test-expr
		\ q0>
		0/1 q0>     "0/1 q0>?" test-expr
		123456789012345678901234567890 q0> not "big q0> not?" test-expr
		-123456789012345678901234567890 q0> "-big q0>?" test-expr
		10  q0> not "10 q0> not?" test-expr
		-10 q0>     "-10 q0>?" test-expr
		\ q0<=
		0/1 q0<= not "0/1 q0<= not?" test-expr
		123456789012345678901234567890 q0<= "big q0<=?" test-expr
		-123456789012345678901234567890 q0<= not
		    "-big q0<= not?" test-expr
		10  q0<=     "10 q0<=?" test-expr
		-10 q0<= not "-10 q0<= not?" test-expr
		\ q0>=
		0/1 q0>= not "0/1 q0>= not?" test-expr
		123456789012345678901234567890 q0>= not
		    "big q0>= not?" test-expr
		-123456789012345678901234567890 q0>= "-big q0>=?" test-expr
		10  q0>= not "10 q0>= not?" test-expr
		-10 q0>=     "-10 q0>=?" test-expr
		\ q=
		1 1 b+ { bn2 }
		1/2 2/1 q= "1/2 2/1 q=?" test-expr
		1/2 2.0 q= "1/2 2.0 q=?" test-expr
		0.5 2/1 q= "0.5 2/1 q=?" test-expr
		1/2 bn2 q= "1/2 bn2 q=?" test-expr
		bn2 1/2 q= "bn2 1/2 q=?" test-expr
		1/2 2   q= "1/2 2 q=?" test-expr
		2 1/2   q= "2 1/2 q=?" test-expr
		\ q<>
		1/2 2/1 q<> not "1/2 2/1 q<> not?" test-expr
		1/2 2.0 q<> not "1/2 2.0 q<> not?" test-expr
		0.5 2/1 q<> not "0.5 2/1 q<> not?" test-expr
		1/2 bn2 q<> not "1/2 bn2 q<> not?" test-expr
		bn2 1/2 q<> not "bn2 1/2 q<> not?" test-expr
		1/2 2   q<> not "1/2 2 q<> not?" test-expr
		2 1/2   q<> not "2 1/2 q<> not?" test-expr
		\ q<
		1/2 2/1 q< not "1/2 2/1 q< not?" test-expr
		1/2 2.0 q< not "1/2 2.0 q< not?" test-expr
		0.5 2/1 q< not "0.5 2/1 q< not?" test-expr
		1/2 bn2 q< not "1/2 bn2 q< not?" test-expr
		bn2 1/2 q<     "bn2 1/2 q<?" test-expr
		1/2 2   q< not "1/2 2 q< not?" test-expr
		2 1/2   q<     "2 1/2 q<?" test-expr
		\ q>
		1/2 2/1 q>     "1/2 2/1 q>?" test-expr
		1/2 2.0 q>     "1/2 2.0 q>?" test-expr
		0.5 2/1 q>     "0.5 2/1 q>?" test-expr
		1/2 bn2 q>     "1/2 bn2 q>?" test-expr
		bn2 1/2 q> not "bn2 1/2 q> not?" test-expr
		1/2 2   q>     "1/2 2 q>?" test-expr
		2 1/2   q> not "2 1/2 q> not?" test-expr
		\ q<=
		1/2 2/1 q<= not "1/2 2/1 q<= not?" test-expr
		1/2 2.0 q<= not "1/2 2.0 q<= not?" test-expr
		0.5 2/1 q<= not "0.5 2/1 q<= not?" test-expr
		1/2 bn2 q<= not "1/2 bn2 q<= not?" test-expr
		bn2 1/2 q<=     "bn2 1/2 q<=?" test-expr
		1/2 2   q<= not "1/2 2 q<= not?" test-expr
		2 1/2   q<=     "2 1/2 q<=?" test-expr
		\ q>=
		1/2 2/1 q>=     "1/2 2/1 q>=?" test-expr
		1/2 2.0 q>=     "1/2 2.0 q>=?" test-expr
		0.5 2/1 q>=     "0.5 2/1 q>=?" test-expr
		1/2 bn2 q>=     "1/2 bn2 q>=?" test-expr
		bn2 1/2 q>= not "bn2 1/2 q>= not?" test-expr
		1/2 2   q>=     "1/2 2 q>=?" test-expr
		2 1/2   q>= not "2 1/2 q>= not?" test-expr
		\ q+
		1/2 2/1 q+ 5/2 q<> "1/2 2/1 q+ 5/2 q<>?" test-expr
		1/2 2.0 q+ 5/2 q<> "1/2 2.0 q+ 5/2 q<>?" test-expr
		0.5 2/1 q+ 5/2 q<> "0.5 2/1 q+ 5/2 q<>?" test-expr
		1/2 bn2 q+ 5/2 q<> "1/2 bn2 q+ 5/2 q<>?" test-expr
		bn2 1/2 q+ 5/2 q<> "bn2 1/2 q+ 5/2 q<>?" test-expr
		1/2 2   q+ 5/2 q<> "1/2 2 q+ 5/2 q<>?" test-expr
		2 1/2   q+ 5/2 q<> "2 1/2 q+ 5/2 q<>?" test-expr
		\ q-
		1/2 2/1 q- -3/2 q<> "1/2 2/1 q- -3/2 q<>?" test-expr
		1/2 2.0 q- -3/2 q<> "1/2 2.0 q- -3/2 q<>?" test-expr
		0.5 2/1 q- -3/2 q<> "0.5 2/1 q- -3/2 q<>?" test-expr
		1/2 bn2 q- -3/2 q<> "1/2 bn2 q- -3/2 q<>?" test-expr
		bn2 1/2 q-  3/2 q<> "bn2 1/2 q- 3/2 q<>?" test-expr
		1/2 2   q- -3/2 q<> "1/2 2 q- -3/2 q<>?" test-expr
		2 1/2   q-  3/2 q<> "2 1/2 q- 3/2 q<>?" test-expr
		\ q*
		1/2 2/1 q* 1/1 q<> "1/2 2/1 q* 1/1 q<>?" test-expr
		1/2 2.0 q* 1/1 q<> "1/2 2.0 q* 1/1 q<>?" test-expr
		0.5 2/1 q* 1/1 q<> "0.5 2/1 q* 1/1 q<>?" test-expr
		1/2 bn2 q* 1/1 q<> "1/2 bn2 q* 1/1 q<>?" test-expr
		bn2 1/2 q* 1/1 q<> "bn2 1/2 q* 1/1 q<>?" test-expr
		1/2 2   q* 1/1 q<> "1/2 2 q* 1/1 q<>?" test-expr
		2 1/2   q* 1/1 q<> "2 1/2 q* 1/1 q<>?" test-expr
		\ q/
		1/2 2/1 q/ 1/4 q<> "1/2 2/1 q/ 1/4 q<>?" test-expr
		1/2 2.0 q/ 1/4 q<> "1/2 2.0 q/ 1/4 q<>?" test-expr
		0.5 2/1 q/ 1/4 q<> "0.5 2/1 q/ 1/4 q<>?" test-expr
		1/2 bn2 q/ 1/4 q<> "1/2 bn2 q/ 1/4 q<>?" test-expr
		bn2 1/2 q/ 4/1 q<> "bn2 1/2 q/ 4/1 q<>?" test-expr
		1/2 2   q/ 1/4 q<> "1/2 2 q/ 1/4 q<>?" test-expr
		2 1/2   q/ 4/1 q<> "2 1/2 q/ 4/1 q<>?" test-expr
		\ q**
		1/2 2/1 q** 1/4 q<> "1/2 2/1 q** 1/4 q<>?" test-expr
		\ qnegate
		-1/2 qnegate 1/2 q<> "-1/2 qnegate 1/2 q<>?" test-expr
		1/2 qnegate -1/2 q<> "1/2 qnegate -1/2 q<>?" test-expr
		\ qfloor
		1/2 qfloor   0<>     "1/2 qfloor 0<>?" test-expr
		\ qceil
		1/2 qceil    1 <>    "1/2 qceil 1 <>?" test-expr
		\ qabs
		-1/2 qabs    1/2 q<> "-1/2 qabs 1/2 q<>" test-expr
		1/2 qabs    1/2 q<> "1/2 qabs 1/2 q<>" test-expr
		\ 1/q
		1/2 1/q     2/1 q<> "1/2 1/q 2/1 q<>" test-expr
		-1/2 1/q    -2/1 q<> "-1/2 1/q -2/1 q<>" test-expr
		\ exact->inexact
		3/2 exact->inexact 1.5 f<>
		    "3/2 exact->inexact 1.5 f<>?" test-expr
		\ inexact->exact
		1.5 inexact->exact 3/2 q<>
		    "1.5 inexact->exact 3/2 q<>?" test-expr
		\ numerator
		3/4 numerator 3 <> "3/4 numerator 3 <>?" test-expr
		5   numerator 5 <> "5 numerator 5 <>?" test-expr
		1.5 numerator  0<> "1.5 numerator 0<>?" test-expr
		\ denominator
		3/4 denominator 4 <> "3/4 denominator 4 <>?" test-expr
		5   denominator 1 <> "5 denominator 1 <>?" test-expr
		1.5 denominator 1 <> "1.5 denominator 1 <>?" test-expr
	;
[else]
	<'> noop alias ratio-test ( -- )
[then]

: number-test ( -- )
	\ number?
	10        number?   not "number? (fixnum)?"		test-expr
	3.3       number?   not "number? (float)?"		test-expr
	3e        number?   not "number? (float)?"		test-expr
	make-hash number?       "number? (make-hash)?"		test-expr
	\ fixnum?
	10        fixnum?   not "fixnum? (fixnum)?"		test-expr
	3.3       fixnum?       "fixnum? (float)?"		test-expr
	3e        fixnum?       "fixnum? (float)?"		test-expr
	make-hash fixnum?       "fixnum? (make-hash)?"		test-expr
	\ unsigned?
	10        unsigned? not "unsigned? (fixnum)?"		test-expr
	3.3       unsigned?     "unsigned? (float)?"		test-expr
	3e        unsigned?     "unsigned? (float)?"		test-expr
	make-hash unsigned?     "unsigned? (make-hash)?"	test-expr
	\ long-long?
	10        long-long?    "long-long? (fixnum)?"		test-expr
	3.3       long-long?    "long-long? (float)?"		test-expr
	3e        long-long?    "long-long? (float)?"		test-expr
	make-hash long-long?    "long-long? (make-hash)?"	test-expr
	\ ulong-long?
	10        ulong-long?   "ulong-long? (fixnum)?"		test-expr
	3.3       ulong-long?   "ulong-long? (float)?"		test-expr
	3e        ulong-long?   "ulong-long? (float)?"		test-expr
	make-hash ulong-long?   "ulong-long? (make-hash)?"	test-expr
	\ integer?
	10        integer?  not "integer? (fixnum)?"		test-expr
	3.3       integer?      "integer? (float)?"		test-expr
	3e        integer?      "integer? (float)?"		test-expr
	make-hash integer?      "integer? (make-hash)?"		test-expr
	\ exact?
	10        exact?    not "exact? (fixnum)?" 		test-expr
	3.3       exact?        "exact? (float)?"		test-expr
	3e        exact?        "exact? (float)?"		test-expr
	make-hash exact?        "exact? (make-hash)?"		test-expr
	\ inexact?
	10        inexact?      "inexact? (fixnum)?"		test-expr
	3.3       inexact?  not "inexact? (float)?"		test-expr
	3e        inexact?  not "inexact? (float)?"		test-expr
	make-hash inexact?      "inexact? (make-hash)?"		test-expr
	\ bignum?
	10        bignum?       "bignum? (fixnum)?"		test-expr
	3.3       bignum?       "bignum? (float)?"		test-expr
	3e        bignum?       "bignum? (float)?"		test-expr
	make-hash bignum?       "bignum? (make-hash)?"		test-expr
	\ ratio?
	10        rational?     "rational? (fixnum)?"		test-expr
	3.3       rational?     "rational? (float)?"		test-expr
	3e        rational?     "rational? (float)?"		test-expr
	make-hash rational?     "rational? (make-hash)?"	test-expr
	\ float?
	10        float?        "float? (fixnum)?"		test-expr
	3.3       float?    not "float? (float)?"		test-expr
	3e        float?    not "float? (float)?"		test-expr
	make-hash float?        "float? (make-hash)?"		test-expr
	\ inf?, inf
	10        inf?          "inf? (fixnum)?"		test-expr
	inf       inf?      not "inf? (inf)?"			test-expr
	3e        inf?          "inf? (float)?"			test-expr
	make-hash inf?          "inf? (make-hash)?"		test-expr
	\ nan?, nan
	10        nan?          "nan? (fixnum)?"		test-expr
	nan       nan?      not "nan? (nan)?"			test-expr
	3e        nan?          "nan? (float)?"			test-expr
	make-hash nan?          "nan? (make-hash)?"		test-expr
	\ complex?
	10        complex?      "complex? (fixnum)?"		test-expr
	3.3       complex?      "complex? (float)?"		test-expr
	3e        complex?      "complex? (float)?"		test-expr
	make-hash complex?      "complex? (make-hash)?"		test-expr
	10 make-long-long { dval10 }
	dval10  long-long? not "make-long-long" test-expr
	10 make-ulong-long { udval10 }
	udval10 ulong-long? not "make-ulong-long" test-expr
	\ exact->inexact, inexact->exact
	3 exact->inexact 3.0   f<> "exact->inexact (3.0)" test-expr
	3.3 exact->inexact 3.3 f<> "exact->inexact (3.3)" test-expr
	\ math.h float functions
	-1.0 	  fabs         1.0  f<> "fabs?"        test-expr
	-1.0 2.0  fmod        -1.0  f<> "fmod?"        test-expr
	3.2  	  floor        3.0  f<> "floor?"       test-expr
	3.2  	  fceil        4.0  f<> "fceil?"       test-expr
	3.2  	  ftrunc       3.0  f<> "ftrunc?"      test-expr
	3.49 	  fround       3.0  f<> "fround (1)?"  test-expr
	2.51 	  fround       3.0  f<> "fround (2)?"  test-expr
	2.0 3.0   f**          8.0 fneq "f**?"         test-expr
	9.0  	  fsqrt        3.0  f<> "fsqrt?"       test-expr
	3.0  	  fexp     20.0855 fneq "fexp?"        test-expr
	3.0  	  fexpm1   19.0855 fneq "fexpm1?"      test-expr
	3.0  	  flog     1.09861 fneq "flog?"        test-expr
	3.0  	  flogp1   1.38629 fneq "flogp1?"      test-expr
	3.0  	  flog2    1.58496 fneq "flog2?"       test-expr
	3.0  	  flog10   0.47712 fneq "flog10?"      test-expr
	3.0  	  falog     1000.0 fneq "falog?"       test-expr
	3.0  	  fsin     0.14112 fneq "fsin?"        test-expr
	3.0  	  fcos   -0.989992 fneq "fcos?"        test-expr
	3.0 fsincos { si cs }
	3.0       fsin si          fneq "fsincos sin?" test-expr
	3.0       fcos cs          fneq "fsincos cos?" test-expr
	3.0  	  ftan   -0.142547 fneq "ftan?"        test-expr
	1.0  	  fasin     1.5708 fneq "fasin?"       test-expr
	1.0  	  facos            f0<> "facos?"       test-expr
	3.0  	  fatan    1.24905 fneq "fatan?"       test-expr
	3.0 1.0   fatan2 3.0 fatan fneq "fatan2?"      test-expr
	3.0  	  fsinh    10.0179 fneq "fsinh?"       test-expr
	3.0  	  fcosh    10.0677 fneq "fcosh?"       test-expr
	3.0  	  ftanh   0.995055 fneq "ftanh?"       test-expr
	3.0  	  fasinh   1.81845 fneq "fasinh?"      test-expr
	3.0  	  facosh   1.76275 fneq "facosh?"      test-expr
	0.5  	  fatanh   0.54931 fneq "fatanh?"      test-expr
	complex-test
	bignum-test
	ratio-test
;

*fth-test-count* 0 [do] number-test [loop]

\ numbers-test.fs ends here
