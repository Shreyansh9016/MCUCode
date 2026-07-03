#include <stdint.h>

/*
 * signed_div_quotient
 * ---------------------------------------------------------------
 * Signed division: returns (param_2 / param_1), correctly signed.
 * param_1 = divisor (signed byte), param_2 = dividend (signed short).
 * Shares its core logic with signed_div_remainder - the two are
 * the same routine, differing only in which result they return.
 */
int16_t signed_div_quotient(int8_t divisor, int16_t dividend)
{
    uint8_t sign_flags = 0;   /* bit1 = result sign, bit0 = unused for quotient path */

    if (dividend < 0) {
        dividend = -dividend;
        sign_flags = 3;
    }
    if (divisor < 0) {
        divisor = -divisor;
        sign_flags ^= 2;
    }

    int16_t quotient = (uint16_t)dividend / (uint8_t)divisor;

    /* sign_flags top bit stays clear here (started at 0), so this
       always takes the quotient path */
    if (sign_flags & 2) {
        quotient = -quotient;
    }

    return quotient;
}