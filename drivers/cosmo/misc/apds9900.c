/*
 *  apds9900.c - Linux kernel modules for ambient light + proximity sensor
 *
 *  Copyright (C) 2010 Lee Kai Koon <kai-koon.lee@avagotech.com>
 *  Copyright (C) 2010 Avago Technologies
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/wakelock.h>

#include <mach/gpio.h>

#define APDS9900_DRV_NAME	"apds9900"
#define DRIVER_VERSION		"1.0.0"

/*
 * Defines
 */

#define APDS9900_ENABLE_REG	0x00
#define APDS9900_ATIME_REG	0x01
#define APDS9900_PTIME_REG	0x02
#define APDS9900_WTIME_REG	0x03
#define APDS9900_AILTL_REG	0x04
#define APDS9900_AILTH_REG	0x05
#define APDS9900_AIHTL_REG	0x06
#define APDS9900_AIHTH_REG	0x07
#define APDS9900_PILTL_REG	0x08
#define APDS9900_PILTH_REG	0x09
#define APDS9900_PIHTL_REG	0x0A
#define APDS9900_PIHTH_REG	0x0B
#define APDS9900_PERS_REG	0x0C
#define APDS9900_CONFIG_REG	0x0D
#define APDS9900_PPCOUNT_REG	0x0E
#define APDS9900_CONTROL_REG	0x0F
#define APDS9900_REV_REG	0x11
#define APDS9900_ID_REG		0x12
#define APDS9900_STATUS_REG	0x13
#define APDS9900_CDATAL_REG	0x14
#define APDS9900_CDATAH_REG	0x15
#define APDS9900_IRDATAL_REG	0x16
#define APDS9900_IRDATAH_REG	0x17
#define APDS9900_PDATAL_REG	0x18
#define APDS9900_PDATAH_REG	0x19

#define CMD_BYTE	0x80
#define CMD_WORD	0xA0
#define CMD_SPECIAL	0xE0

#define CMD_CLR_PS_INT	0xE5
#define CMD_CLR_ALS_INT	0xE6
#define CMD_CLR_PS_ALS_INT	0xE7

#define APDS_9900_IO						   0x99
#define APDS_9900_IOCTL_ACTIVE           _IOW(APDS_9900_IO, 0x01, int)

#define APDS9900_ENABLE_PIEN 	 0x20
#define APDS9900_ENABLE_AIEN 	 0x10
#define APDS9900_ENABLE_WEN 	 0x08 
#define APDS9900_ENABLE_PEN 	 0x04 
#define APDS9900_ENABLE_AEN 	 0x02
#define APDS9900_ENABLE_PON 	 0x01


#define ATIME 	 	0xf6 	// 27.2ms . minimum ALS integration time
#define WTIME 		0xf6 	// 27.2ms . minimum Wait time
#define PTIME 	 	0xff 	// 2.72ms . minimum Prox integration time
#define INT_PERS 	0x33

#if defined(CONFIG_MACH_LGE_COSMO_DOMASTIC)
//SU760_Rev_B,A
#define PPCOUNT 	8
#elif  !defined(CONFIG_MACH_LGE_COSMO_EVB_C) && !defined(CONFIG_MACH_LGE_COSMO_REV_A) && !defined(CONFIG_MACH_LGE_COSMO_REV_B) && !defined(CONFIG_MACH_LGE_COSMO_REV_C)
//Rev_D
#define PPCOUNT 	6 // 20110219 HW requirement <== 12    // 20101228 HW requirement<= 7 // 20101228 HW requirement<= 4 // Minimum prox pulse count
#else
//Rev_C
#define PPCOUNT 	7 // 20101228 HW requirement<= 4 // Minimum prox pulse count
#endif
#define PDRIVE 	 	0 		//100mA of LED Power
#define PDIODE 	 	0x20 	// IR Diode
#define PGAIN 	 	0x00 	//1x Prox gain
#define AGAIN 		0x00 	//1x ALS gain

#if defined(CONFIG_MACH_LGE_COSMO_DOMASTIC) && defined(CONFIG_MACH_LGE_COSMO_DOMASTIC_REV_B) //LGE_CHANGE - 20110216 jinsu.park H/W request for su760 hw_rev_b
#define COSMO_APDS_PROXIITY_HIGH_THRESHHOLD	600
#define COSMO_APDS_PROXIITY_LOW_THRESHHOLD	500
#else
#define COSMO_APDS_PROXIITY_HIGH_THRESHHOLD	480
#define COSMO_APDS_PROXIITY_LOW_THRESHHOLD	450
#endif

#define COSMO_APDS_COEFFICIENT_B	202 //2.02*100
#define COSMO_APDS_COEFFICIENT_C	67 //0.67*100
#define COSMO_APDS_COEFFICIENT_D	128 //1.28*100

#define COSMO_APDS_GA 173 //215 // org_value = 1.83 -> *100 
#define COSMO_APDS_DF 52

#define APDS9900_STATUS_PINT	0x20
#define APDS9900_STATUS_AINT	0x10


#define APDS900_SENSOR_DEBUG 1
 #if APDS900_SENSOR_DEBUG
 #define DEBUG_MSG(args...)  printk(args)
 #else
 #define DEBUG_MSG(args...)
 #endif

//#define APDS900_SCHEDULE_FOR_DEBUG 

//#define APDS9900_POWERONOFF
#define APDS9900_WAKE_LOCK

/*
 * Structs
 */

struct apds9900_data {
	struct i2c_client *client;

