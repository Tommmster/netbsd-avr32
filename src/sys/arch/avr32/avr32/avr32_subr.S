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
 * void MachTLBUpdate(u_int tlbehi, u_int ptentry)
 */
ENTRY(MachTLBUpdate)
	stm --sp, lr

	/* Disable interrupts. */
	mfsr	r9, SR_STATUS
	nop
	mov	r8, r9
	orh	r8, hi(AVR32_STATUS_IM)
	mtsr	SR_STATUS, r8
	sub	pc, -2
	nop

	/* Remove SW bits. */
	andh	r11, hi(~(AVR32_PTE_WIRED | AVR32_PTE_VALID))

	/* Save a copy of the current ASID field. */
	mfsr	r10, SR_TLBEHI
	nop
	andl	r10, lo(AVR32_PG_ASID), COH

	/* Replace TLB entry. */
	mtsr	SR_TLBEHI, r12
	mtsr	SR_TLBELO, r11
	nop
	tlbs
	nop
	tlbw
	nop

	/* 
	 * Restore the previous ASID. Flush the pipeline as stated in the
	 * processor documentation.
	 */
	mtsr	SR_TLBEHI, r10
	sub	pc, -2 
	nop

	/* Restore previous interrupt state. */
	mtsr	SR_STATUS, r9
	sub	pc, -2
	nop

	ldm	sp++, pc
END(MachTLBUpdate)

/*
 * void avr32_compare_write(u_int32_t n);
 * 	Set the value of the COMPARE register.
 */
ENTRY(avr32_compare_write)
	mtsr	SR_COMPARE, r12
	nop
	mov	pc, lr
END(avr32_compare_write)

/*
 * void avr32_count_write(u_int32_t n);
 * 	Set the value of the COUNT register.
 */
ENTRY(avr32_count_write)
	mtsr	SR_COUNT, r12
	nop
	mov	pc, lr
END(avr32_count_write)

/*
 * u_int32_t avr32_count_read(void)
 * 	Return the value of the COUNT register.
 */
ENTRY(avr32_count_read)
	mfsr	r12, SR_COUNT
	nop
	mov	pc, lr
END(avr32_count_read)

/*
 * void cpu_switch_resume(struct lwp *newlwp)
 *
 * Wiredown the USPACE of newlwp in tlb entry #0
 */
ENTRY(cpu_switch_resume)
	stm	--sp, lr

	/* XXX Should invalidate relevant entries only. */
	mfsr	r0, SR_MMUCR
	nop
	sbr	r0, 0x2
	
	ld.w	r0, r12[L_MD]
	ld.w	r1, r0[MD_UPTE_0]

	ld.w	r0, r12[L_ADDR]
	
	/* XXX Watch out for those bits (wired, global). */
	mtsr	SR_TLBELO, r1	
	mtsr	SR_TLBEHI, r0
	
	ldm	sp++, pc
END(cpu_switch_resume)

/*
 * void avr32_tbis(uint32_t tlbehi, uint32_t tlbelo)
 */
ENTRY(avr32_tbis)
	/* Disable interrupts. */
	mfsr	r8, SR_STATUS
	sub	pc, -2
	nop
	mov	r9, r8
	orh	r9, hi(AVR32_STATUS_IM | AVR32_STATUS_EM)
	mtsr	SR_STATUS, r9
	sub	pc, -2
	nop
	
	/* Save a copy of the ASID */
	mfsr	r9, SR_TLBEHI
	nop

#if PARANOIADIAG || 1 /* XXXAVR32 */
	mov	r10, r12
	andl	r10, lo(AVR32_PG_VALID), COH
	brne	1f	

	PANIC("avr32_tbis: invalid entry")
	
1:	mov	r10, r12
	andl	r10, lo(AVR32_PG_ASID), COH
	cp.w	r10, 0	
	brne	2f
	
	mov	r10, r11
	andl	r10, lo(AVR32_PG_GLOBAL), COH
	cp.w	r10, AVR32_PG_GLOBAL
	breq	2f
	
	PANIC("avr32_tbis: non-global PTE entry without ASID")
