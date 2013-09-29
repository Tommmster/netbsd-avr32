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
#include <avr32/avr32_param.h>

#include "assym.h"

	.section .evba.text,"ax",@progbits
	.align 2
exception_vectors:
	bral	handle_unrecoverable /* unrecoverable */
	.align 2
	bral	handle_critical  /* tlb multiple hit */
	.align 2
	bral	handle_critical  /* bus error data fetch */
	.align 2
	bral	handle_critical  /* bus error instruction fetch */
	.align 2
	bral	handle_critical  /* NMI */
	.align 2
	bral	handle_critical  /* Instruction address */
	.align 2
	bral	handle_critical  /* ITLB Protection */
	.align 2
	bral	handle_critical  /* Breakpoint */
	.align 2
	bral	handle_critical  /* Illegal Opcode */
	.align 2
	bral	handle_critical  /* Unimplemented Instruction */
	.align 2
	bral	handle_critical  /* Privilege violation */
	.align 2
	bral	handle_critical  /* Floating Point */
	.align 2
	bral	handle_critical  /* Coprocessor absent */
	.align 2
	bral	avr32_data_addr  /* Data address (read) */
	.align 2
	bral	avr32_data_addr  /* Data address (write) */
	.align 2
	bral	handle_critical  /* DTLB Protection (read) */
	.align 2
	bral	avr32_dtlb_wprot /* DTLB Protection (write) */
	.align 2
	bral	avr32_dtlb_mod   /* DTLB Modified */

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

	.section .work.evba.text,"ax",@progbits
	.globl	avr32_data_addr
	.align	1

avr32_data_addr:
	pushm	r0-r3
	bral	avr32_slow_fault

	.section .work.evba.text,"ax",@progbits
	.globl	avr32_dtlb_wprot
	.align	1

avr32_dtlb_wprot:
	pushm	r0-r3
	bral	avr32_slow_fault

	.section .work.evba.text,"ax",@progbits
	.globl	avr32_dtlb_mod
	.align	1

avr32_dtlb_mod:
	pushm	r0-r3
	bral	avr32_slow_fault

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

	/* Remove software bits in the PTE. */
	andh	r3, hi(~(AVR32_PTE_WIRED | AVR32_PTE_VALID))

	mtsr	SR_TLBELO, r3
	nop
	tlbw
	nop
	nop

	popm	r0-r3
	rete

out_of_range:
	PANIC("trap: index out of range")

/*
 *	usertlbmiss(p0addr)
 */
	.align 1
usertlbmiss:
	/* r3 contains an address in P0. */

	/* Compute segment table index. */
	mov	r2, r3
	lsr	r2, SEGSHIFT	
	lsl	r2, 2	

	/* Get pointer to the segment map. */
	lda.w	r1, segbase
	ld.w	r1, r1[0]
	add	r1, r2

	/* Get the segment map. Detect invalid table entries. */
	ld.w	r0, r1[0]
	cp.w	r0, 0
	breq	avr32_slow_fault

	/* Compute the segment map index. */
	mov	r2, r3
	lsr	r2, PGSHIFT - 2
	andl	r2, lo(NPTEPG - 1) << 2), COH

	/* Get pointer to the segment map. */
	add	r0, r2

	/* Get page PTE. Do not load invalid entries. */
	ld.w	r0, r0[0]
	cp.w	r0, 0
	breq	avr32_slow_fault

	/* Remove software bits in the PTE. */
	andh	r0, hi(~(AVR32_PTE_WIRED | AVR32_PTE_VALID))

	/* Update the TLB. */
	mtsr	SR_TLBELO, r0
	nop
	tlbw	
	sub	pc, -2
	nop

	popm	r0-r3
	rete

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

	mov	r2, 0
	orh	r2, hi(AVR32_STATUS_MINT2)
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
	pushm	r10
	pushm	r11
	rcall	panic
	
1:	nop
	mov	pc, r3

	.section .work.evba.text,"ax",@progbits
	.globl	avr32_user_gen_exception
	.align 1
