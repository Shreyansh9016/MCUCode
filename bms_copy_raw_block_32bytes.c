/*
 * bms_copy_raw_block_32bytes.c
 *
 * Original address : 0x009705  /  FUN_009705
 *
 * Confirmed disassembly:
 *   ldw Y, #0x01C3   ; source = RAM address 0x01C3
 *   ld  A, #0x08       ; 8 iterations × 4 bytes = 32 bytes
 *   callf 0x00F959    ; memcpy_4byte_chunks(X, Y, 8)
 *   retf
 *
 * Copies 32 bytes from AFE conversion result buffer at 0x01C3 (8 entries × 4 bytes)
 * into the caller-supplied destination buffer.
 *
 * param dst : destination buffer (must be >= 32 bytes)
 */

#include <stdint.h>

#define SRC_ADDR    ((volatile uint8_t *)0x01C3u)
#define ITER_COUNT  0x08u
#define BYTE_COUNT  32u

static void memcpy_4byte_chunks(uint8_t *dst, const volatile uint8_t *src, uint8_t n)
{
    while (n-- != 0u) {
        dst[0]=src[0]; dst[1]=src[1]; dst[2]=src[2]; dst[3]=src[3];
        dst+=4; src+=4;
    }
}

void copy_raw_block_32bytes(uint8_t *dst)
{
    memcpy_4byte_chunks(dst, SRC_ADDR, ITER_COUNT);
}
