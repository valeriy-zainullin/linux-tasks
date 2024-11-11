#!/bin/bash

set -xe

script_dir=$(dirname "$BASH_SOURCE[0]")
cd "$script_dir"

if [ ! -d linux-kernel ]; then
	wget https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.11.7.tar.xz
	tar -x -f linux-6.11.7.tar.xz
	rm linux-.tar.xz
	mv linux-6.11.7 linux-kernel
fi

if [ ! -d busybox ]; then
	wget https://busybox.net/downloads/busybox-1.37.0.tar.bz2
	tar -x -f busybox-*.tar.bz2
	rm busybox-*.tar.bz2
	mv busybox-* busybox
	cp busybox-config busybox/configs/homework_config
fi
