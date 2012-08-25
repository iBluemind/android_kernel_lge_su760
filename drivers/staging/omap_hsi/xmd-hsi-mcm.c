/*
 * xmd-hsi-mcm.c
 *
 * Copyright (C) 2011 Intel Mobile Communications. All rights reserved.
 *
 * Author: Chaitanya <Chaitanya.Khened@intel.com>
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

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include "xmd-ch.h"
#include "xmd-hsi-mcm.h"
#include "xmd-hsi-ll-if.h"
#include "xmd_hsi_mem.h"
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/wakelock.h>

#if defined(HSI_MCM_NOTIFY_TO_CHARGER)
#include <linux/cosmo/charger_rt9524.h>
#endif

#define MCM_DBG_LOG 0
#define MCM_DBG_ERR_LOG 1
#define MCM_DBG_ERR_RECOVERY_LOG 1

/* TI HSI driver (from HSI_DRIVER_VERSION 0.4.2) can suppport port 1 and 2, 
	but IMC XMD currently supports port 1 only */
#define XMD_SUPPORT_PORT 1

static struct hsi_chn hsi_all_channels[MAX_HSI_CHANNELS] = {
	{"CONTROL",  HSI_CH_NOT_USED},
	{"CHANNEL1", HSI_CH_FREE},
	{"CHANNEL2", HSI_CH_FREE},
	{"CHANNEL3", HSI_CH_FREE},
	{"CHANNEL4", HSI_CH_FREE},
	{"CHANNEL5", HSI_CH_FREE},
	{"CHANNEL6", HSI_CH_FREE},
	{"CHANNEL7", HSI_CH_FREE},
	{"CHANNEL8", HSI_CH_FREE},
	{"CHANNEL9", HSI_CH_FREE},
	{"CHANNEL10",HSI_CH_FREE},
	{"CHANNEL11",HSI_CH_FREE},
	{"CHANNEL12",HSI_CH_FREE},
	{"CHANNEL13",HSI_CH_FREE},
	{"CHANNEL14",HSI_CH_FREE},
	{"CHANNEL15",HSI_CH_FREE},
};

static unsigned int hsi_mcm_state;
static int is_dlp_reset_in_progress;

static struct work_struct XMD_DLP_RECOVERY_wq;
static struct hsi_channel hsi_channels[MAX_HSI_CHANNELS];
static struct workqueue_struct *hsi_read_wq;
static struct workqueue_struct *hsi_write_wq;
#if defined (HSI_LL_ENABLE_RX_BUF_RETRY_WQ)
static struct workqueue_struct *hsi_buf_retry_wq;
#endif

#if defined(HSI_MCM_NOTIFY_TO_CHARGER)
static unsigned int ifx_hsi_modem_alive = 1;
#endif

#define ENABLE_RECOVERY_WAKE_LOCK

#if defined (ENABLE_RECOVERY_WAKE_LOCK)
#define RECOVERY_WAKELOCK_TIME		(30*HZ)
struct wake_lock xmd_recovery_wake_lock;
#endif

void (*xmd_boot_cb)(void);
static void hsi_read_work(struct work_struct *work);
static void hsi_write_work(struct work_struct *work);

static void xmd_dlp_recovery(int chno);
static int xmd_ch_reinit(int chno);
static void xmd_dlp_recovery_wq(struct work_struct *cp_crash_wq);
#if defined (HSI_LL_ENABLE_RX_BUF_RETRY_WQ)
static void hsi_buf_retry_work(struct work_struct *work);
#endif
extern void ifx_pmu_reset(void);
extern void rmnet_restart_queue(int chno);

void init_q(int chno)
{
	int i;

	hsi_channels[chno].rx_q.head  = 0;
	hsi_channels[chno].rx_q.tail  = 0;
	hsi_channels[chno].tx_q.head  = 0;
	hsi_channels[chno].tx_q.tail  = 0;
	hsi_channels[chno].tx_blocked = 0;
	hsi_channels[chno].pending_rx_msgs = 0;
	hsi_channels[chno].pending_tx_msgs = 0;

	for (i=0; i<NUM_X_BUF; i++)
		hsi_channels[chno].rx_q.data[i].being_used = HSI_FALSE;
}

/* Head grows on reading from q. "data=q[head];head++;" */
struct x_data* read_q(int chno, struct xq* q)
{
	struct x_data *data = NULL;

	if (!q) {
#if MCM_DBG_ERR_LOG
		printk("mcm: NULL Q instance");
#endif
		return NULL;
	}

#if MCM_DBG_LOG
	printk("\nmcm: [read_q]  head = %d, tail = %d\n",q->head,q->tail);
#endif

	spin_lock_bh(&hsi_channels[chno].lock);

	if (q->head == q->tail) {
		spin_unlock_bh(&hsi_channels[chno].lock);
#if MCM_DBG_LOG
		printk("\nmcm: Q empty [read] \n");
#endif
		return NULL;
	}

	data = q->data + q->head;
	q->head = (q->head + 1) % NUM_X_BUF;

	spin_unlock_bh(&hsi_channels[chno].lock);

	return data;
}

