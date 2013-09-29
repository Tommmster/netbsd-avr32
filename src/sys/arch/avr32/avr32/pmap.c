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

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/pool.h>
#include <sys/mutex.h>
#include <sys/socketvar.h>	/* XXX: for sock_loan_thresh */

#include <uvm/uvm.h>

#include <avr32/cache.h>
#include <avr32/pte.h>

#ifndef PMAP_PV_LOWAT
#define PMAP_PV_LOWAT   16
#endif

#define TABLE_LENGTH(t)	\
	(sizeof((t)) / sizeof((t)[0]))

const u_int protection_codes[2][8] = {
	/* kernel */
	{
	AVR32_PG_ACCESS_RO,	/* VM_PROT_NONE | VM_PROT_NONE | VM_PROT_NONE */
	AVR32_PG_ACCESS_RO,	/* VM_PROT_NONE | VM_PROT_NONE | VM_PROT_READ */
	AVR32_PG_ACCESS_RW,	/* VM_PROT_NONE | VM_PROT_WRITE | VM_PROT_NONE */
	AVR32_PG_ACCESS_RW,	/* VM_PROT_NONE | VM_PROT_WRITE | VM_PROT_READ */
	AVR32_PG_ACCESS_RX,	/* VM_PROT_EXEC | VM_PROT_NONE | VM_PROT_NONE */
	AVR32_PG_ACCESS_RX,	/* VM_PROT_EXEC | VM_PROT_NONE | VM_PROT_READ */
	AVR32_PG_ACCESS_RWX,	/* VM_PROT_EXEC | VM_PROT_WRITE | VM_PROT_NONE */
	AVR32_PG_ACCESS_RWX,	/* VM_PROT_EXEC | VM_PROT_WRITE | VM_PROT_READ */
	},
	/* user */
	{
	AVR32_PG_ACCESS_URO,	/* VM_PROT_NONE | VM_PROT_NONE | VM_PROT_NONE */
	AVR32_PG_ACCESS_URO,	/* VM_PROT_NONE | VM_PROT_NONE | VM_PROT_READ */
	AVR32_PG_ACCESS_URW,	/* VM_PROT_NONE | VM_PROT_WRITE | VM_PROT_NONE */
	AVR32_PG_ACCESS_URW,	/* VM_PROT_NONE | VM_PROT_WRITE | VM_PROT_READ */
	AVR32_PG_ACCESS_URX,	/* VM_PROT_EXEC | VM_PROT_NONE | VM_PROT_NONE */
	AVR32_PG_ACCESS_URX,	/* VM_PROT_EXEC | VM_PROT_NONE | VM_PROT_READ */
	AVR32_PG_ACCESS_URWX,	/* VM_PROT_EXEC | VM_PROT_WRITE | VM_PROT_NONE */
	AVR32_PG_ACCESS_URWX,	/* VM_PROT_EXEC | VM_PROT_WRITE | VM_PROT_READ */
	}
};
#define pte_prot(pm, prot) \
	(protection_codes[(pm) == pmap_kernel() ? 0 : 1][(prot)])

int pmap_pv_lowat = PMAP_PV_LOWAT;

bool pmap_initialized = false;

#define PAGE_IS_MANAGED(pa)   \
	(pmap_initialized == true && vm_physseg_find(atop(pa), NULL) != -1)

#define PMAP_IS_ACTIVE(pm)						\
	((pm) == pmap_kernel() || 					\
	 (pm) == curlwp->l_proc->p_vmspace->vm_map.pmap)

/* Forward function declarations */
void pmap_remove_pv(pmap_t, vaddr_t, struct vm_page *);
void pmap_asid_alloc(pmap_t pmap);
void pmap_enter_pv(pmap_t, vaddr_t, struct vm_page *, u_int *);

struct pmap kernel_pmap_store;
void pmap_pv_page_free(struct pool *, void *);
void *pmap_pv_page_alloc(struct pool *, int);

struct segtab	*segbase;
struct segtab   *free_segtab;           /* free list kept locally */

pt_entry_t *Sysmap;
unsigned Sysmapsize;

unsigned pmap_max_asid;       /* max ASID supported by the system */
unsigned pmap_curr_asid;      /* current ASID number */
unsigned pmap_next_asid;      /* next free ASID to use */
unsigned pmap_asid_generation;      /* current ASID generation */
#define PMAP_ASID_RESERVED 0

paddr_t avail_start; /* PA of first available physical page */
paddr_t avail_end;   /* PA of last available physical page */
vaddr_t virtual_end; /* VA of last avail page (end of kernel AS) */

struct pv_entry *pv_table;
int pv_table_npages;

/*
 * The pools from which pmap structures and sub-structures are allocated.
 */
struct pool pmap_pmap_pool;
struct pool pmap_pv_pool;

struct pool_allocator pmap_pv_page_allocator = {
	pmap_pv_page_alloc, pmap_pv_page_free, 0,
};

#define	pmap_pv_alloc()		pool_get(&pmap_pv_pool, PR_NOWAIT)
#define	pmap_pv_free(pv)	pool_put(&pmap_pv_pool, (pv))

/*
 * Misc. functions.
 */
static void mips_flushcache_allpvh(paddr_t);

extern void MachSetPID(int);

/*
 * Flush virtual addresses associated with a given physical address
 */
static void
avr32_flushcache_allpvh(paddr_t pa)
{
	struct vm_page *pg;
	struct pv_entry *pv;

	pg = PHYS_TO_VM_PAGE(pa);
	if (pg == NULL) {
		/* page is unmanaged */
#ifdef DIAGNOSTIC
		printf("avr32_flushcache_allpvh(): unmanged pa = %08lx\n",
		    (u_long)pa);
#endif
		return;
	}

	pv = pg->mdpage.pvh_list;

	/* No current mapping.  Cache was flushed by pmap_remove_pv() */
	if (pv->pv_pmap == NULL)
		return;

	/* Only one index is allowed at a time */
	if (avr32_cache_indexof(pa) != avr32_cache_indexof(pv->pv_va))
		avr32_dcache_wbinv_range_index(pv->pv_va, NBPG);
}


/*
 * pmap_pv_page_alloc:
 *
 * Allocate a page for the pv_entry pool.
 */
void *
pmap_pv_page_alloc(struct pool *pp, int flags)
{
	struct vm_page *pg;
	paddr_t phys;
	pv_entry_t pv;
	vaddr_t va;
	
	pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_USERESERVE);
	if (pg == NULL) {
	   return NULL;
	}
	phys = VM_PAGE_TO_PHYS(pg);
	va = AVR32_PHYS_TO_P1(phys);

	pg = PHYS_TO_VM_PAGE(phys);
	pv = pg->mdpage.pvh_list;
	if ((pv->pv_flags & PV_UNCACHED) == 0 &&
	    avr32_cache_indexof(pv->pv_va) != avr32_cache_indexof(va))
		avr32_dcache_wbinv_range_index(pv->pv_va, PAGE_SIZE);

 	return (void *)va;
}

