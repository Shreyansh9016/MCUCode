/*
 * bms_copy_raw_block_80bytes.c
 *
 * Original address : 0x0096FA  /  FUN_0096fa
 *
 * Confirmed disassembly:
 *   ldw Y, #0x01AF   ; source = RAM address 0x01AF
 *   ld  A, #0x14       ; 20 iterations × 4 bytes = 80 bytes
 *   callf 0x00F959    ; memcpy_4byte_chunks(X, Y, 20)
 *   retf
 *
 * Copies 80 bytes from intermediate calculation block at 0x01AF (20 entries × 4 bytes)
 * into the caller-supplied destination buffer.
 *
 * param dst : destination buffer (must be >= 80 bytes)
 */

#include <stdint.h>

#define SRC_ADDR    ((volatile uint8_t *)0x01AFu)
#define ITER_COUNT  0x14u
#define BYTE_COUNT  80u

static void memcpy_4byte_chunks(uint8_t *dst, const volatile uint8_t *src, uint8_t n)
{
    while (n-- != 0u) {
        dst[0]=src[0]; dst[1]=src[1]; dst[2]=src[2]; dst[3]=src[3];
        dst+=4; src+=4;
    }
}

void copy_raw_block_80bytes(uint8_t *dst)
{
    memcpy_4byte_chunks(dst, SRC_ADDR, ITER_COUNT);
}
