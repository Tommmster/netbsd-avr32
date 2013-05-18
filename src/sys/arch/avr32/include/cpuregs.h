/*	$NetBSD: cpuregs.h,v 1.74 2008/02/19 11:26:40 simonb Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell and Rick Macklem.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)machConst.h 8.1 (Berkeley) 6/10/93
 *
 * machConst.h --
 *
 *	Machine dependent constants.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 *
 * from: Header: /sprite/src/kernel/mach/ds3100.md/RCS/machConst.h,
 *	v 9.2 89/10/21 15:55:22 jhh Exp	 SPRITE (DECWRL)
 * from: Header: /sprite/src/kernel/mach/ds3100.md/RCS/machAddrs.h,
 *	v 1.2 89/08/15 18:28:21 rab Exp	 SPRITE (DECWRL)
 * from: Header: /sprite/src/kernel/vm/ds3100.md/RCS/vmPmaxConst.h,
 *	v 9.1 89/09/18 17:33:00 shirriff Exp  SPRITE (DECWRL)
 */

#ifndef _AVR32_CPUREGS_H_
#define	_AVR32_CPUREGS_H_

#include <sys/cdefs.h>		/* For __CONCAT() */

#if defined(_KERNEL_OPT)
#include "opt_cputype.h"
#endif

/*
 * Address space.
 * 32-bit AVR32 CPUS partition their 32-bit address space into the following segments:
 *
 * P0   0x00000000 - 0x7fffffff  User virtual mem,  mapped
 * P1   0x80000000 - 0x9fffffff  Physical memory, cached, unmapped
 * P2   0xa0000000 - 0xbfffffff  Physical memory, uncached, unmapped
 * P3   0xc0000000 - 0xdfffffff  kernel-virtual,  cached, mapped
 * P4   0xe0000000 - 0xffffffff	 system space, uncached, unmapped
 *
 * AVR32 physical memory is limited to 512Mbytes, which is
 * doubly mapped in P1 (cached) and P2 (uncached.)
 * Caching of mapped addresses is controlled by bits in the TLB entry.
 */

#define	AVR32_P0_START			0x00000000
#define	AVR32_P1_START			0x80000000
#define	AVR32_P2_START			0xa0000000
#define	AVR32_P3_START			0xc0000000
#define	AVR32_P4_START			0xe0000000
#define	AVR32_MAX_MEM_ADDR		0xc0000000

#define	AVR32_PHYS_MASK			0x1fffffff

#define	AVR32_P1_TO_PHYS(x)	((uintptr_t)(x) & AVR32_PHYS_MASK)
#define	AVR32_PHYS_TO_P1(x)	((uintptr_t)(x) | AVR32_P1_START)
#define AVR32_P2_TO_PHYS(x)	((uintptr_t)(x) & AVR32_PHYS_MASK)
#define	AVR32_PHYS_TO_P2(x)	((uintptr_t)(x) | AVR32_P2_START)

#define AVR32_TLB_NUM_PIDS	256
#define AVR32_TLB_PID_mask	0x000000ff
#define AVR32_TLB_PID_SHIFT	0

/*
 * The bits in the status register.
 */
#define AVR32_STATUS_GM		0x00010000 /* Global interrupt bit. */
#define AVR32_STATUS_IM0	0x00020000
#define AVR32_STATUS_IM1	0x00040000
#define AVR32_STATUS_IM2	0x00080000
#define AVR32_STATUS_IM3	0x00100000
#define AVR32_STATUS_IMx	0x001e0000 /* Local interrupt mask. */
#define AVR32_STATUS_IM		0x001f0000 /* Full Interrupt mask. */
#define AVR32_STATUS_EM		0x00200000 /* Exception mask. */

#define AVR32_STATUS_MMASK	0x01c00000
#define AVR32_STATUS_MUSER	0x00000000
#define AVR32_STATUS_MKERN	0x00400000
#define AVR32_STATUS_MXCPT	0x01800000

#define AVR32_USER_MODE(x) \
	(((x) & AVR32_STATUS_MMASK) == AVR32_STATUS_MUSER)

/*
 * MMU control register.
 */
#define AVR32_MMUCR_DLA_MASK	0x00003f00
#define AVR32_MMUCR_DLA_SHIFT	8
#define AVR32_MMUCR_DLA(x)	((x) << AVR32_MMUCR_DLA_SHIFT)

#define AVR32_MMUCR_DRP_MASK	0x000fc000
#define AVR32_MMUCR_DRP_SHIFT	14
#define AVR32_MMUCR_DRP(x)	((x) << AVR32_MMUCR_DRP_SHIFT)

#define AVR32_MMUCR_SMMU	0x00000010 /* Enable segmentation */
#define AVR32_MMUCR_NF		0x00000008 /* TLB entry not found */
#define AVR32_MMUCR_TLB_INV	0x00000004 /* TLB invalidate bit */
#define AVR32_MMUCR_SHARED	0x00000002 /* Shared virtual space */
#define AVR32_MMUCR_PMMU	0x00000001 /* Enable paging */

/*
 * Configuration registers.
 */
#define AVR32_CONFIG0_CPU_ID_MASK   0xff000000
#define AVR32_CONFIG0_CPU_ID_SHIFT  24

#define AVR32_CONFIG0_ARCH_MASK     0x0000e000
#define AVR32_CONFIG0_ARCH_SHIFT    13

#define AVR32_CONFIG0_CPU_REV_MASK  0x000f0000
#define AVR32_CONFIG0_CPU_REV_SHIFT 16

