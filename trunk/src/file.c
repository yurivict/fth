/*-
 * Copyright (c) 2007-2012 Michael Scholz <mi-scholz@users.sourceforge.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @(#)file.c	1.85 9/13/13
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "fth.h"
#include "utils.h"

#if defined(HAVE_SYS_STAT_H)
#include <sys/stat.h>
#endif
#if defined(HAVE_NETINET_IN_H)
#include <netinet/in.h>
#endif
#if defined(HAVE_ARPA_INET_H)
#include <arpa/inet.h>
#endif

static void	ficl_close_pipe(ficlVm *vm);
static void	ficl_file_atime(ficlVm *vm);
static void	ficl_file_basename(ficlVm *vm);
static void	ficl_file_chdir(ficlVm *vm);
static void	ficl_file_chmod(ficlVm *vm);
static void	ficl_file_chroot(ficlVm *vm);
static void	ficl_file_copy(ficlVm *vm);
static void	ficl_file_ctime(ficlVm *vm);
static void	ficl_file_delete(ficlVm *vm);
static void	ficl_file_dir(ficlVm *vm);
static void	ficl_file_dirname(ficlVm *vm);
static void	ficl_file_eval(ficlVm *vm);
static void	ficl_file_exists_p(ficlVm *vm);
static void	ficl_file_fullpath(ficlVm *vm);
static void	ficl_file_install(ficlVm *vm);
static void	ficl_file_length(ficlVm *vm);
static void	ficl_file_mkdir(ficlVm *vm);
static void	ficl_file_mkfifo(ficlVm *vm);
static void	ficl_file_mtime(ficlVm *vm);
static void	ficl_file_pwd(ficlVm *vm);
static void	ficl_file_realpath(ficlVm *vm);
static void	ficl_file_rename(ficlVm *vm);
static void	ficl_file_rmdir(ficlVm *vm);
static void	ficl_file_shell(ficlVm *vm);
static void	ficl_file_split(ficlVm *vm);
static void	ficl_file_symlink(ficlVm *vm);
static void	ficl_file_system(ficlVm *vm);
static void	ficl_file_touch(ficlVm *vm);
static void	ficl_file_truncate(ficlVm *vm);
static void	ficl_file_zero_p(ficlVm *vm);
static void	ficl_open_pipe(ficlVm *vm);
static mode_t	fth_stat(const char *name, struct stat *buf);

/* === FILE === */

#define h_list_of_file_functions "\
*** FILE PRIMITIVES ***\n\
chdir alias for file-chdir\n\
close-pipe          ( fp -- ior )\n\
file-atime          ( name -- time )\n\
file-basename       ( name ext -- base )\n\
file-chdir          ( path -- )\n\
file-chmod          ( name mode -- )\n\
file-chroot         ( path -- )\n\
file-copy           ( src dst -- )\n\
file-ctime          ( name -- time )\n\
file-delete         ( name -- )\n\
file-dir            ( dir -- files-ary )\n\
file-dirname        ( name -- path )\n\
file-eval           ( name -- )\n\
file-fullpath       ( name -- path )\n\
file-install        ( src dst mode -- f )\n\
file-length         ( name -- len )\n\
file-match-dir      ( dir reg -- files-ary )\n\
file-mkdir          ( name mode -- )\n\
file-mkfifo         ( name mode -- )\n\
file-mtime          ( name -- time )\n\
file-pwd            ( -- path )\n\
file-realpath       ( name -- path )\n\
file-rename         ( src dst -- )\n\
file-rmdir          ( name -- )\n\
file-shell          ( cmd -- str )\n\
file-split          ( name -- ary )\n\
file-symlink        ( src dst -- )\n\
file-system         ( cmd -- f )\n\
file-touch          ( name time -- )\n\
file-truncate       ( name size -- )\n\
open-pipe           ( addr u fam -- fp ior )\n\
shell alias for file-shell\n\
File test:\n\
file-block?         ( name -- f )\n\
file-character?     ( name -- f )\n\
file-directory?     ( name -- f )\n\
file-executable?    ( name -- f )\n\
file-exists?        ( name -- f )\n\
file-fifo?          ( name -- f )\n\
file-grpowned?      ( name -- f )\n\
file-owned?         ( name -- f )\n\
file-readable?      ( name -- f )\n\
file-setgid?        ( name -- f )\n\
file-setuid?        ( name -- f )\n\
file-socket?        ( name -- f )\n\
file-sticky?        ( name -- f )\n\
file-symlink?       ( name -- f )\n\
file-writable?      ( name -- f )\n\
file-zero?          ( name -- f )"

#if defined(HAVE_ACCESS)
#define FTH_ACCESS_P(name, mode)	(access(name, mode) == 0)
#else
#define FTH_ACCESS_P(name, mode)	(false)
#endif

#define h_mode_info "\
You can write MODE with octal numbers: prepend the number with 0o, \
this is number zero '0' and lower letter 'o', \
similar to 0x for hexadecimal numbers."

#define h_system_error_info(Fnc) \
"Raise SYSTEM-ERROR exception if " Fnc " fails."
#define h_system_error_and_not_implemented_info(Fnc) \
"Raise SYSTEM-ERROR exception if " Fnc " fails, \
raise NOT-IMPLEMENTED exception if " Fnc " is not available."

void
fth_file_delete(const char *name)
{
#if !defined(_WIN32)
	if (fth_file_writable_p(name))
#endif
		if (unlink(name) == -1)
			FTH_SYSTEM_ERROR_ARG_THROW(unlink, name);
}

static void
ficl_file_delete(ficlVm *vm)
{
#define h_file_delete "( name -- )  delete file\n\
\"main.c\" file-delete\n\
If file NAME exist, delete it, otherwise do nothing (see unlink(2)).  \
"  h_system_error_info("unlink(2)")
	FTH fs;

	FTH_STACK_CHECK(vm, 1, 0);
	fs = ficlStackPopFTH(vm->dataStack);
	if (fth_string_length(fs) <= 0) {
		FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
		/* NOTREACHED */
		return;
	}
	fth_file_delete(fth_string_ref(fs));
}

void
fth_file_chmod(const char *name, mode_t mode)
{
#if defined(HAVE_CHMOD)
	if (chmod(name, mode) == -1)
		FTH_SYSTEM_ERROR_ARG_THROW(chmod, name);
#else
	FTH_NOT_IMPLEMENTED_ERROR(chmod);
#endif
}

static void
ficl_file_chmod(ficlVm *vm)
{
#define h_file_chmod "( name mode -- )  change file access mode\n\
\"main.csh\" 0o755 file-chmod\n\
Change access mode of file NAME to MODE (see chmod(2)).  \
" h_mode_info "  " h_system_error_and_not_implemented_info("chmod(2)")
	FTH fs;
	mode_t mode;

	FTH_STACK_CHECK(vm, 2, 0);
	mode = (mode_t)ficlStackPopInteger(vm->dataStack);
	fs = ficlStackPopFTH(vm->dataStack);
	if (fth_string_length(fs) <= 0) {
		FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
		/* NOTREACHED */
		return;
	}
	fth_file_chmod(fth_string_ref(fs), mode);
}

