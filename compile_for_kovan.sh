make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- -f Makefile.cross
cp *.ko /media/KIPR/
cp -r ../kovan-kmod /media/KIPR/
cp ../kovan-fpga/kovan.bit /media/KIPR/
sync
