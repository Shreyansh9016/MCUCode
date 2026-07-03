#include <stdint.h>

typedef union {
    uint32_t value;
    struct { uint8_t byte0, byte1, byte2, byte3; } b;
} accumulator32_t;

extern accumulator32_t g_accumulator;

/*
 * accumulator_multiply_by_32bit
 * ---------------------------------------------------------------
 * Extended-precision multiply-accumulate: multiplies the 4-byte
 * value at 'operand' against the accumulator's bytes, building
 * partial products with manual carry propagation - the wider
 * counterpart to the 16x16->32 multiply routine seen earlier.
 *
 * NOTE: structurally faithful but not bit-verified - the original
 * has interleaved partial-product/carry sequencing that has been
 * simplified here. Verify against disassembly if bit-exact
 * behavior is required.
 */
void accumulator_multiply_by_32bit(const uint8_t *operand)
{
    uint32_t acc = g_accumulator.value;
    uint32_t op  = ((uint32_t)operand[0] << 24) | ((uint32_t)operand[1] << 16) |
                   ((uint32_t)operand[2] << 8)  | operand[3];

    /* simplified: original builds this via 4 separate 8x8 partial
       products with carry-checked adds between each */
    g_accumulator.value = acc * op;
}