#define AVR32_CONFIG0_FPU_MASK      0x00000040
#define AVR32_CONFIG0_FPU_SHIFT     6

#define AVR32_CONFIG0_MMUT_MASK     0x00000380
#define AVR32_CONFIG0_MMUT_SHIFT    7

#define EXTRACT_BITFIELD(reg, name) \
	(((reg) & (name ## _MASK)) >> (name ## _SHIFT))

#define AVR32_CONFIG0_CPU_ID(c0) \
	(((c0) & AVR32_CONFIG0_CPU_ID_MASK) >> AVR32_CONFIG0_CPU_ID_SHIFT)

#define AVR32_CONFIG0_CPU_REV(c0) \
	(((c0) & AVR32_CONFIG0_CPU_REV_MASK) >> AVR32_CONFIG0_CPU_REV_SHIFT)

#define AVR32_CONFIG0_ARCH_TYPE(c0) \
	(((c0) & AVR32_CONFIG0_ARCH_MASK) >> AVR32_CONFIG0_ARCH_SHIFT)

#define AVR32_CONFIG0_MMU_TYPE(c0) \
	(((c0) & AVR32_CONFIG0_MMUT_MASK) >> AVR32_CONFIG0_MMUT_SHIFT)

#define AVR32_CONFIG0_FPU(c0) \
	(((c0) & AVR32_CONFIG0_FPU_MASK) >> AVR32_CONFIG0_FPU_SHIFT)

#define AVR32_CONFIG1_ICACHE_WAYS_MASK  0x00001c00
#define AVR32_CONFIG1_ICACHE_WAYS_SHIFT 10

#define AVR32_CONFIG1_DCACHE_WAYS_MASK  0x00000007
#define AVR32_CONFIG1_DCACHE_WAYS_SHIFT 0

#define AVR32_CONFIG1_ICACHE_SETS_MASK  0x000f0000
#define AVR32_CONFIG1_ICACHE_SETS_SHIFT 16

#define AVR32_CONFIG1_DCACHE_SETS_MASK  0x000003c0
#define AVR32_CONFIG1_DCACHE_SETS_SHIFT 6

#define AVR32_CONFIG1_ICACHE_LINE_SIZE_MASK  0x0000e000
#define AVR32_CONFIG1_ICACHE_LINE_SIZE_SHIFT 13

#define AVR32_CONFIG1_DCACHE_LINE_SIZE_MASK  0x00000038
#define AVR32_CONFIG1_DCACHE_LINE_SIZE_SHIFT 3

#define AVR32_CONFIG1_DMMU_SZ_MASK  0x03f00000
#define AVR32_CONFIG1_DMMU_SZ_SHIFT 20

#define AVR32_CONFIG1_ICACHE_WAYS(c1) \
	EXTRACT_BITFIELD(c1, AVR32_CONFIG1_ICACHE_WAYS)

#define AVR32_CONFIG1_ICACHE_SETS(c1) \
	EXTRACT_BITFIELD(c1, AVR32_CONFIG1_ICACHE_SETS)

#define AVR32_CONFIG1_ICACHE_LINE_SIZE(c1) \
	EXTRACT_BITFIELD(c1, AVR32_CONFIG1_ICACHE_LINE_SIZE)

#define AVR32_CONFIG1_DCACHE_WAYS(c1) \
	EXTRACT_BITFIELD(c1, AVR32_CONFIG1_DCACHE_WAYS)

#define AVR32_CONFIG1_DCACHE_SETS(c1) \
	EXTRACT_BITFIELD(c1, AVR32_CONFIG1_DCACHE_SETS)

#define AVR32_CONFIG1_DCACHE_LINE_SIZE(c1) \
	EXTRACT_BITFIELD(c1, AVR32_CONFIG1_DCACHE_LINE_SIZE)

#define AVR32_CONFIG1_DMMU_SZ(c1) \
	(1 + EXTRACT_BITFIELD(c1, AVR32_CONFIG1_DMMU_SZ))

#define AVR32_MMU_NONE		0
#define AVR32_MMU_SPLIT		1
#define AVR32_MMU_SHARED	2
#define AVR32_MMU_MPU		3

/*
 * AVR32 System registers.
 */
#define SR_STATUS	0x0000
#define SR_EVBA		0x0004
#define SR_CPUCR	0x000c
#define SR_ECR		0x0010
#define SR_RSR_SUP	0x0014
#define SR_RSR_INT0	0x0018
#define SR_RSR_INT1	0x001c
#define SR_RSR_INT2	0x0020
#define SR_RSR_INT3	0x0024
#define SR_RSR_EX	0x0028
#define SR_RAR_SUP	0x0034
#define SR_RAR_INT0	0x0038
#define SR_RAR_INT1	0x003c
#define SR_RAR_INT2	0x0040
#define SR_RAR_INT3	0x0044
#define SR_RAR_EX	0x0048
#define SR_CONFIG0	0x0100
#define SR_CONFIG1	0x0104
#define SR_COUNT	0x0108
#define SR_COMPARE	0x010c
#define SR_TLBEHI	0x0110
#define SR_TLBELO	0x0114
#define SR_PTBR		0x0118
#define SR_TLBEAR	0x011c
#define SR_MMUCR	0x0120
#define SR_BEAR		0x013c

/*
 * CPU processor IDs.
 */
#define AVR32_AP7000	0x01	/* Atmel AP7000 */

/*
 * AVR32 architecures.
 */
#define CPU_ARCH_AVR32A	0
#define CPU_ARCH_AVR32B	1

#endif /* !_AVR32_CPUREGS_H_ */
