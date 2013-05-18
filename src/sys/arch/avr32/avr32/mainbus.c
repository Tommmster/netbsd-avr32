/* $NetBSD: mainbus.c,v 1.36 2005/12/11 12:18:39 christos Exp $ */

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * DECstation port: Jonathan Stone
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.36 2005/12/11 12:18:39 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <machine/sysconf.h>
#include <machine/autoconf.h>
#include <machine/bus_space_avr32.h>

/* Definition of the mainbus driver. */
static int mainbus_match(struct device *, struct cfdata *, void *);
static void mainbus_attach(struct device *, struct device *, void *);
static int mainbus_search(struct device *, struct cfdata *,
			  const int *, void *);
static int mainbus_print(void *, const char *);

CFATTACH_DECL(mainbus, sizeof(struct device),
    mainbus_match, mainbus_attach, NULL, NULL);

static int mainbus_attached;

static int
mainbus_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{

	return (mainbus_attached ? 0 : 1);	/* don't attach twice */
}

static void
mainbus_attach(struct device *parent, struct device *self, void *aux)
{
	static const const char *devnames[] = {		/* ATTACH ORDER */
		"cpu",					/* 1. CPU */
		"at32bus",				/* 2. System BUS */
	};
	struct mainbus_attach_args ma;
	int i;

	mainbus_attached = 1;

	printf("\n");

	/* search and attach devices in order */
	for (i = 0; i < sizeof(devnames) / sizeof(devnames[0]); i++) {
		ma.ma_name = devnames[i];
		config_search_ia(mainbus_search, self, "mainbus", &ma);
	}
}

static int
mainbus_search(struct device *parent, struct cfdata *cf,
	       const int *ldesc, void *aux)
{
	struct mainbus_attach_args *ma = (void *)aux;
#if 0
	int locator = cf->cf_loc[MAINBUSCF_PLATFORM];
#endif
	/* check device name */
	if (strcmp(ma->ma_name, cf->cf_name) != 0)
		return (0);
#if 0
	/* check platform ID in config file */
	if (locator != MAINBUSCF_PLATFORM_DEFAULT &&
	    !platid_match(&platid, PLATID_DEREFP(locator)))
		return (0);
#endif
	/* attach device */
	if (config_match(parent, cf, ma))
		config_attach(parent, cf, ma, mainbus_print);

	return (0);
}

static int
mainbus_print(void *aux, const char *pnp)
{

	return (pnp ? QUIET : UNCONF);
}
