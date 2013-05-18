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
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <avr32/intr.h>
#include <avr32/locore.h>

#include <machine/autoconf.h>
#include <machine/sysconf.h>
#include <machine/bus_space_avr32.h>
#include <machine/at32intc.h>
#include <machine/at32bus.h>
#include <machine/at32pm.h>

#include <avr32/intr.h>

#define AT32INTC_ICR_CAUSE_MASK	0x0000003f
#define AT32INTC_IPR0           0x0000
#define AT32INTC_ICR0		0x020c
#define AT32INTC_ICR1		0x0208
#define AT32INTC_ICR2		0x0204
#define AT32INTC_ICR3		0x0200
#define AT32INTC_IRR0		0x0100
#define AT32INTC_NIRQS		2048

#define AT32INTC_IPR(x) \
	(AT32INTC_IPR0 + ((x) << 2))

#define AT32INTC_IRR(x) \
	(AT32INTC_IRR0 + ((x) << 2))

#define AT32INTC_ICR(x) \
	(AT32INTC_ICR0 - ((x) << 2))

#define AT32INTC_BASE     0xfff00400U
#define AT32INTC_SPAN     0x00000400U

#define RD4(off) \
	bus_space_read_4(sc->sc_regt, sc->sc_regh, (off))

#define WR4(off, val) \
	bus_space_write_4(sc->sc_regt, sc->sc_regh, (off), (val))

#define LEN(x) \
	(sizeof(x) / sizeof(x[0]))

struct at32intc_softc {
	struct device  sc_dev;

	/* Register space */
	bus_space_tag_t		sc_regt;
	bus_space_handle_t	sc_regh;

	/* Device clock. */
	at32pm_clk *sc_clk;
};

struct at32intc_line {
	int line;
	int level;
	int group;
	void (*handler)(void *, void *);
	void *arg;
};

extern char _evba[];
extern char avr32_intr_0[];
extern char avr32_intr_1[];
extern char avr32_intr_2[];
extern char avr32_intr_3[];

static int	at32intc_match(struct device *, struct cfdata *, void *);
static void	at32intc_attach(struct device *, struct device *, void *);
static void	at32intc_cpu_intr(int, uint32_t, uint32_t);

CFATTACH_DECL(at32intc, sizeof(struct at32intc_softc),
    at32intc_match, at32intc_attach, NULL, NULL);

#define at32intc_found \
	(at32intc_sc != NULL ? 1 : 0)
static struct at32intc_softc *at32intc_sc;
static struct at32intc_line at32intc_lines[AT32INTC_NIRQS];

#define AT32INTC_IPR_LEVEL_SHIFT 30
#define AT32INTC_IPR_VECTOR_MASK  0x3fff

#define AT32INTC_IPR_LEVEL(l) \
	((l) << AT32INTC_IPR_LEVEL_SHIFT)

