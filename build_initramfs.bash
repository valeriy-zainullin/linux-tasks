#!/bin/bash

set -e

if [ "$(id -u)" -ne 0 ]; then
	echo "Must be root do perform this."
	exit 1
fi

cd root
find . -print0 | cpio -o --null --format=newc | gzip > ../boot/initramfs.gz
chmod 777 ../boot/initramfs.gz
