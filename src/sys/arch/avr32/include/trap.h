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

#ifndef _TRAP_H_
#define _TRAP_H_

/*
 * Trap codes
 * also known in trap.c for name strings
 */

#define T_BUS_ERR_LD_ST		2	/* Bus error on dfetch */
#define T_BUS_ERR_IFETCH	3	/* Bus error on ifetch */
#define T_TLB_EX_PROT		6	/* TLB protection on ifetch */
#define T_DTLB_PROT		10	/* DTLB PProt */
#define T_ADDR_ERR_RD		13	/* Data read address error */
#define T_ADDR_ERR_WR		14	/* Data write address error */
#define T_TLB_RD_PROT		15	/* TLB read protection */
#define T_TLB_WR_PROT		16	/* TLB write protection */
#define T_TLB_MOD		17	/* TLB Modified */
#define T_ITLB_MISS		20	/* ITLB Miss */
#define T_TLB_LD_MISS		24	/* TLB Miss on read */
#define T_TLB_ST_MISS		28	/* TLB Miss on write */
#define T_USER			0x8000	/* user-mode flag or'ed with type */

#endif /* _TRAP_H_ */
