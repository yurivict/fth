#! /usr/opt/bin/fth -Dvs
\ examples/scripts/play-sound.fth.  Generated from play-sound.fth.in by configure.
\ play-sound.fth -- plays and records sound files

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
\
\ Ident: $Id$

*filename* nil file-basename constant *script-name*
"This is %s v%s, (c) 2006-2012 Michael Scholz" '( *script-name* string-upcase
   "$Revision: 1.14 $" nil string-split 1 array-ref ) string-format constant *banner*
  
SIGINT lambda: { sig -- }
  "\n\nSignal %d received.  Process %d canceled.\n" '( sig getpid ) fth-print
  2 (bye)
; signal drop

require clm

\ Main program starts here.
let: ( -- )
  #t to opterr                          \ getopt prints error messages
  #( #( "play"        required-argument <char> p )
     #( "search-path" required-argument <char> P )
     #( "record"      required-argument <char> r )
     #( "duration"    required-argument <char> d )
     #( "channels"    required-argument <char> c )
     #( "srate"       required-argument <char> s )
     #( "header-type" required-argument <char> t )
     #( "data-format" required-argument <char> f )
     #( "buffer-size" required-argument <char> b )
     #( "device"      required-argument <char> i )
     #( "quiet"             no-argument <char> q )
     #( "version"           no-argument <char> V )
     #( "help"              no-argument <char> h ) ) { opts }
  "p:P:r:d:c:s:t:f:b:i:qVh" { args }
  #f                { play-file }
  #f                { record-file }
  10.0              { duration }
  1                 { channels }
  44100             { srate }
  mus-next          { header-type }
  mus-lfloat        { data-format }
  512               { rt-bufsize }
  mus-audio-default { device }
  #t                { verbose }
  begin
    *argv* args opts getopt-long ( ch ) dup
  while
      ( ch ) case
	<char> p of
	  optarg to play-file
	endof
	<char> P of
	  *clm-search-list* optarg array-push drop
	endof
	<char> r of
	  optarg to record-file
	endof
	<char> d of
	  optarg string->number to duration
	endof
	<char> c of
	  optarg string->number to channels
	endof
	<char> s of
	  optarg string->number to srate
	endof
	<char> t of
	  optarg string->number to header-type
	endof
	<char> f of
	  optarg string->number to data-format
	endof
	<char> b of
	  optarg string->number to rt-bufsize
	endof
	<char> i of
	  optarg string->number to device
	endof
	<char> q of
	  #f to verbose
	endof
	<char> V of
	  *banner* .$ cr 0 (bye)
	endof
        <char> h of
          "Usage: " *script-name* << " [-" << args << "] [ sound-file ... ]
Plays and records sound files.  Checks environment variable
$CLM_SEARCH_PATH, a colon separated path list, for sound files.
Play:
  -p, --play FILE            play FILE
  -P, --search-path PATH     PATH will be added to *clm-search-list*
                             can be called more than once
Record:
  -r, --record FILE          record to FILE
  -d, --duration SECS        record SECS seconds (" << duration number->string << ")
  -c, --channels CHANS       record with CHANS channels (" << channels number->string << ")
  -s, --srate SRATE          record with SRATE (" << srate number->string << ")
  -t, --header-type TYPE     record file with header TYPE (mus-next)
  -f, --data-format FORMAT   record file with data FORMAT (mus-lfloat)
  -b, --buffer-size SIZE     record with buffer SIZE (" << rt-bufsize number->string << ")
  -i, --device DEVICE        record from DEVICE (mus-audio-default)
General:
  -q, --quiet                do not print sound information
  -V, --version              display version information and exit
  -h, --help                 display this help message and exit
% play-sound bell.snd cage.wave
% play-sound -r test.snd -c 2 -s 48000 -d \"60 60 *\"
Please report bugs to <mi-scholz@users.sourceforge.net>." << .$ cr
          0 (bye)
        endof
        <char> ? of 1 (bye) endof       \ prints error message about invalid option
      endcase
  repeat ( ch ) drop
  \
  *argc* optind - to *argc*
  optind 0 ?do *argv* array-shift drop loop
  \
  verbose if ." \ " *banner* .$ cr then
  play-file if
    play-file find-file file-exists? if
      play-file :verbose verbose play-sound
    else
      "%s doesn't exist in %s" #( play-file *clm-search-list* ) fth-warning
    then
    0 (bye)
  then
  record-file if
    record-file
    :duration      duration
    :verbose       verbose
    :channels      channels
    :srate         srate
    :header-type   header-type
    :data-format   data-format
    :output-device device
    :dac-size      rt-bufsize  record-sound
    0 (bye)
  then
  *argc* 0> if
    *argv* each { file }
      file find-file file-exists? if
	file :verbose verbose play-sound
      else
	"%s doesn't exist in %s" #( file *clm-search-list* ) fth-warning
      then
    end-each
    0 (bye)
  then
  verbose if ." \ nothing to do" cr then
  0 (bye)
;let

\ play-sound.fth ends here