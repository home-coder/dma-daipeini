export ARCH=arm
export CROSS_COMPILE=/home/jiangxj/a810/sdk/lichee/brandy/gcc-linaro/bin/arm-linux-gnueabi-
EXTRA_CFLAGS += $(DEBFLAGS) -Wall

obj-m += dma-mem2mem-eg2.o

KDIR ?= ~/a810/sdk/lichee/linux-3.4
PWD := $(shell pwd)
all:
	make $(EXTRA_CFLAGS) -C $(KDIR) M=$(PWD) modules
clean:
	rm -rf *.o *.ko *.mod.c *.symvers modules.order  .tmp_versions *.c~ *.cmd .dma-mem2mem*

