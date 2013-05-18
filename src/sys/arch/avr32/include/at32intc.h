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

#define AT32INTC_IRQ_LEVEL_0	0
#define AT32INTC_IRQ_LEVEL_1	1
#define AT32INTC_IRQ_LEVEL_2	2
#define AT32INTC_IRQ_LEVEL_3	3

#define AT32INTC_IRQ_COUNT	0
#define AT32INTC_IRQ_USART0	0
#define AT32INTC_IRQ_USART1	0
#define AT32INTC_IRQ_USART2	0
#define AT32INTC_IRQ_USART3	0

#define AT32INTC_GRP_COUNT	0
#define AT32INTC_GRP_USART0	6
#define AT32INTC_GRP_USART1	7
#define AT32INTC_GRP_USART2	8
#define AT32INTC_GRP_USART3	9

#define AT32INTC_NR_IRQS        2048

struct at32intc_ictx {
	uint32_t st;
	uint32_t pc;
};

typedef void (at32intc_handler)(void *, void *);

void *at32intc_intr_establish(int, int, int, at32intc_handler *, void *);