void
fth_file_mkdir(const char *name, mode_t mode)
{
#if defined(HAVE_MKDIR)
#if defined(_WIN32)
	if (mkdir(name) == -1)
#else
	if (mkdir(name, mode) == -1)
#endif
		FTH_SYSTEM_ERROR_ARG_THROW(mkdir, name);
#else
	FTH_NOT_IMPLEMENTED_ERROR(mkdir);
#endif
}

static void
ficl_file_mkdir(ficlVm *vm)
{
#define h_file_mkdir "( name mode -- )  create new directory\n\
\"test-src\" 0o755 file-mkdir\n\
Create directory named NAME with access mode MODE (see mkdir(2)).  \
" h_mode_info "  " h_system_error_and_not_implemented_info("mkdir(2)")
	FTH fs;
	mode_t mode;

	FTH_STACK_CHECK(vm, 2, 0);
	mode = (mode_t)ficlStackPopInteger(vm->dataStack);
	fs = ficlStackPopFTH(vm->dataStack);
	if (fth_string_length(fs) <= 0) {
		FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
		/* NOTREACHED */
		return;
	}
	fth_file_mkdir(fth_string_ref(fs), mode);
}

void
fth_file_rmdir(const char *name)
{
#if defined(HAVE_RMDIR)
	if (rmdir(name) == -1)
		FTH_SYSTEM_ERROR_ARG_THROW(rmdir, name);
#else
	FTH_NOT_IMPLEMENTED_ERROR(rmdir);
#endif
}

static void
ficl_file_rmdir(ficlVm *vm)
{
#define h_file_rmdir "( name -- )  remove empty directory\n\
\"test-src\" file-rmdir\n\
Remove empty directory NAME (see rmdir(2)).  \
" h_system_error_and_not_implemented_info("rmdir(2)")
	FTH fs;

	FTH_STACK_CHECK(vm, 1, 0);
	fs = ficlStackPopFTH(vm->dataStack);
	if (fth_string_length(fs) <= 0) {
		FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
		/* NOTREACHED */
		return;
	}
	fth_file_rmdir(fth_string_ref(fs));
}

void
fth_file_mkfifo(const char *name, mode_t mode)
{
#if defined(HAVE_MKFIFO)
	if (mkfifo(name, mode) == -1)
		FTH_SYSTEM_ERROR_ARG_THROW(mkfifo, name);
#else
	FTH_NOT_IMPLEMENTED_ERROR(mkfifo);
#endif
}

static void
ficl_file_mkfifo(ficlVm *vm)
{
#define h_file_mkfifo "( name mode -- )  create fifo\n\
\"test-fifo\" 0o644 file-mkfifo\n\
Create fifo named NAME with access mode MODE (see mkfifo(2)).  \
" h_mode_info "  " h_system_error_and_not_implemented_info("mkfifo(2)")
	FTH fs;
	mode_t mode;

	FTH_STACK_CHECK(vm, 2, 0);
	mode = (mode_t)ficlStackPopInteger(vm->dataStack);
	fs = ficlStackPopFTH(vm->dataStack);
	if (fth_string_length(fs) <= 0) {
		FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
		/* NOTREACHED */
		return;
	}
	fth_file_mkfifo(fth_string_ref(fs), mode);
}

static char	file_scratch[MAXPATHLEN];

void
fth_file_symlink(const char *src, const char *dst)
{
#if defined(HAVE_SYMLINK)
	if (symlink(src, dst) == -1) {
		char *buf;
		size_t size;

		buf = file_scratch;
		size = sizeof(file_scratch);
		fth_strcpy(buf, size, src);
		fth_strcat(buf, size, " --> ");
		fth_strcat(buf, size, dst);
		FTH_SYSTEM_ERROR_ARG_THROW(symlink, buf);
	}
#else
	 FTH_NOT_IMPLEMENTED_ERROR(symlink);
#endif
}

static void
ficl_file_symlink(ficlVm *vm)
{
#define h_file_symlink "( src dst -- )  create symlink\n\
\"/usr/local/bin/gm4\" \"/home/mike/bin/m4\" file-symlink\n\
Create symlink from SRC named DST (see symlink(2)).  \
" h_system_error_and_not_implemented_info("symlink(2)")
	FTH s, d;

	FTH_STACK_CHECK(vm, 2, 0);
	d = ficlStackPopFTH(vm->dataStack);
	s = ficlStackPopFTH(vm->dataStack);
	if (fth_string_length(s) <= 0) {
		FTH_ASSERT_ARGS(FTH_STRING_P(s), s, FTH_ARG1, "a string");
		/* NOTREACHED */
		return;
	}
	if (fth_string_length(d) <= 0) {
		FTH_ASSERT_ARGS(FTH_STRING_P(d), d, FTH_ARG2, "a string");
		/* NOTREACHED */
		return;
	}
	fth_file_symlink(fth_string_ref(s), fth_string_ref(d));
}

void
fth_file_rename(const char *src, const char *dst)
{
#if defined(HAVE_RENAME)
	if (rename(src, dst) == -1) {
		char *buf;
		size_t size;

		buf = file_scratch;
		size = sizeof(file_scratch);
		fth_strcpy(buf, size, src);
		fth_strcat(buf, size, " --> ");
		fth_strcat(buf, size, dst);
		FTH_SYSTEM_ERROR_ARG_THROW(rename, buf);
	}
#else
	fth_copy_file(src, dst);
	fth_file_delete(src);
#endif
}

static void
ficl_file_rename(ficlVm *vm)
{
#define h_file_rename "( src dst -- )  rename file\n\
\"fth\" \"test-fth\" file-rename\n\
Rename SRC to DST (see rename(2)).  \
" h_system_error_info("rename(2)")
	FTH s, d;

	FTH_STACK_CHECK(vm, 2, 0);
	d = ficlStackPopFTH(vm->dataStack);
	s = ficlStackPopFTH(vm->dataStack);
	if (fth_string_length(s) <= 0) {
		FTH_ASSERT_ARGS(FTH_STRING_P(s), s, FTH_ARG1, "a string");
		/* NOTREACHED */
		return;
	}
	if (fth_string_length(d) <= 0) {
		FTH_ASSERT_ARGS(FTH_STRING_P(d), d, FTH_ARG2, "a string");
		/* NOTREACHED */
		return;
	}
	fth_file_rename(fth_string_ref(s), fth_string_ref(d));
}

