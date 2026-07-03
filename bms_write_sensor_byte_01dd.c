/*
 * bms_write_sensor_byte_01dd.c
 *
 * Reconstructed from Ghidra decompilation + STM8 disassembly.
 * Original address : 0x009031
 * Original name    : FUN_009031
 *
 * ---------------------------------------------------------------
 * What this function does  (confirmed from raw disassembly)
 * ---------------------------------------------------------------
 *
 *  Different from the 16-bit word writers: writes a single byte
 *  to *param_2 (not param_1).
 *
 *  FUN_0097D5 disassembly (confirmed):
 *    ldw Y, #0x01DD   → source = SENSOR_BYTE_ADDR (0x01DD)
 *    ld  A, #0x01     → 1 iteration = 4 bytes
 *    callf 0x00F959   → memcpy_4byte_chunks(X, Y, 1)
 *
 *  Disassembly of FUN_009031:
 *    push X           ; save param_1 (unused as destination here)
 *    push A           ; save param_2 (1-byte destination address)
 *    callf 0x0097D5   ; fetch 4 bytes from 0x01DD into stack slot
 *    ld  A, (0x01,SP) ; A = first byte from 0x01DD
 *    ldw X, (0x02,SP) ; X = param_2 (destination address)
 *    ld  (X), A       ; *param_2 = byte_from_0x01DD
 *    add SP, #0x03
 *    retf
 *
 *  Call site (0x008C1D):
 *    ldw X, #0x018A   → param_1 / source context
 *    callf 0x009031   → param_2 already on stack from prior push
 *
 *  Result: one byte from AFE buffer 0x01DD is written to *param_2.
 *
 *  In BMS context: 0x01DD is the next single-byte measurement slot
 *  in the sequential AFE buffer, likely a status flag or 8-bit
 *  temperature/fault reading.
 */

#include <stdint.h>

#define SENSOR_BYTE_01DD_ADDR   ((volatile uint8_t *)0x01DD)
static void memcpy_4byte_chunks(uint8_t *dst, const volatile uint8_t *src, uint8_t count) { while(count!=0u){dst[0]=src[0];dst[1]=src[1];dst[2]=src[2];dst[3]=src[3];dst+=4;src+=4;count-=1u;} }
static uint8_t fetch_sensor_byte_01dd(uint8_t *buf)
{{
    /* copies 4 bytes from 0x01DD; we use only the first byte */
    memcpy_4byte_chunks(buf, SENSOR_BYTE_01DD_ADDR, 1u);
    return buf[0];
}}

/*
 * write_sensor_byte_01dd_to_ram
 *
 * Reads a single byte from AFE buffer at 0x01DD and writes it
 * to the RAM address supplied in dest_byte.
 *
 *  Confirmed call site: writes to the address passed as param_2
 */
void write_sensor_byte_01dd_to_ram(uint8_t *dest_byte)
{{
    uint8_t buf[4];   /* 1 iteration × 4 bytes */
    *dest_byte = fetch_sensor_byte_01dd(buf);
}}
