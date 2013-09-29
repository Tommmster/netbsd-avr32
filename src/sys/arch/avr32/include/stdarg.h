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

#ifndef _AVR32_STDARG_H_
#define _AVR32_STDARG_H_

#include <machine/ansi.h> 

typedef _BSD_VA_LIST_ va_list;

#ifdef __lint__

#define va_start(ap, last)	((ap) = *(va_list *)0)
#define va_arg(ap, type)	(*(type *)(void *)&(ap))
#define va_end(ap)
#define __va_copy(dest, src)	((dest) = (src))

#elif __GNUC_PREREQ__(3, 0)

#define va_start(ap, last)	__builtin_stdarg_start((ap), last)
#define va_arg(ap, type)	__builtin_va_arg((ap), type)
#define va_end(ap)		__builtin_va_end((ap))
#define __va_copy(dest, src)	__builtin_va_copy((dest), (src))

#elif defined(__PCC__)

#define va_start(ap, last)	__builtin_stdarg_start((ap), last)
#define va_arg(ap, type)	__builtin_va_arg((ap), type)
#define va_end(ap)		__builtin_va_end((ap))
#define __va_copy(dest, src)	__builtin_va_copy((dest), (src))

#else


#define	va_start(ap, last) \
	((ap) = (va_list)__builtin_next_arg(last))

#if BYTE_ORDER == LITTLE_ENDIAN
#define	va_arg(ap, T)							\
	(((T *)(							\
	    (ap) += (/*CONSTCOND*/ __alignof__(T) <= sizeof(int)	\
		? sizeof(int) : ((long)(ap) & 4) + sizeof(T)),		\
	    (ap) - (/*CONSTCOND*/ __alignof__(T) <= sizeof(int)		\
		? sizeof(int) : sizeof(T))				\
 	))[0])
#else
#define	va_arg(ap, T)							\
	(((T *)(							\
	    (ap) += (/*CONSTCOND*/ __alignof__(T) <= sizeof(int)	\
		? sizeof(int) : ((long)(ap) & 4) + sizeof(T))		\
 	))[-1])
#endif

#define	va_end(ap)

#define	__va_copy(dest, src)	((dest) = (src))

#endif

#endif /* !_AVR32_STDARG_H_ */