/* Tail grows on writing in q. "q[tail]=data;tail++;" */
int write_q(struct xq* q, char *buf, int size, struct x_data **data)
{
	int temp = 0;

	if (!q) {
#if MCM_DBG_ERR_LOG
		printk("mcm: NULL q instance");
#endif
		return 0;
	}

	temp = (q->tail + 1) % NUM_X_BUF;

	if (temp != q->head) {
		q->data[q->tail].buf  = buf;
		q->data[q->tail].size = size;
		if (data) {
			*data = q->data + q->tail;
		}
		q->tail = temp;
	} else {
#if MCM_DBG_LOG
		printk("\nmcm:Q full [write], head = %d, tail = %d\n",q->head,q->tail);
#endif
		return 0;
	}

		return q->tail > q->head ? q->tail - q->head:q->tail - q->head + NUM_X_BUF;
}

static int hsi_ch_net_write(int chno, void *data, int len)
{
	/* Non blocking write */
	void *buf = NULL;
	static struct x_data *d = NULL;
	int n = 0;
	int flag = 1;
	int ret = 0;

	if (!data) {
#if MCM_DBG_ERR_LOG
		printk("\nmcm: data is NULL.\n");
#endif
		return -EINVAL;
	}

#ifdef XMD_TX_MULTI_PACKET
	if (d && hsi_channels[chno].write_queued == HSI_TRUE) {
		if (d->being_used == HSI_FALSE && (d->size + len) < HSI_MEM_LARGE_BLOCK_SIZE) {
#if MCM_DBG_LOG
			printk("\nmcm: Adding in the queued buffer for ch %d\n",chno);
#endif
			buf = d->buf + d->size;
			d->size += len;
			flag = 0;
		} else {
			flag = 1;
		}
	}
#endif
	if (flag) {
#ifdef XMD_TX_MULTI_PACKET
		buf = hsi_mem_alloc(HSI_MEM_LARGE_BLOCK_SIZE);
#else
		buf = hsi_mem_alloc(len);
#endif
		flag = 1;
	}

	if (!buf) {
#if MCM_DBG_ERR_LOG
		printk("\nmcm: Failed to alloc memory So Cannot transfer packet.\n");
#endif
#if 1
		hsi_channels[chno].tx_blocked = 1;
#endif
		return -ENOMEM;
	}

	memcpy(buf, data, len);

	if (flag) {
		d = NULL;
		n = write_q(&hsi_channels[chno].tx_q, buf, len, &d);
		if (n != 0) {
			hsi_channels[chno].pending_tx_msgs++;
		}
#if MCM_DBG_LOG
		printk("\nmcm: n = %d\n",n);
#endif
		if (n == 0) {
#if MCM_DBG_LOG
			printk("\nmcm: rmnet TX queue is full for channel %d, So cannot transfer this packet.\n",chno);
#endif
			hsi_channels[chno].tx_blocked = 1;
			hsi_mem_free(buf);
#if 1
			if (hsi_channels[chno].write_queued == HSI_TRUE) {
#if MCM_DBG_LOG
				printk("\nmcm: hsi_ch_net_write wq already in progress\n");
#endif
			}
			else {
			PREPARE_WORK(&hsi_channels[chno].write_work, hsi_write_work);
			queue_work(hsi_write_wq, &hsi_channels[chno].write_work);
			}
#endif
			ret = -EBUSY;
		} else if (n == 1) {
			PREPARE_WORK(&hsi_channels[chno].write_work, hsi_write_work);
			queue_work(hsi_write_wq, &hsi_channels[chno].write_work);
			ret = 0;
		}
	}

	return ret;
}

#if defined(HSI_MCM_NOTIFY_TO_CHARGER)
static void hsi_ch_notify_to_charger(int error)
{
	if(error == 0) { /* No error */
		if(ifx_hsi_modem_alive == 0) {
			set_modem_alive(1);
			ifx_hsi_modem_alive = 1;
#if MCM_DBG_LOG
			printk("\nmcm:set_modem_alive on");
#endif
		}
	}
	else { /* -EREMOTEIO */
			set_modem_alive(0);
			ifx_hsi_modem_alive = 0;
#if MCM_DBG_LOG
			printk("\nmcm:set_modem_alive off");
#endif
	}
}
#endif

/* RIL recovery : hsi_ch_tty_write loops  continuously if there is no operation */
#if defined(MIPI_HSI_TTY_WRITE_TIMEOUT_FEATURE) || defined(MIPI_HSI_NET_WRITE_TIMEOUT_FEATURE)
#define HSI_WRITE_TIMEOUT		(12 * HZ)

static int hsi_ch_write_timeout(int chno, void *buf)
{
	int err = 0, rc = 0;

	rc = wait_event_timeout(hsi_channels[chno].write_wait,
			hsi_channels[chno].write_happening == HSI_FALSE, HSI_WRITE_TIMEOUT);

	if( hsi_mcm_state == HSI_MCM_STATE_ERR_RECOVERY) {
#if MCM_DBG_LOG
		printk("\nmcm:locking 1st mutex end for ch: %d\n", chno);
#endif
		return -EREMOTEIO;
	}

	if (rc == 0) { 
		int ret = hsi_ll_check_channel(chno);

		if(ret == -EPERM) {
			err = -EREMOTEIO;
			
			hsi_channels[chno].write_happening = HSI_FALSE;
			hsi_ll_reset_write_channel(chno);
			hsi_mem_free(buf);
#if MCM_DBG_ERR_LOG
			printk("\nmcm: hsi_ll_check_channel - hsi_ch_write_timeout(...) failed\n");
#endif
		}
		else if(ret == -EACCES){
			hsi_channels[chno].write_happening = HSI_FALSE;
			err = -EREMOTEIO;
			
#if MCM_DBG_ERR_LOG
			printk("\nmcm:unlocking 1st mutex end for ch: %d\n", chno);
#endif
		}
		else if(ret == -EBUSY){
			wait_event(hsi_channels[chno].write_wait,
				hsi_channels[chno].write_happening == HSI_FALSE);

			err = 0;
			
#if MCM_DBG_LOG
			printk("\nmcm:unlocking 2st mutex end for ch: %d\n", chno);
#endif
		}
		else {
			err = 0;
#if MCM_DBG_LOG
			printk("\nmcm:unlocking 3st mutex end for ch: %d\n", chno);
#endif
		}
	}
	else {
#if MCM_DBG_LOG
		printk("\nmcm:unlocking 4st mutex end for ch: %d\n", chno);
#endif
	}

	return err;
}
#endif