void
fth_file_copy(const char *src, const char *dst)
{
	FILE *fpsrc, *fpdst;
	int c;

	fpsrc = fopen(src, "r");
	if (fpsrc == NULL) {
		FTH_SYSTEM_ERROR_ARG_THROW(fopen, src);
		/* NOTREACHED */
		return;
	}
	if (fth_file_directory_p(dst)) {
		char *buf;
		size_t size;

		buf = file_scratch;
		size = sizeof(file_scratch);
		fth_strcpy(buf, size, dst);
		fth_strcat(buf, size, "/");
		fth_strcat(buf, size, src);
		fpdst = fopen(buf, "w");
	} else
		fpdst = fopen(dst, "w");
	if (fpdst == NULL) {
		FTH_SYSTEM_ERROR_ARG_THROW(fopen, dst);
		/* NOTREACHED */
		return;
	}
	while ((c = fgetc(fpsrc)) != EOF)
		fputc(c, fpdst);
	fclose(fpsrc);
	fclose(fpdst);
}

static void
ficl_file_copy(ficlVm *vm)
{
#define h_file_copy "( src dst -- )  copy file\n\
\"fth\" \"test-fth\" file-copy\n\
Copy file SRC to DST.  \
If DST is a directory, copy SRC to DST/SRC.  \
Raise SYSTEM-ERROR exception if fopen(3) fails on any of the two files."
	FTH s, d;

	FTH_STACK_CHECK(vm, 2, 0);
	d = ficlStackPopFTH(vm->dataStack);
	s = ficlStackPopFTH(vm->dataStack);
	if (fth_string_length(s) <= 0) {
		FTH_ASSERT_ARGS(FTH_STRING_P(s), s, FTH_ARG1, "a string");
		/* NOTREACHED */
		return;
	}
	if (fth_string_length(d) <= 0) {
		FTH_ASSERT_ARGS(FTH_STRING_P(d), d, FTH_ARG2, "a string");
		/* NOTREACHED */
		return;
	}
	fth_file_copy(fth_string_ref(s), fth_string_ref(d));
}

bool
fth_file_install(const char *src, const char *dst, mode_t mode)
{
	struct stat st_src, st_dst;
	char *buf;
	size_t size;

	if (src == NULL || dst == NULL)
		return (false);
	buf = file_scratch;
	size = sizeof(file_scratch);
	fth_strcpy(buf, size, dst);
	if (fth_file_directory_p(dst)) {
		fth_strcat(buf, size, "/");
		fth_strcat(buf, size, src);
	}
	if (!fth_file_exists_p(buf) ||
	    (fth_stat(src, &st_src) != 0 &&
	     fth_stat(buf, &st_dst) != 0 &&
	     st_src.st_mtime > st_dst.st_mtime)) {
		fth_file_copy(src, buf);
		fth_file_chmod(buf, mode);
		return (true);
	}
	return (false);
}

static void
ficl_file_install(ficlVm *vm)
{
#define h_file_install "( src dst mode -- f )  install file\n\
: install-lib { src dst mode -- }\n\
  src dst mode file-install if\n\
    \"%s --> %04o %s\" '( src mode dst )\n\
  else\n\
    \"%s is up-to-date\" '( dst )\n\
  then  fth-print  cr\n\
;\n\
\"libsndlib.so\" \"/usr/opt/lib/s7\" 0o755 install-lib\n\
Install SRC to DST with access mode MODE \
if DST doesn't exist or if modification time of SRC is greater than DST's.  \
If DST is a directory, install SRC to DST/SRC.  \
Return #t if SRC could be installed, otherwise #f.  " h_mode_info
	FTH s, d;
	mode_t mode;
	bool flag;

	FTH_STACK_CHECK(vm, 3, 1);
	mode = (mode_t)ficlStackPopInteger(vm->dataStack);
	d = ficlStackPopFTH(vm->dataStack);
	s = ficlStackPopFTH(vm->dataStack);
	if (fth_string_length(s) <= 0) {
		FTH_ASSERT_ARGS(FTH_STRING_P(s), s, FTH_ARG1, "a string");
		/* NOTREACHED */
		return;
	}
	if (fth_string_length(d) <= 0) {
		FTH_ASSERT_ARGS(FTH_STRING_P(d), d, FTH_ARG2, "a string");
		/* NOTREACHED */
		return;
	}
	flag = fth_file_install(fth_string_ref(s), fth_string_ref(d), mode);
	ficlStackPushBoolean(vm->dataStack, flag);
}

FTH
fth_file_split(const char *name)
{
	FTH dir, file;
	char *idx;

	if (name == NULL) {
		dir = fth_make_empty_string();
		file = fth_make_empty_string();
		return (FTH_LIST_2(dir, file));
	}
	idx = strrchr(name, '/');
	if (idx == NULL) {
		dir = fth_make_empty_string();
		file = fth_make_string(name);
		return (FTH_LIST_2(dir, file));
	}
	dir = fth_make_string_len(name, idx - name);
	file = fth_make_string(idx + 1);
	return (FTH_LIST_2(dir, file));
}

static void
ficl_file_split(ficlVm *vm)
{
#define h_file_split "( name -- #( path file ) )  split to dir/basename\n\
\"/home/mike/cage.snd\" file-split => #( \"/home/mike\" \"cage.snd\" )\n\
Split file NAME in dirname and basename and return result in array."
	FTH fs, res;

	FTH_STACK_CHECK(vm, 1, 1);
	fs = ficlStackPopFTH(vm->dataStack);
	if (fth_string_length(fs) <= 0) {
		FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
		/* NOTREACHED */
		return;
	}
	res = fth_file_split(fth_string_ref(fs));
	ficlStackPushFTH(vm->dataStack, res);
}

FTH
fth_file_basename(const char *name, const char *ext)
{
	char *base;
	size_t len;

	if (name == NULL)
		return (fth_make_empty_string());
	base = strrchr(name, '/');
	if (base == NULL)
		base = (char *)name;
	else
		base++;
	if (ext != NULL)
		len = (size_t)(strstr(base, ext) - base);
	else
		len = (size_t)(strchr(base, '.') - base);
	if (len < strlen(base))
		return (fth_make_string_len(base, (ficlInteger)len));
	return (fth_make_string(base));
}

