#include <stdint.h>

typedef struct {
    uint8_t  sign_and_exponent;
    uint16_t mantissa;
} mcu_float_t;

extern mcu_float_t g_float_accumulator;

/*
 * signed_float_to_int16
 * ---------------------------------------------------------------
 * Converts the float accumulator back to a signed 16-bit integer.
 * Saturates to 0 if the exponent is out of int16 range. Shifts
 * the mantissa left or right depending on how far the exponent is
 * from the "already an integer" point, then applies the sign via
 * two's-complement negation.
 *
 * NOTE: structural sketch, not bit-verified - the exponent-range
 * threshold checks (the 0x81/'J'/0x4A-style magic offsets) encode
 * exact bit-field boundaries that are easy to get subtly wrong in
 * translation. Verify against disassembly before relying on exact
 * saturation behavior.
 */
int16_t signed_float_to_int16(void)
{
    if (g_float_accumulator.sign_and_exponent == 0) {
        return 0;   /* zero float */
    }

    /* exponent out of representable int16 range -> saturate to 0 */
    /* (exact threshold logic simplified here - see caveat above) */

    uint16_t magnitude = g_float_accumulator.mantissa;
    int8_t   shift_amount = 0;   /* derived from exponent vs. bias - simplified */

    if (shift_amount < 0) {
        magnitude >>= -shift_amount;
    } else {
        magnitude <<= shift_amount;
    }

    int16_t result = (int16_t)magnitude;
    if (g_float_accumulator.sign_and_exponent & 0x80) {
        result = -result;
    }

    return result;
}