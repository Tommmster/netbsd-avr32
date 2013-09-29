/*	$NetBSD: asm.h,v 1.40 2007/10/17 19:55:36 garbled Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)machAsmDefs.h	8.1 (Berkeley) 6/10/93
 */

/*
 * machAsmDefs.h --
 *
 *	Macros used when writing assembler programs.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 *
 * from: Header: /sprite/src/kernel/mach/ds3100.md/RCS/machAsmDefs.h,
 *	v 1.2 89/08/15 18:28:24 rab Exp  SPRITE (DECWRL)
 */

#ifndef _AVR32_ASM_H_
#define _AVR32_ASM_H_

#include <machine/cdefs.h>	/* for API selection */
#include <avr32/cpuregs.h>

/*
 * WEAK_ALIAS: create a weak alias.
 */
#define	WEAK_ALIAS(alias,sym)						\
	.weak alias;							\
	alias = sym
/*
 * STRONG_ALIAS: create a strong alias.
 */
#define STRONG_ALIAS(alias,sym)						\
	.globl alias;							\
	alias = sym

/*
 * WARN_REFERENCES: create a warning if the specified symbol is referenced.
 */
#ifdef __STDC__
#define	WARN_REFERENCES(_sym,_msg)				\
	.section .gnu.warning. ## _sym ; .ascii _msg ; .text
#else
#define	WARN_REFERENCES(_sym,_msg)				\
	.section .gnu.warning./**/_sym ; .ascii _msg ; .text
#endif /* __STDC__ */

/*
 * ENTRY
 *	Mark the beggining of a procedure.
 */
#define ENTRY(sym)			\
	.align 1;			\
	.section .text,"ax",@progbits;	\
	.globl _C_LABEL(sym);		\
	.type sym,@function;		\
	_C_LABEL(sym):

/*
 * END
 *	Mark end of a procedure.
 */
#define END(sym)			\
	.size _C_LABEL(sym), . - _C_LABEL(sym)

/*
 * EXPORT -- export definition of symbol
 */
#define EXPORT(x)			\
	.globl	_C_LABEL(x);		\
_C_LABEL(x):

/*
 * VECTOR
 *	exception vector entrypoint
 *	XXX: regmask should be used to generate .mask
 */
#define VECTOR(x, regmask)		\
	.ent	_C_LABEL(x),0;		\
	EXPORT(x);			\

#ifdef __STDC__
#define VECTOR_END(x)			\
	EXPORT(x ## End);		\
	END(x)
#else
#define VECTOR_END(x)			\
	EXPORT(x/**/End);		\
	END(x)
#endif

#define FUNCTION(x)			\
	VECTOR(x, unknown)		\
	.type x, @function
/*
 * Macros to panic and printf from assembly language.
 */
#define PANIC(msg)			\
	lda.w	r12, 9f;		\
	rcall	_C_LABEL(panic);	\
	MSG(msg)

#define	PRINTF(msg)			\
	lda.w	r12, 9f;		\
	rcall	_C_LABEL(printf);	\
	MSG(msg)

#define	MSG(msg)			\
	.section .rodata,"a",@progbits; \
9:	.asciz	msg;			\
	.section .text,"ax",@progbits;	\
	.align 1

#endif /* !_AVR32_ASM_H */
