/*
 * bms_configure_afe_registers.c
 *
 * Reconstructed from Ghidra decompilation + STM8 disassembly.
 * Original address : 0x0091FB
 * Original name    : FUN_0091fb
 *
 * ---------------------------------------------------------------
 * What this function does  (confirmed from raw disassembly)
 * ---------------------------------------------------------------
 *
 *  This is the AFE REGISTER CONFIGURATION function — called after
 *  init_measurement_buffers() to program specific BQ76952 register
 *  values into the RAM-mirrored configuration table.
 *
 *  Each call in this function writes one or more configuration
 *  values into RAM addresses in the range 0x018F–0x01E0 (the
 *  same region cleared by init_measurement_buffers).
 *
 *  Confirmed disassembly call sequence with exact addresses:
 *
 *    ldw X,#0x018F; callf 0x0094AA  → config_block_a(0x018F, 399)
 *    ldw X,#0x01AF; callf 0x009577  → config_block_b(0x01AF)
 *    ldw X,#0x01C3; callf 0x0095F6  → config_block_c(0x01C3)
 *    ldw X,#0x01CB; callf 0x009627  → config_single(0x01CB)
 *    ldw X,#0x01CD; push 0x01D0; callf 0x00962F  → config_pair(0x01CD, 0x01D0, 0x2201)
 *    ldw X,#0x01CE; push 0x01D1; callf 0x009644  → config_pair(0x01CE, 0x01D1, 0x2E01)
 *    ldw X,#0x01CF; push 0x01D2; callf 0x009659  → config_pair(0x01CF, 0x01D2, 0x3A01)
 *    ldw X,#0x01D3; push 0x01D4; callf 0x00966E  → config_pair(0x01D3, 0x01D4, 0x4601)
 *    ldw X,#0x01D5; push 0x01D6; callf 0x009683  → config_pair(0x01D5, 0x01D6, 0x5201)
 *    ldw X,#0x01D7; push 0x01D8; callf 0x009698  → config_pair(0x01D7, 0x01D8, 0x5E01)
 *    ldw X,#0x01D9; push 0x01DA; callf 0x0096AD  → config_pair(0x01D9, 0x01DA, 0x6A01)
 *    ldw X,#0x01DB; callf 0x0096C2               → config_word(0x01DB)
 *    ldw X,#0x01DD; callf 0x0096CB               → config_byte(0x01DD)
 *    ldw X,#0x01DE; callf 0x0096D4               → config_word(0x01DE)
 *    ldw X,#0x01E0; callf 0x0096DD               → config_word(0x01E0)
 *
 *  The second argument values passed to the "config_pair" calls
 *  (0x2201, 0x2E01, 0x3A01, 0x4601, 0x5201, 0x5E01, 0x6A01)
 *  increment by 0x0C00 each time — this is a known BQ76952
 *  pattern for programming cell voltage register addresses:
 *    0x22 = first cell address, stepping by 0x0C (12 decimal)
 *    per cell in the BQ76952 direct command address map.
 *
 *  "399" (0x018F call) = 0x18F which is the register offset
 *  for the first configuration block — likely sampling period,
 *  oversampling count, or gain calibration value.
 */

#include <stdint.h>

/* ── Forward declarations for AFE register config helpers ── */
/* These correspond to the called sub-functions in the disassembly.    */
/* Their exact signatures will be clear once you decode 0x0094AA etc.  */

extern void afe_config_block_a  (uint8_t *dest, uint16_t value);     /* FUN_0094AA */
extern void afe_config_block_b  (uint8_t *dest);                     /* FUN_009577 */
extern void afe_config_block_c  (uint8_t *dest);                     /* FUN_0095F6 */
extern void afe_config_single   (uint8_t *dest);                     /* FUN_009627 */
extern void afe_config_pair     (uint8_t *dest_lo,                   /* FUN_00962F */
                                 uint8_t *dest_hi, uint16_t reg_cmd);
extern void afe_config_pair_2e  (uint8_t *dest_lo,                   /* FUN_009644 */
                                 uint8_t *dest_hi, uint16_t reg_cmd);
extern void afe_config_pair_3a  (uint8_t *dest_lo,                   /* FUN_009659 */
                                 uint8_t *dest_hi, uint16_t reg_cmd);
extern void afe_config_pair_46  (uint8_t *dest_lo,                   /* FUN_00966E */
                                 uint8_t *dest_hi, uint16_t reg_cmd);
extern void afe_config_pair_52  (uint8_t *dest_lo,                   /* FUN_009683 */
                                 uint8_t *dest_hi, uint16_t reg_cmd);
extern void afe_config_pair_5e  (uint8_t *dest_lo,                   /* FUN_009698 */
                                 uint8_t *dest_hi, uint16_t reg_cmd);
extern void afe_config_pair_6a  (uint8_t *dest_lo,                   /* FUN_0096AD */
                                 uint8_t *dest_hi, uint16_t reg_cmd);
extern void afe_config_word_db  (uint8_t *dest);                     /* FUN_0096C2 */
extern void afe_config_byte_dd  (uint8_t *dest);                     /* FUN_0096CB */
extern void afe_config_word_de  (uint8_t *dest);                     /* FUN_0096D4 */
extern void afe_config_word_e0  (uint8_t *dest);                     /* FUN_0096DD */