static void
ficl_file_basename(ficlVm *vm)
{
#define h_file_basename "( name ext -- basename )  return basename\n\
\"/home/mike/cage.snd\" #f   file-basename => \"cage.snd\"\n\
\"/home/mike/cage.snd\" nil  file-basename => \"cage\"\n\
\"/home/mike/cage.snd\" \"nd\" file-basename => \"cage.s\"\n\
\"/home/mike/cage.snd\" /\\.(snd|wave)$/ file-basename => \"cage\"\n\
Return basename of file NAME depending on EXT.  \
EXT may be #f, nil/undef, a string or a regexp.  \
If EXT is #f, return file name without path name.  \
If EXT is NIL or UNDEF, discard the part \
from the last dot to the end of basename NAME.  \
If EXT is a string or a regexp, discard found EXT from basename NAME."
	FTH ext, fs, result;
	char *name;

	FTH_STACK_CHECK(vm, 2, 1);
	ext = ficlStackPopFTH(vm->dataStack);
	fs = ficlStackPopFTH(vm->dataStack);
	if (fth_string_length(fs) <= 0) {
		FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
		/* NOTREACHED */
		return;
	}
	name = fth_string_ref(fs);
	if (FTH_FALSE_P(ext))
		result = fth_make_string(fth_basename(name));
	else if (FTH_NIL_P(ext) || FTH_UNDEF_P(ext))
		result = fth_file_basename(name, NULL);
	else if (fth_string_length(ext) > 0)
		result = fth_file_basename(name, fth_string_ref(ext));
	else if (FTH_REGEXP_P(ext)) {
		char *base;
		ficlInteger len;
		FTH fbase;

		base = strrchr(name, '/');
		if (base == NULL)
			base = (char *)name;
		else
			base++;
		fbase = fth_make_string(base);
		len = fth_regexp_search(ext, fbase, 0L, -1L);
		if (len > 0)
			result = fth_make_string_len(base, len);
		else
			result = fbase;
	} else {
		FTH_ASSERT_ARGS(false, ext, FTH_ARG2,
		    "a string, regexp, #f or nil");
		/* NOTREACHED */
		return;
	}
	ficlStackPushFTH(vm->dataStack, result);
}

FTH
fth_file_dirname(const char *name)
{
	char *base;

	if (name == NULL)
		return (fth_make_empty_string());
	base = strrchr(name, '/');
	if (base != NULL)
		return (fth_make_string_len(name, base - name));
	return (fth_make_string("./"));
}

static void
ficl_file_dirname(ficlVm *vm)
{
#define h_file_dirname "( name -- dirname )  return dirname\n\
\"/home/mike/cage.snd\" file-dirname => \"/home/mike\"\n\
Return directory part of file NAME."
	FTH fs, res;

	FTH_STACK_CHECK(vm, 1, 1);
	fs = ficlStackPopFTH(vm->dataStack);
	res = fth_file_dirname(fth_string_ref(fs));
	ficlStackPushFTH(vm->dataStack, res);
}

static void
ficl_file_fullpath(ficlVm *vm)
{
#define h_file_fullpath "( name -- fullpath )  return full path\n\
\"cage.snd\" file-fullpath => \"/home/mike/cage.snd\"\n\
Return current working directory prepended to file NAME.  \
If name starts with a slash, return NAME unchanged.  \
"  h_system_error_info("getcwd(3)")
	FTH fs, res;
	char *name;

	FTH_STACK_CHECK(vm, 1, 1);
	fs = ficlStackPopFTH(vm->dataStack);
	if (fth_string_length(fs) <= 0) {
		FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
		/* NOTREACHED */
		return;
	}
	name = fth_string_ref(fs);
	res = fs;
	if (*name != '/') {
		char *path;

		path = file_scratch;
		if (getcwd(path, sizeof(file_scratch)) == NULL) {
			FTH_SYSTEM_ERROR_ARG_THROW(getcwd, path);
			/* NOTREACHED */
			return;
		}
		res = fth_make_string_format("%s/%s", path, name);
	}
	ficlStackPushFTH(vm->dataStack, res);
}

static char	file_scratch_02[MAXPATHLEN];

FTH
fth_file_realpath(const char *name)
{
	char *resolved, *path;
	size_t size;

	if (name == NULL)
		return (fth_make_empty_string());
	/* replace '~' at the beginning with $HOME */
	path = file_scratch;
	size = sizeof(file_scratch);
	if (strlen(name) > 0 && *name == '~') {
		fth_strcpy(path, size, fth_getenv("HOME", "/tmp"));
		fth_strcat(path, size, name + 1);
	} else
		fth_strcpy(path, size, name);
	resolved = file_scratch_02;
#if defined(HAVE_REALPATH)
	if (realpath(path, resolved) == NULL)
		FTH_SYSTEM_ERROR_ARG_THROW(realpath, resolved);
#else
	fth_strcpy(resolved, size, path);
#endif
	return (fth_make_string(resolved));
}

static void
ficl_file_realpath(ficlVm *vm)
{
#define h_file_realpath "( path -- resolved-path )  return resolved path\n\
\"~\" file-realpath       => \"/home/mike\"\n\
\"/usr/local\" file-chdir => \"/usr/local\"\n\
file-pwd                => \"/usr/local\"\n\
\"../bin\" file-realpath  => \"/usr/bin\"\n\
If PATH starts with '~', replace it with content \
of environment variable $HOME.  \
If realpath(3) function exists , return resolved path, \
otherwise return PATH with '~' replacement.  \
"  h_system_error_info("realpath(3)")
	FTH res, fs;

	FTH_STACK_CHECK(vm, 1, 1);
	fs = ficlStackPopFTH(vm->dataStack);
	res = fth_file_realpath(fth_string_ref(fs));
	ficlStackPushFTH(vm->dataStack, res);
}

static void
ficl_file_pwd(ficlVm *vm)
{
#define h_file_pwd "( -- path )  return working directory\n\
file-pwd => \"/home/mike/src\"\n\
Return current working directory (see getcwd(3)).  \
"  h_system_error_info("getcwd(3)")
	char *path;

	FTH_STACK_CHECK(vm, 0, 1);
	path = file_scratch;
	if (getcwd(path, sizeof(file_scratch)) == NULL) {
		FTH_SYSTEM_ERROR_ARG_THROW(getcwd, path);
		/* NOTREACHED */
		return;
	}
	push_cstring(vm, path);
}

static void
ficl_file_chdir(ficlVm *vm)
{
#define h_file_chdir "( path -- )  change working directory\n\
\"/usr/local\" file-chdir => prints \"/usr/local\"\n\
Change working directory to PATH and, \
if in a repl, print result to current standard output.  \
If PATH is NIL, change working directory to $HOME.  \
PATH may contain `~' as an abbreviation for home directory (see chdir(2)).  \
" h_system_error_and_not_implemented_info("chdir(2)")
	char *path;
	FTH dir;

	FTH_STACK_CHECK(vm, 1, 0);
	dir = fth_pop_ficl_cell(vm);
#if defined(HAVE_CHDIR)
	if (FTH_NIL_P(dir))
		path = fth_getenv("HOME", "/tmp");
	else
		path = fth_string_ref(fth_file_realpath(fth_string_ref(dir)));
	if (chdir(path) == -1)
		FTH_SYSTEM_ERROR_ARG_THROW(chdir, path);
	if (CELL_INT_REF(&FTH_FICL_VM()->sourceId) == -1)
		fth_print(path);
#else
	FTH_NOT_IMPLEMENTED_ERROR(chdir);
#endif
}

