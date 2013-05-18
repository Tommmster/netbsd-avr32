SRC_DIR=../../../../..
SYS_DIR=$(SRC_DIR)/sys

MACHINE_INCLUDE=$(SYS_DIR)/inc/
OPT_DIR=$(SYS_DIR)/opt
COMPAT_COMMON_INCLUDE=.

INCLUDES=-I$(MACHINE_INCLUDE) -I$(OPT_DIR)  -I$(SYS_DIR)   -I$(SRC_DIR)/common/include  -I$(SYS_DIR)/dev -I$(COMPAT_COMMON_INCLUDE)

AVR32_CC=avr32-gcc

AVR32_CFLAGS= -mrelax -mno-pic -march=ap -nostdinc -nostdlib -ffreestanding -ffixed-r1 -D_KERNEL -D__NetBSD__ $(INCLUDES)

SOURCES=$(shell find . -maxdepth 1 -name \*.c -and -not -name sys_syscall.c)
#SOURCES=linux_exec_elf32.c

OBJECTS=$(patsubst %.c, %.o, $(SOURCES))

all: $(OBJECTS)
	:
	
%.o:%.c
	$(AVR32_CC) $(AVR32_CFLAGS) -c $<
clean:
	rm -rf *.o
