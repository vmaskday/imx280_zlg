
/* 
 *  * XUJI's concentrator LED driver
 *   * Author: Liu Yang <liuyang@edmail.zlgmcu.com>
 *    */
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include  <asm/uaccess.h>
#include <linux/delay.h>

#include <mach/lpc32xx_uart.h>
#include <mach/lpc32xx_clkpwr.h>
#include <mach/hardware.h>
#include <mach/platform.h>
#include <mach/lpc32xx_gpio.h>
#include <asm/uaccess.h>
#include <asm/io.h>


#define DATA_LEN	  100						/* 采样点个数			*/

#define CHANNEL0          0x00    
#define CHANNEL1          0x01    
#define CHANNEL2          0x02
#define CHANNEL3          0x03
#define CHANNEL4          0x04
#define CHANNEL5          0x05
#define CHANNEL6          0x06
#define CHANNEL7          0x07

#define CH0_SEL		  	118
#define CH1_SEL			119
#define CH2_SEL			120
#define CH3_SEL			121
#define CH4_SEL			122
#define CH5_SEL			123
#define CH6_SEL			124
#define CH7_SEL			125

#define	DEV_NAME	"daq1000-adc"

#define GPIO_IOBASE io_p2v(GPIO_BASE)

#define CHSLE0		OUTP_STATE_GPO(6)
#define CHSLE1		OUTP_STATE_GPO(7)
#define CHSLE2		OUTP_STATE_GPO(11)

int gpio_direction_output(unsigned gpio, int value);

static inline void ship_select(void)
{
	 __raw_writel(OUTP_STATE_GPO(1), GPIO_P3_OUTP_CLR(GPIO_IOBASE));
}

static inline void ship_release(void)
{
	__raw_writel(OUTP_STATE_GPO(1), GPIO_P3_OUTP_SET(GPIO_IOBASE));

}

static inline void clk_high(void) 
{	
	__raw_writel(OUTP_STATE_GPO(0), GPIO_P3_OUTP_SET(GPIO_IOBASE));
}

static inline  void clk_low(void)
{
	__raw_writel(OUTP_STATE_GPO(0), GPIO_P3_OUTP_CLR(GPIO_IOBASE));
}

static inline u8 readbit(void)
{
	 return  (__raw_readl(GPIO_P3_INP_STATE(GPIO_IOBASE)) >> 28) & 0x01;
}

static inline void loop_ten(int n)
{
#if 0
	int i, j;
	
	for (i = 0; i < n; i++)
		for (j = 0; j < 15; j++);
#else
	udelay(n);
#endif
}

static u16 amic1608_adc_readdate(void) 
{
	int i;
	u16 data = 0;

	/*
 	 * 启动转换
 	 * 使用模拟SPI控制AMiC-1608AI-5ID时，必须先把CLK置低，再把CS置低启动采样，否则将不能正确读取采样值
 	 */
	clk_low();
	//loop_ten(1);
	ship_select();
	/*
 	 * 先发出6个CLK出去，在这段时间内数据无效，所以不用读取数据
 	 */
	for(i = 0; i < 6; i++) {
		clk_high();
		loop_ten(1);
		clk_low();
		loop_ten(1);
	}

	for (i = 0; i< 16; i++) {
		clk_low();
		data <<= 1;	
		loop_ten(1);
		clk_high();
		loop_ten(1);
		data |= readbit();
	}	
	ship_release();

	return data;
}

static void select_channel(u16 channel)
{
	if (channel  & 0x01) {
	 	__raw_writel(CHSLE0, GPIO_P3_OUTP_SET(GPIO_IOBASE));
	} else {
	 	__raw_writel(CHSLE0, GPIO_P3_OUTP_CLR(GPIO_IOBASE));
	}

	if (channel & 0x02) {
	 	__raw_writel(CHSLE1, GPIO_P3_OUTP_SET(GPIO_IOBASE));
	} else {
	 	__raw_writel(CHSLE1, GPIO_P3_OUTP_CLR(GPIO_IOBASE));
	}

	if (channel & 0x04) {
	 	__raw_writel(CHSLE2, GPIO_P3_OUTP_SET(GPIO_IOBASE));
	} else {
	 	__raw_writel(CHSLE2, GPIO_P3_OUTP_CLR(GPIO_IOBASE));
	}

}

DECLARE_MUTEX(channel_mutex);


