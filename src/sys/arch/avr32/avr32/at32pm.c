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

/*
 * This source code file has been inspired by the FreeBSD/avr32 version of
 * the Atmel Power Manager driver, which contains the following information:
 *
 * Copyright (c) 2009 Arnar Mar Sig
 * Copyright (c) 2009 Ulf Lilleengen <lulf@FreeBSD.org>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Power Manager. Main purpose for now is to reserve pm memory so
 * no other device tries to use it and make sure pm clock is enabled.
 * Later on it would be nice to export clock tree info to userspace and
 * allow changing clocks/frequency from userspace
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
#include <sys/malloc.h>
#include <sys/bus.h>

#include <machine/autoconf.h>
#include <machine/sysconf.h>
#include <machine/bus_space_avr32.h>
#include <machine/at32bus.h>
#include <machine/at32pm.h>

#define AT32PM_BASE     0xfff00000U
#define AT32PM_SPAN     0x00000080U

#define RD4(off) \
	bus_space_read_4(sc->sc_regt, sc->sc_regh, (off))

#define WR4(off, val) \
	bus_space_write_4(sc->sc_regt, sc->sc_regh, (off), (val))

/*
 * Private clock data.
 */
struct at32pm_clk {
	uint16_t mask;
	uint16_t index;
};

typedef struct at32pm_softc {
	struct device  sc_dev;

	/* Register space */
	bus_space_tag_t		sc_regt;
	bus_space_handle_t	sc_regh;
} at32pm_softc;

static int	at32pm_match(struct device *, struct cfdata *, void *);
static void	at32pm_attach(struct device *, struct device *, void *);

CFATTACH_DECL(at32pm, sizeof(struct at32pm_softc),
    at32pm_match, at32pm_attach, NULL, NULL);

#define at32pm_found \
	(at32pm_sc != NULL ? 1 : 0)
static struct at32pm_softc *at32pm_sc;

static int
at32pm_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct at32bus_attach_args *aa = aux;

	if (at32pm_found || strcmp(aa->aa_name, "at32pm"))
		return (0);

	return (1);
}

static void
at32pm_attach(struct device *parent, struct device *self, void *aux)
{
	struct at32bus_attach_args *aa = (void *)aux;
	struct at32pm_softc *sc = (struct at32pm_softc *)self;
	uint32_t mask;

	printf("\n");

	sc->sc_regt = aa->aa_regt;
	if (bus_space_alloc(sc->sc_regt,
			    AT32PM_BASE, AT32PM_BASE + AT32PM_SPAN,
			    AT32PM_SPAN, AT32PM_SPAN,
			    0, 0, 0, &sc->sc_regh))
		panic("at32pm_attach: cannot alloc device registers");

	at32pm_sc = sc;
}

at32pm_clk *
at32pm_alloc_clk(device_t dev, const char *name)
{
	static const char *cpumask[] = {
		"PICO"
	};
	static const char *hsbmask[] = {
		"EBI",
		"PBA",
		"PBB",
		"HRAMC",
		"HSB",
		"ISI",
		"USB",
		"LCDC",
		"MACB0",
		"DMA"
	};
	static const char *pbamask[] = {
		"SPI0",
		"SPI1",
		"TWI",
		"USART0",
		"USART1",
		"USART2",
		"USART3",
		"SSC0",
		"SSC1",
		"SSC2",
		"PIOA",
		"PIOB",
		"PIOC",
		"PIOD",
		"PIOE",
		"PSIF",
		"PDC"
	};
	static const char *pbbmask[] = {
		"PM",
		"INTC",
		"HMATRIX",
		"TC0",
		"TC1",
		"PWM",
		"MACB0",
		"MACB1",
		"DAC",
		"MCI",
		"AC97C",
		"ISI",
		"USB",
		"SMC",
		"SDRAMC",
		"ECC"
	};
	at32pm_clk *clk;
	uint16_t mask;
	int i;

#define LEN(x) \
	(sizeof(x) / sizeof(x[0]))

	/* Lookup if we have the clock. */
	for (i = 0; i < LEN(cpumask); i++) {
		if (strcmp(cpumask[i], name) == 0) {
			mask = AT32PM_CPUMASK;
			goto found;
		}
	}

	for (i = 0; i < LEN(hsbmask); i++) {
		if (strcmp(hsbmask[i], name) == 0) {
			mask = AT32PM_HSBMASK;
			goto found;
		}
	}

	for (i = 0; i < LEN(pbamask); i++) {
		if (strcmp(pbamask[i], name) == 0) {
			mask = AT32PM_PBAMASK;
			goto found;
		}
	}

	for (i = 0; i < LEN(pbbmask); i++) {
		if (strcmp(pbbmask[i], name) == 0) {
			mask = AT32PM_PBBMASK;
			goto found;
		}
	}

	/* Not found.  */
	return (NULL);
found:
	clk = malloc(sizeof(struct at32pm_clk), 
		     M_DEVBUF, M_ZERO | M_WAITOK);
	if (clk == NULL)
		return (NULL);
	clk->mask = mask;
	clk->index = i;
	return (clk);
}

void
at32pm_free_clk(at32pm_clk *clk)
{
	free(clk, M_DEVBUF);
}

void
at32pm_clk_enable(struct at32pm_clk *clk)
{
	at32pm_softc *sc = at32pm_sc;
	uint32_t reg;

	reg = RD4(clk->mask);
	WR4(clk->mask, reg | (1 << clk->index));
}

void
at32pm_clk_disable(struct at32pm_clk *clk)
{
	at32pm_softc *sc = at32pm_sc;
	uint32_t reg;

	reg = RD4(clk->mask);
	WR4(clk->mask, reg & ~(1 << clk->index));
}
