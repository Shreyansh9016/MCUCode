/*
 * bms_read_reg_0x12.c
 *
 * Original address : 0x0096C2  /  FUN_0096c2
 *
 * Confirmed disassembly:
 *   pushw X            ; save out_ptr
 *   ld A, #0x12        ; BQ76952 command 0x12 = CC2Current (pack current)
 *   callf 0x009288     ; afe_read_register(0x12, out_ptr)
 *   popw X; retf
 *
 * BQ76952 direct command 0x12 = CC2Current (SLUUBY4 Table 4-1):
 *   16-bit signed value, pack current in 10µA per LSB.
 *   Positive = charging, negative = discharging.
 *
 * Called from configure_afe_registers with out_ptr = (uint8_t*)0x01DB.
 */

#include <stdint.h>

#define BQ76952_CC2_CURRENT_CMD  0x12u   /* CC2Current — pack current */

extern void afe_read_register(uint8_t cmd, uint8_t *out_ptr);  /* FUN_009288 */

void read_pack_current(uint8_t *out_ptr)
{
    afe_read_register(BQ76952_CC2_CURRENT_CMD, out_ptr);
}
