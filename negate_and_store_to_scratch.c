#include <stdint.h>

extern void unknown_load_f2bd(void *value);   /* FUN_00f2bd - not yet analyzed */
extern void float_negate_sign_bit(void *accumulator);   /* FUN_00f57a */
extern uint8_t float_accumulator_store(uint8_t passthrough, uint8_t *dest);

/*
 * negate_and_store_to_scratch
 * ---------------------------------------------------------------
 * Loads a value, negates the accumulator's sign, stores to a
 * fixed scratch table slot (0xA09C).
 */
void negate_and_store_to_scratch(void *value)
{
    unknown_load_f2bd(value);
    float_negate_sign_bit(NULL);
    float_accumulator_store(0, (uint8_t *)0xA09C);
}