static void
ficl_file_truncate(ficlVm *vm)
{
#define h_file_truncate "( name size -- )  truncate file\n\
\"big-test.file\" 1024 file-truncate\n\
Truncate or extend file NAME to SIZE bytes (see truncate(2)).  \
" h_system_error_and_not_implemented_info("truncate(2)")
	char *path;
	ficl2Unsigned size;

	FTH_STACK_CHECK(vm, 2, 0);
	size = ficlStackPop2Unsigned(vm->dataStack);
	path = pop_cstring(vm);
#if defined(HAVE_TRUNCATE)
	if (path != NULL)
		if (truncate(path, (off_t)size) == -1)
			FTH_SYSTEM_ERROR_ARG_THROW(truncate, path);
#else
	FTH_NOT_IMPLEMENTED_ERROR(truncate);
#endif
}

static void
ficl_file_chroot(ficlVm *vm)
{
#define h_file_chroot "( path -- )  change root directory\n\
\"/usr/local/var/ftp\" file-chroot\n\
Change root directory to PATH and, \
if in a repl, print result to current standard output.  \
This function is restricted to the super-user (see chroot(2)).  \
" h_system_error_and_not_implemented_info("chroot(2)")
	char *path;

	FTH_STACK_CHECK(vm, 1, 0);
	path = pop_cstring(vm);
#if defined(HAVE_CHROOT)
	if (path != NULL) {
		if (chroot(path) == -1)
			FTH_SYSTEM_ERROR_ARG_THROW(chroot, path);
		if (CELL_INT_REF(&vm->sourceId) == -1)
			fth_print(path);
	}
#else
	FTH_NOT_IMPLEMENTED_ERROR(chroot);
#endif
}

static void
ficl_file_eval(ficlVm *vm)
{
#define h_file_eval "( name -- )  load and eval file\n\
\"test.fs\" file-eval\n\
Load and eval content of file NAME \
and add NAME to *loaded-files* if it wasn't there.  \
It's similar to INCLUDE except that file name \
must be on stack (INCLUDE is a parseword).  \
With file-eval one can load files from within word definitions.  \
Raise LOAD-ERROR exception if file-eval fails.\n\
See also include and require."
	FTH fs;

	FTH_STACK_CHECK(vm, 1, 0);
	fs = ficlStackPopFTH(vm->dataStack);
	if (fth_string_length(fs) <= 0) {
		FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
		/* NOTREACHED */
		return;
	}
	fth_load_file(fth_string_ref(fs));
}

static void
ficl_file_shell(ficlVm *vm)
{
#define h_file_shell "( cmd -- str )  execute command\n\
\"pwd\" file-shell string-chomp => \"/home/mike\"\n\
Open pipe for reading, feed it with CMD \
and collect string output as long as pipe is open.  \
Afterwards close pipe, set read-only variable EXIT-STATUS \
and return collected string (with trailing CR).\n\
See also file-system and exit-status."
	FTH cmd, io, fs;
	char *line;

	FTH_STACK_CHECK(vm, 1, 1);
	cmd = fth_pop_ficl_cell(vm);
	io = fth_io_popen(cmd, FICL_FAM_READ);
	fs = fth_make_empty_string();
	while ((line = fth_io_read(io)) != NULL)
		fth_string_scat(fs, line);
	fth_io_close(io);
	ficlStackPushFTH(vm->dataStack, fs);
}

static void
ficl_file_system(ficlVm *vm)
{
#define h_file_system "( cmd -- f )  execute command\n\
\"pwd\" file-system => #t\n\
Execute CMD as a shell command.  \
Set read-only variable EXIT-STATUS and return #t for success, #f otherwise.  \
In the latter case you can check EXIT-STATUS.\n\
See also file-shell and exit-status."
	FTH fs;
	bool flag;

	FTH_STACK_CHECK(vm, 1, 1);
	fs = ficlStackPopFTH(vm->dataStack);
	if (fth_string_length(fs) <= 0) {
		FTH_ASSERT_ARGS(FTH_STRING_P(fs), fs, FTH_ARG1, "a string");
		/* NOTREACHED */
		return;
	}
	flag = fth_set_exit_status(system(fth_string_ref(fs))) == 0;
	ficlStackPushBoolean(vm->dataStack, flag);
}

/* --- Forth-like file functions --- */

static void
ficl_open_pipe(ficlVm *vm)
{
#define h_open_pipe "( addr u fam -- fp ior )  open pipe\n\
256 constant max-line\n\
create line-buffer max-line 2 + allot\n\
s\" pwd\" r/o open-pipe throw value FP\n\
line-buffer max-line FP read-line throw drop line-buffer swap type\n\
FP close-pipe throw\n\
Forth-like word.\n\
See also close-pipe, open-file, close-file."
	ficlInteger fam;
	size_t len;
	char *mode, *cmd, *str;
	FILE *fp;

	FTH_STACK_CHECK(vm, 3, 2);
	fam = ficlStackPopInteger(vm->dataStack);
	len = (size_t)ficlStackPopUnsigned(vm->dataStack);
	str = ficlStackPopPointer(vm->dataStack);
	if (len == 0) {
		ficlStackPushPointer(vm->dataStack, NULL);
		ficlStackPushInteger(vm->dataStack, (ficlInteger)EINVAL);
		return;
	}
	switch (FICL_FAM_OPEN_MODE(fam)) {
	case FICL_FAM_READ:
		mode = "r";
		break;
	case FICL_FAM_WRITE:
		mode = "w";
		break;
	default:
		ficlStackPushPointer(vm->dataStack, NULL);
		ficlStackPushInteger(vm->dataStack, (ficlInteger)EINVAL);
		return;
		break;
	}
	cmd = FTH_CALLOC(len + 1, sizeof(char));
	strncpy(cmd, str, len);
	fp = popen(cmd, mode);
	if (fp == NULL) {
		perror("popen");
		ficlStackPushPointer(vm->dataStack, NULL);
		ficlStackPushInteger(vm->dataStack, (ficlInteger)errno);
	} else {
		ficlFile *ff;

		ff = FTH_MALLOC(sizeof(ficlFile));
		ff->f = fp;
		ficlStackPushPointer(vm->dataStack, ff);
		ficlStackPushInteger(vm->dataStack, 0L);
	}
	FTH_FREE(cmd);
}

