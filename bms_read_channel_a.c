/*
 * bms_read_channel_a.c
 *
 * Reconstructed from Ghidra decompilation + STM8 disassembly.
 *
 * Original address : 0x008F54
 * Original name    : FUN_008f54
 *
 * ---------------------------------------------------------------
 * What this function does  (confirmed from raw disassembly)
 * ---------------------------------------------------------------
 *
 *  Call site (0x008BC6):
 *      ldw  X, #0x017A     ; param_1 = 0x017A  (primary output address)
 *      callf 0x008F54      ; call with param_2 = 0x017D (on stack)
 *
 *  Disassembly of 0x008F54 step by step:
 *
 *    pushw X               ; save param_1 = 0x017A
 *    pushw X               ; allocate 2-byte result slot on stack
 *    ldw   X, SP           ; X = &result_slot[0]
 *    addw  X, #0x0002      ; X = &result_slot[0] (byte 0 of slot)
 *    pushw X               ; push &slot_byte0  (arg1 to FUN_00971B)
 *    ldw   X, SP           ; X = SP
 *    addw  X, #0x0003      ; X = &result_slot[1] (byte 1 of slot)
 *    callf 0x00971B        ; fetch_sensor_channel_a(&slot_byte1, &slot_byte0)
 *
 *  FUN_00971B disassembly (confirmed):
 *    ldw   Y, #0x01CD      ; src1 = RAM address 0x01CD (channel A byte 0)
 *    ld    A, #0x01        ; 1 iteration = 4 bytes
 *    callf 0x00F959        ; memcpy_4byte_chunks(X, 0x01CD, 1)
 *                          ; → copies 4 bytes from 0x01CD to stack slot byte1
 *    ldw   X, (0x06,SP)    ; X = &slot_byte0 (second arg from stack)
 *    ldw   Y, #0x01D0      ; src2 = RAM address 0x01D0 (channel A byte 1)
 *    ld    A, #0x01        ; 1 iteration = 4 bytes
 *    callf 0x00F959        ; memcpy_4byte_chunks(X, 0x01D0, 1)
 *                          ; → copies 4 bytes from 0x01D0 to stack slot byte0
 *
 *  Back in FUN_008F54 after the call:
 *    popw  X               ; X = &slot_byte0
 *    ld    A, (0x01,SP)    ; A = slot_byte1  (byte read from 0x01CD)
 *    ldw   X, (0x03,SP)    ; X = param_1 = 0x017A
 *    ld    (X), A          ; *param_1 = slot_byte1
 *                          ; → RAM[0x017A] = sensor_byte_from_0x01CD
 *    ld    A, (0x02,SP)    ; A = slot_byte0  (byte read from 0x01D0)
 *    ldw   X, (0x08,SP)    ; X = param_2 = 0x017D
 *    ld    (X), A          ; *param_2 = slot_byte0
 *                          ; → RAM[0x017D] = sensor_byte_from_0x01D0
 *    add   SP, #0x04       ; clean up stack
 *    retf
 *
 * ---------------------------------------------------------------
 * Summary
 * ---------------------------------------------------------------
 *  Reads one measurement byte from sensor channel A byte-0 (0x01CD)
 *  and one byte from sensor channel A byte-1 (0x01D0) and writes
 *  them into two separate RAM output slots:
 *    *param_1 (0x017A) ← byte from 0x01CD
 *    *param_2 (0x017D) ← byte from 0x01D0
 *
 *  In BMS terms: 0x01CD / 0x01D0 are part of the rolling AFE
 *  measurement buffer (cell voltage pair, temperature pair, or
 *  current-sense pair for channel A). The function unpacks two
 *  single-byte readings from that buffer into the output struct.
 *
 * ---------------------------------------------------------------
 * Renamed identifiers
 * ---------------------------------------------------------------
 *  FUN_00971B → fetch_sensor_channel_a()
 *  FUN_00F959 → memcpy_4byte_chunks()   (confirmed counted memcpy)
 *  0x01CD     → SENSOR_CH_A_BYTE0       (AFE measurement buffer byte 0)
 *  0x01D0     → SENSOR_CH_A_BYTE1       (AFE measurement buffer byte 1)
 *  param_1    → out_primary             (primary   output RAM address)
 *  param_2    → out_secondary           (secondary output RAM address)
 */

#include <stdint.h>

/* ------------------------------------------------------------------ */
/* Sensor channel A source addresses in RAM (AFE measurement buffer).  */
/* Confirmed from FUN_00971B disassembly:                              */
/*   ldw Y, #0x01CD  and  ldw Y, #0x01D0                              */
/* ------------------------------------------------------------------ */
#define SENSOR_CH_A_BYTE0   ((volatile uint8_t *)0x01CD)
#define SENSOR_CH_A_BYTE1   ((volatile uint8_t *)0x01D0)

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
/* fetch_sensor_channel_a — reads two sensor bytes from the AFE        */
/* measurement buffer into the caller's stack slots.                   */
/*                                                                      */
/* Confirmed from FUN_00971B disassembly:                              */
/*   src1 = 0x01CD → copies 4 bytes into slot_byte1                   */
/*   src2 = 0x01D0 → copies 4 bytes into slot_byte0                   */
/* ------------------------------------------------------------------ */
static void fetch_sensor_channel_a(uint8_t *slot_byte1,
                                   uint8_t *slot_byte0)
{
    /* Copy from AFE buffer byte-0 address into slot_byte1 */
    memcpy_4byte_chunks(slot_byte1, SENSOR_CH_A_BYTE0, 1u);

    /* Copy from AFE buffer byte-1 address into slot_byte0 */
    memcpy_4byte_chunks(slot_byte0, SENSOR_CH_A_BYTE1, 1u);
}

/* ------------------------------------------------------------------ */
/* read_channel_a_to_ram                                               */
/*                                                                      */
/* Reads two single-byte sensor measurements from channel A of the     */
/* AFE buffer and writes them into two RAM output slots.               */
/*                                                                      */
/*  out_primary   ← byte from 0x01CD  (written to 0x017A at callsite) */
/*  out_secondary ← byte from 0x01D0  (written to 0x017D at callsite) */
/* ------------------------------------------------------------------ */
void read_channel_a_to_ram(uint8_t *out_primary,
                            uint8_t *out_secondary)
{
    uint8_t slot_byte0;   /* receives data from 0x01D0 */
    uint8_t slot_byte1;   /* receives data from 0x01CD */

    /*
     * Fetch both sensor bytes from the AFE measurement buffer.
     * Matches: callf 0x00971B in original.
     */
    fetch_sensor_channel_a(&slot_byte1, &slot_byte0);

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
    *out_primary   = slot_byte1;   /* byte from 0x01CD → RAM[0x017A] */
    *out_secondary = slot_byte0;   /* byte from 0x01D0 → RAM[0x017D] */
}
