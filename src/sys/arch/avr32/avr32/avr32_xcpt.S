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

	.section .evba.text,"ax",@progbits
	.align 2
exception_vectors:
	bral	handle_critical /* unrecoverable */
	.align 2
	bral	handle_critical /* tlb multiple hit */
	.align 2
	bral	handle_critical /* bus error data fetch */
	.align 2
	bral	handle_critical /* bus error instruction fetch */
	.align 2
	bral	handle_critical /* NMI */
	.align 2
	bral	instruction_address_handler /* Instruction address */
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
	bral	handle_critical

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
	pushm	r8-r9,r10,r11,r12,lr
	mfsr	r12, SR_TLBEAR	/* Faulting address. */
	mfsr	r11, SR_RAR_EX	/* Program counter. */
	mfsr	r10, SR_ECR	/* EVBA offset. */
	nop

	lddpc	r11, L_VM_MIN_KERNEL_ADDRESS
	cp.w	r12, r11
	brlo	usertlbmiss

	lddpc	r11, L_VM_MIN_KERNEL_ADDRESS
	sub	r12, r11

	/*
	 * Failing VPN should be lower than Sysmapsize.
	 */
	lsr	r12, r12, PGSHIFT
	lda.w	r11, Sysmapsize
	ld.w	r11, r11[0]

	cp.w	r12, r11	
	brge	out_of_range

	/* Retrieve the PTE. */
	lda.w	r11, Sysmap	
	ld.w	r11, r11[0]
	add	r11, r11, r12 << 2 
	ld.w	r12, r11[0]

	andh	r12, hi(~(AVR32_PTE_WIRED | AVR32_PTE_VALID)) 
	orl	r12, lo(AVR32_PG_DIRTY) /* XXX Needs further rework */

	mtsr	SR_TLBELO, r12
	nop
	nop

	mfsr	r11, SR_TLBEHI
	nop
	andl	r11, lo(AVR32_PG_VPN)
	orl	r11, lo(AVR32_PG_VALID)
	mtsr	SR_TLBEHI, r11
	nop	
	nop

	tlbw
	nop
	nop

doeret: popm	r8-r9,r10,r11,r12,lr	
	rete

out_of_range:
	lda.w	r12, L_OUT_OF_RANGE_MSG
	rcall	panic
L_OUT_OF_RANGE_MSG:
	.asciz "trap: index out of range\n"

	.align 1
usertlbmiss:
	/* Address in P0. */
	mov	r11, r12
	lsr	r11, SEGSHIFT	
	lsl	r11, 2	

	lda.w	r10, segbase
	ld.w	r10, r10[0]
		
	add	r10, r11
	ld.w	r9, r10[0]

	cp.w	r9, 0
	breq	avr32_slow_fault

	ld.w	r9, r9[0]
	mov	r11, r12

	lsl	r11, PGSHIFT - 2
	mov	r10, (NPTEPG - 1) << 2	
	and	r11, r10

	add	r9, r11
	ld.w	r9, r9[0]

	andh	r9, hi(~(AVR32_PTE_WIRED | AVR32_PTE_VALID)) 
	orl	r9, lo(AVR32_PG_DIRTY) /* XXX Needs further rework */
	mtsr	SR_TLBELO, r9

	mfsr	r11, SR_TLBEHI
	nop
	andl	r11, 0xf0ff	# Keep the asid
	orl	r11, lo(AVR32_PG_VALID)
	mtsr	SR_TLBEHI, r11
	nop	
	nop
	tlbw	

	bral	doeret

avr32_slow_fault:
	/*
	 *  Must call the trap function to handle this situation.
	 *  Store context in the process pcb.
	 *  STATUS (SR_STATUS), CAUSE(SR_ECR), Fault Adress (SR_TLBEAR), PC
	 */
	
	/* Save execution context, then call trap. */
	lda.w	r12, avr32_curlwp
	ld.w	r12, r12[0]
	ld.w	r12, r12[L_ADDR]	/* lwp->l_addr */
	stm	r12, r0-r8,lr,sp

	/* Invoke the trap handler. */
	mfsr	r12, SR_STATUS
	mfsr	r11, SR_ECR
	mfsr	r10, SR_TLBEAR
	mov	r9, SR_RAR_EX 
	mov	r8, sp
	rcall	trap

	/* Now we return: restore the register values.*/
	lda.w	r12, avr32_curlwp
	ld.w	r12, r12[0]
	ld.w	r12, r12[L_ADDR]	#lwp->l_addr

	ldm	r12, r0-r8,lr,sp
	popm	r8-r9,r10-r12,lr	
	rete

	.align 2
