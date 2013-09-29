#ifndef _AVR32_LINUX_MACHDEP_H
#define _AVR32_LINUX_MACHDEP_H

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_siginfo.h>

/*
 * From Linux's include/asm-avr32/sigcontext.h
 */
struct linux_sigcontext {
	unsigned long  lsc_oldmask;
	unsigned long  lsc_regs[17];
}; 

/*
 * From Linux's include/asm-avr32/elf.h
 */
typedef unsigned long linux_elf_greg_t;
#define LINUX_ELF_NGREG (sizeof(struct linux_pt_regs) / sizeof (linux_elf_greg_t))
typedef linux_elf_greg_t linux_elf_gregset_t[LINUX_ELF_NGREG];

/*
 * From Linux's include/asm-avr32/ucontext.h
 */
struct linux_ucontext {
	unsigned long luc_flags;
	struct linux_ucontext *luc_link;
	linux_stack_t luc_stack;
	struct linux_sigcontext luc_mcontext;
	linux_sigset_t luc_sigmask;
};

/*
 * From Linux's arch/avr32/kernel/signal.c
 */
struct linux_rt_sigframe {
	struct linux_siginfo lrs_si;
	struct linux_ucontext lrs_uc;
	unsigned long lrs_code;
};

/*
 * From Linux's include/linux/utsname.h
 */
#define LINUX___NEW_UTS_LEN	64

/*
 * Major device numbers of VT device on both Linux and NetBSD. Used in
 * ugly patch to fake device numbers.
 *
 * LINUX_CONS_MAJOR is from Linux's include/linux/major.h
 */
#define LINUX_CONS_MAJOR 4
#define NETBSD_WSCONS_MAJOR 47 /* XXX */

/*
 * Linux ioctl calls for the keyboard.
 *
 * From Linux's include/linux/kd.h
 */
#define LINUX_KDGKBMODE	0x4b44
#define LINUX_KDSKBMODE	0x4b45
#define LINUX_KDMKTONE	0x4b30
#define LINUX_KDSETMODE	0x4b3a
#define LINUX_KDENABIO	0x4b36
#define LINUX_KDDISABIO	0x4b37
#define LINUX_KDGETLED	0x4b31
#define LINUX_KDSETLED	0x4b32
#define LINUX_KDGKBTYPE	0x4B33
#define LINUX_KDGKBENT	0x4B46 
/*
 * Mode for KDSKBMODE which we don't have (we just use plain mode for this)
 *
 * From Linux's include/linux/kd.h
 */
#define LINUX_K_MEDIUMRAW 2

/*
 * VT ioctl calls in Linux (the ones that the pcvt emulation in
 * wscons can handle)
 *
 * From Linux's include/linux/vt.h
 */
#define LINUX_VT_OPENQRY	0x5600
#define LINUX_VT_GETMODE	0x5601
#define LINUX_VT_SETMODE	0x5602
#define LINUX_VT_GETSTATE	0x5603
#define LINUX_VT_RELDISP	0x5605
#define LINUX_VT_ACTIVATE	0x5606
#define LINUX_VT_WAITACTIVE 	0x5607
#define LINUX_VT_DISALLOCATE	0x5608

/*
 * This range used by VMWare (XXX)
 *
 * From Linux's include/linux/vt.h
 * XXX not needed for avr32
 */
#define LINUX_VMWARE_NONE 200
#define LINUX_VMWARE_LAST 237

/*
 * Range of ioctls to just pass on, so that LKMs (like VMWare) can
 * handle them.
 *
 * From Linux's include/linux/vt.h
 */
#define LINUX_IOCTL_MIN_PASS LINUX_VMWARE_NONE
#define LINUX_IOCTL_MAX_PASS (LINUX_VMWARE_LAST+8)

#ifdef _KERNEL
__BEGIN_DECLS /* XXX from NetBSD/i386. Not arch dependent? */
void linux_syscall_intern(struct proc *);
__END_DECLS
#endif /* !_KERNEL */

#endif /* _AVR32_LINUX_MACHDEP_H */
