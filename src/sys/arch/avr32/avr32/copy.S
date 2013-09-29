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
#include <sys/errno.h>

#include <avr32/asm.h>

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
ENTRY(kcopy)
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
	ldm	sp++, r0, lr
	mov	r12, 0
	mov	pc, lr
END(kcopy)

/*
 * int copystr(void *kfaddr, void *kdaddr, size_t maxlen, size_t *lencopied)
 *
 * Copy a NIL-terminated string, at most maxlen characters long. Return the
 * number of characters copied (including the NIL) in *lencopied. If the
 * string is too long, return ENAMETOOLONG; else return 0.
 */
ENTRY(copystr)
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
	 * Return the number of copied bytes in the
	 * 4th argument if it is non-NULL.
	 */
2:	cp.w	r9, 0		
	breq	3f	
	sub 	r7, r10
	st.w	r9[0], r7
	bral	3f

4:	mov	r12, 0
	bral	2b

3:	ldm	sp++, r7,pc
END(copystr)

/*
 * int copyin(void *uaddr, void *kaddr, size_t len)
 * Copies len bytes of data from the user-space address uaddr to the
 * kernel-space address kaddr.  copyin returns 0 on success or EFAULT
 * if a bad address is encountered.
 */
ENTRY(copyin)
	stm	--sp, lr

	cp.w	r12, 0
	brle	copyerr

	/* Swap r12 <-> r11 for call to memcpy. */
	mov	r8, r12
	mov	r12, r11
	mov	r11, r8

	/* Set up fault handler. */
	lda.w	r8, avr32_curlwp
	ld.w	r8, r8[0]
	ld.w	r8, r8[L_ADDR]
	lda.w	r9, copyerr
	ld.w	r9, r9[0]
	st.w	r8[U_PCB_ONFAULT], r9

	/* All set. */
	rcall	memcpy

	/*
	 * Reset the error handler.
	 */
	lda.w	r8, avr32_curlwp
	ld.w	r8, r8[0]	
	ld.w	r8, r8[L_ADDR]
	mov	r9, 0
	st.w	r8[U_PCB_ONFAULT], r9

	/* Success! */
	ldm	sp++, lr
	mov	r12, 0
	mov	pc, lr
END(copyin)

/* 
 * int copyout(void *kaddr, void *uaddr, size_t len)
 * Copies len bytes of data from the kernel-space address kaddr to the
 * user-space address uaddr.  copyout returns 0 on success or EFAULT
 * if a bad address is encountered.
 */
ENTRY(copyout)
	stm	--sp, lr

	cp.w	r11, 0
	brlt	copyerr

	/* Swap r12 <-> r11 for call to memcpy. */
	mov	r8, r12
	mov	r12, r11
	mov	r11, r8

	/* 
	 * Set copyerr for exception handling.
	 */
	lda.w	r8, avr32_curlwp
	ld.w	r8, r8[0]
	ld.w	r8, r8[L_ADDR]
	lda.w	r9, copyerr
	ld.w	r9, r9[0]
	st.w	r8[U_PCB_ONFAULT], r9

	/* All set. */
	rcall	memcpy

	/*
	 * Reset the error handler.
	 */
	lda.w	r8, avr32_curlwp
	ld.w	r8, r8[0]	
	ld.w	r8, r8[L_ADDR]
	mov	r9, 0
	st.w	r8[U_PCB_ONFAULT], r9

	/* Success! */
	ldm	sp++, lr
	mov	r12, 0
	mov	pc, lr
END(copyout)

ENTRY(copyerr)
	PANIC("copyerr: notyet");
END(copyerr)

/*
 * int copyinstr(void *uaddr, void *kaddr, size_t maxlen, size_t *lencopied)
 * Copy a NIL-terminated string, at most maxlen characters long, from the
 * user's address space. Return the number of characters copied (including
 * the NIL) in *lencopied. If the string is too long, return ENAMETOOLONG;
 * else return 0 or EFAULT.
 */
ENTRY(copyinstr)
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

	ldm	sp++, r6-r7,pc
4:	mov	r12, 1
	bral	3b
END(copyinstr)

/*
 * int copyoutstr(void *kaddr, void *uaddr, size_t maxlen, size_t *lencopied);
 * Copy a NIL-terminated string, at most maxlen characters long, into the
 * user's address space. Return the number of characters copied (including
 * the NIL) in *lencopied. If the string is too long, return ENAMETOOLONG;
 * else return 0 or EFAULT.
 */
ENTRY(copyoutstr)
	/* Must use some extra registers */
	stm	--sp, r6-r7, lr
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
	moveq	r12, 0
	breq	2f
	sub	r12, -1
	sub	r11, -1

	cp.w	r10, 0
	brne	1b
	
4:	mov	r12, ENAMETOOLONG

2:	cp.w	r9, 0
	breq	3f
	sub	r7, r10
	st.w	r9[0], r7

3:	lda.w	r8, avr32_curlwp
	ld.w	r8, r8[0]
	ld.w	r8, r8[L_ADDR]
	mov	r6, 0
	st.w	r8[U_PCB_ONFAULT], r6

	ldm	sp++, r6-r7,lr
	mov	pc, lr
END(copyoutstr)

ENTRY(copystrerr)
	PANIC("copystrerr: notyet");
END(copystrerr)

/* 
 * int suword(void *, int)
 * Stores a word of data to the user-space address.
 */
ENTRY(suword)
	stm	--sp, lr

	/* Set fswberr for exception handling. */
	lda.w	r8, avr32_curlwp
	ld.w	r8, r8[0]
	ld.w	r8, r8[L_ADDR]
	lda.w	r9, fswberr
	ld.w	r9, r9[0]
	st.w	r8[U_PCB_ONFAULT], r9

	cp.w	r12, 0
	brle	fswberr

	/* Store word. */
	st.w	r12[0], r11

	/* Clear out fault handler. */
	lda.w	r8, avr32_curlwp
	ld.w	r8, r8[0]
	ld.w	r8, r8[L_ADDR]
	mov	r9, 0
	st.w	r8[U_PCB_ONFAULT], r9

	/* Success! */
	ldm	sp++, lr
	mov	r12, 0
	mov	pc, lr
END(suword)

ENTRY(fswberr)
	PANIC("fswberr: notyet");
END(fswberr)

/*
 * int badaddr(void addr, int len)
 * See if access to addr with a len type instruction causes a machine check.
 * len is length of access (1=byte, 2=short, 4=long)
 */
ENTRY(badaddr)
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

	cp.w	r11, 4 /* long */
	breq	4f

	PANIC("badaddr: invalid len");

4:	ld.w	r10, r12[0]
	bral	5f

2:	ld.uh	r10, r12[0]
	bral	5f

1:	ld.ub	r10, r12[0]
	bral	5f

5:	mov	r12, 0
	st.w	r8[U_PCB_ONFAULT], r12

	mov	pc, lr
END(badaddr)

/*
 * int subyte(void *, int)
 * Stores a byte of data to the user-space address.
 */
ENTRY(subyte)
	lda.w	r8, avr32_curlwp
	ld.w	r8, r8[0]
	ld.w	r8, r8[L_ADDR]
	lda.w	r9, fswberr
	st.w	r8[U_PCB_ONFAULT], r9
	st.b	r12[0], r11
	mov	r12, 0
	st.w	r8[U_PCB_ONFAULT], r12
	mov	pc, lr
END(subyte)
