#! /bin/sh

find . -type f -name \*.o -exec rm -vf {} \;
rm -f netbsd-avr32.bin
rm -f netbsd-avr32.bin.gz
rm -f netbsd-avr32.uImage
rm -f *.elf
