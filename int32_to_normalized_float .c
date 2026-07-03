#include <stdint.h>

typedef struct {
    uint8_t  exponent;
    uint16_t mantissa;
} mcu_float_t;

extern mcu_float_t g_float_accumulator;

/*
 * int32_to_normalized_float
 * ---------------------------------------------------------------
 * Converts a signed 32-bit integer into the accumulator's
 * normalized float form - the 32-bit counterpart to
 * int16_to_normalized_float. Takes the absolute value, left-
 * shifts until the top bit is set (counting shifts as the
 * exponent), then packs the sign bit back into the mantissa.
 *
 * NOTE: structurally faithful but not bit-verified - treat as a
 * sketch of intent, same caveat as float_accumulator_renormalize.
 */
uint16_t int32_to_normalized_float(int32_t value, uint16_t passthrough_param)
{
    if (value == 0) {
        g_float_accumulator.exponent = 0;
        g_float_accumulator.mantissa = 0;
        return passthrough_param;
    }

    uint8_t sign = 0;
    if (value < 0) {
        value = -value;
        sign = 1;
    }

    uint8_t exponent = 0x17;   /* base bias, mirrors int16 version's 0x8E-family constant */
    while (!(value & 0x80000000)) {
        exponent--;
        value <<= 1;
    }

    g_float_accumulator.mantissa = (uint16_t)((sign << 15) | ((value >> 16) & 0x7FFF));
    g_float_accumulator.exponent = exponent;

    return passthrough_param;
}