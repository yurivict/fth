# Copyright (c) 2016-2017 Michael Scholz <mi-scholz@users.sourceforge.net>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# @(#)SConstruct	1.5 12/31/17

#
# scons -h
#

help			= """
scons -h	# help
scons -c	# clean
scons CC=clang prefix=/usr/opt
scons
scons test
scons install
scons uninstall
scons dist
"""

sc_h			= {}
sc_h['prefix']		= "installation prefix"
sc_h['build']		= "build path"
sc_h['tecla_prefix']	= "search for tecla(7) in DIR/{include,lib}"
sc_h['prg_prefix']	= "prepend PREFIX to installed program name"
sc_h['prg_suffix']	= "append SUFFIX to installed program name"
sc_h['shared_l']	= "disable shared library support"
sc_h['shared_v']	= "enable shared library support"
sc_h['warnings']	= "enable extra C compiler warning flags"
sc_h['cc']		= "C compiler"
sc_h['cflags']		= "additional CFLAGS"
sc_h['tecla_l']		= "do not use tecla(7) command-line editing"
sc_h['tecla_v']		= "use tecla(7) command-line editing"
sc_h['mode_t']		= "Define to 'int' if not defined."
sc_h['off_t']		= "Define to 'long int' if not defined."
sc_h['size_t']		= "Define to 'unsigned int' if not defined."
sc_h['pid_t']		= "Define to 'int' if not defined."
sc_h['uid_t']		= "Define to 'int' if not defined."
sc_h['minix']		= "Define to 1 if on MINIX."
sc_h['posix_1_source']	= "Define to 2 if you need it (Minix)."
sc_h['posix_source']	= "Define to 1 if you need it (Minix)."
sc_h['all_source']	= "Enable extensions on AIX 3, Interix."
sc_h['gnu_source']	= "Enable extensions an systems that have them."
sc_h['posix_pthread']	= "Enable threading extensions on Solaris."
sc_h['tandem_source']	= "Enable extensions on HP NonStop."
sc_h['extensions']	= "Enable general extensions on Solaris."

import os
import sys

initfile		= os.getenv('HOME') + '/.scons/fth.py'
build_dir		= 'build'
vars			= Variables(initfile, ARGUMENTS)
prefix			= sys.prefix
tecla_prefix		= sys.prefix
prg_prefix		= ''
prg_suffix		= ''
libtecla		= True 
shared			= True 
warnings		= False

AddOption('--prefix',
	default = prefix,
	help = sc_h['prefix'])

AddOption('--build',
	default = build_dir,
	help = sc_h['build'])

AddOption('--tecla-prefix',
	dest = 'tecla_prefix',
	default = tecla_prefix,
	metavar = 'DIR',
	help = sc_h['tecla_prefix'])

AddOption('--program-prefix',
	default = prg_prefix,
	dest = 'prg_prefix',
	metavar = 'PREFIX',
	help = sc_h['prg_prefix'])

AddOption('--program-suffix',
	default = prg_suffix,
	dest = 'prg_suffix',
	metavar = 'SUFFIX',
	help = sc_h['prg_suffix'])

AddOption('--without-tecla',
	action = 'store_false',
	dest = 'libtecla',
	default = libtecla,
	help = sc_h['tecla_l'])

AddOption('--disable-shared',
	action = 'store_false',
	dest = 'shared',
	default = shared,
	help = sc_h['shared_l'])

AddOption('--enable-warnings',
	action = 'store_true',
	dest = 'warnings',
	default = warnings,
	help = sc_h['warnings'])

prefix			= GetOption('prefix')
tecla_prefix		= GetOption('tecla_prefix')
prg_prefix		= GetOption('prg_prefix')
prg_suffix		= GetOption('prg_suffix')
build_dir		= GetOption('build')
libtecla		= GetOption('libtecla')
shared			= GetOption('shared')
warnings		= GetOption('warnings')

