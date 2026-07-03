#include <stdint.h>
#include <stddef.h>

/*
 * STM8 compiler runtime library memcpy.
 * Calling convention (STM8 "cvreg" style):
 *   A  = count   (param_1, byte, 0-255)
 *   Y  = source  (in_Y - passed in register, not visible in decompiled args)
 *   dest passed on stack (param_2)
 * Copies in 4-byte chunks with a 1-3 byte tail.
 */
void *memcpy_stm8(uint8_t count, void *dest, const void *src)
{
    uint8_t       *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    void          *ret = dest;

    while (count >= 4) {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
        d[3] = s[3];
        d += 4;
        s += 4;
        count -= 4;
    }

    /* tail: 0, 1, 2, or 3 remaining bytes */
    if (count > 0) {
        d[0] = s[0];
        if (--count > 0) {
            d[1] = s[1];
            if (--count > 0) {
                d[2] = s[2];
            }
        }
    }

    return ret;
}