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
 * Which system models were configured?
 */
#include "opt_atngw100.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/user.h>
#include <sys/mount.h>
#include <sys/kcore.h>
#include <sys/boot_flag.h>
#include <sys/ksyms.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/kprintf.h>
#include <sys/conf.h>
#include <sys/kcore.h>
#include <sys/kernel.h> /* for hz */
#include <uvm/uvm_extern.h>

#include <avr32/cache.h>
#include <avr32/locore.h>
#include <avr32/cpuregs.h>
#include <avr32/reg.h>
#include <machine/kcore.h>
#include <machine/cpu.h>
#include <machine/param.h>
#include <machine/vmparam.h>
#include <machine/sysconf.h>

int  cpu_mhz;
int  cpu_arch;
char cpu_model[128];
int  avr32_num_tlb_entries;

/* Our exported CPU info; we can have only one. */
struct cpu_info cpu_info_store;
struct lwp *avr32_curlwp = NULL;

/* Maps for VM objects. */
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

struct user *proc0paddr;

char machine[] = MACHINE;
char machine_arch[] = MACHINE_ARCH;

int physmem; /* max supported memory, changes to actual */

/*
 * These variables are needed by /sbin/savecore.
 */
u_int32_t dumpmag = 0x8fca0101;	/* magic number */
int	dumpsize = 0;		/* pages */
long	dumplo = 0;		/* blocks */

static void unimpl_bus_reset(void);
static void unimpl_cons_init(void);
static void unimpl_intr_establish(int, int, int, void *, void *);
static int  unimpl_memsize(void *);

struct platform platform = {
	unimpl_bus_reset,
	unimpl_cons_init,
	unimpl_intr_establish,
	unimpl_memsize,
	(struct platform_clock *)NULL,
};

phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int mem_cluster_cnt;

/*
 *  Ensure all platform vectors are always initialized.
 */
static void
unimpl_bus_reset(void)
{
	panic("unimpl_bus_reset");
}

static void
unimpl_cons_init(void)
{
	panic("unimpl_const_init");
}

static void
unimpl_intr_establish(int group, int line, int ilvl, void *ihnd, void *iarg)
{
	panic("unimpl_intr_establish");
}

static int
unimpl_memsize(void *first)
{
	panic("unimpl_memsize");
}

void
mach_init()
{
	int i;
	u_long first;
	u_long last;
	char *kernend;
	extern char edata[];
	extern char end[];

	/* Clear the BSS segment */
	kernend = (void *)avr32_round_page(end);
	memset(edata, 0, (kernend - edata));

	/* Board-specific initialization. */
#ifdef ATNGW100
	atngw100_init();
#else
#error need hardware platform defined at compile time.
#endif

	/* Early console initialization. */
	(*platform.cons_init)();

	/* Setup page size. */
	uvm_setpagesize();

	/*
	 * Initialize locore-function vector.
	 * Clear out the I and D caches.
	 */
	avr32_vector_init();

	/*
	 * Alloc u pages for proc0 stealing P1 memory.
	 */
	lwp0.l_addr = proc0paddr = (struct user *)kernend;
	lwp0.l_md.md_regs = (struct frame *)(kernend + USPACE) - 1;
	memset(lwp0.l_addr, 0, USPACE);
	kernend += USPACE;

	/* Find out how much memory is available. */
	physmem = (*platform.memsize)(kernend);

	/*
	 * Load the rest of the available pages into the VM system.
	 */
	for (i = 0, physmem = 0; i < mem_cluster_cnt; ++i) {
		first = mem_clusters[i].start;

		if (first < AVR32_P1_TO_PHYS(kernend))
			first = round_page(AVR32_P1_TO_PHYS(kernend));
		last = mem_clusters[i].start + mem_clusters[i].size;
		physmem += atop(mem_clusters[i].size);

		uvm_page_physload(atop(first), atop(last), atop(first),
		    atop(last), VM_FREELIST_DEFAULT);
	}

	/*
	 * Intialize the virtual memory system.
	 */
	pmap_bootstrap();
}