/* 
 * pmap_pv_page_free:
 *
 * Free a pv_entry pool page.
 */
void
pmap_pv_page_free(struct pool *pp, void *v)
{
	uvm_pagefree(PHYS_TO_VM_PAGE(AVR32_P1_TO_PHYS((vaddr_t)v)));
	panic("pmap_pv_page_free: notyet");
}

void
pmap_bootstrap(void)
{
	vsize_t bufsz;
	/*
	* Compute the number of pages kmem_map will have.
	*/
	kmeminit_nkmempages();

	/*
	 * Figure out how many PTE's are necessary to map the kernel.
	 * We also reserve space for kmem_alloc_pageable() for vm_fork().
	 */
	
	/* Get size of buffer cache and set an upper limit */
	bufsz = buf_memcalc();
	buf_setvalimit(bufsz);
	
	Sysmapsize = (VM_PHYS_SIZE + (ubc_nwins << ubc_winshift) +
	    bufsz + 16 * NCARGS + pager_map_size) / NBPG +
	    (maxproc * UPAGES) + nkmempages;
	
	/*
	 * Initialize `FYI' variables.   Note we're relying on
	 * the fact that BSEARCH sorts the vm_physmem[] array
	 * for us.  Must do this before uvm_pageboot_alloc()
	 * can be called.
	 */
	avail_start = ptoa(vm_physmem[0].start);
	avail_end = ptoa(vm_physmem[vm_nphysseg - 1].end);
	virtual_end = VM_MIN_KERNEL_ADDRESS + Sysmapsize * NBPG;
	
	
	/*
	 * Now actually allocate the kernel PTE array (must be done
	 * after virtual_end is initialized).
	 */
	Sysmap = (pt_entry_t *)
	    uvm_pageboot_alloc(sizeof(pt_entry_t) * Sysmapsize);
	
	/* 
	 * Allocate memory for the pv_heads.  (A few more of the latter
	 * are allocated than are needed.)
	 *
	 * We could do this in pmap_init when we know the actual
	 * managed page pool size, but its better to use P1
	 * addresses rather than kernel virtual addresses mapped
	 * through the TLB.
	 */
	pv_table_npages = physmem;
	pv_table = (struct pv_entry *)
	    uvm_pageboot_alloc(sizeof(struct pv_entry) * pv_table_npages);
	
	/*
	 * Initialize the pools.
	 */
	pool_init(&pmap_pmap_pool, sizeof(struct pmap), 0, 0, 0, "pmappl",
	    &pool_allocator_nointr, IPL_NONE);
	pool_init(&pmap_pv_pool, sizeof(struct pv_entry), 0, 0, 0, "pvpl",
	    &pmap_pv_page_allocator, IPL_NONE);
	
	/*
	 * Initialize the kernel pmap.
	 */
	pmap_kernel()->pm_count = 1;
	pmap_kernel()->pm_asid = PMAP_ASID_RESERVED;
	pmap_kernel()->pm_asidgen = 0;
	
	pmap_max_asid = AVR32_TLB_NUM_PIDS;
	pmap_curr_asid = 0;
	pmap_next_asid = 1;
	pmap_asid_generation = 0;
	
	MachSetPID(0);
}

bool
pmap_extract(pmap_t pmap, vaddr_t va, paddr_t *pap)
{
	paddr_t pa;
	pt_entry_t *pte;
	
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_extract(%p, 0x%lx) -> ", pmap, va);
#endif
	if (pmap == pmap_kernel()) {
		if (va >= AVR32_P1_START && va < AVR32_P2_START) {
			pa = AVR32_P1_TO_PHYS(va);
			goto done;
		}
#ifdef DIAGNOSTIC
		else if (va >= AVR32_P2_START && va < AVR32_P3_START)
			panic("pmap_extract: P2 address 0x%lx", va);
#endif
		else
			pte = kvtopte(va);
	} else {
		if (!(pte = pmap_segmap(pmap, va))) {
#ifdef DEBUG
			if (pmapdebug & PDB_FOLLOW)
				printf("not in segmap\n");
#endif
			return false;
		}
		pte += (va >> PGSHIFT) & (NPTEPG - 1);
	}
	if (!(pte->pt_entry & AVR32_PTE_VALID)) {
#ifdef DEBUG
		if (pmapdebug & PDB_FOLLOW)
			printf("PTE not valid\n");
#endif
		return false;
	}
	pa = avr32_tlbpfn_to_paddr(pte->pt_entry) | (va & PGOFSET);
done:
	if (pap != NULL) {
		*pap = pa;
	}
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pa 0x%lx\n", (u_long)pa);
#endif
	return true;
}

/*
 *	Remove the given range of addresses from the specified map.
 *
 *	It is assumed that the start and end are properly
 *	rounded to the page size.
 */
void
pmap_remove(pmap_t pmap, vaddr_t sva, vaddr_t eva)
{
	struct vm_page *pg;
	vaddr_t nssva;
	pt_entry_t *pte;
	unsigned entry;
	unsigned asid, needflush;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_REMOVE|PDB_PROTECT))
		printf("pmap_remove(%p, %lx, %lx)\n", pmap, sva, eva);
	remove_stats.calls++;
#endif
	if (pmap == pmap_kernel()) {
		/* remove entries from kernel pmap */
#ifdef PARANOIADIAG
		if (sva < VM_MIN_KERNEL_ADDRESS || eva >= virtual_end)
			panic("pmap_remove: kva not in range");
#endif
		pte = kvtopte(sva);
		for (; sva < eva; sva += NBPG, pte++) {
			entry = pte->pt_entry;
			if (!(entry & AVR32_PTE_VALID))
				continue;
			if (entry & AVR32_PTE_WIRED)
				pmap->pm_stats.wired_count--;
			pmap->pm_stats.resident_count--;
			pg = PHYS_TO_VM_PAGE(avr32_tlbpfn_to_paddr(entry));
			if (pg)
				pmap_remove_pv(pmap, sva, pg);
			pte->pt_entry = 0;

#define AVR32_TBIS(va) \
	avr32_tbis((va) | AVR32_PG_VALID, AVR32_PG_GLOBAL | AVR32_PG_SIZE_4K)

			/*
			 * Flush the TLB for the given address.
			 */
			AVR32_TBIS(sva);
#ifdef DEBUG
			remove_stats.flushes++;

#endif
		}
		return;
	}

