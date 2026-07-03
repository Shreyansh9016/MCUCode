/*
 * bms_write_sensor_word_01de.c
 *
 * Reconstructed from Ghidra decompilation + STM8 disassembly.
 * Original address : 0x009043
 * Original name    : FUN_009043
 *
 * ---------------------------------------------------------------
 * What this function does  (confirmed from raw disassembly)
 * ---------------------------------------------------------------
 *
 *  Identical in structure to bms_write_sensor_word_01db.c,
 *  but reads from AFE buffer address 0x01DE instead of 0x01DB.
 *
 *  FUN_0097E0 disassembly (confirmed):
 *    ldw Y, #0x01DE   → source = SENSOR_WORD_ADDR (0x01DE)
 *    ld  A, #0x02     → 2 iterations = 8 bytes
 *    callf 0x00F959   → memcpy_4byte_chunks(X, Y, 2)
 *
 *  Disassembly of FUN_009043:
 *    pushw X          ; save param_1 = destination address
 *    pushw X          ; allocate 2-byte result slot
 *    callf 0x0097E0   ; fetch 8 bytes from 0x01DE
 *    ldw X, (0x03,SP) ; restore param_1
 *    ldw Y, (0x01,SP) ; load 16-bit result
 *    ldw (X), Y       ; *param_1 = result
 *    add SP, #0x04
 *    retf
 *
 *  Call site (0x008C24):
 *    ldw X, #0x018B   → param_1 = 0x018B (output RAM slot)
 *    callf 0x009043
 *
 *  Result: RAM[0x018B] receives 16-bit word from AFE buffer 0x01DE/0x01DF.
 */

#include <stdint.h>

#define SENSOR_WORD_01DE_ADDR   ((volatile uint8_t *)0x01DE)
#define SENSOR_WORD_01DE_SIZE   8u   /* 2 iterations × 4 bytes */
static void memcpy_4byte_chunks(uint8_t *dst, const volatile uint8_t *src, uint8_t count) { while(count!=0u){dst[0]=src[0];dst[1]=src[1];dst[2]=src[2];dst[3]=src[3];dst+=4;src+=4;count-=1u;} }
static uint16_t fetch_sensor_word_01de(uint8_t *buf)
{{
    memcpy_4byte_chunks(buf, SENSOR_WORD_01DE_ADDR, 2u);
    return (uint16_t)(((uint16_t)buf[0] << 8) | buf[1]);
}}

/*
 * write_sensor_word_01de_to_ram
 *
 * Reads a 16-bit value from AFE buffer at 0x01DE and writes it
 * to the RAM address supplied by the caller.
 *
 *  Confirmed call site: dest_addr = (uint16_t*)0x018B
 */
void write_sensor_word_01de_to_ram(uint16_t *dest_addr)
{{
    uint8_t  buf[SENSOR_WORD_01DE_SIZE];
    uint16_t result = fetch_sensor_word_01de(buf);
    *dest_addr = result;
}}
