#include <stdint.h>

typedef struct {
    uint8_t  exponent;
    uint16_t mantissa;
} mcu_float_t;

extern mcu_float_t g_float_accumulator;

/*
 * float_accumulator_renormalize
 * ---------------------------------------------------------------
 * Restores the float accumulator to normalized form after an
 * arithmetic operation (e.g. add/subtract) may have left the
 * mantissa without its MSB set. Shifts the mantissa left or right
 * as needed, adjusting the exponent to compensate. Passes its
 * input parameter through unchanged (used as a chained return
 * value by callers, consistent with FUN_00f90b's param_1).
 *
 * NOTE: structurally faithful to the decompiled logic, but the
 * exact shift/carry bookkeeping has been simplified - verify
 * against raw disassembly if bit-exact behavior is required.
 */
uint16_t float_accumulator_renormalize(uint16_t passthrough_param)
{
    if (g_float_accumulator.exponent == 0) {
        if (g_float_accumulator.mantissa == 0) {
            return passthrough_param;   /* already zero - nothing to do */
        }
        /* left-shift mantissa until its MSB is set */
        while (!(g_float_accumulator.mantissa & 0x8000)) {
            g_float_accumulator.mantissa <<= 1;
            g_float_accumulator.exponent--;
        }
    } else {
        /* right-shift mantissa to align/denormalize */
        while (g_float_accumulator.exponent != 0) {
            g_float_accumulator.mantissa >>= 1;
            g_float_accumulator.exponent--;
        }
    }

    return passthrough_param;
}