	unsigned int enable;
	unsigned int atime;
	unsigned int ptime;
	unsigned int wtime;
	unsigned int ailt;
	unsigned int aiht;
	unsigned int pilt;
	unsigned int piht;
	unsigned int pers;
	unsigned int config;
	unsigned int ppcount;
	unsigned int control;

	unsigned int lth_prox;
	unsigned int hth_prox;
	// kk 20 Apr 2011
	unsigned int ps_detection;		/* 0 = far-to-near;	1 = near-to-far */

	unsigned int GA;
	unsigned int DF;
	unsigned int LPC;
	
	int irq;
	unsigned int poll_delay;	/* kk 02 May - temporary only for sensor check */

	struct work_struct irq_work;
	struct workqueue_struct	 *irq_wq;
#ifdef APDS900_SCHEDULE_FOR_DEBUG	
	struct delayed_work    poll_dwork; /* kk 02 May - temporary only for sensor check */
#endif
	struct input_dev *input_dev;
#ifdef APDS9900_WAKE_LOCK
	struct wake_lock apds9900_wake_lock;
#endif
};

/*
 * Global data
 */
static struct i2c_client *apds_9900_i2c_client = NULL;
static atomic_t dev_open_count;
static int apds_9900_initlizatied = 0;
static int enable_status = 0;
static int enable_ALS_status = 0;
static int enable_PS_status = 0;	/* kk 19 May 2011 */
static int apds9900_proxidata = 0;
static int apds9900_luxdata = 0;

static int apds9900_oldProximity = 0;

/*
 * Management functions
 */

static int apds9900_set_command(struct i2c_client *client, int command)
{
	struct apds9900_data *data = i2c_get_clientdata(client);
	int ret;
	int clearInt;

	if (command == 0)
		clearInt = CMD_CLR_PS_INT;
	else if (command == 1)
		clearInt = CMD_CLR_ALS_INT;
	else
		clearInt = CMD_CLR_PS_ALS_INT;
		
	ret = i2c_smbus_write_byte(client, clearInt);

	return ret;
}

static int apds9900_set_enable(struct i2c_client *client, int enable)
{
	struct apds9900_data *data = i2c_get_clientdata(client);
	int ret = 0;
	
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS9900_ENABLE_REG, enable);

	DEBUG_MSG("apds9900_set_enable = [%x][%d] \n",enable, ret);
	
//	data->enable = enable;

	return ret;
}

static int apds9900_set_atime(struct i2c_client *client, int atime)
{
	struct apds9900_data *data = i2c_get_clientdata(client);
	int ret = 0;
	
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS9900_ATIME_REG, atime);

	data->atime = atime;

	return ret;
}
static int apds9900_set_ptime(struct i2c_client *client, int ptime)
{
	struct apds9900_data *data = i2c_get_clientdata(client);
	int ret = 0;
	
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS9900_PTIME_REG, ptime);

	data->ptime = ptime;

	return ret;
}

static int apds9900_set_wtime(struct i2c_client *client, int wtime)
{
	struct apds9900_data *data = i2c_get_clientdata(client);
	int ret = 0;
	
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS9900_WTIME_REG, wtime);

	data->wtime = wtime;

	return ret;
}

static int apds9900_set_ailt(struct i2c_client *client, int threshold)
{
	struct apds9900_data *data = i2c_get_clientdata(client);
	int ret = 0;
	
	ret = i2c_smbus_write_word_data(client, CMD_WORD|APDS9900_AILTL_REG, threshold);
	
	data->ailt = threshold;

	return ret;
}

static int apds9900_set_aiht(struct i2c_client *client, int threshold)
{
	struct apds9900_data *data = i2c_get_clientdata(client);
	int ret = 0;
	
	ret = i2c_smbus_write_word_data(client, CMD_WORD|APDS9900_AIHTL_REG, threshold);
	
	data->aiht = threshold;

	return ret;
}

static int apds9900_set_pilt(struct i2c_client *client, int threshold)
{
	struct apds9900_data *data = i2c_get_clientdata(client);
	int ret = 0;
	
	ret = i2c_smbus_write_word_data(client, CMD_WORD|APDS9900_PILTL_REG, threshold);
	
	data->pilt = threshold;	// kk 29 Apr 2011

	return ret;
}

static int apds9900_set_piht(struct i2c_client *client, int threshold)
{
	struct apds9900_data *data = i2c_get_clientdata(client);
	int ret = 0;
	
	ret = i2c_smbus_write_word_data(client, CMD_WORD|APDS9900_PIHTL_REG, threshold);
	
	data->piht = threshold;	// kk 29 Apr 2011

	return ret;
}

static int apds9900_set_pers(struct i2c_client *client, int pers)
{
	struct apds9900_data *data = i2c_get_clientdata(client);
	int ret = 0;
	
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS9900_PERS_REG, pers);

	data->pers = pers;

	return ret;
}

static int apds9900_set_config(struct i2c_client *client, int config)
{
	struct apds9900_data *data = i2c_get_clientdata(client);
	int ret = 0;
	
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS9900_CONFIG_REG, config);

	data->config = config;

	return ret;
}

static int apds9900_set_ppcount(struct i2c_client *client, int ppcount)
{
	struct apds9900_data *data = i2c_get_clientdata(client);
	int ret;
	
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS9900_PPCOUNT_REG, ppcount);

	data->ppcount = ppcount;

	return ret;
}

static int apds9900_set_control(struct i2c_client *client, int control)
{
	struct apds9900_data *data = i2c_get_clientdata(client);
	int ret;
	
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS9900_CONTROL_REG, control);

	data->control = control;

	return ret;
}


