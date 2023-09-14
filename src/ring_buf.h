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

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HLGIH_SAL_RINGBUF_MAX_LENGTH        (16)
#define HLGIH_SAL_RINGBUF_MAX_PAYLOAD_SIZE  (8*1024*1024)
#define HLGIH_SAL_RINGBUF_CELL_MAGIC_NUM    (0x72696e67)

typedef uint8_t  ringbuf_payload_t;

typedef struct hlgih_SAL_RINGBUF_CELL_S
{
    uint32_t magic;                     /**< magic number to identify a ringbuf cell, default to 0x72696e67 */
    uint32_t index;                     /**< index of this cell */
    uint32_t size;                      /**< total size of the payload, in bytes */
    uint32_t len;                       /**< used bytes of the payload */
    int64_t timestamp;                  /**< timestamp when the payload received */
    ringbuf_payload_t* payload;         /**< payload address */
} SAL_RINGBUF_CELL_S;
// typedef uint8_t SAL_RINGBUF_CELL_S;

typedef SAL_RINGBUF_CELL_S *SAL_RINGBUF_CELL_HANDLE;
typedef struct SAL_RINGBUF_S *SAL_RINGBUF_HANDLE;

bool HLGIH_SAL_RingBuf_Create(SAL_RINGBUF_HANDLE *rb, const uint32_t len, const uint32_t payload_size);

bool HLGIH_SAL_RingBuf_Release(SAL_RINGBUF_HANDLE const self);

bool HLGIH_SAL_RingBuf_IsEmpty(SAL_RINGBUF_HANDLE const self);

bool HLGIH_SAL_RingBuf_IsFull(SAL_RINGBUF_HANDLE const self);

bool HLGIH_SAL_RingBuf_Clear(SAL_RINGBUF_HANDLE const self);

void HLGIH_SAL_RingBuf_HeadInc(SAL_RINGBUF_HANDLE const self);

void HLGIH_SAL_RingBuf_TailInc(SAL_RINGBUF_HANDLE const self);

void HLGIH_SAL_RingBuf_RdposInc(SAL_RINGBUF_HANDLE const self);

bool HLGIH_SAL_RingBuf_GetWriteCell(SAL_RINGBUF_HANDLE const self, const int64_t timeout_us, SAL_RINGBUF_CELL_HANDLE* const el);

bool HLGIH_SAL_RingBuf_ReturnWriteCell(SAL_RINGBUF_HANDLE const self, SAL_RINGBUF_CELL_HANDLE const el);

bool HLGIH_SAL_RingBuf_GetReadCell(SAL_RINGBUF_HANDLE const self, const int64_t timeout_us, SAL_RINGBUF_CELL_HANDLE* const el);

bool HLGIH_SAL_RingBuf_ReturnReadCell(SAL_RINGBUF_HANDLE const self, SAL_RINGBUF_CELL_HANDLE const el);

bool HLGIH_SAL_RingBuf_GetFreeSlotsCount(SAL_RINGBUF_HANDLE const self, uint32_t* count);

#ifdef __cplusplus
}
#endif

#endif /** __HLGIH_SAL_RING_BUF_H__ */
