#include <stdint.h>

/*
 * detect_denormal_underflow
 * ---------------------------------------------------------------
 * Checks a 4-byte value for the pattern: most-significant byte is
 * zero, but the value is not actually zero overall (one of the
 * remaining 3 bytes is non-zero). This is the classic signature
 * of a float's exponent having underflowed to zero while its
 * mantissa is still non-zero - i.e. a denormal/underflow flag.
 * Returns 1 if the pattern is detected, otherwise returns
 * passthrough_param unchanged.
 */
uint8_t detect_denormal_underflow(uint8_t passthrough_param, const char *value)
{
    if (value[0] == 0 &&
        (value[1] != 0 || value[2] != 0 || value[3] != 0)) {
        return 1;
    }
    return passthrough_param;
}