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
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/types.h>

#include <avr32/proc.h>

#ifndef EMULNAME
#define EMULNAME(x)	(x)
#endif

void	EMULNAME(syscall_intern)(struct proc *);
void	EMULNAME(syscall_plain)(struct lwp *, u_int, u_int, u_int);
void	EMULNAME(syscall_fancy)(struct lwp *, u_int, u_int, u_int);

vaddr_t MachEmulateBranch(struct frame *, vaddr_t, u_int, int);

void
EMULNAME(syscall_intern)(struct proc *p)
{
	if (trace_is_enabled(p))
		p->p_md.md_syscall = EMULNAME(syscall_fancy);
	else
		p->p_md.md_syscall = EMULNAME(syscall_plain);
}

void
EMULNAME(syscall_plain)(struct lwp *l, u_int status, u_int cause, u_int opc)
{
	panic("syscall_plain: notyet");
}

void
EMULNAME(syscall_fancy)(struct lwp *l, u_int status, u_int cause, u_int opc)
{
	panic("syscall_fancy: notyet");
}
