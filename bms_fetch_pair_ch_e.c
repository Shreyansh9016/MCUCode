/*
 * bms_fetch_pair_ch_e.c
 *
 * Original address : 0x00977F  /  FUN_00977f
 *
 * Confirmed disassembly:
 *   ldw Y, #0x01D5  ; source1
 *   ld  A, #0x01    ; 1 iteration = 4 bytes
 *   callf 0x00F959  ; memcpy_4byte_chunks(X=param_1, 0x01D5, 1)
 *   ldw X, (0x06,SP); restore param_2
 *   ldw Y, #0x01D6  ; source2
 *   ld  A, #0x01
 *   callf 0x00F959  ; memcpy_4byte_chunks(X=param_2, 0x01D6, 1)
 *   popw X; retf
 *
 * Copies 4 bytes from 0x01D5 into *param_1 and
 *         4 bytes from 0x01D6 into *param_2.
 * Note: SafetyStatusF lo→hi
 *
 * The Ghidra return values (0x3226, 0x4b3f etc.) are
 * return-address artifacts — NOT actual returned data.
 */

#include <stdint.h>

#define SRC_BYTE0  ((volatile uint8_t *)0x01D5u)
#define SRC_BYTE1  ((volatile uint8_t *)0x01D6u)

static void copy4(uint8_t *dst, const volatile uint8_t *src)
{
    dst[0]=src[0]; dst[1]=src[1]; dst[2]=src[2]; dst[3]=src[3];
}

void fetch_pair_channel_e(uint8_t *param_1, uint8_t *param_2)
{
    copy4(param_1, SRC_BYTE0);
    copy4(param_2, SRC_BYTE1);
}
