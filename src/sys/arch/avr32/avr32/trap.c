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
#include <sys/lwp.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/savar.h>

#include <uvm/uvm_extern.h>

#include <avr32/proc.h>
#include <avr32/cpu.h>
#include <avr32/psl.h>
#include <avr32/pte.h>
#include <avr32/reg.h>
#include <avr32/trap.h>
#include <avr32/locore.h>
#include <avr32/userret.h>

#define TRAPTYPE(ecr)	(ecr)
#define KERNLAND(x)	((uint32_t)(x) >= VM_MIN_KERNEL_ADDRESS)
#define LENGTH(x)	(sizeof((x)) / sizeof((x)[0]))

const char *trap_type[] = {
	"unrecoverable exception",      /* evba + 0x00  */
	"TLB multiple hit",             /* evba + 0x04  */
	"bus error data fetch",         /* evba + 0x08  */
	"bus error instruction fetch",  /* evba + 0x0C  */
	"NMI",                          /* evba + 0x10  */
	"instruction address",          /* evba + 0x14  */
	"ITLB protection",              /* evba + 0x18  */
	"breakpoint",                   /* evba + 0x1C  */
	"illegal opcode",               /* evba + 0x20  */
	"unimplemented instruction",    /* evba + 0x24  */
	"privilege violation",          /* evba + 0x28  */
	"floating point",               /* evba + 0x2C  */
	"coprocessor absent",           /* evba + 0x30  */
	"data read address error",      /* evba + 0x34  */
	"data write address error",     /* evba + 0x38  */
	"DTLB read protection",         /* evba + 0x3C  */
	"DTLB write protection",        /* evba + 0x40  */
	"DTLB modified",                /* evba + 0x44  */
	"reserved 18",                  /* evba + 0x48  */
	"reserved 19",                  /* evba + 0x4c  */
	"ITLB miss",                    /* evba + 0x50  */
	"reserved 21",                  /* evba + 0x54  */
	"reserved 22",                  /* evba + 0x58  */
	"reserved 23",                  /* evba + 0x5c  */
	"DTLB read miss",               /* evba + 0x60  */
	"reserved 25",                  /* evba + 0x64  */
	"reserved 26",                  /* evba + 0x68  */
	"reserved 27",                  /* evba + 0x6c  */
	"DTLB write miss",              /* evba + 0x70  */
	"reserved 29",                  /* evba + 0x74  */
	"reserved 30",                  /* evba + 0x78  */
	"reserved 31",                  /* evba + 0x7c  */
	"reserved 32",                  /* evba + 0x80  */
	"reserved 33",                  /* evba + 0x84  */
	"reserved 34",                  /* evba + 0x88  */
	"reserved 35",                  /* evba + 0x8c  */
	"reserved 36",                  /* evba + 0x90  */
	"reserved 37",                  /* evba + 0x94  */
	"reserved 38",                  /* evba + 0x98  */
	"reserved 39",                  /* evba + 0x9c  */
	"supervisor call",              /* evba + 0x100 */
};

void
child_return(void *arg)
{
	panic("child_return: notyet");	
}

/*
 * Trap is called from locore to handle most types of processor traps.
 */
void
trap(unsigned int status, unsigned int cause, vaddr_t vaddr, vaddr_t opc,
	struct trapframe *frame) 
{
	int type;
	struct lwp *l = curlwp;
	struct proc *p = curproc;
	vm_prot_t ftype;
	ksiginfo_t ksi;
	struct frame *fp;
	extern void fswintrberr(void);
	KSI_INIT_TRAP(&ksi);

	uvmexp.traps++;

	if ((type = TRAPTYPE(cause)) >= LENGTH(trap_type))
		panic("trap: unknown trap type %d", type);

	if (USERMODE(status)) {
		type |= T_USER;
		LWP_CACHE_CREDS(l, p);
	}

	/* Enable interrupts just at it was before the trap. */
	_splset(status & AVR32_STATUS_IMx);

