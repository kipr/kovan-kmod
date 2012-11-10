obj-m := kovan-kmod.o
kovan-kmod-objs :=kovan-kmod-spi.o
KLOC = ../linux
all:
	make -C $(KLOC) M=$(PWD) modules
clean:
	make -C $(KLOC) M=$(PWD) clean
