#! /usr/bin/env fth
\ install.fth -- installs all *.fs? files to first path in *load-path*.
\
\ @(#)install.fth	1.11 12/8/14

let:
	#t to *fth-verbose*
	"." file-dir { dir }
	nil { scr }
	nil { instpath }
	*load-path* each ( path ) dup
		"." string<> if
			to instpath
			leave
		else
			drop
		then
	end-each
	instpath empty? if
		config-prefix "/share/fth/site-fth" $+ to instpath
	then
	dir each to scr
		scr length 4 > if
			scr -3 nil string-substring ".fs"  string=
			scr -4 nil string-substring ".fsm" string= || if
				\ INSTALL-FILE is unconditional so we
				\ have to test the modification time
				\ ourself.
				scr file-mtime
				instpath "/" $+ scr $+ file-mtime b> if
					scr install-file
				then
			then
		then
	end-each
;let

\ install.fth ends here
