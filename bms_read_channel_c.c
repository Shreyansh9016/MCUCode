/*
 * bms_read_channel_c.c
 *
 * Reconstructed from Ghidra decompilation + STM8 disassembly.
 * Original address : 0x008F8E
 * Original name    : FUN_008f8e
 *
 * ---------------------------------------------------------------
 * What this function does  (confirmed from raw disassembly)
 * ---------------------------------------------------------------
 *
 *  Identical structure to bms_read_channel_a.c / bms_read_channel_b.c.
 *  Reads one byte from AFE measurement buffer byte-0 (0x01CF)
 *  and one byte from AFE measurement buffer byte-1 (0x01D2),
 *  then writes them into two separate RAM output slots:
 *
 *    *out_primary   (0x017C) ← byte from 0x01CF
 *    *out_secondary (0x017F) ← byte from 0x01D2
 *
 *  Note: Cell channel C — byte-pair at 0x01CF/0x01D2
 *
 *  The AFE measurement buffer addresses stride sequentially by +1
 *  across all channel functions (A through G), confirming these are
 *  adjacent bytes in a packed cell-measurement array maintained by
 *  the BQ76952 AFE driver.
 *
 * ---------------------------------------------------------------
 * Helper: fetch_sensor_channel_c (original 0x00974D)
 * ---------------------------------------------------------------
 *  src1 = 0x01CF → copied into slot_byte1
 *  src2 = 0x01D2 → copied into slot_byte0
 * ---------------------------------------------------------------
 */

#include <stdint.h>

#define SENSOR_CH_C_BYTE0   ((volatile uint8_t *)0x01CF)
#define SENSOR_CH_C_BYTE1   ((volatile uint8_t *)0x01D2)

/* ------------------------------------------------------------------ */
/* memcpy_4byte_chunks — confirmed counted-copy helper (FUN_00F959).   */
/* Copies count*4 bytes from src to dst.                               */
/* ------------------------------------------------------------------ */
static void memcpy_4byte_chunks(uint8_t *dst,
                                const volatile uint8_t *src,
                                uint8_t count)
{
    while (count != 0u)
    {
        dst[0] = src[0]; dst[1] = src[1];
        dst[2] = src[2]; dst[3] = src[3];
        dst += 4; src += 4; count -= 1u;
    }
}

static void fetch_sensor_channel_c(uint8_t *slot_byte1, uint8_t *slot_byte0)
{
    memcpy_4byte_chunks(slot_byte1, SENSOR_CH_C_BYTE0, 1u);
    memcpy_4byte_chunks(slot_byte0, SENSOR_CH_C_BYTE1, 1u);
}

/*
 * read_channel_c_to_ram
 *
 *  out_primary   ← byte from 0x01CF   (→ RAM[0x017C])
 *  out_secondary ← byte from 0x01D2   (→ RAM[0x017F])
 */
void read_channel_c_to_ram(uint8_t *out_primary, uint8_t *out_secondary)
{
    uint8_t slot_byte0;
    uint8_t slot_byte1;

    fetch_sensor_channel_c(&slot_byte1, &slot_byte0);

    *out_primary   = slot_byte1;   /* byte from 0x01CF → RAM[0x017C] */
    *out_secondary = slot_byte0;   /* byte from 0x01D2 → RAM[0x017F] */
}