static void
ficl_close_pipe(ficlVm *vm)
{
#define h_close_pipe "( fp -- ior )  close pipe\n\
256 constant max-line\n\
create line-buffer max-line 2 + allot\n\
s\" pwd\" r/o open-pipe throw value FP\n\
line-buffer max-line FP read-line throw drop line-buffer swap type\n\
FP close-pipe throw\n\
Forth-like word.\n\
See also open-pipe, open-file, close-file."
	ficlFile *ff;
	int stat;

	FTH_STACK_CHECK(vm, 1, 1);
	ff = (ficlFile *) ficlStackPopPointer(vm->dataStack);
	stat = fth_set_exit_status(pclose(ff->f));
	FTH_FREE(ff);
	ficlStackPushInteger(vm->dataStack, (ficlInteger)stat);
}

/* === File Test Functions === */

#define MAKE_FILE_TEST_WORD(Name, Opt, Desc)				\
static void								\
ficl_file_ ## Name ## _p(ficlVm *vm)					\
{									\
	bool flag;							\
									\
	FTH_STACK_CHECK(vm, 1, 1);					\
	flag = fth_file_ ## Name ## _p(pop_cstring(vm));		\
	ficlStackPushBoolean(vm->dataStack, flag);			\
}									\
static char* h_file_ ## Name ## _p =					\
"( name -- f )  test if NAME " Desc "\n\
\"abc\" file-" #Name "? => #t|#f\n\
Return #t if NAME " Desc ", otherwise #f (see test(1) option -" #Opt ")."

static mode_t
fth_stat(const char *name, struct stat *buf)
{
	buf->st_mode = 0;
#if defined(HAVE_LSTAT)
	if (fth_strlen(name) > 0)
		lstat(name, buf);
#else
	FTH_NOT_IMPLEMENTED_ERROR(lstat);
#endif
	return (buf->st_mode);
}

bool
fth_file_block_p(const char *name)
{
	struct stat buf;

	return (S_ISBLK(fth_stat(name, &buf)));
}

MAKE_FILE_TEST_WORD(block, b, "is a block special file");

bool
fth_file_character_p(const char *name)
{
	struct stat buf;

	return (S_ISCHR(fth_stat(name, &buf)));
}

MAKE_FILE_TEST_WORD(character, c, "is a character special file");

bool
fth_file_directory_p(const char *name)
{
	struct stat buf;

	return (S_ISDIR(fth_stat(name, &buf)));
}

MAKE_FILE_TEST_WORD(directory, d, "is a directory");

bool
fth_file_exists_p(const char *name)
{
	bool flag;
	int old_errno;

	flag = false;
	old_errno = errno;
	if (name != NULL && *name != '\0' && FTH_ACCESS_P(name, F_OK))
		flag = true;
	errno = old_errno;
	return (flag);
}

static void
ficl_file_exists_p(ficlVm *vm)
{
#define h_file_exists_p "( name -- f )  test if file exists\n\
\"abc\" file-exists? => #t|#f\n\
Return #t if NAME is an existing file, otherwise #f."
	bool flag;

	FTH_STACK_CHECK(vm, 1, 1);
	flag = fth_file_exists_p(pop_cstring(vm));
	ficlStackPushBoolean(vm->dataStack, flag);
}

bool
fth_file_fifo_p(const char *name)
{
	struct stat buf;

	return (S_ISFIFO(fth_stat(name, &buf)));
}

MAKE_FILE_TEST_WORD(fifo, p, "is a named pipe");

bool
fth_file_symlink_p(const char *name)
{
#if defined(_WIN32)
	return (false);
#else
	struct stat buf;

	return (S_ISLNK(fth_stat(name, &buf)));
#endif
}

MAKE_FILE_TEST_WORD(symlink, L, "is a symbolic link");

bool
fth_file_socket_p(const char *name)
{
#if !defined(HAVE_SYS_SOCKET_H)
	return (false);
#else
	struct stat buf;

	return (S_ISSOCK(fth_stat(name, &buf)));
#endif
}

MAKE_FILE_TEST_WORD(socket, S, "is a socket");

bool
fth_file_executable_p(const char *name)
{
#if defined(HAVE_GETEUID) && defined(HAVE_GETEGID)
	struct stat buf;

	if (fth_stat(name, &buf)) {
#if defined(S_IXUSR)
		if (buf.st_uid == geteuid())
			return ((bool) (buf.st_mode & S_IXUSR));
#endif
#if defined(S_IXGRP)
		if (buf.st_gid == getegid())
			return ((bool) (buf.st_mode & S_IXGRP));
#endif
#if defined(S_IXOTH)
		return ((bool) (buf.st_mode & S_IXOTH));
#endif
	}
#endif				/* HAVE_GETEUID */
	return (false);
}

MAKE_FILE_TEST_WORD(executable, x, "is an executable file");

bool
fth_file_readable_p(const char *name)
{
#if defined(HAVE_GETEUID) && defined(HAVE_GETEGID)
	struct stat buf;

	if (fth_stat(name, &buf)) {
#if defined(S_IRUSR)
		if (buf.st_uid == geteuid())
			return ((bool) (buf.st_mode & S_IRUSR));
#endif
#if defined(S_IRGRP)
		if (buf.st_gid == getegid())
			return ((bool) (buf.st_mode & S_IRGRP));
#endif
#if defined(S_IROTH)
		return ((bool) (buf.st_mode & S_IROTH));
#endif
	}
#endif				/* HAVE_GETEUID */
	return (false);
}

MAKE_FILE_TEST_WORD(readable, r, "is a readable file");

bool
fth_file_writable_p(const char *name)
{
#if !defined(HAVE_GETEUID) || !defined(HAVE_GETEGID)
	return (false);
#else				/* !HAVE_GETEUID */
	struct stat buf;

	if (fth_stat(name, &buf)) {
#if defined(S_IWUSR)
		if (buf.st_uid == geteuid())
			return ((bool) (buf.st_mode & S_IWUSR));
#endif
#if defined(S_IWGRP)
		if (buf.st_gid == getegid())
			return ((bool) (buf.st_mode & S_IWGRP));
#endif
#if defined(S_IWOTH)
		return ((bool) (buf.st_mode & S_IWOTH));
#endif
	}
#endif				/* !HAVE_GETEUID */
	return (false);
}

MAKE_FILE_TEST_WORD(writable, w, "is a writable file");

bool
fth_file_owned_p(const char *name)
{
#if defined(HAVE_GETEUID)
	struct stat buf;

	if (fth_stat(name, &buf))
		return (buf.st_uid == geteuid());
#endif
	return (false);
}

MAKE_FILE_TEST_WORD(owned, O, "matches effective uid");

bool
fth_file_grpowned_p(const char *name)
{
#if defined(HAVE_GETEUID)
	struct stat buf;

	if (fth_stat(name, &buf))
		return (buf.st_gid == geteuid());
#endif
	return (false);
}

MAKE_FILE_TEST_WORD(grpowned, G, "matches effective gid");

bool
fth_file_setuid_p(const char *name)
{
#if defined(S_ISUID)
	struct stat buf;

	if (fth_stat(name, &buf))
		return ((bool) (buf.st_mode & S_ISUID));
#endif
	return (false);
}

