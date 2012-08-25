/* drivers/misc/vib-omap-pwm.c
 *
 * Copyright (C) 2009 Motorola, Inc.
 * Copyright (C) 2008 HTC Corporation.
 * Copyright (C) 2007 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/err.h>
#include <linux/hrtimer.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/workqueue.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <plat/dmtimer.h>
#include <linux/gpio.h>
#include <mach/vibrator.h>
#include <linux/regulator/consumer.h>

/* TODO: replace with correct header */
#include "../../staging/android/timed_output.h"

#if defined(CONFIG_MACH_LGE_COSMO_REV_A)	
#define GPIO_VIB_EN		109
#else
#define GPIO_VIB_EN		25
#endif

struct regulator *regulator_vib;

struct pwm_vib_data {
	struct timed_output_dev dev;
	struct work_struct vibrator_work;
	struct hrtimer vibe_timer;
	spinlock_t vibe_lock;

	struct pwm_vib_platform_data *pdata;

	int vib_power_state;
	int vibe_state;
};
struct pwm_vib_data *misc_data;
static struct omap_dm_timer *pwm_timer;
static int vibe_timer_state	=	0;

#define MOTOR_RESONANCE_SRC_CLK OMAP_TIMER_SRC_SYS_CLK
#define CLK_COUNT 38400000


#if defined(CONFIG_MACH_LGE_COSMO_REV_A) || defined(CONFIG_MACH_LGE_COSMO_REV_B) || defined(CONFIG_MACH_LGE_COSMO_REV_C)
#define MOTOR_RESONANCE_HZ 175
#else	// LGE LAB4 CH.PARK@LGE.COM 20101227 REV_D 225Hz
#define MOTOR_RESONANCE_HZ 226
#endif
#define MOTOR_RESONANCE_COUTER_VALUE	 (0xFFFFFFFE - ((CLK_COUNT/MOTOR_RESONANCE_HZ)/128))
#define MOTOR_RESONANCE_TIMER_ID 8

spinlock_t vibe__timer_lock;
static void set_gptimer_pwm_vibrator(int on)
{
	unsigned long	flags;

	if (pwm_timer == NULL) {
		pr_err(KERN_ERR "vibrator pwm timer is NULL\n");
		return;
	}

	spin_lock_irqsave(&vibe__timer_lock, flags);
	if (on) {
		if(!vibe_timer_state) {
			gpio_set_value(GPIO_VIB_EN, 1);
			omap_dm_timer_enable(pwm_timer);
			omap_dm_timer_set_match(pwm_timer, 1, 0xFFFFFFFE);
			omap_dm_timer_set_pwm(pwm_timer, 0, 1, OMAP_TIMER_TRIGGER_OVERFLOW_AND_COMPARE);
			omap_dm_timer_set_load_start(pwm_timer, 1, MOTOR_RESONANCE_COUTER_VALUE);
			vibe_timer_state = 1;
		}
	} else {
		if(vibe_timer_state) {
			omap_dm_timer_stop(pwm_timer);
			omap_dm_timer_disable(pwm_timer);
			gpio_set_value(GPIO_VIB_EN, 0);
			vibe_timer_state = 0;
		}
	}
	spin_unlock_irqrestore(&vibe__timer_lock, flags);
}

static void update_vibrator(struct work_struct *work)
{
	set_gptimer_pwm_vibrator(misc_data->vibe_state);
}

static void vibrator_enable(struct timed_output_dev *dev, int value)
{
	struct pwm_vib_data *data =
	    container_of(dev, struct pwm_vib_data, dev);
	unsigned long	flags;

	spin_lock_irqsave(&data->vibe_lock, flags);
	hrtimer_cancel(&data->vibe_timer);

	if (value == 0)
		data->vibe_state	=	0;
	else {
		data->vibe_state	=	1;
		if( value != 3600000 ) value = (value > 15000 ? 15000 : value);
		hrtimer_start(&data->vibe_timer,
			ktime_set(value / 1000, (value % 1000) * 1000000),
			HRTIMER_MODE_REL);
	}

	spin_unlock_irqrestore(&data->vibe_lock, flags);
	schedule_work(&data->vibrator_work);
}

