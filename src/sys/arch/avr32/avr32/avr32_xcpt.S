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
#include <avr32/avr32_param.h>

#include "assym.h"

	.section .evba.text,"ax",@progbits
	.align 2
exception_vectors:
	bral	handle_unrecoverable /* unrecoverable */
	.align 2
	bral	handle_critical /* tlb multiple hit */
	.align 2
	bral	handle_critical /* bus error data fetch */
	.align 2
	bral	handle_critical /* bus error instruction fetch */
	.align 2
	bral	handle_critical /* NMI */
	.align 2
	bral	handle_critical /* Instruction address */
	.align 2
	bral	handle_critical /* ITLB Protection */
	.align 2
	bral	handle_critical /* Breakpoint */
	.align 2
	bral	handle_critical /* Illegal Opcode */
	.align 2
	bral	handle_critical /* Unimplemented Instruction */
	.align 2
	bral	handle_critical /* Privilege violation */
	.align 2
	bral	handle_critical /* Floating Point */
	.align 2
	bral	handle_critical /* Coprocessor absent */
	.align 2
	bral	handle_critical /* Data address  (read) */
	.align 2
	bral	handle_critical /* Data address (write) */
	.align 2
	bral	handle_critical /* DTLB Protection (read) */
	.align 2
	bral	handle_critical /* DTLB Protection (write) */
	.align 2
	bral	handle_critical /* DTLB Modified */

	.section .tlbx.evba.text,"ax",@progbits
	.globl avr32_itlb_miss
avr32_itlb_miss:
	rjmp	avr32_tlb_miss

	.section .tlbr.evba.text,"ax",@progbits
	.globl avr32_dtlb_rmiss
avr32_dtlb_rmiss:
	rjmp	avr32_tlb_miss
	
	.section .tlbw.evba.text,"ax",@progbits
	.globl avr32_dtlb_wmiss
avr32_dtlb_wmiss:
	rjmp	avr32_tlb_miss

/*
 *	TLB read miss:
 *	TLBEAR : Failing Virtual Address,
 *	TLBEHI[VPN]: Failing Page Number,
 *	TLBEHI[I]: 0
 *
 *	Sysmap: Kernel PTE Array
 *	Sysmapsize: Number of entries.
 */

	.section .work.evba.text,"ax",@progbits
	.globl	avr32_tlb_miss
	.align	1

avr32_tlb_miss:
	pushm	r0-r3

	mfsr	r3, SR_TLBEAR	/* Faulting address. */
	nop

	lddpc	r2, L_VM_MIN_KERNEL_ADDRESS
	cp.w	r3, r2
	brlo	usertlbmiss

	lddpc	r2, L_VM_MIN_KERNEL_ADDRESS
	sub	r3, r2

	/*
	 * Failing VPN should be lower than Sysmapsize.
	 */
	lsr	r3, r3, PGSHIFT
	lda.w	r2, Sysmapsize
	ld.w	r2, r2[0]

	cp.w	r3, r2
	brge	out_of_range

	/* Retrieve the PTE. */
	lda.w	r2, Sysmap	
	ld.w	r2, r2[0]
	add	r2, r2, r3 << 2 
	ld.w	r3, r2[0]

	/* Check for sw valid bit */
	 
	mov	r2, r3
	andh	r2, hi(AVR32_PTE_VALID), COH
	cp.w	r2, 0 
	breq	avr32_kern_gen_exception

	andh	r3, hi(~(AVR32_PTE_WIRED | AVR32_PTE_VALID)) 
	orl	r3, lo(AVR32_PG_DIRTY) /* XXX Needs further rework */

	mtsr	SR_TLBELO, r3
	nop
	nop

	mfsr	r2, SR_TLBEHI
	nop
	andl	r2, lo(AVR32_PG_VPN | AVR32_PG_ASID)
	orl	r2, lo(AVR32_PG_VALID)
	mtsr	SR_TLBEHI, r2
	sub	pc, -2 /* XXXAVR32 Flush pipeline */
	nop	
	nop

	tlbw
	nop
	nop

doeret: popm	r0-r3
	rete

out_of_range:
	lda.w	r12, L_OUT_OF_RANGE_MSG
	rcall	panic
L_OUT_OF_RANGE_MSG:
	.asciz "trap: index out of range\n"

/*
 *	usertlbmiss(p0addr)
 */
	.align 1
usertlbmiss:
#ifdef notyet
	lda.w	r12, L_AVR32_USERTLBMISS
	rcall	panic
