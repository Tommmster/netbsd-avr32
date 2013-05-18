SRC_DIR=../..
SYS_DIR=$(SRC_DIR)/sys

MACHINE_INCLUDE=$(SYS_DIR)/inc/
OPT_DIR=$(SYS_DIR)/opt

INCLUDES=-I$(MACHINE_INCLUDE) -I$(OPT_DIR)  -I$(SYS_DIR)   -I$(SRC_DIR)/common/include  -I$(SYS_DIR)/dev

AVR32_CC=avr32-gcc

AVR32_CFLAGS= -mrelax -mno-pic -march=ap -nostdinc -nostdlib -ffreestanding -D_KERNEL $(INCLUDES)

SOURCES=$(shell find . -name cons.c -or -name clock_subr.c )

OBJECTS=$(patsubst %.c, %.o, $(SOURCES))

all: $(OBJECTS)
	:
	
%.o:%.c
	$(AVR32_CC) $(AVR32_CFLAGS) -c $<
clean:
	rm -rf *.o
