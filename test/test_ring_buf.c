#include <stdint.h>
#include <stdbool.h>

#include "ring_buf.h"
#include "sutest.h"
#include <pthread.h>

#include <stdio.h>

SAL_RINGBUF_HANDLE rbh;
SAL_RINGBUF_CELL_HANDLE ch;
const int rb_len = 16;
const int rb_payload_size = (8*1024*1024);
const uint8_t test_payload[5] = {0x11, 0x22, 0x33, 0x44, 0x00};

void TEST_single_thread(void)
{
    const int timeout_us = (100*1000);

    bool rc = HLGIH_SAL_RingBuf_Create(&rbh, rb_len, rb_payload_size);
    printf("HLGIH_SAL_RingBuf_Create: %d\n", (int)rc);

    uint32_t count = 0;
    rc = HLGIH_SAL_RingBuf_GetFreeSlotsCount(rbh, &count);
    printf("\nHLGIH_SAL_RingBuf_GetFreeSlotsCount: %d, count: %d\n", (int)rc, count);
    TEST("HLGIH_SAL_RingBuf_GetFreeSlots");
    EXPECT(count == rb_len-1U);

    TEST("HLGIH_SAL_RingBuf_GetReadCell timeout");
    rc = HLGIH_SAL_RingBuf_GetReadCell(rbh, timeout_us, &ch);
    if(rc)
    {
        printf("HLGIH_SAL_RingBuf_GetReadCell: %d, ch->magic: %d\n", (int)rc, ch->magic);
        HLGIH_SAL_RingBuf_ReturnReadCell(rbh, ch);
    }

    for(int i=0; i<rb_len+3; i++)
    {
        rc = HLGIH_SAL_RingBuf_GetWriteCell(rbh, timeout_us, &ch);
        printf("HLGIH_SAL_RingBuf_GetWriteCell [%d]: %d, ch addr: %x\n", i, (int)rc, ch);
        if(rc)
        {
            // printf("ch->payload: %x, len: %d\n", ch->payload, ch->len);
            memcpy(ch->payload, test_payload, sizeof(test_payload));
            ch->len = sizeof(test_payload);
            printf("write ringbuf cell[%d]: %d, len: %d, payload: ", i, ch->len);
            for(int j=0; j<ch->len; j++)
            {
                printf("%x, ", ch->payload[j]);
            }
            printf("\n");
            HLGIH_SAL_RingBuf_ReturnWriteCell(rbh, ch);
        }
    }

    rc = HLGIH_SAL_RingBuf_GetFreeSlotsCount(rbh, &count);
    printf("\nHLGIH_SAL_RingBuf_GetFreeSlotsCount: %d, count: %d\n", (int)rc, count);
    TEST("HLGIH_SAL_RingBuf_GetFreeSlots");
    EXPECT(count == 0);

    for(int i=0; i<rb_len/2; i++)
    {
        rc = HLGIH_SAL_RingBuf_GetReadCell(rbh, timeout_us, &ch);
        printf("HLGIH_SAL_RingBuf_GetReadCell: %d\n", (int)rc);
        if(rc)
        {
            printf("read ringbuf cell[%d]: %d, len: %d, payload: ", i, ch->len);
            for(int j=0; j<ch->len; j++)
            {
                printf("%x, ", ch->payload[j]);
            }
            printf("\n");
            HLGIH_SAL_RingBuf_ReturnReadCell(rbh, ch);
        }
    }

    rc = HLGIH_SAL_RingBuf_GetFreeSlotsCount(rbh, &count);
    printf("\nHLGIH_SAL_RingBuf_GetFreeSlotsCount: %d, count: %d\n", (int)rc, count);
    TEST("HLGIH_SAL_RingBuf_GetFreeSlots");
    EXPECT(count == rb_len/2);

    TEST("HLGIH_SAL_RingBuf_Clear");
    rc = HLGIH_SAL_RingBuf_Clear(rbh);
    printf("HLGIH_SAL_RingBuf_Clear: %d\n", (int)rc);

    rc = HLGIH_SAL_RingBuf_GetFreeSlotsCount(rbh, &count);
    printf("\nHLGIH_SAL_RingBuf_GetFreeSlotsCount: %d, count: %d\n", (int)rc, count);
    TEST("HLGIH_SAL_RingBuf_GetFreeSlots");
    EXPECT(count == rb_len-1);
    
    rc = HLGIH_SAL_RingBuf_Release(rbh);
    printf("HLGIH_SAL_RingBuf_Release: %d\n", (int)rc);
}

