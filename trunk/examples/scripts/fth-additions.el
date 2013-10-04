;;; fth-additions.el -- forth-mode additions handling Fth code.

;; Copyright (c) 2006-2012 Michael Scholz <mi-scholz@users.sourceforge.net>
;; All rights reserved.
;;
;; Redistribution and use in source and binary forms, with or without
;; modification, are permitted provided that the following conditions
;; are met:
;; 1. Redistributions of source code must retain the above copyright
;;    notice, this list of conditions and the following disclaimer.
;; 2. Redistributions in binary form must reproduce the above copyright
;;    notice, this list of conditions and the following disclaimer in the
;;    documentation and/or other materials provided with the distribution.
;;
;; THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
;; ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
;; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
;; ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
;; FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
;; DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
;; OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
;; HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
;; LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
;; OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
;; SUCH DAMAGE.
;;
;; @(#)fth-additions.el	1.7 9/13/13

;;; Commentary:

;; Writing Fth code it is best to use gforth-mode from gforth.el.  You
;; can find it in the Gforth distribution.  gforth.el provides
;; variables we can use for our new defining words etc.  The following
;; code snippet is from my .emacs.el.

;;; Code:

(autoload 'forth-mode          "gforth" "Major mode for GForth." t)
(autoload 'forth-block-mode    "gforth" "Major mode for GForth." t)
(autoload 'inferior-forth-mode "gforth" "Major mode for GForth." t)

(require 'forth-mode "gforth")

(setq forth-jit-parser t)
(setq forth-use-objects nil)
(setq forth-use-oof nil)
(setq forth-hilight-level 3)
(setq forth-indent-level 2)
(setq forth-minor-indent-level 1)

(setq forth-custom-words
      '((("instrument:" "event:") definition-starter (font-lock-keyword-face . 1)
	 "[ \t\n]" t name (font-lock-function-name-face . 3))
	(("lambda:" "let:") definition-starter (font-lock-keyword-face . 1))
	((";instrument" ";event" ";let" ";proc") definition-ender (font-lock-keyword-face . 1))
	((".\\\"") compile-only (font-lock-string-face . 1)
	 "[^\\][\"\n]" nil string (font-lock-string-face . 1))
	(("unless" "?dup-not-if" "?dup-unless" "run" "run-instrument" "end-run"
	  "each" "end-each" "map" "map!" "end-map") compile-only (font-lock-keyword-face . 2))
	(("array(" "#(" "hash{" "#{" "list(" "'(" "gdbm{" "}" "vct(" ")" "make-?obj")
	 non-immediate (font-lock-keyword-face . 2))
	(("[defined]" "[undefined]" "defined?") immediate (font-lock-keyword-face . 2)
	 "[ \t\n]" t name (font-lock-function-name-face . 3))
	(("create-hook" "create-symbol" "create-keyword" "create-exception")
	 non-immediate (font-lock-type-face . 2)
	 "[ \t\n]" t name (font-lock-variable-name-face . 3))
	(("[unless]" "[i]" "[each]" "[map]" "[end-each]" "[end-map]")
	 immediate (font-lock-keyword-face . 2))
	(("+to" "f+to") immediate (font-lock-keyword-face . 2)
	 "[ \t\n]" t name (font-lock-variable-name-face . 3))
	(("nil" "undef" "#t" "#f" "two-pi" "half-pi" "float" "double" "a/o" "r/a")
	 non-immediate (font-lock-constant-face . 1))
	("[-0-9.+]+[-/+]?[-0-9.+]+i?" non-immediate (font-lock-constant-face . 1))
	("[<{]{" non-immediate (font-lock-variable-name-face . 1)
	 "}[>}]" nil string (font-lock-variable-name-face . 1))
	(("}>" "}") compile-only (font-lock-variable-name-face . 1 ))
	(":\\sw+" non-immediate (font-lock-builtin-face . 1)) ;keywords
	("'[^( ]\\sw+" non-immediate (font-lock-constant-face . 1)) ;symbols
	("\*[^ ]+.+\*" immediate (font-lock-variable-name-face . 1)) ;global variables
	("\".*[^\\]\"" immediate (font-lock-string-face . 1)) ;string "" (w/o space)
	("\/.*[^\\]\/" immediate (font-lock-string-face . 1)) ;regexp // (w/o space)
	(("s\\\"" "c\\\"" "$\"" "doc\"") immediate (font-lock-string-face . 1)
	 "[^\\]\"" nil string (font-lock-string-face . 1))
	(("re\/") immediate (font-lock-string-face . 1)
	 "[^\\]\/" nil string (font-lock-string-face . 1))
	(("defines") non-immediate (font-lock-type-face . 2)
	 "[ \t\n]" t name (font-lock-function-name-face . 3))
	(("dl-load") non-immediate (font-lock-keyword-face . 1)
	 "[\n\t ]" t string (font-lock-string-face . 1)
	 "[\n\t ]" t string (font-lock-string-face . 1))
	(("lambda-create") non-immediate (font-lock-type-face . 2) nil)))

(setq forth-custom-indent-words
      '((("instrument:" "event:" "lambda:" "let:"
	  "unless" "?dup-not-if" "?dup-unless" "[unless]" "run" "run-instrument"
	  "each" "map" "map!" "map" "[each]" "[map]") (0 . 2) (0 . 2))
	(("end-run" "end-map" "end-each" "[end-each]" "[end-map]"
	  ";instrument" ";event" ";let" ";proc") (-2 . 0) (0 . -2))
	(("<{") (0 . 3) (0 . 3))
	(("}>") (-3 . 0) (0 . -3))
	(("array(" "#(" "hash{" "#{" "list(" "'(" "gdbm{" "vct(" "<{") (0 . 3) (0 . 3))
	((")" "}") (-3 . 0) (0 . -3))
	(("does>") (-1 . 1) (0 . 0))))

;;; fth-additions.el ends here