L_VM_MIN_KERNEL_ADDRESS:
	.long VM_MIN_KERNEL_ADDRESS

	.align 2
handle_critical: 
	mfsr	r12, SR_ECR
	mfsr	r11, SR_RAR_EX
	nop
	rcall	handle_with_panic

	.align 2
supervisor_call_handler:
	lda.w	r12, _inst_scall_msg
	rcall	panic
	rete
_inst_scall_msg:
	.string "inst.scall.text\n"
	
	.align 2
multiple_tlbhit_handler:
	lda.w	r12, _int_tlbmult_msg
	rcall	panic
	rete
_int_tlbmult_msg:
	.string "int.tlbmult.text\n"

	.align 2
databus_datafetch_handler:
	lda.w	r12, _dbus_dfetch_msg
	mfsr	r11, SR_BEAR
	nop
	rcall	panic	
	rete
_dbus_dfetch_msg:
	.string "dbus.dfetch.text\n"

	.align 2
databus_instfetch_handler:
	lda.w	r12, _dbus_ifetch_msg
	rcall	panic
	rete
_dbus_ifetch_msg:
	.string "dbus.ifetch.text\n"

	.align 2
external_nmi_handler:
	lda.w	r12, _ext_nmi_msg
	rcall	panic
	rete
_ext_nmi_msg:
	.string "ext.nmi.text\n"

	.align 2
itlb_protection_handler:
	lda.w	r12, _itlb_iprot_msg
	rcall	panic
	rete
_itlb_iprot_msg:
	.string "itlb.iprot.text\n"

	.align 2
breakpoint_handler:
	lda.w	r12, _ocd_break_msg
	rcall	panic
	rete
_ocd_break_msg:
	.string "ocd.break.text\n"

	.align 2
illegal_opcode_handler:
	lda.w	r12, _inst_opcode_msg
	rcall	panic
	rete
_inst_opcode_msg:
	.string "inst.opcode.text\n"

	.align 2
unimplemented_instruction_handler:
	lda.w	r12, _inst_unimp_msg
	rcall	panic
	rete
_inst_unimp_msg:
	.string "inst.unimp.text\n"

	.align 2
privilege_violation_handler:
	lda.w	r12, _inst_pviol_msg
	rcall	panic
	rete
_inst_pviol_msg:
	.string "inst.pviol.text\n"

	.align 2
copprocesor_absent_handler:
	lda.w	r12, _inst_cop_msg
	rcall	panic
	rete
_inst_cop_msg:
	.string "inst.cop.text\n"

	.align 2
read_data_address_handler:
	lda.w	r12, _dtlb_daddressr_msg
	rcall	panic
	rete
_dtlb_daddressr_msg:
	.string "dtlb.daddressr.text\n"

	.align 2
write_data_address_handler:
	lda.w	r12, _dtlb_daddressw_msg
	rcall	panic
	rete
_dtlb_daddressw_msg:
	.string "dtlb.daddressw.text\n"

	.align 2
read_dtlb_protection_handler:
	lda.w	r12, _dtlb_protr_msg
	rcall	panic
	rete
_dtlb_protr_msg:
	.string "dtlb.protr.text\n"

	.align 2
write_dtlb_protection_handler:
	lda.w	r12, _dtlb_protw_msg
	rcall	panic
	rete
_dtlb_protw_msg:
	.string "dtlb.protw.text\n"	

	.align 2
dtlb_modified_handler:
	lda.w	r12, _dtlb_modif_msg
	rcall	panic
	rete
_dtlb_modif_msg:
	.string "dtlb.modif.text\n"

	.align 2
instruction_address_handler:
	pushm	r10-r12, lr
	mfsr	r12, SR_ECR		/* Faulting address */
	mfsr	r11, SR_RAR_EX		/* Program counter */
	nop
	rcall 	handle_with_panic	/* Does not return */
	popm	r10-r12, lr	
	rete

	.section .scall.evba.text, "ax",@progbits
	.globl	scall_handler
	.align 1
scall_handler:
	bral	handle_critical

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