static int hsi_ch_tty_write(int chno, void *data, int len)
{
	void *buf = NULL;
	int err;

	buf = hsi_mem_alloc(len);

	if (!buf) {
		return -ENOMEM;
	}

	memcpy(buf, data, len);

	hsi_channels[chno].write_happening = HSI_TRUE;

	err = hsi_ll_write(chno, (unsigned char *)buf, len);
	if (err < 0) {
#if MCM_DBG_ERR_LOG
		printk("\nmcm: hsi_ll_write(...) failed. err=%d\n",err);
#endif

/* Should free in error case */
#if 1
		hsi_mem_free(buf);
#endif

		hsi_channels[chno].write_happening = HSI_FALSE;
	}
#if defined(MIPI_HSI_TTY_WRITE_TIMEOUT_FEATURE)	
	else {
#if MCM_DBG_LOG
		printk("\nmcm:locking mutex start for ch: %d\n", chno);
#endif

#if defined (TARGET_CARRIER_ATT) && !defined (MIPI_HSI_CHECK_CP_RX_INFO)
		if(chno == XMD_TTY_CIQ_CHANNEL) {
			wait_event(hsi_channels[chno].write_wait,
				hsi_channels[chno].write_happening == HSI_FALSE);
#if MCM_DBG_LOG
			printk("\nmcm:locking mutex end for ch: %d\n", chno);
#endif
		}
		else
#endif 
		{
			err = hsi_ch_write_timeout(chno, buf);

#if MCM_DBG_LOG
			if (err < 0)
				printk("\nmcm:hsi_ch_write_timeout ret %d for ch: %d\n", err, chno);
#endif
		}

#if defined(HSI_MCM_NOTIFY_TO_CHARGER)
		hsi_ch_notify_to_charger(err);
#endif
	}
#else
	else {
#if MCM_DBG_LOG
		printk("\nmcm:locking mutex start for ch: %d\n", chno);
#endif
		wait_event(hsi_channels[chno].write_wait,
				hsi_channels[chno].write_happening == HSI_FALSE);
#if MCM_DBG_LOG
		printk("\nmcm:locking mutex end for ch: %d\n", chno);
#endif
	}
#endif

	return err;
}

static void* hsi_ch_net_read(int chno, int* len)
{
	struct x_data *data = hsi_channels[chno].curr;
	*len = data->size;
	return data->buf;
}

static void* hsi_ch_tty_read(int chno, int* len)
{
	struct x_data *data = hsi_channels[chno].curr;
	*len = data->size;
	return data->buf;
}

int xmd_ch_write(int chno, void *data, int len)
{
	int err;

#if MCM_DBG_LOG
	printk("\nmcm: write entering, ch %d\n",chno);
#endif

	if (!hsi_channels[chno].write) {
#if MCM_DBG_ERR_LOG
		printk("\nmcm:write func NULL for ch: %d\n",chno);
#endif
		return -EINVAL;
	}

	if (hsi_mcm_state == HSI_MCM_STATE_ERR_RECOVERY) {
#if MCM_DBG_ERR_RECOVERY_LOG
		printk("\nmcm:Dropping packets of channel %d as error recovery is in progress\n", chno);
#endif
		return -ENOTBLK;
	}

	err = hsi_channels[chno].write(chno, data, len);

#if MCM_DBG_LOG
	printk("\nmcm: write returning, ch %d\n",chno);
#endif
	return err;
}

void* xmd_ch_read(int chno, int* len)
{
	return hsi_channels[chno].read(chno,len);
}

void xmd_ch_close(int chno)
{
#if defined(CONFIG_MACH_LGE_COSMOPOLITAN)
	/******************************************
		LGE-RIL CHANNEL : 1 , 2, 3, 4, 5, 8, 9,(11: KR SKT),12
	******************************************/
	if((chno == 1)||(chno == 2)||(chno == 3)
	||(chno == 4)||(chno == 5)||(chno == 8)
#if defined(CONFIG_MACH_LGE_COSMO_DOMASTIC)
	||(chno == 11)
#endif
	||(chno == 9)||(chno == 12))
#else
	if(chno == XMD_RIL_RECOVERY_CHANNEL)
#endif
	{

#if 1
		xmd_dlp_recovery(chno);
#endif					
	}
#if 0
	if (hsi_channels[chno].read_happening == HSI_TRUE) {
#if 1 //MCM_DBG_LOG
		printk("\nmcm:locking read mutex for ch: %d\n",chno);
#endif
		wait_event(hsi_channels[chno].read_wait,
					hsi_channels[chno].read_happening == HSI_FALSE);
#if 1 //MCM_DBG_LOG
		printk("\nmcm:unlocking read mutex for ch: %d\n",chno);
#endif
	}
#endif
	hsi_ll_close(chno);
	spin_lock_bh(&hsi_channels[chno].lock);
	hsi_channels[chno].state = HSI_CH_FREE;
	spin_unlock_bh(&hsi_channels[chno].lock);

	printk("\nmcm:finish closing channel %d.\n", chno);
}

