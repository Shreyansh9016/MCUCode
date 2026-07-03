/*
 * bms_write_sensor_word_018d.c
 *
 * Reconstructed from Ghidra decompilation + STM8 disassembly.
 * Original address : 0x00905A
 * Original name    : FUN_00905a
 *
 * ---------------------------------------------------------------
 * What this function does  (confirmed from raw disassembly)
 * ---------------------------------------------------------------
 *
 *  Identical in structure to bms_write_sensor_word_01de.c
 *  (FUN_009043). Uses the SAME helper FUN_0097E0, which copies
 *  8 bytes (2 iterations × 4 bytes) from AFE buffer at 0x01DE.
 *
 *  Confirmed disassembly of FUN_00905A:
 *    pushw X             ; save param_1 (destination address)
 *    pushw X             ; allocate 2-byte result slot on stack
 *    ldw   X, SP
 *    addw  X, #0x0001    ; X = &result_slot[0]
 *    callf 0x0097E0      ; fetch_sensor_word_01de(&slot)
 *    ldw   X, (0x03,SP)  ; restore param_1 = 0x018D
 *    ldw   Y, (0x01,SP)  ; load 16-bit result
 *    ldw   (X), Y        ; *param_1 = result
 *    add   SP, #0x04
 *    retf
 *
 *  FUN_0097E0 disassembly (confirmed):
 *    ldw Y, #0x01DE      ; source = AFE buffer at 0x01DE
 *    ld  A, #0x02        ; 2 iterations = 8 bytes
 *    callf 0x00F959      ; memcpy_4byte_chunks(X, 0x01DE, 2)
 *
 *  Call site (0x008C2B):
 *    ldw X, #0x018D      ; param_1 = 0x018D (output RAM slot)
 *    callf 0x00905A
 *
 *  This is the LAST in the sequential chain of measurement writers:
 *    FUN_009043 (0x018B) ← 0x01DE (first call with this helper)
 *    FUN_00905A (0x018D) ← 0x01DE (second call — same source, next slot)
 *
 *  Result: RAM[0x018D:0x018E] receives 16-bit word from 0x01DE/0x01DF.
 *  In BMS context: likely a second reading of the same pack-level
 *  measurement (e.g. pack current or total voltage) for redundancy
 *  or for a rolling average calculation.
 */

#include <stdint.h>

/* AFE buffer source address — shared with FUN_009043 */
#define SENSOR_WORD_01DE_ADDR   ((volatile uint8_t *)0x01DE)
#define SENSOR_WORD_01DE_SIZE   8u    /* 2 iterations × 4 bytes */

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

static uint16_t fetch_sensor_word_01de(uint8_t *buf)
{
    memcpy_4byte_chunks(buf, SENSOR_WORD_01DE_ADDR, 2u);
    return (uint16_t)(((uint16_t)buf[0] << 8) | buf[1]);
}

/*
 * write_sensor_word_018d
 *
 * Reads a 16-bit value from AFE buffer at 0x01DE and writes it
 * to the RAM address supplied by the caller.
 *
 *  Confirmed call site: dest_addr = (uint16_t*)0x018D
 */
void write_sensor_word_018d(uint16_t *dest_addr)
{
    uint8_t  buf[SENSOR_WORD_01DE_SIZE];
    uint16_t result = fetch_sensor_word_01de(buf);
    *dest_addr = result;
}