MAKE_FILE_TEST_WORD(setuid, u, "has set uid bit");

bool
fth_file_setgid_p(const char *name)
{
#if defined(S_ISGID)
	struct stat buf;

	if (fth_stat(name, &buf))
		return ((bool) (buf.st_mode & S_ISGID));
#endif
	return (false);
}

MAKE_FILE_TEST_WORD(setgid, g, "has set gid bit");

bool
fth_file_sticky_p(const char *name)
{
#if defined(S_ISVTX)
	struct stat buf;

	if (fth_stat(name, &buf))
		return ((bool) (buf.st_mode & S_ISVTX));
#endif
	return (false);
}

MAKE_FILE_TEST_WORD(sticky, k, "has set sticky bit");

bool
fth_file_zero_p(const char *name)
{
	struct stat buf;

	if (fth_stat(name, &buf))
		return (buf.st_size == 0);
	return (false);
}

static void
ficl_file_zero_p(ficlVm *vm)
{
#define h_file_zero_p "( name -- f )  test if file length is zero\n\
\"abc\" file-zero?\n\
Return #t if file NAME length is zero, otherwise #f."
	bool flag;

	FTH_STACK_CHECK(vm, 1, 1);
	flag = fth_file_zero_p(pop_cstring(vm));
	ficlStackPushBoolean(vm->dataStack, flag);
}

FTH
fth_file_length(const char *name)
{
	struct stat buf;

	if (fth_stat(name, &buf))
		return (fth_make_long_long((ficl2Integer)buf.st_size));
	return (FTH_FALSE);
}

static void
ficl_file_length(ficlVm *vm)
{
#define h_file_length "( name -- len )  return file length\n\
\"abc\" file-length => 1024\n\
If NAME is a file, return its length in bytes, otherwise #f."
	FTH res;

	FTH_STACK_CHECK(vm, 1, 1);
	res = fth_file_length(pop_cstring(vm));
	fth_push_ficl_cell(vm, res);
}

FTH
fth_file_atime(const char *name)
{
	struct stat buf;

	if (fth_stat(name, &buf))
		return (fth_make_long_long((ficl2Integer)buf.st_atime));
	return (FTH_FALSE);
}

static void
ficl_file_atime(ficlVm *vm)
{
#define h_file_atime "( name -- time )  return file access\n\
\"abc\" file-atime time->string => \"Mon Aug 23 01:24:02 CEST 2010\"\n\
If NAME is a file, return its last access time, otherwise #f.  \
One can convert the number in a readable string with time->string.\n\
See also file-ctime, file-mtime and time->string."
	FTH res;

	FTH_STACK_CHECK(vm, 1, 1);
	res = fth_file_atime(pop_cstring(vm));
	fth_push_ficl_cell(vm, res);
}

FTH
fth_file_ctime(const char *name)
{
	struct stat buf;

	if (fth_stat(name, &buf))
		return (fth_make_long_long((ficl2Integer)buf.st_ctime));
	return (FTH_FALSE);
}

static void
ficl_file_ctime(ficlVm *vm)
{
#define h_file_ctime "( name -- time )  return file change time\n\
\"abc\" file-ctime time->string => \"Mon Aug 23 01:24:02 CEST 2010\"\n\
If NAME is a file, return its status change time, otherwise #f.  \
One can convert the number in a readable string with time->string.\n\
See also file-atime, file-mtime and time->string."
	FTH res;

	FTH_STACK_CHECK(vm, 1, 1);
	res = fth_file_ctime(pop_cstring(vm));
	fth_push_ficl_cell(vm, res);
}

FTH
fth_file_mtime(const char *name)
{
	struct stat buf;

	if (fth_stat(name, &buf))
		return (fth_make_long_long((ficl2Integer)buf.st_mtime));
	return (FTH_FALSE);
}

static void
ficl_file_mtime(ficlVm *vm)
{
#define h_file_mtime "( name -- time )  return file modification time\n\
\"abc\" file-mtime time->string => \"Mon Aug 23 01:24:02 CEST 2010\"\n\
If NAME is a file, return its last modification time, otherwise #f.  \
One can convert the number in a readable string with time->string.\n\
See also file-atime, file-ctime and time->string."
	FTH res;

	FTH_STACK_CHECK(vm, 1, 1);
	res = fth_file_mtime(pop_cstring(vm));
	fth_push_ficl_cell(vm, res);
}

#if defined(HAVE_UTIMES)
#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif
#if defined(HAVE_TIME_H)
#include <time.h>
#endif
#endif

static void
ficl_file_touch(ficlVm *vm)
{
#define h_file_touch "( name time|nil -- )  change file modification time\n\
\"foo.bar\" current-time file-touch\n\
\"foo.bar\" nil          file-touch\n\
Change modification time of NAME to TIME.  \
If TIME is nil, use current time.  \
" h_system_error_info("utimes(2)")
	FTH tm;
	char *name;

	FTH_STACK_CHECK(vm, 2, 0);
	tm = fth_pop_ficl_cell(vm);
	name = pop_cstring(vm);
	if (name == NULL)
		return;
	if (!fth_file_exists_p(name)) {
		FILE *fp;

		fp = fopen(name, "w");
		if (fp == NULL)
			FTH_SYSTEM_ERROR_ARG_THROW(fopen, name);
		fclose(fp);
	}
#if defined(HAVE_UTIMES)
	if (FTH_NUMBER_P(tm)) {	/* use specified time */
		static struct timeval tv[2];

		/*-
		 * tv[0]: access time
		 * tv[1]: modification time
		 */

		tv[0].tv_sec = (time_t)fth_ulong_long_ref(tm);
		tv[0].tv_usec = 0;
		tv[1] = tv[0];
		if (utimes(name, tv) == -1)
			FTH_SYSTEM_ERROR_ARG_THROW(utimes, name);
	} else if (utimes(name, NULL) == -1)	/* use current time */
		FTH_SYSTEM_ERROR_ARG_THROW(utimes, name);
#endif
}

/* === DIRECTORY === */

#if defined(HAVE_DIRENT_H)
#include <dirent.h>
#endif

static void
ficl_file_dir(ficlVm *vm)
{
#define h_file_dir "( dir -- files-array )  return files in dir\n\
\".\" file-dir => #( \"./xdef\" \"./xdef.bak\" ... )\n\
Return an array of all file names found in DIR.  \
" h_system_error_info("opendir(3)")
	FTH dir, res;

	FTH_STACK_CHECK(vm, 1, 1);
	dir = ficlStackPopFTH(vm->dataStack);
	res = fth_file_match_dir(dir, fth_make_regexp(".*"));
	ficlStackPushFTH(vm->dataStack, res);
}

