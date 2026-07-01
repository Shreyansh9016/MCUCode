#include <stdint.h>

/* Custom 3-4 byte float format used by this MCU's soft-float library:
   exponent (1 byte) + mantissa (2 bytes), occasionally + 1 overflow byte */
typedef struct {
    uint8_t  exponent;
    uint16_t mantissa;
} mcu_float3_t;

typedef struct {
    uint8_t  exponent;
    uint16_t mantissa;
    uint8_t  overflow;
} mcu_float4_t;

extern mcu_float3_t int_to_float3(uint16_t raw);        /* was FUN_00f893 (partial) */
extern void         float_div_const(uint16_t const_addr); /* was FUN_00f4a5 */
extern void         store_table_entry(void *dest, ...);   /* was FUN_00f817 */

#define SCALE_CONST_ADDR  0x808c   /* fixed flash constant - divisor for every entry */

void build_calibration_table(void)   /* was FUN_008c33 */
{
    /* Table storage - stride 3 for first 5 entries, stride 4 afterward */
    mcu_float3_t table_lo[5];   /* offsets 0x04, 0x07, 0x0a, 0x0d, 0x10 */
    mcu_float4_t table_hi[11];  /* offsets 0x14 .. 0x28, plus a few beyond local frame
                                    (0x5177, 0x678e, 0x7da8, 0x93c2, 0xa9dc -
                                    these look like they've spilled into a fixed
                                    flash/RAM table rather than the stack) */

    uint16_t raw = 0x3e;
    const uint16_t step = 6; /* approximate - actual deltas alternate 6/7,
                                 worth confirming against real disassembly */

    for (int i = 0; i < 5; i++) {
        mcu_float3_t v = int_to_float3(raw);
        float_div_const(SCALE_CONST_ADDR);
        table_lo[i] = v;
        raw += step;
    }

    for (int i = 0; i < 11; i++) {
        mcu_float4_t v = { .exponent = /* ... */, .mantissa = /* ... */ };
        float_div_const(SCALE_CONST_ADDR);
        table_hi[i] = v;
        raw += step;
    }
}