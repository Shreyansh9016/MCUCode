#include <stdint.h>

/*
 * subtract_byte_from_32bit
 * ---------------------------------------------------------------
 * Subtracts a single-byte scalar from the least-significant byte
 * of a 4-byte big-endian value at dest, propagating the borrow
 * up through bytes 2, 1, and 0 as needed. Returns the final
 * byte-2 result (matches original return value semantics).
 */
int8_t subtract_byte_from_32bit(int8_t scalar, char *dest)
{
    uint8_t original_byte3 = (uint8_t)dest[3];
    int8_t  result = (int8_t)(-scalar + (uint8_t)dest[3]);
    dest[3] = result;

    /* borrow occurred if no carry out of the subtraction */
    if (/* no carry */ (uint8_t)(-scalar) + original_byte3 <= 0xFF &&
        (uint8_t)(-scalar + original_byte3) < original_byte3) {
        int8_t byte2 = dest[2];
        dest[2] = dest[2] - 1;
        if (byte2 == 0) {
            if (dest[1] == 0) {
                dest[0] = dest[0] - 1;
            }
            dest[1] = dest[1] - 1;
        }
        result = byte2;
    }

    return result;
}