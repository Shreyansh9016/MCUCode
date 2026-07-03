/*
 * bms_read_reg_0x64.c
 *
 * Original address : 0x0096DD  /  FUN_0096dd
 *
 * Confirmed disassembly:
 *   pushw X
 *   ld A, #0x64        ; BQ76952 command 0x64 = PackVoltage
 *   callf 0x009288     ; afe_read_register(0x64, out_ptr)
 *   popw X; retf
 *
 * BQ76952 direct command 0x64 = PackVoltage (SLUUBY4 Table 4-1):
 *   16-bit unsigned, total pack voltage (including PACK+ to PACK-)
 *   in 1mV per LSB.
 *
 * Called from configure_afe_registers with out_ptr = (uint8_t*)0x01E0.
 */

#include <stdint.h>

#define BQ76952_PACK_VOLTAGE_CMD  0x64u   /* PackVoltage */

extern void afe_read_register(uint8_t cmd, uint8_t *out_ptr);

void read_pack_voltage(uint8_t *out_ptr)
{
    afe_read_register(BQ76952_PACK_VOLTAGE_CMD, out_ptr);
}