static unsigned int apds9900_get_LPC()
{
	struct apds9900_data *data = i2c_get_clientdata(apds_9900_i2c_client);

	if(data != NULL)
	{
		/* Assume
		
		GA 		= 	1.8395	// Glass Attenuation Factor (Open air - 0.46)
		DF		=	52		// Device Factor (Open air - 52)
		ALSIT 	=	27.2 		// ALS Integration Time  ALSIT = (256-ATIME)*2.72
		AGAIN 	= 	1		// ALS Gain
		LPC 		=     GA * DF / (ALSIT * AGAIN) // Lux per count
				=     1.8395*52 / (27.2*8)
				= 	0.4395
		
		
		*/
		unsigned int ALSIT = (256-data->atime)*272; // org *100;
		unsigned int LPC;

		LPC = (data->GA * data->DF * 100) / ALSIT; // AGAIN = 1 , LPC = org *100

		return LPC;
		
	}
	else
		return 0;
}

/* Set some defalut configuration to APDS-9900 */
static void apds_9900_init(void)
{
	struct apds9900_data *data = i2c_get_clientdata(apds_9900_i2c_client);

	/*
		ATIME = 0xFF; // 27.2ms . minimum ALS integration time
		WTIME = 0xf6; // 27.2ms . minimum Wait time
		PTIME = 0xff; // 2.72ms . minimum Prox integration time
	*/
	apds9900_set_atime(apds_9900_i2c_client, ATIME);
	apds9900_set_wtime(apds_9900_i2c_client, WTIME);
	apds9900_set_ptime(apds_9900_i2c_client, PTIME);

	apds9900_set_pers(apds_9900_i2c_client, 0x66); //Interrupt persistence = 0x33 -> Proximity(4) | ALS(4)

	apds9900_set_config(apds_9900_i2c_client, 0x00); // Wait long timer <- no needs so set 0
		
	apds9900_set_ppcount(apds_9900_i2c_client, PPCOUNT); // Pulse count for proximity
	
	apds9900_set_control(apds_9900_i2c_client, PDRIVE | PDIODE | PGAIN | AGAIN);

	// kk 29 Apr 2011
	apds9900_set_pilt(apds_9900_i2c_client, 1023); // to ensure first FAR interrupt
	apds9900_set_piht(apds_9900_i2c_client, 0);

	apds9900_set_ailt(apds_9900_i2c_client, 0xFFFF); // init threshold for als
	apds9900_set_aiht(apds_9900_i2c_client, 0);	

	data->lth_prox = COSMO_APDS_PROXIITY_LOW_THRESHHOLD;
	data->hth_prox = COSMO_APDS_PROXIITY_HIGH_THRESHHOLD;

	// kk 20 Apr 2011 - important to init as FAR position = no detect
	data->ps_detection = 1;	/* 0 = far-to-near;	1 = near-to-far */

	data->GA = COSMO_APDS_GA;
	data->DF = COSMO_APDS_DF;

	data->LPC = apds9900_get_LPC();

	apds9900_oldProximity = 2;


	// kk 25 Apr 2011
	printk("apds_9900_init DONE!\n");
}


/*
 * SysFS support
 */

static ssize_t apds9900_show_atime(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct apds9900_data *data = i2c_get_clientdata(apds_9900_i2c_client);

	if(apds_9900_i2c_client != NULL)
		return sprintf(buf, "%d\n",data->atime);
	else
		return -1;
}

static ssize_t apds9900_store_atime(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);
	unsigned long rdata = simple_strtoul(buf, NULL, 10);	

	if(apds_9900_i2c_client != NULL)
	{
		apds9900_set_atime(apds_9900_i2c_client,rdata);
		data->LPC = apds9900_get_LPC();
	}
	else
		return -1;

	return count;
}

static DEVICE_ATTR(atime, 0664, apds9900_show_atime, apds9900_store_atime);	

static ssize_t apds9900_show_ptime(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct apds9900_data *data = i2c_get_clientdata(apds_9900_i2c_client);

	if(apds_9900_i2c_client != NULL)
		return sprintf(buf, "%d\n",data->ptime);
	else
		return -1;
}

static ssize_t apds9900_store_ptime(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);
	unsigned long rdata = simple_strtoul(buf, NULL, 10);	

	if(apds_9900_i2c_client != NULL)
		apds9900_set_ptime(apds_9900_i2c_client,rdata);
	else
		return -1;

	return count;
}

static DEVICE_ATTR(ptime, 0664, apds9900_show_ptime, apds9900_store_ptime);	

static ssize_t apds9900_show_wtime(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct apds9900_data *data = i2c_get_clientdata(apds_9900_i2c_client);

	if(apds_9900_i2c_client != NULL)
		return sprintf(buf, "%d\n",data->wtime);
	else
		return -1;
}

static ssize_t apds9900_store_wtime(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);
	unsigned long rdata = simple_strtoul(buf, NULL, 10);	

	if(apds_9900_i2c_client != NULL)
		apds9900_set_wtime(apds_9900_i2c_client,rdata);
	else
		return -1;

	return count;
}

static DEVICE_ATTR(wtime, 0664, apds9900_show_wtime, apds9900_store_wtime);	

static ssize_t apds9900_show_ppcount(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct apds9900_data *data = i2c_get_clientdata(apds_9900_i2c_client);

	if(apds_9900_i2c_client != NULL)
		return sprintf(buf, "%d\n",data->ppcount);
	else
		return -1;
}

static ssize_t apds9900_store_ppcount(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);
	unsigned long rdata = simple_strtoul(buf, NULL, 10);	

	if(apds_9900_i2c_client != NULL)
		apds9900_set_ppcount(apds_9900_i2c_client,rdata);
	else
		return -1;

	return count;
}

