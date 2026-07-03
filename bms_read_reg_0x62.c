/*
 * bms_read_reg_0x62.c
 *
 * Original address : 0x0096D4  /  FUN_0096d4
 *
 * Confirmed disassembly:
 *   pushw X
 *   ld A, #0x62        ; BQ76952 command 0x62 = StackVoltage
 *   callf 0x009288     ; afe_read_register(0x62, out_ptr)
 *   popw X; retf
 *
 * BQ76952 direct command 0x62 = StackVoltage (SLUUBY4 Table 4-1):
 *   16-bit unsigned, total stack voltage in 1mV per LSB.
 *
 * Called from configure_afe_registers with out_ptr = (uint8_t*)0x01DE.
 */

#include <stdint.h>

#define BQ76952_STACK_VOLTAGE_CMD  0x62u   /* StackVoltage */

extern void afe_read_register(uint8_t cmd, uint8_t *out_ptr);

void read_stack_voltage(uint8_t *out_ptr)
{
    afe_read_register(BQ76952_STACK_VOLTAGE_CMD, out_ptr);
}
