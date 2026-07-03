/*
 * bms_read_safety_ef.c
 *
 * Reconstructed from Ghidra decompilation + STM8 disassembly.
 * Original address : 0x009659
 * Original name    : FUN_009659
 *
 * Identical structure to bms_read_cell_pair_ab.c / bms_read_cell_pair_cd.c.
 * Reads two single bytes from the BQ76952 AFE using afe_read_single_byte
 * (FUN_00935E) with command bytes 0x06 and 0x07.
 *
 * Confirmed disassembly (pattern confirmed consistent across all pair readers):
 *   ld A, #0x06; callf 0x00935E  →  *out_lo = afe_read_single_byte(0x06)
 *   ld A, #0x07; callf 0x00935E  →  *out_hi = afe_read_single_byte(0x07)
 *
 * BQ76952 register / note: E/F bytes — protection overtemperature flags
 *
 * Call site destinations:
 *   out_lo → RAM[0x01CF]  (cell C byte-0)
 *   out_hi → RAM[0x01D2]  (cell C byte-1)
 */

#include <stdint.h>

#define CMD_LO  0x06u
#define CMD_HI  0x07u

extern uint8_t afe_read_single_byte(uint8_t cmd);  /* FUN_00935E */

void read_safety_status_ef_pair(uint8_t *out_lo, uint8_t *out_hi)
{
    *out_lo = afe_read_single_byte(CMD_LO);
    *out_hi = afe_read_single_byte(CMD_HI);
}