static DEVICE_ATTR(ppcount, 0664, apds9900_show_ppcount, apds9900_store_ppcount);	

static ssize_t apds9900_show_pers(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct apds9900_data *data = i2c_get_clientdata(apds_9900_i2c_client);

	if(apds_9900_i2c_client != NULL)
		return sprintf(buf, "%d\n",data->pers);
	else
		return -1;
}

static ssize_t apds9900_store_pers(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);
	unsigned long rdata = simple_strtoul(buf, NULL, 10);	

	if(apds_9900_i2c_client != NULL)
		apds9900_set_pers(apds_9900_i2c_client,rdata);
	else
		return -1;

	return count;
}

static DEVICE_ATTR(pers, 0664, apds9900_show_pers, apds9900_store_pers);

static ssize_t apds9900_show_pilt(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct apds9900_data *data = i2c_get_clientdata(apds_9900_i2c_client);

	if(apds_9900_i2c_client != NULL)
		return sprintf(buf, "%d\n",data->lth_prox);
	else
		return -1;
}

static ssize_t apds9900_store_pilt(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);
	unsigned long rdata = simple_strtoul(buf, NULL, 10);	

	if(apds_9900_i2c_client != NULL)
	{
		data->lth_prox = rdata;
	}
	else
		return -1;

	return count;
}

static DEVICE_ATTR(pilt, 0664, apds9900_show_pilt, apds9900_store_pilt);	


static ssize_t apds9900_show_piht(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct apds9900_data *data = i2c_get_clientdata(apds_9900_i2c_client);

	if(apds_9900_i2c_client != NULL)
		return sprintf(buf, "%d\n",data->hth_prox);
	else
		return -1;
}

static ssize_t apds9900_store_piht(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);
	unsigned long rdata = simple_strtoul(buf, NULL, 10);	

	if(apds_9900_i2c_client != NULL)
	{
		data->hth_prox = rdata;
	}
	else
		return -1;

	return count;
}

static DEVICE_ATTR(piht, 0664, apds9900_show_piht, apds9900_store_piht);	


static ssize_t apds9900_show_pdata(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct apds9900_data *data = i2c_get_clientdata(apds_9900_i2c_client);

	if(apds_9900_i2c_client != NULL)
	{
		int pdata = i2c_smbus_read_word_data(apds_9900_i2c_client, CMD_WORD|APDS9900_PDATAL_REG);		
	
		return sprintf(buf, "%d\n",pdata);
	}
	else
	{
		return -1;
	}
}

static DEVICE_ATTR(pdata, 0664, apds9900_show_pdata, NULL);	


static ssize_t apds9900_show_GA(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct apds9900_data *data = i2c_get_clientdata(apds_9900_i2c_client);

	if(apds_9900_i2c_client != NULL)
		return sprintf(buf, "%d\n",data->GA);
	else
		return -1;
}

static ssize_t apds9900_store_GA(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);
	unsigned long rdata = simple_strtoul(buf, NULL, 10);	

	if(apds_9900_i2c_client != NULL)
	{
		data->GA = rdata;
		data->LPC = apds9900_get_LPC();
	}
	else
		return -1;

	return count;
}

static DEVICE_ATTR(GA, 0664, apds9900_show_GA, apds9900_store_GA);	

static ssize_t apds9900_show_DF(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct apds9900_data *data = i2c_get_clientdata(apds_9900_i2c_client);

	if(apds_9900_i2c_client != NULL)
		return sprintf(buf, "%d\n",data->DF);
	else
		return -1;
}

static ssize_t apds9900_store_DF(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);
	unsigned long rdata = simple_strtoul(buf, NULL, 10);	

	if(apds_9900_i2c_client != NULL)
	{
		data->DF = rdata;
		data->LPC = apds9900_get_LPC();
	}
	else
		return -1;

	return count;
}

static DEVICE_ATTR(DF, 0664, apds9900_show_DF, apds9900_store_DF);	

static ssize_t apds9900_show_interrupt(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n",enable_status);
}

