obj-m := kovan.o
kovan-objs := kovan-kmod.o kovan-kmod-spi.o
KLOC = ../linux
all:
	make -C $(KLOC) M=$(PWD) modules
clean:
	make -C $(KLOC) M=$(PWD) clean
