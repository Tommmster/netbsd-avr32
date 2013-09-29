/*	$NetBSD$	*/

#include <avr32/asm.h>
#include <avr32/cpuregs.h>

#include "assym.h"

#ifdef MULTIPROCESSOR
#error MULTIPROCESSOR not supported
#endif

/*
 * u_int
 * _atomic_cas_uint(volatile u_int *val, u_int old, u_int new);
 *
 */

ENTRY(_atomic_cas_uint)
	mov	r9, r12

1:	ssrf	AVR32_STATUS_LOCK_SHIFT
	ld.w	r12, r9[0]
	cp.w	r12, r11
	brne	2f
	stcond	r9[0], r10
	brne	1b

2:	mov	pc, lr
END(_atomic_cas_uint)

STRONG_ALIAS(atomic_cas_uint,_atomic_cas_uint)
STRONG_ALIAS(atomic_cas_ulong,_atomic_cas_uint)
STRONG_ALIAS(_atomic_cas_ulong,_atomic_cas_uint)
STRONG_ALIAS(_atomic_cas_32,_atomic_cas_uint)
STRONG_ALIAS(atomic_cas_32,_atomic_cas_uint)
STRONG_ALIAS(_atomic_cas_ptr,_atomic_cas_uint)
STRONG_ALIAS(atomic_cas_ptr,_atomic_cas_uint)

STRONG_ALIAS(_atomic_cas_ulong_ni,_atomic_cas_uint)
STRONG_ALIAS(atomic_cas_ulong_ni,_atomic_cas_uint)
STRONG_ALIAS(_atomic_cas_32_ni,_atomic_cas_uint)
STRONG_ALIAS(atomic_cas_32_ni,_atomic_cas_uint)
STRONG_ALIAS(_atomic_cas_uint_ni,_atomic_cas_uint)
STRONG_ALIAS(atomic_cas_uint_ni,_atomic_cas_uint)
STRONG_ALIAS(_atomic_cas_ptr_ni,_atomic_cas_uint)
STRONG_ALIAS(atomic_cas_ptr_ni,_atomic_cas_uint)
