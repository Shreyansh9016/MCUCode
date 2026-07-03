/*
 * bms_read_cell_pair_cd.c
 *
 * Reconstructed from Ghidra decompilation + STM8 disassembly.
 * Original address : 0x009644
 * Original name    : FUN_009644
 *
 * ---------------------------------------------------------------
 * What this function does  (confirmed from raw disassembly)
 * ---------------------------------------------------------------
 *
 *  Identical structure to bms_read_cell_pair_ab.c (FUN_00962F).
 *  Reads TWO single bytes using afe_read_single_byte (FUN_00935E)
 *  with command bytes 0x04 and 0x05 instead of 0x02 and 0x03.
 *
 *  Confirmed disassembly:
 *    pushw X; pushw X
 *    ld    A, #0x04         ; command 0x04
 *    callf 0x00935E         → afe_read_single_byte(0x04)
 *    popw  X
 *    ldw   X, (0x06,SP)     ; load param_2
 *    pushw X
 *    ld    A, #0x05         ; command 0x05
 *    callf 0x00935E         → afe_read_single_byte(0x05)
 *    popw X; popw X; retf
 *
 *  BQ76952 register mapping for commands 0x04 and 0x05:
 *    0x04 = SafetyStatusC  (low byte  — tertiary protection faults)
 *    0x05 = SafetyStatusC  (high byte — tertiary protection faults)
 *
 *  SafetyStatusC bits (SLUUBY4 Table 4-1):
 *    bit 0  = UTCFET (undertemperature charge FET)
 *    bit 2  = UTDFET (undertemperature discharge FET)
 *    bit 4  = OTCFET (overtemperature charge FET)
 *    bit 6  = OTDFET (overtemperature discharge FET)
 *    bit 8  = VIMBALANCE (voltage imbalance)
 *    bit 10 = COVL  (cell overvoltage latch)
 *
 *  Called from configure_afe_registers:
 *    param_1 (out_lo) = (uint8_t*)0x01CE  (cell B byte-0 slot)
 *    param_2 (out_hi) = (uint8_t*)0x01D1  (cell B byte-1 slot)
 *
 *  Note: Adjacent to FUN_00962F in both address space and output
 *  slots — together they unpack SafetyStatusB and SafetyStatusC
 *  into consecutive RAM positions for the state machine.
 */

#include <stdint.h>

#define BQ76952_SAFETY_STATUS_C_LO_CMD  0x04u
#define BQ76952_SAFETY_STATUS_C_HI_CMD  0x05u

extern uint8_t afe_read_single_byte(uint8_t cmd_byte);  /* FUN_00935E */

/*
 * read_safety_status_c_pair
 *
 * Reads SafetyStatusC low byte and high byte as two separate
 * single-byte transactions and stores them in separate slots.
 *
 *  out_lo ← byte from cmd 0x04 → stored at RAM[0x01CE]
 *  out_hi ← byte from cmd 0x05 → stored at RAM[0x01D1]
 */
void read_safety_status_c_pair(uint8_t *out_lo, uint8_t *out_hi)
{
    *out_lo = afe_read_single_byte(BQ76952_SAFETY_STATUS_C_LO_CMD);
    *out_hi = afe_read_single_byte(BQ76952_SAFETY_STATUS_C_HI_CMD);
}
