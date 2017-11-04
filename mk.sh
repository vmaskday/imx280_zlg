# !/bin/sh

TOP_DIR=`pwd`

make all
cp OUT/uboot/boot.sb OUT/
cp OUT/kernel/arch/arm/boot/zImage OUT/
cp OUT/kernel/arch/arm/boot/dts/imx28-evk.dtb OUT/
