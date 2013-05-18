SRC_DIR  = ../..
SYS_DIR  = $(SRC_DIR)/sys
OPT_DIR  = $(SYS_DIR)/opt
INCLUDES = -I$(OPT_DIR) -I$(SYS_DIR) -I$(SYS_DIR)/inc -I$(SRC_DIR)/common/include -I$(SYS_DIR)/dev

AVR32_CC     = avr32-gcc
AVR32_CFLAGS = -mrelax -mno-pic -march=ap -nostdinc -nostdlib -ffreestanding -D_KERNEL -D__NetBSD__ -DVMSWAP -Ulinux -U__linux__ $(INCLUDES)

SOURCES = $(shell find . -maxdepth 1 -name \*.c -and -not -name sys_syscall.c)
OBJECTS = $(patsubst %.c, %.o, $(SOURCES))

all: $(OBJECTS)
	:
	
%.o: %.c
	$(AVR32_CC) $(AVR32_CFLAGS) -c $<

clean:
	rm -rf *.o
