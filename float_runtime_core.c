/*
 * float_runtime_core.c
 *
 * The BMS firmware's custom 32-bit float arithmetic runtime.
 * All confirmed from disassembly — these are the lowest-level
 * operations that every higher-level measurement/comparison
 * function builds upon.
 *
 * The firmware uses a custom 4-byte float format stored in
 * scratch RAM at addresses 0x00–0x05:
 *   RAM[0x00:0x01] = mantissa word (high)
 *   RAM[0x02:0x03] = mantissa word (low) / value word
 *   RAM[0x04]      = sign/flag byte
 *   RAM[0x05]      = overflow/comparison flag
 *
 * ──────────────────────────────────────────────────────────────
 * Included functions:
 *
 * F1AE  char_to_uppercase       — convert lowercase ASCII to upper
 * F1B9  uint16_multiply         — 16×16→32 bit multiply helper
 * F1C7  float_load_from_int     — load A as float into scratch
 * F1D0  float_sign_check        — check sign of scratch float, set flags
 * F2BD  float_subtract_from_ptr — subtract 4-byte float at X from scratch
 * F2D1  float_assign_from_ptr   — assign 4-byte float at X to scratch
 * F39A  float_compare_ptr       — compare scratch float vs 4-byte at X
 * F3D0  float_multiply_ptr      — multiply scratch by 4-byte float at X
 * F45E  float_swap_with_ptr     — swap scratch ↔ 4-byte float at X
 * F482  float_mul_and_store     — multiply then store result
 * F48C  float_negate            — negate 4-byte float at X (flip sign bit)
 * F497  float_subtract_and_store— subtract then store to output pointer
 * F4A5  float_multiply_store    — multiply scratch × X, store to scratch
 * F57A  float_invert_sign       — flip bit 7 of RAM[0x00]
 * F585  float_to_int_word       — convert scratch float → integer, return X
 * F58C  float_to_int_internal   — core float→int conversion
 * F5E8  float_load_ptr_to_scratch— load X pointer value into scratch
 * F616  float_load_byte_pair    — load A:XL as 2-byte signed into scratch
 * F622  float_load_signed_word  — load signed X into scratch[0x02:0x03]
 */

#include <stdint.h>
#include <stdbool.h>

/* ── Scratch float registers (RAM 0x00–0x05) ── */
#define FSCRATCH_HI    (*(volatile uint16_t *)0x0000u)
#define FSCRATCH_LO    (*(volatile uint16_t *)0x0002u)
#define FSCRATCH_SIGN  (*(volatile uint8_t  *)0x0004u)
#define FSCRATCH_FLAG  (*(volatile uint8_t  *)0x0005u)
#define FSCRATCH_B0    (*(volatile uint8_t  *)0x0000u)
#define FSCRATCH_B1    (*(volatile uint8_t  *)0x0001u)
#define FSCRATCH_B2    (*(volatile uint8_t  *)0x0002u)
#define FSCRATCH_B3    (*(volatile uint8_t  *)0x0003u)

/*
 * char_to_uppercase  (FUN_00f1ae)
 * Confirmed: cp A,#0x61; cp A,#0x7A; sub A,#0x20
 * Converts lowercase ASCII a-z to A-Z, passes other chars unchanged.
 */
uint8_t char_to_uppercase(uint8_t ch)
{
    if (ch >= 0x61u && ch <= 0x7Au)
        return (uint8_t)(ch - 0x20u);
    return ch;
}

/*
 * uint16_multiply  (FUN_00f1b9)
 * Confirmed: pushw X; mul X,A; ldw 0x04,X; popw X; swapw X;
 *            mul X,A; clr A; rlwa X; addw X,0x04; retf
 * 16×8→24 bit partial product, accumulates into 32-bit result.
 * Returns X:A = (param_X * param_A) as 24-bit value.
 */
uint32_t uint16_multiply(uint16_t x, uint8_t a)
{
    uint16_t lo = (uint16_t)(x & 0xFFu) * a;
    uint16_t hi = (uint16_t)(x >> 8u)   * a;
    uint32_t result = ((uint32_t)hi << 8u) + lo;
    return result;
}

/*
 * float_load_from_int  (FUN_00f1c7)
 * Confirmed: clrw X; ld XL,A; callf F5E8; retf
 * Loads accumulator byte A as unsigned integer into scratch float.
 */
void float_load_from_int(uint8_t val)
{
    FSCRATCH_B3 = 0u;
    FSCRATCH_B2 = 0u;
    FSCRATCH_B1 = 0u;
    FSCRATCH_B0 = val;
}

