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

#ifndef _AVR32_PMAP_H_
#define _AVR32_PMAP_H_

/*
 * The user address space is 2Gb (0x0 - 0x80000000).
 * User programs are laid out in memory as follows:
 *                      address
 *      USRTEXT         0x00001000
 *      USRDATA         USRTEXT + text_size
 *      USRSTACK        0x7FFFFFFF
 *
 * The user address space is mapped using a two level structure where
 * virtual address bits 30..22 are used to index into a segment table which
 * points to a page worth of PTEs (4096 page can hold 1024 PTEs).
 * Bits 21..12 are then used to index a PTE which describes a page within
 * a segment.
 *
 * The wired entries in the TLB will contain the following:
 *      0-1     (UPAGES)        for curproc user struct and kernel stack.
 *
 * Note: The kernel doesn't use the same data structures as user programs.
 * All the PTE entries are stored in a single array in Sysmap which is
 * dynamically allocated at boot time.
 */

#define avr32_trunc_seg(x)	((vaddr_t)(x) & ~SEGOFSET)
#define pmap_segmap(m, v)	((m)->pm_segtab->seg_tab[((v) >> SEGSHIFT)])

#define PMAP_SEGTABSIZE		512

struct segtab {
	union pt_entry	*seg_tab[PMAP_SEGTABSIZE];
};

/*
 * Machine dependent pmap structure.
 */
typedef struct pmap {
	int			pm_count;	/* pmap reference count */
	kmutex_t    		pm_lock;	/* lock on pmap */
	struct pmap_statistics	pm_stats;	/* pmap statistics */
	unsigned		pm_asid;	/* TLB address space tag */
	unsigned		pm_asidgen;	/* its generation number */
	struct segtab		*pm_segtab;	/* pointers to pages of PTEs */
} *pmap_t;

extern struct pmap kernel_pmap_store;

/*
 * For each struct vm_page, there is a list of all currently valid virtual
 * mappings of that page.  An entry is a pv_entry_t, the list is pv_table.
 * XXX really should do this as a part of the higher level code.
 */
typedef struct pv_entry {
	struct pv_entry	*pv_next;	/* next pv_entry */
	struct pmap	*pv_pmap;	/* pmap where mapping lies */
	vaddr_t		pv_va;		/* virtual address for mapping */
	int		pv_flags;	/* some flags for the mapping */
} *pv_entry_t;

#define PV_UNCACHED     0x0001          /* page is mapped uncached */
#define PV_MODIFIED     0x0002          /* page has been modified */
#define PV_REFERENCED   0x0004          /* page has been recently referenced */

#define pmap_kernel()		(&kernel_pmap_store)
#define pmap_wired_count(pmap)	((pmap)->pm_stats.wired_count)
#define pmap_resident_count(pmap) ((pmap)->pm_stats.resident_count)

#define pmap_update(pmap)	/* nothing (yet) */
#define pmap_phys_address(x)	avr32_ptob(x)
 
static __inline void
pmap_remove_all(struct pmap *pmap)
{
        /* Nothing. */
}

/*
 *      Bootstrap the system enough to run with virtual memory.
 */
void    pmap_bootstrap(void);

void    pmap_set_modified(paddr_t);

#define PMAP_STEAL_MEMORY	/* enable pmap_steal_memory() */

/*
 * Alternate mapping hooks for pool pages.  Avoids thrashing the TLB.
 */
vaddr_t avr32_map_poolpage(paddr_t);
paddr_t avr32_unmap_poolpage(vaddr_t);
#define PMAP_MAP_POOLPAGE(pa)	avr32_map_poolpage(pa)
#define PMAP_UNMAP_POOLPAGE(va)	avr32_unmap_poolpage(va)

#endif /* !_AVR32_PMAP_H_ */
