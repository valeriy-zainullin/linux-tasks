#!/bin/bash

set -xe

cd linux-6.11
# конфиг уже создан, например, с помощью make menuconfig.
# не забывайте, что существует make help. Еще есть поиск в интернете и kernel.org.
make -j$(nproc)

# INSTALL_PATH=../boot make install
# Как решить проблему "Cannot find LILO" во время make install.
#  Просто не использовать make install, нам достаточно скопировать
#  образ ядра vmlinuz и мы готовы.
#  LILO является маленьким загрузчиком. Альтернативой ему будет GRUB.
#  Но нам не нужен загрузчик, мы пользуемся возможностью QEMU запускать
#  ядро линукса без загрузчика для EFI или BIOS.
# Можем скопировать bzImage вручную.
#   https://unix.stackexchange.com/questions/5518/what-is-the-difference-between-the-following-kernel-makefile-terms-vmlinux-vml
# user@pc linux-course]$ find linux-6.11 -name bzImage
# linux-6.11/arch/x86/boot/bzImage
# linux-6.11/arch/x86_64/boot/bzImage
# cp arch/x86_64/boot/bzImage ../boot/vmlinuz
# А можно просто проигнорировать сообщение, это ок.

sudo chown -R $USER:$USER ../root
INSTALL_PATH=../boot INSTALL_MOD_PATH=../root/lib make install modules_install
sudo chown -R root:root ../root

{ echo "Ignore that \"LILO\" message, if you have any, we don't need a bootloader. Files are copied at this point, make install configures LILO for convenience, because LILO uses block number to locate the kernel. https://serverfault.com/a/383704"; } 2>/dev/null