int xmd_ch_open(struct xmd_ch_info* info, void (*notify_cb)(int chno))
{
	int i, ret;
	int size = ARRAY_SIZE(hsi_channels);

	for (i=0; i<size; i++) {
		if (hsi_channels[i].name)
			if (!strcmp(info->name, hsi_channels[i].name)) {
				if (hsi_channels[i].state == HSI_CH_BUSY ||

					hsi_channels[i].state == HSI_CH_NOT_USED) {
#if MCM_DBG_ERR_LOG
					printk("\nmcm:Channel state not suitable %d\n",i);
#endif
					return -EINVAL;
				}

#if defined(CONFIG_MACH_LGE_COSMOPOLITAN)
				/******************************************
					LGE-RIL CHANNEL : 1 , 2, 3, 4, 5, 8, 9,(11: KR SKT),12
				******************************************/
				if (hsi_mcm_state == HSI_MCM_STATE_ERR_RECOVERY) {
					if (i == XMD_RIL_RECOVERY_CHANNEL) {
						ret = xmd_ch_reinit(i);
						if(ret != 0)
							return ret;
					}
					else {
#if MCM_DBG_ERR_LOG
						printk("\nmcm:hsi_ll_open failed. Not Recovery channel %d Error\n",i);
#endif
						return -ENOTBLK;
					}
				}
#else /* CONFIG_MACH_LGE_COSMOPOLITAN */

				if ((i == XMD_RIL_RECOVERY_CHANNEL) &&
					(hsi_mcm_state == HSI_MCM_STATE_ERR_RECOVERY)) {
					
					ret = xmd_ch_reinit(i);
					if(ret != 0)
						return ret;
				}
#endif					

				if (0 != hsi_ll_open(i)) {
#if MCM_DBG_ERR_LOG
					printk("\nmcm:hsi_ll_open failed for channel %d\n",i);
#endif
					return -EINVAL;
				}

				hsi_channels[i].info = info;

				spin_lock_bh(&hsi_channels[i].lock);
				hsi_channels[i].state = HSI_CH_BUSY;
				spin_unlock_bh(&hsi_channels[i].lock);

				hsi_channels[i].notify = notify_cb;
				switch(info->user)
				{
				case XMD_TTY:
					hsi_channels[i].read = hsi_ch_tty_read;
					hsi_channels[i].write = hsi_ch_tty_write;
				break;
				case XMD_NET:
					hsi_channels[i].read = hsi_ch_net_read;
					hsi_channels[i].write = hsi_ch_net_write;
				break;
				default:
#if MCM_DBG_ERR_LOG
					printk("\nmcm:Neither TTY nor NET \n");
#endif
					return -EINVAL;
				}
				INIT_WORK(&hsi_channels[i].read_work, hsi_read_work);
				INIT_WORK(&hsi_channels[i].write_work, hsi_write_work);
#if defined (HSI_LL_ENABLE_RX_BUF_RETRY_WQ)
				INIT_WORK(&hsi_channels[i].buf_retry_work, hsi_buf_retry_work);
#endif
				return i;
			}
	}
#if MCM_DBG_ERR_LOG
	printk("\n Channel name not proper \n");
#endif
	return -EINVAL;
}

void hsi_read_work(struct work_struct *work)
{
	/* function registered with read work q */
	struct hsi_channel *ch = (struct hsi_channel*) container_of(work,
													struct hsi_channel,
													read_work);
	int chno = ch->info->chno;
	struct x_data *data = NULL;

	if (hsi_channels[chno].read_queued == HSI_TRUE) {
#if MCM_DBG_LOG
		printk("\nmcm: read wq already in progress\n");
#endif
		return;
	}

	hsi_channels[chno].read_queued = HSI_TRUE;

	while ((data = read_q(chno, &hsi_channels[chno].rx_q)) != NULL) {
		char *buf = data->buf;
		hsi_channels[chno].curr = data;

		if(hsi_channels[chno].state == HSI_CH_BUSY)
		{
			if (hsi_mcm_state != HSI_MCM_STATE_ERR_RECOVERY) {
				hsi_channels[chno].notify(chno);
#if MCM_DBG_ERR_RECOVERY_LOG
			} else {
				printk("\nmcm:Dropping RX packets of channel %d from WQ as error recovery is in progress\n", chno);
#endif
			}
		}
		else {
			printk("\nmcm:Dropping RX packets of channel %d from WQ as Not HSI_CH_BUSY\n", chno);
		}

		hsi_mem_free(buf);
		if(chno >= 13) {
#if defined (HSI_LL_ENABLE_RX_BUF_RETRY_WQ)
			if(hsi_channels[chno].rx_blocked) {
				hsi_channels[chno].rx_blocked = 0;
				spin_lock_bh(&hsi_channels[chno].lock);
				hsi_channels[chno].pending_rx_msgs++;
				spin_unlock_bh(&hsi_channels[chno].lock);
				PREPARE_WORK(&hsi_channels[chno].buf_retry_work, hsi_buf_retry_work);
				queue_work(hsi_buf_retry_wq, &hsi_channels[chno].buf_retry_work);
			}
#endif
			hsi_channels[chno].pending_rx_msgs--;
		}
	}
	hsi_channels[chno].read_queued = HSI_FALSE;
	spin_lock_bh(&hsi_channels[chno].lock);
	hsi_channels[chno].read_happening = HSI_FALSE;
	spin_unlock_bh(&hsi_channels[chno].lock);

	wake_up(&hsi_channels[chno].read_wait);
}

