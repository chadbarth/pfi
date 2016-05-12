export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-

KDIR := ../bb-kernel/KERNEL

obj-m+=pfi.o
 
all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean

