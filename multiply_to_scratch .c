#include <stdint.h>

typedef struct {
    uint8_t  sign_and_exponent;
    uint16_t mantissa;
} mcu_float_t;

extern void float_multiply(const mcu_float_t *operand);
extern uint8_t float_accumulator_store(uint8_t passthrough, uint8_t *dest);

/*
 * multiply_to_scratch
 * ---------------------------------------------------------------
 * Multiplies the accumulator by an external float, then stores
 * the result to a fixed scratch table slot (0x87-range address).
 */
void multiply_to_scratch(mcu_float_t *operand)
{
    float_multiply(operand);

    uint8_t ptr_high_byte = (uint8_t)((uintptr_t)operand >> 8);
    float_accumulator_store(0x87, (uint8_t *)(uintptr_t)((0x87 << 8) | ptr_high_byte));
}