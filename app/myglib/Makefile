obj-y = unistd.o fcntl.o sys_call.o stdlib.o ctype.o string.o vsprintf.o printf.o malloc.o getopt.o dirent.o errno.o signal.o sys_call_c.o
lib-dir=lib
lib-target = $(lib-dir)/libmyglib.o
target-dir=/opt/yatos/yatos-glib/

%.o : %.c
	gcc -m32 -static -nostdlib -fno-builtin -fno-stack-protector -c -o $@ $<

%.o : %.asm
	nasm -f elf -o $@ $<


all:$(obj-y)
	@test -d $(lib-dir) || mkdir $(lib-dir)
	ld -r -melf_i386 -o  $(lib-target)  $(obj-y)
clean:
	rm *.o  $(lib-target)
	rm -r -f lib

install:
	@test -d $(target-dir) || mkdir -p $(target-dir)
	cp -r lib /opt/yatos/yatos-glib/
	cp gcc-yatos /usr/bin/
uninstall:
	rm /opt/yatos -f -r
	rm /usr/bin/gcc-yatos