void
avr32_vector_init(void)
{
	extern char _evba[];
	uint32_t config0 = AVR32_MFSR(SR_CONFIG0);
	uint32_t config1 = AVR32_MFSR(SR_CONFIG1);
	uint32_t cpu_arch;
	uint32_t cpu_id;
	uint32_t mmu;

	cpu_info_store.ci_cpu_freq = 150 * 1000 * 1000;
	cpu_info_store.ci_cycles_per_hz =
		(cpu_info_store.ci_cpu_freq + hz/2) / hz ;

	/*
	 * XXX Set-up curlwp/curcpu again.  They may have been clobbered
	 * beween verylocore and here.
	 */
	lwp0.l_cpu = &cpu_info_store;
	cpu_info_store.ci_curlwp = &lwp0;
	curlwp = &lwp0;

	switch (cpu_id = AVR32_CONFIG0_CPU_ID(config0)) {
	case AVR32_AP7000:
		break;
	default:
		panic("CPU type (0x%x) not supported", cpu_id);
	}

	switch (cpu_arch = AVR32_CONFIG0_ARCH_TYPE(config0)) {
	case CPU_ARCH_AVR32B:
		break;
	default:
		panic("CPU architecture (0x%x) not supported", cpu_arch);
	}

	switch(mmu = AVR32_CONFIG0_MMU_TYPE(config0)) {
	case AVR32_MMU_SHARED:
		avr32_num_tlb_entries = AVR32_CONFIG1_DMMU_SZ(config1);
		break;
	default:
		panic("CPU MMU (0x%x) not supported", mmu);
	}

	if (avr32_num_tlb_entries < 1)
		panic("Unknown number of TLBs for CPU type 0x%x", cpu_id);

	/*
	 * Determine cache configuration and initialize our cache
	 * frobbing routine function pointers.
	 */
	avr32_config_cache();

	/* Install EVBA, exception vector base address register. */
	AVR32_MTSR(SR_EVBA, (unsigned)&_evba);

	/* Enable exceptions. */
	AVR32_MTSR(SR_STATUS, AVR32_MFSR(SR_STATUS) & ~AVR32_STATUS_EM);
}

/*
 * Machine-dependent startup code: allocate memory for variable-sized
 * tables.
 */
void
cpu_startup()
{
	vaddr_t minaddr, maxaddr;
	char pbuf[9];
#ifdef DEBUG
	extern int pmapdebug;           /* XXX */
	int opmapdebug = pmapdebug;
	
	pmapdebug = 0;
#endif

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf("%s%s", copyright, version);
	printf("%s\n", cpu_model);
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);
	
	minaddr = 0;
	
	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	                           VM_PHYS_SIZE, 0, false, NULL);
	
	/*
	 * No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use P segments to
	 * map those pages.
	 */

#ifdef DEBUG
	pmapdebug = opmapdebug;
#endif
	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
}

void
cpu_reboot(volatile int howto, char *bootsr)
{
	/*
	 * XXX Cannot panic() here due to recursion issues. This routine
	 * needs extensive overhaul, for now we just print a message and
	 * halt.
	 */
	printf("cpu_reboot: notyet\n");
	while (1)
		;
}

/*
 * cpu_dumpsize: calculate size of machine-dependent kernel core dump headers.
 */
int
cpu_dumpsize(void)
{
	int size;

	size = ALIGN(sizeof(kcore_seg_t)) + ALIGN(sizeof(cpu_kcore_hdr_t)) +
	    ALIGN(mem_cluster_cnt * sizeof(phys_ram_seg_t));
	if (roundup(size, dbtob(1)) != dbtob(1))
		return (-1);

	return (1);
}

/*
 * cpu_dump_mempagecnt: calculate size of RAM (in pages) to be dumped.
 */
u_long
cpu_dump_mempagecnt(void)
{
	u_long i, n;

	n = 0;
	for (i = 0; i < mem_cluster_cnt; i++)
		n += atop(mem_clusters[i].size);

	return (n);
}

/*
 * This is called by main to set dumplo and dumpsize.
 * Dumps always skip the first CLBYTES of disk space
 * in case there might be a disk label stored there.
 * If there is extra space, put dump at the end to
 * reduce the chance that swapping trashes it.
 */
void
cpu_dumpconf(void)
{
	const struct bdevsw *bdev;
	int nblks, dumpblks;	/* size of dump area */

	if (dumpdev == NODEV)
		goto bad;
	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL) {
		dumpdev = NODEV;
		goto bad;
	}
	if (bdev->d_psize == NULL)
		goto bad;
	nblks = (*bdev->d_psize)(dumpdev);
	if (nblks <= ctod(1))
		goto bad;

	dumpblks = cpu_dumpsize();
	if (dumpblks < 0)
		goto bad;
	dumpblks += ctod(cpu_dump_mempagecnt());

	/* If dump won't fit (incl. room for possible label), punt. */
	if (dumpblks > (nblks - ctod(1)))
		goto bad;

	/* Put dump at end of partition */
	dumplo = nblks - dumpblks;

	/* dumpsize is in page units, and doesn't include headers. */
	dumpsize = cpu_dump_mempagecnt();
	return;
