default_boot_dir := ../boot
BOOT_DIR ?= $(shell pwd)/$(default_boot_dir)
# Увидел в файле Kbuild в корне репозитория ядра линукса присваивание ?=.
# Намного более читаемо, чем лишние if-ы..
# https://stackoverflow.com/questions/448910/what-is-the-difference-between-the-gnu-makefile-variable-assignments-a

ifeq ($(wildcard $(BOOT_DIR)),)
 $(error BOOT_DIR is not accessible. This makefile needs path to directory with vmlinuz file for qemu -kernel argument)
endif

initramfs.gz: $(wildcard root/*)
	mkdir -p root/proc root/sys root/tmp root/dev
	cd root && find . -print0 | cpio -o --null --format=newc | gzip > ../initramfs.gz

# https://stackoverflow.com/questions/53623170/how-can-i-access-the-kernel-command-line-from-a-linux-kernel-module
# https://tldp.org/LDP/lkmpg/2.6/html/x323.html
# Не получается передать phonebook.debug=1 при загрузке ядра. Видимо, этот параметр
#   не будет получен модулем, т.к. он передан при загрузке, а модуль загружается
#   позже.. Можно передать при insmod. Видимо, если это встроенный модуль, ядро само
#   передает параметры модуля, которые были переданы в параметрах ядра. Ведь оно
#   тоже делает insmod в каком-то смысле. А если не встроенный, не передаст..
test: initramfs.gz
	qemu-system-x86_64 -kernel $(BOOT_DIR)/vmlinuz-* -initrd initramfs.gz -enable-kvm -cpu host -nographic -append "console=ttyS0"

clean:
	rm -f initramfs.gz

.PHONY: all clean
