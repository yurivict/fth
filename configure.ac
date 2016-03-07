dnl Copyright (c) 2005-2016 Michael Scholz <mi-scholz@users.sourceforge.net>
dnl All rights reserved.
dnl
dnl Redistribution and use in source and binary forms, with or without
dnl modification, are permitted provided that the following conditions
dnl are met:
dnl 1. Redistributions of source code must retain the above copyright
dnl    notice, this list of conditions and the following disclaimer.
dnl 2. Redistributions in binary form must reproduce the above copyright
dnl    notice, this list of conditions and the following disclaimer in the
dnl    documentation and/or other materials provided with the distribution.
dnl
dnl THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
dnl ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
dnl IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
dnl ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
dnl FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
dnl DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
dnl OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
dnl HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
dnl LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
dnl OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
dnl SUCH DAMAGE.
dnl
dnl @(#)configure.ac	1.180 3/7/16
dnl
dnl Process this file with autoconf to produce configure.

AC_COPYRIGHT(
[Copyright (c) 2004-2016 Michael Scholz <mi-scholz@users.sourceforge.net>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.])

AC_PREREQ(2.62)
dnl
dnl AC_INIT arg   1: PACKAGE_NAME
dnl AC_INIT arg   2: PACKAGE_VERSION
dnl AC_INIT arg 1+2: PACKAGE_STRING
dnl AC_INIT arg   3: PACKAGE_BUGREPORT
dnl AC_INIT arg   4: PACKAGE_TARNAME
dnl AC_INIT arg   5: PACKAGE_URL
dnl
AC_INIT([FTH],
    [1.3.8],
    [mi-scholz@users.sourceforge.net],
    [fth],
    [fth.sourceforge.net])
AC_CONFIG_HEADERS([fth-config.h config.h])
AC_CONFIG_SRCDIR([src/fth.c])
AM_MAINTAINER_MODE		# --enable-maintainer-mode

dnl
dnl Set target name.
dnl
AC_CANONICAL_TARGET
AC_ARG_PROGRAM			# --program-prefix
                                # --program-suffix
				# --program-transform-name
AC_DEFINE([fth], [1], [Define to 1 if you have fth.])
AC_DEFINE_UNQUOTED([FTH_TARGET], ["${target}"],
    [Canonical target string.])
AC_DEFINE_UNQUOTED([FTH_TARGET_CPU], ["${target_cpu}"],
    [Canonical target cpu string.])
AC_DEFINE_UNQUOTED([FTH_TARGET_VENDOR], ["${target_vendor}"],
    [Canonical target vendor string.])
AC_DEFINE_UNQUOTED([FTH_TARGET_OS], ["${target_os}"], 
    [Canonical target os string.])
AC_DEFINE_UNQUOTED([FTH_CONFIGURE_ARGS], ["${ac_configure_args}"],
    [Configure arguments.])
AC_DEFINE_UNQUOTED([FTH_PACKAGE_NAME], ["${PACKAGE_TARNAME}"],
    [Package name.])
AC_DEFINE_UNQUOTED([FTH_PACKAGE_VERSION], ["${PACKAGE_VERSION}"],
    [Package version.])

dnl
dnl Additional configure command line options.
dnl
AC_ARG_ENABLE([shared], [AS_HELP_STRING([--disable-shared],
    [disable shared library support])],
    [AC_MSG_CHECKING([whether to disable shared library support])
	if test "X${enableval}" != "Xyes"; then
		AC_MSG_RESULT([yes])
	else
		AC_MSG_RESULT([no])
	fi])
AC_ARG_ENABLE([warnings],[AS_HELP_STRING([--enable-warnings],
    [enable C compiler warning flags @<:@default=no@:>@])],
    [AC_MSG_CHECKING([whether to enable C compiler warning flags])
     AC_MSG_RESULT([${enableval}])])
AC_ARG_WITH([tecla], [AS_HELP_STRING([--without-tecla],
    [do not use tecla(7) command-line editing])])
AC_ARG_WITH([tecla-prefix], [AS_HELP_STRING([--with-tecla-prefix@<:@=DIR@:>@],
    [search for tecla(7) in DIR/include and DIR/lib])],
    [CFLAGS="${CFLAGS} -I${withval}/include"
     CPPFLAGS="${CPPFLAGS} -I${withval}/include"
     LDFLAGS="${LDFLAGS} -L${withval}/lib"])

dnl
dnl Check for programs.
dnl
AC_PROG_CC
AC_GNU_SOURCE
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_CHECK_TOOL([AR], [ar], [:])
AC_CHECK_TOOL([STRIP], [strip], [:])
AC_CHECK_TOOL([MANDOC], [mandoc], [:])
AC_CHECK_TOOL([LLI], [lli], [:])
AC_CHECK_TOOL([LLC], [llc], [:])

dnl
dnl Check for header files.
dnl
AC_HEADER_STDBOOL
AC_CHECK_HEADERS([arpa/inet.h complex.h dirent.h dlfcn.h])
AC_CHECK_HEADERS([errno.h fcntl.h float.h])
AC_CHECK_HEADERS([limits.h netdb.h netinet/in.h openssl/bn.h openssl/err.h])
AC_CHECK_HEADERS([regex.h setjmp.h signal.h stdarg.h sys/param.h sys/socket.h])
AC_CHECK_HEADERS([sys/time.h sys/times.h sys/uio.h sys/un.h sys/wait.h time.h])
# Special FreeBSD library 'libmissing'
# added to ports/math/libmissing (2012/12/20).
AC_CHECK_HEADERS([missing_math.h missing_complex.h],
    [], [], [@%:@include <complex.h>])

dnl
dnl Check for type sizes, typdefs, structs etc.
dnl
AC_CHECK_SIZEOF([long])
AC_CHECK_SIZEOF([long long])
AC_CHECK_SIZEOF([void *])
AC_CHECK_ALIGNOF([void *])
AC_DEFINE_UNQUOTED([FTH_SIZEOF_LONG], [${ac_cv_sizeof_long}],
    [The size of `long', as computed by sizeof.])
AC_DEFINE_UNQUOTED([FTH_SIZEOF_LONG_LONG], [${ac_cv_sizeof_long_long}],
    [The size of `long long', as computed by sizeof.])
AC_DEFINE_UNQUOTED([FTH_SIZEOF_VOID_P], [${ac_cv_sizeof_void_p}],
    [The size of `void *', as computed by sizeof.])
AC_DEFINE_UNQUOTED([FTH_ALIGNOF_VOID_P], [${ac_cv_alignof_void_p}],
    [The normal alignment of `void *', in bytes.])

AC_STRUCT_TM
AC_CHECK_MEMBERS([struct tm.tm_gmtoff, struct tm.tm_zone])
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_PID_T
AC_TYPE_UID_T
AC_TYPE_SIGNAL
AC_CHECK_MEMBERS([struct dirent.d_ino,
    struct dirent.d_fileno, struct dirent.d_namlen], [], [], [
@%:@if HAVE_DIRENT_H
@%:@include <dirent.h>
@%:@endif])
AC_CHECK_TYPES([long long])
AC_CHECK_TYPES([sig_t], [], [], [
@%:@if HAVE_SIGNAL_H
@%:@include <signal.h>
@%:@endif])
AC_CHECK_TYPES([struct sockaddr_un], [], [], [
@%:@if HAVE_SYS_UN_H
@%:@include <sys/un.h>
@%:@endif])
AC_CHECK_MEMBERS([struct sockaddr_un.sun_len], [], [], [
@%:@if HAVE_SYS_UN_H
@%:@include <sys/un.h>
@%:@endif])
AC_CHECK_TYPES([struct tms], [], [], [
@%:@if HAVE_SYS_TIMES_H
@%:@include <sys/times.h>
@%:@endif])
AC_CHECK_DECLS([sys_siglist], [], [], [
@%:@if HAVE_SIGNAL_H
@%:@include <signal.h>
@%:@endif])

dnl
dnl Check for libraries.
dnl
AC_CHECK_LIB([c], [main])
AC_CHECK_LIB([m], [main])
AC_CHECK_LIB([crypto], [BN_new])
# Special FreeBSD library 'libmissing'
# added to ports/math/libmissing (2012/12/20).
AC_CHECK_LIB([missing], [acoshl])
AC_SEARCH_LIBS([socket], [socket])
AC_SEARCH_LIBS([gethostbyname], [nsl])
AC_SEARCH_LIBS([dlopen], [c dl], [AC_DEFINE([HAVE_DLOPEN], [1],
    [Define to 1 if you have `dlopen'.])])
AC_SEARCH_LIBS([regcomp], [c regex gnuregex], [AC_DEFINE([HAVE_POSIX_REGEX],
    [1], [Define to 1 if you have POSIX regex.])])
AC_CHECK_DECLS([isnan, isinf], [], [], [@%:@include <math.h>])

dnl
dnl Check for functions.	      
dnl
# Minix seems to lack asinh(3), acosh(3), atanh(3)
AC_CHECK_FUNCS([acosh asinh atanh])
AC_CHECK_FUNCS([access ceil chdir chmod chroot])
AC_CHECK_FUNCS([execlp execvp floor fork frexp ftruncate])
AC_CHECK_FUNCS([getegid getenv geteuid getgid gethostname getlogin getpid])
AC_CHECK_FUNCS([getppid getservbyname getservbyport gettimeofday getuid])
AC_CHECK_FUNCS([issetugid kill labs ldexp log2 lstat])
AC_CHECK_FUNCS([mkdir mkfifo opendir pow psignal qsort qsort_r])
AC_CHECK_FUNCS([realpath rename rint rmdir setegid setenv seteuid setgid])
AC_CHECK_FUNCS([sethostname setlocale setuid sleep strncasecmp strerror])
AC_CHECK_FUNCS([strptime symlink sysconf times trunc truncate])
AC_CHECK_FUNCS([tzset utimes wait waitpid])

dnl
dnl Check for --with-tecla.
dnl
if test "X${with_tecla}" != "Xno"; then
	AC_CHECK_HEADERS([libtecla.h])
	# This is required if only a static library exists (libtecla.a).
	AC_SEARCH_LIBS([tputs], [curses ncurses])
	AC_CHECK_LIB([tecla], [new_GetLine])
fi

dnl
dnl Missing functions with replacements in lib/.
dnl
AC_CONFIG_LIBOBJ_DIR(lib)
# getopt(3)
AC_CHECK_FUNC([getopt], [], [AC_LIBOBJ(getopt)])
# getopt_long(3)
AC_CHECK_FUNC([getopt_long], [], [AC_LIBOBJ(getopt_long)])
# snprintf(3)
AC_CHECK_FUNC([snprintf], [AC_DEFINE([HAVE_SNPRINTF], [1],
    [Define to 1 if you have `snprintf'.])], [AC_LIBOBJ(snprintf)])
# strftime(3)
AC_CHECK_FUNC([strftime], [AC_DEFINE([HAVE_STRFTIME], [1],
    [Define to 1 if you have `strftime'.])], [AC_LIBOBJ(strftime)])
# strsep(3)
AC_CHECK_FUNC([strsep],[AC_DEFINE([HAVE_STRSEP], [1],
    [Define to 1 if you have `strsep'.])], [AC_LIBOBJ(strsep)])
# strsignal(3)
AC_CHECK_FUNC([strsignal],[AC_DEFINE([HAVE_STRSIGNAL], [1],
[Define to 1 if you have `strsignal'.])], [AC_LIBOBJ(strsignal)])

dnl
dnl Check for C99 complex support.
dnl
AC_MSG_CHECKING([for _Complex_I])
AC_RUN_IFELSE([AC_LANG_PROGRAM([
@%:@if HAVE_COMPLEX_H
@%:@include <complex.h>
@%:@endif],
    [return (creal(1.0 + 1.0 * _Complex_I) != 1.0);])],
    [AC_MSG_RESULT([yes])
     AC_DEFINE([HAVE_COMPLEX_I], [1], [Define to 1 if you have `_Complex_I'.])],
    [AC_MSG_RESULT([no])])
AC_MSG_CHECKING([for 1.0fi])
AC_RUN_IFELSE([AC_LANG_PROGRAM([
@%:@if HAVE_COMPLEX_H
@%:@include <complex.h>
@%:@endif],
    [return (creal(1.0 + 1.0 * 1.0fi) != 1.0);])],
    [AC_MSG_RESULT([yes])
     AC_DEFINE([HAVE_1_0_FI], [1], [Define to 1 if compiler accepts `1.0fi'.])],
    [AC_MSG_RESULT([no])])
AC_CHECK_FUNCS([cabs cabs2 cacos cacosh carg casin casinh])
AC_CHECK_FUNCS([catan catan2 catanh ccos ccosh cexp clog clog10])
AC_CHECK_FUNCS([conj cpow csin csinh csqrt ctan ctanh])

dnl
dnl Extensions in examples/.
dnl
# dbm(3)
SO_DBM_LIB=""
AC_CHECK_HEADERS([ndbm.h])
AC_SEARCH_LIBS([dbm_open], [c dbm ndbm],
    [SO_DBM_LIB="dbm.so"
     AC_DEFINE([HAVE_DBM], [1], [Define to 1 if you have `dbm_open'.])])

dnl
dnl From sndlib/autoconf.ac.
dnl
# If prefix isn't set in the environment eg. via prefix=/home/mike etc,
# ${prefix} isn't set here!
if test "X${prefix}" = "XNONE"; then
	prefix="${ac_default_prefix}"
fi
SO_LD="${CC}"
SO_LDFLAGS=""
LDFLAGS="${LDFLAGS} -L${prefix}/lib"
case "${host}" in
	*-sgi* | *-apple-*)
		SO_LD=":"
		enable_shared="no"
		;;
	*-*-solaris*)
		SO_LDFLAGS="-G"
		;;
	*-*-minix*)
		# no -shared and -fPIC
		enable_shared="no"
		;;
	*-*-netbsd*)
		CFLAGS="-fPIC ${CFLAGS}"
		SO_LDFLAGS="-shared"
		;;
	*-*-openbsd*)
		CFLAGS="-fPIC ${CFLAGS}"
		SO_LDFLAGS="-shared -fPIC"
		LDFLAGS="${LDFLAGS} -Wl,-E"
		;;
	# Fri Mar  8 00:20:24 CET 2013 [ms]
	# Advised by Hannu Vuolasaho <vuokkosetae@gmail.com>
	*-mingw*)
		# no -fPIC
		;;
	*)
		CFLAGS="-fPIC ${CFLAGS}"
		SO_LDFLAGS="-shared"
		;;
esac
AC_SUBST(SO_LD)
AC_SUBST(SO_LDFLAGS)

CWARNFLAGS="-ansi -std=iso9899:1999"
CWARNFLAGS="${CWARNFLAGS} -Wall -Wextra -Wunused -Wno-unused-parameter"
CWARNFLAGS="${CWARNFLAGS} -Wmissing-declarations"
CWARNFLAGS="${CWARNFLAGS} -Wmissing-prototypes"
CWARNFLAGS="${CWARNFLAGS} -Wundef"
#
# CWARNFLAGS="${CWARNFLAGS} -Wconversion"
# clang: okay
# gcc42: doesn't know char and bool
#
# CWARNFLAGS="${CWARNFLAGS} -pedantic"
# Pedantic is problematic because of
# - ISO C restricts enumerator values to range of 'int'
#   (ficlInstructionEightByteTrick in ficl/ficl.h)
# - imaginary constants are an extension (_Complex_I in src/numbers.c)
# - use of non-standard escape character ('\e' in src/utils.c)
#
# For testing use
# 	% ./configure --enable-warnings CFLAGS=-pedantic
# instead.
#

dnl
dnl Check for --enable-warnings.
dnl
if test "X${enable_warnings}" = "Xyes"; then
	CFLAGS="${CFLAGS} ${CWARNFLAGS}"
fi

dnl
dnl Check for --program-prefix|suffix.
dnl
# prog_name: --program-suffix="-1.3" ==> bin/fth-1.3
#                                        include/fth-1.3/
#                                        lib/fth-1.3/
#                                        lib/libfth-1.3.so
prog_name="fth"
if test "X${program_prefix}" != "XNONE" ; then
	prog_name="${program_prefix}${prog_name}"
fi
if test "X${program_suffix}" != "XNONE" ; then
	prog_name="${prog_name}${program_suffix}"
fi

dnl
dnl Check for --enable-shared.
dnl
if test "X${enable_shared}" != "Xno"; then
	FTH_PROG="fth-shared"
	FTH_LIB="lib${prog_name}.so"
else
	AC_DEFINE([FTH_STATIC], [1], [Define to 1 if you want static linking.])
	FTH_PROG="fth-static"
	FTH_LIB="lib${prog_name}.a"
	SO_DBM_LIB=""
fi

dnl
dnl Substitute variables and write config files.
dnl
AC_SUBST(FTH_PROG)
AC_SUBST(FTH_LIB)
AC_DEFINE_UNQUOTED([FTH_LIBS], ["${LIBS}"], [Required libraries.])
AC_SUBST(prog_name)
AC_SUBST(SO_DBM_LIB)

AC_CONFIG_TESTDIR(tests)
AC_CONFIG_FILES([
    Makefile
    ficl/Makefile
    lib/Makefile
    src/Makefile
    tests/Makefile
    tests/atlocal 
    examples/dbm/Makefile])
AC_CONFIG_FILES([examples/scripts/fth-test.fth],
    [chmod +x examples/scripts/fth-test.fth])
AC_CONFIG_FILES([examples/scripts/install.fth],
    [chmod +x examples/scripts/install.fth])
AC_CONFIG_FILES([examples/scripts/play-sound.fth],
    [chmod +x examples/scripts/play-sound.fth])
AC_CONFIG_FILES([examples/scripts/xm.fth],
    [chmod +x examples/scripts/xm.fth])
AC_CONFIG_FILES([stamp-h], [echo timestamp > stamp-h])
AC_CONFIG_FILES([fth.sh], [chmod +x fth.sh])
AC_OUTPUT

dnl
dnl Check for --enable-maintainer-mode
dnl
if test "X${MAINTAINER_MODE_TRUE}" != "X#"; then
	AC_MSG_RESULT([
${PACKAGE_STRING} configuration summary:

Program name ........... : ${prog_name}
Library name ........... : ${FTH_LIB}
Install prefix ......... : ${prefix}
C preprocessor ......... : ${CPP}
C compiler ............. : ${CC}
Shared linker .......... : ${SO_LD}
C preprocessor flags ... : ${CPPFLAGS}
C compiler flags ....... : ${CFLAGS}
Linker flags ........... : ${LDFLAGS}
Shared linker flags .... : ${SO_LDFLAGS}
Libraries .............. : ${LIBS}
])
fi

dnl configure.ac ends here.
