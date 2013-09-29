/*	$NetBSD: linux_machdep.c,v 1.38 2008/04/28 20:23:43 martin Exp $ */

/*-
 * Copyright (c) 1995, 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden and Emmanuel Dreyfus.
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
__KERNEL_RCSID(0, "$NetBSD: linux_machdep.c,v 1.38 2008/04/28 20:23:43 martin Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/exec.h>
#include <sys/file.h>
#include <sys/callout.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/syscallargs.h>
#include <sys/filedesc.h>
#include <sys/exec_elf.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <sys/kauth.h>
#include <miscfs/specfs/specdev.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_ioctl.h>
#include <compat/linux/common/linux_hdio.h>
#include <compat/linux/common/linux_exec.h>
#include <compat/linux/common/linux_machdep.h>

#include <compat/linux/linux_syscallargs.h>
#include <compat/linux/linux_syscall.h>

#include <sys/cpu.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/vmparam.h>
#include <machine/locore.h>

/*
 * To see whether wscons is configured (for virtual console ioctl calls).
 */
#if defined(_KERNEL_OPT)
#include "wsdisplay.h"
#endif
#if (NWSDISPLAY > 0)
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplay_usl_io.h>
#endif

/*
 * Set set up registers on exec.
 * XXX not used at the moment since in sys/kern/exec_conf, LINUX_COMPAT
 * entry uses NetBSD's native setregs instead of linux_setregs
 */
void
linux_setregs(struct lwp *l, struct exec_package *pack, u_long stack)
{
	setregs(l, pack, stack);
}

/*
 * Send an interrupt to process.
 *
 * Adapted from sys/arch/mips/mips/mips_machdep.c
 */

