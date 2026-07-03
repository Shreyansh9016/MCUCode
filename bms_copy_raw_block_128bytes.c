/*
 * bms_copy_raw_block_128bytes.c
 *
 * Original address : 0x0096EF  /  FUN_0096ef
 *
 * Confirmed disassembly:
 *   ldw Y, #0x018F   ; source = RAM address 0x018F
 *   ld  A, #0x20       ; 32 iterations × 4 bytes = 128 bytes
 *   callf 0x00F959    ; memcpy_4byte_chunks(X, Y, 32)
 *   retf
 *
 * Copies 128 bytes from raw sensor input block at 0x018F (32 entries × 4 bytes)
 * into the caller-supplied destination buffer.
 *
 * param dst : destination buffer (must be >= 128 bytes)
 */

#include <stdint.h>

#define SRC_ADDR    ((volatile uint8_t *)0x018Fu)
#define ITER_COUNT  0x20u
#define BYTE_COUNT  128u

static void memcpy_4byte_chunks(uint8_t *dst, const volatile uint8_t *src, uint8_t n)
{
    while (n-- != 0u) {
        dst[0]=src[0]; dst[1]=src[1]; dst[2]=src[2]; dst[3]=src[3];
        dst+=4; src+=4;
    }
}

void copy_raw_block_128bytes(uint8_t *dst)
{
    memcpy_4byte_chunks(dst, SRC_ADDR, ITER_COUNT);
}
