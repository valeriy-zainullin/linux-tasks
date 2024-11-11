#!/bin/bash

run_gui=0
show_klog=0

for arg in "$@"; do
	if [ "$arg" = "gui" ]; then
		run_gui=1
	elif [ "$arg" = "klog" ]; then
		show_klog=1
	fi
done

set -xe
qemu-system-x86_64 -kernel boot/vmlinuz-* -initrd boot/initramfs.gz -serial stdio $(if [ "$run_gui" -eq 0 ]; then echo -nographic; fi) -enable-kvm -cpu host -append "console=ttyS0 run_gui=$run_gui show_klog=$show_klog"
