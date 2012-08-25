/*
 $License:
    Copyright (C) 2010 InvenSense Corporation, All Rights Reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
  $
 */

/**
 *  @defgroup   COMPASSDL (Motion Library - Accelerometer Driver Layer)
 *  @brief      Provides the interface to setup and handle an accelerometers
 *              connected to the secondary I2C interface of the gyroscope.
 *
 *  @{
 *     @file   ami30x.c
 *     @brief  Magnetometer setup and handling methods for Aichi AMI304/AMI305
 *             compass.
*/

/* ------------------ */
/* - Include Files. - */
/* ------------------ */

#ifdef __KERNEL__
#include <linux/module.h>
#endif

#include "mpu.h"
#include "mlsl.h"
#include "mlos.h"

#include <log.h>
#include <delay.h>
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-compass"

#define AMI30X_REG_DATAX (0x10)
#define AMI30X_REG_STAT1 (0x18)
#define AMI30X_REG_CNTL1 (0x1B)
#define AMI30X_REG_CNTL2 (0x1C)
#define AMI30X_REG_CNTL3 (0x1D)
#define AMI30X_REG_CNTL4_1 (0x5C)
#define AMI30X_REG_CNTL4_2 (0x5D)

#define AMI30X_BIT_CNTL1_PC1  (0x80)
#define AMI30X_BIT_CNTL1_ODR1 (0x10)
#define AMI30X_BIT_CNTL1_FS1  (0x02)

#define AMI30X_BIT_CNTL2_IEN  (0x10)
#define AMI30X_BIT_CNTL2_DREN (0x08)
#define AMI30X_BIT_CNTL2_DRP  (0x04)
#define AMI30X_BIT_CNTL3_F0RCE (0x40)

#define AMI306_WINDOW_COTROL
#if defined(AMI306_WINDOW_COTROL)
int flag = 0;
unsigned char offz_val = 0;
unsigned char offz[3] = {0,0,0};
#endif

int ami30x_suspend(void *mlsl_handle,
		   struct ext_slave_descr *slave,
		   struct ext_slave_platform_data *pdata)
{
	int result;
	unsigned char reg;
	result =
	    MLSLSerialRead(mlsl_handle, pdata->address, AMI30X_REG_CNTL1,
			   1, &reg);
	ERROR_CHECK(result);

	reg &= ~(AMI30X_BIT_CNTL1_PC1|AMI30X_BIT_CNTL1_FS1);
	result =
	    MLSLSerialWriteSingle(mlsl_handle, pdata->address,
				  AMI30X_REG_CNTL1, reg);
	ERROR_CHECK(result);

	return result;
}

int ami30x_resume(void *mlsl_handle,
		  struct ext_slave_descr *slave,
		  struct ext_slave_platform_data *pdata)
{
	int result = ML_SUCCESS;
	unsigned char regs[] = {
		 AMI30X_REG_CNTL4_1,
		 0x7E,
		 0xA0
	};

#if defined(AMI306_WINDOW_COTROL)
	if(flag == 0){ /* read OFFZ register */ 
		result =
			MLSLSerialRead(mlsl_handle,pdata->address,0x78,1,&offz_val);
		ERROR_CHECK(result);
		
		flag = 1;
		offz[0] = 0x78; //AMI306_REG_OFFZ
		offz[1] = offz_val - 10;
		offz[2] = 0x00;
	}
#endif
	
	/* Step1. Set CNTL1 reg to power model active (Write CNTL1:PC1=1)*/
	result =
	    MLSLSerialWriteSingle(mlsl_handle, pdata->address,
				  AMI30X_REG_CNTL1,
				  AMI30X_BIT_CNTL1_PC1|AMI30X_BIT_CNTL1_FS1);
	ERROR_CHECK(result);
	/* Step2. Set CNTL2 reg to DRDY active high and enabled (Write CNTL2:DREN=1)*/
	result =
	    MLSLSerialWriteSingle(mlsl_handle, pdata->address,
				  AMI30X_REG_CNTL2,
				  AMI30X_BIT_CNTL2_DREN |
				  AMI30X_BIT_CNTL2_DRP);
	ERROR_CHECK(result);

	/* Step3. Set CNTL4 reg to for measurement speed (Write CNTL4, 0xA07E) */
	result =
	    MLSLSerialWrite(mlsl_handle, pdata->address, DIM(regs), regs);
	ERROR_CHECK(result);

	/* Step4. Set OFFZ reg (Write OFFZ, offz_data)*/
#if defined(AMI306_WINDOW_COTROL)
	result =
	    MLSLSerialWrite(mlsl_handle, pdata->address, DIM(offz), offz);
	ERROR_CHECK(result); //110315//
#endif
	/* Step5. Set CNTL3 reg to forced measurement period (Write CNTL3:FORCE=1)*/
#if 0
	result =
		MLSLSerialWriteSingle(mlsl_handle, pdata->address,
				  AMI30X_REG_CNTL3, AMI30X_BIT_CNTL3_F0RCE);
#endif
	return result;
}

