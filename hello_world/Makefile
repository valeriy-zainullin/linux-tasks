# https://lwn.net/Articles/21823/

ifneq ($(KERNELRELEASE),)
    obj-m	:= hello_world.o
else
KDIR	:= $(shell pwd)/../linux-6.11
PWD		:= $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	make -C ../linux-6.11 M=$(PWD) clean

install: hello_world.ko
	install -o root -g root -m 400 hello_world.ko ../root
endif
