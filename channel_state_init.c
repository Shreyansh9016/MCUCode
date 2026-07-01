#include <stdint.h>

/* Struct this function fills in, matching the block FUN_008b3b clears */
typedef struct {
    uint16_t status;            /* 0x178, 2 bytes */
    uint8_t  ch0_handle;        /* 0x17a */
    uint8_t  ch1_handle;        /* 0x17b */
    uint8_t  ch2_handle;        /* 0x17c */
    uint8_t  ch3_handle;        /* 0x17d - filled by clear only? check ordering */
    uint8_t  ch4_handle;        /* 0x17e */
    uint8_t  ch5_handle;        /* 0x17f */
    uint8_t  ch3b_handle;       /* 0x180 */
    uint8_t  pad181;
    uint8_t  ch4b_handle;       /* 0x182 */
    uint8_t  pad183;
    uint8_t  ch5b_handle;       /* 0x184 */
    uint8_t  pad185;
    uint8_t  ch6_handle;        /* 0x186 */
    uint8_t  pad187;
    uint16_t field_188;         /* 0x188, 2 bytes */
    uint8_t  field_18a;         /* 0x18a */
    uint16_t field_18b;         /* 0x18b, 2 bytes */
    uint16_t field_18d;         /* 0x18d, 2 bytes */
} bms_channel_state_t;          /* placeholder name - purpose unconfirmed */

#define STATE ((bms_channel_state_t *)0x0178)

/* Flash-resident lookup tables, 0x0C00 (3072) bytes apart - purpose TBD,
   likely NTC/ADC calibration curves per measurement channel */
#define TABLE_BASE   0xCD01
#define TABLE_STRIDE 0x0C00

void channel_state_init(void)   /* was FUN_008ba6 */
{
    build_table_a();   /* was FUN_008c33 */
    build_table_b();   /* was FUN_008d9e */
    build_table_c();   /* was FUN_008edf */

    status_field_init(&STATE->status);                                   /* FUN_008f42 */
    channel_init_0(&STATE->ch0_handle, (void *)(TABLE_BASE + 0*TABLE_STRIDE)); /* FUN_008f54 */
    channel_init_1(&STATE->ch1_handle, (void *)(TABLE_BASE + 1*TABLE_STRIDE)); /* FUN_008f71 */
    channel_init_2(&STATE->ch2_handle, (void *)(TABLE_BASE + 2*TABLE_STRIDE)); /* FUN_008f8e */
    channel_init_3(&STATE->ch3b_handle,(void *)(TABLE_BASE + 3*TABLE_STRIDE)); /* FUN_008fab */
    channel_init_4(&STATE->ch4b_handle,(void *)(TABLE_BASE + 4*TABLE_STRIDE)); /* FUN_008fc8 */
    channel_init_5(&STATE->ch5b_handle,(void *)(TABLE_BASE + 5*TABLE_STRIDE)); /* FUN_008fe5 */
    channel_init_6(&STATE->ch6_handle, (void *)(TABLE_BASE + 6*TABLE_STRIDE)); /* FUN_009002 */

    field_188_init(&STATE->field_188);   /* FUN_00901f */

    /* Default calibration/threshold constants packed in */
    calib_defaults_init(&STATE->field_18a, 0x24, 0x1D, 0x87, 0x85, 0x83); /* FUN_009031 */

    field_18b_init(&STATE->field_18b);   /* FUN_009043 */
    field_18d_init(&STATE->field_18d);   /* FUN_00905a */
}