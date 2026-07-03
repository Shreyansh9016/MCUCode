#include <stdint.h>

/*
 * negate_32bit_at_pointer
 * ---------------------------------------------------------------
 * Two's-complement negation of a 4-byte big-endian value in place
 * at the given pointer (byte[0] = MSB, byte[3] = LSB).
 */
void negate_32bit_at_pointer(uint8_t *value)
{
    value[0] = ~value[0];
    value[1] = ~value[1];
    value[2] = ~value[2];
    value[3] = (uint8_t)(-(int8_t)value[3]);

    if (value[3] == 0) {
        value[2]++;
        if (value[2] == 0) {
            value[1]++;
            if (value[1] == 0) {
                value[0]++;
            }
        }
    }
}