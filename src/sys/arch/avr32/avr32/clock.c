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
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/timetc.h>
#include <sys/cpu.h>

#include <avr32/sysconf.h>
#include <avr32/locore.h>

/* 
 * platform_clock_attach:
 *
 *      Register hardware-dependent clock routine to system.
 */
void
platform_clock_attach(void *ctx, struct platform_clock *clock)
{
	clock->self = ctx;
	platform.clock = clock;
}

/*
 * cpu_initclocks:
 *
 *      starts periodic timer, which provides hardclock interrupts to
 *      kern_clock.c.
 *      Leave stathz 0 since there are no other timers available.
 */
void
cpu_initclocks(void)
{
	struct platform_clock *clock = platform.clock;

	if (clock == NULL)
		panic("cpu_initclocks: no clock attached");

	hz = clock->hz;
	tick = 1000000 / hz;

	/* start periodic timer */
	(*clock->init)(clock->self);
}

/*
 * setstatclockrate:
 *
 *      We assume newhz is either stathz or profhz, and that neither will
 *      change after being set up above.  Could recalculate intervals here
 *      but that would be a drag.
 */
void
setstatclockrate(int newhz)
{
	/* nothing we can do */
}
