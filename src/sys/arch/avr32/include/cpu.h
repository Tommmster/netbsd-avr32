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
#ifndef _AVR32_CPU_H_
#define _AVR32_CPU_H_

#include <sys/device.h>
#include <sys/cpu_data.h>

#include <avr32/cpuregs.h>

struct cpu_info {
	struct cpu_data ci_data;	/* MI per-cpu data */
	struct cpu_info *ci_next;	/* Next CPU in list */
	u_long ci_cpu_freq;		/* CPU frequency */
	u_long ci_cycles_per_hz;	/* CPU freq / hz */
	u_long ci_divisor_delay;	/* for delay/DELAY */
	struct lwp *ci_curlwp;		/* currently running lwp */
	int ci_want_resched;
	int ci_mtx_count;
	int ci_mtx_oldspl;
	int ci_idepth;			/* hardware interrupt depth */
};

struct clockframe {
	int	st; 	/* status register at the time of the interrupt */
	int	pc;	/* pc at the time of the interrupt */
};

#define CLKF_INTR(fp)     0
#define CLKF_PC(fp)       0
#define CLKF_USERMODE(fp) AVR32_USER_MODE((fp)->st)

extern struct cpu_info cpu_info_store;
extern struct lwp *avr32_curlwp; 

#define	curlwp			avr32_curlwp
#define	curcpu()		(curlwp->l_cpu)
#define	curpcb			((struct pcb *)curlwp->l_addr)
#define	cpu_number()		(0)
#define	cpu_proc_fork(p1, p2)

/* avr32_machdep.c */
void cpu_identify(void);
void avr32_vector_init(void);

#define aston(l)		((l)->l_md.md_astpending = 1)

#endif /* !_AVR32_CPU_H_ */
