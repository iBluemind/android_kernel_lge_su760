#include "tcpal_os.h"
#include "tcpal_debug.h"

#include <linux/module.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>

#include <asm/io.h>
#include <asm/mach-types.h>

#include "broadcast_tcc3170.h"

TcpalTime_t TcpalGetCurrentTimeMs (void)
{
    TcpalTime_t tick;
    struct timeval tv;

    do_gettimeofday(&tv);
    tick = (TcpalTime_t)tv.tv_sec*1000 + tv.tv_usec/1000;

    return tick;
}

TcpalTime_t TcpalGetTimeIntervalMs (TcpalTime_t _startTime)
{
    TcpalTime_t interval;
    TcpalTime_t currTime = TcpalGetCurrentTimeMs();

    if(currTime > _startTime)
        interval = currTime - _startTime;
    else 
        interval = (TCPAL_MAX_TIMECNT - _startTime) + currTime + 1;

    return interval;
}

void TcpalSleep(I32S _ms)
{
    tdmb_tcc3170_mdelay(_ms);
}

void *TcpalMemoryAllocation(I32U _size)
{
    return kmalloc(_size, GFP_KERNEL);
}

void TcpalMemoryFree(void *_ptr)
{
    kfree(_ptr);
}

void* TcpalMemorySet(void* _dest, I32U _data, I32U _cnt)
{
    return memset(_dest, _data, (size_t)_cnt);
}

void* TcpalMemoryCopy(void* _dest, const void* _src, I32U _cnt)
{
    return memcpy(_dest, _src, (size_t)_cnt);
}

I32S TcpalMemoryCompare(void* _s1, const void* _s2, I32U _cnt)
{
    return memcmp(_s1, _s2, (size_t)_cnt);
}

I32S TcpalCreateSemaphore(TcpalSem_t *_semaphore, I08S *_name, I32U _initialCount)
{
    struct mutex *lock = TcpalMemoryAllocation(sizeof(struct mutex));
    TcbdDebug(DEBUG_TCPAL_OS, "\n");
    mutex_init(lock);
    
    *_semaphore = (TcpalSem_t)lock;

    return 0;
}

I32S TcpalDeleteSemaphore(TcpalSem_t *_semaphore)
{
    struct mutex *lock = (struct mutex*)*_semaphore;
    
    if(lock == NULL) return -1;

    TcpalMemoryFree(lock);
    *_semaphore = 0;
    TcbdDebug(DEBUG_TCPAL_OS, "\n");

    return 0;
}

I32S TcpalSemaphoreLock(TcpalSem_t *_semaphore)
{
    struct mutex *lock = (struct mutex*)*_semaphore;
    if(lock == NULL) 
    {
        TcbdDebug(DEBUG_ERROR, "\n");
        return -1;
    }
    mutex_lock(lock);
    
    return 0;
}

I32S TcpalSemaphoreUnLock(TcpalSem_t *_semaphore)
{
    struct mutex *lock = (struct mutex*)*_semaphore;

    if(lock == NULL) 
    {
        TcbdDebug(DEBUG_ERROR, "\n");
        return -1;
    }
    
    mutex_unlock(lock);
    return 0;
}
