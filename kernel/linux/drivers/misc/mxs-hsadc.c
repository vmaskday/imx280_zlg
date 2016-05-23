/*
 * Copyright (C) 2014 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#define DEBUG

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/sysdev.h>
#include <linux/bitops.h>
#include <linux/sysfs.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/fsl_devices.h>
#include <mach/hardware.h>
#include <mach/clock.h>
#include <mach/system.h>
#include <mach/dmaengine.h>
#include <mach/dma.h>

#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/uaccess.h>


#define SFTRST			(1<<31)
#define CLKGATE			(1<<30)
#define SOFT_TRIGGER	(1<<27)
#define RUN				(1<<0)
#define PWM_CTRL		0x0
#define PWM_CTRL_SET	0x04
#define PWM_CTRL_CLR	0x08
#define PWM_ACTIVE0		0x10
#define PWM_PERIOD0		0x20
#define PWM_ACTIVE1		0x30
#define PWM_PERIOD1		0x40
#define PWM_ACTIVE2		0x50
#define PWM_PERIOD2		0x60
#define HSADC_CTRL0		0x0
#define HSADC_CTRL0_SET	0x04
#define HSADC_CTRL0_CLR	0x08
#define HSADC_CTRL1		0x10
#define HSADC_CTRL1_SET	0x14
#define HSADC_CTRL1_CLR	0x18
#define HSADC_CTRL2		0x20
#define HSADC_CTRL2_SET	0x24
#define HSADC_CTRL2_CLR	0x28
#define HSADC_SEQUENCE_SAMPLES_NUM	0x30
#define HSADC_SEQUENCE_NUM	0x40

#define HSADCFRAC 18 /* 18 ~ 35 */
#define HSADCDIV 18  /* OK: 9,18  NG: 36,72 */
#define PLL0_FREQ 480000000UL
#define REF_HSADC_FREQ ((PLL0_FREQ/HSADCFRAC)*18)
#define HSADC_FREQ (REF_HSADC_FREQ/HSADCDIV)

#define HSADC_DEVICE_NAME "mxs-hsadc" /* hsadc device name */

#define DMA_BUF_SIZE 0x10000 

#define HSADC_DEBUG 0 

enum hsadc_adc_percision {
	HSADC_SAMPLE_PERCISION_8_BIT  = 0,
	HSADC_SAMPLE_PERCISION_10_BIT = 0x20000,
	HSADC_SAMPLE_PERCISION_12_BIT = 0x40000,
	HSADC_SAMPLE_PERCISION_MASK   = 0x60000
};

static int adc_sample_percision = 12;
module_param(adc_sample_percision, int, S_IRUGO);

struct mxs_hsadc_data {
	struct device *dev;
	void __iomem * hsadc_base;
	void __iomem * pwm_base;
	int dev_irq;
	int dma_irq;
	int dma_ch;
	struct clk *ref_hsadc_clk;
	struct clk *hsadc_clk;
	struct clk *pwm_clk;
	struct mxs_dma_desc *desc;
	void * buf;
	dma_addr_t buf_phy;
	unsigned int tx_cnt;
	
	/* char device and file operations interface */
	struct cdev    cdev_inst;    /* cdev instance */
	struct class  *cdev_class;   /* hsadc device class for sysfs */
	int            cdev_major;   /* hsadc major number */
	int            cdev_index;   /* hsadc index number */

	struct semaphore sem;        /* access lock */
	wait_queue_head_t r_wait;    /* wait queue for reading */
	
	unsigned long total_len;	// total read length
	unsigned long remaint_len;  // remaint length to read
	unsigned long seq_len;
	char * cur; // current position
	struct platform_device *pdev;
};

static irqreturn_t hsadc_dma_isr(int irq, void * p);
static irqreturn_t hsadc_isr(int irq, void * p);
void hsadc_cleanup_cdev(struct mxs_hsadc_data* pdx);
int init_hsadc_hw(struct mxs_hsadc_data *pd);

