#include "tcpal_os.h"
#include "tcpal_debug.h"

#include "tcpal_queue.h"

I32S TcbdQueueIsFull(TcbdQueue_t *_queue)
{
    if( _queue->front == ((_queue->rear+1)%_queue->qsize) )
    {
        return 1;
    }

    return 0;
}

I32S TcbdQueueIsEmpty(TcbdQueue_t *_queue)
{
    if( _queue->front == _queue->rear)
    {
        return 1;
    }
    return 0;
}

void TcbdInitQueue(TcbdQueue_t *_queue, I32S _qsize, I08U *_buffer, I32S _buffSize)
{
    _queue->q            = (TcbdQueueItem_t*)TcpalMemoryAllocation(sizeof(TcbdQueueItem_t) * _qsize);
    _queue->front        = 0;
    _queue->rear         = 0;
    _queue->qsize        = _qsize;
    _queue->buffSize     = _buffSize;
    _queue->globalBuffer = _buffer;

    TcpalCreateSemaphore(&_queue->sem, "TcbdQueue", 0);

    TcbdDebug(1, "buff :0x%X, size:%d\n", _buffer, _buffSize);
}

void TcbdDeinitQueue(TcbdQueue_t *_queue)
{
    _queue->front    = 0;
    _queue->rear     = 0;
    _queue->qsize    = 0;
    _queue->buffSize = 0;

    TcpalMemoryFree((void*)_queue->q);
    TcpalDeleteSemaphore(&_queue->sem);
}

void TcbdResetQueue(TcbdQueue_t *_queue)
{
    _queue->front   = 0;
    _queue->rear    = 0;
    _queue->pointer = 0;
}

I32S TcbdEnqueue(TcbdQueue_t *_queue, I08U *_chunk, I32S _size, I32S _type)
{
    if(_chunk == NULL || _size <= 0)
    {
        TcbdDebug(DEBUG_ERROR, "Invalid argument!! \n");
        return -1;
    }

    TcpalSemaphoreLock(&_queue->sem);
    if(_queue->pointer + _size > _queue->buffSize)
    {
        TcbdDebug(0, "Memory full! \n");
        TcbdResetQueue(_queue);
        TcpalSemaphoreUnLock(&_queue->sem);
        return -1;
    }

    if(TcbdQueueIsFull(_queue))
    {
        TcbdDebug(0, "Queue Full!! \n");
        _queue->pointer = 0;
    }

    TcpalMemoryCopy(_queue->globalBuffer + _queue->pointer, _chunk, _size);

    _queue->q[_queue->rear].buffer = _queue->globalBuffer + _queue->pointer;
    _queue->q[_queue->rear].size   = _size;
    _queue->q[_queue->rear].type   = _type;

    _queue->rear     = (_queue->rear + 1) % _queue->qsize;
    _queue->pointer += _size;
    TcbdDebug(0, "pos:%d, size:%d\n", _queue->pointer, _size);

    TcpalSemaphoreUnLock(&_queue->sem);

    return 0;
}

I32S TcbdDequeue(TcbdQueue_t *_queue, I08U *_chunk, I32S *_size, I32S *_type)
{
    *_size = 0;
    TcpalSemaphoreLock(&_queue->sem);
    if(TcbdQueueIsEmpty(_queue))
    {
        TcbdDebug(0, "Queue Empty!! \n");
        TcpalSemaphoreUnLock(&_queue->sem);        
        return -1;
    }
    else 
    {
        TcbdDebug(0, "Dequeue! size%d\n", _queue->q[_queue->front].size);
    }

    *_size = _queue->q[_queue->front].size;
    if(_type) *_type = _queue->q[_queue->front].type;
    TcpalMemoryCopy(_chunk, _queue->q[_queue->front].buffer, *_size);
    _queue->front = (_queue->front + 1) % _queue->qsize;

    _queue->pointer -= *_size;

    TcbdDebug(0, "pos:%d, size:%d\n", _queue->pointer, *_size);
    TcpalSemaphoreUnLock(&_queue->sem);

    return 0;
}

I32S TcbdDequeuePtr(TcbdQueue_t *_queue, I08U **_chunk, I32S *_size, I32S *_type)
{
    *_size = 0;
    TcpalSemaphoreLock(&_queue->sem);
    
    if(TcbdQueueIsEmpty(_queue))
    {
        TcbdDebug(0, "Queue Empty!! \n");
        TcpalSemaphoreUnLock(&_queue->sem);
        return -1;
    }
    else 
    {
        TcbdDebug(0, "Dequeue! size%d\n", _queue->q[_queue->front].size);
    }
    *_size = _queue->q[_queue->front].size;
    *_chunk = _queue->q[_queue->front].buffer;
    if(_type) *_type = _queue->q[_queue->front].type;

    _queue->front = (_queue->front + 1) % _queue->qsize;
    _queue->pointer -= *_size;

    TcbdDebug(0, "pos:%d, size:%d\n", _queue->pointer, *_size);
    TcpalSemaphoreUnLock(&_queue->sem);

    return 0;
}

I32S TcbdGetFirstQueuePtr(TcbdQueue_t *_queue, I08U **_chunk, I32S *_size, I32S *_type)
{
    TcpalSemaphoreLock(&_queue->sem);

    if(TcbdQueueIsEmpty(_queue))
    {
        TcbdDebug(0, "Queue Empty!! \n");
        TcpalSemaphoreUnLock(&_queue->sem);
        return -1;
    }
    *_size = _queue->q[_queue->front].size;
    *_chunk = _queue->q[_queue->front].buffer;
    if(_type) *_type = _queue->q[_queue->front].type;

    TcbdDebug(0, "pos:%d, size:%d\n", _queue->pointer, *_size);
    TcpalSemaphoreUnLock(&_queue->sem);
    
    return 0;
}

