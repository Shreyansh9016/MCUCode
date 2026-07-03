#include <stdint.h>

extern void     signed_float_to_int16(void);   /* FUN_00f58c */
extern uint16_t g_float_low_word;               /* _DAT_000002 */

/*
 * float_to_int16_raw
 * ---------------------------------------------------------------
 * Converts the accumulator to int16 and returns the raw low
 * mantissa word left behind by the conversion - a thin public
 * wrapper around signed_float_to_int16.
 */
uint16_t float_to_int16_raw(void)
{
    signed_float_to_int16();
    return g_float_low_word;
}