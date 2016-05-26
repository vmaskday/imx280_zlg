OUT_DIR := $(CURDIR)/OUT
$(shell [ -d ${OUT_DIR} ] || mkdir -p ${OUT_DIR})
CC=arm-none-eabi-

.PHONY: bootloader kernel tools

all: bootloader kernel tools kernel_4
	@build success .

bootloader:
	${MAKE} -C bootloader/u-boot-2009.08 ARCH=arm CROSS_COMPILE=${CC} O=${OUT_DIR}/uboot mx28_evk_config
	${MAKE} -C bootloader/u-boot-2009.08 ARCH=arm CROSS_COMPILE=${CC} O=${OUT_DIR}/uboot -j4

kernel:
	${MAKE} -C kernel/linux ARCH=arm CROSS_COMPILE=${CC} O=${OUT_DIR}/linux_2.6 EasyARM-iMX280A_defconfig
	${MAKE} -C kernel/linux ARCH=arm CROSS_COMPILE=${CC} O=${OUT_DIR}/linux_2.6 -j4

kernel_4:
	mkdir -p ${OUT_DIR}/linux_4
	make -C kernel/linux_stable/ ARCH=arm CROSS_COMPILE=${CC} O=${OUT_DIR}/kernel_4.4 mxs_defconfig
	make -C kernel/linux_stable/ ARCH=arm CROSS_COMPILE=${CC} O=${OUT_DIR}/kernel_4.4 -j4

tools:
	make -C imxtools/sbtools/ O=$(OUT_DIR)/tools/

