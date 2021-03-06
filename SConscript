# Copyright (c) 2016 Michael Scholz <mi-scholz@users.sourceforge.net>
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
# @(#)SConscript	1.2 3/25/16

import os

Import('env')

prefix		= env['prefix']
shared		= env['shared']
warnings	= env['warnings']
missing		= env['missing']
dbm		= env['dbm']
dbm_lib		= env['dbm_lib']
build_dir	= env['build']
top_srcdir	= env['top_srcdir']
prog_name	= env['NAME']
top_dir		= Dir('.').path
obj_dir		= top_dir + '/src'
so_lib		= 'src/libfth'
pg_fth		= 'src/fth'
sources		= []
sources.extend(Glob('ficl/*.c'))
sources.extend(missing)
sources.extend(Glob('src/*.c', exclude = ['src/fth.c', 'src/misc.c']))
env.MergeFlags(['-g', '-O3'])

if warnings:
	env.Append(CFLAGS = [
		'-ansi',
		'-std=iso9899:1999',	# otherwise va_copy() is hidden
		'-Wall',
		'-Wextra',
		'-Wmissing-declarations',
		'-Wmissing-prototypes',
		'-Wundef',
		'-Wunused'])
env['CPPDEFINES'] = ['HAVE_CONFIG_H']

misc_env = env.Clone(CPPDEFINES = ['HAVE_CONFIG_H',
	'FTH_PREFIX_PATH=\\"' + prefix + '\\"',
	'FTH_PROG_NAME=\\"' + prog_name + '\\"'])

if shared:
	ms = misc_env.SharedObject('src/misc.c')
	so = env.SharedLibrary(so_lib, [sources, ms])
	po = env.SharedObject('src/fth.c')
else:
	ms = misc_env.StaticObject('src/misc.c')
	so = env.StaticLibrary(so_lib, [sources, ms])
	po = env.StaticObject('src/fth.c')

if dbm:
	ext_env = env.Clone(LIBS = [dbm_lib, 'm', 'c'])
	od = ext_env.SharedObject('examples/dbm/fth-dbm.c')
	el = ext_env.SharedLibrary('fth-dbm', od)

#
# substitutions
#
conf_input	= "Generated by scons."
subst_dict	= {
		'@prefix@'		: prefix,
		'@prog_name@'		: prog_name,
		'@configure_input@'	: conf_input,
		'@abs_top_builddir@'	: top_dir,
		'@top_srcdir@'		: top_srcdir,
		'@abs_top_srcdir@'	: top_srcdir }
fth_sh		= top_srcdir + '/fth.sh'
fth_test	= top_srcdir + '/examples/scripts/fth-test.fth'
install		= top_srcdir + '/examples/scripts/install.fth'
play_sound	= top_srcdir + '/examples/scripts/play-sound.fth'
xm		= top_srcdir + '/examples/scripts/xm.fth'
fs		= []
se		= Environment(tools = ['textfile'])
for s in [fth_sh, fth_test, install, play_sound, xm]:
	t = se.Substfile(s + '.in', SUBST_DICT = subst_dict)
	fs.extend(Command(None, s, Chmod(s, 0755)))

#
# install
# uninstall
#
inst		= []
bindir		= prefix + '/bin'
libdir		= prefix + '/lib'
pkglibdir	= libdir + '/' + prog_name
incdir		= prefix + '/include/' + prog_name
datadir		= prefix + '/share/' + prog_name
flibdir		= datadir + '/fth-lib'
fsitedir	= datadir + '/site-fth'
mandir		= prefix + '/man'
man1dir		= mandir + '/man1'
man3dir		= mandir + '/man3'
cat1dir		= mandir + '/cat1'
cat3dir		= mandir + '/cat3'

iso = env.InstallAs(libdir + '/lib' + prog_name + '.so', so)
env.NoClean(iso)

pg = env.Clone(LIBS = [prog_name, 'm', 'c']).Program(pg_fth, [po])
inst.extend(env.InstallAs(bindir + '/' + prog_name, pg))
env.Depends(pg, iso)
inst.extend(iso)

if dbm:
	inst.extend(env.InstallAs(pkglibdir + '/dbm.so', el))

inst.extend(env.Install(incdir, [
	'ficl/ficl.h',
	'ficl/ficllocal.h',
	'ficl/ficltokens.h',
	'fth-config.h',
	'src/fth-lib.h',
	'src/fth.h']))
inst.extend(env.Install(flibdir, Glob('examples/fth-lib/*.f[rs]')))
inst.extend(env.Install(fsitedir, Glob('examples/site-lib/*.fs*')))
inst.extend(env.Install(man1dir, 'fth.1'))
inst.extend(env.Install(man3dir, 'libfth.3'))
if os.path.exists(cat1dir):
	inst.extend(env.Install(cat1dir, 'fth.0'))
if os.path.exists(cat3dir):
	inst.extend(env.Install(cat3dir, 'libfth.0'))
env.Alias('install', inst)
env.Alias('uninstall', env.Command(None, 'README.man', Delete(Flatten(inst))))
env.Default(pg, fs)

#
# test
# clean
#
env.Alias('test', env.Command('test', [pg, fs], fth_sh + ' -Ds ' + fth_test))
# scons -c removes these extra files
Clean('clean', fs)

# SConscript ends here