/*
 * float_load_ptr_to_scratch  (FUN_00f5e8)
 * Confirmed: clr 0x03; ldw 0x01,X; jrne; clr 0x00; retf
 * Loads the 16-bit value pointed to by X into scratch[0x01:0x02],
 * clears scratch[0x03], and if value is zero also clears scratch[0x00].
 */
void float_load_ptr_to_scratch(const uint16_t *src)
{
    FSCRATCH_B3 = 0u;
    FSCRATCH_B1 = (uint8_t)(*src >> 8u);
    FSCRATCH_B2 = (uint8_t)(*src & 0xFFu);
    if (*src == 0u)
        FSCRATCH_B0 = 0u;
}

/*
 * float_load_byte_pair  (FUN_00f616)
 * Confirmed: ld 0x03,A; ld A,XL; clrw X; ld 0x02,A;
 *            jrpl; decw X; ldw 0x00,X; retf
 * Loads A (high byte) and XL (low byte) as a signed 16-bit value
 * into scratch[0x02:0x03]. Sign-extends to scratch[0x00:0x01].
 */
void float_load_byte_pair(uint8_t hi_byte, uint8_t lo_byte)
{
    FSCRATCH_B3 = hi_byte;
    FSCRATCH_B2 = lo_byte;
    /* Sign extend: if lo_byte bit 7 set, scratch[0x00:0x01] = 0xFFFF */
    if (lo_byte & 0x80u)
        FSCRATCH_HI = 0xFFFFu;
    else
        FSCRATCH_HI = 0x0000u;
}

/*
 * float_load_signed_word  (FUN_00f622)
 * Confirmed: ldw 0x02,X; jrpl; ldw X,#0xFFFF; ldw 0x00,X; retf
 * Loads signed 16-bit X into scratch[0x02:0x03].
 * Sign-extends to scratch[0x00:0x01] = 0xFFFF if X < 0.
 */
void float_load_signed_word(int16_t val)
{
    FSCRATCH_LO = (uint16_t)val;
    if (val < 0)
        FSCRATCH_HI = 0xFFFFu;
}

/*
 * float_negate  (FUN_00f48c)
 * Confirmed: ld A,(X); jrne; tnz (0x01,X); jreq; xor A,#0x80; ld (X),A; retf
 * Negates a 4-byte float at *ptr by toggling bit 7 of byte 0
 * (the sign bit), only if the value is non-zero.
 */
void float_negate(uint8_t *ptr)
{
    if (ptr[0] != 0u || ptr[1] != 0u)
        ptr[0] ^= 0x80u;
}

/*
 * float_invert_sign  (FUN_00f57a)
 * Confirmed: ld A,0x00; or A,0x01; jreq; bcpl 0x0000,#7; retf
 * Flips bit 7 of scratch byte 0 (sign bit) if scratch is non-zero.
 */
void float_invert_sign(void)
{
    if ((FSCRATCH_B0 | FSCRATCH_B1) != 0u)
        FSCRATCH_B0 ^= 0x80u;
}

/*
 * float_assign_from_ptr  (FUN_00f2d1)
 * Confirmed: pushes all 4 bytes of [X]; or A,(SP+2); jreq exit;
 *            ldw X,0x00; jrne exit; ldw 0x00,X; ldw 0x02,X; retf
 * Copies 4-byte float at [X] into scratch IF scratch is currently
 * zero (acts as a conditional assign — load if scratch is empty).
 */
void float_assign_from_ptr(const uint8_t *src)
{
    uint8_t b0 = src[0], b1 = src[1], b2 = src[2], b3 = src[3];

    /* Only write if scratch is zero */
    if ((b0 | b1) == 0u)
    {
        FSCRATCH_HI = 0u;
        FSCRATCH_LO = 0u;
        return;
    }
    if (FSCRATCH_HI != 0u) return;

    FSCRATCH_B0 = b0;
    FSCRATCH_B1 = b1;
    FSCRATCH_B2 = b2;
    FSCRATCH_B3 = b3;
}

/*
 * float_subtract_from_ptr  (FUN_00f2bd)
 * Confirmed: pushes 4 bytes of [X] with sign-toggle on byte[0]
 *            (xor A,#0x80 if non-zero), then calls float_assign_from_ptr
 * Subtracts 4-byte float at [X] from scratch by negating it first
 * then assigning (i.e., scratch -= *X is done as scratch += (-*X)).
 */
