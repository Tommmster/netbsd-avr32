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

#include <sys/lwp.h>
#include <sys/systm.h>
#include <machine/locore.h>
#include <machine/param.h>

/* XXXAVR32 Machine dependent. IMPLEMENT AS ASSEMBLY CODE ! */

struct lwp*
cpu_switchto(struct lwp *cur, struct lwp *next)
{
#define L_NAME(lwp) \
	((lwp->l_name) ? (lwp->l_name) : "null")

#define NAME(lwp) \
	((lwp) ? L_NAME(lwp) : "an exiting lwp")

#if 0
	printf("cpu_switchto: from %s to %s\n", NAME(cur), NAME(next));
#endif
#if 0
	printf("L_ADDR(old): %x  L_ADDR(new): %x \n", cur->l_addr, next->l_addr);
	printf("UPAGES(old): %x , %x  \n", cur->l_md.md_upte[0], cur->l_md.md_upte[1]);
	printf("UPAGES(new): %x , %x  \n", next->l_md.md_upte[0], next->l_md.md_upte[1]);
#endif
	return _cpu_switchto(cur, next);
}

void
dump_rx(unsigned int seq, unsigned int reg)
{
	printf("seq: %d, reg: 0x%x\n", seq, reg);
}

void*
setfunc_trampoline(void)
{
	panic("setfunc_trampoline: notyet");
}

void
MachSetPID(int pid)
{
}