#ifdef PARANOIADIAG /* XXXAVR32 */
	if (eva > VM_MAXUSER_ADDRESS)
		panic("pmap_remove: uva not in range");
	if (PMAP_IS_ACTIVE(pmap)) {
		unsigned asid;

		__asm volatile("mfc0 %0,$10; nop" : "=r"(asid));
		asid = (MIPS_HAS_R4K_MMU) ? (asid & 0xff) : (asid & 0xfc0) >> 6;
		if (asid != pmap->pm_asid) {
			panic("inconsistency for active TLB flush: %d <-> %d",
			    asid, pmap->pm_asid);
		}
	}
#endif
	asid = pmap->pm_asid;
	needflush = (pmap->pm_asidgen == pmap_asid_generation);
	while (sva < eva) {
		nssva = avr32_trunc_seg(sva) + NBSEG;
		if (nssva == 0 || nssva > eva)
			nssva = eva;
		/*
		 * If VA belongs to an unallocated segment,
		 * skip to the next segment boundary.
		 */
		if (!(pte = pmap_segmap(pmap, sva))) {
			sva = nssva;
			continue;
		}
		/*
		 * Invalidate every valid mapping within this segment.
		 */
		pte += (sva >> PGSHIFT) & (NPTEPG - 1);
		for (; sva < nssva; sva += NBPG, pte++) {
			entry = pte->pt_entry;
			if (!(entry & AVR32_PTE_VALID))
				continue;
			if (entry & AVR32_PTE_WIRED)
				pmap->pm_stats.wired_count--;
			pmap->pm_stats.resident_count--;
			pg = PHYS_TO_VM_PAGE(avr32_tlbpfn_to_paddr(entry));
			if (pg)
				pmap_remove_pv(pmap, sva, pg);
			pte->pt_entry = 0; /* Invalidate pt entry */
			/*
			 * Flush the TLB for the given address.
			 */
			if (needflush) {
				AVR32_TBIS(sva | asid);
#ifdef DEBUG
				remove_stats.flushes++;
#endif
			}
		}
	}
}

void
pmap_copy(pmap_t dst_pmap, pmap_t src_pmap, vaddr_t dst_addr, vsize_t len, vaddr_t src_addr)
{
	panic("pamp_copy: notyet");
}

/*
 *	pmap_copy_page copies the specified page.
 */
void
pmap_copy_page(paddr_t src, paddr_t dst)
{
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_copy_page(%lx, %lx)\n", (u_long)src, (u_long)dst);
#endif
#ifdef PARANOIADIAG
	if (!(src < MIPS_MAX_MEM_ADDR))
		printf("pmap_copy_page(%lx) src nonphys\n", (u_long)src);
	if (!(dst < MIPS_MAX_MEM_ADDR))
		printf("pmap_copy_page(%lx) dst nonphys\n", (u_long)dst);
#endif

	/*
	 * If we have a virtually-indexed, physically-tagged cache,
	 * and no L2 cache to warn of aliased mappings, we must force an
	 * write-back of all L1 cache lines of the source physical address,
	 * irrespective of their virtual address (cache indexes).
	 * If we don't, our copy loop might read and copy stale DRAM
	 * footprint instead of the fresh (but dirty) data in a WB cache.
	 * XXX invalidate any cached lines of the destination PA
	 *     here also?
	 *
	 * It would probably be better to map the destination as a
	 * write-through no allocate to reduce cache thrash.
	 */
		/*XXX FIXME Not very sophisticated */
	avr32_flushcache_allpvh(src);
#if 0
		avr32_flushcache_allpvh(dst);
#endif

	avr32_pagecopy((void *)AVR32_PHYS_TO_P1(dst),
		       (void *)AVR32_PHYS_TO_P1(src));

	/*
	 * If we have a virtually-indexed, physically-tagged WB cache,
	 * and no L2 cache to warn of aliased mappings,	we must force a
	 * writeback of the destination out of the L1 cache.  If we don't,
	 * later reads (from virtual addresses mapped to the destination PA)
	 * might read old stale DRAM footprint, not the just-written data.
	 * XXX  Do we need to also invalidate any cache lines matching
	 *      the destination as well?
	 *
	 * XXXJRT -- This is totally disgusting.
	 */
	avr32_dcache_wbinv_range(AVR32_PHYS_TO_P1(src), PAGE_SIZE);
	avr32_dcache_wbinv_range(AVR32_PHYS_TO_P1(dst), PAGE_SIZE);
}

/*
 * 	pmap_page_protect:
 *
 * 	Lower the permission for all mappings to a given page.
 */
void
pmap_page_protect(struct vm_page *pg, vm_prot_t prot)
{
	pv_entry_t pv, npv;
	vaddr_t va;

#ifdef DEBUG
	if ((pmapdebug & (PDB_FOLLOW|PDB_PROTECT)) ||
	    (prot == VM_PROT_NONE && (pmapdebug & PDB_REMOVE)))
		printf("pmap_page_protect(%lx, %x)\n",
		    (u_long)VM_PAGE_TO_PHYS(pg), prot);
#endif
	switch (prot) {
	case VM_PROT_READ|VM_PROT_WRITE:
		panic("pmap_page_protect: VM_PROT_READ|VM_PROT_WRITE: notyet");
	case VM_PROT_ALL:
		panic("pmap_page_protect: VM_PROT_ALL: notyet");
		break;

	/* copy_on_write */
	case VM_PROT_READ:
		panic("pmap_page_protect: VM_PROT_READ: notyet");
	case VM_PROT_READ|VM_PROT_EXECUTE:
		panic("pmap_page_protect: VM_PROT_READ|VM_PROT_EXECUTE: notyet");
		pv = pg->mdpage.pvh_list;
		/*
		 * Loop over all current mappings setting/clearing as appropos.
		 */
		if (pv->pv_pmap != NULL) {
			for (; pv; pv = pv->pv_next) {
				va = pv->pv_va;
				pmap_protect(pv->pv_pmap, va, va + PAGE_SIZE,
				    prot);
				pmap_update(pv->pv_pmap);
			}
		}
		break;

	/* remove_all */
	default:
		pv = pg->mdpage.pvh_list;
		if (pv->pv_pmap != NULL) {
			for (npv = pv; npv; npv = npv->pv_next) {
				pmap_remove(npv->pv_pmap, npv->pv_va,
				    npv->pv_va + PAGE_SIZE);
				pmap_update(npv->pv_pmap);
			}
		}
	}
}

void
pmap_kremove(vaddr_t va, vsize_t len)
{
	pt_entry_t *pte;
	vaddr_t eva;
	u_int entry;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_REMOVE))
		printf("pmap_kremove(%lx, %lx)\n", va, len);
