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
__KERNEL_RCSID(0, "$NetBSD$");

/*
 * Which system models were configured?
 */
#include "opt_atngw100.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/timetc.h>
#include <sys/bus.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/sysconf.h>
#include <machine/bus_space_avr32.h>
#include <machine/at32intc.h>
#include <machine/at32bus.h>

#include <avr32/intr.h>

#ifndef AT32CLOCK_RATE
#define AT32CLOCK_RATE 100
#endif

static void at32clock_init(struct device *);
static void at32clock_init_tc(void);

static struct platform_clock at32clock = {
	AT32CLOCK_RATE, 
	at32clock_init,
};

struct at32clock_softc {
	struct device  sc_dev;

	/* Regisat32ter space */
	bus_space_tag_t		sc_regt;
	bus_space_handle_t	sc_regh;
};

static int at32clock_match(struct device *, struct cfdata *, void *);
static void at32clock_attach(struct device *, struct device *, void *);

CFATTACH_DECL(at32clock, sizeof(struct at32clock_softc),
    at32clock_match, at32clock_attach, NULL, NULL);

#define at32clock_found \
	(at32clock_sc != NULL ? 1 : 0)
static struct at32clock_softc *at32clock_sc;

static struct evcnt at32clock_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "at32clock", "compare (clock)");

static struct evcnt at32clock_missed_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "at32clock", "missed compare int");

/* Used to schedule clock interrupts */
static uint32_t next_clk_intr;

static int
at32clock_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct at32bus_attach_args *aa = aux;

	if (at32clock_found || strcmp(aa->aa_name, "at32clock"))
		return (0);

	return (1);
}

static void
at32clock_attach(struct device *parent, struct device *self, void *aux)
{
#if 0
	struct at32bus_attach_args *aa = (void *)aux;
	struct at32clock_softc *sc = (struct at32clock_softc *)self;
#endif
	printf("\n");
	platform_clock_attach(self, &at32clock);
}

/*
 * Handling to be done upon receipt of an AVR32 COMPARE interrupt. This
 * routine is to be called from the master interrupt routine (e.g. cpu_intr),
 * if the COMPARE interrupt is pending.  The caller is responsible for 
 * blocking and renabling the interrupt in the cpu_intr() routine.
 */
static void
at32clock_intr(void *victx, void *vuctx)
{
	struct at32intc_ictx *ictx = victx;
	struct clockframe cf;
	uint32_t new_cnt;

	next_clk_intr += curcpu()->ci_cycles_per_hz;
	avr32_compare_write(next_clk_intr);

	/* Check for lost clock interrupts. */
	new_cnt = avr32_count_read();

	/* 
	 * Missed one or more clock interrupts, so let's start 
	 * counting again from the current value.
	 */
	if ((next_clk_intr - new_cnt) & 0x80000000) {
		next_clk_intr = new_cnt + curcpu()->ci_cycles_per_hz;
		avr32_compare_write(next_clk_intr);
		at32clock_missed_evcnt.ev_count++;
	}

	/* Tick the clock. */
	cf.st = ictx->st;
	cf.pc = ictx->pc;
	hardclock(&cf);
	/* Update statistics. */
	at32clock_evcnt.ev_count++;
}

static void
at32clock_init(struct device *dev)
{
#if 0
	struct at32clock_softc *sc = (void*)dev;
#endif
	evcnt_attach_static(&at32clock_evcnt);

	/* Install the interrupt handler. */
	at32intc_intr_establish(AT32INTC_GRP_COUNT,
				AT32INTC_IRQ_COUNT, 
	                        AT32INTC_IRQ_LEVEL_2, 
	                        at32clock_intr,
	                        NULL);

	/* 
	 * Setup periodic timer (interrupting hz times per second.) 
	 */
	next_clk_intr = avr32_count_read();
	curcpu()->ci_cpu_freq = 200 * 1000 * 1000;
	curcpu()->ci_cycles_per_hz = curcpu()->ci_cpu_freq / (2 * hz);
	curcpu()->ci_divisor_delay = curcpu()->ci_cpu_freq / (2 * 1000000);

	next_clk_intr = avr32_count_read() + curcpu()->ci_cycles_per_hz;
	avr32_compare_write(next_clk_intr);

	/* Initialize timecounter infrastructure. */
	at32clock_init_tc();

	/* Now we can enable all interrupts including hardclock(9). */
	spl0();
}

/*
 * Support for using the AVR32 COMPARE clock as a timecounter.
 */
static void
at32clock_init_tc(void)
{
	static struct timecounter tc =  {
		(timecounter_get_t *)avr32_count_read,	/* get_timecount */
		0,					/* no poll_pps */
		~0u,					/* counter_mask */
		0,					/* frequency */
		"at32clock_counter",			/* name */
		100,					/* quality */
	};

	tc.tc_frequency = curcpu()->ci_cpu_freq;
	tc_init(&tc);
}
