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

#include <sys/cdefs.h>
#include <sys/errno.h>

#include "assym.h"


/*
 * kcopy(const void *src, void *dst, size_t len);
 *
 * Copy len bytes from src to dst, aborting if we encounter a fatal 
 * page fault.
 *
 * kcopy() _must_ save and restore the old fault handler since it is
 * called by uiomove(), which may be in the path of servicing a non-fatal
 * page fault.
 */
	.text
	.globl kcopy
	.align 2

kcopy:
	stm	--sp, r0,lr

	/* Swap r12 <-> r11 for call to memcpy. */
	mov	r8, r12
	mov	r12, r11
	mov	r11, r8

	/* Set up fault handler. */
	lda.w	r8, avr32_curlwp
	ld.w	r8, r8[0]
	ld.w	r8, r8[L_ADDR]
	ld.w	r0, r8[U_PCB_ONFAULT]
	lda.w	r9, kcopyerr
	st.w	r8[U_PCB_ONFAULT], r9

	/* All set. */
	rcall	memcpy

	/* Restore the previous handler. */
	lda.w	r8, avr32_curlwp
	ld.w	r8, r8[0]
	ld.w	r8, r8[L_ADDR]
	st.w	r8[U_PCB_ONFAULT], r0

	/* Success! */
	ldm	sp++, r0,lr
	mov	r12, 0
	mov	pc, lr
	.size	kcopy, . - kcopy

/*
 * int copystr(void *kfaddr, void *kdaddr, size_t maxlen, size_t *lencopied)
 *
 * Copy a NIL-terminated string, at most maxlen characters long.  Return the
 * number of characters copied (including the NIL) in *lencopied.  If the  
 * string is too long, return ENAMETOOLONG; else return 0.
 */
	.text
	.globl copystr
	.align 1

copystr:
	stm	--sp, r7,lr

	mov	r7, r10
	cp.w	r7, 0
	breq	2f	/* No characters to copy */

1:	ld.ub	r8, r12[0]
	sub	r12, -1
	sub	r10, 1
	st.b	r11[0], r8
	sub	r11, -1

	cp.w	r8, 0	/* Nil-terminated */
	breq	4f

	cp.w	r10, 0	/* More to copy? */
	brne	1b
	
	/* kfaddr is not NIL- terminated */
	mov	r12, ENAMETOOLONG

	/* 
	 * return the number of copied bytes in the
	 * 4th argument if its non-NULL 
	 */
2:	cp.w	r9, 0		
	breq	3f	
	sub 	r7, r10
	st.w	r9[0], r7
	bral	3f

4:	mov	r12, 0
	bral	2b

3:	ldm	sp++, r7,pc
	.size 	copystr, . - copystr

/*          
 * int copyout(void *kaddr, void *uaddr, size_t len)
 * Copies len bytes of data from the kernel-space address kaddr to the
 * user-space address uaddr.  copyout returns 0 on success or EFAULT
 * if a bad address is encountered.
 */   
	.text
	.globl copyout
	.align 1

copyout:
#if 1
	stm	--sp, lr

	/* 
	 * Set copyerr for exception handling
	 */
	lda.w	r8, avr32_curlwp
	ld.w	r8, r8[0]
	ld.w	r8, r8[L_ADDR]
	lda.w	r9, copyerr
	ld.w	r9, r9[0]
	st.w	r8[U_PCB_ONFAULT], r9
	
	mov	r9, r12
	mov	r12, r11
	mov	r11, r9
	rcall	memcpy

	/*
	 * reset the error handler
	 */
	lda.w	r8, avr32_curlwp
	ld.w	r8, r8[0]	
	ld.w	r8, r8[L_ADDR]
	mov	r9, 0
	st.w	r8[U_PCB_ONFAULT], r9

	mov	r12, 0	
	ldm	sp++, pc
#else
	lda.w	r12, copyout_panic
	rcall	panic
	.size copyout, . -copyout

	.align 2
copyout_panic: 
	.asciz "copyout"
#endif 

	.text
	.globl copyerr
	.align 1

copyerr:
#if notyet
	stm	--sp, lr

	lda.w	r12, avr32_curlwp
	ld.w	r12, r12[0]	
	ld.w	r12, r12[L_ADDR]

	mov	r11, 0
	st.w	r12[U_PCB_ONFAULT], r11

	mov	r12, EFAULT
	ldm	sp++, pc
