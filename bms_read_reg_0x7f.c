/*
 * bms_read_reg_0x7f.c
 *
 * Original address : 0x0096CB  /  FUN_0096cb
 *
 * Confirmed disassembly:
 *   pushw X
 *   ld A, #0x7F        ; command byte 0x7F
 *   callf 0x00935E     ; afe_read_single_byte(0x7F)  [single-byte read]
 *   popw X; retf
 *
 * BQ76952 command 0x7F = INT_TEMP (internal die temperature).
 * Returns a single byte (8-bit temperature value, 1°C per LSB typically).
 * Uses afe_read_single_byte (FUN_00935E) — NOT the 2-byte afe_read_register —
 * confirming this reads only 1 byte.
 *
 * Called from configure_afe_registers with out_ptr = (uint8_t*)0x01DD.
 */

#include <stdint.h>

#define BQ76952_INT_TEMP_CMD  0x7Fu   /* Internal die temperature */

extern uint8_t afe_read_single_byte(uint8_t cmd);  /* FUN_00935E */

void read_internal_temperature(uint8_t *out_ptr)
{
    *out_ptr = afe_read_single_byte(BQ76952_INT_TEMP_CMD);
}