void hsi_write_work(struct work_struct *work)
{
	/* function registered with write work q */
	struct hsi_channel *ch = (struct hsi_channel*) container_of(work,
													struct hsi_channel,
													write_work);
	int chno = ch->info->chno;
	struct x_data *data = NULL;
	int err = 0;

/* RIL recovery memory initialization  */
#if 0
	if (hsi_mcm_state == HSI_MCM_STATE_ERR_RECOVERY) {
#if MCM_DBG_ERR_RECOVERY_LOG
		printk("\nmcm:Dropping packets of channel %d from WQ as error recovery is in progress\n", chno);
#endif
		goto quit_write_wq;
	}
#endif

	if (hsi_channels[chno].write_queued == HSI_TRUE) {
#if MCM_DBG_LOG
		printk("\nmcm: write wq already in progress\n");
#endif
		return;
	}

	hsi_channels[chno].write_queued = HSI_TRUE;

	while ((data = read_q(chno, &hsi_channels[chno].tx_q)) != NULL) {
/* RIL recovery memory initialization  */
#if 1
		if (hsi_mcm_state == HSI_MCM_STATE_ERR_RECOVERY) {
#if MCM_DBG_ERR_RECOVERY_LOG
			printk("\nmcm:Dropping packets of channel %d from WQ "
					"as error recovery is in progress\n", chno);
#endif
			hsi_mem_free(data->buf);
			hsi_channels[chno].pending_tx_msgs--;
			continue;
		}

		if(hsi_channels[chno].state != HSI_CH_BUSY) {
#if MCM_DBG_LOG
			printk("\nmcm:Dropping packets of channel %d from WQ "
					"because channel is closed\n", chno);
#endif
			hsi_mem_free(data->buf);
			hsi_channels[chno].pending_tx_msgs--;
			continue;
		}
#endif
		hsi_channels[chno].write_happening = HSI_TRUE;
		data->being_used = HSI_TRUE;
		err = hsi_ll_write(chno, (unsigned char *)data->buf, data->size);
		if (err < 0) {
#if MCM_DBG_ERR_LOG
			printk("\nmcm: hsi_ll_write failed\n");
#endif

/* Should free in error case */
#if 1
			hsi_mem_free(data->buf);
#endif

			hsi_channels[chno].write_happening = HSI_FALSE;
		} else {
#if MCM_DBG_LOG
			printk("\nmcm:locking mutex start for ch: %d\n",chno);
#endif

#if defined(MIPI_HSI_NET_WRITE_TIMEOUT_FEATURE)
			err = hsi_ch_write_timeout(chno, data->buf);

#if MCM_DBG_LOG
			if (err < 0)
				printk("\nmcm:hsi_ch_write_timeout ret %d for ch: %d\n", err, chno);
#endif

#else 
			wait_event(hsi_channels[chno].write_wait,
						hsi_channels[chno].write_happening == HSI_FALSE);

#if MCM_DBG_LOG
			printk("\nmcm:unlocking mutex end for ch: %d\n",chno);
#endif

#endif
		}
		
		hsi_channels[chno].pending_tx_msgs--;
		data->being_used = HSI_FALSE;

		if (hsi_channels[chno].tx_blocked == 1) {
			hsi_channels[chno].tx_blocked = 0;
#if MCM_DBG_LOG
			printk("\nmcm: Channel queue free , restarting TX queue for ch %d \n",chno);
#endif
			rmnet_restart_queue(chno);
		}
	}

#if 0
/* RIL recovery memory initialization  */
quit_write_wq:
#endif
	hsi_channels[chno].write_queued = HSI_FALSE;
}

#if defined (HSI_LL_ENABLE_RX_BUF_RETRY_WQ)
static void hsi_buf_retry_work(struct work_struct *work)
{
	struct hsi_ll_rx_tx_data temp_data;
	struct hsi_channel *ch = (struct hsi_channel*) container_of(work,
													struct hsi_channel,
													buf_retry_work);
	int chno = ch->info->chno;
	temp_data.size = ch->pending_rx_size;
	temp_data.buffer = NULL;

	if (temp_data.size == 0) {
#if MCM_DBG_ERR_LOG
		printk("\nHSI_LL: hsi_buf_retry_work fail(size=0) in retry Q for ch %d\n", chno);
#endif
		return;
	}

	/*GFP_NOFAIL not available so switching to while loop*/
	while(temp_data.buffer == NULL) {

		if (hsi_mcm_state == HSI_MCM_STATE_ERR_RECOVERY)
		{

#if MCM_DBG_ERR_LOG
			printk("\nHSI_LL: hsi_buf_retry_work fail1. hsi_mcm_state %d retry Q for ch %d\n", hsi_mcm_state, chno);
#endif
			break;
		}

#if 0		
		temp_data.buffer = kmalloc(temp_data.size,
								   GFP_DMA | GFP_KERNEL);
#else
		temp_data.buffer = hsi_mem_alloc(temp_data.size);
#endif

		if(temp_data.buffer == NULL)
			msleep_interruptible(10);
	}

	if(temp_data.buffer == NULL) {
#if MCM_DBG_ERR_LOG
		printk("\nHSI_LL: hsi_buf_retry_work NULL in retry Q for ch %d\n", chno);
#endif
		ch->pending_rx_size = 0;
		return;
	}
	
#if 1 //MCM_DBG_LOG
	printk("\nHSI_LL: Allocating mem(size=%d) in retry Q for ch %d\n",
			temp_data.size, chno);
#endif

	if(0 > hsi_ll_ioctl(chno, HSI_LL_IOCTL_RX_RESUME, &temp_data)) {
		if(temp_data.buffer) {
#if MCM_DBG_ERR_LOG
		printk("\nHSI_LL: HSI_LL_IOCTL_RX_RESUME fail for ch %d\n", chno);
#endif

#if 0
			kfree(temp_data.buffer);
#else
			hsi_mem_free(temp_data.buffer);
#endif
		}
	}
	ch->pending_rx_size = 0;
}
#endif

