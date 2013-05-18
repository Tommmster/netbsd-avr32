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
#include <sys/systm.h>

#include <avr32/locore.h>
#include <avr32/cpuregs.h>

static const char *xcpt_names[] = {
	"Unrecoverable Exception",	/* evba + 0x00  */
	"TLB Multiple Hit", 		/* evba + 0x04  */
	"Bus error data fetch",		/* evba + 0x08  */
	"Bus error instruction fetch",	/* evba + 0x0C  */
	"NMI",				/* evba + 0x10  */
	"Instruction address",		/* evba + 0x14  */
	"ITLB Protection",		/* evba + 0x18  */
	"Breakpoint",			/* evba + 0x1C  */
	"Illegal Opcode",		/* evba + 0x20  */
	"Unimplemented Instruction",	/* evba + 0x24  */
	"Privilege violation",		/* evba + 0x28  */
	"Floating Point",		/* evba + 0x2C  */
	"Copprocessor Absent",		/* evba + 0x30  */
	"Data address (read)",		/* evba + 0x34  */
	"Data address (write)",		/* evba + 0x38  */
	"DTLB Protection (read)",	/* evba + 0x3C  */
	"DTLB Protection (write)",	/* evba + 0x40  */
	"DTLB Modified",		/* evba + 0x44  */
	"reserved 18",			/* evba + 0x48  */
	"reserved 19",			/* evba + 0x4c  */
	"reserved 20",			/* evba + 0x50  */
	"reserved 21",			/* evba + 0x54  */
	"reserved 22",			/* evba + 0x58  */
	"reserved 23",			/* evba + 0x5c  */
	"DTLB read miss",		/* evba + 0x60  */
	"reserved 25",			/* evba + 0x64  */
	"reserved 26",			/* evba + 0x68  */
	"reserved 27",			/* evba + 0x6c  */
	"DTLB write miss",		/* evba + 0x70  */
	"reserved 29",			/* evba + 0x74  */
	"reserved 30",			/* evba + 0x78  */
	"reserved 31",			/* evba + 0x7c  */
	"reserved 32",			/* evba + 0x80  */
	"reserved 33",			/* evba + 0x84  */
	"reserved 34",			/* evba + 0x88  */
	"reserved 35",			/* evba + 0x8c  */
	"reserved 36",			/* evba + 0x90  */
	"reserved 37",			/* evba + 0x94  */
	"reserved 38",			/* evba + 0x98  */
	"reserved 39",			/* evba + 0x9c  */
	"supervisor call",		/* evba + 0x100 */
};

static const char * 
exception_name(unsigned int offset)
{
#define LEN(x) \
	(sizeof(x) / sizeof(x[0]))

	if (offset < LEN(xcpt_names))
		return xcpt_names[offset];

	panic("offset error :%u \n", offset);
	
}

void
handle_with_panic(unsigned int ecr, unsigned int pc)
{
#if 0
	printf("[Handle with panic] ecr: %u\n", ecr);
	printf("[Handle with panic] PC: %x\n", pc);
	printf("[Handle with panic] STATUS: %x\n", AVR32_MFSR(SR_STATUS));
#endif
	panic(exception_name(ecr));
}
