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
#include <sys/types.h>
#include <uvm/uvm_extern.h>

#include <avr32/cache.h>
#include <avr32/cpuregs.h>
#include <avr32/locore.h>

#define ROUND_LINE(a) \
	(((a) + avr32_pdcache_line_size - 1) & ~(avr32_pdcache_line_size - 1))

#define TRUNC_LINE(a) \
	((a) & ~(avr32_pdcache_line_size - 1))

#define avr32_flush_write_buffer() \
	asm volatile("sync 0" : : : "memory")

/*
 * avr32_cache_op_line:
 *
 *      Perform the specified cache operation on a single line.
 */
#define avr32_cache_op_line(va, op)    \
do {                                   \
	asm volatile(                  \
		"cache %0[0], %1 \n\t" \
		:                      \
		: "r" (va), "n" (op)   \
		: "memory");           \
} while (/*CONSTCOND*/0)

/*
 * avr32_cache_op_all:
 *
 *      Perform the specified cache operation on the entire cache.
 */
#define avr32_cache_op_all(mode, op)   \
do {                                   \
	asm volatile(                  \
		"cache %0[0], %1 \n\t" \
		:                      \
		: "r" (mode), "n" (op) \
		: "memory");           \
} while (/*CONSTCOND*/0)

#define AVR32_ICACHE_OP_FLUSH		0x00
#define AVR32_ICACHE_OP_INV		0x01
#define AVR32_DCACHE_OP_FLUSH		0x08
#define AVR32_DCACHE_OP_INV		0x0b
#define AVR32_DCACHE_OP_WB		0x0c
#define AVR32_DCACHE_OP_WBINV		0x0d

#define AVR32_ICACHE_MODE_ALL		0x00
#define AVR32_DCACHE_MODE_WBINV_ALL	0x04

/* PRIMARY CACHE VARIABLES */
u_int avr32_picache_ways;
u_int avr32_picache_sets;
u_int avr32_picache_way_size;
u_int avr32_picache_way_mask;
u_int avr32_picache_size;
u_int avr32_picache_line_size;
u_int avr32_picache_loopcount;
u_int avr32_picache_stride;

u_int avr32_pdcache_ways;
u_int avr32_pdcache_sets;
u_int avr32_pdcache_way_size;
u_int avr32_pdcache_way_mask;
u_int avr32_pdcache_size;
u_int avr32_pdcache_line_size;
u_int avr32_pdcache_loopcount;
u_int avr32_pdcache_stride;

/*
 * These two variables inform the rest of the kernel about the
 * size of the largest D-cache line present in the system.  The
 * mask can be used to determine if a region of memory is cache
 * line size aligned.
 *
 * Whenever any code updates a data cache line size, it should
 * call avr32_dcache_compute_align() to recompute these values.
 */
u_int avr32_dcache_align;
u_int avr32_dcache_align_mask;

u_int avr32_cache_alias_mask;    /* for virtually-indexed caches */
u_int avr32_cache_prefer_mask;

/*
 * avr32_config_cache:
 *
 *      Configure the cache for the system.
 */
void
avr32_config_cache(void)
{
	unsigned config1 = AVR32_MFSR(SR_CONFIG1);

	avr32_picache_ways = 1U << AVR32_CONFIG1_ICACHE_WAYS(config1);
	avr32_picache_sets = 1U << AVR32_CONFIG1_ICACHE_SETS(config1);

	if (AVR32_CONFIG1_ICACHE_LINE_SIZE(config1) != 0) {
		avr32_picache_line_size = 
			1U << (1 + AVR32_CONFIG1_ICACHE_LINE_SIZE(config1));
	}

	avr32_pdcache_ways = 1U << AVR32_CONFIG1_DCACHE_WAYS(config1);
	avr32_pdcache_sets = 1U << AVR32_CONFIG1_DCACHE_SETS(config1);
	
	if (AVR32_CONFIG1_DCACHE_LINE_SIZE(config1) != 0) {
		avr32_pdcache_line_size = 
			1U << (1 + AVR32_CONFIG1_DCACHE_LINE_SIZE(config1));
	}

	avr32_picache_size = avr32_picache_ways * avr32_picache_sets 
		* avr32_picache_line_size;
	
	avr32_pdcache_size = avr32_pdcache_ways * avr32_pdcache_sets 
		* avr32_pdcache_line_size;

	/*
	 * Compute the "way mask" for each cache.
	 */
	if (avr32_picache_size) {
		KASSERT(avr32_picache_ways != 0);
		avr32_picache_way_size = 
			(avr32_picache_size / avr32_picache_ways);
		avr32_picache_way_mask = 
			avr32_picache_way_size - 1;
	}
	if (avr32_pdcache_size) {
		KASSERT(avr32_pdcache_ways != 0);
		avr32_pdcache_way_size = 
			(avr32_pdcache_size / avr32_pdcache_ways);
		avr32_pdcache_way_mask = 
			avr32_pdcache_way_size - 1;
	}

	if (avr32_pdcache_way_size < PAGE_SIZE) {
		avr32_pdcache_stride = avr32_pdcache_way_size;
		avr32_pdcache_loopcount = avr32_pdcache_ways;
	} else {
		avr32_pdcache_stride = PAGE_SIZE;
		avr32_pdcache_loopcount = 
			(avr32_pdcache_way_size / PAGE_SIZE) * avr32_pdcache_ways;
	}

	avr32_cache_alias_mask =
		((avr32_pdcache_size / avr32_pdcache_ways) - 1) & ~PAGE_MASK;
	avr32_cache_prefer_mask =
		max(avr32_pdcache_size, avr32_picache_size) - 1;
}

