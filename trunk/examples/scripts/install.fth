#! /usr/opt/bin/fth -Dvs
\ examples/scripts/install.fth.  Generated from install.fth.in by configure.
\ install.fth -- installs executable files in ${prefix}/bin

let:
  "./play-sound.fth" { src }
  src file-exists? if
    src ".fth" file-basename { dest }
    src file-mtime config-prefix "/bin/" $+ dest $+ file-mtime b> if
      "install -v -c %s %s/bin/%s" #( src config-prefix dest ) string-format file-system drop
    then
  then
;let

\ install.fth ends here
