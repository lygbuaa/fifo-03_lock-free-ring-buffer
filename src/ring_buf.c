/**
 * @file    hlgih_sal_ringbuf.c
 * @brief   Provides an ringbuf template
 * @details Provides an ringbuf template
 * @author  lygbuaa@gmail.com
 * @version 0.1.0
 * @date    2023-07-04
 * @copyright Copyright (C) 2018-2023, Hugo-Liu-Gmail. All rights reserved.
 *
 * @par changes:
 * <table>
 * <tr><th>Date        <th>Version  <th>Author    <th>Description
 * <tr><td>2023/07/01  <td>0.1.0    <td>Hugo Liu  <td>create initial version
 * </table>
 *
*/

#include <linux/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include "ring_buf.h"

#ifndef LOGPF
#define LOGPF(format, ...) fprintf(stderr ,"[%s:%d] " format "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

/** if set true, allow overwrite on the unread cells */
#define HLGIH_SAL_RINGBUF_ALLOW_OVERWRITE   (false)

// typedef uint32_t ringbuf_index_t;
/** 
 * An integer type which can be accessed as an atomic entity even in the presence of asynchronous interrupts made by signals. 
 * Reading and writing this data type is guaranteed to happen in a single instruction
 */
typedef sig_atomic_t ringbuf_index_t;

/** A ring buffer which supports only one producer and one consumer */
struct SAL_RINGBUF_S
{
    SAL_RINGBUF_CELL_HANDLE buf;            /**< pointer to the start of the ring buffer */
    ringbuf_index_t         end;            /**< offset of the end of the ring buffer */
    ringbuf_index_t         head;           /**< offset to where next cell will be write */
    ringbuf_index_t         tail;           /**< offset of where the cell has done read */
    ringbuf_index_t         rdpos;          /**< offset of where next cell will be read */
    pthread_mutex_t         rdmtx;          /**< read mutex */
    pthread_mutex_t         wtmtx;          /**< write mutex */
    pthread_cond_t          cond;           /**< notify the other worker until some condition is true */
    bool                    write_locking;  /**< indicates whether producer is locking the ring buffer */
    bool                    read_locking;   /**< indicates whether consumer is locking the ring buffer */
};

bool HLGIH_SAL_RingBuf_Create(SAL_RINGBUF_HANDLE* rb, const uint32_t len, const uint32_t payload_size)
{
    if((NULL == rb) || (len < 2) || (len > HLGIH_SAL_RINGBUF_MAX_LENGTH) || (payload_size < 1) || (payload_size > HLGIH_SAL_RINGBUF_MAX_PAYLOAD_SIZE))
    {
        LOGPF("invalid argument");
        *rb = NULL;
        return false;
    }

    *rb = (SAL_RINGBUF_HANDLE)malloc(sizeof(struct SAL_RINGBUF_S));
    if(NULL == *rb)
    {
        LOGPF("malloc ringbuf failed");
        return false;
    }

    SAL_RINGBUF_HANDLE const self = *rb;
    self->head = 0;
    self->tail = 0;
    self->rdpos = 0;
    self->end = 0;

    /** allocate ringbuf cells */
    self->buf = (SAL_RINGBUF_CELL_HANDLE)calloc(len, sizeof(SAL_RINGBUF_CELL_S));
    if(NULL == self->buf)
    {
        LOGPF("malloc ringbuf cells failed");
        return false;
    }

    self->end = len;
    self->rdmtx = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    self->wtmtx = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    self->cond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
    pthread_condattr_t attr;
    pthread_condattr_init(&attr);
    /** use CLOCK_MONOTONIC instead of CLOCK_REALTIME, in case of UTC time jitter */
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    pthread_cond_init(&(self->cond), &attr);
    self->write_locking = false;
    self->read_locking = false;

    /** allocate payloads, set NULL if failed */
    uint32_t errs = 0;
    for(uint32_t i=0; i<len; i++)
    {
        SAL_RINGBUF_CELL_HANDLE pcell = &(self->buf[i]);
        pcell->payload = (ringbuf_payload_t*)malloc(payload_size);
        if(NULL == pcell->payload) 
        {
            LOGPF("malloc ringbuf cells payload [%d] failed", i);
            errs += 1;
            pcell->size = 0U;
            pcell->len = 0U;
            pcell->magic = 0U;
            pcell->timestamp = 0;
        }
        else
        {
            pcell->magic = HLGIH_SAL_RINGBUF_CELL_MAGIC_NUM;
            pcell->size = payload_size;
            pcell->len = 0U;
            pcell->timestamp = 0; 
        }
    }

    return (0 == errs);
}