vars.AddVariables(
	('CC', sc_h['cc'], 'cc'),
	('CFLAGS', sc_h['cflags'], ''),
	('build', sc_h['build'], build_dir),
	('program_prefix', sc_h['prg_prefix'], prg_prefix),
	('program_suffix', sc_h['prg_suffix'], prg_suffix),
	PathVariable('prefix', sc_h['prefix'],
	    prefix, PathVariable.PathIsDir),
	PathVariable('tecla_prefix', sc_h['tecla_prefix'],
	    tecla_prefix, PathVariable.PathIsDir),
	BoolVariable('libtecla', sc_h['tecla_v'], libtecla),
	BoolVariable('shared', sc_h['shared_v'], shared),
	BoolVariable('warnings', sc_h['warnings'], warnings))

env = Environment(ENV = os.environ, variables = vars)

Help(help, True)
Help(vars.GenerateHelpText(env))

prefix			= env['prefix']
tecla_prefix		= env['tecla_prefix']
prg_prefix		= env['program_prefix']
prg_suffix		= env['program_suffix']
build_dir		= env['build']
libtecla		= env['libtecla']
shared			= env['shared']
warnings		= env['warnings']
missing			= []
cflags			= env['CFLAGS']
env['CFLAGS']		= []
env.MergeFlags(cflags)
env['top_srcdir']	= Dir('.').path
env['CPPPATH']		= ['.', 'ficl', 'lib', 'src']
if prefix == tecla_prefix:
	env.Append(CPPPATH = [prefix + '/include'])
	env.Append(LIBPATH = [prefix + '/lib'])
else:
	env.Append(CPPPATH = [prefix + '/include', tecla_prefix + '/include'])
	env.Append(LIBPATH = [prefix + '/lib', tecla_prefix + '/lib'])

# os.uname()[0] uname -s	'NetBSD'
# os.uname()[1] uname -n 	'pumpkin.fth-devel.net'
# os.uname()[2] uname -r	'7.99.26'
# os.uname()[3] uname -v	'full version'
# os.uname()[4] uname -m	'amd64'

env['NAME']		= prg_prefix + 'fth' + prg_suffix
env['VERSION']		= '1.3.8'
env['SOURCE_URL']	= 'http://fth.sourceforge.net'
env['LICENSE']		= 'bsd-2-clause'
env['SUMMARY']		= "FTH Forth Scripting"
env['PACKAGETYPE']	= 'src_tarbz2'
arch			= os.uname()[4]
vendor			= os.getenv('VENDOR')
if not vendor:
	vendor = os.name
env['VENDOR']		= vendor
env['HOST_TRIPLE']	= arch + '-' + vendor + '-' + sys.platform
# PACKAGEROOT is default:
# PACKAGEROOT = "${NAME}-${VERSION}"

#
# Tests without corresponding *.h file for later use.
#
def conf_test(env):
	conf	= env.Configure(clean = False, help = False)
	old_libs = ''
	dbm_lib	= ''
	if 'LIBS' in env:
		old_libs = env['LIBS']
	dbm = conf.CheckFunc('dbm_open')
	if not dbm:
		for l in ['dbm', 'ndbm']:
			if conf.CheckLib(l, 'dbm_open'):
				dbm_lib = l
				dbm = True
				break
	env['LIBS']	= old_libs
	env['dbm']	= dbm
	env['dbm_lib']	= dbm_lib
	if not conf.CheckFunc('getopt'):
		missing.append('lib/getopt.c')
	if not conf.CheckFunc('getopt_long'):
		missing.append('lib/getopt_long.c')
	if not conf.CheckFunc('snprintf'):
		missing.append('lib/snprintf.c')
	if not conf.CheckFunc('strftime'):
		missing.append('lib/strftime.c')
	if not conf.CheckFunc('strsep'):
		missing.append('lib/strsep.c')
	if not conf.CheckFunc('strsignal'):
		missing.append('lib/strsignal.c')
	env['missing']	= missing
	env['sizeof_void_p'] = conf.CheckTypeSize('void *')
	env['sizeof_long'] = conf.CheckTypeSize('long')
	env['sizeof_long_long'] = conf.CheckTypeSize('long long')
	env['mode_t'] = conf.CheckType('mode_t', '#include <sys/types.h>')
	env['size_t'] = conf.CheckType('size_t', '#include <sys/types.h>')
	env['off_t'] = conf.CheckType('off_t', '#include <sys/types.h>')
	env['pid_t'] = conf.CheckType('pid_t', '#include <sys/types.h>')
	env['uid_t'] = conf.CheckType('uid_t', '#include <sys/types.h>')
	env['complex_i'] = conf.CheckDeclaration('_Complex_I',
		'#include <complex.h>')
	# Special FreeBSD library 'libmissing'
	# added to ports/math/libmissing (2012/12/20).
	conf.CheckLib('missing', 'acoshl')
	conf.CheckLib('crypto', 'BN_new')
	conf.CheckLib('socket', 'socket')
	conf.CheckLib('nsl', 'gethostbyname')
	env['posix_regex'] = conf.CheckFunc('regcomp')
	if not env['posix_regex']:
		env['posix_regex'] = conf.CheckLib(['regex', 'gnuregex'],
			'regcomp')
	if not conf.CheckFunc('dlopen'):
		conf.CheckLib('dl', 'dlopen')
	if libtecla:
		env['tecla'] = conf.CheckLib('tecla', None)
		if not env['tecla']:
			# This is required if only a static library
			# exists (libtecla.a).
			conf.CheckLib(['curses', 'ncurses'], 'tputs')
			env['tecla'] = conf.CheckLib('tecla', None) 
	else:
		env['tecla'] = False
	conf.CheckLib('m', 'main')
	conf.CheckLib('c', 'main')
	conf.Finish()

