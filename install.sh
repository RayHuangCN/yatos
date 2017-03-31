#! /bin/bash

make && \
    dd if=arch-target/boot.bin of=bochs_workspace/c.img bs=512 seek=9 count=10 conv=notrunc && \
    dd if=yatos of=bochs_workspace/c.img bs=512 seek=32 count=128 conv=notrunc
