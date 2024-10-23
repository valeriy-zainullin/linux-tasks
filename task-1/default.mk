default_boot_dir := ../boot
BOOT_DIR ?= $(mkfile_dir)/$(default_boot_dir)
# Увидел в файле Kbuild в корне репозитория ядра линукса присваивание ?=.
# Намного более читаемо, чем лишние if-ы..
# https://stackoverflow.com/questions/448910/what-is-the-difference-between-the-gnu-makefile-variable-assignments-a

ifeq ($(wildcard $(BOOT_DIR)),)
 $(error BOOT_DIR is not accessible. This makefile needs path to directory with vmlinuz file for qemu -kernel argument)
endif

KDIR         := $(shell pwd)/../linux-6.11
PWD          := $(shell pwd)

TEST_FILES   := $(wildcard tests/*.sh tests/bin/*) tests/init
TEST_PROGS   := $(wildcard tests/*.c)

all: phonebook.ko

phonebook.ko: $(srcs)
	$(MAKE) -C $(KDIR) M=$(PWD) modules

install: phonebook.ko
	install -o root -g root -m 400 phonebook.ko ../root

root: | tests
	mkdir -p $@

root/phonebook.ko: phonebook.ko | root
	cp $< $@

$(TEST_FILES:tests/%=root/%) : root/% : tests/% | root
	mkdir -p $(dir $@)
	cp $< $@

# https://stackoverflow.com/questions/2548486/compiling-without-libc
# https://stackoverflow.com/questions/61553723/whats-the-difference-between-statically-linked-and-not-a-dynamic-executable
#   Оказывается, в gcc от ubuntu по умолчанию включен PIE. Что, наверно, хорошо. Это можно
#   понять по gcc -v, там выдаются параметры скрипта настройки сборки ./configure, использованные
#   составителями пакета. --enable-default-pie присутствует. Тогда надо --static-pie, чтобы
#   не требовался интерпретатор ld.
$(TEST_PROGS:tests/%.c=root/%.run) : root/%.run : tests/%.c | root
	mkdir -p $(dir $@)
	gcc -nostdlib --static -Wall -Wextra -Wshadow -Werror -O0 -g -std=c99 -I$(mkfile_dir)/include -o $@ $<

initramfs.gz: root/phonebook.ko $(TEST_FILES:tests/%=root/%) $(TEST_PROGS:tests/%.c=root/%.run) | root
	chmod 755 root/init root/bin/* root/*.run
	mkdir -p root/dev root/tmp root/proc root/sys
	cd root && find . -print0 | cpio -o --null --format=newc | gzip > ../initramfs.gz

# https://stackoverflow.com/questions/53623170/how-can-i-access-the-kernel-command-line-from-a-linux-kernel-module
# https://tldp.org/LDP/lkmpg/2.6/html/x323.html
# Не получается передать phonebook.debug=1 при загрузке ядра. Видимо, этот параметр
#   не будет получен модулем, т.к. он передан при загрузке, а модуль загружается
#   позже.. Можно передать при insmod. Видимо, если это встроенный модуль, ядро само
#   передает параметры модуля, которые были переданы в параметрах ядра. Ведь оно
#   тоже делает insmod в каком-то смысле. А если не встроенный, не передаст..
test: initramfs.gz
	qemu-system-x86_64 -kernel $(BOOT_DIR)/vmlinuz-* -initrd initramfs.gz -nographic -enable-kvm -cpu host -append "console=ttyS0"

clean:
	make -C ../linux-6.11 M=$(PWD) clean
	rm -rf root
	rm initramfs.gz

.PHONY: all clean
