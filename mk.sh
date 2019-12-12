# !/bin/sh

TOP_DIR=`pwd`
_cross_path=`cd ./cross_compile/bin/;pwd`
cross=${_cross_path}/arm-linux-gnueabi-
OUT_DIR=${TOP_DIR}/OUT
if [ ! -d "OUT" ]
then
	mkdir OUT
fi
if [ X$1 == X ]
then
	make all
	cp OUT/uboot/boot.sb OUT/
	cp OUT/kernel/arch/arm/boot/zImage OUT/
	cp OUT/kernel/arch/arm/boot/dts/imx28-evk.dtb OUT/
else
	make -C kernel/linux-4.19.88/ ARCH=arm CROSS_COMPILE=${cross} O=${OUT_DIR}/kernel $1
fi
#${cross}readelf -wL OUT/kernel5/vmlinux |grep "CU:" |sed -e 's/CU: .//g' -e 's/:$//g' > kernel/linux-5.3.7/cscope.files
