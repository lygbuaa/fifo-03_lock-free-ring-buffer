/**
 * @file    hlgih_sal_ringbuf.h
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

#ifndef __HLGIH_SAL_RING_BUF_H__
#define __HLGIH_SAL_RING_BUF_H__

#include <stdalign.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t ringbuf_index_t;
typedef uint8_t SAL_RINGBUF_CELL_S;

// typedef struct hlgih_SAL_RINGBUF_CELL_S
// {
//     uint32_t magic;         /**< magic number to identify a ringbuf cell, default to 0x72696e67 */
//     uint32_t len;           /**< length of the payload, in bytes */
//     int64_t timestamp;      /**< timestamp when the payload received */
//     uint8_t* payload;       /**< payload address */
// } SAL_RINGBUF_CELL_S;

typedef struct hlgih_SAL_RINGBUF_S
{
    SAL_RINGBUF_CELL_S     *buf;     /**< pointer to the start of the ring buffer */
    ringbuf_index_t         end;     /**< offset of the end of the ring buffer */
    ringbuf_index_t         head;    /**< offset to where next byte will be inserted */
    ringbuf_index_t         tail;    /**< offset of where next byte will be extracted */
} SAL_RINGBUF_S;

typedef SAL_RINGBUF_S *SAL_RINGBUF_HANDLE;

typedef void (*HLGIH_SAL_RingBuf_Process_Handler) (SAL_RINGBUF_CELL_S const el);

void HLGIH_SAL_RingBuf_Create(SAL_RINGBUF_HANDLE const self, SAL_RINGBUF_CELL_S sto[], ringbuf_index_t sto_len);
// SAL_RINGBUF_HANDLE HLGIH_SAL_RingBuf_Create(ringbuf_index_t size);

void HLGIH_SAL_RingBuf_Release(SAL_RINGBUF_HANDLE const self);

bool HLGIH_SAL_RingBuf_Write(SAL_RINGBUF_HANDLE const self, SAL_RINGBUF_CELL_S const el);

bool HLGIH_SAL_RingBuf_Read(SAL_RINGBUF_HANDLE const self, SAL_RINGBUF_CELL_S *pel);

ringbuf_index_t HLGIH_SAL_RingBuf_GetFreeSlots(SAL_RINGBUF_HANDLE const self);

void HLGIH_SAL_RingBuf_ProcessAll(SAL_RINGBUF_HANDLE const self, HLGIH_SAL_RingBuf_Process_Handler handler);

#ifdef __cplusplus
}
#endif

#endif /** __HLGIH_SAL_RING_BUF_H__ */
