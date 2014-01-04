## Copyright (c) 2006-2014 Michael Scholz <mi-scholz@users.sourceforge.net>
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions
## are met:
## 1. Redistributions of source code must retain the above copyright
##    notice, this list of conditions and the following disclaimer.
## 2. Redistributions in binary form must reproduce the above copyright
##    notice, this list of conditions and the following disclaimer in the
##    documentation and/or other materials provided with the distribution.
##
## THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
## ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
## FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
## DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
## OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
## HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
## LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
## OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
## SUCH DAMAGE.
##
## @(#)fth.m4	1.11 1/4/14

# FTH_CHECK_LIB(action-if-found, [action-if-not-found])
# 
# Usage: FTH_CHECK_LIB([AC_DEFINE([HAVE_FORTH])])
#
# Don't quote this macro: [FTH_CHECK_LIB(...)] isn't correct.
# Instead call it FTH_CHECK_LIB(...).
#
# Checks environment variable $FTH_PROG and name `fth' for valid Fth
# interpreters (for example: setenv FTH_PROG /usr/pkg/bin/fth).
# 
# Six variables will be substituted:
#
# FTH               fth program path         or no
# FTH_VERSION       version string           or ""
# FTH_CFLAGS        -I${prefix}/include/fth  or ""
# FTH_LIBS          -L${prefix}/lib -lfth    or ""
# FTH_HAVE_COMPLEX  yes or no
# FTH_HAVE_RATIO    yes or no

# AC_CHECK_LIB was written by David MacKenzie.
# This version is slightly changed for FTH_CHECK_LIB.

AC_DEFUN([fth_AC_CHECK_LIB], [
	m4_ifval([$3], , [AH_CHECK_LIB([$1])])dnl
	AS_LITERAL_IF([$1],
	    [AS_VAR_PUSHDEF([ac_Lib], [ac_cv_lib_$1_$2])],
	    [AS_VAR_PUSHDEF([ac_Lib], [ac_cv_lib_$1''_$2])])dnl
	AC_CACHE_CHECK([m4_default([$4], [for $2 in -l$1])], ac_Lib,
	    [fth_check_lib_save_LIBS=$LIBS
	     LIBS="-l$1 $5 $LIBS"
	     AC_LINK_IFELSE([AC_LANG_CALL([], [$2])],
	         [AS_VAR_SET(ac_Lib, yes)],
		 [AS_VAR_SET(ac_Lib, no)])
	     LIBS=$fth_check_lib_save_LIBS])
	AS_IF([test AS_VAR_GET(ac_Lib) = yes],
	    [m4_default([$3],
	     [AC_DEFINE_UNQUOTED(AS_TR_CPP(HAVE_LIB$1)) LIBS="-l$1 $LIBS"])])dnl
	AS_VAR_POPDEF([ac_Lib])dnl
])# fth_AC_CHECK_LIB

AC_DEFUN([FTH_CHECK_LIB], [
	AC_PATH_PROGS(FTH, ${FTH_PROG} fth, no)
	FTH_VERSION=""
	FTH_CFLAGS=""
	FTH_LIBS=""
	FTH_HAVE_COMPLEX=no
	FTH_HAVE_RATIO=no
	AC_MSG_CHECKING([for Forth])
	if test "${FTH}" != no ; then
		FTH_VERSION=`${FTH} -e .version`
		FTH_CFLAGS=`${FTH} -e .cflags`
		FTH_LIBS=`${FTH} -e .libs`
		AC_MSG_RESULT([FTH version ${FTH_VERSION}])
		fth_AC_CHECK_LIB([fth],
		    [fth_make_complex],
		    [FTH_HAVE_COMPLEX=yes],
		    [whether FTH supports complex numbers], [${FTH_LIBS}])
		fth_AC_CHECK_LIB([fth],
		    [fth_ratio_floor],
		    [FTH_HAVE_RATIO=yes],
		    [whether FTH supports rational numbers], [${FTH_LIBS}])
		[$1]
	else
		AC_MSG_RESULT([no])
		[$2]
	fi
	AC_SUBST([FTH_VERSION])
	AC_SUBST([FTH_CFLAGS])
	AC_SUBST([FTH_LIBS])
	AC_SUBST([FTH_HAVE_COMPLEX])
	AC_SUBST([FTH_HAVE_RATIO])
])# FTH_CHECK_LIB
	
## fth.m4 ends here
