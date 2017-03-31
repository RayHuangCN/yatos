CROSS_COMPILE =
LDS_FILE = arch/x86/yatos.lds

LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
CPP		= $(CC) -E
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm

STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump

export LD CC CPP AR NM
export STRIP OBJCOPY OBJDUMP

CFLAGS := -Wall -O2 -g -m32 -fno-builtin -fno-stack-protector
CFLAGS += -I$(shell pwd)/include

LDFLAGS := -nostdlib -nostartfiles -static  -melf_i386
export CFLAGS LDFLAGS

TOPDIR := $(shell pwd)
export TOPDIR

TARGET := yatos


obj-y += kernel/
obj-y += arch-target/


all : 
	make -C ./ -f $(TOPDIR)/Makefile.build
	${LD} -T$(LDS_FILE)  $(LDFLAGS)  $(LIBPATH) -o $(TARGET).elf  built-in.o
	${OBJCOPY} -O binary -S $(TARGET).elf  $(TARGET)
	${OBJDUMP} -D $(TARGET).elf > $(TARGET).dis


clean:
	rm -f $(shell find -name "*.o")
	rm -f $(shell find -name "*.d")
	rm -f $(shell find -name "*.bin")
	rm -f $(shell find -name "*~")
	rm -f $(TARGET) $(TARGET).dis $(TARGET).elf