int submit_request(struct mxs_hsadc_data* pdx, unsigned long count)
{
	unsigned long sample_count;
	memset(pdx->buf, 0, count);

	pdx->desc->cmd.cmd.bits.bytes = count;
	pdx->desc->cmd.cmd.bits.pio_words = 0;
	pdx->desc->cmd.cmd.bits.wait4end = 1;
	pdx->desc->cmd.cmd.bits.dec_sem = 1;
	pdx->desc->cmd.cmd.bits.irq = 1;
	pdx->desc->cmd.cmd.bits.command = DMA_WRITE;
	pdx->desc->cmd.address = pdx->buf_phy;
	
	if(mxs_dma_desc_append(pdx->dma_ch, pdx->desc))
	{
		return -EINVAL;
	}

	//
	// byte(s) to sample count
	//
	sample_count = 
		adc_sample_percision == 8 ? count : (count >> 1); // 10-bit & 12-bit mode a sample word is two bytes size

	writel(sample_count, pdx->hsadc_base + HSADC_SEQUENCE_SAMPLES_NUM);
	writel(1,     pdx->hsadc_base + HSADC_SEQUENCE_NUM);

	writel(1<<31 | 1<<30 | 1<<29, pdx->hsadc_base + HSADC_CTRL1); // enable irq

	mxs_dma_reset(pdx->dma_ch);
	mxs_dma_ack_irq(pdx->dma_ch);
	mxs_dma_enable_irq(pdx->dma_ch, 1);
	if(mxs_dma_enable(pdx->dma_ch))
	{
		return -EINVAL;
	}

	writel(RUN, pdx->hsadc_base + HSADC_CTRL0_SET);
	writel(SOFT_TRIGGER, pdx->hsadc_base + HSADC_CTRL0_SET);
	
	return 0;
}


ssize_t hsadc_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
	struct mxs_hsadc_data *pdx = (struct mxs_hsadc_data*)filp->private_data;
	int ret = 0;

	DECLARE_WAITQUEUE(wait, current);

#if HSADC_DEBUG
	printk(KERN_INFO "%s> %s start.\n", HSADC_DEVICE_NAME, __FUNCTION__);
#endif

	/* someone has request zero byte data! */
	if(!count)
		return ret;

	/* push the request into the wait queue, it will be wakeup by the ISR */
	add_wait_queue(&pdx->r_wait, &wait);

	if(filp->f_flags & O_NONBLOCK)
	{
		ret = -EAGAIN;
#if HSADC_DEBUG
		printk(KERN_INFO "%s> file open with O_NONBLOCK.\n", HSADC_DEVICE_NAME);
#endif
		goto acc_out;
	}

	pdx->cur = buf;
	pdx->total_len = 
	pdx->remaint_len = 
		adc_sample_percision == 8 ? count : count &(~0x01);// in 10-bit & 12-bit mode the sample data word size is two bytes

	if(pdx->total_len <= DMA_BUF_SIZE)
		pdx->seq_len = count;
	else
		pdx->seq_len = DMA_BUF_SIZE;

	// init a read and wait for interrupt
	if(submit_request(pdx, pdx->seq_len))
	{
		goto acc_abort;
	}

	while(pdx->remaint_len)
	{
		/* set current process state into sleep */
		__set_current_state(TASK_INTERRUPTIBLE);
		
#if HSADC_DEBUG
		printk(KERN_INFO "%s> suspend.\n", HSADC_DEVICE_NAME);
#endif
		/* schedule other process to run */
		schedule();

#if HSADC_DEBUG
		printk(KERN_INFO "%s> wakeup.\n", HSADC_DEVICE_NAME);
#endif
		/* wakeup point */
		if(signal_pending(current))
		{
			/* no data, we are wakeuped by the signal, do nothing */
			ret = -ERESTARTSYS;
			goto acc_abort;
		}

		if(copy_to_user(pdx->cur, pdx->buf, pdx->seq_len))
		{
			ret = pdx->total_len - pdx->remaint_len;
			goto acc_abort;
		}
		else
		{
			// update position
			pdx->cur += pdx->seq_len;
			pdx->remaint_len -= pdx->seq_len;

			if(pdx->remaint_len > DMA_BUF_SIZE)
			{
				 pdx->seq_len = DMA_BUF_SIZE;
			}
			else
			{
				 // this is the last piease of data to read
				 pdx->seq_len = pdx->remaint_len;
			}
	
			if(pdx->seq_len)
				submit_request(pdx, pdx->seq_len);		
		}
#if HSADC_DEBUG
		printk(KERN_INFO "%s> remaint = 0x%lx\n",
			HSADC_DEVICE_NAME, pdx->remaint_len);
#endif
	}

	ret = pdx->total_len - pdx->remaint_len;