avr32_user_gen_exception:
	popm	r0-r3

	stmts	--sp, r0-lr
	mfsr	r8, SR_RAR_EX
	mfsr	r9, SR_RSR_EX
	nop
	stm	--sp, r8-r9

	/* Enable exceptions. */
	mfsr	r0, SR_STATUS
	nop
	andh	r0, hi(~AVR32_STATUS_EM), COH
	mtsr	SR_STATUS, r0

	/* Call the trap handler. */
	mfsr	r12, SR_ECR
	mfsr	r11, SR_TLBEAR
	mov	r10, sp
	rcall	trap

	/* Check for asynchronous software traps. */
	lda.w	r0, avr32_curlwp
	ld.w	r0, r0[0]
	ld.w	r0, r0[L_MD_ASTPENDING]
	cp.w	r0, 0
	breq	1f

	/* 
	 * We have pending asynchronous traps; all the state is already saved.
	 */
	ld.w    r12, sp[FRAME_PC]
	rcall	ast	

	/*
	 * XXX trap.c may have enabled interrupts. Disable them to avoid
	 * interrupt and exception handling racing over the system registers.
	 * 
	 * Exceptions are disabled to assert the lockdown of the u-area.
	 */
1:	mfsr	r0, SR_STATUS
	nop
	orh	r0, hi(AVR32_STATUS_GM | AVR32_STATUS_EM)
	mtsr	SR_STATUS, r0
	nop

	/*
	 * XXX An asynchronous software trap (above), could have forced an
	 * LWP switch, so we could get back here running in a different 
	 * execution mode. Test the current excecution mode and return in
	 * an appropriate manner.
	 */
	mfsr	r0, SR_STATUS
	nop
	andh	r0, hi(AVR32_STATUS_MMASK), COH

	mov	r1, 0
	orh	r1, hi(AVR32_STATUS_MKERN)
	cp.w	r0, r1
	breq	2f

	mov	r1, 0
	orh	r1, hi(AVR32_STATUS_MXCPT)
	cp.w    r0, r1
	breq    3f

	mov	r1, 0
	orh	r1, hi(AVR32_STATUS_MINT2)
	cp.w	r0, r1
	breq	4f

	stm	--sp, r0
	PANIC("avr32_user_gen_exception: unhandled execution mode: 0x%x")

	/* Return to userland. */
2:	ldm	sp++, r8-r9
	mtsr	SR_RSR_SUP, r9
	mtsr	SR_RAR_SUP, r8
	nop
	ldmts	sp++, r0-lr
	rets

	/* Return from the exception. */
3:	ldm	sp++, r8-r9
	mtsr	SR_RSR_EX, r9
	mtsr	SR_RAR_EX, r8
	nop
	ldmts	sp++, r0-lr
	rete

	/* Return from INT2. */
4:	ldm	sp++, r8-r9
	mtsr	SR_RSR_INT2, r9
	mtsr	SR_RAR_INT2, r8
	nop
	ldmts	sp++, r0-lr
	rete

	.section .work.evba.text,"ax",@progbits
	.globl	avr32_kern_gen_exception
	.align 1
avr32_kern_gen_exception:
	stm	--sp, r4-lr
	mfsr	r8, SR_RAR_EX
	mfsr	r9, SR_RSR_EX
	nop
	stm	--sp, r8-r9

	/* Enable exceptions. */
	mfsr	r0, SR_STATUS
	nop
	andh	r0, hi(~AVR32_STATUS_EM), COH
	mtsr	SR_STATUS, r0

	/* Call the trap handler. */
	mfsr	r12, SR_ECR
	mfsr	r11, SR_TLBEAR
	mov	r10, sp 
	rcall	trap
		
	/*
	 * XXX trap.c may have enabled interrupts. Disable them to avoid
	 * interrupt and exception handling racing over the system registers. 
	 * 
	 * Exceptions are disabled to assert the lockdown of the u-area.
	 */
	mfsr	r0, SR_STATUS
	nop
	orh	r0, hi(AVR32_STATUS_GM | AVR32_STATUS_EM)
	mtsr	SR_STATUS, r0
	nop

	/* 
	 * Restore registers and return from the exception.
	 */
	ldm	sp++, r8-r9
	mtsr	SR_RSR_EX, r9
	mtsr	SR_RAR_EX, r8
	nop
	ldm	sp++, lr
	sub	sp, -4	/* skip sp */
	ldm	sp++, r0-r12
	rete
	.size avr32_kern_gen_exception,.-avr32_kern_gen_exception