#else
	lda.w	r12, copyerr_panic	
	rcall	panic
	
copyerr_panic: 
	.asciz "copyerr"
#endif

/*
 * int copyinstr(void *uaddr, void *kaddr, size_t maxlen, size_t *lencopied)
 * Copy a NIL-terminated string, at most maxlen characters long, from the
 * user's address space.  Return the number of characters copied (including
 * the NIL) in *lencopied.  If the string is too long, return ENAMETOOLONG;
 * else return 0 or EFAULT. 
 */
	.text
	.globl copyinstr
	.align 2

copyinstr:
	/* Must use some extra registers */
	stm	--sp, r6-r7,lr
	mov	r7, r10

	lda.w	r8, avr32_curlwp
	ld.w	r8, r8[0]
	ld.w	r8, r8[L_ADDR]
	lda.w	r6, copystrerr
	ld.w	r6, r6[0]
	st.w	r8[U_PCB_ONFAULT], r6

	cp.w	r10, 0
	breq	4f

1:	ld.ub	r8, r12[0]
	sub	r10, 1
	st.b	r11[0], r8
	cp.w	r8, 0
	breq	2f
	sub	r12, -1
	sub	r11, -1

	cp.w	r10, 0
	brne	1b
	
2:	cp.w	r9, 0
	breq	3f
	sub	r7, r10
	st.w	r9[0], r7

	mov	r12, 0
	
3:	lda.w	r8, avr32_curlwp
	ld.w	r8, r8[0]
	ld.w	r8, r8[L_ADDR]
	mov	r6, 0
	st.w	r8[U_PCB_ONFAULT], r6

	ldm	sp++,r6-r7,pc
4:	mov	r12, 1
	bral	3b
	.size copyinstr, . - copyinstr

	.text
	.globl copystrerr
	.align 2

copystrerr:
	lda.w	r12, copystrerr_panic	
	rcall	panic

	.align 2
copystrerr_panic: 
	.asciz "copystrerr"
	
/*       
 * int suword(void *, int)
 * Stores a word of data to the user-space address.
 */   
	.text
	.globl suword
	.align 1
suword:
	stm	--sp, lr

	/* Set fswberr for exception handling */
	lda.w	r8, avr32_curlwp
	ld.w	r8, r8[0]
	ld.w	r8, r8[L_ADDR]
	lda.w	r9, fswberr
	ld.w	r9, r9[0]
	st.w	r8[U_PCB_ONFAULT], r9

	/* All set: store word */
	st.w	r12[0], r11

	mov	r11, 0
	st.w	r8[U_PCB_ONFAULT], r11
	ldm	sp++, pc
	.size suword, . - suword

fswberr:
	lda.w	r12, fswberr_panic	
	rcall panic

fswberr_panic:
	.asciz "fswberr_panic"		

/*
 * int badaddr(void addr, int len)
 * See if access to addr with a len type instruction causes a machine check.
 * len is length of access (1=byte, 2=short, 4=long)
 */
	.section .text,"ax",@progbits
	.global badaddr
	.align 2

badaddr:
	/* Set up fault handler. */
	lda.w	r8, avr32_curlwp
	ld.w	r8, r8[0]
	ld.w	r8, r8[L_ADDR]
	lda.w	r9, baderr
	st.w	r8[U_PCB_ONFAULT], r9

	cp.w	r11, 1 /* byte */
	breq	1f

	cp.w	r11, 2 /* short */
	breq	2f
#if 1
	cp.w	r11, 4 /* long */
	breq	4f

	lda.w	r12, L_BADADDR_PANIC_MESG
	rcall	panic

L_BADADDR_PANIC_MESG:
	.asciz "badaddr: invalid len"
	.align 2
#endif
4:	ld.w	r10, r12[0]
	bral	5f

2:	ld.uh	r10, r12[0]
	bral	5f

1:	ld.ub	r10, r12[0]
	bral	5f

5:	mov	r12, 0
	st.w	r8[U_PCB_ONFAULT], r12
	mov	pc, lr
	.size badaddr, . - badaddr