acc_abort:
acc_out:
	remove_wait_queue(&pdx->r_wait, &wait);
	__set_current_state(TASK_RUNNING);

#if HSADC_DEBUG
	printk(KERN_INFO "%s> %s end.\n", HSADC_DEVICE_NAME, __FUNCTION__);
#endif

	return ret;
}


int hsadc_ioctl(struct inode *inodep, struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct mxs_hsadc_data *pdx = (struct mxs_hsadc_data*)filp->private_data;
	unsigned long val = 0;

	pdx = NULL;
	val = -1;

#if HSADC_DEBUG
	printk(KERN_INFO "%s> %s.\n", HSADC_DEVICE_NAME, __FUNCTION__);
#endif

	return 0;
}

int hsadc_open(struct inode *inodep, struct file *filp)
{
	struct mxs_hsadc_data *pd = container_of(inodep->i_cdev, struct mxs_hsadc_data, cdev_inst);

	/* lock the device, make the process open the hsadc sequencely */
	down(&pd->sem);

	/* save device instance data in file.private_data */
	filp->private_data = pd; /* pd is dynamic created in hsadc_probe() */
	
	init_hsadc_hw(pd);

#if HSADC_DEBUG
	printk(KERN_INFO "%s> %s.\n", HSADC_DEVICE_NAME, __FUNCTION__);
#endif

	return 0;
}

int hsadc_close(struct inode *inodep, struct file *filp)
{
	struct mxs_hsadc_data *pd = (struct mxs_hsadc_data*)filp->private_data;		

	filp->private_data = NULL;

	/* unlock the device,  process can open hsadc */
	up(&pd->sem);

#if HSADC_DEBUG
	printk(KERN_INFO "%s> %s.\n", HSADC_DEVICE_NAME, __FUNCTION__);
#endif

	return 0;
}


static const struct file_operations hsadc_fops = {
	.owner   = THIS_MODULE,
	.read    = hsadc_read,
	.ioctl   = hsadc_ioctl,
	.open    = hsadc_open,
	.release = hsadc_close,
};

/* mxs-hsadc init driver interface */
int hsadc_init_cdev(struct mxs_hsadc_data* pdx)
{
	int res = 0;
	dev_t devno = 0;

	/* allocate chrdev region dynamical */
	res = alloc_chrdev_region(&devno, 0, 1, HSADC_DEVICE_NAME);

	if(res < 0)
		return res;

	/* init the semaphore as a mutex */
	sema_init(&pdx->sem, 1); /* @== init_MUTEX(&gdevp->sem) */

	init_waitqueue_head(&pdx->r_wait);

	pdx->cdev_major = MAJOR(devno);
	pdx->cdev_index  = 0;
	pdx->cdev_inst.owner = THIS_MODULE;

	/* populate sysfs device entry(diractory) */
    pdx->cdev_class = class_create(THIS_MODULE, HSADC_DEVICE_NAME);

	/* init the cdev kernel object, set the fops for user space interface */
	cdev_init(&pdx->cdev_inst, &hsadc_fops); 

	res = cdev_add(&pdx->cdev_inst, MKDEV(pdx->cdev_major, pdx->cdev_index), 1);
	if(res)
	{
#if HSADC_DEBUG
		printk(KERN_NOTICE "%s> error %d adding hsadc-%d.\n", HSADC_DEVICE_NAME, res, 0);
#endif
		goto fail_cdev_add;
	}

	/* send uevents to udev，the udevd dynamic create device node(inode) in /dev */
	device_create(pdx->cdev_class, NULL, MKDEV(pdx->cdev_major, pdx->cdev_index), NULL, "%s%d", HSADC_DEVICE_NAME, 0);

#if HSADC_DEBUG
	printk(KERN_INFO "%s> initialized.\n", HSADC_DEVICE_NAME);
#endif

	return 0;

fail_cdev_add:
	class_destroy(pdx->cdev_class);

//fail_malloc:
	unregister_chrdev_region(devno, 1);
	return res;
}

/* mxs-hsadc driver unload routine */
void hsadc_cleanup_cdev(struct mxs_hsadc_data* pdx)
{
	device_destroy(pdx->cdev_class, MKDEV(pdx->cdev_major, pdx->cdev_index));
	cdev_del(&pdx->cdev_inst); /* against to cdev_add() */
	class_destroy(pdx->cdev_class);
	unregister_chrdev_region(MKDEV(pdx->cdev_major, pdx->cdev_index), 1); /* release the region */
	
#if HSADC_DEBUG
	printk(KERN_INFO "%s> %s.\n", HSADC_DEVICE_NAME, __FUNCTION__);
#endif

	return;
}

