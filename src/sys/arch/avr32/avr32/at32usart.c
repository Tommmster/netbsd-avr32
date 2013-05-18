/* 	$NetBSD$	*/

/*-
 * Copyright (c) 2007 Ruslan Ermilov and Vsevolod Lobko.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The names of the authors may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/device.h>

#include <sys/proc.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/kauth.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/conf.h>
#include <sys/vnode.h>
#include <sys/bus.h>

#include <machine/autoconf.h>
#include <machine/sysconf.h>
#include <machine/at32intc.h>
#include <machine/at32bus.h>
#include <machine/at32pm.h>

#include <dev/cons.h>
#include <avr32/cpuregs.h>

#define RD4(off) \
	bus_space_read_4(sc->sc_regt, sc->sc_regh, (off))

#define WR4(off, val) \
	bus_space_write_4(sc->sc_regt, sc->sc_regh, (off), (val))

cons_decl(at32usart_);

extern struct consdev *cn_tab;          /* physical console device info */

static dev_type_open(at32usart_open);
static dev_type_open(at32usart_close);
static dev_type_read(at32usart_read);
static dev_type_write(at32usart_write);
static dev_type_ioctl(at32usart_ioctl);
static dev_type_tty(at32usart_tty);
static dev_type_poll(at32usart_poll);
static dev_type_stop(at32usart_stop);

const struct cdevsw at32usart_cdevsw = {
	at32usart_open, at32usart_close, at32usart_read, at32usart_write, 
	at32usart_ioctl, at32usart_stop, at32usart_tty, at32usart_poll, 
	nommap, ttykqfilter, D_TTY,
};

struct consdev at32usart_cons = {
	NULL, NULL, at32usart_cngetc, at32usart_cnputc, at32usart_cnpollc, 
	NULL, NULL, NULL, NODEV, CN_REMOTE,
};

struct at32usart_softc {
	struct device  sc_dev;
	struct tty    *sc_tty;

	/* Register space */
	bus_space_tag_t		sc_regt;
	bus_space_handle_t	sc_regh;
};

static int at32usart_consattached;
extern struct cfdriver at32usart_cd;

static int at32usart_match(struct device *, struct cfdata *, void *);
static void at32usart_attach(struct device *, struct device *, void *);
static void at32usart_intr(void *, void *);

void	at32usart_start(struct tty *);
int	at32usart_param(struct tty *, struct termios *);

#define AT32_USART_CR		0x0000
#define AT32_USART_MR		0x0004
#define AT32_USART_IER		0x0008
#define AT32_USART_IDR		0x000c
#define AT32_USART_IMR		0x0010
#define AT32_USART_CSR		0x0014
#define AT32_USART_RHR		0x0018
#define AT32_USART_THR		0x001c

#define AT32_USART0_BASE	0xffe00c00U
#define AT32_USART1_BASE	0xffe01000U
#define AT32_USART1_SPAN	0x00000400U
#define AT32_USART2_BASE	0xffe01400U
#define AT32_USART3_BASE	0xffe01800U

#define AT32_USART_CR_RXEN	0x00000010U
#define AT32_USART_CR_TXEN	0x00000040U

#define AT32_USART_CSR_RXRDY	0x00000001U
#define AT32_USART_CSR_TXRDY	0x00000002U

#define AT32_USART_IDR_RXRDY	0x00000001U
#define AT32_USART_IDR_TXRDY	0x00000002U
	
#define AT32_USART_IER_RXRDY	0x00000001U
#define AT32_USART_IER_TXRDY	0x00000002U

CFATTACH_DECL(at32usart, sizeof(struct at32usart_softc),
    at32usart_match, at32usart_attach, NULL, NULL);

static int at32usart_found;

static int
at32usart_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct at32bus_attach_args *aa = aux;

	if (at32usart_found || strcmp(aa->aa_name, "at32usart"))
		return (0);

	return (1);
}

static void
at32usart_intr(void *victx, void *vuctx)
{
	struct at32usart_softc *sc = vuctx;
	struct tty *tp = sc->sc_tty;
	unsigned char ch;
	unsigned status;

	status = RD4(AT32_USART_CSR);
	while (status & AT32_USART_CSR_RXRDY) {
		ch = RD4(AT32_USART_RHR);
		if (tp->t_state & TS_ISOPEN)
			(*tp->t_linesw->l_rint)(ch, tp);
		status = RD4(AT32_USART_CSR);
	}
}

static void
at32usart_attach(struct device *parent, struct device *self, void *aux)
{
	struct at32bus_attach_args *aa = (void *)aux;
	struct at32usart_softc *sc = (struct at32usart_softc *)self;
	struct tty *tp;
	int maj, minor;

	sc->sc_regt = aa->aa_regt;
	if (bus_space_alloc(sc->sc_regt, 
			    AT32_USART1_BASE, 
			    AT32_USART1_BASE + AT32_USART1_SPAN, 
			    AT32_USART1_SPAN, AT32_USART1_SPAN,
			    0, 0, 0, &sc->sc_regh))
		panic("\nat32usart_attach: cannot alloc device registers");

	maj = cdevsw_lookup_major(&at32usart_cdevsw);
	minor = sc->sc_dev.dv_unit;

	tp = ttymalloc();
	tp->t_oproc = at32usart_start;
	tp->t_param = at32usart_param;
	sc->sc_tty = tp;
	tp->t_dev = makedev(maj, minor);
	tty_attach(tp);
	if (minor == 0 && at32usart_consattached) {
		/* attach as console */
		cn_tab->cn_dev = tp->t_dev;
		printf(" console");
	}

	/* Install the interrupt handler. */
	at32intc_intr_establish(AT32INTC_GRP_USART1,
				AT32INTC_IRQ_USART1,
				AT32INTC_IRQ_LEVEL_0,
				at32usart_intr,
				sc);

	/* XXX */
	WR4(AT32_USART_CR, AT32_USART_CR_RXEN);
	WR4(AT32_USART_IER, AT32_USART_IER_RXRDY);

	at32usart_found = 1;
	printf("\n");
}

