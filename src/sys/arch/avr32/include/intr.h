/*	$NetBSD$	*/

/*
 * Copyright (c) 1998 Jonathan Stone.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Jonathan Stone for
 *      the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _AVR32_INTR_H_
#define _AVR32_INTR_H_

#include <sys/evcnt.h>
#include <sys/queue.h>

#include <avr32/cpuregs.h>
#include <avr32/locore.h>

#define	IPL_NONE	0	/* disable only this interrupt */
#define	IPL_SOFTCLOCK	1	/* clock software interrupts (SI 0) */
#define	IPL_SOFTBIO	1	/* generic software interrupts (SI 0) */
#define	IPL_SOFTNET	2	/* network software interrupts (SI 1) */
#define	IPL_SOFTSERIAL	2	/* serial software interrupts (SI 1) */
#define	IPL_VM		3
#define	IPL_SCHED	4
#define	IPL_HIGH	5

#define	_IPL_N		6

#ifdef _KERNEL
#ifndef _LOCORE

#define spl0()		_spllower(0)
#define splx(s)		_splset(s)
#define splvm()		_splraise(ipl2spl_table[IPL_VM])
#define splsched()	_splraise(ipl2spl_table[IPL_SCHED])
#define splhigh()	_splraise(ipl2spl_table[IPL_HIGH])

#define splsoftclock()	_splraise(ipl2spl_table[IPL_SOFTCLOCK])
#define splsoftbio()	_splraise(ipl2spl_table[IPL_SOFTBIO])
#define splsoftnet()	_splraise(ipl2spl_table[IPL_SOFTNET])
#define splsoftserial()	_splraise(ipl2spl_table[IPL_SOFTSERIAL])

extern const int *ipl2spl_table;
extern void (*cpu_intr)(int, uint32_t, uint32_t);

typedef int ipl_t;
typedef struct {
	int _spl;
} ipl_cookie_t;

ipl_cookie_t makeiplcookie(ipl_t);

static inline int
splraiseipl(ipl_cookie_t icookie)
{

	return _splraise(icookie._spl);
}

extern struct evcnt avr32_clock_evcnt;
extern struct evcnt avr32_memerr_evcnt;

#endif /* !_LOCORE */
#endif /* _KERNEL */

#endif	/* !_AVR32_INTR_H_ */
