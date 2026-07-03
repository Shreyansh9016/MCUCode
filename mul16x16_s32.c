#include <stdint.h>

/*
 * STM8 runtime library: signed 16x16 -> 32-bit multiply.
 * Calling convention:
 *   param_2 (16-bit) passed normally
 *   in_Y    (16-bit) passed in Y register (not visible in decompiled args)
 *   param_1 is passed through untouched (likely just a chained return value)
 * Result is a 32-bit signed product, normally returned via a fixed
 * accumulator location (DAT_000000..DAT_000003 in the original) -
 * modeled here as a plain return value instead.
 */
int32_t mul16x16_s32(int16_t a, int16_t b)
{
    return (int32_t)a * (int32_t)b;
}