/*
 * bms_write_sensor_word_01db.c
 *
 * Reconstructed from Ghidra decompilation + STM8 disassembly.
 * Original address : 0x00901F
 * Original name    : FUN_00901f
 *
 * ---------------------------------------------------------------
 * What this function does  (confirmed from raw disassembly)
 * ---------------------------------------------------------------
 *
 *  Identical in structure to bms_write_register.c (FUN_008F42).
 *  Calls FUN_0097CA which copies 8 bytes (A=2, so 2×4=8 bytes)
 *  from AFE buffer address 0x01DB into a local stack slot,
 *  then writes the resulting 16-bit word to *param_1.
 *
 *  FUN_0097CA disassembly (confirmed):
 *    ldw Y, #0x01DB   → source = SENSOR_WORD_ADDR (0x01DB)
 *    ld  A, #0x02     → 2 iterations = 8 bytes total
 *    callf 0x00F959   → memcpy_4byte_chunks(X, Y, 2)
 *
 *  Call site (0x008C16):
 *    ldw X, #0x0188   → param_1 = 0x0188 (output RAM slot)
 *    callf 0x00901F
 *
 *  Result: RAM[0x0188] receives 16-bit word from AFE buffer 0x01DB.
 *
 *  In BMS context: 0x01DB/0x01DC holds the next sequential
 *  measurement in the AFE buffer (continuing the byte-by-byte
 *  stride from channels A–G above).
 */

#include <stdint.h>

#define SENSOR_WORD_01DB_ADDR   ((volatile uint8_t *)0x01DB)
#define SENSOR_WORD_01DB_SIZE   8u   /* 2 iterations × 4 bytes */
static void memcpy_4byte_chunks(uint8_t *dst, const volatile uint8_t *src, uint8_t count) { while(count!=0u){dst[0]=src[0];dst[1]=src[1];dst[2]=src[2];dst[3]=src[3];dst+=4;src+=4;count-=1u;} }
static uint16_t fetch_sensor_word_01db(uint8_t *buf)
{{
    /* copies 8 bytes from 0x01DB into buf */
    memcpy_4byte_chunks(buf, SENSOR_WORD_01DB_ADDR, 2u);
    return (uint16_t)(((uint16_t)buf[0] << 8) | buf[1]);
}}

/*
 * write_sensor_word_01db_to_ram
 *
 * Reads a 16-bit value from AFE buffer at 0x01DB and writes it
 * to the RAM address supplied by the caller.
 *
 *  Confirmed call site: dest_addr = (uint16_t*)0x0188
 */
void write_sensor_word_01db_to_ram(uint16_t *dest_addr)
{{
    uint8_t  buf[SENSOR_WORD_01DB_SIZE];
    uint16_t result = fetch_sensor_word_01db(buf);
    *dest_addr = result;
}}