void hsi_ch_cb(unsigned int chno, int result, int event, void* arg)
{
	ll_rx_tx_data *data = (ll_rx_tx_data *) arg;

	if (!(chno <= MAX_HSI_CHANNELS && chno >= 0) ||
		hsi_channels[chno].state == HSI_CH_NOT_USED) {
#if MCM_DBG_ERR_LOG
		printk("\nmcm: Wrong channel number or channel not used\n");
#endif
		return;
	}

	switch(event) {
	case HSI_LL_EV_ALLOC_MEM: {
		if(chno >= 13) {
			if (hsi_channels[chno].pending_rx_msgs >= NUM_X_BUF) {
				data->buffer = NULL;
#if !defined (HSI_LL_ENABLE_RX_BUF_RETRY_WQ)
#if MCM_DBG_ERR_LOG
				printk("\nmcm: Channel %d RX queue is full so sending NAK to CP\n",
						chno);
#endif
#else
				hsi_channels[chno].pending_rx_size = data->size;
				hsi_channels[chno].rx_blocked = 1;
#if MCM_DBG_ERR_LOG
				printk("\nmcm: Channel %d RX queue is full so rx_blocked\n", chno);
#endif
#endif
				return;
			} else {
				hsi_channels[chno].pending_rx_msgs++;
			}
		}

#if MCM_DBG_LOG
		printk("\nmcm: Allocating read memory of size %d to channel %d \n",
					data->size, chno);
#endif
		/* MODEM can't handle NAK so we allocate memory and
			drop the packet after recieving from MODEM */
#if 0
		spin_lock_bh(&hsi_channels[chno].lock);
		if (hsi_channels[chno].state == HSI_CH_FREE) {
			spin_unlock_bh(&hsi_channels[chno].lock);
#if MCM_DBG_ERR_LOG
			printk("\nmcm: channel not yet opened so not allocating memory\n");
#endif
			data->buffer = NULL;
			break;
		}
		spin_unlock_bh(&hsi_channels[chno].lock);
#endif
		data->buffer = (char *)hsi_mem_alloc(data->size);

#if defined (HSI_LL_ENABLE_RX_BUF_RETRY_WQ)
		if(data->buffer == NULL) {
#if MCM_DBG_ERR_LOG
			printk("\nmcm: Channel %d hsi_mem_alloc NULL so set block mode\n",
								chno);
#endif
			hsi_channels[chno].pending_rx_size = data->size;
			PREPARE_WORK(&hsi_channels[chno].buf_retry_work,
						 hsi_buf_retry_work);
			queue_work(hsi_buf_retry_wq,
					   &hsi_channels[chno].buf_retry_work);
		}
#endif
		}
		break;

	case HSI_LL_EV_FREE_MEM: {
#if MCM_DBG_LOG
		printk("\nmcm: Freeing memory for channel %d, ptr = 0x%p \n",
					chno,data->buffer);
#endif
		spin_lock_bh(&hsi_channels[chno].lock);
		if (hsi_channels[chno].state == HSI_CH_FREE) {
			spin_unlock_bh(&hsi_channels[chno].lock);
#if MCM_DBG_ERR_LOG
			printk("\nmcm: channel not yet opened so cant free mem\n");
#endif
			return;
			}
		spin_unlock_bh(&hsi_channels[chno].lock);
		hsi_mem_free(data->buffer);
		}
		break;

	case HSI_LL_EV_RESET_MEM:
		/* if event is break, handle it somehow. */
		break;

	case HSI_LL_EV_WRITE_COMPLETE: {
#if MCM_DBG_LOG
		printk("\nmcm:unlocking mutex for ch: %d\n",chno);
#endif

/* Uplink Throughput issue */
#if 1
		hsi_mem_free(data->buffer);
#endif
		hsi_channels[chno].write_happening = HSI_FALSE;
		wake_up(&hsi_channels[chno].write_wait);
/* Uplink Throughput issue */
#if 0
		hsi_mem_free(data->buffer);
#endif

#if MCM_DBG_LOG
		printk("\nmcm: write complete cb, ch %d\n",chno);
#endif
		}
		break;

	case HSI_LL_EV_READ_COMPLETE: {
		int n = 0;
#if MCM_DBG_LOG
		printk("\nmcm: Read complete... size %d, channel %d, ptr = 0x%p \n",
					data->size, chno,data->buffer);
#endif
		spin_lock_bh(&hsi_channels[chno].lock);
		if (hsi_channels[chno].state == HSI_CH_FREE) {
			if(chno >= 13) {
				hsi_channels[chno].pending_rx_msgs--;
			}
			spin_unlock_bh(&hsi_channels[chno].lock);
#if MCM_DBG_ERR_LOG
			printk("\nmcm: channel %d not yet opened so dropping the packet\n",chno);
#endif
			hsi_mem_free(data->buffer);
#if defined (HSI_LL_ENABLE_RX_BUF_RETRY_WQ)
			if(hsi_channels[chno].rx_blocked) {
				hsi_channels[chno].rx_blocked = 0;
				spin_lock_bh(&hsi_channels[chno].lock);
				hsi_channels[chno].pending_rx_msgs++;
				spin_unlock_bh(&hsi_channels[chno].lock);
				PREPARE_WORK(&hsi_channels[chno].buf_retry_work, hsi_buf_retry_work);
				queue_work(hsi_buf_retry_wq, &hsi_channels[chno].buf_retry_work);
			}
#endif
			return;
		}

		n = write_q(&hsi_channels[chno].rx_q, data->buffer, data->size, NULL);

		spin_unlock_bh(&hsi_channels[chno].lock);

		if (n == 0) {
#if MCM_DBG_ERR_LOG
			printk("\nmcm: Dropping the packet as channel %d is busy sending already read data\n",chno);
#endif
			hsi_mem_free(data->buffer);
#if 1
			/* Schedule work Q to send data to upper layers */
			if (hsi_channels[chno].read_queued == HSI_TRUE) {
#if MCM_DBG_LOG
				printk("\nmcm: read wq already in progress\n");
#endif
			}
			else {
			PREPARE_WORK(&hsi_channels[chno].read_work, hsi_read_work);
			queue_work(hsi_read_wq, &hsi_channels[chno].read_work);
			}
#endif
		} else if (n == 1) {
			if (hsi_channels[chno].read_happening == HSI_FALSE) {
				hsi_channels[chno].read_happening = HSI_TRUE;
			}
			PREPARE_WORK(&hsi_channels[chno].read_work, hsi_read_work);
			queue_work(hsi_read_wq, &hsi_channels[chno].read_work);
		}
		/* if n > 1, no need to schdule the wq again. */
		}
		break;
	default:
		/* Wrong event. */
#if MCM_DBG_ERR_LOG
		printk("\nmcm:Wrong event.ch %d event %d", chno, event);
#endif
		break;
	}
}

