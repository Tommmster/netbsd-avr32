AVR32_CFLAGS  = -mrelax -mno-pic -march=ap -nostdlib -nostdinc -ffreestanding -D_KERNEL 
AVR32_LDFLAGS = -mrelax -mno-pic -march=ap -nostdlib -nostdinc -Xlinker --allow-multiple-definition -Wl,-T,sys/arch/avr32/conf/kern.ldscript
AVR32_CC      = avr32-gcc
AVR32_OBJCOPY = avr32-objcopy
AVR32_MKIMAGE = mkimage

SOURCES=$(shell find . -name \*.c)
OBJECTS=$(shell find . -name \*.o)

all: 
	echo $(ARCH_CONF)
	echo $(OBJECTS)	

link: $(OBJECTS)
	$(AVR32_CC) $(AVR32_LDFLAGS) $(OBJECTS) -o netbsd-avr32.elf

%.o: %.c
	$(AVR32_CC) $(AVR32_CFLAGS) -c $<

clean:
	rm -rf $(OBJECTS)
	rm -rf netbsd-avr32.bin
	rm -rf netbsd-avr32.bin.gz
	rm -rf netbsd-avr32.uImage
	rm -rf netbsd-avr32.elf

install: netbsd-avr32.uImage
	cp -f $< /srv/tftp/uImage

netbsd-avr32.uImage: netbsd-avr32.bin.gz
	$(AVR32_MKIMAGE) -A avr32 -O linux -T kernel -C gzip -a 0x10000000 -e 0x90000000 -n 'NetBSD-avr32' -d $< $@

netbsd-avr32.bin.gz: netbsd-avr32.bin
	gzip< $< > $@ 

netbsd-avr32.bin: 
	$(AVR32_OBJCOPY) -O binary netbsd-avr32.elf $@
