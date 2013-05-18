AVR32_CC = avr32-gcc 
SRC_ROOT = ../../../..

AVR32_CFLAGS = -mrelax -mno-pic -march=ap -nostdlib -nostdinc -ffreestanding -D_KERNEL -D__NetBSD__ -DVMSWAP -Ulinux -U__linux__ -I. -I$(SRC_ROOT) -I$(SRC_ROOT)/common/lib/libc -I$(SRC_ROOT)/sys -I$(SRC_ROOT)/sys/opt -I$(SRC_ROOT)/sys/inc -I$(SRC_ROOT)/common/include 

SOURCES = $(shell find . -name \*.c -or -name \*.S)
OBJECTS = $(patsubst %.c, %.o, $(SOURCES)) $(patsubst %.S, %.o, $(SOURCES))

all: $(OBJECTS)
	:

clean:
	rm -rf *.o

%.o: %.c
	$(AVR32_CC) $(AVR32_CFLAGS) -c $<

%.o: %.S
	$(AVR32_CC) $(AVR32_CFLAGS) -c $<
