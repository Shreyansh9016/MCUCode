#include <stdint.h>

typedef union {
    uint32_t value;
    struct {
        uint8_t byte0;   /* DAT_000000 - MSB */
        uint8_t byte1;
        uint8_t byte2;
        uint8_t byte3;   /* DAT_000003 - LSB */
    } b;
} accumulator32_t;

extern accumulator32_t g_accumulator;

/*
 * float_accumulator_load
 * ---------------------------------------------------------------
 * Loads the global 4-byte accumulator from memory at src.
 * Inverse of float_accumulator_store. Passes param_1 through
 * unchanged (chained return value convention).
 */
uint8_t float_accumulator_load(uint8_t passthrough_param, const uint8_t *src)
{
    g_accumulator.b.byte0 = src[0];
    g_accumulator.b.byte1 = src[1];
    g_accumulator.b.byte2 = src[2];
    g_accumulator.b.byte3 = src[3];
    return passthrough_param;
}