void
avr32_icache_sync_all(void)
{
	avr32_cache_op_all(AVR32_ICACHE_MODE_ALL, AVR32_ICACHE_OP_FLUSH);
}

void
avr32_icache_sync_range(vaddr_t sva, vsize_t size)
{
	panic("avr32_icache_sync_range: notyet");
}

void
avr32_icache_sync_range_index(vaddr_t sva, vsize_t size)
{
	vaddr_t eva;
	vaddr_t va;
	int i;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a P1
	 * address out of them.
	 */
	sva = AVR32_PHYS_TO_P1(sva & avr32_pdcache_way_mask);
	eva = ROUND_LINE(sva + size);
	va  = TRUNC_LINE(sva);

	avr32_dcache_wbinv_range_index(va, (eva - va));

	while (va < eva) {
		vaddr_t tva = va;

		for (i = 0; i < avr32_picache_loopcount; ++i) {
			avr32_cache_op_line((void *)tva,
			                    AVR32_ICACHE_OP_INV);
			tva += avr32_picache_stride;
		}

		va += avr32_picache_line_size;
	}
}

void
avr32_dcache_wbinv_all(void)
{
	avr32_cache_op_all(AVR32_DCACHE_MODE_WBINV_ALL, AVR32_DCACHE_OP_FLUSH);
	avr32_flush_write_buffer();
}

void
avr32_dcache_wbinv_range(vaddr_t sva, vsize_t size)
{
	vaddr_t eva = ROUND_LINE(sva + size);
	vaddr_t  va = TRUNC_LINE(sva);

	while (va < eva) {
		avr32_cache_op_line((void *)va, AVR32_DCACHE_OP_WBINV);
		va += avr32_pdcache_size;
	}

	avr32_flush_write_buffer();
}

void
avr32_dcache_wbinv_range_index(vaddr_t sva, vsize_t size)
{
	vaddr_t eva;
	vaddr_t va;
	int i;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a P1
	 * address out of them.
	 */
	sva = AVR32_PHYS_TO_P1(sva & avr32_pdcache_way_mask);
	eva = ROUND_LINE(sva + size);
	va  = TRUNC_LINE(sva);

	while (va < eva) {
		vaddr_t tva = va;

		for (i = 0; i < avr32_pdcache_loopcount; ++i) {
			avr32_cache_op_line((void *)tva,
			                    AVR32_DCACHE_OP_WBINV);
			tva += avr32_pdcache_stride;
		}

		va += avr32_pdcache_line_size;
	}

	avr32_flush_write_buffer();
}

void
avr32_dcache_inv_range(vaddr_t sva, vsize_t size)
{
	vaddr_t eva = ROUND_LINE(sva + size);
	vaddr_t  va = TRUNC_LINE(sva);

	while (va < eva) {
		avr32_cache_op_line((void *)va, AVR32_DCACHE_OP_INV);
		va += avr32_pdcache_size;
	}

	avr32_flush_write_buffer();
}

void
avr32_dcache_wb_range(vaddr_t sva, vsize_t size)
{
	panic("avr32_dcache_wb_range: notyet");
}
