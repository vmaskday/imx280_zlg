OUT_DIR := $(CURDIR)/OUT
$(shell [ -d ${OUT_DIR} ] || mkdir -p ${OUT_DIR})

all: bootloader kernel tools
	@build success .
bootloader:
	cd bootloader/u-boot-2009.08  ;\
		echo 1| ./build-uboot ;\
		echo 2| ./build-uboot ;\
		echo 3| ./build-uboot

kernel:
	cd linux ;\
	echo 3|./config-kernel ;\
	echo 6|./config-kernel

tools:
	make -C imxtools/sbtools/ O=$(OUT_DIR)/tools/

