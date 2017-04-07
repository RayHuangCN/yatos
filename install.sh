#! /bin/bash

make && \
    dd if=yatos of=../bochs_workspace/c.img bs=512 seek=32 count=128 conv=notrunc
