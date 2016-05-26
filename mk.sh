# !/bin/sh

TOP_DIR=`pwd`

build_bootloader() {
	echo ************ build $0 start **************
	cd ${TOP_DIR}
	cd bootloader/u-boot-2009.08 
	echo 1| ./build-uboot
	echo 2| ./build-uboot
	echo 3| ./build-uboot
}
build_kernel() {
	echo ************ build $0 start **************
	cd ${TOP_DIR}
#	cd kernel/linux
#	echo 3|./config-kernel
#	echo 6|./config-kernel
	cd kernel/linux_stable/
	make ARCH=arm CROSS_COMPILE=arm-none-eabi- O=${TOP_DIR}/OUT/kernel_4.4 mxs_defconfig
	make ARCH=arm CROSS_COMPILE=arm-none-eabi- O=${TOP_DIR}/OUT/kernel_4.4 -j8
}
build_tools() {
	echo ************ build $0 start **************
	cd ${TOP_DIR}
#	mkdir -p ${TOP_DIR}/tools/
	make -C imxtools/sbtools/ O=${TOP_DIR}/tools/
}

build_bootstream() {
	echo ************ build $0 start **************
	cd ${TOP_DIR}
	cd bootloader/imx-bootlets-src-10.12.01/
	./build
}

build_tools
build_kernel
cp OUT/kernel/arch/arm/boot/zImage  bootloader/imx-bootlets-src-10.12.01/
#cp OUT/kernel_4.4/arch/arm/boot/zImage  bootloader/imx-bootlets-src-10.12.01/
build_bootloader
cp ./OUT/bootloader/u-boot bootloader/imx-bootlets-src-10.12.01/
build_bootstream
