objs-bin=cat echo link ls mkdir rmdir sleep sync touch unlink
objs-sbin=init shell

all:$(objs-bin) $(objs-sbin)

$(objs-bin):
	gcc-yatos $@.c -o ../bin/$@

$(objs-sbin):
	gcc-yatos $@.c -o ../sbin/$@
clean:
	rm -f ../bin/*
	rm -f ../sbin/*