L_AVR32_AST:
	.asciz "scall panic: ast"

L_AVR32_PANIC:
	.asciz "scall panic: syscall returned"
L_AVR32_SCALL_MODE_PANIC:
	.asciz "scall panic: incorrect mode: 0x%x."
L_AVR32_INTR2_MODE_PANIC:
	.asciz "avr32_intr_2: incorrect mode: 0x%x."
L_AVR32_USERTLBMISS:
	.asciz "usertlbmiss"

L_AVR32_SLOW_FAULT_PANIC:
	.asciz "avr32_slow_fault"

L_AVR32_SLOW_FAULT:	
	.asciz "avr32_slow_fault: Unknown mode RSR_EX: %x. RAR_EX: %x"

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
	mov	r9, sp
	nop
	rcall handle_with_panic2

	.section .scall.evba.text, "ax",@progbits
	.globl	scall_handler
scall_handler:
	bral	avr32_syscall

	.section .irq.text,"ax",@progbits
	.global avr32_intr_0
	.align 2
avr32_intr_0:
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
	rete

	.global avr32_intr_1
	.align 2
avr32_intr_1:
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
	rete

	.global avr32_intr_2
	.align 2
avr32_intr_2:
	stmts	--sp, r0-lr
	mfsr	r10, SR_RAR_INT2
	mfsr	r11, SR_RSR_INT2
	nop
	stm	--sp, r10-r11

	/* Call the interrupt handler. */
	mov	r12, 2
	lda.w	r9, cpu_intr
	ld.w	r9, r9[0]
	icall	r9

	/* Check for pending ASTs. */
	ld.w	r0, sp[FRAME_SR]
	andh	r0, hi(AVR32_STATUS_MMASK), COH
	cp.w	r0, AVR32_STATUS_MUSER
	brne	1f

	lda.w	r0, avr32_curlwp	
	ld.w	r0, r0[0]	
	ld.w	r0, r0[L_MD_ASTPENDING]
	cp.w	r0, 0
	breq	1f

	/* 
	 * We have pending asynchronous traps; all the state is already saved.
	 */
	ld.w	r12, sp[FRAME_PC]
	rcall	ast

	/*
	 * XXX An asynchronous software trap (above), could have forced an
	 * LWP switch, so we could get back here running in a different
	 * execution mode. Test the current excecution mode and return in
	 * an appropriate manner.
	 */
1:	mfsr	r0, SR_STATUS
	nop
	andh	r0, hi(AVR32_STATUS_MMASK), COH

	/* Return from INT2 mode. */
	mov	r1, 0
	orh	r1, hi(AVR32_STATUS_MINT2)
	cp.w    r0, r1
	breq    2f

	/* Return from supervisor mode. */
	mov	r1, 0
	orh	r1, hi(AVR32_STATUS_MKERN)
	cp.w	r0, r1
	breq	3f

	/* Return from exception mode. */
	mov	r1, 0
	orh	r1, hi(AVR32_STATUS_MXCPT)
	cp.w	r0, r1
	breq	4f

	/* Bail out on anything else. */
	mov	r11, r0
	pushm	r11
	lda.w	r12, L_AVR32_INTR2_MODE_PANIC
	rcall	panic

2:	ldm	sp++, r10-r11
	mtsr	SR_RAR_INT2, r10
	mtsr	SR_RSR_INT2, r11
	nop
	ldmts	sp++, r0-lr
	rete

3:	ldm	sp++, r10-r11
	mtsr	SR_RAR_SUP, r10
	mtsr	SR_RSR_SUP, r11
	nop
	ldmts	sp++, r0-lr
	rets

