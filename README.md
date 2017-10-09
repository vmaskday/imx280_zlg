#porting new linux kernel and u-boot to imx280#

##Init environment##

fetch code:<br>
<pre>
 $ git clone https://github.com/QtForQT/imx280_zlg.git
</pre>
set TopDir<br>
<pre>
 $ cd imx280_zlg
 $ TopDir=`pwd`
</pre>
setup cross compile:<br>
<pre>
 $ cd ${TopDir}
 $ wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/6-2017q2/gcc-arm-none-eabi-6-2017-q2-update-linux.tar.bz2
 $ tar xvf gcc-arm-none-eabi-6-2017-q2-update-linux.tar.bz2
 $ mv gcc-arm-none-eabi-6-2017-q2-update cross_compile/
</pre>
Build tools:<br>
<pre>
 $ cd ${TopDir}
 $make tools
</pre>

##Build code##

###Build u-boot-2017.09###
<pre>
 $ cd ${TopDir}
 $ cd bootloader/u-boot-2017.09/
 $ ./mk.sh
</pre>
Download u-boot-2017.09 to EasyArm-imx280a board<br>
<pre>
 $ ./flash_imx280.sh
</pre>

###Build u-boot-2009###
<pre>
 $ cd ${TopDir}
 $ make bootloader
</pre>

###Build Linux kernel###
<pre>
 $ cd ${TopDir}
 $ make kernel  #build linux kernel 2.6
 $ make kernel_4  #build linux kernel 4.6
</pre>

###Build bootstream###
<pre>
 $ cd ${TopDir}
 $ make bootstream
</pre>

###Download images###
<pre>
 $ cd ${TopDir}
 $ ./flash_imx280.sh bootloader/imx-bootlets-src-10.12.01/imx28_ivt_linux.sb   #download linux kernel
 or
 $ ./flash_imx280.sh bootloader/imx-bootlets-src-10.12.01/imx28_ivt_uboot.sb   #download u-boot
</pre>
