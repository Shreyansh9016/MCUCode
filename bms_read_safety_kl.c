/*
 * bms_read_safety_kl.c
 *
 * Reconstructed from Ghidra decompilation + STM8 disassembly.
 * Original address : 0x009698
 * Original name    : FUN_009698
 *
 * Identical structure to bms_read_cell_pair_ab.c / bms_read_cell_pair_cd.c.
 * Reads two single bytes from the BQ76952 AFE using afe_read_single_byte
 * (FUN_00935E) with command bytes 0x0E and 0x0F.
 *
 * Confirmed disassembly (pattern confirmed consistent across all pair readers):
 *   ld A, #0x0E; callf 0x00935E  →  *out_lo = afe_read_single_byte(0x0E)
 *   ld A, #0x0F; callf 0x00935E  →  *out_hi = afe_read_single_byte(0x0F)
 *
 * BQ76952 register / note: K/L bytes — fault flag set 7
 *
 * Call site destinations:
 *   out_lo → RAM[0x01D7]  (cell F byte-0)
 *   out_hi → RAM[0x01D8]  (cell F byte-1)
 */

#include <stdint.h>

#define CMD_LO  0x0Eu
#define CMD_HI  0x0Fu

extern uint8_t afe_read_single_byte(uint8_t cmd);  /* FUN_00935E */

void read_safety_status_kl_pair(uint8_t *out_lo, uint8_t *out_hi)
{
    *out_lo = afe_read_single_byte(CMD_LO);
    *out_hi = afe_read_single_byte(CMD_HI);
}
