# !/bin/sh

TOP_DIR=`pwd`

build_bootloader() {
	cd ${TOP_DIR}
	cd bootloader/u-boot-2009.08 
	echo 1| ./build-uboot
	echo 2| ./build-uboot
	echo 3| ./build-uboot
}
build_kernel() {
	cd ${TOP_DIR}
	cd linux
	echo 3|./config-kernel
	echo 6|./config-kernel
}
build_tools() {
	cd ${TOP_DIR}
	make -C imxtools/sbtools/ O=$(OUT_DIR)/tools/
}

build_tools
build_bootloader
build_kernel