bool HLGIH_SAL_RingBuf_Release(SAL_RINGBUF_HANDLE const self)
{
    if(NULL != self)
    {
        if(NULL != self->buf)
        {
            for(int i=0; i<self->end; i++)
            {
                SAL_RINGBUF_CELL_S cell = self->buf[i];
                if(NULL != cell.payload)
                {
                    free(cell.payload);
                }
            }
            free(self->buf);
        }
        free(self);
    }
    return true;
}


bool HLGIH_SAL_RingBuf_IsEmpty(SAL_RINGBUF_HANDLE const self)
{
    // return (self->head == self->tail);
    return (self->head == self->rdpos);
}

bool HLGIH_SAL_RingBuf_IsFull(SAL_RINGBUF_HANDLE const self)
{
    ringbuf_index_t head = self->head + 1U;
    if (head == self->end)
    {
        head = 0U;
    }
    return (head == self->tail);
}

bool HLGIH_SAL_RingBuf_Clear(SAL_RINGBUF_HANDLE const self)
{
    // pthread_mutex_lock(&(self->rdmtx));
    self->tail = self->head;
    self->rdpos = self->head;
    // pthread_mutex_unlock(&(self->rdmtx));
}

void HLGIH_SAL_RingBuf_HeadInc(SAL_RINGBUF_HANDLE const self)
{
    ringbuf_index_t head = self->head + 1U;
    if (head == self->end)
    {
        head = 0U;
    }
    /** update the head to next index */
    self->head = head;
}

void HLGIH_SAL_RingBuf_TailInc(SAL_RINGBUF_HANDLE const self)
{
    ringbuf_index_t tail = self->tail;
    ++tail;
    if (tail == self->end)
    {
        tail = 0U;
    }
    /** update the tail to next index */
    self->tail = tail;
}

void HLGIH_SAL_RingBuf_RdposInc(SAL_RINGBUF_HANDLE const self)
{
    ringbuf_index_t rdpos = self->rdpos;
    ++rdpos;
    if (rdpos == self->end)
    {
        rdpos = 0U;
    }
    /** update the rdpos to next index */
    self->rdpos = rdpos;
}

bool HLGIH_SAL_RingBuf_GetWriteCell(SAL_RINGBUF_HANDLE const self, const int64_t timeout_us, SAL_RINGBUF_CELL_HANDLE* const el)
{
    if(NULL == self || NULL == el)
    {
        LOGPF("invalid argument");
        return false;
    }

    /** if ringbuf is full, wait untill read operation done, or reach timeout */
    if(HLGIH_SAL_RingBuf_IsFull(self))
    {
        /** if timeout <=0, return NULL immediately */
        if(timeout_us <= 0)
        {
            LOGPF("ringbuf is full");
            *el = NULL;
            return false; 
        }
        /** avoid multi producer dead-locking */
        if(self->write_locking)
        {
            LOGPF("please call ReturnWriteCell first.");
            *el = NULL;
            return false; 
        }
        struct timespec now;
        /** use CLOCK_MONOTONIC instead of CLOCK_REALTIME, in case of UTC time sync */
        clock_gettime(CLOCK_MONOTONIC, &now);
        int64_t timeout_sec_part = timeout_us / 1000000U;
        int64_t timeout_nsec_part = (timeout_us - (timeout_sec_part * 1000000U)) * 1000U;
        now.tv_sec += timeout_sec_part;
        now.tv_nsec += timeout_nsec_part;
        // LOGPF("HLGIH_SAL_RingBuf_GetWriteCell timeout: %ld.%ld", timeout_sec_part, timeout_nsec_part);
        // LOGPF("write lock-1");
        pthread_mutex_lock(&(self->wtmtx));
        // LOGPF("write lock-2");
        self->write_locking = true;
        pthread_cond_timedwait(&(self->cond), &(self->wtmtx), &now);

        /** check status again, if still full, there is no need to call RingBuf_ReturnWriteCell() */
        if(HLGIH_SAL_RingBuf_IsFull(self))
        {
            *el = NULL;
            self->write_locking = false;
            // LOGPF("write unlock-1");
            pthread_mutex_unlock(&(self->wtmtx));
            // LOGPF("write unlock-2");
            LOGPF("ringbuf is full after timeout %d(us)", timeout_us);
            return false; 
        }
        /** the head is still locking, remember unlock the mutex when calling RingBuf_ReturnWriteCell() */
        else
        {
            *el = &(self->buf[self->head]);
            // LOGPF("HLGIH_SAL_RingBuf_GetWriteCell return addr: %x", *el);
            return true;
        }
    }
    else
    {
        *el = &(self->buf[self->head]);
        // LOGPF("HLGIH_SAL_RingBuf_GetWriteCell return addr: %x", *el);
        return true;  
    }

}

