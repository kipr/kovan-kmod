obj-m := kovan.o
kovan-objs := kovan-kmod.o kovan-kmod-spi.o
ccflags-y := -std=gnu99 -Wno-declaration-after-statement

all:
	make -C $(KERNEL_SOURCE) ARCH=$(ARCH) SUBDIRS=$(CURDIR) CC=${CC} modules
clean:
	make -C $(KLOC) M=$(PWD) clean
