/*	$NetBSD: svr4_exec.c,v 1.63 2008/10/15 06:51:20 wrstuden Exp $	 */

/*-
 * Copyright (c) 1994, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: svr4_exec.c,v 1.63 2008/10/15 06:51:20 wrstuden Exp $");

#if defined(_KERNEL_OPT)
#include "opt_syscall_debug.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>

#include <uvm/uvm_extern.h>

#include <machine/svr4_machdep.h>

#include <compat/svr4/svr4_types.h>
#include <compat/svr4/svr4_syscall.h>
#include <compat/svr4/svr4_errno.h>
#include <compat/svr4/svr4_signal.h>
#include <compat/svr4/svr4_exec.h>

extern char svr4_sigcode[], svr4_esigcode[];
extern struct sysent svr4_sysent[];
extern const char * const svr4_syscallnames[];
#ifndef __HAVE_SYSCALL_INTERN
void syscall(void);
#endif

struct uvm_object *emul_svr4_object;

const struct emul emul_svr4 = {
	"svr4",
	"/emul/svr4",
#ifndef __HAVE_MINIMAL_EMUL
	0,
	native_to_svr4_errno,
	SVR4_SYS_syscall,
	SVR4_SYS_NSYSENT,
#endif
	svr4_sysent,
#ifdef SYSCALL_DEBUG
	svr4_syscallnames,
#else
	NULL,
#endif
	svr4_sendsig,
	trapsignal,
	NULL,
	svr4_sigcode,
	svr4_esigcode,
	&emul_svr4_object,
	svr4_setregs,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
#ifdef __HAVE_SYSCALL_INTERN
	svr4_syscall_intern,
#else
	syscall,
#endif
	NULL,
	NULL,

	uvm_default_mapaddr,
	NULL,	/* e_usertrap */
	NULL,	/* e_sa */
	0,	/* e_ucsize */
	NULL,	/* e_startlwp */
};