	switch (type) {
	default:
	dopanic:
		(void)splhigh();
		printf("trap: %s in %s mode\n",
			trap_type[TRAPTYPE(cause)],
			USERMODE(status) ? "user" : "kernel");
		printf("status=0x%x, cause=0x%x, epc=%#lx, vaddr=%#lx\n",
			status, cause, opc, vaddr);
		if (curlwp != NULL) {
			fp = (struct frame *)l->l_md.md_regs;
			printf("pid=%d cmd=%s usp=0x%x ",
			    p->p_pid, p->p_comm, (int)fp->f_regs[_R_SP]);
		} else
			printf("curlwp == NULL ");
		printf("ksp=%p\n", &status);
#if defined(DDB)
		kdb_trap(type, (mips_reg_t *) frame);
		/* XXX force halt XXX */
#elif defined(KGDB)
		{
			struct frame *f = (struct frame *)&ddb_regs;
			extern mips_reg_t kgdb_cause, kgdb_vaddr;
			kgdb_cause = cause;
			kgdb_vaddr = vaddr;

			/*
			 * init global ddb_regs, used in db_interface.c routines
			 * shared between ddb and gdb. Send ddb_regs to gdb so
			 * that db_machdep.h macros will work with it, and
			 * allow gdb to alter the PC.
			 */
			db_set_ddb_regs(type, (mips_reg_t *) frame);
			PC_BREAK_ADVANCE(f);
			if (kgdb_trap(type, &ddb_regs)) {
				((mips_reg_t *)frame)[21] = f->f_regs[_R_PC];
				return;
			}
		}
#else
		panic("trap: dopanic: notyet");
#endif
		/*NOTREACHED*/
	case T_TLB_MOD:
		panic("trap: T_TLB_MOD: notyet");
#if notyet
		if (KERNLAND(vaddr)) {
			pt_entry_t *pte;
			unsigned entry;
			paddr_t pa;

			pte = kvtopte(vaddr);
			entry = pte->pt_entry;
			if (!avr32_pte_v(entry) /*|| (entry & mips_pg_m_bit())*/) {
				panic("ktlbmod: invalid pte");
			}
			if (entry & avr32_pte_ropage_bit()) {
				/* write to read only page in the kernel */
				ftype = VM_PROT_WRITE;
				goto kernelfault;
			}
			entry |= mips_pg_m_bit();	/* XXXAVR32 Do it on tlbarlo/ tlbarhi? */
			pte->pt_entry = entry;
			vaddr &= ~PGOFSET;
			MachTLBUpdate(vaddr, entry);
			pa = avr32_tlbpfn_to_paddr(entry);
			if (!IS_VM_PHYSADDR(pa)) {
				printf("ktlbmod: va %#lx pa %#llx\n",
				    vaddr, (long long)pa);
				panic("ktlbmod: unmanaged page");
			}
			pmap_set_modified(pa);
			return; /* KERN */
		}
		/*FALLTHROUGH*/
#endif
	case T_TLB_MOD+T_USER: 
		panic("trap: T_TLB_MOD+T_USER: notyet");
#if notyet
	    {
		pt_entry_t *pte;
		unsigned entry;
		paddr_t pa;
		pmap_t pmap;

		pmap  = p->p_vmspace->vm_map.pmap;
		if (!(pte = pmap_segmap(pmap, vaddr)))
			panic("utlbmod: invalid segmap");
		pte += (vaddr >> PGSHIFT) & (NPTEPG - 1);

		entry = pte->pt_entry;
		if (!avr32_pte_v(entry))
			panic("utlbmod: invalid pte");

		if (entry & avr32_pte_ropage_bit()) {
			/* write to read only page */
			ftype = VM_PROT_WRITE;
			goto pagefault;
		}
		/* entry |= mips_pg_m_bit();  XXXAVR32 Do it on tlbarlo/ tlbarhi? */
		pte->pt_entry = entry;
		vaddr = (vaddr & ~PGOFSET) |
			(pmap->pm_asid << AVR32_TLB_PID_SHIFT);
		MachTLBUpdate(vaddr, entry);
		pa = avr32_tlbpfn_to_paddr(entry);
		if (!IS_VM_PHYSADDR(pa)) {
			printf("utlbmod: va %#lx pa %#llx\n",
			    vaddr, (long long)pa);
			panic("utlbmod: unmanaged page");
		}
		pmap_set_modified(pa);
		if (type & T_USER)
			userret(l);
		return; /* GEN */
	    }
#endif
	case T_TLB_LD_MISS:
		panic("trap: T_TLB_LD_MISS: notyet");
	case T_TLB_ST_MISS:
		ftype = (type == T_TLB_LD_MISS) ? VM_PROT_READ : VM_PROT_WRITE;
		if (KERNLAND(vaddr))
			goto kernelfault;
		panic("trap: T_TLB_ST_MISS: notyet");
#if notyet
		/*
		 * It is an error for the kernel to access user space except
		 * through the copyin/copyout routines.
		 */
		if (l == NULL  || l->l_addr->u_pcb.pcb_onfault == NULL)
			goto dopanic;
		/* check for fuswintr() or suswintr() getting a page fault */
		if (l->l_addr->u_pcb.pcb_onfault == (void *)fswintrberr) {
			frame->tf_regs[TF_EPC] = (int)fswintrberr;
			return; /* KERN */
		}
		goto pagefault;
#endif
	case T_TLB_LD_MISS+T_USER:
		panic("trap: T_TLB_LD_MISS+T_USER: notyet");
#if notyet
		ftype = VM_PROT_READ;
		goto pagefault;
#endif
	case T_TLB_ST_MISS+T_USER:
		panic("trap: T_TLB_ST_MISS+T_USER: notyet");
#if notyet
		ftype = VM_PROT_WRITE;
#endif
	pagefault: ;
	    {
		vaddr_t va;
		struct vmspace *vm;
		struct vm_map *map;
		int rv;

		vm = p->p_vmspace;
		map = &vm->vm_map;
		va = trunc_page(vaddr);

		if ((l->l_flag & LW_SA) && (~l->l_pflag & LP_SA_NOBLOCK)) {
			l->l_savp->savp_faultaddr = (vaddr_t)vaddr;
			l->l_pflag |= LP_SA_PAGEFAULT;
		}

		if (p->p_emul->e_fault)
			rv = (*p->p_emul->e_fault)(p, va, ftype);
		else
			rv = uvm_fault(map, va, ftype);
				
#ifdef VMFAULT_TRACE
		printf(
	    "uvm_fault(%p (pmap %p), %lx (0x%x), %d) -> %d at pc %p\n",
		    map, vm->vm_map.pmap, va, vaddr, ftype, rv, (void*)opc);
#endif
		/*
		 * If this was a stack access we keep track of the maximum
		 * accessed stack size.  Also, if vm_fault gets a protection
		 * failure it is due to accessing the stack region outside
		 * the current limit and we need to reflect that as an access
		 * error.
		 */
		if ((void *)va >= vm->vm_maxsaddr) {
			if (rv == 0){
				uvm_grow(p, va);
			}
			else if (rv == EACCES)
				rv = EFAULT;
		}
		l->l_pflag &= ~LP_SA_PAGEFAULT;
		if (rv == 0) {
			if (type & T_USER) {
				userret(l);
			}
			return; /* GEN */
		}
		if ((type & T_USER) == 0)
			goto copyfault;
		if (rv == ENOMEM) {
			printf("UVM: pid %d (%s), uid %d killed: out of swap\n",
			       p->p_pid, p->p_comm,
			       l->l_cred ?
			       kauth_cred_geteuid(l->l_cred) : (uid_t) -1);
			ksi.ksi_signo = SIGKILL;
			ksi.ksi_code = 0;
		} else {
			if (rv == EACCES) {
				ksi.ksi_signo = SIGBUS;
				ksi.ksi_code = BUS_OBJERR;
			} else {
				ksi.ksi_signo = SIGSEGV;
				ksi.ksi_code = SEGV_MAPERR;
			}
		}
		ksi.ksi_trap = type & ~T_USER;
		ksi.ksi_addr = (void *)vaddr;
		break; /* SIGNAL */
	    }
	kernelfault: ;
	    {
		vaddr_t va;
		int rv;

		va = trunc_page(vaddr);
		rv = uvm_fault(kernel_map, va, ftype);
		if (rv == 0)
			return; /* KERN */
		/*FALLTHROUGH*/
	    }
	case T_ADDR_ERR_LD:	/* misaligned access */
	case T_ADDR_ERR_ST:	/* misaligned access */
	case T_BUS_ERR_LD_ST:	/* BERR asserted to CPU */
	copyfault:
		panic("trap: copyfault: notyet");
#if notyet
		if (l == NULL || l->l_addr->u_pcb.pcb_onfault == NULL)
			goto dopanic;
		frame->tf_regs[TF_EPC] = (intptr_t)l->l_addr->u_pcb.pcb_onfault;
		return; /* KERN */
#endif
#if notyet
	case T_ADDR_ERR_LD+T_USER:	/* misaligned or kseg access */
	case T_ADDR_ERR_ST+T_USER:	/* misaligned or kseg access */
	case T_BUS_ERR_IFETCH+T_USER:	/* BERR asserted to CPU */
	case T_BUS_ERR_LD_ST+T_USER:	/* BERR asserted to CPU */
		ksi.ksi_trap = type & ~T_USER;
		ksi.ksi_signo = SIGSEGV; /* XXX */
		ksi.ksi_addr = (void *)vaddr;
		ksi.ksi_code = SEGV_MAPERR; /* XXX */
		break; /* SIGNAL */

	case T_BREAK:
		panic("trap: T_BREAK: notyet");
#if defined(DDB)
		kdb_trap(type, (avr32_reg_t *) frame);
		return;	/* KERN */
#elif defined(KGDB)
		{
			struct frame *f = (struct frame *)&ddb_regs;
			extern avr32_reg_t kgdb_cause, kgdb_vaddr;
			kgdb_cause = cause;
			kgdb_vaddr = vaddr;

			/*
			 * init global ddb_regs, used in db_interface.c routines
			 * shared between ddb and gdb. Send ddb_regs to gdb so
			 * that db_machdep.h macros will work with it, and
			 * allow gdb to alter the PC.
			 */
			db_set_ddb_regs(type, (avr32_reg_t *) frame);
			PC_BREAK_ADVANCE(f);
			if (!kgdb_trap(type, &ddb_regs))
				printf("kgdb: ignored %s\n",
				       trap_type[TRAPTYPE(cause)]);
			else
				((avr32_reg_t *)frame)[21] = f->f_regs[_R_PC];

			return;
		}
#else
		goto dopanic;
#endif
	case T_BREAK+T_USER:
	    {
		vaddr_t va;
		uint32_t instr;
		int rv;

		/* compute address of break instruction */
		va = (DELAYBRANCH(cause)) ? opc + sizeof(int) : opc;

		/* read break instruction */
		instr = fuiword((void *)va);

		if (l->l_md.md_ss_addr != va || instr != MIPS_BREAK_SSTEP) {
			ksi.ksi_trap = type & ~T_USER;
			ksi.ksi_signo = SIGTRAP;
			ksi.ksi_addr = (void *)va;
			ksi.ksi_code = TRAP_TRACE;
			break;
		}
		/*
		 * Restore original instruction and clear BP
		 */
		rv = suiword((void *)va, l->l_md.md_ss_instr);
		if (rv < 0) {
			vaddr_t sa, ea;
			sa = trunc_page(va);
			ea = round_page(va + sizeof(int) - 1);
			rv = uvm_map_protect(&p->p_vmspace->vm_map,
				sa, ea, VM_PROT_ALL, false);
			if (rv == 0) {
				rv = suiword((void *)va, l->l_md.md_ss_instr);
				(void)uvm_map_protect(&p->p_vmspace->vm_map,
				sa, ea, VM_PROT_READ|VM_PROT_EXECUTE, false);
			}
		}
		mips_icache_sync_all();		/* XXXJRT -- necessary? */
		mips_dcache_wbinv_all();	/* XXXJRT -- necessary? */

		if (rv < 0)
			printf("Warning: can't restore instruction at 0x%lx: 0x%x\n",
				l->l_md.md_ss_addr, l->l_md.md_ss_instr);
		l->l_md.md_ss_addr = 0;
		ksi.ksi_trap = type & ~T_USER;
		ksi.ksi_signo = SIGTRAP;
		ksi.ksi_addr = (void *)va;
		ksi.ksi_code = TRAP_BRKPT;
		break; /* SIGNAL */
	    }
	case T_RES_INST+T_USER:
	case T_COP_UNUSABLE+T_USER:
#if !defined(SOFTFLOAT) && !defined(NOFPU)
		if ((cause & MIPS_CR_COP_ERR) == 0x10000000) {
			struct frame *f;

			f = (struct frame *)l->l_md.md_regs;
			savefpregs(fpcurlwp);	  	/* yield FPA */
			loadfpregs(l);          	/* load FPA */
			fpcurlwp = l;
			l->l_md.md_flags |= MDP_FPUSED;
			f->f_regs[_R_SR] |= MIPS_SR_COP_1_BIT;
		} else
#endif
		{
			MachEmulateInst(status, cause, opc, l->l_md.md_regs);
		}
		userret(l);
		return; /* GEN */
	case T_FPE+T_USER:
		panic ("trap: T_FPE+T_USER: notyet");
#if defined(SOFTFLOAT)
		MachEmulateInst(status, cause, opc, l->l_md.md_regs);
#elif !defined(NOFPU)
		MachFPTrap(status, cause, opc, l->l_md.md_regs);
#endif
		userret(l);
		return; /* GEN */
	case T_OVFLOW+T_USER:
	case T_TRAP+T_USER:
		ksi.ksi_trap = type & ~T_USER;
		ksi.ksi_signo = SIGFPE;
		fp = (struct frame *)l->l_md.md_regs;
		ksi.ksi_addr = (void *)fp->f_regs[_R_PC];
		ksi.ksi_code = FPE_FLTOVF; /* XXX */
		break; /* SIGNAL */
#endif
	}
	panic("trap: post-switch: notyet");
#if notyet
	fp = (struct frame *)l->l_md.md_regs;
	fp->f_regs[_R_CAUSE] = cause;
	fp->f_regs[_R_BADVADDR] = vaddr;
	(*p->p_emul->e_trapsignal)(l, &ksi);
	if ((type & T_USER) == 0)
		panic("trapsignal");
	userret(l);
#endif
	return;
}