static ssize_t apds9900_store_interrupt(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
// this value should be same with the value in sensors.cpp
#define STORE_INTERUPT_SELECT_PROXIMITY		0x02
#define STORE_INTERUPT_SELECT_ALS			0x04


	struct i2c_client *client = to_i2c_client(dev);
	struct apds9900_data *data = i2c_get_clientdata(client);
	unsigned long rdata = simple_strtoul(buf, NULL, 10);	
	int enable = (int)rdata;
	int val = 0;
	int ret;

	// kk 25 Apr 2011
	printk("apds9900_store_interrupt = [%d] apds_9900_initlizatied [%d] \n",rdata, apds_9900_initlizatied);

	if(enable_status == 0 && (rdata == STORE_INTERUPT_SELECT_PROXIMITY || rdata == STORE_INTERUPT_SELECT_ALS))
	{
		DEBUG_MSG("apds9900_store_interrupt ignore\n");
		return count;
	}

	// if first enter , initializing
	if(!apds_9900_initlizatied){
		apds_9900_init();
		apds_9900_initlizatied = 1;		
		enable_irq(data->irq);

		/* kk 02 May - temporary only for check sensor status */
		/*
		 * If work is already scheduled then subsequent schedules will not
		 * change the scheduled time that's why we have to cancel it first.
		 */
#ifdef APDS900_SCHEDULE_FOR_DEBUG		 
		__cancel_delayed_work(&data->poll_dwork);
		schedule_delayed_work(&data->poll_dwork, msecs_to_jiffies(data->poll_delay));
#endif
	}

	disable_irq(data->irq);

	// check proximity enable flag
	if(enable & STORE_INTERUPT_SELECT_PROXIMITY)
	{	
		if(enable & 0x01) // enable
		{
			data->enable |= (APDS9900_ENABLE_PIEN|APDS9900_ENABLE_PEN|APDS9900_ENABLE_PON); 	
			data->enable |= (APDS9900_ENABLE_AIEN|APDS9900_ENABLE_AEN|APDS9900_ENABLE_PON);

			/* kk 19 May 2011 */
			// change ALS threshold here to detect sunlight only
			enable_PS_status = 1;

			if(enable_ALS_status == 0)
			{
				apds9900_set_ailt(apds_9900_i2c_client, 0);
				apds9900_set_aiht(apds_9900_i2c_client, (75*(1024*(256-data->atime)))/100);
			}
			else
			{
				apds9900_set_ailt(apds_9900_i2c_client, 0xFFFF);
				apds9900_set_aiht(apds_9900_i2c_client, 0);	
			}
		}
		else		//disable
		{
			enable_PS_status = 0;	// kk 19 May 2011 

			if(enable_ALS_status == 1)
			{
				data->enable &= ~(APDS9900_ENABLE_PIEN|APDS9900_ENABLE_PEN); 

				/* kk 19 May 2011 */
				// restore to normal ALS condition
				apds9900_set_ailt(apds_9900_i2c_client, 0xFFFF);
				apds9900_set_aiht(apds_9900_i2c_client, 0);	
			}
			else
			{
				data->enable = 1;
			}
		}
	}

	// check ALS enable flag
	if(enable & STORE_INTERUPT_SELECT_ALS)
	{	
		if(enable & 0x01) // enable
		{
			enable_ALS_status = 1;
			data->enable |= (APDS9900_ENABLE_AIEN|APDS9900_ENABLE_AEN|APDS9900_ENABLE_PON);
			apds9900_set_ailt(apds_9900_i2c_client, 0xFFFF); // init threshold for als
			apds9900_set_aiht(apds_9900_i2c_client, 0);					
		}
		else
		{
			enable_ALS_status = 0;

			if(enable_PS_status == 0)
			{
				data->enable &= ~(APDS9900_ENABLE_AIEN|APDS9900_ENABLE_AEN);			
				apds9900_set_ailt(apds_9900_i2c_client, 0xFFFF); // init threshold for als
				apds9900_set_aiht(apds_9900_i2c_client, 0);
			}
			else
			{
				apds9900_set_ailt(apds_9900_i2c_client, 0);
				apds9900_set_aiht(apds_9900_i2c_client, (75*(1024*(256-data->atime)))/100); 
			}
		}
	}

	// toggle irq enable
	/*
	if(data->enable & APDS9900_ENABLE_PON) // previous status : some sensor is alive
	{   		
		if((val & APDS9900_ENABLE_PON) == 0 ) // next status : whole sensors is deactived
			disable_irq(data->irq);
	}
	else  // previous status : whole sensors is deactived
	{
		if( val & APDS9900_ENABLE_PON)// next status : some sensor is alive
			enable_irq(data->irq);
	}	
       */

	if(data->enable == 1)
	{
		data->enable = 0;
		#ifdef APDS9900_POWERONOFF
		apds_9900_initlizatied = 0;
		#endif

#ifdef APDS900_SCHEDULE_FOR_DEBUG
		/* kk 02 May - temporary only for check sensor status */
		__cancel_delayed_work(&data->poll_dwork);
#endif
	}

	ret = apds9900_set_enable(client, data->enable);

	enable_status = data->enable;

#ifdef APDS9900_POWERONOFF
	if(enable_status != 0)
#endif		
	{
		enable_irq(data->irq);
	}

	// kk 25 Apr 2011
	printk("apds9900_store_interrupt enable_status = [%x] data->enable [%x] \n",enable_status, data->enable);

	if (ret < 0)
		return ret;
	
	return count;
}

static DEVICE_ATTR(interrupt, 0664,
		   apds9900_show_interrupt, apds9900_store_interrupt);		   


static ssize_t apds9900_show_proxidata(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n",apds9900_proxidata);
}


static DEVICE_ATTR(proxidata, 0664,
		   apds9900_show_proxidata, NULL);	


static ssize_t apds9900_show_luxdata(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n",apds9900_luxdata);
}

static DEVICE_ATTR(luxdata, 0664,
		   apds9900_show_luxdata, NULL);	



static struct attribute *apds9900_attributes[] = {
	&dev_attr_atime.attr,
	&dev_attr_ptime.attr,
	&dev_attr_wtime.attr,
	&dev_attr_ppcount.attr,
	&dev_attr_pers.attr,
	&dev_attr_pilt.attr,
	&dev_attr_piht.attr,
	&dev_attr_GA.attr,
	&dev_attr_DF.attr,
	&dev_attr_pdata.attr,
	&dev_attr_interrupt.attr,
	&dev_attr_proxidata.attr,
	&dev_attr_luxdata.attr,
	NULL
};

static const struct attribute_group apds9900_attr_group = {
	.attrs = apds9900_attributes,
};

/*
 * Initialization function
 */

static int apds9900_init_client(struct i2c_client *client)
{
	struct apds9900_data *data = i2c_get_clientdata(client);
	int err;


	apds9900_set_enable(client, 0);

	mdelay(100);

	err = i2c_smbus_read_byte_data(client, APDS9900_ENABLE_REG);

	if (err != 0)
		return -ENODEV;

	DEBUG_MSG("apds9900_init_client\n");

	data->enable = 0;

	return 0;
}

