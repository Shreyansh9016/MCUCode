#include <stdint.h>

extern int32_t g_accumulator;

/*
 * sign_extend_16_to_32
 * ---------------------------------------------------------------
 * Loads a signed 16-bit value into the 32-bit accumulator,
 * sign-extended.
 */
void sign_extend_16_to_32(int16_t value)
{
    g_accumulator = (int32_t)value;
}