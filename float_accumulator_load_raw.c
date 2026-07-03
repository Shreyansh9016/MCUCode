#include <stdint.h>

typedef struct {
    uint8_t  exponent;
    uint16_t mantissa;
} mcu_float_t;

extern mcu_float_t g_float_accumulator;

/*
 * float_accumulator_load_raw
 * ---------------------------------------------------------------
 * Loads a raw 16-bit value directly into the float accumulator's
 * mantissa field, with no normalization performed. Exponent and
 * sign are cleared. Used to seed the accumulator before further
 * float operations, bypassing conversion logic.
 */
void float_accumulator_load_raw(uint16_t raw_value)
{
    g_float_accumulator.mantissa = raw_value;
    g_float_accumulator.exponent = 0;
}