#define AT32INTC_IPR_VECTOR(l) \
	((((uint32_t)(&avr32_intr_ ## l) - (uint32_t)&_evba)) & AT32INTC_IPR_VECTOR_MASK)

#define AT32INTC_IPR_LEVEL_0 \
	(AT32INTC_IPR_LEVEL(0) | AT32INTC_IPR_VECTOR(0))

#define AT32INTC_IPR_LEVEL_1 \
	(AT32INTC_IPR_LEVEL(1) | AT32INTC_IPR_VECTOR(1))

#define AT32INTC_IPR_LEVEL_2 \
	(AT32INTC_IPR_LEVEL(2) | AT32INTC_IPR_VECTOR(2))

#define AT32INTC_IPR_LEVEL_3 \
	(AT32INTC_IPR_LEVEL(3) | AT32INTC_IPR_VECTOR(3))

#define AT32AP7000_IPR_GROUP_COUNT 0

#define AT32INTC_NR_IPR 64

static int
at32intc_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct at32bus_attach_args *aa = aux;

	if (at32intc_found || strcmp(aa->aa_name, "at32intc"))
		return (0);

	return (1);
}

static void
at32intc_attach(struct device *parent, struct device *self, void *aux)
{
	struct at32bus_attach_args *aa = (void *)aux;
	struct at32intc_softc *sc = (struct at32intc_softc *)self;
	int i;

	printf("\n");

	sc->sc_regt = aa->aa_regt;
	if (bus_space_alloc(sc->sc_regt,
			    AT32INTC_BASE, AT32INTC_BASE + AT32INTC_SPAN,
			    AT32INTC_SPAN, AT32INTC_SPAN,
			    0, 0, 0, &sc->sc_regh))
		panic("at32intc_attach: cannot alloc device registers");

	sc->sc_clk = at32pm_alloc_clk(self, "INTC");
	if (sc->sc_clk == NULL)
		panic("at32intc_attach: cannot allocate device clock");

	/* Enable INTC clock in PM PBB. */
	at32pm_clk_enable(sc->sc_clk);

	/* Map every group to interrupt level 0. */
	for (i = 0; i < AT32INTC_NR_IPR; i++) {
		WR4(AT32INTC_IPR(i), AT32INTC_IPR_LEVEL_0);
	}

	/* Except for the COUNT group, which gets high priority. */
	WR4(AT32INTC_IPR(AT32AP7000_IPR_GROUP_COUNT), 
			 AT32INTC_IPR_LEVEL_2);

	/* Success. */
	at32intc_sc = sc;
}

void *
at32intc_intr_establish(int group, int line, int ilvl, 
		        at32intc_handler *ihnd, void *iarg)
{
	struct at32intc_line *il;
	int index = line + (group << 5);
	int prev;

	if (!at32intc_found)
		panic("at32intc_intr_establish: unconfigured");

	if (index >= AT32INTC_NR_IRQS)
		panic("at32intc_intr_establish: bogus interrupt spec");

	if (ilvl > AT32INTC_IRQ_LEVEL_3)
		panic("at32intc_intr_establish: bogus interrupt level");

	/* Annotate the interrupt line context. */
	il = &at32intc_lines[index];
	if (il->handler != NULL)
		panic("at32intc_intr_establish: line %d already registered",
			line);

	/* 
	 * Register this interrupt line. Link the main architecture-wide
	 * low-level interrupt handle with our interrupt dispatch routine.
	 */
	prev = splhigh();
	cpu_intr = at32intc_cpu_intr;
	il->group = group;
	il->line = line;
	il->level = ilvl;
	il->handler = ihnd;
	il->arg = iarg;
	splx(prev);

	return (void *)il;
}

/*
 * NB: Do not re-enable interrupts here -- reentrancy here can cause all
 * sorts of Bad Things(tm) to happen, including kernel stack overflows.
 */
static void
at32intc_cpu_intr(int ilvl, uint32_t st, uint32_t pc)
{
	struct at32intc_softc *sc = at32intc_sc;
	struct at32intc_line *il;
	struct at32intc_ictx ictx;
	struct cpu_info *ci;
	uint32_t icrx;
	uint32_t irrx;
	uint32_t igrp;
	int line;
	int b;

	ci = curcpu();
	ci->ci_idepth++;
	uvmexp.intrs++;

	ictx.st = st;
	ictx.pc = pc;

	if (ilvl >= 4)
		panic("at32intc_cpu_intr: bad interrupt level");

	icrx = RD4(AT32INTC_ICR(ilvl));
	igrp = icrx & AT32INTC_ICR_CAUSE_MASK;
	irrx = RD4(AT32INTC_IRR(igrp));

	if (irrx == 0)
		panic("cpu_intr: no pending group %u level %d interrupts", 
			igrp, ilvl);

	for (b = 0; b < 32; ++b) {
		if ((irrx & (1U << b)) == 0)
			continue;

		il = &at32intc_lines[b + (igrp << 5)];

		if (il->handler == NULL)
			panic("at32_intc_cpu_intr: no irq handler for ICR %x IRR %x",
				(unsigned)icrx, (unsigned)irrx);

		if (il->level != ilvl)
			panic("at32intc_cpu_intr: interrupt level mismatch");

		if (il->group != igrp)
			panic("at32intc_cpu_intr: interrupt level mismatch");

		il->handler(&ictx, il->arg);
	}

	ci->ci_idepth--;
#ifdef __HAVE_FAST_SOFTINTS
#error __HAVE_FAST_SOFTINTS: notyet
	/* software interrupt */
	ipending &= (MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0);
	if (ipending == 0)
		return;
	_clrsoftintr(ipending);
	softintr_dispatch(ipending);
#endif
}