void xmd_ch_register_xmd_boot_cb(void (*fn)(void))
{
	xmd_boot_cb = fn;
}

void __init xmd_ch_init(void)
{
	int i;
	int size = ARRAY_SIZE(hsi_all_channels);

#if MCM_DBG_LOG
	printk("\nmcm: xmd_ch_init++\n");
#endif

	for (i=0; i<size; i++) {
		hsi_channels[i].state = hsi_all_channels[i].state;
		hsi_channels[i].name = hsi_all_channels[i].name;
		hsi_channels[i].write_happening = HSI_FALSE;
		hsi_channels[i].write_queued = HSI_FALSE;
		hsi_channels[i].read_queued = HSI_FALSE;
		hsi_channels[i].read_happening = HSI_FALSE;
		spin_lock_init(&hsi_channels[i].lock);
		init_waitqueue_head(&hsi_channels[i].write_wait);
		init_waitqueue_head(&hsi_channels[i].read_wait);
		init_q(i);
	}

	hsi_mem_init();

	/* TI HSI driver (from HSI_DRIVER_VERSION 0.4.2) can suppport port 1 and 2, 
		but IMC XMD currently supports port 1 only */
	hsi_ll_init(XMD_SUPPORT_PORT, hsi_ch_cb);

/* Create and initialize work q */
#if 1
	hsi_read_wq = create_singlethread_workqueue("hsi-read-wq");
	hsi_write_wq = create_singlethread_workqueue("hsi-write-wq");
#if defined (HSI_LL_ENABLE_RX_BUF_RETRY_WQ)
	hsi_buf_retry_wq = create_singlethread_workqueue("hsi_buf_retry_wq");
#endif

#else
	hsi_read_wq = create_workqueue("hsi-read-wq");
	hsi_write_wq = create_workqueue("hsi-write-wq");
#if defined (HSI_LL_ENABLE_RX_BUF_RETRY_WQ)
	hsi_buf_retry_wq = create_workqueue("hsi_buf_retry_wq");
#endif
#endif
	INIT_WORK(&XMD_DLP_RECOVERY_wq, xmd_dlp_recovery_wq);

#if defined (ENABLE_RECOVERY_WAKE_LOCK)
	wake_lock_init(&xmd_recovery_wake_lock, WAKE_LOCK_SUSPEND, "xmd-recovery-wake");
#endif

	hsi_mcm_state = HSI_MCM_STATE_INITIALIZED;
}

void xmd_ch_exit(void)
{
#if defined (HSI_LL_ENABLE_RX_BUF_RETRY_WQ)
	flush_workqueue(hsi_buf_retry_wq);
#endif
	flush_workqueue(hsi_read_wq);
	destroy_workqueue(hsi_read_wq);
	flush_workqueue(hsi_write_wq);
	destroy_workqueue(hsi_write_wq);
	hsi_ll_shutdown();
	hsi_mcm_state = HSI_MCM_STATE_UNDEF;

#if defined (ENABLE_RECOVERY_WAKE_LOCK)
	wake_lock_destroy(&xmd_recovery_wake_lock);
#endif	
}

int xmd_is_recovery_state(void)
{
	if(hsi_mcm_state == HSI_MCM_STATE_ERR_RECOVERY) {		
		return 1;
	}

	return 0;		
}