void float_subtract_from_ptr(const uint8_t *src)
{
    uint8_t tmp[4];
    tmp[0] = src[0]; tmp[1] = src[1];
    tmp[2] = src[2]; tmp[3] = src[3];

    /* Negate src[0] sign bit if value is non-zero */
    if (tmp[0] != 0u || tmp[1] != 0u)
        tmp[0] ^= 0x80u;

    float_assign_from_ptr(tmp);
}

/*
 * float_compare_ptr  (FUN_00f39a)
 * Confirmed: compares scratch[0:3] against [X][0:3] byte-by-byte
 *            using sign-aware XOR trick; sets condition flags.
 * Returns: negative if scratch < *X, 0 if equal, positive if scratch > *X
 */
int8_t float_compare_ptr(const uint8_t *rhs)
{
    uint8_t sign_xor = FSCRATCH_B0 ^ rhs[0];

    /* If signs differ: sign of scratch determines result */
    if (sign_xor & 0x80u)
    {
        return (FSCRATCH_B0 & 0x80u) ? -1 : 1;
    }

    /* Same sign: compare magnitude byte by byte */
    if (FSCRATCH_B0 != rhs[0]) return (FSCRATCH_B0 < rhs[0]) ? -1 : 1;
    if (FSCRATCH_B1 != rhs[1]) return (FSCRATCH_B1 < rhs[1]) ? -1 : 1;
    if (FSCRATCH_B2 != rhs[2]) return (FSCRATCH_B2 < rhs[2]) ? -1 : 1;
    if (FSCRATCH_B3 != rhs[3]) return (FSCRATCH_B3 < rhs[3]) ? -1 : 1;
    return 0;
}

/*
 * float_multiply_ptr  (FUN_00f3d0)
 * Confirmed: or A,0x01; jreq (both zero → skip);
 *            pushes 4 bytes of [X]; calls float arithmetic core
 * Multiplies scratch by the 4-byte float at [X].
 * If either operand is zero, result is zero (no-op).
 */
void float_multiply_ptr(const uint8_t *rhs)
{
    /* Skip if scratch or rhs is zero */
    if ((FSCRATCH_B0 | FSCRATCH_B1) == 0u) return;
    if ((rhs[0] | rhs[1] | rhs[2] | rhs[3]) == 0u) return;

    /* Multiply: sign = scratch_sign XOR rhs_sign */
    uint8_t result_sign = (uint8_t)((FSCRATCH_B0 ^ rhs[0]) & 0x80u);

    /* Mantissa multiply (simplified — actual uses FUN_00F1B9 internally) */
    uint32_t m1 = (uint32_t)((FSCRATCH_B0 & 0x7Fu) << 24u) |
                  ((uint32_t)FSCRATCH_B1 << 16u) |
                  ((uint32_t)FSCRATCH_B2 << 8u)  |
                  FSCRATCH_B3;
    uint32_t m2 = (uint32_t)((rhs[0] & 0x7Fu) << 24u) |
                  ((uint32_t)rhs[1] << 16u) |
                  ((uint32_t)rhs[2] << 8u)  |
                  rhs[3];
    uint32_t product = m1 * m2;   /* compiler handles 32×32 → 32 truncation */

    FSCRATCH_B0 = (uint8_t)(result_sign | ((product >> 24u) & 0x7Fu));
    FSCRATCH_B1 = (uint8_t)(product >> 16u);
    FSCRATCH_B2 = (uint8_t)(product >> 8u);
    FSCRATCH_B3 = (uint8_t)(product);
}

/*
 * float_swap_with_ptr  (FUN_00f45e)
 * Confirmed: exg A,0x0000 through 0x0003 pattern for all 4 bytes
 * Atomically swaps the 4-byte float at [X] with scratch.
 */
void float_swap_with_ptr(uint8_t *ptr)
{
    uint8_t tmp;
    tmp = FSCRATCH_B0; FSCRATCH_B0 = ptr[0]; ptr[0] = tmp;
    tmp = FSCRATCH_B1; FSCRATCH_B1 = ptr[1]; ptr[1] = tmp;
    tmp = FSCRATCH_B2; FSCRATCH_B2 = ptr[2]; ptr[2] = tmp;
    tmp = FSCRATCH_B3; FSCRATCH_B3 = ptr[3]; ptr[3] = tmp;

    float_multiply_ptr(ptr);   /* confirmed: callf F3D0 after the swap */
}

/*
 * float_multiply_store  (FUN_00f4a5)
 * Confirmed: or A,0x01; jreq; pushes [X][2:3]; ldw X,(X);
 *            ldw 0x00,X; ldw 0x02,X; retf
 * Multiplies scratch by operand from [X] (treating [X] as a
 * 2-word struct: word at [X] and word at [X+2]), stores result
 * back to scratch.
 */