/* kk 02 May - temporary only for sensor check */
/* Polling routine */
#ifdef APDS900_SCHEDULE_FOR_DEBUG
static void apds9900_polling_work_handler(struct work_struct *work)
{
	struct apds9900_data *data = container_of(work, struct apds9900_data, poll_dwork.work);
	int cdata, irdata, pdata, status, pers;
	
	status = i2c_smbus_read_byte_data(apds_9900_i2c_client, CMD_BYTE|APDS9900_STATUS_REG);
	pers = i2c_smbus_read_byte_data(apds_9900_i2c_client, CMD_BYTE|APDS9900_PERS_REG);
	cdata = i2c_smbus_read_word_data(apds_9900_i2c_client, CMD_WORD|APDS9900_CDATAL_REG);
	irdata = i2c_smbus_read_word_data(apds_9900_i2c_client, CMD_WORD|APDS9900_IRDATAL_REG);
	pdata = i2c_smbus_read_word_data(apds_9900_i2c_client, CMD_WORD|APDS9900_PDATAL_REG);
		
	printk("9900_poll status=%x, pers=%x, cdata=%x, irdata=%x, pdata=%x\n", status, pers, cdata, irdata, pdata);

	schedule_delayed_work(&data->poll_dwork, msecs_to_jiffies(data->poll_delay));	// restart timer
}
#endif

void apds_9900_proximity_handler(struct apds9900_data *data, int cData) 	
{
	int rdata = i2c_smbus_read_word_data(apds_9900_i2c_client, CMD_WORD|APDS9900_PDATAL_REG);		
	
	// kk 25 Apr 2011
	printk("prox sensor report data = [%d][%d][%d]\n",rdata, cData, apds9900_oldProximity);

#ifdef APDS9900_WAKE_LOCK
	if(wake_lock_active(&(data->apds9900_wake_lock)))
	{
		wake_unlock(&(data->apds9900_wake_lock));
	}

	wake_lock_timeout(&(data->apds9900_wake_lock), 2*HZ);	
#endif

	apds9900_proxidata = rdata;
	
// kk 29 Apr 2011	if(rdata > data->hth_prox && (cData < (75*(1024*(256-data->atime)))/100))
	if ( (rdata > data->pilt) && (rdata >= data->piht) && (cData < (75*(1024*(256-data->atime)))/100))
	{

		apds9900_set_enable(apds_9900_i2c_client,0);

		//if(apds9900_oldProximity != 0)
		{
			input_report_abs(data->input_dev, ABS_DISTANCE, 0);/* NEAR */	
			input_sync(data->input_dev);
			apds9900_oldProximity = 0;
		}
		// kk 25 Apr 2011
		apds9900_set_pilt(apds_9900_i2c_client, data->lth_prox);
		apds9900_set_piht(apds_9900_i2c_client, 1023);	
		
		// kk 20 Apr 2011
		data->ps_detection = 0;	/* 0 = far-to-near;	1 = near-to-far */

		printk("apds_9900_proximity_handler = NEAR\n");	
	}
// kk 29 Apr 2011	else if(rdata < data->lth_prox) 
	else if ( (rdata <= data->pilt) && (rdata < data->piht) )
	{
		apds9900_set_enable(apds_9900_i2c_client,0);

		if(apds9900_oldProximity != 1)
		{
			input_report_abs(data->input_dev, ABS_DISTANCE, 1);/* FAR */	
			input_sync(data->input_dev);
			apds9900_oldProximity = 1;
		}
		apds9900_set_pilt(apds_9900_i2c_client, 0);
		apds9900_set_piht(apds_9900_i2c_client, data->hth_prox);

		// kk 20 Apr 2011
		data->ps_detection = 1;	/* 0 = far-to-near;	1 = near-to-far */

		printk("apds_9900_proximity_handler = FAR 2 \n");	
	}
	else if ( (rdata <= data->pilt) || (rdata >= data->piht) )
	{
		apds9900_set_enable(apds_9900_i2c_client,0);

		if(apds9900_oldProximity != 1)
		{
			input_report_abs(data->input_dev, ABS_DISTANCE, 1);/* FAR */	
			input_sync(data->input_dev);
			apds9900_oldProximity = 1;
		}
		apds9900_set_pilt(apds_9900_i2c_client, 0);
		apds9900_set_piht(apds_9900_i2c_client, data->hth_prox);

		// kk 20 Apr 2011
		data->ps_detection = 1;	/* 0 = far-to-near;	1 = near-to-far */

		printk("apds_9900_proximity_handler = FAR 3\n");	
	}
	else 
	{
		printk("apds_9900_proximity_handler no conditions pilt=%x, piht=%x, pdata=%x, cdata=%x\n", data->pilt, data->piht, rdata, cData);	
	}

}