int 
at32usart_cnattach(void)
{
	cn_tab = &at32usart_cons;
	at32usart_consattached = 1;

	return (0);
}

void
at32usart_cnputc(dev_t dev, int ch)
{
	uint32_t csr;
	uint32_t imr;

	/*
	 * XXX This routine runs early, long before hardware configuration
	 * takes place: do not use the bus layer to manipulate the USART, 
	 * instead we manipulate the hardware registers directly, using the
	 * serial port configuration inherited from the bootloader.
	 */

#define EARLY_RD4(reg) \
	(*((volatile uint32_t *)(AT32_USART1_BASE + (reg))))

#define EARLY_WR4(reg, val) \
	(*((volatile uint32_t *)(AT32_USART1_BASE + (reg)))) = (val)

	/* Save a copy of the IMR and then disable interrupts. */
	imr = EARLY_RD4(AT32_USART_IMR);
	EARLY_WR4(AT32_USART_IDR, 
		  AT32_USART_IDR_RXRDY | AT32_USART_IDR_TXRDY);

	/* Wait for the transmitter to become ready. */
	do {
		csr = EARLY_RD4(AT32_USART_CSR);
	} while ((csr & AT32_USART_CSR_TXRDY) == 0);

	/* Trasmitter ready to accept info. */
	EARLY_WR4(AT32_USART_THR, ch);

	/* Wait for transmitter to become empty. */
	do {
		csr = EARLY_RD4(AT32_USART_CSR);
	} while ((csr & AT32_USART_CSR_TXRDY) == 0);

	/* Set interrupts back the way they were. */
	EARLY_WR4(AT32_USART_IER, imr);
}

int
at32usart_cngetc(dev_t dev)
{
	panic("at32usart_cngetc: notyet");
	return (0);
}

void
at32usart_cnpollc(dev_t dev, int on)
{
	panic("at32usart_cnpollc: notyet");
}

/*
 * TTY device
 */

int
at32usart_open(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct at32usart_softc *sc = device_lookup_private(&at32usart_cd, 
							   minor(dev));
	struct tty *tp = sc->sc_tty;
	int error = 0;
	int s;

	s = spltty();

	tp->t_dev = dev;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		tp->t_state |= TS_CARR_ON;
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = TTYDEF_CFLAG | CLOCAL;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = 115200;
		ttsetwater(tp);
	} else if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN,
	    tp) != 0) {
		splx(s);
		return (EBUSY);
	}

	splx(s);

	error = (*tp->t_linesw->l_open)(dev, tp);

	return (error);
}

int
at32usart_close(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct at32usart_softc *sc = device_lookup_private(&at32usart_cd, 
							   minor(dev));
	struct tty *tp = sc->sc_tty;

	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);
	return (0);
}

int
at32usart_read(dev_t dev, struct uio *uio, int flag)
{
	struct at32usart_softc *sc = device_lookup_private(&at32usart_cd, 
							   minor(dev));
	struct tty *tp = sc->sc_tty;

	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}
 
int
at32usart_write(dev_t dev, struct uio *uio, int flag)
{
	struct at32usart_softc *sc = device_lookup_private(&at32usart_cd, 
							   minor(dev));
	struct tty *tp = sc->sc_tty;
 
	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

int
at32usart_poll(dev_t dev, int events, struct lwp *l)
{
	struct at32usart_softc *sc = device_lookup_private(&at32usart_cd, 
							   minor(dev));
	struct tty *tp = sc->sc_tty;
 
	return ((*tp->t_linesw->l_poll)(tp, events, l));
}

int
at32usart_ioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct at32usart_softc *sc = device_lookup_private(&at32usart_cd, 
							   minor(dev));
	struct tty *tp = sc->sc_tty;
	int error;

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return (error);
	return (ttioctl(tp, cmd, data, flag, l));
}

int
at32usart_param(struct tty *tp, struct termios *t)
{
	return (0);
}

struct tty*
at32usart_tty(dev_t dev)
{
	struct at32usart_softc *sc = device_lookup_private(&at32usart_cd, 
							   minor(dev));
	return sc->sc_tty;
}

void
at32usart_start(struct tty *tp)
{
	int cnt;
	int i;
	int s;

	s = spltty();
	if (tp->t_state & (TS_TTSTOP | TS_BUSY))
		goto out;
	ttypull(tp);
	tp->t_state |= TS_BUSY;
	while (tp->t_outq.c_cc != 0) {
		cnt = ndqb(&tp->t_outq, 0);
		for (i=0; i<cnt; i++)
			at32usart_cnputc(0, tp->t_outq.c_cf[i]);
		ndflush(&tp->t_outq, cnt);
	}
	tp->t_state &= ~TS_BUSY;
 out:
	splx(s);
}

void
at32usart_stop(struct tty *tp, int flag)
{
	int s;

	s = spltty();
	if (tp->t_state & TS_BUSY)
		if ((tp->t_state & TS_TTSTOP) == 0)
			tp->t_state |= TS_FLUSH;
	splx(s);
}
