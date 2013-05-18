import os
import sys 

AVR32_CFLAGS=" -mrelax -mno-pic -march=ap -nostdlib -ffreestanding -D_KERNEL $(INCLUDES)\n"

AVR32_CC = "avr32-gcc\n"
AVR32_OBJCOPY = "avr32-objcopy\n"
AVR32_MKIMAGE = "mkimage\n"

SYS_DIR="../sys\n"
MACHINE_INCLUDE="../inc/\n"
OPT_DIR="../opt\n"

INCLUDES="-I$(MACHINE_INCLUDE) -I$(OPT_DIR) -I.. -I$(SYS_DIR)   -I../common/include  -I../dev\n"

def list_c_files():
	files=os.listdir(".")
	return [f for f in files if os.path.splitext(f)[1]=='.c']

def sources(sources=list_c_files()):
	ccfiles = " ".join(sources)
	
	srcs = "SOURCES="
	srcs += ccfiles
	
	return srcs

def objects(sources=list_c_files()):
	objects =[]
	for src in sources[:]:
		objects.append(os.path.splitext(src)[0] + '.o')

	objs = "OBJECTS="	
	objs += " ".join(objects)
	return objs

def rules():
	rules=[]
	
	rules.append("all: $(OBJECTS)\n")
	rules.append("\t:\n")

	rules.append("%.o:%.c\n")
	rules.append("\t$(AVR32_CC) $(AVR32_CFLAGS -c $<\n")
	
	rules.append("clean:\n");
	rules.append("\trm -rf *.o")

	return " ".join(rules)
	

if __name__=='__main__':
	
	if (os.path.exists("Makefile")):
		print("Makefile exists")
		sys.exit()

	print('Creating Makefile')

	makefile = open('Makefile','w')
	
	makefile.write("SYS_DIR=" + SYS_DIR)
	makefile.write("MACHINE_INCLUDE=" + MACHINE_INCLUDE)
	makefile.write("OPT_DIR=" + OPT_DIR)

	makefile.write("INCLUDES="+INCLUDES+'\n')

	makefile.write("AVR32_CC=")
	makefile.write(AVR32_CC +'\n')
	makefile.write("AVR32_CFLAGS=")
	makefile.write(AVR32_CFLAGS +'\n')
	makefile.write(sources()+'\n')	
	makefile.write(objects()+'\n')	
	makefile.write(rules()+'\n')	
	




