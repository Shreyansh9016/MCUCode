#include <stdint.h>

extern int32_t g_accumulator;

/*
 * sign_extend_8_to_32
 * ---------------------------------------------------------------
 * Loads a signed byte into the 32-bit accumulator, sign-extended.
 */
void sign_extend_8_to_32(int8_t value)
{
    g_accumulator = (int32_t)value;
}