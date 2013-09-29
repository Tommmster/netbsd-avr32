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

#ifndef _AVR32_PTE_H_
#define _AVR32_PTE_H_

#ifndef _LOCORE
struct avr32_pte {
unsigned int pfn:22,	/* PFN */
	pg_c:1,		/* Cacheable */
	pg_g:1,		/* Global: Ignore asid bit */
	pg_b:1,		/* Buffereable */
	pg_ap:3,	/* Access control bits */
	pg_sz:2,	/* Page size */
	pg_d:1,		/* Dirty bit */
	pg_w:1;		/* Write through */
};
#endif /* !_LOCORE */

#define AVR32_PG_FRAME		0x3ffff000
#define AVR32_PG_VPN		0xfffff000
#define AVR32_PG_VALID		0x00000200
#define AVR32_PG_NVALID		0X00000000
#define AVR32_PG_INSTR		0x00000100
#define AVR32_PG_ASID		0x000000ff

#define AVR32_PTE_INVALID	0x00000000 /* SW */
#define AVR32_PTE_VALID		0x80000000 /* SW */
#define AVR32_PTE_WIRED		0x40000000 /* SW */
#define AVR32_PG_UNCACHED	0x00000000
#define AVR32_PG_CACHED		0x00000200
#define AVR32_PG_GLOBAL		0x00000100
#define AVR32_PG_BUFF		0x00000080
#define AVR32_PG_ACCESS		0x00000070
#define AVR32_PG_SIZE		0x0000000c
#define AVR32_PG_DIRTY		0x00000002
#define AVR32_PG_WTHRU		0x00000001

/* XXXAVR32 Rename to KRO, RKW, etc */
#define AVR32_PG_ACCESS_RO	0x00000000
#define AVR32_PG_ACCESS_RX	0x00000010
#define AVR32_PG_ACCESS_RW	0x00000020
#define AVR32_PG_ACCESS_RWX	0x00000030

#define AVR32_PG_ACCESS_URO	0x00000040
#define AVR32_PG_ACCESS_URX	0x00000050
#define AVR32_PG_ACCESS_URW	0x00000060
#define AVR32_PG_ACCESS_URWX	0x00000070

/* Write protected */
#define avr32_pte_ropage_bit() (AVR32_PG_ACCESS_RO)

/* Not clean, not write protected, not cached */
#define avr32_pte_rwncpage_bit() (AVR32_PG_ACCESS_RW | AVR32_PG_DIRTY | AVR32_PG_UNCACHED)

/* Clean, not write protected, not cacheable */
#define avr32_pte_cwncpage_bit() (AVR32_PG_ACCESS_RW | AVR32_PG_UNCACHED)

/* Not write protected, not clean */
#define avr32_pte_rwpage_bit() (AVR32_PG_ACCESS_RW | AVR32_PG_DIRTY | AVR32_PG_CACHED)

/* Not write protected, but clean */
#define avr32_pte_cwpage_bit() (AVR32_PG_ACCESS_RW | AVR32_PG_CACHED)

/* XXX */
#define avr32_pte_v(entry)	(entry & AVR32_PTE_VALID)	

#define AVR32_PG_SIZE_1K  0x00000000
#define AVR32_PG_SIZE_4K  0x00000004
#define AVR32_PG_SIZE_64K 0x00000008
#define AVR32_PG_SIZE_1M  0x0000000c

#define avr32_vaddr_to_tlbvpn(x) \
	((x) & AVR32_PG_VPN)

#define avr32_paddr_to_tlbpfn(x) \
	((x) & AVR32_PG_FRAME)

#define avr32_tlbpfn_to_paddr(x) \
	((x) & AVR32_PG_FRAME)

/* 
 * Kernel virtual address to page table entry and vice versa.
 */
#define  kvtopte(va) \
   (Sysmap + (((vaddr_t)(va) - VM_MIN_KERNEL_ADDRESS) >> PGSHIFT))

#define  ptetokv(pte) \
   ((((pt_entry_t *)(pte) - Sysmap) << PGSHIFT) + VM_MIN_KERNEL_ADDRESS)

#ifndef _LOCORE
typedef union pt_entry{
	struct avr32_pte pt_avr32_pte;	/* for copying, etc. */
	unsigned int pt_entry;		/* for getting to bits by name. */
} pt_entry_t;

extern   pt_entry_t *Sysmap;	/* kernel pte table */
extern   u_int Sysmapsize;	/* number of pte's in Sysmap */

#endif /* !_LOCORE */
#endif /* !_AVR32_PTE_H_ */
