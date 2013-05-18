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

#include <avr32/cpuregs.h>

#ifndef EARLY_STACK_SIZE
#define EARLY_STACK_SIZE	8192
#endif

	.section .init,"ax"
	.global _start
_start:
        /* The bootloader must provide a magic number. Check it. */
	lddpc	r0, uboot_magic_number
	cp.w	r12, r0
	brne	uboot_fail

	/* Initialize .bss. */
	lda.w	r1, __bss_start
	lda.w	r2, __bss_end__
	mov	r0, 0
1:	st.w	r1++, r0
	cp	r1, r2
	brlo	1b


#ifndef AVR32_INITIAL_STATUS
#define AVR32_INITIAL_STATUS \
	AVR32_STATUS_MKERN | AVR32_STATUS_IM | AVR32_STATUS_EM
#endif

	/* Initialize status register: mask interrupts and exceptions. */
	mov	r0, 0
	orh	r0, hi(AVR32_INITIAL_STATUS)
	mtsr	SR_STATUS, r0
	nop

	/* Set up the initial stack pointer. */
	lda.w	sp, __data_start
	sub	sp, -EARLY_STACK_SIZE

	/* Mark the last stack frame. */
	mov	lr, 0
	mov	r7, 0

	/* Initialize the machine. */
	rcall	mach_init

	/* Execute main(), which does not return. */
	rcall	main

uboot_fail:
	/* XXX Should do something else like restart. */
	rjmp	uboot_fail

	.align 2
uboot_magic_number:
	.long	0xa2a25441
