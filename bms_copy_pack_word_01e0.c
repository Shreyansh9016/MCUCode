/*
 * bms_copy_pack_word_01e0.c
 *
 * Original address : 0x0097EB  /  FUN_0097eb
 *
 * Confirmed disassembly:
 *   ldw Y, #0x01E0  ; source = pack voltage second word (0x01E0, 2 iter = 8 bytes)
 *   ld  A, #0x02      ; 2 iteration(s) × 4 = 8 bytes
 *   callf 0x00F959   ; memcpy_4byte_chunks(X, Y, 2)
 *   retf
 *
 * Copies 8 bytes from RAM[0x01E0] into the caller-supplied buffer.
 */

#include <stdint.h>

#define SRC_ADDR    ((volatile uint8_t *)0x01E0u)
#define ITER_COUNT  0x02u

static void memcpy_4byte_chunks(uint8_t *dst, const volatile uint8_t *src, uint8_t n)
{
    while (n-- != 0u) {
        dst[0]=src[0]; dst[1]=src[1]; dst[2]=src[2]; dst[3]=src[3];
        dst+=4; src+=4;
    }
}

void copy_pack_word_01e0(uint8_t *dst)
{
    memcpy_4byte_chunks(dst, SRC_ADDR, ITER_COUNT);
}
