import os
import sys 

AVR32_CFLAGS=" -relax -mno-pic -march=ap -nostdlib -freestanding -D_KERNEL $(INCLUDES)\n"

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
	rules.append("\t:\n\r")

	rules.append("%.o:%.c\n\r")
	rules.append("\t$(AVR32_CC) $(AVR32_CFLAGS -c $<\n\r")
	
	rules.append("clean:\n\r");
	rules.append("\trm -rf *.o")

	return " ".join(rules)
	

if __name__=='__main__':
	
	if (os.path.exists("Makefile")):
		print("Makefile exists")
		sys.exit()

	print('Creating Makefile')

	f = open('Makefile','w')
	
	f.write("SYS_DIR=" + SYS_DIR)
	f.write("MACHINE_INCLUDE=" + MACHINE_INCLUDE)
	f.write("OPT_DIR=" + OPT_DIR)

	f.write("INCLUDES="+INCLUDES+'\n')

	f.write("AVR32_CC=")
	f.write(AVR32_CC +'\n')

	f.write("AVR32_CFLAGS=")
	f.write(AVR32_CFLAGS +'\n')
		
	f.write(sources()+'\n')	
	
	f.write(objects()+'\n')	
	f.write(rules()+'\n')	





