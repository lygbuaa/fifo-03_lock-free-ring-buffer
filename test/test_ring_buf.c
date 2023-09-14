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
    // const int wr_timeout_us = 0;
    int discard_count = 0;

    for(int i=0; i<N; i++)
    {
        bool rc = HLGIH_SAL_RingBuf_GetWriteCell(rbh, wr_timeout_us, &ch);
        // printf("HLGIH_SAL_RingBuf_GetWriteCell [%d]: %d, ch addr: %x\n", i, (int)rc, ch);
        if(rc)
        {
            // printf("ch->payload: %x, len: %d\n", ch->payload, ch->len);
            memcpy(ch->payload, test_payload, sizeof(test_payload));
            ch->len = sizeof(test_payload);
            ch->index = i;
            printf("write ringbuf cell[%d], len: %d, payload: ", i, ch->len);
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
    int N = 150;
    const int rd_timeout_us = (100*1000);
    // const int rd_timeout_us = 0;
    int failed_count = 0;

    for(int i=0; i<N; i++)
    {
        bool rc = HLGIH_SAL_RingBuf_GetReadCell(rbh, rd_timeout_us, &ch);
        printf("HLGIH_SAL_RingBuf_GetReadCell [%d]: %d\n", i, (int)rc);
        if(rc)
        {
            printf("read ringbuf cell[%d], index: %d, len: %d, payload: ", i, ch->index, ch->len);
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

void TEST_single_producer_single_consumer(void)
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
        printf("producer creation failed: %d\n", rc1);
        exit(1);
    }

    /** wait untill ringbuf is full */
    // usleep(1000*1000);

    if( (rc2=pthread_create( &consumer, NULL, &consumer_func, NULL)) )
    {
        printf("consumer creation failed: %d\n", rc2);
        exit(1);
    }

    pthread_join( producer, NULL);
    pthread_join( consumer, NULL);

}

void *consumer_func_multi_flight()
{
    int N = 80;
    const int rd_timeout_us = (100*1000);
    // const int rd_timeout_us = 0;
    int failed_count = 0;
    SAL_RINGBUF_CELL_HANDLE ch2;

    for(int i=0; i<N; i++)
    {
        bool rc = HLGIH_SAL_RingBuf_GetReadCell(rbh, rd_timeout_us, &ch);
        printf("HLGIH_SAL_RingBuf_GetReadCell [%d]: %d\n", i, (int)rc);
        if(rc)
        {
            printf("read ringbuf cell[%d], index: %d, len: %d, payload: ", i, ch->index, ch->len);
            for(int j=0; j<ch->len; j++)
            {
                printf("%x, ", ch->payload[j]);
            }
            printf("\n");

            bool rc2 = HLGIH_SAL_RingBuf_GetReadCell(rbh, rd_timeout_us, &ch2);
            printf("HLGIH_SAL_RingBuf_GetReadCell [%d] again: %d\n", i, (int)rc2);
            if(rc2)
            {
                printf("read ringbuf cell[%d] again, index: %d, len: %d, payload: ", i, ch->index, ch->len);
                for(int j=0; j<ch2->len; j++)
                {
                    printf("%x, ", ch2->payload[j]);
                }
                printf("\n");
                HLGIH_SAL_RingBuf_ReturnReadCell(rbh, ch2);
            }

            HLGIH_SAL_RingBuf_ReturnReadCell(rbh, ch);
        }
        else
        {
            failed_count ++;
        }
        usleep(10*1000);
    }
    printf("\n*** consumer failed count: %d ***\n", failed_count);
}

void TEST_multi_flight(void)
{
    pthread_t producer, consumer;
    bool rc = HLGIH_SAL_RingBuf_Create(&rbh, rb_len, rb_payload_size);
    printf("HLGIH_SAL_RingBuf_Create: %d\n", (int)rc);

    uint32_t count = 0;
    rc = HLGIH_SAL_RingBuf_GetFreeSlotsCount(rbh, &count);
    printf("\nHLGIH_SAL_RingBuf_GetFreeSlotsCount: %d, count: %d\n", (int)rc, count);
    TEST("HLGIH_SAL_RingBuf_GetFreeSlots");
    EXPECT(count == rb_len-1U);

    // usleep(1000*1000);
    int rc1, rc2;

    if( (rc1=pthread_create( &producer, NULL, &producer_func, NULL)) )
    {
        printf("producer creation failed: %d\n", rc1);
        exit(1);
    }

    /** wait untill ringbuf is full */
    // usleep(100*1000);

    if( (rc2=pthread_create( &consumer, NULL, &consumer_func_multi_flight, NULL)) )
    {
        printf("consumer creation failed: %d\n", rc2);
        exit(1);
    }

    pthread_join( producer, NULL);
    pthread_join( consumer, NULL);
}

void TEST_single_producer_multi_consumer(void)
{
    pthread_t producer_1, consumer_1, consumer_2;
    bool rc = HLGIH_SAL_RingBuf_Create(&rbh, rb_len, rb_payload_size);
    printf("HLGIH_SAL_RingBuf_Create: %d\n", (int)rc);

    uint32_t count = 0;
    rc = HLGIH_SAL_RingBuf_GetFreeSlotsCount(rbh, &count);
    printf("\nHLGIH_SAL_RingBuf_GetFreeSlotsCount: %d, count: %d\n", (int)rc, count);
    TEST("HLGIH_SAL_RingBuf_GetFreeSlots");
    EXPECT(count == rb_len-1U);

    if( (rc=pthread_create( &producer_1, NULL, &producer_func, NULL)) )
    {
        printf("producer-1 creation failed: %d\n", rc);
        exit(1);
    }

    /** wait untill ringbuf is full */
    // usleep(1000*1000);

    if( (rc=pthread_create( &consumer_1, NULL, &consumer_func, NULL)) )
    {
        printf("consumer-1 creation failed: %d\n", rc);
        exit(1);
    }

    if( (rc=pthread_create( &consumer_2, NULL, &consumer_func, NULL)) )
    {
        printf("consumer-2 creation failed: %d\n", rc);
        exit(1);
    }

    pthread_join( producer_1, NULL);
    pthread_join( consumer_1, NULL);
    pthread_join( consumer_2, NULL);
}
