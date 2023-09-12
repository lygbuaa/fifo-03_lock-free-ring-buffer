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

#include "ring_buf.h"

/*..........................................................................*/
void HLGIH_SAL_RingBuf_Create(SAL_RINGBUF_HANDLE const self, SAL_RINGBUF_CELL_S sto[], ringbuf_index_t sto_len)
{
    self->buf  = &sto[0];
    self->end  = sto_len;
    self->head = 0U;
    self->tail = 0U;
}

// SAL_RINGBUF_HANDLE HLGIH_SAL_RingBuf_Create(ringbuf_index_t size)
// {
//     SAL_RINGBUF_HANDLE = (SAL_RINGBUF_HANDLE) malloc(sizeof(struct SAL_RINGBUF_S));
// }

/*..........................................................................*/
bool HLGIH_SAL_RingBuf_Write(SAL_RINGBUF_HANDLE const self, SAL_RINGBUF_CELL_S const el)
{
    ringbuf_index_t head = self->head + 1U;
    if (head == self->end) {
        head = 0U;
    }
    /** buffer not full? */
    if (head != self->tail) { 
        self->buf[self->head] = el;
        /** update the head to a valid index */
        self->head = head; 
        /** element placed in the buffer */
        return true;  
    }
    else {
        /** element not placed in the buffer */
        return false; 
    }
}

/*..........................................................................*/
bool HLGIH_SAL_RingBuf_Read(SAL_RINGBUF_HANDLE const self, SAL_RINGBUF_CELL_S* const pel)
{
    ringbuf_index_t tail = self->tail;
    /** ring buffer not empty? */
    if (self->head != tail) { 
        *pel = self->buf[tail];
        ++tail;
        if (tail == self->end) {
            tail = 0U;
        }
        /** update the tail to a valid index */
        self->tail = tail; 
        return true;
    }
    else {
        return false;
    }
}

/*..........................................................................*/
ringbuf_index_t HLGIH_SAL_RingBuf_GetFreeSlots(SAL_RINGBUF_HANDLE const self)
{
    ringbuf_index_t head = self->head;
    ringbuf_index_t tail = self->tail;
    /* if buffer empty? */
    if (head == tail) {
        return (ringbuf_index_t)(self->end - 1U);
    }
    else if (head < tail) {
        return (ringbuf_index_t)(tail - head - 1U);
    }
    else {
        return (ringbuf_index_t)(self->end + tail - head - 1U);
    }
}

/*..........................................................................*/
void HLGIH_SAL_RingBuf_ProcessAll(SAL_RINGBUF_HANDLE const self, HLGIH_SAL_RingBuf_Process_Handler handler) 
{
    ringbuf_index_t tail = self->tail;
    /** ring buffer not empty? */
    while (self->head != tail) {
        (*handler)(self->buf[tail]);
        ++tail;
        if (tail == self->end) {
            tail = 0U;
        }
        /** update the tail to a valid index */
        self->tail = tail;
    }
}