#endif 
	/* r3 contains an address in P0. */

	/* Compute segment table index */
	mov	r2, r3
	lsr	r2, SEGSHIFT	
	lsl	r2, 2	

	/* Get pointer to the segment map */
	lda.w	r1, segbase
	ld.w	r1, r1[0]
	add	r1, r2

	/* Get the segment map. Detect invalid table entries */
	ld.w	r0, r1[0]
	cp.w	r0, 0
	breq	avr32_slow_fault

	/* Compute the segment map index */
	mov	r2, r3
	lsl	r2, PGSHIFT - 2
	andl	r2, lo(NPTEPG - 1) << 2), COH

	/* Get pointer to the segment map */
	add	r0, r2

	/* Get page PTE. Do not load invalid entries */
	ld.w	r0, r0[0]
	cp.w	r0, 0
	breq	avr32_slow_fault

	/* Update the TLB */
	andh	r0, hi(~(AVR32_PTE_WIRED | AVR32_PTE_VALID)) 
	orl	r0, lo(AVR32_PG_DIRTY) /* XXX Needs further rework */
	mtsr	SR_TLBELO, r0

#if 0
	mfsr	r2, SR_TLBEHI
	nop
	andl	r2, 0xf0ff	# Keep the asid
	orl	r2, lo(AVR32_PG_VALID)
	mtsr	SR_TLBEHI, r2
	nop	
#endif 
	tlbw	
	sub	pc, -2
	nop

	bral	doeret

	.section .work.evba.text,"ax",@progbits
	.globl	avr32_slow_fault
	.align 1
avr32_slow_fault:
	mfsr	r0, SR_RSR_EX
	nop

	mov	r2, 0
	orh	r2, hi(AVR32_STATUS_MKERN)
	mov	r1, r0
	andh	r1, hi(AVR32_STATUS_MMASK), COH
	lda.w	r3, avr32_kern_gen_exception
	cp.w	r1, r2
	breq	1f

	mov	r2, 0
	orh	r2, hi(AVR32_STATUS_MUSER)
	mov	r1, r0
	andh	r1, hi(AVR32_STATUS_MMASK), COH
	lda.w	r3, avr32_user_gen_exception
	cp.w	r1, r2
	breq	1f

	mov	r2, 0
	orh	r2, hi(AVR32_STATUS_MXCPT)
	mov	r1, r0
	andh	r1, hi(AVR32_STATUS_MMASK), COH
	lda.w	r3, avr32_kern_gen_exception
	cp.w	r1, r2
	breq	1f

	/* Unknown mode */
	lda.w	r12, L_AVR32_SLOW_FAULT
	mfsr	r11, SR_RSR_EX
	nop
	mfsr	r10, SR_RAR_EX
	nop
	pushm	r11
	pushm	r10
	rcall	panic
	
1:	icall	r3

	.section .work.evba.text,"ax",@progbits
	.globl	avr32_user_gen_exception
	.align 1
avr32_user_gen_exception:
	popm	r0-r3
	stmts	--sp, r0-lr
	mfsr	r1, SR_RAR_EX
	nop
	st.w	--sp, r1

	mfsr	r1, SR_RSR_EX
	nop
	st.w	--sp, r1

#if 0
	mov	sp, r0
#endif 
	mfsr	r12, SR_ECR
	mfsr	r11, SR_TLBEAR
	mov	r10, sp

	/* Unmask exceptions */
	mfsr	r0, SR_STATUS
	andh	r0, hi(~AVR32_STATUS_EM), COH
	mtsr	SR_STATUS, r0

	rcall	trap

	ld.w	r0, sp++
	mtsr	SR_RSR_EX, r0
	nop

	ld.w	r0, sp++
	mtsr	SR_RAR_EX, r0
	nop
	ldmts	sp++, r0-lr
	
	rete
	
	.section .work.evba.text,"ax",@progbits
	.globl	avr32_kern_gen_exception
	.align 1
avr32_kern_gen_exception:
	/* Create the kernframe in the stack and call trap */
	pushm	r4-r7, r8-r9, r10, r11, r12, lr
	mfsr	r0, SR_RAR_EX	
	nop
	st.w	--sp, r0

	mfsr	r0, SR_RSR_EX 
	nop
	st.w	--sp, r0

	/* Unmask exceptions */
	mfsr	r0, SR_STATUS
	andh	r0, hi(~AVR32_STATUS_EM), COH
	mtsr	SR_STATUS, r0

	mfsr	r12, SR_ECR
	mfsr	r11, SR_TLBEAR
	mov	r10, sp 
	rcall	trap

	/* 
	 * Restore registers and return from the exception.
	 */ 
	ld.w	r0, sp++
	mtsr	SR_RSR_EX, r0

	ld.w	r1, sp++
	mtsr	SR_RAR_EX, r1
	popm	r0-r3, r4-r7, r8-r9, r10, r11, r12, lr

	rete	
	.size avr32_kern_gen_exception,.-avr32_kern_gen_exception

L_AVR32_PANIC:
	.asciz "scall panic: syscall returned"
L_AVR32_MODE_PANIC:
	.asciz "scall panic: Incorrect mode %x."
L_AVR32_USERTLBMISS:
	.asciz "usertlbmiss"

L_AVR32_SLOW_FAULT_PANIC:
	.asciz "avr32_slow_fault"

L_AVR32_SLOW_FAULT:	
	.asciz "Not user nor kernel mode  STATUS: %x. RAR_EX: %x"