//===============================================================================================================================

static irqreturn_t hsadc_dma_isr(int irq, void * p)
{
	struct mxs_hsadc_data *pd = (struct mxs_hsadc_data *)p;

	mxs_dma_ack_irq(pd->dma_ch);
	mxs_dma_cooked(pd->dma_ch, NULL);

	//dev_dbg(pd->dev, "dma\n");
	wake_up_interruptible(&pd->r_wait);
	return IRQ_HANDLED;
}

static irqreturn_t hsadc_isr(int irq, void * p)
{
	struct mxs_hsadc_data *pd = (struct mxs_hsadc_data *)p;

#if HSADC_DEBUG
	u32 interrupt = readl(pd->hsadc_base + HSADC_CTRL1);
	writel(1<<27, pd->hsadc_base + HSADC_CTRL1_SET);
	writel(1<<26, pd->hsadc_base + HSADC_CTRL1_SET);

	dev_dbg(pd->dev, "irq %08x %s %s %s %s %s \n", interrupt,
		interrupt&(1<<5)?"FIFO_EMPTY":"",
		interrupt&(1<<4)?"END_ONE_SEQ":"",
		interrupt&(1<<3)?"ADC_DONE":"",
		interrupt&(1<<2)?"FIFO_OVERFLOW":"",
		interrupt&(1<<1)?"TIMEOUT":"");
#else
	writel(3<<26, pd->hsadc_base + HSADC_CTRL1_SET);
#endif
	
	return IRQ_HANDLED;
}

void init_sample_percision(struct mxs_hsadc_data *pdx, int percision)
{
	unsigned long val = 0;

	val = readl(pdx->hsadc_base + HSADC_CTRL0);
	val &= ~(HSADC_SAMPLE_PERCISION_MASK);
	
	switch(percision)
	{
	case 8:
		val |= HSADC_SAMPLE_PERCISION_8_BIT;
		break;
	case 10:
		val |= HSADC_SAMPLE_PERCISION_10_BIT;
		break;
	case 12:
		val |= HSADC_SAMPLE_PERCISION_12_BIT;
		break;
	default:
		val |= HSADC_SAMPLE_PERCISION_8_BIT;
		break;	
	}

	writel(val, pdx->hsadc_base + HSADC_CTRL0);
	
	// precharge
	writel(1, pdx->hsadc_base + HSADC_CTRL2_SET);

#if HSADC_DEBUG
	printk(KERN_INFO "%s> hsadc sample data is set to %d-bit mode.\n", HSADC_DEVICE_NAME, sample_percis);
#endif

	return;
}

