#!/bin/bash

set -xe

# конфиг уже создан, например, с помощью make menuconfig. Если нет, создадим.
# не забывайте, что существует make help.
if [ ! -f .config ]; then
	make homework_defconfig
fi
make -C busybox -j$(nproc)

mkdir -p root/bin
cp busybox/busybox root/bin
# Switching into chroot, so that we're in the same directory
#   structure as in the booted system. Symbolic links will have
#   the path busybox called with. So we should call the way
#   it is accessible in the booted system (argv[0] should be
#   the accessible path, preferably absolute, so that symbolic
#   links will have this absolute path).
sudo chroot root /bin/busybox --install -s /bin

mkdir -p boot
cd root
mkdir -p tmp dev proc sys
find . -print0 | cpio -o --null --format=newc | gzip > ../boot/initramfs.gz
chmod 777 ../boot/initramfs.gz