void
linux_sendsig(const ksiginfo_t *ksi, const sigset_t *mask)
{
	const int sig = ksi->ksi_signo;
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct sigacts *ps = p->p_sigacts;
	struct linux_rt_sigframe *fp;
	struct frame *f;
	int i, onstack, error;
	struct linux_rt_sigframe sf;

#ifdef DEBUG_LINUX
	printf("linux_sendsig()\n");
#endif /* DEBUG_LINUX */
	f = (struct frame *)l->l_md.md_regs;

	/*
	 * Do we need to jump onto the signal stack?
	 */
	onstack =
	    (l->l_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	/*
	 * Allocate space for the signal handler context.
	 */
	if (onstack)
		panic("linux_sendsig: onstack notyet");
	else
		fp = (struct linux_rt_sigframe *)(u_int32_t)f->f_regs[_R_SP];

	/*
	 * Build stack frame for signal trampoline.
	 */
	memset(&sf, 0, sizeof sf);

#if LINUX_SYS_rt_sigreturn > 127
#error LINUX_SYS_rt_sigreturn does not fit in a 16-bit opcode.
#endif

	/*
	 * This is the signal trampoline used by Linux, we don't use it,
	 * but we set it up in case an application expects it to be there.
	 *
	 * 	mov	r8, LINUX_SYS_rt_sigreturn
	 * 	scall
	 */
	sf.lrs_code = (0x3008d733 | LINUX_SYS_rt_sigreturn << 20);

	/*
	 * The user context.
	 */
	native_to_linux_sigset(&sf.lrs_uc.luc_sigmask, mask);
	sf.lrs_uc.luc_flags = 0;
	sf.lrs_uc.luc_link = NULL;

	/* This is used regardless of SA_ONSTACK in Linux AVR32. */
	sf.lrs_uc.luc_stack.ss_sp = l->l_sigstk.ss_sp;
	sf.lrs_uc.luc_stack.ss_size = l->l_sigstk.ss_size;
	sf.lrs_uc.luc_stack.ss_flags = 0;
	if (l->l_sigstk.ss_flags & SS_ONSTACK)
		sf.lrs_uc.luc_stack.ss_flags |= LINUX_SS_ONSTACK;
	if (l->l_sigstk.ss_flags & SS_DISABLE)
		sf.lrs_uc.luc_stack.ss_flags |= LINUX_SS_DISABLE;
	memcpy(sf.lrs_uc.luc_mcontext.lsc_regs, f->f_regs,
		sizeof(sf.lrs_uc.luc_mcontext.lsc_regs));
	sendsig_reset(l, sig);

	/*
	 * Install the sigframe onto the stack.
	 */
	fp -= sizeof(struct linux_rt_sigframe);
	mutex_exit(p->p_lock);
	error = copyout(&sf, fp, sizeof(sf));
	mutex_enter(p->p_lock);

	if (error != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
#ifdef DEBUG_LINUX
		printf("linux_sendsig: stack trashed\n");
#endif /* DEBUG_LINUX */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

#ifdef DEBUG_LINUX
	printf("sigcontext is at %p\n", &fp->lrs_sc);
#endif /* DEBUG_LINUX */

	/* Set up the registers to return to sigcode. */
	f->f_regs[_R_SP] = (register_t)fp;
	f->f_regs[_R_R12] = native_to_linux_signo[sig];
	f->f_regs[_R_R11] = 0;
	f->f_regs[_R_R10] = (register_t)&fp->lrs_uc;
	f->f_regs[_R_PC] = (register_t)SIGACTION(p, sig).sa_handler;

#define RESTORER(p, sig) \
	(p->p_sigacts->sa_sigdesc[(sig)].sd_tramp)

	if (ps->sa_sigdesc[sig].sd_vers != 0)	
		f->f_regs[_R_LR] = (register_t)RESTORER(p, sig);
	else
		panic("linux_sendsig: SA_RESTORER");

	/* Remember that we're now on the signal stack. */
	if (onstack)
		l->l_sigstk.ss_flags |= SS_ONSTACK;

	return;
}

/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 */
int
linux_sys_sigreturn(struct lwp *l, const struct linux_sys_sigreturn_args *uap, register_t *retval)
{
	panic("linux_sys_sigreturn: notyet");
	return (EJUSTRETURN);
}

int
linux_sys_rt_sigreturn(struct lwp *l, const void *v, register_t *retval)
{
	struct linux_ucontext *luctx;
	struct trapframe *tf = l->l_md.md_regs;
	struct linux_sigcontext *lsigctx;
	struct linux_rt_sigframe frame, *fp;
	ucontext_t uctx;
	mcontext_t *mctx;
	int error;

	fp = (struct linux_rt_sigframe *)(tf->tf_regs[_R_SP]);
	if ((error = copyin(fp, &frame, sizeof(frame))) != 0) {
		mutex_enter(l->l_proc->p_lock);
		sigexit(l, SIGILL);
		return error;
	}

	luctx = &frame.lrs_uc;
	lsigctx = &luctx->luc_mcontext;

	bzero(&uctx, sizeof(uctx));
	mctx = (mcontext_t *)&uctx.uc_mcontext;

	/* 
	 * Set the flags. Linux always have CPU, stack and signal state,
	 * FPU is optional. uc_flags is not used to tell what we have.
	 */
	uctx.uc_flags = (_UC_SIGMASK|_UC_CPU|_UC_STACK);
	uctx.uc_link = NULL;

	/*
	 * Signal set. 
	 */
	linux_to_native_sigset(&uctx.uc_sigmask, &luctx->luc_sigmask);

	/*
	 * CPU state.
	 */
	memcpy(mctx->__gregs, lsigctx->lsc_regs, sizeof(lsigctx->lsc_regs));

	/*
	 * And the stack.
	 */
	uctx.uc_stack.ss_flags = 0;
	if (luctx->luc_stack.ss_flags & LINUX_SS_ONSTACK)
		uctx.uc_stack.ss_flags |= SS_ONSTACK;

	if (luctx->luc_stack.ss_flags & LINUX_SS_DISABLE)
		uctx.uc_stack.ss_flags |= SS_DISABLE;

	uctx.uc_stack.ss_sp = luctx->luc_stack.ss_sp;
	uctx.uc_stack.ss_size = luctx->luc_stack.ss_size;

	/*
	 * And let setucontext deal with that.
	 */
	mutex_enter(l->l_proc->p_lock);
	error = setucontext(l, &uctx);
	mutex_exit(l->l_proc->p_lock);
	if (error)
		return error;

	return (EJUSTRETURN);
}

/*
 * major device numbers remapping
 */
dev_t
linux_fakedev(dev_t dev, int raw)
{
	return dev;
}

/*
 * We come here in a last attempt to satisfy a Linux ioctl() call
 */
int
linux_machdepioctl(struct lwp *l, const struct linux_sys_ioctl_args *uap, register_t *retval)
{
	panic("linux_machdepioctl: notyet");
	return 0;
}

/*
 * See above. If a root process tries to set access to an I/O port,
 * just let it have the whole range.
 */
int
linux_sys_ioperm(struct lwp *l, const struct linux_sys_ioperm_args *uap, register_t *retval)
{
	panic(" linux_sys_ioperm: notyet");
	return 0;
}

/*
 * wrapper linux_sys_new_uname() -> linux_sys_uname()
 */
int
linux_sys_new_uname(struct lwp *l, const struct linux_sys_new_uname_args *uap, register_t *retval)
{
	return linux_sys_uname(l, (const void *)uap, retval);
}

/*
 * In Linux, cacheflush is currently implemented
 * as a whole cache flush (arguments are ignored)
 * we emulate this broken beahior.
 */
int
linux_sys_cacheflush(struct lwp *l, const struct linux_sys_cacheflush_args *uap, register_t *retval)
{
	panic("linux_sys_cacheflush: notyet");
	return 0;
}

int
linux_usertrap(struct lwp *l, vaddr_t trapaddr, void *arg)
{
	panic("linux_usertrap: notyet");
	return 0;
}
