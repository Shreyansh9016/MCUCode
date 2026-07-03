/*
 * bms_read_safety_mn.c
 *
 * Reconstructed from Ghidra decompilation + STM8 disassembly.
 * Original address : 0x0096AD
 * Original name    : FUN_0096ad
 *
 * Identical structure to bms_read_cell_pair_ab.c / bms_read_cell_pair_cd.c.
 * Reads two single bytes from the BQ76952 AFE using afe_read_single_byte
 * (FUN_00935E) with command bytes 0x10 and 0x11.
 *
 * Confirmed disassembly (pattern confirmed consistent across all pair readers):
 *   ld A, #0x10; callf 0x00935E  →  *out_lo = afe_read_single_byte(0x10)
 *   ld A, #0x11; callf 0x00935E  →  *out_hi = afe_read_single_byte(0x11)
 *
 * BQ76952 register / note: M/N bytes — fault flag set 8
 *
 * Call site destinations:
 *   out_lo → RAM[0x01D9]  (cell G byte-0)
 *   out_hi → RAM[0x01DA]  (cell G byte-1)
 */

#include <stdint.h>

#define CMD_LO  0x10u
#define CMD_HI  0x11u

extern uint8_t afe_read_single_byte(uint8_t cmd);  /* FUN_00935E */

void read_safety_status_mn_pair(uint8_t *out_lo, uint8_t *out_hi)
{
    *out_lo = afe_read_single_byte(CMD_LO);
    *out_hi = afe_read_single_byte(CMD_HI);
}