#endif

	pte = kvtopte(va);
	eva = va + len;
	for (; va < eva; va += PAGE_SIZE, pte++) {
		entry = pte->pt_entry;
		if (!(entry & AVR32_PTE_VALID)) {
			continue;
		}

		/* XXX XXX should also check icache virtual && EXEC mapping */
		avr32_dcache_wbinv_range(va, PAGE_SIZE);
		avr32_tbis(va | AVR32_PG_VALID, pte->pt_entry);
		pte->pt_entry = AVR32_PTE_INVALID | AVR32_PG_GLOBAL;
	}
}

void
pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot)
{
	u_int tlbelo;
	u_int tlbehi;
	u_int npte;
	pt_entry_t *pte;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_ENTER))
		printf("pmap_kenter_pa(%lx, %lx, %x)\n", va, (u_long)pa, prot);
#endif

	if (!PAGE_IS_MANAGED(pa))
		panic("pmap_kenter_pa: page not managed: %p", (void *) pa);

	tlbehi = avr32_vaddr_to_tlbvpn(va)
		| AVR32_PG_VALID;

	npte = avr32_paddr_to_tlbpfn(pa)
		| AVR32_PTE_VALID
		| AVR32_PTE_WIRED
		| AVR32_PG_SIZE_4K
		| AVR32_PG_GLOBAL
		| AVR32_PG_CACHED;
		
	npte |= pte_prot(pmap_kernel(), prot);
	if (prot & VM_PROT_WRITE)
		npte |= AVR32_PG_DIRTY;

	pte = kvtopte(va);
	KASSERT(!(pte->pt_entry & AVR32_PTE_VALID));
	pte->pt_entry = npte;

	tlbehi = avr32_vaddr_to_tlbvpn(va) | AVR32_PG_VALID;
	tlbelo = npte;
	MachTLBUpdate(tlbehi, tlbelo);
}

/*
 *	Make a previously active pmap (vmspace) inactive.
 */
void
pmap_deactivate(struct lwp *l)
{
	/* Nothing to do. */
}

/*
 * Clear the modify bits on the specified physical page.
 */
bool
pmap_clear_modify(struct vm_page *pg)
{
	struct pmap *pmap;
	struct pv_entry *pv;
	pt_entry_t *pte;
	u_int *attrp;
	vaddr_t va;
	unsigned asid;
	bool rv;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_clear_modify(%lx)\n", (u_long)VM_PAGE_TO_PHYS(pg));
#endif
	attrp = &pg->mdpage.pvh_attrs;
	rv = *attrp & PV_MODIFIED;
	*attrp &= ~PV_MODIFIED;
	if (!rv) {
		return rv;
	}
	pv = pg->mdpage.pvh_list;
	if (pv->pv_pmap == NULL) {
		return true;
	}

	/*
	 * remove write access from any pages that are dirty
	 * so we can tell if they are written to again later.
	 * flush the VAC first if there is one.
	 */

	for (; pv; pv = pv->pv_next) {
		pmap = pv->pv_pmap;
		va = pv->pv_va;
		if (pmap == pmap_kernel()) {
			pte = kvtopte(va);
			asid = 0;
		} else {
			pte = pmap_segmap(pmap, va);
			KASSERT(pte);
			pte += ((va >> PGSHIFT) & (NPTEPG - 1));
			asid = pmap->pm_asid;
		}
		if ((pte->pt_entry & AVR32_PG_DIRTY) == 0) {
			continue;
		}

		if (PMAP_IS_ACTIVE(pmap)) {
			avr32_dcache_wbinv_range(va, PAGE_SIZE);
		} else {
			avr32_dcache_wbinv_range_index(va, PAGE_SIZE);
		}

		pte->pt_entry &= ~AVR32_PG_DIRTY;
		if (pmap->pm_asidgen == pmap_asid_generation) {
			avr32_tbis(va | asid, pte->pt_entry);
		}
	}
	return true;
}

void
pmap_activate(struct lwp *l)
{	
	pmap_t pmap = l->l_proc->p_vmspace->vm_map.pmap;

	pmap_asid_alloc(pmap);
	if (l == curlwp) {
		segbase = pmap->pm_segtab;
		MachSetPID(pmap->pm_asid);
	}
}

void
pmap_zero_page(paddr_t phys)
{
	vaddr_t va = AVR32_PHYS_TO_P1(phys);

	memset((void*)va, 0, PAGE_SIZE);	
}

/*
 *	Insert the given physical page (p) at
 *	the specified virtual address (v) in the
 *	target physical map with the protection requested.
 *
 *	If specified, the page will be wired down, meaning
 *	that the related pte can not be reclaimed.
 *
 *	NB:  This is the only routine which MAY NOT lazy-evaluate
 *	or lose information.  That is, this routine must actually
 *	insert this page into the given map NOW.
 */
int
pmap_enter(pmap_t pmap, vaddr_t va, paddr_t pa, vm_prot_t prot, int flags)
{	
	pt_entry_t *pte;
	u_int npte;
	u_int tlbehi;
	u_int tlbelo;
	struct vm_page *pg, *mem;
	unsigned asid;
	bool wired = (flags & PMAP_WIRED) != 0;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_ENTER))
		printf("pmap_enter(%p, %lx, %lx, %x, %x)\n",
		    pmap, va, (u_long)pa, prot, wired);
#endif
#if defined(DEBUG) || defined(DIAGNOSTIC) || defined(PARANOIADIAG)
	if (pmap == pmap_kernel()) {
#ifdef DEBUG
		enter_stats.kernel++;
#endif /* DEBUG */
		if (va < VM_MIN_KERNEL_ADDRESS || va >= virtual_end)
			panic("pmap_enter: kva too big");
	} else {
#ifdef DEBUG
		enter_stats.user++;
#endif /* DEBUG */
		if (va >= VM_MAXUSER_ADDRESS)
			panic("pmap_enter: uva too big");
	}
#endif /* DEBUG || DIAGNOSTIC || PARANOIDADIAG */
#ifdef PARANOIADIAG

	if (!(prot & VM_PROT_READ))
		panic("pmap_enter: prot");
#endif /* PARANOIDDIAG */
	pg = PHYS_TO_VM_PAGE(pa);

	if (pg) {
		u_int *attrs = &pg->mdpage.pvh_attrs;

		/* Set page referenced/modified status based on flags */
		if (flags & VM_PROT_WRITE)
			*attrs |= PV_MODIFIED | PV_REFERENCED;
		else if (flags & VM_PROT_ALL)
			*attrs |= PV_REFERENCED;

		npte = pte_prot(pmap, prot) | AVR32_PG_CACHED;
		if (prot & VM_PROT_WRITE) {
			if (*attrs & PV_MODIFIED)
				npte |= AVR32_PG_DIRTY;
		}

#ifdef DEBUG
		enter_stats.managed++;
#endif /* DEBUG */
	} else {
		panic("pmap_enter: unmanaged page: notyet");
	}

	/*
	 * The only time we need to flush the cache is if we
	 * execute from a physical address and then change the data.
	 * This is the best place to do this.
	 * pmap_protect() and pmap_remove() are mostly used to switch
	 * between R/W and R/O pages.
	 * NOTE: we only support cache flush for read only text.
	 */
