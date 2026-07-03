#include <stdint.h>

typedef struct {
    uint8_t  sign_and_exponent;
    uint16_t mantissa;
} mcu_float_t;

extern mcu_float_t g_float_accumulator;
extern void float_subtract(mcu_float_t *operand);   /* FUN_00f3d0 - not yet analyzed, assumed subtract */
extern uint8_t float_accumulator_store(uint8_t passthrough, uint8_t *dest);   /* FUN_00f817 */

/*
 * reversed_subtract_to_scratch
 * ---------------------------------------------------------------
 * Swaps the accumulator with an external float, runs the normal
 * subtract routine (accumulator -= operand), which after the
 * swap effectively computes (external_value - original_accumulator),
 * then stores the result to a fixed scratch table slot (0x7D-range
 * address, offset by the pointer's high byte).
 */
void reversed_subtract_to_scratch(mcu_float_t *external_value)
{
    mcu_float_t temp = *external_value;
    *external_value = g_float_accumulator;
    g_float_accumulator = temp;

    float_subtract(external_value);

    uint8_t ptr_high_byte = (uint8_t)((uintptr_t)external_value >> 8);
    float_accumulator_store(0x7D, (uint8_t *)(uintptr_t)((0x7D << 8) | ptr_high_byte));
}