#
# src-config.h
#
def CheckMember(self, src_conf_h, struct, member, include):
	self.Message("Checking for C " + struct + "." + member + " ...")
	help = "Define to 1 if `" + struct + "' has member `" + member + "'."
	cmd = "\n\
%s\n\
\n\
int\n\
main(int argc, char *argv[])\n\
{\n\
	%s x;\n\
\n\
	x.%s;\n\
	return (0);\n\
}\n" % (include, struct, member)
	ret = self.TryCompile(cmd, '.c')
	if ret:
		name = struct.upper() + '_' + member.upper()
		name = name.replace(' ', '_')
		src_conf_h.Define('HAVE_' + name, 1, help)
	self.Result(ret)
	return ret

def src_conf_test(env):
	src_conf_h = env.Configure(
		custom_tests = { 'CheckMember' : CheckMember },
		config_h = build_dir + '/src-config.h',
		clean = False, help = False)
	env.AddMethod(CheckMember)
	if env['posix_regex']:
		src_conf_h.Define('HAVE_POSIX_REGEX', 1)
	if env['tecla']:
		src_conf_h.Define('HAVE_LIBTECLA', 1)
	src_conf_h.Define('SIZEOF_VOID_P', env['sizeof_void_p'])
	for m in ['tm_gmtoff', 'tm_zone']:
		src_conf_h.CheckMember(src_conf_h, 'struct tm', m,
			'#include <time.h>')
	for m in ['d_ino', 'd_fileno', 'd_namlen']:
		src_conf_h.CheckMember(src_conf_h, 'struct dirent', m,
			'#include <dirent.h>')
	src_conf_h.CheckMember(src_conf_h, 'struct sockaddr_un', 'sun_len',
		'#include <sys/un.h>')
	src_conf_h.CheckType('struct sockaddr_un', '#include <sys/un.h>')
	src_conf_h.CheckType('struct tms', '#include <sys/times.h>')
	src_conf_h.CheckType('sig_t', '#include <signal.h>')
	for d in ['isinf', 'isnan']:
		src_conf_h.CheckDeclaration(d, '#include <math.h>')
	if not src_conf_h.TryCompile("""
	#include <sys/types.h>
	#include <time.h>
	int
	main(int argc, char *argv[])
	{
		struct tm tm;
		int *p;

		p = &tm.tm_sec;
		return (p == NULL);
	}
	""", '.c'):
		src_conf_h.Define('TM_IN_SYS_TIME', 1)
	if src_conf_h.TryCompile("""
	#include <sys/types.h>
	#include <signal.h>
	int
	main(int argc, char *argv[])
	{
		return (*(signal(0, 0))(0) == 1);
	}
	""", '.c'):
		src_conf_h.Define('RETSIGTYPE', 'int')
	else:
		src_conf_h.Define('RETSIGTYPE', 'void')

	if src_conf_h.TryCompile("""
	#include <stdlib.h>
	#include <stdarg.h>
	#include <string.h>
	#include <float.h>
	int
	main(int argc, char *argv[])
	{
		return (0);
	}
	""", '.c'):
		src_conf_h.Define('STDC_HEADERS', 1)

	if src_conf_h.TryCompile("""
	#include <minix/config.h>
	int
	main(int argc, char *argv[])
	{
		return (0);
	}
	""", '.c'):
		src_conf_h.Define('_MINIX', 1, sc_h['minix'])
		src_conf_h.Define('_POSIX_SOURCE', 1, sc_h['posix_source'])
		src_conf_h.Define('_POSIX_1_SOURCE', 2, sc_h['posix_1_source'])

	if src_conf_h.TryCompile("""
	#define __EXTENSIONS__ 1
	int
	main(int argc, char *argv[])
	{
		return (0);
	}
	""", '.c'):
		src_conf_h.Define('__EXTENSIONS__', 1, sc_h['extensions'])
		src_conf_h.Define('_ALL_SOURCE', 1, sc_h['all_source'])
		src_conf_h.Define('_GNU_SOURCE', 1, sc_h['gnu_source'])
		src_conf_h.Define('_POSIX_PTHREAD_SEMANTICS', 1,
			sc_h['posix_pthread'])
		src_conf_h.Define('_TANDEM_SOURCE', 1, sc_h['tandem_source'])
	for h in ['arpa/inet.h',
		'dirent.h',
		'dlfcn.h',
		'fcntl.h',
		'libtecla.h',
		'ndbm.h',
		'netdb.h',
		'netinet/in.h',
		'openssl/err.h',
		'regex.h',
		'signal.h',
		'stdlib.h',
		'sys/socket.h',
		'sys/stat.h',
		'sys/time.h',
		'sys/times.h',
		'sys/types.h',
		'sys/uio.h',
		'sys/un.h',
		'sys/wait.h',
		'time.h']:
		src_conf_h.CheckCHeader(h)
	# Minix seems to lack asinh(3), acosh(3), atanh(3)
	for f in ['access',
		'acosh',
		'asinh',
		'atanh',
		'ceil',
		'chdir',
		'chmod',
		'chroot',
		'dlopen',
		'execlp',
		'execvp',
		'floor',
		'fork',
		'frexp',
		'ftruncate',
		'getegid',
		'getenv',
		'geteuid',
		'getgid',
		'gethostname',
		'getlogin',
		'getpid',
		'getppid',
		'getservbyname',
		'getservbyport',
		'gettimeofday',
		'getuid',
		'issetugid',
		'kill',
		'ldexp',
		'log2',
		'lstat',
		'mkdir',
		'mkfifo',
		'opendir',
		'pow',
		'psignal',
		'qsort',
		'realpath',
		'rename',
		'rint',
		'rmdir',
		'setegid',
		'setenv',
		'seteuid',
		'setgid',
		'sethostname',
		'setuid',
		'sleep',
		'snprintf',
		'strerror',
		'strptime',
		'strsignal',
		'symlink',
		'sysconf',
		'times',
		'trunc',
		'truncate',
		'tzset',
		'utimes',
		'wait',
		'waitpid']:
		src_conf_h.CheckFunc(f)
	if env['dbm']:
		src_conf_h.Define('HAVE_DBM', 1)
	src_conf_h.Define('fth', 1)
	e = src_conf_h.Finish()
	if e:
		env = e

