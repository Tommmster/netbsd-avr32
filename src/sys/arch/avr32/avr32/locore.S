/* 	$NetBSD$	 */

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
 * struct lwp * cpu_switchto(struct lwp *l1, struct lwp *l2)
 * Returns the lwp we're switching from.
 */
	.align 1
	.text
	.globl _cpu_switchto

_cpu_switchto:
	/*
	 * Save the old context, unless it is exiting.
	 */
	cp.w	r12, 0
	breq	1f

	ld.w	r8, r12[L_ADDR]

	sub	r8, -(U_PCB_CONTEXT + SF_REG_SR)
	mfsr	r9, SR_STATUS
	nop

	andh	r9, hi(~(AVR32_STATUS_IM | AVR32_STATUS_EM)), COH
	st.w	r8++, r9
	stm	r8, r0-r7,sp,lr

#ifdef IPL_ICU_MASK
#error IPL_ICU_MASK
#endif

1:	lda.w	r0, avr32_curlwp 
	st.w	r0[0], r11

	/*
	 * Switch to new context
	 * Load the UPTEs from the new process into the entry #0 and entry #1
	 */

	/* XXX Should invalidate relevant entries only. */
	mfsr	r0, SR_MMUCR	
	nop
	sbr	r0, 0x2
	andl	r0, lo(~0x0000C000)
	andh	r0, hi(~0x00070000)
	mtsr	SR_MMUCR, r0
	
	ld.w	r1, r11[L_MD_UPTE_0]	
	andh	r1, hi(~(AVR32_PTE_VALID | AVR32_PTE_WIRED))
	orl	r1, lo(AVR32_PG_GLOBAL)

	ld.w	r0, r11[L_ADDR]	/* newlwp->l_addr */
	andl	r0, lo(AVR32_PG_VPN)
	orl	r0, lo(AVR32_PG_VALID)
	
	mtsr	SR_TLBELO, r1	
	mtsr	SR_TLBEHI, r0
	sub	pc, -2		/* Flush pipeline on ASID change */
	nop
	tlbw
	nop

	ld.w	r1, r11[L_MD_UPTE_1]	
	andh	r1, hi(~(AVR32_PTE_VALID | AVR32_PTE_WIRED))
	orl	r1, lo(AVR32_PG_GLOBAL)

	ld.w	r0, r11[L_ADDR]	/* newlwp->l_addr */
	sub	r0, -PAGE_SIZE
	andl	r0, lo(AVR32_PG_VPN)
	orl	r0, lo(AVR32_PG_VALID | AVR32_PG_GLOBAL)
	
	mtsr	SR_TLBELO, r1	
	mtsr	SR_TLBEHI, r0
	sub	pc, -2		/* Flush pipeline on ASID change */
	nop
	tlbw
	nop
#if notyet
	/* 
	 * Check for restartable atomic sequences. 
	 */
#endif
	/*
	 * New context is now active. Restore U_PCB registers.
	 */
	ld.w	r8, r11[L_ADDR]	/* r0: newlwp->l_addr */
	ld.w	r9, r8++
	andh	r9, hi(AVR32_STATUS_IM | AVR32_STATUS_EM), COH

	mfsr	r10, SR_STATUS
	nop
	or	r9, r10
	mtsr	SR_STATUS, r9
	frs			
	sub	pc, -2
	ldm	r8++, r0-r7,sp,pc
	.size cpu_switchto, . - cpu_switchto

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
	.align 1
	.text
	.globl lwp_trampoline

lwp_trampoline:
	/* Call lwp_startup(), with args from cpu_switchto()/cpu_setfunc() */
	lda.w	r10, lwp_startup
	lda.w	r11, avr32_curlwp
	ld.w	r11, r11[0]
	icall	r10

	/* Call the routine specified by cpu_setfunc() */
	mov	r12, r1
	icall	r0

	/* Return to user (won't happen if a kernel thread) */
	lda.w	r12, L_LWP_TRAMPOLINE_PANIC
	rcall	panic
	.size lwp_trampoline, . - lwp_trampoline

L_LWP_TRAMPOLINE_PANIC:
	.asciz "lwp_trampoline"

/*
 * Support routines for the spl(9) infrastructure follow.
 */

	.align 2
	.text
	.globl _spllower

_spllower:
#if 1
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
#else
	lda.w	r12, L_spllower_msg
	rcall	panic

	.section .data
L_spllower_msg:
	.asciz "_spllower"
#endif

	.align 2
	.text
	.globl _splraise

_splraise:
#if 1
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
#else
	lda.w	r12, L_splraise_msg
	rcall	panic

	.section .data
L_splraise_msg:
	.asciz "_splraise"
#endif

	.align 2
	.text
	.globl _splset

_splset:
#if 1
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
#else
	lda.w	r12, L_splset_msg
	rcall	panic

	.section .data
L_splset_msg:
	.asciz "_splset"
#endif

	.align 2
	.text
	.globl _splget

_splget:
	lda.w	r12, L_splget_msg
	rcall	panic

	.section .data
L_splget_msg:
	.asciz "_splget"

	.section .text,"ax",@progbits
	.global _splnone
_splnone:
	lda.w	r12, L_splnone_msg
	rcall	panic

	.section .data
L_splnone_msg:
	.asciz "_splnone"
