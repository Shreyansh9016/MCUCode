#include <stdint.h>

/*
 * signed_div_remainder
 * ---------------------------------------------------------------
 * Signed division: returns (param_2 % param_1), correctly signed.
 * Identical logic to signed_div_quotient, except the initial flag
 * byte (0x80) forces the "return remainder" path unconditionally.
 */
int16_t signed_div_remainder(int8_t divisor, int16_t dividend)
{
    uint8_t sign_flags = 0x80;   /* top bit forces remainder path below */
    uint8_t dividend_was_negative = 0;

    if (dividend < 0) {
        dividend = -dividend;
        sign_flags = 0x83;
    }
    if (divisor < 0) {
        divisor = -divisor;
        sign_flags ^= 2;
    }

    int16_t remainder = (uint16_t)dividend % (uint8_t)divisor;
    dividend_was_negative = sign_flags & 1;

    /* top bit of sign_flags survives the shift, so this branch
       always triggers */
    if (dividend_was_negative) {
        return (int16_t)-((uint16_t)dividend % (uint8_t)divisor);
    }

    return remainder;
}