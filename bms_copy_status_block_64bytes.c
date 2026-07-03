/*
 * bms_copy_status_block_64bytes.c
 *
 * Reconstructed from Ghidra decompilation + STM8 disassembly.
 * Original address : 0x009082
 * Original name    : FUN_009082
 *
 * ---------------------------------------------------------------
 * What this function does  (confirmed from raw disassembly)
 * ---------------------------------------------------------------
 *
 *  Confirmed disassembly of FUN_009082:
 *    ldw Y, #0x0168      ; source  = data block at 0x0168
 *    ld  A, #0x10        ; count   = 0x10 = 16 iterations
 *    callf 0x00F959      ; memcpy_4byte_chunks(X, 0x0168, 16)
 *    retf
 *
 *  Ghidra shows: FUN_00f959(0x10, param_1)
 *    → copies 16 × 4 = 64 bytes from 0x0168 into param_1
 *
 *  param_1 is the destination buffer pointer (caller-supplied).
 *
 *  Source region:  0x0168 – 0x01A7  (64 bytes)
 *
 *  Comparison with FUN_009077:
 *    FUN_009077 copies 0x0140–0x01DF (160 bytes — full meas. block)
 *    FUN_009082 copies 0x0168–0x01A7 (64 bytes  — sub-region)
 *
 *  0x0168 is the START of the channel-A through channel-G output
 *  sub-struct within the larger measurement block:
 *    0x0140–0x0167: raw/intermediate data (first 40 bytes)
 *    0x0168–0x01A7: cell channel A–G processed outputs (64 bytes)
 *
 *  In BMS context: this function copies the 7-cell measurement
 *  sub-block (cell voltages or temperatures for cells A–G) into
 *  a caller-supplied buffer, likely for a compact status report
 *  or CAN frame that only needs the cell-level data.
 *
 *  NOTE: 0x10 iterations × 4 bytes = 64 bytes (0x40) which covers
 *  0x0168 to 0x01A7 — the cell-level output sub-region.
 */

#include <stdint.h>

/* Source: start of the cell-level measurement sub-block */
#define STATUS_BLOCK_SRC_ADDR   ((volatile uint8_t *)0x0168)

/* 16 iterations × 4 bytes = 64 bytes */
#define STATUS_BLOCK_ITER_COUNT 0x10u
#define STATUS_BLOCK_BYTE_COUNT (STATUS_BLOCK_ITER_COUNT * 4u)   /* 64 */

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

/*
 * copy_status_block_64bytes
 *
 * Bulk-copies the 64-byte cell measurement sub-block from
 * RAM region 0x0168–0x01A7 into the caller-supplied destination.
 *
 *  param dst : destination buffer (must be ≥ 64 bytes)
 */
void copy_status_block_64bytes(uint8_t *dst)
{
    memcpy_4byte_chunks(dst, STATUS_BLOCK_SRC_ADDR, STATUS_BLOCK_ITER_COUNT);
}
