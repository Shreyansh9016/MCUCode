/*
 * bms_copy_measurement_block_40bytes.c
 *
 * Reconstructed from Ghidra decompilation + STM8 disassembly.
 * Original address : 0x009077
 * Original name    : FUN_009077
 *
 * ---------------------------------------------------------------
 * What this function does  (confirmed from raw disassembly)
 * ---------------------------------------------------------------
 *
 *  Confirmed disassembly of FUN_009077:
 *    ldw Y, #0x0140      ; source  = AFE data block at 0x0140
 *    ld  A, #0x28        ; count   = 0x28 = 40 iterations
 *    callf 0x00F959      ; memcpy_4byte_chunks(X, 0x0140, 40)
 *    retf
 *
 *  Ghidra shows: FUN_00f959(0x28, param_1)
 *    → copies 40 × 4 = 160 bytes from 0x0140 into param_1
 *
 *  param_1 is the destination buffer pointer (caller-supplied).
 *
 *  Source region:  0x0140 – 0x01DF  (160 bytes)
 *  This is the SECOND data block in RAM:
 *    0x0100 – 0x013F  (64 bytes)  ← copied by FUN_00906C (not shown)
 *    0x0140 – 0x01DF  (160 bytes) ← copied by THIS function
 *    0x0168 – 0x0177  (16 bytes)  ← copied by FUN_009082 (next)
 *
 *  In BMS context: 0x0140–0x01DF is the "processed measurements"
 *  region — the output struct populated by all the channel A–G
 *  readers and word-writers from previous functions. This function
 *  bulk-copies the entire processed measurement block into
 *  whatever output buffer the caller supplies (e.g. for CAN
 *  transmission, UART reporting, or flash logging).
 *
 *  NOTE: 0x28 iterations × 4 bytes = 160 bytes (0xA0) which
 *  exactly spans 0x0140 to 0x01DF — the complete cell measurement
 *  output struct.
 */

#include <stdint.h>

/* Source: start of the processed measurement block in RAM */
#define MEAS_BLOCK_SRC_ADDR     ((volatile uint8_t *)0x0140)

/* 40 iterations × 4 bytes per iteration = 160 bytes */
#define MEAS_BLOCK_ITER_COUNT   0x28u
#define MEAS_BLOCK_BYTE_COUNT   (MEAS_BLOCK_ITER_COUNT * 4u)   /* 160 */

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
 * copy_measurement_block_160bytes
 *
 * Bulk-copies the 160-byte processed measurement block from
 * RAM region 0x0140–0x01DF into the caller-supplied destination.
 *
 *  param dst : destination buffer (must be ≥ 160 bytes)
 */
void copy_measurement_block_160bytes(uint8_t *dst)
{
    memcpy_4byte_chunks(dst, MEAS_BLOCK_SRC_ADDR, MEAS_BLOCK_ITER_COUNT);
}
