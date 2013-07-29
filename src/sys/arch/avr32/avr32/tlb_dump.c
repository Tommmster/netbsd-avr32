#include <sys/systm.h>

#include <avr32/cpuregs.h>
#include <avr32/locore.h>

void
tlb_miss()
{
	unsigned int tlbear = AVR32_MFSR(SR_TLBEAR);
	panic("tlb miss %x \n", tlbear);
}

void 
avr32_show_tlb_entry(unsigned int index)
{
	unsigned int tlbehi, tlbehi_save, tlbelo, mmucr, mmucr_save;
	unsigned long flags;

	mmucr_save = AVR32_MFSR(SR_MMUCR);
	tlbehi_save = AVR32_MFSR(SR_TLBEHI);
	mmucr = mmucr_save & 0x13;
	mmucr |= index << 14;
	AVR32_MTSR(SR_MMUCR, mmucr);

	asm volatile("tlbr" : : : "memory");

	tlbehi = AVR32_MFSR(SR_TLBEHI);
	tlbelo = AVR32_MFSR(SR_TLBELO);

	printf("%2u: %c %c %02x   %05x %05x %o  %o  %c %c %c %c\n",
	       index,
	       (tlbehi & 0x200)?'1':'0',
	       (tlbelo & 0x100)?'1':'0',
	       (tlbehi & 0xff),
	       (tlbehi >> 12), (tlbelo >> 12),
	       (tlbelo >> 4) & 7, (tlbelo >> 2) & 3,
	       (tlbelo & 0x200)?'1':'0',
	       (tlbelo & 0x080)?'1':'0',
	       (tlbelo & 0x001)?'1':'0',
	       (tlbelo & 0x002)?'1':'0');

	AVR32_MTSR(SR_MMUCR, mmucr_save);
	AVR32_MTSR(SR_TLBEHI, tlbehi_save);
}

void 
avr32_tlb_dump(void)
{
	unsigned int i;

	printf("ID  V G ASID VPN   PFN   AP SZ C B W D\n");
	for (i = 0; i < 32; i++)
		avr32_show_tlb_entry(i);
}