#ifdef MIPS1
	if ((!MIPS_HAS_R4K_MMU) && prot == (VM_PROT_READ | VM_PROT_EXECUTE)) {
		mips_icache_sync_range(MIPS_PHYS_TO_KSEG0(pa), PAGE_SIZE);
	}
#endif /* MIPS1 */

	if (pmap == pmap_kernel()) {
		if (pg)
			pmap_enter_pv(pmap, va, pg, &npte);

		/* enter entries into kernel pmap */
		pte = kvtopte(va);

		npte |= avr32_paddr_to_tlbpfn(pa)
		  	| AVR32_PG_VALID
		  	| AVR32_PG_CACHED
		  	| AVR32_PG_SIZE_4K
			| AVR32_PG_GLOBAL;
		if (wired) {
			pmap->pm_stats.wired_count++;
			npte |= AVR32_PTE_WIRED;
		}
		if ((pte->pt_entry & AVR32_PTE_VALID) &&
		    avr32_tlbpfn_to_paddr(pte->pt_entry) != pa) {
			pmap_remove(pmap, va, va + NBPG);
#ifdef DEBUG
			enter_stats.mchange++;
#endif
		}
		if (!(pte->pt_entry & AVR32_PTE_VALID))
			pmap->pm_stats.resident_count++;
		pte->pt_entry = npte;

		/*
		 * Update the same virtual address entry.
		 */
		tlbehi = avr32_vaddr_to_tlbvpn(va) | AVR32_PG_VALID
			| pmap->pm_asid;
		tlbelo = npte;
		MachTLBUpdate(tlbehi, tlbelo);
		return 0;
	}

	/*
	 * This address is not in pmap_kernel. Make sure it is not within KVM.
	 */
	if (va >= VM_MAXUSER_ADDRESS)
		panic("pmap_enter: uva not in range");

	if (!(pte = pmap_segmap(pmap, va))) {
		mem = uvm_pagealloc(NULL, 0, NULL,
				    UVM_PGA_USERESERVE|UVM_PGA_ZERO);
		if (mem == NULL) {
			if (flags & PMAP_CANFAIL)
				return ENOMEM;
			panic("pmap_enter: cannot allocate segmap");
		}

		pmap_segmap(pmap, va) = pte =
			(pt_entry_t *)AVR32_PHYS_TO_P1(VM_PAGE_TO_PHYS(mem));
#ifdef PARANOIADIAG
	    {
		int i;
		for (i = 0; i < NPTEPG; i++) {
			if ((pte+i)->pt_entry)
				panic("pmap_enter: new segmap not empty");
		}
	    }
#endif
	}

	/* Done after case that may sleep/return. */
	if (pg)
		pmap_enter_pv(pmap, va, pg, &npte);

	pte += (va >> PGSHIFT) & (NPTEPG - 1);

	/*
	 * Now validate mapping with desired protection/wiring.
	 * Assume uniform modified and referenced status for all
	 * pages in a MACH page.
	 */
	npte |= avr32_paddr_to_tlbpfn(pa)
		| AVR32_PTE_VALID
		| AVR32_PG_CACHED
		| AVR32_PG_SIZE_4K;

	if (wired) {
		pmap->pm_stats.wired_count++;
		npte |= AVR32_PTE_WIRED;
	}
#ifdef DEBUG
	if (pmapdebug & PDB_ENTER) {
		printf("pmap_enter: new pte %x", npte);
		if (pmap->pm_asidgen == pmap_asid_generation)
			printf(" asid %d", pmap->pm_asid);
		printf("\n");
	}
#endif

#ifdef PARANOIADIAG
	if (PMAP_IS_ACTIVE(pmap)) {
		panic("pmap_is_active");
		unsigned asid;

		__asm volatile("mfc0 %0,$10; nop" : "=r"(asid));
		asid = (MIPS_HAS_R4K_MMU) ? (asid & 0xff) : (asid & 0xfc0) >> 6;
		if (asid != pmap->pm_asid) {
			panic("inconsistency for active TLB update: %d <-> %d",
			    asid, pmap->pm_asid);
		}
	}
#endif

	asid = pmap->pm_asid;
	if ((pte->pt_entry & AVR32_PTE_VALID) &&
	    avr32_tlbpfn_to_paddr(pte->pt_entry) != pa) {
		pmap_remove(pmap, va, va + NBPG);
#ifdef DEBUG
		enter_stats.mchange++;
#endif
	}

	if (!(pte->pt_entry & AVR32_PTE_VALID))
		pmap->pm_stats.resident_count++;
	pte->pt_entry = npte;

	if (pmap->pm_asidgen == pmap_asid_generation) {
		tlbehi = avr32_vaddr_to_tlbvpn(va) | AVR32_PG_VALID | asid;	
		tlbelo = npte;
		MachTLBUpdate(tlbehi, tlbelo);
	}

	if (prot == (VM_PROT_READ | VM_PROT_EXECUTE)) {
#ifdef DEBUG
		if (pmapdebug & PDB_ENTER)
			printf("pmap_enter: flush I cache va %lx (%lx)\n",
			    va - NBPG, (u_long)pa);
#endif
		/* XXXJRT */
		avr32_icache_sync_range_index(va, PAGE_SIZE);
	}

	return 0;
}

void
pmap_reference(pmap_t pmap)
{
	if (pmap != NULL)
		pmap->pm_count++;
}

void
pmap_virtual_space(vaddr_t *vstartp, vaddr_t *vendp)
{
	*vstartp = VM_MIN_KERNEL_ADDRESS;   /* kernel is in the P1 segment */
	*vendp = trunc_page(virtual_end);   /* XXX need pmap_growkernel() */
}

/*
 * Bootstrap memory allocator (alternative to vm_bootstrap_steal_memory()).
 * This function allows for early dynamic memory allocation until the virtual
 * memory system has been bootstrapped.  After that point, either kmem_alloc
 * or malloc should be used.  This function works by stealing pages from the
 * (to be) managed page pool, then implicitly mapping the pages (by using
 * their P1 segment  addresses) and zeroing them.
 *
 * It may be used once the physical memory segments have been pre-loaded
 * into the vm_physmem[] array.  Early memory allocation MUST use this
 * interface!  This cannot be used after vm_page_startup(), and will
 * generate a panic if tried.
 *
 * Note that this memory will never be freed, and in essence it is wired
 * down.
 *
 * We must adjust *vstartp and/or *vendp iff we use address space
 * from the kernel virtual address range defined by pmap_virtual_space().
 */
