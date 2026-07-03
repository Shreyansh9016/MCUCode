/*
 * bms_copy_raw_block_8bytes.c
 *
 * Original address : 0x009710  /  FUN_009710
 *
 * Confirmed disassembly:
 *   ldw Y, #0x01CB   ; source = RAM address 0x01CB
 *   ld  A, #0x02       ; 2 iterations × 4 bytes = 8 bytes
 *   callf 0x00F959    ; memcpy_4byte_chunks(X, Y, 2)
 *   retf
 *
 * Copies 8 bytes from pack-level word pair at 0x01CB (2 entries × 4 bytes)
 * into the caller-supplied destination buffer.
 *
 * param dst : destination buffer (must be >= 8 bytes)
 */

#include <stdint.h>

#define SRC_ADDR    ((volatile uint8_t *)0x01CBu)
#define ITER_COUNT  0x02u
#define BYTE_COUNT  8u

static void memcpy_4byte_chunks(uint8_t *dst, const volatile uint8_t *src, uint8_t n)
{
    while (n-- != 0u) {
        dst[0]=src[0]; dst[1]=src[1]; dst[2]=src[2]; dst[3]=src[3];
        dst+=4; src+=4;
    }
}

void copy_raw_block_8bytes(uint8_t *dst)
{
    memcpy_4byte_chunks(dst, SRC_ADDR, ITER_COUNT);
}
