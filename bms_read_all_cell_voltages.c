/*
 * bms_read_all_cell_voltages.c
 *
 * Reconstructed from Ghidra decompilation + STM8 disassembly.
 * Original address : 0x0094AA
 * Original name    : FUN_0094aa
 *
 * ---------------------------------------------------------------
 * What this function does  (confirmed from raw disassembly)
 * ---------------------------------------------------------------
 *
 *  Reads all 16 cell voltages from the BQ76952 AFE using
 *  afe_read_register (FUN_009288) and stores them sequentially
 *  into the caller-supplied buffer (2 bytes per cell = 32 bytes).
 *
 *  Confirmed disassembly call sequence:
 *    ld A,#0x14; callf 0x009288 → Cell 1  → buf[0:1]
 *    ld A,#0x16; callf 0x009288 → Cell 2  → buf[2:3]
 *    ld A,#0x18; callf 0x009288 → Cell 3  → buf[4:5]
 *    ld A,#0x1A; callf 0x009288 → Cell 4  → buf[6:7]
 *    ld A,#0x1C; callf 0x009288 → Cell 5  → buf[8:9]
 *    ld A,#0x1E; callf 0x009288 → Cell 6  → buf[10:11]
 *    ld A,#0x20; callf 0x009288 → Cell 7  → buf[12:13]
 *    ld A,#0x22; callf 0x009288 → Cell 8  → buf[14:15]
 *    ld A,#0x24; callf 0x009288 → Cell 9  → buf[16:17]
 *    ld A,#0x26; callf 0x009288 → Cell 10 → buf[18:19]
 *    ld A,#0x28; callf 0x009288 → Cell 11 → buf[20:21]
 *    ld A,#0x2A; callf 0x009288 → Cell 12 → buf[22:23]
 *    ld A,#0x2C; callf 0x009288 → Cell 13 → buf[24:25]
 *    ld A,#0x2E; callf 0x009288 → Cell 14 → buf[26:27]
 *    ld A,#0x30; callf 0x009288 → Cell 15 → buf[28:29]
 *    ld A,#0x32; callf 0x009288 → Cell 16 → buf[30:31]
 *
 *  Register commands 0x14–0x32 match exactly the BQ76952 direct
 *  command map (SLUUBY4 Table 4-1):
 *    0x14 = Cell1Voltage, 0x16 = Cell2Voltage, ... 0x32 = Cell16Voltage
 *  Each register returns a 16-bit value in mV (1mV per LSB).
 *
 *  Buffer pointer advances by +2 between each call (confirmed:
 *  incw X; incw X or addw X, #0x02 patterns in disassembly).
 *
 *  param_1 (called with 0x018F in configure_afe_registers):
 *    The Ghidra shows param_1=399 but disassembly shows it's
 *    actually a pointer to the 32-byte output buffer at 0x018F.
 */

#include <stdint.h>

#define BQ76952_CELL1_VOLTAGE_CMD   0x14u   /* Cell1Voltage  direct cmd */
#define BQ76952_CELL_CMD_STEP       0x02u   /* +2 per cell              */
#define CELL_COUNT                  16u
#define BYTES_PER_CELL              2u      /* 16-bit voltage value     */
#define CELL_VOLTAGE_BUF_SIZE       (CELL_COUNT * BYTES_PER_CELL)  /* 32 */

extern void afe_read_register(uint8_t cmd_byte, uint8_t *out_ptr);  /* FUN_009288 */

/*
 * read_all_cell_voltages
 *
 * Reads all 16 cell voltages from the BQ76952 into buf[].
 * Each cell voltage is 2 bytes (uint16_t, mV per LSB).
 *
 * buf must point to a buffer of at least 32 bytes (16 cells × 2).
 *
 * Called from configure_afe_registers with buf = (uint8_t*)0x018F.
 */
void read_all_cell_voltages(uint8_t *buf)
{
    uint8_t  cell;
    uint8_t  cmd  = BQ76952_CELL1_VOLTAGE_CMD;
    uint8_t *ptr  = buf;

    for (cell = 0u; cell < CELL_COUNT; cell++)
    {
        afe_read_register(cmd, ptr);
        cmd += BQ76952_CELL_CMD_STEP;   /* next cell register address  */
        ptr += BYTES_PER_CELL;          /* next 2-byte output slot     */
    }
}