vaddr_t
pmap_steal_memory(vsize_t size, vaddr_t *vstartp, vaddr_t *vendp)
{
	int bank, x;
	u_int npgs;
	paddr_t pa;
	vaddr_t va;

	size = round_page(size);
	npgs = atop(size);

	for (bank = 0; bank < vm_nphysseg; bank++) {
		if (uvm.page_init_done == true)
			panic("pmap_steal_memory: called _after_ bootstrap");

		if (vm_physmem[bank].avail_start != vm_physmem[bank].start ||
		    vm_physmem[bank].avail_start >= vm_physmem[bank].avail_end)
			continue;

		if ((vm_physmem[bank].avail_end - vm_physmem[bank].avail_start)
		    < npgs)
			continue;

		/*
		 * There are enough pages here; steal them!
		 */
		pa = ptoa(vm_physmem[bank].avail_start);
		vm_physmem[bank].avail_start += npgs;
		vm_physmem[bank].start += npgs;

		/*
		 * Have we used up this segment?
		 */
		if (vm_physmem[bank].avail_start == vm_physmem[bank].end) {
			if (vm_nphysseg == 1)
				panic("pmap_steal_memory: out of memory!");

			/* Remove this segment from the list. */
			vm_nphysseg--;
			for (x = bank; x < vm_nphysseg; x++) {
				/* structure copy */
				vm_physmem[x] = vm_physmem[x + 1];
			}
		}
		va = AVR32_PHYS_TO_P1(pa);
		memset((void *)va, 0, size);

		return va;
	}

	/*
	 * If we got here, there was no memory left.
	 */
	panic("pmap_steal_memory: no memory to steal");
}

void
pmap_unwire(pmap_t pmap, vaddr_t va)
{
	panic("pmap_unwire: notyet");
}

void
pmap_init(void)
{
	vsize_t		s;
	int		bank, i;
	pv_entry_t	pv;
	uint32_t	mmucr;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_INIT))
		printf("pmap_init()\n");
#endif

	/*
	 * Memory for the pv entry heads has
	 * already been allocated.  Initialize the physical memory
	 * segments.
	 */
	pv = pv_table;
	for (bank = 0; bank < vm_nphysseg; bank++) {
		s = vm_physmem[bank].end - vm_physmem[bank].start;
		for (i = 0; i < s; i++)
			vm_physmem[bank].pgs[i].mdpage.pvh_list = pv++;
	}

	/* 
	 * Set a low water mark on the pv_entry pool, so that we are
	 * more likely to have these around even in extreme memory
	 * starvation.
	 */
	pool_setlowat(&pmap_pv_pool, pmap_pv_lowat);

	/*
	 * Configure the virtual memory machinery: enable the segmentation
	 * feature of the MMU in order to instrument the P0 ... P4 memory
	 * segments. Turn on paging and enforce private virtual memory mode,
	 * i.e., enforce ASID checking when the global bit of the TLB entry
	 * has not been asserted.
	 */
	mmucr = AVR32_MMUCR_PMMU	/* Enable paging */
		| AVR32_MMUCR_TLB_INV	/* Invalidate the TLB */
		| AVR32_MMUCR_SMMU	/* Turn on Px segmentation */
		| AVR32_MMUCR_DLA(2)	/* Wire down the first two TLB items */
		| AVR32_MMUCR_DRP(2);	/* TLB replacement pointer */
	AVR32_MTSR(SR_MMUCR, mmucr);

	/*
	 * Now it is safe to enable pv entry recording.
	 */
	pmap_initialized = true;
}

void
pmap_destroy(pmap_t pmap)
{
	int count;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_CREATE))
		printf("pmap_destroy(%p)\n", pmap);
#endif
	count = --pmap->pm_count;
	if (count > 0)
		return;

	if (pmap->pm_segtab) {
		pt_entry_t *pte;
		int i;
#ifdef PARANOIADIAG
		int j;
#endif

		for (i = 0; i < PMAP_SEGTABSIZE; i++) {
			/* get pointer to segment map */
			pte = pmap->pm_segtab->seg_tab[i];
			if (!pte)
				continue;
#ifdef PARANOIADIAG
			for (j = 0; j < NPTEPG; j++) {
				if ((pte + j)->pt_entry)
					panic("pmap_destroy: segmap not empty");
			}
#endif
			/*
			 * From the original MIPS source file:
			 *
			 * The pica pmap.c flushed the segmap pages here. I'm
			 * not sure why, but I suspect it's because the page(s)
			 * were being accessed by KSEG0 (P1, cached) addresses
			 * and may cause cache coherency problems when the page
			 * is reused with KSEG2 (P3, mapped) addresses. This
			 * may cause problems on machines without VCED/VCEI.
			 */
			avr32_dcache_inv_range((vaddr_t)pte, PAGE_SIZE);
			uvm_pagefree(PHYS_TO_VM_PAGE(AVR32_P1_TO_PHYS(pte)));

			pmap->pm_segtab->seg_tab[i] = NULL;
		}
		*(struct segtab **)pmap->pm_segtab = free_segtab;
		free_segtab = pmap->pm_segtab;
		pmap->pm_segtab = NULL;
	}

	pool_put(&pmap_pmap_pool, pmap);
}

/*
 *	Create and return a physical map.
 *
 *	If the size specified for the map
 *	is zero, the map is an actual physical
 *	map, and may be referenced by the
 *	hardware.
 *
 *	If the size specified is non-zero,
 *	the map will be used in software only, and
 *	is bounded by that size.
 */
pmap_t
pmap_create(void)
{
	pmap_t pmap;
	int i;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_CREATE))
		printf("pmap_create()\n");
#endif

	pmap = pool_get(&pmap_pmap_pool, PR_WAITOK);
	memset(pmap, 0, sizeof(*pmap));

	pmap->pm_count = 1;
	if (free_segtab) {
		pmap->pm_segtab = free_segtab;
		free_segtab = *(struct segtab **)free_segtab;
		pmap->pm_segtab->seg_tab[0] = NULL;
	} else {
		struct segtab *stp;
		struct vm_page *mem;

		do {
			mem = uvm_pagealloc(NULL, 0, NULL,
			    UVM_PGA_USERESERVE|UVM_PGA_ZERO);
			if (mem == NULL) {
				/*
				 * XXX What else can we do?  Could we
				 * XXX deadlock here?
				 */
				uvm_wait("pmap_create");
			}
		} while (mem == NULL);

		pmap->pm_segtab = stp =
		    (struct segtab *) AVR32_PHYS_TO_P1(VM_PAGE_TO_PHYS(mem));
		i = NBPG / sizeof(struct segtab);
		while (--i != 0) {
			stp++;
			*(struct segtab **)stp = free_segtab;
			free_segtab = stp;
		}
	}
