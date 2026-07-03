/*
 * bms_read_channel_b.c
 *
 * Reconstructed from Ghidra decompilation + STM8 disassembly.
 *
 * Original address : 0x008F71
 * Original name    : FUN_008f71
 *
 * ---------------------------------------------------------------
 * What this function does  (confirmed from raw disassembly)
 * ---------------------------------------------------------------
 *
 *  Call site (0x008BD2):
 *      ldw  X, #0x017B     ; param_1 = 0x017B  (primary output address)
 *      callf 0x008F71      ; call with param_2 = 0x017E (on stack)
 *
 *  Disassembly of 0x008F71 step by step:
 *
 *    pushw X               ; save param_1 = 0x017B
 *    pushw X               ; allocate 2-byte result slot on stack
 *    ldw   X, SP           ; X = &result_slot[0]
 *    addw  X, #0x0002      ; X = &result_slot[0] (byte 0 of slot)
 *    pushw X               ; push &slot_byte0  (arg1 to FUN_009734)
 *    ldw   X, SP           ; X = SP
 *    addw  X, #0x0003      ; X = &result_slot[1] (byte 1 of slot)
 *    callf 0x009734        ; fetch_sensor_channel_b(&slot_byte1, &slot_byte0)
 *
 *  FUN_009734 disassembly (confirmed):
 *    ldw   Y, #0x01CE      ; src1 = RAM address 0x01CE (channel B byte 0)
 *    ld    A, #0x01        ; 1 iteration = 4 bytes
 *    callf 0x00F959        ; memcpy_4byte_chunks(X, 0x01CE, 1)
 *                          ; → copies 4 bytes from 0x01CE to stack slot byte1
 *    ldw   X, (0x06,SP)    ; X = &slot_byte0 (second arg from stack)
 *    ldw   Y, #0x01D1      ; src2 = RAM address 0x01D1 (channel B byte 1)
 *    ld    A, #0x01        ; 1 iteration = 4 bytes
 *    callf 0x00F959        ; memcpy_4byte_chunks(X, 0x01D1, 1)
 *                          ; → copies 4 bytes from 0x01D1 to stack slot byte0
 *
 *  Back in FUN_008F71 after the call:
 *    popw  X               ; X = &slot_byte0
 *    ld    A, (0x01,SP)    ; A = slot_byte1  (byte read from 0x01CE)
 *    ldw   X, (0x03,SP)    ; X = param_1 = 0x017B
 *    ld    (X), A          ; *param_1 = slot_byte1
 *                          ; → RAM[0x017B] = sensor_byte_from_0x01CE
 *    ld    A, (0x02,SP)    ; A = slot_byte0  (byte read from 0x01D1)
 *    ldw   X, (0x08,SP)    ; X = param_2 = 0x017E
 *    ld    (X), A          ; *param_2 = slot_byte0
 *                          ; → RAM[0x017E] = sensor_byte_from_0x01D1
 *    add   SP, #0x04       ; clean up stack
 *    retf
 *
 * ---------------------------------------------------------------
 * Difference from FUN_008F54 (bms_read_channel_a.c)
 * ---------------------------------------------------------------
 *  FUN_008F54 reads from 0x01CD / 0x01D0 → writes to 0x017A / 0x017D
 *  FUN_008F71 reads from 0x01CE / 0x01D1 → writes to 0x017B / 0x017E
 *
 *  The source addresses are each offset by +1, and the destination
 *  addresses are each offset by +1. This strongly suggests the two
 *  functions read adjacent bytes of the same 16-bit measurement:
 *    Channel A (008F54): high byte at 0x01CD, low byte at 0x01D0
 *    Channel B (008F71): high byte at 0x01CE, low byte at 0x01D1
 *
 *  Together they unpack a pair of 16-bit sensor values (likely a
 *  cell voltage pair or a temperature pair from the BQ76952 AFE)
 *  byte by byte into the output struct at 0x017A–0x017E.
 *
 * ---------------------------------------------------------------
 * Summary
 * ---------------------------------------------------------------
 *  Reads one measurement byte from sensor channel B byte-0 (0x01CE)
 *  and one byte from sensor channel B byte-1 (0x01D1) and writes
 *  them into two separate RAM output slots:
 *    *param_1 (0x017B) ← byte from 0x01CE
 *    *param_2 (0x017E) ← byte from 0x01D1
 *
 * ---------------------------------------------------------------
 * Renamed identifiers
 * ---------------------------------------------------------------
 *  FUN_009734 → fetch_sensor_channel_b()
 *  FUN_00F959 → memcpy_4byte_chunks()   (confirmed counted memcpy)
 *  0x01CE     → SENSOR_CH_B_BYTE0       (AFE measurement buffer byte 0)
 *  0x01D1     → SENSOR_CH_B_BYTE1       (AFE measurement buffer byte 1)
 *  param_1    → out_primary             (primary   output RAM address)
 *  param_2    → out_secondary           (secondary output RAM address)
 */

