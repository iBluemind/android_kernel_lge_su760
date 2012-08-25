/*
 *  linux/drivers/mmc/sdio_ops.c
 *
 *  Copyright 2006-2007 Pierre Ossman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */

#include <linux/scatterlist.h>

#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sdio.h>

#include "core.h"
#include "sdio_ops.h"

int mmc_send_io_op_cond(struct mmc_host *host, u32 ocr, u32 *rocr)
{
	struct mmc_command cmd;
	int i, err = 0;

	BUG_ON(!host);

	memset(&cmd, 0, sizeof(struct mmc_command));

	cmd.opcode = SD_IO_SEND_OP_COND;
	cmd.arg = ocr;
	cmd.flags = MMC_RSP_SPI_R4 | MMC_RSP_R4 | MMC_CMD_BCR;

	for (i = 100; i; i--) {
		err = mmc_wait_for_cmd(host, &cmd, MMC_CMD_RETRIES);
		if (err)
			break;

		/* if we're just probing, do a single pass */
		if (ocr == 0)
			break;

		/* otherwise wait until reset completes */
		if (mmc_host_is_spi(host)) {
			/*
			 * Both R1_SPI_IDLE and MMC_CARD_BUSY indicate
			 * an initialized card under SPI, but some cards
			 * (Marvell's) only behave when looking at this
			 * one.
			 */
			if (cmd.resp[1] & MMC_CARD_BUSY)
				break;
		} else {
			if (cmd.resp[0] & MMC_CARD_BUSY)
				break;
		}

		err = -ETIMEDOUT;

		mmc_delay(10);
	}
#if 1      /* JamesLee :: Jun17 */
// TI Debugging code from Lee,Cliff [cliff.lee@ti.com] - 2011-07-11
    if(err != 0)
    {
        printk("@@@@@@@@%s(index=%d), err = %d\n", __func__, host->index, err);
        if(host->index==2)
        {
            printk("5010=0x%8x, 5110=0x%8x, 5114=0x%8x, 5124=0x%8x, 5128=0x%8x\n",
                        omap_readl(0x480d5010),
                        omap_readl(0x480d5110),
                        omap_readl(0x480d5114),
                        omap_readl(0x480d5124),
                        omap_readl(0x480d5128));

            printk("512C=0x%8x, 5130=0x%8x, 5200=0x%8x, 5204=0x%8x, 5208=0x%8x\n",
                        omap_readl(0x480d512C),
                        omap_readl(0x480d5130),
                        omap_readl(0x480d5200),
                        omap_readl(0x480d5204),
                        omap_readl(0x480d5208));

            printk("520C=0x%8x, 5210=0x%8x, 5214=0x%8x, 5218=0x%8x, 521C=0x%8x\n",
                        omap_readl(0x480d520C),
                        omap_readl(0x480d5210),
                        omap_readl(0x480d5214),
                        omap_readl(0x480d5218),
                        omap_readl(0x480d521C));

            printk("5220=0x%8x, 5224=0x%8x, 5228=0x%8x, 522C=0x%8x, 5230=0x%8x\n",
                        omap_readl(0x480d5220),
                        omap_readl(0x480d5224),
                        omap_readl(0x480d5228),
                        omap_readl(0x480d522C),
                        omap_readl(0x480d5230));

            printk("5234=0x%8x, 5238=0x%8x, 523C=0x%8x, 5240=0x%8x, 5248=0x%8x\n",
                        omap_readl(0x480d5234),
                        omap_readl(0x480d5238),
                        omap_readl(0x480d523C),
                        omap_readl(0x480d5240),
                        omap_readl(0x480d5248));

            printk("5250=0x%8x, 52FC=0x%8x\n",
                        omap_readl(0x480d5250),
                        omap_readl(0x480d52FC));

            printk("MMCSD5_CLKCTRL=0x%x\n", omap_readl(0x4a009560));
        }
    }
#endif
	if (rocr)
		*rocr = cmd.resp[mmc_host_is_spi(host) ? 1 : 0];

	return err;
}