#endif
2:	mtsr	SR_TLBEHI, r12
	sub	pc, -2
	nop

	mtsr	SR_TLBELO, r11
	nop

	tlbs
	mfsr	r10, SR_MMUCR	
	nop
	andl	r10, lo(AVR32_MMUCR_NF), COH
	brne	3f

	/* Invalidate this tlb entry */
	lddpc 	r10, L_AVR32_TLBEHI_DISABLED
	mtsr	SR_TLBEHI, r10
	sub	pc, -2

	mov	r10, 0
	mtsr	SR_TLBELO, r10
	nop

	tlbw
	nop

	/* Restore ASID before enabling interrupts. */
3:	mtsr 	SR_TLBEHI, r9
	sub	pc, -2

	/* Restore the status register. */
	mtsr	SR_STATUS, r8
	sub	pc, -2
	nop

	mov	pc, lr
END(avr32_tbis)

/*
 * void avr32_tbiap(int sizeofTLB)
 *
 * Invalidate TLB entries belonging to per process user spaces while
 * leaving entries for kernel space marked global intact.
 */
ENTRY(avr32_tbiap)
	/* Disable interrupts. */
	mfsr	r8, SR_STATUS
	nop
	mov	r9, r8
	orh	r9, hi(AVR32_STATUS_GM)
	mtsr	SR_STATUS, r9
	sub	pc, -2
	nop

	/* Get the amount of wired TLB entries. */
	mfsr	r9, SR_MMUCR
	nop
	lddpc	r10, L_AVR32_MMUCR_DLA_MASK
	and	r9, r10
	lsr	r9, AVR32_MMUCR_DLA_SHIFT

	/* Make r9 count in the DRP space. */
	lsl	r9, AVR32_MMUCR_DRP_SHIFT

	/* Same goes for the TLB size. */
	lsl	r12, AVR32_MMUCR_DRP_SHIFT

	/* Make DRP point to the current (non-locked) TLB entry. */
1:	mfsr	r10, SR_MMUCR
	lddpc	r11, L_AVR32_MMUCR_DRP_MASK
	andn	r10, r11
	or	r10, r9
	mtsr	SR_MMUCR, r10
	nop

	/* Zero-out the TLBEHI register. */
	mov	r10, 0
	mtsr	SR_TLBEHI, r10
	sub	pc, -2
	nop

	/* Obtain the TLB entry. */
	tlbr
	nop

	/* Check to see whether it has the G bit. */
	mfsr	r10, SR_TLBELO
	nop
	andl	r10, lo(AVR32_PG_GLOBAL), COH
	brne	2

	/* 
	 * Prepare to invalidate this TLB entry: make TLBEHI point to an
	 * invalid (impossible) virtual address, so that it can be seen in
	 * an eventual TLB dump during debugging. 
	 */
	lddpc	r10, L_AVR32_TLBEHI_DISABLED
	mtsr	SR_TLBEHI, r10
	sub	pc, -2
	nop

	/* Zero-out TLBELO. */
	mov	r10, 0
	mtsr	SR_TLBELO, r10
	nop

	/* Invalidate the TLB entry. */
	tlbw
	sub	pc, -2

	/* Increment the TLB index. */
2:	mov	r10, 1 << AVR32_MMUCR_DRP_SHIFT
	add	r9, r10
	cp.w	r9, r12
	brne	1b

	/* Restore the ASID register. */
	lda.w	r10, pmap_curr_asid
	ld.w	r10, r10
	mtsr	SR_TLBEHI, r10
	sub	pc, -2
	nop

	/* Restore interrupts, return. */
	mtsr	SR_STATUS, r8
	sub	pc, -2
	retal	lr
END(avr32_tbiap)

ENTRY(MachSetPID)
#ifdef PARANOIADIAG
	cp.w	r12, AVR32_PG_ASID
	brls	1f
	
	lda.w	r12, L_AVR32_HIGH_ASID
	rcall	panic
#endif

1:	mtsr	SR_TLBEHI, r12
	sub	pc, -2

	retal	 lr
END(MachSetPID)

	.align 2
L_AVR32_TLBEHI_DISABLED:
	.long AVR32_P1_START & AVR32_PG_VPN
L_AVR32_MMUCR_DLA_MASK:
	.long AVR32_MMUCR_DLA_MASK
L_AVR32_MMUCR_DRP_MASK:
	.long AVR32_MMUCR_DRP_MASK
L_AVR32_TBIS_TLBELO:
	.long AVR32_PG_SIZE_4K
L_AVR32_HIGH_ASID:
	.asciz "MachSetPID: Invalid ASID"
