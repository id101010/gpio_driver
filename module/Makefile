MODULE=led_driver_quad
KERNEL_VER=4.1.10
TOOLCHAIN:=/opt/crosstools/gcc-linaro-4.9-2015.05-x86_64_arm-linux-gnueabihf/bin
PLATFORM:=arm-linux-
CROSS_CMP:=$(TOOLCHAIN)/$(PLATFORM)
ROOT_FS_PATH:=/opt/embedded/bbb/rootfs

ifeq ($(KERNELRELEASE),)
    KERNEL_DIR	?= $(ROOT_FS_PATH)/lib/modules/$(KERNEL_VER)
    KERNEL_SRC = $(KERNEL_DIR)/source
    KERNEL_BUILD = $(KERNEL_DIR)/build
    # The current directory is passed to sub-makes as argument
    PWD := $(shell pwd)

modules:
	@echo 'Invoking build system'
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) ARCH=arm CROSS_COMPILE=$(CROSS_CMP) modules

modules_install:
	@echo 'Installing modules'
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) ARCH=arm INSTALL_MOD_PATH=$(ROOT_FS_PATH) modules_install

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.dtbo *.mod.c .tmp_versions Module.symvers modules.order

.PHONY: modules modules_install clean

else
    obj-m := $(MODULE).o
endif