#include <stdint.h>

/* ------------------------------------------------------------------ */
/* Sensor channel B source addresses in RAM (AFE measurement buffer).  */
/* Confirmed from FUN_009734 disassembly:                              */
/*   ldw Y, #0x01CE  and  ldw Y, #0x01D1                              */
/* Note: one byte offset from channel A (0x01CD / 0x01D0).            */
/* ------------------------------------------------------------------ */
#define SENSOR_CH_B_BYTE0   ((volatile uint8_t *)0x01CE)
#define SENSOR_CH_B_BYTE1   ((volatile uint8_t *)0x01D1)

/* ------------------------------------------------------------------ */
/* memcpy_4byte_chunks — confirmed counted-copy helper (FUN_00F959).   */
/* Always called here with count=1, copying exactly 4 bytes.           */
/* ------------------------------------------------------------------ */
static void memcpy_4byte_chunks(uint8_t *dst,
                                const volatile uint8_t *src,
                                uint8_t count)
{
    while (count != 0u)
    {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = src[3];
        dst   += 4;
        src   += 4;
        count -= 1u;
    }
}

/* ------------------------------------------------------------------ */
/* fetch_sensor_channel_b — reads two sensor bytes from the AFE        */
/* measurement buffer into the caller's stack slots.                   */
/*                                                                      */
/* Confirmed from FUN_009734 disassembly:                              */
/*   src1 = 0x01CE → copies 4 bytes into slot_byte1                   */
/*   src2 = 0x01D1 → copies 4 bytes into slot_byte0                   */
/* ------------------------------------------------------------------ */
static void fetch_sensor_channel_b(uint8_t *slot_byte1,
                                   uint8_t *slot_byte0)
{
    /* Copy from AFE buffer byte-0 address into slot_byte1 */
    memcpy_4byte_chunks(slot_byte1, SENSOR_CH_B_BYTE0, 1u);

    /* Copy from AFE buffer byte-1 address into slot_byte0 */
    memcpy_4byte_chunks(slot_byte0, SENSOR_CH_B_BYTE1, 1u);
}

/* ------------------------------------------------------------------ */
/* read_channel_b_to_ram                                               */
/*                                                                      */
/* Reads two single-byte sensor measurements from channel B of the     */
/* AFE buffer and writes them into two RAM output slots.               */
/*                                                                      */
/*  out_primary   ← byte from 0x01CE  (written to 0x017B at callsite) */
/*  out_secondary ← byte from 0x01D1  (written to 0x017E at callsite) */
/* ------------------------------------------------------------------ */
void read_channel_b_to_ram(uint8_t *out_primary,
                            uint8_t *out_secondary)
{
    uint8_t slot_byte0;   /* receives data from 0x01D1 */
    uint8_t slot_byte1;   /* receives data from 0x01CE */

    /*
     * Fetch both sensor bytes from the AFE measurement buffer.
     * Matches: callf 0x009734 in original.
     */
    fetch_sensor_channel_b(&slot_byte1, &slot_byte0);

    /*
     * Write results to the two caller-supplied output addresses.
     *
     * Matches disassembly:
     *   ld A, (0x01,SP) ; slot_byte1
     *   ldw X, (0x03,SP); out_primary
     *   ld (X), A       ; *out_primary = slot_byte1
     *
     *   ld A, (0x02,SP) ; slot_byte0
     *   ldw X, (0x08,SP); out_secondary
     *   ld (X), A       ; *out_secondary = slot_byte0
     */
    *out_primary   = slot_byte1;   /* byte from 0x01CE → RAM[0x017B] */
    *out_secondary = slot_byte0;   /* byte from 0x01D1 → RAM[0x017E] */
}
