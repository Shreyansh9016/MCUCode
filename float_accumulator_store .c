#include <stdint.h>

typedef union {
    uint32_t value;
    struct {
        uint8_t byte0;
        uint8_t byte1;
        uint8_t byte2;
        uint8_t byte3;
    } b;
} accumulator32_t;

extern accumulator32_t g_accumulator;

/*
 * float_accumulator_store
 * ---------------------------------------------------------------
 * Writes the global 4-byte accumulator out to memory at dest.
 * Inverse of float_accumulator_load. This is the function used
 * throughout the calibration-table builders (e.g.
 * build_calibration_table / FUN_008c33) to write each computed
 * table entry after conversion and scaling.
 */
uint8_t float_accumulator_store(uint8_t passthrough_param, uint8_t *dest)
{
    dest[0] = g_accumulator.b.byte0;
    dest[1] = g_accumulator.b.byte1;
    dest[2] = g_accumulator.b.byte2;
    dest[3] = g_accumulator.b.byte3;
    return passthrough_param;
}