static int vibrator_get_time(struct timed_output_dev *dev)
{
	struct pwm_vib_data *data =
	    container_of(dev, struct pwm_vib_data, dev);
	if (hrtimer_active(&data->vibe_timer)) {
		ktime_t r = hrtimer_get_remaining(&data->vibe_timer);
		return r.tv.sec * 1000 + r.tv.nsec / 1000000;
	} else
		return 0;
}

static enum hrtimer_restart vibrator_timer_func(struct hrtimer *vibe_timer)
{
	struct pwm_vib_data *data =
	    container_of(vibe_timer, struct pwm_vib_data, vibe_timer);

	data->vibe_state = 0;
	schedule_work(&data->vibrator_work);

	return HRTIMER_NORESTART;
}

static int vibrator_probe(struct platform_device *pdev)
{
	struct pwm_vib_platform_data *pdata = pdev->dev.platform_data;
	struct pwm_vib_data *data;
	int ret = 0;

	if (!pdata) {
		ret	=	-EBUSY;
		goto	err0;
	}

	data	=	kzalloc(sizeof(struct pwm_vib_data), GFP_KERNEL);
	if (!data) {
		ret	=	-ENOMEM;
		goto	err0;
	}

	data->pdata	=	pdata;
		
	INIT_WORK(&data->vibrator_work, update_vibrator);

	vibe_timer_state = 0;
	hrtimer_init(&data->vibe_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	data->vibe_timer.function = vibrator_timer_func;
	spin_lock_init(&data->vibe_lock);
	spin_lock_init(&vibe__timer_lock);
	
	data->dev.name		=	"vibrator";
	data->dev.get_time	=	vibrator_get_time;
	data->dev.enable	=	vibrator_enable;
	
	pwm_timer	=	omap_dm_timer_request_specific(MOTOR_RESONANCE_TIMER_ID);
	if (pwm_timer == NULL) {
		pr_err(KERN_ERR "failed to request vibrator pwm timer\n");
		goto	err1;
	}

	/* omap_dm_timer_request_specific enables the timer */
//	dm timer failed to be disabled.
//	omap_dm_timer_disable(pwm_timer);
	omap_dm_timer_set_source(pwm_timer, MOTOR_RESONANCE_SRC_CLK);
//	dm timer should be disabled here.
	omap_dm_timer_disable(pwm_timer);

	ret	=	timed_output_dev_register(&data->dev);
	if (ret < 0)
		goto	err1;

	if (data->pdata->init) {
		ret = data->pdata->init();
		if (ret < 0)
			goto err2;
	}

	gpio_request(GPIO_VIB_EN, "vib_en_gpio");
	gpio_direction_output(GPIO_VIB_EN, 1);
	gpio_set_value(GPIO_VIB_EN, 0);

	misc_data= data;
	platform_set_drvdata(pdev, data);

	vibrator_enable(&data->dev, data->pdata->initial_vibrate);

	pr_info("COSMO vibrator is initialized\n");

	return	0;

err2:
	timed_output_dev_unregister(&data->dev);

err1:
	kfree(data->pdata);
	kfree(data);

err0:
	return	ret;
}

static int vibrator_remove(struct platform_device *pdev)
{
	struct pwm_vib_data *data = platform_get_drvdata(pdev);

	if (data->pdata->exit)
		data->pdata->exit();

	timed_output_dev_unregister(&data->dev);

	kfree(data->pdata);
	kfree(data);

	return	0;
}

/* TO DO: Need to make this drivers own platform data entries */
static struct platform_driver vibrator_omap_pwm_driver = {
	.probe	=	vibrator_probe,
	.remove	=	vibrator_remove,
	.driver	=	{
		.name	=	VIB_PWM_NAME,
		.owner	=	THIS_MODULE,
	},
};

static int __init vibrator_omap_pwm_init(void)
{
	return	platform_driver_register(&vibrator_omap_pwm_driver);
}

static void __exit vibrator_omap_pwm_exit(void)
{
	platform_driver_unregister(&vibrator_omap_pwm_driver);
}

module_init(vibrator_omap_pwm_init);
module_exit(vibrator_omap_pwm_exit);

MODULE_DESCRIPTION("timed output gptimer pwm vibrator device");
MODULE_LICENSE("GPL");

