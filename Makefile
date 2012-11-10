obj-m := kovan.o
kovan-objs := kovan-kmod.o kovan-kmod-spi.o
ccflags-y := -c99 -std=gnu99

KLOC = ../linux
all:
	make -C $(KLOC) M=$(PWD) modules
clean:
	make -C $(KLOC) M=$(PWD) clean