#
# fth-config.h
#
def fth_conf_test(env):
	fth_conf_h = env.Configure(config_h = build_dir + '/fth-config.h',
		clean = False, help = False)
	fth_conf_h.Define('FTH_CONFIGURE_ARGS', '"scons"')
	fth_conf_h.Define('FTH_LIBS', '"' + env.subst("${_LIBFLAGS}") + '"')
	fth_conf_h.Define('FTH_PACKAGE_NAME', '"' + env['NAME'].upper() + '"')
	fth_conf_h.Define('FTH_PACKAGE_TARNAME', '"' + env['NAME'] + '"')
	fth_conf_h.Define('FTH_PACKAGE_VERSION', '"' + env['VERSION'] + '"')
	fth_conf_h.Define('FTH_SIZEOF_LONG', env['sizeof_long'])
	if env['sizeof_long_long']:
		fth_conf_h.Define('FTH_SIZEOF_LONG_LONG',
			env['sizeof_long_long'])
		fth_conf_h.Define('HAVE_LONG_LONG', 1)
	fth_conf_h.Define('FTH_ALIGNOF_VOID_P', env['sizeof_void_p'])
	fth_conf_h.Define('FTH_SIZEOF_VOID_P', env['sizeof_void_p'])
	if not shared:
		fth_conf_h.Define('FTH_STATIC', 1)
	fth_conf_h.Define('FTH_TARGET', '"' + env['HOST_TRIPLE'] + '"')
	fth_conf_h.Define('FTH_TARGET_CPU', '"' + arch + '"')
	fth_conf_h.Define('FTH_TARGET_OS', '"' + sys.platform + '"')
	fth_conf_h.Define('FTH_TARGET_VENDOR', '"' + vendor + '"')
	if not env['mode_t']:
		fth_conf_h.Define('mode_t', 'int', sc_h['mode_t'])
	if not env['size_t']:
		fth_conf_h.Define('size_t', 'unsigned int', sc_h['size_t'])
	if not env['off_t']:
		fth_conf_h.Define('off_t', 'long int', sc_h['off_t'])
	if not env['pid_t']:
		fth_conf_h.Define('pid_t', 'int', sc_h['pid_t'])
	if not env['uid_t']:
		fth_conf_h.Define('uid_t', 'int', sc_h['uid_t'])
	for fh in ['complex.h',
		'errno.h',
		'float.h',
		'limits.h',
		'memory.h',
		'missing/complex.h',
		'missing/math.h',
		'openssl/bn.h',
		'setjmp.h',
		'stdarg.h',
		'strings.h',
		'string.h',
		'unistd.h']:
		fth_conf_h.CheckHeader(fh)
	if env['complex_i']:
		fth_conf_h.Define('HAVE_COMPLEX_I', 1)
		fth_conf_h.Define('HAVE_1_0_FI', 1)
	for cf in ['cabs',
		'cabs2',
		'cacos',
		'cacosh',
		'carg',
		'casin',
		'casinh',
		'catan',
		'catan2',
		'catanh',
		'ccos',
		'ccosh',
		'cexp',
		'clog',
		'clog10',
		'conj',
		'cpow',
		'csin',
		'csinh',
		'csqrt',
		'ctan',
		'ctanh',
		'strncasecmp']:
		fth_conf_h.CheckFunc(cf)
	e = fth_conf_h.Finish()
	if e:
		env = e

conf_test(env)
src_conf_test(env)
fth_conf_test(env)

Export('env')
SConscript('SConscript', variant_dir = build_dir)

print("%20s: %s" % ("host", env.subst("${HOST_TRIPLE}")))
print("%20s: %s v%s" % ("compiler",
	env.subst("${CC}"), env.subst("${CCVERSION}")))
if shared:
	print("%20s: %s" % ("cc command line", env.subst("${SHCCCOM}")))
	print("%20s: %s" % ("ld command line", env.subst("${SHLINKCOM}")))
else:
	print("%20s: %s" % ("cc command line", env.subst("${CCCOM}")))
	print("%20s: %s" % ("ld command line", env.subst("${LINKCOM}")))

print("%20s: %s" % ("prefix", prefix))
print("%20s: %s" % ("tecla-prefix", tecla_prefix))
print("%20s: %s" % ("program-prefix", prg_prefix))
print("%20s: %s" % ("program-suffix", prg_suffix))
print("%20s: %s" % ("libtecla", libtecla))
print("%20s: %s" % ("shared", shared))
print("%20s: %s" % ("warnings", warnings))

# SConstruct ends here
