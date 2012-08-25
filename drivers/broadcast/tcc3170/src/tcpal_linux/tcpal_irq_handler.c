
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>

#include <asm/io.h>
#include <asm/mach-types.h>

#include "broadcast_tcc3170.h"

#include "tcpal_os.h"
#include "tcpal_debug.h"

#include "tcbd_feature.h"
#include "tcbd_api_common.h"

#include "tcbd_stream_parser.h"
#include "tcbd_diagnosis.h"
#include "tcbd_hal.h"

#define __WORKQUEUE__

typedef struct _TcbdIrqData_t
{
    struct work_struct work;
    struct workqueue_struct *workQueue;
    struct _TcbdDevice_t *device;
    int tcbd_irq;
    TcpalTime_t startTick;
} TcbdIrqData_t;

static TcbdIrqData_t TcbdIrqData;
static I08U StreamBuffer[1024*8];

extern int broadcast_drv_if_push_msc_data(unsigned char *buffer_ptr, unsigned int buffer_size);
extern int tunerbb_drv_tcc3170_put_fic(unsigned char * buffer, unsigned int buffer_size);

extern struct spi_device*	TCC_GET_SPI_DRIVER(void);

inline static void TcbdParseStream(TcbdIrqData_t * irqData)
{
    I32S size, ret = 0;
    I08S irqStatus;
    I08S irqError;
    TcbdDevice_t *device = irqData->device;

    ret = TcbdReadIrqStatus(device, &irqStatus, &irqError);
    ret |= TcbdReadStream(device, StreamBuffer, &size);
    if(ret == 0 && !irqError)
    {
        TcbdParser(StreamBuffer, size);
    }
    else
    {
        printk("======================================================================\n"
               "### buffer is full, skip the data (status=0x%02X, error=0x%02X, %d)  ###\n" 
               "======================================================================\n",
               irqStatus, irqError, (int)TcpalGetTimeIntervalMs(irqData->startTick));
        TcbdInitParser(NULL);
    }
    ret |= TcbdClearIrq(device, irqStatus);
}

#if defined(__WORKQUEUE__)
static void TcbdStreamParsingWork(struct work_struct *_param)
{
    struct _TcbdIrqData_t * irqData = container_of(_param, struct _TcbdIrqData_t, work);

    TcbdParseStream(irqData);
    enable_irq(irqData->tcbd_irq);
}
#endif //__WORKQUEUE__

static irqreturn_t TcbdIrqHandler(int _irq, void *_param)
{
#if defined(__WORKQUEUE__)
    queue_work(TcbdIrqData.workQueue, &TcbdIrqData.work);
    TcbdIrqData.startTick = TcpalGetCurrentTimeMs(); 
    disable_irq_nosync(TcbdIrqData.tcbd_irq);
#else  //__WORKQUEUE__
    TcbdParseStream(&TcbdIrqData);
#endif //!__WORKQUEUE__

    return IRQ_HANDLED;
}

static I32S TcbdStreamCallback(I08U *_stream, I32S _size, I08U _subchId, I08U _type)
{
    I08U *stream = NULL;
    TDMB_BB_HEADER_TYPE *lgeHeader;
    switch(_type)
    {
        case 0:
            stream = (_stream - sizeof(TDMB_BB_HEADER_TYPE));
            lgeHeader = (TDMB_BB_HEADER_TYPE*)stream;
            lgeHeader->size = _size;
            lgeHeader->subch_id = _subchId;
            broadcast_drv_if_push_msc_data(stream, _size+sizeof(TDMB_BB_HEADER_TYPE));
            break;
        case 1:
            tunerbb_drv_tcc3170_put_fic(_stream, _size);
            break;
        case 2:
            TcbdUpdateStatus(_stream, NULL);
            break;
        default:
            break;
    }
    return 0;
}

I32S TcpalRegisterIrqHandler(TcbdDevice_t *_device)
{
    struct spi_device *spi = TCC_GET_SPI_DRIVER();
#if defined(__WORKQUEUE__)
    TcbdIrqData.workQueue = create_workqueue("TcbdStreamParsingWork");
    TcbdIrqData.device = _device;
    TcbdIrqData.tcbd_irq = spi->irq;

    INIT_WORK(&TcbdIrqData.work, TcbdStreamParsingWork);
#endif //__WORKQUEUE__
    TcbdInitParser(TcbdStreamCallback);

    TchalIrqSetup();

    printk("[%s:%d] irq:%d\n", __func__, __LINE__, TcbdIrqData.tcbd_irq);
    return  request_irq(TcbdIrqData.tcbd_irq, TcbdIrqHandler,
        IRQF_TRIGGER_FALLING |IRQF_TRIGGER_LOW, "tc317x_stream", &TcbdIrqData);
}

I32S TcpalUnRegisterIrqHandler(void)
{
    disable_irq(TcbdIrqData.tcbd_irq);
    destroy_workqueue(TcbdIrqData.workQueue);

    return 0;
}

I32S TcpalIrqEnable(void)
{
    enable_irq(TcbdIrqData.tcbd_irq);
    return 0;
}

I32S TcpalIrqDisable(void)
{
    disable_irq(TcbdIrqData.tcbd_irq);
    return 0;
}
