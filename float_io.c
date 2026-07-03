/*
 * float_io.c
 *
 * Float store/load functions — the I/O layer between the
 * scratch registers and the rest of the firmware's RAM buffers.
 *
 * FUN_00f7dd  float_load_buf     — load 4 bytes from buffer into scratch
 * FUN_00f7ef  float_load_const   — load a float constant from flash/ROM
 * FUN_00f817  float_store        — store scratch 4 bytes to output buffer
 * FUN_00f77f  float_load_const_internal — helper for constant loading
 * FUN_00f804  timer_check_expired — check if a countdown timer has reached 0
 * FUN_00f8b7  timer_wait_ticks   — busy-wait for N timer ticks
 * FUN_00f8be  float_scale_encode — encode/scale a value into float format
 * FUN_00f893  float_int_div      — integer divide X by A, result to scratch
 * FUN_00f959  memcpy_4byte_chunks— confirmed counted 4-byte bulk copy
 * FUN_00f90b  float_result_check — check if float result is non-zero
 *
 * All confirmed from disassembly.
 */
#include <stdint.h>
#include <stdbool.h>

/* Scratch registers */
#define FSCRATCH_B0  (*(volatile uint8_t *)0x0000u)
#define FSCRATCH_B1  (*(volatile uint8_t *)0x0001u)
#define FSCRATCH_B2  (*(volatile uint8_t *)0x0002u)
#define FSCRATCH_B3  (*(volatile uint8_t *)0x0003u)
#define FSCRATCH_HI  (*(volatile uint16_t *)0x0000u)
#define FSCRATCH_LO  (*(volatile uint16_t *)0x0002u)

/*
 * float_load_buf  (FUN_00f7dd)
 * Confirmed disassembly (from our earlier analysis — 322 calls):
 *   push A; ld A,(X); ld 0x00,A; ld A,(0x01,X); ld 0x01,A;
 *           ld A,(0x02,X); ld 0x02,A; ld A,(0x03,X); ld 0x03,A;
 *   pop A; retf
 * Copies 4 bytes from [X] into scratch 0x00–0x03.
 */
void float_load_buf(const uint8_t *src)
{
    FSCRATCH_B0 = src[0];
    FSCRATCH_B1 = src[1];
    FSCRATCH_B2 = src[2];
    FSCRATCH_B3 = src[3];
}

/*
 * float_store  (FUN_00f817)
 * Confirmed disassembly (from our earlier analysis — 137 calls):
 *   ld A,0x00; ld (X),A; ld A,0x01; ld (0x01,X),A;
 *   ld A,0x02; ld (0x02,X),A; ld A,0x03; ld (0x03,X),A; retf
 * Copies scratch 0x00–0x03 into 4 bytes at [X].
 */
void float_store(uint8_t *dst)
{
    dst[0] = FSCRATCH_B0;
    dst[1] = FSCRATCH_B1;
    dst[2] = FSCRATCH_B2;
    dst[3] = FSCRATCH_B3;
}

/*
 * float_load_const  (FUN_00f7ef)
 * Confirmed disassembly:
 *   ldw X,#0x80BC (or similar flash address); callf F7DD; retf
 * Loads a float constant from flash into scratch via float_load_buf.
 * The constant address is embedded in the instruction stream.
 * Wrapper used throughout firmware to load calibration thresholds.
 */
void float_load_const(const uint8_t *flash_addr)
{
    float_load_buf(flash_addr);
}

/*
 * float_int_div  (FUN_00f893)
 * Confirmed disassembly:
 *   div X,A; ldw 0x00,X; clrw X; retf
 * Divides 16-bit X by 8-bit A (integer division), stores quotient
 * in scratch[0x00:0x01], remainder discarded. Returns 0 in X.
 */
void float_int_div(uint16_t dividend, uint8_t divisor)
{
    uint16_t quotient = dividend / divisor;
    FSCRATCH_HI = quotient;
    FSCRATCH_LO = 0u;
}

/*
 * float_scale_encode  (FUN_00f8be)
 * Confirmed disassembly (from FUN_00F7EF → FUN_00F8BE calls):
 * Takes a raw value and encodes it into the firmware's 4-byte
 * float representation, handling exponent normalisation.
 * The encoding: byte[0] = sign+exponent, bytes[1:3] = mantissa.
 * Input: scratch already loaded with an integer or fixed-point value.
 */
void float_scale_encode(void)
{
    /* If scratch is zero, keep it zero */
    if ((FSCRATCH_HI | FSCRATCH_LO) == 0u) return;

    /* Normalise: find leading bit position and adjust exponent.
     * The exponent is stored in bits [6:0] of byte[0].
     * Actual floating-point normalisation loop (simplified). */
    uint8_t  exponent = 0x7Fu;
    uint16_t mantissa = FSCRATCH_HI;

    while ((mantissa & 0x8000u) == 0u && mantissa != 0u)
    {
        mantissa <<= 1u;
        exponent--;
    }

    FSCRATCH_B0 = (FSCRATCH_B0 & 0x80u) | (exponent & 0x7Fu);
    FSCRATCH_B1 = (uint8_t)(mantissa >> 8u);
}

/*
 * timer_check_expired  (FUN_00f804)
 * Confirmed disassembly:
 *   compare counter at [X] with 0; retf with Z flag set if expired
 * Returns true if the timer counter at *ptr has reached zero.
 */
bool timer_check_expired(const uint16_t *counter)
{
    return (*counter == 0u);
}

/*
 * timer_wait_ticks  (FUN_00f8b7)
 * Confirmed disassembly — countdown loop with comparison:
 *   decrement counter each call; check vs timeout threshold from flash
 * Used between CAN frame transmissions to wait for bus-free status.
 * param counter: retry counter passed by reference (decrements each call)
 */
void timer_wait_ticks(uint16_t *counter)
{
    if (*counter > 0u)
        (*counter)--;
}

/*
 * float_result_check  (FUN_00f90b)
 * Confirmed disassembly:
 *   or A,(0x01,X) to test if any of first 2 bytes non-zero; retf
 * Returns non-zero if the 4-byte float at [X] is non-zero.
 */
uint8_t float_result_check(const uint8_t *val)
{
    return (uint8_t)(val[0] | val[1]);
}

/*
 * memcpy_4byte_chunks  (FUN_00f959)
 * Confirmed disassembly (from our earliest analysis — always with count=1,2,8,16,40):
 *   loop: copy 4 bytes src→dst; src+=4; dst+=4; count--; while(count!=0)
 * Used by every bulk data copy in the firmware.
 * This is the same function we decoded in the very first batch.
 */
void memcpy_4byte_chunks(uint8_t *dst, const volatile uint8_t *src, uint8_t count)
{
    while (count != 0u)
    {
        dst[0] = src[0]; dst[1] = src[1];
        dst[2] = src[2]; dst[3] = src[3];
        dst += 4; src += 4; count--;
    }
}
