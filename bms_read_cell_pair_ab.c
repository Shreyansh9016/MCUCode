/*
 * bms_read_cell_pair_ab.c
 *
 * Reconstructed from Ghidra decompilation + STM8 disassembly.
 * Original address : 0x00962F
 * Original name    : FUN_00962f
 *
 * ---------------------------------------------------------------
 * What this function does  (confirmed from raw disassembly)
 * ---------------------------------------------------------------
 *
 *  Reads TWO single bytes from the BQ76952 using afe_read_single_byte
 *  (FUN_00935E), one per register command, storing each result
 *  into a separate output slot.
 *
 *  Confirmed disassembly:
 *    pushw X               ; save param_1 = out_ptr_lo
 *    pushw X               ; allocate slot
 *    ld    A, #0x02        ; command 0x02 = SafetyStatusB byte 0
 *    callf 0x00935E        ; afe_read_single_byte(0x02) → slot_lo
 *    popw  X
 *    ldw   X, (0x06,SP)    ; load param_2 = out_ptr_hi (second arg)
 *    pushw X
 *    ld    A, #0x03        ; command 0x03 = SafetyStatusB byte 1
 *    callf 0x00935E        ; afe_read_single_byte(0x03) → slot_hi
 *    popw  X; popw X
 *    retf
 *
 *  BQ76952 register mapping for commands 0x02 and 0x03:
 *    0x02 = SafetyStatusB  (low byte  — secondary protection faults)
 *    0x03 = SafetyStatusB  (high byte — secondary protection faults)
 *
 *  Called from configure_afe_registers:
 *    param_1 (out_ptr_lo) = (uint8_t*)0x01CD  (cell A byte-0 slot)
 *    param_2 (out_ptr_hi) = (uint8_t*)0x01D0  (cell A byte-1 slot)
 */

#include <stdint.h>

#define BQ76952_SAFETY_STATUS_B_LO_CMD  0x02u
#define BQ76952_SAFETY_STATUS_B_HI_CMD  0x03u

extern uint8_t afe_read_single_byte(uint8_t cmd_byte);  /* FUN_00935E */

/*
 * read_safety_status_b_pair
 *
 * Reads SafetyStatusB low byte and high byte as two separate
 * single-byte transactions and stores them in separate slots.
 *
 *  out_lo ← byte from cmd 0x02 → stored at RAM[0x01CD]
 *  out_hi ← byte from cmd 0x03 → stored at RAM[0x01D0]
 */
void read_safety_status_b_pair(uint8_t *out_lo, uint8_t *out_hi)
{
    *out_lo = afe_read_single_byte(BQ76952_SAFETY_STATUS_B_LO_CMD);
    *out_hi = afe_read_single_byte(BQ76952_SAFETY_STATUS_B_HI_CMD);
}
