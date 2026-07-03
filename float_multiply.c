#include <stdint.h>

typedef struct {
    uint8_t  sign_and_exponent;
    uint16_t mantissa;
} mcu_float_t;

extern mcu_float_t g_float_accumulator;

/*
 * float_multiply
 * ---------------------------------------------------------------
 * Multiplies the global float accumulator in place by the float
 * pointed to by operand. Exponents add, signs XOR, mantissas
 * multiply (via repeated carry-accumulated partial products -
 * see the FUN_00f550/f55b/f55e/f56d helper cluster in the
 * original, treated here as inlined implementation detail).
 *
 * CONFIRMED ROLE: this is the operation used throughout the
 * calibration-table builders (FUN_008c33, FUN_008edf, etc.) to
 * scale a just-converted raw value by a fixed gain constant
 * stored in flash (e.g. the constant at 0x808c).
 *
 * NOTE: structural sketch only - does not reproduce the exact
 * bit-level rounding/carry behavior of the original. Sufficient
 * for understanding intent; not for bit-exact reimplementation.
 */
void float_multiply(const mcu_float_t *operand)
{
    if (g_float_accumulator.sign_and_exponent == 0 &&
        g_float_accumulator.mantissa == 0) {
        return;   /* multiplying by/into zero - no-op */
    }

    uint8_t result_sign = (g_float_accumulator.sign_and_exponent & 0x80) ^
                           (operand->sign_and_exponent & 0x80);

    uint8_t exp_a = g_float_accumulator.sign_and_exponent & 0x7F;
    uint8_t exp_b = operand->sign_and_exponent & 0x7F;
    uint8_t result_exponent = exp_a + exp_b - 0x7F;   /* bias correction, approximate */

    uint32_t mantissa_product = (uint32_t)g_float_accumulator.mantissa * operand->mantissa;

    g_float_accumulator.mantissa = (uint16_t)(mantissa_product >> 16);
    g_float_accumulator.sign_and_exponent = result_sign | (result_exponent & 0x7F);
}