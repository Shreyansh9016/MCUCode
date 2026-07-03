#include <stdint.h>

typedef union {
    uint32_t value;
    struct { uint8_t byte0, byte1, byte2, byte3; } b;
} accumulator32_t;

extern accumulator32_t g_accumulator;

/*
 * accumulator_compare_32bit
 * Address  : 00f65d
 * Original : FUN_00f65d
 * ---------------------------------------------------------------
 * Compares the global 32-bit accumulator against an external
 * 4-byte big-endian value (byte0 = MSB ... byte3 = LSB - the same
 * ordering used by accumulator_subtract_32bit), one byte at a time
 * from most significant to least significant.
 *
 * Return value (faithful to the original decompiled logic):
 *   0xFF                  - accumulator < operand (at the first
 *                           byte, scanned MSB to LSB, where they
 *                           differ)
 *   1                     - accumulator > operand, differing
 *                           somewhere after byte0 (byte0 is equal
 *                           in this case)
 *   g_accumulator.b.byte3 - all four bytes are equal; the routine
 *                           returns this raw byte rather than a
 *                           sentinel, so a caller testing for
 *                           "result == 0" as the equality case only
 *                           works when byte3 happens to be 0
 *   g_accumulator.b.byte0 - byte0 (the MSB) differs; the raw MSB is
 *                           returned instead of a comparison flag,
 *                           matching the original assembly
 */
uint8_t accumulator_compare_32bit(const uint8_t *operand)
{
    uint8_t isLess;
    uint8_t result;

    result = g_accumulator.b.byte0;

    if (g_accumulator.b.byte0 == operand[0]) {
        isLess = g_accumulator.b.byte1 < operand[1];

        if (g_accumulator.b.byte1 == operand[1]) {
            isLess = g_accumulator.b.byte2 < operand[2];

            if (g_accumulator.b.byte2 == operand[2]) {
                isLess = g_accumulator.b.byte3 < operand[3];

                if (g_accumulator.b.byte3 == operand[3]) {
                    return g_accumulator.b.byte3;
                }
            }
        }

        if (isLess) {
            return 0xff;
        }

        result = 1;
    }

    return result;
}
