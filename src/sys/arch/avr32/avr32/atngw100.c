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
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kcore.h>
#include <sys/device.h>

#include <avr32/locore.h>
#include <avr32/cpuregs.h>
#include <avr32/vmparam.h>
#include <avr32/sysconf.h>

#include <machine/at32intc.h>
#include <machine/at32usart.h>

static void atngw100_bus_reset(void);
static void atngw100_cons_init(void);
static void atngw100_intr_establish(int, int, int, void *, void *);
static int atngw100_memsize(void *);

extern char cpu_model[128];
extern int mem_cluster_cnt;
extern phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];

static const int atngw100_ipl2spl_table[] = {
	0,				/* IPL_NONE */
	AVR32_STATUS_IM0,		/* IPL_SOFTCLOCK */
	AVR32_STATUS_IM0,		/* IPL_SOFTSERIAL */
	AVR32_STATUS_IM1
		| AVR32_STATUS_IM0,	/* IPL_VM */
	AVR32_STATUS_IM2 
		| AVR32_STATUS_IM1
		| AVR32_STATUS_IM0,	/* IPL_SCHED */
	AVR32_STATUS_IM3 
		| AVR32_STATUS_IM2 
		| AVR32_STATUS_IM1 
		| AVR32_STATUS_IM0,	/* IPL_HIGH */
};

void
atngw100_init(void)
{
	platform.bus_reset = atngw100_bus_reset;
	platform.cons_init = atngw100_cons_init;
	platform.intr_establish = atngw100_intr_establish;
	platform.memsize = atngw100_memsize;
	ipl2spl_table = atngw100_ipl2spl_table;
	strcpy(cpu_model, "Atmel AT32AP7000 SoC");
}

/*
 * Initialize the memory system and I/O buses.
 */
static void
atngw100_bus_reset(void)
{
#if notyet
	/*
	 * Reset interrupts, clear any errors from newconf probes
	 */
#endif
}

static void
atngw100_cons_init(void)
{
	at32usart_cnattach();
}

static void
atngw100_intr_establish(int group, int line, int ilvl, void *ihnd, void *iarg)
{
	(void)at32intc_intr_establish(group, line, ilvl, ihnd, iarg);
}

static int
atngw100_memsize(void *first)
{
	int mem;

#ifndef ATNGW100_NR_PAGES
#define ATNGW100_NR_PAGES 8192
#endif

#ifndef ATNGW100_MEM_START
#define ATNGW100_MEM_START 0x10000000
#endif

	/* 32 MB = 8192 4KB pages. */
	mem = ATNGW100_NR_PAGES;

	/*
	 * Now that we know how much memory we have, initialize the
	 * mem cluster array.
	 */
	mem_clusters[0].start = ATNGW100_MEM_START;
	mem_clusters[0].size  = ctob(mem);
	mem_cluster_cnt = 1;

	return mem;
}
