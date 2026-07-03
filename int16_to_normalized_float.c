#include <stdint.h>

/* Shared 3-byte float accumulator used by the soft-float library.
   exponent = biased exponent byte
   mantissa = 16-bit mantissa, bit 15 = sign */
typedef struct {
    uint8_t  exponent;
    uint16_t mantissa;
} mcu_float_t;

extern mcu_float_t g_float_accumulator;   /* DAT_000000 / _DAT_000001..2 */

/*
 * int16_to_normalized_float
 * ---------------------------------------------------------------
 * Converts a signed 16-bit integer into the MCU's normalized
 * internal float format, storing the result in the global float
 * accumulator. Zero is handled as a special case (exponent = 0,
 * mantissa = 0). For non-zero values, left-shifts the value until
 * its top bit is set, counting the shifts to compute the exponent.
 */
void int16_to_normalized_float(int16_t value)
{
    if (value == 0) {
        g_float_accumulator.exponent = 0;
        g_float_accumulator.mantissa = 0;
        return;
    }

    uint8_t exponent = 0x8E;   /* base exponent bias */
    uint8_t sign_bit = 0;
    int16_t shifted  = value;

    if (shifted >= 0) {
        if ((uint8_t)(shifted >> 8) == 0) {
            /* value fits in a single byte - use smaller bias */
            exponent = 0x86;
            sign_bit = 0;
        }
        while (shifted >= 0) {
            exponent--;
            sign_bit ^= 1;
            shifted <<= 1;
        }
    }

    g_float_accumulator.mantissa = (uint16_t)((sign_bit << 15) | (shifted & 0x7FFF));
    g_float_accumulator.exponent = exponent >> 1;
}