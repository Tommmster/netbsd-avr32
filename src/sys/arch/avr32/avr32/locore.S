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

#include <avr32/asm.h>
#include <avr32/cpuregs.h>

#include "assym.h"

/*
 * struct lwp *cpu_switchto(struct lwp *cur, struct lwp *next)
 *
 * Switch to the specified next LWP. Arguments:
 *
 * 	r12	the current LWP
 * 	r11	the LWP to switch to
 *
 * Returns:
 *
 * 	r12	the LWP we have switched from
 */

ENTRY(cpu_switchto)
	/*
	 * Save old context, unless the LWP is exiting.
	 */
	cp.w	r12, 0
	breq	1f

	ld.w	r8, r12[L_ADDR]
	sub	r8, -(U_PCB_CONTEXT + SF_REG_SR)

	mfsr	r9, SR_STATUS
	nop
	andh	r9, hi(AVR32_STATUS_IM | AVR32_STATUS_EM), COH

	st.w	r8++, r9
	stm	r8, r0,r1,r2,r3,r4,r5,r6,r7,sp,lr

	/* Save current ASID. */
	mfsr	r7, SR_TLBEHI
	nop
	andl	r7, lo(AVR32_PG_ASID)

	/*
	 * Switch to new context.
	 */
1:	lda.w	r0, avr32_curlwp
	st.w	r0[0], r11

	/*
	 * Wiredown the USPACE of newproc with TLB entry #0 and #1. Check
	 * if target USPACE is already refered by any TLB entry before
	 * doing that, and make sure TBIS(them) in the case.
	 */
	ld.w	r0, r11[L_ADDR]
	lddpc	r1, L_AVR32_P2_START
	cp.w	r0, r1
	brlt	resume

	ld.w	r0, r11[L_ADDR]
	andl	r0, lo(~(AVR32_PG_ASID | AVR32_PG_INSTR))
	orl	r0, lo(AVR32_PG_VALID)
	mtsr	SR_TLBEHI, r0
	sub	pc, -2
	nop
	tlbs
	nop
	mfsr	r1, SR_MMUCR
	nop
	andl	r1, lo(AVR32_MMUCR_NF), COH
	cp.w	r1, 0
	brne	entry0set /* Not found. */

	lddpc	r0, L_AVR32_TLBEHI_DISABLED
	mtsr	SR_TLBEHI, r0
	sub	pc, -2
	mov	r1, 0
	mtsr	SR_TLBELO, r1

	nop
	nop
	nop
	nop
	nop
	tlbw
	sub	pc, -2

entry0set:
	/* Lock down the first entry in the TLB. */
	lddpc	r0, L_MMUCR_LOCK_DOWN_0
	mtsr	SR_MMUCR, r0
	nop

	ld.w	r0, r11[L_ADDR]
	andl	r0, lo(~(AVR32_PG_ASID | AVR32_PG_INSTR))
	orl	r0, lo(AVR32_PG_VALID)
	mtsr	SR_TLBEHI, r0
	sub	pc, -2
	nop

	ld.w	r1, r11[L_MD_UPTE_0]
	andh	r1, hi(~(AVR32_PTE_VALID | AVR32_PTE_WIRED))
	orl	r1, lo(AVR32_PG_GLOBAL)
	mtsr	SR_TLBELO, r1
	nop
	nop
	nop
	nop
	nop
	tlbw
	sub	pc, -2
	nop
	nop
	nop

	ld.w	r0, r11[L_ADDR]
	sub	r0, -PAGE_SIZE
	andl	r0, lo(~(AVR32_PG_ASID | AVR32_PG_INSTR))
	orl	r0, lo(AVR32_PG_VALID)
	mtsr	SR_TLBEHI, r0
	sub	pc, -2
	nop
	tlbs
	nop
	nop
	nop
	nop
	nop

	mfsr	r1, SR_MMUCR
	nop
	andl	r1, lo(AVR32_MMUCR_NF), COH
	cp.w	r1, 0
	brne	entry1set /* Not found. */

	lddpc	r0, L_AVR32_TLBEHI_DISABLED
	mtsr	SR_TLBEHI, r0
	sub	pc, -2
	mov	r1, 0
	mtsr	SR_TLBELO, r1

	nop
	nop
	nop
	nop
	nop
	tlbw
	sub	pc, -2