#ifdef PARANOIADIAG
	for (i = 0; i < PMAP_SEGTABSIZE; i++)
		if (pmap->pm_segtab->seg_tab[i] != 0)
			panic("pmap_create: pm_segtab != 0");
#endif
	pmap->pm_asid = PMAP_ASID_RESERVED;
	pmap->pm_asidgen = pmap_asid_generation;

	return pmap;
}

/*
 *	Set the physical protection on the
 *	specified range of this map as requested.
 */
void
pmap_protect(pmap_t pmap, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{
	vaddr_t nssva;
	pt_entry_t *pte;
	unsigned entry;
	u_int p;
	unsigned asid, needupdate;
	u_int tlbehi, tlbelo;

	if (prot >= TABLE_LENGTH(protection_codes[0]))
		panic("pmap_protect: impossible protection");
#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_PROTECT))
		printf("pmap_protect(%p, %lx, %lx, %x)\n",
		    pmap, sva, eva, prot);
#endif
	if ((prot & VM_PROT_READ) == VM_PROT_NONE) {
		pmap_remove(pmap, sva, eva);
		return;
	}
	
	p = pte_prot(pmap, prot);

	if (pmap == pmap_kernel()) {
		/*
		 * Change entries in kernel pmap.
		 * This will trap if the page is writable (in order to set
		 * the dirty bit) even if the dirty bit is already set. The
		 * optimization isn't worth the effort since this code isn't
		 * executed much. The common case is to make a user page
		 * read-only.
		 */
#ifdef PARANOIADIAG
		if (sva < VM_MIN_KERNEL_ADDRESS || eva >= virtual_end)
			panic("pmap_protect: kva not in range");
#endif
		pte = kvtopte(sva);
		for (; sva < eva; sva += NBPG, pte++) {
			entry = pte->pt_entry;
			if (!(entry & AVR32_PTE_VALID))
				continue;
			if (entry & AVR32_PG_DIRTY)
				avr32_dcache_wb_range(sva, PAGE_SIZE);
			entry &= ~(AVR32_PG_ACCESS | AVR32_PG_DIRTY);
			entry |= p;
			pte->pt_entry = entry;

			tlbehi = avr32_vaddr_to_tlbvpn(sva) | pmap->pm_asid;
			tlbelo = entry;				
			MachTLBUpdate(tlbehi, tlbelo);
		}
		return;
	}

#ifdef PARANOIADIAG
	if (sva > VM_MAXUSER_ADDRESS)
		panic("pmap_protect: sva not in range");
	if (eva > VM_MAXUSER_ADDRESS)
		panic("pmap_protect: eva not in range");
	if (PMAP_IS_ACTIVE(pmap)) {
		unsigned asid;

		__asm volatile("mfc0 %0,$10; nop" : "=r"(asid));
		asid = (MIPS_HAS_R4K_MMU) ? (asid & 0xff) : (asid & 0xfc0) >> 6;
		if (asid != pmap->pm_asid) {
			panic("inconsistency for active TLB update: %d <-> %d",
			    asid, pmap->pm_asid);
		}
	}
#endif
	if (sva > VM_MAXUSER_ADDRESS)
		panic("pmap_protect: sva not in range");
	if (eva > VM_MAXUSER_ADDRESS)
		panic("pmap_protect: eva not in range");

	asid = pmap->pm_asid;
	needupdate = (pmap->pm_asidgen == pmap_asid_generation);
	while (sva < eva) {
		nssva = avr32_trunc_seg(sva) + NBSEG;
		if (nssva == 0 || nssva > eva)
			nssva = eva;
		/*
		 * If VA belongs to an unallocated segment,
		 * skip to the next segment boundary.
		 */
		if (!(pte = pmap_segmap(pmap, sva))) {
			sva = nssva;
			continue;
		}
		/*
		 * Change protection on every valid mapping within this segment.
		 */
		pte += (sva >> PGSHIFT) & (NPTEPG - 1);
		for (; sva < nssva; sva += NBPG, pte++) {
			entry = pte->pt_entry;
			if (!(entry & AVR32_PTE_VALID))
				continue;
			if (entry & AVR32_PG_DIRTY)
				avr32_dcache_wbinv_range_index(sva, PAGE_SIZE);
			entry &= ~(AVR32_PG_ACCESS | AVR32_PG_DIRTY);
			entry |= p;
			pte->pt_entry = entry;
			/*
			 * Update the TLB if the given address is in the cache.
			 */
			if (needupdate) {
				tlbehi = avr32_vaddr_to_tlbvpn(sva)
				       | AVR32_PG_VALID
				       | asid;
				tlbelo = entry;
				MachTLBUpdate(tlbehi, tlbelo);
			}
		}
	}
}

bool
pmap_clear_reference(struct vm_page *pg)
{
	panic("pmap_clear_reference: notyet");
	return false;
}

void
pmap_collect(pmap_t pmap)
{
	panic("pmap_collect: notyet");
}

bool
pmap_is_referenced(struct vm_page *pg)
{
	panic("pmap_is_referenced: notyet");
}

vaddr_t
avr32_pmap_map_poolpage(paddr_t pa)
{
	vaddr_t va;
	struct vm_page *pg;
	pv_entry_t pv;

	va = AVR32_PHYS_TO_P1(pa);

	pg = PHYS_TO_VM_PAGE(pa);
	pv = pg->mdpage.pvh_list;
	if ((pv->pv_flags & PV_UNCACHED) == 0 &&
	    avr32_cache_indexof(pv->pv_va) != avr32_cache_indexof(va))
		avr32_dcache_wbinv_range_index(pv->pv_va, PAGE_SIZE);

	return va;
}

paddr_t
avr32_pmap_unmap_poolpage(vaddr_t va)
{
	paddr_t pa;

	pa = AVR32_P1_TO_PHYS(va);
	avr32_dcache_inv_range(va, PAGE_SIZE);

	return pa;
}

/******************** pv_entry management ********************/

/*
 * Enter the pmap and virtual address into the
 * physical to virtual map table.
 */
void
pmap_enter_pv(pmap_t pmap, vaddr_t va, struct vm_page *pg, u_int *npte)
{
	pv_entry_t pv, npv;

	pv = pg->mdpage.pvh_list;
#ifdef DEBUG
	if (pmapdebug & PDB_ENTER)
		printf("pmap_enter: pv %p: was %lx/%p/%p\n",
		    pv, pv->pv_va, pv->pv_pmap, pv->pv_next);
#endif
again:
	if (pv->pv_pmap == NULL) {

		/*
		 * No entries yet, use header as the first entry
		 */

#ifdef DEBUG
		if (pmapdebug & PDB_PVENTRY)
			printf("pmap_enter: first pv: pmap %p va %lx\n",
			    pmap, va);
		enter_stats.firstpv++;
#endif
		pv->pv_va = va;
		pv->pv_flags &= ~PV_UNCACHED;
		pv->pv_pmap = pmap;
		pv->pv_next = NULL;
	} else {
		/*
		 * There is at least one other VA mapping this page.
		 * Check if they are cache index compatible.
		 */

		/*
		 * Remove the mapping from the pmap. When this address
		 * is touched again, the uvm will fault it in. Because
		 * of this, each page will only be mapped with one index
		 * at any given time.
		 */
		for (npv = pv; npv; npv = npv->pv_next) {
			if (avr32_cache_indexof(npv->pv_va) !=
			    avr32_cache_indexof(va)) {
				pmap_remove(npv->pv_pmap, npv->pv_va,
				    npv->pv_va + PAGE_SIZE);
				pmap_update(npv->pv_pmap);
				goto again;
			}
		}

		/*
		 * There is at least one other VA mapping this page.
		 * Place this entry after the header.
		 *
		 * Note: the entry may already be in the table if
		 * we are only changing the protection bits.
		 */
	
		for (npv = pv; npv; npv = npv->pv_next) {
			if (pmap == npv->pv_pmap && va == npv->pv_va) {
#ifdef PARANOIADIAG
				pt_entry_t *pte;
				unsigned entry;
	
				if (pmap == pmap_kernel())
					entry = kvtopte(va)->pt_entry;
				else {
					pte = pmap_segmap(pmap, va);
					if (pte) {
						pte += (va >> PGSHIFT) &
						    (NPTEPG - 1);
						entry = pte->pt_entry;
					} else
						entry = 0;
				}
				if (!(entry & AVR32_PTE_VALID) ||
				    avr32_tlbpfn_to_paddr(entry) !=
				    VM_PAGE_TO_PHYS(pg))
					printf(
		"pmap_enter: found va %lx pa %lx in pv_table but != %x\n",
					    va, (u_long)VM_PAGE_TO_PHYS(pg),
					    entry);
#endif
				return;
			}
		}
#ifdef DEBUG
		if (pmapdebug & PDB_PVENTRY)
			printf("pmap_enter: new pv: pmap %p va %lx\n",
			    pmap, va);
#endif
		npv = (pv_entry_t)pmap_pv_alloc();
		if (npv == NULL)
			panic("pmap_enter_pv: pmap_pv_alloc() failed");
		npv->pv_va = va;
		npv->pv_pmap = pmap;
		npv->pv_flags = pv->pv_flags;
		npv->pv_next = pv->pv_next;
		pv->pv_next = npv;
#ifdef DEBUG
		if (!npv->pv_next)
			enter_stats.secondpv++;
#endif
	}
}

/*
 * Remove a physical to virtual address translation.
 * If cache was inhibited on this page, and there are no more cache
 * conflicts, restore caching.
 * Flush the cache if the last page is removed (should always be cached
 * at this point).
 */
void
pmap_remove_pv(pmap_t pmap, vaddr_t va, struct vm_page *pg)
{
	pv_entry_t pv, npv;
	int last;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_PVENTRY))
		printf("pmap_remove_pv(%p, %lx, %lx)\n", pmap, va,
		    (u_long)VM_PAGE_TO_PHYS(pg));
