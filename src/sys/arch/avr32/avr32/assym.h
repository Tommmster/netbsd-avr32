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

/*
 * XXX This entire file is just a temporary shortcut to bootstrap the
 * porting initiative: since we don't have a proper build environment (yet),
 * we generate assym.h manually, on-demand, preserving the semantics of the
 * (future, properly built version of the) file.
 */

#define PAGE_SIZE	4096
#define NBPG		4096
#define PGSHIFT		12      
#define SEGSHIFT	22
#define NPTEPG		(NBPG/4)

#define VM_MIN_KERNEL_ADDRESS   0xC0000000

#define AVR32_PG_VPN		0xfffffc00
#define AVR32_PG_VALID		0x00000200
#define AVR32_PG_INSTR		0x00000100
#define AVR32_PG_ASID		0x000000ff

#define AVR32_PTE_VALID		0x80000000
#define AVR32_PTE_WIRED		0x40000000
#define AVR32_PG_CACHED		0x00000200
#define AVR32_PG_GLOBAL		0x00000100
#define AVR32_PG_BUFF		0x00000080
#define AVR32_PG_ACCESS		0x00000070
#define AVR32_PG_SIZE		0x0000000c
#define AVR32_PG_DIRTY		0x00000002
#define AVR32_PG_WTHRU		0x00000001
#define AVR32_PG_SIZE_4K	0x00000004
/*
 * Byte offsets related to struct lwp.
 */
#define L_ADDR		24	/* offsetof(struct lwp, l_addr) */
#define L_MD		28	/* offsetof(struct lwp, l_md) */

/*
 * Byte offsets related to struct mdlwp.
 */
#define MD_UPTE_0	8	/* offsetof(struct mdlwp, md_upte[0]) */
#define MD_UPTE_1	12	/* offsetof(struct mdlwp, md_upte[1]) */

#define L_MD_UPTE_0	36
#define L_MD_UPTE_1	40

/*
 * Byte offsets related to struct pcb.
 */
#define U_PCB_CONTEXT	0	/* offsetof(struct pcb, pcb_context) */
#define U_PCB_ONFAULT	64	/* offsetof(struct pcb, pcb_onfault) */ 

/*
 * Byte offsets related to pcb_context.
 */
#define SF_REG_SR	0
#define SF_REG_LR	4
#define SF_REG_SP	8
#define SF_REG_R7	12
#define SF_REG_R6	16
#define SF_REG_R5	20
#define SF_REG_R4	24
#define SF_REG_R3	28
#define SF_REG_R2	32
#define SF_REG_R1	36
#define SF_REG_R0	40

#define KERNFRAME_SIZ	(8 + 16 * 4 + 8)/* sizeof(struct kernframe) */

#define TF_BASE	8	/* offsetof(struct kernframe, cf_frame) */

#define FRAME_SIZ	( 16 * 4 + 8)	/* sizeof (struct frame)  */

#define L_PROC	292 	/* offsetof(struct lwp, l_proc) */
#define P_MD_SYSCALL 484	/* offsetof(struct proc, p_md.md_syscall) */
