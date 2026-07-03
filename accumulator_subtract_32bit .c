#include <stdint.h>

typedef union {
    uint32_t value;
    struct { uint8_t byte0, byte1, byte2, byte3; } b;
} accumulator32_t;

extern accumulator32_t g_accumulator;

/*
 * accumulator_subtract_32bit
 * ---------------------------------------------------------------
 * Subtracts an external 4-byte big-endian value from the global
 * accumulator, in place, with borrow propagation byte-by-byte.
 */
void accumulator_subtract_32bit(const uint8_t *operand)
{
    uint8_t borrow;

    borrow = g_accumulator.b.byte3 < operand[3];
    g_accumulator.b.byte3 -= operand[3];

    uint8_t b2_borrow = g_accumulator.b.byte2 < operand[2];
    g_accumulator.b.byte2 = g_accumulator.b.byte2 - operand[2] - borrow;
    borrow = b2_borrow;

    uint8_t b1_borrow = g_accumulator.b.byte1 < operand[1];
    g_accumulator.b.byte1 = g_accumulator.b.byte1 - operand[1] - borrow;
    borrow = b1_borrow;

    g_accumulator.b.byte0 = g_accumulator.b.byte0 - operand[0] - borrow;
}