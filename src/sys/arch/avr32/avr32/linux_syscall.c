/*	$NetBSD$	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 * 
 * This code is derived from software contributed to The NetBSD Foundation
 * by 
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/signal.h>
#include <sys/syscall.h>
#include <sys/syscallvar.h>
#include <sys/sa.h>
#include <sys/savar.h>

#include <uvm/uvm_extern.h>

#include <compat/linux/arch/avr32/linux_machdep.h>

#include <machine/cpu.h>
#include <avr32/trap.h>
#include <avr32/reg.h>
#include <avr32/userret.h>

void linux_syscall_intern(struct proc *);
void linux_syscall_plain(struct lwp *l, u_int status, u_int cause, u_int opc);
void linux_syscall_fancy(struct lwp *l, u_int status, u_int cause, u_int opc);

void
linux_syscall_intern(struct proc *p)
{

	if (trace_is_enabled(p))
		p->p_md.md_syscall = linux_syscall_fancy;
	else
		p->p_md.md_syscall = linux_syscall_plain;
}

void
linux_syscall_fancy(struct lwp *l, u_int status, u_int cause, u_int opc)
{
	panic("linux_syscall_fancy: not yet");
}

void
linux_syscall_plain(struct lwp *l, u_int status, u_int cause, u_int opc)
{
	struct proc *p = l->l_proc;
	struct frame *frame = (struct frame *)l->l_md.md_regs;
	register_t *args, copyargs[8];
	register_t *rval = NULL;	/* XXX gcc */
	register_t copyrval[2];

	size_t nsaved, nargs;
	const struct sysent *callp;
	int error;
	u_int code;

	LWP_CACHE_CREDS(l, p);

	uvmexp.syscalls++;
	
	callp = p->p_emul->e_sysent;
	code = frame->f_regs[_R_R8];

#ifdef KERN_SA
	if (__predict_false((l->l_savp)
            && (l->l_savp->savp_pflags & SAVP_FLAG_DELIVERING)))
		l->l_savp->savp_pflags &= ~SAVP_FLAG_DELIVERING;
#endif

	switch (code) {
	case SYS_syscall:
	case SYS___syscall:
		panic ("linux_syscall_plain: SYS_syscall / SYS__syscall not yet");
#if notyet
		args = copyargs;
		if (code == SYS_syscall) {
			/*
			 * Code is first argument, followed by actual args.
			 */
			code = frame->f_regs[_R_A0] - SYSCALL_SHIFT;
			args[0] = frame->f_regs[_R_A1];
			args[1] = frame->f_regs[_R_A2];
			args[2] = frame->f_regs[_R_A3];
			nsaved = 3;
		} else {
			/*
			 * Like syscall, but code is a quad, so as to maintain
			 * quad alignment for the rest of the arguments.
			 */
			code = frame->f_regs[_R_A0 + _QUAD_LOWWORD] 
			    - SYSCALL_SHIFT;
			args[0] = frame->f_regs[_R_A2];
			args[1] = frame->f_regs[_R_A3];
			nsaved = 2;
		}

		if (code >= p->p_emul->e_nsysent)
			callp += p->p_emul->e_nosys;
		else
			callp += code;
		nargs = callp->sy_argsize / sizeof(register_t);

		if (nargs > nsaved) {
			error = copyin(
			    ((register_t *)(vaddr_t)frame->f_regs[_R_SP] + 4),
			    (args + nsaved),
			    (nargs - nsaved) * sizeof(register_t));
			if (error)
				goto bad;
		}
#endif
		break;

	default:
		if (code >= p->p_emul->e_nsysent)
			callp += p->p_emul->e_nosys;
		else
			callp += code;
		nargs = callp->sy_narg;

		if (nargs < 5) {
			args = copyargs;
			args[0] = frame->f_regs[_R_R12];
			args[1] = frame->f_regs[_R_R11];
			args[2] = frame->f_regs[_R_R10];
			args[3] = frame->f_regs[_R_R9];
		} else {
			panic("linux_syscall_plain: nargs >=5 panic");
		}
		break;
	}
	rval = copyrval;
	rval[0] = 0;
	rval[1] = 0;

	error = sy_call(callp, l, args, rval);

	switch (error) {
	case 0:
		frame->f_regs[_R_R12] = rval[0];
		if (rval[1] != 0)
			panic("linux_syscall_plain: rval[1] != 0");
		break;
	case ERESTART:
		panic("linux_syscall_plain: ERESTART: notyet");
		break;
	case EJUSTRETURN:
		panic("linux_syscall_plain: EJUSTRETURN: notyet");
		break;	/* nothing to do */
	default:
	bad:
		if (p->p_emul->e_errno)
			error = p->p_emul->e_errno[error];
		frame->f_regs[_R_R12] = error; 
		break;  
	}
	userret(l);
}