L_USER_GEN_EXCEPTION_MSG:
	.asciz "avr32_user_gen_exception %p"	

L_KERN_GEN_EXCEPTION_MSG:
	.asciz "avr32_kern_gen_exception %p"

	.align 2
L_VM_MIN_KERNEL_ADDRESS:
	.long VM_MIN_KERNEL_ADDRESS

	.align 2
handle_unrecoverable:
	mfsr	r12, SR_ECR
	nop
	mfsr	r11, SR_STATUS
	nop
	mfsr	r10, SR_RSR_EX
	nop
	mov	r9, r8
	nop
	rcall handle_with_panic2

	.section .scall.evba.text, "ax",@progbits
	.globl	scall_handler
scall_handler:
	mov	r9, sp
	bral	avr32_syscall

	.section .irq.text,"ax",@progbits
	.global avr32_intr_0
	.align 2
avr32_intr_0:
	sub	sp, 4
	stmts	--sp, r0-lr
	mfsr	r10, SR_RAR_INT0
	mfsr	r11, SR_RSR_INT0
	nop
	pushm	r10, r11
	mov	r12, 0
	lda.w	r9, cpu_intr
	ld.w	r9, r9[0]
	icall	r9
	popm	r10, r11
	mtsr	SR_RAR_INT0, r10
	mtsr	SR_RSR_INT0, r11
	ldmts	sp++, r0-lr
	sub	sp, -4
	rete

	.global avr32_intr_1
	.align 2
avr32_intr_1:
	sub	sp, 4
	stmts	--sp, r0-lr
	mfsr	r10, SR_RAR_INT1
	mfsr	r11, SR_RSR_INT1
	nop
	pushm	r10, r11
	mov	r12, 1
	lda.w	r9, cpu_intr
	ld.w	r9, r9[0]
	icall	r9
	popm	r10, r11
	mtsr	SR_RAR_INT1, r10
	mtsr	SR_RSR_INT1, r11
	ldmts	sp++, r0-lr
	sub	sp, -4
	rete

	.global avr32_intr_2
	.align 2
avr32_intr_2:
	sub	sp, 4
	stmts	--sp, r0-lr
	mfsr	r10, SR_RAR_INT2
	mfsr	r11, SR_RSR_INT2
	nop
	pushm	r10, r11
	mov	r12, 2
	lda.w	r9, cpu_intr
	ld.w	r9, r9[0]
	icall	r9
	popm	r10, r11
	mtsr	SR_RAR_INT2, r10
	mtsr	SR_RSR_INT2, r11
	ldmts	sp++, r0-lr
	sub	sp, -4
	rete

	.global avr32_intr_3
	.align 2
avr32_intr_3:
	sub	sp, 4
	stmts	--sp, r0-lr
	mfsr	r10, SR_RAR_INT3
	mfsr	r11, SR_RSR_INT3
	nop
	pushm	r10, r11
	mov	r12, 3
	lda.w	r9, cpu_intr
	ld.w	r9, r9[0]
	icall	r9
	popm	r10, r11
	mtsr	SR_RAR_INT3, r10
	mtsr	SR_RSR_INT3, r11
	ldmts	sp++, r0-lr
	sub	sp, -4
	rete


	.section .work.evba.text,"ax",@progbits
	.align 2
	.global handle_critical
handle_critical: 
	mfsr	r12, SR_ECR
	nop
	mfsr	r11, SR_RAR_EX
	nop
	mfsr	r10, SR_BEAR
	nop
	rcall	handle_with_panic

	.align 2
	.global avr32_syscall
avr32_syscall: 
	stmts	--sp, r0-lr

	mfsr	r0, SR_RAR_SUP
	nop
	st.w	--sp, r0

	mfsr	r0, SR_RSR_SUP
	nop
	st.w	--sp, r0

	lda.w	lr, avr32_curlwp
	ld.w	lr, lr[0]
	ld.w	lr, lr[L_PROC]
	ld.w	lr, lr[P_MD_SYSCALL]

	lda.w	r12, avr32_curlwp
	ld.w	r12, r12[0]

	mov	r11, 0
	mov	r10, 0

	icall	lr

	/* Check for kernel mode */	
	mfsr	r0, SR_STATUS
	nop

	andh	r0, hi(AVR32_STATUS_MMASK), COH
	lddpc	r1, L_AVR32_STATUS_MKERN
	cp.w	r0, r1
	breq	1f
	
	mov	r11, r1
	pushm	r11	
	lda.w	r12, L_AVR32_MODE_PANIC
	rcall	panic

1:	ldmts	sp++, r0
	mtsr	SR_RSR_SUP, r0
	nop

	ldmts 	sp++, r0
	mtsr	SR_RAR_SUP, r0
	nop

	ldmts	sp++, r0-lr
	rets
	
	.align 2
L_AVR32_STATUS_MKERN:
	.long AVR32_STATUS_MKERN
