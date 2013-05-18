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

#ifndef _AVR32_LOCORE_H_
#define _AVR32_LOCORE_H_

#include <sys/cdefs.h>
#include <sys/types.h>

#define AVR32_MFSR(reg)				\
	({					\
		uint32_t res_;			\
		asm volatile(			\
			"mfsr %0, %1\n"		\
			"nop        \n"		\
			: "=&r"(res_)		\
			: "n"(reg)		\
			: "memory");		\
		(res_);				\
	})

#define AVR32_MTSR(reg, val)			\
	({					\
		asm volatile(			\
			"mtsr %1, %0\n"		\
			"nop        \n"		\
			: 			\
			: "r"(val), "n"(reg)	\
			: "memory");		\
		(0);				\
	})

extern int avr32_num_tlb_entries;

int _splraise(int);
int _spllower(int);
int _splget(void);
int _splset(int);
void _splnone(void);

void *setfunc_trampoline(void);
void *lwp_trampoline(void);

void MachTLBUpdate(register_t, register_t);
void avr32_tbiap(int);
void avr32_tbis(vaddr_t);

#define AVR32_TBIAP() \
	avr32_tbiap(avr32_num_tlb_entries)

void avr32_count_write(unsigned int value);
void avr32_compare_write(unsigned int value);
unsigned int avr32_count_read(void);

#define TF_NREGS	16

struct trapframe {
	avr32_reg_t tf_regs[TF_NREGS];
	u_int32_t  tf_ppl;              /* previous priority level */
	int32_t    tf_pad;              /* for 8 byte aligned */
};

#endif /* _AVR32_LOCORE_H_ */
