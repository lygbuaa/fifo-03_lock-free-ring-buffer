/**
 * @file    hlgih_sal_ringbuf.h
 * @brief   Provides an single producer single consumer ringbuf template
 * @details Provides an single producer single consumer ringbuf template
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

/** ringbuf accept uint8_t block as payload */
typedef uint8_t  ringbuf_payload_t;

/** The ringbuf cell definition */
typedef struct hlgih_SAL_RINGBUF_CELL_S
{
    uint32_t magic;                     /**< magic number to identify a ringbuf cell, default to 0x72696e67 */
    uint32_t index;                     /**< index of this cell, can be stamped by the producer */
    uint32_t size;                      /**< total size of the payload, in bytes */
    uint32_t len;                       /**< used bytes of the payload */
    int64_t timestamp;                  /**< timestamp when the payload received */
    ringbuf_payload_t* payload;         /**< payload address */
} SAL_RINGBUF_CELL_S;

/** the handle of ringbuf cell object */
typedef SAL_RINGBUF_CELL_S *SAL_RINGBUF_CELL_HANDLE;

/** the handle of ringbuf object */
typedef struct SAL_RINGBUF_S *SAL_RINGBUF_HANDLE;

/** ------------------------------------------------------------------------ */
/**
 * @brief  HLGIH_SAL_RingBuf_Create
 *         Create a ringbuf object with the given length and payload size.
 *
 * @param[out]  rb                Ringbuf handle object.
 * @param[in]   len               Specifies the ringbuf length.
 * @param[in]   payload_size      Specifies the payload size, in bytes.
 * 
 *
 * @return
 *
 * @retval false                if operation failed
 * @retval true                 if operation succeeded
 */
bool HLGIH_SAL_RingBuf_Create(SAL_RINGBUF_HANDLE *rb, const uint32_t len, const uint32_t payload_size);

/** ------------------------------------------------------------------------ */
/**
 * @brief  HLGIH_SAL_RingBuf_Release
 *         Release a ringbuf object, all buffers will be freed.
 *
 * @param[in]  self               Ringbuf handle object.
 *
 * @return
 *
 * @retval false                if operation failed
 * @retval true                 if operation succeeded
 */
bool HLGIH_SAL_RingBuf_Release(SAL_RINGBUF_HANDLE const self);

/** ------------------------------------------------------------------------ */
/**
 * @brief  HLGIH_SAL_RingBuf_IsEmpty
 *         Check if the ringbuf is empty.
 *
 * @param[in]  self               Ringbuf handle object.
 *
 * @return
 *
 * @retval false                if operation failed
 * @retval true                 if operation succeeded
 */
bool HLGIH_SAL_RingBuf_IsEmpty(SAL_RINGBUF_HANDLE const self);

/** ------------------------------------------------------------------------ */
/**
 * @brief  HLGIH_SAL_RingBuf_IsFull
 *         Check if the ringbuf is full.
 *
 * @param[in]  self               Ringbuf handle object.
 *
 * @return
 *
 * @retval false                if operation failed
 * @retval true                 if operation succeeded
 */
bool HLGIH_SAL_RingBuf_IsFull(SAL_RINGBUF_HANDLE const self);

/** ------------------------------------------------------------------------ */
/**
 * @brief  HLGIH_SAL_RingBuf_Clear
 *         Clear the ringbuf content, but won't release any buffers.
 *
 * @param[in]  self               Ringbuf handle object.
 *
 * @return
 *
 * @retval false                if operation failed
 * @retval true                 if operation succeeded
 */
bool HLGIH_SAL_RingBuf_Clear(SAL_RINGBUF_HANDLE const self);

/** ------------------------------------------------------------------------ */
/**
 * @brief  HLGIH_SAL_RingBuf_GetWriteCell
 *         Get a ringbuf cell object for writing, could fail if ringbuf is full even after timeout.
 *
 * @param[in]  self              Ringbuf handle object.
 * @param[in]  timeout_us        Specifies the timeout in us to wait (if ringbuf is full) before unblocking, will return immediately if timeout_us <= 0.
 * @param[out] el                The pointer to SAL_RINGBUF_CELL_HANDLE which is returned to producer. 
 * 
 *
 * @return
 *
 * @retval false                if operation failed
 * @retval true                 if operation succeeded
 */
bool HLGIH_SAL_RingBuf_GetWriteCell(SAL_RINGBUF_HANDLE const self, const int64_t timeout_us, SAL_RINGBUF_CELL_HANDLE* const el);

/** ------------------------------------------------------------------------ */
/**
 * @brief  HLGIH_SAL_RingBuf_ReturnWriteCell
 *         Return the SAL_RINGBUF_CELL_HANDLE acquired by HLGIH_SAL_RingBuf_GetWriteCell.
 *
 * @param[in]  self              Ringbuf handle object.
 * @param[out] el                The SAL_RINGBUF_CELL_HANDLE acquired by HLGIH_SAL_RingBuf_GetWriteCell. 
 * 
 *
 * @return
 *
 * @retval false                if operation failed
 * @retval true                 if operation succeeded
 */
bool HLGIH_SAL_RingBuf_ReturnWriteCell(SAL_RINGBUF_HANDLE const self, SAL_RINGBUF_CELL_HANDLE const el);

/** ------------------------------------------------------------------------ */
/**
 * @brief  HLGIH_SAL_RingBuf_GetReadCell
 *         Get a ringbuf cell object for reading, could fail if ringbuf is empty even after timeout.
 *
 * @param[in]  self              Ringbuf handle object.
 * @param[in]  timeout_us        Specifies the timeout in us to wait (if ringbuf is empty) before unblocking, will return immediately if timeout_us <= 0.
 * @param[out] el                The pointer to SAL_RINGBUF_CELL_HANDLE which is returned to consumer.
 * 
 *
 * @return
 *
 * @retval false                if operation failed
 * @retval true                 if operation succeeded
 */
bool HLGIH_SAL_RingBuf_GetReadCell(SAL_RINGBUF_HANDLE const self, const int64_t timeout_us, SAL_RINGBUF_CELL_HANDLE* const el);

/** ------------------------------------------------------------------------ */
/**
 * @brief  HLGIH_SAL_RingBuf_ReturnReadCell
 *         Return the SAL_RINGBUF_CELL_HANDLE acquired by HLGIH_SAL_RingBuf_GetReadCell.
 *
 * @param[in]  self              Ringbuf handle object.
 * @param[out] el                The SAL_RINGBUF_CELL_HANDLE acquired by HLGIH_SAL_RingBuf_GetReadCell. 
 * 
 *
 * @return
 *
 * @retval false                if operation failed
 * @retval true                 if operation succeeded
 */
bool HLGIH_SAL_RingBuf_ReturnReadCell(SAL_RINGBUF_HANDLE const self, SAL_RINGBUF_CELL_HANDLE const el);

/** ------------------------------------------------------------------------ */
/**
 * @brief  HLGIH_SAL_RingBuf_GetFreeSlotsCount
 *         Return the total count of cells available for writing
 *
 * @param[in]  self              Ringbuf handle object.
 * @param[out] count             The total count of cells available for writing
 * 
 *
 * @return
 *
 * @retval false                if operation failed
 * @retval true                 if operation succeeded
 */
bool HLGIH_SAL_RingBuf_GetFreeSlotsCount(SAL_RINGBUF_HANDLE const self, uint32_t* count);

#ifdef __cplusplus
}
#endif

#endif /** __HLGIH_SAL_RING_BUF_H__ */