static int mmc_io_rw_direct_host(struct mmc_host *host, int write, unsigned fn,
	unsigned addr, u8 in, u8 *out)
{
	struct mmc_command cmd;
	int err;

	BUG_ON(!host);
	BUG_ON(fn > 7);

	/* sanity check */
	if (addr & ~0x1FFFF)
		return -EINVAL;

	memset(&cmd, 0, sizeof(struct mmc_command));

	cmd.opcode = SD_IO_RW_DIRECT;
	cmd.arg = write ? 0x80000000 : 0x00000000;
	cmd.arg |= fn << 28;
	cmd.arg |= (write && out) ? 0x08000000 : 0x00000000;
	cmd.arg |= addr << 9;
	cmd.arg |= in;
	cmd.flags = MMC_RSP_SPI_R5 | MMC_RSP_R5 | MMC_CMD_AC;

	err = mmc_wait_for_cmd(host, &cmd, 0);
	if (err)
		return err;

	if (mmc_host_is_spi(host)) {
		/* host driver already reported errors */
	} else {
		if (cmd.resp[0] & R5_ERROR)
			return -EIO;
		if (cmd.resp[0] & R5_FUNCTION_NUMBER)
			return -EINVAL;
		if (cmd.resp[0] & R5_OUT_OF_RANGE)
			return -ERANGE;
	}

	if (out) {
		if (mmc_host_is_spi(host))
			*out = (cmd.resp[0] >> 8) & 0xFF;
		else
			*out = cmd.resp[0] & 0xFF;
	}

	return 0;
}

int mmc_io_rw_direct(struct mmc_card *card, int write, unsigned fn,
	unsigned addr, u8 in, u8 *out)
{
	BUG_ON(!card);
	return mmc_io_rw_direct_host(card->host, write, fn, addr, in, out);
}

int mmc_io_rw_extended(struct mmc_card *card, int write, unsigned fn,
	unsigned addr, int incr_addr, u8 *buf, unsigned blocks, unsigned blksz)
{
	struct mmc_request mrq;
	struct mmc_command cmd;
	struct mmc_data data;
	struct scatterlist sg;

	BUG_ON(!card);
	BUG_ON(fn > 7);
	BUG_ON(blocks == 1 && blksz > 512);
	WARN_ON(blocks == 0);
	WARN_ON(blksz == 0);

	/* sanity check */
	if (addr & ~0x1FFFF)
		return -EINVAL;

	memset(&mrq, 0, sizeof(struct mmc_request));
	memset(&cmd, 0, sizeof(struct mmc_command));
	memset(&data, 0, sizeof(struct mmc_data));

	mrq.cmd = &cmd;
	mrq.data = &data;

	cmd.opcode = SD_IO_RW_EXTENDED;
	cmd.arg = write ? 0x80000000 : 0x00000000;
	cmd.arg |= fn << 28;
	cmd.arg |= incr_addr ? 0x04000000 : 0x00000000;
	cmd.arg |= addr << 9;
	if (blocks == 1 && blksz <= 512)
		cmd.arg |= (blksz == 512) ? 0 : blksz;	/* byte mode */
	else
		cmd.arg |= 0x08000000 | blocks;		/* block mode */
	cmd.flags = MMC_RSP_SPI_R5 | MMC_RSP_R5 | MMC_CMD_ADTC;

	data.blksz = blksz;
	data.blocks = blocks;
	data.flags = write ? MMC_DATA_WRITE : MMC_DATA_READ;
	data.sg = &sg;
	data.sg_len = 1;

	sg_init_one(&sg, buf, blksz * blocks);

	mmc_set_data_timeout(&data, card);

	mmc_wait_for_req(card->host, &mrq);

	if (cmd.error)
		return cmd.error;
	if (data.error)
		return data.error;

	if (mmc_host_is_spi(card->host)) {
		/* host driver already reported errors */
	} else {
		if (cmd.resp[0] & R5_ERROR)
			return -EIO;
		if (cmd.resp[0] & R5_FUNCTION_NUMBER)
			return -EINVAL;
		if (cmd.resp[0] & R5_OUT_OF_RANGE)
			return -ERANGE;
	}

	return 0;
}

int sdio_reset(struct mmc_host *host)
{
	int ret;
	u8 abort;

	/* SDIO Simplified Specification V2.0, 4.4 Reset for SDIO */

	ret = mmc_io_rw_direct_host(host, 0, 0, SDIO_CCCR_ABORT, 0, &abort);
	if (ret)
		abort = 0x08;
	else
		abort |= 0x08;

	ret = mmc_io_rw_direct_host(host, 1, 0, SDIO_CCCR_ABORT, abort, NULL);
	return ret;
}