static u16 read_adc_value(int channel)
{
	u16 data = 0;
	u32 sum_value = 0;
	int i = 0;
#if 0
	down(&channel_mutex);
	
	select_channel(channel);
	mdelay(10);
	for (i = 0; i < DATA_LEN; i++) {
		sum_value += amic1608_adc_readdate();
	}

	data = sum_value/DATA_LEN;	

	up(&channel_mutex);
#else
	down(&channel_mutex);
	
	select_channel(channel);

	udelay(100);

	data = amic1608_adc_readdate();

	up(&channel_mutex);

#endif
	return data;
}


#define DEVICE_ADC_ATTR_I(chl_num,gpio_num,attr_name) 					\
	static ssize_t daq1000_adc_sysfs_read_daq1000_##attr_name(		 	\
		struct device *dev, struct device_attribute   *attr, char *buf) 	\
{											\
	gpio_direction_output(gpio_num, 1);						\
	return sprintf(buf, "%d\n", read_adc_value(chl_num));				\
}											\
static DEVICE_ATTR(attr_name, S_IRUGO | S_IWUSR, 					\
		   daq1000_adc_sysfs_read_daq1000_##attr_name, NULL);			\

DEVICE_ADC_ATTR_I(CHANNEL0, CH0_SEL, channel0_I)
DEVICE_ADC_ATTR_I(CHANNEL1, CH1_SEL, channel1_I)
DEVICE_ADC_ATTR_I(CHANNEL2, CH2_SEL, channel2_I)
DEVICE_ADC_ATTR_I(CHANNEL3, CH3_SEL, channel3_I)
DEVICE_ADC_ATTR_I(CHANNEL4, CH4_SEL, channel4_I)
DEVICE_ADC_ATTR_I(CHANNEL5, CH5_SEL, channel5_I)
DEVICE_ADC_ATTR_I(CHANNEL6, CH6_SEL, channel6_I)
DEVICE_ADC_ATTR_I(CHANNEL7, CH7_SEL, channel7_I)

#define DEVICE_ADC_ATTR_V(chl_num,gpio_num,attr_name) 					\
	static ssize_t daq1000_adc_sysfs_read_daq1000_##attr_name(			\
		struct device *dev, struct device_attribute   *attr, char *buf) 	\
{											\
	gpio_direction_output(gpio_num, 0);						\
	return sprintf(buf, "%d\n", read_adc_value(chl_num));				\
}											\
static DEVICE_ATTR(attr_name, S_IRUGO | S_IWUSR, 					\
		   daq1000_adc_sysfs_read_daq1000_##attr_name, NULL);	

DEVICE_ADC_ATTR_V(CHANNEL0, CH0_SEL, channel0_V)
DEVICE_ADC_ATTR_V(CHANNEL1, CH1_SEL, channel1_V)
DEVICE_ADC_ATTR_V(CHANNEL2, CH2_SEL, channel2_V)
DEVICE_ADC_ATTR_V(CHANNEL3, CH3_SEL, channel3_V)
DEVICE_ADC_ATTR_V(CHANNEL4, CH4_SEL, channel4_V)
DEVICE_ADC_ATTR_V(CHANNEL5, CH5_SEL, channel5_V)
DEVICE_ADC_ATTR_V(CHANNEL6, CH6_SEL, channel6_V)
DEVICE_ADC_ATTR_V(CHANNEL7, CH7_SEL, channel7_V)



static int daq1000_adc_sysfs_register(struct device   *dev)
{
	int	err;


	err	= device_create_file(dev, &dev_attr_channel0_I);
	if (err)
		goto err0;

	err	= device_create_file(dev, &dev_attr_channel1_I);
	if (err) 
		goto err1;

	err	= device_create_file(dev, &dev_attr_channel2_I);
	if (err)
		goto err2;

	err	= device_create_file(dev, &dev_attr_channel3_I);
	if (err)
		goto err3;

	err	= device_create_file(dev, &dev_attr_channel4_I);
	if (err)
		goto err4;

	err	= device_create_file(dev, &dev_attr_channel5_I);
	if (err)
		goto err5;

	err	= device_create_file(dev, &dev_attr_channel6_I);
	if (err)
		goto err6;

	err	= device_create_file(dev, &dev_attr_channel7_I);
	if (err)
		goto err7;

	err	= device_create_file(dev, &dev_attr_channel0_V);
	if (err)
		goto err8;

	err	= device_create_file(dev, &dev_attr_channel1_V);
	if (err)
		goto err9;

	err	= device_create_file(dev, &dev_attr_channel2_V);
	if (err)
		goto err10;

	err	= device_create_file(dev, &dev_attr_channel3_V);
	if (err)
		goto err11;

	err	= device_create_file(dev, &dev_attr_channel4_V);
	if (err)
		goto err12;

	err	= device_create_file(dev, &dev_attr_channel5_V);
	if (err)
		goto err13;

	err	= device_create_file(dev, &dev_attr_channel6_V);
	if (err)
		goto err14;

	err	= device_create_file(dev, &dev_attr_channel7_V);
	if (err)
		goto err15;

	return 0;

err15:
	device_remove_file(dev, &dev_attr_channel6_V);
err14:
	device_remove_file(dev, &dev_attr_channel5_V);
err13:
	device_remove_file(dev, &dev_attr_channel4_V);
err12:
	device_remove_file(dev, &dev_attr_channel3_V);
err11:
	device_remove_file(dev, &dev_attr_channel2_V);
err10:
	device_remove_file(dev, &dev_attr_channel1_V);
err9:
	device_remove_file(dev, &dev_attr_channel0_V);
err8:
	device_remove_file(dev, &dev_attr_channel7_I);
err7:
	device_remove_file(dev, &dev_attr_channel6_I);
err6:
	device_remove_file(dev, &dev_attr_channel5_I);
err5:
	device_remove_file(dev, &dev_attr_channel4_I);
err4:
	device_remove_file(dev, &dev_attr_channel3_I);
err3:
	device_remove_file(dev, &dev_attr_channel2_I);
err2:
	device_remove_file(dev, &dev_attr_channel1_I);
err1:
	device_remove_file(dev, &dev_attr_channel0_I);
err0:
	return err;
}

static int
daq1000_adc_sysfs_unregister(struct device *dev)
{
	device_remove_file(dev, &dev_attr_channel7_V);
	device_remove_file(dev, &dev_attr_channel6_V);
	device_remove_file(dev, &dev_attr_channel5_V);
	device_remove_file(dev, &dev_attr_channel4_V);
	device_remove_file(dev, &dev_attr_channel3_V);
	device_remove_file(dev, &dev_attr_channel2_V);
	device_remove_file(dev, &dev_attr_channel1_V);
	device_remove_file(dev, &dev_attr_channel0_V);
	device_remove_file(dev, &dev_attr_channel7_I);
	device_remove_file(dev, &dev_attr_channel6_I);
	device_remove_file(dev, &dev_attr_channel5_I);
	device_remove_file(dev, &dev_attr_channel4_I);
	device_remove_file(dev, &dev_attr_channel3_I);
	device_remove_file(dev, &dev_attr_channel2_I);
	device_remove_file(dev, &dev_attr_channel1_I);
	device_remove_file(dev, &dev_attr_channel0_I);

	return 0;
}

static int __devinit daq1000_adc_probe(struct platform_device   *pdev)
{
	int tmp = 0;

	__raw_writel((1 << 6), GPIO_P3_MUX_CLR(GPIO_IOBASE));   	/* 设置GPO_06为GPO功能引脚      */

	/*
	 ** 在UART控制器中不使用modem控制管脚 , 这主要是配置GPI_28只作为GPI管脚使用
	 */
	tmp = __raw_readl(UARTCTL_CTRL(io_p2v(UART_CTRL_BASE)));
	tmp &= ~(1 << 11);
	__raw_writel(tmp, UARTCTL_CTRL(io_p2v(UART_CTRL_BASE)));


	return daq1000_adc_sysfs_register(&pdev->dev);
}

static int __devexit daq1000_adc_remove(struct platform_device  *pdev)
{
	return daq1000_adc_sysfs_unregister(&pdev->dev);
}

static void daq1000_adc_device_release(struct device *dev)
{
	/* 
	 **	register/unregister device in driver is NOT a regular way.
 	 **	So, this function is needed although it's empty.
	 **/
}

static struct platform_device	daq1000_adc_device	= {
	.name	= DEV_NAME,
	.id		= -1,
	.num_resources	= 0,
	.dev	= {
		.release	= daq1000_adc_device_release,
	},
};

static struct platform_driver	daq1000_adc_driver	= {
	.driver	= {
		.owner	= THIS_MODULE,
		.name	= DEV_NAME,
	},
	.probe	= daq1000_adc_probe,
	.remove	= daq1000_adc_remove,
};

static int __init daq1000_adc_init(void)
{
	int	err;

	err	= platform_device_register(&daq1000_adc_device);
	if (err)
		return err;

	return platform_driver_register(&daq1000_adc_driver);
}

static void __exit daq1000_adc_exit(void)
{
	platform_driver_unregister(&daq1000_adc_driver);
	platform_device_unregister(&daq1000_adc_device);
}

module_init(daq1000_adc_init);
module_exit(daq1000_adc_exit);

MODULE_AUTHOR("zhuguojun <liuyang@edmail.zlgmcu.com>");
MODULE_DESCRIPTION("driver for DAQ1000I-L ADC device");
MODULE_LICENSE("GPL");

