/* $NetBSD: linux_signal.h,v 1.13 2008/04/28 20:23:43 martin Exp $ */

/*-
 * Copyright (c) 1995, 1998, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden, Eric Haszlakiewicz and Emmanuel Dreyfus.
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

#ifndef _AVR32_LINUX_SIGNAL_H
#define _AVR32_LINUX_SIGNAL_H

/*
 * From Linux's include/asm-avr32/ptrace.h
 */
struct linux_pt_regs {
   unsigned long sr;
   unsigned long pc;
   unsigned long lr;
   unsigned long sp;
   unsigned long r12;
   unsigned long r11;
   unsigned long r10;
   unsigned long r9;
   unsigned long r8;
   unsigned long r7;
   unsigned long r6;
   unsigned long r5;
   unsigned long r4;
   unsigned long r3;
   unsigned long r2;
   unsigned long r1;
   unsigned long r0;
        
   unsigned long r12_orig;
};

/*
 * Everything is from Linux's include/asm-avr32/signal.h
 */
#define LINUX_SIGHUP	 	1
#define LINUX_SIGINT		2
#define LINUX_SIGQUIT		3
#define LINUX_SIGILL		4
#define LINUX_SIGTRAP		5
#define LINUX_SIGABRT		6
#define LINUX_SIGIOT		6
#define LINUX_SIGBUS		7	
#define LINUX_SIGFPE		8
#define LINUX_SIGKILL		9
#define LINUX_SIGUSR1		10
#define LINUX_SIGSEGV		11
#define LINUX_SIGUSR2		12
#define LINUX_SIGPIPE		13
#define LINUX_SIGALRM		14
#define LINUX_SIGTERM		15
#define LINUX_SIGSTKFLT		16
#define LINUX_SIGCHLD		17
#define LINUX_SIGCONT		18
#define LINUX_SIGSTOP		19
#define LINUX_SIGTSTP		20
#define LINUX_SIGTTIN		21
#define LINUX_SIGTTOU		22
#define LINUX_SIGURG		23
#define LINUX_SIGXCPU		24
#define LINUX_SIGXFSZ		25
#define LINUX_SIGVTALRM		26
#define LINUX_SIGPROF		27
#define LINUX_SIGWINCH		28
#define LINUX_SIGIO		29
#define LINUX_SIGPOLL		29
#define LINUX_SIGPWR		30
#define LINUX_SIGSYS		31
#define LINUX_SIGUNUSED		31

/* Min/max real-time linux signal */
#define LINUX_SIGRTMIN		32
#define LINUX_SIGRTMAX		(LINUX__NSIG - 1)

#define LINUX__NSIG		64
#define LINUX__NSIG_BPW		32
#define LINUX__NSIG_WORDS (LINUX__NSIG / LINUX__NSIG_BPW)

#define LINUX_SIG_BLOCK		0
#define LINUX_SIG_UNBLOCK	1
#define LINUX_SIG_SETMASK	2

/* sa_flags */
#define LINUX_SA_NOCLDSTOP	0x00000001
#define LINUX_SA_NOCLDWAIT	0x00000002
#define LINUX_SA_SIGINFO	0x00000004
#define LINUX_SA_RESTORER	0x04000000
#define LINUX_SA_ONSTACK	0x08000000
#define LINUX_SA_RESTART	0x10000000
#define LINUX_SA_NODEFER	0x40000000
#define LINUX_SA_RESETHAND	0x80000000
#define LINUX_SA_NOMASK		LINUX_SA_NODEFER
#define LINUX_SA_ONESHOT	LINUX_SA_RESETHAND
#define LINUX_SA_ALLBITS	0xdc010007

#define	LINUX_MINSIGSTKSZ	2048

typedef void (*linux___sighandler_t)(int);

typedef unsigned long linux_old_sigset_t;
typedef struct {
	unsigned long sig[LINUX__NSIG_WORDS];
} linux_sigset_t;

/*
 * include/asm-avr32/signal.h
 */ 
struct linux_sigaction {
	linux___sighandler_t	linux_sa_handler;
	unsigned long		linux_sa_flags;
	void			(*linux_sa_restorer)(void);
	linux_sigset_t		linux_sa_mask;
};

struct linux_k_sigaction {
	struct linux_sigaction sa;
};

/*
 * include/asm-avr32/signal.h
 */ 
struct linux_old_sigaction {
	linux___sighandler_t	linux_sa_handler;
	linux_old_sigset_t	linux_sa_mask;
	unsigned long		linux_sa_flags;
	void			(*linux_sa_restorer)(void);
};

#define	LINUX_SS_ONSTACK	1
#define	LINUX_SS_DISABLE	2

#define	LINUX_MINSIGSTKSZ	2048
#define	LINUX_SIGSTKSZ		8192

struct linux_sigaltstack {
	void *ss_sp;
	int ss_flags;
	size_t ss_size;
};
typedef struct linux_sigaltstack linux_stack_t; /* XXX really needed ? */

#endif /* !_AVR32_LINUX_SIGNAL_H */