void apds_9900_als_handler(struct apds9900_data *data)
{		
	int i = 0;

	int LTH = 0;
	int HTH = 0;
	int TH_GAP = 0;

	int APDS_IAC1 = 0;
	int APDS_IAC2 = 0;
	int APDS_IAC = 0;	
	int LUX;	

	// read reg for als
	int cdata = i2c_smbus_read_word_data(apds_9900_i2c_client, CMD_WORD|APDS9900_CDATAL_REG);
	int irdata = i2c_smbus_read_word_data(apds_9900_i2c_client, CMD_WORD|APDS9900_IRDATAL_REG);
	// kk 25 Apr 2011
	int pdata = i2c_smbus_read_word_data(apds_9900_i2c_client, CMD_WORD|APDS9900_PDATAL_REG);

	// calculation LUX , please check datasheet from AVAGO
	APDS_IAC1 = cdata - (COSMO_APDS_COEFFICIENT_B*irdata)/100;
	APDS_IAC2 = (COSMO_APDS_COEFFICIENT_C*cdata - COSMO_APDS_COEFFICIENT_D*irdata)/100;
    APDS_IAC = APDS_IAC1>APDS_IAC2?APDS_IAC1:APDS_IAC2;
	LUX = (APDS_IAC*data->LPC)/100;	

	// kk 20 Apr 2011 - check PS under sunlight
	if ( (data->ps_detection == 0) && (cdata > (75*(1024*(256-data->atime)))/100))	// PS was previously in far-to-near condition
	{
		// need to inform input event as there will be no interrupt from the PS
		if(apds9900_oldProximity != 1)
		{
			input_report_abs(data->input_dev, ABS_DISTANCE, 1);/* FAR */	
			input_sync(data->input_dev);
			apds9900_oldProximity = 1;
		}
		apds9900_set_pilt(apds_9900_i2c_client, 0);
		apds9900_set_piht(apds_9900_i2c_client, data->hth_prox);

		// kk 20 Apr 2011
		data->ps_detection = 1;	/* 0 = far-to-near;	1 = near-to-far */

		printk("apds_9900_proximity_handler = FAR 4\n");	
	}

	//report vlaue to sensors.cpp

	//set min and max value
	LUX = LUX>10 ? LUX : 10;
	LUX = LUX<10240 ? LUX : 10240;

	apds9900_luxdata = LUX;

	if(enable_ALS_status == 1)
	{
		input_report_abs(data->input_dev, ABS_MISC, LUX);/* x-axis raw acceleration */
		input_sync(data->input_dev);
	}

	apds9900_set_enable(apds_9900_i2c_client,0);

	if (enable_PS_status == 1) { /* kk 19 May 2011 don't change ALS threshold in PS function */
		printk("light sensor report lux =%d cdata = %d irdata = %d, pdata= %d\n",LUX,cdata,irdata, pdata);
		return;
	}
	
	// set threshold for als
	TH_GAP = cdata / 10;
	LTH = cdata  - TH_GAP;
	LTH = LTH > 0 ? LTH : 0;
	HTH = cdata + TH_GAP;
	HTH = HTH < 0xFFFF ? HTH : 0xFFFF;

	apds9900_set_ailt(apds_9900_i2c_client, LTH);
	apds9900_set_aiht(apds_9900_i2c_client, HTH);

	// kk 25 Apr 2011
	printk("light sensor report lux =%d cdata = %d irdata = %d, pdata= %d\n",LUX,cdata,irdata, pdata);
}


void apds_9900_irq_work_func(struct work_struct *work) 	
{
   	struct apds9900_data *data = i2c_get_clientdata(apds_9900_i2c_client);	 

	int status, rdata;
	int org_enable = data->enable;

	status = i2c_smbus_read_byte_data(apds_9900_i2c_client, CMD_BYTE|APDS9900_STATUS_REG);

	// kk 25 Apr 2011
	printk("apds_9900_irq_work_func status =[%x][%d]\n", status, data->ps_detection);

	if(status & APDS9900_STATUS_PINT) 
	{
	
		int cdata = i2c_smbus_read_word_data(apds_9900_i2c_client, CMD_WORD|APDS9900_CDATAL_REG);
		
		//if (cdata < (75*(1024*(256-data->atime)))/100)
		{
			apds_9900_proximity_handler(data, cdata);
		}

		// DEBUG_MSG("Triggered by background ambient noise [%d]\n", cdata);
	}

	if(status & APDS9900_STATUS_AINT)	
	{
		apds_9900_als_handler(data);			
	}

	//DEBUG_MSG("apds_9900_irq_work_func status = %d\n",status);

	
	/*
	// sensor status check and enable irq
	if(data->enable & APDS9900_ENABLE_PON)
	{
		enable_irq(data->irq);
	}
	*/

	// ACK about interupt handling
	if(status & APDS9900_STATUS_PINT || data->enable & APDS9900_ENABLE_PIEN)
	{
		if(status & APDS9900_STATUS_AINT || data->enable & APDS9900_ENABLE_AIEN)
			apds9900_set_command(apds_9900_i2c_client,2);
		else
			apds9900_set_command(apds_9900_i2c_client,0);
	}
	else if(status & APDS9900_STATUS_AINT)
	{
		apds9900_set_command(apds_9900_i2c_client,1);
	}
	
//	data->enable = org_enable;
		
	apds9900_set_control(apds_9900_i2c_client, PDRIVE | PDIODE | PGAIN | AGAIN);
	apds9900_set_enable(apds_9900_i2c_client,data->enable);
}

static irqreturn_t apds_9900_irq_handler(int irq, void *dev_id)						   
{
 	struct apds9900_data *data = dev_id;
	// disable irq , irq will enable after work is ended.
	//disable_irq(data->irq);
	
	queue_work(data->irq_wq,&data->irq_work);

	return IRQ_HANDLED;												   
}	


/*
 * I2C init/probing/exit functions
 */

