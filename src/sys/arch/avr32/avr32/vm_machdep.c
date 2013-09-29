/* 	$NetBSD$	*/

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

#include "opt_ddb.h"
#include "opt_coredump.h"

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: vm_machdep.c,v 1.121.4.1 2009/06/09 17:45:01 snj Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/user.h>
#include <sys/core.h>
#include <sys/exec.h>
#include <sys/sa.h>
#include <sys/savar.h>

#include <uvm/uvm_extern.h>

#include <avr32/pte.h>
#include <avr32/reg.h>


paddr_t kvtophys(vaddr_t);	/* XXX */

/*
 * Finish a fork operation, with process p2 nearly set up.
 * Copy and update the pcb and trap frame, making the child ready to run.
 *
 * Rig the child's kernel stack so that it will start out in
 * lwp_trampoline() and call child_return() with p2 as an
 * argument. This causes the newly-created child process to go
 * directly to user level with an apparent return value of 0 from
 * fork(), while the parent process returns normally.
 *
 * p1 is the process being forked; if p1 == &proc0, we are creating
 * a kernel thread, and the return path and argument are specified with
 * `func' and `arg'.
 *
 * If an alternate user-level stack is requested (with non-zero values
 * in both the stack and stacksize args), set up the user stack pointer
 * accordingly.
 */
void
cpu_lwp_fork(struct lwp *l1, struct lwp *l2, void *stack, size_t stacksize,
    void (*func)(void *), void *arg)
{
	struct pcb *pcb;
	struct frame *f;
	pt_entry_t *pte;
	int i, x, y;

	l2->l_md.md_ss_addr = 0;
	l2->l_md.md_ss_instr = 0;
	l2->l_md.md_astpending = 0;

#ifdef DIAGNOSTIC
	/*
	 * If l1 != curlwp && l1 == &lwp0, we're creating a kernel thread.
	 */
	if (l1 != curlwp && l1 != &lwp0)
		panic("cpu_lwp_fork: curlwp");
#endif

	/*
	 * Copy pcb from proc p1 to p2.
	 * Copy p1 trapframe atop on p2 stack space, so return to user mode
	 * will be to right address, with correct registers.
	 */
	memcpy(&l2->l_addr->u_pcb, &l1->l_addr->u_pcb, sizeof(struct pcb));
	f = (struct frame *)((char *)l2->l_addr + USPACE) - 1;
	memcpy(f, l1->l_md.md_regs, sizeof(struct frame));

	/*
	 * If specified, give the child a different stack.
	 */
	if (stack != NULL)
		f->f_regs[_R_SP] = (uintptr_t)stack + stacksize;

	l2->l_md.md_regs = (void *)f;
	l2->l_md.md_flags = l1->l_md.md_flags;

	x = AVR32_PG_ACCESS;
	y = (AVR32_PG_GLOBAL | AVR32_PG_ACCESS_RW | AVR32_PTE_WIRED | AVR32_PTE_VALID); 
	pte = kvtopte(l2->l_addr);
	for (i = 0; i < UPAGES; i++)
		l2->l_md.md_upte[i] = (pte[i].pt_entry &~ x) | y;

	pcb = &l2->l_addr->u_pcb;
	pcb->pcb_context[10] = (intptr_t)func;			/* r0 */
	pcb->pcb_context[9] = (intptr_t)arg;			/* r1 */
	pcb->pcb_context[8] = (intptr_t)l2;			/* r2 */
	pcb->pcb_context[2] = (intptr_t)f;			/* sp */
	pcb->pcb_context[1] = (intptr_t)lwp_trampoline;		/* lr */
	pcb->pcb_context[0] = (intptr_t)ipl2spl_table[IPL_SCHED];
}

/*
 * Finish a swapin operation.
 * We neded to update the cached PTEs for the user area in the
 * machine dependent part of the proc structure.
 */
void
cpu_swapin(struct lwp *l)
{
	panic("cpu_swapin: notyet");
}

void
cpu_swapout(struct lwp *l)
{
	panic("cpu_swapout: notyet");
}

void
cpu_lwp_free(struct lwp *l, int proc)
{
#if notyet
	if ((l->l_md.md_flags & MDP_FPUSED) && l == fpcurlwp)
		fpcurlwp = NULL; 
#endif
}

void
cpu_lwp_free2(struct lwp *l)
{
	(void)l;
}

#ifdef COREDUMP
/*
 * Dump the machine specific segment at the start of a core dump.
 */
int
cpu_coredump(struct lwp *l, void *iocookie, struct core *chdr)
{
	panic("cpu_coredump: notyet");	
}
#endif

/*
 * Map a user I/O request into kernel virtual address space.
 */
void
vmapbuf(struct buf *bp, vsize_t len)
{
	panic("vmapbuf: notyet");
}

/*
 * Unmap a previously-mapped user I/O request.
 */
void
vunmapbuf(struct buf *bp, vsize_t len)
{
	panic("vunmapbuf: notyet");
}

/*
 * Map a (kernel) virtual address to a physical address.
 */
paddr_t
kvtophys(vaddr_t kva)
{
	panic("kvtophys: notyet");
	return 0;
}
