obj-m += kovan-kmod.o
KLOC = ../linux
all:
	make -C $(KLOC) M=$(PWD) modules
clean:
	make -C $(KLOC) M=$(PWD) clean