int init_hsadc_hw(struct mxs_hsadc_data *pd)
{
	//u32 ctrl0 = 0;
#if 0
	/* reset pwm block */
	writel(SFTRST, pd->pwm_base + PWM_CTRL_SET);
	while (!(readl(pd->pwm_base + PWM_CTRL) & CLKGATE));
	writel(SFTRST, pd->pwm_base + PWM_CTRL_CLR);
	writel(CLKGATE, pd->pwm_base + PWM_CTRL_CLR);
	while ((readl(pd->pwm_base + PWM_CTRL) & CLKGATE));
#endif
	writel(3, pd->pwm_base + PWM_CTRL_CLR); //disable PWM channel 0 1

	writel(0|(1<<16), pd->pwm_base + PWM_ACTIVE0);
	/*  PERIOD Register
		26 25 24 23 22-20 19-18 17-16
		0  1  0  0  000   10    11 */
	writel( (0x020B<<16) | 999, pd->pwm_base + PWM_PERIOD0);

	writel(0|((999/2)<<16), pd->pwm_base + PWM_ACTIVE1);
	/*  PERIOD Register
		26 25 24 23 22-20 19-18 17-16
		0  1  0  0  000   10    11 */
	writel( (0x020B<<16) | 999, pd->pwm_base + PWM_PERIOD1);

	writel(3, pd->pwm_base + PWM_CTRL_SET); //enable PWM channel 0 1

	/* Workaround for ENGR116296
		HSADC: Soft reset causes unexpected request to DMA */
	writel(SFTRST, pd->hsadc_base + HSADC_CTRL0_CLR);

	writel((readl(pd->hsadc_base + HSADC_CTRL0) | SFTRST) & (~CLKGATE),\
		pd->hsadc_base + HSADC_CTRL0);
	writel(CLKGATE, pd->hsadc_base + HSADC_CTRL0_SET);
	writel(CLKGATE, pd->hsadc_base + HSADC_CTRL0_CLR);
	writel(CLKGATE, pd->hsadc_base + HSADC_CTRL0_SET);

	/* reset hsadc block */
	writel(SFTRST, pd->hsadc_base + HSADC_CTRL0_CLR);
	writel(CLKGATE, pd->hsadc_base + HSADC_CTRL0_CLR);
	writel(SFTRST, pd->hsadc_base + HSADC_CTRL0_SET);
	while (!(readl(pd->hsadc_base + HSADC_CTRL0) & CLKGATE));
	writel(SFTRST, pd->hsadc_base + HSADC_CTRL0_CLR);
	writel(CLKGATE, pd->hsadc_base + HSADC_CTRL0_CLR);

	writel(1<<13, pd->hsadc_base + HSADC_CTRL2_CLR); //clear power down
	writel(0x7<<1, pd->hsadc_base + HSADC_CTRL2_SET); //input pin is HSADC0

	//ctrl0 = readl(pd->hsadc_base + HSADC_CTRL0);
	//ctrl0 &= ~(0x381FF03F); //triggered by software, 8-bits sample
	//writel(ctrl0, pd->hsadc_base + HSADC_CTRL0);
	init_sample_percision(pd, adc_sample_percision);

	writel(1, pd->hsadc_base + HSADC_CTRL2_SET); //assert adc precharge

	return 0;
}

static int __devinit mxs_hsadc_probe(struct platform_device *pdev)
{
	struct mxs_hsadc_data *pd;
	struct resource *res;
	int rlevel = 0;

	pd = kzalloc(sizeof(*pd), GFP_KERNEL);
	if (pd)
		rlevel++;
	else
		goto quit;

	pd->dev = &pdev->dev;
	platform_set_drvdata(pdev, pd);
	pd->pdev = pdev;
	rlevel++;
	
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL)
		goto quit;
	pd->hsadc_base = ioremap(res->start, res->end - res->start);
	if (pd->hsadc_base)
		rlevel++;
	else
		goto quit;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (res == NULL)
		goto quit;
	pd->pwm_base = ioremap(res->start, res->end - res->start);
	if (pd->pwm_base)
		rlevel++;
	else
		goto quit;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (res)
		pd->dev_irq = res->start;
	else
		goto quit;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 1);
	if (res)
		pd->dma_irq = res->start;
	else
		goto quit;

	res = platform_get_resource(pdev, IORESOURCE_DMA, 0);
	if (res)
		pd->dma_ch = res->start;
	else
		goto quit;

	pd->ref_hsadc_clk = clk_get(NULL, "ref_hsadc");
	if (pd->ref_hsadc_clk)
		rlevel++;
	else
		goto quit;

	pd->hsadc_clk = clk_get(NULL, "hsadc");
	if (pd->hsadc_clk)
		rlevel++;
	else
		goto quit;

	pd->pwm_clk = clk_get(NULL, "pwm");
	if (pd->pwm_clk)
		rlevel++;
	else
		goto quit;

	clk_enable(pd->ref_hsadc_clk);
	clk_enable(pd->hsadc_clk);
	clk_enable(pd->pwm_clk);
	rlevel++;

	clk_set_rate(pd->ref_hsadc_clk, REF_HSADC_FREQ);
	clk_set_rate(pd->hsadc_clk, HSADC_FREQ);

    if (request_irq(pd->dma_irq, hsadc_dma_isr, 0, "hsadc dma", pd))
		goto quit;
	else
		rlevel++;

	if (request_irq(pd->dev_irq, hsadc_isr, 0, "hsadc irq", pd))
		goto quit;
	else
		rlevel++;

	if (mxs_dma_request(pd->dma_ch, pd->dev, "hsadc"))
		goto quit;
	else
		rlevel++;

	mxs_dma_disable(pd->dma_ch);

	pd->desc = mxs_dma_alloc_desc();
	if (pd->desc==NULL)
		goto quit;
	
	memset(&pd->desc->cmd, 0, sizeof(pd->desc->cmd));
	rlevel++;

	pd->buf = dma_alloc_coherent(NULL, DMA_BUF_SIZE, &pd->buf_phy, GFP_KERNEL);
	if(!pd->buf)
		goto quit;
	
	rlevel++;

	if(hsadc_init_cdev(pd))
		goto quit;

