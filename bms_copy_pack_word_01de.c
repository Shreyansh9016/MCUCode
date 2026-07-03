/*
 * bms_copy_pack_word_01de.c
 *
 * Original address : 0x0097E0  /  FUN_0097e0
 *
 * Confirmed disassembly:
 *   ldw Y, #0x01DE  ; source = stack/pack voltage word (0x01DE, 2 iter = 8 bytes)
 *   ld  A, #0x02      ; 2 iteration(s) × 4 = 8 bytes
 *   callf 0x00F959   ; memcpy_4byte_chunks(X, Y, 2)
 *   retf
 *
 * Copies 8 bytes from RAM[0x01DE] into the caller-supplied buffer.
 */

#include <stdint.h>

#define SRC_ADDR    ((volatile uint8_t *)0x01DEu)
#define ITER_COUNT  0x02u

static void memcpy_4byte_chunks(uint8_t *dst, const volatile uint8_t *src, uint8_t n)
{
    while (n-- != 0u) {
        dst[0]=src[0]; dst[1]=src[1]; dst[2]=src[2]; dst[3]=src[3];
        dst+=4; src+=4;
    }
}

void copy_pack_word_01de(uint8_t *dst)
{
    memcpy_4byte_chunks(dst, SRC_ADDR, ITER_COUNT);
}
