#make ARCH=arm  CROSS_COMPILE=arm-none-eabi- O=OUT m28evk_defconfig
_cross_path=`cd ../../cross_compile/bin/;pwd`
cross=${_cross_path}/arm-none-eabi-
make ARCH=arm  CROSS_COMPILE=${cross} O=OUT mx28evk_defconfig
make ARCH=arm  CROSS_COMPILE=${cross} O=OUT u-boot.sb  -j4
cp ./OUT/spl/u-boot-spl bootstream
cp ./OUT/u-boot bootstream
cp ./OUT/u-boot ./OUT/u-boot.elf #for Ozone debug
cp ./OUT/u-boot ../imx-bootlets-src-10.12.01/
#elftosb -zf imx28 -c arch/arm/cpu/arm926ejs/mxs/u-boot-imx28.bd
cd bootstream;./elftosb -f imx28 -c uboot_ivt.bd -o boot.sb;cd -

${cross}readelf -wL OUT/u-boot |grep "CU:" |sed -e 's/CU: .//g' -e 's/:$//g' > cscope.files