FTH
fth_file_match_dir(FTH string, FTH regexp)
{
#define h_f_match_dir "( dir reg -- files-array )  return matching files\n\
\".\" /(bak|out)$/ file-match-dir\n\
  => #( \"./xdef.bak\" \"./xdef.out\" ... )\n\
Return an array of file names in DIR matching regexp REG.  \
" h_system_error_info("opendir(3)")
#if defined(HAVE_OPENDIR)
	FTH array, fs;
	DIR *dir;
	char *path, *npath;
	struct dirent *d;
	size_t len, flen;

	array = fth_make_empty_array();
	if (!FTH_STRING_P(string))
		return (array);
	len = (size_t)fth_string_length(string);
	path = fth_string_ref(string);
	if (len > 1 && path[len - 1] == '/')
		path[len - 1] = '\0';
	dir = opendir(path);
	if (dir == NULL) {
		FTH_SYSTEM_ERROR_ARG_THROW(opendir, path);
		/* NOTREACHED */
		return (FTH_FALSE);
	}
	if (FTH_STRING_P(regexp))
		regexp = fth_make_regexp(fth_string_ref(regexp));
	while ((d = readdir(dir)) != NULL) {
		npath = (len == 1 && path[0] == '/') ? "" : path;
#if defined(HAVE_STRUCT_DIRENT_D_NAMLEN)
		flen = d->d_namlen;
#else
		flen = fth_strlen(d->d_name);
#endif
#if defined(HAVE_STRUCT_DIRENT_D_INO)
		if (d->d_ino == 0)	/* skip deleted files */
			continue;
#elif defined(HAVE_STRUCT_DIRENT_D_FILENO)
		if (d->d_fileno == 0)
			continue;
#endif
		if (flen == 1 &&
		    d->d_name[0] == '.')
			continue;
		if (flen == 2 &&
		    d->d_name[0] == '.' &&
		    d->d_name[1] == '.')
			continue;
		fs = fth_make_string(d->d_name);
		if (fth_regexp_search(regexp, fs, 0L, -1L) >= 0) {
			FTH s;

			s = fth_make_string_format("%s/%.*s",
			    npath, flen, d->d_name);
			fth_array_push(array, s);
		}
	}
	if (closedir(dir) == -1)
		FTH_SYSTEM_ERROR_ARG_THROW(closedir, path);
	return (array);
#else				/* !HAVE_OPENDIR */
	return (fth_make_empty_array());
#endif				/* HAVE_OPENDIR */
}

void
init_file(void)
{
	/* file */
	FTH_PRI1("file-delete", ficl_file_delete, h_file_delete);
	FTH_PRI1("file-chmod", ficl_file_chmod, h_file_chmod);
	FTH_PRI1("file-mkdir", ficl_file_mkdir, h_file_mkdir);
	FTH_PRI1("file-rmdir", ficl_file_rmdir, h_file_rmdir);
	FTH_PRI1("file-mkfifo", ficl_file_mkfifo, h_file_mkfifo);
	FTH_PRI1("file-symlink", ficl_file_symlink, h_file_symlink);
	FTH_PRI1("file-rename", ficl_file_rename, h_file_rename);
	FTH_PRI1("file-copy", ficl_file_copy, h_file_copy);
	FTH_PRI1("file-install", ficl_file_install, h_file_install);
	FTH_PRI1("file-split", ficl_file_split, h_file_split);
	FTH_PRI1("file-basename", ficl_file_basename, h_file_basename);
	FTH_PRI1("file-dirname", ficl_file_dirname, h_file_dirname);
	FTH_PRI1("file-fullpath", ficl_file_fullpath, h_file_fullpath);
	FTH_PRI1("file-realpath", ficl_file_realpath, h_file_realpath);
	FTH_PRI1("file-pwd", ficl_file_pwd, h_file_pwd);
	FTH_PRI1("file-chdir", ficl_file_chdir, h_file_chdir);
	FTH_PRI1("chdir", ficl_file_chdir, h_file_chdir);
	FTH_PRI1("file-truncate", ficl_file_truncate, h_file_truncate);
	FTH_PRI1("file-chroot", ficl_file_chroot, h_file_chroot);
	FTH_PRI1("file-eval", ficl_file_eval, h_file_eval);
	FTH_PRI1("file-shell", ficl_file_shell, h_file_shell);
	FTH_PRI1("shell", ficl_file_shell, h_file_shell);
	FTH_PRI1("file-system", ficl_file_system, h_file_system);
	FTH_PRI1("open-pipe", ficl_open_pipe, h_open_pipe);
	FTH_PRI1("close-pipe", ficl_close_pipe, h_close_pipe);
	/* file test */
	FTH_PRI1("file-block?", ficl_file_block_p, h_file_block_p);
	FTH_PRI1("file-character?", ficl_file_character_p, h_file_character_p);
	FTH_PRI1("file-directory?", ficl_file_directory_p, h_file_directory_p);
	FTH_PRI1("file-exists?", ficl_file_exists_p, h_file_exists_p);
	FTH_PRI1("file-fifo?", ficl_file_fifo_p, h_file_fifo_p);
	FTH_PRI1("file-symlink?", ficl_file_symlink_p, h_file_symlink_p);
	FTH_PRI1("file-socket?", ficl_file_socket_p, h_file_socket_p);
	FTH_PRI1("file-executable?", ficl_file_executable_p, h_file_executable_p);
	FTH_PRI1("file-readable?", ficl_file_readable_p, h_file_readable_p);
	FTH_PRI1("file-writable?", ficl_file_writable_p, h_file_writable_p);
	FTH_PRI1("file-owned?", ficl_file_owned_p, h_file_owned_p);
	FTH_PRI1("file-grpowned?", ficl_file_grpowned_p, h_file_grpowned_p);
	FTH_PRI1("file-setuid?", ficl_file_setuid_p, h_file_setuid_p);
	FTH_PRI1("file-setgid?", ficl_file_setgid_p, h_file_setgid_p);
	FTH_PRI1("file-sticky?", ficl_file_sticky_p, h_file_sticky_p);
	FTH_PRI1("file-zero?", ficl_file_zero_p, h_file_zero_p);
	FTH_PRI1("file-length", ficl_file_length, h_file_length);
	FTH_PRI1("file-atime", ficl_file_atime, h_file_atime);
	FTH_PRI1("file-ctime", ficl_file_ctime, h_file_ctime);
	FTH_PRI1("file-mtime", ficl_file_mtime, h_file_mtime);
	FTH_PRI1("file-touch", ficl_file_touch, h_file_touch);
	FTH_PRI1("file-dir", ficl_file_dir, h_file_dir);
	FTH_PROC("file-match-dir", fth_file_match_dir, 2, 0, 0, h_f_match_dir);
	FTH_ADD_FEATURE_AND_INFO("file", h_list_of_file_functions);
}

/*
 * file.c ends here
 */
