#! /bin/bash

make && \
    dd if=yatos of=../bochs_workspace/c.img bs=512 seek=2049 count=128 conv=notrunc