4:	ldm	sp++, r10-r11
	mtsr	SR_RAR_EX, r10
	mtsr	SR_RSR_EX, r11
	nop
	ldmts	sp++, r0-lr
	rete

	.global avr32_intr_3
	.align 2
avr32_intr_3:
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
1:	ldmts	sp++, r0-lr
	rete


	.section .work.evba.text,"ax",@progbits
	.align 2
	.global handle_critical
handle_critical: 
	mfsr	r12, SR_ECR
	nop
	mfsr	r11, SR_RAR_EX
	nop
	mfsr	r10, SR_TLBEAR
	nop
	
	rcall	handle_with_panic

	.align 2
	.global avr32_syscall
avr32_syscall: 
	stmts	--sp, r0-lr
	mfsr	r0, SR_RAR_SUP
	mfsr	r1, SR_RSR_SUP
	nop
	stm	--sp, r0-r1

	/* Invoke the system call handler. */
	lda.w	lr, avr32_curlwp
	ld.w	lr, lr[0]
	ld.w	lr, lr[L_PROC]
	ld.w	lr, lr[P_MD_SYSCALL]
	lda.w	r12, avr32_curlwp
	ld.w	r12, r12[0]
	mov	r11, 0
	mov	r10, 0
	icall	lr

	/* Check for asynchronous software traps. */
	lda.w	r0, avr32_curlwp
	ld.w	r0, r0[0]
	ld.w	r1, r0[L_MD_ASTPENDING]
	cp.w	r1, 0
	breq	1f

	/* 
	 * We have pending asynchronous traps; all the state is already saved.
	 */
	ld.w	r12, sp[FRAME_PC]
	rcall	ast	

	/*
	 * XXX Interrupts may be enabled at this point. Disable them to avoid
	 * interrupt and syscall handling racing over the system registers.
	 * 
	 * Exceptions are disabled to assert the lockdown of the u-area.
	 */
1:	mfsr	r0, SR_STATUS
	nop
	orh	r0, hi(AVR32_STATUS_GM | AVR32_STATUS_EM)
	mtsr	SR_STATUS, r0
	nop

	/*
	 * XXX An asynchronous software trap (above), could have forced an
	 * LWP switch, so we could get back here running in a different 
	 * execution mode. Test the current privilege mode and return in an
	 * appropriate manner.
	 */
	mfsr	r0, SR_STATUS
	nop
	andh	r0, hi(AVR32_STATUS_MMASK), COH

	/* Return from supervisor mode. */
	mov	r1, 0
	orh	r1, hi(AVR32_STATUS_MKERN)
	cp.w	r0, r1
	breq	2f

	/* Return from exception mode. */
	mov	r1, 0
	orh	r1, hi(AVR32_STATUS_MXCPT)
	cp.w	r0, r1
	breq	3f

	/* Return from INT2 mode. */
	mov	r1, 0
	orh	r1, hi(AVR32_STATUS_MINT2)
	cp.w	r0, r1
	breq	4f

	/* Bail out on anything else. */
	mov	r11, r0
	pushm	r11
	lda.w	r12, L_AVR32_SCALL_MODE_PANIC
	rcall	panic

	/* Return from supervisor mode. */
2:	ldm	sp++, r8-r9
	mtsr	SR_RSR_SUP, r9
	mtsr	SR_RAR_SUP, r8
	nop
	ldmts	sp++, r0-lr
	rets

	/* Return from exception mode. */
3:	ldm	sp++, r8-r9
	mtsr	SR_RSR_EX, r9
	mtsr	SR_RAR_EX, r8
	nop
	ldmts	sp++, r0-lr
	rete

	/* Return from INT2 mode. */
4:	ldm	sp++, r8-r9
	mtsr	SR_RSR_INT2, r9
	mtsr	SR_RAR_INT2, r8
	nop
	ldmts	sp++, r0-lr
	rete

	.align 2
L_AVR32_STATUS_MKERN:
	.long AVR32_STATUS_MKERN 

L_AVR32_STATUS_MINT2:
	.long AVR32_STATUS_MINT2