static struct i2c_driver apds9900_driver;
static int __devinit apds9900_probe(struct i2c_client *client,
				   const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct apds9900_data *data;
	int err = 0;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE)) {
		err = -EIO;
		goto exit;
	}

	data = kzalloc(sizeof(struct apds9900_data), GFP_KERNEL);
	if (!data) {
		err = -ENOMEM;
		goto exit;
	}

	memset(data, 0x00, sizeof(struct apds9900_data));

	gpio_request(104,"apds9900");
	gpio_direction_output(104, 1);	// OUTPUT 
	gpio_set_value(104, 1);					// ON 
	
	data->client = client;
	i2c_set_clientdata(client, data);
	
	apds_9900_i2c_client = client;

	data->enable = 0;	/* default mode is standard */
	dev_info(&client->dev, "enable = %s\n",
			data->enable ? "1" : "0");

	/* Initialize the APDS9900 chip */
	err = apds9900_init_client(client);
	if (gpio_request(16,"apds-9900") < 0) {
			 DEBUG_MSG("[ERROR!!] gpio_16 can not be requested\n");
			 err = -EIO;
			 goto exit_kfree;
	}
			 
	if(gpio_direction_input(16) < 0){
		DEBUG_MSG("[ERROR!!] gpio_direction_input\n");
		err = -EIO;
		goto exit_kfree;
	}

	data->irq = OMAP_GPIO_IRQ(16);
				
	if(request_irq(data->irq,apds_9900_irq_handler,IRQF_TRIGGER_FALLING,"apds-9900", data) < 0){
		err = -EIO;
		goto exit_request_irq_failed;
	}
	disable_irq(data->irq);
	
		INIT_WORK(&data->irq_work,apds_9900_irq_work_func);
	data->irq_wq = create_singlethread_workqueue("apds_9900_wq");
	if (!data->irq_wq) {
		 DEBUG_MSG("couldn't create irq queue\n");
		 goto exit_create_drdy_wq_failed;
	}

#ifdef APDS900_SCHEDULE_FOR_DEBUG
	data->poll_delay = 1000;	/* kk 02 May - temporary for check sensor status poll every 1s */
	INIT_DELAYED_WORK(&data->poll_dwork, apds9900_polling_work_handler); /* kk 02 May 2011 - temporary only for check sensor status */
#endif

#ifdef APDS9900_WAKE_LOCK
	wake_lock_init(&data->apds9900_wake_lock, WAKE_LOCK_SUSPEND, "apds9900_wakelock");
#endif

	data->input_dev = input_allocate_device();
	if (!data->input_dev) {
		err = -ENOMEM;
		DEBUG_MSG("Failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}
	set_bit(EV_ABS, data->input_dev->evbit);
//	set_bit(EV_LED, data->input_dev->evbit);
	input_set_abs_params(data->input_dev, ABS_DISTANCE, 0, 360, 0, 0);
	input_set_abs_params(data->input_dev, ABS_MISC, 0, 10240, 0, 0);
//	input_set_abs_params(data->input_dev, LED_MISC, 0, 10240, 0, 0);

	data->input_dev->name = "proximity";

	err = input_register_device(data->input_dev);
	if (err) {
		DEBUG_MSG("Unable to register input device: %s\n",
		       data->input_dev->name);
		goto exit_input_register_device_failed;
	}
	/* Register sysfs hooks */
	err = sysfs_create_group(&client->dev.kobj, &apds9900_attr_group);
	if (err)
		goto exit_kfree;

	dev_info(&client->dev, "support ver. %s enabled\n", DRIVER_VERSION);


	return 0;
exit_input_register_device_failed:	
exit_input_dev_alloc_failed:	
exit_create_drdy_wq_failed:
exit_request_irq_failed:
	
exit_kfree:
		dev_info(&client->dev, "probe error\n");
	kfree(data);
exit:
	return err;
}

static int __devexit apds9900_remove(struct i2c_client *client)
{
	struct apds9900_data *data = i2c_get_clientdata(apds_9900_i2c_client);	 

	sysfs_remove_group(&client->dev.kobj, &apds9900_attr_group);

	DEBUG_MSG("apds9900_remove\n");

	/* Power down the device */
	apds9900_set_enable(client, 0);

#ifdef APDS9900_WAKE_LOCK
    wake_lock_destroy(&data->apds9900_wake_lock);
#endif
	kfree(i2c_get_clientdata(client));

	return 0;
}

#ifdef CONFIG_PM

static int apds9900_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct apds9900_data *data = i2c_get_clientdata(apds_9900_i2c_client);	
	int gpiovalue = gpio_get_value(16);

	printk("apds9900_suspend [%d][%d]\n", data->enable, gpiovalue);

	if((data->enable & APDS9900_ENABLE_PIEN) && (gpiovalue == 0))
	{
		apds_9900_irq_work_func(NULL);
	}

	return 0;
}

static int apds9900_resume(struct i2c_client *client)
{
	printk("apds9900_resume [%d]\n",enable_status);

	if(enable_status != 0)
	{
		apds9900_set_enable(client,enable_status);
	}

	return 0;
}

#else

#define apds9900_suspend	NULL
#define apds9900_resume		NULL

#endif /* CONFIG_PM */

static const struct i2c_device_id apds9900_id[] = {
	{ "apds9900", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, apds9900_id);

static struct i2c_driver apds9900_driver = {
	.driver = {
		.name	= APDS9900_DRV_NAME,
		.owner	= THIS_MODULE,
	},
//	.suspend = NULL,
	.suspend = apds9900_suspend,
	.resume	= NULL,
	.probe	= apds9900_probe,
	.remove	= __devexit_p(apds9900_remove),
	.id_table = apds9900_id,
};

static int __init apds9900_init(void)
{
	return i2c_add_driver(&apds9900_driver);
}

static void __exit apds9900_exit(void)
{
	i2c_del_driver(&apds9900_driver);
}

MODULE_AUTHOR("Lee Kai Koon <kai-koon.lee@avagotech.com>");
MODULE_DESCRIPTION("APDS9900 ambient light + proximity sensor driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);

module_init(apds9900_init);
module_exit(apds9900_exit);