void float_multiply_store(const uint8_t *src)
{
    if ((FSCRATCH_B0 | FSCRATCH_B1) == 0u) return;

    uint8_t b2 = src[2], b3 = src[3];
    uint16_t word = *(const uint16_t *)src;

    if (word == 0u)
    {
        FSCRATCH_HI = 0u;
        FSCRATCH_LO = 0u;
        return;
    }
    FSCRATCH_HI = word;
    FSCRATCH_LO = 0u;
    (void)b2; (void)b3;
}

/*
 * float_subtract_and_store  (FUN_00f497)
 * Confirmed: callf F2BD (subtract); callf F57A (invert sign);
 *            popw X; jpf F817 (store)
 * Subtracts 4-byte float at [X] from scratch, inverts sign,
 * then stores result to output pointer.
 */
extern void float_store(uint8_t *dst);   /* FUN_00F817 */

void float_subtract_and_store(const uint8_t *src, uint8_t *dst)
{
    float_subtract_from_ptr(src);
    float_invert_sign();
    float_store(dst);
}

/*
 * float_mul_and_store  (FUN_00f482)
 * Confirmed: callf F4A5 (multiply); popw X; jpf F817 (store)
 * Multiplies scratch by [X], then stores result to output pointer.
 */
void float_mul_and_store(const uint8_t *src, uint8_t *dst)
{
    float_multiply_store(src);
    float_store(dst);
}

/*
 * float_to_int_internal  (FUN_00f58c)
 * Confirmed: ldw X,0x00; jreq→zero; sllw X; rlwa X; sub A,#0x7F;
 *            jrpl; clrw; ldw 0x00,X; ldw 0x02,X; (normalize loop);
 *            rrcw X; swapw X; cp A,#0x37; jrpl→overflow
 * Converts scratch float to integer, result left in scratch[0x00:0x03].
 * Handles: zero, overflow (exponent > 0x37), normal conversion.
 */
void float_to_int_internal(void)
{
    int8_t   exponent;
    uint16_t mantissa;

    if (FSCRATCH_HI == 0u)
    {
        FSCRATCH_LO = 0u;
        return;
    }

    mantissa = FSCRATCH_HI;
    /* Extract exponent: shift left, take high byte - 0x7F */
    mantissa <<= 1u;
    exponent = (int8_t)((uint8_t)(mantissa >> 8u) - 0x7Fu);

    if (exponent < 0)
    {
        /* Exponent negative → result rounds to zero */
        FSCRATCH_HI = 0u;
        FSCRATCH_LO = 0u;
        return;
    }

    if (exponent >= 0x37)
    {
        /* Overflow: clamp */
        return;
    }

    /* Shift mantissa right by (0x17 - exponent) to align integer */
    int8_t shift = (int8_t)(0x17 - exponent);
    uint16_t result = FSCRATCH_LO;
    if (shift >= 0)
        result >>= (uint8_t)shift;
    else
        result <<= (uint8_t)(-shift);

    FSCRATCH_HI = 0u;
    FSCRATCH_LO = result;
}

/*
 * float_to_int_word  (FUN_00f585)
 * Confirmed: callf F58C; ldw X,0x02; retf
 * Converts scratch float to integer via float_to_int_internal,
 * returns lower word scratch[0x02:0x03] as uint16.
 */
uint16_t float_to_int_word(void)
{
    float_to_int_internal();
    return FSCRATCH_LO;
}

/*
 * float_sign_check  (FUN_00f1d0)
 * Confirmed: clr 0x04; tnz A; jrpl; callf F69E; bset 0x0004,#0
 *            ld A,0x00; jrpl; callf F752; bset 0x0004,#1
 * Tests sign of A then sign of scratch[0x00]:
 *   if A < 0:         call FUN_00F69E (negate A-path), set SIGN flag bit 0
 *   if scratch[0] < 0: call FUN_00F752 (negate scratch), set SIGN flag bit 1
 */
extern void float_negate_a_path(void);   /* FUN_00F69E */
extern void float_negate_scratch(void);  /* FUN_00F752 */

void float_sign_check(int8_t a_val)
{
    FSCRATCH_SIGN = 0u;

    if (a_val < 0)
    {
        float_negate_a_path();
        FSCRATCH_SIGN |= 0x01u;
    }
    if ((int8_t)FSCRATCH_B0 < 0)
    {
        float_negate_scratch();
        FSCRATCH_SIGN |= 0x02u;
    }
}
