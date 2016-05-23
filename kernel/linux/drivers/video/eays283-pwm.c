#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>

#include <mach/lcdif.h>
#include <mach/regulator.h>

static int init_bl(struct mxs_platform_bl_data *data)
{
        int ret = 0;

        pwm_clk = clk_get(NULL, "pwm");
        if (IS_ERR(pwm_clk)) {
                ret = PTR_ERR(pwm_clk);
                return ret;
        }
        clk_enable(pwm_clk);
//      printk("%s,%d:set the back light on \n",__func__,__LINE__);
        mxs_reset_block(REGS_PWM_BASE, 1);

        __raw_writel(BF_PWM_ACTIVEn_INACTIVE(0) |
                     BF_PWM_ACTIVEn_ACTIVE(0),
                     REGS_PWM_BASE + HW_PWM_ACTIVEn(3));
        __raw_writel(BF_PWM_PERIODn_CDIV(6) |   /* divide by 64 */
                     BF_PWM_PERIODn_INACTIVE_STATE(2) | /* low */
                     BF_PWM_PERIODn_ACTIVE_STATE(3) |   /* high */
                     BF_PWM_PERIODn_PERIOD(399),
                     REGS_PWM_BASE + HW_PWM_PERIODn(3));
        __raw_writel(BM_PWM_CTRL_PWM3_ENABLE, REGS_PWM_BASE + HW_PWM_CTRL_SET);

        return 0;
}

static void free_bl(struct mxs_platform_bl_data *data)
{
//      printk("%s,%d:set the back light on \n",__func__,__LINE__);
        __raw_writel(BF_PWM_ACTIVEn_INACTIVE(0) |
                     BF_PWM_ACTIVEn_ACTIVE(0),
                     REGS_PWM_BASE + HW_PWM_ACTIVEn(3));
        __raw_writel(BF_PWM_PERIODn_CDIV(6) |   /* divide by 64 */
                     BF_PWM_PERIODn_INACTIVE_STATE(2) | /* low */
                     BF_PWM_PERIODn_ACTIVE_STATE(3) |   /* high */
                     BF_PWM_PERIODn_PERIOD(399),
                     REGS_PWM_BASE + HW_PWM_PERIODn(3));
        __raw_writel(BM_PWM_CTRL_PWM3_ENABLE, REGS_PWM_BASE + HW_PWM_CTRL_CLR);

        clk_disable(pwm_clk);
        clk_put(pwm_clk);
}

static int set_bl_intensity(struct mxs_platform_bl_data *data,
                            struct backlight_device *bd, int suspended)
{
        int intensity = bd->props.brightness;
        int scaled_int;

        if (bd->props.power != FB_BLANK_UNBLANK)
                intensity = 0;
        if (bd->props.fb_blank != FB_BLANK_UNBLANK)
                intensity = 0;
        if (suspended)
                intensity = 0;

        /*
         * This is not too cool but what can we do?
         * Luminance changes non-linearly...
         */
        if (regulator_set_current_limit
            (data->regulator, bl_to_power(intensity), bl_to_power(intensity)))
                return -EBUSY;

        scaled_int = values[intensity / 10];
        if (scaled_int < 100) {
                int rem = intensity - 10 * (intensity / 10);    // r = i % 10;
                scaled_int += rem * (values[intensity / 10 + 1] -
                                     values[intensity / 10]) / 10;
        }

        __raw_writel(BF_PWM_ACTIVEn_INACTIVE(scaled_int*399/100) |
                     BF_PWM_ACTIVEn_ACTIVE(0),
                     REGS_PWM_BASE + HW_PWM_ACTIVEn(3));
        __raw_writel(BF_PWM_PERIODn_CDIV(6) |   /* divide by 64 */
                     BF_PWM_PERIODn_INACTIVE_STATE(2) | /* high  */
                     BF_PWM_PERIODn_ACTIVE_STATE(3) |   /* low */
                     BF_PWM_PERIODn_PERIOD(399),
                     REGS_PWM_BASE + HW_PWM_PERIODn(3));
        return 0;
}


static int mxsbl_get_intensity(struct backlight_device *bd)
{
        struct platform_device *pdev = dev_get_drvdata(&bd->dev);
        struct mxs_bl_data *data = platform_get_drvdata(pdev);

        return data->current_intensity;
}

static struct backlight_ops mxsbl_ops = {
        .get_brightness = mxsbl_get_intensity,
        .update_status  = mxsbl_set_intensity,
};


static struct platform_driver easy283_pwm_driver = {
        .probe          = pwm_probe,
        .remove         = __devexit_p(pwm_remove),
        .suspend        = pwm_suspend,
        .resume         = pwm_resume,
        .driver         = {
                .name   = "easy283-pwm",
                .owner  = THIS_MODULE,
        },
};


static int __init easy283_pwm_init(void)
{
        return platform_driver_register(&easy283_pwm_driver);
}

static void __exit easy283_pwm_exit(void)
{
        platform_driver_unregister(&easy283_pwm_driver);
}


module_init(easy283_pwm_init);
module_exit(easy283_pwm_exit);

MODULE_AUTHOR("ZLG qjw");
MODULE_DESCRIPTION("EasyARM-iMX283 PWM_4 PWM7  Driver");
MODULE_LICENSE("GPL");