extern void rmnet_sync_down_for_recovery(void);
int xmd_ch_reset(void)
{
	int ch_i;
	int size = ARRAY_SIZE(hsi_all_channels);

#if defined (ENABLE_RECOVERY_WAKE_LOCK)
	wake_lock_timeout(&xmd_recovery_wake_lock, RECOVERY_WAKELOCK_TIME);
#endif

#if 1
	rmnet_sync_down_for_recovery();
#endif

#if defined(CONFIG_MACH_LGE_COSMOPOLITAN)
	/*Modem Reset */
	ifx_pmu_reset();
#endif

#if 0 //MCM_DBG_ERR_RECOVERY_LOG
	printk("\nmcm: HSI DLP Error Recovery initiated.\n");
#endif

	for (ch_i=0; ch_i < size; ch_i++) {
		if (hsi_channels[ch_i].write_happening == HSI_TRUE) {
			hsi_channels[ch_i].write_happening = HSI_FALSE;
			wake_up(&hsi_channels[ch_i].write_wait);
		}
	}

#if defined (HSI_LL_ENABLE_RX_BUF_RETRY_WQ)
	flush_workqueue(hsi_buf_retry_wq);
#endif

	flush_workqueue(hsi_write_wq);
	hsi_ll_reset(HSI_LL_RESET_RECOVERY);
	flush_workqueue(hsi_read_wq);

	for (ch_i=0; ch_i < size; ch_i++) {
		init_q(ch_i);
	}

	hsi_mem_reinit();
	
	hsi_ll_restart();

#if defined(CONFIG_MACH_LGE_COSMOPOLITAN)
	/* TODO: Fine tune to required value. */
	msleep(6000);
#endif

#if MCM_DBG_ERR_RECOVERY_LOG
	printk("\nmcm: HSI DLP Error Recovery completed waiting for CP ready indication from RIL.\n");
#endif

	/* Change MCM state to initilized when CP ready
		indication from tty ctrl channel is issued */

	return 0;
}

int xmd_is_dlp_recovery_finish(void)
{
	if(is_dlp_reset_in_progress == 1) {
#if MCM_DBG_ERR_RECOVERY_LOG
		printk("\nmcm: xmd_is_dlp_recovery_finish. Error is_dlp_reset_in_progress.\n");
#endif
		return 0;
	}

	/**************************************************
		LGE-RIL CHANNEL : 1 , 2, 3, 4, 5, 8, 9,(11: KR SKT),12
	**************************************************/
	else if((hsi_channels[1].state != HSI_CH_FREE)||(hsi_channels[2].state != HSI_CH_FREE)
	||(hsi_channels[3].state != HSI_CH_FREE)||(hsi_channels[4].state != HSI_CH_FREE)
	||(hsi_channels[5].state != HSI_CH_FREE)||(hsi_channels[8].state != HSI_CH_FREE)
#if defined(CONFIG_MACH_LGE_COSMO_DOMASTIC)			
	||(hsi_channels[11].state != HSI_CH_FREE)
#endif			
	||(hsi_channels[9].state != HSI_CH_FREE)||(hsi_channels[12].state != HSI_CH_FREE)) {
#if MCM_DBG_ERR_RECOVERY_LOG
		printk("\nmcm: xmd_is_dlp_recovery_finish. Error HSI_CH_FREE.\n");
#endif
		return 0;
	}

	return 1;
}

/* Do not start xmd_ch_reinit during recovery processing */
static int xmd_ch_reinit(int chno)
{
	if(hsi_mcm_state == HSI_MCM_STATE_ERR_RECOVERY) {
		if(xmd_is_dlp_recovery_finish() == 0) {
#if MCM_DBG_ERR_RECOVERY_LOG
			printk("\nmcm: Recovery not completed. Error reinit by chno %d.\n", chno);
#endif
			return -ENOTBLK;
		}
		else {
#if MCM_DBG_ERR_RECOVERY_LOG
			printk("\nmcm: Recovery completed and reinit by chno %d.\n", chno);
#endif
			hsi_mcm_state = HSI_MCM_STATE_INITIALIZED;
		}
	}

	return 0;
}

void xmd_dlp_recovery(int chno)
{
	if(is_dlp_reset_in_progress == 0) {
		if( hsi_mcm_state == HSI_MCM_STATE_ERR_RECOVERY) {
#if 0 //MCM_DBG_ERR_RECOVERY_LOG
			printk("\nmcm: xmd_ch_reset already HSI_MCM_STATE_ERR_RECOVERY in progress\n");
#endif
			return;
		}

#if MCM_DBG_ERR_RECOVERY_LOG
		printk("\nmcm: Ch %d closed so starting Recovery.\n", chno);
#endif

		hsi_mcm_state = HSI_MCM_STATE_ERR_RECOVERY;

		is_dlp_reset_in_progress = 1;

		schedule_work(&XMD_DLP_RECOVERY_wq);
	}
#if 0 //MCM_DBG_ERR_RECOVERY_LOG	
	else
	{
		printk("\nmcm: Ch %d closed, but xmd_dlp_recovery already in progress\n", chno);
	}
#endif	
}

static void xmd_dlp_recovery_wq(struct work_struct *cp_crash_wq)
{
	xmd_ch_reset(); /* Start MCM/DLP cleanup */
	is_dlp_reset_in_progress = 0;
}

#if defined(CONFIG_MACH_LGE_COSMOPOLITAN)
void xmd_set_ifx_cp_dump(void)
{
	hsi_mcm_state = HSI_MCM_STATE_ERR_RECOVERY;

#if MCM_DBG_ERR_RECOVERY_LOG
	printk("\nmcm: xmd_set_ifx_cp_dump.\n");
#endif

}
#endif

