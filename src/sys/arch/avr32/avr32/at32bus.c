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

#include <machine/autoconf.h>
#include <machine/sysconf.h>
#include <machine/bus_space_avr32.h>
#include <machine/at32bus.h>

struct at32bus_softc {
	struct device sc_dev;
	int sc_pri; /* attaching device priority */
};

static int	at32bus_match(struct device *, struct cfdata *, void *);
static void	at32bus_attach(struct device *, struct device *, void *);
static int	at32bus_print(void *, const char*);
static int	at32bus_search(struct device *, struct cfdata *,
			       const int *, void *);

CFATTACH_DECL(at32bus, sizeof(struct at32bus_softc),
    at32bus_match, at32bus_attach, NULL, NULL);

static int
at32bus_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (strcmp(ma->ma_name, match->cf_name))
		return (0);

	return (1);
}

static void
at32bus_attach(struct device *parent, struct device *self, void *aux)
{
	static const const char *devnames[] = {		/* ATTACH ORDER */
		"at32pm",				/* 1. Power Mgr */
		"at32intc",				/* 2. Interrupts */
#ifdef notyet
		"at32usart",				/* 3. USART */
#endif
		"at32clock",				/* 4. Clock */
	};
	struct at32bus_attach_args aa;
	int i;

	printf("\n");

	/* system bus_space */
	aa.aa_regt = avr32_system_bus_space();
	avr32_init_bus_space((struct bus_space_tag_avr32 *)aa.aa_regt,
		NULL, "at32 bus", 0, ~0);

	/* search and attach devices in order */
	for (i = 0; i < sizeof(devnames) / sizeof(devnames[0]); i++) {
		aa.aa_name = devnames[i];
		config_search_ia(at32bus_search, self, "at32bus", &aa);
	}
}

static int
at32bus_print(void *aux, const char *pnp)
{
	return (pnp ? QUIET : UNCONF);
}

static int
at32bus_search(struct device *parent, struct cfdata *cf,
	       const int *ldesc, void *aux)
{
	struct at32bus_attach_args *aa = (void *)aux;

	/* check device name */
	if (strcmp(aa->aa_name, cf->cf_name) != 0)
		return (0);

	/* attach device */
	if (config_match(parent, cf, aa))
		config_attach(parent, cf, aa, at32bus_print);

	return (0);
}
