#include <stdint.h>

typedef union {
    uint32_t value;
    struct { uint8_t byte0, byte1, byte2, byte3; } b;
} accumulator32_t;

extern accumulator32_t g_accumulator;

/*
 * negate_accumulator
 * ---------------------------------------------------------------
 * Two's-complement negation of the global 32-bit accumulator,
 * in place. Same logic as negate_32bit_at_pointer, hard-wired to
 * the accumulator instead of an arbitrary pointer.
 */
void negate_accumulator(void)
{
    g_accumulator.b.byte0 = ~g_accumulator.b.byte0;
    g_accumulator.b.byte1 = ~g_accumulator.b.byte1;
    g_accumulator.b.byte2 = ~g_accumulator.b.byte2;
    g_accumulator.b.byte3 = (uint8_t)(-(int8_t)g_accumulator.b.byte3);

    if (g_accumulator.b.byte3 == 0) {
        g_accumulator.b.byte2++;
        if (g_accumulator.b.byte2 == 0) {
            g_accumulator.b.byte1++;
            if (g_accumulator.b.byte1 == 0) {
                g_accumulator.b.byte0++;
            }
        }
    }
}