entry1set:
	/* Lock down the second entry in the TLB. */
	lddpc	r0, L_MMUCR_LOCK_DOWN_1
	mtsr	SR_MMUCR, r0
	nop

	ld.w	r0, r11[L_ADDR]
	sub	r0, -PAGE_SIZE
	andl	r0, lo(~(AVR32_PG_ASID | AVR32_PG_INSTR))
	orl	r0, lo(AVR32_PG_VALID)
	mtsr	SR_TLBEHI, r0
	sub	pc, -2
	nop

	ld.w	r1, r11[L_MD_UPTE_1]
	andh	r1, hi(~(AVR32_PTE_VALID | AVR32_PTE_WIRED))
	orl	r1, lo(AVR32_PG_GLOBAL)
	mtsr	SR_TLBELO, r1
	nop

	nop
	nop
	nop
	nop
	nop
	tlbw
	sub	pc, -2

resume:
	/* Restore the ASID register. */
	mtsr	SR_TLBEHI, r7
	sub	pc, -2
	nop

	/* New context is now active */
	lda.w	r8, avr32_curlwp
	ld.w	r8, r8[0]

	ld.w	r8, r8[L_ADDR]
	sub	r8, -(U_PCB_CONTEXT + SF_REG_SR)
	ld.w	r9, r8++
	mfsr	r10, SR_STATUS
	nop
	andh	r10, hi(~(AVR32_STATUS_IM | AVR32_STATUS_EM))
	or	r10, r9
	mtsr	SR_STATUS, r10
	frs
	sub	pc, -2

	ldm	r8++, r0,r1,r2,r3,r4,r5,r6,r7,sp,lr
	mov	pc, lr
END(cpu_switchto)

	.align 2
L_AVR32_TLBEHI_DISABLED:
	.long AVR32_P1_START & AVR32_PG_VPN
L_AVR32_P2_START:
	.long AVR32_P2_START
L_MMUCR_LOCK_DOWN_0:
	.long AVR32_MMUCR_DRP(0) | AVR32_MMUCR_DLA(2) | AVR32_MMUCR_SMMU | AVR32_MMUCR_PMMU
L_MMUCR_LOCK_DOWN_1:
	.long AVR32_MMUCR_DRP(1) | AVR32_MMUCR_DLA(2) | AVR32_MMUCR_SMMU | AVR32_MMUCR_PMMU

/*
 * lwp_trampoline
 *
 * Special arrangement for a process about to go user mode right after
 * fork() system call.  When the first CPU tick is scheduled to the
 * forked child, it starts running from here.  Then, a service function
 * is called with one argument supplied to complete final preparations,
 * and the process returns to user mode as if the fork() system call is 
 * handled in a normal way.  No need to save any registers although this
 * calls another.
 */
ENTRY(lwp_trampoline)
	/* Call lwp_startup(), with args from cpu_switchto()/cpu_setfunc() */
	mov	r11, r2
	rcall	lwp_startup

	/* Call the routine specified by cpu_setfunc() */
	mov	r12, r1
	icall	r0

	/* Return to user (won't happen if a kernel thread) */

	/*
	 * XXX Interrupts could be enabled at this point. Disable them to
	 * avoid racing against interrupts over the system registers.
	 * 
	 * Exceptions are disabled to assert the lockdown of the u-area.
	 */
	mfsr	r0, SR_STATUS
	nop
	orh	r0, hi(AVR32_STATUS_GM | AVR32_STATUS_EM)
	mtsr	SR_STATUS, r0
	sub	pc, -2
	nop

	mfsr	r8, SR_STATUS
	andh	r8, hi(AVR32_STATUS_MMASK), COH
	nop
	mov	r9, 0
	orh	r9, hi(AVR32_STATUS_MKERN)
	cp.w	r8, r9
	breq	1f

	mov	r9, 0
	orh	r9, hi(AVR32_STATUS_MXCPT)
	cp.w	r8, r9
	breq	2f

	mov	r9, 0
	orh	r9, hi(AVR32_STATUS_MINT2)
	cp.w	r8, r9
	breq	3f

	stm	--sp, r8
	PANIC("lwp_trampoline: unhandled execution mode: 0x%x")

	/* Return from supervisor mode. */
