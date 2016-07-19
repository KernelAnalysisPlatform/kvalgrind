#!/bin/bash

# usage: memcheck.sh [driver] [VM image file]

./qemu/x86_64-softmmu/qemu-system-x86_64 \
    -m 1024 \
    -drive file=$2 \
    -chardev socket,path=/tmp/qga.sock,server,nowait,id=qga0 \
    -device virtio-serial \
    -device virtserialport,chardev=qga0,name=org.qemu.guest_agent.0 \
    -panda callstack_instr \
    -panda in_vmi_linux \
    -redir tcp:10022::22 \
    -monitor stdio
