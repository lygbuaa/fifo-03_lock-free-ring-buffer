#include <stdint.h>
#include <stdbool.h>

#include "ring_buf.h"
#include "sutest.h"

#include <stdio.h>


SAL_RINGBUF_CELL_S buf[8];
SAL_RINGBUF_S rb;

static void rb_handler(SAL_RINGBUF_CELL_S const el);

static SAL_RINGBUF_CELL_S test_data[] = {
    0xAAU,
    0xBBU,
    0xCCU,
    0xDDU
};
static ringbuf_index_t test_idx;

void TEST_onRun(void) {
    SAL_RINGBUF_CELL_S el;
    ringbuf_index_t i;

    HLGIH_SAL_RingBuf_Create(&rb, buf, ARRAY_LEN(buf));

    TEST("HLGIH_SAL_RingBuf_GetFreeSlots");
    EXPECT(HLGIH_SAL_RingBuf_GetFreeSlots(&rb) == ARRAY_LEN(buf) - 1U);

    TEST("HLGIH_SAL_RingBuf_Write 3");
    HLGIH_SAL_RingBuf_Write(&rb, 0xAAU);
    HLGIH_SAL_RingBuf_Write(&rb, 0xBBU);
    HLGIH_SAL_RingBuf_Write(&rb, 0xCCU);
    EXPECT(HLGIH_SAL_RingBuf_GetFreeSlots(&rb) == ARRAY_LEN(buf) - 1U - 3U);

    TEST("HLGIH_SAL_RingBuf_Read");
    EXPECT(true == HLGIH_SAL_RingBuf_Read(&rb, &el));
    EXPECT(0xAAU == el);
    EXPECT(true == HLGIH_SAL_RingBuf_Read(&rb, &el));
    EXPECT(0xBBU == el);
    EXPECT(true == HLGIH_SAL_RingBuf_Read(&rb, &el));
    EXPECT(0xCCU == el);
    EXPECT(false == HLGIH_SAL_RingBuf_Read(&rb, &el));

    TEST("HLGIH_SAL_RingBuf_ProcessAll test_data");
    for (i = 0U; i < ARRAY_LEN(test_data); ++i) {
        HLGIH_SAL_RingBuf_Write(&rb, test_data[i]);
    }
    test_idx = 0U;
    HLGIH_SAL_RingBuf_ProcessAll(&rb, &rb_handler);
    EXPECT(HLGIH_SAL_RingBuf_GetFreeSlots(&rb) == ARRAY_LEN(buf) - 1U);
}

static void rb_handler(SAL_RINGBUF_CELL_S const el) {
    EXPECT(test_data[test_idx] == el);
    ++test_idx;
}

