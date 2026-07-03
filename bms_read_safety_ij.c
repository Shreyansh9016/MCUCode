/*
 * bms_read_safety_ij.c
 *
 * Reconstructed from Ghidra decompilation + STM8 disassembly.
 * Original address : 0x009683
 * Original name    : FUN_009683
 *
 * Identical structure to bms_read_cell_pair_ab.c / bms_read_cell_pair_cd.c.
 * Reads two single bytes from the BQ76952 AFE using afe_read_single_byte
 * (FUN_00935E) with command bytes 0x0C and 0x0D.
 *
 * Confirmed disassembly (pattern confirmed consistent across all pair readers):
 *   ld A, #0x0C; callf 0x00935E  →  *out_lo = afe_read_single_byte(0x0C)
 *   ld A, #0x0D; callf 0x00935E  →  *out_hi = afe_read_single_byte(0x0D)
 *
 * BQ76952 register / note: I/J bytes — fault flag set 6
 *
 * Call site destinations:
 *   out_lo → RAM[0x01D5]  (cell E byte-0)
 *   out_hi → RAM[0x01D6]  (cell E byte-1)
 */

#include <stdint.h>

#define CMD_LO  0x0Cu
#define CMD_HI  0x0Du

extern uint8_t afe_read_single_byte(uint8_t cmd);  /* FUN_00935E */

void read_safety_status_ij_pair(uint8_t *out_lo, uint8_t *out_hi)
{
    *out_lo = afe_read_single_byte(CMD_LO);
    *out_hi = afe_read_single_byte(CMD_HI);
}