int ami30x_read(void *mlsl_handle,
		struct ext_slave_descr *slave,
		struct ext_slave_platform_data *pdata, unsigned char *data)
{
	unsigned char stat;
	int result = ML_SUCCESS;
	int status = ML_SUCCESS;

	/* Measurement(x,y,z) */
	result =
		MLSLSerialWriteSingle(mlsl_handle, pdata->address,
				      AMI30X_REG_CNTL3,
				      AMI30X_BIT_CNTL3_F0RCE);
	ERROR_CHECK(result);
	udelay(500);
	
	/* Step6. Read status reg and check if data ready (DRDY) */
	result =
	    MLSLSerialRead(mlsl_handle, pdata->address, AMI30X_REG_STAT1,
			   1, &stat);
	ERROR_CHECK(result);

	/* Step6. Does DRDY output the rising edge?*/
	if (1/*stat & 0x40*/) /*Yes*/
	{ 
		int mx, my, mz;
		int tempX, tempY, tempZ;

		mx = my = mz = 0;
		tempX = tempY = tempZ = 0;
		
		/* Step7. Read DATAX(0x10/11),Y(0x12/13),Z(0x14/15) */		
		result =
		    MLSLSerialRead(mlsl_handle, pdata->address,
				   AMI30X_REG_DATAX, 6,
				   (unsigned char *) data);


		tempX=data[1]; tempY=data[3]; tempZ=data[5];

		mx = tempX << 8 | data[0];	  //upper byte + lower byte
		my = tempY << 8 | data[2];
		mz = tempZ << 8 | data[4];

		if (mx>32768)  mx = mx-65536;	  //if MSB == 1, it is minus value. (0x8000 = 32768)
		if (my>32768)  my = my-65536;	  //then subtraction 0x10000(65536)
		if (mz>32768)  mz = mz-65536;

#if defined(AMI306_WINDOW_COTROL)
		if(mz>1700)
		{
			offz[1] += 5;
			if(offz[1]>80)
				offz[1] = 80;
			result =
				MLSLSerialWrite(mlsl_handle, pdata->address, DIM(offz), offz);
			ERROR_CHECK(result);

			/* Measurement(x,y,z) */
			result =
				MLSLSerialWriteSingle(mlsl_handle, pdata->address,
				      AMI30X_REG_CNTL3,
				      AMI30X_BIT_CNTL3_F0RCE);
			ERROR_CHECK(result);
			udelay(500);
			/* Read DATAX(0x10/11),Y(0x12/13),Z(0x14/15) */		
			result =
		    MLSLSerialRead(mlsl_handle, pdata->address,
				   AMI30X_REG_DATAX, 6,
				   (unsigned char *) data);
		}
		else if(mz<-1700)
		{
			offz[1] -= 5;
			if(offz[1]<10)
				offz[1] = 10;
			result =
				MLSLSerialWrite(mlsl_handle, pdata->address, DIM(offz), offz);
			ERROR_CHECK(result);

			/* Measurement(x,y,z) */
			result =
				MLSLSerialWriteSingle(mlsl_handle, pdata->address,
				      AMI30X_REG_CNTL3,
				      AMI30X_BIT_CNTL3_F0RCE);
			ERROR_CHECK(result);
			udelay(500);
			/* Read DATAX(0x10/11),Y(0x12/13),Z(0x14/15) */		
			result =
				MLSLSerialRead(mlsl_handle, pdata->address,
				   AMI30X_REG_DATAX, 6,
				   (unsigned char *) data);

		}
#endif
		ERROR_CHECK(result);
		status = ML_SUCCESS;	
	} else if (stat & 0x20) { /*No*/
		status = ML_ERROR_COMPASS_DATA_OVERFLOW;
	}

	return status;
}

struct ext_slave_descr ami30x_descr = {
	/*.init             = */ NULL,
	/*.exit             = */ NULL,
	/*.suspend          = */ ami30x_suspend,
	/*.resume           = */ ami30x_resume,
	/*.read             = */ ami30x_read,
	/*.config           = */ NULL,
	/*.get_config       = */ NULL,
	/*.name             = */ "ami30x",
	/*.type             = */ EXT_SLAVE_TYPE_COMPASS,
	/*.id               = */ COMPASS_ID_AMI30X,
	/*.reg              = */ 0x06,
	/*.len              = */ 6,
	/*.endian           = */ EXT_SLAVE_LITTLE_ENDIAN,
	/*.range            = */ {5461, 3333}
	/* For AMI305,the range field needs to be modified to {9830.4f}*/
};

struct ext_slave_descr *ami30x_get_slave_descr(void)
{
	return &ami30x_descr;
}
EXPORT_SYMBOL(ami30x_get_slave_descr);

/**
 *  @}
**/
