/*
 * bms_write_register.c
 *
 * Reconstructed from Ghidra decompilation + STM8 disassembly
 * cross-reference of BMS firmware.
 *
 * Original address : 0x008F42
 * Original name    : FUN_008f42
 *
 * ---------------------------------------------------------------
 * What this function does  (confirmed from raw disassembly)
 * ---------------------------------------------------------------
 *
 *  Call site (0x008BBB):
 *      ldw  X, #0x0178     ; param_1 = 0x0178  (a RAM address)
 *      callf 0x008F42
 *
 *  Disassembly of 0x008F42:
 *      pushw X             ; save param_1 (destination RAM address)
 *      pushw X             ; allocate a 2-byte result slot on stack
 *      ldw   X, SP         ; X = SP (points at result slot)
 *      addw  X, #0x0001    ; X = &result + 1  (low byte of slot)
 *      callf 0x009710      ; --- fetch_sensor_block(&result_lo) ---
 *                          ;   009710: ldw Y, #0x01C3   (src address)
 *                          ;           ld  A, #0x08      (8 iterations)
 *                          ;           callf 0x00F959    (counted memcpy)
 *                          ;   F959: copies A*4 = 32 bytes from Y→X
 *                          ;   net effect: fills result slot from sensor
 *                          ;   data block at 0x01C3 (32 bytes)
 *      ldw  X, (0x03,SP)   ; reload param_1 from stack
 *      ldw  Y, (0x01,SP)   ; load result word from stack slot
 *      ldw  (X), Y         ; *param_1 = result   (16-bit write to RAM)
 *      add  SP, #0x04      ; clean up stack
 *      retf
 *
 *  Summary:
 *    Copies 32 bytes from the sensor data block at RAM address 0x01C3
 *    into a local stack buffer via fetch_sensor_block(), then writes
 *    the first 16-bit word of that result back to the caller-supplied
 *    RAM address (param_1 = 0x0178 at the confirmed call site).
 *
 *    In a BMS context, 0x01C3 is likely a rolling measurement buffer
 *    maintained by the AFE driver (BQ76952 cell data, temperatures,
 *    or pack-level readings), and 0x0178 is one slot in the processed
 *    output structure that the state machine reads.
 *
 * ---------------------------------------------------------------
 * Renamed identifiers
 * ---------------------------------------------------------------
 *  FUN_009710  → fetch_sensor_block()
 *  FUN_00F959  → memcpy_4byte_chunks()   (confirmed counted memcpy)
 *  0x01C3      → SENSOR_DATA_BLOCK_ADDR  (source buffer)
 *  param_1     → dest_addr               (caller-supplied write target)
 */

#include <stdint.h>

/* ------------------------------------------------------------------ */
/* Constant: source address of the 32-byte sensor data block in RAM.   */
/* Confirmed from 009710: ldw Y, #0x01C3                               */
/* ------------------------------------------------------------------ */
#define SENSOR_DATA_BLOCK_ADDR   ((volatile uint8_t *)0x01C3)

/* ------------------------------------------------------------------ */
/* Block size: 009710 sets A=8, F959 copies A*4 bytes → 32 bytes.      */
/* ------------------------------------------------------------------ */
#define SENSOR_BLOCK_SIZE        32u    /* bytes */

/* ------------------------------------------------------------------ */
/* Helper: counted 4-byte-chunk memcpy (FUN_00F959).                   */
/*                                                                      */
/* Confirmed disassembly:                                               */
/*   for (uint8_t i = count; i != 0; i -= 4)                           */
/*       copy 4 bytes from src to dst; src += 4; dst += 4;             */
/*                                                                      */
/* In practice always called with count=8 (copies 32 bytes).           */
/* ------------------------------------------------------------------ */
void memcpy_4byte_chunks(uint8_t *dst,
                         const volatile uint8_t *src,
                         uint8_t  count)
{
    while (count != 0u)
    {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = src[3];
        dst   += 4;
        src   += 4;
        count -= 4u;
    }
}

/* ------------------------------------------------------------------ */
/* Helper: fetch one sensor data block from the AFE buffer (FUN_009710)*/
/*                                                                      */
/* Copies SENSOR_BLOCK_SIZE bytes from the fixed sensor-data buffer at */
/* 0x01C3 into the caller's local buffer, then returns the first 16-bit*/
/* word as the result.                                                  */
/*                                                                      */
/* Confirmed disassembly of 009710:                                     */
/*   ldw  Y, #0x01C3   → source = SENSOR_DATA_BLOCK_ADDR               */
/*   ld   A, #0x08     → 8 iterations  (8 × 4 = 32 bytes)             */
/*   callf 0x00F959    → memcpy_4byte_chunks(X, Y, A)                  */
/* ------------------------------------------------------------------ */
uint16_t fetch_sensor_block(uint8_t *local_buf)
{
    memcpy_4byte_chunks(local_buf,
                        SENSOR_DATA_BLOCK_ADDR,
                        SENSOR_BLOCK_SIZE);

    /* Return the first 16-bit word from the filled buffer.            */
    /* Matches: ldw Y,(0x01,SP) after callf 009710 in FUN_008F42.     */
    return (uint16_t)(((uint16_t)local_buf[0] << 8) | local_buf[1]);
}

/* ------------------------------------------------------------------ */
/* Main function: write_sensor_word_to_ram                             */
/*                                                                      */
/* Fetches one 16-bit measurement word from the AFE sensor data block  */
/* and writes it to the RAM address supplied by the caller.            */
/*                                                                      */
/* param dest_addr : pointer to the RAM word that receives the result. */
/*   Confirmed call site: dest_addr = (uint16_t*)0x0178               */
/* ------------------------------------------------------------------ */
void write_sensor_word_to_ram(uint16_t *dest_addr)
{
    /*
     * Local buffer for the 32-byte sensor block.
     * On the real STM8 this lives on the stack (the two pushw X
     * instructions in the disassembly allocate 4 bytes total; the
     * full 32-byte copy lands in the buffer passed to fetch_sensor_block).
     */
    uint8_t  sensor_buf[SENSOR_BLOCK_SIZE];

    /*
     * Copy the sensor data block from 0x01C3 into sensor_buf,
     * and get the first 16-bit word back as the result.
     *
     * Matches: callf 0x009710 in the original, which does
     *   memcpy_4byte_chunks(SP+1, 0x01C3, 8)
     */
    uint16_t result = fetch_sensor_block(sensor_buf);

    /*
     * Write the result word to the caller-supplied RAM address.
     *
     * Matches:
     *   ldw  X, (0x03,SP)   ; restore dest_addr from stack
     *   ldw  Y, (0x01,SP)   ; load result from stack slot
     *   ldw  (X), Y         ; *dest_addr = result
     */
    *dest_addr = result;
}