/* ── RAM config table addresses ── */
#define CFG_BLOCK_A         ((uint8_t *)0x018F)
#define CFG_BLOCK_B         ((uint8_t *)0x01AF)
#define CFG_BLOCK_C         ((uint8_t *)0x01C3)
#define CFG_SINGLE_CB       ((uint8_t *)0x01CB)
#define CFG_CELL_A_LO       ((uint8_t *)0x01CD)
#define CFG_CELL_A_HI       ((uint8_t *)0x01D0)
#define CFG_CELL_B_LO       ((uint8_t *)0x01CE)
#define CFG_CELL_B_HI       ((uint8_t *)0x01D1)
#define CFG_CELL_C_LO       ((uint8_t *)0x01CF)
#define CFG_CELL_C_HI       ((uint8_t *)0x01D2)
#define CFG_CELL_D_LO       ((uint8_t *)0x01D3)
#define CFG_CELL_D_HI       ((uint8_t *)0x01D4)
#define CFG_CELL_E_LO       ((uint8_t *)0x01D5)
#define CFG_CELL_E_HI       ((uint8_t *)0x01D6)
#define CFG_CELL_F_LO       ((uint8_t *)0x01D7)
#define CFG_CELL_F_HI       ((uint8_t *)0x01D8)
#define CFG_CELL_G_LO       ((uint8_t *)0x01D9)
#define CFG_CELL_G_HI       ((uint8_t *)0x01DA)
#define CFG_WORD_DB         ((uint8_t *)0x01DB)
#define CFG_BYTE_DD         ((uint8_t *)0x01DD)
#define CFG_WORD_DE         ((uint8_t *)0x01DE)
#define CFG_WORD_E0         ((uint8_t *)0x01E0)

/*
 * BQ76952 cell voltage register command codes.
 * These step by 0x0C00 per cell — confirmed from the 0x2201,
 * 0x2E01, 0x3A01... pattern in the call arguments.
 * 0x22 is the base direct command for cell 1 voltage in the
 * BQ76952 register map (SLUUBY4 Table 4-1).
 */
#define BQ76952_CELL_A_CMD   0x2201u
#define BQ76952_CELL_B_CMD   0x2E01u
#define BQ76952_CELL_C_CMD   0x3A01u
#define BQ76952_CELL_D_CMD   0x4601u
#define BQ76952_CELL_E_CMD   0x5201u
#define BQ76952_CELL_F_CMD   0x5E01u
#define BQ76952_CELL_G_CMD   0x6A01u

/*
 * configure_afe_registers
 *
 * Programs all AFE configuration values into the RAM mirror table
 * at 0x018F–0x01E0. Called once after init_measurement_buffers()
 * before starting the main measurement cycle.
 *
 * Must be called with the AFE already initialised (afe_hardware_init
 * done) but before the first active measurement read.
 */
void configure_afe_registers(void)
{
    /* Block A: main config parameters (e.g. sampling rate = 399) */
    afe_config_block_a(CFG_BLOCK_A, 399u);

    /* Block B: secondary config (20-byte region at 0x01AF) */
    afe_config_block_b(CFG_BLOCK_B);

    /* Block C: conversion result config (8-byte region at 0x01C3) */
    afe_config_block_c(CFG_BLOCK_C);

    /* Single word config at 0x01CB */
    afe_config_single(CFG_SINGLE_CB);

    /*
     * Per-cell register address programming.
     * Each pair writes the BQ76952 direct command address for
     * that cell's voltage register into lo/hi slots.
     * Command codes step by 0x0C (12) per cell — confirmed from
     * the 0x2201 → 0x2E01 → 0x3A01 ... pattern.
     */
    afe_config_pair   (CFG_CELL_A_LO, CFG_CELL_A_HI, BQ76952_CELL_A_CMD);
    afe_config_pair_2e(CFG_CELL_B_LO, CFG_CELL_B_HI, BQ76952_CELL_B_CMD);
    afe_config_pair_3a(CFG_CELL_C_LO, CFG_CELL_C_HI, BQ76952_CELL_C_CMD);
    afe_config_pair_46(CFG_CELL_D_LO, CFG_CELL_D_HI, BQ76952_CELL_D_CMD);
    afe_config_pair_52(CFG_CELL_E_LO, CFG_CELL_E_HI, BQ76952_CELL_E_CMD);
    afe_config_pair_5e(CFG_CELL_F_LO, CFG_CELL_F_HI, BQ76952_CELL_F_CMD);
    afe_config_pair_6a(CFG_CELL_G_LO, CFG_CELL_G_HI, BQ76952_CELL_G_CMD);

    /* Pack-level measurement config slots */
    afe_config_word_db(CFG_WORD_DB);   /* pack voltage / current word   */
    afe_config_byte_dd(CFG_BYTE_DD);   /* status / flag byte            */
    afe_config_word_de(CFG_WORD_DE);   /* pack word 2                   */
    afe_config_word_e0(CFG_WORD_E0);   /* pack word 3                   */
}
