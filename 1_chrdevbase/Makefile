KERNEL_DIR=../../ebf_linux_kernel_6ull_depth1/build_image/build

ARCH=arm
CROSS_COMPILE=arm-linux-gnueabihf-
export  ARCH  CROSS_COMPILE

CURRENT_PATH := $(shell pwd)
obj-m := chrdevbase.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNEL_DIR) M=$(CURRENT_PATH) modules

.PHONE: clean
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(CURRENT_PATH) clean