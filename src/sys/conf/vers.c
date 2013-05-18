/*
 * Automatically generated file from ../../../../conf/newvers.sh
 * Do not edit.
 */
#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>

const char ostype[] = "NetBSD";
const char osrelease[] = "5.1.0_PATCH";
const char sccs[] = "@(#)NetBSD 5.1.0_PATCH (MYKERNEL) #0: Thu Jan 12 20:05:02 ARST 2012\n\troot@netbsd-5-1:/tomas/new/src/sys/arch/i386/compile/MYKERNEL\n";
const char version[] = "NetBSD 5.1.0_PATCH (MYKERNEL) #0: Thu Jan 12 20:05:02 ARST 2012\n\troot@netbsd-5-1:/tomas/new/src/sys/arch/i386/compile/MYKERNEL\n";
const char kernel_ident[] = "MYKERNEL";
const char copyright[] =
"Copyright (c) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005,\n""    2006, 2007, 2008, 2009, 2010\n""    The NetBSD Foundation, Inc.  All rights reserved.\n""Copyright (c) 1982, 1986, 1989, 1991, 1993\n""    The Regents of the University of California.  All rights reserved.\n"
"\n";

/*
 * NetBSD identity note.
 */
#ifdef __arm__
#define _SHT_NOTE	%note
#else
#define _SHT_NOTE	@note
#endif

#define	_S(TAG)	__STRING(TAG)
__asm(
	".section\t\".note.netbsd.ident\", \"\"," _S(_SHT_NOTE) "\n"
	"\t.p2align\t2\n"
	"\t.long\t" _S(ELF_NOTE_NETBSD_NAMESZ) "\n"
	"\t.long\t" _S(ELF_NOTE_NETBSD_DESCSZ) "\n"
	"\t.long\t" _S(ELF_NOTE_TYPE_NETBSD_TAG) "\n"
	"\t.ascii\t" _S(ELF_NOTE_NETBSD_NAME) "\n"
	"\t.long\t" _S(__NetBSD_Version__) "\n"
	"\t.p2align\t2\n"
);