#endif

	pv = pg->mdpage.pvh_list;

	/*
	 * If it is the first entry on the list, it is actually
	 * in the header and we must copy the following entry up
	 * to the header.  Otherwise we must search the list for
	 * the entry.  In either case we free the now unused entry.
	 */

	last = 0;
	if (pmap == pv->pv_pmap && va == pv->pv_va) {
		npv = pv->pv_next;
		if (npv) {

			/*
			 * Copy current modified and referenced status to
			 * the following entry before copying.
			 */

			npv->pv_flags |=
			    pv->pv_flags & (PV_MODIFIED | PV_REFERENCED);
			*pv = *npv;
			pmap_pv_free(npv);
		} else {
			pv->pv_pmap = NULL;
			last = 1;	/* Last mapping removed */
		}
#ifdef DEBUG
		remove_stats.pvfirst++;
#endif
	} else {
		for (npv = pv->pv_next; npv; pv = npv, npv = npv->pv_next) {
#ifdef DEBUG
			remove_stats.pvsearch++;
#endif
			if (pmap == npv->pv_pmap && va == npv->pv_va)
				break;
		}
		if (npv) {
			pv->pv_next = npv->pv_next;
			pmap_pv_free(npv);
		}
	}

	if (last != 0)
		avr32_dcache_wbinv_range_index(va, PAGE_SIZE);
}


/******************** misc. functions ********************/

/*
 * Allocate TLB address space tag (called ASID or TLBPID) and return it.
 * It takes almost as much or more time to search the TLB for a
 * specific ASID and flush those entries as it does to flush the entire TLB.
 * Therefore, when we allocate a new ASID, we just take the next number. When
 * we run out of numbers, we flush the TLB, increment the generation count
 * and start over. ASID zero is reserved for kernel use.
 */
void
pmap_asid_alloc(pmap_t pmap)
{

	if (pmap->pm_asid == PMAP_ASID_RESERVED ||
	    pmap->pm_asidgen != pmap_asid_generation) {
		if (pmap_next_asid == pmap_max_asid) {
			AVR32_TBIAP();
			pmap_asid_generation++; /* ok to wrap to 0 */
			pmap_next_asid = 1;	/* 0 means invalid */
		}
		pmap->pm_asid = pmap_next_asid++;
		pmap->pm_asidgen = pmap_asid_generation;
	}

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_TLBPID)) {
		if (curlwp)
			printf("pmap_asid_alloc: curlwp %d.%d '%s' ",
			    curlwp->l_proc->p_pid, curlwp->l_lid,
			    curlwp->l_proc->p_comm);
		else
			printf("pmap_asid_alloc: curlwp <none> ");
		printf("segtab %p asid %d\n", pmap->pm_segtab, pmap->pm_asid);
	}
#endif
}

/*
 *      pmap_set_modified:
 *
 *      Sets the page modified reference bit for the specified page.
 */
void
pmap_set_modified(paddr_t pa)
{
	struct vm_page *pg;

	pg = PHYS_TO_VM_PAGE(pa);
	
	if (pg == NULL) panic("pmap_set_modified: pg is NULL");/* XXXAVR32 */
	pg->mdpage.pvh_attrs |= PV_MODIFIED | PV_REFERENCED;
}
