OUT_DIR := $(CURDIR)/OUT
$(shell [ -d ${OUT_DIR} ] || mkdir -p ${OUT_DIR})
_cross_compile=$(shell cd cross_compile/bin/;pwd)

CC=${_cross_compile}/arm-none-eabi-

.PHONY: bootloader kernel tools bootstream 

all: bootloader kernel_4  bootstream
	@echo -e "\t\033[32m build success . \033[0m"

bootloader:
	${MAKE} -C bootloader/u-boot-2009.08 ARCH=arm CROSS_COMPILE=${CC} O=${OUT_DIR}/uboot mx28_evk_config
	${MAKE} -C bootloader/u-boot-2009.08 ARCH=arm CROSS_COMPILE=${CC} O=${OUT_DIR}/uboot -j4
	$(shell cp ${OUT_DIR}/uboot/u-boot bootloader/imx-bootlets-src-10.12.01/)

kernel:
	${MAKE} -C kernel/linux ARCH=arm CROSS_COMPILE=${CC} O=${OUT_DIR}/linux_2.6 EasyARM-iMX280A_defconfig
	${MAKE} -C kernel/linux ARCH=arm CROSS_COMPILE=${CC} O=${OUT_DIR}/linux_2.6 -j4

kernel_4:
	make -C kernel/linux_stable/ ARCH=arm CROSS_COMPILE=${CC} O=${OUT_DIR}/kernel_4.4 mxs_defconfig
	make -C kernel/linux_stable/ ARCH=arm CROSS_COMPILE=${CC} O=${OUT_DIR}/kernel_4.4 -j4
	$(shell cp ${OUT_DIR}/kernel_4.4/arch/arm/boot/zImage bootloader/imx-bootlets-src-10.12.01/)

bootstream:${OUT_DIR}/kernel_4.4/arch/arm/boot/zImage ${OUT_DIR}/uboot/u-boot
	make -C bootloader/imx-bootlets-src-10.12.01/ CROSS_COMPILE=${CC} BOARD=iMX28_EVK

tools:
	make -C elftosb/ O=$(OUT_DIR)/tools/
	rm $(OUT_DIR)/tools/*.o
	make -C imxtools/sbtools/ O=$(OUT_DIR)/tools/
	rm $(OUT_DIR)/tools/*.o
	$(shell cp ${OUT_DIR}/tools/elftosb bootloader/imx-bootlets-src-10.12.01/)

clean:
	rm OUT/ -rf
	make -C bootloader/imx-bootlets-src-10.12.01/ clean
