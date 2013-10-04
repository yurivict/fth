# Copyright (c) 2007-2012 Michael Scholz <mi-scholz@users.sourceforge.net>
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
# @(#)common.mk	1.23 9/13/13

ficlbuilddir   ?= ${top_builddir}/ficl
libbuilddir    ?= ${top_builddir}/lib
fthbuilddir    ?= ${top_builddir}/src

ficl_hdr	= ${ficldir}/ficllocal.h ${ficldir}/ficl.h
ficl_header	= ${ficl_hdr} ${ficldir}/ficltokens.h
src_header	= ${fthdir}/fth.h ${fthdir}/fth-lib.h ${fthdir}/utils.h
fth_config	= ${top_builddir}/fth-config.h ${top_builddir}/config.h 
ficl_common	= ${fth_config} ${ficl_header}
src_common	= ${fth_config} ${src_header} ${ficl_header}
ficl_full	= ${ficl_common} ${src_header}

fth_cflags	= -I${top_builddir} -I${fthdir} -I${ficldir} -I${comdir}
FTH_CFLAGS	= ${fth_cflags} -I${prefix}/include ${CFLAGS}
TRASH		= fth fth-s* *.so *.[ao] *core *.db
RM		= rm -f

.SUFFIXES:
.SUFFIXES: .o .c

.c.o:
	${CC} ${DEFS} ${FTH_CFLAGS} -c $<

clean:
	${RM} ${TRASH}

distclean: clean
	${RM} Makefile

maintainer-clean: clean
	${RM} *~ ${srcdir}/*~ Makefile

Makefile: ${srcdir}/Makefile.in ${top_builddir}/config.status
	cd ${top_builddir} && ${SHELL} ./config.status
	@echo "Makefile updated, restart."
	exit 1

${top_builddir}/config.status: ${top_srcdir}/configure
	(cd ${top_builddir} && ${SHELL} ./config.status --recheck)

.PHONY: all install fth-shared fth-static install-shared install-static \
	install-strip install-site-fth uninstall \
	dist check test clean distclean maintainer-clean depend

# common.mk ends here