bad:
	dumpsize = 0;
}

/*
 * Identify product revision IDs of CPU.
 */
void
cpu_identify(void)
{
	static const char * const label = "cpu0";	/* XXX */
	static const char * const waynames[] = {
		NULL,					/* 0 */
		"direct-mapped",			/* 1 */
		"2-way set-associative",		/* 2 */
		NULL,					/* 3 */
		"4-way set-associative",		/* 4 */
	};
#define nwaynames (sizeof(waynames) / sizeof(waynames[0]))
	const char *cpuname;
	unsigned config0 = AVR32_MFSR(SR_CONFIG0);
	unsigned cpu_id;

	switch(cpu_id = AVR32_CONFIG0_CPU_ID(config0)) {
	case AVR32_AP7000:
		cpuname = "Atmel AT32AP7000 SoC";
		break;
	default:
		cpuname = NULL;
		break;
	}

	if (cpuname != NULL)
		printf("%s (0x%x)", cpuname, cpu_id);
	else
		printf("unknown CPU type (0x%x)", cpu_id);
	
	printf(" Rev. %d", AVR32_CONFIG0_CPU_REV(config0));

	if (AVR32_CONFIG0_FPU(config0))
		printf(", with built-in FPU");
	else
		printf(", no FPU");

	printf("\n");

	KASSERT(avr32_picache_ways < nwaynames);
	KASSERT(avr32_pdcache_ways < nwaynames);

	if (avr32_picache_line_size)
		printf("%s: %dKB/%dB %s Instruction cache\n",
			label, avr32_picache_size / 1024,
			avr32_picache_line_size, waynames[avr32_picache_ways]);
	else
		printf("%s: no Instruction cache present\n", label);

	if (avr32_pdcache_line_size)
		printf("%s: %dKB/%dB %s Data cache\n",
			label, avr32_pdcache_size / 1024,
			avr32_pdcache_line_size, waynames[avr32_pdcache_ways]);
	else
		printf("%s: no Data cache present\n", label);

	/* 
	 * Refer to avr32_vector_init() for the TLB detection logic.
	 */
	printf("%s: unified TLB, %d entries\n",
		label, avr32_num_tlb_entries);
}

void
cpu_need_resched(struct cpu_info *ci, int flags)
{
	aston(ci->ci_data.cpu_onproc);
	ci->ci_want_resched = 1;
}

void
cpu_idle(void)
{
	/* XXXAVR32 Needs further work */
	while (!curcpu()->ci_want_resched)
		;
}

bool
cpu_intr_p(void)
{
	return curcpu()->ci_idepth != 0;
}

void
cpu_need_proftick(struct lwp *l)
{
	panic("cpu_need_proftick: notyet");
}

void
cpu_getmcontext(struct lwp *l, mcontext_t *mcp, unsigned int *flags)
{	
	panic("cpu_getmcontext: notyet");
}

int
cpu_setmcontext(struct lwp *l, const mcontext_t *mcp, unsigned int flags)
{
	struct frame *f = (struct frame *)l->l_md.md_regs;
	const __greg_t *gr = mcp->__gregs;
	struct proc *p = l->l_proc;

	/* Restore register context, if any. */
	if (flags & _UC_CPU) {
		/* Save register context. */
		/* XXX:  Do we validate the addresses?? */
		memcpy(&f->f_regs[_R_PC], &gr[_R_PC],
		       sizeof(avr32_reg_t) * 16);

		/* Do not restore SR. */
	}

	mutex_enter(p->p_lock);
	if (flags & _UC_SETSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	if (flags & _UC_CLRSTACK)
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;
	mutex_exit(p->p_lock);

	return (0);
}

void
setregs(struct lwp *l, struct exec_package *pack, u_long stack)
{
	struct frame *f = (struct frame *)l->l_md.md_regs;

	memset(f, 0, sizeof(struct frame));
	f->f_regs[_R_SP] = (int)stack;
	f->f_regs[_R_PC] = (int)pack->ep_entry & ~1;
	l->l_md.md_ss_addr = 0;
}

void
startlwp(void *args)
{
	panic("startlwp: notyet");
}

void
consinit(void)
{
}
