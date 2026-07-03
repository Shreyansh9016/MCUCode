#include <stdint.h>

typedef struct {
    uint8_t  sign_and_exponent;
    uint16_t mantissa;
} mcu_float_t;

/*
 * float_negate_sign_bit
 * ---------------------------------------------------------------
 * Negates a float by flipping its sign bit (bit 7 of the
 * exponent byte), but only if the value is non-zero - avoids
 * producing a negative-zero representation. This is the cheap,
 * correct way to negate this MCU's float format, as opposed to
 * full two's-complement negation (which is only correct for
 * plain integers, see negate_accumulator from an earlier batch).
 */
void float_negate_sign_bit(mcu_float_t *value)
{
    if (value->sign_and_exponent != 0 || value->mantissa != 0) {
        value->sign_and_exponent ^= 0x80;
    }
}