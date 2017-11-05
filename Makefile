OUT_DIR := $(CURDIR)/OUT
$(shell [ -d ${OUT_DIR} ] || mkdir -p ${OUT_DIR})
_cross_compile=$(shell cd cross_compile/bin/;pwd)
_out_dir=$(shell cd $OUT_DIR;pwd)

CC=${_cross_compile}/arm-none-eabi-

.PHONY: bootloader kernel tools bootstream 

all: bootloader kernel
	@echo -e "\t\033[32m build success . \033[0m"

bootloader:
	${MAKE} -C bootloader/u-boot-2017.09/ ARCH=arm CROSS_COMPILE=${CC} O=${OUT_DIR}/uboot mx28evk_defconfig
	${MAKE} -C bootloader/u-boot-2017.09/ ARCH=arm CROSS_COMPILE=${CC} O=${OUT_DIR}/uboot u-boot.sb -j8
	cp bootloader/u-boot-2017.09/bootstream/uboot_ivt.bd ${OUT_DIR}/uboot/
	cd ${OUT_DIR}/uboot; ../tools/elftosb -f imx28 -c uboot_ivt.bd -o boot.sb
	#$(OUT_DIR)/tools/elftosb -f imx28 -c ${OUT_DIR}/uboot/uboot_ivt.bd -o ${OUT_DIR}/uboot/boot.sb

kernel:
	make -C kernel/linux_stable/ ARCH=arm CROSS_COMPILE=${CC} O=${OUT_DIR}/kernel mxs_defconfig
	make -C kernel/linux_stable/ ARCH=arm CROSS_COMPILE=${CC} O=${OUT_DIR}/kernel -j8

tools:
	make -C elftosb/ O=$(OUT_DIR)/tools/
	rm $(OUT_DIR)/tools/*.o
	make -C imxtools/sbtools/ O=$(OUT_DIR)/tools/
	rm $(OUT_DIR)/tools/*.o
	make -C imxtools/uuc/ O=$(OUT_DIR)/tools/
	rm $(OUT_DIR)/tools/*.o
	strip $(OUT_DIR)/tools/*

clean:
	rm OUT/ -rf