void *producer_func()
{
    int N = 100;
    const int wr_timeout_us = (100*1000);
    int discard_count = 0;

    for(int i=0; i<N; i++)
    {
        bool rc = HLGIH_SAL_RingBuf_GetWriteCell(rbh, wr_timeout_us, &ch);
        printf("HLGIH_SAL_RingBuf_GetWriteCell [%d]: %d, ch addr: %x\n", i, (int)rc, ch);
        if(rc)
        {
            // printf("ch->payload: %x, len: %d\n", ch->payload, ch->len);
            memcpy(ch->payload, test_payload, sizeof(test_payload));
            ch->len = sizeof(test_payload);
            printf("write ringbuf cell[%d]: %d, len: %d, payload: ", i, ch->len);
            for(int j=0; j<ch->len; j++)
            {
                printf("%x, ", ch->payload[j]);
            }
            printf("\n");
            HLGIH_SAL_RingBuf_ReturnWriteCell(rbh, ch);
        }
        else
        {
            discard_count ++;
        }
        usleep(20*1000);
    }
    printf("\n*** producer discard count: %d ***\n", discard_count);
}

void *consumer_func()
{
    int N = 400;
    const int rd_timeout_us = (100*1000);
    int failed_count = 0;

    for(int i=0; i<N; i++)
    {
        bool rc = HLGIH_SAL_RingBuf_GetReadCell(rbh, rd_timeout_us, &ch);
        printf("HLGIH_SAL_RingBuf_GetReadCell [%d]: %d\n", i, (int)rc);
        if(rc)
        {
            printf("read ringbuf cell[%d]: %d, len: %d, payload: ", i, ch->len);
            for(int j=0; j<ch->len; j++)
            {
                printf("%x, ", ch->payload[j]);
            }
            printf("\n");
            HLGIH_SAL_RingBuf_ReturnReadCell(rbh, ch);
        }
        else
        {
            failed_count ++;
        }
        usleep(5*1000);
    }
    printf("\n*** consumer failed count: %d ***\n", failed_count);
}

void TEST_multi_thread(void)
{
    pthread_t producer, consumer;
    bool rc = HLGIH_SAL_RingBuf_Create(&rbh, rb_len, rb_payload_size);
    printf("HLGIH_SAL_RingBuf_Create: %d\n", (int)rc);

    uint32_t count = 0;
    rc = HLGIH_SAL_RingBuf_GetFreeSlotsCount(rbh, &count);
    printf("\nHLGIH_SAL_RingBuf_GetFreeSlotsCount: %d, count: %d\n", (int)rc, count);
    TEST("HLGIH_SAL_RingBuf_GetFreeSlots");
    EXPECT(count == rb_len-1U);

    int rc1, rc2;
    if( (rc1=pthread_create( &producer, NULL, &producer_func, NULL)) )
    {
        printf("Thread creation failed: %d\n", rc1);
        exit(1);
    }

    /** wait untill ringbuf is full */
    // usleep(1000*1000);

    if( (rc2=pthread_create( &consumer, NULL, &consumer_func, NULL)) )
    {
        printf("Thread creation failed: %d\n", rc2);
        exit(1);
    }

    pthread_join( producer, NULL);
    pthread_join( consumer, NULL);

}