1:	ldm	sp++, r8-r9
	mtsr	SR_RAR_SUP, r8
	mtsr	SR_RSR_SUP, r9
	nop
	ldmts	sp++, r0-lr
	rets

	/* Return from exception mode. */
2:	ldm	sp++, r8-r9
	mtsr	SR_RAR_EX, r8
	mtsr	SR_RSR_EX, r9
	nop
	ldmts	sp++, r0-lr
	rete

	/* Return from INT2 mode. */
3:	ldm	sp++, r8-r9
	mtsr	SR_RAR_INT2, r8
	mtsr	SR_RSR_INT2, r9
	nop
	ldmts	sp++, r0-lr
	rete
END(lwp_trampoline)

/*
 * Support routines for the spl(9) infrastructure follow.
 */

ENTRY(_spllower)
	/* Fetch a copy of the status register. */
	mfsr	r8, SR_STATUS
	nop

	/* Turn off every interrupt bit in the future SR. */
	mov	r9, r8
	andh	r9, hi(~AVR32_STATUS_IMx)

	/* Keep every masked interrupt present in r12. */
	mov	r10, r12
	andh	r10, hi(AVR32_STATUS_IMx), COH

	/* Assemble the new status register, store back. */
	or	r9, r10
	mtsr	SR_STATUS, r9
	nop

	/* Return previous interrupt level. */
	mov	r12, r8
	andh	r12, hi(AVR32_STATUS_IM), COH

	/* Return. */
	mov	pc, lr
END(_spllower)

ENTRY(_splraise)
	/* Fetch a copy of the status register. */
	mfsr	r8, SR_STATUS
	nop

	/* Keep future unmasked interrupt bits. */
	mov	r9, r12
	andh	r9, hi(AVR32_STATUS_IMx), COH

	/* Turn on the given bits, retain the rest. */
	mov	r10, r8
	or	r10, r9

	/* Store back. */
	mtsr	SR_STATUS, r10
	nop

	/* Return previous interrupt level. */
	mov	r12, r8
	andh	r12, hi(AVR32_STATUS_IM), COH

	/* Return. */
	mov	pc, lr
END(_splraise)

ENTRY(_splset)
	/* Fetch status register. */
	mfsr	r8, SR_STATUS
	nop

	/* Erase every non-interrupt bit in argument. */
	andh	r12, hi(AVR32_STATUS_IM), COH

	/* Turn off every interrupt bit in SR. */
	mov	r9, r8
	andh	r9, hi(~AVR32_STATUS_IM)

	/* Set previous interrupt bits. */
	or	r9, r12

	/* Store back. */
	mtsr	SR_STATUS, r9
	nop

	/* Compute previous interrupt state. */
	mov	r12, r8
	andh	r12, hi(AVR32_STATUS_IM), COH

	/* Return. */
	mov	pc, lr
END(_splset)

ENTRY(avr32_pagecopy)
	stm	--sp, lr
	mov	r10, PAGE_SIZE
	rcall	memcpy
	ldm	sp++, pc
END(avr32_pagecopy)

ENTRY(_splget)
	PANIC("_splget: notyet")
END(_splget)

ENTRY(_splnone)
	PANIC("_splnone: notyet")
END(_splnone)
