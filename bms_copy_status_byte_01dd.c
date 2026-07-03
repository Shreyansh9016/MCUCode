/*
 * bms_copy_status_byte_01dd.c
 *
 * Original address : 0x0097D5  /  FUN_0097d5
 *
 * Confirmed disassembly:
 *   ldw Y, #0x01DD  ; source = status/flag byte slot (0x01DD, 1 iter = 4 bytes)
 *   ld  A, #0x01      ; 1 iteration(s) × 4 = 4 bytes
 *   callf 0x00F959   ; memcpy_4byte_chunks(X, Y, 1)
 *   retf
 *
 * Copies 4 bytes from RAM[0x01DD] into the caller-supplied buffer.
 */

#include <stdint.h>

#define SRC_ADDR    ((volatile uint8_t *)0x01DDu)
#define ITER_COUNT  0x01u

static void memcpy_4byte_chunks(uint8_t *dst, const volatile uint8_t *src, uint8_t n)
{
    while (n-- != 0u) {
        dst[0]=src[0]; dst[1]=src[1]; dst[2]=src[2]; dst[3]=src[3];
        dst+=4; src+=4;
    }
}

void copy_status_byte_01dd(uint8_t *dst)
{
    memcpy_4byte_chunks(dst, SRC_ADDR, ITER_COUNT);
}
