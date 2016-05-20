/*
 * Copyright (C) 2009-2010 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>

#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>

#include <mach/hardware.h>
#include <mach/device.h>
#include <mach/pinctrl.h>

#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/physmap.h>

#include "device.h"
#include "mx28evk.h"

static struct i2c_board_info __initdata mxs_i2c_device[] = {
	//{ I2C_BOARD_INFO("sgtl5000-i2c", 0xa), .flags = I2C_M_TEN }
	//{ I2C_BOARD_INFO("rtc-pcf8563", 0x51)  }
	{ I2C_BOARD_INFO("pcf8563", 0x51)  }
};

static void __init i2c_device_init(void)
{
	//i2c_register_board_info(0, mxs_i2c_device, ARRAY_SIZE(mxs_i2c_device));
	i2c_register_board_info(1, mxs_i2c_device, ARRAY_SIZE(mxs_i2c_device));
}
#if defined(CONFIG_MTD_M25P80) || defined(CONFIG_MTD_M25P80_MODULE)
static struct flash_platform_data mx28_spi_flash_data = {
	.name = "m25p80",
	.type = "w25x80",
};
#endif

static struct spi_board_info spi_board_info[] __initdata = {
#if defined(CONFIG_MTD_M25P80) || defined(CONFIG_MTD_M25P80_MODULE)
#if 0
	{
		/* the modalias must be the same as spi device driver name */
		.modalias = "m25p80", /* Name of spi_driver for this device */
		.max_speed_hz = 20000000,     /* max spi clock (SCK) speed in HZ */
		.bus_num = 1, /* Framework bus number */
		.chip_select = 0, /* Framework chip select. */
		.platform_data = &mx28_spi_flash_data,
	},
#endif
#endif
	{
		/* the modalias must be the same as spi device driver name */
                .modalias = "spidev", /* Name of spi_driver for this device */
                .max_speed_hz = 20000000,     /* max spi clock (SCK) speed in HZ */
                .bus_num = 1, /* Framework bus number */
                .chip_select = 0, /* Framework chip select.0:ssn0 1 ssn1 2 ssn2 */
	}
};

static void spi_device_init(void)
{
	spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
}

static void __init fixup_board(struct machine_desc *desc, struct tag *tags,
			       char **cmdline, struct meminfo *mi)
{
	mx28_set_input_clk(24000000, 24000000, 32000, 50000000);
}

#if defined(CONFIG_LEDS_MXS) || defined(CONFIG_LEDS_MXS_MODULE)
extern int mxs_led_set(unsigned index, int value);
static struct mxs_led  mx28evk_led[3] = {
	[0] = {
		.name = "led-run",
		.default_trigger = "heartbeat",
		.index = 0,
		.led_set = mxs_led_set, 
		},
	[1] = {
		.name = "led-err",
		.index = 1,
		.led_set = mxs_led_set,
		},
	[2] = {
                .name = "beep",
                .index = 2,
                .led_set = mxs_led_set,
                },

};

struct mxs_leds_plat_data mx28evk_led_data = {
	.num = ARRAY_SIZE(mx28evk_led),
	.leds = mx28evk_led,
};
/*
static struct resource mx28evk_led_res = {
	.flags = IORESOURCE_MEM,
	.start = PWM_PHYS_ADDR,
	.end   = PWM_PHYS_ADDR + 0x3FFF,
};
*/
static void __init mx28evk_init_leds(void)
{
	struct platform_device *pdev;

	pdev = mxs_get_device("mxs-leds", 0);
	if (pdev == NULL || IS_ERR(pdev))
		return;
//	pdev->resource = &mx28evk_led_res;
	pdev->num_resources = 0;
	pdev->dev.platform_data = &mx28evk_led_data;
	mxs_add_device(pdev, 3);
}
#else
static void __init mx28evk_init_leds(void)
{
	;
}
#endif

static void __init mx28evk_device_init(void)
{
	/* Add mx28evk special code */
	i2c_device_init();
	spi_device_init();
	mx28evk_init_leds();
}

static void __init mx28evk_init_machine(void)
{
#ifdef CONFIG_iMX_280
	printk("** kernel for EasyARM-i.MX280 \n");
#endif
#ifdef CONFIG_iMX_283
	printk("**kernel for EasyARM-i.MX283 \n");
#endif
#ifdef CONFIG_iMX_287
	printk("**kernel for EasyARM-i.MX283 \n");
#endif
	mx28_pinctrl_init();
	/* Init iram allocate */
#ifdef CONFIG_VECTORS_PHY_ADDR
	/* reserve the first page for irq vector table*/
	iram_init(MX28_OCRAM_PHBASE + PAGE_SIZE, MX28_OCRAM_SIZE - PAGE_SIZE);
#else
	iram_init(MX28_OCRAM_PHBASE, MX28_OCRAM_SIZE);
#endif

	mx28_gpio_init();
	mx28evk_pins_init();
	mx28_device_init();
	mx28evk_device_init();
}

MACHINE_START(MX28EVK, "Freescale MX28EVK board")
	.phys_io	= 0x80000000,
	.io_pg_offst	= ((0xf0000000) >> 18) & 0xfffc,
	.boot_params	= 0x40000100,
	.fixup		= fixup_board,
	.map_io		= mx28_map_io,
	.init_irq	= mx28_irq_init,
	.init_machine	= mx28evk_init_machine,
	.timer		= &mx28_timer.timer,
MACHINE_END
