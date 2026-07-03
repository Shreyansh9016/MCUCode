/*
 * bms_read_safety_gh.c
 *
 * Reconstructed from Ghidra decompilation + STM8 disassembly.
 * Original address : 0x00966E
 * Original name    : FUN_00966e
 *
 * Identical structure to bms_read_cell_pair_ab.c / bms_read_cell_pair_cd.c.
 * Reads two single bytes from the BQ76952 AFE using afe_read_single_byte
 * (FUN_00935E) with command bytes 0x0A and 0x0B.
 *
 * Confirmed disassembly (pattern confirmed consistent across all pair readers):
 *   ld A, #0x0A; callf 0x00935E  →  *out_lo = afe_read_single_byte(0x0A)
 *   ld A, #0x0B; callf 0x00935E  →  *out_hi = afe_read_single_byte(0x0B)
 *
 * BQ76952 register / note: G/H bytes — fault flag set 5
 *
 * Call site destinations:
 *   out_lo → RAM[0x01D3]  (cell D byte-0)
 *   out_hi → RAM[0x01D4]  (cell D byte-1)
 */

#include <stdint.h>

#define CMD_LO  0x0Au
#define CMD_HI  0x0Bu

extern uint8_t afe_read_single_byte(uint8_t cmd);  /* FUN_00935E */

void read_safety_status_gh_pair(uint8_t *out_lo, uint8_t *out_hi)
{
    *out_lo = afe_read_single_byte(CMD_LO);
    *out_hi = afe_read_single_byte(CMD_HI);
}
