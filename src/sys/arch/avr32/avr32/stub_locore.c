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

#include <sys/lwp.h>
#include <sys/systm.h>
#include <machine/locore.h>
#include <machine/param.h>

#include <sys/user.h>

static const char * 
lname(struct lwp * l)
{

	if (l == NULL) 
		return "NULL";

	
	if (l->l_name == NULL) 
		return "l_name NULL";

	return l->l_name;
}

struct lwp *
_cpu_switchto(struct lwp *l1, struct lwp *l2)
{
#if 1
	printf("[XXXAVR32] cpu_switchto %s:%p -> %s:%p\n", 
		lname(l1), l1, lname(l2), l2);
#endif
	return (void *)_cpu_switchto(l1, l2);
}

/* XXXAVR32 Machine dependent. IMPLEMENT AS ASSEMBLY CODE ! */
void
dump_rx(unsigned int seq, unsigned int reg)
{
	printf("seq: %d, reg: 0x%x\n", seq, reg);
}

void*
setfunc_trampoline(void)
{
	panic("setfunc_trampoline: notyet");
}