bool HLGIH_SAL_RingBuf_ReturnWriteCell(SAL_RINGBUF_HANDLE const self, SAL_RINGBUF_CELL_HANDLE const el)
{
    if(NULL == self || NULL == el)
    {
        LOGPF("invalid argument");
        return false;
    }

    HLGIH_SAL_RingBuf_HeadInc(self);

    /** unlock mutex */
    if(self->write_locking)
    {
        self->write_locking = false;
        // LOGPF("write return lock-1");
        pthread_mutex_unlock(&(self->wtmtx));
        // LOGPF("write return lock-2");
    }

    /** notify the consumer */
    if(self->read_locking)
    {
        // LOGPF("write notify read-1");
        pthread_mutex_lock(&(self->rdmtx));
        pthread_cond_signal(&(self->cond));
        pthread_mutex_unlock(&(self->rdmtx));
        // LOGPF("write notify read-2");
    }
    return true;
}

bool HLGIH_SAL_RingBuf_GetReadCell(SAL_RINGBUF_HANDLE const self, const int64_t timeout_us, SAL_RINGBUF_CELL_HANDLE* const el)
{
    if(NULL == self || NULL == el)
    {
        LOGPF("invalid argument");
        return false;
    }

    /** if ringbuf is empty, wait untill write operation done, or reach timeout */
    if(HLGIH_SAL_RingBuf_IsEmpty(self))
    {
        /** if timeout <=0, return NULL immediately */
        if(timeout_us <= 0)
        {
            *el = NULL;
            LOGPF("ringbuf is empty");
            return false;
        }
        /** avoid multi consumer dead-locking */
        if(self->read_locking)
        {
            LOGPF("please call ReturnReadCell first.");
            *el = NULL;
            return false; 
        }
        struct timespec now;
        /** use CLOCK_MONOTONIC instead of CLOCK_REALTIME, in case of UTC time jitter */
        clock_gettime(CLOCK_MONOTONIC, &now);
        int64_t timeout_sec_part = timeout_us / 1000000U;
        int64_t timeout_nsec_part = (timeout_us - (timeout_sec_part * 1000000U)) * 1000U;
        now.tv_sec += timeout_sec_part;
        now.tv_nsec += timeout_nsec_part;
        // LOGPF("HLGIH_SAL_RingBuf_GetReadCell timeout: %ld.%ld", timeout_sec_part, timeout_nsec_part);
        // LOGPF("read lock-1");
        pthread_mutex_lock(&(self->rdmtx));
        // LOGPF("read lock-2");
        self->read_locking = true;
        pthread_cond_timedwait(&(self->cond), &(self->rdmtx), &now);

        /** check status again */
        if(HLGIH_SAL_RingBuf_IsEmpty(self))
        {
            *el = NULL;
            self->read_locking = false;
            // LOGPF("read unlock-1");
            pthread_mutex_unlock(&(self->rdmtx));
            // LOGPF("read unlock-2");
            LOGPF("ringbuf is empty after timeout %d(us)", timeout_us);
            return false; 
        }
        else
        {
            // *el = &(self->buf[self->tail]);
            *el = &(self->buf[self->rdpos]);
            HLGIH_SAL_RingBuf_RdposInc(self);
            return true;
        }
    }
    else
    {
        
        // *el = &(self->buf[self->tail]);
        *el = &(self->buf[self->rdpos]);
        HLGIH_SAL_RingBuf_RdposInc(self);
        return true;
    }
}

bool HLGIH_SAL_RingBuf_ReturnReadCell(SAL_RINGBUF_HANDLE const self, SAL_RINGBUF_CELL_HANDLE const el)
{
    if(NULL == self || NULL == el)
    {
        LOGPF("invalid argument");
        return false;
    }

    HLGIH_SAL_RingBuf_TailInc(self);

    /** unlock mutex */
    if(self->read_locking)
    {
        self->read_locking = false;
        // LOGPF("read return lock-1");
        pthread_mutex_unlock(&(self->rdmtx));
        // LOGPF("read return lock-2");
    }

    /** notify the producer */
    if(self->write_locking)
    {
        // LOGPF("read notify write-1");
        pthread_mutex_lock(&(self->wtmtx));
        pthread_cond_signal(&(self->cond));
        pthread_mutex_unlock(&(self->wtmtx));
        // LOGPF("read notify write-2");
    }

    return true;
}


/*..........................................................................*/
bool HLGIH_SAL_RingBuf_GetFreeSlotsCount(SAL_RINGBUF_HANDLE const self, uint32_t* count)
{
    ringbuf_index_t head = self->head;
    ringbuf_index_t tail = self->tail;
    /* if buffer empty? */
    if (head == tail) {
        *count = (uint32_t)(self->end - 1U);
    }
    else if (head < tail) {
        *count = (uint32_t)(tail - head - 1U);
    }
    else {
        *count = (uint32_t)(self->end + tail - head - 1U);
    }

    return true;
}
