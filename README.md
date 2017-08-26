porting new linux kernel to imx280
some tools for imx280
update build environment

build steps:
1.download cross compile
example:
https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads
file name: gcc-arm-none-eabi-6-2017-q2-update-linux.tar.bz2

$wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/6-2017q2/gcc-arm-none-eabi-6-2017-q2-update-linux.tar.bz2
$ tar xvf gcc-arm-none-eabi-6-2017-q2-update-linux.tar.bz2
$ mv gcc-arm-none-eabi-6-2017-q2-update cross_compile/

2.compile source files
$make

3.flash imge to device
$./flash_imx280.sh bootloader/imx-bootlets-src-10.12.01/imx28_ivt_linux.sb
