avr32-linux-gcc -mrelax -mno-pic -march=ap -Xlinker --allow-multiple-definition -nostdlib -nostdinc -ffreestanding -Wl,-T,./sys/arch/avr32/conf/kern.ldscript -o netbsd-avr32.elf ./sys/kern/init_sysctl.o `find ./sys ./common -name *.o -and -not -name init_sysctl.o`

avr32-linux-objcopy -O binary netbsd-avr32.elf netbsd-avr32.bin

gzip < netbsd-avr32.bin > netbsd-avr32.bin.gz

mkimage -A avr32 -O linux -T kernel -C gzip -a 0x10000000 -e 0x90000000 -n 'NetBSD' -d netbsd-avr32.bin.gz netbsd-avr32.uImage
