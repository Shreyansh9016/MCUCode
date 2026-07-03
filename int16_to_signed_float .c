#include <stdint.h>

typedef struct {
    uint8_t  sign_and_exponent;   /* bit 7 = sign, bits 6-0 = exponent */
    uint16_t mantissa;
} mcu_float_t;

extern mcu_float_t g_float_accumulator;

/*
 * int16_to_signed_float
 * ---------------------------------------------------------------
 * Converts a signed 16-bit integer into the float accumulator,
 * with the original sign correctly packed into the exponent
 * byte's top bit. This is the complete version of
 * int16_to_normalized_float - use this one as the reference
 * implementation for the int->float conversion path.
 */
void int16_to_signed_float(int16_t value)
{
    if (value == 0) {
        g_float_accumulator.sign_and_exponent = 0;
        g_float_accumulator.mantissa = 0;
        return;
    }

    int16_t magnitude = value;
    if (magnitude < 0) {
        magnitude = -magnitude;
    }

    uint8_t exponent = 0x8E;
    uint8_t running_bit = 0;

    if (magnitude >= 0) {
        if ((uint8_t)(magnitude >> 8) == 0) {
            exponent = 0x86;
            running_bit = 0;
        }
        while (magnitude >= 0) {
            exponent--;
            running_bit ^= 1;
            magnitude <<= 1;
        }
    }

    g_float_accumulator.mantissa = (uint16_t)((running_bit << 15) | (magnitude & 0x7FFF));
    g_float_accumulator.sign_and_exponent =
        (uint8_t)(((value & 0x8000) != 0) << 7) | (exponent >> 1);
}