#if HSADC_DEBUG
	printk(KERN_INFO "%s> probe successed.\n", HSADC_DEVICE_NAME);
#endif

	return 0;

quit:
	pr_err("%s quit at rlevel %d\n", __func__, rlevel);
	switch (rlevel) {
	case 14:
		hsadc_cleanup_cdev(pd);
	case 13:
		if (pd->buf_phy)
			dma_free_coherent(NULL, DMA_BUF_SIZE, pd->buf, pd->buf_phy);
	case 12:
		if (pd->desc)
			mxs_dma_free_desc(pd->desc);
	case 11:
		mxs_dma_release(pd->dma_ch, pd->dev);
	case 10:
		free_irq(pd->dev_irq, pd);
	case 9:
		free_irq(pd->dma_irq, pd);
	case 8:
		clk_disable(pd->pwm_clk);
		clk_disable(pd->hsadc_clk);
		clk_disable(pd->ref_hsadc_clk);
	case 7:
		clk_put(pd->pwm_clk);
	case 6:
		clk_put(pd->hsadc_clk);
	case 5:
		clk_put(pd->ref_hsadc_clk);
	case 4:
		iounmap(pd->pwm_base);
	case 3:
		iounmap(pd->hsadc_base);
	case 2:
		platform_set_drvdata(pdev, NULL);
	case 1:
		kfree(pd);
	case 0:
	default:
		return -ENODEV;
	}
}

static int __devexit mxs_hsadc_remove(struct platform_device *pdev)
{
	struct mxs_hsadc_data *pd = platform_get_drvdata(pdev);
	u32 ctrl = 0;

	hsadc_cleanup_cdev(pd); // clean cdev interface

	if (pd->buf_phy)
		dma_free_coherent(NULL, DMA_BUF_SIZE, pd->buf, pd->buf_phy);

	if (pd->desc)
		mxs_dma_free_desc(pd->desc);
	
	mxs_dma_enable_irq(pd->dma_ch, 0);
	ctrl = readl(pd->hsadc_base + HSADC_CTRL1);
	ctrl &= ~(1<<31 | 1<<30 | 1<<29); // disable irq
	writel(ctrl, pd->hsadc_base + HSADC_CTRL1);
	mxs_dma_disable(pd->dma_ch);
	
	mxs_dma_release(pd->dma_ch, pd->dev);
	free_irq(pd->dev_irq, pd);
	free_irq(pd->dma_irq, pd);

	clk_disable(pd->hsadc_clk);
	clk_disable(pd->pwm_clk);
	clk_disable(pd->ref_hsadc_clk);

	clk_put(pd->hsadc_clk);
	clk_put(pd->pwm_clk);
	clk_put(pd->ref_hsadc_clk);

	iounmap(pd->pwm_base);
	iounmap(pd->hsadc_base);
	
	platform_set_drvdata(pdev, NULL);
	kfree(pd);

#if HSADC_DEBUG
	printk(KERN_INFO "%s> driver removed.\n", HSADC_DEVICE_NAME);
#endif

	return 0;
}

#ifdef CONFIG_PM
static int
mxs_hsadc_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int mxs_hsadc_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define mxs_hsadc_suspend	NULL
#define	mxs_hsadc_resume	NULL
#endif

static struct platform_driver mxs_hsadc_driver = {
	.probe		= mxs_hsadc_probe,
	.remove		= __exit_p(mxs_hsadc_remove),
	.suspend	= mxs_hsadc_suspend,
	.resume		= mxs_hsadc_resume,
	.driver		= {
		.name   = "mxs-hsadc",
		.owner	= THIS_MODULE,
	},
};

static int __init mx28_hsadc_init(void)
{
	return platform_driver_register(&mxs_hsadc_driver);
}

static void __exit mx28_hsadc_exit(void)
{
	platform_driver_unregister(&mxs_hsadc_driver);
}

MODULE_DESCRIPTION("i.MX28 HSADC driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.9");

module_init(mx28_hsadc_init);
module_exit(mx28_hsadc_exit);
