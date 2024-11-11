#!/bin/bash

set -xe

qemu-system-x86_64 -kernel boot/vmlinuz-* -initrd boot/initramfs.gz -nographic -enable-kvm -cpu host -append "console=ttyS0"
