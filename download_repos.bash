#!/bin/bash

set -xe

script_dir=$(dirname "${BASH_SOURCE[0]}")
cd "$script_dir"

if [ ! -d linux-kernel ]; then
	wget https://github.com/valeriy-zainullin/linuxsrc-for-homework/archive/refs/heads/master.zip
	unzip -q master.zip
	mv linuxsrc-for-homework* linux-kernel

	rm master.zip
fi

if [ ! -d busybox ]; then
	wget https://github.com/valeriy-zainullin/busybox-for-homework/archive/refs/heads/1_36_stable.zip
	unzip -q 1_36_stable.zip
	mv busybox-for-homework* busybox

	rm 1_36_stable.zip
	mv busybox-config busybox/configs/homework_defconfig
fi
