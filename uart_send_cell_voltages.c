/*
 * uart_send_cell_voltages.c
 *
 * Original address : 0x00BD7B  /  FUN_00bd7b
 * Function name    : uart_send_cell_voltages
 *
 * ---------------------------------------------------------------
 * What this function does
 * ---------------------------------------------------------------
 *
 * Sends the 16 individual cell voltages over UART in a formatted
 * output block. Called from bms_main_loop's periodic cycle.
 *
 * Confirmed disassembly:
 *   1. Sets local_28[0] = 0x85 (stack sentinel / format selector)
 *   2. Calls copy_measurement_block_160bytes into local_28+1
 *      → fills a 160-byte local buffer with all measurement data
 *      (cell voltages are at offsets 0x2C..0x4B in the struct,
 *       which maps to local_28[0x29]..local_28[0x47] here)
 *   3. Calls FUN_00E3B1 (uart_print_formatted) 12 times with
 *      consecutive string table addresses:
 *        0x8703 → "cellXvoltage %d.%dV" style strings
 *        0x8640, 0x862A, 0x8601, 0x85D8, 0x85AF,
 *        0x8586, 0x855D, 0x8540, 0x8521, 0x8508
 *      (11 strings total = 11 measurement values printed)
 *   4. Returns 0x1300 (arbitrary — return value ignored by caller)
 *
 * The string addresses 0x8703..0x8508 step backwards through the
 * string table, matching our early analysis where we found the
 * string table starts at 0x8260 and covers cell1voltage through
 * cell16voltage, temperatures, and fault flags.
 *
 * This function prints the "lower half" of cell voltages (cells
 * ~5-16 depending on string mapping). FUN_00BE17 prints the
 * corresponding fault/status values.
 */

#include <stdint.h>

#define CELL_VOLTAGE_BUF_SIZE  160u

typedef const char * flash_str_t;

extern void uart_print_formatted(flash_str_t str_addr, ...);    /* FUN_00E3B1        */
extern void copy_measurement_block_160bytes(uint8_t *dst);      /* FUN_009077        */

uint16_t uart_send_cell_voltages(void)
{
    uint8_t buf[CELL_VOLTAGE_BUF_SIZE + 1u];

    /* Sentinel byte — selects the print format variant */
    buf[0] = 0x85u;

    /* Fill buffer with all 160 bytes of measurement data */
    copy_measurement_block_160bytes(buf + 1u);

    /* Print 11 cell measurement strings from the flash string table.
     * Each call to uart_print_formatted picks the relevant bytes
     * out of buf[] based on the format string's %d/%d.%d fields.
     * Strings step backwards through the table (most recent cell first). */
    uart_print_formatted((flash_str_t)0x8703u);   /* cell voltage header / cell 16 */
    uart_print_formatted((flash_str_t)0x8640u);   /* cell 15 voltage               */
    uart_print_formatted((flash_str_t)0x862Au);   /* cell 14 voltage               */
    uart_print_formatted((flash_str_t)0x8601u);   /* cell 13 voltage               */
    uart_print_formatted((flash_str_t)0x85D8u);   /* cell 12 voltage               */
    uart_print_formatted((flash_str_t)0x85AFu);   /* cell 11 voltage               */
    uart_print_formatted((flash_str_t)0x8586u);   /* cell 10 voltage               */
    uart_print_formatted((flash_str_t)0x855Du);   /* cell  9 voltage               */
    uart_print_formatted((flash_str_t)0x8540u);   /* cell  8 voltage               */
    uart_print_formatted((flash_str_t)0x8521u);   /* cell  7 voltage               */
    uart_print_formatted((flash_str_t)0x8508u);   /* cell  6 voltage               */

    return 0x1300u;
}
