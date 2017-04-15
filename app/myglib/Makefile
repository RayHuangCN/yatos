obj-y = start.o exit.o
lib-dir=lib
include-dir=include
lib-target = $(lib-dir)/libglib.a
target-dir=/opt/yatos/yatos-glib/

%.o : %.c
	gcc -m32 -nostdlib -I./include -c -o $@ $<
all:$(obj-y)
	@test -d $(lib-dir) || mkdir $(lib-dir)
	ar crv $(lib-target)  $(obj-y)
clean:
	rm *.o  $(lib-target)
	rm -r -f lib

install:
	@test -d $(target-dir) || mkdir -p $(target-dir)
	cp -r lib /opt/yatos/yatos-glib/
	cp -r include /opt/yatos/yatos-glib/
	cp gcc-yatos /usr/bin/
uninstall:
	rm /opt/yatos -f -r
	rm /usr/bin/gcc-yatos
