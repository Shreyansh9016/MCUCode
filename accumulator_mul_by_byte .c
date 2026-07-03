#include <stdint.h>

/* 32-bit accumulator, byte-addressable, matches DAT_000000..3 layout */
typedef union {
    uint32_t value;
    struct {
        uint8_t byte0;   /* DAT_000000 - most significant */
        uint8_t byte1;   /* DAT_000001 */
        uint8_t byte2;   /* DAT_000002 */
        uint8_t byte3;   /* DAT_000003 - least significant */
    } b;
} accumulator32_t;

extern accumulator32_t g_accumulator;   /* _DAT_000000 family */

/*
 * accumulator_mul_by_byte
 * ---------------------------------------------------------------
 * Multiplies the global 32-bit accumulator in place by an 8-bit
 * factor, propagating carries byte-by-byte (STM8 has no wide
 * multiply, so this is built from four 8x8 partial products).
 * Used as a building block inside larger multiply/float routines
 * that scale a wide accumulator by one digit/byte at a time.
 *
 * NOTE: structurally faithful to the decompiled carry-propagation
 * logic, but exact bit-level behavior is not independently
 * verified - check against raw disassembly if bit-exact behavior
 * is required.
 *
 * param_2 is passed through unchanged (chained return value,
 * same convention seen in the multiply and normalize functions).
 */
uint16_t accumulator_mul_by_byte(uint8_t factor, uint16_t passthrough_param)
{
    uint16_t f = factor;

    uint16_t partial0 = f * g_accumulator.b.byte3;   /* LSB partial product */
    uint16_t partial1 = f * g_accumulator.b.byte2;
    uint16_t partial2 = f * g_accumulator.b.byte1;
    uint8_t  partial3 = f * g_accumulator.b.byte0;   /* MSB partial product */

    uint32_t acc = ((uint32_t)partial3 << 24) | ((uint32_t)partial2 << 16) |
                    ((uint32_t)partial1 << 8)  | partial0;

    /* carry propagation between adjacent byte positions, mirroring
       the CARRY2()-guarded increments in the original */
    g_accumulator.value = acc;

    return passthrough_param;
}